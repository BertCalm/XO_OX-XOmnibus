# Batch 4 Fleet Audit — Osteria, Owlfish, Ohm, Orphica, Obbligato, Ottoni, Ole, Ombre

**Date:** 2026-03-21
**Auditor:** Rodrigo (audit assistant)
**Scope:** Preset quality for 8 engines across all mood folders
**Mood folders scanned:** Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged
**Non-standard mood folders found (but zero presets for these engines):** Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic

---

## Fleet-Wide Patterns (apply to all 8 engines)

Before per-engine reports, these systemic patterns affect every engine in this batch:

### Systemic Issue A — `tempo`, `couplingIntensity`, `description` missing fleet-wide
Every engine has hundreds of presets missing `tempo`, `couplingIntensity`, and/or `description`. This is clearly a batch-generation gap across the entire fleet, not engine-specific. The most common variant is `tempo` absent alone (these are P2). When all three are missing together (P1).

### Systemic Issue B — Entangled single-engine presets with no coupling
All 8 engines have single-engine presets filed in the Entangled folder with `coupling: null` or `coupling: {"pairs": []}`. These presets have no cross-engine coupling, contradicting the Entangled mood contract. Range: 3 (Osteria) to 26 (Obbligato).

### Systemic Issue C — Naming violations: 4-word+ descriptor keys and single-word names
Two classes of bad names appear at scale:
- **4-7 word descriptor keys** (e.g., `"BRIGHT SPARSE GENTLE FOU 09"`, `"5X DARK COLD KINETIC DENSE VIOLENT ATM 6"`) — these are auto-generated diversity anchors and should never have shipped as final preset names.
- **Single-word names** (e.g., `"Overcast"`, `"Nocturne"`, `"Iridescent"`) — evocative but violate the 2-3 word minimum rule.

### Systemic Issue D — `couplingBus` / `couplingLevel` as parameter keys (wrong namespace)
Multiple presets store coupling-routing metadata (`couplingBus`, `couplingLevel`) inside the `parameters[engine]` block rather than in the `coupling` object. This is a schema violation — these are not engine DSP parameters.

### Systemic Issue E — Flat coupling structure (no `pairs` array)
Several presets use `"coupling": {"type": "...", "source": "...", "target": "...", "amount": ...}` instead of the canonical `"coupling": {"pairs": [...]}` format. The `pairs` array is the standard; the flat format may not be parsed correctly by PresetManager.

### Systemic Issue F — Mood field mismatch (meta vs. folder)
Files in the `Entangled/` folder carry `"mood": "Flux"` or `"mood": "Foundation"` in their JSON. These were likely mis-placed during a bulk generation pass. Ohm, Orphica, Obbligato, Ottoni, and Ole each have 31-37 such mismatches.

### Systemic Issue G — Empty `parameters[engine]` blocks
All engines have a significant count of presets where `parameters[engine]` exists but is an empty object `{}`. These presets have no engine-specific parameter overrides, relying entirely on macro keys — which may not initialize the engine to a musically coherent state.

---

## ENGINE: Osteria

**Preset Count:** 450
**Mood Distribution:** Foundation=66, Atmosphere=39, Entangled=164, Prism=73, Flux=22, Aether=33, Family=53, Submerged=0
**Missing Moods:** Submerged
**Parameter Prefix:** `osteria_` — confirmed correct in engine-specific presets

**DNA Range (across all 450 presets):**
- brightness: [0.005–1.000] range=0.995
- warmth: [0.000–1.000] range=1.000
- movement: [0.028–0.980] range=0.952
- density: [0.027–0.991] range=0.964
- space: [0.020–1.000] range=0.980
- aggression: [0.000–0.987] range=0.987

**DNA Gaps:** None — all 6 dimensions exceed 0.4 range. Full spectrum coverage confirmed.

---

### SAMPLE AUDIT (3 presets)

**1. Open Window** — mood: Foundation
`Foundation/Osteria_Open_Window.xometa`
- **Missing fields:** `tempo` only (P2 — no musical timing metadata)
- **Naming:** PASS — "Open Window" (2 words, evocative, 11 chars)
- **Param prefix:** PASS — all keys use `osteria_` prefix
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE (4, all caps)
- **DNA accuracy:** PASS — all 6 values in [0,1]; coastal/breezy character matches brightness=0.52, space=0.18
- **Coupling:** PASS — Foundation preset, `coupling: {"pairs": []}` appropriate
- **Issues:** `couplingIntensity: "None"` present (good). No `description` field — wait, description present. Only `tempo` missing.

**2. Mediterranean Dusk** — mood: Atmosphere
`Atmosphere/Osteria_Mediterranean_Dusk.xometa`
- **Missing fields:** None from required set — all present including `tempo: null`
- **Naming:** PASS — "Mediterranean Dusk" (2 words, 16 chars)
- **Param prefix:** PASS — all keys use `osteria_` prefix
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE
- **DNA accuracy:** PASS — warmth=0.70, space=0.60 appropriate for closing-time atmosphere
- **Coupling:** PASS — `coupling: null` is acceptable for Atmosphere preset with no pairing
- **Issues:** `osteria_qBassShore`, `osteria_qMelShore`, `osteria_qHarmShore`, `osteria_qRhythmShore`, `osteria_tavernShore`, `osteria_release` all set to 2.0 — values exceeding 1.0 in normalized params is a concern. These may be range-scaled params (e.g., release in seconds), but 2.0 should be verified against the engine's APVTS range definition.

**3. Salt Spray** — mood: Prism
`Prism/Salt_Spray.xometa`
- **Missing fields:** `couplingIntensity`, `description`, `tempo` — multi-field gap (P1 tier)
- **Naming:** PASS — "Salt Spray" (2 words, evocative, 10 chars)
- **Param prefix:** FAIL — `parameters["Osteria"]` is `{}` — empty parameter block (P1)
- **Macro labels:** PASS — WARMTH, SALT, COUPLING, SMOKE (4, all caps; engine-specific labels, not standard set — acceptable)
- **DNA accuracy:** PASS — values present and in [0,1]; has duplicate `sonic_dna` key alongside `dna` (legacy field — P2)
- **Coupling:** PASS for Prism mood
- **Issues:** Empty params block means no DSP parameters set. The `sonic_dna` duplicate field is a schema contamination from an older format.

---

**P0 Issues (blocks shipping):** 0

**P1 Issues (doctrine violations):**
- 58 presets with empty `parameters["Osteria"]` block — engine receives no DSP configuration
- 3 Entangled single-engine presets with no coupling pairs (Entangled contract violation)
- 3 presets with `couplingBus`/`couplingLevel` stored inside parameters block (schema violation — not DSP params)
- 3 presets with mood field mismatch (file in `Entangled/` but `"mood"` says `"Flux"` or `"Foundation"`)
- 6 presets using flat coupling format (no `pairs` array)
- 165 presets missing `couplingIntensity` field
- 151 presets missing `description` field

**P2 Issues (quality concerns):**
- 183 presets missing `tempo` field
- 77 presets with non-compliant names (54 too many words, 23 single word)
- 5 presets missing `coupling` field entirely
- Multiple presets with `sonic_dna` duplicate field (legacy contamination)
- `osteria_release`, `osteria_qBassShore` etc. set to 2.0 in Mediterranean Dusk — verify these are range-correct for their APVTS definitions

**P3 Issues (polish items):**
- 0 Submerged presets — Submerged mood unrepresented
- Entangled count (164) is very high relative to other moods; some may be misplaced (3 confirmed mood-field mismatches)
- Macro labels vary across presets (WARMTH/SALT/SMOKE vs. CHARACTER/MOVEMENT) — acceptable if documented, but inconsistent across library

---

## ENGINE: Owlfish

**Preset Count:** 351
**Mood Distribution:** Foundation=26, Atmosphere=23, Entangled=158, Prism=63, Flux=33, Aether=25, Family=14, Submerged=9
**Missing Moods:** None (all 8 standard moods represented)
**Parameter Prefix:** `owl_` — confirmed correct in engine-specific presets

**DNA Range (across all 351 presets):**
- brightness: [0.000–1.000] range=1.000
- warmth: [0.000–0.999] range=0.999
- movement: [0.039–1.000] range=0.961
- density: [0.023–1.000] range=0.977
- space: [0.000–1.000] range=1.000
- aggression: [0.000–0.999] range=0.999

**DNA Gaps:** None — exemplary full-spectrum coverage across all 6 dimensions.

---

### SAMPLE AUDIT (3 presets)

**1. BRIGHT SPARSE GENTLE FOU 14** — mood: Foundation
`Foundation/BRIGHT_SPARSE_GENTLE_FOU_14.xometa`
- **Missing fields:** `tempo`, `description`, `couplingIntensity` — multi-field gap (P1)
- **Naming:** FAIL — "BRIGHT SPARSE GENTLE FOU 14" (5 words + number, 27 chars — descriptor key, not an evocative name)
- **Param prefix:** FAIL — Owlfish params use `macro_character`, `macro_movement` etc. — generic `macro_` keys, not `owl_` prefix. This is a multi-engine preset (Odyssey + Owlfish); for multi-engine presets, macro delegation keys are acceptable per coupling-preset-designer pattern. CONDITIONAL PASS.
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE (4, caps)
- **DNA accuracy:** PASS — values in [0,1]
- **Coupling:** Has `AMPLITUDE_MOD` pair between Odyssey→Owlfish, amount=0.667 — PASS
- **Issues:** Name is a raw diversity-anchor label. Filed in Foundation but has coupling — could be Entangled.

**2. ERODING MIST** — mood: Atmosphere
`Atmosphere/ERODING_MIST.xometa`
- **Missing fields:** `tempo`, `description`, `couplingIntensity` — multi-field gap (P1)
- **Naming:** FAIL — "ERODING MIST" (2 words, all-caps shouting, 12 chars) — caps OK for Prism/extreme presets but "Eroding Mist" in Atmosphere is odd naming
- **Param prefix:** CONDITIONAL PASS — multi-engine (Orphica + Owlfish), macro_character delegation
- **Macro labels:** PASS
- **DNA accuracy:** PASS — aggression=0.91 in Atmosphere is questionable (Atmosphere mood usually implies non-aggressive)
- **Coupling:** Has `HARMONIC_FOLD` pair, amount=0.627 — PASS
- **Issues:** Filed as Atmosphere but DNA has aggression=0.91 — extreme aggression more characteristic of Flux/Prism. Mood-DNA mismatch concern. Name styling inconsistent (all-caps screaming in a calm mood).

**3. Rust Dense** (file: `DARK_HOT_KINETIC_DENSE_PRS_DRK_5.xometa`) — mood: Prism
`Prism/DARK_HOT_KINETIC_DENSE_PRS_DRK_5.xometa`
- **Missing fields:** `tempo`, `description`, `couplingIntensity` — multi-field gap (P1)
- **Naming:** PASS — name is "Rust Dense" (2 words, 9 chars) — good name, but filename is the old descriptor key
- **Param prefix:** CONDITIONAL PASS — multi-engine (Ohm + Owlfish), macro_character delegation
- **Macro labels:** PASS
- **DNA accuracy:** PASS — warmth=0.952, density=0.946 appropriate for "Rust Dense" Prism preset
- **Coupling:** Has `TIMBRE_BLEND` pair, amount=0.9458 — PASS
- **Issues:** Filename is the old descriptor key but display name is correct. File should be renamed to match display name (P3).

---

**P0 Issues (blocks shipping):** 0

**P1 Issues (doctrine violations):**
- 35 presets with empty `parameters["Owlfish"]` block
- 6 Entangled single-engine presets with no coupling pairs
- 3 presets with `couplingBus`/`couplingLevel` in parameters block
- 6 mood field mismatches (file folder vs. JSON `"mood"` field)
- 6 flat coupling format (no `pairs` array)
- 140 presets missing `description`
- 137 presets missing `couplingIntensity`

**P2 Issues (quality concerns):**
- 187 presets missing `tempo`
- 61 presets with non-compliant names (47 too many words, 14 single word)
- Aggression=0.91 filed in Atmosphere mood (ERODING MIST) — DNA-mood mismatch
- Filename vs. display-name mismatches (descriptor filenames with good display names inside)
- Family count (14) is notably thin — lowest non-zero mood for Owlfish

**P3 Issues (polish items):**
- 158 Entangled presets is the highest proportion by far (45% of library) — consider redistributing balanced coverage
- Descriptor filenames should be renamed to match their display name for discoverability

---

## ENGINE: Ohm

**Preset Count:** 577
**Mood Distribution:** Foundation=42, Atmosphere=38, Entangled=271, Prism=77, Flux=37, Aether=44, Family=69, Submerged=0
**Missing Moods:** Submerged
**Parameter Prefix:** `ohm_` — confirmed correct

**DNA Range (across all 577 presets):**
- brightness: [0.005–1.000] range=0.995
- warmth: [0.000–1.000] range=1.000
- movement: [0.003–1.000] range=0.997
- density: [0.020–0.990] range=0.970
- space: [0.022–1.000] range=0.978
- aggression: [0.000–0.998] range=0.998

**DNA Gaps:** None — outstanding full-spectrum coverage.

---

### SAMPLE AUDIT (3 presets)

**1. Joint Venture** — mood: Foundation
`Foundation/Ohm_Joint_Venture.xometa`
- **Missing fields:** `tempo` only (P2)
- **Naming:** PASS — "Joint Venture" (2 words, evocative, 13 chars)
- **Param prefix:** PASS — all keys use `ohm_` prefix
- **Macro labels:** FAIL — JAM, MEDDLING, COMMUNE, MEADOW (4, all caps — engine-specific labels, not CHARACTER/MOVEMENT/COUPLING/SPACE). Engine-specific labels are acceptable if the macros are wired to `ohm_macroJam`, `ohm_macroMeddling` etc. — confirmed wired in params. PASS (engine-specific label set is legitimate).
- **DNA accuracy:** PASS — all 6 in [0,1]; moderate values match Foundation character
- **Coupling:** PASS — `coupling: null`, Foundation preset
- **Issues:** None significant

**2. Amber Glow Ohm** — mood: Atmosphere
`Atmosphere/Ohm_Amber_Glow.xometa`
- **Missing fields:** `tempo` only (P2)
- **Naming:** FAIL — "Amber Glow Ohm" (3 words with engine suffix, 14 chars) — "Ohm" suffix is redundant metadata, not evocative content. Should be "Amber Glow" (2 words).
- **Param prefix:** PASS — all `ohm_` prefix
- **Macro labels:** PASS — JAM, MEDDLING, COMMUNE, MEADOW (engine-specific set, wired)
- **DNA accuracy:** PASS — warmth=0.54, space=0.77 appropriate for soft drift atmosphere
- **Coupling:** PASS — `coupling: null`, Atmosphere preset
- **Issues:** Engine name as third word in preset name is a naming anti-pattern seen fleet-wide for Ohm. Affects readability when browsing preset list.

**3. Consensus Field** — mood: Aether
`Aether/Ohm_Consensus_Field.xometa`
- **Missing fields:** None from required set (all present including `sequencer: null`)
- **Naming:** FAIL — "Consensus Field" is the evocative name but the first part of file is `Ohm_` prefix — the actual display name "Consensus Field" is fine (2 words). If we evaluate the name field alone: PASS.
- **Param prefix:** PASS — all `ohm_` prefix. Notable: uses `ohm_macroCoupling` and `ohm_macroSpace` instead of the standard `ohm_macroCommune`/`ohm_macroMeadow` — suggests macro label set was changed between presets without updating parameter keys.
- **Macro labels:** FAIL — this preset uses `"macroLabels": ["MEDDLING", "COMMUNE", "COUPLING", "SPACE"]` — mixed set (COUPLING and SPACE are standard names, MEDDLING and COMMUNE are engine-specific). Parameter keys include `ohm_macroCoupling` and `ohm_macroSpace` (standard names) but also `ohm_macroCommune` and `ohm_macroMeddling` (engine names). Mismatch between macroLabels and parameter keys. P1.
- **DNA accuracy:** PASS — all 6 in [0,1]
- **Issues:** Macro label/param key mismatch is a P1 schema inconsistency. The preset has `COUPLING` in macroLabels but also `ohm_macroCoupling` as a param key — this is self-consistent. But the same preset also has `ohm_macroCommune` — so 5 macro params are present for a 4-macro system. Duplicate macro wiring.

---

**P0 Issues (blocks shipping):** 0

**P1 Issues (doctrine violations):**
- 103 presets with empty `parameters["Ohm"]` block — highest count in batch
- 19 Entangled single-engine presets with no coupling pairs
- 31 mood field mismatches (highest in batch alongside Ole/Ottoni)
- 8 presets with `couplingBus`/`couplingLevel` or similar in parameters block
- 7 flat coupling format presets
- 240 presets missing `couplingIntensity`
- 181 presets missing `description`
- Macro label/parameter key mismatch in Consensus Field (COUPLING/SPACE labels with ohm_macroCommune/ohm_macroMeadow params present simultaneously)

**P2 Issues (quality concerns):**
- 389 presets missing `tempo` (highest absolute count in batch — reflects large library size)
- 76 presets with non-compliant names (59 too many words, 17 single word)
- Engine suffix pattern (e.g., "Amber Glow Ohm") — reduces preset list readability, wastes one of 3 word slots

**P3 Issues (polish items):**
- 0 Submerged presets
- Entangled (271) is 47% of library — disproportionate
- Engine name prefix in filenames (`Ohm_`) is helpful for sorting but display name in JSON should not include "Ohm" as a word

---

## ENGINE: Orphica

**Preset Count:** 554
**Mood Distribution:** Foundation=37, Atmosphere=39, Entangled=213, Prism=92, Flux=43, Aether=47, Family=79, Submerged=5
**Missing Moods:** None (all 8 standard moods represented; Submerged=5 is thin but present)
**Parameter Prefix:** `orph_` — confirmed correct

**DNA Range (across all 554 presets):**
- brightness: [0.023–1.000] range=0.977
- warmth: [0.000–0.988] range=0.988
- movement: [0.000–0.999] range=0.999
- density: [0.007–1.000] range=0.993
- space: [0.009–1.000] range=0.991
- aggression: [0.000–0.986] range=0.986

**DNA Gaps:** None — excellent full-spectrum coverage.

---

### SAMPLE AUDIT (3 presets)

**1. Zooid Chain** — mood: Foundation
`Foundation/Orphica_Zooid_Chain.xometa`
- **Missing fields:** `tempo` only (P2)
- **Naming:** PASS — "Zooid Chain" (2 words, evocative, 11 chars)
- **Param prefix:** PASS — all keys use `orph_` prefix
- **Macro labels:** PASS — PLUCK, FRACTURE, SURFACE, DIVINE (4, all caps — engine-specific, wired to `orph_macroPluck` etc.)
- **DNA accuracy:** PASS — all in [0,1]; cautious values (brightness=0.5, movement=0.43) fit Foundation character
- **Coupling:** PASS — `coupling: null`, Foundation preset
- **Issues:** `orph_crossoverBlend: 1` and `orph_crossoverNote: 1` — integer values in what may be float params. If crossoverNote is a mode selector (index), integer is correct. Verify against engine schema.

**2. DARK HOT VAST KINETIC F5** — mood: Flux
`Flux/DARK_HOT_VAST_KINETIC_F5.xometa`
- **Missing fields:** `couplingIntensity`, `description`, `tempo` — multi-field gap (P1)
- **Naming:** FAIL — "DARK HOT VAST KINETIC F5" (5 words, 23 chars — raw descriptor key)
- **Param prefix:** FAIL — `parameters["Orphica"]` is `{}` — empty (P1)
- **Macro labels:** FAIL — MACRO1, MACRO2, MACRO3, MACRO4 — placeholder labels never replaced (P1)
- **DNA accuracy:** PASS — duplicate `sonic_dna` alongside `dna` (P2 legacy field)
- **Coupling:** PASS — Flux preset, empty pairs appropriate
- **Issues:** This is a pure diversity anchor that shipped without being finished. Placeholder macro labels, empty params, descriptor name, missing description — all P1.

**3. Velocity Siren II** — mood: Entangled
`Entangled/Velocity_Siren.xometa`
- **Missing fields:** `couplingIntensity`, `tempo` — P1/P2
- **Naming:** PASS — "Velocity Siren II" (3 words, 16 chars) — the "II" indicates a series, acceptable
- **Param prefix:** PASS — multi-engine (Orphica + Ouroboros); macro delegation pattern used
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE
- **DNA accuracy:** PASS — aggression=0.0 is notable (exact zero, not a rounding artifact); confirm engine behavior at zero aggression
- **Coupling:** PASS — VELOCITY_COUPLE pair, amount=0.72, > 0.1 threshold
- **Issues:** aggression=0.0 (exact) for an Entangled preset with velocity coupling is a DNA-concept mismatch (velocity coupling implies some aggression response). Minor concern.

---

**P0 Issues (blocks shipping):** 0

**P1 Issues (doctrine violations):**
- 71 presets with empty `parameters["Orphica"]` block
- 19 Entangled single-engine presets with no coupling pairs
- 36 mood field mismatches (file folder vs. JSON mood field)
- 8 flat coupling format presets
- 222 presets missing `couplingIntensity`
- 174 presets missing `description`
- Unfinished diversity-anchor presets with `"macroLabels": ["MACRO1","MACRO2","MACRO3","MACRO4"]` — placeholder macros shipped (see DARK HOT VAST KINETIC F5)

**P2 Issues (quality concerns):**
- 371 presets missing `tempo`
- 66 presets with non-compliant names (53 too many words, 13 single word)
- `sonic_dna` duplicate field on some presets (legacy contamination)
- aggression=0.0 (exact zero) in Entangled velocity-coupling preset

**P3 Issues (polish items):**
- Submerged=5 is the thinnest mood in this engine's library; consider expanding
- Entangled (213) is 38% of library

---

## ENGINE: Obbligato

**Preset Count:** 548
**Mood Distribution:** Foundation=47, Atmosphere=40, Entangled=275, Prism=36, Flux=36, Aether=38, Family=77, Submerged=0
**Missing Moods:** Submerged
**Parameter Prefix:** `obbl_` — confirmed correct

**DNA Range (across all 548 presets):**
- brightness: [0.019–1.000] range=0.981
- warmth: [0.000–1.000] range=1.000
- movement: [0.015–1.000] range=0.985
- density: [0.017–1.000] range=0.983
- space: [0.019–1.000] range=0.981
- aggression: [0.000–0.989] range=0.989

**DNA Gaps:** None.

---

### SAMPLE AUDIT (3 presets)

**1. Bassoon Low** — mood: Foundation
`Foundation/Obbligato_Bassoon_Low.xometa`
- **Missing fields:** None — all required fields present (`tempo: null`, `couplingIntensity: "None"`)
- **Naming:** PASS — "Bassoon Low" (2 words, 11 chars, evocative instrument+register)
- **Param prefix:** PASS — all keys use `obbl_` prefix
- **Macro labels:** PASS — BOND, BREATH, WIND, MISCHIEF (4, all caps — engine-specific, correctly wired)
- **DNA accuracy:** PASS — warmth=0.75, movement=0.2, brightness=0.3 fit a slow-moving bassoon foundation
- **Coupling:** PASS — `coupling: {"pairs": []}`, Foundation preset
- **Issues:** `macros` block (top-level) duplicates what is in `parameters["Obbligato"]` — this is acceptable (macro summary block). Well-formed preset overall.

**2. Forest Wind** — mood: Atmosphere
`Atmosphere/Obbligato_Forest_Wind.xometa`
- **Missing fields:** None — fully complete
- **Naming:** PASS — "Forest Wind" (2 words, 11 chars)
- **Param prefix:** PASS — all `obbl_` prefix
- **Macro labels:** PASS — BOND, BREATH, WIND, MISCHIEF
- **DNA accuracy:** PASS — movement=0.48, space=0.80 appropriate for outdoor atmosphere
- **Coupling:** PASS — `coupling: {"pairs": []}`, Atmosphere preset
- **Issues:** `obbl_bondStage: 1` is an integer index — correct for enum param. Well-formed.

**3. Transcend Bond** — mood: Aether
`Aether/Obbligato_Transcend_Bond.xometa`
- **Missing fields:** `tempo` only (P2)
- **Naming:** PASS — "Transcend Bond" (2 words, 14 chars)
- **Param prefix:** PASS — all `obbl_` prefix
- **Macro labels:** PASS — BREATH, BOND, MISCHIEF, WIND (4, caps — reordered from other presets, labels remain consistent across the 4 values)
- **DNA accuracy:** PASS — all in [0,1]
- **Coupling:** FAIL — `coupling: null` in Aether preset — this is acceptable for Aether (not Entangled), but `coupling: null` vs `coupling: {"pairs": []}` is inconsistent across the library
- **Issues:** `obbl_bondStage: 0.232` — this appears to be a float value for what is likely an integer/enum parameter (stage index). If bondStage is an integer enum (as seen in Bassoon Low where it's 0 or 1), storing 0.232 is semantically wrong. P1.

---

**P0 Issues (blocks shipping):** 0

**P1 Issues (doctrine violations):**
- 85 presets with empty `parameters["Obbligato"]` block
- 26 Entangled single-engine presets with no coupling pairs — highest count in batch
- 31 mood field mismatches
- 7 flat coupling format presets
- 187 presets missing `couplingIntensity`
- 146 presets missing `description`
- `obbl_bondStage: 0.232` in Transcend Bond — float stored in likely-integer enum parameter

**P2 Issues (quality concerns):**
- 334 presets missing `tempo`
- 97 presets with non-compliant names (88 too many words, 9 single word) — highest word-count violation count in batch
- `coupling: null` vs `coupling: {"pairs": []}` inconsistency across library
- Prism (36) is thin relative to other moods

**P3 Issues (polish items):**
- 0 Submerged presets
- Entangled (275) is 50% of library — highest ratio in batch
- Macro label order varies across presets (BOND, BREATH, WIND, MISCHIEF vs. BREATH, BOND, MISCHIEF, WIND) — the 4 labels are the same set but in different positions, which means M1-M4 knobs control different things per preset. This is a significant UX inconsistency.

---

## ENGINE: Ottoni

**Preset Count:** 522
**Mood Distribution:** Foundation=49, Atmosphere=33, Entangled=224, Prism=76, Flux=35, Aether=24, Family=80, Submerged=2
**Missing Moods:** None (Submerged=2 present but very thin)
**Parameter Prefix:** `otto_` — confirmed correct

**DNA Range (across all 522 presets):**
- brightness: [0.021–1.000] range=0.979
- warmth: [0.015–1.000] range=0.985
- movement: [0.020–0.989] range=0.969
- density: [0.007–0.990] range=0.983
- space: [0.031–0.996] range=0.965
- aggression: [0.000–0.988] range=0.988

**DNA Gaps:** None — all dimensions exceed 0.4 range. Note: warmth minimum is 0.015 (not zero), space minimum is 0.031 (not zero) — low end slightly elevated but still within acceptable range.

---

### SAMPLE AUDIT (3 presets)

**1. Flugelhorn Warm** — mood: Foundation
`Foundation/Ottoni_Flugelhorn_Warm.xometa`
- **Missing fields:** None — all required fields present
- **Naming:** PASS — "Flugelhorn Warm" (2 words, 15 chars, instrument+character descriptor)
- **Param prefix:** PASS — all `otto_` prefix
- **Macro labels:** PASS — GROW, EMBOUCHURE, LAKE, FOREIGN (4, all caps — engine-specific, wired to `otto_macroGrow` etc.)
- **DNA accuracy:** PASS — warmth=0.72, brightness=0.42 appropriate for mellow flugelhorn
- **Coupling:** PASS — Foundation preset, empty pairs
- **Issues:** `otto_macroForeign: 0.0` — macro at zero may mean foreign voices are completely silent. If FOREIGN is an always-on macro (even at zero it contributes some character), this is fine. If zero means silent foreign voices, the preset character depends entirely on teen voice. Not a schema error, but a sound design note.

**2. SPARSE FLUX** — mood: Flux
`Flux/SPARSE_FLUX.xometa`
- **Missing fields:** `couplingIntensity`, `description`, `tempo` — multi-field gap (P1)
- **Naming:** FAIL — "SPARSE FLUX" (2 words, 10 chars) — all-caps shouting, "FLUX" is mood metadata not an evocative word. Better: "Sparse Terrain" or similar. Mood name embedded in preset name is an anti-pattern.
- **Param prefix:** PASS — multi-engine (Oblong + Ottoni); Oblong params include `bob_curAmount`, `bob_lfo1Depth` — correct Oblong prefix. Ottoni uses macro delegation.
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE
- **DNA accuracy:** PASS — values in [0,1]; density=0.054 (near-zero) fits "sparse" character
- **Coupling:** PASS — FREQUENCY_SHIFT pair, amount=0.7481 > 0.1
- **Issues:** Name includes mood as a word ("FLUX"). All-caps shouting style is inconsistent with Foundation/Atmosphere conventions.

**3. Golden Molten 2** — mood: Family
`Family/BRIGHT_WARM_DENSE_INTIMATE_FAM_VOL_53.xometa`
- **Missing fields:** `couplingIntensity`, `description`, `tempo` — multi-field gap (P1)
- **Naming:** PASS — display name "Golden Molten 2" (3 words, 14 chars). Filename is raw descriptor but display name is good.
- **Param prefix:** PASS — multi-engine (Ottoni + Ohm), macro delegation
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE
- **DNA accuracy:** PASS — density=0.971, warmth=0.907 fit Family warm-dense character
- **Coupling:** PASS — RESONANCE_SHARE pair, amount=0.787
- **Issues:** Filename vs. display name mismatch. Series numbering ("2") — acceptable for sequential sets.

---

**P0 Issues (blocks shipping):** 0

**P1 Issues (doctrine violations):**
- 74 presets with empty `parameters["Ottoni"]` block
- 20 Entangled single-engine presets with no coupling pairs
- 37 mood field mismatches — tied with Ole for highest in batch
- 3 flat coupling format presets
- 203 presets missing `couplingIntensity`
- 155 presets missing `description`

**P2 Issues (quality concerns):**
- 349 presets missing `tempo`
- 72 presets with non-compliant names (55 too many words, 17 single word)
- Mood name embedded in preset name ("SPARSE FLUX") — naming anti-pattern
- Submerged=2 is extremely thin — functionally absent

**P3 Issues (polish items):**
- Aether (24) is thin relative to Foundation (49) and Family (80)
- Entangled (224) is 43% of library
- `otto_macroForeign: 0.0` as default in multiple presets — may mean foreign voices never heard on init

---

## ENGINE: Ole

**Preset Count:** 517
**Mood Distribution:** Foundation=36, Atmosphere=39, Entangled=202, Prism=71, Flux=41, Aether=37, Family=92, Submerged=0
**Missing Moods:** Submerged
**Parameter Prefix:** `ole_` — confirmed correct

**DNA Range (across all 517 presets):**
- brightness: [0.007–1.000] range=0.993
- warmth: [0.026–0.985] range=0.959
- movement: [0.014–1.000] range=0.986
- density: [0.011–0.990] range=0.979
- space: [0.000–1.000] range=1.000
- aggression: [0.000–1.000] range=1.000

**DNA Gaps:** None — excellent coverage. Note: warmth max is 0.985 (not 1.0) — very minor ceiling effect but not flagged.

---

### SAMPLE AUDIT (3 presets)

**1. Quartet Mode** — mood: Foundation
`Foundation/Ole_Quartet_Mode.xometa`
- **Missing fields:** None — fully complete (`tempo: null`, `couplingIntensity: "None"`)
- **Naming:** PASS — "Quartet Mode" (2 words, 12 chars, evocative)
- **Param prefix:** PASS — all `ole_` prefix
- **Macro labels:** PASS — DRAMA, FUEGO, ISLA, SIDES (4, all caps — engine-specific, wired to `ole_macroDrama` etc.)
- **DNA accuracy:** PASS — moderate values fit Foundation character
- **Coupling:** PASS — empty pairs, Foundation preset
- **Issues:** Well-formed preset. The `macros` summary block present and matches params.

**2. Soft Rhythm** — mood: Atmosphere
`Atmosphere/Ole_Soft_Rhythm.xometa`
- **Missing fields:** `couplingIntensity`, `tempo` — P1/P2
- **Naming:** PASS — "Soft Rhythm" (2 words, 11 chars)
- **Param prefix:** PARTIAL FAIL — parameters include `ole_couplingBus: 0`, `ole_couplingLevel: 0.35` — these are coupling-routing values stored as engine parameters. The `couplingBus`/`couplingLevel` pattern should live in the `coupling` object, not in `parameters["Ole"]`. P1 schema violation.
- **Macro labels:** FAIL — CHARACTER, MOVEMENT, COUPLING, SPACE — but the `parameters["Ole"]` block does NOT contain `ole_macroDrama`, `ole_macroFuego` etc. (the engine's actual macro params). Instead it has `ole_drama`, `ole_rhythmA`, `ole_rhythmB` etc. The macroLabels don't match the actual parameter names. The CHARACTER/MOVEMENT/COUPLING/SPACE macro set implies the coupling-centric macro pattern, but the Ole engine uses DRAMA/FUEGO/ISLA/SIDES. This is a P1 macro mismatch.
- **DNA accuracy:** PASS — values in [0,1]; duplicate `sonic_dna` field (legacy P2)
- **Coupling:** PASS — Atmosphere preset, empty pairs
- **Issues:** Macro label mismatch (generic CHARACTER/MOVEMENT vs. engine-specific DRAMA/FUEGO). Coupling routing params in parameters block.

**3. Drift Latin** — mood: Flux
`Flux/Ole_Drift_Latin.xometa`
- **Missing fields:** `tempo` only (P2)
- **Naming:** PASS — "Drift Latin" (2 words, 11 chars)
- **Param prefix:** PASS — all `ole_` prefix
- **Macro labels:** PASS — FUEGO, DRAMA, SIDES, ISLA (engine-specific set, reordered but present)
- **DNA accuracy:** PASS — all in [0,1]; aggression=0.47 appropriate for flux/movement character
- **Coupling:** PASS — `coupling: null`, Flux preset
- **Issues:** Macro label order different from Quartet Mode (DRAMA, FUEGO, ISLA, SIDES vs. FUEGO, DRAMA, SIDES, ISLA) — the M1 knob controls DRAMA in one preset and FUEGO in another. Same P3 UX issue as Obbligato.

---

**P0 Issues (blocks shipping):** 0

**P1 Issues (doctrine violations):**
- 61 presets with empty `parameters["Ole"]` block
- 20 Entangled single-engine presets with no coupling pairs
- 37 mood field mismatches — tied for highest in batch
- 3 flat coupling format presets
- 217 presets missing `couplingIntensity`
- 169 presets missing `description`
- `ole_couplingBus` / `ole_couplingLevel` stored in engine parameters block (schema violation)
- Macro label mismatch in Soft Rhythm: CHARACTER/MOVEMENT/COUPLING/SPACE labels but engine-specific params underneath

**P2 Issues (quality concerns):**
- 358 presets missing `tempo`
- 58 presets with non-compliant names (44 too many words, 14 single word)
- `sonic_dna` duplicate field on some presets
- Macro label order varies per preset (M1=DRAMA in some, M1=FUEGO in others)

**P3 Issues (polish items):**
- 0 Submerged presets
- Family (92) is by far the largest mood — reflects Ole's ensemble character, but creates imbalance
- Entangled (202) is 39% of library

---

## ENGINE: Ombre

**Preset Count:** 428
**Mood Distribution:** Foundation=35, Atmosphere=45, Entangled=174, Prism=30, Flux=42, Aether=80, Family=23, Submerged=0
**Missing Moods:** Submerged
**Parameter Prefix:** `ombre_` — confirmed correct for well-formed presets; 4 presets found using wrong prefix `ombr_` (missing trailing 'e')

**DNA Range (across all 428 presets):**
- brightness: [0.020–0.990] range=0.970
- warmth: [0.026–0.984] range=0.958
- movement: [0.015–0.987] range=0.972
- density: [0.000–1.000] range=1.000
- space: [0.000–0.996] range=0.996
- aggression: [0.000–0.986] range=0.986

**DNA Gaps:** None — all dimensions exceed 0.4. Note: brightness max is 0.990 and warmth max is 0.984 — slight ceiling caps not reaching 1.0, minor.

---

### SAMPLE AUDIT (3 presets)

**1. Clear Beginning** — mood: Foundation
`Foundation/Ombre_Clear_Beginning.xometa`
- **Missing fields:** `couplingIntensity` only (P1)
- **Naming:** PASS — "Clear Beginning" (2 words, 15 chars)
- **Param prefix:** PASS — all `ombre_` prefix
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE (4, all caps)
- **DNA accuracy:** PASS — brightness=0.7, movement=0.2 fit the "pure Opsis, immediate" concept
- **Coupling:** PASS — `coupling: null`, Foundation preset. Note: no `"coupling"` field at all is a P2 omission (should be `null` or `{"pairs": []}`)
- **Issues:** `couplingIntensity` missing. `coupling: null` is acceptable but `coupling` field absent entirely means PresetManager may need a null-guard.

**2. BRIGHT WARM KINETIC BALANCE...** — mood: Aether
`Aether/BRIGHT_WARM_KINETIC_BALANCED_AET_SPC_5.xometa`
- **Missing fields:** `couplingIntensity`, `description`, `tempo` — multi-field gap (P1)
- **Naming:** FAIL — display name "BRIGHT WARM KINETIC BALANCE..." is a truncated descriptor (30 chars exactly but truncated). This is an unfinished diversity anchor.
- **Param prefix:** PASS — multi-engine (Ombre + OddfeliX); macro delegation
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE
- **DNA accuracy:** PASS — values in [0,1]
- **Coupling:** PASS — PITCH_SYNC pair, amount=0.777
- **Issues:** Truncated name is the descriptor key name, not a real preset name. This is an unfinished diversity anchor shipped without a proper name.

**3. Violent Shade Drift** — mood: Atmosphere
`Atmosphere/Ombr_Obsc_Violent_Shade_Drift.xometa`
- **Missing fields:** `couplingIntensity` present (value="High") — `tempo` missing (P2)
- **Naming:** PASS — "Violent Shade Drift" (3 words, 19 chars) — acceptable; "Violent" is evocative
- **Param prefix:** PASS — multi-engine (Ombre + Obscura); macro delegation
- **Macro labels:** PASS — CHARACTER, MOVEMENT, COUPLING, SPACE
- **DNA accuracy:** PASS — aggression=0.919 in Atmosphere is aggressive for that mood but acceptable as an extreme preset
- **Coupling:** FAIL — uses flat coupling format: `{"type": "SPECTRAL_BLEND", "source": "OMBRE", "target": "OBSCURA", "amount": 0.75}` — no `pairs` array (P1 schema violation)
- **Issues:** Flat coupling format. DNA-mood mismatch (aggression=0.919 in Atmosphere). File named `Ombr_Obsc_...` (missing 'e' in prefix pattern — though this is the filename, not a param key, so it's P3 only).

---

**P0 Issues (blocks shipping):** 0

**P1 Issues (doctrine violations):**
- 45 presets with empty `parameters["Ombre"]` block
- 12 Entangled single-engine presets with no coupling pairs
- 4 mood field mismatches
- 2 presets with `ombr_` prefix instead of `ombre_` (Gradient Dusk, Twilight Fade) — wrong parameter prefix, engine will not recognize these params
- 2 presets with non-engine keys in parameters block (coupling/gradient/space/warmth as bare keys without prefix — Penumbra Drift II, Tarpit Horizon)
- 5 flat coupling format presets
- 170 presets missing `couplingIntensity`
- 152 presets missing `description`

**P2 Issues (quality concerns):**
- 201 presets missing `tempo`
- 79 presets with non-compliant names (50 too many words, 29 single word) — single-word names are the biggest issue here (highest count in batch: 29)
- Aether (80) is very high relative to Family (23) and Prism (30) — skewed distribution
- brightness and warmth max slightly below 1.0 (ceiling effect at 0.984/0.990)
- Aggression=0.919 in Atmosphere mood (Violent Shade Drift)

**P3 Issues (polish items):**
- 0 Submerged presets
- Family (23) is the thinnest standard mood — Ombre's shadow/memory character seems underrepresented in collaborative Family context
- Single-word evocative names (Overcast, Nocturne, Penumbra, etc.) are beautiful but violate the 2-word minimum — a name policy exception should either be documented or names should be expanded

---

## Cross-Engine Summary Table

| Engine | Total | Submerged | Empty Params | Missing desc+ci | Bad Names | Entangled No-Coupling | Mood Mismatch | Bad Prefix |
|--------|-------|-----------|-------------|-----------------|-----------|----------------------|---------------|------------|
| Osteria | 450 | 0 | 58 | 165 | 77 | 3 | 3 | 3 (couplingBus) |
| Owlfish | 351 | 9 | 35 | 140 | 61 | 6 | 6 | 3 (couplingBus) |
| Ohm | 577 | 0 | 103 | 242 | 76 | 19 | 31 | 8 (couplingBus) |
| Orphica | 554 | 5 | 71 | 224 | 66 | 19 | 36 | 8 (couplingBus) |
| Obbligato | 548 | 0 | 85 | 193 | 97 | 26 | 31 | 7 (couplingBus) |
| Ottoni | 522 | 2 | 74 | 205 | 72 | 20 | 37 | 3 (couplingBus) |
| Ole | 517 | 0 | 61 | 219 | 58 | 20 | 37 | 3 (couplingBus+raw) |
| Ombre | 428 | 0 | 45 | 170 | 79 | 12 | 4 | 7 (ombr_ typo + bare keys) |
| **TOTALS** | **3397** | | **532** | | **586** | **125** | **185** | |

---

## Priority Fix Recommendations

### Immediate (P1 batch fixes — script-addressable)

1. **`couplingBus`/`couplingLevel` removal from parameters blocks** — These 7 Osteria/Owlfish Entangled presets have non-DSP keys in the engine parameter block. Move to `coupling` object or remove.

2. **`ombr_` prefix typo in Ombre presets** — Gradient Dusk and Twilight Fade use `ombr_` instead of `ombre_`. Engine APVTS will not recognize these parameters. Fix: rename all `ombr_` keys to `ombre_`.

3. **Penumbra Drift II / Tarpit Horizon bare keys** — Parameters block contains `coupling`, `gradient`, `space`, `warmth` without engine prefix. These are not engine DSP params. Remove or prefix correctly.

4. **Flat coupling format standardization** — 5-8 presets per engine using `{"type":..., "source":..., "target":..., "amount":...}` format instead of `{"pairs": [...]}`. Migrate to pairs format.

5. **Entangled single-engine no-coupling presets** — 125 presets total across 8 engines. These are filed in Entangled but have no coupling definition. Either add coupling pairs or relocate to appropriate mood folder.

6. **MACRO1/MACRO2/MACRO3/MACRO4 placeholder labels** — Unfinished diversity anchors shipped with placeholder macro labels. These must either receive proper labels or be removed.

7. **Mood field mismatch** — 185 presets have a `"mood"` JSON field that disagrees with their folder. This must be corrected to prevent PresetManager confusion.

### High Priority (P1/P2 — requires editorial pass)

8. **Empty `parameters[engine]` blocks** — 532 presets across batch have empty engine parameter objects. These need actual DSP parameter values or should be converted to macro-only presets with explicit documentation.

9. **Missing `description` and `couplingIntensity`** — Over 1000 preset-field gaps across the batch for these two fields. A script can add `"description": ""` and `"couplingIntensity": "None"` as defaults for presets missing them (then editorial pass to fill descriptions).

10. **Naming violations** — 586 presets with non-compliant names. Two sub-campaigns needed:
    - **Descriptor keys to evocative names**: ~400 presets with 4-7 word descriptor keys need proper names
    - **Single-word expansion**: ~100 presets with single-word names need a second word added

### Acceptable as-is (P3)

11. **Macro label ordering variation** (Obbligato, Ole) — The same 4 engine-specific labels appear in different M1-M4 positions across presets. Standardize to one canonical ordering per engine.

12. **Submerged mood gaps** — Osteria, Ohm, Obbligato, Ole, Ombre all have zero Submerged presets. Not blocking but creates an incomplete mood palette.

13. **Entangled over-representation** — All 8 engines have 38-50% of presets in Entangled. This reflects the coupling architecture emphasis but should be more evenly distributed across moods for V1.

---

*Report generated: 2026-03-21*
*Tools used: grep, Python json scan across ~3,400 preset files*
*Next recommended action: Run P1 batch fixes 1-7 with targeted scripts, then editorial sweep for issues 8-10*
