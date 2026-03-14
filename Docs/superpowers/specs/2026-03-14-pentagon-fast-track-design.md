# XOrphica Family Pentagon — Fast Track Build Specification

**Date:** 2026-03-14
**Status:** Approved
**Scope:** 7 sub-projects building 5 family engines from shared DSP through full XOmnibus integration

---

## Goal

Build all 5 XOrphica Family Pentagon engines (XOhm, XOrphica, XObbligato, XOttoni, XOlé) as standalone instruments, integrate them into XOmnibus, implement the family ecosystem features (macro bleed, family coupling, Family Dinner presets, Obed easter egg), and deliver production-quality documentation and QA.

## Architecture

All 5 engines share a `FamilyWaveguide.h` DSP core (delay line + damping + body resonance + sympathetic bank + organic drift) with engine-specific exciters. Each engine is built standalone first (AU + Standalone), then integrated via thin adapter into XOmnibus. The family ecosystem features (cross-engine macro bleed, named coupling routes) are added in the final integration phase.

## Tech Stack

- JUCE 8.0.12, C++17, CMake + Ninja
- macOS (AU + Standalone), iOS (AUv3 + Standalone) in v2
- `.xometa` JSON presets from day one
- All DSP inline in `.h` headers (XO_OX standard)
- ParamSnapshot pattern for zero-cost per-sample reads

---

## Sub-Project Breakdown

### SP1: FamilyWaveguide.h — Shared DSP Library

**Location:** `~/Documents/GitHub/XO_OX-XOmnibus/Source/DSP/FamilyWaveguide.h`
**Lines:** ~600-800
**Dependencies:** FastMath.h (existing)

**Components:**
- `FamilyDelayLine` — Fractional delay with Lagrange interpolation, tunable pitch
- `FamilyDampingFilter` — One-pole LP in feedback path (material/air absorption)
- `FamilyBodyResonance` — 2-pole resonator modeling instrument body (wood, gourd, metal)
- `FamilySympatheticBank` — 8-12 tuned comb filters for sympathetic string/tube vibration
- `FamilyOrganicDrift` — Slow pitch/timing wander (configurable ±cents, ±ms jitter)

**Exciters (engine-specific but defined in shared header):**
- `PluckExciter` — Noise burst for harp (XOrphica)
- `StrumExciter` — Multi-string sequential trigger (XOlé)
- `PickExciter` — Fingerstyle/clawhammer attack (XOhm)
- `AirJetExciter` — Filtered noise for flute family (XObbligato Brother A)
- `ReedExciter` — Nonlinear waveshaper for reed family (XObbligato Brother B)
- `LipBuzzExciter` — Lip oscillation for brass (XOttoni)
- `BowExciter` — Continuous friction for fiddle (XOhm)

**Test:** Standalone C++ test that instantiates each component, feeds impulse, verifies output is non-zero, non-NaN, and decays.

---

### SP2: XOhm — Standalone Instrument

**Repo:** `~/Documents/GitHub/XOhm/`
**Gallery code:** OHM | Prefix: `ohm_` | Plugin code: `Xohm` | Accent: Sage `#87AE73`
**Concept brief:** `Docs/concepts/xohm_concept_brief.md`
**Estimated LOC:** ~2,000 DSP + ~800 scaffold = ~2,800 total

**Build phases:**
1. Scaffold (`/new-xo-project`) + Parameters.h (~33 params, `ohm_` prefix)
2. FamilyWaveguide integration — PickExciter + BowExciter for Dad's instruments
3. Dad's instrument gallery (banjo, guitar, mandolin, fiddle, harmonica, djembe, kalimba, sitar, ukulele — waveguide presets)
4. In-laws' interference (theremin model, glass harmonica, spectral freeze, granular deconstruction)
5. Obed's FM voice (2-op FM with atomic ratio presets)
6. MEDDLING system (threshold-based voice activation)
7. COMMUNE system (organic drift injection into FM/spectral voices)
8. FX chains (Campfire, Laboratory, Rival)
9. Macros (JAM, MEDDLING, COMMUNE, MEADOW)
10. UI (warm organic panel + cold intrusion panel)
11. 50 presets across 6 moods
12. DSP stability + auval + QA

---

### SP3: XOrphica — Standalone Instrument

**Repo:** `~/Documents/GitHub/XOrphica/`
**Gallery code:** ORPHICA | Prefix: `orph_` | Plugin code: `Xorp` | Accent: Siren Seafoam `#7FDBCA`
**Concept brief:** `Docs/concepts/xorphica_concept_brief.md`
**Estimated LOC:** ~1,800 DSP + ~800 scaffold = ~2,600 total

**Build phases:**
1. Scaffold + Parameters.h (~40 params, `orph_` prefix)
2. FamilyWaveguide integration — PluckExciter for harp strings (4 materials: Nylon/Steel/Crystal/Light)
3. Sympathetic resonance body (FamilySympatheticBank tuned to harp)
4. Microsound engine (per-voice granular: Stutter/Scatter/Freeze/Reverse)
5. Note-based crossover router (MIDI note → LOW/HIGH path with blend zone)
6. FX Path LOW (sub harmonic, tape sat, dark delay, deep plate)
7. FX Path HIGH (shimmer, micro delay, spectral smear, crystal chorus)
8. FRACTURE gradient (per-voice → global sync)
9. Macros (PLUCK, FRACTURE, SURFACE, DIVINE)
10. UI (water-surface themed, crossover visualizer)
11. 50 presets
12. DSP stability + auval + QA

---

### SP4: XObbligato — Standalone Instrument

**Repo:** `~/Documents/GitHub/XObbligato/`
**Gallery code:** OBBLIGATO | Prefix: `obbl_` | Plugin code: `Xobl` | Accent: Rascal Coral `#FF8A7A`
**Concept brief:** `Docs/concepts/xobbligato_concept_brief.md`
**Estimated LOC:** ~1,600 DSP + ~800 scaffold = ~2,400 total

**Build phases:**
1. Scaffold + Parameters.h (~41 params, `obbl_` prefix)
2. FamilyWaveguide integration — AirJetExciter (Brother A) + ReedExciter (Brother B)
3. Worldwide wind instrument presets (8 per brother)
4. Voice routing system (Alternate/Split/Layer/Round Robin/Velocity)
5. BOND interaction engine (multi-breakpoint modulation table across 8 stages)
6. FX Chain A "The Air" (chorus, bright delay, plate, exciter)
7. FX Chain B "The Water" (phaser, dark delay, spring reverb, tape sat)
8. Macros (BREATH, BOND, MISCHIEF, WIND)
9. UI (split panel — bright left, dark right)
10. 50 presets
11. DSP stability + auval + QA

---

### SP5: XOttoni — Standalone Instrument

**Repo:** `~/Documents/GitHub/XOttoni/`
**Gallery code:** OTTONI | Prefix: `otto_` | Plugin code: `Xott` | Accent: Patina `#5B8A72`
**Concept brief:** `Docs/concepts/xottoni_concept_brief.md`
**Estimated LOC:** ~1,800 DSP + ~800 scaffold = ~2,600 total

**Build phases:**
1. Scaffold + Parameters.h (~38 params, `otto_` prefix)
2. FamilyWaveguide integration — LipBuzzExciter with age-scaled capability
3. Three voice engines: Toddler (simple tube), Tween (valved), Teen (full bore)
4. Age-scaled expression (vibrato, dynamics, articulation per cousin)
5. Worldwide brass & sax instrument presets (6 toddler, 6 tween, 10 teen)
6. GROW crossfade engine (age-weighted mix of all three)
7. Foreign harmonics processor (overtone stretch, microtonal drift, cold timbre)
8. FX chains: Toddler (big reverb), Tween (chorus+delay+drive), Teen (full pedalboard)
9. Macros (EMBOUCHURE, GROW, FOREIGN, LAKE)
10. UI (three columns of increasing complexity)
11. 50 presets
12. DSP stability + auval + QA

---

### SP6: XOlé — Standalone Instrument

**Repo:** `~/Documents/GitHub/XOle/`
**Gallery code:** OLE | Prefix: `ole_` | Plugin code: `Xole` | Accent: Hibiscus `#C9377A`
**Concept brief:** `Docs/concepts/xole_concept_brief.md`
**Estimated LOC:** ~2,500 DSP + ~800 scaffold = ~3,300 total

**Build phases:**
1. Scaffold + Parameters.h (~32 params, `ole_` prefix)
2. FamilyWaveguide integration — StrumExciter for 3 aunts
3. Aunt 1: Tres cubano (3 double courses, montuno patterns)
4. Aunt 2: Berimbau (single wire + gourd, coin press pitch bend)
5. Aunt 3: Charango (10 strings, rapid tremolo/arpeggio)
6. Alliance engine (2-vs-1 triangle rotation, rhythmic sync/desync)
7. Husband voices: Oud (Palestinian), Bouzouki (Greek), Pin (Thai) — secondary waveguides
8. DRAMA threshold system (husbands enter at >0.7)
9. SIDES rotation (3 alliance configurations with smooth interpolation)
10. FX chains: Aunt 1 (OD+delay+comp+room), Aunt 2 (filter+stutter+phaser+spring), Aunt 3 (shimmer+chorus+delay+exciter)
11. Husband FX: Oud (tape echo), Bouzouki (chorus), Pin (soft reverb)
12. Macros (FUEGO, DRAMA, SIDES, ISLA)
13. UI (three-column + husband row + alliance triangle indicator)
14. 50 presets
15. DSP stability + auval + QA

---

### SP7: Pentagon Integration into XOmnibus

**Location:** `~/Documents/GitHub/XO_OX-XOmnibus/`
**Dependencies:** SP1-SP6 all complete

**Build phases:**
1. Write 5 adapters implementing `SynthEngine` interface
2. Register all 5 engines via `REGISTER_ENGINE()` macro
3. Add to CMakeLists.txt
4. Copy all presets to `Presets/XOmnibus/{mood}/`
5. Implement family coupling matrix (named relationship routes)
6. Implement cross-engine macro bleed system
7. Create Family Dinner preset category (30 multi-engine presets)
8. Implement Obed hidden engine mode (XOhm MEDDLING=1/COMMUNE=0)
9. Family Portrait visualization easter egg (if time permits)
10. Run Sonic DNA computation on all new presets
11. Update all documentation (CLAUDE.md, master spec, engine catalog, aquatic mythology, water column)
12. Update skills documentation to standards
13. Full QA pass: auval, DSP stability, CPU profiling, preset listening
14. UI/UX upscale pass across all 5 engine panels

---

## Parallel Opportunities

- SP2 (XOhm) and SP3 (XOrphica) can be built in parallel after SP1 (different exciters, independent repos)
- SP4 (XObbligato) and SP5 (XOttoni) can be built in parallel (both wind, but different exciters)
- SP6 (XOlé) must be last engine (most complex, benefits from all prior learning)
- SP7 depends on all engines but adapter writing can start as soon as each engine compiles

## Quality Gates

Each engine must pass before proceeding to integration:
- [ ] Builds clean (0 warnings with -Wall)
- [ ] auval passes (`auval -v aumu Xxxx Xoox`)
- [ ] All 4 macros produce audible change in every preset
- [ ] DSP stability: no NaN, no denormals, no clicks on note-on/off
- [ ] 50 presets in `.xometa` format across 6 moods
- [ ] CPU < 15% single engine on M1

---

*Approved by user 2026-03-14. Proceeding to implementation.*
