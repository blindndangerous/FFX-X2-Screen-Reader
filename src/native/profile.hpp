#pragma once
#include "runtime.hpp"
#include "map_transition.hpp"
#include <stdexcept>
#include <map>

namespace ffx {
// The verified executable decides which game runs. Ffx2 resolves only the
// FFX RVAs listed in ffx2Addresses; profileRva refuses every other one there.
enum class NativeProfile { Reference, Installed, MovieCaptions, Ffx2 };
bool nativeProfileForDigest(const std::string& digest,NativeProfile& profile);
const char* nativeProfileName(NativeProfile profile);
bool validateNativeDependencies();
bool nativeFileDigest(const wchar_t* path,std::string& result);
inline NativeProfile nativeProfile = NativeProfile::Reference;
inline std::map<unsigned,unsigned> extendedAddresses;
struct RvaPair { unsigned reference, installed; };
// Generated from individually matched bodies and instruction references.
// Evidence: reader_audit_20261004_122556.json; no blanket code/data delta.
inline constexpr RvaPair nativeAddresses[] = {
    {0x2a00, 0x2a00},
    {0x241480, 0x2412d0},
    {0x2414a0, 0x2412f0},
    {0x2460e0, 0x245f30},
    {0x28e6e0, 0x28e560},
    {0x2efd90, 0x2efc50},
    {0x2f00e0, 0x2effa0},
    {0x2f06f0, 0x2f05b0},
    {0x302db0, 0x302cd0},
    {0x302e00, 0x302d20},
    {0x302e70, 0x302d90},
    {0x302f70, 0x302e90},
    {0x302fe0, 0x302ef0},
    {0x375df0, 0x375ce0},
    {0x375ef0, 0x375de0},
    {0x376220, 0x376110},
    {0x376a50, 0x376940},
    {0x382730, 0x382670},
    {0x385070, 0x384fb0},
    {0x3852b0, 0x3851f0},
    {0x3853f0, 0x385330},
    {0x38f0b0, 0x38f060},
    {0x38fd40, 0x38fcf0},
    {0x390500, 0x3904b0},
    {0x3908f0, 0x3908a0},
    {0x390a40, 0x3909f0},
    {0x394030, 0x394020},
    {0x39a1a0, 0x39a190},
    {0x39a4c0, 0x39a4b0},
    {0x39cff0, 0x39cfd0},
    {0x39fa20, 0x39fa00},
    {0x3a0c70, 0x3a0c50},
    {0x3ab890, 0x3ab870},
    {0x3abbf0, 0x3abbd0},
    {0x3abe10, 0x3abdf0},
    {0x425010, 0x424f90},
    {0x426770, 0x4266f0},
    {0x42b500, 0x42b480},
    {0x469040, 0x4690a0},
    {0x469170, 0x4691d0},
    {0x46a7c0, 0x46a830},
    {0x46c3c0, 0x46c430},
    {0x46c500, 0x46c570},
    {0x46c840, 0x46c8b0},
    {0x46e920, 0x46e990},
    {0x46fed0, 0x46ff40},
    {0x4713a0, 0x471400},
    {0x471d10, 0x471d70},
    {0x473af0, 0x473b50},
    {0x490ee0, 0x490f70},
    {0x491af0, 0x491b80},
    {0x492290, 0x492320},
    {0x498bd0, 0x498c60},
    {0x49a5a0, 0x49a630},
    {0x49c870, 0x49c900},
    {0x49cd50, 0x49cde0},
    {0x49d300, 0x49d390},
    {0x49d580, 0x49d610},
    {0x49db10, 0x49dba0},
    {0x4a0d70, 0x4a0de0},
    {0x4a2100, 0x4a2160},
    {0x4a2290, 0x4a22f0},
    {0x4a2630, 0x4a2690},
    {0x4a9810, 0x4a9860},
    {0x4a9830, 0x4a9880},
    {0x4a9ab0, 0x4a9b00},
    {0x4aa0b0, 0x4aa100},
    {0x4b4460, 0x4b44b0},
    {0x4b5f60, 0x4b5fb0},
    {0x4b6260, 0x4b62b0},
    {0x4b6490, 0x4b64e0},
    {0x4b7f10, 0x4b7f60},
    {0x4b8980, 0x4b89d0},
    {0x4c1ad0, 0x4c1af0},
    {0x4c1b50, 0x4c1b70},
    {0x4c1c00, 0x4c1c20},
    {0x4c6da0, 0x4c6dd0},
    {0x4cb0b0, 0x4cb0e0},
    {0x4ccb30, 0x4ccb60},
    {0x4ccea0, 0x4cced0},
    {0x4cd2f0, 0x4cd320},
    {0x4cd6a0, 0x4cd6d0},
    {0x4cdb70, 0x4cdba0},
    {0x4cec70, 0x4ceca0},
    {0x4cf9c0, 0x4cf9f0},
    {0x4d0e80, 0x4d0eb0},
    {0x4d16c0, 0x4d16f0},
    {0x4d1dd0, 0x4d1e00},
    {0x4d5f30, 0x4d5f60},
    {0x4d6830, 0x4d6860},
    {0x4d77c0, 0x4d77f0},
    {0x4d7990, 0x4d79c0},
    {0x4d9140, 0x4d9170},
    {0x4d9340, 0x4d9370},
    {0x4d93e0, 0x4d9410},
    {0x4d9ba0, 0x4d9bd0},
    {0x4d9d70, 0x4d9da0},
    {0x4d9df0, 0x4d9e20},
    {0x4da440, 0x4da470},
    {0x4db390, 0x4db3b0},
    {0x4db3e0, 0x4db400},
    {0x4ddfa0, 0x4ddfc0},
    {0x4e0340, 0x4e0350},
    {0x4e2e20, 0x4e2e20},
    {0x4e3880, 0x4e3880},
    {0x5093b0, 0x5093b0},
    {0x648d70, 0x648d70},
    {0x648e40, 0x648e40},
    {0x649310, 0x649310},
    {0x653570, 0x653570},
    {0x653de0, 0x653de0},
    {0x654b40, 0x654b40},
    {0x656060, 0x656060},
    {0x6560d0, 0x6560d0},
    {0x656330, 0x656330},
    {0x6583e0, 0x6583e0},
    {0x6598a0, 0x6598a0},
    {0x659950, 0x659950},
    {0x65b980, 0x65b980},
    {0x839620, 0x839628},
    {0x8416b0, 0x8416c0},
    {0x85a608, 0x85a608},
    {0x85a60c, 0x85a60c},
    {0x8e72f0, 0x8e7300},
    {0x8eb250, 0x8eb260},
    {0x8eb690, 0x8eb6a0},
    {0x8eb890, 0x8eb8a0},
    {0x8eba90, 0x8ebaa0},
    {0x8ebcc8, 0x8ebcd8},
    {0x8ebec8, 0x8ebed8},
    {0xd2a01c, 0xd2a01c},
    {0xd2a024, 0xd2a024},
    {0xd2a03c, 0xd2a03c},
    {0xd2a040, 0xd2a040},
    {0xd2a080, 0xd2a080},
    {0xd2a084, 0xd2a084},
    {0xd2a088, 0xd2a088},
    {0xd2a0a4, 0xd2a0a4},
    {0xd2a8e0, 0xd2a8e0},
    {0xd2ca90, 0xd2ca90},
    {0xd334cc, 0xd334d4},
    {0xd34460, 0xd34468},
    {0xefbbf8, 0xefbc40},
    {0xf01a44, 0xf01a84},
    {0xf01a48, 0xf01a88},
    {0xf01a4c, 0xf01a8c},
    {0xf01a50, 0xf01a90},
    {0xf26ae8, 0xf26b28},
    {0xf3c910, 0xf3c950},
    {0xf3d6b2, 0xf3d6f2},
    {0xf3d6b3, 0xf3d6f3},
    {0xf3d6b4, 0xf3d6f4},
    {0xf3d6c8, 0xf3d708},
    {0xf3d6df, 0xf3d71f},
    {0xf3d6f4, 0xf3d734},
    {0xf3f4f8, 0xf3f538},
    {0x1196f30, 0x1196f70},
    {0x1197730, 0x1197770},
    {0x14676b0, 0x14676f0},
    {0x1468a50, 0x1468a90},
    {0x146a3f8, 0x146a438},
    {0x146a404, 0x146a444},
    {0x146a408, 0x146a448},
    {0x146a40c, 0x146a44c},
    {0x146a9c4, 0x146aa04},
    {0x146a9e4, 0x146aa24},
    {0x146a9f8, 0x146aa38},
    {0x146aadc, 0x146ab1c},
    {0x146ab68, 0x146aba8},
    {0x146abe0, 0x146ac20},
    {0x146abec, 0x146ac2c},
    {0x1471508, 0x1471548},
    {0x147150c, 0x147154c},
    {0x147151c, 0x147155c},
    {0x1471520, 0x1471560},
    {0x1471528, 0x1471568},
    {0x1685f70, 0x1685f70},
    {0x1686108, 0x1686108},
    {0x1efb504, 0x1efb504},
    {0x1f05834, 0x1f05834},
    {0x1f10ea0, 0x1f10ea0},
    {0x1f11430, 0x1f11430},
    {0x1fc44e0, 0x1fc44e0},
    {0x1fc44e4, 0x1fc44e4},
    {0x1fcc092, 0x1fcc092},
};
// FFX reference RVA -> FFX-2.exe RVA. Only functions whose whole body matches
// the FFX function once relocated operands and call displacements are masked.
// Evidence and the rejected candidates: Docs/ffx2-addresses.md.
struct Ffx2Rva { unsigned reference, ffx2; };
inline constexpr Ffx2Rva ffx2Addresses[] = {
    {0x225930, 0x6b50a0}, // PInput::Update, game-thread probe
    {0x22d770, 0x6bcea0}, // DirectInput keyboard updater
    {0x22db80, 0x6bd2b0}, // DirectInput pad updater
    {0x471d10, 0x32ce30}, // per-frame input event
    // Matched by prologue, unique caller graph and log strings, not by a whole
    // masked body: FFX-2 grew these functions. See Docs/ffx2-addresses.md.
    {0x46fed0, 0x32a970}, // map request: event map jump
    {0x5093b0, 0x3ba3b0}, // map load ("RAM Inuse"), load-finished boundary
};
// FFX-2-specific addresses and layouts, found by call-graph and data-flow
// reading of FFX-2.exe. Experimental field sites remain disabled by default.
// Evidence:
// Docs/ffx2-addresses.md, "FFX-2-only addresses".
enum class Ffx2Only : unsigned {
    TopMenuUpdate, // top-level menu module update (module VA 0xd62e20, slot 0)
    TopMenuCursor, // dword cursor index into the item array
    TopMenuItems,  // 12 entries of 8 bytes: dword menu id, word available, word needs member
    TopMenuCount,  // dword number of items in the array
    HelpText,      // pointer to the help line the menu shows (setter 0x365a50)
    Charset,       // pointer to the loaded ffx2sjistbl_us.bin (loader 0x111120)
    StoryMode,     // dword read by 0x392ee0; zero in the main story
    FieldTextWindows, // eight window pointers; cursor state is inside each window
    FieldTextSet,     // sets +0xf0/+0xf4, unlike FFX's +0x90/+0x94
    FieldTextOpen,    // begins a new active window lifetime
    FieldTextPage,    // selected page preparation callback (+0x128)
    FieldTextChoice,  // ordinary choice callback (+0x130); numeric input is separate
};
struct Ffx2OnlyRva { Ffx2Only name; unsigned rva; };
inline constexpr Ffx2OnlyRva ffx2OnlyAddresses[] = {
    {Ffx2Only::TopMenuUpdate, 0x373170},
    {Ffx2Only::TopMenuCursor, 0x12bef0c},
    {Ffx2Only::TopMenuItems, 0x12befe0},
    {Ffx2Only::TopMenuCount, 0x12bf040},
    {Ffx2Only::HelpText, 0x12bd97c},
    {Ffx2Only::Charset, 0x9eade0},
    {Ffx2Only::StoryMode, 0x12ef654},
    {Ffx2Only::FieldTextWindows, 0x18ceb20},
    {Ffx2Only::FieldTextSet, 0x396fc0},
    {Ffx2Only::FieldTextOpen, 0x396280},
    {Ffx2Only::FieldTextPage, 0x39b9f0},
    {Ffx2Only::FieldTextChoice, 0x3926e0},
};
inline unsigned ffx2Rva(Ffx2Only name) {
    if(nativeProfile!=NativeProfile::Ffx2) throw std::out_of_range("FFX-2-only address outside the FFX-2 profile");
    for(const auto& entry:ffx2OnlyAddresses) if(entry.name==name) return entry.rva;
    throw std::out_of_range("Unknown FFX-2-only address");
}
// Hook ids for FFX-2-only functions, outside the FFX RVA space that handlers
// and resource scopes compare against.
inline constexpr unsigned ffx2HookId(unsigned rva) { return 0xfe000000u | rva; }
inline unsigned profileRva(unsigned rva) {
    if(nativeProfile==NativeProfile::Ffx2) {
        if(!rva) return 0;
        for(const auto& pair:ffx2Addresses) if(pair.reference==rva) return pair.ffx2;
        throw std::out_of_range("Unverified FFX-2 RVA " + std::to_string(rva));
    }
    if (!rva || nativeProfile == NativeProfile::Reference) return rva;
    const auto extension=extendedAddresses.find(rva);
    if(extension!=extendedAddresses.end()) return extension->second;
    if(nativeProfile==NativeProfile::MovieCaptions)
        throw std::out_of_range("Unverified movie-caption RVA " + std::to_string(rva));
    size_t low = 0, high = sizeof(nativeAddresses) / sizeof(nativeAddresses[0]);
    while (low < high) {
        const auto middle = low + (high - low) / 2;
        if (nativeAddresses[middle].reference < rva) low = middle + 1;
        else high = middle;
    }
    if (low < sizeof(nativeAddresses) / sizeof(nativeAddresses[0]) &&
        nativeAddresses[low].reference == rva) return nativeAddresses[low].installed;
    throw std::out_of_range("Unverified native RVA " + std::to_string(rva));
}
// PInput::Update. Its entry marks the game thread for every profile.
inline unsigned gameThreadProbe() { return 0x225930; }
// Calls whose dynamic extent loads game resources. FFX-2 has verified map
// request and map load equivalents, but no field-initialize equivalent yet.
inline bool resourceScopeHook(unsigned rva) {
    if(nativeProfile==NativeProfile::Ffx2) return rva==0x46fed0 || rva==0x5093b0;
    return rva==0x46fed0 || rva==0x5093b0 || rva==map_transition::Initialize;
}
inline unsigned logicalRva(uintptr_t pointer) {
    const auto rva = static_cast<unsigned>(pointer - address(0));
    if (nativeProfile == NativeProfile::Installed)
        for (const auto& pair : nativeAddresses) if (pair.installed == rva) return pair.reference;
    return rva;
}
bool configureNativeProfile();
bool configureNativeProfileForPath(const wchar_t* path);
bool resolveNativeExtensions();
bool resolveNativeExtensionsAt(uintptr_t base,unsigned imageSize);
}
