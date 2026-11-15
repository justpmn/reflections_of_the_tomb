#pragma once
// Turns a world::Level into a list of model placements {asset, transform}, so the
// renderer can draw the level with the repo's 3D assets instead of plain geometry.
// Owner: Zahra Isaiho. No OpenGL in here - see LevelSceneRenderer for drawing.
//
// Conventions (same as Level.h):
//   - Level space: cell (col,row) -> centre ((col+0.5)*cs, 0, (row+0.5)*cs), floor at y = 0.
//   - Mirror yaw follows mirror/Mirror.h: angle a = surface along (cos a, sin a) in (x,z),
//     45 = "\" seen from above. The mirror model's glass lies along its X axis and faces +Z,
//     so it is rotated by -a about +Y; its +Z then equals mirrorNormal(a).
//   - Asset sizes in the table are the node-transformed glTF bounds, i.e. what a Model
//     that applies glTF node transforms draws (Joe's Model on feature/joe-temple-environment).

#include <cstddef>
#include <vector>
#include <glm/glm.hpp>
#include "Level.h"

namespace world {

enum class AssetId {
    FloorTile,
    Wall,
    Pillar,
    Doorway,
    Mirror,
    RubyPedestal,
    SpikeTrap,
    Torch,
    TempleDoor,   // the team's door model; Doorway is its fallback
    Count
};

// Which point of the model is put on the placement's anchor point.
enum class Pivot {
    BaseCentre,   // centre in X/Z, lowest Y      (stands on the floor)
    TopCentre,    // centre in X/Z, highest Y     (top surface flush with the floor)
    BackCentre    // centre in X/Y, lowest Z      (back plate against a wall)
};

struct AssetInfo {
    AssetId     id;
    const char* name;
    const char* path;           // relative to the repo root (the game's working directory)
    glm::vec3   boundsMin;      // measured model-space bounds (node transforms applied)
    glm::vec3   boundsMax;
    Pivot       pivot;
    float       footprint;      // largest X/Z extent is scaled to footprint * cellSize
    bool        stretchToWallHeight;   // Y scaled to wallHeight instead of uniform scale
};

// One table with every asset the level scene uses.
const std::vector<AssetInfo>& assetTable();
const AssetInfo& assetInfo(AssetId id);
glm::vec3 assetPivot(const AssetInfo& a);          // model-space pivot point
glm::vec3 assetScale(const AssetInfo& a, const Level& lvl);

struct Placement {
    AssetId    asset;
    glm::mat4  transform;   // model space -> level space
    glm::ivec2 cell;        // the cell this placement belongs to
    int        face = -1;   // wall/torch: side of `cell` it sits on (0=N 1=E 2=S 3=W), else -1
    glm::vec3  anchor;      // level-space point the asset's pivot lands on

    // Drawn instead of `asset` if that model fails to load (Count = no fallback).
    // Fitted separately, so its pivot also lands on `anchor`.
    AssetId    fallback = AssetId::Count;
    glm::mat4  fallbackTransform{1.0f};
};

struct LevelSceneOptions {
    int   torchEvery  = 6;      // a torch on every Nth visible wall face (0 = no torches)
    float torchHeight = 0.6f;   // fraction of wallHeight

    // Cutaway for a fixed camera: walls and doorways whose front faces away from
    // `cutawayToward` (x, z direction pointing at the camera) are lowered to
    // cutawayHeight * wallHeight so they don't hide the room. (0,0) = off.
    glm::vec2 cutawayToward{0.0f, 0.0f};
    float     cutawayHeight = 0.25f;
};

struct LevelScene {
    std::vector<Placement> placements;
    std::size_t count(AssetId id) const;
};

// Floor tiles on open cells (not on pits or traps), one wall slab per visible wall face
// (hidden walls and faces towards a door cell are skipped - the doorway model fills that
// cell), pillars, the temple door (KayKit doorway as fallback), mirrors at their start
// angle, rubies, spike traps and a few wall torches.
LevelScene buildLevelScene(const Level& lvl, const LevelSceneOptions& opt = {});

// Maps level space into a square area of the game world: the level's centre goes to
// (centreXZ.x, y, centreXZ.y) and its larger side is scaled to 2 * halfSize.
glm::mat4 levelToArea(const Level& lvl, glm::vec2 centreXZ, float halfSize, float y = 0.0f);

} // namespace world
