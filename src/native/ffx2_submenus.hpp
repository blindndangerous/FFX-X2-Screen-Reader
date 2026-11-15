#pragma once
// Offline parsers only. All native layouts are UNVERIFIED in game; no hooks.
#include <array>
#include <cstdint>
#include <vector>

namespace ffx::ffx2menu {
enum class NativeList {Items,Abilities};
struct Selection {unsigned index=0,id=0;bool available=false;};
bool parseNativeSelection(NativeList,const std::vector<std::uint8_t>& window,
    const std::vector<std::uint8_t>& rows,Selection&);
// Legacy panels used by Grid/Dressphere controllers have a signed cursor at
// +8 and total at +0x1e. The caller supplies an independently qualified ID list;
// this parser does not assert which controller phase owns a particular list.
bool parsePanelSelection(const std::vector<std::uint8_t>& panel,
    const std::vector<std::uint8_t>& ids,Selection&);
struct ConfigSelection {
    unsigned index=0;
    std::array<std::uint8_t,16> label{},value{},help{};
};
// Config descriptors are engine presentation objects, not encoded strings.
bool parseConfigSelection(int cursor,const std::vector<std::uint8_t>& labels,
    const std::vector<std::uint8_t>& values,const std::vector<std::uint8_t>& help,ConfigSelection&);
}
