
#include "Trap.h"
#include <algorithm>

Trap::Trap()
    : active(true)
{
}

void Trap::activate()
{
    active = true;
}

void Trap::deactivate()
{
    active = false;
}

void Trap::reset()
{
    active = true;
    extensionAmount = 1;
}

bool Trap::isActive() const
{
    return active;
}
void Trap::update(float seconds)
{
    extensionAmount=std::clamp(extensionAmount+(active?1.f:-1.f)*std::max(0.f,seconds)*2.f,0.f,1.f);
}
