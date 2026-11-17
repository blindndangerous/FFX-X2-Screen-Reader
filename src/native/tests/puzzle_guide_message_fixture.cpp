#include "fixture_runtime.hpp"
// Production guide wording with synthetic owned state; no game assets or native calls.
#include "../puzzle_guide_owned.hpp"
#include "../utility_trials_data.hpp"
#include "../puzzle_guide_bindings.hpp"
#include "../navigation_intents.hpp"
#include "../settings.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
void check(bool ok,const char* reason) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
[[noreturn]] void unexpectedFixtureCall() {check(false,"wording-only guide entered navigation or native work");std::abort();}
ffx::AtelIdentity identity;
bool inspectTarget=false;
struct TargetObserved {ffx::Entity target;};
}
// These dependencies must stay unused by a plain step read. Fail rather than
// simulate pathfinding, speech, native memory, or pedestal actions.
namespace ffx {
bool nativeGameThread() {return false;}
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& out) {out=identity;return chunk==identity.span.chunk;}
double distanceXZ(Point,Point) {unexpectedFixtureCall();}
void configurePedestalTarget(Entity&,const char*,unsigned) {unexpectedFixtureCall();}
OwnedTextContext copiedControlContext(const CopiedControls&) {unexpectedFixtureCall();}
std::wstring pedestalStepText(const PedestalPlan&,bool,bool) {unexpectedFixtureCall();}
std::wstring describePedestalApproach(const PedestalPlan&,Point,Point) {unexpectedFixtureCall();}
std::wstring describePedestalPlan(const PedestalPlan&,Point,const std::wstring&) {unexpectedFixtureCall();}
std::wstring join(const std::vector<std::wstring>&,const wchar_t*) {unexpectedFixtureCall();}
std::wstring direction(Point) {unexpectedFixtureCall();}
void resetNavAudio() {if(!inspectTarget) unexpectedFixtureCall();}
void routeNavAudio(const std::vector<Point>&,const Entity&,bool) {unexpectedFixtureCall();}
int setting(Setting) {unexpectedFixtureCall();}
bool inInteractionZone(const InteractionZone&,Point) {unexpectedFixtureCall();}
std::vector<Point> navigationRouteOwned(const Mesh&,Point,const Entity& target,const std::vector<Entity>&) {
    if(inspectTarget) throw TargetObserved{target};unexpectedFixtureCall();
}
namespace nav {
bool scenePedestalPlan(const Scene&,const Entity&,PedestalPlan&) {unexpectedFixtureCall();}
InteractionZone sceneInteractionZone(const WorkerFact&,unsigned,float) {unexpectedFixtureCall();}
bool submitNavigationIntent(const Scene&,const Entity&,Intent,unsigned,unsigned,bool) {unexpectedFixtureCall();}
}
}
int main() {
    ffx_fixture::initializeRuntime();
    using namespace ffx;
    static nav::Scene noTrial;
    for(const auto action:{OwnedGuideAction::Step,OwnedGuideAction::Expected,OwnedGuideAction::Path,OwnedGuideAction::Next,OwnedGuideAction::Previous})
        check(puzzleGuideOwned(action,noTrial,Mesh{},std::vector<Entity>{},speechq::Header{},0,0)==L"No puzzle guide for this area.","manual no-context guide actions use the concise unavailable-guide response");
    check(puzzleGuideOwned(OwnedGuideAction::Completed,noTrial,Mesh{},std::vector<Entity>{},speechq::Header{},0,0).empty(),"completed no-context event stays silent");
    check(puzzleGuideOwned(OwnedGuideAction::TileCompleted,noTrial,Mesh{},std::vector<Entity>{},speechq::Header{},0,0).empty(),"tile-completed no-context event stays silent");
    static nav::Scene observation;
    observation.epoch=1;observation.primary=0x10000;observation.partCount=1;
    identity.span={observation.primary,observation.epoch,1,0x100};
    identity.length=trialUtilities[0].length;identity.checksum=trialUtilities[0].checksum;
    std::memcpy(identity.name,trialUtilities[0].script,8);
    observation.parts[0].span=identity.span;
    for(const bool control:{false,true}) for(const bool message:{false,true})
    for(const int window:{0,6}) for(const bool visible:{false,true}) for(const bool pushing:{false,true}) {
        resetOwnedPuzzleGuide();observation.control=control;observation.message=message;
        observation.window=window;observation.visibleMessage=visible;
        observation.pedestal.armed=observation.pedestal.held=pushing;
        const auto text=puzzleGuideOwned(OwnedGuideAction::Step,observation,Mesh{},std::vector<Entity>{},speechq::Header{},0,0);
        check(text.find(L"Step 1 of")!=std::wstring::npos,"synthetic scene reaches the production trial guide");
        const auto close=text.find(L"Press Enter to close the message");
        check((close!=std::wstring::npos)==visible,"production guide close hint requires visible text");
        check(close==std::wstring::npos || text.find(L"Press Enter to close the message",close+1)==std::wstring::npos,
            "production guide never duplicates its close hint");
        check((text.find(L"Wait for the current scene or push to finish")!=std::wstring::npos)==
            (!visible && (!control || message || window || pushing)),"production guide waits for scripts without asking to close nonexistent text");
    }
    std::puts("PASS: 32 production puzzle-guide message/scene/control/push wording cases; no game evidence required.");
    observation.control=true;observation.message=observation.visibleMessage=false;observation.window=0;
    observation.pedestal.armed=observation.pedestal.held=false;
    identity.span={observation.primary,observation.epoch,1,0x100};
    identity.length=trialUtilities[1].length;identity.checksum=trialUtilities[1].checksum;
    std::memcpy(identity.name,trialUtilities[1].script,8);
    auto& part=observation.parts[0];part.span=identity.span;part.guideReady=true;
    for(const auto binding:nav::guide_klyt1000) part.bindings.rows[part.bindings.count++]={binding.low,0,binding.high,binding.element};
    const auto set=[&](unsigned binding,int value) {for(auto& row:part.bindings.rows) if(row.binding==binding) {row.value=value;return;}check(false,"fixture binding exists");};
    const auto expected=[&] {return puzzleGuideOwned(OwnedGuideAction::Expected,observation,Mesh{},std::vector<Entity>{},speechq::Header{},0,0);};
    set(0x01000a70,2);set(0x0c000061,6);set(0x0c000064,1);set(0x0c000062,3);
    resetOwnedPuzzleGuide();check(expected().find(L"Step 21 of 22.")==0,"filled final door expects removal after treasure");
    set(0x0c000062,0);set(0x0c000063,3);set(0x0c00005b,3);
    const auto exit=expected();
    check(exit.find(L"Step 22 of 22. Target: Exit 1 to Kilika - Antechamber.")==0,
        "final guide names the selectable worker-37 exit row, not an invented destination");
    set(0x0c000062,3);set(0x0c00005b,0);
    check(expected().find(L"Step 21 of 22.")==0,"reinserting the final sphere returns the active guide to removal");
    check(expected().find(L"Step 21 of 22.")==0,"repeated reads cannot latch the old door-clear milestone");
    resetOwnedPuzzleGuide();check(expected().find(L"Step 21 of 22.")==0,"fresh guide respects the refilled recess too");
    set(0x0c000062,0);set(0x0c00005b,3);
    check(expected().find(L"Step 22 of 22.")==0,"removing the reinserted sphere restores the exit step");
    observation.count=2;observation.workers[0].part=observation.workers[1].part=0;
    observation.workers[0].local=39;observation.workers[1].local=37;
    Entity other;other.worker=0;other.name=L"Exit 3 to Kilika - Antechamber";other.position={3,0,300};
    Entity selected;selected.worker=1;selected.name=L"Exit 1 to Kilika - Antechamber";selected.position={1,0,300};
    inspectTarget=true;bool resolved=false;
    Mesh mesh;mesh.openPlane=true;
    try {puzzleGuideOwned(OwnedGuideAction::Path,observation,mesh,{other,selected},speechq::Header{},0,0);}
    catch(const TargetObserved& seen) {resolved=seen.target.worker==1 && seen.target.name==selected.name && seen.target.position.x==1;}
    inspectTarget=false;check(resolved,"guide path selects worker 37 and the matching Exit 1 row even when Exit 3 comes first");
    std::puts("PASS: Kilika final-door removal, reinsertion, fresh-guide recovery and selectable exit wording.");
}
