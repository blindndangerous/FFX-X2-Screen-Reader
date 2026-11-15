// Synthetic text only. No executable, archive, extracted charset or game script.
#include "../ffx2_field_text.hpp"
#include <cstdio>
#include <vector>

namespace {
int failures=0;
void expect(bool value,const char* message) {
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);++failures;}
}
ffx::ffx2text::FieldPage page(std::initializer_list<std::uint8_t> source) {
    ffx::ffx2text::Charset charset;
    charset.glyphs.resize(0xd1,".");
    charset.glyphs[0]=" ";charset.glyphs[1]="A";charset.glyphs[2]="B";
    charset.glyphs[3]="\xc3\xa9";charset.glyphs[0xd0]="Z";
    const std::vector<std::uint8_t> bytes(source);
    return ffx::ffx2text::decodeFieldPage(bytes.data(),bytes.size(),charset);
}
}
int main() {
    auto p=page({0x30,0x31,0x03,0x32,0});
    expect(p.valid && p.prompt=="A B" && !p.selector,"current page joins visible lines and trims whitespace");
    for(const auto end:{0,1,2}) {
        p=page({0x31,static_cast<std::uint8_t>(end),0x32,0});
        expect(p.valid && p.prompt=="A","native page boundary never exposes the next page");
    }
    p=page({0x31,0x10,0x30,0x32,0x10,0x31,0x33,0x10,0xff,0});
    expect(p.valid && p.selector && p.prompt=="A" && p.choices[0]=="B" && p.choices[1]=="\xc3\xa9",
           "native 10 row markers separate prompt and UTF-8 choices");
    p=page({0x10,0x4f,0x31,0});
    expect(p.valid && p.choices[31]=="A","last supported choice row remains bounded");
    expect(!page({0x31,0x10,0x50,0x32,0}).valid,"invalid choice row refuses the whole page");
    p=page({0x2c,0x30,0});
    expect(p.valid && p.prompt=="Z","page-prefixed glyph uses the X-2 charset");
    expect(!page({0x2d,0x30,0}).valid,"missing charset glyph refuses incomplete speech");
    for(const auto code:{0x0b,0x12,0x13,0x19,0x2b})
        expect(!page({0x31,static_cast<std::uint8_t>(code),0x30,0x32,0}).valid,
               "unsupported button/name/value controls never become misleading partial speech");
    expect(!page({0x04,0x40,0}).valid,"unsupported Japanese escape is not silently erased");
    p=page({0x31,0x09,0,0x0c,0x40,0x0e,0x40,0x32,0});
    expect(p.valid && p.prompt=="AB","verified visual controls consume their parameter, including zero");
    expect(!page({0x31,0x10}).valid && !page({0x31,0x2c}).valid,"truncated controls fail closed");
    expect(!page({0x31}).valid,"unterminated input fails closed");
    expect(!ffx::ffx2text::decodeFieldPage(nullptr,4,{}).valid,"null input fails closed");
    expect(page({0}).valid && page({0}).prompt.empty(),"empty native page remains silent");
    if(failures) return 1;
    std::puts("FFX-2 field text fixture passed: page boundaries, choices, UTF-8, unsupported controls and bounds.");
    return 0;
}
