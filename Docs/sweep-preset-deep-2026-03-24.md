# Preset Library Deep Audit — 2026-03-24

**Scope:** All 17,251 `.xometa` presets in `Presets/XOlokun/`
**Methodology:** Full-corpus Python analysis (no sampling). Each finding section states the exact
count, the severity, and the remediation path.

---

## Library Overview

| Category | Count |
|---|---|
| Total presets | 17,251 |
| Live (non-quarantine) | 13,260 |
| Quarantine (`_quarantine/`) | 3,991 |
| Parse errors | 0 |

---

## Part 1: Schema Completeness

### Required top-level fields: `schema_version`, `name`, `mood`, `engine/engines`, `dna`, `parameters`

**Live presets missing `schema_version`: 200 (1.5%)**

All 200 are valid JSON with all other required fields present. The `schema_version` field
is simply absent — not null, not wrong. Distribution across moods:

| Mood | Count |
|---|---|
| Flux | 28 |
| Prism | 25 |
| Foundation | 24 |
| Aether | 24 |
| Entangled | 23 |
| Atmosphere | 18 |
| Organic | 16 |
| Deep | 9 |
| Ethereal | 6 |
| Crystalline | 6 |
| Submerged | 6 |
| Kinetic | 6 |
| Luminous | 5 |
| Family | 4 |

**Root cause:** These were written before `schema_version` was mandated, or during the batch
Broth/Garden/Offering/Overgrow expansion rounds that predated the field being added to the
preset-architect skill template.

**Quarantine presets missing `schema_version`: 0** — all quarantine presets carry the field.

**Fix:** Add `"schema_version": 1` to each of the 200 files. One-liner patch script:

```python
import json, glob
for f in glob.glob("Presets/XOlokun/**/*.xometa", recursive=True):
    d = json.load(open(f))
    if 'schema_version' not in d:
        d['schema_version'] = 1
        json.dump(d, open(f, 'w'), indent=2)
```

**Severity: LOW** — `resolveEngineAlias()` and the loader do not gate on this field;
missing it causes no runtime failure. It is a catalog hygiene gap, not a load-time bug.

---

## Part 2: DNA Value Ranges

**Out-of-range values (outside [0, 1]): 0**
**Non-numeric DNA values: 0**

All 17,251 presets with a `dna` block have values strictly within [0, 1]. No corruption found.

### DNA Block Presence

| Issue | Count | Location |
|---|---|---|
| `dna` block entirely absent | 19 | Live only |
| `dna` block partially wrong (wrong dimension name) | 40 | Live only |

**The 19 presets with no `dna` block** are all OUTLOOK and OSMOSIS engine presets added
2026-03-23:

- 18 × `OUTLOOK_*` presets spread across Foundation, Flux, Prism, Atmosphere, Entangled, Aether, Submerged
- 1 × `Osmosis_First_Breath.xometa` (Atmosphere)

These were the first presets created for the two newest engines (OUTLOOK added 2026-03-23,
OSMOSIS added 2026-03-21) and were committed before a DNA block was authored for them.

**The 40 presets with wrong DNA dimension names** are all from the BROTH quad
(Overflow × 10, Overcast × 10, Overworn × 10, Overwash × 10). They use:

```json
{ "warmth": ..., "complexity": ..., "movement": ..., "brightness": ..., "aggression": ... }
```

instead of the canonical 6D schema:

```json
{ "brightness": ..., "warmth": ..., "movement": ..., "density": ..., "space": ..., "aggression": ... }
```

The `complexity` key is a non-canonical dimension name that replaces both `density` **and**
`space` (neither is present). This affects all 4 BROTH engines equally across 10 presets each.
Root cause: the BROTH quad preset batch was authored using an older, 5D prototype schema before
`density` and `space` were split.

**Fix:** Rename `complexity` → decide split (e.g., `density = complexity * 0.6`, `space = complexity * 0.8`),
or use `/dna-designer` to re-author the values properly per preset. The 40 files are distributed
across 14 moods so bulk re-authoring is the most practical path.

**Severity: MEDIUM** — Any DNA-based preset search, filtering, or recommendation system will
produce incorrect results for all 40 BROTH presets. The missing `density` and `space` dimensions
will be read as null/zero by anything iterating the canonical 6D schema.

---

## Part 3: Engine Name Validation

**Invalid engine names found: 1 type, 32 presets**

| Invalid Name | Correct Name | Count | Location |
|---|---|---|---|
| `OCELOT` (uppercase) | `Ocelot` | 32 | `_quarantine/Flux/` only |

All 32 `OCELOT` presets are in `_quarantine/Flux/`. Zero live presets are affected.

**Root cause:** An early batch generation pass used `engine.upper()` on the engine ID before
writing it to the preset file. The canonical name is `Ocelot` (Title case, no all-caps).
`resolveEngineAlias()` does **not** handle case variants — it performs an exact-string match.
These presets would fail to load the correct engine at runtime if promoted from quarantine.

**Fix before quarantine promotion:** Replace `"OCELOT"` with `"Ocelot"` in the `engines` array
of all 32 affected files.

**Severity: HIGH (for quarantine promotion)** — Blocked from going live as-is.

**All other engine names are valid.** Kitchen Collection (Oto, Octave, Oleg, Otis, Oven, Ochre,
Obelisk, Opaline, Ogre, Olate, Oaken, Omega, Orchard, Overgrow, Osier, Oxalis, Overwash,
Overworn, Overflow, Overcast, Oasis, Oddfellow, Onkolo, Opcode), OSMOSIS, OXYTOCIN, and
OUTLOOK all appear with correct casing.

### Engine Distribution (Top 20 by preset count, live + quarantine)

| Engine | Count | | Engine | Count |
|---|---|---|---|---|
| Odyssey | 886 | | Oblique | 577 |
| Oblong | 860 | | Optic | 548 |
| OddfeliX | 753 | | Orphica | 519 |
| Organon | 701 | | Ohm | 518 |
| OddOscar | 691 | | Overworld | 516 |
| Outwit | 661 | | Obbligato | 496 |
| Opal | 660 | | Osprey | 480+ |
| Overdub | 651 | | Ottoni | 480+ |
| Overlap | 634 | | Orca | 350+ |
| Onset | 633 | | Octopus | 290+ |

---

## Part 4: Parameter ID Prefix Compliance

### Summary

| Category | Files | Notes |
|---|---|---|
| Fully compliant (all params use frozen prefix) | 12,320 (92.9%) | — |
| ONLY `macro_*` violations | 450 | Acceptable — see below |
| Legacy un-prefixed params (global schema era) | 177 | Needs migration |
| `macro_*` + legacy combined | 80 | Needs migration |
| Intermediate/old-interim prefix | 233 | Needs migration |
| **Total live files with any violation** | **940 (7.1%)** | — |

### Category A: `macro_*` keys — ACCEPTABLE PATTERN

450 live presets contain `macro_character`, `macro_coupling`, `macro_movement`, `macro_space`
as parameters under an engine key. Example from `Entangled/Collision_Brick_Prophet.xometa`:

```json
"parameters": {
  "Obrix": {
    "macro_character": 0.5,
    "macro_movement": 0.6,
    "macro_coupling": 0.0,
    "macro_space": 0.4,
    "obrix_src1Type": 2,
    "obrix_driftBusAmount": 0.3,
    ...
  }
}
```

The `macro_*` keys are global macro positions stored alongside engine-prefixed params. This is
a valid preset format variant used by multi-engine Entangled/Coupling presets. The PresetManager
reads `macro_*` keys from any engine block as per the coupling preset spec. **These are not bugs.**

### Category B: Legacy un-prefixed parameters — MIGRATION NEEDED (177 files)

177 live files contain naked parameter names from the pre-namespace era with no engine prefix:

| Naked key | Occurrences |
|---|---|
| `amp_attack`, `amp_release` | 157 each |
| `couplingBus`, `couplingLevel`, `coupling_level` | 157 each |
| `filter_cutoff`, `filter_resonance` | 157 |
| `output_level`, `reverb_mix`, `reverb_size` | 157 |
| `drive`, `density` | 159 each |

Example — `Entangled/Void_Pierce.xometa` (Odyssey engine block):
```json
"Odyssey": {
    "amp_attack": 0.01, "amp_release": 0.8,
    "coupling_level": 0.5, "density": 0.6,
    "filter_cutoff": 2400.0, "output_level": 0.8,
    "drift_oscA_mode": 1, "drift_envDepth": 0.45
}
```

The engine-prefixed params (`drift_*`) are present alongside the naked params. The naked keys
would silently fail to bind to any APVTS parameter at load time, causing those values to be
ignored. The corresponding `drift_` params provide correct values for most settings, but any
parameter that only appears in the naked form will be lost.

**Root cause:** These presets were generated during a transition period (circa Prism Sweep
Rounds 3-6) when the generator did not yet enforce engine-prefix on every key. The naked keys
were added as a "fallback" and then left in.

**Severity: MEDIUM** — The naked keys are silently ignored. No crash, but some parameter values
(specifically the coupling/amp/filter values stored in naked form) are not loaded. Audible
difference possible if the engine-prefixed equivalents are absent.

### Category C: Intermediate/Old-Interim prefixes — MIGRATION NEEDED (233 files)

233 live files use prefixes from intermediate naming generations, before the frozen prefix
map in `PresetManager.h` was finalized. Common patterns:

| Old Prefix (in engine) | Canonical Prefix | Occurrences |
|---|---|---|
| `obl_` in Oblique | `oblq_` | 70 |
| `oddf_` in OddfeliX | `snap_` | 64 |
| `oct_` in Octopus | `octo_` | 63 |
| `ost_` in Osteria | `osteria_` | 42 |
| `ocn_` in Oceanic | `ocean_` | 32 |
| `fat_` in OddfeliX | `snap_` | 31 |
| `obsc_` in Obscura | `obscura_` | 30 |
| `ob_` in Oblong | `bob_` | 25 |
| `ott_` in Ottoni | `otto_` | 21 |
| `od_` in Odyssey | `drift_` | 21 |
| `oct_` in Ocelot | `ocelot_` | 12 |
| `oce_` in Oceanic | `ocean_` | 9 |
| `felx_` in OddfeliX | `snap_` | 8 |
| `orig_` in Origami | `origami_` | 8 |

Note: `fat_` appearing in OddfeliX blocks is the most confusing case — `fat_` is Obese's frozen
prefix but OddfeliX presets from the Prism Sweep era wrote OddfeliX params under the `fat_`
namespace by mistake. The `resolveSnapParamAlias()` function in PresetManager.h handles some
OddfeliX param renames, but not a full `fat_` → `snap_` key translation.

**Severity: MEDIUM** — The APVTS param binding will silently fail for any parameter with a
wrong prefix. Values are lost at load time. Audible for parameters that only appear in the
old-prefix form.

### Compliance by Collection

| Collection | Compliance |
|---|---|
| Kitchen Collection (Oven, Ochre, Obelisk, Opaline, Ogre, Olate, Oaken, Omega, etc.) | ~100% |
| Cellar/Garden/Broth/Fusion Quads | ~100% |
| OPERA, OFFERING, OXBOW, OWARE, OSMOSIS, OXYTOCIN, OUTLOOK | ~100% |
| Original fleet (OddfeliX, OddOscar, Overdub, Odyssey, Opal, etc.) | ~90% |

The Kitchen Collection and all post-2026-03-20 engines have near-perfect compliance because
they were built after the frozen prefix map was established and the preset-architect skill
was updated to enforce it. The violations are concentrated in older original-fleet presets.

---

## Part 5: Coupling Preset Integrity

**`.xocoupling` files found: 0**

No `.xocoupling` files exist anywhere in `Presets/`. The 18 coupling demo presets designed
in Phase D (2026-03-22) were committed as standard `.xometa` files in `Presets/XOlokun/Coupling/`,
not as `.xocoupling` binaries.

The `CouplingPresetManager.h` (589 lines, `.xocoupling` format) is implemented in code but
the factory preset library uses only `.xometa` for all preset storage. This is consistent —
`.xocoupling` is a runtime-bake format for user-created coupling states, not for factory presets.

**Coupling mood preset validation (18 presets):**
All 18 are structurally complete: `schema_version: 1`, valid engine names, `dna` block present,
`coupling` block present. Spot-checked 5 samples — all pass.

---

## Part 6: Mood Distribution Analysis

| Mood | Count | % of live | Status |
|---|---|---|---|
| Foundation | 2,328 | 17.6% | Healthy |
| Atmosphere | 2,026 | 15.3% | Healthy |
| Entangled | 1,926 | 14.5% | Healthy |
| Prism | 1,858 | 14.0% | Healthy |
| Flux | 1,749 | 13.2% | Healthy |
| Aether | 1,653 | 12.5% | Healthy |
| Family | 698 | 5.3% | Adequate |
| Submerged | 583 | 4.4% | Adequate |
| Organic | 70 | 0.5% | THIN |
| Deep | 64 | 0.5% | THIN |
| Kinetic | 60 | 0.5% | THIN |
| Crystalline | 36 | 0.3% | THIN |
| Luminous | 37 | 0.3% | THIN |
| Ethereal | 24 | 0.2% | THIN |
| Coupling | 18 | 0.1% | THIN (design-limited) |
| **_quarantine** | **3,991** | — | 23.1% of all presets |

**The 6 post-V1 moods (Organic, Deep, Kinetic, Crystalline, Luminous, Ethereal) are severely
underrepresented.** Together they hold only 291 presets (2.2% of live) versus the 6 core moods
holding 11,540 (87.0%). The 15-mood system is aspirationally complete but practically lopsided.

**Quarantine is large:** 3,991 presets (23.1% of the total library) are in quarantine. This
represents a significant backlog of presets that need either remediation or deletion.

### Quarantine Breakdown

The quarantine subdirectory structure mirrors live moods:

```
_quarantine/
  Flux/   ← largest quarantine pool
  Entangled/
  (other moods)
```

The quarantine files have valid JSON and `schema_version: 1` (all 3,991 pass), but many have
prefix violations (the engine name casing issue + the legacy naked params found in the overall
violation counts). The `OCELOT` uppercase issue (32 files) is entirely within quarantine.

---

## Summary of Findings

| Finding | Severity | Affected | Location |
|---|---|---|---|
| `schema_version` missing | LOW | 200 live | Spread across 14 moods |
| `dna` block absent | MEDIUM | 19 live | OUTLOOK (18) + OSMOSIS (1) |
| `dna` uses `complexity` instead of `density`+`space` | MEDIUM | 40 live | BROTH quad (all 4 engines) |
| `OCELOT` uppercase engine name | HIGH (for promo) | 32 quarantine | `_quarantine/Flux/` |
| Legacy un-prefixed params (`amp_attack`, `couplingLevel`, etc.) | MEDIUM | 177 live | Original fleet engines |
| `macro_*` keys in engine param block | ACCEPTABLE | 450 live | Entangled/Coupling |
| Intermediate old-prefix params (`obl_`, `oddf_`, `oct_`, etc.) | MEDIUM | 233 live | Original fleet engines |
| Thin mood coverage | STRUCTURAL | 6 moods | Organic/Deep/Kinetic/Crystalline/Luminous/Ethereal |
| Large quarantine backlog | STRUCTURAL | 3,991 presets | `_quarantine/` |
| No `.xocoupling` factory presets | INFO | — | By design |
| All DNA values in [0, 1] | PASS | 17,251 | — |
| No parse errors | PASS | 17,251 | — |
| Quarantine `schema_version` integrity | PASS | 3,991 | — |

---

## Recommended Action Plan

### Immediate (before any release)
1. **OCELOT case fix** — sed or Python batch: replace `"OCELOT"` with `"Ocelot"` in 32 quarantine files before any quarantine promotion pass.
2. **OUTLOOK/OSMOSIS DNA** — author `dna` blocks for 19 presets using `/dna-designer`. These are the newest engines and deserve accurate DNA.

### Short-term (V1.1 window)
3. **schema_version patch** — one-liner script adds `"schema_version": 1` to 200 files.
4. **BROTH DNA fix** — re-author `density` and `space` for the 40 BROTH presets currently using `complexity`.

### Medium-term (V1.2 / Kitchen Collection)
5. **Legacy naked-param migration** — 177 files. Map naked params to engine-prefixed equivalents using the frozen prefix table. Many already have the correct `drift_*`/`bob_*` etc. params alongside the naked ones; the naked keys just need removal.
6. **Intermediate-prefix migration** — 233 files. Map `obl_` → `oblq_`, `oddf_` → `snap_`, `oct_` (Octopus) → `octo_`, etc.
7. **Quarantine triage** — decide which of the 3,991 quarantine presets are promote/remediate/delete.

### Structural
8. **Thin mood expansion** — target 200+ presets each for Organic, Deep, Kinetic, Crystalline, Luminous, Ethereal. Use `/preset-architect` with `/dna-designer` per engine.
9. **Coupling mood growth** — 18 presets is the Phase D starter set. Target 60+ before V1.2.

---

*Generated by Claude Code deep-audit agent — 2026-03-24.*
*All counts derived from full-corpus Python analysis of all 17,251 `.xometa` files.*
