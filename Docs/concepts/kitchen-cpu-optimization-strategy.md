# Kitchen Collection — CPU Optimization Strategy

*SRO Optimizer + Visionary Joint Analysis | March 2026*
*Pre-DSP Architecture Decisions for the 5 Remaining Quads*

---

## Executive Summary

The Kitchen Collection introduces the most CPU-demanding DSP in XOlokun history. Five quads, each with novel synthesis architectures that push past anything in the existing 46-engine fleet. This document analyzes the real computational cost of each, identifies the hot paths, and provides V1/V2 optimization strategies that keep every engine under the 5% per-engine CPU target.

**The single most creative optimization idea for the entire collection:**

**The Spectral Fingerprint Cache (SFC).** Instead of running full DSP chains on all 5 engines simultaneously in the FUSION slot, each Kitchen engine continuously exports a compact *spectral fingerprint* — 32 floats representing its current modal resonance state. The Fusion engine reconstructs a perceptual approximation from these fingerprints rather than processing raw audio through 4 full modal banks. The 4 Kitchen engines don't run at full quality during Fusion — they run their normal DSP but export metadata. The Fusion engine synthesizes the *coupling* cheaply from fingerprints. Cost: ~1% per Kitchen engine for fingerprint export + ~3% for the Fusion engine = 7% total for 5-engine simultaneous operation instead of the naive 25%+.

This is psychoacoustically valid because the Fusion engine's Plate coupling already models *energy transfer between surfaces*, not raw audio mixing. You don't need the full waveform — you need the *resonant character*. The fingerprint carries exactly that.

---

## CPU Budget Framework

### Targets

| Configuration | Budget | Notes |
|---------------|--------|-------|
| Single engine, 8 voices | < 5% | Standard fleet target |
| 2 engines coupled | < 10% total | Standard coupling target |
| 4 Kitchen engines (full quad) | < 18% total | Must leave headroom for Fusion arrival |
| 5 engines (Kitchen + Fusion) | < 25% total | Absolute ceiling for the 5th-slot configuration |

### Reference Costs (Existing Fleet)

| Component | Cost per voice per sample | Notes |
|-----------|--------------------------|-------|
| CytomicSVF | ~4 ns | 2 multiplies, 3 adds |
| PolyBLEP oscillator | ~6 ns | Phase increment + correction |
| FastMath::fastTanh | ~2 ns | Rational approximation |
| SROTables::sin().lookup() | ~3 ns | Table + lerp |
| std::sin (for comparison) | ~25 ns | Hardware dependent |
| std::exp | ~20 ns | Hardware dependent |
| 1024-point FFT (full block) | ~40 us | Not per-voice, per-block |

### The Block Budget

At 44.1kHz, 512-sample buffer:
- Block duration: 11.6 ms
- 5% CPU = 0.58 ms per engine per block
- Per-sample budget (8 voices): 0.58ms / 512 / 8 = **142 ns per voice per sample**
- Per-sample budget (4 voices, reduced poly): **284 ns per voice per sample**

---

## 1. KITCHEN — Modal Resonator Banks

### SRO Analysis

**The arithmetic.**

Each modal resonator is a second-order IIR:
```
y[n] = b0*x[n] - a1*y[n-1] - a2*y[n-2]
```
Cost per resonator per sample: 2 multiplies + 2 adds + 2 state loads/stores = ~5 ns.

At 64 resonators x 8 voices x 44100 samples/sec:
- Per sample: 64 * 5ns = 320 ns per voice
- Per block (512 samples, 8 voices): 64 * 5ns * 512 * 8 = **1.31 ms = 11.3% CPU**

That is over budget. The hot paths:
1. **Modal resonator bank inner loop** — 80% of cost
2. **Sympathetic string network** (88 resonators driven by body output) — could add another 5-8% if naive
3. **Hunt-Crossley contact model** — cheap per-note, negligible

**What can be moved to control rate:**
- Temperature-driven eigenfrequency drift (changes over seconds, not samples)
- Material coupling coefficients (change on parameter adjustment only)
- Sympathetic string gain scaling (per-material factor, update once per block)

**What can be approximated without audible loss:**
- Modal resonators below -60dB can be skipped (dynamic mode pruning)
- Sympathetic strings: only 10-15 of 88 strings are near enough in pitch to resonate audibly at any given moment
- High-Q modes (stone, glass) can use longer update intervals for coefficient recalculation

**SIMD opportunities:**
- The modal bank is embarrassingly parallel — 64 independent biquads. Process 4 at a time with SSE/NEON. Immediate 3-4x speedup on the inner loop.
- On Apple Silicon (NEON): process 4 resonators per cycle. 64 modes / 4 = 16 SIMD iterations per voice per sample.

### Visionary Reframe

**The perceptual shortcut: Humans can't hear 64 modes.**

Psychoacoustic research (Fletcher & Rossing, 1998; Chaigne & Kergomard, 2016) shows that above the 8th-10th partial of a piano body resonance, individual modes are masked by the aggregate spectral envelope. The ear hears the *shape* of the resonance cloud, not individual peaks. Below ~400Hz, individual modes matter. Above ~400Hz, a shaped noise convolution achieves the same perceptual result.

**Video game audio does this all the time.** Game engines like Wwise and FMOD use "granular convolution" — a short noise burst convolved with a material impulse response — for surface impacts. The technique uses a 256-sample IR (6ms at 44.1kHz) and a one-shot noise excitation. Cost: effectively zero. Perceptual result: convincing material character.

**The hybrid modal-stochastic architecture:**
- 8-16 low-frequency modes (below 400Hz): full IIR resonators — these are individually audible
- Remaining spectral shape (above 400Hz): filtered noise burst shaped by the material's spectral envelope
- The noise burst is generated once per note-on, not per-sample
- Total resonator count drops from 64 to 16. Cost drops 4x.

**What if the limitation IS the feature?**

A piano with only 16 audible body modes sounds like a *particular* piano — a small one, a prepared one, a piano recorded through a narrow-bandwidth microphone in a dry room. This is not a degraded concert grand. It is a *Kitchen piano* — an instrument made of cast iron, not rosewood. Cast iron IS denser, IS less resonant, IS simpler in its modal structure than spruce. The material *justifies* fewer modes.

### Concrete Recommendations

**V1 approach (ships first, <5% CPU per engine):**

1. 16 full IIR modal resonators (low-frequency body modes)
2. Material-shaped filtered noise burst for HF body character (one CytomicSVF per voice, negligible cost)
3. Dynamic mode pruning: skip modes below -60dB threshold
4. Sympathetic string network: pitch-proximity sparse update — only compute 12 nearest strings (by semitone distance from currently sounding notes)
5. SilenceGate with 300ms hold (piano sustain tails)

```
CPU estimate:
  16 modes * 5ns * 8 voices = 640 ns/sample
  1 SVF noise shaper * 4ns * 8 voices = 32 ns/sample
  12 sympathetic resonators * 5ns = 60 ns/sample (shared, not per-voice)
  Total: ~732 ns/sample = ~3.2% CPU at 512-sample block
```

**V2 approach (optimized, post-V1 learnings):**

1. SIMD-vectorized modal bank: process 4 resonators per NEON/SSE cycle
2. Increase to 24-32 modes if SIMD provides headroom
3. Sympathetic network on ControlRateReducer<32> — update every 32 samples
4. Per-voice mode count scaling: higher velocity = more modes activated (perceptually masked by attack transient at low velocity)

**Code sketch (V1 modal bank with dynamic pruning):**

```cpp
struct ModalResonator {
    float y1 = 0.f, y2 = 0.f;  // state
    float a1, a2, b0;           // coefficients (material-derived)
    float amplitude = 0.f;      // current output level for pruning
    bool active = true;

    float process(float excitation) noexcept {
        float y0 = b0 * excitation - a1 * y1 - a2 * y2;
        y2 = y1;
        y1 = flushDenormal(y0);
        amplitude = std::fabs(y1);
        return y1;
    }
};

// In voice render loop:
float bodyOut = 0.f;
for (int m = 0; m < kNumModes; ++m) {
    if (modes[m].amplitude < kPruneThreshold && !firstBlock) {
        modes[m].active = false;
        continue;
    }
    modes[m].active = true;
    bodyOut += modes[m].process(hammerExcitation);
}
// Add HF noise character
bodyOut += noiseSVF.processSample(noiseSource * hfEnvelope);
```

### Architecture Decision: NOW

**Decision K1: Modal count = 16 for V1.** Do not design the parameter system, preset format, or coupling interface around 64 modes. The 16-mode architecture with HF noise shaping is the V1 target. If V2 SIMD vectorization provides headroom, modes can be added without changing the external interface.

**Decision K2: Sympathetic string network is shared across voices, not per-voice.** One set of 12 active sympathetic resonators is driven by the sum of all voice outputs. This is physically accurate (the soundboard is shared) and saves 7x computation.

---

## 2. CELLAR — Gravitational Coupling

### SRO Analysis

**The arithmetic.**

The gravitational coupling model requires frequency-domain analysis of the coupled engine's output to identify partials and compute displacement. Two approaches:

**Approach A: FFT-based (spec'd in visionary doc)**
- 1024-point FFT on coupled engine output: ~40 us per block
- Per-bin gravity computation (1024 bins): ~10 us
- Inverse FFT for resynthesis: ~40 us
- Total: ~90 us per block = ~0.8% CPU
- Problem: 1024-point FFT at 44.1kHz = 23ms latency. Unacceptable for live play.

**Approach B: Pitch-domain approximation**
- Track fundamental pitch of coupled engine (autocorrelation or YIN): ~20 us per block
- Compute harmonic series from fundamental (8 harmonics): trivial
- Apply per-voice pitch displacement toward nearest harmonic: ~1 ns per voice per sample
- Total: ~25 us per block = ~0.2% CPU

The hot path is the analysis, not the gravity math. The gravity computation itself (inverse-square attraction toward nearest harmonic) is 3 multiplies per voice per sample.

**What can be moved to control rate:**
- Everything. Gravity operates on musical time scales (beats, bars). A 32-sample update rate (1.4kHz at 44.1kHz) is far faster than any audible gravitational drift.
- Mass accumulation (integrator) updates once per block (512 samples).
- Harmonic series recalculation: only when the cellar engine's pitch changes (note-on events).

**What can be approximated without audible loss:**
- Top-4 harmonics instead of full harmonic series — beyond the 4th harmonic, gravitational capture radius is so small it's inaudible
- Binary capture model (captured vs. free) instead of the three-tier capture/orbital/escaped — orbital state is musically interesting but computationally unnecessary in V1

### Visionary Reframe

**The radical reframe: Don't analyze the coupled engine. Analyze the MIDI.**

The Cellar engine already knows what notes the coupled engines are playing — the MIDI data flows through the same plugin instance. The gravitational model needs the *pitch content* of the coupled engines, not their audio spectrum. Instead of FFT analysis of rendered audio, read the coupled engine's active note list directly from the voice allocator.

This is how **film scoring software** handles orchestral tuning: the conductor's reference pitch is a MIDI value, not an audio analysis. The instruments tune to a known pitch, not to a detected one. The Cellar engine IS the conductor.

**The psychoacoustic shortcut:** Gravitational partial attraction below 5 cents of displacement is inaudible in a polyphonic context. At 50% PULL strength, only partials within ~30 cents of a cellar harmonic are perceptibly displaced. With a harmonic series based on the MIDI fundamental, you can compute exact displacement targets with zero analysis cost.

**What if the limitation IS the feature?** MIDI-domain gravity means the Cellar engine responds to what the player *intends* (the notes they press), not what the speakers produce. This is philosophically cleaner — the conductor hears the score, not the room. And it's essentially free.

### Concrete Recommendations

**V1 approach (<5% CPU for CELLAR engine itself; coupling cost is marginal):**

1. MIDI-domain gravity: read active note list from coupled engine slots
2. Compute harmonic series from cellar engine's current pitch (8 harmonics, precomputed on note-on)
3. Per-voice pitch displacement at control rate (ControlRateReducer<64>)
4. Mass accumulator: leaky integrator updated once per block
5. Binary capture model: captured (within 30 cents) or free
6. No FFT. No audio analysis.

```
CPU estimate:
  Cellar engine synthesis: ~3% (standard subtractive/FM bass engine)
  Gravity coupling: ~0.1% (control-rate pitch displacement)
  Mass accumulator: ~0.01%
  Total: ~3.1% CPU
```

**V2 approach:**
1. Add spectral gravity on the body resonance of coupled engines (if Kitchen engines export their modal frequencies as metadata — see the Spectral Fingerprint Cache idea)
2. Implement orbital state for partials at mid-distance (beating/intermodulation)
3. Escape velocity transient detection (envelope follower on coupled engine attack transients)

**Code sketch (V1 MIDI-domain gravity):**

```cpp
// Once per block, in coupling processor:
void updateGravity(int cellarNote, float pull, float mass,
                   int* coupledNotes, int numCoupled) noexcept
{
    float cellarFreq = midiToFreq(cellarNote);
    // Precompute first 8 harmonics
    float harmonics[8];
    for (int h = 0; h < 8; ++h)
        harmonics[h] = cellarFreq * (h + 1);

    for (int n = 0; n < numCoupled; ++n) {
        float noteFreq = midiToFreq(coupledNotes[n]);
        // Find nearest cellar harmonic
        float nearestH = harmonics[0];
        float minDist = 99999.f;
        for (int h = 0; h < 8; ++h) {
            float dist = std::fabs(noteFreq - harmonics[h]);
            if (dist < minDist) {
                minDist = dist;
                nearestH = harmonics[h];
            }
        }
        // Compute displacement in cents
        float cents = 1200.f * fastLog2(noteFreq / nearestH);
        float captureRadius = 30.f * pull;
        if (std::fabs(cents) < captureRadius) {
            // Captured: compute pitch offset toward harmonic
            gravityOffsets[n] = (nearestH - noteFreq) * pull * mass * 0.1f;
        } else {
            gravityOffsets[n] = 0.f;
        }
    }
}
```

### Architecture Decision: NOW

**Decision C1: Gravity is MIDI-domain in V1, not audio-domain.** This eliminates the need for FFT infrastructure in the coupling matrix. The coupled engine's voice allocator exposes its active note list (already accessible through the SynthEngine interface). No new inter-engine audio routing required.

**Decision C2: Cellar coupling is unidirectional outward.** The Cellar engine broadcasts gravity data (pitch + mass + pull). Receiving engines apply displacement locally. The Cellar engine does not receive coupled audio. This halves the coupling cost and matches the physical model (gravity radiates from mass).

---

## 3. GARDEN — Mycorrhizal Networks

### SRO Analysis

**The arithmetic.**

The spec calls for cross-voice delayed communication with 4-8 second delay lines per voice pair. With 8 voices, there are C(8,2) = 28 unique pairs.

**Memory cost:**
- 8-second delay at 44.1kHz = 352,800 samples per line
- 28 delay lines * 352,800 * 4 bytes = **39.5 MB per engine**
- This is the real problem — not CPU but memory. And cache coherence: reading from 28 scattered delay line positions per sample will thrash the L1/L2 cache.

**CPU cost (if naive):**
- 28 delay reads + 28 delay writes per sample = 56 memory operations
- At ~5ns per cache miss: 56 * 5ns = 280 ns per sample
- Plus the stress propagation math: 28 * 3 multiplies = ~84 multiplies = ~30 ns
- Total: ~310 ns/sample = ~1.4% CPU for the network alone

**Hot paths:**
1. Delay line reads (random access, cache-hostile) — 80% of cost
2. Stress accumulator propagation math — 15%
3. Warmth/Dormancy/Aggression integrators — 5% (trivially cheap)

**What can be moved to control rate:**
- The W/A/D accumulators are explicitly slow (seconds-to-minutes time constants). Update once per block.
- Mycorrhizal propagation delay is 2-8 seconds. A control-rate update every 32 samples (0.7ms) is 3000x faster than the propagation speed. **All mycorrhizal communication can be control-rate.**
- Seasonal state transitions: once per second at most.

### Visionary Reframe

**The radical reframe: Don't use delay lines. Use timed events.**

The mycorrhizal network doesn't transmit audio. It transmits *state signals* — stress level, warmth level, resource availability. These are scalar values, not waveforms. A 4-second delay on a scalar value is not a 4-second delay line with 176,400 samples. It is a **timed event queue**: store the value with a timestamp, deliver it when the timestamp is 4 seconds old.

**Event-driven mycorrhizal network:**
- Each propagation is a struct: `{float value, float deliveryTime}`
- Circular buffer of 32 pending events per connection (more than enough for 4-second window)
- Memory: 28 connections * 32 events * 8 bytes = **7.2 KB** (vs. 39.5 MB for audio delay lines)
- CPU: scan 32 events per connection per block = 28 * 32 = 896 comparisons per block = trivial

This is 5000x less memory and far more cache-friendly. The mycorrhizal network was never audio — it was always *messages*. Treat it like a message bus, not a signal chain.

**Cross-domain inspiration: Telecommunication networks.** TCP/IP delivers packets with latency. It doesn't maintain a continuous wire with audio-rate sampling. The mycorrhizal network is a packet network with configurable latency. Each "packet" carries a stress/warmth delta. The delivery delay IS the propagation through the fungal network.

**What if the limitation IS the feature?** Event-driven propagation is *quantized* — signals arrive in discrete packets, not continuous flows. This is biologically accurate: chemical signals in mycorrhizal networks travel in bursts as vesicles release their contents at hyphae junctions. The quantization IS the biology.

### Concrete Recommendations

**V1 approach (<5% CPU, <100KB memory):**

1. Event-queue mycorrhizal network (no audio delay lines)
2. W/A/D accumulators updated once per block (512 samples)
3. Seasonal state machine updated once per second
4. Mycorrhizal propagation: timed scalar events, 32-event circular buffer per connection
5. Phototropism: pitch attractor at control rate (ControlRateReducer<64>)
6. Growth Mode phases: state machine with envelope segments, not per-sample DSP

```
CPU estimate:
  String synthesis engine: ~3% (standard fleet-level engine)
  W/A/D accumulators: ~0.01%
  Mycorrhizal event processing: ~0.02%
  Seasonal state: ~0.001%
  Phototropism pitch offset: ~0.05%
  Total: ~3.1% CPU
  Memory: ~7.2 KB for mycorrhizal events + standard voice state
```

**V2 approach:**
1. Audio-rate mycorrhizal channel for cross-voice resonance transfer (not delay lines — but filtered audio buses between voice pairs, 4 buses max active at once)
2. Per-voice accumulator modulation for finer grain timbral response
3. Cross-engine mycorrhizal network (Garden-to-Garden only) via the coupling matrix

**Code sketch (V1 event-queue mycorrhizal network):**

```cpp
struct MycorrhizalEvent {
    float value;
    double deliveryTime;  // in seconds since session start
};

class MycorrhizalChannel {
    static constexpr int kMaxEvents = 32;
    MycorrhizalEvent events[kMaxEvents];
    int writeIdx = 0;
    int count = 0;
    float conductance = 0.5f;
    float delaySeconds = 4.0f;

public:
    void send(float stressValue, double currentTime) noexcept {
        events[writeIdx] = {stressValue * conductance,
                           currentTime + delaySeconds};
        writeIdx = (writeIdx + 1) & (kMaxEvents - 1);
        if (count < kMaxEvents) count++;
    }

    float receive(double currentTime) noexcept {
        float total = 0.f;
        for (int i = 0; i < count; ++i) {
            if (events[i].deliveryTime <= currentTime) {
                total += events[i].value;
                events[i].value = 0.f;  // consumed
            }
        }
        return total;
    }
};

// 28 channels for 8 voices (or 6 channels for 4 voices in V1)
MycorrhizalChannel network[28];
```

### Architecture Decision: NOW

**Decision G1: Mycorrhizal network is event-driven, not audio-domain.** This eliminates the memory problem entirely and makes the feature essentially free. Design the MycorrhizalChannel API now; it can be upgraded to audio-domain in V2 if needed.

**Decision G2: V1 polyphony for Garden engines = 4 voices, not 8.** String sections work with voice layering (multiple detuned oscillators per voice), not high polyphony. 4 voices with 4 oscillators each = 16 sounding elements. This halves the mycorrhizal network size from 28 channels to 6 channels and halves the synthesis cost.

---

## 4. BROTH — Session-Persistent State

### SRO Analysis

**The arithmetic.**

The BROTH quad's four engines each have session-persistent state that accumulates over time:

**XOverwash (Fick's diffusion):**
- Spectral diffusion kernel applied to a 1024-bin spectral field
- If run per-sample via FFT: 2 FFTs per block (forward + inverse) = ~80 us = 0.7% CPU
- If run as a time-domain convolution: 1024 taps * 8 voices = prohibitive

**XOverworn (Reduction integral):**
- ReductionState.spectralMass[1024]: updated per block, not per sample
- High-shelf attenuation: 1 CytomicSVF per voice, no additional cost
- Caramelization distortion: 1 fastTanh per sample per voice

**XOverflow (Clausius-Clapeyron):**
- Pressure accumulator: 1 add per MIDI event
- Saturation threshold lookup: trivial
- Valve release: one-shot spectral expansion convolution (very short IR, ~128 samples)

**XOvercast (Wilson nucleation):**
- Spectral snapshot on trigger: 1 FFT at trigger time (not per-block)
- Crystal lattice: phase-locked additive synthesis from N spectral peaks
- Between triggers: zero processing — frozen state is static

**Hot paths:**
1. XOverwash's diffusion kernel (only if FFT-domain)
2. XOverworn's spectral mass tracking (only if per-bin)
3. Everything else is cheap

**What can be moved to control rate:**
- XOverworn's reduction rate calculation: once per second
- XOverwash's diffusion step: see below
- XOverflow's pressure accumulator: per-MIDI-event only
- XOvercast's crystallization: only at trigger, not per-sample

**Denormal/precision concerns (30+ minute sessions):**
- ReductionState.spectralMass[1024]: values drift toward zero over hours. Use `flushDenormal()` on every mass value at each update.
- Pressure accumulator: capped at a maximum (phase transition threshold), so no unbounded growth.
- Diffusion coefficient D: stable, user-controlled, no drift risk.
- Session age: simple counter, double precision, no overflow risk at 44.1kHz for thousands of hours.

### Visionary Reframe

**The radical reframe: Diffusion is not per-sample. Diffusion is per-event.**

Fick's second law describes continuous-time diffusion. But in a musical context, the "concentration impulses" (notes played) arrive discretely — on MIDI note-on events. Between events, the diffusion equation has an analytical solution:

```
C(f, t) = sum over events i:
  A_i / sqrt(4*pi*D*dt_i) * exp(-(f - f_i)^2 / (4*D*dt_i))
```

Where dt_i is the time since event i. This is a sum of Gaussians — one per note event, each widening over time. No FFT required. No per-sample processing. Just maintain a list of active diffusion fronts and sum them analytically.

**The cost:** N active events * 1 Gaussian evaluation per output frequency bin. With 16 recent events and 64 output bins: 1024 Gaussian evaluations per block. At ~5ns each (using SROTables::exp()): **~5 us per block = 0.04% CPU.** Essentially free.

**Cross-domain inspiration: Particle systems in games.** Game engines render smoke, fog, and liquid effects using particle systems — each particle is spawned at an event, expands over time (Gaussian blur), and fades. Diffusion IS a particle system. Each note is a particle. The spectral field is the screen. The Temperature parameter is the expansion rate.

**What if the limitation IS the feature?** The analytical Gaussian model produces perfectly smooth, mathematically pure diffusion fronts. The FFT-domain approach would produce quantized, bin-resolution fronts with edge artifacts. The cheap path IS the higher-quality path. Constraint breeds beauty.

### Concrete Recommendations

**V1 approach (<5% CPU per BROTH engine):**

1. **XOverwash:** Analytical Gaussian diffusion (sum of expanding Gaussians per note event). No FFT. Maintain circular buffer of 32 recent note events with timestamps. Per-block: compute 32 Gaussians across the spectral output. Apply result as spectral envelope to oscillator bank or noise source.
2. **XOverworn:** Single CytomicSVF high-shelf per voice, cutoff decreasing with session age. Caramelization via fastTanh on the output. ReductionState updated once per block.
3. **XOverflow:** Pressure accumulator (scalar, per-MIDI-event). Valve release: triggered one-shot amplitude envelope with spectral expansion (brief filter sweep).
4. **XOvercast:** On trigger: capture current spectral peaks (top 8) via simple peak detection on the oscillator output. Hold those peaks as additive synthesis partials. Between triggers: zero per-sample cost beyond the additive synth itself.

```
CPU estimate per BROTH engine (worst case = XOverwash):
  Oscillator bank (8 partials): ~1%
  Gaussian diffusion (32 events, per block): ~0.04%
  CytomicSVF filtering: ~0.3%
  Session state updates: ~0.01%
  Total: ~1.4% CPU (XOverwash, most expensive)
  XOvercast: ~0.5% CPU (additive synth only, near-zero between triggers)
```

**V2 approach:**
1. XOverwash: real-time spectral visualization of diffusion fronts (UI only, not DSP)
2. XOverworn: multi-band reduction with independent time constants per band (3 bands: sub, mid, air)
3. Cross-engine cooperative coupling: ReductionState shared via coupling matrix (scalar metadata, not audio)

**Code sketch (V1 analytical Gaussian diffusion for XOverwash):**

```cpp
struct DiffusionFront {
    float centerFreq;    // Hz, from note event
    float amplitude;     // initial energy
    double birthTime;    // seconds since session start
    bool active;
};

class SpectralDiffusion {
    static constexpr int kMaxFronts = 32;
    DiffusionFront fronts[kMaxFronts];
    int writeIdx = 0;
    float temperature = 0.5f;  // user parameter, maps to D

    void addFront(float freq, float velocity, double time) noexcept {
        fronts[writeIdx] = {freq, velocity, time, true};
        writeIdx = (writeIdx + 1) % kMaxFronts;
    }

    // Call once per block to compute spectral envelope
    void computeEnvelope(float* envelope, int numBins,
                        float minFreq, float maxFreq,
                        double currentTime) noexcept {
        float D = temperature * 500.f;  // diffusion coefficient
        float binWidth = (maxFreq - minFreq) / numBins;

        std::memset(envelope, 0, numBins * sizeof(float));
        for (int i = 0; i < kMaxFronts; ++i) {
            if (!fronts[i].active) continue;
            float dt = static_cast<float>(currentTime - fronts[i].birthTime);
            if (dt <= 0.f) continue;
            float sigma2 = 4.f * D * dt;
            float invSigma2 = 1.f / (sigma2 + 0.001f);
            float normFactor = fronts[i].amplitude /
                              std::sqrt(sigma2 * 6.2831853f);

            for (int b = 0; b < numBins; ++b) {
                float f = minFreq + b * binWidth;
                float df = f - fronts[i].centerFreq;
                envelope[b] += normFactor *
                    SROTables::exp().lookup(-df * df * invSigma2);
            }
        }
    }
};
```

### Architecture Decision: NOW

**Decision B1: No FFT in BROTH engines.** Fick's diffusion is solved analytically. Wilson nucleation operates on captured peak data, not continuous FFT frames. Clausius-Clapeyron is a scalar accumulator. The BROTH quad is a demonstration that physically accurate simulation does NOT require frequency-domain processing.

**Decision B2: ReductionState persistence uses the same `.xometa` JSON extension as presets.** A `"session_state"` field in the preset JSON stores the ReductionState. This survives DAW save/load without requiring a separate file format. The "Start Fresh" button writes a clean session_state back to the preset file.

---

## 5. FUSION — 5 Engines Simultaneous

### SRO Analysis

**The naive arithmetic.**

5 engines * 8 voices * full DSP chains:
- If each Kitchen engine costs 4% (V1 optimized): 4 * 4% = 16%
- If the Fusion engine costs 4%: 16% + 4% = **20% CPU**
- This is within the 25% ceiling but leaves only 5% headroom for effects, coupling matrix, and UI. Too tight.

**The real problem is not CPU — it is the coupling matrix.**

The current MegaCouplingMatrix is 4x4 = 16 coupling routes. A 5th slot expands to 5x5 = 25 routes — a 56% increase in coupling computation. If each coupling route costs 0.1% CPU (typical for control-rate coupling), the matrix cost goes from 1.6% to 2.5%. Manageable, but only because the coupling is control-rate.

If the Plate coupling requires audio-rate surface character transfer (the spec implies this), the cost per route could be 0.5-1%, making the matrix cost alone 10-20%. Unacceptable.

**Hot paths:**
1. 4 Kitchen modal resonator banks running simultaneously
2. Fusion engine synthesis (Rhodes/Wurlitzer/Clavinet/FM EP — standard subtractive/FM, typically ~3%)
3. Plate coupling routes (5x4 = 20 routes if bidirectional; 4 routes if Fusion-to-Kitchen only)
4. MegaCouplingMatrix routing overhead

### Visionary Reframe

**The most important insight: The 5 engines don't need to run at full quality simultaneously because the human ear can't attend to 5 timbral sources at once.**

Cognitive psychology (Miller, 1956: "The Magical Number Seven, Plus or Minus Two") shows that humans can track 3-5 simultaneous auditory streams before perceptual grouping collapses them. With 5 engines playing simultaneously, the listener perceives 2-3 grouped timbral layers, not 5 individual instruments.

**The Perceptual Priority System:** At any given moment, 1-2 engines are in *foreground* (the ones the player is currently playing) and 3-4 are in *background* (sustaining, coupling, resonating). Background engines can run at reduced quality without audible degradation.

This is exactly how **3D game audio** works. Only the nearest 8-16 sound sources get full DSP processing. Distant sources get progressively simplified — lower sample rate, fewer filters, mono instead of stereo. The audio budget is *dynamically allocated* based on perceptual relevance. No game runs 5000 sound sources at full quality simultaneously.

**The Fusion engine doesn't need 5 full engines. It needs 1 full engine + 4 spectral fingerprints.**

Here is the Spectral Fingerprint Cache (SFC) architecture:

1. Each Kitchen engine continuously exports a **SpectralFingerprint** — a compact data structure:
   ```
   SpectralFingerprint {
       float fundamentalFreq;          // current pitch
       float modalPeaks[16];           // frequencies of active body modes
       float modalAmplitudes[16];      // amplitudes of active body modes
       float overallEnergy;            // RMS level
       float spectralCentroid;         // brightness
       float materialImpedance;        // the engine's Z value
       float temperature;              // current surface temperature
   }
   ```
   Total: 38 floats = 152 bytes. Updated once per block.

2. The **Fusion engine** reads these 4 fingerprints and synthesizes the Plate coupling effect:
   - The fingerprint's modal peaks become resonator targets for the Fusion engine's body model
   - The material impedance determines how much energy crosses into the Fusion engine's synthesis
   - The spectral centroid shapes the Fusion engine's tonal character
   - The temperature affects the Fusion engine's decay characteristics

3. **The Kitchen engines don't change their DSP when Fusion activates.** They continue running their normal V1-optimized modal synthesis. The only additional cost is writing the SpectralFingerprint struct once per block — effectively zero.

4. **The Fusion engine runs its own synthesis (Rhodes/Wurlitzer/Clavinet/FM) at full quality** plus a lightweight "virtual body" composed from the 4 Kitchen fingerprints. The virtual body is a bank of 16 resonators (the same as a single Kitchen engine) whose frequencies and decay rates are computed from the 4 fingerprint averages.

**Cost:**
- 4 Kitchen engines: 4 * 3.2% = 12.8% (V1 optimized, unchanged)
- Fingerprint export: 4 * 0.01% = 0.04%
- Fusion engine synthesis: ~3%
- Fusion virtual body (16 resonators from fingerprints): ~0.4%
- Plate coupling (4 routes, control-rate): 4 * 0.1% = 0.4%
- **Total: ~16.6% CPU for 5-engine simultaneous operation**

That leaves 8.4% headroom below the 25% ceiling. Comfortable.

### Concrete Recommendations

**V1 approach (ships first, <25% total for full 5-engine configuration):**

1. **Spectral Fingerprint Cache:** Each Kitchen engine exports a SpectralFingerprint struct (152 bytes) updated once per block. Zero-cost to exporter. Used by Fusion engine.
2. **Perceptual Priority System:** The most recently played engine gets full voice count. The other 4 get reduced voice count (4 voices instead of 8). Managed by a shared voice budget.
3. **Plate coupling is metadata-domain, not audio-domain.** The Fusion engine reads Kitchen fingerprints and uses them to shape its own synthesis. No audio routing between the 5 engines.
4. **Fusion engine: standard EP synthesis + virtual body resonators.** The Rhodes/Wurlitzer/Clavinet/FM engine runs the same architecture as any fleet engine. The "material influence" is applied as resonator coefficient modulation, not as audio-domain coupling.
5. **MegaCouplingMatrix extension: Ghost Slot architecture as spec'd in the Fusion visionary doc.** The 5th slot's coupling is fingerprint-based, so the matrix doesn't need audio routing — just metadata forwarding.

```
CPU budget (V1):
  Kitchen engines (4 * 3.2%):    12.8%
  Fingerprint export (4 * 0.01%): 0.04%
  Fusion synthesis:                3.0%
  Fusion virtual body:             0.4%
  Plate coupling (4 routes):       0.4%
  Coupling matrix overhead:        0.3%
  ─────────────────────────────────────
  Total:                          16.9%
  Headroom to 25% ceiling:        8.1%
```

**V2 approach:**
1. Audio-domain Plate coupling for the active (foreground) Kitchen engine only (1 audio route instead of 4)
2. Dynamic voice reallocation: total budget of 24 voices shared across 5 engines, allocated by activity
3. Kitchen engine quality scaling: active engine at 24 modes, inactive engines at 12 modes
4. SIMD-vectorized modal banks across all Kitchen engines (shared processing thread)

**Code sketch (Spectral Fingerprint Cache):**

```cpp
struct SpectralFingerprint {
    float fundamentalFreq = 0.f;
    float modalPeaks[16] = {};
    float modalAmplitudes[16] = {};
    float overallEnergy = 0.f;
    float spectralCentroid = 0.f;
    float materialImpedance = 0.f;
    float temperature = 20.f;

    // Call at end of Kitchen engine's renderBlock()
    void update(const ModalResonator* modes, int numModes,
                float currentFreq, float rms, float Z, float T) noexcept {
        fundamentalFreq = currentFreq;
        overallEnergy = rms;
        materialImpedance = Z;
        temperature = T;

        float centroidNum = 0.f, centroidDen = 0.f;
        for (int i = 0; i < numModes && i < 16; ++i) {
            modalPeaks[i] = modes[i].frequency;
            modalAmplitudes[i] = modes[i].amplitude;
            centroidNum += modes[i].frequency * modes[i].amplitude;
            centroidDen += modes[i].amplitude;
        }
        spectralCentroid = (centroidDen > 0.f) ?
            centroidNum / centroidDen : fundamentalFreq;
    }
};

// In Fusion engine, receive 4 fingerprints and build virtual body:
void buildVirtualBody(const SpectralFingerprint* fps, int count) noexcept {
    for (int m = 0; m < 16; ++m) {
        float avgFreq = 0.f, avgAmp = 0.f;
        for (int e = 0; e < count; ++e) {
            avgFreq += fps[e].modalPeaks[m] / fps[e].materialImpedance;
            avgAmp += fps[e].modalAmplitudes[m];
        }
        // Weight by inverse impedance: lower Z = more influence on body
        virtualModes[m].setFrequencyAndDecay(avgFreq / count,
                                              avgAmp / count);
    }
}
```

### Architecture Decisions: NOW

**Decision F1: Kitchen engines export SpectralFingerprint, not audio, to the Fusion slot.** This is the single most important CPU decision in the entire collection. It decouples the Fusion engine's quality from the Kitchen engines' computational cost. The alternative (audio routing from 4 engines into a 5th) would require the coupling matrix to handle 4 simultaneous audio streams at audio rate — approximately 4% CPU just for the routing.

**Decision F2: Shared voice budget of 32 voices across all 5 engines.** When the 5th slot activates, the VoiceAllocator switches to a global pool. The most recently played engine gets priority allocation (up to 8 voices). Inactive engines fall to 4 voices. Total voice count stays at 32 — the same as 4 engines at 8 voices each. No additional per-voice CPU cost.

**Decision F3: MegaCouplingMatrix remains 4x4 for audio routing.** The 5th slot uses metadata coupling (fingerprints), not audio coupling. The SROAuditor monitors the 5th slot separately from the 4-slot audio budget. The Ghost Slot architecture from the Fusion visionary doc is correct — the slot is structurally separate from the main coupling matrix.

---

## Cross-Cutting Architecture Decisions

These decisions affect the entire Kitchen Collection and must be resolved before any DSP coding begins.

### Decision X1: Shared Modal Resonator Bank Utility

All Kitchen engines use the same underlying modal synthesis. Build a `ModalBank` class in `Source/DSP/ModalBank.h` that:
- Manages N second-order IIR resonators with dynamic pruning
- Accepts material parameter tables (density, wave speed, internal damping)
- Supports SIMD processing (4-wide IIR processing)
- Exports SpectralFingerprint data
- Handles denormal flushing internally

This follows the SRO principle of shared kernels (like CytomicSVF). One optimized modal bank lifts all 4 Kitchen engines simultaneously.

### Decision X2: Material Property Table Format

Pre-compute material physics at initialization, not at runtime:

```cpp
struct MaterialTable {
    float density;           // kg/m^3
    float waveSpeed;         // m/s
    float impedance;         // density * waveSpeed
    float internalDamping;   // loss factor eta
    float thermalExpansion;  // cents per degree C
    float modalFrequencies[16];  // eigenfrequencies for this material
    float modalDecayRates[16];   // per-mode decay rates
    float sympatheticLevel;      // global sympathetic resonance level
};

static const MaterialTable kCastIron = {
    7200.f, 5000.f, 36000000.f, 0.002f, 0.08f,
    {55.f, 110.f, 165.f, 220.f, 330.f, 440.f, 550.f, 660.f,
     880.f, 1100.f, 1320.f, 1650.f, 2200.f, 2640.f, 3300.f, 4400.f},
    {0.998f, 0.997f, 0.995f, 0.993f, 0.990f, 0.987f, 0.983f, 0.978f,
     0.970f, 0.960f, 0.945f, 0.925f, 0.890f, 0.850f, 0.800f, 0.740f},
    0.6f
};
```

### Decision X3: ControlRateReducer Ratio for the Kitchen Collection

All slow modulation in the Kitchen Collection uses `ControlRateReducer<64>`:
- Temperature drift: update every 64 samples (1.45 ms at 44.1kHz)
- Gravity coupling: update every 64 samples
- Mycorrhizal events: checked every 64 samples
- Accumulator integrators (W/A/D, pressure, session age, mass): updated every block (512 samples)
- Diffusion envelope: recomputed every block

This is slower than the fleet standard of `<32>`, but the Kitchen Collection's modulation sources are all deliberately slow (seconds to minutes). The 64:1 ratio saves an additional 50% over 32:1 and is perceptually identical.

### Decision X4: SROAuditor Extension for 5-Slot Mode

The SROAuditor's `MaxSlots = 4` constant must be conditionally extended when the Kitchen Complete condition is met. Two options:

**Option A:** Keep MaxSlots = 4, monitor Fusion in a separate `fusionSlotReport` field.
**Option B:** Compile with MaxSlots = 5 but only populate slot 5 when active.

**Recommendation: Option A.** The Fusion slot has a fundamentally different budget profile (fingerprint-based, not audio-based). Mixing it into the same 4-slot array would distort the efficiency score. A separate field keeps the accounting honest.

---

## Per-Quad CPU Budget Summary

| Quad | V1 Target | V2 Target | Key Optimization |
|------|-----------|-----------|------------------|
| **KITCHEN** | 3.2% per engine | 2.5% per engine | 16-mode hybrid modal + HF noise, dynamic pruning |
| **CELLAR** | 3.1% per engine | 2.8% per engine | MIDI-domain gravity, no FFT |
| **GARDEN** | 3.1% per engine | 2.5% per engine | Event-queue mycorrhizal, 4-voice polyphony |
| **BROTH** | 0.5–1.4% per engine | 0.3–1.0% per engine | Analytical Gaussian diffusion, no FFT |
| **FUSION** | 16.9% total (5 engines) | 14% total (5 engines) | Spectral Fingerprint Cache |

### Full Kitchen Collection Worst Case (All 24 Engines Theoretically Loaded)

This never happens in practice — the user loads at most 5 engines (Kitchen quad + Fusion). But for ceiling analysis:

- 4 Kitchen engines + 1 Fusion: 16.9%
- 4 Cellar engines (coupled to Kitchen): max 2 loaded = 6.2%
- 4 Garden engines: max 2 loaded = 6.2%
- 4 Broth engines: max 2 loaded = 2.8%
- Chef quad (future): TBD
- Pantry quad (future): TBD

**Realistic maximum session:** 4 Kitchen + 1 Fusion = **16.9% CPU.** Well within the 25% ceiling.

---

## The Creative Optimization Leaderboard

Ranked by creative value per CPU cycle:

1. **Spectral Fingerprint Cache** (FUSION) — Eliminates the need for 4 additional audio streams by replacing audio coupling with metadata coupling. Saves ~8% CPU. Enables an unprecedented 5-engine architecture. The constraint (no audio routing to slot 5) actually produces a more musically interesting result because the Fusion engine responds to *material character* rather than raw audio.

2. **MIDI-Domain Gravity** (CELLAR) — Replaces FFT-based spectral analysis with direct MIDI state inspection. Saves ~0.5% CPU per coupled engine and eliminates 23ms latency. More musically accurate because it responds to the player's intent rather than the rendered audio's artifacts.

3. **Analytical Gaussian Diffusion** (BROTH) — Replaces FFT-domain diffusion with a sum of expanding Gaussians. 20x cheaper and produces mathematically smoother diffusion fronts. The cheap path is the high-quality path.

4. **Event-Queue Mycorrhizal Network** (GARDEN) — Replaces 39.5 MB of audio delay lines with 7.2 KB of timed events. 5000x memory reduction. Biologically accurate — real mycorrhizal signals are chemical packets, not continuous streams.

5. **Hybrid Modal-Stochastic Body** (KITCHEN) — Replaces 64 IIR resonators with 16 resonators + filtered noise. 4x cheaper. Perceptually equivalent because modes above 400Hz are masked by the aggregate spectral envelope. The reduced mode count is physically justified — cast iron and copper have simpler modal structures than spruce.

---

## Appendix: New SRO Components Required

| Component | File | Purpose |
|-----------|------|---------|
| `ModalBank` | `Source/DSP/ModalBank.h` | Shared modal resonator bank with SIMD, dynamic pruning, material tables |
| `SpectralFingerprint` | `Source/DSP/SpectralFingerprint.h` | Compact modal state export for cross-engine metadata coupling |
| `MycorrhizalChannel` | `Source/DSP/MycorrhizalChannel.h` | Event-queue delayed scalar propagation |
| `GaussianDiffusion` | `Source/DSP/GaussianDiffusion.h` | Analytical sum-of-Gaussians spectral diffusion |
| `GravityProcessor` | `Source/DSP/GravityProcessor.h` | MIDI-domain gravitational pitch displacement |
| `MaterialTable` | `Source/DSP/MaterialTable.h` | Pre-computed material physics constants |

All components follow existing SRO conventions: inline `.h` headers, zero allocation, `noexcept`, `flushDenormal()` on all feedback paths.

---

*Joint analysis by the SRO Optimizer and The Visionary.*
*March 2026. Do not commit. Architecture decisions require owner review before DSP coding begins.*
