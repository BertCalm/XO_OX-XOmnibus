// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// InkCloud.h — XOutwit ink cloud effect
//
// Models an octopus's defensive ink burst: a sharp transient noise burst that
// decays exponentially. Triggered on note-on with velocity + depth scaling.
//
// Adapter usage:
//   inkCloud.prepare(sampleRate, inkDecay)
//   inkCloud.trigger(velocity, inkDepth)
//   float sample = inkCloud.process()
//
// All state inline, no allocation.
//==============================================================================

#include "FastMath.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace xoutwit
{

class InkCloud
{
public:
    //--------------------------------------------------------------------------
    // Called once per block (renderBlock) with current sampleRate + inkDecay param.
    // inkDecay [0.01, 0.5]: shorter = snappier burst.
    // Recomputes coefficients only when parameters change to avoid per-block fastExp().
    void prepare(double sampleRate, float inkDecay) noexcept
    {
        float newSr = static_cast<float>(sampleRate);
        float newDecay = std::clamp(inkDecay, 0.001f, 1.0f);
        if (newSr == sr && newDecay == decayTime)
            return; // no change — skip recompute
        sr = newSr;
        decayTime = newDecay;
        // Recompute decay coeff: exp(-1 / (decayTime * sr))
        decayCoeff = xoutwit::fastExp(-1.0f / (decayTime * sr));
        // LP coeff for dark ink character: 500 Hz one-pole, sample-rate compensated.
        // Replaces hardcoded 0.15f which was only valid at 44.1 kHz.
        lpCoeff = 1.0f - xoutwit::fastExp(-2.0f * 3.14159265f * 500.0f / sr);
    }

    //--------------------------------------------------------------------------
    // Call on note-on. velocity [0,1], inkDepth [0,1] (owit_inkCloud param).
    void trigger(float velocity, float inkDepth) noexcept
    {
        if (inkDepth < 0.001f)
            return;
        // Peak amplitude scales with velocity and depth
        envelope = velocity * inkDepth;
    }

    //--------------------------------------------------------------------------
    // Returns one sample of band-limited noise shaped by the decaying envelope.
    float process() noexcept
    {
        if (envelope < 0.00001f)
        {
            envelope = 0.0f;
            return 0.0f;
        }

        // LFSR white noise
        lfsr ^= lfsr << 13;
        lfsr ^= lfsr >> 17;
        lfsr ^= lfsr << 5;
        float noise = static_cast<float>((int32_t)lfsr) * 4.656612e-10f;

        // One-pole LP (dark ink character — muffled burst, ~500 Hz cutoff)
        lpState = lpState + lpCoeff * (noise - lpState);
        lpState = xoutwit::flushDenormal(lpState);

        float out = lpState * envelope;

        // Decay
        envelope *= decayCoeff;
        envelope = xoutwit::flushDenormal(envelope);

        return out;
    }

    //--------------------------------------------------------------------------
    bool isIdle() const noexcept { return envelope < 0.00001f; }

private:
    float sr = 0.0f;  // Sentinel: must be set by prepare() before use
    float decayTime = 0.08f;
    float decayCoeff = 0.99f;
    float lpCoeff = 0.0709f; // 1-exp(-2π·500/44100); overwritten in prepare()
    float envelope = 0.0f;
    float lpState = 0.0f;

    uint32_t lfsr = 0xAB9F3C1Du;
};

} // namespace xoutwit
