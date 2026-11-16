// Level / TempleMesh / LevelManager tests (Zahra).
// Run with ctest, or directly:  level_test <path-to-assets/levels>

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "beam/BeamSystem.h"
#include "mirror/Mirror.h"
#include "world/Level.h"
#include "world/LevelManager.h"
#include "world/TempleMesh.h"

using namespace world;

void check(bool condition, const std::string& testName)
{
    if (!condition)
        throw std::runtime_error(testName);

    std::cout << "[PASS] " << testName << '\n';
}

bool nearVec(const glm::vec3& a, const glm::vec3& b)
{
    return glm::length(a - b) < 1e-4f;
}

glm::vec3 reflect(const glm::vec3& d, float angleDeg)
{
    return BeamSystem::reflectDirection(d, mirrorNormal(angleDeg));
}

// The level files must use exactly the same angle convention as Mirror.
void testMirrorConventionMatchesTeam()
{
    for (float a : {-80.0f, -45.0f, 0.0f, 20.0f, 45.0f, 80.0f})
    {
        Mirror m(glm::vec2(0.0f), 2.0f, a);
        glm::vec2 n2 = m.getNormal();
        check(nearVec(mirrorNormal(a), {n2.x, 0.0f, n2.y}),
              "mirrorNormal(" + std::to_string((int)a) + ") == Mirror::getNormal()");
    }

    // "\" (45): East -> South.  "/" (-45): East -> North.
    check(nearVec(reflect({1, 0, 0}, 45.0f), {0, 0, 1}), "45 deg mirror: East beam goes South");
    check(nearVec(reflect({1, 0, 0}, -45.0f), {0, 0, -1}), "-45 deg mirror: East beam goes North");
    check(std::abs(glm::length(mirrorNormal(30.0f)) - 1.0f) < 1e-5f, "mirrorNormal is unit length");
}

void testCoordinates()
{
    Level L;
    L.width = 4;
    L.height = 4;
    L.cellSize = 2.0f;
    L.tiles.assign(16, Tile::Floor);

    check(nearVec(L.cellCenter({1, 2}), {3.0f, 0.0f, 5.0f}), "cellCenter");
    check(L.cellCenterXZ({1, 2}) == glm::vec2(3.0f, 5.0f), "cellCenterXZ");
    check(L.worldToCell({3.0f, 0.0f, 5.0f}) == glm::ivec2(1, 2), "worldToCell");
    check(L.tileAt({-1, 0}) == Tile::Wall, "Out of bounds is wall");
}

void testBadLevelsAreRejected()
{
    // no spawn, open border, uneven row
    auto r = loadLevelFromString("name: Bad\ngrid:\n#####\n#L.R.\n###\nend\n");
    check(!r.ok && r.errors.size() >= 3, "Broken grid is rejected");

    r = loadLevelFromString("name: Bad2\ngrid:\n#####\n#LSR#\n##D##\nend\ndoor 0 needs 5\n");
    check(!r.ok, "Door needing a missing ruby is rejected");

    r = loadLevelFromString(
        "name: Bad3\ngrid:\n######\n#LSMR#\n##D###\nend\ndoor 0 needs 0\nmirror 0 angle 135\n");
    check(!r.ok, "Mirror angle outside [-90,90) is rejected");
}

void testLevelFile(const std::string& path, int expectMirrors, int expectRubies)
{
    LoadResult r = loadLevelFromFile(path);
    for (auto& e : r.errors)
        std::cerr << "  " << e << '\n';
    check(r.ok, "Load " + path);

    const Level& L = r.level;
    const std::string tag = L.name + ": ";
    check((int)L.mirrors.size() == expectMirrors, tag + "mirror count");
    check((int)L.rubies.size() == expectRubies, tag + "ruby count");
    check(L.isWalkable(L.playerSpawn), tag + "spawn is walkable");

    // Every mirror angle must fit Mirror's default rotation limits (+-80).
    for (auto& m : L.mirrors)
        check(std::abs(m.angleDeg) <= 80.0f && std::abs(m.solutionDeg) <= 80.0f,
              tag + "mirror angles fit Mirror limits");

    std::vector<std::string> why;
    bool solvable = isSolvable(L, &why);
    for (auto& w : why)
        std::cerr << "  " << w << '\n';
    check(solvable, tag + "solvable with solution angles");

    // Puzzle must NOT already be solved at the starting angles.
    bool solvedAtStart = true;
    for (auto& d : L.doors)
        for (int ruby : d.needsRubies)
        {
            bool lit = false;
            for (int i = 0; i < (int)L.lights.size(); ++i)
                if (traceBeamOnGrid(L, i, false).hitRuby == ruby)
                    lit = true;
            if (!lit)
                solvedAtStart = false;
        }
    check(!solvedAtStart, tag + "not solved at start");

    TempleMesh m = buildTempleMesh(L);
    check(m.floor.triangleCount() > 0 && m.walls.triangleCount() > 0 &&
              m.ceiling.triangleCount() > 0,
          tag + "floor/walls/ceiling meshes built");
    check(m.walls.indices.size() % 3 == 0, tag + "wall index count is a multiple of 3");

    bool horizontal = true;
    for (auto& v : m.walls.vertices)
        if (std::abs(v.normal.y) > 1e-6f)
            horizontal = false;
    check(horizontal, tag + "wall normals are horizontal");
    check(!buildStaticColliders(L).empty(), tag + "colliders built");

    std::cout << "  " << L.name << "  (" << L.width << "x" << L.height << ", "
              << m.floor.triangleCount() + m.walls.triangleCount() + m.ceiling.triangleCount() +
                     m.pillars.triangleCount() + m.pits.triangleCount()
              << " triangles)\n";
}

void testManager(const std::string& dir)
{
    LevelManager mgr({dir + "/level01.txt", dir + "/level02.txt", dir + "/level03.txt"});
    check(mgr.loadFirst(), "LevelManager loads first level");
    check(mgr.current().name == "The Entrance Hall", "First level name");
    check(mgr.next() && mgr.next(), "next() twice");
    check(mgr.isLastLevel() && mgr.index() == 2, "On last level");
    check(!mgr.next(), "next() past the end fails");
    check(mgr.reload() && mgr.index() == 2, "reload() keeps the level");
}

int main(int argc, char** argv)
{
#ifdef LEVELS_DIR
    std::string dir = LEVELS_DIR;
#else
    std::string dir = "assets/levels";
#endif
    if (argc > 1)
        dir = argv[1];

    try
    {
        testMirrorConventionMatchesTeam();
        testCoordinates();
        testBadLevelsAreRejected();
        testLevelFile(dir + "/level01.txt", 1, 1);
        testLevelFile(dir + "/level02.txt", 2, 1);
        testLevelFile(dir + "/level03.txt", 4, 2);
        testManager(dir);
    }
    catch (const std::exception& e)
    {
        std::cerr << "[FAIL] " << e.what() << '\n';
        return 1;
    }

    std::cout << "All level tests passed\n";
    return 0;
}
