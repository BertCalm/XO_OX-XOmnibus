#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// ObscuraEngine — Scanned Synthesis via Mass-Spring Chain Physics.
//
// Implements Bill Verplank / Max Mathews scanned synthesis: a 128-mass
// spring chain is simulated using Verlet integration at ~4 kHz control rate.
// A scanner sweeps across the chain at MIDI note frequency, reading
// displacements via cubic interpolation to produce audio-rate output.
//
// Features:
//   - 128-mass spring chain with configurable stiffness, damping, nonlinearity
//   - Verlet integration (energy-stable, no velocity storage needed)
//   - Excitation: Gaussian impulse (note-on) + continuous bowing force
//   - 3 boundary modes: Fixed / Free / Periodic
//   - 4 initial chain shapes: Sine / Saw / Random / Flat
//   - Scanner with variable width (bright to dark timbres)
//   - Cubic interpolation for smooth audio-rate chain readout
//   - Stereo: L scans forward, R scans backward
//   - DC blocker + soft limiter on output
//   - Physics envelope (ADSR) modulates excitation force
//   - Mono/Legato/Poly4/Poly8 voice modes with LRU stealing + 5ms crossfade
//   - 2 LFOs (Sine/Tri/Saw/Square/S&H)
//   - Full XOmnibus coupling support
//
// Coupling:
//   - Output: post-output stereo audio via getSampleForCoupling
//   - Input: AudioToFM (force on chain masses), AmpToFilter (modulates stiffness),
//            RhythmToBlend (triggers impulse excitation)
//
//==============================================================================

//-- Chain constants -----------------------------------------------------------
static constexpr int kChainSize = 128;

//==============================================================================
// ADSR envelope generator — lightweight, inline, no allocation.
//==============================================================================
struct ObscuraADSR
{
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    Stage stage = Stage::Idle;
    float level = 0.0f;
    float attackRate  = 0.0f;
    float decayRate   = 0.0f;
    float sustainLevel = 1.0f;
    float releaseRate = 0.0f;

    void setParams (float attackSec, float decaySec, float sustain, float releaseSec,
                    float sampleRate) noexcept
    {
        float sr = std::max (1.0f, sampleRate);
        attackRate  = (attackSec  > 0.001f) ? (1.0f / (attackSec  * sr)) : 1.0f;
        decayRate   = (decaySec   > 0.001f) ? (1.0f / (decaySec   * sr)) : 1.0f;
        sustainLevel = sustain;
        releaseRate = (releaseSec > 0.001f) ? (1.0f / (releaseSec * sr)) : 1.0f;
    }

    void noteOn() noexcept
    {
        stage = Stage::Attack;
    }

    void noteOff() noexcept
    {
        if (stage != Stage::Idle)
            stage = Stage::Release;
    }

    float process() noexcept
    {
        switch (stage)
        {
            case Stage::Idle:
                return 0.0f;

            case Stage::Attack:
                level += attackRate;
                if (level >= 1.0f)
                {
                    level = 1.0f;
                    stage = Stage::Decay;
                }
                return level;

            case Stage::Decay:
                level -= decayRate * (level - sustainLevel + 0.0001f);
                if (level <= sustainLevel + 0.0001f)
                {
                    level = sustainLevel;
                    stage = Stage::Sustain;
                }
                return level;

            case Stage::Sustain:
                return level;

            case Stage::Release:
                level -= releaseRate * (level + 0.0001f);
                if (level <= 0.0001f)
                {
                    level = 0.0f;
                    stage = Stage::Idle;
                }
                return level;
        }
        return 0.0f;
    }

    bool isActive() const noexcept { return stage != Stage::Idle; }

    void reset() noexcept
    {
        stage = Stage::Idle;
        level = 0.0f;
    }
};

//==============================================================================
// LFO with multiple shapes.
//==============================================================================
struct ObscuraLFO
{
    enum class Shape { Sine, Triangle, Saw, Square, SandH };

    float phase = 0.0f;
    float phaseInc = 0.0f;
    Shape shape = Shape::Sine;
    float holdValue = 0.0f;
    uint32_t sampleCounter = 0;

    void setRate (float hz, float sampleRate) noexcept
    {
        phaseInc = hz / std::max (1.0f, sampleRate);
    }

    void setShape (int idx) noexcept
    {
        shape = static_cast<Shape> (std::min (4, std::max (0, idx)));
    }

    float process() noexcept
    {
        float out = 0.0f;

        switch (shape)
        {
            case Shape::Sine:
                out = fastSin (phase * 6.28318530718f);
                break;
            case Shape::Triangle:
                out = 4.0f * std::fabs (phase - 0.5f) - 1.0f;
                break;
            case Shape::Saw:
                out = 2.0f * phase - 1.0f;
                break;
            case Shape::Square:
                out = (phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case Shape::SandH:
            {
                float prevPhase = phase - phaseInc;
                if (prevPhase < 0.0f || phase < prevPhase)
                {
                    sampleCounter = sampleCounter * 1664525u + 1013904223u;
                    holdValue = static_cast<float> (sampleCounter & 0xFFFF) / 32768.0f - 1.0f;
                }
                out = holdValue;
                break;
            }
        }

        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

    void reset() noexcept
    {
        phase = 0.0f;
        holdValue = 0.0f;
        sampleCounter = 12345u;
    }
};

//==============================================================================
// DC Blocker — single-pole high-pass at ~5 Hz.
//==============================================================================
struct ObscuraDCBlocker
{
    float x1 = 0.0f;
    float y1 = 0.0f;
    float R = 0.995f;

    void setCoefficient (float sampleRate) noexcept
    {
        // R = 1 - (2*pi*fc / sr), fc ~ 5 Hz
        R = 1.0f - (6.28318530718f * 5.0f / std::max (1.0f, sampleRate));
        if (R < 0.9f) R = 0.9f;
        if (R > 0.9999f) R = 0.9999f;
    }

    float process (float x) noexcept
    {
        float y = x - x1 + R * y1;
        x1 = x;
        y1 = flushDenormal (y);
        return y;
    }

    void reset() noexcept
    {
        x1 = 0.0f;
        y1 = 0.0f;
    }
};

//==============================================================================
// ObscuraVoice — per-voice state including mass-spring chain.
//==============================================================================
struct ObscuraVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // MPE per-voice expression state
    MPEVoiceExpression mpeExpression;

    // Mass-spring chain state (Verlet integration needs current + previous)
    float chain[kChainSize] = {};
    float chainPrev[kChainSize] = {};

    // Control-rate chain snapshots for audio-rate interpolation
    float chainSnapshotA[kChainSize] = {};  // previous control-rate frame
    float chainSnapshotB[kChainSize] = {};  // current control-rate frame

    // Scanner phase accumulator
    float scanPhase = 0.0f;

    // Glide
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideCoeff = 1.0f;

    // Control-rate accumulator
    float controlPhaseAcc = 0.0f;

    // Excitation state
    bool impulseTriggered = false;
    float exciteForce = 0.0f;     // continuous bowing force

    // Envelopes
    ObscuraADSR ampEnv;
    ObscuraADSR physEnv;

    // LFOs (per-voice for independence)
    ObscuraLFO lfo1;
    ObscuraLFO lfo2;

    // DC blockers (stereo)
    ObscuraDCBlocker dcBlockL;
    ObscuraDCBlocker dcBlockR;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    // Random seed for S&H and chain init
    uint32_t rngState = 42u;

    float nextRandom() noexcept
    {
        rngState = rngState * 1664525u + 1013904223u;
        return static_cast<float> (rngState & 0xFFFF) / 32768.0f - 1.0f;
    }

    void initChainShape (int shapeIdx, float excitePos, float exciteWidth) noexcept
    {
        constexpr float pi = 3.14159265358979323846f;

        for (int i = 0; i < kChainSize; ++i)
        {
            float t = static_cast<float> (i) / static_cast<float> (kChainSize - 1);
            float val = 0.0f;

            switch (shapeIdx)
            {
                case 0: // Sine — one period
                    val = std::sin (pi * t) * 0.1f;
                    break;
                case 1: // Saw — ramp
                    val = (2.0f * t - 1.0f) * 0.1f;
                    break;
                case 2: // Random — noise
                    val = nextRandom() * 0.05f;
                    break;
                case 3: // Flat — zero everywhere, excitation only
                default:
                    val = 0.0f;
                    break;
            }

            chain[i] = val;
            chainPrev[i] = val;
        }

        // Apply initial Gaussian excitation impulse
        applyImpulse (excitePos, exciteWidth, 0.15f);

        // Copy to snapshots
        for (int i = 0; i < kChainSize; ++i)
        {
            chainSnapshotA[i] = chain[i];
            chainSnapshotB[i] = chain[i];
        }
    }

    void applyImpulse (float pos, float width, float amplitude) noexcept
    {
        float center = pos * static_cast<float> (kChainSize - 1);
        float sigma = std::max (0.5f, width * static_cast<float> (kChainSize) * 0.25f);
        float invSigma2 = 1.0f / (2.0f * sigma * sigma);

        for (int i = 1; i < kChainSize - 1; ++i)
        {
            float dist = static_cast<float> (i) - center;
            float gauss = amplitude * fastExp (-dist * dist * invSigma2);
            chain[i] += gauss;
        }
    }

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        scanPhase = 0.0f;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        glideCoeff = 1.0f;
        controlPhaseAcc = 0.0f;
        impulseTriggered = false;
        exciteForce = 0.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        rngState = 42u;
        ampEnv.reset();
        physEnv.reset();
        lfo1.reset();
        lfo2.reset();
        dcBlockL.reset();
        dcBlockR.reset();

        for (int i = 0; i < kChainSize; ++i)
        {
            chain[i] = 0.0f;
            chainPrev[i] = 0.0f;
            chainSnapshotA[i] = 0.0f;
            chainSnapshotB[i] = 0.0f;
        }
    }
};

//==============================================================================
// ObscuraEngine — the main engine class.
//==============================================================================
class ObscuraEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 8;
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    // Control rate for physics simulation (~4 kHz)
    static constexpr float kControlRate = 4000.0f;

    // Maximum safe spring constant: k * dt^2 < 1.0
    // dt = 1/4000, dt^2 = 6.25e-8 ... but we use scaled dt.
    // We define dt_phys = 1.0 (normalized), so k_max < 1.0
    static constexpr float kMaxSpringK = 0.95f;

    // Chain displacement soft-clip threshold
    static constexpr float kChainClipScale = 4.0f;
    static constexpr float kChainClipMax = 0.25f;

    //==========================================================================
    // SynthEngine interface — Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);

        // Control rate stepping: how many audio samples per physics step
        controlStepSamples = srf / kControlRate;
        if (controlStepSamples < 1.0f) controlStepSamples = 1.0f;

        // Physics dt^2 (normalized so that spring constant directly controls stability)
        // With control rate at 4kHz: dt = 1/4000, dt^2 = 6.25e-8
        // We scale forces so that k maps [0, kMaxSpringK] and k * dt2_norm < 1
        dt2 = 1.0f;  // normalized; spring constant IS the effective k*dt^2

        // Smoothing coefficient (5ms time constant)
        smoothCoeff = 1.0f - std::exp (-kTwoPi * (1.0f / 0.005f) / srf);

        // Crossfade rate (5ms)
        crossfadeRate = 1.0f / (0.005f * srf);

        // Allocate output cache for coupling reads
        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Initialize voices
        for (auto& v : voices)
        {
            v.reset();
            v.dcBlockL.setCoefficient (srf);
            v.dcBlockR.setCoefficient (srf);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingForceMod = 0.0f;
        couplingStiffMod = 0.0f;
        couplingImpulseTrigger = 0.0f;

        smoothedStiffness = 0.5f;
        smoothedDamping = 0.3f;
        smoothedNonlinear = 0.0f;
        smoothedScanWidth = 0.5f;
        smoothedSustainForce = 0.0f;

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // SynthEngine interface — Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---
        const float pStiffVal      = loadParam (pStiffness, 0.5f);
        const float pDampVal       = loadParam (pDamping, 0.3f);
        const float pNonlinVal     = loadParam (pNonlinear, 0.0f);
        const float pExcPosVal     = loadParam (pExcitePos, 0.5f);
        const float pExcWidthVal   = loadParam (pExciteWidth, 0.3f);
        const float pScanWidthVal  = loadParam (pScanWidth, 0.5f);
        const int   pBoundaryVal   = static_cast<int> (loadParam (pBoundary, 0.0f));
        const float pSustainVal    = loadParam (pSustain, 0.0f);

        const float pAmpA          = loadParam (pAmpAttack, 0.01f);
        const float pAmpD          = loadParam (pAmpDecay, 0.1f);
        const float pAmpS          = loadParam (pAmpSustain, 0.8f);
        const float pAmpR          = loadParam (pAmpRelease, 0.3f);
        const float pPhysA         = loadParam (pPhysAttack, 0.01f);
        const float pPhysD         = loadParam (pPhysDecay, 0.2f);
        const float pPhysS         = loadParam (pPhysSustain, 0.5f);
        const float pPhysR         = loadParam (pPhysRelease, 0.3f);

        const float pLfo1R         = loadParam (pLfo1Rate, 1.0f);
        const float pLfo1D         = loadParam (pLfo1Depth, 0.0f);
        const int   pLfo1S         = static_cast<int> (loadParam (pLfo1Shape, 0.0f));
        const float pLfo2R         = loadParam (pLfo2Rate, 1.0f);
        const float pLfo2D         = loadParam (pLfo2Depth, 0.0f);
        const int   pLfo2S         = static_cast<int> (loadParam (pLfo2Shape, 0.0f));

        const int   voiceModeIdx   = static_cast<int> (loadParam (pVoiceMode, 2.0f));
        const float glideTime      = loadParam (pGlide, 0.0f);
        const int   initShapeIdx   = static_cast<int> (loadParam (pInitShape, 0.0f));

        const float pLevelVal      = loadParam (pLevel, 0.8f);

        const float macroChar      = loadParam (pMacroCharacter, 0.0f);
        const float macroMove      = loadParam (pMacroMovement, 0.0f);
        const float macroCoup      = loadParam (pMacroCoupling, 0.0f);
        const float macroSpace     = loadParam (pMacroSpace, 0.0f);

        // Determine max polyphony from voice mode
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIdx)
        {
            case 0: maxPoly = 1; monoMode = true; break;          // Mono
            case 1: maxPoly = 1; monoMode = true; legatoMode = true; break; // Legato
            case 2: maxPoly = 4; break;                            // Poly4
            case 3: maxPoly = 8; break;                            // Poly8
            default: maxPoly = 4; break;
        }

        // Glide coefficient
        float glideCoeff = 1.0f;
        if (glideTime > 0.001f)
            glideCoeff = 1.0f - std::exp (-1.0f / (glideTime * srf));

        // Apply macro offsets to effective parameters
        float effectiveStiff    = clamp (pStiffVal + macroChar * 0.3f + couplingStiffMod, 0.0f, 1.0f);
        float effectiveDamp     = clamp (pDampVal + macroSpace * 0.2f, 0.0f, 1.0f);
        float effectiveNonlin   = clamp (pNonlinVal + macroChar * 0.2f, 0.0f, 1.0f);
        float effectiveScanW    = clamp (pScanWidthVal + macroMove * 0.3f, 0.0f, 1.0f);
        float effectiveSustain  = clamp (pSustainVal + macroCoup * 0.3f, 0.0f, 1.0f);

        // Map stiffness [0,1] exponentially to spring constant k
        // Ensure k * dt2 < 1.0 for Verlet stability
        float springK = effectiveStiff * effectiveStiff * kMaxSpringK;

        // Damping coefficient: [0,1] -> [0, 0.15]
        float dampCoeff = effectiveDamp * 0.15f;

        // Nonlinear (cubic) spring coefficient
        float cubicK = effectiveNonlin * effectiveNonlin * 0.5f;

        // Scanner width: [0,1] -> [1, N/4] masses
        float scanWidthMasses = 1.0f + effectiveScanW * (static_cast<float> (kChainSize) * 0.25f - 1.0f);

        // Reset coupling accumulators
        float localCoupForce = couplingForceMod;
        float localCoupImpulse = couplingImpulseTrigger;
        couplingForceMod = 0.0f;
        couplingStiffMod = 0.0f;
        couplingImpulseTrigger = 0.0f;

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), msg.getChannel(), maxPoly,
                        monoMode, legatoMode, glideCoeff,
                        pAmpA, pAmpD, pAmpS, pAmpR,
                        pPhysA, pPhysD, pPhysS, pPhysR,
                        pLfo1R, pLfo1D, pLfo1S, pLfo2R, pLfo2D, pLfo2S,
                        initShapeIdx, pExcPosVal, pExcWidthVal);
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber(), msg.getChannel());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
        }

        // --- Update per-voice MPE expression from MPEManager ---
        if (mpeManager != nullptr)
        {
            for (auto& voice : voices)
            {
                if (!voice.active) continue;
                mpeManager->updateVoiceExpression(voice.mpeExpression);
            }
        }

        // If coupling triggered an impulse, apply to all active voices
        if (localCoupImpulse > 0.01f)
        {
            for (auto& voice : voices)
            {
                if (voice.active && !voice.fadingOut)
                    voice.applyImpulse (pExcPosVal, pExcWidthVal, localCoupImpulse * 0.1f);
            }
        }

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Smooth control-rate parameters
            smoothedStiffness    += (effectiveStiff - smoothedStiffness) * smoothCoeff;
            smoothedDamping      += (effectiveDamp - smoothedDamping) * smoothCoeff;
            smoothedNonlinear    += (effectiveNonlin - smoothedNonlinear) * smoothCoeff;
            smoothedScanWidth    += (scanWidthMasses - smoothedScanWidth) * smoothCoeff;
            smoothedSustainForce += (effectiveSustain - smoothedSustainForce) * smoothCoeff;

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // --- Voice-stealing crossfade (5ms) ---
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    if (voice.fadeGain <= 0.0f)
                    {
                        voice.fadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // --- Glide (portamento) ---
                voice.currentFreq += (voice.targetFreq - voice.currentFreq) * voice.glideCoeff;

                // --- Envelopes ---
                float ampLevel = voice.ampEnv.process();
                float physLevel = voice.physEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Val = voice.lfo1.process() * pLfo1D;
                float lfo2Val = voice.lfo2.process() * pLfo2D;

                // LFO1 -> scan width modulation, LFO2 -> excitation position modulation
                float modScanWidth = std::max (1.0f, smoothedScanWidth + lfo1Val * 8.0f);
                float modExcitePos = clamp (pExcPosVal + lfo2Val * 0.2f, 0.0f, 1.0f);

                // --- Physics simulation at control rate ---
                // Accumulate control-rate phase; step physics when threshold reached
                voice.controlPhaseAcc += 1.0f;

                if (voice.controlPhaseAcc >= controlStepSamples)
                {
                    voice.controlPhaseAcc -= controlStepSamples;

                    // Copy current snapshot to previous
                    for (int i = 0; i < kChainSize; ++i)
                        voice.chainSnapshotA[i] = voice.chainSnapshotB[i];

                    // Apply continuous excitation (bowing) force
                    if (smoothedSustainForce > 0.001f)
                    {
                        float forceAmt = smoothedSustainForce * physLevel * 0.02f;
                        float center = modExcitePos * static_cast<float> (kChainSize - 1);
                        float sigma = std::max (1.0f, pExcWidthVal * static_cast<float> (kChainSize) * 0.15f);
                        float invSigma2 = 1.0f / (2.0f * sigma * sigma);

                        for (int i = 1; i < kChainSize - 1; ++i)
                        {
                            float dist = static_cast<float> (i) - center;
                            float gauss = forceAmt * fastExp (-dist * dist * invSigma2);
                            voice.chain[i] += gauss;
                        }
                    }

                    // Apply coupling force modulation
                    if (std::fabs (localCoupForce) > 0.0001f)
                    {
                        float forceScale = localCoupForce * physLevel * 0.01f;
                        for (int i = 1; i < kChainSize - 1; ++i)
                        {
                            // Distribute coupling force with slight spatial variation
                            float t = static_cast<float> (i) / static_cast<float> (kChainSize - 1);
                            voice.chain[i] += forceScale * fastSin (t * kTwoPi);
                        }
                    }

                    // --- Verlet integration step ---
                    updateChain (voice.chain, voice.chainPrev, kChainSize,
                                 springK, dampCoeff, cubicK, pBoundaryVal);

                    // Soft-clip chain displacement for stability
                    for (int i = 0; i < kChainSize; ++i)
                    {
                        float x = voice.chain[i];
                        // Denormal flush
                        if (std::fabs (x) < 1e-15f)
                        {
                            voice.chain[i] = 0.0f;
                            voice.chainPrev[i] = flushDenormal (voice.chainPrev[i]);
                            continue;
                        }
                        // Soft clip: tanh(x * 4) * 0.25
                        voice.chain[i] = fastTanh (x * kChainClipScale) * kChainClipMax;
                        voice.chainPrev[i] = flushDenormal (voice.chainPrev[i]);
                    }

                    // Copy to snapshot B
                    for (int i = 0; i < kChainSize; ++i)
                        voice.chainSnapshotB[i] = voice.chain[i];
                }

                // --- Audio-rate interpolation between snapshots ---
                float interpFrac = voice.controlPhaseAcc / controlStepSamples;
                interpFrac = clamp (interpFrac, 0.0f, 1.0f);

                // --- Scanner: sweep across chain at MIDI note frequency ---
                float scanFreq = voice.currentFreq * std::pow (2.0f, voice.mpeExpression.pitchBendSemitones / 12.0f);
                float scanInc = scanFreq / srf;

                // Forward scan (L channel)
                float scanPosL = voice.scanPhase;
                float outL = scanChain (voice.chainSnapshotA, voice.chainSnapshotB,
                                        interpFrac, scanPosL, modScanWidth);

                // Backward scan (R channel)
                float scanPosR = 1.0f - voice.scanPhase;
                float outR = scanChain (voice.chainSnapshotA, voice.chainSnapshotB,
                                        interpFrac, scanPosR, modScanWidth);

                // Advance scanner phase
                voice.scanPhase += scanInc;
                while (voice.scanPhase >= 1.0f) voice.scanPhase -= 1.0f;
                while (voice.scanPhase < 0.0f) voice.scanPhase += 1.0f;

                // --- DC blocker ---
                outL = voice.dcBlockL.process (outL);
                outR = voice.dcBlockR.process (outR);

                // --- Soft limiter ---
                outL = fastTanh (outL * 3.0f) * 0.33f;
                outR = fastTanh (outR * 3.0f) * 0.33f;

                // --- Apply amplitude envelope, velocity, and crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                outL *= gain;
                outR *= gain;

                // Denormal protection on outputs
                outL = flushDenormal (outL);
                outR = flushDenormal (outR);

                mixL += outL;
                mixR += outR;

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // Apply master level
            float finalL = mixL * pLevelVal;
            float finalR = mixR * pLevelVal;

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample (0, sample, finalL);
                buffer.addSample (1, sample, finalR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample (0, sample, (finalL + finalR) * 0.5f);
            }

            // Cache for coupling reads
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = finalL;
                outputCacheR[static_cast<size_t> (sample)] = finalR;
            }
        }

        envelopeOutput = peakEnv;

        // Update active voice count
        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        activeVoices = count;
    }

    //==========================================================================
    // SynthEngine interface — Coupling
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
            case CouplingType::AudioToFM:
                // External audio applied as force on chain masses
                couplingForceMod += amount * 0.5f;
                break;

            case CouplingType::AmpToFilter:
                // External amplitude modulates stiffness
                couplingStiffMod += amount * 0.3f;
                break;

            case CouplingType::RhythmToBlend:
                // External rhythm triggers an impulse on the chain
                couplingImpulseTrigger += amount;
                break;

            default:
                break;
        }
    }

    //==========================================================================
    // SynthEngine interface — Parameters
    //==========================================================================

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Core Physics Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_stiffness", 1 }, "Obscura Stiffness",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_damping", 1 }, "Obscura Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_nonlinear", 1 }, "Obscura Nonlinear",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_excitePos", 1 }, "Obscura Excite Position",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_exciteWidth", 1 }, "Obscura Excite Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_scanWidth", 1 }, "Obscura Scan Width",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_boundary", 1 }, "Obscura Boundary Mode",
            juce::StringArray { "Fixed", "Free", "Periodic" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_sustain", 1 }, "Obscura Sustain Force",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_level", 1 }, "Obscura Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_ampAttack", 1 }, "Obscura Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_ampDecay", 1 }, "Obscura Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_ampSustain", 1 }, "Obscura Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_ampRelease", 1 }, "Obscura Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- Physics Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_physEnvAttack", 1 }, "Obscura Phys Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_physEnvDecay", 1 }, "Obscura Phys Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_physEnvSustain", 1 }, "Obscura Phys Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_physEnvRelease", 1 }, "Obscura Phys Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- LFO 1 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_lfo1Rate", 1 }, "Obscura LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_lfo1Depth", 1 }, "Obscura LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_lfo1Shape", 1 }, "Obscura LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- LFO 2 ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_lfo2Rate", 1 }, "Obscura LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_lfo2Depth", 1 }, "Obscura LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_lfo2Shape", 1 }, "Obscura LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- Voice Parameters ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_voiceMode", 1 }, "Obscura Voice Mode",
            juce::StringArray { "Mono", "Legato", "Poly4", "Poly8" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_glide", 1 }, "Obscura Glide",
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "obscura_initShape", 1 }, "Obscura Init Shape",
            juce::StringArray { "Sine", "Saw", "Random", "Flat" }, 0));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_macroCharacter", 1 }, "Obscura Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_macroMovement", 1 }, "Obscura Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_macroCoupling", 1 }, "Obscura Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "obscura_macroSpace", 1 }, "Obscura Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pStiffness       = apvts.getRawParameterValue ("obscura_stiffness");
        pDamping         = apvts.getRawParameterValue ("obscura_damping");
        pNonlinear       = apvts.getRawParameterValue ("obscura_nonlinear");
        pExcitePos       = apvts.getRawParameterValue ("obscura_excitePos");
        pExciteWidth     = apvts.getRawParameterValue ("obscura_exciteWidth");
        pScanWidth       = apvts.getRawParameterValue ("obscura_scanWidth");
        pBoundary        = apvts.getRawParameterValue ("obscura_boundary");
        pSustain         = apvts.getRawParameterValue ("obscura_sustain");
        pLevel           = apvts.getRawParameterValue ("obscura_level");

        pAmpAttack       = apvts.getRawParameterValue ("obscura_ampAttack");
        pAmpDecay        = apvts.getRawParameterValue ("obscura_ampDecay");
        pAmpSustain      = apvts.getRawParameterValue ("obscura_ampSustain");
        pAmpRelease      = apvts.getRawParameterValue ("obscura_ampRelease");

        pPhysAttack      = apvts.getRawParameterValue ("obscura_physEnvAttack");
        pPhysDecay       = apvts.getRawParameterValue ("obscura_physEnvDecay");
        pPhysSustain     = apvts.getRawParameterValue ("obscura_physEnvSustain");
        pPhysRelease     = apvts.getRawParameterValue ("obscura_physEnvRelease");

        pLfo1Rate        = apvts.getRawParameterValue ("obscura_lfo1Rate");
        pLfo1Depth       = apvts.getRawParameterValue ("obscura_lfo1Depth");
        pLfo1Shape       = apvts.getRawParameterValue ("obscura_lfo1Shape");
        pLfo2Rate        = apvts.getRawParameterValue ("obscura_lfo2Rate");
        pLfo2Depth       = apvts.getRawParameterValue ("obscura_lfo2Depth");
        pLfo2Shape       = apvts.getRawParameterValue ("obscura_lfo2Shape");

        pVoiceMode       = apvts.getRawParameterValue ("obscura_voiceMode");
        pGlide           = apvts.getRawParameterValue ("obscura_glide");
        pInitShape       = apvts.getRawParameterValue ("obscura_initShape");

        pMacroCharacter  = apvts.getRawParameterValue ("obscura_macroCharacter");
        pMacroMovement   = apvts.getRawParameterValue ("obscura_macroMovement");
        pMacroCoupling   = apvts.getRawParameterValue ("obscura_macroCoupling");
        pMacroSpace      = apvts.getRawParameterValue ("obscura_macroSpace");
    }

    //==========================================================================
    // SynthEngine interface — Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Obscura"; }

    // Daguerreotype Silver #8A9BA8
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF8A9BA8); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices; }

private:
    //==========================================================================
    // Helper: safe parameter load
    //==========================================================================

    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // Mass-Spring Chain: Verlet Integration
    //==========================================================================

    static void updateChain (float* x, float* x_old, int N,
                             float k, float d, float k3,
                             int boundaryMode) noexcept
    {
        // Apply boundary conditions before integration
        switch (boundaryMode)
        {
            case 0: // Fixed: endpoints pinned to zero
                x[0] = 0.0f;
                x[N - 1] = 0.0f;
                x_old[0] = 0.0f;
                x_old[N - 1] = 0.0f;
                break;

            case 1: // Free: endpoints mirror neighbors
                x[0] = x[1];
                x[N - 1] = x[N - 2];
                x_old[0] = x_old[1];
                x_old[N - 1] = x_old[N - 2];
                break;

            case 2: // Periodic: wrap around
                x[0] = x[N - 2];
                x[N - 1] = x[1];
                x_old[0] = x_old[N - 2];
                x_old[N - 1] = x_old[1];
                break;

            default:
                x[0] = 0.0f;
                x[N - 1] = 0.0f;
                x_old[0] = 0.0f;
                x_old[N - 1] = 0.0f;
                break;
        }

        // Verlet integration for interior masses
        for (int i = 1; i < N - 1; ++i)
        {
            float dx_left  = x[i] - x[i - 1];
            float dx_right = x[i + 1] - x[i];

            // Linear spring force: F = k * (x[i+1] - 2*x[i] + x[i-1])
            float f = k * (x[i + 1] - 2.0f * x[i] + x[i - 1]);

            // Cubic nonlinear spring: F = k3 * (dx_right^3 - dx_left^3)
            f += k3 * (dx_right * dx_right * dx_right
                      - dx_left * dx_left * dx_left);

            // Velocity estimate from Verlet
            float v = x[i] - x_old[i];

            // Damping force: F = -d * v
            f -= d * v;

            // Verlet step: x_new = 2*x - x_old + F * dt^2
            // dt^2 is normalized to 1.0; k already encodes the effective dt^2 scaling
            float x_new = 2.0f * x[i] - x_old[i] + f;

            x_old[i] = x[i];
            x[i] = x_new;
        }
    }

    //==========================================================================
    // Scanner: read chain displacement with cubic interpolation.
    //
    // scanPos is [0, 1) — position along chain.
    // scanWidth controls the averaging window (1 = point, N/4 = wide/dark).
    // Returns weighted average of nearby masses using cubic kernel.
    //==========================================================================

    static float scanChain (const float* snapA, const float* snapB,
                            float interpFrac, float scanPos,
                            float scanWidth) noexcept
    {
        // Map scanPos [0,1) to chain index [0, N-1]
        float pos = scanPos * static_cast<float> (kChainSize - 1);

        // For narrow width (< 2), use cubic interpolation at a single point
        if (scanWidth < 2.0f)
        {
            return cubicInterpChain (snapA, snapB, interpFrac, pos);
        }

        // For wider widths, average over a window centered on pos
        float halfWidth = scanWidth * 0.5f;
        float startF = pos - halfWidth;
        float endF = pos + halfWidth;

        // Number of samples in the window
        int numSamples = static_cast<int> (scanWidth) + 1;
        if (numSamples < 2) numSamples = 2;
        if (numSamples > kChainSize) numSamples = kChainSize;

        float step = (endF - startF) / static_cast<float> (numSamples - 1);
        float sum = 0.0f;
        float weightSum = 0.0f;

        for (int s = 0; s < numSamples; ++s)
        {
            float samplePos = startF + static_cast<float> (s) * step;

            // Wrap position to [0, N-1] range
            float wrappedPos = samplePos;
            float chainEnd = static_cast<float> (kChainSize - 1);
            while (wrappedPos < 0.0f) wrappedPos += chainEnd;
            while (wrappedPos >= chainEnd) wrappedPos -= chainEnd;

            // Hann window weighting for smooth rolloff
            float t = static_cast<float> (s) / static_cast<float> (numSamples - 1);
            float weight = 0.5f - 0.5f * std::cos (6.28318530718f * t);

            float val = cubicInterpChain (snapA, snapB, interpFrac, wrappedPos);
            sum += val * weight;
            weightSum += weight;
        }

        return (weightSum > 0.0001f) ? (sum / weightSum) : 0.0f;
    }

    //==========================================================================
    // Cubic Hermite interpolation for chain readout.
    // Reads from two snapshots (A, B) blended by interpFrac.
    //==========================================================================

    static float cubicInterpChain (const float* snapA, const float* snapB,
                                   float interpFrac, float pos) noexcept
    {
        int N = kChainSize;
        int i1 = static_cast<int> (pos);
        float frac = pos - static_cast<float> (i1);

        // Clamp indices
        if (i1 < 0) i1 = 0;
        if (i1 >= N) i1 = N - 1;

        int i0 = (i1 > 0) ? i1 - 1 : 0;
        int i2 = (i1 < N - 1) ? i1 + 1 : N - 1;
        int i3 = (i2 < N - 1) ? i2 + 1 : N - 1;

        // Interpolate between snapshots A and B for each point
        float y0 = snapA[i0] + interpFrac * (snapB[i0] - snapA[i0]);
        float y1 = snapA[i1] + interpFrac * (snapB[i1] - snapA[i1]);
        float y2 = snapA[i2] + interpFrac * (snapB[i2] - snapA[i2]);
        float y3 = snapA[i3] + interpFrac * (snapB[i3] - snapA[i3]);

        // Cubic Hermite interpolation
        float c0 = y1;
        float c1 = 0.5f * (y2 - y0);
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    //==========================================================================
    // MIDI note handling.
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int midiChannel, int maxPoly,
                 bool monoMode, bool legatoMode, float glideCoeff,
                 float ampA, float ampD, float ampS, float ampR,
                 float physA, float physD, float physS, float physR,
                 float lfo1Rate, float lfo1Depth, int lfo1Shape,
                 float lfo2Rate, float lfo2Depth, int lfo2Shape,
                 int initShapeIdx, float excitePos, float exciteWidth)
    {
        float freq = midiToHz (static_cast<float> (noteNumber));

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFreq = freq;

            if (legatoMode && wasActive)
            {
                // Legato: don't retrigger envelopes, just glide
                voice.glideCoeff = glideCoeff;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;

                // Initialize MPE expression for this voice's channel
                voice.mpeExpression.reset();
                voice.mpeExpression.midiChannel = midiChannel;
                if (mpeManager != nullptr)
                    mpeManager->updateVoiceExpression(voice.mpeExpression);

                // Re-excite chain gently on legato retrigger
                voice.applyImpulse (excitePos, exciteWidth, 0.05f);
            }
            else
            {
                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.startTime = voiceCounter++;
                voice.currentFreq = freq;
                voice.glideCoeff = glideCoeff;
                voice.scanPhase = 0.0f;
                voice.controlPhaseAcc = 0.0f;
                voice.fadingOut = false;
                voice.fadeGain = 1.0f;

                // Initialize MPE expression for this voice's channel
                voice.mpeExpression.reset();
                voice.mpeExpression.midiChannel = midiChannel;
                if (mpeManager != nullptr)
                    mpeManager->updateVoiceExpression(voice.mpeExpression);

                voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.physEnv.setParams (physA, physD, physS, physR, srf);
                voice.physEnv.noteOn();

                voice.lfo1.setRate (lfo1Rate, srf);
                voice.lfo1.setShape (lfo1Shape);
                voice.lfo2.setRate (lfo2Rate, srf);
                voice.lfo2.setShape (lfo2Shape);

                voice.dcBlockL.setCoefficient (srf);
                voice.dcBlockR.setCoefficient (srf);
                voice.dcBlockL.reset();
                voice.dcBlockR.reset();

                // Initialize chain shape and apply initial excitation
                voice.rngState = static_cast<uint32_t> (noteNumber * 7919 + 42);
                voice.initChainShape (initShapeIdx, excitePos, exciteWidth);
            }
            return;
        }

        // Polyphonic mode
        int idx = findFreeVoice (maxPoly);
        auto& voice = voices[static_cast<size_t> (idx)];

        // If stealing, initiate crossfade
        if (voice.active)
        {
            voice.fadingOut = true;
            voice.fadeGain = std::min (voice.fadeGain, 0.5f);
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceCounter++;
        voice.currentFreq = freq;
        voice.targetFreq = freq;
        voice.glideCoeff = 1.0f;
        voice.scanPhase = 0.0f;
        voice.controlPhaseAcc = 0.0f;
        voice.fadingOut = false;
        voice.fadeGain = 1.0f;

        // Initialize MPE expression for this voice's channel
        voice.mpeExpression.reset();
        voice.mpeExpression.midiChannel = midiChannel;
        if (mpeManager != nullptr)
            mpeManager->updateVoiceExpression(voice.mpeExpression);

        voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();
        voice.physEnv.setParams (physA, physD, physS, physR, srf);
        voice.physEnv.noteOn();

        voice.lfo1.setRate (lfo1Rate, srf);
        voice.lfo1.setShape (lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate (lfo2Rate, srf);
        voice.lfo2.setShape (lfo2Shape);
        voice.lfo2.reset();

        voice.dcBlockL.setCoefficient (srf);
        voice.dcBlockR.setCoefficient (srf);
        voice.dcBlockL.reset();
        voice.dcBlockR.reset();

        // Initialize chain
        voice.rngState = static_cast<uint32_t> (noteNumber * 7919 + voiceCounter);
        voice.initChainShape (initShapeIdx, excitePos, exciteWidth);
    }

    void noteOff (int noteNumber, int midiChannel = 0)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                // In MPE mode, match by channel too
                if (midiChannel > 0 && voice.mpeExpression.midiChannel > 0
                    && voice.mpeExpression.midiChannel != midiChannel)
                    continue;

                voice.ampEnv.noteOff();
                voice.physEnv.noteOff();
            }
        }
    }

    int findFreeVoice (int maxPoly) const
    {
        int poly = std::min (maxPoly, kMaxVoices);

        // Find inactive voice within polyphony limit
        for (int i = 0; i < poly; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU voice stealing — find oldest voice
        int oldest = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < poly; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t> (i)].startTime;
                oldest = i;
            }
        }
        return oldest;
    }

    static float midiToHz (float midiNote) noexcept
    {
        return 440.0f * std::pow (2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //==========================================================================
    // Member data
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    float smoothCoeff = 0.1f;
    float crossfadeRate = 0.01f;
    float controlStepSamples = 11.025f;  // ~44100 / 4000
    float dt2 = 1.0f;

    std::array<ObscuraVoice, kMaxVoices> voices;
    uint64_t voiceCounter = 0;
    int activeVoices = 0;

    // Smoothed control parameters
    float smoothedStiffness = 0.5f;
    float smoothedDamping = 0.3f;
    float smoothedNonlinear = 0.0f;
    float smoothedScanWidth = 16.0f;
    float smoothedSustainForce = 0.0f;

    // Coupling accumulators
    float envelopeOutput = 0.0f;
    float couplingForceMod = 0.0f;
    float couplingStiffMod = 0.0f;
    float couplingImpulseTrigger = 0.0f;

    // Output cache for coupling reads
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;

    // Cached APVTS parameter pointers
    std::atomic<float>* pStiffness = nullptr;
    std::atomic<float>* pDamping = nullptr;
    std::atomic<float>* pNonlinear = nullptr;
    std::atomic<float>* pExcitePos = nullptr;
    std::atomic<float>* pExciteWidth = nullptr;
    std::atomic<float>* pScanWidth = nullptr;
    std::atomic<float>* pBoundary = nullptr;
    std::atomic<float>* pSustain = nullptr;
    std::atomic<float>* pLevel = nullptr;

    std::atomic<float>* pAmpAttack = nullptr;
    std::atomic<float>* pAmpDecay = nullptr;
    std::atomic<float>* pAmpSustain = nullptr;
    std::atomic<float>* pAmpRelease = nullptr;

    std::atomic<float>* pPhysAttack = nullptr;
    std::atomic<float>* pPhysDecay = nullptr;
    std::atomic<float>* pPhysSustain = nullptr;
    std::atomic<float>* pPhysRelease = nullptr;

    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;

    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide = nullptr;
    std::atomic<float>* pInitShape = nullptr;

    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement = nullptr;
    std::atomic<float>* pMacroCoupling = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;
};

} // namespace xomnibus
