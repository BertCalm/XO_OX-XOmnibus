// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/ModMatrix.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
//
//  O B S E R V A N D U M   E N G I N E
//  Polychromatic Phase Synthesis
//
//  XO_OX Aquatic Identity: Cuttlefish — Epipelagic zone. Chromatophore-rich
//  skin shifts colour and pattern in real time, each "facet" of its skin a
//  different iridescent frequency. "Every frequency, a different colour."
//  Accent: Teal #4ECDC4.
//
//  Architecture: Variable 2-8 Phase Distortion oscillators ("facets") per voice,
//  interpolating between 8 mathematically-derived distortion curves in
//  transfer-function space. Environmental input bus (coupling / sidechain /
//  parametric model) warps the active curve in real time. 2x oversampling for
//  anti-aliasing. 3 ADSRs: amp, filter, distortion amount.
//
//  Signal Flow:
//    MIDI Note → N facet oscillators (each at unique detune + phase offset)
//      → Transfer-function lookup (active curve = lerp between 8 basis curves)
//      → Environmental curve modifier (coupling / sidechain / parametric)
//      → 2x oversampling + simple averaging downsample
//      → 1/N normalization
//      → Distortion amount envelope blend (pure ↔ distorted)
//      → Per-voice CytomicSVF filter (L + R independent, fixes XObsidian bug)
//      → Amplitude envelope × velocity × crossfade gain
//      → Stereo output
//
//  Coupling (XOceanus inter-engine modulation):
//    Output: post-filter stereo audio via getSampleForCoupling (ch0=L, ch1=R)
//            envelope follower on ch2
//    Input:  AudioToFM / PhaseDeflection → modulates phase spread between facets
//            AmpToFilter → modulates filter cutoff
//            EnvToMorph  → modulates curve morph position
//
//  NOTE: CouplingType::PhaseDeflection does not exist in the current enum;
//  AudioToFM is used as a stand-in. When PhaseDeflection is added to
//  Source/Core/SynthEngine.h, update applyCouplingInput() to also match it.
//
//==============================================================================

// ---- Engine-level constants ----
static constexpr int  kObservMaxFacets      = 8;
static constexpr int  kObservOscBudget      = 24;
static constexpr int  kObservCurveSize      = 1024;  // points per distortion curve
static constexpr int  kObservNumCurves      = 8;
static constexpr int  kObservOversample     = 2;
static constexpr float kObservPI            = 3.14159265358979323846f;
static constexpr float kObservTwoPi         = 6.28318530717958647692f;

// Cosine LUT size — power of two for bitmask-wrap
static constexpr int  kObservCosineLUTSize  = 4096;

//==============================================================================
//  Inline ADSR for Observandum
//
//  We use StandardADSR from DSP/StandardADSR.h rather than re-implementing.
//  Three instances per voice: amp, filter, distortion.
//==============================================================================

//==============================================================================
//  ObservandumVoice — Per-Voice State
//==============================================================================
struct ObservandumVoice
{
    bool     active       = false;
    bool     isFadingOut  = false;
    int      midiNote     = -1;
    float    velocity     = 0.0f;
    float    frequency    = 440.0f;
    float    targetFrequency = 440.0f;
    float    currentFrequency = 440.0f;   // portamento-glided frequency
    float    glideCoefficient = 1.0f;     // 1.0 = instant snap
    uint64_t startTime    = 0;            // monotonic counter for LRU stealing

    // Double-precision phase accumulators for each facet (avoids phase drift)
    double   phase[kObservMaxFacets] = {};

    // Voice crossfade (5 ms linear ramp on voice steal)
    float    crossfadeGain = 1.0f;

    // ---- Envelopes ----
    // Amp envelope — controls output amplitude
    StandardADSR ampEnv;
    // Filter envelope — bipolar cutoff modulation (Hz)
    StandardADSR filterEnv;
    // Distortion envelope — additive distortion amount
    StandardADSR distEnv;

    // ---- Per-voice filters: L + R independent (fixes XObsidian one-filter bug) ----
    CytomicSVF filterL;
    CytomicSVF filterR;

    // ---- Per-voice LFOs (D002/D005) ----
    StandardLFO lfo1;
    StandardLFO lfo2;

    void reset() noexcept
    {
        active          = false;
        isFadingOut     = false;
        midiNote        = -1;
        velocity        = 0.0f;
        frequency       = 440.0f;
        targetFrequency = 440.0f;
        currentFrequency = 440.0f;
        glideCoefficient = 1.0f;
        startTime       = 0;
        crossfadeGain   = 1.0f;
        for (int f = 0; f < kObservMaxFacets; ++f)
            phase[f] = 0.0;
        ampEnv.kill();
        filterEnv.kill();
        distEnv.kill();
        filterL.reset();
        filterR.reset();
        lfo1.reset();
        lfo2.reset();
    }
};

//==============================================================================
//
//  ObservandumEngine
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention — the .cpp is a one-line stub.
//
//  Parameter prefix: observ_
//  Gallery accent:   Teal #4ECDC4
//
//==============================================================================
class ObservandumEngine : public SynthEngine
{
public:
    // Maximum voice count scales with facet count: budget / max(2, facets).
    // At 2 facets → 12 voices. At 8 facets → 3 voices.
    static constexpr int kAbsoluteMaxVoices = kObservOscBudget / 2; // = 12

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateDouble = sampleRate;
        sampleRateFloat  = static_cast<float>(sampleRate);

        // 5 ms smoothing coefficient (zipper noise prevention)
        paramSmoothCoeff = 1.0f - std::exp(-kObservTwoPi * (1.0f / 0.005f) / sampleRateFloat);

        // 5 ms voice crossfade rate (linear ramp)
        voiceFadeRate = 1.0f / (0.005f * sampleRateFloat);

        // Output cache for coupling reads
        outputCacheLeft.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheRight.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        envSignalBuffer.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        // Build the 8 distortion curves
        buildDistortionCurves();

        // Build cosine LUT
        for (int i = 0; i < kObservCosineLUTSize; ++i)
        {
            float normPhase = static_cast<float>(i) / static_cast<float>(kObservCosineLUTSize);
            cosineLUT[static_cast<size_t>(i)] = std::cos(kObservTwoPi * normPhase);
        }

        // Initialise env follower filter coefficients
        envFollowerAttackCoeff  = std::exp(-1.0f / (0.001f * sampleRateFloat)); // 1 ms attack
        envFollowerReleaseCoeff = std::exp(-1.0f / (0.050f * sampleRateFloat)); // 50 ms release

        // Silence gate: 200 ms hold (standard pad engine)
        prepareSilenceGate(sampleRate, maxBlockSize, 200.0f);

        // Reset all voices
        for (auto& voice : voices)
            voice.reset();

        currentFacetCount = 2;

        blockLfo1.reset();
        blockLfo2.reset();
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
            voice.reset();

        couplingPhaseDeflectionMod = 0.0f;
        couplingFilterMod          = 0.0f;
        couplingMorphMod           = 0.0f;
        envFollower                = 0.0f;
        envelopeFollowerOutput     = 0.0f;
        smoothedMorph              = 0.0f;
        smoothedDetune             = 0.0f;
        smoothedSpread             = 0.0f;
        smoothedDistortion         = 0.0f;
        smoothedCutoff             = 8000.0f;
        modWheelValue              = 0.0f;
        pitchBendNorm              = 0.0f;

        std::fill(outputCacheLeft.begin(),  outputCacheLeft.end(),  0.0f);
        std::fill(outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  A U D I O
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;
        if (sampleRateFloat <= 0.0f)
            return;

        // ---- ParamSnapshot: read all parameter values once per block ----
        const float paramFacetCount     = loadParam(pFacetCount,     2.0f);
        const float paramCurveMorph     = loadParam(pCurveMorph,     0.0f);   // [0, 100]
        const float paramDetune         = loadParam(pDetune,          0.0f);   // cents spread
        const float paramPhaseSpread    = loadParam(pPhaseSpread,     0.0f);   // degrees [0,180]
        const float paramDistortion     = loadParam(pDistortionAmount,0.5f);   // [0,1]
        const float paramFilterCutoff   = loadParam(pFilterCutoff,    8000.0f);
        const float paramFilterReso     = loadParam(pFilterReso,      0.0f);
        const int   paramFilterType     = static_cast<int>(loadParam(pFilterType, 0.0f)); // 0=LP,1=HP,2=BP
        const float paramMasterLevel    = loadParam(pMasterLevel,     0.8f);
        const float paramStereoWidth    = loadParam(pStereoWidth,     0.3f);
        const float paramEnvDepth       = loadParam(pEnvDepth,        0.0f);   // env bus depth [0,1]
        const int   paramEnvMode        = static_cast<int>(loadParam(pEnvMode, 0.0f));  // 0=coupling,1=sidechain,2=parametric
        const float paramEnvRate        = loadParam(pEnvRate,         0.5f);   // parametric model rate Hz
        const int   paramEnvModel       = static_cast<int>(loadParam(pEnvModel, 0.0f)); // 0=wave,1=turbulence,2=tidal,3=drift

        // Amplitude envelope ADSR
        const float paramAmpA  = loadParam(pAmpAttack,   0.01f);
        const float paramAmpD  = loadParam(pAmpDecay,    0.1f);
        const float paramAmpS  = loadParam(pAmpSustain,  0.8f);
        const float paramAmpR  = loadParam(pAmpRelease,  0.3f);

        // Filter envelope ADSR
        const float paramFltA  = loadParam(pFltAttack,   0.01f);
        const float paramFltD  = loadParam(pFltDecay,    0.2f);
        const float paramFltS  = loadParam(pFltSustain,  0.0f);
        const float paramFltR  = loadParam(pFltRelease,  0.3f);
        const float paramFltEnvAmt = loadParam(pFltEnvAmount, 0.0f);  // bipolar Hz amount [−8000,8000]

        // Distortion envelope ADSR
        const float paramDistA = loadParam(pDistAttack,  0.005f);
        const float paramDistD = loadParam(pDistDecay,   0.3f);
        const float paramDistS = loadParam(pDistSustain, 0.5f);
        const float paramDistR = loadParam(pDistRelease, 0.3f);
        const float paramDistEnvAmt = loadParam(pDistEnvAmount, 0.0f); // [0,1]

        // LFO 1
        const float paramLfo1Rate  = loadParam(pLfo1Rate,  1.0f);
        const float paramLfo1Depth = loadParam(pLfo1Depth, 0.0f);
        const int   paramLfo1Shape = static_cast<int>(loadParam(pLfo1Shape, 0.0f));

        // LFO 2
        const float paramLfo2Rate  = loadParam(pLfo2Rate,  0.3f);
        const float paramLfo2Depth = loadParam(pLfo2Depth, 0.0f);
        const int   paramLfo2Shape = static_cast<int>(loadParam(pLfo2Shape, 0.0f));

        // Morph env amount (D002)
        const float paramMorphEnvAmt = loadParam(pMorphEnvAmount, 0.0f);

        // Voice config
        const int   paramVoiceMode    = static_cast<int>(loadParam(pVoiceMode,  2.0f));
        const float paramGlideTime    = loadParam(pGlideTime, 0.0f);

        // 4 macros
        const float macroChar  = loadParam(pMacro1, 0.0f);  // CHARACTER
        const float macroMove  = loadParam(pMacro2, 0.0f);  // MOVEMENT
        const float macroCoup  = loadParam(pMacro3, 0.0f);  // COUPLING
        const float macroSpace = loadParam(pMacro4, 0.0f);  // SPACE

        // ---- Determine effective facet count (clamped to 2-8) ----
        currentFacetCount = std::max(2, std::min(kObservMaxFacets, static_cast<int>(paramFacetCount + 0.5f)));
        const int numFacets = currentFacetCount;

        // ---- Determine polyphony (voice budget / facets) ----
        const int voiceBudgetMax = std::max(1, kObservOscBudget / numFacets);
        int maxPolyphony = voiceBudgetMax;
        bool monoMode = false, legatoMode = false;
        switch (paramVoiceMode)
        {
            case 0: maxPolyphony = 1; monoMode = true;  break;             // Mono
            case 1: maxPolyphony = 1; monoMode = true; legatoMode = true; break; // Legato
            case 2: maxPolyphony = std::min(voiceBudgetMax, 8);  break;    // Poly8
            case 3: maxPolyphony = std::min(voiceBudgetMax, 12); break;    // Poly12
            default: maxPolyphony = std::min(voiceBudgetMax, 8); break;
        }

        // ---- Glide coefficient ----
        float glideCoeff = 1.0f;
        if (paramGlideTime > 0.001f)
            glideCoeff = 1.0f - std::exp(-1.0f / (paramGlideTime * sampleRateFloat));

        // ---- Macro offsets ----
        // CHARACTER: +distortion, +curveMorph toward high values
        float effectiveMorph      = paramCurveMorph + macroChar * 30.0f;
        float effectiveDistortion = paramDistortion  + macroChar * 0.3f;
        // MOVEMENT: +envDepth, future env rate modulation
        float effectiveEnvDepth   = paramEnvDepth   + macroMove * 0.4f;
        // COUPLING: +phaseSpread, +envDepth from coupling
        float effectivePhaseSpread = paramPhaseSpread + macroCoup * 60.0f;
        // SPACE: +stereoWidth, +detune, +oscillators (handled via facet count parameter)
        float effectiveStereoWidth = paramStereoWidth + macroSpace * 0.3f;
        float effectiveDetune      = paramDetune      + macroSpace * 20.0f;

        // Clamp all effective values
        effectiveMorph        = clamp(effectiveMorph,        0.0f,  100.0f);
        effectiveDistortion   = clamp(effectiveDistortion,   0.0f,  1.0f);
        effectiveEnvDepth     = clamp(effectiveEnvDepth,     0.0f,  1.0f);
        effectivePhaseSpread  = clamp(effectivePhaseSpread,  0.0f,  180.0f);
        effectiveStereoWidth  = clamp(effectiveStereoWidth,  0.0f,  1.0f);
        effectiveDetune       = clamp(effectiveDetune,       0.0f,  100.0f); // cents max

        // ---- Apply phase deflection coupling to phase spread ----
        float deflectedSpread = effectivePhaseSpread * (1.0f + couplingPhaseDeflectionMod);
        deflectedSpread = clamp(deflectedSpread, 0.0f, 360.0f);

        // ---- Morph coupling ----
        float morphWithCoupling = clamp(effectiveMorph + couplingMorphMod * 100.0f, 0.0f, 100.0f);

        // ---- Filter cutoff with coupling ----
        float effectiveCutoff = clamp(
            paramFilterCutoff + couplingFilterMod * 4000.0f + modWheelValue * 10000.0f,
            20.0f, 20000.0f);

        // Save coupling filter mod before zeroing so per-voice cutoff calculation can use it
        const float savedCouplingFilterMod = couplingFilterMod;

        // Coupling accumulators consumed — zero for next block.
        // Assumes applyCouplingInput() was called before renderBlock() by MegaCouplingMatrix.
        couplingPhaseDeflectionMod = 0.0f;
        couplingFilterMod          = 0.0f;
        couplingMorphMod           = 0.0f;

        // Curve morph is now per-voice (D002): see voiceMorph / vCurveA / vCurveB in voice loop

        // ---- Environmental curve modifier (per-block signal generation) ----
        // envSignalBuffer holds one sample per output sample (Mode 2)
        // or is filled from env follower (Mode 1) or coupling (Mode 0)
        if (static_cast<int>(envSignalBuffer.size()) < numSamples)
            envSignalBuffer.assign(static_cast<size_t>(numSamples), 0.0f);

        if (paramEnvMode == 1)
        {
            // Sidechain follower — we track the left input channel if present
            // (In JUCE plugin context the sidechain bus would be separate;
            // here we run the follower on our own output cache as a self-envelope)
            for (int i = 0; i < numSamples; ++i)
            {
                float inputSig = std::fabs(outputCacheLeft.size() > static_cast<size_t>(i)
                                           ? outputCacheLeft[static_cast<size_t>(i)] : 0.0f);
                float coeff = (inputSig > envFollower) ? envFollowerAttackCoeff : envFollowerReleaseCoeff;
                envFollower = envFollower * coeff + inputSig * (1.0f - coeff);
                envFollower = flushDenormal(envFollower);
                envSignalBuffer[static_cast<size_t>(i)] = envFollower * 2.0f - 1.0f; // map [0,1] → [-1,1]
            }
        }
        else if (paramEnvMode == 2)
        {
            // Parametric models — generate per-block
            for (int i = 0; i < numSamples; ++i)
            {
                double t = parametricModelPhase;
                float sig = 0.0f;
                switch (paramEnvModel)
                {
                    case 0: // Wave: sin
                        sig = std::sin(kObservTwoPi * static_cast<float>(t));
                        break;
                    case 1: // Turbulence: smoothed noise
                    {
                        // Simple LCG pseudo-noise with exponential smoothing
                        turbulenceSeed = turbulenceSeed * 6364136223846793005ULL + 1442695040888963407ULL;
                        float raw = static_cast<float>(static_cast<int64_t>(turbulenceSeed >> 33)) / static_cast<float>(0x7FFFFFFF);
                        turbulenceSmoothed = turbulenceSmoothed * 0.95f + raw * 0.05f;
                        sig = flushDenormal(turbulenceSmoothed);
                        break;
                    }
                    case 2: // Tidal: superposition of 3 sines
                        sig = std::sin(kObservTwoPi * static_cast<float>(t))
                            + 0.5f * std::sin(1.7f * kObservTwoPi * static_cast<float>(t))
                            + 0.3f * std::sin(2.9f * kObservTwoPi * static_cast<float>(t));
                        sig *= (1.0f / 1.8f); // normalise
                        break;
                    case 3: // Drift: Brownian motion
                    {
                        turbulenceSeed = turbulenceSeed * 6364136223846793005ULL + 1442695040888963407ULL;
                        float step = static_cast<float>(static_cast<int64_t>(turbulenceSeed >> 33))
                                     / static_cast<float>(0x7FFFFFFF) * 0.002f;
                        driftAccum  = clamp(driftAccum + step, -1.0f, 1.0f);
                        driftSmooth = driftSmooth * 0.998f + driftAccum * 0.002f;
                        sig = flushDenormal(driftSmooth);
                        break;
                    }
                    default:
                        sig = std::sin(kObservTwoPi * static_cast<float>(t));
                        break;
                }
                envSignalBuffer[static_cast<size_t>(i)] = sig;
                parametricModelPhase += static_cast<double>(paramEnvRate) / static_cast<double>(sampleRateFloat);
                if (parametricModelPhase >= 1.0)
                    parametricModelPhase -= 1.0;
            }
        }
        else
        {
            // Mode 0: coupling — use the coupling curve modifier accumulator
            float coupVal = couplingCurveModAccum;
            for (int i = 0; i < numSamples; ++i)
                envSignalBuffer[static_cast<size_t>(i)] = coupVal;
        }
        couplingCurveModAccum = 0.0f;

        // ---- D002 mod matrix — apply per-block ----
        // Block-rate LFOs are ticked here once per block for mod matrix use.
        // Per-voice LFOs still provide sample-rate modulation on their hardcoded routes.
        // Destinations: 0=Off, 1=FilterCutoff, 2=MorphPosition, 3=Pitch, 4=AmpLevel, 5=Distortion
        blockLfo1.setRate(paramLfo1Rate, sampleRateFloat);
        blockLfo1.setShape(paramLfo1Shape);
        blockLfo2.setRate(paramLfo2Rate, sampleRateFloat);
        blockLfo2.setShape(paramLfo2Shape);
        float blockLfo1Val = blockLfo1.process() * paramLfo1Depth;
        float blockLfo2Val = blockLfo2.process() * paramLfo2Depth;

        float modCutoffOffset = 0.0f, modMorphOffset = 0.0f, modPitchOffset = 0.0f;
        float modAmpOffset = 0.0f, modDistOffset = 0.0f;
        {
            ModMatrix<4>::Sources mSrc;
            mSrc.lfo1       = blockLfo1Val;
            mSrc.lfo2       = blockLfo2Val;
            mSrc.env        = 0.0f;
            mSrc.velocity   = lastNoteVelocity;
            mSrc.keyTrack   = lastNoteKeyTrack;
            mSrc.modWheel   = modWheelValue;
            mSrc.aftertouch = aftertouchValue;
            float mDst[6]   = {};
            modMatrix.apply(mSrc, mDst);
            modCutoffOffset = mDst[1] * 5000.0f;
            modMorphOffset  = mDst[2] * 50.0f;
            modPitchOffset  = mDst[3] * 12.0f;
            modAmpOffset    = mDst[4] * 0.5f;
            modDistOffset   = mDst[5] * 0.3f;
        }
        effectiveCutoff = clamp(effectiveCutoff + modCutoffOffset, 20.0f, 20000.0f);
        morphWithCoupling = clamp(morphWithCoupling + modMorphOffset, 0.0f, 100.0f);

        // ---- Compute per-facet detune ratios and phase offsets ----
        // Detune: spread across facets symmetrically in cents (smoothed)
        // Phase offset: spread across facets by smoothedSpread (degrees, smoothed)
        float detuneRatio[kObservMaxFacets];
        float phaseOffset[kObservMaxFacets];
        {
            float detuneHalfRangeCents = (numFacets > 1) ? smoothedDetune * 0.5f : 0.0f;
            float spreadDeg = (numFacets > 1) ? smoothedSpread : 0.0f;
            for (int f = 0; f < numFacets; ++f)
            {
                float t = (numFacets > 1) ? static_cast<float>(f) / static_cast<float>(numFacets - 1) : 0.5f;
                // t in [0,1] → detune offset in [-half, +half] cents
                float centOffset = (t - 0.5f) * 2.0f * detuneHalfRangeCents;
                detuneRatio[f] = std::pow(2.0f, centOffset / 1200.0f);
                // Phase offset in [0, spreadDeg/360] cycle fraction
                float spreadFrac = spreadDeg / 360.0f;
                phaseOffset[f] = (numFacets > 1) ? t * spreadFrac : 0.0f;
            }
        }

        // Stereo panning: alternate facets L/R with blend
        // Facets 0,2,4,6 → left; 1,3,5,7 → right (modified by stereoWidth)
        float facetPanL[kObservMaxFacets], facetPanR[kObservMaxFacets];
        for (int f = 0; f < numFacets; ++f)
        {
            float panPos = (numFacets > 1) ? static_cast<float>(f) / static_cast<float>(numFacets - 1) : 0.5f;
            // panPos 0→1 = full left → full right, lerped with stereoWidth
            float panR = 0.5f + (panPos - 0.5f) * effectiveStereoWidth;
            float panL = 1.0f - panR;
            facetPanL[f] = panL;
            facetPanR[f] = panR;
        }

        // ---- Process MIDI ----
        bool hasMidiNoteOn = false;
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                hasMidiNoteOn = true;
                wakeSilenceGate();
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(),
                       maxPolyphony, monoMode, legatoMode, glideCoeff,
                       paramAmpA, paramAmpD, paramAmpS, paramAmpR,
                       paramFltA, paramFltD, paramFltS, paramFltR,
                       paramDistA, paramDistD, paramDistS, paramDistR,
                       paramLfo1Rate, paramLfo1Shape,
                       paramLfo2Rate, paramLfo2Shape,
                       effectiveCutoff, paramFilterReso, paramFilterType);
            }
            else if (msg.isNoteOff())
            {
                noteOff(msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
            }
            else if (msg.isChannelPressure())
            {
                // D006: aftertouch → distortion amount
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                // D006: CC1 mod wheel → filter cutoff brightening
                modWheelValue = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                // ±2 semitone pitch bend
                int raw = msg.getPitchWheelValue(); // 0-16383, center=8192
                pitchBendNorm = static_cast<float>(raw - 8192) / 8192.0f; // [-1,+1]
            }
        }

        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // Pitch bend ratio (±2 semitones + mod matrix pitch offset in semitones)
        float pitchBendRatio = std::pow(2.0f, (pitchBendNorm * 2.0f + modPitchOffset) / 12.0f);

        // D006: aftertouch → distortion boost
        float atDistBoost = aftertouchValue * 0.2f;

        // ---- Per-sample render loop ----
        float peakEnvLevel = 0.0f;

        for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
        {
            // Smooth control parameters (5 ms)
            smoothedMorph      += (morphWithCoupling   - smoothedMorph)      * paramSmoothCoeff;
            smoothedDistortion += (effectiveDistortion  - smoothedDistortion) * paramSmoothCoeff;
            smoothedCutoff     += (effectiveCutoff      - smoothedCutoff)     * paramSmoothCoeff;
            smoothedDetune     += (effectiveDetune      - smoothedDetune)     * paramSmoothCoeff;
            smoothedSpread     += (deflectedSpread      - smoothedSpread)     * paramSmoothCoeff;
            smoothedMorph      = flushDenormal(smoothedMorph);
            smoothedDistortion = flushDenormal(smoothedDistortion);
            smoothedCutoff     = flushDenormal(smoothedCutoff);
            smoothedDetune     = flushDenormal(smoothedDetune);
            smoothedSpread     = flushDenormal(smoothedSpread);

            // Envelope curve warp: read per-sample from env signal buffer
            float envCurveWarpAmt = envSignalBuffer[static_cast<size_t>(sampleIdx)]
                                  * effectiveEnvDepth * 0.3f;

            float mixLeft = 0.0f, mixRight = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // Voice stealing crossfade
                if (voice.isFadingOut)
                {
                    voice.crossfadeGain -= voiceFadeRate;
                    voice.crossfadeGain  = flushDenormal(voice.crossfadeGain);
                    if (voice.crossfadeGain <= 0.0f)
                    {
                        voice.crossfadeGain = 0.0f;
                        voice.active        = false;
                        continue;
                    }
                }

                // Portamento (exponential glide toward target)
                voice.currentFrequency += (voice.targetFrequency - voice.currentFrequency)
                                          * voice.glideCoefficient;
                voice.currentFrequency = flushDenormal(voice.currentFrequency);

                // Base frequency with pitch bend
                float freq = voice.currentFrequency * pitchBendRatio;

                // ---- Tick envelopes ----
                float ampLevel  = voice.ampEnv.process();
                float fltLevel  = voice.filterEnv.process();
                float distLevel = voice.distEnv.process();

                if (!voice.ampEnv.isActive() && !voice.isFadingOut)
                {
                    voice.active = false;
                    continue;
                }

                // ---- Per-voice LFO tick (D002/D005) ----
                float lfo1Output = flushDenormal(voice.lfo1.process() * paramLfo1Depth);
                float lfo2Output = flushDenormal(voice.lfo2.process() * paramLfo2Depth);

                // ---- D001: Velocity → distortion timbre ----
                float effectiveDistAmt = clamp(smoothedDistortion
                    + voice.velocity * 0.15f
                    + distLevel * paramDistEnvAmt
                    + atDistBoost
                    + modDistOffset, 0.0f, 1.0f);

                // ---- Morph env modulation (D002) ----
                // distLevel reused as morph env signal scaled by morphEnvAmount
                // LFO1 routes to morph position
                float morphEnvOffset = distLevel * paramMorphEnvAmt * 50.0f; // max ±50 morph units
                // Per-voice effective morph: smoothed base + per-voice envelope offset + LFO1
                float voiceMorph = clamp(smoothedMorph + morphEnvOffset + lfo1Output * 50.0f, 0.0f, 100.0f);
                float vMorphIdx = voiceMorph * static_cast<float>(kObservNumCurves - 1) / 100.0f;
                int vCurveA = std::min(static_cast<int>(vMorphIdx), kObservNumCurves - 2);
                int vCurveB = vCurveA + 1;
                float vMorphT = vMorphIdx - static_cast<float>(vCurveA);

                // Effective filter cutoff for this voice: base + filter env (velocity-scaled, D001) + coupling + LFO2
                float voiceCutoff = clamp(smoothedCutoff
                    + fltLevel * paramFltEnvAmt * voice.velocity
                    + savedCouplingFilterMod * 4000.0f
                    + lfo2Output * 4000.0f, 20.0f, 20000.0f);

                // Update per-voice filter coefficients every 16 samples (sub-block rate).
                // This gives 16x better filter cutoff tracking than block-rate at a fraction
                // of the cost of per-sample coefficient computation.
                if ((sampleIdx & 15) == 0)
                {
                    CytomicSVF::Mode fMode = CytomicSVF::Mode::LowPass;
                    switch (paramFilterType)
                    {
                        case 1: fMode = CytomicSVF::Mode::HighPass; break;
                        case 2: fMode = CytomicSVF::Mode::BandPass; break;
                        default: fMode = CytomicSVF::Mode::LowPass; break;
                    }
                    voice.filterL.setMode(fMode);
                    voice.filterR.setMode(fMode);
                    voice.filterL.setCoefficients(voiceCutoff, paramFilterReso, sampleRateFloat);
                    voice.filterR.setCoefficients(voiceCutoff, paramFilterReso, sampleRateFloat);
                }

                // ---- Render all facets with 2x oversampling ----
                float facetOutL = 0.0f, facetOutR = 0.0f;

                for (int f = 0; f < numFacets; ++f)
                {
                    float facetFreq = freq * detuneRatio[f];
                    // Phase increment at 2x sample rate
                    double phaseInc = static_cast<double>(facetFreq)
                                    / (sampleRateDouble * static_cast<double>(kObservOversample));

                    float osOut[kObservOversample];
                    for (int os = 0; os < kObservOversample; ++os)
                    {
                        double ph = voice.phase[f];

                        // Lookup distorted phase via per-voice curve morph (D002)
                        float fIdx = static_cast<float>(ph) * static_cast<float>(kObservCurveSize - 1);
                        int idx0   = static_cast<int>(fIdx);
                        int idx1   = std::min(idx0 + 1, kObservCurveSize - 1);
                        float frac = fIdx - static_cast<float>(idx0);
                        float dA = distortionCurves[vCurveA][idx0]
                                 + (distortionCurves[vCurveA][idx1] - distortionCurves[vCurveA][idx0]) * frac;
                        float dB = distortionCurves[vCurveB][idx0]
                                 + (distortionCurves[vCurveB][idx1] - distortionCurves[vCurveB][idx0]) * frac;
                        float distortedPhase = dA + (dB - dA) * vMorphT;

                        // Envelope curve warp (sin approximation via cosine LUT)
                        distortedPhase += envCurveWarpAmt * lookupCosine(static_cast<float>(ph) - 0.25f);

                        // Blend linear phase with distorted phase by effectiveDistAmt
                        float finalPhase = static_cast<float>(ph)
                                         + (distortedPhase - static_cast<float>(ph)) * effectiveDistAmt;

                        // Apply facet phase offset
                        float phWithOffset = finalPhase + phaseOffset[f];
                        // Wrap to [0,1)
                        phWithOffset -= std::floor(phWithOffset);

                        // Cosine lookup
                        float sample = lookupCosine(phWithOffset);
                        osOut[os] = sample;

                        // Advance phase
                        voice.phase[f] += phaseInc;
                        if (voice.phase[f] >= 1.0)
                            voice.phase[f] -= 1.0;
                    }

                    // Downsample: average of 2x oversampled samples
                    float facetSample = 0.0f;
                    for (int os = 0; os < kObservOversample; ++os)
                        facetSample += osOut[os];
                    facetSample /= static_cast<float>(kObservOversample);

                    // Accumulate with stereo panning per facet
                    facetOutL += facetSample * facetPanL[f];
                    facetOutR += facetSample * facetPanR[f];
                }

                // 1/N normalization
                float normFactor = 1.0f / static_cast<float>(numFacets);
                facetOutL *= normFactor;
                facetOutR *= normFactor;

                // ---- Per-voice filters (L and R independent — fixes XObsidian bug) ----
                facetOutL = voice.filterL.processSample(facetOutL);
                facetOutR = voice.filterR.processSample(facetOutR);

                // ---- Amplitude shaping ----
                float voiceGain = ampLevel * voice.velocity * voice.crossfadeGain * clamp(1.0f + modAmpOffset, 0.0f, 2.0f);
                facetOutL *= voiceGain;
                facetOutR *= voiceGain;

                facetOutL = flushDenormal(facetOutL);
                facetOutR = flushDenormal(facetOutR);

                mixLeft  += facetOutL;
                mixRight += facetOutR;

                peakEnvLevel = std::max(peakEnvLevel, ampLevel);
            }

            // Apply master level
            float finalL = mixLeft  * paramMasterLevel;
            float finalR = mixRight * paramMasterLevel;

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sampleIdx, finalL);
                buffer.addSample(1, sampleIdx, finalR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sampleIdx, (finalL + finalR) * 0.5f);
            }

            // Cache output for coupling
            if (sampleIdx < static_cast<int>(outputCacheLeft.size()))
            {
                outputCacheLeft[static_cast<size_t>(sampleIdx)]  = finalL;
                outputCacheRight[static_cast<size_t>(sampleIdx)] = finalR;
            }
        }

        envelopeFollowerOutput = peakEnvLevel;

        // Update active voice count (atomic for UI thread)
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active)
                ++count;
        activeVoiceCount_.store(count, std::memory_order_relaxed);

        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  C O U P L I N G
    //==========================================================================

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto idx = static_cast<size_t>(sampleIndex);
        if (channel == 0 && idx < outputCacheLeft.size())
            return outputCacheLeft[idx];
        if (channel == 1 && idx < outputCacheRight.size())
            return outputCacheRight[idx];
        if (channel == 2)
            return envelopeFollowerOutput;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        // PhaseDeflection is not yet in the CouplingType enum —
        // using AudioToFM as a stand-in. When PhaseDeflection is added to
        // SynthEngine.h, add: case CouplingType::PhaseDeflection: (same block)
        if (type == CouplingType::AudioToFM)
        {
            // Phase deflection: modulates phase spread between oscillators
            for (int i = 0; i < numSamples; ++i)
            {
                float modSignal = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                couplingPhaseDeflectionMod += (modSignal - couplingPhaseDeflectionMod) * 0.01f;
            }
        }

        if (type == CouplingType::AmpToFilter)
        {
            // Amplitude-driven filter cutoff modulation
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                couplingFilterMod += (s - couplingFilterMod) * 0.01f;
            }
        }

        if (type == CouplingType::EnvToMorph)
        {
            // Envelope drives morph position
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount * 0.3f;
                couplingMorphMod += (s - couplingMorphMod) * 0.01f;
            }
        }

        if (type == CouplingType::RhythmToBlend)
        {
            // Rhythmic source warps env curve modifier
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount * 0.3f;
                couplingCurveModAccum += (s - couplingCurveModAccum) * 0.01f;
            }
        }
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  P A R A M E T E R S
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // ---- Oscillator / Facet parameters ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_facetCount", 1}, "Observandum Facet Count",
            juce::NormalisableRange<float>(2.0f, 8.0f, 1.0f), 2.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_curveMorph", 1}, "Observandum Curve Morph",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_detune", 1}, "Observandum Facet Detune",
            juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f));  // cents

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_phaseSpread", 1}, "Observandum Phase Spread",
            juce::NormalisableRange<float>(0.0f, 180.0f, 0.1f), 0.0f));  // degrees

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_distortionAmount", 1}, "Observandum Distortion",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        // ---- Filter ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_filterCutoff", 1}, "Observandum Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_filterReso", 1}, "Observandum Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"observ_filterType", 1}, "Observandum Filter Type",
            juce::StringArray{"LowPass", "HighPass", "BandPass"}, 0));

        // ---- Stereo / Level ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_stereoWidth", 1}, "Observandum Stereo Width",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_level", 1}, "Observandum Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        // ---- Amplitude Envelope ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_ampAttack", 1}, "Observandum Amp Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_ampDecay", 1}, "Observandum Amp Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_ampSustain", 1}, "Observandum Amp Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_ampRelease", 1}, "Observandum Amp Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // ---- Filter Envelope ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_fltAttack", 1}, "Observandum Filter Env Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_fltDecay", 1}, "Observandum Filter Env Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_fltSustain", 1}, "Observandum Filter Env Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_fltRelease", 1}, "Observandum Filter Env Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_fltEnvAmount", 1}, "Observandum Filter Env Amount",
            juce::NormalisableRange<float>(-8000.0f, 8000.0f, 1.0f), 0.0f));

        // ---- Distortion Envelope ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_distAttack", 1}, "Observandum Dist Env Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.005f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_distDecay", 1}, "Observandum Dist Env Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_distSustain", 1}, "Observandum Dist Env Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_distRelease", 1}, "Observandum Dist Env Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_distEnvAmount", 1}, "Observandum Dist Env Amount",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_morphEnvAmount", 1}, "Observandum Morph Env Amount",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f), 0.0f));

        // ---- Environmental Input Bus ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_envDepth", 1}, "Observandum Env Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"observ_envMode", 1}, "Observandum Env Mode",
            juce::StringArray{"Coupling", "Sidechain", "Parametric"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_envRate", 1}, "Observandum Env Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.3f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"observ_envModel", 1}, "Observandum Env Model",
            juce::StringArray{"Wave", "Turbulence", "Tidal", "Drift"}, 0));

        // ---- LFO 1 ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_lfo1Rate", 1}, "Observandum LFO1 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_lfo1Depth", 1}, "Observandum LFO1 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"observ_lfo1Shape", 1}, "Observandum LFO1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // ---- LFO 2 ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_lfo2Rate", 1}, "Observandum LFO2 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_lfo2Depth", 1}, "Observandum LFO2 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"observ_lfo2Shape", 1}, "Observandum LFO2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // ---- Voice Configuration ----

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"observ_voiceMode", 1}, "Observandum Voice Mode",
            juce::StringArray{"Mono", "Legato", "Poly8", "Poly12"}, 2));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_glide", 1}, "Observandum Glide",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // ---- XOceanus Standard Macros ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_macro1", 1}, "Observandum Macro CHARACTER",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_macro2", 1}, "Observandum Macro MOVEMENT",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_macro3", 1}, "Observandum Macro COUPLING",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"observ_macro4", 1}, "Observandum Macro SPACE",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // D002 mod matrix — 4-slot configurable source→destination routing
        static const juce::StringArray kObservModDests {
            "Off", "Filter Cutoff", "Morph Position", "Pitch", "Amp Level", "Distortion"
        };
        ModMatrix<4>::addParameters(params, "observ_", "Observandum", kObservModDests);
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pFacetCount        = apvts.getRawParameterValue("observ_facetCount");
        pCurveMorph        = apvts.getRawParameterValue("observ_curveMorph");
        pDetune            = apvts.getRawParameterValue("observ_detune");
        pPhaseSpread       = apvts.getRawParameterValue("observ_phaseSpread");
        pDistortionAmount  = apvts.getRawParameterValue("observ_distortionAmount");

        pFilterCutoff      = apvts.getRawParameterValue("observ_filterCutoff");
        pFilterReso        = apvts.getRawParameterValue("observ_filterReso");
        pFilterType        = apvts.getRawParameterValue("observ_filterType");

        pStereoWidth       = apvts.getRawParameterValue("observ_stereoWidth");
        pMasterLevel       = apvts.getRawParameterValue("observ_level");

        pAmpAttack         = apvts.getRawParameterValue("observ_ampAttack");
        pAmpDecay          = apvts.getRawParameterValue("observ_ampDecay");
        pAmpSustain        = apvts.getRawParameterValue("observ_ampSustain");
        pAmpRelease        = apvts.getRawParameterValue("observ_ampRelease");

        pFltAttack         = apvts.getRawParameterValue("observ_fltAttack");
        pFltDecay          = apvts.getRawParameterValue("observ_fltDecay");
        pFltSustain        = apvts.getRawParameterValue("observ_fltSustain");
        pFltRelease        = apvts.getRawParameterValue("observ_fltRelease");
        pFltEnvAmount      = apvts.getRawParameterValue("observ_fltEnvAmount");

        pDistAttack        = apvts.getRawParameterValue("observ_distAttack");
        pDistDecay         = apvts.getRawParameterValue("observ_distDecay");
        pDistSustain       = apvts.getRawParameterValue("observ_distSustain");
        pDistRelease       = apvts.getRawParameterValue("observ_distRelease");
        pDistEnvAmount     = apvts.getRawParameterValue("observ_distEnvAmount");
        pMorphEnvAmount    = apvts.getRawParameterValue("observ_morphEnvAmount");

        pEnvDepth          = apvts.getRawParameterValue("observ_envDepth");
        pEnvMode           = apvts.getRawParameterValue("observ_envMode");
        pEnvRate           = apvts.getRawParameterValue("observ_envRate");
        pEnvModel          = apvts.getRawParameterValue("observ_envModel");

        pLfo1Rate          = apvts.getRawParameterValue("observ_lfo1Rate");
        pLfo1Depth         = apvts.getRawParameterValue("observ_lfo1Depth");
        pLfo1Shape         = apvts.getRawParameterValue("observ_lfo1Shape");
        pLfo2Rate          = apvts.getRawParameterValue("observ_lfo2Rate");
        pLfo2Depth         = apvts.getRawParameterValue("observ_lfo2Depth");
        pLfo2Shape         = apvts.getRawParameterValue("observ_lfo2Shape");

        pVoiceMode         = apvts.getRawParameterValue("observ_voiceMode");
        pGlideTime         = apvts.getRawParameterValue("observ_glide");

        pMacro1            = apvts.getRawParameterValue("observ_macro1");
        pMacro2            = apvts.getRawParameterValue("observ_macro2");
        pMacro3            = apvts.getRawParameterValue("observ_macro3");
        pMacro4            = apvts.getRawParameterValue("observ_macro4");

        modMatrix.attachParameters(apvts, "observ_");
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl(params);
        return juce::AudioProcessorValueTreeState::ParameterLayout(
            std::make_move_iterator(params.begin()),
            std::make_move_iterator(params.end()));
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  I D E N T I T Y
    //==========================================================================

    juce::String getEngineId() const override { return "Observandum"; }

    juce::Colour getAccentColour() const override { return juce::Colour(0xFF4ECDC4); }

    int getMaxVoices() const override
    {
        return std::max(1, kObservOscBudget / std::max(2, currentFacetCount));
    }

    int getActiveVoiceCount() const override
    {
        return activeVoiceCount_.load(std::memory_order_relaxed);
    }

private:
    //==========================================================================
    //  Helper: Safe Atomic Parameter Load
    //==========================================================================

    static float loadParam(const std::atomic<float>* ptr, float fallback) noexcept
    {
        return (ptr != nullptr) ? ptr->load(std::memory_order_relaxed) : fallback;
    }

    static float clamp(float v, float lo, float hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    //==========================================================================
    //  Cosine LUT Lookup with Linear Interpolation
    //==========================================================================

    float lookupCosine(float phase) const noexcept
    {
        // Wrap phase to [0, 1)
        phase = phase - std::floor(phase);
        float floatIdx = phase * static_cast<float>(kObservCosineLUTSize);
        int idx0 = static_cast<int>(floatIdx) & (kObservCosineLUTSize - 1);
        int idx1 = (idx0 + 1) & (kObservCosineLUTSize - 1);
        float frac = floatIdx - std::floor(floatIdx);
        return cosineLUT[static_cast<size_t>(idx0)]
             + frac * (cosineLUT[static_cast<size_t>(idx1)] - cosineLUT[static_cast<size_t>(idx0)]);
    }

    //==========================================================================
    //  Monotonicity Enforcement
    //
    //  After generating or interpolating any curve, call this to guarantee
    //  that the curve is non-decreasing and ends at 1.0. This ensures the
    //  phase distortion function remains a valid monotonic mapping, which
    //  prevents aliasing from phase reversals.
    //==========================================================================

    static void enforceMonotonicity(float* curve, int size) noexcept
    {
        for (int i = 1; i < size; ++i)
            curve[i] = std::max(curve[i], curve[i - 1]);
        // Normalize endpoint to 1.0
        if (curve[size - 1] > 0.0f)
        {
            float scale = 1.0f / curve[size - 1];
            for (int i = 0; i < size; ++i)
                curve[i] *= scale;
        }
        // Force exact endpoints
        curve[0]        = 0.0f;
        curve[size - 1] = 1.0f;
    }

    //==========================================================================
    //  Build the 8 Distortion Curves
    //
    //  All curves map [0,1] → [0,1] monotonically increasing.
    //  Generated once in prepare(); shared across all voices.
    //
    //  Index 0: PURE    — linear (pure cosine output)
    //  Index 1: RAMP    — pow(φ, 0.6) gentle sawtooth approximation
    //  Index 2: EDGE    — φ + 0.3*sin(2πφ)/(2π) resonant bump, normalised
    //  Index 3: NEEDLE  — pow(φ, 3.0) narrow pulse at end of cycle
    //  Index 4: ARCH    — 0.5*(1-cos(πφ)) smooth arch (half-sine rectification)
    //  Index 5: FOLD    — 0.5*(1-cos(2πφ))/2 + φ*0.5 double-frequency blend
    //  Index 6: RING    — acos(T4(cos(2πφ)))/(2π) 4th Chebyshev (exact 4th harmonic)
    //  Index 7: FRACTURE — φ + 0.4*J1(6πφ)/(6π) Bessel-FM-like sidebands
    //==========================================================================

    void buildDistortionCurves()
    {
        const int N = kObservCurveSize;

        // ---- Curve 0: PURE — linear phase (outputs pure cosine) ----
        for (int i = 0; i < N; ++i)
            distortionCurves[0][i] = static_cast<float>(i) / static_cast<float>(N - 1);
        enforceMonotonicity(distortionCurves[0], N);

        // ---- Curve 1: RAMP — gentle sawtooth via power curve ----
        // D(φ) = pow(φ, 0.6) — fast rise, slow tail → sawtooth-like harmonic content
        for (int i = 0; i < N; ++i)
        {
            float phi = static_cast<float>(i) / static_cast<float>(N - 1);
            distortionCurves[1][i] = std::pow(phi, 0.6f);
        }
        enforceMonotonicity(distortionCurves[1], N);

        // ---- Curve 2: EDGE — resonant bump ----
        // D(φ) = φ + 0.3*sin(2πφ)/(2π)
        // The additive sine bumps the phase in the middle of the cycle,
        // creating a resonant peak in the harmonic spectrum.
        for (int i = 0; i < N; ++i)
        {
            float phi = static_cast<float>(i) / static_cast<float>(N - 1);
            distortionCurves[2][i] = phi + 0.3f * std::sin(kObservTwoPi * phi) / kObservTwoPi;
        }
        enforceMonotonicity(distortionCurves[2], N);

        // ---- Curve 3: NEEDLE — narrow pulse at end of cycle ----
        // D(φ) = pow(φ, 3.0) — very slow start, explosive finish
        // Creates narrow cosine pulse → high harmonic content (pulse wave character)
        for (int i = 0; i < N; ++i)
        {
            float phi = static_cast<float>(i) / static_cast<float>(N - 1);
            distortionCurves[3][i] = phi * phi * phi;
        }
        enforceMonotonicity(distortionCurves[3], N);

        // ---- Curve 4: ARCH — smooth arch (half-sine rectification) ----
        // D(φ) = 0.5*(1-cos(πφ))
        // Gentle, symmetric arch — produces 2nd harmonic emphasis, warm tone
        for (int i = 0; i < N; ++i)
        {
            float phi = static_cast<float>(i) / static_cast<float>(N - 1);
            distortionCurves[4][i] = 0.5f * (1.0f - std::cos(kObservPI * phi));
        }
        enforceMonotonicity(distortionCurves[4], N);

        // ---- Curve 5: FOLD — double-frequency blend ----
        // D(φ) = 0.5*(1-cos(2πφ))/2 + φ*0.5
        // Half of a full-frequency arch plus linear blend → octave content
        for (int i = 0; i < N; ++i)
        {
            float phi = static_cast<float>(i) / static_cast<float>(N - 1);
            float foldPart = 0.5f * (1.0f - std::cos(kObservTwoPi * phi)) * 0.5f;
            float linPart  = phi * 0.5f;
            distortionCurves[5][i] = foldPart + linPart;
        }
        enforceMonotonicity(distortionCurves[5], N);

        // ---- Curve 6: RING — 4th order Chebyshev ----
        // Constructed so that cos(2π·D(φ)) ≈ T_4(cos(2πφ)), giving exact 4th harmonic.
        // T_4(x) = 8x^4 - 8x^2 + 1
        // D(φ) = acos(clamp(T_4(cos(2πφ)), -1, 1)) / (2π)
        // Monotonicity must be enforced after computation.
        for (int i = 0; i < N; ++i)
        {
            float phi = static_cast<float>(i) / static_cast<float>(N - 1);
            float x   = std::cos(kObservTwoPi * phi);
            // Chebyshev T_4(x) = 8x^4 - 8x^2 + 1
            float x2  = x * x;
            float T4  = 8.0f * x2 * x2 - 8.0f * x2 + 1.0f;
            T4        = clamp(T4, -1.0f, 1.0f);
            float d   = std::acos(T4) / kObservTwoPi;
            distortionCurves[6][i] = clamp(d, 0.0f, 1.0f);
        }
        enforceMonotonicity(distortionCurves[6], N);

        // ---- Curve 7: FRACTURE — Bessel FM-like sidebands ----
        // D(φ) = φ + 0.4*J1(6πφ)/(6π)
        // where J1(x) ≈ x/2 - x³/16 + x⁵/384 (polynomial approximation for |x| ≤ 6π)
        // Produces FM-like sideband structure through the phase distortion mechanism.
        for (int i = 0; i < N; ++i)
        {
            float phi = static_cast<float>(i) / static_cast<float>(N - 1);
            float arg = 6.0f * kObservPI * phi;
            // J1 polynomial: x/2 - x^3/16 + x^5/384
            float arg2 = arg * arg;
            float j1   = arg * 0.5f - (arg * arg2) / 16.0f + (arg * arg2 * arg2) / 384.0f;
            distortionCurves[7][i] = phi + 0.4f * j1 / (6.0f * kObservPI);
        }
        enforceMonotonicity(distortionCurves[7], N);
    }

    //==========================================================================
    //  MIDI Note Handling
    //==========================================================================

    static float midiNoteToHz(float midiNote) noexcept
    {
        return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
    }

    void noteOn(int noteNum, float velocity,
                int maxPolyphony, bool monoMode, bool legatoMode, float glideCoeff,
                float ampA, float ampD, float ampS, float ampR,
                float fltA, float fltD, float fltS, float fltR,
                float distA, float distD, float distS, float distR,
                float lfo1Rate, int lfo1Shape,
                float lfo2Rate, int lfo2Shape,
                float cutoff, float reso, int filterType)
    {
        float freq = midiNoteToHz(static_cast<float>(noteNum));

        // Track last note state for block-rate mod matrix sources
        lastNoteVelocity = velocity;
        lastNoteKeyTrack = (static_cast<float>(noteNum) - 60.0f) / 60.0f;

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;
            voice.targetFrequency = freq;

            if (legatoMode && wasActive)
            {
                // Legato: glide without envelope retrigger
                voice.glideCoefficient = glideCoeff;
                voice.midiNote = noteNum;
                voice.velocity = velocity;
            }
            else
            {
                voice.active            = true;
                voice.midiNote          = noteNum;
                voice.velocity          = velocity;
                voice.startTime         = voiceTimestamp++;
                voice.currentFrequency  = freq;
                voice.glideCoefficient  = glideCoeff;
                voice.isFadingOut       = false;
                voice.crossfadeGain     = 1.0f;
                for (int f = 0; f < kObservMaxFacets; ++f)
                    voice.phase[f] = 0.0;
                initVoiceEnvelopes(voice, ampA, ampD, ampS, ampR,
                                   fltA, fltD, fltS, fltR,
                                   distA, distD, distS, distR);
                initVoiceFilters(voice, cutoff, reso, filterType);
                voice.lfo1.setRate(lfo1Rate, sampleRateFloat);
                voice.lfo1.setShape(lfo1Shape);
                voice.lfo1.reset();
                voice.lfo2.setRate(lfo2Rate, sampleRateFloat);
                voice.lfo2.setShape(lfo2Shape);
                voice.lfo2.reset();
            }
            return;
        }

        // ---- Polyphonic mode ----
        // Use LRU voice allocation; steal quietest if all busy
        int voiceIdx = findVoiceForNoteOn(maxPolyphony);
        auto& voice  = voices[static_cast<size_t>(voiceIdx)];

        // If stealing an active voice, crossfade to prevent click
        if (voice.active)
        {
            voice.isFadingOut  = true;
            voice.crossfadeGain = std::min(voice.crossfadeGain, 0.5f);
        }

        voice.active            = true;
        voice.midiNote          = noteNum;
        voice.velocity          = velocity;
        voice.startTime         = voiceTimestamp++;
        voice.currentFrequency  = freq;
        voice.targetFrequency   = freq;
        voice.glideCoefficient  = 1.0f; // No glide in poly mode
        voice.isFadingOut       = false;
        voice.crossfadeGain     = 1.0f;
        for (int f = 0; f < kObservMaxFacets; ++f)
            voice.phase[f] = 0.0;

        initVoiceEnvelopes(voice, ampA, ampD, ampS, ampR,
                           fltA, fltD, fltS, fltR,
                           distA, distD, distS, distR);
        initVoiceFilters(voice, cutoff, reso, filterType);
        voice.lfo1.setRate(lfo1Rate, sampleRateFloat);
        voice.lfo1.setShape(lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate(lfo2Rate, sampleRateFloat);
        voice.lfo2.setShape(lfo2Shape);
        voice.lfo2.reset();
    }

    void noteOff(int noteNum)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.midiNote == noteNum && !voice.isFadingOut)
            {
                voice.ampEnv.noteOff();
                voice.filterEnv.noteOff();
                voice.distEnv.noteOff();
            }
        }
    }

    // Find the best voice slot for a new note.
    // Pass 1: find inactive slot.
    // Pass 2: steal the quietest (smallest amp envelope level) active voice.
    int findVoiceForNoteOn(int maxPoly) noexcept
    {
        const int limit = std::min(maxPoly, static_cast<int>(voices.size()));
        // Pass 1: inactive
        for (int i = 0; i < limit; ++i)
            if (!voices[static_cast<size_t>(i)].active)
                return i;
        // Pass 2: steal quietest (smallest crossfadeGain * velocity proxy)
        // We use LRU (oldest startTime) as the Obsidian pattern does
        int    oldest = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < limit; ++i)
        {
            if (voices[static_cast<size_t>(i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t>(i)].startTime;
                oldest = i;
            }
        }
        return oldest;
    }

    void initVoiceEnvelopes(ObservandumVoice& voice,
                            float ampA, float ampD, float ampS, float ampR,
                            float fltA, float fltD, float fltS, float fltR,
                            float distA, float distD, float distS, float distR) noexcept
    {
        voice.ampEnv.setParams(ampA, ampD, ampS, ampR, sampleRateFloat);
        voice.ampEnv.noteOn();

        voice.filterEnv.setParams(fltA, fltD, fltS, fltR, sampleRateFloat);
        voice.filterEnv.noteOn();

        voice.distEnv.setParams(distA, distD, distS, distR, sampleRateFloat);
        voice.distEnv.noteOn();
    }

    void initVoiceFilters(ObservandumVoice& voice, float cutoff, float reso, int filterType) noexcept
    {
        CytomicSVF::Mode fMode = CytomicSVF::Mode::LowPass;
        switch (filterType)
        {
            case 1: fMode = CytomicSVF::Mode::HighPass; break;
            case 2: fMode = CytomicSVF::Mode::BandPass; break;
            default: fMode = CytomicSVF::Mode::LowPass; break;
        }
        voice.filterL.reset();
        voice.filterL.setMode(fMode);
        voice.filterL.setCoefficients(cutoff, reso, sampleRateFloat);

        voice.filterR.reset();
        voice.filterR.setMode(fMode);
        voice.filterR.setCoefficients(cutoff, reso, sampleRateFloat);
    }

    //==========================================================================
    //  M E M B E R   D A T A
    //==========================================================================

    // ---- Audio configuration (set in prepare()) ----
    double sampleRateDouble   = 44100.0;
    float  sampleRateFloat    = 44100.0f;
    float  paramSmoothCoeff   = 0.1f;
    float  voiceFadeRate      = 0.01f;

    // ---- Facet count (updated per block from parameter) ----
    int    currentFacetCount  = 2;

    // ---- Voice pool ----
    std::array<ObservandumVoice, kAbsoluteMaxVoices> voices;
    uint64_t voiceTimestamp   = 0;

    // ---- Smoothed control parameters ----
    float smoothedMorph       = 0.0f;
    float smoothedDistortion  = 0.5f;
    float smoothedCutoff      = 8000.0f;
    float smoothedDetune      = 0.0f;
    float smoothedSpread      = 0.0f;

    // ---- D006: expression ----
    float modWheelValue       = 0.0f;
    float aftertouchValue     = 0.0f;
    float pitchBendNorm       = 0.0f;

    // ---- Coupling accumulators (reset each block) ----
    float couplingPhaseDeflectionMod = 0.0f;
    float couplingFilterMod          = 0.0f;
    float couplingMorphMod           = 0.0f;
    float couplingCurveModAccum      = 0.0f;

    // ---- Envelope follower (Mode 1: sidechain) ----
    float envFollower                = 0.0f;
    float envFollowerAttackCoeff     = 0.999f;
    float envFollowerReleaseCoeff    = 0.999f;
    float envelopeFollowerOutput     = 0.0f;

    // ---- Parametric model state (Mode 2) ----
    double   parametricModelPhase   = 0.0;
    uint64_t turbulenceSeed         = 12345678901234567ULL;
    float    turbulenceSmoothed     = 0.0f;
    float    driftAccum             = 0.0f;
    float    driftSmooth            = 0.0f;

    // ---- Output cache for coupling reads ----
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    // ---- Environmental signal buffer (replaces stack array — safe for any block size) ----
    std::vector<float> envSignalBuffer;

    // ---- 8 Distortion Curves: [curve_index][phase_sample] ----
    // Built once in prepare(). Each curve maps phase [0,1] → [0,1] monotonically.
    float distortionCurves[kObservNumCurves][kObservCurveSize] = {};

    // ---- Cosine LUT for fast oscillator evaluation ----
    std::array<float, kObservCosineLUTSize> cosineLUT{};

    // ---- D002 Mod Matrix ----
    ModMatrix<4> modMatrix;

    // ---- Block-rate LFOs for mod matrix sources ----
    // Ticked once per block before modMatrix.apply(); per-voice LFOs still
    // provide sample-rate modulation on their hardcoded routes.
    StandardLFO blockLfo1;
    StandardLFO blockLfo2;

    // ---- Last noteOn state for block-rate mod matrix sources ----
    float lastNoteVelocity = 0.0f;
    float lastNoteKeyTrack = 0.0f; // (midiNote - 60) / 60, normalised bipolar

    // ---- Cached APVTS parameter pointers (ParamSnapshot pattern) ----
    // Oscillator
    std::atomic<float>* pFacetCount       = nullptr;
    std::atomic<float>* pCurveMorph       = nullptr;
    std::atomic<float>* pDetune           = nullptr;
    std::atomic<float>* pPhaseSpread      = nullptr;
    std::atomic<float>* pDistortionAmount = nullptr;
    // Filter
    std::atomic<float>* pFilterCutoff     = nullptr;
    std::atomic<float>* pFilterReso       = nullptr;
    std::atomic<float>* pFilterType       = nullptr;
    // Level / stereo
    std::atomic<float>* pStereoWidth      = nullptr;
    std::atomic<float>* pMasterLevel      = nullptr;
    // Amp env
    std::atomic<float>* pAmpAttack        = nullptr;
    std::atomic<float>* pAmpDecay         = nullptr;
    std::atomic<float>* pAmpSustain       = nullptr;
    std::atomic<float>* pAmpRelease       = nullptr;
    // Filter env
    std::atomic<float>* pFltAttack        = nullptr;
    std::atomic<float>* pFltDecay         = nullptr;
    std::atomic<float>* pFltSustain       = nullptr;
    std::atomic<float>* pFltRelease       = nullptr;
    std::atomic<float>* pFltEnvAmount     = nullptr;
    // Distortion env
    std::atomic<float>* pDistAttack       = nullptr;
    std::atomic<float>* pDistDecay        = nullptr;
    std::atomic<float>* pDistSustain      = nullptr;
    std::atomic<float>* pDistRelease      = nullptr;
    std::atomic<float>* pDistEnvAmount    = nullptr;
    std::atomic<float>* pMorphEnvAmount   = nullptr;
    // Env bus
    std::atomic<float>* pEnvDepth         = nullptr;
    std::atomic<float>* pEnvMode          = nullptr;
    std::atomic<float>* pEnvRate          = nullptr;
    std::atomic<float>* pEnvModel         = nullptr;
    // LFOs
    std::atomic<float>* pLfo1Rate         = nullptr;
    std::atomic<float>* pLfo1Depth        = nullptr;
    std::atomic<float>* pLfo1Shape        = nullptr;
    std::atomic<float>* pLfo2Rate         = nullptr;
    std::atomic<float>* pLfo2Depth        = nullptr;
    std::atomic<float>* pLfo2Shape        = nullptr;
    // Voice config
    std::atomic<float>* pVoiceMode        = nullptr;
    std::atomic<float>* pGlideTime        = nullptr;
    // Macros
    std::atomic<float>* pMacro1           = nullptr;
    std::atomic<float>* pMacro2           = nullptr;
    std::atomic<float>* pMacro3           = nullptr;
    std::atomic<float>* pMacro4           = nullptr;
};

} // namespace xoceanus
