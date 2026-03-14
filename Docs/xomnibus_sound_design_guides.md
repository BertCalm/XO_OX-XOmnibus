# XOmnibus — Sound Design Guide
*Per-engine reference for sound designers, preset builders, and performers.*
*Covers all 21 engines: features, key parameters, coupling strategies, and recommended pairings.*

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
*Percussive transient synthesis — the neon tetra*

**Accent:** Neon Tetra Blue `#00A6D6` | **Prefix:** `snap_` | **Voices:** 8

### What It Does
Punchy, clicky, transient-rich sounds. Every note starts with a pitch sweep from +24 semitones down to the target — the "snap" effect. Three oscillator modes (Sine+Noise, FM, Karplus-Strong) cover everything from kicks and toms to plucks and metallic hits.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `snap_snap` | 0–1 | 0.3–0.6 | Controls the snap/transient intensity. Higher = more click. |
| `snap_decay` | 0.001–5s | 0.1–0.4 | Envelope decay time. Short for clicks, longer for toms. |
| `snap_filterCutoff` | 20–20kHz | 1–4 kHz | HPF→BPF cascade cutoff. Lower = warmer, higher = brighter. |
| `snap_filterReso` | 0–1 | 0.2–0.5 | Resonance. Higher adds ring to percussion. |
| `snap_detune` | 0–100 cents | 5–15 | Unison detune spread. Wider = bigger. |
| `snap_unison` | 1/2/4 | 2 | Unison voice count. 4 = huge stereo spread. |
| `snap_oscMode` | 0/1/2 | — | 0=Sine+Noise, 1=FM, 2=Karplus-Strong |

### Coupling
- **Sends:** Envelope level (ch2) — great for driving other engines' filters
- **Receives:** AmpToPitch, LFOToPitch — subtle pitch modulation adds life
- **Best as source for:** AmpToFilter (sidechain pump), AmpToChoke (ducking)

### Recommended Pairings
- **+ Overdub:** Snap provides percussion, Dub provides body. Classic dub techno.
- **+ Odyssey:** Snap triggers, Drift provides evolving pad underneath. Ambient percussion.
- **+ Oblique:** Snap's transient + Oblique's prism delay = ricocheting percussion.

### Starter Recipes
**Tight Kick:** snap=0.8, decay=0.15, oscMode=0, filterCutoff=400, pitchLock=on
**Metallic Pluck:** snap=0.5, decay=0.3, oscMode=2 (KS), filterCutoff=3000, reso=0.4
**Tabla Hit:** snap=0.6, decay=0.25, oscMode=1 (FM), filterCutoff=2000, detune=20

---

## 2. ODDOSCAR (OddOscar)
*Lush pad synthesis with wavetable morph — the axolotl*

**Accent:** Axolotl Gill Pink `#E8839B` | **Prefix:** `morph_` | **Voices:** 16

### What It Does
Rich, evolving pads with continuous morphing between Sine→Saw→Square→Noise. Three detuned oscillators + sub oscillator provide massive width. Moog-style 4-pole ladder filter with Perlin noise drift gives analog warmth that breathes.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `morph_scanPos` | 0–1 | 0.2–0.6 | Wavetable morph position. 0=Sine, 0.33=Saw, 0.66=Square, 1=Noise |
| `morph_filterCutoff` | 20–20kHz | 2–6 kHz | Moog ladder filter cutoff |
| `morph_filterReso` | 0–1 | 0.3–0.6 | Self-oscillates above 0.9 |
| `morph_drift` | 0–1 | 0.1–0.3 | Perlin noise pitch drift. Analog warmth. |
| `morph_subLevel` | 0–1 | 0.3–0.5 | Sub oscillator level (one octave below) |

### Coupling
- **Sends:** Slow 0.3Hz LFO sine (ch2) — organic modulation source
- **Receives:** AmpToFilter (pump), EnvToMorph (external envelope drives morph position)
- **Best as source for:** LFOToPitch (gentle pitch drift on other engines)

### Recommended Pairings
- **+ OddfeliX:** Morph pad + Snap percussion = complete dub palette
- **+ Oblong:** Two pad engines layered. Morph provides warmth, Bob provides curiosity.
- **+ Optic:** Morph feeds Optic for spectral analysis, Optic modulates Morph's filter.

### Starter Recipes
**Warm Analog Pad:** scanPos=0.3, filterCutoff=3000, reso=0.3, drift=0.2, subLevel=0.4
**Evolving Texture:** scanPos automated M2, filterCutoff=1500, reso=0.5, drift=0.4
**Noise Wash:** scanPos=0.9, filterCutoff=800, reso=0.6, drift=0.1

---

## 3. OVERDUB (Overdub)
*Dub synth with tape delay and spring reverb*

**Accent:** Olive `#6B7B3A` | **Prefix:** `dub_` | **Voices:** 8

### What It Does
The workhorse. A complete dub production engine: oscillator → filter → tape delay → spring reverb → drive. The tape delay has wear (degradation), wow (pitch flutter), and long feedback tails. Every note can trail off into infinite dub space.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `dub_oscWave` | Sine/Tri/Saw/Sq | Sine or Saw | Oscillator waveform |
| `dub_filterCutoff` | 20–20kHz | 400–2000 | 3-mode filter (LPF/HPF/BPF) |
| `dub_delayTime` | 0.05–2s | 0.3–0.5 | Tape delay time. Try 3/8 note. |
| `dub_delayFeedback` | 0–1.2 | 0.4–0.7 | >1.0 = runaway feedback (use carefully) |
| `dub_delayWear` | 0–1 | 0.2–0.4 | Tape degradation. Higher = more vintage. |
| `dub_delayWow` | 0–1 | 0.1–0.2 | Pitch flutter. Subtle = warmth, heavy = seasick. |
| `dub_driveAmount` | 1–10 | 1.5–3 | Saturation drive. Subtle warmth to full distortion. |
| `dub_reverbMix` | 0–1 | 0.1–0.3 | Spring reverb. A little goes a long way. |

### Coupling
- **Sends:** Pitch envelope (ch2). Short, punchy envelope for filter drives.
- **Receives:** AmpToFilter (pump/sidechain), AmpToPitch, LFOToPitch
- **Best as source for:** AudioToFM (tape-degraded audio as FM source = unique)

### Recommended Pairings
- **+ OddfeliX:** Snap percussion through Dub's delay = instant dub techno
- **+ Oblique:** Dub's sub-bass foundation + Oblique's prismatic tops
- **+ Optic:** Optic's AutoPulse drives Dub's filter for rhythmic pumping

### Starter Recipes
**Classic Dub Bass:** oscWave=Sine, filterCutoff=600, subLevel=0.7, delayMix=0.1, voiceMode=Mono
**Tape Echo Pad:** oscWave=Saw, delayTime=0.375, feedback=0.6, wear=0.4, wow=0.15, reverbMix=0.25
**Siren Lead:** oscWave=Saw, glide=0.5, filterCutoff=2000, reso=0.6, voiceMode=Legato

---

## 4. ODYSSEY (Odyssey)
*Evolving pad synthesis with smooth random drift*

**Accent:** Violet `#7B2D8B` | **Prefix:** `odyssey_` | **Voices:** 8

### What It Does
Alive, breathing pads that never sit still. The Voyager-style drift engine adds smooth random walk to pitch and filter, making every note feel slightly different. Dual oscillators with detuning create wide, cinematic textures.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `odyssey_detune` | 0–50 cents | 8–15 | Oscillator detuning. Width control. |
| `odyssey_drift` | 0–1 | 0.15–0.35 | Random walk amount. The "alive" factor. |
| `odyssey_filterCutoff` | 20–20kHz | 1.5–5 kHz | LP filter cutoff |
| `odyssey_attack` | 0.001–2s | 0.05–0.3 | Slow attacks for cinematic swells |
| `odyssey_release` | 0.001–5s | 0.5–2 | Long release for pad tails |

### Coupling
- **Sends:** Filter envelope (ch2) — good for modulating other engines
- **Receives:** AmpToFilter, AmpToPitch, LFOToPitch
- **Best as source for:** AudioToFM (feeding its drifting output as FM source)

### Recommended Pairings
- **+ OddfeliX:** Drift pad + Snap transients = textured ambient percussion
- **+ Optic:** Feed Odyssey into Optic for spectral analysis → modulation feedback
- **+ Opal:** Two evolving engines stacked = deep ambient clouds

### Starter Recipes
**Cinematic Swell:** attack=0.3, decay=1.0, sustain=0.8, release=2.0, drift=0.25, detune=12
**Dark Pad:** filterCutoff=800, reso=0.4, drift=0.3, detune=20
**Bright Keys:** attack=0.005, decay=0.3, sustain=0.5, filterCutoff=6000, drift=0.1

---

## 5. OBLONG (Oblong)
*Curious harmonic synthesis — the Bob engine*

**Accent:** Amber `#E9A84A` | **Prefix:** `bob_` | **Voices:** 8

### What It Does
Quirky, harmonically rich sounds built on a "curiosity" parameter that introduces unexpected overtone behavior. Sine lookup table + PolyBLEP oscillators with per-oscillator analog drift. The amber character — warm but unpredictable.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `bob_fltCutoff` | 20–20kHz | 1–4 kHz | Main filter cutoff |
| `bob_fltReso` | 0–1 | 0.3–0.6 | Resonance — self-oscillates at high values |
| `bob_curiosity` | 0–1 | 0.2–0.5 | Harmonic unpredictability. Higher = weirder. |
| `bob_drift` | 0–1 | 0.1–0.2 | Per-oscillator analog instability |

### Coupling
- **Sends:** Stereo audio (ch0/1)
- **Receives:** AmpToFilter, AmpToPitch, LFOToPitch
- **Best as source for:** FilterToFilter (chain its filter output into another engine's filter)

### Recommended Pairings
- **+ Overdub:** Bob's harmonic character + Dub's tape delay = vintage keys through tape
- **+ Obese:** Bob provides character, Fat provides weight. Layered bass.
- **+ Oblique:** Bob's curious harmonics through Oblique's prism = kaleidoscopic weirdness

### Starter Recipes
**Funky Stab:** attack=0.002, decay=0.15, sustain=0.4, filterCutoff=3000, curiosity=0.3
**Pluck Bass:** attack=0.001, decay=0.3, sustain=0.0, filterCutoff=1500, reso=0.5
**Warm Lead:** filterCutoff=4000, curiosity=0.15, drift=0.15, glide=0.2

---

## 6. OBESE (Obese)
*Multi-oscillator saturation powerhouse*

**Accent:** Hot Pink `#FF1493` | **Prefix:** `fat_` | **Voices:** 6

### What It Does
Massive. 13 oscillators per voice (1 sub + 4 groups of 3 octave variants). ZDF ladder filter, Mojo saturation system, bitcrusher, arpeggiator. When you need the biggest sound in the room.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `fat_satDrive` | 0–1 | 0.3–0.7 | Saturation amount. Subtle warmth to aggressive crunch. |
| `fat_filterCutoff` | 20–20kHz | 800–4000 | ZDF ladder filter |
| `fat_filterReso` | 0–1 | 0.3–0.6 | Resonance with smooth self-oscillation |
| `fat_unisonSpread` | 0–1 | 0.3–0.6 | Detune spread across 13 oscillators |
| `fat_mojo` | 0–1 | 0.2–0.4 | The Mojo system — adds analog character |

### Coupling
- **Sends:** Stereo audio (ch0/1)
- **Receives:** AmpToFilter, AmpToPitch, LFOToPitch
- **Best as source for:** AudioToRing (its rich harmonic content ring-mods beautifully)

### Recommended Pairings
- **+ OddfeliX:** Fat body + Snap transient = punchy bass with click
- **+ Oblique:** Fat bass foundation + Oblique's bouncing percussive tops
- **+ Optic:** Optic's AutoPulse strobes Fat's filter for club-ready pumping

### Starter Recipes
**Unison Bass:** unisonSpread=0.4, filterCutoff=1200, satDrive=0.5, voiceMode=Mono
**Wide Pad:** unisonSpread=0.6, filterCutoff=4000, satDrive=0.2, release=1.5
**Distortion Lead:** satDrive=0.8, filterCutoff=3000, reso=0.5, voiceMode=Legato

---

## 7. OVERBITE (Overbite)
*Complex multi-oscillator with formant shaping*

**Accent:** Fang White `#F0EDE8` | **Prefix:** `poss_` | **Voices:** 16

### What It Does
The most parameter-rich engine (122 params). Dual oscillators with formant resonance network that shapes sound through vowel-like peaks. Deep modulation matrix. Can produce everything from vocal pads to aggressive bass.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `poss_biteDepth` | 0–1 | 0.3–0.7 | Formant intensity. Higher = more vocal character. |
| `poss_formantMode` | vowels | A/E/I | Which vowel shape the formant filter targets |
| `poss_oscAWave` | multiple | — | Primary oscillator waveform |
| `poss_modDepth` | 0–1 | 0.2–0.5 | Modulation matrix depth |

### Coupling
- **Sends:** Stereo audio (ch0/1)
- **Receives:** AmpToFilter, AudioToFM
- **Best as source for:** AudioToFM (formant-shaped audio as FM source = vocal FM)

### Recommended Pairings
- **+ Overdub:** Vocal formants + dub delay = ethereal vocal echoes
- **+ Organon:** Formant shapes feeding modal resonance = singing metal
- **+ Oceanic:** Formant textures + boid swarm = vocal cloud

### Starter Recipes
**Vocal Pad:** biteDepth=0.5, formant=A, attack=0.1, release=1.5
**Aggressive Bass:** biteDepth=0.7, oscAWave=Saw, filterCutoff=1000
**Choir Texture:** biteDepth=0.4, formant=O, detune=15, unisonCount=4

---

## 8. ONSET (Onset)
*Percussion and drum synthesis*

**Accent:** Electric Blue `#0066FF` | **Prefix:** `onset_` | **Voices:** 8

### What It Does
Purpose-built drum synthesizer with multiple envelope shapes (AD, AHD, ADSR) and noise-based excitation. Designed for electronic percussion from crisp hi-hats to booming kicks.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `onset_noiseLevel` | 0–1 | 0.3–0.6 | Noise component. Higher = more hi-hat/snare character. |
| `onset_envShape` | AD/AHD/ADSR | AD | Envelope type. AD for percussion, ADSR for tonal. |
| `onset_pitch` | Hz | 60–200 | Base pitch. Lower = kick, higher = tom/conga. |
| `onset_decay` | ms | 50–300 | Envelope decay. Tight for clicks, longer for toms. |

### Coupling
- **Sends:** Stereo audio (ch0/1)
- **Receives:** Limited (percussion-focused)
- **Best as source for:** AmpToChoke (percussion ducking other engines)

### Recommended Pairings
- **+ Overdub:** Onset drums through Dub's tape delay = instant dub rhythm
- **+ OddfeliX:** Two percussive engines layered = complex drum kits
- **+ Optic:** Feed Onset into Optic for rhythm-reactive visualizations

### Starter Recipes
**Analog Kick:** pitch=60, noiseLevel=0.1, decay=150, envShape=AD
**Snare:** pitch=200, noiseLevel=0.7, decay=100, envShape=AHD
**Hi-Hat:** pitch=8000, noiseLevel=0.9, decay=30, envShape=AD

---

## 9. OVERWORLD (Overworld)
*Chip synthesis — NES/Genesis/SNES/GameBoy*

**Accent:** Neon Green `#39FF14` | **Prefix:** `era_` | **Voices:** 8

### What It Does
A time machine for video game sound. 6 chip engines (NES, FM, SNES, GameBoy, PCE, Neo Geo) with ERA triangle-pad blending between 3 engines simultaneously. Bit crusher, glitch engine, and FIR echo add authentic retro processing.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `era_engineA/B/C` | chip select | NES/FM/SNES | Which chip engines sit at the triangle vertices |
| `era_blendX/Y` | 0–1 | varies | Barycentric position on the ERA triangle pad |
| `era_bitDepth` | 1–16 bits | 4–8 | Bit crusher depth. Lower = crunchier. |
| `era_glitchRate` | 0–1 | 0.1–0.3 | Glitch engine intensity |

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
*Swarm particle synthesis — boid flocking*

**Accent:** Phosphorescent Teal `#00B4A0` | **Prefix:** `oceanic_` | **Voices:** 4

### What It Does
128 particles per voice, each an oscillator with independent frequency/amplitude/pan. Craig Reynolds' boid rules (separation, alignment, cohesion) govern their behavior. 4 sub-flocks at different frequency ratios (1×, 2×, 1.5×, 3×) create harmonic structure from emergent movement. Murmuration triggers cascade the flock into new formations.

### Key Parameters
| Parameter | Range | Sweet Spot | What It Does |
|-----------|-------|------------|-------------|
| `oceanic_separation` | 0–1 | 0.3–0.5 | How much particles repel. Higher = wider frequency spread. |
| `oceanic_cohesion` | 0–1 | 0.3–0.5 | How much particles attract. Higher = more unison-like. |
| `oceanic_alignment` | 0–1 | 0.4–0.6 | How much particles match neighbors. Affects timbre smoothness. |
| `oceanic_murmuration` | trigger | — | Cascade flock reorganization. Like a starling murmuration. |
| `oceanic_particleWave` | Sine/Saw/Pulse/Noise | Sine | Individual particle waveform |

### Coupling
- **Sends:** Stereo swarm audio (ch0/1)
- **Receives:** AudioToFM (velocity perturbation), AmpToFilter (cohesion mod), RhythmToBlend (murmuration trigger)
- **Best as source for:** AudioToWavetable (dense swarm audio as wavetable source)

### Recommended Pairings
- **+ Opal:** Swarm audio → granular processing = particle cloud
- **+ Origami:** Swarm output → spectral fold = alien textures
- **+ Optic:** Swarm feeding Optic's analyzer = flocking visualized

### Starter Recipes
**Warm Swarm Pad:** separation=0.3, cohesion=0.5, alignment=0.5, wave=Sine
**Chaotic Texture:** separation=0.8, cohesion=0.1, alignment=0.1, wave=Saw
**Breathing Cloud:** separation=0.4, cohesion=0.4, alignment=0.6, murmuration every 4 bars

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
- OBSCURA (mass-spring), OCEANIC (boid swarm), ORGANON (modal resonance)

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
Drift pad, granular shimmer, swarm ambience, bowed strings. Cinematic texture.

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
