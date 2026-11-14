
#include "PuzzleSystem.h"
#include "RubyCollision.h"

PuzzleSystem::PuzzleSystem()
    : realRuby(true),
      falseRuby(false),
      trap(),
      door(),
      realRubyPosition(0.0f),
      falseRubyPosition(0.0f),
      rubyRadius(0.0f),
      solved(false)
{
}

void PuzzleSystem::hitRuby(bool isRealRuby)
{
    if (solved)
    {
        return;
    }

    if (isRealRuby)
    {
        realRuby.hitByLight();

        if (realRuby.isActivated())
        {
            solved = true;
            trap.deactivate();
            door.open();
        }
    }
    else
    {
        falseRuby.hitByLight();
    }
}

void PuzzleSystem::processBeam(const BeamSegment& beam)
{
    if (solved)
    {
        return;
    }

    if (RubyCollision::intersects(
            beam, realRubyPosition, rubyRadius))
    {
        hitRuby(true);
    }
    else if (RubyCollision::intersects(
                 beam, falseRubyPosition, rubyRadius))
    {
        hitRuby(false);
    }
}

void PuzzleSystem::setRubyPositions(
    const glm::vec3& realPosition,
    const glm::vec3& falsePosition,
    float radius)
{
    realRubyPosition = realPosition;
    falseRubyPosition = falsePosition;
    rubyRadius = radius;
}

void PuzzleSystem::reset()
{
    realRuby.reset();
    falseRuby.reset();
    trap.reset();
    door.reset();
    solved = false;
}

bool PuzzleSystem::isSolved() const
{
    return solved;
}

bool PuzzleSystem::isDoorOpen() const
{
    return door.isOpen();
}

bool PuzzleSystem::areTrapsActive() const
{
    return trap.isActive();
}
