# Batch 5 Fleet Audit — Orca, Octopus, Overlap, Outwit, Ostinato, OpenSky, OceanDeep, Ouie

**Date:** 2026-03-21
**Auditor:** Rodrigo's audit assistant
**Scope:** 8 engines, all mood folders, deep sample of 2–3 presets per engine
**Total presets audited across all 8 engines: 3,150**

---

## Notes on Non-Standard Mood Folders

The following non-canonical mood folders exist in `Presets/XOceanus/` and contain presets for some of these engines:

| Folder | Total files | Engines with presence |
|--------|------------|----------------------|
| Ethereal (13) | Orca (2), OpenSky (2) | Non-standard mood — presets counted in totals above |
| Deep (16) | Orca (1), Octopus (1), Overlap (1) | Non-standard — ambiguous |
| Kinetic (32) | Outwit (3), Ostinato (4), Ouie (3) | Non-standard mood |
| Crystalline (15) | None of the 8 engines | N/A |
| Luminous (7) | None of the 8 engines | N/A |
| Organic (19) | None of the 8 engines | N/A |

**P1 Issue (fleet-wide):** Six non-standard mood folders exist outside the 8 canonical moods. Presets landing in Ethereal, Deep, and Kinetic for the engines in this batch should be reviewed and relocated to canonical moods, or the non-standard folders need to be formally ratified as canonical.

---

## ENGINE: Orca

**Preset Count:** 436
**Mood Distribution:** Foundation=44, Atmosphere=45, Entangled=197, Prism=36, Flux=43, Aether=27, Family=32, Submerged=9
**Non-Standard Moods:** Ethereal=2, Deep=1
**Missing Moods:** Submerged is thin (9). All 8 standard moods represented.

**DNA Range:**

| Dimension | Min | Max | Range | Flag |
|-----------|-----|-----|-------|------|
| brightness | 0.014 | 0.996 | 0.982 | OK |
| warmth | 0.000 | 0.990 | 0.990 | OK |
| movement | 0.023 | 1.000 | 0.977 | OK |
| density | 0.010 | 1.000 | 0.990 | OK |
| space | 0.000 | 1.000 | 1.000 | OK |
| aggression | 0.000 | 1.000 | 1.000 | OK |

**DNA Gaps:** None. All dimensions well-covered.

---

### SAMPLE AUDIT (3 presets):

**1. Orca Breach** — mood: Flux
- Missing fields: None
- Naming: PASS — "Orca Breach" (2 words, 11 chars, evocative)
- Param prefix: PASS — all keys use `orca_` prefix
- DNA accuracy: PASS — all 6 dims present, all in [0,1]
- Entangled coupling: N/A
- Issues: None

**2. Orca Hunt** — mood: Atmosphere
- Missing fields: None
- Naming: PASS — "Orca Hunt" (2 words, 9 chars)
- Param prefix: PASS — uses `orca_` prefix throughout
- DNA accuracy: PASS
- Issues: None. Well-formed, rich parameter set.

**3. Strange Core** — mood: Entangled
- Missing fields: `description`, `tempo`
- Naming: PASS — "Strange Core" (2 words, 12 chars)
- Param prefix: FAIL — uses `macro_character` / `macro_coupling` etc. instead of `orca_macroCharacter`. Keys read as `macro_*` without engine prefix.
- DNA accuracy: PASS
- Entangled coupling: PASS — 1 pair (ENVELOPE_FOLLOW), amount=0.841 > 0.1
- Issues: (a) duplicate DNA — both `"dna"` and `"sonic_dna"` fields present (b) missing `description` (c) param keys are `macro_character` instead of `orca_macroCharacter` — wrong namespace

---

**P0 Issues:**
- None (builds ship)

**P1 Issues:**
- 68/436 presets (16%) carry **both** `"dna"` and `"sonic_dna"` keys — stale field from migration. PresetManager must choose one canonical field; the other is dead weight and a future parse ambiguity. Affects primarily Entangled presets.
- Some Entangled-scope presets use legacy unprefixed param keys (e.g. `macro_character` instead of `orca_macroCharacter`). Count not fully measured but confirmed in sampled preset.
- 2 presets in non-canonical `Ethereal` folder, 1 in `Deep`. Relocate or ratify.

**P2 Issues:**
- 100/436 presets (23%) have empty or missing `description`. Submerged mood (9 total) is underpopulated.
- 44 presets use all-caps auto-generated-style names (10%) — these violate the 2-3 word evocative naming rule (e.g. "BRIGHT COLD VIOLENT DENSE F3").
- 37 presets have names with 4+ words (8.5%).
- 12 presets have single-word names (2.8%) — below the 2-word minimum.
- 2 duplicate names found: "5X DARK COLD KINETIC DENSE..." (×5) and "DARK COLD KINETIC VAST DENS..." (×2).

**P3 Issues:**
- Submerged at 9 presets is the weakest mood by far for this engine. Orca's mythology (deep ocean, apex predator) maps naturally to Submerged — low priority given overall strength of coverage.

---

## ENGINE: Octopus

**Preset Count:** 390
**Mood Distribution:** Foundation=30, Atmosphere=36, Entangled=168, Prism=37, Flux=42, Aether=35, Family=22, Submerged=19
**Non-Standard Moods:** Deep=1
**Missing Moods:** None (all 8 represented, all reasonably balanced)

**DNA Range:**

| Dimension | Min | Max | Range | Flag |
|-----------|-----|-----|-------|------|
| brightness | 0.001 | 0.999 | 0.998 | OK |
| warmth | 0.028 | 0.971 | 0.943 | OK |
| movement | 0.015 | 1.000 | 0.985 | OK |
| density | 0.020 | 0.980 | 0.960 | OK |
| space | 0.019 | 1.000 | 0.981 | OK |
| aggression | 0.000 | 0.990 | 0.990 | OK |

**DNA Gaps:** None. All dimensions fully covered.

---

### SAMPLE AUDIT (3 presets):

**1. Reef Grip** — mood: Foundation
- Missing fields: None
- Naming: PASS — "Reef Grip" (2 words, 9 chars, evocative)
- Param prefix: PASS — all keys use `octo_` prefix
- DNA accuracy: PASS
- Issues: None. Exemplary preset — full parameter set, strong description, correct macroLabels.

**2. Octopus Color Shift** — mood: Aether
- Missing fields: `tempo`
- Naming: BORDERLINE — "Octopus Color Shift" is 3 words and 19 chars (within limits), but leading the name with the engine name ("Octopus") is redundant and somewhat jargon-like. Consider "Chromatophore Shift" or "Color Cascade."
- Param prefix: FAIL — uses `octo_inkDepth`, `octo_lfoDepth`, `octo_lfoRate` but also `octo_macroInk` and `octo_macroStretch` — consistent `octo_` prefix across all. PASS on prefix. However **macroLabels are non-standard**: STRETCH, INK, COUPLING, SPACE — these differ from the fleet-standard CHARACTER, MOVEMENT, COUPLING, SPACE. This is allowed per engine identity but creates inconsistency with cross-engine coupling UI.
- DNA accuracy: PASS
- Issues: `tempo` field absent (minor — tempo=null is acceptable but field should be present per schema)

**3. Papillae Texture** — mood: Prism
- Missing fields: None
- Naming: PASS — "Papillae Texture" (2 words, 16 chars, scientifically evocative)
- Param prefix: PASS — full `octo_` prefix
- DNA accuracy: PASS
- Issues: None. Exemplary preset.

---

**P0 Issues:**
- None

**P1 Issues:**
- 56/390 presets (14%) carry **both** `"dna"` and `"sonic_dna"` keys — same migration residue as Orca. Resolve fleet-wide.
- 8/168 Entangled presets (5%) have no coupling pairs — lower rate than Orca, acceptable if intentionally single-engine pads.

**P2 Issues:**
- 94/390 presets (24%) have empty or missing `description`.
- 39 presets with auto-generated all-caps names (10%).
- 35 presets with 4+ word names (9%).
- 6 presets with single-word names (1.5%).
- 3 duplicate names: "Jet Propulsion" (×2), "Chromatophore Reverie" (×2), "5X DARK COLD KINETIC DENSE..." (×4).
- Inconsistent macroLabel sets: some presets use CHARACTER/MOVEMENT/COUPLING/SPACE, others use engine-specific labels (STRETCH/INK/COUPLING/SPACE). Engine-specific macros are intentional per identity but should be documented and consistent within preset groups.

**P3 Issues:**
- 1 preset in non-canonical `Deep` folder. Relocate.
- Family at 22 is low relative to other moods — Octopus's alien intelligence pairs well with many engines.

---

## ENGINE: Overlap

**Preset Count:** 522
**Mood Distribution:** Foundation=40, Atmosphere=47, Entangled=250, Prism=35, Flux=45, Aether=42, Family=40, Submerged=22
**Non-Standard Moods:** Deep=1 (via Entangled subdirectory counting)
**Missing Moods:** None. All 8 moods well-represented. Strongest total of the 8 engines.

**Note on Entangled structure:** Overlap has an additional subdirectory `Presets/XOceanus/Entangled/Overlap/` containing 40 presets. These are correctly mood-tagged as "Entangled" and counted in the 250 total. The subdirectory is non-standard — other engines do not use subdirectories. This creates file organization inconsistency.

**DNA Range:**

| Dimension | Min | Max | Range | Flag |
|-----------|-----|-----|-------|------|
| brightness | 0.000 | 0.990 | 0.990 | OK |
| warmth | 0.000 | 0.982 | 0.982 | OK |
| movement | 0.000 | 0.990 | 0.990 | OK |
| density | 0.000 | 1.000 | 1.000 | OK |
| space | 0.005 | 1.000 | 0.995 | OK |
| aggression | 0.000 | 0.990 | 0.990 | OK |

**DNA Gaps:** None. Excellent coverage.

---

### SAMPLE AUDIT (3 presets):

**1. Biolume Tangle** — mood: Entangled
- Missing fields: `tempo`
- Naming: PASS — "Biolume Tangle" (2 words, 14 chars, evocative neologism)
- Param prefix: PASS — all keys use `olap_` prefix
- DNA accuracy: PASS
- Entangled coupling: FAIL — `couplingIntensity: "None"` and `coupling.pairs: []`. Single-engine Overlap preset in Entangled mood.
- Issues: Solo-engine preset in Entangled mood is a doctrine concern — the Entangled mood implies cross-engine coupling.

**2. Amphichiral Beat** — mood: Entangled (in Entangled/Overlap/ subdirectory)
- Missing fields: None
- Naming: PASS — "Amphichiral Beat" (2 words, 16 chars, technically evocative)
- Param prefix: PASS — all keys use `olap_` prefix
- DNA accuracy: PASS
- Entangled coupling: FAIL — `couplingIntensity: "None"` and `coupling.pairs: []`. Single-engine.
- Issues: Same as above — Overlap uses the Entangled folder for its own internal "knot topology" presets without cross-engine coupling. This is a design decision conflict with the Entangled mood's intended meaning.

**3. Partial Room** — mood: Entangled
- Missing fields: `tempo`
- Naming: PASS — "Partial Room" (2 words, 12 chars)
- Param prefix: PASS — uses `olap_` prefix
- DNA accuracy: PASS
- Entangled coupling: PASS — 1 pair (SPECTRAL_CROSS to Orbital), amount=0.555
- Issues: Both `"dna"` and `"sonic_dna"` present — dual-field contamination.

---

**P0 Issues:**
- None

**P1 Issues:**
- **68/290 solo-engine Entangled presets (23%) have no coupling pairs** — this is the highest ratio of the batch. Overlap uses Entangled as its primary solo-engine mood (knot topology is "self-entangled"). This is a conceptual misuse of the Entangled mood bucket. Consider: (a) relocate solo Overlap topology presets to Foundation/Atmosphere/Prism, or (b) formally document that single-engine Overlap Entangled presets are intentional because the engine is topologically self-coupled.
- 153/522 presets (29%) carry **both** `"dna"` and `"sonic_dna"` keys — highest dual-DNA rate in the batch. Concentrated in Entangled presets generated early in the pipeline.
- Entangled subdirectory `Presets/XOceanus/Entangled/Overlap/` is non-standard. All other engines store flat. Consolidate or document exception.

**P2 Issues:**
- 85/522 presets (16%) have empty or missing `description` — lowest rate in the batch, acceptable.
- 39 presets with auto-generated all-caps names (7%).
- 34 presets with 4+ word names (6.5%).
- 9 presets with single-word names (1.7%).
- 5 duplicate names: "Topology Shift" (×2), "Pulse Web" (×2), "Deep Current" (×2), "BRIGHT COLD KINETIC DENSE..." (×2), "5X DARK COLD KINETIC DENSE..." (×4).

**P3 Issues:**
- Prism at 35 is relatively low given the engine's breadth (knot geometry maps naturally to prismatic exploration).

---

## ENGINE: Outwit

**Preset Count:** 538
**Mood Distribution:** Foundation=43, Atmosphere=49, Entangled=253, Prism=51, Flux=50, Aether=38, Family=40, Submerged=11
**Non-Standard Moods:** Kinetic=3
**Missing Moods:** Submerged is thin (11). All 8 standard moods represented.

**DNA Range:**

| Dimension | Min | Max | Range | Flag |
|-----------|-----|-----|-------|------|
| brightness | 0.000 | 1.000 | 1.000 | OK |
| warmth | 0.000 | 1.000 | 1.000 | OK |
| movement | 0.003 | 1.000 | 0.997 | OK |
| density | 0.000 | 1.000 | 1.000 | OK |
| space | 0.005 | 1.000 | 0.995 | OK |
| aggression | 0.000 | 1.000 | 1.000 | OK |

**DNA Gaps:** None. Full-spectrum coverage.

---

### SAMPLE AUDIT (3 presets):

**1. Still Water Column** — mood: Atmosphere
- Missing fields: None
- Naming: PASS — "Still Water Column" (3 words, 18 chars, narrative and evocative)
- Param prefix: PASS — all keys use `owit_` prefix
- DNA accuracy: PASS
- Issues: Engine-specific macroLabel "CHAOS" (vs MOVEMENT) — intentional identity-driven label. COUPLING and SPACE retained. Consistent with Outwit character.

**2. Outwit Deep Storm** — mood: Atmosphere
- Missing fields: None
- Naming: FAIL — "Outwit Deep Storm" contains the engine name as the first word. 3 words, 17 chars. Redundant engine prefix in name.
- Param prefix: PASS — all `owit_` prefix
- DNA accuracy: PASS
- Issues: Leading engine name in preset name ("Outwit Deep Storm") is avoidable — the engine context is already implied. Suggest "Deep Storm" or "Fractal Storm."

**3. Branch Rule** — mood: Entangled
- Missing fields: `tempo`
- Naming: PASS — "Branch Rule" (2 words, 11 chars)
- Param prefix: PASS — uses `owit_` prefix
- DNA accuracy: PASS
- Entangled coupling: PASS — 1 pair (PITCH_MOD to Origami), amount=0.803
- Issues: Both `"dna"` and `"sonic_dna"` present. Parameter set is sparse (14 params vs ~21 in non-Entangled presets) — Origami parameters not included in the preset despite being a listed engine partner.

---

**P0 Issues:**
- None

**P1 Issues:**
- 156/538 presets (29%) carry **both** `"dna"` and `"sonic_dna"` keys — tied highest with Overlap. Same migration issue.
- 12/253 Entangled presets (5%) have no coupling pairs — low rate, acceptable.
- 3 presets in non-canonical `Kinetic` folder. Relocate to Flux or Prism.

**P2 Issues:**
- 93/538 presets (17%) have empty or missing `description`.
- 35 presets with auto-generated all-caps names (6.5%).
- 33 presets with 4+ word names (6.1%).
- 2 presets with single-word names.
- 3 duplicate names: "Synapse Storm" (×2), "Eight Arms" (×2), "5X DARK COLD KINETIC DENSE..." (×6).
- Multiple presets use engine name as first word ("Outwit Deep Storm" style) — avoid redundancy.

**P3 Issues:**
- Submerged at 11 is low for an engine whose cellular automata DNA fits the dark, deep mood perfectly. Expand.

---

## ENGINE: Ostinato

**Preset Count:** 198
**Mood Distribution:** Foundation=36, Atmosphere=29, Entangled=26, Prism=30, Flux=25, Aether=32, Family=15, Submerged=1
**Non-Standard Moods:** Kinetic=4
**Missing Moods:** Submerged=1 (effectively absent). Family=15 is low.

**DNA Range:**

| Dimension | Min | Max | Range | Flag |
|-----------|-----|-----|-------|------|
| brightness | 0.100 | 0.800 | 0.700 | OK |
| warmth | 0.250 | 0.850 | 0.600 | OK |
| movement | 0.100 | 0.950 | 0.850 | OK |
| density | 0.100 | 0.950 | 0.850 | OK |
| space | 0.000 | 0.950 | 0.950 | OK |
| aggression | 0.000 | 0.920 | 0.920 | OK |

**DNA Gaps:** None technically (all > 0.4). However brightness [0.1–0.8] and warmth [0.25–0.85] are bounded on both ends — the engine never reaches brightness=0 (dark) or brightness=1.0 (ultra-bright). This reflects the acoustic drum mythology accurately but creates a perceptual ceiling.

---

### SAMPLE AUDIT (3 presets):

**1. Solo Djembe** — mood: Foundation
- Missing fields: None (note: `tier` and `guru_bin` fields are non-standard but valid extensions)
- Naming: PASS — "Solo Djembe" (2 words, 11 chars, clear and evocative)
- Param prefix: PASS — all `osti_` prefix
- DNA accuracy: PASS
- Issues: None. Exemplary Guru Bin preset — schema-complete, rich description with cultural context, correct macros (GATHER/FIRE/CIRCLE/SPACE).

**2. World Fusion** — mood: Atmosphere
- Missing fields: None
- Naming: PASS — "World Fusion" (2 words, 12 chars)
- Param prefix: PASS — uses `osti_` prefix. However parameter set is significantly smaller than Solo Djembe — uses `osti_instrument1`...`osti_instrument8` etc (different param naming from Solo Djembe's per-seat system). **This is a schema inconsistency: two presets for the same engine use different parameter key names** (e.g., `osti_seat1_instrument` vs `osti_instrument1`).
- DNA accuracy: PASS
- Issues: **Critical schema mismatch** — engine params appear to have two different parameter naming schemes. World Fusion uses `osti_instrument1`, `osti_tempo`, `osti_swing`; Solo Djembe uses `osti_seat1_instrument`, `osti_tempo`, `osti_swing`. The per-seat namespacing is different. One of these maps to a different version of the engine.

**3. Midnight Havana** — mood: Foundation
- Missing fields: None
- Naming: PASS — "Midnight Havana" (2 words, 15 chars)
- Param prefix: PASS — uses `osti_seat1_*` naming
- DNA accuracy: PASS
- Issues: `osti_swing` value is 33.0 (likely a percentage 0–100 range), while Solo Djembe has `osti_swing: 0.0` (0–1 range). **This is a parameter value-range inconsistency** — if swing is normalized [0,1], then 33.0 is out-of-range by 33×. If swing is [0,100], then Solo Djembe's 0.0 is at minimum but World Fusion's 0.33 is correct. Needs clarification.

---

**P0 Issues:**
- **Ostinato parameter key inconsistency**: `osti_seat1_instrument` vs `osti_instrument1` — two distinct parameter naming conventions exist across presets. PresetManager will silently fail to load one group or the other. This is a potential P0 data integrity issue depending on which parameter set the engine actually reads. **Verify OstinatiEngine.h parameter registration immediately.**

**P1 Issues:**
- `osti_swing` value range inconsistency (0.0 vs 33.0) — implies presets were authored against different versions of the parameter spec or different engines. One group will sound wrong.
- 22/26 Entangled presets (85%) are single-engine with no coupling pairs — `couplingIntensity: "None"` — misuse of Entangled mood or intentional circle-coupling design (B018). Given Blessing B018 exists for circular topology coupling, these may be intentional. However the Entangled placement is confusing without coupling pairs visible in the UI.
- 4 presets in non-canonical `Kinetic` folder.
- Submerged=1 effectively means the mood is absent for this engine.

**P2 Issues:**
- 0 presets with empty descriptions — best rate of the batch. Guru Bin quality fully applied.
- 2 duplicate names: "Solo Djembe" (×2), "Cave Ceremony" (×2).
- 2 presets with single-word names ("Accelerando," "Heartbeat").
- 1 preset with 4+ words ("Tongue Drum Garden II").
- Family=15 is low — Ostinato's CIRCLE mechanic makes it an excellent coupling partner.

**P3 Issues:**
- DNA brightness ceiling at 0.8 and warmth floor at 0.25 limit the perceived sonic range. Intentional per the acoustic drum character but worth noting.

---

## ENGINE: OpenSky

**Preset Count:** 377
**Mood Distribution:** Foundation=62, Atmosphere=68, Entangled=33, Prism=51, Flux=42, Aether=85, Family=34, Submerged=0
**Non-Standard Moods:** Ethereal=2
**Missing Moods:** **Submerged=0** — this is the only engine in this batch with a completely empty standard mood.

**DNA Range:**

| Dimension | Min | Max | Range | Flag |
|-----------|-----|-----|-------|------|
| brightness | 0.050 | 1.000 | 0.950 | OK |
| warmth | 0.050 | 0.800 | 0.750 | OK |
| movement | 0.000 | 0.920 | 0.920 | OK |
| density | 0.100 | 0.920 | 0.820 | OK |
| space | 0.000 | 1.000 | 1.000 | OK |
| aggression | 0.000 | 0.850 | 0.850 | OK |

**DNA Gaps:** None technically. However warmth [0.05–0.8] never reaches warmth=1.0 (fully warm/dark) — consistent with the engine's euphoric-shimmer identity (always in the brighter register). Aggression ceiling at 0.85 similarly reflects the engine's character.

---

### SAMPLE AUDIT (3 presets):

**1. Crystal Morning** — mood: Foundation
- Missing fields: None
- Naming: PASS — "Crystal Morning" (2 words, 15 chars, evocative)
- Param prefix: PASS — all `sky_` prefix
- DNA accuracy: PASS
- Issues: None. Exemplary Guru Bin preset with full mod matrix entries. macroLabels (RISE/WIDTH/GLOW/AIR) are engine-specific — consistent and evocative.

**2. Euphoria** — mood: Flux
- Missing fields: None
- Naming: FAIL — 1 word. "Euphoria" is evocative but violates the 2-word minimum. Suggest "Pure Euphoria" or "Mass Euphoria."
- Param prefix: PASS — all `sky_` prefix
- DNA accuracy: PASS
- Issues: 1-word name violates naming convention.

**3. Sunbreak** — mood: Prism
- Missing fields: None
- Naming: FAIL — 1 word. "Sunbreak" is evocative but violates 2-word minimum.
- Param prefix: PASS
- DNA accuracy: PASS
- Issues: 1-word name. Same as above.

**Additional note on Entangled:** 31/33 Entangled presets (94%) are single-engine with no coupling pairs. OpenSky's Entangled presets appear to be solo shimmer/modulation explorations that do not actually cross-couple with other engines. This is the most extreme example in the batch of mood-bucket misuse.

---

**P0 Issues:**
- **Submerged=0** — OpenSky has zero presets in the Submerged mood. Every engine must have representation across all 8 canonical moods for the UI mood filter to function correctly. This is a P0 gap: the Submerged slot is empty.

**P1 Issues:**
- **31/33 Entangled presets (94%) have no coupling pairs** — highest rate of Entangled misuse in the batch. OpenSky in the Entangled mood folder is functionally just "solo shimmer presets that don't cross-couple." Relocate to Aether or Atmosphere, or create actual Entangled presets with coupling partners.
- 43/377 single-word names (11.4%) — highest 1-word rate in the batch. Consistently naming single-concept states ("Euphoria," "Sunbreak," "Oscillation," "Waveform") violates the 2-word minimum.
- 2 presets in non-canonical `Ethereal` folder.

**P2 Issues:**
- 149/377 presets (40%) have empty or missing `description` — highest empty-description rate in the batch.
- 16 presets with 4+ word names (4.2%).
- 8 duplicate names: "Golden Hour" (×2), "Stratosphere" (×3), "Full Column High" (×2), "Contrail" (×2), "Crepuscular Ray" (×2) — highest duplicate count in the batch.
- `sequencer: null` field is missing from some presets (Guru Bin presets omit it; early presets include it) — schema consistency issue.

**P3 Issues:**
- Aether=85 is disproportionately large (22.5% of all OpenSky presets). The engine's shimmer identity naturally favors Aether, but the imbalance may overshadow other character dimensions.

---

## ENGINE: OceanDeep

**Preset Count:** 296
**Mood Distribution:** Foundation=77, Atmosphere=38, Entangled=24, Prism=44, Flux=33, Aether=27, Family=33, Submerged=20
**Non-Standard Moods:** None — cleanest folder hygiene of the batch.
**Missing Moods:** All 8 represented. Entangled=24 is the weakest.

**DNA Range:**

| Dimension | Min | Max | Range | Flag |
|-----------|-----|-----|-------|------|
| brightness | 0.000 | 0.600 | 0.600 | **RANGE CONCERN** |
| warmth | 0.200 | 0.950 | 0.750 | OK |
| movement | 0.000 | 0.920 | 0.920 | OK |
| density | 0.100 | 1.000 | 0.900 | OK |
| space | 0.000 | 1.000 | 1.000 | OK |
| aggression | 0.000 | 0.920 | 0.920 | OK |

**DNA Gaps:** brightness [0.0–0.6] — range=0.600. Technically above 0.4 threshold but the ceiling at 0.6 is architecturally intentional — Blessing B031 ("Darkness Filter Ceiling") constrains the engine to 50–800 Hz, meaning brightness above 0.6 would misrepresent the actual sound. This is correct by design, not a gap.

---

### SAMPLE AUDIT (3 presets):

**1. Hadal Drift** — mood: Aether
- Missing fields: `description` is present but empty string.
- Naming: PASS — "Hadal Drift" (2 words, 11 chars, scientifically evocative)
- Param prefix: PASS — all `deep_` prefix
- DNA accuracy: PASS
- Entangled coupling: N/A (Aether)
- Issues: Empty `description` field — present but blank. The preset has a `macros` block with live values (PRESSURE=0.68, CREATURE=0.1, etc.) which is good.

**2. Hammerhead x Trench** — mood: Family (Ouie + OceanDeep coupling)
- Missing fields: None. However no `schema_version` field. No `author` field. No `sequencer` field.
- Naming: PASS — "Hammerhead x Trench" (3 words, 19 chars)
- Param prefix: PASS — Ouie uses `ouie_` prefix. OceanDeep parameters are NOT present in the `parameters` block — only `Ouie` params are defined.
- DNA accuracy: PASS
- Family coupling: PASS — 1 pair (AudioToFM, Ouie→OceanDeep, 0.5)
- Issues: (a) Missing `schema_version`, `author`, `sequencer` fields (b) `parameters.OceanDeep` block absent — multi-engine preset should include parameters for all listed engines.

**3. Reef x OceanDeep** — mood: Family (Obrix + OceanDeep)
- Missing fields: `schema_version`, `sequencer` absent
- Naming: PASS — "Reef x OceanDeep" contains engine name but the "Reef x" prefix connotes the coupling relationship, acceptable for Family presets.
- Param prefix: PASS — Obrix uses `obrix_` prefix. OceanDeep params absent again.
- DNA accuracy: PASS
- Issues: Same as above — OceanDeep parameters not specified in multi-engine Family presets.

---

**P0 Issues:**
- None

**P1 Issues:**
- Multiple Family presets (confirmed in 2 of 3 sampled) are **missing parameters for OceanDeep** in the `parameters` block — only the coupling partner's params are provided. This means OceanDeep will load with default/init parameters in these contexts. If Family presets intend to configure both engines, the OceanDeep params must be specified.
- Family presets missing `schema_version` and `author` fields — schema compliance failure.

**P2 Issues:**
- 87/296 presets (29%) have empty or missing `description` — concentrated in the large Foundation batch (77 presets).
- Foundation=77 is disproportionate (26% of all OceanDeep presets). The engine's sub-bass mythology lends itself to Foundation, but this concentration leaves other moods underserved.
- 2 presets with single-word names ("Bioluminescent," "Bedrock").
- 1 preset with 4+ word names ("Sky x Full Column").
- 6 duplicate names: "Depth Charge" (×2), "Flash Fauna" (×2), "Pressure Drop" (×2), "Cave Resonance" (×2), "Dark Thermocline" (×2).
- `brightness` DNA range ceiling at 0.6 is architecturally correct (B031) but means any cross-engine DNA comparison will show OceanDeep as darker than other engines — correct, but document this explicitly so future preset authors don't try to raise brightness past 0.6.

**P3 Issues:**
- Entangled=24 is the lowest count in this engine's distribution. OceanDeep's pressure/depth mythology pairs well with OBESE, OVERDUB, OUROBOROS. Expand.

---

## ENGINE: Ouie

**Preset Count:** 393
**Mood Distribution:** Foundation=74, Atmosphere=58, Entangled=44, Prism=57, Flux=59, Aether=59, Family=36, Submerged=3
**Non-Standard Moods:** Kinetic=3
**Missing Moods:** Submerged=3 (effectively absent). All others well-populated.

**DNA Range:**

| Dimension | Min | Max | Range | Flag |
|-----------|-----|-----|-------|------|
| brightness | 0.100 | 0.800 | 0.700 | OK |
| warmth | 0.200 | 0.850 | 0.650 | OK |
| movement | 0.100 | 0.880 | 0.780 | OK |
| density | 0.150 | 0.880 | 0.730 | OK |
| space | 0.000 | 0.960 | 0.960 | OK |
| aggression | 0.000 | 0.950 | 0.950 | OK |

**DNA Gaps:** None technically. However brightness [0.1–0.8] and warmth [0.2–0.85] are bounded — the engine's duophonic hammerhead identity consistently avoids the ultra-dark and ultra-bright registers. This reflects character, not a gap.

---

### SAMPLE AUDIT (3 presets):

**1. The Bridge** — mood: Aether
- Missing fields: None
- Naming: PASS — "The Bridge" (2 words, 10 chars, metaphorical)
- Param prefix: PASS — all `ouie_` prefix
- DNA accuracy: PASS
- Issues: None. Exemplary Guru Bin preset. Full parameter set, rich description with mythology context, engine-specific macros (HAMMER/AMPULLAE/CARTILAGE/CURRENT) consistent across Ouie presets, `macros` block with live values.

**2. Hammerhead x Trench** — mood: Family (Ouie + OceanDeep)
(Audited above under OceanDeep — identical file, shared assessment)
- Issues from Ouie perspective: `schema_version`, `author`, `sequencer` absent. Missing OceanDeep params in parameters block. Coupling PASS.

**3. Outwit Deep Storm** — mood: Atmosphere (Outwit engine)
Note: This preset was sampled under Outwit. For Ouie, sampling the third preset from Aether instead:

**3. Oscar Hates Felix** — mood: Aether
- Missing fields: None
- Naming: PASS — "Oscar Hates Felix" (3 words, 17 chars, narrative and character-based — feliX-Oscar mythology)
- Param prefix: PASS — all `ouie_` prefix
- DNA accuracy: PASS — `ouie_macroHammer: -0.8` correctly represents STRIFE polarity; aggression=0.85 accurately reflects the aggressive character
- Issues: None. Exemplary Guru Bin preset — full parameter set, rich description with mythology, `macros` block with live values (HAMMER=-0.8 is correct use of bipolar macro).

---

**P0 Issues:**
- None

**P1 Issues:**
- 3 presets in non-canonical `Kinetic` folder. Relocate.
- Submerged=3 is effectively absent. Ouie's hammerhead mythology (deep water, pressure-adapted hunting) maps directly to Submerged. This should be expanded.
- Family presets (specifically "Hammerhead x Trench" shared with OceanDeep) are missing `schema_version`, `author`, `sequencer` fields and OceanDeep parameters in the `parameters` block. May affect all Family presets for Ouie — audit the full Family set.

**P2 Issues:**
- 0 presets with empty descriptions — tied best with Ostinato. Guru Bin quality maintained.
- 16 presets with single-word names (4%) — higher than Orca and Octopus. Common offenders: "Breaching," "Circling," "Riptide" — all 1-word motion verbs that read well but violate the 2-word minimum.
- 7 duplicate names: "Hammer Sweep" (×2), "Lateral Line" (×2), "Migration Path" (×2), "Open Water" (×2), "Harmonic Lock" (×2) — highest duplicate count among non-multi-engine engines.
- 2 presets with 4+ word names ("Sky x Duophonic Sky," "Left Eye Right Eye").

**P3 Issues:**
- Aether=59 is disproportionately large relative to Foundation=74, Atmosphere=58, Flux=59. Ouie's duophonic character expresses naturally in Aether (long tones, HAMMER interaction as ambient texture) but other moods should not be starved.
- DNA warmth [0.2–0.85] and brightness [0.1–0.8] ceilings are identity-consistent but narrow. A few extreme presets pushing toward warmth=0.1 (very cold STRIFE) or brightness=0.9 (overtone-saturated algorithms) would expand the perceived range.

---

## CROSS-ENGINE SUMMARY TABLE

| Engine | Total | Fnd | Atm | Ent | Prism | Flux | Aether | Family | Sub | Gaps |
|--------|-------|-----|-----|-----|-------|------|--------|--------|-----|------|
| Orca | 436 | 44 | 45 | 197 | 36 | 43 | 27 | 32 | 9 | Sub thin |
| Octopus | 390 | 30 | 36 | 168 | 37 | 42 | 35 | 22 | 19 | None |
| Overlap | 522 | 40 | 47 | 250 | 35 | 45 | 42 | 40 | 22 | None |
| Outwit | 538 | 43 | 49 | 253 | 51 | 50 | 38 | 40 | 11 | Sub thin |
| Ostinato | 198 | 36 | 29 | 26 | 30 | 25 | 32 | 15 | 1 | **Sub=1 P0** |
| OpenSky | 377 | 62 | 68 | 33 | 51 | 42 | 85 | 34 | 0 | **Sub=0 P0** |
| OceanDeep | 296 | 77 | 38 | 24 | 44 | 33 | 27 | 33 | 20 | Ent thin |
| Ouie | 393 | 74 | 58 | 44 | 57 | 59 | 59 | 36 | 3 | Sub thin |

---

## FLEET-WIDE ISSUE REGISTER

### P0 (Blocks Shipping)

| ID | Engine | Issue | Action |
|----|--------|-------|--------|
| P0-01 | OpenSky | Submerged=0 — no presets in this canonical mood | Create minimum 8 Submerged presets for OpenSky |
| P0-02 | Ostinato | `osti_seat1_instrument` vs `osti_instrument1` — two incompatible parameter naming schemes across presets | Audit OstinatoEngine.h param registration; determine canonical names; migrate all presets to one scheme |
| P0-03 | Ostinato | `osti_swing` value inconsistency (0.0 vs 33.0) — possible out-of-range values | Confirm swing param range [0,1] or [0,100] and normalize all presets |

### P1 (Doctrine Violations)

| ID | Engine(s) | Issue | Count | Action |
|----|-----------|-------|-------|--------|
| P1-01 | Orca, Octopus, Overlap, Outwit | Both `dna` and `sonic_dna` fields present | 433 presets (14–29%) | Fleet-wide migration: strip `sonic_dna` from all presets that have `dna` |
| P1-02 | Overlap | 68/290 solo-engine Entangled presets with no coupling pairs (23%) | 68 presets | Either relocate solo topology presets to non-Entangled moods, or formally document single-engine Entangled as intentional per Overlap's knot-topology identity |
| P1-03 | OpenSky | 31/33 Entangled presets (94%) are single-engine, no coupling pairs | 31 presets | Relocate to Aether/Atmosphere or create actual coupling-based Entangled presets |
| P1-04 | Ostinato | 22/26 Entangled presets (85%) are single-engine, no coupling pairs | 22 presets | Clarify: is B018 circle-coupling intentionally expressed in Entangled? If yes, document it. If no, relocate. |
| P1-05 | OceanDeep, Ouie | Multi-engine Family presets missing `schema_version`, `author`, `sequencer` and partner engine parameters | Multiple | Add missing fields; populate OceanDeep params in cross-engine Family presets |
| P1-06 | Multiple | Presets in non-standard mood folders (Ethereal, Deep, Kinetic) | ~15 presets | Relocate to canonical moods or formally ratify the non-standard folders |
| P1-07 | Orca (Entangled) | Some Entangled presets use legacy `macro_character` (no engine prefix) instead of `orca_macroCharacter` | Unknown — requires full audit | Migrate legacy param keys to prefixed names |

### P2 (Quality Concerns)

| ID | Engine(s) | Issue | Count | Action |
|----|-----------|-------|-------|--------|
| P2-01 | OpenSky | 40% of presets have empty description | 149/377 | Write descriptions for high-priority Aether/Prism group first |
| P2-02 | Orca, Octopus, OceanDeep | 23–29% empty descriptions | ~281 presets | Batch description-writing pass |
| P2-03 | Orca, Octopus, Overlap, Outwit | 6–10% of presets have auto-generated all-caps names violating 2-word naming rule | ~157 presets | Rename all-caps multi-word names to evocative 2-3 word names |
| P2-04 | OpenSky, Ouie | 11% and 4% single-word names respectively | ~59 presets | Add second word to single-word names |
| P2-05 | OpenSky, OceanDeep, Ouie | Multiple duplicate preset names | 23 pairs total | Deduplicate across all 8 engines |
| P2-06 | OceanDeep | Foundation=77 (26%) imbalance — too concentrated | 77/296 | Thin Foundation duplicates; expand Entangled |
| P2-07 | OpenSky | Aether=85 (22.5%) imbalance | 85/377 | Thin Aether duplicates; create Submerged presets |
| P2-08 | Ostinato | `macroLabels` inconsistency across Ostinato presets (SWING vs FIRE for macro 2) | Unknown | Standardize macroLabel set for each engine (one canonical set per engine) |

### P3 (Polish)

| ID | Engine(s) | Issue | Action |
|----|-----------|-------|--------|
| P3-01 | Orca, Outwit, Ouie | Submerged thin (9, 11, 3 presets) | Expand Submerged for all three engines |
| P3-02 | Ostinato, Ouie | Family count low (15, 36) | Expand Family presets — both engines couple well with other engines |
| P3-03 | OceanDeep | brightness DNA intentionally capped at 0.6 per B031 | Document this formally in CLAUDE.md or preset-auditor skill so future authors don't override |
| P3-04 | Multiple | `sequencer: null` field present in some presets, absent in others | Standardize: include `sequencer: null` in all presets that omit it |
| P3-05 | Orca, Octopus, Outwit | Multiple presets use engine name as first word (e.g. "Orca Breach," "Outwit Deep Storm") | Not a hard violation but leads to redundancy — encourage dropping engine name from preset name |

---

## RECOMMENDATIONS FOR NEXT SPRINT

**Immediate (before release):**
1. Fix P0-01: Create 8+ OpenSky Submerged presets
2. Fix P0-02/P0-03: Audit OstinatoEngine.h and normalize all Ostinato preset param keys and swing values
3. Fix P1-01: Fleet-wide `sonic_dna` field removal script (433 presets, Orca/Octopus/Overlap/Outwit)
4. Fix P1-05: Add missing fields and partner params to OceanDeep+Ouie Family presets

**Short-term (V1.1):**
5. Resolve P1-02/P1-03/P1-04: Document or relocate solo-engine Entangled presets
6. Batch description writing for OpenSky (149 empty) and Orca/Octopus/OceanDeep
7. Rename auto-generated all-caps preset names across the 4 affected engines

**Backlog:**
8. Expand Submerged for Orca, Outwit, Ouie, Ostinato
9. Expand OceanDeep Entangled and Ostinato Family
10. Formalize non-standard mood folder policy (Ethereal, Deep, Kinetic, Crystalline, Luminous, Organic)

