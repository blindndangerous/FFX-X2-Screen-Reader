#include "fixture_runtime.hpp"
#include "../puzzle_guide_owned.hpp"
#include "../puzzle_guide_bindings.hpp"
#include "../puzzle_jump_data.hpp"
#include "../scene_snapshot.hpp"
#include "../utility_trials_data.hpp"
#include "../navigation_intents.hpp"
#include "../navigation_targets.hpp"
#include "../utility_memory.hpp"
#include "../settings.hpp"
#include "../events.hpp"
#include "../dev_pipe.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>

// Synthetic Djose scene (tests/djose_synthetic_scene.inc) through the
// production guide, mesh decoder and scene snapshot writer/loader. It checks
// the gap jump stage and the central lift target, which need a geometry-level
// scene the binding-only route fixtures do not build. With --snapshots it
// instead loads every %FFX_EVIDENCE_ROOT%\scenes\*.ffxscene written by the
// development build's "scene dump <name>" and fails when none exists.
namespace {
using namespace ffx;
AtelIdentity identity;
unsigned checks=0,failures=0;
bool reached=false;
Entity routedTarget;bool routed=false;

void require(bool ok,const char* why) {
    ++checks;
    if(!ok) {++failures;std::fprintf(stderr,"FAIL: %s\n",why);}
}
[[noreturn]] void unexpected(const char* why) {require(false,why);std::exit(1);}
}
namespace ffx {
bool nativeGameThread() {return false;}
// Link-only boundaries of the observation unit; any call fails the fixture.
uintptr_t address(unsigned) {unexpected("native address lookup");}
bool copy(uintptr_t,void*,size_t) {unexpected("native memory read");}
unsigned generation() {return identity.span.epoch;}
bool hook(unsigned,Handler,Handler) {unexpected("native hook registration");}
bool inBattle() {unexpected("native battle read");}
std::atomic<int> scene{0};
Point playerPosition() {unexpected("native player read");}
uintptr_t playerActor() {unexpected("native actor read");}
bool cameraBasis(Point&,Point&) {unexpected("native camera read");}
bool nativeAtelSpan(uintptr_t,AtelSpan&) {unexpected("native ATEL read");}
bool readyAtelWorkerHeader(uintptr_t,unsigned,unsigned&) {unexpected("native worker read");}
AtelResult scanWorker(const AtelScript&,unsigned,const std::vector<unsigned>*) {unexpected("unprepared script scan");}
AtelPreparation atelPreparation(uintptr_t) {unexpected("unprepared ATEL lookup");}
bool OwnedAtelGuardView::callbackAvailable(unsigned,unsigned,const AtelSavedFacts&) const {return false;}
bool visitReadyAtelGuardedScript(const AtelSpan&,OwnedAtelGuardConsumer,void*) {return false;}
void prepareCopiedControlSnapshot(CopiedControls&) {unexpected("native input capture");}
bool captureUtilityValue(const UtilityScript&,unsigned,unsigned,unsigned,int&) noexcept {unexpected("native utility capture");}
PedestalNavigationState capturePedestalNavigationState() {unexpected("native pedestal capture");}
unsigned pedestalConfirmMask() {unexpected("native Confirm lookup");}
std::wstring navigationEntityName(const NavigationTarget*,unsigned,bool,const std::function<std::wstring(int)>&) {unexpected("unprepared entity scan");}
unsigned navigationEntityCategory(const NavigationTarget*,unsigned,bool) {unexpected("unprepared category scan");}
bool navigationSaveSphereModel(unsigned) {unexpected("unprepared model scan");}
bool navigationActorPresence(const NavigationScript*,unsigned,bool) {return false;}
const NavigationScript* navigationCatalog(const AtelScript&) {static NavigationScript catalog{};return &catalog;}
const NavigationTarget* navigationTarget(const NavigationScript*,unsigned) {return nullptr;}
const wchar_t* navigationChestName(const NavigationTarget*) {unexpected("unprepared chest scan");}
std::wstring navigationExitName(const AtelScript&,unsigned,int,const std::wstring&,const std::function<std::wstring(int)>&) {unexpected("unprepared exit scan");}
bool butterflyNavigationLeaving(const AtelSpan&) {unexpected("unprepared butterfly scan");}
bool modMenuActive() {unexpected("native menu read");}
bool gameFocused() {unexpected("live focus read");}
bool utilityConsoleActive() {unexpected("native console read");}
void refreshSwimmingGoal(Entity&) {unexpected("live swimming read");}
void preparePedestalZone(Entity&,uintptr_t) {unexpected("live pedestal read");}
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& out) {out=identity;return chunk==identity.span.chunk;}
bool visitReadyAtelScript(const AtelSpan&,OwnedAtelConsumer,void*) {return false;}
OwnedTextContext copiedControlContext(const CopiedControls&) {
    OwnedTextContext out;out.glyphs[0x31]=L"C";out.glyphs[0x32]=L"X";return out;
}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring out;for(const auto& part:parts) {if(!out.empty()) out+=separator;out+=part;}return out;
}
std::wstring direction(Point) {return L"forward";}
void resetNavAudio() {}
void routeNavAudio(const std::vector<Point>&,const Entity&,bool) {}
// Auto walk off: the guide describes the route and submits no intent.
int setting(Setting) {return 0;}
// Records the target. A real route search is out of scope: "reached" returns
// a zero-length route so the guide speaks its arrival text.
std::vector<Point> navigationRouteOwned(const Mesh&,Point player,const Entity& target,const std::vector<Entity>&) {
    routed=true;routedTarget=target;
    return reached?std::vector<Point>{player,player}:std::vector<Point>{};
}
// Development build only: the guide posts a "step" event to the dev pipe.
std::string lastStepEvent;
void devPipeEvent(const char* channel,const char* text) noexcept {if(!std::strcmp(channel,"step")) lastStepEvent=text;}
namespace nav {
bool submitNavigationIntent(const Scene&,const Entity&,Intent,unsigned,unsigned,bool) {unexpected("navigation intent with Auto walk off");}
}
}

#include "djose_synthetic_scene.inc"

namespace {
bool contains(const std::wstring& text,const wchar_t* part) {return text.find(part)!=std::wstring::npos;}
void show(const std::wstring& text) {std::fwprintf(stderr,L"  guide: %ls\n",text.c_str());}
std::wstring guide(const nav::Scene& shot,const std::vector<Entity>& entities,OwnedGuideAction action) {
    Mesh mesh=nav::decodeNavigationSceneMesh(shot);
    try {return puzzleGuideOwned(action,shot,mesh,entities,speechq::Header{},0,0,0,shot.primary,1);}
    catch(const std::exception& error) {std::string what(error.what());return L"exception: "+std::wstring(what.begin(),what.end());}
}
unsigned spokenStep(const std::wstring& text) {
    const auto at=text.find(L"Step ");return at==std::wstring::npos?0:static_cast<unsigned>(std::wcstoul(text.c_str()+at+5,nullptr,10));
}
// Select guide step n the way "puzzle jump <from>" does: its trial writes,
// then the Seek query (the expected step of an exact state must agree), then
// Next up to n.
void selectStep(DjoseSyntheticScene& djose,unsigned from,unsigned n) {
    resetOwnedPuzzleGuide();djose.applyJump(from);
    const auto expected=guide(*djose.scene,djose.entities,OwnedGuideAction::Expected);
    Mesh mesh=nav::decodeNavigationSceneMesh(*djose.scene);
    const auto seek=puzzleGuideOwned(OwnedGuideAction::Seek,*djose.scene,mesh,djose.entities,speechq::Header{},0,0,from,djose.scene->primary,1);
    if(spokenStep(seek)!=from) show(seek);
    require(spokenStep(seek)==from,"Seek selects the jump start step");
    require(lastStepEvent.find("script=djyt0600 step="+std::to_string(from)+" ")==0,"the guide posts a structured step event");
    require(!spokenStep(expected) || spokenStep(expected)==from,"an exact synthetic trial state agrees with the jump start");
    for(unsigned step=from;step<n;++step) guide(*djose.scene,djose.entities,OwnedGuideAction::Next);
}
std::wstring path(DjoseSyntheticScene& djose,bool arrive) {
    routed=false;reached=arrive;const auto text=guide(*djose.scene,djose.entities,OwnedGuideAction::Path);reached=false;return text;
}

void meshChecks(const nav::Scene& shot) {
    const auto mesh=nav::decodeNavigationSceneMesh(shot);
    require(mesh.triangles.size()==DjoseSyntheticScene::TriangleCount,"synthetic mesh decodes every triangle");
    unsigned gap=0;double low=1e9,high=-1e9;
    for(const auto& t:mesh.triangles) {
        if(t.collisionGroup==50) ++gap;
        for(const auto& p:t.points) {low=std::min(low,p.y);high=std::max(high,p.y);}
    }
    require(gap==2,"gap floor keeps collision group 50");
    require(std::abs(high+18)<.01 && std::abs(low+67)<.01,"floors decode at y -18 and -67");
}

void gapJump(DjoseSyntheticScene& djose) {
    // Step 18 (gap switch, across the gap) with the pedestal in the gap (0xd9=1).
    selectStep(djose,17,18);djose.set(0x0c0000d9,1);
    djose.player({0,-18,60});
    auto text=path(djose,false);
    if(spokenStep(text)!=18) show(text);
    require(spokenStep(text)==18,"Next reaches guide step 18");
    require(routed && routedTarget.name==L"Gap jump point","south of the gap the route goes to the jump point");
    require(std::abs(routedTarget.position.z-74.6)<.01 && std::abs(routedTarget.position.y+18)<.01,"south jump stage is (0,-18,74.6)");
    text=path(djose,true);
    if(!contains(text,L"Gap jump point: reached.") || !contains(text,L"The game jumps you across.")) show(text);
    require(contains(text,L"Gap jump point: reached.") && contains(text,L"The game jumps you across."),"at the stage the guide tells the jump");
    djose.player({0,-18,170});path(djose,false);
    require(routed && routedTarget.name!=L"Gap jump point" && routedTarget.position.z>170,"north of the gap the route goes to the gap switch");
    djose.player({0,-65,60});path(djose,false);
    require(routed && routedTarget.name!=L"Gap jump point","the upper floor never routes to the jump point");
    djose.set(0x0c0000d9,0);djose.player({0,-18,60});path(djose,false);
    require(routed && routedTarget.name!=L"Gap jump point","without the pedestal in the gap there is no jump");
}

void centralLift(DjoseSyntheticScene& djose) {
    selectStep(djose,28,28);djose.player({0,-18,20});
    auto text=path(djose,false);
    require(spokenStep(text)==28,"jump start 28 is guide step 28 (central lift)");
    if(!routed) show(text);
    require(routed && std::abs(routedTarget.position.x)<.01 && std::abs(routedTarget.position.y+18)<.01 &&
        std::abs(routedTarget.position.z-56.6)<.01,"lift target is the north edge of pedestal W1");
    text=path(djose,true);
    if(!contains(text,L"reached. Stand still on the pedestal. It rises to the upper floor.")) show(text);
    require(contains(text,L"reached. Stand still on the pedestal. It rises to the upper floor."),"on the lift the guide says to stand still");
    djose.set(0x0c0000db,0);text=path(djose,false);
    require(!routed && contains(text,L"not positioned yet"),"without the lift circuit there is no lift target");
}

void snapshotChecks(DjoseSyntheticScene& djose) {
    const auto file=(std::filesystem::current_path()/L"djose_synthetic.ffxscene").wstring();
    require(writeSceneSnapshot(file,*djose.scene,"djyt0600",identity.checksum),"synthetic scene snapshot writes");
    auto loaded=std::make_unique<nav::Scene>();SceneSnapshotHeader header{};const char* why="";
    require(loadSceneSnapshot(file,*loaded,&header,&why),why);
    require(!std::memcmp(loaded.get(),djose.scene.get(),sizeof(nav::Scene)),"loaded scene equals the written scene");
    require(!std::strcmp(header.script,"djyt0600") && header.room==djose.scene->room && header.scriptChecksum==identity.checksum,"header names script, room and checksum");
    // The loaded scene drives the guide like the original.
    std::swap(djose.scene,loaded);gapJump(djose);std::swap(djose.scene,loaded);
    const auto bytes=std::filesystem::file_size(file);
    const auto rewrite=[&](std::size_t offset,char byte) {
        std::FILE* f=nullptr;_wfopen_s(&f,file.c_str(),L"r+b");std::fseek(f,static_cast<long>(offset),SEEK_SET);std::fputc(byte,f);std::fclose(f);
    };
    rewrite(sizeof(SceneSnapshotHeader)+100,0x5a);
    require(!loadSceneSnapshot(file,*loaded,nullptr,&why) && !std::strcmp(why,"snapshot body checksum differs"),"a changed body is rejected");
    writeSceneSnapshot(file,*djose.scene,"djyt0600",0);rewrite(offsetof(SceneSnapshotHeader,layout),0x01);
    require(!loadSceneSnapshot(file,*loaded,nullptr,&why) && !std::strcmp(why,"snapshot written by a build with another Scene layout"),"another layout is rejected");
    writeSceneSnapshot(file,*djose.scene,"djyt0600",0);std::filesystem::resize_file(file,bytes-1);
    require(!loadSceneSnapshot(file,*loaded,nullptr,&why) && !std::strcmp(why,"snapshot body is truncated"),"a truncated file is rejected");
    std::filesystem::remove(file);
    require(!loadSceneSnapshot(file,*loaded,nullptr,&why) && !std::strcmp(why,"cannot open the snapshot"),"a missing file is rejected");
}

// Real dumps: opt-in, outside Git. Each must load; the guide runs on it.
int snapshots() {
    wchar_t root[1024];auto length=GetEnvironmentVariableW(L"FFX_EVIDENCE_ROOT",root,1024);
    if(!length || length>=1024) {std::fprintf(stderr,"FAIL: FFX_EVIDENCE_ROOT is not set\n");return 1;}
    const auto folder=std::filesystem::path(root)/L"scenes";unsigned files=0;std::error_code error;
    for(const auto& entry:std::filesystem::directory_iterator(folder,error)) {
        if(entry.path().extension()!=L".ffxscene") continue;++files;
        auto shot=std::make_unique<nav::Scene>();SceneSnapshotHeader header{};const char* why="";
        const bool ok=loadSceneSnapshot(entry.path().wstring(),*shot,&header,&why);
        std::printf("%ls: %s",entry.path().filename().c_str(),ok?"loaded":why);
        require(ok,"scene snapshot loads");if(!ok) {std::printf("\n");continue;}
        const auto mesh=nav::decodeNavigationSceneMesh(*shot);
        std::printf(" room=%u script=%s triangles=%zu workers=%u",header.room,header.script,mesh.triangles.size(),shot->count);
        identity={};
        for(const auto& trial:trialUtilities) if(!std::strcmp(trial.script,header.script) && trial.checksum==header.scriptChecksum && shot->partCount) {
            identity.span=shot->parts[0].span;identity.length=trial.length;identity.checksum=trial.checksum;std::memcpy(identity.name,trial.script,8);
        }
        if(identity.name[0]) {
            resetOwnedPuzzleGuide();const auto text=guide(*shot,{},OwnedGuideAction::Expected);
            std::printf(" guide=\"%ls\"",text.substr(0,120).c_str());
        }
        std::printf("\n");
    }
    if(!files) {std::fprintf(stderr,"FAIL: no .ffxscene file in %ls (write one with the dev pipe: scene dump <name>)\n",folder.c_str());return 1;}
    std::printf("%s: %u scene snapshots, %u checks.\n",failures?"FAIL":"PASS",files,checks);
    return failures?1:0;
}
}

int main(int argc,char** argv) {
    ffx_fixture::initializeRuntime();
    if(argc>1 && !std::strcmp(argv[1],"--snapshots")) return snapshots();
    DjoseSyntheticScene djose;
    meshChecks(*djose.scene);gapJump(djose);centralLift(djose);snapshotChecks(djose);
    std::printf("%s: synthetic Djose scene: gap jump, central lift, snapshot round trip, %u checks.\n",failures?"FAIL":"PASS",checks);
    return failures?1:0;
}
