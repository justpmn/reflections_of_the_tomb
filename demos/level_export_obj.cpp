// Exports a level's temple geometry (world::buildTempleMesh) as a Wavefront .obj,
// so it can be checked in Blender / any 3D viewer or used for report screenshots.
//   ./build/level_export_obj assets/levels/level01.txt build/level01.obj [--ceiling]
// One object per mesh: floor, walls, ceiling, pillars, pits. The ceiling is left out
// by default so you can look into the temple from above; --ceiling includes it.
// Exit code: 0 = ok, 1 = bad arguments / load error, 2 = cannot write the output.

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "world/Level.h"
#include "world/TempleMesh.h"

using namespace world;

struct NamedMesh
{
    const char* name;
    const MeshData* mesh;
};

int main(int argc, char** argv)
{
    std::vector<std::string> paths;
    bool withCeiling = false;
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--ceiling")
            withCeiling = true;
        else
            paths.push_back(a);
    }
    if (paths.size() != 2)
    {
        std::cerr << "usage: level_export_obj <level.txt> <out.obj> [--ceiling]\n";
        return 1;
    }

    LoadResult r = loadLevelFromFile(paths[0]);
    if (!r.ok)
    {
        for (auto& e : r.errors)
            std::cerr << e << '\n';
        return 1;
    }
    const Level& L = r.level;
    const TempleMesh m = buildTempleMesh(L);

    std::vector<NamedMesh> meshes = {{"floor", &m.floor}, {"walls", &m.walls}};
    if (withCeiling)
        meshes.push_back({"ceiling", &m.ceiling});
    meshes.push_back({"pillars", &m.pillars});
    meshes.push_back({"pits", &m.pits});

    std::ofstream out(paths[1]);
    if (!out)
    {
        std::cerr << "cannot write " << paths[1] << '\n';
        return 2;
    }

    char buf[128];
    out << "# Reflections of the Tomb - " << L.name << '\n'
        << "# " << L.width << "x" << L.height << " cells, cell_size " << L.cellSize
        << ", +X = East, +Z = South, +Y = up, CCW front faces\n";

    // OBJ indices are 1-based and global across the whole file. Each Vertex already
    // carries its own position, uv and normal, so one index serves v/vt/vn.
    size_t base = 1, totalTris = 0;
    for (const NamedMesh& nm : meshes)
    {
        const MeshData& md = *nm.mesh;
        out << "\no " << nm.name << '\n';
        for (const Vertex& v : md.vertices)
        {
            std::snprintf(buf, sizeof buf, "v %g %g %g\n", v.pos.x, v.pos.y, v.pos.z);
            out << buf;
        }
        for (const Vertex& v : md.vertices)
        {
            std::snprintf(buf, sizeof buf, "vt %g %g\n", v.uv.x, v.uv.y);
            out << buf;
        }
        for (const Vertex& v : md.vertices)
        {
            std::snprintf(buf, sizeof buf, "vn %g %g %g\n", v.normal.x, v.normal.y, v.normal.z);
            out << buf;
        }
        for (size_t i = 0; i + 2 < md.indices.size(); i += 3)
        {
            out << 'f';
            for (int k = 0; k < 3; ++k)
            {
                size_t idx = base + md.indices[i + k];
                out << ' ' << idx << '/' << idx << '/' << idx;
            }
            out << '\n';
        }
        std::cout << "  " << nm.name << ": " << md.vertices.size() << " vertices, "
                  << md.triangleCount() << " triangles\n";
        base += md.vertices.size();
        totalTris += md.triangleCount();
    }

    if (!out)
    {
        std::cerr << "error while writing " << paths[1] << '\n';
        return 2;
    }
    std::cout << "Wrote " << paths[1] << "  (" << L.name << ", " << totalTris << " triangles"
              << (withCeiling ? ", with ceiling" : ", no ceiling") << ")\n";
    return 0;
}
