#pragma once
#include "navigation.hpp"
#include "controller_model.hpp"
namespace ffx {
void startAutoWalk();
void stopAutoWalk();
bool autoWalkActive();
void updateAutoWalk();
void autoWalkKeyboard(uint8_t* keys);
void autoWalkController(pad::State& state);
void autoWalkPlayerInput(unsigned vk);
// Input owners publish whether Insert, Numpad Insert or Caps Lock is held before
// reporting key presses; those screen-reader chords never cancel auto walk.
void autoWalkReaderModifier(bool held);
unsigned autoWalkInputRevision(); // Copied manual-input cancellation token for delayed routes.
void installAutoWalk();
using NavigationKeyboardAssist=void(*)(uint8_t*);
using NavigationControllerAssist=void(*)(pad::State&);
void setNavigationInputAssist(NavigationKeyboardAssist,NavigationControllerAssist);
}
