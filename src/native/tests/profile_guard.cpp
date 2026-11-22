#include "fixture_runtime.hpp"
#include "../profile.hpp"
#include <cstdio>

namespace ffx {
void log(const char*, ...) {}
}
namespace {
bool selects(const char* digest,ffx::NativeProfile expected) {
    auto selected=ffx::NativeProfile::Reference;
    if(expected==selected) selected=ffx::NativeProfile::Installed;
    return ffx::nativeProfileForDigest(digest,selected) && selected==expected;
}
bool refusesFfxAddress(unsigned rva) {
    try { ffx::profileRva(rva); } catch(const std::out_of_range&) { return true; }
    return false;
}
}
int main() {
    // Hash this real fixture executable with the production Win32 routine.
    // It must be refused before native hook installation can begin.
    if (ffx::configureNativeProfile()) {
        std::fprintf(stderr, "Unsupported executable passed the native profile guard.\n");
        return 1;
    }
    if (ffx::nativeProfile != ffx::NativeProfile::Reference) return 1;
    // Exact image identity picks the game. FFX profiles must not change.
    if(!selects("3d13e5ed53821df8db204cd3c27d470c5c220b18e16849fc4440996c15f277e1",ffx::NativeProfile::Reference) ||
       !selects("0537b2a1047f3266e73495cd4e35f63f0777f4231d417699f979954686da686d",ffx::NativeProfile::Installed) ||
       !selects("18e1b899a091a1a21f0732eab8a49f7067e5347cb442dc35d5910bb23652d123",ffx::NativeProfile::MovieCaptions)) {
        std::fprintf(stderr, "A verified FFX executable no longer selects its profile.\n");
        return 1;
    }
    if(!selects("6ea7f142f129a43bdaea7eaabda4477a9057e60d12fdae367530c63e9a14cd69",ffx::NativeProfile::Ffx2)) {
        std::fprintf(stderr, "Steam FFX-2.exe did not select the FFX-2 profile.\n");
        return 1;
    }
    auto untouched=ffx::NativeProfile::Installed;
    if(ffx::nativeProfileForDigest("6ea7f142f129a43bdaea7eaabda4477a9057e60d12fdae367530c63e9a14cd68",untouched) ||
       untouched!=ffx::NativeProfile::Installed) {
        std::fprintf(stderr, "An unknown executable digest selected a profile.\n");
        return 1;
    }
    if(std::string(ffx::nativeProfileName(ffx::NativeProfile::Ffx2))!="FFX-2") return 1;
    // FFX-2 resolves only the input addresses whose whole function body was
    // matched against FFX (Docs/ffx2-addresses.md). Every other FFX hook,
    // probe and data RVA must be refused there, including the reference
    // pass-through.
    ffx::nativeProfile=ffx::NativeProfile::Ffx2;
    const struct {unsigned ffx,ffx2;} verified[]={
        {0x225930u,0x6b50a0u},{0x22d770u,0x6bcea0u},{0x22db80u,0x6bd2b0u},{0x471d10u,0x32ce30u},
        {0x46fed0u,0x32a970u},{0x5093b0u,0x3ba3b0u}};
    for(const auto& pair:verified) {
        unsigned resolved=0;
        try { resolved=ffx::profileRva(pair.ffx); } catch(const std::out_of_range&) {}
        if(resolved!=pair.ffx2) {
            std::fprintf(stderr, "Verified FFX-2 RVA for 0x%x resolved to 0x%x, expected 0x%x.\n", pair.ffx, resolved, pair.ffx2);
            return 1;
        }
    }
    if(ffx::profileRva(0)!=0) {
        std::fprintf(stderr, "The FFX-2 image base no longer resolves.\n");
        return 1;
    }
    for(const unsigned rva:{0x4b6260u,0x4e2e20u,0x376220u,0x22d700u,0x22dd50u,0x48dfe0u,0xd2a8e0u,0x1fc44e4u}) {
        if(!refusesFfxAddress(rva)) {
            std::fprintf(stderr, "Unverified FFX RVA 0x%x resolved under the FFX-2 profile.\n", rva);
            return 1;
        }
    }
    // FFX-2 opens resource scopes only for the verified map request and map
    // load. The field-initialize scope (0x48dfe0) has no FFX-2 address yet.
    if(!ffx::resourceScopeHook(0x46fed0u) || !ffx::resourceScopeHook(0x5093b0u) || ffx::resourceScopeHook(0x48dfe0u) ||
       ffx::resourceScopeHook(0x225930u) || ffx::gameThreadProbe()!=0x225930u) {
        std::fprintf(stderr, "FFX-2 resource scopes are not exactly map request and map load, or the game-thread probe changed.\n");
        return 1;
    }
    // FFX-2-only addresses (no FFX counterpart) resolve under FFX-2 only, to
    // the FFX-2.exe RVAs recorded in Docs/ffx2-addresses.md.
    const struct {ffx::Ffx2Only name; unsigned rva;} only[]={
        {ffx::Ffx2Only::TopMenuUpdate,0x373170u},{ffx::Ffx2Only::TopMenuCursor,0x12bef0cu},
        {ffx::Ffx2Only::TopMenuItems,0x12befe0u},{ffx::Ffx2Only::TopMenuCount,0x12bf040u},
        {ffx::Ffx2Only::HelpText,0x12bd97cu},{ffx::Ffx2Only::Charset,0x9eade0u},
        {ffx::Ffx2Only::StoryMode,0x12ef654u},
        {ffx::Ffx2Only::ConfigInput,0x388c40u},{ffx::Ffx2Only::ConfigCursor,0x12d5f38u},
        {ffx::Ffx2Only::ConfigTable,0x12d5f40u},{ffx::Ffx2Only::ConfigCount,0x12d5f48u},
        {ffx::Ffx2Only::ConfigMainTable,0x9638b4u},{ffx::Ffx2Only::MenuTextSpaces,0xa103a8u}};
    for(const auto& entry:only) {
        unsigned resolved=0;
        try { resolved=ffx::ffx2Rva(entry.name); } catch(const std::out_of_range&) {}
        if(resolved!=entry.rva) {
            std::fprintf(stderr, "FFX-2-only address %u resolved to 0x%x, expected 0x%x.\n", static_cast<unsigned>(entry.name), resolved, entry.rva);
            return 1;
        }
    }
    if(ffx::ffx2HookId(0x373170u)==0x373170u || ffx::resourceScopeHook(ffx::ffx2HookId(0x373170u))) {
        std::fprintf(stderr, "FFX-2-only hook ids must not reuse the FFX RVA space or open resource scopes.\n");
        return 1;
    }
    for(const auto profile:{ffx::NativeProfile::Reference,ffx::NativeProfile::Installed,ffx::NativeProfile::MovieCaptions}) {
        ffx::nativeProfile=profile;
        bool refused=false;
        try { (void)ffx::ffx2Rva(ffx::Ffx2Only::TopMenuUpdate); } catch(const std::out_of_range&) { refused=true; }
        if(!refused) {
            std::fprintf(stderr, "FFX-2-only address resolved under profile %s.\n", ffx::nativeProfileName(profile));
            return 1;
        }
    }
    // FFX tables must not change. Reference passes every RVA through; the
    // installed FFX.exe keeps its static pairs (probe and DirectInput RVAs are
    // signature extensions there, resolved at runtime).
    ffx::nativeProfile=ffx::NativeProfile::Reference;
    for(const unsigned rva:{0x225930u,0x22d770u,0x22db80u,0x471d10u,0x46fed0u}) if(ffx::profileRva(rva)!=rva) return 1;
    ffx::nativeProfile=ffx::NativeProfile::Installed;
    if(ffx::profileRva(0x471d10u)!=0x471d70u || ffx::profileRva(0x46fed0u)!=0x46ff40u) {
        std::fprintf(stderr, "The installed FFX address table changed.\n");
        return 1;
    }
    for(const auto profile:{ffx::NativeProfile::Reference,ffx::NativeProfile::Installed,ffx::NativeProfile::MovieCaptions}) {
        ffx::nativeProfile=profile;
        if(!ffx::resourceScopeHook(0x46fed0u) || !ffx::resourceScopeHook(0x5093b0u) || !ffx::resourceScopeHook(0x48dfe0u) ||
           ffx::resourceScopeHook(0x225930u) || ffx::gameThreadProbe()!=0x225930u) {
            std::fprintf(stderr, "An FFX resource scope or the game-thread probe changed for profile %s.\n", ffx::nativeProfileName(profile));
            return 1;
        }
    }
    ffx::nativeProfile=ffx::NativeProfile::Reference;
    std::puts("Native executable hash guard passed: unsupported fixture refused, FFX-2 selected with only verified input and load-scope addresses.");
    return 0;
}
