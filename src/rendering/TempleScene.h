#pragma once
#include "Renderer.h"
#include "gameplay/GameSession.h"
#include "world/WorldMetrics.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

class TempleScene {
public:
    Model floor,wall,pillar,mirror,ranger,door,spike,ruby,rubySealed;
    bool animated=false;
    bool load() {
        const std::string kit="assets/vendor/kaykit_dungeon/KayKit_Dungeon_Pack_1.1_FREE/Assets/gltf/";
        bool ok=floor.load(kit+"floor_tile_large.gltf");
        ok=wall.load(kit+"wall.gltf")&&ok;
        ok=pillar.load(kit+"pillar_decorated.gltf")&&ok;
        ok=mirror.load("assets/models/mirrors/mirror_complete.glb")&&ok;
        ok=ranger.load("assets/models/characters/ranger/Ranger.glb",true)&&ok;
        animated=ranger.loadAnimations("assets/animations/characters/kaykit_medium/Rig_Medium_General.glb",
            "assets/animations/characters/kaykit_medium/Rig_Medium_MovementBasic.glb");
        ok=door.load("assets/models/environment/doors/temple_sliding_door.glb")&&ok;
        ok=spike.load("assets/models/traps/spike_trap.glb")&&ok;
        ok=ruby.load("assets/models/ruby/ruby_pedestal_floatingmaster.glb")&&ok;
        ok=rubySealed.load("assets/models/ruby/ruby_pedestal_sealing.glb")&&ok;
        return ok && mirror.findMesh("Mirror_Reflective_Surface")>=0 && ruby.findMesh("RubyCrystal")>=0
            && rubySealed.findMesh("RubyCrystal")>=0;
    }
    static glm::mat4 place(glm::vec3 position,float scale=1,float yaw=0) {
        auto m=glm::translate(glm::mat4(1),position);
        m=glm::rotate(m,yaw,glm::vec3(0,1,0));
        return glm::scale(m,glm::vec3(scale));
    }
    void draw(Renderer& r,const GameSession& g,float time,bool animateWalk=false) {
        if(animated && !ranger.animate(time,g.player.isMoving()||animateWalk)) {
            animated=false;ranger.load("assets/models/characters/ranger/Ranger.glb");
        }
        const auto& l=g.levels.current();const float cs=l.cellSize;
        std::vector<glm::vec3> lights,colors;
        for(size_t i=0;i<g.rubies.size();++i) {
            lights.push_back(l.cellCenter(l.rubies[i].cell,metrics::beamHeight));
            colors.push_back(g.rubies[i].isActivated()?glm::vec3(1.8f,.35f,.18f):glm::vec3(.6f,.04f,.03f));
        }
        for(const auto& e:l.lights) {lights.push_back(l.cellCenter(e.cell,metrics::beamHeight));colors.push_back({1.5f,1,.3f});}
        r.setLights(lights,colors);
        const glm::ivec2 neighbors[]={{0,-1},{1,0},{0,1},{-1,0}};
        for(int z=0;z<l.height;++z) for(int x=0;x<l.width;++x) {
            const glm::ivec2 cell(x,z);const auto tile=l.tileAt(cell);const auto p=l.cellCenter(cell);
            if(tile==world::Tile::Wall) {
                const bool border=x==0||z==0||x==l.width-1||z==l.height-1;
                if(!border) r.drawBox(p+glm::vec3(0,(l.wallHeight-.025f)*.5f,0),{cs-.04f,l.wallHeight-.025f,cs-.04f},{.22f,.25f,.3f});
                for(int side=0;side<4;++side) {
                    const auto n=cell+neighbors[side];
                    if(!l.inBounds(n)||l.tileAt(n)==world::Tile::Wall) continue;
                    const glm::vec3 direction(neighbors[side].x,0,neighbors[side].y);
                    const auto face=p+direction*(cs*.5f-.25f);
                    if(z==l.height-1) {
                        // Explicit camera cutaway at the same inner collision boundary.
                        r.drawBox(face+glm::vec3(0,.18f,0),{cs,.36f,.5f},{.24f,.28f,.33f});
                    } else {
                        const float yaw=side==0?glm::pi<float>():side==1?glm::half_pi<float>():side==2?0:-glm::half_pi<float>();
                        r.drawModel(wall,place(face,cs/4.f,yaw),glm::vec3(.82f,.88f,1));
                    }
                }
            } else if(tile==world::Tile::Pit) {
                r.drawModel(floor,place(p+glm::vec3(0,-l.pitDepth-.025f,0),cs/4),{.13f,.17f,.23f});
                for(const auto& n:neighbors) if(l.tileAt(cell+n)!=world::Tile::Pit) {
                    auto edge=p+glm::vec3(n.x,0,n.y)*cs*.5f;
                    const glm::vec3 size=n.x?glm::vec3(.1f,l.pitDepth,cs):glm::vec3(cs,l.pitDepth,.1f);
                    r.drawBox(edge+glm::vec3(0,-l.pitDepth*.5f,0),size,{.17f,.21f,.28f});
                }
            } else {
                if(l.trapAt(cell)<0) r.drawModel(floor,place(p+glm::vec3(0,-.025f,0),cs/4),{.75f,.82f,.94f});
                if(tile==world::Tile::Pillar) r.drawModel(pillar,place(p,.5f),glm::vec3(1));
            }
        }
        for(const auto& m:l.mirrors) r.drawShadow(l.cellCenter(m.cell),.85f);
        for(const auto& ruby:l.rubies) r.drawShadow(l.cellCenter(ruby.cell),.95f);
        for(const auto& e:l.lights) r.drawShadow(l.cellCenter(e.cell),.6f);
        const auto playerPosition=g.player.position();r.drawShadow({playerPosition.x,0,playerPosition.z},.6f);
        for(size_t i=0;i<g.doors.size();++i) {
            const auto p=l.cellCenter(l.doors[i].cell);
            // Uniform authored scale keeps the arch and panel proportions. Only the
            // sliding panel retracts below the floor; the stone frame never moves.
            const float scale=cs/2.65f;
            for(unsigned int mesh=0;mesh<door.getMeshCount();++mesh) {
                const bool panel=door.meshName(mesh).rfind("panel_",0)==0;
                const auto shift=panel?glm::vec3(0,-3.4f*g.doorLift,0):glm::vec3(0);
                r.drawModelMesh(door,mesh,place(p+shift,scale),glm::vec3(1),0,panel?0.f:-1000.f);
            }
            if(g.doors[i].isOpen()) {
                const auto mark=p+glm::vec3(0,.03f,-.15f);
                r.drawBeam(mark-glm::vec3(.75f,0,0),mark+glm::vec3(.75f,0,0),{1,.52f,.08f},.045f);
            }
        }
        for(size_t i=0;i<g.rubies.size();++i) {
            const auto p=l.cellCenter(l.rubies[i].cell);const bool lit=g.rubies[i].isActivated();
            const Model& asset=lit?rubySealed:ruby;
            const int crystal=asset.findMesh("RubyCrystal");
            for(unsigned int mesh=0;mesh<asset.getMeshCount();++mesh) {
                auto pos=p;float glow=0;
                if(int(mesh)==crystal) {
                    const float centre=asset.meshCenter(mesh).y;
                    pos.y=metrics::beamHeight-centre;
                    if(lit) pos.y*=1-g.rubySealing[i];
                    else pos.y+=std::sin(time*1.8f+float(i))*.025f;
                    glow=lit?.75f:.12f;
                } else if(asset.meshName(mesh).find("Rune")!=std::string::npos) glow=lit?.7f:.12f;
                r.drawModelMesh(asset,mesh,place(pos),glm::vec3(1),glow);
            }
        }
        const int selected=g.nearbyMirror();
        const int surfaceIndex=mirror.findMesh("Mirror_Reflective_Surface");
        const auto surface=mirror.meshCenter(surfaceIndex);const auto normal=mirror.meshNormal(surfaceIndex);
        const float upright=std::atan2(normal.y,normal.z);
        for(size_t i=0;i<g.mirrors.size();++i) {
            const auto p=g.mirrors[i].getCenter();
            auto moving=place({p.x,metrics::beamHeight,p.y},metrics::mirrorScale,glm::radians(-g.mirrors[i].getAngle()));
            moving=glm::rotate(moving,upright,glm::vec3(1,0,0));
            moving=glm::translate(moving,-surface);
            for(unsigned int mesh=0;mesh<mirror.getMeshCount();++mesh) {
                const auto& name=mirror.meshName(mesh);
                const bool panel=name.rfind("Mirror_",0)==0||name.rfind("Bronze_Rim",0)==0||name.rfind("Gold_Accent",0)==0;
                r.drawModelMesh(mirror,mesh,panel?moving:place({p.x,0,p.y},metrics::mirrorScale),glm::vec3(1),int(i)==selected?.10f:0);
            }
        }
        for(const auto& e:l.lights) {
            const auto p=l.cellCenter(e.cell);auto mat=place(p,.55f);
            // Existing emitter pedestal stays grounded; its authored crystal is moved
            // to the horizontal optical plane by the procedural emitter transform.
            r.drawGemstone(mat,{1,.55f,.08f},metrics::beamHeight/.55f-1.75f);
            const auto dir=world::dirToVec(e.dir);
            r.drawBeam(l.cellCenter(e.cell,metrics::beamHeight),l.cellCenter(e.cell,metrics::beamHeight)+dir*.32f,{1,.52f,.08f},.14f);
        }
        for(size_t i=0;i<g.traps.size();++i) {
            const auto p=l.cellCenter(l.traps[i].cell);
            for(unsigned int mesh=0;mesh<spike.getMeshCount();++mesh) {
                const bool spikes=spike.meshName(mesh)=="spikes";
                auto mat=place(p+glm::vec3(0,-.025f+(spikes?g.trapExtension[i]*.85f:0),0),cs/4);
                r.drawModelMesh(spike,mesh,mat,glm::vec3(1),0,spikes?0.f:-1000.f);
            }
        }
        const auto p=g.player.position(),f=g.player.facing();
        r.drawModel(ranger,place({p.x,.002f,p.z},metrics::rangerScale,std::atan2(f.x,f.z)),glm::vec3(1));
        // Thin bright core and a wider translucent gold shell; depth-tested in 3D.
        r.drawBeamSegments(g.beams,{1,.48f,.055f},.10f,.08f);
        r.drawBeamSegments(g.beams,{1,.68f,.15f},.027f,.40f);
    }
};
