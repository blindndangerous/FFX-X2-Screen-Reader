// FFX-2 top-level menu helpers; see ffx2_top_menu.hpp.
#include "ffx2_top_menu.hpp"

namespace ffx::ffx2menu {
namespace {
// Indexed by menu id. The names are the game's own menu_txt name records
// (0x3790d0 maps id 0..6 to records 0x1047+id, 7 to 0x104f, 8 to 0x1050,
// 9 to 0x104e); the menu draws them from a texture instead.
const wchar_t* const englishNames[]={
    L"Items", L"White Magic", L"Equip", L"Garment Grids", L"Abilities",
    L"Accessories", L"Dresspheres", L"Develop", L"Team Members", L"Config",
};
bool blank(const std::wstring& text) {
    for(const auto c:text) if(c!=L' ') return false;
    return true;
}
}

const wchar_t* topMenuItemName(unsigned id) {
    return id<sizeof englishNames/sizeof englishNames[0] ? englishNames[id] : nullptr;
}

bool focusedId(const TopMenuState& state, unsigned& id) {
    if(!state.count || state.count>TopMenuCapacity || state.cursor>=state.count) return false;
    id=state.ids[state.cursor];
    return true;
}

bool cursorMoved(const TopMenuState& before, const TopMenuState& after) {
    unsigned id=0;
    return before.count==after.count && before.cursor!=after.cursor && focusedId(after,id);
}

std::wstring topMenuSpeech(unsigned id, const std::string& helpUtf8) {
    auto help=wideFromUtf8(helpUtf8);
    for(auto& c:help) if(c==L'\n' || c==L'\r') c=L' ';
    if(blank(help)) help.clear();
    const auto* name=topMenuItemName(id);
    if(!name) return help;
    std::wstring speech=name;
    if(!help.empty()) speech+=L". "+help;
    return speech;
}

std::wstring wideFromUtf8(const std::string& text) {
    std::wstring out;
    for(std::size_t at=0;at<text.size();) {
        const auto lead=static_cast<unsigned char>(text[at]);
        std::size_t length=0; unsigned point=0, minimum=0;
        if(lead<0x80) { out.push_back(static_cast<wchar_t>(lead)); ++at; continue; }
        if(lead>=0xc2 && lead<=0xdf) { length=2; point=lead&0x1f; minimum=0x80; }
        else if(lead>=0xe0 && lead<=0xef) { length=3; point=lead&0x0f; minimum=0x800; }
        else if(lead>=0xf0 && lead<=0xf4) { length=4; point=lead&0x07; minimum=0x10000; }
        bool ok=length && at+length<=text.size();
        for(std::size_t i=1;ok && i<length;++i) {
            const auto next=static_cast<unsigned char>(text[at+i]);
            if((next&0xc0)!=0x80) ok=false;
            else point=(point<<6)|(next&0x3f);
        }
        if(!ok || point<minimum || point>0x10ffff || (point>=0xd800 && point<=0xdfff)) { ++at; continue; }
        if(point>=0x10000) {
            point-=0x10000;
            out.push_back(static_cast<wchar_t>(0xd800+(point>>10)));
            out.push_back(static_cast<wchar_t>(0xdc00+(point&0x3ff)));
        } else out.push_back(static_cast<wchar_t>(point));
        at+=length;
    }
    return out;
}
}
