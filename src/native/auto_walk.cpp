#include <cwchar>
#include "input_device.hpp"
// Port of the user's FFXII navigation/auto_walk.cpp: camera-relative input,
// same-poll manual priority, 250 ms stale mask, 600 ms interrupted field gap,
// line rejoin, octant hysteresis, bounded unstick and 15 s progress cap.
#include "auto_walk.hpp"
#include "nav_audio.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "movement_keys.hpp"
#include "pedestal.hpp"
#include "field_controls.hpp"
#include "text_plan.hpp"
#include "swimming_navigation.hpp"
#include "game_bindings.hpp"
#include "controller.hpp"
#include "log_capture.hpp"
#include <atomic>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace ffx {
#ifdef FFX_READER_TEST
uint64_t autoWalkClock();
#endif
namespace {
std::atomic<NavigationKeyboardAssist> keyboardAssist{nullptr};
std::atomic<NavigationControllerAssist> controllerAssist{nullptr};
std::atomic<bool> engaged{false},cancel{false};
std::atomic<unsigned> manualRevision{1};
std::atomic<unsigned> routeFailures{0};
std::atomic<unsigned> mask{0},maskEpoch{0};
std::atomic<unsigned> maskCollisionSerial{0};
std::atomic<uint64_t> stamp{0};
unsigned epoch=0;
uint64_t previous=0,octantSince=0,improved=0,movingSince=0,unstickUntil=0,collisionPendingSince=0;
int octant=-1;
bool unstickRight=false;
double best=-1;
Point last,progressGoal;
Point reachedWaypoint;
bool waypointReached=false;
uint64_t waypointAt=0;
bool stopAnnounced=false;
// Native 82D180 (installed): four 0..256 input ramps rise/decay by 32;
// values below 48 do not move. Neutral must drain eight frames plus the
// keyboard/controller publication pipeline before another precision pulse.
constexpr unsigned PedestalDrainFrames=10;
struct Precision {bool active=false;unsigned drain=0,pulse=0,bits=0,sampleFrames=0;double stride=0,fullStride=0;Point seen,goal,sampleOrigin;} precision;
struct InputTrace {bool active=false;uint64_t at=0,dt=0,logged=0;Point target,world;} inputTrace;
uint64_t nowMs();
unsigned precisionInput(unsigned bits,const NavLeg& leg,Point player,bool ordinaryArrival=false) {
    const auto& zone=leg.target.zone;const auto side=zone.approach;
    const auto across=[&](Point p) {return (p.x-zone.center.x)*side.z-(p.z-zone.center.z)*side.x;};
    const bool axial=zone.valid && distanceXZ(side,{})>.9 && std::abs(across(leg.goal))<.01 && std::abs(across(leg.final))<.01;
    if(!ordinaryArrival && (leg.target.pushControl<1 || leg.target.pushControl>2 || !axial)) {precision.active=false;return bits;}
    const auto distance=distanceXZ(player,leg.goal);
    // A cold start can inherit native momentum before we observe one stride.
    if(!precision.active && distance>std::max(11.13,1+PedestalDrainFrames*precision.stride)) return bits;
    if(!precision.active || distanceXZ(precision.goal,leg.goal)>.01) {
        precision.active=true;precision.goal=leg.goal;precision.drain=PedestalDrainFrames;precision.pulse=0;precision.sampleFrames=0;precision.fullStride=0;
    }
    if(precision.pulse) {
        if(bits==precision.bits) {--precision.pulse;return bits;}
        precision.pulse=0;precision.drain=PedestalDrainFrames;precision.sampleFrames=0;
    }
    if(precision.drain) {--precision.drain;return 0;}
    // Ordinary corrections measure the complete pulse and coast; a maximum
    // partial frame alone made their short recovery window mostly neutral.
    if(ordinaryArrival && precision.sampleFrames) {
        const double travel=distanceXZ(player,precision.sampleOrigin);const auto n=precision.sampleFrames;
        if(travel>.001) precision.fullStride=std::max(precision.stride,travel*8/(n*n-2));
        precision.sampleFrames=0;
    }
    if(ordinaryArrival && bits!=precision.bits) precision.fullStride=0;
    // A two-frame pulse first exceeds native deadzone (64/256). Cardinal
    // pulses have total rise+coast displacement (n*n-2)/8 full-speed frames.
    // Four times the largest observed stride bounds a partly ramped sample.
    const auto stride=std::max(.01,ordinaryArrival && precision.fullStride>0?precision.fullStride:4*precision.stride);
    unsigned frames=2;
    while(precision.stride>.001 && frames<8 && ((frames+1)*(frames+1)-2)*stride/8<std::max(0.0,distance-(ordinaryArrival?.1:.5))) ++frames;
    if(ordinaryArrival) {precision.sampleOrigin=player;precision.sampleFrames=frames;}
    precision.bits=bits;precision.pulse=frames-1;precision.drain=PedestalDrainFrames;
    return bits;
}
void tracePedestalInput(unsigned bits,int lx,int ly,unsigned first=0,unsigned second=0) {
    if(!nativeGameThread() || !inputTrace.active || !engaged || !logCaptureHandler.load()) return;
    const auto now=nowMs();if(inputTrace.logged && now-inputTrace.logged<50) return;
    inputTrace.logged=now;
    const auto player=playerPosition();double pan=0,front=0;
    const double yaw=relativeBearing({1,0,0},pan,front)?std::atan2(front,pan):NAN;
    const auto camera=read<uintptr_t>(address(0x1f11430));
    // Native movement's camera-yaw getter uses target(+50)-eye(+66C),
    // unlike navigation's target(+40)-eye(+65C). Compare both in radians.
    const double x=camera?read<float>(camera+0x50)-read<float>(camera+0x66c):0;
    const double z=camera?read<float>(camera+0x58)-read<float>(camera+0x674):0;
    const double nativeYaw=std::isfinite(x) && std::isfinite(z) && std::hypot(x,z)>.001?std::atan2(x,z):NAN;
    recordLog("pedestal input: t %llu dt %llu age %llu player %.3f %.3f target %.3f %.3f world %.3f %.3f stick %d %d keys %02x %02x yaw %.5f native-yaw %.5f bits %x",
        now,inputTrace.dt,now-inputTrace.at,player.x,player.z,inputTrace.target.x,inputTrace.target.z,
        inputTrace.world.x,inputTrace.world.z,lx,ly,first,second,yaw,nativeYaw,bits);
}
FieldControlsSnapshot messageGuard;
std::array<unsigned,4> scans{0xc8,0xcb,0xd0,0xcd};
unsigned diveScan=0; int diveButton=-1;
std::array<unsigned,8> swimManualScans{};
// Screen-reader modifiers: DIK_INSERT, DIK_NUMPAD0 (Numpad Insert) and DIK_CAPITAL.
// A key first pressed while one is held belongs to the reader until released.
constexpr unsigned readerModifierScans[]={0xd2,0x52,0x3a};
constexpr unsigned cancelScans[]={0x11,0x1e,0x1f,0x20,0xc8,0xd0,0xcb,0xcd,0x1c,0x01};
std::atomic<bool> readerModifier{false};
std::array<bool,256> readerOwned{},physicalDown{}; // DirectInput poll owner only
// Battle resume. A walk that stops because a battle starts (directly, or through
// the resource change or control loss that opens it) is armed; the battle must
// begin within ArmMs. After the battle, the field must be safe with an unchanged
// map epoch for SettleMs before the handler re-requests the route. Movement
// outside the battle, a navigation command after it, a new walk or turning the
// setting off cancels it, and it expires ExpireMs after the battle ends.
enum ResumePhase : int {ResumeNone,ResumeArmed,ResumeBattle,ResumeAfter};
constexpr uint64_t ArmMs=3000,SettleMs=1000,ExpireMs=60000;
std::atomic<int> resumePhase{ResumeNone};
std::atomic<bool> resumeCancel{false};
std::atomic<AutoWalkResume> resumeHandler{nullptr};
uint64_t resumeSince=0,resumeSettled=0;unsigned resumeEpoch=0;
// Field identity when the walk was interrupted: room id and player position. A
// battle returns the player to the same room and spot; anything else (another
// map, a cutscene or script that moved the player) drops the resume.
uint16_t resumeRoom=0;Point resumeWhere;
constexpr double ResumeDriftUnits=22.26; // two steps
uint16_t currentRoom() {return read<uint16_t>(address(0xd2ca90),0xffff);}
bool sameResumeField() {
    const auto p=playerPosition();
    return currentRoom()==resumeRoom && std::isfinite(p.x) && std::isfinite(p.z) && std::hypot(p.x-resumeWhere.x,p.z-resumeWhere.z)<=ResumeDriftUnits;
}
NativeRouteTarget walking; // native thread only: the destination of the current walk
constexpr unsigned octMasks[]={1,9,8,12,4,6,2,3}; // forward, left, backward, right
uint64_t nowMs() {
#ifdef FFX_READER_TEST
    return autoWalkClock();
#else
    return GetTickCount64();
#endif
}
const wchar_t* unsafeReason() {
    // Archived 86C840 reports active dialogue, as used by trial utilities.
    // Common 005E -> 871980 clears controller bit 2 and controlled actor;
    // Common 005D -> 871A50 restores them. This includes silent cutscenes.
    if(!setting(Setting::AutoWalk)) return L"setting disabled";
    if(!gameFocused()) return L"focus lost";
    if(scene || inBattle()) return L"field unavailable";
    if(modMenuActive() || utilityConsoleActive()) return L"menu opened";
    return nativeFieldControlReason();
}
bool safe() {return !unsafeReason();}
// The route ends at the edge of the modelled interaction zone, which in Djose left the player
// 1 to 1.3 steps short and facing the way it walked, so Enter did nothing. After the route
// arrives, walk on toward the target for at most one step, ArrivalHomeMs, or until its
// collision stops the player, then hold still and keep facing it for ArrivalSettleMs (a single
// turn can be lost while the character is still moving). Pedestals are never walked into: that
// would push them. An arrival that never gets inside the zone is reported, not ended as reached.
constexpr uint64_t ArrivalHomeMs=1500,ArrivalBlockedMs=300,ArrivalSettleMs=300;
// An edge endpoint needs another pulse/coast after flush and calibration.
// The 33ms warm-ramp fixture exhausted 2000ms 0.035 units outside the zone.
// Keep the entire correction bounded, including any repeated settlements.
constexpr uint64_t InteractionHomeMs=3000;
constexpr double ArrivalHomeUnits=11.13,ArrivalCloseUnits=2.0;
struct Arrival {bool active=false,settling=false,faced=false;uint64_t since=0,still=0,began=0;unsigned frames=0;Point origin,seen;} arrival;
void announceStop(const wchar_t* spokenReason) {
    if(stopAnnounced) return;
    stopAnnounced=true;
    TextPlan announcement;prepareTextPlan(announcement,"auto-walk",false);
    announcement.headline=planConcat(announcement,planLiteral(announcement,L"Auto walk stopped: "),planLiteral(announcement,spokenReason));
    commitTextPlan(announcement);
}
// Stops the player chose, or that finish the walk, never resume after a battle.
bool resumable(const char* reason) {
    for(const char* final:{"player input","new route","arrived","destination inactive","cancelled","route lost"}) if(!std::strcmp(reason,final)) return false;
    return true;
}
void stop(const char* reason,const wchar_t* spokenReason=nullptr,bool routeFailure=false) {
    if(routeFailure) routeFailures.fetch_add(1,std::memory_order_acq_rel);
    const bool was=engaged.exchange(false); mask=0; cancel=false; stamp=0;messageGuard={};arrival={};
    if(was) {
        recordLog("auto walk stopped: %s",reason);
        if(resumable(reason)) {resumeCancel=false;resumeSince=nowMs();resumeRoom=currentRoom();resumeWhere=playerPosition();resumePhase=inBattle()?ResumeBattle:ResumeArmed;}
        else resumePhase=ResumeNone;
        if(spokenReason && epoch==generation() && (!routeFailure || claimNavRouteStopSpeech())) announceStop(spokenReason);
    }
}
// Exits, events and cutscenes take player control while the walk is still steering, so
// control loss is usually the game's own takeover. Stop silently and speak it only if,
// ControlTakeoverMs later, control is still off in the same room with no scene, message,
// dialogue or battle following, and the walk was not already near its destination.
constexpr uint64_t ControlTakeoverMs=3000;
constexpr double NearDestinationUnits=33.39; // three walking steps
struct ControlTakeover {unsigned epoch=0;uint64_t at=0;bool armed=false;} takeover;
const wchar_t* const ControlDisabled=L"player control disabled";
void controlTaken() {
    if(!engaged) return;
    NavLeg leg;const bool arriving=navLeg(leg) && distanceXZ(playerPosition(),leg.final)<=NearDestinationUnits;
    stop(arriving?"player control disabled near the destination":"player control disabled");
    takeover={};if(!arriving) takeover={epoch,nowMs(),true};
}
void serviceTakeover() {
    if(!takeover.armed) return;
    const auto reason=nativeFieldControlReason();
    if(takeover.epoch!=generation() || scene || inBattle() || !reason || std::wcscmp(reason,ControlDisabled)!=0) {
        recordLog("auto walk control takeover was scripted");takeover={};return;
    }
    if(nowMs()-takeover.at<ControlTakeoverMs) return;
    takeover={};recordLog("auto walk control still disabled");announceStop(ControlDisabled);
}
bool messageAllowsAssist() {
    if(!messageGuard.candidate) return true;
    const auto state=classifyFieldControls(messageGuard);
    if(state==FieldControlsClassification::Pending) return false;
    if(state==FieldControlsClassification::Blocking) {stop("dialogue",L"dialogue");return false;}
    messageGuard={};return true;
}
unsigned command() {
    if(!messageAllowsAssist() || !engaged || cancel || maskEpoch!=generation() || maskCollisionSerial!=navCollisionSerial() || !safe() || nowMs()-stamp.load()>250) return 0;
    return mask.load();
}
void textOpened(Event& e) {
    const auto slot=static_cast<unsigned>(e.args[0]);
    if(slot<8) {
        // Messages already copied the localized page on this same return.
        // Its worker classifies that owned packet. While pending, only synthetic
        // assistance is withheld; original/manual native input stays untouched.
        const auto window=read<uintptr_t>(address(0x14676b0)+slot*4);
        messageGuard=fieldControlsForSource(slot,read<uintptr_t>(window+0x94));
        if(messageGuard.candidate) {messageAllowsAssist();return;}
    }
    takeover={};stop("dialogue",L"dialogue");
}
void controlDisabled(Event&) { controlTaken(); }
}
void setNavigationInputAssist(NavigationKeyboardAssist keyAssist,NavigationControllerAssist padAssist) { keyboardAssist=keyAssist;controllerAssist=padAssist; }
void suspendAutoWalk() {manualRevision.fetch_add(1,std::memory_order_acq_rel);if(nativeGameThread()) stop("reset");else {engaged=false;cancel=true;mask=0;stamp=0;}}
// The battle is over once the game says so, even before serviceResume has moved
// the phase on: input in that first frame belongs to the field, not the battle.
bool resumeBattleOver(int phase) {return phase==ResumeAfter || (phase==ResumeBattle && !inBattle());}
void stopAutoWalk() {
    // A navigation command after the battle replaces the pending resume. Before
    // and during the battle, route teardown also lands here and must not cancel it.
    if(resumeBattleOver(resumePhase.load())) resumeCancel=true;
    manualRevision.fetch_add(1,std::memory_order_acq_rel);if(nativeGameThread()) {takeover={};stop("cancelled");}else {engaged=false;cancel=true;mask=0;stamp=0;}
}
bool toggleAutoWalkOff(void (*cancelCommand)()) {
    if(!autoWalkActive()) return false;
    if(cancelCommand) cancelCommand();else stopAutoWalk();
    output(L"Auto walk stopped",true,"auto-walk");return true;
}
void releaseAutoWalk() {routeFailures.fetch_add(1,std::memory_order_acq_rel);manualRevision.fetch_add(1,std::memory_order_acq_rel);if(nativeGameThread()) stop("route lost");else {engaged=false;cancel=true;mask=0;stamp=0;}}
bool autoWalkResumePending() {return resumePhase.load()!=ResumeNone;}
void setAutoWalkResume(AutoWalkResume handler) {resumeHandler=handler;}
unsigned autoWalkInputRevision() {return manualRevision.load(std::memory_order_acquire);}
bool autoWalkActive() { return engaged && !cancel; }
bool autoWalkSteeringGoal(Point& goal) {if(!nativeGameThread() || !autoWalkActive()) return false;goal=progressGoal;return true;}
unsigned autoWalkRouteFailureRevision() {return routeFailures.load(std::memory_order_acquire);}
void startAutoWalk(bool announce) {
    stop("new route");takeover={};
    if(announce) stopAnnounced=false;
    NavLeg leg;
    if(!safe() || !navLeg(leg) || leg.pendingCollision || (leg.arrived && !leg.arrivalSerial) || leg.target.inactive) return;
    // Ordinary octant/hysteresis steering can leave a narrow butterfly detour.
    // This timed activity retains manual movement and the verified route cues.
    if(leg.target.butterfly) { output(L"Follow the butterfly route manually",false,"auto-walk"); return; }
    diveScan=configuredActionKeyboardScan(5);
    for(unsigned i=0;i<4;++i) {
        const auto keys=configuredActionKeyboardScans(i+4);
        swimManualScans[i*2]=keys[0]; swimManualScans[i*2+1]=keys[1];
    }
    diveButton=configuredActionControllerButton(5);
    if(leg.target.swimDepth && ((nativeInputUsesController() && diveButton<0) || (!nativeInputUsesController() && !diveScan))) {
        output(L"Auto walk unavailable: Dive has no configured control",false,"auto-walk"); return;
    }
    walking=leg.target; resumePhase=ResumeNone;
    epoch=generation(); maskEpoch=epoch; previous=0; octant=-1; best=-1;collisionPendingSince=0;waypointReached=false;
    precision={};precision.seen=playerPosition();inputTrace={};
    const auto now=nowMs(); improved=movingSince=now; unstickUntil=0;
    last=playerPosition();progressGoal=leg.goal; cancel=false; engaged=true;
    scans=movementKeys();
    if(announce) {
        TextPlan announcement;prepareTextPlan(announcement,"auto-walk",false);
        announcement.headline=planConcat(announcement,planLiteral(announcement,L"Auto walking to "),planLiteral(announcement,leg.target.name.data()));
        commitTextPlan(announcement);
    }
    // Precision counts native input events, not calls made while starting
    // from inside one. Keep the cleared mask until that event's sole update.
    if(walking.pushControl<1 || walking.pushControl>2) updateAutoWalk();
}
// Walking outside the battle means the player took over; battle commands and
// confirming results do not.
void movedAfterBattle() {const int phase=resumePhase.load();if(phase==ResumeArmed || resumeBattleOver(phase)) resumeCancel=true;}
void serviceResume() {
    const int phase=resumePhase.load();if(phase==ResumeNone) return;
    const auto now=nowMs();
    if(resumeCancel.exchange(false) || !setting(Setting::AutoWalk)) {resumePhase=ResumeNone;return;}
    if(inBattle()) {resumePhase=ResumeBattle;return;}
    // The encounter takes control before the battle flag is set. Wait while the
    // field is not the player's; expire only after ArmMs of free field control.
    if(phase==ResumeArmed) {if(!safe()) resumeSince=now;else if(now-resumeSince>ArmMs) resumePhase=ResumeNone;return;}
    if(phase==ResumeBattle) {resumePhase=ResumeAfter;resumeSince=resumeSettled=now;resumeEpoch=generation();return;}
    if(now-resumeSince>ExpireMs) {resumePhase=ResumeNone;recordLog("auto walk resume expired");return;}
    if(!safe() || resumeEpoch!=generation()) {resumeEpoch=generation();resumeSettled=now;return;}
    // The field can reload more than once while the battle closes, so a new map
    // generation alone is not a new place; the room id and position decide that.
    if(!sameResumeField()) {resumePhase=ResumeNone;recordLog("auto walk resume dropped: room or position changed");return;}
    if(now-resumeSettled<SettleMs) return;
    resumePhase=ResumeNone;
    const auto handler=resumeHandler.load();
    recordLog("auto walk resume after battle: %s",handler && handler(walking)?"requested":"unavailable");
}
void updateAutoWalk() {
    serviceTakeover();
    if(!engaged) serviceResume();
    // The map announcement owns transition speech, including stale dialogue guards.
    if(engaged && epoch!=generation()) {stop("map changed");return;}
    if(!messageAllowsAssist()) {mask=0;return;}
    if(!engaged) return;
    const auto now=nowMs();
    if(cancel) {stop("player input");return;}
    if(const auto reason=unsafeReason()) {
        if(std::wcscmp(reason,ControlDisabled)==0) {controlTaken();return;}
        const bool explicitStop=!setting(Setting::AutoWalk) || modMenuActive() || utilityConsoleActive();
        stop("context",explicitStop?nullptr:reason);if(explicitStop) resumePhase=ResumeNone;return;
    }
    if(previous && now-previous>600) {stop("field update interrupted",L"game paused");return;}
    const auto frameMs=previous?now-previous:0;previous=now;
    inputTrace.active=false;
    NavLeg leg;
    if(!navLeg(leg)) { stop("route lost",L"route lost",true); return; }
    // Withhold only synthetic input while the worker rebuilds a changed gate.
    // No wait occurs on this owner. An unavailable result stops immediately;
    // a pending result has the existing600ms field-recovery bound.
    if(leg.pendingCollision) {
        mask=0;
        if(!collisionPendingSince) collisionPendingSince=now;
        else if(now-collisionPendingSince>600) stop("collision route update unavailable",L"route blocked",true);
        return;
    }
    if(collisionPendingSince) {collisionPendingSince=0;octant=-1;unstickUntil=0;movingSince=improved=now;best=-1;}
    maskCollisionSerial=leg.collisionSerial;
    if(leg.target.inactive) { stop("destination inactive",L"destination inactive",true); return; }
    if(!leg.arrived && leg.epoch!=epoch) { stop("map changed"); return; }
    const auto player=playerPosition();
    if(!std::isfinite(player.x) || !std::isfinite(player.y) || !std::isfinite(player.z)) { stop("unreadable player",L"player position unavailable"); return; }
    const double delta=distanceXZ(player,last);
    if(delta>55.65) { stop("player relocated",L"player relocated"); return; }
    precision.stride=std::max(precision.stride,distanceXZ(player,precision.seen));precision.seen=player;
    inputTrace.active=leg.target.pushControl>=1 && leg.target.pushControl<=2;
    inputTrace.at=now;inputTrace.dt=frameMs;inputTrace.target=leg.goal;inputTrace.world={};
    if(pedestalWalkLeg(leg,player)) {mask=0;stamp=now;last=player;return;}
    if(!leg.arrived && arrival.active && leg.arrivalSerial) {arrival.settling=false;arrival.frames=0;}
    if(leg.arrived) {
        const auto& target=leg.target;
        if(target.butterfly || target.swimDepth || !target.zone.valid || target.zone.crossing) {
            if(!target.butterfly && !target.swimDepth) faceNavigationTarget(target);
            stop("arrived");return;
        }
        if(!arrival.active) {arrival={};arrival.active=true;arrival.began=arrival.since=arrival.still=now;arrival.origin=arrival.seen=player;arrival.settling=target.pushControl!=0 || (!leg.arrivalSerial && target.zone.obstacleRadius>0);}
        const Point center=target.pushControl?target.position:leg.arrivalSerial?leg.final:target.zone.center;
        // Every ordinary target can carry the live pedestal keep-out, even
        // across the room. Only a correction that enters it is unsafe.
        const auto canHome=[&] {
            if(target.pushControl) return false;
            if(target.zone.obstacleRadius<=0) return true;
            const double x=center.x-player.x,z=center.z-player.z,length=x*x+z*z;
            const auto obstacle=target.zone.obstacle;
            const double t=length>1e-8?std::clamp(((obstacle.x-player.x)*x+(obstacle.z-player.z)*z)/length,0.0,1.0):0;
            return std::hypot(player.x+t*x-obstacle.x,player.z+t*z-obstacle.z)>=target.zone.obstacleRadius+.5;
        };
        if(!arrival.settling) {
            if(distanceXZ(player,arrival.seen)>.1) {arrival.seen=player;arrival.still=now;last=player;}
            double pan=0,front=0;
            const bool done=(distanceXZ(player,center)<=(leg.arrivalSerial?.25:ArrivalCloseUnits) &&
                (!leg.arrivalSerial || inInteractionZone(target.zone,player))) || distanceXZ(player,arrival.origin)>=ArrivalHomeUnits ||
                (!leg.arrivalSerial && (now-arrival.still>=ArrivalBlockedMs || now-arrival.since>=ArrivalHomeMs)) ||
                (leg.arrivalSerial && (now-arrival.began>=InteractionHomeMs || !canHome())) || !relativeBearing({center.x-player.x,0,center.z-player.z},pan,front);
            if(!done) {
                const auto degrees=std::atan2(pan,front)*180/3.14159265358979323846;
                const auto bits=octMasks[(static_cast<int>(std::round(degrees/45))+8)&7];
                mask=leg.arrivalSerial?precisionInput(bits,leg,player,true):bits;stamp=now;return;
            }
            recordLog("auto walk homed %.2f units toward the target",distanceXZ(player,arrival.origin));
            arrival.settling=true;arrival.frames=0;arrival.since=now;
        }
        mask=0;stamp=now;
        const bool faced=faceNavigationTarget(target);arrival.faced=faced || arrival.faced;
        ++arrival.frames;
        if(now-arrival.since<ArrivalSettleMs || (leg.arrivalSerial && arrival.frames<PedestalDrainFrames)) return;
        if(leg.arrivalSerial) {
            const bool inside=inInteractionZone(target.zone,player);
            if(!inside && canHome() && now-arrival.began<InteractionHomeMs) {
                arrival.settling=false;arrival.since=arrival.still=now;arrival.origin=arrival.seen=player;return;
            }
            if(!inside) {stop("arrived outside the interaction zone",L"not in interaction range",true);return;}
            // Exact coincidence needs no turn; any other position must face
            // successfully on this settled frame, rather than latch an old pose.
            if(!faced && distanceXZ(player,target.zone.center)>=.001) {stop("could not face destination",L"could not face destination",true);return;}
            navArrivalConfirmation.store(leg.arrivalSerial,std::memory_order_release);
            refreshNavCollision();stop("arrived");return;
        }
        if(!arrival.faced && !target.pushControl) {stop("arrived outside the interaction zone",L"not in interaction range");return;}
        stop("arrived");return;
    }
    if(leg.target.swimDepth) {
        if(!swimmingGoalActive(leg.target) || player.y>250) { stop("swimming transition",L"swimming transition"); return; }
        const double depth=std::max(0.0,251-player.y)+distanceXZ(player,leg.final);
        if(best<0 || depth<best-1) { best=depth; improved=now; }
        else if(now-improved>=15000) { stop("no diving progress",L"no diving progress"); return; }
        last=player;
        if(distanceXZ(player,leg.goal)<=.1) {mask=16;stamp=now;return;}

    }
    if(!leg.target.swimDepth && delta>.1) { last=player; movingSince=now; }
    const bool pedestalRoute=leg.target.pushControl || leg.target.zone.obstacleRadius>0;
    if(leg.target.pushControl>=1 && leg.target.pushControl<=2 && distanceXZ(leg.goal,leg.final)>.01) {
        if(waypointReached && distanceXZ(reachedWaypoint,leg.goal)>.01) waypointReached=false;
        const auto x=leg.goal.x-leg.start.x,z=leg.goal.z-leg.start.z,length=std::hypot(x,z);
        const auto dx=player.x-leg.goal.x,dz=player.z-leg.goal.z;
        const bool passed=length>.001 && dx*x+dz*z>=0 && std::abs(dx*z-dz*x)/length<=1;
        if((distanceXZ(player,leg.goal)<=1 || passed) && std::abs(player.y-leg.goal.y)<=8 && !waypointReached) {
            waypointReached=true;reachedWaypoint=leg.goal;waypointAt=now;refreshNavCollision();
            progressGoal=leg.goal;best=distanceXZ(player,leg.goal);improved=now;
            mask=0;stamp=now;return;
        }
        // The worker may miss a waypoint crossed between observations. Hold
        // the reached point instead of steering back and forth through it.
        if(waypointReached) {
            const auto& zone=leg.target.zone;const auto side=zone.approach;
            const auto axial=[&](Point p) {return (p.x-zone.center.x)*side.x+(p.z-zone.center.z)*side.z;};
            const auto across=[&](Point p) {return (p.x-zone.center.x)*side.z-(p.z-zone.center.z)*side.x;};
            const bool radial=zone.valid && zone.contactRadius>0 && !zone.talk && distanceXZ(side,{})>.9 &&
                std::abs(across(leg.goal))<.01 && std::abs(across(leg.final))<.01 &&
                axial(leg.goal)>axial(leg.final) && axial(leg.final)>=zone.contactRadius+.1;
            if(radial) {leg.start=leg.goal;leg.goal=leg.final;}
            else {
                mask=0;stamp=now;
                if(now-waypointAt>=250 || distanceXZ(player,reachedWaypoint)>1.5) {
                    pedestalRetryNoProgress();stop("pedestal waypoint replan");
                }
                return;
            }
        }
    } else waypointReached=false;
    if(!leg.target.swimDepth && now-movingSince>1800 && now>=unstickUntil) {
        if(leg.target.butterfly) { stop("butterfly route stalled",L"butterfly route stalled"); return; }
        refreshNavCollision();
        if(leg.target.pushControl>=1 && leg.target.pushControl<=2) {pedestalRetryNoProgress();stop("pedestal stopped before handoff");return;}
        if(!navLeg(leg) || leg.arrived) { stop("replan unavailable",L"replan unavailable",true); return; }
        if(!pedestalRoute) unstickRight=!unstickRight;
        unstickUntil=pedestalRoute?0:now+700;movingSince=now;
        recordLog("auto walk %s at %.2f %.2f %.2f",pedestalRoute?"pedestal replan":"bounded sidestep/replan",player.x,player.y,player.z);
    }
    Point steer=leg.goal;
    const auto& zone=leg.target.zone;
    const auto side=zone.approach;
    const double along=(player.x-zone.center.x)*side.x+(player.z-zone.center.z)*side.z;
    const double goalAlong=(leg.goal.x-zone.center.x)*side.x+(leg.goal.z-zone.center.z)*side.z;
    const double goalAcross=(leg.goal.x-zone.center.x)*side.z-(leg.goal.z-zone.center.z)*side.x;
    const bool finalPushSide=leg.target.pushControl>=1 && leg.target.pushControl<=2 && zone.valid && !zone.talk &&
        distanceXZ(side,{})>.9 && distanceXZ(leg.goal,leg.final)<.01 && along>0 && goalAlong>0 && std::abs(goalAcross)<=1;
    const auto x=leg.goal.x-leg.start.x,z=leg.goal.z-leg.start.z,length2=x*x+z*z;
    if(finalPushSide) {
        // Only the last radial leg may enter clearance. Centre first, then walk
        // straight in; the detour's line-rejoin lead can touch an off-side corner.
        // Match staging's one-unit band; .65 can be skipped by a digital stride.
        const double across=(player.x-zone.center.x)*side.z-(player.z-zone.center.z)*side.x;
        if(std::abs(across)>1) steer={player.x-across*side.z,player.y,player.z+across*side.x};
    } else if(length2>.001) {
        const auto t=std::clamp(((player.x-leg.start.x)*x+(player.z-leg.start.z)*z)/length2,0.0,1.0);
        Point nearest{leg.start.x+t*x,player.y,leg.start.z+t*z};
        if(distanceXZ(nearest,player)>.35) {
            const auto lead=std::min(1.0,t+2/std::sqrt(length2));
            steer={leg.start.x+lead*x,player.y,leg.start.z+lead*z};
        }
    }
    if(!leg.target.swimDepth) {
        // A detour can move away from its destination for longer than the cap.
        // Measure advance toward the current waypoint without rearming on replans.
        const Point goal=leg.target.pushControl>=1 && leg.target.pushControl<=2?steer:leg.goal;
        const double distance=distanceXZ(player,goal);
        if(distanceXZ(progressGoal,goal)>.1) {
            const auto previousDistance=distanceXZ(player,progressGoal);
            if(delta>.1 && (previousDistance<=1.5 || previousDistance<best)) improved=now;
            progressGoal=goal;best=distance;
        }
        if(best<0 || distance<best-1) { best=distance; improved=now; }
        else if(now-improved>=15000) {
            const auto pedestal=leg.target.pushControl?leg.target.position:leg.target.zone.obstacle;
            recordLog("auto walk no progress: player %.3f %.3f %.3f, pedestal %.3f %.3f %.3f, waypoint %.3f %.3f %.3f",
                player.x,player.y,player.z,pedestal.x,pedestal.y,pedestal.z,progressGoal.x,progressGoal.y,progressGoal.z);
            const bool retry=pedestalRetryNoProgress();
            stop("no progress",retry?nullptr:L"no progress"); return;
        }
    }
    double pan=0,front=0;
    if(!relativeBearing({steer.x-player.x,0,steer.z-player.z},pan,front)) { stop("camera unavailable",L"camera unavailable"); return; }
    const auto degrees=std::atan2(pan,front)*180/3.14159265358979323846;
    int desired=(static_cast<int>(std::round(degrees/45))+8)&7;
    if(octant>=0 && desired!=octant && !pedestalRoute) {
        auto difference=degrees-octant*45; while(difference>180) difference-=360; while(difference< -180) difference+=360;
        const int gap=std::abs(desired-octant);
        if(std::abs(difference)<=32.5 || ((gap==1 || gap==7) && now-octantSince<250)) desired=octant;
    }
    if(desired!=octant) { octant=desired; octantSince=now; }
    if(now<unstickUntil) desired=(desired+(unstickRight?2:6))&7;
    inputTrace.target=steer;inputTrace.world={steer.x-player.x,0,steer.z-player.z};
    mask=precisionInput(octMasks[desired] | (leg.target.swimDepth?16u:0u),leg,player); stamp=now;
}
void autoWalkReaderModifier(bool held) {readerModifier=held;}
void autoWalkPlayerInput(unsigned vk) {
    // Reader chords belong to the screen reader: neither auto walk nor an armed
    // pedestal approach may take them as manual movement.
    if(readerModifier) return;
    pedestalPlayerInput(vk);
    if(pedestalConsentVk(vk)) return;
    const bool movement=vk=='W' || vk=='A' || vk=='S' || vk=='D' || vk==VK_UP || vk==VK_DOWN || vk==VK_LEFT || vk==VK_RIGHT;
    if(movement) movedAfterBattle();
    if(movement || vk==VK_ESCAPE || vk==VK_RETURN) {
        manualRevision.fetch_add(1,std::memory_order_acq_rel);
        cancel=true; mask=0;
    }
}
void autoWalkKeyboard(uint8_t* keys) {
    bool modifier=false;
    for(unsigned scan:readerModifierScans) if(keys[scan]&0x80) modifier=true;
    readerModifier=modifier;
    for(unsigned i=0;i<256;++i) {
        const bool down=(keys[i]&0x80)!=0;
        readerOwned[i]=down && (readerOwned[i] || (modifier && !physicalDown[i]));
        physicalDown[i]=down;
    }
    // The pedestal approach sees only the player's keys, never reader-owned ones.
    std::array<uint8_t,256> playerKeys;
    for(unsigned i=0;i<256;++i) playerKeys[i]=readerOwned[i]?0:keys[i];
    pedestalKeyboardInput(playerKeys.data());
    const auto player=[&](unsigned scan) {return (keys[scan]&0x80) && !readerOwned[scan];};
    bool physical=false;
    for(unsigned scan:cancelScans) if(player(scan) && !pedestalConsentKey(scan)) physical=true;
    for(unsigned scan:scans) if(player(scan) && !pedestalConsentKey(scan)) physical=true;
    if(physical) manualRevision.fetch_add(1,std::memory_order_acq_rel);
    bool moved=false;
    for(unsigned scan:cancelScans) if(scan!=0x1c && scan!=0x01 && player(scan)) moved=true;
    for(unsigned scan:scans) if(player(scan)) moved=true;
    if(moved) movedAfterBattle();
    if(const auto assist=keyboardAssist.load()) assist(keys);
    if(!engaged) return;
    // Raw physical buffer is checked before adding configured movement bits. Held-state
    // edge tracking must retain this original buffer, never injected keys.
    for(unsigned scan:cancelScans)
        if(player(scan) && !pedestalConsentKey(scan)) { cancel=true; mask=0; return; }
    const auto bits=command();
    for(unsigned i=0;i<scans.size();++i) if(player(scans[i])) { cancel=true; mask=0; return; }
    if(mask.load()&16) for(auto scan:swimManualScans) if(scan && player(scan)) { cancel=true; mask=0; return; }
    // A reader chord such as Insert+Down must not steer the character either.
    const auto strip=[&](unsigned scan) {if(scan && readerOwned[scan]) keys[scan]=0;};
    for(unsigned scan:cancelScans) strip(scan);
    for(unsigned scan:scans) strip(scan);
    for(unsigned scan:swimManualScans) strip(scan);
    if(bits&16) {
        if(diveScan) keys[diveScan]|=0x80; else { cancel=true; mask=0; }
    }
    for(unsigned i=0;i<scans.size();++i) if(bits&(1u<<i)) keys[scans[i]]|=0x80;
    unsigned first=0,second=0;
    for(unsigned i=0;i<scans.size();++i) if(bits&(1u<<i)) {if(!first) first=scans[i];else second=scans[i];}
    tracePedestalInput(bits,0,0,first,second);
}
void autoWalkController(pad::State& state) {
    const auto consent=pedestalConsentButtons();
    const bool physical=std::abs(int(state.lx))>=5000 || std::abs(int(state.ly))>=5000 ||
        std::abs(int(state.rx))>=5000 || std::abs(int(state.ry))>=5000 ||
        (state.lt>=30 && !(consent&pad::bit(pad::LT))) || (state.rt>=30 && !(consent&pad::bit(pad::RT))) || (state.held&~consent);
    if(physical) manualRevision.fetch_add(1,std::memory_order_acq_rel);
    if(std::abs(int(state.lx))>=5000 || std::abs(int(state.ly))>=5000 ||
        (state.held&(pad::bit(pad::Up)|pad::bit(pad::Down)|pad::bit(pad::Left)|pad::bit(pad::Right)))) movedAfterBattle();
    if(const auto assist=controllerAssist.load()) assist(state);
    if(!engaged) return;
    const bool manual=std::abs(int(state.lx))>=5000 || std::abs(int(state.ly))>=5000 ||
        std::abs(int(state.rx))>=5000 || std::abs(int(state.ry))>=5000 ||
        (state.lt>=30 && !(consent&pad::bit(pad::LT))) || (state.rt>=30 && !(consent&pad::bit(pad::RT)));
    if(manual || (state.held&~consent)) { cancel=true; mask=0; return; }
    if(messageGuard.candidate && classifyFieldControls(messageGuard)==FieldControlsClassification::Pending) return;
    if(maskCollisionSerial!=navCollisionSerial()) return;
    const auto bits=command();
    if(bits&16) {
        if(diveButton<0) { cancel=true; mask=0; return; }
        if(diveButton==pad::LT) state.lt=255;
        else if(diveButton==pad::RT) state.rt=255;
        else state.held|=pad::bit(diveButton);
    }
    state.lx=static_cast<int16_t>((bits&8?24000:0)-(bits&2?24000:0));
    state.ly=static_cast<int16_t>((bits&1?24000:0)-(bits&4?24000:0));
    tracePedestalInput(bits,state.lx,state.ly);
}
namespace {
void movementModeBefore(Event& e) {
    e.before[0]=e.args[0]==read<uintptr_t>(address(0xf00740)) && read<uint8_t>(e.args[0]+0x185)!=e.args[1];
}
void movementModeAfter(Event& e) { if(e.before[0]) { stop("movement mode changed",L"movement mode changed"); resetNavAudio(); } }
}
void installAutoWalk() {
    hook(0x42b260,movementModeBefore,movementModeAfter);
    // Inspect the installed source after the native language selection.
    // The native dialogue/control guard already blocks injection mid-call.
    hook(0x4b8980,nullptr,textOpened);
    hook(0x471980,controlDisabled,nullptr); // verified native player-control disable
}
}
