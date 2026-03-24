# XOlokun Quality Sweep Report -- 2026-03-20

Audit scope: READ-ONLY code quality sweep across the 3 newest engines (ORBWEAVE,
OVERTONE, ORGANISM), preset health, documentation accuracy, seance coverage, and
registration completeness.

---

## 1. Code Quality -- Newest Engines

### 1.1 Hardcoded Sample Rate Initializers (P1)

All three engines initialize their `sr` member to `44100.f` as a default before
`prepare()` is called. This is acceptable as a fallback value (overwritten in
`prepare()`), but sub-components also carry their own `sr = 44100.f` defaults
which creates redundancy and risk if `prepare()` is not called.

| Engine | Instances | Lines |
|--------|-----------|-------|
| ORBWEAVE | 1 (engine `sr`) | L1092 |
| OVERTONE | 3 (OverPartialOsc, OverAllpassReso, engine) | L174, L204, L835 |
| ORGANISM | 5 (OrgSawOsc, OrgSquareOsc, OrgTriOsc, OrgSubOsc, engine) | L106, L124, L141, L158, L879 |

**Severity: P1** -- The default values are overwritten at prepare(), so this is
not a runtime bug. However, the per-component defaults create a risk: if a new
component is added without calling `prepare()`, it would silently use 44100.

**Recommended action:** Consider removing default `sr` values from sub-components
and requiring explicit preparation via the parent engine.

### 1.2 Reverb Buffer Sizes Not Sample-Rate-Scaled (P1)

ORBWEAVE correctly scales its reverb delay line lengths by `sr / 44100.0f`
(OrbweaveFXState::prepare, L154). However:

- **OVERTONE (OverSpaceReverb):** Comb delay buffer sizes are compile-time
  constants (1116, 1188, 1277, 1356 samples). At 96 kHz, these produce reverb
  times roughly half of what they would at 44.1 kHz. The `prepare()` method
  (L268) just calls `reset()` -- no sample-rate scaling.

- **ORGANISM (OrgReverb):** Same pattern. All comb and allpass buffers are
  fixed-size arrays (kComb1=1741 through kComb4=2311, kAP1=347 through
  kAP4=1471). No sample-rate adaptation.

**Severity: P1** -- At 48 kHz (common on modern interfaces) the error is ~9%,
which may be acceptable. At 96 kHz the reverb character changes significantly.

**Recommended action:** Scale comb/allpass lengths by `sr / 44100.f` at
prepare-time, matching the ORBWEAVE pattern.

### 1.3 Denormal Protection (P2 -- Adequate)

All three engines use `flushDenormal()` in feedback paths:

| Engine | flushDenormal calls | Key locations |
|--------|-------------------|---------------|
| ORBWEAVE | 7 | ADSR decay/release (L72/78), FX delay feedback (L932/933), reverb filter+comb (L979/982/988) |
| OVERTONE | 7 | Comb filter (L280/282), allpass (L223/225/268), envelope (L718), biquad (L361) |
| ORGANISM | 5 | Comb filter (L278/280), allpass (L268), envelope (L769), biquad (L219) |

Coverage is adequate. No missing denormal protection in feedback paths found.

### 1.4 Memory Allocation on Audio Thread (P2 -- Clean)

No heap allocations (`new`, `malloc`, `resize`, `assign`) found in any
`renderBlock()` path.

- ORBWEAVE: `vector::assign()` calls are in `prepare()` only (L146-159).
- OVERTONE: All buffers are fixed-size stack arrays in structs.
- ORGANISM: All buffers are fixed-size stack arrays in structs.

**Status: PASS** -- No audio-thread allocations detected.

### 1.5 Unused Variables / Parameters (P2 -- Minor)

- ORBWEAVE `renderBlock`: `macroSpace` is read (L371) and used (L564).
  `knotMorph` (L396) is computed and used (L400). No unused variables found.
- OVERTONE `renderBlock`: `effectiveResoMix` (L809) is computed and used
  (L816/817). `macroCoupling` (L632) is used for shimmer. Clean.
- ORGANISM `renderBlock`: `PB` type alias declared at L332 but only used at
  L348 (org_freeze). Clean -- PB is used.

**Status: PASS** -- No unused variables detected in the 3 engines.

---

## 2. Preset Health

### 2.1 Total Count

**15,199 `.xometa` preset files** found under `Presets/XOlokun/`.

### 2.2 Spot-Check: 3 Random Presets

| File | Valid JSON | name | author | engine | parameters | dna |
|------|-----------|------|--------|--------|------------|-----|
| `Foundation/DARK_HOT_KINETIC_VIOLENT_FND_AGG_3.xometa` | YES | "Rust Blade" | "XO_OX Designs" | ["Oblong","Overbite"] | YES (both engines) | YES (6D) |
| `Flux/Orca_Hunt_Velocity.xometa` | YES | "Hunt Velocity" | "XO_OX Designs" | ["Orca"] | YES | YES (6D) |
| `Aether/Obese_Spectral_Ghost.xometa` | YES | "Spectral Ghost" | "XO_OX Designs" | ["Obese"] | YES | YES (6D) |

All three presets have valid JSON, and contain all required fields: `name`,
`author`, `engines`, `parameters`, and `dna` (with all 6 dimensions:
brightness, warmth, movement, density, space, aggression).

**Status: PASS**

### 2.3 Presets for Newest Engines

Presets referencing ORBWEAVE, OVERTONE, and ORGANISM exist in the `Flux/` mood
directory (at least 10 found via filename search: Overtone_Ratio_Sweep,
Overtone_Depth_Wander, Overtone_Color_Throb, Overtone_Partial_Orbit,
Overtone_Euler_Flux, Organism_Aftertouch_Chaos, Organism_Colony_Revolt,
Organism_Mutation_Drift, Organism_Seed_Roller, Organism_Drift_Colony).

**Note:** No ORBWEAVE-specific presets found by filename search. Orbweave
presets may exist under different naming conventions, but this warrants
verification.

**Severity: P1** -- If no Orbweave presets exist, this engine ships with no
factory content.

**Recommended action:** Verify Orbweave preset count. If zero, generate factory
presets before release.

---

## 3. Documentation Accuracy

### 3.1 Engine Count

CLAUDE.md states **"42 engines"** (line 7). The `Source/Engines/` directory
contains **42 subdirectories**, and `XOlokunProcessor.cpp` has **42
`registered_` lines**. All three numbers match.

**Status: PASS**

### 3.2 ORBWEAVE / OVERTONE / ORGANISM Listed

All three appear in:
- The engine list (line 8 onward in CLAUDE.md)
- The engine accent color table (lines 83-85: ORBWEAVE, OVERTONE, ORGANISM)
- The parameter prefix table (Orbweave=`weave_`, Overtone=`over_`, Organism=`org_`)

**Status: PASS**

### 3.3 Duplicate Rows in Engine Table (P2)

The engine table in CLAUDE.md contains **duplicate entries** for:
- **OVERLAP** -- appears twice (lines 71 and 80) with slightly different accent
  color descriptions ("Bioluminescent Cyan-Green" vs "Bioluminescent Mint")
- **OUTWIT** -- appears twice (lines 72 and 81) with slightly different accent
  color descriptions ("Chromatophore Amber" vs "Ochre Burn")

This makes the visual table show 44 rows instead of 42, which could confuse
readers.

**Severity: P2** -- Cosmetic documentation issue, no functional impact.

**Recommended action:** Remove the duplicate rows (lines 80-81) to match the
actual 42-engine count.

---

## 4. Seance Coverage

### 4.1 Seance Verdict Files Present

Files in `Docs/seances/`:

| Engine | Verdict File | Status |
|--------|-------------|--------|
| ORBWEAVE | `orbweave_seance_verdict.md` | PRESENT |
| OVERTONE | `overtone_seance_verdict.md` | PRESENT |
| ORGANISM | `organism_seance_verdict.md` | PRESENT |
| OSTINATO | `ostinato_seance_verdict.md` | PRESENT |
| OPENSKY | `opensky_seance_verdict.md` | PRESENT |
| OCEANDEEP | `oceandeep_seance_verdict.md` | PRESENT |
| OUIE | `ouie_seance_verdict.md` | PRESENT |
| OBRIX | `obrix_seance_verdict.md` | PRESENT |
| OVERLAP | `overlap_seance_verdict.md` | PRESENT |
| OUTWIT | `outwit_seance_verdict.md` | PRESENT |

All 10 required engines have seance verdicts.

### 4.2 Additional Seance Files

Also present: `ombre_seance_verdict.md`, `orca_seance_verdict.md`,
`octopus_seance_verdict.md` (3 additional engines with verdicts).

Total seance verdict files: **13** (plus 3 non-verdict files: prep docs and
raw seance notes).

**Status: PASS** -- All requested engines have verdict files.

---

## 5. Registration Completeness

### 5.1 Registration Count

`XOlokunProcessor.cpp` contains **42 `registered_` static variable
declarations**, one for each engine:

OddfeliX, OddOscar, Overdub, Odyssey, Oblong, Obese, Onset, Overworld,
Opal, Bite, Organon, Ocelot, Ouroboros, Obsidian, Origami, Oracle,
Obscura, Oceanic, Optic, Oblique, Orbital, Osprey, Osteria, Owlfish,
Ohm, Orphica, Obbligato, Ottoni, Ole, XOverlap, XOutwit, Ombre, Orca,
Octopus, OpenSky, Ostinato, Oceandeep, Ouie, Obrix, Orbweave, Overtone,
Organism.

### 5.2 Cross-Check

- Engine directories: **42**
- Registrations: **42**
- CLAUDE.md declared count: **42**

**Status: PASS** -- All counts match.

---

## Summary Table

| Category | Finding | Severity | Action Needed |
|----------|---------|----------|---------------|
| Reverb SR scaling (OVERTONE, ORGANISM) | Fixed buffer sizes not adapted to sample rate | P1 | Scale by sr/44100 at prepare() |
| Orbweave preset coverage | No Orbweave-named presets found | P1 | Verify; generate factory presets if missing |
| Default sr initializers | Sub-components carry redundant 44100 defaults | P1 | Remove defaults, require explicit prepare() |
| CLAUDE.md duplicate table rows | OVERLAP and OUTWIT appear twice in engine table | P2 | Delete duplicate rows (keep first occurrence) |
| Denormal protection | Adequate across all 3 engines | -- | No action |
| Audio-thread allocation | No heap allocation in renderBlock | -- | No action |
| Preset health | 15,199 presets, spot-check all valid | -- | No action |
| Registration completeness | 42/42 match across all sources | -- | No action |
| Seance coverage | All 10 required verdicts present | -- | No action |

### P0 findings: 0
### P1 findings: 3
### P2 findings: 1

---

*Report generated 2026-03-20 by quality sweep audit (read-only).*
