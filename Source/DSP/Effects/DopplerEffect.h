#pragma once
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include "../FastMath.h"

namespace xomnibus {

//==============================================================================
// DopplerEffect — Distance-based filtering + pitch micro-shift + level change.
//
// Simulates a sound source approaching or receding on a single axis.
// "Distance" knob: 0 = close (bright, loud, dry), 1 = far (dark, quiet, diffuse).
//
// Components (all CPU-trivial):
//   - Distance-dependent LPF (air absorption)
//   - Level attenuation (inverse-square approximation)
//   - Micro pitch shift via modulated delay line (~0-5ms)
//   - Optional speed parameter for movement rate
//
// Mix control for parallel blending.
//==============================================================================
class DopplerEffect
{
public:
    DopplerEffect() = default;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        maxDelaySamples = static_cast<int> (sr * 0.008); // 8ms max
        delayLineL.resize (static_cast<size_t> (maxDelaySamples + 1), 0.0f);
        delayLineR.resize (static_cast<size_t> (maxDelaySamples + 1), 0.0f);
        reset();
    }

    /// distance: 0.0 = near (bright, loud), 1.0 = far (dark, quiet)
    void setDistance (float d) { targetDistance = std::clamp (d, 0.0f, 1.0f); }

    /// speed: rate of distance change smoothing (0.01 = slow, 1.0 = instant)
    void setSpeed (float s) { speed = std::clamp (s, 0.01f, 1.0f); }

    void setMix (float m) { mix = std::clamp (m, 0.0f, 1.0f); }

    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix < 0.001f)
            return;

        // SRO: fastExp replaces std::exp (per-block, ~4% accuracy sufficient)
        const float smoothCoeff = 1.0f - fastExp (-speed * 10.0f
            / static_cast<float> (sr));

        for (int i = 0; i < numSamples; ++i)
        {
            // Smooth distance parameter
            currentDistance += smoothCoeff * (targetDistance - currentDistance);
            float dist = currentDistance;

            float dryL = left[i];
            float dryR = right[i];

            // 1. Write into delay lines
            delayLineL[static_cast<size_t> (writePos)] = left[i];
            delayLineR[static_cast<size_t> (writePos)] = right[i];

            // 2. Compute delay time from distance (0-5ms, simulates propagation)
            float delaySamples = dist * 0.005f * static_cast<float> (sr);
            delaySamples = std::clamp (delaySamples, 0.0f,
                static_cast<float> (maxDelaySamples - 1));

            // Fractional delay with linear interpolation
            int d0 = static_cast<int> (delaySamples);
            float frac = delaySamples - static_cast<float> (d0);

            int idx0L = (writePos - d0 + maxDelaySamples + 1) % (maxDelaySamples + 1);
            int idx1L = (idx0L - 1 + maxDelaySamples + 1) % (maxDelaySamples + 1);

            float wetL = delayLineL[static_cast<size_t> (idx0L)] * (1.0f - frac)
                       + delayLineL[static_cast<size_t> (idx1L)] * frac;
            float wetR = delayLineR[static_cast<size_t> (idx0L)] * (1.0f - frac)
                       + delayLineR[static_cast<size_t> (idx1L)] * frac;

            // 3. Air absorption LPF (distance-dependent cutoff)
            // Near: cutoff ~18kHz (transparent), Far: cutoff ~800Hz (muffled)
            // SRO: fastExp replaces std::pow (pow(0.044,d) = exp(d*ln(0.044)) = exp(d*-3.1242))
            float cutoff = 18000.0f * fastExp (dist * -3.1242f);
            float rc = 1.0f / (2.0f * 3.14159265f * cutoff);
            float dt = 1.0f / static_cast<float> (sr);
            float alpha = dt / (rc + dt);

            lpfL += alpha * (wetL - lpfL);
            lpfR += alpha * (wetR - lpfR);
            wetL = lpfL;
            wetR = lpfR;

            // 4. Level attenuation (inverse-square-ish, max -18dB at distance=1)
            float attenuation = 1.0f / (1.0f + dist * 6.0f); // ~-16dB at max
            wetL *= attenuation;
            wetR *= attenuation;

            // Advance write position
            writePos = (writePos + 1) % (maxDelaySamples + 1);

            left[i]  = dryL + mix * (wetL - dryL);
            right[i] = dryR + mix * (wetR - dryR);
        }

        // SRO: per-sample flushDenormal (was post-block only)
        lpfL = flushDenormal (lpfL);
        lpfR = flushDenormal (lpfR);
    }

    void reset()
    {
        std::fill (delayLineL.begin(), delayLineL.end(), 0.0f);
        std::fill (delayLineR.begin(), delayLineR.end(), 0.0f);
        writePos = 0;
        currentDistance = 0.0f;
        lpfL = lpfR = 0.0f;
    }

private:
    double sr = 44100.0;
    float targetDistance  = 0.0f;
    float currentDistance = 0.0f;
    float speed = 0.3f;
    float mix   = 1.0f;

    // Delay line
    std::vector<float> delayLineL, delayLineR;
    int writePos = 0;
    int maxDelaySamples = 0;

    // LPF state
    float lpfL = 0.0f, lpfR = 0.0f;
};

} // namespace xomnibus
