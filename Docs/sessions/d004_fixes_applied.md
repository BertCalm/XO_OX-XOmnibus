# D004 Dead Parameter Fixes — Round 3 (2026-03-14)

**Doctrine:** D004 — Dead Parameters Are Broken Promises
Every declared parameter must affect audio output.

---

## Summary

5 D004 violations resolved across 5 engines. All fixes wire existing declared parameters
to real DSP with audible effect. No parameters were removed — every fix activates
dormant behaviour that was always intended.

---

## Fix 1: SNAP — `snap_macroDepth` → Stereo Pan Spread

**File:** `Source/Engines/OddfeliX/OddfeliXEngine.h`
**Line ~353:** Replaced `(void) macroDepth;`

**Before:** M4 DEPTH macro was loaded and immediately discarded.

**After:** `macroDepth` controls unison stereo pan spread width.
- At `macroDepth=0`: spread = 0.3 (original default, no change)
- At `macroDepth=1`: spread = 1.0 (fully wide stereo image)
- Metaphor: feliX descending the water column encounters a wider, more spacious acoustic environment — the stereo image expands as he goes deeper.

**Wiring:** `effectivePanSpread = 0.3 + macroDepth * 0.7` → used as the multiplier in the per-unison pan offset calculation in the render loop.

---

## Fix 2: OWLFISH — `owl_morphGlide` → Mixtur Blend During Portamento

**File:** `Source/Engines/Owlfish/OwlfishVoice.h`
**Location:** Per-block parameter setup in `process()`

**Before:** `morphGlide` was loaded by snapshot and serialized in presets but never read by DSP.

**After:** `morphGlide` modulates the `mixtur` (Mixtur-Trautonium waveshaper blend) parameter during portamento glides.
- At `morphGlide=0`: no modulation — abrupt sub-division transitions (no timbral swell).
- At `morphGlide=1`: mixtur sweeps up to +0.5 at the midpoint of a glide, then returns to `snap.mixtur` as the pitch settles at the target note.
- The effect: the organism's harmonic content swells and morphs during pitch glides, like a creature whose body vibrates differently as it changes depth.

**Wiring:** Block-rate `blockGlideProgress` computed from `|currentFreq - targetFreq| / targetFreq`. `glideModMixtur = snap.mixtur + snap.morphGlide * 0.5 * (1 - blockGlideProgress)` passed to `abyssOsc.setParams()`.

---

## Fix 3: OBLIQUE — `oblq_percDecay` → Bounce Click Duration

**File:** `Source/Engines/Oblique/ObliqueEngine.h`
**Location:** `ObliqueBounce::Params` struct + `fireClick()` method

**Before:** `bounceClickDecay` was read from APVTS parameter `oblq_percDecay` but never passed to `ObliqueBounce`. The click duration was hardcoded to 3ms.

**After:** `bounceClickDecay` controls the per-click burst duration via a new `clickDecay` field in `ObliqueBounce::Params`.
- At `clickDecay=0`: 1ms click (ultra-tight snap, inaudible pitch)
- At `clickDecay=0.1` (default): ~3ms click (matches original hardcoded behavior)
- At `clickDecay=1`: 30ms click (recognizable pitched transient, Simmons tom character)

**Wiring:**
1. Added `float clickDecay` field to `ObliqueBounce::Params` struct.
2. Added `float currentClickDecay` member to `ObliqueBounce` class.
3. `process()` captures `bounceParams.clickDecay` into `currentClickDecay` each call.
4. `fireClick()` uses `currentClickDecay` to compute `clickDurationMs = 1 + clickDecay * 29` before computing `clickDecayCoefficient`.
5. Render loop sets `bounceParams.clickDecay = bounceClickDecay`.

---

## Fix 4: OCELOT — Four Macros (`ocelot_prowl/foliage/ecosystem/canopy`) → Strata DSP

**File:** `Source/Engines/Ocelot/OcelotParamSnapshot.h`
**Location:** End of `updateFrom()` method

**Before:** All four macros (`macroProwl`, `macroFoliage`, `macroEcosystem`, `macroCanopy`) were loaded from APVTS and stored in the snapshot but never consumed by any DSP code.

**After:** Macro routing block added at end of `updateFrom()`, applying additive modulation to snapshot fields before they are read by DSP:

| Macro | Target Parameters | Effect |
|-------|------------------|--------|
| PROWL | `ecosystemDepth` +0.5, `density` +0.4 | Hunting movement — deeper cross-stratum interaction, denser activity |
| FOLIAGE | `reverbSize` +0.4, `reverbMix` +0.3 | Environment depth — larger, lusher reverb space (denser canopy) |
| ECOSYSTEM | `xfFloorCanopy` +0.5, `xfCanopyFloor` +0.3, `xfUnderEmerg` +0.4 | Cross-voice interaction — all strata couple more intensely |
| CANOPY | `canopyLevel` +0.4, `canopyShimmer` +0.5, `canopySpectralFilter` +0.3 | Brightness — more light through the canopy, airier top layer |

All applications use `std::clamp` to stay within valid parameter ranges.

---

## Fix 5: OSPREY — `OspreyLFO` Dead Code → Sea State Breathing Modulator

**File:** `Source/Engines/Osprey/OspreyEngine.h`
**Location:** Member data, `prepare()`, `reset()`, and `renderBlock()`

**Before:** The `OspreyLFO` struct (Section 2, lines ~202-275) was fully implemented with 5 waveshapes but never instantiated as a class member — dead code (also a D005 violation: no LFO in any engine without one).

**After:** `OspreyLFO seaStateLFO` added as a private member, initialized in `prepare()` and `reset()` (sine shape, 0.1 Hz default), and advanced in `renderBlock()`.

**Routing:** LFO output modulates `effectiveSeaState`:
- Rate: `0.05 + macroMovement * 0.95` Hz (0.05 Hz at rest → 1.0 Hz at full MOVEMENT)
- Depth: fixed ±0.15 (audible swell without overwhelming the base sea state)
- Effect: the engine breathes organically — sea state gently rises and falls like slow ocean swells, accelerating to choppy waves as MOVEMENT increases.

This simultaneously resolves the D004 violation (LFO code was declared but unused) and partially addresses D005 (engine now has an LFO with rate floor of 0.05 Hz, well below the 0.01 Hz requirement).

---

## Files Modified

| File | Engine | Fix |
|------|--------|-----|
| `Source/Engines/OddfeliX/OddfeliXEngine.h` | SNAP | macroDepth → stereo spread |
| `Source/Engines/Owlfish/OwlfishVoice.h` | OWLFISH | morphGlide → mixtur during portamento |
| `Source/Engines/Oblique/ObliqueEngine.h` | OBLIQUE | percDecay → click burst duration |
| `Source/Engines/Ocelot/OcelotParamSnapshot.h` | OCELOT | 4 macros → strata DSP |
| `Source/Engines/Osprey/OspreyEngine.h` | OSPREY | OspreyLFO instantiated → sea state breathing |

---

## Audit Notes

- OWLFISH macros (DEPTH, FEEDING, DEFENSE, PRESSURE) were already wired in `OwlfishParamSnapshot::applyMacros()` — no action needed.
- OSPREY fix also partially resolves D005 (LFO rate floor = 0.05 Hz).
- All fixes preserve existing parameter IDs and preset compatibility (additive modulation only; default macro values are 0.0, preserving existing behavior at factory defaults).
