# XOverworld — Concept Brief

## Identity
- **Name:** XOverworld
- **Thesis:** "XOverworld is a chip synthesis engine that recreates the sound architectures of the NES, Genesis, and SNES — not by emulating the output, but by rebuilding how the chips actually worked."
- **Sound family:** Hybrid melodic + percussion (any voice can be either — just like the original hardware)
- **Unique capability:** Three-era chip synthesis with authentic architectural constraints. Morph between NES pulse, Genesis FM, and SNES sample playback on a single voice. The constraints ARE the character.

## Character
- **Personality in 3 words:** Nostalgic, precise, playful
- **Engine approach:** Multi-mode chip synthesis (pulse/triangle/noise + 4-op FM + BRR sample playback)
- **Why this engine:** Each console generation used a fundamentally different synthesis method. Recreating the *architecture* (not just the sound) means the constraints that made those soundtracks iconic become creative tools — 4 duty cycles, 8 FM algorithms, Gaussian interpolation, channel-count limits.

## Sound Chip Architecture

### Layer 1: 8-BIT (NES / Ricoh 2A03)
- **Pulse oscillator:** 4 selectable duty cycles (12.5%, 25%, 50%, 75%) — each has a distinct character
- **Triangle oscillator:** Fixed-volume, 4-bit stepped waveform — the NES bass sound
- **Noise channel:** Linear-feedback shift register, short-loop (metallic) or long-loop (hiss) modes
- **DPCM:** 1-bit delta-encoded sample playback (the crunch)
- **Character:** Raw, buzzy, limited, iconic. The sound of constraint.
- **Constraints that matter:** No volume envelope on triangle. Noise pitch is non-linear (specific frequency table). DPCM has only 16 rates.

### Layer 2: FM (Sega Genesis / Yamaha YM2612)
- **4-operator FM:** Sine carriers + modulators with individual envelopes, key scaling, detune
- **8 algorithms:** Different operator routing topologies (serial, parallel, mixed)
- **PSG companion:** 3 additional square waves + noise (from SN76489)
- **DAC mode:** One FM channel can play 8-bit PCM samples (the Genesis "voice" trick)
- **Character:** Metallic, brassy, punchy, aggressive. Electric piano, slap bass, the Sonic snare.
- **Constraints that matter:** Only sine oscillators — all timbre comes from modulation. 6 FM channels total. Discrete key-on/off events.

### Layer 3: SAMPLE (SNES / Sony SPC700 + S-DSP)
- **BRR sample playback:** 4-bit ADPCM compression with characteristic warm/muffled quality
- **Gaussian interpolation:** The SNES's signature slightly-smeared, soft sample playback
- **Hardware ADSR:** Attack/Decay/Sustain/Release with specific rate tables
- **8-tap FIR echo:** The iconic SNES reverb — a simple delay with FIR filtering in the feedback loop
- **Pitch modulation:** One channel's output modulates the next channel's pitch
- **Noise mode:** Per-channel noise replacement
- **Character:** Warm, cinematic, orchestral but compressed. Chrono Trigger, Final Fantasy VI, DKC.
- **Constraints that matter:** BRR compression artifacts. Gaussian interpolation rolls off highs. Echo uses shared RAM (historically limited sample memory). Only 8 voices total.

### The Era Blend Axis
- Continuous morph between any two layers on a single voice
- At the extremes: pure NES, pure Genesis, pure SNES
- In between: hybrid timbres impossible on any single console
- Example: NES pulse duty cycling with Genesis FM modulation depth and SNES Gaussian smoothing

## Drum Synthesis

Unlike ONSET (dedicated drum voices), XOverworld makes drums the way the original hardware did — **the same voices that play melody also play drums**:

### Kick
- **8-BIT:** Triangle with fast pitch sweep (NES classic technique)
- **FM:** Algorithm 7 with high-ratio modulator, fast pitch envelope
- **SAMPLE:** BRR-encoded kick sample with Gaussian smoothing

### Snare
- **8-BIT:** Short noise burst (long-loop mode) + triangle transient
- **FM:** Dual-operator with noise modulator, bright metallic snap
- **SAMPLE:** BRR snare with pitch envelope

### Hi-Hat
- **8-BIT:** Noise channel, short-loop (metallic) for closed, long-loop for open
- **FM:** 6-operator metallic patch (non-harmonic ratios), amplitude envelope shapes open/closed
- **SAMPLE:** BRR cymbal with amplitude envelope

### Toms / Percussion
- **8-BIT:** Triangle with medium pitch sweep + noise layer
- **FM:** Low-ratio modulation with pitch envelope
- **SAMPLE:** BRR tom/perc samples

### Per-Voice Mode
Each voice has a toggle: **MELODIC** or **PERC**
- Melodic: pitch tracks keyboard, sustain available
- Perc: fixed pitch (or per-note pitch table), one-shot envelope, no sustain

## Macro Mapping (M1-M4)

| Macro | Label | What It Controls | Musical Intent |
|-------|-------|-----------------|----------------|
| M1 | ERA | Blend position across 8-BIT ↔ FM ↔ SAMPLE layers | The defining axis — morph between console generations |
| M2 | CIRCUIT | Chip-specific parameters: duty cycle (8-BIT), FM algorithm (FM), interpolation mode (SAMPLE) | Timbral character within the current era |
| M3 | COUPLING | Engine-internal mod or coupling amount when coupled | Cross-engine interaction |
| M4 | SPACE | SNES-style FIR echo depth + external FX | Space and depth |

## Gallery Role
- **Sonic gap it fills:** Chip/retro synthesis — completely unoccupied. No engine recreates console-era sound architecture.
- **Differentiation from FAT's bitcrusher:** FAT applies bit reduction as a post-effect. XOverworld builds from chip architecture up — the synthesis method itself produces the character, not a filter applied after.
- **Differentiation from ONSET drums:** ONSET has dedicated voice roles (kick, snare, hat). XOverworld's voices are fluid — any can be melodic or percussive, reflecting how the original hardware worked.

## Coupling Partners

### Best Pairing: XOverworld + DUB
- **Route:** XOverworld output → DUB send chain (tape delay + spring reverb)
- **Musical effect:** 8-bit melodies through analog dub FX — lo-fi meets lo-fi in completely different dimensions
- **Why it's special:** NES sounds are dry and direct. DUB's tape delay adds warmth and space that the original hardware couldn't produce. The contrast is striking.

### Strong Pairing: XOverworld + ONSET
- **Route:** ONSET drum triggers → XOverworld pitch/trigger
- **Musical effect:** Real synthesized drums driving chip melody triggers — modern rhythm section with retro melodic response
- **Coupling type:** RhythmToBlend (ONSET pattern drives XOverworld era blend — drums morph the console generation)

### Strong Pairing: XOverworld + DRIFT
- **Route:** DRIFT Climax system → XOverworld era blend + FM algorithm
- **Musical effect:** As the JOURNEY macro builds, chip sounds evolve from simple NES pulses to complex Genesis FM to cinematic SNES samples — a journey through gaming history
- **Coupling type:** EnvToMorph (DRIFT envelope → XOverworld era position)

### Interesting Pairing: XOverworld + FAT
- **Route:** XOverworld pulse waves → FAT's 13-oscillator stack
- **Musical effect:** Chip melodies exploded into massive stereo width — "what if the NES had 65 oscillators?"
- **Coupling type:** AudioToFM (chip audio as FM source into FAT's oscillator bank)

### Interesting Pairing: XOverworld + BOB
- **Route:** BOB's warm character stages → XOverworld
- **Musical effect:** The soft, fuzzy warmth of BOB applied to crisp chip sounds — nostalgic warmth amplified

## Visual Identity (First Instinct)
- **Accent color:** Electric Green `#39FF14` — CRT phosphor green, the color of retro gaming
- **Material/texture:** CRT scanlines — subtle horizontal line pattern overlaid on the panel, pixel grid at edges
- **Icon concept:** A pixel-art controller D-pad, or a simple 8-bit waveform (stepped square wave)
- **Panel character:** Dark background within the panel (like a CRT screen), with bright green UI elements. Pixel font for value readouts. Subtle CRT curvature on panel corners.

## Mood Affinity

| Mood | Affinity | Why |
|------|----------|-----|
| Foundation | Strong | Chip drums, bass lines — the rhythmic foundation of game music |
| Atmosphere | Moderate | SNES-era ambient pads (DKC water levels, Metroid) |
| Entangled | Strong | Coupled with ONSET or DUB, chip sounds become something new |
| Prism | Very strong | Chip melodies ARE prismatic — bright, clear, articulate, melodic |
| Flux | Moderate | Glitchy chip artifacts, noise mode, DPCM crunch |
| Aether | Low-Moderate | SNES reverb can go cinematic, but this isn't the engine's primary territory |

## Unique Preset Concepts

| Preset Name | Era | Type | Description |
|-------------|-----|------|-------------|
| "Hyrule Field" | 8-BIT | Melodic | NES pulse arpeggios, 25% duty, triangle bass layer |
| "Green Hill" | FM | Melodic | Genesis slap bass, algorithm 5, bright and punchy |
| "Aquatic Ambiance" | SAMPLE | Pad | SNES BRR with FIR echo, Gaussian warmth, underwater |
| "Final Boss" | FM+8-BIT | Melodic | Aggressive FM lead over pulse wave arpeggios |
| "Select Screen" | 8-BIT | Perc+Melodic | NES noise hi-hats + triangle kick + pulse melody |
| "Emerald Coast" | FM→SAMPLE | Melodic | Era morph from Genesis punch to SNES warmth |
| "World Map" | All three | Melodic | Full era sweep via M1 — NES→Genesis→SNES journey |
| "8-Bit Kit" | 8-BIT | Drum | Full NES drum kit: triangle kick, noise hat, noise snare |
| "Genesis Slap" | FM | Melodic | That iconic Genesis bass — algorithm 7, low ratio |
| "DKC Pad" | SAMPLE | Pad | BRR-compressed orchestral pad with FIR echo tail |

## Parameter Namespace
- **Prefix:** `ow_` (overworld)
- **Examples:** `ow_era`, `ow_dutyCycle`, `ow_fmAlgorithm`, `ow_brrInterpolation`, `ow_noiseMode`

## Voice Architecture
- **Max voices:** 8 (matching the SNES's channel count — the highest of the three consoles)
- **Voice stealing:** Oldest note (matching original hardware behavior)
- **Legato:** Yes (for FM lead lines)
- **Per-voice mode:** Melodic or Percussive toggle
