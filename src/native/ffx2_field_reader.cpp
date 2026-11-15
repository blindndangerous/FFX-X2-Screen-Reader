#include "ffx2_field_text.hpp"
#include "ffx2_top_menu.hpp"
#include "profile.hpp"
#include "events.hpp"
#include "speech_domains.hpp"
#include "log_capture.hpp"
#include <atomic>
#include <cstring>

namespace ffx {
#if FFX2_EXPERIMENTAL_FIELD_TEXT
namespace {
// All offsets are from X-2's own setters, page preparer and choice callback.
// FFX uses separate window/cursor objects. See Docs/ffx2-addresses.md.
constexpr unsigned SlotLimit=8,PageOffset=0xf4,CursorState=0x144,CursorEnabled=0x146,CursorRow=0x148;
struct Packet {
    std::array<uint8_t,4096> text{};
    std::array<uint8_t,1024> charset{};
    unsigned textLength=0,charsetLength=0,slot=0,row=0,version=0,page=0;
    bool focus=false,interrupt=false;
    ULONGLONG copiedAt=0,captureMs=0;
};
struct Seen {
    uintptr_t window=0,source=0;
    unsigned epoch=0,lifetime=0,row=~0u,page=0;
};
// Accessed only by the hook framework's verified PInput owner, never a worker.
std::array<Seen,SlotLimit> lastQueued{};
std::array<unsigned,SlotLimit> lifetimes{};
std::array<unsigned,SlotLimit> pageSerial{};
std::array<std::atomic<unsigned>,SlotLimit> versions{};
// Preparation owner only. A superseded first capture cannot lose the prompt.
std::array<unsigned,SlotLimit> promptPages{};
unsigned workerEpoch=0;
uintptr_t target(Ffx2Only name) {return address(0)+ffx2Rva(name);}

unsigned copyCharset(uintptr_t source,uint8_t* out,unsigned capacity) {
    if(!source) return 0;
    for(unsigned at=0;at<capacity;++at) {
        if(!copy(source+at,out+at,1)) return 0;
        if(!out[at]) return at+1;
    }
    return 0;
}
unsigned copyPage(uintptr_t source,uint8_t* out,unsigned capacity) {
    if(!source) return 0;
    for(unsigned at=0;at<capacity;) {
        if(!copy(source+at,out+at,1)) return 0;
        const auto code=out[at];
        if(code<=2) return at+1;
        unsigned width=1;
        if(code==4 || (code>=6 && code<=0x2f)) {
            if(at+1>=capacity || !copy(source+at+1,out+at+1,1)) return 0;
            width=code==4 && out[at+1]<=0x2f?3:2;
        }
        if(width>capacity-at || (width==3 && !copy(source+at+2,out+at+2,1))) return 0;
        at+=width;
    }
    return 0;
}
void beginLifetime(Event& e) {
    if(e.epoch!=generation()) return;
    const auto slot=static_cast<unsigned>(e.args[0]);
    if(slot<SlotLimit) {++lifetimes[slot];++versions[slot];lastQueued[slot]={};}
}
void capturePage(unsigned slot,uintptr_t window,unsigned epoch,bool choice) {
    if(epoch!=generation() || slot>=SlotLimit || !window || !nativeGameThread()) return;
    if(read<unsigned>(target(Ffx2Only::StoryMode),~0u)!=0 ||
       read<uintptr_t>(target(Ffx2Only::FieldTextWindows)+slot*4)!=window) return;
    std::array<uint8_t,0x14c> view;
    const auto started=GetTickCount64();
    if(!copy(window,view.data(),view.size()) || view[0]!=1 || view[1]!=slot) {
        if(lastQueued[slot].epoch) {++versions[slot];lastQueued[slot]={};}
        return;
    }
    uint32_t source=0;uint16_t cursor=0;
    std::memcpy(&source,view.data()+PageOffset,sizeof(source));
    std::memcpy(&cursor,view.data()+CursorState,sizeof(cursor));
    const bool focus=view[CursorEnabled]==1 && cursor==1;
    if(choice && !focus) {
        if(lastQueued[slot].epoch && lastQueued[slot].row!=~0u) {
            ++versions[slot];lastQueued[slot].row=~0u;
        }
        return;
    }
    const unsigned row=focus?view[CursorRow]:~0u;
    if(focus && row>=ffx2text::ChoiceLimit) return;
    const auto& previous=lastQueued[slot];
    const bool same=previous.epoch==epoch && previous.lifetime==lifetimes[slot] &&
        previous.window==window && previous.source==source;
    if(same && previous.row==row) return;
    Packet p;p.slot=slot;p.row=row;p.focus=focus;
    p.version=++versions[slot];p.page=same?previous.page:++pageSerial[slot];
    p.interrupt=same && previous.row!=~0u && focus;
    p.copiedAt=started;
    p.textLength=copyPage(source,p.text.data(),static_cast<unsigned>(p.text.size()));
    p.charsetLength=copyCharset(read<uintptr_t>(target(Ffx2Only::Charset)),p.charset.data(),static_cast<unsigned>(p.charset.size()));
    if(!p.textLength || !p.charsetLength) return;
    p.captureMs=GetTickCount64()-started;
    if(speechq::captureDomain(speechq::Ffx2FieldTextDomain,p,epoch))
        lastQueued[slot]={window,source,epoch,lifetimes[slot],row,p.page};
}
void pageReady(Event& e) {
    const auto window=e.args[0];
    const auto slot=read<uint8_t>(window+1,255);
    capturePage(slot,window,e.epoch,false);
}
void choiceUpdated(Event& e) {
    const auto slot=static_cast<unsigned>(e.args[0]);
    if(slot<SlotLimit) capturePage(slot,read<uintptr_t>(target(Ffx2Only::FieldTextWindows)+slot*4),e.epoch,true);
}
void process(const speechq::Header& header,const std::wstring& owned) {
    if(header.generation!=generation() || owned.size()*sizeof(wchar_t)!=sizeof(Packet)) return;
    Packet p;std::memcpy(&p,owned.data(),sizeof(p));
    if(p.slot>=SlotLimit || p.version!=versions[p.slot].load()) return;
    if(!p.textLength || p.textLength>p.text.size() || !p.charsetLength || p.charsetLength>p.charset.size()) return;
    ffx2text::Charset charset;
    if(!ffx2text::parseLoadedCharset(p.charset.data(),p.charsetLength,charset)) return;
    const auto page=ffx2text::decodeFieldPage(p.text.data(),p.textLength,charset);
    recordLog("FFX-2 field text slot=%u valid=%d capture-ms=%llu preparation-age-ms=%llu",p.slot,page.valid,p.captureMs,GetTickCount64()-p.copiedAt);
    if(!page.valid || header.generation!=generation() || p.version!=versions[p.slot].load()) return;
    if(workerEpoch!=header.generation) {promptPages={};workerEpoch=header.generation;}
    const bool opening=promptPages[p.slot]!=p.page;
    // Voice qualification has no X-2 match yet. The experiment treats these
    // pages as unvoiced; do not enable it in a distributed build.
    if(opening) speechq::emitOwned(ffx2menu::wideFromUtf8(page.prompt),header,"ffx2-field-prompt");
    promptPages[p.slot]=p.page;
    if(page.selector && p.focus && p.row<page.choices.size() &&
       header.generation==generation() && p.version==versions[p.slot].load())
        speechq::emitOwned(ffx2menu::wideFromUtf8(page.choices[p.row]),header,"ffx2-field-choice",false,p.interrupt && !opening);
}
struct Site {Ffx2Only name;Handler before,after;const uint8_t* signature;unsigned size;};
}
#endif
bool installFfx2FieldText() {
#if FFX2_EXPERIMENTAL_FIELD_TEXT
    if(nativeProfile!=NativeProfile::Ffx2) return false;
    static constexpr uint8_t set[]={0x55,0x8b,0xec,0x83,0x3d},open[]={0x55,0x8b,0xec,0x56,0x8b,0x75,0x08};
    static constexpr uint8_t page[]={0x55,0x8b,0xec,0x81,0xec,0x14,0x01,0x00,0x00};
    static constexpr uint8_t choice[]={0x55,0x8b,0xec,0x8b,0x55,0x08,0x66,0xa1};
    const Site sites[]={
        {Ffx2Only::FieldTextSet,beginLifetime,nullptr,set,sizeof(set)},
        {Ffx2Only::FieldTextOpen,beginLifetime,nullptr,open,sizeof(open)},
        {Ffx2Only::FieldTextPage,nullptr,pageReady,page,sizeof(page)},
        {Ffx2Only::FieldTextChoice,nullptr,choiceUpdated,choice,sizeof(choice)},
    };
    for(const auto& site:sites) {
        uint8_t bytes[9]{};
        if(!copy(target(site.name),bytes,site.size) || std::memcmp(bytes,site.signature,site.size)) return false;
    }
    speechq::registerDomain(speechq::Ffx2FieldTextDomain,process,nullptr);
    for(const auto& site:sites) if(!hookAddress(target(site.name),ffx2HookId(ffx2Rva(site.name)),site.before,site.after)) return false;
    return true;
#else
    return false;
#endif
}
}
