// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/ModMatrix.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <vector>

namespace xoceanus
{

//==============================================================================
//
//  O N D I N E   E N G I N E
//  Formant Vocal Tract Synthesis
//
//  XO_OX Aquatic Identity: The Ondine — a water spirit whose enchanting voice
//  resonates through cave systems that sing with whispering gallery acoustics.
//  Five formant chambers. The cave sings without excitation at high GALLERY.
//  Gallery code: ONDINE | Accent: Resonant Aqua #00C4CC | Prefix: ond_
//
//  References (D003):
//    Klatt (1980): Software for a cascade/parallel formant synthesizer
//    Fant (1960): Acoustic Theory of Speech Production
//    Kelly & Lochbaum (1962): Speech synthesis from text
//    Rodet (1984): Time-domain formant-wave-function synthesis (FOF)
//    Levin & Edgerton (1999): The Overtone Singing Study Guide
//    Lord Rayleigh (1878): The Theory of Sound — whispering gallery acoustics
//
//  Signal Flow:
//    Excitation Source (buzz/noise/breath/coupled audio)
//        ↓
//    5 Parallel Formant Filters (BPF bank: F1-F5)
//        ↓ (with optional GALLERY circular feedback)
//        ↓ (with optional VOCODE spectral envelope from coupling)
//        ↓ (with optional OVERTONE harmonic isolation)
//    Mix of 5 formants → Output CytomicSVF → VCA → Output
//
//  Coupling:
//    Output: stereo (ch0=L, ch1=R), current vowelX 0-1 (ch2)
//    Input:  AudioToFM      → coupling audio replaces/blends excitation
//            AmpToFilter    → output filter cutoff modulation
//            EnvToMorph     → coupling envelope modulates vowelX
//            RhythmToBlend  → coupling rhythm modulates vowelY
//
//==============================================================================

static constexpr int   kOndineMaxVoices = 8;
static constexpr float kOndineTwoPi     = 6.28318530717958647692f;
static constexpr float kOndinePi        = 3.14159265358979323846f;

// 8 preset vowel targets: {F1, F2, F3, F4, F5} in Hz
// Fant (1960) tables + cave resonance extensions
static constexpr float kOndineVowels[8][5] = {
    {800.0f,  1150.0f, 2900.0f, 3900.0f, 4950.0f},  // 0: A (open)
    {350.0f,  2000.0f, 2800.0f, 3600.0f, 4950.0f},  // 1: E
    {270.0f,  2140.0f, 2950.0f, 3900.0f, 4950.0f},  // 2: I
    {450.0f,  800.0f,  2830.0f, 3800.0f, 4950.0f},  // 3: O
    {325.0f,  700.0f,  2700.0f, 3800.0f, 4950.0f},  // 4: U
    {600.0f,  1000.0f, 2400.0f, 3200.0f, 4200.0f},  // 5: Cave 1 (deep grotto)
    {400.0f,  1400.0f, 3000.0f, 3600.0f, 4800.0f},  // 6: Cave 2 (narrow passage)
    {550.0f,  1200.0f, 2600.0f, 3400.0f, 4600.0f},  // 7: Cave 3 (cathedral cavern)
};

// Formant bandwidths (Hz) per vowel per formant (Fant 1960 reference values)
static constexpr float kOndineFormantBW[8][5] = {
    {90.0f,  110.0f, 170.0f, 250.0f, 300.0f},  // A
    {50.0f,  100.0f, 120.0f, 200.0f, 300.0f},  // E
    {45.0f,  100.0f, 120.0f, 200.0f, 300.0f},  // I
    {50.0f,   70.0f, 160.0f, 220.0f, 280.0f},  // O
    {40.0f,   80.0f, 150.0f, 200.0f, 280.0f},  // U
    {80.0f,  120.0f, 200.0f, 280.0f, 350.0f},  // Cave 1
    {60.0f,  100.0f, 180.0f, 240.0f, 320.0f},  // Cave 2
    {70.0f,  110.0f, 190.0f, 260.0f, 330.0f},  // Cave 3
};

//==============================================================================
// OndineVoice — one vocal tract synthesis voice
//==============================================================================
struct OndineVoice
{
    bool  active    = false;
    bool  releasing = false;
    int   note      = -1;
    float velocity  = 0.0f;  // normalized 0..1
    float keyTrack  = 0.0f;  // (note-60)/60, bipolar

    // ---- Excitation oscillator ----
    float excPhase      = 0.0f;  // phase accumulator [0..1)
    float excPhaseInc   = 0.0f;  // phase increment per sample
    float glideFreq     = 440.0f; // current (smoothed) frequency for glide

    // ---- DC blocker (for gallery feedback path) ----
    float dcBlockX = 0.0f;
    float dcBlockY = 0.0f;

    // ---- Gallery feedback delay (1–2ms) ----
    std::vector<float> galleryDelayBuf;
    int galDelayLen  = 96;   // sized in prepare() from sampleRate
    int galWritePos  = 0;

    // ---- 5 formant BPFs (parallel) ----
    CytomicSVF formant[5];

    // ---- Vocoder analysis bandpass followers (for VOCODE) ----
    CytomicSVF vocoderBP[5];
    float vocoderEnv[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // follower state

    // ---- Output filter (post-formant) ----
    CytomicSVF outputFilterL;
    CytomicSVF outputFilterR;

    // ---- Envelopes ----
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // ---- Per-voice LFOs ----
    StandardLFO lfo1;
    StandardLFO lfo2;

    // ---- Last LFO output (for mod matrix sources) ----
    float lastLfo1Val = 0.0f;
    float lastLfo2Val = 0.0f;

    // ---- Coupling AudioToFM accumulator ----
    float couplingExcAccum = 0.0f;

    // ---- Filter coefficient update counter ----
    int filterUpdateCounter = 0;

    // ---- Vowel auto-morph state ----
    float morphAngle = 0.0f; // current morph angle [0..2pi)

    // ---- Per-voice PRNG (for noise excitation) ----
    uint32_t rng = 12345u;

    void reset(float sampleRate) noexcept
    {
        active    = false;
        releasing = false;
        note      = -1;
        velocity  = 0.0f;
        keyTrack  = 0.0f;

        excPhase    = 0.0f;
        excPhaseInc = 0.0f;
        glideFreq   = 440.0f;

        dcBlockX = 0.0f;
        dcBlockY = 0.0f;

        if (!galleryDelayBuf.empty())
            std::fill(galleryDelayBuf.begin(), galleryDelayBuf.end(), 0.0f);
        galWritePos = 0;

        for (int f = 0; f < 5; ++f)
        {
            formant[f].reset();
            vocoderBP[f].reset();
            vocoderEnv[f] = 0.0f;
        }

        outputFilterL.reset();
        outputFilterR.reset();

        ampEnv.reset();
        filterEnv.reset();
        lfo1.reset();
        lfo2.reset();
        lastLfo1Val = 0.0f;
        lastLfo2Val = 0.0f;
        couplingExcAccum = 0.0f;
        filterUpdateCounter = 0;
        morphAngle = 0.0f;
    }
};

//==============================================================================
//
//  OndineEngine — Formant Vocal Tract Synthesis
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention — .cpp is a one-line stub.
//
//  Parameter prefix: ond_
//  Gallery accent:   Resonant Aqua #00C4CC
//
//==============================================================================
class OndineEngine : public SynthEngine
{
public:

    //==========================================================================
    //  P A R A M E T E R   R E G I S T R A T I O N
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using AP  = juce::AudioParameterFloat;
        using APC = juce::AudioParameterChoice;
        using PID = juce::ParameterID;
        using NR  = juce::NormalisableRange<float>;

        // ---- A: Excitation Source (5 params) ----
        params.push_back(std::make_unique<APC>(PID{"ond_excType",1}, "Ondine Exc Type",
            juce::StringArray{"Buzz","Noise","Breath","Coupled"}, 0));

        params.push_back(std::make_unique<AP>(PID{"ond_buzzChar",1}, "Ondine Buzz Char",
            NR{0.0f, 1.0f, 0.001f}, 0.3f));

        params.push_back(std::make_unique<AP>(PID{"ond_breathiness",1}, "Ondine Breathiness",
            NR{0.0f, 1.0f, 0.001f}, 0.15f));

        params.push_back(std::make_unique<AP>(PID{"ond_excLevel",1}, "Ondine Exc Level",
            NR{0.0f, 1.0f, 0.001f}, 0.8f));

        params.push_back(std::make_unique<AP>(PID{"ond_excBright",1}, "Ondine Exc Bright",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        // ---- B: Formant Bank (8 params) ----
        params.push_back(std::make_unique<AP>(PID{"ond_vowelX",1}, "Ondine Vowel X",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ond_vowelY",1}, "Ondine Vowel Y",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ond_formantShift",1}, "Ondine Formant Shift",
            NR{-2000.0f, 2000.0f, 0.5f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ond_formantSpread",1}, "Ondine Formant Spread",
            NR{0.5f, 2.0f, 0.001f}, 1.0f));

        params.push_back(std::make_unique<AP>(PID{"ond_formantTilt",1}, "Ondine Formant Tilt",
            NR{-1.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ond_formantQ",1}, "Ondine Formant Q",
            NR{0.1f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ond_vowelMorphRate",1}, "Ondine Vowel Morph Rate",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ond_vowelMorphDepth",1}, "Ondine Vowel Morph Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        // ---- C: FX (3 params) ----
        params.push_back(std::make_unique<AP>(PID{"ond_vocode",1}, "Ondine Vocode",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ond_overtone",1}, "Ondine Overtone",
            NR{0.0f, 16.0f, 0.01f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ond_gallery",1}, "Ondine Gallery",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        // ---- D: Amp Envelope (4 params) ----
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_ampAtk",1}, "Ondine Amp Atk", r, 0.01f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_ampDec",1}, "Ondine Amp Dec", r, 0.3f));
        }
        params.push_back(std::make_unique<AP>(PID{"ond_ampSus",1}, "Ondine Amp Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.7f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_ampRel",1}, "Ondine Amp Rel", r, 0.5f));
        }

        // ---- E: Filter Envelope + Output Filter (8 params) ----
        {
            NR r{20.0f, 20000.0f, 0.1f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_fltCutoff",1}, "Ondine Flt Cutoff", r, 12000.0f));
        }
        params.push_back(std::make_unique<AP>(PID{"ond_fltReso",1}, "Ondine Flt Reso",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<APC>(PID{"ond_fltType",1}, "Ondine Flt Type",
            juce::StringArray{"LP","HP","BP","Notch"}, 0));

        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_fenvAtk",1}, "Ondine Fenv Atk", r, 0.01f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_fenvDec",1}, "Ondine Fenv Dec", r, 0.3f));
        }
        params.push_back(std::make_unique<AP>(PID{"ond_fenvSus",1}, "Ondine Fenv Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_fenvRel",1}, "Ondine Fenv Rel", r, 0.4f));
        }
        params.push_back(std::make_unique<AP>(PID{"ond_fenvAmt",1}, "Ondine Fenv Amt",
            NR{-1.0f, 1.0f, 0.001f}, 0.3f));

        // ---- F: LFOs (8 params) ----
        static const juce::StringArray kLFOShapes  {"Sine","Tri","Saw","Square","S&H"};
        static const juce::StringArray kLFO1Targets{"VowelX","VowelY","FormantShift","Breathiness"};
        static const juce::StringArray kLFO2Targets{"VowelX","VowelY","FormantShift","Breathiness"};

        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_lfo1Rate",1}, "Ondine LFO1 Rate", r, 0.5f));
        }
        params.push_back(std::make_unique<AP>(PID{"ond_lfo1Depth",1}, "Ondine LFO1 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"ond_lfo1Shape",1}, "Ondine LFO1 Shape",
            kLFOShapes, 0));
        params.push_back(std::make_unique<APC>(PID{"ond_lfo1Target",1}, "Ondine LFO1 Target",
            kLFO1Targets, 0)); // VowelX

        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ond_lfo2Rate",1}, "Ondine LFO2 Rate", r, 0.15f));
        }
        params.push_back(std::make_unique<AP>(PID{"ond_lfo2Depth",1}, "Ondine LFO2 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"ond_lfo2Shape",1}, "Ondine LFO2 Shape",
            kLFOShapes, 1)); // Triangle
        params.push_back(std::make_unique<APC>(PID{"ond_lfo2Target",1}, "Ondine LFO2 Target",
            kLFO2Targets, 1)); // VowelY

        // ---- G: Mod Matrix (4 slots x 3 params = 12 params) ----
        static const juce::StringArray kOndineModDests {
            "Off", "Filter Cutoff", "Vowel X", "Vowel Y", "Formant Shift", "Gallery", "Amp Level"
        };
        ModMatrix<4>::addParameters(params, "ond_", "Ondine", kOndineModDests);

        // ---- H: Macros + Voice (7 params) ----
        // M1=APERTURE: vowelY + formantSpread
        params.push_back(std::make_unique<AP>(PID{"ond_macro1",1}, "Ondine Macro1",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));
        // M2=TIDE: vowelMorphRate + vowelMorphDepth
        params.push_back(std::make_unique<AP>(PID{"ond_macro2",1}, "Ondine Macro2",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M3=VOCODE: vocoder depth
        params.push_back(std::make_unique<AP>(PID{"ond_macro3",1}, "Ondine Macro3",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M4=SPACE: output filter cutoff + width
        params.push_back(std::make_unique<AP>(PID{"ond_macro4",1}, "Ondine Macro4",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<APC>(PID{"ond_voiceMode",1}, "Ondine Voice Mode",
            juce::StringArray{"Mono","Legato","Poly4","Poly8"}, 2));

        {
            NR r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"ond_glide",1}, "Ondine Glide", r, 0.0f));
        }

        params.push_back(std::make_unique<AP>(PID{"ond_velTimbre",1}, "Ondine Vel Timbre",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return {params.begin(), params.end()};
    }

    //==========================================================================
    //  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateFloat = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 44100.0f;
        maxBlock = maxBlockSize;

        // Gallery feedback delay: ~1.5ms at current sample rate
        const int galDelayLen = std::max(2, static_cast<int>(sampleRateFloat * 0.0015f));

        for (int i = 0; i < kOndineMaxVoices; ++i)
        {
            auto& v = voices[i];
            v.galleryDelayBuf.assign(static_cast<size_t>(galDelayLen), 0.0f);
            v.galDelayLen = galDelayLen;
            v.rng = 12345u + static_cast<uint32_t>(i) * 31337u;
            v.reset(sampleRateFloat);
        }

        // Coupling buffers
        couplingExcBuf.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        couplingAmpFilter   = 0.0f;
        couplingEnvVowelX   = 0.0f;
        couplingRhythmVowelY= 0.0f;
        couplingExcLevel    = 0.0f;

        modWheelValue   = 0.0f;
        aftertouchValue = 0.0f;

        lastSampleL   = 0.0f;
        lastSampleR   = 0.0f;
        lastVowelX    = 0.5f;

        activeVoiceCount_.store(0, std::memory_order_relaxed);

        // SilenceGate — formant tails can ring a bit
        prepareSilenceGate(sampleRate, maxBlockSize, 300.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset(sampleRateFloat);

        std::fill(couplingExcBuf.begin(), couplingExcBuf.end(), 0.0f);
        couplingAmpFilter    = 0.0f;
        couplingEnvVowelX    = 0.0f;
        couplingRhythmVowelY = 0.0f;
        couplingExcLevel     = 0.0f;
        modWheelValue        = 0.0f;
        aftertouchValue      = 0.0f;
        lastSampleL          = 0.0f;
        lastSampleR          = 0.0f;
        lastVowelX           = 0.5f;
        activeVoiceCount_.store(0, std::memory_order_relaxed);
    }

    //==========================================================================
    //  C O U P L I N G
    //==========================================================================

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        if (sourceBuffer == nullptr || numSamples <= 0)
            return;

        switch (type)
        {
        case CouplingType::AudioToFM:
        {
            // Coupling audio replaces/blends with internal excitation
            const int copyLen = std::min(numSamples, maxBlock);
            for (int i = 0; i < copyLen; ++i)
                couplingExcBuf[static_cast<size_t>(i)] = sourceBuffer[i] * amount;
            couplingExcLevel = amount;
            for (auto& v : voices)
                if (v.active)
                    v.couplingExcAccum = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::AmpToFilter:
        {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter = rms * amount;
            break;
        }
        case CouplingType::EnvToMorph:
        {
            // Coupling envelope modulates vowelX
            couplingEnvVowelX = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::RhythmToBlend:
        {
            // Coupling rhythm modulates vowelY
            couplingRhythmVowelY = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        default:
        {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter += rms * amount;
            break;
        }
        }
    }

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return lastSampleL;
        if (channel == 1) return lastSampleR;
        if (channel == 2) return lastVowelX;   // current vowelX 0-1
        return 0.0f;
    }

    //==========================================================================
    //  P A R A M E T E R   A T T A C H M E N T
    //==========================================================================

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Group A
        pExcType      = apvts.getRawParameterValue("ond_excType");
        pBuzzChar     = apvts.getRawParameterValue("ond_buzzChar");
        pBreathiness  = apvts.getRawParameterValue("ond_breathiness");
        pExcLevel     = apvts.getRawParameterValue("ond_excLevel");
        pExcBright    = apvts.getRawParameterValue("ond_excBright");

        // Group B
        pVowelX         = apvts.getRawParameterValue("ond_vowelX");
        pVowelY         = apvts.getRawParameterValue("ond_vowelY");
        pFormantShift   = apvts.getRawParameterValue("ond_formantShift");
        pFormantSpread  = apvts.getRawParameterValue("ond_formantSpread");
        pFormantTilt    = apvts.getRawParameterValue("ond_formantTilt");
        pFormantQ       = apvts.getRawParameterValue("ond_formantQ");
        pVowelMorphRate = apvts.getRawParameterValue("ond_vowelMorphRate");
        pVowelMorphDepth= apvts.getRawParameterValue("ond_vowelMorphDepth");

        // Group C
        pVocode   = apvts.getRawParameterValue("ond_vocode");
        pOvertone = apvts.getRawParameterValue("ond_overtone");
        pGallery  = apvts.getRawParameterValue("ond_gallery");

        // Group D
        pAmpAtk = apvts.getRawParameterValue("ond_ampAtk");
        pAmpDec = apvts.getRawParameterValue("ond_ampDec");
        pAmpSus = apvts.getRawParameterValue("ond_ampSus");
        pAmpRel = apvts.getRawParameterValue("ond_ampRel");

        // Group E
        pFltCutoff = apvts.getRawParameterValue("ond_fltCutoff");
        pFltReso   = apvts.getRawParameterValue("ond_fltReso");
        pFltType   = apvts.getRawParameterValue("ond_fltType");
        pFenvAtk   = apvts.getRawParameterValue("ond_fenvAtk");
        pFenvDec   = apvts.getRawParameterValue("ond_fenvDec");
        pFenvSus   = apvts.getRawParameterValue("ond_fenvSus");
        pFenvRel   = apvts.getRawParameterValue("ond_fenvRel");
        pFenvAmt   = apvts.getRawParameterValue("ond_fenvAmt");

        // Group F
        pLfo1Rate   = apvts.getRawParameterValue("ond_lfo1Rate");
        pLfo1Depth  = apvts.getRawParameterValue("ond_lfo1Depth");
        pLfo1Shape  = apvts.getRawParameterValue("ond_lfo1Shape");
        pLfo1Target = apvts.getRawParameterValue("ond_lfo1Target");
        pLfo2Rate   = apvts.getRawParameterValue("ond_lfo2Rate");
        pLfo2Depth  = apvts.getRawParameterValue("ond_lfo2Depth");
        pLfo2Shape  = apvts.getRawParameterValue("ond_lfo2Shape");
        pLfo2Target = apvts.getRawParameterValue("ond_lfo2Target");

        // Group G
        modMatrix.attachParameters(apvts, "ond_");

        // Group H
        pMacro1    = apvts.getRawParameterValue("ond_macro1");
        pMacro2    = apvts.getRawParameterValue("ond_macro2");
        pMacro3    = apvts.getRawParameterValue("ond_macro3");
        pMacro4    = apvts.getRawParameterValue("ond_macro4");
        pVoiceMode = apvts.getRawParameterValue("ond_voiceMode");
        pGlide     = apvts.getRawParameterValue("ond_glide");
        pVelTimbre = apvts.getRawParameterValue("ond_velTimbre");
    }

    //==========================================================================
    //  I D E N T I T Y
    //==========================================================================

    juce::String  getEngineId()     const override { return "Ondine"; }
    juce::Colour  getAccentColour() const override { return juce::Colour(0xFF00C4CC); } // Resonant Aqua
    int           getMaxVoices()    const override { return kOndineMaxVoices; }

    //==========================================================================
    //  R E N D E R   B L O C K
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (sampleRateFloat <= 0.0f) return;
        if (numSamples <= 0) return;

        // ---- SilenceGate: wake on note-on, bail if silent ----
        for (const auto& md : midi)
        {
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // ---- Snapshot parameters (block-rate) ----
        const float macro1 = pMacro1 ? pMacro1->load() : 0.5f;
        const float macro2 = pMacro2 ? pMacro2->load() : 0.0f;
        const float macro3 = pMacro3 ? pMacro3->load() : 0.0f;
        const float macro4 = pMacro4 ? pMacro4->load() : 0.5f;

        // M1=APERTURE: vowelY open up + formantSpread widen
        // M2=TIDE: vowelMorphRate + vowelMorphDepth
        // M3=VOCODE: vocoder depth add
        // M4=SPACE: output filter open

        const int   excType     = pExcType    ? static_cast<int>(pExcType->load())   : 0;
        const float buzzChar    = pBuzzChar   ? pBuzzChar->load()   : 0.3f;
        float       breathiness = pBreathiness? pBreathiness->load(): 0.15f;
        const float excLevel    = pExcLevel   ? pExcLevel->load()   : 0.8f;
        const float excBright   = pExcBright  ? pExcBright->load()  : 0.5f;

        // Aftertouch modulates breathiness (D006: expression input → timbral change)
        breathiness = std::clamp(breathiness + aftertouchValue * 0.4f, 0.0f, 1.0f);

        float vowelX = pVowelX ? pVowelX->load() : 0.5f;
        float vowelY = pVowelY ? pVowelY->load() : 0.5f;

        // Apply coupling modulation to vowelX/Y
        vowelX = std::clamp(vowelX + couplingEnvVowelX, 0.0f, 1.0f);
        vowelY = std::clamp(vowelY + std::fabs(couplingRhythmVowelY), 0.0f, 1.0f);

        // M1=APERTURE: push vowelY toward open and widen spread
        vowelY = std::clamp(vowelY + macro1 * 0.3f, 0.0f, 1.0f);

        const float formantShift   = pFormantShift  ? pFormantShift->load()  : 0.0f;
        float       formantSpread  = pFormantSpread ? pFormantSpread->load() : 1.0f;
        formantSpread = std::clamp(formantSpread + macro1 * 0.4f, 0.5f, 2.0f);

        const float formantTilt    = pFormantTilt   ? pFormantTilt->load()   : 0.0f;
        const float formantQ       = pFormantQ      ? pFormantQ->load()      : 0.5f;
        float       vowelMorphRate = pVowelMorphRate  ? pVowelMorphRate->load()  : 0.0f;
        float       vowelMorphDepth= pVowelMorphDepth ? pVowelMorphDepth->load() : 0.0f;

        // M2=TIDE: scales vowel morph rate + depth
        vowelMorphRate  = std::clamp(vowelMorphRate  + macro2 * 0.5f, 0.0f, 1.0f);
        vowelMorphDepth = std::clamp(vowelMorphDepth + macro2 * 0.5f, 0.0f, 1.0f);

        float vocode   = pVocode  ? pVocode->load()  : 0.0f;
        vocode = std::clamp(vocode + macro3, 0.0f, 1.0f);  // M3=VOCODE

        const float overtone = pOvertone ? pOvertone->load() : 0.0f;
        const float gallery  = pGallery  ? pGallery->load()  : 0.0f;

        // CC1 mod wheel → gallery feedback depth (D006: expression input → timbral change)
        const float effectiveGallery = std::clamp(gallery + modWheelValue * 0.35f, 0.0f, 1.0f);

        const float ampAtk  = pAmpAtk ? pAmpAtk->load() : 0.01f;
        const float ampDec  = pAmpDec ? pAmpDec->load() : 0.3f;
        const float ampSus  = pAmpSus ? pAmpSus->load() : 0.7f;
        const float ampRel  = pAmpRel ? pAmpRel->load() : 0.5f;

        float baseCutoff = (pFltCutoff ? pFltCutoff->load() : 12000.0f);
        // AmpToFilter coupling raises cutoff
        baseCutoff += couplingAmpFilter * 8000.0f;
        // M4=SPACE: macro opens the output filter
        baseCutoff = std::clamp(baseCutoff * (0.5f + macro4), 20.0f, 20000.0f);

        const float fltReso   = pFltReso  ? pFltReso->load()  : 0.0f;
        const int   fltType   = pFltType  ? static_cast<int>(pFltType->load()) : 0;
        const float fenvAtk   = pFenvAtk  ? pFenvAtk->load()  : 0.01f;
        const float fenvDec   = pFenvDec  ? pFenvDec->load()  : 0.3f;
        const float fenvSus   = pFenvSus  ? pFenvSus->load()  : 0.0f;
        const float fenvRel   = pFenvRel  ? pFenvRel->load()  : 0.4f;
        const float fenvAmt   = pFenvAmt  ? pFenvAmt->load()  : 0.3f;

        // LFO rates: enforce floor 0.01 Hz (D005)
        const float lfo1Rate  = std::max(0.01f, pLfo1Rate  ? pLfo1Rate->load()  : 0.5f);
        const float lfo1Depth = pLfo1Depth ? pLfo1Depth->load() : 0.0f;
        const int   lfo1Shape = pLfo1Shape ? static_cast<int>(pLfo1Shape->load()) : 0;
        const int   lfo1Tgt   = pLfo1Target? static_cast<int>(pLfo1Target->load()): 0;

        const float lfo2Rate  = std::max(0.01f, pLfo2Rate  ? pLfo2Rate->load()  : 0.15f);
        const float lfo2Depth = pLfo2Depth ? pLfo2Depth->load() : 0.0f;
        const int   lfo2Shape = pLfo2Shape ? static_cast<int>(pLfo2Shape->load()) : 1;
        const int   lfo2Tgt   = pLfo2Target? static_cast<int>(pLfo2Target->load()): 1;

        const int   voiceMode = pVoiceMode ? static_cast<int>(pVoiceMode->load()) : 2;
        const float glideTime = pGlide     ? pGlide->load() : 0.0f;
        const float velTimbre = pVelTimbre ? pVelTimbre->load() : 0.5f;

        // Glide coefficient (USE IT — always)
        const float glideCoeff = (glideTime > 0.0001f)
            ? fastExp(-1.0f / (glideTime * sampleRateFloat))
            : 0.0f;

        // ---- Mod matrix sources: gather from active voices ----
        {
            float lfo1Sum = 0.0f, lfo2Sum = 0.0f, envSum = 0.0f;
            float velSum  = 0.0f, ktSum   = 0.0f;
            int count = 0;
            for (auto& v : voices)
            {
                if (v.active)
                {
                    lfo1Sum += v.lastLfo1Val;
                    lfo2Sum += v.lastLfo2Val;
                    envSum  += v.ampEnv.getLevel();
                    velSum  += v.velocity;
                    ktSum   += v.keyTrack;
                    ++count;
                }
            }
            if (count > 0)
            {
                const float inv = 1.0f / static_cast<float>(count);
                blockModSrc.lfo1     = lfo1Sum * inv;
                blockModSrc.lfo2     = lfo2Sum * inv;
                blockModSrc.env      = envSum  * inv;
                blockModSrc.velocity = velSum  * inv;
                blockModSrc.keyTrack = ktSum   * inv;
            }
            blockModSrc.modWheel   = modWheelValue;
            blockModSrc.aftertouch = aftertouchValue;
        }

        // ---- Apply mod matrix ----
        // Dests: 0=Off, 1=Filter Cutoff, 2=Vowel X, 3=Vowel Y,
        //        4=Formant Shift, 5=Gallery, 6=Amp Level
        float modDestOffsets[7] = {};
        modMatrix.apply(blockModSrc, modDestOffsets);

        baseCutoff = std::clamp(baseCutoff  + modDestOffsets[1] * 8000.0f, 20.0f, 20000.0f);
        vowelX     = std::clamp(vowelX      + modDestOffsets[2],           0.0f,  1.0f);
        vowelY     = std::clamp(vowelY      + modDestOffsets[3],           0.0f,  1.0f);
        const float modFormantShiftAdd = modDestOffsets[4] * 800.0f;
        const float effectiveGalleryWithMod = std::clamp(effectiveGallery + modDestOffsets[5], 0.0f, 1.0f);
        const float modAmpLevel = modDestOffsets[6];

        // Store vowelX for coupling output (ch2)
        lastVowelX = vowelX;

        // ---- Output buffers ----
        auto* writeL = buffer.getWritePointer(0);
        auto* writeR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : writeL;
        buffer.clear();

        // ---- MIDI processing interleaved with rendering ----
        int midiSamplePos = 0;

        for (const auto& midiEvent : midi)
        {
            const auto& msg    = midiEvent.getMessage();
            const int   msgPos = std::min(midiEvent.samplePosition, numSamples - 1);

            // Render up to this MIDI event
            renderVoicesRange(writeL, writeR, midiSamplePos, msgPos,
                              excType, buzzChar, breathiness, excLevel, excBright,
                              vowelX, vowelY, formantShift + modFormantShiftAdd, formantSpread,
                              formantTilt, formantQ, vowelMorphRate, vowelMorphDepth,
                              vocode, overtone, effectiveGalleryWithMod,
                              ampAtk, ampDec, ampSus, ampRel,
                              baseCutoff, fltReso, fltType, fenvAtk, fenvDec, fenvSus, fenvRel, fenvAmt,
                              lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                              lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                              velTimbre, glideCoeff, modAmpLevel);
            midiSamplePos = msgPos;

            if (msg.isNoteOn())
            {
                handleNoteOn(msg.getNoteNumber(), msg.getVelocity(),
                             ampAtk, ampDec, ampSus, ampRel,
                             fenvAtk, fenvDec, fenvSus, fenvRel,
                             voiceMode, glideCoeff);
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff(msg.getNoteNumber());
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelValue = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isAftertouch())
            {
                aftertouchValue = msg.getAfterTouchValue() / 127.0f;
            }
        }

        // Render remaining samples after last MIDI event
        renderVoicesRange(writeL, writeR, midiSamplePos, numSamples,
                          excType, buzzChar, breathiness, excLevel, excBright,
                          vowelX, vowelY, formantShift + modFormantShiftAdd, formantSpread,
                          formantTilt, formantQ, vowelMorphRate, vowelMorphDepth,
                          vocode, overtone, effectiveGalleryWithMod,
                          ampAtk, ampDec, ampSus, ampRel,
                          baseCutoff, fltReso, fltType, fenvAtk, fenvDec, fenvSus, fenvRel, fenvAmt,
                          lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                          lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                          velTimbre, glideCoeff, modAmpLevel);

        // ---- Cache last output samples for coupling ----
        if (numSamples > 0)
        {
            lastSampleL = writeL[numSamples - 1];
            lastSampleR = writeR[numSamples - 1];
        }

        // ---- Coupling accumulator decay (0.999× per block) ----
        couplingAmpFilter    *= 0.999f;
        couplingEnvVowelX    *= 0.999f;
        couplingRhythmVowelY *= 0.999f;
        couplingExcLevel     *= 0.999f;

        // ---- Update active voice count (atomic) ----
        int av = 0;
        for (auto& v : voices)
            if (v.active) ++av;
        activeVoiceCount_.store(av, std::memory_order_relaxed);

        // SRO: SilenceGate analysis
        analyzeForSilenceGate(buffer, numSamples);
    }

private:

    //==========================================================================
    //  P R N G
    //==========================================================================
    static inline float xorRand(uint32_t& state) noexcept
    {
        state ^= state << 13u;
        state ^= state >> 17u;
        state ^= state << 5u;
        return static_cast<float>(state) * 2.3283064365e-10f; // [0, 1)
    }

    //==========================================================================
    //  V O W E L   I N T E R P O L A T I O N
    //
    //  2D grid: vowelX [0..1] maps columns, vowelY [0..1] maps rows.
    //  Grid layout (3×3 using 8 vowels + wrap):
    //    Y=0: vowels 0,1,2  (A,E,I)
    //    Y=0.5: vowels 3,4,5 (O,U,Cave1)
    //    Y=1: vowels 5,6,7  (Cave1,Cave2,Cave3)
    //  Bilinear interpolation between 4 nearest targets.
    //==========================================================================
    static void computeVowelFreqs(float vowelX, float vowelY,
                                  float outFreqs[5], float outBWs[5]) noexcept
    {
        // Map XY → indices in a 2-row, 4-column grid of 8 vowels
        // Row 0 (y=0..0.5): vowels 0,1,2,3 (A,E,I,O)
        // Row 1 (y=0.5..1): vowels 4,5,6,7 (U,Cave1,Cave2,Cave3)
        // X maps 3 columns (0..1 → cols 0,1,2)

        // Scale to 3 column positions [0..3)
        const float xScaled = vowelX * 3.0f;
        // Scale to 2 row positions [0..2)
        const float yScaled = vowelY * 2.0f;

        const int col0 = static_cast<int>(xScaled);
        const int col1 = std::min(col0 + 1, 3);
        const float tx = xScaled - static_cast<float>(col0);

        const int row0 = static_cast<int>(yScaled);
        const int row1 = std::min(row0 + 1, 1);
        const float ty = yScaled - static_cast<float>(row0);

        // Map (row, col) → vowel index
        // Row 0: cols 0,1,2,3 → vowel indices 0,1,2,3
        // Row 1: cols 0,1,2,3 → vowel indices 4,5,6,7
        const int idx00 = row0 * 4 + col0;
        const int idx10 = row0 * 4 + col1;
        const int idx01 = row1 * 4 + col0;
        const int idx11 = row1 * 4 + col1;

        // Clamp all to valid range
        const int i00 = std::clamp(idx00, 0, 7);
        const int i10 = std::clamp(idx10, 0, 7);
        const int i01 = std::clamp(idx01, 0, 7);
        const int i11 = std::clamp(idx11, 0, 7);

        for (int f = 0; f < 5; ++f)
        {
            // Bilinear interpolation: lerp along X first, then Y
            const float h0 = lerp(kOndineVowels[i00][f], kOndineVowels[i10][f], tx);
            const float h1 = lerp(kOndineVowels[i01][f], kOndineVowels[i11][f], tx);
            outFreqs[f] = lerp(h0, h1, ty);

            const float bw0 = lerp(kOndineFormantBW[i00][f], kOndineFormantBW[i10][f], tx);
            const float bw1 = lerp(kOndineFormantBW[i01][f], kOndineFormantBW[i11][f], tx);
            outBWs[f]  = lerp(bw0, bw1, ty);
        }
    }

    //==========================================================================
    //  G E N E R A T E   E X C I T A T I O N
    //
    //  Generates one sample of the excitation signal.
    //  excType: 0=Buzz, 1=Noise, 2=Breath, 3=Coupled
    //  Returns sample in [-1..1].
    //==========================================================================
    static inline float generateExcitation(OndineVoice& v, int excType,
                                           float buzzChar, float breathiness,
                                           float excBright, float coupledSample) noexcept
    {
        // ---- Buzz: glottal pulse train ----
        // Phase accumulator generates a pulse. buzzChar blends between
        // hard pulse (0) and soft rounded pulse (1).
        // Slight random jitter in pulse width for natural vocal quality.
        v.excPhase += v.excPhaseInc;
        if (v.excPhase >= 1.0f)
            v.excPhase -= 1.0f;

        // Buzz: pulse at top of cycle, then decay back (glottal shape)
        // buzzChar=0: rectangular pulse; buzzChar=1: cosine-smoothed
        float buzz = 0.0f;
        {
            const float halfPulse = std::max(0.02f, 0.1f - buzzChar * 0.08f); // pulse width 2-10%
            if (v.excPhase < halfPulse)
            {
                const float t = v.excPhase / halfPulse;
                // Hard edge at 0, smooth at 1 (opening glottis)
                buzz = lerp(1.0f, fastCos(t * kOndinePi * 0.5f), buzzChar);
            }
            else
            {
                // Glottal closure: exponential decay shaped by excBright
                const float decayPhase = (v.excPhase - halfPulse) / (1.0f - halfPulse);
                const float decayRate = 2.0f + excBright * 4.0f; // bright=fast decay
                buzz = -fastExp(-decayRate * decayPhase) * (1.0f - halfPulse * 0.5f);
            }
        }

        // ---- Noise: white noise lowpass filtered ----
        // Breathiness controls the LP cutoff for the noise component
        float noise = (xorRand(v.rng) * 2.0f - 1.0f);
        // Simple one-pole LP for noise coloring (approximate)
        // excBright: 0=dark noise, 1=white noise
        const float noiseLPCoeff = 0.1f + excBright * 0.85f;
        noise = noise * noiseLPCoeff + noise * (1.0f - noiseLPCoeff); // just use raw noise scaled
        // More accurate: simply scale noise by excBright to emulate spectral tilt
        noise *= (0.3f + excBright * 0.7f);

        float excSample = 0.0f;
        switch (excType)
        {
        case 0: // Buzz only
            excSample = buzz;
            break;
        case 1: // Noise only
            excSample = noise;
            break;
        case 2: // Breath: mix buzz + noise by breathiness
            excSample = lerp(buzz, noise, breathiness);
            break;
        case 3: // Coupled: external audio replaces/blends with buzz
            excSample = lerp(buzz, coupledSample, std::clamp(std::fabs(coupledSample) * 2.0f, 0.0f, 1.0f));
            break;
        default:
            excSample = buzz;
            break;
        }

        // Apply breathiness blend for non-pure-noise modes
        if (excType != 1 && excType != 3)
            excSample = lerp(excSample, noise, breathiness * 0.6f);

        return excSample;
    }

    //==========================================================================
    //  R E N D E R   V O I C E S   R A N G E
    //==========================================================================
    void renderVoicesRange(float* writeL, float* writeR,
                           int startSample, int endSample,
                           int excType, float buzzChar, float breathiness,
                           float excLevel, float excBright,
                           float vowelX, float vowelY,
                           float formantShift, float formantSpread,
                           float formantTilt, float formantQ,
                           float vowelMorphRate, float vowelMorphDepth,
                           float vocode, float overtone, float gallery,
                           float ampAtk, float ampDec, float ampSus, float ampRel,
                           float baseCutoff, float fltReso, int fltType,
                           float fenvAtk, float fenvDec, float fenvSus, float fenvRel,
                           float fenvAmt,
                           float lfo1Rate, float lfo1Depth, int lfo1Shape, int lfo1Tgt,
                           float lfo2Rate, float lfo2Depth, int lfo2Shape, int lfo2Tgt,
                           float velTimbre, float glideCoeff,
                           float modAmpLevel) noexcept
    {
        if (startSample >= endSample) return;

        // Vocoder follower time constants
        // Attack ~5ms, release ~50ms
        const float vocoderAttCoeff = 1.0f - fastExp(-1.0f / (0.005f * sampleRateFloat));
        const float vocoderRelCoeff = 1.0f - fastExp(-1.0f / (0.050f * sampleRateFloat));

        for (int vi = 0; vi < kOndineMaxVoices; ++vi)
        {
            auto& v = voices[vi];
            if (!v.active) continue;

            // ---- Set LFO rates and shapes ----
            v.lfo1.setRate(lfo1Rate, sampleRateFloat);
            v.lfo1.setShape(lfo1Shape);
            v.lfo2.setRate(lfo2Rate, sampleRateFloat);
            v.lfo2.setShape(lfo2Shape);

            // ---- Set envelope parameters ----
            v.ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
            v.filterEnv.setParams(fenvAtk, fenvDec, fenvSus, fenvRel, sampleRateFloat);

            // ---- Frequency target + pitch glide ----
            const float targetFreq = midiToFreq(v.note);
            // Glide: smooth toward target
            if (glideCoeff > 0.0001f)
                v.glideFreq = glideCoeff * v.glideFreq + (1.0f - glideCoeff) * targetFreq;
            else
                v.glideFreq = targetFreq;

            // Phase increment from glide frequency
            v.excPhaseInc = v.glideFreq / sampleRateFloat;

            // ---- D001: Velocity → Timbre ----
            // Velocity scales excitation brightness + formant spread
            const float velBrightBoost  = velTimbre * v.velocity * 0.4f;
            const float velSpreadBoost  = velTimbre * v.velocity * 0.3f;
            const float effectExcBright = std::clamp(excBright + velBrightBoost, 0.0f, 1.0f);
            const float effectSpread    = std::clamp(formantSpread + velSpreadBoost, 0.5f, 2.5f);

            // ---- Vowel morph auto-oscillator ----
            // vowelMorphRate [0..1] → Hz [0..2] morph cycle
            const float morphHz   = vowelMorphRate * 2.0f;
            const float morphInc  = morphHz / sampleRateFloat;

            // ---- Sample loop ----
            for (int i = startSample; i < endSample; ++i)
            {
                // ---- Tick LFOs every sample ----
                const float lfo1Val = v.lfo1.process();
                const float lfo2Val = v.lfo2.process();
                v.lastLfo1Val = lfo1Val;
                v.lastLfo2Val = lfo2Val;

                // ---- Apply LFO modulation ----
                // Target: 0=VowelX, 1=VowelY, 2=FormantShift, 3=Breathiness
                float sampVowelX        = vowelX;
                float sampVowelY        = vowelY;
                float sampFormantShift  = formantShift;
                float sampBreathiness   = breathiness;

                const float lfo1Effect = lfo1Val * lfo1Depth;
                switch (lfo1Tgt)
                {
                case 0: sampVowelX = std::clamp(sampVowelX + lfo1Effect * 0.5f, 0.0f, 1.0f); break;
                case 1: sampVowelY = std::clamp(sampVowelY + lfo1Effect * 0.5f, 0.0f, 1.0f); break;
                case 2: sampFormantShift += lfo1Effect * 800.0f; break;
                case 3: sampBreathiness = std::clamp(sampBreathiness + lfo1Effect * 0.3f, 0.0f, 1.0f); break;
                default: break;
                }

                const float lfo2Effect = lfo2Val * lfo2Depth;
                switch (lfo2Tgt)
                {
                case 0: sampVowelX = std::clamp(sampVowelX + lfo2Effect * 0.5f, 0.0f, 1.0f); break;
                case 1: sampVowelY = std::clamp(sampVowelY + lfo2Effect * 0.5f, 0.0f, 1.0f); break;
                case 2: sampFormantShift += lfo2Effect * 800.0f; break;
                case 3: sampBreathiness = std::clamp(sampBreathiness + lfo2Effect * 0.3f, 0.0f, 1.0f); break;
                default: break;
                }

                // ---- Vowel auto-morph ----
                if (vowelMorphRate > 0.001f && vowelMorphDepth > 0.001f)
                {
                    v.morphAngle += morphInc;
                    if (v.morphAngle >= 1.0f) v.morphAngle -= 1.0f;
                    const float morphOsc = fastSin(v.morphAngle * kOndineTwoPi);
                    sampVowelX = std::clamp(sampVowelX + morphOsc * vowelMorphDepth * 0.4f, 0.0f, 1.0f);
                    sampVowelY = std::clamp(sampVowelY + morphOsc * vowelMorphDepth * 0.25f, 0.0f, 1.0f);
                }

                // ---- Compute formant frequencies ----
                float fFreqs[5], fBWs[5];

                if (overtone > 0.001f)
                {
                    // OVERTONE mode: lock formants to harmonics of fundamental
                    // F2 locks to harmonic round(overtone) of fundamental
                    const float fundFreq = v.glideFreq;
                    const float overtoneTarget = std::max(1.0f, overtone);
                    const int   harmN = static_cast<int>(overtoneTarget + 0.5f);
                    const float harmFrac = overtoneTarget - static_cast<float>(harmN - 1);

                    // F1 = fundamental
                    fFreqs[0] = fundFreq;
                    fBWs[0]   = fundFreq * 0.05f; // narrow Q for isolation

                    // F2 = interpolated harmonic
                    fFreqs[1] = fundFreq * lerp(static_cast<float>(harmN),
                                                static_cast<float>(harmN + 1), harmFrac);
                    fBWs[1]   = fFreqs[1] * 0.04f; // very narrow for throat singing isolation

                    // F3-F5: spread harmonics above F2
                    for (int f = 2; f < 5; ++f)
                    {
                        fFreqs[f] = fFreqs[1] * static_cast<float>(f);
                        fBWs[f]   = fFreqs[f] * 0.05f;
                    }

                    // Blend with vowel positions based on overtone amount (0=vowel, 1=harmonic)
                    // (partial overtone applies partial harmonic locking)
                    if (overtone < 1.0f)
                    {
                        float vowelFreqs[5], vowelBWs[5];
                        computeVowelFreqs(sampVowelX, sampVowelY, vowelFreqs, vowelBWs);
                        for (int f = 0; f < 5; ++f)
                        {
                            fFreqs[f] = lerp(vowelFreqs[f], fFreqs[f], overtone);
                            fBWs[f]   = lerp(vowelBWs[f],  fBWs[f],   overtone);
                        }
                    }
                }
                else
                {
                    // Normal vowel morphing
                    computeVowelFreqs(sampVowelX, sampVowelY, fFreqs, fBWs);
                }

                // Apply global formant shift and spread
                // formantSpread stretches/compresses relative to F1
                const float f1Base = fFreqs[0];
                for (int f = 0; f < 5; ++f)
                {
                    const float relOffset = fFreqs[f] - f1Base;
                    fFreqs[f] = f1Base + relOffset * effectSpread + sampFormantShift;
                    fFreqs[f] = std::clamp(fFreqs[f], 50.0f, sampleRateFloat * 0.45f);
                    fBWs[f]  = std::max(20.0f, fBWs[f] * effectSpread);
                }

                // formantQ scales bandwidth (lower Q = wider = more overlap)
                for (int f = 0; f < 5; ++f)
                    fBWs[f] = fBWs[f] * (2.0f - formantQ * 1.5f);

                // formantTilt: spectral tilt on formant amplitudes
                // Negative = darker (boost low formants), Positive = brighter (boost high)

                // ---- Update filter coefficients every 16 samples ----
                if ((v.filterUpdateCounter & 15) == 0)
                {
                    for (int f = 0; f < 5; ++f)
                    {
                        // Resonance from bandwidth: Q = freq / bandwidth
                        const float q    = std::clamp(fFreqs[f] / std::max(1.0f, fBWs[f]), 0.5f, 40.0f);
                        // Map Q to CytomicSVF resonance [0..1]
                        // Q=0.5 → res=0, Q=40 → res close to 1
                        const float res  = std::clamp(1.0f - 1.0f / (q * 0.5f + 0.5f), 0.0f, 0.97f);
                        v.formant[f].setMode(CytomicSVF::Mode::BandPass);
                        v.formant[f].setCoefficients(fFreqs[f], res, sampleRateFloat);

                        // Also update vocoder analysis BPs (same frequencies)
                        v.vocoderBP[f].setMode(CytomicSVF::Mode::BandPass);
                        v.vocoderBP[f].setCoefficients(fFreqs[f], 0.5f, sampleRateFloat);
                    }

                    // Output filter
                    const float fenvLevel    = v.filterEnv.getLevel();
                    const float fenvOffset   = fenvAmt != 0.0f ? fenvAmt * fenvLevel * 6000.0f : 0.0f;
                    const float effectCutoff = std::clamp(baseCutoff + fenvOffset, 20.0f, 20000.0f);
                    const CytomicSVF::Mode outMode = filterModeFromInt(fltType);
                    v.outputFilterL.setMode(outMode);
                    v.outputFilterR.setMode(outMode);
                    v.outputFilterL.setCoefficients(effectCutoff, fltReso, sampleRateFloat);
                    v.outputFilterR.setCoefficients(effectCutoff, fltReso, sampleRateFloat);
                }
                v.filterUpdateCounter++;

                // ---- Amp envelope ----
                const float ampLevel = v.ampEnv.process();
                v.filterEnv.process(); // advance filter env regardless

                if (!v.ampEnv.isActive())
                {
                    v.active    = false;
                    v.releasing = false;
                    break;
                }

                // ---- Gallery feedback read (from delay buffer) ----
                // Read 1.5ms back (galDelayLen samples)
                const int galReadPos = (v.galWritePos + v.galDelayLen - (v.galDelayLen)) % v.galDelayLen;
                const float galFeedback = v.galleryDelayBuf[static_cast<size_t>(galReadPos)];

                // ---- Coupled excitation sample ----
                const float coupledSample = (excType == 3 && !couplingExcBuf.empty() && i < maxBlock)
                    ? couplingExcBuf[static_cast<size_t>(i)]
                    : v.couplingExcAccum;

                // ---- Generate excitation ----
                float excSample = generateExcitation(v, excType, buzzChar, sampBreathiness,
                                                     effectExcBright, coupledSample);

                // ---- Blend in gallery feedback into excitation ----
                if (gallery > 0.001f)
                {
                    // Tanh soft limiter on feedback (prevents runaway)
                    const float safeFeedback = fastTanh(galFeedback * gallery * 3.0f);
                    excSample += safeFeedback;
                }

                excSample *= excLevel;

                // ---- 5 parallel formant BPFs ----
                float formantSum = 0.0f;
                for (int f = 0; f < 5; ++f)
                {
                    float fOut = v.formant[f].processSample(excSample);

                    // formantTilt: bipolar gain per formant
                    // Negative: boost f0, attenuate f4; Positive: reverse
                    const float tiltFactor = static_cast<float>(f) / 4.0f; // 0..1
                    float tiltGain;
                    if (formantTilt < 0.0f)
                        tiltGain = 1.0f + formantTilt * (1.0f - tiltFactor);
                    else
                        tiltGain = 1.0f + formantTilt * tiltFactor;
                    tiltGain = std::max(0.0f, tiltGain);
                    fOut *= tiltGain;

                    // VOCODE: analyze coupling audio through same-freq BPs
                    if (vocode > 0.001f && !couplingExcBuf.empty() && i < maxBlock)
                    {
                        const float analyzeSample = couplingExcBuf[static_cast<size_t>(i)];
                        const float bpOut = v.vocoderBP[f].processSample(analyzeSample);
                        const float rectified = std::fabs(bpOut);
                        // Envelope follower
                        if (rectified > v.vocoderEnv[f])
                            v.vocoderEnv[f] += (rectified - v.vocoderEnv[f]) * vocoderAttCoeff;
                        else
                            v.vocoderEnv[f] += (rectified - v.vocoderEnv[f]) * vocoderRelCoeff;
                        v.vocoderEnv[f] = flushDenormal(v.vocoderEnv[f]);

                        // Blend: vocode=0 → internal formant, vocode=1 → vocoded amplitude
                        fOut = lerp(fOut, fOut * v.vocoderEnv[f] * 8.0f, vocode);
                    }

                    formantSum += fOut;
                }

                // Normalize by formant count (prevent overload)
                formantSum *= 0.2f;

                // ---- DC blocker in gallery feedback path ----
                {
                    const float dcIn  = formantSum;
                    const float dcOut = dcIn - v.dcBlockX + 0.9975f * v.dcBlockY;
                    v.dcBlockX = dcIn;
                    v.dcBlockY = flushDenormal(dcOut);

                    // Write into gallery delay buffer (what goes around comes around)
                    v.galleryDelayBuf[static_cast<size_t>(v.galWritePos)] = dcOut;
                    v.galWritePos = (v.galWritePos + 1) % v.galDelayLen;

                    formantSum = dcOut;
                }
                formantSum = flushDenormal(formantSum);

                // ---- Output filter ----
                float outL = v.outputFilterL.processSample(formantSum);
                float outR = v.outputFilterR.processSample(formantSum);
                outL = flushDenormal(outL);
                outR = flushDenormal(outR);

                // ---- VCA: amp envelope × velocity × mod ----
                const float gain = ampLevel * (0.7f + v.velocity * 0.3f) * (1.0f + modAmpLevel);
                outL *= gain;
                outR *= gain;

                // ---- Accumulate into output ----
                writeL[i] += outL;
                writeR[i] += outR;
            }
        }
    }

    //==========================================================================
    //  N O T E   H A N D L I N G
    //==========================================================================

    void handleNoteOn(int note, int midiVelocity,
                      float ampAtk, float ampDec, float ampSus, float ampRel,
                      float fenvAtk, float fenvDec, float fenvSus, float fenvRel,
                      int voiceMode, float glideCoeff) noexcept
    {
        const float vel = midiVelocity / 127.0f;

        const int maxVoices = voiceModeToCount(voiceMode);

        // Find a free voice or steal oldest
        int voiceIdx = -1;
        for (int i = 0; i < maxVoices; ++i)
        {
            if (!voices[i].active)
            {
                voiceIdx = i;
                break;
            }
        }

        // Steal the quietest releasing voice, or oldest active voice
        if (voiceIdx < 0)
        {
            // Look for a releasing voice first
            for (int i = 0; i < maxVoices; ++i)
            {
                if (voices[i].releasing)
                {
                    voiceIdx = i;
                    break;
                }
            }
        }
        if (voiceIdx < 0)
            voiceIdx = 0; // fallback: steal voice 0

        auto& v = voices[voiceIdx];

        const bool isLegato = (voiceMode == 1); // Legato mode
        const float prevLevel = v.ampEnv.getLevel();
        const float prevGlideFreq = v.glideFreq;

        v.active    = true;
        v.releasing = false;
        v.note      = note;
        v.velocity  = vel;
        v.keyTrack  = (static_cast<float>(note) - 60.0f) / 60.0f;

        // Glide: carry over previous frequency in legato or when glide is active
        if (isLegato && prevLevel > 0.01f)
            v.glideFreq = prevGlideFreq; // will glide from previous
        else if (glideCoeff > 0.0001f)
            v.glideFreq = prevGlideFreq; // always glide from previous if glide active
        else
            v.glideFreq = midiToFreq(note);

        // Phase: reset on new note unless legato
        if (!isLegato)
            v.excPhase = 0.0f;

        // Envelopes
        if (isLegato && prevLevel > 0.01f)
        {
            v.ampEnv.retriggerFrom(prevLevel, ampAtk, ampDec, ampSus, ampRel);
        }
        else
        {
            v.ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
            v.ampEnv.noteOn();
        }
        v.filterEnv.setParams(fenvAtk, fenvDec, fenvSus, fenvRel, sampleRateFloat);
        v.filterEnv.noteOn();

        // Reset gallery path on new note (prevents bleed)
        if (!isLegato)
        {
            if (!v.galleryDelayBuf.empty())
                std::fill(v.galleryDelayBuf.begin(), v.galleryDelayBuf.end(), 0.0f);
            v.dcBlockX = 0.0f;
            v.dcBlockY = 0.0f;
            v.galWritePos = 0;
            for (int f = 0; f < 5; ++f)
                v.vocoderEnv[f] = 0.0f;
        }
    }

    void handleNoteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && v.note == note && !v.releasing)
            {
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
                v.releasing = true;
                break;
            }
        }
    }

    static int voiceModeToCount(int voiceMode) noexcept
    {
        switch (voiceMode)
        {
        case 0: return 1; // Mono
        case 1: return 1; // Legato
        case 2: return 4; // Poly4
        case 3: return 8; // Poly8
        default: return 4;
        }
    }

    static CytomicSVF::Mode filterModeFromInt(int type) noexcept
    {
        switch (type)
        {
        case 0: return CytomicSVF::Mode::LowPass;
        case 1: return CytomicSVF::Mode::HighPass;
        case 2: return CytomicSVF::Mode::BandPass;
        case 3: return CytomicSVF::Mode::Notch;
        default: return CytomicSVF::Mode::LowPass;
        }
    }

    //==========================================================================
    //  V O I C E S   +   S T A T E
    //==========================================================================

    std::array<OndineVoice, kOndineMaxVoices> voices;

    float sampleRateFloat = 44100.0f;
    int   maxBlock        = 512;

    // Coupling accumulators
    std::vector<float> couplingExcBuf;
    float couplingAmpFilter     = 0.0f;
    float couplingEnvVowelX     = 0.0f;
    float couplingRhythmVowelY  = 0.0f;
    float couplingExcLevel      = 0.0f;

    // Expression
    float modWheelValue   = 0.0f;
    float aftertouchValue = 0.0f;

    // Cached output for coupling
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;
    float lastVowelX  = 0.5f;

    // Mod matrix sources (block-rate aggregate)
    ModMatrix<4>::Sources blockModSrc;

    // Mod matrix
    ModMatrix<4> modMatrix;

    //==========================================================================
    //  P A R A M E T E R   P O I N T E R S
    //==========================================================================

    // Group A
    std::atomic<float>* pExcType      = nullptr;
    std::atomic<float>* pBuzzChar     = nullptr;
    std::atomic<float>* pBreathiness  = nullptr;
    std::atomic<float>* pExcLevel     = nullptr;
    std::atomic<float>* pExcBright    = nullptr;

    // Group B
    std::atomic<float>* pVowelX          = nullptr;
    std::atomic<float>* pVowelY          = nullptr;
    std::atomic<float>* pFormantShift    = nullptr;
    std::atomic<float>* pFormantSpread   = nullptr;
    std::atomic<float>* pFormantTilt     = nullptr;
    std::atomic<float>* pFormantQ        = nullptr;
    std::atomic<float>* pVowelMorphRate  = nullptr;
    std::atomic<float>* pVowelMorphDepth = nullptr;

    // Group C
    std::atomic<float>* pVocode   = nullptr;
    std::atomic<float>* pOvertone = nullptr;
    std::atomic<float>* pGallery  = nullptr;

    // Group D
    std::atomic<float>* pAmpAtk = nullptr;
    std::atomic<float>* pAmpDec = nullptr;
    std::atomic<float>* pAmpSus = nullptr;
    std::atomic<float>* pAmpRel = nullptr;

    // Group E
    std::atomic<float>* pFltCutoff = nullptr;
    std::atomic<float>* pFltReso   = nullptr;
    std::atomic<float>* pFltType   = nullptr;
    std::atomic<float>* pFenvAtk   = nullptr;
    std::atomic<float>* pFenvDec   = nullptr;
    std::atomic<float>* pFenvSus   = nullptr;
    std::atomic<float>* pFenvRel   = nullptr;
    std::atomic<float>* pFenvAmt   = nullptr;

    // Group F
    std::atomic<float>* pLfo1Rate   = nullptr;
    std::atomic<float>* pLfo1Depth  = nullptr;
    std::atomic<float>* pLfo1Shape  = nullptr;
    std::atomic<float>* pLfo1Target = nullptr;
    std::atomic<float>* pLfo2Rate   = nullptr;
    std::atomic<float>* pLfo2Depth  = nullptr;
    std::atomic<float>* pLfo2Shape  = nullptr;
    std::atomic<float>* pLfo2Target = nullptr;

    // Group H
    std::atomic<float>* pMacro1    = nullptr;
    std::atomic<float>* pMacro2    = nullptr;
    std::atomic<float>* pMacro3    = nullptr;
    std::atomic<float>* pMacro4    = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide     = nullptr;
    std::atomic<float>* pVelTimbre = nullptr;
};

} // namespace xoceanus
