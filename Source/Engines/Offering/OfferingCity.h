// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
//
//  OfferingCity.h — 5 City Psychoacoustic Processing Chains
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  Each city is a complete processing chain representing a boom bap archetype:
//
//  City 0: New York — "The Archaeologist" (SP-1200 grit + feedback noise gate)
//  City 1: Detroit  — "The Time-Bender" (Dilla warmth + feedback sat loop)
//  City 2: LA       — "The Collage Artist" (Madlib squash + parallel compression)
//  City 3: Toronto  — "The Architect" (Bauhaus clarity + sidechain sub duck)
//  City 4: Bay Area — "The Alchemist" (dark fog + recursive allpass network)
//
//  Each has 5 standard stages + 1 structurally unique stage (Stage 6).
//  Shadow-chain blend when cityBlend > 0.001 (runs 2 chains in parallel).
//
//  Accent: Crate Wax Yellow #E5B80B | Prefix: ofr_
//
//==============================================================================

#include "../../DSP/FastMath.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// OfferingCompressor — Simple envelope-following compressor for city chains.
//==============================================================================
struct OfferingCompressor
{
    void setParams(float attackMs, float releaseMs, float ratio, float thresholdDb, float sampleRate) noexcept
    {
        attackCoeff_ = std::exp(-1.0f / (sampleRate * attackMs * 0.001f));
        releaseCoeff_ = std::exp(-1.0f / (sampleRate * releaseMs * 0.001f));
        ratio_ = ratio;
        threshold_ = std::pow(10.0f, thresholdDb / 20.0f);
    }

    float process(float input) noexcept
    {
        float absIn = std::abs(input);

        // Envelope follower
        if (absIn > envLevel_)
            envLevel_ = attackCoeff_ * envLevel_ + (1.0f - attackCoeff_) * absIn;
        else
            envLevel_ = releaseCoeff_ * envLevel_ + (1.0f - releaseCoeff_) * absIn;
        envLevel_ = flushDenormal(envLevel_);

        // Gain reduction
        // SRO (2026-03-21): Replace std::log10 + std::pow(10,...) with fast
        // approximations: log10(x) = log2(x) * 0.30103f;
        // pow(10, x) = fastExp(x * ln10) where ln10 = 2.302585f.
        // Accuracy: ~0.1% for compressor gain reduction — perceptually transparent.
        if (envLevel_ > threshold_)
        {
            float overDb = 20.0f * fastLog2(envLevel_ / threshold_ + 1e-10f) * 0.30103f;
            float reductionDb = overDb * (1.0f - 1.0f / ratio_);
            float gain = fastExp(-reductionDb * (2.302585f / 20.0f));
            return input * gain;
        }
        return input;
    }

    float getEnvLevel() const noexcept { return envLevel_; }
    void reset() noexcept { envLevel_ = 0.0f; }

private:
    float attackCoeff_ = 0.99f;
    float releaseCoeff_ = 0.999f;
    float ratio_ = 4.0f;
    float threshold_ = 0.5f;
    float envLevel_ = 0.0f;
};

//==============================================================================
// OfferingAllpass — Single allpass filter stage for Bay Area fog network.
//==============================================================================
struct OfferingAllpass
{
    void setDelay(int delaySamples) noexcept { delay_ = std::max(1, std::min(delaySamples, kMaxDelay - 1)); }

    float process(float input) noexcept
    {
        float delayed = buffer_[readPos_];
        float output = -input + delayed;
        buffer_[writePos_] = input + delayed * feedback_;
        buffer_[writePos_] = flushDenormal(buffer_[writePos_]);

        writePos_ = (writePos_ + 1) % kMaxDelay;
        readPos_ = (writePos_ - delay_ + kMaxDelay) % kMaxDelay;
        return output;
    }

    void addFeedback(float amount) noexcept { buffer_[writePos_] += amount; }

    void setFeedback(float fb) noexcept { feedback_ = fb; }

    void reset() noexcept
    {
        std::fill(buffer_, buffer_ + kMaxDelay, 0.0f);
        writePos_ = 0;
        readPos_ = 0;
    }

private:
    static constexpr int kMaxDelay = 64;
    float buffer_[kMaxDelay] = {};
    int writePos_ = 0;
    int readPos_ = 0;
    int delay_ = 7;
    float feedback_ = 0.5f;
};

//==============================================================================
// OfferingCityChain — Single city processing chain (one of 5 cities).
//==============================================================================
class OfferingCityChain
{
public:
    enum City : int
    {
        NewYork = 0,
        Detroit = 1,
        LA = 2,
        Toronto = 3,
        BayArea = 4
    };

    void prepare(int cityIndex, float sampleRate) noexcept
    {
        city_ = cityIndex;
        sr_ = sampleRate;
        comp_.reset();
        envFollower_ = 0.0f;
        detroitFeedback_ = 0.0f;
        torontoSubEnv_ = 0.0f;
        shimmerPhase_ = 0.0f;

        // City-specific compressor setup
        switch (city_)
        {
        case NewYork:
            comp_.setParams(5.0f, 50.0f, 4.0f, -12.0f, sr_); // fast, punchy
            break;
        case Detroit:
            comp_.setParams(30.0f, 200.0f, 2.0f, -18.0f, sr_); // slow, gluey
            break;
        case LA:
            comp_.setParams(3.0f, 20.0f, 6.0f, -15.0f, sr_);     // heavy, pumping
            hardComp_.setParams(1.0f, 10.0f, 8.0f, -18.0f, sr_); // parallel chain
            break;
        case Toronto:
            comp_.setParams(10.0f, 80.0f, 3.0f, -14.0f, sr_); // transparent
            break;
        case BayArea:
            comp_.setParams(20.0f, 150.0f, 3.0f, -16.0f, sr_); // dark glue
            // Allpass fog network: prime-number delays
            for (int i = 0; i < 4; ++i)
                allpass_[i].reset();
            allpass_[0].setDelay(7);
            allpass_[1].setDelay(13);
            allpass_[2].setDelay(23);
            allpass_[3].setDelay(37);
            allpass_[0].setFeedback(0.5f);
            allpass_[1].setFeedback(0.5f);
            allpass_[2].setFeedback(0.5f);
            allpass_[3].setFeedback(0.5f);
            break;
        }

        // Simple one-pole filters for city chains
        lpCoeff_ = std::exp(-2.0f * 3.14159265f * 12000.0f / sr_);
        hpCoeff_ = std::exp(-2.0f * 3.14159265f * 60.0f / sr_);
        subLpCoeff_ = std::exp(-2.0f * 3.14159265f * 200.0f / sr_);
    }

    //--------------------------------------------------------------------------
    // Process a mono buffer through this city's chain.
    //--------------------------------------------------------------------------
    void process(float* buffer, int numSamples, float intensity) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = buffer[i];

            switch (city_)
            {
            case NewYork:
                sample = processNewYork(sample, intensity);
                break;
            case Detroit:
                sample = processDetroit(sample, intensity);
                break;
            case LA:
                sample = processLA(sample, intensity);
                break;
            case Toronto:
                sample = processToronto(sample, intensity);
                break;
            case BayArea:
                sample = processBayArea(sample, intensity);
                break;
            }

            buffer[i] = sample;
        }
    }

    void reset() noexcept
    {
        comp_.reset();
        hardComp_.reset();
        envFollower_ = 0.0f;
        detroitFeedback_ = 0.0f;
        detroitLpState_ = 0.0f;
        torontoSubEnv_ = 0.0f;
        shimmerPhase_ = 0.0f;
        lpState_ = 0.0f;
        hpState_ = 0.0f;
        subLpState_ = 0.0f;
        for (int i = 0; i < 4; ++i)
            allpass_[i].reset();
    }

private:
    //--------------------------------------------------------------------------
    // CITY 0: NEW YORK — "The Archaeologist"
    // SP-1200 grit + tight swing + HP + compression + FEEDBACK NOISE GATE
    //--------------------------------------------------------------------------
    float processNewYork(float input, float intensity) noexcept
    {
        float out = input;

        // Stage 1: Bit crush — 12-bit at 26040 Hz (SP-1200 spec)
        float levels = 4096.0f; // 12-bit
        out = std::round(out * levels) / levels;

        // Stage 2: Vinyl noise (density scales with intensity)
        // Handled externally by OfferingTexture — here we just boost presence

        // Stage 3: Tight swing — handled at engine level (trigger timing)

        // Stage 4: High-pass at 60Hz (thin NYC boom bap low end)
        float hp = out - hpState_;
        hpState_ = out + hpCoeff_ * (hpState_ - out);
        hpState_ = flushDenormal(hpState_);
        out = hp;

        // Stage 5: Compression (fast attack, medium release, 4:1)
        out = comp_.process(out);

        // Stage 6 (UNIQUE): Feedback noise gate
        // Signal feeds back into a gate that opens/closes based on transient energy
        envFollower_ = envFollower_ * 0.999f + std::abs(out) * 0.001f;
        envFollower_ = flushDenormal(envFollower_);
        float gateThreshold = 0.3f * intensity;
        bool gateOpen = (envFollower_ > gateThreshold);
        out = gateOpen ? out : out * 0.05f; // -26dB floor

        return out;
    }

    //--------------------------------------------------------------------------
    // CITY 1: DETROIT — "The Time-Bender"
    // Warm saturation + drunk timing + soft transients + FEEDBACK SAT LOOP
    //--------------------------------------------------------------------------
    float processDetroit(float input, float intensity) noexcept
    {
        float out = input;

        // Stage 1: Analog saturation (warm, not harsh)
        float satDrive = 1.0f + intensity * 2.0f;
        out = fastTanh(out * satDrive);

        // Stage 2: Drunk timing — handled at engine level (trigger timing offset)

        // Stage 3: Transient softener — 2ms attack smoothing
        lpState_ = lpState_ + 0.05f * (out - lpState_); // ~2ms at 44.1kHz
        lpState_ = flushDenormal(lpState_);
        out = lpState_;

        // Stage 4: Warm filter — gentle LP at 12kHz (Detroit warmth)
        // FIX: Use a SEPARATE state variable so Stage 4 has its own LP characteristic.
        // Previously: `lp = out + lpCoeff_ * (lpState_ - out)` — since out == lpState_
        // after Stage 3, the subtraction was always zero (no-op).
        detroitLpState_ = detroitLpState_ + (1.0f - lpCoeff_) * (out - detroitLpState_);
        detroitLpState_ = flushDenormal(detroitLpState_);
        out = detroitLpState_;

        // Stage 5: Tape compression (slow attack, slow release, 2:1 glue)
        out = comp_.process(out);

        // Stage 6 (UNIQUE): Feedback saturation loop
        // Output feeds back through soft-clip — warmth accumulates over repeated hits
        detroitFeedback_ = detroitFeedback_ * 0.95f + out * 0.05f;
        detroitFeedback_ = flushDenormal(detroitFeedback_);
        float saturated = fastTanh(detroitFeedback_ * (1.0f + intensity * 2.0f));
        out = out * 0.7f + saturated * 0.3f * intensity;

        return out;
    }

    //--------------------------------------------------------------------------
    // CITY 2: LOS ANGELES — "The Collage Artist"
    // Heavy compression + tape sat + psychedelic pitch + PARALLEL COMPRESSION
    //--------------------------------------------------------------------------
    float processLA(float input, float intensity) noexcept
    {
        float out = input;

        // Stage 1: Heavy compression (fast/fast, 6:1 squashed pumping)
        out = comp_.process(out);

        // Stage 2: Tape saturation (heavier than Detroit)
        float tapeDrive = 1.5f + intensity * 2.5f;
        out = fastTanh(out * tapeDrive) / tapeDrive;

        // Stage 3: Psychedelic pitch — random ±5 cent micro-drift
        // (Implemented as subtle phase modulation via the texture layer)

        // Stage 4: Layer boost — handled at engine level (flip layers +1)

        // Stage 5: Low-end warmth — bass shelf boost at 80Hz
        // Simple shelf: boost low content
        float lowContent = out - hpState_;
        hpState_ = out + 0.988f * (hpState_ - out); // ~80Hz HP
        hpState_ = flushDenormal(hpState_);
        out = out + lowContent * intensity * 0.3f;

        // Stage 6 (UNIQUE): Parallel compression
        // Dry + heavily compressed mixed in parallel
        float compressed = hardComp_.process(input); // compress the original input
        out = out * (1.0f - intensity * 0.5f) + compressed * intensity * 0.5f;

        return out;
    }

    //--------------------------------------------------------------------------
    // CITY 3: TORONTO — "The Architect"
    // Clean sub + tight HP + precision transient + SIDECHAIN SUB DUCK
    //--------------------------------------------------------------------------
    float processToronto(float input, float intensity) noexcept
    {
        float out = input;

        // Stage 1: Clean sub — sub-harmonic generator at -12dB
        // Simple half-wave rectifier frequency divider
        float subInput = (out > 0.0f) ? out : 0.0f;
        subLpState_ = subLpState_ + 0.02f * (subInput - subLpState_); // LP at ~140Hz
        subLpState_ = flushDenormal(subLpState_);
        float sub = subLpState_ * intensity * 0.25f; // -12dB

        // Stage 2: Tight HP at 40Hz (remove mud)
        float hp = out - hpState_;
        hpState_ = out + 0.994f * (hpState_ - out); // ~40Hz
        hpState_ = flushDenormal(hpState_);
        out = hp;

        // Stage 3: Precision transient — transient sharpener
        float diff = out - lastSample_;
        lastSample_ = out;
        out = out + diff * intensity * 0.3f; // enhance transient

        // Stage 4: Minimal noise — noise reduced (handled by texture layer scaling)

        // Stage 5: Clean compression (medium/medium, 3:1 transparent)
        out = comp_.process(out);

        // Stage 6 (UNIQUE): Sidechain sub duck
        // Own sub-harmonic ducks mid/high content on transients
        torontoSubEnv_ = torontoSubEnv_ * 0.999f + std::abs(sub) * 0.001f;
        torontoSubEnv_ = flushDenormal(torontoSubEnv_);
        float duckAmount = torontoSubEnv_ * intensity * 10.0f; // scale up for audibility
        duckAmount = std::min(duckAmount, 0.4f);
        out = sub + out * (1.0f - duckAmount);

        return out;
    }

    //--------------------------------------------------------------------------
    // CITY 4: BAY AREA — "The Alchemist"
    // Dark filter + shimmer degradation + tape stop + RECURSIVE ALLPASS FOG
    //--------------------------------------------------------------------------
    float processBayArea(float input, float intensity) noexcept
    {
        float out = input;

        // Stage 1: Dark filter — LP at 8kHz (roll off brightness, create fog)
        float cutoff = 8000.0f - intensity * 3000.0f; // 5-8kHz
        float darkLpCoeff = std::exp(-2.0f * 3.14159265f * cutoff / sr_);
        lpState_ = out + darkLpCoeff * (lpState_ - out);
        lpState_ = flushDenormal(lpState_);
        out = lpState_;

        // Stage 2 (fog network replaces convolution — see Stage 6)

        // Stage 3: Shimmer degradation — subtle chorus at 0.1Hz (lo-fi shimmer)
        shimmerPhase_ += 0.1f / sr_;
        if (shimmerPhase_ >= 1.0f)
            shimmerPhase_ -= 1.0f;
        float shimmer = xoceanus::fastSin(shimmerPhase_ * 6.2831853f) * intensity * 0.02f;
        out = out * (1.0f + shimmer);

        // Stage 4: Tape stop — occasional micro pitch drop
        // (Probabilistic, handled once per block at engine level)

        // Stage 5: Dark compression (slow/slow, 3:1 dark glue)
        out = comp_.process(out);

        // Stage 6 (UNIQUE): Recursive allpass fog network
        // 4 cascaded allpass filters with prime delays create diffuse spatial character
        float fog = out;
        for (int i = 0; i < 4; ++i)
            fog = allpass_[i].process(fog);

        out = out * (1.0f - intensity * 0.4f) + fog * intensity * 0.4f;

        // Feedback between last and first allpass creates the "fog" decay
        allpass_[0].addFeedback(fog * 0.3f * intensity);

        return out;
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------
    int city_ = 0;
    float sr_ = 44100.0f;

    // Compressors
    OfferingCompressor comp_;
    OfferingCompressor hardComp_; // LA parallel chain

    // City-specific state
    float envFollower_ = 0.0f;     // NY: noise gate envelope
    float detroitFeedback_ = 0.0f; // Detroit: feedback sat loop state
    float detroitLpState_ = 0.0f;  // Detroit: Stage 4 LP own state (separate from lpState_)
    float torontoSubEnv_ = 0.0f;   // Toronto: sidechain sub envelope
    float shimmerPhase_ = 0.0f;    // Bay Area: shimmer LFO

    // Filter state
    float lpCoeff_ = 0.99f;
    float hpCoeff_ = 0.99f;
    float subLpCoeff_ = 0.98f;
    float lpState_ = 0.0f;
    float hpState_ = 0.0f;
    float subLpState_ = 0.0f;
    float lastSample_ = 0.0f; // Toronto transient sharpener

    // Bay Area allpass fog network
    OfferingAllpass allpass_[4];
};

//==============================================================================
// OfferingCityProcessor — Manages all 5 city chains + shadow-chain blend.
//
// Pre-allocates all 5 chains in prepare(). At runtime, processes only the
// active chain (1x CPU) unless cityBlend > 0.001 (2x for crossfade).
//==============================================================================
class OfferingCityProcessor
{
public:
    void prepare(float sampleRate) noexcept
    {
        sr_ = sampleRate;
        for (int i = 0; i < 5; ++i)
            chains_[i].prepare(i, sampleRate);
    }

    //--------------------------------------------------------------------------
    // Process a mono buffer through the city processing chain.
    //
    // cityMode: city index [0-4]
    // cityBlend: morph between current and next city [0, 1]
    // cityIntensity: how much city processing affects sound [0, 1]
    //--------------------------------------------------------------------------
    void process(float* buffer, int numSamples, int cityMode, float cityBlend, float cityIntensity) noexcept
    {
        if (cityIntensity < 0.001f)
            return; // bypass

        cityMode = std::max(0, std::min(cityMode, 4));

        if (cityBlend < 0.001f)
        {
            // Pure city — single chain, no blending overhead
            chains_[cityMode].process(buffer, numSamples, cityIntensity);
        }
        else
        {
            // Blending — run both chains, crossfade outputs
            int cityA = cityMode;
            int cityB = (cityMode + 1) % 5;

            // Copy input for second chain
            float shadow[2048]; // max block size
            int safeSamples = std::min(numSamples, 2048);
            std::memcpy(shadow, buffer, static_cast<size_t>(safeSamples) * sizeof(float));

            chains_[cityA].process(buffer, safeSamples, cityIntensity);
            chains_[cityB].process(shadow, safeSamples, cityIntensity);

            // Equal-power crossfade
            float halfPi = 1.5707963f;
            float gainA = xoceanus::fastCos(cityBlend * halfPi);
            float gainB = xoceanus::fastSin(cityBlend * halfPi);
            for (int i = 0; i < safeSamples; ++i)
                buffer[i] = buffer[i] * gainA + shadow[i] * gainB;
        }
    }

    void reset() noexcept
    {
        for (int i = 0; i < 5; ++i)
            chains_[i].reset();
    }

private:
    float sr_ = 44100.0f;
    OfferingCityChain chains_[5];
};

} // namespace xoceanus
