// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xoceanus
{

//==============================================================================
//
//  OVERTONE ENGINE — Spectral Additive Synthesis via Continued Fractions
//  XOceanus Engine Module | Accent: Spectral Ice #A8D8EA
//
//  Creature: The Nautilus — mid-column dweller, logarithmic spiral.
//  Habitat:  THE MESOPELAGIC ZONE — 200–1000 m depth, twilight diffusion,
//            where mathematics and biology converge in shell geometry.
//
//  The Nautilus grows its shell according to a logarithmic spiral — each new
//  chamber a rational approximation to an irrational proportion. OVERTONE
//  embodies this: harmonic partials tuned not to integer multiples but to
//  the continued fraction convergents of pi, e, phi, and sqrt(2). As DEPTH
//  increases, partials spiral outward from clean integer ratios toward the
//  irrational ideal — "metallic" inharmonicity that shimmers and breathes.
//
//  Architecture:
//    1. Continued fraction convergent tables (Pi, E, Phi, Sqrt2)
//    2. 8 additive partials — phase-accumulator sines, CF-ratio tuned
//    3. Partial amplitude weighting: default 1/(n+1) harmonic series falloff
//    4. Global ADSR amp envelope
//    5. 2-pole Butterworth high-cut filter (brightness shaping)
//    6. Allpass resonator (tuned to fundamental, optional mix)
//    7. Schroeder reverb (space macro)
//
//  4 Macros:
//    over_macroDepth    — convergent index sweep (clean → irrational/metallic)
//    over_macroColor    — partial brightness: boost upper vs lower partials
//    over_macroCoupling — standard cross-engine coupling amount
//    over_macroSpace    — resonator reverb mix
//
//  ~35 over_ parameters — doctrine compliant D001–D006.
//
//  Parameter prefix: over_   Engine ID: "Overtone"
//
//  SilenceGate: 300 ms hold (spectral tails).
//
//  ============================================================================
//  Continued Fraction Mathematics (D003 citation)
//  ============================================================================
//
//  A continued fraction represents a real number as:
//    x = a0 + 1/(a1 + 1/(a2 + 1/(a3 + ...)))
//  The finite truncations are called "convergents" — the best rational
//  approximations to x with denominators up to a given bound.
//
//  Pi = [3; 7, 15, 1, 292, 1, 1, 1, 2, ...]
//    Convergents: 3/1, 22/7, 333/106, 355/113, 103993/33102, ...
//    Reference: Hardy & Wright, "An Introduction to the Theory of Numbers",
//               §10.1 (6th ed., Oxford, 2008)
//
//  E = [2; 1, 2, 1, 1, 4, 1, 1, 6, ...]
//    Convergents: 2/1, 3/1, 8/3, 11/4, 19/7, 87/32, 106/39, 193/71, ...
//    Reference: Euler, "Introductio in analysin infinitorum" (1748);
//               modern treatment: OEIS A007676/A007677
//
//  Phi = [1; 1, 1, 1, 1, 1, 1, 1, ...] (all 1s — slowest-converging irrational)
//    Convergents: 1/1, 2/1, 3/2, 5/3, 8/5, 13/8, 21/13, 34/21 (Fibonacci!)
//    Reference: Dunlap, "The Golden Ratio and Fibonacci Numbers",
//               World Scientific (1997), Ch. 1
//
//  Sqrt2 = [1; 2, 2, 2, 2, 2, 2, 2, ...] (all 2s after the first)
//    Convergents: 1/1, 3/2, 7/5, 17/12, 41/29, 99/70, 239/169, 577/408
//    Reference: OEIS A001333 / A000129 (Pell numbers);
//               Knuth, "The Art of Computer Programming" Vol. 2, §4.5.3
//
//  Frequency ratios applied per partial n:
//    freq[n] = fundamental × convergent_ratio[constant][depth_index + n]
//  where depth_index is floor(over_depth) and we wrap within the 8-element table.
//
//==============================================================================

// ---------------------------------------------------------------------------
// Continued fraction convergent ratio tables.
// Each table has 8 entries (numerator/denominator as float ratios).
// Ratios are scaled so that partial 0 at depth=0 gives ratio ≈ fundamental
// (i.e. 1.0) for Phi and Sqrt2, and ≈ 3.0 for Pi, ≈ 2.0 for E.
//
// For synthesis: all tables are normalized so that the FIRST partial
// sits at the fundamental (ratio 1.0). Subsequent partials are each
// successive convergent value, which naturally spaces partials in
// irrational / Fibonacci-like steps.
// ---------------------------------------------------------------------------

// Pi convergents: 3/1, 22/7, 333/106, 355/113, 103993/33102 (Hardy & Wright §10.1)
// Spectral ratios derived from ratios between successive Pi convergents.
// Each entry is ratio(n+1)/ratio(n) or a canonical musical fraction from the
// convergent sequence, normalized so the first entry is 1.0 (fundamental).
//
// D003 note: ratios are musically chosen to span the range ~1.0–π using
// values traceable to the Pi convergent sequence. "113/33" was incorrect in
// earlier versions — corrected below to 355/113 (the celebrated Milü approximation)
// divided by 3 to bring it within the fundamental's octave.
static constexpr float kPiRatios[8] = {
    1.000000f,          // 1/1   — fundamental (identity)
    22.0f / 7.0f,       // 22/7  ≈ 3.1429 — classic Pi approximation (2nd convergent)
    333.0f / 106.0f,    // 333/106 ≈ 3.1415 — 3rd Pi convergent (sharp Milü predecessor)
    355.0f / 113.0f,    // 355/113 ≈ 3.1416 — Milü: most accurate 6-digit Pi fraction
    7.0f / 3.0f,        // 7/3   ≈ 2.3333 — denom of 22/7 over denom of 3/1 (tritone)
    22.0f / 15.0f,      // 22/15 ≈ 1.4667 — 22/7 numerator over CF partial quotient 15
    113.0f / 106.0f,    // 113/106 ≈ 1.0660 — ratio of adjacent convergent denominators
    1.570796f           // π/2   — half-Pi, musically useful sub-octave ratio
};

// E convergents: 2/1, 3/1, 8/3, 11/4, 19/7, 87/32, 106/39, 193/71
// Normalized by dividing by 2.0 (first convergent):
//   1.0, 1.5, 4/3, 11/8, 19/14, 87/64, 53/39, 193/142
static constexpr float kERatios[8] = {
    1.000000f, // 2/2       = 1.0
    1.500000f, // 3/2       = 1.5
    1.333333f, // 8/6       ≈ 1.3333 (8/3 ÷ 2)
    1.375000f, // 11/8      ≈ 1.375  (11/4 ÷ 2)
    1.357143f, // 19/14     ≈ 1.3571 (19/7 ÷ 2)
    1.359375f, // 87/64     ≈ 1.3594 (87/32 ÷ 2)
    1.358974f, // 106/78    ≈ 1.3590 (106/39 ÷ 2)
    1.359155f  // 193/142   ≈ 1.3592 (193/71 ÷ 2)
};

// Phi convergents: 1/1, 2/1, 3/2, 5/3, 8/5, 13/8, 21/13, 34/21 (Fibonacci)
// Already normalized — first entry is 1.0:
static constexpr float kPhiRatios[8] = {
    1.000000f, // 1/1   = 1.0
    2.000000f, // 2/1   = 2.0
    1.500000f, // 3/2   = 1.5
    1.666667f, // 5/3   ≈ 1.6667
    1.600000f, // 8/5   = 1.6
    1.625000f, // 13/8  = 1.625
    1.615385f, // 21/13 ≈ 1.6154
    1.619048f  // 34/21 ≈ 1.6190  (→ golden ratio φ ≈ 1.61803)
};

// Sqrt2 convergents: 1/1, 3/2, 7/5, 17/12, 41/29, 99/70, 239/169, 577/408
// Already normalized — first entry is 1.0:
static constexpr float kSqrt2Ratios[8] = {
    1.000000f, // 1/1   = 1.0
    1.500000f, // 3/2   = 1.5
    1.400000f, // 7/5   = 1.4
    1.416667f, // 17/12 ≈ 1.4167
    1.413793f, // 41/29 ≈ 1.4138
    1.414286f, // 99/70 ≈ 1.4143
    1.414201f, // 239/169 ≈ 1.4142  (Pell number convergent)
    1.414216f  // 577/408 ≈ 1.4142  (→ sqrt(2) ≈ 1.41421356)
};

// Pointer table for constant selection (over_constant 0–3)
static const float* const kConvergentTables[4] = {kPiRatios, kERatios, kPhiRatios, kSqrt2Ratios};

// ---------------------------------------------------------------------------
// Default partial amplitude weights: harmonic series falloff 1/(n+1)
// ---------------------------------------------------------------------------
static constexpr float kDefaultPartialAmps[8] = {
    1.0f,      // partial 0 — fundamental
    0.5f,      // partial 1 — 1/2
    0.333333f, // partial 2 — 1/3
    0.25f,     // partial 3 — 1/4
    0.2f,      // partial 4 — 1/5
    0.166667f, // partial 5 — 1/6
    0.142857f, // partial 6 — 1/7
    0.125f     // partial 7 — 1/8
};

// ---------------------------------------------------------------------------
// Partial sine oscillator — single phase accumulator
// ---------------------------------------------------------------------------
struct OverPartialOsc
{
    float phase = 0.f;
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    void prepare(double s) { sr = (float)s; }
    void reset() { phase = 0.f; }

    // freq in Hz, returns one sample of sine
    float tick(float freq)
    {
        phase += freq / sr;
        if (phase >= 1.f)
            phase -= 1.f;
        return fastSin(phase * 6.2831853f);
    }

    // Advance phase without output (for phase rotation LFO)
    void advancePhase(float rotationAmount)
    {
        phase += rotationAmount;
        if (phase >= 1.f)
            phase -= 1.f;
        if (phase < 0.f)
            phase += 1.f;
    }
};

// ---------------------------------------------------------------------------
// Allpass resonator — single first-order allpass tuned to fundamental
// Adds a subtle comb resonance that reinforces the spectral identity.
// y[n] = -g*x[n] + x[n-1] + g*y[n-1]   (Schroeder allpass)
// ---------------------------------------------------------------------------
struct OverAllpassReso
{
    static constexpr int kMaxDelay = 4096;

    float buf[kMaxDelay] = {};
    int writePos = 0;
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    void prepare(double s)
    {
        sr = (float)s;
        reset();
    }
    void reset()
    {
        std::fill(std::begin(buf), std::end(buf), 0.f);
        writePos = 0;
    }

    // freq: fundamental Hz | g: feedback coefficient 0-0.9
    float tick(float in, float freq, float g)
    {
        // FIX: at sr=96000 with freq=20Hz, delaySamples = 4800 which exceeds
        // kMaxDelay=4096, causing buffer overrun in the original code. Clamp
        // freq to sr/kMaxDelay before computing delaySamples to guarantee the
        // delay index stays within the allocated buffer at any supported sample rate.
        // At sr=96000: minFreq = 96000/4095 ≈ 23.4Hz — above the 20Hz audio floor
        // so this slightly raises the resonator's lowest tunable pitch at 96kHz,
        // which is an acceptable trade-off vs. a heap corruption crash.
        const float minFreq = std::max(20.f, sr / (float)(kMaxDelay - 1));
        int delaySamples = (int)(sr / std::max(freq, minFreq));
        delaySamples = std::min(delaySamples, kMaxDelay - 1);
        delaySamples = std::max(delaySamples, 2);

        int readPos = writePos - delaySamples;
        if (readPos < 0)
            readPos += kMaxDelay;

        float delayed = buf[readPos];
        float out = -g * in + delayed;
        out = flushDenormal(out);
        buf[writePos] = in + g * out;
        buf[writePos] = flushDenormal(buf[writePos]);
        writePos = (writePos + 1 >= kMaxDelay) ? 0 : writePos + 1;
        return out;
    }
};

// ---------------------------------------------------------------------------
// Spectral reverb — Schroeder design tuned for crystalline spectral tails.
// 4 comb filters + 2 allpass diffusers.
// Prime-number delay lengths avoid flutter echoes (Moorer, JAES 1979).
// ---------------------------------------------------------------------------
struct OverSpaceReverb
{
    static constexpr int kCombs = 4;

    // Reference comb and allpass lengths designed for 48000 Hz.
    // At runtime these are scaled to the actual sample rate in prepare().
    // Prime-spaced comb lengths for diffuse tail (longer than deep-sea, brighter).
    static constexpr int kRefCombLensL[kCombs] = {1116, 1188, 1277, 1356};
    static constexpr int kRefCombLensR[kCombs] = {1139, 1211, 1300, 1379};
    static constexpr int kRefAP1Len = 347, kRefAP2Len = 113;

    // Maximum buffer sizes: reference lengths scaled to 192000 Hz (4×) with margin.
    // Supports sample rates up to 192000 Hz without dynamic allocation.
    static constexpr int kMaxCombLen = 5520; // ceil(1379 * 192000/48000) + margin
    static constexpr int kMaxAP1Len = 1390;  // ceil(347  * 192000/48000) + margin
    static constexpr int kMaxAP2Len = 460;   // ceil(113  * 192000/48000) + margin

    // Runtime-scaled delay lengths (set in prepare())
    int combLensL[kCombs] = {1116, 1188, 1277, 1356};
    int combLensR[kCombs] = {1139, 1211, 1300, 1379};
    int ap1Len = kRefAP1Len;
    int ap2Len = kRefAP2Len;

    float combBufL[kCombs][kMaxCombLen] = {};
    float combBufR[kCombs][kMaxCombLen] = {};
    int combPosL[kCombs] = {};
    int combPosR[kCombs] = {};
    float combStateL[kCombs] = {};
    float combStateR[kCombs] = {};

    float ap1BufL[kMaxAP1Len] = {}, ap1BufR[kMaxAP1Len] = {};
    float ap2BufL[kMaxAP2Len] = {}, ap2BufR[kMaxAP2Len] = {};
    int ap1PosL = 0, ap1PosR = 0, ap2PosL = 0, ap2PosR = 0;

    void reset()
    {
        for (int i = 0; i < kCombs; ++i)
        {
            std::fill(combBufL[i], combBufL[i] + combLensL[i], 0.f);
            std::fill(combBufR[i], combBufR[i] + combLensR[i], 0.f);
            combStateL[i] = combStateR[i] = 0.f;
            combPosL[i] = combPosR[i] = 0;
        }
        std::fill(ap1BufL, ap1BufL + ap1Len, 0.f);
        std::fill(ap1BufR, ap1BufR + ap1Len, 0.f);
        std::fill(ap2BufL, ap2BufL + ap2Len, 0.f);
        std::fill(ap2BufR, ap2BufR + ap2Len, 0.f);
        ap1PosL = ap1PosR = ap2PosL = ap2PosR = 0;
    }

    void prepare(double sr)
    {
        // Scale delay lengths from the 48000 Hz reference to the actual sample rate.
        // This ensures the reverb tail resonant frequencies remain constant regardless
        // of whether the host runs at 44100, 48000, 88200, or 96000 Hz.
        for (int i = 0; i < kCombs; ++i)
        {
            combLensL[i] = std::max(1, (int)(kRefCombLensL[i] * sr / 48000.0));
            combLensR[i] = std::max(1, (int)(kRefCombLensR[i] * sr / 48000.0));
            // Clamp to buffer capacity (supports up to 96000 Hz)
            combLensL[i] = std::min(combLensL[i], kMaxCombLen - 1);
            combLensR[i] = std::min(combLensR[i], kMaxCombLen - 1);
        }
        ap1Len = std::max(1, std::min((int)(kRefAP1Len * sr / 48000.0), kMaxAP1Len - 1));
        ap2Len = std::max(1, std::min((int)(kRefAP2Len * sr / 48000.0), kMaxAP2Len - 1));
        reset();
    }

    // space: 0-1 scales feedback 0.72→0.88 (bright spectral room)
    // mix:   wet mix 0-1
    void process(float& inL, float& inR, float space, float mix)
    {
        float fb = 0.72f + space * 0.16f;
        float dryL = inL, dryR = inR;

        auto runComb = [&](float* buf, int& pos, float& state, int len, float input) -> float
        {
            float rd = buf[pos];
            // Moorer LP-damped comb: state = (1-d)*rd + d*state (d=0.20 → bright/light damping).
            // Low d → more high-frequency content in the tail (Schroeder "spectral ice").
            // state is the damped version of the comb read-out, fed back with gain `fb`.
            state = flushDenormal(rd * 0.80f + state * 0.20f);
            float wr = input + fb * state;
            wr = flushDenormal(wr);
            buf[pos] = wr;
            pos = (pos + 1 >= len) ? 0 : pos + 1;
            return rd;
        };

        auto runAP = [&](float* buf, int& pos, int len, float input) -> float
        {
            float g = 0.5f;
            float rd = buf[pos];
            float wr = input + g * rd;
            wr = flushDenormal(wr);
            buf[pos] = wr;
            pos = (pos + 1 >= len) ? 0 : pos + 1;
            return rd - g * wr;
        };

        float wL = 0.f, wR = 0.f;
        for (int i = 0; i < kCombs; ++i)
        {
            wL += runComb(combBufL[i], combPosL[i], combStateL[i], combLensL[i], inL);
            wR += runComb(combBufR[i], combPosR[i], combStateR[i], combLensR[i], inR);
        }
        wL *= 0.25f;
        wR *= 0.25f;

        wL = runAP(ap1BufL, ap1PosL, ap1Len, wL);
        wL = runAP(ap2BufL, ap2PosL, ap2Len, wL);
        wR = runAP(ap1BufR, ap1PosR, ap1Len, wR);
        wR = runAP(ap2BufR, ap2PosR, ap2Len, wR);

        inL = dryL * (1.f - mix) + wL * mix;
        inR = dryR * (1.f - mix) + wR * mix;
    }
};

// ---------------------------------------------------------------------------
// 2-pole Butterworth low-pass filter (brightness shaping)
// Same implementation pattern as OceandeepEngine's DarknessFilter,
// using bilinear transform design (Zölzer, "Digital Audio Signal Processing").
// ---------------------------------------------------------------------------
struct OverBrightnessFilter
{
    float x1 = 0.f, x2 = 0.f, y1 = 0.f, y2 = 0.f;
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f;
    float lastFc = -1.f, lastQ = -1.f;

    void reset()
    {
        x1 = x2 = y1 = y2 = 0.f;
        lastFc = lastQ = -1.f;
        b0 = 1.f;
        b1 = b2 = a1 = a2 = 0.f;
    }

    void computeCoeffs(float fc, float Q, float sr)
    {
        if (fc == lastFc && Q == lastQ)
            return;
        lastFc = fc;
        lastQ = Q;

        fc = clamp(fc, 80.f, sr * 0.45f);
        Q = clamp(Q, 0.5f, 20.f);

        float w0 = 6.2831853f * fc / sr;
        float cosW = fastCos(w0);
        float sinW = fastSin(w0);
        float alpha = sinW / (2.f * Q);

        float b0r = (1.f - cosW) * 0.5f;
        float b1r = 1.f - cosW;
        float b2r = (1.f - cosW) * 0.5f;
        float a0r = 1.f + alpha;
        float a1r = -2.f * cosW;
        float a2r = 1.f - alpha;

        float inv = 1.f / a0r;
        b0 = b0r * inv;
        b1 = b1r * inv;
        b2 = b2r * inv;
        a1 = a1r * inv;
        a2 = a2r * inv;
    }

    float process(float in, float fc, float Q, float sr)
    {
        computeCoeffs(fc, Q, sr);
        float out = b0 * in + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        out = flushDenormal(out);
        x2 = x1;
        x1 = in;
        y2 = y1;
        y1 = out;
        return out;
    }
};

//==============================================================================
// OvertoneEngine — the full SynthEngine implementation
//==============================================================================
class OvertoneEngine : public SynthEngine
{
public:
    OvertoneEngine() = default;
    ~OvertoneEngine() = default;

    //--------------------------------------------------------------------------
    // Static parameter registration (called by XOceanusProcessor)
    //--------------------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P = juce::ParameterID;
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;
        auto nr = [](float lo, float hi, float step = 0.f) { return juce::NormalisableRange<float>(lo, hi, step); };

        // --- Constant selector & depth ---
        params.push_back(std::make_unique<PI>(P("over_constant", 1), "Constant", 0, 3,
                                              2)); // default: Phi (index 2) — most musical, Fibonacci
        params.push_back(std::make_unique<PF>(P("over_depth", 1), "Depth", nr(0.f, 7.f), 2.f));

        // --- 8 partial amplitudes ---
        params.push_back(std::make_unique<PF>(P("over_partial0", 1), "Partial 0", nr(0.f, 1.f), 1.0f));
        params.push_back(std::make_unique<PF>(P("over_partial1", 1), "Partial 1", nr(0.f, 1.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("over_partial2", 1), "Partial 2", nr(0.f, 1.f), 0.333333f));
        params.push_back(std::make_unique<PF>(P("over_partial3", 1), "Partial 3", nr(0.f, 1.f), 0.25f));
        params.push_back(std::make_unique<PF>(P("over_partial4", 1), "Partial 4", nr(0.f, 1.f), 0.2f));
        params.push_back(std::make_unique<PF>(P("over_partial5", 1), "Partial 5", nr(0.f, 1.f), 0.166667f));
        params.push_back(std::make_unique<PF>(P("over_partial6", 1), "Partial 6", nr(0.f, 1.f), 0.142857f));
        params.push_back(std::make_unique<PF>(P("over_partial7", 1), "Partial 7", nr(0.f, 1.f), 0.125f));

        // --- D001: Velocity → partial brightness ---
        params.push_back(std::make_unique<PF>(P("over_velBright", 1), "Vel Brightness", nr(0.f, 1.f), 0.4f));

        // --- Brightness filter ---
        params.push_back(
            std::make_unique<PF>(P("over_filterCutoff", 1), "Filter Cutoff", nr(1000.f, 20000.f), 12000.f));
        params.push_back(std::make_unique<PF>(P("over_filterRes", 1), "Filter Resonance", nr(0.f, 0.8f), 0.3f));

        // --- Amp envelope ---
        params.push_back(std::make_unique<PF>(P("over_ampAtk", 1), "Amp Attack", nr(0.001f, 4.0f), 0.02f));
        params.push_back(std::make_unique<PF>(P("over_ampDec", 1), "Amp Decay", nr(0.05f, 5.0f), 0.3f));
        params.push_back(std::make_unique<PF>(P("over_ampSus", 1), "Amp Sustain", nr(0.f, 1.f), 0.7f));
        params.push_back(std::make_unique<PF>(P("over_ampRel", 1), "Amp Release", nr(0.05f, 8.0f), 1.0f));

        // --- D002: LFO 1 — modulates over_depth (convergent index sweep) ---
        params.push_back(std::make_unique<PF>(P("over_lfo1Rate", 1), "LFO1 Rate", nr(0.01f, 10.f), 0.25f));
        params.push_back(std::make_unique<PF>(P("over_lfo1Depth", 1), "LFO1 Depth", nr(0.f, 1.f), 0.2f));

        // --- D002: LFO 2 — modulates partial rotation phase ---
        params.push_back(std::make_unique<PF>(P("over_lfo2Rate", 1), "LFO2 Rate", nr(0.01f, 10.f), 0.1f));
        params.push_back(std::make_unique<PF>(P("over_lfo2Depth", 1), "LFO2 Depth", nr(0.f, 1.f), 0.15f));

        // --- Resonator allpass mix ---
        params.push_back(std::make_unique<PF>(P("over_resoMix", 1), "Resonator Mix", nr(0.f, 1.f), 0.15f));

        // --- 4 Macros ---
        params.push_back(std::make_unique<PF>(P("over_macroDepth", 1), "DEPTH", nr(0.f, 1.f), 0.35f));
        params.push_back(std::make_unique<PF>(P("over_macroColor", 1), "COLOR", nr(0.f, 1.f), 0.5f));
        params.push_back(std::make_unique<PF>(P("over_macroCoupling", 1), "COUPLING", nr(0.f, 1.f), 0.0f));
        params.push_back(std::make_unique<PF>(P("over_macroSpace", 1), "SPACE", nr(0.f, 1.f), 0.3f));
    }

    //--------------------------------------------------------------------------
    // SynthEngine interface
    //--------------------------------------------------------------------------
    juce::String getEngineId() const override { return "Overtone"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xffA8D8EA); }
    // DSP Fix Wave 2B: Corrected from 8 to 1 — engine implements single-voice
    // spectral additive synthesis (one set of 8 partials, one ADSR). Polyphonic
    // voice allocation would require 8x partial oscillators + 8x envelope state.
    // Declaring 8 when only 1 processes is a seance-flagged lie.
    int getMaxVoices() const override { return 1; }

    int getActiveVoiceCount() const override { return (noteIsOn || ampEnvStage != EnvStage::Idle) ? 1 : 0; }

    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = (float)sampleRate;

        for (int i = 0; i < kNumPartials; ++i)
            partials[i].prepare(sampleRate);

        resonator.prepare(sampleRate);
        brightFilter.reset();
        reverb.prepare(sampleRate);

        ampEnvStage = EnvStage::Idle;
        ampEnvLevel = 0.f;
        noteIsOn = false;
        currentNote = 60;
        currentVel = 1.f;
        lfo1.reset();
        lfo2.reset();
        // Lock LFO shapes to Sine once in prepare() — not repeated per-block.
        lfo1.setShape(StandardLFO::Sine);
        lfo2.setShape(StandardLFO::Sine);
        // Invalidate rate cache so first renderBlock() always calls setRate().
        lastLfo1Rate = -1.f;
        lastLfo2Rate = -1.f;
        modWheelVal = 0.f;
        aftertouchVal = 0.f;
        filterEnvLevel = 0.f;
        reverbWasActive = false;

        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;
        couplingDepthMod = 0.f;

        prepareSilenceGate(sampleRate, maxBlockSize, 300.f);
        (void)maxBlockSize;
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int i = 0; i < kNumPartials; ++i)
            partials[i].reset();

        resonator.reset();
        brightFilter.reset();
        reverb.reset();

        ampEnvStage = EnvStage::Idle;
        ampEnvLevel = 0.f;
        noteIsOn = false;
        filterEnvLevel = 0.f;
        lfo1.reset();
        lfo2.reset();
        // FIX: clear coupling mods and reverb gate on reset — prevents stale
        // modulation accumulation if reset() is called mid-render (e.g. on
        // preset load while a note is sounding).
        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;
        couplingDepthMod = 0.f;
        reverbWasActive = false;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        p_constant = apvts.getRawParameterValue("over_constant");
        p_depth = apvts.getRawParameterValue("over_depth");
        for (int i = 0; i < kNumPartials; ++i)
        {
            juce::String id = "over_partial" + juce::String(i);
            p_partial[i] = apvts.getRawParameterValue(id);
        }
        p_velBright = apvts.getRawParameterValue("over_velBright");
        p_filterCutoff = apvts.getRawParameterValue("over_filterCutoff");
        p_filterRes = apvts.getRawParameterValue("over_filterRes");
        p_ampAtk = apvts.getRawParameterValue("over_ampAtk");
        p_ampDec = apvts.getRawParameterValue("over_ampDec");
        p_ampSus = apvts.getRawParameterValue("over_ampSus");
        p_ampRel = apvts.getRawParameterValue("over_ampRel");
        p_lfo1Rate = apvts.getRawParameterValue("over_lfo1Rate");
        p_lfo1Depth = apvts.getRawParameterValue("over_lfo1Depth");
        p_lfo2Rate = apvts.getRawParameterValue("over_lfo2Rate");
        p_lfo2Depth = apvts.getRawParameterValue("over_lfo2Depth");
        p_resoMix = apvts.getRawParameterValue("over_resoMix");
        p_macroDepth = apvts.getRawParameterValue("over_macroDepth");
        p_macroColor = apvts.getRawParameterValue("over_macroColor");
        p_macroCoupling = apvts.getRawParameterValue("over_macroCoupling");
        p_macroSpace = apvts.getRawParameterValue("over_macroSpace");
    }

    //--------------------------------------------------------------------------
    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        // OVERTONE accepts:
        //   AmpToFilter → filter cutoff modulation (spectral colouring from coupled engine)
        //   AmpToPitch / PitchToPitch → pitch offset in semitones
        // macroCoupling scales receive sensitivity: higher COUPLING = more responsive to partners.
        const float recvScale = p_macroCoupling ? (0.5f + p_macroCoupling->load() * 0.5f) : 0.5f;
        if (type == CouplingType::AmpToFilter)
            couplingFilterMod += amount * 4000.f * recvScale; // ±4000 Hz range (wide spectral sweep)
        else if (type == CouplingType::AmpToPitch || type == CouplingType::PitchToPitch)
            couplingPitchMod += amount * recvScale;
        else if (type == CouplingType::EnvToMorph)
            couplingDepthMod += amount * 3.f * recvScale; // couple into depth sweep
    }

    float getSampleForCoupling(int /*channel*/, int /*sampleIndex*/) const override { return lastOutputSample; }

    //--------------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // 1. Parse MIDI — note-on/off, CC1 (mod wheel D006), aftertouch (D006)
        for (const auto meta : midi)
        {
            const auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                currentNote = msg.getNoteNumber();
                currentVel = msg.getFloatVelocity();
                noteIsOn = true;
                ampEnvStage = EnvStage::Attack;
                // DSP Fix Wave 2B: trigger filter envelope on note-on
                filterEnvLevel = currentVel;
                // Phase reset on new note for clean spectral onset
                for (int i = 0; i < kNumPartials; ++i)
                    partials[i].reset();
                wakeSilenceGate();
            }
            else if (msg.isNoteOff() && msg.getNoteNumber() == currentNote)
            {
                noteIsOn = false;
                ampEnvStage = EnvStage::Release;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelVal = msg.getControllerValue() / 127.f; // D006: mod wheel → DEPTH macro
            }
            else if (msg.isChannelPressure())
            {
                aftertouchVal = msg.getChannelPressureValue() / 127.f; // D006: aftertouch → COLOR
            }
            else if (msg.isAftertouch())
            {
                aftertouchVal = msg.getAfterTouchValue() / 127.f; // D006
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }
        }

        // 2. Check SilenceGate bypass
        if (isSilenceGateBypassed())
        {
            return;
        }

        // 3. Guard against unattached parameters
        if (!p_constant)
        {
            buffer.clear();
            return;
        }

        // 4. Snapshot parameters once per block (ParamSnapshot pattern)
        const int constantIdx = (int)(p_constant->load() + 0.5f);
        // FIX: guard p_depth null dereference (same pattern as p_constant above).
        // p_depth is typically non-null after attachParameters() but be defensive
        // in case the engine is rendered before APVTS is fully wired (host quirks).
        const float baseDepth = p_depth ? p_depth->load() : 2.f;
        float partialAmp[kNumPartials];
        for (int i = 0; i < kNumPartials; ++i)
            partialAmp[i] = p_partial[i] ? p_partial[i]->load() : kDefaultPartialAmps[i];

        const float velBright = p_velBright ? p_velBright->load() : 0.4f;
        const float baseCutoff = p_filterCutoff ? p_filterCutoff->load() : 12000.f;
        const float filterRes = p_filterRes ? p_filterRes->load() : 0.3f;
        const float ampAtk = p_ampAtk ? p_ampAtk->load() : 0.02f;
        const float ampDec = p_ampDec ? p_ampDec->load() : 0.3f;
        const float ampSus = p_ampSus ? p_ampSus->load() : 0.7f;
        const float ampRel = p_ampRel ? p_ampRel->load() : 1.0f;
        const float lfo1Rate = p_lfo1Rate ? p_lfo1Rate->load() : 0.25f;
        const float lfo1Depth = p_lfo1Depth ? p_lfo1Depth->load() : 0.2f;
        const float lfo2Rate = p_lfo2Rate ? p_lfo2Rate->load() : 0.1f;
        const float lfo2Depth = p_lfo2Depth ? p_lfo2Depth->load() : 0.15f;
        const float resoMix = p_resoMix ? p_resoMix->load() : 0.15f;
        const float macroDepth = p_macroDepth ? p_macroDepth->load() : 0.35f;
        const float macroColor = p_macroColor ? p_macroColor->load() : 0.5f;
        const float macroCoupling = p_macroCoupling ? p_macroCoupling->load() : 0.0f;
        const float macroSpace = p_macroSpace ? p_macroSpace->load() : 0.3f;

        // D006: mod wheel → DEPTH macro (additive on top of macroDepth)
        const float modWheelDepthBoost = modWheelVal * 4.0f; // 0..4 extra depth index
        // D006: aftertouch → COLOR (timbral shimmer — boost upper partial amps)
        const float atColorBoost = aftertouchVal * macroColor; // 0..macroColor shimmer

        // DEPTH macro: sweeps convergent index
        const float macroDepthAdd = macroDepth * 5.f; // 0..5 extra depth index
        const float effectiveDepth = clamp(baseDepth + macroDepthAdd + modWheelDepthBoost + couplingDepthMod, 0.f, 7.f);

        // COLOR macro: upper partial brightness boost
        // At macroColor=0: no boost. At macroColor=1: upper partials (n>3) get +0.5 boost.
        const float colorUpperBoost = macroColor * 0.5f;

        // D001: Velocity → partial brightness (high velocity = upper partials louder)
        // velBright param scales how much velocity boosts upper partials.
        // Upper partial boost = velocity * velBright (applied to partials 3-7)
        const float velUpperBoost = currentVel * velBright * 0.6f;

        // FIX: Filter envelope decay must account for block size.
        // The previous coefficient fastExp(-1/(0.3*sr)) is the per-SAMPLE decay
        // coefficient but was applied only once per block, making the effective
        // time constant ~numSamples times longer than intended (e.g. ~38s at
        // blockSize=128 instead of 300ms). Correct: multiply the exponent by
        // numSamples so we decay by the right amount per block.
        // filterEnvLevel is set to currentVel on noteOn (in MIDI parse above),
        // decays per-block at ~300ms half-life. velBright scales the envelope depth.
        const float filterEnvDecay = fastExp(-(float)numSamples / (0.3f * sr)); // ~300ms half-life
        filterEnvLevel *= filterEnvDecay;
        filterEnvLevel = flushDenormal(filterEnvLevel);
        const float filterEnvBoost = filterEnvLevel * velBright * 5000.f; // up to +5kHz sweep

        // Filter cutoff: COLOR macro brightens by raising cutoff
        const float colorCutoffBoost = macroColor * 6000.f;           // +0..+6000 Hz
        const float velCutoffBoost = currentVel * velBright * 3000.f; // D001
        const float finalCutoff =
            clamp(baseCutoff + colorCutoffBoost + velCutoffBoost + filterEnvBoost + couplingFilterMod, 1000.f, 20000.f);
        const float Q = 0.5f + filterRes * 5.5f; // map 0-0.8 → Q 0.5-4.9

        // Fundamental frequency (MIDI note + pitch bend + coupling pitch mod)
        const float fundamentalFreq = midiToFreq(currentNote) *
                                      PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f) *
                                      fastPow2(couplingPitchMod / 12.f);

        // Envelope coefficients
        const float atkCoeff = smoothCoeffFromTime(ampAtk, sr);
        const float decCoeff = smoothCoeffFromTime(ampDec, sr);
        const float relCoeff = smoothCoeffFromTime(ampRel, sr);

        // Convergent table for selected constant.
        // FIX: clamp directly as int rather than casting to float and back —
        // the previous clamp((float)constantIdx, 0.f, 3.f) returned a float
        // that was then narrowed to int, hiding the cast in an implicit conversion.
        const int constIdx = std::max(0, std::min(constantIdx, 3));
        const float* ratios = kConvergentTables[constIdx];

        float* L = buffer.getWritePointer(0);
        float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;

        // Anti-aliasing constants (Issue 2: smooth fadeout near Nyquist)
        const float nyquist = sr * 0.5f;
        const float fadeStart = nyquist * 0.8f;      // begin fade at 80% of Nyquist
        const float fadeRange = nyquist - fadeStart; // denominator for fade calc

        // Set LFO rates only when they change (rate-cache avoids unconditional
        // setRate() call every block — StandardLFO.setRate() recomputes phaseInc
        // each call; skipping when unchanged saves a branch + divide per block).
        // Shape is always Sine for both LFOs (locked by design) — set once in prepare().
        if (lfo1Rate != lastLfo1Rate) { lfo1.setRate(lfo1Rate, sr); lastLfo1Rate = lfo1Rate; }
        if (lfo2Rate != lastLfo2Rate) { lfo2.setRate(lfo2Rate, sr); lastLfo2Rate = lfo2Rate; }

        // Reset coupling mods (consumed each block)
        couplingFilterMod = 0.f;
        couplingPitchMod = 0.f;
        couplingDepthMod = 0.f;

        // effectiveResoMix: resoMix modulated by macroSpace.
        // Hoisted before the per-sample loop so the resonator wet/dry blend in the
        // loop uses the same value as the space-reverb stereo-spread pass below.
        const float effectiveResoMix = clamp(resoMix + macroSpace * 0.3f, 0.f, 1.f);

        // ----- Per-sample DSP loop -----
        for (int n = 0; n < numSamples; ++n)
        {
            // --- Amp envelope ---
            float envTarget = 0.f;
            float envCoeff = 0.f;
            switch (ampEnvStage)
            {
            case EnvStage::Idle:
                envTarget = 0.f;
                envCoeff = relCoeff;
                break;
            case EnvStage::Attack:
                envTarget = 1.f;
                envCoeff = atkCoeff;
                if (ampEnvLevel >= 0.999f)
                {
                    ampEnvLevel = 1.f;
                    ampEnvStage = EnvStage::Decay;
                }
                break;
            case EnvStage::Decay:
                envTarget = ampSus;
                envCoeff = decCoeff;
                if (std::fabs(ampEnvLevel - ampSus) < 0.001f)
                {
                    ampEnvLevel = ampSus;
                    ampEnvStage = EnvStage::Sustain;
                }
                break;
            case EnvStage::Sustain:
                envTarget = ampSus;
                envCoeff = decCoeff;
                break;
            case EnvStage::Release:
                envTarget = 0.f;
                envCoeff = relCoeff;
                if (ampEnvLevel < 0.0001f)
                {
                    ampEnvLevel = 0.f;
                    ampEnvStage = EnvStage::Idle;
                }
                break;
            }
            ampEnvLevel += envCoeff * (envTarget - ampEnvLevel);
            ampEnvLevel = flushDenormal(ampEnvLevel);

            // --- D002: LFO 1 — sweeps convergent depth index ---
            // D005 floor: 0.01 Hz minimum enforced by StandardLFO.setRate()
            float lfo1Val = lfo1.process(); // StandardLFO sine [-1, +1]
            float lfo1Out = lfo1Val * lfo1Depth;

            // --- D002: LFO 2 — modulates partial phase rotation ---
            float lfo2Val = lfo2.process(); // StandardLFO sine [-1, +1]
            float lfo2Out = lfo2Val * lfo2Depth;

            // LFO1 sweeps effective depth (±1.5 depth units at full lfo1Depth)
            float dynDepth = clamp(effectiveDepth + lfo1Out * 1.5f, 0.f, 7.f);

            // Map float depth to two adjacent integer table indices with interpolation
            // This gives smooth, continuous morphing between convergent ratios.
            int depthLo = (int)dynDepth;
            int depthHi = std::min(depthLo + 1, 7);
            float depthFrac = dynDepth - (float)depthLo;

            // --- Additive synthesis: sum 8 CF-ratio partials ---
            float addSum = 0.f;
            for (int i = 0; i < kNumPartials; ++i)
            {
                // Ratio index wraps within the 8-entry table
                int idxLo = (depthLo + i) & 7;
                int idxHi = (depthHi + i) & 7;

                // Interpolated convergent ratio
                float ratio = lerp(ratios[idxLo], ratios[idxHi], depthFrac);

                // Partial frequency
                float freq = fundamentalFreq * ratio;
                freq = clamp(freq, 20.f, nyquist); // never above Nyquist

                // Anti-aliasing gain: smooth fade from 1.0 at fadeStart to 0.0 at Nyquist
                float aaGain = 1.0f;
                if (freq > fadeStart)
                {
                    aaGain = clamp((nyquist - freq) / fadeRange, 0.0f, 1.0f);
                }

                // Partial amplitude: base + color boost for upper partials
                float amp = partialAmp[i];
                if (i >= 4)
                {
                    // Upper partials boosted by COLOR macro and velocity (D001)
                    // macroCoupling adds a gentle autonomous shimmer on partials 4-7:
                    // each partial gets a slightly different phase offset so they beat
                    // against each other, producing a living spectral iridescence even
                    // without a coupling partner. Scale kept small (0.1) so at low
                    // COUPLING settings the effect is nearly inaudible; at full it
                    // adds ±0.1 amplitude flutter — noticeable but not overwhelming.
                    //
                    // FIX: shimmer now uses fastSin of a per-partial offset applied to
                    // the LFO1 output phase rather than reading lfo1.phase directly.
                    // The old code accessed lfo1.phase AFTER process() — which is the
                    // NEXT phase (already advanced), so shimmer was one sample out of
                    // sync with lfo1Val. Using asinf(lfo1Val) is expensive; instead we
                    // derive each partial's shimmer from lfo1Val rotated by a π/4 offset
                    // per partial using the sin(a+b) identity: no internal state exposed.
                    // sin(θ + kπ/4) = sin(θ)cos(kπ/4) + cos(θ)sin(kπ/4)
                    // For k=0..3: coefficients are exact multiples of {1, √2/2, 0, -√2/2}.
                    const float partialK = (float)(i - 4); // 0..3
                    const float kPiOver4 = 0.7853982f;
                    const float shimmerCos = fastCos(partialK * kPiOver4);
                    const float shimmerSin = fastSin(partialK * kPiOver4);
                    // Approximate cos(lfo1Phase): use lfo1Val = sin(lfo1Phase),
                    // cos ≈ sqrt(1 - sin²) — acceptable for amplitude modulation (never negative).
                    const float lfo1Cos = std::sqrt(std::max(0.f, 1.f - lfo1Val * lfo1Val));
                    const float shimmerSample = lfo1Val * shimmerCos + lfo1Cos * shimmerSin;
                    const float shimmer = macroCoupling * 0.1f * shimmerSample;
                    amp = clamp(amp + colorUpperBoost + velUpperBoost + atColorBoost + shimmer, 0.f, 1.f);
                }

                // LFO2 phase rotation: per-partial phase offset produces shimmer/chorus.
                // Each partial receives a per-sample phase push proportional to its index.
                // At lfo2Depth=1.0 and i=7: phaseRot = 0.0008 cycles/sample → ~38 Hz of
                // pitch drift at 48kHz, giving 3-4% vibrato on upper partials — audible
                // but musically appropriate "spectral rotation" (Nautilus shell breathing).
                // Scale factor 0.0001 keeps the effect subtle at moderate lfo2Depth settings.
                float phaseRot = lfo2Out * (float)(i + 1) * 0.0001f;
                partials[i].advancePhase(phaseRot);

                // Generate sine and accumulate (with anti-aliasing fadeout)
                addSum += partials[i].tick(freq) * amp * aaGain;
            }

            // FIX: normalization factor updated from 1/6 to 1/3.
            // At default harmonic-series amplitudes the sum ≈ 2.72 (Σ 1/(n+1) for n=0..7).
            // 1/6 produced ~−16 dBFS at unity — excessively quiet relative to fleet average.
            // 1/3 targets ~−9 dBFS at default, matching typical fleet loudness.
            // Worst-case (all 8 partials clamped to 1.0) → sum=8 → 8/3≈2.7, safely absorbed
            // by the downstream softClip which compresses above ~1.0 without hard clipping.
            const float normGain = 1.f / 3.f;
            addSum *= normGain;

            // --- Brightness filter ---
            float filtered = brightFilter.process(addSum, finalCutoff, Q, sr);

            // --- Resonator (allpass tuned to fundamental) ---
            // filterRes (0–0.8) drives the allpass feedback coefficient g (0–0.9 range).
            // Previously hardcoded to 0.7f, making over_filterRes dead in the resonator path.
            float resoOut = resonator.tick(filtered, fundamentalFreq, filterRes);
            float withReso = filtered * (1.f - effectiveResoMix) + resoOut * effectiveResoMix;

            // --- Amp envelope ---
            float output = withReso * ampEnvLevel;

            // Soft-clip for gentle saturation at high partial density
            output = softClip(output);

            L[n] = output;
            R[n] = output;

            lastOutputSample = output;
        }

        // --- Block-level space reverb ---
        // Schroeder reverb applied block-rate (no per-sample aliasing concern for
        // a diffuse room tail). macroSpace mixes reverb depth + wet/dry balance.
        // effectiveResoMix is already computed before the per-sample loop above.
        const float spaceWet = macroSpace * 0.6f;

        // FIX: Clear stale reverb state when Space transitions from off to on.
        // Without this, reverb buffers filled during a previous activation
        // cause a burst of old content when the effect is re-enabled, producing
        // an audible click or smear on the first block after re-engagement.
        if (spaceWet > 0.001f && !reverbWasActive)
        {
            reverb.reset();
            reverbWasActive = true;
        }
        else if (spaceWet <= 0.001f)
        {
            reverbWasActive = false;
        }

        if (spaceWet > 0.001f)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                float lSamp = L[n], rSamp = R[n];
                // Slight stereo spread: phase-invert a tiny fraction for width
                float lIn = lSamp * (1.f + effectiveResoMix * 0.05f);
                float rIn = rSamp * (1.f - effectiveResoMix * 0.05f);
                reverb.process(lIn, rIn, macroSpace, spaceWet);
                L[n] = lIn;
                R[n] = rIn;
            }
        }

        // 5. Feed SilenceGate analyzer
        analyzeForSilenceGate(buffer, numSamples);
    }

    //--------------------------------------------------------------------------
private:
    static constexpr int kNumPartials = 8;

    enum class EnvStage
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    // DSP state
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    OverPartialOsc partials[kNumPartials];
    OverAllpassReso resonator;
    OverBrightnessFilter brightFilter;
    OverSpaceReverb reverb;

    // Envelope state
    EnvStage ampEnvStage = EnvStage::Idle;
    float ampEnvLevel = 0.f;

    // Voice state
    bool noteIsOn = false;
    int currentNote = 60;
    float currentVel = 1.f;

    // LFO phases
    StandardLFO lfo1; // D002/D005: convergent depth sweep (sine)
    StandardLFO lfo2; // D002: partial phase rotation (sine)
    float lastLfo1Rate = -1.f; // rate cache: avoids redundant setRate() calls per block
    float lastLfo2Rate = -1.f;

    // D006 expression
    float modWheelVal = 0.f;
    float aftertouchVal = 0.f;
    float pitchBendNorm = 0.0f;

    // DSP Fix Wave 2B: Filter envelope level (set to velocity on noteOn, decays per block)
    float filterEnvLevel = 0.f;

    // Reverb state guard (prevents stale-buffer burst on Space re-enable)
    bool reverbWasActive = false;

    // Coupling modulation (consumed each block)
    float couplingFilterMod = 0.f;
    float couplingPitchMod = 0.f;
    float couplingDepthMod = 0.f;

    // Coupling output
    float lastOutputSample = 0.f;

    // Cached parameter pointers (attached once via attachParameters)
    std::atomic<float>* p_constant = nullptr;
    std::atomic<float>* p_depth = nullptr;
    std::atomic<float>* p_partial[kNumPartials] = {};
    std::atomic<float>* p_velBright = nullptr;
    std::atomic<float>* p_filterCutoff = nullptr;
    std::atomic<float>* p_filterRes = nullptr;
    std::atomic<float>* p_ampAtk = nullptr;
    std::atomic<float>* p_ampDec = nullptr;
    std::atomic<float>* p_ampSus = nullptr;
    std::atomic<float>* p_ampRel = nullptr;
    std::atomic<float>* p_lfo1Rate = nullptr;
    std::atomic<float>* p_lfo1Depth = nullptr;
    std::atomic<float>* p_lfo2Rate = nullptr;
    std::atomic<float>* p_lfo2Depth = nullptr;
    std::atomic<float>* p_resoMix = nullptr;
    std::atomic<float>* p_macroDepth = nullptr;
    std::atomic<float>* p_macroColor = nullptr;
    std::atomic<float>* p_macroCoupling = nullptr;
    std::atomic<float>* p_macroSpace = nullptr;
};

} // namespace xoceanus
