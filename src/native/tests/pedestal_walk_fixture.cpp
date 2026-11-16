// Real pedestal session and per-frame walker. Only copied worker handbacks,
// native API boundaries, input devices and speech sinks are synthetic.
#define FFX_PEDESTAL_WALK_INTEGRATION
#define main pedestalComponentFixtureEntry
#include "pedestal_fixture.cpp"
#undef main
#include "../field_controls.hpp"
#include "../text_plan.hpp"
namespace ffx {
uint64_t autoWalkClock() {return pedestalTime;}
std::array<unsigned,4> movementKeys() {return {0xc8,0xcb,0xd0,0xcd};}
bool swimmingGoalActive(const NativeRouteTarget&) {return false;}
void resetNavAudio() {}
FieldControlsSnapshot fieldControlsForSource(unsigned,uintptr_t) {return {};}
FieldControlsClassification classifyFieldControls(const FieldControlsSnapshot&) {return FieldControlsClassification::Blocking;}
// All starts below are silent. Fail if a test accidentally asks this host to
// prepare destination speech; pedestal speech uses the existing semantic sink.
void prepareTextPlan(TextPlan&,const char*,bool) {check(false,"unexpected destination speech preparation");}
TextRef planLiteral(TextPlan&,const wchar_t*) {return 0;}
TextRef planConcat(TextPlan&,TextRef,TextRef) {return 0;}
bool commitTextPlan(const TextPlan&) {return false;}
}
namespace {
Entity beginTrace(Point player) {
    stopAutoWalk();legValid=false;delayRoute=false;
    auto target=setup(2,6,{.222,-10.005,144.104});values[0x0c00005a]=2;
    for(const auto actor:{read<uintptr_t>(worker(6)+0x9c),playerActor()}) {
        write<float>(actor+0x5c,3.5f);write<float>(actor+0x64,3.5f);
    }
    position(playerActor(),player);refreshTargetZone(target,worker(6),1);armPedestal(target,true);
    OwnedNavTarget copied;check(captureOwnedNavTarget(target,copied),"trace copied native target");
    integrationLeg={{9.112,-10.006,138.140},{.222,-10.008,133.398},{.222,-10.005,136.854},copied.native,generation(),false};
    legValid=true;startAutoWalk(false);delayRoute=true;
    return target;
}
unsigned tick() {
    pedestalTime+=17;updateAutoWalk();frame(0);
    pad::State input;autoWalkController(input);
    return (input.ly>0?1:0)|(input.ly<0?4:0)|(input.lx>0?8:0)|(input.lx<0?2:0);
}
unsigned pushes() {unsigned count=0;for(const auto& text:spoken) if(text==L"Pushing pedestal") ++count;return count;}
void replay(const Point* samples,unsigned count) {
    beginTrace(samples[0]);bool released=false,north=false;
    const auto requests=routeRequests;
    approachDiagnostics.clear();
    logCaptureHandler.store([](const LogCapture& packet) noexcept {
        if(std::strstr(packet.format.data(),"pedestal approach") && packet.formatter) {
            char text[1024]{};packet.formatter(packet,text,sizeof(text));approachDiagnostics.emplace_back(text);
        }
    });
    // Logged observations are one second apart. The native tick still runs
    // every 17 ms, while the worker leg deliberately stays at the old waypoint.
    for(unsigned i=1;i<count;++i) for(unsigned frameIndex=1;frameIndex<=59;++frameIndex) {
        const double t=frameIndex/59.0;
        position(playerActor(),{samples[i-1].x+(samples[i].x-samples[i-1].x)*t,-10.008,
            samples[i-1].z+(samples[i].z-samples[i-1].z)*t});
        const auto bits=tick();released|=!bits;north|=bits==1;
    }
    check(released && north,"trace releases the sideways stride and turns north between worker observations");
    check(routeRequests==requests,"radial entry advances without a worker route request");
    logCaptureHandler.store(nullptr);
    check(approachDiagnostics.size()>=2,"trace retains bounded approach diagnostics");
    for(size_t i=0;i+1<approachDiagnostics.size();i+=2) {
        const auto at=approachDiagnostics[i].find(", distance ");double distance=0;
        double x=0,y=0,z=0,px=0,py=0,pz=0,gx=0,gy=0,gz=0;
        check(at!=std::string::npos && sscanf_s(approachDiagnostics[i].c_str()+at,", distance %lf",&distance)==1 &&
            sscanf_s(approachDiagnostics[i+1].c_str(),"pedestal approach geometry: player %lf %lf %lf, pedestal %lf %lf %lf, waypoint %lf %lf %lf",
                &x,&y,&z,&px,&py,&pz,&gx,&gy,&gz)==9 && std::abs(distance-std::hypot(x-gx,z-gz))<.002,
            "logged progress distance measures the displayed effective steering target");
    }
    // After replaying the failed movement, let the actual output drive a
    // 10 units/second plant. The stale worker cannot decide the handoff.
    for(unsigned i=0;i<600 && !capturePedestalNavigationState().contact;++i) {
        const auto bits=tick();const auto p=playerPosition();
        const double x=(bits&8?1.0:0)-(bits&2?1.0:0),z=(bits&1?1.0:0)-(bits&4?1.0:0);
        const double scale=x && z?.7071067811865476:1;
        position(playerActor(),{p.x+x*.17*scale,p.y,p.z+z*.17*scale});
    }
    check(capturePedestalNavigationState().contact && pushes()==1 && spoken.size()==1,"trace reaches contact phase once without duplicate speech despite frozen worker observations");
    pad::State input;autoWalkController(input);
    check(!input.lx && !input.ly,"staging handoff releases movement in the same native tick");
    const auto before=playerPosition();tick();autoWalkController(input);
    check(input.ly>0 && !input.lx,"next native tick uses the owned north contact leg");
    Point goal;check(autoWalkSteeringGoal(goal) && std::abs(goal.x-.222)<.001 && goal.z>before.z,
        "progress exposes the same north contact target used by the walker");
    position(playerActor(),{.222,-10.005,137.2});disable(6,4,0x274b);
    check((read<uint16_t>(address(0xf27080))&0xf000)==0x1000 && pushes()==1,
        "native south contact retains the original north push and one announcement");
    restore();cancelPedestal();frame(0);stopAutoWalk();delayRoute=false;
}
}
int main() {
    base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2100000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(base!=0,"isolated trace memory");installPedestal();installAutoWalk();
    const Point pass[]={{5.285,-10.007,134.055},{-4.449,-10.008,133.456},{-7.682,-10.008,132.661}};
    const Point oscillation[]={{-5.647,-10.008,133.881},{5.822,-10.008,133.467},
        {-5.549,-10.008,133.774},{5.914,-10.008,133.590}};
    replay(pass,_countof(pass));replay(oscillation,_countof(oscillation));
    beginTrace({-7.682,-10.008,132.661});
    const auto requests=routeRequests;delayRoute=false;
    for(unsigned i=0;i<120;++i) tick();
    check(routeRequests>requests,"a player stopped before handoff replans within two seconds");
    check(spoken.empty(),"stationary staging recovery stays silent");
    cancelPedestal();frame(0);stopAutoWalk();
    beginTrace({5,-10.008,133.4});integrationLeg.goal={5,-10.008,133.4};
    const auto missedRequests=routeRequests;
    for(unsigned i=0;i<18;++i) tick();
    check(routeRequests>missedRequests && !autoWalkActive(),"missed nonradial waypoint queues a replan within 306 ms");
    check(spoken.empty(),"missed-waypoint recovery stays silent");
    cancelPedestal();frame(0);stopAutoWalk();delayRoute=false;
    std::puts("PASS: both Kilika live traces, 17ms handoff, frozen worker, 10u/s recovery, shared steering target and single speech");
}
