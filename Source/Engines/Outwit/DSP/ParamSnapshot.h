// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// ParamSnapshot.h — XOutwit parameter cache (one read per block)
//
// Pattern: attachParameters() caches raw pointers once; update() reads all
// values at the top of renderBlock. No APVTS lookups on the audio thread.
//
// Every field accessed by XOutwitAdapter is present here.
//==============================================================================

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace xoutwit
{

struct ParamSnapshot
{
    //--------------------------------------------------------------------------
    // Per-arm cached values (8 arms)
    //--------------------------------------------------------------------------
    std::array<int, 8> armRule{};
    std::array<int, 8> armLength{};
    std::array<float, 8> armLevel{};
    std::array<int, 8> armPitch{};
    std::array<float, 8> armFilter{};
    std::array<int, 8> armWave{};
    std::array<float, 8> armPan{};

    //--------------------------------------------------------------------------
    // Global — Step/Clock
    //--------------------------------------------------------------------------
    float stepRate = 4.0f;
    bool stepSync = false;
    int stepDiv = 4; // index into division table

    //--------------------------------------------------------------------------
    // Global — Coupling / Intelligence
    //--------------------------------------------------------------------------
    float synapse = 0.2f;
    float chromAmount = 0.5f;
    float solve = 0.0f;
    float huntRate = 0.3f;

    //--------------------------------------------------------------------------
    // SOLVE target DNA
    //--------------------------------------------------------------------------
    float targetBrightness = 0.5f;
    float targetWarmth = 0.5f;
    float targetMovement = 0.5f;
    float targetDensity = 0.5f;
    float targetSpace = 0.5f;
    float targetAggression = 0.5f;

    //--------------------------------------------------------------------------
    // Ink Cloud
    //--------------------------------------------------------------------------
    float inkCloud = 0.0f;
    float inkDecay = 0.08f;

    //--------------------------------------------------------------------------
    // Voice
    //--------------------------------------------------------------------------
    float triggerThresh = 0.3f;
    float masterLevel = 0.8f;

    //--------------------------------------------------------------------------
    // Amp Envelope
    //--------------------------------------------------------------------------
    float ampAttack = 0.01f;
    float ampDecay = 0.2f;
    float ampSustain = 0.8f;
    float ampRelease = 0.3f;

    //--------------------------------------------------------------------------
    // Filter (shared across arms)
    //--------------------------------------------------------------------------
    float filterRes = 0.2f;
    int filterType = 0; // 0=LP, 1=BP, 2=HP

    //--------------------------------------------------------------------------
    // Den Reverb
    //--------------------------------------------------------------------------
    float denSize = 0.4f;
    float denDecay = 0.4f;
    float denMix = 0.2f;

    //--------------------------------------------------------------------------
    // LFOs
    //--------------------------------------------------------------------------
    float lfo1Rate = 1.0f;
    float lfo1Depth = 0.0f;
    int lfo1Shape = 0;
    int lfo1Dest = 0;

    float lfo2Rate = 0.3f;
    float lfo2Depth = 0.0f;
    int lfo2Shape = 0;
    int lfo2Dest = 1;

    //--------------------------------------------------------------------------
    // Voice mode
    //--------------------------------------------------------------------------
    int voiceMode = 0; // 0=Poly, 1=Mono
    float glide = 0.0f;

    //--------------------------------------------------------------------------
    // Macros
    //--------------------------------------------------------------------------
    float macroSolve = 0.0f;         // M1 SOLVE
    float macroSynapse = 0.0f;       // M2 SYNAPSE
    float macroChromatophore = 0.0f; // M3 CHROMATOPHORE
    float macroDen = 0.0f;           // M4 DEN

    //==========================================================================
    // attachParameters — call once from attachParameters() on non-audio thread
    //==========================================================================

    void attachParameters(juce::AudioProcessorValueTreeState& apvts)
    {
        apvts_ptr = &apvts;

        // Per-arm pointers
        for (int n = 0; n < 8; ++n)
        {
            auto idx = static_cast<size_t>(n);
            auto prefix = juce::String("owit_arm") + juce::String(n);

            p_armRule[idx] = apvts.getRawParameterValue(prefix + "Rule");
            p_armLength[idx] = apvts.getRawParameterValue(prefix + "Length");
            p_armLevel[idx] = apvts.getRawParameterValue(prefix + "Level");
            p_armPitch[idx] = apvts.getRawParameterValue(prefix + "Pitch");
            p_armFilter[idx] = apvts.getRawParameterValue(prefix + "Filter");
            p_armWave[idx] = apvts.getRawParameterValue(prefix + "Wave");
            p_armPan[idx] = apvts.getRawParameterValue(prefix + "Pan");
        }

        // Globals
        p_stepRate = apvts.getRawParameterValue("owit_stepRate");
        p_stepSync = apvts.getRawParameterValue("owit_stepSync");
        p_stepDiv = apvts.getRawParameterValue("owit_stepDiv");

        p_synapse = apvts.getRawParameterValue("owit_synapse");
        p_chromAmount = apvts.getRawParameterValue("owit_chromAmount");
        p_solve = apvts.getRawParameterValue("owit_solve");
        p_huntRate = apvts.getRawParameterValue("owit_huntRate");

        p_targetBrightness = apvts.getRawParameterValue("owit_targetBrightness");
        p_targetWarmth = apvts.getRawParameterValue("owit_targetWarmth");
        p_targetMovement = apvts.getRawParameterValue("owit_targetMovement");
        p_targetDensity = apvts.getRawParameterValue("owit_targetDensity");
        p_targetSpace = apvts.getRawParameterValue("owit_targetSpace");
        p_targetAggression = apvts.getRawParameterValue("owit_targetAggression");

        p_inkCloud = apvts.getRawParameterValue("owit_inkCloud");
        p_inkDecay = apvts.getRawParameterValue("owit_inkDecay");

        p_triggerThresh = apvts.getRawParameterValue("owit_triggerThresh");
        p_masterLevel = apvts.getRawParameterValue("owit_masterLevel");

        p_ampAttack = apvts.getRawParameterValue("owit_ampAttack");
        p_ampDecay = apvts.getRawParameterValue("owit_ampDecay");
        p_ampSustain = apvts.getRawParameterValue("owit_ampSustain");
        p_ampRelease = apvts.getRawParameterValue("owit_ampRelease");

        p_filterRes = apvts.getRawParameterValue("owit_filterRes");
        p_filterType = apvts.getRawParameterValue("owit_filterType");

        p_denSize = apvts.getRawParameterValue("owit_denSize");
        p_denDecay = apvts.getRawParameterValue("owit_denDecay");
        p_denMix = apvts.getRawParameterValue("owit_denMix");

        p_lfo1Rate = apvts.getRawParameterValue("owit_lfo1Rate");
        p_lfo1Depth = apvts.getRawParameterValue("owit_lfo1Depth");
        p_lfo1Shape = apvts.getRawParameterValue("owit_lfo1Shape");
        p_lfo1Dest = apvts.getRawParameterValue("owit_lfo1Dest");

        p_lfo2Rate = apvts.getRawParameterValue("owit_lfo2Rate");
        p_lfo2Depth = apvts.getRawParameterValue("owit_lfo2Depth");
        p_lfo2Shape = apvts.getRawParameterValue("owit_lfo2Shape");
        p_lfo2Dest = apvts.getRawParameterValue("owit_lfo2Dest");

        p_voiceMode = apvts.getRawParameterValue("owit_voiceMode");
        p_glide = apvts.getRawParameterValue("owit_glide");

        p_macroSolve = apvts.getRawParameterValue("owit_macroSolve");
        p_macroSynapse = apvts.getRawParameterValue("owit_macroSynapse");
        p_macroChromatophore = apvts.getRawParameterValue("owit_macroChromatophore");
        p_macroDen = apvts.getRawParameterValue("owit_macroDen");
    }

    //==========================================================================
    // update — call once per renderBlock; caches all values atomically
    //==========================================================================

    void update() noexcept
    {
        if (!p_stepRate)
            return; // not yet attached — leave defaults

        for (size_t i = 0; i < 8; ++i)
        {
            armRule[i] = p_armRule[i] ? static_cast<int>(p_armRule[i]->load()) : 110;
            armLength[i] = p_armLength[i] ? static_cast<int>(p_armLength[i]->load()) : 16;
            armLevel[i] = p_armLevel[i] ? p_armLevel[i]->load() : 0.7f;
            armPitch[i] = p_armPitch[i] ? static_cast<int>(p_armPitch[i]->load()) : 0;
            armFilter[i] = p_armFilter[i] ? p_armFilter[i]->load() : 4000.0f;
            armWave[i] = p_armWave[i] ? static_cast<int>(p_armWave[i]->load()) : 0;
            armPan[i] = p_armPan[i] ? p_armPan[i]->load() : 0.0f;
        }

        auto load = [](std::atomic<float>* p, float def) -> float { return p ? p->load() : def; };
        auto loadInt = [](std::atomic<float>* p, int def) -> int { return p ? static_cast<int>(p->load()) : def; };

        stepRate = load(p_stepRate, 4.0f);
        stepSync = load(p_stepSync, 0.0f) > 0.5f;
        stepDiv = loadInt(p_stepDiv, 4);

        synapse = load(p_synapse, 0.2f);
        chromAmount = load(p_chromAmount, 0.5f);
        solve = load(p_solve, 0.0f);
        huntRate = load(p_huntRate, 0.3f);

        targetBrightness = load(p_targetBrightness, 0.5f);
        targetWarmth = load(p_targetWarmth, 0.5f);
        targetMovement = load(p_targetMovement, 0.5f);
        targetDensity = load(p_targetDensity, 0.5f);
        targetSpace = load(p_targetSpace, 0.5f);
        targetAggression = load(p_targetAggression, 0.5f);

        inkCloud = load(p_inkCloud, 0.0f);
        inkDecay = load(p_inkDecay, 0.08f);

        triggerThresh = load(p_triggerThresh, 0.3f);
        masterLevel = load(p_masterLevel, 0.8f);

        ampAttack = load(p_ampAttack, 0.01f);
        ampDecay = load(p_ampDecay, 0.2f);
        ampSustain = load(p_ampSustain, 0.8f);
        ampRelease = load(p_ampRelease, 0.3f);

        filterRes = load(p_filterRes, 0.2f);
        filterType = loadInt(p_filterType, 0);

        denSize = load(p_denSize, 0.4f);
        denDecay = load(p_denDecay, 0.4f);
        denMix = load(p_denMix, 0.2f);

        lfo1Rate = load(p_lfo1Rate, 1.0f);
        lfo1Depth = load(p_lfo1Depth, 0.0f);
        lfo1Shape = loadInt(p_lfo1Shape, 0);
        lfo1Dest = loadInt(p_lfo1Dest, 0);

        lfo2Rate = load(p_lfo2Rate, 0.3f);
        lfo2Depth = load(p_lfo2Depth, 0.0f);
        lfo2Shape = loadInt(p_lfo2Shape, 0);
        lfo2Dest = loadInt(p_lfo2Dest, 1);

        voiceMode = loadInt(p_voiceMode, 0);
        glide = load(p_glide, 0.0f);

        macroSolve = load(p_macroSolve, 0.0f);
        macroSynapse = load(p_macroSynapse, 0.0f);
        macroChromatophore = load(p_macroChromatophore, 0.0f);
        macroDen = load(p_macroDen, 0.0f);
    }

private:
    juce::AudioProcessorValueTreeState* apvts_ptr = nullptr;

    // Per-arm raw pointers
    std::array<std::atomic<float>*, 8> p_armRule{};
    std::array<std::atomic<float>*, 8> p_armLength{};
    std::array<std::atomic<float>*, 8> p_armLevel{};
    std::array<std::atomic<float>*, 8> p_armPitch{};
    std::array<std::atomic<float>*, 8> p_armFilter{};
    std::array<std::atomic<float>*, 8> p_armWave{};
    std::array<std::atomic<float>*, 8> p_armPan{};

    // Global raw pointers
    std::atomic<float>* p_stepRate = nullptr;
    std::atomic<float>* p_stepSync = nullptr;
    std::atomic<float>* p_stepDiv = nullptr;

    std::atomic<float>* p_synapse = nullptr;
    std::atomic<float>* p_chromAmount = nullptr;
    std::atomic<float>* p_solve = nullptr;
    std::atomic<float>* p_huntRate = nullptr;

    std::atomic<float>* p_targetBrightness = nullptr;
    std::atomic<float>* p_targetWarmth = nullptr;
    std::atomic<float>* p_targetMovement = nullptr;
    std::atomic<float>* p_targetDensity = nullptr;
    std::atomic<float>* p_targetSpace = nullptr;
    std::atomic<float>* p_targetAggression = nullptr;

    std::atomic<float>* p_inkCloud = nullptr;
    std::atomic<float>* p_inkDecay = nullptr;

    std::atomic<float>* p_triggerThresh = nullptr;
    std::atomic<float>* p_masterLevel = nullptr;

    std::atomic<float>* p_ampAttack = nullptr;
    std::atomic<float>* p_ampDecay = nullptr;
    std::atomic<float>* p_ampSustain = nullptr;
    std::atomic<float>* p_ampRelease = nullptr;

    std::atomic<float>* p_filterRes = nullptr;
    std::atomic<float>* p_filterType = nullptr;

    std::atomic<float>* p_denSize = nullptr;
    std::atomic<float>* p_denDecay = nullptr;
    std::atomic<float>* p_denMix = nullptr;

    std::atomic<float>* p_lfo1Rate = nullptr;
    std::atomic<float>* p_lfo1Depth = nullptr;
    std::atomic<float>* p_lfo1Shape = nullptr;
    std::atomic<float>* p_lfo1Dest = nullptr;

    std::atomic<float>* p_lfo2Rate = nullptr;
    std::atomic<float>* p_lfo2Depth = nullptr;
    std::atomic<float>* p_lfo2Shape = nullptr;
    std::atomic<float>* p_lfo2Dest = nullptr;

    std::atomic<float>* p_voiceMode = nullptr;
    std::atomic<float>* p_glide = nullptr;

    std::atomic<float>* p_macroSolve = nullptr;
    std::atomic<float>* p_macroSynapse = nullptr;
    std::atomic<float>* p_macroChromatophore = nullptr;
    std::atomic<float>* p_macroDen = nullptr;
};

} // namespace xoutwit
