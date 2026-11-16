
#pragma once

class Ruby
{
public:
    explicit Ruby(bool isReal);

    // Called when a light beam hits this ruby.
    void hitByLight();

    // Reset the ruby to its initial state.
    void reset();

    bool isActivated() const;
    bool isRealRuby() const;
    void update(float seconds);
    float sealingProgress() const { return sealing; }

private:
    bool real;
    bool activated;
    float sealing = 0;
};
