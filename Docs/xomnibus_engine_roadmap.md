# XOmnibus — Engine Expansion Roadmap
*Gallery Additions: BITE (XOpossum) + OPAL (XOpal) → then Volume 2 engines*
*Document version: 2.0 | March 2026*

---

## Gallery State at Roadmap Start

| Code | Source | Role | Status |
|------|--------|------|--------|
| SNAP | XOddCouple X | Percussive/rhythmic | Integrated |
| MORPH | XOddCouple O | Wavetable pads | Integrated |
| DUB | XOverdub | FX send/return bus | Integrated |
| DRIFT | XOdyssey | Psychedelic pads, Climax | Integrated |
| BOB | XOblongBob | Warm fuzzy textures | Integrated |
| FAT | XObese | Width/thickness, 13-osc | Integrated |
| ONSET | XOnset (spec) | Dedicated drums | Not built |
| OVERWORLD | XOverworld | Chip synthesis, 6 engines | Integration ready |

**Gaps being filled:**
- No bass-character engine (BITE fills this)
- No time/granular synthesis (OPAL fills this)

---

## Engine 1: BITE — XOpossum

### Identity
- **Gallery code:** BITE
- **Source instrument:** XOpossum
- **Accent color:** Moss Green `#4A7C59` (plush + feral duality)
- **Thesis:** Bass-forward character synth where plush weight meets feral bite
- **Parameter prefix:** `poss_`
- **Max voices:** 16
- **CPU budget:** <10% (single engine)

### Why BITE Next
The gallery has width (FAT), pads (MORPH, DRIFT), character texture (BOB), percussion (SNAP), and FX routing (DUB). It has no dedicated bass voice with character stages. BITE fills the bottom of the frequency spectrum with something that has *personality* — not just a subtractive bass but a living one.

**Key coupling route unlocked:**
> **FAT → BITE** — 13 oscillators stacked through XOpossum's Fur→Gnash→Trash character stages. The widest bass in the gallery.
> **ONSET → BITE** — Snare envelope drives the Bite macro. Every drum hit makes the bass snarl.

### Build Phases

#### Phase 0: COMPLETE
Planning done. Design doc at `~/Documents/GitHub/XOppossum/docs/plans/2026-03-06-xopossum-design.md`.

#### Phase 1 — Parameter Architecture
- All 122 canonical parameter IDs locked with `poss_` prefix
- `AppState` JSON schema, `PresetManager` scaffold
- **Gate:** compiles, save→load round-trips

#### Phase 2 — Core Voice Engine
- OscA (4 belly waveforms), OscB (5 bite waveforms)
- Weight/Sub engine with low-band compensation
- NoiseSource (5 types + routing modes)
- Voice pool: mono/legato/duo/poly4/unison
- Amp envelope
- **Gate:** audible tone, voice modes work, low end holds

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
- Write `src/adapter/XOpossum Adapter.h` against SynthEngine interface
- Verify coupling: AmpToFilter, AudioToFM, AmpToChoke supported
- Write integration spec (`docs/xomnibus_integration_spec.md`)
- Test coupling routes with FAT, ONSET, SNAP locally

#### Phase 4.X — Gallery Install
- Copy DSP headers to `XO_OX-XOmnibus/Source/Engines/BITE/`
- `REGISTER_ENGINE(XOpossumAdapter)`
- Copy presets to XOmnibus Presets directory
- Run `compute_preset_dna.py`
- Design 10 cross-engine Entangled presets
- Update gallery docs

### Coupling Matrix (BITE)

| As Target | Source Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| BITE receives | FAT | AmpToFilter | 13-osc width drives filter — massive animated bass |
| BITE receives | ONSET | AmpToFilter | Drum hits pump the Bite macro — snare makes bass snarl |
| BITE receives | SNAP | AudioToFM | Karplus-Strong pluck FM-modulates the bass |
| BITE receives | OVERWORLD | AudioToFM | Chip audio frequency-modulates bass operators |

| As Source | Target Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| BITE sends | DUB | getSample | Bass through dub echo/spring chain |
| BITE sends | FAT | getSample | Bass amplitude modulates FAT width |

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
> **DRIFT → OPAL** — The Climax bloom granulated into a cloud. Psychedelic pads fragmented.
> **OPAL → DUB** — The grain cloud through the dub echo/spring chain. Granular dub.

### Build Phases

#### Phase 0 — Ideation: COMPLETE (this document)
Concept brief at `Docs/concepts/xopal_concept_brief.md`.

#### Phase 1 — Architecture: To Do
Full architecture spec, parameter list, signal flow design.
Run Synth Architect Protocol via `/new-xo-engine phase=1`.

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
| OPAL receives | DRIFT | AudioToWavetable | Psychedelic pads granulated — Climax bloom frozen into particles |
| OPAL receives | BOB | AudioToWavetable | Warm fuzzy textures scattered through time |
| OPAL receives | SNAP | AudioToWavetable | Karplus-Strong pluck granulated — reverb made of its own attack |
| OPAL receives | FAT | AmpToFilter | 13-osc amplitude modulates grain density |

| As Source | Target Engine | Type | Musical Effect |
|-----------|--------------|------|----------------|
| OPAL sends | DUB | getSample | Grain cloud through dub echo/spring |
| OPAL sends | MORPH | EnvToMorph | Grain envelope drives wavetable morph position |
| OPAL sends | BITE | AmpToFilter | Cloud density drives bass filter — breathing bass |

---

## Combined Timeline

```
Q2 2026
┌─────────────────────────────────────────────────────────┐
│ BITE Phase 1-2  ████████                                 │
│ OPAL Phase 0-1      ████████                             │
└─────────────────────────────────────────────────────────┘

Q3 2026
┌─────────────────────────────────────────────────────────┐
│ BITE Phase 3-5  ████████████                             │
│ OPAL Phase 2         ████████████████                    │
└─────────────────────────────────────────────────────────┘

Q4 2026
┌─────────────────────────────────────────────────────────┐
│ BITE Phase 6-7  ████████                                 │
│ BITE Integration        ████                             │
│ OPAL Phase 2 (cont.)   ████████████                     │
└─────────────────────────────────────────────────────────┘

Q1 2027
┌─────────────────────────────────────────────────────────┐
│ BITE Gallery Install  ██                                  │
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
└─────────────────────────────────────────────────────────┘

Q3 2028
┌─────────────────────────────────────────────────────────┐
│ XOblivion Phase 3-4  ████████                             │
│ XOblivion Gallery Install     ████                        │
└─────────────────────────────────────────────────────────┘

Q4 2028+
┌─────────────────────────────────────────────────────────┐
│ XOntara Phase 0-4 (if pipeline proven)  ████████████████ │
└─────────────────────────────────────────────────────────┘
```

---

## CPU Budget With Both Added

| Configuration | Engines | Est. CPU |
|---------------|---------|----------|
| Current gallery max | 3 engines | ~47.5% |
| + BITE | 4 engines | ~55% (voice reduce) |
| + OPAL | 4 engines | ~57% (voice reduce) |
| BITE + OPAL pair | 2 engines | ~22% |
| OVERWORLD + OPAL | 2 engines | ~27% (heaviest pair) |
| All 4 (BITE+OPAL+OVERWORLD+DUB) | 4 engines | ~49% with mitigation |

OPAL is granular — at 12 grain cloud voices, CPU is variable by density parameter. Eco mode recommendation: cap grain density at 30/sec in 4-engine configs.

---

## Cross-Engine Preset Priority (Gallery Opening)

When both engines are installed, these are the priority cross-engine presets:

1. **OVERWORLD × OPAL** — Chip Scatter — NES audio granulated into time cloud
2. **BITE × OPAL** — Bass Breath — grain density pumping the bass filter
3. **DRIFT × OPAL** — Climax Particles — Climax bloom frozen mid-bloom
4. **FAT × BITE** — Maximum Gravity — 13 oscillators through feral character
5. **OVERWORLD × BITE** — Chip Bass — FM chip audio FM-modulating the bass
6. **OPAL × DUB** — Granular Dub — grain cloud through tape echo + spring
7. **ONSET × BITE** — Living Texture — snare envelope driving Bite macro

---

## Volume 2 Engines (Post BITE + OPAL)

*Source: `Docs/XOmnibus_Master_Architecture- Volume 2.md.txt`*
*Review: `Docs/xomnibus_volume2_review.md`*

### Adopted — Build Queue

| # | Vol 2 Name | XO Name | Short Code | Core Concept | Est. CPU | Status |
|---|---|---|---|---|---|---|
| 1 | XOSCILLUM | **XOscillum** | OSCIL | Psychoacoustic residue pitch — phantom fundamentals | <12% | Phase 1 spec ready |
| 2 | XOBLICUA | **XObliqua** | OBLIQ | Kinematic phase-time — clave attractors as waveshaping | <15% | Concept only |
| 3 | XOMATON | **XOccult** | OCCULT | 1D cellular automata — Wolfram rules as audio | <5% | Concept only |
| 4 | XOFERRO (upgraded) | **XOblivion** | OBLIV | Electromagnetic hysteresis via pre-computed LUTs — magnetic memory, kinetic mass, Barkhausen crackle | <10% | Concept only |
| 5 | XOTARA | **XOntara** | ONTAR | Topological sympathetic resonance — 64-voice modal bank | <20% | Concept only (deferred to 2028+) |

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

*Roadmap owner: XO_OX Designs | Process: `/new-xo-engine` skill | Next action: Begin BITE Phase 1*
