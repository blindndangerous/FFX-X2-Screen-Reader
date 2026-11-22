// Bevelle rail turn plan: which arrow phase each junction needs per guide
// objective, and the plan handoff from the guide thread to the game thread.
#include "bevelle_rail.hpp"
#include "nonblocking_snapshots.hpp"
#include <atomic>
namespace ffx {
namespace {
// Single writer (guide thread), single reader (game thread).
async::AtomicSnapshot<BevelleRailPlan> published;
std::atomic<bool> autoActive{false};
bool lowerTarget(unsigned t) {return t==33 || t==34 || t==35 || t==36 || t==16 || t==37;}
}
int bevelleRailWanted(unsigned worker,unsigned t) {
    const bool lower=lowerTarget(t),upper=t==30 || t==31,dead=t==32,ride=lower || upper || dead;
    switch(worker) {
    // W40 tests a pressed Confirm edge to start the circuit.
    case 40: return ride || t==13?1:-1;
    // W41 northbound, guarded g0b==1: phase 1 climbs to the hub.
    case 41: return lower || dead?1:-1;
    // W42 southbound, no guard: g0b 1 climbs to the hub, 0 U-turns north.
    case 42: return lower || dead?1:upper?0:-1;
    // W43 phases are absolute: 1 west to W30, 2 east to W31.
    case 43: return t==30?1:t==31?2:-1;
    // The W44 U-turn is never needed; the circuit restarts at the rail end.
    case 44: return -1;
    // Hub g0e: 0 dead end, 2 lower rail, 3 upper rail.
    case 45: case 46: case 47: return dead?0:lower?2:upper?3:-1;
    case 48: return t==16 || t==37?1:-1;
    case 49: return t==33?1:-1;
    case 50: return t==34 || t==35 || t==36?1:-1;
    default: return -1;
    }
}
unsigned bevelleRailPhaseBinding(unsigned worker) {
    return worker==41 || worker==42?0x0c00000b:worker==43?0x0c00000c:worker==44?0x0c00000d:
        worker>=45 && worker<=47?0x0c00000e:worker==48?0x0c00000f:worker==49?0x0c000010:worker==50?0x0c000011:0;
}
void publishBevelleRailPlan(const BevelleRailPlan& plan) {
    published.publish(plan);
}
void clearBevelleRailPlan() {
    published.publish({});
}
bool readBevelleRailPlan(BevelleRailPlan& out) {
    return published.tryRead(out);
}
bool bevelleRailAutoActive() {return autoActive.load(std::memory_order_acquire);}
void setBevelleRailAutoActive(bool active) {autoActive.store(active,std::memory_order_release);}
}
