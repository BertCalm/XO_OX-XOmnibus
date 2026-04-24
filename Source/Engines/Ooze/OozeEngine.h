// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
// =============================================================================
// OozeEngine.h — XOoze: Fluid Dynamics Synthesis (#90)
// "The Fluid Engine"
//
// Digital waveguide pipe + Reynolds number jet excitation + Minnaert bubble
// resonance. Three fluid phenomena as one instrument. The laminar-to-turbulent
// transition is the timbral axis — one knob from clean flute to overblown scream.
//
// Physics: Reynolds 1883, Minnaert 1933, Smith 2010, Verge 1997.
// QDD-hardened: waveguide soft limiter, reflection gain ceiling 0.999,
// coupling soft-clip at 30%, allpass fractional delay, DC blocking.
//
// Water Column: Hadal Zone | Creature: Giant Tube Worm
// Accent: Hydrothermal Teal #2D5F5D
// Prefix: ooze_ | Gallery: OOZE
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
// OozeVoice — per-voice waveguide + jet + bubble state and DSP chain
// =============================================================================
struct OozeVoice
{
    bool  active    = false;
    bool  releasing = false;
    int   midiNote  = -1;
    float velocity  = 0.0f;
    float frequency       = 440.0f;
    float targetFrequency = 440.0f;

    // ── Waveguide delay line ─────────────────────────────────────────────────
    static constexpr int kMaxDelay = 2048;
    float delayLine[kMaxDelay] = {};
    int   delayWritePos = 0;
    float delayLength       = 100.0f; // fractional sample length
    float targetDelayLength = 100.0f;

    // Allpass fractional delay state (first-order allpass interpolation)
    float allpassPrev   = 0.0f; // y[n-1] of allpass output
    float allpassPrevIn = 0.0f; // x[n-1] of allpass input

    // Reflection filters — one-pole LP state for each pipe end
    float reflFilterStateA = 0.0f; // open end (inverted)
    float reflFilterStateB = 0.0f; // closed/half end

    // ── Jet excitation state ─────────────────────────────────────────────────
    float jetPhase = 0.0f; // edge-tone oscillator phase [0, 2π)

    // Kolmogorov 1-pole LP for -5/3 noise spectral tilt
    float kolmogorovState = 0.0f;

    // Stochastic bursting state (transitional Reynolds zone)
    float burstPhase = 0.0f; // 0-1 sawtooth for burst duty cycle

    // Cascade phase (temporal evolution of turbulent cascade)
    float cascadePhase = 0.0f;

    // ── Minnaert bubble state ────────────────────────────────────────────────
    float bubblePhase     = 0.0f; // oscillator phase [0, 1)
    float bubbleAmplitude = 0.0f; // current amplitude (exponential decay)
    float bubbleTimer     = 0.0f; // seconds since last trigger

    // ── Envelopes & modulators ───────────────────────────────────────────────
    xoceanus::StandardADSR ampEnv;
    xoceanus::StandardLFO  lfo;

    // Output filter (L/R independent for stereo imaging)
    xoceanus::CytomicSVF outputFilterL;
    xoceanus::CytomicSVF outputFilterR;

    // DC blocker
    xoceanus::DCBlocker dcBlockerL;
    xoceanus::DCBlocker dcBlockerR;

    // Glide processor
    xoceanus::GlideProcessor glide;

    // ── Coupling & voice management ──────────────────────────────────────────
    float lastCouplingOut = 0.0f;

    // Voice stealing crossfade
    float crossfadeGain = 1.0f;
    float crossfadeRate = 0.0f;

    // ── Per-voice LCG noise generator ───────────────────────────────────────
    uint32_t noiseState = 12345u;

    float nextNoise() noexcept
    {
        noiseState = noiseState * 1664525u + 1013904223u;
        return static_cast<float>(noiseState & 0xFFFF) / 32768.0f - 1.0f;
    }

    // ── reset ────────────────────────────────────────────────────────────────
    void reset(double sampleRate) noexcept
    {
        active = false; releasing = false; midiNote = -1;
        velocity = 0.0f; frequency = 440.0f; targetFrequency = 440.0f;
        std::memset(delayLine, 0, sizeof(delayLine));
        delayWritePos = 0;
        delayLength = targetDelayLength = 100.0f;
        allpassPrev = 0.0f; allpassPrevIn = 0.0f;
        reflFilterStateA = 0.0f; reflFilterStateB = 0.0f;
        jetPhase = 0.0f;
        kolmogorovState = 0.0f;
        burstPhase = 0.0f; cascadePhase = 0.0f;
        bubblePhase = 0.0f; bubbleAmplitude = 0.0f; bubbleTimer = 0.0f;
        crossfadeGain = 1.0f; crossfadeRate = 0.0f;
        lastCouplingOut = 0.0f;
        ampEnv.reset(); lfo.reset();
        outputFilterL.reset(); outputFilterR.reset();
        dcBlockerL.prepare(sampleRate);
        dcBlockerR.prepare(sampleRate);
        glide.reset();
    }
};

// =============================================================================
// OozeParamSnapshot — cache all parameter pointers once per block
// =============================================================================
struct OozeParamSnapshot
{
    // Pipe group
    std::atomic<float>* pEndType  = nullptr;
    std::atomic<float>* pGeometry = nullptr;
    std::atomic<float>* pDamping  = nullptr;

    // Fluid group
    std::atomic<float>* pReynolds  = nullptr;
    std::atomic<float>* pPressure  = nullptr;
    std::atomic<float>* pViscosity = nullptr;
    std::atomic<float>* pJetOffset = nullptr;

    // Bubble group
    std::atomic<float>* pBubbleRate  = nullptr;
    std::atomic<float>* pBubbleSize  = nullptr;
    std::atomic<float>* pBubbleDecay = nullptr;
    std::atomic<float>* pBubbleTrack = nullptr;

    // Turbulence group
    std::atomic<float>* pBurstRate = nullptr;
    std::atomic<float>* pCascade   = nullptr;

    // Output group
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso   = nullptr;
    std::atomic<float>* pFilterType   = nullptr;
    std::atomic<float>* pDrive        = nullptr;

    // Envelope group
    std::atomic<float>* pAttack  = nullptr;
    std::atomic<float>* pDecay   = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pRelease = nullptr;

    // Modulation group
    std::atomic<float>* pLfoRate   = nullptr;
    std::atomic<float>* pLfoShape  = nullptr;
    std::atomic<float>* pLfoDepth  = nullptr;
    std::atomic<float>* pLfoTarget = nullptr;

    // Space group
    std::atomic<float>* pReverb     = nullptr;
    std::atomic<float>* pSpaceDecay = nullptr;

    // Performance group
    std::atomic<float>* pTune  = nullptr;
    std::atomic<float>* pFine  = nullptr;
    std::atomic<float>* pGlide = nullptr;

    // Macros
    std::atomic<float>* pMacroChar  = nullptr;
    std::atomic<float>* pMacroMove  = nullptr;
    std::atomic<float>* pMacroCoupl = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;

    // ── Snapshot values (loaded once per block) ──────────────────────────────
    int   endType   = 0;     // 0=Open, 1=Closed, 2=Half
    float geometry  = 0.3f;  // 0=cylindrical, 1=conical
    float damping   = 0.2f;

    float reynolds  = 0.15f;
    float pressure  = 0.5f;
    float viscosity = 0.3f;
    float jetOffset = 0.5f;

    float bubbleRate  = 0.2f;
    float bubbleSize  = 0.5f;
    float bubbleDecay = 0.5f;
    float bubbleTrack = 0.0f;

    float burstRate = 5.0f;
    float cascade   = 0.3f;

    float filterCutoff = 12000.0f;
    float filterReso   = 0.15f;
    int   filterType   = 0;
    float drive        = 0.15f;

    float attack  = 0.05f;
    float decay   = 0.5f;
    float sustain = 0.7f;
    float release = 1.0f;

    float lfoRate   = 0.2f;
    int   lfoShape  = 0;
    float lfoDepth  = 0.3f;
    int   lfoTarget = 0;

    float reverb     = 0.25f;
    float spaceDecay = 2.5f;

    int   tune  = 0;
    float fine  = 0.0f;
    float glide = 0.0f;

    float macroChar  = 0.0f;
    float macroMove  = 0.0f;
    float macroCoupl = 0.0f;
    float macroSpace = 0.0f;

    // ── helpers ──────────────────────────────────────────────────────────────
    inline float load(std::atomic<float>* p, float def) const noexcept
    {
        return p ? p->load(std::memory_order_relaxed) : def;
    }

    void updateFrom() noexcept
    {
        endType   = static_cast<int>(load(pEndType,  0.0f) + 0.5f);
        geometry  = load(pGeometry, 0.3f);
        damping   = load(pDamping,  0.2f);

        reynolds  = load(pReynolds,  0.15f);
        pressure  = load(pPressure,  0.5f);
        viscosity = load(pViscosity, 0.3f);
        jetOffset = load(pJetOffset, 0.5f);

        bubbleRate  = load(pBubbleRate,  0.2f);
        bubbleSize  = load(pBubbleSize,  0.5f);
        bubbleDecay = load(pBubbleDecay, 0.5f);
        bubbleTrack = load(pBubbleTrack, 0.0f);

        burstRate = load(pBurstRate, 5.0f);
        cascade   = load(pCascade,   0.3f);

        filterCutoff = load(pFilterCutoff, 12000.0f);
        filterReso   = load(pFilterReso,   0.15f);
        filterType   = static_cast<int>(load(pFilterType, 0.0f) + 0.5f);
        drive        = load(pDrive,        0.15f);

        attack  = load(pAttack,  0.05f);
        decay   = load(pDecay,   0.5f);
        sustain = load(pSustain, 0.7f);
        release = load(pRelease, 1.0f);

        lfoRate   = load(pLfoRate,   0.2f);
        lfoShape  = static_cast<int>(load(pLfoShape,  0.0f) + 0.5f);
        lfoDepth  = load(pLfoDepth,  0.3f);
        lfoTarget = static_cast<int>(load(pLfoTarget, 0.0f) + 0.5f);

        reverb     = load(pReverb,     0.25f);
        spaceDecay = load(pSpaceDecay, 2.5f);

        tune  = static_cast<int>(load(pTune,  0.0f) + 0.5f);
        fine  = load(pFine,  0.0f);
        glide = load(pGlide, 0.0f);

        macroChar  = load(pMacroChar,  0.0f);
        macroMove  = load(pMacroMove,  0.0f);
        macroCoupl = load(pMacroCoupl, 0.0f);
        macroSpace = load(pMacroSpace, 0.0f);
    }
};

// =============================================================================
// OozeFDN — 4-line feedback delay network reverb
// =============================================================================
struct OozeFDN
{
    static constexpr int kNumLines = 4;
    // Prime delay lengths (samples at 44100 Hz, scaled for actual SR)
    static constexpr int kDelayLengths[kNumLines] = { 1481, 1847, 2203, 2617 };

    std::vector<float> bufL[kNumLines];
    std::vector<float> bufR[kNumLines];
    int   writePos[kNumLines] = {};
    int   delayLen[kNumLines] = {};
    float feedback = 0.5f;
    float wet      = 0.25f;

    void prepare(double sampleRate, int /*maxBlockSize*/) noexcept
    {
        const float sr = static_cast<float>(sampleRate);
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

    void processSample(float inL, float inR, float& outL, float& outR) noexcept
    {
        // Read delayed samples
        float dL[kNumLines], dR[kNumLines];
        for (int i = 0; i < kNumLines; ++i)
        {
            const int readPos = (writePos[i] - delayLen[i]
                + static_cast<int>(bufL[i].size())) % static_cast<int>(bufL[i].size());
            dL[i] = bufL[i][static_cast<size_t>(readPos)];
            dR[i] = bufR[i][static_cast<size_t>(readPos)];
        }

        // Hadamard mixing matrix (normalised 4x4), scaled 0.5
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
// OozeEngine — SynthEngine implementation
// =============================================================================
class OozeEngine : public xoceanus::SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    // =========================================================================
    // SynthEngine interface
    // =========================================================================

    juce::String getEngineId()    const override { return "Ooze"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0x2D, 0x5F, 0x5D); }
    int          getMaxVoices()   const override { return kMaxVoices; }

    // -------------------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P = juce::AudioParameterFloat;
        using C = juce::AudioParameterChoice;
        using I = juce::AudioParameterInt;

        // ── Pipe group ────────────────────────────────────────────────────────
        params.push_back(std::make_unique<C>(
            juce::ParameterID("ooze_end_type", 1), "End Type",
            juce::StringArray{"Open", "Closed", "Half"}, 0));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_geometry", 1), "Geometry",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_damping", 1), "Damping",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        // ── Fluid group ───────────────────────────────────────────────────────
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_reynolds", 1), "Reynolds",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_pressure", 1), "Pressure",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_viscosity", 1), "Viscosity",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        {
            juce::NormalisableRange<float> joRange(0.0f, 1.0f, 0.001f);
            joRange.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("ooze_jet_offset", 1), "Jet Offset", joRange, 0.5f));
        }

        // ── Bubble group ──────────────────────────────────────────────────────
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_bubble_rate", 1), "Bubble Rate",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_bubble_size", 1), "Bubble Size",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_bubble_decay", 1), "Bubble Decay",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_bubble_track", 1), "Bubble Track",
            juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));

        // ── Turbulence group ──────────────────────────────────────────────────
        {
            juce::NormalisableRange<float> brRange(0.1f, 20.0f, 0.01f);
            brRange.setSkewForCentre(4.0f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("ooze_burst_rate", 1), "Burst Rate", brRange, 5.0f));
        }
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_cascade", 1), "Cascade",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // ── Output group ──────────────────────────────────────────────────────
        {
            juce::NormalisableRange<float> fcRange(80.0f, 20000.0f, 0.1f);
            fcRange.setSkewForCentre(1500.0f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("ooze_filter_cutoff", 1), "Filter", fcRange, 12000.0f));
        }
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_filter_reso", 1), "Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));
        params.push_back(std::make_unique<C>(
            juce::ParameterID("ooze_filter_type", 1), "Filter Type",
            juce::StringArray{"LP", "HP", "BP", "Notch"}, 0));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_drive", 1), "Drive",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

        // ── Envelope group ────────────────────────────────────────────────────
        {
            juce::NormalisableRange<float> atRange(0.001f, 5.0f, 0.001f);
            atRange.setSkewForCentre(0.1f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("ooze_attack", 1), "Attack", atRange, 0.05f));
        }
        {
            juce::NormalisableRange<float> dcRange(0.01f, 10.0f, 0.001f);
            dcRange.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("ooze_decay", 1), "Decay", dcRange, 0.5f));
        }
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_sustain", 1), "Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.7f));
        {
            juce::NormalisableRange<float> rlRange(0.01f, 10.0f, 0.001f);
            rlRange.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("ooze_release", 1), "Release", rlRange, 1.0f));
        }

        // ── Modulation group ──────────────────────────────────────────────────
        {
            juce::NormalisableRange<float> lfoRange(0.005f, 12.0f, 0.001f);
            lfoRange.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("ooze_lfo_rate", 1), "LFO Rate", lfoRange, 0.2f));
        }
        params.push_back(std::make_unique<C>(
            juce::ParameterID("ooze_lfo_shape", 1), "LFO Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "S&H"}, 0));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_lfo_depth", 1), "LFO Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(std::make_unique<C>(
            juce::ParameterID("ooze_lfo_target", 1), "LFO Target",
            juce::StringArray{"Reynolds", "Pressure", "BubbleRate", "Filter",
                              "Viscosity", "JetOffset", "Cascade"}, 0));

        // ── Space group ───────────────────────────────────────────────────────
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_reverb", 1), "Reverb",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));
        {
            juce::NormalisableRange<float> sdRange(0.1f, 8.0f, 0.01f);
            sdRange.setSkewForCentre(1.0f);
            params.push_back(std::make_unique<P>(
                juce::ParameterID("ooze_space_decay", 1), "Space Decay", sdRange, 2.5f));
        }

        // ── Performance group ─────────────────────────────────────────────────
        params.push_back(std::make_unique<I>(
            juce::ParameterID("ooze_tune", 1), "Tune", -24, 24, 0));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_fine", 1), "Fine",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_glide", 1), "Glide",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // ── Macros ────────────────────────────────────────────────────────────
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_macro1", 1), "Character",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_macro2", 1), "Movement",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_macro3", 1), "Coupling",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<P>(
            juce::ParameterID("ooze_macro4", 1), "Breath",
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

        snap_.pEndType  = get("ooze_end_type");
        snap_.pGeometry = get("ooze_geometry");
        snap_.pDamping  = get("ooze_damping");

        snap_.pReynolds  = get("ooze_reynolds");
        snap_.pPressure  = get("ooze_pressure");
        snap_.pViscosity = get("ooze_viscosity");
        snap_.pJetOffset = get("ooze_jet_offset");

        snap_.pBubbleRate  = get("ooze_bubble_rate");
        snap_.pBubbleSize  = get("ooze_bubble_size");
        snap_.pBubbleDecay = get("ooze_bubble_decay");
        snap_.pBubbleTrack = get("ooze_bubble_track");

        snap_.pBurstRate = get("ooze_burst_rate");
        snap_.pCascade   = get("ooze_cascade");

        snap_.pFilterCutoff = get("ooze_filter_cutoff");
        snap_.pFilterReso   = get("ooze_filter_reso");
        snap_.pFilterType   = get("ooze_filter_type");
        snap_.pDrive        = get("ooze_drive");

        snap_.pAttack  = get("ooze_attack");
        snap_.pDecay   = get("ooze_decay");
        snap_.pSustain = get("ooze_sustain");
        snap_.pRelease = get("ooze_release");

        snap_.pLfoRate   = get("ooze_lfo_rate");
        snap_.pLfoShape  = get("ooze_lfo_shape");
        snap_.pLfoDepth  = get("ooze_lfo_depth");
        snap_.pLfoTarget = get("ooze_lfo_target");

        snap_.pReverb     = get("ooze_reverb");
        snap_.pSpaceDecay = get("ooze_space_decay");

        snap_.pTune  = get("ooze_tune");
        snap_.pFine  = get("ooze_fine");
        snap_.pGlide = get("ooze_glide");

        snap_.pMacroChar  = get("ooze_macro1");
        snap_.pMacroMove  = get("ooze_macro2");
        snap_.pMacroCoupl = get("ooze_macro3");
        snap_.pMacroSpace = get("ooze_macro4");
    }

    // -------------------------------------------------------------------------
    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate_ = sampleRate;
        maxBlockSize_      = maxBlockSize;

        for (int vi = 0; vi < kMaxVoices; ++vi)
        {
            voices_[vi].reset(sampleRate);
            // Seed per-voice noise with unique value so all voices start decorrelated.
            voices_[vi].noiseState = 12345u + static_cast<uint32_t>(vi) * 1234567u;
        }

        fdn_.prepare(sampleRate, maxBlockSize);
        engineDCBlockerL_.prepare(sampleRate);
        engineDCBlockerR_.prepare(sampleRate);

        voiceBufL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        voiceBufR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheL_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR_.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        pitchBend_        = 1.0f;
        modWheel_         = 0.0f;
        breathCC_         = 0.0f;
        expression_       = 0.0f;
        aftertouchGlobal_ = 0.0f;

        couplingAudioAccum_  = 0.0f;
        couplingMorphAccum_  = 0.0f;
        couplingFilterAccum_ = 0.0f;

        // Reverb-tail hold: pipe resonance can ring for a while
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
        engineDCBlockerL_.prepare(currentSampleRate_);
        engineDCBlockerR_.prepare(currentSampleRate_);
        pitchBend_ = 1.0f;
        modWheel_ = breathCC_ = expression_ = aftertouchGlobal_ = 0.0f;
        couplingAudioAccum_ = couplingMorphAccum_ = couplingFilterAccum_ = 0.0f;
    }

    // =========================================================================
    // renderBlock — main audio generation
    // =========================================================================
    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // ── Step 1: Wake silence gate on any note-on (BEFORE bypass check) ───
        for (const auto& meta : midi)
        {
            if (meta.getMessage().isNoteOn())
                wakeSilenceGate();
        }

        // ── Step 2: Silence gate bypass ───────────────────────────────────────
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            return;
        }

        // ── Step 3: Load parameter snapshot once per block ────────────────────
        snap_.updateFrom();

        // ── Step 4: Apply macros ──────────────────────────────────────────────
        const float m1 = snap_.macroChar;
        const float m2 = snap_.macroMove;
        const float m3 = snap_.macroCoupl;
        const float m4 = snap_.macroSpace;

        // M1 CHARACTER: reynolds + geometry
        const float effReynolds = juce::jlimit(0.0f, 1.0f,
            snap_.reynolds + m1 * 0.5f);
        const float effGeometry = juce::jlimit(0.0f, 1.0f,
            snap_.geometry + m1 * 0.4f);

        // M2 MOVEMENT: pressure + bubble_rate + lfo_depth
        const float effPressure = juce::jlimit(0.0f, 1.0f,
            snap_.pressure + m2 * 0.3f);
        const float effBubbleRate = juce::jlimit(0.0f, 1.0f,
            snap_.bubbleRate + m2 * 0.4f);
        const float effLfoDepth = juce::jlimit(0.0f, 1.0f,
            snap_.lfoDepth + m2 * 0.3f);

        // M3 COUPLING: viscosity - 0.3 + jet_offset + 0.3
        const float effViscosity = juce::jlimit(0.0f, 1.0f,
            snap_.viscosity - m3 * 0.3f);
        const float effJetOffset = juce::jlimit(0.0f, 1.0f,
            snap_.jetOffset + m3 * 0.3f);

        // M4 BREATH (CC2 also maps here): reverb + 0.4, damping - 0.3
        const float breathTotal = std::min(1.0f, m4 + breathCC_);
        const float effReverb = juce::jlimit(0.0f, 1.0f,
            snap_.reverb + breathTotal * 0.4f);
        const float effDamping = juce::jlimit(0.0f, 1.0f,
            snap_.damping - breathTotal * 0.3f);

        // CC1 (mod wheel) → reynolds sweep (D006)
        const float effReynoldsWithMW = juce::jlimit(0.0f, 1.0f,
            effReynolds + modWheel_ * 0.4f);

        // CC11 (expression) → pressure modulation (D006)
        const float effPressureWithExpr = juce::jlimit(0.0f, 1.0f,
            effPressure + expression_ * 0.3f);

        // Aftertouch → viscosity + jamming at AT > 0.63 (D006)
        float atViscosityMod = aftertouchGlobal_ * 0.3f;
        float atJamFactor = 1.0f;
        if (aftertouchGlobal_ > 0.63f)
        {
            const float excess = (aftertouchGlobal_ - 0.63f) / 0.37f;
            // Steep curve: Reynolds freezes toward laminar under high AT
            atJamFactor = 1.0f - excess * excess * 0.6f;
        }
        const float effViscosityWithAT = juce::jlimit(0.0f, 1.0f,
            effViscosity + atViscosityMod);
        const float effReynoldsFinal = juce::jlimit(0.0f, 1.0f,
            effReynoldsWithMW * atJamFactor);

        // ── Step 5: Process MIDI events ───────────────────────────────────────
        processMidi(midi);

        // ── Step 6: Compute tuning constants ─────────────────────────────────
        const float tuneSemitones = static_cast<float>(snap_.tune)
                                  + snap_.fine / 100.0f;
        const float glideTimeSec  = snap_.glide * snap_.glide * 2.0f;
        [[maybe_unused]] const CytomicSVF::Mode filterMode = filterModeFromInt(snap_.filterType);

        // ── Step 7: FDN feedback from space_decay ─────────────────────────────
        // Map spaceDecay [0.1, 8] → feedback [0.2, 0.88]
        fdn_.feedback = juce::jlimit(0.2f, 0.88f,
            0.2f + (snap_.spaceDecay - 0.1f) / (8.0f - 0.1f) * 0.68f);

        // ── Step 8: Output pointers ───────────────────────────────────────────
        float* outL = buffer.getWritePointer(0);
        float* outR = (buffer.getNumChannels() > 1) ? buffer.getWritePointer(1) : outL;

        std::fill(outL, outL + numSamples, 0.0f);
        if (buffer.getNumChannels() > 1)
            std::fill(outR, outR + numSamples, 0.0f);

        // ── Step 9: Render each active voice ──────────────────────────────────
        int activeCount = 0;

        for (auto& voice : voices_)
        {
            if (!voice.active)
                continue;

            ++activeCount;

            std::fill(voiceBufL_.begin(), voiceBufL_.begin() + numSamples, 0.0f);
            std::fill(voiceBufR_.begin(), voiceBufR_.begin() + numSamples, 0.0f);

            // Glide time
            voice.glide.setTime(glideTimeSec, static_cast<float>(currentSampleRate_));

            // Base frequency with tune/fine/pitch bend
            const float baseFreq = midiToFreqTune(voice.midiNote, tuneSemitones);
            voice.targetFrequency = baseFreq * pitchBend_;
            voice.glide.setTarget(voice.targetFrequency);

            // Envelope parameters
            voice.ampEnv.setParams(snap_.attack, snap_.decay, snap_.sustain, snap_.release,
                                   static_cast<float>(currentSampleRate_));

            // LFO
            voice.lfo.setRate(snap_.lfoRate, static_cast<float>(currentSampleRate_));
            voice.lfo.setShape(snap_.lfoShape);

            // Velocity-scaled effects (D001)
            const float velGain       = voice.velocity * voice.velocity;        // vel² pressure
            const float velReynoldsMod = voice.velocity * 0.3f;                 // harder = more turbulent
            const float velFilterMod   = voice.velocity * voice.velocity * 4000.0f; // vel² → filter brightness

            // Per-voice effective parameters
            const float voiceReynolds  = juce::jlimit(0.0f, 1.0f,
                effReynoldsFinal + velReynoldsMod + couplingMorphAccum_);
            const float voicePressure  = juce::jlimit(0.0f, 1.0f,
                effPressureWithExpr + couplingAudioAccum_);
            const float voiceFilterCutoff = juce::jlimit(80.0f, 20000.0f,
                snap_.filterCutoff + velFilterMod + couplingFilterAccum_ * 4000.0f);

            // Block-constant bubble pitch ratio: hoist std::pow out of per-sample loop.
            const float bubbleTrackRatio = fastPow2(snap_.bubbleTrack * (1.0f / 12.0f));

            // ── Per-sample loop ────────────────────────────────────────────────
            for (int s = 0; s < numSamples; ++s)
            {
                // ── LFO tick ─────────────────────────────────────────────────
                const float lfoVal  = voice.lfo.process();
                const float lmod    = lfoVal * effLfoDepth;

                // Apply LFO to target parameter
                float lfoReynolds   = voiceReynolds;
                float lfoPressure   = voicePressure;
                float lfoBubbleRate = effBubbleRate;
                float lfoFilterCut  = voiceFilterCutoff;
                float lfoViscosity  = effViscosityWithAT;
                float lfoJetOffset  = effJetOffset;
                float lfoCascade    = snap_.cascade;

                switch (snap_.lfoTarget)
                {
                    case 0: lfoReynolds   = juce::jlimit(0.0f, 1.0f, lfoReynolds   + lmod * 0.4f); break;
                    case 1: lfoPressure   = juce::jlimit(0.0f, 1.0f, lfoPressure   + lmod * 0.4f); break;
                    case 2: lfoBubbleRate = juce::jlimit(0.0f, 1.0f, lfoBubbleRate + lmod * 0.4f); break;
                    case 3: lfoFilterCut  = juce::jlimit(80.0f, 20000.0f, lfoFilterCut + lmod * 8000.0f); break;
                    case 4: lfoViscosity  = juce::jlimit(0.0f, 1.0f, lfoViscosity  + lmod * 0.4f); break;
                    case 5: lfoJetOffset  = juce::jlimit(0.0f, 1.0f, lfoJetOffset  + lmod * 0.4f); break;
                    case 6: lfoCascade    = juce::jlimit(0.0f, 1.0f, lfoCascade    + lmod * 0.4f); break;
                    default: break;
                }

                // ── Current (glide-processed) frequency ──────────────────────
                const float currentFreq = voice.glide.process();
                voice.frequency = currentFreq;

                // ── Jet excitation ────────────────────────────────────────────
                // Determine laminar/transitional/turbulent zone
                float toneAmount, noiseAmount;
                const float reNorm = lfoReynolds;

                if (reNorm < 0.25f)
                {
                    // Laminar zone
                    toneAmount  = 1.0f;
                    noiseAmount = reNorm * 0.1f;
                }
                else if (reNorm < 0.5f)
                {
                    // Transitional zone — stochastic bursting
                    const float t = (reNorm - 0.25f) * 4.0f; // 0-1 within zone
                    toneAmount  = 1.0f - t * 0.5f;
                    noiseAmount = t * 0.6f;

                    // Burst duty: burstPhase advances at burstRate Hz
                    voice.burstPhase += snap_.burstRate / static_cast<float>(currentSampleRate_);
                    if (voice.burstPhase >= 1.0f) voice.burstPhase -= 1.0f;
                    const float burstDuty = 0.4f; // 40% duty cycle
                    if (voice.burstPhase < burstDuty)
                        noiseAmount *= 2.0f; // burst!
                }
                else
                {
                    // Turbulent zone
                    toneAmount  = 0.3f;
                    noiseAmount = 0.7f + (reNorm - 0.5f) * 0.3f;
                }

                // Edge-tone sine oscillator — frequency at jet offset pitch
                // (fastSin: ~0.01% err, per-sample per-voice)
                const float edgeFreq = currentFreq * (1.0f + lfoJetOffset * 3.0f);
                constexpr float kTwoPi = 6.28318530718f;
                const float jetSine = fastSin(voice.jetPhase) * toneAmount;
                voice.jetPhase += edgeFreq / static_cast<float>(currentSampleRate_) * kTwoPi;
                if (voice.jetPhase >= kTwoPi) voice.jetPhase -= kTwoPi;

                // Kolmogorov-tinted noise (1-pole LP for -5/3 spectral tilt)
                const float rawNoise  = voice.nextNoise();
                const float koloAlpha = 0.3f + lfoViscosity * 0.5f;
                voice.kolmogorovState += koloAlpha * (rawNoise - voice.kolmogorovState);
                voice.kolmogorovState = flushDenormal(voice.kolmogorovState);
                const float jetNoise  = voice.kolmogorovState * noiseAmount;

                // Cascade: temporal AM on noise using a slower oscillator
                if (lfoCascade > 0.001f)
                {
                    voice.cascadePhase += (2.0f + lfoCascade * 8.0f) / static_cast<float>(currentSampleRate_);
                    if (voice.cascadePhase >= 1.0f) voice.cascadePhase -= 1.0f;
                    // cosine AM — cascade modulates turbulent energy envelope
                }

                // Combined excitation scaled by pressure and vel²
                float excitation = (jetSine + jetNoise) * lfoPressure * velGain;

                // ── Minnaert bubble excitation ────────────────────────────────
                // Bubble frequency: offset from note by bubble_track semitones
                // (bubbleTrackRatio precomputed above per-sample loop — fastPow2 cached)
                const float bubbleFreq = currentFreq
                    * bubbleTrackRatio
                    * (0.3f + snap_.bubbleSize * 1.7f); // size shifts pitch

                // Bubble trigger at effectiveBubbleRate Hz
                // lfoBubbleRate [0,1] → rate [0.1, 20] Hz
                const float effectiveBubbleRate = 0.1f + lfoBubbleRate * 19.9f;
                voice.bubbleTimer += 1.0f / static_cast<float>(currentSampleRate_);
                if (effectiveBubbleRate > 0.05f &&
                    voice.bubbleTimer >= (1.0f / effectiveBubbleRate))
                {
                    voice.bubbleTimer = 0.0f;
                    // Start at zero-crossing (avoid clicks!)
                    if (voice.bubbleAmplitude < 0.001f)
                    {
                        voice.bubblePhase = 0.0f;
                        voice.bubbleAmplitude = lfoPressure * 0.3f;
                    }
                }

                // Damped cosine bubble oscillator (fastCos: ~0.002% error per-sample)
                const float bubbleSample = voice.bubbleAmplitude
                    * fastCos(voice.bubblePhase * kTwoPi);
                voice.bubblePhase += bubbleFreq / static_cast<float>(currentSampleRate_);
                if (voice.bubblePhase >= 1.0f) voice.bubblePhase -= 1.0f;

                // Exponential decay — bubbleDecay [0,1] → fast..slow
                const float bubbleDecayRate = 0.001f + (1.0f - snap_.bubbleDecay) * 0.05f;
                voice.bubbleAmplitude = std::max(0.0f, voice.bubbleAmplitude - bubbleDecayRate);
                voice.bubbleAmplitude = flushDenormal(voice.bubbleAmplitude);

                // Bubble injects INTO pipe (disturbs the waveguide)
                excitation += bubbleSample * 0.5f;

                // ── Waveguide delay line ──────────────────────────────────────
                // Target delay length = sampleRate / (2 * effectiveFreq) for open pipe
                const float rawDelay = static_cast<float>(currentSampleRate_) / (2.0f * currentFreq);
                voice.targetDelayLength = juce::jlimit(2.0f, static_cast<float>(OozeVoice::kMaxDelay - 2), rawDelay);

                // Smooth delay length (avoid pitch glitches)
                voice.delayLength += 0.01f * (voice.targetDelayLength - voice.delayLength);
                voice.delayLength  = flushDenormal(voice.delayLength);

                // ── Read with allpass fractional interpolation ────────────────
                // (Read BEFORE writing excitation so the delay is one full period)
                const int   intDelay = static_cast<int>(voice.delayLength);
                const float frac     = voice.delayLength - static_cast<float>(intDelay);

                // Primary read position (one delay period back from write head)
                const int readPos1 = (voice.delayWritePos - intDelay + OozeVoice::kMaxDelay) % OozeVoice::kMaxDelay;
                const float delaySample = voice.delayLine[readPos1];

                // Allpass interpolation: coeff = (1-frac)/(1+frac)
                const float apCoeff  = (1.0f - frac) / (1.0f + frac + 1e-10f);
                const float apOut    = apCoeff * (delaySample - voice.allpassPrev) + voice.allpassPrevIn;
                voice.allpassPrev   = apOut;
                voice.allpassPrevIn = delaySample;

                // ── Reflection filter A (end type A) ─────────────────────────
                // Viscosity → LP cutoff: higher viscosity = more damping = lower cutoff
                // Map [0,1] → [0.05, 0.7] normalized cutoff
                const float reflCutA = juce::jlimit(0.05f, 0.7f, 0.7f - lfoViscosity * 0.6f);
                // 1-pole LP: matched-Z coefficient
                const float reflAlphaA = 1.0f - fastExp(-kTwoPi * reflCutA);
                voice.reflFilterStateA += reflAlphaA * (apOut - voice.reflFilterStateA);
                voice.reflFilterStateA  = flushDenormal(voice.reflFilterStateA);

                // End type: 0=Open (invert), 1=Closed (no invert), 2=Half (open on A, closed on B)
                float reflectedA;
                const int endType = snap_.endType;
                if (endType == 0 || endType == 2)
                    reflectedA = -voice.reflFilterStateA; // open end: inverting
                else
                    reflectedA = voice.reflFilterStateA;  // closed end: non-inverting

                // Geometry modifier: conical pipes have more even harmonics
                // Blend reflection phase relationship
                const float geomBlend = effGeometry;
                // Conical: partial positive reflection even at "open" end
                reflectedA = reflectedA * (1.0f - geomBlend * 0.3f);

                // Apply reflection gain — NEVER exceeds 0.999 (QDD mandate)
                const float reflGain = juce::jlimit(0.0f, 0.999f,
                    1.0f - effDamping * 0.3f);
                reflectedA *= reflGain;

                // Soft limiter on waveguide loop (QDD mandate — prevents runaway)
                reflectedA = fastTanh(reflectedA);
                reflectedA = flushDenormal(reflectedA);

                // Write excitation + reflected-A into delay at current write position.
                // FIX: previously wrote reflectedA to writePos+1 (one-ahead) which was
                // immediately overwritten by excitation on the next sample — reflection
                // never fed back into the loop. Now combined into a single write.
                voice.delayLine[voice.delayWritePos] = flushDenormal(excitation + reflectedA);

                // ── Read from second half of delay for end B ──────────────────
                const int   halfDelay = std::max(1, intDelay / 2);
                const int   readPos2  = (voice.delayWritePos - halfDelay + OozeVoice::kMaxDelay) % OozeVoice::kMaxDelay;
                const float delaySampleB = voice.delayLine[readPos2];

                // Reflection filter B (geometry-dependent cutoff)
                // Conical: brighter reflection at end B → higher cutoff
                const float reflCutB = juce::jlimit(0.05f, 0.8f,
                    reflCutA + effGeometry * 0.3f);
                const float reflAlphaB = 1.0f - fastExp(-kTwoPi * reflCutB);
                voice.reflFilterStateB += reflAlphaB * (delaySampleB - voice.reflFilterStateB);
                voice.reflFilterStateB  = flushDenormal(voice.reflFilterStateB);

                float reflectedB;
                if (endType == 1)
                    reflectedB = voice.reflFilterStateB;   // closed: non-inverting
                else
                    reflectedB = -voice.reflFilterStateB;  // open / half: inverting

                reflectedB *= reflGain;

                // QDD mandate: soft limiter on end-B reflection too
                reflectedB = fastTanh(reflectedB);
                reflectedB = flushDenormal(reflectedB);

                // Write reflected-B back into delay line at end-B's position (closes the loop)
                voice.delayLine[readPos2] = flushDenormal(voice.delayLine[readPos2] + reflectedB);

                // ── Advance write position ────────────────────────────────────
                voice.delayWritePos = (voice.delayWritePos + 1) % OozeVoice::kMaxDelay;

                // ── Output sample = delayed + direct jet component ────────────
                float sample = apOut + reflectedB + excitation * 0.05f;

                // ── Output filter — decimate coefficient refresh to every 16 samples ────
                float sampleL = voice.outputFilterL.processSample(sample);
                float sampleR = voice.outputFilterR.processSample(sample);
                sampleL = flushDenormal(sampleL);
                sampleR = flushDenormal(sampleR);

                // ── Drive saturation ──────────────────────────────────────────
                if (snap_.drive > 0.001f)
                {
                    const float driveGain = 1.0f + snap_.drive * 4.0f;
                    sampleL = fastTanh(sampleL * driveGain);
                    sampleR = fastTanh(sampleR * driveGain);
                }

                // ── Amplitude envelope ────────────────────────────────────────
                const float envLevel = voice.ampEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active    = false;
                    voice.releasing = false;
                    break;
                }

                // ── Velocity scaling (D001: vel² for pressure) ────────────────
                sampleL *= envLevel * (0.4f + velGain * 0.6f);
                sampleR *= envLevel * (0.4f + velGain * 0.6f);

                // ── Voice steal crossfade ──────────────────────────────────────
                if (voice.crossfadeGain < 1.0f)
                {
                    sampleL *= voice.crossfadeGain;
                    sampleR *= voice.crossfadeGain;
                    voice.crossfadeGain = std::min(1.0f, voice.crossfadeGain + voice.crossfadeRate);
                }

                voiceBufL_[static_cast<size_t>(s)] = sampleL;
                voiceBufR_[static_cast<size_t>(s)] = sampleR;
            } // end per-sample loop

            // ── Per-voice DC blocking ──────────────────────────────────────────
            voice.dcBlockerL.processBlock(voiceBufL_.data(), voiceBufL_.data(), numSamples);
            voice.dcBlockerR.processBlock(voiceBufR_.data(), voiceBufR_.data(), numSamples);

            // Sum into output
            for (int s = 0; s < numSamples; ++s)
            {
                outL[s] += voiceBufL_[static_cast<size_t>(s)];
                outR[s] += voiceBufR_[static_cast<size_t>(s)];
            }
        } // end voice loop

        // ── Step 10: Post-mix engine-level DC block ───────────────────────────
        engineDCBlockerL_.processBlock(outL, outL, numSamples);
        engineDCBlockerR_.processBlock(outR, outR, numSamples);

        // ── Step 11: Reverb send + return (FDN) ──────────────────────────────
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

        // ── Step 12: Cache output for coupling reads ──────────────────────────
        for (int s = 0; s < numSamples; ++s)
        {
            outputCacheL_[static_cast<size_t>(s)] = outL[s];
            outputCacheR_[static_cast<size_t>(s)] = outR[s];
        }

        // Update active voice count (atomic, read by UI thread)
        activeVoiceCount_.store(activeCount, std::memory_order_relaxed);

        // Feed silence gate
        analyzeForSilenceGate(buffer, numSamples);

        // Decay per-block coupling accumulators toward zero (do NOT hard-reset to 0 —
        // IIR smoother state must persist between blocks or coupling is silenced each block).
        // Exponential leak: ~63dB attenuation per 1000 blocks at a 0.999 factor.
        couplingAudioAccum_  *= 0.999f;
        couplingMorphAccum_  *= 0.999f;
        couplingFilterAccum_ *= 0.999f;
    }

    // =========================================================================
    // Coupling interface
    // =========================================================================
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
            // External audio = pressure perturbation in the pipe
            // QDD MANDATE: soft-clip and limit to 30% of loop energy
            for (int i = 0; i < numSamples; ++i)
            {
                float src = (sourceBuffer ? sourceBuffer[i] : 0.0f) * amount;
                src = fastTanh(src * 0.3f); // 30% max, soft-clipped
                couplingAudioAccum_ += 0.01f * (src - couplingAudioAccum_);
            }
        }
        else if (type == CouplingType::EnvToMorph)
        {
            // External envelope modulates reynolds number
            for (int i = 0; i < numSamples; ++i)
            {
                float src = (sourceBuffer ? sourceBuffer[i] : 0.0f) * amount * 0.25f;
                couplingMorphAccum_ += 0.01f * (src - couplingMorphAccum_);
            }
        }
        else
        {
            // AmpToFilter and all other types → filter cutoff modulation
            for (int i = 0; i < numSamples; ++i)
            {
                float src = (sourceBuffer ? sourceBuffer[i] : 0.0f) * amount * 0.25f;
                couplingFilterAccum_ += 0.01f * (src - couplingFilterAccum_);
            }
        }
    }

private:
    // =========================================================================
    // filterModeFromInt
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
    // processMidi — note on/off, pitch bend, CCs, aftertouch
    // =========================================================================
    void processMidi(juce::MidiBuffer& midi) noexcept
    {
        for (const auto& meta : midi)
        {
            const auto msg = meta.getMessage();

            if (msg.isNoteOn())
            {
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity());
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
            else if (msg.isControllerOfType(1)) // CC1: mod wheel → reynolds
            {
                modWheel_ = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }
            else if (msg.isControllerOfType(2)) // CC2: breath → M4 Breath macro
            {
                breathCC_ = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }
            else if (msg.isControllerOfType(11)) // CC11: expression → pressure
            {
                expression_ = static_cast<float>(msg.getControllerValue()) / 127.0f;
            }
            else if (msg.isChannelPressure())
            {
                aftertouchGlobal_ = static_cast<float>(msg.getChannelPressureValue()) / 127.0f;
            }
            else if (msg.isAftertouch())
            {
                aftertouchGlobal_ = static_cast<float>(msg.getAfterTouchValue()) / 127.0f;
            }
        }
    }

    // =========================================================================
    // noteOn — allocate/steal voice, initialise waveguide, trigger envelope
    // =========================================================================
    void noteOn(int note, float vel) noexcept
    {
        // Re-trigger existing voice on same note
        for (auto& v : voices_)
        {
            if (v.active && v.midiNote == note)
            {
                const float tuneSemitones = static_cast<float>(snap_.tune) + snap_.fine / 100.0f;
                v.targetFrequency = midiToFreqTune(note, tuneSemitones) * pitchBend_;
                v.glide.setTarget(v.targetFrequency);
                v.velocity  = vel;
                v.releasing = false;
                v.ampEnv.noteOn();
                wakeSilenceGate();
                return;
            }
        }

        // Find a free voice
        OozeVoice* target = nullptr;
        for (auto& v : voices_)
        {
            if (!v.active) { target = &v; break; }
        }

        // Steal: prefer releasing voices, then steal the first
        if (target == nullptr)
        {
            for (auto& v : voices_)
            {
                if (v.releasing) { target = &v; break; }
            }
        }
        if (target == nullptr)
            target = &voices_[0];

        const bool wasActive = target->active;

        // Reset voice state (preserve noise seed for continuity)
        const uint32_t savedNoise = target->noiseState;
        target->reset(currentSampleRate_);
        target->noiseState = savedNoise;

        // Crossfade on steal to prevent clicks
        target->crossfadeGain = wasActive ? 0.0f : 1.0f;
        target->crossfadeRate = wasActive
            ? 1.0f / (0.005f * static_cast<float>(currentSampleRate_))
            : 0.0f;

        // Set note data
        target->midiNote  = note;
        target->velocity  = vel;

        const float tuneSemitones = static_cast<float>(snap_.tune) + snap_.fine / 100.0f;
        target->targetFrequency = midiToFreqTune(note, tuneSemitones) * pitchBend_;
        target->frequency       = target->targetFrequency;

        // Delay length for this pitch
        target->delayLength       = static_cast<float>(currentSampleRate_) / (2.0f * target->frequency);
        target->targetDelayLength = target->delayLength;
        target->delayLength       = juce::jlimit(2.0f, static_cast<float>(OozeVoice::kMaxDelay - 2),
                                                 target->delayLength);

        // Glide: snap on first note, glide on steal/retrigger
        const float glideTime = snap_.glide * snap_.glide * 2.0f;
        target->glide.setTime(glideTime, static_cast<float>(currentSampleRate_));
        if (!wasActive)
            target->glide.snapTo(target->targetFrequency);
        else
            target->glide.setTarget(target->targetFrequency);

        // Seed waveguide with a small impulse (gentle excitation at startup)
        target->delayLine[0] = 0.05f;

        // Trigger amplitude envelope
        target->ampEnv.prepare(static_cast<float>(currentSampleRate_));
        target->ampEnv.setADSR(snap_.attack, snap_.decay, snap_.sustain, snap_.release);
        target->ampEnv.noteOn();

        // LFO setup
        target->lfo.setRate(snap_.lfoRate, static_cast<float>(currentSampleRate_));
        target->lfo.setShape(snap_.lfoShape);

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
    std::array<OozeVoice, kMaxVoices> voices_;
    OozeParamSnapshot snap_;
    OozeFDN           fdn_;

    // Engine-level DC blockers (post-mix)
    xoceanus::DCBlocker engineDCBlockerL_;
    xoceanus::DCBlocker engineDCBlockerR_;

    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    double currentSampleRate_ = 0.0;
    int    maxBlockSize_      = 512;

    // MIDI expression state
    float pitchBend_        = 1.0f; // frequency ratio (1.0 = no bend)
    float modWheel_         = 0.0f; // CC1
    float breathCC_         = 0.0f; // CC2 → M4 Breath
    float expression_       = 0.0f; // CC11
    float aftertouchGlobal_ = 0.0f;

    // Scratch buffers (allocated in prepare, no audio-thread allocation)
    std::vector<float> voiceBufL_;
    std::vector<float> voiceBufR_;

    // Coupling output cache (for getSampleForCoupling reads)
    std::vector<float> outputCacheL_;
    std::vector<float> outputCacheR_;

    // Coupling input accumulators (consumed + cleared each block)
    float couplingAudioAccum_  = 0.0f; // AudioToBuffer  → pressure perturbation
    float couplingMorphAccum_  = 0.0f; // EnvToMorph     → reynolds offset
    float couplingFilterAccum_ = 0.0f; // All other types → filter cutoff mod
};

} // namespace xoceanus
