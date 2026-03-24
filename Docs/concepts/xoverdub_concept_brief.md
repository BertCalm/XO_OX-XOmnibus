# XOverdub — Concept Brief (Retrospective)

*Retrospective Phase 0 | March 2026 | XO_OX Designs*
*Enriching the first generation with the aquatic mythology*

---

## Identity

**XO Name:** XOverdub
**Gallery Code:** OVERDUB
**Accent Color:** Olive `#6B7B3A`
**Parameter Prefix:** `dub_`
**Engine Dir:** `Source/Engines/Dub/`

**Thesis:** Dub synthesis with tape delay, spring reverb, and drive -- sound echoing through temperature layers where warm meets cold.

---

## Aquatic Identity

The thermocline. The invisible boundary where warm surface water meets cold deep water, typically between 200 and 1000 meters down. Sound behaves strangely at the thermocline -- it bends, refracts, bounces between density layers, degrades over distance. A whale call launched at the thermocline can travel thousands of miles because the sound gets trapped between temperature gradients, bouncing endlessly, losing high frequencies with each reflection, gaining a ghostly character that marine biologists call "shadow zones." That is exactly what XOverdub does to every note that passes through it.

The tape delay is sound traveling through water. The wow and flutter are currents -- a 0.3 Hz sine wave wow that pulls the timing like a slow swell, and a 45 Hz flutter that adds the micro-instability of turbulence. The bandpass feedback narrowing is the ocean itself filtering the signal with each bounce, eating the highs and lows until only the mid-range ghost remains. The spring reverb is the metallic resonance of a sunken ship's hull -- six allpass stages ringing like steel plates struck underwater, true stereo decorrelation creating the spatial disorientation of deep water where direction dissolves.

Dub music was always underwater music. The echo chamber, the spring tank, the tape hiss -- these are the sounds of pressure, distance, and degradation. Lee "Scratch" Perry and King Tubby were building thermoclines in their studios, bouncing sound between layers of analog equipment until the original signal became something ancient and oceanic. XOverdub makes the metaphor literal. The olive accent color is kelp at the thermocline depth -- dark green life thriving at the boundary between worlds.

---

## Polarity

**Position:** The Thermocline -- the boundary where warm surface meets cold deep
**feliX-Oscar balance:** 50/50 -- dead center, the meeting point

---

## DSP Architecture (As Built)

The DubEngine is an 8-voice synthesizer with a complete voice engine feeding into a send/return FX chain. The architecture mirrors the standalone XOverdub instrument: Voice Engine -> Send VCA -> Drive -> Tape Delay -> Spring Reverb -> Return -> Master.

**Signal flow per voice:**

```
PolyBLEP Oscillator (Sine | Triangle | Saw | Square/PWM)
  + Sub Oscillator (sine, -1 octave)
  + Noise Generator (xorshift32 PRNG)
    |
    v
Cytomic SVF Filter (LP | HP | BP)
  with Envelope mod + LFO mod + Coupling mod
    |
    v
ADSR Envelope (exponential decay/release)
  x Velocity
    |
    v
Voice Mix (mono -> stereo)
```

**Send/Return FX chain (block-processed, post-voice):**

```
Voice Sum x Send Level
    |
    v
Drive (tanh saturation, variable gain)
    |
    v
Tape Delay
  - Hermite cubic interpolation for sub-sample accuracy
  - Wow: 0.3 Hz sine modulates delay time (tape motor instability)
  - Flutter: 45 Hz LP-filtered noise (capstan jitter)
  - Feedback: bandpass filtered + tanh saturated (tape wear degradation)
  - 5-sample Haas offset between L/R for stereo width
  - Up to 2 seconds delay time
    |
    v
Spring Reverb
  - 6-stage allpass chain (true stereo: offset buffer lengths L vs R)
  - One-pole damping LP post-chain (metallic -> dark)
  - Denormal-protected feedback paths
    |
    v
Return Level -> Mix with Dry -> Soft Limiter (tanh) -> Output
```

**Voice features:**
- 4 PolyBLEP waveforms with PWM on square.
- Octave selector: -2 to +2 octaves.
- Fine tune: +/- 100 cents.
- Analog drift: LP-filtered noise producing +/-5 cents of vintage pitch instability.
- Pitch envelope: exponential decay for log drum sounds (kick drops, tom sweeps).
- LFO: sine wave, routable to pitch, filter cutoff, or amplitude.
- Glide (portamento): exponential frequency slew.
- Poly/Mono/Legato voice modes.

**Parameters (30 total):** `dub_oscWave`, `dub_oscOctave`, `dub_oscTune`, `dub_oscPwm`, `dub_subLevel`, `dub_noiseLevel`, `dub_drift`, `dub_level`, `dub_filterMode`, `dub_filterCutoff`, `dub_filterReso`, `dub_filterEnvAmt`, `dub_attack`, `dub_decay`, `dub_sustain`, `dub_release`, `dub_pitchEnvDepth`, `dub_pitchEnvDecay`, `dub_lfoRate`, `dub_lfoDepth`, `dub_lfoDest`, `dub_sendLevel`, `dub_returnLevel`, `dub_dryLevel`, `dub_driveAmount`, `dub_delayTime`, `dub_delayFeedback`, `dub_delayWear`, `dub_delayWow`, `dub_delayMix`, `dub_reverbSize`, `dub_reverbDamp`, `dub_reverbMix`, `dub_voiceMode`, `dub_glide`, `dub_polyphony`.

---

## Signature Sound

XOverdub sounds like memory. The tape delay with wow and flutter turns clean notes into ghostly echoes that drift in and out of tune, degrading with each repetition until they dissolve into warm noise. The spring reverb adds a metallic spatial quality that is instantly recognizable -- not a room, not a hall, but a resonant metal chamber vibrating sympathetically. The drive adds harmonic density that grows with the feedback, so echoes get warmer and more saturated as they recede. The combined effect is sound with history -- every note sounds like it has been bouncing through the ocean for a long time before it reached your ears. It is the only XOlokun engine where the effects are not decorative but structural. Without the delay and reverb, Overdub is a competent subtractive synth. With them, it is an instrument for making time audible.

---

## Coupling Thesis

The great receiver. Every engine sounds different through Overdub's echo chamber. feliX's transients become rhythmic delay patterns that self-generate. Oscar's pads become infinite, degrading sustains that evolve over tens of seconds. Onset's drums become the ghostly echo of percussion heard from underwater. The send/return architecture means the coupling happens naturally -- any signal routed through Overdub's delay and reverb gains history, space, and the distinctive character of tape degradation.

As a source, Overdub's tape-degraded output makes extraordinary coupling material. The wow and flutter introduce slow pitch drift that, when fed into another engine's pitch modulation, creates organic detuning. The bandpass-narrowed feedback tails are spectrally focused enough to drive formant filters meaningfully.

Overdub accepts AmpToFilter coupling (inverted envelope creates dub pump on the filter), LFOToPitch and AmpToPitch (external pitch modulation), and PitchToPitch. He is the water that other species swim through.

---

## Preset Affinity

| Foundation | Atmosphere | Entangled | Prism | Flux | Aether |
|-----------|-----------|-----------|-------|------|--------|
| High | High | High | Low | Medium | High |

---

*XO_OX Designs | XOverdub -- sound bending at the thermocline, every echo a mile deeper than the last*
