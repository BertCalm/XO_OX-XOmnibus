# XOceanus Preset Quality Audit — Batch 1
**Engines:** OddfeliX, OddOscar, Overdub, Odyssey, Oblong, Obese, Overbite
**Date:** 2026-03-21
**Auditor:** Rodrigo (automated + structural analysis)
**Scope:** Solo engine presets only (multi-engine coupling presets excluded from counts)

---

## Methodology Notes

- **Solo preset count** = presets where `engines` array contains exactly this one engine
- **Multi-engine (coupling) presets** are not counted in mood distributions but do exist in the pool (totals range from 272–586 additional per engine)
- DNA cross-validation thresholds: Atmosphere → aggression < 0.4 AND space > 0.5; Foundation → density > 0.4; Flux → movement > 0.5
- Overbite B008 Blessing (`Five-Macro System: BELLY/BITE/SCURRY/TRASH/PLAY DEAD`) creates a legitimate 5-macro configuration that conflicts with the "exactly 4 macroLabels" audit rule — flagged as P1 doctrine conflict, not P0

---

## ENGINE: OddfeliX

**Preset Count:** 352 solo | +440 multi-engine coupling presets
**Mood Distribution:** Foundation=80, Atmosphere=60, Entangled=53, Prism=60, Flux=37, Aether=48, Family=11, Submerged=3
**Missing Moods:** None (all 8 standard moods present)
**Thin Moods:** Family=11, Submerged=3, Flux=37 (below ~50 threshold)

**DNA Range (50-preset sample):**
brightness=[0.070–0.969] range=0.899
warmth=[0.000–0.987] range=0.987
movement=[0.076–0.959] range=0.883
density=[0.041–0.962] range=0.921
space=[0.057–0.951] range=0.894
aggression=[0.045–0.981] range=0.936

**DNA Gaps:** None (all dimensions range > 0.4)

---

### SAMPLE AUDIT (3 presets):

**1. "Amen Shatter" — mood: Foundation**
Missing fields: NONE
Naming: PASS (2 words, 12 chars)
Param prefix: PASS (legacy pre-migration params present — `oBloom`, `xSnap`, `compAttack` etc., all covered by `resolveEngineAlias()`)
DNA accuracy: PASS (Foundation: density=0.438 — marginal, but passes > 0.4 threshold)
Issues:
- macroLabels FAIL — `["Snap+Morph", "Bloom", "Coupling", "Space"]` — mixed case, not all UPPERCASE
- `coupling: null` instead of `{"pairs": []}` — null coupling object
- `couplingIntensity: "None"` set but `coupling: null` — inconsistent
- Has `"legacy"` key and `"created"` field not in standard schema (extra fields OK, not missing)

**2. "Amber Tide" — mood: Atmosphere**
Missing fields: NONE
Naming: PASS (2 words, 10 chars)
Param prefix: PASS (legacy params)
DNA accuracy: FAIL — space=0.472 (should be > 0.5 for Atmosphere); warmth=0.395 (below Atmosphere expectations)
Issues:
- macroLabels FAIL — `["Snap+Morph", "Bloom", "Coupling", "Space"]` — same fleet-wide legacy macro label issue
- `coupling: null` — null coupling object
- `couplingIntensity: "Subtle"` but coupling is null — inconsistent

**3. "DARK COLD SPARSE VAST F2" — mood: Flux**
Missing fields: NONE
Naming: FAIL — 5 words ("DARK COLD SPARSE VAST F2"), violates 2–3 word rule
Param prefix: FAIL — empty parameters block `{}` (no actual params stored)
DNA accuracy: PASS (Flux: movement=0.506 — barely passes)
Issues:
- Empty parameters block — preset has no engine parameters (P0: will produce silent/default patch)
- Name violates convention — auto-generated descriptor names not evocative
- macroLabels `["MACRO1", "MACRO2", "MACRO3", "MACRO4"]` — placeholder labels, not functional
- `couplingIntensity: "Harmonic"` set but empty params block and no actual coupling data

---

### Fleet-Wide Issues for OddfeliX:

| Issue | Count | % |
|-------|-------|---|
| Empty params block | 16 | 4.5% |
| Wrong/unexpected param prefix | 13 | 3.7% |
| macroLabels not all UPPERCASE | 114 | 32.4% |
| Naming violations (not 2–3 words or >30 chars) | 33 | 9.4% |
| DNA/mood cross-validation fails | 58 | 16.5% |
| Version "1.0" instead of "1.0.0" | 10 | 2.8% |

**P0 Issues:**
- 16 presets have completely empty `parameters` blocks — these will produce default/silent patches
- 13 presets use wrong param prefix (`felix_` or `morph_` keys inside OddfeliX params block) — engine will fail to read parameters

**P1 Issues:**
- 114 presets (32%) use mixed-case macroLabels `["Snap+Morph", "Bloom", "Coupling", "Space"]` — a single legacy template that was mass-applied. All should be `["SNAP+MORPH", "BLOOM", "COUPLING", "SPACE"]` or updated to engine-appropriate labels
- 58 presets fail DNA/mood cross-validation — mostly Atmosphere presets with space < 0.5 and Foundation presets with density values < 0.4 (legacy bass presets with sparse DNA)

**P2 Issues:**
- 33 naming violations: 1-word names ("Finray", "Luminance"), 4+ word auto-generated names ("Ghost in the Machine", "DARK COLD VIOLENT DENSE 1")
- Family=11 and Submerged=3 mood undercoverage — few solo presets represent these moods

**P3 Issues:**
- 10 presets use version "1.0" instead of "1.0.0" — minor version format inconsistency
- Many presets have `coupling: null` rather than `{"pairs": []}` — technically valid but inconsistent with newer schema style

---

## ENGINE: OddOscar

**Preset Count:** 235 solo | +486 multi-engine coupling presets
**Mood Distribution:** Foundation=57, Atmosphere=47, Entangled=3, Prism=35, Flux=44, Aether=33, Family=9, Submerged=7
**Missing Moods:** None (all 8 present)
**Thin Moods:** Entangled=3 (critically thin), Family=9, Submerged=7

**DNA Range (50-preset sample):**
brightness=[0.022–0.990] range=0.968
warmth=[0.034–0.981] range=0.947
movement=[0.063–0.964] range=0.901
density=[0.052–0.977] range=0.925
space=[0.041–0.964] range=0.923
aggression=[0.023–0.982] range=0.959

**DNA Gaps:** None

---

### SAMPLE AUDIT (3 presets):

**1. "Amber Mass" — mood: Foundation**
Missing fields: NONE
Naming: PASS (2 words, 10 chars)
Param prefix: FAIL — empty params block `{}`
DNA accuracy: PASS (density=0.992 — excellent for Foundation)
Issues:
- Empty parameters block — no engine parameters stored
- `sonic_dna` key duplicates `dna` key — redundant but harmless

**2. "DARK KINETIC VIOLENT OPEN P..." — mood: Prism**
Missing fields: NONE
Naming: FAIL — name truncated to 30 chars mid-word ("DARK KINETIC VIOLENT OPEN P..."), 5+ word auto-descriptor
Param prefix: FAIL — empty params block
DNA accuracy: PASS (Prism: no specific threshold)
Issues:
- Name truncated at 30-char limit — name was auto-generated and then hard-cut at character boundary
- Empty parameters block
- `coupling` has old structure `{"intensity": "None", "sourceEngine": "OddOscar", "targetEngine": null, "type": "Envelope"}` — legacy coupling schema, different from current `{"pairs": []}` format

**3. "Below Zero" — mood: Aether**
Missing fields: NONE
Naming: PASS (2 words, 10 chars)
Param prefix: FAIL — empty params block
DNA accuracy: PASS
Issues:
- Empty parameters block
- `sonic_dna` duplicates `dna`

---

### Fleet-Wide Issues for OddOscar:

| Issue | Count | % |
|-------|-------|---|
| Empty params block | 24 | 10.2% |
| Wrong/unexpected param prefix | 14 | 5.9% |
| macroLabels issues | 2 | 0.9% |
| Naming violations | 12 | 5.1% |
| DNA/mood cross-validation fails | 9 | 3.8% |
| Version "1.0" | 16 | 6.8% |

**P0 Issues:**
- 24 presets (10%) have empty params blocks — highest ratio in this batch
- 14 presets use wrong prefix (`oscar_` or `oddo_` keys) — engine cannot read parameters

**P1 Issues:**
- Entangled=3 solo presets is critically thin — OddOscar has almost no solo Entangled presence
- 1 preset uses legacy coupling schema `{"intensity":..., "sourceEngine":..., "targetEngine":..., "type":...}` — incompatible with current `pairs` format
- 9 DNA/mood failures (mostly Atmosphere space < 0.5)

**P2 Issues:**
- 12 naming violations including 1-word names ("Regenerate", "Eigenfrequency", "Undercurrent") and truncated auto-descriptors
- 2 macroLabel issues (minor — only 2 presets affected)

**P3 Issues:**
- 16 presets on version "1.0"
- Duplicate `sonic_dna`/`dna` keys in ~30% of presets — redundant but harmless

---

## ENGINE: Overdub

**Preset Count:** 205 solo | +464 multi-engine coupling presets
**Mood Distribution:** Foundation=54, Atmosphere=32, Entangled=8, Prism=42, Flux=34, Aether=24, Family=11, Submerged=0
**Missing Moods:** Submerged=0
**Thin Moods:** Submerged=0 (absent), Atmosphere=32, Aether=24

**DNA Range (50-preset sample):**
brightness=[0.026–0.965] range=0.938
warmth=[0.046–0.971] range=0.924
movement=[0.023–0.983] range=0.960
density=[0.036–0.974] range=0.938
space=[0.041–1.000] range=0.959
aggression=[0.000–0.995] range=0.995

**DNA Gaps:** None

---

### SAMPLE AUDIT (3 presets):

**1. "Cave Resonance" — mood: Foundation**
Missing fields: NONE
Naming: PASS (2 words, 14 chars)
Param prefix: PASS — all params use `dub_` prefix correctly
DNA accuracy: PASS (density=0.6232, warmth=0.8372)
Issues:
- `sonic_dna` key duplicates `dna` — redundant
- `macros` field included at top level with macro values (redundant with parameters; acceptable)

**2. "Abyss Pad" — mood: Atmosphere**
Missing fields: NONE
Naming: PASS (2 words, 9 chars)
Param prefix: PASS — all params use `dub_` prefix
DNA accuracy: PASS (space=0.789, aggression=0.182)
Issues:
- `coupling: null` — null coupling object (inconsistent with newer schema)
- Has `"legacy"` key (extra field, not schema violation)
- Clean, well-structured preset overall

**3. "BRIGHT HOT VIOLENT KINETIC F5" — mood: Flux**
Missing fields: NONE
Naming: FAIL — 5 words, auto-descriptor pattern
Param prefix: FAIL — empty params block
DNA accuracy: PASS (movement=0.946)
Issues:
- Empty parameters block
- Auto-generated name pattern — not evocative
- macroLabels `["MACRO1", "MACRO2", "MACRO3", "MACRO4"]` — placeholder labels

---

### Fleet-Wide Issues for Overdub:

| Issue | Count | % |
|-------|-------|---|
| Empty params block | 25 | 12.2% |
| Wrong/unexpected param prefix | 7 | 3.4% |
| macroLabels issues | 0 | 0% |
| Naming violations | 14 | 6.8% |
| DNA/mood cross-validation fails | 13 | 6.3% |
| Version "1.0" | 22 | 10.7% |

**P0 Issues:**
- 25 presets (12%) have empty params blocks — second highest ratio
- Submerged mood entirely absent — 0 solo presets

**P1 Issues:**
- 7 presets use wrong param prefix
- 13 DNA/mood failures: 6 Foundation presets with density < 0.4 (dub siren and drum hit types have sparse DNA); 5 Flux presets with movement < 0.5

**P2 Issues:**
- 14 naming violations: 4-word compound names, 1-word names ("Nyahbinghi"), 5-word auto-descriptors
- Atmosphere=32 is thin for a synth with dub atmosphere character

**P3 Issues:**
- 22 presets on version "1.0" — highest raw count in batch
- `coupling: null` common in legacy presets

---

## ENGINE: Odyssey

**Preset Count:** 386 solo | +528 multi-engine coupling presets
**Mood Distribution:** Foundation=78, Atmosphere=100, Entangled=3, Prism=116, Flux=34, Aether=38, Family=7, Submerged=10
**Missing Moods:** None
**Thin Moods:** Entangled=3 (critically thin), Family=7, Flux=34

**DNA Range (50-preset sample):**
brightness=[0.000–0.957] range=0.957
warmth=[0.000–0.990] range=0.990
movement=[0.000–0.946] range=0.946
density=[0.017–0.990] range=0.973
space=[0.000–0.970] range=0.970
aggression=[0.000–0.900] range=0.900

**DNA Gaps:** None (aggression max=0.900 — consider if fleet needs presets pushing to 0.95+ for Odyssey)

---

### SAMPLE AUDIT (3 presets):

**1. "808 Sub" — mood: Foundation**
Missing fields: NONE
Naming: PASS (2 words, 7 chars — though "808 Sub" is mildly jargon-adjacent)
Param prefix: PASS — `drift_` prefix throughout
DNA accuracy: FAIL — density=0.142 (Foundation requires > 0.4); space=0.0, movement=0.0 (extreme values for a bass patch)
Issues:
- Foundation density=0.142 fails cross-validation (sub bass has low density DNA — this is a systematic issue where bass/808 presets use acoustic character DNA rather than sonic density DNA)
- Tags: `["dyssey"]` — typo ("dyssey" should be "odyssey")
- `coupling: null` — inconsistent
- `"legacy"` key present

**2. "The Moment Before" — mood: Atmosphere**
Missing fields: NONE
Naming: PASS (3 words, 17 chars)
Param prefix: PASS — `drift_` throughout
DNA accuracy: PASS (aggression=0.0, space=0.75)
Issues:
- Non-standard extra fields: `macroJourney`, `climaxThreshold`, `bloomTime`, `modRouting` — custom Odyssey climax system fields. Not in base schema but intentional engine-specific extension
- Clean, well-authored preset with detailed description

**3. "Fractal Cascade" — mood: Prism**
Missing fields: NONE
Naming: PASS (2 words, 15 chars)
Param prefix: PASS — `drift_` throughout
DNA accuracy: PASS
Issues:
- Same non-standard extra fields (`macroJourney`, `climaxThreshold`, `bloomTime`, `modRouting`)
- Clean overall

---

### Fleet-Wide Issues for Odyssey:

| Issue | Count | % |
|-------|-------|---|
| Empty params block | 14 | 3.6% |
| Wrong/unexpected param prefix | 8 | 2.1% |
| macroLabels issues | 0 | 0% |
| Naming violations | 16 | 4.1% |
| DNA/mood cross-validation fails | 118 | 30.6% |
| Version "1.0" | 9 | 2.3% |

**P0 Issues:**
- 14 presets with empty params blocks
- Typo in tags: `"dyssey"` (at least one preset confirmed — likely a batch-generation artifact)

**P1 Issues:**
- 118 DNA/mood cross-validation failures (30.6%) — by far the highest in the batch. Two systemic causes:
  1. **Foundation density failures**: Odyssey's bass/808 presets systematically have low density DNA (0.07–0.21). The engine produces rooted bass sounds acoustically, but the DNA metadata doesn't reflect Foundation's "dense/grounded" expectation. ~80 presets affected
  2. **Atmosphere space failures**: Many Odyssey atmospheric presets have space < 0.5 (range 0.27–0.49). The engine creates intimate atmospheres as well as vast ones — the DNA threshold may need Odyssey-specific calibration
- Entangled=3 solo presets — critically thin Entangled presence
- 8 wrong prefix presets

**P2 Issues:**
- 16 naming violations: 1-word names ("Underwater", "Ascension"), 4-word names, 5-word auto-descriptors
- Family=7 is thin

**P3 Issues:**
- Custom Odyssey-specific JSON fields (`macroJourney`, `climaxThreshold`, `bloomTime`, `modRouting`) in Prism/Atmosphere climax presets — not in base schema. Should be documented as intentional extension
- Odyssey aggression max=0.900 — consider whether the engine should have higher-aggression presets

---

## ENGINE: Oblong

**Preset Count:** 321 solo | +586 multi-engine coupling presets
**Mood Distribution:** Foundation=57, Atmosphere=56, Entangled=40, Prism=70, Flux=63, Aether=31, Family=4, Submerged=0
**Missing Moods:** Submerged=0
**Thin Moods:** Submerged=0 (absent), Family=4 (critically thin), Aether=31

**DNA Range (50-preset sample):**
brightness=[0.050–0.963] range=0.913
warmth=[0.026–0.981] range=0.955
movement=[0.000–0.975] range=0.975
density=[0.034–0.977] range=0.943
space=[0.000–0.933] range=0.933
aggression=[0.000–0.970] range=0.970

**DNA Gaps:** None

---

### SAMPLE AUDIT (3 presets):

**1. "Abyssal Root" — mood: Foundation**
Missing fields: NONE
Naming: PASS (2 words, 12 chars)
Param prefix: PASS — `bob_` throughout
DNA accuracy: PASS (density=0.6558)
Issues:
- `sonic_dna` duplicates `dna` key
- Parameters minimal (4 keys) — `bob_couplingBus`, `bob_couplingLevel`, `bob_macro_*`, `bob_curAmount`, `bob_lfo1Depth`. Very sparse parameter set — may not express full engine depth

**2. "Accordion Rust" — mood: Flux**
Missing fields: NONE
Naming: PASS (2 words, 14 chars)
Param prefix: PASS — `bob_` throughout
DNA accuracy: FAIL — movement=0.468 (Flux requires > 0.5)
Issues:
- movement=0.468 barely misses Flux threshold — this is a borderline case (quasi-static accordion texture)
- `coupling: null` — inconsistent

**3. "Alien Gong" — mood: Prism**
Missing fields: NONE
Naming: PASS (2 words, 10 chars)
Param prefix: PASS — `bob_` throughout
DNA accuracy: PASS
Issues:
- movement=0.0 in Prism — zero movement in a Prism preset is unusual but not a violation
- `coupling: null`
- Clean, well-structured

---

### Fleet-Wide Issues for Oblong:

| Issue | Count | % |
|-------|-------|---|
| Empty params block | 17 | 5.3% |
| Wrong/unexpected param prefix | 13 | 4.0% |
| macroLabels issues | 0 | 0% |
| Naming violations | 20 | 6.2% |
| DNA/mood cross-validation fails | 74 | 23.1% |
| Version "1.0" | 18 | 5.6% |

**P0 Issues:**
- 17 presets with empty params blocks
- Submerged=0 — mood entirely absent
- Family=4 — critically thin (4 presets is barely a presence)

**P1 Issues:**
- 74 DNA/mood failures (23.1%) — second highest in batch:
  1. **Atmosphere space failures** (38 presets): Most Oblong Atmosphere presets have space < 0.5 (range 0.33–0.49). Oblong produces close, intimate textures — this is a character-vs-threshold tension
  2. **Foundation density failures** (16 presets): Oblong bass/low presets with density < 0.4
  3. **Flux movement failures** (20 presets): Static Oblong textures in Flux — "Hiss Cathedral", "Reel Room", "Old Radio" have movement=0.0
- 13 wrong prefix presets

**P2 Issues:**
- 20 naming violations: 1-word names ("Lambswool", "Hearthside", "Antenna"), 5-word auto-descriptors
- Aether=31 relatively thin

**P3 Issues:**
- 18 presets on version "1.0"

---

## ENGINE: Obese

**Preset Count:** 151 solo | +496 multi-engine coupling presets
**Mood Distribution:** Foundation=41, Atmosphere=25, Entangled=3, Prism=24, Flux=34, Aether=19, Family=2, Submerged=3
**Missing Moods:** None (all 8 present)
**Thin Moods:** Entangled=3 (critically thin), Family=2 (critically thin), Aether=19, Atmosphere=25

**DNA Range (50-preset sample):**
brightness=[0.070–0.931] range=0.861
warmth=[0.000–0.970] range=0.970
movement=[0.054–0.977] range=0.923
density=[0.029–1.000] range=0.971
space=[0.051–0.992] range=0.941
aggression=[0.020–0.990] range=0.970

**DNA Gaps:** None (brightness max=0.931 — slightly low ceiling; consider whether Obese needs brighter presets)

---

### SAMPLE AUDIT (3 presets):

**1. "Asphalt Drift" — mood: Foundation**
Missing fields: NONE
Naming: PASS (2 words, 13 chars)
Param prefix: PASS — `fat_` throughout
DNA accuracy: PASS (density=0.53, warmth=0.9)
Issues:
- `version: "1.0"` — non-standard version format
- `sonic_dna` duplicates `dna`
- Clean otherwise

**2. "Analog Cloud" — mood: Atmosphere**
Missing fields: NONE — but `tempo: null` is present (fine)
Naming: PASS (2 words, 12 chars)
Param prefix: PASS — `fat_` throughout (uses both `fat_fltCutoff` and `fat_ampAttack` pattern correctly; NOTE: one preset found with `ob_` prefix — see P0)
DNA accuracy: PASS (space=0.6, aggression=0.0)
Issues:
- `coupling: null` — inconsistent
- Full parameter set (30+ params) — exemplary preset depth
- Clean, well-authored

**3. "Arp Cascade" — mood: Flux**
Missing fields: NONE
Naming: PASS (2 words, 11 chars)
Param prefix: PASS — `fat_` throughout
DNA accuracy: PASS (movement=0.9)
Issues:
- `coupling: null`
- Full parameter set — well-authored
- Clean

---

### Fleet-Wide Issues for Obese:

| Issue | Count | % |
|-------|-------|---|
| Empty params block | 16 | 10.6% |
| Wrong/unexpected param prefix | 8 | 5.3% |
| macroLabels issues | 0 | 0% |
| Naming violations | 8 | 5.3% |
| DNA/mood cross-validation fails | 8 | 5.3% |
| Version "1.0" | 15 | 9.9% |

**P0 Issues:**
- 16 presets (10.6%) with empty params blocks
- At least 1 preset (`Molten_Core.xometa`) confirmed using `ob_` prefix instead of `fat_` — engine cannot read parameters
- Family=2 and Entangled=3 — critically thin coverage

**P1 Issues:**
- 8 wrong prefix presets (at minimum `ob_` prefix leakage)
- 8 DNA/mood failures (low rate — best cross-validation score in batch)

**P2 Issues:**
- 8 naming violations: 5-word auto-descriptors ("DARK KINETIC VIOLENT VAST 4"), 1-word? check needed
- Atmosphere=25 is thin for a synth with rich pad/atmospheric character (Mojo drift, 13-oscillator pads)
- Aether=19 thin

**P3 Issues:**
- 15 presets on version "1.0"
- brightness ceiling at 0.931 — may indicate lack of very bright Obese presets

---

## ENGINE: Overbite

**Preset Count:** 162 solo | +272 multi-engine coupling presets
**Mood Distribution:** Foundation=39, Atmosphere=21, Entangled=4, Prism=28, Flux=38, Aether=17, Family=10, Submerged=5
**Missing Moods:** None (all 8 present)
**Thin Moods:** Entangled=4 (critically thin), Aether=17, Atmosphere=21

**DNA Range (50-preset sample):**
brightness=[0.045–0.973] range=0.928
warmth=[0.010–0.963] range=0.953
movement=[0.035–0.943] range=0.908
density=[0.055–0.975] range=0.920
space=[0.000–0.990] range=0.990
aggression=[0.020–0.958] range=0.938

**DNA Gaps:** None

---

### SAMPLE AUDIT (3 presets):

**1. "Belly Rub" — mood: Foundation**
Missing fields: NONE
Naming: PASS (2 words, 9 chars)
Param prefix: PASS — `poss_` throughout, full parameter set (120+ params)
DNA accuracy: PASS (density=0.5, warmth=0.6)
Issues:
- space=0.0 in Foundation — zero space is extreme but not invalid for Foundation
- `coupling: null` — inconsistent
- Exemplary parameter completeness — fullest param set in any sampled preset

**2. "Belly Drone" — mood: Atmosphere**
Missing fields: NONE
Naming: PASS (2 words, 11 chars)
Param prefix: FAIL — uses `poss_oscAWave`, `poss_fur`, `poss_gnash`, `poss_filtEnvSustain` — mixed old schema keys (not in same format as "Belly Rub"). Check: `poss_fur` vs `poss_furAmount`, `poss_glide` vs `poss_glideTime` — inconsistent param naming within same engine
DNA accuracy: FAIL — space=0.30 (Atmosphere requires > 0.5)
Issues:
- Inconsistent param key naming within engine (old vs new key names — `poss_fur` / `poss_gnash` / `poss_glide` vs `poss_furAmount` / `poss_gnashAmount` / `poss_glideTime`)
- space=0.30 fails Atmosphere cross-validation
- macroLabels: `["BELLY", "BITE", "SCURRY", "SPACE"]` — 4th macro is "SPACE" instead of "TRASH" — inconsistent with B008 macros
- `coupling: null`

**3. "Acid Surge" — mood: Flux**
Missing fields: FAIL — missing `description` (wait — description IS present at bottom but JSON key order is non-standard)
Actually: `description` is present. Missing field: `tempo` field absent (not `"tempo": null` but absent entirely)
Naming: PASS (2 words, 10 chars)
Param prefix: FAIL — empty params block
DNA accuracy: PASS (movement=0.93)
Issues:
- Empty parameters block
- `version: "1.0"` — non-standard
- macros key references `CHARACTER/MOVEMENT/COUPLING/SPACE` but macroLabels are `BELLY/BITE/SCURRY/TRASH` — macros dict keys don't match macroLabels
- `tempo` key absent (not null) — minor schema inconsistency

---

### Fleet-Wide Issues for Overbite:

| Issue | Count | % |
|-------|-------|---|
| Empty params block | 15 | 9.3% |
| Wrong/unexpected param prefix | 3 | 1.9% |
| macroLabels count=5 (B008 conflict) | 58 | 35.8% |
| macroLabels count≠4 OR not UPPERCASE | 58 | 35.8% |
| Naming violations | 6 | 3.7% |
| DNA/mood cross-validation fails | 26 | 16.0% |
| Version "1.0" | 15 | 9.3% |

**SPECIAL NOTE — B008 Macro Count Conflict:**
Blessing B008 explicitly defines Overbite as having a "Five-Macro System (BELLY/BITE/SCURRY/TRASH/PLAY DEAD)". 58 presets (35.8%) use 5 macroLabels. The audit rule says "exactly 4". This is a **doctrine conflict**, not a preset error. Overbite is the exception that proves the rule — the UI must support 5 macros for this engine, or PLAY DEAD must be demoted. **This requires an owner decision before mass-fixing.** Flagged as P1 (doctrine violation) pending resolution.

**P0 Issues:**
- 15 presets with empty params blocks
- Param key naming inconsistency within engine: `poss_fur` vs `poss_furAmount`, `poss_glide` vs `poss_glideTime`, `poss_gnash` vs `poss_gnashAmount` — two different generations of param schema coexist

**P1 Issues:**
- 58 presets with 5-macroLabel array — doctrine conflict with 4-macro rule (B008 vs schema rule)
- `macros` dict keys sometimes reference `CHARACTER/MOVEMENT/COUPLING/SPACE` while `macroLabels` use `BELLY/BITE/SCURRY/TRASH` — these should be aligned
- 26 DNA/mood failures: 17 Flux presets with movement < 0.5 (Overbite's static/sustained bass textures placed in Flux); 7 Atmosphere presets with space < 0.5

**P2 Issues:**
- Inconsistent param key naming (two schema generations) — needs normalization pass
- Entangled=4 and Aether=17 thin

**P3 Issues:**
- 15 presets on version "1.0"
- Some presets missing `tempo` key entirely (vs `"tempo": null`)

---

## CROSS-ENGINE SUMMARY

### Shared Systemic Issues (All or Most Engines)

| Issue | Engines Affected | Severity |
|-------|-----------------|----------|
| Empty params blocks (3–12%) | ALL 7 | P0 |
| Wrong param prefix (stale engine names: `felix_`, `oscar_`, `oddo_`) | OddfeliX, OddOscar, Obese | P0 |
| Auto-generated bulk preset names (5–6 word descriptors) | ALL 7 | P1 |
| `coupling: null` instead of `{"pairs": []}` | ALL 7 | P3 |
| `sonic_dna` duplicate of `dna` | OddOscar, Oblong, Obese, Overdub | P3 |
| Version "1.0" vs "1.0.0" | ALL 7 | P3 |
| DNA/mood cross-validation (3–30% fail rate) | ALL 7, Odyssey worst | P1 |
| Entangled solo presets critically thin | OddOscar=3, Odyssey=3, Obese=3, Overbite=4 | P2 |
| Family mood thin | Oblong=4, Obese=2, Odyssey=7, OddfeliX=11 | P2 |
| Submerged=0 | Overdub, Oblong | P1 |

### DNA Cross-Validation Failure Rates

| Engine | Failures | Rate | Primary Cause |
|--------|----------|------|--------------|
| Odyssey | 118 | 30.6% | Bass presets with low density in Foundation; Atmosphere presets with space < 0.5 |
| Oblong | 74 | 23.1% | Atmosphere presets with close/intimate space < 0.5; static textures in Flux |
| OddfeliX | 58 | 16.5% | Atmosphere space < 0.5; Foundation density < 0.4 |
| Overbite | 26 | 16.0% | Static bass textures in Flux; Atmosphere space < 0.5 |
| Overdub | 13 | 6.3% | Low-density Foundation presets; static Flux presets |
| OddOscar | 9 | 3.8% | Atmosphere space < 0.5 |
| Obese | 8 | 5.3% | Minimal — best cross-validation score |

**Note:** Many Odyssey and Oblong failures may reflect a calibration issue with the DNA/mood thresholds rather than authoring errors. Engines with character-led rather than energy-led sounds (drifting bass, intimate pads, static textures) systematically conflict with Atmosphere/Flux threshold rules. Recommend: discuss whether Foundation density and Atmosphere space thresholds should be engine-specific or globally enforced.

### Preset Volume Assessment

| Engine | Solo Presets | Fleet Target (~150+) | Status |
|--------|-------------|---------------------|--------|
| Odyssey | 386 | 150+ | Exceeds |
| OddfeliX | 352 | 150+ | Exceeds |
| Oblong | 321 | 150+ | Exceeds |
| OddOscar | 235 | 150+ | Exceeds |
| Overdub | 205 | 150+ | Exceeds |
| Overbite | 162 | 150+ | Meets |
| Obese | 151 | 150+ | Meets (barely) |

All 7 engines meet or exceed the 150+ fleet target. Volume is not a concern.

---

## PRIORITY ACTION LIST

### P0 — Blocks Shipping

1. **Empty params blocks** — 16–25 presets per engine produce default/silent patches. Identify and either restore params from source instruments or remove preset. Total: ~113 presets across 7 engines.

2. **Wrong param prefix presets** — `felix_`, `oscar_`, `oddo_`, `ob_` keys will not load correctly. Must be migrated to correct prefix or removed. Total: ~63 presets across 5 engines.

3. **Overbite param key inconsistency** — two generations of param schema coexist (`poss_fur` vs `poss_furAmount`). Engine may silently ignore one set of keys. Needs normalization audit.

### P1 — Doctrine Violations

4. **OddfeliX macroLabels** — 114 presets (32%) use mixed-case `["Snap+Morph", "Bloom", "Coupling", "Space"]`. Mass-update to uppercase: `["SNAP+MORPH", "BLOOM", "COUPLING", "SPACE"]` (or preferred labels).

5. **Overbite B008 macro conflict** — 58 presets with 5 macroLabels. Owner decision required: (a) validate 5-macro as Overbite exception, update audit rule; or (b) demote PLAY DEAD, revert to 4. Do not mass-fix until decided.

6. **DNA/mood cross-validation failures** — 118 Odyssey, 74 Oblong, 58 OddfeliX. Recommend: (a) calibration review — determine if thresholds are too strict for certain engine characters; (b) fix obvious misassignments (movement=0.0 in Flux; space=0.0 in Atmosphere).

7. **Missing Submerged mood** — Overdub and Oblong have zero Submerged solo presets. Create at least 5 per engine to complete mood coverage.

8. **Critically thin Entangled** — OddOscar=3, Odyssey=3, Obese=3, Overbite=4 solo Entangled presets. Minimum 10 per engine recommended for mood depth.

9. **Odyssey tag typo** — `"dyssey"` tag in at least one preset (likely batch-generated). Audit and fix all misspelled tags.

### P2 — Quality Concerns

10. **Naming violations** — 6–33 per engine. Focus: remove 1-word names (single-word names like "Finray", "Ascension", "Antenna" are not meaningful in context) and rename 5-word auto-descriptors.

11. **Family mood thin** — Oblong=4, Obese=2 are severely thin. Target 10+ per engine.

12. **OddOscar legacy coupling schema** — At least 1 preset uses `{"intensity":..., "sourceEngine":..., "targetEngine":..., "type":...}` format — incompatible with current `pairs` format.

13. **Odyssey custom JSON extensions** — `macroJourney`, `climaxThreshold`, `bloomTime`, `modRouting` fields are undocumented schema extensions. Should be added to spec or placed in an `engineExtensions` key.

### P3 — Polish Items

14. **Version "1.0" normalization** — 9–22 presets per engine use `"1.0"` instead of `"1.0.0"`. Mass-update.

15. **`coupling: null` normalization** — Replace with `{"pairs": []}` for consistency. Affects all 7 engines.

16. **`sonic_dna` duplicate key** — Remove `sonic_dna` from ~30% of OddOscar/Oblong/Obese/Overdub presets where it duplicates `dna`.

17. **macros dict/macroLabels alignment** — Several Overbite presets have `macros` dict with `CHARACTER/MOVEMENT/COUPLING/SPACE` keys while `macroLabels` are `BELLY/BITE/SCURRY/TRASH` — these must align.

---

*Report generated 2026-03-21. Rodrigo audit assistant.*
