// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include "../../Core/DNAModulationBus.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

namespace xoceanus
{

//==============================================================================
// OtriumChain — Triangular Coupling Pump (FX Pack 1, Sidechain Creative)
//
// Wildcard: 3 partner engines duck each other in a phase-staggered loop
// (the matrix demo). Reuses existing TriangularCoupling primitive — no
// MegaCouplingMatrix changes required.
//
// Parameter prefix: otrm_  (13 params, locked in Pack 1 spec §10 incl. A3)
//
// Routing: Stereo In → Stereo Out (no expansion stage)
// Accent: TBD — Pack 1 color table review
//
// Phase 0 status: SCAFFOLD ONLY.
//   - Class structure, parameters, DNA bus consumption hook are complete.
//   - DSP is a placeholder pass-through with pumpDepth-modulated gain.
//   - Real triangular ducking via TriangularCoupling routes lands in Pack 1
//     implementation (per §9 build sequence).
//
// See: Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §2
//==============================================================================
class OtriumChain
{
public:
    OtriumChain() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = sampleRate;
        pumpDepthSmoothed_.reset(sampleRate, 0.02);
        mixSmoothed_.reset(sampleRate, 0.02);
    }

    void reset()
    {
        pumpDepthSmoothed_.setCurrentAndTargetValue(0.5f);
        mixSmoothed_.setCurrentAndTargetValue(1.0f);
    }

    // Inject DNA bus pointer (set once on message thread before audio starts).
    // Not yet read by the placeholder DSP; Pack 1 implementation wires this
    // into the dnaTilt parameter (see Pack 1 spec §2 wildcard).
    void setDNABus(const DNAModulationBus* bus) noexcept { dnaBus_ = bus; }

    // Stereo in, stereo out. Pack 1 will overload to accept partner-engine
    // envelope state via TriangularCoupling routes; for now, applies a static
    // pumpDepth-modulated gain to demonstrate parameter wiring.
    void processBlock(float* L, float* R, int numSamples,
                      double /*bpm*/ = 0.0, double /*ppqPosition*/ = -1.0)
    {
        if (! pPumpDepth_ || ! pMix_) return;

        pumpDepthSmoothed_.setTargetValue(pPumpDepth_->load(std::memory_order_relaxed));
        mixSmoothed_.setTargetValue(pMix_->load(std::memory_order_relaxed));

        for (int i = 0; i < numSamples; ++i)
        {
            const float depth = pumpDepthSmoothed_.getNextValue();
            const float mix   = mixSmoothed_.getNextValue();
            const float gain  = 1.0f - depth * 0.5f; // placeholder ducking
            const float dryL  = L[i];
            const float dryR  = R[i];
            L[i] = dryL * (1.0f - mix) + dryL * gain * mix;
            R[i] = dryR * (1.0f - mix) + dryR * gain * mix;
        }
    }

    //--------------------------------------------------------------------------
    // APVTS integration — 13 parameters per Pack 1 spec §2.
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "otrm_";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "pumpDepth", 1), "OTRM Pump Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "pumpRate", 1), "OTRM Pump Rate",
            juce::NormalisableRange<float>(0.001f, 40.0f, 0.0f, 0.3f), 1.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "attack", 1), "OTRM Attack",
            juce::NormalisableRange<float>(0.1f, 200.0f, 0.0f, 0.5f), 5.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "release", 1), "OTRM Release",
            juce::NormalisableRange<float>(10.0f, 2000.0f, 0.0f, 0.5f), 200.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "phaseSkew", 1), "OTRM Phase Skew",
            juce::NormalisableRange<float>(0.0f, 360.0f), 120.0f));
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(p + "topology", 1), "OTRM Topology",
            juce::StringArray{"Equilateral", "Isoceles", "Chaotic", "Cyclical"}, 0));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "partnerA_idx", 1), "OTRM Partner A", 0, 3, 0));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "partnerB_idx", 1), "OTRM Partner B", 0, 3, 1));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "partnerC_idx", 1), "OTRM Partner C", 0, 3, 2));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "couplingDepth", 1), "OTRM Coupling Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "dnaTilt", 1), "OTRM DNA Tilt",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "mix", 1), "OTRM Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
        // A3 (locked 2026-04-27): tempo-sync mode for Cyclical topology.
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(p + "syncMode", 1), "OTRM Sync Mode",
            juce::StringArray{"Free", "Sync"}, 0));
    }

    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "otrm_";
        pPumpDepth_ = apvts.getRawParameterValue(p + "pumpDepth");
        pMix_       = apvts.getRawParameterValue(p + "mix");
        // Remaining param pointers cached during Pack 1 implementation when
        // their consuming DSP stages are added.
    }

private:
    double sr_ = 0.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pumpDepthSmoothed_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed_;

    std::atomic<float>* pPumpDepth_ = nullptr;
    std::atomic<float>* pMix_       = nullptr;

    // Set once via setDNABus() on message thread before audio starts;
    // read on audio thread without synchronisation (single-writer, single-reader,
    // before-after pattern). Pack 1 implementation will read DNA per block.
    const DNAModulationBus* dnaBus_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OtriumChain)
};

} // namespace xoceanus
