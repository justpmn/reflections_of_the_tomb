#pragma once
// Level data + loader for Reflections of the Tomb.
// Owner: Zahra Isaiho (Level design & temple environment)
//
// Coordinate convention (shared with beam / mirror / player code):
//   - Grid cell (col, row). col -> +X (East), row -> +Z (South). North = -Z.
//   - World centre of a cell = ((col + 0.5) * cellSize, y, (row + 0.5) * cellSize)
//   - Floor is at y = 0, ceiling at y = wallHeight.
//   - 2D gameplay code (Mirror, BeamTracer) uses glm::vec2(x, z) - see cellCenterXZ().
//   - Mirror angle: same convention as mirror/Mirror.h. The surface runs along
//     (cos a, sin a) in (x, z) and the normal is (-sin a, cos a).
//     Seen from above (north up):  45 = "\"   -45 = "/"   0 = "-"   90 = "|".

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace world {

enum class Tile : char {
    Wall   = '#',
    Floor  = '.',
    Pit    = '_',
    Pillar = 'P'
};

enum class Dir { N, E, S, W };

glm::vec3 dirToVec(Dir d);          // N = (0,0,-1), E = (1,0,0) ...
bool      parseDir(const std::string& s, Dir& out);

// Named *Spawn so it does not clash with the beam module's LightSource.
struct LightSpawn { glm::ivec2 cell{0}; Dir dir = Dir::E; };

struct MirrorSpawn {
    glm::ivec2 cell{0};
    float angleDeg    = 45.0f;   // starting angle in [-90, 90); puzzle should start "wrong"
    float solutionDeg = 0.0f;    // designer's answer, used by the solvability check
    bool  hasSolution = false;
    bool  rotatable   = true;
    float alternateSolutionDeg = 0.0f;
    bool hasAlternateSolution = false;
};

struct RubySpawn { glm::ivec2 cell{0}; };

struct DoorSpawn {
    glm::ivec2 cell{0};
    std::vector<int> needsRubies;   // door opens when ALL these rubies are lit
};

struct TrapSpawn {
    glm::ivec2 cell{0};
    bool timed = false;
    float safeSeconds=5, warningSeconds=1, extendedSeconds=2, phaseOffset=0;
    int disabledByRuby = -1;        // -1 = no ruby disable link
};

struct Level {
    std::string name;
    int   width = 0, height = 0;
    float cellSize   = 2.0f;
    float wallHeight = 3.0f;
    float pitDepth   = 2.0f;

    std::vector<Tile> tiles;        // entity cells (S, L, M, R, D, T) are stored as Floor
    glm::ivec2 playerSpawn{-1, -1};

    std::vector<LightSpawn>  lights;
    std::vector<MirrorSpawn> mirrors;
    std::vector<RubySpawn>   rubies;
    std::vector<DoorSpawn>   doors;
    std::vector<TrapSpawn>   traps;

    bool inBounds(glm::ivec2 c) const { return c.x >= 0 && c.y >= 0 && c.x < width && c.y < height; }
    Tile tileAt(glm::ivec2 c) const;                 // out of bounds -> Wall
    glm::vec3  cellCenter(glm::ivec2 c, float y = 0.0f) const;
    glm::vec2  cellCenterXZ(glm::ivec2 c) const;     // for Mirror / BeamTracer (vec2 = x, z)
    glm::ivec2 worldToCell(const glm::vec3& p) const;

    // Static walkability: floor tile with no mirror / ruby / light on it.
    // Doors and traps are NOT checked here - gameplay owns their open/active state.
    bool isWalkable(glm::ivec2 c) const;
    bool isPit(glm::ivec2 c) const { return tileAt(c) == Tile::Pit; }

    int mirrorAt(glm::ivec2 c) const;   // index or -1
    int rubyAt(glm::ivec2 c) const;
    int doorAt(glm::ivec2 c) const;
    int lightAt(glm::ivec2 c) const;
    int trapAt(glm::ivec2 c) const;
};

struct LoadResult {
    bool ok = false;
    Level level;
    std::vector<std::string> errors;
};

LoadResult loadLevelFromString(const std::string& text);
LoadResult loadLevelFromFile(const std::string& path);
std::vector<std::string> validateLevel(const Level& lvl);

// Unit 3D normal of a mirror at angleDeg: Mirror::getNormal() lifted to (x, 0, z).
glm::vec3 mirrorNormal(float angleDeg);

// ---- Design check (not the real beam system) ----
// Walks a beam cell-by-cell from a light, reflecting with R = D - 2(D.N)N.
// Uses solution angles if useSolution, else starting angles.
struct GridBeam {
    std::vector<glm::ivec2> cells;
    int hitRuby = -1;
};
GridBeam traceBeamOnGrid(const Level& lvl, int lightIndex, bool useSolution);

// True if, with the solution angles, every ruby a door needs gets lit.
bool isSolvable(const Level& lvl, std::vector<std::string>* why = nullptr);

} // namespace world
