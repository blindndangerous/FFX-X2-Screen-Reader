// Development puzzle jump: `puzzle jump <n>` writes the trial variables the
// original actions write before step n, then selects step n in the guide.
// The tables are shared with the route fixture (puzzle_jump_data.hpp).
#ifdef FFX_DEV_UTILITIES
#include "puzzle_jump.hpp"
#include "puzzle_jump_data.hpp"
#include "puzzle_guide_owned.hpp"
#include "navigation_observations.hpp"
#include "utility_trials.hpp"
#include "utility_memory.hpp"
#include "events.hpp"
#include "runtime.hpp"
#include <cwctype>
namespace ffx {
UtilityText puzzleJumpCommand(std::wstring_view arguments) {
    if(!nativeGameThread()) return L"This utility requires the native game owner.";
    unsigned step=0;bool digits=!arguments.empty() && arguments.size()<4;
    for(const auto c:arguments) {if(!std::iswdigit(c)) {digits=false;break;}step=step*10+static_cast<unsigned>(c-L'0');}
    if(!digits) return L"Use puzzle jump followed by a step number.";
    if(inBattle()) return L"Puzzle jump is available in the field.";
    if(utilityBusy()) return L"Puzzle jump waits for the current utility to finish.";
    auto script=utilityScript();
    script.worker=utilityWorker(script,0);script.header=read<uintptr_t>(script.worker);
    const auto trial=currentTrial(script);
    if(!trial || script.name!="djyt0600") return L"Puzzle jump supports the Djose trial only.";
    if(const auto refusal=djoseJumpRefusal(step)) return refusal;
    for(const auto& write:puzzleJumpWrites(djoseJumpStages,step)) {
        int value=write.value;
        // Treasure flags are save bits shared by every trial: only add this one.
        if(write.low==0x01000a70 && write.high==1) value|=utilityValue(script,write.low,write.high,write.element);
        utilityWrite(script,{write.low,write.high,value,write.element});
    }
    if(!nav::submitNavigationQuery(nav::Query::GuideSeek,false,ownedPuzzleGuideRevision(),step))
        return UtilityText(L"Wrote the variables for step ").integer(step).literal(L". The guide did not accept the step; use puzzle expected.");
    UtilityText result(L"Jumped to step ");result.integer(step).literal(L".");
    if(step==29) result.literal(L" Ride the lift up before you continue.");
    return result;
}
}
#endif
