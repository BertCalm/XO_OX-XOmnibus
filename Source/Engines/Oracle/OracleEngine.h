// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/ModMatrix.h"
#include "../../DSP/GlideProcessor.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
//
//  ██╗  ██╗ ██████╗ ██████╗  █████╗  ██████╗██╗     ███████╗
//  ╚██╗██╔╝██╔═══██╗██╔══██╗██╔══██╗██╔════╝██║     ██╔════╝
//   ╚███╔╝ ██║   ██║██████╔╝███████║██║     ██║     █████╗
//   ██╔██╗ ██║   ██║██╔══██╗██╔══██║██║     ██║     ██╔══╝
//  ██╔╝ ██╗╚██████╔╝██║  ██║██║  ██║╚██████╗███████╗███████╗
//  ╚═╝  ╚═╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚══════╝╚══════╝
//
//  OracleEngine — Stochastic GENDY + Maqam Microtonal Synthesis
//  Accent: Prophecy Indigo #4B0082
//  XO_OX Designs | XOceanus Engine Module
//
//==============================================================================
//
//  AQUATIC IDENTITY — THE REEF (Fifth Generation Extremophile)
//
//  XOracle is the ancient reef formation. Geological time made audible.
//  Layer upon layer of coral skeleton compressed into living stone that
//  remembers every current that ever passed through it. Each waveform
//  cycle is a geological epoch — breakpoints shifting like tectonic plates,
//  slow enough to feel inevitable, fast enough to surprise.
//
//  In the XO_OX water column, Oracle lives at reef depth — warm enough
//  for Oscar's patience, shallow enough for feliX's curiosity. It is the
//  fossil record of sound: stochastic evolution preserved in waveform strata.
//
//  HISTORICAL LINEAGE
//
//  Channels Iannis Xenakis's GENDY system (1991) — "Stochastic Waveform
//  Synthesis" where the waveform itself is the composition. Xenakis built
//  GENDY1-GENDY3 on the NeXT computer at CEMAMu, generating entire pieces
//  from nothing but probability distributions walking across breakpoints.
//  The Oracle engine inherits this philosophy: no oscillator, no wavetable,
//  no sample — just N points in time-amplitude space, drifting each cycle.
//
//  The maqam tuning system adds a second historical layer: centuries-old
//  Middle-Eastern modal theory with quarter-tone intervals (50-cent steps)
//  that Western 12-TET cannot represent. The GRAVITY parameter blends
//  between equal temperament and maqam intonation, letting the reef
//  remember tuning systems older than the piano.
//
//  SYNTHESIS TECHNIQUE
//
//  Each voice maintains 8-32 breakpoints defining one waveform cycle.
//  Per cycle, breakpoints undergo stochastic random walks drawn from a
//  morphable Cauchy/Logistic distribution blend. Mirror barriers reflect
//  overshooting values like billiard balls. Cubic Hermite (Catmull-Rom)
//  interpolation renders smooth per-sample output between breakpoints.
//
//  Signal Flow:
//    GENDY Breakpoints (random walk per cycle)
//      -> Cubic Hermite Interpolation (per sample)
//      -> DC Blocker (5Hz first-order HPF)
//      -> Soft Limiter (tanh saturation)
//      -> Amp Envelope * Velocity * Crossfade
//      -> Stereo Decorrelation (1% phase offset on R channel)
//      -> Master Level -> Output
//
//  FEATURES
//    - N breakpoints (8-32) per waveform cycle with stochastic random walk
//    - Two morphable distributions: Cauchy (heavy-tailed) and Logistic (smooth)
//    - Mirror barrier reflection on breakpoint boundaries
//    - Cubic Hermite interpolation between breakpoints (per-sample)
//    - 8 maqamat with quarter-tone microtonal tuning
//    - Gravity parameter blends 12-TET and maqam tuning
//    - DC blocker (5Hz first-order HPF) + soft limiter post-processing
//    - Mono/Legato/Poly4/Poly8 voice modes with LRU stealing + 5ms crossfade
//    - 2 LFOs (Sine/Tri/Saw/Square/S&H)
//    - Full XOceanus coupling support
//
//  COUPLING
//    - Output: post-processing stereo audio via getSampleForCoupling
//    - Input: AudioToFM (perturb breakpoint amplitudes),
//             AmpToFilter (modulate barrier positions),
//             EnvToMorph (drive distribution morph)
//
//  MACROS: PROPHECY, EVOLUTION, GRAVITY, DRIFT
//
//==============================================================================

//==============================================================================
//
//  MAQAM TUNING TABLES
//
//  Each maqam (Arabic: "position" or "place") defines a modal framework
//  with characteristic interval patterns, many using quarter-tones (50 cents)
//  that have no equivalent in Western 12-TET tuning.
//
//  Cent offsets are relative to the scale root (degree 0 = 0 cents).
//  Each maqam has 7 intervals plus the octave (degree 7 = 1200 cents).
//
//  Reference intervals:
//    Quarter-tone  =  50 cents  (halfway between two semitones)
//    Semitone      = 100 cents  (standard Western half-step)
//    Whole tone    = 200 cents  (standard Western whole-step)
//    Aug. second   = 300 cents  (characteristic of Hijaz)
//    Octave        = 1200 cents
//
//==============================================================================

struct MaqamTable
{
    float cents[8]; // Cent offsets for degrees 0-7 (octave at index 7 = 1200)
};

static constexpr int kNumMaqamat = 8;

// The 8 canonical maqamat, ordered by prevalence in Arabic music tradition.
// Notation: Ed = E half-flat (quarter-tone below E), Bd = B half-flat.
static const MaqamTable kMaqamat[kNumMaqamat] = {

    // 0: Rast — "The foundation maqam." Most common in Arabic music.
    //    C  D  Ed  F  G  A  Bd  C
    //    Intervals: 200, 150, 150, 200, 200, 150, 150
    {{0.0f, 200.0f, 350.0f, 500.0f, 700.0f, 900.0f, 1050.0f, 1200.0f}},

    // 1: Bayati — Contemplative, devotional. Used in Quran recitation.
    //    D  Ed  F  G  A  Bb  C  D
    //    Intervals: 150, 150, 200, 200, 100, 200, 200
    {{0.0f, 150.0f, 300.0f, 500.0f, 700.0f, 800.0f, 1000.0f, 1200.0f}},

    // 2: Saba — Sorrowful, yearning. Diminished character.
    //    D  Ed  F  Gb  A  Bb  C  D
    //    Intervals: 150, 150, 100, 300, 100, 200, 200
    {{0.0f, 150.0f, 300.0f, 400.0f, 700.0f, 800.0f, 1000.0f, 1200.0f}},

    // 3: Hijaz — Dramatic, exotic. Signature augmented second (300 cents).
    //    D  Eb  F#  G  A  Bb  C  D
    //    Intervals: 100, 300, 100, 200, 100, 200, 200
    {{0.0f, 100.0f, 400.0f, 500.0f, 700.0f, 800.0f, 1000.0f, 1200.0f}},

    // 4: Sikah — Ethereal, floating. Quarter-tone root creates alien quality.
    //    Ed  F  G  Ab  Bd  C  D  Ed
    //    Intervals: 150, 200, 100, 200, 200, 200, 150
    {{0.0f, 150.0f, 350.0f, 450.0f, 650.0f, 850.0f, 1050.0f, 1200.0f}},

    // 5: Nahawand — Closest to Western harmonic minor. Warm, familiar.
    //    C  D  Eb  F  G  Ab  Bb  C
    //    Intervals: 200, 100, 200, 200, 100, 200, 200
    {{0.0f, 200.0f, 300.0f, 500.0f, 700.0f, 800.0f, 1000.0f, 1200.0f}},

    // 6: Kurd — Phrygian-like. Dark, resolute, minor second opening.
    //    D  Eb  F  G  A  Bb  C  D
    //    Intervals: 100, 200, 200, 200, 100, 200, 200
    {{0.0f, 100.0f, 300.0f, 500.0f, 700.0f, 800.0f, 1000.0f, 1200.0f}},

    // 7: Ajam — The major scale. Bright, celebratory. Ionian mode.
    //    C  D  E  F  G  A  B  C
    //    Intervals: 200, 200, 100, 200, 200, 200, 100
    {{0.0f, 200.0f, 400.0f, 500.0f, 700.0f, 900.0f, 1100.0f, 1200.0f}}};

//==============================================================================
//
//  XORSHIFT64 PRNG
//
//  Fast, stateful pseudo-random number generator — one instance per voice.
//  Uses Marsaglia's xorshift64 algorithm (2003) with shift constants
//  13, 7, 17 chosen for maximal period (2^64 - 1) and good statistical
//  properties. Deterministic from seed, which allows identical note
//  numbers to produce the same stochastic evolution across sessions.
//
//  Audio-thread safe: no allocation, no branching beyond shifts.
//
//==============================================================================

struct Xorshift64
{
    uint64_t state = 1;

    void seed(uint64_t s) noexcept
    {
        state = (s == 0) ? 1 : s; // State must never be zero (absorbing state)
    }

    uint64_t next() noexcept
    {
        uint64_t x = state;
        x ^= x << 13; // Marsaglia xorshift64 triple: (13, 7, 17)
        x ^= x >> 7;
        x ^= x << 17;
        state = x;
        return x;
    }

    // Uniform float in [0, 1) — mask to 32 bits, divide by 2^32
    float uniform() noexcept { return static_cast<float>(next() & 0xFFFFFFFF) / 4294967296.0f; }

    // Uniform float in [-1, 1) — bipolar for stochastic perturbation
    float uniformBipolar() noexcept { return uniform() * 2.0f - 1.0f; }
};

//==============================================================================
//
//  GENDY BREAKPOINT
//
//  A single control point in the stochastic waveform. Xenakis's GENDY system
//  defines waveforms not as fixed functions but as constellations of movable
//  points. Each cycle, every breakpoint drifts — the waveform is never the
//  same twice. In the Oracle's reef metaphor, each breakpoint is a coral
//  polyp slowly repositioning itself over geological time.
//
//==============================================================================

static constexpr int kMaxBreakpoints = 32; // Xenakis used 12-15 in GENDY3; we allow up to 32

struct GENDYBreakpoint
{
    float timeOffset = 0.0f; // Position within the cycle [0, 1]
    float amplitude = 0.0f;  // Waveform height at this position [-1, 1]
};

//==============================================================================
//
//  ADSR ENVELOPE GENERATOR
//
//  Replaced with fleet-standard implementation. See Source/DSP/StandardADSR.h.
//
//==============================================================================

using OracleADSR = StandardADSR;

//==============================================================================
//
//  LOW-FREQUENCY OSCILLATOR
//
//  Replaced with fleet-standard implementation. See Source/DSP/StandardLFO.h.
//
//==============================================================================

using OracleLFO = StandardLFO;

//==============================================================================
//
//  DC BLOCKER — First-Order High-Pass Filter at ~5Hz
//
//  GENDY waveforms are asymmetric by nature (breakpoints have no obligation
//  to sum to zero), so DC offset is inevitable. This first-order HPF removes
//  sub-5Hz content that would otherwise push speaker cones to their limits
//  and waste headroom.
//
//  Transfer function: y[n] = x[n] - x[n-1] + R * y[n-1]
//  where R = 1 - (2*pi*fc/sr), clamped to [0.9, 0.9999]
//
//  The R coefficient controls how quickly the blocker forgets DC. At 44.1kHz
//  with fc=5Hz, R ~= 0.9993 — a very gentle rolloff that preserves all
//  audible bass while blocking true DC.
//
//==============================================================================

struct OracleDCBlocker
{
    float previousInput = 0.0f;    // x[n-1]
    float previousOutput = 0.0f;   // y[n-1]
    float feedbackCoeff = 0.9999f; // R: controls DC rejection cutoff

    void prepare(float sampleRate) noexcept
    {
        constexpr float cutoffHz = 5.0f; // Remove content below 5Hz
        constexpr float twoPi = 6.28318530718f;
        feedbackCoeff = 1.0f - (twoPi * cutoffHz / std::max(1.0f, sampleRate));

        // Clamp R to a safe range:
        //   Below 0.9 = too aggressive (audibly removes bass)
        //   Above 0.9999 = too gentle (DC bleeds through)
        if (feedbackCoeff < 0.9f)
            feedbackCoeff = 0.9f;
        if (feedbackCoeff > 0.9999f)
            feedbackCoeff = 0.9999f;
    }

    float process(float input) noexcept
    {
        float output = input - previousInput + feedbackCoeff * previousOutput;
        previousInput = input;

        // DENORMAL PROTECTION: The feedback path (R * y[n-1]) can produce
        // denormalized floats as the output decays toward zero. On x86,
        // denormals trigger microcode exception handling that is 10-100x
        // slower than normal float ops, causing CPU spikes during silence.
        // flushDenormal() snaps near-zero values to exactly 0.0f.
        previousOutput = flushDenormal(output);

        return previousOutput;
    }

    void reset() noexcept
    {
        previousInput = 0.0f;
        previousOutput = 0.0f;
    }
};

//==============================================================================
//
//  ORACLE VOICE — Per-Voice State
//
//  Each voice is an independent GENDY universe: its own breakpoints, its
//  own PRNG seed, its own stochastic evolution. When a note triggers, the
//  breakpoints are initialized to a sine shape and then left to drift —
//  no two voices evolve the same way, even with identical parameters.
//
//==============================================================================

struct OracleVoice
{
    // --- Voice lifecycle ---
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0; // Monotonic counter for LRU voice stealing

    // --- GENDY waveform: the breakpoint constellation ---
    GENDYBreakpoint breakpoints[kMaxBreakpoints];
    int numBreakpoints = 16; // Active breakpoint count (8-32)

    // --- Waveform phase ---
    float wavePhase = 0.0f;          // Position within current cycle [0, 1)
    float wavePhaseIncrement = 0.0f; // Frequency / sampleRate (per-sample advance)

    // --- Pitch ---
    GlideProcessor glide; // Portamento: smoothly approaches targetFreq

    // --- Per-voice PRNG for stochastic evolution ---
    Xorshift64 rng;

    // --- Envelopes ---
    OracleADSR amplitudeEnvelope;
    OracleADSR stochasticEnvelope; // Controls how much the breakpoints drift over time

    // --- LFOs ---
    OracleLFO lfo1; // LFO1 -> time step modulation
    OracleLFO lfo2; // LFO2 -> amplitude step modulation

    // --- DC blockers (one per stereo channel) ---
    OracleDCBlocker dcBlockerL;
    OracleDCBlocker dcBlockerR;

    // --- Voice-stealing crossfade ---
    float fadeGain = 1.0f;  // Current crossfade gain (1.0 = full, fading toward 0)
    bool fadingOut = false; // True when this voice is being stolen

    // --- Cached output for stereo spread ---
    float lastOutputL = 0.0f;
    float lastOutputR = 0.0f;

    // --- Cycle-boundary crossfade ---
    // When breakpoints evolve at a cycle boundary, the waveform shape changes
    // discontinuously. This crossfade blends the last sample of the previous
    // cycle with the new waveform over 64 samples (~1.5ms at 44.1kHz) to
    // prevent audible clicks at the seam.
    float prevCycleLastSample = 0.0f;
    int cycleBlendCounter = 0;
    static constexpr int kCycleBlendSamples = 64; // ~1.5ms at 44.1kHz

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        wavePhase = 0.0f;
        wavePhaseIncrement = 0.0f;
        glide.reset();
        glide.snapTo(440.0f);
        fadeGain = 1.0f;
        fadingOut = false;
        lastOutputL = 0.0f;
        lastOutputR = 0.0f;
        prevCycleLastSample = 0.0f;
        cycleBlendCounter = 0;
        amplitudeEnvelope.reset();
        stochasticEnvelope.reset();
        lfo1.reset();
        lfo2.reset();
        dcBlockerL.reset();
        dcBlockerR.reset();
        rng.seed(1);

        // Initialize breakpoints to a sine-like shape — the "primordial"
        // waveform before stochastic evolution begins reshaping it
        for (int i = 0; i < kMaxBreakpoints; ++i)
        {
            float normalizedPosition = static_cast<float>(i) / static_cast<float>(kMaxBreakpoints);
            breakpoints[i].timeOffset = normalizedPosition;
            breakpoints[i].amplitude = std::sin(normalizedPosition * 6.28318530718f); // 2*pi
        }
    }

    //--------------------------------------------------------------------------
    // Initialize breakpoints for a given count — distribute evenly with
    // a sine-like amplitude shape for a clean starting waveform.
    //--------------------------------------------------------------------------
    void initBreakpoints(int count) noexcept
    {
        numBreakpoints = std::max(8, std::min(kMaxBreakpoints, count));
        for (int i = 0; i < numBreakpoints; ++i)
        {
            float normalizedPosition = static_cast<float>(i) / static_cast<float>(numBreakpoints);
            breakpoints[i].timeOffset = normalizedPosition;
            breakpoints[i].amplitude = std::sin(normalizedPosition * 6.28318530718f); // 2*pi
        }
    }
};

//==============================================================================
//
//  ORACLE ENGINE — Main Engine Class
//
//  The reef keeper. Manages up to 8 polyphonic voices, each carrying its
//  own GENDY breakpoint constellation. The engine reads parameters once
//  per block (ParamSnapshot pattern), applies macro and coupling offsets,
//  then renders each voice through the stochastic pipeline.
//
//==============================================================================
class OracleEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8; // Poly8 is the widest voice mode
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateDouble = sampleRate;
        sampleRateFloat = static_cast<float>(sampleRateDouble);

        // Parameter smoothing coefficient: 5ms time constant.
        // This prevents zipper noise when parameters change mid-block.
        // Formula: 1 - e^(-2*pi * (1/tau) / sr), where tau = 0.005s
        constexpr float kSmoothingTimeConstant = 0.005f; // 5ms — fast enough to track, slow enough to smooth
        parameterSmoothingCoeff = 1.0f - std::exp(-kTwoPi * (1.0f / kSmoothingTimeConstant) / sampleRateFloat);

        // Voice-stealing crossfade rate: ramp gain from 1.0 to 0.0 over 5ms.
        // At 44.1kHz: 1.0 / (0.005 * 44100) = ~0.00454 per sample.
        constexpr float kCrossfadeDuration = 0.005f; // 5ms — matches XOceanus engine hot-swap standard
        voiceCrossfadeRate = 1.0f / (kCrossfadeDuration * sampleRateFloat);

        // Allocate output cache for coupling reads (other engines read our output)
        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        aftertouch.prepare(sampleRate);

        silenceGate.prepare(sampleRate, maxBlockSize);
        silenceGate.setHoldTime(500.0f); // Oracle has reverb tails

        // Initialize all voices
        for (auto& voice : voices)
        {
            voice.reset();
            voice.dcBlockerL.prepare(sampleRateFloat);
            voice.dcBlockerR.prepare(sampleRateFloat);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingBreakpointMod = 0.0f;
        couplingBarrierMod = 0.0f;
        couplingDistributionMod = 0.0f;

        smoothedTimeStep = 0.3f;
        smoothedAmpStep = 0.3f;
        smoothedDistribution = 0.5f;

        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // SynthEngine interface — Audio
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;

        // ----- ParamSnapshot: read all parameters once per block -----
        // This pattern avoids repeated atomic loads in the per-sample loop.
        // Each parameter is read exactly once from the APVTS atomic cache.

        // Core GENDY parameters
        const int breakpointCount = static_cast<int>(loadParam(pBreakpoints, 16.0f));
        const float timeStepAmount = loadParam(pTimeStep, 0.3f);
        const float amplitudeStepAmount = loadParam(pAmpStep, 0.3f);
        const float distributionMorph = loadParam(pDistribution, 0.5f);
        const float barrierElasticity = loadParam(pBarrierElasticity, 0.5f);
        const int maqamIndex = static_cast<int>(loadParam(pMaqam, 0.0f));
        const float gravityAmount = loadParam(pGravity, 0.0f);
        const float driftAmount = loadParam(pDrift, 0.3f);
        const float masterLevel = loadParam(pLevel, 0.8f);

        // Amplitude envelope ADSR
        const float ampAttackTime = loadParam(pAmpAttack, 0.01f);
        const float ampDecayTime = loadParam(pAmpDecay, 0.1f);
        const float ampSustainLevel = loadParam(pAmpSustain, 0.8f);
        const float ampReleaseTime = loadParam(pAmpRelease, 0.3f);

        // Stochastic evolution envelope ADSR
        const float stochAttackTime = loadParam(pStochEnvAttack, 0.05f);
        const float stochDecayTime = loadParam(pStochEnvDecay, 0.3f);
        const float stochSustainLevel = loadParam(pStochEnvSustain, 0.7f);
        const float stochReleaseTime = loadParam(pStochEnvRelease, 0.5f);

        // LFO parameters
        const float lfo1RateHz = loadParam(pLfo1Rate, 1.0f);
        const float lfo1DepthAmount = loadParam(pLfo1Depth, 0.0f);
        const int lfo1ShapeIndex = static_cast<int>(loadParam(pLfo1Shape, 0.0f));
        const float lfo2RateHz = loadParam(pLfo2Rate, 1.0f);
        const float lfo2DepthAmount = loadParam(pLfo2Depth, 0.0f);
        const int lfo2ShapeIndex = static_cast<int>(loadParam(pLfo2Shape, 0.0f));

        // Voice configuration
        const int voiceModeIndex = static_cast<int>(loadParam(pVoiceMode, 2.0f));
        const float glideTime = loadParam(pGlide, 0.0f);

        // Macros
        const float macroProphecy = loadParam(pMacroProphecy, 0.0f);
        const float macroEvolution = loadParam(pMacroEvolution, 0.0f);
        const float macroGravity = loadParam(pMacroGravity, 0.0f);
        const float macroDrift = loadParam(pMacroDrift, 0.0f);

        // D001: velocity-to-timbre depth — how much velocity scales stochastic drift.
        // Louder notes produce denser, more complex spectra via deeper breakpoint evolution.
        const float velDriftDepth = loadParam(pVelDriftDepth, 0.3f);

        // ----- Voice mode configuration -----
        int maxPolyphony = kMaxVoices;
        bool isMonophonic = false;
        bool isLegatoMode = false;
        switch (voiceModeIndex)
        {
        case 0:
            maxPolyphony = 1;
            isMonophonic = true;
            break; // Mono
        case 1:
            maxPolyphony = 1;
            isMonophonic = true;
            isLegatoMode = true;
            break; // Legato
        case 2:
            maxPolyphony = 4;
            break; // Poly4
        case 3:
            maxPolyphony = 8;
            break; // Poly8
        default:
            maxPolyphony = 4;
            break;
        }

        // Glide coefficient: exponential approach toward target frequency.
        // 0.001s minimum prevents instant glide from bypassing the smoothing.
        float glideCoefficient = 1.0f;
        if (glideTime > 0.001f)
            glideCoefficient = 1.0f - std::exp(-1.0f / (glideTime * sampleRateFloat));

        // ----- Apply macro and coupling offsets to effective parameters -----
        // Macros and coupling modulation are additive — they push parameters
        // beyond their base values, then clamp to safe ranges.

        float effectiveTimeStep = clamp(timeStepAmount + macroEvolution * 0.4f // EVOLUTION macro adds chaos speed
                                            + macroDrift * 0.2f,
                                        0.0f, 1.0f); // DRIFT macro adds gentle motion
        float effectiveAmpStep =
            clamp(amplitudeStepAmount + macroEvolution * 0.3f // EVOLUTION macro adds amplitude drift
                      + macroProphecy * 0.2f,
                  0.0f, 1.0f); // PROPHECY macro adds unpredictability
        float effectiveDistribution =
            clamp(distributionMorph + couplingDistributionMod // Coupling: external envelope drives distribution
                      + macroProphecy * 0.3f,
                  0.0f, 1.0f); // PROPHECY macro shifts toward Cauchy (erratic)
        float effectiveElasticity = clamp(barrierElasticity + couplingBarrierMod, 0.0f, 1.0f);
        float effectiveGravity =
            clamp(gravityAmount + macroGravity * 0.5f, 0.0f, 1.0f); // GRAVITY macro pulls toward maqam tuning
        // D006: mod wheel increases gravity below after MIDI loop — atPressure and modWheelValue resolved there
        // D006: aftertouch added below — atPressure resolved after MIDI loop
        float effectiveDrift = clamp(driftAmount + macroDrift * 0.3f, 0.0f, 1.0f);
        int effectiveMaqam = std::max(0, std::min(kNumMaqamat, maqamIndex));

        // Reset coupling accumulators for next block
        // P25 fix: capture couplingBreakpointMod before zeroing — it is read inside
        // the per-sample loop via evolveBreakpoints(). The other two mods are already
        // captured into effectiveDistribution/effectiveElasticity above.
        const float capturedBreakpointMod = couplingBreakpointMod;
        couplingBreakpointMod = 0.0f;
        couplingBarrierMod = 0.0f;
        couplingDistributionMod = 0.0f;

        // ----- Process MIDI events -----
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), maxPolyphony, isMonophonic, isLegatoMode,
                       glideCoefficient, breakpointCount, effectiveMaqam, effectiveGravity, ampAttackTime, ampDecayTime,
                       ampSustainLevel, ampReleaseTime, stochAttackTime, stochDecayTime, stochSustainLevel,
                       stochReleaseTime, lfo1RateHz, lfo1DepthAmount, lfo1ShapeIndex, lfo2RateHz, lfo2DepthAmount,
                       lfo2ShapeIndex);
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            // D006: channel pressure → aftertouch (applied to GENDY drift below)
            else if (msg.isChannelPressure())
                aftertouch.setChannelPressure(msg.getChannelPressureValue() / 127.0f);
            // D006: CC1 mod wheel → Maqam gravity (stronger scale attraction with wheel; sensitivity 0.4)
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelValue = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // D006: smooth aftertouch and apply to drift — pressure increases stochastic chaos
        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);
        // Sensitivity 0.15: drift is a powerful param — lighter touch to avoid instability
        effectiveDrift = clamp(effectiveDrift + atPressure * 0.15f, 0.0f, 1.0f);
        // D006: mod wheel adds up to +0.4 maqam gravity at full wheel (stronger scale attraction; sensitivity 0.4)
        effectiveGravity = clamp(effectiveGravity + modWheelValue * 0.4f, 0.0f, 1.0f);

        // D002 mod matrix — apply per-block.
        // Destinations: 0=Off, 1=MaqamGravity, 2=LFORate, 3=Pitch, 4=AmpLevel, 5=DriftDepth
        {
            ModMatrix<4>::Sources mSrc;
            mSrc.lfo1       = 0.0f;   // Oracle LFO values are per-voice; use 0 at block level
            mSrc.lfo2       = 0.0f;
            mSrc.env        = 0.0f;
            mSrc.velocity   = 0.0f;
            mSrc.keyTrack   = 0.0f;
            mSrc.modWheel   = modWheelValue;
            mSrc.aftertouch = atPressure;
            float mDst[6]   = {};
            modMatrix.apply(mSrc, mDst);
            // dst 1: Maqam gravity offset (±0.4 range — matches the existing mod wheel sensitivity)
            effectiveGravity = clamp(effectiveGravity + mDst[1] * 0.4f, 0.0f, 1.0f);
            // dst 3: pitch offset in semitones (±12)
            oracleModPitchOffset   = mDst[3] * 12.0f;
            // dst 4: amplitude level offset
            oracleModLevelOffset   = mDst[4] * 0.5f;
            // dst 5: drift depth offset — capped to avoid instability
            effectiveDrift = clamp(effectiveDrift + mDst[5] * 0.3f, 0.0f, 1.0f);
        }

        float peakEnvelopeLevel = 0.0f;

        // ===== PER-SAMPLE RENDER LOOP =====
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            // Smooth stochastic parameters toward their targets to prevent zipper noise
            smoothedTimeStep += (effectiveTimeStep - smoothedTimeStep) * parameterSmoothingCoeff;
            smoothedAmpStep += (effectiveAmpStep - smoothedAmpStep) * parameterSmoothingCoeff;
            smoothedDistribution += (effectiveDistribution - smoothedDistribution) * parameterSmoothingCoeff;
            smoothedTimeStep = flushDenormal(smoothedTimeStep);
            smoothedAmpStep = flushDenormal(smoothedAmpStep);
            smoothedDistribution = flushDenormal(smoothedDistribution);

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // --- Voice-stealing crossfade (5ms) ---
                if (voice.fadingOut)
                {
                    voice.fadeGain -= voiceCrossfadeRate;
                    if (voice.fadeGain <= 0.0f)
                    {
                        voice.fadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // --- Glide (portamento) ---
                float frequency = voice.glide.process() * PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f + oracleModPitchOffset);

                // --- Envelopes ---
                float amplitudeLevel = voice.amplitudeEnvelope.process();
                float stochasticLevel = voice.stochasticEnvelope.process();

                if (!voice.amplitudeEnvelope.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Value = voice.lfo1.process() * lfo1DepthAmount;
                float lfo2Value = voice.lfo2.process() * lfo2DepthAmount;

                // LFO1 modulates time step (how much breakpoint positions drift)
                // LFO2 modulates amplitude step (how much breakpoint heights drift)
                // 0.3 scaling keeps LFO influence musical, not destructive
                float modulatedTimeStep = clamp(smoothedTimeStep + lfo1Value * 0.3f, 0.0f, 1.0f);
                float modulatedAmpStep = clamp(smoothedAmpStep + lfo2Value * 0.3f, 0.0f, 1.0f);

                // D001: velocity scales stochastic drift depth — louder notes produce
                // denser, more complex spectra as breakpoints evolve more aggressively.
                // velBrightness maps [0,1] velocity to [0.3,1.0] so soft notes still evolve.
                float velBrightness = 0.3f + 0.7f * voice.velocity;
                float velScaledDrift = clamp(effectiveDrift + velBrightness * velDriftDepth, 0.0f, 1.0f);

                // Stochastic depth: how much the breakpoints actually move.
                // Controlled by the stochastic envelope (fades in/out with note)
                // and the drift parameter (global evolution intensity, now velocity-modulated).
                float stochasticDepth = stochasticLevel * velScaledDrift;

                // --- Phase increment ---
                voice.wavePhaseIncrement = frequency / sampleRateFloat;

                // --- Advance waveform phase ---
                voice.wavePhase += voice.wavePhaseIncrement;

                // --- Cycle completion: the reef shifts ---
                // When phase wraps past 1.0, one complete waveform cycle has elapsed.
                // This is when GENDY evolves: breakpoints undergo their random walk,
                // producing a new waveform shape for the next cycle.
                if (voice.wavePhase >= 1.0f)
                {
                    voice.wavePhase -= 1.0f;

                    // Save last sample of previous cycle for crossfade smoothing
                    voice.prevCycleLastSample = voice.lastOutputL;
                    voice.cycleBlendCounter = OracleVoice::kCycleBlendSamples;

                    // Evolve breakpoints via stochastic random walk
                    evolveBreakpoints(voice, modulatedTimeStep * stochasticDepth, modulatedAmpStep * stochasticDepth,
                                      smoothedDistribution, effectiveElasticity, capturedBreakpointMod);
                }

                // --- Cubic Hermite interpolation across breakpoints ---
                // This is the hot path: reads the breakpoint constellation and
                // produces a smooth per-sample output via Catmull-Rom splines.
                float rawSampleL = interpolateBreakpoints(voice, voice.wavePhase);

                // --- Cycle boundary crossfade (prevent clicks at seam) ---
                if (voice.cycleBlendCounter > 0)
                {
                    float blendRatio = static_cast<float>(voice.cycleBlendCounter) /
                                       static_cast<float>(OracleVoice::kCycleBlendSamples);
                    rawSampleL = rawSampleL * (1.0f - blendRatio) + voice.prevCycleLastSample * blendRatio;
                    --voice.cycleBlendCounter;
                }

                // --- DC blocker (L channel) ---
                float outputL = voice.dcBlockerL.process(rawSampleL);

                // --- Stereo decorrelation ---
                // R channel reads the same breakpoints at a 1% phase offset,
                // creating subtle stereo width without a second oscillator.
                // 0.01 = 1% of waveform cycle — enough for spatial interest,
                // small enough to preserve mono compatibility.
                float rightChannelPhase = voice.wavePhase + 0.01f;
                if (rightChannelPhase >= 1.0f)
                    rightChannelPhase -= 1.0f;
                float rawSampleR = interpolateBreakpoints(voice, rightChannelPhase);
                if (voice.cycleBlendCounter > 0)
                {
                    float blendRatio = static_cast<float>(voice.cycleBlendCounter + 1) /
                                       static_cast<float>(OracleVoice::kCycleBlendSamples);
                    rawSampleR = rawSampleR * (1.0f - blendRatio) + voice.prevCycleLastSample * blendRatio;
                }
                float outputR = voice.dcBlockerR.process(rawSampleR);

                // --- Soft limiter (tanh saturation) ---
                // GENDY waveforms can exceed [-1, 1] due to Hermite overshoot
                // and coupling modulation. tanh provides a musical soft clip.
                outputL = fastTanh(outputL);
                outputR = fastTanh(outputR);

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float voiceGain = amplitudeLevel * voice.velocity * voice.fadeGain;
                outputL *= voiceGain;
                outputR *= voiceGain;

                // DENORMAL PROTECTION: After multiplication by near-zero envelope
                // values (during release tail), outputs can become denormalized.
                // This causes 10-100x CPU penalty on x86 processors due to
                // microcode traps. Flush to zero prevents the spike.
                outputL = flushDenormal(outputL);
                outputR = flushDenormal(outputR);

                voice.lastOutputL = outputL;
                voice.lastOutputR = outputR;

                mixL += outputL;
                mixR += outputR;

                peakEnvelopeLevel = std::max(peakEnvelopeLevel, amplitudeLevel);
            }

            // --- Master output ---
            const float effectiveMasterLevel = juce::jlimit(0.05f, 1.5f, masterLevel + oracleModLevelOffset);
            float finalL = mixL * effectiveMasterLevel;
            float finalR = mixR * effectiveMasterLevel;

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sampleIndex, finalL);
                buffer.addSample(1, sampleIndex, finalR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sampleIndex, (finalL + finalR) * 0.5f);
            }

            // Cache output for coupling reads (other engines access via getSampleForCoupling)
            if (sampleIndex < static_cast<int>(outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t>(sampleIndex)] = finalL;
                outputCacheR[static_cast<size_t>(sampleIndex)] = finalR;
            }
        }

        envelopeOutput = peakEnvelopeLevel;

        // Update active voice count — safe for UI thread reads via getActiveVoiceCount()
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active)
                ++count;
        activeVoiceCount_.store(count, std::memory_order_relaxed);

        silenceGate.analyzeBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
    }

    //==========================================================================
    // SynthEngine interface — Coupling (Symbiosis in the water column)
    //
    // Oracle participates in the XOceanus coupling ecosystem:
    //   Channel 0: Left audio output (post-processing)
    //   Channel 1: Right audio output (post-processing)
    //   Channel 2: Envelope follower (peak amplitude for sidechain coupling)
    //==========================================================================

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto index = static_cast<size_t>(sampleIndex);
        if (channel == 0 && index < outputCacheL.size())
            return outputCacheL[index];
        if (channel == 1 && index < outputCacheR.size())
            return outputCacheR[index];
        if (channel == 2)
            return envelopeOutput; // Envelope follower for sidechain
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
        case CouplingType::AudioToFM:
            // External audio perturbs breakpoint amplitudes — like an
            // earthquake shaking the reef's geological strata.
            // 0.5 scaling prevents coupling from overwhelming evolution.
            couplingBreakpointMod += amount * 0.5f;
            break;

        case CouplingType::AmpToFilter:
            // External amplitude modulates mirror barrier positions —
            // widens or narrows the space breakpoints can occupy.
            couplingBarrierMod += amount;
            break;

        case CouplingType::EnvToMorph:
            // External envelope drives distribution morph (Cauchy <-> Logistic).
            // 0.4 scaling keeps the blend musically useful.
            couplingDistributionMod += amount * 0.4f;
            break;

        default:
            break;
        }
    }

    //==========================================================================
    // SynthEngine interface — Parameters
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }
    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Core GENDY Parameters ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_breakpoints", 1}, "Oracle Breakpoints",
            juce::NormalisableRange<float>(8.0f, 32.0f, 1.0f), 16.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_timeStep", 1}, "Oracle Time Step",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_ampStep", 1}, "Oracle Amp Step",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_distribution", 1}, "Oracle Distribution",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_barrierElasticity", 1}, "Oracle Barrier Elasticity",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        // --- Maqam Parameters ---
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"oracle_maqam", 1}, "Oracle Maqam",
            juce::StringArray{"12-TET", "Rast", "Bayati", "Saba", "Hijaz", "Sikah", "Nahawand", "Kurd", "Ajam"}, 0));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_gravity", 1}, "Oracle Gravity",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // --- Evolution ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_drift", 1}, "Oracle Drift",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // --- Level ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_level", 1}, "Oracle Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        // --- Amp Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_ampAttack", 1}, "Oracle Amp Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_ampDecay", 1}, "Oracle Amp Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_ampSustain", 1}, "Oracle Amp Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_ampRelease", 1}, "Oracle Amp Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- Stochastic Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_stochEnvAttack", 1}, "Oracle Stoch Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.05f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_stochEnvDecay", 1}, "Oracle Stoch Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_stochEnvSustain", 1}, "Oracle Stoch Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_stochEnvRelease", 1}, "Oracle Stoch Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- LFO 1 ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_lfo1Rate", 1}, "Oracle LFO1 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_lfo1Depth", 1}, "Oracle LFO1 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"oracle_lfo1Shape", 1}, "Oracle LFO1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // --- LFO 2 ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_lfo2Rate", 1}, "Oracle LFO2 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_lfo2Depth", 1}, "Oracle LFO2 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"oracle_lfo2Shape", 1}, "Oracle LFO2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // --- Voice Parameters ---
        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"oracle_voiceMode", 1}, "Oracle Voice Mode",
                                                         juce::StringArray{"Mono", "Legato", "Poly4", "Poly8"}, 2));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_glide", 1}, "Oracle Glide",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // --- Macros ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_macroProphecy", 1}, "Oracle Macro PROPHECY",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_macroEvolution", 1}, "Oracle Macro EVOLUTION",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_macroGravity", 1}, "Oracle Macro GRAVITY",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oracle_macroDrift", 1}, "Oracle Macro DRIFT",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // D001: Velocity → stochastic drift depth.
        // Controls how much velocity scales breakpoint evolution intensity.
        // At 0.0 velocity has no timbral effect (amplitude-only). At 1.0 hard
        // strikes produce fully dense chaotic spectra; soft strikes evolve slowly.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"oracle_velDriftDepth", 1}, "Oracle Vel Drift",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        // D002 mod matrix — 4 user-configurable source→destination slots
        // Oracle has no filter — replace Filter Cutoff with Maqam Gravity (scale attraction strength)
        static const juce::StringArray kOracleModDests {"Off", "Maqam Gravity", "LFO Rate", "Pitch", "Amp Level",
                                                         "Drift Depth"};
        ModMatrix<4>::addParameters(params, "oracle_", "Oracle", kOracleModDests);
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pBreakpoints = apvts.getRawParameterValue("oracle_breakpoints");
        pTimeStep = apvts.getRawParameterValue("oracle_timeStep");
        pAmpStep = apvts.getRawParameterValue("oracle_ampStep");
        pDistribution = apvts.getRawParameterValue("oracle_distribution");
        pBarrierElasticity = apvts.getRawParameterValue("oracle_barrierElasticity");
        pMaqam = apvts.getRawParameterValue("oracle_maqam");
        pGravity = apvts.getRawParameterValue("oracle_gravity");
        pDrift = apvts.getRawParameterValue("oracle_drift");
        pLevel = apvts.getRawParameterValue("oracle_level");

        pAmpAttack = apvts.getRawParameterValue("oracle_ampAttack");
        pAmpDecay = apvts.getRawParameterValue("oracle_ampDecay");
        pAmpSustain = apvts.getRawParameterValue("oracle_ampSustain");
        pAmpRelease = apvts.getRawParameterValue("oracle_ampRelease");

        pStochEnvAttack = apvts.getRawParameterValue("oracle_stochEnvAttack");
        pStochEnvDecay = apvts.getRawParameterValue("oracle_stochEnvDecay");
        pStochEnvSustain = apvts.getRawParameterValue("oracle_stochEnvSustain");
        pStochEnvRelease = apvts.getRawParameterValue("oracle_stochEnvRelease");

        pLfo1Rate = apvts.getRawParameterValue("oracle_lfo1Rate");
        pLfo1Depth = apvts.getRawParameterValue("oracle_lfo1Depth");
        pLfo1Shape = apvts.getRawParameterValue("oracle_lfo1Shape");
        pLfo2Rate = apvts.getRawParameterValue("oracle_lfo2Rate");
        pLfo2Depth = apvts.getRawParameterValue("oracle_lfo2Depth");
        pLfo2Shape = apvts.getRawParameterValue("oracle_lfo2Shape");

        pVoiceMode = apvts.getRawParameterValue("oracle_voiceMode");
        pGlide = apvts.getRawParameterValue("oracle_glide");

        pMacroProphecy = apvts.getRawParameterValue("oracle_macroProphecy");
        pMacroEvolution = apvts.getRawParameterValue("oracle_macroEvolution");
        pMacroGravity = apvts.getRawParameterValue("oracle_macroGravity");
        pMacroDrift = apvts.getRawParameterValue("oracle_macroDrift");

        pVelDriftDepth = apvts.getRawParameterValue("oracle_velDriftDepth");
        modMatrix.attachParameters(apvts, "oracle_");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Oracle"; }

    juce::Colour getAccentColour() const override
    {
        // Prophecy Indigo #4B0082 — deep, ancient, resonant.
        // Chosen to evoke the depth of geological time and the mystery
        // of stochastic process. Lives at reef depth in the water column.
        return juce::Colour(0xFF4B0082);
    }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoiceCount_.load(std::memory_order_relaxed); }

private:
    SilenceGate silenceGate;

    //==========================================================================
    // Helper: safe parameter load
    //==========================================================================

    static float loadParam(std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // Maqam tuning — compute frequency for a MIDI note given maqam + gravity.
    //
    // When maqam is 0 (12-TET) or gravity is 0, returns standard 12-TET.
    // Otherwise, maps each chromatic note to its maqam-adjusted frequency
    // and blends between 12-TET and maqam based on gravity [0,1].
    //==========================================================================

    static float computeMaqamFreq(int midiNote, int maqamIdx, float gravity) noexcept
    {
        // 12-TET frequency
        float tetFreq = 440.0f * std::pow(2.0f, (static_cast<float>(midiNote) - 69.0f) / 12.0f);

        // If maqam is 0 (12-TET) or gravity is zero, use standard tuning
        if (maqamIdx <= 0 || maqamIdx > kNumMaqamat || gravity <= 0.0001f)
            return tetFreq;

        const auto& maq = kMaqamat[maqamIdx - 1];

        // Determine note position within octave
        // Use C as the maqam root (MIDI 60 = C4)
        int noteInOctave = ((midiNote % 12) + 12) % 12; // 0-11
        int octave = (midiNote / 12) - 1;

        // Convert chromatic position to cents from root
        float chromaticCents = static_cast<float>(noteInOctave) * 100.0f;

        // Find the maqam cent offset by interpolating between degrees
        float maqamCentOffset = chromaticCents; // default: same as 12-TET
        for (int d = 0; d < 7; ++d)
        {
            if (chromaticCents >= maq.cents[d] - 0.01f && chromaticCents <= maq.cents[d + 1] + 0.01f)
            {
                float range = maq.cents[d + 1] - maq.cents[d];
                float pos = chromaticCents - maq.cents[d];
                if (range > 0.001f)
                {
                    float t = pos / range;
                    maqamCentOffset = maq.cents[d] + t * (maq.cents[d + 1] - maq.cents[d]);
                }
                else
                {
                    maqamCentOffset = maq.cents[d];
                }
                break;
            }
        }

        // Compute maqam frequency from the C root of the same octave
        int rootMidi = (octave + 1) * 12; // C of this octave
        float rootFreq = 440.0f * std::pow(2.0f, (static_cast<float>(rootMidi) - 69.0f) / 12.0f);
        float maqamFreq = rootFreq * std::pow(2.0f, maqamCentOffset / 1200.0f);

        // Blend between 12-TET and maqam tuning via gravity
        return tetFreq + gravity * (maqamFreq - tetFreq);
    }

    //==========================================================================
    // Distribution sampling — morphable Cauchy/Logistic blend.
    //
    // This is Xenakis's key insight: the probability distribution that
    // governs the random walk determines the musical character of the sound.
    //
    //   distribution = 0.0 -> pure Cauchy (heavy-tailed, erratic, "prophecy")
    //     Cauchy distributions produce occasional extreme jumps — the oracle
    //     making sudden, dramatic proclamations.
    //
    //   distribution = 1.0 -> pure Logistic (smooth, predictable, "geology")
    //     Logistic distributions produce gentle, sigmoid-bounded walks — the
    //     reef shifting slowly over millennia.
    //
    // Xenakis used Cauchy exclusively in GENDY3. We add Logistic as the
    // tame counterpart, with smooth morphing between them.
    //==========================================================================

    static float sampleDistribution(Xorshift64& rng, float distribution) noexcept
    {
        float u = rng.uniform();

        // Clamp to (0.001, 0.999) to avoid singularities:
        // Cauchy's tan() explodes at u=0 and u=1,
        // Logistic's log(u/(1-u)) explodes at u=0 and u=1.
        u = std::max(0.001f, std::min(0.999f, u));

        // Cauchy distribution: tan(pi * (u - 0.5))
        // Heavy-tailed: most values near zero, but occasional wild spikes.
        // Clamped to [-10, 10] then scaled by 0.1 to normalize to ~[-1, 1].
        float cauchy = std::tan(kPI * (u - 0.5f));
        cauchy = std::max(-10.0f, std::min(10.0f, cauchy));
        cauchy *= 0.1f; // Normalize: raw Cauchy range is unbounded

        // Logistic distribution: log(u / (1 - u))
        // Smooth sigmoid-like tails, well-bounded. Most values cluster near zero.
        // Scaled by 0.15 to normalize to ~[-1, 1].
        float logistic = std::log(u / (1.0f - u));
        logistic *= 0.15f; // Normalize: raw Logistic range is roughly [-7, 7]

        // Crossfade between the two distributions
        return cauchy * (1.0f - distribution) + logistic * distribution;
    }

    //==========================================================================
    // Mirror barrier — elastic reflection at boundaries.
    //
    // Xenakis's GENDY used hard boundaries. We add elasticity for musical
    // control: the breakpoints bounce off the walls like billiard balls,
    // with configurable energy loss on each reflection.
    //
    // In the reef metaphor: coral polyps can only grow within the water
    // column. When they reach a boundary (surface or seafloor), they
    // reflect back. Elasticity = how much momentum they retain.
    //
    //   elasticity = 0.0 -> hard clamp (stuck at boundary, no bounce)
    //   elasticity = 1.0 -> full elastic reflection (energy preserved)
    //
    // Max 8 iterations prevents infinite loops from extreme overshoot.
    //==========================================================================

    static float mirrorBarrier(float value, float lowerBound, float upperBound, float elasticity) noexcept
    {
        float range = upperBound - lowerBound;
        if (range <= 0.0001f)
            return (lowerBound + upperBound) * 0.5f;

        int iterations = 0;
        while ((value < lowerBound || value > upperBound) && iterations < 8)
        {
            if (value < lowerBound)
            {
                float overshoot = lowerBound - value;
                value = lowerBound + overshoot * elasticity;
            }
            else if (value > upperBound)
            {
                float overshoot = value - upperBound;
                value = upperBound - overshoot * elasticity;
            }
            ++iterations; // Safety: max 8 reflections per call
        }

        // Final hard clamp as safety net (in case elasticity causes oscillation)
        return std::max(lowerBound, std::min(upperBound, value));
    }

    //==========================================================================
    // Evolve breakpoints — stochastic random walk, once per waveform cycle.
    //
    // This is the heart of GENDY: the reef shifting. Each breakpoint's time
    // and amplitude are perturbed by samples from the morphable distribution,
    // then reflected via mirror barriers. After perturbation, breakpoints
    // are sorted by time and the time span is normalized to [0, 1].
    //
    // Called once per waveform cycle (not per sample), so computational
    // cost scales with frequency, not with sample rate.
    //==========================================================================

    void evolveBreakpoints(OracleVoice& voice, float timeStep, float ampStep, float distribution, float elasticity,
                           float breakpointCouplingMod) noexcept
    {
        int breakpointCount = voice.numBreakpoints;

        // Quadratic scaling provides a gradual onset of chaos:
        // At low values (0.1), step^2 = 0.01 — almost no movement.
        // At high values (0.9), step^2 = 0.81 — aggressive drift.
        // The 0.15 and 0.3 multipliers are empirically tuned to keep
        // time drift subtle (waveform shape preservation) while allowing
        // amplitude drift to be more dramatic (timbral evolution).
        float scaledTimeStep = timeStep * timeStep * 0.15f;   // Time drifts gently
        float scaledAmplitudeStep = ampStep * ampStep * 0.3f; // Amplitude drifts boldly

        for (int i = 0; i < breakpointCount; ++i)
        {
            auto& breakpoint = voice.breakpoints[i];

            // Random walk on time offset (horizontal position in cycle)
            float timeDelta = sampleDistribution(voice.rng, distribution) * scaledTimeStep;
            breakpoint.timeOffset += timeDelta;

            // Random walk on amplitude (vertical position / waveform height)
            float ampDelta = sampleDistribution(voice.rng, distribution) * scaledAmplitudeStep;
            breakpoint.amplitude += ampDelta;

            // Apply coupling modulation to amplitude.
            // 0.05 scaling keeps external influence subtle — coupling should
            // color the evolution, not overwhelm it.
            breakpoint.amplitude += breakpointCouplingMod * 0.05f;

            // Mirror barrier for time position [0, 1]
            breakpoint.timeOffset = mirrorBarrier(breakpoint.timeOffset, 0.0f, 1.0f, elasticity);

            // Mirror barrier for amplitude [-1, 1]
            breakpoint.amplitude = mirrorBarrier(breakpoint.amplitude, -1.0f, 1.0f, elasticity);
        }

        // Sort breakpoints by time offset to maintain waveform ordering.
        // Without sorting, breakpoints could cross each other and create
        // self-intersecting waveforms (which produce harsh aliasing).
        // Insertion sort — O(N) for nearly-sorted data, audio-thread safe (no allocation)
        for (int i = 1; i < breakpointCount; ++i)
        {
            auto key = voice.breakpoints[i];
            int j = i - 1;
            while (j >= 0 && voice.breakpoints[j].timeOffset > key.timeOffset)
            {
                voice.breakpoints[j + 1] = voice.breakpoints[j];
                --j;
            }
            voice.breakpoints[j + 1] = key;
        }

        // Normalize time span: first breakpoint at 0, last at ~1.
        // This ensures the waveform always fills exactly one cycle period.
        if (breakpointCount >= 2)
        {
            float timeMin = voice.breakpoints[0].timeOffset;
            float timeMax = voice.breakpoints[breakpointCount - 1].timeOffset;
            float timeSpan = timeMax - timeMin;

            if (timeSpan > 0.001f)
            {
                for (int i = 0; i < breakpointCount; ++i)
                {
                    voice.breakpoints[i].timeOffset = (voice.breakpoints[i].timeOffset - timeMin) / timeSpan;
                }
            }
            else
            {
                // Degenerate case: all breakpoints collapsed to same time.
                // Redistribute evenly to recover a usable waveform.
                for (int i = 0; i < breakpointCount; ++i)
                {
                    voice.breakpoints[i].timeOffset = static_cast<float>(i) / static_cast<float>(breakpointCount - 1);
                }
            }
        }

        // Anti-aliasing smoothing for high breakpoint counts (> 20).
        // When breakpoints are dense, rapid amplitude changes between
        // neighbors produce harmonics above Nyquist. A 3-tap [0.25, 0.5, 0.25]
        // kernel acts as a gentle lowpass on the waveform shape.
        if (breakpointCount > 20)
        {
            float previousAmplitude = voice.breakpoints[0].amplitude;
            for (int i = 1; i < breakpointCount - 1; ++i)
            {
                float currentAmplitude = voice.breakpoints[i].amplitude;
                float nextAmplitude = voice.breakpoints[i + 1].amplitude;
                float smoothed = previousAmplitude * 0.25f // 3-tap triangle kernel
                                 + currentAmplitude * 0.50f + nextAmplitude * 0.25f;
                previousAmplitude = currentAmplitude;
                voice.breakpoints[i].amplitude = smoothed;
            }
        }
    }

    //==========================================================================
    // Cubic Hermite interpolation between breakpoints (Catmull-Rom).
    //
    // This is the per-sample HOT PATH — called once (or twice for stereo
    // decorrelation) for every sample of every active voice. Uses Catmull-Rom
    // tangent estimation for C1-continuous interpolation between the discrete
    // breakpoint constellation.
    //
    // Hermite basis functions:
    //   h00(t) =  2t^3 - 3t^2 + 1    (value at start)
    //   h10(t) =   t^3 - 2t^2 + t    (tangent at start)
    //   h01(t) = -2t^3 + 3t^2        (value at end)
    //   h11(t) =   t^3 -  t^2        (tangent at end)
    //
    // Output clamped to [-1.5, 1.5] to allow natural Hermite overshoot
    // while preventing extreme spikes. The soft limiter downstream
    // handles the final [-1, 1] constraint.
    //==========================================================================

    float interpolateBreakpoints(const OracleVoice& voice, float phase) const noexcept
    {
        int breakpointCount = voice.numBreakpoints;
        if (breakpointCount < 2)
            return 0.0f;

        // Clamp phase to [0, 1) — 0.99999 prevents indexing past last segment
        phase = std::max(0.0f, std::min(0.99999f, phase));

        // Find the segment: breakpoint[i] <= phase < breakpoint[i+1]
        int segmentIndex = breakpointCount - 2; // Default to last segment
        for (int i = 0; i < breakpointCount - 1; ++i)
        {
            if (phase < voice.breakpoints[i + 1].timeOffset)
            {
                segmentIndex = i;
                break;
            }
        }

        // Get 4 control points for cubic Hermite (Catmull-Rom needs p[i-1..i+2])
        int idx0 = std::max(0, segmentIndex - 1);                   // Previous point
        int idx1 = segmentIndex;                                    // Segment start
        int idx2 = std::min(breakpointCount - 1, segmentIndex + 1); // Segment end
        int idx3 = std::min(breakpointCount - 1, segmentIndex + 2); // Next point

        float time1 = voice.breakpoints[idx1].timeOffset;
        float time2 = voice.breakpoints[idx2].timeOffset;

        float amp0 = voice.breakpoints[idx0].amplitude;
        float amp1 = voice.breakpoints[idx1].amplitude;
        float amp2 = voice.breakpoints[idx2].amplitude;
        float amp3 = voice.breakpoints[idx3].amplitude;

        // Local parameter t within segment [time1, time2]
        float segmentLength = time2 - time1;
        float localT = (segmentLength > 0.00001f) ? (phase - time1) / segmentLength : 0.0f;
        localT = std::max(0.0f, std::min(1.0f, localT));

        // Catmull-Rom tangent estimation: finite differences scaled to segment length
        float time0 = voice.breakpoints[idx0].timeOffset;
        float time3 = voice.breakpoints[idx3].timeOffset;

        float tangent1 = 0.0f; // Tangent at segment start
        float tangent2 = 0.0f; // Tangent at segment end

        // Tangent at p1: (amp2 - amp0) / (time2 - time0), scaled by segment length
        {
            float timeSpan = time2 - time0;
            if (timeSpan > 0.00001f)
                tangent1 = (amp2 - amp0) / timeSpan * segmentLength;
        }

        // Tangent at p2: (amp3 - amp1) / (time3 - time1), scaled by segment length
        {
            float timeSpan = time3 - time1;
            if (timeSpan > 0.00001f)
                tangent2 = (amp3 - amp1) / timeSpan * segmentLength;
        }

        // Hermite basis functions (cubic polynomials in localT)
        float tSquared = localT * localT;
        float tCubed = tSquared * localT;

        float h00 = 2.0f * tCubed - 3.0f * tSquared + 1.0f; // Value weight at start
        float h10 = tCubed - 2.0f * tSquared + localT;      // Tangent weight at start
        float h01 = -2.0f * tCubed + 3.0f * tSquared;       // Value weight at end
        float h11 = tCubed - tSquared;                      // Tangent weight at end

        float result = h00 * amp1 + h10 * tangent1 + h01 * amp2 + h11 * tangent2;

        // Clamp to [-1.5, 1.5]: allow natural Hermite overshoot but prevent
        // extreme spikes. The downstream tanh soft limiter handles final clamping.
        return std::max(-1.5f, std::min(1.5f, result));
    }

    //==========================================================================
    // MIDI note handling — voice allocation and initialization.
    //==========================================================================

    void noteOn(int noteNumber, float velocity, int maxPolyphony, bool isMonophonic, bool isLegatoMode,
                float glideCoeff, int breakpointCount, int maqamIdx, float gravity, float ampAttack, float ampDecay,
                float ampSustain, float ampRelease, float stochAttack, float stochDecay, float stochSustain,
                float stochRelease, float lfo1Rate, float lfo1Depth, int lfo1Shape, float lfo2Rate, float lfo2Depth,
                int lfo2Shape)
    {
        float frequency = computeMaqamFreq(noteNumber, maqamIdx, gravity);

        if (isMonophonic)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.glide.setTarget(frequency);
            voice.glide.setCoeff(glideCoeff);

            if (isLegatoMode && wasActive)
            {
                // Legato: glide to new note without retriggering envelopes.
                // The waveform continues its stochastic evolution uninterrupted.
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
            }
            else
            {
                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.startTime = voiceCounter++;
                voice.glide.snapTo(frequency);
                voice.glide.setCoeff(glideCoeff);
                voice.wavePhase = 0.0f;
                voice.fadingOut = false;
                voice.fadeGain = 1.0f;
                voice.cycleBlendCounter = 0;

                // Seed PRNG from note number for deterministic variation.
                // 2654435761 is the Knuth multiplicative hash constant (golden ratio * 2^32),
                // chosen for good distribution of sequential inputs.
                voice.rng.seed(static_cast<uint64_t>(noteNumber) * 2654435761ULL + 1);

                voice.initBreakpoints(breakpointCount);

                voice.amplitudeEnvelope.setParams(ampAttack, ampDecay, ampSustain, ampRelease, sampleRateFloat);
                voice.amplitudeEnvelope.noteOn();
                voice.stochasticEnvelope.setParams(stochAttack, stochDecay, stochSustain, stochRelease,
                                                   sampleRateFloat);
                voice.stochasticEnvelope.noteOn();

                voice.lfo1.setRate(lfo1Rate, sampleRateFloat);
                voice.lfo1.setShape(lfo1Shape);
                voice.lfo2.setRate(lfo2Rate, sampleRateFloat);
                voice.lfo2.setShape(lfo2Shape);

                voice.dcBlockerL.prepare(sampleRateFloat);
                voice.dcBlockerL.reset();
                voice.dcBlockerR.prepare(sampleRateFloat);
                voice.dcBlockerR.reset();
            }
            return;
        }

        // ----- Polyphonic mode -----
        int voiceIndex = VoiceAllocator::findFreeVoice(voices, std::min(maxPolyphony, kMaxVoices));
        auto& voice = voices[static_cast<size_t>(voiceIndex)];

        // If stealing an active voice, initiate a crossfade to prevent clicks
        if (voice.active)
        {
            voice.fadingOut = true;
            voice.fadeGain = std::min(voice.fadeGain, 0.5f); // Start fade from at most 50%
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.glide.snapTo(frequency); // No glide in poly mode (instant pitch)
        voice.wavePhase = 0.0f;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;
        voice.cycleBlendCounter = 0;

        // Seed PRNG from note number + voice counter for unique per-voice variation.
        // 2654435761 is the Knuth multiplicative hash constant (golden ratio * 2^32).
        voice.rng.seed(static_cast<uint64_t>(noteNumber) * 2654435761ULL + voiceCounter);

        voice.initBreakpoints(breakpointCount);

        voice.amplitudeEnvelope.setParams(ampAttack, ampDecay, ampSustain, ampRelease, sampleRateFloat);
        voice.amplitudeEnvelope.noteOn();
        voice.stochasticEnvelope.setParams(stochAttack, stochDecay, stochSustain, stochRelease, sampleRateFloat);
        voice.stochasticEnvelope.noteOn();

        voice.lfo1.setRate(lfo1Rate, sampleRateFloat);
        voice.lfo1.setShape(lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate(lfo2Rate, sampleRateFloat);
        voice.lfo2.setShape(lfo2Shape);
        voice.lfo2.reset();

        voice.dcBlockerL.prepare(sampleRateFloat);
        voice.dcBlockerL.reset();
        voice.dcBlockerR.prepare(sampleRateFloat);
        voice.dcBlockerR.reset();
    }

    void noteOff(int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                voice.amplitudeEnvelope.noteOff();
                voice.stochasticEnvelope.noteOff();
            }
        }
    }

    //==========================================================================
    //
    //  MEMBER DATA
    //
    //==========================================================================

    // --- Audio engine state ---
    double sampleRateDouble = 0.0;  // Sentinel: must be set by prepare() before use
    float sampleRateFloat = 0.0f;  // Sentinel: must be set by prepare() before use
    float parameterSmoothingCoeff = 0.1f; // 5ms time constant for zipper-free parameter changes
    float voiceCrossfadeRate = 0.01f;     // 5ms gain ramp for voice stealing

    // --- Voice pool ---
    std::array<OracleVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0; // Monotonic counter for LRU voice stealing
    // activeVoiceCount_ promoted to base class std::atomic<int> — UI-safe read via getActiveVoiceCount()

    // --- Smoothed control parameters (updated per-sample) ---
    float smoothedTimeStep = 0.3f;     // Smoothed time step amount
    float smoothedAmpStep = 0.3f;      // Smoothed amplitude step amount
    float smoothedDistribution = 0.5f; // Smoothed Cauchy/Logistic morph position

    // D006: aftertouch — pressure increases GENDY drift (more stochastic chaos on pressure)
    PolyAftertouch aftertouch;

    // D006: mod wheel — CC1 increases Maqam gravity (stronger scale attraction with wheel; sensitivity 0.4)
    float modWheelValue = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // --- Coupling accumulators (reset each block) ---
    float envelopeOutput = 0.0f;          // Peak envelope for sidechain coupling
    float couplingBreakpointMod = 0.0f;   // AudioToFM: external audio perturbs breakpoints
    float couplingBarrierMod = 0.0f;      // AmpToFilter: external amplitude shifts barriers
    float couplingDistributionMod = 0.0f; // EnvToMorph: external envelope drives distribution blend

    // --- Output cache for coupling reads ---
    // Other engines read Oracle's output via getSampleForCoupling().
    // These buffers are written during renderBlock and read between blocks.
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // --- Cached APVTS parameter pointers (ParamSnapshot pattern) ---
    // Set once in attachParameters(), read per block in renderBlock().
    // Atomic pointers provide lock-free access from the audio thread.

    // Core GENDY parameters
    std::atomic<float>* pBreakpoints = nullptr;       // Number of breakpoints (8-32)
    std::atomic<float>* pTimeStep = nullptr;          // Time offset random walk step size
    std::atomic<float>* pAmpStep = nullptr;           // Amplitude random walk step size
    std::atomic<float>* pDistribution = nullptr;      // Cauchy (0) <-> Logistic (1) morph
    std::atomic<float>* pBarrierElasticity = nullptr; // Mirror barrier bounce energy [0, 1]
    std::atomic<float>* pMaqam = nullptr;             // Maqam selection (0=12-TET, 1-8=maqamat)
    std::atomic<float>* pGravity = nullptr;           // 12-TET (0) <-> Maqam (1) tuning blend
    std::atomic<float>* pDrift = nullptr;             // Overall stochastic evolution intensity
    std::atomic<float>* pLevel = nullptr;             // Master output level

    // Amplitude envelope
    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;

    // Stochastic evolution envelope
    std::atomic<float>* pStochEnvAttack = nullptr;
    std::atomic<float>* pStochEnvDecay = nullptr;
    std::atomic<float>* pStochEnvSustain = nullptr;
    std::atomic<float>* pStochEnvRelease = nullptr;

    // LFO 1 (modulates time step)
    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;

    // LFO 2 (modulates amplitude step)
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;

    // Voice configuration
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;

    // Macros
    std::atomic<float>* pMacroProphecy = nullptr;  // Pushes toward erratic, prophetic behavior
    std::atomic<float>* pMacroEvolution = nullptr; // Accelerates stochastic evolution
    std::atomic<float>* pMacroGravity = nullptr;   // Pulls tuning toward maqam system
    std::atomic<float>* pMacroDrift = nullptr;     // Increases overall drift and motion

    // D001: velocity-to-timbre
    std::atomic<float>* pVelDriftDepth = nullptr; // Velocity → stochastic drift depth

    // D002 mod matrix — 4-slot configurable modulation routing
    ModMatrix<4> modMatrix;
    float oracleModPitchOffset  = 0.0f;
    float oracleModLevelOffset  = 0.0f;
};

} // namespace xoceanus
