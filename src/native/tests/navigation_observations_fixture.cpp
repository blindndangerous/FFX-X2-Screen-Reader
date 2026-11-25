#include "fixture_runtime.hpp"
// Focused observation pressure protocol. The scene is synthetic so this host
// exercises native ticket/queue lifetime without reading retained EV inputs.
#include "../navigation_observations.hpp"
#include "../mod_state.hpp"
#include "test_speech_domains.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace {
unsigned fixtureEpoch=1;
bool owner=true;
uint16_t room=1;
unsigned roomReads=0;
constexpr uintptr_t PromptChunk=0x10,PromptWindow=0x20,PromptText=0x30;
uintptr_t promptSource=PromptText;
bool verifiedPrompt=false;
std::map<unsigned,ffx::Handler> before;
std::vector<std::wstring> said;
std::vector<ffx::nav::QueryObservation> acceptedQueries;
unsigned acceptedScenes=0;
void check(bool value,const char* message) {if(!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void acceptQuery(const ffx::speechq::Header&,const ffx::nav::QueryObservation& q) {acceptedQueries.push_back(q);}
void acceptScene(const ffx::speechq::Header&,const ffx::nav::SceneObservation& s) {
    ++acceptedScenes;check(s.scene.count==1,"observer delivers synthetic owned scene");
    for(const auto& q:acceptedQueries) check(s.requested>=q.requested,"scene demand is no older than every admitted request");
    ffx::nav::navigationWaitingForData(false);
}
void resetObserver(unsigned) {}
void drainThroughOneScenePacket() {
    bool deliveredScene=false;
    while(!ffx::speechq::fixture::pending.empty()) {
        auto item=std::move(ffx::speechq::fixture::pending.front());ffx::speechq::fixture::pending.pop_front();
        const auto found=ffx::speechq::fixture::registrations.find(item.domain);
        if(found!=ffx::speechq::fixture::registrations.end() && found->second.handler) found->second.handler(item.header,item.text);
        unsigned kind=0;if(item.domain==10 && item.text.size()*sizeof(wchar_t)>=sizeof(unsigned)) std::memcpy(&kind,item.text.data(),sizeof(kind));
        if(item.domain==10 && kind==1) {deliveredScene=true;break;}
    }
    check(deliveredScene,"one held scene packet drains independently");
}
unsigned busyNoticeCount() {return static_cast<unsigned>(std::count(said.begin(),said.end(),L"Navigation updates delayed."));}
}

namespace ffx {
std::atomic<int> scene{0};
unsigned generation() {return fixtureEpoch;}
bool nativeGameThread() {return owner;}
bool inBattle() {return false;}
bool nativeReaderResourcesReady() noexcept {return true;}
uintptr_t address(unsigned rva) {
    if(rva==0x14676b0) return rva;
    check(rva==0xd2ca90,"only the current room and rail text are mapped");return reinterpret_cast<uintptr_t>(&room);
}
bool copy(uintptr_t source,void* target,size_t bytes) {
    if(owner && bytes==sizeof(uintptr_t)) {
        if(source==0x14676b0) {std::memcpy(target,&PromptWindow,bytes);return true;}
        if(source==PromptWindow+0x94) {std::memcpy(target,&promptSource,bytes);return true;}
    }
    check(owner && source==reinterpret_cast<uintptr_t>(&room) && bytes==sizeof(room),"room read uses the native-owned fixture adapter");
    ++roomReads;std::memcpy(target,&room,bytes);return true;
}
unsigned autoWalkInputRevision() {return 1;}
bool onAtelPrepared(AtelPreparedHandler) {return true;}
int setting(Setting) {return 1;}
bool hook(unsigned rva,Handler enter,Handler) {before[rva]=enter;return true;}
bool output(const std::wstring& text,bool,const char*) {said.push_back(text);return true;}
bool output(const wchar_t* text,bool,const char*) {said.emplace_back(text);return true;}
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool) {return output(text,interrupt,producer);}
namespace nav {unsigned navigationIntentRevision() {return 1;}}
}
namespace ffx::nav {
void prepareNavigationSceneStorage() {}
bool captureNavigationScene(Scene& captured,unsigned revision,bool) {
    captured.epoch=generation();captured.revision=revision;captured.count=1;
    captured.bevelleRailPromptWorker=verifiedPrompt?40:0;
    captured.primary=PromptChunk;captured.partCount=1;captured.parts[0].span={PromptChunk,captured.epoch,2,0};
    return true;
}
void captureNavigationAreaNames(Scene&,const AreaNameDemand&) {}
}
namespace ffx::speechq {void notifyPreparation() noexcept {}}

int main() {
    using namespace ffx;
    modEnabled=false;
    check(!nav::submitNavigationQuery(nav::Query::GuideExpected) && said.empty(),
        "disabled guide before observer startup stays silent");
    modEnabled=true;
    check(!nav::submitNavigationQuery(nav::Query::GuideExpected) && said.size()==1 &&
        said.back()==L"Navigation is not ready. Try again shortly.","manual guide before observer startup explains unavailable data");
    check(!nav::submitNavigationQuery(nav::Query::GuideCompleted) && said.size()==1,
        "automatic guide completion before startup stays silent");
    nav::startNavigationObservations(acceptQuery,acceptScene,resetObserver);
    modEnabled=false;const auto readsBefore=roomReads;
    check(!nav::submitNavigationQuery(nav::Query::Distance) && !nav::captureRequestedNavigationScene(false,true) && roomReads==readsBefore,
        "disabled navigation refuses requests and capture before native reads");
    modEnabled=true;
    for(unsigned i=0;i<5;++i) check(nav::submitNavigationQuery(nav::Query::Distance),"equal independent requests admitted");
    check(nav::captureRequestedNavigationScene(false),"native owner services whole request burst");
    for(unsigned i=1;i<4;++i) check(nav::captureRequestedNavigationScene(false,true),"held worker capture slot admitted");
    check(nav::navigationSceneSlotsForTest()==4 && !nav::captureRequestedNavigationScene(false,true),"held observer pressure never waits or overwrites a borrowed scene");
    check(busyNoticeCount()==1,"first sustained scene-pressure episode announces once");

    owner=false;drainThroughOneScenePacket();owner=true;
    check(nav::navigationSceneSlotsForTest()==3 && nav::captureRequestedNavigationScene(false,true),"one intermittent capture uses the single drained scene slot");
    check(!nav::captureRequestedNavigationScene(false,true) && busyNoticeCount()==1,"intermittent capture does not rearm speech during sustained scene pressure");
    owner=false;drainSpeechDomainsForTest();owner=true;
    check(acceptedQueries.size()==5 && acceptedScenes==5 && !nav::navigationSceneSlotsForTest(),"equal actions retained FIFO and all capture slots recovered");
    for(size_t i=1;i<acceptedQueries.size();++i) check(acceptedQueries[i].serial>acceptedQueries[i-1].serial,"independent request sequence preserved");

    check(nav::captureRequestedNavigationScene(false,true),"fully drained recovery permits a fresh capture");
    for(unsigned i=1;i<4;++i) check(nav::captureRequestedNavigationScene(false,true),"new pressure episode fills all recovered slots");
    check(!nav::captureRequestedNavigationScene(false,true) && busyNoticeCount()==2,"genuine drained recovery rearms the next pressure notice");
    nav::resetNavigationObservations();
    check(!nav::captureRequestedNavigationScene(false,true) && busyNoticeCount()==3,"scene revision rearms pressure notice while old slots retire");
    owner=false;drainSpeechDomainsForTest();owner=true;
    verifiedPrompt=true;
    const auto prompt=[] {return nav::bevelleRailPromptOwner(PromptChunk,2);};
    check(nav::captureRequestedNavigationScene(false,true) && prompt()==40 &&
        !nav::bevelleRailPromptOwner(PromptChunk,3),"rail prompt witness uses the verified worker and script allocation");
    promptSource=PromptText+1;check(!prompt(),"rail prompt owner is rejected when the live source changes");
    promptSource=PromptText;Event e;before.at(0x4b8980)(e);
    check(!prompt(),"slot zero text installation retires the witness even when window and text addresses are reused");
    check(nav::captureRequestedNavigationScene(false,true) && prompt()==40,"a newly verified prompt restores the witness");
    before.at(0x4b8510)(e);check(!prompt(),"closing the native rail window retires the witness");
    std::puts("PASS: rail prompt witness rejects changed sources, allocation mismatches and reused slot-zero text.");
    std::printf("PASS: navigation observation pressure announcement, intermittent capture, drained recovery and revision reset. Synthetic scenes only; no EV or game data read.\n");
}
