# D002 — Modulation Is the Lifeblood

**Status:** RESOLVED — all fleet engines compliant (Round 12C mod wheel completion)

## Definition

Modulation is what separates a living instrument from a static snapshot. Every engine must provide a minimum modulation infrastructure: two LFOs, mod wheel routing, aftertouch routing, four working macros, and at least four mod matrix slots.

## Requirements

- Minimum 2 LFOs with independent rate and depth controls
- LFO rate floor ≤ 0.01 Hz (see also D005 — Engines Must Breathe)
- Mod wheel (CC1) must route to at least one audible timbral parameter
- Aftertouch must route to at least one audible timbral parameter (exceptions: visual/utility engines like OPTIC)
- All 4 macros (M1–M4 / CHARACTER, MOVEMENT, COUPLING, SPACE) must produce audible change
- At least 4 mod matrix slots (source → destination)

## Common Failure Patterns

- LFO declared but `lfoDepth` parameter not wired to any oscillator or filter parameter
- Mod wheel CC1 handler reads the value but does not apply it to DSP
- Macros mapped to parameters that have no audible effect (e.g., mapping to a bypass flag that is always false)
- Single LFO with no second modulator

## Test Criteria

1. Set LFO 1 rate to 1 Hz, depth to max — verify audible modulation
2. Set LFO 2 to 0.5 Hz, different target — verify independent modulation
3. Move mod wheel from 0 to 127 — verify audible timbral change (not just volume)
4. Apply aftertouch — verify audible response
5. Move each macro from min to max — verify audible change for all 4

## Fleet Compliance

All 74 registered engines — COMPLIANT.
- Mod wheel: 22/22 MIDI-capable engines (Round 12C)
- Aftertouch: 23/23 engines (OPTIC intentionally exempt — visual engine)
- Macros: all engines verified during Prism Sweep
