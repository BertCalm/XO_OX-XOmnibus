# Fleet Preset Audit — Batch 6
**Engines:** Obrix, Orbweave, Overtone, Organism, Oxbow, Oware, Optic
**Date:** 2026-03-21
**Auditor:** Rodrigo's Audit Assistant

---

## Audit Methodology

- Step 1: Full preset count per mood via JSON `engines` array match across all 14 mood folders
- Step 2: Deep field audit on 3 sampled presets per engine (different moods)
- Step 3: DNA range computed from all presets using Python JSON parsing
- Step 4: Systematic checks via Python scripts: required fields, naming, macroLabels, param prefixes, Entangled coupling pairs, DNA bounds

**Standard moods:** Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged
**Non-standard moods detected in repo:** Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic

---

## OPTIC — Exemption Notice

**OPTIC is a VISUAL engine (Blessing B005: Zero-Audio Identity).** It is intentionally exempt from:
- **D001** (Velocity Must Shape Timbre) — no audio output
- **D006** (Expression Input Is Not Optional) — confirmed exempt in CLAUDE.md

This exemption is flagged below but does NOT count as a failure in this audit. All Optic parameter blocks being empty (`{}`) is correct and expected behavior.

---

## ENGINE: Obrix

**Preset Count:** 406 (standard moods only)
**Mood Distribution:**
- Foundation=67, Atmosphere=55, Entangled=30, Prism=64, Flux=73, Aether=61, Family=23, Submerged=33
- Non-standard: 0

**Missing Moods:** None

**DNA Range:**
- brightness: 0.030–0.880 (range=0.850)
- warmth: 0.100–0.900 (range=0.800)
- movement: 0.000–0.950 (range=0.950)
- density: 0.100–0.950 (range=0.850)
- space: 0.000–1.000 (range=1.000)
- aggression: 0.000–0.900 (range=0.900)

**DNA Gaps:** None — all dimensions > 0.4 range. Full coverage.

---

### SAMPLE AUDIT (3 presets)

**1. Partch Ground** — mood: Foundation
- Missing fields: None (all required fields present)
- Naming: PASS (2 words, 14 chars)
- Param prefix: PASS (`obrix_` throughout)
- DNA accuracy: PASS (all in [0,1], 6 dimensions)
- Issues:
  - Has `"sequencer": null` key — extra non-required field, not a violation
  - Has `macros` dict mirroring macroLabels — correct pattern

**2. Rust Grind** — mood: Flux
- Missing fields: `coupling` key is absent (no `"coupling": {...}` at top level)
- Naming: PASS (2 words, 10 chars)
- Param prefix: PASS (`obrix_` throughout)
- DNA accuracy: PASS
- Issues:
  - Missing top-level `coupling` key — schema requires it even if empty `{"pairs": []}`
  - This is systemic: **321/406 Obrix presets** are missing the `coupling` key

**3. Quantum Noise** — mood: Aether
- Missing fields: `coupling` key absent (same systemic issue)
- Naming: PASS (2 words, 13 chars)
- Param prefix: PASS (`obrix_` throughout)
- DNA accuracy: PASS
- Issues:
  - Missing top-level `coupling` key (systemic)
  - couplingIntensity set to "None" — field is present, just not the coupling block

---

**P0 Issues:** None

**P1 Issues:**
- **321/406 presets (79%) missing `coupling` key.** Schema requires `"coupling": {"pairs": []}` for non-coupled presets. Loader must handle absence gracefully but this is a doctrine violation (missing required field). Likely a generation artifact from an older build wave.
- **28/30 Entangled presets (93%) have no coupling pairs.** Presets in the Entangled folder must have at least one coupling pair with `amount > 0.1`. These are mislabeled as "Entangled" when they are effectively solo-engine presets.

**P2 Issues:**
- **6 naming violations:** Single-word names found: "Driftwood," "Hologram," "Mutualism," "Countercurrent," "Bioluminescence" + 1 more. Doctrine requires 2–3 evocative words.

**P3 Issues:**
- 1 preset uses `macro_character` prefix instead of `obrix_*` macro keys (Collision_Brick_Prophet.xometa — likely a multi-engine Entangled preset with generic macro fallback, low severity)

---

## ENGINE: Orbweave

**Preset Count:** 374 (standard moods) + 7 non-standard (Crystalline=1, Deep=2, Ethereal=1, Kinetic=1, Luminous=1, Organic=1)
**Total:** 381

**Mood Distribution (standard):**
- Foundation=62, Atmosphere=62, Entangled=55, Prism=52, Flux=50, Aether=41, Family=41, Submerged=4

**Missing Moods:** Submerged is very thin (4 presets)

**DNA Range:**
- brightness: 0.080–1.000 (range=0.920)
- warmth: 0.150–0.900 (range=0.750)
- movement: 0.000–0.984 (range=0.984)
- density: 0.121–0.920 (range=0.799)
- space: 0.100–0.994 (range=0.894)
- aggression: 0.000–0.720 (range=0.720)

**DNA Gaps:** None — all dimensions clear.

---

### SAMPLE AUDIT (3 presets)

**1. Orbweave Core Filament** — mood: Foundation
- Missing fields: `tempo`, `couplingIntensity`, `description`, `coupling` (all absent)
- Naming: FAIL — name is "Orbweave Core Filament" (3 words but contains engine name "Orbweave"). This makes it generic/branded rather than evocative. Not strictly a rule violation but a quality concern.
- Param prefix: PASS (`weave_` throughout)
- DNA accuracy: PASS (all 6 dimensions, all in [0,1])
- Issues:
  - Missing `tempo`, `couplingIntensity`, `description`, `coupling` — systemic for 147/374 presets

**2. Orbweave Canopy Haze** — mood: Atmosphere
- Missing fields: `tempo`, `couplingIntensity`, `description`, `coupling` (all absent)
- Naming: FAIL — name is "Orbweave Canopy Haze" (3 words but prefixed with engine name)
- Param prefix: PASS (`weave_` throughout)
- DNA accuracy: PASS
- Issues:
  - Same systemic missing fields as above
  - LFO depths are extremely low (0.005, 0.037) — could be D005 concern but rate floors need checking

**3. Orbweave Cross Braid** — mood: Entangled
- Missing fields: `tempo`, `couplingIntensity`, `description`, `coupling`
- Naming: PASS (2 words "Cross Braid" — dropping "Orbweave" prefix from the actual `name` field — confirmed OK)
- Param prefix: PASS (`weave_` throughout)
- DNA accuracy: PASS
- Issues:
  - Entangled preset with NO coupling pairs — mislabeled mood
  - Still missing 4 required fields (systemic)

---

**P0 Issues:** None

**P1 Issues:**
- **147/374 presets (39%) missing `coupling`, `couplingIntensity`, `description`, and `tempo`.** These are all absent together, indicating an older generation batch without those schema fields. The missing `description` field is the most impactful — users see empty descriptions in the UI.
- **49/55 Entangled presets (89%) have no coupling pairs.** Entangled mood requires active coupling. These presets must either be re-mooded (e.g. to Prism or Foundation) or have coupling pairs added.

**P2 Issues:**
- **25 naming violations** (most frequent engine in the batch): 4+ word names ("Orbweave Knot Theory Low Velocity" = 5 words/33 chars), single-word names ("Thermohaline"). Orbweave has the most naming violations of any engine in this batch.
- **7 presets in non-standard mood folders** (Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic) — these folders are not in the canonical 8-mood schema. Presets will not surface in standard mood filters.

**P3 Issues:**
- 4 presets use `macro_character` prefix (generic) in multi-engine Entangled presets
- Submerged mood is very thin (4 presets) — target should be 20+
- Many preset names include "Orbweave" engine name as prefix — while technically 2-3 words, they read as branded not evocative

---

## ENGINE: Overtone

**Preset Count:** 368 (standard moods) + 6 non-standard
**Total:** 374

**Mood Distribution (standard):**
- Foundation=68, Atmosphere=68, Entangled=34, Prism=57, Flux=51, Aether=50, Family=30, Submerged=4

**Missing Moods:** Submerged thin (4 presets)

**DNA Range:**
- brightness: 0.100–0.920 (range=0.820)
- warmth: 0.100–0.880 (range=0.780)
- movement: 0.000–0.970 (range=0.970)
- density: 0.050–0.880 (range=0.830)
- space: 0.000–0.950 (range=0.950)
- aggression: 0.000–0.520 (range=0.520)

**DNA Gaps:** aggression range=0.520 — passes 0.4 threshold but is the narrowest dimension. No presets push above 0.52 aggression. Overtone is inherently a spectral/tonal engine; this may be appropriate but the fleet max should be higher for cross-engine contrast.

---

### SAMPLE AUDIT (3 presets)

**1. Sub Nautilus** — mood: Foundation
- Missing fields: `macroLabels` absent (no array), `coupling` absent, `macros` dict absent, `tempo` absent
- Naming: PASS (2 words, 11 chars)
- Param prefix: PASS (`over_` throughout)
- DNA accuracy: PASS — but `space: 0.0` and `aggression: 0.0` are edge values; flat-zero DNA dimensions signal DNA was not thoughtfully authored
- Issues:
  - Missing `macroLabels` — this is the P1 issue affecting 176/368 presets
  - DNA uses zero-values for space and aggression — may be accurate for a sub patch but reduces search findability

**2. Fibonacci Dew** — mood: Atmosphere
- Missing fields: `couplingIntensity` absent, `description` absent, `macroLabels` absent
- Naming: PASS (2 words, 12 chars)
- Param prefix: PASS (`over_`)
- DNA accuracy: FAIL — preset has BOTH `dna` and `sonicDNA` keys with different values. `dna.brightness=0.44`, `sonicDNA.brightness=0.48`. The correct field is `dna`. The conflicting `sonicDNA` key is a data integrity bug.
- Issues:
  - Duplicate conflicting DNA keys (`dna` vs `sonicDNA`) — 176 presets affected
  - Missing `macroLabels`, `couplingIntensity`, `description`
  - `macros` dict uses CHARACTER/MOVEMENT/COUPLING/SPACE but these don't match the engine's own DEPTH/COLOR/COUPLING/SPACE labels — label inconsistency within the preset

**3. Acid Spectral** — mood: Prism
- Missing fields: `coupling` absent
- Naming: PASS (2 words, 12 chars)
- Param prefix: PASS (`over_`)
- DNA accuracy: PASS — but `space: 0.0` and `movement: 0.1` are very low for an acid squelch preset (arguably should be space=0.15+, movement=0.4+)
- Issues:
  - Missing `coupling` key
  - DNA values for `space` and `aggression` seem inconsistent with description ("acid-inspired spectral squelch" should have higher aggression than 0.35)

---

**P0 Issues:** None that block loading

**P1 Issues:**
- **176/368 presets (48%) are missing `macroLabels` entirely.** These presets will render with zero macro labels in the UI — a broken user experience. This is the single worst field-level issue in this audit.
- **176/368 presets (48%) have conflicting `dna` vs `sonicDNA` keys with different values.** The `sonicDNA` key is non-canonical. If any code reads `sonicDNA` instead of `dna`, it will get wrong values. The conflict also means DNA was edited in two places, now diverged.
- **29/34 Entangled presets (85%) have no coupling pairs.** Same systemic Entangled mood mislabeling issue as Orbweave/Obrix.

**P2 Issues:**
- **204/368 presets (55%) missing `tempo` field** — while not always meaningful, it is a required schema field
- **176/368 presets (48%) missing `couplingIntensity` and `description`** — same older generation batch
- `macros` dict in Fibonacci Dew uses CHARACTER labels but macroLabels field uses DEPTH/COLOR labels — schema inconsistency in the 176-preset batch
- aggression dimension capped at 0.52 across entire library — Overtone should have at least a few patches reaching 0.7+ for contrast

**P3 Issues:**
- 3 presets use `macro_character` prefix in multi-engine Entangled presets
- 1 naming violation (4-word name)
- Submerged thin (4 presets)

---

## ENGINE: Organism

**Preset Count:** 405 (standard moods) + 9 non-standard (Crystalline=2, Deep=1, Ethereal=2, Kinetic=1, Luminous=2, Organic=1)
**Total:** 414

**Mood Distribution (standard):**
- Foundation=67, Atmosphere=71, Entangled=46, Prism=59, Flux=69, Aether=45, Family=32, Submerged=7

**Missing Moods:** None; Submerged thin (7 presets)

**DNA Range:**
- brightness: 0.120–0.900 (range=0.780)
- warmth: 0.180–0.860 (range=0.680)
- movement: 0.050–0.980 (range=0.930)
- density: 0.150–0.880 (range=0.730)
- space: 0.000–0.980 (range=0.980)
- aggression: 0.000–0.950 (range=0.950)

**DNA Gaps:** None — strong coverage across all dimensions.

---

### SAMPLE AUDIT (3 presets)

**1. Rule 18 Sparse** — mood: Foundation
- Missing fields: `coupling` key absent, `tempo` absent
- Naming: PASS (3 words, 13 chars — "Rule 18 Sparse" is evocative of the generative identity)
- Param prefix: PASS (`org_` throughout)
- DNA accuracy: PASS (all 6 dimensions, valid range)
- Issues:
  - Missing `coupling` key (systemic)
  - Missing `tempo` (systemic)
  - macroLabels = RULE/SEED/COUPLING/MUTATE — correct engine-specific labels

**2. Limestone Cave** — mood: Atmosphere
- Missing fields: `coupling` absent, `tempo` absent
- Naming: PASS (2 words, 13 chars)
- Param prefix: PASS (`org_`)
- DNA accuracy: PASS
- Issues:
  - Missing `coupling`, `tempo`
  - `org_seed` is hardcoded to 42 for both presets sampled — seed variety may be absent across the library

**3. Aftertouch Chaos** — mood: Flux
- Missing fields: `coupling` absent, `tempo` absent
- Naming: PASS (2 words, 15 chars)
- Param prefix: PASS (`org_`)
- DNA accuracy: QUESTIONABLE — brightness=0.5, warmth=0.5, density=0.25, aggression=0.15 for a "Chaos" preset. Aggression should be 0.6+ for a patch described as chaos-driven.
- Issues:
  - DNA doesn't match character description
  - Missing `coupling`, `tempo`

---

**P0 Issues:** None

**P1 Issues:**
- **135/405 presets (33%) missing `coupling` key.** Same systemic issue as Obrix.
- **40/46 Entangled presets (87%) have no coupling pairs.** Entangled mood mislabeling.
- **210/405 presets (52%) use generic `CHARACTER/MOVEMENT/COUPLING/SPACE` macro labels** instead of the engine-specific `RULE/SEED/COUPLING/MUTATE`. The engine-specific labels are part of Organism's identity and doctrine. Using generic labels breaks the character.

**P2 Issues:**
- **6 naming violations:** Single-word names (Bioluminescent, Thermocline, Bioluminescence, Metastasis) — purely jargon words, no evocativeness; 1 four-word name (Rule 54 Maturation).
- **9 presets in non-standard mood folders** (Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic).
- `org_seed=42` appears to be the default across multiple presets (Limestone Cave, Rule 18 Sparse, Aftertouch Chaos all use 42) — seed diversity may be very low fleet-wide.

**P3 Issues:**
- 3 presets use `macro_character` wrong prefix in multi-engine Entangled presets
- Submerged is thin (7 presets)

---

## ENGINE: Oxbow

**Preset Count:** 135 (standard moods) + 26 non-standard (Crystalline=4, Deep=2, Ethereal=2, Kinetic=6, Luminous=4, Organic=8)
**Total:** 161

**Mood Distribution (standard):**
- Foundation=35, Atmosphere=24, Entangled=17, Prism=12, Flux=22, Aether=11, Family=3, Submerged=11

**Missing Moods:** None at zero, but Family (3) is dangerously thin; Aether (11) and Prism (12) are thin

**DNA Range:**
- brightness: 0.050–0.950 (range=0.900)
- warmth: 0.050–0.900 (range=0.850)
- movement: 0.050–0.980 (range=0.930)
- density: 0.120–0.850 (range=0.730)
- space: 0.100–1.000 (range=0.900)
- aggression: 0.000–0.800 (range=0.800)

**DNA Gaps:** None — solid coverage.

---

### SAMPLE AUDIT (3 presets)

**1. Vinyl Room** — mood: Foundation
- Missing fields: `tempo`
- Naming: PASS (2 words, 11 chars)
- Param prefix: PASS (`oxb_` throughout)
- DNA accuracy: PASS
- Issues:
  - macroLabels = ENTANGLE/EROSION/CONVERGE/CANTILEVER — this is Oxbow's custom engine-specific label set. Correct.
  - Missing `tempo` (minor, systemic)
  - No `macros` dict — but this preset predates the macros dict convention; not a blocker

**2. Transistor Dream** — mood: Atmosphere
- Missing fields: `tempo`
- Naming: PASS (2 words, 15 chars)
- Param prefix: PASS (`oxb_`)
- DNA accuracy: PASS
- Issues:
  - Missing `tempo` only (minor)
  - macroLabels use ENTANGLE/EROSION/CONVERGE/CANTILEVER — correct

**3. Hadal Pressure** — mood: Submerged (awakening tier)
- Missing fields: None — this is a fully-formed awakening preset
- Naming: PASS (2 words, 15 chars)
- Param prefix: PASS (`oxb_`)
- DNA accuracy: PASS (`space=0.98` for an infinite-decay reverb — accurate and excellent)
- Issues:
  - macroLabels = CHARACTER/MOVEMENT/COUPLING/SPACE — **inconsistent with the 124 presets that use ENTANGLE/EROSION/CONVERGE/CANTILEVER**. Awakening-tier Oxbow presets use different labels than the main library.
  - This inconsistency (10 presets with CHARACTER vs 124 with ENTANGLE) means players experience two different macro label schemes on the same engine.

---

**P0 Issues:** None

**P1 Issues:**
- **Macro label inconsistency: 124/135 presets use ENTANGLE/EROSION/CONVERGE/CANTILEVER; 10 presets use CHARACTER/MOVEMENT/COUPLING/SPACE; 1 uses a mixed set (ENTANGLE+CHARACTER hybrid in a coupled preset).** This is a doctrine problem — each engine should have one canonical macro label set. Recommend: standardize on ENTANGLE/EROSION/CONVERGE/CANTILEVER as primary; convert the 10 awakening presets.
- **16/17 Entangled presets (94%) have no coupling pairs.** Worst Entangled compliance ratio in this batch.

**P2 Issues:**
- **26 presets in non-standard mood folders** (Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic). Oxbow has the most non-standard mood spillover of any engine. 8 presets in "Organic" mood alone. These won't surface in the standard 8-mood browser.
- **130/161 presets (81%) missing `tempo`.** Very high rate.
- **3 naming violations:** Single-word names — "Chiasmus," "Clockwork," "Teardrop."
- Family mood critically thin (3 presets).

**P3 Issues:**
- Many non-standard mood presets have the correct `mood` field value set (e.g. `"mood": "Kinetic"`) — these are intentional expansions, not accidents, but they need to be tagged/discoverable.

---

## ENGINE: Oware

**Preset Count:** 125 (standard moods) + 5 non-standard (Kinetic=5)
**Total:** 130

**Mood Distribution (standard):**
- Foundation=27, Atmosphere=23, Entangled=18, Prism=16, Flux=13, Aether=12, Family=6, Submerged=10

**Missing Moods:** None at zero, but Aether (12), Prism (16), Flux (13) are thin for a new engine

**DNA Range:**
- brightness: 0.100–0.980 (range=0.880)
- warmth: 0.080–0.950 (range=0.870)
- movement: 0.100–0.950 (range=0.850)
- density: 0.180–0.880 (range=0.700)
- space: 0.150–1.000 (range=0.850)
- aggression: 0.020–0.700 (range=0.680)

**DNA Gaps:** None — good coverage for a newer engine.

---

### SAMPLE AUDIT (3 presets)

**1. Oware Vibraphone Slow Motor** — mood: Foundation (awakening tier)
- Missing fields: `couplingIntensity` absent, `macros` dict absent, `version` present, all other required fields present
- Naming: FAIL — "Oware Vibraphone Slow Motor" is 4 words and contains the engine name "Oware" as a prefix. The actual name in the `name` field is the full string including "Oware." This violates both the 2-3 word rule and the "evocative not jargon" principle ("Vibraphone" + "Slow Motor" is purely descriptive/technical).
- Param prefix: PASS (`owr_` throughout)
- DNA accuracy: PASS — also has duplicate `sonic_dna` key (same values, not conflicting)
- Issues:
  - Name is 4 words and technical/descriptive ("Vibraphone Slow Motor")
  - `couplingIntensity` absent

**2. Solstice Light** — mood: Atmosphere
- Missing fields: `tempo` absent
- Naming: PASS (2 words, 13 chars — evocative and seasonal)
- Param prefix: PASS (`owr_`)
- DNA accuracy: PASS — has duplicate `sonic_dna` key (identical values, not conflicting)
- Issues:
  - `tempo` missing (systemic)
  - Has `sonic_dna` duplicate key — non-canonical, should be removed

**3. Whale Chime** — mood: Entangled (Orca + Oware)
- Missing fields: `couplingIntensity` absent, `author` absent, `version` present
- Naming: PASS (2 words, 11 chars — evocative)
- Param prefix: PASS (`owr_` for Oware params, `orca_` for Orca params — both correct)
- DNA accuracy: PASS
- Issues:
  - Has coupling pair (Orca→Oware, LFOToPitch, amount=0.42) — CORRECT for Entangled mood, this is a well-formed coupled preset
  - Missing `couplingIntensity` (should be "Moderate" — the `macroLabels` even say COUPLING)
  - Missing `author` field

---

**P0 Issues:** None

**P1 Issues:**
- **7/18 Entangled presets (39%) have no coupling pairs.** Better than other engines but still a significant minority. Note: Oware has more properly coupled Entangled presets (11/18 = 61%) than any other engine in this batch — the Whale Chime pattern is being followed for majority of Entangled. Recommend fixing the remaining 7.
- **9 presets with single-word instrument names:** Xylophone, Kalimba, Celesta, Balafon, Handpan, Angklung, Vibraphone, Crotales, Glockenspiel. These are pure jargon labels with no evocativeness. A vibraphone patch should be named something like "Slow Motor," "Milt's Voice," "Bar Shimmer" — not "Vibraphone." CLAUDE.md explicitly bans jargon names.

**P2 Issues:**
- **110/130 presets (85%) missing `tempo`** — highest rate in the batch
- **13 naming violations total** (9 single-word jargon + 4 multi-word violations including the "Oware Vibraphone Slow Motor" pattern)
- **5 presets in Kinetic non-standard mood folder** — Oware percussion in a rhythmic context makes semantic sense but needs to be discoverable

**P3 Issues:**
- Several presets have a `sonic_dna` duplicate key alongside `dna` (values identical, not conflicting — cleanup needed)
- Some presets missing the `macros` dict (older generation)
- Family mood thin (6 presets)

---

## ENGINE: Optic

**[VISUAL ENGINE — EXEMPT from D001 and D006]**

**Preset Count:** 600 (standard moods — all 8 standard folders)
**Mood Distribution:**
- Foundation=54, Atmosphere=68, Entangled=206, Prism=104, Flux=99, Aether=32, Family=37, Submerged=0

**Non-standard moods:** None

**Missing Moods:** Submerged=0 — **ZERO presets in Submerged mood.** This is the only engine in this batch with a completely empty standard mood.

**DNA Range:**
- brightness: 0.020–1.000 (range=0.980)
- warmth: 0.000–1.000 (range=1.000)
- movement: 0.000–0.996 (range=0.996)
- density: 0.000–0.990 (range=0.990)
- space: 0.015–1.000 (range=0.985)
- aggression: 0.000–1.000 (range=1.000)

**DNA Gaps:** None — Optic has the widest DNA coverage in the batch (all dimensions near full range).

---

### SAMPLE AUDIT (3 presets)

**1. VAST STILL GROUND** — mood: Foundation (Ombre + Optic coupled)
- Missing fields: `tempo`, `description`, `couplingIntensity` absent
- Naming: FAIL — "VAST STILL GROUND" is 3 words but in ALL CAPS. Per doctrine, preset names should be title case or evocative mixed case — all caps reads as a system token, not a human-authored preset name.
- Param prefix: FAIL — Optic parameters use `macro_character`, `macro_coupling`, etc. (generic `macro_` prefix) instead of `optic_`. This is a **systemic issue: 243/600 Optic presets use wrong `macro_` prefix** for parameter keys. Also Ombre parameters use `macro_character` instead of `ombre_`.
- DNA accuracy: PASS — DNA has `brightness=0.05` (very dark, consistent with Ombre's shadow identity)
- Issues:
  - Wrong param prefix (systemic)
  - ALL CAPS name
  - Missing `tempo`, `description`, `couplingIntensity`

**2. Arctic Shard Near** — mood: Prism
- Missing fields: None — fully formed, all required fields present
- Naming: PASS (3 words, 16 chars — evocative)
- Param prefix: N/A — Optic parameter block is empty `{}` which is correct for a visual engine
- DNA accuracy: PASS — but has `sonic_dna` duplicate alongside `dna` (different key name convention)
- Issues:
  - `sonic_dna` duplicate key (should consolidate to `dna` only)
  - Optic `{}` empty params — EXPECTED/CORRECT

**3. Prism on the Table** — mood: Entangled (Osteria + Optic coupled)
- Missing fields: None — fully formed
- Naming: PASS (4 words — FAIL: "Prism on the Table" is 4 words)
- Param prefix: N/A — both Osteria and Optic have empty `{}` blocks
- DNA accuracy: PASS
- Issues:
  - 4-word name ("Prism on the Table")
  - Empty parameter blocks for both engines (correct for Optic; potentially concerning for Osteria — auditors should verify Osteria separately)
  - `amount: 0.6` on coupling pair — PASS (> 0.1 threshold)

---

**P0 Issues:**
- **Submerged mood: 0 presets.** Optic has no representation in Submerged. This is the only complete mood gap found in this batch for a standard mood folder.

**P1 Issues:**
- **243/600 presets (40.5%) use `macro_character`, `macro_movement`, etc. as parameter keys** instead of `optic_*`. This is a param prefix violation. If PresetManager.h enforces prefix validation, these presets will fail to load parameters correctly.
- **250/600 presets (42%) missing `couplingIntensity`** — systemic older generation batch
- **222/600 presets (37%) missing `description`** — empty description in UI
- **17/206 Entangled presets (8%) have no coupling pairs** — relatively good compared to other engines (86 is the wrong read — only 17 Entangled Optic presets lack coupling pairs, which is 8% of 206)

**P2 Issues:**
- **83 naming violations** (the most in the batch by far): Multiple 5-6 word coded names like "DARK COLD FURY FOU 01," "5X DARK COLD KINETIC DENSE VIOLENT FND 2" — these appear to be parameter-descriptor strings used as names rather than human-authored evocative names. This is a systematic generation artifact.
- **86 presets have both `dna` and `sonic_dna` keys** — non-canonical duplicate DNA field
- **9 presets missing `coupling` key** entirely

**P3 Issues:**
- ALL CAPS naming convention on a subset of system-generated presets reads as debug output, not end-user product
- Aether thin (32 presets) relative to the enormous Entangled count (206)

---

## CROSS-ENGINE SUMMARY

### Critical Systemic Issues

| Issue | Obrix | Orbweave | Overtone | Organism | Oxbow | Oware | Optic |
|-------|-------|----------|----------|----------|-------|-------|-------|
| Missing `coupling` key | 321/406 (79%) | 147/374 (39%) | 134/368 (36%) | 135/405 (33%) | — | — | 9/600 (1.5%) |
| Missing `description` | — | 147/374 (39%) | 176/368 (48%) | — | — | — | 222/600 (37%) |
| Missing `macroLabels` | — | — | 176/368 (48%) | — | — | — | — |
| Entangled without coupling | 28/30 (93%) | 49/55 (89%) | 29/34 (85%) | 40/46 (87%) | 16/17 (94%) | 7/18 (39%) | 17/206 (8%) |
| Wrong param prefix | 1 | 4 | 3 | 3 | 0 | 0 | 243/600 (40%) |
| DNA key conflict | — | — | 176/368 (48%) | — | — | — | 86/600 (14%) |
| Missing `tempo` | — | 164/374 (44%) | 204/368 (55%) | 18/405 (4%) | 130/161 (81%) | 110/130 (85%) | 306/600 (51%) |
| Naming violations | 6 | 25 | 1 | 6 | 3 | 13 | 83 |

### P0 Issues (blocks shipping)
1. **Optic: Submerged=0 presets** — a standard mood with zero representation.

### P1 Issues (doctrine violations)
1. **Fleet-wide Entangled coupling gap** — all engines except Optic and Oware have >80% of Entangled presets with no coupling pairs. These must be corrected or reclassified. This is the single highest-impact P1 across the batch.
2. **Overtone: 176 presets missing `macroLabels`** — broken UI for 48% of the library.
3. **Overtone: 176 presets have conflicting `dna` vs `sonicDNA` keys** — data integrity bug.
4. **Organism: 210/405 presets using generic CHARACTER/MOVEMENT macros** instead of engine-specific RULE/SEED/COUPLING/MUTATE — breaks engine identity.
5. **Oxbow: Macro label split** — 124 use ENTANGLE/EROSION/CONVERGE/CANTILEVER vs 10 use CHARACTER/MOVEMENT. Must pick one.
6. **Obrix: 321/406 (79%) missing `coupling` key** — schema violation at massive scale.
7. **Optic: 243/600 (40%) using `macro_character` wrong prefix** — param loading violation.
8. **Oware: 9 single-word jargon instrument names** — explicit doctrine violation (names must be evocative, not jargon).

### P2 Issues (quality concerns)
1. Orbweave has 25 naming violations — worst in the batch. Many names include the engine name as prefix.
2. Oxbow has 26 presets in non-standard mood folders — worst non-standard overflow.
3. Optic has 83 naming violations dominated by system-generated coded names.
4. All engines except Obrix and Optic have thin Submerged mood counts.
5. Overtone aggression capped at 0.52 — limited contrast range.

### P3 Issues (polish)
1. Multiple presets fleet-wide have `sonic_dna` or `sonicDNA` duplicate keys alongside canonical `dna`.
2. Organism `org_seed=42` default overuse — check seed diversity fleet-wide.
3. Many presets missing `macros` dict (separate from `macroLabels` array) in older generation batches.
4. Oxbow non-standard mood presets are semantically intentional but invisible to standard mood browser.

---

## Recommendations (Prioritized)

### Immediate (P0/P1):
1. **Run a script to add `"coupling": {"pairs": []}` to all presets missing the coupling key** — this is a simple JSON migration across Obrix (321), Orbweave (147), Overtone (134), Organism (135).
2. **Overtone batch fix**: (a) Add `macroLabels: ["DEPTH","COLOR","COUPLING","SPACE"]` to 176 presets. (b) Delete `sonicDNA` key from all 176 presets with conflicts, keeping canonical `dna` values.
3. **Organism batch fix**: Audit the 210 presets using generic macroLabels — determine if they have the correct macro parameters and update labels to RULE/SEED/COUPLING/MUTATE.
4. **Re-mood or add coupling to Entangled presets**: For all 7 engines, each Entangled preset must either gain a real coupling pair (amount > 0.1) or be reclassified to a solo mood.
5. **Optic Submerged**: Author 15–20 Submerged Optic presets.
6. **Fix Optic param prefix**: Migrate 243 presets from `macro_character` to `optic_macro*` or the correct `optic_` prefix pattern.
7. **Rename 9 Oware single-word jargon names** to evocative 2-word alternatives.

### Near-term (P2):
1. **Orbweave naming pass**: 25 violations — do a naming audit pass specifically for Orbweave.
2. **Oxbow macro label consolidation**: Pick ENTANGLE/EROSION/CONVERGE/CANTILEVER as canonical, update the 10 awakening presets.
3. **Optic naming pass**: The 83 violations from coded generation names (DARK_COLD_FURY etc.) need a full human-naming pass.
4. **Add `tempo: null` to all presets missing `tempo`** — simple null-fill migration.
5. **Add `description` to all presets missing it** — minimum 1-sentence description per preset.

### Long-term (P3):
1. Consolidate all `sonic_dna`/`sonicDNA` duplicate keys to `dna` fleet-wide.
2. Oxbow non-standard mood presets: decide whether to add Kinetic/Organic/etc. as official moods or migrate to nearest standard mood.
3. Organism seed diversity audit — check if `org_seed=42` is overused.
4. Overtone aggression expansion — commission 10–15 higher-aggression Overtone presets (target 0.7–0.85).
