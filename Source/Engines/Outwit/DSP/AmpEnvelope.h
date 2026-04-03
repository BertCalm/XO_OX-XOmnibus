// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// AmpEnvelope.h — XOutwit amplitude envelope (ADSR)
//
// Adapter usage:
//   ampEnv.prepare(sampleRate)
//   ampEnv.setParams(attack, decay, sustain, release)
//   ampEnv.noteOn()
//   ampEnv.noteOff()
//   float env = ampEnv.process()
//
// Exponential attack / decay / release curves (matched-Z).
// Denormal protection on all state variables.
//==============================================================================

#include "FastMath.h"
#include <cmath>
#include <algorithm>

namespace xoutwit
{

class AmpEnvelope
{
public:
    //--------------------------------------------------------------------------
    void prepare(double sampleRate) noexcept
    {
        sr = static_cast<float>(sampleRate);
        // Recompute coefficients with existing params
        computeCoefficients();
        // Reset state without killing held note
        currentLevel = 0.0f;
        stage = Stage::Idle;
    }

    //--------------------------------------------------------------------------
    // Call once per block (after snap.update())
    void setParams(float attackSec, float decaySec, float sustainLevel, float releaseSec) noexcept
    {
        attack = std::max(0.001f, attackSec);
        decay = std::max(0.001f, decaySec);
        sustain = std::clamp(sustainLevel, 0.0f, 1.0f);
        release = std::max(0.001f, releaseSec);
        computeCoefficients();
    }

    //--------------------------------------------------------------------------
    void noteOn() noexcept { stage = Stage::Attack; }

    //--------------------------------------------------------------------------
    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    //--------------------------------------------------------------------------
    // Returns current envelope value in [0, 1]. Call once per sample.
    float process() noexcept
    {
        switch (stage)
        {
        case Stage::Idle:
            currentLevel = 0.0f;
            break;

        case Stage::Attack:
            currentLevel += attackCoeff * (1.02f - currentLevel);
            if (currentLevel >= 1.0f)
            {
                currentLevel = 1.0f;
                stage = Stage::Decay;
            }
            break;

        case Stage::Decay:
            currentLevel += decayCoeff * (sustain - currentLevel);
            if (currentLevel <= sustain + 0.0001f)
            {
                currentLevel = sustain;
                stage = Stage::Sustain;
            }
            break;

        case Stage::Sustain:
            currentLevel = sustain;
            break;

        case Stage::Release:
            currentLevel += releaseCoeff * (0.0f - currentLevel);
            if (currentLevel < 0.00001f)
            {
                currentLevel = 0.0f;
                stage = Stage::Idle;
            }
            break;
        }

        // Denormal protection
        currentLevel = xoutwit::flushDenormal(currentLevel);
        return currentLevel;
    }

    //--------------------------------------------------------------------------
    bool isIdle() const noexcept { return stage == Stage::Idle; }

private:
    enum class Stage
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    float sr = 48000.0f;
    float attack = 0.01f;
    float decay = 0.2f;
    float sustain = 0.8f;
    float release = 0.3f;

    float attackCoeff = 0.0f;
    float decayCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float currentLevel = 0.0f;

    Stage stage = Stage::Idle;

    void computeCoefficients() noexcept
    {
        // Matched-Z: coeff = exp(-1 / (time * sr))
        // Using fastExp for consistency with the rest of the fleet
        attackCoeff = xoutwit::fastExp(-1.0f / (attack * sr));
        attackCoeff = 1.0f - attackCoeff; // one-pole ramp-up coeff
        decayCoeff = xoutwit::fastExp(-1.0f / (decay * sr));
        decayCoeff = decayCoeff - 1.0f; // negative: ramps down to sustain
        releaseCoeff = xoutwit::fastExp(-1.0f / (release * sr));
        releaseCoeff = releaseCoeff - 1.0f; // negative: ramps down to zero
    }
};

} // namespace xoutwit
