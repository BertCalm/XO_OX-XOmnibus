# XOto Engine Architecture

## Identity
- **Name**: XOto (音 = "sound" in Japanese)
- **Chef**: Oto -- disciplined minimalism, elemental, sound itself
- **Collection**: Chef Quad (Kitchen Essentials), Engine #1
- **Region**: East Asia
- **Prefix**: `oto_`
- **Accent**: Bamboo Green `#7BA05B`

## DSP Architecture

### Synthesis Core: Multi-Model Additive Organ
The engine hosts 4 switchable organ models, each implemented as additive synthesis
with model-specific partial ratio tables and amplitude curves:

| Model | Partials | Technique | Character |
|-------|----------|-----------|-----------|
| 0: Sho | 11 (aitake cluster) | Non-harmonic cluster ratios | Ethereal, sustained |
| 1: Sheng | 11 (harmonic series) | Integer-ratio harmonics | Bright, melodic |
| 2: Khene | 11 (paired pipes) | Deliberately detuned pairs | Raw, buzzy, folk |
| 3: Melodica | 8 (saw + harmonics) | Sawtooth fundamental + falloff | Warm, lo-fi, human |

### Signal Flow (per voice, per sample)
```
Glide -> Base Frequency
  |-> Pitch Bend + Coupling Pitch Mod
  |-> Breath Pressure Instability (drift LFO + tremolo)
  |-> LFO1 Vibrato
  |
  v
Additive Partial Bank (1-11 partials, controlled by Cluster Density)
  |-> Per-partial phase accumulation
  |-> Model-specific waveform (sine or saw for melodica fundamental)
  |-> Amplitude scaling from model tables
  |-> Unison detune (per-partial frequency offset)
  |
  v
Buzz Waveshaping (fastTanh, model-scaled intensity)
  |
  v
+ Chiff Transient (Hann-windowed noise/pitched burst)
  |
  v
+ Crosstalk (one-block-delayed adjacent voice leakage)
  |
  v
Amp Envelope (FilterEnvelope ADSR, model-specific attack shaping)
  x Breath Amplitude Modulation
  x Competition Scaling (adversarial coupling)
  |
  v
CytomicSVF LowPass (velocity->cutoff D001, filter env, LFO1 modulation)
  |
  v
Stereo Pan (cached at note-on, voice-index spread)
  |
  v
Mix Bus -> Coupling Cache -> Silence Gate
```

### Key DSP Components
- **OtoChiffGenerator**: Brief harmonic burst on note-on. Duration, noise mix, and spectral
  content vary per organ model (Sho=40ms breathy, Sheng=15ms sharp, Khene=25ms buzzy,
  Melodica=35ms airy).
- **OtoBreathSource**: Two internal LFOs (0.3 Hz drift + 5.5 Hz tremolo) scaled by the
  Pressure parameter. Produces per-voice pitch drift (up to +/-8 cents) and amplitude
  modulation (up to +/-15%).
- **Cluster Density**: Controls how many of the model's partials are active (1 to N).
  The last partial crossfades smoothly based on fractional density to prevent clicking.
- **Competition**: Adversarial coupling -- when multiple voices sound simultaneously,
  each voice's amplitude is reduced proportionally. Models the shared-breath physics
  of free-reed instruments where air pressure distributes across open reeds.

### Shared DSP Utilities Used
- `StandardLFO` -- voice LFO1 (vibrato) + breath source internal LFOs
- `FilterEnvelope` -- amp ADSR + filter ADSR (both per voice)
- `CytomicSVF` -- per-voice lowpass filter
- `GlideProcessor` -- frequency glide (snap-only for poly voices)
- `VoiceAllocator` -- LRU 8-voice stealing
- `PitchBendUtil` -- MIDI pitch wheel processing
- `ParameterSmoother` -- 8 smoothed parameters (5ms default, 10ms for filter cutoff)

## Doctrine Compliance

| Doctrine | Implementation |
|----------|---------------|
| D001 Velocity -> Timbre | velocity * 4000 Hz filter cutoff boost + velocity-scaled filter envelope |
| D002 Modulation | LFO1 (pitch vibrato + filter mod), 4 working macros, breath source, aftertouch, mod wheel |
| D004 No Dead Params | All 20 parameters wired to DSP |
| D005 Breathing | LFO1 rate floor 0.01 Hz, breath source drift at 0.3 Hz |
| D006 Expression | Mod wheel -> filter cutoff, Aftertouch -> pressure + filter, Velocity -> filter brightness |

## Parameters (20 total)

| # | ID | Type | Default | Role |
|---|-----|------|---------|------|
| 1 | oto_organ | int 0-3 | 0 | Organ model select |
| 2 | oto_cluster | float 0-1 | 0.7 | Active partial count |
| 3 | oto_chiff | float 0-1 | 0.3 | Transient burst amount |
| 4 | oto_detune | float 0-1 | 0.15 | Partial beating |
| 5 | oto_buzz | float 0-1 | 0.0 | Reed waveshaping |
| 6 | oto_pressure | float 0-1 | 0.2 | Breath instability |
| 7 | oto_crosstalk | float 0-1 | 0.0 | Adjacent voice leakage |
| 8 | oto_filterCutoff | float 20-20k | 8000 | Main filter |
| 9 | oto_filterRes | float 0-1 | 0.1 | Filter Q |
| 10 | oto_attack | float 0.001-2 | 0.08 | Amp env attack |
| 11 | oto_decay | float 0.01-5 | 0.5 | Amp env decay |
| 12 | oto_sustain | float 0-1 | 0.8 | Amp env sustain |
| 13 | oto_release | float 0.01-8 | 0.4 | Amp env release |
| 14 | oto_lfo1Rate | float 0.01-20 | 0.3 | LFO rate |
| 15 | oto_lfo1Depth | float 0-1 | 0.0 | LFO depth |
| 16 | oto_competition | float 0-1 | 0.0 | Adversarial coupling |
| 17 | oto_macroA | float 0-1 | 0.0 | CHARACTER: cluster+chiff+buzz |
| 18 | oto_macroB | float 0-1 | 0.0 | MOVEMENT: LFO depth+pressure |
| 19 | oto_macroC | float 0-1 | 0.0 | COUPLING: competition+crosstalk |
| 20 | oto_macroD | float 0-1 | 0.0 | SPACE: filter open |

## Presets (10 initial)

| Preset | Mood | Model | Character |
|--------|------|-------|-----------|
| Oto Sho Aitake | Foundation | Sho | Traditional gagaku cluster |
| Oto Sheng Jade | Atmosphere | Sheng | Bright melodic clarity |
| Oto Khene Folk | Organic | Khene | Raw buzzy Isan folk |
| Oto Melodica Pablo | Foundation | Melodica | Warm dub melodica |
| Oto Celestial Reeds | Ethereal | Sho | Maximum cluster pad |
| Oto Buzz Ritual | Flux | Khene | Aggressive buzzy drone |
| Oto Sheng Silk | Luminous | Sheng | Crystalline harmonics |
| Oto Bamboo Depths | Deep | Sho | Dark subterranean drone |
| Oto Melodica Stab | Kinetic | Melodica | Percussive staccato stabs |
| Oto Four Winds | Prism | Sheng | Full macro expression |

## Integration Notes
- Engine ID: `"Oto"` (used in preset `"engines"` array and `"parameters"` keys)
- Parameter prefix: `oto_` (frozen, never changes)
- Register in `EngineRegistry.h` and `PresetManager.h`
- Coupling types supported: AmpToFilter, LFOToPitch, AmpToPitch, EnvToMorph
- Silence gate hold: 500ms (sustained organ tails)
