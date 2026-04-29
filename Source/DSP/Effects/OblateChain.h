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
// OblateChain — Spectral Gate (FX Pack 1, Sidechain Creative)
//
// Wildcard: STFT gate where each FFT bin is gated by the partner engine's
// spectrum, with partner brightness DNA tilting the threshold curve.
// Sidechain key driven by partner *spectrum*, not just amplitude.
//
// Parameter prefix: obla_  (12 params: 11 base + obla_hqMode per spec §10 A2)
//
// Routing: Stereo In → Stereo Out (per-channel STFT analysis)
// Latency: PDC declared per active FFT size (default 1024 ≈ 23 ms)
//
// Phase 0 status: SCAFFOLD ONLY.
//   - Class structure, parameters, DNA bus consumption hook are complete.
//   - DSP is a placeholder pass-through with threshold-modulated gain.
//   - Real STFT/ISTFT pipeline + per-bin gating lands in Pack 1 implementation.
//
// See: Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §3, §10 (A2)
//==============================================================================
class OblateChain
{
public:
    OblateChain() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = sampleRate;
        thresholdSmoothed_.reset(sampleRate, 0.02);
        mixSmoothed_.reset(sampleRate, 0.02);
    }

    void reset()
    {
        thresholdSmoothed_.setCurrentAndTargetValue(-20.0f);
        mixSmoothed_.setCurrentAndTargetValue(1.0f);
    }

    // Inject DNA bus pointer (set once on message thread before audio starts).
    // Pack 1 implementation wires this into the dnaCoupling parameter — partner
    // brightness DNA scales the per-bin threshold tilt.
    void setDNABus(const DNAModulationBus* bus) noexcept { dnaBus_ = bus; }

    // Stereo in, stereo out. Pack 1 will replace this with STFT/ISTFT
    // resynthesis driven by the sidechain key extractor; for now applies a
    // simple threshold-modulated mute to demonstrate parameter wiring.
    void processBlock(float* L, float* R, int numSamples,
                      double /*bpm*/ = 0.0, double /*ppqPosition*/ = -1.0)
    {
        if (! pThreshold_ || ! pMix_) return;

        thresholdSmoothed_.setTargetValue(pThreshold_->load(std::memory_order_relaxed));
        mixSmoothed_.setTargetValue(pMix_->load(std::memory_order_relaxed));

        for (int i = 0; i < numSamples; ++i)
        {
            const float thresholdDb = thresholdSmoothed_.getNextValue();
            const float mix         = mixSmoothed_.getNextValue();
            const float thresholdLin = juce::Decibels::decibelsToGain(thresholdDb);
            const float dryL = L[i];
            const float dryR = R[i];
            const float gateL = std::abs(dryL) > thresholdLin ? dryL : 0.0f;
            const float gateR = std::abs(dryR) > thresholdLin ? dryR : 0.0f;
            L[i] = dryL * (1.0f - mix) + gateL * mix;
            R[i] = dryR * (1.0f - mix) + gateR * mix;
        }
    }

    //--------------------------------------------------------------------------
    // APVTS integration — 12 parameters per Pack 1 spec §3 + A2 (hqMode).
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "obla_";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "threshold", 1), "OBLA Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f), -20.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "ratio", 1), "OBLA Ratio",
            juce::NormalisableRange<float>(1.0f, 100.0f, 0.0f, 0.3f), 4.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "attack", 1), "OBLA Attack",
            juce::NormalisableRange<float>(0.1f, 50.0f, 0.0f, 0.5f), 2.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "release", 1), "OBLA Release",
            juce::NormalisableRange<float>(5.0f, 500.0f, 0.0f, 0.5f), 100.0f));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "keyEngine", 1), "OBLA Key Engine", 0, 3, 0));
        // A2 (locked 2026-04-27): default 1024, 2048 only when hqMode is on.
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(p + "fftSize", 1), "OBLA FFT Size",
            juce::StringArray{"256", "512", "1024", "2048"}, 2));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "tilt", 1), "OBLA Tilt",
            juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "dnaCoupling", 1), "OBLA DNA Coupling",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "smoothing", 1), "OBLA Smoothing",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        // D005 floor: 0.001 Hz.
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "breathRate", 1), "OBLA Breath Rate",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.1f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "mix", 1), "OBLA Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
        // A2 (locked 2026-04-27): HQ Mode toggle gates the 2048 FFT option.
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(p + "hqMode", 1), "OBLA HQ Mode", false));
    }

    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "obla_";
        pThreshold_ = apvts.getRawParameterValue(p + "threshold");
        pMix_       = apvts.getRawParameterValue(p + "mix");
        // Remaining param pointers cached during Pack 1 implementation when
        // their consuming DSP stages are added.
    }

private:
    double sr_ = 0.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> thresholdSmoothed_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed_;

    std::atomic<float>* pThreshold_ = nullptr;
    std::atomic<float>* pMix_       = nullptr;

    // Set once via setDNABus() on message thread before audio starts;
    // read on audio thread without synchronisation (single-writer, single-reader,
    // before-after pattern). Pack 1 implementation will read DNA per block.
    const DNAModulationBus* dnaBus_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OblateChain)
};

} // namespace xoceanus
