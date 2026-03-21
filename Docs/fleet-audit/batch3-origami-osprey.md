# Fleet Audit — Batch 3: Origami, Oracle, Obscura, Oceanic, Ocelot, Optic, Oblique, Osprey

**Date:** 2026-03-21
**Auditor:** Claude Code (Sonnet 4.6)
**Scope:** Preset quality audit across 8 engines — field completeness, DNA accuracy, param prefix integrity, coupling validity, naming compliance, mood coverage

---

## Fleet-Wide Findings (Applies to ALL 8 Engines)

Before per-engine detail, three issues are systemic across the entire batch:

### SYS-1: Missing `tempo`, `couplingIntensity`, `description` fields (P1 — Doctrine violation)
- `tempo` missing from 45–68% of presets per engine
- `couplingIntensity` missing from 41–60% of presets per engine
- `description` missing from 30–51% of presets per engine
- Root cause: large batches of Entangled presets were generated without these fields

### SYS-2: Naming violations (P1)
- 13–21% of presets per engine violate the 2–3 word / ≤30 char rule
- Common patterns: `HOT_STILL_SPARSE_VAST_FLX_1` (single underscore-joined string), `5X DARK COLD KINETIC DENSE ...` (4–6+ words), `DARK VAST KINETIC FLU 13` (descriptor-dump slugs)
- These read as internal debug labels, not commercial preset names

### SYS-3: DNA cross-validation failures (P2)
- Atmosphere aggression > 0.4: 7–33 violations per engine
- Atmosphere space < 0.5: 5–20 violations per engine
- Foundation density < 0.4: 5–18 violations per engine
- Flux movement < 0.5: 5–27 violations per engine
- No engine passes all cross-validation checks

---

## ENGINE: ORIGAMI

**Preset Count:** 152 single-engine + 506 multi-engine = 658 total
**Mood Distribution:** Foundation=64, Atmosphere=60, Entangled=250, Prism=89, Flux=75, Aether=71, Family=49, Submerged=0
**Missing Moods:** Submerged (0 presets)
**Non-Standard Folders:** None

**DNA Range (30-preset sample):**
- brightness: [0.035 – 0.957] range=0.922
- warmth: [0.040 – 0.976] range=0.936
- movement: [0.014 – 0.972] range=0.958
- density: [0.031 – 0.983] range=0.952
- space: [0.047 – 0.968] range=0.921
- aggression: [0.054 – 0.988] range=0.934

**DNA Gaps:** None. All dimensions exceed 0.4 range threshold.

---

### SAMPLE AUDIT (3 presets)

**1. Sargasso Stillness** — mood: Atmosphere
- Missing fields: `tempo` present (null ✓), `couplingIntensity` present (None ✓), `description` present ✓
- All 13 required fields: PASS
- Naming: PASS — "Sargasso Stillness" (2 words, 19 chars, evocative)
- Param prefix: PASS — all keys use `origami_` correctly
- DNA accuracy: CONDITIONAL PASS — movement=0.88 and space=0.95 seem high for a preset described as "Still at the centre of the gyre." Movement should be lower for stillness character. Atmosphere aggression=0.14 PASS, space=0.95 PASS.
- MacroLabels: PASS — 4 labels, all UPPERCASE
- Issues: Minor DNA-description mismatch (movement 0.88 feels high for "stillness")

**2. Paper Fold Crush** — mood: Flux
- Missing fields: `description` present ✓, `couplingIntensity` present (None ✓), `tempo` MISSING ✗
- Required fields: FAIL — `tempo` absent
- Naming: PASS — "Paper Fold Crush" (3 words, 16 chars)
- Param prefix: FAIL — `parameters.Origami` block is empty `{}` — no actual parameter values stored
- DNA accuracy: Flux movement=0.68 PASS (>0.5). Aggression=0.82 is high but Flux mood allows it.
- MacroLabels: PASS — 4 UPPERCASE labels
- Issues: Empty parameters block (P0 — preset will load with engine defaults, not authored values). `tempo` field missing.

**3. Architects Warm** — mood: Family (multi-engine: Overworld, Orbital, Origami)
- Missing fields: `couplingIntensity` is null (not "None" string), `tempo` present (null ✓)
- Required fields: FAIL — `coupling` is null instead of `{"pairs": []}` or a valid pairs array
- Naming: FAIL — "Architects Warm" is acceptable (2 words, 15 chars) but PASS on rule check
- Param prefix: Origami and Orbital parameter blocks are empty `{}` (only Overworld has params). P0 issue for multi-engine presets.
- DNA accuracy: PASS — reasonable balanced values
- MacroLabels: PASS — 4 UPPERCASE
- Issues: `coupling: null` (should be `{"pairs": []}` at minimum), two of three engine param blocks empty, `couplingIntensity: null` (should be string "None")

**Fleet scan results:**
- Missing required fields (any): 438/658 presets (66%) — dominated by `tempo`, `couplingIntensity`, `description`
- Wrong param prefix (single-engine): 9 instances — keys like `orig_macro_character`, bare `coupling`, `fold`, `space`, `tension` (without prefix)
- Empty params block (single-engine): 27 presets — these load as init patches
- Coupling null: 27 presets
- Naming violations: 116/658 (17%)
- DNA cross-validation failures: 86 violations across 658 presets (Atmosphere aggression>0.4: 28, Atmosphere space<0.5: 16, Foundation density<0.4: 18, Flux movement<0.5: 24)

---

**P0 Issues (blocks shipping):**
- 27 single-engine presets have empty `parameters` block — will load as engine defaults, not authored sounds
- 9 presets use wrong param keys (bare `fold`, `space`, `tension`, `orig_macro_*`) — parameter data silently ignored by engine

**P1 Issues (doctrine violations):**
- 438 presets missing `tempo` and/or `couplingIntensity` — schema contract violation
- 337 presets missing `description` — unacceptable for commercial release
- 116 presets with naming violations (descriptor dumps, single-word underscore strings)
- 6 Entangled presets have empty `coupling.pairs` — Entangled presets MUST have at least one active pair
- Submerged mood: 0 presets — complete gap in mood coverage

**P2 Issues (quality concerns):**
- 86 DNA cross-validation failures — Atmosphere presets with aggression >0.4, Flux presets with movement <0.5
- 27 presets have `coupling: null` instead of `{"pairs": []}` — may cause preset manager errors

**P3 Issues (polish items):**
- `couplingIntensity: null` vs `couplingIntensity: "None"` — inconsistent null vs string representation

---

## ENGINE: ORACLE

**Preset Count:** 194 single-engine + 490 multi-engine = 684 total
**Mood Distribution:** Foundation=45, Atmosphere=81, Entangled=295, Prism=73, Flux=77, Aether=83, Family=27, Submerged=0
**Missing Moods:** Submerged (0 presets)
**Non-Standard Folders:** Ethereal=3 (Oracle presets stored in non-standard folder; mood field says "Aether" not "Ethereal")

**DNA Range (30-preset sample):**
- brightness: [0.023 – 0.976] range=0.953
- warmth: [0.050 – 0.940] range=0.890
- movement: [0.019 – 0.990] range=0.971
- density: [0.029 – 0.955] range=0.926
- space: [0.051 – 0.961] range=0.910
- aggression: [0.057 – 0.983] range=0.926

**DNA Gaps:** None. Excellent coverage across all dimensions.

---

### SAMPLE AUDIT (3 presets)

**1. Chromatic Drift II** — mood: Atmosphere (file: Oracle_Chromatic_Drift.xometa)
- All required fields: PASS — all 13 fields present
- Naming: PASS — "Chromatic Drift II" (3 words, 18 chars, evocative) — NOTE: "II" as suffix may indicate duplicate naming pattern
- Param prefix: PASS — all keys use `oracle_` correctly, full parameter set authored
- DNA accuracy: Atmosphere aggression=0.18 PASS (<0.4), space=0.78 PASS (>0.5). Excellent.
- MacroLabels: PASS — 4 UPPERCASE: PROPHECY, EVOLUTION, GRAVITY, DRIFT (engine-specific, not generic — good)
- Issues: None significant

**2. CRYSTALLINE FLOOD** — mood: Flux (multi-engine: Ouroboros + Oracle)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗, `version` is "1.0" (not "1.0.0") — minor
- Required fields: FAIL — missing 4 fields
- Naming: FAIL — "CRYSTALLINE FLOOD" is ALL-CAPS (2 words, 17 chars) — naming convention requires mixed-case evocative names, not shout-case
- Param prefix: Oracle parameters block uses `macro_character`, `macro_coupling` etc. — these are generic macro keys, not `oracle_` prefixed. Technically acceptable for coupling-style presets but inconsistent.
- DNA: Flux movement=0.524 PASS (>0.5). Aggression=0.6 OK for Flux.
- Coupling: PASS — SPECTRAL_MORPH pair exists, amount=0.706 (>0.1) ✓
- Issues: ALL-CAPS name, 4 missing fields, generic macro_ param keys

**3. Copper Blade** — mood: Aether (multi-engine: Oracle + Organon)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL
- Naming: PASS — "Copper Blade" (2 words, 12 chars)
- Param prefix: Oracle parameters use `macro_character` etc. — same generic key pattern as above
- DNA: Aether mood — no specific cross-validation rule applies. Values reasonable.
- Coupling: PASS — PITCH_SYNC pair, amount=0.7099
- Issues: 4 missing fields

**Fleet scan results:**
- Missing required fields: 438/684 (64%)
- Wrong param prefix (single-engine): 11 presets using `orc_` prefix (e.g., `orc_ampAttack`, `orc_filterCutoff`) — wrong prefix, will silently fail
- Empty params block (single-engine): 28 presets
- Coupling null: 28 presets
- Naming violations: 122/684 (17%)
- DNA cross-validation failures: 82 violations (Atmosphere aggression>0.4: 29, Atmosphere space<0.5: 16, Foundation density<0.4: 10, Flux movement<0.5: 27)

---

**P0 Issues (blocks shipping):**
- 11 presets use `orc_` prefix instead of `oracle_` — ALL parameter data silently ignored at load time (files: Oracle_Void.xometa, Signal_Riot.xometa, Dense_Signal.xometa + 8 others)
- 28 single-engine presets have empty `parameters` block

**P1 Issues (doctrine violations):**
- 438 presets missing `tempo`, `couplingIntensity`, and/or `description`
- 3 Oracle presets stored in non-standard `Ethereal/` folder but have `mood: "Aether"` — folder/mood mismatch
- 7 Entangled presets have empty `coupling.pairs`
- Submerged mood: 0 presets
- 122 naming violations (ALL-CAPS names, descriptor dumps)

**P2 Issues (quality concerns):**
- 82 DNA cross-validation failures
- 1 Oracle Entangled preset has coupling amount ≤ 0.1 (nearly inactive coupling)
- Subfolder `Entangled/Oracle-Organon/` contains presets outside standard folder structure

**P3 Issues (polish items):**
- `version: "1.0"` vs `"1.0.0"` inconsistency (minor, both accepted)
- "Chromatic Drift II" naming pattern — "II" suffix implies first version exists; verify no naming collision

---

## ENGINE: OBSCURA

**Preset Count:** 148 single-engine + 348 multi-engine = 496 total
**Mood Distribution:** Foundation=14, Atmosphere=53, Entangled=310, Prism=13, Flux=23, Aether=66, Family=17, Submerged=0
**Missing Moods:** Submerged (0 presets)
**Non-Standard Folders:** Entangled sub-folders: `Entangled/Obscura-Organon` (2), `Entangled/Overworld-Obscura` (2)

**DNA Range (30-preset sample):**
- brightness: [0.031 – 0.971] range=0.940
- warmth: [0.027 – 0.941] range=0.914
- movement: [0.091 – 0.999] range=0.908
- density: [0.045 – 0.956] range=0.911
- space: [0.073 – 0.938] range=0.865
- aggression: [0.005 – 0.952] range=0.947

**DNA Gaps:** None. All dimensions clear 0.4 threshold.

---

### SAMPLE AUDIT (3 presets)

**1. Shadow Current** — mood: Aether (multi-engine: Oblique + Obscura)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL — 4 missing
- Naming: PASS — "Shadow Current" (2 words, 14 chars, evocative)
- Param prefix: Multi-engine preset uses `macro_character` etc. in both engine blocks — acceptable for coupling presets
- DNA: Aether mood, no specific rule. Values look reasonable. space=0.0552 is very low for Aether — may need review.
- Coupling: PASS — PITCH_SYNC pair, amount=0.8606 (>0.1) ✓
- MacroLabels: PASS — 4 UPPERCASE
- Issues: 4 missing fields; space=0.0552 unusually low for an ethereal sound

**2. ICE FLUX 6** — mood: Flux (multi-engine: Optic + Obscura)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL
- Naming: FAIL — "ICE FLUX 6" (ALL-CAPS + numeric suffix, 10 chars) — internal debug pattern, not commercial
- Param prefix: Obscura params use `macro_character` etc. — acceptable
- DNA: Flux movement=0.8287 PASS (>0.5). aggression=0.7338, brightness=0.6356 — reasonable for Flux mood.
- Coupling: PASS — PITCH_SYNC, amount=0.352 ✓
- Issues: ALL-CAPS numbered name, 4 missing fields

**3. DARK COLD VAST VIOLENT FAM 4** — mood: Family (multi-engine: Orbital + Obscura)
- Missing fields: None missing — all required fields present ✓
- Required fields: PASS
- Naming: FAIL — "DARK COLD VAST VIOLENT FAM 4" (6 words, 28 chars) — violates 2–3 word rule; also ALL-CAPS debug-style
- Param prefix: FAIL — `parameters.Obscura` uses `level` and `pan` keys without `obscura_` prefix
- DNA: Family mood — aggression=0.938 is high; space=0.914 OK; warmth=0.042 is very cold for Family mood (families typically warmer)
- Coupling: Stored as JSON array `[{...}]` not `{"pairs": [...]}` — schema inconsistency
- MacroLabels: PASS — 4 UPPERCASE
- Issues: Naming violation, wrong param prefix (`level`/`pan` not `obscura_level`/`obscura_pan`), coupling in array format vs object format

**Fleet scan results:**
- Missing required fields: 338/496 (68%)
- Wrong param prefix (single-engine): 7 instances
- Empty params block (single-engine): 19 presets
- Coupling null: 14 presets
- Naming violations: 69/496 (13%)
- DNA cross-validation failures: 50 violations

---

**P0 Issues (blocks shipping):**
- 7 single-engine presets with wrong param prefix — data silently ignored
- 19 single-engine presets with empty params block
- Schema inconsistency: some presets store coupling as JSON array `[...]` instead of `{"pairs": [...]}` — PresetManager may fail to parse

**P1 Issues (doctrine violations):**
- 338 presets missing `tempo`, `couplingIntensity`, and/or `description`
- 4 Entangled presets have empty `coupling.pairs`
- Submerged: 0 presets
- Foundation: only 14 presets (3% of library) — severe under-coverage of grounding, playable category

**P2 Issues (quality concerns):**
- 50 DNA cross-validation failures
- Prism: only 13 presets — very thin harmonic/tonal category coverage
- Sub-folders `Entangled/Obscura-Organon` and `Entangled/Overworld-Obscura` outside standard structure

**P3 Issues (polish items):**
- Some Family presets with warmth < 0.2 may feel tonally inconsistent with Family mood identity

---

## ENGINE: OCEANIC

**Preset Count:** 133 single-engine + 354 multi-engine = 487 total
**Mood Distribution:** Foundation=24, Atmosphere=74, Entangled=191, Prism=18, Flux=75, Aether=69, Family=26, Submerged=10
**Missing Moods:** None — all 8 standard moods represented
**Non-Standard Folders:** None

**DNA Range (30-preset sample):**
- brightness: [0.023 – 0.976] range=0.953
- warmth: [0.046 – 0.963] range=0.916
- movement: [0.019 – 0.990] range=0.971
- density: [0.010 – 0.971] range=0.961
- space: [0.032 – 0.988] range=0.956
- aggression: [0.033 – 0.988] range=0.955

**DNA Gaps:** None. Outstanding coverage across all 6 dimensions.

---

### SAMPLE AUDIT (3 presets)

**1. Reef Tone** — mood: Foundation
- Missing fields: `schema_version` present ✓, all required fields present ✓ — PASS
- Required fields: PASS — complete
- Naming: PASS — "Reef Tone" (2 words, 9 chars, evocative)
- Param prefix: PASS — all keys use `ocean_` correctly, comprehensive parameter set authored
- DNA: Foundation density=0.68 PASS (>0.4). movement=0.25 appropriate for stable foundation. Excellent match to description.
- MacroLabels: PASS — 4 UPPERCASE: CHARACTER, MOVEMENT, COUPLING, SPACE
- Version: `"1.0_0"` — non-standard format (expected "1.0" or "1.0.0") — minor
- Issues: Version format `"1.0_0"` — cosmetic

**2. Ice Horizon 2** — mood: Atmosphere (multi-engine: Osprey + Oceanic)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL
- Naming: FAIL — "Ice Horizon 2" (3 words including number — numbered suffix indicates a series, not standalone evocative name)
- Param prefix: Multi-engine, uses `macro_character` etc. — acceptable
- DNA: Atmosphere aggression=0.1174 PASS (<0.4), space=0.9733 PASS (>0.5). Excellent alignment.
- Coupling: PASS — FILTER_MOD, amount=0.842 ✓
- Issues: 4 missing fields, numbered name suffix

**3. Rust Ripple** — mood: Flux (multi-engine: Oceanic + Opal)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL
- Naming: PASS — "Rust Ripple" (2 words, 11 chars) — evocative, visually strong
- Param prefix: Multi-engine, macro_ keys — acceptable
- DNA: Flux movement=0.9897 PASS. aggression=0.9884 — extreme; warmth=0.9556 — extreme; density=0.1092 — very sparse. Potential DNA inaccuracy: Rust Ripple as a sparse warm violent sound is unusual.
- Coupling: PASS — VELOCITY_COUPLE, amount=0.436 ✓
- Issues: 4 missing fields; DNA may be exaggerated (extremes in 5 of 6 dimensions rare in physical sound)

**Fleet scan results:**
- Missing required fields: 296/487 (60%)
- Wrong param prefix (single-engine): 7 instances
- Empty params block (single-engine): 18 presets
- Coupling null: 28 presets
- Naming violations: 98/487 (20%)
- DNA cross-validation failures: 85 violations (worst: Atmosphere aggression>0.4: 33, Flux movement<0.5: 27)

---

**P0 Issues (blocks shipping):**
- 7 single-engine presets with wrong param prefix
- 18 single-engine presets with empty params block

**P1 Issues (doctrine violations):**
- 296 presets missing `tempo`, `couplingIntensity`, and/or `description`
- 8 Entangled presets have empty `coupling.pairs` — highest Entangled issue count for Oceanic's size
- 98 naming violations (20%) — highest rate in this batch

**P2 Issues (quality concerns):**
- 85 DNA cross-validation failures — second worst in this batch; Atmosphere aggression violations (33) suggest aggressive presets misfiled or mis-tagged
- Prism: only 18 presets — thin harmonic coverage

**P3 Issues (polish items):**
- `version: "1.0_0"` format on some presets — cosmetic inconsistency
- Numbered preset suffixes ("Ice Horizon 2") suggest series that may need consolidation or renaming

---

## ENGINE: OCELOT

**Preset Count:** 184 single-engine + 319 multi-engine = 503 total
**Mood Distribution:** Foundation=42, Atmosphere=35, Entangled=201, Prism=85, Flux=35, Aether=37, Family=21, Submerged=10
**Missing Moods:** None — all 8 standard moods represented
**Non-Standard Folders:** Crystalline=7, Deep=7, Kinetic=8, Luminous=7, Organic=8 (37 presets in 5 non-standard moods — Ocelot has its own mood taxonomy)

**DNA Range (30-preset sample):**
- brightness: [0.031 – 0.896] range=0.865
- warmth: [0.047 – 0.951] range=0.904
- movement: [0.091 – 0.970] range=0.879
- density: [0.021 – 0.920] range=0.899
- space: [0.114 – 0.942] range=0.828
- aggression: [0.047 – 0.927] range=0.880

**DNA Gaps:** None — all dimensions clear 0.4 threshold. However, brightness ceiling of 0.896 means the very-bright extreme is underrepresented in sample.

---

### SAMPLE AUDIT (3 presets)

**1. Savanna Strike** — mood: Foundation
- Missing fields: `engines` field present ✓, `tags` is empty list `[]` ✗ (field exists but no values), `version` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `coupling` MISSING ✗, `macroLabels` present ✓
- Required fields: FAIL — missing `version`, `couplingIntensity`, `tempo`, `coupling`
- Schema: Uses `"engine": "OCELOT"` (singular, wrong case) AND `"engines": ["Ocelot"]` — dual redundant field, inconsistent with fleet schema
- Uses `"format": "xometa/1"` instead of `"schema_version": 1` — schema format mismatch
- Uses `"category": "Foundation"` instead of `"mood": "Foundation"` — wrong field name for mood
- Naming: PASS — "Savanna Strike" (2 words, 14 chars)
- Param prefix: PASS — all keys use `ocelot_` correctly, extensive parameter set
- MacroLabels: FAIL — macroLabels says CHARACTER/MOVEMENT/COUPLING/SPACE but macros block says M1/M2/M3/M4 with PROWL/FOLIAGE/ECOSYSTEM/CANOPY — inconsistency between macroLabels and macros
- DNA: Foundation density=0.5 PASS (>0.4). Reasonable values.
- Issues: Multiple schema deviations (singular `engine`, wrong field names, macroLabels/macros mismatch)

**2. Modal Resonance Deep** — mood: Prism
- Missing fields: `description` present ✓, `couplingIntensity` present ✓, `tempo` present ✓, `coupling` present ✓, `author` present ✓ — all required fields present ✓
- Required fields: PASS
- Schema: Has duplicate `dna` and `sonic_dna` fields with identical values — redundant
- Naming: PASS — "Modal Resonance Deep" (3 words, 20 chars, evocative)
- Param prefix: PASS — all `ocelot_` keys correct, comprehensive
- MacroLabels: FAIL — macroLabels=["PROWL","FOLIAGE","ECOSYSTEM","CANOPY"] (correct engine-specific names), but macros block uses `{"M1":"PROWL","M2":"FOLIAGE",...}` — `M1`/`M2` generic keys instead of the label values as keys
- DNA: Prism mood — density=0.72, space=0.65 — feels appropriate
- Issues: macros field uses M1/M2/M3/M4 keys; duplicate `dna`/`sonic_dna` fields

**3. Winter Bell Pure** — mood: Crystalline (non-standard mood folder)
- Missing fields: Same structure as above — required fields present ✓
- Required fields: PASS
- Naming: PASS — "Winter Bell Pure" (3 words, 16 chars)
- Param prefix: PASS — all `ocelot_` keys
- MacroLabels: FAIL — same M1/M2/M3/M4 pattern in macros block
- DNA: brightness=0.82, space=0.74, warmth=0.12 — appropriate for crystalline character
- Non-standard folder: `Crystalline/` is not one of the 8 standard moods — preset manager may not enumerate this category
- Issues: Non-standard mood folder, M1/M2 macro key pattern, duplicate dna/sonic_dna

**Fleet scan results:**
- Missing required fields: 254/503 (50%) — better than average, but 32 presets fully missing `version`
- Wrong param prefix (single-engine): 3 instances (best in batch)
- Empty params block (single-engine): 41 presets
- Coupling null: 44 presets (tied for highest with Osprey at 44/456)
- Naming violations: 66/503 (13%) — best in batch
- DNA cross-validation failures: 34 violations (best in batch by large margin)
- Non-standard version: 32 presets missing `version` entirely

---

**P0 Issues (blocks shipping):**
- 37 presets stored in non-standard mood folders (Crystalline, Deep, Kinetic, Luminous, Organic) — these will NOT appear in mood browser if PresetManager only enumerates 8 standard folders
- 41 single-engine presets with empty params block
- macros block using `M1`/`M2`/`M3`/`M4` keys instead of label values — macro system may fail to map correctly

**P1 Issues (doctrine violations):**
- 254 presets missing required fields
- 10 Entangled presets with empty/null coupling — highest raw number in batch
- Schema deviations: some presets use `engine` (singular), `category` instead of `mood`, `format` instead of `schema_version` — legacy format bleed
- 32 presets missing `version` field
- Ocelot-specific macros (PROWL/FOLIAGE/ECOSYSTEM/CANOPY) are correct per-engine labels but the macros block uses M1/M2/M3/M4 generic keys — breaks macro assignment

**P2 Issues (quality concerns):**
- 3 Submerged presets with brightness > 0.4 (should be dark/deep)
- Duplicate `dna`/`sonic_dna` fields in many presets — one will become stale
- Prism has 85 presets (second-highest), Atmosphere only 35 — imbalanced

**P3 Issues (polish items):**
- Non-standard folder names (Crystalline, Deep, etc.) may be intentional Ocelot identity — discuss with Architect whether to keep as additional moods or migrate to standard

---

## ENGINE: OPTIC

**Preset Count:** 150 single-engine + 450 multi-engine = 600 total
**Mood Distribution:** Foundation=54, Atmosphere=68, Entangled=206, Prism=104, Flux=99, Aether=32, Family=37, Submerged=0
**Missing Moods:** Submerged (0 presets)
**Non-Standard Folders:** None

**DNA Range (30-preset sample):**
- brightness: [0.052 – 0.990] range=0.938
- warmth: [0.065 – 0.958] range=0.893
- movement: [0.016 – 0.982] range=0.966
- density: [0.021 – 0.983] range=0.963
- space: [0.028 – 0.988] range=0.960
- aggression: [0.083 – 0.986] range=0.903

**DNA Gaps:** None. Strong coverage.

---

### SAMPLE AUDIT (3 presets)

**1. ALLHI ATM 06** — mood: Atmosphere (multi-engine: Orbital + Optic)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL
- Naming: FAIL — "ALLHI ATM 06" (3 words, 12 chars) — pure debug slug, not a commercial preset name
- Param prefix: Multi-engine macro_ keys — acceptable
- DNA: Atmosphere aggression=0.396 — borderline (rule: <0.4) — technically PASS but barely. space=0.962 PASS (>0.5).
- Coupling: PASS — TIMBRE_BLEND, amount=0.763 ✓
- MacroLabels: PASS — 4 UPPERCASE
- Issues: Debug-style name, 4 missing fields

**2. VAST STILL GROUND** — mood: Foundation (multi-engine: Ombre + Optic)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL
- Naming: FAIL — "VAST STILL GROUND" (3 words, 17 chars) — ALL-CAPS descriptor; acceptable word count but shout case
- Param prefix: Ombre params: `macro_space` value is 1.009 — slightly OUT OF RANGE (>1.0) ✗
- DNA: Foundation density=0.848 PASS (>0.4). movement=0.518 acceptable for Foundation. aggression=0.456 for Foundation is borderline high.
- Coupling: PASS — ENVELOPE_LINK, amount=0.684 ✓
- Issues: 4 missing fields, ALL-CAPS name, macro param value 1.009 exceeds [0,1] range

**3. Arctic Shard Near** — mood: Prism
- Missing fields: `description` present ✓, `couplingIntensity` present ✓, `tempo` MISSING ✗, `author` MISSING ✗, `version` present ✓
- Required fields: FAIL — missing `tempo`, `author`
- Naming: PASS — "Arctic Shard Near" (3 words, 17 chars) — evocative, though "Near" as third word feels like a positional specifier rather than flavor
- Param prefix: FAIL — `parameters.Optic` block is empty `{}`
- DNA: aggression=0.008, density=0.029 — extremely low. Prism mood doesn't have specific DNA rules but these extreme values for a prism/harmonic sound seem low for density.
- Coupling: PASS — empty pairs with couplingIntensity "None"
- MacroLabels: FAIL — uses SCATTER/DRIFT/COUPLING/SPACE instead of standard CHARACTER/MOVEMENT/COUPLING/SPACE — engine-specific labels, which is acceptable per Ocelot precedent, but inconsistent with most Optic presets
- Issues: Empty params block, 2 missing fields, unusual macro labels

**Fleet scan results:**
- Missing required fields: 306/600 (51%)
- Wrong param prefix (single-engine): 13 instances — 4 using `opt_` prefix (opt_ampAttack etc.) + 9 using `couplingBus`/`couplingLevel` without prefix
- Empty params block (single-engine): 26 presets
- Coupling null: 37 presets
- Entangled coupling issues: 17 presets (highest in batch) — presets titled "Entangle" with empty coupling pairs
- Naming violations: 83/600 (13%)
- DNA cross-validation failures: 85 violations (tied with Oceanic for second worst)

---

**P0 Issues (blocks shipping):**
- 4 presets using `opt_` prefix instead of `optic_` (files: Light_Prism.xometa, Spectral_Bloom.xometa, Optic_Aurora.xometa + 1 more) — parameter data silently lost
- 26 single-engine presets with empty params block
- 17 Entangled presets with empty coupling pairs — highest in batch; particularly egregious for presets literally named "Entangle"
- param value 1.009 in VAST STILL GROUND exceeds [0,1] range

**P1 Issues (doctrine violations):**
- 306 presets missing `tempo`, `couplingIntensity`, and/or `description`
- Submerged: 0 presets
- 83 naming violations including pure debug slugs (ALLHI ATM 06)
- Aether severely under-populated (32 presets vs 104 in Prism) — imbalanced

**P2 Issues (quality concerns):**
- 85 DNA cross-validation failures
- 9 presets have `couplingBus` / `couplingLevel` keys without `optic_` prefix — these may work if the engine strips prefix for internal params, but is inconsistent

**P3 Issues (polish items):**
- Inconsistent macroLabel sets (some presets use SCATTER/DRIFT, others CHARACTER/MOVEMENT) — pick one standard for Optic

---

## ENGINE: OBLIQUE

**Preset Count:** 128 single-engine + 504 multi-engine = 632 total
**Mood Distribution:** Foundation=49, Atmosphere=72, Entangled=234, Prism=98, Flux=73, Aether=75, Family=19, Submerged=12
**Missing Moods:** None — all 8 standard moods represented
**Non-Standard Folders:** None

**DNA Range (30-preset sample):**
- brightness: [0.026 – 0.945] range=0.919
- warmth: [0.028 – 0.939] range=0.911
- movement: [0.046 – 0.978] range=0.932
- density: [0.054 – 0.983] range=0.930
- space: [0.025 – 0.978] range=0.953
- aggression: [0.035 – 0.986] range=0.951

**DNA Gaps:** None. Full coverage on all dimensions.

---

### SAMPLE AUDIT (3 presets)

**1. Peach Edge** — mood: Atmosphere (multi-engine: Oblique + Origami)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL
- Naming: PASS — "Peach Edge" (2 words, 10 chars, evocative and visual)
- Param prefix: Multi-engine macro_ keys — acceptable
- DNA: Atmosphere aggression=0.9863 — FAIL (>0.4 threshold) ✗. space=0.9263 PASS (>0.5). This Atmosphere preset is extremely aggressive — likely miscategorized or DNA mislabeled.
- Coupling: PASS — VELOCITY_COUPLE, amount=0.619 ✓
- MacroLabels: PASS — 4 UPPERCASE
- Issues: 4 missing fields; aggression=0.9863 in Atmosphere mood is a major DNA cross-validation violation — should be Flux or Prism

**2. Color Chord** — mood: Foundation
- Missing fields: All present ✓ — `coupling` present, `couplingIntensity` present (None ✓), `tempo` present (null ✓), `description` present ✓
- Required fields: PASS — complete record
- Naming: PASS — "Color Chord" (2 words, 11 chars, evocative)
- Param prefix: PASS — all `oblq_` keys correct, comprehensive parameter set authored (35+ params)
- DNA: Foundation density=0.6 PASS (>0.4). movement=0.3 appropriate for grounded sound.
- MacroLabels: PASS — 4 UPPERCASE: BOUNCE/COLOR/FOLD/SPACE (engine-specific labels — excellent)
- Issues: None

**3. ONYX PRISM** — mood: Prism (multi-engine: Oblique + Owlfish)
- Missing fields: `description` MISSING ✗, `couplingIntensity` MISSING ✗, `tempo` MISSING ✗, `author` MISSING ✗
- Required fields: FAIL
- Naming: FAIL — "ONYX PRISM" (ALL-CAPS, 2 words, 10 chars) — shout case
- Param prefix: Multi-engine macro_ — acceptable
- DNA: Prism mood — density=0.723, movement=0.897, space=0.922 reasonable. brightness=0.041 very low for "Prism" (prism implies spectral spread, which usually = brightness).
- Coupling: PASS — TIMBRE_BLEND, amount=0.532 ✓
- Issues: ALL-CAPS name, 4 missing fields, brightness 0.041 seems inconsistent with Prism identity

**Fleet scan results:**
- Missing required fields: 346/632 (54%)
- Wrong param prefix (single-engine): 7 instances
- Empty params block (single-engine): 16 presets (lowest in batch)
- Coupling null: 25 presets
- Naming violations: 97/632 (15%)
- DNA cross-validation failures: 70 violations (Atmosphere aggression>0.4: 28)

---

**P0 Issues (blocks shipping):**
- 7 single-engine presets with wrong param prefix
- 16 single-engine presets with empty params block (best in batch)

**P1 Issues (doctrine violations):**
- 346 presets missing `tempo`, `couplingIntensity`, and/or `description`
- 3 Entangled presets with empty coupling pairs
- 97 naming violations
- Family: only 19 presets — thin multi-engine coverage

**P2 Issues (quality concerns):**
- 70 DNA cross-validation failures; Atmosphere aggression pattern (28 violations) suggests many aggressively-toned Oblique presets filed under Atmosphere
- Submerged: only 12 presets — acceptable but thin
- Prism brightness=0.041 in "Onyx Prism" — identity mismatch

**P3 Issues (polish items):**
- ALL-CAPS names (ONYX PRISM, BRIGHT HOT DENSE FLU 02 etc.) — inconsistent with evocative naming standard

---

## ENGINE: OSPREY

**Preset Count:** 107 single-engine + 349 multi-engine = 456 total
**Mood Distribution:** Foundation=27, Atmosphere=58, Entangled=177, Prism=24, Flux=23, Aether=74, Family=65, Submerged=8
**Missing Moods:** None — all 8 standard moods represented
**Non-Standard Folders:** None

**DNA Range (30-preset sample):**
- brightness: [0.022 – 0.949] range=0.927
- warmth: [0.088 – 1.000] range=0.912
- movement: [0.047 – 0.926] range=0.879
- density: [0.096 – 0.991] range=0.895
- space: [0.047 – 0.987] range=0.939
- aggression: [0.046 – 0.955] range=0.909

**DNA Gaps:** None. Warmth ceiling reaches 1.000 — one preset at maximum warmth.

---

### SAMPLE AUDIT (3 presets)

**1. Coherent Shore** — mood: Atmosphere
- Missing fields: All present ✓ — complete record including `coupling`, `couplingIntensity`, `tempo`, `description`, `tags`
- Required fields: PASS
- Naming: PASS — "Coherent Shore" (2 words, 14 chars, evocative and on-brand for Osprey's coastal identity)
- Param prefix: PASS — all `osprey_` keys correct, full parameter set authored
- DNA: Atmosphere aggression=0.12 PASS (<0.4), space=0.72 PASS (>0.5). movement=0.35 — gentle, consistent with "coherent" description.
- MacroLabels: PASS — 4 UPPERCASE: CHARACTER/MOVEMENT/COUPLING/SPACE
- Issues: None — this is a model preset

**2. Azulejo Blue** — mood: Foundation
- Missing fields: All present ✓ — complete record
- Required fields: PASS
- Naming: PASS — "Azulejo Blue" (2 words, 12 chars) — culturally specific, evocative (Portuguese tile reference)
- Param prefix: PASS — all `osprey_` keys correct
- DNA: Foundation density=0.5 PASS (>0.4). movement=0.4 appropriate.
- MacroLabels: PASS — 4 UPPERCASE
- Issues: None — model-quality preset

**3. Hadal Silence** — mood: Aether
- Missing fields: All present ✓ — complete record (has `legacy` block, `created`, `sequencer` — extended schema)
- Required fields: PASS
- Naming: PASS — "Hadal Silence" (2 words, 13 chars) — excellent (Hadal zone is deepest ocean)
- Param prefix: CONDITIONAL PASS — uses `osprey_brightness`, `osprey_seaState`, `osprey_swellPeriod`, `osprey_wetMix` — these appear to be legacy/different parameter names than the Coherent Shore preset (which uses brine/foam/fog/hull). Two different parameter schemas present across Osprey single-engine presets.
- DNA: aggression=0.359 — somewhat high for a preset described as "Pure acoustic void." movement=0.0 — matches stillness description.
- Issues: Osprey uses two different parameter name sets across presets (`osprey_brine`/`osprey_foam` vs `osprey_brightness`/`osprey_seaState`) — likely legacy parameter names from earlier engine version. The PresetManager will silently ignore unrecognized keys.

**Fleet scan results:**
- Missing required fields: 258/456 (56%)
- Wrong param prefix (single-engine): 7 instances
- Empty params block (single-engine): 24 presets
- Coupling null: 42 presets (tied Ocelot for highest rate)
- Entangled coupling issues: 5 (including 2 presets with coupling amount = 0.067 — effectively inactive)
- Naming violations: 97/456 (21%) — second highest rate in batch
- DNA cross-validation failures: 54 violations (Atmosphere aggression>0.4: 26 — very high for library size)

---

**P0 Issues (blocks shipping):**
- Osprey has two incompatible parameter name schemas: `osprey_brine`/`osprey_foam`/`osprey_fog`/`osprey_hull` (current) vs `osprey_brightness`/`osprey_seaState`/`osprey_swellPeriod`/`osprey_wetMix` (legacy). Presets using legacy names load silently with defaults — affects multiple presets including Hadal_Silence.xometa
- 7 single-engine presets with wrong param prefix
- 24 single-engine presets with empty params block

**P1 Issues (doctrine violations):**
- 258 presets missing `tempo`, `couplingIntensity`, and/or `description`
- 2 Entangled presets have coupling amount = 0.067 (below 0.1 threshold — effectively uncoupled)
- 97 naming violations (21%) — highest rate in batch
- Flux: only 23 presets — thin kinetic coverage

**P2 Issues (quality concerns):**
- 54 DNA cross-validation failures; Atmosphere aggression violations (26) represent 45% of Atmosphere presets having aggression >0.4 — systematic miscalibration
- Prism: only 24 presets — thin
- Family overweighted at 65 presets (14% of library) vs Flux at 23 (5%)

**P3 Issues (polish items):**
- Warmth maxing at 1.000 is fine but worth double-checking the preset — extreme values sometimes indicate authored-by-algorithm rather than designed

---

## Summary Table

| Engine | Total | Missing Fields (%) | Empty Params | Wrong Prefix | Entangled Issues | Naming Violations (%) | DNA Failures | Missing Moods |
|--------|-------|--------------------|-------------|-------------|-----------------|----------------------|--------------|---------------|
| Origami | 658 | 66% | 27 | 9 | 6 | 17% | 86 | Submerged |
| Oracle | 684 | 64% | 28 | 11 | 7 | 17% | 82 | Submerged |
| Obscura | 496 | 68% | 19 | 7 | 4 | 13% | 50 | Submerged |
| Oceanic | 487 | 60% | 18 | 7 | 8 | 20% | 85 | None |
| Ocelot | 503 | 50% | 41 | 3 | 10 | 13% | 34 | None (+5 non-standard) |
| Optic | 600 | 51% | 26 | 13 | 17 | 13% | 85 | Submerged |
| Oblique | 632 | 54% | 16 | 7 | 3 | 15% | 70 | None |
| Osprey | 456 | 56% | 24 | 7 | 5 | 21% | 54 | None |

**Best performing:** Ocelot (lowest DNA failures, lowest prefix violations), Oblique (fewest empty params), Osprey Coherent Shore / Azulejo Blue (model-quality single-engine presets)

**Most critical:** Optic (17 Entangled coupling issues, P0 `opt_` prefix violations), Oracle (11 `orc_` prefix violations — P0), Osprey (dual parameter schema conflict — P0), Ocelot (37 presets in non-standard mood folders — invisible to browser)

---

## Fleet-Wide Remediation Priority

### P0 — Fix before any release
1. **Oracle**: Rename `orc_` prefixed params to `oracle_` in 11 presets
2. **Optic**: Rename `opt_` prefixed params to `optic_` in 4 presets; audit `couplingBus`/`couplingLevel` key prefix
3. **Osprey**: Audit all presets using legacy param names (`osprey_brightness`, `osprey_seaState`, `osprey_swellPeriod`, `osprey_wetMix`) — migrate to current schema or confirm these are valid alternative params
4. **All engines**: Populate empty `parameters` blocks (152 instances across batch) — these load as silent init patches
5. **Ocelot**: Audit 37 presets in non-standard mood folders — migrate to standard moods or confirm PresetManager enumerates custom folders
6. **Ocelot**: Fix macros block to use label values as keys (PROWL, FOLIAGE, ECOSYSTEM, CANOPY) not M1/M2/M3/M4
7. **Optic**: Fix 17 Entangled presets with empty coupling pairs — add at minimum one coupling pair

### P1 — Fix before V1 release
1. Add `tempo`, `couplingIntensity`, `description` to the ~400+ presets missing them per engine (likely requires a batch migration script)
2. Rename all naming-violation presets: eliminate ALL-CAPS, descriptor-dump, underscore-joined, and numbered-suffix names
3. Add Submerged presets for Origami, Oracle, Obscura, Optic (0 presets each)
4. Fix 43 Entangled presets across all 8 engines that have empty `coupling.pairs`
5. Correct coupling schema inconsistency: `coupling` must always be `{"pairs": [...]}` not array `[...]` or `null`

### P2 — Fix before 1.1
1. Address 546 DNA cross-validation failures across the batch — particularly Atmosphere presets with aggression > 0.4
2. Thin moods: Obscura Foundation (14), Obscura Prism (13), Oceanic Prism (18), Osprey Flux (23)
3. Remove duplicate `dna`/`sonic_dna` fields from Ocelot presets

### P3 — Polish pass
1. Standardize version field to `"1.0.0"` across all presets
2. `couplingIntensity: null` → `"None"` string
3. Remove Ocelot `format: "xometa/1"` and `engine: "OCELOT"` legacy fields
4. Resolve Osprey Family/Flux imbalance (65 vs 23)
