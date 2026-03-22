#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/ParameterSmoother.h"
#include <cmath>
#include <array>

namespace xomnibus {

//==============================================================================
// OsmosisEngine — the membrane between XOmnibus and the outside world.
//
// Receives external audio via setExternalInput(). Analyzes envelope, pitch,
// and spectral content. Produces coupling signals that let other engines
// respond to the outside world.
//
// Does not generate its own sound — like Optic (B005), its primary output
// is coupling data, not audio. Wet/dry blend passes external audio through
// the membrane with subtle coloring (one-pole filter at membrane frequency).
//
// Macros: PERMEABILITY (wet/dry), SELECTIVITY (filter Q), REACTIVITY (env speed), MEMORY (decay)
// Param prefix: osmo_
// Accent: Surface Tension Silver #C0C0C0
//
class OsmosisEngine : public SynthEngine
{
public:
    OsmosisEngine() = default;

    //-- Identity --------------------------------------------------------------
    juce::String getEngineId() const override { return "Osmosis"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFC0C0C0); }
    int getMaxVoices() const override { return 1; } // mono analysis engine
    bool isAnalysisEngine() const override { return true; } // enables external audio routing without RTTI

    //-- Lifecycle -------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr_ = sampleRate;
        blockSize_ = maxBlockSize;

        // Envelope follower — one-pole with variable attack/release
        envFollowerL_ = 0.0f;
        envFollowerR_ = 0.0f;

        // Pitch detection — zero-crossing rate
        prevSampleL_ = 0.0f;
        zeroCrossingAccum_ = 0.0f;
        detectedPitch_ = 0.0f;

        // Spectral bands (simple RMS in 4 frequency ranges via biquad)
        for (int i = 0; i < 4; ++i)
            bandRMS_[i] = 0.0f;

        // Band-split filters (approximate: 200, 800, 3000 Hz crossovers)
        setupBandFilters();

        // Coupling output cache
        couplingSampleL_ = 0.0f;
        couplingSampleR_ = 0.0f;

        // Smoothers for macro parameters
        permeabilitySmoother_.prepare(static_cast<float>(sr_), 5.0f);
        selectivitySmoother_.prepare(static_cast<float>(sr_), 5.0f);
        reactivitySmoother_.prepare(static_cast<float>(sr_), 5.0f);
        memorySmoother_.prepare(static_cast<float>(sr_), 5.0f);

        // Membrane filter (one-pole LP for coloring pass-through audio)
        membraneLPCoeff_ = 0.0f;
        membraneLPStateL_ = 0.0f;
        membraneLPStateR_ = 0.0f;

        // LFO for subtle membrane modulation
        lfo_.setShape(StandardLFO::Shape::Sine);
        lfo_.setRate(0.5f, static_cast<float>(sr_));
        lfo_.reset();

        prepareSilenceGate(sr_, maxBlockSize, 500.0f);

        externalBufferL_ = nullptr;
        externalBufferR_ = nullptr;
        externalNumSamples_ = 0;
    }

    void releaseResources() override {}

    void reset() override
    {
        envFollowerL_ = 0.0f;
        envFollowerR_ = 0.0f;
        prevSampleL_ = 0.0f;
        zeroCrossingAccum_ = 0.0f;
        detectedPitch_ = 0.0f;
        for (int i = 0; i < 4; ++i)
            bandRMS_[i] = 0.0f;
        membraneLPStateL_ = 0.0f;
        membraneLPStateR_ = 0.0f;
        lfo_.reset();
    }

    //-- External audio injection (called by XOmnibusProcessor) ----------------
    void setExternalInput(const float* left, const float* right, int numSamples)
    {
        externalBufferL_ = left;
        externalBufferR_ = right;
        externalNumSamples_ = numSamples;
    }

    //-- Analysis getters (for coupling and UI) --------------------------------
    float getEnvelopeLevel() const { return (envFollowerL_ + envFollowerR_) * 0.5f; }
    float getDetectedPitch() const { return detectedPitch_; }
    const float* getBandRMS() const { return bandRMS_; }

    //-- Audio -----------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& /*midi*/,
                     int numSamples) override
    {
        // SilenceGate: early-out when no external audio is present (P1-E fix)
        if (isSilenceGateBypassed())
        {
            buffer.clear();
            return;
        }

        // Read macro parameters
        permeabilitySmoother_.set(pPermeability_ ? pPermeability_->load() : 0.5f);
        selectivitySmoother_.set(pSelectivity_ ? pSelectivity_->load() : 0.5f);
        reactivitySmoother_.set(pReactivity_ ? pReactivity_->load() : 0.5f);
        memorySmoother_.set(pMemory_ ? pMemory_->load() : 0.3f);

        const float permeability = permeabilitySmoother_.process();
        const float selectivity  = selectivitySmoother_.process();
        const float reactivity   = reactivitySmoother_.process();
        const float memory       = memorySmoother_.process();

        // Suppress unused warning — selectivity used in Phase 2 for filter Q
        (void)selectivity;

        // Envelope follower coefficients from reactivity
        const float attackMs = 1.0f + (1.0f - reactivity) * 49.0f;  // 1-50ms
        const float releaseMs = 10.0f + memory * 990.0f;             // 10-1000ms
        const float attackCoeff = std::exp(-1.0f / (static_cast<float>(sr_) * attackMs * 0.001f));
        const float releaseCoeff = std::exp(-1.0f / (static_cast<float>(sr_) * releaseMs * 0.001f));

        // Membrane filter frequency from selectivity
        const float membraneFreq = 200.0f + selectivity * 18000.0f;
        membraneLPCoeff_ = std::exp(-2.0f * 3.14159265f * membraneFreq / static_cast<float>(sr_));

        auto* outL = buffer.getWritePointer(0);
        auto* outR = buffer.getWritePointer(1);

        // Zero-crossing counter for pitch detection
        int crossings = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            // Read external input (or silence if none)
            const float inL = (externalBufferL_ && i < externalNumSamples_) ? externalBufferL_[i] : 0.0f;
            const float inR = (externalBufferR_ && i < externalNumSamples_) ? externalBufferR_[i] : 0.0f;

            // Envelope follower
            const float absL = std::fabs(inL);
            const float absR = std::fabs(inR);
            envFollowerL_ = (absL > envFollowerL_)
                ? attackCoeff * envFollowerL_ + (1.0f - attackCoeff) * absL
                : releaseCoeff * envFollowerL_ + (1.0f - releaseCoeff) * absL;
            envFollowerR_ = (absR > envFollowerR_)
                ? attackCoeff * envFollowerR_ + (1.0f - attackCoeff) * absR
                : releaseCoeff * envFollowerR_ + (1.0f - releaseCoeff) * absR;

            // Zero-crossing detection (left channel)
            if ((inL >= 0.0f && prevSampleL_ < 0.0f) || (inL < 0.0f && prevSampleL_ >= 0.0f))
                ++crossings;
            prevSampleL_ = inL;

            // Membrane filter (one-pole LP for pass-through coloring)
            membraneLPStateL_ = membraneLPCoeff_ * membraneLPStateL_ + (1.0f - membraneLPCoeff_) * inL;
            membraneLPStateR_ = membraneLPCoeff_ * membraneLPStateR_ + (1.0f - membraneLPCoeff_) * inR;

            // Output: blend dry external with filtered (permeability = wet/dry)
            outL[i] = inL * (1.0f - permeability) + membraneLPStateL_ * permeability;
            outR[i] = inR * (1.0f - permeability) + membraneLPStateR_ * permeability;

            // Cache for coupling
            couplingSampleL_ = outL[i];
            couplingSampleR_ = outR[i];
        }

        // Pitch estimation from zero-crossings (rough but CPU-free)
        if (numSamples > 0)
        {
            zeroCrossingAccum_ = zeroCrossingAccum_ * 0.7f +
                static_cast<float>(crossings) * 0.3f;
            detectedPitch_ = (zeroCrossingAccum_ * static_cast<float>(sr_))
                / (2.0f * static_cast<float>(numSamples));
        }

        // Simple 4-band RMS (approximate via block-rate analysis)
        // LIMITATION (P1-C): Phase 1 uses total RMS distributed to all 4 bands equally.
        // All bandRMS_[0..3] values are identical. DO NOT wire coupling consumers to
        // individual bands until Phase 2 adds real CytomicSVF band-split.
        updateBandRMS(numSamples);

        // Wake silence gate if external audio present
        if (envFollowerL_ > 0.001f || envFollowerR_ > 0.001f)
            wakeSilenceGate();

        // LFO process (for future membrane modulation)
        for (int i = 0; i < numSamples; ++i)
            lfo_.process();

        analyzeForSilenceGate(buffer, numSamples);
    }

    //-- Coupling --------------------------------------------------------------
    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? couplingSampleL_ : couplingSampleR_;
    }

    void applyCouplingInput(CouplingType /*type*/, float /*amount*/,
                            const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        // Phase 1: Osmosis is a source, not a destination.
        // Phase 2 will add membrane perturbation from coupled engines.
    }

    //-- Parameters ------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_permeability", 1}, "Permeability",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_selectivity", 1}, "Selectivity",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_reactivity", 1}, "Reactivity",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_memory", 1}, "Memory",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Macro mapping (M1-M4)
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_macro1", 1}, "PERMEABILITY",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_macro2", 1}, "SELECTIVITY",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_macro3", 1}, "REACTIVITY",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"osmo_macro4", 1}, "MEMORY",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        return layout;
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pPermeability_ = apvts.getRawParameterValue("osmo_permeability");
        pSelectivity_ = apvts.getRawParameterValue("osmo_selectivity");
        pReactivity_ = apvts.getRawParameterValue("osmo_reactivity");
        pMemory_ = apvts.getRawParameterValue("osmo_memory");
    }

private:
    double sr_ = 44100.0;
    int blockSize_ = 512;

    // External audio pointers (set per block by processor, NOT owned)
    const float* externalBufferL_ = nullptr;
    const float* externalBufferR_ = nullptr;
    int externalNumSamples_ = 0;

    // Envelope follower state
    float envFollowerL_ = 0.0f;
    float envFollowerR_ = 0.0f;

    // Pitch detection (zero-crossing)
    float prevSampleL_ = 0.0f;
    // zeroCrossingCount_ removed (P1-D: was declared but never incremented)
    float zeroCrossingAccum_ = 0.0f;
    float detectedPitch_ = 0.0f;

    // 4-band spectral RMS
    float bandRMS_[4] = {};

    // Membrane filter
    float membraneLPCoeff_ = 0.0f;
    float membraneLPStateL_ = 0.0f;
    float membraneLPStateR_ = 0.0f;

    // Coupling output cache
    float couplingSampleL_ = 0.0f;
    float couplingSampleR_ = 0.0f;

    // Parameter pointers
    std::atomic<float>* pPermeability_ = nullptr;
    std::atomic<float>* pSelectivity_ = nullptr;
    std::atomic<float>* pReactivity_ = nullptr;
    std::atomic<float>* pMemory_ = nullptr;

    // Smoothers
    ParameterSmoother permeabilitySmoother_;
    ParameterSmoother selectivitySmoother_;
    ParameterSmoother reactivitySmoother_;
    ParameterSmoother memorySmoother_;

    // LFO
    StandardLFO lfo_;

    void setupBandFilters()
    {
        // Phase 1: simple block-rate RMS, no per-sample filtering
        // Phase 2 will add proper band-split via CytomicSVF
    }

    void updateBandRMS(int numSamples)
    {
        if (!externalBufferL_ || externalNumSamples_ == 0) return;

        // Approximate 4-band energy via simple frequency-dependent RMS
        // Band 0: sub-bass (<200 Hz) — use envelope follower low-pass approximation
        // Band 1: low-mid (200-800 Hz)
        // Band 2: mid (800-3000 Hz)
        // Band 3: high (>3000 Hz)
        // Phase 1: total RMS only, distribute evenly. Phase 2 adds real band-split.
        float totalRMS = 0.0f;
        const int n = std::min(numSamples, externalNumSamples_);
        for (int i = 0; i < n; ++i)
        {
            const float s = externalBufferL_[i];
            totalRMS += s * s;
        }
        totalRMS = std::sqrt(totalRMS / static_cast<float>(std::max(n, 1)));

        // Smooth and distribute (placeholder — Phase 2 will use real band-split)
        for (int b = 0; b < 4; ++b)
            bandRMS_[b] = bandRMS_[b] * 0.8f + totalRMS * 0.2f;
    }
};

} // namespace xomnibus
