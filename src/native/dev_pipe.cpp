// Development test pipe for scripted live tests. Local clients only: the pipe
// rejects remote clients and its DACL admits only the creating user and
// SYSTEM. One client at a time.
//
// Protocol: UTF-8, one request line and exactly one reply line, both ending
// in "\n". A reply starts with "ok" or "err <kind>: <reason>". Multi-line text
// is joined with " | ".
//   ping                     ok frame=<native frames pumped> seq=<last event>
//   seq                      ok seq=<last event>
//   wait <regex> <ms> [since=<seq>]
//                            ok <seq> <channel> <text>, or err timeout: ...
//                            Matches "<channel> <text>" of events after since
//                            (default: events after the request arrived).
//                            Speech events are "speech <producer> <text>".
//   focus hold on|off        treat the game window as focused (dev only)
//   guide step|next|previous|expected|path|jump <n>   aliases of puzzle ...
//   select <text>, walk <text>   pick (and walk to) the first destination whose
//                            label contains text; "target" event names it
//   scene dump <name>        write the next navigation Scene to
//                            %FFX_EVIDENCE_ROOT%\scenes\<name>.ffxscene; "scene" event
//   Events: speech (spoken text), step (guide step), target, action (command
//   run), key (mod hotkey), scene. Scripts match these, never speech wording.
//   help, help console, warps ...    answered on the pipe thread
//   anything else            a console command run on the native thread
//                            (the next native input callback). A command
//                            that speaks its result replies
//                            "ok queued seq=<n>": wait since=<n> for it.
// Native commands are never cancelled by a game context change. A command
// that cannot run yet waits (up to 5 s) and then replies
// "err busy: <reason>", or "err timeout: game thread not pumping".
#ifdef FFX_DEV_UTILITIES
#include "dev_pipe.hpp"
#include "utilities.hpp"
#include "utility_context.hpp"
#include "utility_memory.hpp"
#include "utility_scripts.hpp"
#include "utility_warps.hpp"
#include "atel_resources.hpp"
#include "events.hpp"
#include "mod_menu.hpp"
#include "settings.hpp"
#include "log_capture.hpp"
#include "navigation.hpp"
#include "nonblocking_snapshots.hpp"
#include <windows.h>
#include <sddl.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cwctype>
#include <regex>
#include <vector>
namespace ffx {
namespace {
constexpr unsigned RingSize=512;
struct Entry {unsigned seq=0;char channel[16]{};char text[496]{};};
Entry ring[RingSize];
SRWLOCK ringLock=SRWLOCK_INIT;
std::atomic<unsigned> lastSequence{0};
struct GameEvent {char channel[16];char text[240];};
async::TryCaptureQueue<GameEvent,64> gameEvents; // Native producer only.
std::atomic<unsigned> droppedGameEvents{0};
std::atomic<unsigned> frames{0};

void copyText(char* out,size_t size,const char* text) {
    size_t i=0;if(text) for(;text[i] && i+1<size;++i) out[i]=text[i]=='\r' || text[i]=='\n'?' ':text[i];
    out[i]=0;
}
void append(const char* channel,const char* text) noexcept {
    AcquireSRWLockExclusive(&ringLock);
    const auto seq=lastSequence.load(std::memory_order_relaxed)+1;
    auto& entry=ring[seq%RingSize];entry.seq=seq;
    copyText(entry.channel,sizeof(entry.channel),channel);copyText(entry.text,sizeof(entry.text),text);
    lastSequence.store(seq,std::memory_order_release);
    ReleaseSRWLockExclusive(&ringLock);
}
void drainGameEvents() {GameEvent event;while(gameEvents.tryPop(event)) append(event.channel,event.text);}

std::string utf8(std::wstring_view text) {
    if(text.empty()) return {};
    const int size=WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    std::string out(size>0?size:0,'\0');
    if(size>0) WideCharToMultiByte(CP_UTF8,0,text.data(),static_cast<int>(text.size()),out.data(),size,nullptr,nullptr);
    return out;
}
std::wstring wide(std::string_view text) {
    if(text.empty()) return {};
    const int size=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring out(size>0?size:0,L'\0');
    if(size>0) MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),out.data(),size);
    return out;
}
// One reply line: newlines become " | ".
std::string flatten(std::wstring_view text) {
    std::wstring out;bool joined=false;
    for(const auto c:text) {
        if(c==L'\r' || c==L'\n') {if(!joined && !out.empty()) out+=L" | ";joined=true;continue;}
        joined=false;out+=c;
    }
    while(out.size()>=3 && out.compare(out.size()-3,3,L" | ")==0) out.resize(out.size()-3);
    return utf8(out);
}
std::string_view trim(std::string_view text) {
    while(!text.empty() && (text.front()==' ' || text.front()=='\t' || text.front()=='\r' || text.front()=='\n')) text.remove_prefix(1);
    while(!text.empty() && (text.back()==' ' || text.back()=='\t' || text.back()=='\r' || text.back()=='\n')) text.remove_suffix(1);
    return text;
}
bool number(std::string_view text,unsigned& value) {
    if(text.empty() || text.size()>9) return false;value=0;
    for(const auto c:text) {if(c<'0' || c>'9') return false;value=value*10+static_cast<unsigned>(c-'0');}
    return true;
}

// Native command crossing. The pipe thread is the only producer of commands
// and the only consumer of replies; the native thread the reverse.
enum class Block : unsigned {None,Console,Menu,Field,Mode,Focus,Preparation,Busy};
const char* blockText(Block block) {
    switch(block) {
    case Block::Console:return "utility console open";
    case Block::Menu:return "mod menu open";
    case Block::Field:return "not in field control";
    case Block::Mode:return "field event, dialogue or menu running";
    case Block::Focus:return "game window not focused (focus hold on allows it)";
    case Block::Preparation:return "field script still loading";
    case Block::Busy:return "another utility request is running";
    default:return "";
    }
}
struct PipeCommand {
    unsigned serial=0;std::array<wchar_t,1024> text{};
    bool mutating=false,force=false,isWarp=false;UtilityWarpRequest warp;
};
struct PipeReply {unsigned serial=0,sequence=0;bool error=false;UtilityText text;};
async::TryCaptureQueue<PipeCommand,4> commands;
async::TryCaptureQueue<PipeReply,4> replies;
std::atomic<unsigned> abandoned{0}; // Highest serial the pipe stopped waiting for.
std::atomic<unsigned> blocked{0};   // Block reason of the retained command.
unsigned nextSerial=0;              // Pipe thread.
// Native thread.
PipeCommand retained;bool haveRetained=false;PipeReply unsent;bool haveUnsent=false;

Block blockFor(const PipeCommand& command) {
    if(utilityConsoleActive()) return Block::Console;
    if(modMenuActive()) return Block::Menu;
    if(!utilityFieldReady()) return Block::Field;
    if(command.mutating) {
        if(!command.force && utilityFieldMode()!=0) return Block::Mode;
        if(!gameFocused() && !devFocusHeld()) return Block::Focus;
        if(utilityBusy()) return Block::Busy;
    }
    if(utilityPreparation()==AtelPreparation::Pending) return Block::Preparation;
    return Block::None;
}
bool mutatingCommand(std::wstring_view command) {
    const auto split=command.find(L' ');const auto verb=command.substr(0,split);
    const auto arguments=split==std::wstring_view::npos?std::wstring_view{}:command.substr(split+1);
    if(verb==L"warp" || verb==L"walk") return true;
    if(verb==L"trial") return arguments!=L"status";
    if(verb==L"var") return arguments.find(L'=')!=std::wstring_view::npos;
    if(verb==L"puzzle") return arguments.substr(0,5)==L"jump ";
    return false;
}
// Same normalization as the console: lowercase, except the text of mark.
std::wstring normalizedCommand(std::string_view line) {
    auto value=wide(line);
    if(value.size()>4 && value.compare(0,5,L"mark ")==0) return value;
    for(auto& c:value) c=static_cast<wchar_t>(std::towlower(c));
    return value;
}
std::string runNative(std::wstring command) {
    if(command.size()>=1024) return "err usage: command is too long";
    PipeCommand packet;packet.serial=++nextSerial;
    packet.mutating=mutatingCommand(command);packet.force=command.rfind(L"warp force ",0)==0;
    packet.isWarp=command.rfind(L"warp ",0)==0;
    if(packet.isWarp) packet.warp=prepareUtilityWarp(std::wstring_view(command).substr(5));
    std::wmemcpy(packet.text.data(),command.c_str(),command.size()+1);
    // Drop any reply left over from an abandoned command.
    PipeReply reply;while(replies.tryPop(reply)) {}
    blocked.store(0,std::memory_order_release);
    if(!commands.tryPush(packet)) return "err busy: pipe command queue full";
    const auto deadline=GetTickCount64()+5000;
    for(;;) {
        drainGameEvents();
        while(replies.tryPop(reply)) {
            if(reply.serial!=packet.serial) continue;
            if(reply.error) return "err busy: "+flatten(renderUtilityText(reply.text));
            const auto text=renderUtilityText(reply.text);
            if(text.empty()) return "ok queued seq="+std::to_string(reply.sequence);
            return "ok "+flatten(text);
        }
        if(GetTickCount64()>=deadline) break;
        Sleep(5);
    }
    abandoned.store(packet.serial,std::memory_order_release);
    const auto reason=static_cast<Block>(blocked.load(std::memory_order_acquire));
    if(reason!=Block::None) return std::string("err busy: ")+blockText(reason);
    return "err timeout: game thread not pumping";
}

// Speech and structured events.
struct Match {unsigned seq;std::string line;};
std::string waitFor(std::string_view arguments) {
    // wait <regex> <ms> [since=<seq>]; the regex may contain spaces.
    arguments=trim(arguments);unsigned since=lastSequence.load(std::memory_order_acquire);
    auto last=arguments.rfind(' ');
    if(last!=std::string_view::npos && arguments.substr(last+1).rfind("since=",0)==0) {
        if(!number(arguments.substr(last+7),since)) return "err usage: since=<seq>";
        arguments=trim(arguments.substr(0,last));last=arguments.rfind(' ');
    }
    unsigned timeout=0;
    if(last==std::string_view::npos || !number(arguments.substr(last+1),timeout)) return "err usage: wait <regex> <ms> [since=<seq>]";
    if(timeout>120000) return "err usage: wait is limited to 120000 ms";
    const auto pattern=trim(arguments.substr(0,last));if(pattern.empty()) return "err usage: wait <regex> <ms> [since=<seq>]";
    std::regex expression;
    try {expression=std::regex(pattern.begin(),pattern.end(),std::regex::ECMAScript);}
    catch(const std::regex_error& error) {return std::string("err usage: bad regex: ")+error.what();}
    const auto deadline=GetTickCount64()+timeout;unsigned scanned=since;
    for(;;) {
        drainGameEvents();
        std::vector<Match> fresh;
        AcquireSRWLockShared(&ringLock);
        const auto current=lastSequence.load(std::memory_order_relaxed);
        if(current>scanned) {
            const auto first=current-scanned>RingSize?current-RingSize+1:scanned+1;
            for(auto seq=first;seq<=current;++seq) {
                const auto& entry=ring[seq%RingSize];
                if(entry.seq==seq) fresh.push_back({seq,std::string(entry.channel)+" "+entry.text});
            }
        }
        ReleaseSRWLockShared(&ringLock);
        if(current>scanned) scanned=current;
        for(const auto& match:fresh) if(std::regex_search(match.line,expression)) return "ok "+std::to_string(match.seq)+" "+match.line;
        if(GetTickCount64()>=deadline) return "err timeout: no match after "+std::to_string(timeout)+" ms (seq="+std::to_string(scanned)+")";
        Sleep(10);
    }
}

const char helpText[]="ok pipe verbs: ping, seq, wait <regex> <ms> [since=<seq>], focus hold on|off, "
    "guide step|next|previous|expected|path|jump <n>, select <text>, walk <text>, scene dump <name>, help console; "
    "other lines run as console commands";
}

void devPipeEvent(const char* channel,const char* text) noexcept {
    if(nativeGameThread()) {
        GameEvent event;copyText(event.channel,sizeof(event.channel),channel);copyText(event.text,sizeof(event.text),text);
        if(!gameEvents.tryPush(event)) droppedGameEvents.fetch_add(1,std::memory_order_relaxed);
        return;
    }
    append(channel,text);
}
void devSpeechTap(const char* producer,const char* text) noexcept {
    char line[496];std::snprintf(line,sizeof(line),"%s %s",producer && *producer?producer:"-",text?text:"");
    append("speech",line);
}
unsigned devPipeSequence() noexcept {return lastSequence.load(std::memory_order_acquire);}

std::string devPipeHandleLine(const std::string& request) {
    const auto line=trim(request);
    if(line.empty()) return "err usage: empty line";
    const auto split=line.find(' ');
    std::string verb(line.substr(0,split));for(auto& c:verb) c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    const auto arguments=split==std::string_view::npos?std::string_view{}:trim(line.substr(split+1));
    drainGameEvents();
    if(verb=="ping") return "ok frame="+std::to_string(frames.load(std::memory_order_acquire))+" seq="+std::to_string(devPipeSequence());
    if(verb=="seq") return "ok seq="+std::to_string(devPipeSequence());
    if(verb=="wait") return waitFor(arguments);
    if(verb=="focus") {
        if(arguments=="hold on") {setDevFocusHold(true);return "ok focus hold on";}
        if(arguments=="hold off") {setDevFocusHold(false);return "ok focus hold off";}
        if(arguments.empty()) return std::string("ok focus hold ")+(devFocusHeld()?"on":"off");
        return "err usage: focus hold on|off";
    }
    if(verb=="help") {
        if(arguments.empty()) return helpText;
        if(arguments=="console") return "ok "+flatten(executeUtility(L"help"));
        return "err usage: help or help console";
    }
    if(verb=="warps") return "ok "+flatten(executeUtility(normalizedCommand(line)));
    if(verb=="guide") {
        if(arguments.empty()) return "err usage: guide step|next|previous|expected|path|jump <n>";
        return runNative(normalizedCommand("puzzle "+std::string(arguments)));
    }
    if(verb=="scene") {
        // scene dump <name>: the name becomes a file name, so keep it plain.
        if(arguments.rfind("dump ",0)!=0) return "err usage: scene dump <name>";
        const auto name=trim(arguments.substr(5));
        const bool plain=!name.empty() && name.size()<=64 && name.find("..")==std::string_view::npos &&
            std::all_of(name.begin(),name.end(),[](char c) {return std::isalnum(static_cast<unsigned char>(c)) || c=='-' || c=='_' || c=='.';});
        if(!plain) return "err usage: scene dump <name> (letters, digits, - _ . only, at most 64)";
    }
    return runNative(normalizedCommand(line));
}

void devPipeClientGone() {setDevFocusHold(false);}

void pumpDevPipe() {
    if(!nativeGameThread()) return;
    frames.fetch_add(1,std::memory_order_acq_rel);
    if(haveUnsent) {if(!replies.tryPush(unsent)) return;haveUnsent=false;}
    if(!haveRetained) {if(!commands.tryPop(retained)) return;haveRetained=true;}
    if(retained.serial<=abandoned.load(std::memory_order_acquire)) {haveRetained=false;blocked.store(0,std::memory_order_release);return;}
    const auto block=blockFor(retained);
    if(block!=Block::None) {blocked.store(static_cast<unsigned>(block),std::memory_order_release);return;}
    blocked.store(0,std::memory_order_release);
    PipeReply reply;reply.serial=retained.serial;reply.sequence=devPipeSequence();
    {
        char event[300];const auto text=utf8(retained.text.data());
        std::snprintf(event,sizeof(event),"run %s",text.c_str());devPipeEvent("action",event);
    }
    try {
        const std::wstring_view command(retained.text.data());
        const bool walk=command.rfind(L"walk ",0)==0;
        if(command.rfind(L"scene dump ",0)==0) {
            if(!devNavigationDump(retained.text.data()+11)) reply.text=L"Navigation is not ready.";
        } else if(walk || command.rfind(L"select ",0)==0) {
            // The navigation worker answers with speech and a "target" event.
            if(!devNavigationSelect(retained.text.data()+(walk?5:7),walk)) reply.text=L"Navigation is not ready.";
        } else reply.text=retained.isWarp?performUtilityWarp(retained.warp):executeUtilityNative(command);
    } catch(const std::exception& error) {
        reply.text=utilityError(L"Utility stopped: ",error.what());recordLog("dev pipe command failed: %s",error.what());
    } catch(...) {reply.text=L"Utility stopped after a native error.";recordLog("dev pipe native exception");}
    haveRetained=false;
    if(!replies.tryPush(reply)) {unsent=reply;haveUnsent=true;}
}

namespace {
HANDLE ioEvent=nullptr;
// Waits for overlapped I/O while it keeps the event ring fed.
bool finish(HANDLE pipe,OVERLAPPED& overlapped,BOOL started,DWORD& bytes) {
    if(!started && GetLastError()!=ERROR_IO_PENDING) return false;
    while(WaitForSingleObject(overlapped.hEvent,20)==WAIT_TIMEOUT) drainGameEvents();
    return GetOverlappedResult(pipe,&overlapped,&bytes,FALSE)!=FALSE;
}
bool writeLine(HANDLE pipe,const std::string& reply) {
    const auto line=reply+"\n";OVERLAPPED overlapped{};overlapped.hEvent=ioEvent;DWORD bytes=0;
    const auto started=WriteFile(pipe,line.data(),static_cast<DWORD>(line.size()),nullptr,&overlapped);
    return finish(pipe,overlapped,started,bytes) && bytes==line.size();
}
void serveClient(HANDLE pipe) {
    std::string pending;char buffer[1024];
    for(;;) {
        OVERLAPPED overlapped{};overlapped.hEvent=ioEvent;DWORD bytes=0;
        const auto started=ReadFile(pipe,buffer,sizeof(buffer),nullptr,&overlapped);
        if(!finish(pipe,overlapped,started,bytes) || !bytes) return;
        pending.append(buffer,bytes);
        for(auto end=pending.find('\n');end!=std::string::npos;end=pending.find('\n')) {
            const auto request=pending.substr(0,end);pending.erase(0,end+1);
            if(!writeLine(pipe,devPipeHandleLine(request))) return;
        }
        if(pending.size()>8192) {pending.clear();if(!writeLine(pipe,"err usage: line too long")) return;}
    }
}
DWORD WINAPI pipeThread(void*) {
    wchar_t name[64];std::swprintf(name,64,L"\\\\.\\pipe\\ffx-sr-dev-%lu",GetCurrentProcessId());
    PSECURITY_DESCRIPTOR descriptor=nullptr;
    // Protected DACL: the creating user (owner rights) and SYSTEM only.
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;;GA;;;OW)(A;;GA;;;SY)",SDDL_REVISION_1,&descriptor,nullptr)) {
        recordLog("dev pipe security descriptor failed error=%lu",GetLastError());return 0;
    }
    SECURITY_ATTRIBUTES security{sizeof(security),descriptor,FALSE};
    const auto pipe=CreateNamedPipeW(name,PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED|FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS,1,4096,4096,0,&security);
    LocalFree(descriptor);
    if(pipe==INVALID_HANDLE_VALUE) {recordLog("dev pipe unavailable error=%lu",GetLastError());return 0;}
    recordLog("dev pipe listening %ls",name);
    for(;;) {
        OVERLAPPED overlapped{};overlapped.hEvent=ioEvent;DWORD bytes=0;
        const auto started=ConnectNamedPipe(pipe,&overlapped);
        const bool connected=started || GetLastError()==ERROR_PIPE_CONNECTED || finish(pipe,overlapped,started,bytes);
        if(connected) {serveClient(pipe);devPipeClientGone();}
        else Sleep(100);
        DisconnectNamedPipe(pipe);
    }
}
}
void startDevPipe() {
    if(ioEvent) return;
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&pipeThread),&pin)) return;
    ioEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!ioEvent) return;
    const auto thread=CreateThread(nullptr,0,pipeThread,nullptr,0,nullptr);
    if(thread) CloseHandle(thread);else recordLog("dev pipe thread unavailable error=%lu",GetLastError());
}
}
#endif
