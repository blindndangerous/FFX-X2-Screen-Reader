// Synthetic cursor/list/descriptor bytes. No original game asset or code.
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
    check(parsePanelSelection(panel,{7,4,9},selected) && selected.index==1 && selected.id==4,"legacy Grid panel selects supplied owned ID list");
    word(panel,8,2);
    check(parsePanelSelection(panel,{2,8,6},selected) && selected.id==6,"legacy Dressphere panel selects supplied owned ID list");
    word(panel,8,3);check(!parsePanelSelection(panel,{7,4,9},selected),"panel cursor at count refused");
    word(panel,8,0xffff);check(!parsePanelSelection(panel,{7,4,9},selected),"negative panel cursor refused");
    word(panel,8,1);check(!parsePanelSelection(panel,{7},selected),"truncated panel list refused");
    check(!parsePanelSelection({}, {7},selected),"truncated panel refused");
    std::vector<std::uint8_t> labels(176),values(176),help(176);
    labels[165]=11;values[165]=22;help[165]=33;ConfigSelection config;
    check(parseConfigSelection(10,labels,values,help,config) && config.index==10 && config.label[5]==11 && config.value[5]==22 && config.help[5]==33,"Config selects last label/value/help descriptor independently");
    check(!parseConfigSelection(-1,labels,values,help,config),"negative Config cursor refused");
    check(!parseConfigSelection(11,labels,values,help,config),"Config cursor above eleven rows refused");
    help.pop_back();check(!parseConfigSelection(10,labels,values,help,config),"partial Config help descriptor refused");
    std::printf("FFX-2 submenu fixture: %u checks, %u failures.\n",checks,failures);return failures?1:0;
}
