# Overbite Parameter Schema Fix — Fleet Audit Report

**Date:** 2026-03-21  
**Engine:** OVERBITE (`poss_` prefix)  
**Source:** `Source/Engines/Bite/BiteEngine.h` (122 parameters, frozen IDs)  
**Presets audited:** 240 (all non-quarantine `.xometa` files with `"Overbite"` in `engines` array)

---

## Summary

The OVERBITE engine accumulated 4 generations of parameter naming across its life as a standalone plugin and XOceanus engine. The `applyPreset()` loader silently drops any unrecognized parameter ID, meaning all legacy-named params were inert — presets loaded with incorrect parameter values (defaults instead of authored values).

| Metric | Count |
|--------|-------|
| Presets scanned | 240 |
| Presets modified | 97 |
| Parameter renames applied | 451 |
| Parameters dropped (no canonical equivalent) | 470 |
| Conflicts (legacy + canonical coexisting) | 0 |
| Remaining non-canonical params | 0 |

---

## Parameter Generations Found

### Gen 1 — Completely Unprefixed (very early prototypes)
No `poss_` prefix at all. These params never loaded in XOceanus regardless.
- `amp_attack`, `amp_release`, `filter_cutoff`, `filter_resonance`, `output_level`
- `reverb_mix`, `reverb_size`, `density`, `coupling_level`, `drive`
- `macro_character`, `macro_coupling`, `macro_movement`, `macro_space`
- `level`, `pan`, `bite`, `coupling`, `resonance`, `space`

### Gen 2 — Standalone Concept Params (pre-XOceanus integration)
Had `poss_` prefix but different param naming scheme. No canonical equivalents.
- `poss_aggression`, `poss_brightness`, `poss_warmth`
- `poss_couplingBus`, `poss_couplingLevel`, `poss_macroCoupling`
- `poss_attack`, `poss_release`
- `poss_biteDepth`, `poss_fangAttack`, `poss_snapRelease`

### Gen 3 — First XOceanus Integration (shortened names)
Had `poss_` prefix, abbreviated naming that didn't match APVTS IDs.
Canonical equivalents exist — these are the mappable renames.

### Gen 4 — Current Canonical (BiteEngine.h frozen IDs)
Full APVTS-matching names: `poss_furAmount`, `poss_glideTime`, `poss_oscAWaveform`, etc.

---

## Legacy → Canonical Rename Map Applied

| Legacy Name | Canonical Name | Presets Fixed |
|-------------|----------------|---------------|
| `poss_fur` | `poss_furAmount` | 16 |
| `poss_glide` | `poss_glideTime` | 14 |
| `poss_gnash` | `poss_gnashAmount` | 16 |
| `poss_gnashAmt` | `poss_gnashAmount` | 26 |
| `poss_chew` | `poss_chewAmount` | 16 |
| `poss_masterVolume` | `poss_level` | 16 |
| `poss_outputLevel` | `poss_level` | 58 |
| `poss_outputPan` | `poss_pan` | 55 |
| `poss_oscAWave` | `poss_oscAWaveform` | 16 |
| `poss_oscBWave` | `poss_oscBWaveform` | 16 |
| `poss_subOct` | `poss_subOctave` | 4 |
| `poss_filterEnvAmt` | `poss_filterEnvAmount` | 16 |
| `poss_filtEnvAttack` | `poss_filterAttack` | 16 |
| `poss_filtEnvDecay` | `poss_filterDecay` | 16 |
| `poss_filtEnvSustain` | `poss_filterSustain` | 16 |
| `poss_filtEnvRelease` | `poss_filterRelease` | 16 |
| `poss_filter_cutoff` | `poss_filterCutoff` | 2 |
| `poss_amp_sustain` | `poss_ampSustain` | 2 |
| `poss_resonance` | `poss_filterReso` | 2 |
| `poss_oscInteractionAmt` | `poss_oscInteractAmount` | 1 |
| `poss_oscInteractionMode` | `poss_oscInteractMode` | 1 |
| `poss_drive` | `poss_driveAmount` | 2 |
| `poss_weightAmt` | `poss_weightLevel` | 4 |
| `poss_macro1` | `poss_macroBelly` | 26 |
| `poss_macro2` | `poss_macroBite` | 26 |
| `poss_macro3` | `poss_macroScurry` | 26 |
| `poss_macro4` | `poss_macroTrash` | 26 |

---

## Dropped Params (No Canonical Equivalent)

These params had no counterpart in the current BiteEngine parameter layout. Keeping them
was harmless (APVTS silently ignores unknown IDs) but they created noise. They have been
removed from preset files.

| Dropped Name | Category | Presets Cleaned |
|--------------|----------|-----------------|
| `poss_biteDepth` | Gen 2 concept | 32 |
| `poss_couplingIn` | Old per-engine coupling | 26 |
| `poss_couplingOut` | Old per-engine coupling | 26 |
| `poss_fangAttack` | Gen 2 concept | 26 |
| `poss_snapRelease` | Gen 2 concept | 26 |
| `poss_oscALevel` | No osc-level in engine | 16 |
| `poss_oscBDetune` | No per-osc detune in engine | 16 |
| `poss_oscBLevel` | No osc-level in engine | 16 |
| `poss_oscAPulseWidth` | No pulse width in engine | 12 |
| `poss_lfoRate` | Ambiguous (which LFO?) | 5 |
| `poss_delayMix` | No canonical delay param | 5 |
| `poss_reverbMix` | No canonical reverb param | 5 |
| `poss_subShape` | No sub shape in engine | 4 |
| `poss_filtEnvVelScale` | No vel-scale in engine | 3 |
| `poss_fmDepth` | No FM in engine | 3 |
| `poss_oscWave` | Ambiguous (A or B?) | 3 |
| `poss_bite` | Conflicts with macroBite | 2 |
| `poss_character` | Gen 2 concept | 2 |
| `poss_coupling` | Old coupling system | 2 |
| `poss_envAmount` | Ambiguous (filter or mod env?) | 2 |
| `poss_lfoDepth` | Ambiguous (which LFO?) | 2 |
| `poss_movement` | Gen 2 concept | 2 |
| `poss_space` | Gen 2 concept | 2 |
| `poss_sub_osc` | Superseded by poss_subLevel | 2 |
| `poss_noiseTransient` | No canonical equivalent | 1 |
| `poss_oscBAsymmetry` | No canonical equivalent | 1 |
| `poss_aggression` | Gen 2 DNA param | 20+ |
| `poss_brightness` | Gen 2 DNA param | 20+ |
| `poss_warmth` | Gen 2 DNA param | 20+ |
| `poss_couplingBus` | Old coupling system | 20+ |
| `poss_couplingLevel` | Old coupling system | 20+ |
| `poss_macroCoupling` | Old macro scheme | 3 |
| `poss_attack` | Gen 2 (missing section qualifier) | 3 |
| `poss_release` | Gen 2 (missing section qualifier) | 3 |
| Gen 1 unprefixed params | Very early schema | various |

---

## Recommendation: Add Overbite Alias Resolver to PresetManager.h

The `resolveSnapParamAlias()` pattern exists for OddfeliX but not for Overbite. Consider
adding a `resolveBiteParamAlias()` function to `Source/Core/PresetManager.h` with the
rename map above, so that any future legacy presets (e.g., user community files) also load
correctly at runtime — not just files cleaned up by this migration script.

See the `applyPreset()` function in `Source/XOceanusProcessor.cpp` lines 1440-1448 for
the OddfeliX integration pattern to follow.

---

## Verification

Post-migration verification confirmed: **0 non-canonical params remain** in any Overbite preset
across all 240 scanned files. All params now match the frozen APVTS IDs in `BiteEngine.h`.

---

*Generated by: fleet-audit/overbite-param-fix — 2026-03-21*
