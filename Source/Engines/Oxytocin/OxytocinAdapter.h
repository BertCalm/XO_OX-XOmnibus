// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
// OxytocinAdapter.h — XOceanus adapter for OxytocinEngine
//
// Engine: XOxytocin (OXYTO) — Engine #48
// Concept: Circuit-modeling synth based on Sternberg's Triangular Theory of Love
// Accent: Synapse Violet #9B5DE5 | Prefix: oxy_ | Voices: 8
// Legend lineages: RE-201 warmth, MS-20 saturation, Moog Ladder commitment filter,
//                  Serge cross-routing entanglement, Buchla topology freedom
// Coupling type: TriangularCoupling (#15)
//   bandA = effective_intimacy
//   bandB = effective_passion
//   bandC = effective_commitment
//   bandD = memory accumulator average
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "OxytocinEngine.h"
#include "OxytocinParamSnapshot.h"

namespace xoceanus
{

// Bring xoxytocin types into scope within this adapter
using namespace xoxytocin;

class OxytocinAdapter : public SynthEngine
{
public:
    OxytocinAdapter() = default;

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Oxytocin"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF9B5DE5); } // Synapse Violet
    int getMaxVoices() const override { return OxytocinEngine::MaxVoices; }

    int getActiveVoiceCount() const override { return activeVoiceCount_.load(); }

    //-- Lifecycle ---------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        engine_.prepare(sampleRate, maxBlockSize);
        // F26 fix: apvts_ may be nullptr if prepare() is called before attachParameters()
        // (valid JUCE lifecycle).  Forming *(nullptr) is undefined behaviour even though
        // the overload ignores the reference — guard before dereferencing.
        if (apvts_ != nullptr)
            snap_.update(*apvts_);
        // SRO SilenceGate: polyphonic synth with reverb-like tails — 500ms hold
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
        couplingCacheL_ = 0.0f;
        couplingCacheR_ = 0.0f;
    }

    void releaseResources() override {}

    void reset() override
    {
        // OxytocinEngine has no explicit reset() — re-prepare at current SR
        // This is safe because prepare() calls memory.reset() and re-initialises voices.
        // Callers guarantee prepare() has been called before reset().
        couplingCacheL_ = 0.0f;
        couplingCacheR_ = 0.0f;
        couplingAmpToFilterMod_ = 0.0f;
        couplingAmpToPitchMod_ = 0.0f;
        couplingAudioToFMMod_ = 0.0f;
        couplingIntimacyMod_ = 0.0f;
        couplingPassionMod_ = 0.0f;
        couplingCommitmentMod_ = 0.0f;
        lastLoveState_ = {};
    }

    //-- Audio -------------------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        // Wake silence gate on note-on
        for (const auto& metadata : midi)
        {
            if (metadata.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }

        // Zero-idle bypass
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            return;
        }

        // Cache params once per block (ParamSnapshot pattern)
        if (apvts_ != nullptr)
        {
            snap_.update(*apvts_);

            // D002 Macros: apply block-rate macro modulations before coupling mods
            // M1 CHARACTER: circuit age (warmth/grime) and circuit noise (texture)
            //   range: circuitAge [0,1], circuitNoise [0,1]. Default 0.5 → no change.
            {
                const float m1 = pMacro1 ? pMacro1->load(std::memory_order_relaxed) : 0.5f;
                snap_.circuitAge  = std::clamp(snap_.circuitAge  + (m1 - 0.5f) * 1.0f, 0.0f, 1.0f);
                snap_.circuitNoise = std::clamp(snap_.circuitNoise + (m1 - 0.5f) * 0.3f, 0.0f, 1.0f);
            }
            // M2 MOVEMENT: LFO rate × up to 4× and warmth/passion envelope rates
            {
                const float m2 = pMacro2 ? pMacro2->load(std::memory_order_relaxed) : 0.5f;
                snap_.lfoRate   = std::clamp(snap_.lfoRate   * (0.5f + m2 * 1.5f), 0.001f, 20.0f);
                snap_.warmthRate = std::clamp(snap_.warmthRate * (0.5f + m2 * 1.5f), 0.01f, 8.0f);
                snap_.passionRate = std::clamp(snap_.passionRate * (0.5f + m2 * 1.5f), 0.001f, 0.5f);
            }
            // M3 COUPLING: intimacy and commitment — core love-triangle coupling outputs
            {
                const float m3 = pMacro3 ? pMacro3->load(std::memory_order_relaxed) : 0.5f;
                snap_.intimacy   = std::clamp(snap_.intimacy   + (m3 - 0.5f) * 0.6f, 0.0f, 1.0f);
                snap_.commitment = std::clamp(snap_.commitment + (m3 - 0.5f) * 0.4f, 0.0f, 1.0f);
            }
            // M4 SPACE: feedback (spatial tail) and detune (stereo spread)
            {
                const float m4 = pMacro4 ? pMacro4->load(std::memory_order_relaxed) : 0.5f;
                snap_.feedback = std::clamp(snap_.feedback + (m4 - 0.5f) * 0.5f, 0.0f, 0.95f);
                snap_.detune   = std::clamp(snap_.detune   + (m4 - 0.5f) * 60.0f, 0.0f, 100.0f);
            }

            // Apply block-rate coupling modulations to snap before processing
            // AmpToFilter → modulate cutoff
            if (couplingAmpToFilterMod_ != 0.0f)
                snap_.cutoff = std::clamp(snap_.cutoff + couplingAmpToFilterMod_ * 4000.0f, 20.0f, 20000.0f);

            // AmpToPitch → modulate pitch (semitone offset)
            if (std::abs(couplingAmpToPitchMod_) > 0.001f)
                snap_.pitch = std::clamp(snap_.pitch + couplingAmpToPitchMod_ * 12.0f, -24.0f, 24.0f);

            // AudioToFM → modulate entanglement depth
            if (couplingAudioToFMMod_ != 0.0f)
                snap_.entanglement = std::clamp(snap_.entanglement + couplingAudioToFMMod_ * 0.4f, 0.0f, 1.0f);

            // TriangularCoupling (#15) → modulate I/P/C independently from source
            // engine's actual love-triangle state (fixes #420: semantic mismatch).
            // couplingIntimacyMod_/PassionMod_/CommitmentMod_ are populated by
            // applyTriangularCouplingInput(), which receives the source's real I/P/C
            // values from MegaCouplingMatrix rather than a generic RMS signal.
            if (couplingIntimacyMod_ != 0.0f || couplingPassionMod_ != 0.0f || couplingCommitmentMod_ != 0.0f)
            {
                snap_.intimacy = std::clamp(snap_.intimacy + couplingIntimacyMod_, 0.0f, 1.0f);
                snap_.passion = std::clamp(snap_.passion + couplingPassionMod_, 0.0f, 1.0f);
                snap_.commitment = std::clamp(snap_.commitment + couplingCommitmentMod_, 0.0f, 1.0f);
            }
        }

        // Run the DSP engine
        engine_.processBlock(buffer, midi, snap_);

        // Cache block-peak stereo levels for coupling reads.
        // F29 fix: previously stored the last sample — unreliable for percussive signals
        // where the transient may be at the start and the tail is near silence.
        // Block-peak gives downstream coupling engines a stable, attack-representative value.
        if (numSamples > 0 && buffer.getNumChannels() >= 2)
        {
            const auto* L = buffer.getReadPointer(0);
            const auto* R = buffer.getReadPointer(1);
            float peakL = 0.0f, peakR = 0.0f;
            for (int s = 0; s < numSamples; ++s)
            {
                peakL = std::max(peakL, std::abs(L[s]));
                peakR = std::max(peakR, std::abs(R[s]));
            }
            couplingCacheL_ = peakL;
            couplingCacheR_ = peakR;
        }

        // Update active voice count (for UI) — reads live voice state on audio thread,
        // then publishes atomically so the UI thread can read it without blocking.
        activeVoiceCount_.store(engine_.getActiveVoiceCount());

        analyzeForSilenceGate(buffer, numSamples);

        // Cache current love-triangle state for getLoveTriangleState() reads.
        // Stored after renderBlock so any engine routing TriangularCoupling FROM
        // Oxytocin receives this block's effective I/P/C values next block.
        lastLoveState_ = {snap_.intimacy, snap_.passion, snap_.commitment};

        // Reset per-block coupling accumulators
        couplingAmpToFilterMod_ = 0.0f;
        couplingAmpToPitchMod_ = 0.0f;
        couplingAudioToFMMod_ = 0.0f;
        couplingIntimacyMod_ = 0.0f;
        couplingPassionMod_ = 0.0f;
        couplingCommitmentMod_ = 0.0f;
    }

    //-- Coupling ----------------------------------------------------------------

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingCacheL_ : couplingCacheR_;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        if (std::abs(amount) < 0.001f || sourceBuffer == nullptr)
            return;

        // Compute RMS of source block
        float rms = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            rms += sourceBuffer[i] * sourceBuffer[i];
        rms = std::sqrt(rms / static_cast<float>(juce::jmax(1, numSamples)));

        switch (type)
        {
        case CouplingType::AmpToFilter:
            // Source amplitude modulates XOxytocin cutoff
            couplingAmpToFilterMod_ += rms * amount;
            break;

        case CouplingType::AmpToPitch:
            // Source amplitude modulates XOxytocin pitch
            couplingAmpToPitchMod_ += rms * amount;
            break;

        case CouplingType::AudioToFM:
            // Source audio FM-modulates entanglement depth (Serge cross-routing)
            couplingAudioToFMMod_ += rms * amount;
            break;

        case CouplingType::KnotTopology:
            // KnotTopology (#14): bidirectional entanglement — treated as AmpToFilter
            // (processKnotRoute already dispatches as AmpToFilter, but in case a
            // direct call arrives, map it to filter modulation).
            couplingAmpToFilterMod_ += rms * amount;
            break;

            // TriangularCoupling (#15) is NOT handled here — it is routed through
            // applyTriangularCouplingInput() which carries separate I/P/C values.
            // The MegaCouplingMatrix calls applyTriangularCouplingInput() directly
            // for TriangularCoupling routes, bypassing applyCouplingInput() entirely.

        default:
            // Other coupling types accepted but not yet mapped — expand as needed.
            break;
        }
    }

    // TriangularCoupling source accessor (fixes #420).
    // Returns the effective I/P/C values from the most recently rendered block.
    // The MegaCouplingMatrix calls this on Oxytocin when it is the SOURCE of a
    // TriangularCoupling route, then forwards the state to the destination engine.
    LoveTriangleState getLoveTriangleState() const override { return lastLoveState_; }

    // TriangularCoupling destination handler (fixes #420 + #434).
    // Receives per-component I/P/C modulation from the source engine's love-triangle
    // state, rather than a generic RMS scalar.  Each component modulates its
    // corresponding Oxytocin love parameter independently.
    //
    // Scaling factors:
    //   Intimacy  × 0.3  — slow-building warmth; large coefficient for warmth accumulation
    //   Passion   × 0.2  — transient excitement; moderate coefficient
    //   Commitment× 0.15 — stable baseline; smallest coefficient to avoid parameter lock-up
    void applyTriangularCouplingInput(LoveTriangleState state, float amount) override
    {
        if (std::abs(amount) < 0.001f)
            return;
        couplingIntimacyMod_ += state.I * amount * 0.3f;
        couplingPassionMod_ += state.P * amount * 0.2f;
        couplingCommitmentMod_ += state.C * amount * 0.15f;
    }

    //-- Parameters --------------------------------------------------------------

    /// Called from XOceanusProcessor::createParameterLayout() to add oxy_ params
    /// to the global APVTS alongside all other engine parameters.
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        apvts_ = &apvts;
        snap_.attachParameters(apvts); // F02: cache 29 raw pointers once; update() has no string lookups
        // D002 macros
        pMacro1 = apvts.getRawParameterValue("oxy_macro1");
        pMacro2 = apvts.getRawParameterValue("oxy_macro2");
        pMacro3 = apvts.getRawParameterValue("oxy_macro3");
        pMacro4 = apvts.getRawParameterValue("oxy_macro4");
    }

private:
    //-- Parameter layout --------------------------------------------------------

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Love components (3)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_intimacy", 1}, "Oxytocin Intimacy",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_passion", 1}, "Oxytocin Passion",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.25f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_commitment", 1}, "Oxytocin Commitment",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

        // Love envelope rates (3)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_warmth_rate", 1}, "Oxytocin Warmth Rate",
                                              juce::NormalisableRange<float>(0.01f, 8.0f, 0.001f, 0.5f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_passion_rate", 1}, "Oxytocin Passion Rate",
                                              juce::NormalisableRange<float>(0.001f, 0.5f, 0.001f, 0.5f), 0.005f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_commit_rate", 1}, "Oxytocin Commit Rate",
                                              juce::NormalisableRange<float>(0.05f, 5.0f, 0.001f, 0.5f), 1.0f));

        // Modulation / circuit character (3)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_entanglement", 1}, "Oxytocin Entanglement",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_circuit_age", 1}, "Oxytocin Circuit Age",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_circuit_noise", 1}, "Oxytocin Circuit Noise",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.05f));

        // Session memory (2)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_memory_depth", 1}, "Oxytocin Memory Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_memory_decay", 1}, "Oxytocin Memory Decay",
                                              juce::NormalisableRange<float>(0.5f, 30.0f, 0.01f, 0.5f), 5.0f));

        // Topology (2)
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oxy_topology", 1}, "Oxytocin Topology", 0, 2, 0));
        params.push_back(
            std::make_unique<PI>(juce::ParameterID{"oxy_topology_lock", 1}, "Oxytocin Topology Lock", 0, 8, 0));

        // Filter / pitch (4)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_feedback", 1}, "Oxytocin Feedback",
                                              juce::NormalisableRange<float>(0.0f, 0.95f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_cutoff", 1}, "Oxytocin Cutoff",
                                              juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_pitch", 1}, "Oxytocin Pitch",
                                              juce::NormalisableRange<float>(-24.0f, 24.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_detune", 1}, "Oxytocin Detune",
                                              juce::NormalisableRange<float>(0.0f, 100.0f), 0.0f));

        // Amplitude ADSR (4)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_attack", 1}, "Oxytocin Attack",
                                              juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.5f), 0.01f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_decay", 1}, "Oxytocin Decay",
                                              juce::NormalisableRange<float>(0.01f, 5.0f, 0.001f, 0.5f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_sustain", 1}, "Oxytocin Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_release", 1}, "Oxytocin Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.001f, 0.5f), 0.5f));

        // LFO1 (3) — D005 floor: rate min 0.001 Hz (~16-minute cycles)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_lfo_rate", 1}, "Oxytocin LFO Rate",
                                              juce::NormalisableRange<float>(0.001f, 20.0f, 0.001f, 0.4f), 1.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_lfo_depth", 1}, "Oxytocin LFO Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oxy_lfo_shape", 1}, "Oxytocin LFO Shape", 0, 4, 0));

        // LFO2 (2) — triangle position modulator (physiological 0.067 Hz = ~15s cycle)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_lfo2_rate", 1}, "Oxytocin LFO2 Rate",
                                              juce::NormalisableRange<float>(0.001f, 20.0f, 0.001f, 0.4f), 0.067f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_lfo2_depth", 1}, "Oxytocin LFO2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Output (3)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_output", 1}, "Oxytocin Output",
                                              juce::NormalisableRange<float>(-60.0f, 6.0f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_pan", 1}, "Oxytocin Pan",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
        params.push_back(std::make_unique<PI>(juce::ParameterID{"oxy_voices", 1}, "Oxytocin Voices", 1, 8, 4));

        // D002 Macros (4 required, range [0,1], default 0.5)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_macro1", 1}, "Oxy Character",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_macro2", 1}, "Oxy Movement",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_macro3", 1}, "Oxy Coupling",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"oxy_macro4", 1}, "Oxy Space",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    }

    //-- Members -----------------------------------------------------------------

    OxytocinEngine engine_;
    ParamSnapshot snap_;
    juce::AudioProcessorValueTreeState* apvts_ = nullptr;

    // D002 macro parameter pointers
    std::atomic<float>* pMacro1 = nullptr; // CHARACTER: circuit age + noise
    std::atomic<float>* pMacro2 = nullptr; // MOVEMENT:  LFO rate + envelope rates
    std::atomic<float>* pMacro3 = nullptr; // COUPLING:  intimacy + commitment
    std::atomic<float>* pMacro4 = nullptr; // SPACE:     feedback + detune

    // Coupling caches
    float couplingCacheL_ = 0.0f;
    float couplingCacheR_ = 0.0f;
    float couplingAmpToFilterMod_ = 0.0f;
    float couplingAmpToPitchMod_ = 0.0f;
    float couplingAudioToFMMod_ = 0.0f;
    // TriangularCoupling (#15) — per-component modulators (replaces scalar couplingTriangularMod_).
    // Populated by applyTriangularCouplingInput(); consumed and reset in renderBlock().
    float couplingIntimacyMod_ = 0.0f;
    float couplingPassionMod_ = 0.0f;
    float couplingCommitmentMod_ = 0.0f;
    // Last rendered block's effective love-triangle state, exposed via getLoveTriangleState()
    // so that downstream TriangularCoupling routes receive real I/P/C values.
    LoveTriangleState lastLoveState_{};

    std::atomic<int> activeVoiceCount_{0};
};

} // namespace xoceanus
