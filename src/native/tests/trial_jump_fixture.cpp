// Synthetic native boundaries only; never loads or touches a game process.
#include "../utility_trial_jump.hpp"
#include "../puzzle_guide_data.hpp"
#include "../pedestal.hpp"
#include "../events.hpp"
#include "../navigation_observations.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace ffx;
namespace {
unsigned checks=0;
void check(bool ok,const char* message) {++checks;if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
bool owner=true,allowed=true,busy=false,verified=true,available=true,missing=false;
unsigned writes=0,announced=0,seeded=0;
int failWrite=-1,failRead=-1;
bool persistentWriteFailure=false;
bool guideAvailable=true;
uint16_t room=108;uintptr_t workerHeader=1;
uint8_t control=2;
Point pedestalPosition{-20.0962619781,-9.990234375,-30.4666366577};
KilikaJumpState memory;
}
namespace ffx {
bool nativeGameThread() {return owner;}
bool utilityActionAllowed(bool) {return allowed;}
bool utilityBusy() {return busy;}
uintptr_t address(unsigned rva) {check(rva==0xd2ca90,"only room global read");return reinterpret_cast<uintptr_t>(&room);}
bool copy(uintptr_t source,void* out,size_t length) {std::memcpy(out,reinterpret_cast<const void*>(source),length);return true;}
bool captureUtilityScript(UtilityScript& s) noexcept {s.name="klyt1000";s.chunk=123;s.allocationSerial=7;s.controller=reinterpret_cast<uintptr_t>(&control);return available;}
const TrialUtility* captureCurrentTrial(const UtilityScript&) noexcept {static TrialUtility t{};return verified?&t:nullptr;}
bool captureUtilityWorker(const UtilityScript&,unsigned,uintptr_t& out) noexcept {out=reinterpret_cast<uintptr_t>(&workerHeader);return available;}
bool capturePedestalSnapshot(const UtilityScript&,const NativeRouteTarget&,PedestalSnapshot& s) noexcept {s.position=pedestalPosition;return available;}
bool captureUtilityValue(const UtilityScript&,unsigned low,unsigned,unsigned,int& value) noexcept {
    if(missing && low==0x0c00005c) return false;value=memory.values.at(low-0x0c000058);return true;
}
int utilityValue(const UtilityScript&,unsigned low,unsigned,unsigned) {auto i=low-0x0c000058;
    if(failRead==static_cast<int>(i)) {failRead=-1;return memory.values.at(i)+1;}return memory.values.at(i);}
void utilityWrite(const UtilityScript&,const UtilityBinding& b) {
    ++writes;const auto i=b.low-0x0c000058;
    if(failWrite==static_cast<int>(i)) {if(!persistentWriteFailure) failWrite=-1;throw std::runtime_error("synthetic setter failure");}
    memory.values.at(i)=b.value;
}
void prepareOwnedKilikaJump(const UtilityScript&,unsigned step) {seeded=step;}
unsigned ownedPuzzleGuideRevision() {return 1;}
void resetOwnedPuzzleGuide() {seeded=0;}
}
namespace ffx::nav {
bool submitNavigationQuery(Query query,bool,unsigned,unsigned,uintptr_t,unsigned) {
    check(query==Query::GuideExpected,"jump asks actual guide for detected step");++announced;return guideAvailable;
}
}
int main() {
    const auto& route=puzzleRoutes[1];
    for(unsigned step=1;step<=15;++step) {
        KilikaJumpState state;
        check(kilikaJumpState(step,state),"supported Kilika inverse exists");
        auto progress=kilikaJumpHistory(route,step);
        advanceKilikaJump(route,state,step==15,progress);
        if(progress.expected+1!=step) std::fprintf(stderr,"step %u detected %u\n",step,progress.expected+1);
        check(progress.expected+1==step,"written start state detects requested step");
        check(state.values[11]==(step>=4?1:0),"entry door flame progress is retained");
        check(state.values[12]==0,"inverse never marks the depression complete");
        unsigned spheres[7]{};
        for(unsigned i=0;i<11;++i) if(state.values[i]>0 && state.values[i]<=6) ++spheres[state.values[i]];
        check(spheres[1]==1 && spheres[2]==1 && spheres[3]==1 && spheres[4]==1 && spheres[6]==1,
            "inverse conserves each distinct sphere");
    }
    KilikaJumpState state;
    for(unsigned step:{0u,16u,22u,23u,999u}) check(!kilikaJumpState(step,state),"unsupported inverse is refused");
    check(kilikaJumpPose(1,{-20.0962619781,-9.990234375,-30.4666366577}),"original pedestal pose accepted");
    check(!kilikaJumpPose(1,{.2215200067,-9.990234375,144.1042022705}),"early step cannot rewind reset pedestal");
    check(kilikaJumpPose(15,{.2215200067,-9.990234375,144.1042022705}),"native reset pose accepted");
    check(!kilikaJumpPose(15,{50.0109901428,-9.990234375,170}),"moving reset corridor is not a settled reset pose");
    check(!kilikaJumpPose(15,{.2215200067,20,144.1042022705}),"wrong floor refused");
    check(trialJumpCommand(L"").find(L"22 steps")!=std::wstring::npos && !writes,"listing has counts and no writes");
    for(unsigned step=1;step<=15;++step) {
        pedestalPosition=step==15?Point{.2215200067,-9.990234375,144.1042022705}:Point{-20.0962619781,-9.990234375,-30.4666366577};
        KilikaJumpState wanted;kilikaJumpState(step,wanted);memory=wanted;
        for(unsigned i:{0u,2u,3u,4u,8u,9u,10u}) memory.values[i]=99;
        writes=announced=seeded=0;
        const auto result=trialJumpCommand(L"kilika "+std::to_wstring(step));
        check(result.find(L"Trial set to step")!=std::wstring::npos && result.find(L"Test only, do not save")!=std::wstring::npos,"success and save warning spoken");
        check(writes==13 && announced==1 && seeded==step,"native writer updates all bindings then requests guide");
        check(memory.values==wanted.values,"writer replaces prior sphere contents with exact inverse state");
        auto progress=kilikaJumpHistory(route,step);advanceKilikaJump(route,memory,step==15,progress);
        check(progress.expected+1==step,"actual writer output passes guide detector");
    }
    pedestalPosition={-20.0962619781,-9.990234375,-30.4666366577};
    kilikaJumpState(8,memory);check(trialJumpCommand(L"kilika 10").find(L"Trial set")!=std::wstring::npos,"skip sphere transfers within the open-chamber phase");
    writes=0;check(trialJumpCommand(L"kilika 14").find(L"phase differs")!=std::wstring::npos && !writes,"refuse crossing native wall and flame transitions");
    for(auto input:{L"kilika 0",L"kilika 16",L"kilika 22",L"kilika -1",L"kilika 1 extra",L"kilika",L"besaid 1"}) {
        writes=0;trialJumpCommand(input);check(!writes,"invalid and unsupported requests never mutate");
    }
    for(unsigned refusal=0;refusal<8;++refusal) {
        writes=0;owner=refusal!=0;allowed=refusal!=1;busy=refusal==2;verified=refusal!=3;available=refusal!=4;room=refusal==5?122:108;missing=refusal==6;
        control=refusal==7?0:2;
        const auto result=trialJumpCommand(L"kilika 4");check(!writes && result.find(L"Trial set")==std::wstring::npos,"owner, field, busy, identity, room, missing-binding and native-control guards");
    }
    owner=allowed=verified=available=true;busy=missing=false;room=108;control=2;
    kilikaJumpState(11,memory);const auto original=memory.values;
    failWrite=5;check(trialJumpCommand(L"kilika 13").find(L"restored")!=std::wstring::npos && memory.values==original,"partial write failure restores previous state");
    failRead=7;check(trialJumpCommand(L"kilika 13").find(L"restored")!=std::wstring::npos && memory.values==original,"readback failure restores previous state");
    guideAvailable=false;
    check(trialJumpCommand(L"kilika 13").find(L"restored")!=std::wstring::npos && memory.values==original && seeded==0,"rejected guide request restores state and invalidates seed");
    guideAvailable=true;
    failWrite=5;persistentWriteFailure=true;
    check(trialJumpCommand(L"kilika 13").find(L"restoration was incomplete")!=std::wstring::npos,"failed rollback demands an immediate reload");
    std::printf("PASS: Kilika trial jump: %u checks, all 15 supported start states and pedestal restrictions.\n",checks);
}
