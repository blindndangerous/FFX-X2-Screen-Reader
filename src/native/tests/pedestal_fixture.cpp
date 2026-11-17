#include <atomic>
// Isolated native input/script callbacks. Never launches or attaches to the game.
#include "../pedestal.hpp"
#include "../auto_walk.hpp"
#include "../log_capture.hpp"
#include "../nav_audio.hpp"
#include "../settings.hpp"
#include "../input_device.hpp"
#include "../owned_retirement.hpp"
#include "../owned_nav_target.hpp"
#include "../navigation_observations.hpp"
#include "ready_speech_adapter.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <cmath>
#include <map>
#include <new>
#include <thread>
namespace {
bool trackPedestalAllocations=false;
unsigned pedestalAllocations=0;
}
void* operator new(size_t size) {
    if(trackPedestalAllocations) ++pedestalAllocations;
    if(auto* p=std::malloc(size?size:1)) return p;
    throw std::bad_alloc();
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p,size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p,size_t) noexcept { ::operator delete[](p); }
namespace ffx {
void refreshSwimmingGoal(Entity& target) { target.inactive=true; }
uintptr_t base=0;
unsigned epoch=1,starts=0,faces=0;
uint64_t pedestalTime=1000;
uint64_t pedestalClock() {return pedestalTime;}
unsigned fixtureConsent=0x40;
bool padOwned=false;
std::atomic<int> scene{0};int dialogue=0;
bool enabled=true,focused=true,battle=false,menu=false,console=false,walking=false,verified=true;
UtilityScript script;
std::map<unsigned,int> values;
std::array<unsigned,16> remap{};
struct Listener { Handler before,after; };
std::map<unsigned,std::vector<Listener>> hooks;
std::vector<std::wstring> spoken;
std::vector<std::string> approachDiagnostics;
std::vector<Point> route;
int __cdecl mode() { return dialogue; }
uint16_t __cdecl nativeRemap(unsigned,unsigned,uint16_t raw) {
    unsigned mapped=0; for(unsigned i=0;i<16;++i) if(raw&(1u<<i)) mapped|=1u<<remap[i]; return static_cast<uint16_t>(mapped);
}
uintptr_t __cdecl worker(unsigned local) { return base+0x1000000+local*0x1000; }
uint16_t __cdecl entry(unsigned type,unsigned channel,uintptr_t) {
    const unsigned actor[]={2,3,4,5,0xffff,0xffff,0xffff},zone[]={2,3,0xffff,5,4,6,7};
    return static_cast<uint16_t>(channel<7?(type==1?actor[channel]:zone[channel]):0xffff);
}
uint8_t __cdecl priority(unsigned type,unsigned channel) { return channel>=7 || (type==1?channel>=4:channel==2)?255:1; }
uintptr_t address(unsigned rva) {
    if(rva==0x46c840) return reinterpret_cast<uintptr_t>(mode);
    if(rva==0x488bb0) return reinterpret_cast<uintptr_t>(nativeRemap);
    if(rva==0x46a7c0) return reinterpret_cast<uintptr_t>(worker);
    if(rva==0x476360) return reinterpret_cast<uintptr_t>(entry);
    if(rva==0x4767e0) return reinterpret_cast<uintptr_t>(priority);
    return base+rva;
}
bool copy(uintptr_t p,void* out,size_t size) {
    __try { memcpy(out,reinterpret_cast<void*>(p),size); return true; } __except(1) { return false; }
}
unsigned generation() { return epoch; }
bool delayRoute=false;
unsigned routeRequests=0;
bool nativeOwner=true;
bool nativeGameThread() {return nativeOwner;}
bool nativeAtelSpan(uintptr_t,AtelSpan&) {return false;} // This semantic host mocks utilities, not physical resource receipts.
bool readyAtelIdentity(uintptr_t,AtelIdentity&) {return false;}
bool readyAtelWorkerHeader(uintptr_t,unsigned,unsigned&) {return false;}
bool hook(unsigned rva,Handler before,Handler after) { hooks[rva].push_back({before,after}); return true; }
void log(const char*,...) {}
bool output(const std::wstring& text,bool,const char*) { spoken.push_back(text); return true; }
bool gameFocused() { return focused; }
bool inBattle() { return battle; }
bool modMenuActive() { return menu; }
bool utilityConsoleActive() { return console; }
bool controllerOwned() { return padOwned; }
std::wstring nativeBindingName(unsigned action,bool pad) {
    const auto config=read<uintptr_t>(address(0x1efb504));
    const auto id=read<unsigned>(config+0x60+action*4,128);
    if(pad) return id==13?L"B":id==12?L"A":L"";
    return id==80?L"Space":id==77?L"Enter":id==118?L"Z":id==127?L"Numpad Enter":L"";
}
int setting(Setting s) { return s==Setting::AutoWalk && enabled; }
UtilityScript utilityScript() { return script; }
bool captureUtilityScript(UtilityScript& out) noexcept {out=script;return true;}
uintptr_t utilityWorker(const UtilityScript&,unsigned local) { return worker(local); }
bool captureUtilityWorker(const UtilityScript&,unsigned local,uintptr_t& out) noexcept {if(!nativeOwner) return false;out=worker(local);return true;}
int utilityValue(const UtilityScript&,unsigned low,unsigned,unsigned) { return values[low]; }
bool captureUtilityValue(const UtilityScript&,unsigned low,unsigned,unsigned,int& out) noexcept {const auto row=values.find(low);out=row==values.end()?0:row->second;return true;}
const TrialUtility* currentTrial(const UtilityScript&) { static TrialUtility trial{}; return verified?&trial:nullptr; }
const TrialUtility* captureCurrentTrial(const UtilityScript& candidate) noexcept {return currentTrial(candidate);}
uintptr_t playerActor() { return base+0x1300000; }
Point playerPosition() { const auto p=playerActor(); return {read<float>(p+0xc),read<float>(p+0x10),read<float>(p+0x14)}; }
Mesh readMesh() { Mesh mesh; mesh.openPlane=true; mesh.fixedHeight=true; mesh.planeY=playerPosition().y; return mesh; }
bool faceNavigationTarget(const Entity&) { ++faces; return true; }
bool faceResult=true;
bool faceNavigationTarget(const NativeRouteTarget&) {++faces;return faceResult;}
#ifndef FFX_PEDESTAL_WALK_INTEGRATION
bool autoWalkActive() { return walking; }
bool autoWalkSteeringGoal(Point& goal) {if(!walking || route.empty()) return false;goal=route.back();return true;}
#endif
unsigned walkStops=0;
bool legValid=false,legArrived=false;
unsigned inputRevision=1;
unsigned routeFailuresFixture=0;
char lastStopReason[256]{};
#ifndef FFX_PEDESTAL_WALK_INTEGRATION
unsigned autoWalkInputRevision() { return inputRevision; }
void stopAutoWalk() { walking=false; ++walkStops; ++inputRevision; }
unsigned autoWalkRouteFailureRevision() {return routeFailuresFixture;}
void startAutoWalk(bool) { walking=true; ++starts; }
#endif
#ifdef FFX_PEDESTAL_WALK_INTEGRATION
NavLeg integrationLeg;
bool navLeg(NavLeg& leg) {leg=integrationLeg;return legValid;}
void refreshNavCollision() {}
#else
bool navLeg(NavLeg& leg) { leg={}; leg.arrived=legArrived; return legValid; }
#endif
void routeNavAudio(const std::vector<Point>& points,const Entity&) { route=points; }
void routeNavAudio(const std::vector<Point>& points,const Entity& target,bool) {routeNavAudio(points,target);}
// Semantic immediate handback adapter: production scheduling/data-age is
// exercised separately by navigation_service_fixture. This host only preserves
// original trial input/PC/consent cases using the real pure routing algorithm.
bool requestNavigationAssistRoute() {
    ++routeRequests;if(delayRoute) return true;
    NativeRouteTarget target;if(!pedestalTargetForTest(target)) return false;
    const auto state=capturePedestalNavigationState();const auto origin=playerPosition();const auto mesh=readMesh();
    if(state.crossing) {
        const auto crossing=interactionZone(worker(39),16);
        static_cast<InteractionShape&>(target.zone)=static_cast<const InteractionShape&>(crossing);
        target.zone.crossing=true;target.zone.talk=false;target.zone.height=2;
    }
    nativeOwner=false;OwnedNavTarget copied{};copied.native=target;Entity owned;
    const bool restored=restoreOwnedNavTarget(copied,owned);const auto points=restored?interactionRoute(mesh,origin,owned.zone):std::vector<Point>{};
    if(!points.empty()) routeNavAudio(points,owned);nativeOwner=true;
    updatePedestalOwnedTarget(target);if(points.empty()) {cancelPedestal();return false;}
#ifdef FFX_PEDESTAL_WALK_INTEGRATION
    OwnedNavTarget ready;captureOwnedNavTarget(owned,ready);
    integrationLeg={points.front(),points[1],points.back(),ready.native,generation(),false};legValid=true;
    startAutoWalk(false);
#else
    startAutoWalk();
#endif
    return true;
}
unsigned automaticHelpRequests=0;
void requestNavigationAssistInstruction() {++automaticHelpRequests;}
unsigned pathQueries=0;
namespace nav {void requestNavigationScene() {}
bool submitNavigationQuery(Query query,bool,unsigned,unsigned,uintptr_t,unsigned) {if(query==Query::Path || query==Query::SilentPath) ++pathQueries;return true;}}
Point fixtureForward{0,0,1};
bool relativeBearing(Point delta,double& pan,double& front) {
    pan=delta.x*fixtureForward.z-delta.z*fixtureForward.x;
    front=delta.x*fixtureForward.x+delta.z*fixtureForward.z;return true;
}
bool cameraBasis(Point& forward,Point& right) { forward=fixtureForward; right={forward.z,0,-forward.x}; return true; }
std::wstring direction(Point p) { return std::abs(p.x)>std::abs(p.z)?p.x>0?L"east":L"west":p.z>0?L"north":L"south"; }
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring out; for(const auto& p:parts) { if(!out.empty()) out+=separator; out+=p; } return out;
}
}
using namespace ffx;
void check(bool result,const char* reason) { if(!result) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); } }
void dispatch(unsigned rva,bool before,Event& event) {
    event.rva=rva; for(const auto& listener:hooks[rva]) { const auto f=before?listener.before:listener.after; if(f) f(event); }
}
void position(uintptr_t actor,Point p) { write<float>(actor+0xc,static_cast<float>(p.x)); write<float>(actor+0x10,static_cast<float>(p.y)); write<float>(actor+0x14,static_cast<float>(p.z)); }
void control(bool on) { write<uint8_t>(script.controller,on?2:0); write<uintptr_t>(address(0xf00740),on?playerActor():0); }
void signal(unsigned local,unsigned callback,unsigned pc) {
    const auto w=worker(local),s=base+0x1400000+local*0x40;
    write<uintptr_t>(w+0x90,s); write<uint16_t>(s+8,static_cast<uint16_t>(callback)); write<uint8_t>(s+0xe,1); write<uint8_t>(w+0x32,1);
    write<uintptr_t>(w+0x12c+0x4c+0x18,script.chunk+script.codeOffset+pc);
}
void disable(unsigned local,unsigned callback,unsigned pc,bool ancestry=true) {
    signal(local,callback,pc);
    Event interpreterEvent; interpreterEvent.args[0]=worker(local);
    if(ancestry) dispatch(0x464180,true,interpreterEvent);
    Event disabled; disabled.parentRva=ancestry?0x464180:0;
    dispatch(0x471980,true,disabled); control(false);
    if(ancestry) dispatch(0x464180,false,interpreterEvent);
}
void restore() { control(true); Event e; dispatch(0x471a50,false,e); }
void complete(unsigned local,unsigned callback) {
    signal(local,callback,0); Event e; e.args[0]=local; dispatch(0x469170,true,e); dispatch(0x469170,false,e);
}
void request(unsigned local,unsigned callback) {
    signal(local,callback,0); Event e; e.args[0]=worker(local); e.args[1]=read<uintptr_t>(worker(local)+0x90);
    dispatch(0x469040,false,e);
}
unsigned frame(unsigned held=0xffffffff) {
    if(held==0xffffffff) held=fixtureConsent;
    Event e; dispatch(0x471d10,true,e);
    for(unsigned rva:{0xf27080u,0xf27084u,0xf27088u,0xf2708cu,0xf27090u,0xf27094u}) write<unsigned>(address(rva),held);
    updatePedestalInput(); return read<unsigned>(address(0xf27080));
}
Entity setup(unsigned controlKind,unsigned local,Point p) {
    cancelPedestal(); frame(0); ++epoch; values.clear(); spoken.clear(); route.clear(); walking=false;
    pedestalTime+=1000;
    enabled=focused=verified=true; battle=menu=console=false; scene=dialogue=0;
    fixtureConsent=controlKind==5 || controlKind==6?0x20:0x40; padOwned=false;inputDeviceMark.store(0);observeInputDevice(false);
    for(unsigned i=0;i<16;++i) remap[i]=i;
    const char* names[]={"","bsyt0100","klyt1000","djyt0600","mcyt0500","djyt0600","dome0700","bvyt1100"};
    script={base+0x1200000,worker(0),base+0x1500000,base+0x1100000,names[controlKind],200000,0x2000,96};
    write<uintptr_t>(address(0xf26ae8),script.controller);
    for(unsigned i=0;i<96;++i) {
        const auto w=worker(i),h=base+0x1500000+i*0x40,a=base+0x1300000+i*0x880;
        write<uintptr_t>(w,h); write<uintptr_t>(w+4,script.chunk); write<uint16_t>(w+0x2e,static_cast<uint16_t>(i));
        write<uint16_t>(h+8,32); write<uint8_t>(h,4); write<uint16_t>(w+0x34,0x80); write<uint8_t>(w+0xaa,1); write<uint8_t>(w+0xab,0x7f);
        write<uintptr_t>(w+0x9c,a); write<uint16_t>(a+2,1);
        write<float>(a+0x5c,2); write<float>(a+0x64,2); write<float>(a+0x4e4,1);
        position(a,p);
        write<float>(w+0x558,static_cast<float>(p.x)); write<float>(w+0x55c,static_cast<float>(p.y)); write<float>(w+0x560,static_cast<float>(p.z));
        write<float>(w+0x558+0x1c,10); write<float>(w+0x558+0x2c,10); write<float>(w+0x558+0x40,5);
        write<float>(w+0x558+0x3c,3.1415927f);
    }
    write<uint8_t>(read<uintptr_t>(worker(local)),controlKind==5 || controlKind==6?3:1);
    position(playerActor(),{p.x,p.y,p.z+4.25}); control(true);
    const auto config=base+0x1600000; write<uintptr_t>(address(0x1efb504),config);
    for(unsigned i=0;i<128;++i) { write<unsigned>(config+0x60+i*4,0); write<uint8_t>(address(0x830e90)+i,static_cast<uint8_t>(i)); }
    const unsigned movement[]={0xc8,0xd0,0xcd,0xcb};
    for(unsigned i=0;i<4;++i) {
        write<unsigned>(config+0x60+i*4,i+29); write<uint8_t>(address(0x830e90)+i+1,static_cast<uint8_t>(movement[i]));
    }
    write<uint8_t>(address(0x830e90)+49,0x1c); write<uint8_t>(address(0x830e90)+52,0x39);
    write<uint8_t>(address(0x830e90)+90,0x2c); write<uint8_t>(address(0x830e90)+99,0x9c);
    write<unsigned>(config+0x60+4*4,77); write<unsigned>(config+0x60+53*4,127); write<unsigned>(config+0x60+18*4,12);
    write<unsigned>(config+0x60+5*4,80); write<unsigned>(config+0x60+54*4,118); write<unsigned>(config+0x60+19*4,13);
    Entity target; target.name=L"Pedestal"; target.worker=local; target.position=p;
    configurePedestalTarget(target,names[controlKind],local);
    return target;
}
PedestalPlan kilikaPlan;
void ready(Entity& target) { refreshTargetZone(target,worker(target.pushLocal),1); armPedestal(target); }
PedestalSnapshot snapshot(Point p,std::initializer_list<PedestalBindings::Value> vars) {return {p,PedestalBindings(vars)};}
PedestalPlan measuredPlan(unsigned control,unsigned local,const PedestalSnapshot& s,const char* reason) {
    pedestalAllocations=0;trackPedestalAllocations=true;
    const auto plan=pedestalPlan(control,local,s);
    trackPedestalAllocations=false;
    check(pedestalAllocations==0,reason);
    check(plan.moves.size()<=3,"production pedestal plan stays within three fixed move rows");
    return plan;
}
void ruleTests() {
    auto s=snapshot({-14.4117,40,-65.6},{{0x5c0000cc,1},{0x5c000100,0}});
    auto p=measuredPlan(1,7,s,"Besaid production plan allocates no heap"); check(p.moves.size()==1 && p.moves[0].goal.z< -147,"Besaid loaded pedestal routes to original depression");
    check(describePedestalPlan(p,s.position).find(L"7.4")!=std::wstring::npos,"Besaid guidance announces remaining slide distance");
    check(describePedestalApproach(p,s.position,{s.position.x+4,s.position.y,s.position.z})==
        L"You are on the east side of the pedestal. Push from the north side.",
        "push guidance distinguishes current side from required opposite side");
    check(describePedestalApproach(p,s.position,{s.position.x,s.position.y,s.position.z+4})==
        L"You are on the north side of the pedestal. Push from the north side.",
        "correct push-side guidance agrees with original Besaid motion");
    s.values[0x5c0000cc]=0; check(!pedestalPlan(1,7,s).blocked.empty(),"Besaid final push requires inserted sphere");
    s=snapshot({.2215,-10,144.1042},{{0x0c00005a,1},{0x0c000064,0}}); p=measuredPlan(2,6,s,"Kilika production plan allocates no heap");
    check(p.moves.size()==3 && p.moves[0].goal.x==s.position.x && p.moves[0].goal.z==170 && p.moves[0].held==0x1000 &&
        p.moves[1].goal.x>50 && p.moves[1].goal.z>=170 && p.moves[2].goal.z>196,"Kilika clears the floor boundary before crossing original depression edge");
    check(describePedestalApproach(p,s.position,{s.position.x+4,s.position.y,s.position.z}).find(L"Push from the south side.")!=std::wstring::npos,
        "Kilika alignment begins on the south side before the eastbound turn");
    auto boundary=snapshot({20.46,-10,144.6},{{0x0c00005a,1},{0x0c000064,0}});
    const auto boundaryPlan=measuredPlan(2,6,boundary,"Kilika boundary recovery plan allocates no heap");
    check(boundaryPlan.moves.size()==3 && boundaryPlan.moves[0].goal.x==boundary.position.x &&
        boundaryPlan.moves[0].goal.z==170 && boundaryPlan.moves[0].held==0x1000,
        "Kilika stopped at the diagonal floor boundary plans north to 170 first");
    // klyt1000 w6 entry 4 (2CE3) finishes the trial for socket 0x5a of 1, 2 or
    // 3: a Kilika Sphere. The guide's step 11 insert writes 2. 4 is the
    // Glyph Sphere the pedestal starts with, and 0 is empty.
    for(int sphere:{2,3}) {
        auto loaded=snapshot({.2215,-10,144.1042},{{0x0c00005a,sphere},{0x0c000064,0}});
        check(pedestalPlan(2,6,loaded).blocked.empty() && !pedestalPlan(2,6,loaded).moves.empty(),"Kilika push accepts every Kilika Sphere value");
    }
    for(int sphere:{0,4}) {
        auto other=snapshot({.2215,-10,144.1042},{{0x0c00005a,sphere},{0x0c000064,0}});
        check(!pedestalPlan(2,6,other).blocked.empty(),"Kilika push stays blocked without a Kilika Sphere");
    }
    s.position.x=50.01099; p=measuredPlan(2,6,s,"Kilika segmented production plan allocates no heap");
    check(describePedestalApproach(p,s.position,{s.position.x-4,s.position.y,s.position.z}).find(L"Push from the south side.")!=std::wstring::npos,
        "Kilika side guidance changes after the original alignment segment");
    s=snapshot({0,-18,24},{{0x0c000099,1},{0x0c00009a,1},{0x0c00009b,0},{0x0c00009d,0},{0x0c00009e,0},{0x0c0000a1,1},{0x0c0000d9,0},{0x0c0000db,0}});
    check(s.values.count==8,"Djose production snapshot contains all eight required facts");
    p=measuredPlan(3,1,s,"Djose production plan allocates no heap"); check(p.moves[0].goal.x==70 && p.moves[0].held==0x2000,"empty Djose charger push targets verified volume");
    // Only the charger's own write (0xa1=2) completes the charging push. A
    // released push near the charger is not complete.
    s.position.x=70; p=pedestalPlan(3,1,s); check(!p.complete,"Djose charger push is not complete from position alone");
    s.values[0x0c0000a1]=2; check(pedestalPlan(3,1,s).complete,"Djose charger push completes from the charger signal");
    s.values[0x0c0000a1]=1;
    // Guide push text: a plan the step does not want never says complete.
    check(pedestalStepText(pedestalPlan(3,1,s),false,true).find(L"not ready for the gap")!=std::wstring::npos,"gap step on the charging plan says the pedestal is not ready");
    s.values[0x0c0000a1]=2; check(pedestalStepText(pedestalPlan(3,1,s),false,false).find(L"not finished")!=std::wstring::npos,"complete plan without the step signal is not called complete");
    check(pedestalStepText(pedestalPlan(3,1,s),true,false).empty(),"complete plan with the step signal keeps the normal text");
    s.values[0x0c0000a1]=1;
    s.position.x=0; s.values[0x0c00009b]=3; s.values[0x0c00009d]=s.values[0x0c00009e]=1;
    p=measuredPlan(3,1,s,"loaded Djose production plan allocates no heap"); check(p.phase==1 && p.moves[0].held==0x1000 && p.moves[0].goal.z>85,"loaded Djose gap uses original launch threshold");
    s=snapshot({34,39,12},{{0x0c0000a3,0},{0x0c0000c6,4},{0x0c000096,4},{0x0c000097,4}});
    const int phases[]={0,1,3,5}; const unsigned input[]={0x2000,0x1000,0x2000,0x8000};
    for(unsigned i=0;i<4;++i) { s.values[0x0c0000a3]=phases[i]; p=measuredPlan(4,1,s,"Macalania production plan allocates no heap"); check(p.moves.size()==1 && p.moves[0].held==input[i] && p.moves[0].mode==PushMode::Scripted,"Macalania preserves native stage/camera input"); }
    s.values[0x0c0000a3]=0; p=pedestalPlan(4,1,s);
    check(describePedestalApproach(p,s.position,{s.position.x+4,s.position.y,s.position.z}).find(L"Push from the south side.")!=std::wstring::npos,
        "Macalania approach uses world motion rather than its differently mapped raw input direction");
    s.values[0x0c0000a3]=2; check(!pedestalPlan(4,1,s).blocked.empty(),"wrong Macalania branch cannot become first-mound success");
    for(unsigned local=56;local<=60;++local) { s=snapshot({},{{0x0c0000de+local-56,0}}); p=pedestalPlan(5,local,s); check(p.moves.size()==1 && p.moves[0].mode==PushMode::Single,"each upper Djose pedestal uses one original Confirm"); }
    // A single Confirm push moves the pedestal by itself, so the text names the
    // button and the direction the pedestal moves, not a push direction that
    // can contradict the side the player stands on.
    { s=snapshot({},{{0x0c0000de,0}}); p=pedestalPlan(5,56,s);
      const auto text=describePedestalPlan(p,s.position,L"Confirm");
      check(text.rfind(L"Press Confirm once. The pedestal then moves ",0)==0 && text.find(L"Push ")==std::wstring::npos,"single Confirm push names the button, then the pedestal movement"); }
    for(unsigned local=44;local<=47;++local) { s=snapshot({},{{0x0c000021,0}}); check(pedestalPlan(6,local,s).moves[0].mode==PushMode::Single,"each Zanarkand pedestal activates its pattern once"); }
    bool missingRejected=false;
    try { (void)pedestalPlan(1,7,snapshot({},{{0x5c0000cc,1} })); }
    catch(const std::out_of_range&) { missingRejected=true; }
    check(missingRejected,"missing required pedestal binding remains rejected");
}
void boundedApproachFixtures() {
    const Point reset{.2215,-10,144.1042};
    const auto begin=[&](Point player) {
        auto target=setup(2,6,reset);values[0x0c00005a]=2;
        position(playerActor(),player);refreshTargetZone(target,worker(6),1);armPedestal(target,true);frame(0);
        return target;
    };
    // Player movement and repeated route starts must not buy an endless orbit.
    begin({reset.x+60,reset.y,reset.z});
    const auto orbitStart=pedestalTime;
    for(unsigned i=1;i<=300 && capturePedestalNavigationState().armed;++i) {
        const double angle=i*.08;
        position(playerActor(),{reset.x+60*std::cos(angle),reset.y,reset.z+60*std::sin(angle)});
        pedestalTime+=100;if(i%40==0) walking=false;frame(0);
    }
    check(!capturePedestalNavigationState().armed && !walking && pedestalTime-orbitStart<=30000,
        "orbiting reset approach stops and releases input within 30 seconds across replans");
    const auto requestsAfter=routeRequests,speechAfter=spoken.size();frame(0);frame(0);
    check(spoken.size()==speechAfter && routeRequests==requestsAfter && !spoken.empty() &&
        spoken.back()==L"Auto walk stopped: no progress",
        "timed-out orbit gives one final reason and cannot restart itself");

    begin({reset.x+200,reset.y,reset.z-4});
    const auto improvingStart=pedestalTime;
    for(unsigned i=1;i<=300 && capturePedestalNavigationState().armed;++i) {
        position(playerActor(),{reset.x+200-i*.15,reset.y,reset.z-4});
        pedestalTime+=100;if(i%40==0) walking=false;frame(0);
    }
    check(!capturePedestalNavigationState().armed && pedestalTime-improvingStart==30000 && !walking,
        "even improving approaches share a hard 30-second deadline across retries");

    begin({reset.x,reset.y,reset.z-40});
    for(unsigned i=1;i<=150;++i) {
        // A changing waypoint and continuous sideways oscillation are not contact progress.
        position(playerActor(),{reset.x+(i%2?3.0:-3.0),reset.y,reset.z-40});
        pedestalTime+=100;frame(0);
    }
    check(!capturePedestalNavigationState().armed && !walking,
        "sideways movement without final-contact improvement stops after 15 seconds");
    approachDiagnostics.clear();
    logCaptureHandler.store([](const LogCapture& packet) noexcept {
        if(std::strstr(packet.format.data(),"pedestal approach") && packet.formatter) {
            char text[1024]{};packet.formatter(packet,text,sizeof(text));approachDiagnostics.emplace_back(text);
        }
    });
    begin({reset.x,reset.y,reset.z-40});
    for(unsigned i=1;i<=30;++i) {
        position(playerActor(),{reset.x,reset.y,reset.z-40+i*.3});pedestalTime+=100;frame(0);
    }
    logCaptureHandler.store(nullptr);
    check(approachDiagnostics.size()==8 && approachDiagnostics.front().find("elapsed 0 ms")!=std::string::npos &&
        approachDiagnostics[6].find("elapsed 3000 ms")!=std::string::npos &&
        approachDiagnostics[7].find("pedestal 0.221 -10.000 144.104")!=std::string::npos &&
        approachDiagnostics[7].find("along 31.000, across 0.000, radius 4.000")!=std::string::npos,
        "approach logs copy elapsed time, live geometry, alignment and radius once per second");
    cancelPedestal();frame(0);
    std::puts("PASS: pedestal orbit, contact-progress and whole-approach deadline fixtures");
}
void liveResetContactFixture() {
    const Point reset{.2215,-10,144.1042},live{2.2215,-10,145.1042};
    auto target=setup(2,6,reset);values[0x0c00005a]=2;
    position(read<uintptr_t>(worker(6)+0x9c),live);
    position(playerActor(),{live.x,live.y,live.z-4.25});
    refreshTargetZone(target,worker(6),1);OwnedNavTarget staging;check(captureOwnedNavTarget(target,staging),"copy reset staging shape");
    armPedestal(target,true);frame(0);
    check(capturePedestalNavigationState().contact,"south staging at a shifted live reset pedestal enters contact phase");
    NativeRouteTarget copied;check(pedestalTargetForTest(copied) && distanceXZ(copied.position,live)<.001 &&
        distanceXZ(copied.zone.center,live)<.001 && !route.empty() &&
        std::abs(route.back().x-live.x)<.001 && route.back().z<live.z-2,
        "south reset contact route uses the live actor centre once, not the reset tile or a double offset");
    updatePedestalOwnedTarget(staging.native);frame(0);
    check(pedestalTargetForTest(copied) && copied.zone.contactRadius==0 && copied.zone.range>0 && walking,
        "a delayed staging observation cannot replace the active south contact walk");
    position(playerActor(),{live.x,live.y,live.z-3.9});disable(6,4,0x274b);
    check((read<uint16_t>(address(0xf27080))&0xf000)==0x1000 && capturePedestalNavigationState().armed,
        "centred south-side native contact claims the north push at the reset spot");
    restore();cancelPedestal();frame(0);
    target=setup(2,6,reset);values[0x0c00005a]=2;
    for(const auto actor:{read<uintptr_t>(worker(6)+0x9c),playerActor()}) {
        write<float>(actor+0x5c,3.5f);write<float>(actor+0x64,3.5f);
    }
    position(playerActor(),{reset.x+.9,reset.y,reset.z-8.5});
    refreshTargetZone(target,worker(6),1);armPedestal(target,true);frame(0);
    check(capturePedestalNavigationState().contact && walking,
        "coarse-stride reset staging hands off while safely outside seven-unit contact");
    position(playerActor(),{reset.x+.9,reset.y,reset.z-6.9});disable(6,4,0x274b);
    check((read<uint16_t>(address(0xf27080))&0xf000)==0x1000,
        "coarse-stride staging still claims only an aligned north push");
    restore();cancelPedestal();frame(0);
    std::puts("PASS: live shifted reset-centre south approach and native contact");
}
void axialResetRouteFixture() {
    auto target=setup(2,6,{.222,-10,144.104});values[0x0c00005a]=2;
    for(const auto actor:{read<uintptr_t>(worker(6)+0x9c),playerActor()}) {
        write<float>(actor+0x5c,3.5f);write<float>(actor+0x64,3.5f);
    }
    position(playerActor(),{5.285,-10,134.055});refreshTargetZone(target,worker(6),1);
    const auto points=interactionRoute(readMesh(),playerPosition(),target.zone);
    check(points.size()>=3,"reset staging route retains an axial entry leg");
    const auto entry=points[points.size()-2],stage=points.back();
    check(std::abs(entry.x-.222)<.001 && std::abs(stage.x-.222)<.001 && stage.z-entry.z>=4-.001,
        "east-side reset approach reaches the south centre line before walking north to staging");
    cancelPedestal();frame(0);
}
// In-game F25 and F26 (Kilika step 15 and the sphere steps next to it).
void kilikaWalkFixtures() {
    Entity slope;slope.pushControl=2;slope.position={50,-10,170};slope.zone.contactRadius=7;slope.zone.height=-1;
    slope.zone.approach={0,0,-1};slope.zone.contactMinY=-25;slope.zone.contactMaxY=0;
    check(pedestalContactZone(slope) && inInteractionZone(slope.zone,{50,-23,163.5}) &&
        !inInteractionZone(slope.zone,{50,-26,163.5}),
        "contact entry uses native horizontal radius and vertical bounds on a slope");
    {
        auto centre=setup(2,6,{50,-10,170});values[0x0c00005a]=2;
        position(playerActor(),{50,-10,165.75});refreshTargetZone(centre,worker(6),1);armPedestal(centre,true);frame(0);
        check(capturePedestalNavigationState().contact,"Kilika third leg enters the contact walk from its south stage");
        refreshTargetZone(centre,worker(6),1);
        check(centre.zone.approach.z==-1 && !inInteractionZone(centre.zone,{48.12,-10,166.5}),
            "Kilika third-leg live contact refresh keeps the south centre line");
        const auto countPush=[] {unsigned count=0;for(const auto& text:spoken) if(text==L"Pushing pedestal north") ++count;return count;};
        const auto said=countPush();position(playerActor(),{56,-10,170});disable(6,4,0x274b);restore();
        refreshTargetZone(centre,worker(6),1);OwnedNavTarget copied{};captureOwnedNavTarget(centre,copied);updatePedestalOwnedTarget(copied.native);
        position(playerActor(),{50,-10,165.75});frame(0);
        check(said==1 && countPush()==said,"a re-approach does not repeat Pushing pedestal for the same leg");
    }
    const Point at{.2215,-10,144.1042};
    const auto kilika=[&](Point player) {
        auto target=setup(2,6,at); values[0x0c00005a]=2;
        position(playerActor(),player); refreshTargetZone(target,worker(6),1); armPedestal(target,true);
        return target;
    };
    // F26 (b): a push contact after another hook already stopped the walk
    // must not stop it again. That bumps the manual input revision, which
    // rejected the mod's own queued route ("manual input revision (612, 613)").
    auto target=kilika({at.x,at.y,at.z-3}); walking=false;
    auto stops=walkStops; disable(6,4,0x274b);
    check(walkStops==stops && (read<uint16_t>(address(0xf27080))&0xf000)==0x1000,
        "a Kilika push contact does not stop a walk that already stopped");
    // F26 (c): the push ended with the pedestal still in place. Do not hold
    // the direction against it; walk back out and in again.
    restore();
    check(frame(0)==0 && capturePedestalNavigationState().armed && !capturePedestalNavigationState().contact,
        "a Kilika push that ended short holds no direction and stays armed");
    for(unsigned i=0;i<2;++i) {
        refreshTargetZone(target,worker(6),1); OwnedNavTarget copied{}; captureOwnedNavTarget(target,copied);
        updatePedestalOwnedTarget(copied.native); frame(0);
        disable(6,4,0x274b); restore(); frame(0);
    }
    check(!capturePedestalNavigationState().armed && !spoken.empty() &&
        spoken.back()==L"Pedestal push did not start. Walk into the pedestal from the push side by hand.",
        "three Kilika pushes that do not move the pedestal stop and say so");
    restore();
    // In-game round 9: fourteen Kilika pushes in a row each moved the
    // pedestal under 1 unit, and none counted toward the cap. Short pushes
    // count, so the third one stops with its own message.
    {
        const auto pedestal=base+0x1300000+6*0x880;
        target=kilika({at.x,at.y,at.z-7.5});
        for(unsigned i=0;i<3;++i) {
            refreshTargetZone(target,worker(6),1); OwnedNavTarget copied{}; captureOwnedNavTarget(target,copied);
            updatePedestalOwnedTarget(copied.native); frame(0);
            disable(6,4,0x274b); frame(0);
            position(pedestal,{at.x+.29*(i+1),at.y,at.z+.55*(i+1)}); restore(); frame(0);
        }
        check(!capturePedestalNavigationState().armed && !spoken.empty() &&
            spoken.back()==L"Pedestal push keeps stopping after a short move. Walk into the pedestal from the push side by hand.",
            "three short Kilika pushes in a row stop and say so");
        position(pedestal,at);
    }
    // After the 24-unit push the pedestal was within 1 of the east goal,
    // but north of the line. The plan was not worked out again, so it said
    // "Push from the north side. Push south 1.5 steps". Plan again from the
    // pedestal's live position after every push.
    {
        const auto pedestal=base+0x1300000+6*0x880;
        target=kilika({at.x,at.y,at.z-7.5});
        disable(6,4,0x274b); frame(0);
        const Point after{50.0109901428-.5,at.y,at.z+16};
        position(pedestal,after); restore(); frame(0);
        const auto state=capturePedestalNavigationState();
        check(state.armed && state.plan.moves.size()==1 && std::abs(state.plan.moves[0].goal.z-198.2762908936)<.01 &&
            state.plan.moves[0].held==0x1000,
            "after a push leaves the pedestal near the east goal, the next push is north from its live position");
        cancelPedestal(); frame(0); position(pedestal,at);
    }
    // F26: a contact near the corner (1.7 across the push axis, inside the
    // native 2) took control but did not push. Treat it as off the push side.
    kilika({at.x+1.7,at.y,at.z-3});
    write<uint16_t>(address(0xf27080),0x2000); disable(6,4,0x274b);
    check((read<uint16_t>(address(0xf27080))&0xf000)==0 && capturePedestalNavigationState().armed,
        "a Kilika walk-in contact near the corner is not pushed and stays armed");
    restore();
    // F26 (a): a walk to the push side that ended short (another hook
    // stopped it at a touch) is requested again.
    target=kilika({at.x-14,at.y,at.z+9}); frame(0);
    const auto asked=routeRequests;
    check(walking,"a Kilika walk-in push starts its approach walk");
    walking=false; frame(0);
    check(routeRequests==asked+1 && walking,"a Kilika approach walk that ended short is requested again");
    {
        cancelPedestal();frame(0);target=kilika({at.x+44,at.y,at.z+44});frame(0);
        const auto beforeSpeech=spoken.size(),beforeRequests=routeRequests;
        for(unsigned i=0;i<8;++i) {
            check(pedestalRetryNoProgress(),"an active push-side approach owns its no-progress retry");
            walking=false;frame(0);
            check(spoken.size()==beforeSpeech+(i==7?1:0),"approach retries stay silent until one final reason");
        }
        check(!walking && !capturePedestalNavigationState().armed && routeRequests==beforeRequests+7 &&
            spoken.back()==L"Auto walk stopped: no progress","exhausted stalled approach reports its cause and stops requesting routes");
        frame(0);frame(0);
        check(spoken.size()==beforeSpeech+1 && routeRequests==beforeRequests+7,"finished push session cannot repeat stop speech or retry");
        target=kilika({at.x-14,at.y,at.z+9});frame(0);
    }
    // Integration review: Backslash pressed again during the walk stops it
    // through stopAutoWalk only ("Auto walk stopped"). The approach must not
    // ask for the route again, or the walk restarts on its own.
    {
        const auto again=routeRequests; stopAutoWalk(); frame(0); frame(0);
        check(routeRequests==again && !walking && !capturePedestalNavigationState().armed,
            "a Kilika approach walk the player stopped is not requested again");
    }
    cancelPedestal(); frame(0);
    target=kilika({at.x-14,at.y,at.z+9});frame(0);
    const auto beforeRouteFailure=routeRequests;
    logCaptureHandler.store([](const LogCapture& packet) noexcept {
        if(std::strstr(packet.format.data(),"pedestal assistance stopped:") && packet.formatter)
            packet.formatter(packet,lastStopReason,sizeof(lastStopReason));
    });
    walking=false;++inputRevision;++routeFailuresFixture;frame(0);logCaptureHandler.store(nullptr);
    check(!std::strcmp(lastStopReason,"pedestal assistance stopped: approach route lost") &&
        routeRequests==beforeRouteFailure && !capturePedestalNavigationState().armed,
        "a lost approach route reports its cause without blaming player input or restarting");
    // F27: on the reset tile north-east of the pedestal, every approach walk
    // touched it off the push side, farther out than the modelled contact
    // radius (nine touches, no move). Learn where the touch happened; the
    // next walk steps straight out of it and stays out.
    {
        const Point touch{at.x+6,at.y,at.z+6};const double out=distanceXZ(touch,at);
        target=kilika(touch); walking=true; frame(0);
        disable(6,4,0x274b);
        check(capturePedestalNavigationState().armed && capturePedestalNavigationState().touch>=out+.2,
            "a Kilika touch off the push side records how far out it happened");
        restore(); walking=false; route.clear(); frame(0);
        const double ox=route.size()>1?route[1].x-route[0].x:0,oz=route.size()>1?route[1].z-route[0].z:0;
        bool clear=route.size()>2;for(size_t i=0;i+1<route.size();++i) clear=clear && distanceXZ(route[i],at)>=out-.01;
        check(clear && ox>0 && oz>0 && std::abs(ox-oz)<.05*std::hypot(ox,oz),
            "the next Kilika approach walk steps straight out of that touch and stays out");
        cancelPedestal(); frame(0);
    }
    // F25: an ordinary auto walk (to the sphere recess) touched the pedestal.
    // The pedestal script gets no direction and the walk resumes.
    setup(2,6,at); values[0x0c00005a]=2; position(playerActor(),{at.x-5.4,at.y,at.z+1});
    walking=true; frame(0);
    write<uint16_t>(address(0xf27080),0x2000); disable(6,4,0x274b);
    check((read<uint16_t>(address(0xf27080))&0xf000)==0 && !walking,"an auto walk touching the Kilika pedestal gives its script no direction");
    // The push script gives control back at 2F70 but turns field interaction
    // on only after wait(5) (2F76-2F82). Until then the pedestal reads
    // inactive, and in game round 9 the resumed walk was refused
    // ("auto walk refused: destination inactive worker=6"). Wait first.
    auto started=starts; restore(); frame(0);
    check(starts==started && !walking,"the interrupted walk waits while the pedestal still has interaction off");
    for(unsigned i=0;i<30;++i) frame(0);
    check(starts==started+1 && walking,"the interrupted walk resumes once interaction is back");
    legValid=legArrived=true; frame(0);
    // Step 8 via the plain Pedestal row: the walk arrived, the touch pushed
    // the player out of reach, and facing failed. Ask for the route again.
    faceResult=false; disable(6,4,0x274b); restore(); auto faced=faces; auto asked8=pathQueries; started=starts; frame(0);
    check(pathQueries==asked8 && faces==faced,"an interrupted walk that had arrived waits before it routes again");
    for(unsigned i=0;i<30;++i) frame(0);
    check(faces==faced+1 && starts==started,"an interrupted walk that had arrived faces its target instead");
    check(pathQueries==asked8+1,"an interrupted walk that had arrived but cannot face its target is routed again");
    faceResult=true;
    legValid=legArrived=false; walking=true; frame(0);
    disable(6,4,0x274b); restore(); started=starts; for(unsigned i=0;i<30;++i) frame(0);
    check(starts==started,"a third touch within ten seconds does not resume the walk");
    walking=false; frame(0);
    write<uint16_t>(address(0xf27080),0x2000); disable(6,4,0x274b);
    check((read<uint16_t>(address(0xf27080))&0xf000)==0x2000,"a manual touch keeps the player's direction");
    restore(); frame(0);
}
int main() {
    base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2100000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(base!=0,"isolated fixture memory"); installPedestal(); ruleTests();
    auto activeTarget=setup(1,7,{0,40,-65.6}); values[0x5c0000cc]=1; ready(activeTarget);
    PedestalPlan activeCopy; pedestalAllocations=0; trackPedestalAllocations=true;
    const bool activeCopied=pedestalActivePlan(activeTarget,activeCopy);
    if(activeCopied) activeCopy.moves.erase(activeCopy.moves.begin(),activeCopy.moves.begin()+1);
    trackPedestalAllocations=false;
    check(activeCopied && activeCopy.moves.size()==1 && activeCopy.moves.front().goal.z< -147,
        "active Besaid plan copies bounded moves and fixed-row erase preserves the remaining endpoint");
    check(pedestalAllocations==0,"active-plan copy and fixed-row erase allocate no heap");
    for(unsigned kind=2;kind<=4;++kind) {
        auto push=setup(kind,kind==2?6:1,{10,0,140}); Entity recess;
        check(pedestalRecessTarget(push,worker(push.pushLocal),recess) && !recess.pushControl && recess.zone.talk &&
            !recess.inactive && recess.name==L"Pedestal sphere recess","integral sphere sockets retain separate ordinary Confirm destinations");
        armPedestal(recess); check(frame(0x20)==0x20,"sphere recess preserves ordinary Confirm without claiming push assistance");
    }
    auto target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    check(pedestalConsentKey(0x39) && pedestalConsentKey(0x2c) && !pedestalConsentKey(0x1c) && pedestalConsentButtons()==pad::bit(pad::East),"configured Space/alternate push and physical B publish consent separately from Confirm");
    check(pedestalInstruction(target).find(L"Do not press Space or Z at the pedestal")!=std::wstring::npos,"walk-push instructions name the configured keys that open the sphere prompt");
    { PedestalPlan held; held.moves.push_back({{0,0,50},0,PushMode::Slide});
      check(describePedestalPlan(held,{0,0,0},L"Space or Z").find(L"Hold Space or Z")!=std::wstring::npos,"held-push instructions announce actual configured push keys"); }
    padOwned=true;inputDeviceMark.store(0);observeInputDevice(true);
    check(pedestalInstruction(target).find(L"Do not press B at the pedestal")!=std::wstring::npos,"controller instructions announce actual configured push button");
    padOwned=false;inputDeviceMark.store(0);observeInputDevice(false);
    check(frame()==0 && walking && !route.empty(),"held Space starts actual DI/pad walking across the pedestal contact gap");
    refreshTargetZone(target,worker(7),1);
    check(target.zone.contactRadius==0 && target.zone.range>0 && frame()==0 && walking,"live refresh preserves native contact walk rather than stopping again at staging");
    check(target.zone.approach.z==1 && !inInteractionZone(target.zone,{target.position.x+1.88,target.position.y,target.position.z+3}),
        "contact walk retains its push-side centre line on live refresh");
    write<uint16_t>(address(0xf27080),0x4040); disable(7,4,0x12ab);
    check(read<uint16_t>(address(0xf27080))==0x1000,"owned disable replaces inward approach direction before same-interpreter initial input read");
    check(frame()==0x1000,"owned Besaid control disable preserves held movement");
    check(frame(0)==0,"release clears injection during original push animation");
    restore(); check(frame(0x20)==0x20,"released assistance does not resume or hijack a later ordinary Confirm");
    // Auto walk can touch the pedestal before the push key is held. The
    // original push script then disables control by itself at 12AB. That
    // contact must not move the pedestal or end the assistance: a later
    // held push still has to work.
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    check(frame(0)==0,"Besaid push assistance waits for the push key");
    write<uint16_t>(address(0xf27080),0x4000); disable(7,4,0x12ab);
    check((read<uint16_t>(address(0xf27080))&0xf000)==0,"contact push without the push key reads no direction");
    restore(); frame(0); frame();
    disable(7,4,0x12ab);
    check(frame()==0x1000,"assistance stays armed after a contact push without the push key");
    check(frame(0)==0,"release after the later push clears injection"); restore();
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1;
    position(playerActor(),{target.position.x,target.position.y,target.position.z+4}); ready(target);
    check(!inInteractionZone(target.zone,playerPosition()) && frame()==0 && walking && distanceXZ(route.back(),target.position)>4.1,
        "starting already touching routes outward to clear native contact before re-entry");
    position(playerActor(),route.back()); walking=false; frame();
    check(pedestalActiveContact(target) && walking,"holding Space re-enters only after reaching the outside stage");
    // Besaid and Kilika: Confirm at the pedestal runs its sphere prompt
    // (Kilika w6 entry 2, disable 0F98), so the push key cannot be pressed
    // there. The original push (Kilika w6 entry 4 at 274B, Besaid w7 entry 4
    // at 12AB) reads only a held direction, after contact from an axis-aligned
    // side (Kilika 27DF-28B9: more than 2 units out, under 2 across). A
    // backslash walk to the pedestal is the consent, and only a contact from
    // the planned side is pushed.
    target=setup(2,6,{.2215,-10,144.1042}); values[0x0c00005a]=2;
    position(playerActor(),{.2215,-10,144.1042-3}); refreshTargetZone(target,worker(6),1); armPedestal(target,true);
    disable(6,4,0x274b);
    // The first Kilika row pushes north (+z), raw 0x1000.
    check((read<uint16_t>(address(0xf27080))&0xf000)==0x1000,"Kilika contact from the planned side pushes without the push key");
    check(frame(0)==0x1000,"Kilika push keeps its direction without the push key");
    restore();
    target=setup(2,6,{.2215,-10,144.1042}); values[0x0c00005a]=2;
    position(playerActor(),{.2215+2,-10,144.1042+2}); refreshTargetZone(target,worker(6),1); armPedestal(target,true);
    write<uint16_t>(address(0xf27080),0x4000); disable(6,4,0x274b);
    check((read<uint16_t>(address(0xf27080))&0xf000)==0 && pedestalActivePlan(target,kilikaPlan),
        "Kilika corner contact reads no direction and stays armed");
    restore();
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; refreshTargetZone(target,worker(7),1); armPedestal(target,true);
    disable(7,4,0x12ab);
    check(frame(0)==0x1000,"Besaid backslash push from the planned side needs no push key");
    restore();
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    check(frame(0)==0 && !walking,"selecting a Besaid pedestal without backslash starts nothing");
    restore();
    target=setup(2,6,{.2215,-10,170}); values[0x0c00005a]=1; ready(target);
    const auto pedestal=read<uintptr_t>(worker(6)+0x9c);
    position(pedestal,{49.1,-10,170}); target.position={49.1,-10,170};
    refreshTargetZone(target,worker(6),1);
    check(target.zone.approach.x== -1 && target.zone.approach.z==0,
        "armed current segment keeps its west-side approach at .9 residual despite live plan threshold");
    target=setup(3,1,{0,-18,24}); values[0x0c000099]=values[0x0c00009a]=1; ready(target);
    position(playerActor(),target.zone.center); check(frame()==0x2000,"lower Djose uses configured Space consent with original scripted direction");
    disable(42,1,0xd17c); check(frame()==0x2000,"lower Djose retains owned direction without exposing sphere Confirm");
    check(frame(0)==0,"lower Djose stops input immediately when Space is released");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    frame(); disable(7,4,0x12ab); Event collected; dispatch(0x471d10,true,collected);
    for(unsigned rva:{0xf27080u,0xf27084u,0xf27088u,0xf2708cu,0xf27090u,0xf27094u}) write<unsigned>(address(rva),0xabcd0040);
    updatePedestalInput();
    for(unsigned rva:{0xf27080u,0xf27084u,0xf27088u,0xf2708cu,0xf27090u,0xf27094u})
        check(read<unsigned>(address(rva))>>16==0xabcd,"native 16-bit input operations preserve adjacent high-word canaries");
    check(read<uint16_t>(address(0xf27080))==0x1000,"word-sized native held input still pushes in the original direction");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target); frame(); disable(7,4,0x1402);
    check(frame()==0,"blocked-path dialogue disable cancels and drains held push"); frame(0); check(frame()==0x40,"blocked-path drain expires on physical release");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target); frame(); disable(7,4,0x12ab,false);
    check(frame()==0,"stale matching worker without interpreter ancestry cannot own disabled control");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target); frame(); disable(7,4,0x12ab);
    write<uintptr_t>(worker(7)+0x90,base+0x1401000); check(frame()==0,"signal replacement cannot inherit a previous push claim");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; position(playerActor(),{50,40,0}); ready(target);
    check(frame()==0 && walking && !route.empty(),"held Space uses ordinary autowalk to reach the correct side");
    check(frame(0)==0 && !walking,"release during approach stops walking immediately");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    check(frame(0x60)==0x20,"Confirm cancels pushing and remains available for sphere interaction");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    std::array<uint8_t,256> keys{}; keys[0xcd]=0x80; pedestalKeyboardInput(keys.data()); check(frame()==0,"manual keyboard movement has priority over a claimed push");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    pad::State padState; padState.held=pad::bit(pad::East); padState.lx=18000; pedestalControllerInput(padState);
    check(frame()==0,"push plus physical stick input cancels without synthetic movement");
    for(unsigned c=0;c<7;++c) {
        target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
        if(c==0) focused=false; if(c==1) menu=true; if(c==2) console=true; if(c==3) battle=true;
        if(c==4) scene=3; if(c==5) enabled=false; if(c==6) ++epoch;
        check(frame()==0,"context/map change cancels push input and drains consent");
    }
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; remap[12]=15; remap[15]=12;
    write<unsigned>(base+0x1600000+0x60+5*4,118); write<unsigned>(base+0x1600000+0x60+19*4,12); ready(target);
    check(pedestalConsentKey(0x2c) && !pedestalConsentKey(0x39) && pedestalConsentButtons()==pad::bit(pad::South),"actual remapped push keyboard and physical controller bindings supply consent");
    frame(); disable(7,4,0x12ab); check(frame()==0x8000 && pedestalLogicalInput(0x8000)==0x1000,"inverse native direction mapping supplies the original logical push direction");
    target=setup(4,1,{34,39,12}); values[0x0c0000c6]=4; values[0x0c000096]=4; values[0x0c000097]=4; ready(target);
    position(playerActor(),target.zone.center); check(frame()==0x2000,"Macalania starts correct native direction at side rectangle");
    disable(29,1,0xe2ad); signal(29,1,0xe2d5); check(frame()==0,"Macalania releases direction after scripted launch");
    values[0x0c0000a3]=1; complete(1,10); check(frame()==0,"Macalania completion drains held Confirm instead of launching next puzzle stage");
    target=setup(7,13,{-18.7,-453.2,263.1}); values[0x0c000029]=values[0x0c00001f]=1; ready(target); position(playerActor(),target.zone.center);
    request(38,15);
    for(unsigned i=0;i<21;++i) { check(frame()==0x8000,"Bevelle optional push keeps consent across repeated native probes"); disable(38,15,0x1c3d3); restore(); complete(38,15); }
    frame(); disable(38,15,0x1c407); check(frame()==0,"Bevelle releases input when original optional push commits");
    complete(38,15); check(frame()==0,"completed Bevelle ride does not turn held consent into another interaction");
    target=setup(6,44,{70,0,25}); ready(target); check(frame()==0x20,"Zanarkand receives exactly one original Confirm activation");
    check(read<unsigned>(address(0xf27090))==0x20,"single activation supplies an edge after a held approach");
    check(frame()==0,"held Confirm does not repeat a single pedestal activation");
    disable(44,2,0x747e); complete(44,2); check(frame()==0,"native single-action completion drains consent");
    target=setup(6,44,{70,0,25}); ready(target); frame(); disable(44,2,0x7480);
    check(frame()==0,"unrelated disable inside same single-action entry cannot claim control");
    const unsigned upperSites[]={0x14540,0x14700,0x148c0,0x14a80,0x14c40};
    const unsigned patternSites[]={0x747e,0x75f6,0x776e,0x78e6};
    for(unsigned local=56;local<=60;++local) {
        target=setup(5,local,{30,-67,117}); ready(target); check(frame()==0x20,"each upper Djose control receives one native Confirm edge");
        disable(local,2,upperSites[local-56]); check(frame()==0,"upper Djose animation never receives repeat Confirm");
        complete(local,2); check(frame()==0,"upper Djose original return completes and drains the held action");
    }
    for(unsigned local=44;local<=47;++local) {
        target=setup(6,local,{70,0,25}); ready(target); frame(); disable(local,2,patternSites[local-44]);
        complete(local,2); check(frame()==0,"each Zanarkand pattern animation owns only its exact native action");
    }
    target=setup(6,44,{70,0,25}); remap[5]=6; remap[6]=5;
    write<unsigned>(base+0x1600000+0x60+5*4,118); write<unsigned>(base+0x1600000+0x60+19*4,13); ready(target);
    check(pedestalConsentKey(0x2c) && !pedestalConsentKey(0x1c) && frame(0x40)==0x40,
        "native regional Confirm remap remains authoritative for one-shot pattern controls");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1;
    write<uintptr_t>(address(0x1efb504),0); ready(target);
    check(frame()==0x40 && !pedestalConsentKey(0x39) && !pedestalConsentButtons(),"missing native configuration cannot arm a synthetic push");
    target=setup(7,13,{-20.26033,-460.016,-36.25044});
    write<uint8_t>(read<uintptr_t>(worker(39)),2);
    position(worker(39)+0x54c,{-30.253,-459.998,-44.302});
    position(worker(39)+0x57c,{-9.742,-459.998,-44.302});
    ready(target);
    check(target.zone.valid && !target.zone.crossing && target.zone.center.z< -48,"Bevelle entrance stages safely before native crossing edge");
    position(playerActor(),target.zone.center); check(frame(0)==0 && !walking,"Bevelle staging does not push without held Confirm");
    check(frame()==0 && walking && !route.empty(),"held Confirm starts ordinary walking through Bevelle entrance trigger");
    const auto crossingStarts=starts;
    check(frame()==0 && walking && starts==crossingStarts,"Bevelle crossing remains active on subsequent held frames");
    refreshTargetZone(target,worker(13),1);
    check(target.zone.crossing && target.zone.shape==2,"live target refresh preserves the consented entrance crossing");
    request(39,6); check(frame()==0 && !walking,"native Bevelle entrance ride ends injected walking and drains consent");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1;
    write<unsigned>(base+0x1600000+0x60+5*4,29); ready(target);
    check(!pedestalConsentKey(0xc8),"movement-overlapping push never disables manual movement priority");
    cancelPedestal();
    // Fixed Native target/plan state has no heap-owned Entity to retire.
    const auto failures=retirement::failures();
    for(unsigned i=0;i<70;++i) {
        target=setup(1,7,{-14.4117,40,-65.6});values[0x5c0000cc]=1;ready(target);cancelPedestal();
    }
    check(retirement::failures()==failures,"fixed session reset cannot consume a retirement pool");
    target=setup(1,7,{-14.4117,40,-65.6});values[0x5c0000cc]=1;ready(target);
    PedestalPlan heldPlan;check(pedestalActivePlan(target,heldPlan) && !heldPlan.moves.empty(),"fixed session remains usable after repeated reset");
    // The real Native session can wait for a copied turn shape without either
    // ending consent or doing pathfinding itself. The shape is admitted later.
    target=setup(1,7,{-30,40,-65.6});values[0x5c0000cc]=1;ready(target);
    position(read<uintptr_t>(worker(7)+0x9c),{-14.411718,40,-65.6});
    const auto requestsBefore=routeRequests,helpBefore=automaticHelpRequests;delayRoute=true;frame();
    check(automaticHelpRequests==helpBefore,"automatic turn never requests the full manual pedestal instructions");
    check(pedestalActivePlan(target,heldPlan) && heldPlan.moves.size()==1 && !walking && routeRequests==requestsBefore,"turn withholds assistance while a fresh copied shape is pending");
    frame();check(pedestalActivePlan(target,heldPlan),"pending geometry does not retire an otherwise owned held session");
    refreshTargetZone(target,worker(7),1);OwnedNavTarget nextTarget{};check(captureOwnedNavTarget(target,nextTarget),"fixture copied turn target");
    updatePedestalOwnedTarget(nextTarget.native);delayRoute=false;frame();
    check(pedestalActivePlan(target,heldPlan),"fresh copied geometry keeps the original remaining turn plan");
    delayRoute=true;target=setup(1,7,{-14.4117,40,-65.6});values[0x5c0000cc]=1;ready(target);frame();
    check(capturePedestalNavigationState().held,"pending route retains explicit held consent");frame(0);check(!capturePedestalNavigationState().armed,"released consent retires a pending route before delayed movement can start");delayRoute=false;
    nativeOwner=false;cancelPedestal();nativeOwner=true;
    check(!pedestalActivePlan(target,heldPlan),"foreign cancellation immediately retires consent without touching Native fields");frame(0);
    axialResetRouteFixture();liveResetContactFixture();boundedApproachFixtures();kilikaWalkFixtures();
    // Drive the real session through the three observed native leg completions.
    // This checks speech ownership, not simulated collision or push physics.
    target=setup(2,6,{.2215,-10,144.1042});values[0x0c00005a]=2;fixtureForward={0,0,1};
    refreshTargetZone(target,worker(6),1);armPedestal(target,true);
    const auto helpAtStart=automaticHelpRequests;
    for(unsigned leg=0;leg<3;++leg) {
        refreshTargetZone(target,worker(6),1);
        OwnedNavTarget copied;check(captureOwnedNavTarget(target,copied),"speech leg target copies");updatePedestalOwnedTarget(copied.native);
        position(playerActor(),{target.position.x+target.zone.approach.x*(target.zone.contactRadius+.25),target.position.y,
            target.position.z+target.zone.approach.z*(target.zone.contactRadius+.25)});
        walking=false;frame(0);frame(0);
        if(leg==1) {
            disable(6,4,0x274b);restore();frame(0);
            refreshTargetZone(target,worker(6),1);captureOwnedNavTarget(target,copied);updatePedestalOwnedTarget(copied.native);
            walking=false;frame(0);frame(0);
        }
        PedestalPlan remaining;check(pedestalActivePlan(target,remaining) && !remaining.moves.empty(),"speech leg remains owned");
        target.position=remaining.moves.front().goal;position(read<uintptr_t>(worker(6)+0x9c),target.position);
        if(leg==2) values[0x0c000064]=1;
        frame(0);
    }
    frame(0);
    check(spoken==std::vector<std::wstring>{L"Pushing pedestal north",L"Pushing pedestal east",L"Pushing pedestal north",L"Pedestal push complete"} &&
        automaticHelpRequests==helpAtStart,"three-leg auto push speaks only direction once per leg and one completion");
    std::puts("PASS: all-trial pedestal sides, Space push/native contact lifecycle, remaps and cancellation");
    return 0;
}
