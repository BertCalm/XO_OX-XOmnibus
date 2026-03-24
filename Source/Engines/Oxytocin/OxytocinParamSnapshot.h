#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

/// ParamSnapshot — cache all 29 oxy_ parameter values once per block.
/// Never query APVTS on the audio thread per-sample; call update() once at
/// the top of processBlock() and then read the plain POD fields throughout.
struct ParamSnapshot
{
    // Love components
    float intimacy    = 0.3f;
    float passion     = 0.25f;
    float commitment  = 0.2f;

    // Love envelope rates
    float warmthRate  = 0.3f;
    float passionRate = 0.005f;
    float commitRate  = 1.0f;

    // Modulation / circuit character
    float entanglement  = 0.4f;
    float circuitAge    = 0.0f;
    float circuitNoise  = 0.05f;

    // Memory
    float memoryDepth = 0.0f;
    float memoryDecay = 5.0f;

    // Topology / topology lock
    int   topology     = 0;   // 0=SERIES, 1=PARALLEL, 2=FEEDBACK
    int   topologyLock = 0;   // 0=free, 1-8=love type lock

    // Filter / pitch
    float feedback = 0.3f;
    float cutoff   = 8000.0f;
    float pitch    = 0.0f;
    float detune   = 0.0f;

    // Amplitude ADSR
    float attack  = 0.01f;
    float decay   = 0.5f;
    float sustain = 0.7f;
    float release = 0.5f;

    // LFO1
    float lfoRate  = 1.0f;
    float lfoDepth = 0.0f;
    int   lfoShape = 0;   // 0=Sine,1=Tri,2=Saw,3=Square,4=S&H

    // LFO2
    float lfo2Rate  = 0.067f;
    float lfo2Depth = 0.0f;

    // Output
    float output = 0.0f;    // dB
    float pan    = 0.0f;
    int   voices = 4;

    // ----------------------------------------------------------------
    void update (juce::AudioProcessorValueTreeState& apvts)
    {
        // P2-2: use memory_order_relaxed — audio parameters updated on UI thread
        // and read on audio thread do not require sequential consistency.
        // Eliminates memory barriers on ARM / Apple Silicon.
        auto getF = [&](const char* id) -> float
        {
            auto* p = apvts.getRawParameterValue (id);
            return p ? p->load (std::memory_order_relaxed) : 0.0f;
        };
        auto getI = [&](const char* id) -> int
        {
            auto* p = apvts.getRawParameterValue (id);
            return p ? static_cast<int> (p->load (std::memory_order_relaxed)) : 0;
        };

        intimacy    = getF ("oxy_intimacy");
        passion     = getF ("oxy_passion");
        commitment  = getF ("oxy_commitment");
        warmthRate  = getF ("oxy_warmth_rate");
        passionRate = getF ("oxy_passion_rate");
        commitRate  = getF ("oxy_commit_rate");
        entanglement  = getF ("oxy_entanglement");
        circuitAge    = getF ("oxy_circuit_age");
        circuitNoise  = getF ("oxy_circuit_noise");
        memoryDepth   = getF ("oxy_memory_depth");
        memoryDecay   = getF ("oxy_memory_decay");
        topology      = getI ("oxy_topology");
        topologyLock  = getI ("oxy_topology_lock");
        feedback      = getF ("oxy_feedback");
        cutoff        = getF ("oxy_cutoff");
        pitch         = getF ("oxy_pitch");
        detune        = getF ("oxy_detune");
        attack        = getF ("oxy_attack");
        decay         = getF ("oxy_decay");
        sustain       = getF ("oxy_sustain");
        release       = getF ("oxy_release");
        lfoRate       = getF ("oxy_lfo_rate");
        lfoDepth      = getF ("oxy_lfo_depth");
        lfoShape      = getI ("oxy_lfo_shape");
        lfo2Rate      = getF ("oxy_lfo2_rate");
        lfo2Depth     = getF ("oxy_lfo2_depth");
        output        = getF ("oxy_output");
        pan           = getF ("oxy_pan");
        voices        = getI ("oxy_voices");
    }
};
