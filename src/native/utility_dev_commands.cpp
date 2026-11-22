#include "utility_dev_commands.hpp"
#ifdef FFX_DEV_UTILITIES
#include "log_capture.hpp"
#include "navigation.hpp"
#include "utility_memory.hpp"
#include <charconv>
#include <cmath>
#include <limits>

namespace ffx {
namespace {
std::wstring_view trim(std::wstring_view text) {
    const auto start=text.find_first_not_of(L" \t\r\n");
    return start==text.npos?std::wstring_view{}:text.substr(start,text.find_last_not_of(L" \t\r\n")-start+1);
}
template<class T> bool parseNumber(std::wstring_view text,T& value,int base) {
    char bytes[24];
    if(base==10 && !text.empty() && text.front()==L'+') {
        text.remove_prefix(1);
        if(text.empty() || text.front()<L'0' || text.front()>L'9') return false;
    }
    if(text.empty() || text.size()>sizeof(bytes)) return false;
    for(size_t i=0;i<text.size();++i) {if(text[i]>127) return false;bytes[i]=static_cast<char>(text[i]);}
    const auto end=bytes+text.size();const auto parsed=std::from_chars(bytes,end,value,base);
    return parsed.ec==std::errc{} && parsed.ptr==end;
}
bool coordinate(UtilityText& text,double value) {
    const auto tenths=std::round(value*10);
    if(!(std::abs(tenths)<0x1p63) || value==-431602080) return false;
    const auto scaled=static_cast<int64_t>(tenths);
    if(scaled<0 && scaled>-10) text.literal(L"-");
    text.integer(scaled/10).literal(L".").integer(std::abs(scaled%10));
    return true;
}
bool fitsVariable(unsigned format,int64_t value) {
    constexpr int64_t minimum[]={0,INT8_MIN,0,INT16_MIN,INT32_MIN,INT32_MIN};
    constexpr int64_t maximum[]={UINT8_MAX,INT8_MAX,UINT16_MAX,INT16_MAX,INT32_MAX,INT32_MAX};
    return value>=minimum[format] && value<=maximum[format];
}
}
UtilityText utilityDevPosition(std::wstring_view arguments) {
    if(!trim(arguments).empty()) return L"Use pos with no arguments.";
    const auto actor=playerActor();
    if(!actor || !read<uint16_t>(actor+2)) return L"Player position is unavailable.";
    const auto player=playerPosition();
    UtilityText reply(L"X ");
    if(!coordinate(reply,player.x) || !coordinate(reply.literal(L", Y "),player.y) ||
        !coordinate(reply.literal(L", Z "),player.z)) return L"Player position is unavailable.";
    // Actor rotation is stored in radians at 0x158.
    const auto angle=read<float>(actor+0x158,std::numeric_limits<float>::quiet_NaN());
    if(!std::isfinite(angle) || angle==-431602080) return L"Player facing is unavailable.";
    const int degrees=static_cast<int>(std::round(std::fmod(angle*57.29577951308232,360.0)));
    const auto room=read<uint16_t>(address(0xd2ca90),0xffff);
    if(room==0xffff) return L"Room is unavailable.";
    const auto script=utilityScript();
    reply.literal(L". Facing ").integer((degrees+360)%360).literal(L" degrees. Room ").integer(room)
        .literal(L". Script ").narrow(script.name.c_str()).literal(L".");
    return reply;
}
UtilityText utilityDevVariable(std::wstring_view arguments) {
    arguments=trim(arguments);
    auto id=arguments.substr(0,arguments.find_first_of(L" \t="));
    arguments=trim(arguments.substr(id.size()));
    if(id.substr(0,2)==L"0x") id.remove_prefix(2);
    UtilityBinding binding{};
    if(id.size()>8 || !parseNumber(id,binding.low,16)) return L"Invalid hexadecimal binding ID.";
    const auto format=binding.low>>28;
    if(format>5) return L"Binding is not an integer.";
    const bool writing=!arguments.empty() && arguments.front()==L'=';
    if(writing) {
        int64_t value=0;
        if(!parseNumber(trim(arguments.substr(1)),value,10)) return L"Invalid decimal value.";
        if(!fitsVariable(format,value)) return L"Value out of range.";
        binding.value=static_cast<int>(value);
    } else if(!arguments.empty() && !parseNumber(arguments,binding.element,10)) return L"Invalid element index.";

    UtilityScript script;
    if(!captureUtilityScript(script)) return L"No active field script.";
    AtelSpan span;
    const auto count=read<uint16_t>(script.header+2);
    const auto offset=read<unsigned>(script.header+0x14,UINT32_MAX);
    // Match preparation's declaration cap. Copy the table once, then let the
    // existing accessor verify the exact live row against prepared metadata.
    std::array<std::array<unsigned,2>,2048> rows;
    if(!nativeAtelSpan(script.chunk,span) || span.serial!=script.allocationSerial || count>rows.size() ||
        offset>span.size || count>(span.size-offset)/sizeof(rows[0]) ||
        (count && !copy(script.chunk+offset,rows.data(),count*sizeof(rows[0])))) return L"Script bindings are unavailable.";
    bool found=false;
    for(unsigned i=0;i<count;++i) if(rows[i][0]==binding.low) {
        if(found) return L"Binding ID is ambiguous.";
        binding.high=rows[i][1];found=true;
    }
    if(!found) return L"Binding not found in the current script.";
    if(binding.element>=(binding.high&0xffff)) return L"Element out of range.";
    int value=0;
    if(!captureUtilityValue(script,binding.low,binding.high,binding.element,value)) return L"Binding value is unavailable.";
    if(writing) {
        utilityWrite(script,binding);
        if(!captureUtilityValue(script,binding.low,binding.high,binding.element,value) || value!=binding.value)
            return L"Binding write could not be verified.";
    }
    wchar_t label[9]{};id.copy(label,id.size());
    UtilityText reply(label);reply.literal(L" = ").integer(value).literal(L".");return reply;
}
UtilityText utilityDevMark(std::wstring_view arguments) {
    arguments=trim(arguments);
    if(arguments.empty()) return L"Enter text after mark.";
    char text[LogDataLimit-sizeof("test mark: ")];
    const auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,arguments.data(),static_cast<int>(arguments.size()),
        text,sizeof(text)-1,nullptr,nullptr);
    if(!size) return L"Mark text could not be logged.";
    text[size]=0;recordLog("test mark: %s",text);
    return L"Marked.";
}
}
#endif
