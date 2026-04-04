// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/ModMatrix.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
//
//  O B S I D I A N   E N G I N E
//  Crystalline Phase Distortion Synthesis
//
//  XO_OX Aquatic Identity: Volcanic glass — formed under impossible pressure
//  at the ocean floor, razor-sharp and beautiful. Fourth-generation species
//  in the XO_OX ecosystem. Lives in The Deep, Oscar-leaning — deep character
//  with occasional surface flash. Accent: Crystal White #E8E0D8.
//
//  Synth Lineage: Resurrects Casio's CZ-series (1984) phase distortion method
//  — where the timbre IS the waveform shape, not a filter applied after.
//  The CZ-101 was the affordable alternative to the DX7, using phase distortion
//  instead of FM to create complex timbres from simple cosine oscillators.
//  Obsidian evolves this 40-year-old technique with capabilities Casio never
//  imagined: a 2D morphable distortion space (density x tilt), two-stage
//  cascade with cross-modulation, Euler-Bernoulli inharmonic stiffness for
//  bell/metallic tones, 4-formant resonance network, and stereo phase
//  divergence from a single oscillator core.
//
//  How Phase Distortion works:
//  A standard cosine oscillator produces a pure tone. Phase distortion warps
//  the phase accumulator's progress through each cycle — speeding up in some
//  regions, slowing in others — which reshapes the waveform and creates
//  harmonics WITHOUT any filter or FM modulation. The brilliance of the CZ
//  approach is that a single parameter (distortion depth) smoothly sweeps
//  from pure cosine to complex timbres, much like a filter sweep but with
//  a fundamentally different harmonic character.
//
//  Signal Flow:
//    MIDI Note -> Phase Accumulator
//      -> 2D Phase Distortion LUT [density x tilt] (Stage 1 L/R)
//      -> Stiffness Engine (Euler-Bernoulli inharmonic partials)
//      -> PD Stage 2 Cascade (cross-modulated by Stage 1 output)
//      -> 4-Formant Bandpass Resonance Network
//      -> Cytomic SVF Main Filter (lowpass)
//      -> Amplitude Envelope * Velocity * Crossfade
//      -> Stereo Voice Sum -> Master Level -> Output
//
//  Coupling (XOceanus inter-engine modulation):
//    Output: post-filter stereo audio via getSampleForCoupling (ch0=L, ch1=R)
//            envelope follower on ch2 for amplitude-driven coupling
//    Input:  AudioToFM    -> modulates PD depth (external audio drives timbre)
//            AmpToFilter  -> modulates filter cutoff (rhythmic filtering)
//            EnvToMorph   -> modulates density/tilt position (timbral morphing)
//            RhythmToBlend -> modulates PD depth (rhythmic timbral sync)
//
//  Recommended couplings (from sound design guide):
//    + Oblong:  Obsidian's crystal purity + Bob's amber curiosity
//    + Origami: PD harmonics -> spectral folding = complex harmonic evolution
//    + Opal:    Obsidian tones frozen and granulated by Opal
//    + Orbital: Phase distortion + additive partials = Crystal Palace quad
//
//==============================================================================

//==============================================================================
//  Phase Distortion Lookup Table Dimensions
//
//  The LUT stores pre-computed distortion functions across a 2D grid:
//    Density (X axis): 32 steps — controls harmonic complexity / inflection count
//    Tilt    (Y axis): 32 steps — controls spectral energy distribution
//    Phase:            512 steps — one full cycle of the distortion function
//
//  Total memory: 32 * 32 * 512 * sizeof(float) = 2,097,152 bytes (~2 MB)
//  This table is shared across all voices — built once in prepare().
//==============================================================================
static constexpr int kLutDensitySteps = 32;
static constexpr int kLutTiltSteps = 32;
static constexpr int kLutPhaseSteps = 512;

// ObsidianADSR and ObsidianLFO have been replaced by StandardADSR and StandardLFO
// from Source/DSP/. Use xoceanus::StandardADSR and xoceanus::StandardLFO directly.

//==============================================================================
//  ObsidianVoice — Per-Voice State
//
//  Each voice holds its own phase accumulators (for two PD stages), envelopes,
//  LFOs, formant filter bank, and main output filter. The stiffness partial
//  ratios are cached per-voice to avoid recomputation each sample.
//
//  Voice stealing uses a 5ms linear crossfade (fadeGain ramps to zero) to
//  prevent clicks when polyphony is exhausted.
//==============================================================================
struct ObsidianVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0; // Monotonic counter for LRU voice stealing

    // ---- Phase Accumulators ----
    float phaseStage1 = 0.0f; // Stage 1 master phase [0, 1)
    float phaseStage2 = 0.0f; // Stage 2 cascade phase [0, 1)

    // ---- Portamento / Glide ----
    float currentFrequency = 440.0f; // Current (gliding) frequency in Hz
    float targetFrequency = 440.0f;  // Destination frequency in Hz
    float glideCoefficient = 1.0f;   // 1.0 = instant (no glide)

    // ---- Envelopes ----
    StandardADSR amplitudeEnvelope;
    StandardADSR phaseDistortionEnvelope;

    // ---- LFOs (per-voice for free-running phase independence) ----
    StandardLFO lfo1;
    StandardLFO lfo2;

    // ---- 4-Band Formant Resonance Network ----
    // Four bandpass filters at fixed musical frequencies create vowel-like
    // resonance when blended with the dry PD signal.
    CytomicSVF formantFilters[4];

    // ---- Main Output Filter ----
    // Cytomic SVF lowpass — the final timbral shaping stage before amplitude.
    CytomicSVF mainFilter;

    // ---- Voice Stealing Crossfade ----
    float crossfadeGain = 1.0f; // Ramps from 1.0 to 0.0 during steal
    bool isFadingOut = false;

    // ---- Stiffness Partial Ratios ----
    // Cached Euler-Bernoulli stretched partial frequencies (first 16 partials).
    // In a real stiff string/bar, partial N has frequency f_n = N * f0 * sqrt(1 + B*N^2),
    // where B is the stiffness coefficient. This stretches upper partials sharp,
    // creating the characteristic bell/piano/metallic inharmonicity.
    float partialRatios[16] = {};

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        phaseStage1 = 0.0f;
        phaseStage2 = 0.0f;
        currentFrequency = 440.0f;
        targetFrequency = 440.0f;
        crossfadeGain = 1.0f;
        isFadingOut = false;
        amplitudeEnvelope.reset();
        phaseDistortionEnvelope.reset();
        lfo1.reset();
        lfo2.reset();
        for (auto& filter : formantFilters)
            filter.reset();
        mainFilter.reset();
        for (int i = 0; i < 16; ++i)
            partialRatios[i] = static_cast<float>(i + 1);
    }
};

//==============================================================================
//
//  ObsidianEngine — The Main Engine Class
//
//  Implements the SynthEngine interface for XOceanus integration.
//  All DSP is inline per project convention — the .cpp file is a stub.
//
//  Parameter prefix: obsidian_
//  Gallery accent: Crystal White #E8E0D8
//
//==============================================================================
class ObsidianEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateDouble = sampleRate;
        sampleRateFloat = static_cast<float>(sampleRateDouble);

        // Smoothing coefficient for control-rate parameter interpolation.
        // Time constant of 5ms prevents zipper noise on knob movements.
        // Formula: coeff = 1 - e^(-2*PI * cutoffHz / sampleRate), where
        // cutoffHz = 1/0.005 = 200 Hz gives a 5ms settling time.
        paramSmoothingCoefficient = 1.0f - std::exp(-kTwoPi * (1.0f / 0.005f) / sampleRateFloat);

        // Voice-stealing crossfade rate: linear ramp over 5ms.
        // At 44100 Hz, this is ~220 samples — fast enough to be inaudible,
        // slow enough to avoid click artifacts from abrupt amplitude change.
        voiceCrossfadeRate = 1.0f / (0.005f * sampleRateFloat);

        // Allocate output cache for coupling reads (other engines can read our output)
        outputCacheLeft.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheRight.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        aftertouch.prepare(sampleRate);

        silenceGate.prepare(sampleRate, maxBlockSize);

        // Build the 2D phase distortion lookup table (~2 MB, one-time cost)
        buildDistortionLUT();

        // Build the cosine lookup table for fast oscillator evaluation.
        // 4096 entries gives <0.01% interpolation error — inaudible.
        for (int i = 0; i < kCosineLUTSize; ++i)
        {
            float normalizedPhase = static_cast<float>(i) / static_cast<float>(kCosineLUTSize);
            cosineLUT[static_cast<size_t>(i)] = std::cos(kTwoPi * normalizedPhase);
        }

        // Initialize all voices and their filter banks
        for (auto& voice : voices)
        {
            voice.reset();
            for (int band = 0; band < 4; ++band)
            {
                voice.formantFilters[band].reset();
                voice.formantFilters[band].setMode(CytomicSVF::Mode::BandPass);
            }
            voice.mainFilter.reset();
            voice.mainFilter.setMode(CytomicSVF::Mode::LowPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& voice : voices)
            voice.reset();

        envelopeFollowerOutput = 0.0f;
        couplingPhaseDistortionDepthMod = 0.0f;
        couplingFilterCutoffMod = 0.0f;
        couplingDensityMod = 0.0f;
        couplingTiltMod = 0.0f;

        smoothedDensity = 0.0f;
        smoothedTilt = 0.0f;
        smoothedDepth = 0.0f;

        // D005: reset the engine-level formant LFO phase
        obsidianLfoPhase = 0.0;

        std::fill(outputCacheLeft.begin(), outputCacheLeft.end(), 0.0f);
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

        // ---- ParamSnapshot: read all parameter values once per block ----
        // This pattern (cache atomic reads at block boundaries) eliminates
        // per-sample atomic loads while keeping parameters responsive.
        // See Architecture Rules in CLAUDE.md.

        // Core Phase Distortion parameters
        const float paramDensityX = loadParam(pDistortionDensityX, 0.5f);
        const float paramTiltY = loadParam(pDistortionTiltY, 0.5f);
        const float paramDepth = loadParam(pPhaseDistortionDepth, 0.5f);
        const float paramStiffness = loadParam(pStiffnessAmount, 0.0f);
        const float paramCascadeBlend = loadParam(pCascadeBlend, 0.0f);
        const float paramCrossModulation = loadParam(pCrossModulationDepth, 0.0f);
        const float paramFormantBlend = loadParam(pFormantResonance, 0.0f);
        const float paramStereoWidth = loadParam(pStereoWidth, 0.0f);
        const float paramFilterCutoff = loadParam(pFilterCutoff, 8000.0f);
        const float paramFilterResonance = loadParam(pFilterResonance, 0.0f);
        const float paramMasterLevel = loadParam(pMasterLevel, 0.8f);

        // Amplitude envelope ADSR
        const float paramAmpAttack = loadParam(pAmpEnvAttack, 0.01f);
        const float paramAmpDecay = loadParam(pAmpEnvDecay, 0.1f);
        const float paramAmpSustain = loadParam(pAmpEnvSustain, 0.8f);
        const float paramAmpRelease = loadParam(pAmpEnvRelease, 0.3f);

        // Phase Distortion depth envelope ADSR
        const float paramPdAttack = loadParam(pPdEnvAttack, 0.01f);
        const float paramPdDecay = loadParam(pPdEnvDecay, 0.2f);
        const float paramPdSustain = loadParam(pPdEnvSustain, 0.5f);
        const float paramPdRelease = loadParam(pPdEnvRelease, 0.3f);

        // LFO 1
        const float paramLfo1Rate = loadParam(pLfo1Rate, 1.0f);
        const float paramLfo1Depth = loadParam(pLfo1Depth, 0.0f);
        const int paramLfo1Shape = static_cast<int>(loadParam(pLfo1Shape, 0.0f));

        // LFO 2
        const float paramLfo2Rate = loadParam(pLfo2Rate, 1.0f);
        const float paramLfo2Depth = loadParam(pLfo2Depth, 0.0f);
        const int paramLfo2Shape = static_cast<int>(loadParam(pLfo2Shape, 0.0f));

        // Voice configuration
        const int voiceModeIndex = static_cast<int>(loadParam(pVoiceMode, 2.0f));
        const float glideTimeSeconds = loadParam(pGlideTime, 0.0f);
        const float paramFormantIntensity = loadParam(pFormantIntensity, 0.0f);

        // XOceanus standard macros: CHARACTER, MOVEMENT, COUPLING, SPACE
        const float macroCharacter = loadParam(pMacroCharacter, 0.0f);
        const float macroMovement = loadParam(pMacroMovement, 0.0f);
        const float macroCoupling = loadParam(pMacroCoupling, 0.0f);
        const float macroSpace = loadParam(pMacroSpace, 0.0f);

        // ---- Determine polyphony from voice mode ----
        int maxPolyphony = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIndex)
        {
        case 0:
            maxPolyphony = 1;
            monoMode = true;
            break; // Mono
        case 1:
            maxPolyphony = 1;
            monoMode = true;
            legatoMode = true;
            break; // Legato
        case 2:
            maxPolyphony = 8;
            break; // Poly8
        case 3:
            maxPolyphony = 16;
            break; // Poly16
        default:
            maxPolyphony = 8;
            break;
        }

        // ---- Glide coefficient ----
        // Exponential approach: coeff = 1 - e^(-1 / (time * sampleRate))
        // At coeff=1.0, frequency snaps instantly (no portamento).
        float glideCoefficient = 1.0f;
        if (glideTimeSeconds > 0.001f)
            glideCoefficient = 1.0f - std::exp(-1.0f / (glideTimeSeconds * sampleRateFloat));

        // ---- Apply macro offsets to base PD parameters ----
        // CHARACTER macro: increases density (+0.5) and depth (+0.3) — more harmonics, more edge
        // MOVEMENT macro: increases cross-modulation (+0.5) — more inter-stage interaction
        // SPACE macro: increases stiffness (+0.4) and stereo width (+0.3) — wider, more metallic
        // COUPLING macro: increases cascade blend (+0.3) — more Stage 2 presence
        float effectiveDensity = clamp(paramDensityX + macroCharacter * 0.5f + couplingDensityMod, 0.0f, 1.0f);
        float effectiveTilt = clamp(paramTiltY + couplingTiltMod, 0.0f, 1.0f);
        float effectiveDepth = clamp(paramDepth + macroCharacter * 0.3f + couplingPhaseDistortionDepthMod, 0.0f, 1.0f);
        float effectiveCrossMod = clamp(paramCrossModulation + macroMovement * 0.5f, 0.0f, 1.0f);
        float effectiveStiff = clamp(paramStiffness + macroSpace * 0.4f, 0.0f, 1.0f);
        float effectiveStereo = clamp(paramStereoWidth + macroSpace * 0.3f, 0.0f, 1.0f);
        float effectiveCascade = clamp(paramCascadeBlend + macroCoupling * 0.3f, 0.0f, 1.0f);
        // D006: mod wheel adds up to +10kHz filter brightening at full wheel (sensitivity 0.5)
        float effectiveCutoff = clamp(
            paramFilterCutoff + couplingFilterCutoffMod * 4000.0f + modWheelValue * 0.5f * 10000.0f, 20.0f, 20000.0f);
        // D006: aftertouch added below — atPressure resolved after MIDI loop
        float effectiveFormant = clamp(paramFormantBlend + paramFormantIntensity, 0.0f, 1.0f);

        // Reset coupling accumulators for next block
        couplingPhaseDistortionDepthMod = 0.0f;
        couplingFilterCutoffMod = 0.0f;
        couplingDensityMod = 0.0f;
        couplingTiltMod = 0.0f;

        // ---- Euler-Bernoulli stiffness coefficient ----
        // Maps the [0,1] stiffness parameter exponentially to [0, 0.15].
        // The quadratic mapping (x^2) gives finer control in the subtle range
        // (0-0.05) where piano-like inharmonicity lives, while still reaching
        // extreme metallic territory (0.15) at full. Real piano wire B ranges
        // from ~0.0001 (bass strings) to ~0.01 (treble strings); our range
        // extends well beyond for creative sound design.
        float stiffnessCoefficient = effectiveStiff * effectiveStiff * 0.15f;

        // ---- Process MIDI events ----
        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (message.isNoteOn())
            {
                silenceGate.wake();
                noteOn(message.getNoteNumber(), message.getFloatVelocity(), maxPolyphony, monoMode, legatoMode,
                       glideCoefficient, paramAmpAttack, paramAmpDecay, paramAmpSustain, paramAmpRelease, paramPdAttack,
                       paramPdDecay, paramPdSustain, paramPdRelease, paramLfo1Rate, paramLfo1Shape, paramLfo2Rate,
                       paramLfo2Shape, effectiveCutoff, paramFilterResonance, stiffnessCoefficient);
            }
            else if (message.isNoteOff())
                noteOff(message.getNoteNumber());
            else if (message.isAllNotesOff() || message.isAllSoundOff())
                reset();
            // D006: channel pressure → aftertouch (applied to formant intensity below)
            else if (message.isChannelPressure())
                aftertouch.setChannelPressure(message.getChannelPressureValue() / 127.0f);
            // D006: CC1 mod wheel → filter cutoff brightening (classic brightness control; sensitivity 0.5)
            else if (message.isController() && message.getControllerNumber() == 1)
                modWheelValue = message.getControllerValue() / 127.0f;
            else if (message.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(message.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // D006: smooth aftertouch and apply to formant intensity (more vowel on pressure)
        aftertouch.updateBlock(numSamples);
        const float atPressure = aftertouch.getSmoothedPressure(0);
        // Sensitivity 0.3: full pressure adds up to +0.3 formant intensity
        effectiveFormant = clamp(effectiveFormant + atPressure * 0.3f, 0.0f, 1.0f);

        // D002 mod matrix — apply per-block.
        // Destinations: 0=Off, 1=FilterCutoff, 2=LFO1Rate, 3=Pitch, 4=AmpLevel, 5=PDDepth
        {
            ModMatrix<4>::Sources mSrc;
            mSrc.lfo1       = 0.0f; // LFO ticked per-voice
            mSrc.lfo2       = 0.0f;
            mSrc.env        = 0.0f;
            mSrc.velocity   = 0.0f;
            mSrc.keyTrack   = 0.0f;
            mSrc.modWheel   = modWheelValue;
            mSrc.aftertouch = atPressure;
            float mDst[6]   = {};
            modMatrix.apply(mSrc, mDst);
            obsidianModCutoffOffset = mDst[1] * 5000.0f;
            obsidianModPitchOffset  = mDst[3] * 12.0f;
            obsidianModLevelOffset  = mDst[4] * 0.5f;
            obsidianModPDDepthOffset = mDst[5] * 0.3f;
        }
        effectiveCutoff = clamp(effectiveCutoff + obsidianModCutoffOffset, 20.0f, 20000.0f);

        // D005: engine-level "stone breathing" formant LFO — 0.1 Hz sine, ±15% formant blend.
        // This creates a slow vowel drift (A→E→O cycle) across the polyphonic field —
        // the "obsidian glass hypnotic quality" described in the aquatic identity.
        // Runs at engine level (not per-voice) so all voices share the same vowel position,
        // creating a unified organic movement rather than individual voice chaos.
        // Phase increments by (0.1 Hz / sampleRate) per sample; we advance by block center.
        const double obsidianLfoHz = 0.1;
        const double obsidianLfoIncrement = obsidianLfoHz / static_cast<double>(sampleRateFloat);
        // Advance LFO phase by the block center position (mid-block approximation)
        obsidianLfoPhase += obsidianLfoIncrement * static_cast<double>(numSamples);
        if (obsidianLfoPhase >= 1.0)
            obsidianLfoPhase -= 1.0;
        const float obsidianLfoValue = std::sin(static_cast<float>(obsidianLfoPhase) * kTwoPi);
        // ±15% formant blend depth: full modulation = ±0.15 on the [0,1] formant range
        effectiveFormant = clamp(effectiveFormant + obsidianLfoValue * 0.15f, 0.0f, 1.0f);

        // ---- Update per-voice filter coefficients once per block ----
        // Updating filter coefficients per-block (not per-sample) is an
        // intentional CPU/quality tradeoff per XOceanus Architecture Rules.
        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;

            voice.mainFilter.setCoefficients(effectiveCutoff, paramFilterResonance, sampleRateFloat);

            // Formant filter center frequencies and Q values.
            // These approximate vocal formant regions:
            //   Band 0:  550 Hz — first formant (F1), vowel height
            //   Band 1: 1650 Hz — second formant (F2), vowel frontness
            //   Band 2: 3250 Hz — third formant (F3), voice timbre
            //   Band 3: 5000 Hz — fourth formant (F4), brilliance/air
            // Q values decrease with frequency (0.6 -> 0.4) for natural bandwidth spread.
            static constexpr float kFormantCenterFrequencies[4] = {550.0f, 1650.0f, 3250.0f, 5000.0f};
            static constexpr float kFormantQValues[4] = {0.6f, 0.5f, 0.45f, 0.4f};
            for (int band = 0; band < 4; ++band)
                voice.formantFilters[band].setCoefficients(kFormantCenterFrequencies[band], kFormantQValues[band],
                                                           sampleRateFloat);
        }

        float peakEnvelopeLevel = 0.0f;

        // CPU fix (OBSIDIAN): precompute all 4 Euler-Bernoulli stretchedRatio values once
        // per block. stiffnessCoefficient is block-rate — std::sqrt x4 per sample per voice
        // is eliminated. The harmonic numbers (2,3,4,5) and stiffnessCoefficient are all
        // block-constant, so the 4 stretched ratios are invariant within the block.
        float cachedStretchedRatios[4] = {};
        if (stiffnessCoefficient > 0.0001f)
        {
            for (int partialIndex = 1; partialIndex <= 4; ++partialIndex)
            {
                float harmonicNumber = static_cast<float>(partialIndex + 1);
                cachedStretchedRatios[partialIndex - 1] =
                    harmonicNumber * std::sqrt(1.0f + stiffnessCoefficient * harmonicNumber * harmonicNumber);
            }
        }

        // ---- Per-Sample Render Loop ----
        for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
        {
            // Smooth control-rate parameters toward their targets (5ms time constant).
            // This prevents zipper noise when density/tilt/depth knobs are moved.
            smoothedDensity += (effectiveDensity - smoothedDensity) * paramSmoothingCoefficient;
            smoothedTilt += (effectiveTilt - smoothedTilt) * paramSmoothingCoefficient;
            smoothedDepth += (effectiveDepth - smoothedDepth) * paramSmoothingCoefficient;
            smoothedDensity = flushDenormal(smoothedDensity);
            smoothedTilt = flushDenormal(smoothedTilt);
            smoothedDepth = flushDenormal(smoothedDepth);

            float stereoMixLeft = 0.0f, stereoMixRight = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // ---- Voice-stealing crossfade (5ms linear ramp to zero) ----
                if (voice.isFadingOut)
                {
                    voice.crossfadeGain -= voiceCrossfadeRate;
                    // Flush denormals: as the crossfade approaches zero, the
                    // multiplication can produce subnormal floats that cause
                    // severe CPU spikes on x86 (up to 100x slower per operation).
                    voice.crossfadeGain = flushDenormal(voice.crossfadeGain);
                    if (voice.crossfadeGain <= 0.0f)
                    {
                        voice.crossfadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // ---- Portamento (exponential frequency glide) ----
                voice.currentFrequency += (voice.targetFrequency - voice.currentFrequency) * voice.glideCoefficient;
                // Flush denormals: frequency difference shrinks exponentially
                // toward zero during glide, producing subnormal floats that
                // would stall the FPU pipeline.
                voice.currentFrequency = flushDenormal(voice.currentFrequency);

                // ---- Process envelopes ----
                float amplitudeLevel = voice.amplitudeEnvelope.process();
                float pdEnvelopeLevel = voice.phaseDistortionEnvelope.process();

                if (!voice.amplitudeEnvelope.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // ---- LFO modulation ----
                float lfo1Output = voice.lfo1.process() * paramLfo1Depth;
                float lfo2Output = voice.lfo2.process() * paramLfo2Depth;

                // LFO routing: LFO1 modulates PD depth (+/- 30%), LFO2 modulates density (+/- 20%)
                float modulatedDepth = clamp(smoothedDepth + lfo1Output * 0.3f, 0.0f, 1.0f);
                float modulatedDensity = clamp(smoothedDensity + lfo2Output * 0.2f, 0.0f, 1.0f);

                // ---- Phase increment ----
                float frequency = voice.currentFrequency * PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);
                float phaseIncrement = frequency / sampleRateFloat;

                // ======== PD STAGE 1 — Primary Phase Distortion ========

                // Left channel: use direct density/tilt values
                float densityLeft = modulatedDensity;
                float tiltLeft = smoothedTilt;

                // Right channel: offset density and tilt by stereo divergence amount.
                // This creates stereo width from a single oscillator by giving L and R
                // slightly different timbres — a technique unique to Obsidian that the
                // original CZ series never had (the CZ-5000 had stereo but via simple panning).
                float stereoOffset = effectiveStereo * 0.1f;
                float densityRight = clamp(densityLeft + stereoOffset, 0.0f, 1.0f);
                float tiltRight = clamp(tiltLeft + stereoOffset, 0.0f, 1.0f);

                // Advance the master phase accumulator
                voice.phaseStage1 += phaseIncrement;
                if (voice.phaseStage1 >= 1.0f)
                    voice.phaseStage1 -= 1.0f;

                // ---- Euler-Bernoulli Stiffness Engine ----
                // Adds inharmonic partials to the fundamental PD tone.
                // For a stiff bar/string, partial N has frequency:
                //   f_n = N * f0 * sqrt(1 + B * N^2)
                // where B is the stiffness coefficient. This creates the characteristic
                // "stretched" partials heard in piano strings, bells, and metallic bars.
                // We sum 4 partials (harmonics 2-5) with 1/N^2 amplitude rolloff for
                // natural spectral decay, scaled by stiffness amount.
                float stiffnessColor = 0.0f;
                if (stiffnessCoefficient > 0.0001f)
                {
                    // CPU fix: use block-rate precomputed stretchedRatios (no std::sqrt per sample).
                    for (int partialIndex = 1; partialIndex <= 4; ++partialIndex)
                    {
                        float harmonicNumber = static_cast<float>(partialIndex + 1);
                        float stretchedRatio = cachedStretchedRatios[partialIndex - 1];
                        float partialPhase = voice.phaseStage1 * stretchedRatio;
                        // Wrap phase to [0, 1) — faster than fmod for positive values
                        partialPhase -= static_cast<float>(static_cast<int>(partialPhase));
                        // 1/N^2 amplitude rolloff: natural spectral envelope for struck objects
                        stiffnessColor += lookupCosine(partialPhase) / (harmonicNumber * harmonicNumber);
                    }
                    // Scale by stiffness amount * 4 to bring partials to audible level
                    stiffnessColor *= stiffnessCoefficient * 4.0f;
                }

                // PD depth envelope modulates how much phase distortion is applied.
                // At pdEnvelopeLevel=0, output is a pure cosine (no harmonics).
                // At pdEnvelopeLevel=1 with full depth, maximum harmonic complexity.
                //
                // D001 fix: velocity shapes timbre, not just amplitude.
                // Harder hits push the PD depth deeper — more harmonic complexity on attack.
                // Sensitivity +0.2: a velocity of 1.0 adds +0.2 to PD depth before clamping.
                // This means soft playing sounds hollow/pure; hard playing sounds complex/edgy —
                // exactly the character of obsidian glass under percussion.
                float velocityPDBoost = voice.velocity * 0.2f;
                float depthEnvelopeAmount = pdEnvelopeLevel * clamp(modulatedDepth + velocityPDBoost, 0.0f, 1.0f);

                // Look up the distorted phase from the 2D LUT for L and R channels
                float distortedPhaseLeft = lookupDistortion(densityLeft, tiltLeft, voice.phaseStage1);
                float distortedPhaseRight = lookupDistortion(densityRight, tiltRight, voice.phaseStage1);

                // Crossfade between linear phase (pure cosine) and distorted phase.
                // This is the core CZ technique: the distortion function warps time
                // within each cycle, creating harmonics from a single cosine.
                float finalPhaseLeft =
                    voice.phaseStage1 + depthEnvelopeAmount * (distortedPhaseLeft - voice.phaseStage1);
                float finalPhaseRight =
                    voice.phaseStage1 + depthEnvelopeAmount * (distortedPhaseRight - voice.phaseStage1);

                // Generate Stage 1 output: cosine(distorted_phase) + stiffness partials
                float stage1Left = lookupCosine(finalPhaseLeft) + stiffnessColor;
                float stage1Right = lookupCosine(finalPhaseRight) + stiffnessColor;

                // ======== PD STAGE 2 — Cascade with Cross-Modulation ========
                // When cascade blend > 0, a second PD stage processes in parallel
                // with its own distortion coordinates (offset by +0.3 density, -0.2 tilt)
                // and can be cross-modulated by Stage 1's output. This creates
                // complex harmonic interactions impossible with a single CZ stage.

                float outputLeft = stage1Left;
                float outputRight = stage1Right;

                if (effectiveCascade > 0.001f)
                {
                    // Stage 2 phase runs at the same fundamental rate
                    voice.phaseStage2 += phaseIncrement;
                    if (voice.phaseStage2 >= 1.0f)
                        voice.phaseStage2 -= 1.0f;

                    // Cross-modulation: Stage 1 output amplitude modulates Stage 2 depth.
                    // This creates sidebands similar to FM but through phase warping.
                    float crossModLeft = stage1Left * effectiveCrossMod;
                    float crossModRight = stage1Right * effectiveCrossMod;

                    // Stage 2 distortion coordinates: offset from Stage 1 for timbral contrast.
                    // +0.3 density = more harmonically dense; -0.2 tilt = warmer bias.
                    float stage2Density = clamp(densityLeft + 0.3f, 0.0f, 1.0f);
                    float stage2Tilt = clamp(tiltLeft - 0.2f, 0.0f, 1.0f);

                    float distortedPhase2Left = lookupDistortion(stage2Density, stage2Tilt, voice.phaseStage2);
                    float distortedPhase2Right =
                        lookupDistortion(clamp(stage2Density + stereoOffset, 0.0f, 1.0f),
                                         clamp(stage2Tilt + stereoOffset, 0.0f, 1.0f), voice.phaseStage2);

                    // Apply cross-modulation: Stage 1 output increases Stage 2 PD depth.
                    // Each channel uses its own cross-mod signal for true stereo interaction.
                    float stage2DepthLeft = clamp(depthEnvelopeAmount * (1.0f + crossModLeft), 0.0f, 1.0f);
                    float stage2DepthRight = clamp(depthEnvelopeAmount * (1.0f + crossModRight), 0.0f, 1.0f);

                    float stage2PhaseLeft =
                        voice.phaseStage2 + stage2DepthLeft * (distortedPhase2Left - voice.phaseStage2);
                    float stage2PhaseRight =
                        voice.phaseStage2 + stage2DepthRight * (distortedPhase2Right - voice.phaseStage2);

                    float stage2OutputLeft = lookupCosine(stage2PhaseLeft);
                    float stage2OutputRight = lookupCosine(stage2PhaseRight);

                    // Blend Stage 1 and Stage 2 cascade (linear crossfade)
                    outputLeft = outputLeft * (1.0f - effectiveCascade) + stage2OutputLeft * effectiveCascade;
                    outputRight = outputRight * (1.0f - effectiveCascade) + stage2OutputRight * effectiveCascade;
                }

                // ======== 4-FORMANT RESONANCE NETWORK ========
                // Four parallel bandpass filters at vocal formant frequencies.
                // Blending this with the dry PD signal adds vowel-like resonance,
                // giving the crystalline PD tones an organic, vocal quality.
                if (effectiveFormant > 0.001f)
                {
                    float formantSumLeft = 0.0f, formantSumRight = 0.0f;
                    for (int band = 0; band < 4; ++band)
                    {
                        // Sum to mono for formant filtering, then restore to stereo.
                        // This preserves the stereo image while applying consistent resonance.
                        float monoInput = (outputLeft + outputRight) * 0.5f;
                        float filteredSample = voice.formantFilters[band].processSample(monoInput);
                        formantSumLeft += filteredSample;
                        formantSumRight += filteredSample;
                    }
                    // Average the 4 bands to normalize level
                    formantSumLeft *= 0.25f;
                    formantSumRight *= 0.25f;

                    // Crossfade between dry PD and formant-filtered signal
                    outputLeft = outputLeft * (1.0f - effectiveFormant) + formantSumLeft * effectiveFormant;
                    outputRight = outputRight * (1.0f - effectiveFormant) + formantSumRight * effectiveFormant;
                }

                // ======== MAIN FILTER ========
                // Cytomic SVF lowpass — final timbral shaping.
                // P0-01 fix: both channels now filtered
                outputLeft = voice.mainFilter.processSample(outputLeft);
                outputRight = voice.mainFilter.processSample(outputRight);

                // ======== AMPLITUDE SHAPING ========
                float voiceGain = amplitudeLevel * voice.velocity * voice.crossfadeGain;
                outputLeft *= voiceGain;
                outputRight *= voiceGain;

                // Flush denormals on final voice output: after multiplication by
                // small envelope values near note release, subnormal floats are
                // common. Without flushing, these cause x86 FPU pipeline stalls
                // that can spike CPU usage by 100x per arithmetic operation.
                outputLeft = flushDenormal(outputLeft);
                outputRight = flushDenormal(outputRight);

                stereoMixLeft += outputLeft;
                stereoMixRight += outputRight;

                peakEnvelopeLevel = std::max(peakEnvelopeLevel, amplitudeLevel);
            }

            // Apply master level
            float finalLeft = stereoMixLeft * paramMasterLevel;
            float finalRight = stereoMixRight * paramMasterLevel;

            // Write to output buffer (addSample preserves any pre-existing content)
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sampleIndex, finalLeft);
                buffer.addSample(1, sampleIndex, finalRight);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sampleIndex, (finalLeft + finalRight) * 0.5f);
            }

            // Cache output for coupling reads by other engines
            if (sampleIndex < static_cast<int>(outputCacheLeft.size()))
            {
                outputCacheLeft[static_cast<size_t>(sampleIndex)] = finalLeft;
                outputCacheRight[static_cast<size_t>(sampleIndex)] = finalRight;
            }
        }

        envelopeFollowerOutput = peakEnvelopeLevel;

        // Update active voice count (atomic-safe for UI thread reads)
        int count = 0;
        for (const auto& voice : voices)
            if (voice.active)
                ++count;
        activeVoices.store(count, std::memory_order_relaxed);

        silenceGate.analyzeBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  C O U P L I N G
    //==========================================================================

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto index = static_cast<size_t>(sampleIndex);
        if (channel == 0 && index < outputCacheLeft.size())
            return outputCacheLeft[index];
        if (channel == 1 && index < outputCacheRight.size())
            return outputCacheRight[index];
        if (channel == 2)
            return envelopeFollowerOutput; // Envelope follower for amplitude-driven coupling
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
        case CouplingType::AudioToFM:
            // External audio modulates PD depth — creates FM-like sidebands
            // through phase warping rather than frequency modulation.
            // 0.5f scaling keeps modulation musical at unity coupling amount.
            couplingPhaseDistortionDepthMod += amount * 0.5f;
            break;

        case CouplingType::AmpToFilter:
            // External amplitude drives filter cutoff — rhythmic filtering.
            // Direct 1:1 scaling (multiplied by 4000 Hz range in renderBlock).
            couplingFilterCutoffMod += amount;
            break;

        case CouplingType::EnvToMorph:
            // External envelope morphs the 2D distortion position.
            // X (density) gets 0.3x scaling, Y (tilt) gets 0.2x — density
            // is more perceptually dramatic so it gets more influence.
            couplingDensityMod += amount * 0.3f;
            couplingTiltMod += amount * 0.2f;
            break;

        case CouplingType::RhythmToBlend:
            // Rhythmic source drives PD depth — syncs timbral movement to tempo.
            // 0.3f scaling for subtle rhythmic PD pulsing.
            couplingPhaseDistortionDepthMod += amount * 0.3f;
            break;

        default:
            break;
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
        // ---- Core Phase Distortion Parameters ----

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"obsidian_densityX", 1}, "Obsidian Density X",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"obsidian_tiltY", 1}, "Obsidian Tilt Y",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"obsidian_depth", 1}, "Obsidian Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_stiffness", 1}, "Obsidian Stiffness",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_cascadeBlend", 1}, "Obsidian Cascade",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_crossModDepth", 1}, "Obsidian Cross-Mod",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_formantIntensity", 1}, "Obsidian Formant",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // P0-04 fix: formant blend (mix between dry PD and formant-filtered signal).
        // Previously shared the ID "obsidian_formantIntensity" with the formant resonance
        // amount, causing both pFormantResonance and pFormantIntensity to point at the
        // same parameter slot. Now correctly registered under its own ID.
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_formantResonance", 1}, "Obsidian Formant Blend",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"obsidian_stereoWidth", 1}, "Obsidian Stereo",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // ---- Filter ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_filterCutoff", 1}, "Obsidian Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_filterReso", 1}, "Obsidian Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // ---- Level ----

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"obsidian_level", 1}, "Obsidian Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        // ---- Amplitude Envelope (ADSR) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_ampAttack", 1}, "Obsidian Amp Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_ampDecay", 1}, "Obsidian Amp Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_ampSustain", 1}, "Obsidian Amp Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_ampRelease", 1}, "Obsidian Amp Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // ---- Phase Distortion Depth Envelope (ADSR) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_depthAttack", 1}, "Obsidian PD Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_depthDecay", 1}, "Obsidian PD Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_depthSustain", 1}, "Obsidian PD Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_depthRelease", 1}, "Obsidian PD Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // ---- LFO 1 ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_lfo1Rate", 1}, "Obsidian LFO1 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_lfo1Depth", 1}, "Obsidian LFO1 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"obsidian_lfo1Shape", 1}, "Obsidian LFO1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // ---- LFO 2 ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_lfo2Rate", 1}, "Obsidian LFO2 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_lfo2Depth", 1}, "Obsidian LFO2 Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"obsidian_lfo2Shape", 1}, "Obsidian LFO2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // ---- Voice Configuration ----

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"obsidian_polyphony", 1}, "Obsidian Voice Mode",
            juce::StringArray{"Mono", "Legato", "Poly8", "Poly16"}, 2));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_glide", 1}, "Obsidian Glide",
            juce::NormalisableRange<float>(0.0f, 2.0f, 0.001f, 0.5f), 0.0f));

        // ---- XOceanus Standard Macros ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_macroCharacter", 1}, "Obsidian Macro CHARACTER",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_macroMovement", 1}, "Obsidian Macro MOVEMENT",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_macroCoupling", 1}, "Obsidian Macro COUPLING",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"obsidian_macroSpace", 1}, "Obsidian Macro SPACE",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // D002 mod matrix — 4 user-configurable source→destination slots
        static const juce::StringArray kObsidianModDests {"Off", "Filter Cutoff", "LFO1 Rate", "Pitch", "Amp Level",
                                                           "PD Depth"};
        ModMatrix<4>::addParameters(params, "obsidian_", "Obsidian", kObsidianModDests);
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // ---- Core PD parameters ----
        pDistortionDensityX = apvts.getRawParameterValue("obsidian_densityX");
        pDistortionTiltY = apvts.getRawParameterValue("obsidian_tiltY");
        pPhaseDistortionDepth = apvts.getRawParameterValue("obsidian_depth");
        pStiffnessAmount = apvts.getRawParameterValue("obsidian_stiffness");
        pCascadeBlend = apvts.getRawParameterValue("obsidian_cascadeBlend");
        pCrossModulationDepth = apvts.getRawParameterValue("obsidian_crossModDepth");
        pFormantResonance =
            apvts.getRawParameterValue("obsidian_formantResonance"); // P0-04 fix: was colliding with pFormantIntensity
        pStereoWidth = apvts.getRawParameterValue("obsidian_stereoWidth");
        pFilterCutoff = apvts.getRawParameterValue("obsidian_filterCutoff");
        pFilterResonance = apvts.getRawParameterValue("obsidian_filterReso");
        pMasterLevel = apvts.getRawParameterValue("obsidian_level");

        // ---- Amplitude envelope ----
        pAmpEnvAttack = apvts.getRawParameterValue("obsidian_ampAttack");
        pAmpEnvDecay = apvts.getRawParameterValue("obsidian_ampDecay");
        pAmpEnvSustain = apvts.getRawParameterValue("obsidian_ampSustain");
        pAmpEnvRelease = apvts.getRawParameterValue("obsidian_ampRelease");

        // ---- PD depth envelope ----
        pPdEnvAttack = apvts.getRawParameterValue("obsidian_depthAttack");
        pPdEnvDecay = apvts.getRawParameterValue("obsidian_depthDecay");
        pPdEnvSustain = apvts.getRawParameterValue("obsidian_depthSustain");
        pPdEnvRelease = apvts.getRawParameterValue("obsidian_depthRelease");

        // ---- LFOs ----
        pLfo1Rate = apvts.getRawParameterValue("obsidian_lfo1Rate");
        pLfo1Depth = apvts.getRawParameterValue("obsidian_lfo1Depth");
        pLfo1Shape = apvts.getRawParameterValue("obsidian_lfo1Shape");
        pLfo2Rate = apvts.getRawParameterValue("obsidian_lfo2Rate");
        pLfo2Depth = apvts.getRawParameterValue("obsidian_lfo2Depth");
        pLfo2Shape = apvts.getRawParameterValue("obsidian_lfo2Shape");

        // ---- Voice configuration ----
        pVoiceMode = apvts.getRawParameterValue("obsidian_polyphony");
        pGlideTime = apvts.getRawParameterValue("obsidian_glide");

        // ---- Formant intensity ----
        pFormantIntensity = apvts.getRawParameterValue("obsidian_formantIntensity");

        // ---- XOceanus macros ----
        pMacroCharacter = apvts.getRawParameterValue("obsidian_macroCharacter");
        pMacroMovement = apvts.getRawParameterValue("obsidian_macroMovement");
        pMacroCoupling = apvts.getRawParameterValue("obsidian_macroCoupling");
        pMacroSpace = apvts.getRawParameterValue("obsidian_macroSpace");
        modMatrix.attachParameters(apvts, "obsidian_");
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  I D E N T I T Y
    //==========================================================================

    juce::String getEngineId() const override { return "Obsidian"; }

    juce::Colour getAccentColour() const override { return juce::Colour(0xFFE8E0D8); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices.load(std::memory_order_relaxed); }

private:
    SilenceGate silenceGate;

    //==========================================================================
    //  Helper: Safe Atomic Parameter Load
    //==========================================================================

    static float loadParam(std::atomic<float>* paramPointer, float fallback) noexcept
    {
        return (paramPointer != nullptr) ? paramPointer->load() : fallback;
    }

    //==========================================================================
    //  Phase Distortion LUT Generation
    //
    //  Builds the 2D distortion function table that maps:
    //    D(density, tilt, phase) -> distorted_phase [0, 1]
    //
    //  The distortion function D(phase) warps the phase accumulator's progress
    //  through each cycle. When the distorted phase is fed to a cosine oscillator,
    //  the waveform is reshaped, creating harmonics from a pure tone — this is the
    //  core principle of Casio's CZ-series phase distortion synthesis.
    //
    //  Density (X axis) controls harmonic complexity:
    //    Low density:  near-linear mapping -> sine-like output (few harmonics)
    //    High density: many inflection points -> FM-like rich harmonic content
    //
    //  Tilt (Y axis) controls spectral energy distribution:
    //    Low tilt:  power exponent > 1 -> slow start, fast middle -> warm, fundamental-heavy
    //    High tilt: power exponent < 1 -> fast start, slow end -> bright, upper-harmonic-heavy
    //
    //==========================================================================

    void buildDistortionLUT()
    {
        for (int densityIndex = 0; densityIndex < kLutDensitySteps; ++densityIndex)
        {
            float density = static_cast<float>(densityIndex) / static_cast<float>(kLutDensitySteps - 1);

            for (int tiltIndex = 0; tiltIndex < kLutTiltSteps; ++tiltIndex)
            {
                float tilt = static_cast<float>(tiltIndex) / static_cast<float>(kLutTiltSteps - 1);

                for (int phaseIndex = 0; phaseIndex < kLutPhaseSteps; ++phaseIndex)
                {
                    float phase = static_cast<float>(phaseIndex) / static_cast<float>(kLutPhaseSteps);

                    // ---- Fold count: 1 to 6 inflection points based on density ----
                    // More folds = more zero-crossings in the derivative = more harmonics.
                    // At density=0: 1 fold (near-sinusoidal). At density=1: 6 folds (complex).
                    float foldCount = 1.0f + density * 5.0f;

                    // Create folded phase: sin(folds * PI * phase) produces inflection points.
                    // Rectification (fabs) maps the bipolar sine to [0,1] magnitude.
                    float foldedPhase = std::fabs(std::sin(foldCount * kPI * phase));

                    // ---- Tilt: power-curve bias for spectral energy distribution ----
                    // Exponent range [0.2, 2.0]:
                    //   tilt=0 -> exponent=2.0 -> quadratic (slow start) -> warm/fundamental-heavy
                    //   tilt=1 -> exponent=0.2 -> near-square-root (fast start) -> bright/harmonic-heavy
                    float tiltExponent = 2.0f - tilt * 1.8f;
                    float tiltedPhase = std::pow(phase, tiltExponent);

                    // ---- Blend linear phase with folded/tilted distortion ----
                    // Quadratic blend factor: density^2 gives gradual onset of distortion.
                    // At low density: output is mostly the linear phase (pure cosine).
                    // At high density: output is a mix of tilted and folded curves.
                    float blendFactor = density * density;
                    float distortedPhase =
                        phase * (1.0f - blendFactor) +
                        (tiltedPhase * (1.0f - blendFactor * 0.5f) + foldedPhase * blendFactor * 0.5f) * blendFactor;

                    // Clamp to [0, 1] — safety against floating-point overshoot
                    distortedPhase = std::max(0.0f, std::min(1.0f, distortedPhase));

                    // Force endpoint continuity: phase=0 maps to 0, phase=1 maps to 1.
                    // This ensures the waveform is periodic (no DC offset or discontinuity
                    // at cycle boundaries, which would cause audible clicks).
                    if (phaseIndex == 0)
                        distortedPhase = 0.0f;
                    else if (phaseIndex == kLutPhaseSteps - 1)
                        distortedPhase = 1.0f;

                    distortionLUT[static_cast<size_t>(densityIndex)][static_cast<size_t>(tiltIndex)]
                                 [static_cast<size_t>(phaseIndex)] = distortedPhase;
                }
            }
        }
    }

    //==========================================================================
    //  Trilinear LUT Interpolation
    //
    //  Interpolates the distortion function across all three dimensions
    //  (density, tilt, phase) for smooth parameter sweeps without stepping
    //  artifacts. Uses 8 LUT samples per lookup (the cube corners).
    //==========================================================================

    float lookupDistortion(float density, float tilt, float phase) const noexcept
    {
        // Clamp inputs to valid range
        density = std::max(0.0f, std::min(1.0f, density));
        tilt = std::max(0.0f, std::min(1.0f, tilt));

        // Wrap phase to [0, 1) — handles accumulator overflow
        phase = phase - static_cast<float>(static_cast<int>(phase));
        if (phase < 0.0f)
            phase += 1.0f;

        // Map normalized [0,1] inputs to LUT index space
        float densityIdx = density * static_cast<float>(kLutDensitySteps - 1);
        float tiltIdx = tilt * static_cast<float>(kLutTiltSteps - 1);
        float phaseIdx = phase * static_cast<float>(kLutPhaseSteps - 1);

        // Integer indices (lower corner of interpolation cube)
        int d0 = static_cast<int>(densityIdx);
        int t0 = static_cast<int>(tiltIdx);
        int p0 = static_cast<int>(phaseIdx);

        // Upper corner indices (clamped to LUT bounds)
        int d1 = std::min(d0 + 1, kLutDensitySteps - 1);
        int t1 = std::min(t0 + 1, kLutTiltSteps - 1);
        int p1 = std::min(p0 + 1, kLutPhaseSteps - 1);

        // Fractional interpolation weights
        float densityFrac = densityIdx - static_cast<float>(d0);
        float tiltFrac = tiltIdx - static_cast<float>(t0);
        float phaseFrac = phaseIdx - static_cast<float>(p0);

        // Fetch all 8 cube corners from the LUT
        auto& lut = distortionLUT;
        float c000 = lut[static_cast<size_t>(d0)][static_cast<size_t>(t0)][static_cast<size_t>(p0)];
        float c001 = lut[static_cast<size_t>(d0)][static_cast<size_t>(t0)][static_cast<size_t>(p1)];
        float c010 = lut[static_cast<size_t>(d0)][static_cast<size_t>(t1)][static_cast<size_t>(p0)];
        float c011 = lut[static_cast<size_t>(d0)][static_cast<size_t>(t1)][static_cast<size_t>(p1)];
        float c100 = lut[static_cast<size_t>(d1)][static_cast<size_t>(t0)][static_cast<size_t>(p0)];
        float c101 = lut[static_cast<size_t>(d1)][static_cast<size_t>(t0)][static_cast<size_t>(p1)];
        float c110 = lut[static_cast<size_t>(d1)][static_cast<size_t>(t1)][static_cast<size_t>(p0)];
        float c111 = lut[static_cast<size_t>(d1)][static_cast<size_t>(t1)][static_cast<size_t>(p1)];

        // Trilinear interpolation: phase -> tilt -> density
        float c00 = c000 + phaseFrac * (c001 - c000);
        float c01 = c010 + phaseFrac * (c011 - c010);
        float c10 = c100 + phaseFrac * (c101 - c100);
        float c11 = c110 + phaseFrac * (c111 - c110);

        float c0 = c00 + tiltFrac * (c01 - c00);
        float c1 = c10 + tiltFrac * (c11 - c10);

        return c0 + densityFrac * (c1 - c0);
    }

    //==========================================================================
    //  Cosine LUT Lookup with Linear Interpolation
    //
    //  4096-entry cosine table with wrap-around interpolation.
    //  Bitmask wrapping (& kCosineLUTSize-1) works because the table size
    //  is a power of two (4096 = 2^12), making this branchless and fast.
    //==========================================================================

    static constexpr int kCosineLUTSize = 4096; // Power of 2 for bitmask wrapping

    float lookupCosine(float phase) const noexcept
    {
        // Wrap phase to [0, 1)
        phase = phase - static_cast<float>(static_cast<int>(phase));
        if (phase < 0.0f)
            phase += 1.0f;

        float floatIndex = phase * static_cast<float>(kCosineLUTSize);
        int index0 = static_cast<int>(floatIndex);
        int index1 = (index0 + 1) & (kCosineLUTSize - 1); // Bitmask wrap (power-of-2 table)
        float fraction = floatIndex - static_cast<float>(index0);
        index0 &= (kCosineLUTSize - 1);

        // Linear interpolation between adjacent LUT entries
        return cosineLUT[static_cast<size_t>(index0)] +
               fraction * (cosineLUT[static_cast<size_t>(index1)] - cosineLUT[static_cast<size_t>(index0)]);
    }

    //==========================================================================
    //  MIDI Note Handling
    //==========================================================================

    void noteOn(int noteNumber, float velocity, int maxPolyphony, bool monoMode, bool legatoMode,
                float glideCoefficient, float ampAttack, float ampDecay, float ampSustain, float ampRelease,
                float pdAttack, float pdDecay, float pdSustain, float pdRelease, float lfo1Rate, int lfo1Shape,
                float lfo2Rate, int lfo2Shape, float cutoff, float resonance, float stiffnessB)
    {
        float frequency = midiNoteToFrequencyHz(static_cast<float>(noteNumber));

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasAlreadyActive = voice.active;

            voice.targetFrequency = frequency;

            if (legatoMode && wasAlreadyActive)
            {
                // Legato: glide to new pitch without retriggering envelopes.
                // This creates smooth melodic lines — essential for lead sounds.
                voice.glideCoefficient = glideCoefficient;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
            }
            else
            {
                voice.active = true;
                voice.noteNumber = noteNumber;
                voice.velocity = velocity;
                voice.startTime = voiceTimestampCounter++;
                voice.currentFrequency = frequency;
                voice.glideCoefficient = glideCoefficient;
                voice.phaseStage1 = 0.0f;
                voice.phaseStage2 = 0.0f;
                voice.isFadingOut = false;
                voice.crossfadeGain = 1.0f;

                voice.amplitudeEnvelope.setParams(ampAttack, ampDecay, ampSustain, ampRelease, sampleRateFloat);
                voice.amplitudeEnvelope.noteOn();
                voice.phaseDistortionEnvelope.setParams(pdAttack, pdDecay, pdSustain, pdRelease, sampleRateFloat);
                voice.phaseDistortionEnvelope.noteOn();

                voice.lfo1.setRate(lfo1Rate, sampleRateFloat);
                voice.lfo1.setShape(lfo1Shape);
                voice.lfo2.setRate(lfo2Rate, sampleRateFloat);
                voice.lfo2.setShape(lfo2Shape);

                initializeVoiceFilters(voice, cutoff, resonance, stiffnessB);
            }
            return;
        }

        // ---- Polyphonic mode ----
        int voiceIndex = VoiceAllocator::findFreeVoice(voices, maxPolyphony);
        auto& voice = voices[static_cast<size_t>(voiceIndex)];

        // If stealing an active voice, initiate crossfade to prevent click
        if (voice.active)
        {
            voice.isFadingOut = true;
            voice.crossfadeGain = std::min(voice.crossfadeGain, 0.5f); // Quick steal: cap at 50% to shorten fade
        }

        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.startTime = voiceTimestampCounter++;
        voice.currentFrequency = frequency;
        voice.targetFrequency = frequency;
        voice.glideCoefficient = 1.0f; // No glide in polyphonic mode
        voice.phaseStage1 = 0.0f;
        voice.phaseStage2 = 0.0f;
        voice.isFadingOut = false;
        voice.crossfadeGain = 1.0f;

        voice.amplitudeEnvelope.setParams(ampAttack, ampDecay, ampSustain, ampRelease, sampleRateFloat);
        voice.amplitudeEnvelope.noteOn();
        voice.phaseDistortionEnvelope.setParams(pdAttack, pdDecay, pdSustain, pdRelease, sampleRateFloat);
        voice.phaseDistortionEnvelope.noteOn();

        voice.lfo1.setRate(lfo1Rate, sampleRateFloat);
        voice.lfo1.setShape(lfo1Shape);
        voice.lfo1.reset();
        voice.lfo2.setRate(lfo2Rate, sampleRateFloat);
        voice.lfo2.setShape(lfo2Shape);
        voice.lfo2.reset();

        initializeVoiceFilters(voice, cutoff, resonance, stiffnessB);
    }

    void noteOff(int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.isFadingOut)
            {
                voice.amplitudeEnvelope.noteOff();
                voice.phaseDistortionEnvelope.noteOff();
            }
        }
    }

    //==========================================================================
    //  Voice Filter Initialization
    //==========================================================================

    void initializeVoiceFilters(ObsidianVoice& voice, float cutoff, float resonance, float /*stiffnessB*/) noexcept
    {
        voice.mainFilter.reset();
        voice.mainFilter.setMode(CytomicSVF::Mode::LowPass);
        voice.mainFilter.setCoefficients(cutoff, resonance, sampleRateFloat);

        // Initialize formant filters at fixed vocal-region frequencies.
        // See renderBlock for the acoustic rationale behind these values.
        static constexpr float kFormantCenterFrequencies[4] = {550.0f, 1650.0f, 3250.0f, 5000.0f};
        static constexpr float kFormantQValues[4] = {0.6f, 0.5f, 0.45f, 0.4f};
        for (int band = 0; band < 4; ++band)
        {
            voice.formantFilters[band].reset();
            voice.formantFilters[band].setMode(CytomicSVF::Mode::BandPass);
            voice.formantFilters[band].setCoefficients(kFormantCenterFrequencies[band], kFormantQValues[band],
                                                       sampleRateFloat);
        }
    }

    //==========================================================================
    //  MIDI Note to Frequency Conversion
    //
    //  Standard equal temperament: f = 440 * 2^((note - 69) / 12)
    //  A4 (MIDI note 69) = 440 Hz reference.
    //==========================================================================

    static float midiNoteToFrequencyHz(float midiNote) noexcept
    {
        return 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
    }

    //==========================================================================
    //  M E M B E R   D A T A
    //==========================================================================

    // ---- Audio configuration ----
    double sampleRateDouble = 44100.0;
    float sampleRateFloat = 44100.0f;
    float paramSmoothingCoefficient = 0.1f; // 5ms time constant (recomputed in prepare)
    float voiceCrossfadeRate = 0.01f;       // 5ms linear fade (recomputed in prepare)

    // ---- Voice pool ----
    std::array<ObsidianVoice, kMaxVoices> voices;
    uint64_t voiceTimestampCounter = 0; // Monotonic counter for LRU voice stealing
    std::atomic<int> activeVoices{0};

    // ---- Smoothed control parameters ----
    // These interpolate toward target values each sample to prevent zipper noise.
    float smoothedDensity = 0.5f;
    float smoothedTilt = 0.5f;
    float smoothedDepth = 0.5f;

    // D005: engine-level formant LFO — "stone breathing" at 0.1 Hz.
    // Modulates formant vowel blend position ±15% across the polyphonic field.
    // Shared by all voices so they move in unison like one living instrument.
    // Advances by block in renderBlock; phase range [0, 1).
    double obsidianLfoPhase = 0.0;

    // D006: aftertouch — pressure increases formant intensity (more vowel character)
    PolyAftertouch aftertouch;

    // D006: mod wheel — CC1 brightens filter cutoff (classic brightness expression; sensitivity 0.5)
    float modWheelValue = 0.0f;
    float pitchBendNorm = 0.0f;

    // ---- Coupling accumulators ----
    // Reset to zero each block; other engines add their modulation contributions.
    float envelopeFollowerOutput = 0.0f;
    float couplingPhaseDistortionDepthMod = 0.0f;
    float couplingFilterCutoffMod = 0.0f;
    float couplingDensityMod = 0.0f;
    float couplingTiltMod = 0.0f;

    // ---- Output cache for coupling reads ----
    // Other engines read this via getSampleForCoupling() during their render pass.
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    // ---- Phase Distortion LUT: [density][tilt][phase] ----
    // ~2 MB, built once in prepare(), shared across all voices.
    std::array<std::array<std::array<float, kLutPhaseSteps>, kLutTiltSteps>, kLutDensitySteps> distortionLUT{};

    // ---- Cosine LUT for fast oscillator evaluation ----
    // 4096 entries * 4 bytes = 16 KB. Linear interpolation gives <0.01% error.
    std::array<float, kCosineLUTSize> cosineLUT{};

    // ---- Cached APVTS parameter pointers (read-once-per-block via ParamSnapshot) ----

    // Core PD parameters
    std::atomic<float>* pDistortionDensityX = nullptr;
    std::atomic<float>* pDistortionTiltY = nullptr;
    std::atomic<float>* pPhaseDistortionDepth = nullptr;
    std::atomic<float>* pStiffnessAmount = nullptr;
    std::atomic<float>* pCascadeBlend = nullptr;
    std::atomic<float>* pCrossModulationDepth = nullptr;
    std::atomic<float>* pFormantResonance = nullptr;
    std::atomic<float>* pStereoWidth = nullptr;
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterResonance = nullptr;
    std::atomic<float>* pMasterLevel = nullptr;

    // Amplitude envelope
    std::atomic<float>* pAmpEnvAttack = nullptr;
    std::atomic<float>* pAmpEnvDecay = nullptr;
    std::atomic<float>* pAmpEnvSustain = nullptr;
    std::atomic<float>* pAmpEnvRelease = nullptr;

    // PD depth envelope
    std::atomic<float>* pPdEnvAttack = nullptr;
    std::atomic<float>* pPdEnvDecay = nullptr;
    std::atomic<float>* pPdEnvSustain = nullptr;
    std::atomic<float>* pPdEnvRelease = nullptr;

    // LFOs
    std::atomic<float>* pLfo1Rate = nullptr;
    std::atomic<float>* pLfo1Depth = nullptr;
    std::atomic<float>* pLfo1Shape = nullptr;
    std::atomic<float>* pLfo2Rate = nullptr;
    std::atomic<float>* pLfo2Depth = nullptr;
    std::atomic<float>* pLfo2Shape = nullptr;

    // Voice configuration
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlideTime = nullptr;
    std::atomic<float>* pFormantIntensity = nullptr;

    // XOceanus macros
    std::atomic<float>* pMacroCharacter = nullptr;
    std::atomic<float>* pMacroMovement = nullptr;
    std::atomic<float>* pMacroCoupling = nullptr;
    std::atomic<float>* pMacroSpace = nullptr;

    // D002 mod matrix — 4-slot configurable modulation routing
    ModMatrix<4> modMatrix;
    float obsidianModCutoffOffset  = 0.0f;
    float obsidianModPitchOffset   = 0.0f;
    float obsidianModLevelOffset   = 0.0f;
    float obsidianModPDDepthOffset = 0.0f;
};

} // namespace xoceanus
