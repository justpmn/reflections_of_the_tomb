# Campaign crash investigation

## Evidence and scope

The Release executable at `9e002f8` completed 30 consecutive campaign runs with exit code 0. A symbolized build also completed under Android Studio's installed LLDB DAP debugger with exit code 0. No matching `campaign_test.exe` Application Error event was returned by the targeted Windows event-log query. Consequently, a new spontaneous crash in that committed build has not been reproduced, and the origin/build date of the reported popup is not established.

The previous integration work left a diagnostic source-repair script (`build-desktop/fix_activation.py`) showing an earlier initialization-order fault: the parallel `rubySealing` vector had initially been sized before the ruby objects were populated. That produces an empty animation vector even though a real ruby exists. The ordering had already been corrected before `9e002f8`, explaining why its campaign tests pass. The architecture still permitted the invalid state, however.

A controlled reconstruction released the legacy animation vector, activated ruby 0 and ran the normal `GameSession::update` function. LLDB captured **0xC0000005: Access violation reading location 0x00000000** at **GameSession.cpp:115**, in the first activated-ruby animation update. Stack:

```text
GameSession::update(float,float,float)  GameSession.cpp:115
main                                  repro_empty_ruby.cpp:10
__tmainCRTStartup
BaseThreadInitThunk
RtlUserThreadStart
```

This is an execution-time read, before shutdown. The capture is [recorded here](diagnostics/legacy-animation-access-violation.json), with the [original source excerpt](diagnostics/legacy-animation-source.txt). A separate unoptimized GCC 16 build stopped earlier at the vector subscript assertion (0xC0000409); the optimized symbolized build exposed the actual null read matching the reported exception type. No Windows error-dialog policy was changed.

This reconstruction confirms the earlier defect's failure mechanism; it is **not** evidence that the unchanged current campaign spontaneously reproduces it. No crash dump from the user's particular popup was available.

## Repair

Ruby owns its activation and sealing progress together. Trap owns its active state and extension together. The two parallel animation arrays were removed from GameSession, rendering and tests. Construction, reset, vector growth and destruction cannot separate an entity from its animation state. This removes the invalid-state representation rather than adding null checks at the failing access.

GameApplication now owns GLFW lifetime and its window with scope-based destructors. All scene/model/shader/UI/audio owners unwind inside `play` while the window/context is alive, including exceptions. Scroll callback bindings detach before their stack-owned zoom value expires. The window is then destroyed, followed by GLFW termination. The campaign binary itself does not link rendering or audio, ruling those components out of the reconstructed campaign fault.

## Regression checks

- `session_lifecycle`: 512 Ruby/Trap vector reallocations and first activation ticks; 200 level changes/resets across differing entity counts; activation, animation, restart and destruction checks.
- `campaign`: full movement/proximity-control progression, doors, reset and hazards retained.
- `application_lifecycle`: five real GL/audio/UI startup/shutdown subprocesses, plus an injected C++ exception after initialization. Expected exit is 10 for that diagnostic exception, rather than an access violation; normal runs exit 0.
- `tools/DebugCampaign.py`: debugger adapter client, source/stack capture and clean-exit verification, with full local transcripts in `build-stability`.

Final repeated-run and debugger results are recorded in INTEGRATION_STATUS.md after the complete build.
