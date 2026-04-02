# Drift Adapter FX Gap Analysis

**Date:** 2026-03-14
**Scope:** 1,353 dropped parameters from Drift/XOdyssey standalone preset migration
**Status:** Analysis complete — recommendation: Targeted Partial Exposure (Option B)

---

## Background

Round 5E (`Docs/preset_schema_migration_5e.md`) migrated 202 Drift/Odyssey presets from the
standalone XOdyssey schema to the XOceanus `drift_` prefix schema. During migration, 1,353
parameter instances were dropped because the DriftEngine adapter does not expose them.

A post-migration audit reveals that 227 of the 392 Drift presets in the library have fewer than
the full 38 canonical params — meaning they arrived with a partial complement (the migrated
presets whose missing params defaulted to the engine's init values). The 165 "clean" presets
were authored natively against the XOceanus schema.

---

## Step 1: What the Adapter Currently Exposes

The DriftEngine adapter (`Source/Engines/Drift/DriftEngine.h`) defines 38 canonical parameters,
all `drift_` prefixed:

**Voice / Oscillators (14 params):**
`drift_oscA_mode`, `drift_oscA_shape`, `drift_oscA_tune`, `drift_oscA_level`,
`drift_oscA_detune`, `drift_oscA_pw`, `drift_oscA_fmDepth`,
`drift_oscB_mode`, `drift_oscB_shape`, `drift_oscB_tune`, `drift_oscB_level`,
`drift_oscB_detune`, `drift_oscB_pw`, `drift_oscB_fmDepth`

**Mix (2 params):**
`drift_subLevel`, `drift_noiseLevel`

**Character stages (3 params):**
`drift_hazeAmount`, `drift_shimmerAmount`, `drift_shimmerTone`

**Filter A — LP (4 params):**
`drift_filterCutoff`, `drift_filterReso`, `drift_filterSlope`, `drift_filterEnvAmt`

**Filter B — Formant (2 params):**
`drift_formantMorph`, `drift_formantMix`

**Amp envelope (4 params):**
`drift_attack`, `drift_decay`, `drift_sustain`, `drift_release`

**Single LFO (3 params):**
`drift_lfoRate`, `drift_lfoDepth`, `drift_lfoDest`

**Voyager Drift (2 params):**
`drift_driftDepth`, `drift_driftRate`

**Global (4 params):**
`drift_level`, `drift_voiceMode`, `drift_glide`, `drift_polyphony`

**Audio routing:** DriftEngine calls `buffer.addSample()` directly — it adds its rendered audio
to the shared mix buffer. It does **not** own a private FX chain. Its audio passes through
the XOceanus MasterFXChain after all engines have contributed to the mix.

**Coupling outputs:** envelope level (channel 2), audio L/R (channels 0/1)
**Coupling inputs:** `AmpToFilter`, `AmpToPitch`, `LFOToPitch`, `PitchToPitch`, `EnvToMorph`

---

## Step 2: What Was Dropped — The 1,353

The standalone XOdyssey (`~/Documents/GitHub/XOdyssey/`) has a richer schema with 130+
canonical parameters. The dropped categories, by volume:

### Category 1: Embedded FX Chain (~22 params, highest volume dropped)

The standalone has a 4-unit FX chain in the signal path after the voice pool:

| Module | Params | Standalone File |
|--------|--------|----------------|
| Chorus | `chorus_enable`, `chorus_mode`, `chorus_rate`, `chorus_depth`, `chorus_mix` | `engine/ChorusFX.h` |
| Phaser | `phaser_enable`, `phaser_mode`, `phaser_rate`, `phaser_depth`, `phaser_feedback`, `phaser_mix` | `engine/PhaserFX.h` |
| Delay | `delay_enable`, `delay_mode`, `delay_time`, `delay_sync`, `delay_sync_rate`, `delay_feedback`, `delay_mix` | `engine/DelayFX.h` |
| Reverb | `reverb_enable`, `reverb_mode`, `reverb_size`, `reverb_damping`, `reverb_pre_delay`, `reverb_mix` | `engine/ReverbFX.h` |

**Musical impact:** In presets where these were active (most of the 283 Drift presets authored
in the standalone), they defined the spatial and textural character. A pad preset with
`reverb_size=0.8, reverb_mix=0.4` becomes a dry, close-mic sound in XOceanus. A preset with
`chorus_depth=0.7, chorus_mix=0.5` loses its motion. These are not small differences —
reverb and chorus are load-bearing for most pad sounds.

### Category 2: 8-Slot Mod Matrix (24 params, extremely high volume)

`mod_1_source` through `mod_8_source/dest/amount` — 8 slots × 3 params each.

**Musical impact:** The mod matrix is what makes XOdyssey presets *evolve*. Routing LFO2 to
`filt_b_morph`, envelope to `shimmer_amount`, and velocity to `osc_a_detune` creates the
"alive pad" character. Without these routings, all modulation collapses to:
- The single `drift_lfoRate/lfoDepth/lfoDest` LFO
- Hardwired Voyager Drift
- External coupling inputs only

### Category 3: Signature Trait Params — Tidal and Fracture (5 params)

`tidal_depth`, `tidal_rate` — the BREATHE macro's engine. TidalPulse is a sin² LFO that
modulates amplitude organically (breathing rather than oscillating). Dropped from adapter.

`fracture_enable`, `fracture_intensity`, `fracture_rate` — the FRACTURE macro's engine.
A circular-buffer stutter glitch that creates controlled disintegration. Dropped from adapter.

**Musical impact:** Tidal Pulse is specifically what differentiates XOdyssey's movement from
a standard LFO sweep. Its sin² shape has soft peaks and gentle valleys, while a sine LFO
has equal positive and negative excursion. Without it, the BREATHE macro has no DSP behind it.
Fracture is the single most distinctive sound design tool — buffer stutter on a pad is a
signature texture. Both are named Signature Traits in the XOdyssey spec.

### Category 4: Filter B Extensions (3 params)

`filt_b_mix`, `filt_b_reso`, `filt_b_shift` — the adapter exposes `drift_formantMorph` and
`drift_formantMix` but not resonance control or pitch shift.

**Musical impact:** Without resonance control, the formant filter cannot be set to a
self-resonant vowel peak — a key sound design technique for vocal leads.

### Category 5: Second and Third LFOs (14 params)

`lfo_2_rate/shape/sync/sync_rate/depth/phase/retrigger` and the same for LFO 3.

**Musical impact:** Most XOdyssey presets using the mod matrix had LFO1→filter, LFO2→pitch,
LFO3→shimmer as a common three-LFO stack. The adapter collapses all three into a single LFO.

### Category 6: Filter Envelope (4 params)

`env_filter_attack/decay/sustain/release` — separate filter envelope beyond filterEnvAmt.

### Category 7: Miscellaneous — Medium Musical Impact

`filt_a_drive` (pre-filter saturation), `global_spread` (stereo width), `osc_a_fm_ratio`,
`osc_b_fine`, `osc_a_fine` (per-cent detuning), `sub_octave`, `sub_shape`, `noise_type`,
`cross_fm_depth`, `cross_fm_ratio` (B→A cross-modulation).

### Category 8: Climax System and Macros

`macro_journey`, `macro_breathe`, `macro_bloom`, `macro_fracture` — these exist at the
XOceanus level as the CHARACTER/MOVEMENT/COUPLING/SPACE macros, but the per-preset macro
mappings (which params JOURNEY modulates, by how much, with what curve) were the dropped
content. The Climax system (`climaxThreshold`, `bloomTime`) was added as top-level schema
fields in the Journey Demo presets, so this is separately handled.

---

## Step 3: Assessment of the Four Options

### Option A — Full Exposure (Map all ~130 standalone params)

**What this means:** Add ~92 new `drift_` parameters to `addParametersImpl()`, instantiate
the standalone FX modules in DriftEngine (ChorusFX, PhaserFX, DelayFX, ReverbFX, TidalPulse,
Fracture, ModMatrix, 3 LFOs, filter envelope), wire them into `renderBlock()`, and write a
re-migration script to restore all 1,353 dropped values.

**Engineering effort:** Very high. The standalone FX classes are header-only and self-contained;
they can be directly included in DriftEngine.h without JUCE dependencies. The DSP porting work
is already done — these files exist in `~/Documents/GitHub/XOdyssey/src/engine/`. The blocker
is wiring them into the parameter system and renderBlock() (estimated 500-800 lines of new
adapter code) and ensuring no heap allocation in renderBlock() (the reverb and delay use
`std::vector` — these would need pre-allocation in `prepare()`). The mod matrix requires a
full ModSources/ModDst machinery that doesn't currently exist in the adapter.

**Preset compatibility:** The 202 migrated presets would need a re-migration pass to restore
the dropped values. This requires a new migration script that reads from an external source of
ground truth (the original standalone `.xometa` files, if they still exist) — since the dropped
values are gone from the current preset files. If the original standalone presets were not
separately archived, the dropped values are permanently lost and new defaults would apply.

**Musical benefit:** Full. Every preset would sound as intended in the standalone. The Climax
system's bloom could include reverb and chorus modulation (as the `v007_journey_demo_report.md`
describes: "bloom modulates shimmer, reverb size, chorus depth, formant mix, and drift depth
simultaneously"). Mod matrix routings would be restored.

**Architectural cleanliness:** This is the architecturally correct end state. The SynthEngine
interface does not prohibit engines from having internal FX chains — DubEngine already has an
embedded delay and reverb (`dub_delayTime`, `dub_reverbSize`). Full exposure brings Drift to
the same level of completeness as Dub.

**Verdict:** Correct direction, prohibitive scope for a single pass. The mod matrix alone is a
300+ line addition. Best treated as a multi-round incremental effort, not a single fix.

---

### Option B — Partial Exposure (Expose the highest-priority 5-8 params)

**What this means:** Select the dropped params with the highest musical impact and add only
those to the adapter. The mod matrix stays as-is; general LFO stays as a single LFO; the full
FX chain is not added. Specific Signature Traits (Tidal, Fracture) and a minimal reverb/delay
are added.

**Engineering effort:** Medium. Each new param is a new entry in `addParametersImpl()`, a new
cached pointer in `attachParameters()`, a new DSP module instantiated in `DriftVoice` or
`DriftEngine`, and wiring in `renderBlock()`.

**Preset compatibility:** Does not help the 1,353 dropped values — those are still gone from
the preset files. However, going forward, new presets can use these params. The 165 natively-
authored presets need no changes; the 227 migrated presets still sound at-default for the
newly-added params (same as today, but now they could be manually authored).

**Musical benefit:** Targeted. The most distinctive XOdyssey sounds (Tidal Pulse breathing,
Fracture glitch, basic reverb tail for spatial depth) are restored. The mod matrix complexity
and 3-LFO system are deferred.

**Architectural cleanliness:** Clean. The SynthEngine interface supports this approach — any
engine can expose whatever parameters are musically meaningful without requiring full parity.

**Verdict:** Recommended for immediate execution. See priority list below.

---

### Option C — XOceanus Routing (Route through shared MasterFXChain)

**What this means:** Remove the standalone FX entirely from consideration; use the shared
18-stage MasterFXChain for reverb, delay, and modulation. No new Drift parameters; instead,
Drift presets encode their spatial needs in the top-level `master_` parameter block.

**Engineering effort:** Zero for the adapter. Medium for preset authoring (each preset needs
updated `master_` params).

**Preset compatibility:** Not compatible. The 1,353 dropped values were engine-private
parameters specifying an embedded FX chain position in the standalone signal flow:
`OscA → Filter → [Fracture] → [Prism] → AmpEnv → [Chorus → Phaser → Delay → Reverb]`.
Routing through MasterFXChain applies the same reverb and delay to all active engines — a
choir of Bob+Drift+Dub all sharing the same spatial envelope. Many presets that worked
because Drift had 40% reverb while Bob had 0% would now both share whatever the master
reverb is set to.

**Musical benefit:** None for the affected presets. The MasterFXChain exists for inter-engine
space and coloring, not for replacing engine-private FX that are calibrated to specific sounds.

**Architectural issue:** Violates the expectation that a preset fully describes its sound in
its parameters block. A Drift preset stored in `Presets/Drift/` with no `master_` params would
sound dry in any context — currently true, but worse if the standing answer is "adjust master
reverb manually."

**Verdict:** Not viable for the stated problem. MasterFXChain supplements engine FX; it does
not replace them.

---

### Option D — Accept the Gap (Document as known limitation)

**What this means:** The 1,353 drops are a one-time migration artifact. Since the preset values
are gone, they cannot be restored without re-authoring. Accept that the migrated presets will
sound somewhat dry and without modulation compared to their original standalone forms. Document
the known differences and move on.

**Engineering effort:** Zero.

**Preset compatibility:** Status quo — 227 presets with partial param sets, 165 fully canonical.

**Musical benefit:** None. Some of the most interesting XOdyssey presets (those that used the
mod matrix for evolving textures, or embedded reverb for spatial depth) will continue to sound
undersized in XOceanus.

**Architectural cleanliness:** Creates a permanent two-tier library: 165 correctly-authored
presets and 227 presets that default-fill most of their synthesis parameters. This is not
technically broken — the engine will run with partial params, using init values for the gaps —
but it means a significant portion of the library never sounds as designed.

**Verdict:** Defensible only if the migration presets are treated as a scaffold to be replaced
by freshly-authored XOceanus-native presets. If that is the intent, document it explicitly and
set a target for replacement.

---

## Step 4: Recommendation

**Adopt Option B (Targeted Partial Exposure) as the immediate pass, with Option A as a
multi-round roadmap target.**

The specific reason Option A is not done in one pass is the mod matrix. The 8-slot ModMatrix
is the most complex dropped item and the one that would require the most adapter redesign.
The FX chain (Chorus, Phaser, Delay, Reverb) and the Signature Traits (Tidal, Fracture) are
relatively self-contained and can be added incrementally without requiring a mod matrix.

### Priority 8 for Immediate Partial Exposure

These 8 params are recommended for the next DriftEngine pass, in order of musical impact:

| Priority | Param IDs (new) | Musical Reason |
|----------|----------------|----------------|
| 1 | `drift_tidalDepth`, `drift_tidalRate` | TidalPulse IS the BREATHE macro's engine. Without it, BREATHE has no DSP. The BREATHE macro is one of ODYSSEY's 4 macros — it must modulate something. These 2 params are the smallest addition with the largest expressive gap closed. |
| 2 | `drift_fractureEnable`, `drift_fractureIntensity`, `drift_fractureRate` | Fracture is a named Signature Trait. The FRACTURE macro currently has no DSP backing in the adapter. 3 params close this entire gap. The `Fracture` class in XOdyssey is already ported (a buffer-stutter engine with a 4096-sample static buffer — safe for real-time). |
| 3 | `drift_reverbMix`, `drift_reverbSize` | Two params to give Drift presets basic spatial depth. The ReverbFX class is already ported. Without these, pads and leads authored with 30-50% reverb sound unnervingly close. This is the highest-volume musical difference heard by a player loading a migrated preset. |

**Total: 7 new params** — from 38 to 45 canonical Drift parameters.

### Implementation Note for Partial Exposure

The Fracture class uses a fixed 4096-sample array (`std::array<float, 4096>`) — no heap
allocation. Tidal Pulse has no state beyond a phase counter — trivial.

ReverbFX uses `std::vector` for comb and allpass buffers — these must be pre-allocated in
`prepare()` and not re-allocated in `renderBlock()`. The current ReverbFX implementation in
`~/Documents/GitHub/XOdyssey/src/engine/ReverbFX.h` uses `vector::assign()` in `prepare()`,
which is correct — allocation happens once, not per block.

### Migration Path for the 202 Affected Presets

For the 7 newly-exposed params, the original standalone values were dropped and are no longer
in the preset files. Migration options:

**Option B-1: Re-author from known defaults.** For `drift_tidalDepth/Rate`, set to 0.0 (off)
— the standalone default for `tidal_depth` was 0.0. Most presets that weren't Tidal-heavy will
be fine. For `drift_fractureEnable`, set to false (standalone default). For `drift_reverbMix`
and `drift_reverbSize`, set to 0.0 — again the off position. This produces presets that sound
the same as today but can now be edited to add Tidal/Fracture/Reverb.

**Option B-2: Re-migrate from original XOdyssey presets.** If the standalone XOdyssey preset
library (`~/Documents/GitHub/XOdyssey/`) still has `.json` or `.xometa` files with the
original values, run a new migration script that reads from the standalone files and populates
the 7 new params with their original values. This is the preferred path for the ~283 presets
that were authored in the standalone. It requires matching by preset name (possible since many
preset names are preserved).

The migration tool path: `Tools/fix_drift_presets.py` (already exists for the Round 5E
migration) can be extended with the 7 new param mappings and re-run.

---

## Option A Roadmap (Multi-Round Full Exposure)

If Option A is pursued in subsequent rounds, the recommended sequence:

| Round | Addition | New Params | Notes |
|-------|----------|-----------|-------|
| B (now) | Tidal, Fracture, Reverb | +7 | Highest musical impact per effort |
| B2 | Chorus + Phaser | +11 | Spatial FX complete. DriftEngine now matches Dub in FX depth |
| B3 | Delay | +7 | Full standalone FX parity |
| B4 | Filter A drive, Filter B reso+shift | +3 | Filter parity |
| B5 | Filter envelope | +4 | Separate filter ADSR |
| B6 | LFO 2 | +7 | Second LFO; most common mod source |
| B7 | LFO 3 | +7 | Third LFO; full mod source parity |
| B8 | Mod matrix (8 slots) | +24 | The hardest round; requires ModSources infrastructure |

Total to full parity: ~70 new params across 8 rounds. At 7 params per pass, this is a
multi-quarter effort. Each round leaves the preset system in a valid state and can be released
independently.

---

## Summary

| Option | Effort | Preset Restore | Musical Impact | Verdict |
|--------|--------|---------------|----------------|---------|
| A Full exposure | Very High | Possible with re-migration | Complete | Correct long-term, too large now |
| B Partial (7 params) | Low | Defaults-only | High (Tidal, Fracture, Reverb) | **Recommended now** |
| C XOceanus routing | Zero | None | None | Not viable |
| D Accept the gap | Zero | None | None | Viable only if presets are re-authored |

**The architectural debt is real.** Drift is the only engine in the fleet where four named
Signature Traits (Tidal Pulse, Fracture, and the Climax's modulation targets — reverb and
chorus) are declared in the standalone but absent from the adapter. The standalone XOdyssey's
signal flow is:

```
Voice pool → Bass Integrity HPF → [Chorus → Phaser → Delay → Reverb] → Master Out
```

The adapter's signal flow is:

```
Voice pool → buffer.addSample() → (XOceanus MasterFXChain)
```

Every waveform reaches the shared mix dry. The 7-param Option B pass closes the most audible
half of that gap in a single afternoon's work.

---

## Files Referenced

- `Source/Engines/Drift/DriftEngine.h` — current adapter (38 params, lines 1022–1190)
- `~/Documents/GitHub/XOdyssey/src/Parameters.h` — standalone canonical param list (~130 params)
- `~/Documents/GitHub/XOdyssey/src/engine/TidalPulse.h` — Tidal implementation (ready to port)
- `~/Documents/GitHub/XOdyssey/src/engine/Fracture.h` — Fracture implementation (ready to port)
- `~/Documents/GitHub/XOdyssey/src/engine/ReverbFX.h` — Reverb implementation (needs pre-alloc check)
- `~/Documents/GitHub/XOdyssey/src/engine/ChorusFX.h` — Chorus (deferred to Option B2)
- `~/Documents/GitHub/XOdyssey/src/engine/PhaserFX.h` — Phaser (deferred to Option B2)
- `~/Documents/GitHub/XOdyssey/src/engine/DelayFX.h` — Delay (deferred to Option B3)
- `~/Documents/GitHub/XOdyssey/src/engine/ModMatrix.h` — ModMatrix (deferred to Option B8)
- `Docs/preset_schema_migration_5e.md` — Round 5E migration details and dropped param catalog
- `Tools/fix_drift_presets.py` — migration script to extend for new params
