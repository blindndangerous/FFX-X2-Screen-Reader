#pragma once
#include "runtime.hpp"
namespace ffx {
struct UtilityContext { unsigned revision=0,epoch=0; HWND window=nullptr; };
// UI-safe snapshots. These functions never call a game getter on the UI thread.
UtilityContext utilityContext();
bool utilityContextValid(const UtilityContext&,bool requireGameFocus=false);
bool utilityGameThread();
void updateUtilityContext();
void invalidateUtilityContext();
void installUtilityContext();
// Called on the verified native owner immediately before a job.
bool utilityActionAllowed(bool force=false);
// Native owner only: the field term of the last update, without the focus term.
bool utilityFieldReady();
// Native owner only: field mode 0x46c840 from the last update, -1 outside the field.
int utilityFieldMode();
}
// Development pipe focus hold (devFocusHeld, setDevFocusHold, gameFocusedOrHeld).
#include "dev_focus.hpp"
