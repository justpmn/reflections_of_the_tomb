// Prints a level from above with the SOLUTION beam drawn on it.
// Handy for checking a new level and for screenshots in the report.
//   ./build/level_preview assets/levels/level03.txt
// Exit code: 0 = solvable, 1 = load error, 2 = not solvable.
#include <iostream>
#include "world/Level.h"

using namespace world;

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: level_preview <level.txt>\n"; return 1; }
    LoadResult r = loadLevelFromFile(argv[1]);
    if (!r.ok) {
        for (auto& e : r.errors) std::cerr << e << "\n";
        return 1;
    }
    const Level& L = r.level;

    std::vector<std::string> pic(L.height, std::string(L.width, ' '));
    for (int y = 0; y < L.height; ++y)
        for (int x = 0; x < L.width; ++x) pic[y][x] = (char)L.tileAt({x, y});

    for (int i = 0; i < (int)L.lights.size(); ++i)
        for (auto c : traceBeamOnGrid(L, i, true).cells) pic[c.y][c.x] = '*';

    pic[L.playerSpawn.y][L.playerSpawn.x] = 'S';
    for (auto& t : L.traps)  pic[t.cell.y][t.cell.x] = 'T';
    for (auto& l : L.lights) pic[l.cell.y][l.cell.x] = 'L';
    for (auto& d : L.doors)  pic[d.cell.y][d.cell.x] = 'D';
    for (auto& m : L.mirrors)
        pic[m.cell.y][m.cell.x] = !m.hasSolution          ? 'M'
                                : m.solutionDeg == 45.0f  ? '\\'   // team convention: 45 = "\"
                                : m.solutionDeg == -45.0f ? '/'
                                                          : 'M';
    for (auto& ru : L.rubies) pic[ru.cell.y][ru.cell.x] = 'R';

    std::cout << L.name << "  (solution beam = *)\n";
    for (auto& row : pic) std::cout << "  " << row << "\n";
    std::vector<std::string> why;
    bool solvable = isSolvable(L, &why);
    std::cout << (solvable ? "  SOLVABLE\n" : "  NOT SOLVABLE\n");
    for (auto& w : why) std::cout << "  - " << w << "\n";
    return solvable ? 0 : 2;
}
