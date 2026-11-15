
#include <cassert>
#include <iostream>
#include "gameplay/Door.h"

int main()
{
    Door door;

    // Door starts closed.
    assert(!door.isOpen());

    // Open the door.
    door.open();
    assert(door.isOpen());

    // Close the door.
    door.close();
    assert(!door.isOpen());

    // Reset should restore closed state.
    door.open();
    door.reset();
    assert(!door.isOpen());

    std::cout << "All Door tests passed!\n";
    return 0;
}
