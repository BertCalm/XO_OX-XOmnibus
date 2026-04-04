// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>
#include "../FastMath.h"

namespace xoceanus
{

//==============================================================================
// AttractorDrive — Chaotic saturation engine following a Lorenz attractor.
//
// Math Mixture: Lorenz Attractor + Fourier-inspired Spectral Tilt + Logistic Map.
//
// A Lorenz Attractor (the butterfly) generates 3D coordinates (X, Y, Z):
//   X → Saturation amount (drive intensity)
//   Y → Slew rate (how fast the drive responds — low = sluggish, high = snappy)
//   Z → Spectral tilt (dark ↔ bright bias of the saturation character)
//
// A Logistic Map modulates the speed of the attractor ODE integration.
// At r → 3.9, the speed becomes chaotically unpredictable.
//
// Parameters:
//   mfx_adBifurcation (CC 33) — logistic map r-value (1.0–3.99)
//   mfx_adDriveBase           — base drive amount before attractor modulation (0–1)
//   mfx_adSpeed               — base ODE integration speed (0–1)
//   mfx_adMix                 — dry/wet (0–1)
//
// Safety: DC-offset filter + hard limiter on output to handle chaotic runaway.
//==============================================================================
class AttractorDrive
{
public:
    AttractorDrive() = default;

    void prepare(double sampleRate)
    {
        sr = static_cast<float>(sampleRate);

        // Lorenz initial conditions (near the attractor, not at origin)
        lx = 1.0f;
        ly = 1.0f;
        lz = 1.0f;

        // Logistic map state
        logisticX = 0.5f;

        // Slew limiters for attractor outputs
        slewDrive = 0.0f;
        slewTilt = 0.0f;

        // Spectral tilt filter state (1-pole shelving approximation)
        tiltStateL = 0.0f;
        tiltStateR = 0.0f;

        // DC blocker
        dcStateL = dcStateR = dcPrevL = dcPrevR = 0.0f;
    }

    void reset()
    {
        lx = 1.0f;
        ly = 1.0f;
        lz = 1.0f;
        logisticX = 0.5f;
        slewDrive = 0.0f;
        slewTilt = 0.0f;
        tiltStateL = tiltStateR = 0.0f;
        dcStateL = dcStateR = dcPrevL = dcPrevR = 0.0f;
    }

    //--------------------------------------------------------------------------
    void processBlock(float* L, float* R, int numSamples, float bifurcation, float driveBase, float speed, float mix)
    {
        if (sr <= 0.0f)
            return;
        if (mix < 0.001f)
            return;

        // Lorenz system parameters (classic values: σ=10, ρ=28, β=8/3)
        constexpr float sigma = 10.0f;
        constexpr float rho = 28.0f;
        constexpr float beta = 8.0f / 3.0f;

        // Clamp bifurcation to valid logistic map range
        float r = std::max(1.0f, std::min(3.99f, 1.0f + bifurcation * 2.99f));

        // ODE integration step (scaled to audio rate).
        // Multiply by (sr / 44100.0f) so the Lorenz orbit completes the same physical
        // time at any sample rate — doubling sr doubles tick rate, so dt must scale up
        // proportionally to keep the orbit speed perceptually constant.
        float basedt = (0.0001f + speed * 0.002f) * (sr / 44100.0f);

        // How often we advance the attractor (every N samples for efficiency)
        constexpr int kAttractorSubsample = 4;

        for (int s = 0; s < numSamples; ++s)
        {
            // --- Advance Lorenz attractor (subsampled) ---
            if ((s % kAttractorSubsample) == 0)
            {
                // Logistic map modulates integration speed
                logisticX = r * logisticX * (1.0f - logisticX);
                logisticX = std::max(0.001f, std::min(0.999f, logisticX));

                float dt = basedt * (0.5f + logisticX);

                // Lorenz ODE (Euler integration — sufficient at audio sub-rate)
                float dx = sigma * (ly - lx);
                float dy = lx * (rho - lz) - ly;
                float dz = lx * ly - beta * lz;

                lx += dx * dt;
                ly += dy * dt;
                lz += dz * dt;

                // Safety clamp — Lorenz attractor is bounded but numerical
                // errors can cause drift with Euler integration
                lx = std::max(-50.0f, std::min(50.0f, lx));
                ly = std::max(-50.0f, std::min(50.0f, ly));
                lz = std::max(0.0f, std::min(60.0f, lz));
            }

            // --- Map attractor coordinates to DSP parameters ---
            // X → drive: normalize from [-50,50] to [0,1], add base
            float attractorDrive = (lx + 50.0f) / 100.0f; // 0 to 1
            float totalDrive = driveBase + attractorDrive * 0.5f;
            totalDrive = std::max(0.0f, std::min(2.0f, totalDrive));

            // Y → slew rate: normalize, use as smoothing coefficient
            float attractorSlew = (ly + 50.0f) / 100.0f;
            float slewCoeff = 0.01f + attractorSlew * 0.2f;
            slewDrive += (totalDrive - slewDrive) * slewCoeff;

            // Z → spectral tilt: normalize from [0,60] to [-1,+1]
            float attractorTilt = (lz / 30.0f) - 1.0f; // -1 to +1
            slewTilt += (attractorTilt - slewTilt) * 0.05f;

            // --- Apply saturation with slew-limited drive ---
            float driveAmount = 1.0f + slewDrive * 8.0f; // 1x to 17x

            float satL = fastTanh(L[s] * driveAmount);
            float satR = fastTanh(R[s] * driveAmount);

            // Normalize back (tanh compresses, so compensate)
            float compensation = 1.0f / std::max(0.3f, fastTanh(driveAmount * 0.7f));
            satL *= compensation * 0.5f;
            satR *= compensation * 0.5f;

            // --- Spectral tilt (simple 1-pole shelving) ---
            // Positive tilt = brighter (less LP), negative = darker (more LP)
            float tiltFreq = 2000.0f * std::pow(2.0f, slewTilt * 2.0f); // 500 Hz to 8 kHz
            float tiltCoeff = 1.0f - std::exp(-6.28318f * tiltFreq / sr);

            tiltStateL += (satL - tiltStateL) * tiltCoeff;
            tiltStateR += (satR - tiltStateR) * tiltCoeff;

            // Blend: tilt < 0 = use more LP (darker), tilt > 0 = use more dry (brighter)
            float tiltBlend = (slewTilt + 1.0f) * 0.5f; // 0 = full LP, 1 = full dry
            float outL = tiltStateL * (1.0f - tiltBlend) + satL * tiltBlend;
            float outR = tiltStateR * (1.0f - tiltBlend) + satR * tiltBlend;

            // --- DC blocker ---
            float dcCoeff = std::exp(-6.28318f * 5.0f / sr); // matched-Z transform
            float dcOutL = outL - dcPrevL + dcStateL * dcCoeff;
            float dcOutR = outR - dcPrevR + dcStateR * dcCoeff;
            dcPrevL = outL;
            dcPrevR = outR;
            dcStateL = dcOutL;
            dcStateR = dcOutR;

            // --- Hard limiter (safety valve for chaotic runaway) ---
            dcOutL = std::max(-1.0f, std::min(1.0f, dcOutL));
            dcOutR = std::max(-1.0f, std::min(1.0f, dcOutR));

            dcOutL = flushDenormal(dcOutL);
            dcOutR = flushDenormal(dcOutR);

            L[s] = L[s] * (1.0f - mix) + dcOutL * mix;
            R[s] = R[s] * (1.0f - mix) + dcOutR * mix;
        }
    }

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)

    // Lorenz attractor state
    float lx = 1.0f, ly = 1.0f, lz = 1.0f;

    // Logistic map state
    float logisticX = 0.5f;

    // Slew-limited parameter outputs
    float slewDrive = 0.0f;
    float slewTilt = 0.0f;

    // Spectral tilt filter
    float tiltStateL = 0.0f, tiltStateR = 0.0f;

    // DC blocker
    float dcStateL = 0.0f, dcStateR = 0.0f;
    float dcPrevL = 0.0f, dcPrevR = 0.0f;
};

} // namespace xoceanus
