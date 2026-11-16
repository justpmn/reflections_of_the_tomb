# Mandatory reflection verification

Runtime uses the continuous finite-segment `BeamTracer`, not a grid solver or a rotation counter. All angles in the supported continuous range [-80, 80] remain available.

## Geometry

- Entrance Hall: the eastbound source misses its ruby unless the east mirror turns it north.
- Hall of Spikes: a stone baffle at column 4, rows 2-3 blocks the diagonal from the northern mirror to the western ruby. Light must turn south at the northern mirror, then west at the southern mirror.
- Inner Sanctum: two independent optical circuits, each with two mirrors. A centre wall at column 7 (rows 1-7) separates the circuits optically; an open lower passage connects player routes. Baffles at columns 3/11, rows 2-3 prevent first-mirror-to-ruby diagonals. Actual reflection order is **0 -> 2 -> ruby 0** for the eastbound left source and **1 -> 3 -> ruby 1** for the westbound right source.

Indices follow loader reading order. The Level 3 four-mirror solution is `[45,-45,-45,45]`, all inside the normal [-80,80] range. Both rubies can remain lit at once. No alternate angle metadata or extra rotation step is needed in Level 3. The optional `alternate` loader metadata remains supported for designer tools; runtime completion never uses it.

## Continuous necessity proof

`tests/mandatory_reflection_test.cpp` checks each mirror removal separately. Every rotating mirror segment is enclosed by a square centred at the authored mirror position, with half-width equal to half the physical segment length. Ruby target spheres are conservatively enclosed by squares of radius 0.38 on the optical plane.

The test merges collinear wall faces. For each pair of envelopes, it checks whether a wall plane separates them, then bounds every possible crossing coordinate by evaluating the endpoint rectangles' corners. The crossing expression is linear-fractional with a denominator of fixed sign; its extrema occur at endpoints. A face spanning that complete interval, with a strict interior margin, blocks every segment between the two envelopes. This includes fractional angles, endpoint reflections and grazing configurations away from the retained strict margin.

The resulting visibility graph deliberately allows more connections than real optics: every unproven connection is retained, and each mirror may redirect light arbitrarily to any visible envelope. Initial graph connections conservatively include every envelope the fixed source ray might reach before a wall. Ruby nodes absorb and cannot reflect. Removing a mirror disconnects at least one door-required ruby in this over-approximation for every chamber. Therefore no real mirror-angle configuration, repeated-bounce path or sequence of permanent seals can complete the chamber without that mirror. Blocking a disabled mirror instead of removing it cannot create any new ray path.

## Continuous BeamTracer checks

- Verify the authored starting pose is unsolved.
- Trace the intended solution sequence and verify every required ruby is hit and every intended mirror appears in `reflectedMirrorIndices`.
- Remove or absorbingly disable each mirror and enumerate integer-degree states over [-80,80]. Ruby hits are unioned across states, explicitly accounting for permanent sealing.
- The same conservative visibility graph factors optically disconnected mirror components. Rays cannot bounce between components even at fractional angles. Searching each complete component with other components removed over-approximates possible hits. This covers their Cartesian product without brute-forcing irrelevant cross-circuit combinations.
- Counts per removal/disable: 1 direct case in level 1; 162 cases in level 2 (161 single-mirror angles plus a direct case); 26,083 cases in level 3 (25,921 two-mirror circuit poses, 161 single-mirror poses and a direct case). Every case is traced for both sources where present and for transparent removal and opaque disabling. The all-real-angle envelope proof remains the primary necessity proof.
- Calibration removes the level 2 baffle and finds a physical diagonal bypass; replacing the baffle blocks that exact beam. This proves the test is exercising meaningful geometry rather than an interaction counter.
- `level3_paths` verifies each source's exact two finite reflections and target, rejects paths crossing solid geometry, checks reflection normals and tests all 16 +/-0.2 degree joint-error combinations. Both circuits work throughout that precision margin.
- `campaign` walks collision-aware routes and uses proximity controls through all five exits. `native_control_campaign` additionally sends real window messages through GLFW's key handling: Enter, Escape, WASD, Q/E, Shift and R. It reaches both final rubies at actual angles near +/-44.92 degrees, resets after the first seal, solves again and reaches victory without direct state assignment.

## Five-level expansion

Levels 1-3 data and BeamTracer are unchanged. The Gauntlet has two eastbound circuits with intended reflection chains `[0,2]` and `[1,3]`. The Final Trial has three eastbound circuits with chains `[0,3]`, `[1,4]`, `[2,5]`. Upper mirrors are +45 degrees and lower mirrors -45 degrees; every circuit has its own required ruby. Baffles block each upper-mirror diagonal to its ruby; full-height optical dividers prevent inter-bay shortcuts.

The existing conservative continuous-angle envelope proof now runs on all five levels and succeeds for every individual mirror removal. Real BeamTracer integer-angle searches also cover every removal and opaque disable: 26,083 configurations per mirror in The Gauntlet and 52,004 per mirror in The Final Trial (two complete two-mirror components, one single mirror and a direct case). Unioning ruby hits allows arbitrary permanent-sealing sequences; the missing mirror still disconnects a required ruby.

The full campaign test now walks every new mirror and all five exits using real collision and proximity rotation. Two additional runs shift all timed traps by 7.25 and 19.1 seconds before walking, checking that safe routes are not an initial-phase coincidence. The native GLFW replay aligns all 17 campaign mirrors using WASD/Q/E/Shift, waits outside spike plates, crosses in safe windows, checks paused trap phases in The Gauntlet, and finishes with zero respawns. It still tests R reset after the first Level 3 seal. Fixture-assigned state is confined to headless unit tests and smoke previews.
