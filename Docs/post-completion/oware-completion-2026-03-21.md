# OWARE Post-Engine Completion Checklist
**Date:** 2026-03-21
**Engine:** XOware — "The Resonant Board"
**Accent:** Akan Goldweight `#B5883E`
**Identity:** Sunken Akan oware board, coral-encrusted on the Atlantic floor — tuned percussion synthesizer spanning the material continuum from African balafon to Javanese gamelan to Western vibraphone.

---

## 1. Engine Registration in XOceanusProcessor.cpp

**STATUS: PASS**

- `#include "Engines/Oware/OwareEngine.h"` present at line 46
- Registration block at line 232–235:
  ```cpp
  // OWARE — tuned percussion synthesizer (wood/metal material continuum)
  static bool registered_Oware = xoceanus::EngineRegistry::instance().registerEngine(
      "Oware", []() -> std::unique_ptr<xoceanus::SynthEngine> {
          return std::make_unique<xoceanus::OwareEngine>();
  ```
- Engine ID string: `"Oware"` (matches preset `"engines": ["Oware"]` convention)

---

## 2. CLAUDE.md Updated

**STATUS: PASS — all three locations updated**

- **Engine count header (line 8):** "OWARE added 2026-03-20" present in changelog parenthetical
- **Registered engine list (line 11):** `OWARE` appears at the end of the module list
- **Engine table (line 85):** `| OWARE | XOware | Akan Goldweight #B5883E |`
- **Parameter prefix table (line 137):** `| Oware | owr_ | owr_material |`
- **Key files table (line 177):** `Source/Engines/Oware/OwareEngine.h` listed with description
- **Engine count in product description:** Updated to "44 engines"

---

## 3. Parameter Prefix Frozen

**STATUS: PASS — prefix `owr_` frozen**

Full parameter list (22 parameters, all wired to DSP):

| Parameter ID | Type | Display Name | Range | Default |
|---|---|---|---|---|
| `owr_material` | Float | Oware Material | 0–1 | 0.2 |
| `owr_malletHardness` | Float | Oware Mallet Hardness | 0–1 | 0.3 |
| `owr_bodyType` | Int | Oware Body Type | 0–3 | 0 |
| `owr_bodyDepth` | Float | Oware Body Depth | 0–1 | 0.5 |
| `owr_buzzAmount` | Float | Oware Buzz Membrane | 0–1 | 0.0 |
| `owr_sympathyAmount` | Float | Oware Sympathy | 0–1 | 0.3 |
| `owr_shimmerRate` | Float | Oware Shimmer Beat Hz | 0–12 Hz | 6.0 |
| `owr_thermalDrift` | Float | Oware Thermal Drift | 0–1 | 0.3 |
| `owr_brightness` | Float | Oware Brightness | 200–20000 Hz | 8000 |
| `owr_damping` | Float | Oware Damping | 0–1 | 0.3 |
| `owr_decay` | Float | Oware Decay | 0.05–10 s | 2.0 |
| `owr_filterEnvAmount` | Float | Oware Filter Env Amount | 0–1 | 0.3 |
| `owr_bendRange` | Float | Oware Pitch Bend Range | 1–24 semitones | 2 |
| `owr_macroMaterial` | Float | Oware Macro MATERIAL | 0–1 | 0.0 |
| `owr_macroMallet` | Float | Oware Macro MALLET | 0–1 | 0.0 |
| `owr_macroCoupling` | Float | Oware Macro COUPLING | 0–1 | 0.0 |
| `owr_macroSpace` | Float | Oware Macro SPACE | 0–1 | 0.0 |
| `owr_lfo1Rate` | Float | Oware LFO1 Rate | (see engine) | — |
| `owr_lfo1Depth` | Float | Oware LFO1 Depth | 0–1 | — |
| `owr_lfo1Shape` | Int | Oware LFO1 Shape | 0–4 | 0 |
| `owr_lfo2Rate` | Float | Oware LFO2 Rate | (see engine) | — |
| `owr_lfo2Depth` | Float | Oware LFO2 Depth | 0–1 | — |
| `owr_lfo2Shape` | Int | Oware LFO2 Shape | 0–4 | 0 |

**Notes:**
- `owr_bodyType` selects body resonator mode: 0=Tube, 1=Frame, 2=Bowl, 3=Open
- `owr_shimmerRate` uses Balinese beat-frequency model (fixed Hz, not ratio-based)
- `owr_thermalDrift` drives slow shared tuning drift + per-voice personality seed

---

## 4. Preset Count and Mood Distribution

**STATUS: 20 presets across 5 moods**

| Mood | Count | Presets |
|---|---|---|
| Foundation | 6 | Kalimba, Temple Bell, Balafon, Rosewood Marimba, Wind Chimes, Vibraphone |
| Atmosphere | 4 | Dawn Bells, Coral Chime, Deep Resonance, Mbira Constellation |
| Entangled | 3 | Akan Gold, Sunken Bronze, Spirit Board |
| Prism | 4 | Crystal Glass, Singing Bowl, Gamelan Bronze, Glockenspiel |
| Flux | 3 | Metal Rain, Buzz Ritual, Drift Wood |

**Gaps to address:** No presets in Aether, Family, or Submerged moods. Target minimum 3/mood for full mood coverage (9 additional presets needed for complete mood matrix).

---

## 5. Coupling Interface

**STATUS: PASS — 4 CouplingTypes accepted as inputs**

OWARE accepts the following coupling inputs via `applyCouplingInput()`:

| CouplingType | Effect | Scale |
|---|---|---|
| `AmpToFilter` | Modulates brightness/filter cutoff | `val * 2000.0f` Hz offset |
| `LFOToPitch` | Modulates pitch across voices | `val * 2.0f` semitones |
| `AmpToPitch` | Direct amplitude-to-pitch coupling | `val` semitones |
| `EnvToMorph` | Morphs material continuum position | `val` (additive to owr_material) |

OWARE **produces** coupling output via `getSampleForCoupling()` — returns stereo L/R cached samples, making it a valid coupling **source** for other engines.

**Coupling note:** EnvToMorph is the most distinctive pairing target — drives material morphing from any engine's envelope output, enabling cross-engine timbre sculpting. Ideal source engines: ORBITAL, ORGANISM, OXBOW.

---

## 6. Sound Design Guide Entry

**STATUS: MISSING — action required**

`Docs/xoceanus_sound_design_guides.md` contains 0 references to OWARE. The engine was added 2026-03-20; the guides doc was last confirmed at "34 of 34 engines" before OWARE's addition.

**Action required:** Add OWARE section to `Docs/xoceanus_sound_design_guides.md`.

Suggested section outline:
- The 7 Pillars (Material Continuum, Mallet Physics, Sympathetic Resonance, Resonator Body, Buzz Membrane, Breathing Gamelan, Thermal Drift)
- Material dial guide: 0.0=Wood/Balafon, 0.33=Metal/Vibraphone, 0.66=Bell/Gamelan, 1.0=Bowl/Tibetan
- Body type guide: Tube (warm, harmonic), Frame (fixed modal resonance), Bowl (sub-octave bloom), Open (dry, no body)
- Mallet hardness guide: soft (<0.4) = bounce + filtered, hard (>0.7) = full-spectrum strike
- Shimmer (Balinese beat Hz): 0=off, 1-4=slow shimmer, 6-12=fast gamelan beating
- Thermal Drift: 0=perfectly stable, 0.5=alive/vintage, 1.0=drifting instrument

---

## 7. Seance Status

**STATUS: NOT SEANCED — action required**

OWARE was added 2026-03-20. It does not appear in:
- `Docs/fleet-seance-scores-2026-03-20.md` (fleet audit doc only covers 42 engines pre-OWARE)
- `Docs/seances/` (no oware_verdict.md or oware_seance.md found)
- `Docs/seance_cross_reference.md` (no OWARE entries)

The fleet audit notes the fleet count as 42 engines. OWARE (plus OXBOW, added same day) pushes the count to 44. Neither has been seanced.

**Action required:** Schedule OWARE seance. Pre-seance notes for ghost council:
- Engine targets 9.8 via The 7 Pillars — evaluate against that ambition
- D001: velocity drives mallet contact model (spectral lowpass) + bounce trigger threshold — verify timbre response is perceptible across velocity range
- D003: physics citations present (Chaigne 1997, Rossing 2000, Fletcher & Rossing 1998, Adrien 1991, Bilbao 2009) — verify implementation matches citation
- D005: LFO1 rate floor must be ≤ 0.01 Hz — verify
- Thermal Drift pillar is the "alive when nobody's playing" feature — evaluate whether it delivers on promise

---

## 8. Macro Assignments (M1–M4)

**STATUS: PASS — 4 macros defined and wired**

| Slot | Internal Param | Display Label (preset) | Target | Depth |
|---|---|---|---|---|
| M1 | `owr_macroMaterial` | CHARACTER | `owr_material` (material continuum) | ±0.8 |
| M2 | `owr_macroMallet` | MOVEMENT | `owr_malletHardness` + `owr_brightness` | ±0.5 / ±4000 Hz |
| M3 | `owr_macroCoupling` | COUPLING | `owr_sympathyAmount` (sympathetic resonance depth) | ±0.4 |
| M4 | `owr_macroSpace` | SPACE | `owr_bodyDepth` (resonator body coupling) | ±0.3 |

**Notes:**
- M1 (CHARACTER) is the signature macro — sweeps the entire material continuum from wood to metal to bell to bowl, driving the engine's core identity
- M2 (MOVEMENT) drives both mallet hardness and filter brightness simultaneously, making it feel like physically changing mallets mid-performance
- M3 (COUPLING) controls sympathetic resonance network depth — makes other ringing voices respond to the current strike
- M4 (SPACE) adjusts body resonator coupling depth — adds/removes the physical resonator chamber

Preset `macroLabels` array uses standard fleet names: `["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]`
Internal parameter display names (in engine): `MATERIAL`, `MALLET`, `COUPLING`, `SPACE` — these are the parameter human-readable names, not the preset macro labels.

---

## Summary

| Item | Status | Action |
|---|---|---|
| 1. XOceanusProcessor.cpp registration | PASS | None |
| 2. CLAUDE.md updated | PASS | None |
| 3. Parameter prefix frozen (owr_) | PASS | None |
| 4. Preset count (20, 5 moods) | PARTIAL | Add Aether/Family/Submerged presets (9 needed) |
| 5. Coupling interface | PASS | None |
| 6. Sound design guide entry | MISSING | Add OWARE section to xoceanus_sound_design_guides.md |
| 7. Seance status | NOT SEANCED | Schedule seance — see pre-seance notes above |
| 8. Macro assignments | PASS | None |

**Blocking items before V1 release:** Items 4 (partial mood coverage), 6 (no sound design guide entry), 7 (no seance score).
