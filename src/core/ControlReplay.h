#pragma once
#include "GameApplication.h"
#include <memory>
#include <string>
struct GLFWwindow;
class GameSession;
// Diagnostic-only native window input replay. It observes gameplay but never
// writes player positions, mirror angles, ruby activation or completion state.
class ControlReplay {
public:
    ControlReplay();
    ~ControlReplay();
    void beforePoll(GLFWwindow*,const GameSession&,GameScreen);
    void afterInput(GLFWwindow*,const GameSession&,GameScreen);
    bool shouldRender() const;
    bool finished() const;
    std::string takeCapture();
private:
    struct State;
    std::unique_ptr<State> state;
};
