
#pragma once

#include <vector>

#include "beam/BeamSegment.h"
#include "beam/BeamTracer.h"

class PuzzleSystem;

class BeamPuzzleBridge
{
public:
    // Existing segment-based integration.
    static void processPath(
        const std::vector<BeamSegment>& path,
        PuzzleSystem& puzzle
    );

    // New: exact absorbing-ruby event.
    static void processAbsorbingTrace(
        const BeamTraceResult& trace,
        const std::vector<RubyTarget>& rubies,
        PuzzleSystem& puzzle
    );
};
