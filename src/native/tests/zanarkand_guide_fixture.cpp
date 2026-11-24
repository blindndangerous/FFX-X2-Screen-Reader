#include "fixture_runtime.hpp"
// Zanarkand guide regressions using owned scenes. No game assets or native calls.
#include "../puzzle_guide_owned.hpp"
#include "../utility_trials_data.hpp"
#include "../puzzle_guide_bindings.hpp"
#include "../navigation_intents.hpp"
#include "../settings.hpp"
#include "../log_capture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "../floor_panels.hpp"
#include "../puzzle_tiles.hpp"
#include "../puzzle_tiles_data.hpp"

namespace {
void check(bool ok,const char* reason) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
[[noreturn]] void unexpectedFixtureCall() {check(false,"wording-only guide entered navigation or native work");std::abort();}
ffx::AtelIdentity identity;
bool inspectTarget=false;
bool railControlNames=false;
bool railInZone=true;
std::wstring railConfirm=L"C";
unsigned railLogs=0;
char railLog[2048]{};
void captureRailLog(const ffx::LogCapture& packet) noexcept {++railLogs;packet.formatter(packet,railLog,sizeof(railLog));}
bool qualifiedFixture=false,narrowFloor=false;
bool routeFixture=false;
unsigned starts=0,routeAudio=0,pushAdviceQueries=0;
constexpr const wchar_t* manualWarning=L"Walk manually around other active panels. A wrong panel resets the pattern.";
std::vector<ffx::Point> plannedRoute;
struct TargetObserved {ffx::Entity target;};
}
// These dependencies must stay unused by a plain step read. Fail rather than
// simulate pathfinding, speech, native memory, or pedestal actions.
namespace ffx {
bool nativeGameThread() {return false;}
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& out) {out=identity;return chunk==identity.span.chunk;}

void configurePedestalTarget(Entity& target,const char* script,unsigned worker) {
    // Bevelle's step read may seek push advice. No positioned pedestal exists
    // in this fixture, so production target() declines that optional advice.
    if(!std::strcmp(script,"bvyt1100") && worker==13) {target.pushControl=5;return;}
    if(!std::strcmp(script,"dome0700")) {if(routeFixture && worker>=44 && worker<=47) target.pushControl=6;return;}
    unexpectedFixtureCall();
}
OwnedTextContext copiedControlContext(const CopiedControls& controls) {
    if(railControlNames) {check(!controls.swap,"rail input names Confirm independently of menu swap");OwnedTextContext result;result.glyphs[0x31]=railConfirm;result.glyphs[0x32]=L"X";return result;}
    unexpectedFixtureCall();
}
std::wstring pedestalStepText(const PedestalPlan&,bool,bool) {unexpectedFixtureCall();}
std::wstring describePedestalApproach(const PedestalPlan&,Point,Point) {unexpectedFixtureCall();}
std::wstring describePedestalPlan(const PedestalPlan&,Point,const std::wstring&) {unexpectedFixtureCall();}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {std::wstring text;for(const auto& part:parts) {if(!text.empty()) text+=separator;text+=part;}return text;}
std::wstring direction(Point) {if(routeFixture) return L"north";unexpectedFixtureCall();}
void resetNavAudio() {if(!inspectTarget && !routeFixture) unexpectedFixtureCall();}
void routeNavAudio(const std::vector<Point>& points,const Entity&,bool) {if(routeFixture) {if(qualifiedFixture) plannedRoute=points;++routeAudio;return;}unexpectedFixtureCall();}
int setting(Setting setting) {if(routeFixture) return setting==Setting::AutoWalk;unexpectedFixtureCall();}
bool inInteractionZone(const InteractionZone&,Point) {if(routeFixture) return false;if(railControlNames) return railInZone;unexpectedFixtureCall();}
std::vector<Point> navigationRouteOwned(const Mesh& mesh,Point start,const Entity& target,const std::vector<Entity>&) {
    if(qualifiedFixture) return panelSafeRoute(mesh,target,routePoints(mesh,start,target.position));
    if(routeFixture) {auto route=plannedRoute;route.insert(route.begin(),start);route.push_back(target.position);return route;}
    if(inspectTarget) throw TargetObserved{target};unexpectedFixtureCall();
}
namespace nav {
bool scenePedestalPlan(const Scene&,const Entity&,PedestalPlan&) {if(routeFixture) ++pushAdviceQueries;return false;}
InteractionZone sceneInteractionZone(const WorkerFact&,unsigned,float) {if(railControlNames) return {};unexpectedFixtureCall();}
bool submitNavigationIntent(const Scene&,const Entity&,Intent,unsigned,unsigned,bool) {if(routeFixture) {++starts;return true;}unexpectedFixtureCall();}
}
}

int main(int argc,char** argv) {
    ffx_fixture::initializeRuntime();using namespace ffx;
    const int test=argc>1?std::atoi(argv[1]):0;
    static nav::Scene observation;
    auto setup=[&](int stage) {
        qualifiedFixture=routeFixture=false;starts=routeAudio=pushAdviceQueries=0;plannedRoute.clear();
        static const nav::Scene empty{};observation=empty;observation.epoch=1;observation.primary=0x10000;observation.partCount=1;observation.control=true;
        for(const auto& trial:trialUtilities) if(!std::strcmp(trial.script,"dome0700")) {
            identity={};identity.span={observation.primary,observation.epoch,1,0x100};
            identity.length=trial.length;identity.checksum=trial.checksum;std::memcpy(identity.name,trial.script,8);
        }
        auto& part=observation.parts[0];part.span=identity.span;part.guideReady=true;
        for(const auto b:nav::guide_dome0700) part.bindings.rows[part.bindings.count++]={b.low,0,b.high,b.element};
        for(unsigned local=57;local<=83;++local) {
            auto& row=observation.workers[observation.count++];row.valid=true;row.part=0;row.local=local;row.type=6;
            row.interactions.channels=0x20;row.interactions.entries[5]=2;
        }
        resetOwnedPuzzleGuide();railLogs=0;railLog[0]=0;logCaptureHandler.store(captureRailLog);
        for(auto& row:part.bindings.rows) {
            if(row.binding==0x20000a00) row.value=2700;
            if(row.binding==0x5c000004) row.value=stage;
            if(row.binding==0x0c000022) row.value=stage?6:0;
            if(row.binding==0x0c000020) row.value=stage?stage-1:0;
        }
    };
    auto set=[&](unsigned binding,int value) {
        for(auto& row:observation.parts[0].bindings.rows) if(row.binding==binding) {row.value=value;return;}
        check(false,"required binding captured");
    };
    auto query=[&](OwnedGuideAction action=OwnedGuideAction::Expected,unsigned local=0,const std::vector<Entity>& entities={}) {
        Mesh mesh;mesh.openPlane=routeFixture;mesh.planeY=observation.player.y;
        if(narrowFloor) {
            // A strip of real collision triangles. Both endpoints are on the
            // floor and outside W69, but there is no floor beside the panel.
            mesh.openPlane=false;
            const Point corners[]={{-47,5,35},{-45,5,33},{-3,5,75},{-5,5,77}};
            for(const auto ids:{std::array<int,3>{0,1,2},std::array<int,3>{0,2,3}}) {
                Triangle t;t.valid=true;
                for(unsigned j=0;j<3;++j) {
                    t.vertices[j]=ids[j];t.points[j]=corners[ids[j]];t.neighbours[j]=-1;
                    t.center.x+=t.points[j].x/3;t.center.z+=t.points[j].z/3;
                }
                t.center.y=5;mesh.triangles.push_back(t);
            }
            mesh.triangles[0].neighbours[2]=1;mesh.triangles[1].neighbours[0]=0;
            check(routePoints(mesh,observation.player,{-5,5,75}).size()>=2,"ordinary collision route crosses the narrow strip");
        }
        if(qualifiedFixture) mesh.panels=readFloorPanels(observation);
        return puzzleGuideOwned(action,observation,mesh,entities,speechq::Header{},0,0,local,observation.primary,1);
    };
    auto position=[&](unsigned worker) {for(const auto& t:puzzleTiles) if(t.worker==worker) {observation.player=t.point;return;}check(false,"tile exists");};
    auto panel=[&](unsigned worker,bool active) {observation.workers[worker-57].interactions.channels=active?0x20:0;};
    const auto qualifyFloor=[&] {
        qualifiedFixture=routeFixture=true;observation.geometryValid=true;observation.parts[0].panelMap=true;
        observation.panelRevision=floorPanelRevision.load();observation.capturedAt=GetTickCount64();
        observation.playerIndex=0;observation.actorCount=1;
        const float radius=2,scale=1;
        auto& bytes=observation.actors[0].bytes;
        std::memcpy(bytes.data()+0x5c,&radius,4);std::memcpy(bytes.data()+0x64,&radius,4);
        std::memcpy(bytes.data()+0x4e4,&scale,4);
        for(unsigned i=0;i<observation.count;++i) {
            auto& w=observation.workers[i];w.global=i;w.interactions.entryReady=true;
            w.interactions.channels=0;
        }
    };
    if(!test || test==1) {
        setup(2);set(0x0c000025,1);set(0x0c000024,1);panel(70,false);position(74);
        check(query().find(L"Next permitted floor panel: Floor panel 19:")!=std::wstring::npos,"pattern 2 permits a second panel in group 70/74/79");
        std::puts("PASS: Zanarkand pattern 2 second duplicate-group panel.");
    }
    if(!test || test==2) {
        setup(4);set(0x0c000027,1);set(0x0c000024,1);panel(68,false);position(72);
        check(query().find(L"Next permitted floor panel: Floor panel 17:")!=std::wstring::npos,"pattern 4 permits a second panel in group 68/72/73");
        std::puts("PASS: Zanarkand pattern 4 second duplicate-group panel.");
    }
    if(!test || test==3) {
        setup(0);query(OwnedGuideAction::Next);set(0x0c000022,6);
        const auto text=query(OwnedGuideAction::Step);
        check(text.find(L"Next permitted floor panel")==std::wstring::npos,"completed pattern 0 never offers its unused alternative");
        check(text.find(L"complete")!=std::wstring::npos,"completed reviewed step explains completion");
        std::puts("PASS: Zanarkand completed pattern 0 stale-step suppression.");
    }
    if(!test || test==25) {
        // ffx-dome: the activation tile has no done flag. Expected names step 1
        // before any first-pattern panel, then step 2 once one is touched.
        setup(0);auto text=query(OwnedGuideAction::Expected);
        check(text.find(L"Step 1 of 11")!=std::wstring::npos && text.find(L"activation tile")!=std::wstring::npos,"Expected names the activation tile at the first-visit start");
        set(0x0c000022,2);text=query(OwnedGuideAction::Expected);
        check(text.find(L"Step 2 of 11")!=std::wstring::npos,"a touched first-pattern panel makes step 2 expected");
        // F10 from step 2 after the pattern completes lands on step 3, not 4.
        setup(0);query(OwnedGuideAction::Next);set(0x0c000022,6);set(0x5c000004,1);text=query(OwnedGuideAction::Next);
        check(text.find(L"Step 3 of 11")!=std::wstring::npos,"F10 after the first pattern completes reads step 3");
        text=query(OwnedGuideAction::Next);check(text.find(L"Step 4 of 11")!=std::wstring::npos,"the next F10 moves on to step 4");
        // Live: a completion observation catches the guide up first, without a
        // word. F10 then still reads step 3.
        setup(0);query(OwnedGuideAction::Next);set(0x0c000022,6);set(0x5c000004,1);query(OwnedGuideAction::Completed);
        text=query(OwnedGuideAction::Next);
        check(text.find(L"Step 3 of 11")!=std::wstring::npos,"F10 after a silent completion catch-up reads step 3");
        std::puts("PASS: Zanarkand Expected start step and F10 catch-up.");
    }
    if(!test || test==4) {
        setup(2);position(66);query();set(0x0c000024,1);panel(66,false);query(OwnedGuideAction::TileCompleted,66);
        set(0x0c000024,0);panel(66,true);query(OwnedGuideAction::TileCompleted,66);
        check(query().find(L"Next permitted floor panel: Floor panel 11:")!=std::wstring::npos,"reset completion cannot mark resetting panel visited");
        check(railLogs && std::strstr(railLog,"Zanarkand") && std::strstr(railLog,"reset-observed=1"),"owned reset diagnostic exists");
        std::puts("PASS: Zanarkand reset receipt and diagnostic.");
    }
    if(!test || test==5) {
        setup(2);query();observation.control=false;
        const auto text=query();check(text.find(L"Next permitted floor panel")==std::wstring::npos,"no floor advice during native transition");
        check(text.find(L"Wait for")!=std::wstring::npos,"busy floor transition gives wait instruction");
        std::puts("PASS: Zanarkand transient observation waits.");
    }
    if(!test || test==6) {
        setup(0);set(0x0c000022,3);query(OwnedGuideAction::Next);const auto text=query(OwnedGuideAction::Next);
        check(text.find(L"Activate the first pedestal")==std::wstring::npos,"inactive first pedestal requires finishing initial pattern");
        check(text.find(L"Next permitted floor panel")!=std::wstring::npos,"first pedestal prerequisite supplies floor action");
        std::puts("PASS: Zanarkand first pedestal prerequisite.");
    }
    if(!test || test==7) {
        setup(2);query();set(0x0c000020,2);set(0x0c000024,7);
        const auto text=query(OwnedGuideAction::Step);
        check(text.find(L"Next permitted floor panel")==std::wstring::npos,"completed pattern 2 never offers alternatives");
        std::puts("PASS: Zanarkand completed main-pattern stale-step suppression.");
    }
    if(!test || test==8) {
        setup(2);check(query().find(manualWarning)!=std::wstring::npos,"floor advice warns about manual crossings");
        set(0x0c000020,2);check(query().find(manualWarning)!=std::wstring::npos,"next pedestal advice warns about floor crossings");
        std::puts("PASS: Zanarkand floor and pedestal auto-walk warning.");
    }
    if(!test || test==9) {
        setup(2);position(66);query();set(0x0c000024,1);panel(66,false);query(OwnedGuideAction::TileCompleted,66);
        // Reset followed by another success between snapshots: same count, rearmed tile.
        panel(66,true);panel(76,false);check(query().find(L"Next permitted floor panel: Floor panel 11:")!=std::wstring::npos,"live rearmed tile overrides old completion receipt at equal count");
        std::puts("PASS: Zanarkand missed reset with equal progress.");
    }
    if(!test || test==10) {
        unsigned steps=0;
        // Every legal pair from 70/74/79 and every single from 68/72/73.
        for(const unsigned omit:{70u,74u,79u}) for(const unsigned single:{68u,72u,73u}) {
            setup(2);unsigned count=0;
            std::vector<unsigned> order{66,single,76};
            for(const unsigned local:{70u,74u,79u}) if(local!=omit) order.push_back(local);
            order.push_back(78);order.push_back(83);
            for(const auto local:order) {
                position(local);const auto text=query();
                const auto expected=L"Next permitted floor panel: Floor panel "+std::to_wstring(local-55)+L":";
                check(text.find(expected)!=std::wstring::npos,"every legal seven-panel combination remains selectable");
                if(local==single) set(0x0c000027,1);
                if(local==70 || local==74 || local==79) set(count==3?0x0c000025:0x0c000026,1);
                panel(local,false);set(0x0c000024,static_cast<int>(++count));query(OwnedGuideAction::TileCompleted,local);++steps;
            }
            set(0x0c000020,2);const auto text=query();
            check(text.find(L"Pattern pedestal 3")!=std::wstring::npos && text.find(L"Next permitted floor panel")==std::wstring::npos,
                "seven counted panels advance to the third pedestal");
        }
        std::printf("PASS: Zanarkand all 9 legal pattern-2 combinations, %u panel selections and next-pedestal transitions.\n",steps);
    }
    if(!test || test==11) {
        setup(2);query();observation.count=0;
        check(query().find(L"No active permitted floor panel")!=std::wstring::npos,"missing workers never become guessed floor targets");
        setup(2);query();for(unsigned local=57;local<=83;++local) panel(local,false);
        check(query().find(L"No active permitted floor panel")!=std::wstring::npos,"all inactive panels stay inactive");
        const unsigned before=railLogs;
        puzzleGuideOwned(OwnedGuideAction::TileCompleted,observation,Mesh{},std::vector<Entity>{},speechq::Header{},0,0,66,observation.primary,2);
        check(railLogs==before,"stale allocation receipt is ignored");
        std::puts("PASS: Zanarkand missing/inactive worker and stale receipt guards.");
    }
    if(!test || test==12) {
        setup(2);observation.player={-45,5,35};
        for(unsigned worker=57;worker<=83;++worker) panel(worker,worker==66 || worker==69);
        const auto expected=query();
        check(expected.find(L"Next permitted floor panel: Floor panel 11:")!=std::wstring::npos,"W66 is legal and intervening W69 would reset pattern 2");
        routeFixture=true;plannedRoute={{-25,5,55}}; // The planner carries the route across active W69.
        const auto text=query(OwnedGuideAction::Path);
        check(starts==0,"route to legal W66 behind wrong active W69 cannot submit walking intent");
        check(routeAudio==0,"route through an active reset panel cannot publish route audio");
        const auto directions=text.find(L"north");
        check(text.find(manualWarning)==0 && directions!=std::wstring::npos && std::wcslen(manualWarning)<directions,
            "panel Path begins with the warning and reads directions even when Auto walk path speech is disabled");
        check(text.find(L"resets the pattern",std::wcslen(manualWarning))==std::wstring::npos &&
            text.find(L"Read path to current step")==std::wstring::npos && text.find(L"Step onto the named panel.")!=std::wstring::npos,
            "panel Path gives its action without repeating the warning or sending the player back to Path");
        std::puts("PASS: Zanarkand Path describes the panel route with a warning and no automated traversal.");
    }
    if(!test || test==13) {
        setup(2);position(66);
        auto& closest=observation.workers[66-57];closest.interactions.entries[5]=1;
        auto selected=[&] {return query().find(L"Next permitted floor panel: Floor panel 11:")!=std::wstring::npos;};
        check(!selected(),"a type 6 floor worker with the wrong channel 5 entry is skipped");
        closest.interactions.entries[5]=2;closest.type=2;
        check(!selected(),"a type 2 worker cannot present a floor panel callback");
        closest.type=6;closest.interactions.channels=0x10;closest.interactions.entries[4]=2;
        check(!selected(),"a floor worker without active channel 5 is skipped");
        closest.interactions.channels=0x20;
        check(selected(),"type 6 channel 5 entry 2 makes the nearest permitted panel selectable");
        std::puts("PASS: Zanarkand floor panel qualification requires native type, channel and entry.");
    }
    if(!test || test==14) {
        setup(0);set(0x20000a00,2775);
        const auto exit=query();
        check(exit.find(L"Exit 3 to Dome - Cloister of Trials")!=std::wstring::npos && exit.find(manualWarning)==std::wstring::npos,
            "encounter exit instruction omits the floor warning");
        qualifyFloor();query(OwnedGuideAction::Path);
        check(starts==1,"encounter exit retains its walking route");
        setup(5);set(0x20000a00,2875);set(0x0c000029,8);set(0x5c000014,0);set(0x5c000008,3);set(0x5c00000c,3);
        const auto chest=query();
        check(chest.find(L"Open the Destruction Sphere chest.")!=std::wstring::npos && chest.find(manualWarning)==std::wstring::npos,
            "treasure chest instruction omits the floor warning");
        qualifyFloor();query(OwnedGuideAction::Path);
        check(starts==1,"treasure chest retains its walking route");
        std::puts("PASS: Zanarkand unrelated exit and chest keep their ordinary Path behavior.");
    }
    if(!test || test==15) {
        setup(2);set(0x0c000020,2);query();routeFixture=true;
        const auto index=observation.count++;
        auto& worker=observation.workers[index];worker.valid=true;worker.local=46;worker.type=3;worker.part=0;
        Entity pedestal;pedestal.worker=index;pedestal.position={140,0,25};pedestal.pushControl=6;pedestal.zone.valid=true;
        const auto text=query(OwnedGuideAction::Path,0,{pedestal});
        check(starts==0 && routeAudio==0,"pattern pedestal route stays descriptive while active panels can reset the floor");
        check(pushAdviceQueries==0,"descriptive pedestal route does not request the standard push advice");
        check(text.find(manualWarning)==0 && text.find(L"Pattern pedestal 3")!=std::wstring::npos,
            "pattern pedestal Path begins with the floor crossing warning");
        observation.player=pedestal.position;
        const auto arrived=query(OwnedGuideAction::Path,0,{pedestal});
        check(pushAdviceQueries==0 && arrived.find(L"within interaction range")!=std::wstring::npos,
            "descriptive pedestal arrival also omits the standard push advice");
        std::puts("PASS: Zanarkand pattern pedestal route warns and leaves traversal manual.");
    }
    if(!test || test==16) {
        setup(0);
        const auto step=query(OwnedGuideAction::Step);
        check(step.find(L"Trial floor activation tile")!=std::wstring::npos && step.find(manualWarning)==std::wstring::npos,
            "activation tile instruction is warning-free when merely reading the step");
        routeFixture=true;plannedRoute={{105,0,-45}}; // W57 is active on the proposed route.
        const auto path=query(OwnedGuideAction::Path);
        check(starts==0 && routeAudio==0 && path.find(manualWarning)==0,
            "activation tile Path cannot walk through an unresolved active first-room pattern");
        observation.count=0;
        check(query(OwnedGuideAction::Path).find(manualWarning)==0 && starts==0,
            "missing callback observations cannot establish a clear route through an unresolved pattern");
        setup(0);for(unsigned worker=57;worker<=83;++worker) panel(worker,false);
        qualifyFloor();
        check(query(OwnedGuideAction::Path).find(manualWarning)==std::wstring::npos && starts==1,
            "the activation approach can walk when every relevant callback is observed inactive");
        std::puts("PASS: Zanarkand activation Path accounts for active, unavailable and inactive first-room panels.");
    }
    const auto select=[&](const wchar_t* name) {
        for(unsigned i=0;i<15;++i) if(query(OwnedGuideAction::Next).find(name)!=std::wstring::npos) return;
        check(false,"fixture could not select the later step");
    };
    if(!test || test==17) {
        setup(2);plannedRoute={{-25,5,55}}; // Active wrong W69 is between player and later targets.
        for(const auto* name:{L"Besaid Sphere source",L"Exit 3 to Dome - Cloister of Trials"}) {
            routeFixture=false;select(name);
            const auto step=query(OwnedGuideAction::Step);
            check(step.find(name)!=std::wstring::npos && step.find(manualWarning)==std::wstring::npos,
                "a later sphere or exit Step gives its usual objective without a floor warning");
            routeFixture=true;
            const auto path=query(OwnedGuideAction::Path);
            check(starts==0 && routeAudio==0 && path.find(manualWarning)==0 && path.find(name)!=std::wstring::npos,
                "an early sphere or exit Path cannot walk through an unresolved active main-room pattern");
        }
        set(0x0c000020,7);set(0x20000a00,2775);qualifyFloor();
        check(query(OwnedGuideAction::Path).find(manualWarning)==std::wstring::npos && starts==1,
            "the completed encounter exit is warning-free with ordinary walking");
        std::puts("PASS: Zanarkand manual selection of later normal-visit objectives leaves unsafe Paths descriptive.");
    }
    if(!test || test==18) {
        setup(5);set(0x20000a00,2875);set(0x0c000029,3);plannedRoute={{-25,5,55}};
        select(L"Open the Destruction Sphere chest.");
        check(query(OwnedGuideAction::Step).find(manualWarning)==std::wstring::npos,
            "the chest instruction alone omits the floor warning");
        routeFixture=true;
        const auto path=query(OwnedGuideAction::Path);
        check(starts==0 && routeAudio==0 && path.find(manualWarning)==0,
            "an early chest Path cannot walk through an unresolved active return pattern");
        set(0x0c000029,8);set(0x5c000008,0);set(0x5c00000c,3);qualifyFloor();
        check(query(OwnedGuideAction::Path).find(manualWarning)==std::wstring::npos && starts==1,
            "the revealed return objective has warning-free ordinary chest walking");
        std::puts("PASS: Zanarkand selected chest Path follows the native return-floor reveal milestone.");
    }
    if((test>=19 && test<=24) || !test) {
        for(int which=19;which<=24;++which) {
            if(test && test!=which) continue;
            setup(2);observation.player={-45,5,35};qualifyFloor();
            auto scalar=[](auto& bytes,unsigned offset,float value) {std::memcpy(bytes.data()+offset,&value,4);};
            for(unsigned i=0;i<27;++i) {
                auto& w=observation.workers[i];w.global=i;w.interactions.entryReady=true;
                panel(w.local,w.local==66 || (w.local==69 && which!=21));
                const auto p=w.local==66?Point{-5,5,75}:Point{-25,5,55};
                scalar(w.geometry,0,float(p.x));scalar(w.geometry,4,5);scalar(w.geometry,8,float(p.z));
                scalar(w.geometry,0x30,2);scalar(w.geometry,0x38,2);
                int16_t reference=-1;std::memcpy(w.geometry.data()+0x28,&reference,2);
            }
            narrowFloor=which==22;
            if(which==23) observation.workers[12].interactions.entryReady=false;
            if(which==24) observation.capturedAt-=1000;
            query();check(!starts && !routeAudio,"reading the step grants no movement consent");
            plannedRoute.clear();const auto text=query(OwnedGuideAction::Path);
            if(which==19) {
                check(starts==1 && plannedRoute.size()>2,"active W69 detour submits a walk");
                for(size_t i=1;i<plannedRoute.size();++i) {
                    // Independent segment/box check at quarter-unit resolution, including the player radius.
                    const auto a=plannedRoute[i-1],b=plannedRoute[i];
                    const auto n=unsigned(std::ceil(distanceXZ(a,b)*4))+1;
                    for(unsigned j=0;j<=n;++j) check(std::abs(a.x+(b.x-a.x)*j/n+25)>3.99 ||
                        std::abs(a.z+(b.z-a.z)*j/n-55)>3.99,"every detour leg clears W69");
                }
            }
            if(which==20) check(starts==1 && plannedRoute.size()>=2 && distanceXZ(plannedRoute.back(),{-5,5,75})<.01,
                "W66 exemption reaches its active center");
            if(which==21) check(starts==1 && plannedRoute.size()==2,"inactive W69 allows direct route");
            if(which>=22) check(!starts && !routeAudio && text.find(L"No safe auto walk route.")==0,
                "blocked, unreadable or stale panel facts give refusal first and no walk");
            std::printf("PASS: panel avoidance fixture %d.\n",which);
            narrowFloor=qualifiedFixture=false;
        }
    }
    logCaptureHandler.store(nullptr);
}
