#ifndef TEMPLE_PLAYER_CONTROLLER_H
#define TEMPLE_PLAYER_CONTROLLER_H

#include "Player.h"

class PlayerController {
public:
    PlayerController(
        Player& player,
        int width,
        int height,
        int step = 20
    );

    bool handleInput(char key);

private:
    Player& player_;

    int width_;
    int height_;
    int step_;
};

#endif
