# XOpal — Phase 1 Architecture Specification

**Engine:** XOpal (Granular Time Synthesis)
**Short Name:** OPAL
**Engine ID:** `"Opal"`
**Parameter Prefix:** `opal_`
**Accent Color:** Iridescent Lavender `#A78BFA`
**Max Voices:** 12 grain clouds (polyphonic — each note = one cloud)
**CPU Budget:** <12% single-engine
**Date:** 2026-03-11 (revised 2026-03-14 — V008 AudioToBuffer integration)

---

## 1. Product Identity

**Thesis:** "XOpal is a granular synthesis engine that fragments any sound into time-scattered particles — from smooth stretched clouds to shattered glass."

**Sound family:** Texture / Pad / Ambient / FX hybrid

**Unique capability:** Real-time granulation of the XOmnibus coupling bus — any other engine's audio can enter XOpal's grain buffer and be scattered through time. NES pulses fragmented. Climax blooms frozen. Bass clouds sustained. This coupling doesn't exist anywhere else. OPAL is the first engine to use coupling for *audio-level transformation* rather than parameter modulation — an entirely new tier of engine interaction.

**V008 Vision (Time Telescope):** OPAL is a universal transformer. Any engine's audio stream enters its grain buffer continuously and is reconstituted as something new. FREEZE (momentary hold, not toggle) allows the performer to capture a specific moment from the coupled stream and hold it — becoming a curator of temporal moments. The coupling mechanism that enables this is `AudioToBuffer`: a new coupling type distinct from `AudioToWavetable`, designed for continuous stereo audio streaming between engines.

**Personality in 3 words:** Iridescent, Fragmented, Suspended

**Gallery gap filled:** Every existing XOmnibus engine synthesizes *harmonically* — oscillators, wavetables, FM operators. None synthesizes in the *time domain*. OPAL introduces a fundamentally new synthesis dimension: granular time manipulation. Freeze, stretch, scatter, and cloud density become musical parameters rather than effects.

---

## 2. Signal Flow Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│                      XOpal Voice (×12)                           │
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐   │
│  │  GRAIN SOURCE                                             │   │
│  │                                                           │   │
│  │  Built-in OSC ──┬──► Source Mix ──► Ring Buffer            │   │
│  │  (sine/saw/     │    (mono, 4 sec,  (continuous write)    │   │
│  │   pulse/noise/  │     ~176KB @44.1k)                      │   │
│  │   two-osc)      │                                         │   │
│  │                  │                                         │   │
│  │  Coupling In ───┘    opal_couplingLevel scales external    │   │
│  │  (any engine)        audio into the buffer                │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│  ┌───────────────────▼───────────────────────────────────────┐   │
│  │  GRAIN SCHEDULER                                          │   │
│  │                                                           │   │
│  │  Density (1-120/s) ──► Trigger timer                      │   │
│  │  Position (0-1) + posScatter ──► Read offset              │   │
│  │  Size (10-800ms) ──► Window length                        │   │
│  │  Window (Hann/Gaussian/Tukey/Rect) ──► Envelope shape     │   │
│  │  Pitch (+scatter ±24st) ──► Playback rate                 │   │
│  │  Pan (stereo scatter) ──► Per-grain panning               │   │
│  │                                                           │   │
│  │  Freeze ──► Locks write position, grains loop in region   │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│  ┌───────────────────▼───────────────────────────────────────┐   │
│  │  GRAIN VOICES (×32 max simultaneous across all clouds)    │   │
│  │                                                           │   │
│  │  Per grain: windowed overlap-add segment                  │   │
│  │  • Pitch-shifted playback (linear interpolation)          │   │
│  │  • Per-grain amplitude envelope (window shape)            │   │
│  │  • Per-grain pan position (base + scatter)                │   │
│  │                                                           │   │
│  │  Cloud Mix: sum all active grains, normalize              │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│  ┌───────────────────▼───────────────────────────────────────┐   │
│  │  FILTER (Cytomic SVF)                                     │   │
│  │                                                           │   │
│  │  Modes: LP / BP / HP / Notch                              │   │
│  │  Key tracking + pre-filter drive                          │   │
│  │  Filter envelope modulation                               │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│  ┌───────────────────▼───────────────────────────────────────┐   │
│  │  CHARACTER STAGES                                         │   │
│  │                                                           │   │
│  │  Shimmer: harmonic fold (octave-up partial injection)     │   │
│  │  Frost: cold limiting (hard knee, no warmth)              │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│  ┌───────────────────▼───────────────────────────────────────┐   │
│  │  AMP ENVELOPE (ADSR — often slow, cloud-appropriate)      │   │
│  │                                                           │   │
│  │  Velocity sensitivity scales initial level                │   │
│  └───────────────────┬───────────────────────────────────────┘   │
│                      │                                           │
│                      ▼                                           │
│              Per-voice output [L, R]                              │
│              ──► Coupling output cache                            │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
                       │
  ┌────────────────────▼───────────────────────────────────────┐
  │  FX CHAIN (post-voice mix, stereo)                         │
  │                                                            │
  │  Smear: time-stretch without pitch change (granular        │
  │         re-scatter at 2x size, half density)               │
  │  Scatter Reverb: grain-informed reverb (size + decay)      │
  │  Delay: tempo-syncable stereo delay                        │
  │  Finish: glue compressor + width control                   │
  └────────────────────┬───────────────────────────────────────┘
                       │
                       ▼
               Final Output [L, R]
```

### 2.1 Grain Buffer (Ring Buffer)

A per-voice circular buffer storing the grain source audio:

- **Size:** 4 seconds at sample rate (e.g., 176,400 samples at 44.1kHz)
- **Memory:** ~688KB per voice at 44.1kHz × 4 bytes × 1 channel (mono)
- **Optimization:** Shared grain buffer across all voices when source is built-in oscillator (identical content). Per-voice buffers only when coupling input differs per voice. Total memory: ~688KB shared + per-voice coupling buffers only as needed.
- **Write behavior:** Continuously writes source audio at the write head position
- **Freeze behavior:** When `opal_freeze > 0`, write head decelerates. At freeze = 1.0, write head stops completely — all grains read from the frozen buffer region.

### 2.2 Grain Scheduler

Each voice runs an independent grain trigger timer:

```
timeSinceLastGrain += 1.0 / sampleRate
grainInterval = 1.0 / density

if (timeSinceLastGrain >= grainInterval):
    spawnGrain(position + random(-posScatter, +posScatter),
               grainSize,
               basePitch + random(-pitchScatter, +pitchScatter),
               basePan + random(-panScatter, +panScatter),
               windowShape)
    timeSinceLastGrain -= grainInterval
```

**Grain spawn:** Finds a free slot in the 32-grain pool. If all slots occupied, steals the oldest grain with a 1ms crossfade.

**Position calculation:** `readOffset = writeHead - (position * bufferSize)` — position 0.0 reads the most recently written audio; position 1.0 reads 4 seconds ago.

### 2.3 Grain Playback

Each active grain:

1. **Reads** from the grain buffer at a rate determined by pitch shift (linear interpolation between samples)
2. **Windows** the output with the selected envelope shape (Hann/Gaussian/Tukey/Rectangular)
3. **Pans** to its assigned stereo position (equal-power pan law)
4. **Sums** into the voice's cloud mix buffer via overlap-add

**Pitch shifting:** Playback rate = `2^(pitchOffset / 12.0)` where pitchOffset is in semitones. Rate 1.0 = original pitch. The grain reads faster or slower through the buffer.

**Window shapes:**
- **Hann (0):** Smooth fade, no clicks, standard granular — `0.5 * (1 - cos(2π * t))`
- **Gaussian (1):** Softer edges, overlaps more smoothly — `exp(-0.5 * ((t - 0.5) / 0.15)^2)`
- **Tukey (2):** Flat top with tapered edges — better for preserving transients
- **Rectangular (3):** Hard edges — creates deliberate clicks for rhythmic textures

### 2.4 Built-in Oscillator

When source mode is not "coupling", a simple oscillator bank feeds the grain buffer:

- **Source 0 — Sine:** Pure tone grain source. Clean, harmonic granulation.
- **Source 1 — Saw:** Rich harmonic content for bright shimmer textures.
- **Source 2 — Pulse:** Variable width for hollow/reedy grain character.
- **Source 3 — Noise:** White noise source for texture-only clouds (no pitch content).
- **Source 4 — Two-Osc:** Osc1 + Osc2 mixed. Detuned pair for richer source material.
- **Source 5 — Coupling:** External engine audio via `AudioToBuffer` coupling (see Section 15).

The oscillator runs at the MIDI note frequency, writing continuously into the grain buffer. This means granulating a saw wave at C3 produces different results than granulating the same saw at C5 — the buffer content is pitch-dependent, and grain pitch scatter operates on top of that.

### 2.5 Filter

Post-cloud Cytomic SVF filter, identical architecture to OVERBITE's filter block:

- **LP (0):** Low pass — tames bright grain shimmer
- **BP (1):** Band pass — vowel-like cloud filtering
- **HP (2):** High pass — removes low rumble from dense clouds
- **Notch (3):** Notch — carves frequency holes in the cloud

Key tracking follows MIDI note, so higher notes open the filter proportionally. Pre-filter drive adds soft saturation before filtering for denser tones.

### 2.6 Character Stages

Two character stages unique to OPAL:

**Shimmer:** Harmonic fold that injects octave-up partials into the cloud. At low amounts, adds airy brightness. At high amounts, the cloud becomes crystalline and bell-like. Implementation: half-wave rectification mixed with original signal.

**Frost:** Cold limiting with a hard knee and no warmth coloring. Unlike soft saturation (which adds even harmonics and warmth), Frost clips cleanly — it makes the cloud feel cold and glassy. At high amounts, individual grain transients become audible through the limiting. Implementation: hard-knee compressor with 0ms attack, 5ms release, zero makeup gain.

### 2.7 FX Chain

**Smear:** Time-stretch effect implemented as a secondary granular pass — re-scatters the cloud mix at 2× grain size and half density. Creates a diffused, smeared version of the cloud output.

**Scatter Reverb:** Algorithmic reverb tuned for granular material — size and decay parameters. Uses a Schroeder-style 4-comb + 2-allpass topology with damping.

**Delay:** Stereo delay with tempo sync option. Time, feedback, and mix controls.

**Finish:** Glue compressor (soft-knee, 2:1 ratio) + stereo width control.

---

## 3. Parameter Taxonomy (86 Parameters)

### 3.1 Grain Source (6 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 1 | `opal_source` | Source | Int | 0–5 | 1 (Saw) | Grain source: Sine/Saw/Pulse/Noise/TwoOsc/Coupling |
| 2 | `opal_oscShape` | Osc Shape | Float | 0.0–1.0 | 0.5 | Source oscillator waveshape (pulse width for Pulse mode) |
| 3 | `opal_osc2Shape` | Osc 2 Shape | Float | 0.0–1.0 | 0.5 | Second oscillator shape (Two-Osc mode) |
| 4 | `opal_osc2Mix` | Osc 2 Mix | Float | 0.0–1.0 | 0.5 | Osc1/Osc2 blend (Two-Osc mode) |
| 5 | `opal_osc2Detune` | Osc 2 Detune | Float | -24.0–24.0 st | 0.1 | Second oscillator detune in semitones |
| 6 | `opal_couplingLevel` | Coupling Level | Float | 0.0–1.0 | 0.0 | External engine audio level into grain buffer |

### 3.2 Grain Scheduler (8 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 7 | `opal_grainSize` | Grain Size | Float | 10.0–800.0 ms | 120.0 | Duration of each grain window |
| 8 | `opal_density` | Density | Float | 1.0–120.0 /s | 20.0 | Grains spawned per second per voice |
| 9 | `opal_position` | Position | Float | 0.0–1.0 | 0.0 | Playhead in grain buffer (0=now, 1=4sec ago) |
| 10 | `opal_posScatter` | Pos Scatter | Float | 0.0–1.0 | 0.1 | Position randomization per grain |
| 11 | `opal_pitchShift` | Pitch Shift | Float | -24.0–24.0 st | 0.0 | Base pitch offset for all grains |
| 12 | `opal_pitchScatter` | Pitch Scatter | Float | 0.0–24.0 st | 0.0 | Per-grain pitch randomization |
| 13 | `opal_panScatter` | Pan Scatter | Float | 0.0–1.0 | 0.3 | Stereo scatter width |
| 14 | `opal_window` | Window | Int | 0–3 | 0 (Hann) | Grain window shape: Hann/Gaussian/Tukey/Rect |

### 3.3 Freeze (2 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 15 | `opal_freeze` | Freeze | Float | 0.0–1.0 | 0.0 | Buffer freeze ratio (1=fully frozen) |
| 16 | `opal_freezeSize` | Freeze Region | Float | 0.01–1.0 | 0.25 | Size of frozen region (fraction of buffer) |

### 3.4 Filter (5 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 17 | `opal_filterCutoff` | Filter Cutoff | Float | 20.0–20000.0 Hz | 8000.0 | Post-cloud SVF cutoff |
| 18 | `opal_filterReso` | Filter Reso | Float | 0.0–1.0 | 0.15 | Filter resonance |
| 19 | `opal_filterMode` | Filter Mode | Int | 0–3 | 0 (LP) | LP/BP/HP/Notch |
| 20 | `opal_filterKeyTrack` | Key Track | Float | 0.0–1.0 | 0.3 | Filter cutoff follows MIDI note |
| 21 | `opal_filterDrive` | Filter Drive | Float | 0.0–1.0 | 0.0 | Pre-filter soft saturation |

### 3.5 Character (2 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 22 | `opal_shimmer` | Shimmer | Float | 0.0–1.0 | 0.0 | Harmonic fold — octave-up partial injection |
| 23 | `opal_frost` | Frost | Float | 0.0–1.0 | 0.0 | Cold limiting — hard knee, glassy |

### 3.6 Amp Envelope (5 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 24 | `opal_ampAttack` | Amp Attack | Float | 0.001–8.0 s | 0.3 | Cloud attack time (often slow) |
| 25 | `opal_ampDecay` | Amp Decay | Float | 0.05–4.0 s | 0.5 | Cloud decay time |
| 26 | `opal_ampSustain` | Amp Sustain | Float | 0.0–1.0 | 0.8 | Cloud sustain level |
| 27 | `opal_ampRelease` | Amp Release | Float | 0.05–8.0 s | 1.5 | Cloud release time (often slow) |
| 28 | `opal_ampVelSens` | Velocity | Float | 0.0–1.0 | 0.4 | Velocity sensitivity |

### 3.7 Filter Envelope (5 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 29 | `opal_filterEnvAmt` | Filter Env Amt | Float | -1.0–1.0 | 0.3 | Filter envelope → cutoff modulation depth |
| 30 | `opal_filterAttack` | Filter Attack | Float | 0.001–8.0 s | 0.5 | Filter envelope attack |
| 31 | `opal_filterDecay` | Filter Decay | Float | 0.05–4.0 s | 0.8 | Filter envelope decay |
| 32 | `opal_filterSustain` | Filter Sustain | Float | 0.0–1.0 | 0.3 | Filter envelope sustain |
| 33 | `opal_filterRelease` | Filter Release | Float | 0.05–8.0 s | 1.0 | Filter envelope release |

### 3.8 LFO 1 (6 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 34 | `opal_lfo1Shape` | LFO1 Shape | Int | 0–5 | 0 (Sine) | Sine/Tri/Saw/Square/Random/Stepped |
| 35 | `opal_lfo1Rate` | LFO1 Rate | Float | 0.01–20.0 Hz | 0.5 | LFO frequency |
| 36 | `opal_lfo1Depth` | LFO1 Depth | Float | 0.0–1.0 | 0.0 | LFO modulation depth |
| 37 | `opal_lfo1Sync` | LFO1 Sync | Int | 0–1 | 0 | Tempo sync on/off |
| 38 | `opal_lfo1Retrigger` | LFO1 Retrig | Int | 0–1 | 0 | Retrigger on note-on |
| 39 | `opal_lfo1Phase` | LFO1 Phase | Float | 0.0–1.0 | 0.0 | Start phase when retriggered |

### 3.9 LFO 2 (6 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 40 | `opal_lfo2Shape` | LFO2 Shape | Int | 0–5 | 0 (Sine) | Sine/Tri/Saw/Square/Random/Stepped |
| 41 | `opal_lfo2Rate` | LFO2 Rate | Float | 0.01–20.0 Hz | 2.0 | LFO frequency |
| 42 | `opal_lfo2Depth` | LFO2 Depth | Float | 0.0–1.0 | 0.0 | LFO modulation depth |
| 43 | `opal_lfo2Sync` | LFO2 Sync | Int | 0–1 | 0 | Tempo sync on/off |
| 44 | `opal_lfo2Retrigger` | LFO2 Retrig | Int | 0–1 | 0 | Retrigger on note-on |
| 45 | `opal_lfo2Phase` | LFO2 Phase | Float | 0.0–1.0 | 0.0 | Start phase when retriggered |

### 3.10 Mod Matrix — 6 slots × 3 (18 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 46 | `opal_modSlot1Src` | Mod 1 Src | Int | 0–7 | 0 (None) | Source: None/LFO1/LFO2/FilterEnv/AmpEnv/Vel/KeyTrack/ModWheel |
| 47 | `opal_modSlot1Dst` | Mod 1 Dst | Int | 0–11 | 0 (None) | Dest: None/GrainSize/Density/Position/PitchScatter/PanScatter/FilterCutoff/Shimmer/Frost/Freeze/OscShape/Level |
| 48 | `opal_modSlot1Amt` | Mod 1 Amt | Float | -1.0–1.0 | 0.0 | Modulation amount (bipolar) |
| 49 | `opal_modSlot2Src` | Mod 2 Src | Int | 0–7 | 0 | Source |
| 50 | `opal_modSlot2Dst` | Mod 2 Dst | Int | 0–11 | 0 | Destination |
| 51 | `opal_modSlot2Amt` | Mod 2 Amt | Float | -1.0–1.0 | 0.0 | Amount |
| 52 | `opal_modSlot3Src` | Mod 3 Src | Int | 0–7 | 0 | Source |
| 53 | `opal_modSlot3Dst` | Mod 3 Dst | Int | 0–11 | 0 | Destination |
| 54 | `opal_modSlot3Amt` | Mod 3 Amt | Float | -1.0–1.0 | 0.0 | Amount |
| 55 | `opal_modSlot4Src` | Mod 4 Src | Int | 0–7 | 0 | Source |
| 56 | `opal_modSlot4Dst` | Mod 4 Dst | Int | 0–11 | 0 | Destination |
| 57 | `opal_modSlot4Amt` | Mod 4 Amt | Float | -1.0–1.0 | 0.0 | Amount |
| 58 | `opal_modSlot5Src` | Mod 5 Src | Int | 0–7 | 0 | Source |
| 59 | `opal_modSlot5Dst` | Mod 5 Dst | Int | 0–11 | 0 | Destination |
| 60 | `opal_modSlot5Amt` | Mod 5 Amt | Float | -1.0–1.0 | 0.0 | Amount |
| 61 | `opal_modSlot6Src` | Mod 6 Src | Int | 0–7 | 0 | Source |
| 62 | `opal_modSlot6Dst` | Mod 6 Dst | Int | 0–11 | 0 | Destination |
| 63 | `opal_modSlot6Amt` | Mod 6 Amt | Float | -1.0–1.0 | 0.0 | Amount |

**Mod sources (0–7):** None, LFO1, LFO2, FilterEnv, AmpEnv, Velocity, KeyTrack, ModWheel
**Mod destinations (0–11):** None, GrainSize, Density, Position, PitchScatter, PanScatter, FilterCutoff, Shimmer, Frost, Freeze, OscShape, Level

### 3.11 Macros (4 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 64 | `opal_macroScatter` | SCATTER | Float | 0.0–1.0 | 0.0 | Grain size ↕ + density inverse — texture dial |
| 65 | `opal_macroDrift` | DRIFT | Float | 0.0–1.0 | 0.0 | posScatter + pitchScatter + panScatter — dissolution dial |
| 66 | `opal_macroCoupling` | COUPLING | Float | 0.0–1.0 | 0.0 | couplingLevel + freeze — the portal |
| 67 | `opal_macroSpace` | SPACE | Float | 0.0–1.0 | 0.0 | Reverb mix + smear — spatial expansion |

### 3.12 FX — Smear (2 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 68 | `opal_fxSmearAmount` | Smear | Float | 0.0–1.0 | 0.0 | Time-stretch diffusion amount |
| 69 | `opal_fxSmearMix` | Smear Mix | Float | 0.0–1.0 | 0.0 | Smear wet/dry |

### 3.13 FX — Scatter Reverb (4 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 70 | `opal_fxReverbSize` | Reverb Size | Float | 0.0–1.0 | 0.4 | Room size |
| 71 | `opal_fxReverbDecay` | Reverb Decay | Float | 0.1–10.0 s | 2.0 | Decay time |
| 72 | `opal_fxReverbDamping` | Reverb Damp | Float | 0.0–1.0 | 0.5 | High-frequency damping |
| 73 | `opal_fxReverbMix` | Reverb Mix | Float | 0.0–1.0 | 0.0 | Reverb wet/dry |

### 3.14 FX — Delay (5 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 74 | `opal_fxDelayTime` | Delay Time | Float | 0.01–2.0 s | 0.35 | Delay time |
| 75 | `opal_fxDelayFeedback` | Delay FB | Float | 0.0–0.95 | 0.3 | Feedback amount |
| 76 | `opal_fxDelayMix` | Delay Mix | Float | 0.0–1.0 | 0.0 | Delay wet/dry |
| 77 | `opal_fxDelaySync` | Delay Sync | Int | 0–1 | 0 | Tempo sync on/off |
| 78 | `opal_fxDelaySpread` | Delay Spread | Float | 0.0–1.0 | 0.3 | L/R time offset for stereo width |

### 3.15 FX — Finish (3 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 79 | `opal_fxFinishGlue` | Glue | Float | 0.0–1.0 | 0.1 | Glue compressor amount |
| 80 | `opal_fxFinishWidth` | Width | Float | 0.0–2.0 | 1.0 | Stereo width (0=mono, 1=normal, 2=wide) |
| 81 | `opal_fxFinishLevel` | Level | Float | 0.0–1.0 | 0.75 | Final output level |

### 3.16 Voice (5 parameters)

| # | Parameter ID | Display Name | Type | Range | Default | DSP Function |
|---|---|---|---|---|---|---|
| 82 | `opal_voiceMode` | Voice Mode | Int | 0–1 | 0 (Poly) | Poly/Mono |
| 83 | `opal_glideTime` | Glide | Float | 0.0–2.0 s | 0.0 | Portamento time |
| 84 | `opal_glideMode` | Glide Mode | Int | 0–1 | 0 (Always) | Always/Legato only |
| 85 | `opal_pan` | Pan | Float | -1.0–1.0 | 0.0 | Master stereo position |
| 86 | `opal_level` | Level | Float | 0.0–1.0 | 0.75 | Master output level |

**Total: 86 frozen parameter IDs.**

---

## 4. Macro Mapping

| Macro | Label | Targets | Curve | Musical Effect |
|---|---|---|---|---|
| M1 | SCATTER | `opal_grainSize` (120→10ms) + `opal_density` (20→100/s) | Inverse exponential | 0=lush clouds → 1=dense shimmer. Grain size shrinks while density rises — the fundamental granular texture control. |
| M2 | DRIFT | `opal_posScatter` (0→1) + `opal_pitchScatter` (0→12st) + `opal_panScatter` (0→1) | Linear | 0=laser-focused → 1=total dissolution. All three scatter dimensions open simultaneously. |
| M3 | COUPLING | `opal_couplingLevel` (0→1) + `opal_freeze` (0→0.8) | Square root (fast initial response) | 0=internal osc only → 1=external audio dominates and partially freezes. The portal to other engines. |
| M4 | SPACE | `opal_fxReverbMix` (0→0.6) + `opal_fxSmearMix` (0→0.5) + `opal_fxDelayMix` (0→0.3) | Linear | 0=dry cloud → 1=massive spatial expansion. All three spatial FX open together. |

**Rule:** All 4 macros produce audible, significant change at every point in their range in every preset. This is verified during preset QA — any preset where a macro is inaudible must be redesigned.

---

## 5. Voice Architecture

- **Max voices:** 12 grain clouds (polyphonic — each MIDI note = one cloud)
- **Grain pool:** 32 simultaneous grains shared across all active voices. Each voice is allocated grains proportionally: `voiceGrainBudget = 32 / activeVoiceCount`.
- **Voice stealing:** Oldest voice (LRU), 5ms crossfade (consistent with all engines)
- **Per-voice state:** Grain buffer read head, grain scheduler timer, per-grain states (×up to 32), filter state, character stage state, ADSR envelope, ADSR filter envelope. All pre-allocated. No dynamic memory.

### 5.1 Note-On Behavior

1. Allocate voice (steal oldest if at capacity)
2. Set oscillator frequency to MIDI note frequency
3. Start grain scheduler — first grain spawns immediately (no delay)
4. Trigger amp envelope attack
5. Trigger filter envelope attack
6. Apply velocity scaling: `level *= (1.0 - velSens) + velSens * (velocity / 127.0)`
7. If glide active: set glide target frequency, interpolate

### 5.2 Note-Off Behavior

1. Amp envelope enters release stage
2. Filter envelope enters release stage
3. Grain scheduler continues during release (grains keep spawning and fading naturally)
4. Voice freed when amp envelope reaches zero

### 5.3 Legato (Mono Mode)

- Grain buffer continues unbroken (no restart)
- Oscillator frequency glides to new note
- Grain scheduler continues without interruption
- Amp envelope does NOT retrigger — sustain level continues
- Filter envelope retriggers from current level (no attack restart)

### 5.4 Grain Budget Scaling

When CPU budget is tight (4-engine config), grain density is capped:
- **1-2 engines active:** Full 32 grain pool, density up to 120/s
- **3 engines active:** 24 grain pool, density capped at 80/s
- **4 engines active:** 16 grain pool, density capped at 30/s (eco mode)

This is managed by the EngineRegistry notifying OPAL of active engine count via `setActiveEngineCount(int count)`.

---

## 6. Grain Scheduler Algorithm

The grain scheduler is the heart of OPAL. Detailed algorithm:

```
struct Grain {
    float readPosition;       // Position in grain buffer (samples)
    float readIncrement;      // Playback rate (pitch shift)
    float windowPhase;        // 0.0 → 1.0 over grain lifetime
    float windowIncrement;    // 1.0 / (grainSizeSamples)
    float panL, panR;         // Equal-power pan coefficients
    bool active;
};

// Per voice, per sample:
void processGrainScheduler(float* outL, float* outR) {
    triggerAccumulator += 1.0f;
    float triggerInterval = sampleRate / density;

    while (triggerAccumulator >= triggerInterval) {
        triggerAccumulator -= triggerInterval;
        spawnGrain();
    }

    float sumL = 0.0f, sumR = 0.0f;
    int activeCount = 0;

    for (auto& grain : grainPool) {
        if (!grain.active) continue;

        // Read from buffer with linear interpolation
        float sample = readBufferInterpolated(grain.readPosition);

        // Apply window envelope
        float window = computeWindow(grain.windowPhase, windowShape);
        sample *= window;

        // Accumulate stereo
        sumL += sample * grain.panL;
        sumR += sample * grain.panR;

        // Advance grain
        grain.readPosition += grain.readIncrement;
        grain.windowPhase += grain.windowIncrement;
        activeCount++;

        // Grain finished
        if (grain.windowPhase >= 1.0f)
            grain.active = false;
    }

    // Normalize by sqrt of active grain count (energy-preserving)
    float norm = (activeCount > 0) ? 1.0f / std::sqrt((float)activeCount) : 0.0f;
    *outL = sumL * norm;
    *outR = sumR * norm;
}

void spawnGrain() {
    Grain& g = findFreeGrain();  // LRU steal if full

    // Position: relative to write head
    float basePos = writeHead - (position * bufferSize);
    float scatter = (random() * 2.0f - 1.0f) * posScatter * bufferSize;
    g.readPosition = wrapBufferIndex(basePos + scatter);

    // Pitch: base + scatter
    float pitchOffset = pitchShift + (random() * 2.0f - 1.0f) * pitchScatter;
    g.readIncrement = std::pow(2.0f, pitchOffset / 12.0f);

    // Window
    float grainSizeSamples = grainSize * 0.001f * sampleRate;
    g.windowPhase = 0.0f;
    g.windowIncrement = 1.0f / grainSizeSamples;

    // Pan: base + scatter
    float panPos = pan + (random() * 2.0f - 1.0f) * panScatter;
    panPos = juce::jlimit(-1.0f, 1.0f, panPos);
    g.panL = std::cos((panPos + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
    g.panR = std::sin((panPos + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

    g.active = true;
}
```

**Random number generation:** Per-voice PRNG (xorshift32) seeded from voice index + note number. Deterministic per note — same note replayed produces same grain pattern. This makes presets reproducible.

---

## 7. Coupling Interface

### 7.1 As Coupling TARGET (receiving from other engines)

| CouplingType | OPAL Behavior | Musical Effect |
|---|---|---|
| `AudioToBuffer` | Streams source stereo audio continuously into grain ring buffer (mixes with or replaces internal osc) | **Primary coupling. The Time Telescope.** Any engine's sound becomes OPAL's grain source. See Section 15 for full design. |
| `AudioToWavetable` | Treated as alias for `AudioToBuffer` at OPAL's `applyCouplingInput` handler — both route to grain buffer write | Legacy compatibility. New routes should use `AudioToBuffer`. |
| `AmpToFilter` | Source amplitude → filter cutoff modulation | Drum hits open OPAL's filter — rhythmic cloud filtering |
| `EnvToMorph` | Source envelope → grain size modulation | Attack shapes → grain size variation. Short envelopes = tiny grains. |
| `LFOToPitch` | Source LFO → pitch scatter depth | Cross-engine modulation of the scatter width |
| `RhythmToBlend` | Rhythm signal → density modulation | Beat-synced grain density — rhythmic clouds |
| `EnvToDecay` | Source envelope → freeze amount | Envelope controls freeze — sound freezes on release |

### 7.2 Unsupported Types (no-op)

| CouplingType | Why |
|---|---|
| `AmpToChoke` | Choking a cloud kills the entire texture instantly — no musical use. Clouds should fade, not choke. |
| `PitchToPitch` | Grain pitch scatter already provides pitch variation; external pitch coupling creates confusion with the scatter system. |
| `AudioToFM` | FM doesn't map to granular — grains are windowed buffer reads, not oscillators. |
| `AudioToRing` | Ring modulation on grain output produces noise, not useful texture. |
| `FilterToFilter` | OPAL's filter is post-cloud — cascading external filter coupling produces murky results. |

### 7.3 As Coupling SOURCE

`getSampleForCoupling()` returns: post-filter, post-character cloud output, stereo, normalized ±1.

| Channel | Content | Use Case |
|---|---|---|
| 0 | Left cloud output | Standard audio coupling source |
| 1 | Right cloud output | Standard audio coupling source |

**Best receiving engines:**
- **OVERDUB** — grain cloud through dub echo/spring chain. Granular dub.
- **ODDOSCAR** — cloud envelope drives wavetable morph position via `EnvToMorph`
- **OVERBITE** — cloud density drives bass filter cutoff via `AmpToFilter`. Breathing bass.

---

## 8. PlaySurface Mapping

### Pad Mode (Cloud Sculptor)

| Axis | Target | Musical Effect |
|---|---|---|
| X | `opal_position` | Scrub through the grain buffer — time travel |
| Y | `opal_density` | Sparse particles ↔ dense shimmer |
| Z (pressure) | `opal_freeze` | Pressure freezes the buffer — hold to suspend |

### Fretless Mode (Scatter Field)

| Axis | Target | Musical Effect |
|---|---|---|
| X | `opal_pitchScatter` | Continuous pitch dissolution |
| Y | `opal_panScatter` | Continuous stereo dissolution |

### Drum Mode (Burst Trigger)

| Event | Behavior |
|---|---|
| Strike | Spawns a burst of 8 grains simultaneously (ignoring density timer) |
| Velocity | Maps to grain size (harder = shorter grains = sharper transient) |
| Result | Percussive grain burst → natural density-controlled tail |

---

## 9. Preset Archetypes with DNA and Mood

| # | Name | Mood | Standalone? | DNA (B/W/M/D/S/A) | Key Settings |
|---|---|---|---|---|---|
| 1 | Glass Cloud | Foundation | Yes | 0.5/0.6/0.3/0.4/0.4/0.1 | Saw source, size 200ms, density 15, low scatter. First encounter — warm, musical cloud. |
| 2 | Frozen Bloom | Atmosphere | Yes | 0.3/0.8/0.5/0.3/0.8/0.0 | Sine source, size 400ms, freeze 0.7, reverb 0.5. Suspended, ethereal pad. |
| 3 | Particle Storm | Flux | Yes | 0.7/0.2/0.9/0.8/0.2/0.5 | Noise source, size 15ms, density 100, max scatter. Dense, chaotic, evolving. |
| 4 | Chip Scatter | Entangled | No | 0.6/0.3/0.8/0.6/0.5/0.3 | Coupling source, size 80ms, pitch scatter 7st. OVERWORLD audio granulated. |
| 5 | Climax Particles | Entangled | No | 0.4/0.7/0.7/0.5/0.7/0.2 | Coupling source, size 300ms, freeze 0.5. ODYSSEY Climax bloom frozen mid-bloom. |
| 6 | Shimmer Veil | Prism | Yes | 0.8/0.4/0.4/0.6/0.3/0.1 | Saw source, size 30ms, density 80, shimmer 0.6. Bright, airy, crystalline. |
| 7 | Deep Drift | Aether | Yes | 0.2/0.7/0.6/0.2/0.9/0.0 | Sine source, size 600ms, density 5, smear 0.7, reverb 0.6. Near-silent, vast, generative. |
| 8 | Granular Dub | Entangled | No | 0.4/0.5/0.6/0.4/0.6/0.2 | Coupling source, size 150ms. OPAL → OVERDUB. Cloud through dub echo. |
| 9 | Bass Breath | Entangled | No | 0.3/0.6/0.5/0.5/0.3/0.3 | Coupling source. OPAL → OVERBITE via AmpToFilter. Cloud density drives bass filter. |
| 10 | Scatter Glass | Flux | Yes | 0.6/0.1/0.8/0.7/0.1/0.7 | Pulse source, rect window, size 10ms, density 60, frost 0.5. Rhythmic, percussive, cold. |

**"Glass Cloud"** is the first-encounter preset: saw oscillator source, medium grain size, gentle density, low scatter. Produces a warm textured cloud that's immediately musical. Macros produce obvious changes. No coupling required.

---

## 10. .xometa Preset Structure

```json
{
  "schema_version": 1,
  "name": "Glass Cloud",
  "mood": "Foundation",
  "engines": ["Opal"],
  "author": "XO_OX Designs",
  "version": "1.0.0",
  "description": "A warm granular cloud that shimmers like light through glass. The first cloud you hear.",
  "tags": ["pad", "texture", "warm", "cloud", "foundation"],
  "macroLabels": ["SCATTER", "DRIFT", "COUPLING", "SPACE"],
  "couplingIntensity": "None",
  "tempo": null,
  "dna": {
    "brightness": 0.5,
    "warmth": 0.6,
    "movement": 0.3,
    "density": 0.4,
    "space": 0.4,
    "aggression": 0.1
  },
  "parameters": {
    "Opal": {
      "opal_source": 1,
      "opal_oscShape": 0.5,
      "opal_osc2Shape": 0.5,
      "opal_osc2Mix": 0.5,
      "opal_osc2Detune": 0.1,
      "opal_couplingLevel": 0.0,
      "opal_grainSize": 200.0,
      "opal_density": 15.0,
      "opal_position": 0.0,
      "opal_posScatter": 0.1,
      "opal_pitchShift": 0.0,
      "opal_pitchScatter": 1.0,
      "opal_panScatter": 0.3,
      "opal_window": 0,
      "opal_freeze": 0.0,
      "opal_freezeSize": 0.25,
      "opal_filterCutoff": 8000.0,
      "opal_filterReso": 0.15,
      "opal_filterMode": 0,
      "opal_filterKeyTrack": 0.3,
      "opal_filterDrive": 0.0,
      "opal_shimmer": 0.15,
      "opal_frost": 0.0,
      "opal_ampAttack": 0.4,
      "opal_ampDecay": 0.5,
      "opal_ampSustain": 0.8,
      "opal_ampRelease": 2.0,
      "opal_ampVelSens": 0.4,
      "opal_filterEnvAmt": 0.2,
      "opal_filterAttack": 0.5,
      "opal_filterDecay": 0.8,
      "opal_filterSustain": 0.3,
      "opal_filterRelease": 1.0,
      "opal_lfo1Shape": 0,
      "opal_lfo1Rate": 0.3,
      "opal_lfo1Depth": 0.15,
      "opal_lfo1Sync": 0,
      "opal_lfo1Retrigger": 0,
      "opal_lfo1Phase": 0.0,
      "opal_lfo2Shape": 0,
      "opal_lfo2Rate": 2.0,
      "opal_lfo2Depth": 0.0,
      "opal_lfo2Sync": 0,
      "opal_lfo2Retrigger": 0,
      "opal_lfo2Phase": 0.0,
      "opal_modSlot1Src": 1,
      "opal_modSlot1Dst": 3,
      "opal_modSlot1Amt": 0.2,
      "opal_modSlot2Src": 0,
      "opal_modSlot2Dst": 0,
      "opal_modSlot2Amt": 0.0,
      "opal_modSlot3Src": 0,
      "opal_modSlot3Dst": 0,
      "opal_modSlot3Amt": 0.0,
      "opal_modSlot4Src": 0,
      "opal_modSlot4Dst": 0,
      "opal_modSlot4Amt": 0.0,
      "opal_modSlot5Src": 0,
      "opal_modSlot5Dst": 0,
      "opal_modSlot5Amt": 0.0,
      "opal_modSlot6Src": 0,
      "opal_modSlot6Dst": 0,
      "opal_modSlot6Amt": 0.0,
      "opal_macroScatter": 0.0,
      "opal_macroDrift": 0.0,
      "opal_macroCoupling": 0.0,
      "opal_macroSpace": 0.0,
      "opal_fxSmearAmount": 0.0,
      "opal_fxSmearMix": 0.0,
      "opal_fxReverbSize": 0.4,
      "opal_fxReverbDecay": 2.0,
      "opal_fxReverbDamping": 0.5,
      "opal_fxReverbMix": 0.15,
      "opal_fxDelayTime": 0.35,
      "opal_fxDelayFeedback": 0.3,
      "opal_fxDelayMix": 0.0,
      "opal_fxDelaySync": 0,
      "opal_fxDelaySpread": 0.3,
      "opal_fxFinishGlue": 0.1,
      "opal_fxFinishWidth": 1.0,
      "opal_fxFinishLevel": 0.75,
      "opal_voiceMode": 0,
      "opal_glideTime": 0.0,
      "opal_glideMode": 0,
      "opal_pan": 0.0,
      "opal_level": 0.75
    }
  },
  "coupling": null,
  "sequencer": null
}
```

---

## 11. Visual Identity

- **Accent Color:** Iridescent Lavender `#A78BFA` — precious, shifting, crystalline
- **Material Concept:** Layers of precious opal — the iridescent fire inside the stone, light refracting through mineral layers
- **Icon Concept:** A fractal grain cloud — concentric scatter of dots at various opacities, suggesting both the opal stone and the granular particle cloud
- **Panel Character:** Translucent depth. The panel surface should feel like looking through glass at layers behind it. Grain particles appear as tiny animated dots that scatter with density/position changes.

**UI Pages (5):**
1. **Main** — Source select, grain size, density, position, freeze, macros, level
2. **Grain** — Full scheduler controls: scatter parameters, window shape, pitch shift, freeze region
3. **Mod** — Filter, character stages, envelopes, LFOs, mod matrix
4. **FX** — Smear, scatter reverb, delay, finish
5. **Browser** — Preset library with mood/DNA filtering

---

## 12. Best Coupling Partners

| Partner | Route | Musical Effect |
|---|---|---|
| **OVERWORLD** | OVERWORLD → OPAL via `AudioToBuffer` | **The killer coupling.** NES pulses, FM operators, SNES samples scattered through time clouds. Chip audio granulated. |
| **ODYSSEY** | ODYSSEY → OPAL via `AudioToBuffer` | Psychedelic pads granulated — Climax bloom FREEZE-captured mid-bloom and held as particle field. |
| **OBLONG** | OBLONG → OPAL via `AudioToBuffer` | Warm fuzzy textures scattered through time — organic cloud material |
| **ODDFELIX** | ODDFELIX → OPAL via `AudioToBuffer` | Karplus-Strong pluck granulated — reverb made of its own attack |
| **OBESE** | OBESE → OPAL via `AmpToFilter` | 13-osc amplitude modulates cloud filter — rhythmic brightness |
| **OVERDUB** | OPAL → OVERDUB via `getSample` | Grain cloud through dub echo/spring chain — granular dub |
| **ODDOSCAR** | OPAL → ODDOSCAR via `EnvToMorph` | Cloud envelope drives wavetable morph position — cloud-reactive wavetable |
| **OVERBITE** | OPAL → OVERBITE via `AmpToFilter` | Cloud density drives bass filter — breathing bass |
| **ONSET** | ONSET → OPAL via `RhythmToBlend` | Drum pattern drives grain density — rhythmic clouds |

---

## 13. CPU Budget Verification

| Component | Cost per sample per voice | Notes |
|---|---|---|
| Grain buffer write | ~2 ops | Ring buffer index + write |
| Grain scheduler | ~10 ops | Timer + spawn logic (amortized) |
| Active grains (avg 8) | ~64 ops | 8 grains × (interpolated read + window + pan) |
| Filter (Cytomic SVF) | ~12 ops | 2 channels × 6 ops |
| Character (shimmer+frost) | ~8 ops | Half-wave rect + hard limiter |
| Amp envelope | ~4 ops | ADSR state machine |
| Filter envelope | ~4 ops | ADSR state machine |
| LFO × 2 | ~8 ops | Phase increment + shape |
| **Total per voice** | **~112 ops** | Average grain count |
| **12 voices worst case** | **~1344 ops/sample** | All voices active |
| **At 44.1kHz** | **~59M ops/s** | ~0.4% of M1 single core |

FX chain (shared, not per-voice):

| Component | Cost per sample | Notes |
|---|---|---|
| Smear (secondary grain pass) | ~50 ops | Reduced grain set |
| Scatter reverb (4 comb + 2 AP) | ~40 ops | Per-channel |
| Delay (stereo) | ~12 ops | Read + write + feedback |
| Finish (glue + width) | ~10 ops | Compressor + M/S |
| **Total FX** | **~112 ops/sample** | |

**Total worst case:** ~1456 ops/sample at 44.1kHz ≈ 64M ops/s ≈ ~0.5% M1 single core.

Well within the 12% single-engine budget. The variable cost is grain count — at maximum density (120/s) with large grains (800ms), simultaneous grain count can peak at 32. Even at 32 grains × 12 voices = worst theoretical ~3840 ops/sample ≈ ~170M ops/s ≈ ~1.1% M1 — still comfortable.

**Memory budget:**
- Shared grain buffer: 44100 × 4 sec × 4 bytes = ~688KB
- Per-voice grain states: 32 grains × ~40 bytes × 12 voices = ~15KB
- FX buffers: ~200KB (delay lines, reverb combs)
- **Total: ~900KB** — negligible

---

## 14. Verification Checklist

- [x] Naming confirmed: XOpal, `opal_` prefix, `#A78BFA`, Engine ID `"Opal"`
- [x] 86 parameters defined with `opal_` namespace IDs — all frozen
- [x] New `AudioToBuffer` coupling type designed (Section 15) — requires addition to `CouplingType` enum
- [x] Primary coupling type identified: `AudioToBuffer` (the Time Telescope — reason OPAL exists)
- [x] `AudioToWavetable` handled as alias at `applyCouplingInput` for legacy compatibility
- [x] `AudioToBuffer` data structure: stereo `AudioRingBuffer` (pre-allocated, lock-free, audio-thread safe)
- [x] Push model chosen: source engine writes to OPAL's ring buffer; OPAL reads on its own schedule
- [x] Feedback loop safety: engine registry cycle detection prevents OPAL→Engine→OPAL chains
- [x] Buffer sizing: 4s at sample rate, pre-allocated in `prepare()`, no audio-thread allocation
- [x] FREEZE gesture: momentary hold (not toggle), latch mode via double-tap, hardware-mapped to Z-pressure
- [x] Voice count = 12, CPU budget verified at <1.1% worst case (well under 12%)
- [x] Memory budget verified at ~900KB + 688KB per AudioToBuffer source (up to 4 sources)
- [x] Grain scheduler algorithm specified with deterministic PRNG
- [x] Grain budget scaling for multi-engine configs (32/24/16 grains)
- [x] 4 window shapes defined (Hann/Gaussian/Tukey/Rectangular)
- [x] Built-in oscillator: 5 source types + coupling
- [x] Filter: Cytomic SVF with 4 modes, key tracking, drive
- [x] Character stages: Shimmer (harmonic fold) + Frost (cold limiting)
- [x] FX chain: Smear + Scatter Reverb + Delay + Finish
- [x] 4 macros defined with target parameters and curves
- [x] Note-on/off/legato behavior specified
- [x] PlaySurface mapping for all 3 modes
- [x] 10 preset archetypes with mood assignments and DNA values
- [x] "Glass Cloud" first-encounter preset fully specified with all 86 params
- [x] Coupling interface: 7 supported types, 5 rejected with reasons
- [x] Coupling source output: 2-channel stereo post-filter
- [x] .xometa JSON structure verified with all 86 parameters
- [x] Mod matrix: 6 slots, 8 sources, 12 destinations
- [x] Freeze system: write-head deceleration + region size + momentary/latch gesture
- [x] UI page structure: 5 pages
- [ ] `AudioToBuffer` enum value added to `Source/Core/SynthEngine.h` CouplingType enum (Phase 2 gate)

---

## 15. AudioToBuffer Coupling Type — Full Design

*V008 implementation spec. This section defines both the new `CouplingType` enum value and the complete data pathway from source engine audio output to OPAL's grain buffer.*

---

### 15.1 Why a New Coupling Type

The existing audio coupling types (`AudioToWavetable`, `AudioToFM`, `AudioToRing`) pass audio through the `MegaCouplingMatrix`'s mono scratch buffer (`couplingBuffer`), mixing L+R to mono in-place:

```cpp
// Current MegaCouplingMatrix::processBlock() — audio routes
couplingBuffer[i] = (source->getSampleForCoupling(0, i)
                   + source->getSampleForCoupling(1, i)) * 0.5f;
```

This is sufficient for FM carrier and ring modulation — both are mono multiplication operations. It is *not* sufficient for OPAL's grain buffer, which needs:

1. **Stereo audio** — grains should preserve L/R spatial character of the source engine
2. **Continuous streaming** — the grain buffer is a 4-second history; every block must write, not just when coupling fires
3. **Variable write rate** — OPAL may freeze the write head (FREEZE gesture) while the source continues writing to a shadow buffer
4. **Low-latency random access** — OPAL reads arbitrary positions in the buffer; random access must be O(1)

`AudioToWavetable` was designed to periodically copy an audio snapshot into a wavetable — a different paradigm. `AudioToBuffer` is specifically designed for continuous streaming into a pre-allocated circular buffer.

---

### 15.2 CouplingType Enum Addition

In `Source/Core/SynthEngine.h`, add to the `CouplingType` enum:

```cpp
enum class CouplingType {
    AmpToFilter,
    AmpToPitch,
    LFOToPitch,
    EnvToMorph,
    AudioToFM,
    AudioToRing,
    FilterToFilter,
    AmpToChoke,
    RhythmToBlend,
    EnvToDecay,
    PitchToPitch,
    AudioToWavetable,
    AudioToBuffer       // NEW — continuous stereo audio stream into target's ring buffer
};
```

In `MegaCouplingMatrix::processBlock()`, add `AudioToBuffer` to the `isAudioRoute` detection:

```cpp
const bool isAudioRoute =
    route.type == CouplingType::AudioToWavetable
 || route.type == CouplingType::AudioToFM
 || route.type == CouplingType::AudioToRing
 || route.type == CouplingType::AudioToBuffer;  // ADD THIS LINE
```

For `AudioToBuffer` routes, bypass the mono mixdown and pass stereo directly — see Section 15.4 for the extended `processBlock` logic.

---

### 15.3 AudioRingBuffer Data Structure

OPAL owns its grain ring buffer. For coupled audio sources, it allocates one additional `AudioRingBuffer` per active `AudioToBuffer` source in `prepare()`:

```cpp
// OpalEngine.h — pre-allocated in prepare(), not on audio thread
struct AudioRingBuffer {
    static constexpr int kChannels = 2;

    std::vector<float> data[kChannels];   // [L][...], [R][...] — interleaved by channel
    int capacity = 0;                     // bufferSize in samples
    std::atomic<int> writeHead{0};        // written by coupling push (audio thread)
    std::atomic<int> shadowWriteHead{0};  // tracks pre-freeze write position
    std::atomic<bool> frozen{false};      // FREEZE state

    void prepare(int sampleRate, float durationSeconds)
    {
        capacity = static_cast<int>(sampleRate * durationSeconds);
        for (int ch = 0; ch < kChannels; ++ch)
            data[ch].assign(static_cast<size_t>(capacity), 0.0f);
        writeHead.store(0, std::memory_order_relaxed);
        shadowWriteHead.store(0, std::memory_order_relaxed);
        frozen.store(false, std::memory_order_relaxed);
    }

    // Called from MegaCouplingMatrix::processBlock() on audio thread
    void pushBlock(const float* srcL, const float* srcR, int numSamples, float level)
    {
        const bool isFrozen = frozen.load(std::memory_order_acquire);
        int head = writeHead.load(std::memory_order_relaxed);
        int shadowHead = shadowWriteHead.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            // Shadow head always advances — records pre-freeze history
            int s = shadowHead % capacity;
            // Only write to main buffer if not frozen
            if (!isFrozen)
            {
                int w = head % capacity;
                data[0][static_cast<size_t>(w)] = srcL[i] * level;
                data[1][static_cast<size_t>(w)] = srcR[i] * level;
                ++head;
            }
            ++shadowHead;
        }

        writeHead.store(head, std::memory_order_release);
        shadowWriteHead.store(shadowHead, std::memory_order_release);
    }

    // Called from grain scheduler — O(1), read-only
    float readAt(int channel, float fractionalOffset) const
    {
        int head = writeHead.load(std::memory_order_acquire);
        // fractionalOffset: 0.0 = most recent, 1.0 = capacity samples ago
        float rawPos = static_cast<float>(head) - fractionalOffset * static_cast<float>(capacity);
        int base = static_cast<int>(rawPos);
        float frac = rawPos - static_cast<float>(base);

        int i0 = ((base % capacity) + capacity) % capacity;
        int i1 = ((base + 1) % capacity + capacity) % capacity;

        const float* ch = data[static_cast<size_t>(channel)].data();
        return ch[i0] + frac * (ch[i1] - ch[i0]);  // linear interpolation
    }
};
```

**Memory cost:** 2 channels × 4 sec × 44100 samples × 4 bytes = **~688KB per source engine**. For a 4-engine config with 3 sources feeding OPAL, worst case is ~2MB additional — still negligible.

**Thread safety contract:**
- `pushBlock()` is called on the audio thread from `MegaCouplingMatrix::processBlock()`. It uses atomic write head — no locks.
- `readAt()` is called on the audio thread from the grain scheduler. It uses `memory_order_acquire` to see the latest `writeHead`. No locks.
- `frozen` flag is set from the FREEZE gesture handler on the audio thread (parameter change driven) — `memory_order_acquire/release` pair guarantees visibility.

---

### 15.4 Push vs Pull Model

**Decision: Push model.** The source engine does not know about OPAL. The `MegaCouplingMatrix` pushes source audio into OPAL's `AudioRingBuffer` each block — this is consistent with the existing coupling architecture where the matrix mediates all engine interaction.

**Sequence per block:**

```
1. MegaCouplingMatrix::processBlock() iterates routes
2. For AudioToBuffer routes:
   a. Get source L/R from source->getSampleForCoupling(0, i) and (1, i)
   b. Call dest->getGrainBuffer(sourceSlot)->pushBlock(srcL, srcR, numSamples, route.amount)
3. OpalEngine::renderBlock() runs after all coupling pushes complete
4. Grain scheduler calls grainBuffer.readAt(channel, position) — reads the now-updated buffer
```

This requires OpalEngine to expose its `AudioRingBuffer` by source slot:

```cpp
// OpalEngine.h
AudioRingBuffer* getGrainBuffer(int sourceSlot)
{
    if (sourceSlot < 0 || sourceSlot >= MaxSlots) return nullptr;
    return &coupledBuffers[static_cast<size_t>(sourceSlot)];
}

private:
    std::array<AudioRingBuffer, MaxSlots> coupledBuffers;
    // Each slot corresponds to a MegaCouplingMatrix source slot (0-3)
```

**Extended MegaCouplingMatrix::processBlock() for AudioToBuffer:**

```cpp
if (route.type == CouplingType::AudioToBuffer)
{
    // Direct stereo push — bypass mono scratch buffer
    auto* opalDest = dynamic_cast<OpalEngine*>(dest);
    if (!opalDest) continue;  // Safety: AudioToBuffer only valid for OpalEngine

    AudioRingBuffer* rb = opalDest->getGrainBuffer(route.sourceSlot);
    if (!rb) continue;

    // Build temp stereo arrays from per-sample coupling cache
    // (Uses the existing couplingBuffer for L and a second scratch buffer for R)
    for (int i = 0; i < limit; ++i)
        couplingBufferL[i] = source->getSampleForCoupling(0, i);
    for (int i = 0; i < limit; ++i)
        couplingBufferR[i] = source->getSampleForCoupling(1, i);

    rb->pushBlock(couplingBufferL.data(), couplingBufferR.data(), limit, route.amount);
    continue;  // Skip dest->applyCouplingInput() — grain buffer handles it directly
}
```

This requires a second scratch buffer `couplingBufferR` in `MegaCouplingMatrix`. Add alongside `couplingBuffer` in `prepare()`:

```cpp
// MegaCouplingMatrix.h
void prepare(int maxBlockSize)
{
    couplingBuffer.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    couplingBufferR.resize(static_cast<size_t>(maxBlockSize), 0.0f);  // ADD
}

private:
    std::vector<float> couplingBuffer;   // L or mono scratch
    std::vector<float> couplingBufferR;  // R scratch for AudioToBuffer
```

---

### 15.5 Feedback Loop Safety

A cycle OPAL→Engine→OPAL is conceivable: OPAL's granulated output feeds Engine X (via `AmpToFilter` or `getSampleForCoupling`), and Engine X also feeds OPAL (via `AudioToBuffer`). This is not catastrophically dangerous (no positive feedback on the coupling bus since each engine processes at most once per block), but it is musically confusing and creates a one-block latency circular dependency.

**Detection:** The `EngineRegistry` should detect cycles at route-add time (message thread) using a depth-first search over the route graph:

```cpp
// EngineRegistry.h — call this before adding a route
bool wouldCreateCycle(int sourceSlot, int destSlot,
                      const std::vector<CouplingRoute>& existingRoutes)
{
    // DFS from destSlot — can we reach sourceSlot?
    std::array<bool, MaxSlots> visited = {};
    std::function<bool(int)> dfs = [&](int node) -> bool {
        if (node == sourceSlot) return true;
        if (visited[node]) return false;
        visited[node] = true;
        for (const auto& r : existingRoutes)
            if (r.sourceSlot == node && dfs(r.destSlot))
                return true;
        return false;
    };
    return dfs(destSlot);
}
```

**Policy:** If `wouldCreateCycle()` returns true for an `AudioToBuffer` route, reject it and log a warning. For other coupling types, a cycle is less dangerous (scalar modulation doesn't accumulate exponentially), so it is permitted but warned.

**Runtime guard (audio thread):** Even if a cycle slips through (e.g., preset loaded with pre-existing cycle), OPAL's `coupledBuffers` contain audio from the *previous* block — the dependency is always one block old. No real-time deadlock is possible.

---

### 15.6 Buffer Sizing and Allocation Rules

| Decision | Rationale |
|---|---|
| Buffer size: 4 seconds | Long enough for meaningful freeze windows at any tempo. At 120 BPM, 4 seconds = 8 bars. |
| Allocated in `prepare()` | Never on audio thread. `OpalEngine::prepare()` calls `coupledBuffers[i].prepare(sampleRate, 4.0f)` for each slot. |
| Fixed capacity | No resize on sample-rate change — call `prepare()` again on SR change (same as all other engines). |
| Mono source handled | If source engine is mono (`getSampleForCoupling(1, i)` returns same as channel 0), the buffer stores identical L+R. No special case needed. |
| Stale buffers | If no `AudioToBuffer` push arrives for a slot in N blocks, the buffer fills with stale audio. The grain scheduler reads old material — this is musically valid (FREEZE-like). No zeroing required. |

**Memory budget addendum (updates Section 13):**
- Coupled grain buffers: 4 slots × 2 channels × 44100 × 4 × 4 bytes = **~2.75MB worst case** (all 4 slots in use)
- Typical case: 1-2 active sources = **~688KB–1.38MB**
- MegaCouplingMatrix second scratch buffer: 1024 samples × 4 bytes = **~4KB**

---

### 15.7 FREEZE Gesture Specification

FREEZE is OPAL's most expressive performance control. V008 specifies it as a **momentary hold** (not a toggle), with optional latch mode.

#### Momentary Mode (default)

```
Performer action:    Press                   Hold              Release
Buffer write head:   ──────────────────────  STOPPED ─────────  RESUMED
Grain reads from:    Normal sliding window   Frozen region      Normal again
opal_freeze value:   0.0          ramps to    1.0    ramps to    0.0
```

The write head deceleration is gradual (10ms ramp from the current `opal_freeze` value to 1.0 on press, reverse on release). This prevents grain read heads from suddenly racing ahead of the write position, which would cause buffer wrap artifacts.

#### Latch Mode (opal_freezeLatch = 1)

Double-tap the FREEZE button (or the Z-pressure trigger) within 300ms to latch. In latch mode:
- First tap: freeze engages (momentary hold)
- Second tap within 300ms: converts to latch (freeze stays on after release)
- Third tap while latched: releases latch, write head resumes

```cpp
// OpalEngine.h — FREEZE gesture state machine
enum class FreezeState { Off, MomentaryHold, Latched };

void handleFreezePress()
{
    const auto now = juce::Time::currentTimeMillis();
    if (freezeState == FreezeState::Latched) {
        freezeState = FreezeState::Off;  // unlatch
    } else if (now - lastFreezeReleaseTime < 300) {
        freezeState = FreezeState::Latched;  // double-tap → latch
    } else {
        freezeState = FreezeState::MomentaryHold;
    }
    // Set frozen flag on all active coupled AudioRingBuffers
    for (auto& rb : coupledBuffers)
        rb.frozen.store(freezeState != FreezeState::Off, std::memory_order_release);
}

void handleFreezeRelease()
{
    lastFreezeReleaseTime = juce::Time::currentTimeMillis();
    if (freezeState == FreezeState::MomentaryHold) {
        freezeState = FreezeState::Off;
        for (auto& rb : coupledBuffers)
            rb.frozen.store(false, std::memory_order_release);
    }
    // If Latched, do nothing — stays frozen
}
```

#### PlaySurface Mapping

| Surface | FREEZE Action |
|---|---|
| Z-axis pressure (Pad mode) | Momentary hold — pressure above 0.7 freezes, releasing unfreezes |
| Long-press Z | Converts to latch |
| Dedicated FREEZE button (UI) | Click = momentary, double-click = latch |

#### Coupled Source Behavior During FREEZE

When OPAL's write head is frozen, the *source engine keeps playing normally* — its audio continues to be pushed into the shadow write head but not the main buffer. On unfreeze, the main write head jumps to the shadow position, meaning OPAL immediately resumes reading current audio — no discontinuity.

This is the crucial design choice: FREEZE captures a window, not an infinite loop. The shadow buffer ensures OPAL rejoins the live audio stream seamlessly on release.

---

### 15.8 getSampleForCoupling — What OPAL Exports

OPAL's coupling output is its granulated audio, ready to feed downstream engines:

```cpp
// OpalEngine.h — per-sample coupling cache (written at end of each renderBlock)
std::array<std::vector<float>, 2> couplingOutputCache;  // [channel][sample]

float getSampleForCoupling(int channel, int sampleIndex) const override
{
    if (channel < 0 || channel > 1) return 0.0f;
    if (sampleIndex < 0 || sampleIndex >= static_cast<int>(couplingOutputCache[0].size()))
        return 0.0f;
    return couplingOutputCache[static_cast<size_t>(channel)][static_cast<size_t>(sampleIndex)];
}
```

The cache is written at the end of `renderBlock()` from the final stereo output (post-FX, post-finish). This is the same pattern used by all XOmnibus engines.

**Signal point:** Post-filter, post-character stages, post-FX chain, pre-level. Amplitude is normalized ±1.

**Downstream engines and their use of OPAL's output:**

| Downstream Engine | Coupling Type Received | What They Do With It |
|---|---|---|
| OVERDUB | `AudioToBuffer` or `AudioToFM` | Grain cloud enters dub echo/spring reverb chain |
| ODDOSCAR | `EnvToMorph` (uses cloud amplitude) | Cloud amplitude envelope drives wavetable morph position |
| OVERBITE | `AmpToFilter` | Cloud RMS drives bass filter cutoff — breathing bass |
| ORACLE | `AudioToBuffer` | Cloud input into stochastic synthesis engine |

---

### 15.9 Integration with MegaCouplingMatrix — Route Registration

To register an `AudioToBuffer` coupling route in the normalled defaults:

```cpp
// Example: OVERWORLD → OPAL as primary default route
matrix.addRoute({
    .sourceSlot = kOverworldSlot,
    .destSlot   = kOpalSlot,
    .type       = CouplingType::AudioToBuffer,
    .amount     = 0.0f,        // default off — user turns it up via opal_couplingLevel
    .isNormalled = true,
    .active     = true
});
```

The `amount` field scales how loudly the source audio is written into OPAL's grain buffer. At `amount = 0`, nothing is written. At `amount = 1.0`, source audio is written at full level. The `opal_couplingLevel` parameter provides additional per-preset control on top of the route amount.

**Route validation at load time:** When a preset is loaded with `AudioToBuffer` routes, `EngineRegistry::wouldCreateCycle()` is called for each route. Invalid routes are silently skipped — never block preset load.

---

### 15.10 Coupling Type Precedence for OPAL

When multiple `AudioToBuffer` sources are active simultaneously (e.g., OVERWORLD in slot 0 and ODDFELIX in slot 1 both routing to OPAL), each fills its own `coupledBuffers[slotN]` independently. OPAL's grain scheduler reads from the buffer with the highest `opal_couplingLevel` (or a mix, per future parameter expansion).

In Phase 2, only one external source is read at a time — determined by `opal_source == 5` and whichever slot has a non-zero `AudioToBuffer` route. Multi-source grain mixing is deferred to Phase 5.

---

## Phase Map

| Phase | Content | Gate |
|---|---|---|
| **Phase 1 (this doc)** | Architecture, parameter taxonomy, signal flow, `AudioToBuffer` design | All 86 params frozen, `AudioToBuffer` spec complete |
| **Phase 2** | Scaffold project, grain buffer, `AudioRingBuffer`, grain scheduler, built-in oscillator, `AudioToBuffer` coupling input | Audible grain cloud from oscillator + first coupled source |
| **Phase 3** | Filter, character stages, envelopes, LFOs, mod matrix | Full modulation working |
| **Phase 4** | FX chain (smear, reverb, delay, finish) | Cloud integrity through FX |
| **Phase 5** | Macros, coupling output, FREEZE gesture, multi-source grain mixing, integration adapter | Macros audible, FREEZE gesture confirmed, coupling routes verified |
| **Phase 6** | 100 presets in `.xometa`, DNA computed | All presets pass macro QA |
| **Phase 7** | UI, polish, QA | Definition of Done met |

---

*XO_OX Designs | Engine: OPAL | Accent: #A78BFA | Prefix: opal_ | 86 parameters frozen | AudioToBuffer: V008*
