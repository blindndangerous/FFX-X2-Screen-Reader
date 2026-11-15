// FFX-2 text decoder fixture. Every table and string here is synthetic: no
// game file or extracted asset. See Docs/ffx2-addresses.md (Text encoding).
#include "../ffx2_text.hpp"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
int checks=0,failures=0;
void expect(bool ok,const char* what) {
    ++checks;
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",what); ++failures; }
}
void expectText(const std::vector<unsigned char>& bytes,const ffx::ffx2text::Charset& charset,
                const std::string& expected,const char* what) {
    ++checks;
    const auto actual=ffx::ffx2text::decode(bytes.data(),bytes.size(),charset);
    if(actual!=expected) {
        std::fprintf(stderr,"FAIL: %s: got \"%s\", expected \"%s\"\n",what,actual.c_str(),expected.c_str());
        ++failures;
    }
}
// Synthetic table: index 0 '0' .. 9 '9', 10 ' ', 11 'A', 12 'B', 13 U+00E9
// (2 bytes), 14 U+2026 (3 bytes), 15 'C'. Entries 16..0xcf are '.', and
// index 0xd0 is 'Z' so the 0x2c page can be tested. Entries are code points,
// so after the wide entries the byte offset no longer equals the index.
std::vector<unsigned char> syntheticTable() {
    std::string points="0123456789 AB\xC3\xA9\xE2\x80\xA6" "C";
    for(unsigned i=16;i<0xd0;++i) points.push_back('.');
    points.push_back('Z');
    return {points.begin(),points.end()};
}
}

int main() {
    using namespace ffx::ffx2text;
    const auto tableBytes=syntheticTable();
    Charset charset;
    expect(parseCharset(tableBytes.data(),tableBytes.size(),charset),"synthetic table parses");
    expect(charset.glyphs.size()==0xd1,"table has one entry per code point");
    expect(charset.glyphs.size()>14 && charset.glyphs[13]=="\xC3\xA9","two-byte UTF-8 entry kept whole");
    expect(charset.glyphs.size()>14 && charset.glyphs[14]=="\xE2\x80\xA6","three-byte UTF-8 entry kept whole");

    // Single-byte characters: index = byte - 0x30.
    expectText({0x3b,0x3c,0x3a,0x31},charset,"AB 1","single-byte characters");
    expectText({0x3d,0x3e,0x3f},charset,"\xC3\xA9\xE2\x80\xA6" "C","wide entries after narrow ones");
    // 0x00 ends the string even inside the buffer.
    expectText({0x3b,0x00,0x3c},charset,"A","NUL ends the string");
    // 0x03 is the line break (the game writes CR LF).
    expectText({0x3b,0x03,0x3c},charset,"A\nB","0x03 is a line break");
    // 0x01, 0x02 and 0x05 are one-byte controls with no output.
    expectText({0x3b,0x01,0x02,0x05,0x3c},charset,"AB","one-byte controls skipped");
    // 0x06..0x2b take one parameter byte and produce no output.
    expectText({0x3b,0x06,0x99,0x3c},charset,"AB","0x06 control skips its parameter");
    expectText({0x3b,0x2b,0x31,0x3c},charset,"AB","0x2b control skips its parameter");
    // 0x04 is the Japanese-table escape: two bytes, three if the next byte is <= 0x2f.
    expectText({0x3b,0x04,0x40,0x3c},charset,"AB","0x04 escape skips one byte");
    expectText({0x3b,0x04,0x2f,0x40,0x3c},charset,"AB","0x04 escape skips two bytes");
    // 0x2c..0x2f select a page: index = (b - 0x2b) * 0xd0 + next - 0x30.
    expectText({0x2c,0x30},charset,"Z","0x2c page selects index 0xd0");
    expectText({0x2d,0x30,0x3b},charset,"A","index past the table produces nothing");
    // Truncated input never reads past the length.
    expectText({0x3b,0x06},charset,"A","truncated control");
    expectText({0x3b,0x2c},charset,"A","truncated page prefix");
    expectText({0x3b,0x04,0x2f},charset,"A","truncated 0x04 escape");
    expect(decode(nullptr,4,charset).empty(),"null source decodes to empty");

    // Malformed tables are refused.
    Charset bad;
    const unsigned char truncated[]={0x41,0xE2,0x80};
    expect(!parseCharset(truncated,sizeof truncated,bad),"truncated UTF-8 refused");
    const unsigned char stray[]={0x41,0x80,0x42};
    expect(!parseCharset(stray,sizeof stray,bad),"stray continuation byte refused");
    const unsigned char overlong[]={0xC0,0x80};
    expect(!parseCharset(overlong,sizeof overlong,bad),"overlong encoding refused");
    expect(!parseCharset(nullptr,0,bad),"empty table refused");

    // The game's loaded copy (0x111120) ends with a NUL after the file padded
    // to 16 bytes; the padding bytes are not initialised. parseLoadedCharset
    // keeps entries up to the first NUL or malformed byte, and refuses a
    // table too short to hold the ASCII range.
    {
        auto loaded=tableBytes;
        loaded.push_back(0x00); loaded.push_back(0x41);
        Charset game;
        expect(parseLoadedCharset(loaded.data(),loaded.size(),game) && game.glyphs.size()==0xd1,
               "loaded table stops at NUL");
        auto padded=tableBytes;
        padded.push_back(0xff); padded.push_back(0x41); padded.push_back(0x00);
        expect(parseLoadedCharset(padded.data(),padded.size(),game) && game.glyphs.size()==0xd1,
               "loaded table stops at uninitialised padding");
        expect(parseLoadedCharset(tableBytes.data(),tableBytes.size(),game) && game.glyphs.size()==0xd1,
               "loaded table stops at the capacity");
        const unsigned char tiny[]={0x30,0x31,0x32,0x00};
        expect(!parseLoadedCharset(tiny,sizeof tiny,game) && game.glyphs.empty(),"short loaded table refused");
        expect(!parseLoadedCharset(nullptr,16,game),"null loaded table refused");
    }

    if(failures) { std::fprintf(stderr,"FFX-2 text fixture: %d failure(s)\n",failures); return 1; }
    std::printf("FFX-2 text fixture: %d checks, %d failures.\n",checks,failures);
    return 0;
}
