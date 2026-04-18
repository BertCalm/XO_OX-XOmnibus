---
name: Engine Fix
about: Report a doctrine violation, seance regression, or DSP correctness issue in a specific engine
title: "[ENGINE FIX] <ENGINE_NAME>: "
labels: engine-fix
assignees: ''
---

## Engine Name
<!-- e.g., OPAL, ONSET, OUROBOROS -->

## Current Seance Score
<!-- e.g., 8.2/10 — or "not yet seanced" -->

## Doctrine Violated
<!-- Check all that apply -->
- [ ] D001 — Velocity Must Shape Timbre
- [ ] D002 — Modulation is the Lifeblood (LFOs, macros, mod wheel, aftertouch)
- [ ] D003 — The Physics IS the Synthesis (citation required for physical models)
- [ ] D004 — Dead Parameters Are Broken Promises
- [ ] D005 — An Engine That Cannot Breathe Is a Photograph (LFO rate floor ≤ 0.01 Hz)
- [ ] D006 — Expression Input Is Not Optional
- [ ] No doctrine — other DSP / audio-correctness issue (describe below)

## Description
<!-- What is wrong? Be specific — which parameter, which behaviour, which preset. -->

## File / Line (if known)
<!-- e.g., Source/Engines/Opal/OpalEngine.h:342 -->

## Steps to Reproduce
1.
2.
3.

## Expected Behavior
<!-- What should happen per the engine's design intent or seance verdict -->

## Actual Behavior
<!-- What currently happens -->

## Proposed Fix
<!-- Optional but encouraged — preferred approach, gotchas to watch for -->

## Impact
<!-- Does this affect any coupling routes? Any preset families broken? How urgent is the fix? -->

## DAW / Environment
- **OS**: <!-- e.g., macOS 15.1 -->
- **Plugin format**: <!-- AU / Standalone / AUv3 -->
- **Sample rate**: <!-- e.g., 48kHz -->
- **XOceanus version / commit**: <!-- e.g., main @ ea08382 -->
