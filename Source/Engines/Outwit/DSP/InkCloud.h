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
    // inkDecay [0.01, 0.5]: shorter = snappier burst
    void prepare(double sampleRate, float inkDecay) noexcept
    {
        sr = static_cast<float>(sampleRate);
        decayTime = std::clamp(inkDecay, 0.001f, 1.0f);
        // Recompute decay coeff: exp(-1 / (decayTime * sr))
        decayCoeff = xoutwit::fastExp(-1.0f / (decayTime * sr));
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

        // One-pole LP (dark ink character — muffled burst)
        lpState = lpState + 0.15f * (noise - lpState);
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
    float envelope = 0.0f;
    float lpState = 0.0f;

    uint32_t lfsr = 0xAB9F3C1Du;
};

} // namespace xoutwit
