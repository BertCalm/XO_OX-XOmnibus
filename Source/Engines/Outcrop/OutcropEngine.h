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
#include <iterator>
#include <vector>

namespace xoceanus
{

//==============================================================================
//
//  O U T C R O P   E N G I N E
//  Wave-Terrain Synthesis
//
//  Gallery code:  OUTCROP   |  Accent: Mountain Moss #5B6F57  |  Prefix: outc_
//
//  XO_OX Aquatic Identity: A submerged ridge surfacing above the tideline —
//  exposed stone sculpted by slow pressure.  The orbit is a current tracing
//  the rock; the rock is the timbre.
//
//  Architecture: 2D parametric orbit traces a 3D height field.  The height
//  h(x, y) at the orbit's current position is the output sample.  Different
//  orbit shapes (circle, Lissajous, rose, spirograph, figure-8, folded chaos)
//  intersect different terrain modes (peaks, ridges, lattice, saddle, mixed,
//  folded) to produce timbres you can literally hear the geometry of — ridges
//  become sidebands, saddle points cause crossfades, isolated peaks click
//  like grains.
//
//  References:
//    - Richard Bidlack, "Chaotic Systems as Simple (but Complex) Compositional
//      Algorithms" (Computer Music Journal, 1992) — orbital waveshaping
//    - Dan Slater, "Chaotic Sound Synthesis" (CMJ, 1998)
//    - Curtis Roads, "The Computer Music Tutorial" (1996) — wave-terrain
//
//  Doctrine Compliance:
//    D001: velocity → filter cutoff + peak amplitude scaling
//    D002: 2 LFOs + mod wheel + aftertouch + 4 macros + 4 mod matrix slots
//    D003: geometric synthesis (terrain/orbit functions are closed-form)
//    D004: every parameter wires into DSP
//    D005: LFO rate floor 0.005 Hz (inherited from StandardLFO)
//    D006: velocity→timbre + mod wheel (CC1) + aftertouch (channel pressure)
//
//  Signal Flow (per voice):
//    note-on → orbit freq = midi→Hz
//      ↓
//    orbit(phase, shape, ratio, jitter) → (x, y) ∈ [-R, +R]²
//      ↓
//    terrain(x, y, mode, peaks/ridges/roughness) → h ∈ [-1, +1]
//      ↓
//    voice amp env × h  →  accumulate into block buffer
//      ↓
//    Sum all voices → stereo split (pan from orbit x) → CytomicSVF → Output
//
//==============================================================================

static constexpr int   kOutcropMaxVoices = 8;
static constexpr float kOutcropTwoPi     = 6.28318530717958647692f;
static constexpr float kOutcropInvTwoPi  = 1.0f / kOutcropTwoPi;
static constexpr float kOutcropPiOver4   = 0.7853981633974483f; // π/4 rad
static constexpr float kOutcropDenormalThreshold = 1e-20f;

static inline void outcropFlushDenormal(float& x) noexcept
{
    if (std::fabs(x) < kOutcropDenormalThreshold) x = 0.0f;
}

//==============================================================================
//  Voice — one note, one orbit
//==============================================================================
struct OutcropVoice
{
    bool  active      = false;
    bool  releasing   = false;
    int   note        = -1;
    float velocity    = 0.0f;

    // Orbit phase in radians [0, 2π).  Advanced each sample by orbitFreq / sr.
    float orbitPhase  = 0.0f;
    float orbitFreq   = 110.0f;
    float glideFreq   = 110.0f;  // smoothed target

    // Per-voice deterministic jitter PRNG (Knuth LCG).
    uint32_t jitterState = 0x12345678u;

    // Per-voice envelopes.
    StandardADSR ampEnv;
    StandardADSR fltEnv;

    // Per-voice filter (state-variable, post-sum would lose polyphonic character).
    CytomicSVF filter;

    // P1-1 (#1126): voice-steal fade — when a voice is stolen, rampSamples counts down
    // from a short fade length, multiplying output by (rampSamples / rampTotal) to
    // prevent clicks. A new note is started once the ramp expires.
    int   stealRampSamples = 0; // samples remaining in fade-out (0 = not fading)
    float stealRampTotal   = 1.0f; // total fade length (set from SR at steal time)

    void reset() noexcept
    {
        active = false;
        releasing = false;
        note = -1;
        velocity = 0.0f;
        orbitPhase = 0.0f;
        stealRampSamples = 0;
        stealRampTotal   = 1.0f;
        ampEnv.kill();
        fltEnv.kill();
        filter.reset();
    }

    // Fast xorshift step (deterministic, no allocation).
    float nextJitter() noexcept
    {
        uint32_t x = jitterState;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        jitterState = x;
        // Convert to bipolar [-1, 1]: reinterpret as signed so the MSB is the
        // sign bit, giving a symmetric distribution.
        return static_cast<float>(static_cast<int32_t>(x)) * (1.0f / 2147483648.0f);
    }
};

//==============================================================================
//
//  OutcropEngine — Wave-Terrain Synthesis
//
//  Parameter prefix: outc_
//
//==============================================================================
class OutcropEngine : public SynthEngine
{
public:
    //==========================================================================
    //  P A R A M E T E R   R E G I S T R A T I O N
    //==========================================================================
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return juce::AudioProcessorValueTreeState::ParameterLayout(
            std::make_move_iterator(params.begin()),
            std::make_move_iterator(params.end()));
    }

    //==========================================================================
    //  I D E N T I T Y
    //==========================================================================
    juce::String  getEngineId()     const override { return "Outcrop"; }
    juce::Colour  getAccentColour() const override { return juce::Colour(0xFF5B6F57); }
    int           getMaxVoices()    const override { return kOutcropMaxVoices; }

    //==========================================================================
    //  L I F E C Y C L E
    //==========================================================================
    void prepare(double sampleRate, int maxBlockSize) override;
    void releaseResources() override {}
    void reset() override;

    //==========================================================================
    //  P A R A M E T E R   A T T A C H
    //==========================================================================
    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override;

    //==========================================================================
    //  R E N D E R
    //==========================================================================
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override;

    //==========================================================================
    //  C O U P L I N G
    //==========================================================================
    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0 || sampleIndex >= (int)couplingCacheL.size()) return 0.0f;
        return (channel == 1) ? couplingCacheR[(size_t)sampleIndex]
                              : couplingCacheL[(size_t)sampleIndex];
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override;

private:
    //==========================================================================
    //  D S P :   O R B I T   &   T E R R A I N
    //==========================================================================
    struct OrbitPos { float x; float y; };

    // Return (x, y) ∈ [-1, +1]² for the given orbit phase, shape, and ratio.
    static OrbitPos computeOrbit(int shape, float phase, float ratio, float phaseOffset) noexcept;

    // Evaluate terrain height h(x, y) ∈ [-1, +1] for the given mode.
    // peakH, peakS, ridgeD, ridgeF, ridgeA, rough all in their parameter ranges.
    static float evaluateTerrain(int mode, float x, float y,
                                 float peakH, float peakS,
                                 float ridgeD, float ridgeF, float cosRidgeA, float sinRidgeA,
                                 float rough) noexcept;

    //==========================================================================
    //  V O I C E   M A N A G E M E N T
    //==========================================================================
    int  findFreeVoice() noexcept;
    int  stealVoice() noexcept;
    void startVoice(int noteNum, float vel, bool legato);
    void releaseVoicesForNote(int noteNum);

    //==========================================================================
    //  P A R A M E T E R   P O I N T E R S
    //==========================================================================
    // Terrain (A)
    std::atomic<float>* pTerrainType = nullptr;
    std::atomic<float>* pPeakHeight  = nullptr;
    std::atomic<float>* pPeakSpread  = nullptr;
    std::atomic<float>* pRidgeDepth  = nullptr;
    std::atomic<float>* pRidgeFreq   = nullptr;
    std::atomic<float>* pRidgeAngle  = nullptr;
    std::atomic<float>* pRoughness   = nullptr;

    // Orbit (B)
    std::atomic<float>* pOrbitShape  = nullptr;
    std::atomic<float>* pOrbitRadius = nullptr;
    std::atomic<float>* pOrbitRatio  = nullptr;
    std::atomic<float>* pOrbitPhase  = nullptr;
    std::atomic<float>* pOrbitJitter = nullptr;

    // Filter (C)
    std::atomic<float>* pFltCutoff   = nullptr;
    std::atomic<float>* pFltReso     = nullptr;
    std::atomic<float>* pFltType     = nullptr;
    std::atomic<float>* pFltEnvAmt   = nullptr;
    std::atomic<float>* pFltKeyTrack = nullptr;
    std::atomic<float>* pFltVelTrack = nullptr; // D001

    // Amp env (D)
    std::atomic<float>* pAmpAtk = nullptr;
    std::atomic<float>* pAmpDec = nullptr;
    std::atomic<float>* pAmpSus = nullptr;
    std::atomic<float>* pAmpRel = nullptr;

    // Filter env (E)
    std::atomic<float>* pFenvAtk = nullptr;
    std::atomic<float>* pFenvDec = nullptr;
    std::atomic<float>* pFenvSus = nullptr;
    std::atomic<float>* pFenvRel = nullptr;

    // LFOs (F)
    std::atomic<float>* pLfo1Rate  = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo1Tgt   = nullptr;
    std::atomic<float>* pLfo2Rate  = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;
    std::atomic<float>* pLfo2Tgt   = nullptr;

    // Macros (H)
    std::atomic<float>* pMacro1 = nullptr;
    std::atomic<float>* pMacro2 = nullptr;
    std::atomic<float>* pMacro3 = nullptr;
    std::atomic<float>* pMacro4 = nullptr;

    // Voice (I)
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide     = nullptr;

    // Output (J)
    std::atomic<float>* pLevel     = nullptr;

    // Mod matrix
    ModMatrix<4> modMatrix;

    //==========================================================================
    //  S T A T E
    //==========================================================================
    float sampleRateF = 44100.0f;
    int   blockCap    = 512;

    std::array<OutcropVoice, kOutcropMaxVoices> voices;

    // LFOs (block-rate)
    StandardLFO lfo1;
    StandardLFO lfo2;

    // MIDI state
    float modWheel     = 0.0f;
    float aftertouch   = 0.0f;
    float pitchBendNorm= 0.0f;

    // Coupling accumulators (block-level mod applied to next block)
    float couplingFilterMod = 0.0f;
    float couplingFmMod     = 0.0f;  // phase deviation
    float couplingRateMod   = 0.0f;  // orbit rate scalar
    float couplingBlendMod  = 0.0f;  // terrain bias

    // Coupling output cache (stereo, per-sample).
    std::vector<float> couplingCacheL;
    std::vector<float> couplingCacheR;

    // Glide coefficient (per block).
    float glideCoeff = 1.0f;

    bool  sustainPedal  = false;   // CC64 hold

    // Mono/legato tracking.
    int lastHeldNote = -1;
};

//==============================================================================
//  I N L I N E   I M P L E M E N T A T I O N
//==============================================================================

// NOTE: All inline definitions live in this header to match the fleet
// convention (DSP in .h, .cpp is a one-line stub).  Split across helpers to
// keep each function readable.

// -----------------------------------------------------------------------------
//  O R B I T   E V A L
// -----------------------------------------------------------------------------
inline OutcropEngine::OrbitPos OutcropEngine::computeOrbit(int shape, float phase,
                                                            float ratio, float phaseOffset) noexcept
{
    const float p  = phase;
    const float p2 = phase * ratio + phaseOffset;
    OrbitPos o{0.0f, 0.0f};
    // All trig uses fastCos/fastSin (0.002%/0.01% error — inaudible for orbit shapes).
    switch (shape)
    {
        case 0: // Circle
            o.x = fastCos(p);
            o.y = fastSin(p);
            break;
        case 1: // Lissajous
            o.x = fastCos(p);
            o.y = fastSin(p2);
            break;
        case 2: // Rose r = cos(k*θ)
        {
            const float r = fastCos(p2);
            o.x = r * fastCos(p);
            o.y = r * fastSin(p);
            break;
        }
        case 3: // Spirograph (simplified hypotrochoid)
            o.x = 0.7f * fastCos(p) + 0.3f * fastCos(p2);
            o.y = 0.7f * fastSin(p) - 0.3f * fastSin(p2);
            break;
        case 4: // Figure-8
            o.x = fastCos(p);
            o.y = fastSin(2.0f * p);
            break;
        case 5: // Folded chaos (phase-modulated)
            o.x = fastCos(p + 0.5f * fastSin(p2));
            o.y = fastSin(p + 0.5f * fastCos(p2));
            break;
        default:
            o.x = fastCos(p);
            o.y = fastSin(p);
            break;
    }
    return o;
}

// -----------------------------------------------------------------------------
//  T E R R A I N   E V A L
// -----------------------------------------------------------------------------
inline float OutcropEngine::evaluateTerrain(int mode, float x, float y,
                                             float peakH, float peakS,
                                             float ridgeD, float ridgeF, float cosRidgeA, float sinRidgeA,
                                             float rough) noexcept
{
    auto peaks = [&]() noexcept
    {
        // Four Gaussian peaks at (±0.5, ±0.5) — polynomial approx of exp(-d²/σ²).
        // h = peakH * Σ max(0, 1 - d²/σ²)² — compact-support, cheaper than exp.
        const float sigma = std::max(0.1f, peakS);
        const float invS2 = 1.0f / (sigma * sigma);
        float sum = 0.0f;
        const float ax[4] = { -0.5f,  0.5f, -0.5f,  0.5f };
        const float ay[4] = { -0.5f, -0.5f,  0.5f,  0.5f };
        for (int i = 0; i < 4; ++i)
        {
            const float dx = x - ax[i];
            const float dy = y - ay[i];
            const float d2 = (dx * dx + dy * dy) * invS2;
            if (d2 < 1.0f)
            {
                const float k = 1.0f - d2;
                sum += k * k;
            }
        }
        return peakH * (sum * 0.5f - 0.5f); // bias toward bipolar
    };

    auto ridges = [&]() noexcept
    {
        // P1-3 (#1126): ca/sa precomputed per-block — no per-sample trig
        const float u  = x * cosRidgeA + y * sinRidgeA;
        return ridgeD * fastSin(ridgeF * u);
    };

    auto lattice = [&]() noexcept
    {
        // Scale by ridgeD so Ridge Depth has audible effect in Lattice mode.
        const float depth = std::max(0.1f, ridgeD);
        return fastCos(ridgeF * x) * fastCos(ridgeF * y) * depth;
    };

    auto saddle = [&]() noexcept
    {
        // Smooth hyperbolic saddle.  Scale coefficient by 1/R² to preserve
        // the saddle shape at all orbit radii (at large radius x*y → big,
        // making tanh saturate to ±1 and losing the saddle geometry).
        const float rSq = std::max(0.01f, peakS * peakS); // reuse peakS as proxy for radius
        return fastTanh((1.8f / rSq) * x * y);
    };

    auto folded = [&]() noexcept
    {
        return fastCos(ridgeF * x + fastSin(ridgeF * y));
    };

    float h = 0.0f;
    switch (mode)
    {
        case 0: h = peaks();                                break;
        case 1: h = ridges();                               break;
        case 2: h = lattice();                              break;
        case 3: h = saddle();                               break;
        case 4: h = 0.5f * peaks() + 0.5f * ridges();       break;
        case 5: h = folded();                               break;
        default: h = 0.0f;                                  break;
    }

    // Roughness — cheap high-freq perturbation (periodic, deterministic).
    if (rough > 0.0f)
        h += rough * fastSin(11.0f * x + 7.0f * y) * 0.5f;

    // Soft saturation via fastTanh — avoids discontinuous hard-clip artifacts
    // when roughness pushes the terrain past ±1 on steep ridges.
    return fastTanh(h);
}

// -----------------------------------------------------------------------------
//  V O I C E   A L L O C A T I O N
// -----------------------------------------------------------------------------
inline int OutcropEngine::findFreeVoice() noexcept
{
    for (int i = 0; i < kOutcropMaxVoices; ++i)
        if (!voices[i].active) return i;
    return -1;
}

inline int OutcropEngine::stealVoice() noexcept
{
    // Steal the quietest releasing voice, or the quietest active if none releasing.
    int  stealIdx = 0;
    float minLevel = 1e9f;
    for (int i = 0; i < kOutcropMaxVoices; ++i)
    {
        const float lv = voices[i].ampEnv.getLevel();
        if (voices[i].releasing && lv < minLevel)
        {
            minLevel = lv;
            stealIdx = i;
        }
    }
    if (minLevel > 1e8f)
    {
        // No releasing voice — steal the quietest overall.
        // Reset minLevel so we find the true minimum across all voices.
        minLevel = 1e9f;
        for (int i = 0; i < kOutcropMaxVoices; ++i)
        {
            const float lv = voices[i].ampEnv.getLevel();
            if (lv < minLevel) { minLevel = lv; stealIdx = i; }
        }
    }
    return stealIdx;
}

inline void OutcropEngine::startVoice(int noteNum, float vel, bool legato)
{
    int v = findFreeVoice();
    if (v < 0)
    {
        // P1-1 (#1126): apply a short steal fade to avoid hard cut.
        v = stealVoice();
        // Start a 5ms fade-out on the stolen voice slot before reinitialising it.
        // The new note content is written immediately (amplitude is silenced by ramp).
        const float fadeMs = 5.0f;
        voices[v].stealRampSamples = static_cast<int>(sampleRateF * fadeMs / 1000.0f);
        voices[v].stealRampTotal   = static_cast<float>(voices[v].stealRampSamples);
    }

    auto& vx = voices[v];
    vx.active   = true;
    vx.releasing= false;
    vx.note     = noteNum;
    vx.velocity = vel;

    const float f = 440.0f * fastPow2((noteNum - 69) * (1.0f / 12.0f));
    vx.orbitFreq = f;
    if (!legato) vx.glideFreq = f;
    vx.orbitPhase = 0.0f;
    vx.jitterState = 0x9E3779B1u ^ (uint32_t)(noteNum * 2654435761u);

    vx.ampEnv.noteOn();
    vx.fltEnv.noteOn();
    vx.filter.reset();

    lastHeldNote = noteNum;
    wakeSilenceGate();
}

inline void OutcropEngine::releaseVoicesForNote(int noteNum)
{
    for (auto& v : voices)
    {
        if (v.active && v.note == noteNum && !v.releasing)
        {
            if (!sustainPedal)
            {
                v.ampEnv.noteOff();
                v.fltEnv.noteOff();
                v.releasing = true;
            }
            // If sustain pedal is down, defer release until pedal lifts (CC64=0 handler above).
        }
    }
}

// -----------------------------------------------------------------------------
//  L I F E C Y C L E
// -----------------------------------------------------------------------------
inline void OutcropEngine::prepare(double sampleRate, int maxBlockSize)
{
    sampleRateF = (sampleRate > 0.0) ? (float) sampleRate : 44100.0f;
    blockCap    = std::max(1, maxBlockSize);

    for (auto& v : voices)
    {
        v.reset();
        v.ampEnv.prepare(sampleRateF);
        v.fltEnv.prepare(sampleRateF);
    }

    lfo1.reset();
    lfo2.reset();

    couplingCacheL.assign((size_t) blockCap, 0.0f);
    couplingCacheR.assign((size_t) blockCap, 0.0f);

    prepareSilenceGate(sampleRate, maxBlockSize, 300.0f);

    couplingFilterMod = couplingFmMod = couplingRateMod = couplingBlendMod = 0.0f;
    modWheel = aftertouch = pitchBendNorm = 0.0f;
    sustainPedal = false;
    lastHeldNote = -1;
}

inline void OutcropEngine::reset()
{
    for (auto& v : voices) v.reset();
    lfo1.reset();
    lfo2.reset();
    std::fill(couplingCacheL.begin(), couplingCacheL.end(), 0.0f);
    std::fill(couplingCacheR.begin(), couplingCacheR.end(), 0.0f);
    couplingFilterMod = couplingFmMod = couplingRateMod = couplingBlendMod = 0.0f;
    modWheel = aftertouch = pitchBendNorm = 0.0f;
    sustainPedal = false;
    lastHeldNote = -1;
}

// -----------------------------------------------------------------------------
//  A D D   P A R A M E T E R S   —   S E C T I O N   A   +   B
// -----------------------------------------------------------------------------
inline void OutcropEngine::addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    using AP  = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;
    using PID = juce::ParameterID;
    using NR  = juce::NormalisableRange<float>;

    // ---- A: Terrain ----
    params.push_back(std::make_unique<APC>(PID{"outc_terrainType",1}, "Outcrop Terrain",
        juce::StringArray{"Peaks","Ridges","Lattice","Saddle","Mixed","Folded"}, 4));

    params.push_back(std::make_unique<AP>(PID{"outc_peakHeight",1}, "Outcrop Peak Height",
        NR{0.0f, 1.0f, 0.001f}, 0.60f));

    params.push_back(std::make_unique<AP>(PID{"outc_peakSpread",1}, "Outcrop Peak Spread",
        NR{0.1f, 2.0f, 0.001f}, 0.70f));

    params.push_back(std::make_unique<AP>(PID{"outc_ridgeDepth",1}, "Outcrop Ridge Depth",
        NR{0.0f, 1.0f, 0.001f}, 0.35f));

    {
        NR r{0.5f, 12.0f, 0.001f};
        r.setSkewForCentre(3.0f);
        params.push_back(std::make_unique<AP>(PID{"outc_ridgeFreq",1}, "Outcrop Ridge Freq", r, 3.0f));
    }

    params.push_back(std::make_unique<AP>(PID{"outc_ridgeAngle",1}, "Outcrop Ridge Angle",
        NR{0.0f, 6.2831853f, 0.001f}, 0.785f));

    params.push_back(std::make_unique<AP>(PID{"outc_roughness",1}, "Outcrop Roughness",
        NR{0.0f, 0.5f, 0.001f}, 0.08f));

    // ---- B: Orbit ----
    params.push_back(std::make_unique<APC>(PID{"outc_orbitShape",1}, "Outcrop Orbit",
        juce::StringArray{"Circle","Lissajous","Rose","Spirograph","Figure8","Folded"}, 1));

    params.push_back(std::make_unique<AP>(PID{"outc_orbitRadius",1}, "Outcrop Orbit Radius",
        NR{0.2f, 2.0f, 0.001f}, 0.80f));

    {
        NR r{0.25f, 8.0f, 0.001f};
        r.setSkewForCentre(1.5f);
        params.push_back(std::make_unique<AP>(PID{"outc_orbitRatio",1}, "Outcrop Orbit Ratio", r, 1.5f));
    }

    params.push_back(std::make_unique<AP>(PID{"outc_orbitPhase",1}, "Outcrop Orbit Phase",
        NR{0.0f, 6.2831853f, 0.001f}, 0.0f));

    params.push_back(std::make_unique<AP>(PID{"outc_orbitJitter",1}, "Outcrop Orbit Jitter",
        NR{0.0f, 0.3f, 0.001f}, 0.02f));

    // ---- C: Filter ----
    {
        NR r{20.0f, 20000.0f, 0.1f};
        r.setSkewForCentre(1000.0f);
        params.push_back(std::make_unique<AP>(PID{"outc_fltCutoff",1}, "Outcrop Filter Cutoff", r, 2200.0f));
    }
    params.push_back(std::make_unique<AP>(PID{"outc_fltReso",1}, "Outcrop Filter Reso",
        NR{0.0f, 1.0f, 0.001f}, 0.25f));
    params.push_back(std::make_unique<APC>(PID{"outc_fltType",1}, "Outcrop Filter Type",
        juce::StringArray{"LP","HP","BP","Notch"}, 0));
    params.push_back(std::make_unique<AP>(PID{"outc_fltEnvAmt",1}, "Outcrop Filter Env Amt",
        NR{-1.0f, 1.0f, 0.001f}, 0.35f));
    params.push_back(std::make_unique<AP>(PID{"outc_fltKeyTrack",1}, "Outcrop Filter Key Track",
        NR{0.0f, 1.0f, 0.001f}, 0.50f));
    params.push_back(std::make_unique<AP>(PID{"outc_fltVelTrack",1}, "Outcrop Filter Vel Track",
        NR{0.0f, 1.0f, 0.001f}, 0.60f)); // D001

    // ---- D: Amp Envelope ----
    {
        NR r{0.0f, 10.0f, 0.001f};
        r.setSkewForCentre(0.5f);
        params.push_back(std::make_unique<AP>(PID{"outc_ampAtk",1}, "Outcrop Amp Attack",  r, 0.010f));
        params.push_back(std::make_unique<AP>(PID{"outc_ampDec",1}, "Outcrop Amp Decay",   r, 0.250f));
        params.push_back(std::make_unique<AP>(PID{"outc_ampRel",1}, "Outcrop Amp Release", r, 0.500f));
    }
    params.push_back(std::make_unique<AP>(PID{"outc_ampSus",1}, "Outcrop Amp Sustain",
        NR{0.0f, 1.0f, 0.001f}, 0.75f));

    // ---- E: Filter Envelope ----
    {
        NR r{0.0f, 10.0f, 0.001f};
        r.setSkewForCentre(0.5f);
        params.push_back(std::make_unique<AP>(PID{"outc_fenvAtk",1}, "Outcrop FEnv Attack",  r, 0.014f));
        params.push_back(std::make_unique<AP>(PID{"outc_fenvDec",1}, "Outcrop FEnv Decay",   r, 0.380f));
        params.push_back(std::make_unique<AP>(PID{"outc_fenvRel",1}, "Outcrop FEnv Release", r, 0.600f));
    }
    params.push_back(std::make_unique<AP>(PID{"outc_fenvSus",1}, "Outcrop FEnv Sustain",
        NR{0.0f, 1.0f, 0.001f}, 0.30f));

    // ---- F: LFOs (D002 / D005 — rate floor 0.005 Hz) ----
    {
        NR r{0.005f, 20.0f, 0.001f};
        r.setSkewForCentre(1.0f);
        params.push_back(std::make_unique<AP>(PID{"outc_lfo1Rate",1}, "Outcrop LFO1 Rate", r, 0.50f));
        params.push_back(std::make_unique<AP>(PID{"outc_lfo2Rate",1}, "Outcrop LFO2 Rate", r, 0.10f));
    }
    params.push_back(std::make_unique<AP>(PID{"outc_lfo1Depth",1}, "Outcrop LFO1 Depth",
        NR{0.0f, 1.0f, 0.001f}, 0.30f));
    params.push_back(std::make_unique<AP>(PID{"outc_lfo2Depth",1}, "Outcrop LFO2 Depth",
        NR{0.0f, 1.0f, 0.001f}, 0.20f));
    params.push_back(std::make_unique<APC>(PID{"outc_lfo1Shape",1}, "Outcrop LFO1 Shape",
        juce::StringArray{"Sine","Triangle","Saw","Square","S&H"}, 1));
    params.push_back(std::make_unique<APC>(PID{"outc_lfo2Shape",1}, "Outcrop LFO2 Shape",
        juce::StringArray{"Sine","Triangle","Saw","Square","S&H"}, 0));
    params.push_back(std::make_unique<APC>(PID{"outc_lfo1Target",1}, "Outcrop LFO1 Target",
        juce::StringArray{"OrbitRadius","OrbitRatio","RidgeAngle","FilterCutoff"}, 2));
    params.push_back(std::make_unique<APC>(PID{"outc_lfo2Target",1}, "Outcrop LFO2 Target",
        juce::StringArray{"OrbitRadius","OrbitRatio","RidgeAngle","FilterCutoff"}, 0));

    // ---- G: Mod Matrix (4 slots) ----
    static const juce::StringArray kOutcropModDests {
        "Off", "Filter Cutoff", "Orbit Radius", "Terrain Blend", "Ridge Depth", "Roughness", "Amp Level"
    };
    ModMatrix<4>::addParameters(params, "outc_", "Outcrop", kOutcropModDests);

    // ---- H: Macros (M1 CHARACTER, M2 MOVEMENT, M3 COUPLING, M4 SPACE) ----
    params.push_back(std::make_unique<AP>(PID{"outc_macro1",1}, "Outcrop Macro CHARACTER",
        NR{0.0f, 1.0f, 0.001f}, 0.50f));
    params.push_back(std::make_unique<AP>(PID{"outc_macro2",1}, "Outcrop Macro MOVEMENT",
        NR{0.0f, 1.0f, 0.001f}, 0.35f));
    params.push_back(std::make_unique<AP>(PID{"outc_macro3",1}, "Outcrop Macro COUPLING",
        NR{0.0f, 1.0f, 0.001f}, 0.25f));
    params.push_back(std::make_unique<AP>(PID{"outc_macro4",1}, "Outcrop Macro SPACE",
        NR{0.0f, 1.0f, 0.001f}, 0.40f));

    // ---- I: Voice ----
    params.push_back(std::make_unique<APC>(PID{"outc_voiceMode",1}, "Outcrop Voice Mode",
        juce::StringArray{"Mono","Legato","Poly8"}, 2));
    {
        NR r{0.0f, 2.0f, 0.001f};
        r.setSkewForCentre(0.2f);
        params.push_back(std::make_unique<AP>(PID{"outc_glide",1}, "Outcrop Glide", r, 0.0f));
    }

    // ---- J: Output ----
    params.push_back(std::make_unique<AP>(PID{"outc_level",1}, "Outcrop Level",
        NR{0.0f, 1.25f, 0.001f}, 0.80f));
} // addParameters

// -----------------------------------------------------------------------------
//  A T T A C H   P A R A M E T E R S
// -----------------------------------------------------------------------------
inline void OutcropEngine::attachParameters(juce::AudioProcessorValueTreeState& apvts)
{
    auto get = [&](const juce::String& id) -> std::atomic<float>*
    {
        auto* p = apvts.getRawParameterValue(id);
        jassert(p != nullptr);
        return p;
    };

    pTerrainType = get("outc_terrainType");
    pPeakHeight  = get("outc_peakHeight");
    pPeakSpread  = get("outc_peakSpread");
    pRidgeDepth  = get("outc_ridgeDepth");
    pRidgeFreq   = get("outc_ridgeFreq");
    pRidgeAngle  = get("outc_ridgeAngle");
    pRoughness   = get("outc_roughness");

    pOrbitShape  = get("outc_orbitShape");
    pOrbitRadius = get("outc_orbitRadius");
    pOrbitRatio  = get("outc_orbitRatio");
    pOrbitPhase  = get("outc_orbitPhase");
    pOrbitJitter = get("outc_orbitJitter");

    pFltCutoff   = get("outc_fltCutoff");
    pFltReso     = get("outc_fltReso");
    pFltType     = get("outc_fltType");
    pFltEnvAmt   = get("outc_fltEnvAmt");
    pFltKeyTrack = get("outc_fltKeyTrack");
    pFltVelTrack = get("outc_fltVelTrack");

    pAmpAtk = get("outc_ampAtk");
    pAmpDec = get("outc_ampDec");
    pAmpSus = get("outc_ampSus");
    pAmpRel = get("outc_ampRel");

    pFenvAtk = get("outc_fenvAtk");
    pFenvDec = get("outc_fenvDec");
    pFenvSus = get("outc_fenvSus");
    pFenvRel = get("outc_fenvRel");

    pLfo1Rate  = get("outc_lfo1Rate");
    pLfo1Depth = get("outc_lfo1Depth");
    pLfo1Shape = get("outc_lfo1Shape");
    pLfo1Tgt   = get("outc_lfo1Target");
    pLfo2Rate  = get("outc_lfo2Rate");
    pLfo2Depth = get("outc_lfo2Depth");
    pLfo2Shape = get("outc_lfo2Shape");
    pLfo2Tgt   = get("outc_lfo2Target");

    pMacro1 = get("outc_macro1");
    pMacro2 = get("outc_macro2");
    pMacro3 = get("outc_macro3");
    pMacro4 = get("outc_macro4");

    pVoiceMode = get("outc_voiceMode");
    pGlide     = get("outc_glide");
    pLevel     = get("outc_level");

    modMatrix.attachParameters(apvts, "outc_");
}

// -----------------------------------------------------------------------------
//  A P P L Y   C O U P L I N G   I N P U T
// -----------------------------------------------------------------------------
inline void OutcropEngine::applyCouplingInput(CouplingType type, float amount,
                                               const float* sourceBuffer, int numSamples)
{
    if (sourceBuffer == nullptr || numSamples <= 0) return;

    // Block-rate peak of the source — cheap and sufficient for our mod taps.
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float a = std::fabs(sourceBuffer[i]);
        if (a > peak) peak = a;
    }
    const float scaled = peak * amount;

    switch (type)
    {
        case CouplingType::AmpToFilter:
            couplingFilterMod = scaled;
            break;
        case CouplingType::AudioToFM:
            // Inject as orbit phase deviation.
            couplingFmMod = scaled * 2.0f;
            break;
        case CouplingType::EnvToMorph:
            // Bias the terrain blend (drives ridge depth / peak balance).
            couplingBlendMod = scaled;
            break;
        case CouplingType::LFOToPitch:
        case CouplingType::AmpToPitch:
            couplingRateMod = scaled;
            break;
        default:
            // Generic fallback — route to filter modulation so every route
            // produces some audible response (see D004 philosophy).
            couplingFilterMod += scaled * 0.5f;
            break;
    }
}

// -----------------------------------------------------------------------------
//  R E N D E R   B L O C K   —   prologue (param snapshot + macros)
// -----------------------------------------------------------------------------
inline void OutcropEngine::renderBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midi, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (numSamples <= 0) return;
    if (sampleRateF <= 0.0f) { buffer.clear(); return; }
    const int nCh = buffer.getNumChannels();
    if (nCh <= 0) return;

    auto loadP = [](std::atomic<float>* p, float dflt) noexcept
    {
        return p ? p->load() : dflt;
    };

    // ---- Param snapshot (ParamSnapshot pattern) ----
    const int   terrainType  = (int) loadP(pTerrainType, 4.0f);
    const float peakHeight   = loadP(pPeakHeight,  0.60f);
    const float peakSpread   = loadP(pPeakSpread,  0.70f);
    const float ridgeDepth   = loadP(pRidgeDepth,  0.35f);
    const float ridgeFreq    = loadP(pRidgeFreq,   3.0f);
    const float ridgeAngleP  = loadP(pRidgeAngle,  0.785f);
    const float roughness    = loadP(pRoughness,   0.08f);

    const int   orbitShape   = (int) loadP(pOrbitShape, 1.0f);
    const float orbitRadiusP = loadP(pOrbitRadius, 0.80f);
    const float orbitRatio   = loadP(pOrbitRatio,  1.5f);
    const float orbitPhaseP  = loadP(pOrbitPhase,  0.0f);
    const float orbitJitter  = loadP(pOrbitJitter, 0.02f);

    const float rawCutoff    = loadP(pFltCutoff,   2200.0f);
    const float fltReso      = loadP(pFltReso,     0.25f);
    const int   fltTypeIdx   = (int) loadP(pFltType, 0.0f);
    const float fltEnvAmt    = loadP(pFltEnvAmt,   0.35f);
    const float fltKeyTrack  = loadP(pFltKeyTrack, 0.50f);
    const float fltVelTrack  = loadP(pFltVelTrack, 0.60f);

    const float ampAtk = loadP(pAmpAtk, 0.010f);
    const float ampDec = loadP(pAmpDec, 0.250f);
    const float ampSus = loadP(pAmpSus, 0.75f);
    const float ampRel = loadP(pAmpRel, 0.500f);

    const float fenvAtk = loadP(pFenvAtk, 0.014f);
    const float fenvDec = loadP(pFenvDec, 0.380f);
    const float fenvSus = loadP(pFenvSus, 0.30f);
    const float fenvRel = loadP(pFenvRel, 0.600f);

    const float lfo1Hz    = std::max(0.005f, loadP(pLfo1Rate, 0.5f));
    const float lfo1Depth = loadP(pLfo1Depth, 0.30f);
    const int   lfo1Shape = (int) loadP(pLfo1Shape, 1.0f);
    const int   lfo1Tgt   = (int) loadP(pLfo1Tgt,   2.0f);
    const float lfo2Hz    = std::max(0.005f, loadP(pLfo2Rate, 0.10f));
    const float lfo2Depth = loadP(pLfo2Depth, 0.20f);
    const int   lfo2Shape = (int) loadP(pLfo2Shape, 0.0f);
    const int   lfo2Tgt   = (int) loadP(pLfo2Tgt,   0.0f);

    const float m1 = loadP(pMacro1, 0.50f);
    const float m2 = loadP(pMacro2, 0.35f);
    const float m3 = loadP(pMacro3, 0.25f);
    const float m4 = loadP(pMacro4, 0.40f);

    const int   voiceMode = (int) loadP(pVoiceMode, 2.0f);
    const float rawGlide  = loadP(pGlide, 0.0f);
    const float level     = loadP(pLevel, 0.80f);

    // ---- Macro expansion ----
    // M1 CHARACTER: peak height + ridge depth (intensify terrain relief)
    const float mPeakHeight = std::clamp(peakHeight + m1 * 0.4f, 0.0f, 1.4f);
    const float mRidgeDepth = std::clamp(ridgeDepth + m1 * 0.4f, 0.0f, 1.4f);
    // M2 MOVEMENT: LFO1 depth boost + orbit ratio drift
    const float mLfo1Depth  = std::clamp(lfo1Depth + m2 * 0.5f, 0.0f, 1.0f);
    const float mOrbitRatio = std::clamp(orbitRatio * (1.0f + m2 * 0.5f), 0.25f, 12.0f);
    // M3 COUPLING: orbit jitter + FM mod sensitivity (applied below)
    const float mOrbitJitter = std::clamp(orbitJitter + m3 * 0.2f, 0.0f, 0.5f);
    const float mCouplingGain = 0.5f + m3 * 1.5f;
    // M4 SPACE: filter cutoff boost + level trim
    const float mCutoff = std::clamp(rawCutoff * (1.0f + m4 * 1.0f), 20.0f, 20000.0f);

    // ---- Glide coefficient ----
    glideCoeff = (rawGlide > 0.001f)
        ? (1.0f - std::exp(-1.0f / (rawGlide * sampleRateF)))
        : 1.0f;

    // ---- Parse MIDI ----
    for (const auto& meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isNoteOn() && msg.getVelocity() > 0)
        {
            const int   newNote = msg.getNoteNumber();
            const float vel     = (float) msg.getVelocity() / 127.0f;
            const bool  legato  = (voiceMode == 1) && (lastHeldNote >= 0);

            if (voiceMode == 0 || voiceMode == 1) // Mono / Legato
            {
                // Reuse voice 0 — retrigger amp envelope unless legato.
                auto& vx = voices[0];
                vx.active = true;
                vx.releasing = false;
                vx.note = newNote;
                vx.velocity = vel;
                const float f = 440.0f * fastPow2((newNote - 69) * (1.0f / 12.0f));
                vx.orbitFreq = f;
                if (!legato)
                {
                    vx.glideFreq = f;
                    vx.orbitPhase = 0.0f;
                    vx.filter.reset(); // clear filter state on fresh attack (was missing)
                    vx.ampEnv.noteOn();
                    vx.fltEnv.noteOn();
                    // Re-seed jitter only on fresh note-on, not on legato slides.
                    vx.jitterState = 0x9E3779B1u ^ (uint32_t)(newNote * 2654435761u);
                }
                else
                {
                    // Legato: keep phase, keep env level.
                    // P2-5 (#1126): reseed jitter on legato too — old note's
                    // pattern under a new pitch is audible as a timing artefact.
                    vx.jitterState = 0x9E3779B1u ^ (uint32_t)(newNote * 2654435761u);
                }
                lastHeldNote = newNote;
                wakeSilenceGate();
            }
            else // Poly8
            {
                startVoice(newNote, vel, false);
            }
        }
        else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0))
        {
            releaseVoicesForNote(msg.getNoteNumber());
            if (lastHeldNote == msg.getNoteNumber()) lastHeldNote = -1;
        }
        else if (msg.isController() && msg.getControllerNumber() == 1)
        {
            modWheel = (float) msg.getControllerValue() / 127.0f;
        }
        else if (msg.isController() && msg.getControllerNumber() == 64)
        {
            // P2-7 (#1126): CC64 sustain pedal
            sustainPedal = (msg.getControllerValue() >= 64);
            if (!sustainPedal)
            {
                // Pedal lifted — release any voices that are held but not actively pressed.
                for (auto& v : voices)
                    if (v.active && v.releasing == false && v.note != lastHeldNote)
                    { v.ampEnv.noteOff(); v.fltEnv.noteOff(); v.releasing = true; }
            }
        }
        else if (msg.isChannelPressure())
        {
            aftertouch = (float) msg.getChannelPressureValue() / 127.0f;
        }
        else if (msg.isPitchWheel())
        {
            pitchBendNorm = (msg.getPitchWheelValue() - 8192) / 8192.0f;
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            // P2-6 (#1126): MIDI panic — reset all voices
            for (auto& v : voices) v.reset();
            lastHeldNote = -1;
        }
    }

    // ---- Silence gate early exit ----
    if (isSilenceGateBypassed() && midi.isEmpty())
    {
        // P2-2 (#1126): zero coupling accumulators so stale mod doesn't bleed
        // into the next active block when the sender resumes.
        couplingFilterMod = couplingFmMod = couplingRateMod = couplingBlendMod = 0.0f;
        // Zero coupling cache so downstream engines see silence.
        const int n = std::min(numSamples, (int) couplingCacheL.size());
        std::fill_n(couplingCacheL.begin(), n, 0.0f);
        std::fill_n(couplingCacheR.begin(), n, 0.0f);
        return;
    }

    // ---- LFOs (block-rate: one step per block, compensate phaseInc for block size) ----
    // P-LFO-BLOCK fix: StandardLFO.setRate stores phaseInc = hz/sampleRate (per-sample).
    // Calling process() once per block advances the phase only 1 sample worth. Multiply hz
    // by numSamples so the accumulator advances the correct number of samples per call.
    lfo1.setRate(lfo1Hz * (float) numSamples, sampleRateF);
    lfo1.setShape(lfo1Shape);
    lfo2.setRate(lfo2Hz * (float) numSamples, sampleRateF);
    lfo2.setShape(lfo2Shape);
    const float lfo1Val = lfo1.process() * mLfo1Depth;
    const float lfo2Val = lfo2.process() * lfo2Depth;

    // Route LFOs to their targets as block-level scalars.
    //   target 0 = OrbitRadius, 1 = OrbitRatio, 2 = RidgeAngle, 3 = FilterCutoff
    auto routeLfo = [](int tgt, float v, float out[4])
    {
        if (tgt >= 0 && tgt < 4) out[tgt] += v;
    };
    float lfoMod[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    routeLfo(lfo1Tgt, lfo1Val, lfoMod);
    routeLfo(lfo2Tgt, lfo2Val, lfoMod);

    const float effOrbitRadius = std::clamp(orbitRadiusP * (1.0f + lfoMod[0] * 0.5f), 0.1f, 2.5f);
    const float effOrbitRatio  = std::clamp(mOrbitRatio * (1.0f + lfoMod[1] * 0.4f),  0.25f, 12.0f);
    const float effRidgeAngle  = ridgeAngleP + lfoMod[2] * 1.0f;
    const float cosEffRidge = fastCos(effRidgeAngle);
    const float sinEffRidge = fastSin(effRidgeAngle);
    const float lfoCutoffScale = fastPow2(lfoMod[3] * 2.0f); // ±2 octaves

    // Mod matrix — feed per-block source snapshot.
    ModMatrix<4>::Sources modSrc;
    modSrc.lfo1 = lfo1Val;
    modSrc.lfo2 = lfo2Val;
    // Use amp envelope level of the first active voice as the Envelope source.
    // In poly mode this favours voice 0; a per-voice mod matrix would be more
    // correct but adds per-sample cost — this is the block-rate approximation.
    {
        float envLevel = 0.0f;
        for (const auto& v : voices)
            if (v.active) { envLevel = v.ampEnv.getLevel(); break; }
        modSrc.env = envLevel;
    }
    // Use the highest-velocity active voice for velocity-based mod sources.
    {
        float bestVel = 0.0f;
        for (const auto& v : voices)
            if (v.active && v.velocity > bestVel) bestVel = v.velocity;
        modSrc.velocity = bestVel;
    }
    modSrc.keyTrack   = (lastHeldNote >= 0) ? (lastHeldNote - 60.0f) / 60.0f : 0.0f;
    modSrc.modWheel   = modWheel;
    modSrc.aftertouch = aftertouch;

    float modOffsets[7] = {0,0,0,0,0,0,0};
    float* modOuts[7] = { &modOffsets[0], &modOffsets[1], &modOffsets[2], &modOffsets[3],
                          &modOffsets[4], &modOffsets[5], &modOffsets[6] };
    modMatrix.apply(modSrc, modOuts, 7);
    // 0=Off, 1=Filter Cutoff, 2=Orbit Radius, 3=Terrain Blend, 4=Ridge Depth, 5=Roughness, 6=Amp Level
    const float matrixCutoff   = modOffsets[1];
    const float matrixRadius   = modOffsets[2];
    const float matrixBlend    = modOffsets[3];
    const float matrixRidge    = modOffsets[4];
    const float matrixRough    = modOffsets[5];
    const float matrixAmp      = modOffsets[6];

    const float finalOrbitRadius = std::clamp(effOrbitRadius + matrixRadius * 0.5f, 0.1f, 2.5f);
    const float finalPeakHeight  = std::clamp(mPeakHeight    + matrixBlend * 0.5f + couplingBlendMod * 0.5f, 0.0f, 1.5f);
    const float finalRidgeDepth  = std::clamp(mRidgeDepth    + matrixRidge * 0.5f - couplingBlendMod * 0.3f, 0.0f, 1.5f);
    const float finalRoughness   = std::clamp(roughness      + matrixRough * 0.3f, 0.0f, 0.8f);

    // ---- Zero output buffer ----
    buffer.clear();
    float* L = buffer.getWritePointer(0);
    float* R = (nCh > 1) ? buffer.getWritePointer(1) : L;

    // Phase increment per sample for a unit frequency — we multiply by orbitFreq.
    const float invSR = 1.0f / sampleRateF;

    // Pre-compute pitch-bend multiplier once per block (same for all voices).
    // Range is ±2 semitones (hardcoded); a future pBendRange parameter could widen this.
    const float pitchBendMul = fastPow2(pitchBendNorm * 2.0f * (1.0f / 12.0f));

    // Shared coupling accumulator values frozen for the block.
    const float couplingFilterAdd = couplingFilterMod * mCouplingGain * 4000.0f; // up to +4 kHz
    // P1-2 (#1126): clamp to ±π/4 rad to prevent terrain phase aliasing under extreme coupling
    const float couplingPhaseAdd  = std::clamp(couplingFmMod * mCouplingGain, -kOutcropPiOver4, kOutcropPiOver4);
    const float rateMod           = 1.0f + couplingRateMod * mCouplingGain;

    // ---- Per-voice render ----
    for (auto& v : voices)
    {
        if (!v.active) continue;

        // Glide.
        if (rawGlide > 0.001f)
            v.glideFreq += (v.orbitFreq - v.glideFreq) * glideCoeff;
        else
            v.glideFreq = v.orbitFreq;

        // Configure per-voice envelopes (cheap to set each block).
        v.ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
        v.fltEnv.setADSR(fenvAtk, fenvDec, fenvSus, fenvRel);

        // Filter mode.
        switch (fltTypeIdx)
        {
            case 0: v.filter.setMode(CytomicSVF::Mode::LowPass);  break;
            case 1: v.filter.setMode(CytomicSVF::Mode::HighPass); break;
            case 2: v.filter.setMode(CytomicSVF::Mode::BandPass); break;
            case 3: v.filter.setMode(CytomicSVF::Mode::Notch);    break;
            default: v.filter.setMode(CytomicSVF::Mode::LowPass); break;
        }

        // Per-voice cutoff (includes velocity track → D001, key track, env, couplings, mod matrix).
        const float keyOffsetSemi = (v.note - 60) * fltKeyTrack;
        const float velBoost      = 1.0f + v.velocity * fltVelTrack * 3.0f; // up to 4× cutoff
        const float baseCutoff    = mCutoff * velBoost * fastPow2(keyOffsetSemi * (1.0f / 12.0f));

        const float pitchBendHz   = v.glideFreq * pitchBendMul; // pitchBendMul pre-computed per block
        const float phaseIncBase  = pitchBendHz * invSR * rateMod;

        // Deterministic per-voice jitter — two independent values for x and y
        // so jitter creates true 2D orbit perturbation, not a correlated DC offset.
        const float jitterX = v.nextJitter() * mOrbitJitter;
        const float jitterY = v.nextJitter() * mOrbitJitter;

        for (int i = 0; i < numSamples; ++i)
        {
            // Advance envelopes.
            const float a = v.ampEnv.process();
            const float fe = v.fltEnv.process();

            if (!v.ampEnv.isActive()) { v.active = false; v.filter.reset(); break; }

            // Orbit phase advance (radians).
            v.orbitPhase += phaseIncBase * kOutcropTwoPi;
            if (v.orbitPhase >= kOutcropTwoPi) v.orbitPhase -= kOutcropTwoPi;

            // Compute orbit coords with coupling FM injected into phase.
            const OrbitPos op = computeOrbit(orbitShape,
                                              v.orbitPhase + couplingPhaseAdd,
                                              effOrbitRatio,
                                              orbitPhaseP);
            const float ox = (op.x + jitterX) * finalOrbitRadius;
            const float oy = (op.y + jitterY) * finalOrbitRadius;

            // P2-3 (#1126): clamp terrain coords to ±4 to prevent trig precision loss
            const float oxc = std::clamp(ox, -4.0f, 4.0f);
            const float oyc = std::clamp(oy, -4.0f, 4.0f);

            // Terrain height → raw sample.
            const float h = evaluateTerrain(terrainType, oxc, oyc,
                                            finalPeakHeight, peakSpread,
                                            finalRidgeDepth, ridgeFreq, cosEffRidge, sinEffRidge,
                                            finalRoughness);

            // Per-sample filter cutoff with env offset.
            const float voiceCutoff = std::clamp(
                (baseCutoff + fe * fltEnvAmt * 8000.0f + couplingFilterAdd + matrixCutoff * 6000.0f) * lfoCutoffScale,
                20.0f, sampleRateF * 0.49f);
            // Fast path: no shelf modes used here — skip shelf-gain branch.
            v.filter.setCoefficients_fast(voiceCutoff, fltReso, sampleRateF);

            const float filtered = v.filter.processSample(h);
            const float ampBoost = std::clamp(1.0f + matrixAmp * 0.5f, 0.0f, 2.0f);
            float voiceOut = filtered * a * ampBoost;

            // P1-1 (#1126): apply steal fade-in ramp to avoid click on voice steal.
            // The new note starts immediately but its amplitude ramps from 0 over ~5ms.
            if (v.stealRampSamples > 0)
            {
                const float rampGain = 1.0f - static_cast<float>(v.stealRampSamples) / v.stealRampTotal;
                voiceOut *= rampGain;
                --v.stealRampSamples;
            }

            // Pan based on orbit x — geometric stereo from the orbit itself.
            // Equal-power constant-power law: gL = cos(angle), gR = sin(angle)
            // where angle sweeps [0, π/2] as pan sweeps [-0.5, +0.5].
            const float pan   = std::clamp(op.x * 0.5f, -0.5f, 0.5f);
            const float angle = (pan + 0.5f) * (kOutcropTwoPi * 0.25f); // [0, π/2]
            const float gL = fastCos(angle);
            const float gR = fastSin(angle);

            L[i] += voiceOut * gL;
            R[i] += voiceOut * gR;
        }
    }

    // ---- Output trim + modwheel/aftertouch sweeten ----
    const float atBoost = 1.0f + aftertouch * 0.25f;
    const float gTotal  = level * atBoost;
    juce::FloatVectorOperations::multiply(L, gTotal, numSamples);
    if (R != L) juce::FloatVectorOperations::multiply(R, gTotal, numSamples);

    // ---- Cache coupling output + feed silence gate ----
    const int n = std::min(numSamples, (int) couplingCacheL.size());
    std::memcpy(couplingCacheL.data(), L, (size_t) n * sizeof(float));
    if (R != L) std::memcpy(couplingCacheR.data(), R, (size_t) n * sizeof(float));
    else        std::memcpy(couplingCacheR.data(), L, (size_t) n * sizeof(float));

    analyzeForSilenceGate(buffer, numSamples);

    // Decay coupling accumulators so stale mod doesn't persist if sender stops.
    // P2-8 (#1126): denormal flush on coupling accumulator decay
    couplingFilterMod *= 0.95f; outcropFlushDenormal(couplingFilterMod);
    couplingFmMod     *= 0.95f; outcropFlushDenormal(couplingFmMod);
    couplingRateMod   *= 0.95f; outcropFlushDenormal(couplingRateMod);
    couplingBlendMod  *= 0.95f; outcropFlushDenormal(couplingBlendMod);
} // renderBlock

} // namespace xoceanus

