# Synth Seance Verdict — OBRIX (Ocean Bricks)

**Seance Date**: 2026-03-19
**Engine**: OBRIX — Modular Brick Synthesis Toy Box
**Gallery Code**: OBRIX | Accent: Reef Jade `#1E8B7E` | Prefix: `obrix_`
**Score**: 6.8 / 10
**Source**: `Source/Engines/Obrix/ObrixEngine.h` (831 lines)

---

## The Council Has Spoken

| Ghost | Core Judgment |
|-------|--------------|
| **Bob Moog** | "Good bones — fix the aliasing, close the feedback loop, then we'll talk about presets." |
| **Don Buchla** | "You have called this thing modular. It is not modular. It is a fixed-topology synth with dropdown menus." |
| **Dave Smith** | "getSampleForCoupling returns 0.5 — that is not a stub, that is a lie." |
| **Ikutaro Kakehashi** | "An instrument without presets is an instrument that asks the user to be an engineer before they can be a musician." |
| **Alan Pearlman** | "The init patch is correct — you press a key, you hear a musical sound. This is the most important thing any synthesizer can do." |
| **Isao Tomita** | "OBRIX has no sense of *where*. Pan is not space." |
| **Vangelis** | "There is no pitch bend. This is not a missing feature. This is a missing lung." |
| **Klaus Schulze** | "OBRIX understands the moment but not the journey. Nothing accumulates. Nothing drifts beyond its boundaries." |

---

## Points of Agreement

1. **Naive saw oscillator must be fixed** (Moog, Buchla, Pearlman). PolyBLEP.h included but never called. Above C5, aliasing is audible and unmusical. #1 technical fix.
2. **Zero presets is critical** (Kakehashi, Smith). Only engine in the 39-engine fleet with no factory presets.
3. **Coupling output is broken** (Smith, Schulze). `getSampleForCoupling()` returns constant 0.5f. Must return actual audio.
4. **Declared slots not wired** (Buchla, Pearlman). 4 modulators/3 processors/3 effects declared; 2/2/1 wired. D004 spirit violation.
5. **CytomicSVF is the right filter** (Moog, Pearlman). Correct topology, numerically stable.
6. **CHARACTER macro is well-designed** (Pearlman, Vangelis). Single gesture: filter + wavefolder.

## Points of Contention

### Buchla vs. Kakehashi — "Modular" as lie or teaching tool
- Buchla: Fixed topology with dropdowns is not modular
- Kakehashi: Construction sets teach; OBRIX is democratically designed
- **Resolution**: Lean into "configurable toy box" identity, not "modular"

### Vangelis vs. Schulze — Expression vs. Evolution (DB004)
- Vangelis: Pitch bend + portamento for immediate emotional expression
- Schulze: Drift accumulator for long-form temporal evolution
- **Resolution**: Not mutually exclusive. Pitch bend = P0 fix. Drift = V2 vision.

---

## Blessings & Warnings

| Ghost | Blessing | Warning |
|-------|----------|---------|
| **Moog** | CytomicSVF filter — correct topology, velocity-scaled cutoff | Naive saw aliases above C5 — PolyBLEP included but never called |
| **Buchla** | Coupling input architecture — inter-engine modularity | "Modular" is false — fixed topology with dropdown menus |
| **Smith** | ParamSnapshot discipline — atomic reads, cached per block | Coupling output returns constant 0.5 — breaks ecosystem trust |
| **Kakehashi** | Democratically designed — teaches synthesis through bricks | Zero presets — "a parts kit, not a product" |
| **Pearlman** | Four macros well-conceived — CHARACTER is performable | 3 procs / 4 mods / 3 FX declared, only 2/2/1 wired |
| **Tomita** | Pedagogically beautiful — velocity→filter teaches dynamics | No spatial intention — sounds float in a void |
| **Vangelis** | CHARACTER macro — "how a performer thinks" | No pitch bend — "a missing lung" |
| **Schulze** | 0.01 Hz LFO floor — respects the long breath | No journey — envelopes reset, LFOs cycle, nothing accumulates |

---

## What the Ghosts Would Build Next

| Ghost | Feature |
|-------|---------|
| **Moog** | `obrix_proc1Feedback` — filter feedback with tanh saturation |
| **Buchla** | Source-to-source FM — audio-rate phase modulation |
| **Smith** | Unison/voice stacking with detune spread |
| **Kakehashi** | SURPRISE/MUTATE — constrained randomization |
| **Pearlman** | Wire Processor 3 as post-amp insert |
| **Tomita** | DISTANCE parameter — air absorption + pre-delay |
| **Schulze** | Drift accumulator — irreversible mod offset per cycle |
| **Vangelis** | Pitch bend + portamento with adjustable glide |

---

## Score Breakdown

| Domain | Score | Notes |
|--------|-------|-------|
| Architecture & intent | 8/10 | Strong concept, clear identity |
| DSP quality | 5/10 | CytomicSVF excellent, but naive osc + stub coupling + unwired slots |
| Playability | 5/10 | Good macros, no pitch bend/portamento/unison |
| Preset library | 0/10 | Zero presets |
| Coupling integration | 4/10 | Receives well, sends nothing |
| Code quality | 8/10 | Clean, denormal-safe, well-structured |
| **Overall** | **6.8/10** | Strong bones, shipped scaffolding as finished building |

---

## Priority Action Items

### P0 — Must fix before presets
1. Apply PolyBLEP to saw, square, pulse oscillators (anti-aliasing)
2. Implement pitch bend (standard MIDI, configurable range)
3. Fix `getSampleForCoupling` — return actual last-rendered audio
4. Add portamento/glide for legato mode

### P1 — Should fix for V1 quality
5. Wire mod slots 3 and 4 (fulfill the 4-modulator promise)
6. Wire processor 3 (post-amp insert per Pearlman)
7. Implement unison/voice stacking with detune spread
8. Create 150+ factory presets across 7 moods

### P2 — V2 enhancements (ghost visions)
9. Source-to-source FM (Buchla)
10. Filter feedback with saturation (Moog)
11. DISTANCE spatial parameter (Tomita)
12. Drift accumulator for entropic evolution (Schulze)
13. SURPRISE/MUTATE constrained randomization (Kakehashi)
