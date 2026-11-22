#pragma once
#include "events.hpp"
#include "nav_entries.hpp"
#include <type_traits>
#include "atel_resources.hpp"

namespace ffx {
namespace nav {struct CrossWaterBake;}
struct Point { double x = 0, y = 0, z = 0; };
// Copied finite scripted crossings, not extra floor or injected movement.
// landing identifies the native movement's permitted side of each line.
struct DirectedCrossing {Point a,b,landing;double height=0;};
struct RouteEvents {std::array<DirectedCrossing,4> rows{};unsigned kind=0,count=0;bool ready=true;};
struct Triangle { int vertices[3]{}, neighbours[3]{}; unsigned collision = 0, collisionGroup = 0;
    Point points[3]{}, center{}; bool valid = false; };
// Owned type-6 player-entry rectangles, indexed by dome0700 local 57..83.
struct FloorPanel {Point center;double cosine=1,sine=0,width=0,depth=0;int worker=-1;};
struct FloorPanels {
    std::array<FloorPanel,27> rows{};std::array<int,10> pattern{};
    uintptr_t chunk=0;unsigned serial=0,epoch=0,revision=0,sample=0,active=0;
    ULONGLONG capturedAt=0;double radius=0;bool required=false,ready=false;
};
struct Mesh { std::vector<Triangle> triangles; bool openPlane=false,fixedHeight=false; double planeY=0;
    int playerTriangle=-1; Point player; bool swimmingFloor=false; std::vector<std::array<double,3>> floorY;
    const nav::CrossWaterBake* crossWater=nullptr;unsigned waterCrossings=0;std::vector<Triangle> terrain;RouteEvents events; FloorPanels panels; };
bool swimmingPlane();
struct InteractionShape { Point center,end; double angle=0,width=0,depth=0,range=0,height=-1,viewAngle=3.141592653589793;
    unsigned shape=0; int worker=-1; bool valid=false,crossing=false,talk=false; };
struct InteractionZone : InteractionShape { std::vector<InteractionShape> competing;
    double contactRadius=0,contactMinY=-1e30,contactMaxY=1e30; Point approach;
    Point obstacle; double obstacleRadius=0; };
struct Entity { std::wstring name, category; unsigned id = 0; int actor = -1, worker = -1;
    Point position; double distance = 0; InteractionZone zone; bool unreachable=false, inactive=false; unsigned pushControl=0,pushLocal=0;
    EntryMask usefulEntries; unsigned panelLocal=0;bool authoredTrialControl=false,butterfly=false,hazardous=false,butterflyStart=false;
    unsigned butterflyEpoch=0; bool swimApproach=false,swimDepth=false,swimReady=true; double swimClearance=0; uintptr_t swimChunk=0;
    // The scan set zone.obstacle to this control's own solid body.
    bool ownKeepOut=false; };
// Native steering carries copied facts, never an allocating Entity/vector/string.
// At most500 loaded workers supply competitors;512 includes authored recess facts.
struct NativeInteractionRows {
    std::array<InteractionShape,512> rows;
    unsigned count=0;
    const InteractionShape* begin() const {return rows.data();}
    const InteractionShape* end() const {return rows.data()+count;}
    const InteractionShape& operator[](size_t index) const {return rows[index];}
};
struct NativeInteractionZone : InteractionShape {
    NativeInteractionRows competing;
    double contactRadius=0,contactMinY=-1e30,contactMaxY=1e30;
    Point approach,obstacle;double obstacleRadius=0;
};
struct NativeRouteTarget {
    std::array<wchar_t,2049> name{};
    unsigned id=0,pushControl=0,pushLocal=0;int actor=-1,worker=-1;
    Point position;NativeInteractionZone zone;
    unsigned panelLocal=0;bool inactive=false,butterfly=false,swimDepth=false;uintptr_t swimChunk=0;
};
static_assert(std::is_trivially_copyable_v<NativeRouteTarget> && std::is_trivially_destructible_v<NativeRouteTarget>);
// A guided sphere step routes to a Kilika, Djose or Macalania pedestal with
// its recess talk zone copied on (guideTargets). Push stand zones are never
// talk zones. Arrival must home into the talk zone and confirm it, like the
// recess itself: in-game 2026-11-20 the push-style stop left the player
// outside the native take range at Macalania step 29.
template<class Target> bool pedestalSocketTarget(const Target& target) {
    return target.pushControl>=2 && target.pushControl<=4 && target.zone.valid && target.zone.talk;
}
std::vector<unsigned> interactionEntries(uintptr_t worker,EntryMask entryMask,unsigned* channels=nullptr);
struct NativeInteractionFacts {unsigned channels=0;std::array<unsigned,7> entries{};bool entryReady=false;};
// Native captures only the original fixed seven-channel getters/flags. Saved
// callback proofs run later over owned script tables and copied saved facts.
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t worker,EntryMask entryMask={});
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t worker,EntryMask,const AtelSpan& observedSpan);
InteractionZone interactionZone(uintptr_t worker,unsigned channels);
void constrainInteractionZone(InteractionZone&,uintptr_t worker);
void configurePedestalTarget(Entity&,const char* verifiedScript,unsigned localWorker);
bool pedestalRecessTarget(const Entity&,uintptr_t worker,Entity& recess);
void refreshTargetZone(Entity&,uintptr_t worker,unsigned channels);
void refreshNavigationTarget(Entity&);
double actorContactRadius(uintptr_t actor,uintptr_t player);
std::wstring pedestalInstruction(const Entity&);
bool inInteractionZone(const InteractionZone&,Point);
bool inInteractionZone(const NativeInteractionZone&,Point);
std::vector<Point> interactionRoute(const Mesh&,Point,const InteractionZone&,bool horizontalApproach=false);
// Owned failure/detour evidence only; bounded rows, no native reads or speech.
// A supplied route emits only above the distance thresholds.
std::vector<std::string> interactionRouteDiagnostics(const Mesh&,Point,const Entity&,const std::vector<Entity>&,const std::vector<Point>& route={});
double panelLegCorridor(const FloorPanels&,unsigned exempt,Point,Point);
bool insidePanel(const FloorPanel&,Point);
bool touchesPanel(const FloorPanel&,Point,Point,double margin);
std::vector<Point> panelSafeRoute(const Mesh&,const Entity&,const std::vector<Point>&);
bool routeLineClear(const Mesh&,Point,Point,double& coveredUntil);
// Besaid and Kilika pedestals start their push script on body contact. Give
// every other target's zone that pedestal as an obstacle to walk around.
void avoidContactPedestals(std::vector<Entity>&);
void avoidContactPedestals(Entity&,const std::vector<Entity>&);
void refreshNavigation();
#ifdef FFX_DEV_UTILITIES
// Development pipe: select (and for walk, route and auto walk to) the first
// listed destination whose label contains text, ignoring case. Native owner;
// the result is spoken and published as a "target" pipe event.
bool devNavigationSelect(const wchar_t* text,bool walk);
// Development pipe: write the next ready navigation Scene to
// %FFX_EVIDENCE_ROOT%\scenes\<name>.ffxscene (+ .txt). Native owner;
// the preparation owner writes and publishes a "scene" pipe event.
bool devNavigationDump(const wchar_t* name);
#endif
bool requestNavigationAssistRoute();
void requestNavigationAssistInstruction();
void resetNavigationScripts();
void announceCurrentMap();
void announceMapArrival(Event&);
void finishMapArrival();
bool activeField();
bool destinationUnreachable(const Mesh&,Point,const Entity&);
Mesh readMesh();
Point playerPosition();
uintptr_t playerActor();
bool faceNavigationTarget(const Entity&);
bool faceNavigationTarget(const NativeRouteTarget&);
double distanceXZ(Point a, Point b);
int findTriangle(const Mesh& mesh, Point point);
bool nearWalkable(const Mesh& mesh, Point point, double distanceSquared = 9);
bool floorPortal(const Triangle&,const Triangle&,Point& midpoint);
// Shared graph/smoothing/consumer guard for actual native event direction.
bool routeSegmentAllowed(const Mesh&,Point from,Point to);
bool routeEdgeAllowed(const Mesh&,int from,int to,Point portal,const Point* origin=nullptr);
std::vector<int> path(const Mesh& mesh, int start, int end,bool requirePortal=false,const Point* origin=nullptr);
std::vector<Point> smooth(const Mesh& mesh, const std::vector<Point>& points);
std::wstring direction(Point delta);
bool cameraBasis(Point& forward,Point& right);
bool relativeBearing(Point delta,double& pan,double& front);
unsigned nearbyWalls(const Mesh&,Point,Point forward,Point right,double range);
std::vector<Point> routePoints(const Mesh&,Point start,Point end);
std::vector<Point> routePoints(const Mesh&,Point start,Point end,int endTriangle);
void scanExits(Point player, std::vector<Entity>& entities);
#ifdef FFX_READER_TEST
bool navigationSelectedForTest(Entity&); // Preparation-owner fixture observation only.
unsigned navigationPendingForTest();
#endif
}
