# XOsprey — Concept Brief

**Date:** March 2026
**Phase:** 0 — Ideation
**Status:** Draft

---

## Identity

- **Name:** XOsprey
- **Gallery code:** OSPREY
- **Thesis:** XOsprey is a turbulence-modulated resonator engine that transforms the open ocean into an instrument — from glassy calm harbors to thunderous gales — where folk resonances, creature voices, and fluid dynamics collide into sounds impossible with any other synthesis method.
- **Sound family:** Texture / Pad / Ambient / Cinematic / Hybrid
- **Unique capability:** A fluid energy model where sea state IS the core synthesis parameter, a shore system where coastlines are timbral regions, and creature voices that emerge from the turbulence. No other engine models an environment as the instrument — every other engine is a sound source *in* a space; XOsprey makes the space itself the source.

---

## Character

- **Personality in 3 words:** Vast, Turbulent, Longing
- **Engine approach:** Turbulence-modulated resonator network — a fluid dynamics energy model excites tuned resonator banks (folk instrument partials) and formant gesture generators (creature voices), with coherence/decorrelation controlled by a sea state parameter
- **Why this approach serves the character:**
  The ocean is not one sound — it's a system of coupled forces (wind, current, swell, surface tension) acting on resonant objects (hulls, rigging, rocks, air cavities). A turbulence-modulated resonator network captures this exactly: the fluid model generates energy patterns, the resonators give that energy musical voice, and the sea state parameter controls how orderly or chaotic the coupling between them is. Calm seas produce shimmering, correlated harmonics (sunlight on water). Storms produce decorrelated, roaring spectral chaos (whitecaps, spray, thunder). The fisherman's boat is literally a resonant body being excited by fluid forces.
- **The coupling thesis:**
  XOsprey's turbulence model can modulate any engine's parameters — imagine OBSIDIAN's crystal lead being buffeted by ocean forces, or ORIGAMI's spectral folds driven by wave patterns. Conversely, any engine can excite XOsprey's resonators — ONSET drum hits as thunder cracks, ODDFELIX plucks as rigging snaps.

---

## The XO Concept Test

1. **XO word:** XOsprey — the osprey (sea hawk) hunts by plunging from calm sky into turbulent water. It bridges two fluid domains, sees through chaos to the signal beneath. The osprey's dive IS the calm-to-storm arc.
2. **One-sentence thesis:** "XOsprey is a turbulence-modulated resonator engine that transforms the open ocean — from glassy calm to thunderous gale — into an instrument where folk resonances and creature voices emerge from fluid dynamics."
3. **Sound only this can make:** A shimmering guitarra portuguesa resonance gradually consumed by ocean turbulence — the partials decorrelate, spectral spray builds, whale call formants rise from the deep, and then the storm passes and the shimmer returns, changed. No combination of oscillators, filters, or effects can replicate this because the turbulence model produces continuously novel energy patterns that are physically coherent (not random noise — actual fluid behavior).

---

## Gallery Gap Filled

| Existing Coverage | What It Does |
|-------------------|-------------|
| OCEANIC | Bottom-up emergence (128 autonomous particles) |
| OUROBOROS | Mathematical chaos (strange attractors, ODEs) |
| ORGANON | Metabolic evolution (biological energy systems) |
| OBSCURA | Physical object simulation (mass-spring chain) |
| **OSPREY** | **Environmental dynamics — fluid turbulence as instrument** |

The gallery has chaos, emergence, metabolism, and physics — but no engine that models a **dynamic environment**. OCEANIC's particles are autonomous agents; OSPREY's turbulence is a physical force field. OCEANIC asks "what do the particles want to do?" OSPREY asks "what is the ocean doing to everything in it?"

**Key distinction from OCEANIC:** OCEANIC is bottom-up (particles follow rules, sound emerges). OSPREY is top-down (you control the sea state, everything responds). OCEANIC models agents. OSPREY models forces.

**Key distinction from XOSMOSIS (parked):** XOSMOSIS attempted literal 1D CFD and was parked for "hostile timbre, limited melodic range." OSPREY solves this by using the fluid model as an *energy source and modulator* rather than an oscillator. The resonator bank provides musical pitch and timbre; the fluid model provides energy, movement, and character. The fluid doesn't make the sound — it shapes it.

---

## Core DSP Architecture (Phase 0 Sketch)

```
Sea State Controller
├── Storm: 0.0 (glassy calm) → 1.0 (full gale)
├── Swell Period: wave rhythm (0.5-30 seconds)
├── Wind Direction: spectral tilt bias
└── Depth: subsurface vs. surface energy ratio
        │
        ▼
Shore Selector (osprey_shore)
├── Atlantic: long rolling swells, guitarra/kora/uilleann resonators
├── Nordic: deep slow waves, hardingfele/langspil resonators
├── Mediterranean: short choppy seas, bouzouki/oud/ney resonators
├── Pacific: vast gentle swells, koto/conch/singing bowl resonators
└── Southern: warm rolling rhythm, cavaquinho/valiha/gamelan resonators
        │
        ▼
Fluid Energy Model (simplified turbulent flow)
├── Laminar mode: correlated sinusoidal energy curves
├── Transitional: periodic + noise injection
├── Turbulent: Perlin noise + spectral cascade
├── Shore-specific: swell shape, chop frequency, depth profile
└── Output: N energy streams (amplitude, frequency mod, phase perturbation)
        │
        ├──────────────────────┐
        ▼                      ▼
Resonator Bank               Creature Voice Generator
(Shore-Selected Partials)     (Shore-Selected Formants)
├── 16 tuned resonators       ├── Gull/Tern/Petrel: sharp formant sweeps
├── Shore selects model set:  ├── Whale species: slow deep formant arcs
│   ├── Atlantic: guitarra,   ├── Wind character: per-shore spectral shape
│   │   kora, uilleann        ├── Bell: harbor buoy, ship bell, temple bell
│   ├── Nordic: hardingfele,  └── Custom: user-defined formant path
│   │   langspil, kulning           │
│   ├── Mediterranean:              │
│   │   bouzouki, oud, ney          │
│   ├── Pacific: koto,              │
│   │   conch, singing bowl         │
│   └── Southern: cavaquinho,       │
│       valiha, gamelan             │
├── Excitation: fluid energy        │
├── Damping: sea state modulated    │
└── Coupling: partials decorrelate  │
    as storm increases              │
        │                           │
        └────────────┬──────────────┘
                     ▼
              Mix Stage
              ├── Resonator / Creature balance
              ├── Depth layering (surface vs. deep)
              └── Stereo field (wave direction → pan motion)
                     │
                     ▼
              Filter (Cytomic SVF — tilt EQ modeling water absorption)
                     │
                     ▼
              Character Stage
              ├── Foam: high-frequency saturation (whitecap texture)
              ├── Brine: subtle bitcrushing (salt crystal granularity)
              └── Hull: cabinet/body resonance (the boat)
                     │
                     ▼
              FX Chain
              ├── Tide Delay: tempo-synced delay with swell-modulated time
              ├── Harbor Reverb: large diffuse space with ocean-floor reflections
              └── Fog: gentle high-frequency rolloff + stereo smear
                     │
                     ▼
              Output (stereo)
```

**Voice model:** Each MIDI note tunes the resonator bank to that pitch. The fluid model runs globally (shared across voices). Up to 8 simultaneous voices. Legato mode available (smooth sea state transitions between notes).

**Shore system:** `osprey_shore` is a continuous parameter (0.0–4.0), not a discrete switch. Integer values are pure shores; fractional values crossfade between adjacent shore models — resonator partials morph, creature voices blend, fluid character interpolates. This means you can "sail" from the Atlantic to the Mediterranean in a single macro sweep, or automate the shore parameter for presets that journey between coastlines.

**CPU strategy:** The fluid energy model is lightweight (Perlin noise + spectral shaping, not actual Navier-Stokes). The resonators are modal filters (cheap per-sample). Creature voices are formant filters with envelope-driven sweeps. Shore morphing uses crossfaded coefficient sets (no extra oscillators). Estimated <10% CPU single-engine at 44.1kHz / 512 block on M1.

---

## Parameter Namespace

All parameter IDs use `osprey_` prefix. Key parameters:

| ID | Range | Description |
|----|-------|-------------|
| `osprey_shore` | 0-4 | Coastline region: Atlantic / Nordic / Mediterranean / Pacific / Southern. Selects resonator model family, creature voice set, and fluid character (swell shape, chop frequency, depth profile). Morphable — intermediate values blend adjacent shores. |
| `osprey_seaState` | 0.0-1.0 | Master calm→storm control. THE parameter. |
| `osprey_swellPeriod` | 0.5-30s | Wave rhythm — modulates resonator energy cyclically |
| `osprey_windDir` | 0-360° | Spectral tilt bias (which resonators receive more energy) |
| `osprey_depth` | 0.0-1.0 | Surface vs. subsurface energy balance |
| `osprey_resonatorModel` | 0-5 | Folk instrument within current shore (overrides shore default) |
| `osprey_resonatorBright` | 0.0-1.0 | Resonator high partial emphasis |
| `osprey_resonatorDecay` | 0.01-8s | How long resonators ring |
| `osprey_sympathyAmount` | 0.0-1.0 | Cross-resonator sympathetic coupling (hardingfele-inspired) |
| `osprey_creatureType` | 0-4 | Creature voice selection (gull/whale/wind/bell/custom) |
| `osprey_creatureRate` | 0.0-1.0 | How often creature voices trigger |
| `osprey_creatureDepth` | 0.0-1.0 | Creature voice intensity |
| `osprey_coherence` | 0.0-1.0 | Partial correlation (1=locked, 0=fully decorrelated) |
| `osprey_foam` | 0.0-1.0 | High-frequency saturation (whitecap texture) |
| `osprey_brine` | 0.0-1.0 | Subtle degradation (salt crystal granularity) |
| `osprey_hull` | 0.0-1.0 | Body resonance amount (the boat) |
| `osprey_filterTilt` | -1.0-1.0 | Water absorption EQ (negative=dark/deep, positive=bright/surface) |
| `osprey_tideDelay` | 0.0-1.0 | Swell-synced delay mix |
| `osprey_harborVerb` | 0.0-1.0 | Large space reverb mix |
| `osprey_fog` | 0.0-1.0 | HF rolloff + stereo smear |
| `osprey_ampAttack` | 0.001-8s | Envelope attack |
| `osprey_ampDecay` | 0.05-8s | Envelope decay |
| `osprey_ampSustain` | 0.0-1.0 | Envelope sustain |
| `osprey_ampRelease` | 0.05-12s | Envelope release (often very long — the ocean doesn't stop) |

*Full parameter list defined in Phase 1 architecture.*

---

## Macro Mapping (M1-M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | SEA STATE | `osprey_seaState` + `osprey_coherence` (inverse) + `osprey_foam` | The one knob. 0=glass harbor at dawn. 0.5=rolling Atlantic swell. 1.0=North Sea gale. Every preset transforms completely across this range. |
| M2 | MOVEMENT | `osprey_swellPeriod` + `osprey_creatureRate` + `osprey_sympathyAmount` | The life in the water. Slow=barely breathing. Fast=churning, crying, ringing. Controls the rhythm of the ocean without changing its intensity. |
| M3 | COUPLING | `osprey_depth` + `osprey_creatureDepth` + coupling amount when coupled | The vertical dimension. What's happening beneath the surface? Whale calls rising, deep currents pushing. In coupled mode, controls how much external engine audio excites the resonators. |
| M4 | SPACE | `osprey_harborVerb` + `osprey_fog` + `osprey_tideDelay` | How far away are we? Close=intimate boat creaking. Far=vast open ocean with fog rolling in. |

All 4 macros produce audible, dramatic change at every point in their range in every preset.

---

## Cultural Lens: Pan-Maritime

### The Ocean Connects All Coasts

XOsprey draws from maritime folk traditions worldwide — not as a sampler of world music, but because the ocean taught every coastal culture similar musical lessons. The wave is a universal teacher. Rolling rhythms, drone-based harmony, call-and-response with the sea, instruments that shimmer like sunlight on water.

### Primary Traditions

**Portuguese Fado** — The emotional anchor. Saudade (the longing for something absent) IS the engine's emotional core. The guitarra portuguesa with its 12 paired courses creates natural shimmer — two strings per course, never perfectly in tune, producing beating patterns like light on water. Fado was born in Lisbon's Alfama district among sailors and dockworkers. The fisherman in XOsprey's concept IS a fadista.

**Cape Verdean Morna** — Cesária Évora's maritime melancholy bridges Portugal and West Africa. The morna's gentle rolling rhythm maps to XOsprey's swell period. The cavaquinho's bright attack cutting through warm vocal pads maps to high resonator partials emerging from turbulent low-frequency energy. Morna proves that maritime longing transcends any single culture.

**Nordic Hardingfele** — The Norwegian Hardanger fiddle has 4-5 sympathetic strings beneath the bowed strings. They ring in response to the played notes, creating shimmering harmonic halos — exactly what XOsprey's sympathetic resonator coupling does. The hardingfele tradition is rooted in Norwegian coastal and fjord communities. Its sound IS sunlight on still water.

**Celtic Uilleann Pipes** — The bag is literally a fluid pressure system. The piper controls air flow (fluid energy) through reed-excited resonators (chanters and drones). The uilleann pipes' simultaneous drone + melody + regulators maps to XOsprey's layered resonator + creature voice + fluid energy architecture. Sean-nós singing's ornamented vocal line evokes the gull cry formant gestures.

**West African Kora** — The 21-string bridge harp of the Mandé people. Kora arpeggios have a rolling, wave-like quality — the two hand planes create interlocking patterns that suggest the rise and fall of ocean swells. Toumani Diabaté's solo recordings sound like the ocean playing itself.

**East Asian Singing Bowls & Gamelan** — Metallic resonance traditions from Tibet, Nepal, Java, Bali. Struck metal resonators produce complex inharmonic spectra that evolve over long decays — physical objects ringing in response to impulse excitation. This IS scanned resonance, and it maps directly to XOsprey's resonator bank being excited by fluid energy impulses.

### The Shore System — Coastlines as Synthesis Regions

The `osprey_shore` parameter isn't a preset selector — it's a continuous synthesis dimension. Each shore defines a complete sonic ecosystem:

| Shore | Value | Swell Character | Resonator Family | Creature Voices | Emotional Quality |
|-------|-------|----------------|-----------------|----------------|-------------------|
| **Atlantic** | 0.0 | Long, rolling, powerful swells. Deep troughs. | Guitarra portuguesa (shimmer), kora (rolling arps), uilleann pipes (drone + chanter) | Humpback whale (deep melodic arcs), Atlantic storm petrel (sharp cries), harbor foghorn | Saudade — vast longing, the sailor's departure |
| **Nordic** | 1.0 | Deep, slow, heavy waves. Fjord reflections. | Hardingfele (sympathetic strings), langspil (bowed drone), kulning (herding call formants) | Beluga whale (high chirps), Arctic tern (piercing cry), ice cracking (impulse) | Stark beauty — midnight sun on still fjord water |
| **Mediterranean** | 2.0 | Short, choppy, bright seas. Quick energy. | Bouzouki (metallic shimmer), oud (warm pluck), ney flute (breathy resonance) | Mediterranean gull (laughing call), dolphin clicks (rapid impulses), cicada drone (summer texture) | Warm melancholy — Aegean blue, ancient harbors |
| **Pacific** | 3.0 | Vast, gentle, immense swells. Slow period. | Koto (precise harmonics), conch shell (horn resonance), singing bowl (metallic sustain) | Pacific humpback (longest whale songs), albatross (low soaring call), reef crackling (biophonic texture) | Oceanic vastness — horizon in every direction |
| **Southern** | 4.0 | Warm rolling rhythm. Trade wind consistency. | Cavaquinho (bright attack), valiha (bamboo tube resonance), gamelan (inharmonic metallic partials) | Southern right whale (deep boom), tropicbird (sharp descending cry), tropical rain (broadband patter) | Equatorial warmth — Cesária Évora's barefoot melancholy |

**Morphing between shores:** At `osprey_shore` = 1.5, you're halfway between Nordic and Mediterranean — hardingfele sympathetic strings blending with bouzouki metallics, beluga chirps morphing into dolphin clicks, deep fjord swells shortening into Aegean chop. This creates thousands of hybrid coastlines that exist nowhere on Earth but feel geographically coherent.

**Shore as coupling target:** When another engine modulates `osprey_shore` via coupling, the coastline itself becomes a dynamic parameter. OUROBOROS chaos slowly drifting the shore value creates a boat lost at sea, passing through unfamiliar waters. OVERWORLD's chip LFO rapidly switching shores creates a radio scanning between coastal stations.

### Instrument Heritage

- **Guitarra Portuguesa** (Lisbon/Coimbra, 18th century) — 12-string pear-shaped guitar. The paired courses create natural chorus and shimmer. The Coimbra style uses a darker tuning; the Lisbon style is brighter. Both evoke the Tagus river mouth meeting the Atlantic.
- **Hardingfele** (Norway, 17th century) — Hardanger fiddle with 4-5 sympathetic understrings. The sympathetic resonance creates a shimmering halo around every note — sound reflecting off still fjord water.
- **Uilleann Pipes** (Ireland, early 18th century) — bellows-driven Irish bagpipes. The bag as fluid pressure reservoir, reeds as excitation, chanter as resonator. A complete fluid→resonator→sound system.
- **Kora** (West Africa, 13th century Mandé Empire) — 21-string bridge harp. Rolling arpeggios that suggest ocean swells. The calabash body resonates like a boat hull.
- **Cristal Baschet** (France, 1952) — glass rods amplified through flame-shaped metal radiators. Wet fingertips excite glass (fluid→resonator coupling). Sound emerges from the metal radiator's resonance, not the glass itself — the glass is just the excitation mechanism.
- **Waterphone** (Richard Waters, 1968, USA) — a stainless steel resonator with bronze rods, partially filled with water. Bowing or striking the rods while tilting the instrument causes water to shift inside, continuously modulating the resonant frequencies. Literally a water-modulated resonator.
- **Aeolian Harp** (ancient Greece → Romantic era) — strings excited by wind. The original turbulence-modulated resonator. Different wind speeds excite different harmonics. XOsprey's fluid energy model is a digital Aeolian harp.

---

## Coupling Interface Design

### OSPREY as Target (receiving from other engines)

| Coupling Type | What XOsprey Does | Musical Effect |
|---------------|-------------------|----------------|
| `AudioToFM` | Source audio excites the resonator bank as additional energy | ONSET drum hits as thunder cracks. ODDFELIX plucks as rigging snaps in wind. |
| `AmpToFilter` | Source amplitude modulates the sea state | Another engine's dynamics control the ocean. Loud=storm, quiet=calm. Rhythmic pumping of the weather. |
| `EnvToMorph` | Source envelope shapes swell period | External rhythm drives wave motion. |
| `LFOToPitch` | Source LFO modulates resonator tuning | Cross-engine pitch drift — the boat pitching in someone else's waves. |
| `AudioToWavetable` | Source audio replaces internal resonator excitation | Any engine's output heard THROUGH the ocean. OBSIDIAN crystal filtered by water. ODYSSEY bloom submerged. |

**Primary coupling:** `AudioToFM` — external audio as ocean energy. Any engine can become a storm.

### OSPREY as Source (sending to other engines)

`getSampleForCoupling()` returns: post-filter resonator output, stereo, normalized ±1.

| Target Engine | Coupling Type | Musical Effect |
|---------------|---------------|----------------|
| OBSIDIAN | AmpToFilter | Ocean energy modulates crystal stiffness — the sea shapes the crystal |
| ORIGAMI | AudioToWavetable | Ocean resonance enters FFT — spectral seascape folding |
| OPAL | AudioToWavetable | Ocean audio granulated — waves shattered into time particles |
| OVERDUB | getSample | Ocean resonance through dub echo chain — maritime dub |
| OCEANIC | AudioToFM | Ocean forces drive swarm attractor — the sea herds the flock |
| ORACLE | AmpToFilter | Wave energy modulates GENDY barriers — ocean constrains chance |

### Coupling types OSPREY should NOT receive

| Type | Why |
|------|-----|
| `AmpToChoke` | Choking the ocean makes no musical sense — the sea doesn't stop |
| `PitchToPitch` | The resonator bank's tuning is already MIDI-controlled; external pitch coupling creates mud |

---

## Visual Identity

- **Accent color:** Azulejo Blue `#1B4F8A`
  - The deep blue of Portuguese ceramic tiles (azulejos) — the most recognizable visual symbol of Portuguese maritime culture
  - Distinguishes from existing blues: Electric Blue `#0066FF` (ONSET), Bioluminescent Cyan `#00CED1` (ORGANON), Phosphorescent Teal `#00B4A0` (OCEANIC)
  - Deeper, more muted — the blue of deep water, not neon surface
- **Material/texture:** Glazed ceramic — the hand-painted surface of azulejo tiles. Imperfect, warm, human-made. Ocean scenes painted on tile (a Portuguese tradition).
- **Gallery panel character:** The warm white gallery shell frames a deep blue ceramic panel. The panel surface should evoke hand-painted tile — slightly textured, with thin white decorative borders suggesting wave patterns. Subtle animation: gentle undulating wave motion in the background, speed tied to swell period.
- **Icon concept:** A stylized osprey in mid-dive — wings swept back, entering water. Rendered in azulejo blue and white tile aesthetic. Simple enough for 32px favicon.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | Medium | Calm harbor presets with stable resonator tuning can anchor a mix |
| Atmosphere | **High** | The ocean IS atmosphere. Deep, evolving, spatial, immersive. |
| Entangled | **High** | Coupling-focused presets where other engines excite XOsprey's resonators, or XOsprey's turbulence modulates other engines. |
| Prism | Medium | Bright surface presets with high sympathetic resonance and creature voices can sparkle |
| Flux | **High** | Storm-transition presets — the calm-to-gale arc is pure flux |
| Aether | **High** | Subsurface deep-ocean presets. Whale calls in vast reverb. Near-silent underwater resonance. |

Primary moods: Atmosphere, Entangled, Flux, Aether.

---

## Preset Strategy (Phase 0 Sketch)

**150 presets at v1.0:**

| Category | Count | Character |
|----------|-------|-----------|
| Harbor Dawn | 20 | Calm, shimmering folk resonances. Guitarra shimmer, kora arpeggiation, singing bowl overtones. Foundation/Atmosphere. |
| Open Water | 20 | Moderate swell, rolling motion. Hardingfele sympathetic halos, uilleann drone. Movement without violence. Atmosphere/Flux. |
| Storm Wall | 20 | Full turbulence. Decorrelated resonators, spectral spray, roaring foam. Thunder creature voices. Flux. |
| Creature Choir | 20 | Whale calls, gull cries, wind voices prominent. Deep formant arcs. Aether/Atmosphere. |
| The Crossing | 20 | Full calm-to-storm-to-calm arc via M1. Narrative presets designed to be performed. Flux/Prism. |
| Submerged | 15 | Underwater perspective. Heavy depth parameter, muffled resonators, slow deep swell. Aether. |
| Coupling Showcases | 20 | Designed for specific engine pairs: OBSIDIAN×OSPREY (crystal sea), ONSET×OSPREY (thunder drums), ORIGAMI×OSPREY (folded waves), etc. Entangled. |
| Maritime Folk | 15 | Closest to traditional instrument sound. Specific resonator models prominent. Clean, playable, melodic. Foundation/Prism. |

---

## What Makes This Different From XOSMOSIS (Parked)

XOSMOSIS was parked for "hostile timbre, limited melodic range." XOsprey avoids both problems:

1. **The fluid model is an energy source, not an oscillator.** XOSMOSIS tried to make CFD output directly into audio — this produces harsh, noisy, unmusical signals. XOsprey uses the fluid model to *excite and modulate* a resonator bank that provides musical pitch and timbre. The ocean doesn't sing — the instruments in the ocean sing.

2. **Melodic range comes from the resonators.** MIDI note input tunes the resonator bank, not the fluid model. Every note plays a pitched, musical tone. The fluid model controls *how* that tone behaves (calm shimmer vs. turbulent chaos), not *what pitch* it is.

3. **The turbulence is a macro, not a parameter.** XOSMOSIS exposed raw fluid dynamics parameters. XOsprey abstracts turbulence into the intuitive "sea state" knob (M1). Producers don't need to understand Reynolds numbers — they turn a knob from "calm" to "storm."

---

## Decision Gate: Phase 0 → Phase 1

- [x] Concept brief written
- [x] XO word feels right (XOsprey — the sea hawk bridging calm and chaos)
- [x] Gallery gap is clear (no environmental dynamics engine)
- [x] At least 2 coupling partner ideas (OBSIDIAN, ORIGAMI, ONSET, OPAL, OCEANIC, OVERDUB, ORACLE)
- [x] Unique capability defined (turbulence-modulated resonator network)
- [x] Cultural lens defined (pan-maritime folk traditions)
- [x] Distinguished from parked XOSMOSIS
- [ ] Excited about the sound ← **your call**

**→ Pending approval for Phase 1: Architect**

---

*XO_OX Designs | Engine: OSPREY | Accent: #1B4F8A | Prefix: osprey_*
