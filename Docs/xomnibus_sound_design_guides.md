# XOmnibus — Sound Design Guide
*Per-engine reference for sound designers, preset builders, and performers.*
*Covers 20 of 29+ registered engines (plus OMBRE): features, key parameters, coupling strategies, and recommended pairings.*
*5 Constellation engines (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) have dedicated synthesis guides in Docs/ (e.g. ohm_synthesis_guide.md) but are not yet integrated into this unified guide.*
*Registered engines not yet covered in this guide: OSPREY, OSTERIA, OWLFISH, OCELOT, OHM, ORPHICA, OBBLIGATO, OTTONI, OLE.*
*V2 concept engines (OSTINATO, OPENSKY, OCEANDEEP, OUIE) are not registered in XOmnibus and are out of scope for this guide.*

---

## How to Use This Guide

Each engine section covers:
1. **Identity** — what the engine sounds like and what it's for
2. **Key Parameters** — the knobs that matter most, with sweet spots
3. **Coupling** — what modulation it sends/receives and best coupling partners
4. **Recommended Pairings** — which engines complement it and why
5. **Starter Recipes** — specific parameter settings for common sound profiles

**Macro mapping convention:** M1=CHARACTER, M2=MOVEMENT, M3=COUPLING, M4=SPACE

---

## 1. ODDFELIX (OddfeliX)
*Percussive transient synthesis — feliX the neon tetra*

**Accent:** Neon Tetra Blue `#00A6D6` | **Prefix:** `snap_` | **Voices:** 8
**Creature:** The neon tetra — a flash of iridescent blue darting through sunlit shallows
**Polarity:** Pure feliX — the surface, where energy is highest and sounds are sharpest

### What It Does
feliX himself. Every note begins with his signature dart — a pitch sweep crashing from up to +24 semitones down to the target, like a tail snap changing direction underwater. Three oscillator modes give him range: Sine+Noise for the body splashes, FM for metallic surface reflections, Karplus-Strong for the pluck of a fishing line breaking the surface. The HPF→BPF filter cascade is the water itself — filtering sunlight by depth. The unison detune is the school — one fish becomes four, darting in slightly different directions.

feliX is the initiator. He strikes first. His transient envelope is the sharpest attack in the gallery, and his coupling output drives everything downstream.

### Macros (Recommended)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **DART** | `snap_snap` + `snap_decay` inverse | The snap of a tail — how sharp the transient is |
| M2 | **SCHOOL** | `snap_unison` + `snap_detune` | The school — how many fish, how spread |
| M3 | **SURFACE** | `snap_filterCutoff` + `snap_filterReso` | The water surface — how much light filters through |
| M4 | **DEPTH** | FX sends + reverb | How deep the splash echoes |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `snap_snap` | 0–1 | 0.3–0.6 | The dart intensity. Higher = more transient. The neon flash. |
| `snap_decay` | 0.001–5s | 0.1–0.4 | How long the ripple lasts. Short for clicks, longer for toms. |
| `snap_filterCutoff` | 20–20kHz | 1–4 kHz | HPF→BPF cascade. The surface filter — lower = deeper water. |
| `snap_filterReso` | 0–1 | 0.2–0.5 | Resonance. Higher = the ring of a droplet hitting still water. |
| `snap_detune` | 0–100 cents | 5–15 | School spread. Wider = bigger swarm. |
| `snap_unison` | 1/2/4 | 2 | School size. 4 = the whole swarm changes direction at once. |
| `snap_oscMode` | 0/1/2 | — | 0=Sine+Noise (body splash), 1=FM (metallic reflection), 2=KS (plucked string) |

### Coupling
- **Sends:** Envelope level (ch2) — feliX's transient drives other engines' filters, gates, chokes
- **Receives:** AmpToPitch, LFOToPitch — gentle pitch drift makes the dart less predictable
- **Best as source for:** AmpToFilter (every snap pumps a pad), AmpToChoke (the dart silences)
- **Ecosystem role:** The initiator. feliX gives sharp envelopes that drive other species. He receives slowly — things that make his darting less predictable.

### Recommended Pairings
- **+ OddOscar:** The primordial coupling. feliX darts, Oscar breathes. Transient over sustain.
- **+ Overdub:** Snap through dub delay — the dart echoing between temperature layers. Classic dub techno.
- **+ Odyssey:** Snap triggers over drifting pad — the neon flash above the open ocean. Ambient percussion.
- **+ Oblique:** Snap transient into prism delay — the dart refracting through shallow water into spectral colors.
- **+ Onset:** Two percussive species schooling — feliX's organic snap alongside Onset's algorithmic precision.

### Starter Recipes
**Tail Snap:** snap=0.8, decay=0.15, oscMode=0, filterCutoff=400 — feliX's signature dart
**Surface Pluck:** snap=0.5, decay=0.3, oscMode=2 (KS), filterCutoff=3000, reso=0.4 — a fishing line breaking the surface
**Metallic Flash:** snap=0.6, decay=0.25, oscMode=1 (FM), filterCutoff=2000, detune=20 — neon scales catching light
**School Stab:** snap=0.4, decay=0.2, unison=4, detune=12, filterCutoff=3000 — the whole swarm darting at once
**Deep Dart:** snap=0.7, decay=0.4, oscMode=0, filterCutoff=800, reso=0.3 — feliX diving below the surface

---

## 2. ODDOSCAR (OddOscar)
*Lush pad synthesis with wavetable morph — Oscar the axolotl*

**Accent:** Axolotl Gill Pink `#E8839B` | **Prefix:** `morph_` | **Voices:** 16
**Creature:** The axolotl — pink gills breathing slowly in a coral cave, regenerating, always transforming
**Polarity:** The Reef — Oscar's home territory, where life is densest

### What It Does
Oscar himself. The O in XO_OX. Where feliX darts and vanishes, Oscar remains. Where feliX is a flash, Oscar is a tide. Three detuned oscillators create the chorus width of a coral colony — many identical polyps, slightly out of phase, producing a collective sound richer than any individual. The morph parameter (Sine→Saw→Square→Noise) mirrors the axolotl's metamorphic ability: the same creature, continuously transforming without ever losing its essential nature. Sine is the axolotl at rest. Saw is mid-regeneration, bristling with new tissue. Square is full display. Noise is the axolotl dissolving into the reef itself. The 4-pole Moog ladder filter is the warmth of the reef — resonant, self-oscillating when pushed, capable of singing on its own. The Perlin noise drift is his breathing — never quite the same cycle twice, never mechanical, always organic.

Oscar is the responder. He sustains. He is the tide that holds everything else afloat.

### Macros (Recommended)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **BLOOM** | `morph_morph` sweep | Metamorphosis — how much the axolotl is transforming |
| M2 | **BREATHE** | `morph_drift` + `morph_filterCutoff` | The gill rhythm — how alive the breathing feels |
| M3 | **REGENERATE** | `morph_sub` + `morph_bloom` (attack) | The axolotl's regenerative power — depth + growth speed |
| M4 | **DEPTH** | FX sends + reverb | How deep in the coral cave the sound echoes |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `morph_morph` | 0–3 | 0.5–1.5 | Metamorphosis. Sine(0)→Saw(1)→Square(2)→Noise(3). The axolotl transforming. |
| `morph_filterCutoff` | 20–20kHz | 2–6 kHz | Reef warmth. The Moog ladder opens like a coral polyp. |
| `morph_filterReso` | 0–1 | 0.3–0.6 | Self-oscillates above 0.9 — the reef singing to itself. |
| `morph_drift` | 0–1 | 0.1–0.3 | Gill breathing. Perlin noise pitch drift. The organic pulse. |
| `morph_sub` | 0–1 | 0.3–0.5 | Heartbeat. Sub oscillator one octave below — the reef's foundation. |
| `morph_detune` | 0–50 cents | 5–15 | Colony width. Three detuned oscillators spreading like polyps. |
| `morph_bloom` | 0.001–10s | 0.3–2 | Growth speed. How slowly the pad blooms open. |

### Coupling
- **Sends:** 0.3Hz internal LFO (ch2) — the gill rhythm, breathing modulation to the entire gallery
- **Receives:** AmpToFilter (inverted dub pump — feliX's snaps duck Oscar's filter, then it blooms back), EnvToMorph (external envelopes shift morph position — other engines literally change what Oscar sounds like)
- **Best as source for:** LFOToPitch (gentle pitch drift that makes any engine wander organically)
- **Ecosystem role:** The responder. Oscar absorbs energy from other species and transforms it into warmth. His 0.3Hz LFO is a tidal pulse that the whole gallery can breathe to.

### Recommended Pairings
- **+ OddfeliX:** The primordial coupling. feliX darts, Oscar breathes. Snap transient over morphing pad — the neon tetra darting through coral.
- **+ Oblong:** Two reef-dwellers layered. Oscar's liquid morph + Bob's curious texture = the living coral colony.
- **+ Optic:** Oscar feeds Optic for spectral analysis — bioluminescent plankton reacting to the reef's slow breathing.
- **+ Odyssey:** Two drift systems overlapping. Oscar's Perlin breathing + Drift's Voyager walk = interference patterns like overlapping ripples.
- **+ Overbite:** Oscar's warmth feeding the anglerfish's lure. EnvToMorph from Overbite shifts Oscar's metamorphosis — the predator reshaping its prey.

### Starter Recipes
**Coral Breath:** morph=0.3, filterCutoff=3000, reso=0.3, drift=0.2, sub=0.4 — the axolotl at rest in warm water
**Metamorphosis:** morph automated via M1, filterCutoff=1500, reso=0.5, drift=0.4 — the creature mid-transformation
**Reef Dissolve:** morph=2.5, filterCutoff=800, reso=0.6, drift=0.1 — Oscar becoming indistinguishable from the coral
**Deep Colony:** morph=0.8, detune=20, sub=0.6, filterCutoff=2000, bloom=1.5 — the full polyp chorus, wide and warm
**Singing Ladder:** morph=0.1, filterCutoff=1200, reso=0.85, drift=0.15 — the filter self-oscillating, the reef singing

---

## 3. OVERDUB (Overdub)
*Dub synthesis with tape delay and spring reverb — the thermocline*

**Accent:** Olive `#6B7B3A` | **Prefix:** `dub_` | **Voices:** 8
**Creature:** The thermocline — the invisible boundary where warm surface water meets cold deep water
**Polarity:** Dead center — 50/50 feliX/Oscar, the meeting point between worlds

### What It Does
The thermocline. The invisible boundary where warm surface water meets cold deep water, typically between 200 and 1000 meters. Sound behaves strangely here — it bends, refracts, bounces between density layers, degrades over distance. A whale call launched at the thermocline can travel thousands of miles, losing high frequencies with each reflection, gaining a ghostly character that marine biologists call "shadow zones." That is exactly what XOverdub does to every note.

The tape delay is sound traveling through water. The wow (0.3Hz sine) pulls timing like a slow swell. The flutter (45Hz noise) adds the micro-instability of turbulence. The bandpass feedback narrowing is the ocean filtering the signal with each bounce — eating highs and lows until only the mid-range ghost remains. The spring reverb is a sunken ship's hull — six allpass stages ringing like steel plates struck underwater. The drive is pressure building at depth.

Dub music was always underwater music. Lee "Scratch" Perry and King Tubby were building thermoclines in their studios. XOverdub makes the metaphor literal. He is the water that other species swim through.

### Macros (Recommended)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **ECHO** | `dub_delayFeedback` + `dub_delayWear` | The distance — how far the sound travels through water |
| M2 | **CURRENT** | `dub_delayWow` + `dub_drift` | The swell — how much the current pulls the timing |
| M3 | **PRESSURE** | `dub_driveAmount` + `dub_filterCutoff` | The depth — how much pressure distorts the signal |
| M4 | **HULL** | `dub_reverbSize` + `dub_reverbMix` | The ship — how much the hull resonates around the echo |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `dub_oscWave` | Sine/Tri/Saw/Sq | Sine or Saw | The source signal — what enters the water |
| `dub_filterCutoff` | 20–20kHz | 400–2000 | The thermocline itself — where warm meets cold. 3 modes (LP/HP/BP). |
| `dub_delayTime` | 0.05–2s | 0.3–0.5 | Distance between reflections. Try 3/8 note for classic dub spacing. |
| `dub_delayFeedback` | 0–1.2 | 0.4–0.7 | How many times the sound bounces. >1.0 = runaway — the echo never stops. |
| `dub_delayWear` | 0–1 | 0.2–0.4 | Tape degradation. Each bounce loses more detail, like sound traveling miles through water. |
| `dub_delayWow` | 0–1 | 0.1–0.2 | The swell. Subtle = warmth, heavy = seasick pitch drift on every echo. |
| `dub_driveAmount` | 1–10 | 1.5–3 | Depth pressure. Subtle warmth to full saturation — the sound compressed by the ocean. |
| `dub_reverbMix` | 0–1 | 0.1–0.3 | Hull resonance. The spring reverb. A little = steel hull. A lot = inside the wreck. |
| `dub_sendLevel` | 0–1 | 0.3–0.6 | How much signal enters the FX chain. The send into the deep. |

### Coupling
- **Sends:** Pitch envelope (ch2) — short, punchy envelopes from the oscillator, before the delay touches them
- **Receives:** AmpToFilter (inverted pump — the classic dub sidechain), AmpToPitch, LFOToPitch
- **Best as source for:** AudioToFM (tape-degraded audio as FM source — the ghost of a note modulating something alive)
- **Ecosystem role:** The great receiver. Every engine sounds different through Overdub's echo chamber. He takes any signal and gives it history — repetition, degradation, space. He is the water that other species swim through.

### Recommended Pairings
- **+ OddfeliX:** feliX's dart echoing through the thermocline — the snap becomes a rhythm. Instant dub techno.
- **+ OddOscar:** Oscar's pads gaining history — each echo slightly different because the drift has moved. Infinite, evolving sustain.
- **+ Oblique:** Dub sub-bass foundation + Oblique's prismatic refraction = the full water column, bottom to shimmer.
- **+ Optic:** Optic's AutoPulse drives Dub's filter for rhythmic pumping — bioluminescent flashes triggering echo gates.
- **+ Overbite:** The anglerfish's bass through the thermocline. The deepest dub imaginable — predator biology echoing through temperature gradients.
- **+ Onset:** Drum hits echoing through water — every splash creating a trail of ghostly repetitions.

### Starter Recipes
**Shadow Zone:** oscWave=Sine, filterCutoff=600, sub=0.7, delayMix=0.1, voiceMode=Mono — the bass note trapped between temperature layers
**Thermocline Echo:** oscWave=Saw, delayTime=0.375, feedback=0.6, wear=0.4, wow=0.15, reverb=0.25 — sound bouncing between warm and cold
**Siren at Depth:** oscWave=Saw, glide=0.5, filterCutoff=2000, reso=0.6, voiceMode=Legato — a whale call sliding through the deep
**Sunken Hull:** oscWave=Tri, reverb=0.5, reverbSize=0.8, drive=2.0, delayFeedback=0.3 — the resonance of a ship on the ocean floor
**Mile-Deep Ghost:** delayFeedback=0.85, wear=0.7, wow=0.2, filterCutoff=800 — a note that has traveled for miles and forgotten what it was

---

## 4. ODYSSEY (Odyssey)
*Evolving pad synthesis with Voyager drift and Climax bloom — the open ocean*

**Accent:** Violet `#7B2D8B` | **Prefix:** `odyssey_` | **Voices:** 8
**Creature:** The open ocean — the pelagic zone, where currents carry you from the familiar into the unknown
**Polarity:** Open Water — between surface and deep, Oscar-leaning but with feliX's restless movement

### What It Does
The open ocean. No reef, no shore, no floor — just the vast middle water where the only landmarks are shifting gradients of light, temperature, and pressure. The Voyager Drift is the current — a smooth random walk on pitch and filter that ensures you never hear the same note the same way twice. Not chaos. The organized unpredictability of a living ocean. Per-voice seeded RNG means each voice drifts independently — a held chord becomes a living organism.

Three oscillator modes are three species encountered on the voyage. Classic PolyBLEP is the familiar shore — clean waveforms you recognize. Supersaw is the school of fish — seven detuned voices swimming in formation, creating a wall of width. FM is the bioluminescent flash — metallic, alien, harmonically complex. Dual oscillators (A and B) mean you can layer the familiar with the alien in a single voice. The Haze Saturation is warmth rising from below. The Formant Filter is the voice of the ocean — vowel shapes (ah, eh, ee, oh, oo) morphing as if the water is trying to speak. The Prism Shimmer is sunlight refracting through the surface far above.

Odyssey is the traveler. She tells a story. Alone, she asks a question.

### Macros (Recommended)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **JOURNEY** | `odyssey_oscA_mode` blend + character stages | The voyage — how far from shore you've traveled |
| M2 | **DRIFT** | `odyssey_driftDepth` + `odyssey_driftRate` | The current — how strong and fast the ocean pulls |
| M3 | **SHIMMER** | `odyssey_hazeAmount` + `odyssey_shimmerAmount` | Depth of light — how much warmth and refraction |
| M4 | **VOICE** | `odyssey_formantMorph` + `odyssey_formantMix` | The ocean speaking — which vowel the water shapes |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `odyssey_driftDepth` | 0–1 | 0.15–0.35 | Current strength. How far each note wanders from home. The "alive" factor. |
| `odyssey_driftRate` | 0–1 | 0.2–0.5 | Current speed. How quickly new targets appear in the random walk. |
| `odyssey_oscA_mode` | 0/1/2 | — | 0=Classic (shore), 1=Supersaw (the school), 2=FM (the deep flash) |
| `odyssey_hazeAmount` | 0–1 | 0.2–0.5 | Thermocline warmth. Pre-filter tanh saturation — warmth rising from below. |
| `odyssey_formantMorph` | 0–4 | 0.5–2.5 | The ocean's vowel. Sweeps ah→eh→ee→oh→oo. Slow automation = the water speaking. |
| `odyssey_shimmerAmount` | 0–1 | 0.1–0.4 | Sunlight refraction. Full-wave rectification adding upper partials like light through water. |
| `odyssey_filterCutoff` | 20–20kHz | 1.5–5 kHz | Depth. Lower = deeper water, less light. The LP filter is the water column itself. |
| `odyssey_attack` | 0.001–2s | 0.05–0.3 | How slowly the voyage begins. Long attacks for cinematic swells. |
| `odyssey_oscA_detune` | 0–100 | 8–15 | School spread (Supersaw mode). Wider = bigger school. |

### Coupling
- **Sends:** Peak envelope (ch2) — always shifting because of the Voyager Drift, making receiving engines feel alive
- **Receives:** AmpToFilter (feliX or Onset create rhythmic filter movement — surface percussion heard as filter pumps in the deep), LFOToPitch, AmpToPitch, EnvToMorph (sweeps formant vowel — other engines change the words the ocean is speaking)
- **Best as source for:** AudioToFM (drifting output as FM source — the voyage as modulation)
- **Ecosystem role:** The traveler. She gives evolving envelopes that make other engines feel alive. When coupled, she tells a story. Alone, she asks a question.

### Recommended Pairings
- **+ OddfeliX:** The neon flash above the open ocean. Snap transients over drifting pad — ambient percussion from another world.
- **+ Overdub:** Drifting pads gaining history — each echo slightly different because the drift has moved between repetitions. Time made audible.
- **+ OddOscar:** Two drift systems creating interference patterns — Oscar's Perlin breathing + Odyssey's Voyager walk = overlapping ripples in open water.
- **+ Optic:** Spectral analysis of the voyage — bioluminescent plankton reacting to the ocean's harmonic content.
- **+ Opal:** Two evolving engines stacked — granular particles scattered across the open ocean. Deep ambient clouds.
- **+ Overbite:** The journey descending to predator depth. Odyssey's shimmer feeding the anglerfish's lure.

### Starter Recipes
**Open Water:** attack=0.3, sustain=0.8, release=2.0, drift=0.25, detune=12, filterCutoff=4000 — the moment the shore disappears behind you
**Pelagic Dark:** filterCutoff=800, reso=0.4, drift=0.3, detune=20, hazeAmount=0.4 — deep water where only blue light reaches
**Shore Keys:** attack=0.005, decay=0.3, sustain=0.5, filterCutoff=6000, drift=0.1 — the familiar coast, gentle current
**Vowel Drift:** formantMorph automated, formantMix=0.6, drift=0.2, shimmer=0.3 — the ocean trying to speak
**Bioluminescent:** oscA_mode=2 (FM), fmDepth=0.4, shimmer=0.5, drift=0.3, filterCutoff=3000 — alien flash in the deep water

---

## 5. OBLONG (Oblong)
*Warm character synthesis — soft coral that breathes*

**Accent:** Amber `#E9A84A` | **Prefix:** `bob_` | **Voices:** 8
**Creature:** Living coral — soft tissue, symbiotic algae, the architecture of the reef itself
**Polarity:** The Reef — 75% Oscar / 25% feliX, warm and patient with a curious, darting playfulness

### What It Does
Living coral. Not the skeletal calcium carbonate tourists mistake for rock — the soft tissue itself, the polyps, the mucus layer, the symbiotic algae that give coral its color. Touch a living coral and it feels warm, slightly yielding, faintly sticky. That is the XOblongBob sound. The Oblong Sine waveform — a sine with a gentle second harmonic folded in — is the polyp: simple in structure, complex in texture. The Velvet Saw and Cushion Pulse are swaying arms that filter-feed from the current. Every waveform has been softened, rounded, given a tactile quality that feels closer to organic tissue than electronic oscillation.

The BobSnoutFilter is the coral's relationship with passing water — four character modes (Snout LP, Snout BP, Snout Form, Snout Soft) each represent a different interaction with current. The BobCuriosityLFO system — five behavior modes (Sniff, Wander, Investigate, Twitch, Nap) — is the coral's nervous system, responding with the slow, exploratory movements of an organism that has no brain but somehow still seems curious. Bob does not compute modulation. Bob sniffs, wanders, investigates, twitches, and naps.

He is Oscar's architecture. The reef that every other species depends on.

### Macros (Recommended)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **SNOUT** | `bob_fltCutoff` + `bob_fltMode` character | The coral mouth — how the polyps filter the current |
| M2 | **CURIOSITY** | `bob_curiosity` + LFO behavior | The nervous system — how the coral responds to stimulation |
| M3 | **TEXTURE** | `bob_textureType` + `bob_textureLevel` | The tissue — what type of organic surface the coral grows |
| M4 | **REEF** | `bob_dustTape` + `bob_drift` | The age — how weathered and analog the coral structure feels |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `bob_fltCutoff` | 20–20kHz | 1–4 kHz | The coral mouth. How open the polyp is to the current. |
| `bob_fltReso` | 0–1 | 0.3–0.6 | Resonance — the mouth singing. Self-oscillates at high values. |
| `bob_curiosity` | 0–1 | 0.2–0.5 | The nervous system. Higher = the coral is more alert, more exploratory, weirder. |
| `bob_drift` | 0–1 | 0.1–0.2 | Per-oscillator analog instability. The organic sway of coral in current. |
| `bob_oscAWave` | 0–3 | — | 0=Oblong Sine (polyp), 1=Soft Triangle, 2=Velvet Saw (arms), 3=Cushion Pulse |
| `bob_textureType` | 0–3 | — | 0=Dust, 1=Blanket, 2=Static, 3=Breath — the surface texture of the coral |
| `bob_dustTape` | 0–1 | 0.1–0.3 | Tape saturation. The age and weathering of the reef structure. |
| `bob_fltMode` | 0–3 | — | 0=Snout LP (warm), 1=Snout BP (nasal), 2=Snout Form (vowel), 3=Snout Soft (gentle) |

### Coupling
- **Sends:** Stereo audio (ch0/1), envelope level (ch2)
- **Receives:** AmpToFilter (drum hits bloom the coral open — Onset drives the Snout), AmpToPitch, LFOToPitch
- **Best as source for:** Amplitude coupling that softens aggressive engines — passing through coral structure warms everything
- **Ecosystem role:** The reef that other species shelter in. His output softens whatever it touches. The contrast between his warmth and feliX-leaning engines' sharpness creates the most musical pairings in the gallery.

### Recommended Pairings
- **+ OddfeliX:** Neon tetra darting through coral — feliX's snap transients against Bob's warm, yielding texture. The signature pairing.
- **+ Overdub:** Bob's curious harmonics through Dub's tape delay — vintage keys heard through a sunken ship's hull.
- **+ Obese:** The reef sheltering the whale. Bob provides character and warmth, Fat provides mass. Layered bass at its best.
- **+ Oblique:** Bob's curious harmonics through Oblique's prismatic refraction — the light scattering through shallow coral water. Kaleidoscopic.
- **+ Onset:** Drum hits blooming the coral open — AmpToFilter from Onset drives Bob's Snout, and every snare makes the reef breathe.
- **+ Organon:** Two biological systems communicating — coral metabolism meeting deep-sea chemistry. Slow, strange, alive.

### Starter Recipes
**Polyp Touch:** oscA=Oblong Sine, fltCutoff=3000, curiosity=0.3, drift=0.15 — the warm, yielding surface of living coral
**Funky Stab:** attack=0.002, decay=0.15, sustain=0.4, fltCutoff=3000, curiosity=0.3 — a coral arm snapping at passing plankton
**Reef Bass:** attack=0.001, decay=0.3, sustain=0.0, fltCutoff=1500, reso=0.5, dustTape=0.2 — the foundation, warm and deep
**Curious Pad:** curiosity=0.5, fltMode=2 (Snout Form), drift=0.2, texture=Breath — the coral responding to something it doesn't understand
**Amber Hour:** oscA=Velvet Saw, fltCutoff=4000, curiosity=0.15, drift=0.15, dustTape=0.3 — the reef at golden hour, glowing

---

## 6. OBESE (Obese)
*Massive width synthesis — the whale*

**Accent:** Hot Pink `#FF1493` | **Prefix:** `fat_` | **Voices:** 6
**Creature:** The whale — the largest creature in the water column, 188 decibels across the ocean basin
**Polarity:** Open Water — the whale belongs to no reef, no shore, no single depth

### What It Does
The whale. When a blue whale vocalizes at 188 decibels, the sound travels through the entire ocean basin. Every creature for a thousand miles hears it — not as a distant echo but as a pressure wave that moves through their body. Thirteen oscillators — one sub, four groups of three, each tuned at root, +12, and -12 semitones with per-oscillator analog drift. The sound does not sit in a mix. It displaces everything else.

The Mojo system is the whale's biology. Each oscillator receives independent random-walk pitch drift of up to three cents. At Mojo zero, XObese is a digital supersaw — precise, clinical, impressive but lifeless. At Mojo one, it breathes. The drift is subtle enough that the ear hears warmth rather than detuning, the same way whale song sounds "warm" even though it is mathematically complex. Four ZDF Ladder Filters — one per oscillator group, each with proper tan() pre-warp, inter-stage soft-clip, and Nyquist-aware resonance scaling — are the whale's vocal tract, shaping raw oscillator mass into something musical rather than merely overwhelming.

The arpeggiator is the whale's rhythm — cyclical patterns of diving and surfacing. The hot pink accent is the belly of a right whale surfacing — unexpected beauty on something that massive.

### Macros (Recommended)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **MASS** | `fat_unisonSpread` + morph position | The whale's size — how much of the frequency spectrum it fills |
| M2 | **SONG** | `fat_filterCutoff` + `fat_filterReso` | The vocal tract — how the whale shapes its call |
| M3 | **MOJO** | `fat_mojo` (analog drift) | The biology — how alive the thirteen oscillators breathe |
| M4 | **SURFACE** | `fat_satDrive` + bitcrush | The breach — the distortion of the whale breaking the surface |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `fat_satDrive` | 0–1 | 0.3–0.7 | The breach. Asymmetric waveshaping — the distortion of 188 dB through water. |
| `fat_filterCutoff` | 20–20kHz | 800–4000 | The vocal tract. Four ZDF ladders shaping thirteen oscillators into song. |
| `fat_filterReso` | 0–1 | 0.3–0.6 | Vocal resonance. Smooth self-oscillation — the whale's harmonic overtones. |
| `fat_unisonSpread` | 0–1 | 0.3–0.6 | School size. How far apart the thirteen oscillators drift. Width measured in miles. |
| `fat_mojo` | 0–1 | 0.2–0.4 | The biology. Per-oscillator random walk drift. 0=digital, 1=alive. |
| `fat_morph` | 0–3 | 0.5–1.5 | Morphable waveform. Continuous Sine→Saw→Square→Noise blend across all 13 oscillators. |
| `fat_subOctave` | -2/-1/0 | -1 | How deep the heartbeat. Sub oscillator octave selector. |

### Coupling
- **Sends:** Stereo audio (ch0/1), envelope level (ch2) — raw, massive amplitude
- **Receives:** AmpToFilter, AmpToPitch, LFOToPitch — pitch and filter modulation that shapes the whale into something that breathes
- **Best as source for:** AudioToRing (thirteen oscillators ring-modding = seismic), AmpToFilter (every note opens receiving engines wide)
- **Ecosystem role:** The whale gives amplitude. Without external modulation, he is a wall. With coupling, he is an ocean.

### Recommended Pairings
- **+ OddfeliX:** The whale surfacing where the neon tetra swims. Fat body + feliX's snap = punchy bass with click. Scale difference as musicality.
- **+ Oblique:** Fat bass foundation + Oblique's prismatic bouncing tops — the full depth of the ocean from whale song to surface refraction.
- **+ Optic:** Optic's AutoPulse strobing Fat's filter — bioluminescent flashes pumping the whale's vocal tract. Club-ready.
- **+ Overdub:** The whale song echoing through the thermocline. Thirteen oscillators degrading beautifully through tape delay over distance.
- **+ Overbite:** The whale meeting the anglerfish. Mass meets predation. The widest bass feeding the deepest bite.
- **+ Oblong:** The whale sheltering in the reef. Fat provides mass, Bob provides character. Together: warm, immense, alive.

### Starter Recipes
**Whale Call:** unisonSpread=0.4, filterCutoff=1200, satDrive=0.5, mojo=0.3, voiceMode=Mono — the fundamental vocalization, a pressure wave you feel in your chest
**Migration Pad:** unisonSpread=0.6, filterCutoff=4000, satDrive=0.2, mojo=0.4, release=1.5 — the pod traveling, wide and sustained
**Breach:** satDrive=0.8, filterCutoff=3000, reso=0.5, voiceMode=Legato, mojo=0.3 — the whale breaking the surface, all mass and distortion
**Deep Pulse:** subOctave=-2, filterCutoff=600, mojo=0.2, morph=0.0, satDrive=0.1 — the heartbeat of the largest animal on Earth
**Hot Pink Arp:** arp on, unisonSpread=0.5, filterCutoff=2000, satDrive=0.3, mojo=0.4 — the cyclical dive-and-surface rhythm

---

## 7. OVERBITE (Overbite)
*Bass-forward character synth — the anglerfish*

**Accent:** Fang White `#F0EDE8` | **Prefix:** `poss_` | **Voices:** 16
**Creature:** The anglerfish — a soft bioluminescent lure hiding the widest jaw in the deep
**Polarity:** The Deep — 80% Oscar / 20% feliX, patient and ambush-hunting, but the Bite macro reveals feral energy

### What It Does
The anglerfish. Two thousand meters below the surface, in water so dark that color has no meaning, a point of light appears. Soft. Warm. Pulsing gently. It looks like safety. It is the most dangerous thing in the ocean. XOverbite is built on this duality: two oscillator banks where OscA ("Belly" — four warm waveforms: Sine, Triangle, Saw, Cushion Pulse) is the lure, and OscB ("Bite" — five aggressive waveforms: Hard Sync, FM, Ring Mod, Noise, Grit) is the jaw.

Five character stages form the anatomy of a predator. **Fur** is the anglerfish's textured skin — pre-filter tanh saturation adding plush even harmonics. The **CytomicSVF filter** is the mouth — four modes (LP, BP, HP, Notch) with key tracking and envelope-driven movement. **Chew** is the jaw muscle — post-filter soft-knee compression adding sustain and body. **Gnash** is the teeth — asymmetric waveshaping where the positive half clips harder, creating odd harmonics that cut like fangs. **Trash** is the aftermath — three dirt modes (Rust bitcrush, Splatter wavefold, Crushed hard-clip) representing what happens after the bite.

The Weight Engine reinforces the sub-harmonic foundation — five shapes, three octaves below. The signal path is an anatomy lesson in predation.

### Macros (Built)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **BELLY** | Closes filter, raises sub + fur + weight | The lure — warm, round, glowing, irresistible |
| M2 | **BITE** | Opens OscB mix, drives gnash + resonance | The jaw — the instant it snaps shut |
| M3 | **SCURRY** | Multiplies LFO rates, compresses envelopes | The nervous system — the twitching energy underneath the patience |
| M4 | **TRASH** | Raises trash amount + resonance | The aftermath — what remains after the anglerfish feeds |
| M5 | **PLAY DEAD** | Extends release, ducks level, closes filter | Going dark — retracting the lure, becoming invisible |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `poss_osc_mix` | 0–1 | 0.2–0.5 | Belly↔Bite balance. The lure vs the jaw. 0 = pure lure, 1 = pure fangs. |
| `poss_filter_cutoff` | 20–20kHz | 800–3000 | The mouth. How wide the jaw opens. |
| `poss_filter_reso` | 0–1 | 0.3–0.6 | How much the mouth resonates — higher = a growl building in the throat. |
| `poss_fur_amount` | 0–1 | 0.2–0.5 | Skin texture. Plush even harmonics before the filter. The lure's glow. |
| `poss_gnash_amount` | 0–1 | 0.1–0.4 | Teeth. Asymmetric distortion — the odd harmonics that cut through everything. |
| `poss_trash_amount` | 0–1 | 0–0.3 | Destruction. Bitcrush/wavefold/hard-clip — what the anglerfish leaves behind. |
| `poss_weight_level` | 0–1 | 0.3–0.6 | Sub-harmonic reinforcement. The physical mass of the predator. |
| `poss_osc_b_wave` | 0–4 | — | 0=HardSync, 1=FM, 2=RingMod, 3=Noise, 4=Grit — which type of bite |

### Coupling
- **Sends:** Stereo audio (ch0/1), envelope level (ch2) — bass-heavy output with predator character
- **Receives:** AmpToFilter (drum hits pump the jaw open — Onset drives the Bite macro), AudioToFM (external audio FM-modulates OscB — feeding the bite with other species' spectral content)
- **Best as source for:** AudioToFM (Gnash-shaped audio as FM source — the predator's harmonic signature modulating other engines)
- **Ecosystem role:** The predator. He receives energy and transforms it into ambush. He feeds from above and lurks below.

### Recommended Pairings
- **+ Onset:** Drum hits driving the Bite macro — every snare triggers the jaw reflex. The snare is the prey.
- **+ Obese:** The whale meeting the anglerfish. Fat's mass driving Overbite's filter = the widest, deepest bass in the ocean.
- **+ Overdub:** Anglerfish bass through the thermocline's tape delay — the deepest dub imaginable. Predator biology echoing through temperature gradients.
- **+ OddfeliX:** The neon tetra entering anglerfish territory — feliX's bright transients feeding Overbite's FM input. Prey becoming predator texture.
- **+ Organon:** Anglerfish biology meeting alien chemistry — Fur saturation into metabolic processing. Deep, strange, and alive.
- **+ Oblong:** The predator sheltered by the reef. Overbite's aggression softened by Bob's warmth — bass that bites but doesn't hurt.

### Starter Recipes
**The Lure:** belly=1.0, bite=0.0, fur=0.4, filterCutoff=800, weight=0.5 — the glowing bioluminescent orb, warm and inviting
**The Jaw:** belly=0.0, bite=1.0, gnash=0.5, filterCutoff=2000, oscB=HardSync — the moment the mouth opens. All fangs.
**Ambush Bass:** belly=0.6, bite=0.3, fur=0.3, gnash=0.2, filterCutoff=1200 — between lure and jaw, where the real danger lives
**Deep Prowl:** belly=0.4, scurry=0.5, filterCutoff=1000, weight=0.6, voiceMode=Mono — the anglerfish circling, nervous energy under patience
**Play Dead:** playDead=1.0, release=3.0, filterCutoff=400 — the lure retracts, the predator becomes invisible, waiting

---

## 8. ONSET (Onset)
*Algorithmic drum synthesis with Cross-Voice Coupling — surface splashes*

**Accent:** Electric Blue `#0066FF` | **Prefix:** `perc_` | **Voices:** 8 (fixed kit)
**Creature:** Surface splashes — fish leaping, tails slapping, rain striking the ocean in a downpour
**Polarity:** Pure feliX — the surface, where energy is highest and every sound is an event, not a process

### What It Does
Surface splashes. Eight voices, each a different kind of impact event — the physics of water breaking. The kick is a body entering water from height — the BridgedT oscillator's pitch envelope sweeps down like displacement pressure. The snare is a wave breaking on rock — the NoiseBurst circuit's dual-sine body with high-passed noise burst. The hats are rain — the MetallicOsc's six inharmonic square waves through bandpass filtering, the bright patter of droplets on the ocean surface.

The dual-layer architecture mirrors impact physics: **Layer X** (circuit models — BridgedT, NoiseBurst, MetallicOsc) is the body of the collision, the analog truth of a 808 kick or metallic hat. **Layer O** (algorithmic synthesis — FM, Modal Resonator, Karplus-Strong, Phase Distortion) is the character, the alien synthesis that no drum machine has ever produced. The blend knob per voice is the key: at zero, you get an 808 kick; at one, you get an FM percussion hit tuned to the same frequency; in between, something that has never existed in hardware.

**Cross-Voice Coupling (XVC)** is schooling behavior made audible. Eight voices reacting to each other — kick pumps the snare's filter, hat chokes the open hat, clap triggers the tom. Not a sequencer — an ecosystem of rhythmic organisms reacting in real time. The same way a flash of silver in a school triggers a chain reaction.

### Macros (Built)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **MACHINE** | Layer X↔O blend bias across all voices | The species — how analog (fish) vs algorithmic (electric) the kit sounds |
| M2 | **PUNCH** | Snap transient + body amount per voice | The impact — how hard the surface breaks |
| M3 | **SPACE** | FX sends (delay + reverb) | The depth — how far the splash echoes underwater |
| M4 | **MUTATE** | Per-hit pitch/timbre drift amount | The school — how much each hit differs from the last |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `perc_v1_pitch` | Hz | varies | Base frequency per voice. The size of the splash. (v1–v8 per voice) |
| `perc_v1_decay` | ms | 50–300 | How long the ripple lasts. Tight clicks to resonant toms. |
| `perc_v1_blend` | 0–1 | 0.3–0.6 | Layer X↔O crossfade. Analog circuit ↔ algorithmic synthesis. The magic zone. |
| `perc_v1_algoMode` | 0–3 | — | 0=FM, 1=Modal, 2=KS, 3=PhaseDist — which algorithm Layer O uses |
| `perc_v1_snap` | 0–1 | 0.3–0.7 | Transient intensity. How sharp the impact. feliX's dart as a drum parameter. |
| `perc_v1_body` | 0–1 | 0.3–0.6 | Resonant sustain. How much the water rings after impact. |
| `perc_v1_character` | 0–1 | 0.2–0.5 | Layer O character amount. How alien the splash sounds. |
| `perc_v1_tone` | 0–1 | 0.3–0.7 | Per-voice filter. The surface tension — how much high frequency passes. |
| `perc_v1_level` | 0–1 | 0.7–1.0 | Per-voice output level. |
| `perc_v1_pan` | -1–1 | varies | Per-voice stereo position. |
| `perc_v1_envShape` | 0–1 | 0.3–0.7 | Envelope shape per voice. |
| `perc_level` | 0–1 | 0.8 | Master output level. |
| `perc_drive` | 0–1 | 0.1–0.4 | Master saturation drive. |
| `perc_masterTone` | 0–1 | 0.4–0.7 | Master tone filter. |
| `perc_xvc_amount` | 0–1 | 0.1–0.4 | Cross-Voice Coupling intensity. How reactive the school is. |
| `perc_macro_machine` | 0–1 | varies | MACHINE macro — Layer X↔O blend bias across all voices. |
| `perc_macro_punch` | 0–1 | varies | PUNCH macro — snap transient + body amount per voice. |
| `perc_macro_space` | 0–1 | varies | SPACE macro — FX sends (delay + reverb). |
| `perc_macro_mutate` | 0–1 | varies | MUTATE macro — per-hit pitch/timbre drift amount. |

### Coupling
- **Sends:** Stereo audio + 8 independent peak amplitudes — each voice is an independent coupling source
- **Receives:** Pattern modulation, density control — things that reshape rhythm without killing precision
- **Best as source for:** AmpToFilter (drum hits pumping pad filters), AmpToChoke (percussion gating other engines), AmpToPitch (drum-triggered pitch sweeps on synth engines)
- **Ecosystem role:** The pulse. He drives everything — the school of fish, fast, coordinated, reacting before any individual can think. Eight coupling sources firing in rhythm.

### Recommended Pairings
- **+ Overdub:** Every splash echoing through the thermocline — drum hits gaining history and ghostly tape delay tails. Instant dub rhythm.
- **+ OddfeliX:** Two feliX-polarity engines schooling — organic snap alongside algorithmic precision. The neon tetra swimming with the electric current.
- **+ Overbite:** Kick drives the anglerfish's Bite macro — every downbeat triggers the jaw reflex. The snare is the prey.
- **+ Oblong:** Drum hits blooming the coral — AmpToFilter from Onset opens Bob's Snout filter. Every backbeat makes the reef breathe.
- **+ Optic:** Eight drum voices feeding spectral analysis — bioluminescent plankton reacting to eight different impact frequencies. Rhythm-reactive light.
- **+ Odyssey:** Surface percussion heard as filter pumps in the open ocean — the deep hearing the surface.

### Starter Recipes
**Body Drop:** perc_v1_pitch=55, perc_v1_decay=200, perc_v1_blend=0.0, perc_v1_snap=0.7, perc_v1_body=0.5 — a body entering water from height, the BridgedT circuit at its purest
**Breaking Wave:** perc_v2_pitch=180, perc_v2_decay=120, perc_v2_blend=0.5, perc_v2_algoMode=1 (Modal), perc_v2_snap=0.5 — wave on rock, circuit model meeting modal resonance
**Rain Patter:** perc_v3_pitch=8000, perc_v3_decay=25, perc_v3_blend=0.3, perc_v3_character=0.4, perc_v3_tone=0.7 — light rain on the ocean surface, six inharmonic squares
**School Reaction:** perc_xvc_amount=0.4, V1 kick pumps V2 snare filter, V3 chokes V4 — the school: each splash triggers the next
**Alien Percussion:** perc_v1_blend=0.8 across all voices, perc_v1_algoMode=0 (FM), perc_v1_character=0.6 — Layer O dominant, every hit an FM flash from the deep

---

## 9. OVERWORLD (Overworld)
*Chip synthesis — NES/Genesis/SNES/GameBoy*

**Accent:** Neon Green `#39FF14` | **Prefix:** `ow_` | **Voices:** 8

### What It Does
A time machine for video game sound. 6 chip engines (NES, FM, SNES, GameBoy, PCE, Neo Geo) with ERA triangle-pad blending between 3 engines simultaneously. Bit crusher, glitch engine, and FIR echo add authentic retro processing.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `ow_vertexA/B/C` | 0–5 | 0/1/2 | Which chip engines sit at the triangle vertices |
| `ow_era/eraY` | 0–1 | varies | Barycentric position on the ERA triangle pad |
| `ow_glitchAmount` | 0–1 | 0.1–0.3 | Glitch engine intensity |
| `ow_colorTheme` | 0–14 | varies | Visual theme selector |

### Coupling
- **Sends:** Chip synthesis output
- **Receives:** AmpToFilter (external amplitude modulates chip filter), EnvToMorph (morph ERA blend), AudioToFM (external audio as FM source)

### Recommended Pairings
- **+ OddfeliX:** Chip melodies + Snap percussion = retro game soundtrack
- **+ Obese:** Chip leads over Fat's massive bass = chiptune EDM
- **+ Optic:** Retro audio feeding Optic's visualizer = playable music visualizer

### Starter Recipes
**NES Square Lead:** engineA=NES, pulseWidth=0.5, bitDepth=8
**FM Bass:** engineA=FM, algorithm=1, feedback=0.3
**SNES Pad:** engineA=SNES, era blend toward SNES vertex, reverb=0.2

---

## 10. OPAL (Opal)
*Granular cloud synthesis*

**Accent:** Lavender `#A78BFA` | **Prefix:** `opal_` | **Voices:** 12 clouds

### What It Does
12 independent grain clouds, each spawning up to 32 simultaneous grains. Scatter controls for position, pitch, and pan create shimmering, evolving textures. Spectral freeze captures a moment in time.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `opal_grainSize` | 1–500 ms | 20–80 | Individual grain length. Shorter = more granular texture. |
| `opal_density` | 1–32 | 8–16 | Grains per cloud. Higher = denser, more continuous. |
| `opal_posScatter` | 0–1 | 0.1–0.4 | Randomize grain read position. Higher = more chaotic. |
| `opal_pitchScatter` | 0–24 st | 2–7 | Random pitch offset per grain. Creates shimmer. |
| `opal_freeze` | on/off | — | Freeze current grain buffer. Infinite sustain. |
| `opal_panScatter` | 0–1 | 0.3–0.6 | Stereo spread of grains. Wide = immersive. |

### Coupling
- **Sends:** Granular audio output (ch0/1)
- **Receives:** AudioToWavetable (feed external audio as grain source!)
- **Best as source for:** AudioToWavetable on other engines (granular as source material)

### Recommended Pairings
- **+ Odyssey:** Drift pad frozen into Opal grains = evolving ambient cloud
- **+ Organon:** Granular textures feeding modal resonance = ethereal singing
- **+ Optic:** Opal's shimmer analyzed by Optic = responsive visual textures

### Starter Recipes
**Shimmer Pad:** grainSize=40, density=16, pitchScatter=5, panScatter=0.5, freeze=off
**Frozen Moment:** Play a note, engage freeze, then modulate position/pitch
**Granular Rain:** grainSize=5, density=32, positionScatter=0.8, pitchScatter=12

---

## 11. ORBITAL (Orbital)
*64-partial additive synthesis with formant shaping*

**Accent:** Warm Red `#FF6B6B` | **Prefix:** `orb_` | **Voices:** 6

### What It Does
Pure additive synthesis: 64 harmonic partials per voice, each with independent amplitude. Formant filter shapes the partial envelope with spectral tilt and vowel modes. Clean, precise, and endlessly sculptable.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `orb_brightness` | -1 to +1 | -0.3 to +0.3 | Spectral tilt. Negative = dark, positive = bright. |
| `orb_oddEven` | 0–1 | 0.5 | Balance between odd and even harmonics |
| `orb_formantShape` | 0–1 | — | Spectral formant shaping amount |
| `orb_formantShift` | 0–1 | — | Shifts formant center frequency up/down |
| `orb_morph` | 0–1 | 0.3–0.7 | Crossfades between Profile A and Profile B partial maps |

### Coupling
- **Sends:** Stereo audio (ch0/1)
- **Receives:** AudioToWavetable (replace/blend partial mix from external source)
- **Best as source for:** AudioToFM (pure harmonics as clean FM source)

### Recommended Pairings
- **+ Oblong:** Additive purity + Bob's curiosity = complementary harmonic textures
- **+ Origami:** Orbital's clean partials → Origami's spectral folding = controlled chaos
- **+ Obscura:** Additive output scanned through Obscura's spring chain

### Starter Recipes
**Organ:** morph=0.5, brightness=0, oddEven=0.7 (mostly odd)
**Bright Bell:** brightness=0.5, morph=0.8, ampDecay=0.8, ampSustain=0.0
**Formant Pad:** formantShape=0.8, formantShift=0.5, morph=0.5, oddEven=0.5

---

## 12. ORGANON (Organon)
*Biochemical metabolism synth — Variational Free Energy drives Port-Hamiltonian modal arrays*

**Accent:** Bioluminescent Cyan `#00CED1` | **Prefix:** `organon_` | **Voices:** 4

### What It Does
ORGANON is a deep-sea chemotroph: it eats audio signals and converts their information content into living harmonic structures. Each of the 4 voices is an independent organism with its own metabolic state machine based on the Variational Free Energy / Active Inference framework (Karl Friston, 2010). The organism ingests audio — from a coupling partner or its own internal noise substrate — filters it through an enzyme selectivity bandpass, analyzes its entropy, then uses that signal to drive a Port-Hamiltonian modal array (descended from Julius O. Smith III's modal synthesis). Free energy accumulates on note-on and dissipates on release; when the organism starves it dims, when fed rich signal it blooms. Blessed (B011): unanimous seance praise, described as publishable research.

No two performances sound identical — metabolic state accumulates from coupling history.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `organon_metabolicRate` | 0.1–10 Hz | 0.5–3.0 | Speed of energy turnover. 0.1 Hz = glacial deep-sea, 10 Hz = frantic surface feeding. Aftertouch accelerates this. |
| `organon_enzymeSelect` | 20–20000 Hz | 200–4000 | Bandpass center for ingestion. Narrow = specialist feeder, high = generalist. |
| `organon_catalystDrive` | 0–2 | 0.4–1.2 | How aggressively metabolic energy excites the modal array. 0 = silent, 2 = potentially self-exciting. |
| `organon_dampingCoeff` | 0.01–0.99 | 0.15–0.45 | Damping in the modal ODE. Low = long crystalline tails, high = muted percussive response. |
| `organon_signalFlux` | 0–1 | 0.4–0.8 | Gain of the ingestion input — coupling audio or internal noise. 0 = starved, 1 = fully fed. |
| `organon_phasonShift` | 0–1 | 0–0.6 | Temporal offset between the 4 voices' metabolic cycles. 0 = unison pulse, 1 = fully polyrhythmic (voices fully out of phase). Named after quasicrystal phason modes. |
| `organon_isotopeBalance` | 0–1 | 0.3–0.7 | Spectral weighting of modal partials. 0 = subharmonic (dark), 0.5 = natural harmonic series, 1 = upper partials (metallic, alien). |
| `organon_lockIn` | 0–1 | 0–0.5 | Sync metabolic rate to DAW tempo. 0 = free-running organic drift, 1 = quantized to beat subdivisions. |
| `organon_membrane` | 0–1 | 0.1–0.4 | Membrane porosity — reverb send level. High porosity leaks energy into shared reverb. VFE surprise modulates this automatically: stressed organisms sweat more. |
| `organon_noiseColor` | 0–1 | 0.3–0.7 | Spectral tilt of internal noise substrate (when no coupling partner feeds the organism). 0 = dark rumble, 0.5 = white, 1 = bright hiss. Coupling input bypasses this. |

### Coupling
- **Sends:** Metabolic modal output (ch0/1) — usable as AudioToFM or AudioToWavetable source for partner engines
- **Receives:** AudioToFM, AudioToWavetable (audio feeds the ingestion stage — organism eats it), RhythmToBlend, EnvToDecay, AmpToFilter, EnvToMorph, LFOToPitch, PitchToPitch
- **Best as target for:** Any audio-output engine (Ouroboros, Obese, Onset) — ORGANON literally digests their output into harmonic structure
- **Mod wheel:** Adds up to +3 Hz to metabolic rate | **Aftertouch:** Boosts metabolic rate and free energy — plays the organism harder

### Variational Free Energy (B011 — Seance Blessed)
The VFE state machine tracks three internal readouts per voice that shape sound automatically:
- **freeEnergy:** Envelopes output amplitude — builds on note-on, dissipates on release
- **surprise:** Shifts fundamental pitch and widens stereo spread when organism is stressed; also modulates `organon_membrane` reverb send
- **adaptationGain:** Scales catalyst drive — well-adapted organisms amplify their own excitation

### Recommended Pairings
- **+ Ouroboros:** Chaos attractor output feeds ORGANON's ingestion — organism metabolizes chaos into harmonic structure
- **+ Onset:** Drum transients feed the enzyme bandpass — percussive bursts bloom into resonant modal clouds
- **+ Opal:** ORGANON's modal output drives Opal grain scatter — living harmonic organism seeds a granular cloud
- **+ Optic:** ORGANON's evolving modal output drives Optic's visual modulator in real-time

### Starter Recipes
**Hydrothermal Bloom:** metabolicRate=1.5, signalFlux=0.7, catalystDrive=0.6, dampingCoeff=0.2, isotopeBalance=0.5, membrane=0.3
**Deep Crystal:** metabolicRate=0.3, dampingCoeff=0.08, isotopeBalance=0.8, catalystDrive=0.4, phasonShift=0.0, noiseColor=0.3
**Polyrhythmic Colony:** phasonShift=0.9, metabolicRate=2.0, lockIn=0.5, signalFlux=0.6, catalystDrive=0.8
**Starved Drone:** signalFlux=0.1, metabolicRate=0.5, dampingCoeff=0.05, noiseColor=0.2, membrane=0.5

---

## 13. OUROBOROS (Ouroboros)
*Chaotic ODE synthesis — the strange attractor*

**Accent:** Strange Attractor Red `#FF2D2D` | **Prefix:** `ouro_` | **Voices:** 6

### What It Does
RK4-integrated chaotic ordinary differential equations at 4× oversampling. Four selectable attractor topologies (Lorenz, Rossler, Chua, Aizawa) with Phase-Locked Chaos ("The Leash") for tonal control. 3D attractor trajectory projected to stereo via theta/phi angles. Accepts external ODE perturbation (`ouro_injection`) from coupled engines — meaning drum hits can literally push the chaos. This is not delay-line or feedback synthesis. It is the sound of a differential equation solving itself in real time.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `ouro_topology` | 0–3 | — | Attractor type: 0=Lorenz, 1=Rossler, 2=Chua, 3=Aizawa |
| `ouro_rate` | 80–200 | 95–145 | Integration speed — controls attractor orbital frequency (tonal register) |
| `ouro_chaosIndex` | 0–1 | 0.35–0.65 | Chaos intensity. <0.35 = stable. >0.75 = Chua breakdown territory |
| `ouro_leash` | 0–1 | 0.5–0.9 | Phase-Locked Chaos: >0.7 = almost pitched, 0.4–0.6 = uncanny middle, <0.4 = pure chaos |
| `ouro_theta` | 0–π | 0.0, 1.571 | 3D projection polar angle. 1.571 (π/2) = Z-axis = warmer, DC-shifted output |
| `ouro_phi` | −π–π | 0.0, 0.785 | 3D projection azimuth. 0.785 (π/4) = maximum stereo width |
| `ouro_damping` | 0–1 | 0.25–0.45 | Attractor energy loss — higher = slower, more sustained chaos |
| `ouro_injection` | 0–1 | 0.12–0.22 | External ODE perturbation — coupling target, allows external engines to push the attractor |

### Attractor Topologies
- **Lorenz (0):** Angular, fast-moving, the canonical chaos. Default starting point.
- **Rossler (1):** Slower, smooth asymmetric orbit. Most pitch-adjacent. Best for melodic chaos.
- **Chua (2):** Warm, buzzy, circuit-flavored. At high chaosIndex + high leash = edge of breakdown. Most organic.
- **Aizawa (3):** Dense, layered. Highest harmonic density. Least explored.

### Coupling
- **Sends:** Attractor velocity outputs (B007 — Seance Blessed) — chaotic, harmonically rich
- **Receives:** AmpToFilter (shapes chaos output), AmpToPitch (injection source via ouro_injection)
- **Best as source for:** AudioToFM (attractor velocity as FM source = unique sidebands)
- **Best as target for:** ONSET velocity output → injection (drum hits perturb the attractor)

### Recommended Pairings
- **+ Onset:** ONSET velocity → ouro_injection = rhythm gives chaos a heartbeat
- **+ Opal:** Ouroboros velocity drives Opal grain scatter — chaos organizes the granular cloud
- **+ Optic:** Attractor velocity drives Optic visualizer. Optic modulates chaos back.
- **+ Oblique:** Ouroboros chaos through Oblique's prism = fractured attractor

### Starter Recipes
**Strange Attractor (Lorenz):** topology=0, chaosIndex=0.5, leash=0.55, rate=145, theta=1.571
**Vent Drone (Rossler):** topology=1, chaosIndex=0.15, leash=0.9, rate=120, theta=0.8, phi=1.2
**Chua Atmosphere:** topology=2, chaosIndex=0.75, leash=0.7, damping=0.65, rate=95
**Perturbed (couple with Onset):** topology=0, injection=0.18, chaosIndex=0.55, leash=0.45

---

## 14. OBSIDIAN (Obsidian)
*Phase distortion synthesis — Casio CZ revival*

**Accent:** Crystal White `#E8E0D8` | **Prefix:** `obsidian_` | **Voices:** 16

### What It Does
Casio CZ-series phase distortion with modern enhancements. A 32×32 LUT for 2D phase distortion (density × tilt) plus Euler-Bernoulli inharmonicity (stiffness) for bell/metallic tones. Stereo phase divergence creates width from a single oscillator.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `obsidian_pdDepth` | 0–1 | 0.3–0.7 | Phase distortion depth. Higher = more harmonics. |
| `obsidian_density` | 0–1 | 0.3–0.6 | PD LUT X axis. Controls harmonic density. |
| `obsidian_tilt` | 0–1 | 0.4–0.6 | PD LUT Y axis. Spectral tilt. |
| `obsidian_stiffness` | 0–1 | 0.1–0.4 | Inharmonicity. Makes partials non-integer → bells. |

### Coupling
- **Sends:** Stereo post-filter (ch0/1)
- **Receives:** AudioToFM (modulate PD depth), AmpToFilter, EnvToMorph (density/tilt)
- **Best as source for:** EnvToMorph (its PD envelope drives morph beautifully)

### Recommended Pairings
- **+ Oblong:** Obsidian's crystal purity + Bob's amber curiosity
- **+ Origami:** PD harmonics → spectral folding = complex harmonic evolution
- **+ Opal:** Obsidian tones frozen and granulated by Opal

### Starter Recipes
**CZ Bass:** pdDepth=0.5, density=0.3, tilt=0.2, stiffness=0.0
**Crystal Bell:** pdDepth=0.7, stiffness=0.4, density=0.5, tilt=0.6
**Metallic Pad:** pdDepth=0.4, stiffness=0.2, stereoDiv=0.3, release=1.5

---

## 15. ORIGAMI (Origami)
*Spectral folding FFT synthesis*

**Accent:** Vermillion Fold `#E63946` | **Prefix:** `origami_` | **Voices:** 8

### What It Does
Real-time FFT (2048-point STFT, 4× overlap) with 4 spectral operations: FOLD, MIRROR, ROTATE, STRETCH. Takes any input signal and folds its frequency spectrum like origami paper. Spectral freeze captures a moment. Phase vocoder maintains quality.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `origami_foldPoint` | 0–1 | 0.3–0.6 | Where the spectrum folds. Center = octave fold. |
| `origami_foldDepth` | 0–1 | 0.2–0.5 | How much folding occurs |
| `origami_operation` | FOLD/MIRROR/ROTATE/STRETCH | FOLD | Which spectral operation |
| `origami_freeze` | on/off | — | Freeze current spectrum |

### Coupling
- **Sends:** Spectrally processed audio (ch0/1)
- **Receives:** AudioToWavetable (external audio as fold source!), AmpToFilter (amp→fold depth), EnvToMorph (envelope→fold point), RhythmToBlend (rhythm→freeze)
- **Best as source for:** AudioToWavetable (folded spectra as wavetable for other engines)

### Recommended Pairings
- **+ Orbital:** Clean additive partials → spectral fold = controlled harmonic manipulation
- **+ Oceanic:** Swarm audio → spectral fold = alien textures
- **+ Overdub:** Dub audio → fold → tape delay = psychedelic dub

### Starter Recipes
**Octave Fold:** operation=FOLD, foldPoint=0.5, foldDepth=0.4
**Spectral Mirror:** operation=MIRROR, foldPoint=0.3, foldDepth=0.6
**Freeze + Stretch:** Play a note, freeze, then stretch slowly

---

## 16. ORACLE (Oracle)
*Stochastic GENDY + Maqam microtonal synthesis (B010 Blessed)*

**Accent:** Prophecy Indigo `#4B0082` | **Prefix:** `oracle_` | **Voices:** 8

### What It Does
Iannis Xenakis' GENDY algorithm: stochastic breakpoint waveforms where 8–32 breakpoints per waveform cycle undergo random walks drawn from a morphable Cauchy/Logistic distribution blend. Mirror barriers reflect overshooting values. Cubic Hermite (Catmull-Rom) interpolation renders smooth per-sample output between breakpoints. Plus 9-option Maqam tuning system (12-TET + 8 maqamat with quarter-tone microtonal steps) blended via the GRAVITY parameter. Two ADSR envelopes: amplitude and stochastic activity. Sounds like prophecy — alien, unpredictable, ancient.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `oracle_breakpoints` | 8–32 | 12–20 | Breakpoints per cycle. More = more complex waveform. |
| `oracle_timeStep` | 0–1 | 0.2–0.4 | How far breakpoints walk in time per cycle. Higher = more rhythmic instability. |
| `oracle_ampStep` | 0–1 | 0.2–0.5 | How far breakpoints walk in amplitude per cycle. Higher = wilder timbral chaos. |
| `oracle_distribution` | 0–1 | 0.3–0.7 | Morph between Logistic (smooth, 0.0) and Cauchy (heavy-tailed, 1.0) distribution. |
| `oracle_barrierElasticity` | 0–1 | 0.3–0.7 | Elasticity of mirror barriers. 0 = hard reflect, 1 = soft bounce. |
| `oracle_maqam` | 12-TET/Rast/Bayati/Saba/Hijaz/Sikah/Nahawand/Kurd/Ajam | 12-TET | Tuning system. 12-TET = standard. Any other = Maqam quarter-tone intonation. |
| `oracle_gravity` | 0–1 | 0.3–0.8 | Blend between 12-TET (0.0) and selected Maqam tuning (1.0). |
| `oracle_drift` | 0–1 | 0.2–0.5 | Long-term waveform drift. Slowly reshapes the stochastic character over time. |

### Macros
- **PROPHECY** — pushes oracle_ampStep + oracle_distribution toward maximum chaos
- **EVOLUTION** — drives oracle_drift + stochastic envelope shape
- **GRAVITY** — sweeps oracle_gravity (12-TET → full Maqam intonation)
- **DRIFT** — long-term oracle_drift + oracle_timeStep combined

### Coupling (B010 Blessed)
- **Sends:** Stochastic stereo audio (ch0/1) with 1% phase decorrelation on R
- **Receives:** AudioToFM (perturbs breakpoint amplitudes), AmpToFilter (modulates barrier positions), EnvToMorph (drives distribution morph between Logistic and Cauchy)
- **Best as source for:** AudioToFM (stochastic output as FM source = unique non-repeating sidebands)

### Recommended Pairings
- **+ Obscura:** Stochastic excitation → mass-spring resonance = alien instruments
- **+ Organon:** Two generative systems coupled = emergent world music
- **+ Opal:** Oracle tones granulated by Opal = stochastic cloud

### Starter Recipes
**Xenakis Drone:** breakpoints=16, ampStep=0.3, timeStep=0.1, maqam=12-TET, gravity=0.0, distribution=0.8
**Maqam Lead:** breakpoints=12, ampStep=0.15, timeStep=0.05, maqam=Rast, gravity=0.8, drift=0.2
**Chaos Texture:** breakpoints=32, ampStep=0.6, timeStep=0.5, distribution=1.0 (Cauchy), barrierElasticity=0.2

---

## 17. OBSCURA (Obscura)
*Scanned synthesis — 128-mass spring chain physics*

**Accent:** Daguerreotype Silver `#8A9BA8` | **Prefix:** `obscura_` | **Voices:** 8

### What It Does
Implements the scanned synthesis technique pioneered by Bill Verplank and Max Mathews at CCRMA (Stanford, late 1990s). A 128-mass spring chain is simulated at ~4 kHz using Verlet integration. At note-on, a Gaussian impulse excites the chain at `obscura_excitePos` with width `obscura_exciteWidth`. `obscura_sustain` applies a continuous bowing force to maintain energy. A scanner sweeps across the chain at MIDI note frequency, reading displacements via cubic Hermite interpolation to produce audio-rate output — L channel scans forward, R channel scans backward. Boundary modes and initial chain shape determine fundamental timbral character. Two envelopes: amplitude and physics excitation.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `obscura_stiffness` | 0–1 | 0.3–0.6 | Spring stiffness. Higher = brighter, more metallic harmonic content. |
| `obscura_damping` | 0–1 | 0.1–0.3 | Energy dissipation per step. Lower = longer sustain and ring. |
| `obscura_nonlinear` | 0–1 | 0.05–0.2 | Spring nonlinearity coefficient. Introduces inharmonicity and chaos. |
| `obscura_excitePos` | 0–1 | 0.3–0.7 | Position on chain where impulse strikes. 0.5 = center. |
| `obscura_exciteWidth` | 0–1 | 0.2–0.5 | Width of the Gaussian impulse. Narrow = bright attack, wide = soft. |
| `obscura_scanWidth` | 0–1 | 0.3–0.6 | Scanner readout width. Narrow = bright, wide = darker Hann-windowed average. |
| `obscura_boundary` | Fixed/Free/Periodic | Fixed | Boundary conditions at chain ends. Fixed = string-like. Free = rod-like. Periodic = infinite loop. |
| `obscura_sustain` | 0–1 | 0.0–0.4 | Continuous bowing force applied to chain. 0 = impulse-only attack, >0 = sustained tone. |
| `obscura_initShape` | Sine/Saw/Random/Flat | Sine | Initial chain displacement shape at note-on. |

### Macros
- **CHARACTER** — drives obscura_stiffness + obscura_nonlinear (timbre hardness axis)
- **MOVEMENT** — drives obscura_scanWidth + LFO depth (animation)
- **COUPLING** — drives coupling send level
- **SPACE** — drives obscura_damping inverse + envelope release

### Coupling
- **Sends:** Stereo scan output (ch0/1) — forward scan L, backward scan R
- **Receives:** AudioToFM (applies external force to chain masses), AmpToFilter (modulates stiffness), RhythmToBlend (triggers impulse excitation)
- **Best as source for:** AudioToWavetable (scanned waveform as live wavetable source for other engines)

### Recommended Pairings
- **+ Oracle:** Stochastic GENDY excitation drives the chain = alien resonator
- **+ Obsidian:** Phase distortion + scanned synthesis = complex metallics
- **+ Organon:** Two physical modeling engines coupled = emergent acoustics

### Starter Recipes
**Bowed String:** sustain=0.35, stiffness=0.4, damping=0.15, boundary=Fixed, initShape=Sine
**Bell Cluster:** sustain=0.0, stiffness=0.7, nonlinear=0.15, boundary=Free, initShape=Random
**Ambient Drone:** sustain=0.25, stiffness=0.2, damping=0.05, boundary=Periodic, scanWidth=0.6

---

## 18. OCEANIC (Oceanic)
*Swarm particle synthesis — 128 oscillators that flock like creatures*

**Accent:** Phosphorescent Teal `#00B4A0` | **Prefix:** `ocean_` | **Voices:** Mono/Poly2/Poly4 (max 4)

### What It Does
128 oscillating particles per voice that self-organize using Craig Reynolds' boid flocking rules (separation, alignment, cohesion) plus an attractor anchored to the MIDI note frequency. Particles live in a 3D perceptual space — log-frequency, amplitude, pan. The emergent flocking behavior produces evolving, organic timbres impossible with static additive synthesis.

Two independent envelopes: **Amp ADSR** controls loudness; **Swarm ADSR** controls the *strength of the boid rules* — the collective intelligence of the school — entirely separately. At `ocean_swarmEnvSustain=0` the boid forces extinguish during sustain: particles drift freely while the note holds at full amplitude. At `ocean_swarmEnvAttack=2.5s` the school takes 2.5 seconds to learn to be a school — the attack is organic, scattered, then coheres into a tone. B013 Blessed: the Chromatophore Modulator — aftertouch boosts separation, modeling cephalopod color-shift physics.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `ocean_separation` | 0–1 | 0.2–0.7 | How strongly particles repel nearby neighbors — spreads swarm spectrally |
| `ocean_alignment` | 0–1 | 0.3–0.7 | How strongly particles match flock velocity — smooths collective motion |
| `ocean_cohesion` | 0–1 | 0.3–0.7 | How strongly particles pull toward flock centroid — tightens the school |
| `ocean_tether` | 0–1 | 0.3–0.8 | Attractor strength toward MIDI note — lower = near-unpitched drift |
| `ocean_scatter` | 0–1 | 0.1–0.4 | Velocity-proportional note-on perturbation — harder hits scatter further |
| `ocean_subflocks` | 1–4 | 2–4 | Sub-flocks at 1×/2×/1.5×/3× MIDI frequency — emergent chord cluster |
| `ocean_damping` | 0–1 | 0.3–0.7 | Velocity damping on particles — higher = slower, more sustained drift |
| `ocean_waveform` | 0–3 | 0–1 | Per-particle: 0=Sine, 1=Saw (PolyBLEP), 2=Pulse (PolyBLEP), 3=Noise |
| `ocean_swarmEnvAttack` | 0–5s | 0.05–2.5 | Time for boid forces to build to full strength after note-on |
| `ocean_swarmEnvDecay` | 0–5s | 0.1–1.0 | Time for boid forces to fall from peak to sustain level |
| `ocean_swarmEnvSustain` | 0–1 | 0.4–1.0 | Sustained boid force level. 0 = school dissolves, particles drift freely |
| `ocean_swarmEnvRelease` | 0–5s | 0.3–2.0 | Time for boid forces to extinguish after note-off |
| `ocean_lfo1Rate` | Hz | 0.01–10 | LFO1 rate — modulates separation |
| `ocean_lfo1Depth` | 0–1 | 0.1–0.4 | LFO1 depth on separation — breathing swarm |
| `ocean_lfo2Rate` | Hz | 0.01–10 | LFO2 rate — modulates cohesion |
| `ocean_lfo2Depth` | 0–1 | 0.1–0.3 | LFO2 depth on cohesion — tidal pull |

### Coupling
- **Sends:** Post-limiter stereo swarm audio
- **Receives:** AudioToFM (velocity perturbation on all particles), AmpToFilter (modulates cohesion), RhythmToBlend (triggers **murmuration** — cascade reorganization of all 128 particles)
- **Best as source for:** Opal (swarm granulated), Overdub (swarm through tape delay)

### The Chromatophore (B013 Blessed)
Aftertouch (channel pressure) boosts separation ×0.25 — pressing harder makes particles scatter faster, exactly like a cephalopod accelerating chromatophore cycles under stress. Mod wheel (CC1) boosts cohesion ×0.4 — tightening the school. The two gestures are physical opposites: pressure scatters, wheel contracts.

### Murmuration
`RhythmToBlend` coupling triggers a cascade reorganization: a perturbation wave propagates from particle 0 through all 128 particles with 0.97× attenuation per particle. The first particles are maximally disrupted; the last barely moved. ONSET → OCEANIC via RhythmToBlend: every drum hit sends a murmuration wave through the school.

### Recommended Pairings
- **+ Onset:** RhythmToBlend from Onset triggers murmuration — drums reorganize the swarm
- **+ Overdub:** Swarm audio through tape delay — organic dub textures
- **+ Opal:** Swarm output granulated — double-emergent cloud

### Starter Recipes
**Boid School:** ocean_subflocks=4, ocean_tether=0.6, ocean_separation=0.4, ocean_cohesion=0.5, ocean_waveform=0 (Sine)
**Chromatophore Pad:** ocean_separation=0.3, ocean_tether=0.7, ocean_cohesion=0.5 — use aftertouch for color
**Slow Cohere:** ocean_swarmEnvAttack=2.5, ocean_swarmEnvSustain=0.8, ocean_separation=0.5, ocean_cohesion=0.4
**Noise Flock:** ocean_waveform=3 (Noise), ocean_subflocks=4, ocean_tether=0.4, ocean_separation=0.6 — self-organizing texture

---

## 19. OPTIC (Optic)
*Visual modulation engine + AutoPulse*

**Accent:** Phosphor Green `#00FF41` | **Prefix:** `optic_` | **Voices:** 0 (modulation only)

### What It Does
Not a synth — a modulation engine. Analyzes incoming audio through 8-band spectrum analysis, generates Winamp/Milkdrop-style visualizations, and feeds modulation signals back into the coupling matrix. AutoPulse mode generates self-evolving thumping trance rhythms without MIDI input.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `optic_autoPulse` | Off/On | On | Enable self-generating rhythmic pulse |
| `optic_pulseRate` | 0.5–16 Hz | 1–4 | Pulse rate. 2 Hz = 120 BPM quarter note. |
| `optic_pulseShape` | 0–1 | 0.1–0.3 | 0 = sharp kick, 1 = soft swell |
| `optic_pulseEvolve` | 0–1 | 0.3–0.7 | How much spectral analysis morphs the pulse |
| `optic_pulseSubdiv` | 0–1 | 0 or 0.7 | 0=whole, 0.5=8th, 0.7=16th note subdivisions |
| `optic_modDepth` | 0–1 | 0.4–0.7 | Overall modulation output level |
| `optic_reactivity` | 0–1 | 0.6–0.8 | How strongly audio drives the analysis |
| `optic_vizMode` | Scope/Spectrum/Milkdrop/Particles | Milkdrop | Visualization style |

### Coupling
- **Sends:** Composite modulation (ch0/1), envelope (ch2), 8 individual mod channels (ch3-10: pulse, bass, mid, high, centroid, flux, energy, transient)
- **Receives:** AudioToFM, AudioToRing, AudioToWavetable (all feed the analyzer), AmpToFilter, RhythmToBlend
- **Best as source for:** AmpToFilter (rhythmic filter pumping), RhythmToBlend (pulse drives blend on other engines)

### Recommended Pairings
- **+ Overdub:** AutoPulse drives Dub's filter = trance bass pump
- **+ Oblique:** Optic modulates Oblique's filter while Oblique feeds back = disco machine
- **+ Obese:** Optic strobes Fat's filter = club-ready sidechain simulation
- **+ Ouroboros:** Feed chaos into Optic, Optic modulates chaos back = feedback loop

### Starter Recipes
**Trance Pump (pair with bass engine):** autoPulse=On, rate=2.0, shape=0.15, evolve=0.5, couple AmpToFilter at 0.6
**Ambient Spectral Mod:** autoPulse=Off, reactivity=0.8, modMixSpec=0.9, couple via AmpToFilter at 0.3
**Strobe (pair with any engine):** autoPulse=On, rate=4.0, subdiv=0.7, shape=0.05, accent=0.9

---

## 20. OBLIQUE (Oblique)
*Prismatic bounce engine — RTJ × Funk × Tame Impala*

**Accent:** Prism Violet `#BF40FF` | **Prefix:** `oblq_` | **Voices:** 8

### What It Does
Prismatic light bouncing off mirrors. A dual oscillator with wavefolder (grit) feeds a 6-tap "prism" delay where each tap is filtered at a different frequency (literally splitting sound into spectral colors). A bouncing-ball physics engine fires percussive clicks on every note. A 6-stage allpass phaser adds Tame Impala swirl.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `oblq_oscFold` | 0–1 | 0.2–0.5 | Wavefold amount. RTJ grit factor. |
| `oblq_bounceRate` | 20–500 ms | 60–120 | Initial bounce interval. Lower = faster ricochet. |
| `oblq_bounceGravity` | 0.3–0.95 | 0.65–0.8 | Interval shrink per bounce. Lower = faster acceleration. |
| `oblq_bounceDamp` | 0.3–0.95 | 0.7–0.85 | Volume decay per bounce. |
| `oblq_bounceSwing` | 0–1 | 0.1–0.25 | Swing on even bounces. Funk groove. |
| `oblq_prismDelay` | 10–500 ms | 60–150 | Base prism delay time |
| `oblq_prismSpread` | 0–1 | 0.5–0.7 | Time spread between 6 facets |
| `oblq_prismColor` | 0–1 | 0.6–0.8 | Spectral color spread (0=mono, 1=full rainbow) |
| `oblq_prismFeedback` | 0–0.95 | 0.3–0.5 | Kaleidoscopic multiplication. Higher = more reflections. |
| `oblq_phaserDepth` | 0–1 | 0.4–0.7 | Phaser sweep depth. The Tame Impala factor. |
| `oblq_phaserRate` | 0.05–8 Hz | 0.3–0.8 | Phaser LFO rate |

### Coupling
- **Sends:** Stereo audio (ch0/1), envelope (ch2)
- **Receives:** AmpToFilter, AmpToPitch, LFOToPitch, PitchToPitch, EnvToDecay, RhythmToBlend
- **Best as source for:** AmpToFilter (bouncing envelope drives other engines' filters)

### Recommended Pairings
- **+ Overdub:** Oblique's prismatic tops over Dub's sub-bass foundation
- **+ Optic:** Optic's AutoPulse drives Oblique's filter; Oblique feeds Optic's analyzer
- **+ OddfeliX:** Snap transient triggers + Oblique's prism = percussive rainbows
- **+ Obese:** Fat's massive bass + Oblique's bouncing tops = full-spectrum funk

### Starter Recipes
**Disco Stab:** oscWave=Saw, fold=0.25, bounceRate=90, swing=0.2, prismColor=0.75, phaserMix=0.4
**RTJ Industrial:** oscWave=Pulse, fold=0.6, bounceRate=45, gravity=0.6, prismFeedback=0.5
**Tame Impala Phase:** oscWave=Saw, fold=0.15, phaserDepth=0.85, phaserRate=0.35, phaserMix=0.7
**House Of Mirrors:** oscWave=Triangle, prismFeedback=0.85, prismColor=0.95, prismWidth=1.0

---

## Cross-Engine Coupling Quick Reference

### Best Coupling Pairs by Type

| Coupling Type | Best Source → Dest | Why |
|--------------|-------------------|-----|
| AmpToFilter | Optic → any | AutoPulse creates rhythmic filter pumping |
| AmpToPitch | OddfeliX → Odyssey | Snap envelope bends Drift's pitch subtly |
| LFOToPitch | OddOscar → any | Morph's 0.3Hz LFO adds gentle drift |
| EnvToMorph | Obsidian → OddOscar | PD envelope drives wavetable morph position |
| AudioToFM | Ouroboros → any | Chaotic audio as FM source = unique sidebands |
| AudioToRing | Obese → Oblique | Rich harmonic content ring-mods prismatic taps |
| FilterToFilter | Oblong → Oblique | Bob's filter cascaded into Oblique's filter |
| AmpToChoke | OddfeliX → Overdub | Snap ducking the dub bass on hits |
| RhythmToBlend | Optic → Oblique | AutoPulse rhythm drives prism blend |
| EnvToDecay | Optic → Overdub | Visual energy shapes dub delay decay |
| PitchToPitch | Oracle → any | Maqam microtonal pitch applied to partner |
| AudioToWavetable | any → Opal | Feed any engine's audio as granular source |

### Engine Categories for Pairing

**Rhythm engines** (provide rhythmic modulation):
- OPTIC (AutoPulse), ODDFELIX (transient envelope), ONSET (percussion)

**Bass engines** (provide foundation):
- OVERDUB (dub bass), OBESE (massive bass), OBLONG (curious bass)

**Pad engines** (provide sustained texture):
- ODDOSCAR (morph pad), ODYSSEY (drift pad), OPAL (granular cloud)

**Character engines** (add unique timbre):
- OBLIQUE (prismatic bounce), ORIGAMI (spectral fold), ORACLE (stochastic)

**Physical engines** (simulate physics):
- OBSCURA (mass-spring scanned synthesis), OCEANIC (boid swarm synthesis), ORGANON (VFE metabolism synthesis)

**Chaos engines** (generate complexity):
- OUROBOROS (ODE attractor), ORGANON (metabolic entropy), ORACLE (stochastic GENDY walk)

---

## 4-Slot Combo Recipes

XOmnibus supports 4 simultaneous engines. Here are proven combinations:

### The Dub Station
`ODDFELIX + OVERDUB + ODYSSEY + OPTIC`
Snap percussion, dub bass, drift pad, Optic driving filter pumps. Complete dub techno rig.

### The Funk Machine
`OBLIQUE + OVERDUB + OBESE + ODDFELIX`
Prismatic bounce tops, dub bass, fat sub, snap transients. Full-spectrum funk.

### Ambient Laboratory
`ODYSSEY + OPAL + ORGANON + OPTIC`
Drift pad, granular cloud, modal resonance, visual modulation. Evolving ambient.

### Chaos Engine
`OUROBOROS + ORGANON + ORACLE + OPTIC`
Three self-generating systems + visual feedback. Alien generative music.

### Crystal Palace
`OBSIDIAN + ORIGAMI + ORBITAL + OBLIQUE`
Phase distortion, spectral folding, additive partials, prismatic delay. Harmonic architecture.

### World Music Fusion
`ORACLE + OBSCURA + OVERWORLD + OVERBITE`
Maqam tuning, scanned synthesis, chip textures, formant shaping. Cross-cultural synthesis.

### Club Ready
`OPTIC + OBLIQUE + OBESE + ONSET`
AutoPulse trance modulation, prismatic bounce, massive bass, drum synthesis. Dance floor.

### Film Score
`ODYSSEY + OPAL + OCEANIC + OBSCURA`
Drift pad, granular shimmer, string ensemble ambience, bowed strings. Cinematic texture.

### Memory Palace
`OMBRE + OPAL + ODYSSEY + OPTIC`
Dual-narrative ghosts, granular shimmer, drift pad, visual modulation. Sound that remembers and forgets.

---

## 21. OMBRE (XOmbre)
*Dual-narrative synthesis — memory meets perception*

**Accent:** Shadow Mauve `#7B6B8A` | **Prefix:** `ombre_` | **Voices:** 8

### What It Does
A single engine containing two cognitive halves. **Oubli** (forgetting) captures audio into a circular memory buffer that gradually decays — granular reconstruction from dissolving traces. **Opsis** (seeing) is a reactive oscillator shaped by velocity transients — immediate, present-tense sound. The `blend` parameter crossfades between them. The `interference` parameter feeds each half into the other: Opsis output becomes Oubli's memory, and Oubli's ghosts haunt Opsis's pitch.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `ombre_blend` | 0–1 | 0.3–0.7 | Oubli ↔ Opsis balance. 0=pure ghost, 1=pure now. |
| `ombre_interference` | 0–1 | 0.2–0.5 | Cross-modulation between halves. Higher = more haunted. |
| `ombre_memoryDecay` | 0.01–30s | 2–8 | How fast Oubli forgets. Long = persistent ghosts. |
| `ombre_memoryGrain` | 5–500ms | 40–120 | Grain size for memory reconstruction. Small = glitchy, large = smeared. |
| `ombre_memoryDrift` | 0–1 | 0.1–0.4 | Pitch/time drift in memory. Higher = more degraded recall. |
| `ombre_oscShape` | 0/1/2/3 | — | 0=Sine, 1=Saw, 2=Square, 3=Triangle |
| `ombre_reactivity` | 0–1 | 0.3–0.7 | How velocity shapes Opsis transient. Higher = more percussive. |
| `ombre_subLevel` | 0–1 | 0.2–0.4 | Sub oscillator level (one octave down, sine). |
| `ombre_filterCutoff` | 20–20kHz | 2–6 kHz | Shared LP filter cutoff. |

### Coupling
- **Sends:** Envelope level (ch2), audio (ch0-1) — full-spectrum coupling source
- **Receives:** AmpToFilter (envelope → filter), LFOToPitch (pitch drift), AudioToFM (FM on Opsis), AudioToWavetable (external audio → Oubli memory)
- **Best as source for:** AmpToFilter (ghostly sidechain), AudioToWavetable (feed memories to other engines)

### Recommended Pairings
- **+ Opal:** Ombre provides the dissolving memories, Opal adds granular shimmer on top. Haunted ambient.
- **+ Odyssey:** Blend=0.7 (mostly Opsis), interference high — Drift's evolving pad feeds into Oubli's memory buffer via coupling. The pad remembers itself.
- **+ Optic:** AutoPulse modulates Ombre's filter cutoff. Memory buffer responds to visual rhythm.
- **+ Oblique:** Ombre's ghost output through Oblique's prism delay creates recursive memory echoes.
- **+ Organon:** Modal resonance feeds into Oubli memory — struck bells that slowly forget their pitch.

### Starter Recipes
**Ghost Pad:** blend=0.2, memoryDecay=12, memoryGrain=200, interference=0.4, oscShape=Saw, attack=0.5, sustain=0.8, release=2.0, filterCutoff=3000
**Reactive Lead:** blend=0.85, reactivity=0.8, oscShape=Saw, subLevel=0.4, interference=0.15, memoryDecay=1.5, attack=0.005, decay=0.2, sustain=0.7
**Dissolving Bells:** blend=0.5, memoryDecay=6, memoryGrain=30, memoryDrift=0.6, oscShape=Sine, reactivity=0.5, interference=0.5, filterCutoff=6000
**Memory Drone:** blend=0.1, memoryDecay=25, memoryGrain=400, memoryDrift=0.3, interference=0.6, oscShape=Triangle, attack=2.0, sustain=1.0, release=4.0

---

## 22. OHM (XOhm)

**Accent:** Sage `#87AE73` | **Prefix:** `ohm_` | **Max voices:** 12 | **Macro axis:** MEDDLING / COMMUNE

### What It Is

OHM is the Hippy Dad of the Constellation Family — a physical modeling ensemble built around a folk-string waveguide core surrounded by eccentric in-laws. At rest it sounds like a single plucked or bowed instrument in a warm room. Push MEDDLING and the in-laws arrive: theremin interference, glass harmonica partials, a granular scatter engine. Push COMMUNE and they start listening to each other.

### DSP Engine

Karplus-Strong waveguide at the core: delay line → damping filter → body resonance filter → output. Body material (Wood/Metal/Gourd/Air) adjusts Q and resonance frequency. `FamilySympatheticBank` adds tuned secondary resonances; `FamilyOrganicDrift` adds continuous slow pitch microvariations. Three interference sources gated by MEDDLING: theremin sine oscillator, glass partial generator, and Hann-windowed grain engine (4.6B-state LCG seed). `OhmObedFM` — 2-operator FM with 8 selectable harmonic ratios (H 1:1 through Pu 11:6) — arrives only when MEDDLING > 0.7.

### Macros

**JAM (M1)** — Dad presence: scales waveguide output level. Automate for solo/verse dynamics.

**MEDDLING (M2)** — In-law threshold: below `ohm_meddlingThresh` all interference is silent; above it, theremin/glass/grain blend in proportionally. Obed FM arrives at 0.7+.

**COMMUNE (M3)** — Merges in-law interference into the Dad signal at low amplitude rather than adding on top. Full COMMUNE absorbs the in-laws into a modulated, beating complex string.

**MEADOW (M4)** — Scales reverb mix and delay feedback simultaneously. Low = dry living room; high = backyard dusk with long echoes.

### Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `ohm_dadInstrument` | 0–8 (choice) | Banjo, Guitar, Mandolin, Dobro, Fiddle (bow), Harmonica, Djembe, Kalimba, Sitar |
| `ohm_pluckBrightness` | 0–1 | Pick attack transient brightness |
| `ohm_bowPressure` | 0–1 | Bow contact pressure (Fiddle mode only) |
| `ohm_bodyMaterial` | 0–3 (choice) | Wood, Metal (Q=8), Gourd, Air |
| `ohm_sympatheticAmt` | 0–1 | Sympathetic string resonance volume |
| `ohm_damping` | 0.8–0.999 | Feedback loop damping — higher = longer sustain |
| `ohm_inlawLevel` | 0–1 | Master level of theremin/glass/grain interference |
| `ohm_thereminScale` | 0.5–2.0 | Pitch ratio of theremin relative to fundamental |
| `ohm_spectralFreeze` | 0–1 | Locks in-law signal — instant drone |
| `ohm_grainSize` | 10–500ms | Grain window length |
| `ohm_obedLevel` | 0–1 | Obed FM carrier amplitude |
| `ohm_fmRatioPreset` | 0–7 (choice) | H 1:1, C 3:2, N 5:4, O 2:1, Fe 5:3, Au 7:4, U 9:5, Pu 11:6 |
| `ohm_fmIndex` | 0–8 | FM modulation depth |
| `ohm_delayTime` | 0.05–2.0s | Echo repeat time |
| `ohm_reverbMix` | 0–1 | Reverb wet level (also scaled by MEADOW) |

### Sound Design Recipes

**Campfire Guitar** — Instrument: Guitar, Body: Wood. JAM 0.7, MEDDLING 0, COMMUNE 0, MEADOW 0.4. Damping 0.996. Clean, warm, slightly reverberant.

**Theremin Séance** — Instrument: Kalimba, Body: Air. JAM 0.5, MEDDLING 0.8, COMMUNE 0.2, MEADOW 0.5. thereminScale 1.5, inlawLevel 0.7. Tine backdrop for wavering ethereal interference.

**Sunday Communion Drone** — Instrument: Harmonica. spectralFreeze 0.85, COMMUNE 0.8, MEDDLING 0.85, MEADOW 0.7, delayTime 1.2s. Hold a note — in-law interference freezes into a crystalline sustained tone.

**The Family Argument** — Body: Metal. JAM 0.6, MEDDLING 1.0, COMMUNE 0.0. FM ratio Pu (11:6), fmIndex 6.0, grainSize 15ms. Everything fighting simultaneously.

### Coupling

Accepts `LFOToPitch` (external pitch mod), `AmpToFilter` (raises damping), `EnvToMorph` (scales exciter intensity). Spectral freeze + COMMUNE makes OHM an always-available drone source for other engines.

---

## 23. ORPHICA (XOrphica)

**Accent:** Siren Seafoam `#7FDBCA` | **Prefix:** `orph_` | **Max voices:** 16 | **Identity:** Microsound harp, siphonophore

### What It Is

ORPHICA is a microsound harp — a waveguide plucked string with a per-voice grain processor that captures the string's own output into a circular buffer and plays it back as overlapping grain clouds. The string and cloud share the same voice and the same pitch. The result sits between a plucked instrument and a dispersed texture: precise in the transient, diffuse and shimmering in the sustain.

### DSP Engine

`PluckExciter` → Karplus-Strong waveguide → `FamilyDampingFilter` → `FamilyBodyResonance` → `FamilySympatheticBank`. String material (Nylon/Steel/Crystal/Light) adjusts damping and brightness. `FamilyOrganicDrift` adds continuous pitch microvariations. Per-voice `OrphicaMicrosound` engine: 8192-sample circular buffer, four simultaneous Hann-windowed grains. Four grain modes: Stutter (immediate repeat), Scatter (randomized read position), Freeze (locked read — instant drone), Reverse (reads backwards). FX chain splits at a configurable crossover note: LOW path (sub-octave, tape saturation, dark delay, deep plate reverb) vs HIGH path (shimmer reverb, micro delay, spectral smear, crystal chorus). LOW leans left, HIGH leans right for natural stereo imaging.

### Macros

**PLUCK (M1)** — Attack transient sharpness and exciter gain. Low = soft airy initiation; high = hard defined transient. Also energizes the sympathetic bank.

**FRACTURE (M2)** — Microsound intensity: adds to `orph_microMix` and increases grain scatter. At zero the grain layer is silent. Above 0.7 produces a grain storm on sustained notes.

**SURFACE (M3)** — Biases the crossover note split toward LOW (warm, reverberant) or HIGH (bright, shimmer, chorus-widened) FX path.

**DIVINE (M4)** — Scales shimmer reverb mix, deep plate mix, and spectral smear simultaneously. The atmosphere macro. Push to dissolve the instrument into a luminous field.

### Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `orph_stringMaterial` | 0–3 (choice) | Nylon, Steel, Crystal, Light |
| `orph_pluckBrightness` | 0–1 | Base pluck transient brightness |
| `orph_pluckPosition` | 0–1 | 0=near bridge (bright), 1=near nut (dark) |
| `orph_stringCount` | 1–6 | Sympathetic strings per voice |
| `orph_bodySize` | 0–1 | Body resonance frequency and Q |
| `orph_sympatheticAmt` | 0–1 | Sympathetic string amplitude |
| `orph_damping` | 0.8–0.999 | Feedback loop damping |
| `orph_microMode` | 0–3 (choice) | Stutter, Scatter, Freeze, Reverse |
| `orph_microRate` | 0.5–50 Hz | Grain trigger rate |
| `orph_microSize` | 5–200ms | Grain window length |
| `orph_microDensity` | 1–20 | Grain overlap count |
| `orph_microScatter` | 0–1 | Position randomization in Scatter mode |
| `orph_microMix` | 0–1 | Grain cloud dry/wet |
| `orph_crossoverNote` | 36–84 (MIDI) | LOW/HIGH FX path split point |
| `orph_subAmount` | 0–1 | Sub-octave sine in LOW path |
| `orph_shimmerMix` | 0–1 | Shimmer reverb in HIGH path |
| `orph_spectralSmear` | 0–1 | Granular dissolve on HIGH path output |

### Sound Design Recipes

**The Harp** — Material: Nylon. PLUCK 0.5, FRACTURE 0, SURFACE 0.5, DIVINE 0.25. Micro mix 0. Transparent, pitched, natural decay.

**Frozen Lake** — Micro mode: Freeze. PLUCK 0.3, FRACTURE 0.8, SURFACE 0.3, DIVINE 0.7. Micro mix 0.9, size 120ms, density 12. Play a chord, freeze it — drone persists under new notes.

**Shattered Glass** — Material: Light, mode: Scatter. PLUCK 0.9, FRACTURE 0.9, SURFACE 0.8. Scatter 0.8, rate 25 Hz, size 20ms. Clear initial note disperses into pitch-smeared debris.

**Crystal Delay** — Material: Crystal. PLUCK 0.7, FRACTURE 0, SURFACE 0.7, DIVINE 0.6. shimmerMix 0.5. Long glassy sustain with octave-up ghost from shimmer verb.

### Coupling

Accepts `LFOToPitch`, `AmpToFilter` (increases damping), `EnvToMorph` (scales pluck intensity). 16 voices and the dual LOW/HIGH stereo spread make ORPHICA a natural coupling source for OPAL or OVERWORLD's ERA crossfade.

---

## 24. OBBLIGATO (XObbligato)

**Accent:** Rascal Coral `#FF8A7A` | **Prefix:** `obbl_` | **Max voices:** 12 | **Identity:** Dual wind, BOND macro

### What It Is

OBBLIGATO is a dual wind instrument — two brothers sharing a waveguide. Brother A plays flute-family instruments (air jet exciter); Brother B plays reed-family instruments (reed exciter). They share a delay line and sympathetic resonance bank. The BOND system models their emotional relationship through eight stages (Harmony → Fight → Cry → Forgive → Protect → Transcend), reshaping breath, detune, sympathetic amplitude, and stereo width in real time.

### DSP Engine

Both brothers: same waveguide architecture (delay line → `FamilyDampingFilter` → `FamilyBodyResonance` → `FamilySympatheticBank`), different exciters. Brother A's `AirJetExciter` models air-jet/embouchure-hole nonlinearity; `ohm_airFlutterA` adds natural flute vibrato. Brother B's `ReedExciter` models a clamped-end vibrating reed; reed stiffness and bite control flexibility and harmonic content. Per-instrument body resonance ratios and Q values are instrument-specific (e.g., shakuhachi: 1.0× Q=5.0; ocarina: 0.6× Q=6.0; oboe: 1.8× Q=5; bassoon: 0.6× Q=3).

### Voice Routing Modes

Alternate (even→A, odd→B), Split (below C4→A, above→B), Layer (two voices per note, one from each), Round Robin (A-B-A-B), Velocity (soft→A, hard→B). Brother A pans to 0.35 (left of center), Brother B to 0.65 (right). BOND Fight stage pushes them to opposite sides; Transcend collapses to near-mono.

### Macros

**BREATH (M1)** — Global lung capacity: scales both brothers' pressure/breath parameters proportionally. Low = thin/airy; high = dense focused resonance.

**BOND (M2)** — Eight-stage emotional arc. Stage progression: Harmony(0) → Play(0.14) → Annoy(0.28) → Fight(0.43) → Cry(0.57) → Forgive(0.71) → Protect(0.86) → Transcend(1.0). Each stage reshapes detune, panning, sympathetic amplitude, and breath pressure.

**MISCHIEF (M3)** — Detuning chaos: A goes sharp, B goes flat by up to ±8 cents independently of BOND. Low = tight unison; high = beating chorus.

**WIND (M4)** — Band-limited noise floor (~1 kHz cutoff): environmental texture from dry studio to outdoor air.

### Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `obbl_instrumentA` | 0–7 (choice) | Flute, Piccolo, Pan Flute, Shakuhachi, Bansuri, Ney, Recorder, Ocarina |
| `obbl_breathA` | 0–1 | Brother A air pressure |
| `obbl_embouchureA` | 0–1 | Embouchure quality — shapes resonance mode |
| `obbl_airFlutterA` | 0–1 | Natural vibrato via breath flutter |
| `obbl_instrumentB` | 0–7 (choice) | Clarinet, Oboe, Bassoon, Soprano Sax, Duduk, Zurna, Shawm, Musette |
| `obbl_breathB` | 0–1 | Brother B breath pressure |
| `obbl_reedStiffness` | 0–1 | Reed flex — lower is more rubbery |
| `obbl_reedBite` | 0–1 | Reed harmonic edge above stiffness |
| `obbl_voiceRouting` | 0–4 (choice) | Alternate, Split, Layer, Round Robin, Velocity |
| `obbl_bondStage` | 0–1 | Continuous emotional stage (0=Harmony, 1=Transcend) |
| `obbl_bondIntensity` | 0–1 | Scale of all BOND emotional modulations |
| `obbl_bondRate` | 0.01–2.0 | Stage transition smoothing speed |
| `obbl_sympatheticAmt` | 0–1 | Sympathetic resonance amplitude (BOND-modulated) |
| `obbl_fxAChorus` | 0–1 | Brother A air chorus pitch modulation depth |
| `obbl_fxBSpring` | 0–1 | Brother B spring reverb |
| `obbl_macroWind` | 0–1 | Wind noise floor |

### Sound Design Recipes

**Morning Practice** — A: Bansuri, B: Duduk. Routing: Split. BREATH 0.5, BOND 0, MISCHIEF 0.05, WIND 0.4. Two instruments warming up — complementary body ratios in the low-mid register.

**The Argument** — Routing: Alternate. BOND 0.43 (Fight), MISCHIEF 0.8, WIND 0.15. Bond intensity 0.9, rate 0.05 (slow). Brothers detune and pan hard.

**Outdoor Transcendence** — BOND 1.0 (Transcend). A: Flute, B: Clarinet. Layer routing. BREATH 0.6, MISCHIEF 0, WIND 0.7. springReverb 0.5. Beatless unison with high wind ambience.

**Narrative Arc** — Draw BOND automation 0→1 over 8–16 bars at rate 0.01. The full emotional arc glides from Harmony through every stage to Transcend.

### Coupling

Accepts `LFOToPitch`, `AmpToFilter`, `EnvToMorph`. OBBLIGATO's stereo field (brothers panned L/R, widened by BOND fight stage) is a natural modulation source — coupling its amplitude to ORPHICA's `AmpToFilter` creates wind-driven harp decay shaping.

---

## 25. OTTONI (XOttoni)

**Accent:** Patina `#5B8A72` | **Prefix:** `otto_` | **Max voices:** 12 | **Identity:** Triple brass, GROW macro

### What It Is

OTTONI is a brass family portrait — three generations of players. Toddler (conch/vuvuzela/toy trumpet): all pressure and imprecision. Tween (trumpet/cornet/flugelhorn): finding the valve. Teen (French horn/tuba/ophicleide): disciplined, with vibrato. The GROW macro sweeps through these ages in a single gesture, morphing from loose and childlike to full and virtuosic.

### DSP Engine

All three voices share a `LipBuzzExciter` feeding a Karplus-Strong waveguide loop → `FamilyDampingFilter` → `FamilyBodyResonance` → `FamilySympatheticBank`. `LipBuzzExciter` receives an `ageScale` argument derived from GROW that shifts from loose/unfocused (low age) to tight/centered (high age). Teen voice has per-voice `vibPhase` accumulator — vibrato only appears as GROW approaches 1.0 via `growTeen` multiplier. Three-layer blend uses a triangular crossfade: toddler peaks at GROW 0, tween at 0.5, teen at 1.0 — intermediate values genuinely blend two age groups. Foreign Harmonics section: Stretch (delay line lengthened up to 10% — inharmonic partials), Drift (microtonal pitch wavering via sinusoidal delay modulation, up to 2 cents), Cold (body resonance shifted higher with sharper Q, up to +4 Q units).

### Macros

**EMBOUCHURE (M1)** — Global mouth-pressure multiplier: `(0.5 + EMBOUCHURE)` scales all three voices' individual pressure settings. Center = unmodified. Crescendo/decrescendo macro.

**GROW (M2)** — Engine's signature gesture: sweeps toddler→tween→teen via triangular crossfade. Teen vibrato appears only near 1.0. Automate across an entire arrangement for a developmental arc.

**FOREIGN (M3)** — Scales all three exotic deviations: Stretch (flat partials), Drift (microtonal wavering), Cold (nasal body resonance shift). Low = studio brass; high = ethnographic/unresolved.

**LAKE (M4)** — Reverb room size and delay mix (ping-pong stereo, 250ms, 60% feedback). Low = small room; high = alpine lake or cathedral.

### Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `otto_toddlerInst` | 0–5 (choice) | Conch, Shofar, Didgeridoo, Alphorn, Vuvuzela, Toy Trumpet |
| `otto_toddlerPressure` | 0–1 | Toddler lip pressure |
| `otto_tweenInst` | 0–5 (choice) | Trumpet, Alto Sax, Cornet, Flugelhorn, Trombone, Baritone Sax |
| `otto_tweenEmbouchure` | 0–1 | Tween embouchure quality |
| `otto_tweenValve` | 0–1 | Valve modulation — subtle pitch wobble |
| `otto_teenInst` | 0–9 (choice) | French Horn, Trombone, Tuba, Euphonium, Tenor Sax, Dungchen, Serpent, Ophicleide, Sackbut, Bass Sax |
| `otto_teenEmbouchure` | 0–1 | Teen embouchure quality |
| `otto_teenBore` | 0–1 | Bore width — wider lowers damping and darkens tone |
| `otto_teenVibratoRate` | 3–8 Hz | Teen vibrato speed |
| `otto_teenVibratoDepth` | 0–1 | Teen vibrato depth (scales with GROW) |
| `otto_foreignStretch` | 0–1 | Partial detuning via delay line extension |
| `otto_foreignDrift` | 0–1 | Microtonal pitch drift |
| `otto_foreignCold` | 0–1 | Body resonance frequency shift + sharper Q |
| `otto_driveAmount` | 0–1 | Soft-clip (tanh) saturation on summed output |
| `otto_sympatheticAmt` | 0–1 | Sympathetic resonance amplitude |
| `otto_damping` | 0.8–0.999 | Feedback loop damping |

### Sound Design Recipes

**Coming of Age** — Automate GROW 0→1 over 8 bars. Toddler: Toy Trumpet, pressure 0.3. Tween: Cornet, embouchure 0.5. Teen: French Horn, bore 0.4. EMBOUCHURE 0.6, FOREIGN 0, LAKE 0.5.

**Distant Ceremony** — GROW 0.5. Toddler: Didgeridoo, Teen: Dungchen. FOREIGN 0.8, LAKE 0.9. Foreign cold + drift produces a distant ritual sound from no particular culture.

**Young Brass Section** — GROW 0.5. Tween: Flugelhorn, Teen: Euphonium (bore 0.7). Drive 0.15, LAKE 0.4. Warm, slightly unfocused brass pad.

**Old Horn Cold Day** — GROW 0.9. Teen: Serpent, bore 0.8. FOREIGN 1.0, LAKE 0.6. Vibrato rate 3.5 Hz. Strained nasal character of an old player on an old instrument.

### Coupling

Accepts `LFOToPitch`, `AmpToFilter`, `EnvToMorph`. Teen vibrato (pitch-modulating) coupling into OHM's `LFOToPitch` transfers teenage brass vibrato to the folk string — a cross-instrument expressive transfer unique in the fleet. OTTONI's harmonically dense drive-saturated output is well-suited as an audio source into OPAL's granular cloud.

---

## 26. OLE (XOlé)

**Accent:** Hibiscus `#C9377A` | **Prefix:** `ole_` | **Max voices:** 18 | **Identity:** Afro-Latin trio, DRAMA macro

### What It Is

OLE is an Afro-Latin string trio of three aunts — Tres Cubano (Cuba), Berimbau (Brazil), Charango (Andes) — plus three latent husbands (Oud, Bouzouki, Thai Pin) who arrive when DRAMA exceeds 0.7. The Alliance system formalizes disagreement: at any moment two aunts are paired against the third. SIDES sweeps through all three configurations; DRAMA raises the temperature and deploys the husbands.

### DSP Engine

All voices run a pluck/strum exciter into a Karplus-Strong waveguide loop. Voices 0–11 cycle between aunts (voice index mod 3). Voices 12–17 are husband voices, `isHusband` flagged, activated above DRAMA 0.7. Aunt 1 (Tres Cubano): `StrumExciter` — multi-pulse attack. Aunt 2 (Berimbau): `StrumExciter` with `coinPress` bending pitch up to +4 semitones, plus gourd size parameter shifting body resonance. Aunt 3 (Charango): per-voice `tremoloPhase` oscillator (5–25 Hz) amplitude-modulating output. Husbands use `PluckExciter` at 60% brightness average of Aunts 1 and 3. Stereo placement: Aunt 1 → 0.2 (far left), Aunt 2 → 0.5 (center), Aunt 3 → 0.8 (far right). Husbands center at 0.5. ISLA widens all positions proportionally.

### Macros

**FUEGO (M1)** — Strum exciter intensity for all aunts: directly scales `voiceBright × FUEGO` into the StrumExciter. Purely an attack macro — no effect on sustain or decay.

**DRAMA (M2)** — Emotional temperature. Below 0.7: no effect. Above 0.7: husband voices become active, scaling linearly 0→full at DRAMA 1.0. Individual husband levels set the final balance.

**SIDES (M3)** — Rotates through the three alliance configurations. At 0: Aunt 1 isolated, 2+3 paired. At 0.33: Aunt 2 isolated. At 0.67: Aunt 3 isolated. Intermediate values crossfade between adjacent configs. Sweeping in real time rotates dominant voice across stereo field.

**ISLA (M4)** — Stereo width: pushes Aunt 1 further left, Aunt 3 further right while Aunt 2 stays centered. Also reads as outdoor openness without adding reverb.

### Key Parameters

| Parameter | Range | Function |
|-----------|-------|----------|
| `ole_aunt1StrumRate` | 1–30 | Tres Cubano strum rate / attack energy |
| `ole_aunt1Brightness` | 0–1 | Tres Cubano pick brightness |
| `ole_aunt2CoinPress` | 0–1 | Berimbau coin press pitch bend (0 to +4 semitones) |
| `ole_aunt2GourdSize` | 0–1 | Berimbau gourd resonance — larger = darker body |
| `ole_aunt3Tremolo` | 5–25 Hz | Charango tremolo rate |
| `ole_aunt3Brightness` | 0–1 | Charango pick brightness |
| `ole_allianceConfig` | 0–2 (choice) | 1v2+3 / 2v1+3 / 3v1+2 — base alliance position |
| `ole_allianceBlend` | 0–1 | Strength of alliance gain differential |
| `ole_husbandOudLevel` | 0–1 | Oud level when DRAMA > 0.7 |
| `ole_husbandBouzLevel` | 0–1 | Bouzouki level when DRAMA > 0.7 |
| `ole_husbandPinLevel` | 0–1 | Thai Pin level when DRAMA > 0.7 |
| `ole_sympatheticAmt` | 0–1 | Sympathetic resonance shared across all voices |
| `ole_damping` | 0.8–0.999 | Feedback loop decay |
| `ole_driftRate` | 0.05–0.5 Hz | Organic drift rate |
| `ole_driftDepth` | 0–20 cents | Organic drift depth |

### Sound Design Recipes

**The Aunts at Rest** — FUEGO 0.4, DRAMA 0, SIDES 0, ISLA 0.4. A1: bright 0.6, strum 6. A2: coinPress 0, gourd 0.5. A3: tremolo 10 Hz. Alliance blend 0.4. Balanced trio.

**Full Drama** — FUEGO 0.8, DRAMA 1.0, SIDES 0.3, ISLA 0.7. All husband levels 0.7. Full 6-voice cast — aunts in the outer field, husbands darkening the center.

**Berimbau Solo** — Aunt 1 + 3 levels 0. DRAMA 0. FUEGO 0.7. Alliance blend 0 (no differentiation). coinPress automated 0→0.8 over 4 beats. GourdSize 0.8. Pure coin-press pitch slides.

**Rotating Stage** — SIDES automated 0→1 over 8 bars. ISLA 0.6, alliance blend 0.7. Dominant voice rotates left→center→right without changing any notes.

### Coupling

Accepts `LFOToPitch`, `AmpToFilter`, `EnvToMorph`. Best coupling use: route OBBLIGATO's amplitude into OLE's `EnvToMorph` — wind duet dynamics drive strum intensity in real time. OLE's wide stereo field (especially at high ISLA) makes it a natural L/R anchor in a four-engine XOmnibus setup.
