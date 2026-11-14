
#include "Trap.h"

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
}

bool Trap::isActive() const
{
    return active;
}
