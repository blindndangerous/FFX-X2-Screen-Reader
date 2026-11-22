#pragma once
#include "navigation.hpp"

namespace ffx {
// W15 parks at these node/pedestal pairs. W38:16 places the rectangle and
// installs the stop-specific handler in callback slot 3 (1c47b..1c59b).
struct BevelleStop {
    int node,pedestal;
    Point center;
    unsigned entry;
    double standX; // interior of the handler's X band
    const wchar_t* input;
    double standZ=0; // 0: rectangle center; else the handler's Z band
    unsigned targetId() const {return 0xb100u+pedestal;}
};
inline const BevelleStop* bevelleStop(int node,int pedestal) {
    static constexpr BevelleStop stops[]={
        {17,1,{-60.2,264.8,84.6},8,-65,L"Down"},
        {22,3,{19.8,264.8,84.6},9,25.75,L"Left"}, // W38 E9 tests c04c(15) Left at 1bd15; live Left boarded, Right did not
        {35,4,{195.7,745.7,4.7},10,190,L"Down"},
        {43,6,{357.7,462.8,204.7},11,352,L"Down"},
        {43,9,{357.7,462.8,204.7},11,363,L"Up"},
        {53,10,{399.7,462.8,364.7},12,404.75,L"Down"},
        {58,12,{359.8,462.8,524.7},13,364.75,L"Down"},
        // W53 calls the empty pedestal here. W38 E15 (1c3d0) needs Left
        // with player Z >= 267 inside the rectangle (Z up to 270.1).
        {64,15,{-18.7,-453.2,263.1},15,-18.7,L"Left",268.6}
    };
    for(const auto& stop:stops) if(stop.node==node && stop.pedestal==pedestal) return &stop;
    return nullptr;
}
// W53 call point. Kept outside the boarding range 0xb100-0xb1ff: entering
// it takes control (the call ride), which is an arrival, not a boarding.
constexpr unsigned BevelleCallPointId=0xb235u;
}
