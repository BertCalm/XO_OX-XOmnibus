# XOctave Engine Architecture

**Engine #45** | Chef Quad member #2 | Accent: Bordeaux `#6B2D3E` | Prefix: `oct_`

## Identity

XOctave is the Chef -- classically trained, structurally precise. Named after both
the French given name and the musical interval. Where Oto (the Sommelier) improvises,
Octave knows exactly where the resolution lies.

## The 4 Organ Models

Switchable via `oct_organ` (0-3). Each model uses fundamentally different DSP:

### Model 0: Cavaille-Coll Romantic Pipe Organ
- **DSP**: Additive synthesis with 12 drawbar partials (harmonic series 1-12)
- **Partial amplitudes**: From Audsley (1905) pipe organ analysis -- strong fundamental,
  rolling off through upper harmonics
- **Registration**: Blends 8' / 4' / 2' ranks via `oct_registration` parameter
- **Wind noise**: Continuous broadband noise shaped by low-pass filter
- **Chiff**: Subtle bloom (30% of chiff parameter)
- **Room resonance**: 3-mode bandpass (120/380/1200 Hz) simulating cathedral stone
- **Character**: Dark, sustained, symphonic, MASSIVE
- **Attack**: 3x multiplier on base attack (minimum 50ms) -- fills the space slowly

### Model 1: Baroque Positiv Organ
- **DSP**: Additive with brighter partial weighting (2nd harmonic dominant)
- **Partial amplitudes**: From Schnitger organ analysis -- transparent, principal-dominant
- **Registration**: Principal (8') and Flute (4') blend
- **Chiff**: PROMINENT -- the defining feature. Full chiff parameter weight (1.0x).
  OctaveChiffGenerator produces a noise burst shaped by pipe resonance frequency
- **Room resonance**: 50% depth (smaller Positiv case)
- **Character**: Articulate, transparent, historical, precise
- **Attack**: 1.5x multiplier (minimum 5ms)

### Model 2: French Musette Accordion
- **DSP**: 3 detuned oscillators per voice with odd-harmonic reed timbre
  (sin + sin*3 * 0.33 + sin*5 * 0.15 + sin*7 * 0.08)
- **Beating**: `oct_detune` controls beating rate (1-9 Hz between reeds)
- **Bellows dynamics**: velocity * 0.6 + pressure * 0.4 = bellows amplitude
- **Buzz**: Reed buzz/rattle at high pressure via tanh saturation
- **No room resonance**: Accordion is a close-mic instrument
- **Character**: Vibrant, warm, romantic, Paris streets

### Model 3: Farfisa Compact Organ
- **DSP**: PolyBLEP bandlimited square wave
- **Vibrato**: Fixed 5.5 Hz (original transistor circuit), depth from `oct_detune`
- **Registration**: Mixes in octave-up square for timbral brightness
- **Buzz**: Transistor saturation via tanh with adjustable drive (1-7x)
- **No room resonance**: Dry, direct, lo-fi
- **Character**: Buzzy, raw, immediate, punk-elegant
- **Attack**: 0.1x multiplier (minimum 1ms) -- IMMEDIATE response

## Shared Chef Parameters (6)

Same vocabulary as XOto, but weighted differently per model:

| Parameter | Cavaille-Coll | Baroque | Musette | Farfisa |
|-----------|--------------|---------|---------|---------|
| `oct_cluster` | Ensemble detuning | Ensemble detuning | Ensemble detuning | Ensemble detuning |
| `oct_chiff` | Subtle bloom (30%) | Full transient (100%) | No effect | No effect |
| `oct_detune` | Minimal | Minimal | Reed beating Hz | Vibrato depth |
| `oct_buzz` | No effect | No effect | Reed rattle | Transistor overdrive |
| `oct_pressure` | Wind pressure (amp) | Wind pressure (amp) | Bellows pressure | Amplitude |
| `oct_crosstalk` | Key bleed (SVF) | Key bleed (SVF) | No effect | No effect |

## Parameter Count: 27

- 1 organ model selector
- 6 shared Chef params
- 3 tone shaping (brightness, registration, room depth)
- 4 ADSR envelope
- 1 filter envelope amount
- 1 pitch bend range
- 4 macros (CHARACTER, MOVEMENT, COUPLING, SPACE)
- 6 LFO params (2 LFOs x rate/depth/shape)
- 1 competition param

## Doctrine Compliance

- **D001**: Velocity shapes upper partial brightness (additive models) and bellows
  dynamics (Musette). Filter envelope amount scales with velocity.
- **D002**: 2 LFOs (LFO1 -> brightness, LFO2 -> vibrato/tremulant), aftertouch,
  mod wheel, 4 macros. All parameters wired.
- **D003**: Acoustic references cited (Audsley 1905, Jaffe & Smith 1983,
  Millot et al. 2001, Nolle 1979). Chiff based on pipe lip physics.
- **D004**: All 27 parameters affect audio output.
- **D005**: LFO1 rate floor 0.005 Hz (200-second cycle). Breathing is always present.
- **D006**: Aftertouch -> pressure, Mod wheel -> registration blend, Velocity -> timbre.

## Shared DSP Utilities Used

- `StandardLFO` -- 2 per voice (brightness + vibrato)
- `StandardADSR` -- amplitude envelope
- `FilterEnvelope` -- filter modulation
- `CytomicSVF` -- output filter + room resonance modes
- `PolyBLEP` -- Farfisa square wave oscillator
- `GlideProcessor` -- portamento
- `ParameterSmoother` -- 8 smoothed parameters
- `VoiceAllocator` -- LRU voice stealing
- `PitchBendUtil` -- pitch wheel + semitone conversion
- `SilenceGate` -- zero-idle bypass (500ms hold)

## Initial Presets (10)

| Name | Model | Mood | Character |
|------|-------|------|-----------|
| Bordeaux Cathedral | Cavaille-Coll | Foundation | Full dark pipe organ |
| Tracker Action | Baroque | Foundation | Articulate with chiff |
| Rue Mouffetard | Musette | Foundation | Gentle Paris accordion |
| Garage Organ | Farfisa | Foundation | Raw garage rock |
| Stone Breath | Cavaille-Coll | Atmosphere | Dark ambient cathedral |
| Schnitger Glass | Baroque | Prism | Crystalline bright organ |
| Bellows Fury | Musette | Flux | Aggressive beating reeds |
| Transistor Fuzz | Farfisa | Flux | Heavy overdrive organ |
| Nave Infinity | Cavaille-Coll | Aether | Clustered infinite reverb |
| Valse Brillante | Musette | Prism | Elegant waltz accordion |
