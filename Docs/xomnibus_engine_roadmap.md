# XOlokun — Engine Expansion Roadmap

*Document version: 3.3 | March 2026 — 29+ engines registered (24 original + 5 Constellation: OHM/ORPHICA/OBBLIGATO/OTTONI/OLE + OMBRE); OVERLAP and OUTWIT are Phase 3 complete but not yet installed*

---

## Current Gallery State (March 2026)

**29+ engines registered** in XOlokun. **24 original engines** integrated in `Source/Engines/` with SynthEngine adapters (OSPREY, OSTERIA, and OWLFISH are registered but not listed in the volume tables below — they were added between Volume 1.5 and the Constellation wave). **5 Constellation family engines** (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) registered as stubs. **OMBRE** dual-narrative engine integrated. **2 engines** (OVERLAP and OUTWIT) are Phase 3 complete in standalone form but not yet registered in XOlokun. 2,369+ factory presets across 7 moods (Foundation/Atmosphere/Entangled/Prism/Flux/Aether/Family).

### Volume 1 — Core Engines (Original Instruments)

| Code | Engine Dir | Source Instrument | Role | Lines | Status |
|------|-----------|------------------|------|-------|--------|
| ODDFELIX | Snap | OddfeliX | Percussive/rhythmic Karplus-Strong | 635 | ✅ Integrated |
| ODDOSCAR | Morph | OddOscar | Wavetable pads | 795 | ✅ Integrated |
| OVERDUB | Dub | XOverdub | FX send/return dub bus | 1,310 | ✅ Integrated |
| ODYSSEY | Drift | XOdyssey | Psychedelic pads, Climax | 1,435 | ✅ Integrated |
| OBLONG | Bob | XOblongBob | Warm fuzzy textures | 1,739 | ✅ Integrated |
| OBESE | Fat | XObese | Width/thickness, 13-osc | 1,496 | ✅ Integrated |
| ONSET | Onset | XOnset | Algorithmic drum synthesis | 1,581 | ✅ Integrated |
| OVERWORLD | Overworld | XOverworld | Chip synthesis (NES/Genesis/SNES) | 509 | ✅ Integrated |
| OPAL | Opal | XOpal | Granular time-scatter | 3,095 | ✅ Integrated |
| OVERBITE | Bite | XOverbite (was XOpossum) | Bass-forward character synth | 2,388 | ✅ Integrated |

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
| OCEANIC | Oceanic | XOceanic | Paraphonic string ensemble + chromatophore pedalboard | 1,424 | ✅ Integrated |
| OPTIC | Optic | XOptic | Visual modulation + AutoPulse | 732 | ✅ Integrated |
| OBLIQUE | Oblique | XOblique | Prismatic bounce / RTJ × Funk × Tame Impala | 1,146 | ✅ Integrated |
| OCELOT | Ocelot | XOcelot | Canopy-layered ecosystem / sample mangling | 3,010 | ✅ Integrated |
| OMBRE | Ombre | XOmbre | Dual-narrative (memory/forgetting + perception) | ~800 | ✅ Integrated |

---

## Standalone Instrument Status

These are the full standalone instruments (AU + Standalone builds) that exist alongside their XOlokun adapters:

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

### Engine: OCELOT — XOcelot ✅ Integrated

- **Gallery code:** OCELOT
- **Accent color:** Ocelot Tawny `#C5832B`
- **Parameter prefix:** `ocelot_`
- **Thesis:** Canopy-layered ecosystem synth — 4 interacting strata (Floor/Understory/Canopy/Emergent) with biome transformations (Jungle/Underwater/Winter)
- **Max voices:** 8
- **XOlokun integration:** 3,010 lines in `Source/Engines/Ocelot/`
- **Standalone repo:** `~/Documents/GitHub/XOcelot/` — scaffold with Phase 1 architecture
- **Sound design guide:** Section not yet written (add to `xolokun_sound_design_guides.md`)

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

### Engine: OUIE — XOuïe

- **Gallery code:** OUIE
- **Accent color:** Hammerhead Steel `#708090`
- **Parameter prefix:** `ouie_`
- **Thesis:** Duophonic synthesis — two voices with selectable algorithms (8 options each), STRIFE↔LOVE bipolar interaction axis. The hammerhead shark at the thermocline.
- **Max voices:** 2 (duophonic) + 4 unison per voice = 8 oscillators max
- **CPU budget:** <8%
- **Concept brief:** `Docs/concepts/xouie_concept_brief.md` ✅
- **Standalone repo:** Not yet created

**Key architecture decisions:**
- 2 voices, each selects from 8 algorithms in smooth/rough clusters
- Smooth: VA, Wavetable, FM, Additive | Rough: Phase Dist, Wavefolder, KS, Noise
- HAMMER macro: bipolar STRIFE↔LOVE interaction axis (signature feature)
- Voice modes: Split (low/high), Layer (both on every note), Duo (true duophonic)
- 4 macros: HAMMER (interaction), AMPULLAE (sensitivity), CARTILAGE (flexibility), CURRENT (environment)
- ~65 canonical parameters, 80 factory presets
- Inspired by Arturia MiniFreak/MicroFreak architecture

**Current status:** Phase 0 complete — design approved. Ready for Phase 1 architecture.

**Invoke Phase 1:** `/new-xo-engine phase=1 name=XOuïe identity="Duophonic synthesis — two voices, two algorithms, one predator. Brotherly love and brotherly strife at the thermocline." code=XOui`

**Coupling Matrix (OUIE)**

| As Target | Source Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OUIE receives | DRIFT | EnvToMorph | Climax bloom sweeps HAMMER — journey destabilizes the brothers |
| OUIE receives | ONSET | AmpToFilter | Drum hits modulate filter — rhythmic predator |
| OUIE receives | FAT | AmpToFilter | Whale mass drives filter depth |
| OUIE receives | OPAL | AudioToFM | Grain particles FM-modulate Voice B |
| OUIE receives | OCEANIC | EnvToMorph | Spectral separation drives algorithm morph |

| As Source | Target Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OUIE sends | DUB | getSample | Duophonic output through dub echo |
| OUIE sends | OPAL | AudioToWavetable | Two-voice texture granulated into particles |
| OUIE sends | BITE | AmpToFilter | Shark's attack drives anglerfish's bite |
| OUIE sends | OCEANIC | getSample | Two-voice output spectrally separated |
| OUIE sends | DRIFT | getSample | Duophonic texture as Odyssey's starting point |

---

## Volume 2 Engines (Future — Not Yet Started)

*Source: `Docs/XOlokun_Master_Architecture- Volume 2.md.txt`*
*Review: `Docs/xolokun_volume2_review.md`*

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

XOBSESSION's best ideas (Mandelbrot c-plane navigation, Julia mode, DIVE parameter, Riemann sheet rotation) will be absorbed into a future OUROBOROS version update. See `Docs/xolokun_volume2_review.md` Section F for merge plan.

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

## Mythology Engines (The Bookends)

These two engines complete the XO_OX Aquatic Mythology — the top and bottom of the water column.
See `Docs/xo_ox_aquatic_mythology.md` for the full mythology.

### OPENSKY — XOpenSky

- **Gallery code:** OPENSKY
- **Accent color:** Sunburst `#FF8C00`
- **Parameter prefix:** `sky_`
- **Thesis:** Euphoric shimmer synth — supersaw anthems, crystalline pads, the soaring high above the water. Van Halen's jump, Vangelis's heaven.
- **Polarity:** Pure feliX — above the surface entirely
- **Max voices:** 16 | **CPU budget:** <10%
- **Concept brief:** `Docs/concepts/xopensky_concept_brief.md` ✅
- **Status:** Phase 0 complete — design approved

### OCEANDEEP — XOceanDeep

- **Gallery code:** OCEANDEEP
- **Accent color:** Trench Violet `#2D0A4E`
- **Parameter prefix:** `deep_`
- **Thesis:** Abyssal bass engine — 808 pressure, bioluminescent creatures, sunken treasure at 10,000 leagues under the sea.
- **Polarity:** Pure Oscar — the absolute ocean floor
- **Max voices:** 8 | **CPU budget:** <12%
- **Concept brief:** `Docs/concepts/xoceandeep_concept_brief.md` ✅
- **Status:** Phase 0 complete — design approved

### The Full Column Coupling
> **OPENSKY × OCEANDEEP** — The entire XO_OX mythology in one patch. feliX's sky over Oscar's floor. Euphoric shimmer over abyssal 808. The coupling that completes the universe.

---

## Build Priority (as of March 2026)

```
NOW
├── XOcelot — Phase 2 build (scaffold exists, arch done)
├── XOstinato — Phase 1 architecture (design approved)
│
NEXT
├── XOuïe — Phase 1 architecture (concept brief done)
├── XOpenSky — Phase 1 architecture (concept brief done)
├── XOceanDeep — Phase 1 architecture (concept brief done)
├── XOstinato — Phase 2-7 build
│
LATER
├── XOscillum — Phase 0-1 (spec ready)
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
13. **OPENSKY × OCEANDEEP** — The Full Column — feliX's sky over Oscar's floor, the entire mythology
14. **OPENSKY × OVERDUB** — Heavenly Dub — euphoric leads through Jamaican tape echo
15. **OCEANDEEP × OVERDUB** — The Deepest Dub — abyssal sub through dub delay
16. **OPENSKY × OPAL** — Light Particles — shimmer granulated into suspended crystals
17. **OCEANDEEP × ONSET** — Pressure Drums — 808 sub shaking the percussion from below
18. **OUIE × OCEANDEEP** — Apex Dive — duophonic leads over abyssal 808
19. **OUIE × OPENSKY** — Thermal Rising — two-voice texture erupting into euphoric shimmer
20. **OUIE × DUB** — Brothers Echo — duophonic argument echoing through dub tape delay
21. **OUIE × BITE** — Predator Meets Predator — hammerhead driving anglerfish's bite filter

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
