#pragma once
// Draws a LevelScene with the team's Model class and Renderer::drawModel.
// Owner: Zahra Isaiho. Each asset in assetTable() is loaded once and drawn for
// every placement that uses it. Needs a current OpenGL context.

#include <array>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "LevelScene.h"

class Model;
class Renderer;

namespace world {

class LevelSceneRenderer {
public:
    LevelSceneRenderer();
    ~LevelSceneRenderer();

    LevelSceneRenderer(const LevelSceneRenderer&) = delete;
    LevelSceneRenderer& operator=(const LevelSceneRenderer&) = delete;

    // Loads every asset in assetTable(). assetRoot is prepended to each path
    // ("" = the working directory, i.e. the repo root when running the game).
    // Returns false if any asset failed; the ones that loaded are still drawn.
    bool load(const std::string& assetRoot = "");

    // levelToWorld places the whole level in the game world (see levelToArea()).
    void draw(Renderer& renderer, const LevelScene& scene,
              const glm::mat4& levelToWorld = glm::mat4(1.0f)) const;

    void cleanup();

    // Flat colour per asset (main's Model has no textures; Joe's uses it as a tint).
    static glm::vec3 colorFor(AssetId id);

private:
    static constexpr std::size_t kCount = static_cast<std::size_t>(AssetId::Count);
    std::array<std::unique_ptr<Model>, kCount> models_;
    std::array<bool, kCount> loaded_{};
};

} // namespace world
