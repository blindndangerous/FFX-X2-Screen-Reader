#include "puzzle_guide_owned.hpp"
#include "puzzle_guide.hpp"
#include "puzzle_guide_data.hpp"
#include "bevelle_boarding.hpp"
#include "puzzle_guide_progress.hpp"
#include "puzzle_tiles.hpp"
#include "floor_panels.hpp"
#include "puzzle_tiles_data.hpp"
#include "zanarkand_patterns.hpp"
#include "utility_trials_data.hpp"
#include "navigation_intents.hpp"
#include "nav_audio.hpp"
#include "nav_format.hpp"
#include "butterfly_navigation.hpp"
#include "settings.hpp"
#include "process_storage.hpp"
#include "navigation_cancel.hpp"
#include "camera_direction.hpp"
#include "navigation_order.hpp"
#include "log_capture.hpp"
#include "bevelle_rail.hpp"
#include "hotkey_names.hpp"
#include "dev_pipe.hpp"
#include <cstdio>
#include <atomic>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <cstring>
namespace ffx {
namespace {
struct ManualPush {int phase=-1;Point position{};bool moving=false;};
struct State {unsigned epoch=0,revision=0,step=0,expected=0,part=0,begin=0,end=0,railWindow=0;const PuzzleRoute* route=nullptr;int pattern=-1,progress=-1,railPhase=0;std::uint64_t placed=0,removed=0,railLogAt=0;ManualPush manualPush;};
ProcessStorage<State> storage;
std::atomic<unsigned> revision{1};std::atomic<bool> enabled{false};
std::atomic<bool> refreshPending{false};
int value(const nav::Scene& observation,const State& state,unsigned low,unsigned high=1,unsigned element=0) {
    const auto& facts=observation.parts[state.part];if(!facts.guideReady) throw std::runtime_error("The puzzle observations are unavailable");
    for(unsigned i=0;i<facts.bindings.count;++i) {const auto& row=facts.bindings.rows[i];if(row.binding==low && row.high==high && row.element==element) return row.value;}
    throw std::runtime_error("Required puzzle observation is missing");
}
// W15 finishes the ascending ride at node 64, pedestal 14 (134d6/134dc).
// W53 calls the pedestal to the upper platform (15) before the optional ride.
bool bevelleFinalLanding(const nav::Scene& observation,const State& state) {
    const int node=value(observation,state,0x0c000008);
    const int pedestal=value(observation,state,0x0c000009);
    return (node==64 && (pedestal==14 || pedestal==15)) || (pedestal==16 && node>=64 && node<=69) || (pedestal==17 && node==69);
}
// djyt0600 has no flag for step 10 (reset tile 67 writes only 0xd9=0, which
// is already 0 there) or step 28 (the central lift). Native witnesses: the
// lower pedestal W1 away from the charger (the step 9 push leaves it at
// x 70, pedestal_rules; the reset returns it to x 0, and later it only moves
// along x 0 into the gap or up with the lift), and the player on the upper
// floor (y -65; the lower floor is -18), which only the lift reaches. Without
// them the guide stayed on those steps and F10 was needed (live 2026-11-21).
bool djoseWitness(const nav::Scene& observation,const State& state,const PuzzleStep& step) {
    if(step.worker==63) return observation.player.y<-40;
    if(step.worker!=67) return false;
    for(unsigned i=0;i<observation.count;++i) {const auto& worker=observation.workers[i];
        if(worker.valid && worker.part==state.part && worker.local==1) return worker.position.x<35;}
    return false;
}
bool done(const nav::Scene& observation,const State& state,const PuzzleStep& step) {if(!step.check) return false;
    if(!step.done.low) {
        if(!std::strcmp(state.route->script,"djyt0600")) return djoseWitness(observation,state,step);
        if(std::strcmp(state.route->script,"klyt1000") || step.worker!=41) return false;
        for(unsigned i=0;i<observation.count;++i) {const auto& worker=observation.workers[i];
            if(worker.valid && worker.part==state.part && worker.local==6) return kilikaResetReached(worker.position);}
        return false;
    }
    const auto current=value(observation,state,step.done.low,step.done.high,step.done.element);
    // A collected chest may skip the rail objectives only after native arrival.
    // The prize persists across visits. The landing evidence is room-local.
    if(step.worker==16 && !std::strcmp(state.route->script,"bvyt1100"))
        return (current&step.done.value)!=0 && bevelleFinalLanding(observation,state);
    // mcyt0500: Destruction still in the lower treasure recess is room-local
    // evidence too, before the one-shot 0xcd flag is seen.
    if(step.worker==19 && !std::strcmp(state.route->script,"mcyt0500") && value(observation,state,0x0c000099)==10)
        return (current&step.done.value)!=0;
    if(step.also.low && value(observation,state,step.also.low,step.also.high,step.also.element)!=step.also.value) return false;
    // bvyt1100 starts at node 0 (W0 pc f4). The entrance push writes 1
    // (W13 pc 4b00), then riding writes 19 (4c7d) and later rail nodes.
    // follow() may miss 1 while settling; any nonzero node proves departure.
    if(!std::strcmp(state.route->script,"bvyt1100") && step.done.low==0x0c000008 && step.done.value==1) return current!=0;
    return step.done.low==0x01000a70?(current&step.done.value)!=0:current==step.done.value;}
void advance(const nav::Scene& observation,State& state) {
    PuzzleProgress progress{state.expected,state.placed,state.removed};
    advancePuzzleProgress(*state.route,state.begin,state.end,progress,[&](const PuzzleStep& step) {return done(observation,state,step);},
        [&](const PuzzleStep& step) {if(!step.done.low) return false;const auto current=value(observation,state,step.done.low,step.done.high,step.done.element);
            return step.done.low==0x01000a70?(current&step.done.value)==0:current==0;});
    state.expected=progress.expected;state.placed=progress.placed;state.removed=progress.removed;
    // Kilika W36 keeps 0x63=3 after the first removal, but reinsertion fills
    // 0x62 and relights the door. This final clearance is not a durable pass.
    if(!std::strcmp(state.route->script,"klyt1000") && state.expected==21 && !done(observation,state,state.route->steps[20])) {
        state.expected=20;state.placed&=~(std::uint64_t{1}<<20);state.removed&=~(std::uint64_t{1}<<20);
    }
}
bool context(const nav::Scene& observation,State& state) {
    const auto current=revision.load(std::memory_order_acquire);const PuzzleRoute* route=nullptr;unsigned selected=observation.partCount;
    for(unsigned part=0;part<observation.partCount;++part) {const auto& fact=observation.parts[part];if(fact.span.chunk!=observation.primary) continue;AtelIdentity identity;
        if(!readyAtelIdentity(fact.span.chunk,identity) || identity.span.serial!=fact.span.serial) continue;
        bool verified=false;for(const auto& trial:trialUtilities) if(identity.length==trial.length && identity.checksum==trial.checksum && !std::strcmp(identity.name,trial.script)) {verified=true;break;}
        if(!verified) continue;for(const auto& candidate:puzzleRoutes) if(!std::strcmp(candidate.script,identity.name)) {route=&candidate;selected=part;break;}}
    if(!route || route->steps.empty()) return false;
    state.part=selected;unsigned begin=0,end=static_cast<unsigned>(route->steps.size());
    if(route->returnStep) {
        const auto visit=zanarkandPhase(value(observation,state,0x20000a00));
        if(visit==ZanarkandPhase::FirstVisit) end=route->returnStep;
        else if(visit==ZanarkandPhase::GateOpen) {begin=route->returnStep-1;end=route->returnStep;}
        else if(visit==ZanarkandPhase::ReturnVisit) begin=route->returnStep;
        else end=0;
    }
    if(state.epoch!=observation.epoch || state.revision!=current || state.route!=route || state.begin!=begin || state.end!=end) {state.step=state.expected=begin;state.pattern=state.progress=-1;state.placed=state.removed=0;state.railWindow=0;state.railPhase=0;state.manualPush={};state.epoch=observation.epoch;state.revision=current;state.route=route;state.begin=begin;state.end=end;}
    return true;
}
void synchronizeTiles(const nav::Scene& observation,State& state,unsigned local=0) {
    const int pattern=value(observation,state,0x5c000004),progress=value(observation,state,pattern==0?0x0c000022:pattern==5?0x0c000029:0x0c000024);
    const bool reset=state.pattern==pattern && progress<state.progress;
    if(local || reset || (state.pattern>=0 && state.pattern!=pattern)) {
        unsigned channels=0;bool found=false;
        for(unsigned i=0;i<observation.count;++i) {const auto& row=observation.workers[i];
            if(row.valid && row.part==state.part && row.local==local) {found=true;channels=row.interactions.channels;break;}}
        const auto now=GetTickCount64();
        recordLog("Zanarkand floor event=%s worker=%u stage=%d previous-stage=%d count=%d previous-count=%d reset-observed=%u milestone=%d quotas=%d/%d/%d/%d found=%u channels=0x%x age-ms=%llu",
            local?"tile-completion":"state-change",local,pattern,state.pattern,progress,state.progress,reset,value(observation,state,0x0c000020),
            value(observation,state,0x0c000025),value(observation,state,0x0c000026),value(observation,state,0x0c000027),value(observation,state,0x0c000028),
            found,channels,observation.capturedAt && now>=observation.capturedAt?now-observation.capturedAt:0);
    }
    state.pattern=pattern;state.progress=progress;
}
constexpr const wchar_t* floorWalkWarning=L"Walk manually around other active panels. A wrong panel resets the pattern.";
bool activeFloorPanel(const nav::WorkerFact& worker) {
    return worker.type==6 && (worker.interactions.channels&0x20) && worker.interactions.entries[5]==2;
}
bool tile(const nav::Scene& observation,State& state,Point& point,std::wstring& label,unsigned& local) {
    if(std::strcmp(state.route->script,"dome0700")) return false;synchronizeTiles(observation,state);const PuzzleTile* target=nullptr;double best=std::numeric_limits<double>::infinity();
    // A lit floor panel turns its own touch shape off (dome0700 W58 a9a2
    // setCollisionShapeActive(0)), so the live scene shows it with no interaction
    // channel. Panels the session never saw lit (guide reset mid-pattern) are
    // skipped that way too. Use current ready facts, not completion receipts:
    // a reset can rearm a panel and restore the same count between captures.
    for(const auto& candidate:puzzleTiles) {nav::checkComputation();if(static_cast<int>(candidate.stage)!=state.pattern) continue;
        bool active=false;for(unsigned i=0;i<observation.count;++i) {const auto& fact=observation.workers[i];
            if(fact.valid && fact.part==state.part && fact.local==candidate.worker) {
                active=activeFloorPanel(fact);break;
            }}
        if(!active) continue;
        bool blocked=!candidate.blocked.empty();
        for(const auto& guard:candidate.blocked) if(value(observation,state,guard.low,guard.high,guard.element)!=guard.value) blocked=false;if(blocked) continue;
        const auto distance=distanceXZ(observation.player,candidate.point);if(distance<best) {best=distance;target=&candidate;}}
    if(!target) {label=L"No active permitted floor panel is available in the current observations. Wait for the scene to finish, then read the step again.";return false;}
    local=target->worker;point=target->point;label=L"Pattern "+std::to_wstring(state.pattern)+L". Next permitted floor panel: "+zanarkandGlyphLabel(target->worker)+L".";return true;
}
// Djose electric gap: with the pedestal dropped in it (0xd9=1) the game
// carries the player across by a scripted jump. The gap floor (collision
// group 50) stays blocked, so no walking route crosses. Live: holding
// forward at x 0 from z 74.6 landed the player at z 162.9, and back again.
// A step across the gap routes to the jump point on the player's side.
constexpr wchar_t djoseGapJumpLabel[]=L"Gap jump point";
bool djoseGapJump(const nav::Scene& observation,const State& state,const PuzzleStep& step,Point& stage) {
    if(std::strcmp(state.route->script,"djyt0600")) return false;
    try {if(value(observation,state,0x0c0000d9)!=1) return false;} catch(...) {return false;}
    constexpr double gap=117;
    const bool south=observation.player.z<gap;
    if(south==(step.fallback.z<gap) || observation.player.y<-40) return false;
    stage=south?Point{0,-18,74.6}:Point{0,-18,162.9};
    return true;
}
Entity target(const nav::Scene& observation,const State& state,const PuzzleStep& step,const std::vector<Entity>& entities) {
    Entity result;result.name=step.label;
    if(Point stage;djoseGapJump(observation,state,step,stage)) {
        // A point target with no zone: the walk intent is point-only, and it
        // rejects a route that ends anywhere but the stage (live, build 53).
        result.name=djoseGapJumpLabel;result.position=stage;return result;
    }
    // Djose central lift: worker 63 at the back door is only a camera hint.
    // Once the lift circuit is on (0xdb=1) the lower pedestal (W1) is the
    // lift. Live: standing still on top of it carried player and pedestal
    // from Y -18 to -65; walking over it did not. Stop near its centre.
    if(!std::strcmp(state.route->script,"djyt0600") && step.worker==63) {
        if(value(observation,state,0x0c0000db)!=1) throw std::runtime_error("This control is not positioned yet; perform the preceding step first");
        for(unsigned i=0;i<observation.count;++i) {const auto& fact=observation.workers[i];
            if(!fact.valid || fact.part!=state.part || fact.local!=1 || fact.actor<0) continue;
            const auto p=fact.position;if(!(std::abs(p.x)<50000 && std::abs(p.z)<50000)) break;
            // A copied point, not the pedestal entity: the live pedestal row
            // is inactive once the lift circuit is on, and the route refresh
            // dropped the walk as "destination inactive" (live, build 53).
            // The centre is inside the pedestal body: a walk there stalled at
            // its north edge (6.5 out) for the 15 s no-progress cap, and the
            // lift rose from there (live, build 54). Stop at that north edge.
            result.position=p;result.position.z+=6.6;return result;
        }
        throw std::runtime_error("This control is not positioned yet; perform the preceding step first");
    }
    const bool push=step.platform || (!std::strcmp(state.route->script,"dome0700") && step.worker>=44 && step.worker<=47);
    if(push) configurePedestalTarget(result,state.route->script,step.worker);
    for(const auto& found:entities) if(found.worker>=0 && static_cast<unsigned>(found.worker)<observation.count) {
        const auto& fact=observation.workers[found.worker];if(fact.part==state.part && fact.local==step.worker && (found.pushControl!=0)==(result.pushControl!=0)) {result=found;result.name=step.label;return result;}}
    for(unsigned i=0;i<observation.count;++i) {const auto& fact=observation.workers[i];if(!fact.valid || fact.part!=state.part || fact.local!=step.worker) continue;
        bool positioned=(fact.type>=1 && fact.type<=3) || fact.type==5 || fact.type==6;
        if(fact.type==1) {uint16_t alive=0;if(fact.actor<0) positioned=false;else std::memcpy(&alive,observation.actors[fact.actor].bytes.data()+2,2);positioned=positioned && alive;}
        const auto p=fact.position;if(positioned && std::abs(p.x)<50000 && std::abs(p.z)<50000 && (p.x || p.y || p.z)) {result.position=p;result.worker=i;result.actor=fact.actor;result.id=0xfffe;if(fact.actor>=0) {uint16_t model=0;std::memcpy(&model,observation.actors[fact.actor].bytes.data(),2);result.id=model;}result.zone=nav::sceneInteractionZone(fact,fact.interactions.channels&1,observation.extraTalkRange);return result;}}
    if(!result.pushControl && (step.fallback.x || step.fallback.y || step.fallback.z)) {result.position=step.fallback;return result;}
    throw std::runtime_error("This control is not positioned yet; perform the preceding step first");
}
std::wstring controlName(const nav::Scene& observation,unsigned raw,const wchar_t* fallback) {
    auto controls=observation.controls;controls.swap=false;const auto names=copiedControlContext(controls);
    const unsigned masks[]={0x10,0x20,0x40,0x80,4,8,1,2,0x800,0x100};std::vector<std::wstring> choices;
    for(unsigned i=0;i<_countof(masks);++i) if(raw&masks[i]) {const auto& name=names.glyphs[0x30+i];if(!name.empty() && name!=L"unassigned control") choices.push_back(name);}
    for(unsigned i=0;i<4;++i) if(raw&(0x1000u<<i)) {const auto& name=names.glyphs[0x40+(1u<<i)];if(!name.empty() && name!=L"unassigned control") choices.push_back(name);}
    return choices.empty()?std::wstring(fallback):join(choices,L" or ");
}
// step is the guide step that wants this push; its own signal decides when
// the push is done, never the pedestal position.
std::wstring instruction(const nav::Scene& observation,const Entity& target,const PuzzleStep* step=nullptr,bool stepDone=false) {
    if(!target.pushControl) return {};PedestalPlan plan;if(!nav::scenePedestalPlan(observation,target,plan)) return L"The current pedestal state is unavailable.";
    if(step) {const auto text=pedestalStepText(plan,stepDone,step->done.low==0x0c0000d9 && step->done.value==1 && target.pushControl==3);if(!text.empty()) return text;}
    const bool single=!plan.moves.empty() && plan.moves.front().mode==PushMode::Single;
    const auto consent=controlName(observation,single?observation.confirmMask:0x40,single?L"Confirm":L"the configured push control");
    const auto side=describePedestalApproach(plan,target.position,observation.player);return (side.empty()?L"":side+L" ")+describePedestalPlan(plan,target.position,consent);
}
// A scene or an assisted push owns the player while the script is part-way
// through its writes (a door scene briefly empties a socket), so the guide
// holds its last exact step until the player has control again.
unsigned bevelleRailInput(const nav::Scene& observation,const State& state) {
    if(std::strcmp(state.route->script,"bvyt1100") || !observation.parts[state.part].guideReady || observation.window ||
        (observation.pedestal.armed && observation.pedestal.held) || value(observation,state,0x0c000008)==0) return 0;
    // W40:5 presses action 5 (1c743); W41..51:5 test held action 5.
    // Enabled regions can be distant or awaiting the accepted turn's completion.
    // W40's 0x14 and the junctions' 0x2c distinguish those accepted inputs.
    for(unsigned i=0;i<observation.count;++i) {const auto& worker=observation.workers[i];
        if(!worker.valid || worker.part!=state.part || worker.local<40 || worker.local>51 || worker.type!=3 ||
            !(worker.interactions.channels&8) || worker.interactions.entries[3]!=5) continue;
        if(observation.visibleMessage && observation.bevelleRailPromptWorker!=worker.local) continue;
        if(value(observation,state,worker.local==40?0x0c000014:0x0c00002c)!=0) continue;
        if(inInteractionZone(nav::sceneInteractionZone(worker,8,0),observation.player)) return worker.local;
    }
    return 0;
}
bool bevelleRailTurnReady(const nav::Scene& observation,const State& state,unsigned worker) {
    // W19/21/23/24/25 cycle these arrow phases automatically. No direction
    // key is read by the matching W41/44/48/49/50 input callback.
    switch(worker) {
    case 41:return value(observation,state,0x0c00000b)==1;
    case 44:return value(observation,state,0x0c00000d)==3;
    case 48:return value(observation,state,0x0c00000f)==1;
    case 49:return value(observation,state,0x0c000010)==1;
    case 50:return value(observation,state,0x0c000011)==1;
    default:return true;
    }
}
std::wstring railConfirmName(const nav::Scene& observation) {
    // Script action 5 is logical Confirm (0x20), not configuration row 5 /
    // push (0x40). The native snapshot resolves field remapping; by default
    // its 0x20 mask selects glyph 0x31, C on the keyboard.
    return controlName(observation,observation.confirmMask,L"Confirm");
}
std::wstring waitText(const nav::Scene& observation,bool message) {
    return puzzleGuideWaitText(message,observation.controls.controller?railConfirmName(observation):std::wstring(L"Enter"));
}
bool bevelleRailFresh(const nav::Scene& observation) {
    const auto now=GetTickCount64();
    // Arrow phases last 12 script frames.
    return observation.capturedAt && now>=observation.capturedAt && now-observation.capturedAt<=100;
}
int bevelleRailPhase(const nav::Scene& observation,const State& state,unsigned worker) {
    const unsigned binding=worker==41 || worker==42?0x0c00000b:worker==43?0x0c00000c:worker==44?0x0c00000d:
        worker>=45 && worker<=47?0x0c00000e:worker==48?0x0c00000f:worker==49?0x0c000010:worker==50?0x0c000011:0;
    return binding?value(observation,state,binding):0;
}
std::wstring bevelleRailAdvice(const nav::Scene& observation,const State& state,unsigned worker,int phase,unsigned objective) {
    if(worker==43) {
        // W43 accepts held Confirm in both phases. W15 reads W20's arrow
        // at 11529: phase 1 goes to W30, phase 2 goes to W31.
        const int wanted=objective==30?1:objective==31?2:0;
        if(!wanted) return L"Let this junction pass.";
        const std::wstring direction=wanted==1?L"left":L"right";
        if(phase==wanted) return L"Press "+railConfirmName(observation)+L" now to turn "+direction+L".";
        const auto wait=L"Wait for the "+direction+L" arrow.";
        return phase==1 || phase==2?L"Release "+railConfirmName(observation)+L". "+wait:wait;
    }
    if(!bevelleRailTurnReady(observation,state,worker)) return L"Wait for the turn arrow.";
    return L"Press "+railConfirmName(observation)+(worker==40?L" now to start.":L" now to turn.");
}
std::wstring bevelleRailCue(const nav::Scene& observation,State& state) {
    if(std::strcmp(state.route->script,"bvyt1100") || !bevelleRailFresh(observation)) return {};
    const auto worker=bevelleRailInput(observation,state);
    if(!worker) {state.railWindow=0;return {};}
    const auto phase=bevelleRailPhase(observation,state,worker);
    if(state.railWindow==worker && state.railPhase==phase) return {};
    state.railWindow=worker;state.railPhase=phase;
    return bevelleRailAdvice(observation,state,worker,phase,state.route->steps[state.expected].worker);
}
std::wstring manualPushCue(const nav::Scene& observation,State& state) {
    if(std::strcmp(state.route->script,"mcyt0500")) return {};
    auto& push=state.manualPush;
    // Movement keys cancel assisted pushing. Observe the original pedestal
    // on the guide worker, without arming assistance or touching game input.
    if(!observation.parts[state.part].guideReady || observation.pedestal.armed) {push={};return {};}
    for(unsigned i=0;i<observation.count;++i) {const auto& worker=observation.workers[i];
        if(!worker.valid || worker.part!=state.part || worker.local!=1 || worker.type!=1 || worker.actor<0) continue;
        const int phase=value(observation,state,0x0c0000a3);
        // W1:10 and W1:12 write these destination phases directly. Phase 99
        // is a failed branch, and W1:8/9 reset the pedestal to phases 0/5.
        const bool destination=(push.phase==0 && (phase==1 || phase==2)) || (push.phase==1 && phase==3) ||
            (push.phase==3 && phase==4) || (push.phase==5 && phase==6);
        const bool moved=std::hypot(worker.position.x-push.position.x,worker.position.y-push.position.y,
            worker.position.z-push.position.z)>.1;
        if(observation.control) {
            const bool completed=destination && (push.moving || moved);
            push={phase,worker.position,false};
            return completed?L"Pedestal push complete.":L"";
        }
        if(phase!=push.phase && !destination) {push={};return {};}
        const bool launch=push.phase==0 || push.phase==1 || push.phase==3 || push.phase==5;
        if(launch && moved && !push.moving) {
            push.moving=true;return L"Pushing pedestal.";
        }
        return {};
    }
    push={};return {};
}
void logBevelleRailQuery(const nav::Scene& observation,State& state,OwnedGuideAction action) {
    if(action==OwnedGuideAction::Completed || action==OwnedGuideAction::TileCompleted || std::strcmp(state.route->script,"bvyt1100") ||
        (observation.control && !observation.message && !observation.visibleMessage && !observation.window && !(observation.pedestal.armed && observation.pedestal.held))) return;
    const auto now=GetTickCount64();if(state.railLogAt && now-state.railLogAt<1000) return;state.railLogAt=now;
    // Preparation-owner only: format already-owned facts, one line per query,
    // at most once a second. Missing workers/bindings remain explicit.
    const auto binding=[&](unsigned low) {for(unsigned i=0;i<observation.parts[state.part].bindings.count;++i) {
        const auto& row=observation.parts[state.part].bindings.rows[i];if(row.binding==low && row.high==1 && !row.element) return row.value;}return -1;};
    char workers[2][192]{};
    for(unsigned slot=0;slot<2;++slot) {
        strcpy_s(workers[slot],"missing");
        for(unsigned i=0;i<observation.count;++i) {const auto& w=observation.workers[i];if(w.part!=state.part || w.local!=(slot?39u:40u)) continue;
            const auto& e=w.interactions.entries;
            _snprintf_s(workers[slot],_TRUNCATE,"valid=%u,type=%u,channels=0x%x,entries=[%u,%u,%u,%u,%u,%u,%u]",
                w.valid,w.type,w.interactions.channels,e[0],e[1],e[2],e[3],e[4],e[5],e[6]);break;}
    }
    const auto input=bevelleRailInput(observation,state);
    int w40Zone=-1;for(unsigned i=0;i<observation.count;++i) {const auto& w=observation.workers[i];if(w.valid && w.part==state.part && w.local==40) {w40Zone=inInteractionZone(nav::sceneInteractionZone(w,8,0),observation.player)?1:0;break;}}
    recordLog("Bevelle rail query visibleMessage=%u window=%d control=%u message=%u node=0x0c000008:%d ready=%u accepted=%d/%d input-worker=%u prompt=%u in-zone=%d turn-ready=%u age-ms=%llu W40={%s} W39={%s}",
        observation.visibleMessage,observation.window,observation.control,observation.message,binding(0x0c000008),observation.parts[state.part].guideReady,
        binding(0x0c000014),binding(0x0c00002c),input,observation.bevelleRailPromptWorker,w40Zone,input?bevelleRailTurnReady(observation,state,input):false,
        observation.capturedAt && now>=observation.capturedAt?now-observation.capturedAt:0,workers[0],workers[1]);
}
bool settling(const nav::Scene& observation,const State& state) {return (!observation.control || observation.message || observation.visibleMessage || observation.window || (observation.pedestal.armed && observation.pedestal.held)) && !bevelleRailInput(observation,state);}
// Manual F9 and F10 selection starts from the step the guide last found.
// A sphere put back where it came from. A pickup step whose `also` says its
// source recess is empty (Besaid w39) is undone when the hand is empty and the
// source holds a sphere again; the right next move leaves the source empty.
void putBack(const nav::Scene& observation,State& state) {
    if(state.expected<=state.begin || state.expected>=state.end) return;
    const auto& pickup=state.route->steps[state.expected-1];
    if(!pickup.check || pickup.checkpoint || !pickup.also.low || pickup.also.value!=0 || pickup.done.value==0) return;
    if(value(observation,state,pickup.done.low,pickup.done.high,pickup.done.element)==pickup.done.value) return;
    if(value(observation,state,pickup.also.low,pickup.also.high,pickup.also.element)==0) return;
    --state.expected;
}
void follow(const nav::Scene& observation,State& state) {
    if(settling(observation,state)) return;
    const auto before=state.expected;putBack(observation,state);advance(observation,state);
    if(state.expected!=before) {state.step=state.expected;state.railWindow=0;}
}
const BevelleStop* bevelleStop(const nav::Scene& observation,const State& state) {
    return ffx::bevelleStop(value(observation,state,0x0c000008),value(observation,state,0x0c000009));
}
// A different stop on the same upper rail can be across a gap. W15 parks
// at 17/1 on the source side and 22/3 on the opposite side (1173f,115f6).
const wchar_t* bevelleRide(const nav::Scene& observation,const State& state,const PuzzleStep& step) {
    if(std::strcmp(state.route->script,"bvyt1100") || !observation.parts[state.part].guideReady) return nullptr;
    const auto* stop=bevelleStop(observation,state);
    const int pedestal=stop?stop->pedestal:0;
    switch(step.worker) {
    case 30: return pedestal==1?nullptr:L"Ride the rail pedestal. At the upper rail second junction, confirm left for the Bevelle Sphere source.";
    case 31: return pedestal==3?nullptr:L"Ride the rail pedestal. At the upper rail second junction, confirm right for the power recess.";
    // W15 arrival pairs 35/4, 53/10 and 58/12. W38:16 places the
    // corresponding boarding regions at Z 4.7, 364.7 and 524.7.
    // W53 parks the empty pedestal at 64/15; W38 E15 boards it from there.
    case 13: return pedestal==15?L"Board the called pedestal at the upper platform stop. It rides to the optional chest.":nullptr;
    case 32: return pedestal==4?nullptr:L"Ride the pedestal to the lower dead-end platform. Use the upper rail first junction to descend.";
    case 33: return pedestal==10?nullptr:L"Ride the pedestal to the Glyph Sphere platform at the lower rail second junction.";
    case 34: case 35: case 36:
        return pedestal==12?nullptr:L"Ride the pedestal to the lower rail power recess and Glyph Sphere barrier platform.";
    case 16: case 37:
        return bevelleFinalLanding(observation,state)?nullptr:L"Ride the final ascending rail to the upper platform.";
    default: return nullptr;
    }
}
// With Auto walk on, the game thread confirms each junction itself
// (bevelle_rail.cpp). Publish every enabled rail region with the phase the
// current step needs; regions the step does not need are let pass.
void publishRailPlan(const nav::Scene& observation,const State& state) {
    const auto& step=state.route->steps[state.expected];
    if(std::strcmp(state.route->script,"bvyt1100") || !observation.parts[state.part].guideReady || observation.window ||
        (step.worker!=13 && !bevelleRide(observation,state,step))) {clearBevelleRailPlan();return;}
    BevelleRailPlan plan;plan.epoch=observation.epoch;plan.at=observation.capturedAt;plan.objective=step.worker;
    for(unsigned i=0;i<observation.count && plan.count<BevelleRailPlan::Rows;++i) {const auto& worker=observation.workers[i];
        if(!worker.valid || worker.part!=state.part || worker.local<40 || worker.local>51 || worker.type!=3 ||
            !(worker.interactions.channels&8) || worker.interactions.entries[3]!=5) continue;
        // A reward or sphere dialogue can share a prompt region; Confirm would
        // page it. Only a verified rail prompt may be confirmed.
        if(observation.visibleMessage && observation.bevelleRailPromptWorker!=worker.local) continue;
        const auto zone=nav::sceneInteractionZone(worker,8,0);if(!zone.valid) continue;
        plan.rows[plan.count++]={worker.local,bevelleRailWanted(worker.local,step.worker),zone};
    }
    if(plan.count) publishBevelleRailPlan(plan);else clearBevelleRailPlan();
}
std::wstring autoRailText(unsigned worker,unsigned objective) {
    const int wanted=bevelleRailWanted(worker,objective);
    if(worker==40) return wanted<0?L"This step does not need the rail pedestal.":L"Starting the rail pedestal.";
    if(wanted<0) return L"Passing this junction.";
    switch(worker) {
    case 41: case 42: return wanted==1?L"Turning toward the hub.":L"Turning back north on the upper rail.";
    case 43: return wanted==1?L"Turning toward the Bevelle Sphere source.":L"Turning toward the power recess.";
    case 45: case 46: case 47: return wanted==0?L"Choosing the lower dead-end platform.":wanted==2?L"Choosing the lower rail.":L"Choosing the upper rail.";
    case 48: return L"Turning toward the final ascending rail.";
    case 49: return L"Turning toward the Glyph Sphere platform.";
    case 50: return L"Turning toward the lower rail power recess platform.";
    default: return L"Passing this junction.";
    }
}
// One cue per region entry: the mod turns, so arrow phases are not spoken.
std::wstring autoRailCue(const nav::Scene& observation,State& state) {
    if(std::strcmp(state.route->script,"bvyt1100")) return {};
    const auto worker=bevelleRailInput(observation,state);
    if(!worker) {state.railWindow=0;return {};}
    if(state.railWindow==worker) return {};
    state.railWindow=worker;state.railPhase=-1;
    recordLog("bevelle rail auto cue W%u objective W%u step %u",worker,state.route->steps[state.expected].worker,state.expected);
    return autoRailText(worker,state.route->steps[state.expected].worker);
}
bool pushStep(const State& state,const PuzzleStep& step) {return step.platform || (!std::strcmp(state.route->script,"dome0700") && step.worker>=44 && step.worker<=47);}
std::wstring path(const nav::Scene& observation,const Mesh& mesh,const std::vector<Entity>& entities,const Entity& target,bool platform,unsigned manual,unsigned intentRevision,const std::wstring& finish={},bool describeOnly=false) {
    constexpr double UnitsPerStep=11.13;resetNavAudio();
    if(mesh.panels.required) {
        const auto gap=mesh.triangles.empty() && !mesh.openPlane?"collision map unavailable":panelFactsGap(mesh.panels,GetTickCount64());
        if(gap) {recordLog("floor panel guide route refused: %s",gap);return noSafePanelRoute;}
    }
    if(target.pushControl && !target.zone.valid) return L"The pedestal collision data is unavailable.";
    if(mesh.triangles.empty() && !mesh.openPlane) return L"The current collision map is unavailable. Read the puzzle step or use the pathfinder.";
    const auto points=navigationRouteOwned(mesh,observation.player,target,entities);
    if(mesh.panels.required && (points.empty() || !panelFactsFresh(mesh.panels.revision,mesh.panels.capturedAt,GetTickCount64()))) {
        recordLog("floor panel guide route refused: %s (target W%d panel %u)",points.empty()?(panelRouteGap?panelRouteGap:"no route"):"stale panel facts",target.worker,target.panelLocal);
        return noSafePanelRoute;
    }
    if(points.empty()) return platform?L"No walking path to that platform in the current map state. Use its lift, pedestal, or rail control first.":L"No walking path in the current map state. Complete the preceding sphere or platform action first.";
    const bool walk=!describeOnly && !bevelleEntrance(target.pushControl,target.position) && setting(Setting::AutoWalk);
    if(!describeOnly) routeNavAudio(points,target,walk);
    if(walk) {
        const bool reached=target.zone.valid && inInteractionZone(target.zone,observation.player);
        const auto kind=target.pushControl?reached?nav::Intent::ArmAndFace:nav::Intent::ArmAndStart:nav::Intent::Start;
        if(!nav::submitNavigationIntent(observation,target,kind,manual,intentRevision)) return L"Navigation action is unavailable or busy. Request the route again after it recovers.";
    }
    double total=0;std::vector<std::wstring> segments;
    for(unsigned i=0;i+1<points.size();++i) {const double length=distanceXZ(points[i],points[i+1])/UnitsPerStep;total+=length;if(length<=.005) continue;segments.push_back(routeSegmentText(length,direction({points[i+1].x-points[i].x,points[i+1].y-points[i].y,points[i+1].z-points[i].z}),setting(Setting::PartialSteps)!=0));}
    if((target.zone.valid && inInteractionZone(target.zone,observation.player)) || total<.05) {
        if(describeOnly) return target.name+L": within interaction range.";
        if(!target.pushControl && !finish.empty() && (target.name==djoseGapJumpLabel || target.name==L"Central lift")) return target.name+L": reached. "+finish;
        return target.name+(target.pushControl?L": at the push position. "+instruction(observation,target):L": within interaction range. Follow the current step.");
    }
    if(describeOnly) return join(segments,L", then ")+L".";
    if(walk && !setting(Setting::AutoWalkPath)) return {};
    if(target.pushControl) return join(segments,L", then ")+L". "+instruction(observation,target);
    return join(segments,L", then ")+L". "+(!finish.empty()?finish:platform?L"Interact with the platform control; riding is a separate step.":L"Interact with the puzzle control.");
}
}
std::wstring puzzleTargetPathOwned(const nav::Scene& observation,const Mesh& mesh,const std::vector<Entity>& entities,Entity target,const std::wstring& finish,unsigned manual,unsigned intentRevision) {
    if(nativeGameThread()) return {};nav::CameraScope camera(observation.forward,observation.right,observation.cameraValid);
    if(target.worker>=0) {const unsigned local=static_cast<unsigned>(target.worker);bool mapped=false,positioned=false;
        const auto name=target.name;const auto fallback=target.position;const bool talk=target.zone.talk;
        for(unsigned i=0;i<observation.count;++i) {const auto& row=observation.workers[i];if(!row.valid || row.part>=observation.partCount || row.local!=local || observation.parts[row.part].span.chunk!=observation.primary) continue;
            mapped=true;
            for(const auto& current:entities) if(current.worker==static_cast<int>(i) && current.zone.valid && distanceXZ(current.zone.center,fallback)<75 && current.zone.talk==talk) {target=current;target.name=name;positioned=true;break;}
            if(!positioned) {
                const auto zone=nav::sceneInteractionZone(row,talk?1u:16u,observation.extraTalkRange);
                if(zone.valid && (!talk || distanceXZ(zone.center,fallback)<75)) {
                    target.position=talk?zone.center:fallback;target.zone=zone;target.worker=i;target.actor=row.actor;target.id=0xfffe;
                    if(row.actor>=0 && static_cast<unsigned>(row.actor)<observation.actorCount) {uint16_t model=0;std::memcpy(&model,observation.actors[row.actor].bytes.data(),2);target.id=model;}
                    positioned=true;
                }
            }
            break;}
        if(!mapped) return L"This control is not positioned yet; perform the preceding step first";
        // Preserve the original authored-coordinate fallback without assigning
        // it a false actor identity or stale collision shape.
        if(!positioned) {target.worker=target.actor=-1;target.id=0;target.position=fallback;target.zone={};}
    }
    return path(observation,mesh,entities,target,false,manual,intentRevision,finish);
}
std::wstring ownedGuidePedestalStep(const nav::Scene& observation,const Entity& pedestal) {
    if(nativeGameThread() || !pedestal.pushControl || !enabled.load(std::memory_order_acquire) || !storage.ready()) return {};
    const auto& state=storage.get();
    if(!state.route || state.epoch!=observation.epoch || state.revision!=revision.load(std::memory_order_acquire) || state.step>=state.end) return {};
    const auto& step=state.route->steps[state.step];if(step.worker!=pedestal.pushLocal || pushStep(state,step)) return {};
    Entity probe;configurePedestalTarget(probe,state.route->script,step.worker);if(probe.pushControl!=pedestal.pushControl) return {};
    std::wstring text=deviceKeyText(step.instruction);const auto tag=L"Target: "+std::wstring(step.label)+L". ";
    if(!text.compare(0,tag.size(),tag)) text.erase(0,tag.size());
    return L"Guide step "+std::to_wstring(state.step-state.begin+1)+L": "+text;
}
bool ownedGuideTargetsTile(const nav::Scene& observation,const Entity& tile) {
    if(nativeGameThread() || tile.pushControl || tile.worker<0 || static_cast<unsigned>(tile.worker)>=observation.count) return false;
    if(!enabled.load(std::memory_order_acquire) || !storage.ready()) return false;
    const auto& state=storage.get();
    if(!state.route) return false;
    if(state.epoch!=observation.epoch) return false;
    if(state.revision!=revision.load(std::memory_order_acquire)) return false;
    if(state.step>=state.end) return false;
    const auto& step=state.route->steps[state.step];const auto& worker=observation.workers[tile.worker];
    if(!worker.valid || worker.part>=observation.partCount) return false;
    if(observation.parts[worker.part].span.chunk!=observation.primary) return false;
    if(worker.local!=step.worker) return false;
    // Only reset tiles: their step-on handler has no gate, so the row is usable.
    return std::wstring_view(step.label).find(L"reset tile")!=std::wstring_view::npos;
}
bool ownedGuidePedestalFace(const nav::Scene& observation,const Entity& pedestal,Point& face) {
    // Kilika (2), Djose (3) and Macalania (4) pedestals hold their sphere
    // recess on the pedestal worker itself. A sphere step routes to that
    // recess, at interaction range, not to the push position: touching the
    // pedestal starts the original push script, which takes control.
    if(pedestal.pushControl<2 || pedestal.pushControl>4 || ownedGuidePedestalStep(observation,pedestal).empty()) return false;
    face={};const auto& state=storage.get();
    if(std::string_view(state.route->script)=="mcyt0500") try {
        // W1 entry 2 (pc 1666) runs the sphere action only from W29 side
        // 0xc5=2, south of the pedestal, or side 8, north of it, while the
        // pedestal is on the walkway (0xa3=5).
        face={0,0,value(observation,state,0x0c0000a3)==5?1.0:-1.0};return true;
    } catch(...) {return false;}
    if(std::string_view(state.route->script)!="djyt0600") return true;
    try {
        // Djose W15 owns the east socket (0x9d) and W16 the west socket (0x9e).
        // An insert goes to an empty socket, a take to a loaded one; east first.
        const auto& step=state.route->steps[state.step];
        const bool insert=step.done.low==0x0c000094 && step.done.value==0;
        const int east=value(observation,state,0x0c00009d),west=value(observation,state,0x0c00009e);
        const bool useEast=insert?east==0 || west!=0:east!=0 || west==0;
        face={useEast?1.0:-1.0,0,0};return true;
    } catch(...) {return false;}
}
void resetOwnedPuzzleGuide() {enabled=false;refreshPending=false;revision.fetch_add(1,std::memory_order_acq_rel);}
bool ownedPuzzleGuideActive() {return enabled.load(std::memory_order_acquire);}
unsigned ownedPuzzleGuideRevision() {return revision.load(std::memory_order_acquire);}
bool ownedGuideStepTarget(const nav::Scene& observation,const Entity& e) {
    if(!enabled.load(std::memory_order_acquire) || !storage.ready() || e.worker<0 || static_cast<unsigned>(e.worker)>=observation.count) return false;
    const auto& state=storage.get();if(!state.route || state.epoch!=observation.epoch) return false;
    const auto& fact=observation.workers[e.worker];if(!fact.valid) return false;
    for(const unsigned index:{state.step,state.expected}) {
        if(index<state.begin || index>=state.end || index>=state.route->steps.size()) continue;
        const auto& step=state.route->steps[index];
        const bool push=step.platform || (!std::strcmp(state.route->script,"dome0700") && step.worker>=44 && step.worker<=47);
        if(nav::guideStepRow(fact.part,fact.local,e.pushControl!=0,state.part,step.worker,push)) return true;
    }
    return false;
}
bool claimOwnedGuideRefresh() {bool empty=false;return refreshPending.compare_exchange_strong(empty,true,std::memory_order_acq_rel);}
void releaseOwnedGuideRefresh() {refreshPending=false;}
std::wstring puzzleGuideOwned(OwnedGuideAction action,const nav::Scene& observation,const Mesh& mesh,const std::vector<Entity>& entities,const speechq::Header&,unsigned manual,unsigned intentRevision,unsigned local,uintptr_t chunk,unsigned serial) {
    if(nativeGameThread()) return {};nav::CameraScope camera(observation.forward,observation.right,observation.cameraValid);
    if(!storage.ready()) storage.prepare();auto& state=storage.get();if(!context(observation,state)) {enabled=false;if(action==OwnedGuideAction::Completed) refreshPending=false;return action==OwnedGuideAction::Completed || action==OwnedGuideAction::TileCompleted?std::wstring{}:L"No puzzle guide for this area.";}
    if(state.begin==state.end) {enabled=false;refreshPending=false;return action==OwnedGuideAction::Completed || action==OwnedGuideAction::TileCompleted?std::wstring{}:L"Finish Zanarkand's native story and encounter sequence before the return-visit puzzle guide becomes available.";}
    logBevelleRailQuery(observation,state,action);
#ifdef FFX_DEV_UTILITIES
    // Development puzzle jump: the jump already wrote this step's variables.
    // Placements before it that read done stay passed; the others were taken
    // back out by the route itself. No frame replay is needed.
    if(action==OwnedGuideAction::Seek) {
        if(!local || local>state.end-state.begin) return L"That step is not part of this puzzle guide.";
        const unsigned index=state.begin+local-1;state.step=state.expected=index;state.placed=state.removed=0;state.railWindow=0;state.manualPush={};
        for(unsigned i=state.begin;i<index && i<64;++i) if(puzzleStepPlaced(*state.route,i)) {
            if(done(observation,state,state.route->steps[i])) state.placed|=std::uint64_t{1}<<i;else state.removed|=std::uint64_t{1}<<i;}
        action=OwnedGuideAction::Step;
    }
#endif
    // Auto walk turns the Bevelle rail without the guide being asked first.
    const bool railAuto=bevelleRailAutoActive() && !std::strcmp(state.route->script,"bvyt1100");
    if(action==OwnedGuideAction::Completed) {refreshPending=false;try {if(enabled.load() || railAuto) {follow(observation,state);publishRailPlan(observation,state);
        auto cue=bevelleRailAutoActive()?autoRailCue(observation,state):bevelleRailCue(observation,state);return cue.empty()?manualPushCue(observation,state):cue;}}catch(...) {enabled=false;}return {};}
    if(action==OwnedGuideAction::TileCompleted) {if(!std::strcmp(state.route->script,"dome0700") && chunk==observation.parts[state.part].span.chunk && serial==observation.parts[state.part].span.serial && local>=57 && local<=83) {synchronizeTiles(observation,state,local);}return {};}
    // The first push can finish before the next native completion observation.
    if(state.manualPush.phase<0 && observation.control) manualPushCue(observation,state);
    enabled=true;if(action==OwnedGuideAction::Next || action==OwnedGuideAction::Previous ||
        (action==OwnedGuideAction::Path && state.step==state.expected &&
            (!std::strcmp(state.route->script,"bvyt1100") || !std::strcmp(state.route->script,"bsyt0100")))) {
        // F9/F10 first catch up with play. When that already moved the
        // selection to the step now expected, the press lands there: moving
        // on again skipped it (dome0700 F10 from step 2 read step 4).
        const auto before=state.step;follow(observation,state);
        if(state.step!=before && (action==OwnedGuideAction::Next || action==OwnedGuideAction::Previous)) action=OwnedGuideAction::Step;
    }
    const bool domeStart=action==OwnedGuideAction::Expected && !std::strcmp(state.route->script,"dome0700");
    if(action==OwnedGuideAction::Next) {if(state.step+1<state.end) ++state.step;}else if(action==OwnedGuideAction::Previous) {if(state.step>state.begin) --state.step;}else if(action==OwnedGuideAction::Expected) {follow(observation,state);
        // An unidentified trial state keeps the F9/F10 selection. Live Djose:
        // Shift+F11 after F10 chose step 11 dropped it, so the pedestal row
        // went inactive and auto walk refused it.
        if(state.route->steps[state.expected].check || state.expected+1>=state.end) state.step=state.expected;}
    unsigned selected=action==OwnedGuideAction::Expected?state.expected:state.step;
    // dome0700 first visit: the activation tile (step 1) has no done flag. A
    // first-pattern panel count above 0 shows the player is past it, so step
    // 2 is expected; before that, step 1 is.
    const bool domeFirst=domeStart && state.begin==0 && state.expected==0 && state.end>1;
    if(domeFirst) selected=state.step=value(observation,state,0x0c000022)>0?1u:0u;
    if(!std::strcmp(state.route->script,"bvyt1100")) {
        if(!observation.parts[state.part].guideReady) return L"The puzzle observations are unavailable.";
        if((action==OwnedGuideAction::Expected || action==OwnedGuideAction::Path) && state.expected<=state.begin+1 &&
            value(observation,state,0x0c000008)==0 && value(observation,state,0x0c000009)==0) selected=state.step=state.begin;
    }
    // dome0700: pedestal steps wait for the floor pattern. Step 2 is read and
    // stays selected, so the next F9/F10 press starts from what was spoken.
    if(!std::strcmp(state.route->script,"dome0700") && selected>=2 && selected<=5 && value(observation,state,0x0c000022)!=6) selected=state.step=1;
    if(action==OwnedGuideAction::Expected && !domeFirst && !state.route->steps[selected].check && selected+1<state.end) {
        std::wstring text=L"The current trial state does not identify an exact puzzle step. Use "+modKeyName(ModKey::PuzzleStep)+L" and "+modKeyName(ModKey::PuzzlePath)+L" to select the step you have reached.";
        if(state.step!=selected) text+=L" Step "+std::to_wstring(state.step-state.begin+1)+L" stays selected.";
        return text;
    }
    const auto& current=state.route->steps[selected];const auto prefix=(state.route->returnStep?state.begin>=state.route->returnStep?std::wstring(L"Return-visit treasure. "):std::wstring(L"First-visit trial. "):std::wstring{})+L"Step "+std::to_wstring(selected-state.begin+1)+L" of "+std::to_wstring(state.end-state.begin)+L". ";
#ifdef FFX_DEV_UTILITIES
    {char event[160];std::snprintf(event,sizeof(event),"script=%s step=%u total=%u expected=%u action=%u",state.route->script,selected-state.begin+1,state.end-state.begin,state.expected-state.begin+1,static_cast<unsigned>(action));devPipeEvent("step",event);}
#endif
    if(!std::strcmp(state.route->script,"bsyt0100") && observation.visibleMessage)
        return prefix+waitText(observation,true);
    if(!std::strcmp(state.route->script,"dome0700") && ((selected>0 && selected<=5) || selected==11)) {
        if(settling(observation,state)) return prefix+waitText(observation,observation.visibleMessage);
        if(done(observation,state,current)) return prefix+L"This floor pattern is complete. Read the next guide step.";
        const int stage=value(observation,state,0x5c000004);
        if(selected==11 || (selected==1 && stage==0) || (selected>=2 && stage==static_cast<int>(selected-1))) {
            Point point;std::wstring label;unsigned localPanel=0;const bool available=tile(observation,state,point,label,localPanel);
            if(!available) return action==OwnedGuideAction::Path?std::wstring(noSafePanelRoute)+L" "+label:prefix+label;
            if(action==OwnedGuideAction::Path) {
                Entity panel;panel.name=L"Floor puzzle panel";panel.position=point;
                if(mesh.panels.required) {
                    if(!mesh.panels.ready) {recordLog("floor panel guide route refused: %s",mesh.panels.gap?mesh.panels.gap:"panel facts not ready");resetNavAudio();return std::wstring(noSafePanelRoute)+L" "+label;}
                    panel.panelLocal=localPanel;panel.worker=mesh.panels.rows[localPanel-57].worker;
                    panel.position=mesh.panels.rows[localPanel-57].center;panel.name=zanarkandGlyphLabel(localPanel);
                    const auto directions=path(observation,mesh,entities,panel,false,manual,intentRevision,L"Step onto the named panel.");
                    if(directions==noSafePanelRoute) return directions+L" "+label;
                    return directions;
                }
                return std::wstring(floorWalkWarning)+L" "+prefix+label+L" "+path(observation,mesh,entities,panel,false,manual,intentRevision,{},true)+L" Step onto the named panel.";
            }
            return prefix+label+L" "+(mesh.panels.ready && panelFactsFresh(mesh.panels.revision,mesh.panels.capturedAt,GetTickCount64())?L"":floorWalkWarning)+L" Step onto it. For its path, choose Read path to current step in the Cloister of Trials menu.";
        }
    }
    if(!std::strcmp(state.route->script,"bsyt0100") && current.worker==7 && value(observation,state,0x5c0000cc)!=1) return prefix+L"The Besaid Sphere must be in the pedestal before the final push. Insert it into the pedestal sphere recess first."+(action==OwnedGuideAction::Path?L" "+path(observation,mesh,entities,target(observation,state,state.route->steps[8],entities),false,manual,intentRevision):L"");
    const auto ride=bevelleRide(observation,state,current);
    if(const auto rail=bevelleRailInput(observation,state)) {
        if(bevelleRailAutoActive()) return std::wstring(autoRailText(rail,current.worker))+L" "+prefix+(ride?ride:current.instruction)+L" The mod confirms the rail turns while Auto walk is on.";
        const auto input=bevelleRailFresh(observation)?
            bevelleRailAdvice(observation,state,rail,bevelleRailPhase(observation,state,rail),current.worker):L"Wait for a fresh rail cue.";
        return input+L" "+prefix+(ride?ride:current.instruction)+L" Direction keys do not choose a branch on the rail pedestal.";
    }
    if(!std::strcmp(state.route->script,"bvyt1100") && observation.visibleMessage) return prefix+waitText(observation,true);
    const bool held=settling(observation,state);
    if(!std::strcmp(state.route->script,"bvyt1100") && current.worker==13 && !ride && value(observation,state,0x0c000009)==14) {
        if(held) return prefix+waitText(observation,false);
        const std::wstring call=std::wstring(current.instruction)+L" The call point is where the Destruction Sphere treasure was. Walk into it.";
        if(action!=OwnedGuideAction::Path) return prefix+call;
        for(const auto& point:entities) {
            if(point.id!=BevelleCallPointId || point.inactive || !point.zone.valid) continue;
            if(inInteractionZone(point.zone,observation.player)) return prefix+L"Wait here for the pedestal to arrive.";
            return prefix+call+L" "+path(observation,mesh,entities,point,false,manual,intentRevision,L"Stop at the call point.");
        }
        return prefix+call+L" The call point is not active yet.";
    }
    if(ride) {
        if(held || observation.visibleMessage) return prefix+waitText(observation,observation.visibleMessage);
        std::wstring text=prefix+ride;
        if(action!=OwnedGuideAction::Path) {
            if(const auto* stop=bevelleStop(observation,state))
                text+=L" Board at the Rail pedestal boarding position by holding "+std::wstring(stop->input)+L" until the pedestal moves.";
            return text;
        }
        if(const auto* stop=bevelleStop(observation,state)) {
            for(const auto& boarding:entities) {
                if(boarding.id!=stop->targetId() || boarding.worker<0 || static_cast<unsigned>(boarding.worker)>=observation.count) continue;
                const auto& w=observation.workers[boarding.worker];
                if(!w.valid || w.local!=38 || w.part!=state.part || boarding.inactive || !boarding.zone.valid ||
                    std::abs(boarding.position.y-observation.player.y)>=boarding.zone.height) continue;
                const auto input=L"Hold "+std::wstring(stop->input)+L" at the boarding position until the pedestal moves.";
                if(inInteractionZone(boarding.zone,observation.player)) return input;
                const auto approach=path(observation,mesh,entities,boarding,true,manual,intentRevision,L"Stop at the boarding position.");
                return approach.empty()?input:input+L" "+approach;
            }
        }
        return text+L" Follow the rail direction prompts. No walking route to a boarding position is available here.";
    }
    const bool zanarkand=!std::strcmp(state.route->script,"dome0700");
    const bool floorTraversal=zanarkand && selected>=1 && selected<=5;
    std::wstring text=prefix+deviceKeyText(current.instruction);
    if(action==OwnedGuideAction::Path) {
        const bool describeOnly=zanarkand && !mesh.panels.required;
        if(describeOnly) text=std::wstring(floorWalkWarning)+L" "+text;
        const auto goal=target(observation,state,current,entities);
        const bool jump=goal.name==djoseGapJumpLabel,lift=!std::strcmp(state.route->script,"djyt0600") && current.worker==63;
        // The jump direction is a world axis (across z 117); speak it in the same
        // camera-relative words as the route, or name only the pedestal without a camera.
        const std::wstring jumpWay=jump?cameraDirectionLabel({0,0,goal.position.z<117?1.0:-1.0},observation.forward,observation.cameraValid):L"";
        const auto directions=path(observation,mesh,entities,goal,!jump && current.platform,manual,intentRevision,
            jump?(jumpWay==L"unknown"?std::wstring(L"Hold toward the pedestal in the gap."):L"Hold "+jumpWay+L", toward the pedestal in the gap.")+L" The game jumps you across. Then request the route again.":
            lift?std::wstring(L"Stand still on the pedestal. It rises to the upper floor."):std::wstring{},describeOnly);
        if(mesh.panels.required && directions==noSafePanelRoute) return directions+L" "+text;
        return text+L" "+directions;
    }
    if(floorTraversal && !mesh.panels.ready) text+=L" "+std::wstring(floorWalkWarning);
    std::wstring push;
    if(held || observation.visibleMessage) push=waitText(observation,observation.visibleMessage);
    else if(pushStep(state,current)) {try {push=instruction(observation,target(observation,state,current,entities),&current,done(observation,state,current));}catch(...) {}}
    return push.empty()?text:text+L" "+push;
}
}
