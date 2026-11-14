
#include "Door.h"

Door::Door()
    : opened(false)
{
}

void Door::open()
{
    opened = true;
}

void Door::close()
{
    opened = false;
}

void Door::reset()
{
    opened = false;
}

bool Door::isOpen() const
{
    return opened;
}
