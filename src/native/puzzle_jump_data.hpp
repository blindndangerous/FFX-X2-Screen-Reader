#pragma once
// Development puzzle jump tables. The route fixture replays these same stages
// (tests/puzzle_guide_route_fixture.inc), so the jump and the guide checks
// share one source of truth.
#include <vector>
namespace ffx {
struct PuzzleJumpWrite {unsigned low;int value;unsigned element=0;unsigned high=1;};
// action names the original trial action. expected is the step the guide
// reads after the stage (0: it cannot identify one). next is the route step
// that follows the stage when expected is 0.
struct PuzzleJumpStage {const char* action;std::vector<PuzzleJumpWrite> writes;int expected;unsigned next=0;};
inline unsigned puzzleJumpNext(const PuzzleJumpStage& stage) {return stage.next?stage.next:static_cast<unsigned>(stage.expected);}
// djyt0600: 0x94 carried sphere (1 Djose, 3 charged, 10 Destruction);
// 0x95/0x96 entry, 0x97/0x98 door, 0x99/0x9a charging, 0x9b charged
// door (worker 54) and 0x9c left circuit (worker 55) recess sockets;
// 0x9d/0x9e pedestal sockets (east face toward the charging wall, west
// face toward the reset tile); 0x9f Destruction Sphere recess (worker 62);
// 0x28e door opened; 0xa1 charger (1 powered, 2 sticky once the pedestal
// sphere is charged); 0xd9 pedestal in gap, cleared by reset tile 67 only
// while 0x9b is not 3; 0xda gap switch; 0xdb lift circuit; 0xe5 hidden
// glyph (1 revealed, 2 touched); 0xde-0xe1 upper pedestals; 0xe3 all five
// pushed (1) then sphere set (2). The entry state is the script's own
// initial values, and every stage writes only what the original action
// writes: no stage fakes a flag to reach the next step.
inline const std::vector<PuzzleJumpStage> djoseJumpStages={
    {"entry",{{0x0c000095,1},{0x0c000096,1},{0x0c00009b,1},{0x0c00009c,1},{0x0c00009d,1},{0x0c00009f,10}},1},
    {"take entry sphere 1",{{0x0c000094,1},{0x0c000095,0}},2},
    {"insert door recess 1",{{0x0c000097,1},{0x0c000094,0}},3},
    {"take entry sphere 2",{{0x0c000094,1},{0x0c000096,0}},4},
    {"insert door recess 2 opens door",{{0x0c000098,1},{0x0c000094,0},{0x0000028e,1}},5},
    {"take charged-door recess sphere",{{0x0c000094,1},{0x0c00009b,0}},6},
    {"insert charging recess 1",{{0x0c000099,1},{0x0c000094,0}},7},
    {"take left circuit sphere",{{0x0c000094,1},{0x0c00009c,0}},8},
    {"insert charging recess 2 powers charger",{{0x0c00009a,1},{0x0c000094,0},{0x0c0000a1,1}},9},
    {"push loaded pedestal under charger",{{0x0c00009d,3},{0x0c0000a1,2}},0,10},
    // Reset tile 67 writes only 0xd9=0, which is already 0 here, so the
    // guide cannot see steps 10 and 11. The charged door recess (0x9b=3)
    // stays set until step 19 and recovers the route.
    {"reset tile returns pedestal",{{0x0c0000d9,0}},0,11},
    {"take charged sphere from east face",{{0x0c000094,3},{0x0c00009d,0}},0,12},
    {"insert charged door recess",{{0x0c00009b,3},{0x0c000094,0}},13},
    {"take charging recess 1",{{0x0c000094,1},{0x0c000099,0}},14},
    {"insert west pedestal socket",{{0x0c00009e,1},{0x0c000094,0}},15},
    {"take charging recess 2",{{0x0c000094,1},{0x0c00009a,0}},16},
    {"insert east pedestal socket",{{0x0c00009d,1},{0x0c000094,0}},17},
    {"push pedestal into gap",{{0x0c0000d9,1}},18},
    {"press gap switch",{{0x0c0000da,1}},19},
    {"take charged door sphere",{{0x0c000094,3},{0x0c00009b,0}},20},
    {"insert left circuit",{{0x0c00009c,3},{0x0c000094,0}},21},
    {"reset tile clears gap once door recess is empty",{{0x0c0000d9,0}},22},
    {"take east pedestal sphere",{{0x0c000094,1},{0x0c00009d,0}},23},
    {"insert entry recess 1",{{0x0c000095,1},{0x0c000094,0}},24},
    {"take west pedestal sphere",{{0x0c000094,1},{0x0c00009e,0}},25},
    {"insert entry recess 2 completes lift circuit",{{0x0c000096,1},{0x0c000094,0},{0x0c0000db,1},{0x0c0000e5,1}},26},
    {"touch hidden glyph",{{0x0c0000e5,2}},27},
    {"take Destruction Sphere",{{0x0c000094,10},{0x0c00009f,0}},0,28},
    {"push upper pedestal 1",{{0x0c0000de,1}},30},
    {"push upper pedestals 2 to 4",{{0x0c0000df,1},{0x0c0000e0,1},{0x0c0000e1,1}},33},
    {"push upper pedestal 5",{{0x0c0000e3,1}},34},
    {"insert Destruction Sphere",{{0x0c000094,0},{0x0c0000e3,2}},35},
    {"open Destruction treasure",{{0x01000a70,4}},36},
};
// Why a Djose start step cannot be reached by variable writes alone, or
// nullptr when it can. The jump never moves pedestals or the player.
inline const wchar_t* djoseJumpRefusal(unsigned step) {
    if(step==10) return L"Step 10 needs the pedestal under the charger.";
    if(step==18) return L"Step 18 needs the pedestal in the gap and the player across it.";
    if(step>=19 && step<=21) return L"Steps 19 to 21 need the pedestal in the gap.";
    if(step>=30 && step<=33) return L"Steps 30 to 33 need the earlier upper pedestals in the wall.";
    if(!step || step>36) return L"Djose has steps 1 to 36.";
    return nullptr;
}
// Room variables are absolute: every route binding gets its value after the
// stages that come before the step, or 0. Save data outside the room
// (high byte not 0x0c) is only ever set, never cleared.
inline std::vector<PuzzleJumpWrite> puzzleJumpWrites(const std::vector<PuzzleJumpStage>& stages,unsigned step) {
    std::vector<PuzzleJumpWrite> result;
    const auto find=[&](const PuzzleJumpWrite& write)->PuzzleJumpWrite* {
        for(auto& item:result) if(item.low==write.low && item.high==write.high && item.element==write.element) return &item;return nullptr;};
    for(const auto& stage:stages) for(const auto& write:stage.writes) if(!find(write) && (write.low>>24)==0x0c) result.push_back({write.low,0,write.element,write.high});
    for(const auto& stage:stages) {
        if(puzzleJumpNext(stage)>step) break;
        for(const auto& write:stage.writes) {
            if(auto* item=find(write)) item->value=write.value;
            else if(write.value) result.push_back(write);
        }
    }
    return result;
}
}
