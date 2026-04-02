// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "../FastMath.h"
#include "../CytomicSVF.h"
#include "../ParameterSmoother.h"

namespace xoceanus {

//==============================================================================
// fXBreath — Membrane Collection organic air texture.
//
// Generates noise shaped by the input signal's envelope, adding breath
// and aspirant texture. On voice: realistic breath noise, "air."
// On synth: organic feel on sterile digital sources.
//
// Signal flow:
//   input → envelope follower → scale noise amplitude
//         → spectral tilt (LP filter on noise) → shape noise color
//         → mix with dry signal
//
// CPU budget: ~15 ops/sample
// Zero CPU when mix = 0 (early return in processBlock).
//
// Parameters:
//   breathAmount — [0, 1] : how much noise is added relative to signal envelope
//   tilt         — [0, 1] : 0=bright (12kHz LP), 1=dark (500Hz LP)
//   sensitivity  — [0, 1] : envelope follower gain (how much input drives noise)
//   mix          — [0, 1] : wet blend (adds to dry — noise is additive)
//==============================================================================
class fXBreath
{
public:
    fXBreath() = default;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = static_cast<float> (sampleRate);
        envState_ = 0.0f;

        noiseTiltL_.setMode (CytomicSVF::Mode::LowPass);
        noiseTiltR_.setMode (CytomicSVF::Mode::LowPass);
        noiseTiltL_.reset();
        noiseTiltR_.reset();

        // 5ms time constant (0.005f seconds) — fleet standard
        breathSmoother_.prepare (static_cast<float> (sampleRate), 0.005f);
        tiltSmoother_.prepare   (static_cast<float> (sampleRate), 0.005f);
        mixSmoother_.prepare    (static_cast<float> (sampleRate), 0.005f);

        breathSmoother_.snapTo (0.0f);
        tiltSmoother_.snapTo   (0.5f);
        mixSmoother_.snapTo    (0.0f);

        rngState_ = 0x12345678u;
    }

    void setBreathAmount (float b) noexcept
    {
        breathAmount_ = std::clamp (b, 0.0f, 1.0f);
        breathSmoother_.set (breathAmount_);
    }

    // tilt: 0 = bright (LP at 12kHz), 1 = dark (LP at 500Hz)
    void setTilt (float t) noexcept
    {
        tilt_ = std::clamp (t, 0.0f, 1.0f);
        tiltSmoother_.set (tilt_);
    }

    void setSensitivity (float s) noexcept
    {
        sensitivity_ = std::clamp (s, 0.0f, 1.0f);
    }

    void setMix (float m) noexcept
    {
        mix_ = std::clamp (m, 0.0f, 1.0f);
        mixSmoother_.set (mix_);
    }

    //--------------------------------------------------------------------------
    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix_ < 0.001f) return;  // zero CPU at mix=0

        const float smoothMix    = mixSmoother_.process();
        const float smoothBreath = breathSmoother_.process();
        const float smoothTilt   = tiltSmoother_.process();

        // Tilt filter: 500Hz (dark) to 12kHz (bright)
        const float tiltFreq = 500.0f + (1.0f - smoothTilt) * 11500.0f;
        // CytomicSVF setCoefficients(freq, resonance [0,1], sampleRate)
        // resonance = 0.5 → neutral / Butterworth-ish tilt
        noiseTiltL_.setCoefficients (tiltFreq, 0.5f, sr_);
        noiseTiltR_.setCoefficients (tiltFreq, 0.5f, sr_);

        // Envelope follower coefficients (5ms attack, 50ms release)
        const float attack  = std::exp (-1.0f / (sr_ * 0.005f));
        const float release = std::exp (-1.0f / (sr_ * 0.05f));

        for (int i = 0; i < numSamples; ++i)
        {
            const float mono = (std::fabs (left[i]) + std::fabs (right[i])) * 0.5f;

            // One-pole envelope follower
            const float coeff = (mono > envState_) ? attack : release;
            envState_ = coeff * envState_ + (1.0f - coeff) * mono;
            envState_ = xoceanus::flushDenormal(envState_);

            // Scale by sensitivity: sensitivity=0 → 1x, sensitivity=1 → 10x (clamped)
            const float envScaled = std::min (envState_ * (1.0f + sensitivity_ * 9.0f), 1.0f);

            // Generate independent noise for L and R (stereo breath texture)
            const float noiseL = whiteNoise() * envScaled * smoothBreath;
            const float noiseR = whiteNoise() * envScaled * smoothBreath;

            // Apply spectral tilt (LP colors noise from bright to dark)
            const float filteredL = noiseTiltL_.processSample (noiseL);
            const float filteredR = noiseTiltR_.processSample (noiseR);

            // Add to dry (additive mix — breath adds to signal, not replaces it)
            left[i]  += filteredL * smoothMix;
            right[i] += filteredR * smoothMix;
        }
    }

    //--------------------------------------------------------------------------
    void reset() noexcept
    {
        envState_ = 0.0f;
        noiseTiltL_.reset();
        noiseTiltR_.reset();
        breathSmoother_.snapTo (0.0f);
        tiltSmoother_.snapTo   (0.5f);
        mixSmoother_.snapTo    (0.0f);
        rngState_ = 0x12345678u;
    }

private:
    float sr_       = 44100.0f;
    float envState_ = 0.0f;

    float breathAmount_ = 0.0f;
    float tilt_         = 0.5f;
    float sensitivity_  = 0.5f;
    float mix_          = 0.0f;

    CytomicSVF noiseTiltL_, noiseTiltR_;
    ParameterSmoother breathSmoother_, tiltSmoother_, mixSmoother_;

    uint32_t rngState_ = 0x12345678u;

    // Fast white noise — Lehmer LCG (same RNG as StandardLFO S&H)
    float whiteNoise() noexcept
    {
        rngState_ = rngState_ * 196314165u + 907633515u;
        return static_cast<float> (static_cast<int32_t> (rngState_)) / 2147483648.0f;
    }
};

} // namespace xoceanus
