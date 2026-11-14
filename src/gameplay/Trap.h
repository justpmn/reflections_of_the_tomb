
#pragma once

class Trap
{
public:
    Trap();

    // Extend the spikes.
    void activate();

    // Retract the spikes.
    void deactivate();

    // Return to the initial state.
    void reset();

    // Check whether the trap is dangerous.
    bool isActive() const;

private:
    bool active;
};
