# Outshine Phase 1B — Rebirth Mode Design Specification

**Date:** 2026-03-25
**Status:** Approved — Implementation Ready (pending Architect Gates)
**Replaces:** Python CLI Rebirth Mode (deprecated as of Phase 1B)
**Spec Lineage:** QDD CI Run 2026-03-25, 5-level audit, 16+ reviewers

---

## 1. Overview

Rebirth Mode transforms producer samples through engine-inspired DSP chains, producing "reborn" instruments where each XOceanus engine's sonic character shapes the sample's timbre. The sample IS the primary audio source — it flows through the FX chain and comes out transformed. The engine's DSP acts as a transfer function on the sample.

**What Rebirth is NOT:**
- NOT routing samples through live SynthEngine instances (the coupling system is parameter modulation, not audio-through — this was proven non-viable in QDD LVL1-3 review)
- NOT convolution reverb (the DSP chain has dynamic parameters, not a static impulse response)
- NOT a wet/dry volume blend for velocity layers (DOC-004 requires spectral variation)

**What Rebirth IS:**
- Engine-Inspired Transformation: standalone FX chains that encode each engine's mathematical character
- The sample excites the DSP chain; the chain shapes the sample's energy
- "Rebirth is not measuring a room. The engine is playing the sample as an instrument."

### 1.1 Architecture Decision Record

The original design proposed routing samples through live SynthEngine instances via the coupling system (`applyCouplingInput`). This was unanimously blocked (12-0) in QDD review because:
- 3/5 proposed profiles (ONSET, OWARE, OPERA) have zero coupling effect — the coupling types either don't exist (`AudioToFilter`) or aren't implemented by those engines
- Coupling is a parameter modulation system, not audio-through — the sample's spectral content never enters the engine's signal chain
- The producer hears the engine's own oscillators with a subtle parameter wobble, not their sample transformed

Option B (Engine-Inspired FX Chain) was blessed unanimously (8-0) by the Ghost Council as the correct architecture. The sample is always the primary audio source. DSP modules extracted from or inspired by engine implementations shape it.

### 1.2 Relationship to Python CLI

The Python CLI (`Tools/xoutshine.py`) has an existing Rebirth Mode with 5 profiles (OBESE, OUROBOROS, OPAL, ORIGAMI, OVERDUB) using approximate DSP transforms. The C++ Phase 1B is a **new system sharing the pipeline architecture but with different, more sophisticated DSP algorithms.** The Python CLI Rebirth is deprecated as of Phase 1B. The Python CLI docstring must be updated to state: "Prototype implementation. Production Rebirth Mode is in the XOceanus desktop app."

---

## 2. Pipeline Architecture

```
[Source Sample Buffer (any SR, any bit depth)]
  │
  ├─ Step 1: Resample to target SR (44.1 or 48kHz, matching engine convention)
  │          Use juce::LagrangeInterpolator. Pre-compute full resampled buffer once.
  │
  ├─ Step 2: Input Normalization
  │          Target: -18 dBFS integrated LUFS (EBU R128)
  │          Provides 18dB headroom for DSP stages that add gain
  │
  ├─ Step 3: Audio Analysis
  │          • Onset detection (transient ratio: 0.0 = sustained, 1.0 = sharp transient)
  │          • Spectral centroid (brightness indicator)
  │          • Spectral flatness (tonal vs noise-like: 0.0 = pure tone, 1.0 = white noise)
  │          • Sample duration
  │          These 4 metrics form a feature vector for auto-parameter selection.
  │
  ├─ Step 4: Engine-Inspired FX Chain
  │          Per-profile DSP module chain. Modules come from Source/DSP/Effects/.
  │          Inter-stage soft-clip guards (tanh(x * 0.7f)) between modules that add gain.
  │          Parameters are velocity-scaled for spectral variation (see Section 6).
  │
  ├─ Step 5: Output Gain Compensation
  │          Match input integrated LUFS ±0.5 LU
  │
  ├─ Step 6: True Peak Limiter
  │          Ceiling: -1.0 dBTP via BrickwallLimiter.h (already exists in codebase)
  │
  └─ Output: Reborn Sample Buffer (at target SR)
```

---

## 3. File Structure

```
Source/Export/
├── RebirthPipeline.h    — Pipeline orchestrator (generic, profile-agnostic)
├── RebirthProfiles.h    — Profile definitions (DSP chain configurations + parameter tables)
└── RebirthDSP.h         — Additional DSP modules not already in Source/DSP/Effects/
                           (e.g., FFT formant estimator for OPERA profile)
                           MUST wrap existing modules where possible. MUST NOT reimplement
                           Saturator, Combulator, BrickwallLimiter, TransientDesigner, etc.
```

### 3.1 RebirthPipeline.h

The pipeline is a **generic DSP chain executor**. It reads a profile configuration and executes the chain. There MUST be no `if (profile == OBRIX)` conditional logic in the pipeline. Profiles are data structures that the pipeline iterates.

```cpp
namespace xoceanus {

struct RebirthSettings {
    RebirthProfileID profileId = RebirthProfileID::OBRIX;
    float intensity = 0.7f;           // 0.0 = dry, 1.0 = full transformation
    float chaosAmount = 0.0f;         // 0.0 = deterministic, 1.0 = full randomization
                                      // (Phase 1B design only, implementation Phase 1C)
    bool enabled = false;
};

class RebirthPipeline {
public:
    // Process a single sample buffer through the Rebirth FX chain.
    // Returns a new buffer with the reborn audio.
    // velocityNorm: 0.0 (softest layer) to 1.0 (hardest layer) — drives spectral parameter scaling
    // rng: seeded RNG for per-round-robin variation
    juce::AudioBuffer<float> process(
        const juce::AudioBuffer<float>& source,
        double sourceSampleRate,
        const RebirthSettings& settings,
        float velocityNorm,
        juce::Random& rng,
        ProgressCallback progress = nullptr);

    // Preview: process first 2 seconds only, at full quality.
    // Target: <3 seconds wall time on M1.
    juce::AudioBuffer<float> preview(
        const juce::AudioBuffer<float>& source,
        double sourceSampleRate,
        const RebirthSettings& settings);

    // Cancel a running process/preview (thread-safe)
    void cancel();

private:
    std::atomic<bool> cancelFlag_ { false };

    // Pipeline stages (called sequentially by process())
    juce::AudioBuffer<float> resample(
        const juce::AudioBuffer<float>& source,
        double sourceSR,
        double targetSR);

    float computeIntegratedLUFS(
        const juce::AudioBuffer<float>& buffer,
        double sampleRate);

    AnalysisResult analyzeAudio(
        const juce::AudioBuffer<float>& buffer,
        double sampleRate);

    juce::AudioBuffer<float> applyChain(
        const juce::AudioBuffer<float>& buffer,
        double sampleRate,
        const RebirthProfile& profile,
        float velocityNorm,
        const AnalysisResult& analysis,
        juce::Random& rng);

    void compensateGain(
        juce::AudioBuffer<float>& buffer,
        double sampleRate,
        float targetLUFS);

    void applyTruePeakLimit(
        juce::AudioBuffer<float>& buffer,
        double sampleRate);
};

struct AnalysisResult {
    float transientRatio;    // 0.0 = sustained, 1.0 = sharp attack
    float spectralCentroid;  // Hz — brightness indicator
    float spectralFlatness;  // 0.0 = tonal, 1.0 = noise-like
    float durationS;         // seconds
};

} // namespace xoceanus
```

### 3.2 RebirthProfiles.h

Profiles are defined as data structures: arrays of `{DSPModuleID, parameters}` tuples. The pipeline iterates them generically. Some profiles may require DSP modules defined in `RebirthDSP.h` (e.g., OPERA's FFT formant estimator). This is acceptable — adding a new profile that requires a novel algorithm requires adding that algorithm to `RebirthDSP.h`.

```cpp
namespace xoceanus {

enum class RebirthProfileID {
    OBRIX,      // Character — multi-band saturation + comb filters + ecological reverb
    ONSET,      // Percussion — transient shaping + noise burst + filter sweep
    OWARE,      // Resonance — Akan-interval comb bank + sympathetic resonance
    OPERA,      // Tonal — FFT formant estimation + resonator bank
    OVERWASH    // Diffusion — allpass cascade + spectral blur + slow modulation
};

enum class RebirthDSPModuleID {
    Saturator,           // Source/DSP/Effects/Saturator.h
    Combulator,          // Source/DSP/Effects/Combulator.h
    TransientDesigner,   // Source/DSP/Effects/TransientDesigner.h (or new if absent)
    BrickwallLimiter,    // Source/DSP/Effects/BrickwallLimiter.h
    AllpassDiffuser,     // New in RebirthDSP.h — multi-tap allpass cascade
    FormantResonator,    // New in RebirthDSP.h — FFT peak detection + resonator bank
    NoiseBurst,          // New in RebirthDSP.h — short noise transient injector
    SpectralTilt,        // Source/DSP/Effects/SpectralTilt.h (if exists) or new
    SoftClipGuard,       // Inline: tanh(x * 0.7f) between gain-adding stages
    WetDryMix            // Final intensity blend
};

struct DSPModuleConfig {
    RebirthDSPModuleID moduleId;
    // Per-module parameter map (key → base value)
    std::unordered_map<std::string, float> params;
    // Per-module velocity scaling (key → {minAtVel0, maxAtVel1})
    std::unordered_map<std::string, std::pair<float, float>> velocityScale;
};

struct RebirthProfile {
    RebirthProfileID id;
    const char* engineName;        // "OBRIX", "ONSET", etc.
    const char* producerLabel;     // "Harmonic Character", "Percussive Crunch", etc.
    const char* description;       // 1-line sonic description for UI
    const char* characterBrief;    // Acceptance test: what should the output sound like?
    float recommendedTransientMax; // 0.0-1.0 — warn if sample transient ratio exceeds this
    float tailSeconds;             // Extra render time after sample ends (for reverb/diffusion tails)
    std::vector<DSPModuleConfig> chain; // Ordered list of DSP modules to apply
};

// Returns the built-in profile for the given ID
const RebirthProfile& getRebirthProfile(RebirthProfileID id);

} // namespace xoceanus
```

### 3.3 RebirthDSP.h

New DSP modules not already in `Source/DSP/Effects/`. Each module follows the pattern: `prepare(sampleRate, maxBlockSize)`, `process(buffer, numSamples, params)`, `reset()`.

Required new modules:

1. **AllpassDiffuser** — Multi-tap allpass cascade for OVERWASH diffusion (4+ stages, configurable delay times, feedback)
2. **FormantResonator** — FFT-based formant peak detection + parallel bandpass resonator bank for OPERA (NOT LPC — LPC is too expensive for <3s preview. Use windowed FFT with peak picking.)
3. **NoiseBurst** — Short filtered noise transient injector for ONSET (attack enhancement)
4. **SpectralTilt** — If not already in codebase. Frequency-dependent gain tilt for velocity spectral shaping.

---

## 4. Profile Definitions

### 4.1 OBRIX — "Harmonic Character"

**Sonic character target:** Reef-modal harmonic complexity. The sample gains overtone density and comb-filtered resonance, as if resonating inside a coral reef structure. At default settings, the sample is clearly recognizable but richer and more harmonically alive.

**Producer label:** "Harmonic Character"
**Material category:** TONAL
**Best for:** Melodic content, keys, plucks, textured percussion
**Transient max warning:** 0.7 (percussive material may sound over-saturated)
**Tail seconds:** 0.5

**DSP Chain:**

| Step | Module | Key Parameters |
|------|--------|---------------|
| 1 | Saturator | drive: base 0.4, vel-scaled 0.2→0.8 |
| 2 | SoftClipGuard | tanh(x * 0.7f) / tanh(0.7f) |
| 3 | Combulator | 3 combs, feedback: base 0.85, vel-scaled 0.75→0.95, damping: 0.3 |
| 4 | LushReverb (or equivalent) | decay: 0.4s, mix: 0.25, vel-scaled 0.15→0.35 |
| 5 | WetDryMix | intensity parameter |

**Velocity spectral scaling:**
- Vel 0 (soft): Low saturation drive (0.2), bright comb damping (0.5), short reverb (0.15 mix) → airy, fragile
- Vel 1 (hard): High saturation drive (0.8), dark comb damping (0.1), longer reverb (0.35 mix) → dense, compressed, thick

---

### 4.2 ONSET — "Percussive Crunch"

**Sonic character target:** Transient emphasis with added noise texture. The sample's attack is sculpted and enhanced with a brief noise burst that adds "crack." The body is filter-swept. Designed for drums and percussion.

**Producer label:** "Percussive Crunch"
**Material category:** PERCUSSIVE
**Best for:** Kicks, snares, hi-hats, claps, toms, percussion one-shots
**Transient max warning:** none (designed for transient material)
**Tail seconds:** 0.15

**DSP Chain:**

| Step | Module | Key Parameters |
|------|--------|---------------|
| 1 | TransientDesigner | attack: base +6dB, vel-scaled +2→+12dB; sustain: -3dB |
| 2 | NoiseBurst | burst length: 5ms, level: -24dB, vel-scaled -30→-18dB, filter: 4kHz HP |
| 3 | Saturator | drive: base 0.3, vel-scaled 0.1→0.6 |
| 4 | Biquad LP filter sweep | cutoff: vel-scaled 8kHz→18kHz, Q: 0.7 |
| 5 | WetDryMix | intensity parameter |

**Velocity spectral scaling:**
- Vel 0 (soft): Minimal transient boost (+2dB), quiet noise, low saturation, darker filter → gentle, muted
- Vel 1 (hard): Maximum transient boost (+12dB), loud noise burst, high saturation, bright filter → aggressive, punchy

---

### 4.3 OWARE — "Resonant Body"

**Sonic character target:** The sample excites a tuned resonant body based on Akan percussion intervals. Comb filters tuned to musical ratios create pitched resonance from unpitched material. Sympathetic resonance adds shimmer.

**Producer label:** "Resonant Body"
**Material category:** HARMONIC
**Best for:** Tuned percussion, metallic samples, bell-like content, melodic hits
**Transient max warning:** none
**Tail seconds:** 0.8

**DSP Chain:**

| Step | Module | Key Parameters |
|------|--------|---------------|
| 1 | Combulator | 3 combs tuned to Akan intervals (auto-selected by spectral centroid); feedback: base 0.9, vel-scaled 0.8→0.98 |
| 2 | SoftClipGuard | tanh(x * 0.7f) / tanh(0.7f) |
| 3 | Sympathetic resonance | 2nd comb bank at harmonic intervals, feedback 0.3-0.5, mix 0.15 |
| 4 | SpectralTilt | vel-scaled: soft = +3dB/oct brightness, hard = -2dB/oct darkness |
| 5 | WetDryMix | intensity parameter |

**Akan interval sets** (auto-selected by spectral centroid from OwareEngine.h ratios):
- Wood: [1.0, 2.76, 5.40]
- Metal: [1.0, 2.0, 3.0]
- Bell: [1.0, 2.32, 4.18]

Selection heuristic: spectral centroid < 1kHz → Wood, 1–4kHz → Metal, > 4kHz → Bell.

**Velocity spectral scaling:**
- Vel 0 (soft): Lower comb feedback (0.8), brighter tilt, less sympathetic → clean, bright ring
- Vel 1 (hard): Higher comb feedback (0.98), darker tilt, more sympathetic → dense, dark, buzzy

---

### 4.4 OPERA — "Harmonic Shimmer"

**Sonic character target:** The sample's formant structure is analyzed via FFT peak detection and resynthesized through a parallel resonator bank inspired by OPERA's Kuramoto additive architecture. Tonal material gains vocal/choral quality. Non-tonal material gains harmonic focus.

**Producer label:** "Harmonic Shimmer"
**Material category:** TONAL
**Best for:** Vocal samples, pads, sustained tonal material, chords
**Transient max warning:** 0.5 (transient material will have attack blur from resonator ringing)
**Tail seconds:** 1.0

**DSP Chain:**

| Step | Module | Key Parameters |
|------|--------|---------------|
| 1 | FormantResonator | FFT analysis: 2048-point windowed FFT, top 4 peaks as formant centers; parallel bandpass resonators at formant frequencies, Q: base 8.0, vel-scaled 4.0→16.0; resonator mix: 0.4, dry: 0.6 |
| 2 | SpectralTilt | vel-scaled: soft = flat, hard = +2dB/oct high boost → shimmer |
| 3 | Slow LFO modulation | rate: 0.3Hz, depth: ±50Hz on resonator center frequencies |
| 4 | WetDryMix | intensity parameter |

**FormantResonator implementation note:** Use windowed FFT peak picking (NOT LPC). LPC is too expensive for the <3s preview target on M1. Windowed FFT with peak detection is sufficient for perceptual formant capture at this quality level.

**Velocity spectral scaling:**
- Vel 0 (soft): Wide Q resonators (4.0), no spectral tilt, minimal LFO → subtle warmth
- Vel 1 (hard): Narrow Q resonators (16.0), bright tilt, more LFO depth → dramatic, vocal, shimmering

---

### 4.5 OVERWASH — "Deep Diffusion"

**Sonic character target:** The sample is washed through a multi-stage allpass diffusion network that blurs time and spectral content. Transients dissolve. The result is ambient, evolving, textural. Like hearing the sample through deep water.

**Producer label:** "Deep Diffusion"
**Material category:** TEXTURAL
**Best for:** Pads, textures, ambient material, field recordings, loops
**Transient max warning:** 0.4 (percussive material WILL lose its attack — this is by design but should be warned)
**Tail seconds:** 2.0

**DSP Chain:**

| Step | Module | Key Parameters |
|------|--------|---------------|
| 1 | AllpassDiffuser | 4 stages, delay times: [7.1ms, 11.3ms, 17.9ms, 23.7ms]; feedback per stage: base 0.6, vel-scaled 0.4→0.8 |
| 2 | Biquad LP filter | cutoff: vel-scaled 12kHz→6kHz (harder = darker) |
| 3 | Slow LFO on allpass delay times | rate: 0.15Hz, depth: ±1.5ms — chorus-like movement |
| 4 | SpectralTilt | vel-scaled: soft = +1dB/oct bright, hard = -3dB/oct dark → submerged |
| 5 | WetDryMix | intensity parameter |

**AllpassDiffuser delay time rationale:** Prime-ish ratios (7.1, 11.3, 17.9, 23.7 ms) avoid constructive resonance artifacts that would colorize the output with visible spectral peaks.

**Velocity spectral scaling:**
- Vel 0 (soft): Low diffusion feedback (0.4), bright filter, bright tilt → ethereal, open
- Vel 1 (hard): High diffusion feedback (0.8), dark filter (6kHz), dark tilt → deep, submerged, oceanic

---

## 5. Gain Staging Specification

### 5.1 Input Normalization

- Compute integrated LUFS of the source sample (ITU-R BS.1770-4 K-weighted — see Section 12)
- Apply gain to reach -18 dBFS LUFS target
- Store original LUFS value for output compensation

### 5.2 Inter-Stage Headroom

- After any module that adds gain (Saturator, Combulator with feedback > 0.9), insert a SoftClipGuard:

```cpp
// SoftClipGuard formula — soft-limits to ±1.0 without hard clipping
output = tanh(input * 0.7f) / tanh(0.7f);
```

- Document per-profile headroom budget in `RebirthProfiles.h` comments

### 5.3 Output Compensation

- Compute integrated LUFS of the processed buffer
- Apply gain to match input LUFS ±0.5 LU
- Apply BrickwallLimiter at -1.0 dBTP true peak ceiling

### 5.4 Acoustic Identity Preservation

- At default intensity (0.7), the source sample MUST be recognizable in the output
- At intensity 0.0, output = dry sample (no processing)
- At intensity 1.0, output = full transformation (may not be recognizable — this is creative intent)
- Intensity is applied as a final wet/dry mix AFTER the gain-compensated FX chain output

---

## 6. Velocity Spectral Shaping (DOC-004 Compliance)

DOC-004: "A velocity layer is not a volume knob — must include spectral, harmonic, and saturation changes."

Rebirth generates spectrally distinct velocity layers by running the FX chain multiple times with velocity-scaled parameters:

- `velocityNorm = 0.0` (Layer 1, softest): FX parameters at their "soft" values
- `velocityNorm = 1.0` (Layer N, hardest): FX parameters at their "hard" values
- Intermediate layers: linear interpolation

Each profile's `velocityScale` map defines min/max ranges per parameter. The pipeline reads `velocityNorm` and interpolates:

```cpp
effectiveValue = baseValue + velocityNorm * (maxValue - baseValue);
```

The LUFS normalization in Step 5 equalizes perceived loudness across layers, so the spectral difference is what the player experiences — not a volume ramp.

### 6.1 Velocity Parameter Table Summary

| Profile | Vel 0 (soft) | Vel 1 (hard) |
|---------|-------------|-------------|
| OBRIX | Drive 0.2, bright reverb 0.15 mix | Drive 0.8, dark reverb 0.35 mix |
| ONSET | Attack +2dB, quiet noise -30dB, LP at 8kHz | Attack +12dB, noise -18dB, LP at 18kHz |
| OWARE | Comb feedback 0.8, bright tilt +3dB/oct | Comb feedback 0.98, dark tilt -2dB/oct |
| OPERA | Q 4.0, no tilt | Q 16.0, tilt +2dB/oct |
| OVERWASH | Diffusion 0.4, LP 12kHz, bright tilt | Diffusion 0.8, LP 6kHz, dark tilt |

---

## 7. Preview System

### 7.1 Scout Sample Preview

When Rebirth Mode is enabled, the system auto-selects one representative sample per category (shortest in each category as initial heuristic) and renders a preview:
- Single sample, single velocity layer (vel 0.7), no round-robin variation
- Full quality FX chain (not reduced quality)
- Target: <3 seconds wall time on Apple M1

### 7.2 A/B Toggle

- Preview plays as [ORIGINAL] vs [REBORN] toggle
- Keyboard shortcut for instant comparison
- Intensity slider re-triggers preview render on mouse-up (debounced 300ms)

### 7.3 Preview Communication

- UI must display: "Preview (representative sample — final results may vary)"
- Preview spinner while rendering

---

## 8. Progress, Cancel, and Error Recovery

### 8.1 Progress

- Per-sample progress: "Rendering kick_01.wav — 47 of 312 samples — 4:23 remaining"
- ETA updated live based on actual per-sample render time (not static estimate)
- Post-render summary: "312 samples reborn. 3 failed (see log). 1.2 GB output."

### 8.2 Cancel

- Cancel stops at current sample boundary (does not abort mid-sample)
- All completed renders are preserved on disk (incremental checkpointing)
- Partial results are usable
- Resume option to continue from where cancel stopped

### 8.3 Error Recovery

- Per-sample failure: log error, skip to next sample, mark in output manifest
- Engine crash isolation: each render pass runs in try/catch with watchdog timeout (3x average render time)
- 46 of 50 completed samples must not be held hostage to 4 failures

---

## 9. XOutshine Integration

### 9.1 Settings Extension

Add `RebirthSettings` to `OutshineSettings` struct:

```cpp
struct OutshineSettings {
    // ... existing fields ...
    RebirthSettings rebirth;  // NEW — Phase 1B
};
```

### 9.2 Pipeline Integration

In `XOutshine::enhance()`, when `settings.rebirth.enabled == true`:
1. For each analyzed sample, for each velocity layer:
   - Call `RebirthPipeline::process()` with the sample buffer, profile, and `velocityNorm`
   - Each round-robin variant uses a different seeded RNG (existing per-RR randomization pattern)
2. The existing velocity amplitude taper and one-pole LP filter in `enhance()` are REPLACED by the Rebirth FX chain's velocity-scaled parameters — not layered on top

### 9.3 Per-Category Profile Assignment

Auto-mode assigns profiles based on sample classification:

| Sample Category | Auto-Assigned Profile |
|----------------|----------------------|
| Kick | ONSET |
| Snare | ONSET |
| HiHat* | ONSET |
| Clap | ONSET |
| Tom | ONSET |
| Percussion | ONSET |
| Bass | OBRIX |
| Lead | OBRIX |
| Keys | OBRIX |
| Pluck | OBRIX |
| String | OBRIX |
| Vocal | OPERA |
| Woodwind | OPERA |
| Brass | OPERA |
| Pad | OVERWASH |
| FX | OBRIX (general-purpose default) |
| Loop | OBRIX (general-purpose default) |
| Unknown | OBRIX (general-purpose default) |

Producer can override per-category or per-sample.

---

## 10. UI Changes

### 10.1 Enable Rebirth Panel

The dimmed "Rebirth Mode — Phase 1B" teaser in `OutshineAutoMode.h` becomes active:

```
┌─────────────────────────────────────────────────┐
│  Rebirth Mode                                    │
│                                                  │
│  [OBRIX] [ONSET] [OWARE] [OPERA] [OVERWASH]    │  ← Profile card selector (not dropdown)
│  Harmonic   Percussive  Resonant  Harmonic  Deep │
│  Character  Crunch      Body      Shimmer   Diff │
│                                                  │
│  Intensity ─────────●──────────── 0.7            │
│  "Controls harmonic density and comb resonance"  │  ← Per-profile intensity description
│                                                  │
│  [▶ Preview]  [ORIGINAL | REBORN]                │  ← A/B toggle                        │
│                                                  │
│  ⚠ OVERWASH not recommended for drums            │  ← Material mismatch warning (contextual)
│                                                  │
│  ▸ Advanced Controls — Phase 1C                  │  ← Collapsed, greyed out
│  ▸ Expert Mode: All 73 Engines — Phase 1C        │  ← Collapsed, greyed out
└─────────────────────────────────────────────────┘
```

### 10.2 Profile Cards

Each card shows:
- Engine-inspired name (OBRIX, ONSET, etc.)
- Producer-facing label ("Harmonic Character", "Percussive Crunch", etc.)
- Material category tag (TONAL / PERCUSSIVE / AMBIENT / HARMONIC / TEXTURAL)
- Visual differentiation (accent color from engine's registered color)

### 10.3 Material Mismatch Warning

When the selected profile's `recommendedTransientMax` is exceeded by the sample's detected transient ratio, show a warning icon with tooltip explaining the mismatch.

**Warning thresholds by profile:**

| Profile | recommendedTransientMax | Warning text |
|---------|------------------------|-------------|
| OBRIX | 0.7 | "Highly percussive material may sound over-saturated" |
| ONSET | 1.0 (no warning) | — |
| OWARE | 1.0 (no warning) | — |
| OPERA | 0.5 | "Transient material will have attack blur from resonator ringing" |
| OVERWASH | 0.4 | "Percussive attacks will dissolve — this is by design" |

---

## 11. CHAOS Mode (Design Spec Only — Implementation Phase 1C)

CHAOS mode addresses Buchla's condition ("5/71 is too conservative. Needs CHAOS mode").

### 11.1 Concept

`chaosAmount` (0.0–1.0) interpolates from deterministic profile parameters toward bounded-random parameter ranges. At `chaosAmount = 0.0`, the profile runs exactly as defined. At `chaosAmount = 1.0`, every parameter in the FX chain is randomized within its velocity-scale range, plus additional ±20% jitter.

### 11.2 Architecture

CHAOS does not require new pipeline code. It is a parameter generation layer above the profile execution layer:

```cpp
effectiveValue = lerp(deterministicValue,
                      randomInRange(min * 0.8f, max * 1.2f),
                      chaosAmount);
```

### 11.3 Multi-Profile Chaining (CHAOS Level 2)

At `chaosAmount > 0.7`, CHAOS also enables random profile chaining: the system picks 2 profiles and runs them sequentially. This doubles processing time but produces results neither profile alone could create.

### 11.4 Phase 1B Notes

- `chaosAmount` field is present in `RebirthSettings` in Phase 1B
- Pipeline MUST ignore `chaosAmount` (treat as 0.0) until Phase 1C implements the CHAOS layer
- UI should expose CHAOS slider as greyed-out with "Phase 1C" label

---

## 12. K-Weighted LUFS (Replaces Approximate LUFS)

Phase 1A's LUFS measurement was simplified (spec limitation #5). Phase 1B upgrades to proper ITU-R BS.1770-4 K-weighted LUFS.

### 12.1 Algorithm

1. **Pre-filter — "head" filter:** 2nd-order high-shelf (+3.999dB at high frequencies)
2. **Pre-filter — RLB weighting:** 2nd-order high-pass (fc ≈ 38Hz)
3. **Mean-square measurement** on K-weighted signal
4. **Integration** over full sample duration
5. **Result** in LUFS (Loudness Units relative to Full Scale)

### 12.2 Filter Coefficients

K-weighting pre-filter (stage 1 — high-shelf, Fs = 48kHz reference):

```
b0 =  1.53512485958697
b1 = -2.69169618940638
b2 =  1.19839281085285
a1 = -1.69065929318241
a2 =  0.73248077421585
```

Scale coefficients for non-48kHz sample rates using the bilinear transform. Do not hardcode 48kHz coefficients for all sample rates.

### 12.3 Scope of Impact

This upgrade affects:
- Input normalization (Step 2 of Rebirth pipeline)
- Output compensation (Step 5 of Rebirth pipeline)
- The existing per-category LUFS targets in `XOutshine::normalize()` — these should also be upgraded from simplified to K-weighted measurement for consistency

---

## 13. Architect Gates

Implementation MUST NOT begin until these gates are cleared:

| Gate | Status | Description |
|------|--------|-------------|
| GATE-01 | CLEARED | Velocity spectral strategy defined per profile (Section 6 + per-profile tables in Section 4) |
| GATE-02 | OPEN | Python CLI deprecation note published (update `xoutshine.py` docstring) |
| GATE-03 | OPEN | Confirm `RebirthDSP.h` wraps existing DSP modules (implementation review before writing new code) |
| GATE-04 | CLEARED | CHAOS mode design spec written (Section 11) |
| GATE-05 | OPEN | `RebirthProfiles.h` data structure reviewed before pipeline implementation begins |

**GATE-02 action:** Add the following to the top-level docstring of `Tools/xoutshine.py`:

```python
"""
...
NOTE: Prototype implementation. Production Rebirth Mode is in the XOceanus desktop app.
The Python CLI Rebirth profiles (OBESE, OUROBOROS, OPAL, ORIGAMI, OVERDUB) are deprecated
as of Phase 1B (2026-03-25) and will not receive further updates.
...
"""
```

---

## 14. Known Limitations

| # | Limitation | Phase |
|---|-----------|-------|
| 1 | CHAOS mode is design-only, not implemented | Phase 1C |
| 2 | Expert mode (all 73 engines) not available | Phase 1C |
| 3 | Per-parameter advanced controls not exposed | Phase 1C |
| 4 | Session save/load for Rebirth settings | Phase 1C |
| 5 | XPN upgrade pipeline (import → re-process → re-export) | Phase 1B (separate task) |
| 6 | FormantResonator uses FFT peak detection, not LPC — approximate but fast | By design |
| 7 | Profiles are "inspired by" engines as of 2026-03-25, not live extractions — may diverge over time | By design |

---

## 15. Spec Lineage

- **QDD CI Run:** 2026-03-25, 5-level audit, 16+ reviewers
- **Original design:** Coupling-based audio routing through live SynthEngine instances (BLOCKED 12-0)
- **Approved architecture:** Option B — Engine-Inspired FX Chain (BLESSED 8-0 by Ghost Council)
- **User decisions resolved:**
  - 1a: FFT formant detection for OPERA (not LPC)
  - 2a: CHAOS design only in Phase 1B, implementation in Phase 1C
  - 3a: Deprecate Python CLI Rebirth
  - 4a: All 5 profiles (OBRIX, ONSET, OWARE, OPERA, OVERWASH)
  - 5a: Per-profile velocity FX parameter scaling
- **Ghost Council:** Moog, Buchla, Chowning, Mathews, Neve, Kakehashi, Ciani, Kurzweil
- **Architect conditions:** 5 gates, GATE-01 and GATE-04 pre-cleared by this spec; GATE-02, GATE-03, GATE-05 open
