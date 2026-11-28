#include "../ready_game_text.hpp"
#include "../speech_queue.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace ffx {void installTextPlans();void installBattleAnnouncements();void resetBattleAnnouncements();void installSelectionDetails();void battleOpeningMessage(uintptr_t);}
namespace {
thread_local bool nativeOwner=false;
std::atomic<unsigned> allocations{0},frees{0};
std::atomic<bool> hold{true};
HANDLE entered=nullptr,resume=nullptr,wake=nullptr,drained=nullptr;
uint8_t actors[31][0xf90]{};uint8_t players[20][40]{};
uint8_t openingSource[1024]{},battleState[0x200c]{},globalState[8]{},messageWindow[0x20]{};
uintptr_t messageWindows[8]={0,0,0,0,0,0,0,reinterpret_cast<uintptr_t>(messageWindow)};
uintptr_t monsterArray=reinterpret_cast<uintptr_t>(&actors[20]),actorArray=reinterpret_cast<uintptr_t>(actors);
struct Hook {unsigned rva;ffx::Handler enter,leave;};Hook hooks[16]{};unsigned hookCount=0;
void check(bool ok,const char* s){if(!ok){std::fprintf(stderr,"FAIL: %s\n",s);std::exit(1);}}
void encode(uint8_t* out,const wchar_t* s){constexpr wchar_t alphabet[]=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz";while(*s){auto p=std::wcschr(alphabet,*s++);check(p!=nullptr,"fixture glyph");*out++=static_cast<uint8_t>(0x30+p-alphabet);}*out=0;}
Hook find(unsigned r){for(unsigned i=0;i<hookCount;++i)if(hooks[i].rva==r)return hooks[i];check(false,"installed production hook");return {};}
void held(){check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"worker held before drain");}
void release(){nativeOwner=false;hold=false;SetEvent(resume);}
std::wstring take(ffx::speechq::Header* header=nullptr){auto deadline=GetTickCount64()+3000;ffx::speechq::Utterance u;while(!ffx::speechq::take(u)){auto now=GetTickCount64();if(now>=deadline)break;WaitForSingleObject(wake,static_cast<DWORD>(deadline-now));}if(header)*header=u.header;return u.text;}
void opening(const wchar_t* s){encode(openingSource,s);nativeOwner=true;ffx::battleOpeningMessage(reinterpret_cast<uintptr_t>(openingSource));nativeOwner=false;}
}
void* operator new(size_t n){if(nativeOwner)++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{if(nativeOwner&&p)++frees;std::free(p);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,size_t)noexcept{::operator delete(p);}
void operator delete[](void* p,size_t)noexcept{::operator delete(p);}
namespace ffx {
unsigned generation(){return speechq::generation();}int setting(Setting s){return s==Setting::Speech?1:0;}
uintptr_t address(unsigned r){check(nativeOwner,"engine address lookup on native owner");if(r==0x14676b0)return reinterpret_cast<uintptr_t>(messageWindows);if(r==0xd2a8e0)return reinterpret_cast<uintptr_t>(battleState);if(r==0xd33360)return reinterpret_cast<uintptr_t>(globalState);if(r==0xd34460)return reinterpret_cast<uintptr_t>(&monsterArray);if(r==0xd334cc)return reinterpret_cast<uintptr_t>(&actorArray);if(r==0x394030)return reinterpret_cast<uintptr_t>(+[](unsigned id)->uintptr_t{check(nativeOwner,"actor getter on owner");return id<31?reinterpret_cast<uintptr_t>(actors[id]):0;});if(r==0x385070)return reinterpret_cast<uintptr_t>(+[](unsigned id)->uintptr_t{check(nativeOwner,"player getter on owner");return id<20?reinterpret_cast<uintptr_t>(players[id]):0;});check(false,"unused engine getter forbidden");return 0;}
bool copy(uintptr_t a,void* b,size_t n){check(nativeOwner,"bounded native copy owner");std::memcpy(b,reinterpret_cast<const void*>(a),n);return true;}
void prepareCopiedControlSnapshot(CopiedControls& c){c={};}OwnedTextContext copiedControlContext(const CopiedControls&){check(!nativeOwner,"decode on worker");return {};}
std::wstring join(const std::vector<std::wstring>& v,const wchar_t* sep){check(!nativeOwner,"join on worker");std::wstring s;for(const auto& x:v)if(!x.empty()){if(!s.empty())s+=sep;s+=x;}return s;}
bool inBattle(){check(nativeOwner,"battle state on native owner");return true;}void protectSpeech(bool v){speechq::protect(v);}
void resetBattle(){resetBattleAnnouncements();}
uint8_t equippedItem(unsigned,bool){check(false,"unused setter/game API");return 255;}bool readCopiedTextDetail(){return false;}void resetCopiedTextDetail(){}
bool hook(unsigned r,Handler e,Handler l){check(nativeOwner,"hook bootstrap on owner");check(hookCount<16,"fake hook table capacity");hooks[hookCount++]={r,e,l};return true;}
namespace speechq{void captureBeforeDrainForTest(){if(hold.load()){SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}void drainMarker(const Header&,const std::wstring&){SetEvent(drained);}}
}
using namespace ffx;
void gameOverWorkerFixture() {
    nativeOwner=true;resetBattleAnnouncements();nativeOwner=false;opening(L"Before defeat");
    check(take()==L"Enemies: Larva x2, Dinonix" && take()==L"Before defeat","formation protection active before Game Over");
    ResetEvent(entered);ResetEvent(resume);hold=true;
    TextPlan barrier;nativeOwner=true;prepareTextPlan(barrier,"fixture-barrier");
    check(commitTextPlan(barrier),"empty barrier captured");nativeOwner=false;held();
    const unsigned effect=670;std::memcpy(battleState+0x2008,&effect,sizeof(effect));globalState[4]=3;
    Event e;e.rva=0x388e20;e.epoch=generation();
    const auto started=std::chrono::steady_clock::now();
    nativeOwner=true;find(e.rva).enter(e);globalState[4]=4;e.result=4;find(e.rva).leave(e);nativeOwner=false;
    const auto captured=std::chrono::steady_clock::now();
    // Owned capture must survive native storage reuse before preparation runs.
    std::memset(battleState,0,sizeof(battleState));std::memset(globalState,0,sizeof(globalState));
    release();speechq::Header header{};
    check(take(&header)==L"Game Over","Game Over worker emits only the copied presentation event");
    check((header.flags&speechq::Interrupt)!=0,"Game Over interrupts prior battle speech");
    std::printf("METRIC Game Over native capture %.3f ms, event to usable worker utterance/data age %.3f ms; native rendering and audible delivery unmeasured.\n",
        std::chrono::duration<double,std::milli>(captured-started).count(),
        std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count());
}
int main(){for(unsigned i=0;i<31;++i){uintptr_t marker=1;std::memcpy(actors[i],&marker,sizeof(marker));}for(unsigned i=20;i<23;++i){actors[i][0xdc8]=1;actors[i][0x15]=1;}encode(actors[20]+0x540,L"Larva");encode(actors[21]+0x540,L"Larva");encode(actors[22]+0x540,L"Dinonix");entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);drained=CreateEventW(nullptr,TRUE,FALSE,nullptr);check(entered&&resume&&wake&&drained,"barrier handles");nativeOwner=true;installSelectionDetails();installTextPlans();installBattleAnnouncements();nativeOwner=false;check(hookCount==5,"five actual battle announcement hooks installed");speechq::registerDomain(31,speechq::drainMarker,nullptr);check(speechq::start(GetModuleHandleW(nullptr),wake),"real speech worker starts");held();opening(L"Pre-emptive strike");std::memset(openingSource,0,sizeof(openingSource));TextPlan command;nativeOwner=true;prepareTextPlan(command,"battle-command");command.headline=planLiteral(command,L"Attack");check(commitTextPlan(command),"ordinary interrupting command committed");nativeOwner=false;std::memset(actors[20]+0x540,0,40);std::memset(actors[21]+0x540,0,40);std::memset(actors[22]+0x540,0,40);release();check(take()==L"Enemies: Larva x2, Dinonix","formation groups captured names");check(take()==L"Pre-emptive strike","opening follows formation");ffx::speechq::Header commandHeader{};check(take(&commandHeader)==L"Attack","ordinary command follows opening");check((commandHeader.flags&speechq::Interrupt)==0,"Native captured interrupt flag cleared by worker protection phase");
    encode(actors[20]+0x540,L"Yuna");Event ko;ko.rva=0x38c800;ko.args[1]=20;nativeOwner=true;find(ko.rva).enter(ko);actors[20][0xdcc]=1;actors[20][0xdc8]=0;std::memset(actors[20]+0x540,0,40);find(ko.rva).leave(ko);nativeOwner=false;check(take()==L"Yuna: KO","KO uses name copied before death state and source overwrite");
    opening(L"Same");std::memset(openingSource,0,sizeof(openingSource));opening(L"Same");std::memset(openingSource,0,sizeof(openingSource));check(take()==L"Same"&&take()==L"Same","equal subsequent opening lines both emitted");actors[20][0xdc8]=1;actors[20][0xdcc]=0;encode(actors[20]+0x540,L"Larva");encode(actors[21]+0x540,L"Larva");encode(actors[22]+0x540,L"Dinonix");nativeOwner=true;resetBattleAnnouncements();nativeOwner=false;opening(L"");TextPlan releasedCommand;nativeOwner=true;prepareTextPlan(releasedCommand,"battle-command");releasedCommand.headline=planLiteral(releasedCommand,L"After reset");check(commitTextPlan(releasedCommand),"command after reset committed");nativeOwner=false;ffx::speechq::Header resetHeader{};check(take(&resetHeader)==L"After reset","empty opening does not announce a fresh formation; command after reset emitted");check((resetHeader.flags&speechq::Interrupt)!=0,"reset invalidates old protection phase");
    opening(L"Fresh phase");check(take()==L"Enemies: Larva x2, Dinonix" && take()==L"Fresh phase","ready formation can activate protection after the empty-message guard");
    ResetEvent(entered);ResetEvent(resume);ResetEvent(drained);hold=true;TextPlan canceled;nativeOwner=true;prepareTextPlan(canceled,"battle-command");canceled.headline=planLiteral(canceled,L"Canceled");check(commitTextPlan(canceled),"pre-stop command captured");nativeOwner=false;held();speechq::interrupt();unsigned marker=0;check(speechq::captureDomain(31,&marker,sizeof(marker),nullptr,0,generation()),"post-stop drain marker captured");release();check(WaitForSingleObject(drained,3000)==WAIT_OBJECT_0,"worker drained canceled command before marker");check(!speechq::pending(),"Stop cancellation suppresses captured stale command");TextPlan afterStop;nativeOwner=true;prepareTextPlan(afterStop,"battle-command");afterStop.headline=planLiteral(afterStop,L"After Stop");check(commitTextPlan(afterStop),"post-stop command captured");nativeOwner=false;ffx::speechq::Header stopHeader{};check(take(&stopHeader)==L"After Stop","post-stop command emitted");check((stopHeader.flags&speechq::Interrupt)!=0,"Stop cancellation leaves speech protection false");check(!speechq::domainsProtectSpeech(stopHeader),"Stop invalidates active copied formation protection");
    nativeOwner=true;resetBattleAnnouncements();nativeOwner=false;opening(L"Second fresh phase");
    check(take()==L"Enemies: Larva x2, Dinonix" && take()==L"Second fresh phase","new encounter after Stop owns a fresh protection phase");
    check(speechq::domainsProtectSpeech(stopHeader),"copied formation protection active before explicit release");
    speechq::protect(false);check(!speechq::domainsProtectSpeech(stopHeader),"explicit release invalidates the copied phase");TextPlan afterRelease;nativeOwner=true;prepareTextPlan(afterRelease,"battle-command");afterRelease.headline=planLiteral(afterRelease,L"After explicit release");check(commitTextPlan(afterRelease),"command after explicit release captured");nativeOwner=false;ffx::speechq::Header releaseHeader{};check(take(&releaseHeader)==L"After explicit release","command after explicit release emitted");check((releaseHeader.flags&speechq::Interrupt)!=0,"explicit protection release permits next command interruption");
    gameOverWorkerFixture();
    check(allocations.load()==0&&frees.load()==0,"all native event capture paths remain allocation/free free");std::printf("PASS: held formation/opening/command order, protected command flags, copied KO, repeated equal openings, empty-opening guard, Stop cancellation, explicit release, copied Game Over; native allocation/free=%u/%u.\n",allocations.load(),frees.load());return 0;}
