#include "nav_audio.hpp"
#include "floor_panels.hpp"
#include "auto_walk.hpp"
#include "field_controls.hpp"
#include "utilities.hpp"
#include "mod_menu.hpp"
#include "nav_format.hpp"
#include "navigation_scene.hpp"
#include "navigation_observations.hpp"
#include "owned_nav_target.hpp"
#include "nonblocking_snapshots.hpp"
#include "process_storage.hpp"
#include "camera_direction.hpp"
#include "navigation_events.hpp"
#include "settings.hpp"
#include "butterfly_paths.hpp"
#include "butterfly_navigation.hpp"
#include "wall_cues.hpp"
#include "log_capture.hpp"
#include "dev_focus.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
namespace ffx {
namespace {
constexpr unsigned Domain=11,Slots=4,PointLimit=65536,Writing=0x80000000u;
constexpr unsigned MissingObservations=2;constexpr ULONGLONG MissingMs=500;
struct RouteCapture {OwnedNavTarget target;std::array<Point,PointLimit> points;unsigned count=0,epoch=0,revision=0,collisionSerial=0;bool assisted=false;};
enum : unsigned {RoutePacket,TargetPacket,CameraPacket};
struct Packet {unsigned kind=RoutePacket,slot=0,ticket=0;};
struct CameraCapture {unsigned kind=CameraPacket;nav::CopiedCamera camera;unsigned revision=0,serial=0,audioContext=0,collisionSerial=0,epoch=0;Point player;};
std::atomic<unsigned> cameraSerial{0};
async::AtomicSnapshot<CameraCapture> latestCamera;
ProcessStorage<std::array<RouteCapture,Slots>> captures;
std::array<std::atomic<unsigned>,Slots> tickets{};
std::atomic<unsigned> nextTicket{0},clearRevision{1};
unsigned nextArrivalSerial=0; // Preparation owner only; never reused by a replan.
std::atomic<bool> known{false};
std::atomic<DWORD> preparationOwner{0};
bool onOwner() {return !nativeGameThread() && preparationOwner.load(std::memory_order_acquire)==GetCurrentThreadId();}
struct LegSnapshot {NavLeg leg;unsigned revision=0,valid=0;};
async::AtomicSnapshot<LegSnapshot> publishedLeg;
// The leg is about 60 KB, so a game-thread read can overlap a worker publish and fail.
// The worker skips republishing an unchanged leg, and the game thread keeps its last
// whole read for a short time instead of treating one overlap as a lost route.
alignas(LegSnapshot) std::array<unsigned char,sizeof(LegSnapshot)> lastPublished{},heldLeg{};
ULONGLONG heldAt=0;
void publishSnapshot(LegSnapshot& p) {
    if(!std::memcmp(lastPublished.data(),&p,sizeof(p))) return;
    std::memcpy(lastPublished.data(),&p,sizeof(p));publishedLeg.publish(p);
}
void publishEmpty() {LegSnapshot p;std::memset(&p,0,sizeof(p));publishSnapshot(p);}
struct State {
    Mesh collision;
    std::vector<Entity> live;
    std::vector<Point> legs;
    std::vector<Point> arrivalRoute;
    size_t arrivalNext=1;
    bool arrivalRouted=false;
    size_t nextLeg=1;
    std::wstring destination,pedestal;
    Entity target;
    Point player;
    unsigned epoch=0,revision=0,routeEpoch=0,audioContext=0,collisionSerial=0,routeCollisionSerial=0;
    nav::CollisionSnapshot copiedCollision,routeCollision;
    bool arrived=false,allowed=false,ready=false,automaticPush=false;
    // Pedestal whose arrival was last spoken; a held push re-routes to it.
    int spokenWorker=-1;unsigned spokenId=0,spokenEpoch=0;
    // Consecutive observations without the route's destination, and when the
    // first of them was captured.
    unsigned missing=0,arrivalSerial=0;ULONGLONG missingSince=0;
    speechq::Header header{};
    nav::CopiedCamera camera;
    PedestalNavigationState push;
    // Target whose direction was last spoken; a field camera cut names its new direction.
    Entity spokenTarget;std::wstring spokenTargetName;unsigned spokenTargetEpoch=0;ULONGLONG spokenTargetAt=0;
} ;
ProcessStorage<State> state;
State& owned() {return state.get();}
void release(unsigned slot,unsigned ticket) {if(slot<Slots) tickets[slot].compare_exchange_strong(ticket,0,std::memory_order_release,std::memory_order_relaxed);}
void clearOwned() {
    auto& s=owned();s.legs.clear();s.destination.clear();s.target={};s.routeEpoch=0;s.nextLeg=1;s.arrived=false;
    s.revision=clearRevision.load(std::memory_order_acquire);known=false;publishEmpty();
}
bool current() {return state.ready() && owned().epoch==generation() && owned().revision==clearRevision.load(std::memory_order_acquire);}
void reconcile() {if(state.ready() && !current()) {clearOwned();owned().ready=false;}}
void publishLeg() {
    auto& s=owned();LegSnapshot p;std::memset(&p,0,sizeof(p));p.revision=s.revision;
    if(s.legs.size()>1 && s.nextLeg<s.legs.size() && !s.destination.empty() && (s.routeEpoch==s.epoch || s.arrived)) {
        p.valid=1;p.leg.start=s.legs[s.nextLeg-1];p.leg.goal=s.legs[s.nextLeg];p.leg.final=s.legs.back();p.leg.epoch=s.epoch;p.leg.arrived=s.arrived;p.leg.collisionSerial=s.routeCollisionSerial;
        p.leg.arrivalSerial=s.arrivalSerial;
        p.leg.arrivalInside=p.leg.final;
        if(s.arrivalSerial && s.arrivalRouted) {
            p.leg.arrivalUnreachable=s.arrivalRoute.empty();
            if(!p.leg.arrivalUnreachable) {
                p.leg.start=s.arrivalRoute[s.arrivalNext-1];p.leg.goal=s.arrivalRoute[s.arrivalNext];
                p.leg.arrivalInside=s.arrivalRoute.back();
            }
        }
        if(s.collision.panels.required) {
            const auto& panels=s.collision.panels;
            if(!panels.ready || !panelFactsFresh(panels.revision,panels.capturedAt,GetTickCount64())) {publishEmpty();return;}
            p.leg.panelRevision=panels.revision;p.leg.panelCapturedAt=panels.capturedAt;
            p.leg.panelCorridor=panelLegCorridor(panels,s.target.panelLocal,p.leg.start,p.leg.goal)-panels.radius;
        }
        OwnedNavTarget copied;std::memset(&copied,0,sizeof(copied));if(!captureOwnedNavTarget(s.target,copied)) {publishEmpty();return;}p.leg.target=copied.native;
    }
    publishSnapshot(p);
}
void emit(std::wstring text,const char* producer) {speechq::emitOwned(std::move(text),owned().header,producer,false,false);}
bool sameShape(const InteractionShape& a,const InteractionShape& b) {
    return a.valid==b.valid && a.shape==b.shape && a.crossing==b.crossing && a.talk==b.talk && a.worker==b.worker &&
        distanceXZ(a.center,b.center)<.1 && std::abs(a.center.y-b.center.y)<.1 && distanceXZ(a.end,b.end)<.1 &&
        std::abs(a.end.y-b.end.y)<.1 && std::abs(a.range-b.range)<.1 && std::abs(a.height-b.height)<.1 &&
        std::abs(a.width-b.width)<.1 && std::abs(a.depth-b.depth)<.1 && std::abs(a.angle-b.angle)<.0001 && std::abs(a.viewAngle-b.viewAngle)<.0001;
}
bool sameTarget(const Entity& a,const Entity& b) {
    if(a.actor!=b.actor || a.worker!=b.worker || a.id!=b.id || a.pushControl!=b.pushControl || a.pushLocal!=b.pushLocal) return false;
    const auto& x=a.zone;const auto& y=b.zone;
    bool equal=sameShape(x,y) && x.competing.size()==y.competing.size() && std::abs(x.contactRadius-y.contactRadius)<.1 &&
        distanceXZ(x.approach,y.approach)<.1 && distanceXZ(x.obstacle,y.obstacle)<.1 && std::abs(x.obstacleRadius-y.obstacleRadius)<.1 &&
        std::abs(x.contactMinY-y.contactMinY)<.1 && std::abs(x.contactMaxY-y.contactMaxY)<.1;
    for(size_t i=0;equal && i<x.competing.size();++i) equal=sameShape(x.competing[i],y.competing[i]);
    return equal && a.swimReady==b.swimReady && std::abs(a.swimClearance-b.swimClearance)<.1 && a.swimApproach==b.swimApproach &&
        a.inactive==b.inactive && distanceXZ(a.position,b.position)<.1 && std::abs(a.position.y-b.position.y)<.1;
}
bool ownsAutomaticPush(const Entity& target) {
    const auto& push=owned().push;
    return push.armed && push.held && push.epoch==owned().epoch && push.control>=1 && push.control<=2 &&
        push.control==target.pushControl && push.worker==target.worker && push.id==target.id && push.local==target.pushLocal;
}
// In game (Macalania, 2026-11-19): holding the push key re-routes the
// approach to the same pedestal, which spoke the arrival a second time.
bool repeatsPushArrival(const Entity& target) {
    const auto& s=owned();const auto& push=s.push;
    return target.pushControl && push.armed && push.held && push.epoch==s.epoch && push.control==target.pushControl &&
        push.worker==target.worker && push.id==target.id && s.spokenEpoch==s.epoch && s.spokenWorker==target.worker && s.spokenId==target.id;
}
void noteSpokenTarget(const Entity& target) {
    auto& s=owned();s.spokenTarget=target;s.spokenTargetName=target.name.empty()?target.category:target.name;
    s.spokenTargetEpoch=generation();s.spokenTargetAt=GetTickCount64();
}
void setRoute(const std::vector<Point>& points,const Entity& target,bool newRoute=false,bool assisted=false) {
    reconcile();auto& s=owned();
    if(s.collision.panels.required || target.panelLocal) {
        auto safe=panelSafeRoute(s.collision,target,points);
        if(safe.empty()) {loseNavRoute(L"no safe route around active panels");return;}
        s.legs=std::move(safe);
    } else s.legs=points;
    s.epoch=generation();s.nextLeg=1;s.routeEpoch=s.epoch;s.arrived=false;s.missing=0;
    s.arrivalRoute.clear();s.arrivalNext=1;s.arrivalRouted=false;
    s.routeCollisionSerial=s.collisionSerial;s.routeCollision=s.copiedCollision;
    s.destination=target.name.empty()?target.category:target.name;s.target=target;known=!s.destination.empty();
    if(newRoute) s.automaticPush=assisted && (ownsAutomaticPush(target) || repeatsPushArrival(target));
    if(newRoute && known) noteSpokenTarget(target);
    const bool confirm=!s.collision.panels.required && (newRoute?assisted:s.arrivalSerial!=0) && target.zone.valid && target.zone.talk && !target.zone.crossing && (!target.pushControl || pedestalSocketTarget(target)) && !target.butterfly && !target.swimDepth && !target.swimApproach;
    s.arrivalSerial=confirm?(++nextArrivalSerial?nextArrivalSerial:++nextArrivalSerial):0;
    publishLeg();
    if(newRoute) navRouteStopReported=false;
}
void advance() {
    reconcile();auto& s=owned();if(!s.ready || !s.allowed || !current() || s.collisionSerial!=navCollisionSerial()) {pauseNavRouteAudio();return;}
    nav::CameraScope camera(s.camera.forward,s.camera.right,s.camera.valid);
    NavAudioState audio;audio.contextSerial=s.audioContext;audio.active=true;
    if(s.routeEpoch==s.epoch && s.target.butterfly && !s.arrived) {
        if(s.legs.size()<2) {loseNavRoute(L"route unavailable");return;}
        auto fresh=std::find_if(s.live.begin(),s.live.end(),[&](const Entity& e) {return e.id==s.target.id && e.actor==s.target.actor && e.worker==s.target.worker;});
        if(fresh==s.live.end() || fresh->inactive || fresh->hazardous) {loseNavRoute(fresh==s.live.end()?L"destination gone":fresh->inactive?L"destination inactive":L"destination became hazardous");return;}
        std::vector<Entity> red;for(const auto& e:s.live) if(e.hazardous) red.push_back(e);
        std::vector<Point> remaining{s.player};remaining.insert(remaining.end(),s.legs.begin()+std::min(s.nextLeg,s.legs.size()),s.legs.end());
        if(!butterflyRouteClear(remaining,red) || !butterflyInTarget(*fresh,s.legs.back())) {
            const auto route=navigationRouteOwned(s.collision,s.player,*fresh,s.live);
            if(route.empty()) {loseNavRoute(L"no safe butterfly route");return;}
            s.legs=route;s.nextLeg=1;
        }
        s.target=*fresh;
    }
    if(s.routeEpoch==s.epoch && s.legs.size()>1 && !s.arrived) {
        const bool horizontal=s.collision.fixedHeight && !s.target.swimDepth;
        while(s.nextLeg+1<s.legs.size() && (horizontal || std::abs(s.player.y-s.legs[s.nextLeg].y)<=8)) {
            const auto goal=s.legs[s.nextLeg],prior=s.legs[s.nextLeg-1];const auto x=goal.x-prior.x,z=goal.z-prior.z,length=std::hypot(x,z);
            const auto dx=s.player.x-goal.x,dz=s.player.z-goal.z;const bool passed=length>.001 && dx*x+dz*z>=0 && std::abs(dx*z-dz*x)/length<=1;
            if(distanceXZ(s.player,goal)>1 && !passed) break;++s.nextLeg;
        }
        const auto goal=s.legs[s.nextLeg];const double steps=distanceXZ(s.player,goal)/11.13;
        const auto pointRange=setting(Setting::AutoWalk)?1.0:11.13;
        const bool reached=s.target.panelLocal?insidePanel(s.collision.panels.rows[s.target.panelLocal-57],s.player):
            s.nextLeg+1==s.legs.size() && (s.target.swimDepth?navigationTargetReached(s.target,s.player):
            (s.target.zone.valid?navigationTargetReached(s.target,s.player):distanceXZ(s.player,goal)<=pointRange) && (horizontal || std::abs(s.player.y-goal.y)<=8));
        if(reached) {
            s.arrived=true;
        } else if(!s.target.swimDepth || distanceXZ(s.player,goal)>.1) {
            double pan=0,front=1;
            if(relativeBearing({goal.x-s.player.x,0,goal.z-s.player.z},pan,front)) {audio.beacon=true;audio.pan=static_cast<float>(pan);audio.front=static_cast<float>(front);audio.steps=static_cast<float>(steps);}
        }
    }
    // The worker selects the nearest reachable zone endpoint from the settled
    // approach side, rather than forcing a return to the original destination.
    // Keep that path stable during pulse/coast calibration. Geometry/target
    // revisions go through setRoute and invalidate it with the original route.
    if(s.routeEpoch==s.epoch && s.arrived && s.arrivalSerial && !inInteractionZone(s.target.zone,s.player)) {
        if(!s.arrivalRouted) {
            auto correction=interactionRoute(s.collision,s.player,s.target.zone);
            s.arrivalRoute=std::move(correction);s.arrivalNext=1;s.arrivalRouted=true;
            if(s.arrivalRoute.size()<2) s.arrivalRoute.clear();
        }
        // Precision pulses can settle about 0.4 units from a corner (Djose
        // pedestal, live). Within one unit, advance when the direct segment to
        // the following point keeps the pedestal keep-out clearance.
        const auto clearOfObstacle=[&](Point a,Point b) {
            const auto& zone=s.target.zone;if(zone.obstacleRadius<=0) return true;
            const double x=b.x-a.x,z=b.z-a.z,length=x*x+z*z;
            const double t=length>1e-8?std::clamp(((zone.obstacle.x-a.x)*x+(zone.obstacle.z-a.z)*z)/length,0.0,1.0):0;
            return std::hypot(a.x+t*x-zone.obstacle.x,a.z+t*z-zone.obstacle.z)>=zone.obstacleRadius+.5;
        };
        while(s.arrivalNext+1<s.arrivalRoute.size() && std::abs(s.player.y-s.arrivalRoute[s.arrivalNext].y)<=8) {
            const double distance=distanceXZ(s.player,s.arrivalRoute[s.arrivalNext]);
            if(distance>1 || (distance>.25 && !clearOfObstacle(s.player,s.arrivalRoute[s.arrivalNext+1]))) break;
            ++s.arrivalNext;
        }
    }
    if(!current()) return;
    publishLeg();publishNavAudio(audio);
    if(s.arrived && s.routeEpoch && (!s.arrivalSerial || navArrivalConfirmation.load(std::memory_order_acquire)==s.arrivalSerial)) {
        if(!s.automaticPush) {
            emit(s.destination+L": route destination reached"+(s.target.pushControl?L". "+s.pedestal:L""),"navigation-arrival");
            s.spokenWorker=s.target.pushControl?s.target.worker:-1;s.spokenId=s.target.id;s.spokenEpoch=s.epoch;
        }
        if(setting(Setting::Beacon) && setting(Setting::Arrival)) navAudioArrival();s.routeEpoch=0;
    }
}
void finishConfirmedArrival() {
    const auto& s=owned();
    if(s.arrived && s.routeEpoch && s.arrivalSerial && s.routeCollisionSerial==s.collisionSerial &&
        navArrivalConfirmation.load(std::memory_order_acquire)==s.arrivalSerial) advance();
}
void process(const speechq::Header& header,const std::wstring& bytes) {
    preparationOwner.store(GetCurrentThreadId(),std::memory_order_release);
    if(bytes.size()*sizeof(wchar_t)<sizeof(unsigned)) return;
    unsigned kind;std::memcpy(&kind,bytes.data(),sizeof(kind));
    if(kind==CameraPacket) {
        if(bytes.size()*sizeof(wchar_t)!=sizeof(CameraCapture)) return;
        CameraCapture cut;std::memcpy(&cut,bytes.data(),sizeof(cut));reconcile();
        CameraCapture latest;
        if(!latestCamera.tryRead(latest) || !latest.camera.valid || latest.epoch!=header.generation ||
            latest.revision!=cut.revision || latest.serial!=cut.serial || latest.audioContext!=cut.audioContext || latest.collisionSerial!=cut.collisionSerial) return;
        cut.camera=latest.camera;
        auto& s=owned();
        if(header.generation!=generation() || cut.revision!=clearRevision.load(std::memory_order_acquire) ||
            cut.serial!=cameraSerial.load(std::memory_order_acquire) || cut.audioContext!=navAudioContextSerial() || cut.collisionSerial!=navCollisionSerial()) return;
        const bool route=s.ready && s.allowed && s.collisionSerial==cut.collisionSerial && s.audioContext==cut.audioContext &&
            s.routeEpoch==s.epoch && !s.arrived && s.nextLeg<s.legs.size();
        s.camera=cut.camera;s.header=header;
        if(route) {
            const auto goal=s.legs[s.nextLeg];const Point delta{goal.x-s.player.x,0,goal.z-s.player.z};
            if(s.automaticPush) return;
            if(distanceXZ(s.player,goal)>.1) {emit(L"Route to "+s.destination+L": "+routeSegmentText(distanceXZ(s.player,goal)/11.13,
                cameraDirectionLabel(delta,s.camera.forward,s.camera.valid),setting(Setting::PartialSteps)!=0),"navigation-camera-cut");return;}
        }
        // No route hint for this cut: a short cue, plus where the last spoken
        // target now lies from the new camera.
        std::wstring text=L"Camera changed.";
        if(s.spokenTargetAt && s.spokenTargetEpoch==header.generation && GetTickCount64()-s.spokenTargetAt<=60000 && !s.spokenTargetName.empty()) {
            auto position=s.spokenTarget.position;
            if(s.epoch==header.generation) for(const auto& e:s.live)
                if(e.actor==s.spokenTarget.actor && e.worker==s.spokenTarget.worker && e.id==s.spokenTarget.id) {position=e.position;break;}
            const auto direction=cameraDirectionLabel({position.x-cut.player.x,0,position.z-cut.player.z},cut.camera.forward,cut.camera.valid);
            if(std::wcscmp(direction,L"here") && std::wcscmp(direction,L"unknown")) text+=L" "+s.spokenTargetName+L" now "+direction+L".";
        }
        emit(std::move(text),"navigation-camera-cut");
        return;
    }
    if(kind>TargetPacket || bytes.size()*sizeof(wchar_t)!=sizeof(Packet)) return;Packet packet;std::memcpy(&packet,bytes.data(),sizeof(packet));
    if(packet.slot>=Slots || tickets[packet.slot].load(std::memory_order_acquire)!=packet.ticket) return;
    struct Return {unsigned slot,ticket;~Return(){release(slot,ticket);}} returned{packet.slot,packet.ticket};
    const auto& c=captures.get()[packet.slot];if(c.epoch!=generation() || c.revision!=clearRevision.load(std::memory_order_acquire) || c.collisionSerial!=navCollisionSerial()) return;
    reconcile();auto& s=owned();s.header=header;Entity target;if(!restoreOwnedNavTarget(c.target,target)) return;
    if(packet.kind==RoutePacket) {std::vector<Point> points(c.points.begin(),c.points.begin()+c.count);setRoute(points,target,true,c.assisted);}
    else if(packet.kind==TargetPacket) updateNavTarget(target);
    if(s.ready) advance();else nav::requestNavigationScene();
}
void maintenance() {
    preparationOwner.store(GetCurrentThreadId(),std::memory_order_release);reconcile();if(!captures.ready()) return;
    for(unsigned slot=0;slot<Slots;++slot) {const auto token=tickets[slot].load(std::memory_order_acquire);if(!token || (token&Writing)) continue;
        const auto& c=captures.get()[slot];if(c.epoch!=generation() || c.revision!=clearRevision.load(std::memory_order_acquire)) release(slot,token);}
}
void reset(unsigned) {maintenance();if(state.ready()) {clearOwned();owned().collision={};owned().live.clear();owned().ready=false;}}
bool captureRoute(unsigned kind,const std::vector<Point>& points,const Entity& target,bool assisted=false) {
    if(!captures.ready() || points.size()>PointLimit) return false;
    unsigned slot=Slots,token=(nextTicket.fetch_add(1)+1)&~Writing;if(!token) token=(nextTicket.fetch_add(1)+1)&~Writing;
    for(unsigned i=0;i<Slots;++i) {unsigned empty=0;if(tickets[i].compare_exchange_strong(empty,token|Writing,std::memory_order_acquire)) {slot=i;break;}}
    if(slot==Slots) return false;auto& c=captures.get()[slot];c.epoch=generation();c.revision=clearRevision.load(std::memory_order_acquire);c.count=static_cast<unsigned>(points.size());c.collisionSerial=navCollisionSerial();
    c.assisted=assisted;
    if(!captureOwnedNavTarget(target,c.target)) {release(slot,token|Writing);return false;}
    std::copy(points.begin(),points.end(),c.points.begin());tickets[slot].store(token,std::memory_order_release);
    if(!speechq::captureDomain(Domain,Packet{kind,slot,token},c.epoch)) {release(slot,token);return false;}return true;
}
}
bool navRouteKnown() {return known.load(std::memory_order_acquire);}
void noteNavCameraTarget(const Entity& target) {if(onOwner() && state.ready() && !(target.name.empty() && target.category.empty())) noteSpokenTarget(target);}
bool navLeg(NavLeg& leg) {
    // tryRead writes its result only on success, so a failed native read leaves the held leg intact.
    const bool native=nativeGameThread();LegSnapshot local;const char* reason=nullptr;
    LegSnapshot& p=native?*reinterpret_cast<LegSnapshot*>(heldLeg.data()):local;
    if(publishedLeg.tryRead(p)) {if(native) heldAt=GetTickCount64();}
    else if(!native || !heldAt || GetTickCount64()-heldAt>250) reason="busy";
    if(!reason) reason=p.revision!=clearRevision.load(std::memory_order_acquire)?"cleared":p.leg.epoch!=generation()?"map changed":!p.valid?"no route":nullptr;
    if(reason) {if(native) recordLog("navigation leg unavailable: %s",reason);return false;}
    leg=p.leg;leg.pendingCollision=leg.collisionSerial!=navCollisionSerial();return true;
}
void prepareNavRetirement() {
    captures.prepare();state.prepare();owned().revision=clearRevision.load(std::memory_order_acquire);
    speechq::registerDomain(Domain,process,reset);speechq::registerDomainMaintenance(Domain,maintenance);installWallCues();
}
void clearNavRoute() {navRouteStopReported=true;clearRevision.fetch_add(1,std::memory_order_acq_rel);known=false;pauseNavRouteAudio();stopNavPreview();if(onOwner() && state.ready()) clearOwned();}
void loseNavRoute(const wchar_t* reason) {
    // A pending battle resume speaks its own outcome ("not found" when the
    // destination is really gone), so a route lost while it waits stays silent.
    // A Bevelle boarding stop (0xb100+pedestal) vanishes whenever the pedestal
    // leaves or a recess dialog opens; the ride itself is the outcome.
    const bool boardingStop=onOwner() && owned().target.id>=0xb100u && owned().target.id<=0xb1ffu;
    if(boardingStop) recordLog("navigation route to Bevelle boarding stop dropped: %ls",reason);
    const bool announce=onOwner() && current() && known.load(std::memory_order_acquire) && !boardingStop &&
        !(owned().arrived && !owned().routeEpoch) && !autoWalkResumePending() && claimNavRouteStopSpeech();
    releaseAutoWalk();clearNavRoute();
    if(announce) emit(L"Route lost: "+std::wstring(reason),"navigation-route-lost");
}
void captureNavCamera() {
    if(!nativeGameThread()) return;
    static nav::CopiedCamera previous;static unsigned previousRevision=0,previousEpoch=0;
    const auto revision=clearRevision.load(std::memory_order_acquire),epoch=generation();
    nav::CopiedCamera camera;
    // Sampled in the whole field, route or not: a cut without a route speaks a
    // short cue. The first valid sample of a room is only the baseline.
    if(gameFocusedOrHeld() && !scene && !inBattle() && !modMenuActive() && !utilityConsoleActive() && !nativeFieldControlReason()) camera.valid=cameraBasis(camera.forward,camera.right);
    if(!camera.valid) cameraSerial.fetch_add(1,std::memory_order_acq_rel);
    const bool cut=previousRevision==revision && previousEpoch==epoch && nav::cameraCut(previous,camera);
    if(cut) cameraSerial.fetch_add(1,std::memory_order_acq_rel);
    CameraCapture sample{CameraPacket,camera,revision,cameraSerial.load(std::memory_order_acquire),navAudioContextSerial(),navCollisionSerial(),epoch};
    if(cut) sample.player=playerPosition();
    latestCamera.publish(sample);
    // A running pedestal push re-speaks its own direction on a cut.
    if(cut && !pedestalPushOwnsCamera.load(std::memory_order_acquire)) speechq::captureDomain(Domain,sample,epoch);
    previous=camera;previousRevision=revision;previousEpoch=epoch;
}
void resetNavAudio() {pauseNavAudio();clearNavRoute();publishPositionalCue(0,0,false,generation());}
void refreshNavCollision() {nav::requestNavigationScene();captureWallCues();}
void routeNavAudio(const std::vector<Point>& points,const Entity& target) {routeNavAudio(points,target,false);}
void routeNavAudio(const std::vector<Point>& points,const Entity& target,bool assisted) {
    if(!onOwner()) {if(!captureRoute(RoutePacket,points,target,assisted)) output(L"Navigation route is busy. Try again after it recovers.",false,"navigation-pressure");nav::requestNavigationScene();return;}
    setRoute(points,target,true,assisted);if(owned().ready) advance();
}
void updateNavTarget(const Entity& target) {
    if(!onOwner()) {if(!captureRoute(TargetPacket,{},target)) output(L"Navigation updates are busy. Updates resume when it recovers.",false,"navigation-pressure");return;}
    reconcile();auto& s=owned();
    finishConfirmedArrival();
    if(s.target.panelLocal || s.routeEpoch!=s.epoch || target.actor!=s.target.actor || target.worker!=s.target.worker || target.id!=s.target.id || target.pushControl!=s.target.pushControl || target.pushLocal!=s.target.pushLocal || sameTarget(s.target,target)) return;
    s.target=target;if(target.inactive) {loseNavRoute(L"destination inactive");return;}
    const auto route=navigationRouteOwned(s.collision,s.player,target,s.live);
    if(route.empty()) {loseNavRoute(L"destination outside the connected floor");}
    else setRoute(route,target);
}
void updateNavAudio(Point,bool) {nav::requestNavigationScene();captureWallCues();}
void updateOwnedNavAudio(const nav::Scene& observation,const Mesh& mesh,const std::vector<Entity>& targets,const speechq::Header& header,const std::wstring& pedestal) {
    if(observation.collisionSerial!=navCollisionSerial()) {nav::requestNavigationScene();return;}
    if(nativeGameThread()) return;preparationOwner.store(GetCurrentThreadId(),std::memory_order_release);reconcile();auto& s=owned();s.epoch=observation.epoch;s.revision=clearRevision.load(std::memory_order_acquire);s.player=observation.player;s.audioContext=observation.audioContext;s.allowed=observation.allowed;s.ready=true;s.header=header;s.pedestal=pedestal;
    s.collisionSerial=observation.collisionSerial;s.copiedCollision=observation.geometry.collision;
    s.push=observation.pedestal;
    // Keep silence through the final native completion/late arrival snapshot.
    // A newly requested route resets the latch, including manual socket routes.
    if(s.routeEpoch==s.epoch && ownsAutomaticPush(s.target)) s.automaticPush=true;
    // A dispatched script can leave the floor unchanged. Keep input fenced
    // until the worker checks a current observation, then renew the same leg.
    if(s.routeEpoch==s.epoch && mesh.panels.ready && mesh.panels.revision!=floorPanelRevision.load() &&
        GetTickCount64()-mesh.panels.capturedAt<=250) {nav::requestNavigationScene();return;}
    if(s.routeEpoch==s.epoch && (s.collision.panels.required || mesh.panels.required)) {
        const auto& previous=s.collision.panels;
        const auto& next=mesh.panels;
        if(!panelFactsFresh(next.revision,next.capturedAt,GetTickCount64()))
            loseNavRoute(L"floor panel state changed or unavailable. Walk manually.");
        else if(!sameFloorPanels(previous,next)) {
            const unsigned bit=s.target.panelLocal?1u<<(s.target.panelLocal-57):0;
            auto afterTouch=previous;
            afterTouch.active&=~bit;afterTouch.pattern=next.pattern;
            if((previous.active&bit) && !(next.active&bit) && sameFloorPanels(afterTouch,next) &&
                touchesPanel(previous.rows[s.target.panelLocal-57],s.player,s.player,previous.radius)) s.arrived=true;
            else loseNavRoute(L"floor panel state changed or unavailable. Walk manually.");
        }
    }
    s.collision=mesh;s.live=targets;s.camera={observation.forward,observation.right,observation.cameraValid};nav::CameraScope camera(observation.forward,observation.right,observation.cameraValid);
    finishConfirmedArrival();
    if(s.routeEpoch==s.epoch && !(s.target.panelLocal && s.arrived)) {
        const auto target=std::find_if(targets.begin(),targets.end(),[&](const Entity& e) {return e.actor==s.target.actor && e.worker==s.target.worker && e.id==s.target.id && e.pushControl==s.target.pushControl && e.pushLocal==s.target.pushLocal;});
        // Guide point routes have no entity owner; they retain their copied goal.
        // The field list rebuilt after a battle or scene can lack the destination
        // for a frame. Lose the route only after two observations spanning half a second.
        if(!s.target.panelLocal && s.target.worker>=0 && target==targets.end()) {
            if(mesh.panels.required) {loseNavRoute(L"floor panel route target unavailable");return;}
            if(!s.missing++) s.missingSince=observation.capturedAt;
            if(s.missing>=MissingObservations && observation.capturedAt-s.missingSince>=MissingMs) loseNavRoute(L"destination gone");
            return;
        }
        if(!s.target.panelLocal && s.target.worker>=0) {s.missing=0;updateNavTarget(*target);if(!s.routeEpoch) {advance();return;}}
        // A native ledge sequence may relocate the player or change crossing
        // ownership without moving the destination. Retire/replan the actual
        // remaining legs; never keep steering back across a returning ledge.
        // Collision gates may change without any destination movement. Rebuild
        // the complete route once for a new native serial/remap snapshot. Do
        // not scan every retained leg against every floor triangle on movement.
        bool valid=s.collision.events.ready && s.routeCollisionSerial==s.collisionSerial &&
            s.routeCollision.actorFlags==s.copiedCollision.actorFlags && s.routeCollision.actorIsControlled==s.copiedCollision.actorIsControlled &&
            s.routeCollision.remaps==s.copiedCollision.remaps;
        Point previous=s.player;
        if(s.collision.events.count) for(size_t i=s.nextLeg;valid && i<s.legs.size();++i) {nav::checkComputation();valid=routeSegmentAllowed(s.collision,previous,s.legs[i]);previous=s.legs[i];}
        if(!valid) {
            const auto route=navigationRouteOwned(s.collision,s.player,s.target,s.live);
            if(route.empty()) {loseNavRoute(L"no connected route");return;}
            setRoute(route,s.target);
        }
    }
    advance();
}
}
