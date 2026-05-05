// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include "../../Core/DNAModulationBus.h"
#include "../../Core/PartnerAudioBus.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OligoChain — Frequency-Selective Ducker (FX Pack 1, Sidechain Creative)
//
// Wildcard: 4-band Linkwitz-Riley split with per-band ducking. Partner DNA
// shapes per-band release time — brightness → high band, density → lo-mid,
// aggression → low band.
//
// Parameter prefix: olig_  (13 params per Pack 1 spec §4)
//
// Routing: Stereo In → Stereo Out (4-band split applied per channel)
//
// Signal flow (per spec §4):
//   1. 4-band Linkwitz-Riley LR4 split (24 dB/oct) — three crossovers, each
//      built from two cascaded Butterworth Q=0.7071 biquad LP stages. Bands
//      derived as:
//        bandLow   = LP_low(in)
//        bandLoMid = LP_mid(in) - LP_low(in)
//        bandHiMid = LP_high(in) - LP_mid(in)
//        bandHigh  = in - LP_high(in)
//      This sums back to `in` exactly (telescoping subtraction), so the
//      recombine stage has no phase artefact at unity duck.
//   2. Per-band envelope followers run on the *key* (sidechain partner audio)
//      split through the same crossovers. One-pole AR with denormal flush.
//   3. DNA-aware release scaling — `dnaScale` blends static release ↔ DNA-
//      modulated release per band:
//        low    ← partner aggression
//        lo-mid ← partner density
//        hi-mid ← average(density, brightness)
//        high   ← partner brightness
//      The mapping is "high DNA → faster release / more aggressive ducking".
//   4. Per-band VCAs: gain = max(0, 1 - depth_band × env_band)
//   5. Recombine — sum the four ducked bands. Wet/dry blend on output.
//
// D005 — `breathRate` (≤ 0.001 Hz floor) drifts crossover frequencies by
// ±5 % via a slow LFO so the spectral split itself "breathes". The drift
// is applied at block boundaries (cheap, inaudible quantisation at the
// rates this LFO runs).
//
// Audio-thread contract:
//   - No allocation, no blocking I/O.
//   - Parameter pointers cached once via cacheParameterPointers().
//   - Filter coefficients recalculated once per block (cheap).
//   - Denormal flush in biquad output and envelope state.
//
// See: Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §4
//==============================================================================
class OligoChain
{
public:
    OligoChain() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = sampleRate;
        mixSmoothed_.reset(sampleRate, 0.02);
        recalcCrossover(lpLow_,  100.0f);
        recalcCrossover(lpMid_,  800.0f);
        recalcCrossover(lpHigh_, 4000.0f);
    }

    void reset()
    {
        mixSmoothed_.setCurrentAndTargetValue(1.0f);
        for (auto& s : sigStates_) s = {};
        for (auto& s : keyStates_) s = {};
        envLow_ = envLoMid_ = envHiMid_ = envHigh_ = 0.0f;
        breathPhase_ = 0.0f;
    }

    void setDNABus(const DNAModulationBus* bus) noexcept { dnaBus_ = bus; }
    void setPartnerAudioBus(const PartnerAudioBus* bus) noexcept { partnerBus_ = bus; }

    void processBlock(float* L, float* R, int numSamples,
                      double /*bpm*/ = 0.0, double /*ppqPosition*/ = -1.0)
    {
        if (! pLowDepth_ || ! pMix_) return;

        // ---- Block-rate parameter loads (ParamSnapshot pattern) ----
        const float lowDepth   = pLowDepth_  ->load(std::memory_order_relaxed);
        const float loMidDepth = pLoMidDepth_->load(std::memory_order_relaxed);
        const float hiMidDepth = pHiMidDepth_->load(std::memory_order_relaxed);
        const float highDepth  = pHighDepth_ ->load(std::memory_order_relaxed);
        const float atkMs      = pAttack_     ? pAttack_    ->load(std::memory_order_relaxed) : 5.0f;
        const float relMsBase  = pRelease_    ? pRelease_   ->load(std::memory_order_relaxed) : 200.0f;
        const float dnaScale   = pDnaScale_   ? pDnaScale_  ->load(std::memory_order_relaxed) : 0.0f;
        const float lowSplitHz = pLowSplit_   ? pLowSplit_  ->load(std::memory_order_relaxed) : 100.0f;
        const float midSplitHz = pMidSplit_   ? pMidSplit_  ->load(std::memory_order_relaxed) : 800.0f;
        const float highSplitHz= pHighSplit_  ? pHighSplit_ ->load(std::memory_order_relaxed) : 4000.0f;
        const int   keyEngine  = pKeyEngine_  ? clampSlot(pKeyEngine_->load(std::memory_order_relaxed)) : 0;
        const float breathRate = pBreathRate_ ? pBreathRate_->load(std::memory_order_relaxed) : 0.1f;

        mixSmoothed_.setTargetValue(pMix_->load(std::memory_order_relaxed));

        // ---- Breath LFO drift on crossovers (D005, ≤ 0.001 Hz floor) ----
        // The drift modulates all three crossover frequencies by a small ±5%,
        // so the spectral split itself breathes. Phase advances per block, not
        // per sample, since the LFO runs at sub-2 Hz.
        const float twoPi = juce::MathConstants<float>::twoPi;
        constexpr float kBreathDepth = 0.05f;
        const float breath = std::sin(breathPhase_) * kBreathDepth;
        if (sr_ > 0.0)
            breathPhase_ += twoPi * breathRate / static_cast<float>(sr_) * static_cast<float>(numSamples);
        if (breathPhase_ > twoPi) breathPhase_ -= twoPi;

        recalcCrossover(lpLow_,  lowSplitHz  * (1.0f + breath));
        recalcCrossover(lpMid_,  midSplitHz  * (1.0f + breath));
        recalcCrossover(lpHigh_, highSplitHz * (1.0f + breath));

        // ---- DNA-aware per-band release ----
        // brightness → high band release · density → lo-mid · aggression → low.
        // hi-mid uses the average of density+brightness for spectral symmetry.
        // dnaScale ∈ [0,1] blends static release ↔ DNA-modulated. At dnaScale=1,
        // a DNA value of 1.0 shrinks release to 40% of base; 0.0 expands to 160%.
        float brightness = 0.5f, density = 0.5f, aggression = 0.5f;
        if (dnaBus_ != nullptr)
        {
            brightness = dnaBus_->get(keyEngine, DNAModulationBus::Axis::Brightness);
            density    = dnaBus_->get(keyEngine, DNAModulationBus::Axis::Density);
            aggression = dnaBus_->get(keyEngine, DNAModulationBus::Axis::Aggression);
        }
        auto modulatedRelease = [&](float dnaAxis) noexcept
        {
            const float scale = 1.0f - dnaScale * (dnaAxis - 0.5f) * 1.2f;
            return relMsBase * juce::jmax(0.1f, scale);
        };
        const float relMsLow   = modulatedRelease(aggression);
        const float relMsLoMid = modulatedRelease(density);
        const float relMsHiMid = modulatedRelease((density + brightness) * 0.5f);
        const float relMsHigh  = modulatedRelease(brightness);

        const float atkCoef     = msToCoef(atkMs,    sr_);
        const float relCoefLow  = msToCoef(relMsLow, sr_);
        const float relCoefLoMid= msToCoef(relMsLoMid, sr_);
        const float relCoefHiMid= msToCoef(relMsHiMid, sr_);
        const float relCoefHigh = msToCoef(relMsHigh, sr_);

        // ---- Sidechain key (mono partner audio) ----
        const float* key = (partnerBus_ != nullptr) ? partnerBus_->getMono(keyEngine) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            const float mix = mixSmoothed_.getNextValue();

            // === Key-side: split into 4 bands and follow each envelope ===
            const float kIn = (key != nullptr) ? key[i] : 0.0f;
            const float kLowOut  = processLR4(lpLow_,  kIn, keyStates_[0]);
            const float kMidOut  = processLR4(lpMid_,  kIn, keyStates_[1]);
            const float kHighOut = processLR4(lpHigh_, kIn, keyStates_[2]);
            const float kBandLow   = kLowOut;
            const float kBandLoMid = kMidOut  - kLowOut;
            const float kBandHiMid = kHighOut - kMidOut;
            const float kBandHigh  = kIn      - kHighOut;

            envLow_   = followAR(envLow_,   std::abs(kBandLow),   atkCoef, relCoefLow);
            envLoMid_ = followAR(envLoMid_, std::abs(kBandLoMid), atkCoef, relCoefLoMid);
            envHiMid_ = followAR(envHiMid_, std::abs(kBandHiMid), atkCoef, relCoefHiMid);
            envHigh_  = followAR(envHigh_,  std::abs(kBandHigh),  atkCoef, relCoefHigh);

            // === Per-band VCAs ===
            const float gainLow   = juce::jmax(0.0f, 1.0f - lowDepth   * envLow_);
            const float gainLoMid = juce::jmax(0.0f, 1.0f - loMidDepth * envLoMid_);
            const float gainHiMid = juce::jmax(0.0f, 1.0f - hiMidDepth * envHiMid_);
            const float gainHigh  = juce::jmax(0.0f, 1.0f - highDepth  * envHigh_);

            // === Signal-side: split L into 4 bands, duck, recombine ===
            const float dryL = L[i];
            const float lLowOut  = processLR4(lpLow_,  dryL, sigStates_[0]);
            const float lMidOut  = processLR4(lpMid_,  dryL, sigStates_[1]);
            const float lHighOut = processLR4(lpHigh_, dryL, sigStates_[2]);
            const float wetL =   lLowOut                    * gainLow
                             + (lMidOut  - lLowOut)         * gainLoMid
                             + (lHighOut - lMidOut)         * gainHiMid
                             + (dryL     - lHighOut)        * gainHigh;

            // === Same for R ===
            const float dryR = R[i];
            const float rLowOut  = processLR4(lpLow_,  dryR, sigStates_[3]);
            const float rMidOut  = processLR4(lpMid_,  dryR, sigStates_[4]);
            const float rHighOut = processLR4(lpHigh_, dryR, sigStates_[5]);
            const float wetR =   rLowOut                    * gainLow
                             + (rMidOut  - rLowOut)         * gainLoMid
                             + (rHighOut - rMidOut)         * gainHiMid
                             + (dryR     - rHighOut)        * gainHigh;

            L[i] = dryL * (1.0f - mix) + wetL * mix;
            R[i] = dryR * (1.0f - mix) + wetR * mix;
        }
    }

    //--------------------------------------------------------------------------
    // APVTS integration — 13 parameters per Pack 1 spec §4.
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "olig_";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "lowDepth", 1), "OLIG Low Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "loMidDepth", 1), "OLIG Lo-Mid Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "hiMidDepth", 1), "OLIG Hi-Mid Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "highDepth", 1), "OLIG High Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "attack", 1), "OLIG Attack",
            juce::NormalisableRange<float>(0.1f, 50.0f, 0.0f, 0.5f), 5.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "release", 1), "OLIG Release",
            juce::NormalisableRange<float>(10.0f, 1000.0f, 0.0f, 0.5f), 200.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "dnaScale", 1), "OLIG DNA Scale",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "lowSplit", 1), "OLIG Low Split",
            juce::NormalisableRange<float>(40.0f, 200.0f, 0.0f, 0.5f), 100.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "midSplit", 1), "OLIG Mid Split",
            juce::NormalisableRange<float>(200.0f, 2000.0f, 0.0f, 0.5f), 800.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "highSplit", 1), "OLIG High Split",
            juce::NormalisableRange<float>(2000.0f, 10000.0f, 0.0f, 0.5f), 4000.0f));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "keyEngine", 1), "OLIG Key Engine", 0, 3, 0));
        // D005 floor: 0.001 Hz, drift on crossovers.
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "breathRate", 1), "OLIG Breath Rate",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.1f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "mix", 1), "OLIG Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
    }

    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "olig_";
        pLowDepth_   = apvts.getRawParameterValue(p + "lowDepth");
        pLoMidDepth_ = apvts.getRawParameterValue(p + "loMidDepth");
        pHiMidDepth_ = apvts.getRawParameterValue(p + "hiMidDepth");
        pHighDepth_  = apvts.getRawParameterValue(p + "highDepth");
        pAttack_     = apvts.getRawParameterValue(p + "attack");
        pRelease_    = apvts.getRawParameterValue(p + "release");
        pDnaScale_   = apvts.getRawParameterValue(p + "dnaScale");
        pLowSplit_   = apvts.getRawParameterValue(p + "lowSplit");
        pMidSplit_   = apvts.getRawParameterValue(p + "midSplit");
        pHighSplit_  = apvts.getRawParameterValue(p + "highSplit");
        pKeyEngine_  = apvts.getRawParameterValue(p + "keyEngine");
        pBreathRate_ = apvts.getRawParameterValue(p + "breathRate");
        pMix_        = apvts.getRawParameterValue(p + "mix");
    }

private:
    static int clampSlot(float v) noexcept
    {
        const int n = static_cast<int>(v + 0.5f);
        return n < 0 ? 0 : (n > 3 ? 3 : n);
    }

    // 2nd-order Butterworth biquad — Q=0.7071 = 12 dB/oct LP. Cascading two
    // stages (`LR4State` below) gives 24 dB/oct = a true Linkwitz-Riley LR4
    // crossover, which is what the spec §4 "Linkwitz-Riley split" promises.
    struct Biquad
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        struct State { float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f; };

        // const-callable: coefficients are immutable per block; state is the
        // mutating reference parameter. Denormal-flushed output.
        float process(float in, State& s) const noexcept
        {
            float out = b0 * in + b1 * s.x1 + b2 * s.x2 - a1 * s.y1 - a2 * s.y2;
            // Denormal flush — extremely small values pin to zero so subnormals
            // don't accumulate in the y feedback chain.
            constexpr float kDenormThreshold = 1.0e-30f;
            if (std::abs(out) < kDenormThreshold) out = 0.0f;
            s.x2 = s.x1; s.x1 = in;
            s.y2 = s.y1; s.y1 = out;
            return out;
        }
    };

    // LR4 cascaded state — two Butterworth biquad stages in series.
    // Each crossover (low/mid/high) needs one LR4State per channel.
    struct LR4State { Biquad::State s1, s2; };

    static float processLR4(const Biquad& b, float in, LR4State& s) noexcept
    {
        return b.process(b.process(in, s.s1), s.s2);
    }

    static float msToCoef(float ms, double sr) noexcept
    {
        if (sr <= 0.0 || ms <= 0.0f) return 0.0f;
        return std::exp(-1.0f / (ms * 0.001f * static_cast<float>(sr)));
    }

    // One-pole AR follower with denormal flush. Both branches track toward
    // the current `input`: rising → attack coef, falling → release coef.
    // Earlier draft used `rel * prev` on the release branch, which decayed
    // toward zero even when the input was holding at a non-zero value
    // (chatter at steady state). Caught by review — fixed to the standard
    // one-pole form.
    static float followAR(float prev, float input, float atk, float rel) noexcept
    {
        const float coef = (input > prev) ? atk : rel;
        const float next = coef * prev + (1.0f - coef) * input;
        constexpr float kDenormThreshold = 1.0e-30f;
        return std::abs(next) < kDenormThreshold ? 0.0f : next;
    }

    void recalcCrossover(Biquad& b, float freqHz) noexcept
    {
        if (sr_ <= 0.0 || freqHz <= 0.0f) return;
        const float w0 = 2.0f * juce::MathConstants<float>::pi
                        * freqHz / static_cast<float>(sr_);
        const float cosW0 = std::cos(w0);
        const float sinW0 = std::sin(w0);
        constexpr float kButterQ = 0.7071068f;
        const float alpha = sinW0 / (2.0f * kButterQ);
        const float a0    = 1.0f + alpha;
        b.b0 = ((1.0f - cosW0) * 0.5f) / a0;
        b.b1 =  (1.0f - cosW0)         / a0;
        b.b2 = b.b0;
        b.a1 = (-2.0f * cosW0)         / a0;
        b.a2 = (1.0f - alpha)          / a0;
    }

    double sr_ = 0.0;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed_;

    // Three crossover LP biquads (low / mid / high). Coefficients shared
    // across L / R / key (state per instance).
    Biquad lpLow_, lpMid_, lpHigh_;

    // LR4 = cascaded biquad pair, so each "state" is a LR4State (two stages).
    // 6 signal states (3 LPs × 2 channels) + 3 key states (mono key).
    std::array<LR4State, 6> sigStates_{};
    std::array<LR4State, 3> keyStates_{};

    // Per-band envelope state (mono — derived from the mono key).
    float envLow_ = 0.0f, envLoMid_ = 0.0f, envHiMid_ = 0.0f, envHigh_ = 0.0f;

    // Breath LFO phase — drifts crossover frequencies (D005 floor on rate).
    float breathPhase_ = 0.0f;

    // Cached APVTS pointers (13 params). Set once on message thread via
    // cacheParameterPointers(); read lock-free per block.
    std::atomic<float>* pLowDepth_   = nullptr;
    std::atomic<float>* pLoMidDepth_ = nullptr;
    std::atomic<float>* pHiMidDepth_ = nullptr;
    std::atomic<float>* pHighDepth_  = nullptr;
    std::atomic<float>* pAttack_     = nullptr;
    std::atomic<float>* pRelease_    = nullptr;
    std::atomic<float>* pDnaScale_   = nullptr;
    std::atomic<float>* pLowSplit_   = nullptr;
    std::atomic<float>* pMidSplit_   = nullptr;
    std::atomic<float>* pHighSplit_  = nullptr;
    std::atomic<float>* pKeyEngine_  = nullptr;
    std::atomic<float>* pBreathRate_ = nullptr;
    std::atomic<float>* pMix_        = nullptr;

    // Set once on message thread before audio starts; read lock-free on the
    // audio thread (single-writer / single-reader before-after pattern).
    const DNAModulationBus* dnaBus_     = nullptr;
    const PartnerAudioBus*  partnerBus_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OligoChain)
};

} // namespace xoceanus
