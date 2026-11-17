// Production guide wording with synthetic owned state; no game assets or native calls.
#include "../puzzle_guide_owned.hpp"
#include "../utility_trials_data.hpp"
#include "../navigation_intents.hpp"
#include "../settings.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
void check(bool ok,const char* reason) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
[[noreturn]] void unexpectedFixtureCall() {check(false,"wording-only guide entered navigation or native work");std::abort();}
ffx::AtelIdentity identity;
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
void resetNavAudio() {unexpectedFixtureCall();}
void routeNavAudio(const std::vector<Point>&,const Entity&,bool) {unexpectedFixtureCall();}
int setting(Setting) {unexpectedFixtureCall();}
bool inInteractionZone(const InteractionZone&,Point) {unexpectedFixtureCall();}
std::vector<Point> navigationRouteOwned(const Mesh&,Point,const Entity&,const std::vector<Entity>&) {unexpectedFixtureCall();}
namespace nav {
bool scenePedestalPlan(const Scene&,const Entity&,PedestalPlan&) {unexpectedFixtureCall();}
InteractionZone sceneInteractionZone(const WorkerFact&,unsigned,float) {unexpectedFixtureCall();}
bool submitNavigationIntent(const Scene&,const Entity&,Intent,unsigned,unsigned,bool) {unexpectedFixtureCall();}
}
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    using namespace ffx;
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
}
