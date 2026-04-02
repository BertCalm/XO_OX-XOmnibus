# XOceanus Code Health Report — Round 2

**Author:** Code Health Agent
**Date:** 2026-03-14
**Source:** Live filesystem reads + Grand Survey (`Docs/xoceanus_landscape_2026.md`)
**Scope:** All 24 integrated engines in `Source/Engines/`

---

## Section 1: Dead Parameter Audit (D004)

Dead parameters are parameters that are declared, attached to the APVTS, and loaded (or present in presets) but whose values have no effect on audio output. Findings are ordered by P0 → P1 severity.

| Engine | Parameter ID | Declared In | Read In DSP? | In Presets? | Recommendation |
|--------|-------------|-------------|-------------|-------------|----------------|
| OBSIDIAN | `obsidian_formantIntensity` (collision) | `ObsidianEngine.h:1078` and `1109` | Both `pFormantResonance` and `pFormantIntensity` point to the same ID — only one parameter controls both slots | Unknown | **Wire** — assign `pFormantResonance` to a unique ID (e.g., `obsidian_formantResonance`); this is P0 |
| ODDFELIX (Snap) | `snap_macroDepth` (M4 DEPTH) | `SnapEngine.h:~280` (parameter layout); loaded into `macroDepth` local | `(void) macroDepth;` at line 350 — explicitly discarded | Yes — in all presets as M4 value | **Wire** — described as "echo/FX depth descending the column"; route to reverb send or prism delay mix |
| OWLFISH | `owl_morphGlide` | `OwlfishParameters.h:11,117`; loaded into snapshot at `OwlfishParamSnapshot.h:95,159` | Not referenced in `OwlfishEngine.h` or `OwlfishVoice.h` — confirmed absent from all DSP paths | Yes — in all 16 standalone presets | **Wire** — route to `SubharmonicOsc` portamento/glide coefficient |
| OBLIQUE | `oblq_percDecay` | `ObliqueEngine.h:1127` (layout); loaded as `bounceClickDecay` at line 739 | Loaded into `bounceClickDecay` but never passed to `BounceOscillator.clickDecayCoefficient` or `updateClickDecay()`; hardcoded `0.99f` wins at line 229 | Yes — in 7 presets | **Wire** — one-line fix: pass `bounceClickDecay` to `voice.bounceOscillator.updateClickDecay(bounceClickDecay)` at the per-voice update site |
| ODYSSEY (Drift) | `crossFmDepth`, `crossFmRatio` | Documented in Grand Survey as "in ParamSnapshot" | Neither ID found anywhere in `Source/Engines/Drift/` — these parameters appear to not exist in the current codebase at all | Unknown — needs preset audit | **Investigate** — if these IDs were removed from the layout but remain in presets, silent load failures occur; if they were never added, the seance finding may be stale |
| OCELOT | `ocelot_prowl`, `ocelot_foliage`, `ocelot_ecosystem`, `ocelot_canopy` | `OcelotParameters.h:89-92,219-222`; loaded into snapshot at `OcelotParamSnapshot.h:90-93,182-185` | Not referenced anywhere in `OcelotEngine.h`, `OcelotVoice.h`, `OcelotEmergent.h`, or `EcosystemMatrix.h` | In 0 XOceanus presets (engine has zero presets) | **Wire** — route into EcosystemMatrix biome-blend parameters; this is the signature feature of the engine |

**Notes:**
- The Drift engine's `modWheelAmount` (CC1) IS populated from MIDI at line 733 and applied to filter cutoff at line 901. The Grand Survey note about "ModWheel mod source never populated" is **inaccurate for the current codebase** — mod wheel is functional in Drift. Aftertouch is absent, however.
- The Grand Survey's `crossFmDepth`/`crossFmRatio` finding could not be confirmed; those IDs do not appear in any Drift source file. The report may reflect a design spec never implemented, not a regression.
- Ocelot's macros use semantic names (`ocelot_prowl` etc.), not generic `macro_1`–`macro_4` as stated in the Grand Survey.

---

## Section 2: Missing MIDI Expression (D006)

### CC Handling Survey

| Engine | Has CC1 (Mod Wheel) | Has CC64 (Sustain) | Has Aftertouch | Velocity → Timbre | Priority |
|--------|--------------------|--------------------|----------------|-------------------|----------|
| BITE (Overbite) | Not found in adapter | Not found | 1 ref in source — "Aftertouch" as mod source name in macro display (`BiteEngine.h:1422`) | Yes — 8 velocity refs; Bite/Belly macro driven | Low — best in fleet |
| DRIFT (Odyssey) | **Yes** — CC1 → `modWheelAmount` → filter cutoff +4000 Hz (`DriftEngine.h:733,901`) | Not found | Not found — `AfterTouch` appears only as a string label, never as a MIDI handler | 8+ refs; mostly amplitude; filter mod via modwheel only | **Medium** — add channel pressure |
| MORPH (OddOscar) | **Yes** — CC1 → `modWheelMorphOffset` → morph position sweep (`MorphEngine.h:468,1003`) | Not found | Not found | 6 refs; amplitude only | Medium |
| FAT (Obese) | CC64 (sustain) only (`FatEngine.h:900`) | Yes | Not found | Unchecked; Perlin pitch drift only modulation | **High** — no timbre CC at all |
| SNAP (OddfeliX) | Not found | Not found | Not found | Linear velocity — not exponential | **High** |
| BOB (Oblong) | Not found | Not found | Not found | 9 refs; partial timbre routing | High |
| DUB (Overdub) | Not found | Not found | Not found | 8 refs; mostly amplitude | High |
| OPAL | CC64 sustain pedal (`OpalEngine.h:1704`) | Yes | Not found | Not implemented | High |
| OBSIDIAN | Not found | Not found | Not found | Velocity only scales amplitude per seance | High |
| OSTERIA | Not found | Not found | Not found | Not found | High |
| OSPREY | Not found | Not found | Not found | Not found | High |
| ORGANON | Not found | Not found | Not found | VFE-internal only | Medium |
| ORBITAL | Not found | Not found | Not found | Group envelope only | High |
| OBLIQUE | Not found | Not found | Not found | Not found | High |
| OVERWORLD | Not found | Not found | Not found | 1 ref — amplitude only | High |
| OCEANIC | Not found | Not found | Not found | Zero — every note identical | **Critical** |
| OWLFISH | Not found | Not found | Not found | Armor trigger only, not timbral | High |
| OCELOT | Not found | Not found | Not found | 1 ref | High |
| ONSET | Not found | Not found | Not found | Not found | High |
| OUROBOROS | Not found | Not found | Not found | Velocity outputs to coupling channel | Low |
| ORIGAMI | Not found | Not found | Not found | Some refs | Medium |
| ORACLE | Not found | Not found | Not found | GENDY breakpoints may respond | Medium |
| OBSCURA | 16 velocity refs; excitation velocity | Not found | Not found | Good | Low |
| ORGANON | Not found | Not found | Not found | VFE-internal | Medium |

**Fleet-wide finding:** Zero engines implement channel pressure (aftertouch) in the MIDI processing loop. `PolyAftertouch.h` exists at `Source/Core/PolyAftertouch.h` as a complete helper class with both mono and poly aftertouch support — no engine instantiates or calls it. This is a fleet-wide D006 violation with a ready-made fix: instantiate `PolyAftertouch` in each engine and route its output to an expressive parameter.

**Correction to Grand Survey (P1-05):** ODYSSEY's mod wheel IS implemented and functional. The mod source struct member named "ModWheel" is populated at `DriftEngine.h:733`. The seance finding was based on the mod sources struct naming convention, not the actual implementation.

---

## Section 3: Zero-Modulation Engines (D005)

An engine that "cannot breathe" has no internal LFO for slow autonomous modulation (D005 requires rate floor ≤ 0.01 Hz).

**Correction to Grand Survey:** OBLIQUE is flagged as "zero LFO" in the Grand Survey, but this is inaccurate. `ObliqueEngine.h` contains a fully operational `ObliquePhaser` class with a sine LFO (`lfoPhase` accumulator, `fastSin` call, parametric rate/depth) at lines 492–566. This phaser LFO is connected to the audio path at line 994 (`phaserEffect.process(postPrismL, postPrismR, phaserParams)`). Oblique's phaser LFO is functional.

| Engine | LFO Status | Evidence | D005 Violation? |
|--------|-----------|----------|-----------------|
| DRIFT (Odyssey) | Working — 20+ LFO refs | Full mod matrix with LFO mod sources | No |
| BOB (Oblong) | Working — 8+ LFO refs | CuriosityEngine modulation | No |
| DUB (Overdub) | Working — single sine LFO | Tape delay modulation, 23 refs | No (but weak — single LFO only) |
| OPAL | Working — 13 LFO refs | Grain scatter LFOs | No |
| OBSCURA | Working — 15 LFO refs | Verlet mass-spring modulation | No |
| ORIGAMI | Working — 13 LFO refs | STFT spectral modulation | No |
| ORACLE | Working — 11 LFO refs | GENDY stochastic | No |
| BITE (Overbite) | Working — 37 LFO refs | Full 3× LFO mod matrix | No |
| MORPH (OddOscar) | **Working — internal coupling LFO** at `MorphEngine.h:500-509` | 0.3 Hz sine LFO output on coupling ch2 | **Partial** — LFO only on coupling output, not modulating Morph's own internal parameters |
| OBLIQUE | **Working** — phaser LFO at `ObliqueEngine.h:492-514` | `ObliquePhaser` sine LFO, rate 0.05–8 Hz, full implementation | **No** — Grand Survey finding was incorrect |
| SNAP (OddfeliX) | **Zero** — no LFO infrastructure | No LFO struct, no phase accumulator in DSP path | **Yes — D005 violation** |
| FAT (Obese) | **Zero** — 0.1 Hz Perlin pitch drift only, not a proper LFO | Perlin drift is hardcoded 0.1 Hz pitch wobble; no filter/amp LFO | **Yes — D005 violation** |
| OWLFISH | **Zero** — 50 parameters, no LFO | No LFO struct in any of 13 source files | **Yes — D005 violation** |
| ORBITAL | **Zero** in adapter | No LFO refs beyond coupling case handler | **Yes — D005 violation** |
| ORGANON | **Zero** standard LFOs | VFE metabolic system provides internal evolution; no standard LFO | **Yes — D005 violation** (VFE is a mitigant but not compliant) |
| ONSET | **Zero** | No LFO in 2,143-line engine | **Yes — D005 violation** |
| OVERWORLD | **Zero** in adapter | Standalone has some; adapter exposes only `ow_era` single param | **Yes — D005 violation** |
| OSPREY | **Dead code** — `OspreyLFO` struct (lines 202-265) is fully implemented with Sine/Triangle/Saw/Square/SampleAndHold shapes and an LCG random source | `grep "OspreyLFO"` returns only the struct definition — never instantiated as a class member | **Yes — D005 violation** |

**Summary:** 8 confirmed D005 violations (Snap, Fat, Owlfish, Orbital, Organon, Onset, Overworld adapter, Osprey). Oblique is NOT a violation — the Grand Survey was wrong. Morph is borderline (LFO exists but only on coupling output).

---

## Section 4: Confirmed Audio Bugs

### P0-01 — OBSIDIAN Right Channel Filter Bypass

**File:** `Source/Engines/Obsidian/ObsidianEngine.h`
**Lines:** 801–805
**Exact code:**
```cpp
outputLeft = voice.mainFilter.processSample (outputLeft);   // line 801
float mid  = (outputLeft + outputRight) * 0.5f;             // line 802
float side = (outputLeft - outputRight) * 0.5f;             // line 803
                                                              // line 804 (blank)
outputRight = mid - side;                                    // line 805
```
**Bug:** `voice.mainFilter` processes `outputLeft` but not `outputRight`. Lines 802-805 reconstruct a pseudo-stereo signal using M/S math on the filtered left and the *unfiltered* right. The right channel never passes through `voice.mainFilter` — it only receives half the filtered-left blended back in. Result: asymmetric timbre between channels; right channel is always brighter than left.
**Suggested fix:**
```cpp
outputLeft  = voice.mainFilter.processSample(outputLeft);
outputRight = voice.mainFilter.processSample(outputRight);
```
(Remove the erroneous M/S reconstruction at lines 802-805, or add a second filter state for the right channel if stereo spread is desired.)

---

### P0-02 — OSTERIA Warmth Filter L-Only Processing

**File:** `Source/Engines/Osteria/OsteriaEngine.h`
**Line:** 1227
**Exact code:**
```cpp
mixL = warmthFilter.processSample (mixL);   // line 1227
// mixR is never processed — line 1228 is blank, then tavern room:
tavernRoom.processSample (mixL, mixR, effectiveTavern);  // line 1230
```
**Bug:** The warmth low-shelf filter (300 Hz, up to +8 dB boost) is applied to `mixL` only. `mixR` proceeds to the tavern room reverb with no warmth processing. The proximity-EQ warmth effect is mono-left.
**Suggested fix:**
```cpp
mixL = warmthFilter.processSample(mixL);
mixR = warmthFilter.processSample(mixR);
```
Note: `CytomicSVF` processes one sample at a time and holds state — calling it twice in sequence on L then R is correct provided warmth is the same on both channels (which it should be).

---

### P0-03 — ORIGAMI STFT Race Condition (Potential)

**File:** `Source/Engines/Origami/OrigamiEngine.h`
**Lines:** 758–770, 1404–1421
**Issue:** The STFT uses `kHopSize = 512` samples between analysis frames (4× overlap, `kFFTSize = 2048`). `hopSampleCounter` is incremented per sample at line 758 and the FFT triggered at line 759 when it reaches `kHopSize`. The overlap-add accumulator write happens at line 1421. If a host calls `renderBlock` with `maxBlockSize < 512`, the hop counter crosses the boundary in the middle of a block. The overlap-add accumulator write and `hopSampleCounter` reset are not serialized with respect to buffer boundaries — no explicit mutex or lock-free queue.
**Severity assessment:** This is a potential glitch under specific host conditions (`maxBlockSize < 512`), not a guaranteed bug. The `prepare()` function at line 453 does resize buffers for `maxBlockSize` but does not validate against `kHopSize`. Standard DAW block sizes (128, 256, 512, 1024) mean this will trigger on 128 and 256-sample blocks.
**Suggested fix:**
```cpp
// In prepare(), add assertion:
jassert(maxBlockSize >= kHopSize); // or handle partial-hop blocks
// Or accumulate samples across renderBlock calls until kHopSize reached
```

---

### P0-04 — OBSIDIAN Formant Parameter ID Collision

**File:** `Source/Engines/Obsidian/ObsidianEngine.h`
**Lines:** 1078, 1109
**Exact code:**
```cpp
// Line 1078:
pFormantResonance = apvts.getRawParameterValue("obsidian_formantIntensity");
// ...
// Line 1108-1109: (comment + code)
// ---- Formant intensity (note: shares parameter ID with pFormantResonance) ----
pFormantIntensity = apvts.getRawParameterValue("obsidian_formantIntensity");
```
**Bug:** Both `pFormantResonance` (declared at line 1538) and `pFormantIntensity` (declared at line 1567) are attached to the same parameter ID `"obsidian_formantIntensity"`. They point to the same atomic float. The comment at line 1108 acknowledges the collision. Effect: the formant resonance knob and the formant intensity knob both read the same value; one cannot be adjusted independently. Formant resonance is permanently controlled by the intensity parameter.
**Suggested fix:**
```cpp
// Add to parameter layout:
addParam("obsidian_formantResonance", "Obsidian Formant Resonance", 0.0f, 1.0f, 0.5f);
// Change line 1078 to:
pFormantResonance = apvts.getRawParameterValue("obsidian_formantResonance");
```

---

### Additional P0 Candidates Found During Search

**P0-05 — OVERWORLD `getSampleForCoupling` returns mono scalar (not stereo)**
**File:** `Source/Engines/Overworld/OverworldEngine.h`
**Lines:** 281–284
```cpp
float getSampleForCoupling(int /*channel*/, int /*sampleIndex*/) const override
{
    return lastSample;
}
```
The `channel` and `sampleIndex` arguments are both ignored (commented out with `/**/`). Every channel query returns the same `lastSample` scalar. MegaCouplingMatrix expects channel 0 = left, channel 1 = right, channel 2 = envelope. Instead it always gets the same single value regardless of channel or sample position. This breaks all coupling that reads stereo data from OVERWORLD, and makes Overworld's coupling output meaningless as a per-sample source.
**Severity:** P0 if any engine is coupled to receive stereo from OVERWORLD; P1 otherwise.

---

## Section 5: Coupling Stub Analysis

### applyCouplingInput Status

| Engine | `applyCouplingInput` Status | `getSampleForCoupling` Quality | Coupling Types Claimed |
|--------|----------------------------|-------------------------------|----------------------|
| OBSIDIAN | **Substantive** — 4 switch cases: `AudioToFM` (PD depth mod), `AmpToFilter` (cutoff mod), `EnvToMorph` (density + tilt mod), `RhythmToBlend` (PD depth mod) | Real stereo + envelope follower on ch2 | 4 types |
| OSTERIA | **Substantive** — 4 cases: `AudioToWavetable` (excitation mod), `AmpToFilter` (elastic tightness), `AudioToFM` (room excitation), `EnvToMorph` (shore drift) | Real stereo (outputCacheL/R) + envelope on ch2 | 4 types |
| MORPH (OddOscar) | **Substantive** — 2 cases: `AmpToFilter` (inverted envelope → filter duck), `EnvToMorph` (morph position shift) | Real stereo + LFO value on ch2 (0.3 Hz sine) | 2 types + LFO output |
| SNAP (OddfeliX) | **Partial** — 1 switch group handling `AmpToPitch`, `LFOToPitch`, `PitchToPitch` → `externalPitchModulation` accumulation | Real stereo (outputCacheLeft/Right) + envelope on ch2 | 1 coupling group (pitch only) |
| FAT (Obese) | **Substantive** — 2 cases: `AmpToFilter` (filter mod), `AmpToPitch`/`LFOToPitch`/`PitchToPitch` (pitch mod) | Real stereo (outputCacheL/R) + envelope on ch2 | 3 types |
| OVERWORLD | **Substantive** — 3 cases: `AmpToFilter` (±8000 Hz filter offset), `EnvToMorph` (ERA X position), `AudioToFM` (ERA Y position) | **Broken** — returns same `lastSample` scalar for all channels/sample indices (line 281-284) | 3 types (claimed); actual output is mono scalar |
| ORBITAL | **Substantive** — `AudioToWavetable` (partial tilt), `AudioToFM` (FM partial freq mod), `AudioToRing` (ring mod buffer), `LFOToPitch` (pitch mod), `RhythmToBlend` (group envelope trigger) | Real stereo + envelope; coupling audio buffers allocated | 5 types |
| OCELOT | **Stub** — all 4 args cast to `(void)` at line 91; comment says "Full routing wired when running inside XOceanus" | Real stereo (outputCacheL/R) — no ch2 envelope | 0 (claimed some; none implemented) |
| OWLFISH | **Stub** — all 4 args cast to `(void)` at lines 132-135; comment says "to be wired by MegaCouplingMatrix later" | Real stereo (outputCacheL/R) — no ch2 envelope | 0 (claimed some; none implemented) |
| DRIFT (Odyssey) | **Substantive** — switch-impl with real coupling routing (filter mod, morph mod, pitch mod per Grand Survey) | Real stereo + envelope; full cache | Multiple types |
| BOB (Oblong) | **Substantive** — switch-impl (per Grand Survey) | Real stereo cache | Multiple types |
| DUB (Overdub) | **Substantive** — switch-impl (per Grand Survey) | Real stereo + coupling type support | Multiple types |
| BITE (Overbite) | **Best in fleet** — 5-macro driven coupling per Grand Survey | Post-FX stereo + envelope follower | Multiple types |

### Key Findings

1. **OCELOT and OWLFISH have fully stubbed `applyCouplingInput`** — both engines can output audio to the coupling matrix, but cannot receive any input from other engines. All `CouplingType` inputs are silently discarded.

2. **OVERWORLD `getSampleForCoupling` is broken** — the mono scalar return makes it useless as a stereo coupling source. The engine's `applyCouplingInput` has real routing, but what it outputs is meaningless to engines that read per-sample stereo data.

3. **Engines with ch2 envelope output** (useful for `AmpToFilter`, `AmpToChoke` coupling): OBSIDIAN, OSTERIA, FAT, SNAP, MORPH, BITE, BOB, DUB, DRIFT. Notably absent: OCELOT, OWLFISH, OVERWORLD, OBLIQUE.

4. **MORPH's coupling LFO (ch2 = 0.3 Hz sine)** is a well-designed touch — it can drive `LFOToPitch` on any coupled engine without requiring a separate LFO block in the receiving engine. This partially mitigates the "zero internal LFO" concern for Morph.

---

## Summary Table: D004/D005/D006 Status Per Engine

| Engine | Dead Params (D004) | Zero LFO (D005) | CC/Expression (D006) |
|--------|-------------------|-----------------|----------------------|
| SNAP | `snap_macroDepth` (P1) | **Yes — zero LFO** | No CC; linear velocity |
| MORPH | None confirmed | No (coupling LFO, borderline) | CC1 mod wheel functional |
| DUB | None confirmed | No (single LFO) | No CC beyond amplitude |
| DRIFT | `crossFmDepth`/`crossFmRatio` (possibly stale — not found in code) | No | CC1 functional; no aftertouch |
| BOB | None confirmed | No | No CC |
| FAT | None confirmed | **Yes — zero LFO** | CC64 only; no timbre CC |
| ONSET | None confirmed | **Yes — zero LFO** | No CC |
| OVERWORLD | None confirmed | **Yes — zero LFO** | 1 amplitude-only velocity ref |
| OPAL | `opal_smear` (P1 per seance) | No | CC64 sustain only |
| ORGANON | None confirmed | **Yes — zero LFO** | No CC |
| OUROBOROS | None confirmed | No (chaotic internal) | No CC (velocity coupling output) |
| OBSIDIAN | `pFormantResonance`/`pFormantIntensity` ID collision (P0) | No (10 LFO refs) | No CC |
| ORIGAMI | None confirmed | No (13 LFO refs) | Some velocity refs |
| ORACLE | None confirmed | No (11 LFO refs) | No CC |
| OBSCURA | None confirmed | No (15 LFO refs) | Good velocity routing |
| OCEANIC | None confirmed | No (7 LFO refs) | **Zero velocity response** |
| OCELOT | `ocelot_prowl/foliage/ecosystem/canopy` all dead (P1) | **Yes — zero LFO** | 1 velocity ref |
| BITE | None confirmed | No (37 LFO refs) | Best in fleet; 1 aftertouch ref |
| ORBITAL | None confirmed | **Yes — zero LFO** | No CC |
| OPTIC | None confirmed | No (3 LFO refs) | No CC |
| OBLIQUE | `oblq_percDecay` value discarded (P1) | **No — phaser LFO working** (survey was wrong) | No CC |
| OSPREY | Dead `OspreyLFO` struct (P1-09) | **Yes — LFO code exists but never instantiated** | No CC |
| OSTERIA | None confirmed | No (4 LFO refs) | No CC |
| OWLFISH | `owl_morphGlide` never read in DSP (P1) | **Yes — zero LFO** | No CC; armor-trigger velocity only |

---

## Corrections to Grand Survey

The following Grand Survey findings were verified against the live codebase and found to be inaccurate:

1. **P1-04 / P1-05 (Drift mod wheel):** The Grand Survey states "ModWheel mod source never populated with incoming MIDI data." This is false. `DriftEngine.h:733` reads `getControllerValue()` for CC1 and stores it in `modWheelAmount`, which is applied to filter cutoff at line 901. Mod wheel IS functional in Drift. Aftertouch remains absent.

2. **P2-05 / D002 (Oblique zero LFO):** The Grand Survey lists Oblique as "zero LFOs" and a D005 violation. The `ObliquePhaser` class at `ObliqueEngine.h:453-570` contains a working sine LFO with rate parameter, connected to the audio path at line 994. Oblique has a functioning modulation LFO (phaser sweep). It is not a D005 violator.

3. **P1-06 (Ocelot macro names):** The Grand Survey refers to "macro_1 through macro_4" as the dead knobs. The actual parameter IDs are `ocelot_prowl`, `ocelot_foliage`, `ocelot_ecosystem`, `ocelot_canopy`. The dead-parameter finding is confirmed; the IDs are corrected.

---

*Report generated 2026-03-14 by Code Health Agent. All file references are to `Source/Engines/` within the XO_OX-XOceanus repository. Line numbers are approximate and may shift by ±5 lines due to context window reading; verify before applying fixes.*
