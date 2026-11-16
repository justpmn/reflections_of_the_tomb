# Mandatory reflection verification

Runtime uses the continuous finite-segment `BeamTracer`, not a grid solver or a rotation counter. All angles in the supported continuous range [-80, 80] remain available.

## Geometry

- Entrance Hall: the eastbound source misses its ruby unless the east mirror turns it north.
- Hall of Spikes: a stone baffle at column 4, rows 2-3 blocks the diagonal from the northern mirror to the western ruby. Light must turn south at the northern mirror, then west at the southern mirror.
- Inner Sanctum: two stone baffles at column 6, rows 2-4 and 6-8 block diagonal shortcuts. Light travels from the north mirror south to the east mirror, then west to the west mirror. The west mirror first sends it north to ruby 0, then south to ruby 1. Permanent sealing is essential to this sequence.

Indices follow the loader's reading order. The third chamber's reflection order is **0 -> 2 -> 1**. Its solution angles are `[45,45,-45]`, followed by `[45,-45,-45]`.

`alternate` in a level file is optional designer verification metadata for a sealing sequence. It is never read by gameplay interaction or completion logic. The design checker unions ruby hits over designer poses; runtime completion always comes from actual absorbing traces.

## Continuous necessity proof

`tests/mandatory_reflection_test.cpp` checks each mirror removal separately. Every rotating mirror segment is enclosed by a square centred at the authored mirror position, with half-width equal to half the physical segment length. Ruby target spheres are conservatively enclosed by squares of radius 0.38 on the optical plane.

The test merges collinear wall faces. For each pair of envelopes, it checks whether a wall plane separates them, then bounds every possible crossing coordinate by evaluating the endpoint rectangles' corners. The crossing expression is linear-fractional with a denominator of fixed sign; its extrema occur at endpoints. A face spanning that complete interval, with a strict interior margin, blocks every segment between the two envelopes. This includes fractional angles, endpoint reflections and grazing configurations away from the retained strict margin.

The resulting visibility graph deliberately allows more connections than real optics: every unproven connection is retained, and each mirror may redirect light arbitrarily to any visible envelope. Initial graph connections conservatively include every envelope the fixed source ray might reach before a wall. Ruby nodes absorb and cannot reflect. Removing a mirror disconnects at least one door-required ruby in this over-approximation for every chamber. Therefore no real mirror-angle configuration, repeated-bounce path or sequence of permanent seals can complete the chamber without that mirror. Blocking a disabled mirror instead of removing it cannot create any new ray path.

## Continuous BeamTracer checks

- Verify the authored starting pose is unsolved.
- Trace the intended solution sequence and verify every required ruby is hit and every intended mirror appears in `reflectedMirrorIndices`.
- Remove each mirror, enumerate every integer-degree combination of all remaining mirrors over [-80,80], and union ruby hits across all configurations. This deliberately accounts for sealing targets at different times.
- Counts: 1 configuration for the one-mirror removal; 161 for each two-mirror removal; 25,921 for each three-mirror removal. The interval visibility proof covers the fractional angles between these samples; the sample search alone is not claimed as a continuous proof.
- The campaign test walks actual collision-aware routes to every mirror, solves through the proximity-gated rotation API, returns to the west mirror for the second seal, and traverses all three exits.
