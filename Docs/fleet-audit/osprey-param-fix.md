# Osprey Parameter Schema Fix — Fleet Audit

**Date:** 2026-03-21
**Engine:** OSPREY (`osprey_` prefix)
**Status:** COMPLETE — all 84 affected presets migrated

---

## Summary

84 of 166 Osprey presets contained legacy parameter keys that the current engine does not
register with APVTS. These presets loaded as init patches because JUCE silently ignores
unknown parameter IDs. All 84 have been migrated to canonical parameters. Zero legacy keys
remain across the fleet.

---

## Canonical Parameter Set (Current Engine)

Extracted from `Source/Engines/Osprey/OspreyEngine.h` `addParametersImpl()`:

| Parameter ID | Range | Default | Description |
|---|---|---|---|
| `osprey_shore` | 0–4 | 0.0 | Shore type (5 coastal biomes) |
| `osprey_seaState` | 0–1 | 0.2 | Sea state / main LFO rate |
| `osprey_swellPeriod` | 0.5–30s | 8.0 | Swell period |
| `osprey_windDir` | 0–1 | 0.5 | Wind direction |
| `osprey_depth` | 0–1 | 0.3 | Water depth character |
| `osprey_resonatorBright` | 0–1 | 0.5 | Resonator brightness |
| `osprey_resonatorDecay` | 0.01–8s | 1.0 | Resonator decay |
| `osprey_sympathyAmount` | 0–1 | 0.3 | Sympathetic resonance amount |
| `osprey_creatureRate` | 0–1 | 0.2 | Creature animation rate |
| `osprey_creatureDepth` | 0–1 | 0.3 | Creature modulation depth |
| `osprey_coherence` | 0–1 | 0.7 | Spectral coherence / timbral character |
| `osprey_foam` | 0–1 | 0.0 | Foam texture density |
| `osprey_brine` | 0–1 | 0.0 | Brine harmonic character |
| `osprey_hull` | 0–1 | 0.2 | Hull low-frequency warmth |
| `osprey_filterTilt` | 0–1 | 0.5 | Filter tilt (LP/HP spectral tilt) |
| `osprey_filterEnvDepth` | 0–1 | 0.25 | Velocity → filter tilt sweep |
| `osprey_harborVerb` | 0–1 | 0.2 | Harbor reverb mix |
| `osprey_fog` | 0–1 | 0.1 | Fog / delay texture mix |
| `osprey_ampAttack` | 0–8s | 0.5 | Amp envelope attack |
| `osprey_ampDecay` | 0.05–8s | 1.0 | Amp envelope decay |
| `osprey_ampSustain` | 0–1 | 0.7 | Amp envelope sustain |
| `osprey_ampRelease` | 0.05–12s | 2.0 | Amp envelope release |
| `osprey_voiceMode` | 0/1/2 | 0 | Voice mode (Poly/Mono/Legato) |
| `osprey_glide` | 0–2s | 0.0 | Portamento time |
| `osprey_macroCharacter` | 0–1 | 0.0 | Macro: CHARACTER |
| `osprey_macroMovement` | 0–1 | 0.0 | Macro: MOVEMENT |
| `osprey_macroCoupling` | 0–1 | 0.0 | Macro: COUPLING |
| `osprey_macroSpace` | 0–1 | 0.0 | Macro: SPACE |
| `osprey_lfo1Shape` | 0–4 | 0.0 | LFO1 shape (Sine/Tri/Saw/Square/S&H) |
| `osprey_lfo2Rate` | 0.01–8Hz | 0.2 | LFO2 rate |
| `osprey_lfo2Depth` | 0–1 | 0.0 | LFO2 depth |
| `osprey_lfo2Shape` | 0–4 | 1.0 | LFO2 shape |

---

## Legacy Parameter Groups Found

### Group A — Macro Rename Variants (3 schemas, 100 presets)

| Legacy Key | Count | → Canonical Key | Notes |
|---|---|---|---|
| `macro_character` | 25 | `osprey_macroCharacter` | Missing engine prefix |
| `macro_movement` | 25 | `osprey_macroMovement` | Missing engine prefix |
| `macro_coupling` | 25 | `osprey_macroCoupling` | Missing engine prefix |
| `macro_space` | 25 | `osprey_macroSpace` | Missing engine prefix |
| `osp_macro_character` | 2 | `osprey_macroCharacter` | Wrong prefix |
| `osp_macro_movement` | 2 | `osprey_macroMovement` | Wrong prefix |
| `osp_macro_coupling` | 2 | `osprey_macroCoupling` | Wrong prefix |
| `osp_macro_space` | 2 | `osprey_macroSpace` | Wrong prefix |
| `osprey_macro1` | 6 | `osprey_macroCharacter` | Numbered → named |
| `osprey_macro2` | 6 | `osprey_macroMovement` | Numbered → named |
| `osprey_macro3` | 6 | `osprey_macroCoupling` | Numbered → named |
| `osprey_macro4` | 6 | `osprey_macroSpace` | Numbered → named |

### Group B — Amp Envelope Variants

| Legacy Key | Count | → Canonical Key | Notes |
|---|---|---|---|
| `osprey_attack` | 6 | `osprey_ampAttack` | Missing `amp` segment |
| `osp_ampAttack` | 5 | `osprey_ampAttack` | Wrong prefix; 2 files had ms values (÷1000 applied) |
| `osp_ampDecay` | 2 | `osprey_ampDecay` | Wrong prefix; ms values (÷1000 applied) |
| `osp_ampRelease` | 5 | `osprey_ampRelease` | Wrong prefix; 2 files had ms values (÷1000 applied) |
| `osp_ampSustain` | 2 | `osprey_ampSustain` | Wrong prefix; 0–1 safe |
| `amp_attack` | 1 | `osprey_ampAttack` | Missing engine prefix |
| `amp_release` | 1 | `osprey_ampRelease` | Missing engine prefix |

**Note on ms vs. s detection:** `osp_ampAttack` values > 10 were treated as milliseconds
(Osprey_Nocturne: 544.7ms → 0.5447s; Nighthawk_Glide: 523.2ms → 0.5232s). Values ≤ 10
were treated as seconds (Dive Vector: 0.03s unchanged).

### Group C — Filter Params

| Legacy Key | Count | → Canonical Key | Notes |
|---|---|---|---|
| `osprey_filterCutoff` | 25 | `osprey_filterTilt` | Hz values log-normalized (20–20kHz → 0–1) |
| `osp_filterCutoff` | 5 | `osprey_filterTilt` | Mixed: Hz values normalized, 0–1 values kept |
| `filter_cutoff` | 1 | `osprey_filterTilt` | Already normalized (0–1) |
| `osprey_filterReso` | 10 | DROPPED | No canonical resonance param; engine uses tilt filter |
| `osp_filterReso` | 5 | DROPPED | No canonical resonance param |
| `osprey_filterRes` | 1 | DROPPED | No canonical resonance param |
| `filter_resonance` | 1 | DROPPED | No canonical resonance param |

**Normalization used for Hz → filterTilt:**
`tilt = (log(hz) - log(20)) / (log(20000) - log(20))`

Example mappings: 5000Hz → 0.799, 16440Hz → 0.972, 2800Hz → 0.715

### Group D — Reverb / Wet Mix

| Legacy Key | Count | → Canonical Key | Notes |
|---|---|---|---|
| `osprey_reverbMix` | 17 | `osprey_harborVerb` | Direct 0–1 rename |
| `osp_reverbMix` | 2 | `osprey_harborVerb` | Direct 0–1 rename |
| `reverb_mix` | 1 | `osprey_harborVerb` | Missing engine prefix |
| `osprey_wetMix` | 3 | `osprey_harborVerb` | Overall wet mix → reverb |
| `reverb_size` | 1 | `osprey_resonatorDecay` | 0–1 × 8 = decay in seconds |

### Group E — Brightness / Tonal Character

| Legacy Key | Count | → Canonical Key | Notes |
|---|---|---|---|
| `osprey_brightness` | 5 | `osprey_resonatorBright` | Direct 0–1 rename |

### Group F — LFO Rate / Depth

| Legacy Key | Count | → Canonical Key | Notes |
|---|---|---|---|
| `osprey_lfoRate` | 6 | `osprey_seaState` | LFO rate → sea state (0–1) |
| `osp_lfoRate` | 2 | `osprey_seaState` | Wrong prefix |
| `osprey_lfo1Rate` | 1 | `osprey_seaState` | Explicit LFO1 rate |
| `osp_lfoDepth` | 2 | `osprey_depth` | LFO depth → depth character |
| `osprey_lfo1Depth` | 1 | `osprey_depth` | Explicit LFO1 depth |

### Group G — Shore / Coastline

| Legacy Key | Count | → Canonical Key | Notes |
|---|---|---|---|
| `osprey_shoreBlend` | 14 | `osprey_shore` | × 4.0 (0–1 blend → 0–4 shore type) |
| `osprey_coastlineIndex` | 2 | `osprey_shore` | × 2.0 (0–2 index → 0–4 shore type) |

### Group H — Semantic Descriptor → Closest Canonical

| Legacy Key | Count | → Canonical Key | Semantic rationale |
|---|---|---|---|
| `osprey_space` | 5 | `osprey_harborVerb` | Spatial depth → reverb |
| `osp_space` | 2 | `osprey_harborVerb` | Same |
| `osprey_character` | 3 | `osprey_coherence` | Timbral character → coherence |
| `osp_character` | 2 | `osprey_coherence` | Same |
| `osprey_movement` | 5 | `osprey_creatureRate` | Animated movement → creature rate |
| `osp_movement` | 2 | `osprey_creatureRate` | Same |
| `osprey_warmth` | 2 | `osprey_hull` | Low-freq warmth → hull |
| `osprey_density` | 2 | `osprey_foam` | Texture density → foam |
| `density` | 1 | `osprey_foam` | Missing prefix |
| `osprey_aggression` | 2 | `osprey_brine` | Harmonic aggression → brine |
| `drive` | 1 | `osprey_brine` | Saturation drive → brine |

### Group I — Delay → Fog

| Legacy Key | Count | → Canonical Key | Notes |
|---|---|---|---|
| `osprey_delayFeedback` | 3 | `osprey_fog` | Delay feedback → fog texture |
| `osp_delayMix` | 2 | `osprey_fog` | Delay mix → fog texture |

### Group J — DROPPED: Output Level / Pan

No canonical output level or pan parameter exists. Engine output is controlled by DAW fader.

| Dropped Key | Count |
|---|---|
| `osprey_level` | 14 |
| `osprey_outputLevel` | 7 |
| `osprey_masterLevel` | 1 |
| `output_level` | 1 |
| `level` | 4 |
| `osprey_outputPan` | 3 |
| `pan` | 4 |

### Group K — DROPPED: Coupling Bus Params

Coupling is managed at the host level (`MegaCouplingMatrix`), not as engine parameters.

| Dropped Key | Count |
|---|---|
| `couplingBus` | 3 |
| `couplingLevel` | 3 |
| `coupling_level` | 1 |
| `osprey_couplingIn` | 6 |
| `osprey_couplingOut` | 6 |
| `osp_coupling` | 2 |

### Group L — DROPPED: Legacy Engine Design Params

These params existed in an earlier engine design that has since been replaced. No canonical
equivalent exists. Presets with only these params will sound like init patches (engine defaults)
which is acceptable — the preset was authored against a fundamentally different engine.

| Dropped Key | Count |
|---|---|
| `osprey_altitude` | 3 |
| `osprey_soarHeight` | 4 |
| `osprey_soar` | 2 |
| `osprey_thermalDrift` | 4 |
| `osprey_thermalRate` | 1 |
| `osprey_thermalDepth` | 1 |
| `osprey_wingFlutter` | 4 |
| `osprey_wingSpan` | 3 |
| `osp_wingSpan` | 2 |
| `osprey_stereoWidth` | 4 |
| `osprey_resonatorTuning` | 4 |
| `osprey_waveEnergy` | 2 |
| `osprey_dive` | 2 |
| `osprey_geoFrequency` | 2 |
| `osprey_coastalTexture` | 1 |
| `osprey_coastDepth` | 1 |
| `osprey_tidalRate` | 1 |
| `osprey_windLevel` | 1 |
| `osprey_excitationLevel` | 2 |
| `osprey_peakEnvFollower` | 2 |
| `osprey_macroDive` | 3 |
| `osprey_macroLift` | 3 |
| `osp_pitchDive` | 3 |
| `osp_diveTail` | 3 |
| `osp_diveSpeed` | 2 |
| `osp_velocity` | 1 |

---

## Migration Statistics

| Metric | Count |
|---|---|
| Total Osprey presets (non-quarantine) | 166 |
| Presets with legacy params (before fix) | 84 |
| Presets clean (after fix) | 166 |
| Total rename operations | 230 |
| Total scaled/normalized operations | 44 |
| Total dropped keys | 133 |
| Unknown / unhandled keys | 0 |
| Collision cases (legacy + canonical coexisting) | 0 |

---

## Mood Distribution

| Mood | Total Osprey Presets |
|---|---|
| Entangled | 64 |
| Family | 18 |
| Aether | 21 |
| Atmosphere | 16 |
| Foundation | 14 |
| Prism | 14 |
| Flux | 9 |
| Submerged | 8 |
| Coupling | 2 |
| **Total** | **166** |

---

## Verification

Post-fix verification script confirmed: 0 presets with legacy keys remaining across all 166
non-quarantine Osprey presets.

```
Clean presets: 166
Still-legacy presets: 0
```

---

## Presets That Will Still Sound Sparse (Legacy Design Only)

The following presets contained exclusively legacy params that had no canonical equivalent
(Group L — old engine design). They now load with engine defaults, which may not match
the original intent. Candidates for reauthoring:

- `Osprey_Thermal_Column.xometa` — altitude, wingSpan, macroDive/Lift, delayFeedback, reverbMix
- `Osprey_Frozen_Altitude.xometa` — same schema
- `Dive Bomb Fill.xometa` — osp_pitchDive, osp_diveTail, osp_velocity
- `Dive Pattern.xometa` — osp_pitchDive, osp_diveTail, osp_wingSpan
- `Era Sweep Dive.xometa` — osp_wingSpan
- `Barb_Radiant_II.xometa` — couplingBus, couplingLevel only (now empty Osprey block)
- `Ember_Surge_II.xometa` — same
- `Gentle_Prismatic_II.xometa` — macro_* only (macros migrated; other params at defaults)

---

## Files Changed

84 `.xometa` files across 9 mood directories. All changes are committed-ready. No engine
source code was modified.
