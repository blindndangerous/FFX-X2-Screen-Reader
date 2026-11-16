// Kilika and Besaid pedestals start their push script on any body contact,
// which disables player control (in-game F23, F25). Walks to other targets,
// and to the pedestal's own sphere recess, must stay out of that contact.
bool keepsClear(const std::vector<Point>& route,Point center,double radius) {
    if(route.empty()) return false;
    for(size_t i=1;i<route.size();++i) {
        const Point a=route[i-1],b=route[i];const double x=b.x-a.x,z=b.z-a.z,length=x*x+z*z;
        const double t=length>0?std::clamp(((center.x-a.x)*x+(center.z-a.z)*z)/length,0.0,1.0):0;
        if(std::hypot(a.x+t*x-center.x,a.z+t*z-center.z)<radius) return false;
    }
    return true;
}
void contactPedestalFixtures(const Mesh& room) {
    const Point center{50,0,40};
    Entity pedestal; pedestal.worker=6; pedestal.pushControl=2; pedestal.position=center;
    pedestal.zone.valid=true; pedestal.zone.shape=1; pedestal.zone.center=center; pedestal.zone.contactRadius=5; pedestal.zone.height=-1;
    Entity recess; recess.worker=6; recess.name=L"Pedestal sphere recess"; recess.position=center;
    recess.zone.valid=true; recess.zone.talk=true; recess.zone.shape=1; recess.zone.center=center; recess.zone.range=5.8; recess.zone.height=5;
    recess.zone.approach={0,0,1};
    Entity storage; storage.worker=40; storage.name=L"Side-wall storage recess 1"; storage.position={36,0,40};
    storage.zone.valid=true; storage.zone.talk=true; storage.zone.shape=1; storage.zone.center={36,0,40}; storage.zone.range=3; storage.zone.height=5;
    std::vector<Entity> list{pedestal,recess,storage};
    avoidContactPedestals(list);
    check(list[0].zone.obstacleRadius==0 && list[0].zone.contactRadius==5,"the pushed pedestal keeps its own contact staging");
    const auto toRecess=interactionRoute(room,{50,0,25},list[1].zone);
    check(keepsClear(toRecess,center,5),"a walk to the Kilika pedestal sphere recess goes around the pedestal");
    check(!toRecess.empty() && distanceXZ(toRecess.back(),center)>=5.5 && inInteractionZone(list[1].zone,toRecess.back()),
        "the sphere recess walk stops in talk range, outside pedestal contact");
    const auto toStorage=interactionRoute(room,{56,0,40},list[2].zone);
    check(keepsClear(toStorage,center,5) && inInteractionZone(list[2].zone,toStorage.back()),
        "a walk past the Kilika pedestal to a storage recess goes around it");
    auto inner=list[1].zone; inner.range=4;
    check(interactionRoute(room,{50,0,25},inner).empty(),"an unreachable talk zone never discards the pedestal keep-out");
    // In-game F26: walking around the Kilika pedestal to its push side
    // touched it five times. Only the last, inward leg may come close.
    InteractionZone staging; staging.valid=true; staging.shape=1; staging.center=center; staging.contactRadius=5; staging.approach={-1,0,0};
    const auto around=interactionRoute(room,{62,0,40},staging);
    check(around.size()>2 && inInteractionZone(staging,around.back()) &&
        keepsClear(std::vector<Point>(around.begin(),around.end()-1),center,7.5) && keepsClear(around,center,5),
        "a walk around the Kilika pedestal to its push side keeps a wide margin");
    // In-game F25: a walk that starts touching the pedestal steps straight
    // out first instead of sliding along it.
    const Point touching{44.9,0,41.2};
    const auto away=interactionRoute(room,touching,list[1].zone);
    const double rx=touching.x-center.x,rz=touching.z-center.z;
    const double sx=away.size()>1?away[1].x-touching.x:0,sz=away.size()>1?away[1].z-touching.z:0;
    check(away.size()>2 && (rx*sx+rz*sz)>.95*std::hypot(rx,rz)*std::hypot(sx,sz) &&
        keepsClear(std::vector<Point>(away.begin()+1,away.end()),center,5.5),
        "a walk that starts touching the Kilika pedestal steps straight out first");
    // In-game F27: from the reset tile, north-east of the pedestal, the first
    // leg ran straight at it, and the walk ran on into it (nine touches). No
    // leg before the last may aim at the pedestal: three units past its end
    // the walk must still be clear.
    const auto runsOnClear=[&](const std::vector<Point>& points,size_t from,double radius) {
        for(size_t i=from+1;i+1<points.size();++i) {
            const Point a=points[i-1],b=points[i];const double length=distanceXZ(a,b);if(length<.01) continue;
            const Point past{b.x+(b.x-a.x)/length*3,b.y,b.z+(b.z-a.z)/length*3};
            if(distanceXZ(past,center)<radius) return false;
        }
        return true;
    };
    const Point northEast{center.x+43.8,0,center.z+43.8};
    const auto fromTile=interactionRoute(room,northEast,staging);
    check(fromTile.size()>2 && inInteractionZone(staging,fromTile.back()) && runsOnClear(fromTile,0,7) &&
        keepsClear(std::vector<Point>(fromTile.begin(),fromTile.end()-1),center,7.5),
        "a walk from north-east of the Kilika pedestal to its west push side does not aim at it");
    const Point touchingNorthEast{center.x+3.46,0,center.z+3.46};
    const auto fromTouch=interactionRoute(room,touchingNorthEast,staging);
    const double ox=fromTouch.size()>1?fromTouch[1].x-touchingNorthEast.x:0,oz=fromTouch.size()>1?fromTouch[1].z-touchingNorthEast.z:0;
    check(fromTouch.size()>2 && inInteractionZone(staging,fromTouch.back()) && ox>0 && oz>0 &&
        std::abs(ox-oz)<.05*std::hypot(ox,oz) && runsOnClear(fromTouch,1,7),
        "a walk that starts touching the Kilika pedestal on its north-east side steps out, then goes around");
    // B': escape the full clearance band before turning, leaving room for
    // the route owner's one-unit waypoint handoff. Cover inside, edge and near.
    auto fire=list[2].zone; fire.center={50,0,20};
    for(const auto start:{Point{45.3,0,41.2},Point{45,0,40},touching,Point{46,0,46}}) {
        for(const auto& destination:{list[1].zone,list[2].zone,fire}) {
            const auto escape=interactionRoute(room,start,destination);
            const double x=start.x-center.x,z=start.z-center.z;
            const double dx=escape.size()>1?escape[1].x-start.x:0,dz=escape.size()>1?escape[1].z-start.z:0;
            check(escape.size()>2 && distanceXZ(escape[1],center)>=8.4 && x*dx+z*dz>0 &&
                std::abs(x*dz-z*dx)<.001 && keepsClear(std::vector<Point>(escape.begin()+1,escape.end()),center,5.5),
                "nearby pedestal/storage/fire walks escape radially before turning");
        }
    }
    // A: the observed alternating corners (-4,+6)/(-4,-6) must route to
    // the west axis outside the learned keep-out, then approach radially.
    staging.obstacle=center; staging.obstacleRadius=7.6;
    for(double z:{6.0,-6.0}) {
        const auto approach=interactionRoute(room,{center.x-4,0,center.z+z},staging);
        check(approach.size()>2 && inInteractionZone(staging,approach.back()),
            "both Kilika corner contacts retain a route to the west push side");
        const auto before=approach[approach.size()-2],last=approach.back();
        check(keepsClear(std::vector<Point>(approach.begin()+1,approach.end()-1),center,7.6) &&
            std::abs(before.z-center.z)<.001 && before.x<center.x-10 &&
            std::abs(last.z-center.z)<.001 && last.x<center.x-5,
            "the final push-side leg starts outside clearance with zero perpendicular offset");
    }
}
void zoneFixtures() {
    Mesh counter;
    counter.triangles={triangle({0,0,0},{8,0,0},{0,0,10}),triangle({8,0,0},{8,0,10},{0,0,10}),
        triangle({10,0,0},{20,0,0},{10,0,10}),triangle({20,0,0},{20,0,10},{10,0,10})};
    counter.triangles[0].neighbours[0]=1; counter.triangles[1].neighbours[0]=0;
    counter.triangles[2].neighbours[0]=3; counter.triangles[3].neighbours[0]=2;
    const unsigned vertices[][3]={{0,1,2},{1,3,2},{4,5,6},{5,7,6}};
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<3;++j) counter.triangles[i].vertices[j]=vertices[i][j];
    const Point player{1,0,5}; InteractionZone zone;
    zone.valid=true; zone.shape=1; zone.center={12,0,5}; zone.range=5; zone.height=2;
    check(routePoints(counter,player,zone.center).empty(),"counter blocks a route to the actor center");
    auto route=interactionRoute(counter,player,zone);
    check(!route.empty() && route.back().x<=8 && inInteractionZone(zone,route.back()),"reachable front-of-counter interaction zone replaces inaccessible actor center");
    Entity target; target.position=zone.center; target.zone=zone;
    check(!destinationUnreachable(counter,player,target),"unreachable filter keeps a shop with a reachable counter zone");
    target.zone={}; check(destinationUnreachable(counter,player,target),"filter excludes a disconnected destination");
    check(!destinationUnreachable(Mesh{},player,target),"unknown floor cannot silently hide a destination");
    zone.range=2; check(interactionRoute(counter,player,zone).empty(),"no route is invented when a zone misses every connected floor");
    zone.range=5; zone.center.y=100; check(interactionRoute(counter,player,zone).empty(),"talk height excludes a separate floor");
    zone.center.y=0; zone.height=-1;
    check(!inInteractionZone(zone,{12,6,5}),"negative circular height uses native three-dimensional range");
    zone.height=0; check(!inInteractionZone(zone,{12,0,5}),"zero circular talk height is not an accessible zone");
    zone.shape=3; zone.width=2; zone.depth=4; zone.angle=1.5707963267948966; zone.range=0; zone.height=2;
    route=interactionRoute(counter,player,zone);
    check(!route.empty() && route.back().x<=8 && inInteractionZone(zone,route.back()),"rotated native rectangle reaches its accessible edge");
    zone.center={6,0,0}; zone.end={6,0,10}; zone.shape=2; zone.height=2; zone.crossing=true;
    route=interactionRoute(counter,player,zone);
    check(!route.empty() && route.back().x>6 && route.back().x<=8,"line exit destination crosses the native trigger rather than stopping on it");
    zone.shape=3; zone.center={6,0,5}; zone.width=1; zone.depth=2; zone.angle=0;
    route=interactionRoute(counter,player,zone);
    check(!route.empty() && route.back().x>5,"rectangle crossing target is inside the native boundary");
    Mesh room; room.triangles={triangle({0,0,0},{100,0,0},{0,0,100}),triangle({100,0,0},{100,0,100},{0,0,100})};
    room.triangles[0].neighbours[0]=1; room.triangles[1].neighbours[0]=0;
    const unsigned ids[][3]={{0,1,2},{1,3,2}};
    for(unsigned i=0;i<2;++i) for(unsigned j=0;j<3;++j) room.triangles[i].vertices[j]=ids[i][j];
    InteractionZone pedestal; pedestal.valid=true; pedestal.shape=1; pedestal.center={50,0,40};
    pedestal.range=20; pedestal.height=5; pedestal.talk=true; pedestal.worker=7;
    InteractionShape recess; recess.valid=true; recess.shape=3; recess.center={53,0,40};
    recess.width=20; recess.depth=5; recess.range=10; recess.height=5; recess.talk=true; recess.worker=44;
    pedestal.competing={recess};
    check(!inInteractionZone(pedestal,{31,0,40}),"inside pedestal range is insufficient while the rectangular recess wins");
    auto push=interactionRoute(room,{20,0,40},pedestal);
    check(!push.empty() && inInteractionZone(pedestal,push.back()) && distanceXZ(push.back(),pedestal.center)<10,
        "pedestal route enters the native selection region instead of stopping at its larger talk radius");
    InteractionZone slot; static_cast<InteractionShape&>(slot)=recess;
    slot.competing={static_cast<const InteractionShape&>(pedestal)};
    auto insert=interactionRoute(room,{20,0,40},slot);
    check(!insert.empty() && inInteractionZone(slot,insert.back()) && !inInteractionZone(pedestal,insert.back()),
        "pedestal and recess have distinct actionable route endpoints");
    check(!inInteractionZone(pedestal,{50,0,30}),"equally scored overlapping controls cannot claim arrival");
    // A guide sphere step names one pedestal face, so the route ends on it.
    InteractionZone face; face.valid=true; face.shape=1; face.center={50,0,40}; face.range=6; face.height=5; face.talk=true; face.worker=7;
    face.approach={1,0,0};
    check(!inInteractionZone(face,{45,0,40}),"socket face zone refuses the opposite face");
    check(!inInteractionZone(face,{50,0,45}),"socket face zone refuses a side face");
    const auto east=interactionRoute(room,{20,0,40},face);
    check(!east.empty() && inInteractionZone(face,east.back()) && east.back().x>53,"socket face route ends on the named face");
    pedestal.competing[0].viewAngle=.05;
    check(inInteractionZone(pedestal,{50,0,30}),"native facing cone excludes a competing control outside the facing direction");
    pedestal.competing[0].center=pedestal.center; pedestal.competing[0].range=100;
    pedestal.competing[0].width=100; pedestal.competing[0].depth=100;
    pedestal.competing[0].viewAngle=3.141592653589793; pedestal.shape=3; pedestal.range=100;
    check(interactionRoute(room,{20,0,40},pedestal).empty(),"indistinguishable native controls cannot produce a false arrival");
    InteractionZone contact; contact.valid=true; contact.shape=1; contact.center={50,0,40};
    contact.contactRadius=7; contact.approach={0,0,1};
    contact.competing={recess};
    auto approach=interactionRoute(room,{20,0,40},contact);
    check(!approach.empty() && std::abs(approach.back().x-50)<.65 && approach.back().z>47,
        "Besaid push route approaches the rear collision boundary rather than its talk circle");
    check(!inInteractionZone(contact,{50,0,59}) && !inInteractionZone(contact,{50,0,33}),
        "talk range and the opposite pedestal face cannot report push arrival");
    check(inInteractionZone(contact,{50,0,47.25}) && !inInteractionZone(contact,{53,0,47}),
        "push arrival requires close aligned contact and ignores sphere-recess talk competition");
    auto opposite=interactionRoute(room,{50,0,20},contact);
    check(opposite.size()>2 && inInteractionZone(contact,opposite.back()),
        "approaching Besaid from the opposite face detours around the physical pedestal");
    bool clear=true;
    for(size_t i=1;i<opposite.size();++i) {
        const auto a=opposite[i-1],b=opposite[i];
        const double x=b.x-a.x,z=b.z-a.z,length=x*x+z*z;
        const double t=length>0?std::clamp(((50-a.x)*x+(40-a.z)*z)/length,0.0,1.0):0;
        if(distanceXZ({a.x+t*x,0,a.z+t*z},contact.center)<7) clear=false;
    }
    check(clear,"every push detour leg stays outside the actor collision cylinder");
    contact.contactMinY=-2; contact.contactMaxY=4;
    check(inInteractionZone(contact,{50,3,47.25}) && !inInteractionZone(contact,{50,5,47.25}),
        "native vertical bounds permit slope contact while excluding a separate height");
    contact.contactMinY=-1e30; contact.contactMaxY=1e30;
    contact.approach={};
    approach=interactionRoute(room,{20,0,40},contact);
    check(!approach.empty() && approach.back().x<43 && std::abs(approach.back().z-40)<.65,
        "Kilika chooses a reachable cardinal push face without aiming at the recess");
    contact.contactRadius=10; contact.approach={0,0,1};
    approach=interactionRoute(room,{20,0,40},contact);
    check(!approach.empty() && approach.back().z>50,
        "updated actor collision size changes the push approach endpoint");
    Mesh water; water.openPlane=true; water.fixedHeight=true; water.planeY=-50;
    auto swim=routePoints(water,{1,-50,1},{90,-50,90});
    check(swim.size()==2 && swim.back().y==-50,"native free swimming routes stay on the current height plane");
    contact.center.y=-50; contact.approach={0,0,1}; contact.contactRadius=7;
    const auto openDetour=interactionRoute(water,{50,-50,20},contact);
    check(openDetour.size()>2 && inInteractionZone(contact,openDetour.back()),
        "open-plane contact routes also avoid crossing the target actor");
    zone.valid=true; zone.shape=1; zone.center={50,-50,40}; zone.range=5; zone.height=2; zone.crossing=false;
    check(!interactionRoute(water,{1,-50,1},zone).empty(),"free swimming reaches native interaction zones without a walking mesh");
    zone.center.y=0; check(interactionRoute(water,{1,-50,1},zone).empty(),"free swimming cannot route to ship-deck interactions at another height");
    zone.valid=false; check(interactionRoute(counter,player,zone).empty(),"unresolved geometry never manufactures an interaction destination");
    contactPedestalFixtures(room);
}
