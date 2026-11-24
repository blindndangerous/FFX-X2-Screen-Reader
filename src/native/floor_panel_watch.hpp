#pragma once
#include "floor_panels.hpp"
#include "utility_memory.hpp"

namespace ffx::nav {
// Native owner only. Keep just the inputs to the captured panel route. A W38
// reset can affect a different worker, so checking only the caller is unsafe.
struct FloorPanelWatch {
    AtelSpan span{};
    uintptr_t controller=0,player=0;
    ULONGLONG capturedAt=0;
    unsigned revision=0,count=0,zeroGlobal=0,zeroHeader=0;
    double radius=0;
    std::array<WorkerFact,27> panels{};
    std::array<int,_countof(floorPatternBindings)> pattern{};

    bool capture(const Scene& observation,unsigned part,uintptr_t currentPlayer) {
        const auto& script=observation.parts[part];
        revision=0;
        if(!observation.geometryValid || !script.guideReady || observation.playerIndex<0) return false;
        span=script.span;controller=observation.controller;player=currentPlayer;count=observation.count;
        radius=collisionBodyRadius(observation.actors[observation.playerIndex]);
        if(!std::isfinite(radius) || radius<=0) return false;
        if(!readFloorPattern(script,pattern)) return false;
        unsigned seen=0;zeroGlobal=count;
        for(unsigned i=0;i<count;++i) {
            const auto& w=observation.workers[i];
            if(w.part!=part) continue;
            if(!w.local) {zeroGlobal=i;zeroHeader=w.header;continue;}
            if(w.local<57 || w.local>83) continue;
            const unsigned slot=w.local-57,bit=1u<<slot;
            if(!w.valid || w.type!=6 || !w.interactions.entryReady || (seen&bit)) return false;
            panels[slot]=w;seen|=bit;
        }
        if(zeroGlobal==count || seen!=((1u<<27)-1)) return false;
        capturedAt=observation.capturedAt;revision=observation.panelRevision;return true;
    }

    bool matches(uintptr_t chunk) const {
        AtelSpan current;
        if(!revision || chunk!=span.chunk ||
            !nativeAtelSpan(chunk,current) || current.serial!=span.serial || current.epoch!=span.epoch ||
            read<uintptr_t>(address(0xf26ae8))!=controller || read<uintptr_t>(controller+0x48)!=chunk ||
            read<uint16_t>(controller+0xc)!=count || read<uint16_t>(controller+0x1a,0xffff)!=0 || playerActor()!=player) return false;
        float left,right,scale;
        if(!copy(player+0x5c,&left,4) || !copy(player+0x64,&right,4) || !copy(player+0x4e4,&scale,4) ||
            (left+right)*.5*scale!=radius) return false;
        using Getter=uintptr_t(__cdecl*)(int);
        const auto get=reinterpret_cast<Getter>(address(0x46a7c0));
        const auto zero=get(zeroGlobal);
        if(!zero || read<uintptr_t>(zero)!=chunk+zeroHeader || read<uintptr_t>(zero+4)!=chunk ||
            read<uint16_t>(zero+0x2e,0xffff)!=0) return false;
        UtilityScript script;script.controller=controller;script.chunk=chunk;script.worker=zero;
        script.header=chunk+zeroHeader;script.allocationSerial=span.serial;
        for(unsigned i=0;i<pattern.size();++i) {
            int value=0;
            if(!captureUtilityValue(script,floorPatternBindings[i],1,0,value) || value!=pattern[i]) return false;
        }
        for(const auto& w:panels) {
            const auto worker=get(w.global);
            if(!worker || read<uintptr_t>(worker)!=chunk+w.header || read<uintptr_t>(worker+4)!=chunk ||
                read<uint16_t>(worker+0x2e,0xffff)!=w.local || read<uint8_t>(chunk+w.header)!=6) return false;
            const auto interaction=captureNativeInteractionFacts(worker,EntryMask{},span);
            if(!interaction.entryReady || interaction.channels!=w.interactions.channels) return false;
            if(!(interaction.channels&0x20)) continue;
            if(interaction.entries[5]!=2 || w.interactions.entries[5]!=2) return false;
            decltype(w.geometry) geometry;
            if(!copy(worker+0x284,geometry.data(),geometry.size())) return false;
            // Exactly the type-6 shape fields consumed by playerEntryRegion.
            const auto same=[&](unsigned offset,unsigned size) {
                return !std::memcmp(geometry.data()+offset,w.geometry.data()+offset,size);
            };
            if(!same(0,12) || !same(0x1c,4) || !same(0x28,2) || !same(0x30,12)) return false;
        }
        return true;
    }
};
}
