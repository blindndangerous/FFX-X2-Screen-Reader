#include "fixture_runtime.hpp"
#include "external_fixture.hpp"
#include "../owned_mesh_decoder.hpp"
#include <cstdio>
#include <cstring>
#include <algorithm>

using namespace ffx;
namespace {
void check(bool ok,const char* why) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
}
int main() {
    const auto rawTriangles=ffx_fixture::load<5760>("kilika-floor/triangles.bin",0x5a3b1bf6ce950080ull);
    const auto rawVertices=ffx_fixture::load<2160>("kilika-floor/vertices.bin",0xfb96a01f0371f28full);
    std::vector<RawNavigationTriangle> triangles(360);
    std::vector<RawNavigationVertex> vertices(270);
    std::memcpy(triangles.data(),rawTriangles.bytes,sizeof(rawTriangles.bytes));
    std::memcpy(vertices.data(),rawVertices.bytes,sizeof(rawVertices.bytes));
    nav::CollisionSnapshot collision;collision.actorIsControlled=true;
    // Live log: 53 is open; 54, 55 and 58 are closed. Other remaps stay
    // permissive: even this optimistic view cannot supply a shorter crossing.
    for(unsigned group:{54u,55u,58u}) collision.remaps[group-48]=1;
    Entity target;target.worker=35;target.position={59.96554946899414,9.990234375,209.56072998046875};
    target.zone.valid=true;target.zone.shape=3;target.zone.center=target.position;
    target.zone.height=-1;target.zone.width=target.zone.depth=10;
    unsigned cases=0;
    for(const Point start:std::initializer_list<Point>{{49.4769287109375,-9.998660277529202,189.728515625},
            {83.890,-9.999,175.936}}) {
        const auto mesh=decodeOwnedNavigationMesh(triangles,vertices,91.41224670410156f,collision,false,-1,start);
        check(mesh.triangles[0].collision==1 && mesh.triangles[1].collision==1,"settled pedestal keeps native group-55 floor blocked");
        const auto route=interactionRoute(mesh,start,target.zone);
        check(route.size()>=4 && inInteractionZone(target.zone,route.back()),"both live starts retain a reachable recess approach");
        double length=0;bool stairs=false;
        for(size_t i=1;i<route.size();++i) {
            length+=distanceXZ(route[i-1],route[i]);stairs|=route[i].x<-40;
            double covered=0;check(routeLineClear(mesh,route[i-1],route[i],covered),"every returned leg has continuous floor, including stairs");
        }
        double covered=0;check(!routeLineClear(mesh,start,target.position,covered),"direct recess shortcut still rejects the real floor gap");
        check(stairs && length>220 && length<280,"original floor requires the stairs around the height gap");
        if(start.x>80) check(std::abs(length-273.708)<.02 && std::abs(covered-.129274)<.001,"retained floor reproduces live detour and first uncovered interval");
        const auto diagnostic=interactionRouteDiagnostics(mesh,start,target,{},route);
        check(!diagnostic.empty() && diagnostic.front().find("interaction-route-height-change")==0,
            "required height change is informational, not an unexplained detour");
        check(std::any_of(diagnostic.begin(),diagnostic.end(),[](const std::string& row) {return row.find("blocker=floor-or-height-gap")!=std::string::npos;}),
            "informational height diagnostic retains the actual gap evidence");
        ++cases;
    }
    std::printf("PASS: %u retained Kilika floor starts, exact live detour, closed group55, continuous stairs and height diagnostic.\n",cases);
}
