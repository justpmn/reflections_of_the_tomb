
#include <cassert>
#include <iostream>
#include "gameplay/Trap.h"

int main()
{
    Trap trap;

    // Traps should start active.
    assert(trap.isActive());

    // Solving the puzzle deactivates the trap.
    trap.deactivate();
    assert(!trap.isActive());

    // Reactivate the trap.
    trap.activate();
    assert(trap.isActive());

    // Reset restores the initial state.
    trap.deactivate();
    trap.reset();
    assert(trap.isActive());

    std::cout << "All Trap tests passed!\n";
    return 0;
}
