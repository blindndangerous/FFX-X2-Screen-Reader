#pragma once
// FFX-2 top-level menu (module VA 0xd62e20, update 0x373170): pure helpers
// for speaking the focused item. Evidence in Docs/ffx2-addresses.md
// (FFX-2-only addresses).
#include <array>
#include <cstdint>
#include <string>

namespace ffx::ffx2menu {
// 0x375900 builds the list from menu ids 0..11 into 8-byte entries.
constexpr unsigned TopMenuCapacity = 12;

// Copied from game memory: cursor (VA 0x16bef0c), item count (VA 0x16bf040)
// and the id word of each entry (VA 0x16befe0, stride 8).
struct TopMenuState {
    std::uint32_t cursor = 0, count = 0;
    std::array<std::uint32_t, TopMenuCapacity> ids{};
};

// English item name for a menu id, or nullptr. The one table of names; a
// translated list can replace it later.
const wchar_t* topMenuItemName(unsigned id);

// The id at the cursor. False when the list is empty, too long, or the
// cursor is outside it.
bool focusedId(const TopMenuState& state, unsigned& id);

// True only when the cursor changed inside one unchanged, valid list.
bool cursorMoved(const TopMenuState& before, const TopMenuState& after);

// "<name>. <help>". The help line is UTF-8 from ffx2text::decode; line
// breaks become spaces, blank help is dropped. An unknown id speaks only
// the help line.
std::wstring topMenuSpeech(unsigned id, const std::string& helpUtf8);

// UTF-8 to UTF-16. Malformed bytes are dropped.
std::wstring wideFromUtf8(const std::string& text);
}
