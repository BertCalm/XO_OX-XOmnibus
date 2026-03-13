# XOcelot — Concept Brief

**Date:** March 2026
**Phase:** 1 — Architecture complete
**Status:** Ready for Phase 2 scaffold + build

---

## Identity

- **Name:** XOcelot
- **Gallery code:** OCELOT
- **Thesis:** XOcelot is a canopy-layered sample-mangling synthesizer that builds dense, living soundscapes from interacting strata — floor percussion, understory chop, canopy texture, and emergent creature calls — like a sonic rainforest where every layer feeds the others.
- **Sound family:** Textural beat-making / Organic groove / Psychedelic hybrid
- **Unique capability:** Four-strata ecosystem synthesis where layers cross-pollinate in real time. The Floor drives rhythm into the Canopy. The Canopy filters down into the Understory. Creature calls emerge from spectral analysis of the combined output. No other engine treats *interaction between synthesis layers* as the primary instrument — in XOcelot, the ecosystem IS the sound.

---

## Character

- **Personality in 3 words:** Lush, Prowling, Hypnotic
- **Engine approach:** Multi-strata ecosystem synthesis — physical-modeled percussion + sample-mangling + spectral pads + formant creatures, all cross-feeding
- **Why ecosystem synthesis serves the character:**
  A jungle isn't one sound. It's layers calling to each other — insects trigger bird calls, rain drives frog choruses, wind moves the canopy which rustles the floor. XOcelot models this as synthesis: each stratum listens to the others and responds. The result is sound that *breathes* and *prowls* — alive in a way that static layering can never achieve.
- **The coupling thesis:**
  Alone, XOcelot is a self-contained ecosystem producing grooves, textures, and evolving soundscapes. Coupled with other engines, it becomes a *habitat* — BOB's warm bass becomes undergrowth that drives canopy shimmer. ONSET's drums become the heartbeat that triggers creature calls. DRIFT's sweeps become weather systems moving through the forest.

---

## Inspirations & Sonic DNA

### Production Reference: Aesop Rock / Blockhead
- Dusty, chopped sample textures — the Understory layer's core identity
- Off-kilter rhythmic sensibility — swing and humanize parameters on the Floor
- Dense layering that shouldn't work but does — the ecosystem model
- Lo-fi warmth and tape saturation — SP-1200 grit throughout the signal path
- Key albums: *Labor Days*, *None Shall Pass*, *The Impossible Kid*, *Spirit World Field Guide*

### Cultural Reference: Tropicalia
- Os Mutantes, Tom Ze, Gilberto Gil, Caetano Veloso
- The collision of traditional Brazilian instruments with psychedelic electronics
- Berimbau, cuica, agogo, pandeiro — physical-modeled in the Floor layer
- Tape manipulation as art — the Understory's degradation engine
- Warmth + weirdness coexisting — the fundamental XOcelot aesthetic

### Ecological Reference: Tropical Rainforest
- Canopy stratification as synthesis architecture (Floor / Understory / Canopy / Emergent)
- Animal communication as musical events (formant-synthesized calls)
- Density and humidity as timbral qualities
- Everything interconnected — nothing exists in isolation

---

## The XO Concept Test

1. **XO word:** XOcelot — Ocelot: nocturnal jungle cat, stealthy, beautiful, elusive. The prowling predator moving between the layers of the forest.
2. **One-sentence thesis:** "XOcelot is a canopy-layered sample-mangling synthesizer that builds living soundscapes from four interacting strata — percussion floor, chopped understory, spectral canopy, and emergent creature calls."
3. **Sound only this can make:** A berimbau-driven groove where the rhythm's amplitude opens the canopy layer's spectral filter, which triggers formant-synthesized bird trills in the emergent layer, while an SP-1200-crushed chop weaves through the understory — all four strata feeding each other in real time. No DAW plugin or XOmnibus engine does cross-strata ecosystem synthesis.

---

## Gallery Gap Filled

| Existing engines | Synthesis dimension |
|-----------------|---------------------|
| SNAP, MORPH, DRIFT, BOB, FAT | Harmonic (oscillators, wavetables, FM, subtractive) |
| DUB | Temporal (delays, tape echo) |
| OPAL | Granular (time-scattered particles) |
| OVERWORLD | Architectural (chip synthesis recreation) |
| ORGANON | Metabolic (self-evolving harmonic structures) |
| BITE, ONSET | Character / transient |
| OUROBOROS | Recursive (self-feeding) |
| **OCELOT** | **Ecosystem — layered strata that cross-pollinate** |

No current engine models *interaction between internal synthesis layers* as the primary sound-shaping mechanism. OCELOT introduces strata, cross-feeding, and emergent behavior as musical parameters.

---

## Core DSP Architecture (Phase 0 sketch)

```
                    ┌─────────────────────────────────────┐
                    │         ECOSYSTEM MIXER              │
                    │  (strata cross-feed matrix)          │
                    └──┬────────┬────────┬────────┬───────┘
                       │        │        │        │
              ┌────────▼──┐ ┌──▼────────┐ ┌──▼──────┐ ┌──▼────────┐
              │   FLOOR    │ │UNDERSTORY │ │ CANOPY  │ │ EMERGENT  │
              │            │ │           │ │         │ │           │
              │ Physical-  │ │ Sample    │ │Spectral │ │ Formant   │
              │ modeled    │ │ mangler + │ │additive │ │ creature  │
              │ percussion │ │ grain chop│ │pad synth│ │ call gen  │
              │            │ │           │ │         │ │           │
              │ Berimbau   │ │ SP-1200   │ │ Buchla- │ │ Bird      │
              │ Cuica      │ │ Mellotron │ │ style   │ │ Primate   │
              │ Agogo      │ │ tape warp │ │ complex │ │ Insect    │
              │ Kalimba    │ │ bit-crush │ │ osc +   │ │ Frog      │
              │ Pandeiro   │ │ lo-fi     │ │ wavefold│ │ (formant  │
              │            │ │           │ │         │ │  synth)   │
              └─────┬──────┘ └─────┬─────┘ └────┬───┘ └─────┬─────┘
                    │              │             │           │
                    └──────────────┴─────────────┴───────────┘
                                       │
                                       ▼
                              Humidity Stage
                         (saturation + tape warmth
                          + bit-rate reduction)
                                       │
                                       ▼
                              Amp Envelope (ADSR)
                                       │
                                       ▼
                                FX Chain
                         ├── Canopy Reverb (dense, diffuse)
                         ├── Tape Delay (lo-fi, ping-pong)
                         └── Vinyl Hiss (atmosphere)
                                       │
                                       ▼
                              Output (stereo)
```

### Strata Detail

#### FLOOR — Physical-Modeled Percussion
The rhythmic heartbeat. Physical models of iconic instruments from the Tropicalia and broader tropical world:

| Model | Real Instrument | Character | Nod To |
|-------|----------------|-----------|--------|
| **Berimbau** | Single-string struck bow with gourd resonator | Metallic ping with tonal sweep, capoeira rhythm | Tropicalia, Nana Vasconcelos |
| **Cuica** | Friction drum with internal stick | Monkey-like squealing pitch sweeps | Brazilian samba, speaking drum |
| **Agogo** | Double cone bell | Bright metallic ping, two-tone | Afro-Brazilian percussion |
| **Kalimba** | Thumb piano with tine resonators | Warm plucked metallic tone, natural decay | Mbira / African thumb piano |
| **Pandeiro** | Brazilian tambourine with tunable head | Slap, finger roll, thumb strike | Bossa nova, samba |
| **Log Drum** | Hollowed wooden slit drum | Deep woody thud, pitched | Teponaztli, indigenous percussion |

Each model has 2-3 parameters controlling its physical behavior (tension, strike position, damping). MIDI notes select the model + pitch.

#### UNDERSTORY — Sample Mangler + Grain Chopper
The dusty, chopped heart of the Aesop Rock influence. Takes internal oscillator output OR coupling bus audio and mangles it:

- **Chop engine:** Slices audio into rhythmic segments, resequences with swing
- **Bit-crush:** SP-1200 style 12-bit / 26kHz sample rate reduction
- **Tape warp:** Mellotron-style speed wobble, flutter, and degradation
- **Grain scatter:** Small grain cloud (distinct from OPAL's full granular — this is focused on rhythmic chopping, not cloud textures)
- **Vinyl crackle:** Noise layer that responds to amplitude

#### CANOPY — Spectral Additive Pad Synth
The lush overhead texture. Inspired by West Coast synthesis and the dense shimmer of sunlight through leaves:

- **Complex oscillator:** Buchla-style with wavefold and wave-wrap
- **Partial control:** 8 harmonics with individual level + detune (additive)
- **Spectral filter:** Frequency-domain filter that responds to Floor amplitude
- **Shimmer:** Pitch-shifted feedback for crystalline upper harmonics
- **Breathe:** Slow amplitude modulation simulating wind through canopy

#### EMERGENT — Formant Creature Call Generator
The most unusual layer. Synthesizes animal-like calls using formant synthesis:

- **Vocal tract model:** 3 formant filters with pitch, bandwidth, and amplitude
- **Call patterns:** Rhythmic trigger patterns that fire formant sweeps
- **Species presets:** Bird trill, primate howl, insect drone, frog chirp (all synthesized, no samples)
- **Trigger source:** Can be MIDI, Floor amplitude, or Canopy spectral peaks
- **Expressiveness:** Nod to Ondes Martenot / Theremin — continuous pitch and formant control via PlaySurface

### The Ecosystem Cross-Feed Matrix
The defining feature. A 4x4 matrix controlling how each stratum influences the others:

```
              TO:
              Floor  Understory  Canopy  Emergent
FROM:
Floor          —     chop rate   filter  trigger
Understory   swing     —        morph   pitch
Canopy       damp    grain pos    —     formant
Emergent     accent  scatter    shimmer   —
```

Each cross-feed has a bipolar amount knob (-1 to +1). At 0, layers are independent. Positive values create sympathetic interaction. Negative values create contrary motion — when the Floor gets loud, the Canopy gets quiet (like animals going silent when something big moves through the forest).

---

## Biome System — DKC-Inspired Environmental Transformation

The `ocelot_biome` parameter fundamentally transforms how all four strata behave — not just filtering or EQ, but changing the physical models, mangler character, pad behavior, and creature types. Inspired by how Donkey Kong Country's David Wise created entirely different sonic worlds for jungle, underwater, and snow levels using the same SNES BRR architecture.

### Biome: Jungle (Default)
The core XOcelot identity — warm, dense, rhythmic, alive.
- **Floor:** Berimbau, cuica, agogo, kalimba, pandeiro, log drum (full percussion palette)
- **Understory:** Standard chop + bit-crush + tape warp. SP-1200 grit, warm dust.
- **Canopy:** Dense spectral pads, thick wavefold, active shimmer. Sunlight through leaves.
- **Emergent:** Tropical bird trill, primate howl, insect drone, frog chirp.
- **FX character:** Dense diffuse reverb (forest canopy), warm tape delay.
- **Humidity:** Saturation + analog warmth.

### Biome: Underwater
*"Aquatic Ambiance" territory.* Everything slows down, gets filtered, becomes deep and weightless. The ocelot dives beneath the surface.
- **Floor:** Percussion becomes muffled and submerged — low-pass filtered strikes, longer decays, water-logged resonance. Kalimba becomes glass marimba (pitched water drops). Log drum becomes deep oceanic thud. Cuica becomes sonar ping.
- **Understory:** Tape speed halved — extreme slow-motion wobble. Bit-crush softened (higher bit depth). Chops stretch and blur. Everything moves through viscous liquid. Nod to the slow, warped quality of sound underwater.
- **Canopy:** Pads become deep, dark, oceanic. Spectral filter shifts low — rolling, subaquatic movement. Shimmer becomes bioluminescent — subtle, cold, beautiful. Wavefold softens into gentle wave motion. Long, slow breathe parameter (tidal).
- **Emergent:** Creature calls transform — whale song (deep formant sweeps), dolphin clicks (rapid high-pitched chirps), bubble streams (noise bursts through resonant filter), deep current hum (sub-bass formant drone).
- **FX character:** Massive reverb (ocean cavern), long delay with heavy filtering (echoes through water). Reverb pre-delay increases (sound travels slower underwater).
- **Humidity:** Becomes "depth" — low-pass filter + subtle chorus (the acoustic distortion of deep water).

### Biome: Winter
*"In a Snow-Bound Land" territory.* Sparse, crystalline, cold, hauntingly beautiful. The jungle stripped bare by ice.
- **Floor:** Percussion becomes crystalline and fragile. Kalimba becomes ice chimes (higher pitch, longer ring, no damping). Agogo becomes sleigh bell (brighter, thinner). Log drum becomes frozen wood crack. Berimbau becomes bowed icicle (long, singing, cold). Strike character shifts from warm thud to brittle crack.
- **Understory:** Tape speed fluctuates (cold warps the mechanism). Higher tape age = more dropouts (frozen tape sticking). Bit-crush becomes colder — the digital artifacts sound like ice crystals. Chop rate slows. Dust becomes gentle snowfall (softer, higher-frequency noise).
- **Canopy:** Pads become sparse and wide. Fewer partials, more space between them. High shimmer (ice crystals catching light). Breathe parameter becomes wind gusts — irregular, cold, sweeping. Spectral filter opens wide (cold air is transparent). Detuning increases subtly (things contract in the cold).
- **Emergent:** Creature calls transform — distant wolf howl (low formant, long decay, reverb-heavy), winter bird call (sparse, high, plaintive), wind through branches (noise + resonant comb filter), cracking ice (impulse through tuned resonator).
- **FX character:** Wide, cold reverb (snow absorbs close reflections, emphasizes distance). Delay becomes sparser, cleaner (sound carries far in cold air). Crystal clear high end.
- **Humidity:** Becomes "frost" — the saturation character shifts from warm tape to cold digital clarity with subtle high-frequency sparkle.

### Biome as Preset Tool
The biome is stored per-preset in `.xometa`. A producer can:
- Build a jungle groove in Jungle biome
- Duplicate the preset, switch to Underwater, and have an instant DKC water-level version
- The same strata balance, ecosystem depth, and macro mappings apply — but the *character* of every layer transforms

### Biome Transitions
When `ocelot_biome` is automated or coupled, transitions between biomes use 200ms crossfade on all transformed parameters — smooth enough for real-time morphing. Sweeping from Jungle through Underwater to Winter in a single performance is a valid musical gesture (seasons changing, diving and surfacing, altitude shift).

---

## Parameter Namespace

All parameter IDs use `ocelot_` prefix. Key parameters:

### Global
| ID | Range | Description |
|----|-------|-------------|
| `ocelot_biome` | 0-2 | Biome mode: Jungle (0) / Underwater (1) / Winter (2) — transforms all four strata character |
| `ocelot_strataBalance` | 0-1 | Blend between lower (Floor/Understory) and upper (Canopy/Emergent) strata |
| `ocelot_ecosystemDepth` | 0-1 | Master cross-feed intensity (0 = independent layers, 1 = full ecosystem) |
| `ocelot_humidity` | 0-1 | Global saturation/tape warmth (the air in the jungle) |
| `ocelot_swing` | 0-1 | Global rhythmic humanization |
| `ocelot_density` | 0-1 | Overall layer density/activity level |

### Floor (Physical Percussion)
| ID | Range | Description |
|----|-------|-------------|
| `ocelot_floorModel` | 0-5 | Percussion model: berimbau/cuica/agogo/kalimba/pandeiro/log drum |
| `ocelot_floorTension` | 0-1 | String/skin tension (pitch character) |
| `ocelot_floorStrike` | 0-1 | Strike position (brightness) |
| `ocelot_floorDamping` | 0-1 | Resonance decay time |
| `ocelot_floorPattern` | 0-15 | Rhythmic pattern selector |
| `ocelot_floorLevel` | 0-1 | Floor stratum level |

### Understory (Sample Mangler)
| ID | Range | Description |
|----|-------|-------------|
| `ocelot_chopRate` | 1-32 | Chop divisions (tempo-synced) |
| `ocelot_chopSwing` | 0-1 | Chop timing humanization |
| `ocelot_bitDepth` | 4-16 | Bit depth (12 = SP-1200 sweet spot) |
| `ocelot_sampleRate` | 4000-44100 | Sample rate reduction |
| `ocelot_tapeWobble` | 0-1 | Mellotron-style speed flutter |
| `ocelot_tapeAge` | 0-1 | Tape degradation (hiss, dropout, saturation) |
| `ocelot_dustLevel` | 0-1 | Vinyl/dust noise amount |
| `ocelot_understoryLevel` | 0-1 | Understory stratum level |

### Canopy (Spectral Pad)
| ID | Range | Description |
|----|-------|-------------|
| `ocelot_canopyWavefold` | 0-1 | Buchla-style wavefolder depth |
| `ocelot_canopyPartials` | 1-8 | Active harmonic count |
| `ocelot_canopyDetune` | 0-1 | Inter-partial detuning (shimmer) |
| `ocelot_canopySpectralFilter` | 20-20000 | Spectral filter center frequency |
| `ocelot_canopyBreathe` | 0-1 | Wind-like amplitude modulation rate |
| `ocelot_canopyShimmer` | 0-1 | Pitch-shifted harmonic feedback |
| `ocelot_canopyLevel` | 0-1 | Canopy stratum level |

### Emergent (Creature Calls)
| ID | Range | Description |
|----|-------|-------------|
| `ocelot_creatureType` | 0-3 | Call type: bird trill / primate howl / insect drone / frog chirp |
| `ocelot_creatureRate` | 0.1-20 | Call frequency (Hz or tempo-synced) |
| `ocelot_creaturePitch` | 0-1 | Base formant pitch |
| `ocelot_creatureSpread` | 0-1 | Formant bandwidth (narrow = focused, wide = breathy) |
| `ocelot_creatureTrigger` | 0-2 | Trigger source: MIDI / Floor amplitude / Canopy peaks |
| `ocelot_creatureLevel` | 0-1 | Emergent stratum level |

### FX
| ID | Range | Description |
|----|-------|-------------|
| `ocelot_reverbSize` | 0-1 | Canopy reverb size (dense, diffuse) |
| `ocelot_reverbMix` | 0-1 | Reverb wet level |
| `ocelot_delayTime` | 0-1 | Tape delay time (tempo-syncable) |
| `ocelot_delayFeedback` | 0-1 | Delay feedback with tape degradation |
| `ocelot_delayMix` | 0-1 | Delay wet level |

### Amp Envelope
| ID | Range | Description |
|----|-------|-------------|
| `ocelot_ampAttack` | 0.001-8s | Attack time |
| `ocelot_ampDecay` | 0.05-4s | Decay time |
| `ocelot_ampSustain` | 0-1 | Sustain level |
| `ocelot_ampRelease` | 0.05-8s | Release time |

---

## Macro Mapping (M1-M4)

| Macro | Label | Controls | Behavior |
|-------|-------|----------|----------|
| M1 | PROWL | `ocelot_strataBalance` + `ocelot_floorModel` crossfade + `ocelot_humidity` | The character dial. Low = floor-heavy percussive groove. Mid = balanced ecosystem. High = canopy-dominant lush pads with creature calls floating above. The ocelot moving from forest floor to treetops. |
| M2 | FOLIAGE | `ocelot_density` + `ocelot_canopyPartials` + `ocelot_chopRate` + `ocelot_creatureRate` | The movement dial. Low = sparse, minimal, breathing slow. High = dense, rapid, teeming with activity. How thick is the jungle. |
| M3 | ECOSYSTEM | `ocelot_ecosystemDepth` + coupling amount when coupled | The interaction dial. Low = independent layers, clean separation. High = full cross-feed, layers driving each other, alive and unpredictable. When coupled with external engines, this also controls coupling amount. |
| M4 | CANOPY | `ocelot_reverbSize` + `ocelot_reverbMix` + `ocelot_delayMix` + `ocelot_canopyShimmer` | The space dial. How high is the canopy above you. Low = intimate, close, dry. High = vast overhead space, shimmering reverb, echoes bouncing between trees. |

All 4 macros produce audible, significant change at every point in their range in every preset.

---

## Coupling Interface Design

### OCELOT as Target (receiving from other engines)

| Coupling Type | What XOcelot Does | Musical Effect |
|---------------|-------------------|----------------|
| `AudioToWavetable` | Writes source audio into Understory chop buffer | Any engine's sound gets chopped, bit-crushed, and tape-warped — the Aesop Rock treatment |
| `AudioToFM` | Source audio frequency-modulates the Canopy oscillator | External engines add harmonic complexity to the lush pad layer |
| `AmpToFilter` | Source amplitude drives Canopy spectral filter | Drum hits from ONSET open the jungle canopy — rhythmic brightness |
| `RhythmToBlend` | Source rhythm pattern drives strata balance | ONSET's kick pattern shifts emphasis between Floor and Canopy — the jungle breathes with the beat |
| `EnvToMorph` | Source envelope sweeps creature formant pitch | External dynamics control what the Emergent layer "says" |
| `LFOToPitch` | Source LFO modulates Floor percussion tension | Cross-engine vibrato on the berimbau/cuica — expressive wobble |
| `EnvToDecay` | Source envelope controls Floor damping | External dynamics control percussion resonance — tight vs. ringing |

**Primary coupling:** `AudioToWavetable` — feeding audio into the Understory chop buffer. This is XOcelot's "bring me your sound and I'll make it funky" superpower. BOB's warm bass chopped into dusty grooves. DRIFT's sweeps sliced into rhythmic textures. OVERWORLD's chip melodies bit-crushed through the SP-1200.

### OCELOT as Source (sending to other engines)

`getSampleForCoupling()` returns: post-humidity mixed output (all four strata blended), stereo, normalized +/- 1.

Good receiving engines:
- **DUB** — Ecosystem output through dub delay. Tropical dub. The obvious pairing.
- **OPAL** — XOcelot's percussive textures granulated into clouds. Kalimba grains. Berimbau particles.
- **FAT** — Creature calls fattened into massive stereo formant stacks.
- **DRIFT** — Ecosystem output as the source for DRIFT's journey — the jungle as starting point for an odyssey.

### Coupling types OCELOT should NOT receive
- `AmpToChoke` — choking an ecosystem kills everything unnaturally (no musical use)
- `PitchToPitch` — the Floor models have fixed pitch tables; external pitch coupling creates confusion with physical models
- `FilterToFilter` — XOcelot's spectral filter is ecosystem-driven; external filter coupling conflicts with the cross-feed matrix

---

## Instrument & Synth Nods (Reference Guide)

These instruments and synths informed XOcelot's design. Each nod is baked into a specific parameter, behavior, or stratum:

| Reference | Where in XOcelot | What It Contributes |
|-----------|-----------------|---------------------|
| **Berimbau** | Floor model | Single-string physical model with gourd resonance. The sound of capoeira and Tropicalia's rhythmic soul. Nana Vasconcelos made this a synth before synths existed. |
| **Cuica** | Floor model | Friction drum — stick rubbed inside a drum head creates pitch-sweeping squeals. Sounds like a monkey. The voice of samba's percussive choir. |
| **Agogo** | Floor model | Double cone bell from Afro-Brazilian tradition. Two pitches, infinite rhythmic variation. |
| **Kalimba / Mbira** | Floor model | Tuned metal tines over a wooden resonator. The thumb piano. Hugh Tracey field recordings. Organic, pitched, percussive. |
| **Pandeiro** | Floor model | Brazilian frame drum. Jingle + head = infinite articulation from a single hand-held instrument. |
| **E-mu SP-1200** | Understory bit-crush | 12-bit / 26.04kHz sampling. The grit that defined golden-age hip-hop. Aesop Rock's production lineage runs through this box. The "wrong" sample rate is the sound. |
| **Mellotron** | Understory tape warp | Tape-replay keyboard. Each key triggers a strip of magnetic tape — wobble, wow, flutter, and eventual degradation are features, not bugs. The original sample-playback instrument. |
| **Buchla Music Easel** | Canopy complex osc | West Coast synthesis — wavefold, wave-wrap, complex oscillator. Organic, alien, unpredictable. The anti-Moog. Suzanne Ciani's instrument. |
| **EMS Synthi AKS** | Ecosystem cross-feed matrix concept | Pin-matrix patching — any output to any input. The Tropicalia experimentalists' secret weapon. The cross-feed matrix is a spiritual descendant. |
| **Ondes Martenot** | Emergent expressiveness | Early electronic instrument with continuous pitch ribbon. Ghostly, vocal, expressive. The creature call layer's expressiveness on PlaySurface channels this energy. |
| **Theremin** | Emergent pitch control | Continuous pitch in space. No physical contact. The original "sounds like an alien animal" instrument. |
| **MPC 2000XL / 3000** | Understory chop engine | Roger Linn's grid. Chop, sequence, swing. The rhythmic framework that Blockhead and Aesop Rock's production sits on. |
| **Moog Voyager** | Canopy filter warmth | The warm, round, fat filter sound. Not as a recreation, but as a timbral target for the Canopy's spectral filter when humidity is high. |

---

## Visual Identity

- **Accent color:** Ocelot Tawny `#C5832B`
  - Warm golden-brown of the jungle cat's coat
  - Distinct from BOB's Amber `#E9A84A` (darker, earthier, less yellow)
  - Distinct from SNAP's Terracotta `#C8553D` (more gold, less red)
  - Evokes warmth, earth, organic materials, vintage gear
- **Material/texture:** Ocelot rosette pattern — organic spotted shapes, not geometric. Like looking at dappled sunlight through a jungle canopy.
- **Gallery panel character:** Warm, earthy, living. The panel should feel like peering into dense foliage — layered depth with warm light filtering through. Subtle organic movement if animation budget allows (slow drift like leaves in wind).
- **Icon concept:** A stylized ocelot eye — the vertical slit pupil surrounded by the spotted rosette markings. Alternatively, four horizontal strata lines (Floor/Understory/Canopy/Emergent) with an ocelot pawprint connecting them.

---

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | **High** | Floor percussion creates funky rhythmic foundations. Berimbau grooves, kalimba patterns, chopped bass lines. |
| Atmosphere | **High** | Canopy pads + creature calls + humidity = dense atmospheric textures. Rainforest at dawn. |
| Entangled | **High** | The ecosystem cross-feed IS entanglement. Coupling-focused presets showcase layers driving each other. |
| Prism | Medium | Bright kalimba leads and agogo melodies can be prismatic, but this isn't the engine's primary territory. |
| Flux | Medium | High ecosystem depth + high density creates unpredictable, evolving textures. |
| Aether | **High** | The Underwater and Winter biomes unlock XOcelot's ethereal side — oceanic depths (whale song, bioluminescent shimmer, subaquatic pads) and frozen landscapes (ice chimes, wind howl, crystalline space). David Wise proved this territory is gold. |

Primary moods: Foundation, Atmosphere, Entangled, Aether.

---

## Preset Strategy (Phase 0 sketch)

**120 presets at v1.0:**

| Category | Count | Biome | Character |
|----------|-------|-------|-----------|
| Jungle Floor Grooves | 18 | Jungle | Berimbau, cuica, kalimba-driven rhythmic patterns. Funky, organic, Foundation mood. The peanut butter. |
| Canopy Atmospheres | 12 | Jungle | Lush spectral pads with creature calls emerging. Atmospheric, dense, humid. |
| Dusty Chops | 18 | Jungle | Understory-heavy — chopped, bit-crushed, tape-warped textures. Aesop Rock territory. SP-1200 grit. |
| Ecosystem Evolvers | 12 | Jungle | High cross-feed depth — layers driving each other into emergent behavior. Entangled mood. |
| Tropicalia Fusion | 12 | Jungle | Physical percussion + spectral pads + lo-fi warmth. The Tropicalia sweet spot. Spacey. |
| Underwater Depths | 15 | Underwater | David Wise-inspired oceanic pads, subaquatic percussion, whale song, bioluminescent shimmer. Aether/Atmosphere mood. The "Aquatic Ambiance" zone. |
| Winter Stillness | 13 | Winter | Ice chimes, frozen wood, wind howl, crystalline sparse pads, distant wolf calls. Aether mood. The "Snow-Bound Land" zone. |
| Biome Morphs | 5 | Cross-biome | Presets designed for real-time biome sweeping — Jungle to Underwater, Winter to Jungle, full seasonal cycles. Entangled/Flux mood. |
| Coupling Showcases | 15 | Mixed | Designed for use with specific engine pairs (DUB x OCELOT, ONSET x OCELOT, OPAL x OCELOT, etc.) |

### Naming Style
Evocative, nature-adjacent, 2-3 words. Nods to jungle ecology, DKC levels, Tropicalia, and hip-hop production without jargon:

**Jungle examples:** "Tawny Prowl", "Humid Groove", "Canopy Drip", "Berimbau Dawn", "Dusty Kalimba", "Cuica Conversation", "Foliage Thick", "Vine Swing", "Peanut Butter Strut", "Dappled Light", "Monsoon Tape", "Root System", "Howler Dub", "Understory Chop", "Ocelot Stalk", "Tropical Crackle", "Frog Chorus", "Golden Paw", "Night Canopy", "Slow Prowl"

**Underwater examples:** "Coral Cathedral", "Abyssal Hum", "Kelp Sway", "Deep Current", "Whale Hymn", "Sunken Temple", "Pressure Drop", "Bioluminous", "Reef Pulse", "Tide Memory", "Sonar Lullaby", "Glass Marimba", "Ocean Floor", "Bubble Rise", "Subaquatic Dawn"

**Winter examples:** "Frozen Canopy", "Ice Kalimba", "Snowbound", "Crystal Crack", "Wolf Distance", "Pine Shatter", "Frost Chime", "White Silence", "Cold Ember", "Bare Branch", "Glacier Hymn", "Powder Drift", "Winter Ocelot"

---

## Voice Architecture

- **Max voices:** 8
- **Voice stealing:** Quietest voice (ecosystem textures benefit from graceful fadeout over abrupt cut)
- **Legato:** Yes (for Canopy pad lines and Floor percussion rolls)
- **Mono mode:** Available (for Floor-focused bass grooves)

---

## Decision Gate: Phase 0 -> Phase 1 ✅ COMPLETE

- [x] Concept brief written
- [x] XO word feels right (XOcelot — ocelot as jungle prowler, stealthy, beautiful, moving between strata)
- [x] Gallery gap is clear (no ecosystem/cross-strata synthesis engine exists)
- [x] At least 2 coupling partner ideas exist (DUB, ONSET, OPAL, DRIFT, BOB)
- [x] Excited about the sound
- [x] Unique capability defined (four-strata cross-feeding ecosystem synthesis)
- [x] Inspirational lineage clear (Aesop Rock + Tropicalia + rainforest ecology)
- [x] Instrument nods documented (berimbau, cuica, SP-1200, Mellotron, Buchla, EMS Synthi, Ondes Martenot)

## Decision Gate: Phase 1 -> Phase 2 ✅ COMPLETE

Architecture spec: `Docs/xocelot_phase1_architecture.md`

- [x] 62 parameter IDs frozen with `ocelot_` prefix
- [x] Cross-feed matrix typed (12 routes, 3 route types, negative-amount semantics locked)
- [x] Macro mapping defined (PROWL / FOLIAGE / ECOSYSTEM / CANOPY)
- [x] Coupling interface designed (7 supported, 4 unsupported with reasons)
- [x] Signal flow complete (4-strata → ecosystem mixer → humidity → amp env → FX)
- [x] Voice architecture defined (8 voices, quietest-stealing, legato)
- [x] CPU budget verified (<13% single, <28% dual-engine)
- [x] Hero preset archetypes defined (8 presets, DNA values assigned)
- [x] Biome system architecture defined (BiomeMorph, 200ms crossfade, compile-time profiles)

**-> Proceed to Phase 2: Scaffold + Build**
*Invoke: `/new-xo-project name=XOcelot identity="Canopy-layered sample-mangling synthesizer — four interacting strata with biome system" code=XOcl`*

---

*XO_OX Designs | Engine: OCELOT | Accent: #C5832B | Prefix: ocelot_*
