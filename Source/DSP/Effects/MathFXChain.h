// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "EntropyCooler.h"
#include "VoronoiShatter.h"
#include "QuantumSmear.h"
#include "AttractorDrive.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace xoceanus
{

//==============================================================================
// MathFXChain — 4 mathematical FX processors from the XOM-FX-CORE manifesto.
//
// Signal chain (fixed order):
//   1. Entropy Cooler  — Heat Eq + Shannon Entropy + Maxwell's Demon (filter)
//   2. Voronoi Shatter — Fibonacci Spiral + Voronoi + Tensegrity (granular)
//   3. Quantum Smear   — Schrödinger + Airy + Peano Curve (delay/reverb)
//   4. Attractor Drive — Lorenz + Logistic Map (distortion)
//
// Each stage bypasses at zero CPU when mix = 0.
// MIDI CC mapping: CC30 = Entropy Cooler, CC31 = Voronoi, CC32 = Quantum, CC33 = Attractor
//==============================================================================
class MathFXChain
{
public:
    MathFXChain() = default;

    void prepare(double sampleRate)
    {
        entropyCooler.prepare(sampleRate);
        voronoiShatter.prepare(sampleRate);
        quantumSmear.prepare(sampleRate);
        attractorDrive.prepare(sampleRate);
    }

    void reset()
    {
        entropyCooler.reset();
        voronoiShatter.reset();
        quantumSmear.reset();
        attractorDrive.reset();
    }

    //--------------------------------------------------------------------------
    // Process a stereo block in-place. Caller reads all params.
    void processBlock(float* L, float* R, int numSamples,
                      // Entropy Cooler
                      float ecStability, float ecCoolRate, float ecThreshold, float ecMix,
                      // Voronoi Shatter
                      float vsCrystallize, float vsTension, float vsGrainSize, float vsMix,
                      // Quantum Smear
                      float qsObservation, float qsFeedback, float qsDelayCenter, float qsMix,
                      // Attractor Drive
                      float adBifurcation, float adDriveBase, float adSpeed, float adMix)
    {
        juce::ScopedNoDenormals noDenormals; // Issue #902: FTZ/DAZ protection (missed in original Wave 1 chains)
        entropyCooler.processBlock(L, R, numSamples, ecStability, ecCoolRate, ecThreshold, ecMix);
        voronoiShatter.processBlock(L, R, numSamples, vsCrystallize, vsTension, vsGrainSize, vsMix);
        quantumSmear.processBlock(L, R, numSamples, qsObservation, qsFeedback, qsDelayCenter, qsMix);
        attractorDrive.processBlock(L, R, numSamples, adBifurcation, adDriveBase, adSpeed, adMix);
    }

    //--------------------------------------------------------------------------
    // cacheParameterPointers — store atomic pointers for processBlockFromSlot()
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts)
    {
        auto get = [&](const char* id) { return apvts.getRawParameterValue(id); };
        p_ecStability    = get("mfx_ecStability");
        p_ecCoolRate     = get("mfx_ecCoolRate");
        p_ecThreshold    = get("mfx_ecThreshold");
        p_ecMix          = get("mfx_ecMix");
        p_vsCrystallize  = get("mfx_vsCrystallize");
        p_vsTension      = get("mfx_vsTension");
        p_vsGrainSize    = get("mfx_vsGrainSize");
        p_vsMix          = get("mfx_vsMix");
        p_qsObservation  = get("mfx_qsObservation");
        p_qsFeedback     = get("mfx_qsFeedback");
        p_qsDelayCenter  = get("mfx_qsDelayCenter");
        p_qsMix          = get("mfx_qsMix");
        p_adBifurcation  = get("mfx_adBifurcation");
        p_adDriveBase    = get("mfx_adDriveBase");
        p_adSpeed        = get("mfx_adSpeed");
        p_adMix          = get("mfx_adMix");
    }

    //--------------------------------------------------------------------------
    // processBlockFromSlot — reads params from internally-cached pointers.
    void processBlockFromSlot(float* L, float* R, int numSamples)
    {
        auto rd = [](std::atomic<float>* p, float def) {
            return p ? p->load(std::memory_order_relaxed) : def;
        };
        processBlock(L, R, numSamples,
            rd(p_ecStability,   0.5f), rd(p_ecCoolRate,    0.3f),
            rd(p_ecThreshold,   0.5f), rd(p_ecMix,         0.0f),
            rd(p_vsCrystallize, 0.0f), rd(p_vsTension,     0.5f),
            rd(p_vsGrainSize,   30.0f),rd(p_vsMix,         0.0f),
            rd(p_qsObservation, 0.3f), rd(p_qsFeedback,    0.4f),
            rd(p_qsDelayCenter, 150.0f),rd(p_qsMix,        0.0f),
            rd(p_adBifurcation, 0.3f), rd(p_adDriveBase,   0.3f),
            rd(p_adSpeed,       0.3f), rd(p_adMix,         0.0f));
    }

    //--------------------------------------------------------------------------
    // addParameters overload accepting ParameterLayout& (EpicChainSlotController API)
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
    {
        using AP = juce::AudioParameterFloat;
        using NR = juce::NormalisableRange<float>;
        layout.add(std::make_unique<AP>("mfx_ecStability",  "MathFX Entropy Stability",   NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP>("mfx_ecCoolRate",   "MathFX Entropy Cool Rate",    NR(0.0f,1.0f,0.001f),0.3f));
        layout.add(std::make_unique<AP>("mfx_ecThreshold",  "MathFX Entropy Threshold",    NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP>("mfx_ecMix",        "MathFX Entropy Mix",          NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP>("mfx_vsCrystallize","MathFX Voronoi Crystallize",  NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP>("mfx_vsTension",    "MathFX Voronoi Tension",      NR(0.0f,1.0f,0.001f),0.5f));
        layout.add(std::make_unique<AP>("mfx_vsGrainSize",  "MathFX Voronoi Grain Size",   NR(5.0f,100.0f,0.1f,0.4f),30.0f));
        layout.add(std::make_unique<AP>("mfx_vsMix",        "MathFX Voronoi Mix",          NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP>("mfx_qsObservation","MathFX Quantum Observation",  NR(0.0f,1.0f,0.001f),0.3f));
        layout.add(std::make_unique<AP>("mfx_qsFeedback",   "MathFX Quantum Feedback",     NR(0.0f,0.95f,0.001f),0.4f));
        layout.add(std::make_unique<AP>("mfx_qsDelayCenter","MathFX Quantum Delay Center", NR(10.0f,500.0f,0.1f,0.4f),150.0f));
        layout.add(std::make_unique<AP>("mfx_qsMix",        "MathFX Quantum Mix",          NR(0.0f,1.0f,0.001f),0.0f));
        layout.add(std::make_unique<AP>("mfx_adBifurcation","MathFX Attractor Bifurcation",NR(0.0f,1.0f,0.001f),0.3f));
        layout.add(std::make_unique<AP>("mfx_adDriveBase",  "MathFX Attractor Drive Base", NR(0.0f,1.0f,0.001f),0.3f));
        layout.add(std::make_unique<AP>("mfx_adSpeed",      "MathFX Attractor Speed",      NR(0.0f,1.0f,0.001f),0.3f));
        layout.add(std::make_unique<AP>("mfx_adMix",        "MathFX Attractor Mix",        NR(0.0f,1.0f,0.001f),0.0f));
    }

    //--------------------------------------------------------------------------
    // Parameter declarations for APVTS (legacy vector API — kept for MasterFXChain compatibility)
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        // 1. Entropy Cooler (4 params)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_ecStability", 1}, "MathFX Entropy Stability",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_ecCoolRate", 1}, "MathFX Entropy Cool Rate",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_ecThreshold", 1}, "MathFX Entropy Threshold",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_ecMix", 1}, "MathFX Entropy Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // 2. Voronoi Shatter (4 params)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_vsCrystallize", 1}, "MathFX Voronoi Crystallize",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_vsTension", 1}, "MathFX Voronoi Tension",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_vsGrainSize", 1}, "MathFX Voronoi Grain Size",
                                              juce::NormalisableRange<float>(5.0f, 100.0f, 0.1f, 0.4f), 30.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_vsMix", 1}, "MathFX Voronoi Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // 3. Quantum Smear (4 params)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_qsObservation", 1}, "MathFX Quantum Observation",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_qsFeedback", 1}, "MathFX Quantum Feedback",
                                              juce::NormalisableRange<float>(0.0f, 0.95f, 0.001f), 0.4f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_qsDelayCenter", 1}, "MathFX Quantum Delay Center",
                                              juce::NormalisableRange<float>(10.0f, 500.0f, 0.1f, 0.4f), 150.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_qsMix", 1}, "MathFX Quantum Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // 4. Attractor Drive (4 params)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_adBifurcation", 1}, "MathFX Attractor Bifurcation",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_adDriveBase", 1}, "MathFX Attractor Drive Base",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_adSpeed", 1}, "MathFX Attractor Speed",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"mfx_adMix", 1}, "MathFX Attractor Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    }

private:
    EntropyCooler entropyCooler;
    VoronoiShatter voronoiShatter;
    QuantumSmear quantumSmear;
    AttractorDrive attractorDrive;

    // Cached param pointers (populated by cacheParameterPointers)
    std::atomic<float>* p_ecStability   = nullptr;
    std::atomic<float>* p_ecCoolRate    = nullptr;
    std::atomic<float>* p_ecThreshold   = nullptr;
    std::atomic<float>* p_ecMix         = nullptr;
    std::atomic<float>* p_vsCrystallize = nullptr;
    std::atomic<float>* p_vsTension     = nullptr;
    std::atomic<float>* p_vsGrainSize   = nullptr;
    std::atomic<float>* p_vsMix         = nullptr;
    std::atomic<float>* p_qsObservation = nullptr;
    std::atomic<float>* p_qsFeedback    = nullptr;
    std::atomic<float>* p_qsDelayCenter = nullptr;
    std::atomic<float>* p_qsMix         = nullptr;
    std::atomic<float>* p_adBifurcation = nullptr;
    std::atomic<float>* p_adDriveBase   = nullptr;
    std::atomic<float>* p_adSpeed       = nullptr;
    std::atomic<float>* p_adMix         = nullptr;
};

} // namespace xoceanus
