// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// ParamSnapshot.h — xoverlap::ParamSnapshot
//
// Caches all 41 olap_ parameter values once per block via update().
// Accessing parameter values through raw pointers avoids per-sample
// tree lookups, which is critical for real-time audio performance.
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
    // update() — call once per block with the live APVTS.
    // Reads every parameter through its raw pointer for zero-overhead access.
    void update(juce::AudioProcessorValueTreeState& apvts) noexcept
    {
        auto getFloat = [&](const char* id, float def) -> float
        {
            auto* p = apvts.getRawParameterValue(id);
            return p ? p->load() : def;
        };
        auto getInt = [&](const char* id, int def) -> int
        {
            auto* p = apvts.getRawParameterValue(id);
            return p ? static_cast<int>(p->load()) : def;
        };

        // Topology
        knot = getInt("olap_knot", 0);
        tangleDepth = getFloat("olap_tangleDepth", 0.4f);
        torusP = getInt("olap_torusP", 3);
        torusQ = getInt("olap_torusQ", 2);

        // Voice
        pulseRate = getFloat("olap_pulseRate", 0.5f);
        entrain = getFloat("olap_entrain", 0.3f);
        spread = getFloat("olap_spread", 0.7f);
        voiceMode = getInt("olap_voiceMode", 0);
        glide = getFloat("olap_glide", 0.0f);

        // FDN
        delayBase = getFloat("olap_delayBase", 10.0f);
        dampening = getFloat("olap_dampening", 0.5f);
        feedback = getFloat("olap_feedback", 0.7f);

        // Timbre
        brightness = getFloat("olap_brightness", 0.5f);
        bioluminescence = getFloat("olap_bioluminescence", 0.2f);
        current = getFloat("olap_current", 0.1f);
        currentRate = getFloat("olap_currentRate", 0.03f);

        // Envelope
        attack = getFloat("olap_attack", 0.05f);
        decay = getFloat("olap_decay", 1.0f);
        sustain = getFloat("olap_sustain", 0.7f);
        release = getFloat("olap_release", 2.0f);

        // Filter
        filterCutoff = getFloat("olap_filterCutoff", 8000.0f);
        filterRes = getFloat("olap_filterRes", 0.1f);
        filterEnvAmt = getFloat("olap_filterEnvAmt", 0.3f);
        filterEnvDecay = getFloat("olap_filterEnvDecay", 0.5f);

        // LFO 1
        lfo1Rate = getFloat("olap_lfo1Rate", 0.8f);
        lfo1Shape = getInt("olap_lfo1Shape", 0);
        lfo1Depth = getFloat("olap_lfo1Depth", 0.3f);
        lfo1Dest = getInt("olap_lfo1Dest", 0);

        // LFO 2
        lfo2Rate = getFloat("olap_lfo2Rate", 0.15f);
        lfo2Shape = getInt("olap_lfo2Shape", 0);
        lfo2Depth = getFloat("olap_lfo2Depth", 0.2f);
        lfo2Dest = getInt("olap_lfo2Dest", 2);

        // Post FX
        chorusMix = getFloat("olap_chorusMix", 0.2f);
        chorusRate = getFloat("olap_chorusRate", 0.08f);
        diffusion = getFloat("olap_diffusion", 0.3f);

        // Macros
        macroKnot = getFloat("olap_macroKnot", 0.3f);
        macroPulse = getFloat("olap_macroPulse", 0.4f);
        macroEntrain = getFloat("olap_macroEntrain", 0.3f);
        macroBloom = getFloat("olap_macroBloom", 0.3f);

        // Expression
        modWheelDest = getInt("olap_modWheelDest", 0);
        modWheelDepth = getFloat("olap_modWheelDepth", 0.5f);
        atPressureDest = getInt("olap_atPressureDest", 1);
        atPressureDepth = getFloat("olap_atPressureDepth", 0.4f);
    }
};

} // namespace xoverlap
