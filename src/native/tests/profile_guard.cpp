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
    // FFX-2 has no verified addresses yet. Every FFX hook, probe and data RVA
    // must be refused there, including the reference pass-through.
    ffx::nativeProfile=ffx::NativeProfile::Ffx2;
    for(const unsigned rva:{0x46fed0u,0x5093b0u,0x471d10u,0x4b6260u,0x4e2e20u,0x225930u,0x1fc44e4u}) {
        if(!refusesFfxAddress(rva)) {
            std::fprintf(stderr, "FFX RVA 0x%x resolved under the FFX-2 profile.\n", rva);
            return 1;
        }
    }
    ffx::nativeProfile=ffx::NativeProfile::Reference;
    std::puts("Native executable hash guard passed: unsupported fixture refused, FFX-2 selected without FFX addresses.");
    return 0;
}
