#pragma once
#include "utility_text.hpp"
#include <string_view>
namespace ffx {
#ifdef FFX_DEV_UTILITIES
// Native owner only. `arguments` is the step number after "puzzle jump ".
UtilityText puzzleJumpCommand(std::wstring_view arguments);
#endif
}
