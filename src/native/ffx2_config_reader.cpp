// FFX-2 Config submenu reader (EXPERIMENTAL, static evidence only). Hooks the
// Config input phase 0x388c40 and speaks the focused setting when the row or
// its choice changes: the game's label, the selected value and the value help
// line the game shows (0x3891b4..0x3891cc passes that help id to 0x3638a0).
// Built only with FFX2_EXPERIMENTAL_CONFIG=1 until a live test confirms the
// record table, text space and phase. See Docs/ffx2-addresses.md (Config).
#include "ffx2_config.hpp"
#include "ffx2_text.hpp"
#include "x2_boot.hpp"
#include "speech_domains.hpp"
#include "events.hpp"
#include "log_capture.hpp"
#include "profile.hpp"
#include "runtime.hpp"
#include <algorithm>
#include <cstring>

namespace ffx {
#if FFX2_EXPERIMENTAL_CONFIG
namespace {
using ffx2menu::ConfigChange;
using ffx2menu::ConfigRecordSelection;
using ffx2menu::ConfigSnapshot;
constexpr unsigned InputPhase=3,PhaseOffset=0x28,MainRows=9,MenuTextSpace=1,SpaceEntryBytes=12;

uintptr_t ffx2Address(Ffx2Only name) {return address(0)+ffx2Rva(name);}

// Main story only: the active table must be the 9-row main table. Last
// Mission uses its own table, which this reader does not claim to know.
bool readSelection(const Event& e,ConfigRecordSelection& out) {
    if(read<std::uint32_t>(e.args[0]+PhaseOffset,0xffffffffu)!=InputPhase) return false;
    if(read<std::uint32_t>(ffx2Address(Ffx2Only::StoryMode),0xffffffffu)!=0) return false;
    const auto table=read<uintptr_t>(ffx2Address(Ffx2Only::ConfigTable),0);
    if(table!=ffx2Address(Ffx2Only::ConfigMainTable)) return false;
    if(read<std::uint8_t>(ffx2Address(Ffx2Only::ConfigCount),0)!=MainRows) return false;
    const auto cursor=read<std::uint32_t>(ffx2Address(Ffx2Only::ConfigCursor),0xffffffffu);
    if(cursor>=MainRows) return false;
    const auto record=read<uintptr_t>(table+cursor*4,0);
    std::uint8_t prefix[16]{};
    return record && copy(record,prefix,sizeof prefix) &&
        ffx2menu::parseConfigRecord(static_cast<int>(cursor),prefix,sizeof prefix,out);
}

// 0x21d7c0/0x21d840: refuse a busy or absent space; the range lookup and
// record are copied from the loaded table, then the string by its terminator.
std::uint32_t captureMenuText(uintptr_t table,unsigned id,unsigned field,std::uint8_t* out,unsigned capacity) {
    std::uint8_t header[ffx2menu::TextHeaderBytes]{};
    if(!copy(table,header,0xc)) return 0;
    std::uint32_t count=0;std::memcpy(&count,header,sizeof count);
    if(!count || count>ffx2menu::MaxTextRanges || !copy(table+0xc,header+0xc,count*20)) return 0;
    ffx2menu::TextRange range;std::uint32_t record=0;
    if(!ffx2menu::findTextRange(header,0xc+count*20,id,range) || !ffx2menu::textRecordOffset(range,id,record)) return 0;
    std::uint8_t fields[8]{};
    if(!copy(table+record,fields,sizeof fields)) return 0;
    const unsigned offset=fields[field]|(unsigned{fields[field+1]}<<8);
    return captureFfx2Encoded(table+ffx2menu::textStringsOffset(range)+offset,out,capacity);
}

void prepareConfig(const speechq::Header& header,const std::wstring& owned) {
    if(header.generation!=generation() || owned.size()*sizeof(wchar_t)!=sizeof(ConfigSnapshot)) return;
    ConfigSnapshot s;std::memcpy(&s,owned.data(),sizeof s);
    if(s.charsetBytes!=ffx2menu::EnglishCharsetBytes || s.labelBytes>s.label.size() ||
       s.valueBytes>s.value.size() || s.helpBytes>s.help.size()) return;
    ffx2text::Charset charset;
    if(std::find(s.charset.begin(),s.charset.end(),0)!=s.charset.end() ||
       !ffx2text::parseCharset(s.charset.data(),s.charsetBytes,charset) ||
       charset.glyphs.size()!=ffx2menu::EnglishCharsetGlyphs) return;
    const auto speech=ffx2menu::configSpeech(static_cast<ConfigChange>(s.change),
        ffx2text::decode(s.label.data(),s.labelBytes,charset),
        ffx2text::decode(s.value.data(),s.valueBytes,charset),
        ffx2text::decode(s.help.data(),s.helpBytes,charset));
    recordLog("FFX-2 config change=%u",s.change);
    if(header.generation==generation() && !speech.empty())
        speechq::emitOwned(speech,header,"ffx2-config",false,true);
}

void configBefore(Event& e) {
    ConfigRecordSelection state;
    e.before[0]=readSelection(e,state);
    e.before[1]=static_cast<intptr_t>(state.index);
    e.before[2]=static_cast<intptr_t>(state.labelId);
    e.before[3]=static_cast<intptr_t>(state.valueIndex);
}
void configAfter(Event& e) {
    if(!e.before[0]) return;
    ConfigRecordSelection before,after;
    if(!readSelection(e,after)) return;
    before.index=static_cast<unsigned>(e.before[1]);
    before.labelId=static_cast<unsigned>(e.before[2]);
    before.valueIndex=static_cast<unsigned>(e.before[3]);
    const auto change=ffx2menu::configChange(before,after);
    if(change==ConfigChange::None) return;
    const auto space=ffx2Address(Ffx2Only::MenuTextSpaces)+MenuTextSpace*SpaceEntryBytes;
    if(read<std::uint8_t>(space,1)!=0) return;
    const auto table=read<uintptr_t>(space+8,0);
    if(!table) return;
    ConfigSnapshot s{};s.change=static_cast<std::uint32_t>(change);
    if(change==ConfigChange::Row)
        s.labelBytes=captureMenuText(table,after.labelId,ffx2menu::TextNameField,s.label.data(),static_cast<unsigned>(s.label.size()));
    s.valueBytes=captureMenuText(table,after.valueId,ffx2menu::TextNameField,s.value.data(),static_cast<unsigned>(s.value.size()));
    s.helpBytes=captureMenuText(table,after.valueId,ffx2menu::TextHelpField,s.help.data(),static_cast<unsigned>(s.help.size()));
    s.charsetBytes=captureFfx2Charset(s.charset);
    speechq::captureDomain(speechq::Ffx2ConfigDomain,s,e.epoch);
}
}
#endif

bool installFfx2Config() {
#if FFX2_EXPERIMENTAL_CONFIG
    if(nativeProfile!=NativeProfile::Ffx2) return false;
    const auto rva=ffx2Rva(Ffx2Only::ConfigInput);
    const auto target=address(0)+rva;
    // push ebp; mov ebp,esp; sub esp,1c; call rel32 (frame gate 0x51c100).
    static constexpr std::uint8_t prologue[]={0x55,0x8b,0xec,0x83,0xec,0x1c,0xe8};
    std::uint8_t actual[sizeof prologue]{};
    if(!copy(target,actual,sizeof actual) || std::memcmp(actual,prologue,sizeof prologue)) {
        recordLog("FFX-2 config hook refused: prologue mismatch at rva=%x",rva);return false;
    }
    speechq::registerDomain(speechq::Ffx2ConfigDomain,prepareConfig,nullptr);
    return hookAddress(target,ffx2HookId(rva),configBefore,configAfter);
#else
    return false;
#endif
}
}
