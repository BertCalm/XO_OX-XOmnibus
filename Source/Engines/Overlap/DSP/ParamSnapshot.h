// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// ParamSnapshot.h — xoverlap::ParamSnapshot
//
// Pattern: cachePointers() stores raw atomic<float>* once on the non-audio
// thread (call from attachParameters() or prepare()). update() reads all 41
// values via pointer->load() — zero allocations, zero hash-map lookups on
// the audio thread.
//==============================================================================

#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>

namespace xoverlap
{

//==============================================================================
struct ParamSnapshot
{
    //==========================================================================
    // Topology
    int knot = 0;
    float tangleDepth = 0.4f;
    int torusP = 3;
    int torusQ = 2;

    // Voice
    float pulseRate = 0.5f;
    float entrain = 0.3f;
    float spread = 0.7f;
    int voiceMode = 0;
    float glide = 0.0f;

    // FDN
    float delayBase = 10.0f;
    float dampening = 0.5f;
    float feedback = 0.7f;

    // Timbre
    float brightness = 0.5f;
    float bioluminescence = 0.2f;
    float current = 0.1f;
    float currentRate = 0.03f;

    // Envelope
    float attack = 0.05f;
    float decay = 1.0f;
    float sustain = 0.7f;
    float release = 2.0f;

    // Filter
    float filterCutoff = 8000.0f;
    float filterRes = 0.1f;
    float filterEnvAmt = 0.3f;
    float filterEnvDecay = 0.5f;

    // LFO 1
    float lfo1Rate = 0.8f;
    int lfo1Shape = 0;
    float lfo1Depth = 0.3f;
    int lfo1Dest = 0;

    // LFO 2
    float lfo2Rate = 0.15f;
    int lfo2Shape = 0;
    float lfo2Depth = 0.2f;
    int lfo2Dest = 2;

    // Post FX
    float chorusMix = 0.2f;
    float chorusRate = 0.08f;
    float diffusion = 0.3f;

    // Macros
    float macroKnot = 0.3f;
    float macroPulse = 0.4f;
    float macroEntrain = 0.3f;
    float macroBloom = 0.3f;

    // Expression
    int modWheelDest = 0;
    float modWheelDepth = 0.5f;
    int atPressureDest = 1;
    float atPressureDepth = 0.4f;

    //==========================================================================
    // cachePointers — call once from attachParameters() on the non-audio thread.
    // Stores raw atomic<float>* for every olap_ parameter so update() can read
    // them without any string lookups or allocations.
    void cachePointers(juce::AudioProcessorValueTreeState& apvts)
    {
        // Topology
        p_knot          = apvts.getRawParameterValue("olap_knot");
        p_tangleDepth   = apvts.getRawParameterValue("olap_tangleDepth");
        p_torusP        = apvts.getRawParameterValue("olap_torusP");
        p_torusQ        = apvts.getRawParameterValue("olap_torusQ");

        // Voice
        p_pulseRate  = apvts.getRawParameterValue("olap_pulseRate");
        p_entrain    = apvts.getRawParameterValue("olap_entrain");
        p_spread     = apvts.getRawParameterValue("olap_spread");
        p_voiceMode  = apvts.getRawParameterValue("olap_voiceMode");
        p_glide      = apvts.getRawParameterValue("olap_glide");

        // FDN
        p_delayBase  = apvts.getRawParameterValue("olap_delayBase");
        p_dampening  = apvts.getRawParameterValue("olap_dampening");
        p_feedback   = apvts.getRawParameterValue("olap_feedback");

        // Timbre
        p_brightness      = apvts.getRawParameterValue("olap_brightness");
        p_bioluminescence = apvts.getRawParameterValue("olap_bioluminescence");
        p_current         = apvts.getRawParameterValue("olap_current");
        p_currentRate     = apvts.getRawParameterValue("olap_currentRate");

        // Envelope
        p_attack  = apvts.getRawParameterValue("olap_attack");
        p_decay   = apvts.getRawParameterValue("olap_decay");
        p_sustain = apvts.getRawParameterValue("olap_sustain");
        p_release = apvts.getRawParameterValue("olap_release");

        // Filter
        p_filterCutoff   = apvts.getRawParameterValue("olap_filterCutoff");
        p_filterRes      = apvts.getRawParameterValue("olap_filterRes");
        p_filterEnvAmt   = apvts.getRawParameterValue("olap_filterEnvAmt");
        p_filterEnvDecay = apvts.getRawParameterValue("olap_filterEnvDecay");

        // LFO 1
        p_lfo1Rate  = apvts.getRawParameterValue("olap_lfo1Rate");
        p_lfo1Shape = apvts.getRawParameterValue("olap_lfo1Shape");
        p_lfo1Depth = apvts.getRawParameterValue("olap_lfo1Depth");
        p_lfo1Dest  = apvts.getRawParameterValue("olap_lfo1Dest");

        // LFO 2
        p_lfo2Rate  = apvts.getRawParameterValue("olap_lfo2Rate");
        p_lfo2Shape = apvts.getRawParameterValue("olap_lfo2Shape");
        p_lfo2Depth = apvts.getRawParameterValue("olap_lfo2Depth");
        p_lfo2Dest  = apvts.getRawParameterValue("olap_lfo2Dest");

        // Post FX
        p_chorusMix  = apvts.getRawParameterValue("olap_chorusMix");
        p_chorusRate = apvts.getRawParameterValue("olap_chorusRate");
        p_diffusion  = apvts.getRawParameterValue("olap_diffusion");

        // Macros
        p_macroKnot    = apvts.getRawParameterValue("olap_macroKnot");
        p_macroPulse   = apvts.getRawParameterValue("olap_macroPulse");
        p_macroEntrain = apvts.getRawParameterValue("olap_macroEntrain");
        p_macroBloom   = apvts.getRawParameterValue("olap_macroBloom");

        // Expression
        p_modWheelDest    = apvts.getRawParameterValue("olap_modWheelDest");
        p_modWheelDepth   = apvts.getRawParameterValue("olap_modWheelDepth");
        p_atPressureDest  = apvts.getRawParameterValue("olap_atPressureDest");
        p_atPressureDepth = apvts.getRawParameterValue("olap_atPressureDepth");
    }

    //==========================================================================
    // update — call once per renderBlock on the audio thread.
    // Reads all cached pointers via load(relaxed) — no allocations, no string
    // lookups.
    void update() noexcept
    {
        if (!p_knot)
            return; // cachePointers() not yet called — leave defaults

        auto load    = [](std::atomic<float>* p, float def) -> float
            { return p ? p->load(std::memory_order_relaxed) : def; };
        auto loadInt = [](std::atomic<float>* p, int def) -> int
            { return p ? static_cast<int>(p->load(std::memory_order_relaxed)) : def; };

        // Topology
        knot        = loadInt(p_knot, 0);
        tangleDepth = load(p_tangleDepth, 0.4f);
        torusP      = loadInt(p_torusP, 3);
        torusQ      = loadInt(p_torusQ, 2);

        // Voice
        pulseRate = load(p_pulseRate, 0.5f);
        entrain   = load(p_entrain,   0.3f);
        spread    = load(p_spread,    0.7f);
        voiceMode = loadInt(p_voiceMode, 0);
        glide     = load(p_glide, 0.0f);

        // FDN
        delayBase  = load(p_delayBase,  10.0f);
        dampening  = load(p_dampening,  0.5f);
        feedback   = load(p_feedback,   0.7f);

        // Timbre
        brightness      = load(p_brightness,      0.5f);
        bioluminescence = load(p_bioluminescence, 0.2f);
        current         = load(p_current,         0.1f);
        currentRate     = load(p_currentRate,     0.03f);

        // Envelope
        attack  = load(p_attack,  0.05f);
        decay   = load(p_decay,   1.0f);
        sustain = load(p_sustain, 0.7f);
        release = load(p_release, 2.0f);

        // Filter
        filterCutoff   = load(p_filterCutoff,   8000.0f);
        filterRes      = load(p_filterRes,      0.1f);
        filterEnvAmt   = load(p_filterEnvAmt,   0.3f);
        filterEnvDecay = load(p_filterEnvDecay, 0.5f);

        // LFO 1
        lfo1Rate  = load(p_lfo1Rate,     0.8f);
        lfo1Shape = loadInt(p_lfo1Shape, 0);
        lfo1Depth = load(p_lfo1Depth,    0.3f);
        lfo1Dest  = loadInt(p_lfo1Dest,  0);

        // LFO 2
        lfo2Rate  = load(p_lfo2Rate,     0.15f);
        lfo2Shape = loadInt(p_lfo2Shape, 0);
        lfo2Depth = load(p_lfo2Depth,    0.2f);
        lfo2Dest  = loadInt(p_lfo2Dest,  2);

        // Post FX
        chorusMix  = load(p_chorusMix,  0.2f);
        chorusRate = load(p_chorusRate, 0.08f);
        diffusion  = load(p_diffusion,  0.3f);

        // Macros
        macroKnot    = load(p_macroKnot,    0.3f);
        macroPulse   = load(p_macroPulse,   0.4f);
        macroEntrain = load(p_macroEntrain, 0.3f);
        macroBloom   = load(p_macroBloom,   0.3f);

        // Expression
        modWheelDest    = loadInt(p_modWheelDest,   0);
        modWheelDepth   = load(p_modWheelDepth,     0.5f);
        atPressureDest  = loadInt(p_atPressureDest, 1);
        atPressureDepth = load(p_atPressureDepth,   0.4f);
    }

private:
    //==========================================================================
    // Cached raw pointers — set once in cachePointers(), read each block in
    // update(). All nullptr until cachePointers() is called.

    // Topology
    std::atomic<float>* p_knot        = nullptr;
    std::atomic<float>* p_tangleDepth = nullptr;
    std::atomic<float>* p_torusP      = nullptr;
    std::atomic<float>* p_torusQ      = nullptr;

    // Voice
    std::atomic<float>* p_pulseRate = nullptr;
    std::atomic<float>* p_entrain   = nullptr;
    std::atomic<float>* p_spread    = nullptr;
    std::atomic<float>* p_voiceMode = nullptr;
    std::atomic<float>* p_glide     = nullptr;

    // FDN
    std::atomic<float>* p_delayBase = nullptr;
    std::atomic<float>* p_dampening = nullptr;
    std::atomic<float>* p_feedback  = nullptr;

    // Timbre
    std::atomic<float>* p_brightness      = nullptr;
    std::atomic<float>* p_bioluminescence = nullptr;
    std::atomic<float>* p_current         = nullptr;
    std::atomic<float>* p_currentRate     = nullptr;

    // Envelope
    std::atomic<float>* p_attack  = nullptr;
    std::atomic<float>* p_decay   = nullptr;
    std::atomic<float>* p_sustain = nullptr;
    std::atomic<float>* p_release = nullptr;

    // Filter
    std::atomic<float>* p_filterCutoff   = nullptr;
    std::atomic<float>* p_filterRes      = nullptr;
    std::atomic<float>* p_filterEnvAmt   = nullptr;
    std::atomic<float>* p_filterEnvDecay = nullptr;

    // LFO 1
    std::atomic<float>* p_lfo1Rate  = nullptr;
    std::atomic<float>* p_lfo1Shape = nullptr;
    std::atomic<float>* p_lfo1Depth = nullptr;
    std::atomic<float>* p_lfo1Dest  = nullptr;

    // LFO 2
    std::atomic<float>* p_lfo2Rate  = nullptr;
    std::atomic<float>* p_lfo2Shape = nullptr;
    std::atomic<float>* p_lfo2Depth = nullptr;
    std::atomic<float>* p_lfo2Dest  = nullptr;

    // Post FX
    std::atomic<float>* p_chorusMix  = nullptr;
    std::atomic<float>* p_chorusRate = nullptr;
    std::atomic<float>* p_diffusion  = nullptr;

    // Macros
    std::atomic<float>* p_macroKnot    = nullptr;
    std::atomic<float>* p_macroPulse   = nullptr;
    std::atomic<float>* p_macroEntrain = nullptr;
    std::atomic<float>* p_macroBloom   = nullptr;

    // Expression
    std::atomic<float>* p_modWheelDest    = nullptr;
    std::atomic<float>* p_modWheelDepth   = nullptr;
    std::atomic<float>* p_atPressureDest  = nullptr;
    std::atomic<float>* p_atPressureDepth = nullptr;
};

} // namespace xoverlap
