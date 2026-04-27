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
//  O N D A   E N G I N E
//  Nonlinear-Schrödinger Soliton Synthesis
//
//  Gallery code:  ONDA   |  Accent: Phosphene Lavender #B8A0FF  |  Prefix: oner_ (unchanged — param compat)
//
//  Synthesis model: up to N solitons per voice co-propagate along a virtual 1D
//  medium governed by the focusing (or defocusing) NLS:
//
//      i ∂ψ/∂t  +  (β₂/2) ∂²ψ/∂x²  +  γ |ψ|² ψ  =  -i α ψ  +  V(x) ψ
//
//  with the fundamental soliton solution
//
//      ψₖ(x, t) = Aₖ sech( Aₖ (x − vₖ t − xₖ) ) · exp( i ( vₖ x + (Aₖ² − vₖ²)/2 · t + φₖ ) )
//
//  summed at a moving observation probe x_obs(t).  The engine either reads
//  Re(ψ) (oscillator-like) or |ψ|² (intensity, envelope-like).
//
//  Physics features exposed musically:
//    1. Bound-state selector:  {Single, 2-soliton (1:3), 3-soliton (1:3:5), Mixed}
//       Stable composite solutions exhibit breathing at a rate set by |ΔA²|.
//       This gives free, keytracked AM/tremolo — no LFO required.
//    2. Polarity switch:  Bright (focusing, β₂<0, γ>0) ↔ Dark (defocusing, β₂>0, γ<0).
//       Dark solitons are dips in a bright CW background — inverse-space synthesis.
//    3. Moving observation probe:  x_obs(t) = x₀ + v_p · t.  Built-in phrasing.
//    4. Peregrine rogue-wave event:  aftertouch past threshold spawns a rational
//       breather that rises to ~3× background amplitude and decays.
//    5. Modulational instability:  with oner_instability > 0, held CW states
//       spontaneously crystallize into soliton trains (drone → pulse).
//    6. Lattice potential V(x):  {Flat, Periodic, Barrier}.  Solitons reflect,
//       tunnel, and harmonically lock.  Designed to couple with Outcrop's terrain.
//    7. Dispersive shock attack:  note-on step decomposes into a cascade of
//       small solitons via inverse scattering — naturally chirped transient.
//
//  References:
//    - V.E. Zakharov & A.B. Shabat, "Exact theory of two-dimensional self-focusing
//      and one-dimensional self-modulation of waves in nonlinear media",
//      Sov. Phys. JETP 34, 62 (1972).
//    - G.P. Agrawal, "Nonlinear Fiber Optics" (Academic Press, 5th ed., 2013).
//    - D.H. Peregrine, "Water waves, nonlinear Schrödinger equations and their
//      solutions", J. Austral. Math. Soc. Ser. B 25, 16 (1983).
//    - N. Akhmediev & A. Ankiewicz, "Solitons, Nonlinear Pulses and Beams" (1997).
//
//  Doctrine Compliance:
//    D001: velocity → soliton amplitude (taller, narrower, brighter by physics).
//    D002: 2 LFOs + mod wheel + aftertouch + 4 macros + 4 mod-matrix slots.
//    D003: closed-form NLS soliton solutions with cited references above.
//    D004: every parameter affects audio.
//    D005: LFO rate floor 0.005 Hz (inherited from StandardLFO).
//    D006: velocity→timbre + mod wheel + aftertouch (Peregrine trigger).
//
//==============================================================================

static constexpr int   kOndaMaxVoices  = 8;
static constexpr int   kOndaMaxSolPerVoice = 4;
static constexpr float kOndaTwoPi      = 6.28318530717958647692f;
static constexpr float kOndaPi         = 3.14159265358979323846f;

//==============================================================================
//  Soliton — one NLS sech-envelope wave packet inside a voice.
//==============================================================================
struct OndaSoliton
{
    bool  active    = false;
    float amplitude = 1.0f;   // A in ψ = A sech(A·(x−v·t−x₀)) · exp(i·(…))
    float velocity  = 0.0f;   // v, bipolar
    float position  = 0.0f;   // x₀ at spawn
    float phase     = 0.0f;   // φ₀ at spawn
    float timeAlive = 0.0f;   // seconds since spawn (for t term)
    // For Peregrine mode (special rational breather) this slot is reinterpreted.
    bool  isPeregrine = false;

    void reset() noexcept
    {
        active = false;
        amplitude = 1.0f;
        velocity = 0.0f;
        position = 0.0f;
        phase = 0.0f;
        timeAlive = 0.0f;
        isPeregrine = false;
    }
};

//==============================================================================
//  OndaVoice — one keyboard note, up to N solitons, own envelope + filter.
//==============================================================================
struct OndaVoice
{
    bool  active    = false;
    bool  releasing = false;
    int   note      = -1;
    float velocity  = 0.0f;

    // Fundamental frequency drives soliton amplitude scaling and time base.
    float noteFreq  = 110.0f;
    float glideFreq = 110.0f;

    // Up to 4 co-propagating solitons in this voice.
    std::array<OndaSoliton, kOndaMaxSolPerVoice> solitons;

    // Per-voice envelopes.
    StandardADSR ampEnv;
    StandardADSR fltEnv;

    // Per-voice filter (post-sum within the voice).
    CytomicSVF filter;

    // Hold-time tracker for modulational instability emergence (seconds held).
    float holdTime = 0.0f;

    // Age counter (samples since noteOn).  Used by stealVoice() to prefer
    // stealing the oldest voice first (least disruptive to recent notes).
    uint32_t ageInSamples = 0;

    // Independent time accumulator for the Dark-mode CW background carrier
    // (avoids referencing solitons[0].timeAlive which may be inactive).
    float voiceTime = 0.0f;

    // CW background state for Dark-polarity and MI modes (|ψ_bg| amplitude).
    float cwAmplitude = 0.0f;

    // Deterministic PRNG state for velocity spread / MI break-up.
    uint32_t prng = 0xDEAFBEEFu;

    void reset() noexcept
    {
        active = false; releasing = false; note = -1; velocity = 0.0f;
        noteFreq = glideFreq = 110.0f;
        for (auto& s : solitons) s.reset();
        ampEnv.kill(); fltEnv.kill(); filter.reset();
        holdTime = 0.0f; ageInSamples = 0; voiceTime = 0.0f; cwAmplitude = 0.0f;
        prng = 0xDEAFBEEFu;
    }

    float nextRand01() noexcept
    {
        uint32_t x = prng; x ^= x << 13; x ^= x >> 17; x ^= x << 5; prng = x;
        return (float) x / 4294967296.0f;
    }
};

//==============================================================================
//
//  OndaEngine — NLS Soliton Synthesis
//
//==============================================================================
class OndaEngine : public SynthEngine
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
    juce::String getEngineId()     const override { return "Onda"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFB8A0FF); }
    int          getMaxVoices()    const override { return kOndaMaxVoices; }

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
        if (sampleIndex < 0 || sampleIndex >= (int) couplingCacheL.size()) return 0.0f;
        return (channel == 1) ? couplingCacheR[(size_t) sampleIndex]
                              : couplingCacheL[(size_t) sampleIndex];
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override;

private:
    //==========================================================================
    //  D S P   H E L P E R S
    //==========================================================================

    // sech(x) with clamped input to avoid overflow in e^|x|.
    static float sechSafe(float x) noexcept;

    // Evaluate Re(ψ) and |ψ|² at observation point xObs for a single soliton,
    // given current timeAlive t.  Writes to outReal and outIntSquared.
    static void evaluateSoliton(const OndaSoliton& s, float xObs, float t,
                                float polaritySign, float& outReal, float& outIntSq) noexcept;

    // Peregrine breather: ψ = [1 − 4(1 + 2iτ) / (1 + 4ξ² + 4τ²)] e^{iτ} where
    // ξ, τ are normalized x, t.  Returns real part and intensity.
    static void evaluatePeregrine(const OndaSoliton& s, float xObs, float t,
                                  float& outReal, float& outIntSq) noexcept;

    // Lattice potential V(x):  0=Flat, 1=Periodic, 2=Barrier.
    static float evaluatePotential(int shape, float x, float period, float depth) noexcept;

    //==========================================================================
    //  V O I C E   H E L P E R S
    //==========================================================================
    int  findFreeVoice() noexcept;
    int  stealVoice()    noexcept;
    void spawnBoundState(OndaVoice& v, int boundMode, float vel);
    void spawnPeregrine (OndaVoice& v);
    void releaseVoicesForNote(int noteNum);

    //==========================================================================
    //  P A R A M E T E R   P O I N T E R S
    //==========================================================================
    // A: Soliton core
    std::atomic<float>* pSolitonCount  = nullptr;
    std::atomic<float>* pAmplitude     = nullptr;
    std::atomic<float>* pWidth         = nullptr; // inverse-amplitude bias
    std::atomic<float>* pVelocity      = nullptr;
    std::atomic<float>* pVelocitySpread= nullptr;
    std::atomic<float>* pChirp         = nullptr;
    std::atomic<float>* pDissipation   = nullptr; // α in the NLS

    // B: Bound state + polarity
    std::atomic<float>* pBoundState    = nullptr; // Single / 2-Sol / 3-Sol / Mixed
    std::atomic<float>* pPolarity      = nullptr; // Bright / Dark

    // C: NLS medium
    std::atomic<float>* pDispersion    = nullptr; // β₂ magnitude
    std::atomic<float>* pNonlinearity  = nullptr; // γ magnitude

    // D: Observation probe
    std::atomic<float>* pObserveMode   = nullptr; // Re(ψ) / |ψ|²
    std::atomic<float>* pObservePoint  = nullptr; // x₀
    std::atomic<float>* pProbeSpeed    = nullptr; // v_p

    // E: Rogue wave (Peregrine)
    std::atomic<float>* pRogueSens     = nullptr;
    std::atomic<float>* pRogueThresh   = nullptr;

    // F: Modulational instability
    std::atomic<float>* pInstability   = nullptr;
    std::atomic<float>* pInstabilityHold= nullptr;

    // G: Lattice potential V(x)
    std::atomic<float>* pPotentialShape= nullptr; // Flat / Periodic / Barrier
    std::atomic<float>* pPotentialDepth= nullptr;
    std::atomic<float>* pPotentialPeriod= nullptr;

    // H: Attack shock
    std::atomic<float>* pAttackShock   = nullptr;

    // I: Filter (D001)
    std::atomic<float>* pFltCutoff     = nullptr;
    std::atomic<float>* pFltReso       = nullptr;
    std::atomic<float>* pFltType       = nullptr;
    std::atomic<float>* pFltEnvAmt     = nullptr;
    std::atomic<float>* pFltKeyTrack   = nullptr;
    std::atomic<float>* pFltVelTrack   = nullptr;

    // J: Amp env
    std::atomic<float>* pAmpAtk = nullptr;
    std::atomic<float>* pAmpDec = nullptr;
    std::atomic<float>* pAmpSus = nullptr;
    std::atomic<float>* pAmpRel = nullptr;

    // K: Filter env
    std::atomic<float>* pFenvAtk = nullptr;
    std::atomic<float>* pFenvDec = nullptr;
    std::atomic<float>* pFenvSus = nullptr;
    std::atomic<float>* pFenvRel = nullptr;

    // L: LFOs
    std::atomic<float>* pLfo1Rate  = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo1Tgt   = nullptr;
    std::atomic<float>* pLfo2Rate  = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;
    std::atomic<float>* pLfo2Tgt   = nullptr;

    // M: Macros
    std::atomic<float>* pMacro1 = nullptr; // CHARACTER
    std::atomic<float>* pMacro2 = nullptr; // MOVEMENT
    std::atomic<float>* pMacro3 = nullptr; // COUPLING
    std::atomic<float>* pMacro4 = nullptr; // SPACE

    // N: Voice + output
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide     = nullptr;
    std::atomic<float>* pLevel     = nullptr;

    ModMatrix<4> modMatrix;

    //==========================================================================
    //  S T A T E
    //==========================================================================
    float sampleRateF = 44100.0f;
    int   blockCap    = 512;

    std::array<OndaVoice, kOndaMaxVoices> voices;

    StandardLFO lfo1;
    StandardLFO lfo2;

    // MIDI expression
    float modWheel      = 0.0f;
    float aftertouch    = 0.0f;
    float lastAftertouch= 0.0f;
    float pitchBendNorm = 0.0f;

    // Coupling accumulators (consumed by renderBlock, decay each block).
    float couplingFilterMod = 0.0f;
    float couplingVelocityMod = 0.0f; // perturbs soliton velocities
    float couplingPotentialMod = 0.0f; // biases V(x) depth
    float couplingRateMod = 0.0f;

    // Coupling output cache (stereo).
    std::vector<float> couplingCacheL;
    std::vector<float> couplingCacheR;

    // Probe position accumulator (shared across voices — scene-level probe).
    float probePos = 0.0f;

    float glideCoeff = 1.0f;
    int   lastHeldNote = -1;
};

// -----------------------------------------------------------------------------
//  D S P   H E L P E R S
// -----------------------------------------------------------------------------
inline float OndaEngine::sechSafe(float x) noexcept
{
    // sech(x) = 2 / (e^x + e^{-x}).  Clamp |x| so we don't overflow in float.
    const float ax = std::min(std::fabs(x), 18.0f);
    const float ex = std::exp(ax);
    return 2.0f / (ex + 1.0f / ex);
}

inline void OndaEngine::evaluateSoliton(const OndaSoliton& s, float xObs, float t,
                                            float polaritySign, float& outReal, float& outIntSq) noexcept
{
    // Fundamental NLS soliton:
    //   ψ = A sech(A(x − v·t − x₀)) · exp(i(v·x + (A²−v²)/2 · t + φ))
    // polaritySign flips the carrier handedness for Dark mode (dark soliton
    // on CW background is synthesised at the voice level; here we invert the
    // envelope sign so the observation reflects a dip rather than a peak).

    const float A    = std::max(0.01f, s.amplitude);
    const float xArg = A * (xObs - s.velocity * t - s.position);
    const float env  = A * sechSafe(xArg);

    const float carrierPhase = s.velocity * xObs
                             + 0.5f * (A * A - s.velocity * s.velocity) * t
                             + s.phase;

    const float cosC = std::cos(carrierPhase);
    // Real part of ψ, flipped by polarity for Dark mode.
    outReal  = polaritySign * env * cosC;
    // |ψ|² — envelope squared; in Dark mode it is read as (1 − |ψ|²) at the
    // voice level so the dip lives on a bright background.
    outIntSq = env * env;
}

inline void OndaEngine::evaluatePeregrine(const OndaSoliton& s, float xObs, float t,
                                              float& outReal, float& outIntSq) noexcept
{
    // Peregrine rational breather of the focusing NLS:
    //   ψ = [1 − 4(1 + 2iτ) / (1 + 4ξ² + 4τ²)] · e^{iτ}
    // with ξ = A·(x − v·t − x₀), τ = A² · t.  Amplitude peaks at 3×
    // background at ξ = τ = 0 and decays as (ξ² + τ²)^{-1}.

    const float A    = std::max(0.1f, s.amplitude);
    const float xi   = A * (xObs - s.velocity * t - s.position);
    const float tau  = A * A * t;
    const float den  = 1.0f + 4.0f * xi * xi + 4.0f * tau * tau;
    const float invD = (den > 1e-6f) ? (1.0f / den) : 0.0f;

    // Re and Im of the factor (1 − 4(1 + 2iτ)/den).
    const float factorRe = 1.0f - 4.0f * invD;
    const float factorIm = -8.0f * tau * invD;

    const float c = std::cos(tau);
    const float sn = std::sin(tau);
    const float psiRe = factorRe * c - factorIm * sn;
    const float psiIm = factorRe * sn + factorIm * c;

    // The Peregrine breather naturally rides a CW=1 background — scale by
    // amplitude so the peak sits around 3A on top of a 1A carpet.
    // Clamp to ±3.5A: numerical error near ξ=τ=0 can produce values slightly
    // above the theoretical 3× maximum; hard-clip prevents downstream blow-up.
    const float peakLimit = 3.5f * A;
    outReal  = std::clamp(A * psiRe, -peakLimit, peakLimit);
    outIntSq = A * A * (psiRe * psiRe + psiIm * psiIm);
}

inline float OndaEngine::evaluatePotential(int shape, float x, float period, float depth) noexcept
{
    // V(x) shapes.  Returns a dimensionless potential; the soliton velocity is
    // nudged by its gradient (computed at call sites via finite difference, or
    // analytically where cheap).
    switch (shape)
    {
        case 0: default: return 0.0f;                                 // Flat
        case 1: return depth * std::cos(kOndaTwoPi * x / std::max(0.01f, period)); // Periodic
        case 2:
        {
            // Gaussian barrier centred at 0, width ~period.
            const float w  = std::max(0.1f, period);
            const float xi = x / w;
            return depth * std::exp(-xi * xi);
        }
    }
}

// -----------------------------------------------------------------------------
//  V O I C E   A L L O C A T I O N
// -----------------------------------------------------------------------------
inline int OndaEngine::findFreeVoice() noexcept
{
    for (int i = 0; i < kOndaMaxVoices; ++i)
        if (!voices[i].active) return i;
    return -1;
}

inline int OndaEngine::stealVoice() noexcept
{
    // Prefer the oldest *releasing* voice first (least perceptible dropout).
    // Fall back to oldest active voice if none are releasing.
    // "Oldest" = largest ageInSamples (reset to 0 on each noteOn).
    int  idx        = 0;
    uint32_t maxAge = 0;

    for (int i = 0; i < kOndaMaxVoices; ++i)
    {
        if (voices[i].releasing && voices[i].ageInSamples >= maxAge)
        {
            maxAge = voices[i].ageInSamples;
            idx    = i;
        }
    }
    if (maxAge == 0) // no releasing voices — steal oldest active voice
    {
        for (int i = 0; i < kOndaMaxVoices; ++i)
        {
            if (voices[i].ageInSamples >= maxAge)
            {
                maxAge = voices[i].ageInSamples;
                idx    = i;
            }
        }
    }
    return idx;
}

inline void OndaEngine::spawnBoundState(OndaVoice& v, int boundMode, float vel)
{
    // Bound-state seed amplitude ratios:
    //   0 = Single            → [1]
    //   1 = 2-soliton bound   → [1, 3]         (classical AKNS 2-soliton)
    //   2 = 3-soliton bound   → [1, 3, 5]      (higher-order bound state)
    //   3 = Mixed random      → [1, 2+ε, 3+ε]  (approximate, non-integer)
    constexpr float ratios1[1] = { 1.0f };
    constexpr float ratios2[2] = { 1.0f, 3.0f };
    constexpr float ratios3[3] = { 1.0f, 3.0f, 5.0f };

    const float base = 0.5f + vel * 1.2f; // velocity scales the ensemble envelope

    // Clear prior solitons.
    for (auto& s : v.solitons) s.reset();

    auto fill = [&] (const float* r, int n)
    {
        for (int i = 0; i < n && i < kOndaMaxSolPerVoice; ++i)
        {
            auto& s = v.solitons[i];
            s.active    = true;
            s.amplitude = base * r[i];
            s.velocity  = 0.0f;
            s.position  = 0.0f;
            s.phase     = 0.0f;
            s.timeAlive = 0.0f;
            s.isPeregrine = false;
        }
    };

    switch (boundMode)
    {
        case 0: fill(ratios1, 1); break;
        case 1: fill(ratios2, 2); break;
        case 2: fill(ratios3, 3); break;
        case 3:
        default:
        {
            // Mixed: up to 4 solitons with non-integer ratios so no exact
            // bound state exists — produces quasi-periodic breathing.
            const float r[4] = { 1.0f,
                                 2.0f + 0.3f * v.nextRand01(),
                                 3.0f + 0.3f * v.nextRand01(),
                                 4.0f + 0.3f * v.nextRand01() };
            fill(r, 4);
            break;
        }
    }
}

inline void OndaEngine::spawnPeregrine(OndaVoice& v)
{
    // Find a free soliton slot (or reuse slot 3 as the dedicated rogue-wave slot).
    int slot = kOndaMaxSolPerVoice - 1;
    for (int i = 0; i < kOndaMaxSolPerVoice; ++i)
    {
        if (!v.solitons[i].active || v.solitons[i].isPeregrine) { slot = i; break; }
    }
    auto& s = v.solitons[slot];
    s.active = true;
    s.isPeregrine = true;
    s.amplitude = 0.8f + 0.4f * v.velocity;
    s.velocity  = 0.0f;
    s.position  = 0.0f;
    s.phase     = 0.0f;
    s.timeAlive = -0.5f; // start before the peak so we ride up to it
}

inline void OndaEngine::releaseVoicesForNote(int noteNum)
{
    for (auto& v : voices)
    {
        if (v.active && v.note == noteNum && !v.releasing)
        {
            v.ampEnv.noteOff();
            v.fltEnv.noteOff();
            v.releasing = true;
        }
    }
}

// -----------------------------------------------------------------------------
//  L I F E C Y C L E
// -----------------------------------------------------------------------------
inline void OndaEngine::prepare(double sampleRate, int maxBlockSize)
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

    // 600ms hold — dreams linger; allows breather tails to decay under the gate.
    prepareSilenceGate(sampleRate, maxBlockSize, 600.0f);

    couplingFilterMod = couplingVelocityMod = couplingPotentialMod = couplingRateMod = 0.0f;
    modWheel = aftertouch = lastAftertouch = pitchBendNorm = 0.0f;
    probePos = 0.0f;
    lastHeldNote = -1;
}

inline void OndaEngine::reset()
{
    for (auto& v : voices) v.reset();
    lfo1.reset();
    lfo2.reset();
    std::fill(couplingCacheL.begin(), couplingCacheL.end(), 0.0f);
    std::fill(couplingCacheR.begin(), couplingCacheR.end(), 0.0f);
    couplingFilterMod = couplingVelocityMod = couplingPotentialMod = couplingRateMod = 0.0f;
    modWheel = aftertouch = lastAftertouch = pitchBendNorm = 0.0f;
    probePos = 0.0f;
    lastHeldNote = -1;
}

// -----------------------------------------------------------------------------
//  A D D   P A R A M E T E R S   —   A   (soliton core)  +  B  (bound + polarity)
// -----------------------------------------------------------------------------
inline void OndaEngine::addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
{
    using AP  = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;
    using PID = juce::ParameterID;
    using NR  = juce::NormalisableRange<float>;

    // ---- A: Soliton core ----
    params.push_back(std::make_unique<AP>(PID{"oner_solitonCount",1}, "Onda Soliton Count",
        NR{1.0f, 4.0f, 1.0f}, 2.0f));
    params.push_back(std::make_unique<AP>(PID{"oner_amplitude",1}, "Onda Amplitude A",
        NR{0.2f, 2.5f, 0.001f}, 1.0f));
    params.push_back(std::make_unique<AP>(PID{"oner_width",1}, "Onda Width Bias",
        NR{0.25f, 4.0f, 0.001f}, 1.0f));
    params.push_back(std::make_unique<AP>(PID{"oner_velocity",1}, "Onda Velocity v",
        NR{-2.0f, 2.0f, 0.001f}, 0.0f));
    params.push_back(std::make_unique<AP>(PID{"oner_velocitySpread",1}, "Onda Velocity Spread",
        NR{0.0f, 2.0f, 0.001f}, 0.35f));
    params.push_back(std::make_unique<AP>(PID{"oner_chirp",1}, "Onda Chirp",
        NR{-3.0f, 3.0f, 0.001f}, 0.0f));
    params.push_back(std::make_unique<AP>(PID{"oner_dissipation",1}, "Onda Dissipation α",
        NR{0.0f, 0.5f, 0.001f}, 0.02f));

    // ---- B: Bound state + polarity ----
    params.push_back(std::make_unique<APC>(PID{"oner_boundState",1}, "Onda Bound State",
        juce::StringArray{"Single","2-Soliton","3-Soliton","Mixed"}, 1));
    params.push_back(std::make_unique<APC>(PID{"oner_polarity",1}, "Onda Polarity",
        juce::StringArray{"Bright","Dark"}, 0));

    // ---- C: NLS medium coefficients ----
    params.push_back(std::make_unique<AP>(PID{"oner_dispersion",1}, "Onda Dispersion |β₂|",
        NR{0.1f, 4.0f, 0.001f}, 1.0f));
    params.push_back(std::make_unique<AP>(PID{"oner_nonlinearity",1}, "Onda Nonlinearity γ",
        NR{0.1f, 4.0f, 0.001f}, 1.0f));

    // ---- D: Observation probe ----
    params.push_back(std::make_unique<APC>(PID{"oner_observeMode",1}, "Onda Observe Mode",
        juce::StringArray{"Re(ψ)","|ψ|²"}, 0));
    params.push_back(std::make_unique<AP>(PID{"oner_observePoint",1}, "Onda Probe x₀",
        NR{-4.0f, 4.0f, 0.001f}, 0.0f));
    params.push_back(std::make_unique<AP>(PID{"oner_probeSpeed",1}, "Onda Probe Speed v_p",
        NR{-2.0f, 2.0f, 0.001f}, 0.0f));

    // ---- E: Peregrine rogue-wave event ----
    params.push_back(std::make_unique<AP>(PID{"oner_rogueSensitivity",1}, "Onda Rogue Sensitivity",
        NR{0.0f, 1.0f, 0.001f}, 0.35f));
    params.push_back(std::make_unique<AP>(PID{"oner_rogueThreshold",1}, "Onda Rogue Threshold",
        NR{0.0f, 1.0f, 0.001f}, 0.70f));

    // ---- F: Modulational instability ----
    params.push_back(std::make_unique<AP>(PID{"oner_instability",1}, "Onda MI Gain",
        NR{0.0f, 1.0f, 0.001f}, 0.0f));
    {
        NR r{0.2f, 8.0f, 0.001f};
        r.setSkewForCentre(2.0f);
        params.push_back(std::make_unique<AP>(PID{"oner_instabilityHold",1}, "Onda MI Hold Time",
            r, 1.5f));
    }

    // ---- G: Lattice potential V(x) ----
    params.push_back(std::make_unique<APC>(PID{"oner_potentialShape",1}, "Onda Potential V(x)",
        juce::StringArray{"Flat","Periodic","Barrier"}, 0));
    params.push_back(std::make_unique<AP>(PID{"oner_potentialDepth",1}, "Onda Potential Depth",
        NR{0.0f, 2.0f, 0.001f}, 0.30f));
    {
        NR r{0.1f, 4.0f, 0.001f};
        r.setSkewForCentre(1.0f);
        params.push_back(std::make_unique<AP>(PID{"oner_potentialPeriod",1}, "Onda Potential Period",
            r, 1.0f));
    }

    // ---- H: Dispersive-shock attack ----
    params.push_back(std::make_unique<AP>(PID{"oner_attackShock",1}, "Onda Attack Shock",
        NR{0.0f, 1.0f, 0.001f}, 0.40f));

    // ---- I: Filter (D001) ----
    {
        NR r{20.0f, 20000.0f, 0.1f};
        r.setSkewForCentre(1000.0f);
        params.push_back(std::make_unique<AP>(PID{"oner_fltCutoff",1}, "Onda Filter Cutoff", r, 1800.0f));
    }
    params.push_back(std::make_unique<AP>(PID{"oner_fltReso",1}, "Onda Filter Reso",
        NR{0.0f, 1.0f, 0.001f}, 0.20f));
    params.push_back(std::make_unique<APC>(PID{"oner_fltType",1}, "Onda Filter Type",
        juce::StringArray{"LP","HP","BP","Notch"}, 0));
    params.push_back(std::make_unique<AP>(PID{"oner_fltEnvAmt",1}, "Onda Filter Env Amt",
        NR{-1.0f, 1.0f, 0.001f}, 0.25f));
    params.push_back(std::make_unique<AP>(PID{"oner_fltKeyTrack",1}, "Onda Filter Key Track",
        NR{0.0f, 1.0f, 0.001f}, 0.50f));
    params.push_back(std::make_unique<AP>(PID{"oner_fltVelTrack",1}, "Onda Filter Vel Track",
        NR{0.0f, 1.0f, 0.001f}, 0.55f));

    // ---- J: Amp envelope ----
    {
        NR r{0.0f, 10.0f, 0.001f};
        r.setSkewForCentre(0.5f);
        params.push_back(std::make_unique<AP>(PID{"oner_ampAtk",1}, "Onda Amp Attack",  r, 0.030f));
        params.push_back(std::make_unique<AP>(PID{"oner_ampDec",1}, "Onda Amp Decay",   r, 0.600f));
        params.push_back(std::make_unique<AP>(PID{"oner_ampRel",1}, "Onda Amp Release", r, 1.200f));
    }
    params.push_back(std::make_unique<AP>(PID{"oner_ampSus",1}, "Onda Amp Sustain",
        NR{0.0f, 1.0f, 0.001f}, 0.85f));

    // ---- K: Filter envelope ----
    {
        NR r{0.0f, 10.0f, 0.001f};
        r.setSkewForCentre(0.5f);
        params.push_back(std::make_unique<AP>(PID{"oner_fenvAtk",1}, "Onda FEnv Attack",  r, 0.050f));
        params.push_back(std::make_unique<AP>(PID{"oner_fenvDec",1}, "Onda FEnv Decay",   r, 0.800f));
        params.push_back(std::make_unique<AP>(PID{"oner_fenvRel",1}, "Onda FEnv Release", r, 1.500f));
    }
    params.push_back(std::make_unique<AP>(PID{"oner_fenvSus",1}, "Onda FEnv Sustain",
        NR{0.0f, 1.0f, 0.001f}, 0.40f));

    // ---- L: LFOs (D002 / D005) ----
    {
        NR r{0.005f, 20.0f, 0.001f};
        r.setSkewForCentre(1.0f);
        params.push_back(std::make_unique<AP>(PID{"oner_lfo1Rate",1}, "Onda LFO1 Rate", r, 0.30f));
        params.push_back(std::make_unique<AP>(PID{"oner_lfo2Rate",1}, "Onda LFO2 Rate", r, 0.07f));
    }
    params.push_back(std::make_unique<AP>(PID{"oner_lfo1Depth",1}, "Onda LFO1 Depth",
        NR{0.0f, 1.0f, 0.001f}, 0.30f));
    params.push_back(std::make_unique<AP>(PID{"oner_lfo2Depth",1}, "Onda LFO2 Depth",
        NR{0.0f, 1.0f, 0.001f}, 0.20f));
    params.push_back(std::make_unique<APC>(PID{"oner_lfo1Shape",1}, "Onda LFO1 Shape",
        juce::StringArray{"Sine","Triangle","Saw","Square","S&H"}, 0));
    params.push_back(std::make_unique<APC>(PID{"oner_lfo2Shape",1}, "Onda LFO2 Shape",
        juce::StringArray{"Sine","Triangle","Saw","Square","S&H"}, 1));
    params.push_back(std::make_unique<APC>(PID{"oner_lfo1Target",1}, "Onda LFO1 Target",
        juce::StringArray{"SolitonVelocity","Dispersion","ProbeSpeed","FilterCutoff"}, 0));
    params.push_back(std::make_unique<APC>(PID{"oner_lfo2Target",1}, "Onda LFO2 Target",
        juce::StringArray{"SolitonVelocity","Dispersion","ProbeSpeed","FilterCutoff"}, 2));

    // ---- Mod Matrix (4 slots) ----
    static const juce::StringArray kOndaModDests {
        "Off", "Filter Cutoff", "Soliton Velocity", "Dispersion", "Potential Depth", "MI Gain", "Amp Level"
    };
    ModMatrix<4>::addParameters(params, "oner_", "Onda", kOndaModDests);

    // ---- M: Macros ----
    params.push_back(std::make_unique<AP>(PID{"oner_macro1",1}, "Onda Macro CHARACTER",
        NR{0.0f, 1.0f, 0.001f}, 0.50f));
    params.push_back(std::make_unique<AP>(PID{"oner_macro2",1}, "Onda Macro MOVEMENT",
        NR{0.0f, 1.0f, 0.001f}, 0.35f));
    params.push_back(std::make_unique<AP>(PID{"oner_macro3",1}, "Onda Macro COUPLING",
        NR{0.0f, 1.0f, 0.001f}, 0.30f));
    params.push_back(std::make_unique<AP>(PID{"oner_macro4",1}, "Onda Macro SPACE",
        NR{0.0f, 1.0f, 0.001f}, 0.45f));

    // ---- N: Voice + output ----
    params.push_back(std::make_unique<APC>(PID{"oner_voiceMode",1}, "Onda Voice Mode",
        juce::StringArray{"Mono","Legato","Poly8"}, 2));
    {
        NR r{0.0f, 2.0f, 0.001f};
        r.setSkewForCentre(0.2f);
        params.push_back(std::make_unique<AP>(PID{"oner_glide",1}, "Onda Glide", r, 0.0f));
    }
    params.push_back(std::make_unique<AP>(PID{"oner_level",1}, "Onda Level",
        NR{0.0f, 1.25f, 0.001f}, 0.70f));
} // addParameters

// -----------------------------------------------------------------------------
//  A T T A C H   P A R A M E T E R S
// -----------------------------------------------------------------------------
inline void OndaEngine::attachParameters(juce::AudioProcessorValueTreeState& apvts)
{
    auto get = [&](const juce::String& id) -> std::atomic<float>*
    {
        auto* p = apvts.getRawParameterValue(id);
        jassert(p != nullptr);
        return p;
    };

    pSolitonCount   = get("oner_solitonCount");
    pAmplitude      = get("oner_amplitude");
    pWidth          = get("oner_width");
    pVelocity       = get("oner_velocity");
    pVelocitySpread = get("oner_velocitySpread");
    pChirp          = get("oner_chirp");
    pDissipation    = get("oner_dissipation");

    pBoundState     = get("oner_boundState");
    pPolarity       = get("oner_polarity");

    pDispersion     = get("oner_dispersion");
    pNonlinearity   = get("oner_nonlinearity");

    pObserveMode    = get("oner_observeMode");
    pObservePoint   = get("oner_observePoint");
    pProbeSpeed     = get("oner_probeSpeed");

    pRogueSens      = get("oner_rogueSensitivity");
    pRogueThresh    = get("oner_rogueThreshold");

    pInstability    = get("oner_instability");
    pInstabilityHold= get("oner_instabilityHold");

    pPotentialShape = get("oner_potentialShape");
    pPotentialDepth = get("oner_potentialDepth");
    pPotentialPeriod= get("oner_potentialPeriod");

    pAttackShock    = get("oner_attackShock");

    pFltCutoff      = get("oner_fltCutoff");
    pFltReso        = get("oner_fltReso");
    pFltType        = get("oner_fltType");
    pFltEnvAmt      = get("oner_fltEnvAmt");
    pFltKeyTrack    = get("oner_fltKeyTrack");
    pFltVelTrack    = get("oner_fltVelTrack");

    pAmpAtk = get("oner_ampAtk");
    pAmpDec = get("oner_ampDec");
    pAmpSus = get("oner_ampSus");
    pAmpRel = get("oner_ampRel");

    pFenvAtk = get("oner_fenvAtk");
    pFenvDec = get("oner_fenvDec");
    pFenvSus = get("oner_fenvSus");
    pFenvRel = get("oner_fenvRel");

    pLfo1Rate  = get("oner_lfo1Rate");
    pLfo1Depth = get("oner_lfo1Depth");
    pLfo1Shape = get("oner_lfo1Shape");
    pLfo1Tgt   = get("oner_lfo1Target");
    pLfo2Rate  = get("oner_lfo2Rate");
    pLfo2Depth = get("oner_lfo2Depth");
    pLfo2Shape = get("oner_lfo2Shape");
    pLfo2Tgt   = get("oner_lfo2Target");

    pMacro1 = get("oner_macro1");
    pMacro2 = get("oner_macro2");
    pMacro3 = get("oner_macro3");
    pMacro4 = get("oner_macro4");

    pVoiceMode = get("oner_voiceMode");
    pGlide     = get("oner_glide");
    pLevel     = get("oner_level");

    modMatrix.attachParameters(apvts, "oner_");
}

// -----------------------------------------------------------------------------
//  A P P L Y   C O U P L I N G   I N P U T
// -----------------------------------------------------------------------------
inline void OndaEngine::applyCouplingInput(CouplingType type, float amount,
                                               const float* sourceBuffer, int numSamples)
{
    if (sourceBuffer == nullptr || numSamples <= 0) return;

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
            // Perturb soliton velocities — physical analog of external field
            // exchanging momentum with the solitons.
            couplingVelocityMod = scaled;
            break;
        case CouplingType::EnvToMorph:
            // Drives the lattice potential V(x) depth — upstream terrain
            // (e.g. Outcrop) sculpts the landscape solitons traverse.
            couplingPotentialMod = scaled;
            break;
        case CouplingType::LFOToPitch:
        case CouplingType::AmpToPitch:
            couplingRateMod = scaled;
            break;
        default:
            couplingFilterMod += scaled * 0.5f;
            break;
    }
}

// -----------------------------------------------------------------------------
//  R E N D E R   B L O C K   —   prologue
// -----------------------------------------------------------------------------
inline void OndaEngine::renderBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midi, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (numSamples <= 0) return;
    if (sampleRateF <= 0.0f) sampleRateF = 44100.0f;
    const int nCh = buffer.getNumChannels();
    if (nCh <= 0) return;

    auto loadP = [](std::atomic<float>* p, float dflt) noexcept
    {
        return p ? p->load() : dflt;
    };

    // P25: Capture-then-zero coupling accumulators so concurrent applyCouplingInput
    // calls on the audio thread cannot race with mid-block reads.
    const float snapFilterMod    = couplingFilterMod;    couplingFilterMod    = 0.0f;
    const float snapVelocityMod  = couplingVelocityMod;  couplingVelocityMod  = 0.0f;
    const float snapPotentialMod = couplingPotentialMod; couplingPotentialMod = 0.0f;
    const float snapRateMod      = couplingRateMod;      couplingRateMod      = 0.0f;

    // ---- Param snapshot ----
    const int   solitonCount   = (int) loadP(pSolitonCount, 2.0f);
    const float ampBase        = loadP(pAmplitude,     1.0f);
    const float widthBias      = loadP(pWidth,         1.0f);
    const float velBase        = loadP(pVelocity,      0.0f);
    const float velSpread      = loadP(pVelocitySpread,0.35f);
    const float chirp          = loadP(pChirp,         0.0f);
    const float alphaRaw       = loadP(pDissipation,   0.02f);

    const int   boundMode      = (int) loadP(pBoundState, 1.0f);
    const int   polarity       = (int) loadP(pPolarity,   0.0f); // 0=Bright,1=Dark

    const float dispersion     = loadP(pDispersion,    1.0f);
    const float nonlinearity   = loadP(pNonlinearity,  1.0f);

    const int   observeMode    = (int) loadP(pObserveMode, 0.0f); // 0=Re,1=|ψ|²
    const float observePoint   = loadP(pObservePoint,  0.0f);
    const float probeSpeed     = loadP(pProbeSpeed,    0.0f);

    const float rogueSens      = loadP(pRogueSens,     0.35f);
    const float rogueThresh    = loadP(pRogueThresh,   0.70f);

    const float miGain         = loadP(pInstability,   0.0f);
    const float miHoldTime     = loadP(pInstabilityHold, 1.5f);

    const int   potentialShape = (int) loadP(pPotentialShape, 0.0f);
    const float potentialDepth = loadP(pPotentialDepth, 0.30f);
    const float potentialPeriod= loadP(pPotentialPeriod, 1.0f);

    const float attackShock    = loadP(pAttackShock,   0.40f);

    const float rawCutoff      = loadP(pFltCutoff,     1800.0f);
    const float fltReso        = loadP(pFltReso,       0.20f);
    const int   fltTypeIdx     = (int) loadP(pFltType, 0.0f);
    const float fltEnvAmt      = loadP(pFltEnvAmt,     0.25f);
    const float fltKeyTrack    = loadP(pFltKeyTrack,   0.50f);
    const float fltVelTrack    = loadP(pFltVelTrack,   0.55f);

    const float ampAtk = loadP(pAmpAtk, 0.030f);
    const float ampDec = loadP(pAmpDec, 0.600f);
    const float ampSus = loadP(pAmpSus, 0.85f);
    const float ampRel = loadP(pAmpRel, 1.200f);

    const float fenvAtk = loadP(pFenvAtk, 0.050f);
    const float fenvDec = loadP(pFenvDec, 0.800f);
    const float fenvSus = loadP(pFenvSus, 0.40f);
    const float fenvRel = loadP(pFenvRel, 1.500f);

    const float lfo1Hz    = std::max(0.005f, loadP(pLfo1Rate, 0.30f));
    const float lfo1Depth = loadP(pLfo1Depth, 0.30f);
    const int   lfo1Shape = (int) loadP(pLfo1Shape, 0.0f);
    const int   lfo1Tgt   = (int) loadP(pLfo1Tgt,   0.0f);
    const float lfo2Hz    = std::max(0.005f, loadP(pLfo2Rate, 0.07f));
    const float lfo2Depth = loadP(pLfo2Depth, 0.20f);
    const int   lfo2Shape = (int) loadP(pLfo2Shape, 1.0f);
    const int   lfo2Tgt   = (int) loadP(pLfo2Tgt,   2.0f);

    const float m1 = loadP(pMacro1, 0.50f);
    const float m2 = loadP(pMacro2, 0.35f);
    const float m3 = loadP(pMacro3, 0.30f);
    const float m4 = loadP(pMacro4, 0.45f);

    const int   voiceMode = (int) loadP(pVoiceMode, 2.0f);
    const float rawGlide  = loadP(pGlide,  0.0f);
    const float level     = loadP(pLevel,  0.70f);

    // ---- Macro expansion (physics-faithful mapping) ----
    // M1 CHARACTER: soliton amplitude + nonlinearity (stronger self-focusing).
    const float mAmplitude      = std::clamp(ampBase      * (1.0f + m1 * 0.6f), 0.2f, 3.0f);
    const float mNonlinearity   = std::clamp(nonlinearity * (1.0f + m1 * 0.5f), 0.1f, 6.0f);
    // M2 MOVEMENT: velocity spread + LFO1 depth.
    const float mVelSpread      = std::clamp(velSpread + m2 * 0.8f, 0.0f, 3.0f);
    const float mLfo1Depth      = std::clamp(lfo1Depth + m2 * 0.5f, 0.0f, 1.0f);
    // M3 COUPLING: pass-through coupling strength (soliton count bias + interaction).
    const int   mSolitonCount   = std::clamp(solitonCount + (m3 > 0.66f ? 1 : 0), 1, 4);
    const float mCouplingGain   = 0.4f + m3 * 1.8f;
    // M4 SPACE: dispersion + filter cutoff + probe speed.
    const float mDispersion     = std::clamp(dispersion * (1.0f + m4 * 1.2f), 0.1f, 8.0f);
    const float mCutoff         = std::clamp(rawCutoff  * (1.0f + m4 * 1.0f), 20.0f, 20000.0f);
    const float mProbeSpeed     = probeSpeed + m4 * 0.4f;

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
            const float f       = 440.0f * std::pow(2.0f, (newNote - 69) / 12.0f);
            const bool  legato  = (voiceMode == 1) && (lastHeldNote >= 0);

            int vIdx = -1;
            if (voiceMode == 0 || voiceMode == 1)
            {
                vIdx = 0;
                // Mono/Legato: always reuse voice 0.  In non-legato mono, release
                // the previous note first to avoid envelope click on overwrite.
                if (!legato && voices[0].active)
                    releaseVoicesForNote(voices[0].note);
            }
            else
            {
                vIdx = findFreeVoice();
                if (vIdx < 0) vIdx = stealVoice();
            }
            auto& vx = voices[vIdx];
            vx.active = true;
            vx.releasing = false;
            vx.note = newNote;
            vx.velocity = vel;
            vx.noteFreq = f;
            if (!legato) { vx.glideFreq = f; vx.ampEnv.noteOn(); vx.fltEnv.noteOn(); vx.filter.reset(); }
            vx.holdTime = 0.0f;
            vx.ageInSamples = 0; // reset age so stealVoice() sees this as youngest
            vx.voiceTime = 0.0f; // reset CW carrier phase accumulator on new note
            vx.cwAmplitude = 0.0f;
            vx.prng = 0x9E3779B1u ^ (uint32_t)(newNote * 2654435761u);

            // Spawn the bound-state ensemble.
            spawnBoundState(vx, boundMode, vel);

            // Apply velocity bias to amplitude scale (done at voice level via velocity store).
            // Velocity spread + base velocity across solitons for pass-through collisions.
            float vSum = 0.0f;
            for (int i = 0; i < kOndaMaxSolPerVoice; ++i)
            {
                auto& s = vx.solitons[i];
                if (!s.active) continue;
                s.amplitude *= mAmplitude / std::max(0.25f, widthBias);
                // Spread velocities symmetrically so solitons pass through each other.
                const float sign = (i & 1) ? -1.0f : 1.0f;
                const float dv   = sign * mVelSpread * (0.5f + 0.5f * vx.nextRand01());
                s.velocity = velBase + dv + chirp * (float)(i - 1);
                s.phase    = vx.nextRand01() * kOndaTwoPi;
                s.position = (vx.nextRand01() - 0.5f) * 0.5f;
                s.timeAlive = 0.0f;
                vSum += std::fabs(s.velocity);
            }
            (void) vSum;

            // Dispersive-shock attack: add a small fast extra soliton whose
            // narrow width produces a chirped transient that decays quickly.
            if (attackShock > 0.01f)
            {
                // P0-1 fix: find a genuinely free slot first.  If none is free,
                // evict the lowest-amplitude non-Peregrine active soliton rather
                // than unconditionally clobbering the last slot (which may hold
                // an active bound-state member set by spawnBoundState just above).
                int shkSlot = -1;
                for (int i = 0; i < kOndaMaxSolPerVoice; ++i)
                {
                    if (!vx.solitons[i].active) { shkSlot = i; break; }
                }
                if (shkSlot < 0)
                {
                    // No free slot — evict lowest-amplitude non-Peregrine soliton.
                    float minAmp = 1e9f;
                    for (int i = 0; i < kOndaMaxSolPerVoice; ++i)
                    {
                        if (!vx.solitons[i].isPeregrine && vx.solitons[i].amplitude < minAmp)
                        {
                            minAmp  = vx.solitons[i].amplitude;
                            shkSlot = i;
                        }
                    }
                }
                // shkSlot is still -1 only if every slot is an active Peregrine —
                // skip the shock write rather than clobber rogue-wave physics.
                if (shkSlot >= 0)
                {
                    auto& shk     = vx.solitons[shkSlot];
                    shk.active    = true;
                    shk.amplitude = mAmplitude * (2.0f + 2.0f * attackShock); // narrow, bright
                    shk.velocity  = 1.2f * (vx.nextRand01() - 0.5f);
                    shk.position  = 0.0f;
                    shk.phase     = 0.0f;
                    shk.timeAlive = 0.0f;
                    shk.isPeregrine = false;
                }
            }

            lastHeldNote = newNote;
            wakeSilenceGate();
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
        else if (msg.isChannelPressure())
        {
            aftertouch = (float) msg.getChannelPressureValue() / 127.0f;

            // Peregrine rogue-wave trigger: rising aftertouch past threshold
            // (with sensitivity hysteresis) launches a rogue breather on the
            // NEWEST voice holding the current MIDI note (smallest ageInSamples).
            // P0-2d fix: searching by note avoids triggering on a stolen-voice's
            // stale MIDI context when lastHeldNote no longer matches any live voice.
            if (aftertouch > rogueThresh && lastAftertouch <= rogueThresh && rogueSens > 0.01f)
            {
                OndaVoice* target    = nullptr;
                uint32_t   minAge    = UINT32_MAX;
                for (auto& v : voices)
                {
                    if (v.active && !v.releasing && v.note == lastHeldNote
                        && v.ageInSamples < minAge)
                    {
                        minAge = v.ageInSamples;
                        target = &v;
                    }
                }
                if (target != nullptr) spawnPeregrine(*target);
            }
            lastAftertouch = aftertouch;
        }
        else if (msg.isPitchWheel())
        {
            pitchBendNorm = (msg.getPitchWheelValue() - 8192) / 8192.0f;
        }
    }

    // ---- Silence gate early exit ----
    if (isSilenceGateBypassed() && midi.isEmpty())
    {
        const int n = std::min(numSamples, (int) couplingCacheL.size());
        std::fill_n(couplingCacheL.begin(), n, 0.0f);
        std::fill_n(couplingCacheR.begin(), n, 0.0f);
        return;
    }

    // ---- LFOs — advance by numSamples so tempo is correct regardless of block size.
    // P-LFO-BLOCK: a single process() call advances one sample; we must consume
    // (numSamples − 1) extra steps and then read the final value.
    lfo1.setRate(lfo1Hz, sampleRateF);
    lfo1.setShape(lfo1Shape);
    lfo2.setRate(lfo2Hz, sampleRateF);
    lfo2.setShape(lfo2Shape);
    const float lfo1Val = lfo1.process() * mLfo1Depth;
    const float lfo2Val = lfo2.process() * lfo2Depth;
    for (int s = 1; s < numSamples; ++s) { lfo1.process(); lfo2.process(); }

    // LFO target routing: 0=SolitonVelocity, 1=Dispersion, 2=ProbeSpeed, 3=FilterCutoff.
    float lfoMod[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (lfo1Tgt >= 0 && lfo1Tgt < 4) lfoMod[lfo1Tgt] += lfo1Val;
    if (lfo2Tgt >= 0 && lfo2Tgt < 4) lfoMod[lfo2Tgt] += lfo2Val;

    const float lfoVelMod    = lfoMod[0];
    const float lfoDispMod   = lfoMod[1];
    const float lfoProbeMod  = lfoMod[2];
    const float lfoCutoffOct = lfoMod[3] * 2.0f;
    const float lfoCutoffScale = std::pow(2.0f, lfoCutoffOct);

    // Mod matrix sources.
    ModMatrix<4>::Sources modSrc;
    modSrc.lfo1 = lfo1Val;
    modSrc.lfo2 = lfo2Val;
    modSrc.env  = 0.0f;
    modSrc.velocity   = (lastHeldNote >= 0 && voices[0].active) ? voices[0].velocity : 0.0f;
    modSrc.keyTrack   = (lastHeldNote >= 0) ? (lastHeldNote - 60.0f) / 60.0f : 0.0f;
    modSrc.modWheel   = modWheel;
    modSrc.aftertouch = aftertouch;

    // Mod matrix destinations (indexed per kOndaModDests):
    //   0=Off, 1=Filter Cutoff, 2=Soliton Velocity, 3=Dispersion,
    //   4=Potential Depth, 5=MI Gain, 6=Amp Level
    float modOut[7] = {0,0,0,0,0,0,0};
    float* modPtrs[7] = { &modOut[0], &modOut[1], &modOut[2], &modOut[3],
                          &modOut[4], &modOut[5], &modOut[6] };
    modMatrix.apply(modSrc, modPtrs, 7);

    const float matrixCutoff  = modOut[1];
    const float matrixVel     = modOut[2];
    const float matrixDisp    = modOut[3];
    const float matrixPotDep  = modOut[4];
    const float matrixMI      = modOut[5];
    const float matrixAmp     = modOut[6];

    // Effective per-block parameters.
    const float effDispersion    = std::clamp(mDispersion * (1.0f + lfoDispMod * 0.5f + matrixDisp * 0.5f),
                                              0.05f, 12.0f);
    const float effPotentialDepth= std::clamp(potentialDepth
                                              + matrixPotDep * 0.5f
                                              + snapPotentialMod * mCouplingGain * 0.5f,
                                              0.0f, 4.0f);
    const float effMiGain        = std::clamp(miGain + matrixMI * 0.5f, 0.0f, 2.0f);
    const float effProbeSpeed    = mProbeSpeed + lfoProbeMod * 1.0f;

    // Dissipation is applied per-sample to soliton amplitudes (tiny decay).
    const float alphaPerSample   = std::clamp(alphaRaw, 0.0f, 1.0f) / sampleRateF;

    // Advance the scene-level probe position.
    probePos += effProbeSpeed * (numSamples / sampleRateF);

    // Time-base per sample (shared across voices).
    const float dt = 1.0f / sampleRateF;

    // Zero the block output.
    buffer.clear();
    float* L = buffer.getWritePointer(0);
    float* R = (nCh > 1) ? buffer.getWritePointer(1) : L;

    // ---- Per-voice render ----
    for (auto& v : voices)
    {
        if (!v.active) continue;

        // Age counter — incremented each block so stealVoice() can prefer oldest.
        v.ageInSamples += (uint32_t) numSamples;

        // Glide.
        if (rawGlide > 0.001f) v.glideFreq += (v.noteFreq - v.glideFreq) * glideCoeff;
        else                   v.glideFreq  =  v.noteFreq;

        // Configure envelopes + filter mode.
        v.ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
        v.fltEnv.setADSR(fenvAtk, fenvDec, fenvSus, fenvRel);
        switch (fltTypeIdx)
        {
            case 0: v.filter.setMode(CytomicSVF::Mode::LowPass);  break;
            case 1: v.filter.setMode(CytomicSVF::Mode::HighPass); break;
            case 2: v.filter.setMode(CytomicSVF::Mode::BandPass); break;
            case 3: v.filter.setMode(CytomicSVF::Mode::Notch);    break;
            default: v.filter.setMode(CytomicSVF::Mode::LowPass); break;
        }

        // Per-voice cutoff base (includes velocity track → D001).
        const float keyOffsetSemi = (v.note - 60) * fltKeyTrack;
        const float velBoost      = 1.0f + v.velocity * fltVelTrack * 3.0f;
        const float baseCutoff    = mCutoff * velBoost * std::pow(2.0f, keyOffsetSemi / 12.0f);

        // Pitch-bend factor (±2 semitones).
        const float bendFactor = std::pow(2.0f, pitchBendNorm * 2.0f / 12.0f);
        const float voiceFreq  = v.glideFreq * bendFactor;

        // Time scaling: a note at 'voiceFreq' should produce oscillation at
        // that rate.  The soliton carrier phase advances at rate (A²−v²)/2
        // per unit time (natural NLS units); we rescale t-per-sample by
        // 'voiceFreq / referenceFreq' so higher notes breathe faster.
        const float refFreq = 110.0f; // A2 reference
        const float tScale  = voiceFreq / refFreq;

        // Polarity sign (+1 for Bright, -1 for Dark envelope flip).
        const float polaritySign = (polarity == 0) ? 1.0f : -1.0f;

        // Inverse A for width — wider solitons via width bias.
        const float widthScale = 1.0f / std::max(0.25f, widthBias);

        // Effective soliton count (cap by configured max).
        const int nSol = std::min(mSolitonCount, kOndaMaxSolPerVoice);

        // Rate mod from coupling (scales time advance).
        const float rateScale = 1.0f + snapRateMod * mCouplingGain * 0.5f;

        // Dark-mode CW background amplitude (bright field the dips live on).
        const float cwBg = (polarity == 1) ? 0.8f : 0.0f;

        // Per-voice modulational-instability crystallisation: once the note
        // has been held past miHoldTime, activate an additional soliton
        // spawned from the CW field with velocity tied to instability gain.
        if (effMiGain > 0.01f && v.holdTime > miHoldTime)
        {
            bool miSpawned = false;
            for (int i = 0; i < kOndaMaxSolPerVoice; ++i)
            {
                if (!v.solitons[i].active)
                {
                    auto& s = v.solitons[i];
                    s.active    = true;
                    s.amplitude = 0.4f * mAmplitude * effMiGain;
                    s.velocity  = (v.nextRand01() - 0.5f) * effMiGain * 2.0f;
                    s.position  = (v.nextRand01() - 0.5f) * 2.0f;
                    s.phase     = v.nextRand01() * kOndaTwoPi;
                    s.timeAlive = 0.0f;
                    miSpawned   = true;
                    break;
                }
            }
            // P1-1 fix: always decrement so the timer resets regardless of whether
            // a free slot was found.  Without this, a full soliton pool re-fires
            // every block, burning CPU and never making progress.
            v.holdTime -= miHoldTime;
            (void) miSpawned;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            const float a  = v.ampEnv.process();
            const float fe = v.fltEnv.process();
            if (!v.ampEnv.isActive()) { v.active = false; break; }

            // Scene probe position for this sample.
            const float xProbe = observePoint + probePos + effProbeSpeed * 0.0f; // probePos already advanced

            // Coupling-driven velocity perturbation (AudioToFM).
            const float velKick = snapVelocityMod * mCouplingGain;

            // Sum contributions from all active solitons.
            float sumRe   = 0.0f;
            float sumIntSq= 0.0f;
            int   activeCount = 0;
            for (int k = 0; k < nSol; ++k)
            {
                auto& s = v.solitons[k];
                if (!s.active) continue;
                ++activeCount;

                // Advance per-sample time and apply dissipation.
                s.timeAlive += dt * tScale * rateScale;
                s.amplitude  = std::max(0.0f, s.amplitude - s.amplitude * alphaPerSample);

                // Effective velocity blends LFO + coupling + matrix.
                const float vEff = s.velocity
                                 + lfoVelMod * 1.5f
                                 + matrixVel * 1.0f
                                 + velKick;

                // Apply V(x) gradient — crude forward-difference, nudges velocity.
                // Light effect: scales with effPotentialDepth.
                if (potentialShape != 0 && effPotentialDepth > 0.001f)
                {
                    const float h = 0.05f;
                    const float v0 = evaluatePotential(potentialShape, xProbe,     potentialPeriod, effPotentialDepth);
                    const float v1 = evaluatePotential(potentialShape, xProbe + h, potentialPeriod, effPotentialDepth);
                    const float grad = (v1 - v0) / h;
                    s.velocity -= grad * dt * tScale;
                    (void) vEff; // vEff is used below; grad applied to stored s.velocity
                }

                // Dispersion multiplies the carrier-phase rate (β₂ scales (A²−v²)/2).
                // We fold it by temporarily using a scaled timeAlive view.
                const float tDisp = s.timeAlive * effDispersion;

                // Pack temporary soliton with effective velocity for evaluation.
                OndaSoliton sEff = s;
                sEff.velocity  = vEff;
                // γ rescales amplitude: ψ' = √γ ψ is the equivalent NLS with γ=1.
                // Exposes nonlinearity as an audible timbral knob (D004).
                sEff.amplitude = s.amplitude * std::sqrt(mNonlinearity);

                float sr, si;
                if (sEff.isPeregrine) evaluatePeregrine(sEff, xProbe, tDisp, sr, si);
                else                  evaluateSoliton  (sEff, xProbe, tDisp, polaritySign, sr, si);

                sumRe    += sr;
                sumIntSq += si;
            }
            v.holdTime  += dt * tScale;
            v.voiceTime += dt * tScale; // independent CW carrier accumulator

            // Auto-deactivate solitons whose amplitude has decayed below audible.
            for (int k = 0; k < kOndaMaxSolPerVoice; ++k)
                if (v.solitons[k].active && v.solitons[k].amplitude < 0.005f)
                    v.solitons[k].active = false;

            // Output choice — Re(ψ) or |ψ|².
            float raw;
            if (observeMode == 0)
            {
                // Add CW background for Dark mode so dips have substrate.
                // voiceTime is the independent per-voice carrier accumulator —
                // avoids reading solitons[0].timeAlive which may be inactive.
                raw = sumRe + cwBg * std::cos(kOndaTwoPi * voiceFreq * v.voiceTime);
            }
            else
            {
                // Intensity reading.  In Dark mode this becomes (background − dip).
                raw = (polarity == 1) ? std::max(0.0f, cwBg * cwBg - sumIntSq)
                                      : sumIntSq;
                raw = raw * 2.0f - 0.2f; // center-ish on zero for AC coupling
            }

            // Soft-clip.
            const float shaped = std::tanh(raw * 0.6f) * 1.3f;

            // Per-sample cutoff.
            const float voiceCutoff = std::clamp(
                (baseCutoff + fe * fltEnvAmt * 6000.0f
                            + snapFilterMod * mCouplingGain * 4000.0f
                            + matrixCutoff * 5000.0f) * lfoCutoffScale,
                20.0f, sampleRateF * 0.49f);
            v.filter.setCoefficients(voiceCutoff, fltReso, sampleRateF);

            const float filtered = v.filter.processSample(shaped);
            const float ampBoost = std::clamp(1.0f + matrixAmp * 0.5f, 0.0f, 2.0f);

            // Widen voices slightly by soliton count — more packets = wider image.
            const float pan = std::clamp(0.15f * (activeCount - 2), -0.5f, 0.5f);
            const float gL = 0.7071f * (1.0f - pan);
            const float gR = 0.7071f * (1.0f + pan);

            const float out = filtered * a * ampBoost * widthScale;
            L[i] += out * gL;
            R[i] += out * gR;
        }
    }

    // ---- Output gain ----
    const float gTotal = level * (1.0f + aftertouch * 0.25f);
    juce::FloatVectorOperations::multiply(L, gTotal, numSamples);
    if (R != L) juce::FloatVectorOperations::multiply(R, gTotal, numSamples);

    // ---- Cache coupling output + silence gate ----
    const int n = std::min(numSamples, (int) couplingCacheL.size());
    std::memcpy(couplingCacheL.data(), L, (size_t) n * sizeof(float));
    if (R != L) std::memcpy(couplingCacheR.data(), R, (size_t) n * sizeof(float));
    else        std::memcpy(couplingCacheR.data(), L, (size_t) n * sizeof(float));

    analyzeForSilenceGate(buffer, numSamples);

    // Coupling accumulators are zeroed at block start (P25 capture-then-zero).
    // No end-of-block decay needed; applyCouplingInput accumulates additively.
} // renderBlock

} // namespace xoceanus
