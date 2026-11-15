#include "LevelManager.h"

namespace world {

bool LevelManager::loadIndex(int i) {
    errors_.clear();
    if (i < 0 || i >= count()) {
        errors_.push_back("level index " + std::to_string(i) + " out of range");
        return false;
    }
    LoadResult r = loadLevelFromFile(paths_[i]);
    if (!r.ok) {
        errors_ = r.errors;
        return false;           // keep the previous level loaded
    }
    current_ = std::move(r.level);
    index_ = i;
    loaded_ = true;
    return true;
}

bool LevelManager::next() {
    if (isLastLevel()) return false;
    return loadIndex(index_ + 1);
}

} // namespace world
