#pragma once
#include "events.hpp"
namespace ffx {
bool announceEscSetting(uintptr_t entry);
bool escMenuActive();
bool escControlsAvailable();
bool readEscControls();
void installEscDetails();
void resetEscDetails();
int escVideoFieldIndex(unsigned field);
}
