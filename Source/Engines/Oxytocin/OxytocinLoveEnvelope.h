// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>

/// LoveEnvelope — three distinct envelope shapes for Passion, Intimacy,
/// and Commitment, each reflecting the psychological quality of that love
/// component.
///
/// All envelopes share the same state machine but use different shapes and
/// time constants derived from the matching ParamSnapshot rates.
///
/// P1-1: exp() coefficients are cached at block rate via updateCoefficients().
///       Never computed per-sample — avoids iOS-class audio thread overruns
///       (same OPERA P0 lesson applied to envelopes).
///
/// PERF-2: invSr is cached in prepare(), not recomputed per sample.

enum class LoveEnvelopeType
{
    Passion,
    Intimacy,
    Commitment
};

class LoveEnvelope
{
public:
    enum class Stage
    {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release
    };

    LoveEnvelope() = default;

    void prepare(double sampleRate) noexcept
    {
        jassert(sampleRate > 0.0); // P1-7
        sr = sampleRate;
        invSr = (sampleRate > 0.0) ? static_cast<float>(1.0 / sampleRate) : 0.0f;
        // Force recompute on first updateCoefficients() call
        lastPassionRate = -1.0f;
        lastWarmthRate = -1.0f;
        lastCommitRate = -1.0f;
        reset();
    }

    void reset() noexcept
    {
        stage = Stage::Idle;
        value = 0.0f;
        phase = 0.0f;
    }

    void noteOn() noexcept
    {
        stage = Stage::Attack;
        phase = 0.0f;
    }
    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
        {
            stage = Stage::Release;
            phase = 0.0f;
            releaseStart = value;
        }
    }

    /// E5: scale the passion attack rate by a velocity-derived factor.
    /// velScale = 0.5 at pp, 1.0 at ff  → higher velocity = faster attack.
    /// Applied inside tickPassion() by multiplying the runtime attack time.
    void setAttackScale(float s) noexcept { attackScale = std::max(0.1f, s); }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    float getValue() const noexcept { return value; }

    /// P1-1: Cache all exp() coefficients for this block.
    /// Called once per block before the sample loop in OxytocinVoice::processBlock().
    void updateCoefficients(float passionRate, float warmthRate, float commitRate) noexcept
    {
        // Passion coefficients
        if (passionRate != lastPassionRate)
        {
            float attackTime = std::max(0.0001f, passionRate);
            float decayTime = attackTime * 8.0f;
            float releaseTime = attackTime * 4.0f;
            cachedPassionDecayCoeff = std::exp(-invSr / std::max(0.0001f, decayTime));
            cachedPassionReleaseCoeff = std::exp(-invSr / std::max(0.0001f, releaseTime));
            lastPassionRate = passionRate;
        }

        // Intimacy coefficients
        if (warmthRate != lastWarmthRate)
        {
            float attackTime = std::max(0.01f, warmthRate);
            float releaseTime = attackTime * 2.0f;
            // Sigmoid attack uses phase advancement — no exp needed in attack stage
            cachedIntimacyReleaseCoeff = std::exp(-invSr / std::max(0.0001f, releaseTime));
            // Sigmoid exp is called per-sample in the attack stage; it is a single
            // exp call per sample only during attack phase (which is relatively short).
            // Cache the phase increment instead:
            cachedIntimacyPhaseInc = invSr / attackTime;
            lastWarmthRate = warmthRate;
        }

        // Commitment coefficients
        if (commitRate != lastCommitRate)
        {
            float attackTime = std::max(0.05f, commitRate);
            float releaseTime = attackTime * 10.0f;
            cachedCommitReleaseCoeff = std::exp(-invSr / std::max(0.001f, releaseTime));
            cachedCommitPhaseInc = (0.95f * invSr) / attackTime; // linear ramp increment
            lastCommitRate = commitRate;
        }
    }

    /// Call once per sample.  Returns current envelope value in [0..1].
    float tick(LoveEnvelopeType type,
               float passionRate, // seconds
               float warmthRate,  // seconds
               float commitRate)  // seconds
    {
        // NOTE: invSr is already cached as a member (PERF-2).
        // exp() coefficients are pre-computed in updateCoefficients() (P1-1).

        switch (type)
        {
        case LoveEnvelopeType::Passion:
            tickPassion(passionRate);
            break;
        case LoveEnvelopeType::Intimacy:
            tickIntimacy(warmthRate);
            break;
        case LoveEnvelopeType::Commitment:
            tickCommitment(commitRate);
            break;
        }

        return value;
    }

private:
    // ------------------------------------------------------------------
    // PASSION: exponential rise, fast decay, low sustain
    void tickPassion(float pRate)
    {
        // E5: attackScale is set once per block from velocity (0.5 pp → 1.0 ff).
        // Higher velocity compresses the attack time → more urgent passion onset.
        const float attackTime = std::max(0.0001f, pRate) / std::max(0.1f, attackScale);
        const float sustainLevel = 0.15f;

        switch (stage)
        {
        case Stage::Idle:
            value = 0.0f;
            break;

        case Stage::Attack:
        {
            // Exponential rise approximation
            float attackRate = invSr / attackTime;
            value += (1.0f - value) * std::min(1.0f, attackRate * 5.0f);
            phase += invSr;
            if (phase >= attackTime || value >= 0.999f)
            {
                value = 1.0f;
                phase = 0.0f;
                stage = Stage::Decay;
            }
            break;
        }

        case Stage::Decay:
        {
            // P1-1: use cached coefficient
            value = sustainLevel + (value - sustainLevel) * cachedPassionDecayCoeff;
            phase += invSr;
            if (std::abs(value - sustainLevel) < 0.001f)
            {
                value = sustainLevel;
                stage = Stage::Sustain;
            }
            break;
        }

        case Stage::Sustain:
            value = sustainLevel;
            break;

        case Stage::Release:
        {
            // P1-1: use cached coefficient
            value = value * cachedPassionReleaseCoeff;
            if (value < 0.0001f)
            {
                value = 0.0f;
                stage = Stage::Idle;
            }
            break;
        }
        }
    }

    // ------------------------------------------------------------------
    // INTIMACY: sigmoid attack, high sustain (0.9), slow release
    void tickIntimacy(float /*wRate*/)
    {
        const float sustainLevel = 0.9f;

        switch (stage)
        {
        case Stage::Idle:
            value = 0.0f;
            break;

        case Stage::Attack:
        {
            // Use cached phase increment
            phase += cachedIntimacyPhaseInc;
            if (phase >= 1.0f)
                phase = 1.0f;

            // Sigmoid: 1 / (1 + exp(-6*(t - 0.5)))
            // This exp() runs only during attack, not sustained — acceptable cost.
            float sigmoid = 1.0f / (1.0f + std::exp(-6.0f * (phase - 0.5f)));
            value = sigmoid * sustainLevel;

            if (phase >= 1.0f)
            {
                value = sustainLevel;
                stage = Stage::Sustain;
                phase = 0.0f;
            }
            break;
        }

        case Stage::Decay:
            // Intimacy has no distinct decay phase — goes straight to sustain
            value = sustainLevel;
            stage = Stage::Sustain;
            break;

        case Stage::Sustain:
            value = sustainLevel;
            break;

        case Stage::Release:
        {
            // P1-1: use cached coefficient
            value = value * cachedIntimacyReleaseCoeff;
            if (value < 0.0001f)
            {
                value = 0.0f;
                stage = Stage::Idle;
            }
            break;
        }
        }
    }

    // ------------------------------------------------------------------
    // COMMITMENT: slow linear ramp, sustain 0.95, very slow release
    void tickCommitment(float /*cRate*/)
    {
        const float sustainLevel = 0.95f;

        switch (stage)
        {
        case Stage::Idle:
            value = 0.0f;
            break;

        case Stage::Attack:
        {
            // P1-1: use cached linear increment
            value += cachedCommitPhaseInc;
            if (value >= sustainLevel)
            {
                value = sustainLevel;
                stage = Stage::Sustain;
                phase = 0.0f;
            }
            break;
        }

        case Stage::Decay:
            value = sustainLevel;
            stage = Stage::Sustain;
            break;

        case Stage::Sustain:
            value = sustainLevel;
            break;

        case Stage::Release:
        {
            // P1-1: use cached coefficient
            value = value * cachedCommitReleaseCoeff;
            if (value < 0.0001f)
            {
                value = 0.0f;
                stage = Stage::Idle;
            }
            break;
        }
        }
    }

    Stage stage = Stage::Idle;
    float value = 0.0f;
    float phase = 0.0f;
    float releaseStart = 0.0f;
    float attackScale = 1.0f; // E5: velocity → passion attack speed (1.0 = unscaled)
    double sr = 0.0;          // P1-7: default 0
    float invSr = 0.0f;       // PERF-2: cached 1/sr

    // P1-1: block-rate cached envelope coefficients
    float lastPassionRate = -1.0f;
    float lastWarmthRate = -1.0f;
    float lastCommitRate = -1.0f;
    float cachedPassionDecayCoeff = 0.0f;
    float cachedPassionReleaseCoeff = 0.0f;
    float cachedIntimacyReleaseCoeff = 0.0f;
    float cachedIntimacyPhaseInc = 0.0f;
    float cachedCommitReleaseCoeff = 0.0f;
    float cachedCommitPhaseInc = 0.0f;
};
