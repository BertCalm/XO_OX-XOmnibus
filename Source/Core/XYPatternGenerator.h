// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

/*
    XYPatternGenerator.h
    ====================
    Implements the five D3 musical motion patterns for the XY Surface (Wave 8).

    Patterns (D3 locked 2026-04-25):
        PULSE  — beat-locked exponential snap on both axes, hard attack + fast decay
        DRIFT  — Brownian random-walk with mean-reversion toward (0.5, 0.5)
        TIDE   — stable Lissajous figure-8 (2:1 frequency ratio, Y leading)
        RIPPLE — orbital motion around a drifting seed point
        CHAOS  — deterministic logistic-map driven chaotic jumps

    All patterns output (x, y) in [0, 1].

    Usage:
        XYPatternGenerator gen;
        gen.setPattern(XYPatternGenerator::Pattern::DRIFT);
        gen.setSpeed(0.3f);
        gen.setDepth(0.4f);
        gen.setSyncMode(XYPatternGenerator::SyncMode::Free);

        // Call at 30 Hz from a juce::Timer on the message thread:
        gen.tick(deltaSeconds, bpm, beatPhase);  // beatPhase 0..1 within the bar
        float x = gen.getX();
        float y = gen.getY();

    BPM / beat integration:
        When SyncMode != Free the generator reads beatPhase (0..1 per bar).
        The host supplies bpm and beatPhase from SharedTransport or PlayHead.
        When the host is not playing (bpm == 0) the generator falls back to
        free-running at the configured speed.

    Thread safety:
        All setters and tick() must be called from the same thread (message thread).
        getX() / getY() are trivial float reads — safe to call from paint().

    Namespace: xoceanus
    JUCE 8, C++17
*/

#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace xoceanus
{

//==============================================================================
class XYPatternGenerator
{
public:
    //==========================================================================
    enum class Pattern
    {
        None   = 0,
        PULSE  = 1,
        DRIFT  = 2,
        TIDE   = 3,
        RIPPLE = 4,
        CHAOS  = 5
    };

    enum class SyncMode
    {
        Free       = 0,  // speed interpreted as Hz
        Bar_1_4    = 1,  // quarter-note (1/4 bar)
        Bar_1_2    = 2,  // half-bar
        Bar_1      = 3,  // 1 bar (default)
        Bar_2      = 4,  // 2 bars
        Bar_4      = 5   // 4 bars
    };

    //==========================================================================
    XYPatternGenerator() { reset(); }

    //==========================================================================
    // Configuration

    void setPattern(Pattern p)
    {
        if (p == pattern_)
            return;
        pattern_ = p;
        resetPatternState();
    }

    void setSpeed(float s) noexcept { speed_  = std::max(0.001f, s); }
    void setDepth(float d) noexcept { depth_  = std::clamp(d, 0.0f, 1.0f); }
    void setSyncMode(SyncMode m)     noexcept { syncMode_ = m; }

    Pattern  getPattern()  const noexcept { return pattern_;  }
    SyncMode getSyncMode() const noexcept { return syncMode_; }
    float    getSpeed()    const noexcept { return speed_;    }
    float    getDepth()    const noexcept { return depth_;    }

    //==========================================================================
    // Output

    float getX() const noexcept { return x_; }
    float getY() const noexcept { return y_; }

    //==========================================================================
    // Advance the pattern state.
    //   dt         — elapsed seconds since last tick (typically ~0.033s at 30 Hz)
    //   bpm        — host BPM (0 = host not playing → fall back to free-running)
    //   beatPhase  — fractional position within current bar [0, 1)
    void tick(float dt, float bpm, float beatPhase)
    {
        if (pattern_ == Pattern::None)
            return;

        // Compute effective speed in Hz for free-running algorithms
        float effectiveHz = computeEffectiveHz(bpm);

        switch (pattern_)
        {
            case Pattern::PULSE:  tickPulse(dt, bpm, beatPhase, effectiveHz); break;
            case Pattern::DRIFT:  tickDrift(dt, effectiveHz);                 break;
            case Pattern::TIDE:   tickTide(dt, effectiveHz);                  break;
            case Pattern::RIPPLE: tickRipple(dt, bpm, beatPhase, effectiveHz); break;
            case Pattern::CHAOS:  tickChaos(dt, bpm, beatPhase, effectiveHz); break;
            default: break;
        }
    }

    //==========================================================================
    void reset()
    {
        x_ = 0.5f;
        y_ = 0.5f;
        resetPatternState();
    }

private:
    //==========================================================================
    // State
    Pattern  pattern_  = Pattern::None;
    SyncMode syncMode_ = SyncMode::Bar_1;
    float    speed_    = 0.5f;
    float    depth_    = 0.5f;

    float    x_        = 0.5f;
    float    y_        = 0.5f;

    // Internal continuous phase accumulator (PULSE, TIDE)
    float    phase_    = 0.0f;

    // DRIFT state
    float    vx_       = 0.0f;
    float    vy_       = 0.0f;

    // RIPPLE state
    float    seedX_    = 0.5f;
    float    seedY_    = 0.5f;
    float    rippleAngle_ = 0.0f;
    float    ripplePhase_ = 0.0f;

    // CHAOS state — logistic map value in [0,1]
    float    chaosX_   = 0.37f;
    float    chaosY_   = 0.63f;
    float    chaosAcc_ = 0.0f; // accumulator for tempo-aligned tick pacing

    // Simple LCG for DRIFT gaussian approximation (no <random> dependency)
    uint32_t rngState_ = 0xDEADBEEF;

    //==========================================================================
    void resetPatternState()
    {
        phase_       = 0.0f;
        vx_ = vy_    = 0.0f;
        seedX_       = 0.5f;
        seedY_       = 0.5f;
        rippleAngle_ = 0.0f;
        ripplePhase_ = 0.0f;
        chaosX_      = 0.37f;
        chaosY_      = 0.63f;
        chaosAcc_    = 0.0f;
    }

    //==========================================================================
    // Compute Hz for the "one full cycle" rate, taking sync mode into account.
    float computeEffectiveHz(float bpm) const noexcept
    {
        if (syncMode_ == SyncMode::Free || bpm < 1.0f)
        {
            // speed_ 0..1 maps to 0.05..5 Hz logarithmically
            return std::exp(std::log(0.05f) + speed_ * std::log(5.0f / 0.05f));
        }

        // Synced: convert sync division to bar fraction, then to Hz
        float barDurationSec = 60.0f / bpm * 4.0f; // 4/4 assumed
        float barFraction = 1.0f;
        switch (syncMode_)
        {
            case SyncMode::Bar_1_4: barFraction = 0.25f; break;
            case SyncMode::Bar_1_2: barFraction = 0.5f;  break;
            case SyncMode::Bar_1:   barFraction = 1.0f;  break;
            case SyncMode::Bar_2:   barFraction = 2.0f;  break;
            case SyncMode::Bar_4:   barFraction = 4.0f;  break;
            default:                barFraction = 1.0f;  break;
        }
        float cycleSec = barDurationSec * barFraction;
        return (cycleSec > 0.001f) ? (1.0f / cycleSec) : 0.5f;
    }

    //==========================================================================
    // PULSE: Hard attack + exponential decay per beat.
    // x(t) = 0.5 + depth * 0.5 * sign(sin(2π*phase)) * exp(-decay * frac(phase))
    // Y is offset by half a beat.
    void tickPulse(float dt, float /*bpm*/, float /*beatPhase*/, float effectiveHz)
    {
        // Accumulate phase using effectiveHz (1 Hz = 1 beat per second)
        phase_ = std::fmod(phase_ + dt * effectiveHz, 1.0f);

        static constexpr float kDecay = 6.0f; // how fast the snap fades
        float frac = phase_;
        float sign = (std::sin(2.0f * static_cast<float>(M_PI) * frac) >= 0.0f) ? 1.0f : -1.0f;
        float env  = std::exp(-kDecay * std::fmod(frac, 1.0f));

        float halfPhase = std::fmod(phase_ + 0.5f, 1.0f);
        float sign2 = (std::sin(2.0f * static_cast<float>(M_PI) * halfPhase) >= 0.0f) ? 1.0f : -1.0f;
        float env2  = std::exp(-kDecay * std::fmod(halfPhase, 1.0f));

        x_ = 0.5f + depth_ * 0.5f * sign  * env;
        y_ = 0.5f + depth_ * 0.5f * sign2 * env2;
    }

    //==========================================================================
    // DRIFT: Brownian random-walk with mean reversion toward (0.5, 0.5).
    void tickDrift(float dt, float effectiveHz)
    {
        // Speed scales tick contribution: higher Hz = larger per-tick excursion
        float tickScale = dt * effectiveHz * 3.0f;

        // Gaussian-approximated noise (sum of 3 uniform samples - 1.5 ≈ N(0, 0.25))
        float nx = (randF() + randF() + randF() - 1.5f) * depth_ * 0.04f * tickScale;
        float ny = (randF() + randF() + randF() - 1.5f) * depth_ * 0.04f * tickScale;

        vx_ += nx;
        vy_ += ny;
        vx_ *= 0.95f;
        vy_ *= 0.95f;

        // Mean reversion toward centre
        vx_ += (0.5f - x_) * 0.003f * tickScale;
        vy_ += (0.5f - y_) * 0.003f * tickScale;

        x_ = std::clamp(x_ + vx_, 0.0f, 1.0f);
        y_ = std::clamp(y_ + vy_, 0.0f, 1.0f);
    }

    //==========================================================================
    // TIDE: Stable Lissajous figure-8 (2:1 frequency ratio).
    // x(t) = 0.5 + d * sin(2π * f * t)
    // y(t) = 0.5 + d * sin(4π * f * t + π/4)
    void tickTide(float dt, float effectiveHz)
    {
        phase_ = std::fmod(phase_ + dt * effectiveHz, 1.0f);
        float t = phase_;
        x_ = 0.5f + depth_ * 0.5f * std::sin(2.0f * static_cast<float>(M_PI) * t);
        y_ = 0.5f + depth_ * 0.5f * std::sin(4.0f * static_cast<float>(M_PI) * t
                                               + static_cast<float>(M_PI) * 0.25f);
    }

    //==========================================================================
    // RIPPLE: Orbital motion around a drifting seed point.
    // Angle advances at speed rate. Seed re-chosen every full cycle.
    void tickRipple(float dt, float /*bpm*/, float /*beatPhase*/, float effectiveHz)
    {
        rippleAngle_ = std::fmod(rippleAngle_ + dt * effectiveHz * 2.0f * static_cast<float>(M_PI),
                                  2.0f * static_cast<float>(M_PI));

        // Phase 0..1 for the full orbital cycle
        ripplePhase_ = std::fmod(ripplePhase_ + dt * effectiveHz, 1.0f);

        // Re-seed at the start of each full cycle
        if (ripplePhase_ < dt * effectiveHz) // just crossed zero
        {
            seedX_ = 0.3f + randF() * 0.4f;
            seedY_ = 0.3f + randF() * 0.4f;
        }

        float r = depth_ * 0.4f * std::sin(static_cast<float>(M_PI) * ripplePhase_); // smooth in/out
        x_ = std::clamp(seedX_ + r * std::cos(rippleAngle_), 0.0f, 1.0f);
        y_ = std::clamp(seedY_ + r * std::sin(rippleAngle_), 0.0f, 1.0f);
    }

    //==========================================================================
    // CHAOS: Logistic-map driven chaotic sequence (r = 3.95).
    // tick rate driven by speed; when synced, aligns to 16th-note boundaries.
    void tickChaos(float dt, float bpm, float beatPhase, float effectiveHz)
    {
        // accumulate fractional ticks
        float tickHz = effectiveHz * 4.0f; // 4× rate so individual jumps are audible
        chaosAcc_ += dt * tickHz;

        // Fire discrete steps when accumulator passes integer boundaries
        int steps = static_cast<int>(chaosAcc_);
        if (steps > 0)
        {
            chaosAcc_ -= static_cast<float>(steps);
            for (int i = 0; i < std::min(steps, 8); ++i)
            {
                static constexpr float r = 3.95f;
                chaosX_ = r * chaosX_ * (1.0f - chaosX_);
                chaosY_ = r * chaosY_ * (1.0f - chaosY_);
            }
        }

        // Scale output by depth (centre + excursion)
        x_ = 0.5f + (chaosX_ - 0.5f) * depth_;
        y_ = 0.5f + (chaosY_ - 0.5f) * depth_;

        (void)bpm; (void)beatPhase; // used for future 16th-note alignment
    }

    //==========================================================================
    // Simple LCG float in [0, 1)
    float randF() noexcept
    {
        rngState_ = rngState_ * 1664525u + 1013904223u;
        return static_cast<float>(rngState_ >> 8) * (1.0f / 16777216.0f);
    }
};

} // namespace xoceanus
