#include "ffx2_submenus.hpp"
#include <algorithm>
#include <cstring>
namespace ffx::ffx2menu {
namespace {
unsigned word(const std::vector<std::uint8_t>& b,std::size_t at){return b[at]|(unsigned{b[at+1]}<<8);}
std::uint32_t dword(const std::vector<std::uint8_t>& b,std::size_t at){return word(b,at)|(std::uint32_t{word(b,at+2)}<<16);}
}
bool parseNativeSelection(NativeList kind,const std::vector<std::uint8_t>& window,
    const std::vector<std::uint8_t>& rows,Selection& result) {
    result={};
    if(kind!=NativeList::Items && kind!=NativeList::Abilities) return false;
    if(window.size()<0x9c || !dword(window,0x98) || window[0x48]>=0x80 ||
        !window[0x45] || window[0x45]>2) return false;
    const unsigned row=word(window,0x4c);
    if(row>=0x8000) return false;
    const unsigned column=std::min<unsigned>(window[0x48],window[0x45]-1);
    const std::uint64_t index=std::uint64_t{dword(window,0x5c+4*column)}+2u*row;
    const unsigned stride=kind==NativeList::Items?4:16;
    if(index>=rows.size()/stride) return false;
    const auto at=static_cast<std::size_t>(index)*stride;
    result.index=static_cast<unsigned>(index);
    result.id=kind==NativeList::Items?word(rows,at):dword(rows,at+4);
    result.available=rows[at+(kind==NativeList::Items?3:0)]!=0;
    return true;
}
bool parsePanelSelection(const std::vector<std::uint8_t>& panel,
    const std::vector<std::uint8_t>& ids,Selection& result) {
    result={};
    if(panel.size()<0x20) return false;
    const unsigned cursor=word(panel,8),count=word(panel,0x1e);
    if(!count || count>=0x8000 || cursor>=count || ids.size()<count) return false;
    result={cursor,ids[cursor],true};return true;
}
bool parseConfigSelection(int cursor,const std::vector<std::uint8_t>& labels,
    const std::vector<std::uint8_t>& values,const std::vector<std::uint8_t>& help,ConfigSelection& result) {
    result={};
    if(cursor<0 || cursor>=11) return false;
    const auto at=static_cast<std::size_t>(cursor)*16;
    if(labels.size()<at+16 || values.size()<at+16 || help.size()<at+16) return false;
    result.index=static_cast<unsigned>(cursor);
    std::memcpy(result.label.data(),labels.data()+at,16);
    std::memcpy(result.value.data(),values.data()+at,16);
    std::memcpy(result.help.data(),help.data()+at,16);return true;
}
}
