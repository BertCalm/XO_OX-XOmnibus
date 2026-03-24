# D006 Mod Wheel Completion — Round 12C

**Date:** 2026-03-14
**Round:** 12C (Prism Sweep)
**Status:** COMPLETE — BUILD PASS

---

## Summary

Round 12C wired mod wheel (CC#1) to the 7 remaining MIDI-capable engines that lacked it:
Bob, Bite, Dub, Oceanic, Ocelot, Overworld, and Osprey. This completes **22/22** MIDI-capable
engines for mod wheel coverage — D006 ("Expression Input Is Not Optional") is now fully resolved
across the entire fleet.

---

## Engines Wired

### 1. Bob (Oblong) — `Source/Engines/Bob/BobEngine.h`

- **Target:** `bob_fltCutoff` — filter cutoff boost
- **Sensitivity:** `+0–4000 Hz` (full wheel opens filter from base cutoff to +4000 Hz)
- **DSP application:** Added to `cutoffMod` before the final `clamp(20–18000 Hz)` in the per-voice render loop
- **Member:** `float modWheelAmount = 0.0f;` added next to `PolyAftertouch aftertouch`
- **Musical rationale:** Mod wheel as a real-time filter swell — the classic synth wah/opens-up gesture. Fully complementary to aftertouch (which adds warmth/character, while mod wheel opens the cutoff frequency).

### 2. Bite (Overbite) — `Source/Engines/Bite/BiteEngine.h`

- **Target:** `poss_macroBite` (BITE macro depth boost)
- **Sensitivity:** `+0–0.4` (full wheel adds 0.4 to BITE macro, clamped at 1.0)
- **DSP application:** Added into `effectiveBite` computation alongside `atPressure * 0.3f`
- **Member:** `float modWheelAmount = 0.0f;` added next to `PolyAftertouch aftertouch`
- **Musical rationale:** Mod wheel gradually dials in the feral, aggressive bite edge. Combined with aftertouch (which also adds 0.3 to BITE), a performer can layer both for maximum aggression, or use wheel alone for a deliberate build.

### 3. Dub (Overdub) — `Source/Engines/Dub/DubEngine.h`

- **Target:** `dub_sendLevel` — send VCA amount (signal into tape delay / spring reverb chain)
- **Sensitivity:** `+0–0.35` (full wheel adds 0.35 to send level, capped at 1.0)
- **DSP application:** Added to `effectiveSendLvl` alongside `atPressure * 0.3f`
- **Member:** `float modWheelAmount = 0.0f;` added next to `PolyAftertouch aftertouch`
- **Musical rationale:** Dub technique: performer rides the send into the echo returns. Mod wheel becomes the "echo throw" — raising it pushes more signal through the tape delay and spring reverb, creating the characteristic dub swell effect.

### 4. Oceanic — `Source/Engines/Oceanic/OceanicEngine.h`

- **Target:** `ocean_cohesion` — boid cohesion force
- **Sensitivity:** `+0–0.4` (full wheel adds 0.4 to `effectiveCoh`, clamped at 1.0)
- **DSP application:** Added into `effectiveCoh` at block pre-computation (before MIDI loop — safe, `modWheelAmount` is a persistent member)
- **Member:** `float modWheelAmount = 0.0f;` added next to `PolyAftertouch aftertouch`
- **Musical rationale:** Mod wheel tightens the boid school — boids pull toward their flock center more strongly, creating denser harmonic clusters. Counter-intuitive but dramatic: raising the wheel causes the swarm to school up into a tight chord-like mass vs. the dispersed particle texture at wheel-off.

### 5. Ocelot — `Source/Engines/Ocelot/OcelotEngine.h`

- **Target:** `ocelot_ecosystemDepth` — ecosystem cross-stratum modulation depth
- **Sensitivity:** `+0–0.35` (full wheel adds 0.35 to `snapshot.ecosystemDepth`, clamped at 1.0)
- **DSP application:** Added into the `snapshot.ecosystemDepth` post-MIDI line alongside `atPressure * 0.3f`
- **Member:** `float modWheelAmount = 0.0f;` added in the private section
- **Musical rationale:** Mod wheel deepens the 12-route EcosystemMatrix cross-feed between habitat strata (Floor, Understory, Canopy, Emergent). Raising the wheel causes each stratum to increasingly bleed into the others — the ocelot's territory becomes more interconnected, thickening the texture.

### 6. Overworld — `Source/Engines/Overworld/OverworldEngine.h`

- **Target:** `ow_glitchMix` — glitch engine wet mix
- **Sensitivity:** `+0–0.4` (full wheel adds 0.4 to `effectiveGlitchMix`, clamped at 1.0)
- **DSP application:** Added into `effectiveGlitchMix` alongside `macGlitch * 0.8f`
- **Member:** `float modWheelAmount = 0.0f;` added next to `PolyAftertouch aftertouch`
- **Note:** `ow_turbulenceIntensity` does not exist as a named parameter; turbulence in Overworld is the `GlitchEngine`. Mod wheel targets glitch mix — which produces the "chip artifacts progressively" behavior described in the brief. The actual glitch amount (`effectiveGlitchAmt`) is left unchanged to preserve dynamics, while the wet/dry blend is raised.
- **Musical rationale:** Mod wheel introduces pixel-noise and bit-scramble artifacts from the GlitchEngine progressively — a clean retro sound at wheel-off, rising into crunchy 8-bit destruction at full wheel.

### 7. Osprey — `Source/Engines/Osprey/OspreyEngine.h`

- **Target:** `osprey_seaState` — effective sea state (turbulence intensity is derived from seaState via `FluidCharacter.turbulenceOnset`)
- **Sensitivity:** `+0–0.4` (full wheel adds 0.4 to `effectiveSeaState`, clamped at 1.0)
- **DSP application:** Applied after LFO modulation, before the MIDI loop (after all macro and LFO contributions to `effectiveSeaState`)
- **Member:** `float modWheelAmount = 0.0f;` added next to `PolyAftertouch aftertouch`
- **Note:** `osprey_turbulenceIntensity` does not exist as a named parameter. Turbulence in Osprey is a derived quantity computed from `(seaState - fluid.turbulenceOnset) / (1 - turbulenceOnset)`. Raising `effectiveSeaState` via mod wheel increases all three fluid energy components (swell, chop, turbulence) simultaneously. At low base seaState, wheel-up will first introduce chop then turbulence as it crosses the `turbulenceOnset` threshold — exactly the "storm energy increases" behavior described.
- **Musical rationale:** Mod wheel becomes a storm-intensity fader — from gentle swell at rest to full ocean turbulence at full wheel, matching the osprey's plunge-dive drama.

---

## Build Verification

```
cmake --build build --target XOlokun_All
```

**Result:** PASS — 66/66 targets, 0 errors, 0 warnings related to mod wheel changes. AU component installed to `/Users/joshuacramblet/Library/Audio/Plug-Ins/Components/XOlokun.component`.

---

## Fleet Coverage

| Round | Engines Added | Running Total |
|-------|--------------|---------------|
| Round 7A | Bob, Bite, Dub, Oceanic, Ocelot, Overworld, Osprey (original 7) | ~9/22 |
| Round 11E | Onset, Opal, Organon, Ouroboros, Obscura, Owlfish | 15/22 |
| **Round 12C** | **Bob, Bite, Dub, Oceanic, Ocelot, Overworld, Osprey** | **22/22** |

All 22 MIDI-capable engines now implement CC#1 mod wheel. The 4 family engines (Obbligato, Ohm, Ole, Orphica, Ottoni) are out of scope per the brief.

**D006 ("Expression Input Is Not Optional") is FULLY RESOLVED fleet-wide:**
- Velocity → timbre: RESOLVED (Round 9E filter envelopes fleet-wide)
- Aftertouch (CC channel pressure): 22/22 (complete as of Rounds 10+11)
- Mod wheel (CC#1): **22/22** (complete as of Round 12C)

---

## Substitutions and Deviations

| Engine | Requested Target | Actual Target | Reason |
|--------|-----------------|---------------|--------|
| Overworld | "turbulence intensity" | `effectiveGlitchMix` (`ow_glitchMix`) | No `turbulenceIntensity` parameter exists. Turbulence in Overworld is the GlitchEngine; mod wheel raises glitch wet mix. Functionally produces "chip artifacts progressively." |
| Osprey | "turbulence intensity" | `effectiveSeaState` | No standalone turbulence parameter exists. Turbulence is derived from seaState via `turbulenceOnset` threshold. Raising seaState via mod wheel progressively introduces turbulence once the onset threshold is crossed — identical behavior to "storm energy increasing." |

All other engines wired exactly as specified.
