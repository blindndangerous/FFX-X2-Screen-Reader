// FFX-2 top-level menu reader. Hooks the menu update (0x373170) and speaks
// the focused item when the cursor moves: the mod's English item name, then
// the help line the game shows. Addresses and evidence: profile.hpp
// (ffx2OnlyAddresses) and Docs/ffx2-addresses.md (FFX-2-only addresses).
#include "x2_boot.hpp"
#include "ffx2_text.hpp"
#include "ffx2_top_menu.hpp"
#include "ffx2_menu_snapshot.hpp"
#include "speech_domains.hpp"
#include "events.hpp"
#include "log_capture.hpp"
#include "profile.hpp"
#include "runtime.hpp"
#include <algorithm>
#include <cstring>

namespace ffx {
namespace {
using ffx2menu::TopMenuState;

uintptr_t ffx2Address(Ffx2Only name) { return address(0)+ffx2Rva(name); }

bool readState(TopMenuState& state) {
    state.cursor=read<std::uint32_t>(ffx2Address(Ffx2Only::TopMenuCursor),0xffffffffu);
    state.count=read<std::uint32_t>(ffx2Address(Ffx2Only::TopMenuCount),0);
    if(!state.count || state.count>ffx2menu::TopMenuCapacity) return false;
    std::uint32_t entries[ffx2menu::TopMenuCapacity*2]{};
    if(!copy(ffx2Address(Ffx2Only::TopMenuItems),entries,sizeof entries)) return false;
    for(unsigned i=0;i<ffx2menu::TopMenuCapacity;++i) state.ids[i]=entries[i*2];
    return true;
}

// Copy a terminated encoded string without reading past its terminator. Control
// parameters may contain NUL; this copies their widths without decoding text.
unsigned captureHelp(uintptr_t source,std::array<std::uint8_t,256>& bytes) {
    if(!source || source>UINTPTR_MAX-bytes.size()) return 0;
    unsigned at=0;
    while(at<bytes.size()) {
        std::uint8_t byte=0;
        if(!copy(source+at,&byte,1)) return 0;
        bytes[at++]=byte;
        if(!byte) return at;
        unsigned parameters=byte>=6 && byte<=0x2f?1u:0u;
        if(byte==4) {
            if(at==bytes.size() || !copy(source+at,&byte,1)) return 0;
            bytes[at++]=byte;parameters=byte<=0x2f?1u:0u;
        }
        if(parameters) {
            if(at==bytes.size() || !copy(source+at,bytes.data()+at,1)) return 0;
            ++at;
        }
    }
    return 0; // Refuse an incomplete string; retain the known item label.
}

// The loaded ffx2sjistbl_us.bin. Copy on its native owner; parse only the
// owned snapshot on preparation. Same-address reloads cannot retain old glyphs.
void captureText(ffx2menu::MenuSnapshot& snapshot) {
    const auto source=read<uintptr_t>(ffx2Address(Ffx2Only::Charset),0);
    // The buffer is the file rounded up to 16 bytes plus one NUL. Capture
    // only the known file extent, never its uninitialised padding or neighbours.
    if(source && copy(source,snapshot.charset.data(),snapshot.charset.size()))
        snapshot.charsetBytes=static_cast<unsigned>(snapshot.charset.size());
    const auto text=read<uintptr_t>(ffx2Address(Ffx2Only::HelpText),0);
    snapshot.helpBytes=captureHelp(text,snapshot.help);
}

void prepareMenu(const speechq::Header& header,const std::wstring& owned) {
    if(header.generation!=generation() || owned.size()*sizeof(wchar_t)!=sizeof(ffx2menu::MenuSnapshot)) return;
    ffx2menu::MenuSnapshot snapshot;std::memcpy(&snapshot,owned.data(),sizeof snapshot);
    if(snapshot.charsetBytes>snapshot.charset.size() || snapshot.helpBytes>snapshot.help.size()) return;
    ffx2text::Charset charset;
    std::string help;
    if(snapshot.charsetBytes==ffx2menu::EnglishCharsetBytes &&
        std::find(snapshot.charset.begin(),snapshot.charset.end(),0)==snapshot.charset.end() &&
        ffx2text::parseCharset(snapshot.charset.data(),snapshot.charsetBytes,charset) &&
        charset.glyphs.size()==ffx2menu::EnglishCharsetGlyphs)
        help=ffx2text::decode(snapshot.help.data(),snapshot.helpBytes,charset);
    const auto speech=ffx2menu::topMenuSpeech(snapshot.id,help);
    recordLog("FFX-2 top menu id=%u charset glyphs=%u",snapshot.id,static_cast<unsigned>(charset.glyphs.size()));
    if(header.generation==generation() && !speech.empty())
        speechq::emitOwned(speech,header,"ffx2-top-menu",false,true);
}

void topMenuBefore(Event& e) {
    TopMenuState state;
    e.before[0]=read<unsigned>(e.args[0]+0x1c,0xffffffffu)==5 && readState(state);
    e.before[1]=static_cast<intptr_t>(state.cursor);
    e.before[2]=static_cast<intptr_t>(state.count);
}
void topMenuAfter(Event& e) {
    if(!e.before[0]) return;
    if(read<unsigned>(e.args[0]+0x1c,0xffffffffu)!=5) return;
    TopMenuState before, after;
    before.cursor=static_cast<std::uint32_t>(e.before[1]);
    before.count=static_cast<std::uint32_t>(e.before[2]);
    if(!readState(after)) return;
    before.ids=after.ids;
    if(!ffx2menu::cursorMoved(before,after)) return;
    // 0x379050 maps ids to names differently when this flag is not zero; the
    // name table covers only the zero (main story) mapping.
    const auto mode=read<std::uint32_t>(ffx2Address(Ffx2Only::StoryMode),0xffffffffu);
    if(mode!=0) return;
    unsigned id=0;
    if(!ffx2menu::focusedId(after,id)) return;
    ffx2menu::MenuSnapshot snapshot{};snapshot.id=id;
    captureText(snapshot);
    speechq::captureDomain(speechq::Ffx2MenuDomain,snapshot,e.epoch);
}

// 0x373170 starts push ebp; mov ebp,esp; push ecx; push ebx; call rel32.
bool prologueMatches(uintptr_t target) {
    const std::uint8_t expected[]={0x55,0x8b,0xec,0x51,0x53,0xe8};
    std::uint8_t actual[sizeof expected]{};
    return copy(target,actual,sizeof actual) && !std::memcmp(actual,expected,sizeof expected);
}
}

bool installFfx2TopMenu() {
    if(nativeProfile!=NativeProfile::Ffx2) return false;
    const auto rva=ffx2Rva(Ffx2Only::TopMenuUpdate);
    const auto target=address(0)+rva;
    if(!prologueMatches(target)) { recordLog("FFX-2 top menu hook refused: prologue mismatch at rva=%x",rva); return false; }
    speechq::registerDomain(speechq::Ffx2MenuDomain,prepareMenu,nullptr);
    return hookAddress(target,ffx2HookId(rva),topMenuBefore,topMenuAfter);
}
}
