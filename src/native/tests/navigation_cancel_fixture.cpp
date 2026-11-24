#include "fixture_runtime.hpp"
// Reuse only the synthetic native/input and semantic speech adapters. Both
// route retirement and Auto walk below link their production implementations.
#include "../navigation_scene.hpp"
#include "../navigation_observations.hpp"
#include "../butterfly_navigation.hpp"
#include "../floor_panels.hpp"
#define FFX_NAV_CUES_INTEGRATION_TEST
#define main autoWalkComponentFixtureEntry
#include "auto_walk_fixture.cpp"
#undef main
#include "test_speech_domains.hpp"

namespace ffx {
void prepareNavRetirement();
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool) {return output(text,interrupt,producer);}
bool cameraBasis(Point& f,Point& right) {f=forward;right={f.z,0,-f.x};return camera;}
std::vector<Entity> butterflyTargets() {return {};}
void refreshButterflyTarget(Entity&) {}
void refreshButterflyStarter(Entity&) {}
void installWallCues() {}
void captureWallCues() {}
void cancelWallCues() {}
void publishNavAudio(const NavAudioState&) {}
void pauseNavAudio() {}
void pauseNavRouteAudio() {}
void stopNavPreview() {}
void navAudioArrival() {}
void publishPositionalCue(float,float,bool,unsigned) {}
}
namespace ffx::speechq {void notifyPreparation() noexcept {}}
namespace ffx::nav {void requestNavigationScene() {}}

int main() {
    using namespace ffx;
    memory=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2100000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(memory!=0,"isolated native field configuration");prepareNavRetirement();
    static nav::Scene observation;Mesh mesh;mesh.openPlane=mesh.fixedHeight=true;
    Entity target;target.name=L"Cancel fixture goal";target.position={0,0,100};
    for(bool nativeClear:{false,true}) {
        nativeOwner=false;observation.epoch=epoch;observation.allowed=true;observation.cameraValid=true;
        observation.forward={0,0,1};observation.right={1,0,0};observation.audioContext=navAudioContextSerial();observation.collisionSerial=navCollisionSerial();
        speechq::Header header{};header.generation=epoch;
        updateOwnedNavAudio(observation,mesh,{target},header,L"");routeNavAudio({{},target.position},target);
        nativeOwner=true;begin();check(autoWalkActive(),"real route starts real Auto walk");speech.clear();
        nativeOwner=nativeClear;clearNavRoute();
        nativeOwner=true;updateAutoWalk();
        check(speech.empty() && !autoWalkActive(),"clearNavRoute then updateAutoWalk before explicit stop stays silent");
        stopAutoWalk();
    }
    // Retained W69 center and W66 destination with synthetic floor, pose and stride.
    // Exercise the real route, published leg, start cue, keyboard and input ramps.
    begin();stopAutoWalk();player={-45,5,35};mesh.planeY=5;
    auto& panels=mesh.panels;panels.required=panels.ready=true;panels.radius=2;panels.chunk=1;
    panels.epoch=epoch;panels.serial=1;panels.active=(1u<<9)|(1u<<12);panels.revision=floorPanelRevision.load();
    panels.rows[9]={{-5,5,75},1,0,2,2,9};panels.rows[12]={{-25,5,55},1,0,2,2,12};
    Entity panel;panel.worker=9;panel.panelLocal=66;panel.name=L"Floor panel 11";panel.position=panels.rows[9].center;
    speechq::Header panelHeader{};panelHeader.generation=epoch;
    auto observePanels=[&] {
        // Live Dome captures land almost every frame; each one counts a sample.
        ++floorPanelSample;nativeOwner=false;observation.player=player;panels.capturedAt=GetTickCount64();
        updateOwnedNavAudio(observation,mesh,{},panelHeader,L"");
    };
    observePanels();const auto detour=navigationRouteOwned(mesh,player,panel,{});
    check(detour.size()>2,"shared planner detours around the intervening floor panel");
    routeNavAudio(detour,panel,true);nativeOwner=true;speech.clear();
    static bool refusalLogged=false;
    logCaptureHandler.store([](const LogCapture& p) noexcept {
        if(!std::strcmp(p.format.data(),"floor panel walk refused: start announcement unavailable")) refusalLogged=true;
    });
    refuseTextPlan=true;startAutoWalk();refuseTextPlan=false;
    logCaptureHandler.store(nullptr);
    check(refusalLogged && !autoWalkActive() && speech.size()==1 && speech.front()==noSafePanelRoute && !lastInterrupt,
        "rejected start announcement is logged and gives a non-interrupting refusal");
    speech.clear();startAutoWalk();
    check(autoWalkActive() && !speech.empty() && speech.front()==L"Auto walking to Floor panel 11","named panel cue at walk start");
    const auto walkPanels=[&](auto sample) {
        std::array<double,4> ramp{};unsigned queued=0;
        for(unsigned frame=0;frame<12000 && autoWalkActive();++frame) {
            observePanels();nativeOwner=true;clockValue+=16;updateAutoWalk();
            auto keys=keyboard();const unsigned bits=(keys[0xc8]?1:0)|(keys[0xcb]?2:0)|(keys[0xd0]?4:0)|(keys[0xcd]?8:0);
            for(unsigned i=0;i<4;++i) ramp[i]=std::clamp(ramp[i]+(queued&(1u<<i)?32:-32),0.0,256.0);
            queued=bits;
            const double x=ramp[3]-ramp[1],z=ramp[0]-ramp[2],length=std::hypot(x,z);
            if(length>=48) {const double step=std::min(1.0,length/256);player.x+=x/length*step;player.z+=z/length*step;}
            sample();
        }
    };
    bool panelTouched=false;
    walkPanels([&] {
        check(std::abs(player.x+25)>4 || std::abs(player.z-55)>4,"steered motion preserves W69 player-radius clearance");
        if(!panelTouched && std::abs(player.x+5)<2 && std::abs(player.z-75)<2) {
            panelTouched=true;panels.active&=~(1u<<9);++panels.pattern[4];
            panels.revision=++floorPanelRevision;
        }
    });
    check(panelTouched && !autoWalkActive() && speech.size()==2 && speech.back()==L"Floor panel 11: route destination reached",
        "W66 touch produces normal arrival without route loss");
    panels.active|=1u<<9;
    player={-5,5,65};observePanels();routeNavAudio({player,panel.position},panel,true);
    nativeOwner=true;speech.clear();startAutoWalk();
    player.z=73;observePanels();nativeOwner=true;updateAutoWalk();
    check(autoWalkActive() && speech.size()==1,"touching only the rectangle boundary keeps the walking cue");
    player.z=73.25;observePanels();nativeOwner=true;updateAutoWalk();
    check(!autoWalkActive() && speech.size()==2 && speech.back()==L"Floor panel 11: route destination reached",
        "strict entry before the target bit clears also arrives");
    for(bool anotherPanelChanges:{true,false}) {
        player={-5,5,65};observePanels();routeNavAudio({player,panel.position},panel,true);
        nativeOwner=true;speech.clear();startAutoWalk();
        check(autoWalkActive() && speech.size()==1,"panel clear fixture starts the real walker");
        if(anotherPanelChanges) player.z=72; // Body overlaps W66, but its center is outside.
        panels.active&=~(1u<<9);
        if(anotherPanelChanges) panels.active&=~(1u<<12);
        panels.revision=++floorPanelRevision;
        observePanels();nativeOwner=true;updateAutoWalk();
        check(!autoWalkActive() && speech.size()==2 &&
            speech.back()==L"Route lost: floor panel state changed or unavailable. Walk manually.",
            anotherPanelChanges?"clearing W66 and W69 together loses the route without arrival":
                "clearing W66 with the player ten units away loses the route without arrival");
        panels.active|=(1u<<9)|(1u<<12);
    }
    // Parallel to W69, only .125 outside the player-radius margin.
    player={-29.125,5,35};Entity parallel;parallel.name=L"Parallel";parallel.position={player.x,5,75};
    observePanels();routeNavAudio({player,parallel.position},parallel,true);nativeOwner=true;
    speech.clear();startAutoWalk();check(autoWalkActive(),"tight corridor reaches production pulse control");
    bool injected=false;
    for(unsigned frame=0;frame<20 && autoWalkActive();++frame) {
        clockValue+=16;updateAutoWalk();auto keys=keyboard();
        injected|=std::any_of(keys.begin(),keys.end(),[](uint8_t k){return k!=0;});
    }
    check(!injected && !autoWalkActive() && !speech.empty() && speech.back().find(L"panel too close")!=std::wstring::npos,
        "cold pulse stops with speech before injecting any key");
    for(bool stale:{false,true}) {
        player={-45,5,35};observePanels();routeNavAudio(detour,panel,true);
        nativeOwner=true;speech.clear();startAutoWalk();
        bool moving=false;
        for(unsigned i=0;i<30 && !moving;++i) {
            clockValue+=16;updateAutoWalk();const auto keys=keyboard();
            moving=keys[0xc8] || keys[0xcd];
        }
        check(moving,"guarded walk has live synthetic input before invalidation");
        floorPanelScriptActive=true;
        const auto heldKeys=keyboard();pad::State heldPad{};autoWalkController(heldPad);
        check(!heldKeys[0xc8] && !heldKeys[0xcd] && !heldPad.lx && !heldPad.ly,
            "keyboard and controller withhold the live mask inside a panel script");
        floorPanelScriptActive=false;
        if(stale) {
            panels.capturedAt=GetTickCount64()-251;nativeOwner=false;
            updateOwnedNavAudio(observation,mesh,{},panelHeader,L"");nativeOwner=true;
        } else floorPanelRevision.fetch_add(1); // Native mutation before a worker gets to replan.
        const auto keys=keyboard();pad::State pad{};autoWalkController(pad);
        check(!keys[0xc8] && !keys[0xcd] && !pad.lx && !pad.ly,"pending input cannot outlive panel facts");
        if(!stale) {
            panels.revision=floorPanelRevision.load();
            panels.active&=~(1u<<9);panels.active|=1;panels.rows[0]={{-40,5,40},1,0,2,2,0};
            observePanels();nativeOwner=true;
            panels.active&=~1u;panels.active|=1u<<9;
        }
        updateAutoWalk();check(!autoWalkActive() && speech.back()==L"Route lost: floor panel state changed or unavailable. Walk manually.",
            "panel changes and stale facts stop the real walker with speech");
        panels.revision=floorPanelRevision.load();
    }
    std::puts("PASS: guarded W69 detour reaches W66 on the input ramp model; activation and stale facts retire keyboard/controller movement.");
    // W75:0 and W69:0 pass these centers to 0057 and (2,2) to 0058.
    // Exercise a nonzero stored angle at geometry+34 on the retained squares.
    const std::array<Point,2> centers={Point{-95,5,45},Point{-25,5,55}};
    constexpr float storedAngle=.4f;
    panels.radius=5;panels.active=(1u<<18)|(1u<<12);
    for(unsigned i=0;i<centers.size();++i) {
        std::array<uint8_t,0x4c> raw{};
        const auto put=[&](unsigned offset,float v){std::memcpy(raw.data()+offset,&v,sizeof(v));};
        const int16_t ref=-1;std::memcpy(raw.data()+0x28,&ref,sizeof(ref));
        put(0,float(centers[i].x));put(4,5);put(8,float(centers[i].z));
        put(0x30,2);put(0x34,storedAngle);put(0x38,2);
        const auto shape=playerEntryRegion(raw,0x20);
        check(shape.valid && shape.angle==-storedAngle,"stored type-6 angle is negated");
        const int row=i?12:18;
        panels.rows[row]={shape.center,std::cos(shape.angle),std::sin(shape.angle),shape.width,shape.depth,row};
    }
    player={-130,5,40};Entity across;across.name=L"Across both panels";across.position={10,5,60};
    observePanels();routeNavAudio(navigationRouteOwned(mesh,player,across,{}),across,true);
    nativeOwner=true;speech.clear();startAutoWalk();check(autoWalkActive(),"two-panel detour starts");
    const auto clearance=[&](Point p,double allowance) {
        for(auto c:centers) {
            const double x=p.x-c.x,z=p.z-c.z;
            const double u=x*std::cos(storedAngle)+z*std::sin(storedAngle);
            const double v=z*std::cos(storedAngle)-x*std::sin(storedAngle);
            check(std::hypot(std::max(0.,std::abs(u)-2),std::max(0.,std::abs(v)-2))>panels.radius+allowance,
                "published leg and player clear both rotated forbidden panels");
        }
    };
    walkPanels([&] {
        NavLeg current;check(navLeg(current),"two-panel walk publishes a leg");
        const auto length=distanceXZ(current.start,current.goal);
        const int steps=std::max(1,int(std::ceil(length)));
        // Distance to a closed rectangle changes by at most distance walked.
        // Half the spacing covers every point between consecutive samples.
        for(int j=0;j<=steps;++j) {
            const double t=double(j)/steps;
            clearance({current.start.x+t*(current.goal.x-current.start.x),5,current.start.z+t*(current.goal.z-current.start.z)},length/(2*steps));
        }
        clearance(player,0);
    });
    if(distanceXZ(player,across.position)>1.5)
        std::fprintf(stderr,"Two-panel walk ended at %.3f, %.3f: %ls\n",player.x,player.z,speech.back().c_str());
    check(!autoWalkActive() && distanceXZ(player,across.position)<=1.5 && speech.back()==L"Across both panels: route destination reached",
        "two-panel detour reaches far side");
    std::puts("PASS: every published leg and sampled position clears rotated W75 and W69 through arrival.");
    mesh.panels={};mesh.planeY=0;clearNavRoute();
    // Real route geometry, waypoint advancement and input steering, with only
    // movement/collision supplied by this synthetic host. No native push replay.
    for(unsigned scenario:{0u,1u,2u,3u})
    for(unsigned observationFrames:{1u,6u})
    for(double heading:{0.0,22.5,67.5,117.0,203.0,316.0}) for(double stride:{.25,.7,1.3,1.65}) {
        const bool liveReset=scenario!=0;
        const Point pedestal=liveReset?Point{.222,-10.005,144.104}:Point{};
        begin();player=liveReset?Point{47.568,-9.999,184.646}:Point{44,0,44};
        // Both sides of the live loop around the (0.222, 133.398) ring waypoint.
        if(scenario==2) player={5.630,-10.008,132.756};
        if(scenario==3) player={-5.745,-10.008,133.989};
        const double angle=heading*3.14159265358979323846/180;
        forward={std::sin(angle),0,std::cos(angle)};
        target={};target.name=L"Pedestal";target.pushControl=2;target.position=pedestal;
        target.zone.valid=true;target.zone.shape=1;target.zone.height=100;
        target.zone.center=pedestal;
        target.zone.contactRadius=7;target.zone.obstacleRadius=liveReset?0:9.5;target.zone.approach={0,0,-1};
        speechq::Header header{};header.generation=epoch;
        const auto publish=[&] {
            nativeOwner=false;observation.player=player;observation.forward=forward;
            observation.right={forward.z,0,-forward.x};
            updateOwnedNavAudio(observation,mesh,{target},header,L"");nativeOwner=true;
        };
        const auto route=[&] {
            publish();nativeOwner=false;
            const auto points=interactionRoute(mesh,player,target.zone);
            check(!points.empty(),"push-side fixture has a connected route");
            routeNavAudio(points,target);nativeOwner=true;startAutoWalk(false);
        };
        route();bool inward=false,touched=false;unsigned retries=0;const auto began=clockValue;
        for(unsigned frame=0;frame<1800 && !touched;++frame) {
            if(frame%observationFrames==0) publish();
            clockValue+=33;updateAutoWalk();NavLeg current;
            check(navLeg(current),"push approach keeps its real route leg");
            // pedestal_input::approach checks the live endpoint on Native;
            // it does not wait for the worker's arrival publication.
            if(inInteractionZone(target.zone,player) && !inward) {
                stopAutoWalk();target.zone.contactRadius=target.zone.obstacleRadius=0;
                target.zone.range=6.85;inward=true;route();
                check(navLeg(current),"contact handoff observes its newly published leg");
            }
            // The pedestal session now owns bounded silent recovery when a
            // worker misses an intermediate hold. This host supplies that
            // scheduling boundary; pedestal_walk_fixture links the real owner.
            if(!autoWalkActive() && speech.empty() && retries++<8) {route();check(navLeg(current),"replanned push leg stays available");}
            if(!autoWalkActive()) {
                std::fprintf(stderr,"approach stopped: live=%d observation-frames=%u heading=%.1f stride=%.2f frame=%u player=%.3f,%.3f waypoint=%.3f,%.3f inward=%d arrived=%d\n",
                    liveReset,observationFrames,heading,stride,frame,player.x,player.z,current.goal.x,current.goal.z,inward,current.arrived);
                if(!speech.empty()) std::fwprintf(stderr,L"last speech: %ls\n",speech.back().c_str());
            }
            check(autoWalkActive(),"northeast reset approach reaches contact without a no-progress stop");
            const auto keys=keyboard();
            const double front=!!keys[0xc8]-!!keys[0xd0],right=!!keys[0xcd]-!!keys[0xcb];
            const double length=std::hypot(front,right);
            if(!length) continue;
            const Point next{player.x+stride*(front*forward.x+right*forward.z)/length,player.y,
                player.z+stride*(front*forward.z-right*forward.x)/length};
            Point steering;const bool nativeFinal=autoWalkSteeringGoal(steering) && distanceXZ(steering,current.final)<.01;
            if(distanceXZ(current.goal,current.final)>.01 && !nativeFinal) {
                if(distanceXZ(next,pedestal)<=(liveReset?7:9.5)) std::fprintf(stderr,
                    "clearance: scenario=%u observations=%u heading=%.1f stride=%.2f frame=%u player=%.3f,%.3f next=%.3f,%.3f goal=%.3f,%.3f final=%.3f,%.3f steer=%.3f,%.3f inward=%d\n",
                    scenario,observationFrames,heading,stride,frame,player.x,player.z,next.x,next.z,current.goal.x,current.goal.z,current.final.x,current.final.z,steering.x,steering.z,inward);
                check(distanceXZ(next,pedestal)>(liveReset?7:9.5),"detour movement retains the wider pedestal clearance before the final leg");
            }
            if(distanceXZ(next,pedestal)<=7) {
                check(inward,"outside staging hands off before native contact");
                check(next.z-pedestal.z< -2 && std::abs(next.x-pedestal.x)<1.5,"only centred south-side movement can touch the pedestal");
                touched=true;
            }
            player=next;
        }
        check(touched,"northeast reset detour and final inward leg reach native contact");
        check(clockValue-began<30000,"push staging and contact converge within the whole-approach budget");
        stopAutoWalk();nativeOwner=false;clearNavRoute();nativeOwner=true;
    }
    std::puts("PASS: production pedestal detours and live step-15 loop coordinates reach aligned contact across six headings, four strides and 33/198 ms observations.");
    // A lower-recess observation can enter the zone before native movement
    // settles. Exercise the real walker/cues handshake, including failed entry.
    unsigned arrivalCases=0;uint64_t longestArrival=0,longestSuccessfulArrival=0;
    static std::vector<std::string> arrivalDiagnostics;
    logCaptureHandler.store([](const LogCapture& packet) noexcept {
        if(std::strstr(packet.format.data(),"auto walk arrival ") && packet.formatter) {
            char text[1024]{};packet.formatter(packet,text,sizeof(text));arrivalDiagnostics.emplace_back(text);
        }
    });
    for(double speed:{10.0,50.0}) for(unsigned frameMs:{17u,33u})
    for(unsigned latency:{1u,2u}) for(bool warm:{false,true}) for(bool blocked:{false,true})
    for(unsigned endpoint:{0u,1u,2u,3u}) for(double outside:{.2,2.0,4.5}) {
        begin();stopAutoWalk();nativeOwner=false;clearNavRoute();nativeOwner=true;
        player={0,0,16.8};target={};target.worker=35;target.name=L"Lower Kilika Sphere recess";
        target.position={0,0,20};target.zone.valid=target.zone.talk=true;
        target.zone.shape=3;target.zone.center=target.position;
        target.zone.width=target.zone.depth=3;target.zone.height=5;
        // Route endpoints can sit on the zone edge. A remote live pedestal
        // still supplies a keep-out, but it cannot block safe local correction.
        if(endpoint==2) {target.zone.obstacle={100,0,100};target.zone.obstacleRadius=7;}
        observation.epoch=epoch;observation.allowed=true;observation.collisionSerial=navCollisionSerial();
        observation.audioContext=navAudioContextSerial();observation.cameraValid=true;observation.forward=forward;
        speechq::Header header{};header.generation=epoch;
        const auto publish=[&] {
            nativeOwner=false;observation.player=player;
            updateOwnedNavAudio(observation,mesh,{target},header,L"");nativeOwner=true;
        };
        publish();nativeOwner=false;routeNavAudio({player,{0,0,endpoint==3?22.5:endpoint?17.0:17.5}},target,true);nativeOwner=true;
        speech.clear();
        player={0,0,17.1};publish();NavLeg current;
        check(navLeg(current) && current.arrived && speech.empty(),"transient lower-recess entry has no premature success speech");
        player={0,0,17-outside};std::array<unsigned,3> pending{};std::array<double,4> ramps{};
        // Start native sampling here: the setup displacement is not one native
        // movement frame and must not become the calibrated maximum stride.
        publish();startAutoWalk(false);arrivalDiagnostics.clear();
        check(navLeg(current) && current.goal.z>=17 && current.goal.z<=17.25,
            "homing selects the nearest reachable inside point, not the old far-side destination");
        if(warm) {pending.fill(4);ramps[2]=256;}
        const auto began=clockValue;
        for(unsigned frame=0;frame<16000/frameMs && autoWalkActive();++frame) {
            clockValue+=frameMs;updateAutoWalk();const auto keys=keyboard();
            const unsigned bits=(keys[0xc8]?1u:0)|(keys[0xcb]?2u:0)|(keys[0xd0]?4u:0)|(keys[0xcd]?8u:0);
            const auto consumed=pending[0];
            for(unsigned i=0;i+1<latency;++i) pending[i]=pending[i+1];pending[latency-1]=bits;
            for(unsigned i=0;i<4;++i) ramps[i]=std::clamp(ramps[i]+(consumed&(1u<<i)?32:-32),0.0,256.0);
            const auto x=ramps[3]-ramps[1],z=ramps[0]-ramps[2],length=std::hypot(x,z);
            if(!blocked && length>=48) {
                const auto scale=std::min(1.0,length/256)*speed*frameMs/1000;
                player.x+=scale*x/length;player.z+=scale*z/length;
            }
            publish();
        }
        publish();
        check(!autoWalkActive() && speech.size()==1,"ordinary lower-recess arrival has one final outcome");
        if(blocked) {
            check(speech.back()==L"Auto walk stopped: not in interaction range",
                "failed lower-recess correction emits failure without a preceding arrival");
            check(arrivalDiagnostics.size()==3 && arrivalDiagnostics[0].find("destination ")!=std::string::npos &&
                arrivalDiagnostics[0].find("nearest-inside 0.000 17.000")!=std::string::npos &&
                arrivalDiagnostics[0].find("distance ")!=std::string::npos && arrivalDiagnostics[1].find("shape 3")!=std::string::npos &&
                arrivalDiagnostics[2].find("no progress deadline")!=std::string::npos && arrivalDiagnostics[2].find("homing-ms ")!=std::string::npos,
                "every failed arrival logs pose, destination, shape, nearest inside point, distance, duration and end reason");
        }
        else {
            if(!inInteractionZone(target.zone,player)) std::fprintf(stderr,"arrival case: endpoint=%u outside=%.1f speed=%.1f frame=%u latency=%u warm=%d position=%.3f,%.3f elapsed=%llu\n",
                endpoint,outside,speed,frameMs,latency,warm,player.x,player.z,static_cast<unsigned long long>(clockValue-began));
            check(inInteractionZone(target.zone,player) && speech.back()==L"Lower Kilika Sphere recess: route destination reached",
                "successful lower-recess correction settles inside the zone before its sole arrival");
            longestSuccessfulArrival=std::max(longestSuccessfulArrival,clockValue-began);
        }
        if(blocked) {target.inactive=true;publish();check(speech.size()==1,
            "worker invalidation after a native arrival failure cannot repeat the failure");}
        check(clockValue-began<=(blocked?3500u:15500u),"ordinary arrival correction has bounded no-progress and whole recovery deadlines");
        longestArrival=std::max(longestArrival,clockValue-began);++arrivalCases;
    }
    std::printf("PASS: %u ordinary arrival ramp cases, cold/warm, 17/33ms, one/two-frame pipeline and blocked recovery; longest simulated recovery %llums, longest success %llums.\n",arrivalCases,static_cast<unsigned long long>(longestArrival),static_cast<unsigned long long>(longestSuccessfulArrival));
    // Live Djose step 14: correction around the pedestal keep-out settled
    // 0.39 units from its corner waypoint and stopped on no progress.
    {
        begin();stopAutoWalk();nativeOwner=false;clearNavRoute();nativeOwner=true;
        target={};target.worker=36;target.name=L"Pedestal";target.position={0,0,24};
        target.zone.valid=target.zone.talk=true;target.zone.shape=1;target.zone.center=target.position;target.zone.range=10;target.zone.height=5;
        target.zone.approach={1,0,0};target.zone.obstacle=target.position;target.zone.obstacleRadius=6;
        observation.epoch=epoch;observation.allowed=true;observation.collisionSerial=navCollisionSerial();
        observation.audioContext=navAudioContextSerial();observation.cameraValid=true;observation.forward={0,0,1};
        speechq::Header header{};header.generation=epoch;
        const auto publish=[&] {nativeOwner=false;observation.player=player;updateOwnedNavAudio(observation,mesh,{target},header,L"");nativeOwner=true;};
        player={14,0,24};publish();nativeOwner=false;routeNavAudio({player,{8.5,0,24}},target,true);nativeOwner=true;
        speech.clear();player={8.6,0,24};publish();
        player={-2,0,32.5};publish();NavLeg current;
        check(navLeg(current) && current.arrived && distanceXZ(current.goal,current.arrivalInside)>.01,
            "pedestal correction from the north face routes through a corner");
        const auto corner=current.goal;
        // Settle 0.39 units short of the corner, as observed live.
        const double x=corner.x-player.x,z=corner.z-player.z,length=std::hypot(x,z);
        player={corner.x-.39*x/length,0,corner.z-.39*z/length};publish();
        check(navLeg(current) && distanceXZ(current.goal,corner)>.01,
            "a settled correction 0.39 units from a clear corner advances to the next correction point");
        std::printf("PASS: pedestal correction corner %.2f,%.2f advances at 0.39 units; next %.2f,%.2f.\n",corner.x,corner.z,current.goal.x,current.goal.z);
        stopAutoWalk();nativeOwner=false;clearNavRoute();nativeOwner=true;
    }
    logCaptureHandler.store(nullptr);
    // A remote keep-out permits correction; a pedestal across the correction
    // segment must still stop it. Transient entry was on the far side.
    begin();stopAutoWalk();nativeOwner=false;clearNavRoute();nativeOwner=true;
    player={0,0,20};target={};target.worker=35;target.name=L"Guarded recess";
    target.position=target.zone.center=player;target.zone.valid=target.zone.talk=true;
    target.zone.shape=3;target.zone.width=target.zone.depth=3;target.zone.height=5;
    target.zone.obstacle={0,0,18};target.zone.obstacleRadius=.5;
    observation.epoch=epoch;observation.allowed=true;observation.collisionSerial=navCollisionSerial();
    observation.audioContext=navAudioContextSerial();speechq::Header guardedHeader{};guardedHeader.generation=epoch;
    nativeOwner=false;observation.player=player;updateOwnedNavAudio(observation,mesh,{target},guardedHeader,L"");
    routeNavAudio({player,player},target,true);nativeOwner=true;startAutoWalk(false);speech.clear();
    player={0,0,16.8};
    for(unsigned frame=0;frame<100 && autoWalkActive();++frame) {
        clockValue+=33;updateAutoWalk();const auto keys=keyboard();
        check(!keys[0xc8] && !keys[0xcb] && !keys[0xd0] && !keys[0xcd],"arrival correction never walks across the active pedestal keep-out");
    }
    check(!autoWalkActive() && speech.size()==1 && speech.back()==L"Auto walk stopped: not in interaction range",
        "unsafe correction reports one failure without crossing the pedestal");
    // Once a zone was observed in range, a later owned floor snapshot can
    // prove that the displaced player has no connected correction route.
    begin();stopAutoWalk();nativeOwner=false;clearNavRoute();nativeOwner=true;
    target.zone.obstacleRadius=0;player={0,0,20};observation.player=player;
    nativeOwner=false;updateOwnedNavAudio(observation,mesh,{target},guardedHeader,L"");
    routeNavAudio({player,player},target,true);nativeOwner=true;
    player={0,0,12.5};observation.player=player;
    Mesh disconnected;nativeOwner=false;updateOwnedNavAudio(observation,disconnected,{target},guardedHeader,L"");nativeOwner=true;
    NavLeg unreachable;
    check(navLeg(unreachable) && unreachable.arrived && unreachable.arrivalUnreachable,
        "owned geometry marks a correction with no connected floor unreachable");
    speech.clear();startAutoWalk(false);const auto unreachableBegan=clockValue;
    while(autoWalkActive() && clockValue-unreachableBegan<1000) {
        clockValue+=33;updateAutoWalk();const auto keys=keyboard();
        check(!keys[0xc8] && !keys[0xcb] && !keys[0xd0] && !keys[0xcd],"unreachable correction never injects movement");
    }
    check(!autoWalkActive() && speech.size()==1 && speech.back()==L"Auto walk stopped: not in interaction range",
        "unreachable correction reports one bounded failure");
    // Replans within one walk cannot restart settlement indefinitely. Exact
    // coincidence also needs no turn when the native facing setter declines.
    for(bool moving:{false,true}) {
        begin();stopAutoWalk();nativeOwner=false;clearNavRoute();nativeOwner=true;
        player={0,0,20};target={};target.worker=35;target.name=L"Lower Kilika Sphere recess";
        target.position=target.zone.center=player;target.zone.valid=target.zone.talk=true;
        target.zone.shape=3;target.zone.width=target.zone.depth=3;target.zone.height=5;
        observation.epoch=epoch;observation.allowed=true;observation.collisionSerial=navCollisionSerial();
        observation.audioContext=navAudioContextSerial();speechq::Header header{};header.generation=epoch;
        const auto publish=[&] {nativeOwner=false;observation.player=player;updateOwnedNavAudio(observation,mesh,{target},header,L"");nativeOwner=true;};
        publish();nativeOwner=false;routeNavAudio({player,player},target,true);nativeOwner=true;
        startAutoWalk(false);speech.clear();faceResult=moving;
        for(unsigned frame=0;frame<40 && autoWalkActive();++frame) {
            if(moving) target.position.x=target.zone.center.x=frame%2?.2:-.2;
            publish();clockValue+=17;updateAutoWalk();
        }
        publish();faceResult=true;
        check(!autoWalkActive() && speech.size()==1 && speech.back()==L"Lower Kilika Sphere recess: route destination reached",
            "same-walk moving-target tokens and exact-centre facing refusal each finish with one settled arrival");
    }
    nativeOwner=false;clearNavRoute();nativeOwner=true;
    VirtualFree(reinterpret_cast<void*>(memory),0,MEM_RELEASE);
    std::puts("PASS: production route-clear/Auto walk interleaving stays silent for native and worker cancellation.");
}
