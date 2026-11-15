// FFX-2 top-level menu reader. Hooks the menu update (0x373170) and speaks
// the focused item when the cursor moves: the mod's English item name, then
// the help line the game shows. Addresses and evidence: profile.hpp
// (ffx2OnlyAddresses) and Docs/ffx2-addresses.md (FFX-2-only addresses).
#include "x2_boot.hpp"
#include "ffx2_text.hpp"
#include "ffx2_top_menu.hpp"
#include "events.hpp"
#include "log_capture.hpp"
#include "profile.hpp"
#include "runtime.hpp"
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

// The loaded ffx2sjistbl_us.bin. Parsed again whenever the game's pointer
// changes; a failed parse is not cached.
ffx2text::Charset charset;
uintptr_t charsetSource=0;
bool charsetReady() {
    const auto source=read<uintptr_t>(ffx2Address(Ffx2Only::Charset),0);
    if(!source) return false;
    if(source==charsetSource) return true;
    // The buffer is the file rounded up to 16 bytes plus one NUL; the last
    // copy sizes keep a short buffer near the end of a mapping readable.
    for(const std::size_t size:{std::size_t{1024},std::size_t{512},std::size_t{336}}) {
        std::uint8_t bytes[1024];
        if(!copy(source,bytes,size)) continue;
        ffx2text::Charset parsed;
        if(!ffx2text::parseLoadedCharset(bytes,size,parsed)) break;
        charset=std::move(parsed);
        charsetSource=source;
        recordLog("FFX-2 charset loaded glyphs=%u",static_cast<unsigned>(charset.glyphs.size()));
        return true;
    }
    recordLog("FFX-2 charset unreadable at %p",reinterpret_cast<void*>(source));
    return false;
}

std::string helpLine() {
    const auto text=read<uintptr_t>(ffx2Address(Ffx2Only::HelpText),0);
    if(!text || !charsetReady()) return {};
    for(const std::size_t size:{std::size_t{256},std::size_t{96}}) {
        std::uint8_t bytes[256];
        if(copy(text,bytes,size)) return ffx2text::decode(bytes,size,charset);
    }
    return {};
}

bool storyModeLogged=false;
void topMenuBefore(Event& e) {
    TopMenuState state;
    e.before[0]=readState(state);
    e.before[1]=static_cast<intptr_t>(state.cursor);
    e.before[2]=static_cast<intptr_t>(state.count);
}
void topMenuAfter(Event& e) {
    if(!e.before[0]) return;
    TopMenuState before, after;
    before.cursor=static_cast<std::uint32_t>(e.before[1]);
    before.count=static_cast<std::uint32_t>(e.before[2]);
    if(!readState(after)) return;
    before.ids=after.ids;
    if(!ffx2menu::cursorMoved(before,after)) return;
    // 0x379050 maps ids to names differently when this flag is not zero; the
    // name table covers only the zero (main story) mapping.
    const auto mode=read<std::uint32_t>(ffx2Address(Ffx2Only::StoryMode),0xffffffffu);
    if(mode!=0) {
        if(!storyModeLogged) recordLog("FFX-2 top menu not read: story mode flag %u",mode);
        storyModeLogged=true;
        return;
    }
    unsigned id=0;
    if(!ffx2menu::focusedId(after,id)) return;
    const auto speech=ffx2menu::topMenuSpeech(id,helpLine());
    recordLog("FFX-2 top menu cursor=%u id=%u",after.cursor,id);
    if(!speech.empty()) outputAt(speech,true,"ffx2-top-menu",e.epoch);
}

// 0x373170 starts push ebp; mov ebp,esp; push ecx; push ebx; call rel32.
bool prologueMatches(uintptr_t target) {
    const std::uint8_t expected[]={0x55,0x8b,0xec,0x51,0x53,0xe8};
    std::uint8_t actual[sizeof expected]{};
    return copy(target,actual,sizeof actual) && !std::memcmp(actual,expected,sizeof expected);
}
}

bool installFfx2TopMenu() {
    const auto rva=ffx2Rva(Ffx2Only::TopMenuUpdate);
    const auto target=address(0)+rva;
    if(!prologueMatches(target)) { recordLog("FFX-2 top menu hook refused: prologue mismatch at rva=%x",rva); return false; }
    return hookAddress(target,ffx2HookId(rva),topMenuBefore,topMenuAfter);
}
}
