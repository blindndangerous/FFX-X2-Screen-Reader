#include "fixture_runtime.hpp"
// Production guide wording with synthetic owned state; no game assets or native calls.
#include "../puzzle_guide_owned.hpp"
#include "../utility_trials_data.hpp"
#include "../puzzle_guide_bindings.hpp"
#include "../navigation_intents.hpp"
#include "../settings.hpp"
#include "../log_capture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

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
struct TargetObserved {ffx::Entity target;};
}
// These dependencies must stay unused by a plain step read. Fail rather than
// simulate pathfinding, speech, native memory, or pedestal actions.
namespace ffx {
bool nativeGameThread() {return false;}
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& out) {out=identity;return chunk==identity.span.chunk;}
double distanceXZ(Point,Point) {unexpectedFixtureCall();}
void configurePedestalTarget(Entity& target,const char* script,unsigned worker) {
    // Step reads may seek push advice. No positioned pedestal exists in this
    // fixture, so production target() declines that optional advice.
    if(!std::strcmp(script,"bvyt1100") && worker==13) {target.pushControl=5;return;}
    if(!std::strcmp(script,"mcyt0500") && worker==1) {target.pushControl=4;return;}
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
std::wstring direction(Point) {unexpectedFixtureCall();}
void resetNavAudio() {if(!inspectTarget) unexpectedFixtureCall();}
void routeNavAudio(const std::vector<Point>&,const Entity&,bool) {unexpectedFixtureCall();}
int setting(Setting) {unexpectedFixtureCall();}
bool inInteractionZone(const InteractionZone&,Point) {if(railControlNames) return railInZone;unexpectedFixtureCall();}
std::vector<Point> navigationRouteOwned(const Mesh&,Point,const Entity& target,const std::vector<Entity>&) {
    if(inspectTarget) throw TargetObserved{target};unexpectedFixtureCall();
}
namespace nav {
bool scenePedestalPlan(const Scene&,const Entity&,PedestalPlan&) {unexpectedFixtureCall();}
InteractionZone sceneInteractionZone(const WorkerFact&,unsigned,float) {if(railControlNames) return {};unexpectedFixtureCall();}
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
    const auto expected=[&] {
        observation.capturedAt=GetTickCount64();
        return puzzleGuideOwned(OwnedGuideAction::Expected,observation,Mesh{},std::vector<Entity>{},speechq::Header{},0,0);
    };
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
    identity.length=trialUtilities[4].length;identity.checksum=trialUtilities[4].checksum;
    std::memcpy(identity.name,trialUtilities[4].script,8);
    part.bindings.count=0;
    for(const auto binding:nav::guide_bvyt1100) part.bindings.rows[part.bindings.count++]={binding.low,0,binding.high,binding.element};
    // No query observes node 1: the player confirms and finishes riding before
    // asking again. Exercise production done()/follow(), not a copied predicate.
    resetOwnedPuzzleGuide();
    check(expected().find(L"Step 1 of 15.")==0,"fresh Bevelle node 0 still expects the entrance push");
    observation.message=true;
    check(expected().find(L"Step 1 of 15.")==0,"Bevelle prompt keeps the current step while settling");
    observation.message=false;observation.control=false;set(0x0c000008,19);
    check(expected().find(L"Step 1 of 15.")==0,"Bevelle ride keeps the current step without control");
    observation.control=true;
    check(expected().find(L"Step 2 of 15.")==0,"Bevelle 0 -> 19 without observing 1 advances past the entrance push");
    for(const int node:{1,19,65,66}) {
        resetOwnedPuzzleGuide();set(0x0c000008,node);
        check(expected().find(L"Step 2 of 15.")==0,"fresh guide recognizes an already departed Bevelle rail pedestal");
    }
    set(0x0c000008,0);resetOwnedPuzzleGuide();
    check(expected().find(L"Step 1 of 15.")==0,"Bevelle room reset restores the entrance step");
    // The optional push's node 16 can also be missed, but the chest checkpoint
    // is durable and already skips it when the completed ride reaches node 17.
    set(0x0c000029,1);set(0x0c000009,15);
    check(expected().find(L"Step 13 of 15.")==0,"calling the upper pedestal does not complete the optional push");
    set(0x0c000009,17);set(0x0c00002a,1);
    check(expected().find(L"Step 15 of 15.")==0,"optional chest checkpoint recovers when node 16 was never observed");
    std::puts("PASS: Bevelle skipped entrance node, settling, late guide, room reset and skipped optional-push node.");
    railControlNames=true;
    observation.confirmMask=0x20;
    // Native W15 enables W40 while waiting for its action-5 confirmation.
    // Player control stays disabled; an invisible raw message mode may remain.
    observation.count=1;auto& rail=observation.workers[0];rail={};
    rail.valid=true;rail.part=0;rail.local=40;rail.type=3;
    rail.interactions.channels=8;rail.interactions.entries[3]=5;
    const auto query=[&](OwnedGuideAction action,bool fresh=true) {
        observation.capturedAt=GetTickCount64()-(fresh?0:1000);
        return puzzleGuideOwned(action,observation,Mesh{},std::vector<Entity>{},speechq::Header{},0,0);
    };
    bool railPolicy=true;
    const auto verifyRail=[&](bool ok,const char* description) {
        std::printf("%s: %s\n",ok?"PASS":"FAIL",description);railPolicy&=ok;
    };
    observation.control=false;observation.message=true;observation.visibleMessage=false;
    for(auto& row:part.bindings.rows) row.value=0;
    set(0x0c000008,19);rail.local=43;resetOwnedPuzzleGuide();
    for(const auto action:{OwnedGuideAction::Expected,OwnedGuideAction::Path}) {
        set(0x0c00000c,2);
        verifyRail(query(action).find(L"Release C. Wait for the left arrow.")==0,
            "W30 rail query lets the wrong right turn pass without taking a walking path");
        set(0x0c00000c,1);
        verifyRail(query(action).find(L"Press C now to turn left.")==0,
            "W30 rail query begins with usable input for the desired left turn");
        const auto stale=query(action,false);
        verifyRail(stale.find(L"Wait for a fresh rail cue.")==0 && stale.find(L"Press C now")==std::wstring::npos,
            "stale W43 phase cannot prompt immediate input on an explicit rail query");
    }
    verifyRail(query(OwnedGuideAction::Completed)==L"Press C now to turn left.","W30 proactive cue starts with the desired input");
    set(0x0c00000c,2);
    verifyRail(query(OwnedGuideAction::Completed)==L"Release C. Wait for the left arrow.","W30 proactive cue lets the wrong turn pass");
    for(auto& row:part.bindings.rows) if(row.binding==0x0d000032) row.value=row.element==25?1:0;
    verifyRail(query(OwnedGuideAction::Completed)==L"Press C now to turn right.","W31 retargets the same live phase to the right turn");
    verifyRail(query(OwnedGuideAction::Path).find(L"Press C now to turn right.")==0,"W31 path starts with input for the right turn");
    set(0x0c00000c,1);
    verifyRail(query(OwnedGuideAction::Expected).find(L"Release C. Wait for the right arrow.")==0,"W31 query lets the wrong left turn pass");
    set(0x0c000025,1);
    verifyRail(query(OwnedGuideAction::Completed)==L"Let this junction pass.","the objective below the upper junction needs neither W43 branch");
    verifyRail(query(OwnedGuideAction::Path).find(L"Let this junction pass.")==0,"Path also lets the junction pass for a lower-floor objective");
    for(auto& row:part.bindings.rows) row.value=0;
    set(0x0c000008,19);observation.visibleMessage=true;
    for(const unsigned worker:{40u,45u,46u,47u}) {
        rail.local=worker;resetOwnedPuzzleGuide();
        observation.bevelleRailPromptWorker=0;
        const auto reward=query(OwnedGuideAction::Expected);
        const auto rewardPath=query(OwnedGuideAction::Path);
        verifyRail(reward.find(L"close the message")!=std::wstring::npos && rewardPath.find(L"close the message")!=std::wstring::npos &&
            query(OwnedGuideAction::Completed).empty(),"reward dialogue remains blocking in an enabled rail prompt region");
        observation.bevelleRailPromptWorker=worker==40?45:40;
        verifyRail(query(OwnedGuideAction::Expected).find(L"close the message")!=std::wstring::npos,
            "another rail worker's visible prompt cannot authorize this callback");
        observation.bevelleRailPromptWorker=worker;
        const auto prompt=query(OwnedGuideAction::Path);
        verifyRail(prompt.find(L"Press C now")==0 && prompt.find(L"close the message")==std::wstring::npos,
            "verified visible prompt exposes only its own immediate rail input");
    }
    observation.bevelleRailPromptWorker=0;observation.visibleMessage=false;
    check(railPolicy,"Bevelle rail objectives, actionable freshness and visible prompt identity");
    // Both reported failures must be visible in the same red run. Geometry is
    // supplied by the existing zone dependency; this fixture tests guide policy.
    bool railRegression=true;
    for(const unsigned inputWorker:{40u,43u}) {
        for(auto& row:part.bindings.rows) row.value=0;
        set(0x0c000008,inputWorker==40?1:19);resetOwnedPuzzleGuide();
        rail.local=inputWorker;observation.control=false;observation.message=true;
        observation.visibleMessage=inputWorker==40;
        observation.bevelleRailPromptWorker=observation.visibleMessage?40:0;
        const auto text=expected();
        const bool ok=text.find(L"Step 2 of 15.")!=std::wstring::npos &&
            text.find(inputWorker==40?L"Press C now to start.":L"Wait for the left arrow.")==0 &&
            text.find(L"close the message")==std::wstring::npos && text.find(L"Wait for the current scene")==std::wstring::npos;
        std::printf("%s: Bevelle %s gives timing and objective guidance.\n",ok?"PASS":"FAIL",inputWorker==40?"visible entrance prompt":"junction without W40");
        railRegression&=ok;
    }
    check(railRegression,"both live-reported rail input cases");
    rail.local=40;observation.visibleMessage=false;observation.bevelleRailPromptWorker=0;
    for(const bool message:{false,true}) {
        for(auto& row:part.bindings.rows) row.value=0;
        observation.control=false;observation.message=message;resetOwnedPuzzleGuide();
        check(expected().find(L"Step 1 of 15.")==0,"unmoved Bevelle pedestal cannot become a rail input stop");
        set(0x0c000008,1);
        const auto stopped=expected();
        check(stopped.find(L"Step 2 of 15.")!=std::wstring::npos,"Bevelle rail input stop advances without ordinary player control");
        check(stopped.find(L"Press C now to start.")==0 &&
            stopped.find(L"Wait for")==std::wstring::npos && stopped.find(L"close the message")==std::wstring::npos,
            "rail input stop speaks its configured confirmation instead of scene or dialogue waiting");
        for(unsigned repeat=0;repeat<3;++repeat)
            check(expected().find(L"Step 2 of 15.")!=std::wstring::npos,"repeated rail-stop queries keep progression without waiting for control");
        // Console collect writes the chest bit, without the normal route's 0x26.
        set(0x01000a70,16);
        check(expected().find(L"Step 2 of 15.")!=std::wstring::npos,"collection keeps the required upper rail access step");
        check(expected().find(L"Step 2 of 15.")!=std::wstring::npos,"repeated rail-stop reads still require transport");
    }
    // Old saved treasure alone must not bypass a fresh room, even after boarding.
    resetOwnedPuzzleGuide();set(0x0c000008,0);
    check(expected().find(L"Step 1 of 15.")==0,"old Bevelle treasure bit does not skip the fresh room");
    set(0x0c000008,1);
    check(expected().find(L"Step 2 of 15.")!=std::wstring::npos,"old treasure bit plus boarding is not a new collection checkpoint");
    set(0x0c000026,1);resetOwnedPuzzleGuide();
    check(expected().find(L"Press C now to start.")==0 && expected().find(L"Upper platform sphere recess")==std::wstring::npos,"revealing the final route does not complete its ascent");
    railConfirm=L"A";observation.controls.controller=true;observation.controls.swap=true;
    check(expected().find(L"Press A now to start.")==0,"rail stop names the configured controller action");
    railConfirm=L"unassigned control";
    check(expected().find(L"Press Confirm now to start.")==0,"rail stop has an action-name fallback for missing binding names");
    railConfirm=L"C";observation.controls={};
    observation.confirmMask=0x40;
    check(expected().find(L"Press X now to start.")==0,"field-remapped Confirm names the actual raw binding");
    observation.confirmMask=0x20;
    check(query(OwnedGuideAction::Path).find(L"Press C now to start.")==0,
        "rail-stop path request gives usable input guidance without attempting a walking path");
    for(unsigned blocked=0;blocked<7;++blocked) {
        for(auto& row:part.bindings.rows) row.value=0;
        set(0x0c000008,1);resetOwnedPuzzleGuide();
        rail.valid=blocked!=0;rail.part=blocked==1?1:0;rail.interactions.channels=blocked==2?0:8;
        rail.interactions.entries[3]=blocked==3?6:5;
        rail.type=blocked==4?2:3;observation.visibleMessage=false;observation.window=blocked==5?6:0;
        observation.pedestal.armed=observation.pedestal.held=blocked==6;
        const auto held=expected();
        check(held.find(L"Step 1 of 15.")==0 && held.find(L"Press C now")==std::wstring::npos,
            "unqualified rail callback, wrong type, window and held push retain settling protection");
    }
    std::puts("PASS: Bevelle disabled-control rail input, repeat queries, configured input, safe settling and current-session chest checkpoint.");
    for(auto& row:part.bindings.rows) row.value=0;
    set(0x0c000008,19);observation.window=0;observation.visibleMessage=false;
    observation.pedestal.armed=observation.pedestal.held=false;observation.control=false;
    rail.valid=true;rail.part=0;rail.type=3;rail.interactions.channels=8;rail.interactions.entries[3]=5;
    for(unsigned worker=40;worker<=51;++worker) {
        rail.local=worker;resetOwnedPuzzleGuide();
        const auto text=expected();
        check(text.find(L"Step 2 of 15.")!=std::wstring::npos,"each enabled nearby rail input region preserves the current objective");
        check(text.find(L"Direction keys do not choose a branch")!=std::wstring::npos,
            "rail guide distinguishes confirm timing from direction-key steering");
        const bool timed=worker==41 || worker==44 || worker==48 || worker==49 || worker==50;
        check((text.find(L"Wait for the turn arrow.")==0)==timed,
            "only script-gated junctions describe their unavailable cycling phase");
        if(timed) {
            const unsigned binding=worker==41?0x0c00000b:worker==44?0x0c00000d:worker==48?0x0c00000f:worker==49?0x0c000010:0x0c000011;
            set(binding,worker==44?3:1);
            check(expected().find(L"Press C now to turn.")==0,"accepted arrow phase gives direct confirm advice");
            set(binding,0);
        }
        if(worker==43) {
            set(0x0c00000c,1);
            check(expected().find(L"Press C now to turn left.")==0,"upper second junction phase 1 offers the source turn");
            set(0x0c00000c,2);
            check(expected().find(L"Release C. Wait for the left arrow.")==0,"upper second junction phase 2 waits for the source turn");
            set(0x0c00000c,0);
            check(expected().find(L"Wait for the left arrow.")==0,"unknown arrow phase waits for the desired turn");
        }
        if(!timed && worker!=43) check(text.find(L"Press C now")==0,"ungated rail input regions offer immediate input");
        const auto latch=worker==40?0x0c000014:0x0c00002c;
        set(latch,1);
        check(expected().find(L"Wait for the current scene")!=std::wstring::npos,"accepted confirm remains a scene until its continuation finishes");
        set(latch,0);railInZone=false;
        check(expected().find(L"Wait for the current scene")!=std::wstring::npos,"distant enabled input regions cannot turn a moving ride into a stop");
        railInZone=true;observation.visibleMessage=true;
        const bool railMessage=worker==40 || (worker>=45 && worker<=47);
        observation.bevelleRailPromptWorker=railMessage?worker:0;
        const auto visible=expected();
        check((visible.find(L"close the message")==std::wstring::npos)==railMessage,
            "only callbacks that own a rail message can override visible-dialogue guidance");
        observation.visibleMessage=false;observation.bevelleRailPromptWorker=0;
    }
    std::puts("PASS: all 12 Bevelle rail callbacks, cycling-arrow phases, accepted-input latches, proximity and unrelated dialogue guards.");
    rail.local=43;observation.count=3;
    auto& entrance=observation.workers[1];entrance=rail;entrance.local=40;entrance.interactions.channels=0;
    auto& launch=observation.workers[2];launch=rail;launch.local=39;launch.type=2;launch.interactions.channels=32;launch.interactions.entries[5]=6;
    logCaptureHandler=captureRailLog;Sleep(1100);expected();
    check(railLogs==1 && std::strstr(railLog,"visibleMessage=0 window=0 control=0") &&
        std::strstr(railLog,"node=0x0c000008:19") && std::strstr(railLog,"input-worker=43") &&
        std::strstr(railLog,"W40={valid=1,type=3,channels=0x0,entries=[") &&
        std::strstr(railLog,"W39={valid=1,type=2,channels=0x20,entries=["),"query diagnostic contains actual owned predicate inputs");
    for(unsigned repeat=0;repeat<10;++repeat) expected();
    check(railLogs==1,"rapid guide queries emit at most one rail diagnostic per second");
    Sleep(1100);
    puzzleGuideOwned(OwnedGuideAction::Completed,observation,Mesh{},std::vector<Entity>{},speechq::Header{},0,0);
    check(railLogs==1,"background guide completion cannot emit the diagnostic after its rate limit expires");
    expected();check(railLogs==2,"later explicit guide query emits the next diagnostic");
    logCaptureHandler=nullptr;
    std::puts("PASS: Bevelle query-only diagnostic fields, rapid-query rate limit and later-query renewal.");
    const auto tick=[&] {observation.capturedAt=GetTickCount64();return puzzleGuideOwned(OwnedGuideAction::Completed,observation,Mesh{},std::vector<Entity>{},speechq::Header{},0,0);};
    resetOwnedPuzzleGuide();rail.local=43;set(0x0c00000c,1);
    check(tick().empty(),"inactive guide never announces a junction");
    expected();
    check(tick()==L"Press C now to turn left.","active guide proactively announces W43 left window");
    for(unsigned i=0;i<10;++i) check(tick().empty(),"same window and phase is spoken only once");
    set(0x0c00000c,2);
    check(tick()==L"Release C. Wait for the left arrow.","W43 phase change reports the wait without another query");
    check(tick().empty(),"changed phase is deduplicated");
    railInZone=false;check(tick().empty(),"window closure stays silent");railInZone=true;
    check(!tick().empty(),"next circuit reopens the same phase");
    set(0x0c00002c,1);check(tick().empty(),"accepted turn does not announce press now");set(0x0c00002c,0);
    for(unsigned worker=40;worker<=51;++worker) {
        rail.local=worker;railInZone=false;tick();railInZone=true;
        const unsigned binding=worker==41?0x0c00000b:worker==44?0x0c00000d:worker==48?0x0c00000f:worker==49?0x0c000010:worker==50?0x0c000011:0;
        if(binding) set(binding,0);
        const auto cue=tick();check(!cue.empty(),"every qualified rail window opens proactively");
        if(binding) {check(cue.find(L"Press C now")==std::wstring::npos,"closed arrow phase cannot invite confirmation");set(binding,worker==44?3:1);check(tick().find(L"Press C now")!=std::wstring::npos,"turn-ready transition announces confirm");}
        check(tick().empty(),"all rail workers deduplicate repeated observations");
    }
    rail.local=43;set(0x0c00000c,1);observation.capturedAt=GetTickCount64()-1000;
    check(puzzleGuideOwned(OwnedGuideAction::Completed,observation,Mesh{},std::vector<Entity>{},speechq::Header{},0,0).empty(),"old snapshots never announce a current timing cue");
    check(!tick().empty(),"fresh observation after stale data reestablishes timing");
    resetOwnedPuzzleGuide();check(tick().empty(),"guide reset cancels timing speech");expected();check(!tick().empty(),"re-enabled guide can announce the current window");
    std::puts("PASS: Bevelle proactive opening/phase cues, deduplication, next circuit, acceptance, freshness and reset.");
    identity.length=trialUtilities[3].length;identity.checksum=trialUtilities[3].checksum;
    std::memcpy(identity.name,trialUtilities[3].script,8);
    part.bindings.count=0;
    for(const auto binding:nav::guide_mcyt0500) part.bindings.rows[part.bindings.count++]={binding.low,0,binding.high,binding.element};
    observation.count=1;rail={};rail.valid=true;rail.part=0;rail.local=1;rail.type=1;rail.actor=0;
    const auto baseline=[&](int phase,Point position) {
        set(0x0c0000a3,phase);rail.position=position;observation.control=true;
        observation.message=false;resetOwnedPuzzleGuide();
        check(expected().find(L"Step ")==0,"explicit Macalania guide read provides the current step");
    };
    baseline(0,{34,39,12});set(0x0c0000a3,1);rail.position.z=52;
    check(tick()==L"Pedestal push complete.","first guide read seeds a coalesced W1 push completion before any previous Completed event");
    check(tick().empty(),"coalesced completion after the first guide read is deduplicated");
    struct NativePush {int from,to;Point start,moving,end;const char* name;};
    const NativePush pushes[]={
        {0,1,{34,39,12},{34,39,13},{34,39,52},"W1:10 first mound push starts with source phase 0"},
        {0,2,{34,39,12},{33,39,12},{-83,39,12},"W1:10 alternate mound push starts with source phase 0"},
        {1,3,{34,39,52},{33,39,52},{-83,10,52},"W1:10 lower-floor push starts with source phase 1"},
        {3,4,{-83,10,52},{-83,10,53},{-83,10,66},"W1:10 bridge push starts with source phase 3"},
        {5,6,{11,0,-105},{10,0,-105},{-26,-20,-105},"W1:12 upper-walkway push starts with source phase 5"},
    };
    for(const auto& push:pushes) {
        baseline(push.from,push.start);
        observation.control=false;check(tick().empty(),"control loss alone is silent");
        rail.position=push.moving;check(tick()==L"Pushing pedestal.",push.name);
        check(tick().empty(),"manual push start is deduplicated");
        set(0x0c0000a3,push.to);rail.position=push.end;
        check(tick().empty(),"destination phase waits for field control");
        observation.control=true;check(tick()==L"Pedestal push complete.","verified W1 destination completes without phase 99");
        check(tick().empty(),"manual completion is deduplicated");
    }
    baseline(0,{34,39,12});set(0x0c0000a3,1);
    check(tick().empty(),"destination phase without corroborating motion stays silent");
    baseline(3,{-83,10,52});observation.control=false;set(0x0c0000a3,0);rail.position={34,39,12};
    check(tick().empty(),"W1:8 reset does not announce manual push motion");
    observation.control=true;check(tick().empty(),"W1:8 reset does not announce completion");
    baseline(4,{-83,10,66});observation.control=false;set(0x0c0000a3,5);rail.position={11,0,-105};
    check(tick().empty(),"W1:9 walkway reset does not announce manual push motion");
    observation.control=true;check(tick().empty(),"W1:9 walkway reset does not announce completion");
    observation.pedestal.armed=true;observation.control=false;rail.position.x-=1;
    check(tick().empty(),"armed assistance owns its own push speech");
    observation.pedestal.armed=false;rail.position.x-=1;
    check(tick().empty(),"handback while control is absent does not adopt assisted motion");
    set(0x0c0000a3,6);rail.position.x=-26;observation.control=true;
    check(tick().empty(),"handback does not announce an assisted destination");
    baseline(0,{34,39,12});observation.control=false;rail.position.z+=1;
    check(tick()==L"Pushing pedestal.","a later independent manual push is announced");
    set(0x0c0000a3,99);check(tick().empty(),"phase 99 never completes a push");
    observation.control=true;check(tick().empty(),"field control at phase 99 does not imply completion");
    baseline(0,{34,39,12});part.guideReady=false;check(tick().empty(),"unavailable observations clear the manual baseline");part.guideReady=true;
    set(0x0c0000a3,1);rail.position.z=52;check(tick().empty(),"guide readiness recovery does not invent a completion");
    resetOwnedPuzzleGuide();rail.position.z+=1;check(tick().empty(),"inactive guide never announces manual movement");
    std::puts("PASS: manual Macalania push start/completion, deduplication, reset, assisted handback and unavailable-state guards.");
}
