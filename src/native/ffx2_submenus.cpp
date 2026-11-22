#include "ffx2_submenus.hpp"
#include <algorithm>
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
    if(panel.size()<0x20 || word(panel,0x18)!=0) return false;
    const unsigned cursor=word(panel,8),count=word(panel,0x1e);
    if(!count || count>=0x8000 || cursor>=count || ids.size()<count) return false;
    result={cursor,ids[cursor],true};return true;
}
bool parseGridSelection(const std::vector<std::uint8_t>& panel,
    const std::vector<std::uint8_t>& nodes,GridSelection& result) {
    result={};
    if(panel.size()<0x20 || word(panel,0x18)!=2 || word(panel,0x1e)!=32) return false;
    const unsigned node=word(panel,8),grid=word(panel,0xe);
    if(node>=32 || grid>=0x8000 || grid>=nodes.size()/32) return false;
    const unsigned id=nodes[grid*32+node];result={grid,node,id,id!=0};return true;
}
bool parseConfigRecord(int cursor,const std::vector<std::uint8_t>& record,ConfigRecordSelection& result) {
    return parseConfigRecord(cursor,record.data(),record.size(),result);
}
bool parseConfigRecord(int cursor,const std::uint8_t* record,std::size_t size,ConfigRecordSelection& result) {
    result={};
    if(!record || cursor<0 || cursor>=9 || size<16) return false;
    const auto at=[record](std::size_t offset){return record[offset]|(unsigned{record[offset+1]}<<8);};
    const unsigned count=at(4),value=at(12);
    if(!count || count>3 || value>=count) return false;
    result={static_cast<unsigned>(cursor),at(2),at(6+value*2),value,count};return true;
}
}
