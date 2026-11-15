#pragma once
// Offline parsers only. All native layouts are UNVERIFIED in game; no hooks.
#include <cstdint>
#include <vector>

namespace ffx::ffx2menu {
enum class NativeList {Items,Abilities};
struct Selection {unsigned index=0,id=0;bool available=false;};
bool parseNativeSelection(NativeList,const std::vector<std::uint8_t>& window,
    const std::vector<std::uint8_t>& rows,Selection&);
// Dressphere parent panel +8 is a signed cursor into the sorted byte ID list;
// +0x1e holds the count. The caller supplies owned bytes from one qualified phase.
bool parsePanelSelection(const std::vector<std::uint8_t>& panel,
    const std::vector<std::uint8_t>& ids,Selection&);
struct GridSelection {unsigned grid=0,node=0,dressphere=0;bool populated=false;};
// Node panel +8 selects a node; +0xe selects a grid. Each grid owns 32 bytes.
bool parseGridSelection(const std::vector<std::uint8_t>& panel,
    const std::vector<std::uint8_t>& nodes,GridSelection&);
struct ConfigRecordSelection {unsigned index=0,labelId=0,valueId=0,valueIndex=0,valueCount=0;};
// First 16 bytes of the selected main-story setting record, excluding callbacks.
// The 16-byte arrays at 16D5F50/16D6000/16D60B0 are animation, not text.
bool parseConfigRecord(int cursor,const std::vector<std::uint8_t>& selectedRecord,ConfigRecordSelection&);
}
