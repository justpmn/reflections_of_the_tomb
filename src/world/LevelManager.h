// Owns all levels, tracks which one is active, and forwards drawing/queries to it.
#pragma once
#include "Level.h"
#include <cstddef>
#include <vector>

namespace world {

class LevelManager {
public:
    LevelManager();                       // builds the default set of levels

    void addLevel(const Level& level);
    bool setCurrent(std::size_t index);   // false if index is out of range
    bool next();                          // false if already on the last level
    bool previous();                      // false if already on the first level

    std::size_t currentIndex() const { return current_; }
    std::size_t levelCount() const { return levels_.size(); }

    const Level& current() const { return levels_[current_]; }
    void draw(const PlotFn& plot) const { current().draw(plot); }

private:
    std::vector<Level> levels_;
    std::size_t current_ = 0;
};

} 

