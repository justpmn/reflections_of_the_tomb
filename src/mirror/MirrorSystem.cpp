
#include "MirrorSystem.h"

#include <stdexcept>

std::size_t MirrorSystem::addMirror(const Mirror& mirror) {
    mirrors.push_back(mirror);
    return mirrors.size() - 1;
}

Mirror& MirrorSystem::getMirror(std::size_t index) {
    if (index >= mirrors.size()) {
        throw std::out_of_range("Mirror index out of range");
    }

    return mirrors[index];
}

const Mirror& MirrorSystem::getMirror(std::size_t index) const {
    if (index >= mirrors.size()) {
        throw std::out_of_range("Mirror index out of range");
    }

    return mirrors[index];
}

void MirrorSystem::selectMirror(std::size_t index) {
    if (index >= mirrors.size()) {
        throw std::out_of_range("Mirror index out of range");
    }

    selectedMirrorIndex = index;
}

std::size_t MirrorSystem::getSelectedMirrorIndex() const {
    return selectedMirrorIndex;
}

void MirrorSystem::rotateSelectedLeft(float degrees) {
    if (mirrors.empty()) {
        return;
    }

    mirrors[selectedMirrorIndex].rotateLeft(degrees);
}

void MirrorSystem::rotateSelectedRight(float degrees) {
    if (mirrors.empty()) {
        return;
    }

    mirrors[selectedMirrorIndex].rotateRight(degrees);
}

std::size_t MirrorSystem::getMirrorCount() const {
    return mirrors.size();
}
