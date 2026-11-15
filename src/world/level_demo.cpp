// level_demo.cpp - test program for Level + LevelManager (not part of the final game).
// Writes level1.ppm and level2.ppm so you can check each chamber on its own.
#include "LevelManager.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void render(const world::LevelManager& mgr, const std::string& file) {
    const int W = 800, H = 600;
    std::vector<world::Color> img(W * H, world::Color{20, 18, 16});
    auto plot = [&](int x, int y, world::Color c) {
        if (x >= 0 && y >= 0 && x < W && y < H) img[y * W + x] = c;
    };
    mgr.draw(plot);

    std::ofstream f(file, std::ios::binary);
    f << "P6\n" << W << " " << H << "\n255\n";
    for (const auto& c : img) f.write(reinterpret_cast<const char*>(&c), 3);
    std::cout << "Wrote " << file << " (level " << mgr.currentIndex() + 1 << ")\n";
}

int main() {
    world::LevelManager mgr;
    std::cout << "Levels: " << mgr.levelCount() << "\n";

    render(mgr, "level1.ppm");
    std::cout << "Walkable (400,300): " << mgr.current().isWalkable(400, 300)
              << "  Walkable (60,60): " << mgr.current().isWalkable(60, 60) << "\n";

    if (mgr.next()) render(mgr, "level2.ppm");
    std::cout << "next() again: " << (mgr.next() ? "moved" : "no more levels") << "\n";
    return 0;
}
