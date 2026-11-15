// FFX-2 top-level menu fixture. All states and strings are synthetic: no game
// file or extracted asset. See Docs/ffx2-addresses.md (FFX-2-only addresses).
#include "../ffx2_top_menu.hpp"
#include <cstdio>
#include <string>

namespace {
int failures=0;
void expect(bool ok,const char* what) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",what); ++failures; }
}
void expectSpeech(const std::wstring& actual,const std::wstring& expected,const char* what) {
    if(actual!=expected) {
        std::fprintf(stderr,"FAIL: %s: got \"%ls\", expected \"%ls\"\n",what,actual.c_str(),expected.c_str());
        ++failures;
    }
}
ffx::ffx2menu::TopMenuState state(unsigned cursor,unsigned count) {
    ffx::ffx2menu::TopMenuState s;
    s.cursor=cursor; s.count=count;
    for(unsigned i=0;i<ffx::ffx2menu::TopMenuCapacity;++i) s.ids[i]=i;
    return s;
}
}

int main() {
    using namespace ffx::ffx2menu;
    // Names by menu id, in the order of the game's menu_txt name records.
    const wchar_t* expected[]={L"Items",L"White Magic",L"Equip",L"Garment Grids",L"Abilities",
                               L"Accessories",L"Dresspheres",L"Develop",L"Team Members",L"Config"};
    for(unsigned id=0;id<10;++id) {
        const auto* name=topMenuItemName(id);
        expect(name && std::wstring(name)==expected[id],"item name by id");
    }
    expect(topMenuItemName(10)==nullptr && topMenuItemName(0xffffffffu)==nullptr,"unknown ids have no name");

    // Focus: the id stored at the cursor, only inside the list.
    auto s=state(2,8); s.ids[2]=9; unsigned id=0;
    expect(focusedId(s,id) && id==9,"focused id read at the cursor");
    expect(!focusedId(state(8,8),id),"cursor at count refused");
    expect(!focusedId(state(0,0),id),"empty list refused");
    expect(!focusedId(state(0,TopMenuCapacity+1),id),"count above capacity refused");

    // Speak on cursor change only, inside one unchanged list.
    expect(cursorMoved(state(0,8),state(1,8)),"down moves");
    expect(cursorMoved(state(0,8),state(7,8)),"wrap upward moves");
    expect(!cursorMoved(state(3,8),state(3,8)),"same cursor is silent");
    expect(!cursorMoved(state(3,8),state(0,9)),"rebuilt list is silent");
    expect(!cursorMoved(state(3,8),state(9,8)),"cursor outside the list is silent");
    expect(!cursorMoved(state(0,0),state(0,0)),"no list is silent");

    // Speech: "<name>. <help>"; line breaks become spaces; help is UTF-8.
    expectSpeech(topMenuSpeech(0,"Use items."),L"Items. Use items.","name then help");
    expectSpeech(topMenuSpeech(9,""),L"Config","name only when there is no help");
    expectSpeech(topMenuSpeech(3,"First line\nsecond line."),L"Garment Grids. First line second line.",
                 "line break becomes a space");
    expectSpeech(topMenuSpeech(6,"Caf\xC3\xA9 \xE2\x80\xA6"),L"Dresspheres. Caf\x00e9 \x2026","UTF-8 help widened");
    expectSpeech(topMenuSpeech(42,"Help."),L"Help.","unknown id speaks help only");
    expectSpeech(topMenuSpeech(42,""),L"","unknown id without help is silent");
    expectSpeech(topMenuSpeech(1,"  "),L"White Magic","blank help ignored");
    expect(wideFromUtf8("\xF0\x9F\x98\x80")==std::wstring(L"\xD83D\xDE00"),"four-byte UTF-8 becomes a surrogate pair");
    expect(wideFromUtf8("A\xFF" "B")==L"AB","malformed byte dropped");

    if(failures) { std::fprintf(stderr,"FFX-2 top menu fixture: %d failure(s)\n",failures); return 1; }
    std::puts("FFX-2 top menu fixture passed: names, focus, cursor moves, speech.");
    return 0;
}
