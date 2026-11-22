#pragma once
// Development pipe "focus hold": scripted tests may treat the game window as
// focused while another window has focus. Release builds compile this to
// gameFocused() alone. Header-only so field gates need no new link input.
#include "settings.hpp"
#ifdef FFX_DEV_UTILITIES
#include <atomic>
#endif
namespace ffx {
#ifdef FFX_DEV_UTILITIES
inline std::atomic<bool> devFocusHoldFlag{false};
inline bool devFocusHeld() noexcept {return devFocusHoldFlag.load(std::memory_order_acquire);}
void setDevFocusHold(bool held) noexcept; // utility_context.cpp; clears on map change.
inline bool gameFocusedOrHeld() {return devFocusHeld() || gameFocused();}
#else
inline bool gameFocusedOrHeld() {return gameFocused();}
#endif
}
