#pragma once
#include "navigation.hpp"
#include "controller_model.hpp"
namespace ffx {
void startAutoWalk(bool announce=true);
unsigned autoWalkRouteFailureRevision();
void stopAutoWalk();
// Stops for a lost route. Unlike stopAutoWalk it is not a player command, so a
// pending battle resume survives; a walk it ends is never resumed.
void releaseAutoWalk();
// Backslash toggle: stops an active walk and says so. False when no walk is active.
// Backslash during a walk: runs cancelCommand (the navigation command's full cancel
// sequence, which stops the walk) or, without one, stopAutoWalk, then says so.
bool toggleAutoWalkOff(void (*cancelCommand)()=nullptr);
// True while a walk stopped by a battle is waiting to resume.
bool autoWalkResumePending();
// Stops like stopAutoWalk but keeps a pending battle resume. For field resets only.
void suspendAutoWalk();
// Called once on the native thread when a walk stopped by a battle may resume.
// The handler re-requests a route to the copied destination; it returns false
// when no request could be queued.
using AutoWalkResume=bool(*)(const NativeRouteTarget&);
void setAutoWalkResume(AutoWalkResume);
bool autoWalkActive();
// Native owner: current effective steering target, including pedestal alignment.
bool autoWalkSteeringGoal(Point&);
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
