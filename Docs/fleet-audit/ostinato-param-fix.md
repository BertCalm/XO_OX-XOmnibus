# Ostinato Parameter Fix Audit
**Date:** 2026-03-21
**Scope:** All Ostinato presets in `Presets/XOceanus/` (excluding `_quarantine/`)
**Total Ostinato presets scanned:** 198

---

## Problem 1: Dual Parameter Naming (osti_instrument{N} vs osti_seat{N}_instrument)

### Root Cause
The canonical engine parameter ID is `osti_seat{N}_instrument` as declared in `OstinatoEngine.h`:
```cpp
juce::String pre = "osti_seat" + juce::String (s + 1) + "_";
params.push_back(std::make_unique<Choice>(
    juce::ParameterID(pre + "instrument", 1), name + " Instrument", ...));
```
One preset was authored with the shorter non-canonical form `osti_instrument{N}`, which silently fails to load (APVTS lookup returns null).

### Files Fixed (1)
| File | Fix Applied |
|------|-------------|
| `Atmosphere/Ostinato_World_Fusion.xometa` | Renamed `osti_instrument1`–`osti_instrument8` → `osti_seat1_instrument`–`osti_seat8_instrument`. Removed 13 additional non-canonical global params (`osti_level`, `osti_space`, `osti_attack`, `osti_decay`, `osti_sustain`, `osti_release`, `osti_velFilter`, `osti_velAmp`, `osti_lfo1Rate`, `osti_lfo1Depth`, `osti_lfo1Target`, `osti_polyphony`, `osti_pitchBendRange`, `osti_macroSwing`, `osti_macroBody`). Added all canonical seat sub-params and global params with engine defaults. |

### Additional Non-Canonical Params Found (3 Entangled coupling presets)
These presets used invented param names that do not exist in `OstinatoEngine.h`. Mapped to nearest canonical equivalents:

| File | Non-Canonical Param | Canonical Replacement | Notes |
|------|--------------------|-----------------------|-------|
| `Entangled/Collision_Hammer_Pattern.xometa` | `osti_rhythmDensity: 0.65` | `osti_macroFire: 0.65` | Fire drives exciter energy and rhythmic density |
| `Entangled/Collision_Hammer_Pattern.xometa` | `osti_accentDepth: 0.72` | `osti_macroCircle: 0.72` | Circle drives sympathetic accent resonance |
| `Entangled/Collision_Hammer_Pattern.xometa` | `osti_filterCutoff: 0.48` | `osti_masterFilter: 8744.0` | Normalized 0–1 mapped to Hz (200–18000 range, linear) |
| `Entangled/Collision_Hammer_Pattern.xometa` | `osti_outputLevel: 0.80` | `osti_masterLevel: 0.80` | Direct value-preserving rename |
| `Entangled/Symbiosis_Ground_And_Sky.xometa` | `osti_rhythmDensity: 0.55` | `osti_macroFire: 0.55` | |
| `Entangled/Symbiosis_Ground_And_Sky.xometa` | `osti_accentDepth: 0.62` | `osti_macroCircle: 0.62` | |
| `Entangled/Symbiosis_Ground_And_Sky.xometa` | `osti_filterCutoff: 0.45` | `osti_masterFilter: 8210.0` | |
| `Entangled/Symbiosis_Ground_And_Sky.xometa` | `osti_outputLevel: 0.82` | `osti_masterLevel: 0.82` | |
| `Entangled/Symbiosis_Pulse_Shimmer.xometa` | `osti_rhythmDensity: 0.50` | `osti_macroFire: 0.50` | |
| `Entangled/Symbiosis_Pulse_Shimmer.xometa` | `osti_accentDepth: 0.55` | `osti_macroCircle: 0.55` | |
| `Entangled/Symbiosis_Pulse_Shimmer.xometa` | `osti_filterCutoff: 0.52` | `osti_masterFilter: 9456.0` | |
| `Entangled/Symbiosis_Pulse_Shimmer.xometa` | `osti_outputLevel: 0.80` | `osti_masterLevel: 0.80` | |

---

## Problem 2: Swing Value Range Error

### Root Cause
`osti_swing` is declared in `OstinatoEngine.h` with range `0.0f–100.0f`:
```cpp
params.push_back(std::make_unique<Float>(
    juce::ParameterID("osti_swing", 1), "Osti Swing",
    juce::NormalisableRange<float>(0.0f, 100.0f), 0.0f));
```
19 presets stored swing as a normalized 0–1 value (e.g., `0.33` meaning "33%") instead of the correct absolute 0–100 scale value (`33.0`). These load as nearly-zero swing (0–1% effective swing), silently destroying the intended groove character.

### Fix Applied
All affected values were multiplied by 100 to convert from normalized to absolute scale.

### Files Fixed (19)
| File | Old Value | Corrected Value |
|------|-----------|----------------|
| `Atmosphere/Ostinato_World_Fusion.xometa` | 0.33 | 33.0 |
| `Aether/Ostinato_Afrobeat_Station.xometa` | 0.33 | 33.0 |
| `Aether/Ostinato_Cave_Ceremony.xometa` | 0.1 | 10.0 |
| `Aether/Ostinato_Fire_Circle_Complete.xometa` | 0.33 | 33.0 |
| `Aether/Ostinato_Midnight_Istanbul.xometa` | 0.25 | 25.0 |
| `Aether/Ostinato_Samba_Macchiato.xometa` | 0.15 | 15.0 |
| `Aether/Ostinato_The_Full_Fire.xometa` | 0.2 | 20.0 |
| `Aether/Ostinato_The_Silence_Between.xometa` | 0.05 | 5.0 |
| `Aether/Ostinato_Tongue_Drum_Garden_II.xometa` | 0.1 | 10.0 |
| `Entangled/Oware_x_Ostinato_Fire_Circle.xometa` | 0.15 | 15.0 |
| `Flux/Ostinato_Body_Swap.xometa` | 0.15 | 15.0 |
| `Flux/Ostinato_Gravity_Well.xometa` | 0.15 | 15.0 |
| `Flux/Ostinato_TechnoMachine_Swing_Furnace.xometa` | 0.18 | 18.0 |
| `Prism/Ostinato_Balafon_Twilight.xometa` | 0.33 | 33.0 |
| `Prism/Ostinato_Candombe_Midnight.xometa` | 0.4 | 40.0 |
| `Prism/Ostinato_Ewe_Agbadza.xometa` | 0.33 | 33.0 |
| `Prism/Ostinato_Gnawa_Procession.xometa` | 0.5 | 50.0 |
| `Submerged/Ostinato_Bone_Conductor.xometa` | 0.2 | 20.0 |
| `Submerged/Ostinato_Ghost_Orchestra.xometa` | 0.1 | 10.0 |

---

## Canonical Parameter Reference (OstinatoEngine.h)

### Per-Seat Parameters (N = 1–8)
| Parameter ID | Type | Range | Default |
|-------------|------|-------|---------|
| `osti_seat{N}_instrument` | Choice | 0–11 | varies |
| `osti_seat{N}_articulation` | Choice | 0–3 | 0 |
| `osti_seat{N}_tuning` | Float | -12–12 semitones | 0.0 |
| `osti_seat{N}_decay` | Float | 0.01–2.0s | 0.5 |
| `osti_seat{N}_brightness` | Float | 0–1 | 0.5 |
| `osti_seat{N}_body` | Float | 0–1 | 0.5 |
| `osti_seat{N}_level` | Float | 0–1 | 0.7 |
| `osti_seat{N}_pan` | Float | -1–1 | varies |
| `osti_seat{N}_pattern` | Choice | 0–N | 0 |
| `osti_seat{N}_patternVol` | Float | 0–1 | 0.5 |
| `osti_seat{N}_velSens` | Float | 0–1 | 0.7 |
| `osti_seat{N}_pitchEnv` | Float | -1–1 | 0.0 |
| `osti_seat{N}_exciterMix` | Float | 0–1 | 0.5 |
| `osti_seat{N}_bodyModel` | Choice | 0–4 | 0 |

### Global Parameters
| Parameter ID | Type | Range | Default |
|-------------|------|-------|---------|
| `osti_macroGather` | Float | 0–1 | 0.5 |
| `osti_macroFire` | Float | 0–1 | 0.5 |
| `osti_macroCircle` | Float | 0–1 | 0.0 |
| `osti_macroSpace` | Float | 0–1 | 0.0 |
| `osti_tempo` | Float | 40–300 BPM | 120.0 |
| `osti_swing` | Float | **0–100** | 0.0 |
| `osti_masterTune` | Float | -24–24 semitones | 0.0 |
| `osti_masterDecay` | Float | 0.5–2.0 | 1.0 |
| `osti_masterFilter` | Float | 200–20000 Hz | 18000.0 |
| `osti_masterReso` | Float | 0–1 | 0.1 |
| `osti_reverbSize` | Float | 0–1 | 0.4 |
| `osti_reverbDamp` | Float | 0–1 | 0.3 |
| `osti_reverbMix` | Float | 0–1 | 0.15 |
| `osti_compThresh` | Float | -40–0 dB | -12.0 |
| `osti_compRatio` | Float | 1–20 | 4.0 |
| `osti_compAttack` | Float | 0.1–50 ms | 5.0 |
| `osti_compRelease` | Float | 10–500 ms | 50.0 |
| `osti_circleAmount` | Float | 0–1 | 0.0 |
| `osti_humanize` | Float | 0–1 | 0.3 |
| `osti_masterLevel` | Float | 0–1 | 0.8 |

---

## Post-Fix Validation

Final scan result: **VALIDATION PASSED — no remaining issues** across all 198 Ostinato presets.

- Zero presets with `osti_instrument{N}` (non-canonical) naming
- Zero presets with `osti_swing` in 0–1 normalized range
- Zero presets with any of the known non-canonical global param names

---

## Recommendations for Future Preset Authoring

1. Always use `osti_seat{N}_{param}` pattern for per-seat params, never `osti_{param}{N}`
2. `osti_swing` accepts **percent values (0–100)**, not a 0–1 fraction — match the engine's `NormalisableRange`
3. For coupling presets with sparse Ostinato params, use only canonical param IDs from the table above
4. Non-existent params (e.g., `osti_rhythmDensity`) are silently ignored by APVTS — no error is thrown, so author-side validation is essential
