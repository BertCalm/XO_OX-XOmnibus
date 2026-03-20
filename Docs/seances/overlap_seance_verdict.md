# OVERLAP Seance Verdict
**Engine**: XOverlap (OVERLAP)
**Date**: 2026-03-17
**Source**: Source/Engines/Overlap/XOverlapAdapter.h
**Accent Color**: Bioluminescent Cyan-Green #00FFB4
**Parameter Prefix**: olap_

---

## DOCTRINE COMPLIANCE

| Doctrine | Status | Notes |
|----------|--------|-------|
| D001 — Velocity shapes timbre | PASS | On every NoteOn, `filterEnvVelocity = velocity` is stored and multiplies the filter envelope: `envMod = filterEnvLevel * filterEnvVelocity * params.filterEnvAmt` (Adapter.h:223). Harder hits open the filter further. Voice oscillator also scales by velocity: `oscOut * envLevel * velocity` (Voice.h:251). Dual-path — amplitude AND timbre both respond. |
| D002 — Modulation is lifeblood | PASS | LFO1 and LFO2 both implemented with 5 shapes (Sine/Triangle/Saw/Square/S&H) and 6 destinations each (Tangle/Dampening/Pulse Rate/Delay/Filter/Spread). Mod wheel (CC1) routes to 4 destinations with depth. Aftertouch routes to 4 destinations with depth. All 4 macros confirmed active in renderBlock(): M1 KNOT (7-breakpoint topology morph, lines 148–154), M2 PULSE (pulse rate + spread, lines 156–158), M3 ENTRAIN (entrainment + feedback, lines 160–162), M4 BLOOM (bioluminescence + filter open, lines 164–166). Full compliance. |
| D003 — Physics rigor | N/A (PASS as applicable) | Not an acoustic physical model, but the Kuramoto oscillator coupling is mathematically cited inline: `dθᵢ/dt = ωᵢ + (K/N) Σⱼ sin(θⱼ − θᵢ)` (Entrainment.h:10–11). FDN knot matrices are near-unitary circulants; `interpolate()` guarantees stability at all tangle depths. Torus knot delay ratios are derived from genuine Lissajous geometry (KnotMatrix.h:173–182). The math governs the metaphor governs the sound. |
| D004 — Dead parameters are broken promises | PASS | **Fixed (commit c261a81):** All 5 dead parameters wired: `olap_brightness` scales bioluminescence shimmer (0.2–1.0×); `olap_current` + `olap_currentRate` drive a pitch-drift sine LFO on `targetGlideFreq`; `olap_glide` wired to `Voice.h glideCoeff`; `olap_voiceMode` implements mono voice stealing. All 5 confirmed live. |
| D005 — Engine must breathe | PASS | LFO1 rate floor is 0.01 Hz (Adapter.h:480). LFO2 rate floor is 0.01 Hz (Adapter.h:488). Both LFOs always advance phase each sample. Bioluminescence modulators in Bioluminescence.h drift autonomously at 0.3–0.79 Hz regardless of note state. The engine breathes at rest. |
| D006 — Expression input is not optional | PASS | Velocity→timbre confirmed (D001). CC1 mod wheel parsed per-sample in MIDI loop (Adapter.h:258–259), routes to 4 destinations with configurable depth (lines 171–178). Channel aftertouch parsed per-sample (Adapter.h:260–261), routes to 4 destinations with configurable depth (lines 180–188). Both controllers affect core synthesis parameters — topology, entrainment, bioluminescence, filter cutoff, pulse rate. |

---

## SILENCE GATE

SilenceGate is fully integrated.

- `SilenceGate silenceGate` is declared as a member (Adapter.h:540).
- `silenceGate.prepare(sampleRate, maxBlockSize)` called in `prepare()` (line 47).
- Pre-pass MIDI scan wakes the gate before the bypass check: NoteOn events call `silenceGate.wake()` (lines 233–235).
- Bypass check: `if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }` (line 237). This correctly exits only when idle and no MIDI is pending.
- Post-render analysis: `silenceGate.analyzeBlock(rL, rR, numSamples)` called after the sample loop (line 368).

Integration is textbook-correct. Zero-idle CPU cost achieved.

---

## FASTMATH ADOPTION

FastMath is adopted fleet-wide throughout all OVERLAP DSP files.

- **FastMath.h** (DSP/FastMath.h): Forwarding header that re-exports all `xomnibus::` FastMath functions under the `xoverlap::` namespace via `using` declarations (lines 15–31). Pulls from `../../../DSP/FastMath.h` (the shared XOmnibus FastMath library).
- **Adapter.h**: `fastTan()` for SVF filter coefficient (line 227), `fastPow2()` for filter frequency scaling (lines 177, 224, 640), `fastCos()`/`fastSin()` for stereo panning (lines 292–293), `fastSin()` in LFO computation (line 619).
- **Voice.h**: `fastSin()` for oscillator waveform (line 232), `fastCos()` for pulse envelope (line 242), `fastExp()` in envelope coefficient (line 142).
- **Entrainment.h**: `fastSin()`/`fastCos()` for Kuramoto circular mean and phase nudge (lines 64–65, 94).
- **Bioluminescence.h**: `fastSin()` for tap amplitude modulation (line 93).
- **PostFX.h**: `fastSin()` for chorus LFO (lines 78–79).
- **FDN.h**: No trigonometric calls needed — matrix multiply and one-pole filters only. No `std::sin`/`std::cos`/`std::exp` present.

No hot-path `std::sin`, `std::cos`, or `std::exp` calls remain. FastMath adoption is complete (34/34).

---

## COUPLING SUPPORT

Seven `CouplingType` enums handled in `applyCouplingInput()` (Adapter.h:377–413):

| CouplingType | Routing in OVERLAP |
|---|---|
| `AudioToFM` | Modulates FDN delay base proportionally: `extDelayMod = buf[0] * amount` → `modDelayBase * (1 + extDelayMod * 0.3)` |
| `AudioToRing` | Ring modulates stereo output: `extRingMod = buf[0] * amount` → output multiplied by `(1 + extRingMod)` |
| `AmpToFilter` | Raises filter cutoff: `extFilterMod = amount * 4000.0f` → added to `modFilterCutoff` |
| `EnvToMorph` | Pushes tangle depth: `extPitchMod = amount` → `modTangleDepth += extPitchMod * 0.05` |
| `LFOToPitch` | Tangle depth perturbation: `extPitchMod = buf[0] * amount` |
| `PitchToPitch` | Same as LFOToPitch (semitone offset → tangle perturbation) |
| `FilterToFilter` | Multiplicative cutoff shift: `extFilterMod = amount * modFilterCutoffCache` — **BUG: cache is never updated; always 8000 Hz** |

Output (via `getSampleForCoupling()`): stereo L/R cached per-sample as `lastSampleL`/`lastSampleR`. Suitable as source for `AudioToFM`, `AudioToRing`, and `AudioToBuffer` (OPAL Time Telescope).

---

## SUMMARY

**Engine is NOT V1-ready as-is. The D004 failure requires resolution before ship.**

OVERLAP is architecturally strong — the Kuramoto entrainment, knot-topology FDN, and SilenceGate/FastMath integration are all exemplary. Four of six doctrines pass cleanly. The engine fails D004 with 5 dead parameters, and carries two additional blocking-class bugs: a stale `FilterToFilter` coupling cache and 4 preset files using pre-refactor parameter names that will silently load with defaults.

The good news: three of the five dead parameters (glide, voiceMode, brightness) have partial infrastructure in Voice.h already. The wiring work is small relative to the DSP quality already in place. Fix these issues and OVERLAP clears 8.5/10.

**Score: 7.2 / 10 → Updated: 8.4 / 10** (D004 FAIL resolved, commit c261a81)

---

## NEXT ACTIONS

### CRITICAL — D004 Dead Parameters (5 params, ~2–4 hours work)

1. **`olap_brightness`** — Adapter.h:449. Intended as filter/tonal brightness control. Wire `params.brightness` to shift `effectiveCutoff` (e.g., multiply by `0.5 + params.brightness`) before the SVF coefficient block (line 224). Or route it to `dampeningCoeff` in the FDN for high-frequency content shaping.

2. **`olap_current`** + **`olap_currentRate`** — Adapter.h:453–456. These are declared as an "Ocean Current" slow pitch drift. Wire them as a per-block LFO-style offset to `modPulseRate` or `modDelayBase`, using `currentRate` as the oscillator frequency and `current` as depth. Alternatively add a slow sine oscillator in `prepare()`/`renderBlock()` driven by these two params.

3. **`olap_voiceMode`** — Adapter.h:435–436. The allocateVoice() function always runs polyphonic. Wire `params.voiceMode` (0=Poly, 1=Mono, 2=Legato) by checking it in `handleNoteOn()`: Mono mode should steal the single most recent voice; Legato should steal and glide. Infrastructure for glide smoothing already exists in Voice.h (lines 196–221).

4. **`olap_glide`** — Adapter.h:437–438. Voice.h has a one-pole glide smoother (`glideFreq += 0.005f * (targetGlideFreq - glideFreq)`, line 221) but the coefficient `0.005f` is hardcoded. Replace it with a coefficient derived from `params.glide` (convert ms to a one-pole smoothing factor) and pass it into `voice.process()` or set it per NoteOn.

### HIGH — Stale Presets (4 files, migration required)

5. **Migrate 4 Flux presets** that use pre-refactor parameter names. Files:
   - `Presets/XOmnibus/Flux/Overlap_Loop_Collision.xometa`
   - `Presets/XOmnibus/Flux/Overlap_Knot_Unraveling.xometa`
   - `Presets/XOmnibus/Flux/Overlap_Tidal_Surge.xometa`
   - `Presets/XOmnibus/Flux/Event_Horizon.xometa`

   Old names to replace: `olap_knotType` → `olap_knot`, `olap_delayFeedback` → `olap_feedback`, `olap_damping` → `olap_dampening`, `olap_modDepth` → `olap_lfo1Depth`, `olap_modRate` → `olap_lfo1Rate`, `olap_macroCharacter` → `olap_macroKnot`, `olap_macroSpace` → `olap_macroBloom`, `olap_macroCoupling` → `olap_macroEntrain`, `olap_macroDecay` → `olap_macroPulse`.

### HIGH — Preset Population (105 presets needed)

6. **Current count: 45. Target: 150+.** OVERLAP is the most under-populated engine in the fleet relative to target. Priority sound design areas: long-sustain Unknot pads, Figure-Eight percussive glitch beds, Torus(3,2) evolving textures, high-entrain unison drones, extreme bloom shimmer atmospheres, coupled presets (OPAL + OVERLAP pair, OUTWIT → OVERLAP topology drive).

### MEDIUM — FilterToFilter Bug

7. **`modFilterCutoffCache`** (Adapter.h:595) is declared as `float modFilterCutoffCache = 8000.0f` and never updated in `renderBlock()`. When `FilterToFilter` coupling fires, `extFilterMod = amount * modFilterCutoffCache` always sends `amount * 8000` regardless of the engine's actual running filter cutoff. Fix: update `modFilterCutoffCache = modFilterCutoff` in `renderBlock()` after the macro block and before `applyCouplingInput` processing (around line 190).

### LOW — Aftertouch Dest Mislabel

8. **`olap_atPressureDest` option 2** is labeled `"Brightness"` in the StringArray (Adapter.h:520) but the `case 2` branch routes to `modBioluminescence` (line 185), not `params.brightness`. Either rename the label to `"Bioluminescence"` or reroute case 2 to actually modulate brightness. The mislabel creates user confusion in any UI displaying the destination name.

### OBSERVATION — LFO Destinations Missing Key Params

9. Both LFO destination lists omit `Bioluminescence` and `Entrain` — the two most distinctive OVERLAP parameters (Adapter.h:486, 494). These are only reachable via macros and expression. Adding them as LFO destinations would significantly expand patch depth with minimal code change (add cases 6 and 7 to `applyLFOModulation()`).
