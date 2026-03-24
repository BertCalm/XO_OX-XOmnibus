# Preset Schema Audit — Round 3 Prism Sweep

**Date:** 2026-03-14
**Scope:** All 1,625 `.xometa` preset files across all 6 mood folders
**Method:** Cross-reference every parameter key in every preset against the engine's canonical `addParametersImpl()` list

---

## Summary Table

| Engine | Preset Count | Presets with Ghost Params | Ghost Param Count | Ghost Param IDs (sample) |
|--------|-------------|--------------------------|-------------------|--------------------------|
| OddfeliX | 236 | 217 | 78 | `couplingAmount`, `masterBalance`, `xSnap`, `snap_attack`, `snap_sustain`, `snap_release`, `snap_oscTune`, `snap_oscShape`, `snap_filterEnvAmt` + 69 more |
| Odyssey | 373 | 202 | 86 | `osc_a_mode`, `env_amp_attack`, `filt_a_cutoff`, `master_gain`, `delay_enable`, `reverb_enable`, `chorus_enable`, `chorus_mix`, `chorus_depth` + 77 more |
| Oblong | 343 | 167 | 51 | `oscA_wave`, `oscA_drift`, `flt_cutoff`, `env_attack`, `env_release`, `bob_mode`, `cur_mode`, `cur_amount`, `space_mix`, `space_size` + 41 more |
| Overdub | 181 | 41 | 35 | `osc_wave`, `filter_cutoff`, `env_attack`, `send_level`, `delay_feedback`, `delay_time`, `filter_resonance`, `reverb_size`, `reverb_mix` + 26 more |
| Overworld | 40 | 40 | 76 | ALL UPPERCASE (`ow_AMP_ATTACK`, `ow_ERA`, `ow_FM_ALGORITHM` etc.) — 100% case mismatch, affects all 40 presets |
| Ocelot | 15 | 15 | 1 | `ocelot_sampleRateRed` (engine has `ocelot_sampleRate`) |
| Opal | 118 | 11 | 5 | `opal_pulseWidth`, `opal_detuneCents`, `opal_smear`, `opal_reverbMix`, `opal_masterVolume` |
| Overbite | 60 | 10 | 16 | `poss_oscAWave`, `poss_oscALevel`, `poss_chew`, `poss_gnash`, `poss_fur`, `poss_glide`, `poss_filterEnvAmt`, `poss_filtEnvAttack/Decay/Sustain/Release`, `poss_masterVolume` |
| OddOscar | 155 | 7 | 15 | `morph_attack`, `morph_waveA`, `morph_waveB`, `morph_position`, `morph_blend`, `morph_driftDepth`, `morph_voiceMode` + 8 more |
| Onset | 120 | 3 | 12 | `onset_level`, `onset_decay`, `onset_attack`, `onset_drumKit`, `onset_kitSelect`, `onset_filterCutoff`, `onset_filterReso`, `onset_tune`, `onset_tone`, `onset_drive`, `onset_snap`, `onset_body` |
| Obese | 154 | 2 | 8 | `fat_filterCutoff`, `fat_filterReso`, `fat_oscMode`, `fat_unisonVoices`, `fat_attack`, `fat_decay`, `fat_sustain`, `fat_release` |
| Organon | 139 | 1 | 8 | `org_metabolicRate`, `org_entropy`, `org_membrane`, `org_cellDensity`, `org_adaptation`, `org_phasonShift`, `org_lockIn`, `org_level` |

---

## Clean Engines

Engines with presets in the library that are 100% schema-compliant:

| Engine | Preset Count |
|--------|-------------|
| Ouroboros | 60 |
| XOwlfish | 15 |
| Optic | 7 |
| Oblique | 6 |

Engines with zero presets in the XOlokun library (no presets to audit):

- Obscura, Obsidian, Oceanic, Oracle, Orbital, Origami, Osprey, Osteria

---

## Detail Section

---

### OddfeliX (`snap_` prefix) — 236 presets, 217 affected, 78 ghost params

**Root cause:** Two distinct waves of schema rot:

**Wave 1 — Old multi-engine object (111 presets affected).** A large batch of presets was authored with a now-obsolete schema where the `OddfeliX` parameter block contained an embedded OddOscar section (`oMorph`, `oBloom`, `oDecay`, `oFilterCutoff`, `oSub`, `oDetune`, `oLevel`, `oPolyphony`, `oDrift`, `oRelease`, `oSustain`) and an embedded second `snap_` voice (`xOscMode`, `xSnap`, `xDecay`, `xFilterCutoff`, `xFilterReso`, `xDetune`, `xLevel`, `xPitchLock`, `xUnison`, `xPolyphony`) plus FX blocks with no prefix (`delayEnabled`, `delayTime`, `delayFeedback`, `delayMix`, `delayWobble`, `reverbEnabled`, `reverbDecay`, `reverbSize`, `reverbMix`, `reverbDamping`, `phaserEnabled`, `phaserRate`, `phaserDepth`, `phaserMix`, `lofiEnabled`, `lofiBitDepth`, `lofiRateReduce`, `lofiWowFlutter`, `lofiMix`, `compressorEnabled`, `compThreshold`, `compRatio`, `compAttack`, `compRelease`, `couplingAmount`, `masterBalance`, `macro1`, `macro2`, `macro3`, `macro4`).

**Wave 2 — Known ghost params from the code health report (96+ presets affected).** These were flagged in the earlier bug report:
- `snap_attack` (99 presets), `snap_sustain` (97), `snap_release` (97), `snap_oscTune` (96), `snap_oscShape` (96), `snap_filterEnvAmt` (95) — parameters that existed in a prior iteration of SnapEngine before the envelope was stripped down to a single `snap_snap` decay model.
- `snap_resonance` (3 presets), `snap_filterEnv`, `snap_ampAttack/Decay/Sustain/Release`, `snap_snapAmount`, `snap_snapTone` (1 preset each)

**Also present:** 7 presets contain `morph_morph`, `morph_bloom`, `morph_decay`, `morph_sustain`, `morph_release`, `morph_filterCutoff`, `morph_filterReso`, `morph_drift`, `morph_sub`, `morph_detune`, `morph_level`, `morph_polyphony` inside the `OddfeliX` block — these are valid OddOscar param names but are misplaced in the OddfeliX block (they are not ghost params for OddOscar itself; they are ghost params here because they belong in the `OddOscar` block).

**Current canonical params (14):** `snap_snap`, `snap_decay`, `snap_detune`, `snap_filterCutoff`, `snap_filterReso`, `snap_level`, `snap_oscMode`, `snap_pitchLock`, `snap_polyphony`, `snap_unison`, `snap_macroDart`, `snap_macroDepth`, `snap_macroSchool`, `snap_macroSurface`

**Recommendation:** (b) Remove all ghost keys from presets. Do not add them to the engine.
- 111 presets with Wave 1 ghost block need full parameter block replacement.
- 96+ presets with `snap_attack/sustain/release/oscTune/oscShape/filterEnvAmt` need those 6 keys stripped.
- **Total preset files needing update: ~217** (some may overlap between Wave 1 and Wave 2).

---

### Odyssey (`drift_` prefix) — 373 presets, 202 affected, 86 ghost params

**Root cause:** Two distinct old schemas.

**Wave 1 — Unprefixed old-schema params (196 presets).** A large portion of Odyssey presets were authored against an old XOdyssey parameter schema that predates the `drift_` prefix convention. Examples: `osc_a_mode`, `osc_a_level`, `env_amp_attack`, `env_amp_sustain`, `env_amp_release`, `master_gain`, `filt_a_cutoff`, `filt_a_reso`, `filt_a_slope`, `haze_amount`, `sub_level`, `drift_depth`, `drift_rate`, `shimmer_amount`, `shimmer_tone`, `noise_level`, `noise_type`, `reverb_enable`, `reverb_size`, `reverb_mix`, `reverb_damping`, `delay_enable`, `delay_time`, `delay_feedback`, `delay_mix`, `delay_mode`, `chorus_enable`, `chorus_rate`, `chorus_depth`, `chorus_mix`, `phaser_enable`, `phaser_rate`, `phaser_depth`, `phaser_mix`, `phaser_feedback`, `tidal_depth`, `tidal_rate`, `fracture_enable`, `fracture_intensity`, `fracture_rate`, `voice_mode`, `voice_glide_time`, `global_spread`, `mod_1_source`, `mod_1_dest`, `mod_1_amount`, `macro_journey` plus various `osc_b_*`, `filt_b_*` params, `env_filter_*` params, `sub_shape`, `sub_octave`.

**Wave 2 — Incorrectly double-prefixed params (3 presets).** Three presets contain params like `drift_osc_a_mode`, `drift_haze_amount`, `drift_reverb_size`, `drift_master_gain` — these are the old unprefixed names with `drift_` prepended by a tool that partially migrated them.

**One-off ghost params:** `odyssey_detune` (3 presets — has wrong engine prefix instead of `drift_detune`), `osc_a_fine`, `osc_b_fine`, `osc_b_tune`.

**Current canonical params (38):** All use `drift_` prefix — `drift_oscA_mode`, `drift_oscA_level`, `drift_oscA_detune`, `drift_oscA_pw`, `drift_oscA_shape`, `drift_oscA_tune`, `drift_oscA_fmDepth`, `drift_oscB_*` (same 7), `drift_subLevel`, `drift_noiseLevel`, `drift_hazeAmount`, `drift_filterCutoff`, `drift_filterReso`, `drift_filterSlope`, `drift_filterEnvAmt`, `drift_shimmerAmount`, `drift_shimmerTone`, `drift_attack`, `drift_decay`, `drift_sustain`, `drift_release`, `drift_driftDepth`, `drift_driftRate`, `drift_lfoRate`, `drift_lfoDepth`, `drift_lfoDest`, `drift_formantMix`, `drift_formantMorph`, `drift_glide`, `drift_level`, `drift_voiceMode`, `drift_polyphony`.

**Recommendation:** (b) Remove all ghost keys from presets. The unprefixed params were the XOdyssey v1 standalone schema; the canonical XOlokun schema always uses `drift_`. 202 preset files need updating.

---

### Oblong (`bob_` prefix) — 343 presets, 167 affected, 51 ghost params

**Root cause:** Old-schema unprefixed params. The 167 affected presets use unprefixed names for most core parameters, indicating they were authored against an older XOblong/BobEngine schema before the `bob_` prefix was enforced.

**High-frequency ghosts (>50 presets each):** `oscA_wave`, `oscA_drift`, `flt_cutoff`, `env_attack`, `env_release`, `oscA_shape`, `oscB_blend`, `env_sustain`, `oscB_wave`, `flt_mode`, `oscB_detune`, `flt_resonance`, `env_decay`, `bob_mode` (note: `bob_mode` is a ghost — the canonical param is `bob_bobMode`).

**Medium-frequency ghosts (10–100 presets):** `space_mix`, `space_size`, `cur_mode`, `cur_amount`, `lfo1_rate`, `lfo1_depth`, `flt_envAmt`, `flt_character`, `smear_mix`, `smear_depth`, `motEnv_depth/attack/decay/sustain/release`, `oscB_fm`, `glide`, `flt_drive`, `dust_amount/age/tone`, `smear_width`, `oscA_tune`, `tex_level/mode/tone/width`, `space_decay`, `space_air`.

**Low-frequency ghosts:** `lfo2_rate`, `lfo2_depth`, `oscB_sync`, `lfo1_shape`, `vel_filter`, `smear_rate`, `stutter_rate`, `stutter_depth`.

**Mapping to canonical:** Most ghost params have a direct canonical equivalent (e.g., `oscA_wave` → `bob_oscA_wave`, `flt_cutoff` → `bob_fltCutoff`, `env_attack` → `bob_ampAttack`, `space_mix` → implicit via reverb, `cur_mode` → `bob_curMode`). However: `stutter_rate`, `stutter_depth`, `vel_filter`, `space_air`, and parts of `space_*` have no canonical equivalent — the BobEngine space/reverb is exposed as `bob_texLevel/Mode/Tone/Width` and the engine has no reverb tail or stutter parameters.

**Recommendation:** (b) Remove ghost keys and replace with canonical `bob_` equivalents where a mapping exists. For `stutter_*`, `vel_filter`, `space_air` — delete with no replacement. **167 preset files need updating.**

---

### Overdub (`dub_` prefix) — 181 presets, 41 affected, 35 ghost params

**Root cause:** Old-schema unprefixed params from the standalone XOverdub preset format.

**High-frequency ghosts (>10 presets):** `osc_wave`, `filter_cutoff`, `env_attack`, `env_sustain`, `env_release`, `send_level`, `env_decay`, `filter_resonance`, `delay_feedback`, `delay_time`, `reverb_size`, `reverb_mix`, `delay_wear`, `osc_sub_level`, `delay_wow`, `drive_amount`, `filter_env_amt`, `lfo_rate`, `lfo_depth`, `lfo_dest`, `osc_noise_level`, `osc_octave`, `delay_sync`, `voice_mode`, `return_level`.

**Low-frequency ghosts (<10 presets):** `osc_pwm`, `osc_drift`, `voice_glide`, `pitch_env_depth`, `pitch_env_decay`, `reverb_damp`, `dub_oscMode`, `dub_oscShape`, `dub_sendAmount`, `osc_level`.

**Canonical mappings exist for most:** `osc_wave` → `dub_oscWave`, `filter_cutoff` → `dub_filterCutoff`, `send_level` → `dub_sendLevel`, `delay_feedback` → `dub_delayFeedback`, etc. Note: `dub_oscMode`, `dub_oscShape`, `dub_sendAmount` — these look like correct `dub_` prefix params but are still ghost params because the engine uses `dub_oscWave` (not `dub_oscMode`/`dub_oscShape`) and `dub_sendLevel` (not `dub_sendAmount`).

**Recommendation:** (b) Remove ghost keys from presets. The 41 affected presets need old-schema keys replaced with `dub_` canonical equivalents. **41 preset files need updating.**

---

### Overworld (`ow_` prefix) — 40 presets, 40 affected, 76 ghost params

**Root cause:** ALL_CAPS vs camelCase ID mismatch. The `generate_overworld_presets.py` tool wrote params using SCREAMING_SNAKE_CASE (e.g., `ow_AMP_ATTACK`, `ow_ERA`, `ow_FM_ALGORITHM`, `ow_ECHO_FIR_0`) while the actual engine (from `XOverworld/src/engine/Parameters.h`) registers them as camelCase (e.g., `ow_ampAttack`, `ow_era`, `ow_fmAlgorithm`, `ow_echoFir0`).

**This is a total fleet failure for Overworld:** All 40 presets use SCREAMING_SNAKE_CASE exclusively. Not a single preset uses the correct camelCase form.

**Scope:** 76 ghost params represent all Overworld parameters — the entire parameter namespace of every Overworld preset is non-functional.

**Mapping is 1:1 (UPPER_SNAKE → camelCase):**
- `ow_ERA` → `ow_era`, `ow_ERA_Y` → `ow_eraY`, `ow_VOICE_MODE` → `ow_voiceMode`
- `ow_MASTER_VOL` → `ow_masterVol`, `ow_MASTER_TUNE` → `ow_masterTune`
- `ow_PULSE_DUTY` → `ow_pulseDuty`, `ow_PULSE_SWEEP` → `ow_pulseSweep`
- `ow_TRI_ENABLE` → `ow_triEnable`, `ow_NOISE_MODE` → `ow_noiseMode`, `ow_NOISE_PERIOD` → `ow_noisePeriod`
- `ow_DPCM_ENABLE` → `ow_dpcmEnable`, `ow_DPCM_RATE` → `ow_dpcmRate`, `ow_NES_MIX` → `ow_nesMix`
- `ow_FM_ALGORITHM` → `ow_fmAlgorithm`, `ow_FM_FEEDBACK` → `ow_fmFeedback`
- `ow_FM_OP1_LEVEL` → `ow_fmOp1Level`, `ow_FM_OP2_LEVEL` → `ow_fmOp2Level` (etc. for OP3/OP4)
- `ow_FM_OP1_MULT` → `ow_fmOp1Mult` (etc.), `ow_FM_OP1_DETUNE` → `ow_fmOp1Detune` (etc.)
- `ow_FM_ATTACK` → `ow_fmAttack`, `ow_FM_DECAY` → `ow_fmDecay`, `ow_FM_SUSTAIN` → `ow_fmSustain`, `ow_FM_RELEASE` → `ow_fmRelease`
- `ow_FM_LFO_RATE` → `ow_fmLfoRate`, `ow_FM_LFO_DEPTH` → `ow_fmLfoDepth`
- `ow_BRR_SAMPLE` → `ow_brrSample`, `ow_BRR_INTERP` → `ow_brrInterp`
- `ow_SNES_ATTACK` → `ow_snesAttack`, `ow_SNES_DECAY` → `ow_snesDecay`, `ow_SNES_SUSTAIN` → `ow_snesSustain`, `ow_SNES_RELEASE` → `ow_snesRelease`
- `ow_PITCH_MOD` → `ow_pitchMod`, `ow_NOISE_REPLACE` → `ow_noiseReplace`
- `ow_ECHO_DELAY` → `ow_echoDelay`, `ow_ECHO_FEEDBACK` → `ow_echoFeedback`, `ow_ECHO_MIX` → `ow_echoMix`
- `ow_ECHO_FIR_0` through `ow_ECHO_FIR_7` → `ow_echoFir0` through `ow_echoFir7`
- `ow_GLITCH_AMOUNT` → `ow_glitchAmount`, `ow_GLITCH_TYPE` → `ow_glitchType`, `ow_GLITCH_RATE` → `ow_glitchRate`, `ow_GLITCH_DEPTH` → `ow_glitchDepth`, `ow_GLITCH_MIX` → `ow_glitchMix`
- `ow_AMP_ATTACK` → `ow_ampAttack`, `ow_AMP_DECAY` → `ow_ampDecay`, `ow_AMP_SUSTAIN` → `ow_ampSustain`, `ow_AMP_RELEASE` → `ow_ampRelease`
- `ow_FILTER_CUTOFF` → `ow_filterCutoff`, `ow_FILTER_RESO` → `ow_filterReso`, `ow_FILTER_TYPE` → `ow_filterType`
- `ow_CRUSH_BITS` → `ow_crushBits`, `ow_CRUSH_RATE` → `ow_crushRate`, `ow_CRUSH_MIX` → `ow_crushMix`
- `ow_ERA_DRIFT_RATE` → `ow_eraDriftRate`, `ow_ERA_DRIFT_DEPTH` → `ow_eraDriftDepth`, `ow_ERA_DRIFT_SHAPE` → `ow_eraDriftShape`
- `ow_ERA_PORTA_TIME` → `ow_eraPortaTime`, `ow_ERA_MEM_TIME` → `ow_eraMemTime`, `ow_ERA_MEM_MIX` → `ow_eraMemMix`
- `ow_VERTEX_A` → `ow_vertexA`, `ow_VERTEX_B` → `ow_vertexB`, `ow_VERTEX_C` → `ow_vertexC`

**Recommendation:** (b) Fix `generate_overworld_presets.py` to use camelCase keys, then regenerate all 40 Overworld presets. All 40 preset files are affected. The fix is mechanical — a lowercase+camelCase transform script applied to every `ow_` key.

---

### Ocelot (`ocelot_` prefix, flat preset structure) — 15 presets, 15 affected, 1 ghost param

**Root cause:** Single parameter rename mismatch. All 15 Ocelot presets use `ocelot_sampleRateRed`; the canonical engine param (in `OcelotParameters.h`) is `ocelot_sampleRate`.

**Ghost param:**
- `ocelot_sampleRateRed` (15 presets — e.g. "Canopy Rain", "Prowling Groove", "Jungle Pulse") → canonical: `ocelot_sampleRate`

**Recommendation:** (b) Rename `ocelot_sampleRateRed` → `ocelot_sampleRate` in all 15 preset files. **15 preset files need updating.**

---

### Opal (`opal_` prefix) — 118 presets, 11 affected, 5 ghost params

**Root cause:** Parameter renames and missing FX sub-namespace. 11 presets reference params from an earlier Opal schema:

**Ghost params (all appear in the same 11 presets):**
- `opal_pulseWidth` → no canonical equivalent; OpalEngine does not have a pulse-width osc parameter
- `opal_detuneCents` → no canonical equivalent; detuning is handled differently (the engine has `opal_osc2Detune` and `opal_pitchScatter`)
- `opal_smear` → renamed to `opal_fxSmearAmount` and `opal_fxSmearMix` in the canonical schema
- `opal_reverbMix` → renamed to `opal_fxReverbMix` in the canonical schema (note the `fx` sub-namespace)
- `opal_masterVolume` → no canonical equivalent; the engine uses `opal_level`

**Recommendation:** (b) For the 11 affected presets: remove `opal_pulseWidth` and `opal_detuneCents` (no canonical mapping), remap `opal_smear` → `opal_fxSmearAmount` (value preserved), `opal_reverbMix` → `opal_fxReverbMix` (value preserved), `opal_masterVolume` → `opal_level` (value preserved). **11 preset files need updating.**

---

### Overbite (`poss_` prefix) — 60 presets, 10 affected, 16 ghost params

**Root cause:** Old-schema param names from a pre-canonical XOppossum preset format. The 10 affected presets use abbreviated names or old-schema param IDs.

**Ghost params (all 16 appear in the same 10 presets):**
- `poss_oscAWave` → canonical: `poss_oscAWaveform`
- `poss_oscALevel` → no direct canonical (osc levels are controlled via macro; the engine has `poss_oscMix` for osc balance)
- `poss_oscAPulseWidth` → no canonical equivalent
- `poss_oscBWave` → canonical: `poss_oscBWaveform`
- `poss_oscBLevel` → no direct canonical (same as oscALevel)
- `poss_oscBDetune` → no canonical (`poss_unisonDetune` and `poss_oscBInstability` are the closest)
- `poss_filterEnvAmt` → canonical: `poss_filterEnvAmount`
- `poss_fur` → canonical: `poss_furAmount`
- `poss_chew` → canonical: `poss_chewAmount`
- `poss_gnash` → canonical: `poss_gnashAmount`
- `poss_filtEnvAttack` → canonical: `poss_filterAttack`
- `poss_filtEnvDecay` → canonical: `poss_filterDecay`
- `poss_filtEnvSustain` → canonical: `poss_filterSustain`
- `poss_filtEnvRelease` → canonical: `poss_filterRelease`
- `poss_glide` → canonical: `poss_glideTime`
- `poss_masterVolume` → canonical: `poss_level`

**Recommendation:** (b) Apply renames for the 12 with canonical equivalents; delete `poss_oscALevel`, `poss_oscBLevel`, `poss_oscAPulseWidth`, and `poss_oscBDetune` (no canonical mapping). **10 preset files need updating.**

---

### OddOscar (`morph_` prefix) — 155 presets, 7 affected, 15 ghost params

**Root cause:** Three distinct old schemas, each represented in a small number of presets.

**Group 1 — Wavetable-era params (4–5 presets):** `morph_waveA`, `morph_waveB`, `morph_position`, `morph_morphSpeed` — these appear in presets like "Spectral Symbiosis", "Ecosystem", "Wavetable Diet". These suggest a now-removed wavetable oscillator mode.

**Group 2 — Extended envelope params (5 presets):** `morph_attack` (5 presets) — the canonical engine only has `morph_decay`, `morph_sustain`, `morph_release` (attack is not present in current MorphEngine).

**Group 3 — Old dual-osc schema (1 preset each):** `morph_oscA_wave`, `morph_oscA_shape`, `morph_oscA_tune`, `morph_oscB_wave`, `morph_oscB_shape`, `morph_oscB_tune`, `morph_blend`, `morph_filterEnv`, `morph_driftDepth`, `morph_voiceMode`.

**Recommendation:** (b) Remove all ghost keys. `morph_attack` has no canonical mapping (no attack param in the engine). All other ghosts are from old schemas. **7 preset files need updating.**

---

### Onset (`perc_` prefix) — 120 presets, 3 affected, 12 ghost params

**Root cause:** Three presets ("Rhythmic Feeding", "Impact Nutrition", "Beat Force") were authored using a simplified high-level `onset_` parameter namespace that predates or is separate from the canonical `perc_` prefix schema.

**Ghost params:**
- `onset_level`, `onset_decay`, `onset_attack`, `onset_drumKit`, `onset_filterCutoff`, `onset_filterReso`, `onset_kitSelect`, `onset_tune`, `onset_tone`, `onset_drive`, `onset_snap`, `onset_body`

None of these map 1:1 to the canonical `perc_` schema. The `onset_` prefix has no canonical meaning in OnsetEngine — the real params use `perc_v{N}_` per-voice and `perc_macro_`, `perc_fx_`, `perc_char_`, `perc_xvc_` global prefixes.

**Recommendation:** (b) Delete the 12 `onset_` ghost keys from these 3 presets (or rewrite the presets using the full `perc_` canonical schema). **3 preset files need updating.**

---

### Obese (`fat_` prefix) — 154 presets, 2 affected, 8 ghost params

**Root cause:** Two presets ("Midnight Strobe", "Supersize Organism") use param IDs from an old FatEngine schema where filter and amp parameters were named differently.

**Ghost params:**
- `fat_filterCutoff` (2 presets) → canonical: `fat_fltCutoff`
- `fat_filterReso` (2 presets) → canonical: `fat_fltReso`
- `fat_oscMode` (1 preset) → no canonical equivalent in current FatEngine
- `fat_unisonVoices` (1 preset) → no canonical equivalent (unison is controlled via `fat_morph`)
- `fat_attack` (1 preset) → canonical: `fat_ampAttack`
- `fat_decay` (1 preset) → canonical: `fat_ampDecay`
- `fat_sustain` (1 preset) → canonical: `fat_ampSustain`
- `fat_release` (1 preset) → canonical: `fat_ampRelease`

**Recommendation:** (b) Fix the 2 affected presets. Apply renames: `fat_filterCutoff` → `fat_fltCutoff`, `fat_filterReso` → `fat_fltReso`, `fat_attack` → `fat_ampAttack`, `fat_decay` → `fat_ampDecay`, `fat_sustain` → `fat_ampSustain`, `fat_release` → `fat_ampRelease`. Delete `fat_oscMode` and `fat_unisonVoices`. **2 preset files need updating.**

---

### Organon (`organon_` prefix) — 139 presets, 1 affected, 8 ghost params

**Root cause:** One preset ("Chaos Metabolism") was authored using an `org_` short-prefix rather than the canonical `organon_` prefix. Additionally, several param names don't match canonical IDs even with the correct prefix.

**Ghost params (all in 1 preset — "Chaos Metabolism"):**
- `org_metabolicRate` → canonical: `organon_metabolicRate` (prefix mismatch)
- `org_entropy` → no canonical `organon_entropy` exists (not in engine)
- `org_membrane` → canonical: `organon_membrane` (prefix mismatch)
- `org_cellDensity` → no canonical `organon_cellDensity` exists (not in engine)
- `org_adaptation` → no canonical `organon_adaptation` exists (not in engine)
- `org_phasonShift` → canonical: `organon_phasonShift` (prefix mismatch)
- `org_lockIn` → canonical: `organon_lockIn` (prefix mismatch)
- `org_level` → no canonical `organon_level` exists (not in engine)

**Canonical Organon params (10):** `organon_metabolicRate`, `organon_enzymeSelect`, `organon_catalystDrive`, `organon_dampingCoeff`, `organon_signalFlux`, `organon_phasonShift`, `organon_isotopeBalance`, `organon_lockIn`, `organon_membrane`, `organon_noiseColor`

**Recommendation:** (b) Fix "Chaos Metabolism" preset: rename the 4 params with prefix-only issues, delete `org_entropy`, `org_cellDensity`, `org_adaptation`, and `org_level` (no canonical equivalents). **1 preset file needs updating.**

---

## Known Problem Area Verification

### SNAP ghost params (from code health report)

**Confirmed.** The six known ghost params `snap_oscTune`, `snap_attack`, `snap_sustain`, `snap_release`, `snap_filterEnvAmt`, `snap_oscShape` are present and accounted for in the OddfeliX engine section above. Counts: `snap_attack` (99), `snap_sustain` (97), `snap_release` (97), `snap_oscTune` (96), `snap_oscShape` (96), `snap_filterEnvAmt` (95).

### OBSIDIAN `formantResonance` vs `formantIntensity` collision

**Not present as an issue.** There are zero Obsidian presets in the XOlokun preset library (`Presets/XOlokun/**/*.xometa`). The collision exists at the engine code level (where `obsidian_formantResonance` was a former name, now replaced by `obsidian_formantIntensity` in the canonical `ObsidianEngine.h`), but since there are no presets in the library referencing it, there is no active schema violation to fix. Status: code-level risk only — if Obsidian presets are ever generated using the old name, they will fail.

---

## Fleet-Wide Ghost Param Totals

| Category | Preset Files | Total Ghost Param Instances |
|----------|-----------|-----------------------------|
| Old unprefixed schema | ~490 | ~15,000+ instances across Odyssey, Oblong, Overdub, OddfeliX Wave 1 |
| SNAP known ghosts (6 specific params) | ~96–99 each | ~578 instances |
| Overworld UPPER_SNAKE_CASE mismatch | 40 | ~3,040 instances (76 params × 40 presets) |
| Overbite short names | 10 | ~160 instances |
| OddfeliX embedded multi-engine block | 111 | ~4,800 instances |
| OddOscar old schemas | 7 | ~80 instances |
| Onset onset_ prefix | 3 | ~36 instances |
| Opal renamed/restructured params | 11 | ~55 instances |
| Ocelot sampleRateRed | 15 | 15 instances |
| Obese old names | 2 | 16 instances |
| Organon org_ prefix | 1 | 8 instances |

**Total preset files with at least one ghost parameter: ~448 of 1,625 (27.6%)**

---

## Priority Ranking for Fixes

| Priority | Engine | Files | Action |
|----------|--------|-------|--------|
| P0 | Overworld | 40 | 100% broken — regenerate all presets from generator script after fixing `generate_overworld_presets.py` to use camelCase |
| P1 | OddfeliX | 217 | Strip Wave 1 old-schema blob (111 presets) + strip 6 known SNAP ghost params (96–99 presets each) |
| P1 | Odyssey | 202 | Replace ~196 presets' unprefixed params with `drift_` canonical names |
| P1 | Oblong | 167 | Replace ~167 presets' unprefixed params with `bob_` canonical names |
| P2 | Overdub | 41 | Replace 41 presets' unprefixed params with `dub_` canonical names |
| P2 | Overbite | 10 | Apply 12 renames; delete 4 unmappable params |
| P3 | Ocel | 15 | Rename `ocelot_sampleRateRed` → `ocelot_sampleRate` in 15 files |
| P3 | Opal | 11 | Apply 3 renames; delete 2 unmappable params |
| P3 | OddOscar | 7 | Strip old-schema keys |
| P4 | Obese | 2 | Apply 6 renames; delete 2 unmappable params |
| P4 | Onset | 3 | Delete 12 `onset_` ghost keys |
| P4 | Organon | 1 | Fix 1 preset: 4 renames + 4 deletions |
