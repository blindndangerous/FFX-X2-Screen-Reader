// Synthetic Djose trial scene (djyt0600), built from the live facts recorded in
// puzzle_guide_owned.cpp: floors at y -18 and -67, the electric gap at z 117
// (collision group 50), jump stages (0,-18,74.6) and (0,-18,162.9), the
// central lift pedestal W1 at (0,-18,50) and the gap switch W61 at
// (0,-19.5,180). No game data: geometry is four flat quads. Trial values come
// from puzzle_jump_data.hpp, the same writes "puzzle jump <n>" makes.
// Include after the fixture's ffx stubs; it sets the stubbed ATEL identity.
class DjoseSyntheticScene {
public:
    static constexpr unsigned TriangleCount=8;
    std::unique_ptr<nav::Scene> scene=std::make_unique<nav::Scene>();
    std::vector<Entity> entities;
    int gapSwitchEntity=-1;

    DjoseSyntheticScene() {
        const TrialUtility* trial=nullptr;
        for(const auto& candidate:trialUtilities) if(!std::strcmp(candidate.script,"djyt0600")) trial=&candidate;
        if(!trial) unexpected("Djose trial identity exists");
        auto& s=*scene;
        s.epoch=1;s.primary=0x10000;s.partCount=1;s.control=true;s.confirmMask=0x20;s.room=trial->room;
        identity={};identity.span={s.primary,s.epoch,1,0x100};
        identity.length=trial->length;identity.checksum=trial->checksum;std::memcpy(identity.name,"djyt0600",8);
        auto& part=s.parts[0];part.span=identity.span;part.bindingsReady=part.guideReady=true;
        s.actorCount=1;s.playerIndex=0;
        for(const auto& b:nav::guide_djyt0600) part.bindings.rows[part.bindings.count++]={b.low,0,b.high,b.element};
        positioned(1,{0,-18,50},0);    // lower pedestal: the central lift once 0xdb=1
        positioned(61,{0,-19.5,180});  // gap switch, across the gap
        positioned(63,{0,-18,-100});   // back door camera hint
        Entity gapSwitch;gapSwitch.name=L"Gap switch";gapSwitch.worker=static_cast<int>(index(61));
        gapSwitch.position=s.workers[index(61)].position;gapSwitchEntity=gapSwitch.worker;entities.push_back(gapSwitch);
        mesh();player({0,-18,20});
    }
    void player(Point p) {scene->player=p;scene->capturedAt=GetTickCount64();}
    void set(unsigned low,int value) {
        for(auto& row:scene->parts[0].bindings.rows) if(row.binding==low) {row.value=value;return;}
        unexpected("binding absent from guide_djyt0600");
    }
    // Trial state as "puzzle jump <n>" leaves it. Writes to values the guide
    // does not capture (0x96 lift circuit) have no row and are skipped.
    void applyJump(unsigned step) {
        auto& bindings=scene->parts[0].bindings;
        for(unsigned i=0;i<bindings.count;++i) bindings.rows[i].value=0;
        for(const auto& w:puzzleJumpWrites(djoseJumpStages,step))
            for(unsigned i=0;i<bindings.count;++i) {auto& row=bindings.rows[i];
                if(row.binding==w.low && row.element==w.element && row.high==w.high) row.value=w.value;}
    }
private:
    unsigned index(unsigned local) const {
        for(unsigned i=0;i<scene->count;++i) if(scene->workers[i].local==local) return i;
        unexpected("synthetic worker exists");
    }
    void positioned(unsigned local,Point p,int actor=-1) {
        auto& w=scene->workers[scene->count];w.global=scene->count++;
        w.valid=true;w.part=0;w.local=local;w.type=3;w.actor=actor;w.position=p;w.interactions.channels=1;
        const auto field=[&](unsigned offset,float v) {std::memcpy(w.geometry.data()+offset,&v,sizeof(v));};
        field(0,static_cast<float>(p.x));field(4,static_cast<float>(p.y));field(8,static_cast<float>(p.z));
        field(0x1c,7);field(0x30,7);field(0x38,7);field(0x40,7);
    }
    void mesh() {
        auto& s=*scene;constexpr float scale=10;unsigned vertices=0,triangles=0;
        const auto quad=[&](double x0,double x1,double z0,double z1,double y,unsigned group) {
            const unsigned first=vertices;
            for(const auto& corner:{Point{x0,y,z0},Point{x1,y,z0},Point{x1,y,z1},Point{x0,y,z1}}) {
                auto& v=s.vertices[vertices++];
                v.coordinates[0]=static_cast<std::int16_t>(corner.x*scale);v.coordinates[1]=static_cast<std::int16_t>(corner.y*scale);
                v.coordinates[2]=static_cast<std::int16_t>(corner.z*scale);v.coordinates[3]=0;
            }
            for(const auto& t:{std::array<unsigned,3>{0,1,2},std::array<unsigned,3>{0,2,3}}) {
                auto& raw=s.triangles[triangles++];raw.collision=group;
                for(unsigned j=0;j<3;++j) {raw.vertices[j]=static_cast<std::int16_t>(first+t[j]);raw.neighbours[j]=-1;}
            }
        };
        quad(-40,40,0,110,-18,1);     // south floor, entrance side
        quad(-40,40,110,124,-18,50);  // electric gap floor (blocked group 50)
        quad(-40,40,124,200,-18,1);   // north floor, gap switch side
        quad(-40,40,40,110,-67,2);    // upper floor (the lift rises to it)
        s.geometryValid=true;auto& g=s.geometry.geometry;g.count=triangles;g.vertexCount=vertices;g.scale=scale;
        if(triangles!=TriangleCount) unexpected("synthetic triangle count");
    }
};
