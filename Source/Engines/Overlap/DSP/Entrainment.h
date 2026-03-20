#pragma once
// XOverlap Entrainment — Kuramoto phase-coupling model for 6 pulse oscillators.
//
// Models hydrodynamic synchronization between jellyfish bell contractions.
// At high entrainment (→1): all voice pulse phases converge to a shared mean,
// creating bloom pulsing where all voices contract simultaneously.
// At low entrainment (→0): voices pulse at independent natural rates (drift).
//
// Kuramoto coupling: dθ_i/dt = ω_i + (K/N) * Σ_j sin(θ_j - θ_i)
// Approximated per-block as: θ_i += dt * K * sin(θ_mean - θ_i)
// where K = entrain coefficient and θ_mean = mean phase of all active voices.
//
// Note: this is applied AFTER voice::process() has already advanced phases.
// It nudges phases toward the mean — a correction, not a replacement.

#include "Voice.h"
#include <array>
#include <cmath>

namespace xoverlap {

class Entrainment
{
public:
    void prepare(float sr) noexcept { sampleRate = sr; }

    //==========================================================================
    // Kuramoto nudge: push each active voice's pulse phase toward the mean.
    // Called once per sample in the renderBlock inner loop.
    void process(std::array<Voice, 6>& voices, float entrain, float /*pulseRate*/) noexcept
    {
        if (entrain < 0.001f)
            return;

        // Compute mean pulse phase via circular mean (handle 0/2π wraparound)
        float sinSum = 0.0f, cosSum = 0.0f;
        int   activeCount = 0;

        for (auto& v : voices)
        {
            if (v.isActive())
            {
                constexpr float twoPi = 6.28318530f;
                float theta = v.pulsePhase * twoPi;
                sinSum += std::sin(theta);
                cosSum += std::cos(theta);
                ++activeCount;
            }
        }

        if (activeCount < 2)
            return;  // need at least 2 voices to couple

        float meanTheta = std::atan2(sinSum, cosSum);
        float meanPhase = meanTheta / 6.28318530f;
        if (meanPhase < 0.0f) meanPhase += 1.0f;

        // Nudge each voice's phase toward mean (small delta proportional to entrain)
        // dt per sample = 1/sampleRate; coupling strength K = entrain
        float dt = 1.0f / sampleRate;
        float K  = entrain * 4.0f;  // scale: max coupling = 4 Hz phase velocity

        for (auto& v : voices)
        {
            if (!v.isActive()) continue;

            // Phase difference (wrapped to [-0.5, 0.5])
            float diff = meanPhase - v.pulsePhase;
            if (diff >  0.5f) diff -= 1.0f;
            if (diff < -0.5f) diff += 1.0f;

            // Kuramoto nudge: θ_i += K * sin(diff * 2π) * dt
            float nudge = K * std::sin(diff * 6.28318530f) * dt;
            v.pulsePhase += nudge;
            if (v.pulsePhase >= 1.0f) v.pulsePhase -= 1.0f;
            if (v.pulsePhase < 0.0f)  v.pulsePhase += 1.0f;
        }
    }

private:
    float sampleRate = 44100.0f;
};

} // namespace xoverlap
