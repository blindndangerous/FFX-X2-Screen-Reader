// Dispatch depth/water routes before target kind, and refuse red-hazard routes.
// The owned form uses the supplied snapshot instead of refreshing native actors.
// See ../../Docs/features/navigation/index.md#routes-and-validity.
#include "butterfly_navigation.hpp"
#include "butterfly.hpp"
#include "butterfly_paths.hpp"
#include "swimming_route.hpp"
namespace ffx {
namespace {
std::vector<Entity> hazards(const std::vector<Entity>& targets) {
    std::vector<Entity> red;
    for(const auto& e:targets) if(e.hazardous) red.push_back(e);
    return red;
}
}
std::vector<Point> navigationRoute(const Mesh& mesh,Point origin,const Entity& target) {
    if(target.swimDepth) return swimmingRoute(mesh,origin,target);
    if(!target.butterfly) return target.zone.valid?interactionRoute(mesh,origin,target.zone,target.swimApproach):routePoints(mesh,origin,target.position);
    if(target.hazardous) return {};
    auto fresh=target;
    if(fresh.butterflyStart) refreshButterflyStarter(fresh); else refreshButterflyTarget(fresh);
    if(fresh.inactive || fresh.hazardous) return {};
    return butterflyRoute(mesh,origin,fresh,hazards(butterflyTargets()));
}
std::vector<Point> navigationRouteOwned(const Mesh& mesh,Point origin,const Entity& target,const std::vector<Entity>& liveTargets) {
    if(mesh.panels.required && (target.butterfly || target.swimDepth)) return {};
    if(target.swimDepth) return swimmingRoute(mesh,origin,target);
    if(!target.butterfly) {
        auto current=target;avoidContactPedestals(current,liveTargets);
        auto route=current.zone.valid?interactionRoute(mesh,origin,current.zone,current.swimApproach):routePoints(mesh,origin,current.position);
        if(mesh.panels.required || target.panelLocal) return panelSafeRoute(mesh,target,route);
        return route;
    }
    if(target.inactive || target.hazardous) return {};
    return butterflyRoute(mesh,origin,target,hazards(liveTargets));
}
bool navigationTargetReached(const Entity& target,Point point) {
    if(target.swimDepth) return point.y>250;
    return target.butterfly? !target.hazardous && butterflyInTarget(target,point):inInteractionZone(target.zone,point);
}
}
