#pragma once
#include <cmath>
#include <algorithm>
#include <array>
#include <cstring>

namespace xoverworld {

//==============================================================================
// FIREcho — SNES-style FIR-filtered echo / delay
//
// Models the SNES SPC700's echo unit: a delay buffer fed back through an
// 8-tap FIR filter. The FIR coefficients provide tone shaping on the echo
// tail — dull, ringy, or resonant depending on coefficient values.
//
// Parameters:
//   delay       int [0,15] — delay tap in units of 16ms (SNES convention)
//               actual delay = (delay+1) * 0.016 * sampleRate samples
//   feedback    float [0, 0.99] — echo feedback coefficient
//   mix         float [0, 1]    — wet/dry blend
//   firCoeffs   float[8]        — 8-tap FIR coefficients [-1, 1]
//
// Signal chain: input → delay buffer → FIR → feedback → mix with dry
//==============================================================================
class FIREcho
{
public:
    static constexpr int kMaxDelaySamples = 48000; // ~1s at 48kHz

    FIREcho()
    {
        std::memset(delayBuf, 0, sizeof(delayBuf));
        firCoeffs.fill(0.0f);
        firCoeffs[0] = 1.0f; // default: flat (delta)
    }

    void prepare(float sampleRate)
    {
        sr = sampleRate;
        writePos = 0;
        recalcDelay();
        std::memset(delayBuf, 0, sizeof(delayBuf));
    }

    // delay in SNES units [0,15]: each unit = 16ms
    void setDelay(int d)
    {
        delayUnits = std::max(0, std::min(15, d));
        recalcDelay();
    }

    void setFeedback(float fb)
    {
        feedback = std::max(-0.99f, std::min(0.99f, fb));
    }

    void setMix(float m)
    {
        mix = std::max(0.0f, std::min(1.0f, m));
    }

    void setFIRCoefficients(const float coeffs[8])
    {
        for (int i = 0; i < 8; ++i)
            firCoeffs[i] = std::max(-1.0f, std::min(1.0f, coeffs[i]));
    }

    float process(float x)
    {
        // Write dry input into delay buffer
        delayBuf[writePos] = x;

        // Read from delay tap
        const int readIdx = (writePos - delaySamples + kMaxDelaySamples) % kMaxDelaySamples;

        // Apply 8-tap FIR to the echo tap
        float echoOut = 0.0f;
        for (int t = 0; t < 8; ++t)
        {
            int idx = (readIdx - t * (delaySamples / 8 + 1) + kMaxDelaySamples) % kMaxDelaySamples;
            echoOut += firCoeffs[t] * delayBuf[idx];
        }

        // Clamp echo to prevent runaway feedback
        echoOut = std::max(-1.0f, std::min(1.0f, echoOut));

        // Feed back into delay buffer: mix echo into future write position
        // (write position holds dry; feedback added here means next reads see it)
        delayBuf[writePos] += feedback * echoOut;
        delayBuf[writePos] = std::max(-1.0f, std::min(1.0f, delayBuf[writePos]));

        // Flush denormals
        if (std::abs(delayBuf[writePos]) < 1e-15f)
            delayBuf[writePos] = 0.0f;

        writePos = (writePos + 1) % kMaxDelaySamples;

        return x + (echoOut - x) * mix;
    }

private:
    void recalcDelay()
    {
        // SNES: 1 unit = 16ms; clamp to buffer size
        delaySamples = std::max(1, std::min(
            static_cast<int>((delayUnits + 1) * 0.016f * sr),
            kMaxDelaySamples - 1));
    }

    float sr        = 44100.0f;
    int   delayUnits = 3;
    int   delaySamples = 2646; // (3+1)*16ms @ 44100
    float feedback  = 0.3f;
    float mix       = 0.0f;

    std::array<float, 8> firCoeffs;

    float delayBuf[kMaxDelaySamples];
    int   writePos = 0;
};

} // namespace xoverworld
