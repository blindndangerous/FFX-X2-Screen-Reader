#pragma once
// FFX-2 Config submenu (slot 8, update 0x388880, input phase 0x388c40):
// pure helpers for the menu text lookup and the spoken line. All layouts are
// static evidence, UNVERIFIED in game; see Docs/ffx2-addresses.md (Config).
#include "ffx2_submenus.hpp"
#include "ffx2_menu_snapshot.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace ffx::ffx2menu {
// One id range of a loaded menu text table (battle/kernel/menu_txt.bin), as
// read by 0x21e3a0: dword count at +0, 20-byte ranges from +0xc.
struct TextRange {std::uint32_t first=0,last=0,stride=0,strings=0,records=0;};
constexpr unsigned MaxTextRanges=8;
constexpr unsigned TextHeaderBytes=0xc+MaxTextRanges*20;
// Finds the range holding `id & 0xfff` in owned header bytes. The game falls
// back to the first range for an unknown id; this refuses it instead.
bool findTextRange(const std::uint8_t* header,std::size_t size,unsigned id,TextRange&);
// Table-relative offsets of the 8-byte record and of the string base.
bool textRecordOffset(const TextRange&,unsigned id,std::uint32_t& record);
inline std::uint32_t textStringsOffset(const TextRange& range) {return range.strings+range.records;}
// Record word +0 is the name (0x21d840 with space 1, via 0x379170); word +4
// is the help (0x21d7c0 with space 1, via 0x379150).
constexpr unsigned TextNameField=0,TextHelpField=4;

enum class ConfigChange : std::uint32_t {None,Row,Value};
// The row is the setting cursor; a same-row change of the selected choice is
// a value change. Anything else (first sight, a different record) is silent.
ConfigChange configChange(const ConfigRecordSelection& before,const ConfigRecordSelection& after);

// Owned capture of one Config change: encoded label, value name and the value
// help line the input phase sets through 0x3638a0 (0x3891c0 to 0x3891cc).
struct ConfigSnapshot {
    std::uint32_t change=0,labelBytes=0,valueBytes=0,helpBytes=0,charsetBytes=0;
    std::array<std::uint8_t,EnglishCharsetBytes> charset{};
    std::array<std::uint8_t,64> label{},value{};
    std::array<std::uint8_t,128> help{};
};

// Row: "Subtitles: ON. Display subtitles." Value: "OFF. Hide subtitles."
// Empty parts are left out; a value change without a value name is silent.
std::wstring configSpeech(ConfigChange,const std::string& label,const std::string& value,const std::string& help);
}
// ffx2_config_reader.cpp: false unless built with FFX2_EXPERIMENTAL_CONFIG=1
// and the FFX-2 profile matched the verified input phase prologue.
namespace ffx { bool installFfx2Config(); }
