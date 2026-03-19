#pragma once

//==============================================================================
// Bioluminescence.h — xoverlap::Bioluminescence
//
// Additive shimmer layer inspired by the bioluminescent flashes of deep-sea
// organisms. A bank of short comb-filter taps sampled from a circular buffer
// are modulated by slowly-drifting sine oscillators, creating an iridescent
// "glow" on top of the FDN output.
//
// Architecture:
//   fdnMono → circular buffer → N taps at harmonically-spaced offsets
//          → each tap × slowly-drifting amplitude modulator → sum → scale
//
// The delayBase parameter controls tap spacing (musical resonance locked to
// the FDN delay base), and amount controls wet mix [0, 1].
//
// At amount = 0: silent.
// At amount = 1: maximum bioluminescent shimmer.
//
// All state is fixed-size. No allocation after prepare().
//==============================================================================

#include "FastMath.h"
#include <array>
#include <cmath>

namespace xoverlap {

//==============================================================================
class Bioluminescence
{
public:
    //==========================================================================
    void prepare (double sampleRate) noexcept
    {
        sr = static_cast<float> (sampleRate);

        // Clear circular buffer
        buffer.fill (0.0f);
        writeHead = 0;

        // Initialise modulator phases with slightly spread initial angles
        for (int i = 0; i < kTaps; ++i)
        {
            modPhase[i] = static_cast<float> (i) / static_cast<float> (kTaps);
            // Each modulator runs at a slightly different rate (detuned)
            // base rate ≈ 0.3 Hz; offset per tap ≈ 0.07 Hz
            modRate[i] = (0.3f + static_cast<float> (i) * 0.07f) / sr;
        }
    }

    //==========================================================================
    // process() — produce one bioluminescent shimmer sample.
    //
    // fdnMono:   mono mix of FDN outputs
    // delayBase: FDN delay base in ms (taps are harmonically derived from this)
    // amount:    wet gain [0, 1]
    float process (float fdnMono, float delayBase, float amount) noexcept
    {
        if (amount < 0.001f)
        {
            // Still write to buffer to keep it fresh
            buffer[static_cast<size_t> (writeHead)] = flushDenormal (fdnMono);
            if (++writeHead >= kBufLen) writeHead = 0;
            return 0.0f;
        }

        // Write input to circular buffer
        buffer[static_cast<size_t> (writeHead)] = flushDenormal (fdnMono);

        float sum = 0.0f;
        float baseOffset = std::max (1.0f, delayBase * 0.001f * sr);

        for (int i = 0; i < kTaps; ++i)
        {
            // Tap offset: i-th harmonic of baseOffset (primes give inharmonic shimmer)
            float tapOffset = baseOffset * kTapRatios[i];
            int   tapInt    = static_cast<int> (tapOffset);
            float tapFrac   = tapOffset - static_cast<float> (tapInt);

            // Linear interpolation between two adjacent buffer positions
            int rp1 = writeHead - tapInt;
            int rp2 = rp1 - 1;
            while (rp1 < 0) rp1 += kBufLen;
            while (rp2 < 0) rp2 += kBufLen;
            rp1 %= kBufLen;
            rp2 %= kBufLen;
            float tapSample = buffer[static_cast<size_t> (rp1)] * (1.0f - tapFrac)
                            + buffer[static_cast<size_t> (rp2)] * tapFrac;

            // Modulate tap amplitude with slowly-drifting sine
            float modAmp = 0.5f + 0.5f * fastSin (modPhase[i] * 6.28318530f);

            // Advance modulator phase
            modPhase[i] += modRate[i];
            if (modPhase[i] >= 1.0f) modPhase[i] -= 1.0f;

            sum += tapSample * modAmp;
        }

        // Advance write head
        if (++writeHead >= kBufLen) writeHead = 0;

        // Normalise by tap count and apply amount
        float out = (sum / static_cast<float> (kTaps)) * amount * 0.35f;
        return flushDenormal (out);
    }

private:
    //==========================================================================
    static constexpr int   kTaps   = 7;
    static constexpr int   kBufLen = 8192;  // ~186ms at 44.1kHz — enough for any delayBase

    // Near-prime tap ratio multipliers for inharmonic shimmer character
    static constexpr float kTapRatios[kTaps] = { 1.0f, 1.31f, 1.71f, 2.09f, 2.61f, 3.19f, 3.89f };

    float sr        = 44100.0f;

    std::array<float, kBufLen> buffer {};
    int                        writeHead = 0;

    std::array<float, kTaps> modPhase {};
    std::array<float, kTaps> modRate  {};
};

} // namespace xoverlap
