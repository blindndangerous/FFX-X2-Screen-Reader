#include "fixture_runtime.hpp"
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
std::vector<std::string> inputDiagnostics;
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
unsigned pushes() {unsigned count=0;for(const auto& text:spoken) if(text.find(L"Pushing pedestal ")==0) ++count;return count;}
void inputTraceChecks() {
    beginTrace({5,-10.008,133.4});inputDiagnostics.clear();
    const auto camera=base+0x1700000;write<uintptr_t>(address(0x1f11430),camera);
    write<float>(camera+0x50,10);write<float>(camera+0x58,21);
    write<float>(camera+0x66c,10);write<float>(camera+0x674,20);
    logCaptureHandler.store([](const LogCapture& packet) noexcept {
        if(std::strstr(packet.format.data(),"pedestal input:") && packet.formatter) {
            char text[1024]{};packet.formatter(packet,text,sizeof(text));inputDiagnostics.emplace_back(text);
        }
    });
    for(unsigned i=0;i<30;++i) {
        pedestalTime+=33;updateAutoWalk();frame(0);
        std::array<uint8_t,256> keys{};autoWalkKeyboard(keys.data());
        pad::State pad;autoWalkController(pad);
    }
    check(inputDiagnostics.size()>5 && inputDiagnostics.size()<=20,"both emitters share the twenty-lines-per-second trace bound");
    uint64_t previous=0;
    for(const auto& line:inputDiagnostics) {
        unsigned long long time=0,dt=0,age=0;
        check(sscanf_s(line.c_str(),"pedestal input: t %llu dt %llu age %llu",&time,&dt,&age)==3 &&
            (dt==33 || (!previous && dt==0)) && age==0 && (!previous || time-previous>=50),"trace carries frame time and command age at emission");
        check(line.find("player 5.000 133.400")!=std::string::npos && line.find("target ")!=std::string::npos &&
            line.find("world ")!=std::string::npos && line.find("stick ")!=std::string::npos &&
            line.find("keys ")!=std::string::npos && line.find("yaw 0.00000 native-yaw 0.00000")!=std::string::npos,
            "trace contains pose, steering target, world command, actual axes/keys and both camera yaws");
        previous=time;
    }
    const auto count=inputDiagnostics.size();
    integrationLeg.target.pushControl=0;pedestalTime+=100;updateAutoWalk();
    pad::State pad;autoWalkController(pad);
    check(inputDiagnostics.size()==count,"ordinary routes do not emit pedestal input traces");
    logCaptureHandler.store(nullptr);write<uintptr_t>(address(0x1f11430),0);
    cancelPedestal();frame(0);stopAutoWalk();delayRoute=false;
}
void precisionLifecycleChecks() {
    beginTrace({5,-10.008,133.4});
    pad::State input;autoWalkController(input);
    check(!input.lx && !input.ly,"push start stays neutral before the first native input event");
    for(unsigned i=0;i<20 && !input.lx;++i) {tick();input={};autoWalkController(input);}
    check(input.lx<0 && !input.ly,"precision emits the first west pulse after draining");
    fixtureForward={1,0,0};tick();input={};autoWalkController(input);
    check(!input.lx && !input.ly,"camera turn cancels a cached pulse in the same native update");
    for(unsigned i=0;i<9;++i) {
        tick();input={};autoWalkController(input);
        check(!input.lx && !input.ly,"camera turn drains native input before the replacement direction");
    }
    tick();input={};autoWalkController(input);
    check(!input.lx && input.ly<0,"replacement pulse uses the new camera basis");
    autoWalkPlayerInput(VK_RIGHT);tick();
    check(!autoWalkActive(),"manual movement cancels a precision pulse immediately");
    cancelPedestal();frame(0);stopAutoWalk();fixtureForward={0,0,1};
    beginTrace({5,-10.008,133.4});input={};autoWalkController(input);
    check(!input.lx && !input.ly,"rapid restart cannot reuse the previous pulse");
    cancelPedestal();frame(0);stopAutoWalk();delayRoute=false;
}
// Installed native 82D180: independent 0..256 directional ramps change by
// 32 per field frame; magnitude below 48 is idle. Device publication adds
// one keyboard frame or two controller frames before the ramp consumes input.
void rampApproach(double speed,unsigned frameMs,unsigned latency,bool keyboardDevice,double heading,bool warm) {
    fixtureForward={std::sin(heading),0,std::cos(heading)};
    beginTrace({5.332,-10.008,133.688});
    integrationLeg.start={9.112,-10.008,138.140};
    integrationLeg.goal={.222,-10.008,132.854};
    std::array<unsigned,3> pending{};std::array<double,4> ramps{};
    // Begin on the positive-X side with momentum continuing the live lateral
    // stride away from the waypoint, rather than an unrelated inward collision.
    constexpr unsigned octants[]={1,9,8,12,4,6,2,3};
    const auto east=octants[(static_cast<int>(std::round(std::atan2(fixtureForward.z,fixtureForward.x)*4/3.14159265358979323846))+8)&7];
    if(warm) {pending.fill(east);for(unsigned i=0;i<ramps.size();++i) if(east&(1u<<i)) ramps[i]=256;}
    bool touched=false;
    const auto began=pedestalTime;
    for(unsigned frameIndex=0;frameIndex<30000/frameMs && !touched;++frameIndex) {
        pedestalTime+=frameMs;updateAutoWalk();frame(0);
        check(autoWalkActive(),"native ramp approach stays active until contact");
        unsigned bits=0;
        if(keyboardDevice) {
            std::array<uint8_t,256> keys{};autoWalkKeyboard(keys.data());
            bits=(keys[0xc8]?1:0)|(keys[0xcb]?2:0)|(keys[0xd0]?4:0)|(keys[0xcd]?8:0);
        } else {
            pad::State state;autoWalkController(state);
            bits=(state.ly>0?1:0)|(state.lx<0?2:0)|(state.ly<0?4:0)|(state.lx>0?8:0);
        }
        if(!frameIndex) check(!bits,"near-waypoint steering releases inherited movement before correcting direction");
        pending[frameIndex%pending.size()]=bits;
        const unsigned consumed=frameIndex>=latency?pending[(frameIndex-latency)%pending.size()]:(warm?east:0);
        for(unsigned i=0;i<ramps.size();++i) ramps[i]=std::clamp(ramps[i]+(consumed&(1u<<i)?32:-32),0.0,256.0);
        const auto x=ramps[3]-ramps[1],z=ramps[0]-ramps[2],length=std::hypot(x,z);
        const auto magnitude=length<48?0:std::min(256.0,length)/256;
        auto p=playerPosition();
        if(length) {
            const auto step=magnitude*speed*frameMs/1000;
            p.x+=(z*fixtureForward.x+x*fixtureForward.z)/length*step;
            p.z+=(z*fixtureForward.z-x*fixtureForward.x)/length*step;
        }
        position(playerActor(),p);
        if(distanceXZ(p,{.222,-10.005,144.104})<=7) {
            if(!capturePedestalNavigationState().contact || std::abs(p.x-.222)>1.5 || p.z>=144.104)
                std::fprintf(stderr,"unsafe ramp case speed=%.1f ms=%u latency=%u keyboard=%d yaw=%.3f frame=%u p=%.3f,%.3f bits=%x\n",
                    speed,frameMs,latency,keyboardDevice,heading,frameIndex,p.x,p.z,bits);
            check(capturePedestalNavigationState().contact,"ramped movement stages before native contact");
            check(std::abs(p.x-.222)<=1.5 && p.z<144.104,"ramped movement contacts only the centred south side");
            touched=true;
        }
    }
    if(!touched) std::fprintf(stderr,"ramp case speed=%.1f ms=%u latency=%u keyboard=%d yaw=%.3f\n",speed,frameMs,latency,keyboardDevice,heading);
    check(touched && pedestalTime-began<30000,"native directional ramps converge within the whole approach deadline");
    check(pushes()==1 && spoken.size()==1,"ramped approach retains exactly one push announcement");
    cancelPedestal();frame(0);stopAutoWalk();delayRoute=false;fixtureForward={0,0,1};
}
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
    check(released,"trace releases the sideways stride between worker observations");
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
    for(unsigned i=0;i<1200 && !capturePedestalNavigationState().contact;++i) {
        const auto bits=tick();const auto p=playerPosition();
        north|=bits==1;
        const double x=(bits&8?1.0:0)-(bits&2?1.0:0),z=(bits&1?1.0:0)-(bits&4?1.0:0);
        const double scale=x && z?.7071067811865476:1;
        position(playerActor(),{p.x+x*.17*scale,p.y,p.z+z*.17*scale});
    }
    check(capturePedestalNavigationState().contact && pushes()==1 && spoken.size()==1,"trace reaches contact phase once without duplicate speech despite frozen worker observations");
    check(north,"released approach recovers along the north staging axis");
    pad::State input;autoWalkController(input);
    check(!input.lx && !input.ly,"staging handoff releases movement in the same native tick");
    const auto before=playerPosition();
    for(unsigned i=0;i<12 && !input.ly;++i) {tick();autoWalkController(input);}
    check(input.ly>0 && !input.lx,"owned north contact input follows bounded native momentum drain");
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
    for(const auto warm:{false,true})
    for(const auto heading:{0.0,.392699081698724,1.178097245096172,2.042035224833366,3.543018381548489,5.515240436302081})
    for(const auto speed:{10.0,50.0}) for(const auto frameMs:{17u,33u}) for(const auto keyboardDevice:{false,true})
        rampApproach(speed,frameMs,keyboardDevice?1:2,keyboardDevice,heading,warm);
    inputTraceChecks();precisionLifecycleChecks();
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
    std::puts("PASS: 96 cold/warm native-ramp approach cases, six camera headings, keyboard/pad latency, 10/50u/s, bounded input trace, live-trace recovery and single speech");
}
