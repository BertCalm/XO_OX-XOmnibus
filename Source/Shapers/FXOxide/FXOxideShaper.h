// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/ShaperEngine.h"
#include "../../DSP/Effects/fXOxide.h"
#include <atomic>

namespace xoceanus
{

//==============================================================================
// FXOxideShaper — Singularity FX insert wrapper for fXOxide.
//
// Exposes OXIDIZE's 6-stage corrosion pipeline as a ShaperEngine insert slot
// so any engine can route through entropy-as-aesthetic degradation.
//
// Age source: envelope follower (silence corrodes, loud stays fresh).
// This differs from the OXIDIZE engine itself, where note lifetime drives age.
//
// Gallery code: FXOXIDE | Accent: Verdigris #4A9E8E | Prefix: fxo_
//
// Coupling output:
//   ch0: current age (0 = pristine, 1 = fully corroded) — drives other engines
//   ch1: mix amount (0-1) — useful for sidechain gating
//
// SilenceGate hold: 500ms (reverb-class — corrosion leaves a decay tail).
//
//==============================================================================
class FXOxideShaper : public ShaperEngine
{
public:
    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        fx_.prepare(sampleRate, maxBlockSize);
        silenceGate.prepare(static_cast<float>(sampleRate), 500); // 500ms — decay tail
        ageCoupling_  = 0.0f;
        mixCoupling_  = 0.0f;
    }

    void releaseResources() override {}

    void reset() override
    {
        fx_.reset();
        ageCoupling_ = 0.0f;
        mixCoupling_ = 0.0f;
    }

    //==========================================================================
    // Audio
    //==========================================================================

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi*/, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0 || isBypassed())
            return;

        // ParamSnapshot — read all atomics once per block
        const float magnitude       = loadP(pMagnitude,      0.5f);
        const float ageSpeed        = loadP(pAgeSpeed,       0.3f);
        const int   corrosionMode   = static_cast<int>(loadP(pCorrosionMode,  0.0f));
        const float corrosionDepth  = loadP(pCorrosionDepth, 0.35f);
        const float erosionFloor    = loadP(pErosionFloor,   200.0f);
        const int   erosionMode     = static_cast<int>(loadP(pErosionMode,    0.0f));
        const float entropyDepth    = loadP(pEntropyDepth,   0.25f);
        const float entropySmooth   = loadP(pEntropySmooth,  0.6f);
        const float wowDepth        = loadP(pWowDepth,       0.20f);
        const float flutterDepth    = loadP(pFlutterDepth,   0.15f);
        const float dropoutRate     = loadP(pDropoutRate,    0.10f);
        const float dropoutSmear    = loadP(pDropoutSmear,   0.50f);
        const float patinaDensity   = loadP(pPatinaDensity,  0.15f);
        const float mix             = loadP(pMix,            0.75f);

        // Coupling modulation applied to magnitude (additive, clamped)
        float effMagnitude = juce::jlimit(0.0f, 1.0f, magnitude + couplingMagnitudeMod_);

        // Push all parameters into fXOxide
        fx_.setMagnitude(effMagnitude);
        fx_.setAgeSpeed(ageSpeed);
        fx_.setCorrosionMode(corrosionMode);
        fx_.setCorrosionDepth(corrosionDepth);
        fx_.setErosionFloor(erosionFloor);
        fx_.setErosionMode(erosionMode);
        fx_.setEntropyDepth(entropyDepth);
        fx_.setEntropySmooth(entropySmooth);
        fx_.setWowDepth(wowDepth);
        fx_.setFlutterDepth(flutterDepth);
        fx_.setDropoutRate(dropoutRate);
        fx_.setDropoutSmear(dropoutSmear);
        fx_.setPatinaDensity(patinaDensity);
        fx_.setMix(mix);

        float* L = buffer.getWritePointer(0);
        float* R = buffer.getNumChannels() >= 2 ? buffer.getWritePointer(1) : nullptr;

        // fXOxide processes stereo in-place; if mono, pass L for both channels
        // (fXOxide writes both pointers so we need a scratch for R-from-L case)
        if (R != nullptr)
        {
            fx_.processBlock(L, R, numSamples);
        }
        else
        {
            // Mono: process L in-place; R pointer is same as L (fXOxide handles null-R
            // gracefully as long as we don't pass nullptr — use L for both)
            fx_.processBlock(L, L, numSamples);
        }

        // Update coupling outputs
        ageCoupling_ = fx_.getCurrentAge();
        mixCoupling_ = mix;

        // Reset per-block coupling accumulator
        couplingMagnitudeMod_ = 0.0f;
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0)
            return ageCoupling_;  // corrosion age — 0=pristine, 1=fully corroded
        if (channel == 1)
            return mixCoupling_;  // current mix level
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float*, int) override
    {
        switch (type)
        {
        case CouplingType::AmpToFilter:
            // A louder coupled signal presses more magnitude into the corrosion chain
            couplingMagnitudeMod_ += amount * 0.25f;
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Parameters — 14 primary (matching fXOxide setter surface)
    //==========================================================================

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        using PF = juce::AudioParameterFloat;
        using PC = juce::AudioParameterChoice;

        auto corrosionChoices = juce::StringArray{"Valve", "Transformer", "BrokenSpeaker", "TapeSat", "Rust", "Acid"};
        auto erosionChoices   = juce::StringArray{"Vinyl (LP)", "Tape (BP)", "Failure (Notch)"};

        // Master (1)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_magnitude", 1}, "FXOxide Magnitude",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        // Envelope follower / age (1)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_age_speed", 1}, "FXOxide Age Speed",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // Corrosion waveshaper (2)
        params.push_back(std::make_unique<PC>(juce::ParameterID{"fxo_corrosion_mode", 1}, "FXOxide Corrosion Mode",
                                              corrosionChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_corrosion_depth", 1}, "FXOxide Corrosion Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));

        // Erosion filter (2)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_erosion_floor", 1}, "FXOxide Erosion Floor",
                                              juce::NormalisableRange<float>(20.0f, 5000.0f, 0.1f, 0.3f), 200.0f));
        params.push_back(std::make_unique<PC>(juce::ParameterID{"fxo_erosion_mode", 1}, "FXOxide Erosion Mode",
                                              erosionChoices, 0));

        // Entropy quantizer (2)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_entropy_depth", 1}, "FXOxide Entropy Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_entropy_smooth", 1}, "FXOxide Entropy Smooth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.6f));

        // Wobble (2)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_wow_depth", 1}, "FXOxide Wow Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_flutter_depth", 1}, "FXOxide Flutter Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

        // Dropout gate (2)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_dropout_rate", 1}, "FXOxide Dropout Rate",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_dropout_smear", 1}, "FXOxide Dropout Smear",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        // Patina noise (1)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_patina_density", 1}, "FXOxide Patina Density",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

        // Output (1)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"fxo_mix", 1}, "FXOxide Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.75f));

        return {params.begin(), params.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pMagnitude      = apvts.getRawParameterValue("fxo_magnitude");
        pAgeSpeed       = apvts.getRawParameterValue("fxo_age_speed");
        pCorrosionMode  = apvts.getRawParameterValue("fxo_corrosion_mode");
        pCorrosionDepth = apvts.getRawParameterValue("fxo_corrosion_depth");
        pErosionFloor   = apvts.getRawParameterValue("fxo_erosion_floor");
        pErosionMode    = apvts.getRawParameterValue("fxo_erosion_mode");
        pEntropyDepth   = apvts.getRawParameterValue("fxo_entropy_depth");
        pEntropySmooth  = apvts.getRawParameterValue("fxo_entropy_smooth");
        pWowDepth       = apvts.getRawParameterValue("fxo_wow_depth");
        pFlutterDepth   = apvts.getRawParameterValue("fxo_flutter_depth");
        pDropoutRate    = apvts.getRawParameterValue("fxo_dropout_rate");
        pDropoutSmear   = apvts.getRawParameterValue("fxo_dropout_smear");
        pPatinaDensity  = apvts.getRawParameterValue("fxo_patina_density");
        pMix            = apvts.getRawParameterValue("fxo_mix");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getShaperId() const override { return "FXOxide"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF4A9E8E); } // Verdigris

private:
    static float loadP(std::atomic<float>* p, float fb) noexcept
    {
        return p ? p->load(std::memory_order_relaxed) : fb;
    }

    fXOxide fx_;

    // Coupling outputs (updated end of each processBlock)
    float ageCoupling_  = 0.0f;
    float mixCoupling_  = 0.0f;

    // Coupling input accumulator (reset each block)
    float couplingMagnitudeMod_ = 0.0f;

    // Parameter pointers
    std::atomic<float>* pMagnitude      = nullptr;
    std::atomic<float>* pAgeSpeed       = nullptr;
    std::atomic<float>* pCorrosionMode  = nullptr;
    std::atomic<float>* pCorrosionDepth = nullptr;
    std::atomic<float>* pErosionFloor   = nullptr;
    std::atomic<float>* pErosionMode    = nullptr;
    std::atomic<float>* pEntropyDepth   = nullptr;
    std::atomic<float>* pEntropySmooth  = nullptr;
    std::atomic<float>* pWowDepth       = nullptr;
    std::atomic<float>* pFlutterDepth   = nullptr;
    std::atomic<float>* pDropoutRate    = nullptr;
    std::atomic<float>* pDropoutSmear   = nullptr;
    std::atomic<float>* pPatinaDensity  = nullptr;
    std::atomic<float>* pMix            = nullptr;
};

} // namespace xoceanus
