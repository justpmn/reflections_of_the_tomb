#include "LevelScene.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace world {

namespace {

#define KAYKIT "assets/vendor/kaykit_dungeon/KayKit_Dungeon_Pack_1.1_FREE/Assets/gltf/"

// Every asset the level scene uses. Order must match AssetId.
// Bounds measured with cgltf (node transforms applied); level_scene_test re-measures
// the files and fails if they drift from these numbers.
std::vector<AssetInfo> makeTable() {
    return {
        {AssetId::FloorTile, "floor_tile_large", KAYKIT "floor_tile_large.gltf",
         {-2.0f, -0.1f, -2.0f}, {2.0f, 0.05f, 2.0f}, Pivot::TopCentre, 1.0f, false},
        {AssetId::Wall, "wall", KAYKIT "wall.gltf",
         {-2.0f, 0.0f, -0.5f}, {2.0f, 4.0f, 0.5f}, Pivot::BaseCentre, 1.0f, true},
        {AssetId::Pillar, "pillar_decorated", KAYKIT "pillar_decorated.gltf",
         {-1.116f, 0.0f, -0.75f}, {1.116f, 4.0f, 0.96f}, Pivot::BaseCentre, 0.5f, true},
        {AssetId::Doorway, "wall_doorway", KAYKIT "wall_doorway.gltf",
         {-2.0f, 0.0f, -0.5f}, {2.0f, 4.0f, 0.5f}, Pivot::BaseCentre, 1.0f, true},
        {AssetId::Mirror, "mirror_complete", "assets/models/mirrors/mirror_complete.glb",
         {-0.71f, 0.0f, -0.625f}, {0.71f, 3.027f, 0.625f}, Pivot::BaseCentre, 0.7f, false},
        {AssetId::RubyPedestal, "ruby_pedestal_floating", "assets/models/ruby/ruby_pedestal_floating.glb",
         {-0.665f, 0.0f, -0.665f}, {0.665f, 1.7f, 0.665f}, Pivot::BaseCentre, 0.6f, false},
        {AssetId::SpikeTrap, "spike_trap", "assets/models/traps/spike_trap.glb",
         {-2.0f, -2.0f, -2.0f}, {2.0f, 0.1f, 2.0f}, Pivot::TopCentre, 1.0f, false},
        {AssetId::Torch, "torch_mounted", KAYKIT "torch_mounted.gltf",
         {-0.275f, -0.381f, 0.0f}, {0.275f, 0.682f, 0.616f}, Pivot::BackCentre, 0.3f, false},
    };
}

const glm::ivec2 kNeighbour[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};  // N E S W

// Yaw (radians about +Y) that turns the model's +Z towards direction d (x, z).
float yawTowards(glm::vec2 d) { return std::atan2(d.x, d.y); }

glm::mat4 place(const AssetInfo& a, const Level& L, glm::vec3 anchor, float yaw,
                float heightFactor = 1.0f) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), anchor);
    m = glm::rotate(m, yaw, glm::vec3(0, 1, 0));
    m = glm::scale(m, assetScale(a, L) * glm::vec3(1.0f, heightFactor, 1.0f));
    return glm::translate(m, -assetPivot(a));
}

bool isOpen(const Level& L, glm::ivec2 c) { return L.inBounds(c) && L.tileAt(c) != Tile::Wall; }

}  // namespace

const std::vector<AssetInfo>& assetTable() {
    static const std::vector<AssetInfo> table = makeTable();
    return table;
}

const AssetInfo& assetInfo(AssetId id) { return assetTable()[static_cast<std::size_t>(id)]; }

glm::vec3 assetPivot(const AssetInfo& a) {
    glm::vec3 c = 0.5f * (a.boundsMin + a.boundsMax);
    switch (a.pivot) {
        case Pivot::BaseCentre: return {c.x, a.boundsMin.y, c.z};
        case Pivot::TopCentre:  return {c.x, a.boundsMax.y, c.z};
        case Pivot::BackCentre: return {c.x, c.y, a.boundsMin.z};
    }
    return c;
}

glm::vec3 assetScale(const AssetInfo& a, const Level& L) {
    glm::vec3 size = a.boundsMax - a.boundsMin;
    float s  = a.footprint * L.cellSize / std::max(size.x, size.z);
    float sy = a.stretchToWallHeight ? L.wallHeight / size.y : s;
    return {s, sy, s};
}

std::size_t LevelScene::count(AssetId id) const {
    return (std::size_t)std::count_if(placements.begin(), placements.end(),
                                      [id](const Placement& p) { return p.asset == id; });
}

LevelScene buildLevelScene(const Level& L, const LevelSceneOptions& opt) {
    LevelScene out;
    auto add = [&](AssetId id, glm::ivec2 cell, int face, glm::vec3 anchor, float yaw,
                   float heightFactor = 1.0f) {
        out.placements.push_back(
            {id, place(assetInfo(id), L, anchor, yaw, heightFactor), cell, face, anchor});
    };
    // Front faces direction d: cut away when it points away from the camera.
    auto cutaway = [&](glm::ivec2 d) {
        return glm::dot(glm::vec2(d), opt.cutawayToward) < -1e-6f;
    };
    const float cs = L.cellSize;
    int visibleFaces = 0;

    for (int r = 0; r < L.height; ++r) {
        for (int c = 0; c < L.width; ++c) {
            glm::ivec2 cell{c, r};
            Tile t = L.tileAt(cell);
            glm::vec3 centre = L.cellCenter(cell);

            if (t == Tile::Wall) {
                // One slab per face that touches open space, flush with the cell edge
                // (same faces TempleMesh builds). Hidden walls produce nothing.
                const AssetInfo& w = assetInfo(AssetId::Wall);
                float thickness = (w.boundsMax.z - w.boundsMin.z) * assetScale(w, L).z;
                for (int s = 0; s < 4; ++s) {
                    glm::ivec2 nb = cell + kNeighbour[s];
                    if (!isOpen(L, nb)) continue;
                    // The doorway model fills the whole door cell, frame included,
                    // so wall faces towards a door cell would only poke through it.
                    if (L.doorAt(nb) >= 0) continue;
                    glm::vec3 d(kNeighbour[s].x, 0.0f, kNeighbour[s].y);
                    float yaw = yawTowards(kNeighbour[s]);
                    bool low = cutaway(kNeighbour[s]);
                    add(AssetId::Wall, cell, s, centre + d * (0.5f * cs - 0.5f * thickness), yaw,
                        low ? opt.cutawayHeight : 1.0f);

                    bool plainFloor = L.tileAt(nb) == Tile::Floor && L.doorAt(nb) < 0;
                    if (opt.torchEvery > 0 && plainFloor && !low &&
                        visibleFaces++ % opt.torchEvery == 0)
                        add(AssetId::Torch, cell, s,
                            centre + d * (0.5f * cs) + glm::vec3(0, opt.torchHeight * L.wallHeight, 0),
                            yaw);
                }
                continue;
            }
            if (t == Tile::Pit) continue;    // open hole: TempleMesh supplies the pit sides

            if (L.trapAt(cell) >= 0) add(AssetId::SpikeTrap, cell, -1, centre, 0.0f);
            else                     add(AssetId::FloorTile, cell, -1, centre, 0.0f);

            if (t == Tile::Pillar) add(AssetId::Pillar, cell, -1, centre, 0.0f);

            int m = L.mirrorAt(cell);
            if (m >= 0) add(AssetId::Mirror, cell, -1, centre, glm::radians(-L.mirrors[m].angleDeg));

            if (L.rubyAt(cell) >= 0) add(AssetId::RubyPedestal, cell, -1, centre, 0.0f);

            if (L.doorAt(cell) >= 0) {
                // Face the doorway towards the open cell inside the temple.
                float yaw = 0.0f;
                bool low = false;
                for (int s = 0; s < 4; ++s) {
                    glm::ivec2 nb = cell + kNeighbour[s];
                    if (isOpen(L, nb)) {
                        yaw = yawTowards(kNeighbour[s]);
                        low = cutaway(kNeighbour[s]);
                        break;
                    }
                }
                add(AssetId::Doorway, cell, -1, centre, yaw, low ? opt.cutawayHeight : 1.0f);
            }
        }
    }
    return out;
}

glm::mat4 levelToArea(const Level& L, glm::vec2 centreXZ, float halfSize, float y) {
    glm::vec3 levelCentre(0.5f * L.width * L.cellSize, 0.0f, 0.5f * L.height * L.cellSize);
    float s = 2.0f * halfSize / (std::max(L.width, L.height) * L.cellSize);
    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(centreXZ.x, y, centreXZ.y));
    m = glm::scale(m, glm::vec3(s));
    return glm::translate(m, -levelCentre);
}

} // namespace world
