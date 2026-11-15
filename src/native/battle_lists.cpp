#include "battle_lists.hpp"
#include "runtime.hpp"

namespace ffx {
namespace {
// BtlWindow types whose rows are a scrolling submenu list (Docs/gameArchitecture.md).
bool indexedList(unsigned type) {
    switch(type) {
    case 1: case 2: case 3: case 4: case 5: case 6: case 7: // Black/White Magic, Skill, Overdrive, Summon, Item, Weapon
    case 0xa: case 0xe: case 0xf: case 0x11: case 0x14: // Change, Special, Armor, Use, Mix
        return true;
    default:
        return false;
    }
}
}
TextRef planBattleListName(TextPlan& p,TextRef name,uintptr_t window) {
    if(!window || !indexedList(read<uint16_t>(window+6))) return name;
    const auto count=read<unsigned>(window+0x24);
    if(count>0x1000) return name;
    const auto index=planMenuIndex(p,read<uint8_t>(window+0x42),count);
    if(!index) return name;
    return planPair(p,TextPlanOp::IfText,name,planConcat(p,name,index));
}
}
