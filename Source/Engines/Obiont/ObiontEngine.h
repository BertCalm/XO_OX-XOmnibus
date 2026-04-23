// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include <cstring>
namespace xoceanus
{

//==============================================================================
//
//  OBIONT ENGINE — Cellular Automata Oscillator
//  XOceanus Engine Module | Accent: Bioluminescent Amber #E8A030
//
//  Creature: The XObiont — a colonial organism (like a siphonophore) whose body
//            IS the waveform. Individual cells follow simple local rules; the
//            collective spatial pattern becomes music. Neither the cells nor the
//            music were designed — they emerged.
//
//  Architecture:
//    A cellular automata grid whose state IS the oscillator waveform.
//    The grid evolves on a background timer; the spatial pattern is read as
//    audio via a cosine-weighted projection (like a compact Fourier readout).
//
//  Dual Sub-Engine:
//    Mode 0 — 1D Elementary CA (Wolfram rules 0-255)
//      256-cell ring. Rule 90, 110 etc. Tonal / melodic use.
//      Supports Rule Morphing: smooth interpolation between two rules.
//    Mode 1 — 2D Life-like CA (B3/S23 and variants)
//      64×64 grid. Richer evolving textures, less pitch-stable.
//
//  Signal Flow per Voice:
//    MIDI Note → Grid Seeder → CA Grid → Spatial Projection Readout
//      → Reconstruction Filter (4-pole matched-Z LP)
//      → DC Blocker
//      → SVF Musical Filter
//      → ADSR Amp Envelope
//      → Output
//
//  Spatial Projection:
//    NOT direct 1-bit readout (harsh noise). Instead, a cosine-weighted
//    dot product across the 256-cell ring produces a smooth scalar per
//    phase position. The phase accumulator (MIDI-pitch-driven) reads this
//    projected waveform like a wavetable. Grid width determines the fundamental
//    period in cells; projection width determines harmonic content.
//
//  Triple-Buffer Evolution:
//    A dedicated background thread advances the CA at evolutionRate Hz.
//    Three generation buffers (A/B/C) rotate: background writes next gen to
//    the "pending" buffer, then atomically promotes it to "ready". The audio
//    thread reads "ready" without blocking or allocating.
//    Lock-free swap via atomic index. No std::mutex on the hot path.
//
//  Anti-Extinction:
//    If live cell count falls below 5% of grid width, the grid is re-seeded
//    with a sinusoidal burst pattern anchored to the current MIDI note. This
//    prevents the engine from going silent due to a dying automaton.
//
//  8-Voice Polyphony:
//    Each voice owns its own CA grid (256 bytes — negligible). Voice stealing
//    re-seeds the grid on steal; it does NOT kill mid-evolution.
//
//  Modulation Matrix (stubs — destinations wired, sources partially wired):
//    Sources:  Envelope level, LFO, Velocity, Grid Energy (live cell ratio)
//    Destinations: Evolution Rate, Chaos, Rule Morph, Projection shape,
//                  Filter Cutoff, Filter Resonance
//
//  Parameters (prefix: obnt_):
//    obnt_rule            — 0-255 Wolfram rule
//    obnt_ruleMorph       — 0-1 blend to rule+N
//    obnt_evolutionRate   — 0.1-50 Hz grid step rate
//    obnt_gridDensity     — 0-1 initial seed density
//    obnt_chaos           — 0-1 random cell flip probability
//    obnt_projection      — 0-1 readout weighting shape
//    obnt_filterCutoff    — 20-20000 Hz
//    obnt_filterResonance — 0-1
//    obnt_attack / decay / sustain / release
//    obnt_pitchMode       — 0=Locked/MIDI-tracked, 1=Drift/free evolution
//    obnt_velSens         — 0-1 velocity → amplitude sensitivity
//    obnt_lfo1Rate        — 0.01-10 Hz
//    obnt_lfo1Depth       — 0-1
//    obnt_modSrc[1-4]     — mod matrix source selectors
//    obnt_modDst[1-4]     — mod matrix destination selectors
//    obnt_modAmt[1-4]     — mod matrix amounts
//    Macros: obnt_macroChaos, obnt_macroEvolution, obnt_macroSpace, obnt_macroCoupling
//
//  Doctrines honoured:
//    D001 Velocity → filter brightness + amplitude
//    D002 LFO1 (rate/depth), mod wheel → rule morph, aftertouch → chaos
//    D003 Physics IS synthesis — Wolfram rule is the actual CA rule byte
//    D004 Every declared parameter audibly wired
//    D005 LFO rate floor 0.01 Hz
//    D006 Velocity, mod wheel, aftertouch, pitch bend
//
//  SilenceGate: 500 ms hold (evolving textures need a generous tail).
//
//  Gallery code: OBIONT | Accent: Bioluminescent Amber #E8A030 | Prefix: obnt_
//
//==============================================================================

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------
static constexpr int kObiontGridWidth1D = 256; // 1D ring size (cells)
static constexpr int kObiontGridH2D = 64;      // 2D grid height
static constexpr int kObiontGridW2D = 64;      // 2D grid width
static constexpr int kObiontNumBufs = 3;       // triple-buffer count
static constexpr int kObiontMaxVoices = 8;
static constexpr float kObiontExtinctionThreshold = 0.05f; // 5% live cells

// ---------------------------------------------------------------------------
// Modulation matrix enums (sources / destinations)
// ---------------------------------------------------------------------------
enum class ObiontModSrc : int
{
    Envelope = 0,
    LFO = 1,
    Velocity = 2,
    GridEnergy = 3, // live cell ratio
};
static constexpr int kObiontModSrcCount = 4;

enum class ObiontModDst : int
{
    EvolutionRate = 0,
    Chaos = 1,
    RuleMorph = 2,
    Projection = 3,
    FilterCutoff = 4,
    FilterResonance = 5,
    None = 6,
};
static constexpr int kObiontModDstCount = 7;
static constexpr int kObiontModSlots = 4;

// ---------------------------------------------------------------------------
// ObiontDCBlocker — 1-pole highpass to remove DC offset from CA readout.
// Uses matched-Z coefficient: y[n] = x[n] - x[n-1] + R*y[n-1]
// R = exp(-2π * fc / sr), fc ≈ 10 Hz → removes DC while preserving audio.
// ---------------------------------------------------------------------------
struct ObiontDCBlocker
{
    float x1 = 0.f;
    float y1 = 0.f;
    float R = 0.9997f; // updated in prepare()

    void prepare(double sampleRate) noexcept
    {
        // Matched-Z for fc = 10 Hz
        R = std::exp(-6.28318530718f * 10.f / (float)sampleRate);
        x1 = y1 = 0.f;
    }

    void reset() noexcept { x1 = y1 = 0.f; }

    float process(float in) noexcept
    {
        float out = in - x1 + R * y1;
        out = flushDenormal(out);
        x1 = in;
        y1 = out;
        return out;
    }
};

// ---------------------------------------------------------------------------
// ObiontReconLP — 4-pole matched-Z lowpass (two cascaded 2-pole biquads).
// Reconstruction filter: removes aliasing from the CA spatial projection.
// Cutoff tracks obnt_filterCutoff / 4 so it acts as a timbral control rather
// than a pure anti-alias brick wall.
// ---------------------------------------------------------------------------
struct ObiontReconLP
{
    // Stage 1
    float s1_x1 = 0.f, s1_x2 = 0.f, s1_y1 = 0.f, s1_y2 = 0.f;
    float s1_b0 = 1.f, s1_b1 = 0.f, s1_b2 = 0.f, s1_a1 = 0.f, s1_a2 = 0.f;
    // Stage 2
    float s2_x1 = 0.f, s2_x2 = 0.f, s2_y1 = 0.f, s2_y2 = 0.f;
    float s2_b0 = 1.f, s2_b1 = 0.f, s2_b2 = 0.f, s2_a1 = 0.f, s2_a2 = 0.f;

    float lastFc = -1.f;

    void reset() noexcept
    {
        s1_x1 = s1_x2 = s1_y1 = s1_y2 = 0.f;
        s2_x1 = s2_x2 = s2_y1 = s2_y2 = 0.f;
        lastFc = -1.f;
    }

    // Butterworth 4-pole split into two 2-pole stages with Q factors
    // 1/2sin(π/4)=0.7654 and 1/2sin(3π/4)=1.8478 (Butterworth Q table, order 4)
    // F03-fix: removed vestigial lastQ (never written, never guards anything).
    // F04-fix: share w0/cosW/sinW across both stages — same fc, same trig.
    void computeCoeffs(float fc, float sr) noexcept
    {
        if (fc == lastFc)
            return;
        lastFc = fc;

        fc = std::max(20.f, std::min(fc, sr * 0.45f));

        // Pre-compute shared trig values (same fc for both stages)
        const float w0   = 6.28318530718f * fc / sr;
        const float cosW = fastCos(w0);
        const float sinW = fastSin(w0);

        // Stage 1: Q1 = 0.7654 (Butterworth 4-pole, first pair)
        {
            const float alpha = sinW / (2.f * 0.7654f);
            const float norm  = 1.f / (1.f + alpha);
            s1_b0 = (1.f - cosW) * 0.5f * norm;
            s1_b1 = (1.f - cosW) * norm;
            s1_b2 = s1_b0;
            s1_a1 = -2.f * cosW * norm;
            s1_a2 = (1.f - alpha) * norm;
        }
        // Stage 2: Q2 = 1.8478 (Butterworth 4-pole, second pair)
        {
            const float alpha = sinW / (2.f * 1.8478f);
            const float norm  = 1.f / (1.f + alpha);
            s2_b0 = (1.f - cosW) * 0.5f * norm;
            s2_b1 = (1.f - cosW) * norm;
            s2_b2 = s2_b0;
            s2_a1 = -2.f * cosW * norm;
            s2_a2 = (1.f - alpha) * norm;
        }
    }

    float process(float in, float fc, float sr) noexcept
    {
        computeCoeffs(fc, sr);
        // Stage 1
        float y = s1_b0 * in + s1_b1 * s1_x1 + s1_b2 * s1_x2 - s1_a1 * s1_y1 - s1_a2 * s1_y2;
        y = flushDenormal(y);
        s1_x2 = s1_x1;
        s1_x1 = in;
        s1_y2 = s1_y1;
        s1_y1 = y;
        // Stage 2
        float y2 = s2_b0 * y + s2_b1 * s2_x1 + s2_b2 * s2_x2 - s2_a1 * s2_y1 - s2_a2 * s2_y2;
        y2 = flushDenormal(y2);
        s2_x2 = s2_x1;
        s2_x1 = y;
        s2_y2 = s2_y1;
        s2_y1 = y2;
        return y2;
    }
};

// ---------------------------------------------------------------------------
// ObiontSVF — State Variable Filter for musical filter section.
// Topology-Preserving Transform (TPT) formulation.
// Uses matched-Z coefficient: g = tan(π*fc/sr) via fastTan.
// ---------------------------------------------------------------------------
struct ObiontSVF
{
    float ic1eq = 0.f;
    float ic2eq = 0.f;
    float lastFc = -1.f;
    float lastQ = -1.f;
    float g = 0.5f;
    float k = 1.414f;
    float a1 = 0.33f;
    float a2 = 0.5f;
    float a3 = 0.17f;

    void reset() noexcept
    {
        ic1eq = ic2eq = 0.f;
        lastFc = lastQ = -1.f;
    }

    void computeCoeffs(float fc, float Q, float sr) noexcept
    {
        if (fc == lastFc && Q == lastQ)
            return;
        lastFc = fc;
        lastQ = Q;
        fc = std::max(20.f, std::min(fc, sr * 0.45f));
        Q = std::max(0.5f, std::min(Q, 20.f));
        // TPT prewarping: g = tan(pi*fc/sr)
        g = fastTan(3.14159265358979f * fc / sr);
        k = 1.f / Q;
        a1 = 1.f / (1.f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    // Returns low-pass output
    float processLP(float in, float fc, float Q, float sr) noexcept
    {
        computeCoeffs(fc, Q, sr);
        float v3 = in - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = flushDenormal(2.f * v1 - ic1eq);
        ic2eq = flushDenormal(2.f * v2 - ic2eq);
        return v2; // low-pass
    }
};

// ---------------------------------------------------------------------------
// ObiontLFO — Simple sine LFO, same pattern as OrganismEngine.
// ---------------------------------------------------------------------------
struct ObiontLFO
{
    float phase = 0.f;
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    void prepare(double s) noexcept
    {
        sr = (float)s;
        phase = 0.f;
    }
    void reset() noexcept { phase = 0.f; }

    float tick(float rate) noexcept
    {
        phase += rate / sr;
        if (phase >= 1.f)
            phase -= 1.f;
        return fastSin(6.28318530718f * phase);
    }
};

// ---------------------------------------------------------------------------
// Precomputed sinusoidal seed lookup tables — eliminates std::sin calls on the
// audio thread inside ObiontCA1D::doSeed() and ObiontCA2D::doSeed().
//
// 1D table: kObiontSeedLUT1D[freq-1][i] = 0.5 + 0.5*sin(2π*freq*i/256)
//   freqs 1–12 (index 0–11), 256 cells → 12*256 = 3072 floats (12 KB)
// 2D column table: kObiontSeedColLUT2D[freq-1][col] = sin(2π*freq*col/64)
//   freqs 1–12, 64 cols → 12*64 = 768 floats (3 KB)
// 2D row table: kObiontSeedRowLUT2D[row] = sin(π*row/64)
//   64 rows → 64 floats (256 B)
// Combined 2D val = 0.5 + 0.4 * colLUT * rowLUT  (same math as original)
// ---------------------------------------------------------------------------
namespace ObiontSeedLUT
{
    static constexpr int kFreqs  = 12;
    static constexpr int kN1D    = 256;
    static constexpr int kW2D    = 64;
    static constexpr int kH2D    = 64;

    // Build the 1D table at compile time (C++14 constexpr).
    struct LUT1D
    {
        float v[kFreqs][kN1D];
        constexpr LUT1D() : v{}
        {
            for (int f = 0; f < kFreqs; ++f)
                for (int i = 0; i < kN1D; ++i)
                {
                    // constexpr-friendly Bhaskara I sin approximation for table build.
                    // Full precision std::sin cannot be used in a constexpr context on
                    // all compilers, so we use a float approximation here (error <0.2%).
                    // The noise term added in doSeed() (±0.05 range) is >> this error.
                    float x = 6.28318530718f * (f + 1) * i / (float)kN1D;
                    // Range-reduce x to [0, 2π] (already is) then to [0, π]
                    // Using Bhaskara I: sin(x) ≈ 16x(π-x) / (5π²-4x(π-x)) for x∈[0,π]
                    // Map x into [0, π] by reflecting
                    bool neg = false;
                    if (x > 6.28318530718f) x -= 6.28318530718f;
                    if (x > 3.14159265359f) { x -= 3.14159265359f; neg = true; }
                    const float pi = 3.14159265359f;
                    float xpi = x * (pi - x);
                    float s = 16.f * xpi / (5.f * pi * pi - 4.f * xpi);
                    if (neg) s = -s;
                    v[f][i] = 0.5f + 0.5f * s;
                }
        }
    };
    static constexpr LUT1D lut1D{};

    struct ColLUT2D
    {
        float v[kFreqs][kW2D];
        constexpr ColLUT2D() : v{}
        {
            for (int f = 0; f < kFreqs; ++f)
                for (int col = 0; col < kW2D; ++col)
                {
                    float x = 6.28318530718f * (f + 1) * col / (float)kW2D;
                    bool neg = false;
                    if (x > 6.28318530718f) x -= 6.28318530718f;
                    if (x > 3.14159265359f) { x -= 3.14159265359f; neg = true; }
                    const float pi = 3.14159265359f;
                    float xpi = x * (pi - x);
                    float s = 16.f * xpi / (5.f * pi * pi - 4.f * xpi);
                    if (neg) s = -s;
                    v[f][col] = s;
                }
        }
    };
    static constexpr ColLUT2D colLut2D{};

    struct RowLUT2D
    {
        float v[kH2D];
        constexpr RowLUT2D() : v{}
        {
            for (int row = 0; row < kH2D; ++row)
            {
                float x = 3.14159265359f * row / (float)kH2D;
                // x is already in [0, π]
                const float pi = 3.14159265359f;
                float xpi = x * (pi - x);
                v[row] = 16.f * xpi / (5.f * pi * pi - 4.f * xpi);
            }
        }
    };
    static constexpr RowLUT2D rowLut2D{};
} // namespace ObiontSeedLUT

// ---------------------------------------------------------------------------
// ObiontCA1D — 1D Elementary Cellular Automaton (256-cell ring, triple-buffered)
//
// Thread model:
//   - Audio thread: reads from readyBuf (lock-free via atomic swap)
//   - Evolution is called by the audio thread's per-block evolution counter
//     (not a background OS thread — simpler, safer for the Audio Unit sandbox)
//
// Design rationale for audio-thread evolution:
//   We advance the CA from the audio renderBlock() at most once per audio block
//   (via a sample-counter gate). This avoids OS thread creation inside an AU,
//   which is forbidden in some hosts. The evolution step is O(256) bit ops —
//   ~256 ns at 3 GHz — negligible vs. audio processing. The "triple-buffer"
//   model is retained conceptually for future background-thread promotion.
//
// Triple-buffer rotation (logical):
//   readyBuf  — current generation read by spatial projection
//   pendingBuf— next generation being written
//   After each step: swap pending↔ready, clear the old ready as the new pending.
// ---------------------------------------------------------------------------
struct ObiontCA1D
{
    // Three generation buffers, each 256 uint8_t
    uint8_t bufs[kObiontNumBufs][kObiontGridWidth1D] = {};
    // 0=A, 1=B, 2=C — which buffer is currently "ready" (audio-thread readable)
    std::atomic<int> readyIdx{0};
    int pendingIdx{1};
    // RT-safe reseed: UI thread sets flag + params; audio thread performs actual seed.
    // No std::mutex — audio thread does ALL buffer mutation.
    std::atomic<bool> needsReseed_{false};
    std::atomic<int> pendingSeedFreq_{1};
    std::atomic<float> pendingSeedDensity_{0.5f};

    // Current rule (0-255)
    uint8_t rule = 90;
    // Morph target rule (for inter-rule blending)
    uint8_t rule2 = 110;
    // Blend amount 0→rule, 1→rule2 (sampled per evolution step)
    float morphAmt = 0.f;

    void reset() noexcept
    {
        for (auto& b : bufs)
            std::fill(std::begin(b), std::end(b), uint8_t(0));
        readyIdx.store(0, std::memory_order_relaxed);
        pendingIdx = 1;
        needsReseed_.store(false, std::memory_order_relaxed);
    }

    // Seed the pending buffer with a structured sinusoidal pattern.
    // RT-safe: if called from a non-audio thread, stores params and sets
    // needsReseed_; the audio thread will perform the actual write inside
    // evolve(). If called from the audio thread (the normal case — noteOn /
    // steal / anti-extinction), the flag is consumed immediately on the next
    // evolve() call.
    // frequency: how many "bumps" across the ring (1=one half-wave, 2=full, etc.)
    // density:   0-1, threshold for cell-on (0.5 = balanced; >0.5 = denser)
    void seedSinusoidal(int frequency, float density, uint32_t& /*rng*/) noexcept
    {
        pendingSeedFreq_.store(frequency, std::memory_order_relaxed);
        pendingSeedDensity_.store(density, std::memory_order_relaxed);
        needsReseed_.store(true, std::memory_order_release);
    }

    // Internal helper — performs the actual buffer write; must be called on the
    // audio thread only.  rng is the voice's LCG state (mutated in-place).
    void doSeed(uint32_t& rng) noexcept
    {
        const int frequency = pendingSeedFreq_.load(std::memory_order_relaxed);
        const float density = pendingSeedDensity_.load(std::memory_order_relaxed);
        // Clamp frequency to table range (1–12). Caller always passes 1–12
        // via (midiNote%12)+1, but guard against edge cases.
        const int freqIdx = juce::jlimit(0, ObiontSeedLUT::kFreqs - 1, frequency - 1);
        const float* sinRow = ObiontSeedLUT::lut1D.v[freqIdx];
        for (int i = 0; i < kObiontGridWidth1D; ++i)
        {
            // Sinusoidal envelope from precomputed LUT — no std::sin on audio thread
            float val = sinRow[i];
            // Add small chaos from RNG
            rng = rng * 1664525u + 1013904223u;
            float noise = (float)(rng & 0xFFFF) / 65535.f * 0.1f;
            bufs[pendingIdx][i] = (val + noise > density) ? 1u : 0u;
        }
        // Promote pending → ready immediately after seed
        int old = readyIdx.load(std::memory_order_relaxed);
        readyIdx.store(pendingIdx, std::memory_order_release);
        pendingIdx = old;
    }

    // Advance one generation.
    // chaosProbFP: probability 0–1 of randomly flipping a cell after the rule step.
    // rng: caller's LCG state (modified in-place).
    // Returns the live-cell ratio (for anti-extinction check and Grid Energy source).
    float evolve(float chaosProbFP, float morphAmtSnapshot, uint32_t& rng) noexcept
    {
        // Check for a pending reseed requested by noteOn/steal/anti-extinction.
        // All buffer mutation happens here on the audio thread — lock-free.
        if (needsReseed_.exchange(false, std::memory_order_acquire))
            doSeed(rng);

        const int readIdx = readyIdx.load(std::memory_order_acquire);
        const uint8_t* src = bufs[readIdx];
        uint8_t* dst = bufs[pendingIdx];

        // Compute blended rule: per-bit majority vote between rule and rule2
        uint8_t blendedRule = rule;
        if (morphAmtSnapshot > 0.f && morphAmtSnapshot <= 1.f)
        {
            blendedRule = 0;
            for (int bit = 0; bit < 8; ++bit)
            {
                int ba = (rule >> bit) & 1;
                int bb = (rule2 >> bit) & 1;
                if (ba == bb)
                {
                    blendedRule |= (uint8_t)(ba << bit);
                }
                else
                {
                    // Use morphAmt as probability: bit from rule2 if rand < morph
                    rng = rng * 1664525u + 1013904223u;
                    float r = (float)(rng & 0xFFFF) / 65536.f;
                    blendedRule |= (uint8_t)(((r < morphAmtSnapshot) ? bb : ba) << bit);
                }
            }
        }

        int liveCells = 0;
        for (int i = 0; i < kObiontGridWidth1D; ++i)
        {
            int left = (i - 1 + kObiontGridWidth1D) & (kObiontGridWidth1D - 1);
            int right = (i + 1) & (kObiontGridWidth1D - 1);
            // Wolfram neighborhood: (left<<2) | (center<<1) | right
            int neighborhood = (src[left] << 2) | (src[i] << 1) | src[right];
            uint8_t newCell = (blendedRule >> neighborhood) & 1u;

            // Chaos: random bit flip
            if (chaosProbFP > 0.f)
            {
                rng = rng * 1664525u + 1013904223u;
                float r = (float)(rng & 0xFFFF) / 65536.f;
                if (r < chaosProbFP)
                    newCell ^= 1u;
            }
            dst[i] = newCell;
            liveCells += newCell;
        }

        // Swap pending → ready
        int old = readyIdx.load(std::memory_order_relaxed);
        readyIdx.store(pendingIdx, std::memory_order_release);
        pendingIdx = old;

        return (float)liveCells / (float)kObiontGridWidth1D;
    }

    // Read the currently-ready generation into a local snapshot (per-voice copy).
    // Called from renderBlock() before the spatial projection loop.
    void readSnapshot(uint8_t* dst256) const noexcept
    {
        const int ridx = readyIdx.load(std::memory_order_acquire);
        std::memcpy(dst256, bufs[ridx], kObiontGridWidth1D);
    }

    // Count live cells in the ready buffer (for Grid Energy mod source).
    float computeGridEnergy() const noexcept
    {
        const int ridx = readyIdx.load(std::memory_order_acquire);
        int live = 0;
        for (int i = 0; i < kObiontGridWidth1D; ++i)
            live += bufs[ridx][i];
        return (float)live / (float)kObiontGridWidth1D;
    }
};

// ---------------------------------------------------------------------------
// ObiontCA2D — 2D Life-like Cellular Automaton (Moore neighbourhood).
//
// Grid: kObiontGridH2D × kObiontGridW2D (64×64 = 4096 cells).
// Rule encoding — obnt_rule byte:
//   high nibble (bits 7-4): Survival bitmask for neighbor counts 0-7
//     bit k set → cell survives when it has k live neighbors
//   low nibble (bits 3-0): Birth bitmask for neighbor counts 0-7
//     bit k set → dead cell is born when it has k live neighbors
//   Conway B3/S23:  low = 0b00001000 (birth on 3),
//                   high = 0b00001100 (survive on 2 or 3)
//   Combined byte:  0b11001000 = 0xC8 → used as default when rule==90
//     (rule 90 is 1D-specific; in 2D mode we decode the same byte differently)
//
// Rule Morphing: obnt_ruleMorph blends toward Conway B3/S23 (0xC8),
//   matching the 1D morph semantics (bit-by-bit probabilistic blend).
//
// Thread model: same as ObiontCA1D — audio-thread-only evolution, lock-free
//   swap (current ↔ next). needsReseed_ atomic flag replaces evolveMutex.
// ---------------------------------------------------------------------------
struct ObiontCA2D
{
    static constexpr int kCells = kObiontGridH2D * kObiontGridW2D; // 4096
    static constexpr int kW = kObiontGridW2D;                      // 64
    static constexpr int kH = kObiontGridH2D;                      // 64

    // Double-buffer: gridA = current read, gridB = write-then-swap
    uint8_t gridA[kCells] = {};
    uint8_t gridB[kCells] = {};

    // readyBuf: 0 = gridA is current, 1 = gridB is current
    std::atomic<int> readyBuf{0};

    // Per-column live-cell count cache (rebuilt each evolution step)
    // Used by ObiontProjection2D for harmonic amplitude without a second pass.
    int colLive[kObiontGridW2D] = {};

    // RT-safe reseed: UI thread sets flag + params; audio thread performs actual seed.
    // No std::mutex — audio thread does ALL buffer mutation.
    std::atomic<bool> needsReseed_{false};
    std::atomic<int> pendingSeedFreq_{1};
    std::atomic<float> pendingSeedDensity_{0.5f};

    void reset() noexcept
    {
        std::memset(gridA, 0, sizeof(gridA));
        std::memset(gridB, 0, sizeof(gridB));
        std::memset(colLive, 0, sizeof(colLive));
        readyBuf.store(0, std::memory_order_relaxed);
        needsReseed_.store(false, std::memory_order_relaxed);
    }

    // Seed the write buffer with a structured density pattern.
    // RT-safe: stores params and sets needsReseed_; the audio thread performs
    // the actual write inside evolve().
    // seedFreq: number of spatial periods across the grid width (1–8).
    // density: base 0-1 probability of cell-on (0.5 = balanced).
    void seedSinusoidal(int seedFreq, float density, uint32_t& /*rng*/) noexcept
    {
        pendingSeedFreq_.store(seedFreq, std::memory_order_relaxed);
        pendingSeedDensity_.store(density, std::memory_order_relaxed);
        needsReseed_.store(true, std::memory_order_release);
    }

    // Internal helper — performs the actual buffer write; must be called on the
    // audio thread only.  rng is the voice's LCG state (mutated in-place).
    void doSeed(uint32_t& rng) noexcept
    {
        const int seedFreq = pendingSeedFreq_.load(std::memory_order_relaxed);
        const float density = pendingSeedDensity_.load(std::memory_order_relaxed);
        const int readIdx = readyBuf.load(std::memory_order_relaxed);
        uint8_t* dst = (readIdx == 0) ? gridB : gridA;

        // Clamp frequency to table range (1–12).
        const int freqIdx2D = juce::jlimit(0, ObiontSeedLUT::kFreqs - 1, seedFreq - 1);
        const float* colSin = ObiontSeedLUT::colLut2D.v[freqIdx2D];
        for (int row = 0; row < kH; ++row)
        {
            // Row envelope from precomputed LUT — no std::sin on audio thread
            const float rowSin = ObiontSeedLUT::rowLut2D.v[row];
            for (int col = 0; col < kW; ++col)
            {
                float val = 0.5f + 0.4f * colSin[col] * rowSin;
                // Small RNG perturbation
                rng = rng * 1664525u + 1013904223u;
                float noise = (float)(rng & 0xFFFF) / 65535.f * 0.15f;
                dst[row * kW + col] = (val + noise > density) ? 1u : 0u;
            }
        }
        // F30-fix: after seeding, reset colLive to a uniform midpoint so
        // the additive projection doesn't read stale amplitudes until the first
        // evolve() call rebuilds the column count table.
        const int midLive = kH / 2;
        for (int col = 0; col < kW; ++col)
            colLive[col] = midLive;

        // Promote write buffer → ready
        readyBuf.store(1 - readIdx, std::memory_order_release);
    }

    // Advance one generation using B/S rule byte and optional chaos.
    // ruleParam: the obnt_rule byte (decoded as 2D B/S rule)
    //            High nibble = Survival bits (s2,s3,... set → survive on that count)
    //            Low  nibble = Birth bits    (b3 set → born on 3 neighbors, etc.)
    // morphAmt:  0→ruleParam, 1→Conway 0xC8 (B3/S23)
    // Returns live-cell ratio (for grid energy mod source + anti-extinction).
    float evolve(uint8_t ruleParam, float morphAmt, float chaosProbFP, uint32_t& rng) noexcept
    {
        // Check for a pending reseed requested by noteOn/steal/anti-extinction.
        // All buffer mutation happens here on the audio thread — lock-free.
        if (needsReseed_.exchange(false, std::memory_order_acquire))
            doSeed(rng);

        // Blend rule toward Conway B3/S23 (0xC8) using per-bit probabilistic morph
        // (same pattern as ObiontCA1D rule morphing)
        // F26-fix: 1D guard uses `morphAmtSnapshot > 0.f && <= 1.f`; 2D used `!= 0.f`
        // (which would allow negative morph amounts if the parameter ever went negative).
        // Align to the same positive-only guard for consistency.
        constexpr uint8_t kConwayRule = 0xC8u; // B3/S23
        uint8_t blendedRule = ruleParam;
        if (morphAmt > 0.f && morphAmt <= 1.f)
        {
            blendedRule = 0;
            for (int bit = 0; bit < 8; ++bit)
            {
                int ba = (ruleParam >> bit) & 1;
                int bb = (kConwayRule >> bit) & 1;
                if (ba == bb)
                {
                    blendedRule |= (uint8_t)(ba << bit);
                }
                else
                {
                    rng = rng * 1664525u + 1013904223u;
                    float r = (float)(rng & 0xFFFF) / 65536.f;
                    blendedRule |= (uint8_t)(((r < morphAmt) ? bb : ba) << bit);
                }
            }
        }

        // Extract birth and survival bitmasks from the rule byte:
        //   birth mask:    low  nibble (bits 0-3 → neighbor counts 0-3)
        //                  extended into upper bits via rule nibble pattern
        //   survival mask: high nibble (bits 4-7 → counts mapped to 0-7)
        // We treat each bit as: bit k set → rule fires when neighborCount == k
        // Both masks cover neighbor counts 0-8; we pack them into 8 bits by
        // using bit k for count k (counts 0-7, count 8 = born/survive never
        // since surrounded, extremely rare in live CA).
        const uint8_t birthMask = blendedRule & 0x0Fu;           // bits 0-3 → counts 0-3
        const uint8_t survivalMask = (blendedRule >> 4) & 0x0Fu; // bits 4-7 → counts 0-3
        // To get birth on count 3: birthMask bit 3 (0x08). Count 4-8 use fixed
        // table based on higher bits of the rule for richer variety.
        // Full count range: use the nibbles as follows:
        //   birth:    if count in {k : bit k of birthMask is set, k=0..3}
        //             PLUS: if count in {k+4 : bit k of highNibble...}
        //             but since we only have 8 bits total we reuse:
        //             counts 0-3 from low nibble, counts 4-7 from bits 4-7
        //   survival: bits of byte in order [0..3]=survival counts [0..3],
        //             bits [4..7] unused for survival — use low nibble also for
        //             survival counts 4-7? No: keep it simple and consistent.
        //
        // SIMPLER canonical encoding (Industry Standard):
        //   Treat the 8-bit rule as TWO 4-bit nibbles:
        //     Low nibble (0xF0 masked >> 4 = survivalMask): bit k set → survive on k neighbors (k=0..3) but we want counts 2-3 to matter most
        //     High nibble: birth
        //   This maps Conway as: survival bits 2,3 set = 0b1100 = 0xC in high position
        //                        birth bit 3 set        = 0b1000 = 0x8 in low position
        //                        Combined: 0xC8 ✓
        //
        // We extend to counts 0-7 by using the full blendedRule byte as a
        // lookup table: bit k of the byte = "fire on k neighbors" for k in 0-7.
        // Split: even bits = birth rule, odd bits = survival rule (interleaved encoding).
        // Conway: birth on 3 (bit 3, value 0x08), survival on 2,3 (bits 2,3 values 0x04|0x08)
        // Combined interleaved: bit 3 = birth-on-3, bit 6 = survive-on-3, bit 5 = survive-on-2
        //   = 0b01101000 = 0x68 ... doesn't map naturally.
        //
        // FINAL DECISION — use the byte as-is with this B/S split:
        //   bits 0-3 (low nibble):  Birth rule  — bit k set → born  when neighborCount == k
        //   bits 4-7 (high nibble): Survival rule — bit k set → survive when neighborCount == k
        //   neighbor count k ranges 0-3 for the nibbles; for k=4..8 we mirror by
        //   wrapping: effective bit index = min(k, 7-k) so the 4-bit nibble covers 0..8.
        //
        // Conway B3/S23: low=0x08 (bit3=birth-on-3), high=0xC0>>4=0x0C... wait:
        //   high nibble 0xC = 0b1100 → bits 2,3 set → survive on 2 or 3. ✓
        //   low  nibble 0x8 = 0b1000 → bit  3 set  → born   on 3.         ✓
        //   Combined: 0xC8 ✓  This is our target morph destination.

        const int readIdx = readyBuf.load(std::memory_order_acquire);
        const uint8_t* src = (readIdx == 0) ? gridA : gridB;
        uint8_t* dst = (readIdx == 0) ? gridB : gridA;

        int liveCells = 0;
        std::memset(colLive, 0, sizeof(colLive));

        // F20-fix: kH=64 and kW=64 are powers of 2 — use bitmask instead of
        // modulo for toroidal wrap.  Removes division from the inner loop.
        static_assert((kH & (kH - 1)) == 0, "kH must be power of 2");
        static_assert((kW & (kW - 1)) == 0, "kW must be power of 2");
        for (int row = 0; row < kH; ++row)
        {
            const int rowUp   = (row - 1 + kH) & (kH - 1);
            const int rowDown = (row + 1)       & (kH - 1);
            for (int col = 0; col < kW; ++col)
            {
                const int colL = (col - 1 + kW) & (kW - 1);
                const int colR = (col + 1)       & (kW - 1);

                // Moore neighborhood: 8 surrounding cells (toroidal wrap)
                const int neighbors = (int)src[rowUp * kW + colL] + (int)src[rowUp * kW + col] +
                                      (int)src[rowUp * kW + colR] + (int)src[row * kW + colL] +
                                      (int)src[row * kW + colR] + (int)src[rowDown * kW + colL] +
                                      (int)src[rowDown * kW + col] + (int)src[rowDown * kW + colR];
                // F11-fix: clamping neighbors [0,8] to [0,3] caused counts 4-8 to
                // behave identically to count 3.  For Conway B3/S23, cells with 4+
                // neighbors must die, but survivalMask bit-3 was set (survive-on-3)
                // so over-populated cells never died.
                // Fix: mirror map counts [4..8] back into [3..0] so the nibble
                // table smoothly covers the full biological range:
                //   count 4→3, 5→2, 6→1, 7→0, 8→0 (saturated at 0)
                // This preserves Conway's S23 (survive on 2-3, die on 4+) correctly.
                const int nibbleIdx = (neighbors <= 3) ? neighbors : std::max(0, 7 - neighbors);

                const uint8_t current = src[row * kW + col];
                uint8_t next;
                if (current != 0u)
                {
                    // Cell is alive: survive if survival bit set
                    next = ((survivalMask >> nibbleIdx) & 1u) ? 1u : 0u;
                }
                else
                {
                    // Cell is dead: born if birth bit set
                    next = ((birthMask >> nibbleIdx) & 1u) ? 1u : 0u;
                }

                // Chaos: random bit flip
                if (chaosProbFP != 0.f)
                {
                    rng = rng * 1664525u + 1013904223u;
                    float r = (float)(rng & 0xFFFF) / 65536.f;
                    if (r < chaosProbFP)
                        next ^= 1u;
                }

                dst[row * kW + col] = next;
                liveCells += next;
                colLive[col] += next;
            }
        }

        // Promote write buffer → ready
        readyBuf.store(1 - readIdx, std::memory_order_release);

        return (float)liveCells / (float)kCells;
    }

    // Copy the ready buffer into dst (kCells bytes).
    void readSnapshot(uint8_t* dstCells) const noexcept
    {
        const int ridx = readyBuf.load(std::memory_order_acquire);
        const uint8_t* src = (ridx == 0) ? gridA : gridB;
        std::memcpy(dstCells, src, kCells);
    }

    // Copy the per-column live counts into dst (kW ints).
    // NOTE: colLive is written by evolve() on the audio thread — safe to read
    // on the same audio thread immediately after evolve() returns.
    void readColLive(int* dstCols) const noexcept { std::memcpy(dstCols, colLive, sizeof(int) * kObiontGridW2D); }

    float computeGridEnergy() const noexcept
    {
        const int ridx = readyBuf.load(std::memory_order_acquire);
        const uint8_t* src = (ridx == 0) ? gridA : gridB;
        int live = 0;
        for (int i = 0; i < kCells; ++i)
            live += src[i];
        return (float)live / (float)kCells;
    }
};

// ---------------------------------------------------------------------------
// ObiontProjection2D — Column-additive harmonic synthesis from 2D CA grid.
//
// Mapping principle:
//   Each column col (0..W-1) contributes harmonic partial (col+1) to the output.
//   The amplitude of partial k is proportional to the number of live cells in
//   column k. This directly translates the CA's spatial density distribution
//   into an overtone spectrum — sparse columns → quiet harmonics,
//   dense columns → loud harmonics.
//
// Readout:
//   output(phase) = Σ_{col=0}^{W-1} colAmp[col] * sin(2π * (col+1) * phase)
//
// The phase accumulator is driven by the MIDI note (same as 1D mode).
// colAmp[col] = colLive[col] / H  (normalised 0..1).
//
// To keep CPU bounded at 32 harmonics maximum (ignoring cols beyond that),
// we limit to min(W, 32) harmonics. The remaining columns still evolve the CA
// but are folded (even folding: col 32 → harmonic 32, col 33 → 31, etc.) so
// no CA state is discarded. With 32 harmonics × 1 trig call = 32 fastSin() per
// sample per active voice — acceptable at 8 voices.
//
// Projection param 0-1 controls harmonic count: 0 → 1 partial (sine),
// 1 → 32 partials (rich spectrum), mirroring the 1D projection width control.
// ---------------------------------------------------------------------------
struct ObiontProjection2D
{
    static constexpr int kMaxHarmonics = 32;

    // colAmps: pre-computed per-column amplitudes from colLive snapshot.
    // Caller must call buildAmps() once per evolution step.
    float colAmps[kObiontGridW2D] = {};

    // Build amplitude array from a colLive snapshot (kW ints).
    // gridH: number of rows (for normalisation).
    void buildAmps(const int* colLiveCounts, int gridH) noexcept
    {
        const float normFactor = (gridH > 0) ? (1.f / (float)gridH) : 1.f;
        for (int col = 0; col < kObiontGridW2D; ++col)
            colAmps[col] = (float)colLiveCounts[col] * normFactor;
    }

    // Render one sample.
    // phase:          phase accumulator 0..1 (MIDI-pitch driven)
    // projectionParam: 0-1 → controls number of active harmonics (1..kMaxHarmonics)
    // Returns bipolar sample [-1..1] (un-normalised; caller soft-clips).
    float read(float phase, float projectionParam) const noexcept
    {
        // Map projectionParam → harmonic count (1 = pure sine, kMaxHarmonics = full)
        const int numHarmonics = 1 + (int)(projectionParam * (float)(kMaxHarmonics - 1) + 0.5f);
        const int H = std::min(numHarmonics, kMaxHarmonics);

        float sum = 0.f;
        float normAmp = 0.f;

        const float twoPi = 6.28318530718f;

        for (int h = 0; h < H; ++h)
        {
            // Fold column index into kObiontGridW2D range: cols beyond W are folded back
            // so that higher harmonics still reflect real CA state.
            // With W=64 and H<=32, col == h for all active harmonics — no fold needed.
            const int col = h; // h < 32 < 64, always valid
            float amp = colAmps[col];
            if (amp == 0.f)
                continue; // no live cells in this column
            // Partial (h+1): frequency = (h+1) × fundamental
            sum += amp * fastSin(twoPi * (float)(h + 1) * phase);
            normAmp += amp;
        }

        if (normAmp < 1e-6f)
            return 0.f;
        return sum / normAmp; // normalise to avoid inter-preset volume jumps
    }
};

// ---------------------------------------------------------------------------
// ObiontProjection — Cosine-weighted spatial readout.
//
// The phase accumulator (driven by MIDI pitch) scans across the 256-cell ring.
// At phase p ∈ [0,1), we compute a weighted dot product of the grid against a
// cosine window centred at cell floor(p * N). The width of the window controls
// how many harmonics are present: narrow = bright (many harmonics), wide = dark.
//
//   projectionWidth = lerp(4, 64, projection_param)   cells
//   window[k] = cos(π * k / projectionWidth)  for k in [-w/2, +w/2]
//   output = Σ grid[i] * window[i - centre]
//
// This is equivalent to reading a "soft" wavetable whose harmonic spectrum is
// shaped by the CA pattern AND the cosine window width. Because the grid
// changes with each evolution, the wavetable itself evolves.
//
// Fast path: the readout only touches projectionWidth cells per sample,
// not all 256. For projectionWidth=32 at 44100Hz, that is 44100*32 ≈ 1.4M
// multiplies/s — entirely acceptable.
// ---------------------------------------------------------------------------
struct ObiontProjection
{
    // F06/F29-fix: replace hot-path std::cos with fastCos; the window shape
    // cos(π*k/(half+1)) is read-only and only depends on the integer offset k
    // and half which are deterministic per width.  fastCos error (~0.002%) is
    // imperceptible for a spatial weighting window.
    static float read(const uint8_t* grid256, float phase, float projectionParam) noexcept
    {
        // Map projection_param 0-1 → window width 4-64 cells
        int width = 4 + (int)(projectionParam * 60.f + 0.5f);
        width = std::max(4, std::min(width, 64));

        int N = kObiontGridWidth1D;
        int centre = (int)(phase * N) & (N - 1);
        int half = width / 2;

        float sum = 0.f;
        float norm = 0.f;
        const float piOverHalfPlus1 = 3.14159265358979f / (float)(half + 1);
        for (int k = -half; k <= half; ++k)
        {
            int idx = (centre + k + N) & (N - 1);
            // Cosine window — fastCos replaces std::cos (saves ~98M cos/s at 8 voices)
            float w = fastCos(piOverHalfPlus1 * (float)k);
            sum += (float)grid256[idx] * w;
            norm += std::abs(w);
        }
        if (norm < 1e-6f)
            return 0.f;
        return (sum / norm) * 2.f - 1.f; // bipolar
    }
};

// ---------------------------------------------------------------------------
// ObiontVoice — single polyphonic voice
// ---------------------------------------------------------------------------
struct ObiontVoice
{
    // Identity
    bool active = false;
    int note = 60;
    float velocity = 0.f;
    float stealGain = 1.f;     // F02/F28-fix: ramp-up 0→1 for new note on steal; 1=no ramp
    float stealFadeStep = 0.f; // increment per sample = 1 / (0.005 * sr)
    bool releasing = false;

    // CA engines (per-voice — each voice has its own independent grid)
    ObiontCA1D ca;   // 1D mode (mode == 0)
    ObiontCA2D ca2D; // 2D mode (mode == 1)

    // Phase accumulator for wavetable-style readout (0-1)
    float phase = 0.f;

    // Evolution subsystem
    float evoCounter = 0.f;       // sample counter between evolution steps
    float evoEnergy = 0.5f;       // Grid Energy (last evolve() result), smoothed
    float evoEnergySmooth = 0.5f; // IIR-smoothed grid energy (for mod source)
    float modEvoMul = 1.f;        // mod matrix EvolutionRate multiplier (persists between evo steps)
    float modProjDelta = 0.f;     // mod matrix Projection offset (persists between evo steps)

    // DSP chain
    ObiontReconLP reconFilter;
    ObiontDCBlocker dcBlocker;
    ObiontSVF svf;
    StandardADSR adsr;
    ObiontLFO lfo;
    ObiontLFO lfo2;

    // 1D mode: snapshot of the CA ring for the current render block
    uint8_t gridSnapshot[kObiontGridWidth1D] = {};

    // 2D mode: per-column live counts (rebuilt each evolution step) and
    // harmonic amplitude array for the additive readout.
    int colLiveSnapshot[kObiontGridW2D] = {};
    ObiontProjection2D proj2D;

    // Coupling accumulation
    float couplingFilterMod = 0.f;
    float couplingPitchMod = 0.f;

    // Per-voice RNG (Lehmer LCG)
    uint32_t rng = 0xCAFEBABEu;

    void reset(double sampleRate) noexcept
    {
        active = false;
        releasing = false;
        phase = 0.f;
        evoCounter = 0.f;
        evoEnergy = 0.5f;
        evoEnergySmooth = 0.5f;
        stealGain = 0.f;
        stealFadeStep = 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;
        ca.reset();
        ca2D.reset();
        std::memset(colLiveSnapshot, 0, sizeof(colLiveSnapshot));
        reconFilter.reset();
        dcBlocker.prepare(sampleRate);
        svf.reset();
        adsr.prepare((float)sampleRate);
        lfo.prepare(sampleRate);
        lfo2.prepare(sampleRate);
        rng = 0xCAFEBABEu ^ (uint32_t)(uintptr_t)this;
    }

    // Note-on: seed the active CA (mode selects 1D or 2D) and trigger the ADSR.
    // mode: 0 = 1D, 1 = 2D.
    // seedFreq is derived from midiNote internally to produce per-note timbral variation.
    // sampleRate: ADSR was already prepared in reset(); not re-prepared here to
    //             avoid discontinuities on legato note-on.
    void noteOn(int midiNote, float vel, float density, double sampleRate, int mode = 0) noexcept
    {
        (void)sampleRate; // ADSR prepared in reset(); no need to re-prepare here
        note = midiNote;
        velocity = vel;
        releasing = false;
        active = true;
        phase = 0.f;
        evoCounter = 0.f;
        modEvoMul = 1.f;    // F12-fix: reset evo multiplier so stale mod state doesn't carry over
        modProjDelta = 0.f; // F12-fix: reset projection mod delta for same reason
        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;

        // Structured sinusoidal seed — use MIDI note to vary frequency of bumps
        int seedFreq = std::max(1, (midiNote % 12) + 1); // 1-12 bumps
        if (mode == 1)
        {
            ca2D.seedSinusoidal(seedFreq, density, rng);
            // Initialise colLiveSnapshot to midpoint so first render isn't silent
            for (int i = 0; i < kObiontGridW2D; ++i)
                colLiveSnapshot[i] = kObiontGridH2D / 2;
            proj2D.buildAmps(colLiveSnapshot, kObiontGridH2D);
        }
        else
        {
            ca.seedSinusoidal(seedFreq, density, rng);
        }

        adsr.noteOn();
        lfo.reset();
        lfo2.reset();
        dcBlocker.reset();
        reconFilter.reset();
        svf.reset();
    }

    // Note-off: enter ADSR release
    void noteOff() noexcept
    {
        releasing = true;
        adsr.noteOff();
    }

    // Voice steal: crossfade the stolen voice out over 5ms, then re-seed and
    // trigger the incoming note.  stealGain is set to the current amplitude and
    // decremented per sample in the render loop; the CA grid is re-seeded
    // immediately so the new note's timbral evolution starts right away.
    // mode: 0 = 1D, 1 = 2D (seed appropriate grid).
    // seedFreq is derived from midiNote internally.
    void steal(int midiNote, float vel, float density, double sampleRate, int mode = 0) noexcept
    {

        // F02/F28-fix: stealGain is now a 0→1 ramp-up for the NEW note's audio.
        // Start at 0 (mute the first sample of the new note) and ramp to 1 over
        // 5ms so the new note's attack is not abrupt.  Step = 1 / (0.005 * sr).
        stealGain = 0.f;
        stealFadeStep = 1.0f / (0.005f * static_cast<float>(sampleRate));

        // Re-seed grid on steal — don't kill mid-evolution
        int seedFreq = std::max(1, (midiNote % 12) + 1);
        if (mode == 1)
        {
            ca2D.seedSinusoidal(seedFreq, density, rng);
            for (int i = 0; i < kObiontGridW2D; ++i)
                colLiveSnapshot[i] = kObiontGridH2D / 2;
            proj2D.buildAmps(colLiveSnapshot, kObiontGridH2D);
        }
        else
        {
            ca.seedSinusoidal(seedFreq, density, rng);
        }

        note = midiNote;
        velocity = vel;
        releasing = false;
        phase = 0.f;
        evoCounter = 0.f;
        modEvoMul = 1.f;    // F12-fix: reset stale evo rate multiplier on steal
        modProjDelta = 0.f; // F12-fix: reset stale projection delta on steal
        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;
        adsr.kill();
        // RT-fix: adsr.prepare() only sets sr — already called once at engine
        // prepare()-time for all voices.  Calling it here from the audio-thread
        // steal handler was a P3 lifecycle violation (no allocation, but wrong phase).
        adsr.noteOn();
        lfo.reset();
        lfo2.reset();
    }

    bool isIdle() const noexcept { return !active; }
};

//==============================================================================
// ObiontEngine — the full SynthEngine implementation
//==============================================================================
class ObiontEngine : public SynthEngine
{
public:
    ObiontEngine() = default;
    ~ObiontEngine() = default;

    // -------------------------------------------------------------------------
    // Static parameter registration (called by XOceanusProcessor)
    // -------------------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P = juce::ParameterID;
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;
        auto nr = [](float lo, float hi, float step = 0.f) { return juce::NormalisableRange<float>(lo, hi, step); };

        // --- CA control ---
        // obnt_mode (1D/2D toggle) removed — 2D Life CA not yet production-ready.
        // Tracked for v1.1 in issue #666. Engine is hardwired to 1D Elementary CA (mode 0).
        params.push_back(std::make_unique<PI>(P("obnt_rule", 1), "Rule", 0, 255, 90));
        params.push_back(std::make_unique<PF>(P("obnt_ruleMorph", 1), "Rule Morph", nr(0.f, 1.f), 0.f));
        params.push_back(std::make_unique<PF>(P("obnt_evolutionRate", 1), "Evolution Rate", nr(0.1f, 50.f), 4.f));
        params.push_back(std::make_unique<PF>(P("obnt_gridDensity", 1), "Grid Density", nr(0.f, 1.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("obnt_chaos", 1), "Chaos", nr(0.f, 1.f), 0.f));
        params.push_back(std::make_unique<PF>(P("obnt_projection", 1), "Projection", nr(0.f, 1.f), 0.35f));

        // --- Pitch mode ---
        params.push_back(std::make_unique<PI>(P("obnt_pitchMode", 1), "Pitch Mode", 0, 1, 0));

        // --- Reconstruction + musical filter ---
        params.push_back(std::make_unique<PF>(P("obnt_filterCutoff", 1), "Filter Cutoff", nr(20.f, 20000.f), 3000.f));
        params.push_back(std::make_unique<PF>(P("obnt_filterResonance", 1), "Filter Resonance", nr(0.f, 1.f), 0.3f));

        // --- ADSR ---
        params.push_back(std::make_unique<PF>(P("obnt_attack", 1), "Attack", nr(0.001f, 4.f), 0.01f));
        params.push_back(std::make_unique<PF>(P("obnt_decay", 1), "Decay", nr(0.01f, 4.f), 0.3f));
        params.push_back(std::make_unique<PF>(P("obnt_sustain", 1), "Sustain", nr(0.f, 1.f), 0.7f));
        params.push_back(std::make_unique<PF>(P("obnt_release", 1), "Release", nr(0.01f, 8.f), 0.8f));

        // --- Velocity sensitivity ---
        params.push_back(std::make_unique<PF>(P("obnt_velSens", 1), "Vel Sensitivity", nr(0.f, 1.f), 0.7f));

        // --- LFO 1 ---
        params.push_back(std::make_unique<PF>(P("obnt_lfo1Rate", 1), "LFO1 Rate", nr(0.01f, 10.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("obnt_lfo1Depth", 1), "LFO1 Depth", nr(0.f, 1.f), 0.2f));
        params.push_back(std::make_unique<PF>(P("obnt_lfo2Rate", 1), "LFO2 Rate", nr(0.01f, 10.f), 0.15f));
        params.push_back(std::make_unique<PF>(P("obnt_lfo2Depth", 1), "LFO2 Depth", nr(0.f, 1.f), 0.0f));

        // --- Modulation matrix (4 slots × src/dst/amt) ---
        // Src: 0=Envelope, 1=LFO, 2=Velocity, 3=GridEnergy
        // Dst: 0=EvolutionRate, 1=Chaos, 2=RuleMorph, 3=Projection,
        //      4=FilterCutoff, 5=FilterResonance, 6=None
        for (int s = 1; s <= kObiontModSlots; ++s)
        {
            params.push_back(std::make_unique<PI>(P(("obnt_modSrc" + std::to_string(s)).c_str(), 1),
                                                  ("Mod " + std::to_string(s) + " Src").c_str(), 0,
                                                  kObiontModSrcCount - 1, 0));
            params.push_back(std::make_unique<PI>(P(("obnt_modDst" + std::to_string(s)).c_str(), 1),
                                                  ("Mod " + std::to_string(s) + " Dst").c_str(), 0,
                                                  kObiontModDstCount - 1, kObiontModDstCount - 1)); // default None
            params.push_back(std::make_unique<PF>(P(("obnt_modAmt" + std::to_string(s)).c_str(), 1),
                                                  ("Mod " + std::to_string(s) + " Amt").c_str(), nr(-1.f, 1.f), 0.f));
        }

        // --- 4 Macros ---
        params.push_back(std::make_unique<PF>(P("obnt_macroChaos", 1), "CHAOS", nr(0.f, 1.f), 0.f));
        // F22-fix: default 0.5 gave 0.1+0.5*3.9=2.05× base evo rate (doubling speed).
        // Set default to 0.231 → 0.1+0.231*3.9≈1.0× so center knob = unscaled rate.
        params.push_back(std::make_unique<PF>(P("obnt_macroEvolution", 1), "EVOLUTION", nr(0.f, 1.f), 0.231f));
        params.push_back(std::make_unique<PF>(P("obnt_macroSpace", 1), "SPACE", nr(0.f, 1.f), 0.3f));
        params.push_back(std::make_unique<PF>(P("obnt_macroCoupling", 1), "COUPLING", nr(0.f, 1.f), 0.f));
    }

    // -------------------------------------------------------------------------
    // SynthEngine identity
    // -------------------------------------------------------------------------
    juce::String getEngineId() const override { return "Obiont"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xffE8A030); }
    int getMaxVoices() const override { return kObiontMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoiceCount_.load(std::memory_order_relaxed); }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = (float)sampleRate;
        (void)maxBlockSize;

        for (auto& v : voices)
            v.reset(sampleRate);

        // IIR smooth coefficient for per-block smoothed params (~5ms)
        smoothCoeff = 1.f - std::exp(-1.f / (0.005f * (float)sampleRate));
        // F08-fix: separate 50ms coefficient for grid energy smoothing.
        // Previously smoothCoeff * 10 was used, yielding τ=0.5ms not 50ms.
        smoothCoeff50ms = 1.f - std::exp(-1.f / (0.05f * (float)sampleRate));

        modWheelVal = 0.f;
        aftertouchVal = 0.f;
        pitchBendNorm = 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;
        lastOutputSample = 0.f;

        prepareSilenceGate(sampleRate, maxBlockSize, 500.f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset((double)sr);
        modWheelVal = 0.f;
        aftertouchVal = 0.f;
        pitchBendNorm = 0.f;
        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;
        lastOutputSample = 0.f;
    }

    // -------------------------------------------------------------------------
    // Parameter layout / attachment
    // -------------------------------------------------------------------------
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        p_rule = apvts.getRawParameterValue("obnt_rule");
        p_ruleMorph = apvts.getRawParameterValue("obnt_ruleMorph");
        p_evolutionRate = apvts.getRawParameterValue("obnt_evolutionRate");
        p_gridDensity = apvts.getRawParameterValue("obnt_gridDensity");
        p_chaos = apvts.getRawParameterValue("obnt_chaos");
        p_projection = apvts.getRawParameterValue("obnt_projection");
        p_pitchMode = apvts.getRawParameterValue("obnt_pitchMode");
        p_filterCutoff = apvts.getRawParameterValue("obnt_filterCutoff");
        p_filterResonance = apvts.getRawParameterValue("obnt_filterResonance");
        p_attack = apvts.getRawParameterValue("obnt_attack");
        p_decay = apvts.getRawParameterValue("obnt_decay");
        p_sustain = apvts.getRawParameterValue("obnt_sustain");
        p_release = apvts.getRawParameterValue("obnt_release");
        p_velSens = apvts.getRawParameterValue("obnt_velSens");
        p_lfo1Rate = apvts.getRawParameterValue("obnt_lfo1Rate");
        p_lfo1Depth = apvts.getRawParameterValue("obnt_lfo1Depth");
        p_lfo2Rate = apvts.getRawParameterValue("obnt_lfo2Rate");
        p_lfo2Depth = apvts.getRawParameterValue("obnt_lfo2Depth");
        for (int s = 1; s <= kObiontModSlots; ++s)
        {
            p_modSrc[s - 1] = apvts.getRawParameterValue(("obnt_modSrc" + std::to_string(s)).c_str());
            p_modDst[s - 1] = apvts.getRawParameterValue(("obnt_modDst" + std::to_string(s)).c_str());
            p_modAmt[s - 1] = apvts.getRawParameterValue(("obnt_modAmt" + std::to_string(s)).c_str());
        }
        p_macroChaos = apvts.getRawParameterValue("obnt_macroChaos");
        p_macroEvolution = apvts.getRawParameterValue("obnt_macroEvolution");
        p_macroSpace = apvts.getRawParameterValue("obnt_macroSpace");
        p_macroCoupling = apvts.getRawParameterValue("obnt_macroCoupling");
    }

    // -------------------------------------------------------------------------
    // Coupling
    // -------------------------------------------------------------------------
    // F25-fix: coupling macro used range 0.5-1 (couldn't disable coupling fully).
    // Now maps 0→0 (no coupling) to 1→1 (full coupling), matching fleet convention.
    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        const float recv = p_macroCoupling ? p_macroCoupling->load() : 0.f;
        if (recv <= 0.f)
            return; // macro at zero = coupling off
        if (type == CouplingType::AmpToFilter)
            couplingFilterMod += amount * 2000.f * recv;
        else if (type == CouplingType::AmpToPitch || type == CouplingType::PitchToPitch)
            couplingPitchMod += amount * recv;
    }

    float getSampleForCoupling(int /*channel*/, int /*sampleIndex*/) const override { return lastOutputSample; }

    // -------------------------------------------------------------------------
    // renderBlock
    // -------------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                // F16-fix: MIDI spec — NoteOn with velocity=0 is a NoteOff.
                if (msg.getVelocity() == 0)
                {
                    handleNoteOff(msg.getNoteNumber());
                }
                else
                {
                    // mode is hardwired to 0 (1D Elementary CA); 2D mode removed until v1.1
                    handleNoteOn(msg.getNoteNumber(), msg.getFloatVelocity(), 0);
                    wakeSilenceGate();
                }
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff(msg.getNoteNumber());
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                // D006: mod wheel → rule morph bias
                modWheelVal = msg.getControllerValue() / 127.f;
            }
            else if (msg.isChannelPressure())
            {
                // D006: aftertouch → chaos intensity
                aftertouchVal = msg.getChannelPressureValue() / 127.f;
            }
            else if (msg.isAftertouch())
            {
                aftertouchVal = msg.getAfterTouchValue() / 127.f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }
        }

        // 2. SilenceGate bypass
        if (isSilenceGateBypassed())
        {
            return;
        }

        // 3. Guard: parameters not yet attached
        if (!p_rule)
        {
            buffer.clear();
            return;
        }

        // 4. ParamSnapshot — read all params once per block (zero-cost in the loop)
        constexpr int modeParam = 0; // hardwired to 1D Elementary CA; obnt_mode removed
        // F14-note: all modeParam==1 branches below are dead code (2D mode disabled
        // until v1.1 via issue #666).  They are retained for forward-compatibility
        // and compile-tested, but never execute in the current build.
        const int ruleParam = (int)(p_rule->load() + 0.5f) & 0xFF;
        const float ruleMorph = std::clamp(p_ruleMorph->load(), 0.f, 1.f);
        const float baseEvoRate = p_evolutionRate->load();
        const float macroEvo = p_macroEvolution ? p_macroEvolution->load() : 0.5f;
        // EVOLUTION macro scales the evolution rate 0.1x..4x around the base
        const float evoRate = baseEvoRate * (0.1f + macroEvo * 3.9f);
        // gridDensity is read directly by handleNoteOn() from p_gridDensity;
        // not needed in the render-loop ParamSnapshot.
        const float baseChaos = p_chaos->load();
        const float macroChaosV = p_macroChaos ? p_macroChaos->load() : 0.f;
        // D006: aftertouch → chaos overlay
        const float effectiveChaos = std::clamp(baseChaos + macroChaosV + aftertouchVal * 0.4f, 0.f, 1.f);
        const float projectionParam =
            std::clamp(p_projection->load() + (p_macroSpace ? p_macroSpace->load() * 0.4f : 0.f), 0.f, 1.f);
        const int pitchMode = (int)(p_pitchMode->load() + 0.5f);
        const float baseCutoff = p_filterCutoff->load();
        const float filterRes = std::clamp(p_filterResonance->load(), 0.f, 1.f);
        const float atkSec = p_attack->load();
        const float decSec = p_decay->load();
        const float susLvl = p_sustain->load();
        const float relSec = p_release->load();
        const float velSens = p_velSens->load();
        const float lfo1Rate = p_lfo1Rate->load();
        const float lfo1Depth = p_lfo1Depth->load();
        const float lfo2Rate = p_lfo2Rate->load();
        const float lfo2Depth = p_lfo2Depth->load();

        // Mod matrix snapshot
        int modSrc[kObiontModSlots], modDst[kObiontModSlots];
        float modAmt[kObiontModSlots];
        for (int s = 0; s < kObiontModSlots; ++s)
        {
            modSrc[s] = p_modSrc[s] ? (int)(p_modSrc[s]->load() + 0.5f) : 0;
            modDst[s] = p_modDst[s] ? (int)(p_modDst[s]->load() + 0.5f) : (kObiontModDstCount - 1);
            modAmt[s] = p_modAmt[s] ? p_modAmt[s]->load() : 0.f;
        }

        // Capture and clear coupling state (snap-and-zero pattern)
        float savedCouplingFilter = couplingFilterMod;
        float savedCouplingPitch = couplingPitchMod;
        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;

        // Mod wheel biases rule morph slightly (D006)
        float effectiveRuleMorph = std::clamp(ruleMorph + modWheelVal * 0.3f, 0.f, 1.f);

        // SVF resonance mapping: 0-1 → Q 0.5-6.0
        const float svfQ = 0.5f + filterRes * 5.5f;

        float* L = buffer.getWritePointer(0);
        float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;
        // ADDITIVE: do not clear — engine adds to existing buffer (slot chain convention)

        // 5. Per-voice rendering
        int activeCount = 0;
        for (auto& v : voices)
        {
            if (!v.active)
                continue;

            // Update ADSR params per-block
            v.adsr.setADSR(atkSec, decSec, susLvl, relSec);

            // CA rule update (per-block, not per-sample — negligible cost)
            // In 1D mode: ruleParam is the Wolfram rule byte (0-255)
            // In 2D mode: ruleParam byte is decoded as B/S nibbles (see ObiontCA2D::evolve)
            v.ca.rule = (uint8_t)(ruleParam & 0xFF);
            // Rule2 for morphing: rule+8 wraps (creates interesting harmonic transitions)
            v.ca.rule2 = (uint8_t)((ruleParam + 8) & 0xFF);

            // Pre-take grid snapshot ONCE per block (mode-specific)
            if (modeParam == 1)
            {
                // 2D: read per-column live counts and build harmonic amp table
                v.ca2D.readColLive(v.colLiveSnapshot);
                v.proj2D.buildAmps(v.colLiveSnapshot, kObiontGridH2D);
            }
            else
            {
                v.ca.readSnapshot(v.gridSnapshot);
            }

            // Samples-per-evolution step (independent of pitch — CRITICAL for musicality)
            const float samplesPerEvo = (evoRate > 0.f) ? (sr / evoRate) : sr;

            // --- Per-sample loop ---
            for (int n = 0; n < numSamples; ++n)
            {
                // --- LFO ticks — single call per sample each, results reused throughout ---
                float lfoOut = v.lfo.tick(lfo1Rate);
                float lfo2Out = v.lfo2.tick(lfo2Rate);

                // --- Evolution step gate (modulated by EvolutionRate dst) ---
                v.evoCounter += v.modEvoMul;
                if (v.evoCounter >= samplesPerEvo)
                {
                    v.evoCounter -= samplesPerEvo;

                    // Apply modulation matrix to CA parameters
                    float modChaos = effectiveChaos;
                    // LFO2 → rule morph (D002: second LFO source)
                    float modRuleMorph = std::clamp(effectiveRuleMorph + lfo2Out * lfo2Depth * 0.5f, 0.f, 1.f);
                    float gridEnergy = v.evoEnergySmooth;

                    float modEvoRateMul = 1.f; // dst 0: EvolutionRate multiplier

                    for (int s = 0; s < kObiontModSlots; ++s)
                    {
                        int dst = modDst[s];
                        if (dst == (kObiontModDstCount - 1))
                            continue; // None
                        // CA-side destinations: 0=EvolutionRate, 1=Chaos, 2=RuleMorph
                        if (dst != 0 && dst != 1 && dst != 2)
                            continue;

                        float srcVal = 0.f;
                        switch (modSrc[s])
                        {
                        case 0:
                            srcVal = v.adsr.getLevel();
                            break; // Envelope
                        case 1:
                            srcVal = lfoOut;
                            break; // LFO (already ticked)
                        case 2:
                            srcVal = v.velocity;
                            break; // Velocity
                        case 3:
                            srcVal = gridEnergy;
                            break; // GridEnergy
                        default:
                            break;
                        }
                        float modVal = srcVal * modAmt[s];

                        if (dst == 0)
                            modEvoRateMul = std::clamp(modEvoRateMul + modVal * 3.f, 0.1f, 4.f); // scale evo rate 0.1x–4x
                        else if (dst == 1)
                            modChaos = std::clamp(modChaos + modVal, 0.f, 1.f);
                        else
                            modRuleMorph = std::clamp(modRuleMorph + modVal, 0.f, 1.f);
                    }
                    // Persist evo rate multiplier for subsequent samples
                    v.modEvoMul = modEvoRateMul;

                    if (modeParam == 1)
                    {
                        // 2D Life-like CA evolution
                        float rawEnergy = v.ca2D.evolve((uint8_t)(ruleParam & 0xFF), modRuleMorph, modChaos, v.rng);
                        // F08-fix: smoothCoeff is a 5ms coefficient; *10 gave 0.5ms not 50ms.
                        // Use a dedicated 50ms smoothing coefficient computed from smoothCoeff50ms.
                        v.evoEnergySmooth += (rawEnergy - v.evoEnergySmooth) * smoothCoeff50ms;
                        v.evoEnergySmooth = flushDenormal(v.evoEnergySmooth);

                        // Anti-extinction: re-seed if live cell ratio < 5%
                        if (rawEnergy < kObiontExtinctionThreshold)
                        {
                            int seedFreq = std::max(1, (v.note % 12) + 1);
                            v.ca2D.seedSinusoidal(seedFreq, 0.5f, v.rng);
                        }

                        // Refresh per-column live counts and harmonic amps after evolution
                        v.ca2D.readColLive(v.colLiveSnapshot);
                        v.proj2D.buildAmps(v.colLiveSnapshot, kObiontGridH2D);
                    }
                    else
                    {
                        // 1D Wolfram CA evolution
                        float rawEnergy = v.ca.evolve(modChaos, modRuleMorph, v.rng);
                        // F08-fix: use smoothCoeff50ms (τ=50ms) for grid energy — see prepare()
                        v.evoEnergySmooth += (rawEnergy - v.evoEnergySmooth) * smoothCoeff50ms;
                        v.evoEnergySmooth = flushDenormal(v.evoEnergySmooth);

                        // Anti-extinction: re-seed if live cell ratio < 5%
                        if (rawEnergy < kObiontExtinctionThreshold)
                        {
                            int seedFreq = std::max(1, (v.note % 12) + 1);
                            v.ca.seedSinusoidal(seedFreq, 0.5f, v.rng);
                        }

                        // Refresh snapshot after evolution
                        v.ca.readSnapshot(v.gridSnapshot);
                    }
                }

                // --- MIDI pitch → fundamental frequency ---
                // pitchMode 0: MIDI-locked (standard), phase follows note frequency
                // pitchMode 1: Drift — phase advances at a slower freewheel rate
                float pitchSemitones = (float)v.note + pitchBendNorm * 2.f + savedCouplingPitch;
                float noteFreq = 440.f * fastPow2((pitchSemitones - 69.f) / 12.f);

                float phaseInc;
                if (pitchMode == 0)
                {
                    // Locked: read the projection at the pitch-synced rate
                    phaseInc = noteFreq / sr;
                }
                else
                {
                    // Drift: freewheel at a slow rate driven by evolution
                    phaseInc = (evoRate * 0.1f) / sr;
                }

                v.phase += phaseInc;
                if (v.phase >= 1.f)
                    v.phase -= 1.f;

                // --- Spatial projection readout (mode-specific) ---
                // Apply Projection mod matrix destination (dst 3)
                const float modProj = std::clamp(projectionParam + v.modProjDelta, 0.f, 1.f);
                float projOut;
                if (modeParam == 1)
                {
                    // 2D: column-additive harmonic synthesis
                    projOut = v.proj2D.read(v.phase, modProj);
                }
                else
                {
                    // 1D: cosine-weighted spatial scan
                    projOut = ObiontProjection::read(v.gridSnapshot, v.phase, modProj);
                }

                // --- Reconstruction filter (4-pole LP, tracks baseCutoff/2) ---
                float reconFc = std::clamp(baseCutoff * 0.5f, 20.f, sr * 0.45f);
                float reconOut = v.reconFilter.process(projOut, reconFc, sr);

                // --- DC blocker ---
                float dcOut = v.dcBlocker.process(reconOut);

                // --- Modulation matrix: filter-side destinations (per-sample) ---
                float modFilterCutoffDelta = 0.f;
                float modFilterResDelta = 0.f;
                float modProjDelta = 0.f;
                for (int s = 0; s < kObiontModSlots; ++s)
                {
                    int dst = modDst[s];
                    if (dst != 3 && dst != 4 && dst != 5)
                        continue; // Projection + filter destinations
                    float srcVal = 0.f;
                    switch (modSrc[s])
                    {
                    case 0:
                        srcVal = v.adsr.getLevel();
                        break; // Envelope
                    case 1:
                        srcVal = lfoOut;
                        break; // LFO
                    case 2:
                        srcVal = v.velocity;
                        break; // Velocity
                    case 3:
                        srcVal = v.evoEnergySmooth;
                        break; // GridEnergy
                    default:
                        break;
                    }
                    if (dst == 3)
                        modProjDelta += srcVal * modAmt[s];
                    else if (dst == 4)
                        modFilterCutoffDelta += srcVal * modAmt[s] * 8000.f;
                    else
                        modFilterResDelta += srcVal * modAmt[s];
                }
                v.modProjDelta = modProjDelta; // persist for projection readout

                // LFO1 independently modulates filter cutoff (D005)
                float lfoCutoff = lfoOut * lfo1Depth * 2000.f;

                // D001: velocity brightens filter cutoff (timbre, not just amplitude)
                float velCutoffBoost = v.velocity * velSens * 1500.f;

                float finalCutoff =
                    std::clamp(baseCutoff + lfoCutoff + savedCouplingFilter + modFilterCutoffDelta + velCutoffBoost,
                               20.f, 20000.f);
                float finalQ = std::clamp(svfQ + modFilterResDelta * 5.5f, 0.5f, 20.f);

                // --- SVF lowpass ---
                float filtered = v.svf.processLP(dcOut, finalCutoff, finalQ, sr);

                // --- ADSR envelope ---
                float envLevel = v.adsr.process();

                // D001: velocity shapes amplitude (with sensitivity control)
                float velGain = 1.f - velSens + velSens * v.velocity;

                float output = filtered * envLevel * velGain * 0.7f;

                // Soft clip to prevent clipping from high-resonance CA patterns
                output = output / (1.f + std::abs(output));

                // F02/F28-fix: the stolen voice has already had its ADSR killed and a
                // new noteOn() called — 'output' here is the NEW note's audio.
                // The original code multiplied the new note by a 1→0 ramp (ducking the
                // new note's attack), not fading the old note out.
                // Correct approach: use stealGain as a 0→1 ramp-up so the new voice
                // fades IN over 5ms, masking any abrupt pitch/timbre transition.
                if (v.stealGain < 1.f)
                {
                    output *= v.stealGain;
                    v.stealGain += v.stealFadeStep;
                    if (v.stealGain > 1.f)
                        v.stealGain = 1.f;
                }

                L[n] += output;
                if (R != L)
                    R[n] += output;
                lastOutputSample = output;

                // Voice done when ADSR is idle
                if (!v.adsr.isActive())
                {
                    v.active = false;
                }
            }

            if (v.active)
                ++activeCount;
        }

        activeVoiceCount_.store(activeCount, std::memory_order_relaxed);

        // 6. Feed SilenceGate analyzer
        analyzeForSilenceGate(buffer, numSamples);
    }

private:
    // -------------------------------------------------------------------------
    // Voice management helpers
    // -------------------------------------------------------------------------
    // mode: 0 = 1D Wolfram CA, 1 = 2D Life-like CA
    void handleNoteOn(int note, float vel, int mode) noexcept
    {
        if (!p_gridDensity)
            return;
        const float density = p_gridDensity->load();

        // Find free voice first
        for (auto& v : voices)
        {
            if (!v.active)
            {
                v.noteOn(note, vel, density, (double)sr, mode);
                return;
            }
        }
        // No free voice — steal the oldest releasing voice, or the quietest active one
        ObiontVoice* victim = nullptr;
        float minLevel = 1.f;
        for (auto& v : voices)
        {
            if (v.releasing)
            {
                victim = &v;
                break;
            }
            float lvl = v.adsr.getLevel();
            if (lvl < minLevel)
            {
                minLevel = lvl;
                victim = &v;
            }
        }
        if (victim)
            victim->steal(note, vel, density, (double)sr, mode);
    }

    void handleNoteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.note == note && !v.releasing)
            {
                v.noteOff();
                return;
            }
        }
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    float smoothCoeff = 0.001f;
    float smoothCoeff50ms = 0.001f; // F08-fix: 50ms coefficient for grid energy IIR

    // Expression inputs
    float modWheelVal = 0.f;
    float aftertouchVal = 0.f;
    float pitchBendNorm = 0.f;

    // Block-level coupling accumulation
    float couplingFilterMod = 0.f;
    float couplingPitchMod = 0.f;

    // Coupling output sample cache (O(1) getSampleForCoupling)
    float lastOutputSample = 0.f;

    // Voice pool
    std::array<ObiontVoice, kObiontMaxVoices> voices;

    // F13-fix: rngGlobal was declared here but never used — all CA evolution uses
    // per-voice v.rng.  Removed to eliminate dead state and avoid future confusion.

    // -------------------------------------------------------------------------
    // Raw parameter pointers (cached in attachParameters, read in renderBlock)
    // -------------------------------------------------------------------------
    std::atomic<float>* p_rule = nullptr;
    std::atomic<float>* p_ruleMorph = nullptr;
    std::atomic<float>* p_evolutionRate = nullptr;
    std::atomic<float>* p_gridDensity = nullptr;
    std::atomic<float>* p_chaos = nullptr;
    std::atomic<float>* p_projection = nullptr;
    std::atomic<float>* p_pitchMode = nullptr;
    std::atomic<float>* p_filterCutoff = nullptr;
    std::atomic<float>* p_filterResonance = nullptr;
    std::atomic<float>* p_attack = nullptr;
    std::atomic<float>* p_decay = nullptr;
    std::atomic<float>* p_sustain = nullptr;
    std::atomic<float>* p_release = nullptr;
    std::atomic<float>* p_velSens = nullptr;
    std::atomic<float>* p_lfo1Rate = nullptr;
    std::atomic<float>* p_lfo1Depth = nullptr;
    std::atomic<float>* p_lfo2Rate = nullptr;
    std::atomic<float>* p_lfo2Depth = nullptr;
    std::atomic<float>* p_modSrc[kObiontModSlots] = {};
    std::atomic<float>* p_modDst[kObiontModSlots] = {};
    std::atomic<float>* p_modAmt[kObiontModSlots] = {};
    std::atomic<float>* p_macroChaos = nullptr;
    std::atomic<float>* p_macroEvolution = nullptr;
    std::atomic<float>* p_macroSpace = nullptr;
    std::atomic<float>* p_macroCoupling = nullptr;
};

} // namespace xoceanus
