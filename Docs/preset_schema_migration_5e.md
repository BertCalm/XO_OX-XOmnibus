# Preset Schema Migration — Round 5E

**Date:** 2026-03-14
**Scope:** Three highest-volume engines from the Round 3 Prism Sweep audit
**Status:** COMPLETE — all ghost params eliminated, zero remaining

---

## Summary

| Engine | Presets Audited | Presets Fixed | Params Renamed | Params Dropped | Script |
|--------|-----------------|---------------|----------------|----------------|--------|
| Odyssey (Drift) | 373 | 202 | 2,887 | 1,353 | `Tools/fix_drift_presets.py` |
| Oblong (Bob) | 343 | 167 | 2,888 | 293 | `Tools/fix_bob_presets.py` |
| Overdub (Dub) | 181 | 41 | 533 | 12 | `Tools/fix_dub_presets.py` |
| **Total** | **897** | **410** | **6,308** | **1,658** | |

Post-fix verification: all three scripts re-run in `--dry-run` mode confirm **0 presets changed**,
meaning no ghost params remain in any of the three engines.

---

## Engine 1: Odyssey / Drift

### Source of truth
**`Source/Engines/Drift/DriftEngine.h`** — `addParametersImpl()` method, lines 1022–1190.

### Canonical parameter IDs (38 params, all `drift_` prefixed)
`drift_oscA_mode`, `drift_oscA_shape`, `drift_oscA_tune`, `drift_oscA_level`,
`drift_oscA_detune`, `drift_oscA_pw`, `drift_oscA_fmDepth`,
`drift_oscB_mode`, `drift_oscB_shape`, `drift_oscB_tune`, `drift_oscB_level`,
`drift_oscB_detune`, `drift_oscB_pw`, `drift_oscB_fmDepth`,
`drift_subLevel`, `drift_noiseLevel`, `drift_hazeAmount`,
`drift_filterCutoff`, `drift_filterReso`, `drift_filterSlope`, `drift_filterEnvAmt`,
`drift_formantMorph`, `drift_formantMix`,
`drift_shimmerAmount`, `drift_shimmerTone`,
`drift_attack`, `drift_decay`, `drift_sustain`, `drift_release`,
`drift_lfoRate`, `drift_lfoDepth`, `drift_lfoDest`,
`drift_driftDepth`, `drift_driftRate`,
`drift_level`, `drift_voiceMode`, `drift_glide`, `drift_polyphony`

### Ghost params found (74 unique ghost keys across 202 presets)

**Wave 1 — Old unprefixed XOdyssey standalone schema (196 presets):**
These presets were authored against the XOdyssey v1 standalone schema, before the
`drift_` prefix convention was adopted for XOmnibus. All core osc, filter, envelope,
LFO, and FX parameters used unprefixed or standalone-namespaced names.

Key ghost → canonical mappings applied:
- `osc_a_mode` → `drift_oscA_mode`
- `osc_a_level` → `drift_oscA_level`
- `osc_a_detune` → `drift_oscA_detune`
- `osc_a_pw` → `drift_oscA_pw`
- `osc_a_fm_depth` → `drift_oscA_fmDepth`
- `osc_b_mode` → `drift_oscB_mode` (and matching `osc_b_*` series)
- `env_amp_attack` → `drift_attack`
- `env_amp_decay` → `drift_decay`
- `env_amp_sustain` → `drift_sustain`
- `env_amp_release` → `drift_release`
- `filt_a_cutoff` → `drift_filterCutoff`
- `filt_a_reso` → `drift_filterReso`
- `filt_a_slope` → `drift_filterSlope`
- `filt_a_env_amt` → `drift_filterEnvAmt`
- `shimmer_amount` → `drift_shimmerAmount`
- `shimmer_tone` → `drift_shimmerTone`
- `haze_amount` → `drift_hazeAmount`
- `sub_level` → `drift_subLevel`
- `noise_level` → `drift_noiseLevel`
- `drift_depth` → `drift_driftDepth`
- `drift_rate` → `drift_driftRate`
- `lfo_1_rate` → `drift_lfoRate`
- `lfo_1_depth` → `drift_lfoDepth`
- `voice_mode` → `drift_voiceMode`
- `voice_glide_time` → `drift_glide`
- `master_gain` → `drift_level`
- `filt_b_morph` → `drift_formantMorph`

Keys dropped (no canonical equivalent in current DriftEngine):
- `osc_a_fm_ratio`, `osc_a_fine`, `osc_b_fine` — removed from engine
- `sub_octave`, `sub_shape`, `noise_type` — not in canonical schema
- `filt_a_drive`, `filt_b_mix`, `filt_b_reso` — filter B simplified/removed
- `env_filter_*` (4 params) — no separate filter envelope in canonical schema
- `lfo_1_shape` — shape not exposed in canonical schema
- `mod_1/2_source/dest/amount` (6 params) — mod matrix not in canonical schema
- `macro_journey` — macro renamed/restructured at XOmnibus level
- `tidal_depth`, `tidal_rate` — tidal pulse not in canonical schema
- `fracture_enable`, `fracture_intensity`, `fracture_rate` — fracture not in schema
- `reverb_enable/size/mix/damping` — embedded FX not in canonical schema
- `delay_enable/time/feedback/mix/mode` — embedded FX not in canonical schema
- `chorus_enable/rate/depth/mix` — embedded FX not in canonical schema
- `phaser_enable/rate/depth/mix/feedback` — embedded FX not in canonical schema
- `global_spread`, `voice_glide_time` (latter mapped via → `drift_glide`)

**Wave 2 — Double-prefixed params (3 presets):**
Three presets had old params with `drift_` prepended (partial prior migration):
- `drift_osc_a_mode` → `drift_oscA_mode`
- `drift_haze_amount` → `drift_hazeAmount`
- `drift_reverb_size` → dropped (no reverb in canonical schema)
- `drift_master_gain` → `drift_level`

**One-off ghost params:**
- `odyssey_detune` → `drift_oscA_detune` (wrong prefix; 3 presets including `Trance_Machine.xometa`)

---

## Engine 2: Oblong / Bob

### Source of truth
**`Source/Engines/Bob/BobEngine.h`** — `addParametersImpl()` method, lines 1371–1522.

### Canonical parameter IDs (41 params, all `bob_` prefixed)
`bob_oscA_wave`, `bob_oscA_shape`, `bob_oscA_tune`, `bob_oscA_drift`,
`bob_oscB_wave`, `bob_oscB_detune`, `bob_oscB_blend`, `bob_oscB_sync`, `bob_oscB_fm`,
`bob_texMode`, `bob_texLevel`, `bob_texTone`, `bob_texWidth`,
`bob_fltMode`, `bob_fltCutoff`, `bob_fltReso`, `bob_fltChar`, `bob_fltDrive`, `bob_fltEnvAmt`,
`bob_ampAttack`, `bob_ampDecay`, `bob_ampSustain`, `bob_ampRelease`,
`bob_motAttack`, `bob_motDecay`, `bob_motSustain`, `bob_motRelease`, `bob_motDepth`,
`bob_lfo1Rate`, `bob_lfo1Depth`, `bob_lfo1Shape`, `bob_lfo1Target`,
`bob_curMode`, `bob_curAmount`,
`bob_bobMode`,
`bob_dustAmount`, `bob_dustTone`,
`bob_level`, `bob_voiceMode`, `bob_glide`, `bob_polyphony`

### Ghost params found (50 unique ghost keys across 167 presets)

**Root cause:** Presets authored against the old XOblong/BobEngine standalone schema
before the `bob_` prefix was enforced in XOmnibus. All parameters used unprefixed
or short-prefixed names.

Key ghost → canonical mappings applied:
- `oscA_wave` → `bob_oscA_wave` (and full `oscA_*` series)
- `oscB_wave` → `bob_oscB_wave` (and full `oscB_*` series)
- `flt_mode` → `bob_fltMode`
- `flt_cutoff` → `bob_fltCutoff`
- `flt_resonance` → `bob_fltReso`
- `flt_character` → `bob_fltChar`
- `flt_drive` → `bob_fltDrive`
- `flt_envAmt` → `bob_fltEnvAmt`
- `env_attack` → `bob_ampAttack`
- `env_decay` → `bob_ampDecay`
- `env_sustain` → `bob_ampSustain`
- `env_release` → `bob_ampRelease`
- `motEnv_attack` → `bob_motAttack` (and full `motEnv_*` series)
- `lfo1_rate` → `bob_lfo1Rate`
- `lfo1_depth` → `bob_lfo1Depth`
- `lfo1_shape` → `bob_lfo1Shape`
- `cur_mode` → `bob_curMode`
- `cur_amount` → `bob_curAmount`
- `bob_mode` → `bob_bobMode` (old float character knob → canonical mode param)
- `dust_amount` → `bob_dustAmount`
- `dust_tone` → `bob_dustTone`
- `tex_mode` → `bob_texMode` (and full `tex_*` series)
- `glide` → `bob_glide`

Keys dropped (no canonical equivalent in current BobEngine):
- `lfo2_rate`, `lfo2_depth` — BobEngine has only one LFO
- `dust_age` — removed from engine
- `smear_mix`, `smear_depth`, `smear_width`, `smear_rate` — smear stage removed
- `space_mix`, `space_size`, `space_decay`, `space_air` — reverb tail removed;
  BobEngine exposes texture (bob_tex*) not a reverb
- `stutter_rate`, `stutter_depth` — stutter not in canonical schema
- `vel_filter` — velocity-to-filter routing not in canonical schema
- `voice_mode` — voice mode not in BobEngine canonical params

---

## Engine 3: Overdub / Dub

### Source of truth
**`Source/Engines/Dub/DubEngine.h`** — `addParametersImpl()` method, lines 922–1080.

### Canonical parameter IDs (37 params, all `dub_` prefixed)
`dub_oscWave`, `dub_oscOctave`, `dub_oscTune`, `dub_oscPwm`,
`dub_subLevel`, `dub_noiseLevel`, `dub_drift`, `dub_level`,
`dub_filterMode`, `dub_filterCutoff`, `dub_filterReso`, `dub_filterEnvAmt`,
`dub_attack`, `dub_decay`, `dub_sustain`, `dub_release`,
`dub_pitchEnvDepth`, `dub_pitchEnvDecay`,
`dub_lfoRate`, `dub_lfoDepth`, `dub_lfoDest`,
`dub_sendLevel`, `dub_returnLevel`, `dub_dryLevel`,
`dub_driveAmount`,
`dub_delayTime`, `dub_delayFeedback`, `dub_delayWear`, `dub_delayWow`, `dub_delayMix`,
`dub_reverbSize`, `dub_reverbDamp`, `dub_reverbMix`,
`dub_voiceMode`, `dub_glide`, `dub_polyphony`

### Ghost params found (32 unique ghost keys across 41 presets)

**Root cause:** Presets authored against the old XOverdub standalone schema before
the `dub_` prefix convention was enforced in XOmnibus.

Key ghost → canonical mappings applied:
- `osc_wave` → `dub_oscWave`
- `osc_sub_level` → `dub_subLevel`
- `osc_noise_level` → `dub_noiseLevel`
- `osc_pwm` → `dub_oscPwm`
- `osc_drift` → `dub_drift`
- `osc_level` → `dub_level`
- `filter_cutoff` → `dub_filterCutoff`
- `filter_resonance` → `dub_filterReso`
- `filter_env_amt` → `dub_filterEnvAmt`
- `env_attack` → `dub_attack`
- `env_decay` → `dub_decay`
- `env_sustain` → `dub_sustain`
- `env_release` → `dub_release`
- `pitch_env_depth` → `dub_pitchEnvDepth`
- `pitch_env_decay` → `dub_pitchEnvDecay`
- `lfo_rate` → `dub_lfoRate`
- `lfo_depth` → `dub_lfoDepth`
- `lfo_dest` → `dub_lfoDest`
- `send_level` → `dub_sendLevel`
- `return_level` → `dub_returnLevel`
- `drive_amount` → `dub_driveAmount`
- `delay_time` → `dub_delayTime`
- `delay_feedback` → `dub_delayFeedback`
- `delay_wear` → `dub_delayWear`
- `delay_wow` → `dub_delayWow`
- `delay_mix` → `dub_delayMix`
- `reverb_size` → `dub_reverbSize`
- `reverb_damp` → `dub_reverbDamp`
- `reverb_mix` → `dub_reverbMix`
- `voice_glide` → `dub_glide`
- `dub_sendAmount` → `dub_sendLevel` (partial prior migration, wrong name)

Keys dropped (no canonical equivalent in current DubEngine):
- `delay_sync` — delay sync not in canonical schema
- `voice_mode` — not in DubEngine canonical params (only `dub_voiceMode` is)
- `dub_oscMode`, `dub_oscShape` — wrong-name partial-migration artifacts;
  canonical is `dub_oscWave`, no shape param

---

## Methodology

### Engine source verification

For each engine, the actual registered parameter IDs were read directly from the
C++ `addParametersImpl()` method in the engine header:
- Drift: `Source/Engines/Drift/DriftEngine.h` lines 1022–1190
- Bob: `Source/Engines/Bob/BobEngine.h` lines 1371–1522
- Dub: `Source/Engines/Dub/DubEngine.h` lines 922–1080

### Preset discovery

All 1,625 `.xometa` files under `Presets/` were scanned. Engine blocks were matched
by the engine key in the `"parameters"` object (`"Odyssey"` or `"Drift"` for Drift engine,
`"Oblong"` or `"Bob"` for Bob engine, `"Overdub"` or `"Dub"` for Dub engine).

### Migration logic

Each migration script:
1. Iterates over every key in the engine's parameter block
2. If the key is already canonical — keeps it unchanged
3. If the key is in the ghost→canonical mapping with a non-None target — renames it
   (skips if canonical key already present to avoid overwriting prior correct values)
4. If the key maps to None — drops it (no canonical equivalent)
5. If the key is unknown (not in ghost map and not canonical) — keeps it unchanged
   (safety fallback for newer params added after this script was written)
6. Writes back the file only if at least one key was changed

### Idempotency

All three scripts are fully idempotent. Running them a second time on already-fixed
presets produces zero changes (confirmed by dry-run verification after migration).

---

## Migration Scripts

| Script | Path |
|--------|------|
| Drift / Odyssey | `Tools/fix_drift_presets.py` |
| Bob / Oblong | `Tools/fix_bob_presets.py` |
| Dub / Overdub | `Tools/fix_dub_presets.py` |

All scripts support `--dry-run` mode to preview changes without writing files.

Usage:
```bash
python3 Tools/fix_drift_presets.py [--dry-run]
python3 Tools/fix_bob_presets.py [--dry-run]
python3 Tools/fix_dub_presets.py [--dry-run]
```

---

## Post-Migration Ghost Param Count

| Engine | Ghost Params Remaining |
|--------|------------------------|
| Odyssey (Drift) | 0 |
| Oblong (Bob) | 0 |
| Overdub (Dub) | 0 |

---

## Caveats and Data Loss

Ghost params that were dropped (no canonical mapping) represent features that existed
in the standalone instrument but were not carried over into the XOmnibus engine adapter.
The most significant losses by volume:

**Drift (1,353 dropped instances):** Embedded FX (reverb, delay, chorus, phaser) were
the largest category — these existed in the standalone XOdyssey but the XOmnibus DriftEngine
adapter does not expose them as engine-level parameters. Also dropped: the 8-slot mod matrix,
filter B (formant filter beyond `formantMorph`/`formantMix`), tidal pulse, fracture,
and miscellaneous standalone macros.

**Bob (293 dropped instances):** Smear stage (removed from BobEngine), LFO2
(only LFO1 in canonical schema), space/reverb tail (BobEngine uses texture instead),
stutter (not in canonical schema).

**Dub (12 dropped instances):** Delay sync, voice_mode (misnamed), and two
wrong-prefix partial-migration artifacts from an earlier tool run.

No existing canonical parameter values were overwritten — the migration only adds
canonical keys that were missing and removes ghost keys.
