
#include "BeamPuzzleBridge.h"

#include "PuzzleSystem.h"

void BeamPuzzleBridge::processPath(
    const std::vector<BeamSegment>& path,
    PuzzleSystem& puzzle)
{
    for (const BeamSegment& segment : path)
    {
        if (puzzle.isSolved())
            break;

        puzzle.processBeam(segment);
    }
}

void BeamPuzzleBridge::processAbsorbingTrace(
    const BeamTraceResult& trace,
    const std::vector<RubyTarget>& rubies,
    PuzzleSystem& puzzle)
{
    if (trace.hitRubyIndex < 0)
        return;

    const auto index =
        static_cast<std::size_t>(trace.hitRubyIndex);

    if (index >= rubies.size())
        return;

    // Let Jesse's system decide what the hit means.
    puzzle.hitRuby(rubies[index].isReal);
}
