// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "FastMath.h"
#include <algorithm>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// FilterEnvelope — General-purpose ADSR envelope for filter/pitch modulation.
//
// Provides the independent filter envelope that was the #1 seance finding
// across the fleet: "bass programming requires a filter envelope separate
// from the amp ADSR." This module can target any parameter — filter cutoff,
// pitch, waveshape, etc. — via the output level scaled by the caller.
//
// Design: linear attack (snappy), exponential decay/release (natural).
// Matches the Onset envelope pattern, which is the most battle-tested in
// the fleet. Denormal-safe, sample-rate-aware, zero-allocation.
//
// Usage:
//   FilterEnvelope env;
//   env.prepare (sampleRate);
//   env.setADSR (0.005f, 0.3f, 0.3f, 0.5f);  // A=5ms, D=300ms, S=30%, R=500ms
//   env.trigger();
//
//   // Per-sample in render loop:
//   float envLevel = env.process();
//   float cutoffBoost = envLevel * velocity * 300.0f;  // +300 Hz at peak
//
//==============================================================================
struct FilterEnvelope
{
    enum class Stage
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------

    /// Call once when sample rate changes.
    void prepare(float sampleRate) noexcept
    {
        sr = sampleRate;
        recalcCoeffs();
    }

    /// Set ADSR times and sustain level.
    /// Attack/Decay/Release in seconds, Sustain in [0, 1].
    void setADSR(float attackSec, float decaySec, float sustainLevel, float releaseSec) noexcept
    {
        atkTime = std::max(attackSec, 0.0001f); // floor 0.1ms — prevents DC click
        decTime = std::max(decaySec, 0.001f);   // floor 1ms — prevents coeff underflow
        susLvl = std::clamp(sustainLevel, 0.0f, 1.0f);
        relTime = std::max(releaseSec, 0.001f); // floor 1ms
        recalcCoeffs();
    }

    //--------------------------------------------------------------------------
    // Triggers
    //--------------------------------------------------------------------------

    /// Start the envelope from attack phase. Call on noteOn.
    void trigger() noexcept
    {
        stage = Stage::Attack;
        // Don't reset level to 0 — allows re-triggering from current level
        // for legato-friendly behavior (no click from jump to zero).
    }

    /// Hard trigger — reset level to 0 before attack. Use when you want a
    /// clean start regardless of current state (e.g., mono voice retrigger).
    void triggerHard() noexcept
    {
        level = 0.0f;
        stage = Stage::Attack;
    }

    /// Enter release phase. Call on noteOff.
    void release() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    /// Immediately silence the envelope. Use for voice stealing.
    void kill() noexcept
    {
        level = 0.0f;
        stage = Stage::Idle;
    }

    //--------------------------------------------------------------------------
    // Processing
    //--------------------------------------------------------------------------

    /// Advance one sample and return envelope level [0, 1].
    float process() noexcept
    {
        switch (stage)
        {
        case Stage::Idle:
            return 0.0f;

        case Stage::Attack:
            level += attackRate;
            if (level >= 1.0f)
            {
                level = 1.0f;
                stage = Stage::Decay;
            }
            return level;

        case Stage::Decay:
            level -= (level - susLvl) * decayCoeff;
            level = flushDenormal(level);
            if (std::abs(level - susLvl) < 0.001f || level <= susLvl)
            {
                level = susLvl;
                stage = Stage::Sustain;
            }
            return level;

        case Stage::Sustain:
            return level;

        case Stage::Release:
            level -= level * releaseCoeff;
            level = flushDenormal(level);
            if (level < 1e-6f)
            {
                level = 0.0f;
                stage = Stage::Idle;
            }
            return level;
        }
        return 0.0f;
    }

    //--------------------------------------------------------------------------
    // State queries
    //--------------------------------------------------------------------------

    bool isActive() const noexcept { return stage != Stage::Idle; }
    float getLevel() const noexcept { return level; }
    Stage getStage() const noexcept { return stage; }

    //--------------------------------------------------------------------------
    // State — public for snapshot/restore in undo system
    //--------------------------------------------------------------------------
    Stage stage = Stage::Idle;
    float level = 0.0f;

private:
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671) // Default — overwritten by prepare()
    float atkTime = 0.005f;
    float decTime = 0.3f;
    float susLvl = 0.0f;
    float relTime = 0.5f;

    float attackRate = 0.0f;
    float decayCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    void recalcCoeffs() noexcept
    {
        if (sr <= 0.0f)
            return;
        // Linear attack: reach 1.0 in atkTime seconds
        attackRate = 1.0f / (sr * atkTime);
        // Exponential decay: -4.6 = ln(0.01) → reaches 1% in decTime seconds
        decayCoeff = 1.0f - std::exp(-4.6f / (sr * decTime));
        releaseCoeff = 1.0f - std::exp(-4.6f / (sr * relTime));
    }
};

} // namespace xoceanus
