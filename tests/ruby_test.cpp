
#include <cassert>
#include <iostream>
#include "gameplay/Ruby.h"

int main()
{
    Ruby realRuby(true);
    Ruby falseRuby(false);

    // Neither ruby should start activated.
    assert(!realRuby.isActivated());
    assert(!falseRuby.isActivated());

    // Simulate a light beam hitting both rubies.
    realRuby.hitByLight();
    falseRuby.hitByLight();

    // Only the real ruby should activate.
    assert(realRuby.isActivated());
    assert(!falseRuby.isActivated());

    // Reset the real ruby.
    realRuby.reset();
    assert(!realRuby.isActivated());

    std::cout << "All Ruby tests passed!\n";
    return 0;
}
