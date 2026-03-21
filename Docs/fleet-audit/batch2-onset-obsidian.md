# Batch 2 Preset Audit — ONSET · OVERWORLD · OPAL · ORBITAL · ORGANON · OUROBOROS · OBSIDIAN

**Auditor:** Rodrigo's audit assistant
**Date:** 2026-03-21
**Method:** Full automated sweep of all presets (518–751 per engine depending on engine) + 3-preset deep check per engine. DNA range computed from complete population. Field checks across all 8 standard moods.

---

## ENGINE: ONSET

**Preset Count:** 518
**Mood Distribution:** Foundation=96 (exclusive+shared), Atmosphere=40, Entangled=123, Flux=50, Aether=42 (includes shared presets counted per engine; cross-engine presets are double-counted in per-engine totals)
**Non-standard moods with count:** Prism=0, Family=0, Submerged=0
**Missing Moods:** Prism, Family, Submerged

**DNA Range (518 presets):**
- brightness: 0.005–0.999 (range=0.994)
- warmth: 0.020–0.990 (range=0.970)
- movement: 0.017–0.990 (range=0.973)
- density: 0.000–1.000 (range=1.000)
- space: 0.000–1.000 (range=1.000)
- aggression: 0.000–1.000 (range=1.000)

**DNA Gaps:** None — all 6 dimensions exceed 0.4 range. Full spectrum coverage confirmed.

**DNA Cross-Validation:**
- Atmosphere (need agg<0.4 AND space>0.5): 17/40 violations (42.5%)
- Foundation (need density>0.4): 18/96 violations (18.8%)
- Flux (need movement>0.5): 7/50 violations (14%)

---

### SAMPLE AUDIT (3 presets)

**1. "Flux Deep" — mood: Foundation**
File: `Foundation/COLD_DENSE_INTIMATE_VIOLENT_FND_AGG_2.xometa`
- Missing fields: `description`, `couplingIntensity`, `tempo`
- Naming: FAIL — "Flux Deep" is 2 words and ≤30 chars, but the name contradicts the mood field ("Flux" in a Foundation preset signals disorientation; minor semantic issue). Name is 8 chars — borderline evocativeness.
- Param prefix: FAIL — uses generic `macro_character`, `macro_coupling` etc. (no `perc_` prefix). These are macro-only params so technically `macro_` is ambiguous; however Onset's single-engine presets use `perc_macro_machine` style — this couled preset uses bare `macro_` which may cause PresetManager routing confusion.
- DNA accuracy: PASS — aggression=0.978, density=0.988 consistent with "Foundation/violent/dense" tags.
- Issues: Missing 3 required fields (description, couplingIntensity, tempo). Version "1.0" vs expected "1.0.0". No `perc_` prefix on any parameter.

**2. "Hat Cathedral" — mood: Atmosphere**
File: `Atmosphere/Onset_Hat_Cathedral.xometa`
- Missing fields: None — all required fields present.
- Naming: PASS — "Hat Cathedral" is 2 words, 13 chars, evocative.
- Param prefix: PASS — all params use `perc_` correctly (`perc_drive`, `perc_fx_reverb_decay`, `perc_v3_algoMode`, etc.).
- DNA accuracy: FAIL (cross-validation) — Atmosphere requires aggression<0.4 AND space>0.5. aggression=0.1 (PASS), space=0.95 (PASS). Actually PASSES the cross-validation check.
- Issues: `coupling.pairs` is empty `[]` with `couplingIntensity: "None"` — structurally correct. Version "1.0.0". This is a well-formed exemplar preset. No issues.

**3. "SAVAGE ETHER COLLAPSE" — mood: Aether**
File: `Aether/savage_ether_collapse.xometa`
- Missing fields: `description`, `couplingIntensity`, `tempo`
- Naming: FAIL — "SAVAGE ETHER COLLAPSE" is 3 words (passes word count) but 20 chars. PASSES length. However ALL_CAPS naming is inconsistent with the "evocative, title-case" convention visible in canonical presets like "Hat Cathedral" and "Digital Mist".
- Param prefix: FAIL — uses bare `macro_character`, `macro_coupling`, etc. under Onset key with no `perc_` prefix.
- DNA accuracy: PASS — aggression=0.889, density=0.715, consistent with "savage/collapse" character.
- Issues: Missing description, couplingIntensity, tempo. ALL_CAPS naming style inconsistency. Version "1.0" (not "1.0.0"). Bare macro_ params without engine prefix.

**P0 Issues (blocks shipping):**
- 9 single-engine Onset presets use wrong parameter prefixes: `ons_` (32 bad keys across files) and bare unprefixed keys (22 bad keys). These will silently fail on preset load — PresetManager cannot route params without engine prefix.

**P1 Issues (doctrine violations):**
- 126/518 presets (24%) missing `description` field.
- 144/518 presets (28%) missing `couplingIntensity` field.
- 20 Entangled presets have empty coupling pairs or amount≤0.1 — coupling presets without coupling are mislabeled by mood.
- 3 presets have mood field mismatch (stored in Entangled folder but mood="Flux" in JSON).

**P2 Issues (quality concerns):**
- 51 presets have names violating 2-3 word / ≤30 char rule (12 are 1 word, 39 are 4+ words with descriptor sequences like "BRIGHT SPARSE GENTLE AET 09").
- 17/40 Atmosphere presets fail DNA cross-validation (aggression≥0.4 or space≤0.5).
- 18/96 Foundation presets have density≤0.4.
- 75 presets have duplicate `sonic_dna` key alongside `dna` — redundant field, legacy cruft.

**P3 Issues (polish items):**
- 202 presets missing `tempo` field (minor — null is acceptable but field should be present for schema consistency).
- 158 presets use version `"1.0"` instead of `"1.0.0"`.
- Missing Submerged, Prism, Family mood representation — no presets in these 3 moods.

---

## ENGINE: OVERWORLD

**Preset Count:** 577 (cross-engine doubles included)
**Mood Distribution:** Foundation=54, Atmosphere=36 (exclusive+shared), Entangled=79, Flux=75, Aether=59
**Missing Moods:** Prism=0, Family=0, Submerged=0

**DNA Range (577 presets):**
- brightness: 0.001–1.000 (range=0.999)
- warmth: 0.026–0.988 (range=0.962)
- movement: 0.000–1.000 (range=1.000)
- density: 0.012–0.988 (range=0.976)
- space: 0.000–0.988 (range=0.988)
- aggression: 0.000–1.000 (range=1.000)

**DNA Gaps:** None — all dimensions exceed 0.4 range.

**DNA Cross-Validation:**
- Atmosphere (need agg<0.4 AND space>0.5): 21/45 violations (46.7%) — most severe Atmosphere violation rate in this batch.
- Foundation (need density>0.4): 15/65 violations (23%)
- Flux (need movement>0.5): 25/83 violations (30.1%) — severe. Many "STILL" variants filed under Flux with movement<0.15.

---

### SAMPLE AUDIT (3 presets)

**1. "Boss Fight" — mood: Foundation**
File: `Foundation/OVERWORLD_Boss_Fight.xometa`
- Missing fields: `couplingIntensity` present (="None"), `tempo` present (=null). Missing: nothing. All required fields present.
- Naming: PASS — "Boss Fight" is 2 words, 10 chars, evocative.
- Param prefix: PASS — uses `ow_era`, `ow_macro_era`, `ow_macro_chaos`, `ow_outputLevel`, `ow_couplingBus` — all correctly prefixed.
- DNA accuracy: PASS — brightness=0.75, density=0.7, aggression=0.8 consistent with "boss fight" character. Foundation cross-val: density=0.7>0.4 PASS.
- Issues: None. Clean exemplar preset.

**2. "Idle Screen" — mood: Atmosphere**
File: `Atmosphere/OVERWORLD_Idle_Screen.xometa`
- Missing fields: None. All required fields present.
- Naming: PASS — "Idle Screen" is 2 words, 11 chars, evocative.
- Param prefix: PASS — all `ow_` prefixed correctly.
- DNA accuracy: FAIL (cross-validation) — Atmosphere needs agg<0.4 AND space>0.5. aggression=0.1 (PASS), space=0.65 (PASS). Actually PASSES cross-validation.
- Issues: None. Well-formed.

**3. "SILKEN THREAD" — mood: Entangled**
File: `Entangled/SILKEN_THREAD.xometa`
- Missing fields: `description`, `couplingIntensity`, `tempo`.
- Naming: PASS — "SILKEN THREAD" is 2 words, 13 chars. All-caps inconsistency with canonical style but passes rules.
- Param prefix: FAIL — parameters use bare `macro_character`, `macro_coupling` etc. under Overworld key with no `ow_` prefix. Same couled-preset macro-only issue as Onset.
- DNA accuracy: PASS — movement=0.837, space=0.791 consistent with Entangled mood.
- Coupling check: PASS — pairs array has one entry (Ole→Overworld, HARMONIC_FOLD, amount=0.64>0.1). Coupling intent valid.
- Issues: Missing description, couplingIntensity, tempo. Version "1.0". Bare macro_ params.

**P0 Issues:**
- 7 single-engine Overworld presets use wrong/missing parameter prefixes (22 bare-unprefixed keys). Same routing failure risk as Onset.
- 4 presets have `coupling: null` (rather than `{"pairs": []}`). Null coupling object will likely throw NullPointerException in PresetManager.

**P1 Issues:**
- 210/577 presets (36%) missing `description`.
- 225/577 presets (39%) missing `couplingIntensity`.
- 7 presets stored in Entangled folder but internal mood field is "Flux" or "Foundation" — wrong folder placement.
- 4 presets have `coupling: null` (should be `{"pairs": []}`).
- 25/83 Flux presets have movement≤0.5 — severe DNA cross-validation failure; many "STILL" variants systematically mislabeled as Flux.

**P2 Issues:**
- 89 presets with bad names (38 are single underscore-joined strings like "BRIGHT_HOT_DENSE_VAST_AET3_4" — these are 1-word after split but actually 5-word descriptor sequences that violate naming convention).
- 21/45 Atmosphere presets fail DNA cross-validation (highest rate of this batch).
- 79 presets with duplicate `sonic_dna` key.

**P3 Issues:**
- 280 presets missing `tempo` field.
- 245 presets use version "1.0".
- Missing Prism, Family, Submerged moods entirely.

---

## ENGINE: OPAL

**Preset Count:** 667
**Mood Distribution:** Foundation=31 (low), Atmosphere=64, Entangled=116, Flux=77, Aether=63, Submerged=6
**Missing Moods:** Prism=0, Family=0

**DNA Range (667 presets):**
- brightness: 0.000–1.000 (range=1.000)
- warmth: 0.000–0.999 (range=0.999)
- movement: 0.000–1.000 (range=1.000)
- density: 0.000–0.990 (range=0.990)
- space: 0.000–1.000 (range=1.000)
- aggression: 0.000–0.988 (range=0.988)

**DNA Gaps:** None — all dimensions exceed 0.4 range.

**DNA Cross-Validation:**
- Atmosphere (need agg<0.4 AND space>0.5): 40/90 violations (44.4%) — very high.
- Foundation (need density>0.4): 17/48 violations (35.4%) — high. OPAL Foundation preset count (31 exclusive) is the lowest among 5-mood engines.
- Flux (need movement>0.5): 36/95 violations (37.9%) — high.

---

### SAMPLE AUDIT (3 presets)

**1. "Metallic Shimmer" — mood: Foundation**
File: `Foundation/Opal_Metallic_Shimmer.xometa`
- Missing fields: None. All required fields present.
- Naming: PASS — "Metallic Shimmer" is 2 words, 16 chars, evocative.
- Param prefix: PASS — all `opal_` prefixed correctly (opal_ampAttack, opal_grainSize, etc.).
- DNA accuracy: PASS — brightness=0.8, density=0.75, aggression=0.15. Foundation cross-val: density=0.75>0.4 PASS.
- Issues: None. Exemplar preset.

**2. "Shimmer Veil" — mood: Atmosphere**
File: `Atmosphere/Opal_Shimmer_Veil.xometa`
- Missing fields: None. All required fields present.
- Naming: PASS — "Shimmer Veil" is 2 words, 12 chars, evocative.
- Param prefix: PARTIAL FAIL — uses `opal_macroScatter`, `opal_macroDrift` as macro labels but macroLabels array still lists "CHARACTER", "MOVEMENT", "COUPLING", "SPACE". The macro parameter keys (opal_macroScatter, opal_macroDrift) don't match the macroLabels declared. This creates a label/param key mismatch.
- DNA accuracy: PASS — brightness=0.8, space=0.75, aggression=0.05. Atmosphere cross-val: agg<0.4 AND space>0.5 PASS.
- Issues: Macro label/param key mismatch (macroLabels say CHARACTER/MOVEMENT but params use macroScatter/macroDrift). This likely means the macro labels shown to the user won't match what the macros actually control.

**3. "OPAL Grain Morph" — mood: Entangled**
File: `Entangled/Octopus_OPAL_Grain_Morph.xometa`
- Missing fields: None. All required fields present.
- Naming: PASS — "OPAL Grain Morph" is 3 words, 15 chars. Acceptable, though Opal engine is self-referential in a preset name — minor style concern.
- Param prefix: PASS — Octopus params use `octo_` correctly. Note: Opal engine is listed in engines array but has no parameters block in this preset (only Octopus params). Opal is the coupling source but has no stored state.
- DNA accuracy: PASS — brightness=0.82, movement=0.82 consistent with granular morphing character.
- Coupling check: PASS — pairs has AudioToFM from Opal→Octopus, amount=0.55>0.1 PASS.
- Issues: Opal is listed in `engines` but has no `parameters.Opal` block. Depending on PresetManager behavior this may cause a null-deref or silent failure on loading Opal state.

**P0 Issues:**
- 5 single-engine Opal presets with wrong parameter prefixes (bare unprefixed keys: `coupling`, `drift`, `scatter`).
- 2 presets with `coupling: null`.
- Opal engine in engines array but missing parameters block in Octopus_OPAL_Grain_Morph — potential null-deref on load.
- Macro label/param key mismatch in Shimmer Veil (macroScatter/macroDrift vs CHARACTER/MOVEMENT labels).

**P1 Issues:**
- 262/667 presets (39%) missing `description`.
- 327/667 presets (49%) missing `couplingIntensity` — highest rate in this batch.
- 9 Entangled presets with empty coupling pairs or null coupling.
- 5 presets with folder/mood mismatch (in Entangled folder but mood="Flux" or "Foundation").
- 40/90 Atmosphere presets fail DNA cross-validation (44.4% — highest in batch).

**P2 Issues:**
- 98 presets with bad names (highest count in batch) — 30 are 1-word, 68 are 4+ words. Many are raw descriptor sequences.
- 36/95 Flux presets with movement≤0.5 (37.9% — high).
- Foundation preset count is lowest in batch (31 exclusive presets vs Atmosphere 64).
- 65 presets with duplicate `sonic_dna` key.

**P3 Issues:**
- 358 presets missing `tempo` field.
- 344 presets use version "1.0".
- Missing Prism and Family moods.

---

## ENGINE: ORBITAL

**Preset Count:** 552
**Mood Distribution:** Foundation=46, Atmosphere=73, Entangled=134, Flux=40, Aether=95, Submerged=10
**Missing Moods:** Prism=0, Family=0

**DNA Range (552 presets):**
- brightness: 0.000–0.989 (range=0.989)
- warmth: 0.000–0.986 (range=0.986)
- movement: 0.024–1.000 (range=0.976)
- density: 0.000–1.000 (range=1.000)
- space: 0.007–1.000 (range=0.993)
- aggression: 0.020–0.999 (range=0.979)

**DNA Gaps:** None — all dimensions exceed 0.4 range. Best-balanced coverage of this batch.

**DNA Cross-Validation:**
- Atmosphere (need agg<0.4 AND space>0.5): 39/74 violations (52.7%) — highest rate in batch.
- Foundation (need density>0.4): 4/49 violations (8.2%) — best in batch.
- Flux (need movement>0.5): 7/41 violations (17.1%) — best in batch.

---

### SAMPLE AUDIT (3 presets)

**1. "VIOLENT GROUND" — mood: Foundation**
File: `Foundation/VIOLENT_GROUND.xometa`
- Missing fields: `description`, `couplingIntensity`, `tempo`.
- Naming: PASS — "VIOLENT GROUND" is 2 words, 14 chars. Valid though ALL_CAPS style inconsistency.
- Param prefix: FAIL — uses bare `macro_character`, `macro_coupling` under both Orbital and Opal keys. No `orb_` prefix.
- DNA accuracy: PASS — aggression=0.933, movement=0.938, density=0.652. Foundation: density>0.4 PASS.
- Coupling check: Coupling present (Orbital→Opal FILTER_MOD, amount=0.737 PASS).
- Issues: Missing 3 required fields. No engine-prefixed params. Version "1.0".

**2. "DARK WARM TIGHT VIOLENT ATM..." — mood: Atmosphere**
File: `Atmosphere/DARK_WARM_TIGHT_VIOLENT_ATM_AGG_05.xometa`
- Missing fields: `description`, `couplingIntensity`, `tempo`.
- Naming: FAIL — name is "DARK WARM TIGHT VIOLENT ATM..." (the truncated version) — the actual name field content is "DARK WARM TIGHT VIOLENT ATM..." which is 5+ words AND >30 chars. Severe naming violation. The name contains the descriptor template, not an evocative name.
- Param prefix: FAIL — bare macro_ only.
- DNA accuracy: PASS — aggression=0.903, warmth=0.941 consistent with "warm violent" descriptor. Atmosphere cross-val: aggression=0.903 is ≥0.4 — FAILS. This is a high-aggression preset in Atmosphere mood which misrepresents the mood expectation.
- Issues: Name is raw descriptor system output, not a proper name. Missing 3 fields. DNA cross-validation failure (agg=0.903 in Atmosphere). Version "1.0".

**3. "Partial Room" — mood: Entangled**
File: `Entangled/Partial_Room.xometa`
- Missing fields: None. All required fields present.
- Naming: PASS — "Partial Room" is 2 words, 12 chars, evocative.
- Param prefix: PASS — Overlap params use `olap_` correctly (olap_couplingBus, olap_decay, etc.). Note: Orbital listed in engines but has no `parameters.Orbital` block.
- DNA accuracy: PASS — space=0.75, brightness=0.575, aggression=0.275 consistent with spatial/entangled character.
- Coupling check: PASS — SPECTRAL_CROSS from Overlap→Orbital, amount=0.555>0.1 PASS.
- Issues: Duplicate `sonic_dna` key present alongside `dna`. Orbital in engines but no Orbital params block.

**P0 Issues:**
- 5 single-engine Orbital presets with wrong parameter prefixes (bare or `couplingBus`/`couplingLevel` without `orb_`).
- Orbital missing from parameters block in Partial_Room despite being in engines array.

**P1 Issues:**
- 231/552 presets (42%) missing `description`.
- 232/552 presets (42%) missing `couplingIntensity`.
- 39/74 Atmosphere presets fail DNA cross-validation (52.7% — highest in batch; many high-aggression Orbital presets placed in Atmosphere).
- 3 presets with folder/mood mismatch.

**P2 Issues:**
- 93 presets with bad names (23 single-word, 70 four+ words — many raw descriptor sequences).
- Atmosphere DNA cross-validation is the most critical quality issue for this engine.
- 53 presets with duplicate `sonic_dna` key.

**P3 Issues:**
- 275 presets missing `tempo` field.
- 250 presets use version "1.0".
- Missing Prism and Family moods.

---

## ENGINE: ORGANON

**Preset Count:** 751 (largest in this batch)
**Mood Distribution:** Foundation=55, Atmosphere=85, Entangled=183 (highest), Flux=91, Aether=86
**Missing Moods:** Prism=0, Family=0, Submerged=0

**DNA Range (751 presets):**
- brightness: 0.000–1.000 (range=1.000)
- warmth: 0.020–1.000 (range=0.980)
- movement: 0.014–1.000 (range=0.986)
- density: 0.021–1.000 (range=0.979)
- space: 0.000–1.000 (range=1.000)
- aggression: 0.000–0.990 (range=0.990)

**DNA Gaps:** None — all dimensions exceed 0.4 range.

**DNA Cross-Validation:**
- Atmosphere (need agg<0.4 AND space>0.5): 46/89 violations (51.7%).
- Foundation (need density>0.4): 18/55 violations (32.7%).
- Flux (need movement>0.5): 29/91 violations (31.9%).

---

### SAMPLE AUDIT (3 presets)

**1. "Pink Noise Cell" — mood: Foundation**
File: `Foundation/Organon_Pink_Noise_Cell.xometa`
- Missing fields: None. All required fields present.
- Naming: PASS — "Pink Noise Cell" is 3 words, 15 chars, evocative.
- Param prefix: PASS — uses `organon_metabolicRate`, `organon_membrane`, `organon_enzymeSelect` etc. — all `organon_` prefixed.
- DNA accuracy: PASS — warmth=0.62, density=0.48, aggression=0.18. Foundation cross-val: density=0.48>0.4 PASS.
- Issues: `enzymeSelect` has value `1` (integer) in Pink_Noise_Cell but value `800.0` (float frequency) in Tidal Pulse — same param name used for different value semantics across presets. Possible documentation gap or range inconsistency.

**2. "Tone Shimmer 3" — mood: Atmosphere**
File: `Atmosphere/DARK_COLD_KINETIC_SPARSE_VAST_ATM_2.xometa`
- Missing fields: `description`, `couplingIntensity`, `tempo`.
- Naming: FAIL — name is "Tone Shimmer 3". The filename is "DARK_COLD_KINETIC_SPARSE_VAST_ATM_2" — the name field doesn't match the file descriptor at all. "Tone Shimmer 3" is a numbered sequence name (violates uniqueness and evocativeness convention — raw sequence number suffix).
- Param prefix: FAIL — uses bare `macro_character`, `macro_coupling` under Organon key. No `organon_` prefix.
- DNA accuracy: PASS — space=0.9076, movement=0.9221 consistent with "vast/kinetic" descriptor. Atmosphere cross-val: aggression=0.5425≥0.4 — FAILS.
- Issues: Name/filename mismatch. Bare macro_ params. Missing 3 fields. DNA cross-val fails (high aggression in Atmosphere). Version "1.0".

**3. "Oxide Organ" — mood: Entangled**
File: `Entangled/Oxide_Organ.xometa`
- Missing fields: `description`, `couplingIntensity`, `tempo`.
- Naming: PASS — "Oxide Organ" is 2 words, 11 chars, evocative.
- Param prefix: PARTIAL — Organon uses bare `macro_character`, `macro_coupling` (no organon_ prefix). Overdub uses `macro_character` plus one `dub_lfoDepth` (correct Overdub prefix).
- DNA accuracy: PASS — warmth=0.89, density=0.93 consistent with "scorching/warm-dense" tags.
- Coupling check: PASS — Overdub→Organon FILTER_MOD, amount=0.74>0.1 PASS.
- Issues: Missing 3 fields. Organon params bare macro_ only. Version "1.0".

**P0 Issues:**
- 9 single-engine Organon presets with wrong parameter prefixes: 4 use `org_` prefix (incorrect — should be `organon_`), 5 have bare unprefixed params. These will fail on load.
- 20 Entangled presets have `coupling: null` — highest null-coupling count in batch. Null coupling will likely throw in PresetManager.
- 139 presets with `coupling: null` across all moods — this is a systemic issue for Organon specifically (139/751 = 18.5% of all Organon presets).

**P1 Issues:**
- 317/751 presets (42%) missing `description`.
- 347/751 presets (46%) missing `couplingIntensity`.
- 23 presets with mood field mismatch (in Entangled folder, mood="Foundation" in JSON — highest mislabeled count in batch).
- 46/89 Atmosphere violations (51.7%).
- 29/91 Flux violations (31.9%).

**P2 Issues:**
- 108 presets with bad names (highest 1-word count: 36; 72 are 4+-word descriptor sequences).
- `organon_enzymeSelect` has inconsistent value type across presets (integer 1 vs float 800.0) — possible param semantics drift.
- 47 presets with duplicate `sonic_dna` key.

**P3 Issues:**
- 386 presets missing `tempo` field (highest in batch).
- 382 presets use version "1.0" (highest in batch).
- Missing Prism, Family, Submerged moods.

---

## ENGINE: OUROBOROS

**Preset Count:** 624
**Mood Distribution:** Foundation=69, Atmosphere=33 (low), Entangled=167, Flux=79, Aether=47
**Missing Moods:** Prism=0, Family=0, Submerged=0

**DNA Range (624 presets):**
- brightness: 0.000–1.000 (range=1.000)
- warmth: 0.000–1.000 (range=1.000)
- movement: 0.000–1.000 (range=1.000)
- density: 0.021–1.000 (range=0.979)
- space: 0.000–1.000 (range=1.000)
- aggression: 0.000–1.000 (range=1.000)

**DNA Gaps:** None — full spectrum across all 6 dimensions.

**DNA Cross-Validation:**
- Atmosphere (need agg<0.4 AND space>0.5): 19/39 violations (48.7%).
- Foundation (need density>0.4): 19/72 violations (26.4%).
- Flux (need movement>0.5): 28/84 violations (33.3%).

---

### SAMPLE AUDIT (3 presets)

**1. "Endless Descent" — mood: Foundation**
File: `Foundation/Endless_Descent.xometa`
- Missing fields: `couplingIntensity`, `tempo`.
- Naming: PASS — "Endless Descent" is 2 words, 15 chars, evocative.
- Param prefix: FAIL — uses `uro_ampAttack`, `uro_ampDecay`, `uro_ampRelease`, `uro_character` etc. The correct prefix is `ouro_`. The `uro_` prefix is a legacy/drift error.
- DNA accuracy: INCONSISTENCY — the file has both `dna` and `sonic_dna` keys with identical values. The `description` says "dark extreme brightness preset" but brightness=0.09 is extremely dark, not bright — the description text is inverted/inaccurate.
- Foundation cross-val: density=0.42>0.4 PASS (barely).
- Issues: Wrong prefix `uro_` (should be `ouro_`). Missing couplingIntensity and tempo. Duplicate `sonic_dna`. Description text contradicts DNA values ("extreme brightness" vs brightness=0.09).

**2. "Orbit Drone" — mood: Atmosphere**
File: `Atmosphere/Orbit Drone.xometa`
- Missing fields: None — all required fields present.
- Naming: PASS — "Orbit Drone" is 2 words, 11 chars, evocative.
- Param prefix: PASS — uses `ouro_topology`, `ouro_rate`, `ouro_chaosIndex`, `ouro_leash`, `ouro_damping`, `ouro_injection` — all `ouro_` prefixed.
- DNA accuracy: PASS — brightness=0.15, warmth=0.8, movement=0.15, density=0.2, aggression=0.0. Atmosphere cross-val: agg=0.0<0.4 AND space=0.3 — FAILS space requirement (space=0.3, needs >0.5).
- Issues: `coupling: null` (not `{"pairs": []}`). Atmosphere DNA cross-validation fail (space too low for atmosphere).

**3. "Hot Venom" — mood: Entangled**
File: `Entangled/Hot_Venom.xometa`
- Missing fields: `description`, `couplingIntensity`, `tempo`.
- Naming: PASS — "Hot Venom" is 2 words, 9 chars, evocative.
- Param prefix: FAIL — bare `macro_character`, `macro_coupling` under Ouroboros key. No `ouro_` prefix.
- DNA accuracy: PASS — aggression=0.83, warmth=0.87, density=0.84 consistent with "scorching/aggressive" tags.
- Coupling check: PASS — Ouroboros→Orca PITCH_FOLLOW, amount=0.71>0.1 PASS.
- Issues: Missing 3 fields. Bare macro_ params. Version "1.0".

**P0 Issues:**
- 12 single-engine Ouroboros presets with wrong parameter prefixes: 4 files use `uro_` prefix (wrong — must be `ouro_`); 8 files use bare unprefixed params.
- 13 presets with `coupling: null` (second highest null count after Organon).
- Description text in "Endless Descent" directly contradicts DNA values (brightness described as "extreme brightness" but value is 0.09).

**P1 Issues:**
- 183/624 presets (29%) missing `description`.
- 211/624 presets (34%) missing `couplingIntensity`.
- 16 presets with mood field mismatch (in Entangled folder, mood="Flux" or "Foundation").
- 11 Entangled presets with empty/null/weak coupling.
- 28/84 Flux violations (33.3%).
- Atmosphere preset count (33) is low compared to other moods; Entangled at 167 is disproportionately weighted.

**P2 Issues:**
- 55 presets with bad names (16 single-word, 39 four+ words).
- 19/72 Foundation presets with density≤0.4.
- 90 presets with duplicate `sonic_dna` key (highest count in batch).

**P3 Issues:**
- 300 presets missing `tempo` field.
- 246 presets use version "1.0".
- Missing Prism, Family, Submerged moods.

---

## ENGINE: OBSIDIAN

**Preset Count:** 518
**Mood Distribution:** Foundation=73, Atmosphere=40, Entangled=161, Flux=41, Aether=43, Submerged=11
**Missing Moods:** Prism=0, Family=0

**DNA Range (518 presets):**
- brightness: 0.000–1.000 (range=1.000)
- warmth: 0.000–0.997 (range=0.997)
- movement: 0.000–0.978 (range=0.978)
- density: 0.000–1.000 (range=1.000)
- space: 0.000–1.000 (range=1.000)
- aggression: 0.000–1.000 (range=1.000)

**DNA Gaps:** None — full spectrum across all 6 dimensions.

**DNA Cross-Validation:**
- Atmosphere (need agg<0.4 AND space>0.5): 10/44 violations (22.7%) — best in batch.
- Foundation (need density>0.4): 19/77 violations (24.7%).
- Flux (need movement>0.5): 10/42 violations (23.8%) — best in batch.

---

### SAMPLE AUDIT (3 presets)

**1. "Iron Chord" — mood: Foundation**
File: `Foundation/Obsidian_Iron_Chord.xometa`
- Missing fields: `description` present, `couplingIntensity` present, `tempo` present. Actually missing: none. All required fields present.
- Naming: PASS — "Iron Chord" is 2 words, 10 chars, evocative.
- Param prefix: PASS — uses `obsidian_depth`, `obsidian_character`, `obsidian_filterCutoff` etc. All `obsidian_` prefixed.
- DNA accuracy: PASS — warmth=0.62, density=0.82, aggression=0.2. Foundation cross-val: density=0.82>0.4 PASS.
- Issues: `obsidian_filterCutoff` value is 4200 (absolute Hz). Other presets (Crystal Formation) use 6000.0. Should be normalized [0,1] or Hz consistently documented. Inconsistency in value domain.
- Note: macros are present with values (CHARACTER=0.42, MOVEMENT=0.18, COUPLING=0.0, SPACE=0.42). Clean structure.

**2. "Crystal Formation" — mood: Atmosphere**
File: `Atmosphere/Obsidian_Crystal_Formation.xometa`
- Missing fields: None. All required fields present.
- Naming: PASS — "Crystal Formation" is 2 words, 17 chars, evocative.
- Param prefix: PASS — uses `obsidian_ampAttack`, `obsidian_macroCharacter`, `obsidian_lfo1Depth` etc. All correct.
- DNA accuracy: PASS — aggression=0.1<0.4 AND space=0.88>0.5 — Atmosphere cross-val PASS.
- Issues: `obsidian_filterCutoff` is 6000.0 (absolute Hz, same inconsistency as Iron Chord). `legacy` block present with null values — minor legacy cruft. No significant issues.

**3. "Glass Cathedral" — mood: Entangled**
File: `Entangled/Evolution_Glass_Cathedral.xometa`
- Missing fields: None. All required fields present.
- Naming: PASS — name field is "Glass Cathedral" (2 words, 15 chars). File is "Evolution_Glass_Cathedral" — the "Evolution" prefix in filename but not name is slightly misleading but not a violation.
- Param prefix: PASS — Obsidian uses `obsidian_depth`, `obsidian_pdDepth`, `obsidian_filterCutoff`. Oblique uses `oblq_prismColor`, `oblq_bounceRate`. All correctly prefixed.
- DNA accuracy: PASS — movement=0.68, space=0.70, aggression=0.40. Entangled: appropriate coupling values.
- Coupling check: PASS — 3 coupling pairs, all amounts (0.55, 0.48, 0.42) > 0.1. PASS.
- Extra field: `journey` block with temporal envelope data. Non-standard field — not in schema. Potential unknown-field warning in PresetManager.
- Issues: `obsidian_filterCutoff` = 0.55 (normalized, different from Foundation/Atmosphere absolute Hz) — confirms inconsistent filterCutoff value domain within Obsidian library. `journey` extra field may be silently dropped.

**P0 Issues:**
- 9 single-engine Obsidian presets with wrong parameter prefixes: 9 files use `obs_` prefix (wrong — should be `obsidian_`), 14 keys are bare/unprefixed.
- `obsidian_filterCutoff` value domain is inconsistent: some presets use absolute Hz (4200, 6000.0), others use normalized [0,1] (0.55). This will cause incorrect filter behavior across presets — this is a P0 for audio correctness.

**P1 Issues:**
- 134/518 presets (26%) missing `description`.
- 161/518 presets (31%) missing `couplingIntensity`.
- 5 presets with folder/mood mismatch (in Entangled folder but mood="Atmosphere", "Flux", or "Foundation").
- 6 Entangled presets with empty coupling pairs.
- Non-standard `journey` field in Glass Cathedral (schema violation).

**P2 Issues:**
- 50 presets with bad names (14 single-word, 36 four+ words).
- 10/44 Atmosphere cross-validation violations (22.7% — best in batch but still ~1 in 5).
- 19/77 Foundation density violations (24.7%).
- 65 presets with duplicate `sonic_dna` key.

**P3 Issues:**
- 221 presets missing `tempo` field (lowest in batch — relatively better).
- 179 presets use version "1.0".
- Missing Prism and Family moods.

---

## CROSS-ENGINE FLEET SUMMARY

### Mood Coverage

| Engine | Foundation | Atmosphere | Entangled | Flux | Aether | Submerged | Prism | Family |
|--------|-----------|------------|-----------|------|--------|-----------|-------|--------|
| Onset | 96 | 40 | 123 | 50 | 42 | 0 | 0 | 0 |
| Overworld | 54 | 36 | 79 | 75 | 59 | 0 | 0 | 0 |
| Opal | 31 | 64 | 116 | 77 | 63 | 6 | 0 | 0 |
| Orbital | 46 | 73 | 134 | 40 | 95 | 10 | 0 | 0 |
| Organon | 55 | 85 | 183 | 91 | 86 | 0 | 0 | 0 |
| Ouroboros | 69 | 33 | 167 | 79 | 47 | 0 | 0 | 0 |
| Obsidian | 73 | 40 | 161 | 41 | 43 | 11 | 0 | 0 |

**Fleet-wide missing moods:** Prism=0 for all 7, Family=0 for all 7. This is a systematic gap across the entire fleet — neither Prism nor Family mood has any representation in this batch.

### DNA Cross-Validation Summary

| Engine | Atmosphere violations | Foundation violations | Flux violations |
|--------|-----------------------|----------------------|-----------------|
| Onset | 42.5% | 18.8% | 14% |
| Overworld | 46.7% | 23.1% | 30.1% |
| Opal | 44.4% | 35.4% | 37.9% |
| Orbital | 52.7% | 8.2% | 17.1% |
| Organon | 51.7% | 32.7% | 31.9% |
| Ouroboros | 48.7% | 26.4% | 33.3% |
| Obsidian | 22.7% | 24.7% | 23.8% |

**Observation:** Atmosphere DNA cross-validation is the most systemic failure across the fleet — every engine except Obsidian violates the rule in 40–52% of Atmosphere presets. The rule (agg<0.4 AND space>0.5) may be too strict, or a large number of presets that should be in Flux/Entangled are mislabeled as Atmosphere. Recommend either relaxing the cross-validation rule or running a mood re-sort pass on Atmosphere.

### Parameter Prefix Issues (P0)

| Engine | Correct prefix | Wrong-prefix presets | Prefix errors found |
|--------|---------------|----------------------|---------------------|
| Onset | perc_ | 9 | ons_ (4 files), bare (5 files) |
| Overworld | ow_ | 7 | bare unprefixed (7 files) |
| Opal | opal_ | 5 | bare unprefixed (5 files) |
| Orbital | orb_ | 5 | bare or couplingBus/Level (5 files) |
| Organon | organon_ | 9 | org_ (4 files), bare (5 files) |
| Ouroboros | ouro_ | 12 | uro_ (4 files), bare (8 files) |
| Obsidian | obsidian_ | 9 | obs_ (5 files), bare (4 files) |

**Total wrong-prefix presets:** 56 across the 7 engines.

### Null Coupling Issue

| Engine | coupling: null count | % of total |
|--------|---------------------|------------|
| Onset | 34 | 6.6% |
| Overworld | 76 | 13.2% |
| Opal | 110 | 16.5% |
| Orbital | 17 | 3.1% |
| Organon | 139 | 18.5% |
| Ouroboros | 71 | 11.4% |
| Obsidian | 22 | 4.2% |

**Total coupling: null presets:** 469. If PresetManager does not handle null coupling defensively, this is a fleet-wide crash risk.

### Missing Fields Summary

| Engine | Missing description | Missing couplingIntensity | Missing tempo |
|--------|--------------------|-----------------------------|---------------|
| Onset | 126 (24%) | 144 (28%) | 202 (39%) |
| Overworld | 210 (36%) | 225 (39%) | 280 (49%) |
| Opal | 262 (39%) | 327 (49%) | 358 (54%) |
| Orbital | 231 (42%) | 232 (42%) | 275 (50%) |
| Organon | 317 (42%) | 347 (46%) | 386 (51%) |
| Ouroboros | 183 (29%) | 211 (34%) | 300 (48%) |
| Obsidian | 134 (26%) | 161 (31%) | 221 (43%) |

### sonic_dna Duplicate Key

Total presets with legacy `sonic_dna` key alongside `dna`: Onset=75, Overworld=79, Opal=65, Orbital=53, Organon=47, Ouroboros=90, Obsidian=65. **Total: 474 presets with this duplicate field.**

---

## PRIORITIZED ACTION LIST

### P0 — Must fix before shipping

1. **Parameter prefix errors (56 presets):** Fix `ons_`→`perc_`, `org_`→`organon_`, `uro_`→`ouro_`, `obs_`→`obsidian_` across all identified files. Fix bare-unprefixed params.
2. **Null coupling objects (469 presets):** Replace all `coupling: null` with `{"pairs": []}`. This is a potential runtime crash.
3. **Obsidian filterCutoff value domain inconsistency:** Some presets use absolute Hz (4200, 6000), others use normalized [0,1]. Establish canonical domain and normalize all Obsidian presets.
4. **Engines listed without parameters block:** Opal in Octopus_OPAL_Grain_Morph, Orbital in Partial_Room — missing engine params blocks despite engine being in engines array.
5. **Macro label/param key mismatch in Opal (Shimmer Veil):** macroLabels say CHARACTER/MOVEMENT but params use macroScatter/macroDrift.

### P1 — Doctrine violations

6. **Mood field mismatch (57 presets across 7 engines):** Move misplaced files to correct mood folders OR update internal mood field to match folder. Organon has 23 such files.
7. **Entangled presets with empty/null coupling (73 presets):** Add meaningful coupling pairs or move to correct non-Entangled mood.
8. **Description field missing (1,463 presets across 7 engines):** Priority: add descriptions to all single-engine presets first. Coupled presets second.
9. **couplingIntensity missing (1,647 presets):** Bulk-fill based on coupling pairs: None if empty, Light/Moderate/Deep/Extreme based on amount.
10. **Atmosphere DNA cross-validation failures (fleet-wide ~46% average):** Either re-sort high-aggression Atmosphere presets to Flux, or formally revise the Atmosphere DNA rule to permit wider aggression range.
11. **Flux movement violations (Overworld 30%, Opal 38%, Ouroboros 33%):** Re-sort still/slow presets to Foundation or create explicit "Slow Flux" distinction.

### P2 — Quality concerns

12. **Naming convention violations (534 presets fleet-wide):** Replace descriptor-sequence names (BRIGHT_HOT_DENSE_VAST_AET3_4, etc.) with evocative 2–3 word names.
13. **sonic_dna duplicate key (474 presets):** Remove legacy `sonic_dna` key from all affected files.
14. **Organon enzymeSelect value inconsistency:** Audit all Organon presets for enzymeSelect value type (integer vs Hz float).
15. **Ouroboros description inaccuracy in Endless Descent:** Description says "extreme brightness" but brightness DNA=0.09. Correct the description.
16. **Foundation density coverage (Opal 35%, Organon 33%):** Many Foundation presets have density≤0.4 — inconsistent with mood expectation.

### P3 — Polish items

17. **Version field normalization:** Bulk-update all "1.0" → "1.0.0" (1,582 presets across 7 engines).
18. **tempo field missing (2,022 presets):** Add `"tempo": null` to all presets missing the field.
19. **Prism mood (0 presets for all 7 engines):** Create at least 5–10 Prism presets per engine.
20. **Family mood (0 presets for all 7 engines):** Create at least 5–10 Family presets per engine.
21. **Non-standard journey field (Obsidian Glass Cathedral):** Document or remove the undeclared `journey` field.
22. **ALL_CAPS name style:** Normalize to title-case per canonical examples (Hat Cathedral, Boss Fight, Pink Noise Cell).
