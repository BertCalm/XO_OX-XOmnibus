# XOceanDeep — Concept Brief

**Date:** March 12, 2026
**Phase:** 0 — Ideation complete
**Status:** Approved for Phase 1 architecture

---

## Identity

- **Name:** XOceanDeep
- **Gallery code:** OCEANDEEP
- **Accent color:** Trench Violet `#2D0A4E`
- **Parameter prefix:** `deep_`
- **Thesis:** Abyssal bass engine — the crushing pressure, bioluminescent creatures, sunken treasure, and 808 rumble of 10,000 leagues under the sea. The darkest sounds in the XO_OX universe.
- **Sound family:** Bass / Sub / Dark Texture / Creature FX
- **Unique capability:** Bass that feels like pressure — sub frequencies with the weight of the entire ocean above them. Creature sounds that dart and flash. Dark textures that creak like shipwrecks. The 808 reimagined as a geological event. Other engines go low. OceanDeep goes *under*.
- **Max voices:** 8
- **CPU budget:** <12%

---

## Character

- **Personality in 3 words:** Abyssal, Massive, Alien
- **Polarity:** Pure Oscar — the axolotl descends to the absolute floor
- **Engine approach:** Pressure synthesis — sub oscillator stack with waveguide body resonance (shipwreck/cave modeling), bioluminescent exciter stage (sharp transient flashes in the dark), creature modulation (darting LFO behaviors with random walk), and hydrostatic compression (dynamic pressure that increases weight with depth)
- **Why this approach serves the character:**
  The deep ocean isn't just "low frequency." It's pressure, darkness, alien biology, and ancient wreckage. The sub oscillator stack provides the 808-weight foundation. The waveguide body gives each bass voice a physical space — a shipwreck hull, a cave, a trench wall. The bioluminescent exciter adds sharp, bright transients that flash in the darkness like creature lures. The creature modulation makes sounds dart and bounce unpredictably, like freaky deep-sea animals bouncing off the walls of a wreck.

---

## The XO Concept Test

1. **XO word:** XOceanDeep ✓ — Ocean Deep: the absolute floor of the water column, Oscar's throne room
2. **One-sentence thesis:** "XOceanDeep is an abyssal bass engine where 808 rumble meets bioluminescent creatures in the crushing darkness of 10,000 leagues under the sea — sunken treasure that will shiver your timbers." ✓
3. **Sound only this can make:** Sub bass with physical pressure modeling — the frequency doesn't just go low, it feels heavy. Creature sounds that dart through the dark with bioluminescent flash transients. Shipwreck textures that creak and groan under pressure. No other XOlokun engine combines geological bass weight with alien deep-sea creature behavior. ✓

---

## Aquatic Identity

The ocean floor. Oscar's throne room. No light reaches here — the only illumination comes from creatures that make their own. Bioluminescent flashes in absolute darkness, anglerfish lures, the faint glow of volcanic vents on the horizon.

The pressure is immense. Every sound carries the weight of the entire water column above it. Down here, bass isn't just a frequency — it's a physical force. Sunken ships creak under the weight. Ancient creatures dart between the wreckage, bouncing off walls, flashing in panic.

The 808 kick down here isn't a drum hit. It's the heartbeat of the Earth.

---

## Gallery Gap Filled

| Existing engines | Bass capability |
|-----------------|----------------|
| XOverbite (BITE) | Character bass — Belly warmth + Bite feral |
| XObese (FAT) | Width bass — 13-oscillator wall |
| XOverdub (DUB) | Dub bass — sine + delay + space |
| **XOceanDeep** | **Pressure bass — sub weight + creature + darkness** |

No current engine models bass as *physical pressure with deep-sea creature behavior*. Overbite has character stages. Obese has oscillator count. OceanDeep has the weight of the ocean.

---

## Core DSP Architecture (Phase 0 sketch)

```
SUB OSCILLATOR STACK
├── Primary sub: sine / triangle / 808 shape
├── Secondary sub: one octave below, shaped pulse
├── Harmonic layer: saw/square for upper presence
├── Phase relationship control (for phase-aligned sub weight)
│
▼
HYDROSTATIC COMPRESSOR
├── Pressure-modeled dynamics — deeper = heavier transients
├── deep_pressure: 0 (surface dynamics) to 1 (abyssal crush)
├── Slow attack, heavy ratio — everything gets massive
│
▼
WAVEGUIDE BODY (environment resonance)
├── Shipwreck hull (metallic, creaking, enclosed)
├── Trench wall (stone, infinite, dark)
├── Cave (warm, resonant, Oscar's home)
├── Open abyss (vast, cold, no reflections)
├── Body size + damping + material controls
│
▼
BIOLUMINESCENT EXCITER
├── Sharp transient flash — bright burst in the dark
├── deep_flashRate: how often creatures flash (0 = dark, 1 = frenzy)
├── deep_flashBright: intensity of each flash
├── Adds high-frequency transient spikes to sub content
├── The only "light" in the engine — everything else is dark
│
▼
CREATURE MODULATION
├── Random-walk LFO (Brownian motion — darting behavior)
├── Targets: pitch, filter, pan, waveguide position
├── deep_creatureSpeed: how fast things dart (slow drift ↔ frantic)
├── deep_creatureDepth: how far they travel
├── Multiple independent creature paths (2-3 simultaneous)
│
▼
DARKNESS FILTER
├── Low-pass with resonance — the deep absorbs highs
├── Key-tracked (higher notes get more filtering — depth increases)
├── deep_darkness: 0 = some light bleeds through, 1 = total blackout
│
▼
FX CHAIN
├── Pressure Reverb (massive, dark, slow — the trench itself)
├── Creature Delay (short, darting echoes — bouncing off wreckage)
├── Rumble Generator (sub-20Hz content — felt not heard)
│
▼
Output (stereo)
```

---

## Parameter Namespace

All parameter IDs use `deep_` prefix. Key parameters:

| ID | Range | Description |
|----|-------|-------------|
| `deep_subShape` | 0-3 | Sub oscillator shape: sine / triangle / 808 / pulse |
| `deep_subOctave` | -2 to 0 | Sub octave offset |
| `deep_harmonicLevel` | 0-1 | Upper harmonic layer level |
| `deep_pressure` | 0-1 | Hydrostatic compressor — weight/crush |
| `deep_bodyType` | 0-3 | Waveguide: shipwreck / trench / cave / open abyss |
| `deep_bodySize` | 0-1 | Environment size |
| `deep_bodyDamp` | 0-1 | Material absorption |
| `deep_flashRate` | 0-1 | Bioluminescent flash frequency |
| `deep_flashBright` | 0-1 | Flash transient intensity |
| `deep_creatureSpeed` | 0-1 | Random-walk LFO speed (drift ↔ frantic) |
| `deep_creatureDepth` | 0-1 | Modulation depth of creature movement |
| `deep_darkness` | 0-1 | Low-pass darkness filter |
| `deep_filterReso` | 0-1 | Darkness filter resonance |
| `deep_reverbMix` | 0-1 | Pressure reverb wet |
| `deep_reverbSize` | 0-1 | Trench size |
| `deep_delayMix` | 0-1 | Creature delay wet |
| `deep_rumble` | 0-1 | Sub-20Hz rumble generator |
| `deep_attack` | 0.001-2s | Amp attack |
| `deep_decay` | 0.01-4s | Amp decay |
| `deep_sustain` | 0-1 | Amp sustain |
| `deep_release` | 0.01-8s | Amp release |

*Full parameter list defined in Phase 1 architecture.*

---

## Macro Mapping (M1-M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | PRESSURE | `deep_pressure` + `deep_darkness` + `deep_subOctave` | 0 = shallow, dynamic, some light. 1 = abyssal crush, total darkness, lowest octave. The depth dial. |
| M2 | CREATURE | `deep_creatureSpeed` + `deep_creatureDepth` + `deep_flashRate` | 0 = empty, still, dark water. 1 = frantic darting, flashing, alive with alien life. The life dial. |
| M3 | WRECK | `deep_bodyType` blend + `deep_bodySize` + `deep_delayMix` | 0 = open abyss, nothing to bounce off. 1 = deep inside a sunken ship, metallic echoes everywhere. The environment dial. |
| M4 | ABYSS | `deep_reverbSize` + `deep_reverbMix` + `deep_rumble` | 0 = close, tight, dry. 1 = infinite trench, sub-20Hz rumble, the Earth itself vibrating. The vastness dial. |

---

## Coupling Interface Design

### OCEANDEEP as Target (receiving)

| Coupling Type | What XOceanDeep Does | Musical Effect |
|---------------|---------------------|----------------|
| `AmpToFilter` | Amplitude drives darkness filter | Drum hits open a flash of light in the deep |
| `AudioToFM` | External audio FM-modulates sub oscillator | Any engine's output becomes creature movement in the bass |
| `EnvToMorph` | Envelope drives pressure amount | External dynamics control the crushing weight |

### OCEANDEEP as Source (sending)

| Target Engine | Coupling Type | Musical Effect |
|-------------|---------------|----------------|
| OPENSKY | AmpToFilter | Abyssal bass drives sky filter — pressure from below makes the sky shimmer |
| OVERDUB | getSample | Sub bass through dub echo — the deepest dub imaginable |
| ONSET | AmpToFilter | Bass pressure pumps drum filter — sub shakes the percussion |
| OVERBITE | getSample | Two deep predators layered — anglerfish x abyssal |
| OPAL | AudioToWavetable | Sub bass granulated into pressure particles |

### Signature Coupling Route
> **OCEANDEEP × OPENSKY** — "The Full Column" — abyssal 808 bass under euphoric shimmer. Oscar's floor under feliX's sky. The entire XO_OX mythology in one patch.

### Coupling types OCEANDEEP should NOT receive
- `AmpToChoke` — you don't silence the ocean floor
- `PitchToPitch` — sub bass needs precise tuning, external pitch creates mud

---

## Preset Strategy (Phase 0 sketch)

**80 presets at v1.0:**

| Category | Count | Character |
|----------|-------|-----------|
| 808 Abyss | 15 | 808 bass reimagined as geological events. Earth-shaking sub. |
| Sunken Treasure | 15 | Metallic, creaking, shipwreck resonances with sub foundation |
| Creature Feature | 15 | Darting, flashing, bioluminescent creature textures |
| Pressure Drop | 10 | Pure sub weight — minimal harmonics, maximum mass |
| Dark Ambient | 10 | Trench drones, rumble textures, abyssal stillness |
| Coupling Showcases | 10 | Designed for specific engine pairs (especially The Full Column) |
| Leviathan | 5 | Maximum everything — all macros up, all seats taken, the kraken wakes |

### Naming Convention
Names should feel like descending:
- "10,000 Leagues"
- "Sunken Cathedral"
- "Creature In The Dark"
- "The Pressure Down Here"
- "Oscar's Throne"
- "Shiver Me Timbers"
- "Bioluminescent"
- "The Floor"

---

## Visual Identity

- **Accent color:** Trench Violet `#2D0A4E`
- **UI concept:** Dark. The darkest UI in the XO_OX gallery. Near-black background with trench violet accents and occasional bioluminescent flashes (animated accent highlights on parameter changes). The panel should feel like looking into total darkness with faint light coming from below.
- **Color palette:** Near-black base, trench violet accents, bioluminescent cyan highlights for active states. The opposite of OpenSky in every way.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | **High** | Sub bass is THE foundation |
| Atmosphere | Medium | Dark ambient / trench drone presets |
| Entangled | **High** | The Full Column coupling showcase |
| Prism | Low | Nothing bright lives here |
| Flux | Medium | Creature movement presets feel unstable |
| Aether | **High** | Abyssal stillness, barely-there sub rumble |

---

## Decision Gate: Phase 0 → Phase 1

- [x] Concept brief written
- [x] XO word confirmed (XOceanDeep — the ocean floor, Oscar's throne room)
- [x] Gallery gap clear (no pressure-bass + creature engine exists)
- [x] Coupling partners defined (OPENSKY, OVERDUB, ONSET, OVERBITE, OPAL)
- [x] Aquatic mythology position (bottom of the water column)
- [x] Excited about the sound

**→ Proceed to Phase 1: Architecture**
*Invoke: `/new-xo-engine phase=1 name=XOceanDeep identity="Abyssal bass engine — 808 pressure, bioluminescent creatures, sunken treasure at 10,000 leagues" code=XOdp`*

---

*XO_OX Designs | Engine: OCEANDEEP | Accent: #2D0A4E | Prefix: deep_*
