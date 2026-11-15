#pragma once
#include <array>
#include <cstdint>

namespace ffx::ffx2menu {
// One native observation. No game pointer survives capture. These are copy
// ceilings, not inferred allocation sizes; a failed copy leaves the label only.
struct MenuSnapshot {
    unsigned id=0,charsetBytes=0,helpBytes=0;
    std::array<std::uint8_t,1024> charset;
    std::array<std::uint8_t,256> help;
};
}
