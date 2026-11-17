#include "fixture_runtime.hpp"
#include <cstdio>
#include <string>
#include <stdexcept>
#include <cassert>

namespace {
constexpr DWORD RequiredMode=SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX;
DWORD earlyMode=GetErrorMode();
unsigned earlyAbort=_set_abort_behavior(0,0);
int earlyCrtOutput=_set_error_mode(_REPORT_ERRMODE);
DWORD earlyWer=0;
struct BeforeMain {
    BeforeMain() {
        WerGetFlags(GetCurrentProcess(),&earlyWer);
        wchar_t value[8]{};
        if(GetEnvironmentVariableW(L"FFX_FIXTURE_STATIC_ABORT",value,8)) std::abort();
    }
} beforeMain;
void check(bool ok,const char* why) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
}
int wmain(int argc,wchar_t** argv) {
    if(argc>1) {
        if(!std::wcscmp(argv[1],L"abort")) std::abort();
        if(!std::wcscmp(argv[1],L"throw")) throw std::runtime_error("fixture intentional terminate");
        if(!std::wcscmp(argv[1],L"assert")) {assert(false && "fixture intentional assertion");return 2;}
        auto* page=static_cast<volatile unsigned*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READONLY));
        if(!page) return 2;
        *page=1;return 2;
    }
    check((earlyMode&RequiredMode)==RequiredMode,"error dialog suppression precedes C++ static initialization");
    check(!(earlyAbort&(_WRITE_ABORT_MSG|_CALL_REPORTFAULT)),"CRT abort UI and report-fault paths disabled before main");
    check((earlyWer&WER_FAULT_REPORTING_NO_UI)!=0,"WER has process-local no-UI flag before main");
    check(earlyCrtOutput==_OUT_TO_STDERR,"CRT assertions use stderr, never a message box");
    wchar_t executable[32768]{};check(GetModuleFileNameW(nullptr,executable,32768)!=0,"fixture executable path");
    unsigned cases=0;
    for(const wchar_t* mode:{L"av",L"abort",L"throw",L"static",L"assert"}) {
        std::wstring command=L"\""+std::wstring(executable)+L"\" "+mode;
        if(!std::wcscmp(mode,L"static")) SetEnvironmentVariableW(L"FFX_FIXTURE_STATIC_ABORT",L"1");
        // Prove the child's embedded initializer, not the runner's inheritance.
        SetErrorMode(0);
        STARTUPINFOW startup{};startup.cb=sizeof(startup);PROCESS_INFORMATION child{};
        const bool started=CreateProcessW(executable,command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&child)!=FALSE;
        ffx_fixture::initializeRuntime();SetEnvironmentVariableW(L"FFX_FIXTURE_STATIC_ABORT",nullptr);
        check(started,"start deliberate failing child without desktop focus");
        const auto wait=WaitForSingleObject(child.hProcess,5000);
        if(wait!=WAIT_OBJECT_0) {TerminateProcess(child.hProcess,2);WaitForSingleObject(child.hProcess,5000);}
        DWORD code=0;GetExitCodeProcess(child.hProcess,&code);CloseHandle(child.hThread);CloseHandle(child.hProcess);
        check(wait==WAIT_OBJECT_0,"failing fixture exits without a blocking Windows error dialog");
        check(code==(!std::wcscmp(mode,L"av")?0xc0000005u:3u),"failure exit status remains visible to the test runner");
        ++cases;
    }
    std::printf("PASS: fixture runtime pre-main error mode, CRT/WER suppression and %u headless failure subprocesses.\n",cases);
}
