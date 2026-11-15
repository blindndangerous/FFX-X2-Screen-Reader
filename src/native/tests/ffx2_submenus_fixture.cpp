// Synthetic cursor/list/setting-record bytes. No original game asset or code.
#include "../ffx2_submenus.hpp"
#include <cstdio>
namespace {
unsigned checks=0,failures=0;
void check(bool ok,const char* name){++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
void word(std::vector<std::uint8_t>& b,unsigned at,unsigned value){b[at]=static_cast<std::uint8_t>(value);b[at+1]=static_cast<std::uint8_t>(value>>8);}
}
int main(){
    using namespace ffx::ffx2menu;
    std::vector<std::uint8_t> window(0x9c),items(32),abilities(128);
    window[0x45]=2;window[0x48]=1;word(window,0x4c,1);window[0x60]=1;window[0x98]=8;
    word(items,12,0x1234);items[15]=3;abilities[48]=1;abilities[52]=0x56;abilities[53]=0x34;
    Selection selected;
    check(parseNativeSelection(NativeList::Items,window,items,selected) && selected.index==3 && selected.id==0x1234 && selected.available,"Items combines row and second-column base");
    check(parseNativeSelection(NativeList::Abilities,window,abilities,selected) && selected.index==3 && selected.id==0x3456 && selected.available,"Abilities uses 16-byte row and dword ID at +4");
    items[15]=0;
    check(parseNativeSelection(NativeList::Items,window,items,selected) && !selected.available,"unavailable item stays distinguishable");
    auto broken=window;word(broken,0x4c,0xffff);
    check(!parseNativeSelection(NativeList::Items,broken,items,selected),"negative cursor refused");
    broken=window;broken[0x48]=0xff;
    check(!parseNativeSelection(NativeList::Items,broken,items,selected),"closed window refused");
    broken=window;broken[0x45]=0;
    check(!parseNativeSelection(NativeList::Items,broken,items,selected),"zero columns refused");
    broken=window;broken[0x98]=0;
    check(!parseNativeSelection(NativeList::Items,broken,items,selected),"unpopulated list refused");
    broken=window;broken[0x60]=0xff;broken[0x61]=broken[0x62]=broken[0x63]=0xff;
    check(!parseNativeSelection(NativeList::Items,broken,items,selected),"index overflow refused");
    check(!parseNativeSelection(NativeList::Items,{},items,selected),"short window refused");
    check(!parseNativeSelection(NativeList::Abilities,window,std::vector<std::uint8_t>(63),selected),"partial selected ability row refused");
    check(!parseNativeSelection(static_cast<NativeList>(99),window,items,selected),"unknown layout refused");
    std::vector<std::uint8_t> panel(0x20);word(panel,8,1);word(panel,0x1e,3);
    check(parsePanelSelection(panel,{7,4,9},selected) && selected.index==1 && selected.id==4,"Dressphere parent selects supplied owned ID list");
    word(panel,8,2);
    check(parsePanelSelection(panel,{2,8,6},selected) && selected.id==6,"changed Dressphere cursor selects another ID");
    word(panel,8,3);check(!parsePanelSelection(panel,{7,4,9},selected),"panel cursor at count refused");
    word(panel,8,0xffff);check(!parsePanelSelection(panel,{7,4,9},selected),"negative panel cursor refused");
    word(panel,8,1);check(!parsePanelSelection(panel,{7},selected),"truncated panel list refused");
    check(!parsePanelSelection({}, {7},selected),"truncated panel refused");
    word(panel,0x18,2);check(!parsePanelSelection(panel,{7,4,9},selected),"node pane cannot masquerade as Dressphere list");
    std::vector<std::uint8_t> gridPanel(0x20),nodes(96);GridSelection grid;
    word(gridPanel,8,1);word(gridPanel,0xe,2);word(gridPanel,0x18,2);word(gridPanel,0x1e,32);nodes[65]=7;
    check(parseGridSelection(gridPanel,nodes,grid) && grid.grid==2 && grid.node==1 && grid.dressphere==7 && grid.populated,"Garment Grid selects node from correct grid bank");
    nodes[65]=0;check(parseGridSelection(gridPanel,nodes,grid) && !grid.populated,"zero node is empty rather than a dressphere");
    word(gridPanel,8,32);check(!parseGridSelection(gridPanel,nodes,grid),"node at capacity refused");
    word(gridPanel,8,0xffff);check(!parseGridSelection(gridPanel,nodes,grid),"negative node refused");
    word(gridPanel,8,1);word(gridPanel,0xe,0xffff);check(!parseGridSelection(gridPanel,nodes,grid),"negative grid refused");
    word(gridPanel,0xe,3);check(!parseGridSelection(gridPanel,nodes,grid),"grid past supplied bank refused");
    word(gridPanel,0xe,2);nodes.pop_back();check(!parseGridSelection(gridPanel,nodes,grid),"partial grid bank refused");
    nodes.resize(96);word(gridPanel,0x18,0);check(!parseGridSelection(gridPanel,nodes,grid),"Dressphere list cannot masquerade as node pane");
    word(gridPanel,0x18,2);word(gridPanel,0x1e,31);check(!parseGridSelection(gridPanel,nodes,grid),"unqualified node extent refused");
    std::vector<std::uint8_t> record(16);ConfigRecordSelection config;
    word(record,2,0x1055);word(record,4,3);word(record,6,0x1077);word(record,8,0x1088);word(record,10,0x1099);word(record,12,1);
    check(parseConfigRecord(8,record,config) && config.index==8 && config.labelId==0x1055 && config.valueId==0x1088 && config.valueIndex==1 && config.valueCount==3,"Config selects label and current value IDs from actual record prefix");
    word(record,12,2);check(parseConfigRecord(0,record,config) && config.valueId==0x1099,"last of three Config options selected");
    check(!parseConfigRecord(-1,record,config),"negative Config cursor refused");
    check(!parseConfigRecord(9,record,config),"Config cursor above nine main-story rows refused");
    word(record,12,3);check(!parseConfigRecord(0,record,config),"Config value at choice count refused");
    word(record,12,0xffff);check(!parseConfigRecord(0,record,config),"negative Config value refused");
    word(record,12,0);word(record,4,0);check(!parseConfigRecord(0,record,config),"empty Config choices refused");
    word(record,4,4);check(!parseConfigRecord(0,record,config),"Config choices cannot overlap value-index field");
    word(record,4,2);record.pop_back();check(!parseConfigRecord(0,record,config),"short Config record refused");
    std::printf("FFX-2 submenu fixture: %u checks, %u failures.\n",checks,failures);return failures?1:0;
}
