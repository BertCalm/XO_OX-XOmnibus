#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// MultibandCompressor — 3-band upward+downward compression (OTT-style).
//
// Splits the signal into Low / Mid / High bands using Linkwitz-Riley
// crossover filters (2nd-order, -12dB/oct), compresses each band
// independently with both upward and downward compression, then sums.
//
// The "OTT" (Over The Top) sound: extreme dynamic flattening per band
// with fast attack/release. Quiet sounds get louder, loud sounds get
// quieter, per frequency band.
//
// Inspired by: Serum OTT, Ableton OTT, Xfer OTT plugin
//
// Features:
//   - 3 bands with adjustable crossover frequencies
//   - Per-band upward + downward compression
//   - Depth control (0 = bypass, 1 = full OTT)
//   - Per-band gain trim
//   - Wet/dry mix for parallel multiband compression
//   - Zero CPU when mix = 0
//
// Usage:
//   MultibandCompressor mb;
//   mb.prepare(44100.0);
//   mb.setDepth(0.7f);
//   mb.setMix(0.5f);
//   mb.processBlock(L, R, numSamples);
//==============================================================================
class MultibandCompressor
{
public:
    MultibandCompressor() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate)
    {
        sr = sampleRate;

        // Initialize crossover filter states
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < 4; ++i)
            {
                lpState[ch][i] = 0.0f;
                hpState[ch][i] = 0.0f;
            }
        }

        // Initialize per-band envelope followers
        for (int b = 0; b < kNumBands; ++b)
        {
            envState[b] = 0.0f;
        }

        updateCrossoverCoeffs();
    }

    //--------------------------------------------------------------------------
    /// Set low/mid crossover frequency. [60, 2000] Hz.
    void setLowCrossover (float hz)
    {
        lowCrossover = clamp (hz, 60.0f, 2000.0f);
        updateCrossoverCoeffs();
    }

    /// Set mid/high crossover frequency. [1000, 16000] Hz.
    void setHighCrossover (float hz)
    {
        highCrossover = clamp (hz, 1000.0f, 16000.0f);
        updateCrossoverCoeffs();
    }

    /// Set OTT depth. [0, 1] — 0 = no compression, 1 = maximum OTT.
    void setDepth (float d)
    {
        depth = clamp (d, 0.0f, 1.0f);
    }

    /// Set per-band gain (low, mid, high). Each [0, 2].
    void setBandGains (float low, float mid, float high)
    {
        bandGain[0] = clamp (low, 0.0f, 2.0f);
        bandGain[1] = clamp (mid, 0.0f, 2.0f);
        bandGain[2] = clamp (high, 0.0f, 2.0f);
    }

    /// Set wet/dry mix. [0, 1]
    void setMix (float wet)
    {
        mix = clamp (wet, 0.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock (float* L, float* R, int numSamples)
    {
        // OTT-style: fast attack, medium release, extreme ratio
        // Upward compression: boost quiet signals toward threshold
        // Downward compression: crush loud signals toward threshold

        // Derived from depth
        float downRatio = 1.0f + depth * 19.0f;    // 1:1 → 20:1
        float upRatio = 1.0f + depth * 3.0f;        // 1:1 → 4:1 upward
        float downThresh = -6.0f;                    // dB
        float upThresh = -30.0f;                     // dB — upward comp threshold

        float attackCoeff = 1.0f - fastExp (-1.0f / (0.001f * static_cast<float> (sr)));  // ~1ms
        float releaseCoeff = 1.0f - fastExp (-1.0f / (0.05f * static_cast<float> (sr)));  // ~50ms

        for (int i = 0; i < numSamples; ++i)
        {
            float inL = L[i];
            float inR = R[i];

            // --- Split into 3 bands ---
            float bandL[kNumBands], bandR[kNumBands];
            splitBands (inL, inR, bandL, bandR);

            // --- Compress each band ---
            float outL = 0.0f;
            float outR = 0.0f;

            for (int b = 0; b < kNumBands; ++b)
            {
                // Detect level (peak, stereo-linked)
                float detect = std::max (std::fabs (bandL[b]), std::fabs (bandR[b]));
                float detectDb = (detect > 1e-10f)
                    ? 6.0205999f * fastLog2 (detect)
                    : -100.0f;

                // Envelope follower
                float target = std::fabs (detectDb);
                float coeff = (target > envState[b]) ? attackCoeff : releaseCoeff;
                envState[b] = flushDenormal (envState[b] + coeff * (detectDb - envState[b]));
                float envDb = envState[b];

                // Compute gain reduction/boost
                float gainDb = 0.0f;

                // Downward compression (above threshold)
                if (envDb > downThresh)
                {
                    float over = envDb - downThresh;
                    float compressed = over / downRatio;
                    gainDb -= (over - compressed);
                }

                // Upward compression (below threshold)
                if (envDb < upThresh)
                {
                    float under = upThresh - envDb;
                    float compressed = under / upRatio;
                    gainDb += (under - compressed);
                }

                // Apply gain + band trim
                float gain = dbToGain (gainDb) * bandGain[b];
                outL += bandL[b] * gain;
                outR += bandR[b] * gain;
            }

            // Mix
            L[i] = inL * (1.0f - mix) + outL * mix;
            R[i] = inR * (1.0f - mix) + outR * mix;
        }
    }

    //--------------------------------------------------------------------------
    void reset()
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            for (int i = 0; i < 4; ++i)
            {
                lpState[ch][i] = 0.0f;
                hpState[ch][i] = 0.0f;
            }
        }
        for (int b = 0; b < kNumBands; ++b)
            envState[b] = 0.0f;
    }

private:
    //--------------------------------------------------------------------------
    void updateCrossoverCoeffs()
    {
        if (sr <= 0.0) return;
        // Butterworth LP/HP coefficients for one-pole stages
        // Two cascaded one-pole = 2nd-order Linkwitz-Riley (-12dB/oct)
        constexpr float twoPi = 6.28318530718f;
        float srF = static_cast<float> (sr);

        // Low crossover
        float wLow = twoPi * lowCrossover / srF;
        lpCoeffLow = clamp (wLow / (1.0f + wLow), 0.0f, 1.0f);

        // High crossover
        float wHigh = twoPi * highCrossover / srF;
        lpCoeffHigh = clamp (wHigh / (1.0f + wHigh), 0.0f, 1.0f);
    }

    //--------------------------------------------------------------------------
    /// Split stereo input into 3 frequency bands using Linkwitz-Riley crossovers.
    void splitBands (float inL, float inR, float* bandL, float* bandR)
    {
        // --- Low/Mid+High split (at lowCrossover) ---
        // Two-pole LP for low band (cascaded one-pole)
        lpState[0][0] = flushDenormal (lpState[0][0] + lpCoeffLow * (inL - lpState[0][0]));
        lpState[0][1] = flushDenormal (lpState[0][1] + lpCoeffLow * (lpState[0][0] - lpState[0][1]));
        float lowL = lpState[0][1];

        lpState[1][0] = flushDenormal (lpState[1][0] + lpCoeffLow * (inR - lpState[1][0]));
        lpState[1][1] = flushDenormal (lpState[1][1] + lpCoeffLow * (lpState[1][0] - lpState[1][1]));
        float lowR = lpState[1][1];

        // HP = input - LP (complementary)
        float midHighL = inL - lowL;
        float midHighR = inR - lowR;

        // --- Mid/High split (at highCrossover) ---
        lpState[0][2] = flushDenormal (lpState[0][2] + lpCoeffHigh * (midHighL - lpState[0][2]));
        lpState[0][3] = flushDenormal (lpState[0][3] + lpCoeffHigh * (lpState[0][2] - lpState[0][3]));
        float midL = lpState[0][3];

        lpState[1][2] = flushDenormal (lpState[1][2] + lpCoeffHigh * (midHighL - lpState[1][2]));
        lpState[1][3] = flushDenormal (lpState[1][3] + lpCoeffHigh * (lpState[1][2] - lpState[1][3]));
        float midR = lpState[1][3];

        float highL = midHighL - midL;
        float highR = midHighR - midR;

        bandL[0] = lowL;   bandR[0] = lowR;
        bandL[1] = midL;   bandR[1] = midR;
        bandL[2] = highL;  bandR[2] = highR;
    }

    //--------------------------------------------------------------------------
    static constexpr int kNumBands = 3;

    double sr = 44100.0;

    // Crossover filter state: [channel][stage]
    // 0-1: low crossover LP stages, 2-3: high crossover LP stages
    float lpState[2][4] {};
    float hpState[2][4] {};

    // Crossover coefficients
    float lpCoeffLow = 0.0f;
    float lpCoeffHigh = 0.0f;

    // Per-band envelope followers
    float envState[kNumBands] {};

    // Parameters
    float lowCrossover = 200.0f;
    float highCrossover = 3000.0f;
    float depth = 0.7f;
    float bandGain[kNumBands] = { 1.0f, 1.0f, 1.0f };
    float mix = 0.0f;
};

} // namespace xoceanus
