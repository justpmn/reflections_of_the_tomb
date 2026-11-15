#pragma once
// Loads levels in order and moves to the next one when the exit door is used.
// Owner: Zahra Isaiho.

#include <string>
#include <vector>
#include "Level.h"

namespace world {

class LevelManager {
public:
    explicit LevelManager(std::vector<std::string> levelPaths)
        : paths_(std::move(levelPaths)) {}

    bool loadIndex(int i);                 // false + errors() on failure
    bool loadFirst() { return loadIndex(0); }
    bool next();                           // false when there are no more levels
    bool reload() { return loadIndex(index_); }   // restart after a trap

    bool hasLevel() const { return loaded_; }
    const Level& current() const { return current_; }
    int  index() const { return index_; }
    int  count() const { return (int)paths_.size(); }
    bool isLastLevel() const { return index_ == count() - 1; }
    const std::vector<std::string>& errors() const { return errors_; }

private:
    std::vector<std::string> paths_;
    Level current_;
    int index_ = -1;
    bool loaded_ = false;
    std::vector<std::string> errors_;
};

} // namespace world
