// LOCAL TEST TOOL ONLY. Never include this command in an upstream release.
#include "utility_trial_jump.hpp"
#ifdef FFX_DEV_UTILITIES
#include "utilities.hpp"
#include "puzzle_guide_data.hpp"
#include "puzzle_guide_owned.hpp"
#include "pedestal.hpp"
#include "events.hpp"
#include "navigation_observations.hpp"
namespace ffx {
UtilityText trialJumpCommand(std::wstring_view arguments) {
    if(arguments.empty()) return L"Trial jumps: Kilika, 22 steps. Supported: 1 through 14 with the pedestal at its original spot; 15 only at its native reset spot. Door, fire chamber and flame phases must already match. Steps 16 through 22 are unsupported. Use trial kilika followed by a step. Test only, do not save.";
    constexpr std::wstring_view prefix=L"kilika ";
    if(arguments.substr(0,prefix.size())!=prefix) return L"Use trial kilika followed by a step from 1 through 15. Test only, do not save.";
    auto number=arguments.substr(prefix.size());
    while(!number.empty() && number.front()==L' ') number.remove_prefix(1);
    unsigned step=0;
    if(number.empty() || number.size()>2) return L"Use trial kilika followed by a step from 1 through 15.";
    for(auto c:number) {if(c<L'0' || c>L'9') return L"The trial step must be a whole number.";step=step*10+static_cast<unsigned>(c-L'0');}
    KilikaJumpState wanted;
    if(!kilikaJumpState(step,wanted)) return L"Unsupported Kilika step. Use 1 through 14 at the original pedestal spot, or 15 at the native reset spot. Steps 16 through 22 require unverified native plunge, collision and reveal changes.";
    if(!nativeGameThread()) return L"This utility requires the native game owner.";
    if(!utilityActionAllowed() || utilityBusy()) return L"Trial jump refused: finish the current script, battle or dialogue and return to field control.";
    UtilityScript script;
    if(!captureUtilityScript(script) || script.name!="klyt1000" || !captureCurrentTrial(script) ||
        read<uint16_t>(address(0xd2ca90))!=108 || !captureUtilityWorker(script,0,script.worker))
        return L"Trial jump refused: enter the Kilika Cloister of Trials room first.";
    // Same native control bit as captureNavigationScene, including script pushes
    // whose dialogue mode is already zero while the player remains disabled.
    if(!(read<uint8_t>(script.controller)&2))
        return L"Trial jump refused: a native script still owns player control. Wait for it to finish.";
    script.header=read<uintptr_t>(script.worker);
    NativeRouteTarget target;target.pushControl=2;target.pushLocal=6;
    PedestalSnapshot pedestal;
    if(!capturePedestalSnapshot(script,target,pedestal) || !kilikaJumpPose(step,pedestal.position))
        return L"Trial jump refused: pedestal must already be at its original spot for steps 1 through 14, or its settled native reset spot for step 15. Pedestal teleport is not verified safe.";
    KilikaJumpState before;
    for(unsigned i=0;i<before.values.size();++i) if(!captureUtilityValue(script,0x0c000058+i,1,0,before.values[i]))
        return L"Trial jump refused: required Kilika variables are unavailable. Nothing changed.";
    if(!kilikaJumpEnvironment(before,wanted))
        return L"Trial jump refused: the native door, fire chamber or flame phase differs. Complete that native transition first, then jump within its phase. Nothing changed. Test only, do not save.";
    // Validate the whole payload and native witness before the first setter.
    const auto& route=puzzleRoutes[1];auto progress=kilikaJumpHistory(route,step);
    advanceKilikaJump(route,wanted,step==15,progress);
    if(progress.expected+1!=step) return L"Trial jump refused: inverse state failed the guide detector. Nothing changed.";
    try {
        for(unsigned i=0;i<wanted.values.size();++i) utilityWrite(script,{0x0c000058+i,1,wanted.values[i],0});
        for(unsigned i=0;i<wanted.values.size();++i) if(utilityValue(script,0x0c000058+i,1)!=wanted.values[i])
            throw std::runtime_error("Kilika variable readback failed");
        prepareOwnedKilikaJump(script,step);
        if(!nav::submitNavigationQuery(nav::Query::GuideExpected,false,ownedPuzzleGuideRevision()))
            throw std::runtime_error("The guide request is unavailable");
    } catch(...) {
        bool restored=true;
        for(unsigned i=0;i<before.values.size();++i) try {utilityWrite(script,{0x0c000058+i,1,before.values[i],0});} catch(...) {restored=false;}
        for(unsigned i=0;i<before.values.size();++i) try {if(utilityValue(script,0x0c000058+i,1)!=before.values[i]) restored=false;} catch(...) {restored=false;}
        resetOwnedPuzzleGuide();
        return restored?L"Trial jump failed. Previous variables restored. Test only, do not save; reload your original save.":
            L"Trial jump failed and restoration was incomplete. Test only, do not save. Reload your original save now.";
    }
    return UtilityText(L"Trial set to step ")+utilityInteger(step)+L". Test only, do not save. Reload your original save after testing.";
}
}
#endif
