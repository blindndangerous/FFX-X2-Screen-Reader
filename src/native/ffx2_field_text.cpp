#include "ffx2_field_text.hpp"
namespace ffx::ffx2text {
namespace {
void flatten(std::string& text) {
    std::string result;
    bool space=false;
    for(const auto c:text) {
        if(c==' ' || c=='\r' || c=='\n' || c=='\t') {space=!result.empty();continue;}
        if(space) result+=' ';
        result+=c;space=false;
    }
    text=std::move(result);
}
}
FieldPage decodeFieldPage(const std::uint8_t* bytes,std::size_t length,const Charset& charset) {
    FieldPage page;
    if(!bytes || !length || length>4096) return page;
    int row=-1;
    auto text=[&]() -> std::string& {return row<0?page.prompt:page.choices[row];};
    for(std::size_t at=0;at<length;) {
        const auto code=bytes[at++];
        if(code<=2) {
            flatten(page.prompt);
            for(auto& choice:page.choices) flatten(choice);
            page.valid=true;return page;
        }
        if(code==3) {text()+=' ';continue;}
        if(code==5) continue;
        // The 39A7B0 control branch changes presentation, not text content.
        if(code==0x09 || code==0x0c || code==0x0e) {
            if(at==length) return {};
            ++at;continue;
        }
        if(code==0x10) {
            if(at==length) return {};
            const auto marker=bytes[at++];
            if(marker==0xff) row=-1;
            else if(marker>=0x30 && marker<0x30+ChoiceLimit) {row=marker-0x30;page.selector=true;}
            else return {};
            continue;
        }
        unsigned index=0;
        if(code>=0x30) index=code-0x30;
        else if(code>=0x2c) {
            if(at==length || bytes[at]<0x30) return {};
            index=(code-0x2b)*0xd0+bytes[at++]-0x30;
        } else return {}; // Unresolved names, values, button glyphs and language escapes.
        if(index>=charset.glyphs.size()) return {};
        text()+=charset.glyphs[index];
    }
    return {}; // A bounded prefix is not a complete current page.
}
}
