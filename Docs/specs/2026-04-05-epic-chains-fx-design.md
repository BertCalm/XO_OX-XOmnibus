# Epic Chains — FX Chain Engines & Unified FX Slot System

**Date:** 2026-04-05
**Author:** XO_OX Designs
**Status:** Design Spec — Ready for Implementation

---

## Table of Contents

1. [Overview](#1-overview)
2. [The Four Epic Chain Engines](#2-the-four-epic-chain-engines)
   - 2.1 [ONRUSH — Expressive Lead](#21-onrush--expressive-lead)
   - 2.2 [OMNISTEREO — Stereo Widener](#22-omnistereo--stereo-widener)
   - 2.3 [OBLITERATE — Heavy Stutter](#23-obliterate--heavy-stutter)
   - 2.4 [OBSCURITY — Dark Ambient](#24-obscurity--dark-ambient)
3. [Unified FX Slot System](#3-unified-fx-slot-system)
   - 3.1 [Architecture Overview](#31-architecture-overview)
   - 3.2 [EpicChainSlotController Class Design](#32-epicchainslotcontroller-class-design)
   - 3.3 [Signal Flow and processBlock Logic](#33-signal-flow-and-processblock-logic)
   - 3.4 [Parameter ID Convention](#34-parameter-id-convention)
   - 3.5 [Parameter Budget](#35-parameter-budget)
4. [DSP Conventions](#4-dsp-conventions)
   - 4.1 [Native Primitives (No juce::dsp)](#41-native-primitives-no-jucedsp)
   - 4.2 [Phantom Sniff Corrections](#42-phantom-sniff-corrections)
5. [Preset Migration](#5-preset-migration)
   - 5.1 [Legacy Detection](#51-legacy-detection)
   - 5.2 [Migration Map](#52-migration-map)
   - 5.3 [Edge Cases](#53-edge-cases)
6. [Accent Colors](#6-accent-colors)
7. [Files to Create and Modify](#7-files-to-create-and-modify)
   - 7.1 [New Files](#71-new-files)
   - 7.2 [Modified Files](#72-modified-files)
8. [Future Extensibility](#8-future-extensibility)
9. [CLAUDE.md Update Checklist](#9-claudemd-update-checklist)
10. [Architect Review Resolutions](#10-architect-review-resolutions)

---

## 1. Overview

Epic Chains introduces four new FX chain engines adapted from standalone JUCE plugin concepts (the Gemini Pedalboard Series) into XOceanus FX engines, alongside a unified 3-slot FX assignment system that replaces the existing fixed FX chain positions in `MasterFXChain`.

**Goals:**
- Expand the FX palette with four deeply characterful effect chains: expressive lead processing (ONRUSH), stereo widening and dimension (OMNISTEREO), heavy stutter and destruction (OBLITERATE), and dark ambient texture (OBSCURITY).
- Unify all FX chains — existing (Aquatic, Math, Boutique) and new (Epic Chains) plus the three Singularity FX (fXOnslaught, fXObscura, fXOratory) — into a single assignable 3-slot controller, giving performers maximum creative routing flexibility.
- Maintain full backward compatibility via a `fxVersion` preset migration system.

**Source concepts (Gemini Pedalboard Series):**

| XOceanus Engine | Source Concept |
|----------------|---------------|
| ONRUSH | PolySwell |
| OMNISTEREO | ChromaSpace |
| OBLITERATE | GritGlitch |
| OBSCURITY | VoidAtmosphere |

---

## 2. The Four Epic Chain Engines

### 2.1 ONRUSH — Expressive Lead

**Source concept:** PolySwell (Gemini Pedalboard Series)
**Routing:** Mono In → Stereo Out
**Accent Color:** Molten Amber `#FF6F00`
**Parameter prefix:** `onr_`

#### Signal Chain (5 Stages)

```
Mono Input
   │
   ▼
[Stage 1] Auto-Swell (Boss SG-1)
   │
   ▼
[Stage 2] Ring Modulator (Fairfield Randy's Revenge)
   │
   ▼
[Stage 3] Hard Clip Distortion (Boss DS-1 Golden)
   │
   ▼
[Stage 4] Envelope Filter (Boss TW-1 T Wah)
   │
   ▼
[Stage 5] Pitch-Sequenced BBD Delay (Chase Bliss Thermae)
   │
   ▼
Stereo Output
```

#### Stage Descriptions

**Stage 1 — Auto-Swell (Boss SG-1)**
Custom envelope follower using a one-pole IIR via `FastMath::fastExp`. ParameterSmoother applied to gain output. Schmitt trigger with hysteresis prevents spurious re-triggering on signals near the threshold boundary.

**Stage 2 — Ring Modulator (Fairfield Randy's Revenge)**
Dual PolyBLEP instances (sine + square carrier). Morph crossfade is manual: `out = sine * (1-blend) + square * blend`, then multiplied by the input signal. `FastMath::fastTanh` applied to carrier for nonlinear character. DC offset leak capacitor simulated.

> Note: PolyBLEP has no native morph API — manual crossfade of two instances is the required implementation.

**Stage 3 — Hard Clip Distortion (Boss DS-1 Golden)**
Custom Saturator mode: soft-knee `x/(1+|x|)` or hard clamp selectable. Internal 4x oversampling with polyphase IIR anti-aliasing. Post-clip tilt EQ via CytomicSVF (low-shelf mode) for voice balance.

**Stage 4 — Envelope Filter (Boss TW-1 T Wah)**
Envelope follower drives logarithmic frequency mapping: `base_freq * FastMath::fastPow2(env * depth)`. CytomicSVF LP 12 dB/oct. Cutoff updated per-sample via ParameterSmoother to prevent stepping artifacts.

**Stage 5 — Pitch-Sequenced BBD Delay (Chase Bliss Thermae)**
Fractional delay line with Hermite interpolation. Step sequencer with 2 pitch steps (in semitones) that modulate the delay read-pointer rate. Host-synced via `juce::AudioPlayHead`. Mono-to-stereo expansion occurs at this stage (L = step 1 pitch, R = step 2 pitch).

#### Parameters (17)

| Parameter ID | Range | Description |
|-------------|-------|-------------|
| `onr_swellThresh` | dB | Auto-swell gate threshold |
| `onr_swellAttack` | ms | Swell attack time |
| `onr_swellRelease` | ms | Swell release time |
| `onr_ringFreq` | 20–2000 Hz | Ring modulator carrier frequency |
| `onr_ringWave` | 0.0–1.0 | Carrier wave blend: 0 = sine, 1 = square |
| `onr_ringMix` | % | Ring modulator wet/dry mix |
| `onr_distDrive` | 0–40 dB | Distortion drive amount |
| `onr_distTilt` | -1.0–1.0 | Post-clip tilt EQ balance |
| `onr_distOut` | dB | Output level after distortion |
| `onr_envBaseFreq` | Hz | Envelope filter base frequency |
| `onr_envDepth` | octaves | Envelope filter sweep depth |
| `onr_envRes` | 0.707–10.0 Q | Envelope filter resonance |
| `onr_delayBase` | ms | BBD delay base time |
| `onr_seqStep1` | semitones | Sequencer step 1 pitch offset |
| `onr_seqStep2` | semitones | Sequencer step 2 pitch offset |
| `onr_delayFb` | % | Delay feedback amount |
| `onr_delayMix` | % | Delay wet/dry mix |

---

### 2.2 OMNISTEREO — Stereo Widener

**Source concept:** ChromaSpace (Gemini Pedalboard Series)
**Routing:** Stereo In → Stereo Out (true stereo throughout)
**Accent Color:** Prismatic Silver `#B0C4DE`
**Parameter prefix:** `omni_`

#### Signal Chain (5 Stages)

```
Stereo Input
   │
   ▼
[Stage 1] Tape Saturation & Hysteresis (Strymon Deco V2)
   │
   ▼
[Stage 2] Precision Parametric EQ (Boss SP-1 Spectrum)
   │
   ▼
[Stage 3] Bucket-Brigade Vibrato (Boss VB-2)
   │
   ▼
[Stage 4] BBD Chorus Ensemble (Boss CE-1)
   │
   ▼
[Stage 5] FDN Reverb (Meris MercuryX)
   │
   ▼
Stereo Output
```

#### Stage Descriptions

**Stage 1 — Tape Saturation & Hysteresis (Strymon Deco V2)**
Saturator in Tape mode (asymmetric + bias). Jiles-Atherton memory state maintained across blocks for hysteresis modeling. Stereo wow/flutter via per-channel fractional delay modulated by StandardLFO in S&H mode, fed through a one-pole lowpass (ParameterSmoother at approximately 2 Hz) for organic random drift that avoids the harsh stepping of raw S&H.

> Note: StandardLFO has no Perlin noise mode. Filtered S&H (StandardLFO S&H → one-pole lowpass) is the correct approximation.

**Stage 2 — Precision Parametric EQ (Boss SP-1 Spectrum)**
CytomicSVF in Peak mode (RBJ bell equation). True stereo — L and R processed identically with shared coefficients. Filter coefficients recalculated only when parameters change (not per-sample).

**Stage 3 — Bucket-Brigade Vibrato (Boss VB-2)**
Short fractional delay line (max 20 ms) modulated by StandardLFO (sine mode). A 4th-order Chebyshev lowpass darkening filter is linked to delay time — deeper modulation = more high-frequency roll-off, matching the BBD bucket-loss characteristic. Unlatch crossfade (50 ms) for switching in/out without audio discontinuity.

**Stage 4 — BBD Chorus Ensemble (Boss CE-1)**
L/R stereo split: StandardLFO drives left delay at phase 0, right delay at phase 180° (via `setPhaseOffset(0.5f)`). Compander emulation: compress signal before the delay, expand after (squeeze + breathe effect). Fixed 50% dry blend to preserve transient integrity.

**Stage 5 — FDN Reverb (Meris MercuryX)**
LushReverb (8-tap Hadamard FDN). Direct reuse of the existing shared DSP primitive. Features: damping, per-line shelving filters, per-line delay modulation, and post-FDN allpass diffusers.

#### Parameters (16)

| Parameter ID | Range | Description |
|-------------|-------|-------------|
| `omni_tapeDrive` | dB | Tape saturation drive |
| `omni_tapeBias` | offset | Tape bias (asymmetry amount) |
| `omni_tapeWow` | % | Wow/flutter depth |
| `omni_eqFreq` | 100–10k Hz | Parametric EQ center frequency |
| `omni_eqQ` | 0.1–10.0 | Parametric EQ Q factor |
| `omni_eqGain` | -15 to +15 dB | Parametric EQ gain |
| `omni_vibRate` | 0.1–10 Hz | Vibrato LFO rate |
| `omni_vibDepth` | 0–5 ms | Vibrato delay depth |
| `omni_vibUnlatch` | ms | Vibrato unlatch crossfade time |
| `omni_choRate` | Hz | Chorus LFO rate |
| `omni_choDepth` | ms | Chorus delay depth |
| `omni_choCompander` | % | Compander effect depth |
| `omni_revSize` | decay multiplier | Reverb room size |
| `omni_revDamp` | Hz | Reverb high-frequency damping |
| `omni_revMod` | % | Reverb modulation depth |
| `omni_revMix` | % | Reverb wet/dry mix |

---

### 2.3 OBLITERATE — Heavy Stutter

**Source concept:** GritGlitch (Gemini Pedalboard Series)
**Routing:** Mono In → Stereo Out
**Accent Color:** Apocalypse Red `#8B0000`
**Parameter prefix:** `oblt_`

#### Signal Chain (5 Stages)

```
Mono Input
   │
   ▼
[Stage 1] Shimmer Reverb (OBNE Dark Star)
   │
   ▼
[Stage 2] Octave Fuzz (Boss FZ-2 Hyper Fuzz)
   │
   ▼
[Stage 3] Reverse Pitch Delay (Boss PS-3 Mode 7)
   │
   ▼
[Stage 4] Granular Looper (Hologram Microcosm)
   │
   ▼
[Stage 5] Hard-Chopped Tremolo (Boss PN-2)
   │
   ▼
Stereo Output
```

#### Stage Descriptions

**Stage 1 — Shimmer Reverb (OBNE Dark Star)**
Custom FDN with a PSOLA pitch shifter in the feedback path, implemented as dual crossfading delay lines. Pitch shift is fixed at +12 or -12 semitones per voice. Approximately 50% of the reverb tail is routed back through the shifter before re-injection, creating the self-ascending shimmer characteristic.

**Stage 2 — Octave Fuzz (Boss FZ-2 Hyper Fuzz)**
Full-wave rectification via custom Saturator mode: `abs(x) * gain`. 8x oversampled to suppress aliasing artifacts from the hard nonlinearity. Post-rectification: Baxandall tone stack modeled via two CytomicSVF instances (low-shelf + high-shelf).

**Stage 3 — Reverse Pitch Delay (Boss PS-3 Mode 7)**
Dual circular buffers (2 seconds maximum): Buffer A writes forward at the current write pointer; Buffer B reads backward at a mirrored read pointer. Hann window crossfade applied at buffer boundaries. Pitch shifting is achieved by adjusting the read-pointer rate (rate > 1.0 = up, rate < 1.0 = down).

**Stage 4 — Granular Looper (Hologram Microcosm)**
Custom `GrainScheduler` that spawns `Grain` objects. Per-grain parameters: start index, length (10–100 ms), Gaussian window envelope, and stereo scatter panning. Stochastic variation via `juce::Random`. Maintains a dedicated capture `AudioBuffer` for the grain source material.

**Stage 5 — Hard-Chopped Tremolo (Boss PN-2)**
StandardLFO with shape selection (triangle to square). Gain applied via ParameterSmoother per-channel (L + R independently) for stereo ping-pong. Square mode uses slew limiting to prevent amplitude clicks while preserving the hard-chop character. R-channel phase offset creates ping-pong motion.

#### Parameters (19)

| Parameter ID | Range | Description |
|-------------|-------|-------------|
| `oblt_shimInterval` | semitones | Shimmer pitch shift amount (±12) |
| `oblt_shimDecay` | s | Shimmer reverb decay time |
| `oblt_shimMix` | % | Shimmer wet/dry mix |
| `oblt_fuzzGain` | dB | Octave fuzz drive |
| `oblt_fuzzRectMix` | % | Rectification blend |
| `oblt_fuzzTreble` | dB | Baxandall high-shelf gain |
| `oblt_fuzzBass` | dB | Baxandall low-shelf gain |
| `oblt_revDelayTime` | ms | Reverse delay loop length |
| `oblt_revDelayPitch` | ratio | Reverse delay pitch ratio |
| `oblt_revDelayMix` | % | Reverse delay wet/dry mix |
| `oblt_granDensity` | Hz | Grain spawn density |
| `oblt_granSize` | ms | Grain size (10–100 ms) |
| `oblt_granScatter` | % | Grain stereo scatter amount |
| `oblt_granMix` | % | Granular wet/dry mix |
| `oblt_tremRate` | Hz or Sync | Tremolo LFO rate |
| `oblt_tremShape` | 0.0=Tri, 1.0=Square | Tremolo LFO shape |
| `oblt_tremWidth` | % | Tremolo depth |
| `oblt_tremPhase` | degrees | R-channel phase offset for ping-pong |
| `oblt_tremMix` | % | Tremolo wet/dry mix |

---

### 2.4 OBSCURITY — Dark Ambient

**Source concept:** VoidAtmosphere (Gemini Pedalboard Series)
**Routing:** Mono In → Stereo Out
**Accent Color:** Void Purple `#2D1B69`
**Parameter prefix:** `obsc_`

#### Signal Chain (5 Stages)

```
Mono Input
   │
   ▼
[Stage 1] PLL Synthesizer (EQD Data Corrupter)
   │
   ▼
[Stage 2] Asymmetric Diode Overdrive (Boss PW-2)
   │
   ▼
[Stage 3] Dimension Chorus (Boss DC-2)
   │
   ▼
[Stage 4] Degrading BBD Delay (Boss DM-1)
   │
   ▼
[Stage 5] Industrial Multi-Tap Reverb (Death By Audio Rooms)
   │
   ▼
Stereo Output
```

#### Stage Descriptions

**Stage 1 — PLL Synthesizer (EQD Data Corrupter)**
Custom pitch tracker: extreme lowpass filter isolates fundamental → Schmitt trigger zero-crossing detector → period measurement → PolyBLEP square oscillator locked to tracked frequency. Sub-octaves generated via bitshift on the measured period (`period >> 1`, `period >> 2`). Glide smoothing applied via ParameterSmoother on the frequency output.

**Stage 2 — Asymmetric Diode Overdrive (Boss PW-2)**
Custom waveshaper with intentionally mismatched positive/negative clipping:
- Positive swing: `1.0 - exp(-x * drive)` (silicon diode response)
- Negative swing: `-1.0 + exp(x * drive * 0.5)` (germanium diode response)

4x oversampled. Pre-clip resonant low-shelf ("Fat") and post-clip wide-Q peak ("Muscle"), both via CytomicSVF.

**Stage 3 — Dimension Chorus (Boss DC-2)**
Two parallel fractional delay lines. StandardLFO A modulates Delay 1; StandardLFO B at 180° phase (via `setPhaseOffset(0.5f)`) modulates Delay 2. Hard-panned L/R. No feedback path — retains spatial clarity. Four discrete macro buttons map to hardcoded Depth/Rate pairs (matching the original DC-2's 4-button tactile interface concept), implemented as 4 mutually-exclusive boolean parameters.

**Stage 4 — Degrading BBD Delay (Boss DM-1)**
Fractional delay line. Feedback path includes: CytomicSVF lowpass fixed at 1.5 kHz (simulating bucket-brigade bandwidth loss) + `FastMath::fastTanh` saturation (simulating bucket-brigade distortion accumulation). Each repeat is progressively darker and crunchier. The `obsc_dlvCompound` parameter controls saturation depth; `obsc_dlvGrit` controls the filter cutoff directly.

**Stage 5 — Industrial Multi-Tap Reverb (Death By Audio Rooms)**
Eight delay taps at prime-number spacings, with cross-feedback between taps. Intentionally no allpass diffusers — discrete echoes rather than a smooth wash is the design intent. Hard clipper at input simulates ADC overload / input transformer saturation.

#### Parameters (20)

| Parameter ID | Range | Description |
|-------------|-------|-------------|
| `obsc_pllGlide` | ms | PLL frequency glide time |
| `obsc_pllSub1` | dB | Sub-octave 1 level (-1 octave) |
| `obsc_pllSub2` | dB | Sub-octave 2 level (-2 octaves) |
| `obsc_pllSquare` | dB | PLL square oscillator level |
| `obsc_odDrive` | dB | Diode overdrive drive |
| `obsc_odFatFreq` | Hz | Pre-clip low-shelf frequency |
| `obsc_odFatRes` | 0.5–5.0 Q | Pre-clip low-shelf resonance |
| `obsc_odMuscleFreq` | Hz | Post-clip wide-Q peak frequency |
| `obsc_odMuscleGain` | dB | Post-clip wide-Q peak gain |
| `obsc_odLevel` | dB | Overdrive output level |
| `obsc_dimMode1` | bool | Dimension chorus mode 1 (exclusive) |
| `obsc_dimMode2` | bool | Dimension chorus mode 2 (exclusive) |
| `obsc_dimMode3` | bool | Dimension chorus mode 3 (exclusive) |
| `obsc_dimMode4` | bool | Dimension chorus mode 4 (exclusive) |
| `obsc_dlvTime` | ms | BBD delay time |
| `obsc_dlvFeedback` | % | BBD delay feedback |
| `obsc_dlvCompound` | saturation amount | Feedback path saturation depth |
| `obsc_dlvGrit` | filter cutoff Hz | BBD bandwidth (degradation frequency) |
| `obsc_revSize` | decay multiplier | Multi-tap reverb decay |
| `obsc_revMix` | % | Reverb wet/dry mix |

---

## 3. Unified FX Slot System

### 3.1 Architecture Overview

The `EpicChainSlotController` replaces fixed FX chain positions in `MasterFXChain` with **3 fully assignable slots**. Each slot can hold any of the 10 available FX chains, in any order, with independent parameter sets.

**Available chains per slot (10):**

| Chain ID | Engine | Type |
|---------|--------|------|
| Off | — | Zero-cost passthrough |
| Aquatic | AquaticFXSuite | Suite |
| Math | MathFXChain | Suite |
| Boutique | BoutiqueFXChain | Suite |
| Onslaught | fXOnslaught | Singularity |
| Obscura | fXObscura | Singularity |
| Oratory | fXOratory | Singularity |
| Onrush | OnrushChain | Epic |
| Omnistereo | OmnistereoChain | Epic |
| Obliterate | ObliterateChain | Epic |
| Obscurity | ObscurityChain | Epic |

**Important:** The three Singularity FX engines (fXOnslaught, fXObscura, fXOratory) are now slot-only. They no longer exist as a separate always-on layer in the signal path.

**Signal flow:**

```
Engine Output
     │
     ▼
  [Slot 1]  ← any of the 10 chains, or Off
     │
     ▼
  [Slot 2]  ← any of the 10 chains, or Off
     │
     ▼
  [Slot 3]  ← any of the 10 chains, or Off
     │
     ▼
Master Output
```

**Slot behaviors:**
- Slots set to `Off` are zero-cost passthrough (no processing).
- The same chain can be loaded in multiple slots simultaneously (independent instances, independent state).
- Chain swap uses a 50 ms linear crossfade, matching the engine hot-swap convention.

### 3.2 EpicChainSlotController Class Design

```cpp
class EpicChainSlotController {
public:
    static constexpr int kNumSlots  = 3;
    static constexpr int kNumChains = 10;

    enum ChainID {
        Off = 0,
        Aquatic, Math, Boutique,
        Onslaught, Obscura, Oratory,
        Onrush, Omnistereo, Obliterate, Obscurity
    };

    void prepare(double sampleRate, int maxBlockSize);
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);
    static void addParameters(
        juce::AudioProcessorValueTreeState::ParameterLayout& layout);
    void cacheParameterPointers(
        juce::AudioProcessorValueTreeState& apvts);

private:
    struct FXSlot {
        ChainID activeChain  = Off;
        ChainID pendingChain = Off;
        float crossfadeProgress = 1.0f; // 1.0 = stable, 0.0 = crossfade start

        // All 10 chain instances pre-allocated in prepare()
        AquaticFXSuite    aquatic;
        MathFXChain       math;
        BoutiqueFXChain   boutique;
        fXOnslaught       onslaught;
        fXObscura         obscura;
        fXOratory         oratory;
        OnrushChain       onrush;
        OmnistereoChain   omnistereo;
        ObliterateChain   obliterate;
        ObscurityChain    obscurity;

        juce::AudioBuffer<float> crossfadeBuf; // scratch buffer for crossfade
    };

    std::array<FXSlot, kNumSlots> slots_;
};
```

**Allocation strategy:** All chain instances are pre-allocated once in `prepare()`. No heap allocation occurs on the audio thread. Only the active chain's `processBlock()` is called during normal operation.

### 3.3 Signal Flow and processBlock Logic

Per-slot processing logic executed in `processBlock()`:

```
for each slot:

  if activeChain == Off:
      → passthrough (no processing)

  else if crossfade is active (crossfadeProgress < 1.0):
      1. Copy buffer → crossfadeBuf
      2. Process outgoing chain on crossfadeBuf (old chain)
      3. Process incoming chain on buffer (new chain)
      4. Blend:
           buffer = buffer * crossfadeProgress
                  + crossfadeBuf * (1.0 - crossfadeProgress)
      5. Advance crossfadeProgress linearly
           (50ms / sampleRate samples per block increment)
      6. When crossfadeProgress >= 1.0:
           activeChain = pendingChain
           crossfadeProgress = 1.0

  else:
      → process active chain on buffer
      → apply per-slot mix:
           buffer = dry * (1.0 - slotMix) + wet * slotMix
```

**Chain swap trigger:** When `slot{N}_chain` parameter changes, `pendingChain` is set to the new `ChainID` and `crossfadeProgress` is reset to 0.0f. The outgoing chain continues to process during the fade.

### 3.4 Parameter ID Convention

All slot parameters use the format: `slot{N}_{chainPrefix}_{paramName}`

**Control parameters (9 total, 3 per slot):**

| Parameter ID | Description |
|-------------|-------------|
| `slot1_chain` | Active chain selector (ChainID enum) |
| `slot1_mix` | Slot wet/dry mix (0.0–1.0) |
| `slot1_bypass` | Slot bypass toggle |
| `slot2_chain` | Active chain selector |
| `slot2_mix` | Slot wet/dry mix |
| `slot2_bypass` | Slot bypass toggle |
| `slot3_chain` | Active chain selector |
| `slot3_mix` | Slot wet/dry mix |
| `slot3_bypass` | Slot bypass toggle |

**Chain parameters (examples):**

| Parameter ID | Source param | Slot |
|-------------|-------------|------|
| `slot1_aqua_fathom_mix` | AquaticFXSuite fathom mix | 1 |
| `slot1_onr_swellThresh` | OnrushChain swell threshold | 1 |
| `slot2_obsc_pllGlide` | ObscurityChain PLL glide | 2 |
| `slot3_math_entropy_depth` | MathFXChain entropy depth | 3 |

### 3.5 Parameter Budget

| Source | Count |
|--------|-------|
| Existing chain params (Aquatic + Math + Boutique + 3 Singularity FX) | 121 |
| New Epic Chain params (ONRUSH 17 + OMNISTEREO 16 + OBLITERATE 19 + OBSCURITY 20) | 72 |
| **Total per slot** | **193** |
| × 3 slots | 579 |
| + Control params (3 chain + 3 mix + 3 bypass) | 9 |
| **Grand total** | **~588** |

---

## 4. DSP Conventions

### 4.1 Native Primitives (No juce::dsp)

All DSP in XOceanus uses internal primitives exclusively. `juce::dsp::*` wrappers are never used.

| Primitive | Use Case |
|-----------|---------|
| `CytomicSVF` | All filtering: LP, HP, BP, Peak, Shelf, AllPass |
| `FastMath` | Fast approximations: `fastTanh`, `fastExp`, `fastPow2`, `fastLog2`, `fastSin`, `fastCos` |
| `Saturator` | Saturation / distortion: Tube, Tape, Digital, FoldBack + custom soft-knee modes |
| `LushReverb` | FDN reverb (8-tap Hadamard, shared DSP primitive) |
| `StandardLFO` | Modulation: Sine, Triangle, Saw, Square, S&H; phase offset via `setPhaseOffset()` |
| `PolyBLEP` | Oscillators: Sine, Saw, Square, Triangle, Pulse; BLEP anti-aliasing built in |
| `WavetableOscillator` | Wavetable morphing: 256 frames, Hermite interpolation |
| `ParameterSmoother` | All parameter smoothing: exponential, 5 ms default |
| `StandardADSR` | Envelopes |

### 4.2 Phantom Sniff Corrections

Four phantom assumptions were caught during design review and corrected before implementation begins:

| # | Phantom | Correction |
|---|---------|-----------|
| 1 | ONRUSH ring mod: assumed PolyBLEP has a native morph/blend mode | PolyBLEP has no native morph. Use manual crossfade of two PolyBLEP instances: `out = sine * (1-blend) + square * blend` |
| 2 | OMNISTEREO wow/flutter: assumed StandardLFO has a Perlin noise mode | StandardLFO has no Perlin mode. Use filtered S&H: StandardLFO in S&H mode → one-pole lowpass via ParameterSmoother at ~2 Hz |
| 3 | LFO frequency smoothing: assumed setRate() smooths internally | setRate() has no built-in smoothing. Wrap call site in a ParameterSmoother and pass the smoothed value |
| 4 | LFO shape morphing: assumed StandardLFO supports continuous morphing between shapes | Not supported. All morph cases resolved via manual crossfade or separate instances (already used in ONRUSH Stage 2) |

---

## 5. Preset Migration

### 5.1 Legacy Detection

`PresetManager` detects legacy presets (pre-Epic Chains / pre-slot system) by checking for the **absence** of the `slot1_chain` parameter key in the loaded JSON.

```
if preset["parameters"]["slot1_chain"] is absent:
    → trigger fxVersion 1→2 migration
```

After migration, the preset is tagged with `"fxVersion": 2`.

**Parameter aliases:** In addition to preset migration, `PresetManager` registers permanent parameter aliases mapping all pre-slot parameter IDs to their slot-prefixed equivalents. This ensures DAW automation recorded to old parameter IDs (`aqua_fathom_mix`, `master_onslFlowRate`, etc.) continues to function transparently after the slot system is active.

### 5.2 Migration Map

| Legacy Parameter Pattern | Migrated To | Target Slot |
|-------------------------|------------|-------------|
| `aqua_*` params with non-zero mix | Slot 1 (chain = Aquatic), keys become `slot1_aqua_*` | Slot 1 |
| `mfx_*` params with non-zero mix | Slot 2 (chain = Math), keys become `slot2_mfx_*` | Slot 2 |
| `bfx_*` params with non-zero mix | Slot 3 (chain = Boutique), keys become `slot3_bfx_*` | Slot 3 |
| `master_onsl*` | First empty slot, chain = Onslaught | First available |
| `master_obs*` | First empty slot, chain = Obscura | First available |
| `master_ora*` | First empty slot, chain = Oratory | First available |

### 5.3 Edge Cases

**More active chains than available slots:** If migration finds more than 3 active legacy chains (e.g., all 3 suites plus at least one Singularity FX active), the priority order is:

1. Suite chains (Aquatic → Slot 1, Math → Slot 2, Boutique → Slot 3)
2. Singularity FX are dropped (not migrated)

A console warning is emitted for every dropped chain:
```
[PresetManager] fxVersion migration warning: Singularity FX '<name>' dropped — no available slot.
```

**Version handling summary:**

| `fxVersion` value | Action |
|-------------------|--------|
| 2 | Load directly — no migration |
| 1 | Trigger migration → write `"fxVersion": 2` |
| absent | Treat as version 1, trigger migration |

---

## 6. Accent Colors

| Engine | Color Name | Hex |
|--------|-----------|-----|
| ONRUSH | Molten Amber | `#FF6F00` |
| OMNISTEREO | Prismatic Silver | `#B0C4DE` |
| OBLITERATE | Apocalypse Red | `#8B0000` |
| OBSCURITY | Void Purple | `#2D1B69` |

---

## 7. Files to Create and Modify

### 7.1 New Files

| Path | Contents |
|------|---------|
| `Source/DSP/Effects/OnrushChain.h` | ONRUSH 5-stage chain (Auto-Swell → Ring Mod → Distortion → Env Filter → Pitch-Seq Delay) |
| `Source/DSP/Effects/OmnistereoChain.h` | OMNISTEREO 5-stage chain (Tape Sat → Param EQ → Vibrato → Chorus → FDN Reverb) |
| `Source/DSP/Effects/ObliterateChain.h` | OBLITERATE 5-stage chain (Shimmer Rev → Octave Fuzz → Reverse Delay → Granular → Tremolo) |
| `Source/DSP/Effects/ObscurityChain.h` | OBSCURITY 5-stage chain (PLL Synth → Asym Diode → Dimension Chorus → BBD Delay → Industrial Reverb) |
| `Source/Core/EpicChainSlotController.h` | 3-slot assignable FX controller (all 10 chains, crossfade, parameter routing) |

All DSP headers follow the XOceanus DSP header convention: all implementation inline in `.h`; corresponding `.cpp` is a one-line include stub.

### 7.2 Modified Files

| Path | Change |
|------|--------|
| `Source/Core/MasterFXChain.h` | Replace fixed chain positions with `EpicChainSlotController`. Remove standalone Singularity FX layer (now slot-only). |
| `Source/Core/PresetManager.h` | Add `fxVersion 1→2` migration logic. Add `slot1_chain` absence detection. Emit console warnings for dropped chains. Register permanent parameter aliases (old IDs → slot-prefixed IDs) for DAW automation compatibility. |
| `CMakeLists.txt` | Add new `.cpp` stub files to the build target source list: `OnrushChain.cpp`, `OmnistereoChain.cpp`, `ObliterateChain.cpp`, `ObscurityChain.cpp`, `EpicChainSlotController.cpp`. |
| `CLAUDE.md` | Add all 4 Epic Chain engines to: (1) engine modules registered list, (2) Engine Modules table, (3) Parameter Prefix table, (4) Key Files table. |

---

## 8. Future Extensibility

The slot system is designed for expansion with minimal architectural impact.

**Adding a 4th slot:**
- Add `slot4_*` parameters in `addParameters()`.
- Increase `kNumSlots = 4` in `EpicChainSlotController`.
- No structural change required.

**Adding a new chain to the pool:**
1. Add the new `ChainID` enum value.
2. Add the instance to `FXSlot` struct.
3. Add parameters in `addParameters()` under the `slot{N}_newchain_*` prefix.
4. Handle the new ID in the `processBlock()` dispatch switch.

**New FX concept batches queued:**
- `MonsterousFXCollection.pdf` — next batch candidate
- `SunkenTreasureFX.pdf` — next batch candidate

**Parameter budget impact per new chain:** ~20 params × 3 slots = ~60 additional APVTS parameters.

---

## 9. CLAUDE.md Update Checklist

When this feature is implemented, the following CLAUDE.md sections must be updated:

**Product Identity — Engine modules (registered) list:**
Add: `ONRUSH, OMNISTEREO, OBLITERATE, OBSCURITY`

**Engine Modules table — add 4 rows:**

| Short Name | Source Instrument | Accent Color |
|-----------|------------------|-------------|
| ONRUSH | PolySwell (Gemini Pedalboard Series) | Molten Amber `#FF6F00` |
| OMNISTEREO | ChromaSpace (Gemini Pedalboard Series) | Prismatic Silver `#B0C4DE` |
| OBLITERATE | GritGlitch (Gemini Pedalboard Series) | Apocalypse Red `#8B0000` |
| OBSCURITY | VoidAtmosphere (Gemini Pedalboard Series) | Void Purple `#2D1B69` |

**Parameter Prefix table — add 4 rows:**

| Engine ID | Parameter Prefix | Example |
|-----------|-----------------|---------|
| Onrush | `onr_` | `onr_swellThresh` |
| Omnistereo | `omni_` | `omni_tapeDrive` |
| Obliterate | `oblt_` | `oblt_shimInterval` |
| Obscurity | `obsc_` | `obsc_pllGlide` |

**Key Files table — add entries:**

| Path | Purpose |
|------|---------|
| `Source/DSP/Effects/OnrushChain.h` | ONRUSH 5-stage expressive lead FX chain |
| `Source/DSP/Effects/OmnistereoChain.h` | OMNISTEREO 5-stage stereo widener FX chain |
| `Source/DSP/Effects/ObliterateChain.h` | OBLITERATE 5-stage heavy stutter FX chain |
| `Source/DSP/Effects/ObscurityChain.h` | OBSCURITY 5-stage dark ambient FX chain |
| `Source/Core/EpicChainSlotController.h` | Unified 3-slot assignable FX controller |

**Also update `Docs/specs/xoceanus_master_specification.md`** section 3.1 engine table with the 4 new rows.

---

## 10. Architect Review Resolutions

Reviewed 2026-04-05. Verdict: **APPROVE WITH CONDITIONS** — 2 blocking findings resolved below.

### Finding #1: Singularity FX Stage Repositioning — RESOLVED (Option C)

**Decision:** Accept the sonic break. XOceanus is pre-release — no external users have DAW sessions depending on Singularity FX at their current fixed positions (stages 5.5/8.5/14.5).

**Action items:**
- Singularity FX move to end-of-chain slot positions (no pre/post complexity)
- Any factory presets using Singularity FX must be re-tuned after migration to sound correct at their new signal position
- Document the stage repositioning in the release changelog

### Finding #2: Parameter ID Automation Breakage — RESOLVED (Option A)

**Decision:** Register hidden parameter aliases. Old parameter IDs (`aqua_fathom_mix`, `master_onslFlowRate`, etc.) remain valid as read-through aliases to their new slot-prefixed equivalents.

**Implementation:**
- `PresetManager` maintains a `std::unordered_map<juce::String, juce::String> paramAliases_` mapping old IDs → new slot-prefixed IDs
- When a DAW or preset requests a parameter by old ID, the alias map resolves it to the correct slot parameter
- Alias map is populated during migration: 121 existing chain params + 28 Singularity FX params = ~149 alias entries
- Aliases are permanent — once registered, never removed (forward compatibility)
- DAW automation recorded to old IDs continues to work transparently

### Finding #3: Mythological Identity — DEFERRED

Mythological identities (aquatic creature, depth zone, narrative) for the 4 Epic Chains will be assigned during the preset design phase, before presets are written.

### Finding #4: Epic Chain Macros — DEFERRED

Consideration of 1-2 macros per Epic Chain (sweeping characterful parameters simultaneously) deferred to a follow-up pass after initial implementation.

---

*End of spec — Epic Chains FX Design v1.0 — 2026-04-05*
