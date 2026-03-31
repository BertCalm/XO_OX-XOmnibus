#pragma once
//==============================================================================
// OxytocinAdapter.h — XOlokun adapter for OxytocinEngine
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

namespace xolokun {

class OxytocinAdapter : public SynthEngine
{
public:
    OxytocinAdapter() = default;

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Oxytocin"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF9B5DE5); }  // Synapse Violet
    int getMaxVoices() const override { return OxytocinEngine::MaxVoices; }

    int getActiveVoiceCount() const override { return activeVoiceCount_.load(); }

    //-- Lifecycle ---------------------------------------------------------------

    void prepare (double sampleRate, int maxBlockSize) override
    {
        engine_.prepare (sampleRate, maxBlockSize);
        snap_.update (*(apvts_));   // safe no-op if apvts_ not yet set
        // SRO SilenceGate: polyphonic synth with reverb-like tails — 500ms hold
        prepareSilenceGate (sampleRate, maxBlockSize, 500.0f);
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
        couplingAmpToFilterMod_  = 0.0f;
        couplingAmpToPitchMod_   = 0.0f;
        couplingAudioToFMMod_    = 0.0f;
        couplingTriangularMod_   = 0.0f;
    }

    //-- Audio -------------------------------------------------------------------

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi, int numSamples) override
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
            buffer.clear();
            return;
        }

        // Cache params once per block (ParamSnapshot pattern)
        if (apvts_ != nullptr)
        {
            snap_.update (*apvts_);

            // Apply block-rate coupling modulations to snap before processing
            // AmpToFilter → modulate cutoff
            if (couplingAmpToFilterMod_ != 0.0f)
                snap_.cutoff = std::clamp (snap_.cutoff + couplingAmpToFilterMod_ * 4000.0f,
                                           20.0f, 20000.0f);

            // AmpToPitch → modulate pitch (semitone offset)
            if (std::abs (couplingAmpToPitchMod_) > 0.001f)
                snap_.pitch = std::clamp (snap_.pitch + couplingAmpToPitchMod_ * 12.0f,
                                          -24.0f, 24.0f);

            // AudioToFM → modulate entanglement depth
            if (couplingAudioToFMMod_ != 0.0f)
                snap_.entanglement = std::clamp (snap_.entanglement + couplingAudioToFMMod_ * 0.4f,
                                                 0.0f, 1.0f);

            // TriangularCoupling (#15) → modulate I/P/C from source engine's love triangle
            if (couplingTriangularMod_ != 0.0f)
            {
                snap_.intimacy   = std::clamp (snap_.intimacy   + couplingTriangularMod_ * 0.3f, 0.0f, 1.0f);
                snap_.passion    = std::clamp (snap_.passion    + couplingTriangularMod_ * 0.2f, 0.0f, 1.0f);
                snap_.commitment = std::clamp (snap_.commitment + couplingTriangularMod_ * 0.15f, 0.0f, 1.0f);
            }
        }

        // Run the DSP engine
        engine_.processBlock (buffer, midi, snap_);

        // Cache last stereo sample for coupling reads
        if (numSamples > 0 && buffer.getNumChannels() >= 2)
        {
            const auto* L = buffer.getReadPointer (0);
            const auto* R = buffer.getReadPointer (1);
            couplingCacheL_ = L[numSamples - 1];
            couplingCacheR_ = R[numSamples - 1];
        }

        // Update active voice count (for UI) — reads live voice state on audio thread,
        // then publishes atomically so the UI thread can read it without blocking.
        activeVoiceCount_.store (engine_.getActiveVoiceCount());

        analyzeForSilenceGate (buffer, numSamples);

        // Reset per-block coupling accumulators
        couplingAmpToFilterMod_  = 0.0f;
        couplingAmpToPitchMod_   = 0.0f;
        couplingAudioToFMMod_    = 0.0f;
        couplingTriangularMod_   = 0.0f;
    }

    //-- Coupling ----------------------------------------------------------------

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingCacheL_ : couplingCacheR_;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        if (std::abs (amount) < 0.001f || sourceBuffer == nullptr) return;

        // Compute RMS of source block
        float rms = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            rms += sourceBuffer[i] * sourceBuffer[i];
        rms = std::sqrt (rms / static_cast<float> (juce::jmax (1, numSamples)));

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
                // KnotTopology (#14): bidirectional entanglement — also bleeds triangle state
                couplingTriangularMod_ += rms * amount;
                break;

            case CouplingType::TriangularCoupling:
                // TriangularCoupling (#15): source love-triangle state bleeds into this engine.
                // I/P/C values from the source modulate this engine's corresponding components.
                couplingTriangularMod_ += rms * amount;
                break;

            default:
                // Other coupling types accepted but not yet mapped — expand as needed.
                break;
        }
    }

    //-- Parameters --------------------------------------------------------------

    /// Called from XOlokunProcessor::createParameterLayout() to add oxy_ params
    /// to the global APVTS alongside all other engine parameters.
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        apvts_ = &apvts;
        snap_.attachParameters (apvts); // F02: cache 29 raw pointers once; update() has no string lookups
    }

private:
    //-- Parameter layout --------------------------------------------------------

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PI = juce::AudioParameterInt;

        // Love components (3)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_intimacy", 1 }, "Oxytocin Intimacy",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_passion", 1 }, "Oxytocin Passion",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.25f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_commitment", 1 }, "Oxytocin Commitment",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        // Love envelope rates (3)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_warmth_rate", 1 }, "Oxytocin Warmth Rate",
            juce::NormalisableRange<float> (0.01f, 8.0f, 0.001f, 0.5f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_passion_rate", 1 }, "Oxytocin Passion Rate",
            juce::NormalisableRange<float> (0.001f, 0.5f, 0.001f, 0.5f), 0.005f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_commit_rate", 1 }, "Oxytocin Commit Rate",
            juce::NormalisableRange<float> (0.05f, 5.0f, 0.001f, 0.5f), 1.0f));

        // Modulation / circuit character (3)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_entanglement", 1 }, "Oxytocin Entanglement",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.4f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_circuit_age", 1 }, "Oxytocin Circuit Age",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_circuit_noise", 1 }, "Oxytocin Circuit Noise",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.05f));

        // Session memory (2)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_memory_depth", 1 }, "Oxytocin Memory Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_memory_decay", 1 }, "Oxytocin Memory Decay",
            juce::NormalisableRange<float> (0.5f, 30.0f, 0.01f, 0.5f), 5.0f));

        // Topology (2)
        params.push_back (std::make_unique<PI> (
            juce::ParameterID { "oxy_topology", 1 }, "Oxytocin Topology", 0, 2, 0));
        params.push_back (std::make_unique<PI> (
            juce::ParameterID { "oxy_topology_lock", 1 }, "Oxytocin Topology Lock", 0, 8, 0));

        // Filter / pitch (4)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_feedback", 1 }, "Oxytocin Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f), 0.3f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_cutoff", 1 }, "Oxytocin Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_pitch", 1 }, "Oxytocin Pitch",
            juce::NormalisableRange<float> (-24.0f, 24.0f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_detune", 1 }, "Oxytocin Detune",
            juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));

        // Amplitude ADSR (4)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_attack", 1 }, "Oxytocin Attack",
            juce::NormalisableRange<float> (0.001f, 2.0f, 0.001f, 0.5f), 0.01f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_decay", 1 }, "Oxytocin Decay",
            juce::NormalisableRange<float> (0.01f, 5.0f, 0.001f, 0.5f), 0.5f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_sustain", 1 }, "Oxytocin Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.7f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_release", 1 }, "Oxytocin Release",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.001f, 0.5f), 0.5f));

        // LFO1 (3) — D005 floor: rate min 0.001 Hz (~16-minute cycles)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_lfo_rate", 1 }, "Oxytocin LFO Rate",
            juce::NormalisableRange<float> (0.001f, 20.0f, 0.001f, 0.4f), 1.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_lfo_depth", 1 }, "Oxytocin LFO Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (
            juce::ParameterID { "oxy_lfo_shape", 1 }, "Oxytocin LFO Shape", 0, 4, 0));

        // LFO2 (2) — triangle position modulator (physiological 0.067 Hz = ~15s cycle)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_lfo2_rate", 1 }, "Oxytocin LFO2 Rate",
            juce::NormalisableRange<float> (0.001f, 20.0f, 0.001f, 0.4f), 0.067f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_lfo2_depth", 1 }, "Oxytocin LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        // Output (3)
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_output", 1 }, "Oxytocin Output",
            juce::NormalisableRange<float> (-60.0f, 6.0f), 0.0f));
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "oxy_pan", 1 }, "Oxytocin Pan",
            juce::NormalisableRange<float> (-1.0f, 1.0f), 0.0f));
        params.push_back (std::make_unique<PI> (
            juce::ParameterID { "oxy_voices", 1 }, "Oxytocin Voices", 1, 8, 4));
    }

    //-- Members -----------------------------------------------------------------

    OxytocinEngine  engine_;
    ParamSnapshot   snap_;
    juce::AudioProcessorValueTreeState* apvts_ = nullptr;

    // Coupling caches
    float couplingCacheL_         = 0.0f;
    float couplingCacheR_         = 0.0f;
    float couplingAmpToFilterMod_ = 0.0f;
    float couplingAmpToPitchMod_  = 0.0f;
    float couplingAudioToFMMod_   = 0.0f;
    float couplingTriangularMod_  = 0.0f;

    std::atomic<int> activeVoiceCount_ { 0 };
};

} // namespace xolokun
