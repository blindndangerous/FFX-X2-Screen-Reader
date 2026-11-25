#include "fixture_runtime.hpp"
// Production command/scene/resource/route/intent pipeline over retained EVs.
// Native scene/getter and mutation endpoints are controlled adapters. Domain
// transport is semantic; actual capture/SDK/MinHook ownership has separate hosts.
#define FFX_NAVIGATION_SERVICE_TEST
#define main navigationSceneComponentEntry
#include "navigation_scene_fixture.cpp"
#undef main
#include "../navigation_intents.hpp"
#include "../owned_nav_target.hpp"
#include "../puzzle_guide_owned.hpp"
#include "../puzzle_guide_targets.hpp"
#include "../nav_audio.hpp"
#include "../sphere_grid.hpp"
#include "../auto_walk.hpp"
#include "../floor_panels.hpp"
#include "../log_capture.hpp"
#include "../reachability.hpp"
#include "../text_plan.hpp"
#include "../mod_state.hpp"
#include "../input_device.hpp"
namespace {
unsigned starts=0,faces=0,arms=0,actions=0,refusals=0;
bool entranceCrossing=false,armedWithWalk=false;
bool (*resumeWalk)(const ffx::NativeRouteTarget&)=nullptr;
std::array<uint8_t,0x668> cameraBytes{};
}
namespace ffx {
// The unrelated Gil producer is covered by reader/TextPlan hosts; reaching it
// in this navigation command fixture is an error, never invented text behavior.
void prepareTextPlan(TextPlan&,const char*,bool) {check(false,"unexpected Gil producer");}
TextRef planLiteral(TextPlan&,const wchar_t*) {check(false,"unexpected Gil literal");return 0;}
TextRef planInteger(TextPlan&,int64_t) {check(false,"unexpected Gil value");return 0;}
TextRef planConcat(TextPlan&,TextRef,TextRef) {check(false,"unexpected Gil formatting");return 0;}
bool commitTextPlan(const TextPlan&) {check(false,"unexpected Gil commit");return false;}
std::vector<Entity> butterflyTargets() {check(false,"legacy live butterfly scan entered");return {};}
void refreshButterflyTarget(Entity&) {check(false,"legacy affine butterfly refresh entered");}
void refreshButterflyStarter(Entity&) {check(false,"legacy affine starter refresh entered");}
void hotkey(unsigned vk,void(*fn)(bool)) {navigationKeys[vk]=fn;}
bool nativeReaderResourcesReady() noexcept {return true;}
bool deferNativeAction(NativeAction action) {
    check(nativeOwner,"intent stays on Native action owner");bool ready=action.prepare(action);
    if(ready) {action.invoke(action);++actions;}else ++refusals;
    if(action.complete) action.complete(action,ready);if(action.retire) action.retire(action);return true;
}
void installAutoWalk() {}void installPedestal() {}
void stopAutoWalk() {++manualInputRevision;}
void suspendAutoWalk() {stopAutoWalk();}
void releaseAutoWalk() {stopAutoWalk();}
bool autoWalkResumePending() {return false;}
bool autoWalkActive() {return false;}
bool toggleAutoWalkOff(void(*)()) {return false;}
void setAutoWalkResume(bool(*resume)(const NativeRouteTarget&)) {resumeWalk=resume;}
bool utilityBusy() {return false;}
std::wstring nativeButtonMaskName(uint16_t) {return L"Confirm";}
void startAutoWalk(bool) {check(nativeOwner,"approved start stays Native");++starts;}
void cancelPedestal() {}
bool pedestalEntranceCrossing() {return entranceCrossing;}
bool armPedestalOwned(const NativeRouteTarget&,const UtilityScript&,const PedestalPlan&,bool walk) {check(nativeOwner,"arming stays Native");++arms;armedWithWalk=walk;return true;}
void updatePedestalOwnedTarget(const NativeRouteTarget&) {check(nativeOwner,"POD assistance handback stays Native");}
bool faceNavigationTarget(const NativeRouteTarget&) {check(nativeOwner,"facing stays Native");++faces;return true;}
bool sphereGridOpen() {return false;}void spherePathKey(bool) {}
void resetBattle() {}void resetNavigationScripts() {}
void finishMapArrival() {}void announceCurrentMap() {}void prepareNavigationScriptRetirement() {}
void consoleReplyReady(const wchar_t*,size_t) {}
void adjustSetting(Setting,int) {}std::wstring settingValue(Setting) {return L"On";}
std::wstring menuIndex(unsigned,unsigned) {return {};}
void installWallCues() {}void captureWallCues() {}void cancelWallCues() {}
void publishNavAudio(const NavAudioState&) {}void pauseNavAudio() {}void pauseNavRouteAudio() {}void stopNavPreview() {}
void navAudioArrival() {}void publishPositionalCue(float,float,bool,unsigned) {}
void setFieldAudioAllowed(bool) {}
}
#ifndef FFX_NAVIGATION_REAL_QUEUE_TEST
namespace {
void ownerEvent() {ffx::Event e;e.rva=0x225930;for(const auto& listener:navigationListeners[e.rva]) if(listener.after) listener.after(e);}
void work() {nativeOwner=false;ffx::drainSpeechDomainsForTest();ffx::drainReachabilityForTest();ffx::drainSpeechDomainsForTest();nativeOwner=true;}
void key(unsigned vk,bool shift=false) {nativeOwner=true;navigationKeys.at(vk)(shift);}
void fresh(unsigned events=3) {for(unsigned i=0;i<events;++i) {ownerEvent();work();}}
ffx::Entity selected() {ffx::Entity target;nativeOwner=false;check(ffx::navigationSelectedForTest(target),"owned command selection available");nativeOwner=true;return target;}
}
int main() {try {
    check(ffx::address(0x46c840)!=0x46c840,"message-mode getter must resolve to a fixture callback, never raw game RVA");
    std::setvbuf(stdout,nullptr,_IONBF,0);
    using namespace ffx;check(startAtelResources(),"pipeline resource bootstrap");installAtelResources();installNavigation();
    {
        prepare("dome0700");setScene();put(savedData.data(),uint16_t{320});
        for(unsigned local=57;local<=83;++local) {
            auto* geometry=workers[local].data()+0x284;
            put(geometry+0x28,int16_t{-1});put(geometry+0x30,2.f);put(geometry+0x38,2.f);
        }
        const auto capturePanels=[&] {
            check(nav::captureNavigationScene(sceneCapture,1,true),"captures first-visit panel scene");
            return readFloorPanels(sceneCapture);
        };
        FloorPanels panels;
        // Reproduce the reported age of zero independently of host scheduling.
        const auto freshPanels=[&] {return !panelFactsGap(panels,panels.capturedAt);};
        const auto& script=navigationListeners.at(0x464180).front();
        const auto run=[&](unsigned local,auto body) {
            Event e{};e.epoch=generation();e.args[0]=workerGetter(local);
            script.before(e);body();if(script.after) script.after(e);
        };
        run(38,[&] {panels=capturePanels();});
        check(!freshPanels(),"first Dome capture inside an already running script stays unavailable");
        nativeOwner=false;
        Entity point;point.position=sceneCapture.player;
        check(!nav::submitNavigationIntent(sceneCapture,point,nav::Intent::Start,autoWalkInputRevision(),nav::navigationIntentRevision()),
            "an unverified Dome capture cannot queue a walk as an ordinary point route");
        nativeOwner=true;
        panels=capturePanels();
        check(freshPanels(),"all 27 panels and pattern bindings qualify on the first settled capture");
        for(unsigned local:{38u,69u,69u,75u}) {
            run(local,[&] {++scalar;});panels=capturePanels();
        }
        run(56,[] {});
        static unsigned reports=0;
        static char line[512]{};
        logCaptureHandler.store([](const LogCapture& packet) noexcept {
            if(std::strstr(packet.format.data(),"floor panel opcode bumps")) {
                packet.formatter(packet,line,sizeof(line));++reports;
                check(!nativeOwner,"panel bump reporting stays on the worker");
            }
        });
        const auto loggedSample=[&] {
            const auto text="sample "+std::to_string(floorPanelSample.load())+":";
            return std::strstr(line,text.c_str())!=nullptr;
        };
        work();work();
        check(reports==1 && loggedSample() && std::strstr(line,"W38=1 ") && std::strstr(line,"W69=2 ") &&
            std::strstr(line,"W75=1 ") && std::strstr(line,"W83=0"),"worker log counts panel invalidations");
        Sleep(5000);work();
        check(reports==1,"idle panel bump report stays quiet past the five-second limit");
        panels=capturePanels();work();
        check(reports==2 && loggedSample() && std::strstr(line,"W69=2 "),"a sample change alone emits a panel bump report");
        run(69,[&] {++scalar;});work();
        check(reports==2,"a changed counter still respects the five-second limit");
        Sleep(5000);work();
        check(reports==3 && loggedSample() && std::strstr(line,"W69=3 "),"a counter change alone emits a panel bump report");
        logCaptureHandler.store(nullptr);
        panels=capturePanels();
        const auto burstStart=std::chrono::steady_clock::now();
        for(unsigned frame=0;frame<30;++frame) {
            for(unsigned i=0;i<workers.size();++i) run(i,[] {});
        }
        const auto burstTime=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-burstStart);
        std::printf("METRIC 30 batches of all %u Dome workers, all panels active: %.3f ms total.\n",
            static_cast<unsigned>(workers.size()),burstTime.count());
        check(freshPanels(),"periodic no-op W38 and W57-W83 runs keep the captured route fresh");
        panels=capturePanels();
        run(38,[&] {workers[69][0xab]^=0x20;});
        const auto invalidated=floorPanelRevision.load();
        run(75,[] {});
        check(floorPanelRevision.load()==invalidated,"an idle tick cannot repeatedly invalidate the same observation");
        run(38,[&] {workers[69][0xab]^=0x20;});
        check(!freshPanels(),"group activation followed by reset still invalidates the old route");
        panels=capturePanels();check(freshPanels(),"settled panel recapture permits routing");
        run(69,[&] {put(workers[75].data()+0x284+0x34,.4f);});
        check(!freshPanels(),"another panel's geometry change invalidates the route");
        panels=capturePanels();run(69,[] {});
        check(freshPanels(),"settled geometry survives the next scheduler tick");
        run(38,[&] {++scalar;});
        check(!freshPanels(),"changed pattern bindings invalidate the route");
        panels=capturePanels();check(freshPanels(),"settled pattern qualifies");
        // The retained W44:2 pedestal entry writes stage binding 0x5c000004.
        run(44,[&] {++scalar;});
        check(!freshPanels(),"a pedestal script changing the pattern also retires the panel route");
        panels=capturePanels();
        run(38,[&] {
            ++scalar;run(69,[&] {--scalar;});
        });
        check(!freshPanels(),"a nested script restoring the old pattern cannot revive the route");
        panels=capturePanels();
        run(69,[&] {
            check(!panelMovementFresh(panels.revision,panels.capturedAt,GetTickCount64()),"native movement refuses while a panel script is executing");
            run(75,[] {});
            check(!panelMovementFresh(panels.revision,panels.capturedAt,GetTickCount64()),"returning from a nested script keeps the outer fence");
            panels=capturePanels();
        });
        check(!freshPanels(),"an observation captured inside a script cannot later authorize movement");
        panels=capturePanels();check(freshPanels(),"first complete observation after script return qualifies");
        run(38,[&] {put(workers[83].data()+4,uintptr_t{0});});
        check(!freshPanels(),"lost panel worker ownership invalidates the route");
        panels=capturePanels();check(!freshPanels(),"incomplete recapture keeps the route refused");
        put(workers[83].data()+4,chunk);panels=capturePanels();
        check(freshPanels(),"restored worker identity qualifies on a complete recapture");
        Event unknown{};unknown.epoch=generation();script.before(unknown);script.after(unknown);
        check(!freshPanels(),"unreadable interpreter owner invalidates the capture");
        // The live Dome captures a scene almost every frame, so one lands
        // between the leg publication and the queued start (ffx-s4).
        // A flat two-triangle floor; the panels lie on the diagonal far beyond the goal.
        static std::array<RawNavigationTriangle,2> floor{{{{0,1,2},{-1,-1,1},0},{{0,2,3},{0,-1,-1},0}}};
        static std::array<RawNavigationVertex,4> corners{{{{-200,0,-200,0}},{{200,0,-200,0}},{{200,0,200,0}},{{-200,0,200,0}}}};
        static std::array<uint8_t,12> floorRoot{};put(floorRoot.data()+0xa,int16_t{4});
        triangleCount=2;trianglesPointer=reinterpret_cast<uintptr_t>(floor.data());
        verticesPointer=reinterpret_cast<uintptr_t>(corners.data());rootPointer=reinterpret_cast<uintptr_t>(floorRoot.data());
        panels=capturePanels();check(freshPanels(),"Dome start fixture begins settled");const auto origin=playerPosition();
        const Point goal{origin.x+5,origin.y,origin.z+10};
        auto domeStarts=[&](bool everyFrame) {
            const auto before=starts;
            check(queuePuzzleGuidePoint(goal,-1,false,L"Stand on the activation tile.",UtilityText{}),"admit Dome guide point");
            for(unsigned i=0;i<3;++i) {if(everyFrame) nav::requestNavigationScene();ownerEvent();work();}
            return starts==before+1;
        };
        check(domeStarts(false),"unchanged Dome floor starts a guide route");
        check(domeStarts(true),"a newer capture of the unchanged Dome floor keeps the queued start");
        triangleCount=0;trianglesPointer=verticesPointer=rootPointer=0;
        std::puts("PASS: periodic no-op panel scripts keep facts fresh; state changes and incomplete observations fence movement until settled.");
    }
    // Native encoded title getter: all requested IDs share a controlled ready
    // game-format string; exact getter bounds, copying and decoder are real.
    areaBytes[0]=0x50;areaBytes[1]=0x82;areaBytes[2]=0x74;areaBytes[3]=0x70;areaBytes[4]=0;
    prepare("cdsp0000");setScene();cameraPointer=reinterpret_cast<uintptr_t>(cameraBytes.data());put(cameraBytes.data()+0x40,1.f);
    for(const int mode:{0,1}) {
        messageMode=mode;
        check(nav::captureNavigationScene(sceneCapture,nav::navigationObservationRevision(),false) &&
            sceneCapture.message==(mode!=0) && !sceneCapture.visibleMessage,
            "scene capture calls the owned message-mode adapter without touching fixture code pages");
    }
    messageMode=0;
    entranceCrossing=true;
    const auto beforeRepeat=manualInputRevision;
    key(VK_OEM_5);
    check(!navigationSaid.empty() && navigationSaid.back()==L"Rail pedestal: crossing in progress." &&
        manualInputRevision==beforeRepeat && !navigationPendingForTest(),
        "repeat backslash announces the crossing without canceling or requesting another route");
    entranceCrossing=false;
    put(actorBodies[0].bytes.data()+0x4fc,300.f);
    const auto begin=std::chrono::steady_clock::now();key(VK_OEM_3);fresh();
    check(!navigationPendingForTest() && !navigationSaid.empty() && areaGetterCalls>0,"cold scan resolves names in one batched Native getter handback");
    auto target=selected();const auto cold=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    const auto warmNameCalls=areaGetterCalls;const auto before=navigationSaid.size();for(unsigned i=0;i<5;++i) key('P');fresh(1);
    check(areaGetterCalls==warmNameCalls,"warm copied scenes reuse exact bank/epoch/revision title facts");
    check(!navigationPendingForTest() && navigationSaid.size()==before+5,"five equal distance actions keep five outputs");
    for(size_t i=before+1;i<navigationSaid.size();++i) check(navigationSaid[i]==navigationSaid[before],"equal independent action contents preserved");
    // Select the recovered cdsp controller's actual depth exit and use the
    // production swimming route. This is a copied-script integration fixture.
    bool depth=target.swimDepth;for(unsigned i=0;i<128 && !depth;++i) {key(VK_OEM_6);fresh(1);target=selected();depth=target.swimDepth;}
    check(depth,"retained cdsp depth controller remains selectable");
    key(VK_OEM_5);ownerEvent();work();
    NavLeg leg;check(navLeg(leg) && leg.target.swimDepth,"owned command publishes a complete Native steering snapshot");
    const auto oldStarts=starts;ownerEvent();check(starts==oldStarts+1 && actions>0,"copied route hands approved start back to Native");work();
    key(VK_OEM_5);ownerEvent();work();const auto canceledStarts=starts;++manualInputRevision;ownerEvent();work();
    check(starts==canceledStarts && refusals>0,"manual input cancels delayed start without reactivating movement");
    const auto copiedBefore=navigationSaid.size();key('P');ownerEvent();
    for(auto& body:actorBodies) std::fill(body.bytes.begin(),body.bytes.end(),static_cast<uint8_t>(0xff));
    for(auto& worker:workers) std::fill(worker.begin()+0x558,worker.begin()+0x5a4,static_cast<uint8_t>(0xff));work();
    check(navigationSaid.size()==copiedBefore+1 && !foreignReads,"source overwrite cannot change copied output or trigger Worker engine reads");
    setScene();put(actorBodies[0].bytes.data()+0x4fc,300.f);
    for(unsigned i=0;i<4;++i) {key('P');ownerEvent();}const auto heldBefore=navigationSaid.size();
    key('P');ownerEvent();work();fresh();
    check(!navigationPendingForTest() && navigationSaid.size()>=heldBefore+5,"held observation pressure recovers and retains independent admitted commands");
    key('P');resetNavigation();work();fresh();check(!navigationPendingForTest(),"reset clears retired commands without stale effects");
    check(!foreignReads,"entire owned command, labeling, route and cue pipeline has no Worker engine getter/read");
    prepare("bsyt0100");setScene();check(nav::captureNavigationScene(sceneCapture,nav::navigationObservationRevision(),false),"guide captures actual Besaid Native declarations");
    check(sceneCapture.partCount==1 && sceneCapture.parts[0].guideReady,"every Besaid guide checkpoint is a qualified original declaration");
    for(unsigned row=0;row<sceneCapture.parts[0].bindings.count;++row) sceneCapture.parts[0].bindings.rows[row].value=0;
    auto changeGuideValue=[&](unsigned low,int v) {bool found=false;for(auto& row:sceneCapture.parts[0].bindings.rows) if(row.binding==low) {row.value=v;found=true;}check(found,"copied guide value exists");};
    speechq::Header guideHeader;guideHeader.generation=generation();
    auto guide=[&](OwnedGuideAction action) {nativeOwner=false;const auto text=puzzleGuideOwned(action,sceneCapture,Mesh{},std::vector<Entity>{},guideHeader,manualInputRevision,nav::navigationIntentRevision());nativeOwner=true;return text;};
    resetOwnedPuzzleGuide();check(guide(OwnedGuideAction::Step).find(L"Step 1 of")!=std::wstring::npos,"owned manual guide starts at original first step");
    check(guide(OwnedGuideAction::Previous).find(L"Step 1 of")!=std::wstring::npos,"owned previous clamps at first step");
    changeGuideValue(0x5c0000ec,1);check(guide(OwnedGuideAction::Step).find(L"Step 1 of")!=std::wstring::npos,"state change cannot auto-advance manual selection");
    check(guide(OwnedGuideAction::Expected).find(L"Step 2 of")!=std::wstring::npos,"expected follows actual copied durable checkpoint");
    check(guide(OwnedGuideAction::Step).find(L"Step 2 of")!=std::wstring::npos,"expected moves manual selection to the exact step");
    check(guide(OwnedGuideAction::Next).find(L"Step 3 of")!=std::wstring::npos,"manual next advances one step");guide(OwnedGuideAction::Completed);
    check(guide(OwnedGuideAction::Step).find(L"Step 3 of")!=std::wstring::npos,"completion without a new exact step leaves manual selection unchanged");
    for(unsigned i=0;i<20;++i) guide(OwnedGuideAction::Next);check(guide(OwnedGuideAction::Step).find(L"Step 13 of 13")!=std::wstring::npos,"last-step clamp preserved");
    check(guide(OwnedGuideAction::Step).find(L"Besaid Sphere must be in the pedestal")!=std::wstring::npos,"missing original prerequisite preserved");
    changeGuideValue(0x5c0000cc,1);check(guide(OwnedGuideAction::Step).find(L"push it onto the final floor depression")!=std::wstring::npos,"original final objective preserved");
    changeGuideValue(0x5c0000dc,1);check(guide(OwnedGuideAction::Expected).find(L"Step 7 of")!=std::wstring::npos,"durable checkpoint advances expected through empty carried sphere");
    for(auto& body:actorBodies) std::fill(body.bytes.begin(),body.bytes.end(),static_cast<uint8_t>(0xff));
    const auto equalGuide=guide(OwnedGuideAction::Step);check(equalGuide==guide(OwnedGuideAction::Step) && !foreignReads,"equal guide actions and source overwrite use only copied facts");
    std::printf("PASS: owned real Besaid guide declarations, manual/expected/clamp/durable prerequisite semantics, equal requests and source overwrite; component cases; production command wiring is checked separately below.\n");
    setScene();resetOwnedPuzzleGuide();const auto beforeGuideQueue=navigationSaid.size();
    for(unsigned i=0;i<4;++i) check(nav::submitNavigationQuery(nav::Query::GuideStep,false,ownedPuzzleGuideRevision()),"admit distinct owned guide commands");fresh();
    check(navigationSaid.size()==beforeGuideQueue+4,"four equal guide commands retain four actual pipeline responses");
    for(size_t i=beforeGuideQueue;i<navigationSaid.size();++i) check(navigationSaid[i].find(L"Step 1 of")==0,"queued guide starts at original manual first step");
    check(nav::submitNavigationQuery(nav::Query::GuideNext,false,ownedPuzzleGuideRevision()),"admit guide next");fresh();check(navigationSaid.back().find(L"Step 2 of")==0,"queued guide next changes one manual step");
    // Live 2026-11-25: F8 > Cloister of Trials > Read current puzzle step was
    // silent because the queue drops non-menu speech while the menu is open.
    speechq::fixture::modal=true;const auto beforeMenuStep=navigationSaid.size();
    check(nav::submitNavigationQuery(nav::Query::GuideStep,false,ownedPuzzleGuideRevision()),"admit guide step from the open mod menu");fresh();
    check(navigationSaid.size()==beforeMenuStep+1 && navigationSaid.back().find(L"Step 2 of")==0,"a guide row in the open mod menu answers as the menu");
    speechq::fixture::modal=false;
    const auto completionSpeech=navigationSaid.size();unsigned claimed=0;for(unsigned i=0;i<32;++i) if(claimOwnedGuideRefresh()) {++claimed;check(nav::submitNavigationQuery(nav::Query::GuideCompleted,false,ownedPuzzleGuideRevision()),"admit one environmental completion update");}fresh();
    check(claimed==1 && navigationSaid.size()==completionSpeech,"environmental completion replaces only silent state, without suppressing independent voiced commands");
    check(nav::submitNavigationQuery(nav::Query::GuideStep,false,ownedPuzzleGuideRevision()),"manual repeat after completion");fresh();check(navigationSaid.back().find(L"Step 2 of")==0,"queued completion preserves manual selection");
    check(!foreignReads,"queued guide and native scene pipeline never enters Worker getters");
    std::printf("PASS: owned guide integrated command FIFO, four equal responses, one-step navigation and bounded silent completion refresh; semantic transport endpoints only.\n");
    setScene();const auto origin=playerPosition();Entity fixedGoal;fixedGoal.name=L"Floor puzzle panel";fixedGoal.position={origin.x+5,origin.y,origin.z+10};
    const auto fixedStarts=starts;check(queuePuzzleGuidePoint(fixedGoal.position,fixedGoal.worker,fixedGoal.zone.talk,L"Stand beside the panel and wait for its spoken cue.",UtilityText{}),"admit owned fixed guide target");fresh(3);
    check(starts==fixedStarts+1,"fixed approved point starts only after matching Native scene/controller/pose/route guard");
    check(queuePuzzleGuidePoint(fixedGoal.position,fixedGoal.worker,fixedGoal.zone.talk,L"Stand beside the panel and wait for its spoken cue.",UtilityText{}),"admit delayed fixed target");ownerEvent();work();const auto canceledFixed=starts;++manualInputRevision;ownerEvent();work();check(starts==canceledFixed,"manual input cancels fixed-point start without reactivation");
    check(queuePuzzleGuidePoint(fixedGoal.position,fixedGoal.worker,fixedGoal.zone.talk,L"",UtilityText{}),"admit abandoned target payload");resetNavigation();speechq::fixture::pending.clear();work();
    check(queuePuzzleGuidePoint(fixedGoal.position,fixedGoal.worker,fixedGoal.zone.talk,L"",UtilityText{}),"fresh target payload recovers after canceled marker");fresh();check(!foreignReads,"owned direct guide routing never reads engine/getter on Worker");
    const auto typedBefore=navigationSaid.size();UtilityText intro(L"Step 1 of 11. Reach the panel.");
    check(queuePuzzleGuidePoint(fixedGoal.position,-1,false,L"Stand beside the panel.",intro),"admit fixed typed guide point without Native Entity");intro={};fresh(3);
    check(navigationSaid.size()==typedBefore+1 && navigationSaid.back().find(L"Step 1 of 11. Reach the panel.")==0,"copied direct guide prefix and route form one output after source reset");
    check(!foreignReads,"typed direct guide uses no Worker engine reads");
    // Keep the current EV allocation across Off/On; no prepare/load call here.
    AtelIdentity resumedIdentity;check(readyAtelIdentity(chunk,resumedIdentity),"current guide resource is prepared before toggle");
    const auto resumedSerial=resumedIdentity.span.serial;
    const auto resumeBegin=std::chrono::steady_clock::now();
    modEnabled=false;modLifetimeEpoch=++epoch;resetNavigation();work();
    nativeOwner=false;drainAtelResourcesForTest();nativeOwner=true;
    modEnabled=true;modLifetimeEpoch=++epoch;resetNavigation();work();
    check(readyAtelIdentity(chunk,resumedIdentity) && resumedIdentity.span.serial==resumedSerial,"current-area allocation survives toggle and worker maintenance");
    const auto resumedBefore=navigationSaid.size();key(VK_OEM_6);fresh();
    check(!navigationPendingForTest() && navigationSaid.size()>resumedBefore,"navigation resumes in current area without a resource load");
    const auto resumeMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-resumeBegin).count();
    const auto guideResumeBefore=navigationSaid.size();
    check(nav::submitNavigationQuery(nav::Query::GuideStep,false,ownedPuzzleGuideRevision()),"current-area guide request accepted after toggle");fresh();
    check(navigationSaid.size()==guideResumeBefore+1 && navigationSaid.back().find(L"Step ")==0,"current-area puzzle guide resumes without a resource load");
    // Break only the native controller readiness, keeping the live EV intact.
    put(controller.data()+0x1a,static_cast<uint16_t>(1));
    const std::wstring waitText=L"Navigation data is unavailable. Waiting for the field to become ready.";
    const auto waitsBefore=std::count(navigationSaid.begin(),navigationSaid.end(),waitText);
    key(VK_OEM_6);fresh(1);
    check(std::count(navigationSaid.begin(),navigationSaid.end(),waitText)==waitsBefore+1,"first unavailable command has waiting feedback");
    key(VK_OEM_6);fresh(1);
    check(std::count(navigationSaid.begin(),navigationSaid.end(),waitText)==waitsBefore+2,"second unavailable command is not silenced by a latch");
    check(nav::submitNavigationQuery(nav::Query::GuideExpected,false,ownedPuzzleGuideRevision()),"waiting guide request accepted");fresh(1);
    check(nav::submitNavigationQuery(nav::Query::AssistInstruction),"waiting instruction request accepted");fresh(1);
    check(std::count(navigationSaid.begin(),navigationSaid.end(),waitText)==waitsBefore+4,"guide and instruction commands each speak while waiting");
    const std::wstring readyText=L"Navigation ready.";
    const auto readyBefore=std::count(navigationSaid.begin(),navigationSaid.end(),readyText);
    put(controller.data()+0x1a,static_cast<uint16_t>(0));nav::requestNavigationScene();fresh();
    check(!navigationPendingForTest() && std::count(navigationSaid.begin(),navigationSaid.end(),readyText)==readyBefore+1,"ready field drains requests and announces recovery once");
    nav::requestNavigationScene();fresh();
    check(std::count(navigationSaid.begin(),navigationSaid.end(),readyText)==readyBefore+1,"later ready observations do not repeat recovery");
    put(controller.data()+0x1a,static_cast<uint16_t>(1));key(VK_OEM_6);fresh(1);
    modEnabled=false;modLifetimeEpoch=++epoch;resetNavigation();work();
    prepare("cdsp0000");setScene(); // Native area change and preparation while Off.
    modEnabled=true;modLifetimeEpoch=++epoch;resetNavigation();work();fresh();
    check(std::count(navigationSaid.begin(),navigationSaid.end(),readyText)==readyBefore+1,"toggle cancels old waiting announcement and commands");
    check(readyAtelIdentity(chunk,resumedIdentity) && std::strcmp(resumedIdentity.name,"cdsp0000")==0,"field loaded while Off has its own current allocation on enable");
    key(VK_OEM_6);fresh();check(!navigationPendingForTest(),"field loaded while Off serves navigation without another load");
    check(!foreignReads,"toggle and waiting recovery use no Worker engine reads");
    prepare("bvyt1100");setScene();resetNavigation();resetOwnedPuzzleGuide();scalar=0;
    put(actorBodies[14].bytes.data()+0x14,-36.f); // W13 rail pedestal at the entrance.
    key(VK_OEM_3);fresh();target=selected();
    for(unsigned i=0;i<128 && target.pushControl!=7;++i) {key(VK_OEM_6);fresh(1);target=selected();}
    check(target.pushControl==7 && target.zone.valid && !target.inactive,"Bevelle entrance has a ready staging zone");
    auto* pawn=actorBodies[0].bytes.data();
    put(pawn+0xc,static_cast<float>(target.zone.center.x));put(pawn+0x10,static_cast<float>(target.zone.center.y));
    put(pawn+0x14,static_cast<float>(target.zone.center.z));
    const auto beforeStaging=arms,beforeFacing=faces;
    check(nav::submitNavigationQuery(nav::Query::GuidePath,false,ownedPuzzleGuideRevision()),"guide entrance Path admitted");
    fresh();
    check(arms==beforeStaging && navigationSaid.back().find(L"backslash")!=std::wstring::npos,
        "guide Path at staging explains backslash and leaves the crossing idle");
    const auto beforeStartSpeech=navigationSaid.size();
    key(VK_OEM_5);fresh();
    check(arms==beforeStaging+1 && armedWithWalk && faces==beforeFacing+1 && navigationSaid.size()==beforeStartSpeech,
        "staging backslash delivers ArmAndFace with walk consent and leaves speech to the native crossing");
    const auto afterStaging=arms,afterFacing=faces,afterStarts=starts;
    check(nav::submitNavigationQuery(nav::Query::SilentPath),"silent entrance Path admitted");
    fresh();
    check(arms==afterStaging && faces==afterFacing && starts==afterStarts && navigationSaid.size()==beforeStartSpeech,
        "SilentPath keeps the entrance silent and idle");
    OwnedNavTarget entrance;
    check(captureOwnedNavTarget(target,entrance) && resumeWalk && resumeWalk(entrance.native),"entrance Resume admitted");
    fresh();
    check(arms==afterStaging && faces==afterFacing && starts==afterStarts && navigationSaid.size()==beforeStartSpeech+1 &&
        navigationSaid.back()==L"Auto walk not resumed: Rail pedestal. Press backslash.",
        "Resume speaks the entrance refusal without starting a walk");
    inputDeviceMark.store(1);
    check(resumeWalk(entrance.native),"controller entrance Resume admitted");fresh();inputDeviceMark.store(0);
    check(arms==afterStaging && starts==afterStarts && navigationSaid.size()==beforeStartSpeech+2 &&
        navigationSaid.back()==L"Auto walk not resumed: Rail pedestal. Press Right trigger.",
        "controller Resume names the Read route binding instead of backslash");
    for(unsigned i=0;i<128;++i) check(nav::submitNavigationQuery(nav::Query::Refresh),"admit waiting refresh to fill navigation backlog");
    work();check(navigationPendingForTest()==128,"pending navigation queue is full before environmental completion");
    check(claimOwnedGuideRefresh() && nav::submitNavigationQuery(nav::Query::GuideCompleted,false,ownedPuzzleGuideRevision()),
        "transport accepts a claimed guide refresh before the worker checks backlog");
    work();
    check(navigationPendingForTest()==128 && claimOwnedGuideRefresh(),"dropping a guide refresh from a full backlog releases its claim");
    releaseOwnedGuideRefresh();
    std::puts("PASS: current-area toggle navigation/guide recovery, per-command waiting feedback and one ready announcement.");
    std::printf("METRIC warm current-area toggle through first navigation output %.3fms; controlled owner/worker scheduling.\n",resumeMs);
    std::printf("PASS: owned direct guide target lease, fixed Native start, manual cancellation and abandoned-marker recovery; semantic endpoints only.\n");
    std::printf("PASS: production owned navigation commands, batched names, five equal outputs, depth route, guarded Native start/manual cancellation, source reuse, pressure and reset.\nMETRIC cold copied pipeline %.3fms; semantic domain/mutation adapters, real EV/getter/resource/route code. No game/backend/audible claim.\n",cold);
    return 0;
}catch(const std::exception& e) {observe=false;std::fprintf(stderr,"FAIL: service fixture exception %s\n",e.what());return 1;}}

#endif
