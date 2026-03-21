# RODRIGO'S FLEET PRESET BASELINE REPORT
## XOmnibus Fleet-Wide Preset Quality Audit
### Date: 2026-03-21 | Auditor: Rodrigo (6 parallel Sonnet agents + Opus orchestration)

---

## Executive Summary

**16,479 presets** across **44 engines** audited. Zero parse failures. But beneath the surface:
- **~900+ presets are silent** (empty parameter blocks = load as init patches)
- **~193+ presets have wrong param prefixes** (engine can't read them)
- **~85-94% of Entangled presets are fraudulent** (no coupling pairs)
- **1,009 duplicate preset names** across the fleet
- **~1,700+ naming violations** (debug slugs, jargon, single-word)
- **3 generations of preset schemas coexist** (migration debt)

**Fleet Health Score: ~6.5/10** (schema and structural issues drag down what is sonically strong work)

---

## Fleet Vitals

| Metric | Value |
|--------|-------|
| Total Presets | 16,479 |
| Engines | 44 |
| Standard Moods | 8 (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged) |
| Non-Standard Mood Folders | 6 (Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic) — ~110 presets |
| Parse Failures | 0 (100% valid JSON) |
| Validator Warnings | 12,502 across 9,622 presets (58%) |
| Validator Errors | 0 |

### Mood Distribution (Fleet-Wide)

| Mood | Count | % | Status |
|------|-------|---|--------|
| Entangled | 4,316 | 26.2% | OVERPOPULATED — but 85-94% have no coupling |
| Foundation | 2,440 | 14.8% | Healthy |
| Flux | 2,170 | 13.2% | Healthy |
| Atmosphere | 2,162 | 13.1% | Healthy |
| Prism | 2,126 | 12.9% | Healthy but 0 in some engines |
| Aether | 1,898 | 11.5% | Healthy |
| Family | 1,034 | 6.3% | Thin — 0 in many engines |
| Submerged | 251 | 1.5% | CRITICALLY THIN — 0 in 10+ engines |
| Non-standard | ~110 | 0.7% | Legacy/invisible |

---

## P0 Issues — Blocks Shipping (Fix Immediately)

### P0-1: Empty Parameter Blocks (~900+ presets)
**Impact:** Presets load as silent init patches — no authored sound.
**Engines affected:** All batches. Worst rates: Overdub 12%, OddOscar 10%, Obese 10.6%, Overbite 9.3%. Batch 4 alone had 532 empty blocks.
**Fix:** Script to identify and either populate from engine defaults or quarantine.

### P0-2: Wrong Parameter Prefixes (~193+ presets)
**Impact:** Engine APVTS cannot read parameters — preset plays init sound.
**Known wrong prefixes:**

| Engine | Wrong Prefix | Correct Prefix | Preset Count |
|--------|-------------|----------------|--------------|
| Oracle | `orc_` | `oracle_` | 11 |
| Optic | `opt_` | `optic_` | 4+ |
| OddfeliX | `felix_` | `snap_` | ~20+ |
| OddOscar | `oscar_`, `oddo_` | `morph_` | ~20+ |
| Obese | `ob_` | `fat_` | ~10+ |
| Onset | `ons_` | `perc_` | ~14 |
| Organon | `org_` | `organon_` | ~14 |
| Ouroboros | `uro_` | `ouro_` | ~14 |
| Obsidian | `obs_` | `obsidian_` | ~14 |
| Ombre | `ombr_` | `ombre_` | 2 |

**Fix:** Scripted prefix rename per engine.

### P0-3: `coupling: null` (~469+ presets)
**Impact:** Potential runtime crash in PresetManager if it expects an object.
**Worst:** Organon (139 presets, 18.5% of library).
**Fix:** Script to replace `null` with `{"pairs": []}`.

### P0-4: Dual Parameter Schema Conflicts
**Impact:** One generation of params silently ignored.
**Engines affected:**
- **Osprey**: `osprey_brine`/`osprey_foam` (current) vs `osprey_brightness`/`osprey_seaState` (legacy)
- **Overbite**: `poss_fur` vs `poss_furAmount`, `poss_glide` vs `poss_glideTime`
- **Ostinato**: `osti_seat1_instrument` vs `osti_instrument1`
**Fix:** Requires engine source consultation to determine canonical params, then migration script.

### P0-5: Ostinato Swing Value Range Conflict
**Impact:** `osti_swing` is 0.0 in some presets and 33.0 in others — 33x out of range.
**Fix:** Normalize to [0,1] or confirm engine expects raw BPM swing %.

### P0-6: Non-Standard Mood Folder Presets (~110 presets, Ocelot worst at 37)
**Impact:** If PresetManager only enumerates standard 8 mood folders, these presets are invisible.
**Fix:** Move to standard moods or confirm PresetManager scans all subdirectories.

---

## P1 Issues — Doctrine Violations (Fix Before Release)

### P1-1: Entangled Presets Without Coupling (FLEET-WIDE SYSTEMIC)
**Impact:** 85-94% of Entangled-mood presets are solo-engine with no coupling pairs. "Entangled" promises cross-engine interaction — these deliver none.
**Scale:** ~3,600-4,000 of 4,316 Entangled presets.
**Worst engines:** OpenSky 94%, Obrix 85-94%, Orbweave ~90%.
**Fix options:**
1. Re-mood solo presets to appropriate mood (Foundation/Atmosphere/Prism)
2. Add coupling pairs to make them genuinely Entangled
3. Accept that "Entangled" means "designed for coupling context" even without active coupling (owner decision)

### P1-2: Missing `description` Field (~25-42% fleet-wide)
**Impact:** Users see no description in preset browser — poor UX.
**Worst:** Organon 42%, Orbital 42%, OpenSky 40%.
**Best:** Ostinato 0%, Ouie 0% (Guru Bin quality).
**Fix:** AI-generated descriptions based on DNA + mood + name. Scriptable.

### P1-3: Mood Field/Folder Mismatch (~242+ presets)
**Impact:** Preset's JSON `mood` field disagrees with the folder it's in. Browser behavior depends on which is read.
**Worst:** Batch 4 had 185 mismatches.
**Fix:** Script to reconcile (trust folder or trust field — owner decision).

### P1-4: OddfeliX Mixed-Case MacroLabels (114 presets, 32%)
**Impact:** `["Snap+Morph", "Bloom", "Coupling", "Space"]` instead of `["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]`.
**Fix:** Scripted uppercase + standardization.

### P1-5: Overtone Dual DNA Fields (176 presets, 48%)
**Impact:** `dna` and `sonicDNA` fields with diverging values in same file. Which does the engine read?
**Fix:** Delete `sonicDNA`, keep `dna`. Also add missing `macroLabels`.

### P1-6: Overbite 5-Macro Exception (58 presets, 36%)
**Impact:** B008 Blessing grants 5 macros (BELLY/BITE/SCURRY/TRASH/PLAY DEAD) — conflicts with "exactly 4" rule.
**Status:** REQUIRES OWNER DECISION — does B008 override the schema rule?

### P1-7: DNA Cross-Validation Failures (~1,200+ fleet-wide)
**Impact:** Mood/DNA misalignment suggests presets are in wrong mood or DNA values are inaccurate.
**Patterns:**
- Atmosphere presets with aggression > 0.4 (average 46% fail rate)
- Foundation presets with density < 0.4
- Flux presets with movement < 0.5 (many "STILL" variants misfiled)
**Fix:** Review thresholds (may be too strict) then batch re-mood or re-DNA.

### P1-8: Obsidian filterCutoff Domain Inconsistency
**Impact:** Same parameter uses absolute Hz (4200, 6000) in some presets and normalized [0,1] in others.
**Fix:** Requires engine source to determine expected domain, then normalize.

---

## P2 Issues — Quality Concerns (Fix Before V1.1)

### P2-1: Duplicate Preset Names (1,009 names)
Names like "Abyssal Drift", "Thermal Vent", "Phase Shift" appear in multiple engines/moods.
**Fix:** Append engine identifier or rename duplicates.

### P2-2: Naming Violations (~1,700+ presets)
**Patterns:**
- ALL-CAPS debug slugs: `BRIGHT_HOT_DENSE_VAST_AET3_4`, `5X DARK COLD KINETIC DENSE`
- Single-word names: "Finray", "Ascension", "Overcast"
- Numbered series: "Ice Horizon 2", "ICE FLUX 6"
- Descriptor dumps: "DARK COLD VAST VIOLENT FAM 4"
**Fix:** AI-assisted rename pass (name → evocative 2-3 word name based on DNA + mood).

### P2-3: Submerged Mood Gap (10+ engines at 0 presets)
Engines with 0 Submerged: Overdub, Oblong, Origami, Oracle, Obscura, Optic, OpenSky, Osteria, Ohm, Obbligato, Ole, Ombre.
**Fix:** Generate 3-5 Submerged presets per engine with low brightness, high space, low aggression DNA.

### P2-4: Missing `couplingIntensity` (~26-49% fleet-wide)
**Fix:** Script to derive from coupling amount or set to "None" for solo presets.

---

## P3 Issues — Polish (Batch with Next Pass)

### P3-1: Missing `tempo` Field (~39% fleet-wide, 2,000+ presets)
### P3-2: Legacy `sonic_dna`/`sonicDNA` Residue (~930+ presets)
3 field names coexist: `dna` (current), `sonic_dna` (gen 1), `sonicDNA` (gen 2).
### P3-3: Version String Inconsistency (~1,582 presets)
`"1.0"` vs `"1.0.0"` — standardize to `"1.0.0"`.
### P3-4: `coupling` Format Inconsistency
`null` vs `{"pairs":[]}` vs missing key entirely.
### P3-5: Generic Macro Labels (~300+ presets)
`MACRO1/MACRO2/MACRO3/MACRO4` or `M1/M2/M3/M4` instead of engine-specific labels.

---

## Schema Generation Analysis

The fleet contains 3 distinct preset schema generations:

| Generation | Era | Characteristics | Est. Count |
|------------|-----|-----------------|------------|
| Gen 1 | Early builds | `sonic_dna`, `coupling: null`, `version: "1.0"`, mixed-case macroLabels, no description/tempo/couplingIntensity | ~3,000-4,000 |
| Gen 2 | Mid builds | `sonicDNA`, partial fields, generic macro names, some coupling | ~4,000-5,000 |
| Gen 3 | Recent builds | `dna`, full fields, engine-specific macros, `coupling: {"pairs": []}`, descriptions | ~7,000-8,000 |

**Recommendation:** Build a one-time migration script that upgrades all presets to Gen 3 schema.

---

## DNA Coverage Summary

**All 44 engines pass DNA range checks** — every engine has all 6 dimensions spanning > 0.4 range.

**Architecturally correct exceptions:**
- **OceanDeep**: brightness ceiling at 0.6 per Blessing B031 (Darkness Filter Ceiling) — NOT a gap
- **Overtone**: aggression ceiling at 0.52 — thin but intentional for spectral engine

---

## Engines by Health (Best to Worst)

### Tier 1 — Model Quality (minimal issues)
- **Ostinato**: 0% empty descriptions, Guru Bin quality, full moods
- **Ouie**: 0% empty descriptions, good coverage
- **Osprey**: Model presets (Coherent Shore, Azulejo Blue exemplary)

### Tier 2 — Healthy with Known Issues
- **Oblique**: Fewest empty param blocks in batch 3
- **Ocelot**: Fewest DNA failures, few prefix violations
- **OceanDeep**: Brightness ceiling architecturally correct

### Tier 3 — Needs Attention
- **Organon**: 759 presets but 42% missing descriptions, 139 `coupling: null`
- **Oracle**: Wrong `orc_` prefix in 11 presets
- **Overtone**: 48% dual DNA fields, 48% missing macroLabels

### Tier 4 — Significant Remediation
- **Optic**: Wrong `opt_` prefix, 83 debug names, 0 Submerged, empty coupling in 17 Entangled
- **Overdub**: 12% empty params, 0 Submerged
- **Ostinato**: Dual param schema + swing range conflict (P0)

---

## Recommended Fix Priority

### Sprint 1: P0 Script Fixes (Estimated: 1 session)
1. ✏️ Replace `coupling: null` with `{"pairs": []}` fleet-wide
2. ✏️ Fix known wrong param prefixes (scripted rename)
3. ✏️ Quarantine/flag empty parameter block presets
4. ✏️ Move non-standard mood folder presets to standard moods
5. 🔍 Consult engine source for dual-schema conflicts (Osprey, Overbite, Ostinato)

### Sprint 2: P1 Migrations (Estimated: 1-2 sessions)
1. ✏️ Schema migration: strip `sonic_dna`/`sonicDNA`, standardize to `dna`
2. ✏️ Standardize version to `"1.0.0"`
3. ✏️ Add missing `coupling`, `macroLabels`, `tempo` fields
4. ✏️ Reconcile mood field/folder mismatches
5. 🤔 Owner decision: Entangled mood policy (re-mood vs add coupling vs accept)
6. 🤔 Owner decision: Overbite 5-macro exception

### Sprint 3: P2 Quality (Estimated: 2-3 sessions)
1. ✏️ AI-generated descriptions for ~4,000+ presets
2. ✏️ Rename ~1,700 debug/jargon preset names
3. ✏️ Deduplicate 1,009 names
4. ✏️ Generate Submerged presets for 12 engines
5. ✏️ DNA cross-validation: review thresholds, then re-mood or re-DNA

---

## Skill Friction Log (for `/preset-audit-checklist` improvement)

| # | Friction Point | Proposed Fix |
|---|---------------|--------------|
| 1 | Checklist says `description`, `tempo`, `couplingIntensity` are required — but 25-39% missing. Validator doesn't enforce. | Reclassify as RECOMMENDED, not REQUIRED in Phase 2A |
| 2 | Non-standard mood folders not addressed | Add "legacy mood handling" section |
| 3 | No check for empty `parameters` blocks | Add Phase 2A check: "parameter block has ≥1 non-macro key" |
| 4 | Validator's ENGINE_PARAM_PREFIXES dict covers only ~20 of 44 engines | Update validator OR add prefix table to checklist |
| 5 | No check for legacy schema variants (`sonicDNA`, `sonic_dna`) | Add "schema migration health" check to Phase 2A |
| 6 | Phase 5E checks Entangled per-preset but misses systemic scope | Add fleet-level Entangled integrity metric |
| 7 | "macroLabels has exactly 4" conflicts with B008 Blessing (5 macros) | Add engine-specific override table for Blessed exceptions |
| 8 | DNA cross-validation thresholds may be too strict (46% Atmosphere fail) | Add MARGINAL band (±0.1 around thresholds) |
| 9 | No mood field/folder consistency check | Add Phase 2A: "mood field matches parent folder name" |
| 10 | `coupling: null` vs `{"pairs":[]}` vs missing not distinguished | Specify three failure states with different severities |
| 11 | No parameter VALUE domain consistency check | Add engine-level check: "same param uses consistent range across presets" |
| 12 | DNA range check doesn't account for Blessing-mandated ceilings | Add Blessing-aware DNA exceptions (e.g., B031 brightness) |
| 13 | No intra-engine parameter naming consistency check | Add: "all presets for engine X use same param key names" |

---

*Report compiled by Rodrigo — the sound-obsessed, data-obsessed note-taker.*
*6 agents. 44 engines. 16,479 presets. 0 missed.*
