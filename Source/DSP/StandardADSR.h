// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "FastMath.h"
#include <cmath>
#include <algorithm>

namespace xoceanus {

//==============================================================================
// StandardADSR — Shared amplitude envelope for the XOceanus fleet.
//
// Consolidates the identical ADSR pattern found in 20+ engines into a single
// correct implementation. Supports three shapes:
//   - AD:   Attack → Decay to zero (percussive, no sustain)
//   - AHD:  Attack → Hold → Decay to zero (with hold plateau)
//   - ADSR: Attack → Decay → Sustain → Release (standard keyboard envelope)
//
// Design: linear attack (snappy), exponential decay/release (natural).
// Denormal-safe, sample-rate-aware. Based on OnsetEnvelope (the most
// battle-tested implementation in the fleet).
//
// Usage:
//   StandardADSR env;
//   env.prepare (sampleRate);
//   env.setADSR (0.01f, 0.3f, 0.7f, 0.5f);  // A=10ms, D=300ms, S=70%, R=500ms
//   env.noteOn();
//
//   // Per-sample in render loop:
//   float level = env.process();
//   if (!env.isActive()) { voice.active = false; }
//
// Drop-in replacement for: OrcaADSR, OctoADSR, OracleADSR, ObscuraADSR,
// OuieADSR, OrigamiADSR, OpalADSR, BobAdsrEnvelope, DubAdsrEnvelope, etc.
//==============================================================================
struct StandardADSR
{
    enum class Stage { Idle, Attack, Hold, Decay, Sustain, Release };
    enum class Shape { AD = 0, AHD = 1, ADSR = 2 };

    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------

    /// Call once when sample rate changes.
    void prepare (float sampleRate) noexcept
    {
        sr = std::max (1.0f, sampleRate);
    }

    /// Set ADSR parameters. Shape defaults to ADSR.
    /// Attack/Decay/Release in seconds, Sustain in [0, 1].
    void setADSR (float attackSec, float decaySec, float sustain, float releaseSec) noexcept
    {
        // Minimum times to prevent division-by-zero and coefficient underflow
        float aSec = std::max (attackSec, 0.0001f);   // 0.1ms floor
        float dSec = std::max (decaySec, 0.001f);     // 1ms floor
        float rSec = std::max (releaseSec, 0.001f);   // 1ms floor

        attackRate = 1.0f / (sr * aSec);

        // Exponential decay: -4.6 = ln(0.01) → reaches 1% in dSec seconds
        decayCoeff   = 1.0f - std::exp (-4.6f / (sr * dSec));
        releaseCoeff = 1.0f - std::exp (-4.6f / (sr * rSec));

        sustainLevel = std::clamp (sustain, 0.0f, 1.0f);
    }

    /// Set envelope shape (AD, AHD, or ADSR).
    void setShape (Shape s) noexcept { shape = s; }

    /// Set hold time in seconds (only used in AHD shape).
    void setHold (float holdSec) noexcept
    {
        holdSamples = std::max (0, static_cast<int> (sr * holdSec));
    }

    //--------------------------------------------------------------------------
    // Simplified API — matches the 15-engine pattern exactly
    //--------------------------------------------------------------------------

    /// Set parameters in the simple format used by most engines.
    /// Always uses ADSR shape, no hold.
    void setParams (float attackSec, float decaySec, float sustain, float releaseSec,
                    float sampleRate) noexcept
    {
        prepare (sampleRate);
        setADSR (attackSec, decaySec, sustain, releaseSec);
        shape = Shape::ADSR;
    }

    //--------------------------------------------------------------------------
    // Triggers
    //--------------------------------------------------------------------------

    /// Start the envelope (attack phase). Resets level to 0.
    void noteOn() noexcept
    {
        stage = Stage::Attack;
        level = 0.0f;
        holdSamplesLeft = holdSamples;
    }

    /// Enter release phase. Call on noteOff.
    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    /// Immediate silence. Use for voice stealing.
    void kill() noexcept
    {
        level = 0.0f;
        stage = Stage::Idle;
    }

    /// Legato retrigger: restart attack from the current level rather than 0.
    /// Used when a new note overlaps an active voice (monophonic legato).
    /// Attack rate is adjusted so the rise from currentLevel → 1.0 takes attackSec.
    void retriggerFrom (float currentLevel,
                        float attackSec, float decaySec, float sustain, float releaseSec) noexcept
    {
        setADSR (attackSec, decaySec, sustain, releaseSec);
        shape = Shape::ADSR;
        level = currentLevel;
        stage = Stage::Attack;
        // Re-scale attack rate so the rise covers (1 - currentLevel) in attackSec.
        float range = 1.0f - currentLevel;
        if (range < 0.001f)
        {
            // Already near peak — skip attack, go straight to decay.
            level = 1.0f;
            stage = Stage::Decay;
        }
        else if (attackSec >= 0.0001f)
        {
            attackRate = range / (sr * attackSec);
        }
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
                    if (shape == Shape::AHD && holdSamplesLeft > 0)
                        stage = Stage::Hold;
                    else
                        stage = Stage::Decay;
                }
                return level;

            case Stage::Hold:
                if (--holdSamplesLeft <= 0)
                    stage = Stage::Decay;
                return level;

            case Stage::Decay:
                if (shape == Shape::ADSR && sustainLevel > 0.0f)
                {
                    // Decay toward sustain level
                    level -= (level - sustainLevel) * decayCoeff;
                    level = flushDenormal (level);
                    if (level <= sustainLevel + 0.001f)
                    {
                        level = sustainLevel;
                        stage = Stage::Sustain;
                    }
                }
                else
                {
                    // Decay toward zero (AD / AHD shapes, or ADSR with sustain=0)
                    level -= level * decayCoeff;
                    level = flushDenormal (level);
                    if (level < 1e-6f)
                    {
                        level = 0.0f;
                        stage = Stage::Idle;
                    }
                }
                return level;

            case Stage::Sustain:
                return level;

            case Stage::Release:
                level -= level * releaseCoeff;
                level = flushDenormal (level);
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

    void reset() noexcept
    {
        stage = Stage::Idle;
        level = 0.0f;
    }

    //--------------------------------------------------------------------------
    // State — public for snapshot/restore in undo system
    //--------------------------------------------------------------------------
    Stage stage = Stage::Idle;
    float level = 0.0f;

private:
    float sr = 48000.0f;
    Shape shape = Shape::ADSR;

    float attackRate   = 0.01f;
    float decayCoeff   = 0.001f;
    float releaseCoeff = 0.001f;
    float sustainLevel = 0.7f;

    int holdSamples     = 0;
    int holdSamplesLeft = 0;
};

} // namespace xoceanus
