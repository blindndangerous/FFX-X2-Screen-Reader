#pragma once
#include <array>
#include <cstdint>

namespace ffx::ffx2menu {
// Static archive evidence: the pinned English charset file is 310 bytes.
// No game pointer survives capture; a failed copy leaves the label only.
constexpr unsigned EnglishCharsetBytes=310;
constexpr unsigned EnglishCharsetGlyphs=192;
struct MenuSnapshot {
    unsigned id=0,charsetBytes=0,helpBytes=0;
    std::array<std::uint8_t,EnglishCharsetBytes> charset;
    std::array<std::uint8_t,256> help;
};
}
