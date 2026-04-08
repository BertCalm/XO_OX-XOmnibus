// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xoceanus
{

//==============================================================================
// ObrixEngine — Ocean Bricks: The Living Reef
//
// A runtime-configurable synthesis toy box where you snap "ocean bricks"
// together to build sound. OBRIX is not an instrument. It is a habitat.
// Where the other 43 engines are creatures with fixed identities — anglerfish,
// axolotl, octopus — OBRIX is the reef itself: the place where creatures live,
// the ecosystem that grows by accretion. It doesn't have a fixed form.
// It grows. It adapts. It responds to its inhabitants.
// New coral species (bricks) wash ashore with every Brick Drop.
//
// Historical lineage: ARP 2600 (semi-modular with normalled routing, 1971) →
//   Serge Modular ("patch-programmable" philosophy, 1974) → Korg MS-20 (dual
//   sources + dual filters, 1978). OBRIX inherits their semi-modular DNA:
//   a fixed signal path with reconfigurable bricks — constraint as creativity.
//
// Brick pool per voice (pre-allocated):
//   2 Sources (Shells)    — oscillators / noise (PolyBLEP anti-aliased)
//   3 Processors (Coral)  — Proc1 on Src1, Proc2 on Src2, Proc3 post-mix
//   4 Modulators (Currents) — envelopes / LFOs / velocity / aftertouch
//   3 Effects (Tide Pools) — delay / chorus / reverb in series
//
// Signal flow (the Constructive Collision):
//   Src1 ──[FM mod]──→ Src2
//   Src1 → Proc1(+fb) ─┐
//                       ├─ Mix → Proc3 → Amp → FX1→FX2→FX3
//   Src2 → Proc2(+fb) ─┘
//
// Wave 2 additions (60 params):
//   Source-to-source FM (obrix_fmDepth ±1 → ±24 st frequency deviation)
//   Filter feedback with tanh saturation (obrix_proc1/2Feedback 0→self-osc)
//   Real wavetable banks — Analog / Vocal / Metallic / Organic (obrix_wtBank)
//   Unison voice stacking with detune spread (obrix_unisonDetune 0–50 ct)
//
// Wave 3 additions (65 params total):
//   Drift Bus — global ultra-slow LFO (0.001-0.05 Hz) with per-voice irrational
//               phase offsets; Berlin School ensemble drift from one oscillator.
//               Modulates pitch (+/-50 ct) and filter cutoff (+/-200 Hz).
//               (obrix_driftRate, obrix_driftDepth)
//   Journey Mode — suppress note-off: sound evolves indefinitely (obrix_journeyMode)
//   Per-Brick Spatial — DISTANCE: HF rolloff via matched-Z 1-pole LP (air absorption)
//                       AIR: LP/HP split at 1 kHz with tilt gains (warm/cold atmosphere)
//                       Coefficients hoisted to block rate for efficiency.
//                       (obrix_distance, obrix_air)
//
// Wave 4 — Biophonic Synthesis (79 params):
//   Harmonic Field — global JI attractor/repulsor. Voices are pulled toward or away
//                    from 7-limit just-intonation ratios (3-limit / 5-limit / 7-limit).
//                    Per-voice jifiOffset[2] (cents) converges via 1st-order IIR.
//                    (obrix_fieldStrength, obrix_fieldPolarity, obrix_fieldRate,
//                     obrix_fieldPrimeLimit)
//   Environmental Parameters ("The Water") — four thermodynamic state variables:
//                    Temperature (entropy drift rate), Pressure (LFO rate scaling),
//                    Current (directional LFO/cutoff bias), Turbidity (spectral noise).
//                    (obrix_envTemp, obrix_envPressure, obrix_envCurrent, obrix_envTurbidity)
//   Brick Ecology — Competition (amplitude cross-suppression between src1/src2),
//                   Symbiosis (noise src1 amplitude → FM index on src2).
//                   (obrix_competitionStrength, obrix_symbiosisStrength)
//   Stateful Synthesis — Stress Memory (velocity leaky integrator, τ=30–60s → timbre),
//                        Bleaching (sustained high-register play → harmonic attenuation),
//                        State Reset trigger (explicit reef reset affordance).
//                        (obrix_stressDecay, obrix_bleachRate, obrix_stateReset)
//   FX Mode — Serial (default, existing behavior) vs Parallel (each slot on dry, summed).
//             (obrix_fxMode)
//
// Wave 5 — Reef Residency (82 params total):
//   Reef Residency — coupling input becomes a third ecological organism in the Brick
//                    Ecology system. Instead of linear pitch/cutoff modulation, the
//                    coupled engine participates as Competitor (amplitude cross-suppression),
//                    Symbiote (coupling amplitude drives FM depth + field strength), or
//                    Parasite (coupling energy accumulates stress and bleaching).
//                    (obrix_reefResident, obrix_residentStrength)
//
// Gallery code: OBRIX | Accent: Reef Jade #1E8B7E | Prefix: obrix_
// Blessing B016-AMENDED: MIDI-layer voice independence is inviolable. Synthesis-layer
//   interdependence (shared attractors, cross-voice amplitude relationships, environmental
//   globals) is permitted provided no synthesis-layer coupling propagates back to affect
//   MIDI routing or voice stealing.
//==============================================================================

//==============================================================================
// Brick Enums — extensible by design (new brick = new enum + new case)
// Mythology: Shells (sources) · Coral (processors) · Currents (modulators) · Tide Pools (effects)
//==============================================================================
enum class ObrixSourceType
{ // Shells — the living generators
    Off = 0,
    Sine,
    Saw,
    Square,
    Triangle,
    Noise,
    Wavetable,
    Pulse,
    LoFiSaw,
    kCount
};

enum class ObrixProcType
{ // Coral — the filter/transformer organisms
    Off = 0,
    LPFilter,
    HPFilter,
    BPFilter,
    Wavefolder,
    RingMod,
    kCount
};

enum class ObrixModType
{ // Currents — the modulators that flow between bricks
    Off = 0,
    ADSREnvelope,
    LFO,
    Velocity,
    Aftertouch,
    kCount
};

enum class ObrixEffectType
{ // Tide Pools — the spatial environment at the reef edge
    Off = 0,
    Delay,
    Chorus,
    Reverb,
    kCount
};

enum class ObrixModTarget
{
    None = 0,
    Pitch,
    FilterCutoff,
    FilterReso,
    Volume,
    WavetablePos,
    PulseWidth,
    EffectMix,
    Pan,
    kCount
};

// ObrixGestureType — PlaySurface pad behavior modes (maps to obrix_gestureMode param)
// The FLASH chromatophore: rapid light-and-color change, like octopus and cuttlefish
// signaling, startling, attracting. Four chromatophore patterns:
//   Ripple         — surface disturbance, quick filter sweep from above
//   Bioluminescent Pulse — deep-sea light emission, sustained glow, slow bloom
//   Undertow       — current reversal pulling inward, inverted envelope
//   Surge          — wave crashing over the reef crest, maximum energy
enum class ObrixGestureType
{
    Ripple = 0,
    Pulse,
    Flow,
    Tide,
    kCount
};

//==============================================================================
// ObrixADSR — alias to StandardADSR (shared fleet implementation).
// The four stages that describe the life of every note: Attack is urgency,
// Decay is settling, Sustain is patience, Release is farewell.
// API is identical: setParams(a,d,s,r,sr), noteOn(), noteOff(),
// isActive(), reset(), process().
//==============================================================================
using ObrixADSR = StandardADSR;

//==============================================================================
// ObrixLFO — alias to StandardLFO (shared fleet implementation).
// Supports 5 shapes (Sine/Triangle/Saw/Square/S&H), audio-rate capable
// (30Hz→1020Hz via MOVEMENT macro × rateMultiplier), setRate(hz, sr),
// reset(), process(). Shape field is public int, directly assignable.
//==============================================================================
using ObrixLFO = StandardLFO;

//==============================================================================
// ObrixFXState — per-slot effect state (3 independent FX slots)
//==============================================================================
struct ObrixFXState
{
    std::vector<float> delayBufL, delayBufR;
    int delayWritePos = 0;
    std::vector<float> chorusBufL, chorusBufR;
    int chorusWritePos = 0;
    float chorusLFOPhase = 0.0f;
    // FDN reverb: 8 feedback delay lines + 2 allpass input diffusors
    std::vector<float> reverbBuf[8];
    int reverbPos[8]{};
    float reverbFilt[8]{};
    std::vector<float> apBuf[2]; // allpass diffusion stages (142, 379 samples at 44.1kHz)
    int apPos[2]{};
    float apState[2]{};

    void prepare(float sr)
    {
        int maxDelay = static_cast<int>(sr * 0.5f) + 1;
        delayBufL.assign(static_cast<size_t>(maxDelay), 0.0f);
        delayBufR.assign(static_cast<size_t>(maxDelay), 0.0f);
        delayWritePos = 0;
        int chorusMax = static_cast<int>(sr * 0.03f) + 16; // +16 guards against sub-1ms at low sr
        chorusBufL.assign(static_cast<size_t>(chorusMax), 0.0f);
        chorusBufR.assign(static_cast<size_t>(chorusMax), 0.0f);
        chorusWritePos = 0;
        chorusLFOPhase = 0.0f;
        float srScale = sr / 44100.0f;
        // OBRIX-999: 8-line FDN — prime-spaced delays for dense, non-resonant reverb
        static constexpr int kRevLens[8] = {1087, 1283, 1511, 1789, 2011, 2333, 2677, 3001};
        for (int i = 0; i < 8; ++i)
        {
            int len = static_cast<int>(static_cast<float>(kRevLens[i]) * srScale) + 1;
            reverbBuf[i].assign(static_cast<size_t>(len), 0.0f);
            reverbPos[i] = 0;
            reverbFilt[i] = 0.0f;
        }
        // 2-stage allpass input diffusors (142, 379 samples at 44.1kHz)
        static constexpr int kApLens[2] = {142, 379};
        for (int i = 0; i < 2; ++i)
        {
            int len = static_cast<int>(static_cast<float>(kApLens[i]) * srScale) + 1;
            apBuf[i].assign(static_cast<size_t>(len), 0.0f);
            apPos[i] = 0;
            apState[i] = 0.0f;
        }
    }

    void reset()
    {
        std::fill(delayBufL.begin(), delayBufL.end(), 0.0f);
        std::fill(delayBufR.begin(), delayBufR.end(), 0.0f);
        delayWritePos = 0;
        std::fill(chorusBufL.begin(), chorusBufL.end(), 0.0f);
        std::fill(chorusBufR.begin(), chorusBufR.end(), 0.0f);
        chorusWritePos = 0;
        for (int i = 0; i < 8; ++i)
        {
            std::fill(reverbBuf[i].begin(), reverbBuf[i].end(), 0.0f);
            reverbFilt[i] = 0.0f;
        }
        for (int i = 0; i < 2; ++i)
        {
            std::fill(apBuf[i].begin(), apBuf[i].end(), 0.0f);
            apState[i] = 0.0f;
        }
    }
};

//==============================================================================
// ObrixVoice
//==============================================================================
struct ObrixVoice
{
    bool active = false;
    int note = -1;
    float velocity = 0.0f;
    float aftertouch = 0.0f;
    uint64_t startTime = 0;

    // Sources: PolyBLEP oscillators for anti-aliased types, manual phase for others
    PolyBLEP srcOsc[2];
    float srcPhase[2]{};
    float srcFreq[2]{};
    float targetFreq[2]{}; // portamento targets
    uint32_t noiseRng = 54321u;

    // Amplitude envelope
    ObrixADSR ampEnv;

    // 4 modulator slots (all wired)
    ObrixADSR modEnvs[4];
    ObrixLFO modLFOs[4];

    // 3 processor filters (Proc1→Src1, Proc2→Src2, Proc3→post-mix)
    CytomicSVF procFilters[3];

    // Cached cutoff/resonance per filter slot — used for delta-threshold guard.
    // setCoefficients() (which computes an expensive sin()) is skipped when the
    // new value differs by less than kFilterDeltaHz / kFilterDeltaRes.
    float procLastCut[3]{-1.0f, -1.0f, -1.0f}; // -1 forces update on first sample
    float procLastRes[3]{-1.0f, -1.0f, -1.0f};

    // Filter feedback state (Proc1, Proc2, Proc3) — one sample memory for the loop
    float procFbState[3]{};

    float pan = 0.0f;

    // Voice steal crossfade — 5ms linear ramp-down prevents clicks on polyphony overflow
    int stealFadeSamples = 0;   // samples remaining in fade-out (0 = not stealing)
    int stealFadeTotal   = 0;   // total fade length for this steal event
    // Pending note parameters — stored during the fade, applied when it completes
    int   pendingNote        = -1;
    float pendingVel         = 0.0f;
    float pendingFreq1       = 440.0f;
    float pendingFreq2       = 440.0f;
    float pendingPan         = 0.0f;
    int   pendingModTypes[4]{};
    float pendingModRates[4]{};
    float pendingAmpA = 0.01f, pendingAmpD = 0.3f, pendingAmpS = 0.7f, pendingAmpR = 0.3f;

    // Biophonic Phase 1 — Harmonic Field JI correction (cents per source, IIR state)
    float jifiOffset[2]{};

    // OBRIX-003: JI cents cache — avoids per-sample table lookup in findNearestJICents().
    // Recomputed only when the input ratio drifts beyond kJICacheThreshold.
    float cachedJIRatio[2]{-1.0f, -1.0f};
    float cachedJICents[2]{};

    void reset() noexcept
    {
        active = false;
        note = -1;
        velocity = 0.0f;
        aftertouch = 0.0f;
        for (auto& p : srcPhase)
            p = 0.0f;
        for (auto& f : srcFreq)
            f = 440.0f;
        for (auto& f : targetFreq)
            f = 440.0f;
        for (auto& o : srcOsc)
            o.reset();
        ampEnv.reset();
        for (auto& e : modEnvs)
            e.reset();
        for (auto& l : modLFOs)
            l.reset();
        for (auto& f : procFilters)
            f.reset();
        procFbState[0] = procFbState[1] = procFbState[2] = 0.0f;
        procLastCut[0] = procLastCut[1] = procLastCut[2] = -1.0f;
        procLastRes[0] = procLastRes[1] = procLastRes[2] = -1.0f;
        pan = 0.0f;
        jifiOffset[0] = jifiOffset[1] = 0.0f;
        cachedJIRatio[0] = cachedJIRatio[1] = -1.0f;
        cachedJICents[0] = cachedJICents[1] = 0.0f;
        stealFadeSamples = 0;
        stealFadeTotal   = 0;
        pendingNote      = -1;
    }
};

//==============================================================================
// ObrixEngine
//==============================================================================
class ObrixEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;

    // Delta-threshold for filter coefficient updates.
    // setCoefficients() (which calls std::sin internally) is skipped when the
    // new cutoff differs by less than kFilterDeltaHz and resonance by less than
    // kFilterDeltaRes.  At audio rates the modulation can change rapidly, so the
    // threshold is conservative: 1 Hz / 0.001 — perceptually transparent while
    // eliminating redundant trig on samples where nothing has moved.
    static constexpr float kFilterDeltaHz = 1.0f;
    static constexpr float kFilterDeltaRes = 0.001f;

    // OBRIX-003: JI ratio cache threshold. findNearestJICents() is only recomputed
    // when the octave-folded ratio changes by more than this amount. The JI target
    // moves slowly (driven by note fundamental + pitch mod), so 0.01 (~17 cents)
    // eliminates >95% of redundant table searches while remaining perceptually exact.
    static constexpr float kJICacheThreshold = 0.01f;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = static_cast<float>(sampleRate);
        for (auto& v : voices)
            v.reset();
        for (auto& fx : fxSlots)
            fx.prepare(sr);
        buildWavetables();
        // Cache reef HP coefficient at 2kHz (used in parasite mode; sample-rate dependent)
        reefHpCoeff_ = std::exp(-kTwoPi * 2000.0f / sr);
        // Invalidate stress decay coefficient cache so it is recomputed on the
        // first block after a sample-rate change (#620 block-rate caching).
        cachedStressDecay_ = -1.0f;
        // SRO SilenceGate: complex reef ecology has reverb tails — 500ms hold
        prepareSilenceGate(sampleRate, maxBlockSize, 500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        for (auto& fx : fxSlots)
            fx.reset();
        activeVoices.store(0, std::memory_order_relaxed);
        lastSampleL = lastSampleR = 0.0f;
        gestureLevel = 0.0f;
        gesturePhase = 0.0f;

        // Wave 3 — Drift Bus + Spatial filter state
        driftPhase_ = 0.0f;
        journeyMode_ = false;
        distFiltL_ = distFiltR_ = 0.0f;
        airFiltL_ = airFiltR_ = 0.0f;

        // Wave 4 — Biophonic Synthesis state
        stressLevel_ = 0.0f;
        bleachLevel_ = 0.0f;
        prevResetTrig_ = 0.0f;

        // Wave 5 — Reef Residency state
        reefCouplingRms_ = 0.0f;
        reefCouplingHpFilt_ = 0.0f;
        reefCouplingHfRms_ = 0.0f;
        reefCouplingBufLen_ = 0;

        // Coupling accumulators — must be zeroed on reset to prevent state leaking
        // across preset changes and AllNotesOff events
        couplingPitchMod.store(0.0f);
        couplingCutoffMod.store(0.0f);
        couplingWtPosMod.store(0.0f);
        couplingDecayMod.store(0.0f);
    }

    // ═══ SIGNAL TRACER ═══
    // Tap-point system for OBRIX Pocket's learning mode.
    // Read-only observers on the signal path — zero impact when disabled.

    enum class TracerTap : uint8_t
    {
        Src1Output = 0,  // After oscillator 1
        Src2Output = 1,  // After oscillator 2
        Proc1Output = 2, // After processor 1 (filter on Src1)
        Proc2Output = 3, // After processor 2 (filter on Src2)
        PostMix = 4,     // After Src1+Src2 blend
        Proc3Output = 5, // After post-mix insert
        PostFX = 6,      // After effects chain
        PostSpatial = 7, // After distance + air filters
        kNumTaps = 8
    };

    // Lock-free tap control — set from UI thread, read from audio thread
    std::atomic<int> activeTap_{-1}; // -1 = tracing disabled
    std::atomic<bool> tracerEnabled_{false};

    // Ring buffer per tap: stores last N samples for UI visualization
    static constexpr int kTapBufferSize = 512;
    std::array<std::array<float, kTapBufferSize>, 8> tapBuffers_{};
    std::array<std::atomic<int>, 8> tapWritePos_{};

    // Called from UI thread
    void enableTracer(int tapIndex)
    {
        activeTap_.store(tapIndex, std::memory_order_release);
        tracerEnabled_.store(true, std::memory_order_release);
    }

    void disableTracer()
    {
        tracerEnabled_.store(false, std::memory_order_release);
        activeTap_.store(-1, std::memory_order_release);
    }

    // Called from audio thread — inlined for zero overhead when disabled
    inline void recordTap(TracerTap tap, float sample) noexcept
    {
        if (!tracerEnabled_.load(std::memory_order_relaxed))
            return;
        if (activeTap_.load(std::memory_order_relaxed) != static_cast<int>(tap))
            return;
        auto idx = static_cast<size_t>(tap);
        int pos = tapWritePos_[idx].load(std::memory_order_relaxed);
        tapBuffers_[idx][static_cast<size_t>(pos)] = sample;
        tapWritePos_[idx].store((pos + 1) % kTapBufferSize, std::memory_order_relaxed);
    }

    // Called from UI thread to read tap data for visualization
    void getTapData(TracerTap tap, float* dest, int numSamples) const
    {
        auto idx = static_cast<size_t>(tap);
        int writePos = tapWritePos_[idx].load(std::memory_order_acquire);
        for (int i = 0; i < numSamples && i < kTapBufferSize; ++i)
        {
            int readIdx = (writePos - numSamples + i + kTapBufferSize) % kTapBufferSize;
            dest[i] = tapBuffers_[idx][static_cast<size_t>(readIdx)];
        }
    }

    //==========================================================================
    // Audio — The Constructive Collision
    // ARCHITECTURE NOTE (Wave 5 target): This method is intentionally monolithic
    // to avoid per-sample function call overhead in the inner voice loop.
    // Future factoring should use JUCE_FORCEINLINE helpers: processGestureEnvelope(),
    // processVoiceSample(), and processSpatial(). Test with clang -O2 before changing.
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (sr <= 0.0f) { buffer.clear(); return; }
        if (numSamples <= 0)
            return;

        // SRO SilenceGate: wake on note-on, bypass when silent
        for (const auto& md : midi)
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            couplingPitchMod.store(0.0f);
            couplingCutoffMod.store(0.0f);
            couplingWtPosMod.store(0.0f);
            couplingDecayMod.store(0.0f);
            return;
        }

        // === ParamSnapshot ===
        const auto src1Type = static_cast<int>(loadP(pSrc1Type, 2.0f)); // 2=Saw default
        const auto src2Type = static_cast<int>(loadP(pSrc2Type, 0.0f));
        const float src1Tune = loadP(pSrc1Tune, 0.0f);
        const float src2Tune = loadP(pSrc2Tune, 0.0f);
        const float src1PW = loadP(pSrc1PW, 0.5f);
        const float src2PW = loadP(pSrc2PW, 0.5f);
        const float srcMix = loadP(pSrcMix, 0.5f);

        const auto proc1Type = static_cast<int>(loadP(pProc1Type, 1.0f));
        const float proc1Cut = loadP(pProc1Cutoff, 8000.0f);
        const float proc1Res = loadP(pProc1Reso, 0.0f);
        const auto proc2Type = static_cast<int>(loadP(pProc2Type, 0.0f));
        const float proc2Cut = loadP(pProc2Cutoff, 4000.0f);
        const float proc2Res = loadP(pProc2Reso, 0.0f);
        const auto proc3Type = static_cast<int>(loadP(pProc3Type, 0.0f));
        const float proc3Cut = loadP(pProc3Cutoff, 4000.0f);
        const float proc3Res = loadP(pProc3Reso, 0.0f);

        const float ampA = loadP(pAmpAttack, 0.01f);
        // EnvToDecay coupling: scale decay time multiplicatively; couplingDecayMod consumed early
        // so that noteOn calls in the MIDI loop below receive the modulated value.
        float ampD = loadP(pAmpDecay, 0.3f);
        {
            float decayMod = couplingDecayMod.exchange(0.0f);
            if (decayMod != 0.0f)
            {
                float clampedMod = clamp(decayMod, -0.8f, 4.0f);
                ampD = std::max(0.01f, ampD * (1.0f + clampedMod));
            }
        }
        const float ampS = loadP(pAmpSustain, 0.7f);
        const float ampR = loadP(pAmpRelease, 0.5f);

        // 4 modulator slots
        const int modType[4] = {static_cast<int>(loadP(pModType[0], 1.0f)), static_cast<int>(loadP(pModType[1], 2.0f)),
                                static_cast<int>(loadP(pModType[2], 3.0f)), static_cast<int>(loadP(pModType[3], 4.0f))};
        const int modTgt[4] = {
            static_cast<int>(loadP(pModTarget[0], 2.0f)), static_cast<int>(loadP(pModTarget[1], 2.0f)),
            static_cast<int>(loadP(pModTarget[2], 4.0f)), static_cast<int>(loadP(pModTarget[3], 2.0f))};
        const float modDepth[4] = {loadP(pModDepth[0], 0.5f), loadP(pModDepth[1], 0.0f), loadP(pModDepth[2], 0.5f),
                                   loadP(pModDepth[3], 0.0f)};
        const float modRate[4] = {loadP(pModRate[0], 1.0f), loadP(pModRate[1], 1.0f), loadP(pModRate[2], 1.0f),
                                  loadP(pModRate[3], 1.0f)};

        // 3 FX slots
        const int fxType[3] = {static_cast<int>(loadP(pFxType[0], 0.0f)), static_cast<int>(loadP(pFxType[1], 0.0f)),
                               static_cast<int>(loadP(pFxType[2], 0.0f))};
        const float fxMix[3] = {loadP(pFxMix[0], 0.0f), loadP(pFxMix[1], 0.0f), loadP(pFxMix[2], 0.0f)};
        const float fxParam[3] = {loadP(pFxParam[0], 0.3f), loadP(pFxParam[1], 0.3f), loadP(pFxParam[2], 0.3f)};

        const float level = loadP(pLevel, 0.8f);
        const float macroChar = loadP(pMacroChar, 0.0f);
        const float macroMove = loadP(pMacroMove, 0.0f);
        const float macroCoup = loadP(pMacroCoup, 0.0f);
        const float macroSpace = loadP(pMacroSpace, 0.0f);

        const float bendRange = loadP(pPitchBendRange, 2.0f);
        const float glideTime = loadP(pGlideTime, 0.0f);

        const auto gestureType = static_cast<int>(loadP(pGestureType, 0.0f));
        const float flashTrig = loadP(pFlashTrigger, 0.0f);

        // Wave 2 params
        const float fmDepth = loadP(pFmDepth, 0.0f);
        const float proc1Fb = loadP(pProc1Fb, 0.0f);
        const float proc2Fb = loadP(pProc2Fb, 0.0f);
        const float proc3Fb = loadP(pProc3Fb, 0.0f);
        const auto wtBankVal = static_cast<int>(loadP(pWtBank, 0.0f));
        const float unisonDetune = loadP(pUnisonDetune, 0.0f);

        // Wave 3 params — Drift Bus, Journey, Spatial
        const float driftRate = loadP(pDriftRate, 0.005f); // Hz: 0.001–0.05
        const float driftDepth = loadP(pDriftDepth, 0.03f); // OBRIX-1002: subtle default drift (gentle ensemble breathing)
        const bool journeyOn = loadP(pJourneyMode, 0.0f) > 0.5f;
        const float distance = loadP(pDistance, 0.0f); // 0=close, 1=far (HF rolloff)
        const float air = loadP(pAir, 0.5f);           // 0=warm(bass), 1=cold(treble)
        journeyMode_ = journeyOn;                      // cached for noteOff suppression in MIDI handler

        // Wave 4 params — Biophonic Synthesis (all default 0 for backward compat)
        const float fieldStrength = loadP(pFieldStrength, 0.12f);                      // OBRIX-1002: gentle JI attraction (first keypress reveals the reef)
        const float fieldPolarity = loadP(pFieldPolarity, 1.0f);                      // 1=full attractor
        const float fieldRate = loadP(pFieldRate, 0.01f);                             // IIR convergence rate
        const auto fieldPrimeLimit = static_cast<int>(loadP(pFieldPrimeLimit, 1.0f)); // 0=3-limit,1=5-limit,2=7-limit
        const float envTemp = loadP(pEnvTemp, 0.35f);                                  // OBRIX-1002: warm oceanic default temperature
        const float envPressure = loadP(pEnvPressure, 0.5f);                          // 0.5=neutral
        const float envCurrent = loadP(pEnvCurrent, 0.0f);                            // 0=no bias
        const float envTurbidity = loadP(pEnvTurbidity, 0.02f);                        // OBRIX-1002: faint spectral shimmer by default
        const float competitionStrength = loadP(pCompetitionStrength, 0.0f);          // 0=off
        const float symbiosisStrength = loadP(pSymbiosisStrength, 0.0f);              // 0=off
        const float stressDecay = loadP(pStressDecay, 0.0f);                          // 0=off
        const float bleachRate = loadP(pBleachRate, 0.0f);                            // 0=off
        const float newResetTrig = loadP(pStateReset, 0.0f);
        const auto fxMode = static_cast<int>(loadP(pFxMode, 0.0f)); // 0=Serial

        // Wave 5 params — Reef Residency (82 total)
        const auto reefResident = static_cast<int>(loadP(pReefResident, 0.0f)); // 0=Off
        const float residentStrength = (reefResident != 0) ? loadP(pResidentStrength, 0.3f) : 0.0f;

        // === REEF RESIDENCY: Pre-compute coupling buffer RMS (block-rate) ===
        // Coupling buffer was stored by applyCouplingInput() before renderBlock() runs.
        float reefRms = 0.0f;
        float reefHfRms = 0.0f;
        if (reefResident != 0 && reefCouplingBufLen_ > 0)
        {
            // Full-band RMS of coupling input
            float sumSq = 0.0f;
            for (int i = 0; i < reefCouplingBufLen_; ++i)
                sumSq += reefCouplingBuf_[i] * reefCouplingBuf_[i];
            reefRms = std::sqrt(sumSq / static_cast<float>(reefCouplingBufLen_));
            reefCouplingRms_ = reefCouplingRms_ * 0.9f + reefRms * 0.1f; // smooth
            reefCouplingRms_ = flushDenormal(reefCouplingRms_);

            // Parasite mode: high-frequency content via 1-pole HP at 2kHz
            if (reefResident == 3)
            {
                float hpCoeff = reefHpCoeff_; // cached in prepare() — avoids std::exp on audio thread
                float hfSumSq = 0.0f;
                for (int i = 0; i < reefCouplingBufLen_; ++i)
                {
                    float in = reefCouplingBuf_[i];
                    reefCouplingHpFilt_ = hpCoeff * (reefCouplingHpFilt_ + in - reefCouplingBufPrev_);
                    reefCouplingBufPrev_ = flushDenormal(in);
                    reefCouplingHpFilt_ = flushDenormal(reefCouplingHpFilt_);
                    hfSumSq += reefCouplingHpFilt_ * reefCouplingHpFilt_;
                }
                reefHfRms = std::sqrt(hfSumSq / static_cast<float>(reefCouplingBufLen_));
                reefCouplingHfRms_ = reefCouplingHfRms_ * 0.9f + reefHfRms * 0.1f;
                reefCouplingHfRms_ = flushDenormal(reefCouplingHfRms_);
            }
        }
        reefCouplingBufLen_ = 0; // consumed

        // State Reset: rising edge triggers hard reef reset
        if (newResetTrig > 0.5f && prevResetTrig_ <= 0.5f)
        {
            stressLevel_ = 0.0f;
            bleachLevel_ = 0.0f;
        }
        prevResetTrig_ = newResetTrig;

        // Stateful Synthesis block-level updates
        // Stress Memory: leaky integrator τ = 30+stressDecay*30 seconds
        // Model: RC low-pass filter with time constant τ (exponential moving average)
        // #620: cache the stress decay coefficient — stressDecay changes at most
        // once per parameter automation step; recomputing std::exp every block is wasteful.
        if (std::abs(stressDecay - cachedStressDecay_) > 0.0005f)
        {
            float tau = 30.0f + stressDecay * 30.0f;
            cachedStressDecayCoeff_  = std::exp(-1.0f / (tau * sr));
            cachedParasiteAlpha_     = 1.0f - cachedStressDecayCoeff_;
            cachedParasiteDecay0_    = std::exp(-1.0f / (30.0f * sr)); // τ=30s fixed path
            cachedStressDecay_       = stressDecay;
        }
        if (stressDecay > 0.001f)
        {
            float avgVel = 0.0f;
            int vcount = 0;
            for (const auto& v : voices)
                if (v.active)
                {
                    avgVel += v.velocity;
                    ++vcount;
                }
            if (vcount > 0)
                avgVel /= static_cast<float>(vcount);
            const float decay = cachedStressDecayCoeff_;
            stressLevel_ = stressLevel_ * decay + avgVel * (1.0f - decay);
            stressLevel_ = juce::jlimit(0.0f, 1.0f, stressLevel_); // #672: clamp both sides
            stressLevel_ = flushDenormal(stressLevel_);
        }
        // Bleaching: cumulative brightness attenuation from sustained high-register playing
        if (bleachRate > 0.001f)
        {
            float avgBright = 0.0f;
            int bcount = 0;
            for (const auto& v : voices)
                if (v.active && v.note > 60)
                {
                    avgBright += (v.note - 60) / 48.0f;
                    ++bcount;
                }
            if (bcount > 0)
                avgBright /= static_cast<float>(bcount);
            bleachLevel_ = std::min(1.0f, bleachLevel_ + avgBright * bleachRate / (sr * 60.0f));
            bleachLevel_ *= std::pow(1.0f - 1.0f / (sr * 300.0f), static_cast<float>(numSamples)); // natural recovery ~5 min
            bleachLevel_ = flushDenormal(bleachLevel_);
        }

        // === REEF RESIDENCY: Parasite — coupling energy feeds stress and bleach ===
        // Parasite contributions are additive to existing velocity-driven stress/bleach.
        // When stressDecay is active, the velocity block above has already updated
        // stressLevel_ via the leaky integrator. We add the parasite's contribution
        // as an injection term to avoid double-decaying the state.
        if (reefResident == 3 && residentStrength > 0.001f && reefCouplingRms_ > 0.001f)
        {
            float parasiteStressInput = reefCouplingRms_ * residentStrength;
            if (stressDecay > 0.001f)
            {
                // Velocity stress block already ran — just inject parasite contribution
                // Use cached alpha (1 - decay) to avoid another std::exp call.
                stressLevel_ += parasiteStressInput * cachedParasiteAlpha_;
                stressLevel_ = juce::jlimit(0.0f, 1.0f, stressLevel_); // #672: clamp both sides
                stressLevel_ = flushDenormal(stressLevel_);
            }
            else
            {
                // stressDecay=0 means no velocity stress runs — parasite drives the full integrator
                // cachedParasiteDecay0_ holds exp(-1/(30*sr)) computed at block boundary.
                const float decay = cachedParasiteDecay0_;
                stressLevel_ = stressLevel_ * decay + parasiteStressInput * (1.0f - decay);
                stressLevel_ = juce::jlimit(0.0f, 1.0f, stressLevel_); // #672: clamp both sides
                stressLevel_ = flushDenormal(stressLevel_);
            }

            // High-frequency coupling content contributes to bleaching
            // Same additive pattern: only inject if bleachRate block already ran
            if (reefCouplingHfRms_ > 0.01f)
            {
                float parasiteBleachInput = reefCouplingHfRms_ * residentStrength;
                bleachLevel_ = std::min(1.0f, bleachLevel_ + parasiteBleachInput / (sr * 60.0f));
                if (bleachRate <= 0.001f) // no natural recovery from velocity block — apply here
                    bleachLevel_ *= std::pow(1.0f - 1.0f / (sr * 300.0f), static_cast<float>(numSamples));
                bleachLevel_ = flushDenormal(bleachLevel_);
            }
        }

        // Pre-compute spatial filter coefficients once per block (not per sample)
        // DISTANCE: fc sweeps 20kHz (close) → ~1kHz (far) via matched-Z 1-pole LP
        const float distFc = 20000.0f * (1.0f - distance * 0.95f);
        const float distCoeff = 1.0f - std::exp(-kTwoPi * distFc / sr);
        // AIR: fixed 1kHz LP/HP split for spectral tilt
        const float airCoeff = 1.0f - std::exp(-kTwoPi * 1000.0f / sr);
        // air=0 → lpGain=1.3, hpGain=0.7; air=0.5 → both 1.0; air=1 → lpGain=0.7, hpGain=1.3
        const float airLpGain = 1.0f + (0.5f - air) * 0.6f;
        const float airHpGain = 1.0f + (air - 0.5f) * 0.6f;

        const int voiceModeIdx = static_cast<int>(loadP(pVoiceMode, 3.0f));
        const int polyLimit = (voiceModeIdx <= 1) ? 1 : (voiceModeIdx == 2) ? 4 : 8;
        // OBRIX-001: Release orphaned voices when polyLimit shrinks (e.g. poly→mono).
        // Without this, voices in slots beyond the new limit continue sounding
        // indefinitely because noteOn() only allocates within the new limit.
        if (polyLimit < polyLimit_)
        {
            for (int i = polyLimit; i < kMaxVoices; ++i)
                if (voices[i].active)
                    voices[i].ampEnv.noteOff();
        }
        polyLimit_ = polyLimit;

        // Glide coefficient: 0 = instant, approaching 1 = very slow
        const float glideCoeff = (glideTime > 0.001f) ? 1.0f - std::exp(-1.0f / (glideTime * sr)) : 1.0f;

        // === FLASH gesture trigger (detect rising edge) ===
        if (flashTrig > 0.5f && prevFlashTrig <= 0.5f)
        {
            gestureLevel = 1.0f;
            gesturePhase = 0.0f;
        }
        prevFlashTrig = flashTrig;

        // === MIDI ===
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                // Primary voice
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), ampA, ampD, ampS, ampR, src1Tune, src2Tune, modType,
                       modRate, voiceModeIdx, glideCoeff, 0.0f, 0.0f, false);
                // Unison extra voices (derived from voice mode)
                const int unisonSize = calcUnisonSize(voiceModeIdx, unisonDetune);
                for (int u = 1; u < unisonSize; ++u)
                {
                    float spread = (unisonSize > 1)
                        ? (static_cast<float>(u) / static_cast<float>(unisonSize - 1) * 2.0f - 1.0f)
                        : 0.0f;
                    float detST = spread * unisonDetune / 100.0f;
                    float panOff = spread * 0.75f;
                    noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), ampA, ampD, ampS, ampR, src1Tune, src2Tune,
                           modType, modRate, voiceModeIdx, glideCoeff, detST, panOff, true);
                }
            }
            else if (msg.isNoteOff())
            {
                if (!journeyMode_) // Journey mode: suppress note-off — sound evolves indefinitely
                    noteOff(msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isChannelPressure())
            {
                // Braces added to prevent dangling-else with the subsequent else-if chain.
                for (auto& v : voices)
                    if (v.active)
                        v.aftertouch = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
                pitchBend_ = (msg.getPitchWheelValue() - 8192) / 8192.0f;
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheel_ = msg.getControllerValue() / 127.0f;
        }

        // === Consume coupling accumulators ===
        float blockPitchCoupling = couplingPitchMod.exchange(0.0f);
        float blockCutoffCoupling = couplingCutoffMod.exchange(0.0f);
        float blockWtPosCoupling = couplingWtPosMod.exchange(0.0f);
        // couplingDecayMod was already consumed and reset in the ParamSnapshot section above
        // (applied to ampD before the MIDI noteOn loop). couplingRingMod routes through
        // pitchMod/cutoffMod in applyCouplingInput — no separate block variable needed.

        // === Sample Loop ===
        for (int s = 0; s < numSamples; ++s)
        {
            float mixL = 0.0f, mixR = 0.0f;

            // FLASH gesture envelope (decays globally)
            float gestureOut = 0.0f;
            if (gestureLevel > 0.001f)
            {
                gesturePhase += 5.0f / sr;
                if (gesturePhase >= 1.0f)
                    gesturePhase -= 1.0f; // OBRIX-1000: wrap to prevent float precision loss over time
                switch (gestureType)
                {
                case 0:
                    gestureOut = gestureLevel * fastSin(gesturePhase * kTwoPi * 8.0f);
                    break;
                case 1:
                    gestureOut = gestureLevel;
                    break;
                case 2:
                    gestureOut = gestureLevel * fastSin(gesturePhase * kTwoPi * 2.0f);
                    break;
                case 3:
                    gestureOut = gestureLevel * (4.0f * std::fabs(gesturePhase - 0.5f) - 1.0f);
                    break;
                }
                gestureLevel *= (1.0f - 3.0f / sr);
                gestureLevel = flushDenormal(gestureLevel);
            }

            // === DRIFT BUS (Wave 3) — Schulze's ultra-slow ensemble LFO ===
            // One global oscillator ticked per sample. Per-voice slot offsets
            // are applied in the voice loop below (see kDriftSlotOffset).
            driftPhase_ += driftRate / sr;
            if (driftPhase_ >= 1.0f)
                driftPhase_ -= 1.0f;

            // === BIOPHONIC: Turbidity noise (computed once per sample, applied per voice) ===
            // Turbidity adds spectral impurity — subtle noise-floor shimmer on filter cutoff.
            float turbNoiseThisSample = 0.0f;
            if (envTurbidity > 0.001f)
            {
                turbRng_ = turbRng_ * 1664525u + 1013904223u;
                turbNoiseThisSample =
                    (static_cast<float>(turbRng_ & 0xFFFF) / 32768.0f - 1.0f) * envTurbidity * 1200.0f;
            }

            for (int vi = 0; vi < kMaxVoices; ++vi)
            {
                auto& voice = voices[vi];
                if (!voice.active)
                    continue;

                // --- Voice steal crossfade ---
                // stealFade linearly ramps from 1.0 → 0.0 over stealFadeTotal samples.
                // When the ramp hits 0 the pending note is installed via initVoice().
                float stealFade = 1.0f;
                if (voice.stealFadeSamples > 0)
                {
                    stealFade = static_cast<float>(voice.stealFadeSamples)
                                / static_cast<float>(std::max(1, voice.stealFadeTotal));
                    --voice.stealFadeSamples;
                    if (voice.stealFadeSamples == 0 && voice.pendingNote >= 0)
                    {
                        // Fade complete — initialise the pending note in-place
                        initVoice(voice, vi,
                                  voice.pendingNote, voice.pendingVel,
                                  voice.pendingFreq1, voice.pendingFreq2,
                                  voice.pendingPan,
                                  voice.pendingAmpA, voice.pendingAmpD,
                                  voice.pendingAmpS, voice.pendingAmpR,
                                  voice.pendingModTypes, voice.pendingModRates);
                        voice.pendingNote = -1;
                        continue; // New note starts on the next sample
                    }
                }

                // --- Portamento ---
                for (int i = 0; i < 2; ++i)
                {
                    voice.srcFreq[i] += (voice.targetFreq[i] - voice.srcFreq[i]) * glideCoeff;
                    voice.srcFreq[i] = flushDenormal(voice.srcFreq[i]); // OBRIX-1001: prevent denormal CPU spikes
                }

                // --- All 4 Modulators ---
                // MOVEMENT macro scales LFO depth BEFORE routing
                float moveScale = 1.0f + macroMove * 3.0f;
                // DSP Fix Wave 2B: MOVEMENT macro also multiplies LFO RATE,
                // unlocking audio-rate modulation (up to 30Hz * 34x = 1020 Hz).
                // This lifts the 30 Hz LFO ceiling flagged in the OBRIX seance
                // without changing the frozen parameter range. Players access
                // audio-rate crossover by pushing the MOVEMENT macro.
                // modWheel further scales rate: at full modWheel+MOVEMENT, LFOs
                // can reach ~1000 Hz for true audio-rate FM/AM territory.
                // Biophonic Pressure: scales LFO rate (high=fast/tense, low=slow/spacious)
                float pressureScale =
                    (envPressure > 0.5f)
                        ? 1.0f + (envPressure - 0.5f) * 2.0f * 3.0f                  // 0.5→1: scale 1.0→4.0
                        : std::max(0.3f, 1.0f - (0.5f - envPressure) * 2.0f * 0.7f); // 0.5→0: scale 1.0→0.3
                float rateMultiplier = (1.0f + macroMove * 10.0f + modWheel_ * 23.0f) * pressureScale;
                float modVals[4]{};
                for (int m = 0; m < 4; ++m)
                {
                    if (modType[m] == 1)
                        modVals[m] = voice.modEnvs[m].process() * modDepth[m];
                    else if (modType[m] == 2)
                    {
                        // Apply rate multiplier by advancing extra phase
                        voice.modLFOs[m].setRate(modRate[m] * rateMultiplier, sr);
                        modVals[m] = voice.modLFOs[m].process() * modDepth[m] * moveScale;
                    }
                    else if (modType[m] == 3)
                        modVals[m] = voice.velocity * modDepth[m];
                    else if (modType[m] == 4)
                        modVals[m] = voice.aftertouch * modDepth[m];
                }

                // --- Route modulation to all targets ---
                // 4 Currents × 8 destinations = 32 possible routings
                // This is the reef's nervous system — modulation flows like water
                // through every brick, touching pitch, timbre, space, and expression
                float pitchMod = 0.0f, cutoffMod = 0.0f, resoMod = 0.0f;
                float volMod = 0.0f, wtPosMod = 0.0f, pwMod = 0.0f;
                float fxMixMod = 0.0f, panMod = 0.0f;

                for (int m = 0; m < 4; ++m)
                {
                    switch (modTgt[m])
                    {
                    case 1:
                        pitchMod += modVals[m] * 1200.0f;
                        break;
                    case 2:
                        cutoffMod += modVals[m] * 6000.0f;
                        break;
                    case 3:
                        resoMod += modVals[m];
                        break;
                    case 4:
                        volMod += modVals[m];
                        break;
                    case 5:
                        wtPosMod += modVals[m];
                        break;
                    case 6:
                        pwMod += modVals[m];
                        break;
                    case 7:
                        fxMixMod += modVals[m];
                        break;
                    case 8:
                        panMod += modVals[m];
                        break;
                    default:
                        break;
                    }
                }


                // --- Macro modulation (Guru Bin remapping) ---
                // CHARACTER: cutoff + exponential fold depth + resonance boost
                float charFoldScale =
                    1.0f + macroChar * macroChar * 8.0f; // exponential fold: 1× at zero → 9× drive at full
                cutoffMod += macroChar * 3000.0f;        // CHARACTER sweeps up to +3 kHz (dark→bright timbral shift)
                resoMod += macroChar * 0.3f; // slight resonance boost preserves filter character at high drive

                // MOVEMENT: stereo detune (+1 semitone = 100 cents at full)
                pitchMod += macroMove * 100.0f; // MOVEMENT: +1 semitone stereo detune spread at maximum

                // D006: mod wheel intensifies filter sweep
                cutoffMod += modWheel_ * 4000.0f; // mod wheel sweeps up to +4 kHz (full range of musical interest)

                // === BIOPHONIC: Environmental Parameters (Wave 4) ===
                // Current: directional bias on filter center and pitch
                cutoffMod += envCurrent * 2000.0f;
                pitchMod += envCurrent * 80.0f;
                // Turbidity: spectral noise from shared per-sample turbidity RNG
                cutoffMod += turbNoiseThisSample;
                // Stress Memory application: high stress → raised cutoff (brighter, harder)
                if (stressDecay > 0.001f)
                    cutoffMod += stressLevel_ * 900.0f * stressDecay;
                // Bleaching application: accumulated bleach → lower cutoff (duller, quieter)
                if (bleachRate > 0.001f && bleachLevel_ > 0.01f)
                    cutoffMod -= bleachLevel_ * 700.0f * bleachRate;

                // FLASH gesture → filter cutoff burst
                cutoffMod += gestureOut * 4000.0f;

                // Coupling modulation
                pitchMod  += blockPitchCoupling  * 100.0f;
                cutoffMod += blockCutoffCoupling * 3000.0f;
                wtPosMod  += blockWtPosCoupling;  // EnvToMorph: direct ±1 addition to morph position

                // OBRIX-002: COUPLING macro self-routing fallback (Kakehashi/Smith seance).
                // When no partner engine is coupled (accumulators + reef all zero), the macro
                // creates internal shimmer between active bricks: a slow chorus-like pitch
                // micro-detune and filter breathing derived from the Drift Bus phase with
                // per-voice offset. At low values this is subtle warmth; at high values
                // it becomes expressive internal cross-modulation. The shimmer uses an
                // independent rate (~3 Hz base × voice offset) so it doesn't correlate
                // with existing LFOs — it feels like the bricks breathing together.
                bool hasCouplingInput = (std::fabs(blockPitchCoupling)  > 0.001f ||
                                         std::fabs(blockCutoffCoupling) > 0.001f ||
                                         std::fabs(blockWtPosCoupling)  > 0.001f ||
                                         reefCouplingRms_ > 0.001f);
                if (macroCoup > 0.001f && !hasCouplingInput)
                {
                    float shimmerPhase = driftPhase_ * 60.0f + static_cast<float>(vi) * 0.37f;
                    float shimmer = fastSin(shimmerPhase * kTwoPi) * macroCoup;
                    pitchMod += shimmer * 12.0f;      // ±12 cents micro-detune (gentle chorus)
                    cutoffMod += shimmer * 600.0f;    // ±600 Hz filter breathing
                }

                // COUPLING macro scales sensitivity
                pitchMod *= (1.0f + macroCoup * 2.0f);
                cutoffMod *= (1.0f + macroCoup * 1.0f);

                // Pitch bend
                pitchMod += pitchBend_ * bendRange * 100.0f;

                // === DRIFT BUS per-voice application (Wave 3) ===
                // Each voice slot reads the global ultra-slow LFO at a unique phase
                // offset. 0.23 is irrational relative to 1.0, so no two voice slots
                // ever phase-lock — producing Berlin School ensemble drift from one
                // oscillator (Schulze's trick).
                // Temperature amplifies drift (thermal energy → increased entropy)
                float effectiveDriftDepth = driftDepth * (1.0f + envTemp * 3.0f);
                if (effectiveDriftDepth > 0.001f)
                {
                    static constexpr float kDriftSlotOffset = 0.23f; // irrational spacing
                    float voiceDrift = fastSin((driftPhase_ + vi * kDriftSlotOffset) * kTwoPi) * effectiveDriftDepth;
                    pitchMod += voiceDrift * 50.0f;   // +-50 cents pitch drift at full depth
                    cutoffMod += voiceDrift * 200.0f; // +-200 Hz filter drift (subtle harmonic breathing)
                }

                // D001: Velocity shapes timbre — cutoff AND wavefolder depth
                float velTimbre = voice.velocity * 2000.0f;
                float velFoldBoost = 1.0f + voice.velocity * 2.0f;

                // --- Compute frequencies ---
                float freq1 = voice.srcFreq[0] * fastPow2(pitchMod / 1200.0f);
                float freq2 = voice.srcFreq[1] * fastPow2(pitchMod / 1200.0f);
                float effPW1 = clamp(src1PW + pwMod, 0.05f, 0.95f);
                float effPW2 = clamp(src2PW + pwMod, 0.05f, 0.95f);

                // === HARMONIC FIELD (Biophonic Phase 1) ===
                // JI attractor/repulsor: each source frequency is pulled toward (or away
                // from) the nearest just-intonation ratio in the selected prime limit.
                // Per-voice IIR convergence (jifiOffset) gives smooth magnetic drift.
                // === REEF RESIDENCY: Symbiote gently boosts field strength ===
                float effFieldStrength = fieldStrength;
                if (reefResident == 2 && residentStrength > 0.001f && reefCouplingRms_ > 0.001f &&
                    fieldStrength > 0.001f)
                    effFieldStrength = std::min(1.0f, fieldStrength + reefCouplingRms_ * residentStrength * 0.3f);
                if (effFieldStrength > 0.001f)
                {
                    float baseFreq = 440.0f * fastPow2((static_cast<float>(voice.note) - 69.0f) / 12.0f);
                    for (int si = 0; si < 2; ++si)
                    {
                        if (si == 1 && src2Type == 0)
                            continue;
                        float srcF = (si == 0) ? freq1 : freq2;
                        float ratio = srcF / std::max(1.0f, baseFreq);
                        while (ratio >= 2.0f)
                            ratio *= 0.5f;
                        while (ratio < 1.0f)
                            ratio *= 2.0f;
                        // OBRIX-003: Cache JI cents lookup. The ratio changes slowly (note
                        // fundamental + pitch mod), so we skip the table search when the
                        // octave-folded ratio hasn't moved beyond kJICacheThreshold.
                        if (std::fabs(ratio - voice.cachedJIRatio[si]) > kJICacheThreshold)
                        {
                            voice.cachedJICents[si] = findNearestJICents(ratio, fieldPrimeLimit);
                            voice.cachedJIRatio[si] = ratio;
                        }
                        float jiCents = voice.cachedJICents[si];
                        // polarity > 0.5 = attract, < 0.5 = repel
                        float polFactor =
                            (fieldPolarity >= 0.5f) ? (fieldPolarity - 0.5f) * 2.0f : -(0.5f - fieldPolarity) * 2.0f;
                        float targetOffset = jiCents * polFactor;
                        voice.jifiOffset[si] += fieldRate * (targetOffset * effFieldStrength - voice.jifiOffset[si]);
                        voice.jifiOffset[si] = flushDenormal(voice.jifiOffset[si]);
                    }
                    freq1 *= fastPow2(voice.jifiOffset[0] / 1200.0f);
                    if (src2Type > 0)
                        freq2 *= fastPow2(voice.jifiOffset[1] / 1200.0f);
                }

                // Apply WT position modulation to morph position for wavetable sources
                float effPW1wt = (src1Type == 6) ? clamp(effPW1 + wtPosMod, 0.05f, 0.95f) : effPW1;
                float effPW2wt = (src2Type == 6) ? clamp(effPW2 + wtPosMod, 0.05f, 0.95f) : effPW2;

                // --- Source 1 (rendered first — drives FM into Src2) ---
                float src1 = renderSourceSample(src1Type, voice, 0, freq1, effPW1wt, 0.0f, wtBankVal);
                recordTap(TracerTap::Src1Output, src1);

                // --- Source 2 (Src1 FM-modulates Src2 frequency at audio rate) ---
                float src2 = 0.0f;
                if (src2Type > 0)
                {
                    float fmSemitones = src1 * fmDepth * 24.0f; // ±24 st at max depth
                    // === BRICK ECOLOGY: Symbiosis (Biophonic Phase 3) ===
                    // When src1 is Noise, its amplitude (spectral density) drives extra FM on
                    // src2 — noise energy feeds tonal modulation (noise nurtures the oscillator).
                    if (symbiosisStrength > 0.001f && src1Type == 5)
                        fmSemitones += std::fabs(src1) * symbiosisStrength * 18.0f;
                    // === REEF RESIDENCY: Symbiote — coupling amplitude drives FM depth ===
                    // The resident's energy FEEDS cross-modulation: coupling RMS acts as
                    // an FM depth multiplier. When coupling is active, FM increases — the
                    // resident's energy nourishes spectral complexity between the sources.
                    // Uses existing fmDepth as base when available, but guarantees min ±6st
                    // of resident-driven FM even when fmDepth=0 so the symbiote is never silent.
                    if (reefResident == 2 && residentStrength > 0.001f && reefCouplingRms_ > 0.001f)
                    {
                        float baseFM = std::max(std::fabs(fmDepth), 0.25f); // min 0.25 = ±6st
                        fmSemitones += reefCouplingRms_ * residentStrength * baseFM * 36.0f;
                    }
                    src2 = renderSourceSample(src2Type, voice, 1, freq2, effPW2wt, fmSemitones, wtBankVal);
                }
                recordTap(TracerTap::Src2Output, src2);

                // === SPLIT PROCESSOR ROUTING (the Constructive Collision) ===

                // Proc1 processes Source 1 independently (with optional feedback)
                float sig1 = src1;
                if (proc1Type > 0 && proc1Type <= 3)
                {
                    setFilterMode(voice.procFilters[0], proc1Type);
                    float cut = clamp(proc1Cut + cutoffMod + velTimbre, 20.0f, 20000.0f);
                    float res = clamp(proc1Res + resoMod, 0.0f, 1.0f);
                    // Delta-threshold: skip expensive trig if cutoff/res haven't changed
                    // enough to be audible (kFilterDeltaHz = 1 Hz, kFilterDeltaRes = 0.001).
                    if (std::fabs(cut - voice.procLastCut[0]) > kFilterDeltaHz ||
                        std::fabs(res - voice.procLastRes[0]) > kFilterDeltaRes)
                    {
                        voice.procFilters[0].setCoefficients(cut, res, sr);
                        voice.procLastCut[0] = cut;
                        voice.procLastRes[0] = res;
                    }
                    // Filter feedback: route output back through tanh into input
                    float fbIn = sig1 + fastTanh(voice.procFbState[0] * proc1Fb * 4.0f);
                    float filtOut = voice.procFilters[0].processSample(fbIn);
                    voice.procFbState[0] = flushDenormal(filtOut);
                    sig1 = filtOut;
                }
                else if (proc1Type == 4) // Wavefolder — per-proc on sig1
                {
                    float fold = charFoldScale * velFoldBoost;
                    sig1 = fastTanh(fastSin(sig1 * fold * kPi));
                }
                else if (proc1Type == 5 && src2Type > 0) // Ring mod — sig1 × src2
                {
                    sig1 = sig1 * src2;
                }
                recordTap(TracerTap::Proc1Output, sig1);

                // Proc2 processes Source 2 independently (with optional feedback)
                float sig2 = src2;
                if (proc2Type > 0 && proc2Type <= 3 && src2Type > 0)
                {
                    setFilterMode(voice.procFilters[1], proc2Type);
                    float cut = clamp(proc2Cut + cutoffMod * 0.5f + velTimbre, 20.0f, 20000.0f);
                    float res = clamp(proc2Res + resoMod, 0.0f, 1.0f);
                    if (std::fabs(cut - voice.procLastCut[1]) > kFilterDeltaHz ||
                        std::fabs(res - voice.procLastRes[1]) > kFilterDeltaRes)
                    {
                        voice.procFilters[1].setCoefficients(cut, res, sr);
                        voice.procLastCut[1] = cut;
                        voice.procLastRes[1] = res;
                    }
                    float fbIn = sig2 + fastTanh(voice.procFbState[1] * proc2Fb * 4.0f);
                    float filtOut = voice.procFilters[1].processSample(fbIn);
                    voice.procFbState[1] = flushDenormal(filtOut);
                    sig2 = filtOut;
                }
                else if (proc2Type == 4 && src2Type > 0) // Wavefolder — per-proc on sig2
                {
                    float fold = charFoldScale * velFoldBoost;
                    sig2 = fastTanh(fastSin(sig2 * fold * kPi));
                }
                else if (proc2Type == 5 && src2Type > 0) // Ring mod — sig2 × src1
                {
                    sig2 = sig2 * src1;
                }
                recordTap(TracerTap::Proc2Output, sig2);

                // === BRICK ECOLOGY: Competition (Biophonic Phase 3) ===
                // Cross-suppression: the louder brick suppresses the quieter one.
                // Mimics resource competition between coral polyp populations.
                // 0.1 floor preserves each brick's identity even at max competition.
                if (competitionStrength > 0.001f && src2Type > 0)
                {
                    float env1 = std::fabs(sig1);
                    float env2 = std::fabs(sig2);
                    sig1 *= clamp(1.0f - env2 * competitionStrength, 0.1f, 1.0f);
                    sig2 *= clamp(1.0f - env1 * competitionStrength, 0.1f, 1.0f);
                }
                // === REEF RESIDENCY: Competitor — coupling RMS suppresses both sources ===
                // The coupled engine fights for amplitude space in the reef.
                // When the resident is loud, the bricks duck. Same 0.1 floor as ecology.
                // Uses competitionStrength as additional scale when > 0, otherwise
                // residentStrength alone drives the suppression (resident IS the competitor).
                if (reefResident == 1 && residentStrength > 0.001f && reefCouplingRms_ > 0.001f)
                {
                    float compScale = std::max(0.5f, competitionStrength + 0.5f);
                    float suppress = reefCouplingRms_ * residentStrength * compScale;
                    sig1 *= clamp(1.0f - suppress, 0.1f, 1.0f);
                    if (src2Type > 0)
                        sig2 *= clamp(1.0f - suppress, 0.1f, 1.0f);
                }

                // Mix the independently-processed sources
                float signal = (src2Type > 0) ? sig1 * srcMix + sig2 * (1.0f - srcMix) : sig1;
                recordTap(TracerTap::PostMix, signal);

                // Proc3: post-mix insert (wavefolder / ring mod / filter)
                if (proc3Type > 0 && proc3Type <= 3)
                {
                    setFilterMode(voice.procFilters[2], proc3Type);
                    float cut = clamp(proc3Cut + cutoffMod * 0.3f, 20.0f, 20000.0f);
                    float res = clamp(proc3Res + resoMod, 0.0f, 1.0f);
                    if (std::fabs(cut - voice.procLastCut[2]) > kFilterDeltaHz ||
                        std::fabs(res - voice.procLastRes[2]) > kFilterDeltaRes)
                    {
                        voice.procFilters[2].setCoefficients(cut, res, sr);
                        voice.procLastCut[2] = cut;
                        voice.procLastRes[2] = res;
                    }
                    float sig3 = signal + fastTanh(voice.procFbState[2] * proc3Fb * 4.0f);
                    signal = voice.procFilters[2].processSample(sig3);
                    voice.procFbState[2] = flushDenormal(signal);
                }
                else if (proc3Type == 4) // Wavefolder — post-mix on signal
                {
                    voice.procFbState[2] = 0.0f; // clear stale filter state
                    float fold = charFoldScale * velFoldBoost;
                    signal = fastTanh(fastSin(signal * fold * kPi));
                }
                else if (proc3Type == 5 && src2Type > 0) // Ring mod — post-mix × src2 carrier
                {
                    voice.procFbState[2] = 0.0f; // clear stale filter state
                    signal *= src2; // ring-mod the processed mix against src2 as carrier
                }
                else if (proc3Type > 3)
                {
                    voice.procFbState[2] = 0.0f; // future proc types: clear stale state
                }
                recordTap(TracerTap::Proc3Output, signal);

                // --- Amp envelope ---
                float ampLevel = voice.ampEnv.process();
                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                float gain = ampLevel * voice.velocity * (1.0f + volMod) * stealFade;
                gain = clamp(gain, 0.0f, 2.0f);

                signal *= gain;
                signal = flushDenormal(signal);

                float effPan = clamp(voice.pan + panMod, -1.0f, 1.0f);
                float panL = 0.5f - effPan * 0.5f;
                float panR = 0.5f + effPan * 0.5f;
                mixL += signal * panL;
                mixR += signal * panR;
            }

            // --- Effects chain: Serial (default) or Parallel (Biophonic Phase 5) ---
            // Serial: each FX processes the output of the previous (depth layering).
            // Parallel: each FX processes the dry signal independently; wet outputs summed.
            if (fxMode == 0) // Serial — existing behavior
            {
                for (int fx = 0; fx < 3; ++fx)
                {
                    if (fxType[fx] > 0)
                    {
                        float effMix = clamp(fxMix[fx] + macroSpace * (1.0f - fxMix[fx]) + fxMixMod, 0.0f, 1.0f);
                        if (effMix > 0.001f)
                            applyEffect(fxType[fx], mixL, mixR, effMix, fxParam[fx], macroSpace, fxSlots[fx]);
                    }
                }
            }
            else // Parallel — each slot processes dry independently; wet contributions summed
            {
                // Accumulate wet-above-dry from each active slot, then normalize by slot
                // count to prevent energy inflation when multiple slots are active.
                float dryL = mixL, dryR = mixR;
                float wetSumL = 0.0f, wetSumR = 0.0f;
                int activeSlots = 0;
                for (int fx = 0; fx < 3; ++fx)
                {
                    if (fxType[fx] > 0)
                    {
                        float slotL = dryL, slotR = dryR;
                        float effMix = clamp(fxMix[fx] + macroSpace * (1.0f - fxMix[fx]) + fxMixMod, 0.0f, 1.0f);
                        if (effMix > 0.001f)
                        {
                            applyEffect(fxType[fx], slotL, slotR, 1.0f, fxParam[fx], macroSpace, fxSlots[fx]);
                            wetSumL += (slotL - dryL) * effMix;
                            wetSumR += (slotR - dryR) * effMix;
                            ++activeSlots;
                        }
                    }
                }
                float normGain = activeSlots > 1 ? 1.0f / static_cast<float>(activeSlots) : 1.0f;
                mixL = dryL + wetSumL * normGain;
                mixR = dryR + wetSumR * normGain;
            }
            recordTap(TracerTap::PostFX, mixL);

            // === PER-BRICK SPATIAL (Wave 3) — Tomita's scene-making ===
            //
            // Two complementary spatial processors that position the sound in an
            // imaginary acoustic space. DISTANCE models air absorption (HF rolloff
            // at far distances); AIR models spectral tilt (warm/cold atmosphere).
            // Coefficients are pre-computed once per block above.

            // DISTANCE: 1-pole LP simulating HF air absorption
            if (distance > 0.001f)
            {
                distFiltL_ += distCoeff * (mixL - distFiltL_);
                distFiltR_ += distCoeff * (mixR - distFiltR_);
                distFiltL_ = flushDenormal(distFiltL_);
                distFiltR_ = flushDenormal(distFiltR_);
                mixL = distFiltL_;
                mixR = distFiltR_;
            }

            // AIR: LP/HP split at 1 kHz; gains tilt spectrum warm or cold
            if (std::fabs(air - 0.5f) > 0.02f)
            {
                airFiltL_ += airCoeff * (mixL - airFiltL_);
                airFiltR_ += airCoeff * (mixR - airFiltR_);
                airFiltL_ = flushDenormal(airFiltL_);
                airFiltR_ = flushDenormal(airFiltR_);
                float hpL = mixL - airFiltL_;
                float hpR = mixR - airFiltR_;
                mixL = airFiltL_ * airLpGain + hpL * airHpGain;
                mixR = airFiltR_ * airLpGain + hpR * airHpGain;
            }
            recordTap(TracerTap::PostSpatial, mixL);

            mixL *= level;
            mixR *= level;

            // Store for coupling output
            lastSampleL = mixL;
            lastSampleR = mixR;

            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, s, mixL);
                buffer.addSample(1, s, mixR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, s, (mixL + mixR) * 0.5f);
            }
        }

        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoices.store(count, std::memory_order_relaxed);

        // SRO SilenceGate: feed output to the gate for silence detection
        analyzeForSilenceGate(buffer, numSamples);

        // Brick complexity for coupling metadata (0–1 normalized)
        // Smith: architectural complexity signal — how many bricks are active
        static constexpr int kMaxBricks = 2 + 3 + 3; // 2 sources + 3 procs + 3 effects
        int numActiveBricks = (src1Type > 0 ? 1 : 0) + (src2Type > 0 ? 1 : 0) + (proc1Type > 0 ? 1 : 0) +
                              (proc2Type > 0 ? 1 : 0) + (proc3Type > 0 ? 1 : 0) + (fxType[0] > 0 ? 1 : 0) +
                              (fxType[1] > 0 ? 1 : 0) + (fxType[2] > 0 ? 1 : 0);
        float voiceGate = (activeVoices.load(std::memory_order_relaxed) > 0) ? 1.0f : 0.0f;
        brickComplexity = voiceGate * static_cast<float>(std::max(1, numActiveBricks)) / static_cast<float>(kMaxBricks);
    }

    //==========================================================================
    // Coupling — fixed: real audio + brick complexity
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0)
            return lastSampleL;
        if (channel == 1)
            return lastSampleR;
        // Channel 2: Brick Complexity — 0–1 signal encoding how many bricks are
        // active in this voice. Blessed by Dave Smith (Seance): "architectural
        // metadata as coupling." Other engines can modulate their own behavior
        // based on OBRIX's configuration: a full reef responds differently than
        // a sparse one. When the reef is crowded, the coupled engine knows.
        if (channel == 2)
            return brickComplexity;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
        case CouplingType::AudioToFM:
            couplingPitchMod.store(couplingPitchMod.load() + amount * 0.5f);
            break;
        case CouplingType::AmpToFilter:
            couplingCutoffMod.store(couplingCutoffMod.load() + amount);
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod.store(couplingPitchMod.load() + amount * 0.5f);
            break;
        case CouplingType::AmpToPitch:
            couplingPitchMod.store(couplingPitchMod.load() + amount * 0.3f);
            break;
        case CouplingType::AmpToChoke:
            for (auto& v : voices)
                if (v.active)
                    v.ampEnv.noteOff();
            break;
        case CouplingType::EnvToMorph:
            // Partner envelope drives wavetable morph position (±1 range).
            couplingWtPosMod.store(couplingWtPosMod.load() + amount);
            break;
        case CouplingType::PitchToPitch:
            // Harmony coupling: partner pitch shifts OBRIX pitch. Scaling of 0.5f matches
            // AudioToFM and LFOToPitch, giving ±50 cents at full amount through * 100.0f
            // in renderBlock. Use amount * 12.0f for ±1200 cents (±1 octave) if desired.
            couplingPitchMod.store(couplingPitchMod.load() + amount * 0.5f);
            break;
        case CouplingType::EnvToDecay:
            // Partner envelope scales OBRIX amp decay time (applied multiplicatively in renderBlock).
            couplingDecayMod.store(couplingDecayMod.load() + amount);
            break;
        case CouplingType::AudioToRing:
            // Partner audio energy ring-excites OBRIX: drives pitch and filter simultaneously,
            // creating a ring-mod-like timbral effect without per-sample buffer dependency.
            couplingPitchMod.store(couplingPitchMod.load() + amount * 0.3f);
            couplingCutoffMod.store(couplingCutoffMod.load() + amount * 0.5f);
            break;
        case CouplingType::RhythmToBlend:
            couplingCutoffMod.store(couplingCutoffMod.load() + amount * 0.3f);  // rhythm energy → filter movement
            break;
        case CouplingType::FilterToFilter:
            couplingCutoffMod.store(couplingCutoffMod.load() + amount * 0.8f);  // partner filter → our filter (strong)
            break;
        default:
            break;
        }

        // === REEF RESIDENCY: Store coupling buffer for ecological processing ===
        // The buffer is consumed in renderBlock() after applyCouplingInput() runs.
        // We copy into a pre-allocated scratch buffer (no allocation on audio thread).
        if (sourceBuffer != nullptr && numSamples > 0)
        {
            int len = std::min(numSamples, kMaxReefBufSize);
            std::memcpy(reefCouplingBuf_, sourceBuffer, static_cast<size_t>(len) * sizeof(float));
            reefCouplingBufLen_ = len;
        }
    }

    //==========================================================================
    // Parameters (82 total — Wave 5: Reef Residency)
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;
        using PC = juce::AudioParameterChoice;

        auto srcChoices =
            juce::StringArray{"Off", "Sine", "Saw", "Square", "Triangle", "Noise", "Wavetable", "Pulse", "Driftwood"};
        auto wtChoices = juce::StringArray{"Analog", "Vocal", "Metallic", "Organic"};
        auto procChoices = juce::StringArray{"Off", "LP Filter", "HP Filter", "BP Filter", "Wavefolder", "Ring Mod"};
        auto modChoices = juce::StringArray{"Off", "Envelope", "LFO", "Velocity", "Aftertouch"};
        auto tgtChoices = juce::StringArray{"None",   "Pitch",       "Filter Cutoff", "Filter Reso", "Volume",
                                            "WT Pos", "Pulse Width", "FX Mix",        "Pan"};
        auto fxChoices = juce::StringArray{"Off", "Delay", "Chorus", "Reverb"};
        auto gestChoices = juce::StringArray{"Ripple", "Bioluminescent Pulse", "Undertow", "Surge"};

        // Sources (7)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_src1Type", 1}, "Obrix Source 1 Type", srcChoices,
                                 2)); // 2 = Saw default (Pearlman/seance: "filter has nothing to sculpt with Sine")
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_src2Type", 1}, "Obrix Source 2 Type", srcChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_src1Tune", 1}, "Obrix Source 1 Tune",
                                              juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_src2Tune", 1}, "Obrix Source 2 Tune",
                                              juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_src1PW", 1}, "Obrix Source 1 Pulse Width",
                                              juce::NormalisableRange<float>(0.05f, 0.95f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_src2PW", 1}, "Obrix Source 2 Pulse Width",
                                              juce::NormalisableRange<float>(0.05f, 0.95f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_srcMix", 1}, "Obrix Source Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        // Processors (9: 3 slots × 3 params)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_proc1Type", 1}, "Obrix Processor 1 Type", procChoices, 1));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc1Cutoff", 1}, "Obrix Proc 1 Cutoff",
                                              juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc1Reso", 1}, "Obrix Proc 1 Resonance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_proc2Type", 1}, "Obrix Processor 2 Type", procChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc2Cutoff", 1}, "Obrix Proc 2 Cutoff",
                                              juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 4000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc2Reso", 1}, "Obrix Proc 2 Resonance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_proc3Type", 1}, "Obrix Processor 3 Type", procChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc3Cutoff", 1}, "Obrix Proc 3 Cutoff",
                                              juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 4000.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc3Reso", 1}, "Obrix Proc 3 Resonance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // Amp envelope (4)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_ampAttack", 1}, "Obrix Amp Attack",
                                              juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_ampDecay", 1}, "Obrix Amp Decay",
                                              juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.3f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_ampSustain", 1}, "Obrix Amp Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_ampRelease", 1}, "Obrix Amp Release",
                                              juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // 4 Modulators (16: 4 slots × 4 params)
        // Mod 1: Env→Cutoff (default)  Mod 2: LFO→Cutoff (default, depth=0)
        // Mod 3: Vel→Volume (Pearlman) Mod 4: AT→Cutoff (Pearlman, depth=0)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_mod1Type", 1}, "Obrix Mod 1 Type", modChoices, 1));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_mod1Target", 1}, "Obrix Mod 1 Target", tgtChoices, 2));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_mod1Depth", 1}, "Obrix Mod 1 Depth",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_mod1Rate", 1}, "Obrix Mod 1 Rate",
                                              juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_mod2Type", 1}, "Obrix Mod 2 Type", modChoices, 2));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_mod2Target", 1}, "Obrix Mod 2 Target", tgtChoices, 2));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_mod2Depth", 1}, "Obrix Mod 2 Depth",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_mod2Rate", 1}, "Obrix Mod 2 Rate",
                                              juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_mod3Type", 1}, "Obrix Mod 3 Type", modChoices, 3));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_mod3Target", 1}, "Obrix Mod 3 Target", tgtChoices, 4));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_mod3Depth", 1}, "Obrix Mod 3 Depth",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.5f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_mod3Rate", 1}, "Obrix Mod 3 Rate",
                                              juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_mod4Type", 1}, "Obrix Mod 4 Type", modChoices, 4));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_mod4Target", 1}, "Obrix Mod 4 Target", tgtChoices, 2));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_mod4Depth", 1}, "Obrix Mod 4 Depth",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_mod4Rate", 1}, "Obrix Mod 4 Rate",
                                              juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        // 3 Effects (9: 3 slots × 3 params)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_fx1Type", 1}, "Obrix Effect 1 Type", fxChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fx1Mix", 1}, "Obrix Effect 1 Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fx1Param", 1}, "Obrix Effect 1 Param",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_fx2Type", 1}, "Obrix Effect 2 Type", fxChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fx2Mix", 1}, "Obrix Effect 2 Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fx2Param", 1}, "Obrix Effect 2 Param",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_fx3Type", 1}, "Obrix Effect 3 Type", fxChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fx3Mix", 1}, "Obrix Effect 3 Mix",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fx3Param", 1}, "Obrix Effect 3 Param",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // Level (1)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_level", 1}, "Obrix Level",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        // Macros (4)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_macroCharacter", 1}, "Obrix Macro CHARACTER",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_macroMovement", 1}, "Obrix Macro MOVEMENT",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_macroCoupling", 1}, "Obrix Macro COUPLING",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_macroSpace", 1}, "Obrix Macro SPACE",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // Voice mode (1)
        params.push_back(std::make_unique<PC>(juce::ParameterID{"obrix_polyphony", 1}, "Obrix Voice Mode",
                                              juce::StringArray{"Mono", "Legato", "Poly4", "Poly8"}, 3));

        // Expression (2)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_pitchBendRange", 1}, "Obrix Pitch Bend Range",
                                              juce::NormalisableRange<float>(1.0f, 24.0f, 1.0f), 2.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_glideTime", 1}, "Obrix Glide Time",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.3f), 0.0f));

        // FLASH gesture (2)
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_gestureType", 1}, "Obrix Gesture Type", gestChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_flashTrigger", 1}, "Obrix FLASH Trigger",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

        // Wave 2: Source FM + Filter Feedback + Wavetable Banks + Unison (5)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fmDepth", 1}, "Obrix FM Depth",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc1Feedback", 1}, "Obrix Proc 1 Feedback",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc2Feedback", 1}, "Obrix Proc 2 Feedback",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_proc3Feedback", 1}, "Obrix Proc 3 Feedback",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(
            std::make_unique<PC>(juce::ParameterID{"obrix_wtBank", 1}, "Obrix Wavetable Bank", wtChoices, 0));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_unisonDetune", 1}, "Obrix Unison Detune",
                                              juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 0.0f));

        // Wave 3: Drift Bus + Journey + Spatial (5 params → 65 total)
        // Drift Bus: global ultra-slow LFO for Berlin School ensemble pitch wander
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_driftRate", 1}, "Obrix Drift Rate",
                                              juce::NormalisableRange<float>(0.001f, 0.05f, 0.001f, 0.3f),
                                              0.005f)); // 200s period default
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_driftDepth", 1}, "Obrix Drift Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        // Journey Mode: suppress note-off so sound sustains and evolves indefinitely
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_journeyMode", 1}, "Obrix Journey Mode",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f),
                                              0.0f)); // 0=off, 1=on (toggle)
        // Spatial: DISTANCE (HF rolloff) + AIR (spectral tilt warm/cold)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_distance", 1}, "Obrix Distance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                              0.0f)); // 0=close/bright, 1=far/dark
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_air", 1}, "Obrix Air",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                              0.5f)); // 0=warm/bass, 1=cold/treble

        // Wave 4: Biophonic Synthesis (14 params → 79 total)
        // Harmonic Field — JI attractor/repulsor
        auto fieldPrimeChoices =
            juce::StringArray{"3-Limit (Pythagorean)", "5-Limit (Extended JI)", "7-Limit (Septimal)"};
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fieldStrength", 1}, "Obrix Field Strength",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"obrix_fieldPolarity", 1},
                                                                    "Obrix Field Polarity",
                                                                    true)); // true=attractor, false=repulsor
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_fieldRate", 1}, "Obrix Field Rate",
                                              juce::NormalisableRange<float>(0.001f, 0.1f, 0.001f, 0.3f), 0.01f));
        params.push_back(std::make_unique<PC>(juce::ParameterID{"obrix_fieldPrimeLimit", 1}, "Obrix Field Prime Limit",
                                              fieldPrimeChoices, 1)); // default: 5-limit
        // Environmental Parameters — thermodynamic synthesis medium
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_envTemp", 1}, "Obrix Temperature",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_envPressure", 1}, "Obrix Pressure",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f)); // 0.5=neutral
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_envCurrent", 1}, "Obrix Current",
                                              juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_envTurbidity", 1}, "Obrix Turbidity",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        // Brick Ecology — competition and symbiosis
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_competitionStrength", 1}, "Obrix Competition",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_symbiosisStrength", 1}, "Obrix Symbiosis",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        // Stateful Synthesis — stress, bleaching, reset
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_stressDecay", 1}, "Obrix Stress Decay",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_bleachRate", 1}, "Obrix Bleach Rate",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_stateReset", 1}, "Obrix State Reset",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f),
                                              0.0f)); // momentary trigger
        // FX Mode — serial vs parallel routing
        auto fxModeChoices = juce::StringArray{"Serial", "Parallel"};
        params.push_back(std::make_unique<PC>(juce::ParameterID{"obrix_fxMode", 1}, "Obrix FX Mode", fxModeChoices,
                                              0)); // default: Serial (existing behavior)

        // Wave 5: Reef Residency (82 total)
        // Coupling input becomes a third ecological organism in the Brick Ecology system.
        auto residentChoices = juce::StringArray{"Off", "Competitor", "Symbiote", "Parasite"};
        params.push_back(std::make_unique<PC>(juce::ParameterID{"obrix_reefResident", 1}, "Obrix Reef Resident",
                                              residentChoices, 0)); // default: Off (backward compatible)
        params.push_back(std::make_unique<PF>(juce::ParameterID{"obrix_residentStrength", 1}, "Obrix Resident Strength",
                                              juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
                                              0.3f)); // Guru Bin: 0.3 sweet spot
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pSrc1Type = apvts.getRawParameterValue("obrix_src1Type");
        pSrc2Type = apvts.getRawParameterValue("obrix_src2Type");
        pSrc1Tune = apvts.getRawParameterValue("obrix_src1Tune");
        pSrc2Tune = apvts.getRawParameterValue("obrix_src2Tune");
        pSrc1PW = apvts.getRawParameterValue("obrix_src1PW");
        pSrc2PW = apvts.getRawParameterValue("obrix_src2PW");
        pSrcMix = apvts.getRawParameterValue("obrix_srcMix");
        pProc1Type = apvts.getRawParameterValue("obrix_proc1Type");
        pProc1Cutoff = apvts.getRawParameterValue("obrix_proc1Cutoff");
        pProc1Reso = apvts.getRawParameterValue("obrix_proc1Reso");
        pProc2Type = apvts.getRawParameterValue("obrix_proc2Type");
        pProc2Cutoff = apvts.getRawParameterValue("obrix_proc2Cutoff");
        pProc2Reso = apvts.getRawParameterValue("obrix_proc2Reso");
        pProc3Type = apvts.getRawParameterValue("obrix_proc3Type");
        pProc3Cutoff = apvts.getRawParameterValue("obrix_proc3Cutoff");
        pProc3Reso = apvts.getRawParameterValue("obrix_proc3Reso");
        pAmpAttack = apvts.getRawParameterValue("obrix_ampAttack");
        pAmpDecay = apvts.getRawParameterValue("obrix_ampDecay");
        pAmpSustain = apvts.getRawParameterValue("obrix_ampSustain");
        pAmpRelease = apvts.getRawParameterValue("obrix_ampRelease");

        const char* modIds[4][4] = {{"obrix_mod1Type", "obrix_mod1Target", "obrix_mod1Depth", "obrix_mod1Rate"},
                                    {"obrix_mod2Type", "obrix_mod2Target", "obrix_mod2Depth", "obrix_mod2Rate"},
                                    {"obrix_mod3Type", "obrix_mod3Target", "obrix_mod3Depth", "obrix_mod3Rate"},
                                    {"obrix_mod4Type", "obrix_mod4Target", "obrix_mod4Depth", "obrix_mod4Rate"}};
        for (int i = 0; i < 4; ++i)
        {
            pModType[i] = apvts.getRawParameterValue(modIds[i][0]);
            pModTarget[i] = apvts.getRawParameterValue(modIds[i][1]);
            pModDepth[i] = apvts.getRawParameterValue(modIds[i][2]);
            pModRate[i] = apvts.getRawParameterValue(modIds[i][3]);
        }

        const char* fxIds[3][3] = {{"obrix_fx1Type", "obrix_fx1Mix", "obrix_fx1Param"},
                                   {"obrix_fx2Type", "obrix_fx2Mix", "obrix_fx2Param"},
                                   {"obrix_fx3Type", "obrix_fx3Mix", "obrix_fx3Param"}};
        for (int i = 0; i < 3; ++i)
        {
            pFxType[i] = apvts.getRawParameterValue(fxIds[i][0]);
            pFxMix[i] = apvts.getRawParameterValue(fxIds[i][1]);
            pFxParam[i] = apvts.getRawParameterValue(fxIds[i][2]);
        }

        pLevel = apvts.getRawParameterValue("obrix_level");
        pMacroChar = apvts.getRawParameterValue("obrix_macroCharacter");
        pMacroMove = apvts.getRawParameterValue("obrix_macroMovement");
        pMacroCoup = apvts.getRawParameterValue("obrix_macroCoupling");
        pMacroSpace = apvts.getRawParameterValue("obrix_macroSpace");
        pVoiceMode = apvts.getRawParameterValue("obrix_polyphony");
        pPitchBendRange = apvts.getRawParameterValue("obrix_pitchBendRange");
        pGlideTime = apvts.getRawParameterValue("obrix_glideTime");
        pGestureType = apvts.getRawParameterValue("obrix_gestureType");
        pFlashTrigger = apvts.getRawParameterValue("obrix_flashTrigger");

        // Wave 2
        pFmDepth = apvts.getRawParameterValue("obrix_fmDepth");
        pProc1Fb = apvts.getRawParameterValue("obrix_proc1Feedback");
        pProc2Fb = apvts.getRawParameterValue("obrix_proc2Feedback");
        pProc3Fb = apvts.getRawParameterValue("obrix_proc3Feedback");
        pWtBank = apvts.getRawParameterValue("obrix_wtBank");
        pUnisonDetune = apvts.getRawParameterValue("obrix_unisonDetune");

        // Wave 3
        pDriftRate = apvts.getRawParameterValue("obrix_driftRate");
        pDriftDepth = apvts.getRawParameterValue("obrix_driftDepth");
        pJourneyMode = apvts.getRawParameterValue("obrix_journeyMode");
        pDistance = apvts.getRawParameterValue("obrix_distance");
        pAir = apvts.getRawParameterValue("obrix_air");

        // Wave 4 — Biophonic Synthesis
        pFieldStrength = apvts.getRawParameterValue("obrix_fieldStrength");
        pFieldPolarity = apvts.getRawParameterValue("obrix_fieldPolarity");
        pFieldRate = apvts.getRawParameterValue("obrix_fieldRate");
        pFieldPrimeLimit = apvts.getRawParameterValue("obrix_fieldPrimeLimit");
        pEnvTemp = apvts.getRawParameterValue("obrix_envTemp");
        pEnvPressure = apvts.getRawParameterValue("obrix_envPressure");
        pEnvCurrent = apvts.getRawParameterValue("obrix_envCurrent");
        pEnvTurbidity = apvts.getRawParameterValue("obrix_envTurbidity");
        pCompetitionStrength = apvts.getRawParameterValue("obrix_competitionStrength");
        pSymbiosisStrength = apvts.getRawParameterValue("obrix_symbiosisStrength");
        pStressDecay = apvts.getRawParameterValue("obrix_stressDecay");
        pBleachRate = apvts.getRawParameterValue("obrix_bleachRate");
        pStateReset = apvts.getRawParameterValue("obrix_stateReset");
        pFxMode = apvts.getRawParameterValue("obrix_fxMode");

        // Wave 5 — Reef Residency
        pReefResident = apvts.getRawParameterValue("obrix_reefResident");
        pResidentStrength = apvts.getRawParameterValue("obrix_residentStrength");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Obrix"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFF1E8B7E); }
    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return activeVoices.load(std::memory_order_relaxed); }

private:
    static constexpr float kTwoPi = 6.28318530717958647692f;
    static constexpr float kPi = 3.14159265358979323846f;

    static float loadP(std::atomic<float>* p, float fb) noexcept { return p ? p->load() : fb; }

    //==========================================================================
    // Source rendering — PolyBLEP for anti-aliased types, manual for others
    //==========================================================================
    float renderSourceSample(int type, ObrixVoice& voice, int idx, float freq, float pw, float fmSemitones = 0.0f,
                             int wtBank = 0) noexcept
    {
        // FM: apply frequency deviation (±24 st from Src1 output)
        float effFreq = (fmSemitones != 0.0f) ? freq * fastPow2(fmSemitones / 12.0f) : freq;
        // Clamp to Nyquist for non-bandlimited source types (Sine, WT, LoFi)
        if (type != 2 && type != 3 && type != 4)
        {
            effFreq = std::max(0.0f, effFreq);  // prevent negative phase increment
            effFreq = std::min(effFreq, sr * 0.49f);
        }
        float dt = effFreq / sr;

        switch (type)
        {
        case 1: // Sine — manual phase
        {
            float out = fastSin(voice.srcPhase[idx] * kTwoPi);
            voice.srcPhase[idx] += dt;
            if (voice.srcPhase[idx] >= 1.0f)
                voice.srcPhase[idx] -= 1.0f;
            return out;
        }
        case 2: // Saw — PolyBLEP anti-aliased
        {
            voice.srcOsc[idx].setFrequency(effFreq, sr);
            voice.srcOsc[idx].setWaveform(PolyBLEP::Waveform::Saw);
            return voice.srcOsc[idx].processSample();
        }
        case 3: // Square — PolyBLEP
        {
            voice.srcOsc[idx].setFrequency(effFreq, sr);
            voice.srcOsc[idx].setWaveform(PolyBLEP::Waveform::Square);
            return voice.srcOsc[idx].processSample();
        }
        case 4: // Triangle — PolyBLEP (integrated square with BLAMP)
        {
            voice.srcOsc[idx].setFrequency(effFreq, sr);
            voice.srcOsc[idx].setWaveform(PolyBLEP::Waveform::Triangle);
            return voice.srcOsc[idx].processSample();
        }
        case 5: // Noise
        {
            voice.noiseRng = voice.noiseRng * 1664525u + 1013904223u;
            return static_cast<float>(voice.noiseRng & 0xFFFF) / 32768.0f - 1.0f;
        }
        case 6: // Real wavetable banks (Wave 2) — pw = morph position within bank
        {
            int bank = std::max(0, std::min(3, wtBank));
            float tablePos = voice.srcPhase[idx] * kWTSize;
            auto ti = static_cast<int>(tablePos);
            float frac = tablePos - static_cast<float>(ti);
            ti = ti % kWTSize;
            float s0 = wavetables[bank][ti];
            float s1 = wavetables[bank][(ti + 1) % kWTSize];
            float tableOut = s0 + frac * (s1 - s0);
            // pw morphs between sine (pw=0) and full table character (pw=1)
            float morph = (pw - 0.05f) / 0.9f;
            float sineOut = fastSin(voice.srcPhase[idx] * kTwoPi);
            voice.srcPhase[idx] += dt;
            if (voice.srcPhase[idx] >= 1.0f)
                voice.srcPhase[idx] -= 1.0f;
            return sineOut * (1.0f - morph) + tableOut * morph;
        }
        case 7: // Pulse — PolyBLEP with variable width
        {
            voice.srcOsc[idx].setFrequency(effFreq, sr);
            voice.srcOsc[idx].setWaveform(PolyBLEP::Waveform::Pulse);
            voice.srcOsc[idx].setPulseWidth(pw);
            return voice.srcOsc[idx].processSample();
        }
        case 8: // Lo-Fi Saw — intentionally naive (Guild Lo-Fi specialist request)
        {
            float out = 2.0f * voice.srcPhase[idx] - 1.0f;
            voice.srcPhase[idx] += dt;
            if (voice.srcPhase[idx] >= 1.0f)
                voice.srcPhase[idx] -= 1.0f;
            return out;
        }
        default:
            return 0.0f;
        }
    }

    //==========================================================================
    // findNearestJIRatio — returns the ratio in the given prime limit closest to 'r'
    // Input r should be folded into [1.0, 2.0). Tables sorted ascending within octave.
    // D001 compliance: all ratios are integer fractions, mathematically exact.
    //==========================================================================
    static float findNearestJIRatio(float r, int primeLimit) noexcept
    {
        // 3-limit (Pythagorean): only perfect 5ths and octave-equivalents
        static constexpr float kJI3[] = {1.0f,        9.0f / 8.0f,   81.0f / 64.0f,   4.0f / 3.0f,
                                         3.0f / 2.0f, 27.0f / 16.0f, 243.0f / 128.0f, 2.0f / 1.0f};
        // 5-limit (extended JI): adds pure major/minor thirds and sixths
        static constexpr float kJI5[] = {1.0f,        16.0f / 15.0f, 9.0f / 8.0f, 6.0f / 5.0f, 5.0f / 4.0f,
                                         4.0f / 3.0f, 45.0f / 32.0f, 3.0f / 2.0f, 8.0f / 5.0f, 5.0f / 3.0f,
                                         9.0f / 5.0f, 15.0f / 8.0f,  2.0f / 1.0f};
        // 7-limit (harmonic): adds subminor/supermajor intervals and harmonic 7th
        static constexpr float kJI7[] = {1.0f,         16.0f / 15.0f, 9.0f / 8.0f, 7.0f / 6.0f, 6.0f / 5.0f,
                                         5.0f / 4.0f,  9.0f / 7.0f,   4.0f / 3.0f, 7.0f / 5.0f, 3.0f / 2.0f,
                                         14.0f / 9.0f, 8.0f / 5.0f,   5.0f / 3.0f, 7.0f / 4.0f, 9.0f / 5.0f,
                                         15.0f / 8.0f, 2.0f / 1.0f};

        const float* table;
        int size;
        switch (primeLimit)
        {
        case 0:
            table = kJI3;
            size = 8;
            break;
        case 2:
            table = kJI7;
            size = 17;
            break;
        default:
            table = kJI5;
            size = 13;
            break;
        }

        float nearest = table[0];
        float minDist = std::fabs(r - nearest);
        for (int i = 1; i < size; ++i)
        {
            float d = std::fabs(r - table[i]);
            if (d < minDist)
            {
                minDist = d;
                nearest = table[i];
            }
        }
        return nearest;
    }

    //==========================================================================
    // findNearestJICents — CPU-optimised replacement for the hot path in the
    // Harmonic Field block.  Replaces the per-sample std::log2 call with:
    //   1. A precomputed parallel table of 1200*log2(jiRatio[i]) (compile-time)
    //   2. fastLog2(r) for the dynamic input ratio (single cheap approximation)
    // Sonic behaviour is identical: same nearest-ratio search, same cent formula.
    //==========================================================================
    static float findNearestJICents(float r, int primeLimit) noexcept
    {
        // Ratio tables — identical to findNearestJIRatio above.
        static constexpr float kJI3[] = {1.0f,        9.0f / 8.0f,   81.0f / 64.0f,   4.0f / 3.0f,
                                         3.0f / 2.0f, 27.0f / 16.0f, 243.0f / 128.0f, 2.0f / 1.0f};
        static constexpr float kJI5[] = {1.0f,        16.0f / 15.0f, 9.0f / 8.0f, 6.0f / 5.0f, 5.0f / 4.0f,
                                         4.0f / 3.0f, 45.0f / 32.0f, 3.0f / 2.0f, 8.0f / 5.0f, 5.0f / 3.0f,
                                         9.0f / 5.0f, 15.0f / 8.0f,  2.0f / 1.0f};
        static constexpr float kJI7[] = {1.0f,         16.0f / 15.0f, 9.0f / 8.0f, 7.0f / 6.0f, 6.0f / 5.0f,
                                         5.0f / 4.0f,  9.0f / 7.0f,   4.0f / 3.0f, 7.0f / 5.0f, 3.0f / 2.0f,
                                         14.0f / 9.0f, 8.0f / 5.0f,   5.0f / 3.0f, 7.0f / 4.0f, 9.0f / 5.0f,
                                         15.0f / 8.0f, 2.0f / 1.0f};

        // Precomputed cent offsets: centOffsets[i] = 1200 * log2(jiRatios[i])
        // All values are compile-time constants — zero runtime cost.
        static constexpr float kCents3[] = {
            0.0f,      // 1/1     =   0.000 c
            203.910f,  // 9/8     = 203.910 c
            407.820f,  // 81/64   = 407.820 c
            498.045f,  // 4/3     = 498.045 c
            701.955f,  // 3/2     = 701.955 c
            905.865f,  // 27/16   = 905.865 c
            1109.775f, // 243/128 =1109.775 c
            1200.0f    // 2/1     =1200.000 c
        };
        static constexpr float kCents5[] = {
            0.0f,      // 1/1     =   0.000 c
            111.731f,  // 16/15   = 111.731 c
            203.910f,  // 9/8     = 203.910 c
            315.641f,  // 6/5     = 315.641 c
            386.314f,  // 5/4     = 386.314 c
            498.045f,  // 4/3     = 498.045 c
            590.224f,  // 45/32   = 590.224 c
            701.955f,  // 3/2     = 701.955 c
            813.686f,  // 8/5     = 813.686 c
            884.359f,  // 5/3     = 884.359 c
            1017.596f, // 9/5     =1017.596 c
            1088.269f, // 15/8   =1088.269 c
            1200.0f    // 2/1     =1200.000 c
        };
        static constexpr float kCents7[] = {
            0.0f,      // 1/1     =   0.000 c
            111.731f,  // 16/15   = 111.731 c
            203.910f,  // 9/8     = 203.910 c
            266.871f,  // 7/6     = 266.871 c
            315.641f,  // 6/5     = 315.641 c
            386.314f,  // 5/4     = 386.314 c
            435.084f,  // 9/7     = 435.084 c
            498.045f,  // 4/3     = 498.045 c
            582.512f,  // 7/5     = 582.512 c
            701.955f,  // 3/2     = 701.955 c
            764.916f,  // 14/9    = 764.916 c
            813.686f,  // 8/5     = 813.686 c
            884.359f,  // 5/3     = 884.359 c
            968.826f,  // 7/4     = 968.826 c
            1017.596f, // 9/5     =1017.596 c
            1088.269f, // 15/8   =1088.269 c
            1200.0f    // 2/1     =1200.000 c
        };

        const float* table;
        const float* cents;
        int size;
        switch (primeLimit)
        {
        case 0:
            table = kJI3;
            cents = kCents3;
            size = 8;
            break;
        case 2:
            table = kJI7;
            cents = kCents7;
            size = 17;
            break;
        default:
            table = kJI5;
            cents = kCents5;
            size = 13;
            break;
        }

        // Find nearest ratio index (same logic as findNearestJIRatio).
        int nearest = 0;
        float minDist = std::fabs(r - table[0]);
        for (int i = 1; i < size; ++i)
        {
            float d = std::fabs(r - table[i]);
            if (d < minDist)
            {
                minDist = d;
                nearest = i;
            }
        }

        // Compute cents offset: cents[nearest] - 1200*fastLog2(r).
        // No std::log2 — pure integer-bit approximation.
        return cents[nearest] - 1200.0f * fastLog2(r);
    }

    //==========================================================================
    void setFilterMode(CytomicSVF& filter, int procType) noexcept
    {
        switch (procType)
        {
        case 1:
            filter.setMode(CytomicSVF::Mode::LowPass);
            break;
        case 2:
            filter.setMode(CytomicSVF::Mode::HighPass);
            break;
        case 3:
            filter.setMode(CytomicSVF::Mode::BandPass);
            break;
        default:
            filter.setMode(CytomicSVF::Mode::LowPass);
            break;
        }
    }

    //==========================================================================
    void applyEffect(int type, float& L, float& R, float mix, float param, float space, ObrixFXState& fx) noexcept
    {
        float dryL = L, dryR = R;
        // Reverb damping tracks param (Guru Bin fix)
        float reverbDamp = 0.1f + param * 0.4f;

        switch (type)
        {
        case 1: // Delay
        {
            if (fx.delayBufL.empty())
                break;
            int bufSz = static_cast<int>(fx.delayBufL.size());
            int delaySamples = static_cast<int>((0.05f + param * 0.45f + space * 0.2f) * sr);
            delaySamples = std::max(1, std::min(delaySamples, bufSz - 1));
            int readPos = (fx.delayWritePos - delaySamples + bufSz) % bufSz;
            float wetL = fx.delayBufL[static_cast<size_t>(readPos)];
            float wetR = fx.delayBufR[static_cast<size_t>(readPos)];
            fx.delayBufL[static_cast<size_t>(fx.delayWritePos)] = flushDenormal(L + wetL * (param * 0.7f));
            fx.delayBufR[static_cast<size_t>(fx.delayWritePos)] = flushDenormal(R + wetR * (param * 0.7f));
            fx.delayWritePos = (fx.delayWritePos + 1) % bufSz;
            L = dryL * (1.0f - mix) + wetL * mix;
            R = dryR * (1.0f - mix) + wetR * mix;
            break;
        }
        case 2: // Chorus
        {
            if (fx.chorusBufL.empty())
                break;
            int bufSz = static_cast<int>(fx.chorusBufL.size());
            float depth = 0.002f * sr * (0.3f + param * 0.7f);
            float centerDelay = 0.007f * sr;
            fx.chorusLFOPhase += (0.3f + space * 2.0f) / sr;
            if (fx.chorusLFOPhase >= 1.0f)
                fx.chorusLFOPhase -= 1.0f;
            float lfoVal = fastSin(fx.chorusLFOPhase * kTwoPi);
            float delayL = centerDelay + lfoVal * depth;
            float delayR = centerDelay - lfoVal * depth;
            fx.chorusBufL[static_cast<size_t>(fx.chorusWritePos)] = L;
            fx.chorusBufR[static_cast<size_t>(fx.chorusWritePos)] = R;
            auto readInterp = [&](const std::vector<float>& buf, float del)
            {
                float rp = static_cast<float>(fx.chorusWritePos) - del;
                if (rp < 0.0f)
                    rp += static_cast<float>(bufSz);
                int i0 = static_cast<int>(rp) % bufSz;
                float frac = rp - std::floor(rp);
                return buf[static_cast<size_t>(i0)] * (1.0f - frac) + buf[static_cast<size_t>((i0 + 1) % bufSz)] * frac;
            };
            float wL = readInterp(fx.chorusBufL, delayL);
            float wR = readInterp(fx.chorusBufR, delayR);
            fx.chorusWritePos = (fx.chorusWritePos + 1) % bufSz;
            L = dryL * (1.0f - mix) + wL * mix;
            R = dryR * (1.0f - mix) + wR * mix;
            break;
        }
        case 3: // Reverb — 8-line FDN with Householder mixing + allpass input diffusion (OBRIX-999)
        {
            float input = (L + R) * 0.5f * (0.5f + space * 0.5f);

            // 2-stage allpass input diffusion: smears transients before entering the FDN
            // Each stage: y = -g*x + buf[rp] + g*y_prev  (Schroeder allpass)
            static constexpr float kApGain = 0.7f;
            for (int i = 0; i < 2; ++i)
            {
                int len = static_cast<int>(fx.apBuf[i].size());
                if (len == 0) continue;
                int rp = (fx.apPos[i] + 1) % len; // oldest sample
                float bufOut = fx.apBuf[i][static_cast<size_t>(rp)];
                float out = -kApGain * input + bufOut + kApGain * fx.apState[i];
                fx.apBuf[i][static_cast<size_t>(fx.apPos[i])] = flushDenormal(input + kApGain * bufOut);
                fx.apPos[i] = (fx.apPos[i] + 1) % len;
                fx.apState[i] = flushDenormal(out);
                input = out;
            }

            // Read 8 FDN delay lines and apply per-line damping LP
            float tap[8];
            float fb = 0.3f + param * 0.55f; // feedback gain (reverb time)
            for (int i = 0; i < 8; ++i)
            {
                int len = static_cast<int>(fx.reverbBuf[i].size());
                if (len == 0) { tap[i] = 0.0f; continue; }
                tap[i] = fx.reverbBuf[i][static_cast<size_t>(fx.reverbPos[i])];
                // Per-line damping LP (matches RT60 rolloff at high frequencies)
                fx.reverbFilt[i] = flushDenormal(fx.reverbFilt[i] + (tap[i] - fx.reverbFilt[i]) * reverbDamp);
                tap[i] = fx.reverbFilt[i];
            }

            // Householder mixing matrix: H = I - (2/N)*ones
            // Each output line = tap[i] - (2/8)*sum = tap[i] - 0.25*sum
            float tapSum = 0.0f;
            for (int i = 0; i < 8; ++i) tapSum += tap[i];
            float householderK = 0.25f; // 2/N = 2/8

            // Write back FDN with Householder-mixed feedback + input injection
            for (int i = 0; i < 8; ++i)
            {
                int len = static_cast<int>(fx.reverbBuf[i].size());
                if (len == 0) continue;
                float mixed = tap[i] - householderK * tapSum; // Householder row
                float fbSample = mixed * fb + input;
                fx.reverbBuf[i][static_cast<size_t>(fx.reverbPos[i])] = flushDenormal(fbSample);
                fx.reverbPos[i] = (fx.reverbPos[i] + 1) % len;
            }

            // Stereo decorrelation: alternate lines L/R with cross-bleed
            float rvL = tap[0] + tap[2] + tap[4] + tap[6] - 0.3f * (tap[1] + tap[3]);
            float rvR = tap[1] + tap[3] + tap[5] + tap[7] - 0.3f * (tap[0] + tap[2]);
            // Normalise: sum of 4 full lines + 2 partial cross-bleed terms
            rvL *= 0.22f;
            rvR *= 0.22f;
            L = dryL * (1.0f - mix) + rvL * mix;
            R = dryR * (1.0f - mix) + rvR * mix;
            break;
        }
        }
    }

    //==========================================================================
    // initVoice — full voice initialisation extracted from the noteOn non-legato path.
    // Safe to call from the audio thread: no allocations, no I/O.
    void initVoice(ObrixVoice& v, int slot, int noteNum, float vel,
                   float freq1, float freq2, float panOffset,
                   float ampA, float ampD, float ampS, float ampR,
                   const int modTypes[4], const float modRates[4]) noexcept
    {
        v.reset();
        v.active     = true;
        v.note       = noteNum;
        v.velocity   = vel;
        v.startTime  = ++voiceCounter;
        v.noiseRng   = static_cast<uint32_t>(slot * 777 + noteNum * 31 + voiceCounter);
        v.srcFreq[0]    = freq1;
        v.srcFreq[1]    = freq2;
        v.targetFreq[0] = freq1;
        v.targetFreq[1] = freq2;
        v.pan        = panOffset;
        v.ampEnv.setParams(ampA, ampD, ampS, ampR, sr);
        v.ampEnv.noteOn();
        for (int m = 0; m < 4; ++m)
        {
            if (modTypes[m] == 1)
            {
                float rate = modRates[m];
                v.modEnvs[m].setParams(0.01f, rate * 0.3f + 0.01f, 0.0f, rate * 0.3f + 0.01f, sr);
                v.modEnvs[m].noteOn();
            }
            if (modTypes[m] == 2)
            {
                v.modLFOs[m].setRate(modRates[m], sr);
                v.modLFOs[m].shape = 0;
            }
        }
    }

    void noteOn(int noteNum, float vel, float ampA, float ampD, float ampS, float ampR, float tune1, float tune2,
                const int modTypes[4], const float modRates[4], int voiceMode, float glideCoeff,
                float detuneOffsetST = 0.0f, float panOffset = 0.0f, bool isUnisonExtra = false)
    {
        bool isLegato = (voiceMode == 1) && !isUnisonExtra;
        // Unison extras search the full voice pool; primary voices respect polyLimit
        int maxVoicesNow = isUnisonExtra ? kMaxVoices : std::min(kMaxVoices, polyLimit_);

        // Find free voice or steal oldest
        int slot = -1;
        uint64_t oldest = UINT64_MAX;
        int oldestSlot = 0;

        // Legato: retrigger the same active voice
        if (isLegato)
        {
            for (int i = 0; i < maxVoicesNow; ++i)
                if (voices[i].active)
                {
                    slot = i;
                    break;
                }
        }

        if (slot < 0)
        {
            for (int i = 0; i < maxVoicesNow; ++i)
            {
                if (!voices[i].active)
                {
                    slot = i;
                    break;
                }
                if (voices[i].startTime < oldest)
                {
                    oldest = voices[i].startTime;
                    oldestSlot = i;
                }
            }

            if (slot < 0)
            {
                // All voices active — steal the oldest one.
                // If it isn't already mid-fade, start a 5ms linear ramp-down
                // and defer the incoming note to the render loop.
                auto& sv = voices[oldestSlot];
                if (sv.active && sv.stealFadeSamples == 0)
                {
                    sv.stealFadeTotal   = std::max(1, static_cast<int>(sr * 0.005f));
                    sv.stealFadeSamples = sv.stealFadeTotal;
                    sv.pendingNote      = noteNum;
                    sv.pendingVel       = vel;
                    sv.pendingFreq1     = 440.0f * fastPow2((static_cast<float>(noteNum) - 69.0f + tune1) / 12.0f + detuneOffsetST);
                    sv.pendingFreq2     = 440.0f * fastPow2((static_cast<float>(noteNum) - 69.0f + tune2) / 12.0f + detuneOffsetST);
                    sv.pendingPan       = panOffset;
                    sv.pendingAmpA      = ampA;
                    sv.pendingAmpD      = ampD;
                    sv.pendingAmpS      = ampS;
                    sv.pendingAmpR      = ampR;
                    for (int m = 0; m < 4; ++m)
                    {
                        sv.pendingModTypes[m] = modTypes[m];
                        sv.pendingModRates[m] = modRates[m];
                    }
                    return; // Fade will complete in renderBlock — new note deferred
                }
                slot = oldestSlot; // Voice already fading — take it now (hard cut)
            }
        }

        auto& v = voices[slot];

        float newFreq1 = 440.0f * fastPow2((static_cast<float>(noteNum) - 69.0f + tune1) / 12.0f + detuneOffsetST);
        float newFreq2 = 440.0f * fastPow2((static_cast<float>(noteNum) - 69.0f + tune2) / 12.0f + detuneOffsetST);

        if (isLegato && v.active)
        {
            // Portamento: set target, don't reset voice
            v.targetFreq[0] = newFreq1;
            v.targetFreq[1] = newFreq2;
            v.note = noteNum;
            v.velocity = vel;
        }
        else
        {
            initVoice(v, slot, noteNum, vel, newFreq1, newFreq2, panOffset,
                      ampA, ampD, ampS, ampR, modTypes, modRates);
        }
    }

    void noteOff(int noteNum)
    {
        for (auto& v : voices)
            if (v.active && v.note == noteNum)
            {
                v.ampEnv.noteOff();
                for (auto& e : v.modEnvs)
                    e.noteOff();
            }
    }

    //==========================================================================
    // State
    //==========================================================================
    float sr = 0.0f; // sentinel: must be set by prepare() before use (#671)
    uint64_t voiceCounter = 0;
    std::array<ObrixVoice, kMaxVoices> voices{};
    std::atomic<int> activeVoices{0};
    float modWheel_ = 0.0f;
    float pitchBend_ = 0.0f;
    int polyLimit_ = 8;
    std::atomic<float> couplingPitchMod{0.0f};
    std::atomic<float> couplingCutoffMod{0.0f};
    std::atomic<float> couplingWtPosMod{0.0f};  // EnvToMorph: partner envelope → wavetable position
    std::atomic<float> couplingDecayMod{0.0f};  // EnvToDecay: partner envelope → amp decay time scale
    // AudioToRing routes directly into couplingPitchMod and couplingCutoffMod — no separate member needed.

    // Coupling output (fixed: real audio)
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;
    float brickComplexity = 0.0f;

    // FLASH gesture state
    float gestureLevel = 0.0f;
    float gesturePhase = 0.0f;
    float prevFlashTrig = 0.0f;

    // Wave 3 — Drift Bus + Journey + Spatial
    float driftPhase_ = 0.0f;  // Drift Bus global LFO phase (0-1, wraps at 1)
    bool journeyMode_ = false; // Journey: suppress note-off (cached from param each block)
    float distFiltL_ = 0.0f;   // DISTANCE 1-pole LP filter state (L)
    float distFiltR_ = 0.0f;   // DISTANCE 1-pole LP filter state (R)
    float airFiltL_ = 0.0f;    // AIR LP/HP split filter state (L)
    float airFiltR_ = 0.0f;    // AIR LP/HP split filter state (R)

    // Wave 4 — Biophonic Synthesis
    float stressLevel_ = 0.0f;   // velocity leaky integrator τ=30–60s (Stress Memory)
    float bleachLevel_ = 0.0f;   // cumulative brightness attenuation (Bleaching)
    float prevResetTrig_ = 0.0f; // edge detector for obrix_stateReset
    uint32_t turbRng_ = 31415u;  // per-engine noise RNG for turbidity fluctuation

    // #620 block-rate coefficient cache — avoids std::exp on every audio block
    float cachedStressDecay_      = -1.0f; // sentinel — forces compute on first block / after prepare()
    float cachedStressDecayCoeff_ = 0.9999f; // exp(-1/(30*sr)) @ 48 kHz ≈ 0.999999
    float cachedParasiteAlpha_    = 1.0f - 0.9999f;
    float cachedParasiteDecay0_   = 0.9999f; // fixed τ=30s path

    // Wave 5 — Reef Residency (pre-allocated, no audio-thread allocation)
    static constexpr int kMaxReefBufSize = 4096; // max block size we'll ever see
    float reefCouplingBuf_[kMaxReefBufSize]{};   // coupling buffer copy from applyCouplingInput
    std::atomic<int> reefCouplingBufLen_{0};     // valid samples in reefCouplingBuf_
    float reefCouplingRms_ = 0.0f;               // smoothed full-band RMS of coupling input
    float reefCouplingHpFilt_ = 0.0f;            // 1-pole HP filter state for HF extraction
    float reefCouplingBufPrev_ = 0.0f;           // previous sample for HP difference equation
    float reefCouplingHfRms_ = 0.0f;             // smoothed high-frequency RMS (parasite bleach)
    float reefHpCoeff_ = 0.0f;                   // cached 1-pole HP coefficient at 2kHz (computed in prepare)

    // 3 independent FX slots
    ObrixFXState fxSlots[3];

    // Parameter pointers
    std::atomic<float>*pSrc1Type = nullptr, *pSrc2Type = nullptr;
    std::atomic<float>*pSrc1Tune = nullptr, *pSrc2Tune = nullptr;
    std::atomic<float>*pSrc1PW = nullptr, *pSrc2PW = nullptr;
    std::atomic<float>* pSrcMix = nullptr;
    std::atomic<float>*pProc1Type = nullptr, *pProc1Cutoff = nullptr, *pProc1Reso = nullptr;
    std::atomic<float>*pProc2Type = nullptr, *pProc2Cutoff = nullptr, *pProc2Reso = nullptr;
    std::atomic<float>*pProc3Type = nullptr, *pProc3Cutoff = nullptr, *pProc3Reso = nullptr;
    std::atomic<float>*pAmpAttack = nullptr, *pAmpDecay = nullptr;
    std::atomic<float>*pAmpSustain = nullptr, *pAmpRelease = nullptr;
    std::atomic<float>*pModType[4]{}, *pModTarget[4]{}, *pModDepth[4]{}, *pModRate[4]{};
    std::atomic<float>*pFxType[3]{}, *pFxMix[3]{}, *pFxParam[3]{};
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>*pMacroChar = nullptr, *pMacroMove = nullptr;
    std::atomic<float>*pMacroCoup = nullptr, *pMacroSpace = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>*pPitchBendRange = nullptr, *pGlideTime = nullptr;
    std::atomic<float>*pGestureType = nullptr, *pFlashTrigger = nullptr;

    // Wave 2 parameter pointers
    std::atomic<float>* pFmDepth = nullptr;
    std::atomic<float>* pProc1Fb = nullptr;
    std::atomic<float>* pProc2Fb = nullptr;
    std::atomic<float>* pProc3Fb = nullptr;
    std::atomic<float>* pWtBank = nullptr;
    std::atomic<float>* pUnisonDetune = nullptr;

    // Wave 3 parameter pointers
    std::atomic<float>* pDriftRate = nullptr;   // Drift Bus rate (0.001–0.05 Hz)
    std::atomic<float>* pDriftDepth = nullptr;  // Drift Bus depth (0=off, 1=full)
    std::atomic<float>* pJourneyMode = nullptr; // Journey: suppress note-off (0/1 toggle)
    std::atomic<float>* pDistance = nullptr;    // DISTANCE HF rolloff (0=close, 1=far)
    std::atomic<float>* pAir = nullptr;         // AIR spectral tilt (0=warm, 1=cold)

    // Wave 4 — Biophonic Synthesis parameter pointers
    std::atomic<float>* pFieldStrength = nullptr;       // Harmonic Field strength
    std::atomic<float>* pFieldPolarity = nullptr;       // 1=attractor, 0=repulsor
    std::atomic<float>* pFieldRate = nullptr;           // IIR convergence rate
    std::atomic<float>* pFieldPrimeLimit = nullptr;     // 0=3-limit, 1=5-limit, 2=7-limit
    std::atomic<float>* pEnvTemp = nullptr;             // Temperature (drift amplifier)
    std::atomic<float>* pEnvPressure = nullptr;         // Pressure (LFO rate scale)
    std::atomic<float>* pEnvCurrent = nullptr;          // Current (directional bias)
    std::atomic<float>* pEnvTurbidity = nullptr;        // Turbidity (spectral noise)
    std::atomic<float>* pCompetitionStrength = nullptr; // Brick competition strength
    std::atomic<float>* pSymbiosisStrength = nullptr;   // Brick symbiosis strength
    std::atomic<float>* pStressDecay = nullptr;         // Stress Memory decay scale
    std::atomic<float>* pBleachRate = nullptr;          // Bleaching accumulation rate
    std::atomic<float>* pStateReset = nullptr;          // Momentary reef state reset
    std::atomic<float>* pFxMode = nullptr;              // 0=Serial, 1=Parallel

    // Wave 5 — Reef Residency parameter pointers
    std::atomic<float>* pReefResident = nullptr;     // 0=Off, 1=Competitor, 2=Symbiote, 3=Parasite
    std::atomic<float>* pResidentStrength = nullptr; // 0.0–1.0, Guru Bin default 0.3

    // Wavetable banks (Wave 2): 4 banks × 512 samples, built once in prepare()
    static constexpr int kWTSize = 512;
    float wavetables[4][kWTSize]{};

    //==========================================================================
    // Wave 2 helpers
    //==========================================================================

    // Unison size derived from voice mode (Mono=8, Poly4=2, Legato/Poly8=1)
    static int calcUnisonSize(int voiceModeIdx, float unisonDetune) noexcept
    {
        if (unisonDetune < 0.1f)
            return 1;
        switch (voiceModeIdx)
        {
        case 0:
            return 8; // Mono: full 8-voice supersaw
        case 2:
            return 2; // Poly4: 2-voice per note (2 notes max simultaneously)
        default:
            return 1; // Legato / Poly8: no stacking
        }
    }

    // Build 4 single-cycle wavetable banks (runs once at prepare time)
    void buildWavetables() noexcept
    {
        // Bank 0 — Analog: additive saw (Σ sin(2πkx)/k, 12 harmonics)
        for (int i = 0; i < kWTSize; ++i)
        {
            float t = static_cast<float>(i) / kWTSize;
            float v = 0.0f;
            for (int k = 1; k <= 12; ++k)
                v += std::sin(t * kTwoPi * k) / static_cast<float>(k);
            wavetables[0][i] = v * 0.55f;
        }

        // Bank 1 — Vocal: emphasized 2nd + 4th harmonics (formant vowel character)
        for (int i = 0; i < kWTSize; ++i)
        {
            float t = static_cast<float>(i) / kWTSize;
            float v = std::sin(t * kTwoPi) + 0.80f * std::sin(t * kTwoPi * 2.0f) + 0.30f * std::sin(t * kTwoPi * 3.0f) +
                      0.60f * std::sin(t * kTwoPi * 4.0f) + 0.15f * std::sin(t * kTwoPi * 5.0f) +
                      0.35f * std::sin(t * kTwoPi * 6.0f);
            wavetables[1][i] = v * 0.30f;
        }

        // Bank 2 — Metallic: odd harmonics with slight inharmonic stretch
        for (int i = 0; i < kWTSize; ++i)
        {
            float t = static_cast<float>(i) / kWTSize;
            float v = std::sin(t * kTwoPi) + 0.70f * std::sin(t * kTwoPi * 3.02f) +
                      0.50f * std::sin(t * kTwoPi * 5.05f) + 0.35f * std::sin(t * kTwoPi * 7.10f) +
                      0.20f * std::sin(t * kTwoPi * 9.15f) + 0.12f * std::sin(t * kTwoPi * 11.2f);
            wavetables[2][i] = v * 0.38f;
        }

        // Bank 3 — Organic: warm beating partials (ultra-slight detuning creates aliveness)
        for (int i = 0; i < kWTSize; ++i)
        {
            float t = static_cast<float>(i) / kWTSize;
            float v = std::sin(t * kTwoPi) + 0.50f * std::sin(t * kTwoPi * 2.002f) +
                      0.25f * std::sin(t * kTwoPi * 3.0f) + 0.18f * std::sin(t * kTwoPi * 4.003f) +
                      0.10f * std::sin(t * kTwoPi * 5.001f);
            wavetables[3][i] = v * 0.48f;
        }
    }
};

} // namespace xoceanus
