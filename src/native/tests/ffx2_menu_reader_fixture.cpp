// Synthetic memory boundary: exercise the actual hook and preparation receiver.
#include "../x2_boot.hpp"
#include "../events.hpp"
#include "../profile.hpp"
#include "../speech_domains.hpp"
#include <cstdio>
#include <cstring>
#include <map>

namespace {
int checks=0,failures=0;
bool nativeOwner=true,queueAccepted=true;
unsigned nativeOutputs=0,epoch=7;
ffx::Handler enter=nullptr,leave=nullptr;
ffx::speechq::DomainHandler receiver=nullptr;
ffx::speechq::Header anchor{};
std::wstring packet,spoken;
std::map<uintptr_t,std::vector<unsigned char>> memory;
void check(bool ok,const char* name) {++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
template<class T> void put(uintptr_t at,T value) {
    auto& bytes=memory[at];bytes.resize(sizeof value);std::memcpy(bytes.data(),&value,sizeof value);
}
void global(ffx::Ffx2Only key,unsigned value) {put(0x400000+ffx::ffx2Rva(key),value);}
void move(unsigned from,unsigned to) {
    ffx::Event e;e.epoch=epoch;e.args[0]=0x500000;
    global(ffx::Ffx2Only::TopMenuCursor,from);enter(e);
    global(ffx::Ffx2Only::TopMenuCursor,to);leave(e);
}
void drain() {
    nativeOwner=false;
    if(receiver && !packet.empty()) receiver(anchor,packet);
    packet.clear();nativeOwner=true;
}
}
namespace ffx {
uintptr_t address(unsigned rva) {return 0x400000+rva;}
unsigned generation(){return epoch;}
bool copy(uintptr_t source,void* out,size_t count) {
    check(nativeOwner,"worker never reads game memory");
    for(const auto& entry:memory) if(source>=entry.first && source-entry.first<=entry.second.size() &&
        count<=entry.second.size()-(source-entry.first)) {
        std::memcpy(out,entry.second.data()+source-entry.first,count);return true;
    }
    return false;
}
bool hookAddress(uintptr_t,unsigned,Handler before,Handler after){enter=before;leave=after;return true;}
bool outputAt(const std::wstring&,bool,const char*,unsigned){++nativeOutputs;return true;}
void formatLogCapture(LogCapture&,char*,size_t){}
namespace speechq {
void registerDomain(unsigned,DomainHandler fn,DomainReset){receiver=fn;}
bool captureDomain(unsigned,const void* data,unsigned bytes,const wchar_t*,unsigned,unsigned current) {
    check(nativeOwner,"snapshot enqueued by native owner");
    if(!queueAccepted)return false;
    packet.assign(static_cast<const wchar_t*>(data),bytes/sizeof(wchar_t));anchor.generation=current;return true;
}
bool emitOwned(std::wstring text,const Header&,const char*,bool,bool) {
    check(!nativeOwner,"speech assembled on preparation worker");spoken=std::move(text);return true;
}
}
}
int main() {
    using namespace ffx;
    nativeProfile=NativeProfile::Reference;
    check(!installFfx2TopMenu() && !enter && !receiver,"FFX profile installs no FFX-2 reader or worker");
    nativeProfile=NativeProfile::Ffx2;
    memory[address(ffx2Rva(Ffx2Only::TopMenuUpdate))]={0x55,0x8b,0xec,0x51,0x53,0xe8};
    global(Ffx2Only::TopMenuCount,2);global(Ffx2Only::StoryMode,0);
    std::vector<unsigned> entries(24);entries[2]=9;
    auto& rows=memory[address(ffx2Rva(Ffx2Only::TopMenuItems))];rows.resize(entries.size()*4);
    std::memcpy(rows.data(),entries.data(),rows.size());put(0x50001c,5u);
    global(Ffx2Only::Charset,0x600000);global(Ffx2Only::HelpText,0x610000);
    memory[0x600000]=std::vector<unsigned char>(1024,0);
    std::fill_n(memory[0x600000].begin(),0xd0,'x');memory[0x600000][0]='A';
    memory[0x610000]=std::vector<unsigned char>(256,0);memory[0x610000][0]=0x30;
    check(installFfx2TopMenu(),"verified prologue installs hook");
    check(receiver!=nullptr,"bootstrap registers preparation receiver");
    move(0,1);
    check(nativeOutputs==0,"hook does not decode or emit speech");
    check(!packet.empty(),"hook captures owned charset and help");
    // Releasing/reusing both source buffers cannot change a queued observation.
    memory[0x600000][0]='B';memory[0x610000][0]=0x31;
    drain();check(spoken==L"Config. A","worker uses original snapshot after native memory changes");
    memory[0x610000][0]=0x30;move(1,0);drain();
    check(spoken==L"Items. B","same-address table reload does not reuse a stale charset cache");
    spoken.clear();move(0,0);drain();check(spoken.empty(),"idle cursor stays silent");
    global(Ffx2Only::StoryMode,1);move(0,1);drain();check(spoken.empty(),"non-story mode stays silent");
    global(Ffx2Only::StoryMode,0);put(0x50001c,6u);move(0,1);drain();
    check(spoken.empty(),"other module phases stay silent");put(0x50001c,5u);
    global(Ffx2Only::Charset,0);move(0,1);drain();check(spoken==L"Config","missing charset retains label only");
    global(Ffx2Only::Charset,0x600000);memory[0x600000].assign(1024,0);
    move(1,0);drain();check(spoken==L"Items","malformed or unloaded table retains label only");
    spoken.clear();queueAccepted=false;move(1,0);drain();check(spoken.empty(),"queue pressure drops without native fallback");
    queueAccepted=true;move(0,1);++epoch;drain();check(spoken.empty(),"retired observation stays silent");
    std::printf("FFX-2 reader fixture: %d checks, %d failures.\n",checks,failures);
    return failures?1:0;
}
