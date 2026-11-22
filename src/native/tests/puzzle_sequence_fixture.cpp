#include "fixture_runtime.hpp"
#include "../puzzle_guide_owned.hpp"
#include "../puzzle_guide_bindings.hpp"
#include "../utility_trials_data.hpp"
#include "../navigation_intents.hpp"
#include "../navigation_targets.hpp"
#include "../utility_memory.hpp"
#include "../settings.hpp"
#include "../events.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <set>

// Ordered owned observations, not an ATEL emulator. Link the production guide,
// scene/zone decoder, pedestal rules and geometry helpers. Only external resource,
// audio, input and route-service boundaries are replaced here. Expected actions
// below are independent of puzzleRoutes/puzzleTiles: never import either table.
// Sources: research-puzzles-repo.md 79-147, 234-249, 261-297 and the four r25
// reports in the 2026-11-17 research scratchpad; exact ATEL/log anchors accompany
// each sequence. Original scripts, logs and collision assets stay outside Git.
namespace {
using namespace ffx;
AtelIdentity identity;
const char* sequence="setup";
const char* eventName="setup";
unsigned checks=0,events=0,failures=0,xfails=0;
bool strict=false;
bool observedTarget=false,boardingPath=false,boardingStarted=false;
Entity routedTarget;

void require(bool ok,const char* why) {
    ++checks;
    if(!ok) {++failures;std::fprintf(stderr,"FAIL: %s / %s: %s\n",sequence,eventName,why);}
}
[[noreturn]] void unexpected(const char* why) {
    require(false,why);std::exit(1);
}
// An expected failure is one particular invariant at one named event. It cannot
// swallow a wrong step, unrelated invariant, exception, crash, or setup failure.
// XPASS is a failure: remove the annotation when the production repair lands.
void invariant(bool ok,const char* why,const char* known=nullptr) {
    if(!known || strict) {require(ok,why);return;}
    ++checks;
    if(ok) {++failures;std::fprintf(stderr,"XPASS: %s / %s: %s\n",sequence,eventName,known);}
    else {++xfails;std::printf("XFAIL: %s / %s: %s (%s)\n",sequence,eventName,known,why);}
}
std::wstring joined(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring out;for(const auto& part:parts) {if(!out.empty()) out+=separator;out+=part;}return out;
}
}
namespace ffx {
bool nativeGameThread() {return false;}
// Link-only boundaries for unused native capture/scanner functions in the real
// observation unit. Any accidental call fails; none reads a game process.
uintptr_t address(unsigned) {unexpected("native address lookup");}
bool copy(uintptr_t,void*,size_t) {unexpected("native memory read");}
unsigned generation() {return identity.span.epoch;}
bool hook(unsigned,Handler,Handler) {unexpected("native hook registration");}
bool inBattle() {unexpected("native battle read");}
std::atomic<int> scene{0};
Point playerPosition() {unexpected("native player read");}
uintptr_t playerActor() {unexpected("native actor read");}
bool cameraBasis(Point&,Point&) {unexpected("native camera read");}
bool nativeAtelSpan(uintptr_t,AtelSpan&) {unexpected("native ATEL read");}
bool readyAtelWorkerHeader(uintptr_t,unsigned,unsigned&) {unexpected("native worker read");}
AtelResult scanWorker(const AtelScript&,unsigned,const std::vector<unsigned>*) {unexpected("unprepared script scan");}
AtelPreparation atelPreparation(uintptr_t) {unexpected("unprepared ATEL lookup");}
bool OwnedAtelGuardView::callbackAvailable(unsigned local,unsigned,const AtelSavedFacts&) const {return local==38 || local==53;}
bool visitReadyAtelGuardedScript(const AtelSpan&,OwnedAtelGuardConsumer visit,void* arg) {
    AtelScript script;script.name="bvyt1100";script.codeLength=identity.length;script.checksum=identity.checksum;
    script.workers.resize(54);for(auto& w:script.workers) w.header=~0u;
    script.workers[38].header=script.workers[53].header=0;visit({script,nullptr},arg);return true;
}
void prepareCopiedControlSnapshot(CopiedControls&) {unexpected("native input capture");}
bool captureUtilityValue(const UtilityScript&,unsigned,unsigned,unsigned,int&) noexcept {unexpected("native utility capture");}
PedestalNavigationState capturePedestalNavigationState() {unexpected("native pedestal capture");}
unsigned pedestalConfirmMask() {unexpected("native Confirm lookup");}
std::wstring navigationEntityName(const NavigationTarget*,unsigned,bool,const std::function<std::wstring(int)>&) {unexpected("unprepared entity scan");}
unsigned navigationEntityCategory(const NavigationTarget*,unsigned,bool) {unexpected("unprepared category scan");}
bool navigationSaveSphereModel(unsigned) {unexpected("unprepared model scan");}
bool navigationActorPresence(const NavigationScript*,unsigned,bool) {return false;}
const NavigationScript* navigationCatalog(const AtelScript&) {static NavigationScript catalog{};return &catalog;}
const NavigationTarget* navigationTarget(const NavigationScript*,unsigned) {return nullptr;}
const wchar_t* navigationChestName(const NavigationTarget*) {unexpected("unprepared chest scan");}
std::wstring navigationExitName(const AtelScript&,unsigned,int,const std::wstring&,const std::function<std::wstring(int)>&) {unexpected("unprepared exit scan");}
bool butterflyNavigationLeaving(const AtelSpan&) {unexpected("unprepared butterfly scan");}
bool modMenuActive() {unexpected("native menu read");}
bool gameFocused() {unexpected("live focus read");}
bool utilityConsoleActive() {unexpected("native console read");}
void refreshSwimmingGoal(Entity&) {unexpected("live swimming read");}
void preparePedestalZone(Entity&,uintptr_t) {unexpected("live pedestal read");}
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& out) {out=identity;return chunk==identity.span.chunk;}
// No prepared ATEL script is needed for the observation-to-guide contract. This
// explicitly declines optional pedestal-plan advice rather than inventing it.
bool visitReadyAtelScript(const AtelSpan&,OwnedAtelConsumer,void*) {return false;}
OwnedTextContext copiedControlContext(const CopiedControls&) {
    OwnedTextContext out;out.glyphs[0x31]=L"C";out.glyphs[0x32]=L"X";return out;
}
std::wstring join(const std::vector<std::wstring>& p,const wchar_t* s) {return joined(p,s);}
std::wstring direction(Point) {if(boardingPath) return L"toward the boarding position";unexpected("unexpected route formatting");}
void resetNavAudio() {}
void routeNavAudio(const std::vector<Point>&,const Entity& target,bool assisted) {
    require(boardingPath && assisted && target.worker==routedTarget.worker && target.id==routedTarget.id,"boarding audio route retains the scanned identity");
}
int setting(Setting key) {if(boardingPath) return key==Setting::AutoWalk;unexpected("unexpected settings request");}
std::vector<Point> navigationRouteOwned(const Mesh&,Point player,const Entity& target,const std::vector<Entity>&) {
    observedTarget=true;routedTarget=target;
    boardingPath=target.name==L"Rail pedestal boarding position";boardingStarted=false;
    if(boardingPath) return {player,target.position};
    // Capture other production targets, then decline traversal. The oracle
    // checks evidenced level access, NOT a fabricated successful walking path.
    return {};
}
namespace nav {
bool submitNavigationIntent(const Scene&,const Entity& target,Intent intent,unsigned,unsigned,bool) {
    if(!boardingPath) unexpected("unexpected navigation intent");
    boardingStarted=intent==Intent::Start && target.id==routedTarget.id && target.worker==routedTarget.worker;
    return boardingStarted;
}
}
}

namespace {
enum class Floor {Entrance,UpperRail,FinalPlatform,BesaidCorridor,Zanarkand};
struct Write {unsigned binding;int value;unsigned high=1,element=0;};
struct Channel {unsigned worker,channels;};
struct Expect {
    unsigned step;
    const wchar_t* target; // Empty means no actionable target, e.g. a message.
    int worker=-1;        // Native worker, -1 for a floor panel, -2 for a rail action without walking.
    Point point{};
    bool panel=false;
};
struct Event {
    const char* name;
    std::vector<Write> writes;
    std::vector<Channel> channels;
    Point player;
    Floor floor;
    Expect expected;
    bool control=true,message=false,visible=false;
    int window=0;
    unsigned completion=0;
    bool reset=false,reopen=false,newEpoch=false,pathFirst=false;
};

class Replay {
    std::unique_ptr<nav::Scene> scene=std::make_unique<nav::Scene>();
    unsigned previousStep=0;
    std::set<unsigned> rewardWorkers;
    unsigned treasureMask=0;
    const char* script;
public:
    explicit Replay(const char* name,const char* resource):script(resource) {
        sequence=name;eventName="setup";resetOwnedPuzzleGuide();
        scene->epoch=1;scene->primary=0x10000;scene->partCount=1;scene->control=true;scene->confirmMask=0x20;
        for(const auto& trial:trialUtilities) if(!std::strcmp(trial.script,script)) {
            identity={};identity.span={scene->primary,scene->epoch,1,0x100};
            identity.length=trial.length;identity.checksum=trial.checksum;std::memcpy(identity.name,script,8);
        }
        require(!std::strcmp(identity.name,script),"verified trial identity exists");
        auto& part=scene->parts[0];part.span=identity.span;part.bindingsReady=part.guideReady=true;scene->actorCount=1;scene->playerIndex=0;
        // Capture schema only; it is not used to derive expected steps or panels.
        const nav::GuideBinding* bindings=nullptr;unsigned count=0;
        if(!std::strcmp(script,"bvyt1100")) {
            for(const unsigned low:{0x0c00001fu,0x0c000020u,0x0c000024u}) part.bindings.rows[part.bindings.count++]={low,0,1,0};
            bindings=nav::guide_bvyt1100;count=_countof(nav::guide_bvyt1100);treasureMask=16;}
        if(!std::strcmp(script,"bsyt0100")) {bindings=nav::guide_bsyt0100;count=_countof(nav::guide_bsyt0100);treasureMask=1;}
        if(!std::strcmp(script,"dome0700")) {bindings=nav::guide_dome0700;count=_countof(nav::guide_dome0700);treasureMask=32;}
        require(bindings && part.bindings.count+count<=part.bindings.rows.size(),"bounded guide capture schema");
        for(unsigned i=0;i<count;++i) part.bindings.rows[part.bindings.count++]={bindings[i].low,0,bindings[i].high,bindings[i].element};
    }
    void write(const Write& w) {
        for(auto& row:scene->parts[0].bindings.rows) if(row.binding==w.binding && row.high==w.high && row.element==w.element) {row.value=w.value;return;}
        unexpected("event writes a binding absent from the capture schema");
    }
    int value(unsigned binding) const {
        for(const auto& row:scene->parts[0].bindings.rows) if(row.binding==binding) return row.value;
        unexpected("oracle binding absent");
    }
    nav::WorkerFact& worker(unsigned local) {
        for(unsigned i=0;i<scene->count;++i) if(scene->workers[i].local==local) return scene->workers[i];
        auto& w=scene->workers[scene->count];w.global=scene->count++;w.valid=true;w.part=0;w.local=local;w.type=2;return w;
    }
    void positioned(unsigned local,Point p) {
        auto& w=worker(local);w.position=p;w.type=3;w.interactions.channels=1;
        const auto field=[&](unsigned offset,float v) {std::memcpy(w.geometry.data()+offset,&v,sizeof(v));};
        field(0,static_cast<float>(p.x));field(4,static_cast<float>(p.y));field(8,static_cast<float>(p.z));
        field(0x1c,20);field(0x30,20);field(0x38,20);field(0x40,20);
    }
    void railWindow(unsigned local,Point p) {
        auto& w=worker(local);w.type=3;w.position=p;w.interactions.channels=8;w.interactions.entries[3]=5;
        const auto field=[&](unsigned offset,float v) {std::memcpy(w.geometry.data()+offset,&v,sizeof(v));};
        field(0,static_cast<float>(p.x));field(4,static_cast<float>(p.y));field(8,static_cast<float>(p.z));
        field(0x1c,20);field(0x30,20);field(0x38,20);field(0x48,20);
    }
    void boarding(unsigned entry,Point p) {
        positioned(38,p);auto& w=worker(38);w.interactions.channels=8;w.interactions.entries[3]=entry;
        const float width=7,depth=4;
        std::memcpy(w.geometry.data()+0x30,&width,4);std::memcpy(w.geometry.data()+0x38,&depth,4);
    }
    void seed(std::initializer_list<Write> writes) {for(const auto& w:writes) write(w);}
    std::vector<Entity> refreshedTargets() {
        std::vector<Entity> targets;
        for(unsigned i=0;i<scene->count;++i) if(scene->workers[i].local==38 && !std::strcmp(script,"bvyt1100")) {
            require(nav::scanNavigationScene(*scene,targets,{},{}),"owned boarding refresh succeeds");break;
        }
        return targets;
    }
    std::wstring query(OwnedGuideAction action,unsigned local=0) {
        boardingPath=false;boardingStarted=false;Mesh mesh;mesh.openPlane=true;
        return puzzleGuideOwned(action,*scene,mesh,refreshedTargets(),speechq::Header{},0,0,local,scene->primary,1);
    }
    void run(const Event& e) {
        ++events;eventName=e.name;
        if(e.newEpoch) {++scene->epoch;identity.span.epoch=scene->epoch;scene->parts[0].span=identity.span;}
        if(e.reopen) resetOwnedPuzzleGuide();
        for(const auto& w:e.writes) write(w);
        for(const auto& c:e.channels) {auto& w=worker(c.worker);w.interactions.channels=c.channels;w.interactions.entries[5]=2;}
        scene->player=e.player;scene->control=e.control;scene->message=e.message;scene->visibleMessage=e.visible;scene->window=e.window;
        scene->capturedAt=GetTickCount64();
        if(e.completion) require(query(OwnedGuideAction::TileCompleted,e.completion).empty(),"tile receipt alone must not claim a reward");
        if(e.pathFirst) {
            observedTarget=false;query(OwnedGuideAction::Path);
            require(observedTarget && routedTarget.worker>=0 && scene->workers[routedTarget.worker].local==43,
                "first command after close routes to W43 before Expected or a completion refresh");
        }
        const auto text=query(OwnedGuideAction::Expected);
        const auto stepAt=text.find(L"Step ");
        const unsigned step=stepAt==std::wstring::npos?0:static_cast<unsigned>(std::wcstoul(text.c_str()+stepAt+5,nullptr,10));
        if(step!=e.expected.step) std::fwprintf(stderr,L"Observed speech: %ls\n",text.c_str());
        require(step==e.expected.step,"spoken step agrees with the evidence trace");
        require(!previousStep || step>=previousStep || e.reset,"no step regression without an explicit game reset");
        previousStep=step;
        const bool hasTarget=e.expected.target && *e.expected.target;
        if(hasTarget) {
            if(text.find(e.expected.target)==std::wstring::npos) std::fwprintf(stderr,L"Observed speech: %ls\n",text.c_str());
            require(text.find(e.expected.target)!=std::wstring::npos,"spoken target agrees with the evidence trace");
        } else if(!e.visible) require(text.find(L"Wait for")!=std::wstring::npos,"native transition requests waiting instead of a target");
        // Strict contract: a close hint does not excuse an earlier imperative.
        // R23's repeated Confirm removed/reinserted the sphere (research 119-123;
        // Docs/cloisters.md's diagnostic acknowledgement warning).
        if(e.visible) {
            const auto close=text.find(L"Press Enter to close the message");
            require(close!=std::wstring::npos,"visible message requests one close");
            require(close==std::wstring::npos || text.find(L"Press Enter to close the message",close+1)==std::wstring::npos,"one close hint only");
            bool action=false;
            for(const auto verb:{L"Take ",L"Insert ",L"Push ",L"Open ",L"Retrieve ",L"Ride ",L"Step onto ",L"Activate ",L"Touch "})
                action=action || text.find(verb)!=std::wstring::npos;
            invariant(!action,"message speech contains no competing puzzle action");
        } else require(text.find(L"Press Enter to close the message")==std::wstring::npos,"closed message never requests another Confirm");

        // Every event checks completion/claim invariants, including repeated reads.
        if(value(0x01000a70)&treasureMask) rewardWorkers.insert(!std::strcmp(script,"dome0700")?32u:16u);
        if(!std::strcmp(script,"bvyt1100") && value(0x0c00002a)) rewardWorkers.insert(17);
        require(text.find(L"Obtained")==std::wstring::npos,"guide does not manufacture a reward claim");
        require(!(value(0x01000a70)&treasureMask) || text.find(L"Target: Destruction Sphere treasure")==std::wstring::npos,"collected reward is never spoken as the next chest action");
        require(!rewardWorkers.count(17) || text.find(L"Target: Second treasure chest")==std::wstring::npos,"optional reward is never requested twice");
        observedTarget=false;
        const auto pathText=query(OwnedGuideAction::Path);
        const bool railOnly=e.expected.worker==-2;
        if(railOnly) require(!observedTarget && pathText.find(e.expected.target)!=std::wstring::npos,"Path gives the native ride action without walking");
        if(hasTarget && !railOnly) require(observedTarget || !e.control,"production path resolves an actionable target or native rail input owns it");
        if(observedTarget && e.expected.worker==38 && value(0x0c000009)==15) {
            require(routedTarget.position.z>=267.5 && routedTarget.position.z<=269.6,"W38:15 target inside the E15 Z band");
            require(pathText.find(L"Hold Left")!=std::wstring::npos,"W38:15 names held Left at the upper platform stop");
        } else if(observedTarget && e.expected.worker==38) {
            require(boardingStarted,"boarding intent starts for the same identity that survives scene refreshes");
            require(routedTarget.position.x>=24 && routedTarget.position.x<27.5,"W38:9 target inside X activation band");
            require(pathText.find(L"Hold Left")!=std::wstring::npos,"W38:9 names held Left at boarding (E9 tests c04c(15) at 1bd15)");
            const auto id=routedTarget.id;const auto index=routedTarget.worker;
            for(unsigned delay:{16u,600u}) {
                scene->capturedAt+=delay;
                const auto fresh=refreshedTargets();
                require(std::any_of(fresh.begin(),fresh.end(),[&](const Entity& target) {
                    return target.id==id && target.worker==index && target.actor==routedTarget.actor &&
                        distanceXZ(target.position,routedTarget.position)<.01;
                }),"boarding remains identifiable through scene refreshes spanning the route loss timeout");
            }
        }
        if(observedTarget) {
            const int local=routedTarget.worker<0?-1:static_cast<int>(scene->workers[routedTarget.worker].local);
            if(hasTarget) require(local==e.expected.worker,"production path targets the expected native worker");
            if(e.expected.panel) require(distanceXZ(routedTarget.position,e.expected.point)<.01 && std::abs(routedTarget.position.y-e.expected.point.y)<.01,"production path selects the independently recorded panel coordinate");
            // Level reachability is a necessary contract, not a collision proof.
            // Bevelle live bug G: entrance y=179.993 cannot reach upper route
            // waypoint y=264.833 on foot (15:20:04,15:23:10,15:24:05).
            // Final-platform W37 is accessible only after the ascending ride.
            bool reachable=false;
            if(!std::strcmp(script,"bvyt1100")) {
                if(e.floor==Floor::Entrance) reachable=std::abs(e.player.y-179.993)<.01 && local==13;
                if(e.floor==Floor::UpperRail) reachable=std::abs(e.player.y-264.809)<.01 && (local==30 || local==31 || local==13 || local==38) && std::abs(routedTarget.position.y-e.player.y)<1;
                if(e.floor==Floor::FinalPlatform) reachable=std::abs(e.player.y+453.177)<.01 && (local==16 || local==37 || local==13 || local==17 || local==56 || local==53 || local==38);
            }
            // bsyt0100 W43/W44 share the opened sphere corridor. dome0700's
            // main floor (y5) and pedestal corridor (y0) connect on foot. These
            // coarse component checks cannot certify safe avoidance of panels.
            if(e.floor==Floor::BesaidCorridor) reachable=(local==43 || local==44) && e.player.y>=38 && e.player.y<=45 && routedTarget.position.y>=38 && routedTarget.position.y<=45;
            if(e.floor==Floor::Zanarkand) reachable=(local==-1 || local==46) && e.player.y>=0 && e.player.y<=5 && routedTarget.position.y>=0 && routedTarget.position.y<=5;
            invariant(reachable,"target is reachable from the current evidenced floor/transport state");
            require(local<0 || !rewardWorkers.count(static_cast<unsigned>(local)),"no repeated chest target or duplicate reward request after collection");
            if(e.expected.panel) {
                // W57..83 entry 2 turns off the accepted touch shape. W38:9
                // rearms it after reset. Do not infer eligibility from receipts.
                const unsigned panel=static_cast<unsigned>(std::wcstoul(std::wcsstr(e.expected.target,L"panel ")+6,nullptr,10))+55;
                require(worker(panel).interactions.channels!=0,"no stale completed-panel target");
            }
        } else {
            require(!hasTarget || railOnly || pathText.find(L"confirm the rail direction")!=std::wstring::npos,"no missing target hidden by a generic error");
        }
        if(!hasTarget) {
            invariant(!observedTarget,"message-open or settling guide cannot dispatch a stale interaction target");
            if(e.visible) require(pathText==text,"Path and Expected give only the same close instruction");
        }
        const auto refresh=query(OwnedGuideAction::Completed);
        const auto repeat=query(OwnedGuideAction::Completed);
        require(repeat.empty(),"duplicate completion refresh produces no second cue or reward claim");
        require(refresh.find(L"Obtained")==std::wstring::npos,"completion refresh cannot claim a native reward");
    }
};

constexpr Point entrance{-37.972,179.993,-175.351};
constexpr Point upper{-77.4,264.809,84.6};
constexpr Point finalPlatform{-50.2,-453.177,263.1};
const Expect rail{1,L"Target: Rail pedestal",13};
const Expect upperSource{2,L"Target: Upper Bevelle Sphere source",30};
const Expect upperRide{2,L"Ride the rail pedestal.",-2};
const Expect platform{12,L"Target: Upper platform sphere recess",37};

void bevelleSeed(Replay& r) {
    // Native initial sphere places: upper source4, lower source6, Glyph7,
    // Destruction9. Each sphere has twelve matrix columns (element=row*12+place).
    r.seed({{0x0c00001f,1},{0x0d000032,1,0xc0048,28},{0x0d000032,1,0xc0048,42},
            {0x0d000032,1,0xc0048,55},{0x0d000032,1,0xc0048,69}});
}
void bevelleEntrance() {
    // bevelle-r25-live.md: 15:15:09 trial collect, HP Sphere, one Enter,
    // Step 12 at entrance. Collection did not perform the ascent.
    Replay r("Bevelle entrance collection (G)","bvyt1100");
    bevelleSeed(r);
    r.positioned(13,{-20,179.993,-200});r.positioned(37,finalPlatform);
    r.run({"before collect",{}, {},entrance,Floor::Entrance,rail});
    Event after{"collect returned without transport",{{0x01000a70,16}}, {},entrance,Floor::Entrance,rail};
    r.run(after);
    after.name="repeat request still at entrance";after.writes.clear();r.run(after);
}

void bevelleRide() {
    // bvyt1100 W13 4b00/4c7d: node 1 then 19. Normal ascent precedes
    // W54:6 -> W16:8 collection; child 1370f..1379a grants HP Sphere.
    // bevelle-r25-fix-report.md: W54 Common005D at 1ce0a restores control.
    // This stream samples settled transport endpoints, NOT every intermediate
    // lower-rail action. Direct collection on the upper rail does not perform
    // W54's movement/ascent either; that is another instance of bug G.
    Replay r("Bevelle normal ride and rail collection","bvyt1100");
    bevelleSeed(r);
    r.positioned(13,{-20,179.993,-200});r.positioned(30,upper);r.positioned(37,finalPlatform);
    r.run({"entrance",{}, {},entrance,Floor::Entrance,rail});
    r.run({"normal rail departure",{{0x0c000008,19}}, {},upper,Floor::UpperRail,upperRide});
    Event collected{"direct collection on upper rail",{{0x01000a70,16}}, {},upper,Floor::UpperRail,upperRide};
    r.run(collected);
    r.railWindow(43,upper);
    r.run({"collection restored control at active junction",{{0x0c00000c,1}}, {},upper,Floor::UpperRail,{2,L"Press C now to turn left.",-2}});
    // W38 may still have its old shape at the opposite stop on the same level.
    r.boarding(8,{-60.2,264.8,84.6});
    r.run({"stale boarding shape across gap",{{0x0c000008,22},{0x0c000009,3}}, {{43,0}}, {13.6,264.809,102.3},Floor::UpperRail,upperRide});
    r.boarding(8,{19.8,264.8,84.6});
    r.run({"wrong boarding callback at correct stop",{}, {},{13.6,264.809,102.3},Floor::UpperRail,upperRide});
    r.worker(38).interactions.entries[3]=9;
    r.run({"unpowered pedestal cannot board",{{0x0c00001f,0}}, {},{13.6,264.809,102.3},Floor::UpperRail,upperRide});
    r.run({"native boarding latch occupied",{{0x0c000020,1},{0x0c000024,1}}, {},{13.6,264.809,102.3},Floor::UpperRail,upperRide});
    r.seed({{0x0c000024,0}});
    r.run({"opposite upper stop requires boarding",{{0x0c000008,22},{0x0c000009,3}}, {{43,0}}, {13.6,264.809,102.3},Floor::UpperRail,{2,L"Ride the rail pedestal.",38}});
    r.boarding(8,{-60.2,264.8,84.6});
    r.run({"source stop reached",{{0x0c000008,17},{0x0c000009,1}}, {},upper,Floor::UpperRail,upperSource});
    eventName="17/1 west boarding band for the next objective";++events;
    r.query(OwnedGuideAction::Next);observedTarget=false;
    const auto sourcePath=r.query(OwnedGuideAction::Path);
    require(observedTarget && routedTarget.worker>=0 && routedTarget.position.x>=-68 && routedTarget.position.x< -62 &&
        !inInteractionZone(routedTarget.zone,{-60.2,264.8,84.6}),"W38:8 routes inside the activation band and excludes the rectangle center");
    require(boardingStarted && sourcePath.find(L"Hold Down")!=std::wstring::npos,"W38:8 names held Down at the source stop");
    r.query(OwnedGuideAction::Expected);
    r.worker(38).interactions.channels=0;
    // W33 Destruction placement 0x26 precedes the final ascent. 0x25 retains
    // upper power; sphere 3 is carried (element37), lower socket44 is empty.
    r.run({"final route unlocked before ascent",{{0x0c000025,1},{0x0c000026,1},{0x0d000032,1,0xc0048,37},
        {0x0d000032,0,0xc0048,28},{0x0d000032,0,0xc0048,42},{0x0d000032,0,0xc0048,55},
        {0x0d000032,1,0xc0048,59},{0x0d000032,0,0xc0048,69}}, {},upper,Floor::UpperRail,{11,L"Ride the final ascending rail",-2}});
    r.run({"ascent not settled on node alone",{{0x0c000008,64}}, {},upper,Floor::UpperRail,{11,L"Ride the final ascending rail",-2}});
    r.run({"later native ascending rail arrival",{{0x0c000009,14}}, {},finalPlatform,Floor::FinalPlatform,platform});
    r.run({"repeat after reward",{}, {},finalPlatform,Floor::FinalPlatform,platform});
    r.positioned(13,{-20,-453.177,263.1});
    // W38 E14 at stop 14 is empty; W53 (enabled by W54 at the chest) calls
    // the pedestal to stop 15, where W38 E15 boards it with Left at Z>=267.
    r.railWindow(53,{-18.3,-453.13,302.85});r.worker(53).interactions.channels=0x20;r.worker(53).interactions.entries[5]=5;
    r.run({"optional sphere inserted",{{0x0c000029,1},{0x0d000032,0,0xc0048,37}}, {},finalPlatform,Floor::FinalPlatform,{13,L"Target: Rail pedestal call point",53}});
    r.worker(53).interactions.channels=0;
    r.boarding(15,{-18.7,-453.2,263.1});
    {auto& w=r.worker(38);const float width=4,depth=7;std::memcpy(w.geometry.data()+0x30,&width,4);std::memcpy(w.geometry.data()+0x38,&depth,4);}
    r.run({"pedestal called to the upper platform stop",{{0x0c000009,15},{0x0c00001f,1}}, {},finalPlatform,Floor::FinalPlatform,{13,L"Board the called pedestal",38}});
    r.worker(38).interactions.channels=0;
    r.positioned(17,{-19.375,-454.658,118.552});
    r.run({"optional ride launched",{{0x0c000009,16}}, {},finalPlatform,Floor::FinalPlatform,{14,L"Target: Second treasure chest",17}});
    r.positioned(56,{-18.675,-453.129,383.513});
    r.run({"ride grants optional reward",{{0x0c000008,66},{0x0c00002a,1},{0x0c000009,17}}, {},finalPlatform,Floor::FinalPlatform,{15,L"Target: Exit to the antechamber",56}});
    r.run({"completed repeat",{}, {},finalPlatform,Floor::FinalPlatform,{15,L"Target: Exit to the antechamber",56}});
}

// Coordinates are literal dome0700 worker positions, independently checked from
// the three PUSHF operands immediately before native 0x57 in entry0:
// W66 b146, W70 b78a, W73 bedd, W74 c1d4, W76 c5b2, W78 c8ba, W83 d0e7.
// Never read the guide's permitted-tile table to derive expected coordinates.
// ATEL SHA256 a530da97346bc9d9bee90ab5239663d283d128d13203fe3e55bc4e308c1d29c6.
struct Panel {unsigned worker;Point point;const wchar_t* label;};
const Panel panels[]={
    {66,{-5,5,75},L"Next permitted floor panel: Floor panel 11:"},
    {70,{-25,5,45},L"Next permitted floor panel: Floor panel 15:"},
    {73,{-55,5,65},L"Next permitted floor panel: Floor panel 18:"},
    {74,{-95,5,85},L"Next permitted floor panel: Floor panel 19:"},
    {76,{-85,5,-75},L"Next permitted floor panel: Floor panel 21:"},
    {78,{-105,5,35},L"Next permitted floor panel: Floor panel 23:"},
    {83,{-155,5,-55},L"Next permitted floor panel: Floor panel 28:"}
};
Expect panelExpectation(unsigned i) {return {4,panels[i].label,-1,panels[i].point,true};}
void zanarkandSeed(Replay& r) {
    r.seed({{0x20000a00,2768},{0x5c000004,2},{0x0c000022,6},{0x0c000020,1}});
    for(unsigned w=57;w<=83;++w) {auto& row=r.worker(w);row.type=6;row.interactions.channels=0x20;row.interactions.entries[5]=2;}
    r.positioned(46,{140,0,25});
}
void zanarkandRecovery() {
    // Live FFX_SR_Native.log 2026-11-17 16:31:34.065 W70: count1,
    // quotas1/0/0/0, channels0; 16:31:49.119 W69: count0, quotas0/0/0/0,
    // channels0x20. Earlier 16:29:55.099 -> 16:30:09.720 drops 4 -> 0.
    // Recovery uses an ATEL-valid seven-panel order: all 66/76/78/83, two
    // 70/74/79 and one 68/72/73. W38:24 62f1..6343 completes at seven.
    Replay r("Zanarkand pattern 2 wrong-panel reset and seven-panel recovery","dome0700");zanarkandSeed(r);
    r.run({"pattern 2 active",{}, {},panels[1].point,Floor::Zanarkand,panelExpectation(1)});
    Event accepted{"W70 accepted",{{0x0c000024,1},{0x0c000025,1}},{{70,0}},panels[2].point,Floor::Zanarkand,panelExpectation(2)};accepted.completion=70;r.run(accepted);
    Event reset{"W69 wrong panel resets",{{0x0c000024,0},{0x0c000025,0},{0x0c000026,0},{0x0c000027,0},{0x0c000028,0}},{{70,0x20},{69,0x20}},panels[1].point,Floor::Zanarkand,panelExpectation(1)};reset.completion=69;reset.reset=true;r.run(reset);
    Event busy{"native reset handback still owns control",{}, {},panels[1].point,Floor::Zanarkand,{4,L""}};busy.control=false;r.run(busy);
    // No fixture-side quota evaluator: every resulting native write is explicit.
    const std::vector<std::vector<Write>> results={
        {{0x0c000024,1}},
        {{0x0c000024,2},{0x0c000025,1}},
        {{0x0c000024,3},{0x0c000027,1}},
        {{0x0c000024,4},{0x0c000026,1}},
        {{0x0c000024,5}},{{0x0c000024,6}},
        {{0x0c000024,7},{0x0c000020,2},{0x5c000004,0},{0x0c000025,0},{0x0c000026,0},{0x0c000027,0}}
    };
    r.run({"recovery first panel",{}, {},panels[0].point,Floor::Zanarkand,panelExpectation(0)});
    for(unsigned i=0;i<_countof(panels);++i) {
        const auto next=i+1<_countof(panels)?panelExpectation(i+1):Expect{5,L"Target: Pattern pedestal 3",46};
        Event e{"accepted recovery panel",results[i],{{panels[i].worker,0}},i+1<_countof(panels)?panels[i+1].point:Point{140,0,25},Floor::Zanarkand,next};
        e.completion=panels[i].worker;r.run(e);
    }
    r.run({"completed pattern repeat",{}, {},{140,0,25},Floor::Zanarkand,{5,L"Target: Pattern pedestal 3",46}});
}

void besaidMessage() {
    // R23: research 119-123 and Docs/cloisters.md:316. One Confirm closes
    // the recess result; another can remove the just-inserted sphere. ATEL
    // bsyt0100 W44 writes hand0/pedestal1; W43 is the next Destruction pickup.
    Replay r("Besaid recess message, one Confirm, fresh query","bsyt0100");
    r.seed({{0x5c0000ec,1},{0x5c0000d4,1},{0x5c0000d8,1},{0x5c0000dc,1},{0x5c0000e4,1},{0x5c0000ac,1}});
    r.positioned(44,{-14.576,40.241,-62.599});r.positioned(43,{95.172,38.249,3.972});
    const Point p{-14.576,40.241,-60};
    r.run({"carrying Besaid Sphere",{}, {},p,Floor::BesaidCorridor,{9,L"Target: Pedestal sphere recess",44}});
    Event open{"insert result message open",{{0x5c0000ac,0},{0x5c0000cc,1}}, {},p,Floor::BesaidCorridor,{9,L""}};
    open.control=false;open.message=true;open.visible=true;open.window=6;
    r.run(open);
    open.name="visible message outlasts control and window flags";open.control=true;open.message=false;open.window=0;open.writes.clear();r.run(open);
    Event closed{"one Confirm closes without toggling sphere",{}, {},p,Floor::BesaidCorridor,{10,L"Target: Destruction Sphere",43}};
    closed.pathFirst=true;r.run(closed);
    r.run({"fresh query without another Confirm",{}, {},p,Floor::BesaidCorridor,{10,L"Target: Destruction Sphere",43}});
    require(r.value(0x5c0000cc)==1 && r.value(0x5c0000ac)==0,"single-close replay leaves pedestal sphere inserted");
}

void coalescedReset() {
    // W38:9 (5af7..5b0f) clears count/quotas and rearms collision. A reset
    // and a subsequent success between captures can leave count unchanged.
    // research 103-109, 240-243; zanarkand-r25-fix-report.md, equal-count case.
    Replay r("Zanarkand coalesced reset and late guide activation","dome0700");zanarkandSeed(r);
    r.run({"first observed panel",{}, {},panels[0].point,Floor::Zanarkand,panelExpectation(0)});
    Event one{"W66 accepted",{{0x0c000024,1}},{{66,0}},panels[4].point,Floor::Zanarkand,panelExpectation(4)};one.completion=66;r.run(one);
    Event hidden{"reset then W76 between captures (count stays one)",{},{{66,0x20},{76,0}},panels[0].point,Floor::Zanarkand,panelExpectation(0)};hidden.reset=true;r.run(hidden);
    hidden.name="late guide activation uses current channels";hidden.reopen=true;hidden.channels.clear();r.run(hidden);
}

void missedRailObservation() {
    // bvyt1100 W0 f4 -> node0, W13 4b00 ->1, 4c7d ->19. Node1 can
    // disappear before a ready observation (research 81-87). A saved treasure
    // bit alone is not evidence of traversal in the newly initialized room.
    Replay r("Bevelle missed departure and late activation","bvyt1100");
    bevelleSeed(r);
    r.seed({{0x01000a70,16}});r.positioned(13,{-20,179.993,-200});r.positioned(30,upper);
    r.run({"old treasure bit at fresh entrance",{}, {},entrance,Floor::Entrance,rail});
    r.railWindow(40,upper);
    Event prompt{"node1 coalesced, node19 Confirm window",{{0x0c000008,19}}, {},upper,Floor::UpperRail,{2,L"Press C now to start.",-2}};prompt.control=false;r.run(prompt);
    r.run({"ride settled",{{0x0c000008,17},{0x0c000009,1}},{{40,0}},upper,Floor::UpperRail,upperSource});
    Event late{"guide opened after departure",{}, {},upper,Floor::UpperRail,upperSource};late.reopen=true;r.run(late);
    Event roomReset{"new room epoch keeps old reward but resets transport",{{0x0c000008,0},{0x0c000009,0}}, {},entrance,Floor::Entrance,rail};roomReset.reset=true;roomReset.newEpoch=true;r.run(roomReset);
}
}
int main(int argc,char** argv) {
    ffx_fixture::initializeRuntime();strict=argc>1 && !std::strcmp(argv[1],"--strict");
    bevelleEntrance();bevelleRide();zanarkandRecovery();besaidMessage();coalescedReset();missedRailObservation();
    std::printf("%s: puzzle sequence replay: 6 sequences, %u events, %u checks, %u XFAIL, %u unexpected failures.\n",failures?"FAIL":"PASS",events,checks,xfails,failures);
    return failures?1:0;
}
