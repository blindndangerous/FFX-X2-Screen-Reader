#pragma once
#ifdef FFX_DEV_UTILITIES
#include "puzzle_guide_progress.hpp"
#include "utility_text.hpp"
#include <array>
namespace ffx {
struct KilikaJumpState {std::array<int,13> values{};};
// klyt1000 location 6, byte declarations 58..64, high=1, element=0.
// Values come from the original socket transfers and the existing guide replay.
inline bool kilikaJumpState(unsigned step,KilikaJumpState& state) {
    if(step<1 || step>15) return false;
    state.values={1,0,4,0,0,0,0,2,6,3,0,0,0};
    auto& v=state.values;
    for(unsigned completed=1;completed<step;++completed) switch(completed) {
    case 1: v[0]=0;v[3]=1;break;
    case 2: v[1]=1;v[3]=0;break;
    case 3: v[1]=0;v[3]=1;v[11]=1;break;
    case 4: v[5]=1;v[3]=0;break;
    case 5: v[5]=5;v[3]=1;break;
    case 6: v[6]=1;v[3]=0;break;
    case 7: v[5]=0;break;
    case 8: v[2]=0;v[3]=4;break;
    case 9: v[4]=4;v[3]=0;break;
    case 10: v[7]=0;v[3]=2;break;
    case 11: v[2]=2;v[3]=0;break;
    case 12: v[4]=0;v[3]=4;break;
    case 13: v[7]=4;v[3]=0;break;
    }
    return true;
}
inline void advanceKilikaJump(const PuzzleRoute& route,const KilikaJumpState& state,bool reset,PuzzleProgress& progress) {
    const auto value=[&](const UtilityBinding& b) {return b.low>=0x0c000058 && b.low<=0x0c000064?state.values[b.low-0x0c000058]:0;};
    advancePuzzleProgress(route,0,static_cast<unsigned>(route.steps.size()),progress,
        [&](const PuzzleStep& s) {if(!s.check) return false;if(!s.done.low) return s.worker==41 && reset;
            return s.done.low!=0x01000a70 && value(s.done)==s.done.value && (!s.also.low || value(s.also)==s.also.value);},
        [&](const PuzzleStep& s) {return s.done.low && value(s.done)==0;});
}
// The guide is history-sensitive: e.g. an empty entry door alone cannot prove
// whether its sphere was ever inserted. Replay the known prefix through the real
// detector; never overwrite expected with the requested number.
inline PuzzleProgress kilikaJumpHistory(const PuzzleRoute& route,unsigned step) {
    PuzzleProgress progress;
    for(unsigned current=1;current<step;++current) {KilikaJumpState state;if(!kilikaJumpState(current,state)) break;
        advanceKilikaJump(route,state,false,progress);}
    return progress;
}
inline bool kilikaJumpPose(unsigned step,Point p) {
    if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return false;
    const Point wanted=step==15?Point{.2215200067,-9.990234375,144.1042022705}:Point{-20.0962619781,-9.990234375,-30.4666366577};
    return step>=1 && step<=15 && std::abs(p.x-wanted.x)<.1 && std::abs(p.y-wanted.y)<.1 && std::abs(p.z-wanted.z)<.1;
}
// Native event tails change map collision/graphics as well as these bindings.
// Until those calls are qualified, skip sphere transfers only within the same
// environmental phase. Never claim that changing a byte removed a wall.
inline bool kilikaJumpEnvironment(const KilikaJumpState& before,const KilikaJumpState& wanted) {
    const auto phase=[](const KilikaJumpState& state) {
        const auto& v=state.values;
        if(v[11]<0 || v[11]>1 || v[12]!=0) return -1;
        const int fire=v[7]==2?0:v[7]==0?1:v[7]==4?2:-1;
        if(fire<0) return -1;
        const bool chamber=v[6]==1 && v[5]==0;
        return v[11]*16+(v[1]!=0?8:0)+(chamber?4:0)+fire;
    };
    return phase(before)>=0 && phase(before)==phase(wanted);
}
UtilityText trialJumpCommand(std::wstring_view);
}
#endif
