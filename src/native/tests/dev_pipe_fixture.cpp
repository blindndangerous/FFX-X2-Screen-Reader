#include "fixture_runtime.hpp"
#include "../dev_pipe.hpp"
#include "../utilities.hpp"
#include "../utility_context.hpp"
#include "../utility_memory.hpp"
#include "../utility_scripts.hpp"
#include "../utility_warps.hpp"
#include "../atel_resources.hpp"
#include "../events.hpp"
#include "../mod_menu.hpp"
#include "../settings.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <thread>

// Development pipe: request parsing, the speech tap and event ring, the
// native crossing (never cancelled by a context change) and one real local
// pipe round trip. Game state is synthetic.
namespace {
DWORD mainThread=GetCurrentThreadId();
std::atomic<bool> consoleOpen{false},menuOpen{false},fieldReady{true},focused{true},busy{false};
std::atomic<int> mode{0};
std::atomic<bool> pending{false};
std::wstring lastNative;
unsigned nativeRuns=0,checks=0;
void check(bool ok,const char* why) {++checks;if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
void checkText(const std::string& actual,const std::string& expected,const char* why) {
    if(actual!=expected) std::fprintf(stderr,"expected [%s]\nactual   [%s]\n",expected.c_str(),actual.c_str());
    check(actual==expected,why);
}
bool starts(const std::string& text,const char* prefix) {return text.rfind(prefix,0)==0;}
}
namespace ffx {
bool nativeGameThread() {return GetCurrentThreadId()==mainThread;}
bool utilityConsoleActive() {return consoleOpen;}
bool modMenuActive() {return menuOpen;}
bool utilityFieldReady() {return fieldReady;}
int utilityFieldMode() {return mode;}
void setDevFocusHold(bool on) noexcept {devFocusHoldFlag=on;}
bool gameFocused() {return focused;}
bool utilityBusy() {return busy;}
AtelPreparation utilityPreparation(uintptr_t*) {return pending?AtelPreparation::Pending:AtelPreparation::Ready;}
std::wstring executeUtility(const std::wstring& command) {
    return command==L"help"?L"Console help.\r\nSecond line.\r\n":L"Warps: one\r\ntwo";
}
UtilityText executeUtilityNative(std::wstring_view command) {
    check(nativeGameThread(),"pipe commands run on the native thread");
    lastNative=command;++nativeRuns;
    if(command==L"pos") return L"Position 1, 2, 3.";
    if(command.rfind(L"puzzle ",0)==0) return {}; // Spoken by the guide worker.
    if(command==L"throw") throw std::runtime_error("boom");
    return UtilityText(L"Ran ").append(UtilityText(std::wstring(command).c_str()));
}
UtilityWarpRequest prepareUtilityWarp(std::wstring_view) {return {};}
std::wstring lastSelect;bool lastWalk=false;
bool devNavigationSelect(const wchar_t* text,bool walk) {check(nativeGameThread(),"select runs on the native thread");lastSelect=text;lastWalk=walk;return text[0]!=L'!';}
UtilityText performUtilityWarp(const UtilityWarpRequest&) {return L"Warped.";}
std::wstring lastDump;
bool devNavigationDump(const wchar_t* name) {check(nativeGameThread(),"scene dump runs on the native thread");lastDump=name;return true;}
}

namespace {
using namespace ffx;
// Runs a request on a pipe-like thread while this (native) thread pumps.
std::string pumped(const std::string& request,unsigned frames=100000,unsigned holdFrames=0,std::atomic<bool>* release=nullptr) {
    std::string reply;std::atomic<bool> done{false};
    std::thread pipe([&] {reply=devPipeHandleLine(request);done=true;});
    for(unsigned frame=0;!done && frame<frames;++frame) {
        if(release && frame==holdFrames) *release=true;
        pumpDevPipe();Sleep(1);
    }
    pipe.join();
    return reply;
}
std::string readLine(HANDLE pipe) {
    std::string line;char c=0;DWORD bytes=0;
    while(ReadFile(pipe,&c,1,&bytes,nullptr) && bytes==1 && c!='\n') line+=c;
    return line;
}
bool writeLine(HANDLE pipe,const std::string& line) {
    const auto text=line+"\n";DWORD bytes=0;
    return WriteFile(pipe,text.data(),static_cast<DWORD>(text.size()),&bytes,nullptr) && bytes==text.size();
}

void parsing() {
    checkText(devPipeHandleLine(""),"err usage: empty line","empty line");
    check(starts(devPipeHandleLine("ping"),"ok frame="),"ping replies with the frame counter");
    check(starts(devPipeHandleLine("  SEQ \r"),"ok seq="),"verbs are trimmed and case-insensitive");
    checkText(devPipeHandleLine("focus hold maybe"),"err usage: focus hold on|off","focus usage");
    checkText(devPipeHandleLine("focus hold on"),"ok focus hold on","focus hold on");
    check(devFocusHeld(),"focus hold sets the shared flag");
    checkText(devPipeHandleLine("focus"),"ok focus hold on","focus status");
    checkText(devPipeHandleLine("focus hold off"),"ok focus hold off","focus hold off");
    checkText(devPipeHandleLine("wait"),"err usage: wait <regex> <ms> [since=<seq>]","wait needs a timeout");
    checkText(devPipeHandleLine("wait Step"),"err usage: wait <regex> <ms> [since=<seq>]","wait needs ms");
    checkText(devPipeHandleLine("wait x 999999"),"err usage: wait is limited to 120000 ms","wait limit");
    checkText(devPipeHandleLine("wait x 10 since=abc"),"err usage: since=<seq>","since must be a number");
    check(starts(devPipeHandleLine("wait ([ 10"),"err usage: bad regex"),"bad regex is reported, not thrown");
    checkText(devPipeHandleLine("help console"),"ok Console help. | Second line.","console help is one line");
    checkText(devPipeHandleLine("warps"),"ok Warps: one | two","warps are one line");
    checkText(devPipeHandleLine("guide"),"err usage: guide step|next|previous|expected|path|jump <n>","guide usage");
}

void speechTap() {
    const auto before=devPipeSequence();
    std::thread speech([] {devSpeechTap("puzzle-guide","Step 4 of 36. Insert the sphere.");});speech.join();
    check(devPipeSequence()==before+1,"speech tap takes one sequence number");
    const auto since=" since="+std::to_string(before);
    checkText(devPipeHandleLine("wait ^speech puzzle-guide Step 4 of 36 100"+since),
        "ok "+std::to_string(before+1)+" speech puzzle-guide Step 4 of 36. Insert the sphere.","wait finds earlier speech since seq");
    check(starts(devPipeHandleLine("wait Step 4 30"),"err timeout: no match after 30 ms"),"default since skips old speech");
    // A native-thread event crosses the lock-free queue, not the ring lock.
    devPipeEvent("step","script=djyt0600 step=5 total=36 expected=5 action=0");
    checkText(devPipeHandleLine("wait ^step script=djyt0600 step=5 50 since="+std::to_string(before+1)),
        "ok "+std::to_string(before+2)+" step script=djyt0600 step=5 total=36 expected=5 action=0","native step event");
    std::thread later([] {Sleep(30);devSpeechTap("utility-reply","Line one\r\nline two");});
    const auto reply=devPipeHandleLine("wait utility-reply 2000");later.join();
    check(starts(reply,"ok ") && reply.find("speech utility-reply Line one  line two")!=std::string::npos,"wait sees speech that arrives while waiting; newlines removed");
    // Ring wrap: old entries drop, the newest stay matchable.
    const auto wrapStart=devPipeSequence();
    std::thread flood([] {for(unsigned i=0;i<700;++i) devSpeechTap("flood",std::to_string(i).c_str());});flood.join();
    check(starts(devPipeHandleLine("wait ^speech flood 699$ 50 since="+std::to_string(wrapStart)),"ok "),"newest entry survives the wrap");
    check(starts(devPipeHandleLine("wait ^speech flood 3$ 30 since="+std::to_string(wrapStart)),"err timeout"),"overwritten entry is gone");
}

void nativeCrossing() {
    checkText(pumped("pos"),"ok Position 1, 2, 3.","native reply is one line");
    check(lastNative==L"pos","command text crosses unchanged");
    checkText(pumped("POS"),"ok Position 1, 2, 3.","native commands are lowercased like the console");
    const auto queued=pumped("guide step");
    check(lastNative==L"puzzle step","guide aliases puzzle");
    check(starts(queued,"ok queued seq="),"spoken result replies queued with the speech sequence");
    pumped("mark Pedestal A");check(lastNative==L"mark Pedestal A","mark keeps its text case");
    check(starts(pumped("throw"),"ok Utility stopped: boom"),"native exception becomes a reply");
    // A context change (leaving field, then a mode change) does not cancel a
    // waiting command: it runs as soon as the field is ready again.
    std::atomic<bool> release{false};
    fieldReady=false;auto runs=nativeRuns;
    std::thread restore([&] {while(!release) Sleep(1);fieldReady=true;});
    checkText(pumped("pos",100000,40,&release),"ok Position 1, 2, 3.","command waits through a context change");
    restore.join();check(nativeRuns==runs+1,"waiting command ran once");
    // Mutating verbs need mode 0 and focus (or focus hold).
    mode=3;runs=nativeRuns;
    checkText(pumped("trial on",6000),"err busy: field event, dialogue or menu running","mutating command reports mode block");
    mode=0;for(unsigned i=0;i<5;++i) pumpDevPipe();
    check(nativeRuns==runs,"abandoned command never runs later");
    checkText(pumped("trial status"),"ok Ran trial status","read-only trial ignores mode");
    focused=false;
    checkText(pumped("var 0x10=1",6000),"err busy: game window not focused (focus hold on allows it)","mutation needs focus");
    devPipeHandleLine("focus hold on");
    checkText(pumped("var 0x10=1"),"ok Ran var 0x10=1","focus hold allows mutation");
    devPipeClientGone();check(!devFocusHeld(),"disconnect clears focus hold");
    focused=true;consoleOpen=true;
    checkText(pumped("pos",6000),"err busy: utility console open","open console blocks");
    consoleOpen=false;pending=true;
    checkText(pumped("pos",6000),"err busy: field script still loading","pending preparation blocks");
    pending=false;
    checkText(devPipeHandleLine("pos"),"err timeout: game thread not pumping","no native pump times out");
    runs=nativeRuns;for(unsigned i=0;i<5;++i) pumpDevPipe();
    check(nativeRuns==runs,"timed-out command is dropped");
    checkText(pumped("pos"),"ok Position 1, 2, 3.","pipe recovers after a timeout");
    // select/walk go to the navigation owner; the worker answers with a target event.
    check(starts(pumped("select Save Sphere"),"ok queued seq="),"select is queued");
    check(lastSelect==L"save sphere" && !lastWalk,"select passes the label text");
    check(starts(pumped("walk Lift"),"ok queued seq=") && lastSelect==L"lift" && lastWalk,"walk passes the label text");
    checkText(pumped("select !"),"ok Navigation is not ready.","select reports navigation not ready");
    mode=2;checkText(pumped("walk lift",6000),"err busy: field event, dialogue or menu running","walk is a mutating verb");mode=0;
    for(unsigned i=0;i<5;++i) pumpDevPipe();
    mode=2;check(starts(pumped("select lift"),"ok queued"),"select is read-only and ignores mode");mode=0;
    // scene dump: the name is a file name and is checked on the pipe thread.
    check(starts(pumped("scene dump Djose-Lift_1"),"ok queued seq=") && lastDump==L"djose-lift_1","scene dump passes the name");
    check(starts(devPipeHandleLine("scene dump ../x"),"err usage: scene dump"),"scene dump rejects a path");
    check(starts(devPipeHandleLine("scene dump a/b"),"err usage: scene dump"),"scene dump rejects a separator");
    check(starts(devPipeHandleLine("scene"),"err usage: scene dump"),"scene needs dump");
}

void realPipe() {
    startDevPipe();
    wchar_t name[64];std::swprintf(name,64,L"\\\\.\\pipe\\ffx-sr-dev-%lu",GetCurrentProcessId());
    HANDLE pipe=INVALID_HANDLE_VALUE;
    for(unsigned i=0;i<200 && pipe==INVALID_HANDLE_VALUE;++i) {
        pipe=CreateFileW(name,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
        if(pipe==INVALID_HANDLE_VALUE) Sleep(10);
    }
    check(pipe!=INVALID_HANDLE_VALUE,"local client connects to the pipe");
    check(writeLine(pipe,"ping"),"write ping");
    check(starts(readLine(pipe),"ok frame="),"ping round trip");
    check(writeLine(pipe,"seq\nfocus hold on"),"two requests in one write");
    check(starts(readLine(pipe),"ok seq="),"first of two requests");
    checkText(readLine(pipe),"ok focus hold on","second of two requests");
    // The fixture main thread is the native thread: it pumps below while a
    // client thread waits for the reply.
    check(writeLine(pipe,"pos"),"write native command");
    std::string reply;std::atomic<bool> got{false};
    std::thread reader([&] {reply=readLine(pipe);got=true;});
    for(unsigned i=0;i<5000 && !got;++i) {pumpDevPipe();Sleep(1);}
    reader.join();
    checkText(reply,"ok Position 1, 2, 3.","native command over the real pipe");
    CloseHandle(pipe);
    for(unsigned i=0;i<200 && devFocusHeld();++i) Sleep(10);
    check(!devFocusHeld(),"client disconnect clears focus hold");
    pipe=INVALID_HANDLE_VALUE;
    for(unsigned i=0;i<200 && pipe==INVALID_HANDLE_VALUE;++i) {
        pipe=CreateFileW(name,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
        if(pipe==INVALID_HANDLE_VALUE) Sleep(10);
    }
    check(pipe!=INVALID_HANDLE_VALUE,"a second client connects after the first leaves");
    check(writeLine(pipe,"seq") && starts(readLine(pipe),"ok seq="),"second client round trip");
    CloseHandle(pipe);
}
}

int main(int argc,char** argv) {
    if(argc==3 && std::string(argv[1])=="--serve") {
        // Manual client check (.local\devpipe.ps1): serve the synthetic game for N ms.
        startDevPipe();const auto until=GetTickCount64()+std::strtoul(argv[2],nullptr,10);
        for(ULONGLONG speak=0;GetTickCount64()<until;) {
            if(GetTickCount64()>=speak) {devSpeechTap("fixture-speech","Synthetic speech line.");speak=GetTickCount64()+250;}
            pumpDevPipe();Sleep(1);
        }
        return 0;
    }
    parsing();
    speechTap();
    nativeCrossing();
    realPipe();
    std::printf("dev pipe fixture: %u checks passed\n",checks);
    return 0;
}
