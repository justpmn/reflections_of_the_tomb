#pragma once
#include "world/LevelManager.h"
#include "beam/BeamTracer.h"
#include "player/PlayerMovement3D.h"
#include "gameplay/Ruby.h"
#include "gameplay/Door.h"
#include "gameplay/Trap.h"

// Rendering-independent campaign state. Uses the team's continuous BeamTracer,
// movement, Ruby, Door and Trap implementations, never the grid design checker.
class GameSession {
public:
    explicit GameSession(const std::string& levelDirectory);
    bool restart();
    bool reset();
    void update(float x, float z, float dt);
    enum class Phase { Countdown, Playing, FadeOut, Title, FadeIn, Victory };
    enum class CharacterState { Alive, Dying, Respawning, Invulnerable };
    Phase phase = Phase::Countdown;
    CharacterState characterState = CharacterState::Alive;
    float phaseTime=0, characterTime=0;
    bool fellInPit=false, paused=false;
    unsigned int entrySerial=0;
    bool controlsEnabled() const;
    float characterOpacity() const;
    std::string countdownLabel() const;
    void trace();
    int nearbyMirror() const;
    bool rotate(float degrees);
    bool advance();
    bool solved() const;
    bool victory = false;
    int deaths = 0;
    world::LevelManager levels;
    player3d::PlayerMovement3D player;
    std::vector<Mirror> mirrors;
    std::vector<Ruby> rubies;
    std::vector<Door> doors;
    std::vector<Trap> traps;
    std::vector<BeamSegment> beams;
    std::vector<WallSegment> walls;
    std::vector<player3d::ObstacleXZ> obstacles;
    float doorLift = 0;
private:
    bool initializeLevel(bool countdown=false);
};
