# XOmnibus — Engine Expansion Roadmap
*Next 2 Gallery Additions: OVERBITE (XOverbite) + OPAL (XOpal)*
*Document version: 1.0 | March 2026*

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

---

*Roadmap owner: XO_OX Designs | Process: `/new-xo-engine` skill | Next action: Begin OVERBITE Phase 1*
