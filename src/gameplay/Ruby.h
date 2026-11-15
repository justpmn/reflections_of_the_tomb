
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

private:
    bool real;
    bool activated;
};
