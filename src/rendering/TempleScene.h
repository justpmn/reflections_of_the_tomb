#pragma once
#include "Renderer.h"
#include "gameplay/GameSession.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

// Imported team models fitted to the same world units used by collision/physics.
class TempleScene {
public:
    Model floor, wall, pillar, mirror, ranger, door, spike, ruby;
    bool animated=false;
    bool load() {
        const std::string kit="assets/vendor/kaykit_dungeon/KayKit_Dungeon_Pack_1.1_FREE/Assets/gltf/";
        bool ok=floor.load(kit+"floor_tile_large.gltf");
        ok=wall.load(kit+"wall.gltf") && ok;
        ok=pillar.load(kit+"pillar_decorated.gltf") && ok;
        ok=mirror.load("assets/models/mirrors/mirror_complete.glb") && ok;
        ok=ranger.load("assets/models/characters/ranger/Ranger.glb",true) && ok;
        animated=ranger.loadAnimations("assets/animations/characters/kaykit_medium/Rig_Medium_General.glb",
            "assets/animations/characters/kaykit_medium/Rig_Medium_MovementBasic.glb");
        ok=door.load("assets/models/environment/doors/temple_sliding_door.glb") && ok;
        ok=spike.load("assets/models/traps/spike_trap.glb") && ok;
        ok=ruby.load("assets/models/ruby/ruby_pedestal_floating.glb") && ok;
        return ok;
    }
    static glm::mat4 fit(const Model& m,glm::vec3 p,glm::vec3 size,float angle=0) {
        const auto center=(m.minBounds+m.maxBounds)*.5f;
        auto extent=glm::max(m.maxBounds-m.minBounds,glm::vec3(.001f));
        auto mat=glm::translate(glm::mat4(1),p);
        mat=glm::rotate(mat,angle,glm::vec3(0,1,0));
        mat=glm::scale(mat,size/extent);
        return glm::translate(mat,glm::vec3(-center.x,-m.minBounds.y,-center.z));
    }
    void draw(Renderer& r,const GameSession& g, float animationTime, bool animateWalk=false) {
        if(animated && !ranger.animate(animationTime,g.player.isMoving()||animateWalk)) {
            animated=false;
            ranger.load("assets/models/characters/ranger/Ranger.glb");
        }
        const auto& l=g.levels.current();
        const float cs=l.cellSize;
        for(int z=0;z<l.height;++z) for(int x=0;x<l.width;++x) {
            const glm::ivec2 cell(x,z); const auto t=l.tileAt(cell);
            const auto p=l.cellCenter(cell);
            if(t==world::Tile::Wall) {
                // Low south parapet preserves the player's sightline through the cutaway temple.
                const float h=z==l.height-1?.65f:l.wallHeight;
                r.drawModel(wall,fit(wall,p,{cs,h,cs}),glm::vec3(1));
            } else if(t==world::Tile::Pit) {
                r.drawModel(floor,fit(floor,p+glm::vec3(0,-l.pitDepth,0),{cs,.18f,cs}),{.2f,.22f,.3f});
            } else {
                r.drawModel(floor,fit(floor,p+glm::vec3(0,-.16f,0),{cs,.16f,cs}),glm::vec3(1));
                if(t==world::Tile::Pillar) r.drawModel(pillar,fit(pillar,p,{cs*.5f,2.1f,cs*.5f}),glm::vec3(1));
            }
        }
        for(size_t i=0;i<g.doors.size();++i) {
            auto p=l.cellCenter(l.doors[i].cell); p.y=g.doorLift*3.4f;
            r.drawModel(door,fit(door,p,{cs,3.f,.35f}),{.8f,.7f,.45f});
        }
        for(size_t i=0;i<g.rubies.size();++i) {
            // Crystal centre in this model is fitted to y=1.3, matching the sphere target.
            const auto p=l.cellCenter(l.rubies[i].cell);
            const bool lit=g.rubies[i].isActivated();
            auto mat=glm::translate(glm::mat4(1),p);
            // Fit actual crystal centre to beam height while resting the pedestal on the floor.
            const float crystalScale=1.3f/(ruby.meshCenter(8).y-ruby.minBounds.y);
            mat=glm::scale(mat,glm::vec3(crystalScale));
            mat=glm::translate(mat,{0,-ruby.minBounds.y,0});
            r.drawModel(ruby,mat,glm::vec3(1));
            if(lit) {
                const auto top=p+glm::vec3(0,1.95f,0);
                r.drawBeam(top-glm::vec3(.18f,0,0),top+glm::vec3(.18f,0,0),{.2f,1,.4f},.08f);
            }
        }
        const int selected=g.nearbyMirror();
        for(size_t i=0;i<g.mirrors.size();++i) {
            const auto p=g.mirrors[i].getCenter();
            // Mesh 17 is the team's reflective surface. Fit its actual bounds to
            // the physical segment and beam height; keep the pedestal on the floor.
            const auto surface=mirror.meshCenter(17);
            const float vertical=1.3f/(surface.y-mirror.minBounds.y);
            auto mat=glm::translate(glm::mat4(1),{p.x,1.3f,p.y});
            // Model surface is local XY; physics surface tangent is (cos a,sin a) in XZ.
            mat=glm::rotate(mat,glm::radians(-g.mirrors[i].getAngle()),glm::vec3(0,1,0));
            mat=glm::scale(mat,glm::vec3(1.3f/mirror.meshSize(17).x,vertical,vertical));
            mat=glm::translate(mat,-surface);
            for(unsigned int mesh=0;mesh<22;++mesh)
                r.drawModelMesh(mirror,mesh,mat,static_cast<int>(i)==selected?glm::vec3(1,.85f,.45f):glm::vec3(.8f));
            auto base=glm::translate(glm::mat4(1),{p.x,-mirror.minBounds.y*.65f,p.y});
            base=glm::scale(base,glm::vec3(.65f));
            for(unsigned int mesh=22;mesh<mirror.getMeshCount();++mesh)
                r.drawModelMesh(mirror,mesh,base,glm::vec3(1));
        }
        for(const auto& e:l.lights) {
            auto mat=glm::translate(glm::mat4(1),l.cellCenter(e.cell));
            // Procedural gemstone's crystal is at local y=1.75; scale and offset to beam height.
            mat=glm::translate(mat,{0,.425f,0});
            mat=glm::scale(mat,glm::vec3(.5f));
            r.drawGemstone(mat,{1,.72f,.15f});
        }
        for(size_t i=0;i<g.traps.size();++i) {
            auto p=l.cellCenter(l.traps[i].cell);
            r.drawModel(spike,fit(spike,p,{1.6f,g.traps[i].isActive()?.65f:.06f,1.6f}),{.8f,.4f,.3f});
        }
        const auto p=g.player.position(),f=g.player.facing();
        r.drawModel(ranger,fit(ranger,{p.x,0,p.z},{.75f,1.65f,.7f},std::atan2(f.x,f.z)),{.8f,.9f,.75f});
        r.drawBeamSegments(g.beams,{1,.79f,.22f},.065f);
    }
};
