# XOmnibus — Engine Expansion Roadmap

*Document version: 3.0 | March 2026 — Full audit: 20 engines integrated, XOstinato designed, XOcelot scaffolded*

---

## Current Gallery State (March 2026)

**20 engines integrated** in `Source/Engines/`, all with SynthEngine adapters. 1,595 factory presets across 6 moods.

### Volume 1 — Core Engines (Original Instruments)

| Code | Engine Dir | Source Instrument | Role | Lines | Status |
|------|-----------|------------------|------|-------|--------|
| SNAP | Snap | OddfeliX | Percussive/rhythmic Karplus-Strong | 635 | ✅ Integrated |
| MORPH | Morph | OddOscar | Wavetable pads | 795 | ✅ Integrated |
| DUB | Dub | XOverdub | FX send/return dub bus | 1,310 | ✅ Integrated |
| DRIFT | Drift | XOdyssey | Psychedelic pads, Climax | 1,435 | ✅ Integrated |
| BOB | Bob | XOblongBob | Warm fuzzy textures | 1,739 | ✅ Integrated |
| FAT | Fat | XObese | Width/thickness, 13-osc | 1,496 | ✅ Integrated |
| ONSET | Onset | XOnset | Algorithmic drum synthesis | 1,581 | ✅ Integrated |
| OVERWORLD | Overworld | XOverworld | Chip synthesis (NES/Genesis/SNES) | 509 | ✅ Integrated |
| OPAL | Opal | XOpal | Granular time-scatter | 3,095 | ✅ Integrated |
| BITE | Bite | XOpossum | Bass-forward character synth | 2,388 | ✅ Integrated |

### Volume 1.5 — Expansion Engines (Integrated)

| Code | Engine Dir | Source Instrument | Role | Lines | Status |
|------|-----------|------------------|------|-------|--------|
| ORBITAL | Orbital | XOrbital | Additive harmonic partials | 1,109 | ✅ Integrated |
| ORGANON | Organon | XOrganon | Metabolic/dissipative synthesis | 1,160 | ✅ Integrated |
| OUROBOROS | Ouroboros | XOuroboros | Self-feeding recursive synthesis | 1,095 | ✅ Integrated |
| OBSIDIAN | Obsidian | XObsidian | Phase distortion synthesis | 1,304 | ✅ Integrated |
| ORIGAMI | Origami | XOrigami | Fold dynamics / waveshaping | 1,594 | ✅ Integrated |
| ORACLE | Oracle | XOracle | Breakpoint function synthesis | 1,479 | ✅ Integrated |
| OBSCURA | Obscura | XObscura | Stiffness / physical string modeling | 1,393 | ✅ Integrated |
| OCEANIC | Oceanic | XOceanic | Wavetable/FM spectral separation | 1,424 | ✅ Integrated |
| OPTIC | Optic | XOptic | Visual modulation + AutoPulse | 732 | ✅ Integrated |
| OBLIQUE | Oblique | XOblique | Prismatic bounce / RTJ × Funk × Tame Impala | 1,146 | ✅ Integrated |

---

## Standalone Instrument Status

These are the full standalone instruments (AU + Standalone builds) that exist alongside their XOmnibus adapters:

| Instrument | Repo | Status | Files | Presets |
|-----------|------|--------|-------|---------|
| XOverdub | `~/Documents/GitHub/XOverdub/` | ✅ All protocols complete, auval passes | — | 40 |
| XOdyssey | `~/Documents/GitHub/XOdyssey/` | ✅ All 9 phases complete | 34 files | 10 hero |
| XOblongBob | `~/Documents/GitHub/XOblongBob/` | ✅ Build complete, auval passes | 72 files | 167 |
| XOverworld | `~/Documents/GitHub/XOverworld/` | ✅ DSP + UI complete, auval passes | 18 files | expanding to 120 |
| XOpossum | `~/Documents/GitHub/XOppossum/` | ✅ All 7 phases complete, auval passes | 53 files | 342 |
| XOpal | `~/Documents/GitHub/XOpal/` | 🔨 In progress (scaffold + Phase 1 arch) | 10 files | 100 target |
| XOcelot | `~/Documents/GitHub/XOcelot/` | 🔨 Scaffolded (Phase 1 arch complete) | 17 files | — |

---

## Engines In Development

### Engine: OCELOT — XOcelot

- **Gallery code:** OCELOT
- **Accent color:** Ocelot Tawny `#C5832B`
- **Parameter prefix:** `ocelot_`
- **Thesis:** Canopy-layered ecosystem synth — 4 interacting strata (Floor/Understory/Canopy/Emergent) with biome transformations (Jungle/Underwater/Winter)
- **Max voices:** 8
- **CPU budget:** <14%
- **Phase 1 architecture:** `Docs/xocelot_phase1_architecture.md` ✅
- **Concept brief:** `Docs/concepts/xocelot_concept_brief.md` ✅
- **Standalone repo:** `~/Documents/GitHub/XOcelot/` — 17 source files, ~1,853 lines (scaffold)
- **XOmnibus integration:** Engine dir exists but not yet wired to scaffold

**Current status:** Phase 1 architecture complete, standalone scaffold started. Needs Phase 2 (core DSP implementation).

**Coupling Matrix (OCELOT)**

| As Target | Source Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OCELOT receives | DUB | AudioToWavetable | Dub delay output chopped into Understory |
| OCELOT receives | ONSET | AmpToFilter | Drum hits open Canopy spectral filter |
| OCELOT receives | ONSET | RhythmToBlend | Kick pattern shifts strata balance |
| OCELOT receives | BOB | AudioToWavetable | Warm textures fed into Understory chop buffer |
| OCELOT receives | DRIFT | AudioToFM | Psychedelic sweeps FM-modulate Canopy oscillator |
| OCELOT receives | OVERWORLD | AudioToWavetable | Chip audio chopped through SP-1200 mangler |

| As Source | Target Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OCELOT sends | DUB | getSample | Ecosystem output through dub echo — tropical dub |
| OCELOT sends | OPAL | AudioToWavetable | Percussion textures granulated into clouds |
| OCELOT sends | FAT | getSample | Creature calls fattened into massive formant stacks |
| OCELOT sends | DRIFT | getSample | Ecosystem as starting point for DRIFT's journey |

---

## Engines Designed (Ready for Architecture/Build)

### Engine: OSTINATO — XOstinato

- **Gallery code:** OSTINATO
- **Accent color:** Firelight Orange `#E8701A`
- **Parameter prefix:** `osti_`
- **Thesis:** Communal drum circle engine — 8 seats, 12 world percussion instruments, hybrid physical modeling. The fire at the center of the circle. Multiculturalism, peace, unity, love, community, family.
- **Max voices:** 16 (2 per seat)
- **CPU budget:** <25%
- **Design doc:** `Docs/plans/2026-03-12-xostinato-design.md` ✅
- **Concept brief:** `Docs/concepts/xostinato_concept_brief.md` ✅
- **Standalone repo:** Not yet created

**Key architecture decisions:**
- 8 seats in a circle, any instrument in any seat (no tradition restrictions)
- 12 instruments × 3-4 articulations (48 total): Djembe, Dundun, Conga, Bongos, Cajón, Taiko, Tabla, Doumbek, Frame Drum, Surdo, Tongue Drum, Beatbox
- Hybrid synthesis: Exciter → Modal Membrane (6-8 resonators) → Waveguide Body → Radiation Filter
- Pattern system: 96 authentic patterns + live MIDI override with fade-back
- 4 macros: GATHER (sync), FIRE (energy), CIRCLE (inter-seat interaction), SPACE (environment)
- FX: Circle Spatial Engine → Fire Stage → Gathering Reverb → Pulse Compressor
- ~140 canonical parameters, 120 factory presets

**Current status:** Phase 0 complete — design approved. Ready for Phase 1 architecture.

**Invoke Phase 1:** `/new-xo-engine phase=1 name=XOstinato identity="Communal drum circle engine — 8 seats, 12 world percussion instruments, hybrid physical modeling" code=XOst`

**Coupling Matrix (OSTINATO)**

| As Target | Source Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OSTINATO receives | BITE | AmpToFilter | Bass amplitude opens Fire Stage — bass makes circle roar |
| OSTINATO receives | DRIFT | EnvToMorph | Climax bloom sweeps GATHER macro |
| OSTINATO receives | OVERWORLD | AudioToFM | Chip audio FM-modulates drum exciters — 8-bit percussion |
| OSTINATO receives | OPAL | AmpToFilter | Grain density modulates pattern density |
| OSTINATO receives | FAT | AmpToFilter | 13-osc amplitude drives FIRE macro |

| As Source | Target Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OSTINATO sends | DUB | getSample | Drum circle through dub tape echo — world dub |
| OSTINATO sends | BITE | AmpToFilter | Drum accents pump bass filter — rhythmic snarl |
| OSTINATO sends | OPAL | AudioToWavetable | Drum hits scattered into grain clouds |
| OSTINATO sends | DRIFT | AmpToFilter | Circle energy drives pad filter |
| OSTINATO sends | BOB | AmpToFilter | Drum dynamics shape warm texture |

---

## Volume 2 Engines (Future — Not Yet Started)

*Source: `Docs/XOmnibus_Master_Architecture- Volume 2.md.txt`*
*Review: `Docs/xomnibus_volume2_review.md`*

### Build Queue

| # | XO Name | Short Code | Core Concept | Est. CPU | Status |
|---|---------|------------|--------------|----------|--------|
| 1 | **XOscillum** | OSCIL | Psychoacoustic residue pitch — phantom fundamentals | <12% | Phase 1 spec ready |
| 2 | **XObliqua** | OBLIQ | Kinematic phase-time — clave attractors as waveshaping | <15% | Concept only |
| 3 | **XOccult** | OCCULT | 1D cellular automata — Wolfram rules as audio | <5% | Concept only |
| 4 | **XOblivion** | OBLIV | Electromagnetic hysteresis via pre-computed LUTs — magnetic memory, kinetic mass, Barkhausen crackle | <10% | Concept only |
| 5 | **XOntara** | ONTAR | Topological sympathetic resonance — 64-voice modal bank | <20% | Concept only (deferred) |

**Note:** XObliqua (OBLIQ, kinematic phase-time) is a different engine from XOblique (OBLIQUE, prismatic bounce) which is already integrated.

### Merged — OUROBOROS v2

XOBSESSION's best ideas (Mandelbrot c-plane navigation, Julia mode, DIVE parameter, Riemann sheet rotation) will be absorbed into a future OUROBOROS version update. See `Docs/xomnibus_volume2_review.md` Section F for merge plan.

### Killed

XOBOLIC — duplicate of ORGANON (metabolic/informational dissipative synthesis already implemented).

### Parked — Future Research

| Engine | Concept | Why Parked |
|--------|---------|------------|
| XOMEMBRA | 2D wave interference mesh → emergent polyrhythm | CPU-heavy 2D waveguide, research-grade DSP |
| XOGRAMA | 720-voice optical sine bank with laser aperture | 720 simultaneous oscillators, extreme engineering |
| XOSMOSIS | 1D computational fluid dynamics (Burgers' equation) | Document flags "CRITICAL CPU BOTTLENECK" |

XOFERRO has been upgraded and adopted as **XOblivion** (engine #4 in build queue). The key change: replace the Jiles-Atherton ODE solver with pre-computed 2D hysteresis LUTs, dropping CPU from research-grade bottleneck to <10%.

---

## Build Priority (as of March 2026)

```
NOW
├── XOcelot — Phase 2 build (scaffold exists, arch done)
├── XOstinato — Phase 1 architecture (design approved)
│
NEXT
├── XOstinato — Phase 2-7 build
├── XOscillum — Phase 0-1 (spec ready)
│
LATER
├── XObliqua — Phase 0+
├── XOccult — Phase 0+
├── XOblivion — Phase 0+
├── XOntara — Phase 0+ (deferred)
```

---

## Cross-Engine Preset Priority

Priority coupling showcase presets for newly added engines:

1. **OSTINATO × DUB** — World Dub — entire drum circle through Jamaican dub FX
2. **OSTINATO × OPAL** — Scattered Gathering — drum hits granulated into particle clouds
3. **ONSET × OSTINATO** — Machine Meets Human — algorithmic drums alongside physical circle
4. **OVERWORLD × OPAL** — Chip Scatter — NES audio granulated into time cloud
5. **BITE × OPAL** — Bass Breath — grain density pumping the bass filter
6. **DRIFT × OPAL** — Climax Particles — Climax bloom frozen mid-bloom
7. **OCELOT × DUB** — Tropical Dub — ecosystem output through tape echo/spring chain
8. **ONSET × OCELOT** — Jungle Pulse — drum hits driving canopy filter + strata balance
9. **OCELOT × OPAL** — Canopy Particles — kalimba/berimbau textures granulated into clouds
10. **OVERWORLD × OCELOT** — Chip Chop — chip audio bit-crushed through SP-1200 mangler
11. **OSTINATO × BITE** — Rhythm & Bass — drum circle accents pumping the bass filter
12. **OSTINATO × DRIFT** — Ceremony — circle energy breathing the psychedelic pads

---

## CPU Budget Estimates

| Configuration | Engines | Est. CPU |
|---------------|---------|----------|
| Current gallery max | 3 engines | ~47.5% |
| + OSTINATO | 4 engines | ~60% (voice reduce) |
| OSTINATO + DUB pair | 2 engines | ~30% |
| OSTINATO + OPAL | 2 engines | ~35% |
| OCELOT + OPAL | 2 engines | ~27% |
| OSTINATO solo | 1 engine | ~20-25% (8 seats active) |

OSTINATO is the heaviest single engine due to 8 simultaneous physical models. Eco mode recommendation: reduce to 1 voice per seat in 4-engine configs.

---

*Roadmap owner: XO_OX Designs | Process: `/new-xo-engine` skill | Next action: XOcelot Phase 2 or XOstinato Phase 1*
