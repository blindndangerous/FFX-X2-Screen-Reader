#pragma once
#include "interaction_region.hpp"
#include "navigation_scene.hpp"
#include <atomic>

namespace ffx {
inline constexpr const wchar_t* noSafePanelRoute=L"No safe auto walk route. Walk manually around other active panels.";
inline std::atomic<unsigned> floorPanelRevision{1};
inline std::atomic<uintptr_t> floorPanelChunk{0};
inline std::atomic<unsigned> floorPanelSample{0};
inline std::atomic<bool> floorPanelScriptActive{false};
inline constexpr unsigned floorPatternBindings[]={0x20000a00,0x5c000004,0x0c000020,0x0c000022,0x0c000024,
    0x0c000025,0x0c000026,0x0c000027,0x0c000028,0x0c000029};

inline bool panelFactsFresh(unsigned revision,ULONGLONG capturedAt,ULONGLONG now) {
    return revision==floorPanelRevision.load(std::memory_order_acquire) &&
        capturedAt && now>=capturedAt && now-capturedAt<=250; // Same bound as navigation intent dispatch.
}
inline bool panelMovementFresh(unsigned revision,ULONGLONG capturedAt,ULONGLONG now) {
    return !floorPanelScriptActive.load(std::memory_order_acquire) && panelFactsFresh(revision,capturedAt,now);
}

// Why panel facts cannot route now, for the log only; nullptr when usable.
inline const char* panelFactsGap(const FloorPanels& panels,ULONGLONG now) {
    if(!panels.ready) return panels.gap?panels.gap:"panel facts not ready";
    return panelFactsFresh(panels.revision,panels.capturedAt,now)?nullptr:"stale panel facts";
}

inline double collisionBodyRadius(const nav::ActorBody& actor) {
    const auto field=[&](unsigned offset) {float v;std::memcpy(&v,actor.bytes.data()+offset,sizeof(v));return v;};
    return (field(0x5c)+field(0x64))*.5*field(0x4e4);
}

inline bool readFloorPattern(const nav::PartFact& script,std::array<int,10>& pattern) {
    for(unsigned i=0;i<pattern.size();++i) {
        unsigned j=0;
        for(;j<script.bindings.count;++j) {
            const auto& row=script.bindings.rows[j];
            if(row.binding==floorPatternBindings[i] && row.high==1 && !row.element) {pattern[i]=row.value;break;}
        }
        if(j==script.bindings.count) return false;
    }
    return true;
}
inline FloorPanels readFloorPanels(const nav::Scene& observation) {
    FloorPanels result;
    result.required=observation.room==320; // TrialUtility::room for dome0700, including an unreadable primary script.
    for(unsigned part=0;part<observation.partCount;++part) {
        const auto& script=observation.parts[part];
        if(script.span.chunk!=observation.primary || !script.panelMap) continue;
        result.required=true;
        result.chunk=script.span.chunk;result.serial=script.span.serial;result.epoch=observation.epoch;
        result.sample=observation.panelSample;result.revision=observation.panelRevision;result.capturedAt=observation.capturedAt;
        if(!result.revision) {result.gap="panel state unverified";return result;}
        if(!observation.geometryValid || !script.guideReady || !script.span.serial || observation.epoch!=script.span.epoch ||
            observation.playerIndex<0 || unsigned(observation.playerIndex)>=observation.actorCount) {result.gap="scene or guide not ready";return result;}
        result.radius=collisionBodyRadius(observation.actors[observation.playerIndex]);
        if(!std::isfinite(result.radius) || result.radius<=0) {result.gap="player radius";return result;}
        // A quota can change the guide exemption even before the visible target moves.
        if(!readFloorPattern(script,result.pattern)) {result.gap="pattern binding missing";return result;}
        unsigned seen=0;
        for(unsigned i=0;i<observation.count;++i) {
            const auto& w=observation.workers[i];
            if(!w.valid || w.part!=part || w.local<57 || w.local>83) continue;
            const auto index=w.local-57,bit=1u<<index;
            if(w.type!=6 || w.global!=i || !w.interactions.entryReady || (seen&bit)) {result.gap="panel worker not ready";return result;}
            seen|=bit;
            if(!(w.interactions.channels&0x20)) continue;
            if(w.interactions.entries[5]!=2) {result.gap="panel entry state";return result;}
            const auto shape=playerEntryRegion(w.geometry,0x20);
            if(!shape.valid) {result.gap="panel shape";return result;}
            result.active|=bit;
            result.rows[index]={shape.center,std::cos(shape.angle),std::sin(shape.angle),
                shape.width,shape.depth,static_cast<int>(i)};
        }
        result.ready=seen==((1u<<27)-1);result.gap=result.ready?nullptr:"panel workers missing";
        return result;
    }
    return result;
}
inline bool sameFloorPanels(const FloorPanels& a,const FloorPanels& b) {
    if(!a.ready || !b.ready || a.chunk!=b.chunk || a.serial!=b.serial || a.epoch!=b.epoch ||
        a.pattern!=b.pattern || a.active!=b.active || a.radius!=b.radius) return false;
    for(unsigned i=0;i<a.rows.size();++i) if(a.active&(1u<<i)) {
        const auto& x=a.rows[i];const auto& y=b.rows[i];
        if(x.worker!=y.worker || x.center.x!=y.center.x || x.center.y!=y.center.y || x.center.z!=y.center.z ||
            x.cosine!=y.cosine || x.sine!=y.sine || x.width!=y.width || x.depth!=y.depth) return false;
    }
    return true;
}
}
