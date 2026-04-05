// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "AnomalyEngine.h"
#include "DissolvingArchive.h"
#include "ArtifactCathedral.h"
#include "SubmersionEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace xoceanus
{

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

    void prepare(double sampleRate)
    {
        anomaly.prepare(sampleRate);
        dissolving.prepare(sampleRate);
        cathedral.prepare(sampleRate);
        submersion.prepare(sampleRate);
    }

    void reset()
    {
        anomaly.reset();
        dissolving.reset();
        cathedral.reset();
        submersion.reset();
    }

    void processBlock(float* L, float* R, int numSamples,
                      // Anomaly
                      float anTextureBlend, float anReverbSize, float anTremoloRate, bool anTimeSlip, float anSlipSpeed,
                      float anMix,
                      // Dissolving Archive
                      float daChance, float daDissolve, float daGrainMix, float daReverbMix, float daMix,
                      // Artifact Cathedral
                      float acPacketLoss, float acBitCrush, float acDarkMix, float acSunMix, float acModDepth,
                      float acDecay, bool acFreeze, float acMix,
                      // Submersion
                      int smStages, float smLFORate, float smLFODepth, float smLoopFB, float smClock, float smMix)
    {
        anomaly.processBlock(L, R, numSamples, anTextureBlend, anReverbSize, anTremoloRate, anTimeSlip, anSlipSpeed,
                             anMix);
        dissolving.processBlock(L, R, numSamples, daChance, daDissolve, daGrainMix, daReverbMix, daMix);
        cathedral.processBlock(L, R, numSamples, acPacketLoss, acBitCrush, acDarkMix, acSunMix, acModDepth, acDecay,
                               acFreeze, acMix);
        submersion.processBlock(L, R, numSamples, smStages, smLFORate, smLFODepth, smLoopFB, smClock, smMix);
    }

    //--------------------------------------------------------------------------
    // cacheParameterPointers — store atomic pointers for processBlockFromSlot()
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts)
    {
        auto get = [&](const char* id) { return apvts.getRawParameterValue(id); };
        p_anTextureBlend = get("bfx_anTextureBlend");
        p_anReverbSize   = get("bfx_anReverbSize");
        p_anTremoloRate  = get("bfx_anTremoloRate");
        p_anTimeSlip     = get("bfx_anTimeSlip");
        p_anSlipSpeed    = get("bfx_anSlipSpeed");
        p_anMix          = get("bfx_anMix");
        p_daChance       = get("bfx_daChance");
        p_daDissolve     = get("bfx_daDissolve");
        p_daGrainMix     = get("bfx_daGrainMix");
        p_daReverbMix    = get("bfx_daReverbMix");
        p_daMix          = get("bfx_daMix");
        p_acPacketLoss   = get("bfx_acPacketLoss");
        p_acBitCrush     = get("bfx_acBitCrush");
        p_acDarkMix      = get("bfx_acDarkMix");
        p_acSunMix       = get("bfx_acSunMix");
        p_acModDepth     = get("bfx_acModDepth");
        p_acDecay        = get("bfx_acDecay");
        p_acFreeze       = get("bfx_acFreeze");
        p_acMix          = get("bfx_acMix");
        p_smStages       = get("bfx_smStages");
        p_smLFORate      = get("bfx_smLFORate");
        p_smLFODepth     = get("bfx_smLFODepth");
        p_smLoopFB       = get("bfx_smLoopFB");
        p_smClock        = get("bfx_smClock");
        p_smMix          = get("bfx_smMix");
    }

    //--------------------------------------------------------------------------
    // processBlockFromSlot — reads params from internally-cached pointers.
    void processBlockFromSlot(float* L, float* R, int numSamples)
    {
        auto rd  = [](std::atomic<float>* p, float def) {
            return p ? p->load(std::memory_order_relaxed) : def;
        };
        auto rdb = [](std::atomic<float>* p, bool def) -> bool {
            return p ? (p->load(std::memory_order_relaxed) > 0.5f) : def;
        };
        processBlock(L, R, numSamples,
            rd(p_anTextureBlend, 0.5f), rd(p_anReverbSize,  0.5f),
            rd(p_anTremoloRate,  2.0f), rdb(p_anTimeSlip,   false),
            rd(p_anSlipSpeed,    0.0f), rd(p_anMix,         0.0f),
            rd(p_daChance,       0.0f), rd(p_daDissolve,    0.0f),
            rd(p_daGrainMix,     0.5f), rd(p_daReverbMix,   0.3f),
            rd(p_daMix,          0.0f),
            rd(p_acPacketLoss,   0.0f), rd(p_acBitCrush,    0.0f),
            rd(p_acDarkMix,      0.5f), rd(p_acSunMix,      0.5f),
            rd(p_acModDepth,     0.3f), rd(p_acDecay,       0.5f),
            rdb(p_acFreeze,      false),rd(p_acMix,         0.0f),
            static_cast<int>(rd(p_smStages, 4.0f)),
            rd(p_smLFORate,      0.5f), rd(p_smLFODepth,    0.5f),
            rd(p_smLoopFB,       0.3f), rd(p_smClock,       0.5f),
            rd(p_smMix,          0.0f));
    }

    //--------------------------------------------------------------------------
    // addParameters overload accepting ParameterLayout& (EpicChainSlotController API)
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
    {
        using AP = juce::AudioParameterFloat;
        using APB = juce::AudioParameterBool;
        using NR = juce::NormalisableRange<float>;
        layout.add(std::make_unique<AP> ("bfx_anTextureBlend","BFX Anomaly Texture Blend",  NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP> ("bfx_anReverbSize",  "BFX Anomaly Reverb Size",    NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP> ("bfx_anTremoloRate", "BFX Anomaly Tremolo Rate",   NR(0.1f,10.0f,0.01f,0.3f),2.0f));
        layout.add(std::make_unique<APB>("bfx_anTimeSlip",    "BFX Anomaly Time-Slip",      false));
        layout.add(std::make_unique<AP> ("bfx_anSlipSpeed",   "BFX Anomaly Slip Speed",     NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP> ("bfx_anMix",         "BFX Anomaly Mix",            NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP> ("bfx_daChance",      "BFX Archive Chance",         NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP> ("bfx_daDissolve",    "BFX Archive Dissolve",       NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP> ("bfx_daGrainMix",    "BFX Archive Grain Mix",      NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP> ("bfx_daReverbMix",   "BFX Archive Reverb Mix",     NR(0.0f,1.0f,0.001f),0.3f));
        layout.add(std::make_unique<AP> ("bfx_daMix",         "BFX Archive Mix",            NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP> ("bfx_acPacketLoss",  "BFX Cathedral Packet Loss",  NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP> ("bfx_acBitCrush",    "BFX Cathedral Bit Crush",    NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP> ("bfx_acDarkMix",     "BFX Cathedral Dark Mix",     NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP> ("bfx_acSunMix",      "BFX Cathedral Sun Mix",      NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP> ("bfx_acModDepth",    "BFX Cathedral Mod Depth",    NR(0.0f,1.0f,0.001f),0.3f));
        layout.add(std::make_unique<AP> ("bfx_acDecay",       "BFX Cathedral Decay",        NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<APB>("bfx_acFreeze",      "BFX Cathedral Freeze",       false));
        layout.add(std::make_unique<AP> ("bfx_acMix",         "BFX Cathedral Mix",          NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP> ("bfx_smStages",      "BFX Submersion APF Stages",  NR(2.0f,8.0f,1.0f),4.0f));
        layout.add(std::make_unique<AP> ("bfx_smLFORate",     "BFX Submersion LFO Rate",    NR(0.05f,5.0f,0.01f,0.3f),0.5f));
        layout.add(std::make_unique<AP> ("bfx_smLFODepth",    "BFX Submersion LFO Depth",   NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP> ("bfx_smLoopFB",      "BFX Submersion Loop Feedback",NR(0.0f,0.95f,0.001f),0.3f));
        layout.add(std::make_unique<AP> ("bfx_smClock",       "BFX Submersion Master Clock",NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP> ("bfx_smMix",         "BFX Submersion Mix",         NR(0.0f,1.0f,0.001f),0.0f));
    }

    //--------------------------------------------------------------------------
    // Legacy vector API — kept for MasterFXChain compatibility
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PB = juce::AudioParameterBool;

        // 1. Anomaly Engine (CC 40-41) — 6 params
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_anTextureBlend", 1}, "BFX Anomaly Texture Blend",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_anReverbSize", 1}, "BFX Anomaly Reverb Size",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_anTremoloRate", 1}, "BFX Anomaly Tremolo Rate",
                                              juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.3f), 2.0f));
        params.push_back(std::make_unique<PB>(juce::ParameterID{"bfx_anTimeSlip", 1}, "BFX Anomaly Time-Slip", false));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_anSlipSpeed", 1}, "BFX Anomaly Slip Speed",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_anMix", 1}, "BFX Anomaly Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // 2. Dissolving Archive (CC 42-43) — 5 params
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_daChance", 1}, "BFX Archive Chance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_daDissolve", 1}, "BFX Archive Dissolve",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_daGrainMix", 1}, "BFX Archive Grain Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_daReverbMix", 1}, "BFX Archive Reverb Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_daMix", 1}, "BFX Archive Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // 3. Artifact Cathedral (CC 44-45) — 8 params
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_acPacketLoss", 1}, "BFX Cathedral Packet Loss",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_acBitCrush", 1}, "BFX Cathedral Bit Crush",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_acDarkMix", 1}, "BFX Cathedral Dark Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_acSunMix", 1}, "BFX Cathedral Sun Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_acModDepth", 1}, "BFX Cathedral Mod Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_acDecay", 1}, "BFX Cathedral Decay",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PB>(juce::ParameterID{"bfx_acFreeze", 1}, "BFX Cathedral Freeze", false));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_acMix", 1}, "BFX Cathedral Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // 4. Submersion Engine (CC 46-47) — 6 params
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_smStages", 1}, "BFX Submersion APF Stages",
                                              juce::NormalisableRange<float>(2.0f, 8.0f, 1.0f), 4.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_smLFORate", 1}, "BFX Submersion LFO Rate",
                                              juce::NormalisableRange<float>(0.05f, 5.0f, 0.01f, 0.3f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_smLFODepth", 1}, "BFX Submersion LFO Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_smLoopFB", 1}, "BFX Submersion Loop Feedback",
                                              juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_smClock", 1}, "BFX Submersion Master Clock",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"bfx_smMix", 1}, "BFX Submersion Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    }

private:
    AnomalyEngine anomaly;
    DissolvingArchive dissolving;
    ArtifactCathedral cathedral;
    SubmersionEngine submersion;

    // Cached param pointers (populated by cacheParameterPointers)
    std::atomic<float>* p_anTextureBlend = nullptr;
    std::atomic<float>* p_anReverbSize   = nullptr;
    std::atomic<float>* p_anTremoloRate  = nullptr;
    std::atomic<float>* p_anTimeSlip     = nullptr;
    std::atomic<float>* p_anSlipSpeed    = nullptr;
    std::atomic<float>* p_anMix          = nullptr;
    std::atomic<float>* p_daChance       = nullptr;
    std::atomic<float>* p_daDissolve     = nullptr;
    std::atomic<float>* p_daGrainMix     = nullptr;
    std::atomic<float>* p_daReverbMix    = nullptr;
    std::atomic<float>* p_daMix          = nullptr;
    std::atomic<float>* p_acPacketLoss   = nullptr;
    std::atomic<float>* p_acBitCrush     = nullptr;
    std::atomic<float>* p_acDarkMix      = nullptr;
    std::atomic<float>* p_acSunMix       = nullptr;
    std::atomic<float>* p_acModDepth     = nullptr;
    std::atomic<float>* p_acDecay        = nullptr;
    std::atomic<float>* p_acFreeze       = nullptr;
    std::atomic<float>* p_acMix          = nullptr;
    std::atomic<float>* p_smStages       = nullptr;
    std::atomic<float>* p_smLFORate      = nullptr;
    std::atomic<float>* p_smLFODepth     = nullptr;
    std::atomic<float>* p_smLoopFB       = nullptr;
    std::atomic<float>* p_smClock        = nullptr;
    std::atomic<float>* p_smMix          = nullptr;
};

} // namespace xoceanus
