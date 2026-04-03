// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/// ParamSnapshot — cache all 29 oxy_ parameter values once per block.
/// Never query APVTS on the audio thread per-sample; call update() once at
/// the top of processBlock() and then read the plain POD fields throughout.
///
/// F02 fix (2026-03-24): pointer caching. attachParameters() must be called
/// once at construction/plugin-load time to cache all 29 std::atomic<float>*
/// pointers. update() then calls .load(relaxed) directly — no string lookup.
struct ParamSnapshot
{
    // Love components
    float intimacy = 0.3f;
    float passion = 0.25f;
    float commitment = 0.2f;

    // Love envelope rates
    float warmthRate = 0.3f;
    float passionRate = 0.005f;
    float commitRate = 1.0f;

    // Modulation / circuit character
    float entanglement = 0.4f;
    float circuitAge = 0.0f;
    float circuitNoise = 0.05f;

    // Memory
    float memoryDepth = 0.0f;
    float memoryDecay = 5.0f;

    // Topology / topology lock
    int topology = 0;     // 0=SERIES, 1=PARALLEL, 2=FEEDBACK
    int topologyLock = 0; // 0=free, 1-8=love type lock

    // Filter / pitch
    float feedback = 0.3f;
    float cutoff = 8000.0f;
    float pitch = 0.0f;
    float detune = 0.0f;

    // Amplitude ADSR
    float attack = 0.01f;
    float decay = 0.5f;
    float sustain = 0.7f;
    float release = 0.5f;

    // LFO1
    float lfoRate = 1.0f;
    float lfoDepth = 0.0f;
    int lfoShape = 0; // 0=Sine,1=Tri,2=Saw,3=Square,4=S&H

    // LFO2
    float lfo2Rate = 0.067f;
    float lfo2Depth = 0.0f;

    // Output
    float output = 0.0f; // dB
    float pan = 0.0f;
    int voices = 4;

    // ----------------------------------------------------------------
    /// Call once during attachParameters() — caches all 29 raw pointers so
    /// update() performs zero string lookups on the audio thread.
    void attachParameters(juce::AudioProcessorValueTreeState& apvts)
    {
        pIntimacy = apvts.getRawParameterValue("oxy_intimacy");
        pPassion = apvts.getRawParameterValue("oxy_passion");
        pCommitment = apvts.getRawParameterValue("oxy_commitment");
        pWarmthRate = apvts.getRawParameterValue("oxy_warmth_rate");
        pPassionRate = apvts.getRawParameterValue("oxy_passion_rate");
        pCommitRate = apvts.getRawParameterValue("oxy_commit_rate");
        pEntanglement = apvts.getRawParameterValue("oxy_entanglement");
        pCircuitAge = apvts.getRawParameterValue("oxy_circuit_age");
        pCircuitNoise = apvts.getRawParameterValue("oxy_circuit_noise");
        pMemoryDepth = apvts.getRawParameterValue("oxy_memory_depth");
        pMemoryDecay = apvts.getRawParameterValue("oxy_memory_decay");
        pTopology = apvts.getRawParameterValue("oxy_topology");
        pTopologyLock = apvts.getRawParameterValue("oxy_topology_lock");
        pFeedback = apvts.getRawParameterValue("oxy_feedback");
        pCutoff = apvts.getRawParameterValue("oxy_cutoff");
        pPitch = apvts.getRawParameterValue("oxy_pitch");
        pDetune = apvts.getRawParameterValue("oxy_detune");
        pAttack = apvts.getRawParameterValue("oxy_attack");
        pDecay = apvts.getRawParameterValue("oxy_decay");
        pSustain = apvts.getRawParameterValue("oxy_sustain");
        pRelease = apvts.getRawParameterValue("oxy_release");
        pLfoRate = apvts.getRawParameterValue("oxy_lfo_rate");
        pLfoDepth = apvts.getRawParameterValue("oxy_lfo_depth");
        pLfoShape = apvts.getRawParameterValue("oxy_lfo_shape");
        pLfo2Rate = apvts.getRawParameterValue("oxy_lfo2_rate");
        pLfo2Depth = apvts.getRawParameterValue("oxy_lfo2_depth");
        pOutput = apvts.getRawParameterValue("oxy_output");
        pPan = apvts.getRawParameterValue("oxy_pan");
        pVoices = apvts.getRawParameterValue("oxy_voices");
    }

    // ----------------------------------------------------------------
    /// Call once per audio block. Reads all 29 cached atomic<float>* pointers
    /// with memory_order_relaxed — zero string lookups on the audio thread.
    void update()
    {
        auto loadF = [](std::atomic<float>* p, float def) -> float
        { return p ? p->load(std::memory_order_relaxed) : def; };
        auto loadI = [](std::atomic<float>* p, int def) -> int
        { return p ? static_cast<int>(p->load(std::memory_order_relaxed)) : def; };

        intimacy = loadF(pIntimacy, 0.3f);
        passion = loadF(pPassion, 0.25f);
        commitment = loadF(pCommitment, 0.2f);
        warmthRate = loadF(pWarmthRate, 0.3f);
        passionRate = loadF(pPassionRate, 0.005f);
        commitRate = loadF(pCommitRate, 1.0f);
        entanglement = loadF(pEntanglement, 0.4f);
        circuitAge = loadF(pCircuitAge, 0.0f);
        circuitNoise = loadF(pCircuitNoise, 0.05f);
        memoryDepth = loadF(pMemoryDepth, 0.0f);
        memoryDecay = loadF(pMemoryDecay, 5.0f);
        topology = loadI(pTopology, 0);
        topologyLock = loadI(pTopologyLock, 0);
        feedback = loadF(pFeedback, 0.3f);
        cutoff = loadF(pCutoff, 8000.0f);
        pitch = loadF(pPitch, 0.0f);
        detune = loadF(pDetune, 0.0f);
        attack = loadF(pAttack, 0.01f);
        decay = loadF(pDecay, 0.5f);
        sustain = loadF(pSustain, 0.7f);
        release = loadF(pRelease, 0.5f);
        lfoRate = loadF(pLfoRate, 1.0f);
        lfoDepth = loadF(pLfoDepth, 0.0f);
        lfoShape = loadI(pLfoShape, 0);
        lfo2Rate = loadF(pLfo2Rate, 0.067f);
        lfo2Depth = loadF(pLfo2Depth, 0.0f);
        output = loadF(pOutput, 0.0f);
        pan = loadF(pPan, 0.0f);
        voices = loadI(pVoices, 4);
    }

    /// Legacy overload — accepts APVTS reference but ignores it; kept for
    /// call-site compatibility during migration. Use update() after
    /// attachParameters() has been called.
    void update(juce::AudioProcessorValueTreeState& /*apvts*/) { update(); }

private:
    // Cached parameter pointers — set once in attachParameters(), read every block.
    std::atomic<float>* pIntimacy = nullptr;
    std::atomic<float>* pPassion = nullptr;
    std::atomic<float>* pCommitment = nullptr;
    std::atomic<float>* pWarmthRate = nullptr;
    std::atomic<float>* pPassionRate = nullptr;
    std::atomic<float>* pCommitRate = nullptr;
    std::atomic<float>* pEntanglement = nullptr;
    std::atomic<float>* pCircuitAge = nullptr;
    std::atomic<float>* pCircuitNoise = nullptr;
    std::atomic<float>* pMemoryDepth = nullptr;
    std::atomic<float>* pMemoryDecay = nullptr;
    std::atomic<float>* pTopology = nullptr;
    std::atomic<float>* pTopologyLock = nullptr;
    std::atomic<float>* pFeedback = nullptr;
    std::atomic<float>* pCutoff = nullptr;
    std::atomic<float>* pPitch = nullptr;
    std::atomic<float>* pDetune = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pLfoRate = nullptr;
    std::atomic<float>* pLfoDepth = nullptr;
    std::atomic<float>* pLfoShape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pPan = nullptr;
    std::atomic<float>* pVoices = nullptr;
};
