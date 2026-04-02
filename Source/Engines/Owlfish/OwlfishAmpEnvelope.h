// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "OwlfishFastMath.h"
#include <cmath>

namespace xowlfish {

//==============================================================================
// AmpEnvelope -- Simple ADSR envelope for the monophonic owlfish voice.
//
// Linear attack, exponential decay/release. Designed for monophonic legato:
// noteOn() starts attack from current level (no click on re-trigger).
// noteOff() starts release from current level (smooth tail).
//
// All times in MILLISECONDS (matching OwlfishParamSnapshot values directly).
//   Attack:  0.001 ms - 8000 ms
//   Decay:   50 ms - 4000 ms
//   Sustain: 0.0 - 1.0 (level, not time)
//   Release: 50 ms - 8000 ms
//
// No JUCE dependencies. No allocations. Denormal-safe.
//==============================================================================

class AmpEnvelope
{
public:
    enum class State { Idle, Attack, Decay, Sustain, Release };

    AmpEnvelope() = default;

    //--------------------------------------------------------------------------
    /// Prepare for playback. Call once when sample rate is known.
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    //--------------------------------------------------------------------------
    /// Reset to idle state, level to zero.
    void reset()
    {
        state = State::Idle;
        level = 0.0f;
    }

    //--------------------------------------------------------------------------
    /// Set ADSR parameters.
    /// @param attackMs   Attack time in milliseconds (0.001 - 8000).
    /// @param decayMs    Decay time in milliseconds (50 - 4000).
    /// @param sustain    Sustain level (0.0 - 1.0).
    /// @param releaseMs  Release time in milliseconds (50 - 8000).
    void setParams (float attackMs, float decayMs, float sustain, float releaseMs)
    {
        // ---- Attack rate: linear ramp, per-sample increment ----
        // Clamp to minimum 0.001 ms to avoid division by zero
        float atkMs = attackMs;
        if (atkMs < 0.001f) atkMs = 0.001f;
        float attackSamples = atkMs * 0.001f * static_cast<float> (sr);
        attackRate = 1.0f / attackSamples;

        // ---- Decay rate: exponential coefficient per sample ----
        float dkMs = decayMs;
        if (dkMs < 0.1f) dkMs = 0.1f;
        float decaySamples = dkMs * 0.001f * static_cast<float> (sr);
        // Coefficient so that after decaySamples, ~63% of the way to target
        decayRate = 1.0f - std::exp (-1.0f / decaySamples);

        // ---- Sustain level ----
        sustainLevel = sustain;
        if (sustainLevel < 0.0f) sustainLevel = 0.0f;
        if (sustainLevel > 1.0f) sustainLevel = 1.0f;

        // ---- Release rate: exponential coefficient per sample ----
        float relMs = releaseMs;
        if (relMs < 0.1f) relMs = 0.1f;
        float releaseSamples = relMs * 0.001f * static_cast<float> (sr);
        releaseRate = 1.0f - std::exp (-1.0f / releaseSamples);
    }

    //--------------------------------------------------------------------------
    /// Trigger attack. Starts from current level for legato re-trigger smoothness.
    void noteOn()
    {
        state = State::Attack;
        // Do NOT reset level -- start from wherever we are for glitch-free legato
    }

    //--------------------------------------------------------------------------
    /// Trigger release from current level.
    void noteOff()
    {
        if (state != State::Idle)
            state = State::Release;
    }

    //--------------------------------------------------------------------------
    /// Process one sample. Returns envelope level in [0, 1].
    float processSample()
    {
        switch (state)
        {
            case State::Idle:
                return 0.0f;

            case State::Attack:
            {
                // Linear ramp toward 1.0
                level += attackRate;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    state = State::Decay;
                }
                break;
            }

            case State::Decay:
            {
                // Exponential decay toward sustainLevel
                level = sustainLevel + (level - sustainLevel) * (1.0f - decayRate);
                level = flushDenormal (level);

                // Transition when close enough to sustain
                if (std::fabs (level - sustainLevel) < 0.001f)
                {
                    level = sustainLevel;
                    state = State::Sustain;
                }
                break;
            }

            case State::Sustain:
            {
                level = sustainLevel;
                break;
            }

            case State::Release:
            {
                // Exponential decay toward 0
                level *= (1.0f - releaseRate);
                level = flushDenormal (level);

                if (level < 0.001f)
                {
                    level = 0.0f;
                    state = State::Idle;
                }
                break;
            }
        }

        return level;
    }

    //--------------------------------------------------------------------------
    /// Returns false only when in Idle state (voice can be freed).
    bool isActive() const { return state != State::Idle; }

    /// Get current envelope level (0–1). Read at block rate for filter env modulation.
    float getLevel() const { return level; }

    /// Get current envelope state.
    State getState() const { return state; }

private:
    State  state = State::Idle;
    float  level = 0.0f;
    double sr    = 44100.0;

    float attackRate   = 0.0f;    // per-sample linear increment
    float decayRate    = 0.0f;    // per-sample exponential coefficient
    float sustainLevel = 0.8f;
    float releaseRate  = 0.0f;    // per-sample exponential coefficient
};

} // namespace xowlfish
