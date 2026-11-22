// FFX-2 Config submenu: pure text lookup and speech, then the experimental
// hook over synthetic memory. Built twice: default (no hook) and with
// FFX2_EXPERIMENTAL_CONFIG=1. Labels mirror menu_txt.bin; the charset is a
// synthetic stand-in, never the game's file.
#include "../ffx2_config.hpp"
#include "../x2_boot.hpp"
#include "../events.hpp"
#include "../profile.hpp"
#include "../speech_domains.hpp"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#ifndef FFX2_EXPERIMENTAL_CONFIG
#define FFX2_EXPERIMENTAL_CONFIG 0
#endif

namespace {
int checks=0,failures=0;
bool nativeOwner=true,queueAccepted=true;
unsigned nativeOutputs=0,epoch=7;
ffx::Handler enter=nullptr,leave=nullptr;
ffx::speechq::DomainHandler receiver=nullptr;
ffx::speechq::Header anchor{};
std::wstring packet,spoken;
bool interrupted=false;
std::map<uintptr_t,std::vector<unsigned char>> memory;
void check(bool ok,const char* name) {++checks;if(!ok){++failures;std::fprintf(stderr,"FAIL: %s\n",name);}}
template<class T> void put(uintptr_t at,T value) {
    auto& bytes=memory[at];bytes.resize(sizeof value);std::memcpy(bytes.data(),&value,sizeof value);
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
bool captureDomain(unsigned domain,const void* data,unsigned bytes,const wchar_t*,unsigned,unsigned current) {
    check(nativeOwner,"snapshot enqueued by native owner");
    check(domain==Ffx2ConfigDomain || domain==Ffx2MenuDomain,"known FFX-2 domain");
    if(!queueAccepted)return false;
    packet.assign(static_cast<const wchar_t*>(data),bytes/sizeof(wchar_t));anchor.generation=current;return true;
}
bool emitOwned(std::wstring text,const Header&,const char*,bool,bool interrupt) {
    check(!nativeOwner,"speech assembled on preparation worker");spoken=std::move(text);interrupted=interrupt;return true;
}
}
}

namespace {
using namespace ffx;
using namespace ffx::ffx2menu;
// 74 one-byte glyphs then 118 two-byte glyphs: the measured 310-byte,
// 192-glyph shape. Encoded byte = 0x30 + glyph index.
const std::string Ascii=" .:,'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!?-()/&";
std::vector<unsigned char> charset() {
    std::vector<unsigned char> bytes(Ascii.begin(),Ascii.end());
    for(unsigned i=0;i<118;++i) {bytes.push_back(0xc3);bytes.push_back(0xa9);}
    return bytes;
}
std::vector<unsigned char> encode(const std::string& text) {
    std::vector<unsigned char> out;
    for(std::size_t i=0;i<text.size();++i) {
        if(text[i]=='{') {out.push_back(0x0a);out.push_back(static_cast<unsigned char>(text[i+1]));i+=2;continue;} // "{C}" colour
        out.push_back(static_cast<unsigned char>(0x30+Ascii.find(text[i])));
    }
    out.push_back(0);return out;
}
// One-range table like menu_txt.bin: ids 0..0x13a, stride 8.
constexpr uintptr_t TextTable=0x800000;
constexpr std::uint32_t Records=0x20,Strings=0x9d8;
std::vector<unsigned char> textTable(const std::map<unsigned,std::pair<std::string,std::string>>& entries) {
    std::vector<unsigned char> table(Records+Strings,0);
    const std::uint32_t header[]={1,0,0,0,0x13a,8,Strings,Records};
    std::memcpy(table.data(),header,sizeof header); // count, two pad dwords, then the range at +0xc
    std::vector<unsigned char> strings;
    auto add=[&](const std::string& s){const auto at=static_cast<std::uint16_t>(strings.size());auto e=encode(s);strings.insert(strings.end(),e.begin(),e.end());return at;};
    for(const auto& [id,text]:entries) {
        const auto name=add(text.first),help=add(text.second);
        std::memcpy(table.data()+Records+(id&0xfff)*8,&name,2);std::memcpy(table.data()+Records+(id&0xfff)*8+4,&help,2);
    }
    table.insert(table.end(),strings.begin(),strings.end());
    return table;
}
void record(unsigned row,unsigned label,std::vector<unsigned> values,unsigned selected) {
    std::vector<unsigned char> bytes(0x20,0);
    auto word=[&](unsigned at,unsigned v){bytes[at]=static_cast<unsigned char>(v);bytes[at+1]=static_cast<unsigned char>(v>>8);};
    word(2,label);word(4,static_cast<unsigned>(values.size()));
    for(unsigned i=0;i<values.size();++i) word(6+i*2,values[i]);
    word(0xc,selected);
    memory[0x700000+row*0x20]=bytes;
}
void global(Ffx2Only key,std::uint32_t value) {put(address(ffx2Rva(key)),value);}
constexpr uintptr_t ConfigObject=0x500000;
#if FFX2_EXPERIMENTAL_CONFIG
void choose(unsigned row,unsigned value) {memory[0x700000+row*0x20][0xc]=static_cast<unsigned char>(value);}
// One input-phase call: `change` runs where the game updates its state.
template<class F> void frame(F change) {
    Event e;e.epoch=epoch;e.args[0]=ConfigObject;
    enter(e);change();leave(e);
}
void drain() {
    nativeOwner=false;
    if(receiver && !packet.empty()) receiver(anchor,packet);
    packet.clear();nativeOwner=true;
}
std::wstring row(unsigned from,unsigned to) {
    spoken.clear();global(Ffx2Only::ConfigCursor,from);
    frame([&]{global(Ffx2Only::ConfigCursor,to);});drain();return spoken;
}
#endif

void pureChecks() {
    std::vector<unsigned char> header(0xc+2*20,0);
    const std::uint32_t ranges[]={2,0,0,0,0x10,8,0x100,0x20,0x20,0x30,12,0x200,0x80};
    std::memcpy(header.data(),ranges,sizeof ranges);
    TextRange range;std::uint32_t at=0;
    check(findTextRange(header.data(),header.size(),0x1005,range) && range.first==0 && textRecordOffset(range,0x1005,at) && at==0x20+5*8,
        "id masked to 12 bits finds first range record");
    check(findTextRange(header.data(),header.size(),0x25,range) && range.first==0x20 && textRecordOffset(range,0x25,at) && at==0x80+5*12 &&
        textStringsOffset(range)==0x280,"second range uses its own stride and string base");
    check(!findTextRange(header.data(),header.size(),0x40,range),"unknown id refuses the game's first-range fallback");
    check(!findTextRange(header.data(),0xc+20,0x25,range),"truncated range header refused");
    header[0]=9;check(!findTextRange(header.data(),header.size(),0x5,range),"implausible range count refused");
    ConfigRecordSelection a{6,0x10b7,0x10b8,0,2},b=a;
    check(configChange(a,b)==ConfigChange::None,"unchanged selection is silent");
    b.valueIndex=1;b.valueId=0x10b9;check(configChange(a,b)==ConfigChange::Value,"same row choice change is a value change");
    b=a;b.index=7;b.labelId=0x10ba;check(configChange(a,b)==ConfigChange::Row,"cursor move is a row change");
    check(configSpeech(ConfigChange::Row,"Subtitles","ON","Display subtitles.")==L"Subtitles: ON. Display subtitles.","row speech");
    check(configSpeech(ConfigChange::Value,"Subtitles","OFF","Hide subtitles.")==L"OFF. Hide subtitles.","value speech skips the unchanged label");
    check(configSpeech(ConfigChange::Row,"Guide Map","","")==L"Guide Map","row with unreadable value keeps the label");
    check(configSpeech(ConfigChange::Value,"Guide Map","",
        "Hide on-screen map.").empty(),"value change without a value name is silent");
    check(configSpeech(ConfigChange::Row,"ATB\nMode","Wait"," ")==L"ATB Mode: Wait","line breaks become spaces, blank help dropped");
    check(configSpeech(ConfigChange::None,"a","b","c").empty(),"no change is silent");
    std::uint8_t prefix[16]{};prefix[2]=0xb7;prefix[3]=0x10;prefix[4]=2;prefix[6]=0xb8;prefix[7]=0x10;prefix[8]=0xb9;prefix[9]=0x10;prefix[12]=1;
    ConfigRecordSelection parsed;
    check(parseConfigRecord(6,prefix,sizeof prefix,parsed) && parsed.labelId==0x10b7 && parsed.valueId==0x10b9 && parsed.valueIndex==1,
        "pointer record parse matches the vector parse");
    check(!parseConfigRecord(6,prefix,15,parsed) && !parseConfigRecord(6,nullptr,16,parsed),"short or null record refused");
}

void setupMemory() {
    memory[address(ffx2Rva(Ffx2Only::ConfigInput))]={0x55,0x8b,0xec,0x83,0xec,0x1c,0xe8};
    global(Ffx2Only::StoryMode,0);
    global(Ffx2Only::ConfigTable,static_cast<std::uint32_t>(address(ffx2Rva(Ffx2Only::ConfigMainTable))));
    put(address(ffx2Rva(Ffx2Only::ConfigCount)),std::uint8_t{9});
    global(Ffx2Only::ConfigCursor,5);
    std::vector<std::uint32_t> rows(9);for(unsigned i=0;i<9;++i) rows[i]=0x700000+i*0x20;
    auto& table=memory[address(ffx2Rva(Ffx2Only::ConfigMainTable))];table.resize(36);std::memcpy(table.data(),rows.data(),36);
    // Shipped label/value ids and English strings (menu_txt.bin).
    record(0,0x10c6,{0x10c7,0x10c8,0x10c9},0);
    record(1,0x10cf,{0x10cb,0x10ca},0);
    record(5,0x10c3,{0x10c4,0x10c5},0);
    record(6,0x10b7,{0x10b8,0x10b9},0);
    record(7,0x10ba,{0x10bb,0x10bc},0);
    record(8,0x10c0,{0x10c1,0x0fff},0);
    for(unsigned r:{2u,3u,4u}) record(r,0x10b4,{0x10b5,0x10b6},0);
    memory[TextTable]=textTable({
        {0x10c6,{"Spherechanges","UNUSED"}},{0x10c7,{"Normal","Display full spherechange sequences."}},
        {0x10c8,{"Short","Display shortened spherechange sequences."}},{0x10c9,{"OFF","Skip spherechange sequences."}},
        {0x10cf,{"ATB Mode","UNUSED"}},{0x10cb,{"Active","Set Active Time Battle mode to {C}Active{A}."}},
        {0x10ca,{"Wait","Set Active Time Battle mode to {C}Wait{A}."}},
        {0x10c3,{"Vibration","UNUSED"}},{0x10c4,{"ON","Turn vibration function on."}},{0x10c5,{"OFF","Turn vibration function off."}},
        {0x10b7,{"Subtitles","UNUSED"}},{0x10b8,{"ON","Display subtitles."}},{0x10b9,{"OFF","Hide subtitles."}},
        {0x10ba,{"Subtitle Names","UNUSED"}},{0x10bb,{"ON","Display speaker's name in subtitles."}},
        {0x10c0,{"Guide Map","UNUSED"}},{0x10c1,{"ON","Display on-screen map."}},
        {0x10b4,{"Cursor","UNUSED"}},{0x10b5,{"Memory","Remember previous cursor position in battle menus."}},
    });
    const auto spaces=address(ffx2Rva(Ffx2Only::MenuTextSpaces));
    memory[spaces]=std::vector<unsigned char>(5*12,0);
    std::memcpy(memory[spaces].data()+12+8,&TextTable,4);
    memory[0x600000]=charset();global(Ffx2Only::Charset,0x600000);
    put(ConfigObject+0x28,3u);
}

#if FFX2_EXPERIMENTAL_CONFIG
void hookChecks() {
    nativeProfile=NativeProfile::Ffx2;
    setupMemory();
    auto& prologue=memory[address(ffx2Rva(Ffx2Only::ConfigInput))];
    prologue[5]=0x20;check(!installFfx2Config() && !enter,"prologue mismatch installs nothing");prologue[5]=0x1c;
    check(installFfx2Config() && enter && leave && receiver,"verified prologue installs hook and worker");
    check(row(5,6)==L"Subtitles: ON. Display subtitles.","row move speaks label, value and value help");
    check(nativeOutputs==0,"hook does not decode or emit speech");
    check(interrupted,"menu navigation interrupts the previous line");
    global(Ffx2Only::ConfigCursor,6);spoken.clear();
    frame([]{choose(6,1);});
    // The worker must use the captured strings even if the table moves now.
    const auto text=memory[TextTable];memory[TextTable].assign(text.size(),0);
    drain();check(spoken==L"OFF. Hide subtitles.","choice change speaks value and help from the owned snapshot");
    memory[TextTable]=text;
    spoken.clear();frame([]{});drain();check(spoken.empty(),"idle frame stays silent");
    check(row(0,1)==L"ATB Mode: Active. Set Active Time Battle mode to Active.","colour controls produce no text");
    check(row(6,0)==L"Spherechanges: Normal. Display full spherechange sequences.","three-choice row");
    check(row(1,7)==L"Subtitle Names: ON. Display speaker's name in subtitles.","punctuation kept as the table spells it");
    global(Ffx2Only::ConfigCursor,8);choose(8,1);spoken.clear();frame([]{choose(8,0);});drain();
    check(spoken==L"ON. Display on-screen map.","value back to a known id");
    spoken.clear();frame([]{choose(8,1);});drain();check(spoken.empty(),"value outside every text range is silent");
    // Gates: every one must keep the reader silent.
    put(ConfigObject+0x28,8u);check(row(5,6).empty(),"confirm/cancel phases stay silent");put(ConfigObject+0x28,3u);
    global(Ffx2Only::StoryMode,1);check(row(5,6).empty(),"non-story mode stays silent");global(Ffx2Only::StoryMode,0);
    global(Ffx2Only::ConfigTable,0x123456);check(row(5,6).empty(),"other Config table (Last Mission) stays silent");
    global(Ffx2Only::ConfigTable,static_cast<std::uint32_t>(address(ffx2Rva(Ffx2Only::ConfigMainTable))));
    put(address(ffx2Rva(Ffx2Only::ConfigCount)),std::uint8_t{8});check(row(5,6).empty(),"other row count stays silent");
    put(address(ffx2Rva(Ffx2Only::ConfigCount)),std::uint8_t{9});
    check(row(5,9).empty(),"cursor past the table stays silent");
    const auto spaces=address(ffx2Rva(Ffx2Only::MenuTextSpaces));
    memory[spaces][12]=1;check(row(5,6).empty(),"busy text space stays silent");memory[spaces][12]=0;
    global(Ffx2Only::Charset,0);check(row(5,6).empty(),"missing charset stays silent");global(Ffx2Only::Charset,0x600000);
    memory[0x600000].assign(310,'x');memory[0x600000][100]=0xff;check(row(5,6).empty(),"malformed charset stays silent");
    memory[0x600000]=charset();
    check(row(5,6)==L"Subtitles: OFF. Hide subtitles.","reader recovers after gates clear, with the choice made earlier");
    spoken.clear();queueAccepted=false;row(5,6);check(spoken.empty(),"queue pressure drops without native fallback");queueAccepted=true;
    global(Ffx2Only::ConfigCursor,5);frame([]{global(Ffx2Only::ConfigCursor,6);});++epoch;spoken.clear();drain();
    check(spoken.empty(),"retired observation stays silent");
}
#endif
}

int main() {
    pureChecks();
    nativeProfile=NativeProfile::Reference;
    check(!installFfx2Config() && !enter && !receiver,"FFX profile installs no Config reader or worker");
#if FFX2_EXPERIMENTAL_CONFIG
    hookChecks();
#else
    nativeProfile=NativeProfile::Ffx2;setupMemory();
    check(!installFfx2Config() && !enter && !receiver,"default build installs no Config hook even under FFX-2");
#endif
    std::printf("FFX-2 Config fixture (%s): %d checks, %d failures.\n",FFX2_EXPERIMENTAL_CONFIG?"experiment":"default-off",checks,failures);
    return failures?1:0;
}
