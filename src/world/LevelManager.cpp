#include "LevelManager.h"
// TODO: Implement LevelManager
namespace world {

LevelManager::LevelManager() {
    // Level(x, y, width, height, wallThickness, tileSize)
    addLevel(Level(50, 50, 700, 500, 30, 40));   // Level 1: main chamber
    addLevel(Level(100, 100, 600, 400, 30, 40)); // Level 2: smaller chamber
}

void LevelManager::addLevel(const Level& level) {
    levels_.push_back(level);
}

bool LevelManager::setCurrent(std::size_t index) {
    if (index >= levels_.size()) return false;
    current_ = index;
    return true;
}

bool LevelManager::next() {
    return setCurrent(current_ + 1);
}

bool LevelManager::previous() {
    if (current_ == 0) return false;
    return setCurrent(current_ - 1);
}

} // namespace world

