#pragma once
#include <memory>
#include <string>
class Audio {
public:
    Audio();
    ~Audio();
    void music();
    void effect(const std::string& file);
    void toggle();
    bool muted = false;
    bool available() const;
private:
    struct State;
    std::unique_ptr<State> state;
};
