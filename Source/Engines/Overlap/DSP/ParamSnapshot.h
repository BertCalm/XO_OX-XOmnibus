#pragma once
// XOverlap ParamSnapshot stub — build-only placeholder.
#include <juce_audio_processors/juce_audio_processors.h>

namespace xoverlap {

struct ParamSnapshot {
    float attack = 0.05f, decay = 1.0f, sustain = 0.7f, release = 2.0f;
    float tangleDepth = 0.4f, dampening = 0.5f, pulseRate = 0.5f;
    float delayBase = 10.0f, filterCutoff = 8000.0f, spread = 0.7f;
    int   knot = 0;
    float entrain = 0.3f, feedback = 0.7f, bioluminescence = 0.2f;
    float filterRes = 0.1f, filterEnvAmt = 0.3f, filterEnvDecay = 0.5f;
    float lfo1Rate = 0.8f, lfo2Rate = 0.15f;
    float lfo1Depth = 0.3f, lfo2Depth = 0.2f;
    float chorusMix = 0.2f, chorusRate = 0.08f, diffusion = 0.3f;
    int   torusP = 3, torusQ = 2;
    float current = 0.1f, currentRate = 0.03f;
    int   lfo1Shape = 0, lfo2Shape = 0;
    int   lfo1Dest = 0, lfo2Dest = 2;
    float macroKnot = 0.3f, macroPulse = 0.4f, macroEntrain = 0.3f, macroBloom = 0.3f;
    float modWheelDepth = 0.5f, atPressureDepth = 0.4f;
    int   modWheelDest = 0, atPressureDest = 1;

    void update(juce::AudioProcessorValueTreeState& apvts) {
        auto get = [&](const char* id) -> float {
            auto* p = apvts.getRawParameterValue(id);
            return p ? p->load() : 0.0f;
        };
        attack       = get("olap_attack");
        decay        = get("olap_decay");
        sustain      = get("olap_sustain");
        release      = get("olap_release");
        tangleDepth  = get("olap_tangleDepth");
        dampening    = get("olap_dampening");
        pulseRate    = get("olap_pulseRate");
        delayBase    = get("olap_delayBase");
        filterCutoff = get("olap_filterCutoff");
        filterRes    = get("olap_filterRes");
        filterEnvAmt   = get("olap_filterEnvAmt");
        filterEnvDecay = get("olap_filterEnvDecay");
        spread       = get("olap_spread");
        knot         = static_cast<int>(get("olap_knot"));
        entrain      = get("olap_entrain");
        feedback     = get("olap_feedback");
        bioluminescence = get("olap_bioluminescence");
        lfo1Rate     = get("olap_lfo1Rate");
        lfo2Rate     = get("olap_lfo2Rate");
        lfo1Depth    = get("olap_lfo1Depth");
        lfo2Depth    = get("olap_lfo2Depth");
        chorusMix    = get("olap_chorusMix");
        chorusRate   = get("olap_chorusRate");
        diffusion    = get("olap_diffusion");
        torusP          = static_cast<int>(get("olap_torusP"));
        torusQ          = static_cast<int>(get("olap_torusQ"));
        lfo1Shape       = static_cast<int>(get("olap_lfo1Shape"));
        lfo2Shape       = static_cast<int>(get("olap_lfo2Shape"));
        lfo1Dest        = static_cast<int>(get("olap_lfo1Dest"));
        lfo2Dest        = static_cast<int>(get("olap_lfo2Dest"));
        macroKnot       = get("olap_macroKnot");
        macroPulse      = get("olap_macroPulse");
        macroEntrain    = get("olap_macroEntrain");
        macroBloom      = get("olap_macroBloom");
        modWheelDepth   = get("olap_modWheelDepth");
        atPressureDepth = get("olap_atPressureDepth");
        modWheelDest    = static_cast<int>(get("olap_modWheelDest"));
        atPressureDest  = static_cast<int>(get("olap_atPressureDest"));
    }
};

} // namespace xoverlap
