#include "ffx2_config.hpp"
#include "ffx2_top_menu.hpp"

namespace ffx::ffx2menu {
namespace {
std::uint32_t dword(const std::uint8_t* b,std::size_t at) {
    return b[at]|(std::uint32_t{b[at+1]}<<8)|(std::uint32_t{b[at+2]}<<16)|(std::uint32_t{b[at+3]}<<24);
}
std::wstring line(const std::string& text) {
    auto out=wideFromUtf8(text);
    for(auto& c:out) if(c==L'\n' || c==L'\r') c=L' ';
    const auto first=out.find_first_not_of(L' ');
    if(first==std::wstring::npos) return {};
    return out.substr(first,out.find_last_not_of(L' ')-first+1);
}
}

bool findTextRange(const std::uint8_t* header,std::size_t size,unsigned id,TextRange& result) {
    result={};
    if(!header || size<0xc) return false;
    const auto count=dword(header,0);
    if(!count || count>MaxTextRanges || size<0xc+std::size_t{count}*20) return false;
    id&=0xfff;
    for(std::uint32_t i=0;i<count;++i) {
        const auto* range=header+0xc+i*20;
        const TextRange candidate{dword(range,0),dword(range,4),dword(range,8),dword(range,12),dword(range,16)};
        if(id<candidate.first || id>candidate.last) continue;
        if(candidate.stride<8 || candidate.stride>64) return false;
        result=candidate;return true;
    }
    return false;
}

bool textRecordOffset(const TextRange& range,unsigned id,std::uint32_t& record) {
    id&=0xfff;
    if(id<range.first || id>range.last || range.stride<8) return false;
    const std::uint64_t at=std::uint64_t{range.records}+std::uint64_t{id-range.first}*range.stride;
    if(at>0x100000) return false;
    record=static_cast<std::uint32_t>(at);return true;
}

ConfigChange configChange(const ConfigRecordSelection& before,const ConfigRecordSelection& after) {
    if(before.index!=after.index) return ConfigChange::Row;
    if(before.labelId==after.labelId && before.valueIndex!=after.valueIndex) return ConfigChange::Value;
    return ConfigChange::None;
}

std::wstring configSpeech(ConfigChange change,const std::string& labelUtf8,const std::string& valueUtf8,const std::string& helpUtf8) {
    const auto label=line(labelUtf8),value=line(valueUtf8),help=line(helpUtf8);
    std::wstring speech;
    if(change==ConfigChange::Row) {
        speech=label;
        if(!value.empty()) speech+=(speech.empty()?L"":L": ")+value;
    } else if(change==ConfigChange::Value) speech=value;
    else return {};
    if(speech.empty()) return {};
    if(!help.empty()) speech+=L". "+help;
    return speech;
}
}
