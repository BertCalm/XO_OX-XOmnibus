// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <vector>
#include "../FastMath.h"
#include "../CytomicSVF.h"
#include "../StandardLFO.h"

namespace xoceanus
{

//==============================================================================
// fXOratory — "Poetic Meter" boutique effect.
//
// A multi-tap delay where tap timing and gain follow poetic meter patterns.
// Iambic Pentameter, Haiku, Fibonacci — each pattern creates a unique rhythmic
// signature. Each tap has its own spatial position (Tomita narrative panning)
// and filter character (Moog per-tap resonant LP). The delay doesn't just
// echo — it *recites*.
//
// Signal flow:
//   input → circular delay buffer
//         → 12 read heads at [tapIndex × syllableMs]
//         → each tap: stress-weighted gain × per-tap SVF LP × spatial pan
//         → tap sum × drift evolution (Kakehashi)
//         → feedback path (SVF LP damping) → back to buffer
//         → mix with dry
//
// Ghost guidance baked in:
//   Moog      — per-tap CytomicSVF LP with resonance (replaces one-pole)
//   Tomita    — spatial narrative per pattern (not mechanical L/R alternation)
//   Schulze   — syllableMs extended to 1000ms (temporal architecture)
//   Kakehashi — drift parameter for evolving tap gains + Fibonacci default
//   Vangelis  — syllableMs and feedback macro-performable design
//
// CPU budget: ~85 ops/sample (~0.4% @ 48kHz)
// Bricks: CytomicSVF ×13 (12 per-tap + 1 feedback), StandardLFO ×1, FastMath
//
// Usage:
//   fXOratory fx;
//   fx.prepare(48000.0, 512);
//   fx.setPattern(6);              // 0=Iambic, ..., 6=Fibonacci (default)
//   fx.setSyllable(80.0f);         // ms between taps
//   fx.setAccent(0.7f);            // stressed vs unstressed contrast
//   fx.setSpread(0.6f);            // stereo width of spatial narrative
//   fx.setFeedback(0.4f);          // delay feedback
//   fx.setDamping(3000.0f);        // Hz — feedback LP cutoff
//   fx.setDampingResonance(0.2f);  // feedback LP resonance (Moog)
//   fx.setDrift(0.0f);             // tap gain evolution rate (Kakehashi)
//   fx.setMix(0.4f);
//   fx.processBlock(L, R, numSamples);
//==============================================================================
class fXOratory
{
public:
    fXOratory() = default;

    //--------------------------------------------------------------------------
    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr = sampleRate;

        // Delay buffer: max 12 taps × 1000ms = 12 seconds
        int maxSamples = static_cast<int>(sr * 12.0) + 1;
        delayL.assign(static_cast<size_t>(maxSamples), 0.0f);
        delayR.assign(static_cast<size_t>(maxSamples), 0.0f);
        bufferSize = maxSamples;
        writePos = 0;

        // Per-tap CytomicSVF LP filters (Moog)
        for (int t = 0; t < kMaxTaps; ++t)
        {
            tapFilterL[t].setMode(CytomicSVF::Mode::LowPass);
            tapFilterR[t].setMode(CytomicSVF::Mode::LowPass);
            tapFilterL[t].reset();
            tapFilterR[t].reset();

            // Per-tap filter frequency: earlier taps brighter, later darker
            float ratio = static_cast<float>(t) / static_cast<float>(kMaxTaps);
            tapFilterFreq[t] = 12000.0f * (1.0f - ratio * 0.7f); // 12kHz → 3.6kHz
        }

        // Feedback damping filter (Moog resonant LP)
        feedbackFilterL.setMode(CytomicSVF::Mode::LowPass);
        feedbackFilterR.setMode(CytomicSVF::Mode::LowPass);
        feedbackFilterL.setCoefficients(dampingFreq, dampingResonance, static_cast<float>(sr));
        feedbackFilterR.setCoefficients(dampingFreq, dampingResonance, static_cast<float>(sr));
        feedbackFilterL.reset();
        feedbackFilterR.reset();

        // Drift LFO (Kakehashi) — very slow, modulates tap gains
        driftLFO.setShape(StandardLFO::Sine);
        driftLFO.setRate(0.05f, static_cast<float>(sr)); // ~20s cycle
        driftLFO.reset();

        // Initialize effective tap gains from current pattern
        updatePattern();
    }

    //--------------------------------------------------------------------------
    /// Select meter pattern (0-7). Default: 6 (Fibonacci).
    void setPattern(int p)
    {
        pattern = (p >= 0 && p < kNumPatterns) ? p : 6;
        updatePattern();
    }

    /// Syllable duration in ms (5-1000). Schulze: extended to 1000ms.
    void setSyllable(float ms) { syllableMs = clamp(ms, 5.0f, 1000.0f); }

    /// Accent depth (0-1). How much louder stressed taps are vs unstressed.
    /// At 1.0, unstressed taps are silent. At 0.0, all taps equal.
    void setAccent(float a) { accent = clamp(a, 0.0f, 1.0f); }

    /// Stereo spread of spatial narrative (0-1).
    void setSpread(float s) { spread = clamp(s, 0.0f, 1.0f); }

    /// Delay feedback (0-0.9).
    void setFeedback(float fb) { feedback = clamp(fb, 0.0f, 0.9f); }

    /// Feedback damping LP cutoff in Hz (200-16000).
    void setDamping(float hz)
    {
        dampingFreq = clamp(hz, 200.0f, 16000.0f);
        feedbackFilterL.setCoefficients(dampingFreq, dampingResonance, static_cast<float>(sr));
        feedbackFilterR.setCoefficients(dampingFreq, dampingResonance, static_cast<float>(sr));
    }

    /// Feedback damping LP resonance (0-0.8). Moog: the feedback path *sings*.
    void setDampingResonance(float r)
    {
        dampingResonance = clamp(r, 0.0f, 0.8f);
        feedbackFilterL.setCoefficients(dampingFreq, dampingResonance, static_cast<float>(sr));
        feedbackFilterR.setCoefficients(dampingFreq, dampingResonance, static_cast<float>(sr));
    }

    /// Drift rate (0-1). Kakehashi: tap gains evolve over time.
    /// 0 = static pattern. 1 = all taps drift fully between stressed/unstressed.
    void setDrift(float d) { drift = clamp(d, 0.0f, 1.0f); }

    /// Dry/wet mix (0-1).
    void setMix(float m) { mix = clamp(m, 0.0f, 1.0f); }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock(float* L, float* R, int numSamples)
    {
        juce::ScopedNoDenormals noDenormals; // #602: flush denormals in SVF/feedback path

        if (mix < 0.001f)
        {
            if (!bypassed_)
            {
                reset();
                bypassed_ = true;
            }
            return;
        }
        bypassed_ = false;

        const float srF = static_cast<float>(sr);

        // Active taps for current pattern
        int numActive = std::max(1, patterns[pattern].numTaps); // #605: guard div-by-zero in gainComp

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = L[i];
            float inR = R[i];

            // === Drift LFO (Kakehashi) — modulates tap gains ===
            float driftVal = driftLFO.process();

            // === Read taps ===
            float tapSumL = 0.0f;
            float tapSumR = 0.0f;

            for (int t = 0; t < numActive; ++t)
            {
                // Tap delay position in samples
                float delaySamples = static_cast<float>((t + 1)) * syllableMs * 0.001f * srF;
                delaySamples = clamp(delaySamples, 1.0f, static_cast<float>(bufferSize - 2));

                // Read with linear interpolation
                float tapL = readDelay(delayL, delaySamples);
                float tapR = readDelay(delayR, delaySamples);

                // Per-tap filter (Moog): earlier taps brighter, later darker
                tapFilterL[t].setCoefficients_fast(tapFilterFreq[t], 0.3f, srF);
                tapFilterR[t].setCoefficients_fast(tapFilterFreq[t], 0.3f, srF);
                tapL = tapFilterL[t].processSample(tapL);
                tapR = tapFilterR[t].processSample(tapR);

                // Tap gain: base stress pattern + drift modulation (Kakehashi)
                float baseGain = patterns[pattern].stress[t];
                float driftOffset =
                    driftVal * drift * 0.5f * fastSin(static_cast<float>(t) * 2.39996f); // golden angle stagger
                float tapGain = clamp(baseGain + driftOffset, 0.0f, 1.0f);

                // Apply accent: unstressed taps are quieter
                tapGain = (1.0f - accent) + accent * tapGain;

                // Spatial panning (Tomita narrative)
                float panL, panR;
                getSpatialPan(t, numActive, panL, panR);

                tapSumL += tapL * tapGain * panL;
                tapSumR += tapR * tapGain * panR;
            }

            // Gain compensation (Architect condition #1)
            float gainComp = 1.0f / std::sqrt(static_cast<float>(numActive));
            tapSumL *= gainComp;
            tapSumR *= gainComp;

            // === Feedback path (Moog resonant LP damping) ===
            float fbL = flushDenormal(feedbackFilterL.processSample(tapSumL)) * feedback;
            float fbR = flushDenormal(feedbackFilterR.processSample(tapSumR)) * feedback;

            // === Write to delay buffer ===
            delayL[static_cast<size_t>(writePos)] = flushDenormal(inL + fbL);
            delayR[static_cast<size_t>(writePos)] = flushDenormal(inR + fbR);
            writePos = (writePos + 1) % bufferSize;

            // === Mix ===
            L[i] = inL * (1.0f - mix) + tapSumL * mix;
            R[i] = inR * (1.0f - mix) + tapSumR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        std::fill(delayL.begin(), delayL.end(), 0.0f);
        std::fill(delayR.begin(), delayR.end(), 0.0f);
        writePos = 0;

        for (int t = 0; t < kMaxTaps; ++t)
        {
            tapFilterL[t].reset();
            tapFilterR[t].reset();
        }
        feedbackFilterL.reset();
        feedbackFilterR.reset();
        driftLFO.reset();
    }

private:
    //--------------------------------------------------------------------------
    // Spatial narrative panning (Tomita)
    // Each meter pattern has a unique spatial story — not mechanical L/R alternation.
    void getSpatialPan(int tapIndex, int numTaps, float& panL, float& panR) const
    {
        float t = (numTaps > 0) ? static_cast<float>(tapIndex) / static_cast<float>(numTaps) : 0.0f;

        float panAngle = 0.0f; // [-1, +1]

        switch (pattern)
        {
        case 0: // Iambic: taps march left to right (a procession)
            panAngle = -1.0f + 2.0f * t;
            break;

        case 1: // Trochee: taps march right to left (retreat)
            panAngle = 1.0f - 2.0f * t;
            break;

        case 2: // Anapest: taps gather to center (convergence)
            panAngle = (t < 0.5f) ? (-1.0f + 4.0f * t) : (3.0f - 4.0f * t);
            break;

        case 3: // Dactyl: taps scatter from center (explosion)
            panAngle = (t < 0.5f) ? (1.0f - 4.0f * t) : (-3.0f + 4.0f * t);
            break;

        case 4: // Spondee: all taps center (wall of sound)
            panAngle = 0.0f;
            break;

        case 5: // Haiku: three pillars — left, center, right
        {
            int third = (tapIndex < numTaps / 3) ? 0 : (tapIndex < 2 * numTaps / 3) ? 1 : 2;
            static constexpr float positions[] = {-0.8f, 0.0f, 0.8f};
            panAngle = positions[third];
            break;
        }

        case 6: // Fibonacci: nautilus spiral from center outward
            panAngle = fastSin(t * 6.28318530718f * 1.618f);
            break;

        case 7: // Free Verse: pseudo-random placement (deterministic)
            panAngle = fastSin(static_cast<float>(tapIndex) * 7.919f);
            break;

        default:
            panAngle = (tapIndex % 2 == 0) ? -t : t;
            break;
        }

        // Apply spread and convert to equal-power pan law
        panAngle *= spread;
        float angle = (panAngle * 0.5f + 0.5f) * 1.5707963f; // [0, pi/2]
        panL = fastCos(angle);
        panR = fastSin(angle);
    }

    //--------------------------------------------------------------------------
    float readDelay(const std::vector<float>& buf, float delaySamples) const
    {
        int d = static_cast<int>(delaySamples);
        float frac = delaySamples - static_cast<float>(d);
        if (d < 1)
            d = 1;
        if (d >= bufferSize - 1)
            d = bufferSize - 2;

        int r0 = (writePos - d + bufferSize) % bufferSize;
        int r1 = (r0 - 1 + bufferSize) % bufferSize;

        return flushDenormal(lerp(buf[static_cast<size_t>(r0)], buf[static_cast<size_t>(r1)], frac));
    }

    //--------------------------------------------------------------------------
    void updatePattern()
    {
        // Update per-tap filter frequencies based on pattern's active tap count
        int numActive = patterns[pattern].numTaps;
        for (int t = 0; t < kMaxTaps; ++t)
        {
            float ratio = (numActive > 1) ? static_cast<float>(t) / static_cast<float>(numActive - 1) : 0.0f;
            tapFilterFreq[t] = 12000.0f * (1.0f - ratio * 0.7f);
        }
    }

    //--------------------------------------------------------------------------
    static constexpr int kMaxTaps = 12;
    static constexpr int kNumPatterns = 8;

    struct MeterPattern
    {
        const char* name;
        int numTaps;
        float stress[kMaxTaps];
    };

    // Meter patterns: stressed (1.0) vs unstressed (0.0)
    // Architect condition #2: unstressed taps have 0.15 gain floor for Haiku etc.
    static constexpr MeterPattern patterns[kNumPatterns] = {
        {"Iambic", 10, {0.15f, 1.0f, 0.15f, 1.0f, 0.15f, 1.0f, 0.15f, 1.0f, 0.15f, 1.0f, 0, 0}},
        {"Trochee", 8, {1.0f, 0.15f, 1.0f, 0.15f, 1.0f, 0.15f, 1.0f, 0.15f, 0, 0, 0, 0}},
        {"Anapest", 9, {0.15f, 0.15f, 1.0f, 0.15f, 0.15f, 1.0f, 0.15f, 0.15f, 1.0f, 0, 0, 0}},
        {"Dactyl", 9, {1.0f, 0.15f, 0.15f, 1.0f, 0.15f, 0.15f, 1.0f, 0.15f, 0.15f, 0, 0, 0}},
        {"Spondee", 6, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0, 0, 0, 0, 0, 0}},
        {"Haiku", 12, {0.15f, 0.15f, 0.15f, 0.15f, 1.0f, 0.15f, 0.15f, 0.15f, 0.15f, 0.15f, 0.15f, 1.0f}},
        {"Fibonacci", 12, {1.0f, 1.0f, 0.15f, 1.0f, 0.15f, 1.0f, 0.15f, 0.15f, 1.0f, 0.15f, 0.15f, 1.0f}},
        {"FreeVerse", 12, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f}},
    };

    //--------------------------------------------------------------------------
    double sr = 44100.0;

    // Delay buffers
    std::vector<float> delayL;
    std::vector<float> delayR;
    int bufferSize = 0;
    int writePos = 0;

    // Per-tap LP filters (Moog)
    CytomicSVF tapFilterL[kMaxTaps];
    CytomicSVF tapFilterR[kMaxTaps];
    float tapFilterFreq[kMaxTaps]{};

    // Feedback damping filter (Moog resonant LP)
    CytomicSVF feedbackFilterL;
    CytomicSVF feedbackFilterR;

    // Drift LFO (Kakehashi)
    StandardLFO driftLFO;

    // Parameters
    int pattern = 6; // Fibonacci default (Kakehashi)
    float syllableMs = 80.0f;
    float accent = 0.7f;
    float spread = 0.6f;
    float feedback = 0.4f;
    float dampingFreq = 3000.0f;
    float dampingResonance = 0.2f; // Moog resonant damping
    float drift = 0.0f;            // Kakehashi drift
    float mix = 0.0f;
    bool bypassed_ = true;
};

} // namespace xoceanus
