// LevelScene tests (Zahra): placements match the level, land on the right cells,
// and every asset in the table exists with the measured size.

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define CGLTF_IMPLEMENTATION
#include "third_party/cgltf.h"

#include "world/Level.h"
#include "world/LevelScene.h"
#include "world/ShowcaseCamera.h"
#include "world/TempleMesh.h"

using namespace world;

#ifndef ASSETS_ROOT
#define ASSETS_ROOT "."
#endif
#ifndef LEVELS_DIR
#define LEVELS_DIR "assets/levels"
#endif

void check(bool condition, const std::string& testName)
{
    if (!condition)
        throw std::runtime_error(testName);

    std::cout << "[PASS] " << testName << '\n';
}

bool nearVec(const glm::vec3& a, const glm::vec3& b, float eps = 1e-4f)
{
    return glm::length(a - b) < eps;
}

glm::vec3 apply(const glm::mat4& m, const glm::vec3& p)
{
    return glm::vec3(m * glm::vec4(p, 1.0f));
}

const glm::ivec2 kNeighbour[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};

// Node-transformed bounds of a glTF file, the same numbers the asset table stores.
bool measure(const std::string& path, glm::vec3& mn, glm::vec3& mx)
{
    cgltf_options options{};
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success)
        return false;
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success)
    {
        cgltf_free(data);
        return false;
    }
    mn = glm::vec3(FLT_MAX);
    mx = glm::vec3(-FLT_MAX);
    for (cgltf_size n = 0; n < data->nodes_count; ++n)
    {
        const cgltf_node& node = data->nodes[n];
        if (!node.mesh)
            continue;
        float M[16];
        cgltf_node_transform_world(&node, M);
        for (cgltf_size p = 0; p < node.mesh->primitives_count; ++p)
        {
            const cgltf_primitive& prim = node.mesh->primitives[p];
            for (cgltf_size a = 0; a < prim.attributes_count; ++a)
            {
                if (prim.attributes[a].type != cgltf_attribute_type_position)
                    continue;
                const cgltf_accessor* acc = prim.attributes[a].data;
                for (cgltf_size v = 0; v < acc->count; ++v)
                {
                    float q[3];
                    cgltf_accessor_read_float(acc, v, q, 3);
                    glm::vec3 w(M[0] * q[0] + M[4] * q[1] + M[8] * q[2] + M[12],
                                M[1] * q[0] + M[5] * q[1] + M[9] * q[2] + M[13],
                                M[2] * q[0] + M[6] * q[1] + M[10] * q[2] + M[14]);
                    mn = glm::min(mn, w);
                    mx = glm::max(mx, w);
                }
            }
        }
    }
    cgltf_free(data);
    return mx.x >= mn.x;
}

void testAssetTable()
{
    const auto& table = assetTable();
    check(table.size() == (size_t)AssetId::Count, "Asset table has one entry per AssetId");

    for (size_t i = 0; i < table.size(); ++i)
    {
        const AssetInfo& a = table[i];
        const std::string name = a.name;
        check((size_t)a.id == i, name + ": table order matches AssetId");

        const std::string path = std::string(ASSETS_ROOT) + "/" + a.path;
        check(std::filesystem::exists(path), name + ": file exists (" + a.path + ")");

        glm::vec3 mn, mx;
        check(measure(path, mn, mx), name + ": loads with cgltf");
        check(nearVec(mn, a.boundsMin, 0.01f) && nearVec(mx, a.boundsMax, 0.01f),
              name + ": measured size matches the table");
    }
}

void testLevel(const std::string& file)
{
    LoadResult r = loadLevelFromFile(std::string(LEVELS_DIR) + "/" + file);
    check(r.ok, "Load " + file);
    const Level& L = r.level;
    const std::string tag = L.name + ": ";
    const float cs = L.cellSize;

    LevelScene scene = buildLevelScene(L);

    // ---- counts ----
    size_t floorCells = 0, pillars = 0;
    for (int y = 0; y < L.height; ++y)
        for (int x = 0; x < L.width; ++x)
        {
            Tile t = L.tileAt({x, y});
            if ((t == Tile::Floor || t == Tile::Pillar) && L.trapAt({x, y}) < 0)
                ++floorCells;
            if (t == Tile::Pillar)
                ++pillars;
        }
    // Visible wall faces (same as TempleMesh builds), minus faces towards a door cell,
    // which the doorway model covers.
    size_t wallFaces = buildTempleMesh(L).walls.triangleCount() / 2;
    size_t doorFaces = 0;
    for (const DoorSpawn& d : L.doors)
        for (const glm::ivec2& n : kNeighbour)
            if (L.inBounds(d.cell + n) && L.tileAt(d.cell + n) == Tile::Wall)
                ++doorFaces;

    check(scene.count(AssetId::FloorTile) == floorCells, tag + "one floor tile per open, non-trap cell");
    check(scene.count(AssetId::Wall) == wallFaces - doorFaces,
          tag + "one wall per visible wall face (hidden and door-side faces skipped)");
    check(scene.count(AssetId::Pillar) == pillars, tag + "pillar count");
    check(scene.count(AssetId::Doorway) == L.doors.size(), tag + "doorway count");
    check(scene.count(AssetId::Mirror) == L.mirrors.size(), tag + "mirror count");
    check(scene.count(AssetId::RubyPedestal) == L.rubies.size(), tag + "ruby count");
    check(scene.count(AssetId::SpikeTrap) == L.traps.size(), tag + "spike trap count");
    check(scene.count(AssetId::Torch) >= 1 && scene.count(AssetId::Torch) < wallFaces / 2,
          tag + "a few torches");

    // ---- positions ----
    bool pivotsLand = true, centred = true, onFace = true, fits = true, wallHeight = true;
    for (const Placement& p : scene.placements)
    {
        const AssetInfo& a = assetInfo(p.asset);
        if (!nearVec(apply(p.transform, assetPivot(a)), p.anchor))
            pivotsLand = false;

        glm::vec3 c = L.cellCenter(p.cell);
        if (p.face < 0)
        {
            // Cell-centred assets land on cellCenter(cell) at floor height.
            if (!nearVec(p.anchor, c))
                centred = false;
        }
        else
        {
            // Walls and torches sit on the cell edge facing an open neighbour.
            glm::ivec2 nb = p.cell + kNeighbour[p.face];
            glm::vec3 d(kNeighbour[p.face].x, 0.0f, kNeighbour[p.face].y);
            float along = glm::dot(p.anchor - c, d);
            if (L.tileAt(p.cell) != Tile::Wall || L.tileAt(nb) == Tile::Wall || L.doorAt(nb) >= 0 ||
                along <= 0.0f ||
                along > 0.5f * cs + 1e-4f)
                onFace = false;
        }

        // The scaled model must stay inside its cell (X/Z).
        for (int i = 0; i < 8; ++i)
        {
            glm::vec3 corner((i & 1) ? a.boundsMax.x : a.boundsMin.x,
                             (i & 2) ? a.boundsMax.y : a.boundsMin.y,
                             (i & 4) ? a.boundsMax.z : a.boundsMin.z);
            glm::vec3 w = apply(p.transform, corner) - c;
            bool mountedOnFace = p.asset == AssetId::Torch;   // sticks out into the room
            if (!mountedOnFace && (std::abs(w.x) > 0.5f * cs + 1e-3f || std::abs(w.z) > 0.5f * cs + 1e-3f))
                fits = false;
            if (p.asset == AssetId::Wall && (w.y < -1e-3f || w.y > L.wallHeight + 1e-3f))
                wallHeight = false;
        }
    }
    check(pivotsLand, tag + "every transform puts the asset pivot on its anchor");
    check(centred, tag + "cell assets land on cellCenter(cell)");
    check(onFace, tag + "walls/torches sit on a wall cell's edge facing open space");
    check(fits, tag + "every scaled asset (except wall torches) fits inside its cell");
    check(wallHeight, tag + "walls span floor to wallHeight");

    // ---- mirror angle (team convention: 45 = "\") ----
    bool mirrorsOk = true;
    for (const Placement& p : scene.placements)
    {
        if (p.asset != AssetId::Mirror)
            continue;
        float a = L.mirrors[L.mirrorAt(p.cell)].angleDeg;
        glm::vec3 surface = glm::normalize(glm::vec3(p.transform * glm::vec4(1, 0, 0, 0)));
        glm::vec3 facing = glm::normalize(glm::vec3(p.transform * glm::vec4(0, 0, 1, 0)));
        float ar = glm::radians(a);
        if (!nearVec(surface, {std::cos(ar), 0.0f, std::sin(ar)}) || !nearVec(facing, mirrorNormal(a)))
            mirrorsOk = false;
    }
    check(mirrorsOk, tag + "mirror model faces mirrorNormal(startAngle)");
}

void testCutaway()
{
    LoadResult r = loadLevelFromFile(std::string(LEVELS_DIR) + "/level01.txt");
    const Level& L = r.level;
    LevelSceneOptions opt;
    opt.cutawayToward = {0.0f, 1.0f};   // camera south of the level, looking north
    LevelScene full = buildLevelScene(L);
    LevelScene cut = buildLevelScene(L, opt);
    check(cut.count(AssetId::Wall) == full.count(AssetId::Wall), "Cutaway keeps every wall placement");

    bool ok = true, sawLow = false;
    for (const Placement& p : cut.placements)
    {
        if (p.asset != AssetId::Wall && p.asset != AssetId::Doorway)
            continue;
        const AssetInfo& a = assetInfo(p.asset);
        float top = apply(p.transform, {0.0f, a.boundsMax.y, 0.0f}).y;
        glm::vec3 front = glm::vec3(p.transform * glm::vec4(0, 0, 1, 0));
        bool facesAway = front.z < -1e-4f;   // front points north, away from the camera
        float expected = facesAway ? opt.cutawayHeight * L.wallHeight : L.wallHeight;
        if (std::abs(top - expected) > 1e-3f)
            ok = false;
        sawLow = sawLow || facesAway;
    }
    check(ok && sawLow, "Cutaway lowers only walls/doorway facing away from the camera");
}

void testShowcaseCamera(const std::string& file)
{
    LoadResult r = loadLevelFromFile(std::string(LEVELS_DIR) + "/" + file);
    const Level& L = r.level;
    const std::string tag = L.name + ": ";
    const float fov = 45.0f, aspect = 1280.0f / 720.0f;   // same as Renderer
    glm::mat4 levelToWorld = levelToArea(L, {0.0f, 0.0f}, 4.0f);
    ShowcaseCamera cam = ShowcaseCamera::fitLevel(L, levelToWorld, fov, aspect);
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);

    bool allOnScreen = true, tight = true;
    for (float yaw : {0.0f, 30.0f, 45.0f, 90.0f, 180.0f, 270.0f})
        for (float pitch : {15.0f, 45.0f, 60.0f, 85.0f})
        {
            cam.yawDeg = yaw;
            cam.pitchDeg = pitch;
            float maxNdc = 0.0f;
            for (int i = 0; i < 8; ++i)
            {
                glm::vec3 c((i & 1) ? L.width * L.cellSize : 0.0f, (i & 2) ? L.wallHeight : 0.0f,
                            (i & 4) ? L.height * L.cellSize : 0.0f);
                glm::vec4 clip = proj * cam.view() * levelToWorld * glm::vec4(c, 1.0f);
                glm::vec3 ndc = glm::vec3(clip) / clip.w;
                if (clip.w <= 0.0f || std::abs(ndc.x) > 1.0f || std::abs(ndc.y) > 1.0f || std::abs(ndc.z) > 1.0f)
                    allOnScreen = false;
                maxNdc = std::max({maxNdc, std::abs(ndc.x), std::abs(ndc.y)});
            }
            if (maxNdc < 0.9f)
                tight = false;
        }
    check(allOnScreen, tag + "showcase camera fits the whole level at every yaw/pitch");
    check(tight, tag + "showcase camera frames the level tightly (reaches 90% of the screen edge)");

    cam.reset();
    glm::vec3 toEye = cam.eye() - cam.target;
    check(nearVec(glm::normalize(glm::vec3(toEye.x, 0, toEye.z)),
                  {cam.towardCamera().x, 0.0f, cam.towardCamera().y}),
          tag + "towardCamera points from the level to the eye");

    cam.orbit(0.0f, 500.0f);
    cam.zoomBy(100.0f);
    check(cam.pitchDeg == ShowcaseCamera::kMaxPitch && cam.zoom == ShowcaseCamera::kMaxZoom,
          tag + "pitch and zoom are clamped");
}

void testCutawayThreshold()
{
    LoadResult r = loadLevelFromFile(std::string(LEVELS_DIR) + "/level01.txt");
    const Level& L = r.level;   // wall 3.0, cell 2.0 -> threshold atan(1.5) = 56.3 deg
    check(cutawayNeeded(L, 45.0f) && cutawayNeeded(L, 56.0f), "Cutaway needed below ~56 deg (level01)");
    check(!cutawayNeeded(L, 57.0f) && !cutawayNeeded(L, 85.0f), "No cutaway when the camera sees over walls");
}

void testLevelToArea()
{
    LoadResult r = loadLevelFromFile(std::string(LEVELS_DIR) + "/level01.txt");
    const Level& L = r.level;
    glm::mat4 m = levelToArea(L, {1.0f, -2.0f}, 4.0f, -0.5f);
    glm::vec3 centre(0.5f * L.width * L.cellSize, 0.0f, 0.5f * L.height * L.cellSize);
    check(nearVec(apply(m, centre), {1.0f, -0.5f, -2.0f}), "levelToArea: level centre lands on the area centre");
    check(std::abs(apply(m, {L.width * L.cellSize, 0, 0}).x - apply(m, {0, 0, 0}).x - 8.0f) < 1e-4f,
          "levelToArea: larger side spans 2 * halfSize");
}

int main()
{
    try
    {
        testAssetTable();
        testLevel("level01.txt");
        testLevel("level02.txt");
        testLevel("level03.txt");
        testCutaway();
        testCutawayThreshold();
        testShowcaseCamera("level01.txt");
        testShowcaseCamera("level02.txt");
        testShowcaseCamera("level03.txt");
        testLevelToArea();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[FAIL] " << e.what() << '\n';
        return 1;
    }

    std::cout << "All level scene tests passed\n";
    return 0;
}
