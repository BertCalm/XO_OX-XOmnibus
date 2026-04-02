# Preset QA Report — 2026-03-21

**Scope:** All Oware, Oxbow, Orbweave, Overtone, Organism, and Ocelot presets
**Total files scanned:** 1,263 `.xometa` files
**Date:** 2026-03-21

---

## Summary

| Check | Status | Issues |
|-------|--------|--------|
| Valid JSON | PASS | 0 |
| Engine ID matches prefix | 1 FAIL | 1 critical (wrong engine entirely) |
| Parameter prefix correct | 2 FAILS | 2 files with wrong prefix |
| Parameter structure (nested key) | 2 FAILS | 2 files with wrong parameter structure |
| sonicDNA values in 0.0–1.0 | PASS | 0 |
| Mood field matches directory | 20 FAILS | Presets in 6 undocumented mood dirs |
| No duplicate preset names (same engine) | 13 FAILS | 13 same-engine name duplicates |
| No duplicate names (cross-engine) | 15 WARNINGS | 15 cross-engine name conflicts |
| Empty parameter blocks | 4 WARNINGS | 4 Ocelot presets with no params |

---

## File Counts by Engine

| Engine | Files |
|--------|-------|
| Oware | 20 |
| Oxbow | 150 |
| Orbweave | 331 |
| Overtone | 327 |
| Organism | 366 |
| Ocelot | 69 |
| **Total** | **1,263** |

---

## CRITICAL: Engine ID Mismatch (1 file)

### `Prism/Overtone_Riddim.xometa`
- **Problem:** File is named `Overtone_Riddim` but `"engines"` array contains `["Overdub"]` and all parameters are under `"Overdub"` key with `dub_` prefix.
- **Root cause:** This is an Overdub preset that was filed in the wrong location with a wrong filename.
- **Fix:** Either rename to `Overdub_Riddim.xometa` and move to an Overdub mood directory, or replace the content with a genuine Overtone preset.

---

## CRITICAL: Parameter Prefix Wrong (2 files)

### `Foundation/Ocelot_Streak.xometa`
- **Problem:** All 16 parameters use `ocel_` prefix instead of the correct `ocelot_` prefix.
- **Examples:** `ocel_ampAttack`, `ocel_filterCutoff`, `ocel_huntMode`, etc.
- **Per CLAUDE.md:** Ocelot parameter prefix is `ocelot_`.
- **Fix:** Rename all parameter keys from `ocel_*` to `ocelot_*`. Also note ADSR values (666.6, 401.2, 409.6) appear to be milliseconds, which is valid for the Ocelot engine (range 0.001–8000ms per `OcelotParameters.h`), but the engine will not load them due to the wrong prefix.

### `Aether/Ocelot_Glacier.xometa`
- **Problem:** Parameter block contains only 4 macro-style keys with wrong prefix: `ocel_macro_character`, `ocel_macro_coupling`, `ocel_macro_movement`, `ocel_macro_space`.
- **Root cause:** Appears to be a stub or macro-only preset using an old/inconsistent prefix schema.
- **Fix:** Either populate with full `ocelot_` parameters, or if this is intentionally macro-only, correct the keys to use the `ocelot_` prefix.

---

## CRITICAL: Wrong Parameter Structure (2 files)

### `Entangled/Ocelot_Dawn.xometa`
- **Problem:** Parameters are stored as a flat dict directly under `"parameters"` rather than nested under the engine key. The file uses `"parameters": { "ocelot_ampAttack": ... }` instead of `"parameters": { "Ocelot": { "ocelot_ampAttack": ... } }`.
- **Also:** The `"engines"` array is correct (`["Ocelot"]`), but `"mood"` is `"Flux"` while the file is in the `Entangled/` directory (mood mismatch).
- **Also:** Uses a non-standard schema with `"engine": "OCELOT"` (singular, uppercase) and `"format": "xometa/1"` instead of standard fields.
- **Fix:** Restructure parameters to `{ "Ocelot": { "ocelot_...": ... } }` format and update mood field.

### `Prism/Overtone_Riddim.xometa`
- **Problem:** Same file as the engine mismatch above — parameters are nested under `"Overdub"` key, not `"Overtone"`.
- **Fix:** See engine mismatch fix above.

---

## WARNING: Empty Parameter Blocks (4 files)

These Ocelot files have `"parameters": { "Ocelot": {} }` — a valid JSON structure but no actual DSP values. The engine will load with all-default parameters, which may or may not be intentional.

| File | Notes |
|------|-------|
| `Family/Ocelot_Daylight_Sprint.xometa` | Created 2026-03-16; has DNA and description but no params |
| `Flux/Ocelot_Sprint.xometa` | Has macros and DNA but no params |
| `Foundation/Ocelot_Quick_Tone.xometa` | Created 2026-03-16; marked "workhorse/utility" |
| `Foundation/Ocelot_Sharp_Stab.xometa` | Created 2026-03-16; marked "stab/quick" |

**Recommendation:** Populate these with actual Ocelot parameter values or explicitly mark them as init-patch stubs.

---

## WARNING: Mood Field Mismatches (20 files)

Six directories exist that are **not in the documented 8-mood system** (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged). Presets in these directories have `"mood"` fields pointing to canonical moods, suggesting they were created for canonical moods and later moved (or the directories are new moods not yet documented).

**Undocumented mood directories and their contents:**

| Directory | Preset `mood` field | Affected Files |
|-----------|---------------------|----------------|
| `Crystalline/` | `"Prism"` | `ORBWEAVE_Glass_Braid`, `ORGANISM_Crystal_Growth`, `OVERTONE_Ice_Spectrum` |
| `Deep/` | `"Submerged"` | `ORBWEAVE_Torus_Abyss`, `ORGANISM_Primordial_Soup`, `OVERTONE_Euler_Foundation`, `Orbweave_DarkAmbient_Braid_Pressure` |
| `Ethereal/` | `"Aether"` | `ORBWEAVE_Kelp_Drift`, `ORGANISM_Fractal_Tide`, `OVERTONE_Pi_Shimmer` |
| `Kinetic/` | `"Flux"` | `ORBWEAVE_Braid_Runner`, `ORGANISM_Traffic_Flow`, `OVERTONE_Convergent_Rush` |
| `Luminous/` | `"Prism"` | `ORBWEAVE_Silk_Thread`, `ORGANISM_Rule_110`, `OVERTONE_Golden_Ratio` |
| `Organic/` | `"Atmosphere"` | `ORBWEAVE_Living_Knot`, `ORGANISM_Reef_Colony`, `OVERTONE_Nautilus_Breath` |

**One additional mismatch in a canonical directory:**
- `Entangled/Ocelot_Dawn.xometa` — `"mood": "Flux"` (see also structural issues above)

**Resolution options:**
1. If these are new moods being piloted: update `"mood"` fields in all affected files to match the directory names, and add the new moods to the master specification.
2. If these are canonical presets temporarily in wrong directories: move files back to their correct canonical mood directories.

---

## CRITICAL: Same-Engine Duplicate Preset Names (13 pairs)

The CLAUDE.md spec states: "Naming: 2-3 words, evocative, max 30 chars, **no duplicates**, no jargon."

These are same-engine name collisions — the most severe category:

| Duplicate Name | Engine | Files |
|----------------|--------|-------|
| "Frozen Colony" | Organism | `Aether/ORGANISM_Awakening_Frozen_Colony.xometa` vs `Aether/Organism_Frozen_Colony.xometa` |
| "Ancient Colony" | Organism | `Aether/Organism_Ancient_Colony.xometa` vs `Atmosphere/Organism_Ancient_Colony.xometa` |
| "Primordial Soup" | Organism | `Aether/Organism_Primordial_Soup.xometa` vs `Deep/ORGANISM_Primordial_Soup.xometa` |
| "Slow Bloom" | Organism | `Aether/Organism_Slow_Bloom.xometa` vs `Foundation/Organism_Slow_Bloom.xometa` |
| "Spore Cloud" | Organism | `Atmosphere/ORGANISM_Awakening_Spore_Cloud.xometa` vs `Atmosphere/Organism_Spore_Cloud.xometa` |
| "Kelp Cathedral" | Orbweave | `Atmosphere/Orbweave_Kelp_Cathedral.xometa` vs `Submerged/Orbweave_DarkAmbient_Kelp_Cathedral.xometa` |
| "Nautilus Breath" | Overtone | `Atmosphere/Overtone_Nautilus_Breath.xometa` vs `Organic/OVERTONE_Nautilus_Breath.xometa` |
| "Crystal Growth" | Organism | `Crystalline/ORGANISM_Crystal_Growth.xometa` vs `Prism/Organism_Crystal_Growth.xometa` |
| "Resonant Colony" | Organism | `Entangled/Organism_Resonant_Colony.xometa` vs `Foundation/Organism_Resonant_Colony.xometa` |
| "Colony Pulse" | Organism | `Family/Organism_Colony_Pulse.xometa` vs `Prism/Organism_Colony_Pulse.xometa` |
| "Glitch Colony" | Organism | `Flux/Organism_Glitch_Colony.xometa` vs `Prism/Organism_Glitch_Colony.xometa` |
| "Cell Division" | Organism | `Foundation/Organism_Cell_Division.xometa` vs `Prism/Organism_Cell_Division.xometa` |
| "Traffic Flow" | Organism | `Foundation/Organism_Traffic_Flow.xometa` vs `Kinetic/ORGANISM_Traffic_Flow.xometa` |

**Pattern:** 10 of 13 same-engine duplicates involve the Organism engine — likely from rapid preset generation across multiple sessions creating copies across different mood directories. Several duplicate pairs involve one UPPERCASE-prefixed file and one mixed-case file, suggesting mood-reassignment events created the copy.

**Fix:** For each pair, determine which version is canonical (correct mood, complete metadata) and delete the other. Note that "Frozen Colony" / "Spore Cloud" / "Primordial Soup" all have an `ORGANISM_Awakening_*` variant — the Awakening variants may be the intended Guru Bin tier presets and should keep their full name to differentiate.

---

## WARNING: Cross-Engine Duplicate Names (15 pairs)

Same name used by two different engines. Less critical than same-engine duplicates (different engines appear as separate items in the preset browser), but violates the "no duplicates" spec and causes confusion.

| Name | Engines Involved | Files |
|------|-----------------|-------|
| "Tectonic Hum" | Organism + Oxbow | `Aether/` vs `Submerged/` |
| "Bioluminescent" | Orbweave + Organism | Both in `Atmosphere/` |
| "Kelp Forest" | Orbweave + Organism | Both in `Atmosphere/` |
| "Phase Ghost" | Orbweave + Oxbow | `Atmosphere/` vs `Entangled/` |
| "Tidal Shift" | Orbweave + Organism | `Atmosphere/` vs `Flux/` |
| "Storm Approach" | Organism + Oxbow | `Atmosphere/` vs `Flux/` |
| "Singing Bowl" | Oxbow + Oware | `Atmosphere/` vs `Prism/` |
| "Phase Lock" | Orbweave + Organism | Both in `Entangled/` |
| "Coupling Donor" | Organism + Overtone | Both in `Entangled/` |
| "Coupling Receiver" | Organism + Overtone | Both in `Entangled/` |
| "Macro Entangle" | Organism + Overtone | Both in `Entangled/` |
| "Pitch Entangle" | Organism + Overtone | Both in `Entangled/` |
| "Spectral Knot" | Overtone + Orbweave | `Entangled/` vs `Prism/` |
| "Macro Dance" | Organism + Overtone | Both in `Flux/` |
| "Silk Thread" | Orbweave + Oxbow | `Luminous/` vs `Organic/` |

**Note:** The Entangled "Coupling Donor/Receiver/Macro Entangle/Pitch Entangle" quartet (Organism + Overtone) may be intentional paired coupling presets — if so, adding an engine qualifier (e.g., "Organism Coupling Donor" vs "Overtone Coupling Donor") would resolve the spec violation while preserving intent.

---

## PASS: Valid JSON

All 1,263 files parsed without JSON errors.

---

## PASS: sonicDNA / dna Values in Range

All `"dna"`, `"sonicDNA"`, and `"sonic_dna"` fields across all 1,263 files have values in the valid 0.0–1.0 range.

**Notes on DNA field naming inconsistency (informational):**
- Most files use `"dna"` (the standard field)
- Oware presets also include a redundant `"sonic_dna"` field (same values)
- Some early Overtone presets include both `"dna"` and `"sonicDNA"` (duplication, not a range error)
- None of these inconsistencies cause range violations

---

## Action Item Summary

### Immediate / Must Fix
1. **`Prism/Overtone_Riddim.xometa`** — Wrong engine (Overdub content in Overtone file). Rename or replace.
2. **`Foundation/Ocelot_Streak.xometa`** — Wrong param prefix (`ocel_` → `ocelot_`). Rename all keys.
3. **`Aether/Ocelot_Glacier.xometa`** — Wrong param prefix (`ocel_macro_*` → `ocelot_*`). Likely also needs full param population.
4. **`Entangled/Ocelot_Dawn.xometa`** — Fix parameter structure (needs nesting under `"Ocelot"` key) and mood field.
5. **13 same-engine duplicate names** — Identify canonical version per pair and delete the copy.

### Should Fix
6. **20 mood mismatches** — Either update `"mood"` fields to match directories (if new moods are being added) or move files to canonical directories.
7. **15 cross-engine duplicate names** — Differentiate names per the spec; add engine qualifiers where pairs are intentional.

### Consider Fixing
8. **4 empty Ocelot parameter blocks** — Populate with actual parameter values or document as intentional init stubs.

---

*Report generated: 2026-03-21 | Tool: manual Python QA scan*
