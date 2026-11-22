#pragma once
// Bevelle rail auto turn (bvyt1100). The puzzle guide decides which phase each
// enabled junction region needs for the current step; the game thread holds
// Confirm only while the car is in that region and the live phase matches.
// Junction map: Docs/features/puzzles/index.md, Bevelle rail section.
#include "navigation.hpp"
#include <cstdint>
namespace ffx {
struct BevelleRailRow { unsigned worker=0; int wanted=-1; InteractionShape zone; };
struct BevelleRailPlan {
    static constexpr unsigned Rows=12;
    // objective: worker the current step heads for (bevelleRailWanted target).
    unsigned epoch=0,count=0,objective=0; uint64_t at=0; BevelleRailRow rows[Rows];
};
// bevelle_rail_plan.cpp: no game access, shared with the guide fixtures.
// Phase wanted at junction `worker` for objective worker `target`; -1 means
// let the junction pass. W40 returns 1 for a single Confirm press.
int bevelleRailWanted(unsigned worker,unsigned target);
// Binding that holds the arrow phase read by junction `worker`, or 0.
unsigned bevelleRailPhaseBinding(unsigned worker);
// Guide thread writes, game thread reads.
void publishBevelleRailPlan(const BevelleRailPlan&);
void clearBevelleRailPlan();
bool readBevelleRailPlan(BevelleRailPlan& out);
// True while the focused game thread can turn the rail (Auto walk on, no
// menu, console or battle). The guide then speaks what the mod does, not timing.
bool bevelleRailAutoActive();
void setBevelleRailAutoActive(bool active);
// bevelle_rail.cpp: game thread, once per input frame after the pedestal input.
void updateBevelleRail();
void clearBevelleRailInput(); // Before the field aggregator collects native input.
void releaseBevelleRailIfUnavailable(); // Also called on native menu input.
}
