#pragma once
#include "text_plan.hpp"
#include <cstdint>

namespace ffx {
// Appends ", N of M" to a battle list row name when Menu indexing is On.
// N is BtlWindow sel_idx (+0x42) and M is command_list_len (+0x24), the
// same row and length battle cursor speech already reads. The main command
// bar, Left/Right command bars and Gil amount windows are not indexed.
TextRef planBattleListName(TextPlan&,TextRef name,uintptr_t window);
}
