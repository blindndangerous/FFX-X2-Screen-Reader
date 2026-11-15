// FFX-2 text decoder; see ffx2_text.hpp.
#include "ffx2_text.hpp"

namespace ffx::ffx2text {
namespace {
// Length of the well-formed UTF-8 sequence at data[0], or 0.
std::size_t sequenceLength(const std::uint8_t* data, std::size_t left) {
    const unsigned lead=data[0];
    std::size_t length=0; unsigned point=0, minimum=0;
    if(lead<0x80) return 1;
    if(lead>=0xc2 && lead<=0xdf) { length=2; point=lead&0x1f; minimum=0x80; }
    else if(lead>=0xe0 && lead<=0xef) { length=3; point=lead&0x0f; minimum=0x800; }
    else if(lead>=0xf0 && lead<=0xf4) { length=4; point=lead&0x07; minimum=0x10000; }
    else return 0;
    if(left<length) return 0;
    for(std::size_t i=1;i<length;++i) {
        if((data[i]&0xc0)!=0x80) return 0;
        point=(point<<6)|(data[i]&0x3f);
    }
    if(point<minimum || point>0x10ffff || (point>=0xd800 && point<=0xdfff)) return 0;
    return length;
}
void append(std::string& out, std::size_t index, const Charset& charset) {
    if(index<charset.glyphs.size()) out+=charset.glyphs[index];
}
}

bool parseCharset(const std::uint8_t* data, std::size_t size, Charset& out) {
    out.glyphs.clear();
    if(!data || !size) return false;
    std::vector<std::string> glyphs;
    for(std::size_t at=0;at<size;) {
        const auto length=sequenceLength(data+at,size-at);
        if(!length) return false;
        glyphs.emplace_back(reinterpret_cast<const char*>(data+at),length);
        at+=length;
    }
    out.glyphs=std::move(glyphs);
    return true;
}

bool parseLoadedCharset(const std::uint8_t* data, std::size_t capacity, Charset& out) {
    out.glyphs.clear();
    if(!data) return false;
    std::vector<std::string> glyphs;
    for(std::size_t at=0;at<capacity && data[at];) {
        const auto length=sequenceLength(data+at,capacity-at);
        if(!length) break; // uninitialised padding after the file
        glyphs.emplace_back(reinterpret_cast<const char*>(data+at),length);
        at+=length;
    }
    if(glyphs.size()<MinimumLoadedGlyphs) return false;
    out.glyphs=std::move(glyphs);
    return true;
}

std::string decode(const std::uint8_t* source, std::size_t length, const Charset& charset) {
    std::string out;
    if(!source) return out;
    for(std::size_t at=0;at<length;) {
        const unsigned byte=source[at];
        if(byte==0x00) break;
        if(byte>=0x30) { append(out,byte-0x30,charset); at+=1; continue; }
        if(byte==0x03) { out+='\n'; at+=1; continue; }
        if(byte==0x01 || byte==0x02 || byte==0x05) { at+=1; continue; }
        if(byte==0x04) {
            // Japanese-table escape: three bytes when the next byte is a page byte.
            at+=(at+1<length && source[at+1]<=0x2f) ? 3 : 2; continue;
        }
        if(byte>=0x2c) {
            if(at+1>=length) break;
            const unsigned next=source[at+1];
            if(next>=0x30) append(out,(byte-0x2b)*0xd0+(next-0x30),charset);
            at+=2; continue;
        }
        at+=2; // 0x06..0x2b: control with one parameter byte
    }
    return out;
}
}
