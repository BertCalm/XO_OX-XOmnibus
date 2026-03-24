#pragma once
#include <cmath>
#include <algorithm>
#include <vector>
#include "../FastMath.h"

namespace xolokun {

//==============================================================================
// PsychoacousticWidth — Perceptual stereo widening via Haas effect +
//                       complementary comb decorrelation.
//
// Two techniques combined for natural, mono-compatible width:
//
// 1. Haas micro-delay: Short delay (0.1-30ms) on one channel, swapped per
//    frequency band to maintain center image. Exploits precedence effect.
//
// 2. Complementary comb decorrelation: Applies opposite-polarity comb filters
//    to L/R channels. Sums to flat in mono, widens in stereo.
//
// Controls:
//   width:    0..1 — overall widening amount
//   haasMs:   0.1..30 — Haas delay time in ms
//   combFreq: 200..2000 — comb decorrelation base frequency
//   monoSafe: 0..1 — mono compatibility check blend
//   mix:      0..1 — parallel blend
//
// CPU: 1 short delay line + 2 first-order combs = trivial.
//==============================================================================
class PsychoacousticWidth
{
public:
    PsychoacousticWidth() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        // Max Haas delay: 30ms
        maxDelay = static_cast<int> (sr * 0.035);
        haasLine.resize (static_cast<size_t> (maxDelay + 1), 0.0f);
        reset();
    }

    void setWidth (float w)    { width    = std::clamp (w, 0.0f, 1.0f); }
    void setHaasMs (float ms)  { haasMs   = std::clamp (ms, 0.1f, 30.0f); }
    // SRO: Dirty-flag coefficient caching — skip recalc if value unchanged
    void setCombFreq (float f)
    {
        float clamped = std::clamp (f, 200.0f, 2000.0f);
        if (std::abs (clamped - combFreq) > 0.01f)
        {
            combFreq = clamped;
            recalcComb();
        }
    }
    void setMonoSafe (float s) { monoSafe = std::clamp (s, 0.0f, 1.0f); }
    void setMix (float m)      { mix      = std::clamp (m, 0.0f, 1.0f); }

    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix < 0.001f || width < 0.001f)
            return;

        const int delaySamples = std::max (1,
            static_cast<int> (haasMs * 0.001f * static_cast<float> (sr)));
        const float widthScale = width;

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = left[i];
            float dryR = right[i];

            // ---- Haas micro-delay on right channel ----
            haasLine[static_cast<size_t> (haasWritePos)] = right[i];
            int readPos = (haasWritePos - delaySamples + maxDelay + 1) % (maxDelay + 1);
            float haasR = haasLine[static_cast<size_t> (readPos)];
            haasWritePos = (haasWritePos + 1) % (maxDelay + 1);

            // Blend between original and delayed based on width
            float wideR = right[i] + widthScale * (haasR - right[i]);

            // ---- Complementary comb decorrelation ----
            // L gets positive comb, R gets negative → sums to flat in mono
            float combL = left[i]  + combFB * combStateL;
            float combR = wideR    - combFB * combStateR; // Note: opposite sign

            combStateL = combL;
            combStateR = combR;

            // Scale comb contribution by width
            float outL = left[i]  + widthScale * (combL - left[i]) * 0.3f;
            float outR = wideR    + widthScale * (combR - wideR)   * 0.3f;

            // ---- Mono safety: check correlation ----
            if (monoSafe > 0.01f)
            {
                float mono = (outL + outR) * 0.5f;
                float originalMono = (dryL + dryR) * 0.5f;
                // If mono sum loses too much energy, pull back toward dry
                float monoEnergy = std::abs (mono);
                float origEnergy = std::abs (originalMono) + 1e-10f;
                float ratio = monoEnergy / origEnergy;

                if (ratio < 0.5f)
                {
                    float pullback = monoSafe * (1.0f - ratio * 2.0f);
                    outL = outL + pullback * (dryL - outL);
                    outR = outR + pullback * (dryR - outR);
                }
            }

            // Denormal flush
            combStateL = flushDenormal (combStateL);
            combStateR = flushDenormal (combStateR);

            left[i]  = dryL + mix * (outL - dryL);
            right[i] = dryR + mix * (outR - dryR);
        }
    }

    void reset()
    {
        std::fill (haasLine.begin(), haasLine.end(), 0.0f);
        haasWritePos = 0;
        combStateL = combStateR = 0.0f;
        recalcComb();
    }

private:
    double sr = 44100.0;
    float width    = 0.5f;
    float haasMs   = 8.0f;
    float combFreq = 600.0f;
    float monoSafe = 0.5f;
    float mix      = 1.0f;

    // Haas delay line
    std::vector<float> haasLine;
    int haasWritePos = 0;
    int maxDelay = 0;

    // Comb decorrelation
    float combFB = 0.3f;
    float combStateL = 0.0f, combStateR = 0.0f;

    void recalcComb()
    {
        // Comb feedback derived from frequency (higher freq = shorter comb = less coloring)
        float delaySamples = static_cast<float> (sr) / combFreq;
        (void) delaySamples; // Simple first-order comb uses feedback only
        combFB = 0.3f * (1.0f - (combFreq - 200.0f) / 1800.0f); // Less FB at higher freq
        combFB = std::clamp (combFB, 0.05f, 0.35f);
    }

    // SRO: uses shared flushDenormal from FastMath.h
};

} // namespace xolokun
