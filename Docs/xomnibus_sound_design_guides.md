# XOmnibus — Sound Design Guide
*Per-engine reference for sound designers, preset builders, and performers.*
*Covers the first 20 engines: features, key parameters, coupling strategies, and recommended pairings.*
*5 Constellation engines (OHM/ORPHICA/OBBLIGATO/OTTONI/OLE) have dedicated synthesis guides in Docs/ (e.g. ohm_synthesis_guide.md) but are not yet in this unified guide.*
*Engines not yet covered in this guide: OHM, ORPHICA, OBBLIGATO, OTTONI, OLE, OCELOT, OSTINATO, OPENSKY, OCEANDEEP, OUIE.*

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

**Accent:** Electric Blue `#0066FF` | **Prefix:** `onset_` | **Voices:** 8 (fixed kit)
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
| `onset_vN_pitch` | Hz | varies | Base frequency per voice. The size of the splash. |
| `onset_vN_decay` | ms | 50–300 | How long the ripple lasts. Tight clicks to resonant toms. |
| `onset_vN_blend` | 0–1 | 0.3–0.6 | Layer X↔O crossfade. Analog circuit ↔ algorithmic synthesis. The magic zone. |
| `onset_vN_algo` | 0–3 | — | 0=FM, 1=Modal, 2=KS, 3=PhaseDist — which algorithm Layer O uses |
| `onset_vN_snap` | 0–1 | 0.3–0.7 | Transient intensity. How sharp the impact. feliX's dart as a drum parameter. |
| `onset_vN_body` | 0–1 | 0.3–0.6 | Resonant sustain. How much the water rings after impact. |
| `onset_vN_character` | 0–1 | 0.2–0.5 | Layer O character amount. How alien the splash sounds. |
| `onset_vN_tone` | 0–1 | 0.3–0.7 | Per-voice filter. The surface tension — how much high frequency passes. |
| `onset_xvc_amount` | 0–1 | 0.1–0.4 | Cross-Voice Coupling intensity. How reactive the school is. |

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
**Body Drop:** V1 pitch=55, decay=200, blend=0.0, snap=0.7, body=0.5 — a body entering water from height, the BridgedT circuit at its purest
**Breaking Wave:** V2 pitch=180, decay=120, blend=0.5, algo=1 (Modal), snap=0.5 — wave on rock, circuit model meeting modal resonance
**Rain Patter:** V3 pitch=8000, decay=25, blend=0.3, character=0.4, tone=0.7 — light rain on the ocean surface, six inharmonic squares
**School Reaction:** xvc_amount=0.4, V1 kick pumps V2 snare filter, V3 chokes V4 — the school: each splash triggers the next
**Alien Percussion:** blend=0.8 across all voices, algo=0 (FM), character=0.6 — Layer O dominant, every hit an FM flash from the deep

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
| `opal_grainDensity` | 1–32 | 8–16 | Grains per cloud. Higher = denser, more continuous. |
| `opal_positionScatter` | 0–1 | 0.1–0.4 | Randomize grain read position. Higher = more chaotic. |
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

**Accent:** Warm Red `#FF6B6B` | **Prefix:** `orbital_` | **Voices:** 6

### What It Does
Pure additive synthesis: 64 harmonic partials per voice, each with independent amplitude. Formant filter shapes the partial envelope with spectral tilt and vowel modes. Clean, precise, and endlessly sculptable.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `orbital_partialTilt` | -1 to +1 | -0.3 to +0.3 | Spectral tilt. Negative = dark, positive = bright. |
| `orbital_oddEven` | 0–1 | 0.5 | Balance between odd and even harmonics |
| `orbital_formantMode` | tilt/vowels | — | Spectral shaping mode |
| `orbital_partialCount` | 1–64 | 16–48 | How many partials are active |

### Coupling
- **Sends:** Stereo audio (ch0/1)
- **Receives:** AudioToWavetable (replace/blend partial mix from external source)
- **Best as source for:** AudioToFM (pure harmonics as clean FM source)

### Recommended Pairings
- **+ Oblong:** Additive purity + Bob's curiosity = complementary harmonic textures
- **+ Origami:** Orbital's clean partials → Origami's spectral folding = controlled chaos
- **+ Obscura:** Additive output scanned through Obscura's spring chain

### Starter Recipes
**Organ:** partialCount=32, tilt=0, oddEven=0.7 (mostly odd)
**Bright Bell:** partialCount=48, tilt=0.5, decay=0.8, sustain=0.0
**Vowel Pad:** formantMode=vowel, vowelMorph=0.5, partialCount=64

---

## 12. ORGANON (Organon)
*Self-feeding modal synthesis with entropy analysis*

**Accent:** Bioluminescent Cyan `#00CED1` | **Prefix:** `organon_` | **Voices:** 4

### What It Does
A 32-mode resonator network that feeds back into itself. Shannon entropy analysis of its own output drives the behavior — when the sound becomes predictable, it mutates. Creates sounds that seem alive: metallic bells, singing bowls, self-evolving drones.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `organon_entropy` | 0–1 | 0.3–0.6 | How much entropy analysis affects modal behavior |
| `organon_modeSpread` | 0–1 | 0.4–0.7 | Frequency spread across 32 modes |
| `organon_feedback` | 0–1 | 0.3–0.6 | Self-feeding intensity. Higher = more alive. |
| `organon_damping` | 0–1 | 0.2–0.5 | How quickly modes decay |

### Coupling
- **Sends:** Modal synthesis output (ch0/1)
- **Receives:** AudioToFM (external audio perturbs modes), AudioToWavetable (drives modes)
- **Best as source for:** AudioToWavetable (its evolving harmonics are great source material)

### Recommended Pairings
- **+ Overbite:** Formant-shaped excitation → modal resonance = vocal metal
- **+ Ouroboros:** Two self-feeding engines coupled = chaos system
- **+ Optic:** Entropy-driven modulation visualized in real-time

### Starter Recipes
**Singing Bowl:** modeSpread=0.4, feedback=0.4, entropy=0.3, damping=0.2
**Self-Evolving Drone:** feedback=0.7, entropy=0.6, modeSpread=0.6
**Metallic Bell:** modeSpread=0.8, damping=0.5, feedback=0.2, entropy=0.1

---

## 13. OUROBOROS (Ouroboros)
*Delay-line feedback synthesis — the strange attractor*

**Accent:** Strange Attractor Red `#FF2D2D` | **Prefix:** `ouroboros_` | **Voices:** 6

### What It Does
A 12-tap delay line that feeds back into itself with nonlinear waveshaping. The ouroboros (snake eating its tail) topology creates sounds from nowhere — internal oscillation, ring modulation, and strange attractor behavior. Chaotic but musical.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `ouroboros_feedback` | 0–1 | 0.4–0.7 | Feedback amount. >0.7 = self-oscillation territory. |
| `ouroboros_tapSpread` | 0–1 | 0.3–0.6 | Spread of 12 delay taps |
| `ouroboros_nonlinearity` | 0–1 | 0.2–0.5 | Waveshaping in the feedback path |
| `ouroboros_modDepth` | 0–1 | 0.2–0.4 | Ring modulation depth |

### Coupling
- **Sends:** Feedback output (ch0/1) — chaotic, harmonically rich
- **Receives:** AudioToFM (external audio modulates delay times)
- **Best as source for:** AudioToFM (its chaotic output creates complex FM sidebands)

### Recommended Pairings
- **+ Organon:** Two feedback systems coupled = emergent complexity
- **+ Optic:** Chaotic output drives wild visualizations. Optic modulates back.
- **+ Oblique:** Ouroboros chaos through Oblique's prism = fractured feedback

### Starter Recipes
**Strange Attractor:** feedback=0.6, nonlinearity=0.4, tapSpread=0.5
**Self-Oscillating Drone:** feedback=0.8, tapSpread=0.3, nonlinearity=0.2
**Ring Mod Texture:** feedback=0.4, modDepth=0.6, tapSpread=0.7

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
*Stochastic GENDY + Maqam microtonal synthesis*

**Accent:** Prophecy Indigo `#4B0082` | **Prefix:** `oracle_` | **Voices:** 8

### What It Does
Iannis Xenakis' GENDY algorithm: stochastic breakpoint waveforms where 8-32 breakpoints per cycle undergo random walks. Plus 8 Maqam scales with quarter-tone microtonal tuning. Sounds like prophecy — alien, unpredictable, ancient.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `oracle_breakpoints` | 8–32 | 12–20 | Breakpoints per cycle. More = more complex. |
| `oracle_walkRange` | 0–1 | 0.2–0.5 | Random walk magnitude. Higher = wilder. |
| `oracle_maqamScale` | 0–7 | — | Which Maqam scale (Rast, Bayati, Saba, etc.) |
| `oracle_maqamGravity` | 0–1 | 0.3–0.7 | Blend between 12-TET and Maqam tuning |
| `oracle_distribution` | Cauchy/Logistic | Cauchy | Random walk distribution shape |

### Coupling
- **Sends:** Stochastic audio (ch0/1)
- **Receives:** AudioToFM (perturb breakpoints), AmpToFilter (modulate barriers), EnvToMorph (distribution morph)
- **Best as source for:** AudioToFM (stochastic output as FM source = unique timbres)

### Recommended Pairings
- **+ Obscura:** Stochastic excitation → mass-spring resonance = alien instruments
- **+ Organon:** Two generative systems coupled = emergent world music
- **+ Opal:** Oracle tones granulated by Opal = stochastic cloud

### Starter Recipes
**Xenakis Drone:** breakpoints=16, walkRange=0.3, maqamGravity=0.0
**Maqam Lead:** breakpoints=12, walkRange=0.15, maqamScale=Rast, maqamGravity=0.8
**Chaos Texture:** breakpoints=32, walkRange=0.6, distribution=Cauchy

---

## 17. OBSCURA (Obscura)
*Scanned synthesis — mass-spring physics*

**Accent:** Daguerreotype Silver `#8A9BA8` | **Prefix:** `obscura_` | **Voices:** 8

### What It Does
A 128-mass spring chain simulated with Verlet integration. Excite the chain with Gaussian impulses or bowing, then "scan" it at audio rate (like a pickup on a vibrating string). Different boundary modes (Fixed/Free/Periodic) create radically different timbres.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `obscura_stiffness` | 0–1 | 0.3–0.6 | Spring stiffness. Higher = brighter, metallic. |
| `obscura_damping` | 0–1 | 0.1–0.3 | Energy dissipation. Lower = longer sustain. |
| `obscura_nonlinearity` | 0–1 | 0.1–0.3 | Spring nonlinearity. Adds chaos. |
| `obscura_boundary` | Fixed/Free/Periodic | Fixed | Boundary conditions at chain ends |
| `obscura_excitation` | Impulse/Bow | Impulse | How the chain is excited |

### Coupling
- **Sends:** Stereo scan output (ch0/1) — forward scan L, backward scan R
- **Receives:** AudioToFM (force on chain), AmpToFilter (stiffness modulation), RhythmToBlend (impulse trigger)
- **Best as source for:** AudioToWavetable (scanned waveform as wavetable source)

### Recommended Pairings
- **+ Oracle:** Stochastic excitation of the spring chain = alien resonance
- **+ Obsidian:** Phase distortion + scanned synthesis = complex metallics
- **+ Organon:** Two physical modeling engines coupled = emergent acoustics

### Starter Recipes
**Bowed String:** excitation=Bow, stiffness=0.4, damping=0.15, boundary=Fixed
**Bell Cluster:** excitation=Impulse, stiffness=0.7, nonlinearity=0.2, boundary=Free
**Ambient Drone:** excitation=Bow, stiffness=0.2, damping=0.05, boundary=Periodic

---

## 18. OCEANIC (Oceanic)
*Paraphonic string ensemble — the creature and the eyes that see it*

**Accent:** Phosphorescent Teal `#00B4A0` | **Prefix:** `oceanic_` | **Voices:** 1 (paraphonic — all 128 MIDI notes, constant CPU)

### What It Does
A warm vintage string ensemble with a bioluminescent effects processor that reveals impossible colors hiding inside the harmonic content. Divide-down oscillator bank with 6 registration stops (Violin, Viola, Cello, Bass, Contrabass, Horn), triple-phase BBD ensemble chorus, and a 5-pedal Chromatophore chain (FREEZE, SCATTER, TIDE, ABYSS, MIRROR). The strings are the creature; the pedalboard is the eye that reveals hidden spectral colors.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `oceanic_ensemble` | 0–1 | 0.4–0.7 | Triple-BBD ensemble depth — the Solina soul |
| `oceanic_filterCutoff` | 20–20k Hz | 800–4000 | Shared paraphonic filter — shapes all notes at once |
| `oceanic_pedalMix` | 0–1 | 0.3–0.6 | Dry strings vs chromatophore processing |
| `oceanic_abyssMix` | 0–1 | 0.2–0.4 | Shimmer reverb send (Dattorro + pitch shifting) |
| `oceanic_freezeMix` | 0–1 | 0.0–0.3 | Spectral freeze — holds current harmonic content |
| `oceanic_chromRate` | Hz | 0.2–2 | Chromatophore modulation speed — organic pulsing |

### Coupling
- **Sends:** Post-pedalboard stereo output (the processed string sound)
- **Receives:** AudioToWavetable (any engine through chromatophore chain), AmpToFilter (drum hits sweep the filter), EnvToMorph (external crescendos intensify ensemble), LFOToPitch (cross-engine pitch wander)
- **Best as source for:** Overdub (shimmer strings through dub delay), Opal (string output granulated)

### Recommended Pairings
- **+ Overdub:** Oceanic strings through tape echo — deepest dub pads
- **+ Opal:** String ensemble granulated into time-scattered cloud
- **+ Onset:** Drum transients modulate the string filter — strings breathe with drums

### Starter Recipes
**Solina Foundation:** ensemble=0.5, pedalMix=0, filter=4000Hz — pure dry string ensemble
**Bioluminescent Pad:** ensemble=0.6, pedalMix=0.5, abyssMix=0.3, chromRate=0.4 — glowing shimmer
**Frozen Cathedral:** freezeMix=0.7, abyssMix=0.5, pedalMix=0.8 — suspended infinite strings

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
- OBSCURA (mass-spring), OCEANIC (paraphonic string ensemble), ORGANON (modal resonance)

**Chaos engines** (generate complexity):
- OUROBOROS (feedback), ORGANON (entropy), ORACLE (stochastic walk)

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
