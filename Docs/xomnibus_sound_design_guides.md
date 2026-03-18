# XOmnibus — Sound Design Guide
*Per-engine reference for sound designers, preset builders, and performers.*
*Covers all 34 registered engines: features, key parameters, coupling strategies, and recommended pairings.*
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

**Accent:** Neon Tetra Blue `#00A6D6` | **Prefix:** `snap_` | **Voices:** 8 (polyphony: 1/2/4/8)
**Creature:** The neon tetra — a flash of iridescent blue darting through sunlit shallows
**Polarity:** Pure feliX — the surface, where energy is highest and sounds are sharpest

### What It Does
feliX himself. A **decay-only percussive engine** — there is no attack, sustain, or release parameter. Every note is instantaneous: the pitch sweep fires the moment a note lands, crashing downward (or upward with `snap_sweepDirection`) from the dart height set by `snap_snap`, and the sound decays away with no sustain. This is deliberate. feliX is a transient; he does not linger.

Three oscillator modes give him range: Sine+Noise for body splashes, FM for metallic surface reflections, Karplus-Strong for the pluck of a fishing line breaking the surface. The HPF→BPF filter cascade (Cytomic SVF) is the water itself — `snap_filterEnvDepth` controls how much the decay envelope opens the filter at the transient peak. The unison detune is the school — one fish becomes two or four, darting in slightly different directions.

feliX is the initiator. He strikes first. His decay envelope is the sharpest envelope in the gallery, and his coupling output drives everything downstream.

### Macros (Recommended)
| Macro | Name | Mapping | Aquatic Meaning |
|-------|------|---------|-----------------|
| M1 | **DART** | `snap_macroDart` → `snap_snap` + `snap_decay` inverse | The snap of a tail — how sharp the transient is |
| M2 | **SCHOOL** | `snap_macroSchool` → `snap_unison` + `snap_detune` | The school — how many fish, how spread |
| M3 | **SURFACE** | `snap_macroSurface` → `snap_filterCutoff` + `snap_filterReso` | The water surface — how much light filters through |
| M4 | **DEPTH** | `snap_macroDepth` → `snap_filterEnvDepth` + FX sends | How deep the filter opens on each hit |

### Key Parameters
| Parameter | Range | Default | Sweet Spot | What It Does |
|-----------|-------|---------|------------|-------------|
| `snap_oscMode` | 0/1/2 | 0 | — | 0=Sine+Noise (body splash), 1=FM (metallic reflection), 2=Karplus-Strong (plucked string) |
| `snap_snap` | 0–1 | 0.4 | 0.3–0.6 | Dart intensity — controls pitch sweep height and transient sharpness. The neon flash. |
| `snap_decay` | 0–8s | 0.5 | 0.05–0.5 | How long the ripple lasts. Short for clicks, longer for toms. Skewed (0.3 curve). |
| `snap_sweepDirection` | -1 to +1 | -1.0 | -1.0 | Pitch sweep direction: -1=downward (classic drum), +1=upward (effect snare/pitched tom). |
| `snap_filterCutoff` | 20–20kHz | 2000 | 1–4 kHz | HPF→BPF cascade center. Lower = deeper water; higher = sunlit surface. |
| `snap_filterReso` | 0–1 | 0.3 | 0.2–0.5 | Resonance. Higher = the ring of a droplet hitting still water. |
| `snap_filterEnvDepth` | 0–1 | 0.3 | 0.2–0.6 | How far the decay envelope pushes the filter open at the transient peak. Velocity × depth. |
| `snap_detune` | 0–50 cents | 10 | 5–15 | School spread. Wider = bigger swarm. |
| `snap_unison` | 1/2/4 | 1 | 1–2 | School size — sub-voices per note. 4 = the whole swarm darting at once. |
| `snap_polyphony` | 1/2/4/8 | 4 | 1–4 | How many simultaneous notes. 1 = mono percussion; 4 = melodic use. |
| `snap_level` | 0–1 | 0.8 | 0.7–1.0 | Output level before FX chain. |
| `snap_pitchLock` | on/off | off | — | When on, ignores MIDI note pitch — feliX always fires at the same tuned frequency. Useful for pure percussion. |

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
**Tail Snap:** snap_snap=0.8, snap_decay=0.15, snap_oscMode=0, snap_filterCutoff=400, snap_sweepDirection=-1 — feliX's signature dart
**Surface Pluck:** snap_snap=0.5, snap_decay=0.3, snap_oscMode=2 (KS), snap_filterCutoff=3000, snap_filterReso=0.4 — a fishing line breaking the surface
**Metallic Flash:** snap_snap=0.6, snap_decay=0.25, snap_oscMode=1 (FM), snap_filterCutoff=2000, snap_detune=20 — neon scales catching light
**School Stab:** snap_snap=0.4, snap_decay=0.2, snap_unison=4, snap_detune=12, snap_filterCutoff=3000 — the whole swarm darting at once
**Upward Snare:** snap_snap=0.7, snap_decay=0.18, snap_sweepDirection=+1, snap_oscMode=1 (FM), snap_filterCutoff=3500 — the dart that defies gravity
**Pitched Tone:** snap_snap=0.3, snap_decay=1.5, snap_oscMode=2 (KS), snap_pitchLock=off, snap_filterEnvDepth=0.5 — feliX as melodic instrument

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

---

## 27. OVERLAP (XOverlap)

**Accent:** Bioluminescent Cyan-Green `#00FFB4` | **Prefix:** `olap_` | **Voices:** 6 | **Identity:** Lion's Mane jellyfish — knot-topology Feedback Delay Network synthesizer

### What It Does

OVERLAP is a 6-voice FDN (Feedback Delay Network) synthesizer whose routing matrix morphs between four mathematical knot topologies: Unknot, Trefoil, Figure-Eight, and Torus. Each voice is a pulsing oscillator fed through the shared FDN; the knot geometry determines how delay lines cross-feed each other. Voices couple via Kuramoto hydrodynamic entrainment — when `olap_entrain` is high, voice phases synchronize like bioluminescent pulses rippling through a jellyfish bell. A separate Bioluminescence layer adds shimmer above the FDN mix. Post-FDN processing runs through a Zavalishin TPT state-variable lowpass filter and a chorus/diffusion stage.

### Key Parameters

| Parameter | Range | Default | What It Does |
|-----------|-------|---------|--------------|
| `olap_knot` | Unknot / Trefoil / Figure-Eight / Torus | Unknot | FDN routing topology |
| `olap_tangleDepth` | 0–1 | 0.4 | Interpolation toward full knot matrix — 0 = identity routing, 1 = full topology |
| `olap_torusP` | 2–7 | 3 | Torus knot P parameter (active when knot = Torus) |
| `olap_torusQ` | 2–7 | 2 | Torus knot Q parameter (active when knot = Torus) |
| `olap_delayBase` | 1–50 ms | 10 ms | FDN delay line base length — sets pitch/density of the resonant network |
| `olap_dampening` | 0–1 | 0.5 | High-frequency energy loss in FDN feedback — 0 = bright ring, 1 = muted |
| `olap_feedback` | 0–0.99 | 0.7 | FDN recirculation — higher values approach infinite sustain |
| `olap_pulseRate` | 0.01–8 Hz | 0.5 Hz | Voice oscillator pulse rate — controls bioluminescence flash rhythm |
| `olap_entrain` | 0–1 | 0.3 | Kuramoto coupling strength — how strongly voices synchronize pulse phase |
| `olap_spread` | 0–1 | 0.7 | Stereo spread of the 6 voice outputs across the -1 to +1 field |
| `olap_voiceMode` | Poly / Mono / Legato | Poly | Voice allocation mode |
| `olap_glide` | 0–500 ms | 0 ms | Portamento time |
| `olap_brightness` | 0–1 | 0.5 | Voice oscillator harmonic content |
| `olap_bioluminescence` | 0–1 | 0.2 | Bioluminescence layer level — parallel shimmer above FDN mix |
| `olap_current` | 0–1 | 0.1 | Ocean current modulation depth applied to voice oscillators |
| `olap_currentRate` | 0.005–0.5 Hz | 0.03 Hz | Ocean current modulation rate |
| `olap_attack` | 1 ms–2 s | 50 ms | Voice amplitude envelope attack |
| `olap_decay` | 10 ms–5 s | 1 s | Voice amplitude envelope decay |
| `olap_sustain` | 0–1 | 0.7 | Voice amplitude envelope sustain |
| `olap_release` | 10 ms–10 s | 2 s | Voice amplitude envelope release |
| `olap_filterCutoff` | 20–20k Hz | 8 kHz | SVF lowpass cutoff frequency |
| `olap_filterRes` | 0–1 | 0.1 | SVF resonance — 0 = Butterworth, 1 = near self-oscillation |
| `olap_filterEnvAmt` | -1–1 | 0.3 | Filter envelope amount — bipolar (negative sweeps downward) |
| `olap_filterEnvDecay` | 10 ms–5 s | 0.5 s | Filter envelope decay time |
| `olap_lfo1Rate` | 0.01–20 Hz | 0.8 Hz | LFO 1 rate |
| `olap_lfo1Shape` | Sine / Triangle / Saw / Square / S&H | Sine | LFO 1 waveform |
| `olap_lfo1Depth` | 0–1 | 0.3 | LFO 1 depth |
| `olap_lfo1Dest` | Tangle / Dampening / Pulse Rate / Delay / Filter / Spread | Tangle | LFO 1 routing destination |
| `olap_lfo2Rate` | 0.01–20 Hz | 0.15 Hz | LFO 2 rate |
| `olap_lfo2Shape` | Sine / Triangle / Saw / Square / S&H | Sine | LFO 2 waveform |
| `olap_lfo2Depth` | 0–1 | 0.2 | LFO 2 depth |
| `olap_lfo2Dest` | Tangle / Dampening / Pulse Rate / Delay / Filter / Spread | Pulse Rate | LFO 2 routing destination |
| `olap_chorusMix` | 0–1 | 0.2 | Post-FDN chorus wet amount |
| `olap_chorusRate` | 0.01–0.5 Hz | 0.08 Hz | Chorus modulation rate |
| `olap_diffusion` | 0–1 | 0.3 | Post-FDN diffusion (smears transients, widens stereo) |

### Macros

| Macro | Parameter | What It Controls |
|-------|-----------|-----------------|
| `olap_macroKnot` (M1 KNOT) | 0–1 | Sweeps knot topology from Unknot → Trefoil → Figure-Eight → Torus while scaling tangle depth |
| `olap_macroPulse` (M2 PULSE) | 0–1 | Simultaneously raises pulse rate (0.01→8 Hz) and spread (0.3→1.0) |
| `olap_macroEntrain` (M3 ENTRAIN) | 0–1 | Sets entrainment coupling strength and raises FDN feedback proportionally |
| `olap_macroBloom` (M4 BLOOM) | 0–1 | Raises bioluminescence depth and opens filter cutoff |

### Expression

| Controller | Destination Options |
|-----------|-------------------|
| Mod Wheel (`olap_modWheelDest`) | Tangle / Entrain / Bioluminescence / Filter |
| Aftertouch (`olap_atPressureDest`) | Tangle / Entrain / Brightness / Pulse Rate |

### Coupling

OVERLAP accepts 7 coupling types from upstream engines:

| Incoming Type | Effect Inside OVERLAP |
|--------------|----------------------|
| `AudioToFM` | Modulates FDN delay base — audio-rate FM of the resonant network |
| `AudioToRing` | Ring-modulates the final stereo output |
| `AmpToFilter` | Opens filter cutoff proportionally to upstream amplitude |
| `EnvToMorph` | Pushes tangle depth toward more knotted topology |
| `LFOToPitch` | Maps pitch semitones to tangle depth perturbation |
| `PitchToPitch` | Same as LFOToPitch — semitone offset becomes tangle perturbation |
| `FilterToFilter` | Multiplicative filter cutoff shift from upstream filter state |

### The Knot Topology System

The FDN routing matrix is determined by `olap_knot` combined with `olap_tangleDepth`. At tangleDepth 0, all knot types reduce to the identity matrix (direct pass-through). As depth rises, the matrix morphs toward the full knot recirculation pattern:

- **Unknot** — simple loop topology; each delay feeds the next in a ring
- **Trefoil** — three-crossing alternating feedback; produces beating between voice pairs
- **Figure-Eight** — two-loop crossing at the center; creates a characteristic double-resonance density
- **Torus** — defined by P and Q parameters (`olap_torusP`, `olap_torusQ`); delay ratios are derived from the torus winding number, producing inharmonic frequency lattices at high feedback

The KNOT macro sweeps through all four sequentially — use it as the primary timbral morphing control for performances.

### Sound Design Recipes

**Bioluminescent Pad** — `olap_knot` Trefoil, tangleDepth 0.5, feedback 0.75, entrain 0.4, pulseRate 0.3 Hz, bioluminescence 0.4, filterCutoff 6 kHz, attack 400 ms, release 4 s. Mod wheel → Bioluminescence. Aftertouch → Entrain. Result: slow pulsing chordal shimmer that tightens phase-lock under pressure.

**Deep Tangle Drone** — KNOT macro 0.9 (Torus territory), delayBase 30 ms, feedback 0.92, dampening 0.7, entrain 0.8, BLOOM 0.6. Voices lock into a single breathing unit. Hold a low note for 8 bars and automate KNOT 0→1.

**FDN Percussion** — `olap_attack` 1 ms, decay 80 ms, sustain 0, release 100 ms, filterEnvAmt 0.9, filterEnvDecay 60 ms, knot Figure-Eight, delayBase 5 ms, feedback 0.4. PULSE macro 0.7. Short metallic transients with topology-defined pitch.

**Synchronized Pulse Field** — PULSE macro 0.5 (pulseRate ~4 Hz), ENTRAIN macro 1.0, spread 1.0, bioluminescence 0.3, filterCutoff 12 kHz. All 6 voices lock in phase and pulse together as a unified rhythmic texture.

---

## 28. OUTWIT (XOutwit)

**Accent:** Chromatophore Amber `#CC6600` | **Prefix:** `owit_` | **Voices:** Mono (4-note poly via 8 independent arms) | **Identity:** Giant Pacific Octopus — 8-arm Wolfram cellular automaton synthesizer

### What It Does

OUTWIT is an 8-arm synthesizer where each arm is an independent Wolfram elementary cellular automaton running its own Rule (0–255). The CA generates a 1D binary pattern; the arm reads pattern density and transition events to drive an oscillator, filter, and pitch. Arms are not merely layered — they communicate: the SYNAPSE parameter couples arm N's step event into arm N+1's state as an inter-arm excitation signal, creating a ring of eight distributed intelligence units. A global Amp Envelope shapes the composite sound per note. The SOLVE macro activates a genetic-algorithm hunt toward a target sonic DNA (6 descriptors: brightness, warmth, movement, density, space, aggression). An Ink Cloud module adds a separate decay tail on note trigger. Den Reverb is the on-board spatial processor.

### Per-Arm Parameters (x8, prefix `owit_arm0` through `owit_arm7`)

Each arm has 7 parameters using the pattern `owit_armN{Param}`:

| Parameter suffix | Range | Default | What It Does |
|-----------------|-------|---------|--------------|
| `Rule` | 0–255 | 110, 30, 90, 184, 60, 45, 150, 105 | Wolfram elementary CA rule — determines pattern evolution |
| `Length` | 4–64 cells | 16 | CA row width — wider rows produce denser, slower-cycling patterns |
| `Level` | 0–1 | 0.7 | Arm output amplitude |
| `Pitch` | -24 to +24 semitones | 0 | Semitone offset from root note for this arm |
| `Filter` | 20–20k Hz | 4 kHz | Per-arm lowpass filter cutoff |
| `Wave` | Saw / Pulse / Sine | Saw | Oscillator waveform for this arm |
| `Pan` | -1 to +1 | Arms spread L→R: -0.8 to +0.9 | Stereo position; default spreads arms across the field |

### Global Parameters

| Parameter | Range | Default | What It Does |
|-----------|-------|---------|--------------|
| `owit_stepRate` | 0.01–40 Hz | 4 Hz | CA clock rate — how fast all arms advance one cellular generation |
| `owit_stepSync` | on/off | off | Sync step rate to host tempo |
| `owit_stepDiv` | 1/32 to 2/1 | 1/8 | Step division when synced to host |
| `owit_synapse` | 0–1 | 0.2 | Inter-arm coupling strength — arm N step event excites arm N+1 |
| `owit_chromAmount` | 0–1 | 0.5 | Chromatophore modulation depth — CA pattern modulates timbre |
| `owit_solve` | 0–1 | 0 | SOLVE base activation level for the GA hunt |
| `owit_huntRate` | 0–1 | 0.3 | Speed of the SOLVE genetic algorithm search |
| `owit_targetBrightness` | 0–1 | 0.5 | SOLVE target: desired brightness descriptor |
| `owit_targetWarmth` | 0–1 | 0.5 | SOLVE target: desired warmth descriptor |
| `owit_targetMovement` | 0–1 | 0.5 | SOLVE target: desired movement descriptor |
| `owit_targetDensity` | 0–1 | 0.5 | SOLVE target: desired density descriptor |
| `owit_targetSpace` | 0–1 | 0.5 | SOLVE target: desired space descriptor |
| `owit_targetAggression` | 0–1 | 0.5 | SOLVE target: desired aggression descriptor |
| `owit_inkCloud` | 0–1 | 0 | Ink Cloud trigger level — noise burst on note-on |
| `owit_inkDecay` | 10–500 ms | 80 ms | Ink Cloud decay time |
| `owit_triggerThresh` | 0–1 | 0.3 | CA pattern density threshold that gates arm output |
| `owit_masterLevel` | 0–1 | 0.8 | Overall output level before soft limiter |
| `owit_ampAttack` | 1 ms–2 s | 10 ms | Global amp envelope attack |
| `owit_ampDecay` | 1 ms–2 s | 200 ms | Global amp envelope decay |
| `owit_ampSustain` | 0–1 | 0.8 | Global amp envelope sustain |
| `owit_ampRelease` | 1 ms–4 s | 300 ms | Global amp envelope release |
| `owit_filterRes` | 0–1 | 0.2 | Filter resonance shared across all arms |
| `owit_filterType` | LP / BP / HP | LP | Filter mode applied to each arm |
| `owit_denSize` | 0–1 | 0.4 | Den Reverb room size |
| `owit_denDecay` | 0–1 | 0.4 | Den Reverb decay time |
| `owit_denMix` | 0–1 | 0.2 | Den Reverb wet/dry mix |
| `owit_voiceMode` | Poly / Mono | Poly | Voice allocation |
| `owit_glide` | 0–1 | 0 | Portamento amount |

### LFOs

| Parameter | Range | Default | Notes |
|-----------|-------|---------|-------|
| `owit_lfo1Rate` | 0.01–20 Hz | 1 Hz | LFO 1 rate |
| `owit_lfo1Depth` | 0–1 | 0 | LFO 1 depth |
| `owit_lfo1Shape` | Sine / Triangle / Saw / Square / S&H | Sine | LFO 1 waveform |
| `owit_lfo1Dest` | StepRate / FilterCutoff / ChromAmount / ArmLevels | StepRate | LFO 1 destination |
| `owit_lfo2Rate` | 0.01–20 Hz | 0.3 Hz | LFO 2 rate |
| `owit_lfo2Depth` | 0–1 | 0 | LFO 2 depth |
| `owit_lfo2Shape` | Sine / Triangle / Saw / Square / S&H | Sine | LFO 2 waveform |
| `owit_lfo2Dest` | StepRate / FilterCutoff / ChromAmount / ArmLevels | FilterCutoff | LFO 2 destination |

### Macros

| Macro | Parameter | What It Controls |
|-------|-----------|-----------------|
| `owit_macroSolve` (M1 SOLVE) | 0–1 | Activates and intensifies the GA hunt toward the target DNA descriptors |
| `owit_macroSynapse` (M2 SYNAPSE) | 0–1 | Adds up to +0.6 to synapse coupling — pushes arms toward collective behavior |
| `owit_macroChromatophore` (M3 CHROMATOPHORE) | 0–1 | Adds up to +0.7 to chromAmount — maximum CA-driven timbre modulation |
| `owit_macroDen` (M4 DEN) | 0–1 | Simultaneously raises denSize, denDecay, and denMix — fills the octopus's den |

### Expression

Mod wheel adds up to +0.4 to `owit_synapse` — playing pressure increases inter-arm coupling, tightening the octopus's distributed intelligence. Aftertouch adds up to +0.3 to `owit_chromAmount` — pressure increases chromatophore modulation depth.

### Coupling

OUTWIT accepts 9 coupling types from upstream engines:

| Incoming Type | Effect Inside OUTWIT |
|--------------|---------------------|
| `AudioToFM` | Modulates step rate (±20 Hz) — upstream audio drives CA clock speed |
| `AmpToFilter` | Opens all arm filter cutoffs by up to +8 kHz |
| `EnvToMorph` | Increases chromatophore depth (up to +0.7) |
| `LFOToPitch` | Applies semitone pitch offset across all arms (±12 semitones) |
| `RhythmToBlend` | Increases synapse coupling (up to +0.5) — upstream rhythm tightens inter-arm sync |
| `AmpToChoke` | Scales arm levels down (inverse of amplitude — creates sidechain-like ducking) |
| `AudioToRing` | Ring-modulates arm output amplitude by upstream audio |
| `FilterToFilter` | Adds +4 kHz offset to all arm filter cutoffs |
| `PitchToPitch` | Adds harmony offset (+7 semitones) across all arms |

### The Wolfram CA System

Each arm maintains a 1D cellular array of length `armNLength`. At each CA step (rate = `owit_stepRate`), the engine applies the arm's Rule to generate the next row. Rules are standard Wolfram elementary CA: the next state of each cell is determined by the current state of the cell and its two neighbors, producing one of 256 possible rule tables. Key Rules for sound design:

- **Rule 110** — complex aperiodic patterns (Turing-complete); default arm 0
- **Rule 30** — chaotic, pseudo-random; default arm 1; useful for noise-like textures
- **Rule 90** — Sierpinski triangle fractal; default arm 2; strong rhythmic pulse at power-of-2 lengths
- **Rule 184** — traffic flow model; default arm 3; produces slow-moving density waves
- **Rule 60** — XOR fractal; default arm 4; creates repeating melodic sequences at short lengths
- **Rule 45** — complex/chaotic hybrid; default arm 5
- **Rule 150** — symmetric wave fractal; default arm 6
- **Rule 105** — complex aperiodic; default arm 7

The `owit_triggerThresh` parameter gates each arm: arm output is only produced when pattern density exceeds the threshold, creating natural rhythm from the CA dynamics.

### Sound Design Recipes

**Distributed Pulse** — All arm Rules set to defaults. stepRate 8 Hz, synapse 0.5, chromAmount 0.6, arm pitches: 0, 7, 12, 19, 24, 7, -12, -5 (octave + fifth spread). SYNAPSE macro 0.6. Arms phase-lock via synapse and produce a rhythmically interlocked harmonic stack.

**CA Drone** — Arms 0–3 active (levels 0.8), arms 4–7 at 0. stepRate 0.5 Hz, arm lengths 48–64, triggerThresh 0.1. DEN macro 0.7. Very slow CA evolution with dense sustain and full reverb — meditative texture.

**SOLVE Hunt** — Set target DNA: brightness 0.8, warmth 0.3, movement 0.9, density 0.6, space 0.4, aggression 0.7. SOLVE macro 0 → 1 over 8 bars. The GA progressively mutates arm rules and parameters toward the target — run live as a performance gesture.

**Ink Cloud Attack** — inkCloud 0.8, inkDecay 200 ms, ampAttack 1 ms, ampDecay 50 ms, ampSustain 0. Every note trigger fires a noise burst (ink) followed by the arm CA texture decaying quickly. Percussion from distributed intelligence.

**Synapse Ring** — stepRate 12 Hz, synapse 1.0 (SYNAPSE macro 1.0), arm pitches 0/2/4/7/9/12/14/16 (D major scale offsets). triggerThresh 0.2. Each arm immediately triggers the next; the ring propagates a traveling wave across the stereo field.

---

## OSPREY (Osprey)
*Turbulence-Modulated Resonator Synthesis — the surface hunter*

**Accent:** Azulejo Blue `#1B4F8A` | **Prefix:** `osprey_` | **Voices:** 8 (polyphonic, LRU stealing)
**Creature:** The osprey hunts at the boundary between air and ocean — the only raptor that plunges into water. XOsprey lives in the Surface zone: turbulent interface where wind meets wave, where resonance is born from chaos.
**Polarity:** Balanced feliX/Oscar — lives at the threshold between the two worlds
**Blessing:** B012 ShoreSystem — 5-coastline cultural data shared across engines

### What It Does
Turbulence-modulated modal synthesis shaped by five coastal resonator profiles (Atlantic, Nordic, Mediterranean, Pacific, Southern). A physics-inspired FluidEnergyModel replaces traditional oscillators — at low sea states it produces smooth sinusoidal swells; at high states it stacks 4 octaves of Perlin-style noise modeling the ocean energy cascade. Sixteen modal resonators per voice (3 instrument groups × 4 formants + 4 sympathetic strings) are excited by this fluid energy rather than a physical gesture.

Three creature formant voices per voice add the living element — birds, wind, distant calls. The ShoreSystem morphs between five coastal instrument families, smoothly transitioning the timbral DNA of a kora (Atlantic) into a koto (Pacific) as the Shore knob sweeps.

### Macros
| Macro | Name | Mapping |
|-------|------|---------|
| M1 | **CHARACTER** | `osprey_macroCharacter` — Shore morphing + resonator brightness |
| M2 | **MOVEMENT** | `osprey_macroMovement` — Creature rate + sea state energy |
| M3 | **COUPLING** | `osprey_macroCoupling` — Sympathy amount + coupling send level |
| M4 | **SPACE** | `osprey_macroSpace` — Harbor verb + fog amount |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `osprey_shore` | 0–4 | 1–3 | Coastal resonator profile. 0=Atlantic, 1=Nordic, 2=Mediterranean, 3=Pacific, 4=Southern. |
| `osprey_seaState` | 0–1 | 0.2–0.6 | Fluid energy turbulence. Low=smooth swells, high=chaotic chop. |
| `osprey_swellPeriod` | s | 2–8 | Period of primary wave oscillation. The tempo of the ocean. |
| `osprey_windDir` | 0–1 | varies | Wind direction relative to shore. Shapes excitation character. |
| `osprey_depth` | 0–1 | 0.3–0.7 | Resonator bank depth. How far below the surface the resonance extends. |
| `osprey_resonatorBright` | 0–1 | 0.4–0.7 | Brightness of modal resonators. |
| `osprey_resonatorDecay` | s | 0.5–3.0 | Modal ring time. How long the water body resonates after excitation. |
| `osprey_sympathyAmount` | 0–1 | 0.1–0.4 | Sympathetic string response level. Cross-resonance between partials. |
| `osprey_creatureRate` | Hz | 0.1–2.0 | Creature formant animation rate. How fast the living voice moves. |
| `osprey_creatureDepth` | 0–1 | 0.2–0.5 | Creature formant modulation depth. How much the voice evolves. |
| `osprey_coherence` | 0–1 | 0.3–0.7 | Noise coherence. Low=stochastic, high=tonal. |
| `osprey_foam` | 0–1 | 0.1–0.3 | High-frequency foam texture added to output. |
| `osprey_brine` | 0–1 | 0.1–0.4 | Mid-frequency salt character — saturated tonal weight. |
| `osprey_hull` | 0–1 | 0.2–0.5 | Low-end hull resonance — the body of the vessel. |
| `osprey_filterTilt` | -1–1 | -0.2–0.2 | Tilt filter shelving. Negative=darker, positive=brighter. |
| `osprey_filterEnvDepth` | 0–1 | 0.2–0.5 | Filter envelope depth. Velocity-triggered filter sweep. |
| `osprey_harborVerb` | 0–1 | 0.2–0.5 | Harbor reverb (4-allpass chain). Enclosed water acoustic. |
| `osprey_fog` | 0–1 | 0.1–0.4 | Fog HF diffusion — softens transients and upper partials. |
| `osprey_ampAttack` | ms | 5–50 | Amplitude envelope attack. |
| `osprey_ampDecay` | ms | 50–500 | Amplitude envelope decay. |
| `osprey_ampSustain` | 0–1 | 0.5–0.8 | Amplitude envelope sustain level. |
| `osprey_ampRelease` | ms | 100–1000 | Amplitude envelope release. |
| `osprey_voiceMode` | 0–1 | — | 0=polyphonic, 1=mono legato. |
| `osprey_glide` | 0–2 s | 0–0.2 | Portamento time. |

### Coupling
- **Sends:** Stereo resonator audio, per-voice amplitude envelope output
- **Receives:** AudioToWavetable (external audio feeds the fluid excitation model), AmpToFilter (external amplitude modulates the tilt filter)
- **Best as source for:** AmpToFilter (resonator amplitude drives other engines' filters), AudioToWavetable (coastal resonance feeds granular engines)
- **Ecosystem role:** The surface layer — where energy from above enters the column. Pairs naturally with OSTERIA (its cultural twin) and deep engines that receive surface energy.

### Recommended Pairings
- **+ OSTERIA:** The companion diptych. OSPREY is the ocean; OSTERIA is the human answer to it. Shore values can mirror or diverge.
- **+ Opal:** Osprey's resonant swells frozen into Opal grain clouds. Shore becomes granular texture.
- **+ Overdub:** Osprey's surface turbulence gaining tape delay history. Coastal sound becoming memory.
- **+ Ouroboros:** FluidEnergy excitation feeding the strange attractor. Chaos feeding chaos.
- **+ Onset:** Surface splashes + resonant body. Percussion arrives, the shore absorbs it.

### Starter Recipes
**Atlantic Modal:** osprey_shore=0, osprey_seaState=0.3, osprey_resonatorDecay=2.0, osprey_sympathyAmount=0.3, osprey_harborVerb=0.4 — kora-like resonant pluck, North Atlantic acoustic
**Pacific Storm:** osprey_shore=3, osprey_seaState=0.8, osprey_coherence=0.2, osprey_foam=0.4, osprey_fog=0.3 — violent Pacific turbulence, inharmonic chaos
**Nordic Whisper:** osprey_shore=1, osprey_seaState=0.1, osprey_creatureRate=0.3, osprey_creatureDepth=0.4, osprey_fog=0.5 — still Nordic fjord, creature voices barely present
**Sympathy Web:** osprey_sympathyAmount=0.5, osprey_resonatorDecay=3.0, osprey_depth=0.6 — dense cross-resonance, every partial triggering its neighbors

---

## OSTERIA (Osteria)
*Ensemble Synthesis with Elastic Coupling and Timbral Memory — the shore tavern*

**Accent:** Porto Wine `#722F37` | **Prefix:** `osteria_` | **Voices:** 8 (polyphonic, LRU stealing)
**Creature:** The osteria is where the fisherman goes after the sea. Stories become songs. Strangers become an ensemble. OSTERIA is the human answer to OSPREY's inhuman vastness — communal synthesis, living at the Open Water zone.
**Polarity:** Balanced feliX/Oscar — communal, warm, accumulated history
**Blessing:** B012 ShoreSystem — shared with OSPREY, the OSPREY × OSTERIA diptych

### What It Does
A jazz quartet (Bass, Harmony, Melody, Rhythm) stretched across five coastal cultures via the ShoreSystem. Each voice independently absorbs local folk instrument character through formant resonator banks — Guitarra Portuguesa, Kora, Oud, Shakuhachi, Gamelan. Voices are connected by spring forces: elastic rubber-band coupling pulls them toward a shared centroid, creating tension when stretched and musical unity when tight.

Timbral memory means borrowed influences persist across notes — the quartet accumulates a living history of everywhere it's been. The Tavern Room is an FDN reverb with Householder-like mixing and per-shore absorption. Murmur generates crowd/conversation texture via filtered noise.

### Macros
| Macro | Name | Mapping |
|-------|------|---------|
| M1 | **CHARACTER** | `osteria_macroCharacter` — shore morphing + patina character |
| M2 | **MOVEMENT** | `osteria_macroMovement` — elastic coupling + memory recall |
| M3 | **COUPLING** | `osteria_macroCoupling` — sympathy crossfeed + coupling level |
| M4 | **SPACE** | `osteria_macroSpace` — tavern mix + hall + murmur |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `osteria_qBassShore` | 0–4 | 0–2 | Bass voice shore position. 0=Atlantic, 4=Southern. |
| `osteria_qHarmShore` | 0–4 | varies | Harmony voice shore position. |
| `osteria_qMelShore` | 0–4 | varies | Melody voice shore position. |
| `osteria_qRhythmShore` | 0–4 | varies | Rhythm voice shore position. |
| `osteria_qElastic` | 0–1 | 0.2–0.6 | Spring coupling stiffness. How tightly voices pull toward shared centroid. |
| `osteria_qStretch` | 0–1 | 0.1–0.5 | Spring stretch limit. How far apart voices can wander before snapping back. |
| `osteria_qMemory` | 0–1 | 0.3–0.7 | Timbral memory retention. How long borrowed shore influence persists. |
| `osteria_qSympathy` | 0–1 | 0.1–0.4 | Cross-voice sympathy crossfeed. Harmony bleeds into melody, etc. |
| `osteria_bassLevel` | 0–1 | 0.6–0.9 | Bass voice output level. |
| `osteria_harmLevel` | 0–1 | 0.5–0.8 | Harmony voice output level. |
| `osteria_melLevel` | 0–1 | 0.5–0.8 | Melody voice output level. |
| `osteria_rhythmLevel` | 0–1 | 0.4–0.7 | Rhythm voice output level. |
| `osteria_ensWidth` | 0–1 | 0.4–0.8 | Ensemble stereo width. |
| `osteria_blendMode` | 0–2 | — | Quartet blend mode. 0=independent, 1=spring-centered, 2=unison. |
| `osteria_tavernMix` | 0–1 | 0.2–0.5 | Tavern Room FDN reverb send amount. |
| `osteria_tavernShore` | 0–4 | varies | Tavern room acoustic character (per-shore absorption). |
| `osteria_murmur` | 0–1 | 0.05–0.2 | Crowd murmur texture level. |
| `osteria_warmth` | 0–1 | 0.3–0.6 | Formant warmth — mid-range resonance character. |
| `osteria_oceanBleed` | 0–1 | 0.0–0.2 | Ocean bleed — slight OSPREY-like excitation noise in output. |
| `osteria_patina` | 0–1 | 0.1–0.4 | Harmonic fold character stage. Gentle even-order distortion. |
| `osteria_porto` | 0–1 | 0.1–0.4 | Porto character stage. tanh warmth — soft saturation. |
| `osteria_smoke` | 0–1 | 0.1–0.3 | Smoke character stage. HF haze lowpass — softens upper harmonics. |
| `osteria_filterEnvDepth` | 0–1 | 0.2–0.5 | Velocity-triggered filter envelope depth. |
| `osteria_attack` | ms | 10–100 | Amp envelope attack. |
| `osteria_decay` | ms | 50–500 | Amp envelope decay. |
| `osteria_sustain` | 0–1 | 0.5–0.8 | Amp envelope sustain. |
| `osteria_release` | ms | 100–1000 | Amp envelope release. |
| `osteria_sessionDelay` | 0–1 | 0.1–0.3 | Session delay send amount. |
| `osteria_hall` | 0–1 | 0.1–0.3 | Hall allpass reverb amount. |
| `osteria_chorus` | 0–1 | 0.1–0.3 | Chorus ensemble modulation amount. |
| `osteria_tape` | 0–1 | 0.0–0.2 | Tape saturation/flutter amount. |

### Coupling
- **Sends:** Post-room stereo audio
- **Receives:** AudioToWavetable (any engine becomes a shore the quartet absorbs), AmpToFilter (external dynamics modulate elastic tension)
- **Best as source for:** AudioToWavetable (cultural resonance as source material for granular engines)
- **Ecosystem role:** The shore tavern — acoustic anchor for cultural pairing with any OSPREY instance.

### Recommended Pairings
- **+ OSPREY:** The diptych. Ocean + shore. Shore values mirror or diverge depending on musical intent.
- **+ Overdub:** Ensemble warmth with tape delay history. The tavern's stories echoing into evening.
- **+ Opal:** Osteria's formant output granularized by Opal. Voices frozen into shimmering clouds.
- **+ Oblong:** Osteria ensemble feed into Bob's modal resonance. Folk instruments blooming the reef.
- **+ Odyssey:** Open-water drift pad beneath the coastal ensemble. Depth and surface.

### Starter Recipes
**Mediterranean Duo:** osteria_qBassShore=2, osteria_qHarmShore=2, osteria_qMelShore=2, osteria_qElastic=0.4, osteria_tavernMix=0.3, osteria_porto=0.3 — oud warmth, Mediterranean acoustic, spring coupling tight
**Elastic Stretch:** osteria_qBassShore=0, osteria_qMelShore=4, osteria_qElastic=0.1, osteria_qStretch=0.8 — Atlantic bass vs Southern melody, maximum drift before spring pulls them home
**Memory Accumulation:** osteria_qMemory=0.8, osteria_qSympathy=0.3, osteria_blendMode=1 — quartet accumulates shore history, sympathy crossfeed builds ensemble mind
**Smoke and Porto:** osteria_porto=0.5, osteria_smoke=0.4, osteria_patina=0.2, osteria_murmur=0.15 — character stages full, crowd texture present, late-night tavern warmth

---

## OWLFISH (Owlfish)
*Mixtur-Trautonium Oscillator + Sacrificial Armor — the abyssal organ*

**Accent:** Abyssal Gold `#B8860B` | **Prefix:** `owl_` | **Voices:** 1 (monophonic)
**Creature:** The owlfish lives in the deep abyss — bioluminescent, heavily armored, predating in near-total darkness. Its Mixtur-Trautonium heritage makes it a creature of subharmonic stacks and resonant depth.
**Polarity:** Pure Oscar — the deep, the heavy, the organ of the abyss
**Blessing:** B014 Mixtur-Trautonium Oscillator — unanimous praise from the ghost council, genuinely novel

### What It Does
Monophonic organ synthesizer inspired by the Trautonium's subharmonic synthesis — a fundamental tone with a stack of integer-ratio sub-oscillators (the "Mixtur"). Each sub-oscillator runs at a division of the fundamental: 1/2, 1/3, 1/4, 1/5 — Trautonium subharmonics, not octave doublings. The blend and level of each partial shapes the organ register: thin and reedy at top, massive and bowed at depth.

The Sacrificial Armor system is the owlfish's defense: a compressor-triggered scatter that breaks apart the signal when it comes under pressure, then reconstitutes it. Under soft dynamics, smooth and tonal. Under hard attack, the armor shatters into granular fragments before reforming.

Signal flow: Abyss Habitat (Mixtur oscillator stack) → Owl Optics (filter with keytracking) → Diet (spectral shaping) → Sacrificial Armor (compressor + scatter) → Amp Envelope → Abyss Reverb → Output.

### Macros
| Macro | Name | Mapping |
|-------|------|---------|
| M1 | **DEPTH** | `owl_depth` — mixtur subharmonic presence, how far into the abyss |
| M2 | **FEEDING** | `owl_feeding` — filter resonance + grain mix |
| M3 | **DEFENSE** | `owl_defense` — armor threshold + scatter amount |
| M4 | **PRESSURE** | `owl_pressure` — comp ratio + armor decay |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `owl_mixtur` | 0–1 | 0.3–0.7 | Mixtur subharmonic stack blend. How much of the sub-oscillators are present. |
| `owl_fundWave` | 0–3 | 0–1 | Fundamental waveform. 0=sine, 1=triangle, 2=saw, 3=square. |
| `owl_subWave` | 0–3 | 0–2 | Sub-oscillator waveform (applies to all sub partials). |
| `owl_subDiv1` | 2–16 | 2–4 | First sub-oscillator integer division. |
| `owl_subDiv2` | 2–16 | 3–6 | Second sub-oscillator integer division. |
| `owl_subDiv3` | 2–16 | 4–8 | Third sub-oscillator integer division. |
| `owl_subDiv4` | 2–16 | 5–10 | Fourth sub-oscillator integer division. |
| `owl_subLevel1` | 0–1 | 0.6–0.9 | Level of first sub partial. |
| `owl_subLevel2` | 0–1 | 0.4–0.7 | Level of second sub partial. |
| `owl_subLevel3` | 0–1 | 0.2–0.5 | Level of third sub partial. |
| `owl_subLevel4` | 0–1 | 0.1–0.3 | Level of fourth sub partial. |
| `owl_subMix` | 0–1 | 0.3–0.7 | Overall sub-oscillator mix vs fundamental. |
| `owl_bodyFreq` | Hz | 100–600 | Body resonance frequency. The bioluminescent core tone. |
| `owl_bodyLevel` | 0–1 | 0.2–0.5 | Body resonance level. |
| `owl_filterCutoff` | Hz | 400–4k | Filter cutoff with key tracking. |
| `owl_filterReso` | 0–1 | 0.2–0.6 | Filter resonance. |
| `owl_filterTrack` | 0–1 | 0.5–1.0 | Key tracking amount. 1.0 = full octave tracking. |
| `owl_filterEnvDepth` | 0–1 | 0.2–0.5 | Filter envelope depth (velocity-triggered). |
| `owl_grainSize` | ms | 20–200 | Granular fragment size (used when Armor scatters). |
| `owl_grainDensity` | 1–32 | 4–16 | Grain density during armor scatter. |
| `owl_grainPitch` | 0–1 | 0.0–0.2 | Grain pitch scatter during armor break. |
| `owl_grainMix` | 0–1 | 0.0–0.4 | Grain mix level (0=no scatter, 1=full scatter). |
| `owl_feedRate` | 0–1 | 0.2–0.6 | Diet spectral feed rate — how fast the spectral shaping evolves. |
| `owl_armorThreshold` | dB | -30–0 | Sacrificial Armor trigger threshold. |
| `owl_armorDecay` | ms | 50–500 | Armor scatter decay — how quickly it reconstitutes. |
| `owl_armorScatter` | 0–1 | 0.2–0.7 | Armor scatter intensity when triggered. |
| `owl_armorDuck` | 0–1 | 0.3–0.7 | Armor duck amount — how much fundamental ducks while armor scatters. |
| `owl_armorDelay` | ms | 10–100 | Armor trigger delay — onset hold before scatter fires. |
| `owl_compRatio` | 1–20 | 3–8 | Compressor ratio (feeds armor trigger). |
| `owl_compThreshold` | dB | -40–0 | Compressor threshold. |
| `owl_compAttack` | ms | 1–50 | Compressor attack. |
| `owl_compRelease` | ms | 50–500 | Compressor release. |
| `owl_reverbSize` | 0–1 | 0.4–0.8 | Abyss Reverb size. Large = deep abyssal space. |
| `owl_reverbDamp` | 0–1 | 0.3–0.6 | Reverb damping. High = muffled deep-sea acoustic. |
| `owl_reverbPreDelay` | ms | 0–80 | Reverb pre-delay. |
| `owl_reverbMix` | 0–1 | 0.2–0.5 | Reverb wet mix. |
| `owl_ampAttack` | ms | 10–200 | Amp envelope attack. |
| `owl_ampDecay` | ms | 50–500 | Amp envelope decay. |
| `owl_ampSustain` | 0–1 | 0.5–0.8 | Amp envelope sustain. |
| `owl_ampRelease` | ms | 100–2000 | Amp envelope release. |
| `owl_portamento` | 0–2 s | 0–0.3 | Portamento glide time. |
| `owl_legatoMode` | bool | on | Legato mode — new notes glide from previous pitch. |
| `owl_morphGlide` | 0–1 | 0.3–0.7 | Mixtur morph glide smoothing. |
| `owl_outputLevel` | 0–1 | 0.8 | Master output level. |
| `owl_outputPan` | -1–1 | 0 | Master pan. |
| `owl_couplingLevel` | 0–1 | varies | Coupling output send level. |
| `owl_couplingBus` | 0–3 | — | Coupling bus assignment. |

### Coupling
- **Sends:** Monophonic audio (ch0/1) — deep subharmonic organ voice as coupling source
- **Receives:** AmpToFilter (external amplitude modulates filter cutoff), LFOToPitch (external LFO pitch modulation), AudioToFM (external audio feeds into Mixtur as FM source)
- **Best as source for:** AudioToWavetable (subharmonic organ as source material for granular engines), AmpToFilter (owlfish envelope pumping other engines' filters)
- **Ecosystem role:** The abyssal organ — deepest tonal anchor in the column. Paired with surface engines for maximum depth contrast.

### Recommended Pairings
- **+ Obese:** Owlfish subharmonics beneath Fat's massive bass — Oscar polarity amplified to the extreme.
- **+ Osprey:** Abyssal depth + surface turbulence. Maximum water column span.
- **+ Opal:** Owlfish organ frozen into Opal grain clouds. Subharmonic textures granularized.
- **+ Overdub:** Deep organ gaining tape delay. Dub subharmonic echo.
- **+ Ouroboros:** Owlfish tones feeding the strange attractor injection. Deep order meeting deep chaos.

### Starter Recipes
**Trautonium Stack:** owl_mixtur=0.7, owl_subDiv1=2, owl_subDiv2=3, owl_subDiv3=4, owl_subDiv4=5 — classic Trautonium subharmonic register, full natural harmonic series below fundamental
**Armor Break:** owl_armorThreshold=-20, owl_armorScatter=0.7, owl_grainMix=0.5, owl_grainSize=40 — signal shatters under hard attack, granular scatter reveals then reconstitutes
**Abyssal Organ:** owl_filterCutoff=800, owl_filterReso=0.3, owl_reverbSize=0.8, owl_reverbDamp=0.6 — deep cathedral with abyssal damping, massive space
**Bioluminescent Pulse:** owl_bodyFreq=220, owl_bodyLevel=0.4, owl_depth=0.8 — pulsing body resonance in the darkness, subharmonic glow

---

## OCELOT (Ocelot)
*Forest Strata Synthesis — four ecosystem layers in cross-modulation*

**Accent:** Ocelot Tawny `#C5832B` | **Prefix:** `ocelot_` | **Voices:** polyphonic (voice pool)
**Creature:** The ocelot hunts across all forest layers — floor to canopy — moving between ecological strata with predatory precision. XOcelot models this as synthesis: four distinct synthesis layers (Floor, Understory, Canopy, Emergent) that cross-modulate each other through a 12-route EcosystemMatrix.
**Polarity:** Balanced feliX/Oscar — organic, territorial, layered

### What It Does
Four synthesis strata, each using a different synthesis approach, connected by a 12-route cross-modulation matrix. The Floor layer uses physical models (adjustable via `ocelot_floorModel`), the Understory provides rhythmic chopped texture, the Canopy delivers wavefold spectral shimmer, and the Emergent layer adds creature formant voices.

The Biome selector (`ocelot_biome`) morphs between forest biome character profiles — each biome shapes the timbral DNA of all four strata simultaneously. The EcosystemMatrix allows any stratum to modulate any other (12 cross-routes: Floor→Understory, Floor→Canopy, Floor→Emergent, Understory→Floor, etc.), creating complex internal ecosystems.

A lofi processing chain (bit depth reduction, sample rate reduction, tape wobble, tape age, dust) can give the entire engine a field-recording character — archival forest recordings.

### Macros
| Macro | Name | Mapping |
|-------|------|---------|
| M1 | **PROWL** | `ocelot_prowl` — Floor intensity + creature trigger rate |
| M2 | **FOLIAGE** | `ocelot_foliage` — Canopy shimmer + understory grain density |
| M3 | **ECOSYSTEM** | `ocelot_ecosystem` — EcosystemMatrix cross-modulation depth |
| M4 | **CANOPY** | `ocelot_canopy` — Canopy wavefold + spectral filter |

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `ocelot_biome` | 0–N | varies | Biome profile. Shapes all strata timbral DNA simultaneously. |
| `ocelot_strataBalance` | 0–1 | 0.4–0.6 | Balance between strata — shifts weight from Floor toward Canopy. |
| `ocelot_ecosystemDepth` | 0–1 | 0.3–0.6 | EcosystemMatrix cross-modulation depth. How much strata affect each other. |
| `ocelot_humidity` | 0–1 | 0.3–0.6 | Humidity — affects filter character across all strata. |
| `ocelot_swing` | 0–1 | 0.0–0.3 | Rhythmic swing amount (applied to Understory chop). |
| `ocelot_density` | 0–1 | 0.4–0.7 | Event density across all strata. |
| `ocelot_floorModel` | 0–N | 2–4 | Physical model type for Floor layer. |
| `ocelot_floorTension` | 0–1 | 0.4–0.7 | Floor model string/membrane tension. |
| `ocelot_floorStrike` | 0–1 | 0.5–0.8 | Floor model excitation intensity. |
| `ocelot_floorDamping` | 0–1 | 0.2–0.5 | Floor model damping. Higher=shorter resonance. |
| `ocelot_floorPattern` | 0–N | varies | Floor rhythmic pattern selection. |
| `ocelot_floorLevel` | 0–1 | 0.6–0.9 | Floor stratum output level. |
| `ocelot_floorPitch` | 0–1 | 0.5 | Floor stratum pitch offset (0.5=center). |
| `ocelot_floorVelocity` | 0–1 | 0.6–0.9 | Floor velocity sensitivity. |
| `ocelot_chopRate` | bpm | 4–32 | Understory chop rate. |
| `ocelot_chopSwing` | 0–1 | 0.0–0.3 | Understory chop swing. |
| `ocelot_bitDepth` | 4–24 | 12–16 | Lofi bit depth reduction (24=off). |
| `ocelot_sampleRate` | Hz | 8000–44100 | Lofi sample rate reduction (44100=off). |
| `ocelot_tapeWobble` | 0–1 | 0.0–0.1 | Tape pitch wobble amount. |
| `ocelot_tapeAge` | 0–1 | 0.0–0.1 | Tape degradation character. |
| `ocelot_dustLevel` | 0–1 | 0.0–0.1 | Dust/crackle noise level. |
| `ocelot_understoryLevel` | 0–1 | 0.4–0.7 | Understory stratum output level. |
| `ocelot_understorySrc` | 0–1 | varies | Understory source selection. |
| `ocelot_canopyWavefold` | 0–1 | 0.2–0.5 | Canopy wavefolder amount. |
| `ocelot_canopyPartials` | 1–8 | 3–5 | Number of canopy partials in spectral synthesis. |
| `ocelot_canopyDetune` | 0–1 | 0.1–0.3 | Canopy partial detuning for shimmer. |
| `ocelot_canopySpectralFilter` | 0–1 | 0.5–0.8 | Canopy spectral filter shape. |
| `ocelot_canopyBreathe` | 0–1 | 0.2–0.5 | Canopy breathing LFO depth. |
| `ocelot_canopyShimmer` | 0–1 | 0.1–0.4 | Canopy shimmer modulation. |
| `ocelot_canopyLevel` | 0–1 | 0.4–0.7 | Canopy stratum output level. |
| `ocelot_canopyPitch` | 0–1 | 0.5 | Canopy pitch offset (0.5=center). |
| `ocelot_creatureType` | 0–N | varies | Emergent creature formant type. |
| `ocelot_creatureRate` | Hz | 0.1–2.0 | Creature formant animation rate. |
| `ocelot_creaturePitch` | 0–1 | 0.5 | Creature pitch offset. |
| `ocelot_creatureSpread` | 0–1 | 0.2–0.5 | Creature formant stereo spread. |
| `ocelot_creatureTrigger` | 0–1 | 0–1 | Creature trigger mode (0=continuous, 1=note-triggered). |
| `ocelot_creatureLevel` | 0–1 | 0.2–0.5 | Emergent creature stratum output level. |
| `ocelot_creatureAttack` | 0–1 | 0.1–0.4 | Creature envelope attack. |
| `ocelot_creatureDecay` | 0–1 | 0.3–0.7 | Creature envelope decay. |
| `ocelot_xf_floorUnder` | 0–1 | 0.0–0.3 | Floor → Understory chop rate cross-modulation. |
| `ocelot_xf_floorCanopy` | 0–1 | 0.0–0.3 | Floor → Canopy filter cross-modulation. |
| `ocelot_xf_floorEmerg` | 0–1 | 0.0–0.2 | Floor → Emergent trigger threshold cross-modulation. |
| `ocelot_xf_underFloor` | 0–1 | 0.0–0.2 | Understory → Floor swing cross-modulation. |
| `ocelot_xf_underCanopy` | 0–1 | 0.0–0.3 | Understory → Canopy morph cross-modulation. |
| `ocelot_xf_underEmerg` | 0–1 | 0.0–0.2 | Understory → Emergent pitch cross-modulation. |
| `ocelot_xf_canopyFloor` | 0–1 | 0.0–0.2 | Canopy → Floor damping cross-modulation. |
| `ocelot_xf_canopyUnder` | 0–1 | 0.0–0.3 | Canopy → Understory grain position cross-modulation. |
| `ocelot_xf_canopyEmerg` | 0–1 | 0.0–0.2 | Canopy → Emergent formant cross-modulation. |
| `ocelot_xf_emergFloor` | 0–1 | 0.0–0.2 | Emergent → Floor accent cross-modulation. |
| `ocelot_xf_emergUnder` | 0–1 | 0.0–0.2 | Emergent → Understory scatter cross-modulation. |
| `ocelot_xf_emergCanopy` | 0–1 | 0.0–0.2 | Emergent → Canopy shimmer cross-modulation. |
| `ocelot_reverbSize` | 0–1 | 0.4–0.7 | Reverb room size. |
| `ocelot_reverbMix` | 0–1 | 0.2–0.4 | Reverb wet mix. |
| `ocelot_delayTime` | 0–1 | 0.3–0.5 | Delay time (normalized). |
| `ocelot_delayFeedback` | 0–1 | 0.2–0.4 | Delay feedback. |
| `ocelot_delayMix` | 0–1 | 0.1–0.2 | Delay wet mix. |
| `ocelot_filterEnvDepth` | 0–1 | 0.2–0.5 | Filter envelope depth (velocity-triggered). |
| `ocelot_ampAttack` | ms | 5–100 | Amp envelope attack. |
| `ocelot_ampDecay` | ms | 100–1000 | Amp envelope decay. |
| `ocelot_ampSustain` | 0–1 | 0.7–0.9 | Amp envelope sustain. |
| `ocelot_ampRelease` | ms | 200–2000 | Amp envelope release. |
| `ocelot_couplingLevel` | 0–1 | varies | Coupling output send level. |
| `ocelot_couplingBus` | 0–3 | — | Coupling bus assignment. |

### Coupling
- **Sends:** Stereo audio from all strata (post-mix), per-stratum amplitude envelopes
- **Receives:** AudioToWavetable (external audio as understory or canopy source), AmpToFilter (ecosystem depth modulation), LFOToPitch (creature pitch modulation)
- **Best as source for:** AmpToFilter (ecosystem dynamics driving other engines), AudioToWavetable (forest strata as source material)
- **Ecosystem role:** The forest floor and canopy — organic layered synthesis with rich internal cross-modulation. Pairs naturally with engines that can receive rhythmic complexity.

### Recommended Pairings
- **+ Onset:** Ocelot's floor physical models + Onset's drum voices. Percussion with ecological context.
- **+ Opal:** Canopy shimmer granularized by Opal. Forest shimmer frozen into grain clouds.
- **+ Ouroboros:** EcosystemMatrix feeding the strange attractor injection. Organic chaos meeting mathematical chaos.
- **+ Odyssey:** Open-water drift beneath the forest canopy. Depth and organic complexity.
- **+ Optic:** Ocelot's creature formants feeding Optic's visual engine. Bioluminescent forest display.

### Starter Recipes
**Forest Floor:** ocelot_floorModel=3, ocelot_floorTension=0.55, ocelot_floorStrike=0.6, ocelot_strataBalance=0.3, ocelot_ecosystemDepth=0.2 — physical model dominant, minimal cross-modulation, grounded
**Canopy Shimmer:** ocelot_strataBalance=0.8, ocelot_canopyWavefold=0.4, ocelot_canopyBreathe=0.5, ocelot_canopyShimmer=0.4 — canopy dominant, shimmer LFO active, airy texture
**Ecosystem Pulse:** ocelot_ecosystemDepth=0.7, ocelot_xf_floorUnder=0.4, ocelot_xf_underCanopy=0.3, ocelot_xf_canopyEmerg=0.3 — cross-modulation chain active, floor pulses through understory to canopy to creature
**Archival Field Recording:** ocelot_bitDepth=12, ocelot_tapeWobble=0.08, ocelot_tapeAge=0.1, ocelot_dustLevel=0.1, ocelot_reverbMix=0.3 — lofi field recording character, forest sounds as degraded archive

---

## 29. ORCA — Apex Predator Synthesis

**Gallery code:** ORCA | **Accent:** Deep Ocean `#1B2838`
**Parameter prefix:** `orca_`
**Aquatic mythology:** The orca (killer whale) — apex predator of every ocean. Coordinated, intelligent, devastating. XOrca maps five biological subsystems directly into DSP: vocal dialect (wavetable + formant), echolocation (resonant comb), apex hunt macro (coordinated aggression), breach sub-bass (sidechain displacement), and countershading (band-split bitcrusher).
**Synthesis type:** Wavetable oscillator with 5-band formant network, resonant comb filter echolocation, sub-bass breach layer, dynamic bitcrushing with high/low band split
**Polyphony:** Mono / Legato / Poly8 / Poly16 (default: Legato)

### Core Parameters

| Parameter | Range | Default | Sweet Spot |
|-----------|-------|---------|------------|
| `orca_wtPosition` | 0–1 | 0.0 | 0.0–0.4 (whale-call to metallic) |
| `orca_wtScanRate` | 0–1 | 0.5 | 0.2–0.6 (slow scan for vocal movement) |
| `orca_formantIntensity` | 0–1 | 0.5 | 0.4–0.8 |
| `orca_formantShift` | 0–1 | 0.5 | 0.3–0.7 (shifts all 5 formants together) |
| `orca_glide` | 0–5 s | 0.3 | 0.1–1.0 (heavy portamento for whale song) |
| `orca_echoRate` | 0.5–40 Hz | 5.0 | 2–15 (clicks to ringing tone) |
| `orca_echoReso` | 0–0.995 | 0.85 | 0.7–0.95 (high for metallic ring) |
| `orca_echoDamp` | 0–0.99 | 0.3 | 0.1–0.5 |
| `orca_echoMix` | 0–1 | 0.0 | 0.2–0.5 |
| `orca_huntMacro` | 0–1 | 0.0 | 0–1 (master aggression: filter, crush, formant, breach, echo reso) |
| `orca_breachSub` | 0–1 | 0.5 | 0.4–0.8 |
| `orca_breachShape` | 0–1 | 0.0 | 0=sine sub, >0.5=triangle sub |
| `orca_breachThreshold` | -60–0 dB | -18 | -24 to -12 |
| `orca_breachRatio` | 1–20 | 8.0 | 6–12 (hard sidechain compression) |
| `orca_crushBits` | 1–16 | 16.0 | 8–16 (16=clean, lower=decimated dorsal) |
| `orca_crushDownsample` | 1–64 | 1.0 | 1–8 |
| `orca_crushMix` | 0–1 | 0.0 | 0–0.4 (countershading high-end) |
| `orca_crushSplitFreq` | 100–4000 Hz | 800 | 400–1200 (low stays clean, high gets crushed) |
| `orca_filterCutoff` | 20–20000 Hz | 8000 | 2000–12000 |
| `orca_filterReso` | 0–1 | 0.0 | 0–0.4 |
| `orca_level` | 0–1 | 0.8 | 0.7–0.9 |
| `orca_ampAttack` | 0–10 s | 0.01 | 0.001–0.05 |
| `orca_ampDecay` | 0–10 s | 0.1 | 0.05–0.5 |
| `orca_ampSustain` | 0–1 | 0.8 | 0.6–0.9 |
| `orca_ampRelease` | 0–20 s | 0.3 | 0.1–2.0 |
| `orca_modAttack` | 0–10 s | 0.01 | 0.001–0.1 |
| `orca_modDecay` | 0–10 s | 0.3 | 0.1–1.0 |
| `orca_modSustain` | 0–1 | 0.5 | 0.3–0.7 |
| `orca_modRelease` | 0–20 s | 0.5 | 0.2–1.0 |
| `orca_lfo1Rate` | 0.01–30 Hz | 0.2 | 0.05–0.5 (slow wavetable scan) |
| `orca_lfo1Depth` | 0–1 | 0.5 | 0.3–0.7 |
| `orca_lfo1Shape` | Sine/Tri/Saw/Sq/S&H | Sine | Sine for organic movement |
| `orca_lfo2Rate` | 0.01–30 Hz | 8.0 | 4–20 (echolocation click modulation) |
| `orca_lfo2Depth` | 0–1 | 0.0 | 0–0.5 |
| `orca_lfo2Shape` | Sine/Tri/Saw/Sq/S&H | Sine | — |
| `orca_polyphony` | Mono/Legato/Poly8/Poly16 | Legato | Legato for portamento lines |
| `orca_macroCharacter` | 0–1 | 0.0 | 0–0.5 (adds to HUNT macro intensity) |
| `orca_macroMovement` | 0–1 | 0.0 | 0–0.6 (formant intensity + WT scan + echo rate) |
| `orca_macroCoupling` | 0–1 | 0.0 | 0–0.5 (echolocation mix send depth) |
| `orca_macroSpace` | 0–1 | 0.0 | 0–0.5 |

### Macro Mappings
- **M1 CHARACTER**: Adds to `orca_huntMacro` aggression — drives filter cutoff, formant intensity, crush mix, breach sub, and echo resonance as a coordinated unit
- **M2 MOVEMENT**: Formant intensity offset + wavetable scan position + echolocation click rate — shapes how the pod vocalizes and hunts
- **M3 COUPLING**: Echolocation mix level — how much the comb-filter echo layer bleeds into the output and into coupling sends
- **M4 SPACE**: Registered macro; use for reverb/delay send depth in coupled configurations

### Coupling Compatibility
ORCA accepts: `AudioToFM` (modulates wavetable position via FM), `AmpToFilter` (external amplitude → formant intensity), `AmpToChoke` (external amplitude triggers breach sidechain), `EnvToMorph` (envelope → echolocation click rate modulation), `AudioToRing` (ring modulation source on the pod dialect layer), `LFOToPitch` (pitch modulation from upstream LFOs)

### Starter Recipes
**Pod Dialect:** orca_wtPosition=0.1, orca_formantIntensity=0.6, orca_formantShift=0.5, orca_glide=0.4, orca_echoMix=0.0 — pure vocal synthesis, orca click-calls without percussion; long glide for whale-song movement
**Echolocation Hunt:** orca_echoRate=12.0, orca_echoReso=0.92, orca_echoMix=0.4, orca_huntMacro=0.5 — rapid-fire comb pinging merges into metallic tones; HUNT at 0.5 adds aggression to filter and crush
**Breach Event:** orca_breachSub=0.8, orca_breachShape=0.0, orca_breachThreshold=-12.0, orca_breachRatio=12.0, orca_huntMacro=0.8 — massive sine sub displaces the mix on each note trigger; HUNT fully engaged for maximum countershading decimation

### Designer Notes
ORCA is an apex predator — it should never sound polite. The HUNT macro is the engine's identity: push it and everything moves as a single coordinated organism. Filter opens, formants intensify, echo rings harder, high-end gets crushed, sub displaces the bottom. It is designed to be pushed, not left at zero.

The wavetable + formant system (Pod Dialect) covers enormous timbral territory: sine-like whale calls at position 0, complex metallic vocal textures as position increases. Heavy portamento (`orca_glide` 0.3–1.0 seconds) transforms chromatic lines into the sliding pitch bends of actual orca vocalizations. Echolocation sits underneath as a resonant textural layer — at low rates it clicks, at high rates it rings. Mod wheel (CC#1) scans wavetable position live, enabling performance-time timbral sweeps from organic to metallic.

---

## 30. OCTOPUS — Decentralized Alien Intelligence

**Gallery code:** OCTOPUS | **Accent:** Chromatophore Magenta `#E040FB`
**Parameter prefix:** `octo_`
**Aquatic mythology:** The octopus — alien intelligence distributed across eight arms, each with its own neural cluster. XOctopus maps this directly: eight independent LFOs (arms) run at prime-ratio-related rates, modulating different sonic dimensions simultaneously. The instrument is never still, never symmetric, never predictable. It camouflages (chromatophore morphing filter), inks (velocity-triggered noise freeze), squeezes through microtonal gaps (shapeshifter), and grabs with wet suckers (fast bandpass transients).
**Synthesis type:** Wavetable oscillator with 8-arm polyrhythmic LFO modulation, envelope-follower-driven morphing filter (LP→BP→HP→Notch), velocity-triggered noise freeze burst, microtonal pitch drift + extreme portamento, ultra-fast bandpass transient layer
**Polyphony:** Mono / Legato / Poly8 / Poly16 (default: Poly8)

### Core Parameters

| Parameter | Range | Default | Sweet Spot |
|-----------|-------|---------|------------|
| `octo_armCount` | 1–8 | 4 | 4–8 (more arms = more alien complexity) |
| `octo_armSpread` | 0–1 | 0.5 | 0.3–0.8 (how different the arm rates are from each other) |
| `octo_armBaseRate` | 0.05–20 Hz | 1.0 | 0.2–3.0 |
| `octo_armDepth` | 0–1 | 0.5 | 0.3–0.7 (modulation depth per arm) |
| `octo_chromaSens` | 0–1 | 0.5 | 0.4–0.7 (envelope follower sensitivity) |
| `octo_chromaSpeed` | 0–1 | 0.5 | 0.3–0.7 (filter topology adaptation rate) |
| `octo_chromaMorph` | 0–1 | 0.0 | 0.0–1.0 (static: LP=0, BP=0.33, HP=0.66, Notch=1.0) |
| `octo_chromaDepth` | 0–1 | 0.5 | 0.3–0.7 (how intensely the filter morphs) |
| `octo_chromaFreq` | 100–16000 Hz | 2000 | 500–6000 |
| `octo_inkThreshold` | 0–1 | 0.9 | 0.7–0.95 (velocity required to trigger ink cloud) |
| `octo_inkDensity` | 0–1 | 0.8 | 0.6–0.9 (noise burst density) |
| `octo_inkDecay` | 0.5–30 s | 5.0 | 2–10 (how long the ink cloud lingers) |
| `octo_inkMix` | 0–1 | 0.0 | 0–0.5 (normally triggered by high velocity, not static mix) |
| `octo_shiftMicro` | -100–100 cents | 0.0 | -20–20 (microtonal detuning from equal temperament) |
| `octo_shiftGlide` | 0–10 s | 0.5 | 0.1–3.0 (extreme portamento — "boneless" pitch) |
| `octo_shiftDrift` | 0–1 | 0.0 | 0.1–0.4 (random pitch drift between notes) |
| `octo_suckerReso` | 0–0.995 | 0.8 | 0.7–0.95 (high reso for pronounced plonk) |
| `octo_suckerFreq` | 200–8000 Hz | 2000 | 500–4000 (center frequency of the bandpass pluck) |
| `octo_suckerDecay` | 0.005–0.5 s | 0.05 | 0.01–0.15 |
| `octo_suckerMix` | 0–1 | 0.0 | 0.1–0.4 (subtle sticky transient underneath) |
| `octo_wtPosition` | 0–1 | 0.0 | 0–0.5 |
| `octo_wtScanRate` | 0–1 | 0.3 | 0.1–0.5 (arms also modulate WT position) |
| `octo_filterCutoff` | 20–20000 Hz | 8000 | 2000–10000 |
| `octo_filterReso` | 0–1 | 0.0 | 0–0.3 |
| `octo_level` | 0–1 | 0.8 | 0.7–0.9 |
| `octo_ampAttack` | 0–10 s | 0.01 | 0.001–0.05 |
| `octo_ampDecay` | 0–10 s | 0.3 | 0.05–0.8 |
| `octo_ampSustain` | 0–1 | 0.7 | 0.5–0.8 |
| `octo_ampRelease` | 0–20 s | 0.5 | 0.2–2.0 |
| `octo_modAttack` | 0–10 s | 0.01 | 0.001–0.1 |
| `octo_modDecay` | 0–10 s | 0.5 | 0.1–1.0 |
| `octo_modSustain` | 0–1 | 0.5 | 0.3–0.7 |
| `octo_modRelease` | 0–20 s | 0.5 | 0.2–1.0 |
| `octo_lfo1Rate` | 0.01–30 Hz | 0.5 | 0.1–2.0 |
| `octo_lfo1Depth` | 0–1 | 0.3 | 0.2–0.5 |
| `octo_lfo1Shape` | Sine/Tri/Saw/Sq/S&H | Sine | — |
| `octo_lfo2Rate` | 0.01–30 Hz | 2.0 | 1.0–8.0 |
| `octo_lfo2Depth` | 0–1 | 0.0 | 0–0.4 |
| `octo_lfo2Shape` | Sine/Tri/Saw/Sq/S&H | Sine | — |
| `octo_polyphony` | Mono/Legato/Poly8/Poly16 | Poly8 | Poly8 for multi-arm texture |
| `octo_macroCharacter` | 0–1 | 0.0 | 0–0.6 (arm depth + sucker intensity + filter brightness) |
| `octo_macroMovement` | 0–1 | 0.0 | 0–0.6 (arm rate + chroma speed + WT scan) |
| `octo_macroCoupling` | 0–1 | 0.0 | 0–0.5 (chromatophore depth + ink mix) |
| `octo_macroSpace` | 0–1 | 0.0 | 0–0.5 (ink decay time + pitch drift) |

### Macro Mappings
- **M1 CHARACTER**: Arm modulation depth + sucker mix + filter cutoff/resonance — controls how much the decentralized arms impose themselves on the sound
- **M2 MOVEMENT**: Arm base rate + chromatophore adaptation speed + wavetable scan rate + chroma morph target — how fast the organism adapts and shifts
- **M3 COUPLING**: Chromatophore filter morph depth + ink cloud mix — how intensely the skin changes color and how readily it inks
- **M4 SPACE**: Ink cloud decay time extension + pitch drift — how long the ink cloud lingers and how far the pitch wanders between notes

### Coupling Compatibility
OCTOPUS accepts: `AudioToFM` (modulates wavetable position via FM — external audio shapes the oscillator timbre), `AmpToFilter` (external amplitude → chromatophore sensitivity, making the filter morph respond to upstream dynamics), `EnvToMorph` (envelope → arm rate modulation — upstream envelopes speed up the arm polyrhythm), `AudioToRing` (ring modulation on arm output amplitude), `LFOToPitch` (additional pitch modulation layered onto shapeshifter drift)

### Starter Recipes
**Alien Texture:** octo_armCount=8, octo_armSpread=0.7, octo_armBaseRate=0.8, octo_armDepth=0.6, octo_chromaDepth=0.5, octo_chromaSpeed=0.4 — all 8 arms running at prime-ratio rates with deep chromatophore adaptation; never repeats, never settles
**Ink Attack:** octo_inkThreshold=0.75, octo_inkDensity=0.9, octo_inkDecay=8.0, octo_inkMix=0.0 — hit keys at high velocity to trigger noise freeze; play softly for dry synth, slam hard for the ink cloud eruption
**Boneless Lead:** octo_shiftGlide=2.0, octo_shiftDrift=0.2, octo_shiftMicro=15.0, octo_suckerReso=0.85, octo_suckerMix=0.3 — extreme portamento with microtonal offset and sucker transients; slides through frequencies like a boneless body through a crack

### Designer Notes
OCTOPUS is the most generative engine in the fleet. The 8-arm LFO system operates independently — each arm runs at a prime-ratio multiple of the base rate (so arm ratios are 1×, 1.3×, 1.7×, 2.1×, 2.3×, 2.9×, 3.1×, 3.7× or similar), meaning the arms never exactly re-synchronize. The result is modulation that is always moving, always changing, but always internally coherent because it shares a common tempo reference. `octo_armSpread` is the key variable: at 0 all arms run at the same rate (synchronized), at 1 they diverge maximally.

The ink cloud is a performance weapon, not a mixing tool. Set `octo_inkThreshold` to 0.75–0.9 and leave `octo_inkMix` at 0. Play phrases normally; the ink only erupts when you play an accent note at maximum velocity. This creates dramatic contrast: melodic content suddenly obliterated by a wall of saturated noise that slowly dissolves. Pair with OPAL (granular) to freeze the ink cloud into indefinite sustain, or with OVERDUB (tape delay) to smear the noise into a murky wash.
