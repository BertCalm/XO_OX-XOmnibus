# SP1: FamilyWaveguide.h Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the shared physical-modeling DSP library (`Source/DSP/FamilyWaveguide.h`) that all 5 XOrphica Family Constellation engines (XOhm, XOrphica, XObbligato, XOttoni, XOlé) will use.

**Architecture:** All components are header-only, allocation-free, namespace `xolokun`. FamilyWaveguide.h contains 5 waveguide primitives (delay line, damping filter, body resonance, sympathetic bank, organic drift) plus 7 engine-specific exciters. Every exciter seeds a FamilyDelayLine; the delay line + FamilyDampingFilter in the feedback path forms the Karplus-Strong resonator core. FamilyBodyResonance and FamilySympatheticBank add timbral richness post-resonator.

**Tech Stack:** C++17, inline `.h` headers, `namespace xolokun`, no JUCE dependency (pure DSP), FastMath.h (`flushDenormal`, `fastTanh`) as the only dep.

---

## File Structure

| File | Action | Purpose |
|------|--------|---------|
| `Source/DSP/FamilyWaveguide.h` | **Create** | All 5 primitives + 7 exciters (~700 LOC) |
| `Tests/FamilyWaveguideTest.cpp` | **Create** | Standalone C++ test (impulse → non-zero, non-NaN, decays) |
| `Tests/CMakeLists.txt` | **Create** | Minimal CMake to build the test binary |

---

## Chunk 1: Core Waveguide Primitives

### Task 1: FamilyDelayLine — Fractional Delay with Lagrange Interpolation

**Files:**
- Create: `Source/DSP/FamilyWaveguide.h` (partial — just FamilyDelayLine)

**Background:** A delay line is a circular buffer. For pitch-accurate waveguide synthesis, you need fractional delay (delay length is rarely a whole number of samples). Lagrange interpolation reads between samples smoothly.

- [ ] **Step 1: Create the file with FamilyDelayLine**

```cpp
// Source/DSP/FamilyWaveguide.h
#pragma once
#include "FastMath.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <array>
#include <algorithm>

namespace xolokun {

//==============================================================================
// FamilyWaveguide — Shared physical-modeling DSP for the XOrphica Family Constellation.
//
// Five waveguide primitives + seven exciters. All components are:
//   - Header-only, no memory allocation in process path
//   - Denormal-safe (uses flushDenormal() from FastMath.h)
//   - Real-time safe: no allocations, no blocking I/O after prepare()
//
// Usage pattern:
//   1. Call prepare(sampleRate, maxDelaySamples) once before audio starts
//   2. Set delayLength to pitch (sampleRate / frequency)
//   3. Feed an exciter output into write(), then read() each sample
//   4. Route read() output through FamilyDampingFilter in feedback loop
//==============================================================================

//==============================================================================
// FamilyDelayLine — Circular buffer with 4-point Lagrange fractional read.
//
// The core of waveguide synthesis. Delay length controls pitch:
//   delayLength = sampleRate / fundamentalFrequency
//
// Lagrange 4-point interpolation gives smooth fractional reads with no
// audible artifacts, unlike linear interpolation which causes HF rolloff.
//==============================================================================
class FamilyDelayLine {
public:
    FamilyDelayLine() = default;

    // Call once before audio starts. maxDelaySamples must be >= sampleRate/20 (50 Hz min pitch).
    void prepare(int maxDelaySamples) {
        buffer.assign(maxDelaySamples + 4, 0.0f); // +4 for interpolation guard
        mask = static_cast<int>(buffer.size()) - 1;
        writePos = 0;
    }

    void reset() {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;
    }

    // Write one sample into the delay line.
    void write(float x) {
        buffer[writePos & mask] = x;
        ++writePos;
    }

    // Read from the delay line with fractional Lagrange interpolation.
    // delayLength: samples of delay (e.g. sampleRate / frequency).
    float read(float delayLength) const {
        // Integer and fractional parts of delay
        int d = static_cast<int>(delayLength);
        float frac = delayLength - static_cast<float>(d);

        // 4-point Lagrange coefficients
        float y0 = buffer[(writePos - d - 2) & mask];
        float y1 = buffer[(writePos - d - 1) & mask];
        float y2 = buffer[(writePos - d    ) & mask];
        float y3 = buffer[(writePos - d + 1) & mask];

        // Lagrange 4-point interpolation
        float c0 = y1;
        float c1 = y2 - (1.0f/3.0f)*y0 - 0.5f*y1 - (1.0f/6.0f)*y3;
        float c2 = 0.5f*(y0 + y2) - y1;
        float c3 = (1.0f/6.0f)*(y3 - y0) + 0.5f*(y1 - y2);
        return c0 + frac * (c1 + frac * (c2 + frac * c3));
    }

private:
    std::vector<float> buffer;
    int mask = 0;
    int writePos = 0;
};
```

- [ ] **Step 2: Verify file exists**

```bash
ls -la /path/to/XO_OX-XOlokun/Source/DSP/FamilyWaveguide.h
```

Expected: file present, non-zero size.

---

### Task 2: FamilyDampingFilter — One-Pole LP in Feedback Path

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append after FamilyDelayLine)

**Background:** In a Karplus-Strong loop, the feedback signal passes through a low-pass filter each cycle. This simulates energy absorption — higher frequencies decay faster than lower frequencies, exactly like a plucked string or blown tube. Coefficient `a` (0.0–1.0) controls material brightness: 0.0 = instant death, 1.0 = infinite sustain (and infinite resonance — clamp!).

- [ ] **Step 1: Append FamilyDampingFilter to the header**

```cpp
//==============================================================================
// FamilyDampingFilter — One-pole low-pass filter for waveguide feedback path.
//
// y[n] = (1-a)*x[n] + a*y[n-1]
//
// `damping` controls HF absorption:
//   0.0 = no filtering (harsh, instant decay)
//   0.95 = warm, medium sustain (good for plucked strings)
//   0.999 = very long sustain (bowed strings, organ pipes)
//   1.0 = forbidden (infinite feedback) — clamped internally to 0.9999
//==============================================================================
class FamilyDampingFilter {
public:
    void prepare() { state = 0.0f; }
    void reset()   { state = 0.0f; }

    // Process one sample through the one-pole LP.
    // damping: 0.0 (bright/dead) → 0.999 (dark/sustaining)
    float process(float x, float damping) {
        float a = std::min(damping, 0.9999f);
        state = (1.0f - a) * x + a * state;
        state = flushDenormal(state);
        return state;
    }

private:
    float state = 0.0f;
};
```

---

### Task 3: FamilyBodyResonance — 2-Pole Biquad Body Resonator

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

**Background:** Real instrument bodies have resonant modes — the wood of a guitar, the gourd of a sitar. A 2-pole resonator (biquad bandpass) emphasizes specific frequencies to simulate body character. Multiple instances tuned to harmonic ratios give a rich body response.

- [ ] **Step 1: Append FamilyBodyResonance**

```cpp
//==============================================================================
// FamilyBodyResonance — 2-pole resonator simulating instrument body modes.
//
// Implements a biquad bandpass resonator. Tuned to the fundamental or a
// harmonic ratio to add body coloration. Use 1-3 instances per voice.
//
// frequency: resonance center (Hz)  — e.g., 200 Hz for guitar body
// resonance: Q factor (1.0–20.0)   — higher = narrower, more ringy
// gain:      linear mix amount      — typically 0.1–0.4
//==============================================================================
class FamilyBodyResonance {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        reset();
    }

    void reset() {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    // Compute biquad bandpass coefficients from frequency and Q.
    void setParams(float frequency, float resonance) {
        float w0 = 2.0f * 3.14159265f * frequency / sr;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * std::max(resonance, 0.1f));

        float b0 =  alpha;
        float b1 =  0.0f;
        float b2 = -alpha;
        float a0 =  1.0f + alpha;
        float a1 = -2.0f * cosw0;
        float a2 =  1.0f - alpha;

        // Normalize by a0
        nb0 = b0 / a0;  nb1 = b1 / a0;  nb2 = b2 / a0;
        na1 = a1 / a0;  na2 = a2 / a0;
    }

    float process(float x) {
        float y = nb0*x + nb1*x1 + nb2*x2 - na1*y1 - na2*y2;
        y = flushDenormal(y);
        x2 = x1;  x1 = x;
        y2 = y1;  y1 = y;
        return y;
    }

private:
    float sr = 44100.0f;
    float nb0=0, nb1=0, nb2=0, na1=0, na2=0;
    float x1=0, x2=0, y1=0, y2=0;
};
```

---

### Task 4: FamilySympatheticBank — 8 Tuned Comb Filters

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

**Background:** A sympathetic string vibrates in resonance when a nearby string is plucked at its harmonic. On a sitar, dozens of extra strings resonate sympathetically. A comb filter (delay + feedback) is a simple resonator — 8 of them tuned to harmonic ratios create the shimmering, sustaining quality of instruments with sympathetic strings.

- [ ] **Step 1: Append FamilySympatheticBank**

```cpp
//==============================================================================
// FamilySympatheticBank — 8 tuned comb filters simulating sympathetic strings/tubes.
//
// Each comb filter resonates at a specific harmonic of the fundamental.
// Tuning: harmonic ratios relative to baseFrequency (1x, 2x, 3x, 4x, 5x, 6x, 8x, 12x).
// Mix: output is a weighted sum of all 8 combs, scaled by `amount`.
//
// Call tune(baseFrequency) when note changes.
// Call process(input, amount) each sample. amount: 0.0 = off, 1.0 = full blend.
//==============================================================================
class FamilySympatheticBank {
public:
    static constexpr int kNumCombs = 8;
    static constexpr float kHarmonics[kNumCombs] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f, 12.0f};

    void prepare(double sampleRate, int maxBlockSize) {
        sr = static_cast<float>(sampleRate);
        for (int i = 0; i < kNumCombs; ++i)
            combs[i].assign(static_cast<int>(sr / 20.0f) + 4, 0.0f); // min 20 Hz
        std::fill(positions, positions + kNumCombs, 0);
        std::fill(lengths, lengths + kNumCombs, 100.0f);
        (void)maxBlockSize;
    }

    void reset() {
        for (int i = 0; i < kNumCombs; ++i) {
            std::fill(combs[i].begin(), combs[i].end(), 0.0f);
            positions[i] = 0;
        }
        std::fill(states, states + kNumCombs, 0.0f);
    }

    // Set comb delays to harmonics of baseFrequency (Hz).
    void tune(float baseFrequency) {
        for (int i = 0; i < kNumCombs; ++i)
            lengths[i] = std::max(sr / (baseFrequency * kHarmonics[i]), 2.0f);
    }

    // Process one sample. amount: 0.0 (off) → 1.0 (full sympathetic blend).
    float process(float input, float amount) {
        float out = 0.0f;
        for (int i = 0; i < kNumCombs; ++i) {
            int len = static_cast<int>(lengths[i]);
            int sz = static_cast<int>(combs[i].size());
            // Read from delay
            int readPos = (positions[i] - len + sz) % sz;
            float delayed = combs[i][readPos];
            // Simple comb feedback (0.85 decay)
            float fed = flushDenormal(input + 0.85f * delayed);
            combs[i][positions[i] % sz] = fed;
            positions[i] = (positions[i] + 1) % sz;
            states[i] = flushDenormal(states[i] * 0.9f + delayed * 0.1f);
            out += states[i];
        }
        return (out / kNumCombs) * amount;
    }

private:
    float sr = 44100.0f;
    std::vector<float> combs[kNumCombs];
    int positions[kNumCombs] = {};
    float lengths[kNumCombs] = {};
    float states[kNumCombs] = {};
};

// Static member definition
constexpr float FamilySympatheticBank::kHarmonics[FamilySympatheticBank::kNumCombs];
```

---

### Task 5: FamilyOrganicDrift — Slow Pitch/Timing Wander

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

**Background:** Real instruments drift. A sitar player's intonation wanders slightly; a wooden flute is affected by breath temperature. FamilyOrganicDrift generates slow, band-limited noise using two sine LFOs at slightly different rates (avoids periodicity) to produce a naturalistic pitch/timing wander. Output is ±cents or ±ms jitter.

- [ ] **Step 1: Append FamilyOrganicDrift**

```cpp
//==============================================================================
// FamilyOrganicDrift — Slow pitch/timing wander for humanized waveguide voices.
//
// Uses two sine LFOs at prime-ratio rates to avoid periodicity.
// Output: pitch deviation in semitone-fraction (multiply by delayLength for sample offset).
//
// rateHz:    base drift rate (0.05–0.5 Hz typical)
// depthCents: ±cents of pitch deviation (0.0–20.0 typical)
//==============================================================================
class FamilyOrganicDrift {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        phase1 = phase2 = 0.0f;
    }

    void reset() { phase1 = phase2 = 0.0f; }

    // Returns pitch deviation as a fraction of a semitone.
    // Multiply by (delayLength * 0.05946f) to get sample-offset for pitch modulation.
    float tick(float rateHz, float depthCents) {
        float inc1 = rateHz / sr;
        float inc2 = rateHz * 1.618f / sr; // golden ratio avoids harmonics

        phase1 = std::fmod(phase1 + inc1, 1.0f);
        phase2 = std::fmod(phase2 + inc2, 1.0f);

        float lfo = 0.6f * std::sin(phase1 * 6.28318f) + 0.4f * std::sin(phase2 * 6.28318f);
        return lfo * (depthCents / 100.0f); // cents → semitone fraction
    }

private:
    float sr = 44100.0f;
    float phase1 = 0.0f, phase2 = 0.0f;
};
```

---

## Chunk 2: Exciters

### Task 6: PluckExciter — Noise Burst for Harp (XOrphica)

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

**Background:** A harp string is excited by a fast noise burst — the finger's pluck releases stored energy as broadband noise, which the delay line then filters into a pitched tone. The exciter fires once on note-on, then stays silent. The delay line sustains the tone.

- [ ] **Step 1: Append PluckExciter**

```cpp
//==============================================================================
// Exciters — Engine-specific signal generators that seed FamilyDelayLine.
//
// Each exciter fires on note-on, producing a short burst that the delay line
// converts into a sustaining pitched resonance. All exciters are stateful
// (track their own phase/position) and allocation-free after init.
//==============================================================================

//------------------------------------------------------------------------------
// PluckExciter — Short filtered noise burst for harp and plucked string tones.
//
// trigger(): call on note-on. Sets internal phase to begin burst.
// tick(): call each sample. Returns exciter output (decays to 0 after burstSamples).
//
// brightness: 0.0 (dark gut string) → 1.0 (crystal/glass harp)
//------------------------------------------------------------------------------
class PluckExciter {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        filterState = 0.0f;
        remaining = 0;
        // Simple LCG for real-time safe noise (no std::rand on audio thread)
        seed = 12345;
    }

    void reset() { remaining = 0; filterState = 0.0f; }

    // Fire exciter. burstMs: length of noise burst (1-5 ms typical).
    void trigger(float burstMs = 2.0f) {
        remaining = static_cast<int>(sr * burstMs * 0.001f);
    }

    // Returns one sample of exciter output.
    // brightness: 0.0 = warm/dark, 1.0 = bright/crystal
    float tick(float brightness) {
        if (remaining <= 0) return 0.0f;
        --remaining;

        // Real-time safe LCG noise
        seed = seed * 1664525u + 1013904223u;
        float noise = static_cast<float>(static_cast<int32_t>(seed)) * 4.656612e-10f;

        // One-pole HP as brightness control: bright = more HF content
        float cutoff = 0.1f + brightness * 0.8f; // LP coefficient
        filterState = cutoff * noise + (1.0f - cutoff) * filterState;
        filterState = flushDenormal(filterState);

        // Decay envelope over burst
        float env = static_cast<float>(remaining) / static_cast<float>(remaining + 1);
        return filterState * env;
    }

private:
    float sr = 44100.0f;
    float filterState = 0.0f;
    int remaining = 0;
    uint32_t seed = 12345;
};
```

---

### Task 7: StrumExciter — Multi-String Sequential Trigger (XOlé)

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

- [ ] **Step 1: Append StrumExciter**

```cpp
//------------------------------------------------------------------------------
// StrumExciter — Staggered multi-string plucks for strummed chord textures.
//
// Fires up to 6 PluckExciters with inter-string delay to simulate strum.
// strumRateMs: time between successive string triggers (1-30 ms).
// numStrings: how many strings in the strum (1-6).
// direction: +1.0 = down strum, -1.0 = up strum (reverses string order).
//------------------------------------------------------------------------------
class StrumExciter {
public:
    static constexpr int kMaxStrings = 6;

    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        for (auto& p : plucks) p.prepare(sampleRate);
        reset();
    }

    void reset() {
        for (auto& p : plucks) p.reset();
        std::fill(countdown, countdown + kMaxStrings, 0);
        strumPos = 0;
    }

    // Trigger a strum. Successive strings will fire after strumRateMs intervals.
    void trigger(int numStrings, float strumRateMs, float direction) {
        numStrings = std::min(numStrings, kMaxStrings);
        int interval = static_cast<int>(sr * strumRateMs * 0.001f);
        int order = (direction >= 0.0f) ? 1 : -1;
        for (int i = 0; i < numStrings; ++i) {
            int idx = (order > 0) ? i : (numStrings - 1 - i);
            countdown[idx] = i * interval; // stagger each string
        }
        strumPos = 0;
    }

    // Tick one sample. Returns summed output of all active strings.
    // brightness applies to each individual pluck.
    float tick(float brightness) {
        float out = 0.0f;
        for (int i = 0; i < kMaxStrings; ++i) {
            if (countdown[i] > 0) {
                --countdown[i];
                if (countdown[i] == 0) plucks[i].trigger(2.0f);
            }
            out += plucks[i].tick(brightness);
        }
        return out;
    }

private:
    float sr = 44100.0f;
    PluckExciter plucks[kMaxStrings];
    int countdown[kMaxStrings] = {};
    int strumPos = 0;
};
```

---

### Task 8: PickExciter — Fingerstyle/Clawhammer Attack (XOhm)

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

- [ ] **Step 1: Append PickExciter**

```cpp
//------------------------------------------------------------------------------
// PickExciter — Fingerstyle or clawhammer attack for banjo/guitar/mandolin.
//
// Similar to PluckExciter but with an asymmetric attack: the pick/nail
// slides across the string before release, creating a characteristic
// tonal "twang". Simulated by brief bandpass filtered burst.
//
// attackMs: nail-slide duration (0.5–3 ms)
// hardness: 0.0 = thumb (warm), 1.0 = fingernail (bright attack transient)
//------------------------------------------------------------------------------
class PickExciter {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        body.prepare(sampleRate);
        base.prepare(sampleRate);
        remaining = 0;
        seed = 98765u;
    }

    void reset() {
        remaining = 0;
        base.reset();
        body.reset();
    }

    void trigger(float attackMs = 1.5f) {
        remaining = static_cast<int>(sr * attackMs * 0.001f);
    }

    float tick(float hardness) {
        if (remaining <= 0) return 0.0f;
        --remaining;

        seed = seed * 1664525u + 1013904223u;
        float noise = static_cast<float>(static_cast<int32_t>(seed)) * 4.656612e-10f;

        // Hard pick: boost 2-4 kHz band (nail transient character)
        float mid = std::min(hardness * 4000.0f + 200.0f, 4000.0f);
        body.setParams(mid, 2.5f + hardness * 5.0f);
        float shaped = noise * (1.0f - hardness) + body.process(noise) * hardness;

        float env = static_cast<float>(remaining) / static_cast<float>(remaining + 4);
        return flushDenormal(shaped * env * 0.8f);
    }

private:
    float sr = 44100.0f;
    FamilyBodyResonance body, base;
    int remaining = 0;
    uint32_t seed = 98765u;
};
```

---

### Task 9: AirJetExciter — Filtered Noise for Flute (XObbligato Brother A)

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

- [ ] **Step 1: Append AirJetExciter**

```cpp
//------------------------------------------------------------------------------
// AirJetExciter — Breath-driven air jet for flute-family instruments.
//
// Models the air jet striking the embouchure hole: noise filtered by a
// bandpass tuned to the resonant frequency, modulated by breath pressure.
// Continuous (not one-shot) — outputs as long as breathPressure > 0.
//
// breathPressure: 0.0 (no sound) → 1.0 (full tone), controls both amplitude and HF noise
// frequency:      fundamental pitch (Hz) — tunes the jet resonance window
//------------------------------------------------------------------------------
class AirJetExciter {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        jetr.prepare(sampleRate);
        hp.prepare();
        seed = 54321u;
        hpState = 0.0f;
    }

    void reset() {
        hpState = 0.0f;
        jetr.reset();
        hp.reset();
    }

    float tick(float breathPressure, float frequency) {
        seed = seed * 1664525u + 1013904223u;
        float noise = static_cast<float>(static_cast<int32_t>(seed)) * 4.656612e-10f;

        // Bandpass tuned near resonance — air jet couples to tube
        jetr.setParams(frequency * 0.5f, 1.5f);
        float jet = jetr.process(noise);

        // HP to remove DC offset from breath pressure
        float bpShaped = jet * breathPressure;
        hpState = 0.999f * (hpState + bpShaped - bpShaped);
        hpState = flushDenormal(hpState);

        // Breath noise floor (audible breath at low pressure = natural flutter)
        float breathNoise = noise * (1.0f - breathPressure) * 0.05f;
        return flushDenormal(bpShaped + breathNoise);
    }

private:
    float sr = 44100.0f;
    FamilyBodyResonance jetr;
    FamilyDampingFilter hp;
    float hpState = 0.0f;
    uint32_t seed = 54321u;
};
```

---

### Task 10: ReedExciter — Nonlinear Waveshaper for Reed (XObbligato Brother B)

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

- [ ] **Step 1: Append ReedExciter**

```cpp
//------------------------------------------------------------------------------
// ReedExciter — Nonlinear reed model for clarinet/oboe/bassoon family.
//
// The reed is a pressure-controlled valve: at low pressure it opens freely,
// at high pressure it clamps shut. This nonlinear behavior creates the
// characteristic odd-harmonic saturation of reed instruments.
// Models the clarinet reed equation: y = x - (x^3)/3 (smooth cubic clip).
//
// Continuous exciter — outputs as long as breathPressure > 0.
// reedStiffness: 0.0 (soft/clarinet) → 1.0 (hard/bassoon brightness)
//------------------------------------------------------------------------------
class ReedExciter {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        filter.prepare();
        seed = 77777u;
    }

    void reset() { filter.reset(); }

    float tick(float breathPressure, float reedStiffness) {
        seed = seed * 1664525u + 1013904223u;
        float noise = static_cast<float>(static_cast<int32_t>(seed)) * 4.656612e-10f * 0.1f;

        // Reed pressure signal: DC + noise
        float pressure = breathPressure + noise;

        // Nonlinear reed valve: cubic saturation (odd harmonics)
        float drive = 1.0f + reedStiffness * 4.0f;
        float driven = pressure * drive;
        float clipped = fastTanh(driven); // odd-harmonic saturation

        // LP filter models the reed's mechanical mass (low-pass character)
        float damping = 0.7f + reedStiffness * 0.2f;
        return filter.process(clipped, damping);
    }

private:
    float sr = 44100.0f;
    FamilyDampingFilter filter;
    uint32_t seed = 77777u;
};
```

---

### Task 11: LipBuzzExciter — Lip Oscillation for Brass (XOttoni)

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append)

- [ ] **Step 1: Append LipBuzzExciter**

```cpp
//------------------------------------------------------------------------------
// LipBuzzExciter — Lip buzz oscillation for brass and natural horn family.
//
// Models lip vibration as a feedback oscillator. The lips form a pressure-
// controlled valve that oscillates at the tube's resonant frequency.
// At low embouchure tension: loose/buzzy (tuba/trombone quality).
// At high tension: clean/bright (trumpet/piccolo trumpet quality).
//
// Simulated as a band-limited sawtooth through the lips' mass-spring resonance.
// embouchureTension: 0.0 (loose/deep) → 1.0 (tight/bright)
// ageScale: 0.0 (toddler/conch) → 1.0 (teen/French horn) — maps XOttoni's age model
//------------------------------------------------------------------------------
class LipBuzzExciter {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        phase = 0.0f;
        lipFilter.prepare();
    }

    void reset() {
        phase = 0.0f;
        lipFilter.reset();
    }

    float tick(float frequency, float embouchureTension, float ageScale) {
        // Frequency adjusted by age: younger → less stable intonation
        float ageJitter = (1.0f - ageScale) * 0.02f; // up to ±2% pitch wobble
        float adjFreq = frequency * (1.0f + ageJitter * std::sin(phase * 13.7f));

        // Sawtooth oscillator (lip buzz fundamental)
        phase += adjFreq / sr;
        if (phase >= 1.0f) phase -= 1.0f;
        float saw = 2.0f * phase - 1.0f;

        // Lip mass-spring filter: looser lips → more LP (darker, buzzier)
        float lipDamping = 0.5f + embouchureTension * 0.45f;
        float buzzed = lipFilter.process(saw, lipDamping);

        // Age scales harmonic complexity: toddler = soft, teen = full buzz
        float amplitude = 0.3f + ageScale * 0.7f;
        return flushDenormal(buzzed * amplitude);
    }

private:
    float sr = 44100.0f;
    float phase = 0.0f;
    FamilyDampingFilter lipFilter;
};
```

---

### Task 12: BowExciter — Continuous Friction for Fiddle (XOhm)

**Files:**
- Modify: `Source/DSP/FamilyWaveguide.h` (append, then close namespace)

- [ ] **Step 1: Append BowExciter and close namespace**

```cpp
//------------------------------------------------------------------------------
// BowExciter — Continuous friction model for bowed string instruments.
//
// Models Helmholtz motion: the bow alternately sticks and slips on the string.
// Stick phase: bow drags string with it (velocity coupling).
// Slip phase: string springs back (release + flyback).
//
// Approximated with velocity-dependent friction: tanh saturation of the
// relative velocity between bow and string. The output is the friction force
// that excites the string delay line.
//
// bowPressure:  0.0 (lifted) → 1.0 (full pressure) — controls stick/slip
// bowSpeed:     0.0 (slow/gentle) → 1.0 (fast/aggressive) — controls brightness
// stringVelocity: feed back the waveguide output to close the Helmholtz loop
//------------------------------------------------------------------------------
class BowExciter {
public:
    void prepare(double sampleRate) {
        sr = static_cast<float>(sampleRate);
        bowVel = 0.0f;
        frictionLP.prepare();
    }

    void reset() {
        bowVel = 0.0f;
        frictionLP.reset();
    }

    float tick(float bowPressure, float bowSpeed, float stringVelocity) {
        if (bowPressure < 0.001f) return 0.0f;

        // Relative velocity: bow surface vs string
        float relVel = bowSpeed - stringVelocity;

        // Friction curve: tanh models the nonlinear stick/slip transition
        float friction = bowPressure * fastTanh(relVel * (5.0f + bowPressure * 10.0f));

        // LP filter smooths friction impulses (bow hair contact)
        float smooth = frictionLP.process(friction, 0.6f + bowPressure * 0.3f);
        return flushDenormal(smooth);
    }

private:
    float sr = 44100.0f;
    float bowVel = 0.0f;
    FamilyDampingFilter frictionLP;
};

} // namespace xolokun
```

---

## Chunk 3: Test + Integration

### Task 13: Write Standalone C++ Test

**Files:**
- Create: `Tests/FamilyWaveguideTest.cpp`
- Create: `Tests/CMakeLists.txt`

**Background:** Since FamilyWaveguide.h has no JUCE dependency, we can test it with a plain C++ binary. The test verifies: (a) all exciters produce non-zero output after trigger, (b) the delay line sustains and decays, (c) no NaN or Inf ever appears, (d) output decays toward zero (not growing = no feedback runaway).

- [ ] **Step 1: Write the test file**

```cpp
// Tests/FamilyWaveguideTest.cpp
// Plain C++ standalone test — no JUCE required.
// Build: cd Tests && mkdir -p build && cd build && cmake .. && make && ./FamilyWaveguideTest

#include "../Source/DSP/FamilyWaveguide.h"
#include <cstdio>
#include <cmath>
#include <cassert>
#include <string>

static bool isFinite(float x) { return std::isfinite(x); }

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

// Helper: run a Karplus-Strong loop for N samples, return max amplitude and whether it decayed
struct KSResult { float peak; float finalAmp; bool noNaN; };

KSResult runKS(xolokun::FamilyDelayLine& dl, xolokun::FamilyDampingFilter& df,
               float delayLen, int numSamples, float damping) {
    float peak = 0.0f;
    float finalAmp = 0.0f;
    bool noNaN = true;
    for (int i = 0; i < numSamples; ++i) {
        float out = dl.read(delayLen);
        float fed = df.process(out, damping);
        dl.write(fed);
        if (!isFinite(out)) noNaN = false;
        float amp = std::abs(out);
        if (amp > peak) peak = amp;
        if (i == numSamples - 1) finalAmp = amp;
    }
    return {peak, finalAmp, noNaN};
}

int main() {
    int passed = 0, failed = 0;
    const double SR = 44100.0;
    const float pitch = 440.0f; // A4
    const float delayLen = static_cast<float>(SR) / pitch; // ~100 samples

    auto check = [&](bool condition, const char* name, const char* msg) {
        if (condition) { printf("[PASS] %s\n", name); ++passed; }
        else           { printf("[FAIL] %s — %s\n", name, msg); ++failed; }
    };

    // -------------------------------------------------------------------
    // Test 1: FamilyDelayLine prepare and basic write/read
    // -------------------------------------------------------------------
    {
        xolokun::FamilyDelayLine dl;
        dl.prepare(static_cast<int>(SR) + 4);
        dl.write(1.0f);
        for (int i = 0; i < 10; ++i) dl.write(0.0f);
        float r = dl.read(5.0f);
        check(isFinite(r), "DelayLine_basic_read_finite", "Read returned NaN/Inf");
    }

    // -------------------------------------------------------------------
    // Test 2: Karplus-Strong loop produces non-zero peak and decays
    // -------------------------------------------------------------------
    {
        xolokun::FamilyDelayLine dl;
        xolokun::FamilyDampingFilter df;
        dl.prepare(static_cast<int>(SR) + 4);
        df.prepare();

        // Seed with impulse
        for (int i = 0; i < static_cast<int>(delayLen); ++i)
            dl.write(i == 0 ? 1.0f : 0.0f);

        auto res = runKS(dl, df, delayLen, static_cast<int>(SR * 2.0f), 0.995f);
        check(res.noNaN, "KS_loop_no_NaN", "NaN/Inf detected in KS loop output");
        check(res.peak > 0.01f, "KS_loop_peak_nonzero", "KS loop produced zero peak — damping filter broken?");
        check(res.finalAmp < res.peak * 0.5f, "KS_loop_decays", "KS output did not decay after 2 seconds");
    }

    // -------------------------------------------------------------------
    // Test 3: PluckExciter fires non-zero, goes silent after burst
    // -------------------------------------------------------------------
    {
        xolokun::PluckExciter pe;
        pe.prepare(SR);
        pe.trigger(2.0f); // 2 ms burst
        float maxOut = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 2000; ++i) {
            float s = pe.tick(0.5f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "PluckExciter_no_NaN", "PluckExciter produced NaN/Inf");
        check(maxOut > 1e-6f, "PluckExciter_nonzero", "PluckExciter produced no output after trigger");
        // After ~4000 samples (>2ms at 44100) it should be silent
        float lateOut = 0.0f;
        for (int i = 0; i < 1000; ++i) lateOut += std::abs(pe.tick(0.5f));
        check(lateOut < 1e-6f, "PluckExciter_silent_after_burst", "PluckExciter still outputting past burst window");
    }

    // -------------------------------------------------------------------
    // Test 4: StrumExciter fires multiple strings staggered
    // -------------------------------------------------------------------
    {
        xolokun::StrumExciter se;
        se.prepare(SR);
        se.trigger(4, 5.0f, 1.0f); // 4 strings, 5ms apart, down strum
        float maxOut = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 5000; ++i) {
            float s = se.tick(0.5f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "StrumExciter_no_NaN", "StrumExciter produced NaN/Inf");
        check(maxOut > 1e-6f, "StrumExciter_nonzero", "StrumExciter produced no output");
    }

    // -------------------------------------------------------------------
    // Test 5: PickExciter fires, non-zero
    // -------------------------------------------------------------------
    {
        xolokun::PickExciter pe;
        pe.prepare(SR);
        pe.trigger(1.5f);
        float maxOut = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 1000; ++i) {
            float s = pe.tick(0.7f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "PickExciter_no_NaN", "PickExciter NaN/Inf");
        check(maxOut > 1e-6f, "PickExciter_nonzero", "PickExciter no output");
    }

    // -------------------------------------------------------------------
    // Test 6: AirJetExciter continuous output at breathPressure=1
    // -------------------------------------------------------------------
    {
        xolokun::AirJetExciter ae;
        ae.prepare(SR);
        float maxOut = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 1000; ++i) {
            float s = ae.tick(1.0f, 440.0f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "AirJetExciter_no_NaN", "AirJetExciter NaN/Inf");
        check(maxOut > 1e-6f, "AirJetExciter_nonzero_at_full_breath", "AirJetExciter silent at full breath");
        // At zero breath: should be near silent
        float silentOut = 0.0f;
        ae.reset();
        for (int i = 0; i < 1000; ++i) silentOut += std::abs(ae.tick(0.0f, 440.0f));
        check(silentOut < 0.01f, "AirJetExciter_silent_at_zero_breath", "AirJetExciter not silent at zero breath");
    }

    // -------------------------------------------------------------------
    // Test 7: ReedExciter continuous, odd-harmonic saturation
    // -------------------------------------------------------------------
    {
        xolokun::ReedExciter re;
        re.prepare(SR);
        float maxOut = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 1000; ++i) {
            float s = re.tick(0.8f, 0.5f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "ReedExciter_no_NaN", "ReedExciter NaN/Inf");
        check(maxOut > 1e-6f, "ReedExciter_nonzero", "ReedExciter no output");
    }

    // -------------------------------------------------------------------
    // Test 8: LipBuzzExciter oscillates at given frequency
    // -------------------------------------------------------------------
    {
        xolokun::LipBuzzExciter le;
        le.prepare(SR);
        float maxOut = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 2000; ++i) {
            float s = le.tick(220.0f, 0.5f, 0.5f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "LipBuzzExciter_no_NaN", "LipBuzzExciter NaN/Inf");
        check(maxOut > 1e-6f, "LipBuzzExciter_nonzero", "LipBuzzExciter no output");
    }

    // -------------------------------------------------------------------
    // Test 9: BowExciter — output when pressure+speed nonzero
    // -------------------------------------------------------------------
    {
        xolokun::BowExciter be;
        be.prepare(SR);
        float maxOut = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 1000; ++i) {
            float s = be.tick(0.8f, 0.5f, 0.1f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "BowExciter_no_NaN", "BowExciter NaN/Inf");
        check(maxOut > 1e-6f, "BowExciter_nonzero_with_pressure", "BowExciter silent with bow pressure");
        // Zero pressure → silent
        float zeroOut = 0.0f;
        be.reset();
        for (int i = 0; i < 100; ++i) zeroOut += std::abs(be.tick(0.0f, 0.5f, 0.0f));
        check(zeroOut < 1e-6f, "BowExciter_silent_at_zero_pressure", "BowExciter not silent with no bow pressure");
    }

    // -------------------------------------------------------------------
    // Test 10: FamilyBodyResonance resonates at set frequency
    // -------------------------------------------------------------------
    {
        xolokun::FamilyBodyResonance br;
        br.prepare(SR);
        br.setParams(440.0f, 5.0f);
        float maxOut = 0.0f;
        bool anyNaN = false;
        // Feed impulse
        for (int i = 0; i < 2000; ++i) {
            float s = br.process(i == 0 ? 1.0f : 0.0f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "BodyResonance_no_NaN", "BodyResonance NaN/Inf");
        check(maxOut > 1e-6f, "BodyResonance_resonates", "BodyResonance no output from impulse");
    }

    // -------------------------------------------------------------------
    // Test 11: FamilySympatheticBank — output when input present
    // -------------------------------------------------------------------
    {
        xolokun::FamilySympatheticBank sb;
        sb.prepare(SR, 512);
        sb.tune(440.0f);
        float maxOut = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 4000; ++i) {
            float input = (i < 100) ? 0.5f : 0.0f;
            float s = sb.process(input, 1.0f);
            if (!isFinite(s)) anyNaN = true;
            maxOut = std::max(maxOut, std::abs(s));
        }
        check(!anyNaN, "SympatheticBank_no_NaN", "SympatheticBank NaN/Inf");
        check(maxOut > 1e-8f, "SympatheticBank_resonates", "SympatheticBank no output after impulse");
    }

    // -------------------------------------------------------------------
    // Test 12: FamilyOrganicDrift — output within ±1 semitone range
    // -------------------------------------------------------------------
    {
        xolokun::FamilyOrganicDrift od;
        od.prepare(SR);
        float maxDrift = 0.0f;
        bool anyNaN = false;
        for (int i = 0; i < 44100; ++i) {
            float d = od.tick(0.2f, 5.0f); // 0.2 Hz, ±5 cents
            if (!isFinite(d)) anyNaN = true;
            maxDrift = std::max(maxDrift, std::abs(d));
        }
        check(!anyNaN, "OrganicDrift_no_NaN", "OrganicDrift NaN/Inf");
        // ±5 cents = ±0.05 semitone fraction
        check(maxDrift < 0.1f, "OrganicDrift_within_range", "OrganicDrift exceeds expected range");
        check(maxDrift > 1e-6f, "OrganicDrift_nonzero", "OrganicDrift always zero");
    }

    // -------------------------------------------------------------------
    printf("\n=== FamilyWaveguide Tests: %d passed, %d failed ===\n", passed, failed);
    return (failed == 0) ? 0 : 1;
}
```

- [ ] **Step 2: Write Tests/CMakeLists.txt**

```cmake
# Tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(FamilyWaveguideTest LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Adjust path if running from XO_OX-XOlokun root
add_executable(FamilyWaveguideTest FamilyWaveguideTest.cpp)
target_include_directories(FamilyWaveguideTest PRIVATE ${CMAKE_SOURCE_DIR}/..)
```

- [ ] **Step 3: Build the test**

```bash
cd ~/Documents/GitHub/XO_OX-XOlokun/Tests
mkdir -p build && cd build
cmake .. -DCMAKE_CXX_STANDARD=17
make
```

Expected: compiles with 0 errors, 0 warnings.

- [ ] **Step 4: Run the test**

```bash
./FamilyWaveguideTest
```

Expected output:
```
[PASS] DelayLine_basic_read_finite
[PASS] KS_loop_no_NaN
[PASS] KS_loop_peak_nonzero
[PASS] KS_loop_decays
[PASS] PluckExciter_no_NaN
[PASS] PluckExciter_nonzero
[PASS] PluckExciter_silent_after_burst
... (all 20 tests pass)
=== FamilyWaveguide Tests: 20 passed, 0 failed ===
```

---

### Task 14: Commit SP1

**Files:**
- All of the above

- [ ] **Step 1: Stage and commit**

```bash
cd ~/Documents/GitHub/XO_OX-XOlokun
git add Source/DSP/FamilyWaveguide.h Tests/FamilyWaveguideTest.cpp Tests/CMakeLists.txt
git commit -m "feat(SP1): add FamilyWaveguide.h — shared physical modeling DSP for XOrphica Constellation

5 waveguide primitives: FamilyDelayLine (Lagrange interp), FamilyDampingFilter (1-pole LP),
FamilyBodyResonance (biquad), FamilySympatheticBank (8 comb filters), FamilyOrganicDrift (dual-LFO wander).
7 exciters: Pluck, Strum, Pick, AirJet, Reed, LipBuzz, Bow.
All components: header-only, allocation-free, denormal-safe, namespace xolokun.
Standalone C++ test: 20 assertions, all pass (no JUCE required).

Co-Authored-By: Claude Sonnet 4.6 <noreply@anthropic.com>"
```

---

## Summary

After SP1 is complete, the build sequence continues:

- **SP2 + SP3 in parallel:** XOhm and XOrphica standalone instruments (both use FamilyWaveguide.h)
- **SP4 + SP5 in parallel:** XObbligato and XOttoni standalone instruments
- **SP6:** XOlé standalone instrument (most complex — benefits from prior learning)
- **SP7:** Constellation XOlokun integration (5 adapters, macro bleed, Family Dinner presets, Obed easter egg)

Each engine follows the pattern: `/new-xo-project` scaffold → Parameters.h → FamilyWaveguide integration → voice architecture → FX chains → macros → UI → 50 presets → auval + QA → commit.
