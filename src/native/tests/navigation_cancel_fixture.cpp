// Reuse only the synthetic native/input and semantic speech adapters. Both
// route retirement and Auto walk below link their production implementations.
#include "../navigation_scene.hpp"
#include "../navigation_observations.hpp"
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
    VirtualFree(reinterpret_cast<void*>(memory),0,MEM_RELEASE);
    std::puts("PASS: production route-clear/Auto walk interleaving stays silent for native and worker cancellation.");
}
