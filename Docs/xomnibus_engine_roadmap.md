# XOmnibus — Engine Expansion Roadmap
*Gallery Additions: OVERBITE (XOverbite) + OPAL (XOpal) → then Volume 2 engines*
*Document version: 2.1 | March 2026 — Added XOcelot (OCELOT) to Volume 2 queue*

---

## Gallery State at Roadmap Start

| Code | Source | Role | Status |
|------|--------|------|--------|
| ODDFELIX | OddfeliX | Percussive/rhythmic | Integrated |
| ODDOSCAR | OddOscar | Wavetable pads | Integrated |
| OVERDUB | XOverdub | FX send/return bus | Integrated |
| ODYSSEY | XOdyssey | Psychedelic pads, Climax | Integrated |
| OBLONG | XOblong | Warm fuzzy textures | Integrated |
| OBESE | XObese | Width/thickness, 13-osc | Integrated |
| ONSET | XOnset (spec) | Dedicated drums | Not built |
| OVERWORLD | XOverworld | Chip synthesis, 6 engines | Integration ready |

**Gaps being filled:**
- No bass-character engine (OVERBITE fills this)
- No time/granular synthesis (OPAL fills this)

---

## Engine 1: OVERBITE — XOverbite

### Identity
- **Gallery code:** OVERBITE
- **Source instrument:** XOverbite
- **Accent color:** Fang White `#F0EDE8`
- **Thesis:** Bass-forward character synth where plush weight meets feral bite
- **Parameter prefix:** `poss_`
- **Max voices:** 16
- **CPU budget:** <10% (single engine)

### Why OVERBITE Next
The gallery has width (OBESE), pads (ODDOSCAR, ODYSSEY), character texture (OBLONG), percussion (ODDFELIX), and FX routing (OVERDUB). It has no dedicated bass voice with character stages. OVERBITE fills the bottom of the frequency spectrum with something that has *personality* — not just a subtractive bass but a living one.

**Key coupling route unlocked:**
> **OBESE → OVERBITE** — 13 oscillators stacked through XOverbite's Fur→Gnash→Trash character stages. The widest bass in the gallery.
> **ONSET → OVERBITE** — Snare envelope drives the Bite macro. Every drum hit makes the bass snarl.

### Build Phases

#### Phase 0: COMPLETE
Planning done. Design doc at `~/Documents/GitHub/XOverbite/docs/plans/2026-03-06-xoverbite-design.md`.

#### Phase 1 — Parameter Architecture: COMPLETE
- All 122 canonical parameter IDs locked with `poss_` prefix
- `AppState` JSON schema, `PresetManager` scaffold
- Reference: `Docs/xoverbite_parameter_architecture.md`
- Hero preset: `Presets/XOmnibus/Foundation/Belly_Growl.xometa` (122 params, round-trip verified)
- **Gate:** compiles, save→load round-trips ✓

#### Phase 2 — Core Voice Engine: COMPLETE
- OscA shape parameter + analog drift; OscB shape + instability
- Weight engine (5 shapes, 3 octaves, fine tune)
- NoiseSource (5 types: White/Pink/Brown/Crackle/Hiss, 4 routing modes)
- Osc Interaction (4 modes: Soft Sync, Low FM, Phase Push, Grit Multiply)
- 3 LFOs (7 shapes each: +Random, +Stepped), Mod envelope (3rd ADSR)
- Voice: glide (Legato/Always), velocity sensitivity, pan
- Macro 5 (Play Dead): release extend, level duck, filter close
- Filter key tracking + pre-filter drive
- **Gate:** audible tone, voice modes work, low end holds ✓

#### Phase 3 — Filter + Character Stages
- FilterBlock: Burrow LP / Snarl BP / Wire HP / Hollow Notch
- Chew (post-filter contour), Drive (internal overdrive)
- Fur (pre-filter soft saturation)
- Gnash (post-filter asymmetric bite)
- Trash (dirt modes: rust / splatter / fold / crushed)
- **Gate:** plush-to-feral sweep audible, low end holds under drive

#### Phase 4 — Modulation + Macros
- Filter envelope, mod envelope, LFO1, LFO2
- 8-slot mod matrix
- 5 macros: Belly, Bite, Scurry, Trash, Play Dead (with easing curves)
- OscInteraction: soft sync, low FM, phase push, grit multiply
- **Gate:** macros produce layered tonal movement

#### Phase 5 — FX Chain
- Motion: plush chorus / uneasy doubler / oil flange
- Echo: dark tape / murky digital / short slap / ping
- Space: burrow room / fog chamber / drain hall
- Finish: glue / clip / width / low mono focus
- **Gate:** bass integrity preserved through entire FX chain

#### Phase 6 — Presets
- 10 hero presets first (one per macro combo extreme)
- Fill to 150 presets in `.xometa` format
- Category: Foundation (40), Atmosphere (25), Entangled (20), Prism (25), Flux (25), Aether (15)
- **Gate:** all macros respond, deterministic load, DNA computed

#### Phase 7 — UI + Polish
- Dark charcoal base, cream/moss/olive, selective rust/acid accents
- 5 pages: Main / Osc+Filter / Mod / FX / Browser
- Full QA pass
- **Gate:** spec Section 21 Definition of Done criteria met

#### Phase 3.X — XOmnibus Integration Prep
*After Phase 7*
- Write `src/adapter/XOverbiteAdapter.h` against SynthEngine interface
- Verify coupling: AmpToFilter, AudioToFM, AmpToChoke supported
- Write integration spec (`docs/xomnibus_integration_spec.md`)
- Test coupling routes with OBESE, ONSET, ODDFELIX locally

#### Phase 4.X — Gallery Install
- Copy DSP headers to `XO_OX-XOmnibus/Source/Engines/Overbite/`
- `REGISTER_ENGINE(XOverbiteAdapter)`
- Copy presets to XOmnibus Presets directory
- Run `compute_preset_dna.py`
- Design 10 cross-engine Entangled presets
- Update gallery docs

### Coupling Matrix (OVERBITE)

| As Target | Source Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OVERBITE receives | OBESE | AmpToFilter | 13-osc width drives filter — massive animated bass |
| OVERBITE receives | ONSET | AmpToFilter | Drum hits pump the Bite macro — snare makes bass snarl |
| OVERBITE receives | ODDFELIX | AudioToFM | Karplus-Strong pluck FM-modulates the bass |
| OVERBITE receives | OVERWORLD | AudioToFM | Chip audio frequency-modulates bass operators |

| As Source | Target Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OVERBITE sends | OVERDUB | getSample | Bass through dub echo/spring chain |
| OVERBITE sends | OBESE | getSample | Bass amplitude modulates OBESE width |

---

## Engine 2: OPAL — XOpal

### Identity
- **Gallery code:** OPAL
- **Source instrument:** XOpal (new — not yet built)
- **Accent color:** Iridescent Lavender `#A78BFA`
- **Thesis:** Granular synthesis engine that fragments any sound into time-scattered particles — from smooth stretched clouds to shattered glass
- **Parameter prefix:** `opal_`
- **Max voices:** 12 grain clouds (polyphonic — each note = one cloud)
- **CPU budget:** <12%

### Why OPAL Second
Every existing XOmnibus engine synthesizes *harmonically* — oscillators, wavetables, FM operators. None synthesizes in the *time domain*. OPAL introduces a fundamentally new synthesis dimension: granular time manipulation.

**Key coupling route unlocked:**
> **OVERWORLD → OPAL** — Chip audio (NES pulses, FM operators, SNES samples) enters the OPAL grain buffer. Real-time granulation of retro synthesis. This coupling doesn't exist anywhere else.
> **ODYSSEY → OPAL** — The Climax bloom granulated into a cloud. Psychedelic pads fragmented.
> **OPAL → OVERDUB** — The grain cloud through the dub echo/spring chain. Granular dub.

### Build Phases

#### Phase 0 — Ideation: COMPLETE (this document)
Concept brief at `Docs/concepts/xopal_concept_brief.md`.

#### Phase 1 — Architecture: COMPLETE
- 86 frozen `opal_` parameter IDs across 16 categories
- Full signal flow: grain buffer → scheduler → 32-grain pool → filter → character → FX
- Grain scheduler algorithm with deterministic PRNG
- Coupling interface: `AudioToWavetable` primary, 6 supported types
- 10 preset archetypes with DNA values, "Glass Cloud" first-encounter preset
- 4 macros: SCATTER, DRIFT, COUPLING, SPACE
- CPU verified at <1.1% worst case (well under 12% budget)
- Reference: `Docs/xopal_phase1_architecture.md`
- **Gate:** all params frozen, coupling defined, signal flow complete ✓

#### Phase 2 — Scaffold + Build
- `/new-xo-project name=XOpal identity="..." code=XOpl`
- 5 build phases (see XOpal Architecture Doc for detail)
- Coupling input from day one — `opal_couplingLevel` routes external audio into grain buffer
- All params `opal_` prefixed
- Presets in `.xometa` from day one

#### Phase 3.X — XOmnibus Integration
- Adapter implementing `AudioToWavetable` coupling type (external audio → grain buffer)
- Test OVERWORLD → OPAL coupling specifically
- Integration spec at `docs/xomnibus_integration_spec.md`

#### Phase 4.X — Gallery Install
- Copy to `XO_OX-XOmnibus/Source/Engines/OPAL/`
- 10 coupling-showcase Entangled presets (especially OVERWORLD×OPAL)
- Update gallery docs

### Coupling Matrix (OPAL)

| As Target | Source Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OPAL receives | OVERWORLD | AudioToWavetable | Chip audio → grain buffer. NES/FM/SNES fragmented into time clouds. |
| OPAL receives | ODYSSEY | AudioToWavetable | Psychedelic pads granulated — Climax bloom frozen into particles |
| OPAL receives | OBLONG | AudioToWavetable | Warm fuzzy textures scattered through time |
| OPAL receives | ODDFELIX | AudioToWavetable | Karplus-Strong pluck granulated — reverb made of its own attack |
| OPAL receives | OBESE | AmpToFilter | 13-osc amplitude modulates grain density |

| As Source | Target Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OPAL sends | OVERDUB | getSample | Grain cloud through dub echo/spring |
| OPAL sends | ODDOSCAR | EnvToMorph | Grain envelope drives wavetable morph position |
| OPAL sends | OVERBITE | AmpToFilter | Cloud density drives bass filter — breathing bass |

---

## Combined Timeline

```
Q2 2026
┌─────────────────────────────────────────────────────────┐
│ OVERBITE Phase 1-2  ████████                                 │
│ OPAL Phase 0-1      ████████                             │
└─────────────────────────────────────────────────────────┘

Q3 2026
┌─────────────────────────────────────────────────────────┐
│ OVERBITE Phase 3-5  ████████████                             │
│ OPAL Phase 2         ████████████████                    │
└─────────────────────────────────────────────────────────┘

Q4 2026
┌─────────────────────────────────────────────────────────┐
│ OVERBITE Phase 6-7  ████████                                 │
│ OVERBITE Integration        ████                             │
│ OPAL Phase 2 (cont.)   ████████████                     │
└─────────────────────────────────────────────────────────┘

Q1 2027
┌─────────────────────────────────────────────────────────┐
│ OVERBITE Gallery Install  ██                                  │
│ OPAL Phase 3-4   ████████████                            │
│ OPAL Gallery Install          ████                       │
│ XOscillum Phase 0-1              ████████                 │
└─────────────────────────────────────────────────────────┘

Q2 2027
┌─────────────────────────────────────────────────────────┐
│ XOscillum Phase 2  ████████████████                       │
│ XObliqua Phase 0-1             ████████                   │
└─────────────────────────────────────────────────────────┘

Q3 2027
┌─────────────────────────────────────────────────────────┐
│ XOscillum Phase 3-4  ████████                             │
│ XObliqua Phase 2        ████████████████                  │
│ XOccult Phase 0-1                   ████████              │
└─────────────────────────────────────────────────────────┘

Q4 2027
┌─────────────────────────────────────────────────────────┐
│ XOscillum Gallery Install  ██                             │
│ XObliqua Phase 3-4    ████████                            │
│ XOccult Phase 2           ████████████████                │
│ OUROBOROS v2 planning (absorb XOBSESSION concepts)  ████ │
└─────────────────────────────────────────────────────────┘

Q1 2028
┌─────────────────────────────────────────────────────────┐
│ XObliqua Gallery Install  ██                              │
│ XOccult Phase 3-4    ████████                             │
│ XOccult Gallery Install       ████                        │
│ XOblivion Phase 0-1               ████████                │
└─────────────────────────────────────────────────────────┘

Q2 2028
┌─────────────────────────────────────────────────────────┐
│ XOblivion Phase 2  ████████████████                       │
│ XOcelot Phase 0-1              ████████                   │
└─────────────────────────────────────────────────────────┘

Q3 2028
┌─────────────────────────────────────────────────────────┐
│ XOblivion Phase 3-4  ████████                             │
│ XOblivion Gallery Install     ████                        │
│ XOcelot Phase 2          ████████████████                  │
└─────────────────────────────────────────────────────────┘

Q4 2028
┌─────────────────────────────────────────────────────────┐
│ XOcelot Phase 2 (cont.)  ████████                         │
│ XOcelot Phase 3-4            ████████████                  │
└─────────────────────────────────────────────────────────┘

Q1 2029
┌─────────────────────────────────────────────────────────┐
│ XOcelot Gallery Install  ████                             │
│ XOntara Phase 0-4 (if pipeline proven)  ████████████████ │
└─────────────────────────────────────────────────────────┘
```

---

## CPU Budget With Both Added

| Configuration | Engines | Est. CPU |
|---------------|---------|----------|
| Current gallery max | 3 engines | ~47.5% |
| + OVERBITE | 4 engines | ~55% (voice reduce) |
| + OPAL | 4 engines | ~57% (voice reduce) |
| OVERBITE + OPAL pair | 2 engines | ~22% |
| OVERWORLD + OPAL | 2 engines | ~27% (heaviest pair) |
| All 4 (OVERBITE+OPAL+OVERWORLD+OVERDUB) | 4 engines | ~49% with mitigation |

OPAL is granular — at 12 grain cloud voices, CPU is variable by density parameter. Eco mode recommendation: cap grain density at 30/sec in 4-engine configs.

---

## Cross-Engine Preset Priority (Gallery Opening)

When both engines are installed, these are the priority cross-engine presets:

1. **OVERWORLD × OPAL** — Chip Scatter — NES audio granulated into time cloud
2. **OVERBITE × OPAL** — Bass Breath — grain density pumping the bass filter
3. **ODYSSEY × OPAL** — Climax Particles — Climax bloom frozen mid-bloom
4. **OBESE × OVERBITE** — Maximum Gravity — 13 oscillators through feral character
5. **OVERWORLD × OVERBITE** — Chip Bass — FM chip audio FM-modulating the bass
6. **OPAL × OVERDUB** — Granular Dub — grain cloud through tape echo + spring
7. **ONSET × OVERBITE** — Living Texture — snare envelope driving Bite macro
8. **OCELOT × OVERDUB** — Tropical Dub — ecosystem output through tape echo/spring chain
9. **ONSET × OCELOT** — Jungle Pulse — drum hits driving canopy filter + strata balance
10. **OCELOT × OPAL** — Canopy Particles — kalimba/berimbau textures granulated into clouds
11. **OVERWORLD × OCELOT** — Chip Chop — chip audio bit-crushed through SP-1200 mangler

---

## Volume 2 Engines (Post OVERBITE + OPAL)

*Source: `Docs/XOmnibus_Master_Architecture- Volume 2.md.txt`*
*Review: `Docs/xomnibus_volume2_review.md`*

### Adopted — Build Queue

| # | Vol 2 Name | XO Name | Short Code | Core Concept | Est. CPU | Status |
|---|---|---|---|---|---|---|
| 1 | XOSCILLUM | **XOscillum** | OSCIL | Psychoacoustic residue pitch — phantom fundamentals | <12% | Phase 1 spec ready |
| 2 | XOBLICUA | **XObliqua** | OBLIQ | Kinematic phase-time — clave attractors as waveshaping | <15% | Concept only |
| 3 | XOMATON | **XOccult** | OCCULT | 1D cellular automata — Wolfram rules as audio | <5% | Concept only |
| 4 | XOFERRO (upgraded) | **XOblivion** | OBLIV | Electromagnetic hysteresis via pre-computed LUTs — magnetic memory, kinetic mass, Barkhausen crackle | <10% | Concept only |
| 5 | — | **XOcelot** | OCELOT | Canopy-layered ecosystem synth — 4-strata cross-feeding (physical percussion + sample mangler + spectral pads + formant creatures) with biome system (Jungle/Underwater/Winter). Aesop Rock × Tropicália × rainforest. | <14% | Phase 0 concept brief |
| 6 | XOTARA | **XOntara** | ONTAR | Topological sympathetic resonance — 64-voice modal bank | <20% | Concept only (deferred to 2028+) |

### XOcelot Details

- **Gallery code:** OCELOT
- **Source instrument:** XOcelot (new — not yet built)
- **Accent color:** Ocelot Tawny `#C5832B`
- **Thesis:** Canopy-layered ecosystem synth — 4 interacting strata (Floor/Understory/Canopy/Emergent) with biome transformations
- **Parameter prefix:** `ocelot_`
- **Max voices:** 8
- **CPU budget:** <14% (4-strata synthesis + cross-feed matrix + biome transforms)
- **Concept brief:** `Docs/concepts/xocelot_concept_brief.md`

**Why OCELOT belongs in the queue:**
No engine models *interaction between internal synthesis layers* as the primary instrument. OCELOT fills the ecosystem/organic groove gap — physical-modeled Tropicália percussion, Aesop Rock-style sample mangling, spectral pads, and formant creature calls, all cross-feeding. The biome system (Jungle/Underwater/Winter) unlocks DKC-inspired Aether territory. High coupling potential with every existing engine.

**Coupling Matrix (OCELOT)**

| As Target | Source Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OCELOT receives | DUB | AudioToWavetable | Dub delay output chopped into Understory — dub meets hip-hop chop |
| OCELOT receives | ONSET | AmpToFilter | Drum hits open Canopy spectral filter — rhythmic brightness |
| OCELOT receives | ONSET | RhythmToBlend | Kick pattern shifts strata balance — jungle breathes with the beat |
| OCELOT receives | BOB | AudioToWavetable | Warm fuzzy textures fed into Understory chop buffer — dusty warmth |
| OCELOT receives | DRIFT | AudioToFM | Psychedelic sweeps FM-modulate Canopy oscillator |
| OCELOT receives | OVERWORLD | AudioToWavetable | Chip audio chopped and bit-crushed through SP-1200 mangler |

| As Source | Target Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OCELOT sends | DUB | getSample | Ecosystem output through dub echo — tropical dub |
| OCELOT sends | OPAL | AudioToWavetable | Percussion textures granulated into clouds — kalimba particles |
| OCELOT sends | FAT | getSample | Creature calls fattened into massive formant stacks |
| OCELOT sends | DRIFT | getSample | Ecosystem as starting point for DRIFT's journey |

### Merged — OUROBOROS v2

XOBSESSION's best ideas (Mandelbrot c-plane navigation, Julia mode, DIVE parameter, Riemann sheet rotation) will be absorbed into a future OUROBOROS version update. See `Docs/xomnibus_volume2_review.md` Section F for merge plan.

### Killed

XOBOLIC — duplicate of ORGANON (metabolic/informational dissipative synthesis already implemented).

### Parked — Future Research

| Engine | Concept | Why Parked |
|---|---|---|
| XOMEMBRA | 2D wave interference mesh → emergent polyrhythm | CPU-heavy 2D waveguide, research-grade DSP |
| XOGRAMA | 720-voice optical sine bank with laser aperture | 720 simultaneous oscillators, extreme engineering |
| XOSMOSIS | 1D computational fluid dynamics (Burgers' equation) | Document flags "CRITICAL CPU BOTTLENECK" |

XOFERRO has been upgraded and adopted as **XOblivion** (engine #4 in build queue). The key change: replace the Jiles-Atherton ODE solver with pre-computed 2D hysteresis LUTs, dropping CPU from research-grade bottleneck to <10%.

Remaining parked concepts are preserved in the Volume 2 source document for future consideration.

---

*Roadmap owner: XO_OX Designs | Process: `/new-xo-engine` skill | Next action: Begin OVERBITE Phase 1*
