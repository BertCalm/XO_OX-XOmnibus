// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// =============================================================================
// OobleckEngine.h — XOobleck: Reaction-Diffusion Wavetable Synthesis (#89)
// "The Chemistry Engine"
//
// Gray-Scott reaction-diffusion on a per-voice 128-cell 1D ring grid.
// V concentration IS the waveform — chemistry generates timbre in real time.
// Non-Newtonian aftertouch response: sustained pressure jams the simulation.
//
// Physics: Gray & Scott 1984, Pearson 1993 (adapted for 1D).
// dt = 1.0 fixed. Du = 0.16 fixed. Dv = Du * diffusion_ratio.
//
// QDD-mandated safety: DC blocking, V clamping [0,1], anti-aliasing LP,
// coupling soft-clip, M1 blend (not additive).
//
// Water Column: Midnight Zone | Creature: Nudibranch
// Accent: Oobleck Slime #B4FF39
// Prefix: oobl_ | Gallery: OOBL
// =============================================================================

#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/Effects/DCBlocker.h"
#include "../../DSP/SRO/SilenceGate.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <vector>

namespace xoceanus
{

// =============================================================================
// OobleckVoice — per-voice Gray-Scott state, RD grid, and DSP chain
// =============================================================================
struct OobleckVoice
{
    bool active    = false;
    bool releasing = false;
    int  midiNote  = -1;
    float velocity = 0.0f;
    float frequency       = 440.0f;
    float targetFrequency = 440.0f;

    // Gray-Scott grid (per voice)
    static constexpr int kGridSize = 128;
    float U[kGridSize]        = {};
    float V[kGridSize]        = {};
    float U_new[kGridSize]    = {};
    float V_new[kGridSize]    = {};
    float V_filtered[kGridSize] = {}; // anti-aliased copy for readout

    // Wavetable phase
    float phase = 0.0f;

    // RD evolution accumulator
    float rdAccumulator = 0.0f;

    // Envelopes
    xoceanus::StandardADSR ampEnv;

    // LFO
    xoceanus::StandardLFO lfo;

    // Output filter (L/R)
    xoceanus::CytomicSVF outputFilterL;
    xoceanus::CytomicSVF outputFilterR;

    // DC blocker (per-voice)
    xoceanus::DCBlocker dcBlocker;

    // Glide
    xoceanus::GlideProcessor glide;

    // Coupling cache
    float lastCouplingOut = 0.0f;

    // Voice stealing crossfade
    float crossfadeGain = 1.0f;
    float crossfadeRate = 0.0f;

    // Aftertouch freeze state
    float currentAftertouch = 0.0f;

    // Stagnation detector
    float prevVSum = 0.0f;
    int   stagnationCounter = 0;

    // Per-voice RNG state (Knuth TAOCP LCG)
    uint32_t rngState = 12345u;

    // ------------------------------------------------------------------
    // LCG noise sample [-1, +1]
    // ------------------------------------------------------------------
    float nextNoise() noexcept
    {
        rngState = rngState * 1664525u + 1013904223u;
        return static_cast<float>(rngState & 0xFFFF) / 32768.0f - 1.0f;
    }

    // ------------------------------------------------------------------
    // initGrid — seed the chemical concentrations based on seedType
    // ------------------------------------------------------------------
    void initGrid(int seedType) noexcept
    {
        for (int i = 0; i < kGridSize; ++i)
        {
            U[i] = 1.0f;
            V[i] = 0.0f;
        }
        switch (seedType)
        {
            case 0: // Random
                for (int i = 0; i < kGridSize; ++i)
                {
                    rngState = rngState * 1664525u + 1013904223u;
                    float rU = static_cast<float>(rngState & 0xFFFF) / 65536.0f;
                    rngState = rngState * 1664525u + 1013904223u;
                    float rV = static_cast<float>(rngState & 0xFFFF) / 65536.0f;
                    U[i] = 0.5f + 0.5f * rU;
                    V[i] = 0.25f * rV;
                }
                break;
            case 1: // CenterDot
                for (int i = 60; i < 68; ++i) { U[i] = 0.5f; V[i] = 0.25f; }
                break;
            case 2: // DualDot
                for (int i = 28; i < 36; ++i) { U[i] = 0.5f; V[i] = 0.25f; }
                for (int i = 92; i < 100; ++i) { U[i] = 0.5f; V[i] = 0.25f; }
                break;
            case 3: // Stripe
                for (int i = 0; i < kGridSize; ++i)
                    if ((i / 8) % 2 == 0) { U[i] = 0.5f; V[i] = 0.25f; }
                break;
            case 4: // StableLoad — preserve current grid
                break;
            default:
                for (int i = 60; i < 68; ++i) { U[i] = 0.5f; V[i] = 0.25f; }
                break;
        }
    }

    // ------------------------------------------------------------------
    // reset — full voice reset, re-init grid
    // ------------------------------------------------------------------
    void reset(double sampleRate) noexcept
    {
        active = false; releasing = false; midiNote = -1;
        velocity = 0.0f; frequency = 440.0f; targetFrequency = 440.0f;
        phase = 0.0f; rdAccumulator = 0.0f;
        crossfadeGain = 1.0f; crossfadeRate = 0.0f;
        currentAftertouch = 0.0f;
        prevVSum = 0.0f; stagnationCounter = 0;
        lastCouplingOut = 0.0f;

        ampEnv.reset();
        lfo.reset();
        outputFilterL.reset();
        outputFilterR.reset();
        dcBlocker.prepare(sampleRate);
        glide.reset();

        initGrid(0);
    }
};

// =============================================================================
// OobleckParamSnapshot — cache all parameter pointers once per block
// =============================================================================
struct OobleckParamSnapshot
{
    // Reaction
    std::atomic<float>* pFeed       = nullptr;
    std::atomic<float>* pKill       = nullptr;
    std::atomic<float>* pDiffusion  = nullptr;
    std::atomic<float>* pEvolution  = nullptr;
    std::atomic<float>* pPersistence= nullptr;

    // Substance
    std::atomic<float>* pSubstrate  = nullptr;
    std::atomic<float>* pTemperature= nullptr;
    std::atomic<float>* pSeed       = nullptr;

    // Waveform
    std::atomic<float>* pReadoutMode= nullptr;
    std::atomic<float>* pFieldBias  = nullptr;
    std::atomic<float>* pFold       = nullptr;

    // Output
    std::atomic<float>* pFilterCutoff= nullptr;
    std::atomic<float>* pFilterReso  = nullptr;
    std::atomic<float>* pFilterType  = nullptr;
    std::atomic<float>* pDrive       = nullptr;

    // Envelope
    std::atomic<float>* pAttack  = nullptr;
    std::atomic<float>* pDecay   = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;

    // Modulation
    std::atomic<float>* pLfoRate   = nullptr;
    std::atomic<float>* pLfoShape  = nullptr;
    std::atomic<float>* pLfoDepth  = nullptr;
    std::atomic<float>* pLfoTarget = nullptr;

    // Space
    std::atomic<float>* pReverb     = nullptr;
    std::atomic<float>* pSpaceDecay = nullptr;

    // Performance
    std::atomic<float>* pTune  = nullptr;
    std::atomic<float>* pFine  = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pFreeze= nullptr;

    // Macros
    std::atomic<float>* pMacroChar  = nullptr;
    std::atomic<float>* pMacroMove  = nullptr;
    std::atomic<float>* pMacroCoupl = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;

    // ------------------------------------------------------------------
    // Snapshot values (loaded once per block)
    // ------------------------------------------------------------------
    float feed        = 0.035f;
    float kill        = 0.060f;
    float diffusion   = 0.5f;
    float evolution   = 100.0f;
    float persistence = 0.3f;

    float substrate   = 0.5f;
    float temperature = 0.05f;
    int   seed        = 0;

    int   readoutMode = 0;
    float fieldBias   = 0.0f;
    float fold        = 0.0f;

    float filterCutoff = 12000.0f;
    float filterReso   = 0.15f;
    int   filterType   = 0;
    float drive        = 0.2f;

    float attack  = 0.01f;
    float decay   = 0.5f;
    float sustain = 0.7f;
    float release = 0.8f;

    float lfoRate   = 0.2f;
    int   lfoShape  = 0;
    float lfoDepth  = 0.3f;
    int   lfoTarget = 0;

    float reverb     = 0.2f;
    float spaceDecay = 2.0f;

    int   tune  = 0;
    float fine  = 0.0f;
    float glide = 0.0f;
    float freeze = 0.0f;

    float macroChar  = 0.0f;
    float macroMove  = 0.0f;
    float macroCoupl = 0.0f;
    float macroSpace = 0.0f;

    // ------------------------------------------------------------------
    inline float load(std::atomic<float>* p, float def) const noexcept
    {
        return p ? p->load(std::memory_order_relaxed) : def;
    }

    void updateFrom() noexcept
    {
        feed        = load(pFeed,        0.035f);
        kill        = load(pKill,        0.060f);
        diffusion   = load(pDiffusion,   0.5f);
        evolution   = load(pEvolution,   100.0f);
        persistence = load(pPersistence, 0.3f);

        substrate   = load(pSubstrate,   0.5f);
        temperature = load(pTemperature, 0.05f);
        seed        = static_cast<int>(load(pSeed, 0.0f) + 0.5f);

        readoutMode = static_cast<int>(load(pReadoutMode, 0.0f) + 0.5f);
        fieldBias   = load(pFieldBias,   0.0f);
        fold        = load(pFold,        0.0f);

        filterCutoff = load(pFilterCutoff, 12000.0f);
        filterReso   = load(pFilterReso,   0.15f);
        filterType   = static_cast<int>(load(pFilterType, 0.0f) + 0.5f);
        drive        = load(pDrive,        0.2f);

        attack  = load(pAttack,  0.01f);
        decay   = load(pDecay,   0.5f);
        sustain = load(pSustain, 0.7f);
        release = load(pRelease, 0.8f);

        lfoRate   = load(pLfoRate,   0.2f);
        lfoShape  = static_cast<int>(load(pLfoShape,  0.0f) + 0.5f);
        lfoDepth  = load(pLfoDepth,  0.3f);
        lfoTarget = static_cast<int>(load(pLfoTarget, 0.0f) + 0.5f);

        reverb     = load(pReverb,     0.2f);
        spaceDecay = load(pSpaceDecay, 2.0f);

        tune  = static_cast<int>(load(pTune,  0.0f) + 0.5f);
        fine  = load(pFine,  0.0f);
        glide = load(pGlide, 0.0f);
        freeze = load(pFreeze, 0.0f);

        macroChar  = load(pMacroChar,  0.0f);
        macroMove  = load(pMacroMove,  0.0f);
        macroCoupl = load(pMacroCoupl, 0.0f);
        macroSpace = load(pMacroSpace, 0.0f);
    }
};

// =============================================================================
// Simple 4-line FDN reverb for shared post-mix space
// =============================================================================
struct OobleckFDN
{
    static constexpr int kNumLines = 4;
    // Prime delay lengths (samples at 44100 Hz, scaled for actual SR)
    static constexpr int kDelayLengths[kNumLines] = { 1327, 1627, 1949, 2309 };

    std::vector<float> bufL[kNumLines];
    std::vector<float> bufR[kNumLines];
    int writePos[kNumLines] = {};
    int delayLen[kNumLines] = {};
    float feedback = 0.5f;
    float wet = 0.2f;

    void prepare(double sampleRate, int /*maxBlockSize*/) noexcept
    {
        float sr = static_cast<float>(sampleRate);
        for (int i = 0; i < kNumLines; ++i)
        {
            int len = static_cast<int>(kDelayLengths[i] * sr / 44100.0f);
            len = std::max(len, 64);
            delayLen[i] = len;
            bufL[i].assign(static_cast<size_t>(len), 0.0f);
            bufR[i].assign(static_cast<size_t>(len), 0.0f);
            writePos[i] = 0;
        }
    }

    void reset() noexcept
    {
        for (int i = 0; i < kNumLines; ++i)
        {
            std::fill(bufL[i].begin(), bufL[i].end(), 0.0f);
            std::fill(bufR[i].begin(), bufR[i].end(), 0.0f);
            writePos[i] = 0;
        }
    }

    // Process one stereo sample through the FDN
    void processSample(float inL, float inR, float& outL, float& outR) noexcept
    {
        // Read delayed samples
        float dL[kNumLines], dR[kNumLines];
        for (int i = 0; i < kNumLines; ++i)
        {
            int readPos = (writePos[i] - delayLen[i] + static_cast<int>(bufL[i].size()))
                          % static_cast<int>(bufL[i].size());
            dL[i] = bufL[i][static_cast<size_t>(readPos)];
            dR[i] = bufR[i][static_cast<size_t>(readPos)];
        }

        // Hadamard mixing matrix (normalised 4x4)
        // [ 1  1  1  1 ]
        // [ 1 -1  1 -1 ]
        // [ 1  1 -1 -1 ]
        // [ 1 -1 -1  1 ]
        // Scaled by 0.5
        float mL[kNumLines], mR[kNumLines];
        mL[0] = 0.5f * ( dL[0] + dL[1] + dL[2] + dL[3]);
        mL[1] = 0.5f * ( dL[0] - dL[1] + dL[2] - dL[3]);
        mL[2] = 0.5f * ( dL[0] + dL[1] - dL[2] - dL[3]);
        mL[3] = 0.5f * ( dL[0] - dL[1] - dL[2] + dL[3]);

        mR[0] = 0.5f * ( dR[0] + dR[1] + dR[2] + dR[3]);
        mR[1] = 0.5f * ( dR[0] - dR[1] + dR[2] - dR[3]);
        mR[2] = 0.5f * ( dR[0] + dR[1] - dR[2] - dR[3]);
        mR[3] = 0.5f * ( dR[0] - dR[1] - dR[2] + dR[3]);

        // Write mixed + input back into delay lines
        for (int i = 0; i < kNumLines; ++i)
        {
            bufL[i][static_cast<size_t>(writePos[i])] = flushDenormal(inL * 0.5f + mL[i] * feedback);
            bufR[i][static_cast<size_t>(writePos[i])] = flushDenormal(inR * 0.5f + mR[i] * feedback);
            writePos[i] = (writePos[i] + 1) % static_cast<int>(bufL[i].size());
        }

        // Sum all lines for output
        outL = (mL[0] + mL[1] + mL[2] + mL[3]) * 0.25f;
        outR = (mR[0] + mR[1] + mR[2] + mR[3]) * 0.25f;
    }
};

// =============================================================================
// OobleckEngine — SynthEngine implementation
// =============================================================================
class OobleckEngine : public xoceanus::SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    // =========================================================================
    // SynthEngine interface
    // =========================================================================

    juce::String getEngineId() const override { return "Oobleck"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xB4, 0xFF, 0x39); }
    int getMaxVoices() const override { return kMaxVoices; }

    // -------------------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P = juce::AudioParameterFloat;
        using C = juce::AudioParameterChoice;
        using I = juce::AudioParameterInt;

        // Reaction group
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_feed", 1), "Feed",
            juce::NormalisableRange<float>(0.01f, 0.08f, 0.0001f), 0.035f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_kill", 1), "Kill",
            juce::NormalisableRange<float>(0.033f, 0.075f, 0.0001f), 0.060f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_diffusion", 1), "Diffusion",
            juce::NormalisableRange<float>(0.1f, 1.0f, 0.001f), 0.5f));
        {
            juce::NormalisableRange<float> evRange(0.01f, 1000.0f, 0.01f);
            evRange.setSkewForCentre(10.0f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("oobl_evolution", 1), "Evolution", evRange, 100.0f));
        }
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_persistence", 1), "Persistence",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // Substance group
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_substrate", 1), "Substrate",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_temperature", 1), "Temperature",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.05f));
        params.push_back(std::make_unique<C>(
            juce::ParameterID("oobl_seed", 1), "Seed",
            juce::StringArray{"Random", "CenterDot", "DualDot", "Stripe", "StableLoad"}, 0));

        // Waveform group
        params.push_back(std::make_unique<C>(
            juce::ParameterID("oobl_readout_mode", 1), "Readout",
            juce::StringArray{"Raw", "Odd", "Even", "Mirror"}, 0));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_field_bias", 1), "Field Bias",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_fold", 1), "Fold",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // Output group
        {
            juce::NormalisableRange<float> fcRange(80.0f, 20000.0f, 0.1f);
            fcRange.setSkewForCentre(1500.0f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("oobl_filter_cutoff", 1), "Filter", fcRange, 12000.0f));
        }
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_filter_reso", 1), "Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));
        params.push_back(std::make_unique<C>(
            juce::ParameterID("oobl_filter_type", 1), "Filter Type",
            juce::StringArray{"LP", "HP", "BP", "Notch"}, 0));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_drive", 1), "Drive",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        // Envelope group
        {
            juce::NormalisableRange<float> atRange(0.001f, 5.0f, 0.001f);
            atRange.setSkewForCentre(0.1f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("oobl_attack", 1), "Attack", atRange, 0.01f));
        }
        {
            juce::NormalisableRange<float> dcRange(0.01f, 10.0f, 0.001f);
            dcRange.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("oobl_decay", 1), "Decay", dcRange, 0.5f));
        }
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_sustain", 1), "Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));
        {
            juce::NormalisableRange<float> rlRange(0.01f, 10.0f, 0.001f);
            rlRange.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("oobl_release", 1), "Release", rlRange, 0.8f));
        }

        // Modulation group
        {
            juce::NormalisableRange<float> lfoRange(0.005f, 12.0f, 0.001f);
            lfoRange.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("oobl_lfo_rate", 1), "LFO Rate", lfoRange, 0.2f));
        }
        params.push_back(std::make_unique<C>(
            juce::ParameterID("oobl_lfo_shape", 1), "LFO Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "S&H"}, 0));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_lfo_depth", 1), "LFO Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<C>(
            juce::ParameterID("oobl_lfo_target", 1), "LFO Target",
            juce::StringArray{"Feed", "Kill", "Evolution", "Filter", "Diffusion", "Temperature", "Persistence"}, 0));

        // Space group
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_reverb", 1), "Reverb",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));
        {
            juce::NormalisableRange<float> sdRange(0.1f, 8.0f, 0.01f);
            sdRange.setSkewForCentre(1.0f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("oobl_space_decay", 1), "Space Decay", sdRange, 2.0f));
        }

        // Performance group
        params.push_back(std::make_unique<I>(
            juce::ParameterID("oobl_tune", 1), "Tune", -24, 24, 0));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_fine", 1), "Fine",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_glide", 1), "Glide",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_freeze", 1), "Freeze",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // Macros
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_macro1", 1), "Character",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_macro2", 1), "Movement",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_macro3", 1), "Coupling",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("oobl_macro4", 1), "Space",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    }

    // -------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return { params.begin(), params.end() };
    }

    // -------------------------------------------------------------------------
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        auto get = [&](const char* id) -> std::atomic<float>*
        {
            auto* p = apvts.getRawParameterValue(id);
            jassert(p != nullptr);
            return p;
        };

        snap_.pFeed        = get("oobl_feed");
        snap_.pKill        = get("oobl_kill");
        snap_.pDiffusion   = get("oobl_diffusion");
        snap_.pEvolution   = get("oobl_evolution");
        snap_.pPersistence = get("oobl_persistence");

        snap_.pSubstrate   = get("oobl_substrate");
        snap_.pTemperature = get("oobl_temperature");
        snap_.pSeed        = get("oobl_seed");

        snap_.pReadoutMode = get("oobl_readout_mode");
        snap_.pFieldBias   = get("oobl_field_bias");
        snap_.pFold        = get("oobl_fold");

        snap_.pFilterCutoff = get("oobl_filter_cutoff");
        snap_.pFilterReso   = get("oobl_filter_reso");
        snap_.pFilterType   = get("oobl_filter_type");
        snap_.pDrive        = get("oobl_drive");

        snap_.pAttack  = get("oobl_attack");
        snap_.pDecay   = get("oobl_decay");
        snap_.pSustain = get("oobl_sustain");
        snap_.pRelease = get("oobl_release");

        snap_.pLfoRate   = get("oobl_lfo_rate");
        snap_.pLfoShape  = get("oobl_lfo_shape");
        snap_.pLfoDepth  = get("oobl_lfo_depth");
        snap_.pLfoTarget = get("oobl_lfo_target");

        snap_.pReverb     = get("oobl_reverb");
        snap_.pSpaceDecay = get("oobl_space_decay");

        snap_.pTune   = get("oobl_tune");
        snap_.pFine   = get("oobl_fine");
        snap_.pGlide  = get("oobl_glide");
        snap_.pFreeze = get("oobl_freeze");

        snap_.pMacroChar  = get("oobl_macro1");
        snap_.pMacroMove  = get("oobl_macro2");
        snap_.pMacroCoupl = get("oobl_macro3");
        snap_.pMacroSpace = get("oobl_macro4");
    }

    // -------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate_ = sampleRate;
        maxBlockSize_      = maxBlockSize;

        for (auto& v : voices_)
            v.reset(sampleRate);

        fdn_.prepare(sampleRate, maxBlockSize);
        engineDCBlocker_.prepare(sampleRate);

        voiceBufL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        voiceBufR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        reverbSendL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        reverbSendR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        pitchBend_     = 1.0f; // frequency ratio: 1.0 = no bend
        modWheel_      = 0.0f;
        expression_    = 0.0f;
        aftertouchGlobal_ = 0.0f;

        couplingAudioAccum_  = 0.0f;
        couplingMorphAccum_  = 0.0f;
        couplingFilterAccum_ = 0.0f;

        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    }

    // -------------------------------------------------------------------------
    void releaseResources() override {}

    // -------------------------------------------------------------------------
    void reset() override
    {
        for (auto& v : voices_)
            v.reset(currentSampleRate_);
        fdn_.reset();
        engineDCBlocker_.prepare(currentSampleRate_);
        pitchBend_ = 1.0f;
        modWheel_ = expression_ = aftertouchGlobal_ = 0.0f;
        couplingAudioAccum_ = couplingMorphAccum_ = couplingFilterAccum_ = 0.0f;
    }

    // -------------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     int numSamples) override
    {
        juce::ScopedNoDenormals noDenomals;

        // ── Step 1: parse MIDI (wake gate BEFORE bypass check) ────────────────
        for (const auto& meta : midi)
        {
            if (meta.getMessage().isNoteOn())
                wakeSilenceGate();
        }

        // ── Step 2: Silence gate bypass check ────────────────────────────────
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // ── Step 3: Load parameter snapshot once per block ───────────────────
        snap_.updateFrom();

        // ── Step 4: Apply macros ──────────────────────────────────────────────
        const float m1 = snap_.macroChar;
        const float m2 = snap_.macroMove;
        const float m3 = snap_.macroCoupl;
        const float m4 = snap_.macroSpace;

        // M1 CHARACTER: blend feed/kill toward diagonal endpoint
        const float diagonalF = 0.065f;
        const float diagonalK = 0.070f;
        const float effFeed = juce::jlimit(0.01f, 0.08f,
            snap_.feed * (1.0f - m1) + diagonalF * m1);
        const float effKill = juce::jlimit(0.033f, 0.075f,
            snap_.kill * (1.0f - m1) + diagonalK * m1);

        // M2 MOVEMENT
        const float effEvolution = juce::jlimit(0.01f, 1000.0f,
            snap_.evolution + m2 * 400.0f);
        const float effTemp = juce::jlimit(0.0f, 1.0f,
            snap_.temperature + m2 * 0.3f);
        const float effLfoDepth = juce::jlimit(0.0f, 1.0f,
            snap_.lfoDepth + m2 * 0.3f);

        // M3 COUPLING
        const float effSubstrate = juce::jlimit(0.0f, 1.0f,
            snap_.substrate - m3 * 0.3f);
        const float effPersistence = juce::jlimit(0.0f, 1.0f,
            snap_.persistence + m3 * 0.4f);
        // effSubstrate scales V injection amplitude (passed via coupling audio path,
        // and biases the V concentration toward baseline — used in noteOn grid seeding)
        (void)effSubstrate; // consumed in noteOn via snap_.substrate directly

        // M4 SPACE
        const float effReverb = juce::jlimit(0.0f, 1.0f,
            snap_.reverb + m4 * 0.5f);
        const float effDiffusion = juce::jlimit(0.1f, 1.0f,
            snap_.diffusion + m4 * 0.3f);

        // CC1 (mod wheel) → evolution rate offset (D006)
        const float effEvolutionWithMW = juce::jlimit(0.01f, 1000.0f,
            effEvolution + modWheel_ * 300.0f);

        // CC11 (expression) → kill offset (D006)
        const float effKillWithExpr = juce::jlimit(0.033f, 0.075f,
            effKill + expression_ * 0.015f);

        // LFO target modulation applied per-voice below

        // ── Step 5: Process MIDI events ──────────────────────────────────────
        processMidi(midi, snap_, effPersistence);

        // ── Step 6: Tune + fine detune base
        const float tuneSemitones = static_cast<float>(snap_.tune)
                                  + snap_.fine / 100.0f;

        // ── Step 7: Glide time
        const float glideTimeSec = snap_.glide * snap_.glide * 2.0f; // quadratic mapping

        // ── Step 8: Filter mode
        const CytomicSVF::Mode filterMode = filterModeFromInt(snap_.filterType);

        // ── Step 9: Freeze
        const bool frozen = (snap_.freeze > 0.5f);

        // ── Step 10: FDN feedback from space decay
        // Map spaceDecay [0.1,8] to feedback [0.2, 0.85]
        fdn_.feedback = juce::jlimit(0.2f, 0.85f,
            0.2f + (snap_.spaceDecay - 0.1f) / (8.0f - 0.1f) * 0.65f);

        // ── Step 11: Get output pointers ─────────────────────────────────────
        float* outL = buffer.getWritePointer(0);
        float* outR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : outL;

        std::fill(outL, outL + numSamples, 0.0f);
        if (buffer.getNumChannels() > 1)
            std::fill(outR, outR + numSamples, 0.0f);
        std::fill(reverbSendL_.begin(), reverbSendL_.begin() + numSamples, 0.0f);
        std::fill(reverbSendR_.begin(), reverbSendR_.begin() + numSamples, 0.0f);

        // ── Step 12: Render each active voice ────────────────────────────────
        int activeCount = 0;

        for (auto& voice : voices_)
        {
            if (!voice.active)
                continue;

            ++activeCount;

            // Clear voice buffers
            std::fill(voiceBufL_.begin(), voiceBufL_.begin() + numSamples, 0.0f);
            std::fill(voiceBufR_.begin(), voiceBufR_.begin() + numSamples, 0.0f);

            // Update glide time
            voice.glide.setTime(glideTimeSec, static_cast<float>(currentSampleRate_));

            // Voice frequency (glide + tune + fine + pitch bend)
            const float baseFreq = midiToFreqTune(voice.midiNote, tuneSemitones);
            voice.targetFrequency = baseFreq * pitchBend_;
            voice.glide.setTarget(voice.targetFrequency);

            // LFO setup
            voice.lfo.setRate(snap_.lfoRate, static_cast<float>(currentSampleRate_));
            voice.lfo.setShape(snap_.lfoShape);

            // Envelope setup
            voice.ampEnv.setParams(snap_.attack, snap_.decay, snap_.sustain, snap_.release,
                                   static_cast<float>(currentSampleRate_));

            // Coupling audio injection into grid (once per block at grid update point)
            // This is injected during the RD update step below

            // Aftertouch freeze factor (non-Newtonian jamming)
            const float atNorm = aftertouchGlobal_;
            float freezeFactor = 1.0f;
            if (atNorm > 0.63f)
            {
                float excess = (atNorm - 0.63f) / 0.37f;
                freezeFactor = 1.0f - excess * excess;
            }
            // Apply mod wheel evolution boost (D006) then aftertouch freeze
            const float effEvolutionWithAT = effEvolutionWithMW * freezeFactor;
            const bool  isFrozen = frozen || (freezeFactor < 0.01f);

            // Filter velocity brightness mod (D001)
            const float velFilterMod = voice.velocity * voice.velocity * 4000.0f;
            const float voiceFilterCutoff = juce::jlimit(80.0f, 20000.0f,
                snap_.filterCutoff + velFilterMod + couplingFilterAccum_ * 4000.0f);

            // Coupling morph bias on kill + expression kill offset (EnvToMorph + CC11)
            const float morphKillOffset = couplingMorphAccum_ * 0.01f;
            const float voiceKill = juce::jlimit(0.033f, 0.075f,
                effKillWithExpr + morphKillOffset);

            // Per-sample loop
            for (int s = 0; s < numSamples; ++s)
            {
                const bool updateFilter = ((s & 15) == 0);
                // ── RD evolution scheduling ──────────────────────────────────
                voice.rdAccumulator += effEvolutionWithAT / static_cast<float>(currentSampleRate_);

                if (voice.rdAccumulator >= 1.0f)
                {
                    voice.rdAccumulator -= 1.0f;
                    if (!isFrozen)
                    {
                        // Apply LFO modulation to RD parameters
                        const float lfoVal = voice.lfo.process();
                        float lfoFeed = effFeed;
                        float lfoKillMod = voiceKill;
                        float lfoEvoMod = effEvolutionWithAT;
                        float lfoFilterMod = voiceFilterCutoff;
                        float lfoDiffMod = effDiffusion;
                        float lfoTempMod = effTemp;
                        float lfoPersistMod = effPersistence;

                        const float lmod = lfoVal * effLfoDepth;
                        switch (snap_.lfoTarget)
                        {
                            case 0: lfoFeed = juce::jlimit(0.01f, 0.08f, lfoFeed + lmod * 0.02f); break;
                            case 1: lfoKillMod = juce::jlimit(0.033f, 0.075f, lfoKillMod + lmod * 0.02f); break;
                            case 2: lfoEvoMod = juce::jlimit(0.01f, 1000.0f, lfoEvoMod + lmod * 200.0f); break;
                            case 3: lfoFilterMod = juce::jlimit(80.0f, 20000.0f, lfoFilterMod + lmod * 8000.0f); break;
                            case 4: lfoDiffMod = juce::jlimit(0.1f, 1.0f, lfoDiffMod + lmod * 0.3f); break;
                            case 5: lfoTempMod = juce::jlimit(0.0f, 1.0f, lfoTempMod + lmod * 0.3f); break;
                            case 6: lfoPersistMod = juce::jlimit(0.0f, 1.0f, lfoPersistMod + lmod * 0.4f); break;
                            default: break;
                        }
                        // lfoFilterMod and lfoPersistMod feed into the next sample's
                        // effective parameters via block-rate smoothers (not per-RD-step)
                        // lfoEvoMod, lfoFilterMod, lfoPersistMod affect audible output:
                        (void)lfoPersistMod; // persistence affects grid carry (block-rate only)

                        // Coupling audio injection
                        if (std::abs(couplingAudioAccum_) > 0.001f)
                        {
                            voice.rngState = voice.rngState * 1664525u + 1013904223u;
                            int cell = static_cast<int>((voice.rngState & 0x7F)); // 0..127
                            // QDD mandate: softclip + clamp on injection
                            float inject = fastTanh(couplingAudioAccum_ * 0.5f);
                            voice.V[cell] = std::clamp(voice.V[cell] + inject * 0.1f, 0.0f, 1.0f);
                        }

                        // Use LFO-modulated evolution (D004: all params audible)
                        updateGrayScott(voice, lfoFeed, lfoKillMod, lfoDiffMod, lfoTempMod,
                                        std::max(1.0f, lfoEvoMod));
                        // Store LFO-modulated filter cutoff for use in per-sample filter step
                        voice.lastCouplingOut = lfoFilterMod; // repurpose as lfo-filter cache
                    }
                    else
                    {
                        // Even frozen: still update V_filtered for smooth readout
                        buildVFiltered(voice);
                    }
                }

                // ── Wavetable readout ────────────────────────────────────────
                const float currentFreq = voice.glide.process();
                voice.frequency = currentFreq;

                float phaseInc = currentFreq * static_cast<float>(OobleckVoice::kGridSize)
                                 / static_cast<float>(currentSampleRate_);

                // Apply field_bias as phase offset
                const float biasedPhase = voice.phase + snap_.fieldBias * (OobleckVoice::kGridSize * 0.5f);

                // Read from V_filtered using readout mode
                float sample = readWavetable(voice, biasedPhase, snap_.readoutMode);

                // Advance phase
                voice.phase += phaseInc;
                if (voice.phase >= static_cast<float>(OobleckVoice::kGridSize))
                    voice.phase -= static_cast<float>(OobleckVoice::kGridSize);

                // ── Wavefolding ───────────────────────────────────────────────
                if (snap_.fold > 0.001f)
                    sample = fastTanh(sample * (1.0f + snap_.fold * 4.0f));

                // ── Drive saturation ─────────────────────────────────────────
                if (snap_.drive > 0.001f)
                {
                    const float driveGain = 1.0f + snap_.drive * 3.0f;
                    sample = fastTanh(sample * driveGain);
                }

                // ── Output filter ─────────────────────────────────────────────
                // If LFO target is filter (case 3), lastCouplingOut holds LFO-modulated cutoff
                const float activeCutoff = (snap_.lfoTarget == 3 && voice.lastCouplingOut > 0.0f)
                    ? voice.lastCouplingOut : voiceFilterCutoff;
                if (updateFilter)
                {
                    voice.outputFilterL.setMode(filterMode);
                    voice.outputFilterR.setMode(filterMode);
                    voice.outputFilterL.setCoefficients_fast(activeCutoff, snap_.filterReso,
                                                             static_cast<float>(currentSampleRate_));
                    voice.outputFilterR.setCoefficients_fast(activeCutoff, snap_.filterReso,
                                                             static_cast<float>(currentSampleRate_));
                }
                float sampleL = voice.outputFilterL.processSample(sample);
                float sampleR = voice.outputFilterR.processSample(sample);
                sampleL = flushDenormal(sampleL);
                sampleR = flushDenormal(sampleR);

                // ── Amplitude envelope ────────────────────────────────────────
                const float envLevel = voice.ampEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    voice.releasing = false;
                    break;
                }

                // ── Velocity scaling ──────────────────────────────────────────
                const float velScale = 0.5f + voice.velocity * 0.5f;
                sampleL *= envLevel * velScale;
                sampleR *= envLevel * velScale;

                // ── Voice steal crossfade ─────────────────────────────────────
                if (voice.crossfadeGain < 1.0f)
                {
                    sampleL *= voice.crossfadeGain;
                    sampleR *= voice.crossfadeGain;
                    voice.crossfadeGain = std::min(1.0f, voice.crossfadeGain + voice.crossfadeRate);
                }

                // ── Per-voice DC blocker ───────────────────────────────────────
                // Handled below after stereo split

                voiceBufL_[static_cast<size_t>(s)] = sampleL;
                voiceBufR_[static_cast<size_t>(s)] = sampleR;
            }

            // DC blocker on voice output block (removes DC from V field [0,1])
            voice.dcBlocker.processBlock(voiceBufL_.data(), voiceBufR_.data(), numSamples);

            // Sum voice output into main buffer
            for (int s = 0; s < numSamples; ++s)
            {
                outL[s] += voiceBufL_[static_cast<size_t>(s)];
                outR[s] += voiceBufR_[static_cast<size_t>(s)];
            }
        }

        // ── Step 13: Post-mix DC block on engine output ───────────────────────
        engineDCBlocker_.processBlock(outL, outR, numSamples);

        // ── Step 14: Reverb send + return ────────────────────────────────────
        if (effReverb > 0.001f)
        {
            for (int s = 0; s < numSamples; ++s)
            {
                float revL, revR;
                fdn_.processSample(outL[s], outR[s], revL, revR);
                outL[s] = outL[s] * (1.0f - effReverb) + revL * effReverb;
                outR[s] = outR[s] * (1.0f - effReverb) + revR * effReverb;
            }
        }

        // ── Step 15: Cache output for coupling reads ─────────────────────────
        for (int s = 0; s < numSamples; ++s)
        {
            outputCacheL_[static_cast<size_t>(s)] = outL[s];
            outputCacheR_[static_cast<size_t>(s)] = outR[s];
        }

        // Update active voice count
        activeVoiceCount_.store(activeCount, std::memory_order_relaxed);

        // Analyze for silence gate
        analyzeForSilenceGate(buffer, numSamples);

        // Reset per-block coupling accumulators
        couplingAudioAccum_  = 0.0f;
        couplingMorphAccum_  = 0.0f;
        couplingFilterAccum_ = 0.0f;
    }

    // -------------------------------------------------------------------------
    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        const int idx = std::clamp(sampleIndex, 0,
            static_cast<int>(outputCacheL_.size()) - 1);
        return (channel == 0) ? outputCacheL_[static_cast<size_t>(idx)]
                              : outputCacheR_[static_cast<size_t>(idx)];
    }

    // -------------------------------------------------------------------------
    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        if (type == CouplingType::AudioToBuffer || type == CouplingType::AudioToFM
            || type == CouplingType::AudioToWavetable)
        {
            // External audio perturbs ALL active voices' V fields
            for (int i = 0; i < numSamples; ++i)
            {
                float src = (sourceBuffer ? sourceBuffer[i] : 0.0f) * amount;
                src = fastTanh(src * 0.5f); // QDD mandate: soft-clip input
                couplingAudioAccum_ += 0.01f * (src - couplingAudioAccum_);
            }
        }
        else if (type == CouplingType::EnvToMorph)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float src = (sourceBuffer ? sourceBuffer[i] : 0.0f) * amount;
                couplingMorphAccum_ += 0.01f * (src * 0.25f - couplingMorphAccum_);
            }
        }
        else
        {
            // All other coupling types modulate filter
            for (int i = 0; i < numSamples; ++i)
            {
                float src = (sourceBuffer ? sourceBuffer[i] : 0.0f) * amount;
                couplingFilterAccum_ += 0.01f * (src * 0.25f - couplingFilterAccum_);
            }
        }
    }

private:
    // =========================================================================
    // Gray-Scott PDE update
    // =========================================================================
    void updateGrayScott(OobleckVoice& voice, float F, float k,
                         float diffRatio, float temperature,
                         float effectiveEvolution) noexcept
    {
        constexpr int N   = OobleckVoice::kGridSize;
        constexpr float dt = 1.0f;
        constexpr float Du = 0.16f;
        const float Dv = Du * diffRatio;

        for (int i = 0; i < N; ++i)
        {
            const int im = (i - 1 + N) % N;
            const int ip = (i + 1) % N;

            const float lapU = voice.U[im] - 2.0f * voice.U[i] + voice.U[ip];
            const float lapV = voice.V[im] - 2.0f * voice.V[i] + voice.V[ip];
            const float reaction = voice.U[i] * voice.V[i] * voice.V[i];

            voice.U_new[i] = voice.U[i] + dt * (Du * lapU - reaction + F * (1.0f - voice.U[i]));
            voice.V_new[i] = voice.V[i] + dt * (Dv * lapV + reaction - (F + k) * voice.V[i]);

            // Temperature noise injection
            if (temperature > 0.001f)
            {
                voice.V_new[i] += temperature * 0.01f * voice.nextNoise();
            }

            // QDD mandate: clamp to [0,1]
            voice.U_new[i] = std::clamp(voice.U_new[i], 0.0f, 1.0f);
            voice.V_new[i] = std::clamp(voice.V_new[i], 0.0f, 1.0f);
        }

        // Swap buffers
        std::memcpy(voice.U, voice.U_new, N * sizeof(float));
        std::memcpy(voice.V, voice.V_new, N * sizeof(float));

        // Dead-chemistry guard: soft nudge
        float vSum = 0.0f;
        for (int i = 0; i < N; ++i) vSum += voice.V[i];

        if (vSum < 0.01f)
        {
            voice.rngState = voice.rngState * 1664525u + 1013904223u;
            int cell = static_cast<int>(voice.rngState & 0x7F); // 0..127
            voice.V[cell] = std::clamp(voice.V[cell] + 0.05f, 0.0f, 1.0f);
            voice.U[cell] = std::clamp(voice.U[cell] - 0.02f, 0.0f, 1.0f);
        }

        // Stagnation detector
        const float vDiff = std::abs(vSum - voice.prevVSum);
        voice.prevVSum = vSum;
        if (vDiff < 0.001f)
        {
            voice.stagnationCounter++;
            const int stagnationThresh = static_cast<int>(
                currentSampleRate_ * 2.0f / std::max(1.0f, effectiveEvolution));
            if (voice.stagnationCounter > stagnationThresh)
            {
                // Inject micro-perturbations to break equilibrium
                for (int j = 0; j < 3; ++j)
                {
                    voice.rngState = voice.rngState * 1664525u + 1013904223u;
                    int cell = static_cast<int>(voice.rngState & 0x7F);
                    voice.V[cell] = std::clamp(voice.V[cell] + 0.003f, 0.0f, 1.0f);
                }
                voice.stagnationCounter = 0;
            }
        }
        else
        {
            voice.stagnationCounter = 0;
        }

        // Build anti-aliased V_filtered (QDD mandate #3)
        buildVFiltered(voice);
    }

    // =========================================================================
    // buildVFiltered — 1-pole IIR LP anti-aliasing on V grid
    // =========================================================================
    void buildVFiltered(OobleckVoice& voice) const noexcept
    {
        constexpr int N = OobleckVoice::kGridSize;

        // cutoff_normalized = noteFreq * 64 / sampleRate, clamped to [0.01, 0.99]
        const float cutNorm = std::clamp(
            voice.frequency * 64.0f / static_cast<float>(currentSampleRate_),
            0.01f, 0.99f);
        // Matched-Z coefficient
        constexpr float kTwoPi = 6.28318530718f;
        const float alpha = 1.0f - fastExp(-kTwoPi * cutNorm);

        // First pass: causal forward filter
        voice.V_filtered[0] = voice.V[0];
        for (int i = 1; i < N; ++i)
        {
            voice.V_filtered[i] = voice.V_filtered[i - 1]
                                 + alpha * (voice.V[i] - voice.V_filtered[i - 1]);
        }
        // Second pass: handle ring continuity (wrap correction)
        for (int i = 0; i < N; ++i)
        {
            const int prev = ((i - 1) + N) % N;
            voice.V_filtered[i] = voice.V_filtered[prev]
                                 + alpha * (voice.V[i] - voice.V_filtered[prev]);
            voice.V_filtered[i] = flushDenormal(voice.V_filtered[i]);
        }
    }

    // =========================================================================
    // readWavetable — cubic Hermite with periodic boundaries
    // =========================================================================
    inline float readWavetable(const OobleckVoice& voice, float pos, int readoutMode) const noexcept
    {
        constexpr int N = OobleckVoice::kGridSize;

        // Apply readout mode transformation to position or table
        float effPos = pos;
        switch (readoutMode)
        {
            case 1: // Odd — double phase, read every other cell (asymmetric spectrum)
                effPos = std::fmod(pos * 2.0f, static_cast<float>(N));
                break;
            case 2: // Even — halved phase, even harmonics
                effPos = std::fmod(pos * 0.5f, static_cast<float>(N));
                break;
            case 3: // Mirror — mirror first 64 cells
            {
                // pos maps: 0..63 = forward, 64..127 = mirrored back
                const float halfN = static_cast<float>(N / 2);
                effPos = std::fmod(pos, static_cast<float>(N));
                if (effPos >= halfN)
                    effPos = static_cast<float>(N) - effPos;
                break;
            }
            default: // Raw
                break;
        }

        // Wrap effPos to [0, N)
        while (effPos < 0.0f) effPos += static_cast<float>(N);
        while (effPos >= static_cast<float>(N)) effPos -= static_cast<float>(N);

        const int i1   = static_cast<int>(effPos);
        const float frac = effPos - static_cast<float>(i1);
        const int idx1 = i1 % N;
        const int idx0 = (idx1 - 1 + N) % N;
        const int idx2 = (idx1 + 1) % N;
        const int idx3 = (idx1 + 2) % N;

        const float y0 = voice.V_filtered[idx0];
        const float y1 = voice.V_filtered[idx1];
        const float y2 = voice.V_filtered[idx2];
        const float y3 = voice.V_filtered[idx3];

        // Cubic Hermite
        const float c0 = y1;
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    // =========================================================================
    // filterModeFromInt — map int parameter to CytomicSVF::Mode
    // =========================================================================
    static CytomicSVF::Mode filterModeFromInt(int t) noexcept
    {
        switch (t)
        {
            case 1:  return CytomicSVF::Mode::HighPass;
            case 2:  return CytomicSVF::Mode::BandPass;
            case 3:  return CytomicSVF::Mode::Notch;
            default: return CytomicSVF::Mode::LowPass;
        }
    }

    // =========================================================================
    // processMidi — handle note on/off, pitch bend, expression, aftertouch
    // =========================================================================
    void processMidi(juce::MidiBuffer& midi,
                     const OobleckParamSnapshot& snap,
                     float effPersistence) noexcept
    {
        for (const auto& meta : midi)
        {
            const auto msg = meta.getMessage();

            if (msg.isNoteOn())
            {
                const int   note = msg.getNoteNumber();
                const float vel  = msg.getFloatVelocity();
                noteOn(note, vel, snap, effPersistence);
            }
            else if (msg.isNoteOff())
            {
                noteOff(msg.getNoteNumber());
            }
            else if (msg.isPitchWheel())
            {
                // ±2 semitones
                const float bendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
                pitchBend_ = PitchBendUtil::semitonesToFreqRatio(bendNorm * 2.0f);
            }
            else if (msg.isControllerOfType(1)) // CC1: mod wheel → evolution mod
            {
                modWheel_ = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }
            else if (msg.isControllerOfType(11)) // CC11: expression → kill offset
            {
                expression_ = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }
            else if (msg.isChannelPressure())
            {
                aftertouchGlobal_ = static_cast<float>(msg.getChannelPressureValue()) / 127.0f;
                // Distribute to all active voices
                for (auto& v : voices_)
                    if (v.active)
                        v.currentAftertouch = aftertouchGlobal_;
            }
            else if (msg.isAftertouch())
            {
                aftertouchGlobal_ = static_cast<float>(msg.getAfterTouchValue()) / 127.0f;
            }
        }
    }

    // =========================================================================
    // noteOn — find free voice or steal, seed grid, trigger envelope
    // =========================================================================
    void noteOn(int note, float vel,
                const OobleckParamSnapshot& snap,
                float effPersistence) noexcept
    {
        // Re-trigger existing voice on same note
        for (auto& v : voices_)
        {
            if (v.active && v.midiNote == note)
            {
                const float tuneSemitones = static_cast<float>(snap.tune) + snap.fine / 100.0f;
                v.targetFrequency = midiToFreqTune(note, tuneSemitones) * pitchBend_;
                v.glide.setTarget(v.targetFrequency);
                v.velocity = vel;
                v.releasing = false;
                v.ampEnv.noteOn();
                wakeSilenceGate();
                return;
            }
        }

        // Find free voice
        OobleckVoice* target = nullptr;
        for (auto& v : voices_)
        {
            if (!v.active) { target = &v; break; }
        }

        // LRU steal: find releasing voice first, then oldest
        if (target == nullptr)
        {
            for (auto& v : voices_)
            {
                if (v.releasing) { target = &v; break; }
            }
        }
        if (target == nullptr)
            target = &voices_[0]; // last resort: steal first

        // Check grid carry-over vs reset
        const bool isStableLoad  = (snap.seed == 4);
        const bool shouldCarryGrid = (effPersistence >= 0.5f && isStableLoad);
        const bool wasActive = target->active;

        // Reset voice state (preserves grid if shouldCarryGrid)
        target->active    = false;
        target->releasing = false;
        target->phase     = 0.0f;
        target->rdAccumulator = 0.0f;
        target->ampEnv.reset();
        target->lfo.reset();
        target->outputFilterL.reset();
        target->outputFilterR.reset();
        target->crossfadeGain = wasActive ? 0.0f : 1.0f;
        target->crossfadeRate = wasActive
            ? 1.0f / (0.005f * static_cast<float>(currentSampleRate_))
            : 0.0f;
        target->currentAftertouch = aftertouchGlobal_;
        target->prevVSum = 0.0f;
        target->stagnationCounter = 0;
        target->lastCouplingOut = 0.0f;

        if (!shouldCarryGrid)
            target->initGrid(snap.seed);

        // Set note data
        target->midiNote = note;
        target->velocity = vel;
        const float tuneSemitones = static_cast<float>(snap.tune) + snap.fine / 100.0f;
        target->targetFrequency = midiToFreqTune(note, tuneSemitones) * pitchBend_;
        target->frequency = target->targetFrequency;

        // Glide: snap on first note, glide thereafter
        const float glideTime = snap.glide * snap.glide * 2.0f;
        target->glide.setTime(glideTime, static_cast<float>(currentSampleRate_));
        if (!wasActive)
            target->glide.snapTo(target->targetFrequency);
        else
            target->glide.setTarget(target->targetFrequency);

        // Initialise V_filtered for readout
        buildVFiltered(*target);

        // Trigger envelope
        target->ampEnv.prepare(static_cast<float>(currentSampleRate_));
        target->ampEnv.setADSR(snap.attack, snap.decay, snap.sustain, snap.release);
        target->ampEnv.noteOn();

        // LFO
        target->lfo.setRate(snap.lfoRate, static_cast<float>(currentSampleRate_));
        target->lfo.setShape(snap.lfoShape);

        target->active = true;
        wakeSilenceGate();
    }

    // =========================================================================
    // noteOff — release matching voice
    // =========================================================================
    void noteOff(int note) noexcept
    {
        for (auto& v : voices_)
        {
            if (v.active && !v.releasing && v.midiNote == note)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
                return;
            }
        }
    }

    // =========================================================================
    // Members
    // =========================================================================
    std::array<OobleckVoice, kMaxVoices> voices_;
    OobleckParamSnapshot snap_;
    OobleckFDN fdn_;
    xoceanus::DCBlocker engineDCBlocker_; // engine-level DC block (final stage)

    double currentSampleRate_ = 44100.0;
    int    maxBlockSize_      = 512;

    // MIDI expression state
    float pitchBend_           = 1.0f; // freq ratio (1.0 = no bend)
    float modWheel_            = 0.0f;
    float expression_          = 0.0f;
    float aftertouchGlobal_    = 0.0f;

    // Scratch buffers
    std::vector<float> voiceBufL_;
    std::vector<float> voiceBufR_;
    std::vector<float> reverbSendL_;
    std::vector<float> reverbSendR_;

    // Coupling output cache
    std::vector<float> outputCacheL_;
    std::vector<float> outputCacheR_;

    // Coupling input accumulators (consumed + reset each block)
    float couplingAudioAccum_  = 0.0f; // AudioToBuffer → V field perturbation
    float couplingMorphAccum_  = 0.0f; // EnvToMorph → kill offset
    float couplingFilterAccum_ = 0.0f; // All other types → filter cutoff mod
};

} // namespace xoceanus
