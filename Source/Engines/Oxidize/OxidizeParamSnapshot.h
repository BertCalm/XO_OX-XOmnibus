// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace xooxidize
{

/// OxidizeParamSnapshot — cache all 50 oxidize_ parameter values once per block.
/// Never query APVTS on the audio thread per-sample; call updateFrom() once at
/// the top of processBlock() and then read the plain POD fields throughout.
///
/// Usage:
///   1. Call attachTo(apvts) once during plugin initialisation — caches all
///      std::atomic<float>* pointers (zero string lookups at block time).
///   2. Call updateFrom() at the top of each audio block — loads current values
///      via memory_order_relaxed into plain floats for DSP use.
struct OxidizeParamSnapshot
{
    // -------------------------------------------------------------------------
    // Oscillator (6)
    // -------------------------------------------------------------------------
    int   waveform       = 0;     // 0=Saw, 1=Pulse, 2=Triangle, 3=Noise, 4=Hybrid
    float tune           = 0.0f;  // -24..+24 semitones
    float fine           = 0.0f;  // -100..+100 cents
    float patinaDensity  = 0.15f; // 0-1 — noise osc level (age-modulated)
    float patinaTone     = 0.6f;  // 0-1 — noise spectrum tilt (low=rumble, high=crackle)
    float oscMix         = 0.8f;  // 0-1 — basic osc vs. patina osc balance

    // -------------------------------------------------------------------------
    // Aging (6)
    // -------------------------------------------------------------------------
    float ageRate         = 0.3f;  // 0-1 — speed of aging (0=frozen, 1=30s to full)
    float ageOffset       = 0.0f;  // 0-1 — starting age position (velocity-mappable)
    float ageCurve        = 0.0f;  // -1..+1 — trajectory (-1=log, 0=linear, +1=exp)
    float ageVelSens      = 0.5f;  // 0-1 — velocity → initial age offset depth
    float preserveAmount  = 0.85f; // 0.1-1 — passivation ceiling (max degradation)
    int   reverseAge      = 0;     // 0/1 — Buchla mode: start destroyed, age to pristine

    // -------------------------------------------------------------------------
    // Corrosion (4)
    // -------------------------------------------------------------------------
    float corrosionDepth    = 0.35f; // 0-1 — waveshaper intensity (age-modulated)
    int   corrosionMode     = 0;     // 0=Valve, 1=Transformer, 2=BrokenSpeaker, 3=TapeSat, 4=Rust, 5=Acid
    float corrosionTone     = 0.5f;  // 0-1 — post-saturation tilt EQ
    float corrosionVariance = 0.15f; // 0-1 — randomization depth

    // -------------------------------------------------------------------------
    // Erosion (5)
    // -------------------------------------------------------------------------
    float erosionRate     = 0.4f;   // 0-1 — how fast filter closes with age
    float erosionFloor    = 200.0f; // 20-5000 Hz — minimum cutoff at full age
    float erosionRes      = 0.2f;   // 0-1 — resonant peak at eroding cutoff
    int   erosionMode     = 0;      // 0=Vinyl (smooth), 1=Tape (mid-scoop), 2=Failure (notchy)
    float erosionVariance = 0.1f;   // 0-1 — randomization depth

    // -------------------------------------------------------------------------
    // Entropy (5)
    // -------------------------------------------------------------------------
    float entropyDepth    = 0.25f; // 0-1 — overall degradation depth (age-modulated)
    float entropySmooth   = 0.6f;  // 0-1 — anti-aliasing filter character
    int   entropyMode     = 0;     // 0=Digital (bitcrush), 1=Analog (noise rise), 2=Both
    float entropyBias     = 0.5f;  // 0-1 — digital vs. analog balance (mode=Both)
    float entropyVariance = 0.1f;  // 0-1 — randomization depth

    // -------------------------------------------------------------------------
    // Wobble (5)
    // -------------------------------------------------------------------------
    float wowDepth      = 0.2f;  // 0-1 — slow pitch drift depth (age-modulated)
    float wowRate       = 0.5f;  // 0.01-4 Hz — wow LFO rate
    float flutterDepth  = 0.15f; // 0-1 — fast pitch chatter depth (age-modulated)
    float flutterRate   = 12.0f; // 6-20 Hz — flutter LFO rate
    float wobbleSpread  = 0.3f;  // 0-1 — L/R wobble independence (stereo)

    // -------------------------------------------------------------------------
    // Dropout (4)
    // -------------------------------------------------------------------------
    float dropoutRate     = 0.1f; // 0-1 — probability per block (age-modulated, quadratic)
    float dropoutDepth    = 0.8f; // 0-1 — signal cut depth (1.0 = full silence)
    float dropoutSmear    = 0.5f; // 0-1 — envelope shape (0=hard click, 1=soft tape lift)
    float dropoutVariance = 0.2f; // 0-1 — timing randomization

    // -------------------------------------------------------------------------
    // Sediment Reverb (3)
    // -------------------------------------------------------------------------
    float sedimentTail = 0.5f;  // 0-1 — decay time (T60 floor: 300s at max)
    float sedimentTone = 0.4f;  // 0-1 — reverb spectrum tilt
    float sedimentMix  = 0.25f; // 0-1 — dry/wet (age-modulated: more wet with age)

    // -------------------------------------------------------------------------
    // Modulation (4)
    // -------------------------------------------------------------------------
    float lfo1Rate  = 0.3f;  // 0.005-20 Hz — general purpose LFO (D005 floor: 0.005 Hz)
    float lfo1Depth = 0.2f;  // 0-1 — LFO1 modulation amount
    float lfo2Rate  = 1.5f;  // 0.005-20 Hz — second LFO
    float lfo2Depth = 0.1f;  // 0-1 — LFO2 modulation amount

    // -------------------------------------------------------------------------
    // Expression (4)
    // -------------------------------------------------------------------------
    float velSens    = 0.6f; // 0-1 — velocity → amplitude + age offset
    float aftertouch = 0.5f; // 0-1 — aftertouch → fight entropy (Vangelis)
    float modWheel   = 0.5f; // 0-1 — mod wheel → age rate
    float pitchBend  = 2.0f; // 0-12 semitones — pitch bend range

    // -------------------------------------------------------------------------
    // Macros (4)
    // -------------------------------------------------------------------------
    float macroPATINA  = 0.0f; // M1 CHARACTER — patinaDensity, corrosionDepth, corrosionTone
    float macroAGE     = 0.0f; // M2 MOVEMENT  — ageRate, wowDepth, flutterDepth
    float macroENTROPY = 0.0f; // M3 COUPLING  — entropyDepth, erosionRate, dropoutRate
    float macroSEDIMENT= 0.0f; // M4 SPACE     — sedimentMix, sedimentTail, dropoutRate

    // =========================================================================
    // attachTo() — call ONCE during plugin initialisation.
    // Caches all 50 raw parameter pointers so updateFrom() has zero string
    // lookups on the audio thread.
    // =========================================================================
    void attachTo(juce::AudioProcessorValueTreeState& apvts)
    {
        // Oscillator
        pWaveform       = apvts.getRawParameterValue("oxidize_waveform");
        pTune           = apvts.getRawParameterValue("oxidize_tune");
        pFine           = apvts.getRawParameterValue("oxidize_fine");
        pPatinaDensity  = apvts.getRawParameterValue("oxidize_patinaDensity");
        pPatinaTone     = apvts.getRawParameterValue("oxidize_patinaTone");
        pOscMix         = apvts.getRawParameterValue("oxidize_oscMix");

        // Aging
        pAgeRate         = apvts.getRawParameterValue("oxidize_ageRate");
        pAgeOffset       = apvts.getRawParameterValue("oxidize_ageOffset");
        pAgeCurve        = apvts.getRawParameterValue("oxidize_ageCurve");
        pAgeVelSens      = apvts.getRawParameterValue("oxidize_ageVelSens");
        pPreserveAmount  = apvts.getRawParameterValue("oxidize_preserveAmount");
        pReverseAge      = apvts.getRawParameterValue("oxidize_reverseAge");

        // Corrosion
        pCorrosionDepth    = apvts.getRawParameterValue("oxidize_corrosionDepth");
        pCorrosionMode     = apvts.getRawParameterValue("oxidize_corrosionMode");
        pCorrosionTone     = apvts.getRawParameterValue("oxidize_corrosionTone");
        pCorrosionVariance = apvts.getRawParameterValue("oxidize_corrosionVariance");

        // Erosion
        pErosionRate     = apvts.getRawParameterValue("oxidize_erosionRate");
        pErosionFloor    = apvts.getRawParameterValue("oxidize_erosionFloor");
        pErosionRes      = apvts.getRawParameterValue("oxidize_erosionRes");
        pErosionMode     = apvts.getRawParameterValue("oxidize_erosionMode");
        pErosionVariance = apvts.getRawParameterValue("oxidize_erosionVariance");

        // Entropy
        pEntropyDepth    = apvts.getRawParameterValue("oxidize_entropyDepth");
        pEntropySmooth   = apvts.getRawParameterValue("oxidize_entropySmooth");
        pEntropyMode     = apvts.getRawParameterValue("oxidize_entropyMode");
        pEntropyBias     = apvts.getRawParameterValue("oxidize_entropyBias");
        pEntropyVariance = apvts.getRawParameterValue("oxidize_entropyVariance");

        // Wobble
        pWowDepth     = apvts.getRawParameterValue("oxidize_wowDepth");
        pWowRate      = apvts.getRawParameterValue("oxidize_wowRate");
        pFlutterDepth = apvts.getRawParameterValue("oxidize_flutterDepth");
        pFlutterRate  = apvts.getRawParameterValue("oxidize_flutterRate");
        pWobbleSpread = apvts.getRawParameterValue("oxidize_wobbleSpread");

        // Dropout
        pDropoutRate     = apvts.getRawParameterValue("oxidize_dropoutRate");
        pDropoutDepth    = apvts.getRawParameterValue("oxidize_dropoutDepth");
        pDropoutSmear    = apvts.getRawParameterValue("oxidize_dropoutSmear");
        pDropoutVariance = apvts.getRawParameterValue("oxidize_dropoutVariance");

        // Sediment Reverb
        pSedimentTail = apvts.getRawParameterValue("oxidize_sedimentTail");
        pSedimentTone = apvts.getRawParameterValue("oxidize_sedimentTone");
        pSedimentMix  = apvts.getRawParameterValue("oxidize_sedimentMix");

        // Modulation
        pLfo1Rate  = apvts.getRawParameterValue("oxidize_lfo1Rate");
        pLfo1Depth = apvts.getRawParameterValue("oxidize_lfo1Depth");
        pLfo2Rate  = apvts.getRawParameterValue("oxidize_lfo2Rate");
        pLfo2Depth = apvts.getRawParameterValue("oxidize_lfo2Depth");

        // Expression
        pVelSens    = apvts.getRawParameterValue("oxidize_velSens");
        pAftertouch = apvts.getRawParameterValue("oxidize_aftertouch");
        pModWheel   = apvts.getRawParameterValue("oxidize_modWheel");
        pPitchBend  = apvts.getRawParameterValue("oxidize_pitchBend");

        // Macros
        pMacroPATINA   = apvts.getRawParameterValue("oxidize_macroPATINA");
        pMacroAGE      = apvts.getRawParameterValue("oxidize_macroAGE");
        pMacroENTROPY  = apvts.getRawParameterValue("oxidize_macroENTROPY");
        pMacroSEDIMENT = apvts.getRawParameterValue("oxidize_macroSEDIMENT");
    }

    // =========================================================================
    // updateFrom() — call ONCE per audio block.
    // Reads all 50 cached atomic<float>* pointers with memory_order_relaxed —
    // zero string lookups on the audio thread.
    // Null guard: if pAgeRate is nullptr, attachTo() was never called — return
    // immediately so the engine runs on safe default values.
    // =========================================================================
    void updateFrom()
    {
        if (pAgeRate == nullptr) return; // attachTo() not yet called — use defaults

        auto loadF = [](std::atomic<float>* p, float def) -> float {
            return p ? p->load(std::memory_order_relaxed) : def;
        };
        auto loadI = [](std::atomic<float>* p, int def) -> int {
            return p ? static_cast<int>(p->load(std::memory_order_relaxed)) : def;
        };

        // Oscillator
        waveform       = loadI(pWaveform,       0);
        tune           = loadF(pTune,           0.0f);
        fine           = loadF(pFine,           0.0f);
        patinaDensity  = loadF(pPatinaDensity,  0.15f);
        patinaTone     = loadF(pPatinaTone,     0.6f);
        oscMix         = loadF(pOscMix,         0.8f);

        // Aging
        ageRate        = loadF(pAgeRate,        0.3f);
        ageOffset      = loadF(pAgeOffset,      0.0f);
        ageCurve       = loadF(pAgeCurve,       0.0f);
        ageVelSens     = loadF(pAgeVelSens,     0.5f);
        preserveAmount = loadF(pPreserveAmount, 0.85f);
        reverseAge     = loadI(pReverseAge,     0);

        // Corrosion
        corrosionDepth    = loadF(pCorrosionDepth,    0.35f);
        corrosionMode     = loadI(pCorrosionMode,     0);
        corrosionTone     = loadF(pCorrosionTone,     0.5f);
        corrosionVariance = loadF(pCorrosionVariance, 0.15f);

        // Erosion
        erosionRate     = loadF(pErosionRate,     0.4f);
        erosionFloor    = loadF(pErosionFloor,    200.0f);
        erosionRes      = loadF(pErosionRes,      0.2f);
        erosionMode     = loadI(pErosionMode,     0);
        erosionVariance = loadF(pErosionVariance, 0.1f);

        // Entropy
        entropyDepth    = loadF(pEntropyDepth,    0.25f);
        entropySmooth   = loadF(pEntropySmooth,   0.6f);
        entropyMode     = loadI(pEntropyMode,     0);
        entropyBias     = loadF(pEntropyBias,     0.5f);
        entropyVariance = loadF(pEntropyVariance, 0.1f);

        // Wobble
        wowDepth     = loadF(pWowDepth,     0.2f);
        wowRate      = loadF(pWowRate,      0.5f);
        flutterDepth = loadF(pFlutterDepth, 0.15f);
        flutterRate  = loadF(pFlutterRate,  12.0f);
        wobbleSpread = loadF(pWobbleSpread, 0.3f);

        // Dropout
        dropoutRate     = loadF(pDropoutRate,     0.1f);
        dropoutDepth    = loadF(pDropoutDepth,    0.8f);
        dropoutSmear    = loadF(pDropoutSmear,    0.5f);
        dropoutVariance = loadF(pDropoutVariance, 0.2f);

        // Sediment Reverb
        sedimentTail = loadF(pSedimentTail, 0.5f);
        sedimentTone = loadF(pSedimentTone, 0.4f);
        sedimentMix  = loadF(pSedimentMix,  0.25f);

        // Modulation
        lfo1Rate  = loadF(pLfo1Rate,  0.3f);
        lfo1Depth = loadF(pLfo1Depth, 0.2f);
        lfo2Rate  = loadF(pLfo2Rate,  1.5f);
        lfo2Depth = loadF(pLfo2Depth, 0.1f);

        // Expression
        velSens    = loadF(pVelSens,    0.6f);
        aftertouch = loadF(pAftertouch, 0.5f);
        modWheel   = loadF(pModWheel,   0.5f);
        pitchBend  = loadF(pPitchBend,  2.0f);

        // Macros
        macroPATINA   = loadF(pMacroPATINA,   0.0f);
        macroAGE      = loadF(pMacroAGE,      0.0f);
        macroENTROPY  = loadF(pMacroENTROPY,  0.0f);
        macroSEDIMENT = loadF(pMacroSEDIMENT, 0.0f);
    }

private:
    // =========================================================================
    // Cached parameter pointers — set once in attachTo(), read every block.
    // All initialised to nullptr; updateFrom() null-guards on pAgeRate.
    // =========================================================================

    // Oscillator
    std::atomic<float>* pWaveform       = nullptr;
    std::atomic<float>* pTune           = nullptr;
    std::atomic<float>* pFine           = nullptr;
    std::atomic<float>* pPatinaDensity  = nullptr;
    std::atomic<float>* pPatinaTone     = nullptr;
    std::atomic<float>* pOscMix         = nullptr;

    // Aging
    std::atomic<float>* pAgeRate        = nullptr; // used as null-guard sentinel
    std::atomic<float>* pAgeOffset      = nullptr;
    std::atomic<float>* pAgeCurve       = nullptr;
    std::atomic<float>* pAgeVelSens     = nullptr;
    std::atomic<float>* pPreserveAmount = nullptr;
    std::atomic<float>* pReverseAge     = nullptr;

    // Corrosion
    std::atomic<float>* pCorrosionDepth    = nullptr;
    std::atomic<float>* pCorrosionMode     = nullptr;
    std::atomic<float>* pCorrosionTone     = nullptr;
    std::atomic<float>* pCorrosionVariance = nullptr;

    // Erosion
    std::atomic<float>* pErosionRate     = nullptr;
    std::atomic<float>* pErosionFloor    = nullptr;
    std::atomic<float>* pErosionRes      = nullptr;
    std::atomic<float>* pErosionMode     = nullptr;
    std::atomic<float>* pErosionVariance = nullptr;

    // Entropy
    std::atomic<float>* pEntropyDepth    = nullptr;
    std::atomic<float>* pEntropySmooth   = nullptr;
    std::atomic<float>* pEntropyMode     = nullptr;
    std::atomic<float>* pEntropyBias     = nullptr;
    std::atomic<float>* pEntropyVariance = nullptr;

    // Wobble
    std::atomic<float>* pWowDepth     = nullptr;
    std::atomic<float>* pWowRate      = nullptr;
    std::atomic<float>* pFlutterDepth = nullptr;
    std::atomic<float>* pFlutterRate  = nullptr;
    std::atomic<float>* pWobbleSpread = nullptr;

    // Dropout
    std::atomic<float>* pDropoutRate     = nullptr;
    std::atomic<float>* pDropoutDepth    = nullptr;
    std::atomic<float>* pDropoutSmear    = nullptr;
    std::atomic<float>* pDropoutVariance = nullptr;

    // Sediment Reverb
    std::atomic<float>* pSedimentTail = nullptr;
    std::atomic<float>* pSedimentTone = nullptr;
    std::atomic<float>* pSedimentMix  = nullptr;

    // Modulation
    std::atomic<float>* pLfo1Rate  = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo2Rate  = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;

    // Expression
    std::atomic<float>* pVelSens    = nullptr;
    std::atomic<float>* pAftertouch = nullptr;
    std::atomic<float>* pModWheel   = nullptr;
    std::atomic<float>* pPitchBend  = nullptr;

    // Macros
    std::atomic<float>* pMacroPATINA   = nullptr;
    std::atomic<float>* pMacroAGE      = nullptr;
    std::atomic<float>* pMacroENTROPY  = nullptr;
    std::atomic<float>* pMacroSEDIMENT = nullptr;
};

} // namespace xooxidize
