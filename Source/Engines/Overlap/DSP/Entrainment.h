// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// Entrainment.h — xoverlap::Entrainment
//
// Kuramoto-model hydrodynamic phase coupling across the 6 OVERLAP voices.
// In the Kuramoto model, each oscillator's phase is nudged toward the mean
// phase of its neighbours at a rate proportional to the coupling strength K.
//
//   dθᵢ/dt = ωᵢ + (K/N) Σⱼ sin(θⱼ − θᵢ)
//
// Here we apply a discrete per-sample update: the phase of each voice's
// internal oscillator is gently shifted toward the centroid phase of all
// active voices, scaled by the entrainment parameter [0, 1].
//
// At entrainment = 0: voices run freely at their MIDI pitches.
// At entrainment = 1: voices phase-lock into a single collective pulse —
//                     the "jellyfish bell contraction" sync state.
//
// The pulseRate parameter acts as an additional synchrony modulator:
// higher pulse rates create faster entrainment convergence.
//==============================================================================

#include "Voice.h"
#include "FastMath.h"
#include <array>
#include <cmath>

namespace xoverlap {

//==============================================================================
class Entrainment
{
public:
    //==========================================================================
    void prepare (double /*sampleRate*/) noexcept
    {
        // Reset control-rate counter and cached mean phase
        controlCounter_ = 0;
        cachedMeanPhase_ = 0.0f;
    }

    //==========================================================================
    // process() — called once per sample after voices have been ticked.
    // Applies Kuramoto coupling by nudging each voice's phase toward the
    // circular mean of all active voice phases.
    //
    // voices:     array of 6 Voice objects (phases read/written)
    // entrain:    coupling strength K  [0, 1]
    // pulseRate:  modulates convergence speed [0.01, 8] Hz
    //
    // SRO (2026-03-21): std::atan2 moved to control rate (every 64 samples).
    // Kuramoto synchronization time constant ~0.5s means 1kHz update rate
    // (64-sample blocks at 44.1kHz = ~689Hz) is perceptually equivalent
    // to sample-rate precision. Saves ~63/64 atan2 calls per sample.
    static constexpr int kControlRateDiv = 64;

    template <size_t N>
    void process (std::array<Voice, N>& voices,
                  float entrain,
                  float pulseRate) noexcept
    {
        if (entrain < 0.001f) return;

        // 1. Update circular mean only at control rate
        ++controlCounter_;
        if (controlCounter_ >= kControlRateDiv)
        {
            controlCounter_ = 0;

            float sinSum = 0.0f;
            float cosSum = 0.0f;
            int   count  = 0;
            for (auto& v : voices)
            {
                if (!v.isActive()) continue;
                float theta = v.phase * 6.28318530f;
                sinSum += fastSin (theta);
                cosSum += fastCos (theta);
                ++count;
            }

            if (count >= 2)
                cachedMeanPhase_ = std::atan2 (sinSum, cosSum);  // in [-π, π]
        }

        // Bail if fewer than 2 voices have ever been active
        // (cachedMeanPhase_ remains 0 until first valid computation)

        // 2. Coupling rate scales with entrain and pulse rate.
        //    Max shift per sample = entrain * pulseRate / sampleRate
        //    (handled as fractional phase shift in [0,1) units)
        // We compute a conservative max per-sample phase nudge:
        //   baseRate ≈ 0.0002 * pulseRate (tuned so full K=1 sync in ~0.5s)
        float nudgeAmt = entrain * 0.0002f * std::max (0.01f, pulseRate);

        // 3. Apply nudge to each active voice using cached mean phase
        for (auto& v : voices)
        {
            if (!v.isActive()) continue;

            float theta     = v.phase * 6.28318530f;
            float phaseDiff = cachedMeanPhase_ - theta;

            // Wrap to [-π, π]
            while (phaseDiff >  3.14159265f) phaseDiff -= 6.28318530f;
            while (phaseDiff < -3.14159265f) phaseDiff += 6.28318530f;

            // Kuramoto nudge: dθ = K * sin(Δθ) per discrete step
            float sinDiff = fastSin (phaseDiff);
            float phaseShift = nudgeAmt * sinDiff;

            // Convert back to normalised [0,1) phase and apply
            v.phase = v.phase + phaseShift / 6.28318530f;

            // Keep phase in [0, 1)
            if (v.phase >= 1.0f) v.phase -= 1.0f;
            if (v.phase <  0.0f) v.phase += 1.0f;
        }
    }

private:
    int   controlCounter_  = 0;
    float cachedMeanPhase_ = 0.0f;
};

} // namespace xoverlap
