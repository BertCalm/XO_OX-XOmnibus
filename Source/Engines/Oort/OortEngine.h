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
//  O O R T   E N G I N E
//  Stochastic Cloud Synthesis (GENDY Algorithm)
//
//  XO_OX Aquatic Identity: The Oort Cloud — a vast halo of primordial bodies
//  orbiting the sun at the edge of the solar system. Chaotic, ancient, and
//  mutating over astronomical timescales. Each breakpoint = a voice in the crowd.
//  INTENT parameter: Cage (0, pure chance) <-> Xenakis (1, shaped distributions).
//
//  References (D003):
//    Iannis Xenakis, GENDY algorithm (1991)
//    Xenakis, "Formalized Music" (1971)
//    Einstein, Brownian motion (1905)
//    Markov, Markov chains (1906)
//
//  Architecture: GENDY polyphonic breakpoint waveform synthesis.
//    - Waveform = N breakpoints (amplitude, duration) linearly interpolated
//    - Per-cycle mutation via probability distributions (Gaussian/Cauchy/Logistic/Uniform)
//    - Markov chain state memory biases walk toward previous steps
//    - Poisson-distributed event overlay for granular textures
//    - CytomicSVF filter -> VCA -> output
//
//  Signal Flow:
//    GENDY Breakpoint Waveform (per-voice, mutating per-cycle)
//      + Poisson Event Layer (when eventDensity > 0)
//      -> DC Block (when dcBlock=1, essential for Cauchy)
//      -> Wavefold (foldAmt)
//      -> CytomicSVF Filter (post-GENDY)
//      -> VCA (amp envelope * velocity)
//      -> Output
//
//  Coupling:
//    Output: stereo (ch0=L, ch1=R), scatter value 0-1 (ch2)
//    Input:  AudioToFM      -> modulates breakpoint durations (FM-like effect)
//            AmpToFilter    -> filter cutoff modulation
//            EnvToMorph     -> scatter (distribution width) modulation
//            RhythmToBlend  -> triggers Poisson events
//
//  Gallery code: OORT | Accent: Oort Cloud Violet #9B7FD4 | Prefix: oort_
//
//==============================================================================

static constexpr int   kOortMaxVoices = 8;
static constexpr int   kOortMaxBP     = 16;    // maximum breakpoints per voice
static constexpr float kOortTwoPi     = 6.28318530717958647692f;
static constexpr float kOortPi        = 3.14159265358979323846f;

//==============================================================================
// GendyBreakpoint — one amplitude/duration vertex in the GENDY waveform
//==============================================================================
struct GendyBreakpoint
{
    float amplitude = 0.0f;   // -1.0 to +1.0
    float duration  = 256.0f; // in samples (time to next breakpoint)
};

//==============================================================================
// OortPoissonGrain — short transient event triggered by Poisson process
//==============================================================================
struct OortPoissonGrain
{
    bool  active     = false;
    float amplitude  = 0.0f;
    float level      = 0.0f;    // current envelope level
    float decayCoeff = 0.0f;    // per-sample decay coefficient
};

//==============================================================================
// OortVoice — one GENDY synthesis voice
//==============================================================================
struct OortVoice
{
    bool  active    = false;
    bool  releasing = false;
    int   note      = -1;
    float velocity  = 0.0f;  // normalized 0..1
    float keyTrack  = 0.0f;  // (note-60)/60, bipolar

    // ---- GENDY breakpoint state ----
    std::vector<GendyBreakpoint> breakpoints;  // sized kOortMaxBP in prepare()
    int   bpCount   = 8;      // active breakpoints (2-16)
    int   bpIdx     = 0;      // current breakpoint index
    float bpPhase   = 0.0f;   // progress from bpIdx to bpIdx+1 in [0,1]

    // ---- Markov chain state ----
    float prevAmpStep  = 0.0f;
    float prevTimeStep = 0.0f;

    // ---- Glide state ----
    float glideBaseFreq = 440.0f; // current (smoothed) base frequency

    // ---- Poisson event layer ----
    float poissonTimer = 0.0f;   // counts down to next event in samples
    OortPoissonGrain grain;

    // ---- DC blocker state ----
    float dcBlockX = 0.0f;  // input delay (x[n-1])
    float dcBlockY = 0.0f;  // output delay (y[n-1])

    // ---- Filter ----
    CytomicSVF filterL;
    CytomicSVF filterR;

    // ---- Envelopes ----
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // ---- Per-voice LFOs ----
    StandardLFO lfo1;
    StandardLFO lfo2;

    // ---- Last LFO output (for mod matrix sources) ----
    float lastLfo1Val = 0.0f;
    float lastLfo2Val = 0.0f;

    // ---- PRNG state (xorshift32, per-voice) ----
    uint32_t rng = 12345u;

    // ---- Coupling AudioToFM accumulator ----
    float fmCouplingAccum = 0.0f;

    void reset(float sampleRate) noexcept
    {
        active    = false;
        releasing = false;
        note      = -1;
        velocity  = 0.0f;
        keyTrack  = 0.0f;

        bpIdx   = 0;
        bpPhase = 0.0f;
        prevAmpStep  = 0.0f;
        prevTimeStep = 0.0f;
        glideBaseFreq = 440.0f;
        poissonTimer = 0.0f;
        grain.active  = false;
        grain.level   = 0.0f;
        dcBlockX = 0.0f;
        dcBlockY = 0.0f;
        filterL.reset();
        filterR.reset();
        ampEnv.reset();
        filterEnv.reset();
        lfo1.reset();
        lfo2.reset();
        lastLfo1Val = 0.0f;
        lastLfo2Val = 0.0f;
        fmCouplingAccum = 0.0f;

        // Initialise breakpoints to a sine-like waveform
        const int bpN = static_cast<int>(breakpoints.size());
        if (bpN <= 0) return;
        const float sr = (sampleRate > 0.0f) ? sampleRate : 44100.0f;
        const float defaultDur = sr / (440.0f * static_cast<float>(bpN));
        for (int i = 0; i < bpN; ++i)
        {
            const float phase = static_cast<float>(i) / static_cast<float>(bpN);
            breakpoints[i].amplitude = fastSin(phase * kOortTwoPi);
            breakpoints[i].duration  = std::max(1.0f, defaultDur);
        }
    }
};

//==============================================================================
// OortEngine — GENDY Stochastic Cloud Synthesis
//==============================================================================
class OortEngine : public SynthEngine
{
public:
    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateFloat = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 44100.0f;
        maxBlock = maxBlockSize;

        // Size voice breakpoint buffers and reset
        for (int i = 0; i < kOortMaxVoices; ++i)
        {
            voices[i].breakpoints.resize(kOortMaxBP);
            voices[i].rng = 12345u + static_cast<uint32_t>(i) * 31337u;
            voices[i].reset(sampleRateFloat);
        }

        // Reset coupling accumulators
        couplingFMBuf      = 0.0f;
        couplingAmpFilter  = 0.0f;
        couplingEnvScatter = 0.0f;
        couplingRhythm     = 0.0f;

        modWheelValue   = 0.0f;
        aftertouchValue = 0.0f;

        lastSampleL = 0.0f;
        lastSampleR = 0.0f;
        lastScatter = 0.0f;

        activeVoiceCount_.store(0, std::memory_order_relaxed);

        // SilenceGate — GENDY can have long stochastic tails
        prepareSilenceGate(sampleRate, maxBlockSize, 400.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset(sampleRateFloat);

        couplingFMBuf      = 0.0f;
        couplingAmpFilter  = 0.0f;
        couplingEnvScatter = 0.0f;
        couplingRhythm     = 0.0f;
        modWheelValue      = 0.0f;
        aftertouchValue    = 0.0f;
        lastSampleL = 0.0f;
        lastSampleR = 0.0f;
        lastScatter = 0.0f;
        activeVoiceCount_.store(0, std::memory_order_relaxed);
    }

    //==========================================================================
    // Coupling — applyCouplingInput (called BEFORE renderBlock)
    //==========================================================================

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        if (sourceBuffer == nullptr || numSamples <= 0)
            return;

        switch (type)
        {
        case CouplingType::AudioToFM:
        {
            // Modulates breakpoint durations (FM-like effect on GENDY period)
            couplingFMBuf = sourceBuffer[numSamples - 1] * amount;
            for (auto& v : voices)
                if (v.active)
                    v.fmCouplingAccum = sourceBuffer[numSamples - 1] * amount;
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
            // Coupling envelope modulates scatter (distribution width)
            couplingEnvScatter = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::RhythmToBlend:
        {
            // Coupling rhythm triggers Poisson events
            couplingRhythm = sourceBuffer[numSamples - 1] * amount;
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

    //==========================================================================
    // getSampleForCoupling — O(1) cached output
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return lastSampleL;
        if (channel == 1) return lastSampleR;
        if (channel == 2) return lastScatter; // current scatter value 0..1
        return 0.0f;
    }

    //==========================================================================
    // Parameters
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using NRange = juce::NormalisableRange<float>;
        using AP  = juce::AudioParameterFloat;
        using APC = juce::AudioParameterChoice;
        using PID = juce::ParameterID;

        // ---- Group A: GENDY Core (8 params) ----
        // Choice range 4-16: index 0="4", index 4="8", index 12="16"
        static const juce::StringArray kBpChoices {"4","5","6","7","8","9","10","11","12","13","14","15","16"};
        params.push_back(std::make_unique<APC>(PID{"oort_breakpoints", 1}, "oort_breakpoints",
            kBpChoices, 4)); // default index 4 = "8" breakpoints

        params.push_back(std::make_unique<AP>(PID{"oort_scatter", 1}, "oort_scatter",
            NRange{0.0f, 1.0f, 0.001f}, 0.15f));

        static const juce::StringArray kDistTypes {"Gaussian","Cauchy","Logistic","Uniform"};
        params.push_back(std::make_unique<APC>(PID{"oort_distType", 1}, "oort_distType",
            kDistTypes, 0));

        params.push_back(std::make_unique<AP>(PID{"oort_intent", 1}, "oort_intent",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"oort_mutationRate", 1}, "oort_mutationRate",
            NRange{0.0f, 1.0f, 0.001f}, 0.3f));

        params.push_back(std::make_unique<AP>(PID{"oort_ampScatter", 1}, "oort_ampScatter",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"oort_timeScatter", 1}, "oort_timeScatter",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"oort_stepFloor", 1}, "oort_stepFloor",
            NRange{0.0f, 0.5f, 0.0001f}, 0.05f));

        // ---- Group B: Waveform Shaping (4 params) ----
        params.push_back(std::make_unique<AP>(PID{"oort_symmetry", 1}, "oort_symmetry",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"oort_foldAmt", 1}, "oort_foldAmt",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        static const juce::StringArray kDCOnOff {"Off","On"};
        params.push_back(std::make_unique<APC>(PID{"oort_dcBlock", 1}, "oort_dcBlock",
            kDCOnOff, 1)); // default On

        params.push_back(std::make_unique<AP>(PID{"oort_pitchTrack", 1}, "oort_pitchTrack",
            NRange{0.0f, 1.0f, 0.001f}, 1.0f));

        // ---- Group C: Markov Chain (4 params) ----
        params.push_back(std::make_unique<AP>(PID{"oort_markovMix", 1}, "oort_markovMix",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"oort_stateMemory", 1}, "oort_stateMemory",
            NRange{0.0f, 1.0f, 0.001f}, 0.3f));

        static const juce::StringArray kResetTrig {"Off","On"};
        params.push_back(std::make_unique<APC>(PID{"oort_resetTrigger", 1}, "oort_resetTrigger",
            kResetTrig, 0));

        params.push_back(std::make_unique<AP>(PID{"oort_convergePitch", 1}, "oort_convergePitch",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        // ---- Group D: Event Density (4 params) ----
        params.push_back(std::make_unique<AP>(PID{"oort_eventDensity", 1}, "oort_eventDensity",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"oort_eventAmp", 1}, "oort_eventAmp",
            NRange{0.0f, 1.0f, 0.001f}, 0.8f));

        {
            NRange r{0.001f, 0.5f, 0.0001f};
            r.setSkewForCentre(0.02f);
            params.push_back(std::make_unique<AP>(PID{"oort_eventDecay", 1}, "oort_eventDecay", r, 0.02f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_densityJitter", 1}, "oort_densityJitter",
            NRange{0.0f, 1.0f, 0.001f}, 0.3f));

        // ---- Group E: Filter + Filter Envelope (9 params) ----
        {
            NRange r{20.0f, 20000.0f, 0.1f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_fltCutoff", 1}, "oort_fltCutoff", r, 8000.0f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_fltReso", 1}, "oort_fltReso",
            NRange{0.0f, 1.0f, 0.001f}, 0.1f));

        static const juce::StringArray kFltTypes {"LP","HP","BP","Notch"};
        params.push_back(std::make_unique<APC>(PID{"oort_fltType", 1}, "oort_fltType",
            kFltTypes, 0));

        params.push_back(std::make_unique<AP>(PID{"oort_fltEnvAmt", 1}, "oort_fltEnvAmt",
            NRange{-1.0f, 1.0f, 0.001f}, 0.3f));

        params.push_back(std::make_unique<AP>(PID{"oort_fltKeyTrack", 1}, "oort_fltKeyTrack",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_fltAtk", 1}, "oort_fltAtk", r, 0.01f));
        }
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_fltDec", 1}, "oort_fltDec", r, 0.3f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_fltSus", 1}, "oort_fltSus",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_fltRel", 1}, "oort_fltRel", r, 0.4f));
        }

        // ---- Group F: Amp Envelope + velTimbre (5 params) ----
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_ampAtk", 1}, "oort_ampAtk", r, 0.005f));
        }
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_ampDec", 1}, "oort_ampDec", r, 0.4f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_ampSus", 1}, "oort_ampSus",
            NRange{0.0f, 1.0f, 0.001f}, 0.7f));

        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_ampRel", 1}, "oort_ampRel", r, 0.6f));
        }

        params.push_back(std::make_unique<AP>(PID{"oort_velTimbre", 1}, "oort_velTimbre",
            NRange{0.0f, 1.0f, 0.001f}, 0.6f));

        // ---- Group G: LFOs (8 params) ----
        static const juce::StringArray kLFOShapes  {"Sine","Tri","Saw","Square","S&H"};
        static const juce::StringArray kLFOTargets {"Scatter","Intent","Filter Cutoff","Event Density"};

        {
            NRange r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_lfo1Rate", 1}, "oort_lfo1Rate", r, 0.5f));
        }
        params.push_back(std::make_unique<AP>(PID{"oort_lfo1Depth", 1}, "oort_lfo1Depth",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"oort_lfo1Shape", 1}, "oort_lfo1Shape",
            kLFOShapes, 0));
        params.push_back(std::make_unique<APC>(PID{"oort_lfo1Target", 1}, "oort_lfo1Target",
            kLFOTargets, 0)); // Scatter

        {
            NRange r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"oort_lfo2Rate", 1}, "oort_lfo2Rate", r, 0.12f));
        }
        params.push_back(std::make_unique<AP>(PID{"oort_lfo2Depth", 1}, "oort_lfo2Depth",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"oort_lfo2Shape", 1}, "oort_lfo2Shape",
            kLFOShapes, 2)); // Saw
        params.push_back(std::make_unique<APC>(PID{"oort_lfo2Target", 1}, "oort_lfo2Target",
            kLFOTargets, 1)); // Intent

        // ---- Group H: Mod Matrix (4 slots x 3 params = 12 params) ----
        static const juce::StringArray kOortModDests {
            "Off", "Filter Cutoff", "Scatter", "Intent",
            "Event Density", "Mutation Rate", "Amp Level"
        };
        ModMatrix<4>::addParameters(params, "oort_", "Oort", kOortModDests);

        // ---- Group I: Macros + Voice (7 params) ----
        // M1=SOLIDARITY: inverted scatter + breakpoint spread
        params.push_back(std::make_unique<AP>(PID{"oort_macro1", 1}, "oort_macro1",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        // M2=INTENT: distribution type morph
        params.push_back(std::make_unique<AP>(PID{"oort_macro2", 1}, "oort_macro2",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        // M3=DRIFT: mutation rate + Markov memory
        params.push_back(std::make_unique<AP>(PID{"oort_macro3", 1}, "oort_macro3",
            NRange{0.0f, 1.0f, 0.001f}, 0.3f));
        // M4=SPACE: filter cutoff + width
        params.push_back(std::make_unique<AP>(PID{"oort_macro4", 1}, "oort_macro4",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        static const juce::StringArray kVoiceModes {"Mono","Legato","Poly4","Poly8"};
        params.push_back(std::make_unique<APC>(PID{"oort_voiceMode", 1}, "oort_voiceMode",
            kVoiceModes, 2));

        {
            NRange r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"oort_glide", 1}, "oort_glide", r, 0.0f));
        }

        static const juce::StringArray kGlideModes {"Legato","Always"};
        params.push_back(std::make_unique<APC>(PID{"oort_glideMode", 1}, "oort_glideMode",
            kGlideModes, 0));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return {params.begin(), params.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Group A
        pBreakpoints   = apvts.getRawParameterValue("oort_breakpoints");
        pScatter       = apvts.getRawParameterValue("oort_scatter");
        pDistType      = apvts.getRawParameterValue("oort_distType");
        pIntent        = apvts.getRawParameterValue("oort_intent");
        pMutationRate  = apvts.getRawParameterValue("oort_mutationRate");
        pAmpScatter    = apvts.getRawParameterValue("oort_ampScatter");
        pTimeScatter   = apvts.getRawParameterValue("oort_timeScatter");
        pStepFloor     = apvts.getRawParameterValue("oort_stepFloor");

        // Group B
        pSymmetry      = apvts.getRawParameterValue("oort_symmetry");
        pFoldAmt       = apvts.getRawParameterValue("oort_foldAmt");
        pDCBlock       = apvts.getRawParameterValue("oort_dcBlock");
        pPitchTrack    = apvts.getRawParameterValue("oort_pitchTrack");

        // Group C
        pMarkovMix     = apvts.getRawParameterValue("oort_markovMix");
        pStateMemory   = apvts.getRawParameterValue("oort_stateMemory");
        pResetTrigger  = apvts.getRawParameterValue("oort_resetTrigger");
        pConvergePitch = apvts.getRawParameterValue("oort_convergePitch");

        // Group D
        pEventDensity  = apvts.getRawParameterValue("oort_eventDensity");
        pEventAmp      = apvts.getRawParameterValue("oort_eventAmp");
        pEventDecay    = apvts.getRawParameterValue("oort_eventDecay");
        pDensityJitter = apvts.getRawParameterValue("oort_densityJitter");

        // Group E
        pFltCutoff     = apvts.getRawParameterValue("oort_fltCutoff");
        pFltReso       = apvts.getRawParameterValue("oort_fltReso");
        pFltType       = apvts.getRawParameterValue("oort_fltType");
        pFltEnvAmt     = apvts.getRawParameterValue("oort_fltEnvAmt");
        pFltKeyTrack   = apvts.getRawParameterValue("oort_fltKeyTrack");
        pFltAtk        = apvts.getRawParameterValue("oort_fltAtk");
        pFltDec        = apvts.getRawParameterValue("oort_fltDec");
        pFltSus        = apvts.getRawParameterValue("oort_fltSus");
        pFltRel        = apvts.getRawParameterValue("oort_fltRel");

        // Group F
        pAmpAtk        = apvts.getRawParameterValue("oort_ampAtk");
        pAmpDec        = apvts.getRawParameterValue("oort_ampDec");
        pAmpSus        = apvts.getRawParameterValue("oort_ampSus");
        pAmpRel        = apvts.getRawParameterValue("oort_ampRel");
        pVelTimbre     = apvts.getRawParameterValue("oort_velTimbre");

        // Group G
        pLfo1Rate      = apvts.getRawParameterValue("oort_lfo1Rate");
        pLfo1Depth     = apvts.getRawParameterValue("oort_lfo1Depth");
        pLfo1Shape     = apvts.getRawParameterValue("oort_lfo1Shape");
        pLfo1Target    = apvts.getRawParameterValue("oort_lfo1Target");
        pLfo2Rate      = apvts.getRawParameterValue("oort_lfo2Rate");
        pLfo2Depth     = apvts.getRawParameterValue("oort_lfo2Depth");
        pLfo2Shape     = apvts.getRawParameterValue("oort_lfo2Shape");
        pLfo2Target    = apvts.getRawParameterValue("oort_lfo2Target");

        // Group H
        modMatrix.attachParameters(apvts, "oort_");

        // Group I
        pMacro1    = apvts.getRawParameterValue("oort_macro1");
        pMacro2    = apvts.getRawParameterValue("oort_macro2");
        pMacro3    = apvts.getRawParameterValue("oort_macro3");
        pMacro4    = apvts.getRawParameterValue("oort_macro4");
        pVoiceMode = apvts.getRawParameterValue("oort_voiceMode");
        pGlide     = apvts.getRawParameterValue("oort_glide");
        pGlideMode = apvts.getRawParameterValue("oort_glideMode");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String  getEngineId()     const override { return "Oort"; }
    juce::Colour  getAccentColour() const override { return juce::Colour(0xFF9B7FD4); } // Oort Cloud Violet
    int           getMaxVoices()    const override { return kOortMaxVoices; }

    //==========================================================================
    // renderBlock
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
        const float macro2 = pMacro2 ? pMacro2->load() : 0.5f;
        const float macro3 = pMacro3 ? pMacro3->load() : 0.3f;
        const float macro4 = pMacro4 ? pMacro4->load() : 0.5f;

        // M1=SOLIDARITY: inverted scatter multiplier
        //   macro1=0 -> 2x scatter (chaotic), macro1=1 -> 0 scatter (focused/ordered)
        const float solidarityScatterMult = 2.0f * (1.0f - macro1);

        // M2=INTENT: scales intent (distribution character morph)
        const float intentMacroMult = macro2 * 2.0f;

        // M3=DRIFT: scales mutationRate + convergePitch
        const float driftMult = macro3 * 2.0f;

        // M4=SPACE: scales filter cutoff
        const float spaceMult = 0.5f + macro4; // [0.5..1.5]

        // ---- Base GENDY params ----
        // oort_breakpoints choice: index 0="4" ... index 12="16"
        const int bpCountIdx = pBreakpoints ? static_cast<int>(pBreakpoints->load()) : 4;
        const int bpCount = std::clamp(bpCountIdx + 4, 4, kOortMaxBP);

        float scatter = pScatter ? pScatter->load() : 0.15f;
        scatter = std::clamp(scatter * (solidarityScatterMult + 0.0001f), 0.0f, 2.0f);

        const int   distType    = pDistType    ? static_cast<int>(pDistType->load())    : 0;
        float       intent      = pIntent      ? pIntent->load()      : 0.5f;
        intent = std::clamp(intent * intentMacroMult, 0.0f, 1.0f);

        float mutationRate = pMutationRate ? pMutationRate->load() : 0.3f;
        mutationRate = std::clamp(mutationRate * driftMult, 0.0f, 2.0f);

        const float ampScatter   = pAmpScatter   ? pAmpScatter->load()   : 0.5f;
        const float timeScatter  = pTimeScatter  ? pTimeScatter->load()  : 0.5f;
        const float stepFloor    = pStepFloor    ? pStepFloor->load()    : 0.05f;
        const float symmetry     = pSymmetry     ? pSymmetry->load()     : 0.5f;
        const float foldAmt      = pFoldAmt      ? pFoldAmt->load()      : 0.0f;
        const int   dcBlock      = pDCBlock      ? static_cast<int>(pDCBlock->load())  : 1;
        const float pitchTrack   = pPitchTrack   ? pPitchTrack->load()   : 1.0f;
        const float markovMix    = pMarkovMix    ? pMarkovMix->load()    : 0.0f;
        const float stateMemory  = pStateMemory  ? pStateMemory->load()  : 0.3f;
        const int   resetTrig    = pResetTrigger ? static_cast<int>(pResetTrigger->load()) : 0;
        float       convergePitch= pConvergePitch? pConvergePitch->load(): 0.0f;
        convergePitch = std::clamp(convergePitch * driftMult, 0.0f, 2.0f);

        float eventDensity = pEventDensity ? pEventDensity->load() : 0.0f;
        // RhythmToBlend coupling drives Poisson event density
        eventDensity = std::clamp(eventDensity + std::fabs(couplingRhythm), 0.0f, 1.0f);

        const float eventAmp     = pEventAmp     ? pEventAmp->load()     : 0.8f;
        const float eventDecay   = pEventDecay   ? pEventDecay->load()   : 0.02f;
        const float densityJitter= pDensityJitter? pDensityJitter->load(): 0.3f;

        float baseCutoff = (pFltCutoff ? pFltCutoff->load() : 8000.0f) * spaceMult;
        // AmpToFilter coupling raises cutoff
        baseCutoff += couplingAmpFilter * 8000.0f;
        baseCutoff = std::clamp(baseCutoff, 20.0f, 20000.0f);

        const float fltReso      = pFltReso      ? pFltReso->load()      : 0.1f;
        const int   fltType      = pFltType      ? static_cast<int>(pFltType->load())  : 0;
        const float fltEnvAmt    = pFltEnvAmt    ? pFltEnvAmt->load()    : 0.3f;
        const float fltKeyTrack  = pFltKeyTrack  ? pFltKeyTrack->load()  : 0.5f;
        const float fltAtk       = pFltAtk       ? pFltAtk->load()       : 0.01f;
        const float fltDec       = pFltDec       ? pFltDec->load()       : 0.3f;
        const float fltSus       = pFltSus       ? pFltSus->load()       : 0.0f;
        const float fltRel       = pFltRel       ? pFltRel->load()       : 0.4f;

        const float ampAtk       = pAmpAtk       ? pAmpAtk->load()       : 0.005f;
        const float ampDec       = pAmpDec       ? pAmpDec->load()       : 0.4f;
        const float ampSus       = pAmpSus       ? pAmpSus->load()       : 0.7f;
        const float ampRel       = pAmpRel       ? pAmpRel->load()       : 0.6f;
        const float velTimbre    = pVelTimbre    ? pVelTimbre->load()    : 0.6f;

        // LFO rates: enforce floor 0.01 Hz (D005)
        const float lfo1Rate  = std::max(0.01f, pLfo1Rate  ? pLfo1Rate->load()  : 0.5f);
        const float lfo1Depth = pLfo1Depth ? pLfo1Depth->load() : 0.0f;
        const int   lfo1Shape = pLfo1Shape ? static_cast<int>(pLfo1Shape->load()) : 0;
        const int   lfo1Tgt   = pLfo1Target? static_cast<int>(pLfo1Target->load()): 0;

        const float lfo2Rate  = std::max(0.01f, pLfo2Rate  ? pLfo2Rate->load()  : 0.12f);
        const float lfo2Depth = pLfo2Depth ? pLfo2Depth->load() : 0.0f;
        const int   lfo2Shape = pLfo2Shape ? static_cast<int>(pLfo2Shape->load()) : 2;
        const int   lfo2Tgt   = pLfo2Target? static_cast<int>(pLfo2Target->load()): 1;

        const int   voiceMode = pVoiceMode ? static_cast<int>(pVoiceMode->load()) : 2;
        const float glideTime = pGlide     ? pGlide->load()     : 0.0f;
        const int   glideMode = pGlideMode ? static_cast<int>(pGlideMode->load()) : 0;

        // Glide coefficient (USE IT — Lesson 8)
        const float glideCoeff = (glideTime > 0.0001f)
            ? fastExp(-1.0f / (glideTime * sampleRateFloat))
            : 0.0f;

        // Poisson lambda (events/sample)
        const float lambdaPerSample = (eventDensity > 0.0f)
            ? (eventDensity * 100.0f) / sampleRateFloat
            : 0.0f;

        // Breakpoint duration clamp
        // min: sampleRate/20000 (prevents aliasing above 20kHz)
        // max: sampleRate/20 (ensures waveform stays in audible range)
        const float bpDurMin = sampleRateFloat / 20000.0f;
        const float bpDurMax = sampleRateFloat / 20.0f;

        // EnvToMorph coupling modulates scatter (Distribution width)
        scatter = std::clamp(scatter + std::fabs(couplingEnvScatter) * 0.5f, 0.0f, 2.0f);
        // Store for coupling output (ch2)
        lastScatter = std::clamp(scatter * 0.5f, 0.0f, 1.0f);

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
        // Destinations: 0=Off, 1=Filter Cutoff, 2=Scatter, 3=Intent,
        //               4=Event Density, 5=Mutation Rate, 6=Amp Level
        float modDestOffsets[7] = {};
        modMatrix.apply(blockModSrc, modDestOffsets);

        baseCutoff    = std::clamp(baseCutoff   + modDestOffsets[1] * 8000.0f, 20.0f, 20000.0f);
        scatter       = std::clamp(scatter      + modDestOffsets[2] * 0.5f,    0.0f,  2.0f);
        intent        = std::clamp(intent       + modDestOffsets[3],           0.0f,  1.0f);
        eventDensity  = std::clamp(eventDensity + modDestOffsets[4],           0.0f,  1.0f);
        mutationRate  = std::clamp(mutationRate + modDestOffsets[5],           0.0f,  2.0f);
        const float modAmpLevel = modDestOffsets[6];

        // CC1 mod wheel -> scatter (D006: expression input)
        scatter = std::clamp(scatter + modWheelValue * 0.3f, 0.0f, 2.0f);
        // Aftertouch -> intent (timbral change, D001/D006)
        intent  = std::clamp(intent  + aftertouchValue * 0.3f, 0.0f, 1.0f);

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
                              bpCount, scatter, distType, intent, mutationRate,
                              ampScatter, timeScatter, stepFloor,
                              symmetry, foldAmt, dcBlock, pitchTrack,
                              markovMix, stateMemory, convergePitch,
                              lambdaPerSample, eventAmp, eventDecay, densityJitter,
                              bpDurMin, bpDurMax,
                              baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                              fltAtk, fltDec, fltSus, fltRel,
                              ampAtk, ampDec, ampSus, ampRel, velTimbre,
                              lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                              lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                              glideCoeff, glideMode, modAmpLevel);
            midiSamplePos = msgPos;

            if (msg.isNoteOn())
            {
                handleNoteOn(msg.getNoteNumber(), msg.getVelocity(),
                             ampAtk, ampDec, ampSus, ampRel,
                             fltAtk, fltDec, fltSus, fltRel,
                             bpCount, voiceMode, glideCoeff, glideMode, resetTrig);
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
                          bpCount, scatter, distType, intent, mutationRate,
                          ampScatter, timeScatter, stepFloor,
                          symmetry, foldAmt, dcBlock, pitchTrack,
                          markovMix, stateMemory, convergePitch,
                          lambdaPerSample, eventAmp, eventDecay, densityJitter,
                          bpDurMin, bpDurMax,
                          baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                          fltAtk, fltDec, fltSus, fltRel,
                          ampAtk, ampDec, ampSus, ampRel, velTimbre,
                          lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                          lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                          glideCoeff, glideMode, modAmpLevel);

        // ---- Cache last output samples for coupling ----
        if (numSamples > 0)
        {
            lastSampleL = writeL[numSamples - 1];
            lastSampleR = writeR[numSamples - 1];
        }

        // ---- Coupling accumulator decay (0.999x per block — Lesson 10) ----
        couplingAmpFilter  *= 0.999f;
        couplingEnvScatter *= 0.999f;
        couplingRhythm     *= 0.999f;
        couplingFMBuf      *= 0.999f;

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
    // PRNG — xorshift32 (fast, no allocation, per-voice)
    //==========================================================================
    static inline float xorRand(uint32_t& state) noexcept
    {
        state ^= state << 13u;
        state ^= state >> 17u;
        state ^= state << 5u;
        return static_cast<float>(state) * 2.3283064365e-10f; // [0, 1)
    }

    //==========================================================================
    // randomStep — generate one random step with the selected distribution.
    //
    // intent:     0 = Uniform, 1 = full shaped distribution
    // scale:      overall step magnitude
    // stepFloor:  minimum step magnitude (when non-zero)
    //
    // D003: Cauchy uses proper heavy-tail formula with hard clamp at +-4*scale.
    //==========================================================================
    static float randomStep(uint32_t& rng, int distType, float scale, float intent,
                            float stepFloor) noexcept
    {
        const float u1 = std::max(1e-7f, xorRand(rng));
        const float u2 = xorRand(rng);

        // Uniform step (baseline for INTENT lerp)
        const float uRaw = xorRand(rng);
        const float uniformStep = (uRaw - 0.5f) * 2.0f * scale;

        float shapedStep = uniformStep;
        switch (distType)
        {
        case 0: // Gaussian — Box-Muller transform
        {
            float g = scale * std::sqrt(-2.0f * std::log(u1))
                            * fastCos(kOortTwoPi * u2);
            g = std::clamp(g, -4.0f * scale, 4.0f * scale);
            shapedStep = g;
            break;
        }
        case 1: // Cauchy — D003: gamma*tan(pi*(u-0.5)), hard clamp at +-4*scale
        {
            const float gamma = scale * 0.5f;
            float c = gamma * std::tan(kOortPi * (u1 - 0.5f));
            c = std::clamp(c, -4.0f * scale, 4.0f * scale);
            shapedStep = c;
            break;
        }
        case 2: // Logistic
        {
            const float s = scale * 0.3f;
            const float denom = std::max(1e-7f, 1.0f - u1);
            float l = s * std::log(u1 / denom);
            l = std::clamp(l, -4.0f * scale, 4.0f * scale);
            shapedStep = l;
            break;
        }
        case 3: // Uniform (explicit)
        default:
            shapedStep = uniformStep;
            break;
        }

        // INTENT morph: lerp from Uniform (intent=0) to shaped (intent=1)
        float step = lerp(uniformStep, shapedStep, intent);

        // Apply step floor (minimum step size)
        if (stepFloor > 0.0f && step != 0.0f && std::fabs(step) < stepFloor)
            step = (step > 0.0f) ? stepFloor : -stepFloor;

        return step;
    }

    //==========================================================================
    // mutateCycle — Markov random walk on all breakpoints after one full cycle
    //==========================================================================
    void mutateCycle(OortVoice& v, int bpCount, float scatter, int distType, float intent,
                     float ampScatterMult, float timeScatterMult, float stepFloor,
                     float markovMix, float stateMemory, float mutationRate,
                     float convergePitch, float basePeriod,
                     float bpDurMin, float bpDurMax, float symmetry) noexcept
    {
        // mutationRate gates how fully we apply the walk (probabilistic skip)
        const float muteGate = xorRand(v.rng);
        if (muteGate > mutationRate) return;

        for (int i = 0; i < bpCount; ++i)
        {
            // ---- Amplitude mutation ----
            float ampStep = randomStep(v.rng, distType, scatter * ampScatterMult, intent, stepFloor);
            // Markov: blend with memory of previous step
            const float finalAmpStep = lerp(ampStep, v.prevAmpStep * stateMemory, markovMix);
            v.prevAmpStep = finalAmpStep;
            float amp = v.breakpoints[i].amplitude + finalAmpStep;
            // Symmetry bias: shifts the clamp center
            amp += (symmetry - 0.5f) * 0.1f;
            amp = std::clamp(amp, -1.0f, 1.0f);
            v.breakpoints[i].amplitude = amp;

            // ---- Duration mutation ----
            float timeStep = randomStep(v.rng, distType, scatter * timeScatterMult, intent, stepFloor);
            const float finalTimeStep = lerp(timeStep, v.prevTimeStep * stateMemory, markovMix);
            v.prevTimeStep = finalTimeStep;
            float dur = v.breakpoints[i].duration
                      + finalTimeStep * std::max(1.0f, basePeriod) * 0.1f;

            // convergePitch: bias durations toward harmonic ratios of base period
            if (convergePitch > 0.0f && basePeriod > 0.0f && bpCount > 0)
            {
                const float harmTarget = basePeriod / static_cast<float>(bpCount);
                dur = lerp(dur, harmTarget, convergePitch * 0.3f);
            }

            dur = flushDenormal(dur);
            dur = std::clamp(dur, bpDurMin, bpDurMax);
            v.breakpoints[i].duration = dur;
        }
    }

    //==========================================================================
    // renderVoicesRange — render all active voices from [startSample, endSample)
    //==========================================================================
    void renderVoicesRange(float* writeL, float* writeR, int startSample, int endSample,
                           int bpCount, float scatter, int distType, float intent, float mutationRate,
                           float ampScatter, float timeScatter, float stepFloor,
                           float symmetry, float foldAmt, int dcBlock, float pitchTrack,
                           float markovMix, float stateMemory, float convergePitch,
                           float lambdaPerSample, float eventAmp, float eventDecay,
                           float densityJitter,
                           float bpDurMin, float bpDurMax,
                           float baseCutoff, float fltReso, int fltType,
                           float fltEnvAmt, float fltKeyTrack,
                           float fltAtk, float fltDec, float fltSus, float fltRel,
                           float ampAtk, float ampDec, float ampSus, float ampRel,
                           float velTimbre,
                           float lfo1Rate, float lfo1Depth, int lfo1Shape, int lfo1Tgt,
                           float lfo2Rate, float lfo2Depth, int lfo2Shape, int lfo2Tgt,
                           float glideCoeff, int /*glideMode*/,
                           float modAmpLevel) noexcept
    {
        if (startSample >= endSample) return;

        for (auto& v : voices)
        {
            if (!v.active) continue;

            // ---- Set LFO rates and shapes ----
            v.lfo1.setRate(lfo1Rate, sampleRateFloat);
            v.lfo1.setShape(lfo1Shape);
            v.lfo2.setRate(lfo2Rate, sampleRateFloat);
            v.lfo2.setShape(lfo2Shape);

            // ---- Update envelope parameters ----
            v.ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
            v.filterEnv.setParams(fltAtk, fltDec, fltSus, fltRel, sampleRateFloat);

            // ---- D001: Velocity -> Timbre (narrows scatter at high velocity) ----
            const float velScatter = scatter * (1.0f - velTimbre * v.velocity);

            // ---- Target frequency + base period ----
            const float targetFreq = midiToFreq(v.note);
            const float basePeriod = sampleRateFloat / std::max(1.0f, v.glideBaseFreq);

            // ---- Filter coefficients update counter ----
            int sampleIdx = startSample;

            for (int i = startSample; i < endSample; ++i, ++sampleIdx)
            {
                // ---- Tick LFOs every sample (Lesson 3: ALL smoothers per-sample) ----
                const float lfo1Val = v.lfo1.process();
                const float lfo2Val = v.lfo2.process();
                v.lastLfo1Val = lfo1Val;
                v.lastLfo2Val = lfo2Val;

                // ---- Apply LFO modulation to target parameters ----
                // lfoTarget: 0=Scatter, 1=Intent, 2=FilterCutoff, 3=EventDensity
                const float lfo1Effect = lfo1Val * lfo1Depth;
                const float lfo2Effect = lfo2Val * lfo2Depth;

                float sampScatter     = velScatter;
                float sampIntent      = intent;
                float sampCutOff      = baseCutoff;
                float sampLambda      = lambdaPerSample;

                switch (lfo1Tgt)
                {
                case 0: sampScatter = std::clamp(sampScatter + lfo1Effect * 0.5f, 0.0f, 2.0f); break;
                case 1: sampIntent  = std::clamp(sampIntent  + lfo1Effect,        0.0f, 1.0f); break;
                case 2: sampCutOff  = std::clamp(sampCutOff  + lfo1Effect * 4000.0f, 20.0f, 20000.0f); break;
                case 3: sampLambda  = std::clamp(sampLambda  + lfo1Effect * (100.0f / sampleRateFloat), 0.0f, 1.0f); break;
                default: break;
                }
                switch (lfo2Tgt)
                {
                case 0: sampScatter = std::clamp(sampScatter + lfo2Effect * 0.5f, 0.0f, 2.0f); break;
                case 1: sampIntent  = std::clamp(sampIntent  + lfo2Effect,        0.0f, 1.0f); break;
                case 2: sampCutOff  = std::clamp(sampCutOff  + lfo2Effect * 4000.0f, 20.0f, 20000.0f); break;
                case 3: sampLambda  = std::clamp(sampLambda  + lfo2Effect * (100.0f / sampleRateFloat), 0.0f, 1.0f); break;
                default: break;
                }

                // ---- Glide: smooth pitch toward target (USE IT — Lesson 8) ----
                if (glideCoeff > 0.0f)
                {
                    v.glideBaseFreq = lerp(targetFreq, v.glideBaseFreq, glideCoeff);
                    v.glideBaseFreq = flushDenormal(v.glideBaseFreq);
                }
                else
                {
                    v.glideBaseFreq = targetFreq;
                }

                // ---- GENDY waveform sample generation ----
                // Current segment duration; guard against zero
                const float dur = std::max(1.0f, v.breakpoints[v.bpIdx].duration);

                // Phase increment — with optional pitch tracking
                float phaseInc = 1.0f / dur;
                if (pitchTrack > 0.0f)
                {
                    // Compute total cycle duration (sum of all breakpoint durations)
                    float totalDur = 0.0f;
                    for (int b = 0; b < bpCount; ++b)
                        totalDur += v.breakpoints[b].duration;
                    totalDur = std::max(1.0f, totalDur);

                    // Target cycle duration from MIDI pitch
                    const float targetCycLen = (v.glideBaseFreq > 0.0f)
                        ? (sampleRateFloat / v.glideBaseFreq)
                        : basePeriod;

                    // Scale phaseInc so that total cycle = targetCycLen
                    const float scaleFactor = totalDur / std::max(1.0f, targetCycLen);
                    phaseInc *= lerp(1.0f, scaleFactor, pitchTrack);
                    phaseInc = std::max(1e-7f, phaseInc);
                }

                // AudioToFM coupling: FM-like modulation of period
                if (std::fabs(v.fmCouplingAccum) > 1e-6f)
                {
                    phaseInc *= (1.0f + v.fmCouplingAccum * 0.5f);
                    phaseInc = std::max(1e-7f, phaseInc);
                }

                v.bpPhase += phaseInc;

                // Interpolated amplitude between current and next breakpoint
                const int nextBpIdx = (v.bpIdx + 1) % bpCount;
                float sample = lerp(v.breakpoints[v.bpIdx].amplitude,
                                    v.breakpoints[nextBpIdx].amplitude,
                                    std::clamp(v.bpPhase, 0.0f, 1.0f));

                // ---- Advance to next breakpoint if phase crossed 1.0 ----
                if (v.bpPhase >= 1.0f)
                {
                    v.bpPhase -= 1.0f;
                    v.bpIdx = nextBpIdx;

                    // If we've completed a full cycle (wrapped to index 0), mutate
                    if (v.bpIdx == 0)
                    {
                        mutateCycle(v, bpCount, sampScatter, distType, sampIntent,
                                    ampScatter, timeScatter, stepFloor,
                                    markovMix, stateMemory, mutationRate,
                                    convergePitch, basePeriod,
                                    bpDurMin, bpDurMax, symmetry);
                    }
                }

                // ---- Poisson event overlay ----
                if (sampLambda > 0.0f)
                {
                    v.poissonTimer -= 1.0f;
                    if (v.poissonTimer <= 0.0f)
                    {
                        // Trigger a new grain event
                        v.grain.active    = true;
                        v.grain.amplitude = eventAmp;
                        v.grain.level     = 1.0f;
                        const float decaySamples = std::max(1.0f, eventDecay * sampleRateFloat);
                        v.grain.decayCoeff = 1.0f - fastExp(-1.0f / decaySamples);

                        // Poisson inter-event: exponential(1/lambda), scaled
                        const float u = std::max(1e-7f, xorRand(v.rng));
                        const float meanInterval = 1.0f / std::max(1e-7f, sampLambda);
                        const float jitter = densityJitter * meanInterval
                                           * (xorRand(v.rng) - 0.5f) * 2.0f;
                        v.poissonTimer = std::max(1.0f, -std::log(u) * meanInterval + jitter);
                    }

                    if (v.grain.active)
                    {
                        sample += v.grain.level * v.grain.amplitude;
                        v.grain.level -= v.grain.level * v.grain.decayCoeff;
                        v.grain.level  = flushDenormal(v.grain.level);
                        if (v.grain.level < 1e-5f)
                        {
                            v.grain.active = false;
                            v.grain.level  = 0.0f;
                        }
                    }
                }

                // ---- Wavefold ----
                if (foldAmt > 0.0f)
                {
                    const float foldGain = 1.0f + foldAmt * 3.0f;
                    sample *= foldGain;
                    // Fold by reflecting outside [-1, 1]
                    while (sample >  1.0f) sample = 2.0f  - sample;
                    while (sample < -1.0f) sample = -2.0f - sample;
                }

                // ---- DC blocker: first-order HPF at ~5 Hz (Lesson: essential for Cauchy) ----
                // y[n] = x[n] - x[n-1] + 0.995 * y[n-1]
                if (dcBlock == 1)
                {
                    const float dcOut = sample - v.dcBlockX + 0.995f * v.dcBlockY;
                    v.dcBlockX = sample;
                    v.dcBlockY = flushDenormal(dcOut);
                    sample = dcOut;
                }

                // ---- Filter coefficient update every 16 samples (Lesson 4) ----
                if ((sampleIdx & 15) == 0)
                {
                    const float fltEnvLevel = v.filterEnv.getLevel();
                    const float keyTrackHz  = fltKeyTrack * v.keyTrack * 2000.0f;
                    float cutoff = sampCutOff
                                 + fltEnvAmt * fltEnvLevel * 8000.0f
                                 + keyTrackHz;
                    cutoff = std::clamp(cutoff, 20.0f, 20000.0f);

                    CytomicSVF::Mode mode = CytomicSVF::Mode::LowPass;
                    switch (fltType)
                    {
                    case 1: mode = CytomicSVF::Mode::HighPass; break;
                    case 2: mode = CytomicSVF::Mode::BandPass; break;
                    case 3: mode = CytomicSVF::Mode::Notch;    break;
                    default: break;
                    }
                    v.filterL.setMode(mode);
                    v.filterR.setMode(mode);
                    v.filterL.setCoefficients_fast(cutoff, fltReso, sampleRateFloat);
                    v.filterR.setCoefficients_fast(cutoff, fltReso, sampleRateFloat);
                }

                // Tick filter envelope every sample (Lesson 3)
                v.filterEnv.process();

                // ---- Filter ----
                const float sampleL = v.filterL.processSample(sample);
                const float sampleR = v.filterR.processSample(sample);

                // ---- Amp envelope ----
                const float ampLevel = v.ampEnv.process();

                // Deactivate voice when envelope finishes
                if (!v.ampEnv.isActive())
                {
                    v.active    = false;
                    v.releasing = false;
                }

                // ---- VCA: amp envelope * (1 + amp mod offset) ----
                const float gain = ampLevel * std::clamp(1.0f + modAmpLevel * 0.5f, 0.0f, 2.0f);

                writeL[i] += sampleL * gain;
                writeR[i] += sampleR * gain;
            }
        }
    }

    //==========================================================================
    // handleNoteOn
    //==========================================================================
    void handleNoteOn(int note, int midiVel,
                      float ampAtk, float ampDec, float ampSus, float ampRel,
                      float fltAtk, float fltDec, float fltSus, float fltRel,
                      int bpCount, int voiceMode, float glideCoeff, int glideMode,
                      int resetTrig) noexcept
    {
        const float vel = static_cast<float>(midiVel) / 127.0f;
        const float targetFreq = midiToFreq(note);

        // Determine max polyphony from voiceMode
        // 0=Mono, 1=Legato, 2=Poly4, 3=Poly8
        const int maxVoices = (voiceMode == 0 || voiceMode == 1) ? 1
                            : (voiceMode == 2) ? 4
                            : 8;

        // ---- Voice allocation ----
        OortVoice* target = nullptr;

        if (voiceMode == 0 || voiceMode == 1)
        {
            // Mono/Legato: always use voice 0
            target = &voices[0];
        }
        else
        {
            // Poly: find free voice
            for (int vi = 0; vi < maxVoices && vi < kOortMaxVoices; ++vi)
            {
                if (!voices[vi].active)
                {
                    target = &voices[vi];
                    break;
                }
            }
            // No free voice: steal a releasing voice
            if (target == nullptr)
            {
                for (int vi = 0; vi < maxVoices && vi < kOortMaxVoices; ++vi)
                {
                    if (voices[vi].releasing)
                    {
                        target = &voices[vi];
                        break;
                    }
                }
            }
            // Still none: steal voice 0 (oldest)
            if (target == nullptr)
                target = &voices[0];
        }

        if (target == nullptr) return;

        const bool isLegato = (voiceMode == 1) && target->active;
        const float prevFreq = target->glideBaseFreq;

        // ---- Reset/retrigger breakpoints ----
        if (resetTrig == 1 || !isLegato)
        {
            const float initPeriod = sampleRateFloat / std::max(1.0f, targetFreq);
            const float durPerBP   = initPeriod / std::max(1.0f, static_cast<float>(bpCount));
            for (int b = 0; b < bpCount && b < kOortMaxBP; ++b)
            {
                const float phase = static_cast<float>(b) / static_cast<float>(bpCount);
                target->breakpoints[b].amplitude = fastSin(phase * kOortTwoPi);
                target->breakpoints[b].duration  = std::max(1.0f, durPerBP);
            }
            target->bpIdx        = 0;
            target->bpPhase      = 0.0f;
            target->prevAmpStep  = 0.0f;
            target->prevTimeStep = 0.0f;
        }

        target->active    = true;
        target->releasing = false;
        target->note      = note;
        target->velocity  = vel;
        target->keyTrack  = (static_cast<float>(note) - 60.0f) / 60.0f;
        target->fmCouplingAccum = 0.0f;
        target->grain.active = false;

        // ---- Glide: set starting frequency ----
        // glideMode=0 (Legato): only glide on legato transitions
        // glideMode=1 (Always): always glide from previous pitch
        if (glideMode == 1 || isLegato)
        {
            target->glideBaseFreq = (prevFreq > 0.0f) ? prevFreq : targetFreq;
        }
        else
        {
            target->glideBaseFreq = targetFreq;
        }

        // ---- Trigger envelopes ----
        if (isLegato)
        {
            target->ampEnv.retriggerFrom(target->ampEnv.getLevel(),
                                         ampAtk, ampDec, ampSus, ampRel);
        }
        else
        {
            target->ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
            target->ampEnv.noteOn();
        }
        target->filterEnv.setParams(fltAtk, fltDec, fltSus, fltRel, sampleRateFloat);
        target->filterEnv.noteOn();

        // Randomise Poisson timer to avoid sync on attack
        target->poissonTimer = xorRand(target->rng) * 100.0f + 1.0f;
    }

    //==========================================================================
    // handleNoteOff
    //==========================================================================
    void handleNoteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && !v.releasing && v.note == note)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
            }
        }
    }

    //==========================================================================
    // Data members
    //==========================================================================

    float sampleRateFloat = 44100.0f;
    int   maxBlock = 512;

    std::array<OortVoice, kOortMaxVoices> voices;

    // Coupling accumulators
    float couplingFMBuf      = 0.0f;
    float couplingAmpFilter  = 0.0f;
    float couplingEnvScatter = 0.0f;
    float couplingRhythm     = 0.0f;

    // Modulation
    ModMatrix<4>           modMatrix;
    ModMatrix<4>::Sources  blockModSrc;

    // Expression inputs
    float modWheelValue   = 0.0f;
    float aftertouchValue = 0.0f;

    // Coupling output cache
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;
    float lastScatter = 0.0f; // ch2: current scatter intensity 0..1

    // activeVoiceCount_ is inherited from SynthEngine base class

    //==========================================================================
    // Parameter pointers (cached from APVTS, read-only on audio thread)
    //==========================================================================

    // Group A — GENDY Core
    std::atomic<float>* pBreakpoints   = nullptr;
    std::atomic<float>* pScatter       = nullptr;
    std::atomic<float>* pDistType      = nullptr;
    std::atomic<float>* pIntent        = nullptr;
    std::atomic<float>* pMutationRate  = nullptr;
    std::atomic<float>* pAmpScatter    = nullptr;
    std::atomic<float>* pTimeScatter   = nullptr;
    std::atomic<float>* pStepFloor     = nullptr;

    // Group B — Waveform Shaping
    std::atomic<float>* pSymmetry      = nullptr;
    std::atomic<float>* pFoldAmt       = nullptr;
    std::atomic<float>* pDCBlock       = nullptr;
    std::atomic<float>* pPitchTrack    = nullptr;

    // Group C — Markov Chain
    std::atomic<float>* pMarkovMix     = nullptr;
    std::atomic<float>* pStateMemory   = nullptr;
    std::atomic<float>* pResetTrigger  = nullptr;
    std::atomic<float>* pConvergePitch = nullptr;

    // Group D — Event Density
    std::atomic<float>* pEventDensity  = nullptr;
    std::atomic<float>* pEventAmp      = nullptr;
    std::atomic<float>* pEventDecay    = nullptr;
    std::atomic<float>* pDensityJitter = nullptr;

    // Group E — Filter + Filter Envelope
    std::atomic<float>* pFltCutoff     = nullptr;
    std::atomic<float>* pFltReso       = nullptr;
    std::atomic<float>* pFltType       = nullptr;
    std::atomic<float>* pFltEnvAmt     = nullptr;
    std::atomic<float>* pFltKeyTrack   = nullptr;
    std::atomic<float>* pFltAtk        = nullptr;
    std::atomic<float>* pFltDec        = nullptr;
    std::atomic<float>* pFltSus        = nullptr;
    std::atomic<float>* pFltRel        = nullptr;

    // Group F — Amp Envelope + VelTimbre
    std::atomic<float>* pAmpAtk        = nullptr;
    std::atomic<float>* pAmpDec        = nullptr;
    std::atomic<float>* pAmpSus        = nullptr;
    std::atomic<float>* pAmpRel        = nullptr;
    std::atomic<float>* pVelTimbre     = nullptr;

    // Group G — LFOs
    std::atomic<float>* pLfo1Rate      = nullptr;
    std::atomic<float>* pLfo1Depth     = nullptr;
    std::atomic<float>* pLfo1Shape     = nullptr;
    std::atomic<float>* pLfo1Target    = nullptr;
    std::atomic<float>* pLfo2Rate      = nullptr;
    std::atomic<float>* pLfo2Depth     = nullptr;
    std::atomic<float>* pLfo2Shape     = nullptr;
    std::atomic<float>* pLfo2Target    = nullptr;

    // Group I — Macros + Voice
    std::atomic<float>* pMacro1        = nullptr;
    std::atomic<float>* pMacro2        = nullptr;
    std::atomic<float>* pMacro3        = nullptr;
    std::atomic<float>* pMacro4        = nullptr;
    std::atomic<float>* pVoiceMode     = nullptr;
    std::atomic<float>* pGlide         = nullptr;
    std::atomic<float>* pGlideMode     = nullptr;
};

} // namespace xoceanus
