// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include "../../Core/DNAModulationBus.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>

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
// Phase 0 status: SCAFFOLD ONLY.
//   - Class structure, parameters, DNA bus consumption hook are complete.
//   - DSP is a placeholder pass-through with summed-depth-modulated gain.
//   - Real LR4 split + per-band VCAs + DNA-aware release scaling lands in
//     Pack 1 implementation.
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
        depthSumSmoothed_.reset(sampleRate, 0.02);
        mixSmoothed_.reset(sampleRate, 0.02);
    }

    void reset()
    {
        depthSumSmoothed_.setCurrentAndTargetValue(0.0f);
        mixSmoothed_.setCurrentAndTargetValue(1.0f);
    }

    // Inject DNA bus pointer (set once on message thread before audio starts).
    // Pack 1 implementation wires this into the dnaScale parameter — partner
    // DNA scales per-band release time (brightness/density/aggression axes).
    void setDNABus(const DNAModulationBus* bus) noexcept { dnaBus_ = bus; }

    // Stereo in, stereo out. Pack 1 will replace this with a 4-band LR4
    // split, per-band envelope followers, and DNA-aware release-scaled VCAs.
    // For now applies a single-band gain reduction proportional to the sum
    // of the 4 depth params, demonstrating parameter wiring.
    void processBlock(float* L, float* R, int numSamples,
                      double /*bpm*/ = 0.0, double /*ppqPosition*/ = -1.0)
    {
        if (! pLowDepth_ || ! pLoMidDepth_ || ! pHiMidDepth_ || ! pHighDepth_ || ! pMix_)
            return;

        const float depthSum = pLowDepth_->load(std::memory_order_relaxed)
                             + pLoMidDepth_->load(std::memory_order_relaxed)
                             + pHiMidDepth_->load(std::memory_order_relaxed)
                             + pHighDepth_->load(std::memory_order_relaxed);
        depthSumSmoothed_.setTargetValue(depthSum * 0.25f); // average across 4 bands
        mixSmoothed_.setTargetValue(pMix_->load(std::memory_order_relaxed));

        for (int i = 0; i < numSamples; ++i)
        {
            const float depth = depthSumSmoothed_.getNextValue();
            const float mix   = mixSmoothed_.getNextValue();
            const float gain  = 1.0f - depth * 0.5f; // placeholder ducking
            const float dryL  = L[i];
            const float dryR  = R[i];
            L[i] = dryL * (1.0f - mix) + dryL * gain * mix;
            R[i] = dryR * (1.0f - mix) + dryR * gain * mix;
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
        pMix_        = apvts.getRawParameterValue(p + "mix");
        // Remaining param pointers cached during Pack 1 implementation when
        // their consuming DSP stages are added.
    }

private:
    double sr_ = 0.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depthSumSmoothed_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed_;

    std::atomic<float>* pLowDepth_   = nullptr;
    std::atomic<float>* pLoMidDepth_ = nullptr;
    std::atomic<float>* pHiMidDepth_ = nullptr;
    std::atomic<float>* pHighDepth_  = nullptr;
    std::atomic<float>* pMix_        = nullptr;

    // Set once via setDNABus() on message thread before audio starts;
    // read on audio thread without synchronisation (single-writer, single-reader,
    // before-after pattern). Pack 1 implementation will read DNA per block.
    const DNAModulationBus* dnaBus_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OligoChain)
};

} // namespace xoceanus
