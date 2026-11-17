#pragma once
// Test hosts must never leave a modal crash dialog over the player's game.
// Run before C++ static constructors, including external evidence loading.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#pragma push_macro("small")
#include <windows.h>
#include <werapi.h>
#pragma pop_macro("small")
#include <cstdlib>
#pragma comment(lib, "wer.lib")

namespace ffx_fixture {
inline int initializeRuntime() noexcept {
    SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _set_error_mode(_OUT_TO_STDERR);
    WerSetFlags(WER_FAULT_REPORTING_NO_UI);
    return 0;
}
inline void initializeBeforeCpp() noexcept {initializeRuntime();}
}
#pragma section(".CRT$XCT", long, read)
extern "C" {
__declspec(allocate(".CRT$XCT")) __declspec(selectany)
extern void (__cdecl* const ffxFixtureRuntimeInitializer)() = ffx_fixture::initializeBeforeCpp;
}
// Keep the COMDAT even with /OPT:REF; a header-only initializer has no caller.
#ifdef _M_IX86
#pragma comment(linker, "/include:_ffxFixtureRuntimeInitializer")
#else
#pragma comment(linker, "/include:ffxFixtureRuntimeInitializer")
#endif
