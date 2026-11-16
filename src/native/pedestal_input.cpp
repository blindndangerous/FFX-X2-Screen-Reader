#include "owned_nav_target.hpp"
#include "navigation_observations.hpp"
// The configured push action supplies consent; original scripts own movement.
#include "pedestal.hpp"
#include "pedestal_rvas.hpp"
#include "auto_walk.hpp"
#include "nav_audio.hpp"
#include "movement_keys.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include <atomic>
#include <cmath>
#include <algorithm>
namespace ffx {
#ifdef FFX_READER_TEST
uint64_t pedestalClock();
#endif
namespace {
uint64_t nowMs() {
#ifdef FFX_READER_TEST
    return pedestalClock();
#else
    return GetTickCount64();
#endif
}
constexpr uint64_t ApproachBudgetMs=30000,ContactProgressMs=15000;
struct Session {
    NativeRouteTarget target;
    UtilityScript script;
    PedestalPlan plan;
    unsigned epoch=0,segment=0,consent=0;
    std::array<unsigned,3> directions{};
    bool walk=false,held=false,owned=false,started=false,single=false,stopping=false,approaching=false,crossing=false,contact=false,committed=false,waitingShape=false;
    bool ended=false,walked=false,stalled=false;
    uintptr_t signal=0,pawn=0;
    uint64_t previous=0,progress=0,requested=0;
    uint64_t approachAt=0,contactProgress=0,diagnosticAt=0;
    double contactBest=-1;
    unsigned requests=0,failures=0,announcedSegment=~0u;
    unsigned walkInput=0,walkFailure=0; // Auto walk input revision after our own last stop.
    double touch=0; // Farthest off-side touch (in-game F27), as a radius.
    Point last,lastPlayer,pushFrom;

} active;
std::atomic<bool> armed{false},manual{false};
unsigned drain=0,injected=0,published=0;
std::atomic<unsigned> cancelRevision{1};
unsigned observedCancel=1;
thread_local uintptr_t interpreter=0;
thread_local unsigned interpreterEpoch=0;
// A touch by an ordinary auto walk (not a push) that the Kilika or Besaid
// pedestal script turned into a push start.
bool walkSeen=false,strayResume=false;
unsigned strayResumes=0,strayFrames=0;
constexpr unsigned StrayResumeFrames=20;
uint64_t strayAt=0;
// The learned touch outlives one assistance session, so pressing the walk
// key again after a stop keeps it, until the map changes.
struct {unsigned epoch=0;uintptr_t chunk=0;int worker=-1;double radius=0;} remembered;
// Stop only a walk that is running. stopAutoWalk also advances the manual
// input revision, which rejects the mod's own queued navigation actions
// (in-game F26: "navigation action rejected: manual input revision").
void haltAutoWalk() {if(autoWalkActive()) stopAutoWalk();active.walkInput=autoWalkInputRevision();active.walkFailure=autoWalkRouteFailureRevision();}
// Route around a learned off-side touch as a wider keep-out radius. The
// contact radius itself stays, so the final walk-in still reaches contact.
void learnedTouch(NativeRouteTarget& target) {
    if(active.touch>target.zone.contactRadius && target.zone.contactRadius>0) {
        target.zone.obstacle=target.zone.center; target.zone.obstacleRadius=active.touch;
    }
}
bool fieldControl() {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    return controller && (read<uint8_t>(controller)&2) && read<uintptr_t>(address(0xf00740));
}
void suppress(unsigned mask) {
    for(const auto rva:pedestal_rva::aggregates)
        write<uint16_t>(address(rva),static_cast<uint16_t>(read<uint16_t>(address(rva))&~mask));
}
void inject(unsigned bits) {
    // Replace held directions, not OR them: a stale second direction turns
    // the push diagonal, and the native push stops on 0.5 drift (klyt1000 2B07-2B2C).
    const auto held=read<uint16_t>(address(pedestal_rva::Held))&~(bits?pedestalInputMask(0xf000):0u);
    published|=bits&~held;
    write<uint16_t>(address(pedestal_rva::Held),static_cast<uint16_t>(held|bits));
    write<uint16_t>(address(pedestal_rva::Pressed),static_cast<uint16_t>(read<uint16_t>(address(pedestal_rva::Pressed))|(bits&~injected)));
    injected=bits;
}
void announcePush() {
    if(active.announcedSegment==active.segment) return;
    active.announcedSegment=active.segment;output(L"Pushing pedestal",false,"pedestal-push");
}
void stop(const char* reason,bool announce=false) {
    if(published) suppress(published);
    if(armed || active.epoch) {
        if(active.held) drain|=active.consent;
        haltAutoWalk(); recordLog("pedestal assistance stopped: %s",reason);
        if(announce) output(L"Pedestal assistance stopped",false,"pedestal-push");
    }
    armed=false;manual=false;active={};injected=published=0;publishPedestalConsent(0);
    // Only the fixed target/plan and qualified eight-character script carrier
    // remain. Resetting this Native-owned session needs no container retirement.
}
bool context() {
    if(!setting(Setting::AutoWalk) || !gameFocused() || scene || inBattle() || modMenuActive() || utilityConsoleActive()) return false;
    using Mode=int(__cdecl*)();
    return reinterpret_cast<Mode>(address(0x46c840))()==0;
}
unsigned pc(uintptr_t worker,const UtilityScript& script) {
    const auto signal=read<uintptr_t>(worker+0x90);
    const auto level=read<uint8_t>(worker+0x32);
    const auto type=read<uint8_t>(read<uintptr_t>(worker));
    if(!signal || level>((type==5 || type==6)?1u:8u) || (read<uint8_t>(signal+0xe)&15)!=level)
        return 0xffffffff;
    const auto code=script.chunk+script.codeOffset;
    const auto cursor=read<uintptr_t>(worker+0x12c+level*0x4c+0x18);
    return cursor>=code && cursor<code+script.length?static_cast<unsigned>(cursor-code):0xffffffff;
}
unsigned pc(uintptr_t worker) {return pc(worker,active.script);}
// The disable sites where the original contact push takes control:
// Besaid w7 entry 4 and Kilika w6 entry 4.
bool contactPushSite(unsigned control,unsigned at) {
    if(control==1) return at==0x12ab || at==0x139b || at==0x13c7 || at==0x13e1 || at==0x13ed;
    return control==2 && at==0x274b;
}
bool producer(uintptr_t worker,unsigned local,unsigned entry) {
    return worker && read<uintptr_t>(worker+4)==active.script.chunk && read<uint16_t>(worker+0x2e,0xffff)==local &&
        read<uint16_t>(read<uintptr_t>(worker+0x90)+8,0xffff)==entry;
}
bool ownsDisable(Event& event,bool requireHeld=true) {
    if(!armed || (requireHeld && !active.held) || active.epoch!=generation() || event.parentRva!=0x464180 || interpreterEpoch!=generation() ||
        !producer(interpreter,active.plan.producer,active.plan.entry)) return false;
    const auto at=pc(interpreter);
    switch(active.target.pushControl) {
    case 1: case 2: return contactPushSite(active.target.pushControl,at);
    case 3: return at==0xd17c;
    case 4: return at==0xe2ad || at==0xe44a;
    case 5: {
        const unsigned sites[]={0x14540,0x14700,0x148c0,0x14a80,0x14c40};
        return active.plan.producer>=56 && active.plan.producer<=60 && at==sites[active.plan.producer-56];
    }
    case 6: {
        const unsigned sites[]={0x747e,0x75f6,0x776e,0x78e6};
        return active.plan.producer>=44 && active.plan.producer<=47 && at==sites[active.plan.producer-44];
    }
    case 7: return at==0x1c3d3 || at==0x1c407;
    default: return false;
    }
}
bool needsHeldInput() {
    const auto worker=utilityWorker(active.script,active.plan.producer);
    if(!producer(worker,active.plan.producer,active.plan.entry)) return false;
    const auto at=pc(worker);
    switch(active.target.pushControl) {
    case 1: return at>=0x12ab && at<0x1b42;
    case 2: return at>=0x274b && at<0x2c17;
    case 3: return at>=0xd17c && at<0xd346;
    case 4: return at>=0xe2ad && at<0xe2d5;
    case 7: return active.plan.phase==1 && at>=0x1c3d3 && at<0x1c407;
    default: return false;
    }
}
void entered(Event& e) { e.before[0]=interpreter; e.before[1]=interpreterEpoch; interpreter=e.args[0]; interpreterEpoch=generation(); }
void left(Event& e) { interpreter=static_cast<uintptr_t>(e.before[0]); interpreterEpoch=static_cast<unsigned>(e.before[1]); }
// The original push pushes only after contact from an axis-aligned side:
// more than 2 units out along the push axis and under 2 across for Kilika
// (w6 entry 4, 27DF-28B9), 2.3 for Besaid (w7 entry 4, 11DD-121F). A
// walk-consent push claims only a contact on the planned side.
bool onPushSide() {
    if(active.segment>=active.plan.moves.size()) return false;
    PedestalSnapshot snapshot;if(!capturePedestalSnapshot(active.script,active.target,snapshot)) return false;
    const auto goal=active.plan.moves[active.segment].goal,p=snapshot.position,player=playerPosition();
    const double gx=goal.x-p.x,gz=goal.z-p.z,x=player.x-p.x,z=player.z-p.z;
    const double edge=active.target.pushControl==1?2.3:2;
    // Claim only a contact well inside the native limit across the axis. In
    // game F26 a contact near the corner took control but never pushed, and
    // the walk-in staging point is within 1 unit of the axis anyway.
    const double across=edge-.5;
    if(std::abs(gx)>=std::abs(gz)) return (gx>0?-x:x)>edge && std::abs(z)<across;
    return (gz>0?-z:z)>edge && std::abs(x)<across;
}
// An ordinary auto walk (to a sphere recess or past the pedestal) touched a
// Kilika or Besaid pedestal, and its script took control (in-game F25).
// Give the script no direction so the pedestal stays put, and resume the
// walk when control returns. A manual touch keeps the player's direction.
void strayTouch(Event& e) {
    if(!walkSeen || e.parentRva!=0x464180 || interpreterEpoch!=generation() || !interpreter) return;
    for(unsigned control:{1u,2u}) {
        UtilityScript script;if(!pedestalScript(control,script)) continue;
        if(read<uintptr_t>(interpreter+4)!=script.chunk || read<uint16_t>(interpreter+0x2e,0xffff)!=(control==1?7u:6u) ||
            read<uint16_t>(read<uintptr_t>(interpreter+0x90)+8,0xffff)!=4 || !contactPushSite(control,pc(interpreter,script))) continue;
        suppress(pedestalInputMask(0xf000)); haltAutoWalk();
        const auto now=nowMs();if(now-strayAt>10000) strayResumes=0;
        strayAt=now; strayResume=strayResumes<2; ++strayResumes; strayFrames=0;
        recordLog("pedestal touched by auto walk: %s",strayResume?"resuming the walk":"walk not resumed");
        return;
    }
}
void disabled(Event& e) {
    if(!armed) {try {strayTouch(e);} catch(...) {} return;}
    if(ownsDisable(e) && (!active.walk || active.owned || onPushSide())) {
        active.signal=read<uintptr_t>(interpreter+0x90); active.owned=active.started=true; active.contact=false; haltAutoWalk();
        active.pushFrom=active.target.position;
        PedestalSnapshot contact;if(capturePedestalSnapshot(active.script,active.target,contact)) active.pushFrom=contact.position;
        active.progress=nowMs();
        {const auto player=playerPosition();
         recordLog("pedestal push contact: player offset %.2f %.2f, segment %u",player.x-active.pushFrom.x,player.z-active.pushFrom.z,active.segment);}
        // The same interpreter reads its direction immediately after disabling
        // control. Replace approach input before that read, not next frame.
        if(active.target.pushControl<=2) {
            suppress(pedestalInputMask(0xf000)|active.consent);
            inject(active.directions[active.segment]);
        }
        if(active.target.pushControl==7 && pc(interpreter)==0x1c407) {
            active.committed=true; if(published) suppress(published); published=0;
        }
    }
    else if((!active.held || active.walk) && !active.owned && ownsDisable(e,false)) {
        // Auto walk touched the pedestal before the push key was held, and the
        // original push script took control by itself. Give it no direction so
        // the pedestal stays put, and stay armed for the real held push.
        haltAutoWalk(); suppress(pedestalInputMask(0xf000));
        if(active.walk) {
            // In-game F27: on Kilika's reset tile every walk touched the
            // pedestal well outside the modelled contact. Keep later walks
            // outside this touch.
            const auto player=playerPosition();const double out=distanceXZ(player,active.target.position);
            if(out<15) {active.touch=std::max(active.touch,out+.3);remembered={active.epoch,active.script.chunk,active.target.worker,active.touch};}
            if(active.contact) {active.target.zone.valid=false;active.waitingShape=true;nav::requestNavigationScene();}
            else learnedTouch(active.target);
            active.approaching=active.contact=active.walked=false;
            recordLog("pedestal contact off the push side: walking to it, player offset %.2f %.2f",player.x-active.target.position.x,player.z-active.target.position.z);
        }
        else recordLog("pedestal contact before the push key: waiting for it");
    }
    else stop("unrelated player-control disable");
}
// A Kilika or Besaid push ends when control returns. If the pedestal is not
// at the segment goal, the next frame walks back out and in again (in-game
// F26: holding the direction against the pedestal for 18 s did nothing).
void restored(Event&) {
    if(!armed) return;
    if(active.owned && active.walk && active.target.pushControl<=2) {
        active.ended=true;
        try {recordLog("pedestal push ended at pc %x",pc(utilityWorker(active.script,active.plan.producer)));} catch(...) {}
    }
    active.owned=false;
}
void text(Event&) { stop("dialogue"); }
void collected(Event&) { published=0; }
void requestStarted(Event& e) {
    if(!armed || active.epoch!=generation() || read<uintptr_t>(e.args[0]+4)!=active.script.chunk) return;
    const auto local=read<uint16_t>(e.args[0]+0x2e,0xffff),entry=read<uint16_t>(e.args[1]+8,0xffff);
    if(active.target.pushControl==7 && active.plan.phase==0 && local==39 && entry==6) stop("entrance rail push launched");
    else if(active.held && (active.target.pushControl==5 || active.target.pushControl==6) &&
        local==active.plan.producer && entry==active.plan.entry) active.started=true;
}
void finishing(Event& e) {
    e.before[0]=0;
    if(!armed || active.epoch!=generation()) return;
    using Worker=uintptr_t(__cdecl*)(int);
    const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(static_cast<int>(e.args[0]));
    e.before[0]=read<uintptr_t>(worker+4); e.before[1]=read<uint16_t>(worker+0x2e,0xffff);
    e.before[2]=read<uint16_t>(read<uintptr_t>(worker+0x90)+8,0xffff);
}
void finished(Event& e) {
    if(!armed || !active.started || active.epoch!=generation() || static_cast<uintptr_t>(e.before[0])!=active.script.chunk) return;
    const unsigned local=static_cast<unsigned>(e.before[1]),entry=static_cast<unsigned>(e.before[2]);
    if(active.target.pushControl==7 && active.plan.phase==1 && local==38 && entry==15 && !active.committed) {
        active.started=active.owned=false; active.signal=0; return;
    }
    const bool motion=active.target.pushControl==4 && local==1 && (entry==10 || entry==12);
    const bool once=(active.target.pushControl==5 || active.target.pushControl==6 || active.target.pushControl==7) &&
        local==active.plan.producer && entry==active.plan.entry;
    if(motion || once) {
        output(active.target.pushControl==6?L"Pedestal activated":L"Pedestal push complete",false,"pedestal-push");
        stop("native push completed");
    }
}
bool approach() {
    const auto player=playerPosition();
    if(!active.target.zone.crossing && inInteractionZone(active.target.zone,player)) {
        haltAutoWalk(); active.approaching=active.walked=false; active.requests=0; faceNavigationTarget(active.target); return true;
    }
    if(autoWalkActive()) {active.approaching=active.walked=true; return false;}
    // The player stopped the walk (Backslash again, or any movement or
    // menu key). stopAutoWalk and physical input both advance the input
    // revision; our own stops record it. Do not walk again on our own.
    if(active.approaching && autoWalkRouteFailureRevision()!=active.walkFailure) {stop("approach route lost");return false;}
    if(active.approaching && autoWalkInputRevision()!=active.walkInput) {stop("approach walk stopped by the player");return false;}
    // Ask again when the walk ended short of the zone (another hook stopped
    // it at a touch), or when a request never started a walk.
    const auto now=nowMs();
    if(active.approaching && !active.walked && now-active.requested<1500) return false;
    if(active.requests>=8) {
        output(active.stalled?L"Auto walk stopped: no progress":L"Pedestal push route unavailable. Walk to the push side by hand.",false,"pedestal-push");
        stop("approach walk failed");return false;
    }
    ++active.requests; active.walked=false; active.requested=now;
    active.approaching=requestNavigationAssistRoute(); active.walked=autoWalkActive(); return false;
}
void freshTarget(Point position) {
    const auto previous=active.target.position;
    active.target.position=position;
    if(active.crossing) return;
    // Known trial staging/contact shapes are rigid offsets from this actor.
    // Only horizontal movement is retained until the next owned observation;
    // vertical changes or a turn require a fresh qualified shape.
    if(std::abs(position.y-previous.y)>.1) {active.target.zone.valid=false;active.waitingShape=true;}
    else {
        const Point delta{position.x-previous.x,0,position.z-previous.z};
        for(auto* point:{&active.target.zone.center,&active.target.zone.end,&active.target.zone.obstacle}) {point->x+=delta.x;point->z+=delta.z;}
    }
    nav::requestNavigationScene();

}
void resetApproach() {
    active.approachAt=active.contactProgress=active.diagnosticAt=0;active.contactBest=-1;
}
bool monitorApproach(const PedestalSnapshot& snapshot,Point player,uint64_t now) {
    if(active.target.pushControl>2 || active.owned || active.started) return true;
    if(!active.approachAt) active.approachAt=active.contactProgress=now;
    const auto goal=active.plan.moves[active.segment].goal,p=snapshot.position;
    const double gx=goal.x-p.x,gz=goal.z-p.z;
    const Point side=std::abs(gx)>=std::abs(gz)?Point{gx>0?-1.0:1.0,0,0}:Point{0,0,gz>0?-1.0:1.0};
    const auto& zone=active.target.zone;
    const double radius=active.contact?zone.range+.15:zone.contactRadius;
    const Point contact{p.x+side.x*std::max(0.0,radius-.25),p.y,p.z+side.z*std::max(0.0,radius-.25)};
    const double distance=distanceXZ(player,contact);
    // Keep the best contact distance across staging, replans and off-side touches.
    if(active.contactBest<0 || distance<active.contactBest-1) {active.contactBest=distance;active.contactProgress=now;}
    const bool deadline=now-active.approachAt>=ApproachBudgetMs,stalled=now-active.contactProgress>=ContactProgressMs;
    if(!active.diagnosticAt || now-active.diagnosticAt>=1000 || deadline || stalled) {
        active.diagnosticAt=now;NavLeg leg;const bool routed=navLeg(leg);
        const auto waypoint=routed?leg.goal:contact;
        recordLog("pedestal approach: elapsed %llu ms, idle %llu ms, segment %u, phase %s, requests %u, distance %.3f, best %.3f, route %u",
            now-active.approachAt,now-active.contactProgress,active.segment,active.waitingShape?"pending":active.contact?"contact":"staging",active.requests,distance,active.contactBest,routed?1u:0u);
        recordLog("pedestal approach geometry: player %.3f %.3f %.3f, pedestal %.3f %.3f %.3f, waypoint %.3f %.3f %.3f, along %.3f, across %.3f, radius %.3f",
            player.x,player.y,player.z,p.x,p.y,p.z,waypoint.x,waypoint.y,waypoint.z,
            (player.x-p.x)*side.x+(player.z-p.z)*side.z,(player.x-p.x)*side.z-(player.z-p.z)*side.x,radius);
    }
    if(!deadline && !stalled) return true;
    output(L"Auto walk stopped: no progress",false,"pedestal-push");
    stop(deadline?"approach deadline":"no contact progress");return false;
}
}
PedestalNavigationState capturePedestalNavigationState() {
    PedestalNavigationState s;if(!nativeGameThread() || !armed || active.epoch!=generation()) return s;
    s.armed=true;s.epoch=active.epoch;s.control=active.target.pushControl;s.local=active.target.pushLocal;
    s.id=active.target.id;s.worker=active.target.worker;s.held=active.held;s.contact=active.contact;s.crossing=active.crossing;s.touch=active.touch;
    if(active.segment>active.plan.moves.size()) return {};
    s.plan=active.plan;s.plan.moves.erase(s.plan.moves.begin(),s.plan.moves.begin()+active.segment);return s;
}
bool pedestalRetryNoProgress() {
    if(!nativeGameThread() || !armed || active.epoch!=generation() || !active.held || !active.approaching || active.owned || active.started) return false;
    active.stalled=true;return true;
}
void cancelPedestal() {cancelRevision.fetch_add(1,std::memory_order_acq_rel);if(nativeGameThread()) {observedCancel=cancelRevision.load(std::memory_order_acquire);stop("selection or context changed");}else {armed=false;manual=true;publishPedestalConsent(0);}}
bool pedestalActiveCrossing(const Entity& target) {
    return armed && active.epoch==generation() && active.crossing && target.pushControl==7 &&
        target.pushLocal==active.target.pushLocal && target.worker==active.target.worker && target.id==active.target.id;
}
bool pedestalActiveContact(const Entity& target) {
    return armed && active.epoch==generation() && active.contact && target.pushControl==active.target.pushControl &&
        target.pushLocal==active.target.pushLocal && target.worker==active.target.worker && target.id==active.target.id;
}
bool pedestalActivePlan(const Entity& target,PedestalPlan& plan) {
    if(!armed || active.epoch!=generation() || target.pushControl!=active.target.pushControl ||
        target.pushLocal!=active.target.pushLocal || target.worker!=active.target.worker || target.id!=active.target.id) return false;
    plan=active.plan; plan.moves.erase(plan.moves.begin(),plan.moves.begin()+active.segment); return true;
}
void armPedestalOwned(const NativeRouteTarget& target,const UtilityScript& script,const PedestalPlan& plan,bool walk) {
    if(!nativeGameThread()) return;
    stop("new target");
    observedCancel=cancelRevision.load(std::memory_order_acquire);
    if(!target.pushControl || !setting(Setting::AutoWalk)) return;
    try {
        const auto consent=plan.moves.empty()?0:plan.moves.front().mode==PushMode::Single?pedestalConfirmMask():0x40;
        if(plan.complete || !plan.blocked.empty() || plan.moves.empty() || !target.zone.valid || target.inactive || !consent) return;
        active.target=target;active.script=script;active.plan=plan; active.epoch=generation(); active.consent=consent; active.walkInput=autoWalkInputRevision();
        active.walkFailure=autoWalkRouteFailureRevision();
        if(remembered.epoch==active.epoch && remembered.chunk==script.chunk && remembered.worker==target.worker) active.touch=remembered.radius;
        learnedTouch(active.target);
        active.walk=active.held=walk && plan.walkConsent;
        unsigned index=0;for(const auto& move:plan.moves) {
            const auto bits=pedestalInputMask(move.held);
            if(move.held && pedestalLogicalInput(bits)!=move.held) {stop("invalid inverse input mapping");return;}
            active.directions[index++]=bits;
        }
        active.pawn=read<uintptr_t>(address(0xf00740));
        if(!active.pawn || active.pawn!=playerActor() || !read<uint16_t>(active.pawn+2)) {stop("controlled actor unavailable");return;}
        if(!publishPedestalConsent(consent)) {stop("consent bindings unavailable");recordLog("pedestal consent bindings unavailable");return;}
        active.last=target.position; active.lastPlayer=playerPosition(); active.progress=nowMs();
        if(active.held) active.approachAt=active.contactProgress=active.progress;
        armed=true;
    }catch(...) {stop("unavailable verified state");}
}
void armPedestal(const Entity& target,bool walk) {
    UtilityScript script;if(!pedestalScript(target.pushControl,script)) {stop("new target");return;}
    try {OwnedNavTarget copied;if(!captureOwnedNavTarget(target,copied)) return;const auto plan=currentPedestalPlan(target);armPedestalOwned(copied.native,script,plan,walk);}catch(...) {stop("unavailable verified state");}
}
void updatePedestalOwnedTarget(const NativeRouteTarget& target) {
    if(!nativeGameThread() || !armed || active.epoch!=generation() || target.worker!=active.target.worker || target.id!=active.target.id || target.pushControl!=active.target.pushControl || target.pushLocal!=active.target.pushLocal) return;
    // A worker may return a shape captured before the Native owner changed phase.
    if(target.pushControl<=2 && target.zone.valid && active.contact!=(target.zone.contactRadius==0)) {
        nav::requestNavigationScene();return;
    }
    active.target=target; learnedTouch(active.target);
    if(target.zone.valid && !target.inactive) active.waitingShape=false;
}
void updatePedestalInput() {
    const auto revision=cancelRevision.load(std::memory_order_acquire);if(revision!=observedCancel) {observedCancel=revision;stop("selection or context changed");}
    const auto raw=read<uint16_t>(address(pedestal_rva::Held));
    const auto drained=drain; drain&=raw; if(drained) suppress(drained);
    if(strayResume && (armed || autoWalkActive())) strayResume=false; // Another walk or a push took over.
    // Control comes back at 2F70 (Kilika), but field interaction only after
    // a further wait(5) (2F76-2F82). Until then the pedestal reads inactive
    // and a new walk to it is refused (in-game round 9, step 8 and step 11).
    if(strayResume && fieldControl() && ++strayFrames>=StrayResumeFrames) {
        strayResume=false;
        // Step 8 via the plain Pedestal row: the walk had arrived, but the
        // touch moved the player out of reach, so facing fails. Route again.
        NavLeg leg;if(navLeg(leg) && leg.arrived) {if(!faceNavigationTarget(leg.target)) nav::submitNavigationQuery(nav::Query::SilentPath);}
        else startAutoWalk(false);
    }
    walkSeen=!armed && autoWalkActive();
    if(!armed) return;
    const bool wasHeld=active.held;
    active.held=active.walk || (raw&active.consent)!=0;
    if(!active.held) {
        if(wasHeld || active.owned || active.started || active.single || active.stopping) { stop("push action released"); return; }
        active.approaching=autoWalkActive(); return;
    }
    suppress(active.consent);
    const auto logical=pedestalLogicalInput(raw&~active.consent);
    if(logical&0x0fff) { stop("manual native action"); return; }
    if(active.epoch!=generation() || !context() || manual.exchange(false)) { stop("context or manual input"); return; }
    const auto now=nowMs();
    if(active.previous && now-active.previous>600) { stop("input interrupted"); return; }
    active.previous=now;
    try {
        UtilityScript current;
        if(!pedestalScript(active.target.pushControl,current) || current.chunk!=active.script.chunk) { stop("map changed"); return; }
        PedestalSnapshot snapshot;if(!capturePedestalSnapshot(active.script,active.target,snapshot)) {stop("unavailable native push facts");return;}
        const auto live=pedestalPlan(active.target.pushControl,active.target.pushLocal,snapshot);
        if(distanceXZ(snapshot.position,active.last)>.1) { active.last=snapshot.position; active.progress=now; }
        const auto player=playerPosition();
        if(distanceXZ(player,active.lastPlayer)>.1) { active.lastPlayer=player;if(active.target.pushControl>2) active.progress=now; }
        if(!monitorApproach(snapshot,player,now)) return;
        if(now-active.progress>15000 && !(active.approaching && !active.owned && !active.started)) {
            const auto waypoint=active.plan.moves[active.segment].goal;
            recordLog("pedestal no progress: player %.3f %.3f %.3f, pedestal %.3f %.3f %.3f, waypoint %.3f %.3f %.3f",
                player.x,player.y,player.z,snapshot.position.x,snapshot.position.y,snapshot.position.z,waypoint.x,waypoint.y,waypoint.z);
            stop("no progress",true); return;
        }
        if(live.complete && active.target.pushControl<=3) {
            output(L"Pedestal push complete",false,"pedestal-push"); stop("destination reached"); return;
        }
        if(!live.blocked.empty() && (!active.started || active.plan.moves[active.segment].mode==PushMode::Slide)) {
            output(live.blocked.data(),false,"pedestal-push");stop("state guard changed");return;
        }
        const bool walking=autoWalkActive();
        if((logical&0xf000) && !walking && !active.approaching) { stop("manual directional input"); return; }
        auto& move=active.plan.moves[active.segment];
        if(move.mode==PushMode::Slide && distanceXZ(snapshot.position,move.goal)<.8) {
            active.stopping=true;
            if(!fieldControl()) return; // Native ending animation owns control.
            if(active.segment+1==active.plan.moves.size()) {
                output(L"Pedestal push complete",false,"pedestal-push"); stop("destination reached"); return;
            }
            ++active.segment; active.started=active.stopping=active.contact=active.approaching=active.ended=false; injected=0;
            active.requests=active.failures=0;
            resetApproach();
            freshTarget(snapshot.position);
            active.target.zone.valid=false;
            active.waitingShape=true;
            requestNavigationAssistInstruction();
        }
        if(active.owned) {
            const auto worker=utilityWorker(active.script,active.plan.producer);
            if(read<uintptr_t>(worker+0x90)!=active.signal || !producer(worker,active.plan.producer,active.plan.entry)) {
                stop("native push signal changed"); return;
            }
            if(!active.stopping && needsHeldInput()) inject(active.directions[active.segment]);
            return;
        }
        if(!fieldControl()) { stop("player control unavailable"); return; }
        if(active.ended) {
            active.ended=false;
            const double moved=distanceXZ(snapshot.position,active.pushFrom);
            recordLog("pedestal push stopped short: moved %.2f (east %.2f, north %.2f), player offset %.2f %.2f",moved,
                snapshot.position.x-active.pushFrom.x,snapshot.position.z-active.pushFrom.z,player.x-snapshot.position.x,player.z-snapshot.position.z);
            // In-game round 9: fourteen pushes in a row moved it under 1
            // unit each. A push that short counts toward the cap as well.
            if(moved<1.5 && ++active.failures>=3) {
                output(moved<.3?L"Pedestal push did not start. Walk into the pedestal from the push side by hand.":
                    L"Pedestal push keeps stopping after a short move. Walk into the pedestal from the push side by hand.",false,"pedestal-push");
                stop(moved<.3?"push did not start":"push keeps stopping short"); return;
            }
            if(moved>=1.5) {active.failures=0;resetApproach();}
            // Plan the rest from where the pedestal is now. A push can end
            // past its goal or off the axis, and the plan made at arm time
            // then names the wrong side (in game: "Push south 1.5 steps").
            if(!live.moves.empty() && live.moves.size()<=active.directions.size()) {
                unsigned index=0;std::array<unsigned,3> directions{};bool valid=true;
                for(const auto& next:live.moves) {
                    const auto bits=pedestalInputMask(next.held);
                    if(next.held && pedestalLogicalInput(bits)!=next.held) valid=false;
                    directions[index++]=bits;
                }
                if(valid) {active.plan=live;active.segment=0;active.directions=directions;}
            }
            // Walk back to the push side and in again: the original script
            // starts a push only on a fresh contact.
            if(published) suppress(published);
            injected=published=0;
            active.started=active.contact=active.approaching=active.walked=false; active.requests=0;
            freshTarget(snapshot.position);
            active.target.zone.valid=false; active.waitingShape=true;
            return;
        }
        if(active.started && move.mode==PushMode::Scripted) return;
        freshTarget(snapshot.position);
        if(active.waitingShape) return; // Only synthetic assistance waits for copied facts; original processing continues.
        if(active.target.inactive || !active.target.zone.valid) { stop("push target inactive",true); return; }
        if(!approach()) return;
        const auto& next=active.plan.moves[active.segment];
        if(active.target.pushControl<=2 && !active.started) {
            if(!active.contact) {
                const auto radius=active.target.zone.contactRadius,height=active.target.zone.height<0?1e30:active.target.zone.height;
                const auto side=active.target.zone.approach;
                const auto minY=active.target.zone.contactMinY,maxY=active.target.zone.contactMaxY;
                active.target.zone={};active.target.zone.shape=1;active.target.zone.center=active.target.position;
                active.target.zone.range=radius-.15;active.target.zone.height=height;active.target.zone.valid=std::isfinite(radius) && radius>.5;
                active.target.zone.approach=side;
                active.target.zone.contactMinY=minY;active.target.zone.contactMaxY=maxY;
                if(!active.target.zone.valid) {stop("contact target unavailable",true);return;}
                active.contact=true;
                announcePush();
                approach();
            }
            return;
        }
        if(next.mode==PushMode::Crossing) {
            if(!active.crossing) {
                nav::requestNavigationScene();active.target.zone.valid=false;active.waitingShape=true;
                active.crossing=true; approach();
            }
            return;
        }
        if(next.mode==PushMode::Single) {
            if(!active.single) { inject(active.consent&(~active.consent+1u)); active.single=true; }
        } else {
            if(!active.started && !injected) announcePush();
            inject(active.directions[active.segment]);
        }
    } catch (...) { stop("unreadable native push state"); }
}
void pedestalPlayerInput(unsigned vk) {
    if(armed && !pedestalConsentVk(vk) && (vk==VK_ESCAPE || vk==VK_UP || vk==VK_DOWN || vk==VK_LEFT || vk==VK_RIGHT ||
        vk=='W' || vk=='A' || vk=='S' || vk=='D')) manual=true;
}
void pedestalKeyboardInput(const uint8_t* keys) {
    if(!armed) return;
    for(unsigned scan:{0x11u,0x1eu,0x1fu,0x20u,0xc8u,0xd0u,0xcbu,0xcdu,0x01u})
        if((keys[scan]&0x80) && !pedestalConsentKey(scan)) manual=true;
    for(unsigned scan=1;scan<256;++scan) if((keys[scan]&0x80) && pedestalMovementKey(scan)) manual=true;
}
void pedestalControllerInput(const pad::State& state) {
    if(!armed) return;
    const auto consent=pedestalConsentButtons();
    if((state.held&~consent) || std::abs(int(state.lx))>=5000 || std::abs(int(state.ly))>=5000 ||
        std::abs(int(state.rx))>=5000 || std::abs(int(state.ry))>=5000 ||
        (state.lt>=30 && !(consent&pad::bit(pad::LT))) || (state.rt>=30 && !(consent&pad::bit(pad::RT)))) manual=true;
}
void installPedestal() {
    observedCancel=cancelRevision.load(std::memory_order_acquire);
    hook(0x471d10,collected,nullptr);
    hook(pedestal_rva::Interpreter,entered,left);
    hook(0x471980,disabled,nullptr); hook(0x471a50,nullptr,restored);
    hook(0x469040,nullptr,requestStarted); hook(0x469170,finishing,finished);
    hook(0x4b8980,text,nullptr);
}
#ifdef FFX_READER_TEST
bool pedestalTargetForTest(NativeRouteTarget& target) {if(!nativeGameThread() || !armed || active.epoch!=generation()) return false;target=active.target;return true;}
#endif
}
