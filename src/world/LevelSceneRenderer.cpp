#include "LevelSceneRenderer.h"

#include <iostream>

#include "rendering/Model.h"
#include "rendering/Renderer.h"

namespace world {

LevelSceneRenderer::LevelSceneRenderer() = default;

LevelSceneRenderer::~LevelSceneRenderer() { cleanup(); }

bool LevelSceneRenderer::load(const std::string& assetRoot) {
    bool all = true;
    for (const AssetInfo& a : assetTable()) {
        std::size_t i = static_cast<std::size_t>(a.id);
        models_[i] = std::make_unique<Model>();
        loaded_[i] = models_[i]->load(assetRoot + a.path);
        if (!loaded_[i]) {
            std::cerr << "LevelSceneRenderer: failed to load " << a.name << " (" << a.path << ")\n";
            all = false;
        }
    }
    return all;
}

void LevelSceneRenderer::draw(Renderer& renderer, const LevelScene& scene,
                              const glm::mat4& levelToWorld) const {
    for (const Placement& p : scene.placements) {
        std::size_t i = static_cast<std::size_t>(p.asset);
        if (loaded_[i]) {
            renderer.drawModel(*models_[i], levelToWorld * p.transform, colorFor(p.asset));
            continue;
        }
        // Model missing: draw the placement's fallback asset if it has one.
        if (p.fallback == AssetId::Count) continue;
        std::size_t f = static_cast<std::size_t>(p.fallback);
        if (loaded_[f])
            renderer.drawModel(*models_[f], levelToWorld * p.fallbackTransform, colorFor(p.fallback));
    }
}

void LevelSceneRenderer::cleanup() {
    for (std::size_t i = 0; i < kCount; ++i) {
        if (models_[i]) models_[i]->cleanup();
        models_[i].reset();
        loaded_[i] = false;
    }
}

glm::vec3 LevelSceneRenderer::colorFor(AssetId id) {
    switch (id) {
        case AssetId::FloorTile:    return {0.62f, 0.55f, 0.42f};   // sandstone
        case AssetId::Wall:         return {0.45f, 0.40f, 0.33f};
        case AssetId::Pillar:       return {0.70f, 0.64f, 0.52f};
        case AssetId::Doorway:
        case AssetId::TempleDoor:   return {0.50f, 0.33f, 0.18f};
        case AssetId::Mirror:       return {0.80f, 0.82f, 0.88f};
        case AssetId::RubyPedestal: return {0.85f, 0.12f, 0.20f};
        case AssetId::SpikeTrap:    return {0.38f, 0.38f, 0.40f};
        case AssetId::Torch:        return {1.00f, 0.60f, 0.15f};
        case AssetId::Count:        break;
    }
    return {1.0f, 0.0f, 1.0f};
}

} // namespace world
