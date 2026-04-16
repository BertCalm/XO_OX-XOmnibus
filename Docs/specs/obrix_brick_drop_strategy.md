# OBRIX Brick Drop Strategy

**Filed**: 2026-03-19
**Identity**: OBRIX is the baby brother of XOceanus — a living reef that grows over time
**Mythology**: Baby coral polyp that builds its reef brick by brick. Reef Jade #1E8B7E.

---

## Core Concept

OBRIX ships with a "Standard Brick Kit" and grows through periodic brick drops — new oscillator types, processor types, effect types, and modulator types that plug into the existing architecture. Each brick = 5-40 lines of DSP. The fleet is the brick quarry.

## Architecture Contract

Adding a new brick requires ONLY:
1. New enum value (e.g., `Grain = 9` in ObrixSourceType)
2. New case statement in the relevant render function (5-40 lines)
3. New entry in the StringArray choices in addParametersImpl
4. No other code changes. No new parameters. No architectural modifications.

This contract must be maintained through all waves of development.

## Release Cadence

| Timeline | Drop | Contents | Effort |
|----------|------|----------|--------|
| Launch | Standard Kit | 8 sources, 6 procs, 5 mods, 4 effects | Included |
| Month 3 | Chaos Pack | Chaos osc + Chaos LFO + Saturator | ~40 LOC |
| Month 5 | Retro Pack | Chip osc + Bit Crusher + Tape Delay | ~40 LOC |
| Month 7 | Organic Pack | Grain osc + Metabolic osc + Spring Reverb | ~65 LOC |
| Month 9 | Deep Pack | Subharmonic osc + Formant Filter + Shimmer | ~65 LOC |
| Month 11 | Swarm Pack | Swarm mod + Automata mod + Biolume FX | ~55 LOC |
| Year 2+ | Ongoing | Community votes, seasonal, artist collabs | ~30 LOC/drop |

Each drop includes 3-4 bricks + 10-15 new presets + Field Guide blog post.

## Fleet as Brick Quarry

Every XOceanus engine contains DSP that can be miniaturized into an OBRIX brick:

### Source Bricks (from existing engines)
| Brick | Donor Engine | LOC | Description |
|-------|-------------|-----|-------------|
| Metabolic | ORGANON | ~15 | Free-energy oscillator |
| Chaos | OUROBOROS | ~15 | Strange attractor oscillator |
| Grain | OPAL | ~25 | Simplified single-grain source |
| Chip | OVERWORLD | ~10 | NES/Genesis/SNES waveforms |
| Subharmonic | OWLFISH | ~15 | Mixtur-Trautonium divider |
| Physical | ONSET | ~20 | Modal percussion strike |
| Formant | OSPREY | ~15 | Vocal formant oscillator |
| Stochastic | ORACLE | ~15 | GENDY-inspired random walk |

### Processor Bricks
| Brick | Donor | LOC | Description |
|-------|-------|-----|-------------|
| Saturator | OBESE | ~10 | Tube/tape/transistor drive |
| Comb Filter | OVERDUB | ~15 | Karplus-Strong resonator |
| Bit Crusher | OVERWORLD | ~8 | Sample rate/bit depth reducer |
| Formant Filter | OSPREY | ~20 | Vowel filter (A/E/I/O/U morph) |
| Phaser | fleet DSP | ~20 | Allpass chain |

### Effect Bricks
| Brick | Donor | LOC | Description |
|-------|-------|-----|-------------|
| Spring Reverb | OVERDUB (B004) | ~25 | Metallic splash reverb |
| Tape Delay | OVERDUB | ~20 | Degrading delay with wow/flutter |
| Shimmer | OPENSKY | ~30 | Pitch-shifted reverb |
| Bioluminescence | Aquatic FX | ~25 | Reactive glow effect |

### Modulator Bricks
| Brick | Donor | LOC | Description |
|-------|-------|-----|-------------|
| Swarm | OCEANIC | ~15 | Boid-separation mod source |
| Automata | OCTOPUS | ~15 | Cellular automata pattern |
| Euclidean | new | ~10 | Euclidean rhythm gate |
| Chaos LFO | OUROBOROS | ~10 | Lorenz attractor modulator |

**Flywheel**: Every future engine built also becomes a brick quarry. ORGANISM → Automata brick. OVERTONE → Continued Fractions brick. The fleet feeds OBRIX. OBRIX introduces users to the fleet.

## Collectible / Easter Egg System

- **Hidden bricks**: Discovered through specific parameter combinations (cheat codes)
- **Seasonal bricks**: Frost reverb (December), Monsoon delay (summer)
- **Community bricks**: Producers propose DSP, winners get implemented
- **Achievements**: "Used all 12 processor types" → unlocks cosmetic theme
- **Brick of the Month**: Field Guide / Patreon content per drop

## Visual Growth System

### The Reef Grows
- Day 1: Small, simple reef — 3-4 visible bricks. Baby coral.
- Month 3: First drop — new bricks swim in, attach to structure. Reef grows.
- Month 9: Lush reef — dozens of types, glowing rare bricks, animated creatures.
- Year 2: The reef IS the user's synthesis journey. Every discovery visualized.

### Visual Treatments by Rarity
- **Standard bricks**: Solid Reef Jade tiles, clean geometry
- **Rare / Easter egg**: Bioluminescent glow, subtle pulse animation
- **Seasonal**: Unique color treatments (frost blue, coral pink, etc.)
- **Fleet-derived**: Tinted with donor engine's accent color
  - Chaos source → hint of OUROBOROS Strange Attractor Red #FF2D2D
  - Grain source → hint of OPAL Lavender #A78BFA
  - Chip source → hint of OVERWORLD Neon Green #39FF14
  - Spring Reverb → hint of OVERDUB Olive #6B7B3A

### Animations
- **Active connections**: Thin light lines between bricks, pulsing with audio
- **SURPRISE**: Reef dissolves → bricks scatter like startled fish → reform
- **Journey mode**: Whole reef sways like coral in deep current
- **New brick arrival**: Swims in from edge of screen, finds its place

## Mythology Position

OBRIX is not engine #39. It is the reef itself — the living structure all creatures inhabit.

| Entity | Role |
|--------|------|
| 38 engines | Mature creatures of the reef — fixed identities |
| OBRIX | The reef structure — grows from every creature's contribution |

In the water column atlas, OBRIX spans all depth zones as substrate, not a single creature at a single depth. The baby brother that becomes the foundation everything else lives on.

## Business Model Integration

- **Patreon**: "Patrons get new bricks 2 weeks early" — perfect tier content
- **Premium boutique**: Curated brick packs with artist-designed presets
- **Community**: Brick voting keeps engagement between major releases
- **Reduced burnout**: 10-line case statements with huge perceived value
- **Content pipeline**: Each drop = blog post + presets + lore entry
