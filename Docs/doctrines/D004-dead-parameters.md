# D004 — Dead Parameters Are Broken Promises

**Status:** RESOLVED — all declared parameters wired (Round 3B + multiple Phantom Sniff passes)

## Definition

Every parameter that appears in the UI or in the parameter list must affect the audio output. A knob that does nothing is a lie to the performer. A parameter ID that exists but produces no audible change is a broken promise.

## Requirements

- Every `AudioParameterFloat`, `AudioParameterInt`, `AudioParameterBool`, and `AudioParameterChoice` must produce a measurable change in audio output when swept from min to max
- Parameters may have subtle effects (e.g., a fine-tune parameter) but must be audible
- Parameters used for display/readout only (meters, visualizers) must be clearly documented as such
- Bypass parameters are acceptable — their effect is switching between two audible states

## Common Failure Patterns

- Parameter declared in `createParameterLayout()` but never read in `renderBlock()`
- Parameter read but result discarded: `float val = param->load(); // unused`
- Parameter used to set a variable that is never applied: `cutoffMod_ = val; // cutoffMod_ never applied to filter`
- Coupling output parameters that are populated but never routed to `outputCacheL/R`

## Detection Method (Phantom Sniff)

The Phantom Sniff technique: for each parameter, set to min, render 1024 samples, set to max, render 1024 samples, compare. If outputs are identical, the parameter is dead.

Automated detection: `Tools/phantom_sniff.py` (if available) or manual audit per engine.

## Test Criteria

1. For each parameter in the engine:
   a. Set to minimum value
   b. Render audio
   c. Set to maximum value
   d. Render audio
   e. Compare — if identical, D004 is failed
2. Exception: parameters that are conditional on other parameters being in a specific state (e.g., an FM ratio that only matters when FM depth > 0)

## Fleet Compliance

All 74 registered engines — COMPLIANT as of the most recent Phantom Sniff pass (2026-03-29).
24 dead wires fixed in the most recent pass across 11 engines.
