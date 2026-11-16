
#include "Ruby.h"
#include <algorithm>

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
    sealing = 0;
}

bool Ruby::isActivated() const
{
    return activated;
}

bool Ruby::isRealRuby() const
{
    return real;
}
void Ruby::update(float seconds)
{
    if(activated) sealing=std::min(1.f,sealing+std::max(0.f,seconds)*1.8f);
}
