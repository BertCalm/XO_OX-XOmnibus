## Summary

<!-- What does this PR change and why? 1-3 sentences. -->

## Engines Affected

<!-- List engines touched by this change, or "none" if infrastructure/docs only. -->

## Testing Performed

- [ ] `cmake --build build` passes with no errors
- [ ] `auval -v aumu Xolk XoOx` passes (required if audio code was touched)
- [ ] Preset smoke test — affected engine presets load and produce sound
- [ ] No regressions in coupling routes involving affected engines

## Checklist

- [ ] No dead parameters introduced (D004 — every declared param affects audio output)
- [ ] No memory allocations on the audio thread
- [ ] No blocking I/O on the audio thread
- [ ] Parameter IDs are unchanged (frozen after first release)
- [ ] If a new engine was added: CLAUDE.md checklist completed (4 sections updated)
- [ ] If presets were added/changed: `.xometa` format correct, 6D DNA filled, macros produce audible change
- [ ] If coupling was touched: routes don't propagate back to MIDI layer (B016)

## Notes for Reviewer

<!-- Anything unusual, tradeoffs made, or follow-up work deferred. -->
