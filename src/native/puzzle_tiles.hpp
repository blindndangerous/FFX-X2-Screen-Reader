#pragma once
#include "puzzle_guide.hpp"
namespace ffx {
// A nonempty blocked list is a conjunction: every quota slot must be full.
struct PuzzleTile { unsigned stage, worker; Point point; std::vector<UtilityBinding> blocked; };
void puzzleTileCompleted(uintptr_t worker, unsigned entry);
}
