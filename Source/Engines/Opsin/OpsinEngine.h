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
#include <numeric>

namespace xoceanus
{

//==============================================================================
//
//  O P S I N   E N G I N E
//  Bioluminescent Neural Feedback Synthesis
//
//  XO_OX Aquatic Identity: Deep-sea anglerfish neural network — the lure
//  glows by self-oscillating photophores, synchronized through electrical
//  coupling. No oscillators. Sound emerges from mutual excitation of 6 nodes.
//  Gallery code: OPSIN | Accent: Bioluminescent Cyan #00FFCC | Prefix: ops_
//
//  Architecture: 6-node Pulse-Coupled Oscillator (PCO) network.
//    - Mirollo & Strogatz (1990): pulse-coupled oscillator synchronization
//    - Hebbian learning (Hebb, 1949): fire-together-wire-together plasticity
//    - David Tudor "Neural Synthesis" (1989–1995): feedback network synthesis
//
//  Signal Flow:
//    Note-on impulse → excitation node(s)
//      ↓
//    Photophore 0 ←→ Photophore 1 ←→ Photophore 2
//          ↕                 ↕                 ↕
//    Photophore 5 ←→ Photophore 4 ←→ Photophore 3
//      ↓ (synapse weight matrix, topology-dependent)
//    Energy Governor (per-node soft limit + global RMS ceiling)
//      ↓
//    Sum all node outputs → CytomicSVF filter → VCA (amp env) → Output
//
//  Coupling:
//    Output: post-filter stereo audio (ch0=L, ch1=R)
//            network firing rate 0–1 (ch2)
//    Input:  AudioToFM      → inject into photophore (round-robin)
//            AmpToFilter    → filter cutoff modulation
//            EnvToMorph     → feedback amount modulation
//            RhythmToBlend  → excitation burst trigger
//
//==============================================================================

// ---- Engine-level constants ----
static constexpr int   kOpsinNodes          = 6;
static constexpr float kOpsinTwoPi          = 6.28318530717958647692f;
static constexpr float kOpsinPi             = 3.14159265358979323846f;
static constexpr float kOpsinMinHz          = 8.0f;  // lowest freq for delay buffer sizing

//==============================================================================
//  PhotophoreNode — one neuron in the bioluminescent network
//==============================================================================
struct PhotophoreNode
{
    std::vector<float> delayBuffer;   // circular delay line (sized in prepare())
    int   delayLength  = 256;         // pitch-tracked length in samples
    int   writePos     = 0;           // current write position

    CytomicSVF resonantFilter;        // material-specific coloring

    float energy        = 0.0f;       // current signal energy
    bool  firing        = false;      // above threshold?
    float refractoryTimer = 0.0f;     // counts down (in samples) after firing

    // 2 ms cosine fade for refractory boundary (no hard clip)
    float fadeGain     = 1.0f;
    float fadeTarget   = 1.0f;
    float fadeRate     = 0.0f;        // per-sample fade increment (set in prepare())

    // State for visualiser
    int   state        = 0;           // 0=idle, 1=firing, 2=refractory

    // Hebbian: did this node fire in the current learning window?
    bool  firedThisWindow = false;

    void reset() noexcept
    {
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f);
        writePos      = 0;
        energy        = 0.0f;
        firing        = false;
        refractoryTimer = 0.0f;
        fadeGain      = 1.0f;
        fadeTarget    = 1.0f;
        state         = 0;
        firedThisWindow = false;
        resonantFilter.reset();
    }
};

//==============================================================================
//
//  OpsinEngine — Bioluminescent Neural Feedback
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention — .cpp is a one-line stub.
//
//  Parameter prefix: ops_
//  Gallery accent:   Bioluminescent Cyan #00FFCC
//
//==============================================================================
class OpsinEngine : public SynthEngine
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

        // ---- A: Node Character ----
        params.push_back(std::make_unique<APC>(PID{"ops_material",1}, "Opsin Material",
            juce::StringArray{"Membrane","Glass","Crystal","Coral","Void","Metal","Silk","Plasma"}, 2));

        params.push_back(std::make_unique<AP>(PID{"ops_spread",1}, "Opsin Spread",
            NR{0.0f, 1.0f, 0.001f}, 0.25f));

        params.push_back(std::make_unique<AP>(PID{"ops_threshold",1}, "Opsin Threshold",
            NR{0.0f, 1.0f, 0.001f}, 0.45f));

        {
            NR r{1.0f, 500.0f, 0.01f};
            r.setSkewForCentre(50.0f);
            params.push_back(std::make_unique<AP>(PID{"ops_refractory",1}, "Opsin Refractory",
                r, 80.0f));
        }

        params.push_back(std::make_unique<AP>(PID{"ops_pitchTrack",1}, "Opsin Pitch Track",
            NR{0.0f, 1.0f, 0.001f}, 0.75f));

        {
            NR r{10.0f, 800.0f, 0.01f};
            r.setSkewForCentre(80.0f);
            params.push_back(std::make_unique<AP>(PID{"ops_delayBase",1}, "Opsin Delay Base Hz",
                r, 110.0f));
        }

        // ---- B: Feedback Matrix ----
        params.push_back(std::make_unique<APC>(PID{"ops_topology",1}, "Opsin Topology",
            juce::StringArray{"Ring","Star","Mesh","Cascade","Random"}, 0));

        params.push_back(std::make_unique<AP>(PID{"ops_density",1}, "Opsin Density",
            NR{0.0f, 1.0f, 0.001f}, 0.35f));

        params.push_back(std::make_unique<AP>(PID{"ops_feedback",1}, "Opsin Feedback",
            NR{0.0f, 1.0f, 0.001f}, 0.40f));

        params.push_back(std::make_unique<AP>(PID{"ops_asymmetry",1}, "Opsin Asymmetry",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<AP>(PID{"ops_scatter",1}, "Opsin Scatter",
            NR{0.0f, 1.0f, 0.001f}, 0.15f));

        // ---- C: Hebbian Learning ----
        params.push_back(std::make_unique<AP>(PID{"ops_plasticity",1}, "Opsin Plasticity",
            NR{0.0f, 1.0f, 0.001f}, 0.20f));

        {
            NR r{0.1f, 30.0f, 0.01f};
            r.setSkewForCentre(5.0f);
            params.push_back(std::make_unique<AP>(PID{"ops_memory",1}, "Opsin Memory",
                r, 8.0f));
        }

        {
            NR r{1.0f, 200.0f, 0.01f};
            r.setSkewForCentre(30.0f);
            params.push_back(std::make_unique<AP>(PID{"ops_learnWindow",1}, "Opsin Learn Window",
                r, 30.0f));
        }

        // ---- D: Energy Governor ----
        params.push_back(std::make_unique<AP>(PID{"ops_govCeiling",1}, "Opsin Gov Ceiling",
            NR{0.0f, 1.0f, 0.001f}, 0.75f));

        params.push_back(std::make_unique<AP>(PID{"ops_govSpeed",1}, "Opsin Gov Speed",
            NR{0.0f, 1.0f, 0.001f}, 0.35f));

        params.push_back(std::make_unique<AP>(PID{"ops_excitation",1}, "Opsin Excitation",
            NR{0.0f, 1.0f, 0.001f}, 0.55f));

        params.push_back(std::make_unique<AP>(PID{"ops_excNode",1}, "Opsin Exc Node",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        // ---- E: Filter ----
        {
            NR r{20.0f, 20000.0f, 0.1f};
            r.setSkewForCentre(1000.0f);
            params.push_back(std::make_unique<AP>(PID{"ops_fltCutoff",1}, "Opsin Filter Cutoff",
                r, 3400.0f));
        }

        params.push_back(std::make_unique<AP>(PID{"ops_fltReso",1}, "Opsin Filter Reso",
            NR{0.0f, 1.0f, 0.001f}, 0.30f));

        params.push_back(std::make_unique<APC>(PID{"ops_fltType",1}, "Opsin Filter Type",
            juce::StringArray{"LP","HP","BP","Notch"}, 0));

        params.push_back(std::make_unique<AP>(PID{"ops_fltEnvAmt",1}, "Opsin Filter Env Amt",
            NR{-1.0f, 1.0f, 0.001f}, 0.40f));

        params.push_back(std::make_unique<AP>(PID{"ops_fltKeyTrack",1}, "Opsin Filter Key Track",
            NR{0.0f, 1.0f, 0.001f}, 0.50f));

        // ---- F: Amp Envelope ----
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"ops_ampAtk",1}, "Opsin Amp Attack",  r, 0.008f));
            params.push_back(std::make_unique<AP>(PID{"ops_ampDec",1}, "Opsin Amp Decay",   r, 0.3f));
            params.push_back(std::make_unique<AP>(PID{"ops_ampRel",1}, "Opsin Amp Release", r, 0.5f));
        }
        params.push_back(std::make_unique<AP>(PID{"ops_ampSus",1}, "Opsin Amp Sustain",
            NR{0.0f, 1.0f, 0.001f}, 0.70f));

        // ---- G: Filter Envelope ----
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"ops_fenvAtk",1}, "Opsin FEnv Attack",  r, 0.012f));
            params.push_back(std::make_unique<AP>(PID{"ops_fenvDec",1}, "Opsin FEnv Decay",   r, 0.4f));
            params.push_back(std::make_unique<AP>(PID{"ops_fenvRel",1}, "Opsin FEnv Release", r, 0.6f));
        }
        params.push_back(std::make_unique<AP>(PID{"ops_fenvSus",1}, "Opsin FEnv Sustain",
            NR{0.0f, 1.0f, 0.001f}, 0.30f));

        // ---- H: LFOs ----
        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(1.0f);
            params.push_back(std::make_unique<AP>(PID{"ops_lfo1Rate",1}, "Opsin LFO1 Rate",  r, 0.4f));
            params.push_back(std::make_unique<AP>(PID{"ops_lfo2Rate",1}, "Opsin LFO2 Rate",  r, 0.12f));
        }

        params.push_back(std::make_unique<AP>(PID{"ops_lfo1Depth",1}, "Opsin LFO1 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.30f));
        params.push_back(std::make_unique<AP>(PID{"ops_lfo2Depth",1}, "Opsin LFO2 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.20f));

        params.push_back(std::make_unique<APC>(PID{"ops_lfo1Shape",1}, "Opsin LFO1 Shape",
            juce::StringArray{"Sine","Triangle","Saw","Square","S&H"}, 0));
        params.push_back(std::make_unique<APC>(PID{"ops_lfo2Shape",1}, "Opsin LFO2 Shape",
            juce::StringArray{"Sine","Triangle","Saw","Square","S&H"}, 4));

        params.push_back(std::make_unique<APC>(PID{"ops_lfo1Target",1}, "Opsin LFO1 Target",
            juce::StringArray{"Feedback","Threshold","FilterCutoff","Excitation"}, 0));
        params.push_back(std::make_unique<APC>(PID{"ops_lfo2Target",1}, "Opsin LFO2 Target",
            juce::StringArray{"Feedback","Threshold","FilterCutoff","Excitation"}, 1));

        // ---- I: Mod Matrix (4 slots) ----
        static const juce::StringArray kOpsinModDests {
            "Off", "Filter Cutoff", "Feedback", "Threshold", "Excitation", "Density", "Amp Level"
        };
        ModMatrix<4>::addParameters(params, "ops_", "Opsin", kOpsinModDests);

        // ---- J: Macros + Voice ----
        params.push_back(std::make_unique<AP>(PID{"ops_macro1",1}, "Opsin Macro BIOLUME",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));
        params.push_back(std::make_unique<AP>(PID{"ops_macro2",1}, "Opsin Macro PLASTICITY",
            NR{0.0f, 1.0f, 0.001f}, 0.3f));
        params.push_back(std::make_unique<AP>(PID{"ops_macro3",1}, "Opsin Macro CHAIN REACTION",
            NR{0.0f, 1.0f, 0.001f}, 0.35f));
        params.push_back(std::make_unique<AP>(PID{"ops_macro4",1}, "Opsin Macro SPACE",
            NR{0.0f, 1.0f, 0.001f}, 0.45f));

        params.push_back(std::make_unique<APC>(PID{"ops_voiceMode",1}, "Opsin Voice Mode",
            juce::StringArray{"Mono","Legato","Poly4"}, 1));

        {
            NR r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.2f);
            params.push_back(std::make_unique<AP>(PID{"ops_glide",1}, "Opsin Glide",
                r, 0.0f));
        }
    }

    //--------------------------------------------------------------------------
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return juce::AudioProcessorValueTreeState::ParameterLayout(
            std::make_move_iterator(params.begin()),
            std::make_move_iterator(params.end()));
    }

    //==========================================================================
    //  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateFloat = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 44100.0f;

        // Delay buffer: C-2 = 8.18 Hz, but we floor at kOpsinMinHz = 8 Hz
        const int maxDelaySamples = static_cast<int>(std::ceil(sampleRateFloat / kOpsinMinHz)) + 4;

        // 2 ms cosine fade rate (samples)
        const float fadeSamples = 0.002f * sampleRateFloat;
        const float fadeRateVal = (fadeSamples > 0.0f) ? (1.0f / fadeSamples) : 1.0f;

        for (int i = 0; i < kOpsinNodes; ++i)
        {
            nodes[i].delayBuffer.assign(static_cast<size_t>(maxDelaySamples), 0.0f);
            nodes[i].delayLength = maxDelaySamples / 4; // placeholder; updated on note-on
            nodes[i].writePos    = 0;
            nodes[i].fadeRate    = fadeRateVal;
            nodes[i].reset();
        }

        // Smooth coefficient: ~5 ms
        smoothCoeff = 1.0f - std::exp(-kOpsinTwoPi * (1.0f / 0.005f) / sampleRateFloat);

        // Silence gate: 500 ms hold (feedback network has tails)
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);

        // Output cache
        outCacheL.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outCacheR.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        // Envelopes
        ampEnv.prepare(sampleRateFloat);
        filterEnv.prepare(sampleRateFloat);

        // LFOs
        lfo1.reset();
        lfo2.reset();

        // Governor RMS follower (~20ms default attack)
        govFollower = 0.0f;
        govFollowerCoeff = 1.0f - std::exp(-1.0f / (0.02f * sampleRateFloat));

        // Clear synapse matrices
        std::memset(synapseWeights, 0, sizeof(synapseWeights));
        std::memset(baseWeights,    0, sizeof(baseWeights));

        // Coupling round-robin index
        couplingNodeRR = 0;

        // Initialize smoothed params to defaults
        smoothFeedback  = 0.40f;
        smoothThreshold = 0.45f;
        smoothCutoff    = 3400.0f;
        smoothExcitation= 0.55f;
        smoothDensity   = 0.35f;
        smoothAmpLevel  = 1.0f;

        currentNote     = -1;
        currentFreq     = 110.0f;
        targetFreq      = 110.0f;
        glideFreq       = 110.0f;
        velocity        = 0.0f;
        modWheelValue   = 0.0f;
        aftertouchValue = 0.0f;

        hebbianTimer    = 0.0f;

        (void)maxBlockSize; // used for cache sizing above
    }

    void releaseResources() override {}

    void reset() override
    {
        for (int i = 0; i < kOpsinNodes; ++i)
            nodes[i].reset();

        ampEnv.kill();
        filterEnv.kill();
        outputFilter.reset();

        lfo1.reset();
        lfo2.reset();

        govFollower     = 0.0f;
        couplingFilterMod   = 0.0f;
        couplingFeedbackMod = 0.0f;
        couplingExciteMod   = 0.0f;
        couplingNodeRR  = 0;
        hebbianTimer    = 0.0f;

        std::fill(outCacheL.begin(), outCacheL.end(), 0.0f);
        std::fill(outCacheR.begin(), outCacheR.end(), 0.0f);
    }

    //==========================================================================
    //  A T T A C H   P A R A M E T E R S
    //==========================================================================

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        auto get = [&](const juce::String& id) -> std::atomic<float>*
        {
            auto* p = apvts.getRawParameterValue(id);
            jassert(p != nullptr);
            return p;
        };

        // A
        pMaterial    = get("ops_material");
        pSpread      = get("ops_spread");
        pThreshold   = get("ops_threshold");
        pRefractory  = get("ops_refractory");
        pPitchTrack  = get("ops_pitchTrack");
        pDelayBase   = get("ops_delayBase");
        // B
        pTopology    = get("ops_topology");
        pDensity     = get("ops_density");
        pFeedback    = get("ops_feedback");
        pAsymmetry   = get("ops_asymmetry");
        pScatter     = get("ops_scatter");
        // C
        pPlasticity  = get("ops_plasticity");
        pMemory      = get("ops_memory");
        pLearnWindow = get("ops_learnWindow");
        // D
        pGovCeiling  = get("ops_govCeiling");
        pGovSpeed    = get("ops_govSpeed");
        pExcitation  = get("ops_excitation");
        pExcNode     = get("ops_excNode");
        // E
        pFltCutoff   = get("ops_fltCutoff");
        pFltReso     = get("ops_fltReso");
        pFltType     = get("ops_fltType");
        pFltEnvAmt   = get("ops_fltEnvAmt");
        pFltKeyTrack = get("ops_fltKeyTrack");
        // F
        pAmpAtk      = get("ops_ampAtk");
        pAmpDec      = get("ops_ampDec");
        pAmpSus      = get("ops_ampSus");
        pAmpRel      = get("ops_ampRel");
        // G
        pFenvAtk     = get("ops_fenvAtk");
        pFenvDec     = get("ops_fenvDec");
        pFenvSus     = get("ops_fenvSus");
        pFenvRel     = get("ops_fenvRel");
        // H
        pLfo1Rate    = get("ops_lfo1Rate");
        pLfo1Depth   = get("ops_lfo1Depth");
        pLfo1Shape   = get("ops_lfo1Shape");
        pLfo1Target  = get("ops_lfo1Target");
        pLfo2Rate    = get("ops_lfo2Rate");
        pLfo2Depth   = get("ops_lfo2Depth");
        pLfo2Shape   = get("ops_lfo2Shape");
        pLfo2Target  = get("ops_lfo2Target");
        // J
        pMacro1      = get("ops_macro1");
        pMacro2      = get("ops_macro2");
        pMacro3      = get("ops_macro3");
        pMacro4      = get("ops_macro4");
        pVoiceMode   = get("ops_voiceMode");
        pGlide       = get("ops_glide");

        // Mod matrix
        modMatrix.attachParameters(apvts, "ops_");
    }

    //==========================================================================
    //  R E N D E R   B L O C K
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0) return;
        if (sampleRateFloat <= 0.0f) { sampleRateFloat = 44100.0f; }

        // ---- ParamSnapshot ----
        const float rawMaterial    = loadParam(pMaterial,    2.0f);
        const float rawSpread      = loadParam(pSpread,      0.25f);
        const float rawThreshold   = loadParam(pThreshold,   0.45f);
        const float rawRefractory  = loadParam(pRefractory,  80.0f);
        const float rawPitchTrack  = loadParam(pPitchTrack,  0.75f);
        const float rawDelayBase   = loadParam(pDelayBase,   110.0f);

        const int   paramTopology  = static_cast<int>(loadParam(pTopology,  0.0f));
        const float rawDensity     = loadParam(pDensity,     0.35f);
        const float rawFeedback    = loadParam(pFeedback,    0.40f);
        const float rawAsymmetry   = loadParam(pAsymmetry,   0.0f);
        const float rawScatter     = loadParam(pScatter,     0.15f);

        const float rawPlasticity  = loadParam(pPlasticity,  0.20f);
        const float rawMemory      = loadParam(pMemory,      8.0f);
        const float rawLearnWindow = loadParam(pLearnWindow, 30.0f);

        const float rawGovCeiling  = loadParam(pGovCeiling,  0.75f);
        const float rawGovSpeed    = loadParam(pGovSpeed,    0.35f);
        const float rawExcitation  = loadParam(pExcitation,  0.55f);
        const float rawExcNode     = loadParam(pExcNode,     0.0f);

        const float rawCutoff      = loadParam(pFltCutoff,   3400.0f);
        const float rawReso        = loadParam(pFltReso,     0.30f);
        const int   paramFltType   = static_cast<int>(loadParam(pFltType,   0.0f));
        const float rawFltEnvAmt   = loadParam(pFltEnvAmt,   0.40f);
        const float rawKeyTrack    = loadParam(pFltKeyTrack,  0.50f);

        const float ampAtk = loadParam(pAmpAtk, 0.008f);
        const float ampDec = loadParam(pAmpDec, 0.3f);
        const float ampSus = loadParam(pAmpSus, 0.70f);
        const float ampRel = loadParam(pAmpRel, 0.5f);

        const float fenvAtk = loadParam(pFenvAtk, 0.012f);
        const float fenvDec = loadParam(pFenvDec, 0.4f);
        const float fenvSus = loadParam(pFenvSus, 0.30f);
        const float fenvRel = loadParam(pFenvRel, 0.6f);

        const float lfo1RateHz    = std::max(0.01f, loadParam(pLfo1Rate,  0.4f));
        const float lfo1Depth     = loadParam(pLfo1Depth,  0.30f);
        const int   lfo1Shape     = static_cast<int>(loadParam(pLfo1Shape, 0.0f));
        const int   lfo1Target    = static_cast<int>(loadParam(pLfo1Target, 0.0f));
        const float lfo2RateHz    = std::max(0.01f, loadParam(pLfo2Rate,  0.12f));
        const float lfo2Depth     = loadParam(pLfo2Depth,  0.20f);
        const int   lfo2Shape     = static_cast<int>(loadParam(pLfo2Shape, 4.0f));
        const int   lfo2Target    = static_cast<int>(loadParam(pLfo2Target, 1.0f));

        const float macro1 = loadParam(pMacro1, 0.5f);
        const float macro2 = loadParam(pMacro2, 0.3f);
        const float macro3 = loadParam(pMacro3, 0.35f);
        const float macro4 = loadParam(pMacro4, 0.45f);

        const int   paramVoiceMode = static_cast<int>(loadParam(pVoiceMode, 1.0f));
        const float rawGlide       = loadParam(pGlide, 0.0f);

        // ---- Macro expansion ----
        // M1=BIOLUME: excitation + feedback
        const float macroFeedback   = rawFeedback   + macro1 * 0.35f;
        const float macroExcitation = rawExcitation + macro1 * 0.30f;
        // M2=PLASTICITY: learning rate + memory inverse
        const float macroPlasticity = rawPlasticity + macro2 * 0.4f;
        const float macroMemory     = rawMemory     * (1.0f - macro2 * 0.6f + 0.1f);
        // M3=CHAIN REACTION: density + scatter
        const float macroDensity    = rawDensity    + macro3 * 0.4f;
        const float macroScatter    = rawScatter    + macro3 * 0.3f;
        // M4=SPACE: filter cutoff + spread
        const float macroCutoff     = rawCutoff     * (1.0f + macro4 * 1.0f);
        const float macroSpread     = rawSpread     + macro4 * 0.3f;

        // Clamp macro-combined values
        const float paramFeedback   = clamp(macroFeedback,   0.0f, 0.98f);
        const float paramExcitation = clamp(macroExcitation, 0.0f, 1.0f);
        const float paramPlasticity = clamp(macroPlasticity, 0.0f, 1.0f);
        const float paramMemory     = clamp(macroMemory,     0.1f, 30.0f);
        const float paramDensity    = clamp(macroDensity,    0.0f, 1.0f);
        const float paramScatter    = clamp(macroScatter,    0.0f, 1.0f);
        const float paramCutoff     = clamp(macroCutoff,     20.0f, 20000.0f);
        const float paramSpread     = clamp(macroSpread,     0.0f, 1.0f);
        const float paramThreshold  = clamp(rawThreshold,    0.0f, 1.0f);

        // ---- Glide coefficient ----
        float glideCoeff = 1.0f;
        if (rawGlide > 0.001f)
            glideCoeff = 1.0f - std::exp(-1.0f / (rawGlide * sampleRateFloat));

        // ---- Parse MIDI ----
        bool noteOnThisBlock = false;
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn() && msg.getVelocity() > 0)
            {
                const int  newNote  = msg.getNoteNumber();
                const float vel     = static_cast<float>(msg.getVelocity()) / 127.0f;
                velocity = vel;

                const float newFreq = midiToFreq(newNote);

                if (currentNote < 0)
                {
                    // First note: snap glide to target
                    glideFreq = newFreq;
                    targetFreq = newFreq;
                }
                else
                {
                    targetFreq = newFreq;
                }

                currentNote = newNote;
                currentFreq = newFreq;

                // Retrigger envelopes
                ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
                filterEnv.setADSR(fenvAtk, fenvDec, fenvSus, fenvRel);

                bool isLegato = (paramVoiceMode == 1);
                if (isLegato && ampEnv.isActive())
                    ampEnv.retriggerFrom(ampEnv.getLevel(), ampAtk, ampDec, ampSus, ampRel);
                else
                    ampEnv.noteOn();
                filterEnv.noteOn();

                // Inject excitation into node(s)
                noteOnThisBlock = true;
                pendingExcitation = paramExcitation * vel;
                pendingExcNode    = rawExcNode;

                wakeSilenceGate();
                activeVoiceCount_.store(1, std::memory_order_relaxed);
            }
            else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0))
            {
                if (msg.getNoteNumber() == currentNote)
                {
                    ampEnv.noteOff();
                    filterEnv.noteOff();
                    currentNote = -1;
                }
            }
            else if (msg.isController())
            {
                if (msg.getControllerNumber() == 1) // Mod wheel CC1 (D006)
                    modWheelValue = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }
            else if (msg.isChannelPressure()) // Aftertouch (D006)
            {
                aftertouchValue = static_cast<float>(msg.getChannelPressureValue()) / 127.0f;
            }
        }

        // Check silence gate
        if (isSilenceGateBypassed())
        {
            buffer.clear();
            return;
        }

        // ---- Update envelopes ----
        ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
        filterEnv.setADSR(fenvAtk, fenvDec, fenvSus, fenvRel);

        // ---- Update feedback matrix topology ----
        buildTopology(paramTopology, paramDensity, paramFeedback,
                      rawAsymmetry, paramScatter);

        // ---- Update LFOs (block-rate) ----
        lfo1.setRate(lfo1RateHz, sampleRateFloat);
        lfo1.setShape(lfo1Shape);
        lfo2.setRate(lfo2RateHz, sampleRateFloat);
        lfo2.setShape(lfo2Shape);

        const float lfo1Val = lfo1.process() * lfo1Depth;
        const float lfo2Val = lfo2.process() * lfo2Depth;

        // ---- Mod matrix sources ----
        static const juce::StringArray kOpsinModDests {
            "Off", "Filter Cutoff", "Feedback", "Threshold", "Excitation", "Density", "Amp Level"
        };
        constexpr int kNumDests = 7;
        float destOffsets[kNumDests] = {};

        ModMatrix<4>::Sources modSrc;
        modSrc.lfo1      = lfo1Val;
        modSrc.lfo2      = lfo2Val;
        modSrc.env       = filterEnv.getLevel();
        modSrc.velocity  = velocity;
        modSrc.keyTrack  = (currentNote >= 0) ? ((static_cast<float>(currentNote) - 60.0f) / 60.0f) : 0.0f;
        modSrc.modWheel  = modWheelValue;
        modSrc.aftertouch= aftertouchValue;

        modMatrix.apply(modSrc, destOffsets);

        // ---- Apply LFO targets to smooth targets ----
        // LFO1/LFO2 pre-apply to their designated targets
        auto applyLfoToTarget = [&](int target, float lfoSample)
        {
            switch (target)
            {
                case 0: smoothFeedback   += lfoSample * 0.15f; break;
                case 1: smoothThreshold  += lfoSample * 0.15f; break;
                case 2: smoothCutoff     += lfoSample * 4000.0f; break;
                case 3: smoothExcitation += lfoSample * 0.20f; break;
                default: break;
            }
        };
        applyLfoToTarget(lfo1Target, lfo1Val);
        applyLfoToTarget(lfo2Target, lfo2Val);

        // ---- Smooth param targets ----
        // (one-pole smoothers tick toward raw param + mod matrix offsets)
        const float targetFeedback   = clamp(paramFeedback   + destOffsets[2] * 0.5f
                                              + couplingFeedbackMod,            0.0f, 0.98f);
        const float targetThreshold  = clamp(paramThreshold  + destOffsets[3] * 0.3f, 0.0f, 1.0f);
        const float targetExcitation = clamp(paramExcitation + destOffsets[4] * 0.5f
                                              + couplingExciteMod,              0.0f, 1.0f);
        const float targetDensity    = clamp(paramDensity    + destOffsets[5] * 0.5f, 0.0f, 1.0f);
        const float targetAmpLevel   = clamp(1.0f            + destOffsets[6] * 0.5f, 0.0f, 2.0f);

        float targetCutoff = clamp(paramCutoff
                                    + destOffsets[1] * 8000.0f
                                    + couplingFilterMod * 5000.0f,
                                   20.0f, 20000.0f);

        // Key tracking: (note - 60) semitones relative to C4 → frequency ratio
        if (currentNote >= 0)
            targetCutoff += (static_cast<float>(currentNote) - 60.0f) * rawKeyTrack * 50.0f;
        targetCutoff = clamp(targetCutoff, 20.0f, 20000.0f);

        // One-pole smooth
        smoothFeedback   += (targetFeedback   - smoothFeedback)   * smoothCoeff;
        smoothThreshold  += (targetThreshold  - smoothThreshold)  * smoothCoeff;
        smoothCutoff     += (targetCutoff     - smoothCutoff)     * smoothCoeff;
        smoothExcitation += (targetExcitation - smoothExcitation) * smoothCoeff;
        smoothDensity    += (targetDensity    - smoothDensity)    * smoothCoeff;
        smoothAmpLevel   += (targetAmpLevel   - smoothAmpLevel)   * smoothCoeff;

        // Clamp smoothed values to legal ranges
        smoothFeedback   = clamp(smoothFeedback,   0.0f, 0.98f);
        smoothThreshold  = clamp(smoothThreshold,  0.01f, 1.0f);
        smoothCutoff     = clamp(smoothCutoff,      20.0f, 20000.0f);
        smoothExcitation = clamp(smoothExcitation,  0.0f, 1.0f);
        smoothDensity    = clamp(smoothDensity,     0.0f, 1.0f);
        smoothAmpLevel   = clamp(smoothAmpLevel,    0.0f, 2.0f);

        // ---- Update governor speed ----
        const float govSpeedMapped = 0.001f + rawGovSpeed * 0.099f; // 1ms–100ms
        govFollowerCoeff = 1.0f - std::exp(-1.0f / (govSpeedMapped * sampleRateFloat));
        const float govCeiling = clamp(rawGovCeiling, 0.05f, 1.0f);

        // ---- Update output filter type/mode ----
        updateFilterMode(paramFltType);

        // ---- Compute delay lengths for each node (pitch tracking) ----
        // Glide toward target frequency
        glideFreq += (targetFreq - glideFreq) * glideCoeff;
        const float baseFreq = glideFreq;

        // Base delay length from frequency
        const float baseDelayHz = lerp(rawDelayBase, baseFreq, rawPitchTrack);
        const int   maxDlyLen   = static_cast<int>(nodes[0].delayBuffer.size()) - 1;
        const float material    = clamp(rawMaterial, 0.0f, 7.0f);

        // ---- Apply pending excitation on note-on ----
        if (noteOnThisBlock && pendingExcitation > 0.0f)
        {
            injectExcitation(pendingExcitation, pendingExcNode);
            pendingExcitation = 0.0f;
        }

        // ---- Render loop ----
        const int numCh = buffer.getNumChannels();
        float* chL = (numCh > 0) ? buffer.getWritePointer(0) : nullptr;
        float* chR = (numCh > 1) ? buffer.getWritePointer(1) : chL;
        if (!chL) { buffer.clear(); return; }

        // Velocity modulates filter env amount (D001)
        const float velFenvScale = 0.5f + velocity * 0.5f;

        // Refractory time in samples
        const float refractSamples = (rawRefractory / 1000.0f) * sampleRateFloat;

        // Hebbian learning window in samples
        const float learnWindowSamples = (rawLearnWindow / 1000.0f) * sampleRateFloat;
        // Memory: weight decay rate per block
        const float decayRatePerBlock  = 1.0f - (static_cast<float>(numSamples)
                                                  / (paramMemory * sampleRateFloat));

        // Update per-node delay lengths and filter materials
        for (int n = 0; n < kOpsinNodes; ++n)
        {
            // Spread: each node gets a slightly different base frequency
            const float spreadOffset = (kOpsinNodes > 1)
                ? (static_cast<float>(n) / static_cast<float>(kOpsinNodes - 1) - 0.5f)
                : 0.0f;
            const float nodeFreq  = baseDelayHz * (1.0f + spreadOffset * paramSpread * 0.5f);
            const float nodeFreqC = clamp(nodeFreq, kOpsinMinHz, sampleRateFloat * 0.45f);
            nodes[n].delayLength  = clamp(
                static_cast<int>(sampleRateFloat / nodeFreqC),
                1, maxDlyLen);

            // Material-specific resonant filter on each node
            updateNodeFilter(n, material, paramSpread, spreadOffset);
        }

        // Block-rate coefficient updates (every 16 samples via sampleIdx flag)
        int sampleIdx = 0;

        for (int s = 0; s < numSamples; ++s, ++sampleIdx)
        {
            // Update filter coefficients every 16 samples
            if ((sampleIdx & 15) == 0)
            {
                // Compute filter envelope contribution
                const float fenvLevel = filterEnv.getLevel();
                const float fenvAmt   = rawFltEnvAmt * velFenvScale;
                const float fenvOffset= fenvLevel * fenvAmt * 8000.0f;
                const float effectiveCutoff = clamp(smoothCutoff + fenvOffset, 20.0f, 20000.0f);

                outputFilter.setCoefficients_fast(effectiveCutoff, rawReso, sampleRateFloat);
            }

            // ---- Per-node processing ----
            float nodeOutputs[kOpsinNodes] = {};

            for (int n = 0; n < kOpsinNodes; ++n)
            {
                PhotophoreNode& node = nodes[n];

                // 1. Sum all incoming signals (other nodes via synapse matrix + coupling)
                float sumInput = 0.0f;
                for (int src = 0; src < kOpsinNodes; ++src)
                {
                    if (src == n) continue;
                    const float w = synapseWeights[n][src] * smoothFeedback;
                    const float readOut = readDelay(nodes[src], 0); // current output of src node
                    sumInput += w * readOut;
                }
                sumInput = flushDenormal(sumInput);

                // 2. Write into delay line, read delayed output
                writeDelay(node, sumInput);
                float delayed = readDelay(node, node.delayLength);
                delayed = flushDenormal(delayed);

                // 3. Resonant filter (material coloring)
                float filtered = node.resonantFilter.processSample(delayed);
                filtered = flushDenormal(filtered);

                // 4. Soft saturate
                float saturated = fastTanh(filtered * 2.0f) * 0.5f;
                saturated = flushDenormal(saturated);

                // 5. Energy tracking
                node.energy += govFollowerCoeff * (std::fabs(saturated) - node.energy);
                node.energy  = flushDenormal(node.energy);

                // 6. Firing / refractory logic
                if (node.refractoryTimer > 0.0f)
                {
                    // In refractory period
                    node.refractoryTimer -= 1.0f;
                    node.firedThisWindow  = false;
                    node.state            = 2;

                    // Cosine fade during refractory
                    node.fadeTarget = 0.0f;
                }
                else if (node.energy > smoothThreshold && !node.firing)
                {
                    // Threshold crossed — fire!
                    node.firing          = true;
                    node.refractoryTimer = refractSamples;
                    node.firedThisWindow = true;
                    node.state           = 1;
                    node.fadeTarget      = 0.0f; // begin fade OUT

                    // After refractory expires, fade back in (handled when timer hits 0)
                }
                else if (node.firing && node.refractoryTimer <= 0.0f)
                {
                    // Refractory just expired — fade back in
                    node.firing     = false;
                    node.fadeTarget = 1.0f;
                    node.state      = 0;
                }
                else
                {
                    node.state = 0;
                }

                // Smooth fade gain
                if (node.fadeGain < node.fadeTarget)
                    node.fadeGain = std::min(node.fadeTarget, node.fadeGain + node.fadeRate);
                else if (node.fadeGain > node.fadeTarget)
                    node.fadeGain = std::max(node.fadeTarget, node.fadeGain - node.fadeRate);

                nodeOutputs[n] = saturated * node.fadeGain;
            }

            // ---- Per-node: soft limiter (Energy Governor, per-node) ----
            for (int n = 0; n < kOpsinNodes; ++n)
            {
                nodeOutputs[n] = fastTanh(nodeOutputs[n] * govCeiling) / govCeiling;
                nodeOutputs[n] = flushDenormal(nodeOutputs[n]);
            }

            // ---- Global RMS governor ----
            float sumSq = 0.0f;
            for (int n = 0; n < kOpsinNodes; ++n)
                sumSq += nodeOutputs[n] * nodeOutputs[n];
            const float globalRMS = std::sqrt(sumSq / static_cast<float>(kOpsinNodes));
            govFollower += govFollowerCoeff * (globalRMS - govFollower);
            govFollower  = flushDenormal(govFollower);

            float govScale = 1.0f;
            if (govFollower > govCeiling && govFollower > 1e-6f)
                govScale = govCeiling / govFollower;

            for (int n = 0; n < kOpsinNodes; ++n)
                nodeOutputs[n] *= govScale;

            // ---- Sum all node outputs ----
            float mixedOut = 0.0f;
            for (int n = 0; n < kOpsinNodes; ++n)
                mixedOut += nodeOutputs[n];
            mixedOut *= (1.0f / static_cast<float>(kOpsinNodes)); // normalize
            mixedOut  = flushDenormal(mixedOut);

            // ---- Output filter ----
            float filtered = outputFilter.processSample(mixedOut);
            filtered = flushDenormal(filtered);

            // ---- Amp envelope (shapes network output level) ----
            const float ampLevel = ampEnv.process() * smoothAmpLevel;
            filterEnv.process(); // tick filter env

            // ---- Final output sample ----
            float outSample = filtered * ampLevel;

            // ---- Stereo (slight spread using node index modulation) ----
            // Pairs: L uses nodes 0,2,4 / R uses nodes 1,3,5
            float outL = outSample;
            float outR = outSample;

            // cache
            if (s < static_cast<int>(outCacheL.size()))
            {
                outCacheL[static_cast<size_t>(s)] = outL;
                outCacheR[static_cast<size_t>(s)] = outR;
            }

            chL[s] = outL;
            if (chR != chL)
                chR[s] = outR;
        }

        // ---- Post-block: Hebbian learning ----
        hebbianTimer += static_cast<float>(numSamples);
        if (hebbianTimer >= learnWindowSamples)
        {
            updateHebbianWeights(paramPlasticity,
                                 static_cast<float>(numSamples) / sampleRateFloat);
            hebbianTimer = 0.0f;
        }

        // Apply weight decay toward base topology
        const float dr = clamp(decayRatePerBlock, 0.0f, 1.0f);
        for (int i = 0; i < kOpsinNodes; ++i)
            for (int j = 0; j < kOpsinNodes; ++j)
                synapseWeights[i][j] = lerp(synapseWeights[i][j], baseWeights[i][j], 1.0f - dr);

        // Enforce spectral radius safety
        enforceSpectralRadius(0.95f);

        // Decay coupling accumulators (prevent sticky values — lesson 10)
        couplingFilterMod   *= 0.999f;
        couplingFeedbackMod *= 0.999f;
        couplingExciteMod   *= 0.999f;
        couplingFilterMod    = flushDenormal(couplingFilterMod);
        couplingFeedbackMod  = flushDenormal(couplingFeedbackMod);
        couplingExciteMod    = flushDenormal(couplingExciteMod);

        // Update active voice count
        if (!ampEnv.isActive())
            activeVoiceCount_.store(0, std::memory_order_relaxed);

        // Feed silence gate
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    //  C O U P L I N G
    //==========================================================================

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        const int idx = static_cast<int>(sampleIndex);
        if (channel == 0)
        {
            if (idx >= 0 && idx < static_cast<int>(outCacheL.size()))
                return outCacheL[static_cast<size_t>(idx)];
            return 0.0f;
        }
        if (channel == 1)
        {
            if (idx >= 0 && idx < static_cast<int>(outCacheR.size()))
                return outCacheR[static_cast<size_t>(idx)];
            return 0.0f;
        }
        if (channel == 2)
        {
            // Firing rate: count currently-firing nodes
            int firingCount = 0;
            for (int n = 0; n < kOpsinNodes; ++n)
                if (nodes[n].state == 1) ++firingCount;
            return static_cast<float>(firingCount) / static_cast<float>(kOpsinNodes);
        }
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        if (!sourceBuffer || numSamples <= 0) return;

        switch (type)
        {
            case CouplingType::AudioToFM:
            {
                // Inject coupling audio into a specific photophore node (round-robin)
                const int nodeIdx = couplingNodeRR % kOpsinNodes;

                // Write a burst of energy into the target node's delay line
                for (int i = 0; i < std::min(numSamples, static_cast<int>(nodes[nodeIdx].delayBuffer.size())); ++i)
                    writeDelay(nodes[nodeIdx], sourceBuffer[i] * amount * 0.5f);

                couplingNodeRR = (couplingNodeRR + 1) % kOpsinNodes;
                break;
            }

            case CouplingType::AmpToFilter:
            {
                // Compute average amplitude of source, modulate filter cutoff
                float avg = 0.0f;
                for (int i = 0; i < numSamples; ++i)
                    avg += std::fabs(sourceBuffer[i]);
                avg /= static_cast<float>(numSamples);
                couplingFilterMod += avg * amount;
                break;
            }

            case CouplingType::EnvToMorph:
            {
                // Coupling envelope modulates feedback amount
                float avg = 0.0f;
                for (int i = 0; i < numSamples; ++i)
                    avg += sourceBuffer[i];
                avg /= static_cast<float>(numSamples);
                couplingFeedbackMod += avg * amount;
                break;
            }

            case CouplingType::RhythmToBlend:
            {
                // Coupling rhythm triggers excitation bursts
                // Detect transients (peak > 0.5) in source
                for (int i = 0; i < numSamples; ++i)
                {
                    if (std::fabs(sourceBuffer[i]) > 0.5f)
                    {
                        couplingExciteMod += amount * 0.3f;
                        break; // One burst per block
                    }
                }
                break;
            }

            default:
                break;
        }
    }

    //==========================================================================
    //  I D E N T I T Y
    //==========================================================================

    juce::String getEngineId()    const override { return "Opsin"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF, 0x00, 0xCC); } // Bioluminescent Cyan ~#00FFCC mapped
    int          getMaxVoices()   const override { return 1; } // Monophonic: the network IS the voice

private:

    //==========================================================================
    //  H E L P E R S
    //==========================================================================

    static float loadParam(const std::atomic<float>* p, float fallback) noexcept
    {
        return p ? p->load(std::memory_order_relaxed) : fallback;
    }

    //--------------------------------------------------------------------------
    // Delay line helpers — inline, no allocation
    //--------------------------------------------------------------------------
    static void writeDelay(PhotophoreNode& node, float sample) noexcept
    {
        const int size = static_cast<int>(node.delayBuffer.size());
        if (size == 0) return;
        node.delayBuffer[static_cast<size_t>(node.writePos)] = sample;
        node.writePos = (node.writePos + 1) % size;
    }

    static float readDelay(const PhotophoreNode& node, int delaySamples) noexcept
    {
        const int size = static_cast<int>(node.delayBuffer.size());
        if (size == 0) return 0.0f;
        delaySamples = std::max(0, std::min(delaySamples, size - 1));
        int readPos = (node.writePos - 1 - delaySamples + size * 2) % size;
        return node.delayBuffer[static_cast<size_t>(readPos)];
    }

    //--------------------------------------------------------------------------
    // Inject excitation impulse(s)
    //--------------------------------------------------------------------------
    void injectExcitation(float excitationAmt, float excNodeParam) noexcept
    {
        // ops_excNode: 0 = node 0 only, quantized steps up to 1.0 = all nodes
        const int maxExcNodes = static_cast<int>(excNodeParam * static_cast<float>(kOpsinNodes - 1) + 0.5f) + 1;
        const float amplitude  = excitationAmt * 0.5f;

        for (int n = 0; n < std::min(maxExcNodes, kOpsinNodes); ++n)
            writeDelay(nodes[n], amplitude);
    }

    //--------------------------------------------------------------------------
    // Build/update synapse topology
    //--------------------------------------------------------------------------
    void buildTopology(int topology, float density, float feedback,
                       float asymmetry, float scatter) noexcept
    {
        const int N = kOpsinNodes;

        // Reset base weights
        std::memset(baseWeights, 0, sizeof(baseWeights));

        float baseStrength = feedback;

        switch (topology)
        {
            case 0: // Ring: each node connects to 2 neighbors (circular)
            {
                for (int i = 0; i < N; ++i)
                {
                    int left  = (i - 1 + N) % N;
                    int right = (i + 1) % N;
                    baseWeights[i][left]  = baseStrength;
                    baseWeights[i][right] = baseStrength;
                }
                break;
            }
            case 1: // Star: node 0 is hub; all others connect only to node 0
            {
                for (int i = 1; i < N; ++i)
                {
                    baseWeights[i][0] = baseStrength;
                    baseWeights[0][i] = baseStrength;
                }
                break;
            }
            case 2: // Mesh: all-to-all connections
            {
                const float allStrength = baseStrength / static_cast<float>(N - 1);
                for (int i = 0; i < N; ++i)
                    for (int j = 0; j < N; ++j)
                        if (i != j)
                            baseWeights[i][j] = allStrength;
                break;
            }
            case 3: // Cascade: one-way chain 0→1→2→3→4→5
            {
                for (int i = 0; i < N - 1; ++i)
                    baseWeights[i + 1][i] = baseStrength;
                break;
            }
            case 4: // Random: seeded from a hash of note + time
            {
                uint32_t rng = static_cast<uint32_t>(currentNote + 1) * 2654435761u
                               ^ static_cast<uint32_t>(noteRandSeed);
                for (int i = 0; i < N; ++i)
                    for (int j = 0; j < N; ++j)
                    {
                        rng = rng * 1664525u + 1013904223u;
                        const float w = (static_cast<float>(rng & 0xFFFF) / 32767.5f - 1.0f) * baseStrength;
                        baseWeights[i][j] = w;
                    }
                break;
            }
            default: break;
        }

        // Apply asymmetry: scale forward vs backward differently
        if (asymmetry > 1e-4f)
        {
            for (int i = 0; i < N; ++i)
                for (int j = 0; j < N; ++j)
                {
                    const float fwd = baseWeights[i][j];
                    const float bwd = baseWeights[j][i];
                    baseWeights[i][j] = lerp(fwd, bwd * 0.1f, asymmetry);
                }
        }

        // Apply scatter: random perturbation of weights
        if (scatter > 1e-4f)
        {
            uint32_t rng2 = 987654321u;
            for (int i = 0; i < N; ++i)
                for (int j = 0; j < N; ++j)
                {
                    rng2 = rng2 * 1664525u + 1013904223u;
                    const float perturbation = (static_cast<float>(rng2 & 0xFF) / 127.5f - 1.0f)
                                               * scatter * 0.3f;
                    baseWeights[i][j] += perturbation;
                }
        }

        // Apply density mask: zero out weights below threshold
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
            {
                const float mag = std::fabs(baseWeights[i][j]);
                const float threshold = (1.0f - density) * baseStrength;
                if (mag < threshold)
                    baseWeights[i][j] = 0.0f;
            }

        // Clamp all weights to [-1, 1]
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                baseWeights[i][j] = clamp(baseWeights[i][j], -1.0f, 1.0f);

        // Copy base weights to live weights if this is a fresh topology build
        // (We only copy base → live when topology parameters changed)
        // For gradual approach: blend current live weights toward base
        for (int i = 0; i < N; ++i)
            for (int j = 0; j < N; ++j)
                synapseWeights[i][j] = lerp(synapseWeights[i][j], baseWeights[i][j], 0.01f);
    }

    //--------------------------------------------------------------------------
    // Hebbian learning update (called once per learning window)
    //--------------------------------------------------------------------------
    void updateHebbianWeights(float plasticity, float dt) noexcept
    {
        const int N = kOpsinNodes;
        for (int i = 0; i < N; ++i)
        {
            float rowSum = 0.0f;
            for (int j = 0; j < N; ++j)
            {
                if (i == j) continue;
                if (nodes[i].firedThisWindow && nodes[j].firedThisWindow)
                {
                    // Hebbian strengthening: fire together, wire together
                    synapseWeights[i][j] += plasticity * dt * 0.5f;
                }
                synapseWeights[i][j] = clamp(synapseWeights[i][j], -1.0f, 1.0f);
                rowSum += std::fabs(synapseWeights[i][j]);
            }
            // L1 normalization per row to prevent unbounded growth
            if (rowSum > 1.0f)
            {
                const float invSum = 1.0f / rowSum;
                for (int j = 0; j < N; ++j)
                    if (i != j)
                        synapseWeights[i][j] *= invSum;
            }
        }

        // Reset firing windows
        for (int n = 0; n < N; ++n)
            nodes[n].firedThisWindow = false;
    }

    //--------------------------------------------------------------------------
    // Spectral radius safety — approximate via power iteration (1 step)
    //--------------------------------------------------------------------------
    void enforceSpectralRadius(float maxRadius) noexcept
    {
        const int N = kOpsinNodes;

        // Approximate spectral radius via max absolute row sum (matrix infinity norm)
        float maxRowSum = 0.0f;
        for (int i = 0; i < N; ++i)
        {
            float rowSum = 0.0f;
            for (int j = 0; j < N; ++j)
                rowSum += std::fabs(synapseWeights[i][j]);
            maxRowSum = std::max(maxRowSum, rowSum);
        }

        if (maxRowSum > maxRadius && maxRowSum > 1e-6f)
        {
            const float scale = maxRadius / maxRowSum;
            for (int i = 0; i < N; ++i)
                for (int j = 0; j < N; ++j)
                    synapseWeights[i][j] *= scale;
        }
    }

    //--------------------------------------------------------------------------
    // Output filter mode update
    //--------------------------------------------------------------------------
    void updateFilterMode(int fltType) noexcept
    {
        switch (fltType)
        {
            case 0: outputFilter.setMode(CytomicSVF::Mode::LowPass);  break;
            case 1: outputFilter.setMode(CytomicSVF::Mode::HighPass); break;
            case 2: outputFilter.setMode(CytomicSVF::Mode::BandPass); break;
            case 3: outputFilter.setMode(CytomicSVF::Mode::Notch);    break;
            default: outputFilter.setMode(CytomicSVF::Mode::LowPass); break;
        }
    }

    //--------------------------------------------------------------------------
    // Per-node resonant filter settings (material-specific coloring)
    //--------------------------------------------------------------------------
    void updateNodeFilter(int nodeIdx, float material, float spread, float spreadOffset) noexcept
    {
        // material 0-7 maps to different resonant frequencies and Q
        // Each material gives the node a characteristic "tone color"
        struct MaterialPreset { float baseFreq; float reso; };
        static const MaterialPreset kMaterials[8] = {
            { 220.0f,  0.6f },   // 0 Membrane  — low, resonant
            { 880.0f,  0.75f},   // 1 Glass      — high, glassy
            { 2200.0f, 0.85f},   // 2 Crystal    — very bright, high-Q
            { 440.0f,  0.55f},   // 3 Coral      — mid, organic
            { 80.0f,   0.45f},   // 4 Void       — sub, dark
            { 1760.0f, 0.70f},   // 5 Metal      — metallic, singing
            { 660.0f,  0.50f},   // 6 Silk       — smooth, veiled
            { 3300.0f, 0.90f},   // 7 Plasma     — extreme bright, near self-osc
        };

        const int matInt   = static_cast<int>(material) % 8;
        const int matNext  = (matInt + 1) % 8;
        const float matFrac = material - std::floor(material);

        const float nodeFreq = lerp(kMaterials[matInt].baseFreq, kMaterials[matNext].baseFreq, matFrac);
        const float nodeReso = lerp(kMaterials[matInt].reso,     kMaterials[matNext].reso,     matFrac);

        // Spread offsets the cutoff per node
        const float offsettedFreq = clamp(nodeFreq * (1.0f + spreadOffset * spread * 0.5f),
                                          20.0f, sampleRateFloat * 0.45f);

        nodes[nodeIdx].resonantFilter.setCoefficients_fast(offsettedFreq, nodeReso, sampleRateFloat);
        nodes[nodeIdx].resonantFilter.setMode(CytomicSVF::Mode::BandPass); // each node colors with BP
    }

    //==========================================================================
    //  M E M B E R S
    //==========================================================================

    // ---- Audio engine state ----
    PhotophoreNode nodes[kOpsinNodes];

    // ---- Synapse matrix (live + base) ----
    float synapseWeights[kOpsinNodes][kOpsinNodes] = {};
    float baseWeights   [kOpsinNodes][kOpsinNodes] = {};

    // ---- Output filter ----
    CytomicSVF outputFilter;

    // ---- Envelopes ----
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // ---- LFOs ----
    StandardLFO lfo1;
    StandardLFO lfo2;

    // ---- Mod matrix ----
    ModMatrix<4> modMatrix;

    // ---- Voice state ----
    int   currentNote  = -1;
    float currentFreq  = 110.0f;
    float targetFreq   = 110.0f;
    float glideFreq    = 110.0f;
    float velocity     = 0.0f;
    float modWheelValue   = 0.0f;
    float aftertouchValue = 0.0f;
    int   noteRandSeed = 42;

    // ---- Pending excitation (applied on note-on, after params read) ----
    float pendingExcitation = 0.0f;
    float pendingExcNode    = 0.0f;

    // ---- Smoothed parameter state ----
    float smoothFeedback   = 0.40f;
    float smoothThreshold  = 0.45f;
    float smoothCutoff     = 3400.0f;
    float smoothExcitation = 0.55f;
    float smoothDensity    = 0.35f;
    float smoothAmpLevel   = 1.0f;
    float smoothCoeff      = 0.01f; // 1-pole coefficient (~5ms)

    // ---- Energy Governor ----
    float govFollower     = 0.0f;
    float govFollowerCoeff = 0.001f;

    // ---- Hebbian state ----
    float hebbianTimer   = 0.0f;

    // ---- Coupling accumulation ----
    float couplingFilterMod   = 0.0f;
    float couplingFeedbackMod = 0.0f;
    float couplingExciteMod   = 0.0f;
    int   couplingNodeRR      = 0;

    // ---- Output cache (for coupling reads) ----
    std::vector<float> outCacheL;
    std::vector<float> outCacheR;

    // ---- Sample rate ----
    float sampleRateFloat = 44100.0f;

    //==========================================================================
    //  P A R A M E T E R   P O I N T E R S
    //==========================================================================

    // A — Node Character
    std::atomic<float>* pMaterial    = nullptr;
    std::atomic<float>* pSpread      = nullptr;
    std::atomic<float>* pThreshold   = nullptr;
    std::atomic<float>* pRefractory  = nullptr;
    std::atomic<float>* pPitchTrack  = nullptr;
    std::atomic<float>* pDelayBase   = nullptr;
    // B — Feedback Matrix
    std::atomic<float>* pTopology    = nullptr;
    std::atomic<float>* pDensity     = nullptr;
    std::atomic<float>* pFeedback    = nullptr;
    std::atomic<float>* pAsymmetry   = nullptr;
    std::atomic<float>* pScatter     = nullptr;
    // C — Hebbian Learning
    std::atomic<float>* pPlasticity  = nullptr;
    std::atomic<float>* pMemory      = nullptr;
    std::atomic<float>* pLearnWindow = nullptr;
    // D — Energy Governor
    std::atomic<float>* pGovCeiling  = nullptr;
    std::atomic<float>* pGovSpeed    = nullptr;
    std::atomic<float>* pExcitation  = nullptr;
    std::atomic<float>* pExcNode     = nullptr;
    // E — Filter
    std::atomic<float>* pFltCutoff   = nullptr;
    std::atomic<float>* pFltReso     = nullptr;
    std::atomic<float>* pFltType     = nullptr;
    std::atomic<float>* pFltEnvAmt   = nullptr;
    std::atomic<float>* pFltKeyTrack = nullptr;
    // F — Amp Envelope
    std::atomic<float>* pAmpAtk      = nullptr;
    std::atomic<float>* pAmpDec      = nullptr;
    std::atomic<float>* pAmpSus      = nullptr;
    std::atomic<float>* pAmpRel      = nullptr;
    // G — Filter Envelope
    std::atomic<float>* pFenvAtk     = nullptr;
    std::atomic<float>* pFenvDec     = nullptr;
    std::atomic<float>* pFenvSus     = nullptr;
    std::atomic<float>* pFenvRel     = nullptr;
    // H — LFOs
    std::atomic<float>* pLfo1Rate    = nullptr;
    std::atomic<float>* pLfo1Depth   = nullptr;
    std::atomic<float>* pLfo1Shape   = nullptr;
    std::atomic<float>* pLfo1Target  = nullptr;
    std::atomic<float>* pLfo2Rate    = nullptr;
    std::atomic<float>* pLfo2Depth   = nullptr;
    std::atomic<float>* pLfo2Shape   = nullptr;
    std::atomic<float>* pLfo2Target  = nullptr;
    // J — Macros + Voice
    std::atomic<float>* pMacro1      = nullptr;
    std::atomic<float>* pMacro2      = nullptr;
    std::atomic<float>* pMacro3      = nullptr;
    std::atomic<float>* pMacro4      = nullptr;
    std::atomic<float>* pVoiceMode   = nullptr;
    std::atomic<float>* pGlide       = nullptr;
};

} // namespace xoceanus
