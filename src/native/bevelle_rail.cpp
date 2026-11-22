// Bevelle rail auto turn, game thread. The guide publishes the enabled junction
// regions and the phase each needs (bevelle_rail_plan.cpp); this file holds
// Confirm while the car is in a region and that phase is live.
#include "bevelle_rail.hpp"
#include "pedestal.hpp"
#include "pedestal_rvas.hpp"
#include "utility_memory.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "events.hpp"
#include "runtime.hpp"
#include "log_capture.hpp"
#include "puzzle_guide.hpp"
#include "field_controls.hpp"
#include "navigation_observations.hpp"
#include "mod_state.hpp"
namespace ffx {
namespace {
constexpr unsigned PhaseBindings[]={0x0c00000b,0x0c00000c,0x0c00000d,0x0c00000e,0x0c00000f,0x0c000010,0x0c000011};
constexpr unsigned PhaseCount=sizeof PhaseBindings/sizeof PhaseBindings[0];
// Arrow phases last 12 script frames (about 400 ms). Junctions without a
// guard (W42, W43, hub) take the branch of the first held frame in their
// region, so Confirm is held only early in a matching phase and released
// before the cycler can flip under it.
constexpr uint64_t PhaseLateMs=280,PlanStaleMs=3000,PressMs=100,PressRetryMs=1000,RefreshMs=250;
struct Track { int value=-1000; uint64_t since=0; bool seenChange=false; };
Track phases[PhaseCount];
unsigned holding=0;uint64_t pressedAt=0,pressUntil=0;
uint16_t heldAdded=0,pressedAdded=0;
void addInput(unsigned rva,uint16_t& added,unsigned mask) {
    const auto bits=read<uint16_t>(address(rva));
    const auto missing=static_cast<uint16_t>(mask&~bits);
    if(missing && write<uint16_t>(address(rva),bits|missing)) added|=missing;
}
void clearInput(unsigned rva,uint16_t& added) {
    if(added) write<uint16_t>(address(rva),static_cast<uint16_t>(read<uint16_t>(address(rva))&~added));
    added=0;
}
void inject(unsigned mask,bool edge) {
    addInput(pedestal_rva::Held,heldAdded,mask);
    if(edge) addInput(pedestal_rva::Pressed,pressedAdded,mask);
}
void release(const char* reason) {
    clearBevelleRailInput();
    if(holding) recordLog("bevelle rail auto confirm released W%u: %s",holding,reason);
    holding=0;pressUntil=0;
}
void idle(const char* reason) {release(reason);for(auto& t:phases) t={};}
int phaseIndex(unsigned binding) {for(unsigned i=0;i<PhaseCount;++i) if(PhaseBindings[i]==binding) return static_cast<int>(i);return -1;}
// The car leaves the hub when Confirm is accepted and reaches the lower-rail
// junction regions after a fixed ride (live 2026-11-18: W48 7.9 s, W49 9.4 s,
// W50 10.8-10.9 s). The turn missed when the wanted arrow phase was ending as
// the car arrived and took when it began on arrival, so the hub press waits
// for a departure that brings the car in as the wanted phase starts.
constexpr int64_t PhaseMs=400,ArrivalBandMs=100,ResumeMs=5672,FlipGuardMs=70;
// Hub press to junction region entry (live 2026-11-18): W50 10811 from W45 and
// W47, W49 9309-9310 from W45. W48 is estimated from its cue (7874 ms) less
// the 67 ms cue lag seen at W49, so it is not yet measured.
int64_t lowerRailLeadMs(unsigned worker) {return worker==48?7807:worker==49?9309:worker==50?10811:-1;}
// Confirm needs the wanted phase for a few frames early in the region: W50
// took when it began 66 ms after entry and missed when it ended 66 ms after.
constexpr int64_t WantedLeftMs=200;
bool hubDepartureLinesUp(const BevelleRailPlan& plan,uint64_t now,unsigned hub) {
    // While the hub prompt shows, the plan holds only the prompt's row, so the
    // lower-rail turn comes from the objective.
    for(unsigned worker=48;worker<=50;++worker) {
        const BevelleRailRow row{worker,bevelleRailWanted(worker,plan.objective),{}};
        const auto lead=lowerRailLeadMs(row.worker);
        if(lead<0 || row.wanted<0) continue;
        const int index=phaseIndex(bevelleRailPhaseBinding(row.worker));
        if(index<0) return true;
        const auto& g=phases[index];
        if(g.value!=0 && g.value!=1) return false;
        // Live 2026-11-18: the arrow cycler freezes at the hub press and
        // resumes ResumeMs later with the other value on a fresh grid, so
        // the phase at the junction depends only on the value at the press.
        // Skip presses near a flip so the game reads the same value.
        if(g.seenChange && now-g.since>PhaseMs-FlipGuardMs) return false;
        const int64_t elapsed=lead-ResumeMs,flips=elapsed/PhaseMs,into=elapsed%PhaseMs;
        const int resumed=1-g.value,value=flips%2?1-resumed:resumed;
        const bool ok=value==row.wanted?PhaseMs-into>=WantedLeftMs:PhaseMs-into<=ArrivalBandMs;
        static unsigned loggedHub=0;static uint64_t loggedAt=0;
        if(ok) recordLog("bevelle rail auto confirm W%u departs: W%u should arrive in phase %d at %lld ms (last flip %llu ms ago to %d)",hub,row.worker,value,
            static_cast<long long>(into),static_cast<unsigned long long>(now-g.since),g.value);
        if(!ok && (loggedHub!=hub || now-loggedAt>1000)) {
            loggedHub=hub;loggedAt=now;
            recordLog("bevelle rail auto confirm W%u waits: W%u would arrive in phase %d at %lld ms",hub,row.worker,value,static_cast<long long>(into));
        }
        return ok;
    }
    recordLog("bevelle rail auto confirm W%u departs: no lower-rail turn for objective W%u",hub,plan.objective);
    return true;
}
}
void clearBevelleRailInput() {
    clearInput(pedestal_rva::Held,heldAdded);
    clearInput(pedestal_rva::Pressed,pressedAdded);
}
void releaseBevelleRailIfUnavailable() {
    if((holding || bevelleRailAutoActive()) && (!modEnabled.load() || scene || !gameFocused())) {
        setBevelleRailAutoActive(false);idle("field unavailable");
    }
}
void updateBevelleRail() {
    const bool active=setting(Setting::AutoWalk) && gameFocused() && !scene && !modMenuActive() && !utilityConsoleActive() && !inBattle();
    setBevelleRailAutoActive(active);
    BevelleRailPlan plan;
    const auto now=GetTickCount64();
    if(!active) {idle("plan unavailable");return;}
    UtilityScript script;if(!pedestalScript(7,script)) {idle("script unavailable");return;}
    // The guide publishes the plan on script completions, and none fire while
    // a rail prompt waits for Confirm (live 2026-11-18). Refresh it here.
    static uint64_t refreshedAt=0;
    if(now<refreshedAt || now-refreshedAt>=RefreshMs) {refreshedAt=now;puzzleGuideRailRefresh();}
    if(!readBevelleRailPlan(plan) || !plan.count || plan.count>BevelleRailPlan::Rows || plan.epoch!=generation() ||
        now<plan.at || now-plan.at>PlanStaleMs) {idle("plan unavailable");return;}
    for(unsigned i=0;i<PhaseCount;++i) {
        int v=-1;if(!captureUtilityValue(script,PhaseBindings[i],1,0,v)) {idle("phase unavailable");return;}
        auto& t=phases[i];
        if(t.value!=v) {
            t.seenChange=t.value!=-1000;t.value=v;t.since=now;
        }
    }
    const auto player=playerPosition();
    const auto mask=pedestalConfirmMask();
    unsigned visibleSlots=0;
    const bool message=nativeVisibleMessageText(&visibleSlots);
    const auto prompt=visibleSlots==1?nav::bevelleRailPromptOwner(script.chunk,script.allocationSerial):0;
    for(unsigned r=0;r<plan.count;++r) {
        const auto& row=plan.rows[r];
        if(!mask || row.wanted<0 || (message && prompt!=row.worker)) continue;
        InteractionZone zone;static_cast<InteractionShape&>(zone)=row.zone;
        if(!inInteractionZone(zone,player)) continue;
        const bool guarded=row.worker==41 || row.worker==44 || (row.worker>=48 && row.worker<=50);
        int accepted=1;if(!captureUtilityValue(script,row.worker==40?0x0c000014:0x0c00002c,1,0,accepted)) continue;
        if(accepted && !guarded) {if(holding) release("accepted");continue;}
        if(row.worker==40) {
            // W40 tests a pressed edge (1c743): press briefly, retry if ignored.
            if(pressUntil && now<pressUntil) {inject(mask,false);return;}
            if(pressUntil) release("press ended");
            if(now-pressedAt<PressRetryMs) return;
            pressedAt=now;pressUntil=now+PressMs;holding=40;inject(mask,true);
            recordLog("bevelle rail auto confirm W40 press");return;
        }
        const int index=phaseIndex(bevelleRailPhaseBinding(row.worker));
        if(index<0) continue;
        const auto& t=phases[index];
        // Guarded junctions reject Confirm in the wrong phase every frame and
        // accept it once the right phase arrives. A short press timed to the
        // phase was lost at W50 (live 2026-11-18), so hold for the whole region.
        if(guarded) {
            const bool edge=holding!=row.worker;
            if(edge) recordLog("bevelle rail auto confirm W%u hold, phase %d at %.1f,%.1f,%.1f",row.worker,t.value,player.x,player.y,player.z);
            static int lastPhase=-1000,lastAccepted=-1000;
            if(edge || t.value!=lastPhase || accepted!=lastAccepted)
                recordLog("bevelle rail W%u in region phase %d accepted %d",row.worker,t.value,accepted);
            lastPhase=t.value;lastAccepted=accepted;
            holding=row.worker;inject(mask,edge);return;
        }
        const bool fresh=t.seenChange && now-t.since<=PhaseLateMs;
        if(t.value!=row.wanted || !fresh) {if(holding) release(t.value!=row.wanted?"phase changed":"phase late");return;}
        const bool edge=holding!=row.worker;
        if(edge && row.worker>=45 && row.worker<=47 && !hubDepartureLinesUp(plan,now,row.worker)) {if(holding) release("waiting for departure");return;}
        if(edge) recordLog("bevelle rail auto confirm W%u phase %d age %llu ms",row.worker,t.value,static_cast<unsigned long long>(now-t.since));
        holding=row.worker;inject(mask,edge);return;
    }
    if(holding) release(message?"prompt changed":"left region");
}
}
