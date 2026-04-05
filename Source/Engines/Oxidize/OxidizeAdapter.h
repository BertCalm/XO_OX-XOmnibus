// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
// OxidizeAdapter.h — XOceanus SynthEngine adapter for OxidizeEngine
//
// Engine: XOxidize (OXIDIZE) — Engine #77
// Concept: Degradation as synthesis — Second Law of Thermodynamics as a
//          performance interface. Age accumulator drives corrosion, erosion,
//          entropy, wobble, and dropout across 8 polyphonic voices.
// Accent: Verdigris #4A9E8E | Prefix: oxidize_ | Voices: 8
//
// Coupling supported:
//   AmplitudeModulation → couplingAgeBoost (louder source = faster oxidation)
//   FrequencyModulation → couplingWobbleMod (coupling pitch modulates wow/flutter rate)
//   SpectralShaping     → couplingFilterMod (spectral centroid shifts erosion floor)
//   AudioToRing         → ring-modulates corroded voice output
//==============================================================================

#include "../../Core/SynthEngine.h"
#include "OxidizeEngine.h"
#include "OxidizeParamSnapshot.h"

namespace xoceanus
{

class OxidizeAdapter : public SynthEngine
{
public:
    OxidizeAdapter() = default;

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId()    const override { return "Oxidize"; }
    juce::Colour getAccentColour()const override { return juce::Colour(0x4A, 0x9E, 0x8E); } // Verdigris
    int          getMaxVoices()   const override { return OxidizeEngine::MaxVoices; }

    int getActiveVoiceCount() const override { return activeVoiceCount_.load(std::memory_order_relaxed); }

    //-- Lifecycle ---------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        engine_.prepare(sampleRate, maxBlockSize);
        // SRO SilenceGate: polyphonic synth with sediment reverb tail → 500ms hold
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
        sampleRate_     = sampleRate;
        maxBlockSize_   = maxBlockSize;
    }

    void releaseResources() override {}

    void reset() override
    {
        engine_.reset();
        couplingAgeBoost_     = 0.0f;
        couplingWobbleMod_    = 0.0f;
        couplingFilterMod_    = 0.0f;
    }

    //-- Audio -------------------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // Reset per-block coupling accumulators at the START of each block so any
        // applyCouplingInput() calls arriving this block accumulate from zero.
        couplingAgeBoost_   = 0.0f;
        couplingWobbleMod_  = 0.0f;
        couplingFilterMod_  = 0.0f;

        // Wake silence gate on any note-on event
        for (const auto& metadata : midi)
        {
            if (metadata.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }

        // Zero-idle bypass (SRO — eliminates idle CPU when engine is silent)
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // ── Step 1: Update snapshot from APVTS (ParamSnapshot pattern) ────────
        snap_.updateFrom();

        // ── Step 2: Apply macro modulations to snapshot (block-rate) ──────────
        applyMacros();

        // ── Apply coupling modulations to snapshot before DSP ─────────────────
        if (couplingAgeBoost_ > 0.0f)
            engine_.setCouplingAgeBoost(couplingAgeBoost_);

        if (std::abs(couplingWobbleMod_) > 0.001f)
            engine_.setCouplingWobbleMod(couplingWobbleMod_);

        if (std::abs(couplingFilterMod_) > 0.001f)
            engine_.setCouplingFilterMod(couplingFilterMod_);

        // ── Steps 3–9: Delegate to engine ─────────────────────────────────────
        engine_.processBlock(buffer, midi, snap_, numSamples,
                             hostBPM_, hostBeatPos_, hostIsPlaying_);

        // Update active voice count for UI display
        activeVoiceCount_.store(engine_.getActiveVoiceCount(), std::memory_order_relaxed);

        analyzeForSilenceGate(buffer, numSamples);
    }

    //-- Coupling ----------------------------------------------------------------

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        return engine_.getSampleForCoupling(channel, sampleIndex);
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        if (std::abs(amount) < 0.001f || sourceBuffer == nullptr)
            return;

        // Compute RMS of the source block (used by AmplitudeModulation + SpectralShaping)
        float rms = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            rms += sourceBuffer[i] * sourceBuffer[i];
        rms = std::sqrt(rms / static_cast<float>(juce::jmax(1, numSamples)));

        switch (type)
        {
        case CouplingType::AmpToPitch:
            // Coupling RMS boosts age rate — louder source = faster oxidation
            couplingAgeBoost_ += rms * amount;
            break;

        case CouplingType::AudioToFM:
            // Coupling mid-point sample drives wobble rate modulation
            // Use block mid-point as a frequency proxy (avoids full FFT)
            couplingWobbleMod_ += sourceBuffer[numSamples / 2] * amount;
            break;

        case CouplingType::FilterToFilter:
        {
            // Estimate spectral centroid as the ratio of high-energy samples vs total.
            // Lightweight O(n) approximation: count samples above RMS threshold.
            // Result: 0.0 = low centroid (dark), 1.0 = high centroid (bright).
            int aboveThreshold = 0;
            for (int i = 0; i < numSamples; ++i)
                if (std::fabs(sourceBuffer[i]) > rms) ++aboveThreshold;
            float centroid = (numSamples > 0)
                             ? static_cast<float>(aboveThreshold) / static_cast<float>(numSamples)
                             : 0.5f;
            couplingFilterMod_ += (centroid - 0.5f) * 2.0f * amount; // bipolar: [-1, +1]
            break;
        }

        case CouplingType::AudioToRing:
            // Pass source buffer directly to engine for ring modulation in voices
            engine_.setCouplingRingBuffer(sourceBuffer, numSamples, amount);
            break;

        case CouplingType::AmpToFilter:
            // Generic amp → filter: map to erosion floor shift
            couplingFilterMod_ += rms * amount;
            break;

        default:
            // Other coupling types silently accepted — engine-level dispatch
            // handles any coupling type the MegaCouplingMatrix might route here.
            break;
        }
    }

    //-- Parameters --------------------------------------------------------------

    /// Called from XOceanusProcessor::createParameterLayout() to add oxidize_ params.
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return juce::AudioProcessorValueTreeState::ParameterLayout(
            std::make_move_iterator(params.begin()),
            std::make_move_iterator(params.end()));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        snap_.attachTo(apvts);
    }

    //-- Host transport injection -----------------------------------------------
    // Called by the processor (or host bridge) before renderBlock() to deliver
    // DAW transport state for tempo-synced dropout (P2-01).
    // If the host is not playing or no tempo is available, pass bpm=0 and
    // isPlaying=false — the engine falls back to free-running dropout.
    void setHostTransport(float bpm, double beatPos, bool isPlaying) noexcept
    {
        hostBPM_      = bpm;
        hostBeatPos_  = beatPos;
        hostIsPlaying_ = isPlaying;
    }

private:
    //-- Macro application -------------------------------------------------------
    // M1 CHARACTER: patinaDensity, corrosionDepth, corrosionTone
    // M2 MOVEMENT:  ageRate, wowDepth, flutterDepth
    // M3 COUPLING:  entropyDepth, erosionRate, dropoutRate
    // M4 SPACE:     sedimentMix, sedimentTail, dropoutSmear
    //
    // Default macro value = 0.5 → no change.
    // Range: [0, 1] where 0.5 = centre/no-modulation.
    //==========================================================================
    void applyMacros() noexcept
    {
        const float m1 = snap_.macroPATINA;    // M1 CHARACTER
        const float m2 = snap_.macroAGE;       // M2 MOVEMENT
        const float m3 = snap_.macroENTROPY;   // M3 COUPLING
        const float m4 = snap_.macroSEDIMENT;  // M4 SPACE

        // M1 CHARACTER — surface texture and warmth
        const float m1Offset = (m1 - 0.5f) * 2.0f; // [-1, +1]
        snap_.patinaDensity  = std::clamp(snap_.patinaDensity  + m1Offset * 0.3f,  0.0f, 1.0f);
        snap_.corrosionDepth = std::clamp(snap_.corrosionDepth + m1Offset * 0.4f,  0.0f, 1.0f);
        snap_.corrosionTone  = std::clamp(snap_.corrosionTone  + m1Offset * 0.25f, 0.0f, 1.0f);

        // M2 MOVEMENT — aging speed and pitch instability
        const float m2Mult = 0.5f + m2 * 1.5f; // [0.5, 2.0] — scales parameters
        snap_.ageRate    = std::clamp(snap_.ageRate    * m2Mult, 0.0f, 1.0f);
        snap_.wowDepth   = std::clamp(snap_.wowDepth   * m2Mult, 0.0f, 1.0f);
        snap_.flutterDepth = std::clamp(snap_.flutterDepth * m2Mult, 0.0f, 1.0f);

        // M3 COUPLING — degradation depth + dropout intensity
        const float m3Offset = (m3 - 0.5f) * 2.0f; // [-1, +1]
        snap_.entropyDepth = std::clamp(snap_.entropyDepth + m3Offset * 0.4f,  0.0f, 1.0f);
        snap_.erosionRate  = std::clamp(snap_.erosionRate  + m3Offset * 0.35f, 0.0f, 1.0f);
        snap_.dropoutRate  = std::clamp(snap_.dropoutRate  + m3Offset * 0.25f, 0.0f, 1.0f);

        // M4 SPACE — reverb accumulation
        const float m4Offset = (m4 - 0.5f) * 2.0f; // [-1, +1]
        snap_.sedimentMix   = std::clamp(snap_.sedimentMix   + m4Offset * 0.35f, 0.0f, 1.0f);
        snap_.sedimentTail  = std::clamp(snap_.sedimentTail  + m4Offset * 0.4f,  0.0f, 1.0f);
        snap_.dropoutSmear  = std::clamp(snap_.dropoutSmear  + m4Offset * 0.15f, 0.0f, 1.0f);
    }

    //-- Parameter layout --------------------------------------------------------

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF  = juce::AudioParameterFloat;
        using PI  = juce::AudioParameterInt;
        using PB  = juce::AudioParameterBool;
        using PNR = juce::NormalisableRange<float>;

        // ── Oscillator (6) ────────────────────────────────────────────────────
        params.push_back(std::make_unique<PI>(
            juce::ParameterID{"oxidize_waveform", 1}, "Oxidize Waveform", 0, 4, 0));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_tune", 1}, "Oxidize Tune",
            PNR(-24.0f, 24.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_fine", 1}, "Oxidize Fine",
            PNR(-100.0f, 100.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_patinaDensity", 1}, "Oxidize Patina Density",
            PNR(0.0f, 1.0f), 0.15f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_patinaTone", 1}, "Oxidize Patina Tone",
            PNR(0.0f, 1.0f), 0.6f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_oscMix", 1}, "Oxidize Osc Mix",
            PNR(0.0f, 1.0f), 0.8f));

        // ── Aging (6) ─────────────────────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_ageRate", 1}, "Oxidize Age Rate",
            PNR(0.0f, 1.0f), 0.3f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_ageOffset", 1}, "Oxidize Age Offset",
            PNR(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_ageCurve", 1}, "Oxidize Age Curve",
            PNR(-1.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_ageVelSens", 1}, "Oxidize Age Vel Sens",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_preserveAmount", 1}, "Oxidize Preserve",
            PNR(0.1f, 1.0f), 0.85f));

        params.push_back(std::make_unique<PB>(
            juce::ParameterID{"oxidize_reverseAge", 1}, "Oxidize Reverse Age", false));

        // ── Corrosion (4) ─────────────────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_corrosionDepth", 1}, "Oxidize Corrosion",
            PNR(0.0f, 1.0f), 0.35f));

        params.push_back(std::make_unique<PI>(
            juce::ParameterID{"oxidize_corrosionMode", 1}, "Oxidize Corrosion Mode", 0, 5, 0));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_corrosionTone", 1}, "Oxidize Corrosion Tone",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_corrosionVariance", 1}, "Oxidize Corrosion Variance",
            PNR(0.0f, 1.0f), 0.15f));

        // ── Erosion (5) ───────────────────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_erosionRate", 1}, "Oxidize Erosion Rate",
            PNR(0.0f, 1.0f), 0.4f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_erosionFloor", 1}, "Oxidize Erosion Floor",
            PNR(20.0f, 5000.0f, 0.1f, 0.4f), 200.0f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_erosionRes", 1}, "Oxidize Erosion Resonance",
            PNR(0.0f, 1.0f), 0.2f));

        params.push_back(std::make_unique<PI>(
            juce::ParameterID{"oxidize_erosionMode", 1}, "Oxidize Filter Mode", 0, 2, 0));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_erosionVariance", 1}, "Oxidize Erosion Variance",
            PNR(0.0f, 1.0f), 0.1f));

        // ── Entropy (5) ───────────────────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_entropyDepth", 1}, "Oxidize Entropy",
            PNR(0.0f, 1.0f), 0.25f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_entropySmooth", 1}, "Oxidize Entropy Smooth",
            PNR(0.0f, 1.0f), 0.6f));

        params.push_back(std::make_unique<PI>(
            juce::ParameterID{"oxidize_entropyMode", 1}, "Oxidize Entropy Mode", 0, 2, 0));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_entropyBias", 1}, "Oxidize Entropy Bias",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_entropyVariance", 1}, "Oxidize Entropy Variance",
            PNR(0.0f, 1.0f), 0.1f));

        // ── Wobble (5) ────────────────────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_wowDepth", 1}, "Oxidize Wow",
            PNR(0.0f, 1.0f), 0.2f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_wowRate", 1}, "Oxidize Wow Rate",
            PNR(0.01f, 4.0f, 0.001f, 0.5f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_flutterDepth", 1}, "Oxidize Flutter",
            PNR(0.0f, 1.0f), 0.15f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_flutterRate", 1}, "Oxidize Flutter Rate",
            PNR(6.0f, 20.0f, 0.01f), 12.0f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_wobbleSpread", 1}, "Oxidize Wobble Stereo",
            PNR(0.0f, 1.0f), 0.3f));

        // ── Dropout (5) ───────────────────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_dropoutRate", 1}, "Oxidize Dropout Rate",
            PNR(0.0f, 1.0f), 0.1f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_dropoutDepth", 1}, "Oxidize Dropout Depth",
            PNR(0.0f, 1.0f), 0.8f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_dropoutSmear", 1}, "Oxidize Dropout Smear",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_dropoutVariance", 1}, "Oxidize Dropout Variance",
            PNR(0.0f, 1.0f), 0.2f));

        // P2-01: Tempo-sync mode for dropout gate (Kakehashi seance recommendation)
        // 0=Free (default — existing free-running random behaviour, backward compatible)
        // 1=1/8th note grid  2=1/16th note grid  3=1/32nd note grid
        params.push_back(std::make_unique<PI>(
            juce::ParameterID{"oxidize_dropoutSync", 1}, "Oxidize Dropout Sync", 0, 3, 0));

        // ── Sediment Reverb (3) ───────────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_sedimentTail", 1}, "Oxidize Sediment Tail",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_sedimentTone", 1}, "Oxidize Sediment Tone",
            PNR(0.0f, 1.0f), 0.4f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_sedimentMix", 1}, "Oxidize Sediment Mix",
            PNR(0.0f, 1.0f), 0.25f));

        // ── Modulation (4) — D005 floor: 0.005 Hz ────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_lfo1Rate", 1}, "Oxidize LFO1 Rate",
            PNR(0.005f, 20.0f, 0.001f, 0.4f), 0.3f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_lfo1Depth", 1}, "Oxidize LFO1 Depth",
            PNR(0.0f, 1.0f), 0.2f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_lfo2Rate", 1}, "Oxidize LFO2 Rate",
            PNR(0.005f, 20.0f, 0.001f, 0.4f), 1.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_lfo2Depth", 1}, "Oxidize LFO2 Depth",
            PNR(0.0f, 1.0f), 0.1f));

        // ── Expression (4) — D001/D006 ────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_velSens", 1}, "Oxidize Velocity Sens",
            PNR(0.0f, 1.0f), 0.6f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_aftertouch", 1}, "Oxidize Aftertouch",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_modWheel", 1}, "Oxidize Mod Wheel",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_pitchBend", 1}, "Oxidize Pitch Bend",
            PNR(0.0f, 12.0f, 1.0f), 2.0f));

        // ── Macros (4) — D002 required ────────────────────────────────────────
        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_macroPATINA", 1}, "Oxidize CHARACTER",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_macroAGE", 1}, "Oxidize MOVEMENT",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_macroENTROPY", 1}, "Oxidize COUPLING",
            PNR(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(
            juce::ParameterID{"oxidize_macroSEDIMENT", 1}, "Oxidize SPACE",
            PNR(0.0f, 1.0f), 0.5f));
    }

    //-- Members -----------------------------------------------------------------

    OxidizeEngine                           engine_;
    xooxidize::OxidizeParamSnapshot         snap_;
    double                                  sampleRate_    = 44100.0;
    int                                     maxBlockSize_  = 512;

    // Per-block coupling accumulators — set by applyCouplingInput(), consumed + reset at START of renderBlock()
    float couplingAgeBoost_   = 0.0f; // AmplitudeModulation → faster aging
    float couplingWobbleMod_  = 0.0f; // FrequencyModulation → wobble rate mod
    float couplingFilterMod_  = 0.0f; // SpectralShaping → erosion floor shift

    // Host transport state — injected via setHostTransport() before renderBlock().
    // Defaults: bpm=0, beatPos=0, isPlaying=false → engine falls back to free mode.
    float  hostBPM_       = 0.0f;
    double hostBeatPos_   = 0.0;
    bool   hostIsPlaying_ = false;
};

} // namespace xoceanus
