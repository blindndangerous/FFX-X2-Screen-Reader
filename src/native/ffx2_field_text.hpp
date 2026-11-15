#pragma once
#include "ffx2_text.hpp"
#include <array>

// Static-only experiment. Enable explicitly for a separately authorized live check.
#ifndef FFX2_EXPERIMENTAL_FIELD_TEXT
#define FFX2_EXPERIMENTAL_FIELD_TEXT 0
#endif

namespace ffx::ffx2text {
constexpr unsigned ChoiceLimit=32;
struct FieldPage {
    std::string prompt;
    std::array<std::string,ChoiceLimit> choices;
    bool valid=false,selector=false;
};
// One current page only: 00/01/02 terminate. Unsupported substitutions fail
// closed rather than silently deleting names, values or button instructions.
FieldPage decodeFieldPage(const std::uint8_t*,std::size_t,const Charset&);
}
namespace ffx { bool installFfx2FieldText(); }
