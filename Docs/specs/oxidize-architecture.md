# OXIDIZE — Architecture Specification

> Engine #77. Degradation as synthesis. The Second Law of Thermodynamics as a performance interface.

## Identity

| Field | Value |
|-------|-------|
| Engine ID | `Oxidize` |
| Display Name | XOxidize |
| Parameter Prefix | `oxidize_` |
| Plugin Code | `Oxdz` |
| Accent Color | Verdigris `#4A9E8E` |
| Max Voices | 8 |
| Voice Stealing | LRU oldest-note with 5ms crossfade |

## Signal Flow

```
Per-Voice:
  ┌──────────────────────────────────────────────────┐
  │              AGE ACCUMULATOR                      │
  │  noteAge += ageRate * (1/sr) * ageCurve(t)       │
  │  aftertouch resistance: noteAge -= at * fightBack │
  │  clamped to [0, preserveAmount]                   │
  │  reverseMode: noteAge = preserveAmount - noteAge  │
  └──────────────┬───────────────────────────────────┘
                 │ age (0.0 → 1.0)
                 ▼
  [Patina Osc] + [Basic Osc] ──→ [Corrosion Waveshaper]
  (noise src)    (saw/pulse)      (6 modes, drive ↑ w/ age)
                                         │
                                         ▼
                                  [Erosion Filter]
                                  (CytomicSVF, 3 modes)
                                  (cutoff ↓ with age)
                                         │
                                         ▼
                                  [Entropy Quantizer]
                                  (dual: digital + analog)
                                  (bits/rate ↓ with age)
                                         │
                                         ▼
                                  [Wobble Processor]
                                  (wow + flutter LFOs)
                                  (depth ↑ with age)
                                         │
                                         ▼
                                  [Dropout Gate]
                                  (probability ↑ with age)
                                  (smear: soft tape vs hard click)
                                         │
                                         ▼
                                  [VCA Envelope]
                                         │
                                         ▼
                                  outputCacheL/R (coupling)

Shared (all voices):
  [Sediment Reverb FDN]
  (accumulates across notes)
  (multi-tap: pre-erosion, post-entropy, post-dropout)
  (T60 floor: 300s, not infinite)
```

## Voice Architecture

```cpp
struct OxidizeVoice {
    // Identity
    int note = -1;
    float velocity = 0.0f;
    bool active = false;
    uint64_t startTime = 0;  // LRU stealing

    // Age State (per-voice, independent — Schulze mandate)
    float noteAge = 0.0f;           // 0.0 = pristine, 1.0 = fully oxidized
    float snapshotAge = 0.0f;       // preset-stored initial age (Smith)
    bool reverseMode = false;       // Buchla: start destroyed, age to pristine

    // Oscillators
    float oscPhase = 0.0f;          // basic oscillator phase
    float patinaPhase = 0.0f;       // noise oscillator phase
    uint32_t noiseState = 0;        // PRNG state for crackle

    // Corrosion (waveshaper)
    // No state needed — pure function of input + age

    // Erosion Filter
    CytomicSVF erosionFilter;
    int erosionMode = 0;            // 0=Vinyl, 1=Tape, 2=Failure (Moog)

    // Entropy
    float analogNoiseFloor = 0.0f;  // analog entropy accumulator
    // Digital entropy: sample-and-hold at reduced rate
    float digitalHoldL = 0.0f;
    float digitalHoldR = 0.0f;
    int digitalCounter = 0;

    // Wobble
    StandardLFO wowLFO;
    StandardLFO flutterLFO;

    // Dropout
    float dropoutEnv = 1.0f;        // 1.0 = playing, 0.0 = silent
    uint32_t dropoutPRNG = 0;       // PRNG for dropout timing

    // Envelope
    StandardADSR ampEnvelope;

    // Coupling output cache
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;
};
```

### Age Accumulator (Per-Voice, Block-Rate)

Updated once per block (128 samples), not per-sample. This is the core mechanic:

```cpp
// Block-rate age update (Guru Bin CPU optimization)
void updateAge(OxidizeVoice& v, const OxidizeSnapshot& snap, int blockSize) {
    float dt = static_cast<float>(blockSize) / sampleRate_;

    // Base aging rate (seconds to reach full oxidation)
    // ageRate 0.0 = frozen, 1.0 = 30s to full age, higher = faster
    float rate = snap.ageRate;

    // Aftertouch fights entropy (Vangelis: performer resists degradation)
    float fightBack = snap.aftertouch * snap.velSens * 0.8f;
    rate = std::max(0.0f, rate - fightBack);

    // Coupling-driven aging (louder coupling = faster age)
    rate += couplingAgeBoost_;

    // Apply age curve (concave/linear/convex aging trajectory)
    float curvedDt = dt * rate * ageCurveFunction(v.noteAge, snap.ageCurve);

    if (v.reverseMode)
        v.noteAge = std::max(0.0f, v.noteAge - curvedDt);
    else
        v.noteAge = std::min(v.noteAge + curvedDt, snap.preserveAmount);
}
```

### Age → Parameter Derivation (Block-Rate Lookup Tables)

Each degradation stage derives its intensity from `noteAge` via pre-computed lookup tables (62KB shared):

```cpp
// Called once per block — derives all age-dependent values
struct AgeDerivedState {
    float corrosionDrive;      // waveshaper intensity
    float erosionCutoff;       // filter cutoff Hz
    float erosionRes;          // filter resonance
    float entropyBits;         // effective bit depth
    float entropyRate;         // effective sample rate
    float analogNoiseLevel;    // noise floor rise
    float wobbleDepth;         // wow+flutter intensity
    float dropoutProbability;  // per-block dropout chance
    float patinaLevel;         // noise oscillator mix
    float sedimentSend;        // reverb send amount
};

AgeDerivedState deriveFromAge(float age, const OxidizeSnapshot& snap) {
    AgeDerivedState s;
    float a = age / snap.preserveAmount;  // normalize to 0-1

    s.corrosionDrive   = snap.corrosionDepth * ageLUT_corrosion[int(a * 1023)];
    s.erosionCutoff    = lerp(20000.0f, snap.erosionFloor, ageLUT_erosion[int(a * 1023)]);
    s.erosionRes       = snap.erosionRes * a;
    s.entropyBits      = lerp(16.0f, 2.0f, a * snap.entropyDepth);
    s.entropyRate      = lerp(sampleRate_, 4000.0f, a * snap.entropyDepth);
    s.analogNoiseLevel = a * snap.entropyDepth * 0.3f;
    s.wobbleDepth      = a * snap.wowDepth;
    s.dropoutProbability = a * a * snap.dropoutRate;  // quadratic: rare early, frequent late
    s.patinaLevel      = a * snap.patinaDensity;
    s.sedimentSend     = snap.sedimentMix * (0.3f + 0.7f * a);
    return s;
}
```

## Parameter Table (45 parameters)

### Oscillator (6)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_waveform` | Waveform | 0-4 | 0 (Saw) | Saw/Pulse/Triangle/Noise/Hybrid |
| `oxidize_tune` | Tune | -24..+24 | 0 | Semitone tuning |
| `oxidize_fine` | Fine | -100..+100 | 0 | Cent detuning |
| `oxidize_patinaDensity` | Patina Density | 0-1 | 0.15 | Noise oscillator level (age-modulated) |
| `oxidize_patinaTone` | Patina Tone | 0-1 | 0.6 | Noise spectrum tilt (low=rumble, high=crackle) |
| `oxidize_oscMix` | Osc Mix | 0-1 | 0.8 | Basic osc vs patina osc balance |

### Aging (6)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_ageRate` | Age Rate | 0-1 | 0.3 | Speed of aging (0=frozen, 1=30s to full) |
| `oxidize_ageOffset` | Age Offset | 0-1 | 0.0 | Starting age position (velocity-mappable) |
| `oxidize_ageCurve` | Age Curve | -1..+1 | 0 | Aging trajectory (-1=log, 0=linear, +1=exp) |
| `oxidize_ageVelSens` | Age Vel Sens | 0-1 | 0.5 | Velocity → initial age offset depth |
| `oxidize_preserveAmount` | Preserve | 0.1-1 | 0.85 | Passivation ceiling (max degradation) |
| `oxidize_reverseAge` | Reverse Age | 0/1 | 0 (Off) | Buchla mode: start destroyed, age to pristine |

### Corrosion (4)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_corrosionDepth` | Corrosion | 0-1 | 0.35 | Waveshaper intensity (age-modulated) |
| `oxidize_corrosionMode` | Mode | 0-5 | 0 | Valve/Transformer/BrokenSpeaker/TapeSat/Rust/Acid |
| `oxidize_corrosionTone` | Tone | 0-1 | 0.5 | Post-saturation tilt EQ |
| `oxidize_corrosionVariance` | Variance | 0-1 | 0.15 | Randomization depth |

### Erosion (5)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_erosionRate` | Erosion Rate | 0-1 | 0.4 | How fast the filter closes with age |
| `oxidize_erosionFloor` | Floor | 20-5000 | 200 | Minimum cutoff Hz at full age |
| `oxidize_erosionRes` | Resonance | 0-1 | 0.2 | Resonant peak at eroding cutoff |
| `oxidize_erosionMode` | Filter Mode | 0-2 | 0 | Vinyl(smooth)/Tape(mid-scoop)/Failure(notchy) |
| `oxidize_erosionVariance` | Variance | 0-1 | 0.1 | Randomization depth |

### Entropy (5)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_entropyDepth` | Entropy | 0-1 | 0.25 | Overall degradation depth (age-modulated) |
| `oxidize_entropySmooth` | Smooth | 0-1 | 0.6 | Anti-aliasing filter character |
| `oxidize_entropyMode` | Mode | 0-2 | 0 | Digital(bitcrush)/Analog(noise rise)/Both |
| `oxidize_entropyBias` | Bias | 0-1 | 0.5 | Digital vs. analog balance (when mode=Both) |
| `oxidize_entropyVariance` | Variance | 0-1 | 0.1 | Randomization depth |

### Wobble (5)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_wowDepth` | Wow | 0-1 | 0.2 | Slow pitch drift depth (age-modulated) |
| `oxidize_wowRate` | Wow Rate | 0.01-4 | 0.5 | Wow LFO rate Hz |
| `oxidize_flutterDepth` | Flutter | 0-1 | 0.15 | Fast pitch chatter depth (age-modulated) |
| `oxidize_flutterRate` | Flutter Rate | 6-20 | 12 | Flutter LFO rate Hz |
| `oxidize_wobbleSpread` | Stereo | 0-1 | 0.3 | L/R wobble independence |

### Dropout (4)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_dropoutRate` | Dropout Rate | 0-1 | 0.1 | Probability per block (age-modulated, quadratic) |
| `oxidize_dropoutDepth` | Depth | 0-1 | 0.8 | How much signal is cut (1.0 = full silence) |
| `oxidize_dropoutSmear` | Smear | 0-1 | 0.5 | Envelope shape (0=hard click, 1=soft tape lift) |
| `oxidize_dropoutVariance` | Variance | 0-1 | 0.2 | Timing randomization |

### Sediment Reverb (3)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_sedimentTail` | Tail | 0-1 | 0.5 | Decay time (T60 floor: 300s at max) |
| `oxidize_sedimentTone` | Tone | 0-1 | 0.4 | Reverb spectrum tilt |
| `oxidize_sedimentMix` | Mix | 0-1 | 0.25 | Dry/wet (age-modulated: more wet with age) |

### Modulation (4)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_lfo1Rate` | LFO1 Rate | 0.005-20 | 0.3 | General purpose LFO (D005 floor: 0.005 Hz) |
| `oxidize_lfo1Depth` | LFO1 Depth | 0-1 | 0.2 | LFO1 modulation amount |
| `oxidize_lfo2Rate` | LFO2 Rate | 0.005-20 | 1.5 | Second LFO |
| `oxidize_lfo2Depth` | LFO2 Depth | 0-1 | 0.1 | LFO2 modulation amount |

### Expression (4)

| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| `oxidize_velSens` | Velocity Sens | 0-1 | 0.6 | Velocity → amplitude + age offset |
| `oxidize_aftertouch` | Aftertouch | 0-1 | 0.5 | Aftertouch → fight entropy (Vangelis) |
| `oxidize_modWheel` | Mod Wheel | 0-1 | 0.5 | Mod wheel → age rate |
| `oxidize_pitchBend` | Pitch Bend | 0-12 | 2 | Pitch bend range in semitones |

### Macros (4)

| ID | Name | Controls |
|----|------|----------|
| `oxidize_macroPATINA` | M1 CHARACTER | patinaDensity, corrosionDepth, corrosionTone |
| `oxidize_macroAGE` | M2 MOVEMENT | ageRate, wowDepth, flutterDepth |
| `oxidize_macroENTROPY` | M3 COUPLING | entropyDepth, erosionRate, dropoutRate |
| `oxidize_macroSEDIMENT` | M4 SPACE | sedimentMix, sedimentTail, dropoutRate |

**Total: 46 parameters**

## Corrosion Waveshaper Modes

```cpp
enum CorrosionMode {
    Valve = 0,          // tanh(x * drive) — smooth tube saturation
    Transformer,        // x / (1 + |x * drive|) — low-mid thickening
    BrokenSpeaker,      // asymmetric clip + noise injection
    TapeSat,            // tanh with 3rd harmonic emphasis
    Rust,               // asymmetric: positive clips harder with age
    Acid                // wavefolder: sin(x * drive * pi) — metallic
};
```

## Erosion Filter Modes (Moog Enhancement)

| Mode | Character | Implementation |
|------|-----------|---------------|
| Vinyl | Smooth, gentle LP roll-off | CytomicSVF LowPass, res 0.1-0.3 |
| Tape | Mid-scoop, nasal resonance | CytomicSVF BandPass → subtract from dry | 
| Failure | Notchy, unstable collapse | CytomicSVF Notch, resonance sweeps randomly |

## Coupling Contract

### As Coupling Source
```cpp
float getSampleForCoupling(int channel, int sampleIndex) override {
    return (channel == 0) ? outputCacheL[sampleIndex] : outputCacheR[sampleIndex];
}
```

### As Coupling Destination
```cpp
void applyCouplingInput(CouplingType type, float amount,
                         const float* sourceBuffer, int numSamples) override {
    switch (type) {
        case CouplingType::AmplitudeModulation:
            // Coupling RMS drives ageRate boost
            couplingAgeBoost_ = computeRMS(sourceBuffer, numSamples) * amount;
            break;
        case CouplingType::FrequencyModulation:
            // Coupling pitch drives wobble rate modulation
            couplingWobbleMod_ = sourceBuffer[numSamples/2] * amount;
            break;
        case CouplingType::SpectralShaping:
            // Coupling spectrum shapes erosion filter
            couplingFilterMod_ = computeSpectralCentroid(sourceBuffer, numSamples) * amount;
            break;
        case CouplingType::AudioToRing:
            // Coupling audio ring-modulates the corroded signal
            std::copy(sourceBuffer, sourceBuffer + numSamples, couplingRingBuffer_.data());
            couplingRingAmount_ = amount;
            break;
        default:
            break;
    }
}
```

## Sediment Reverb (Shared FDN)

One FDN shared across all voices (Guru Bin: architecturally correct + CPU efficient):

```
4-delay-line FDN with Hadamard feedback matrix
Delay times: 1117, 1571, 1949, 2311 samples (coprime, tuned to avoid metallic modes)
Per-delay LP filter (sedimentTone controls cutoff)
Input: weighted sum from all voices (3 tap points per voice)
T60 range: 2s (sedimentTail=0) to 300s (sedimentTail=1)
Output mixed to stereo via decorrelation matrix
```

Multi-tap input points (Tomita enhancement):
1. Post-corrosion (sends early character into the field)
2. Post-entropy (sends degraded texture)
3. Post-dropout (sends dropout silence gaps as rhythmic imprint)

## CPU Budget

| Component | Rate | Cost Estimate |
|-----------|------|---------------|
| Age accumulator + derivation | Block (128 samples) | Negligible |
| Patina noise oscillator | Sample | Low (PRNG + filter) |
| Basic oscillator | Sample | Low (phase accumulator) |
| Corrosion waveshaper | Sample | Low (lookup table) |
| Erosion filter (CytomicSVF) | Sample | Medium |
| Entropy quantizer | Sample/Reduced | Low (sample-and-hold) |
| Wobble (2 LFOs + pitch mod) | Sample | Low |
| Dropout gate | Block | Negligible |
| VCA envelope | Sample | Low |
| Sediment FDN (shared) | Sample | Medium (1 instance) |

**Estimated: 8 voices at ~8-10% CPU (Release, arm64)**

## File Structure

```
Source/Engines/Oxidize/
├── OxidizeEngine.h          # Full engine implementation (all DSP inline)
├── OxidizeEngine.cpp         # 3-line stub
├── OxidizeParamSnapshot.h    # Parameter cache struct + attachTo/updateFrom
├── OxidizeVoice.h            # Voice struct + per-voice processing
├── OxidizeSediment.h         # Shared FDN reverb
├── OxidizeCorrosion.h        # 6 waveshaper mode implementations
├── OxidizeLookupTables.h     # Pre-computed age→parameter curves (62KB)
└── OxidizeAdapter.h          # SynthEngine adapter (thin wrapper)
```

## Mythology Assignment (Pending)

- **Water Column Position:** Mid-depth — the twilight zone where materials corrode
- **Creature:** TBD (invoke `/mythology-keeper` in Phase 2)
- **feliX-Oscar Polarity:** Oscar-dominant (transformation, decay, entropy)

## fXOxide Phase 2 Plan

After engine ships, extract degradation stages into Singularity FX:
- Replace note-age accumulator with **envelope follower** (quiet = faster aging)
- Signal chain: Input → Patina → Corrosion → Erosion → Entropy → Wobble → Dropout → Sediment → Output
- Single `fxo_magnitude` macro scales all stages (RC-20's MAGNITUDE concept)
- Estimated: `Source/DSP/Effects/fXOxide.h` (~400 lines)
