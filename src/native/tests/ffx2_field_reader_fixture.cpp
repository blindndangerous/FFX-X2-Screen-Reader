// Native adapter host with synthetic memory, hooks and speech preparation.
#include "../ffx2_field_text.hpp"
#include "../profile.hpp"
#include "../events.hpp"
#include "../speech_domains.hpp"
#include <cstdio>
#include <cstring>
#include <map>

namespace {
constexpr uintptr_t Base=0x10000000,Window=0x20000000,Text=0x21000000,Charset=0x22000000;
constexpr unsigned Windows=0x18ceb20,Ready=0x39b9f0,Open=0x396280,SetText=0x396fc0,Choice=0x3926e0;
struct Listener {ffx::Handler before,after;};
std::map<unsigned,Listener> hooks;
std::map<uintptr_t,std::vector<uint8_t>> memory;
ffx::speechq::DomainHandler receiver=nullptr;
struct Pending {ffx::speechq::Header header{};std::wstring data;};
std::vector<Pending> pending;
std::vector<std::wstring> spoken;
std::vector<bool> interruptions;
unsigned epoch=1;
bool accept=true;
int failures=0;
void expect(bool value,const char* what) {if(!value) {++failures;std::fprintf(stderr,"FAIL: %s\n",what);}}
template<class T> void put(uintptr_t base,unsigned offset,T value) {std::memcpy(memory[base].data()+offset,&value,sizeof(value));}
void invoke(unsigned rva,uintptr_t argument) {
    const auto it=hooks.find(rva);if(it==hooks.end()) {expect(false,"expected hook installed");return;}
    ffx::Event e;e.args[0]=argument;e.epoch=epoch;e.rva=ffx::ffx2HookId(rva);
    if(it->second.before) it->second.before(e);
    if(it->second.after) it->second.after(e);
}
void drain() {auto packets=std::move(pending);pending.clear();for(const auto& p:packets) if(receiver) receiver(p.header,p.data);}
void image() {
    memory.clear();
    const std::map<unsigned,std::vector<uint8_t>> code={
        {SetText,{0x55,0x8b,0xec,0x83,0x3d}},
        {Open,{0x55,0x8b,0xec,0x56,0x8b,0x75,0x08}},
        {Ready,{0x55,0x8b,0xec,0x81,0xec,0x14,0x01,0x00,0x00}},
        {Choice,{0x55,0x8b,0xec,0x8b,0x55,0x08,0x66,0xa1}},
    };
    for(const auto& entry:code) memory[Base+entry.first]=entry.second;
    memory[Base+Windows].resize(8*4);put<uint32_t>(Base+Windows,0,Window);
    memory[Base+0x9eade0].resize(4);put<uint32_t>(Base+0x9eade0,0,Charset);
    memory[Base+0x12ef654].resize(4);
    memory[Charset].assign(0x80,'x');memory[Charset][0]=' ';memory[Charset][1]='A';memory[Charset][2]='B';memory[Charset].push_back(0);
    memory[Window].resize(0x5a0);memory[Window][0]=1;memory[Window][1]=0;
    put<uint32_t>(Window,0xf4,Text);
    memory[Text]={0x31,0x01,0x32,0};
}
}
namespace ffx {
uintptr_t address(unsigned rva) {return Base+profileRva(rva);}
bool copy(uintptr_t source,void* out,size_t size) {
    for(const auto& entry:memory) if(source>=entry.first && source-entry.first<=entry.second.size() && size<=entry.second.size()-(source-entry.first)) {
        std::memcpy(out,entry.second.data()+(source-entry.first),size);return true;
    }
    return false;
}
unsigned generation() {return epoch;}
void recordLog(const char*,...) {}
bool nativeGameThread() {return true;}
bool hookAddress(uintptr_t target,unsigned id,Handler before,Handler after) {
    expect(target==Base+(id&0x00ffffff),"hook id retains X-2 address");hooks[id&0x00ffffff]={before,after};return true;
}
namespace speechq {
void registerDomain(unsigned,DomainHandler handler,DomainReset) {receiver=handler;}
bool captureDomain(unsigned,const void* data,unsigned bytes,const wchar_t*,unsigned,unsigned capturedEpoch) {
    if(!accept) return false;
    Pending p;p.header.generation=capturedEpoch;p.header.queuedAt=GetTickCount64();
    p.data.assign(static_cast<const wchar_t*>(data),bytes/sizeof(wchar_t));pending.push_back(std::move(p));return true;
}
bool emitOwned(std::wstring text,const Header&,const char*,bool,bool interrupt) {
    if(text.empty()) return false;
    spoken.push_back(std::move(text));interruptions.push_back(interrupt);return true;
}
}
}
int main() {
    using namespace ffx;
    nativeProfile=NativeProfile::Reference;
    expect(!installFfx2FieldText() && hooks.empty(),"FFX profile never installs X-2 field hooks");
    nativeProfile=NativeProfile::Ffx2;image();
#if !FFX2_EXPERIMENTAL_FIELD_TEXT
    expect(!installFfx2FieldText() && hooks.empty() && !receiver,"default build installs no experimental field hooks or worker");
#else
    memory[Base+Ready][0]=0x90;
    expect(!installFfx2FieldText() && hooks.empty() && !receiver,"all signatures checked before any registration");
    image();expect(installFfx2FieldText() && hooks.size()==4 && receiver,"explicit X-2 experiment installs four verified-role hooks");
    if(hooks.size()!=4 || !receiver) return 1;
    invoke(Open,0);invoke(Ready,Window);
    expect(pending.size()==1,"ready page captured once");
    memory.clear();drain();
    expect(spoken==std::vector<std::wstring>{L"A"},"worker uses owned bytes after all native memory disappears; next page stays hidden");
    image();invoke(Ready,Window);drain();expect(spoken.size()==1,"idle ready callback does not repeat same native page");
    invoke(Open,0);invoke(Ready,Window);drain();expect(spoken.size()==2,"equal text speaks again for a new window lifetime");
    memory[Text]={0x31,0x10,0x30,0x31,0x10,0x31,0x32,0};
    invoke(SetText,0);memory[Window][0x146]=1;put<uint16_t>(Window,0x144,1);
    invoke(Ready,Window);drain();expect(spoken.size()==4 && spoken.back()==L"A","choice opening speaks prompt and current row only");
    memory[Window][0x148]=1;invoke(Choice,0);drain();
    expect(spoken.size()==5 && spoken.back()==L"B" && interruptions.back(),"choice move interrupts with the focused native row");
    invoke(Choice,0);drain();expect(spoken.size()==5,"idle choice callback stays silent");
    put<uint16_t>(Window,0x144,2);memory[Window][0x148]=0;invoke(Choice,0);drain();expect(spoken.size()==5,"accepted/closed selector does not announce old focus");
    put<uint16_t>(Window,0x144,1);accept=false;invoke(Choice,0);expect(pending.empty(),"full capture queue never blocks");
    accept=true;invoke(Choice,0);drain();expect(spoken.size()==6,"rejected capture does not suppress a later retry");
    invoke(Open,0);invoke(Ready,Window);++epoch;drain();expect(spoken.size()==6,"generation change rejects pending speech");
    memory[Text]={0x31,0x13,0x30,0};invoke(Open,0);invoke(Ready,Window);drain();expect(spoken.size()==6,"unresolved name suppresses whole page");
    image();memory[Window][0]=0;invoke(Open,0);invoke(Ready,Window);drain();expect(spoken.size()==6,"inactive window remains silent");
    image();put<uint32_t>(Base+0x12ef654,0,1);invoke(Open,0);invoke(Ready,Window);drain();expect(spoken.size()==6,"unsupported story mode remains silent");
    image();invoke(Open,8);invoke(Choice,8);drain();expect(spoken.size()==6,"invalid native slot never reads a window");
    spoken.clear();image();memory[Text]={0x31,0x10,0x30,0x31,0x10,0x31,0x32,0};
    memory[Window][0x146]=1;put<uint16_t>(Window,0x144,1);
    invoke(Open,0);invoke(Ready,Window);memory[Window][0x148]=1;invoke(Choice,0);drain();
    expect(spoken==std::vector<std::wstring>({L"A",L"B"}),"rapid focus keeps the prompt and only the latest choice");
    spoken.clear();invoke(Open,0);invoke(Ready,Window);invoke(Open,0);drain();
    expect(spoken.empty(),"new same-generation lifetime retires queued old text");
    invoke(Ready,Window);put<uint16_t>(Window,0x144,2);invoke(Choice,0);drain();
    expect(spoken.empty(),"confirm retires speech still awaiting preparation");
#endif
    if(failures) return 1;
    std::puts(FFX2_EXPERIMENTAL_FIELD_TEXT?"FFX-2 field reader experiment fixture passed.":"FFX-2 field reader default-off fixture passed.");
    return 0;
}
