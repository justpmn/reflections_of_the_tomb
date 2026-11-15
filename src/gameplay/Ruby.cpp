
#include "Ruby.h"

Ruby::Ruby(bool isReal)
    : real(isReal), activated(false)
{
}

void Ruby::hitByLight()
{
    if (real)
    {
        activated = true;
    }
}

void Ruby::reset()
{
    activated = false;
}

bool Ruby::isActivated() const
{
    return activated;
}

bool Ruby::isRealRuby() const
{
    return real;
}
