#pragma once

//==============================================================================
// OperaEngine.h — opera::OperaEngine
//
// XOpera Engine #45 — XO_OX Designs
// The keystone composition file: wires OperaPartialBank, KuramotoField,
// OperaConductor, OperaBreathEngine, and ReactiveStage into a complete
// SynthEngine implementation with 8-voice polyphony, 45 parameters,
// 4 macros, modulation routing, and coupling interface.
//
// Architecture:
//   Per voice: PartialBank + KuramotoField + BreathEngine + 2x Envelope
//   Global: Conductor, ReactiveStage, 2x LFO, Vibrato LFO
//
// References:
//   - Kuramoto (1975) — coupled oscillator field
//   - Peterson & Barney (1952) — formant data
//   - Acebron et al. (2005) — mean-field O(2N) reduction
//   - Ciani — spatial Kuramoto panning
//   - Tomita — reactive reverb concept
//   - Vangelis — emotional memory, response speed
//   - Schulze — phase-transition hysteresis
//
// All code inline in this header. No allocation on audio thread.
// #pragma once. namespace opera.
//==============================================================================

#include <juce_audio_processors/juce_audio_processors.h>
#include "OperaConstants.h"
#include "OperaPartialBank.h"
#include "KuramotoField.h"
#include "OperaConductor.h"
#include "OperaBreathEngine.h"
#include "ReactiveStage.h"
#include "../../DSP/FastMath.h"

#include <cmath>
#include <cstring>
#include <algorithm>
#include <atomic>

namespace opera {

//==============================================================================
// Constants (shared ones from OperaConstants.h; engine-specific below)
//==============================================================================

static constexpr int   kNumCouplingTypes = 6;

//==============================================================================
// Modulation Destination Enum
//==============================================================================

enum class ModDest : int
{
    Drama        = 0,
    Voice        = 1,
    Breath       = 2,
    Effort       = 3,
    Tilt         = 4,
    FilterCutoff = 5,
    VibDepth     = 6,
    ResSens      = 7
};

//==============================================================================
// ModOffsets — accumulated per-sample modulation offsets from all sources
//==============================================================================

struct ModOffsets
{
    float drama        = 0.0f;
    float voice        = 0.0f;
    float breath       = 0.0f;
    float effort       = 0.0f;
    float tilt         = 0.0f;
    float filterCutoff = 0.0f;   // in Hz
    float vibDepth     = 0.0f;
    float resSens      = 0.0f;

    void clear() noexcept
    {
        drama = voice = breath = effort = tilt = filterCutoff = vibDepth = resSens = 0.0f;
    }
};

//==============================================================================
// Simple ADSR Envelope (self-contained, mirrors XOlokun FilterEnvelope)
// Linear attack, exponential decay/release. Denormal-safe.
//==============================================================================

struct OperaEnvelope
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    Stage stage = Stage::Idle;
    float level = 0.0f;

    void prepare (float sampleRate) noexcept
    {
        sr = sampleRate;
        recalcCoeffs();
    }

    void setADSR (float attackSec, float decaySec, float sustainLevel, float releaseSec) noexcept
    {
        atkTime = std::max (attackSec, 0.0001f);
        decTime = std::max (decaySec, 0.001f);
        susLvl  = std::clamp (sustainLevel, 0.0f, 1.0f);
        relTime = std::max (releaseSec, 0.001f);
        recalcCoeffs();
    }

    void trigger() noexcept
    {
        stage = Stage::Attack;
    }

    void triggerHard() noexcept
    {
        level = 0.0f;
        stage = Stage::Attack;
    }

    void release() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    void kill() noexcept
    {
        level = 0.0f;
        stage = Stage::Idle;
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;

            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                return level;

            case Stage::Decay:
                level -= (level - susLvl) * decayCoeff;
                level = flushDen (level);
                if (level <= susLvl + 0.001f) { level = susLvl; stage = Stage::Sustain; }
                return level;

            case Stage::Sustain:
                return level;

            case Stage::Release:
                level -= level * releaseCoeff;
                level = flushDen (level);
                if (level < 1e-6f) { level = 0.0f; stage = Stage::Idle; }
                return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    float getLevel() const noexcept { return level; }
    Stage getStage() const noexcept { return stage; }

private:
    float sr = 48000.0f;
    float atkTime = 0.005f, decTime = 0.3f, susLvl = 0.0f, relTime = 0.5f;
    float attackRate = 0.0f, decayCoeff = 0.0f, releaseCoeff = 0.0f;

    void recalcCoeffs() noexcept
    {
        if (sr <= 0.0f) return;
        attackRate   = 1.0f / (sr * atkTime);
        decayCoeff   = 1.0f - std::exp (-4.6f / (sr * decTime));
        releaseCoeff = 1.0f - std::exp (-4.6f / (sr * relTime));
    }

    static float flushDen (float x) noexcept
    {
        uint32_t bits;
        std::memcpy (&bits, &x, sizeof (bits));
        if ((bits & 0x7F800000) == 0 && (bits & 0x007FFFFF) != 0) return 0.0f;
        return x;
    }
};

//==============================================================================
// Simple LFO (self-contained, mirrors XOlokun StandardLFO)
// Sine wave only for this engine's use. D005 compliant floor.
//==============================================================================

struct OperaLFO
{
    float phase    = 0.0f;
    float phaseInc = 0.0f;

    void setRate (float hz, float sampleRate) noexcept
    {
        phaseInc = hz / sampleRate;
    }

    float process() noexcept
    {
        float out = FastMath::fastSin (phase * kTwoPi);
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
        return out;
    }

    void reset() noexcept { phase = 0.0f; }
};

//==============================================================================
// Simple 2-pole SVF (State Variable Filter)
// Lowpass mode. Cutoff in Hz, Q from 0.5 to 10.
//==============================================================================

struct OperaSVF
{
    float ic1eq = 0.0f;
    float ic2eq = 0.0f;

    // Cached SVF coefficients — recomputed only when cutoff/Q change by > 0.001.
    // Eliminates per-sample FastMath::fastTan calls (critical for iOS CPU budget).
    float svfG_       = 0.0f;
    float svfK_       = 0.0f;
    float svfA1_      = 0.0f;
    float svfA2_      = 0.0f;
    float svfA3_      = 0.0f;
    float lastCutoff_ = -1.0f;
    float lastRes_    = -1.0f;

    void reset() noexcept { ic1eq = ic2eq = 0.0f; }

    /// Process one sample, return lowpass output.
    /// SRO (2026-03-21): std::tan → FastMath::fastTan — accurate to 0.03% below
    /// 0.25×sampleRate. Saves ~16 std::tan calls/sample (2 SVFs × 8 voices).
    /// SRO (2026-03-22): block-rate coefficient cache — recompute only when cutoff
    /// or Q drift by > 0.001. Eliminates fastTan on iOS hot path.
    float process (float input, float cutoffHz, float Q, float sampleRate) noexcept
    {
        // Clamp cutoff to prevent instability
        cutoffHz = std::clamp (cutoffHz, 20.0f, sampleRate * 0.499f);
        Q = std::max (Q, 0.5f);

        // Recompute g/k and derived coefficients only when parameters change.
        if (std::abs (cutoffHz - lastCutoff_) > 0.001f || std::abs (Q - lastRes_) > 0.001f)
        {
            svfG_       = FastMath::fastTan (kPi * cutoffHz / sampleRate);
            svfK_       = 1.0f / Q;
            svfA1_      = 1.0f / (1.0f + svfG_ * (svfG_ + svfK_));
            svfA2_      = svfG_ * svfA1_;
            svfA3_      = svfG_ * svfA2_;
            lastCutoff_ = cutoffHz;
            lastRes_    = Q;
        }

        const float a1 = svfA1_;
        const float a2 = svfA2_;
        const float a3 = svfA3_;

        float v3 = input - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;

        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        // Flush denormals
        uint32_t b1, b2;
        std::memcpy (&b1, &ic1eq, sizeof (b1));
        std::memcpy (&b2, &ic2eq, sizeof (b2));
        if ((b1 & 0x7F800000) == 0 && (b1 & 0x007FFFFF) != 0) ic1eq = 0.0f;
        if ((b2 & 0x7F800000) == 0 && (b2 & 0x007FFFFF) != 0) ic2eq = 0.0f;

        return v2;  // lowpass output
    }
};

//==============================================================================
// OperaVoice — Per-voice state
//==============================================================================

struct OperaVoice
{
    enum class State { Idle, Attack, Sustain, Release };

    State  state       = State::Idle;
    int    note        = -1;
    float  velocity    = 0.0f;
    uint64_t startTime = 0;

    // DSP modules
    OperaPartialBank partialBank;
    KuramotoField    kuramotoField;
    OperaBreathEngine breathEngine;

    // Envelopes
    OperaEnvelope ampEnv;
    OperaEnvelope filterEnv;

    // Filter
    OperaSVF filterL;
    OperaSVF filterR;

    // Portamento glide state
    float targetFreq  = 440.0f;   // frequency the voice is gliding TO
    float currentFreq = 440.0f;   // frequency currently used for rendering (glides toward targetFreq)

    // Vibrato phase (per-voice for detuned vibrato in unison)
    float vibratoPhase = 0.0f;

    // Mono coupling tap buffer (post-Kuramoto, pre-FX)
    float couplingTap[kMaxBlockSize] = {};

    // --- Pan cache (SRO 2026-03-24) -----------------------------------------
    // Per-partial constant-power pan coefficients, cached at Kuramoto block rate.
    // Panning derives from the Kuramoto order parameter (psi, r) which only
    // updates every kKuraBlock = 8 samples. Computing fastCos + fastSin per
    // partial per sample wasted ~512 trig calls/sample at 32 partials × 8 voices.
    // Cache is invalidated (panCacheValid = false) whenever the Kuramoto field
    // updates, and rebuilt immediately before the next partial render loop.
    // Unison layers are indexed in the second dimension (0..3).
    // Savings: ~448 trig calls/sample eliminated (7/8 of 512) → ~30-40% CPU.
    float cachedPanL[kMaxPartials][4] = {};  // [partialIdx][unisonLayer]
    float cachedPanR[kMaxPartials][4] = {};  // [partialIdx][unisonLayer]
    bool  panCacheValid = false;

    inline void prepare (float sampleRate) noexcept
    {
        partialBank.prepare (sampleRate);
        kuramotoField.prepare (static_cast<double> (sampleRate), kMaxPartials);
        breathEngine.prepare (static_cast<double> (sampleRate));
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        filterL.reset();
        filterR.reset();
        vibratoPhase = 0.0f;
    }

    inline void resetFull() noexcept
    {
        state = State::Idle;
        note  = -1;
        velocity = 0.0f;
        targetFreq  = 440.0f;
        currentFreq = 440.0f;
        partialBank.reset();
        kuramotoField.reset();
        breathEngine.reset();
        ampEnv.kill();
        filterEnv.kill();
        filterL.reset();
        filterR.reset();
        vibratoPhase = 0.0f;
        std::memset (couplingTap, 0, sizeof (couplingTap));
        panCacheValid = false;
    }
};

//==============================================================================
// ParamSnapshot — All 45 parameters cached once per block.
// Never call param->load() per-sample.
//==============================================================================

struct ParamSnapshot
{
    // Core synthesis (18)
    float drama          = 0.35f;
    float voice          = 0.5f;
    int   vowelA         = 0;
    int   vowelB         = 3;
    float breath         = 0.2f;
    float effort         = 0.5f;
    int   partials       = 32;
    float detune         = 0.1f;
    float tilt           = 0.0f;
    int   fundamental    = 0;
    float resSens        = 0.5f;
    float portamento     = 0.0f;
    int   unison         = 1;
    float width          = 0.5f;
    float vibRate        = 5.5f;
    float vibDepth       = 0.25f;
    float stage          = 0.3f;
    float responseSpeed  = 0.5f;

    // Filter (6)
    float filterCutoff   = 8000.0f;
    float filterRes      = 0.0f;
    float filterEnvAmt   = 0.3f;
    float filterA        = 0.01f;
    float filterD        = 0.3f;
    float filterS        = 0.5f;
    float filterR        = 0.4f;

    // Amp envelope (4)
    float ampA           = 0.05f;
    float ampD           = 0.2f;
    float ampS           = 0.8f;
    float ampR           = 0.6f;

    // LFOs (6)
    float lfo1Rate       = 0.1f;
    float lfo1Depth      = 0.3f;
    int   lfo1Dest       = 1;
    float lfo2Rate       = 3.0f;
    float lfo2Depth      = 0.0f;
    int   lfo2Dest       = 0;

    // Conductor (4)
    int   arcMode        = 0;
    int   arcShape       = 1;
    float arcTime        = 8.0f;
    float arcPeak        = 0.8f;

    // Expression routing (6)
    int   modWheelDest   = 0;
    float modWheelAmt    = 0.5f;
    int   atDest         = 3;
    float atAmt          = 0.5f;
    float velToFilter    = 0.4f;
    float velToEffort    = 0.3f;
};

//==============================================================================
// OperaEngine — The complete engine composition.
//==============================================================================

class OperaEngine
{
public:
    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId() const { return "Opera"; }

    juce::Colour getAccentColour() const
    {
        return juce::Colour (0xFFD4AF37);  // Aria Gold
    }

    int getMaxVoices() const { return kMaxVoices; }

    int getActiveVoiceCount() const
    {
        return activeVoiceCount_.load (std::memory_order_relaxed);
    }

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize)
    {
        sr_        = static_cast<float> (sampleRate);
        invSr_     = 1.0f / sr_;
        blockSize_ = std::min (maxBlockSize, kMaxBlockSize);

        for (auto& v : voices_)
        {
            v.prepare (sr_);
            v.resetFull();
        }

        lfo1_.reset();
        lfo2_.reset();
        vibratoLFO_.reset();
        conductor_.prepare (sampleRate);
        reactiveStage_.prepare (sampleRate, blockSize_);

        modWheelValue_  = 0.0f;
        aftertouchValue_ = 0.0f;
        pitchBendSemitones_ = 0.0f;
        voiceCounter_ = 0;

        // Clear coupling buffers
        std::memset (couplingCacheL_, 0, sizeof (couplingCacheL_));
        std::memset (couplingCacheR_, 0, sizeof (couplingCacheR_));
        std::memset (couplingFMBuffer_, 0, sizeof (couplingFMBuffer_));
        std::memset (couplingRingBuffer_, 0, sizeof (couplingRingBuffer_));
        std::memset (couplingFilterBuffer_, 0, sizeof (couplingFilterBuffer_));
        std::memset (couplingMorphBuffer_, 0, sizeof (couplingMorphBuffer_));
        std::memset (couplingKBuffer_, 0, sizeof (couplingKBuffer_));
        std::memset (couplingPhaseBuffer_, 0, sizeof (couplingPhaseBuffer_));
    }

    void releaseResources()
    {
        // Nothing to free — all buffers are fixed-size
    }

    void reset()
    {
        for (auto& v : voices_)
            v.resetFull();

        lfo1_.reset();
        lfo2_.reset();
        vibratoLFO_.reset();
        conductor_.reset();
        reactiveStage_.reset();
        modWheelValue_ = 0.0f;
        aftertouchValue_ = 0.0f;
        pitchBendSemitones_ = 0.0f;
    }

    //==========================================================================
    // Parameter Layout — 45 parameters with opera_ prefix
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return { params.begin(), params.end() };
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using FloatParam = juce::AudioParameterFloat;
        using IntParam   = juce::AudioParameterInt;
        using NR = juce::NormalisableRange<float>;

        // --- Core Synthesis (18) ---
        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_drama", 1), "Drama",
            NR (0.0f, 1.0f, 0.001f), 0.35f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_voice", 1), "Voice",
            NR (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_vowelA", 1), "Vowel A", 0, 5, 0));

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_vowelB", 1), "Vowel B", 0, 5, 3));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_breath", 1), "Breath",
            NR (0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_effort", 1), "Effort",
            NR (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_partials", 1), "Partials", 4, 48, 32));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_detune", 1), "Detune",
            NR (0.0f, 1.0f, 0.001f), 0.1f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_tilt", 1), "Spectral Tilt",
            NR (-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_fundamental", 1), "Fundamental", -36, 36, 0));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_resSens", 1), "Resonance Sens",
            NR (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_portamento", 1), "Portamento",
            NR (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_unison", 1), "Unison", 1, 4, 1));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_width", 1), "Stereo Width",
            NR (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_vibRate", 1), "Vibrato Rate",
            NR (0.01f, 20.0f, 0.01f), 5.5f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_vibDepth", 1), "Vibrato Depth",
            NR (0.0f, 1.0f, 0.001f), 0.25f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_stage", 1), "Stage",
            NR (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_responseSpeed", 1), "Response Speed",
            NR (0.0f, 1.0f, 0.001f), 0.5f));

        // --- Filter (7) ---
        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_filterCutoff", 1), "Filter Cutoff",
            NR (20.0f, 20000.0f, 1.0f, 0.3f), 8000.0f));  // skew 0.3 for log feel

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_filterRes", 1), "Filter Resonance",
            NR (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_filterEnvAmt", 1), "Filter Env Amt",
            NR (-1.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_filterA", 1), "Filter Attack",
            NR (0.0f, 1.0f, 0.001f), 0.01f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_filterD", 1), "Filter Decay",
            NR (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_filterS", 1), "Filter Sustain",
            NR (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_filterR", 1), "Filter Release",
            NR (0.0f, 1.0f, 0.001f), 0.4f));

        // --- Amp Envelope (4) ---
        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_ampA", 1), "Amp Attack",
            NR (0.0f, 1.0f, 0.001f), 0.05f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_ampD", 1), "Amp Decay",
            NR (0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_ampS", 1), "Amp Sustain",
            NR (0.0f, 1.0f, 0.001f), 0.8f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_ampR", 1), "Amp Release",
            NR (0.0f, 2.0f, 0.001f), 0.6f));  // 2.0 → ~80s via cubic scale; supports long-tail Flux presets

        // --- LFOs (6) ---
        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_lfo1Rate", 1), "LFO1 Rate",
            NR (0.01f, 30.0f, 0.01f), 0.1f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_lfo1Depth", 1), "LFO1 Depth",
            NR (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_lfo1Dest", 1), "LFO1 Dest", 0, 7, 1));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_lfo2Rate", 1), "LFO2 Rate",
            NR (0.01f, 30.0f, 0.01f), 3.0f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_lfo2Depth", 1), "LFO2 Depth",
            NR (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_lfo2Dest", 1), "LFO2 Dest", 0, 7, 0));

        // --- Conductor (4) ---
        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_arcMode", 1), "Arc Mode", 0, 2, 1));  // default=1: Conductor on

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_arcShape", 1), "Arc Shape", 0, 3, 1));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_arcTime", 1), "Arc Time",
            NR (0.5f, 3600.0f, 0.1f), 8.0f));  // extended to 3600s for Schulze-scale arcs

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_arcPeak", 1), "Arc Peak",
            NR (0.0f, 1.0f, 0.001f), 0.8f));

        // --- Expression Routing (6) ---
        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_modWheelDest", 1), "MW Dest", 0, 7, 0));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_modWheelAmt", 1), "MW Amount",
            NR (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<IntParam> (
            juce::ParameterID ("opera_atDest", 1), "AT Dest", 0, 7, 3));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_atAmt", 1), "AT Amount",
            NR (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_velToFilter", 1), "Vel->Filter",
            NR (0.0f, 1.0f, 0.001f), 0.4f));

        params.push_back (std::make_unique<FloatParam> (
            juce::ParameterID ("opera_velToEffort", 1), "Vel->Effort",
            NR (0.0f, 1.0f, 0.001f), 0.3f));
    }

    //==========================================================================
    // Attach raw parameter pointers from the shared APVTS
    //==========================================================================

    void attachParameters (juce::AudioProcessorValueTreeState& apvts)
    {
        // Core synthesis
        p_drama         = apvts.getRawParameterValue ("opera_drama");
        p_voice         = apvts.getRawParameterValue ("opera_voice");
        p_vowelA        = apvts.getRawParameterValue ("opera_vowelA");
        p_vowelB        = apvts.getRawParameterValue ("opera_vowelB");
        p_breath        = apvts.getRawParameterValue ("opera_breath");
        p_effort        = apvts.getRawParameterValue ("opera_effort");
        p_partials      = apvts.getRawParameterValue ("opera_partials");
        p_detune        = apvts.getRawParameterValue ("opera_detune");
        p_tilt          = apvts.getRawParameterValue ("opera_tilt");
        p_fundamental   = apvts.getRawParameterValue ("opera_fundamental");
        p_resSens       = apvts.getRawParameterValue ("opera_resSens");
        p_portamento    = apvts.getRawParameterValue ("opera_portamento");
        p_unison        = apvts.getRawParameterValue ("opera_unison");
        p_width         = apvts.getRawParameterValue ("opera_width");
        p_vibRate       = apvts.getRawParameterValue ("opera_vibRate");
        p_vibDepth      = apvts.getRawParameterValue ("opera_vibDepth");
        p_stage         = apvts.getRawParameterValue ("opera_stage");
        p_responseSpeed = apvts.getRawParameterValue ("opera_responseSpeed");

        // Filter
        p_filterCutoff  = apvts.getRawParameterValue ("opera_filterCutoff");
        p_filterRes     = apvts.getRawParameterValue ("opera_filterRes");
        p_filterEnvAmt  = apvts.getRawParameterValue ("opera_filterEnvAmt");
        p_filterA       = apvts.getRawParameterValue ("opera_filterA");
        p_filterD       = apvts.getRawParameterValue ("opera_filterD");
        p_filterS       = apvts.getRawParameterValue ("opera_filterS");
        p_filterR       = apvts.getRawParameterValue ("opera_filterR");

        // Amp envelope
        p_ampA          = apvts.getRawParameterValue ("opera_ampA");
        p_ampD          = apvts.getRawParameterValue ("opera_ampD");
        p_ampS          = apvts.getRawParameterValue ("opera_ampS");
        p_ampR          = apvts.getRawParameterValue ("opera_ampR");

        // LFOs
        p_lfo1Rate      = apvts.getRawParameterValue ("opera_lfo1Rate");
        p_lfo1Depth     = apvts.getRawParameterValue ("opera_lfo1Depth");
        p_lfo1Dest      = apvts.getRawParameterValue ("opera_lfo1Dest");
        p_lfo2Rate      = apvts.getRawParameterValue ("opera_lfo2Rate");
        p_lfo2Depth     = apvts.getRawParameterValue ("opera_lfo2Depth");
        p_lfo2Dest      = apvts.getRawParameterValue ("opera_lfo2Dest");

        // Conductor
        p_arcMode       = apvts.getRawParameterValue ("opera_arcMode");
        p_arcShape      = apvts.getRawParameterValue ("opera_arcShape");
        p_arcTime       = apvts.getRawParameterValue ("opera_arcTime");
        p_arcPeak       = apvts.getRawParameterValue ("opera_arcPeak");

        // Expression routing
        p_modWheelDest  = apvts.getRawParameterValue ("opera_modWheelDest");
        p_modWheelAmt   = apvts.getRawParameterValue ("opera_modWheelAmt");
        p_atDest        = apvts.getRawParameterValue ("opera_atDest");
        p_atAmt         = apvts.getRawParameterValue ("opera_atAmt");
        p_velToFilter   = apvts.getRawParameterValue ("opera_velToFilter");
        p_velToEffort   = apvts.getRawParameterValue ("opera_velToEffort");
    }

    //==========================================================================
    // renderBlock — The main audio rendering entry point.
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples)
    {
        juce::ScopedNoDenormals noDenormals;

        numSamples = std::min (numSamples, blockSize_);
        if (numSamples <= 0) return;

        // ------------------------------------------------------------------
        // STEP 1: Process MIDI events
        // ------------------------------------------------------------------
        for (const auto metadata : midi)
        {
            auto msg = metadata.getMessage();

            if (msg.isNoteOn())
            {
                handleNoteOn (msg.getNoteNumber(),
                              static_cast<float> (msg.getVelocity()) / 127.0f);
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff (msg.getNoteNumber());
            }
            else if (msg.isControllerOfType (1))   // Mod Wheel
            {
                modWheelValue_ = static_cast<float> (msg.getControllerValue()) / 127.0f;
            }
            else if (msg.isControllerOfType (20))  // Conductor trigger
            {
                if (msg.getControllerValue() > 63)
                    conductor_.trigger();
                else
                    conductor_.stop();
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue_ = static_cast<float> (msg.getChannelPressureValue()) / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                // Pitch bend: +/- 2 semitones (standard)
                int bendValue = msg.getPitchWheelValue();  // 0..16383, center = 8192
                pitchBendSemitones_ = (static_cast<float> (bendValue) - 8192.0f) / 8192.0f * 2.0f;
            }
        }

        // ------------------------------------------------------------------
        // STEP 2: ParamSnapshot — read all params ONCE per block
        // ------------------------------------------------------------------
        cacheParamSnapshot();

        // ------------------------------------------------------------------
        // STEP 3: Configure LFOs for this block
        // ------------------------------------------------------------------
        lfo1_.setRate (snap_.lfo1Rate, sr_);
        lfo2_.setRate (snap_.lfo2Rate, sr_);
        vibratoLFO_.setRate (snap_.vibRate, sr_);

        // ------------------------------------------------------------------
        // STEP 4: Configure conductor for this block
        // ------------------------------------------------------------------
        conductor_.setArcMode (snap_.arcMode);
        conductor_.setArcShape (snap_.arcShape);
        conductor_.setArcTime (snap_.arcTime);
        conductor_.setArcPeak (snap_.arcPeak);

        // ------------------------------------------------------------------
        // STEP 5: Configure envelopes for all active voices
        // ------------------------------------------------------------------
        float ampAtkSec = 0.0001f + snap_.ampA * snap_.ampA * snap_.ampA * 5.0f;
        float ampDecSec = 0.001f  + snap_.ampD * snap_.ampD * snap_.ampD * 10.0f;
        float ampRelSec = 0.001f  + snap_.ampR * snap_.ampR * snap_.ampR * 10.0f;

        float fltAtkSec = 0.0001f + snap_.filterA * snap_.filterA * snap_.filterA * 5.0f;
        float fltDecSec = 0.001f  + snap_.filterD * snap_.filterD * snap_.filterD * 10.0f;
        float fltRelSec = 0.001f  + snap_.filterR * snap_.filterR * snap_.filterR * 10.0f;

        for (auto& v : voices_)
        {
            if (v.state != OperaVoice::State::Idle)
            {
                v.ampEnv.setADSR (ampAtkSec, ampDecSec, snap_.ampS, ampRelSec);
                v.filterEnv.setADSR (fltAtkSec, fltDecSec, snap_.filterS, fltRelSec);
            }
        }

        // ------------------------------------------------------------------
        // STEP 6: Per-sample rendering
        // ------------------------------------------------------------------
        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

        // Clear output buffer
        buffer.clear (0, numSamples);

        // ------------------------------------------------------------------
        // STEP 6 PRE-BLOCK: Update formant weights & Nyquist gains per voice
        // (block-rate — these don't depend on per-sample state)
        // ------------------------------------------------------------------
        for (int v = 0; v < kMaxVoices; ++v)
        {
            auto& voice = voices_[v];
            if (voice.state == OperaVoice::State::Idle) continue;

            float f0Block = voice.currentFreq;
            voice.partialBank.updateFormants (f0Block, snap_.vowelA, snap_.vowelB, snap_.voice);
            voice.partialBank.computeNyquistGains (f0Block);
        }

        for (int s = 0; s < numSamples; ++s)
        {
            // 6a. Accumulate modulation offsets for this sample
            ModOffsets mods;
            mods.clear();

            float lfo1Val = lfo1_.process() * snap_.lfo1Depth;
            float lfo2Val = lfo2_.process() * snap_.lfo2Depth;

            applyModulation (mods, static_cast<ModDest> (snap_.lfo1Dest), lfo1Val);
            applyModulation (mods, static_cast<ModDest> (snap_.lfo2Dest), lfo2Val);
            applyModulation (mods, static_cast<ModDest> (snap_.modWheelDest),
                             modWheelValue_ * snap_.modWheelAmt);
            applyModulation (mods, static_cast<ModDest> (snap_.atDest),
                             aftertouchValue_ * snap_.atAmt);

            // Apply coupling K offset (from KnotTopology coupling)
            float couplingKOff = (s < blockSize_) ? couplingKBuffer_[s] : 0.0f;

            // 6b. Compute effective parameters with modulation
            float drama_eff   = std::clamp (snap_.drama + mods.drama, 0.0f, 1.0f);
            float voice_eff   = std::clamp (snap_.voice + mods.voice, 0.0f, 1.0f);
            float breath_eff  = std::clamp (snap_.breath + mods.breath, 0.0f, 1.0f);
            float effort_eff  = std::clamp (snap_.effort + mods.effort, 0.0f, 1.0f);
            float tilt_eff    = std::clamp (snap_.tilt + mods.tilt, -1.0f, 1.0f);
            float cutoff_eff  = std::clamp (snap_.filterCutoff + mods.filterCutoff, 20.0f, 20000.0f);
            float vibDepth_eff = std::clamp (snap_.vibDepth + mods.vibDepth, 0.0f, 1.0f);
            float resSens_eff = std::clamp (snap_.resSens + mods.resSens, 0.0f, 1.0f);

            // 6c. Conductor K contribution
            float manualK = drama_eff * kKmax;
            float effectiveK = conductor_.computeEffectiveK (manualK);

            // Add coupling K offset
            if (couplingKOff != 0.0f)
                effectiveK = std::clamp (effectiveK + couplingKOff * kKmax, 0.0f, kKmax);

            // 6d. Vibrato — pitch multiplier for all voices
            float vibSample = vibratoLFO_.process() * vibDepth_eff;
            // vibDepth=1.0 -> 100 cents max deviation
            float vibPitchMult = std::pow (2.0f, vibSample * (100.0f / 1200.0f));

            // 6e. Coupling FM and Ring mod values for this sample
            float couplingFM   = (s < blockSize_) ? couplingFMBuffer_[s]   : 0.0f;
            float couplingRing = (s < blockSize_) ? couplingRingBuffer_[s] : 0.0f;
            float couplingFilt = (s < blockSize_) ? couplingFilterBuffer_[s] : 0.0f;
            float couplingMorph = (s < blockSize_) ? couplingMorphBuffer_[s] : 0.0f;
            float couplingPhase = (s < blockSize_) ? couplingPhaseBuffer_[s] : 0.0f;

            // 6f. Apply coupling morph to voice
            if (couplingMorph != 0.0f)
                voice_eff = std::clamp (voice_eff + couplingMorph, 0.0f, 1.0f);

            // 6g. Render each active voice
            float sampleL = 0.0f;
            float sampleR = 0.0f;
            [[maybe_unused]] float monoTap = 0.0f;

            for (int v = 0; v < kMaxVoices; ++v)
            {
                auto& voice = voices_[v];
                if (voice.state == OperaVoice::State::Idle) continue;

                // --- Voice-specific computations ---

                // Velocity mappings (D001: velocity shapes timbre, not just amplitude)
                float vel = voice.velocity;
                float velFilterOff = vel * snap_.velToFilter * 8000.0f;
                float velEffortOff = vel * snap_.velToEffort * 0.5f;
                float velKOff      = vel * 0.15f;
                float velTiltOff   = vel * 0.1f;

                float voiceEffort = std::clamp (effort_eff + velEffortOff, 0.0f, 1.0f);
                float voiceTilt   = std::clamp (tilt_eff + velTiltOff, -1.0f, 1.0f);

                // --- Portamento glide ---
                // Update targetFreq each sample for pitch bend tracking
                voice.targetFreq = midiNoteToFreq (voice.note + snap_.fundamental,
                                                    pitchBendSemitones_);

                if (snap_.portamento > 0.0f)
                {
                    // portamento param 0..1 maps to 0..0.5 seconds glide time
                    float portTime = snap_.portamento * 0.5f;
                    float glideCoeff = 1.0f - std::exp (-kTwoPi / (sr_ * portTime));
                    voice.currentFreq += (voice.targetFreq - voice.currentFreq) * glideCoeff;
                }
                else
                {
                    voice.currentFreq = voice.targetFreq;
                }

                float f0 = voice.currentFreq * vibPitchMult;

                // Apply coupling FM offset to fundamental
                if (couplingFM != 0.0f)
                    f0 *= (1.0f + couplingFM * 0.1f);  // +/-10% max FM deviation

                // --- Kuramoto field update (every 8 samples) ---
                if (voice.kuramotoField.shouldUpdate())
                {
                    // Extract theta and omega arrays from partial bank
                    float theta[kMaxPartials];
                    float omega[kMaxPartials];
                    int np = voice.partialBank.numPartials;

                    for (int i = 0; i < np; ++i)
                    {
                        theta[i] = voice.partialBank.partials[i].theta;
                        omega[i] = voice.partialBank.partials[i].omega;
                    }

                    // Apply coupling phase perturbation
                    if (couplingPhase != 0.0f)
                    {
                        for (int i = 0; i < np; ++i)
                            theta[i] += couplingPhase;
                    }

                    float voiceDrama = std::clamp (drama_eff + velKOff, 0.0f, 1.0f);
                    voice.kuramotoField.updateField (
                        theta, omega, np,
                        voiceDrama,
                        resSens_eff,
                        snap_.responseSpeed);

                    // Write updated phases back to partial bank
                    for (int i = 0; i < np; ++i)
                        voice.partialBank.partials[i].theta = theta[i];

                    // Invalidate pan cache — Kuramoto state (psi, r) just changed.
                    // Cache will be rebuilt below before the partial render loop.
                    voice.panCacheValid = false;
                }

                // --- Render partials (with unison stacking) ---
                // (Formant weights & Nyquist gains already computed at block rate above)
                float pL = 0.0f, pR = 0.0f;
                float monoSample = 0.0f;

                int unisonCount = std::clamp (snap_.unison, 1, 4);
                float unisonGain = 1.0f / std::sqrt (static_cast<float> (unisonCount));
                // spread: detune * 2% max total spread
                float unisonSpread = snap_.detune * 0.02f;

                // Unison pan positions: 1={C}, 2={L,R}, 3={L,C,R}, 4={L,C,R,C}
                static constexpr float kUniPan[4][4] = {
                    { 0.0f,  0.0f,  0.0f, 0.0f },  // 1 voice: center
                    {-1.0f,  1.0f,  0.0f, 0.0f },  // 2 voices: L, R
                    {-1.0f,  0.0f,  1.0f, 0.0f },  // 3 voices: L, C, R
                    {-1.0f,  0.0f,  0.0f, 1.0f }   // 4 voices: L, C, C, R
                };

                // --- Pan cache rebuild (SRO 2026-03-24) ----------------------
                // Rebuild pan cache whenever Kuramoto field updated or on first
                // render. The panning formula depends on psi/r (Kuramoto outputs,
                // updated every kKuraBlock = 8 samples) and snap_.width / uniPan
                // (block-rate constants). Caching eliminates 2 fastCos+fastSin
                // calls per partial per sample for 7 of every 8 samples.
                // Expected savings: ~448 trig calls/sample at 32 partials × 8
                // voices (7/8 × 512) → ~30-40% CPU reduction.
                if (!voice.panCacheValid)
                {
                    int npCache = voice.partialBank.numPartials;
                    float rCache   = voice.kuramotoField.getOrderParameter();
                    float psiCache = voice.kuramotoField.getMeanPhase();

                    for (int uL = 0; uL < unisonCount; ++uL)
                    {
                        float uniPanCache = kUniPan[unisonCount - 1][uL] * snap_.width;

                        for (int i = 0; i < npCache; ++i)
                        {
                            float pan = OperaPartialBank::computePartialPan (
                                i, voice.partialBank.partials[i].theta,
                                psiCache, rCache, snap_.width);
                            pan = std::clamp (pan + uniPanCache, -1.0f, 1.0f);

                            float angle = (pan + 1.0f) * 0.25f * kPi;
                            voice.cachedPanL[i][uL] = FastMath::fastCos (angle);
                            voice.cachedPanR[i][uL] = FastMath::fastSin (angle);
                        }
                    }
                    voice.panCacheValid = true;
                }
                // -------------------------------------------------------------

                for (int uLayer = 0; uLayer < unisonCount; ++uLayer)
                {
                    // Compute frequency offset for this unison layer
                    float freqMult = 1.0f;
                    if (unisonCount > 1)
                    {
                        // Spread layers symmetrically: -spread..+spread
                        float layerPos = (static_cast<float> (uLayer) / static_cast<float> (unisonCount - 1)) * 2.0f - 1.0f;
                        freqMult = 1.0f + unisonSpread * layerPos;
                    }

                    // Render the partial bank for one sample
                    int np = voice.partialBank.numPartials;

                    for (int i = 0; i < np; ++i)
                    {
                        auto& p = voice.partialBank.partials[i];

                        // Amplitude: formant weight * tilt * Nyquist fade * cluster boost
                        float tiltGain = OperaPartialBank::computeSpectralTilt (
                            i, np, voiceTilt);
                        float amp = voice.partialBank.formantWeights[i]
                                  * tiltGain
                                  * voice.partialBank.nyquistGains[i]
                                  * voice.kuramotoField.getClusterBoost (i);

                        amp *= unisonGain;

                        if (amp < 1e-8f)
                        {
                            // Only advance phase on the first unison layer
                            if (uLayer == 0)
                                p.theta += p.omega * invSr_;
                            continue;
                        }

                        // Apply coupling ring modulation
                        float ringMod = 1.0f;
                        if (couplingRing != 0.0f)
                            ringMod = 1.0f + couplingRing;

                        // Unison layers beyond 0 use a phase offset to avoid identical signals
                        float uniPhase = p.theta * freqMult + static_cast<float> (uLayer) * 1.618f;
                        float sample = amp * ringMod * FastMath::fastSin (uniPhase);

                        // Spatial panning — use cached constant-power coefficients.
                        // Cache is valid for kKuraBlock samples (same lifetime as psi/r).
                        // SRO 2026-03-24: replaces computePartialPan + 2 trig calls per sample.
                        float panL = voice.cachedPanL[i][uLayer];
                        float panR = voice.cachedPanR[i][uLayer];

                        pL += sample * panL;
                        pR += sample * panR;
                        monoSample += sample;

                        // Only advance phase on the first unison layer (shared partial bank)
                        if (uLayer == 0)
                            p.theta += p.omega * invSr_;
                    }
                }

                // Phase wrapping every sample for stability
                {
                    int np = voice.partialBank.numPartials;
                    for (int i = 0; i < np; ++i)
                    {
                        float& theta = voice.partialBank.partials[i].theta;
                        if (theta >= kTwoPi) theta -= kTwoPi * std::floor (theta / kTwoPi);
                        if (theta < 0.0f) theta += kTwoPi;
                    }
                }

                // --- Breath component ---
                float breathL = 0.0f, breathR = 0.0f;
                {
                    // Compute formant center frequencies for breath sympathetic peaks
                    const auto& fTable = getFormantTable();
                    const auto& vowA = fTable.get (snap_.vowelA);
                    const auto& vowB = fTable.get (snap_.vowelB);
                    FormantProfile morphed = OperaPartialBank::interpolateFormants (
                        vowA, vowB, voice_eff);

                    float formantFreqs[3] = { morphed.freq[0], morphed.freq[1], morphed.freq[2] };

                    voice.breathEngine.processSample (
                        breathL, breathR,
                        voiceEffort, breath_eff,
                        f0,
                        voice.kuramotoField.getOrderParameter(),
                        formantFreqs, 3);
                }

                // Mix partials + breath
                float mixL = pL + breathL;
                float mixR = pR + breathR;

                // --- Coupling tap: post-Kuramoto, pre-FX ---
                voice.couplingTap[s] = monoSample;
                monoTap += monoSample;

                // --- Amp envelope ---
                float ampGain = voice.ampEnv.process();
                mixL *= ampGain;
                mixR *= ampGain;

                // --- Filter envelope + SVF ---
                float fltEnvLevel = voice.filterEnv.process();
                float fltEnvOffset = snap_.filterEnvAmt * fltEnvLevel * 8000.0f;  // bipolar: != 0 check
                float voiceCutoff = std::clamp (
                    cutoff_eff + fltEnvOffset + velFilterOff + couplingFilt,
                    20.0f, 20000.0f);
                float Q = 0.5f + snap_.filterRes * 9.5f;

                mixL = voice.filterL.process (mixL, voiceCutoff, Q, sr_);
                mixR = voice.filterR.process (mixR, voiceCutoff, Q, sr_);

                // --- Check voice completion ---
                if (!voice.ampEnv.isActive())
                {
                    voice.state = OperaVoice::State::Idle;
                    voice.note = -1;
                }

                // Accumulate into output
                sampleL += mixL;
                sampleR += mixR;
            }

            // Store mono coupling tap (sum of all voices, pre-FX)
            if (s < kMaxBlockSize)
            {
                couplingCacheL_[s] = sampleL;
                couplingCacheR_[s] = sampleR;
            }

            outL[s] = sampleL;
            outR[s] = sampleR;
        }

        // ------------------------------------------------------------------
        // STEP 7: Apply Reactive Stage (reverb) — block processing
        // ------------------------------------------------------------------
        if (snap_.stage > 0.0001f)
        {
            // Compute average order parameter across active voices
            float avgR = 0.0f;
            int activeCount = 0;
            for (const auto& v : voices_)
            {
                if (v.state != OperaVoice::State::Idle)
                {
                    avgR += v.kuramotoField.getOrderParameter();
                    ++activeCount;
                }
            }
            if (activeCount > 0)
                avgR /= static_cast<float> (activeCount);

            reactiveStage_.processBlock (outL, outR, numSamples, snap_.stage, avgR);
        }

        // ------------------------------------------------------------------
        // STEP 8: Soft clip output
        // ------------------------------------------------------------------
        for (int s = 0; s < numSamples; ++s)
        {
            outL[s] = softClip (outL[s]);
            outR[s] = softClip (outR[s]);
        }

        // ------------------------------------------------------------------
        // STEP 9: Update coupling cache (post-reverb for getSampleForCoupling)
        // Note: couplingCacheL_/R_ was set pre-reverb for mid-chain coupling.
        // If we want post-FX coupling, uncomment below:
        // for (int s = 0; s < numSamples; ++s) {
        //     couplingCacheL_[s] = outL[s];
        //     couplingCacheR_[s] = outR[s];
        // }
        // ------------------------------------------------------------------

        // ------------------------------------------------------------------
        // STEP 10: Clear coupling input buffers for next block
        // ------------------------------------------------------------------
        std::memset (couplingFMBuffer_, 0, sizeof (float) * static_cast<size_t> (numSamples));
        std::memset (couplingRingBuffer_, 0, sizeof (float) * static_cast<size_t> (numSamples));
        std::memset (couplingFilterBuffer_, 0, sizeof (float) * static_cast<size_t> (numSamples));
        std::memset (couplingMorphBuffer_, 0, sizeof (float) * static_cast<size_t> (numSamples));
        std::memset (couplingKBuffer_, 0, sizeof (float) * static_cast<size_t> (numSamples));
        std::memset (couplingPhaseBuffer_, 0, sizeof (float) * static_cast<size_t> (numSamples));

        // ------------------------------------------------------------------
        // STEP 11: Update active voice count for thread-safe UI query
        // ------------------------------------------------------------------
        int count = 0;
        for (const auto& v : voices_)
            if (v.state != OperaVoice::State::Idle) ++count;
        activeVoiceCount_.store (count, std::memory_order_relaxed);
    }

    //==========================================================================
    // Coupling Interface
    //==========================================================================

    /// Return cached coupling sample. O(1). Post-Kuramoto, pre-reverb.
    float getSampleForCoupling (int channel, int sampleIndex) const
    {
        if (sampleIndex < 0 || sampleIndex >= kMaxBlockSize) return 0.0f;
        return (channel == 0) ? couplingCacheL_[sampleIndex]
                              : couplingCacheR_[sampleIndex];
    }

    /// Receive modulation from another engine via the coupling matrix.
    /// Called before renderBlock() for block-level coupling.
    void applyCouplingInput (int type, float amount,
                             const float* sourceBuffer, int numSamples)
    {
        numSamples = std::min (numSamples, kMaxBlockSize);

        // CouplingType enum values from SynthEngine.h:
        // AmpToFilter=0, AmpToPitch=1, ..., AudioToFM=4, AudioToRing=5,
        // FilterToFilter=6, ..., KnotTopology=13, PhaseModulation(PitchToPitch)=10

        switch (type)
        {
            case 4:  // AudioToFM
                for (int i = 0; i < numSamples; ++i)
                    couplingFMBuffer_[i] += sourceBuffer[i] * amount;
                break;

            case 5:  // AudioToRing
                for (int i = 0; i < numSamples; ++i)
                    couplingRingBuffer_[i] += sourceBuffer[i] * amount;
                break;

            case 0:  // AmpToFilter
                for (int i = 0; i < numSamples; ++i)
                    couplingFilterBuffer_[i] += sourceBuffer[i] * amount * 4000.0f;
                break;

            case 3:  // EnvToMorph
                for (int i = 0; i < numSamples; ++i)
                    couplingMorphBuffer_[i] += sourceBuffer[i] * amount;
                break;

            case 13: // KnotTopology
                for (int i = 0; i < numSamples; ++i)
                    couplingKBuffer_[i] += sourceBuffer[i] * amount * 2.0f;
                break;

            case 10: // PitchToPitch
                for (int i = 0; i < numSamples; ++i)
                    couplingPhaseBuffer_[i] += sourceBuffer[i] * amount * 0.5f;
                break;

            case 6:  // FilterToFilter
                for (int i = 0; i < numSamples; ++i)
                    couplingFilterBuffer_[i] += sourceBuffer[i] * amount * 2000.0f;
                break;

            case 1:  // AmpToPitch — pitch modulation from amplitude
                for (int i = 0; i < numSamples; ++i)
                    couplingFMBuffer_[i] += sourceBuffer[i] * amount * 0.5f;
                break;

            default:
                // Unsupported coupling type — silently ignore
                break;
        }
    }

    //==========================================================================
    // Macro Mapping (Section 10 of spec)
    //
    // Macros are computed externally by the host/preset system and
    // applied as parameter offsets. This helper can be called by the host
    // to compute the effective parameter values from a macro position.
    //
    // M1 DRAMA:  drama (exp x^2.5), effort (+0..+0.3), filterCutoff (+0..+4000Hz), vibDepth (+0..+0.15)
    // M2 VOICE:  voice (linear), effort (linear), breath (inverse 0.5..0), tilt (-0.3..+0.3)
    // M3 CHORUS: unison (stepped 1-4), detune (0..0.5), width (0.2..1.0), resSens (0.3..0.8)
    // M4 STAGE:  stage (linear), width (+0..+0.3), filterCutoff (-500Hz), breath (+0..+0.1)
    //
    // These are documented for the preset/macro system to route.
    // The engine itself does not contain a macro processor — it relies on
    // parameter automation from the host or XOlokun macro strip.
    //==========================================================================

private:
    //==========================================================================
    // MIDI Handling
    //==========================================================================

    void handleNoteOn (int noteNumber, float vel)
    {
        int voiceIdx = allocateVoice (noteNumber);
        if (voiceIdx < 0) return;

        auto& voice = voices_[voiceIdx];

        // Store emotional memory from previous note on this voice
        bool wasActive = (voice.state != OperaVoice::State::Idle && voice.note >= 0);
        if (wasActive)
        {
            float theta[kMaxPartials];
            for (int i = 0; i < voice.partialBank.numPartials; ++i)
                theta[i] = voice.partialBank.partials[i].theta;
            voice.kuramotoField.onNoteOff (theta, nullptr, voice.partialBank.numPartials);
        }

        // Setup voice state
        voice.state    = OperaVoice::State::Attack;
        voice.note     = noteNumber;
        voice.velocity = vel;
        voice.startTime = voiceCounter_++;

        // Compute fundamental frequency
        float f0 = midiNoteToFreq (noteNumber + snap_.fundamental, pitchBendSemitones_);

        // --- Portamento: set target freq; keep currentFreq for glide if legato ---
        voice.targetFreq = f0;
        if (snap_.portamento <= 0.0f || !wasActive)
            voice.currentFreq = f0;  // instant: no glide
        // else: keep voice.currentFreq at its old value so it glides

        // Setup partial bank
        voice.partialBank.numPartials = snap_.partials;
        voice.partialBank.setupForNote (
            f0, snap_.partials, snap_.detune,
            snap_.vowelA, snap_.vowelB, snap_.voice);

        // Apply emotional memory (Vangelis: phases carry across note boundaries)
        {
            float theta[kMaxPartials];
            for (int i = 0; i < voice.partialBank.numPartials; ++i)
                theta[i] = voice.partialBank.partials[i].theta;

            voice.kuramotoField.onNoteOn (theta, voice.partialBank.numPartials);

            for (int i = 0; i < voice.partialBank.numPartials; ++i)
                voice.partialBank.partials[i].theta = theta[i];
        }

        // Trigger envelopes
        voice.ampEnv.triggerHard();
        voice.filterEnv.triggerHard();

        // Reset breath engine for clean start
        voice.breathEngine.reset();

        // Reset filter state for voice
        voice.filterL.reset();
        voice.filterR.reset();

        // Invalidate pan cache — note frequency / theta have changed.
        // The cache will be rebuilt at the next Kuramoto update or immediately
        // on the first render sample (panCacheValid = false guard).
        voice.panCacheValid = false;

        // Trigger conductor arc if in conductor/both mode
        if (snap_.arcMode >= 1)
            conductor_.trigger();
    }

    void handleNoteOff (int noteNumber)
    {
        for (auto& voice : voices_)
        {
            if (voice.note == noteNumber && voice.state != OperaVoice::State::Idle
                && voice.state != OperaVoice::State::Release)
            {
                voice.state = OperaVoice::State::Release;
                voice.ampEnv.release();
                voice.filterEnv.release();

                // Store emotional memory for Kuramoto field
                float theta[kMaxPartials];
                for (int i = 0; i < voice.partialBank.numPartials; ++i)
                    theta[i] = voice.partialBank.partials[i].theta;
                voice.kuramotoField.onNoteOff (theta, nullptr, voice.partialBank.numPartials);
            }
        }
    }

    //==========================================================================
    // Voice Allocation — LRU with release priority
    //
    // Priority: 1. Free (idle) voice
    //           2. Oldest voice in release stage
    //           3. Oldest active voice (LRU steal)
    //==========================================================================

    int allocateVoice (int noteNumber)
    {
        // Check if this note is already playing (retrigger)
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices_[i].note == noteNumber
                && voices_[i].state != OperaVoice::State::Idle)
                return i;
        }

        // Find a free voice
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices_[i].state == OperaVoice::State::Idle)
                return i;
        }

        // Steal oldest released voice
        int bestIdx = -1;
        uint64_t oldestTime = UINT64_MAX;

        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices_[i].state == OperaVoice::State::Release
                && voices_[i].startTime < oldestTime)
            {
                oldestTime = voices_[i].startTime;
                bestIdx = i;
            }
        }

        if (bestIdx >= 0)
        {
            voices_[bestIdx].ampEnv.kill();
            voices_[bestIdx].filterEnv.kill();
            return bestIdx;
        }

        // Steal oldest active voice (LRU)
        oldestTime = UINT64_MAX;
        bestIdx = 0;

        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices_[i].startTime < oldestTime)
            {
                oldestTime = voices_[i].startTime;
                bestIdx = i;
            }
        }

        voices_[bestIdx].ampEnv.kill();
        voices_[bestIdx].filterEnv.kill();
        return bestIdx;
    }

    //==========================================================================
    // Modulation routing
    //==========================================================================

    static void applyModulation (ModOffsets& mods, ModDest dest, float value) noexcept
    {
        switch (dest)
        {
            case ModDest::Drama:        mods.drama        += value;            break;
            case ModDest::Voice:        mods.voice        += value;            break;
            case ModDest::Breath:       mods.breath       += value;            break;
            case ModDest::Effort:       mods.effort       += value;            break;
            case ModDest::Tilt:         mods.tilt         += value;            break;
            case ModDest::FilterCutoff: mods.filterCutoff += value * 4000.0f;  break;
            case ModDest::VibDepth:     mods.vibDepth     += value;            break;
            case ModDest::ResSens:      mods.resSens      += value;            break;
        }
    }

    //==========================================================================
    // ParamSnapshot cache
    //==========================================================================

    void cacheParamSnapshot() noexcept
    {
        if (p_drama == nullptr) return;  // Parameters not yet attached

        snap_.drama         = p_drama->load();
        snap_.voice         = p_voice->load();
        snap_.vowelA        = static_cast<int> (p_vowelA->load());
        snap_.vowelB        = static_cast<int> (p_vowelB->load());
        snap_.breath        = p_breath->load();
        snap_.effort        = p_effort->load();
        snap_.partials      = static_cast<int> (p_partials->load());
        snap_.detune        = p_detune->load();
        snap_.tilt          = p_tilt->load();
        snap_.fundamental   = static_cast<int> (p_fundamental->load());
        snap_.resSens       = p_resSens->load();
        snap_.portamento    = p_portamento->load();
        snap_.unison        = static_cast<int> (p_unison->load());
        snap_.width         = p_width->load();
        snap_.vibRate       = p_vibRate->load();
        snap_.vibDepth      = p_vibDepth->load();
        snap_.stage         = p_stage->load();
        snap_.responseSpeed = p_responseSpeed->load();

        snap_.filterCutoff  = p_filterCutoff->load();
        snap_.filterRes     = p_filterRes->load();
        snap_.filterEnvAmt  = p_filterEnvAmt->load();
        snap_.filterA       = p_filterA->load();
        snap_.filterD       = p_filterD->load();
        snap_.filterS       = p_filterS->load();
        snap_.filterR       = p_filterR->load();

        snap_.ampA          = p_ampA->load();
        snap_.ampD          = p_ampD->load();
        snap_.ampS          = p_ampS->load();
        snap_.ampR          = p_ampR->load();

        snap_.lfo1Rate      = p_lfo1Rate->load();
        snap_.lfo1Depth     = p_lfo1Depth->load();
        snap_.lfo1Dest      = static_cast<int> (p_lfo1Dest->load());
        snap_.lfo2Rate      = p_lfo2Rate->load();
        snap_.lfo2Depth     = p_lfo2Depth->load();
        snap_.lfo2Dest      = static_cast<int> (p_lfo2Dest->load());

        snap_.arcMode       = static_cast<int> (p_arcMode->load());
        snap_.arcShape      = static_cast<int> (p_arcShape->load());
        snap_.arcTime       = p_arcTime->load();
        snap_.arcPeak       = p_arcPeak->load();

        snap_.modWheelDest  = static_cast<int> (p_modWheelDest->load());
        snap_.modWheelAmt   = p_modWheelAmt->load();
        snap_.atDest        = static_cast<int> (p_atDest->load());
        snap_.atAmt         = p_atAmt->load();
        snap_.velToFilter   = p_velToFilter->load();
        snap_.velToEffort   = p_velToEffort->load();
    }

    //==========================================================================
    // Utility functions
    //==========================================================================

    /// MIDI note number to frequency (Hz) with semitone offset and pitch bend.
    static float midiNoteToFreq (int note, float bendSemitones) noexcept
    {
        float n = static_cast<float> (note) + bendSemitones;
        return 440.0f * std::pow (2.0f, (n - 69.0f) / 12.0f);
    }

    /// Soft clipper: tanh-style saturation, prevents hard clipping.
    static float softClip (float x) noexcept
    {
        if (x > 3.0f) return 1.0f;
        if (x < -3.0f) return -1.0f;
        // Pade approximant tanh(x) ~ x*(27+x^2)/(27+9*x^2)
        float x2 = x * x;
        return x * (27.0f + x2) / (27.0f + 9.0f * x2);
    }

    //==========================================================================
    // State
    //==========================================================================

    float sr_    = 48000.0f;
    float invSr_ = 1.0f / 48000.0f;
    int   blockSize_ = 512;

    // Voice pool
    OperaVoice voices_[kMaxVoices];
    uint64_t voiceCounter_ = 0;
    std::atomic<int> activeVoiceCount_ { 0 };

    // Global DSP modules
    OperaLFO       lfo1_;
    OperaLFO       lfo2_;
    OperaLFO       vibratoLFO_;
    OperaConductor conductor_;
    ReactiveStage  reactiveStage_;

    // Per-block parameter snapshot
    ParamSnapshot snap_;

    // Expression state (updated by MIDI CC)
    float modWheelValue_      = 0.0f;
    float aftertouchValue_    = 0.0f;
    float pitchBendSemitones_ = 0.0f;

    // Coupling output cache (post-Kuramoto, pre-reverb)
    float couplingCacheL_[kMaxBlockSize] = {};
    float couplingCacheR_[kMaxBlockSize] = {};

    // Coupling input buffers (filled by applyCouplingInput, consumed by renderBlock)
    float couplingFMBuffer_[kMaxBlockSize]     = {};
    float couplingRingBuffer_[kMaxBlockSize]   = {};
    float couplingFilterBuffer_[kMaxBlockSize] = {};
    float couplingMorphBuffer_[kMaxBlockSize]  = {};
    float couplingKBuffer_[kMaxBlockSize]      = {};
    float couplingPhaseBuffer_[kMaxBlockSize]  = {};

    //==========================================================================
    // Raw parameter pointers (cached in attachParameters)
    //==========================================================================

    // Core synthesis
    std::atomic<float>* p_drama         = nullptr;
    std::atomic<float>* p_voice         = nullptr;
    std::atomic<float>* p_vowelA        = nullptr;
    std::atomic<float>* p_vowelB        = nullptr;
    std::atomic<float>* p_breath        = nullptr;
    std::atomic<float>* p_effort        = nullptr;
    std::atomic<float>* p_partials      = nullptr;
    std::atomic<float>* p_detune        = nullptr;
    std::atomic<float>* p_tilt          = nullptr;
    std::atomic<float>* p_fundamental   = nullptr;
    std::atomic<float>* p_resSens       = nullptr;
    std::atomic<float>* p_portamento    = nullptr;
    std::atomic<float>* p_unison        = nullptr;
    std::atomic<float>* p_width         = nullptr;
    std::atomic<float>* p_vibRate       = nullptr;
    std::atomic<float>* p_vibDepth      = nullptr;
    std::atomic<float>* p_stage         = nullptr;
    std::atomic<float>* p_responseSpeed = nullptr;

    // Filter
    std::atomic<float>* p_filterCutoff  = nullptr;
    std::atomic<float>* p_filterRes     = nullptr;
    std::atomic<float>* p_filterEnvAmt  = nullptr;
    std::atomic<float>* p_filterA       = nullptr;
    std::atomic<float>* p_filterD       = nullptr;
    std::atomic<float>* p_filterS       = nullptr;
    std::atomic<float>* p_filterR       = nullptr;

    // Amp envelope
    std::atomic<float>* p_ampA          = nullptr;
    std::atomic<float>* p_ampD          = nullptr;
    std::atomic<float>* p_ampS          = nullptr;
    std::atomic<float>* p_ampR          = nullptr;

    // LFOs
    std::atomic<float>* p_lfo1Rate      = nullptr;
    std::atomic<float>* p_lfo1Depth     = nullptr;
    std::atomic<float>* p_lfo1Dest      = nullptr;
    std::atomic<float>* p_lfo2Rate      = nullptr;
    std::atomic<float>* p_lfo2Depth     = nullptr;
    std::atomic<float>* p_lfo2Dest      = nullptr;

    // Conductor
    std::atomic<float>* p_arcMode       = nullptr;
    std::atomic<float>* p_arcShape      = nullptr;
    std::atomic<float>* p_arcTime       = nullptr;
    std::atomic<float>* p_arcPeak       = nullptr;

    // Expression routing
    std::atomic<float>* p_modWheelDest  = nullptr;
    std::atomic<float>* p_modWheelAmt   = nullptr;
    std::atomic<float>* p_atDest        = nullptr;
    std::atomic<float>* p_atAmt         = nullptr;
    std::atomic<float>* p_velToFilter   = nullptr;
    std::atomic<float>* p_velToEffort   = nullptr;
};

} // namespace opera
