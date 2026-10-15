
#pragma once

#include "Mirror.h"
#include <vector>
#include <cstddef>

class MirrorSystem {
public:
    // Add a mirror and return its index
    std::size_t addMirror(const Mirror& mirror);

    // Access mirrors
    Mirror& getMirror(std::size_t index);
    const Mirror& getMirror(std::size_t index) const;

    // Select which mirror receives rotation input
    void selectMirror(std::size_t index);
    std::size_t getSelectedMirrorIndex() const;

    // Rotate the selected mirror
    void rotateSelectedLeft(float degrees = 5.0f);
    void rotateSelectedRight(float degrees = 5.0f);

    std::size_t getMirrorCount() const;

private:
    std::vector<Mirror> mirrors;
    std::size_t selectedMirrorIndex = 0;
};
