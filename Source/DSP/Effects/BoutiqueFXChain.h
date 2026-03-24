#pragma once
#include "AnomalyEngine.h"
#include "DissolvingArchive.h"
#include "ArtifactCathedral.h"
#include "SubmersionEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace xolokun {

//==============================================================================
// BoutiqueFXChain — 4 boutique mashup FX processors (XOM-FX-BOUTIQUE).
//
// Inspired by Chase Bliss × Old Blood Noise Endeavors hardware.
// "Environment Generators" — transform inputs into living soundscapes.
//
// Signal chain (fixed order):
//   1. Anomaly Engine  — CB Lost+Found × OBNE Minim (texture + reverb + reverse)
//   2. Dissolving Archive — CB Habit × OBNE Parting (memory well + grains)
//   3. Artifact Cathedral — CB Lossy × OBNE Dark Light (data-rot reverb)
//   4. Submersion Engine — CB MOOD MKII × OBNE Bathing (APF + looper + clock)
//
// MIDI CC: 40-47 (see parameter declarations).
// Each stage bypasses at zero CPU when mix = 0.
//==============================================================================
class BoutiqueFXChain
{
public:
    BoutiqueFXChain() = default;

    void prepare (double sampleRate)
    {
        anomaly.prepare (sampleRate);
        dissolving.prepare (sampleRate);
        cathedral.prepare (sampleRate);
        submersion.prepare (sampleRate);
    }

    void reset()
    {
        anomaly.reset();
        dissolving.reset();
        cathedral.reset();
        submersion.reset();
    }

    void processBlock (float* L, float* R, int numSamples,
                       // Anomaly
                       float anTextureBlend, float anReverbSize, float anTremoloRate,
                       bool anTimeSlip, float anSlipSpeed, float anMix,
                       // Dissolving Archive
                       float daChance, float daDissolve, float daGrainMix,
                       float daReverbMix, float daMix,
                       // Artifact Cathedral
                       float acPacketLoss, float acBitCrush, float acDarkMix,
                       float acSunMix, float acModDepth, float acDecay,
                       bool acFreeze, float acMix,
                       // Submersion
                       int smStages, float smLFORate, float smLFODepth,
                       float smLoopFB, float smClock, float smMix)
    {
        anomaly.processBlock (L, R, numSamples,
            anTextureBlend, anReverbSize, anTremoloRate,
            anTimeSlip, anSlipSpeed, anMix);
        dissolving.processBlock (L, R, numSamples,
            daChance, daDissolve, daGrainMix, daReverbMix, daMix);
        cathedral.processBlock (L, R, numSamples,
            acPacketLoss, acBitCrush, acDarkMix, acSunMix,
            acModDepth, acDecay, acFreeze, acMix);
        submersion.processBlock (L, R, numSamples,
            smStages, smLFORate, smLFODepth, smLoopFB, smClock, smMix);
    }

    //--------------------------------------------------------------------------
    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PB = juce::AudioParameterBool;

        // 1. Anomaly Engine (CC 40-41) — 6 params
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_anTextureBlend", 1 }, "BFX Anomaly Texture Blend",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_anReverbSize", 1 }, "BFX Anomaly Reverb Size",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_anTremoloRate", 1 }, "BFX Anomaly Tremolo Rate",
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.3f), 2.0f));
        params.push_back (std::make_unique<PB> (juce::ParameterID { "bfx_anTimeSlip", 1 }, "BFX Anomaly Time-Slip", false));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_anSlipSpeed", 1 }, "BFX Anomaly Slip Speed",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_anMix", 1 }, "BFX Anomaly Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 2. Dissolving Archive (CC 42-43) — 5 params
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_daChance", 1 }, "BFX Archive Chance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_daDissolve", 1 }, "BFX Archive Dissolve",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_daGrainMix", 1 }, "BFX Archive Grain Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_daReverbMix", 1 }, "BFX Archive Reverb Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_daMix", 1 }, "BFX Archive Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 3. Artifact Cathedral (CC 44-45) — 8 params
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_acPacketLoss", 1 }, "BFX Cathedral Packet Loss",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_acBitCrush", 1 }, "BFX Cathedral Bit Crush",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_acDarkMix", 1 }, "BFX Cathedral Dark Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_acSunMix", 1 }, "BFX Cathedral Sun Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_acModDepth", 1 }, "BFX Cathedral Mod Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_acDecay", 1 }, "BFX Cathedral Decay",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PB> (juce::ParameterID { "bfx_acFreeze", 1 }, "BFX Cathedral Freeze", false));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_acMix", 1 }, "BFX Cathedral Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // 4. Submersion Engine (CC 46-47) — 6 params
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_smStages", 1 }, "BFX Submersion APF Stages",
            juce::NormalisableRange<float> (2.0f, 8.0f, 1.0f), 4.0f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_smLFORate", 1 }, "BFX Submersion LFO Rate",
            juce::NormalisableRange<float> (0.05f, 5.0f, 0.01f, 0.3f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_smLFODepth", 1 }, "BFX Submersion LFO Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_smLoopFB", 1 }, "BFX Submersion Loop Feedback",
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.001f), 0.3f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_smClock", 1 }, "BFX Submersion Master Clock",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<PF> (juce::ParameterID { "bfx_smMix", 1 }, "BFX Submersion Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    }

private:
    AnomalyEngine anomaly;
    DissolvingArchive dissolving;
    ArtifactCathedral cathedral;
    SubmersionEngine submersion;
};

} // namespace xolokun
