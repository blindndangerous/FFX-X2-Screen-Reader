#include "fixture_runtime.hpp"
#include "../utility_scripts.hpp"
#include "../utility_trials.hpp"
#include "../utility_trials_data.hpp"
#include "../events.hpp"
#include <array>
#include <map>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

// Production continuation hooks with synthetic native memory/API boundaries.
// The retained-script audit separately establishes which chest owns cleanup.
namespace {
using namespace ffx;
std::array<uint8_t,0x22000> chunk;
std::array<uint8_t,0x500> workerBytes;
std::array<uint8_t,32> signalBytes;
std::array<uint8_t,0x500> callerBytes;
std::array<uint8_t,32> callerSignalBytes;
std::array<uint8_t,32> controllerBytes;
std::map<unsigned,std::pair<Handler,Handler>> hooks;
unsigned epoch=1,serial=1,restores=0,checks=0,requests=0;
bool native=true,finishedOk=false,revokeBeforeRequest=false,revokeBeforeInitialize=false;
bool trialIdentityReady=true;
unsigned treasureFlags=0;
uintptr_t controller=reinterpret_cast<uintptr_t>(controllerBytes.data());
UtilityScript script;
uintptr_t workerAddress() {return reinterpret_cast<uintptr_t>(workerBytes.data());}
uintptr_t signalAddress() {return reinterpret_cast<uintptr_t>(signalBytes.data());}
uintptr_t callerAddress() {return reinterpret_cast<uintptr_t>(callerBytes.data());}
uintptr_t callerSignal() {return reinterpret_cast<uintptr_t>(callerSignalBytes.data());}
bool control() {return (controllerBytes[0]&2)!=0;}
void setControl(bool enabled) {controllerBytes[0]=enabled?2:0;}
void check(bool ok,const char* why) {++checks;if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
template<class T> void put(uintptr_t p,T value) {std::memcpy(reinterpret_cast<void*>(p),&value,sizeof(value));}
uintptr_t __cdecl worker(int global) {return global==0?workerAddress():global==1?callerAddress():0;}
void __cdecl enableControl() {check(native,"control restoration stays on the native owner");++restores;setControl(true);}
void setup(const TrialUtility& trial) {
    ++epoch;chunk.fill(0);workerBytes.fill(0);signalBytes.fill(0);
    callerBytes.fill(0);callerSignalBytes.fill(0);controllerBytes.fill(0);
    setControl(true);finishedOk=false;controller=reinterpret_cast<uintptr_t>(controllerBytes.data());native=true;
    trialIdentityReady=true;treasureFlags=0;
    put(controller+0xc,uint16_t{2});
    script={};script.chunk=reinterpret_cast<uintptr_t>(chunk.data());script.controller=controller;
    script.worker=workerAddress();script.header=script.chunk+0x80;script.length=trial.length;
    script.codeOffset=0x1000;script.allocationSerial=serial;script.name=trial.script;
    const auto& action=trial.collect;
    put(script.chunk+0x30,script.codeOffset);put(script.header,uint8_t{1});
    put(script.header+8,uint16_t{16});put(script.header+0x20,0x100u);
    put(script.chunk+0x100+action.entry*4,action.entryPc);
    put(script.chunk+script.codeOffset+action.returnPc,uint8_t{0x3c});
    put(script.worker,script.header);put(script.worker+4,script.chunk);
    put(script.worker+0x2e,static_cast<uint16_t>(action.worker));
    put(script.worker+0x88,signalAddress());
    put(signalAddress()+8,static_cast<uint16_t>(action.entry));put(signalAddress()+14,uint8_t{1});
}
void begin(const TrialUtility& trial) {setup(trial);runUtilityScript(script,trial.collect);}
void finish(const ScriptAction& action,bool exact=true,bool nextSignalDisables=false) {
    const auto level=read<uint8_t>(signalAddress()+14)&15u;
    put(script.worker+0x12c+level*0x4c+0x18,script.chunk+script.codeOffset+(exact?action.returnPc:action.pc));
    Event event;event.args[0]=0;
    hooks.at(0x469170).first(event);
    // Native release may dispatch the next scene before the after hook runs.
    put(script.worker+0x80,uintptr_t{0});put(script.worker+0x90,uintptr_t{0});
    if(nextSignalDisables) setControl(false);
    hooks.at(0x469170).second(event);
}
unsigned crossing(const ScriptAction& chest,unsigned source=1,unsigned callerEntry=6,unsigned callerPc=0x1cde2) {
    // W54:6 has already disabled control and begun its native route when it
    // reaches REQEW W16:8. Model that child's exact native initialization.
    setControl(false);
    put(callerAddress()+4,script.chunk);
    put(callerAddress()+0x2e,uint16_t{54});
    put(callerAddress()+0x90,callerSignal());
    put(callerSignal()+8,static_cast<uint16_t>(callerEntry));
    put(signalAddress()+10,static_cast<uint16_t>(source));
    put(signalAddress()+14,uint8_t{3});
    put(script.worker+0x90,signalAddress());
    const auto code=script.chunk+script.codeOffset;
    put(callerSignal()+14,uint8_t{2});
    put(callerAddress()+0x32,uint8_t{2});
    put(callerAddress()+0x12c+2*0x4c+0x18,code+callerPc);
    put(code+0x1cde2,uint8_t{0x38});
    const auto pc=script.worker+0x12c+3*0x4c+0x18;
    put(pc,code+chest.entryPc);
    Event event;event.args[0]=script.worker;event.args[1]=signalAddress();
    hooks.at(0x469040).second(event);
    return static_cast<unsigned>(read<uintptr_t>(pc)-code);
}
void ownershipFixtures(const TrialUtility& bevelle) {
    setup(bevelle);setControl(false);const auto baselineRequests=requests;
    bool blocked=false;
    try {runUtilityScript(script,bevelle.collect);}
    catch(const std::runtime_error&) {blocked=true;}
    check(blocked && !control() && !utilityBusy() && requests==baselineRequests,
        "a message-free rail scene with field control disabled blocks direct collection");
    setup(bevelle);revokeBeforeRequest=true;
    runUtilityScript(script,bevelle.collect);revokeBeforeRequest=false;
    check(!control() && !utilityBusy() && requests==baselineRequests,
        "a rail scene taking control before deferred dispatch cancels the chest without sending a request");
    setup(bevelle);revokeBeforeInitialize=true;const auto count=restores;
    runUtilityScript(script,bevelle.collect);revokeBeforeInitialize=false;
    finish(bevelle.collect);
    check(!control() && restores==count && finishedOk,
        "a rail scene taking control after request submission retains it through chest completion");
}
void repeatFixtures(const TrialUtility& bevelle) {
    const auto& chest=bevelle.collect;
    setup(bevelle);treasureFlags=0x20;
    check(crossing(chest)==chest.entryPc,"an unopened native chest runs its original reward");
    treasureFlags|=bevelle.mask;
    check(crossing(chest)==chest.entryPc,"native W54:6 keeps its reward when the player never used direct collection");
    begin(bevelle);
    const auto count=restores;
    treasureFlags=0x20|bevelle.mask; // The native W16 body records collection.
    finish(chest);
    check(finishedOk && restores==count+1,"direct collection completes before the physical crossing");
    check(crossing(chest)==chest.returnPc,"physical crossing after direct collection enters the child RET instead of the reward body");
    finish(chest);
    check(read<uintptr_t>(callerAddress()+0x90)==callerSignal() && !control() &&
        restores==count+1 && treasureFlags==(0x20|bevelle.mask),
        "the repeated child returns without touching W54's signal, control, or treasure flags");
    // W54's unmodified caller continuation owns its later control restore.
    enableControl();
    check(control() && restores==count+2,"physical caller resumes and restores control through its own continuation");
    check(crossing(chest,1,6,0x1cde3)==chest.entryPc,"a different W54:6 caller PC retains the original entry");
    check(crossing(chest,0)==chest.entryPc,"another caller retains the ordinary native entry");
    check(crossing(chest,1,5)==chest.entryPc,"another W54 entry retains the ordinary native entry");
    trialIdentityReady=false;
    check(crossing(chest)==chest.entryPc,"missing ready identity leaves the original native entry");
    trialIdentityReady=true;native=false;
    check(crossing(chest)==chest.entryPc,"foreign owner leaves the original native entry");
    native=true;++serial;script.allocationSerial=serial;
    check(crossing(chest)==chest.entryPc,"same-address replacement cannot inherit direct collection");
    begin(bevelle);treasureFlags=bevelle.mask;finish(chest);++epoch;
    check(crossing(chest)==chest.entryPc,"a later epoch keeps the native reward");
}
}
namespace ffx {
uintptr_t address(unsigned rva) {
    if(rva==0x46a7c0) return reinterpret_cast<uintptr_t>(worker);
    if(rva==0x471a50) return reinterpret_cast<uintptr_t>(enableControl);
    if(rva==0xf26ae8) return reinterpret_cast<uintptr_t>(&controller);
    std::abort();
}
bool copy(uintptr_t from,void* to,size_t size) {if(from<0x10000)return false;std::memcpy(to,reinterpret_cast<void*>(from),size);return true;}
unsigned generation() {return epoch;}
bool nativeGameThread() {return native;}
bool hook(unsigned rva,Handler before,Handler after) {hooks[rva]={before,after};return true;}
bool nativeAtelSpan(uintptr_t p,AtelSpan& span) {span={p,epoch,serial,static_cast<unsigned>(chunk.size())};return p==script.chunk;}
uintptr_t utilityWorker(const UtilityScript&,unsigned) {return workerAddress();}
bool captureUtilityScript(UtilityScript& result) noexcept {result=script;return true;}
const TrialUtility* captureCurrentTrial(const UtilityScript&) noexcept {return trialIdentityReady?&trialUtilities[4]:nullptr;}
bool captureUtilityValue(const UtilityScript&,unsigned low,unsigned high,unsigned element,int& value) noexcept {
    if(low!=0x01000a70 || high!=1 || element) return false;
    value=static_cast<int>(treasureFlags);return true;
}
void utilityRequest(const UtilityScript&,unsigned,unsigned,FixedCallback<void(bool),64> done,FixedCallback<bool(),64> qualified,bool,bool) {
    if(revokeBeforeRequest) {
        setControl(false);
        if(!qualified()) {done(false);return;}
    }
    check(qualified(),"native continuation reservation qualifies");
    ++requests;
    if(revokeBeforeInitialize) setControl(false);
    put(script.worker+0x80,signalAddress());put(script.worker+0x90,signalAddress());
    Event event;event.args[0]=script.worker;event.args[1]=signalAddress();hooks.at(0x469040).second(event);done(true);
    setControl(false); // Original chest body takes control to display the reward.
}
void utilityReply(const wchar_t*) {}
void puzzleCompleted(uintptr_t,uintptr_t,bool completed,bool owned) {finishedOk=completed&&owned;}
void viaScriptCompleted(bool,bool) {}
void puzzleTileCompleted(uintptr_t,unsigned) {}
void puzzleGuideCompleted() {}
}
int main(int argc,char** argv) {
    ffx_fixture::initializeRuntime();installUtilityScripts();
    const auto& bevelle=trialUtilities[4];
    if(argc==2 && std::strcmp(argv[1],"ownership")==0) {ownershipFixtures(bevelle);return 0;}
    if(argc==2 && std::strcmp(argv[1],"repeat")==0) {repeatFixtures(bevelle);return 0;}
    begin(bevelle);const auto before=restores;
    check(!control() && utilityBusy() && restores==before,"chest dialogue keeps native control disabled until final return");
    finish(bevelle.collect);
    check(control() && restores==before+1 && finishedOk && !utilityBusy(),"Bevelle final chest return restores native control once");
    for(unsigned fault=0;fault<8;++fault) {
        begin(bevelle);const auto count=restores;
        if(fault==1) ++epoch;
        if(fault==2) ++serial;
        if(fault==3) controller=2;
        if(fault==4) put(signalAddress()+8,uint16_t{7});
        if(fault==5) put(script.worker+4,script.chunk+4);
        if(fault==6) native=false;
        if(fault==7) put(script.worker+0x90,uintptr_t{0});
        finish(bevelle.collect,fault!=0);
        check(!control() && restores==count,"interrupted, stale, replaced, foreign-controller/entry/chunk completion cannot restore control");
    }
    begin(bevelle);const auto next=restores;finish(bevelle.collect,true,true);
    check(!control() && restores==next+1,"next native scene retains disabled control after chest cleanup");
    finish(bevelle.collect);
    check(!control() && restores==next+1,"a repeated completion cannot restore control twice");
    for(unsigned n:{0u,1u,2u,3u,5u}) {
        begin(trialUtilities[n]);const auto count=restores;
        // These original continuations execute Common005D themselves.
        setControl(true);finish(trialUtilities[n].collect);
        check(control() && restores==count,"other temples retain their native control restoration without a duplicate call");
    }
    ownershipFixtures(bevelle);
    repeatFixtures(bevelle);
    std::printf("PASS: utility control completion, six temples, ownership and lifetime guards, next native scene, and repeated Bevelle crossing (%u checks).\n",checks);
}
