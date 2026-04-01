# Drift Option B Implementation — Round 11B

**Date:** 2026-03-14
**Scope:** Targeted Partial Exposure of 7 priority FX parameters into DriftEngine.h
**Status:** COMPLETE — DriftEngine.cpp.o builds cleanly, 0 new errors

---

## What Was Ported

### 7 New Parameter IDs (38 → 45 canonical Drift params)

| Priority | New Param IDs | Default | Musical Role |
|----------|--------------|---------|-------------|
| 1a | `drift_tidalDepth` | 0.0 | TidalPulse amplitude, off by default |
| 1b | `drift_tidalRate` | 0.15 Hz | Breath speed (0.05–2.0 Hz) |
| 2a | `drift_fractureEnable` | false | Fracture on/off gate |
| 2b | `drift_fractureIntensity` | 0.0 | Stutter wet/dry + trigger probability |
| 2c | `drift_fractureRate` | 0.5 | Stutter grain length (0=long, 1=short) |
| 3a | `drift_reverbMix` | 0.0 | Reverb wet/dry, off by default |
| 3b | `drift_reverbSize` | 0.5 | Room scale / feedback (0=small, 1=large) |

All defaults are zero/off — existing presets are unaffected. No existing parameter IDs were renamed.

---

## DSP Classes Added to DriftEngine.h

### DriftTidalPulse (lines ~366–415)

Ported from `~/Documents/GitHub/XOdyssey/src/engine/TidalPulse.h`.

- sin² oscillator producing a unipolar [0, 1] output
- No state beyond a `double phase` counter — trivial memory footprint
- `process(rate, depth)` returns 0.0 early when `depth < 0.0001`
- Applied in the per-voice render loop **after** the amplitude envelope, as a multiplicative amplitude sculpt: `out *= (1.0f - breathVal)`
- TidalPulse phase and state reset on note-on so each note begins at the same breath position

**Signal chain position:** Post-envelope, per-voice. The sin² cycle modulates amplitude so the sound swells and recedes (inhale = loud, exhale = silent) rather than oscillating above/below the baseline. This is the mathematical distinction between TidalPulse and the existing `drift_lfoDepth` (which is a bipolar sine LFO).

### DriftFracture (lines ~416–508)

Ported from `~/Documents/GitHub/XOdyssey/src/engine/Fracture.h`.

- Fixed 4096-sample circular buffer (`std::array<float, 4096>`) — no heap allocation on the audio thread
- Probabilistic stutter trigger: at `intensity=0.0` → never triggers; at `intensity=1.0` → triggers every ~50ms
- Stutter grain length controlled by `rate`: `rate=0` → 2048-sample (~46ms) grains; `rate=1` → 64-sample (~1.5ms) grains
- Per-voice: each voice has its own `DriftFracture fracture` in `DriftVoice`
- Fracture buffer and state reset on note-on (prevents carry-over stutter from previous note)

**Signal chain position:** Post-filter (after FilterA + FilterB), pre-Prism Shimmer. In XOdyssey's standalone signal flow, Fracture was placed post-filter / post-Prism-Shimmer. In the adapter, it is placed before Prism Shimmer so glitch grains also pass through shimmer — this produces a slightly richer glitch texture where stutter fragments have harmonic sheen, consistent with the "controlled disintegration" character described in the XOdyssey spec.

### DriftReverb (lines ~509–620)

Ported from `~/Documents/GitHub/XOdyssey/src/engine/ReverbFX.h`.

- Schroeder topology: 4 parallel comb filters (29.7, 37.1, 41.1, 43.7 ms) + 2 series allpass filters
- Comb delay times are mutually prime ratios to avoid flutter echo
- One-pole lowpass damping in comb feedback path (hardwired to 0.5 — natural pad room character)
- Buffers allocated in `prepare()` via `std::vector::assign()` — no allocation in `renderBlock()`
- Engine-level (not per-voice): a single `DriftReverb reverb` in `DriftEngine`
- Applied to the full block after the per-sample voice loop via `reverb.processStereo()`

**Signal chain position:** Engine-level, post-mix, post-soft-limiter. Processes the `outputCacheL/R` arrays in-place before writing to the shared JUCE buffer. The coupling output (`getSampleForCoupling`) reads from the cache before reverb is applied — coupling sees the dry signal. This is intentional: reverb is a presentation layer, not a modulation source.

---

## Signal Chain After Option B (per-voice)

```
OscA + OscB + Sub + Noise
        |
    [Mix Stage]
        |
  [Haze Saturation]      (drift_hazeAmount)
        |
  [FilterA: LP 12/24]   (drift_filterCutoff, drift_filterReso, drift_filterSlope)
        |
  [FilterB: Formant]    (drift_formantMorph, drift_formantMix)
        |
  [Fracture glitch]     (drift_fractureEnable, drift_fractureIntensity, drift_fractureRate)  ← NEW
        |
  [Prism Shimmer]       (drift_shimmerAmount, drift_shimmerTone)
        |
  [Amp Envelope × velocity]
        |
  [LFO amp modulation]
        |
  [TidalPulse × 1 - breathVal]  (drift_tidalDepth, drift_tidalRate)  ← NEW
        |
  [voice pan + stereo spread]
        |
[Engine-level mix of all voices]
        |
  [Level × soft limiter]
        |
  → outputCacheL/R  ← coupling reads from here (dry)
        |
  [Reverb (engine-level block pass)]  (drift_reverbMix, drift_reverbSize)  ← NEW
        |
  [Write to shared JUCE AudioBuffer]
```

---

## What Was Left Out (Deferred)

Per the Option B recommendation from `drift_fx_gap_analysis.md`, the following were intentionally deferred:

| Deferred Group | Params | Deferred to Round |
|---------------|--------|------------------|
| Chorus + Phaser | +11 params | Option B2 |
| Delay | +7 params | Option B3 |
| Filter A drive + Filter B reso+shift | +3 params | Option B4 |
| Filter envelope (separate ADSR) | +4 params | Option B5 |
| LFO 2 | +7 params | Option B6 |
| LFO 3 | +7 params | Option B7 |
| Mod matrix (8 slots × 3 params) | +24 params | Option B8 |

The mod matrix is the most architecturally significant deferred item. It requires a `ModSources` struct, a `ModMatrix` evaluator, and wiring all mod destinations — estimated 300+ lines of new adapter code. It is the correct long-term target but is properly its own multi-round effort.

---

## Coverage Improvement

| Metric | Before (38 params) | After (45 params) |
|--------|-------------------|------------------|
| Canonical params exposed | 38 | 45 (+7) |
| BREATHE macro has DSP | No | Yes (TidalPulse) |
| FRACTURE macro has DSP | No | Yes (DriftFracture) |
| Engine-level reverb | No | Yes (DriftReverb) |
| Migrated presets with dropped values | 1,353 | 1,353 (unchanged — values were lost at migration) |
| New presets can use Tidal/Fracture/Reverb | No | Yes |

The 1,353 dropped values from the Round 5E migration are not recoverable from the preset files (the original values were lost in migration). However, going forward, all new Drift presets authored against the XOlokun schema can use the 7 new parameters. The 165 natively-authored Drift presets require no changes. The 227 migrated presets default to off for all 7 new params (same sonic result as before, but now editable).

---

## Migration Note

To take advantage of the 7 new params for the 227 migrated presets, the recommended path (from the gap analysis) is **Option B-2**: run `Tools/fix_drift_presets.py` extended with the new param mappings against the original XOdyssey standalone preset library at `~/Documents/GitHub/XOdyssey/` (if the `.json` source files still have the original standalone values). The migration tool can match presets by name and populate `drift_tidalDepth/Rate`, `drift_fractureEnable/Intensity/Rate`, and `drift_reverbMix/Size` from the standalone `tidal_depth/rate`, `fracture_enable/intensity/rate`, and `reverb_mix/size` keys.

If the standalone preset files no longer have the original values, Option B-1 applies: all 7 new params stay at defaults (off) for migrated presets, and sound designers can manually author the new parameters going forward.

---

## Audio Thread Safety

All three new DSP classes satisfy the XOlokun architecture constraints:

- **DriftTidalPulse**: Phase counter only — zero heap allocation, no locks, no I/O
- **DriftFracture**: `std::array<float, 4096>` — stack-like fixed allocation in the struct, no heap, no locks
- **DriftReverb**: `std::vector` buffers pre-allocated in `prepare()` via `assign()`. `processStereo()` performs only arithmetic and index math — no allocation, no locks, no I/O. The `if (combSizes[0] == 0) return` guard protects against calls before `prepare()`.

---

## Files Modified

- `Source/Engines/Drift/DriftEngine.h` — primary change file

## Files Referenced (read-only)

- `Docs/drift_fx_gap_analysis.md` — analysis driving this pass
- `[external] XOdyssey/src/engine/TidalPulse.h` — source for DriftTidalPulse
- `[external] XOdyssey/src/engine/Fracture.h` — source for DriftFracture
- `[external] XOdyssey/src/engine/ReverbFX.h` — source for DriftReverb

---

## Next Steps

1. **Option B2** (Chorus + Phaser) — next highest musical impact after reverb
2. **Preset authoring** — write 5–10 new Drift presets demonstrating TidalPulse breathing and Fracture glitch
3. **Migration script** — extend `Tools/fix_drift_presets.py` with 7 new param mappings for the 227 migrated presets
4. **UI** — expose 7 new params in the DriftEngine panel (currently accessible via preset authoring / parameter ID, not visible in the UI)
