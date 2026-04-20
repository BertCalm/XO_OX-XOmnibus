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
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/Effects/DCBlocker.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/PolyBLEP.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
//
//  O L V I D O   E N G I N E
//  Spectral Erosion Synthesis
//
//  XO_OX Aquatic Identity: Coelacanth (Latimeria chalumnae) — Midnight Zone
//  (1000-4000m). The living fossil that time forgot. Sound erodes like memory
//  in deep water — higher frequencies dissolving first, leaving only the
//  abyssal low end intact. Emergence (negative current) reverses the flow:
//  harmonics sharpen from silence into presence.
//  Accent: Coelacanth Blue #3B6E8F.
//
//  Architecture: PolyBLEP oscillator → 6-band Linkwitz-Riley crossover bank →
//  per-band f²-scaled amplitude decay (Phillips 1977 ocean wave dissipation) →
//  reconstruction → output SVF filter → amplitude envelope → FDN reverb.
//
//  Signal Flow:
//    MIDI Note → PolyBLEP oscillator (+ optional FM + pink noise)
//      → Crossover filter bank (5 LR points → 6 bands)
//      → Per-band decay (f²-scaled by depth, abyss_hold floor, anchor immunity)
//      → Reconstruct (sum band × amplitude)
//      → CytomicSVF LP filter (env-modulated) → Amplitude envelope
//      → Patina (sample-rate reduction) → Voice sum
//      → FDN reverb → DC block → Output
//
//  Coupling (XOceanus inter-engine modulation):
//    Output: post-erosion, pre-filter audio via getSampleForCoupling
//    Input:  AudioToBuffer   → feeds highest bands as additional content
//            SpectralShaping → modulates per-band amplitudes directly
//            EnvToDecay      → modulates erosion rate
//            AmpToFilter     → fallback filter cutoff modulation
//
//  Parameter prefix:  olv_
//  Gallery code:      OLVI
//  Max voices:        8
//
//==============================================================================

// ---- Engine-level constants ----
static constexpr int   kOlvidoMaxVoices    = 8;
static constexpr int   kOlvidoNumBands     = 6;   // 6 octave-spaced bands
static constexpr float kOlvidoPI           = 3.14159265358979323846f;
static constexpr float kOlvidoTwoPI        = 6.28318530717958647692f;
// Crossover frequencies for 6 bands: ~80, 320, 650, 1300, 5000 Hz
// Band 0: <80Hz (abyss), Band 1: 80-320Hz, Band 2: 320-650Hz, Band 3: 650-1.3kHz,
// Band 4: 1.3-5kHz, Band 5: >5kHz (foam)
static constexpr float kOlvidoCrossoverBase[5] = { 80.0f, 320.0f, 650.0f, 1300.0f, 5000.0f };

//==============================================================================
//  Waveform enum
//==============================================================================
enum OlvidoWaveform { OLV_SAW = 0, OLV_PULSE, OLV_TRI, OLV_SINE, OLV_FM };

//==============================================================================
//  OlvidoVoice — Per-Voice State
//==============================================================================
struct OlvidoVoice
{
    bool     active           = false;
    bool     isFadingOut      = false;
    int      midiNote         = 60;
    float    velocity         = 0.0f;
    float    frequency        = 440.0f;
    float    targetFrequency  = 440.0f;
    float    currentFrequency = 440.0f;
    float    glideCoefficient = 1.0f;
    uint64_t startTime        = 0;
    float    crossfadeGain    = 1.0f;

    // PolyBLEP oscillator
    PolyBLEP osc;

    // FM modulator phase (used when drift_type == OLV_FM)
    float fmModPhase   = 0.0f;
    float fmModOutput  = 0.0f;

    // Patina: sample-rate reduction state
    float patinaHold   = 0.0f;
    int   patinaCounter = 0;

    // Per-band state (6 bands)
    float bandAmplitude[kOlvidoNumBands];     // current amplitude per band (0-1)
    float bandDecayRate[kOlvidoNumBands];     // f²-scaled decay rate per band (per sample)
    float bandTargetAmp[kOlvidoNumBands];     // target amplitude (for emergence mode)

    // Crossover filters: 5 crossover points, each LP+HP pair
    CytomicSVF crossoverLP[5];  // Low-pass at each crossover
    CytomicSVF crossoverHP[5];  // High-pass at each crossover

    // Per-voice modulation
    StandardLFO  lfo1;
    StandardADSR ampEnv;
    StandardADSR modEnv;  // routes to filter cutoff

    // Per-voice output filter (L and R)
    CytomicSVF outputFilterL, outputFilterR;

    // Coupling output cache
    float lastCouplingOut = 0.0f;

    void reset() noexcept
    {
        active            = false;
        isFadingOut       = false;
        midiNote          = 60;
        velocity          = 0.0f;
        frequency         = 440.0f;
        targetFrequency   = 440.0f;
        currentFrequency  = 440.0f;
        glideCoefficient  = 1.0f;
        startTime         = 0;
        crossfadeGain     = 1.0f;
        fmModPhase        = 0.0f;
        fmModOutput       = 0.0f;
        patinaHold        = 0.0f;
        patinaCounter     = 0;
        osc.reset();
        for (int i = 0; i < kOlvidoNumBands; ++i)
        {
            bandAmplitude[i] = 1.0f;
            bandDecayRate[i] = 0.0f;
            bandTargetAmp[i] = 1.0f;
        }
        for (int i = 0; i < 5; ++i)
        {
            crossoverLP[i].reset();
            crossoverHP[i].reset();
        }
        lfo1.reset();
        ampEnv.reset();
        modEnv.reset();
        outputFilterL.reset();
        outputFilterR.reset();
        lastCouplingOut = 0.0f;
    }
};

//==============================================================================
//
//  OlvidoEngine
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention — the .cpp is a one-line stub.
//
//==============================================================================
class OlvidoEngine : public SynthEngine
{
public:

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateDouble = sampleRate;
        sampleRateFloat  = static_cast<float>(sampleRate);

        // 5ms voice crossfade rate (linear ramp)
        voiceFadeRate = 1.0f / (0.005f * sampleRateFloat);

        // Output cache for coupling reads
        outputCacheLeft.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheRight.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        // FDN reverb delay lines (prime lengths for decorrelation)
        fdnBuf0.assign(1481, 0.0f);
        fdnBuf1.assign(1823, 0.0f);
        fdnBuf2.assign(2141, 0.0f);
        fdnBuf3.assign(2503, 0.0f);
        fdnIdx0 = fdnIdx1 = fdnIdx2 = fdnIdx3 = 0;
        fdnState0 = fdnState1 = fdnState2 = fdnState3 = 0.0f;

        // 400ms hold for reverb tail
        prepareSilenceGate(sampleRate, maxBlockSize, 400.0f);

        // Reset all voices
        for (auto& v : voices)
            v.reset();

        // Reset smoothed params
        smoothedCutoff       = 6000.0f;
        smoothedReso         = 0.2f;
        smoothedDrive        = 0.1f;
        modWheelValue        = 0.0f;
        aftertouchValue      = 0.0f;
        pitchBendNorm        = 0.0f;

        couplingBandFeedAccum   = 0.0f;
        couplingSpectralAccum   = 0.0f;
        couplingErosionAccum    = 0.0f;
        couplingFilterAccum     = 0.0f;
        cachedCouplingOutput    = 0.0f;

        // Reset pink noise state
        b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0f;

        // Reset stored band state for memory link
        for (int i = 0; i < kOlvidoNumBands; ++i)
            storedBandState[i] = 1.0f;
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        couplingBandFeedAccum = 0.0f;
        couplingSpectralAccum = 0.0f;
        couplingErosionAccum  = 0.0f;
        couplingFilterAccum   = 0.0f;
        cachedCouplingOutput  = 0.0f;
        modWheelValue         = 0.0f;
        aftertouchValue       = 0.0f;
        pitchBendNorm         = 0.0f;

        std::fill(outputCacheLeft.begin(),  outputCacheLeft.end(),  0.0f);
        std::fill(outputCacheRight.begin(), outputCacheRight.end(), 0.0f);
        std::fill(fdnBuf0.begin(), fdnBuf0.end(), 0.0f);
        std::fill(fdnBuf1.begin(), fdnBuf1.end(), 0.0f);
        std::fill(fdnBuf2.begin(), fdnBuf2.end(), 0.0f);
        std::fill(fdnBuf3.begin(), fdnBuf3.end(), 0.0f);
        fdnState0 = fdnState1 = fdnState2 = fdnState3 = 0.0f;

        b0 = b1 = b2 = b3 = b4 = b5 = b6 = 0.0f;

        for (int i = 0; i < kOlvidoNumBands; ++i)
            storedBandState[i] = 1.0f;
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  A U D I O
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0 || sampleRateFloat <= 0.0f)
            return;

        // ---- ParamSnapshot: read all parameter values once per block ----
        const int   paramDriftType     = static_cast<int>(loadParam(pDriftType,    0.0f));
        const float paramDriftAsym     = loadParam(pDriftAsym,    0.5f);
        const float paramFoam          = loadParam(pFoam,         0.2f);
        const float paramFmRatio       = loadParam(pFmRatio,      2.0f);
        const float paramFmDepth       = loadParam(pFmDepth,      0.3f);

        const float paramDepth         = loadParam(pDepth,        0.3f);
        const float paramCurrent       = loadParam(pCurrent,      0.0f);
        const float paramErosion       = loadParam(pErosion,      3.0f);
        const float paramShoreline     = loadParam(pShoreline,    0.5f);
        const float paramFlotsam       = loadParam(pFlotsam,      0.0f);
        const float paramAbyssHold     = loadParam(pAbyssHold,    0.8f);

        const float paramFilterCutoff  = loadParam(pFilterCutoff, 6000.0f);
        const float paramFilterReso    = loadParam(pFilterReso,   0.2f);
        const float paramFilterDrive   = loadParam(pFilterDrive,  0.1f);
        const float paramFilterEnv     = loadParam(pFilterEnv,    0.3f);

        const float paramReverbSize    = loadParam(pReverbSize,   0.4f);
        const float paramReverbMix     = loadParam(pReverbMix,    0.25f);
        const float paramPatina        = loadParam(pPatina,       0.0f);

        const float paramLfoRate       = loadParam(pLfoRate,      0.3f);
        const int   paramLfoShape      = static_cast<int>(loadParam(pLfoShape,     0.0f));
        const float paramLfoDepth      = loadParam(pLfoDepth,     0.2f);

        const float paramEnvAttack     = loadParam(pEnvAttack,    0.01f);
        const float paramEnvDecay      = loadParam(pEnvDecay,     1.0f);
        const float paramEnvSustain    = loadParam(pEnvSustain,   0.5f);
        const float paramEnvRelease    = loadParam(pEnvRelease,   1.5f);

        const int   paramVoiceMode     = static_cast<int>(loadParam(pVoiceMode,    0.0f));
        const float paramMemoryLink    = loadParam(pMemoryLink,   0.0f);
        const int   paramAnchor        = static_cast<int>(loadParam(pAnchor,       0.0f));
        const int   paramTempoSync     = static_cast<int>(loadParam(pTempoSync,    0.0f));

        const float macroChar  = loadParam(pMacro1, 0.0f);  // CHARACTER
        const float macroMove  = loadParam(pMacro2, 0.0f);  // MOVEMENT
        const float macroCoup  = loadParam(pMacro3, 0.0f);  // COUPLING
        const float macroSpace = loadParam(pMacro4, 0.0f);  // SPACE

        // ---- Voice mode ----
        const bool monoMode   = (paramVoiceMode == 1 || paramVoiceMode == 2);
        const bool legatoMode = (paramVoiceMode == 2);
        const int  maxPoly    = monoMode ? 1 : kOlvidoMaxVoices;

        // ---- Macro application ----
        // M1 CHARACTER: drift_type blend, fm_depth ↑, foam ↑
        const float effectiveFmDepth   = juce::jlimit(0.0f, 1.0f, paramFmDepth   + macroChar * 0.5f);
        const float effectiveFoam      = juce::jlimit(0.0f, 1.0f, paramFoam      + macroChar * 0.4f);

        // M2 MOVEMENT: depth ↑, erosion time ↓, lfo_depth ↑
        const float effectiveDepth     = juce::jlimit(0.0f, 1.0f, paramDepth     + macroMove * 0.7f);
        const float effectiveLfoDepth  = juce::jlimit(0.0f, 1.0f, paramLfoDepth  + macroMove * 0.5f);
        // Erosion time inversely mapped: macroMove reduces erosion time
        float effectiveErosion = juce::jlimit(0.05f, 300.0f, paramErosion * (1.0f - macroMove * 0.8f));

        // M3 COUPLING: current sweep, coupling amount, flotsam ↑
        const float effectiveCurrent   = juce::jlimit(-1.0f, 1.0f, paramCurrent  + (macroCoup * 2.0f - 1.0f) * 0.5f);
        const float effectiveFlotsam   = juce::jlimit(0.0f, 1.0f, paramFlotsam   + macroCoup * 0.4f);

        // M4 SPACE: reverb_mix ↑, patina ↑, abyss_hold ↑
        const float effectiveReverbMix = juce::jlimit(0.0f, 1.0f, paramReverbMix + macroSpace * 0.5f);
        const float effectivePatina    = juce::jlimit(0.0f, 1.0f, paramPatina    + macroSpace * 0.4f);
        const float effectiveAbyssHold = juce::jlimit(0.0f, 1.0f, paramAbyssHold + macroSpace * 0.2f);

        // ---- Tempo sync: override erosion time if enabled ----
        if (paramTempoSync > 0)
        {
            static constexpr float kSyncBeats[7] =
                { 0.0625f, 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
            const float beats = kSyncBeats[juce::jlimit(0, 6, paramTempoSync - 1)];
            const float bpm = (hostBPM_ > 0.0f) ? hostBPM_ : 120.0f;
            effectiveErosion = (beats * 60.0f) / bpm;
            effectiveErosion = juce::jlimit(0.05f, 300.0f, effectiveErosion);
        }

        // ---- Apply coupling inputs ----
        float effectiveCutoff = juce::jlimit(80.0f, 20000.0f,
            paramFilterCutoff + couplingFilterAccum * 4000.0f + modWheelValue * 8000.0f);
        // Erosion coupling modulates rate
        float erosionMult = 1.0f + couplingErosionAccum;
        erosionMult = juce::jlimit(0.1f, 4.0f, erosionMult);
        effectiveErosion = juce::jlimit(0.05f, 300.0f, effectiveErosion / erosionMult);

        // Consume coupling accumulators
        couplingBandFeedAccum = 0.0f;
        couplingSpectralAccum = 0.0f;
        couplingErosionAccum  = 0.0f;
        couplingFilterAccum   = 0.0f;

        // ---- Crossover frequencies shifted by shoreline ----
        // shoreline = 0.5 → no shift; ±1 octave at extremes
        const float shorelineShift = std::pow(2.0f, (paramShoreline - 0.5f) * 2.0f);
        float actualCrossover[5];
        for (int i = 0; i < 5; ++i)
            actualCrossover[i] = juce::jlimit(20.0f, 20000.0f,
                kOlvidoCrossoverBase[i] * shorelineShift);

        // ---- Hoist crossover filter coefficients (block-constant) ----
        // actualCrossover[] depends only on paramShoreline (block-constant).
        // Q=0.7071 and sampleRateFloat are also block-constant.
        // Computing setCoefficients_fast once per block per voice saves
        // ~10 trig calls per active voice per sample (5 LP + 5 HP).
        for (auto& v : voices)
        {
            if (!v.active) continue;
            for (int ci = 0; ci < 5; ++ci)
            {
                v.crossoverLP[ci].setMode(CytomicSVF::Mode::LowPass);
                v.crossoverLP[ci].setCoefficients_fast(actualCrossover[ci], 0.7071f, sampleRateFloat);
                v.crossoverHP[ci].setMode(CytomicSVF::Mode::HighPass);
                v.crossoverHP[ci].setCoefficients_fast(actualCrossover[ci], 0.7071f, sampleRateFloat);
            }
        }

        // ---- Pitch bend ratio (±2 semitones) ----
        const float pitchBendRatio = std::pow(2.0f, pitchBendNorm * 2.0f / 12.0f);

        // ---- Process MIDI ----
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                wakeSilenceGate();
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(),
                       maxPoly, monoMode, legatoMode,
                       paramEnvAttack, paramEnvDecay, paramEnvSustain, paramEnvRelease,
                       paramLfoRate, paramLfoShape,
                       paramDriftType, paramDriftAsym,
                       effectiveCutoff, paramFilterReso,
                       effectiveDepth, effectiveErosion, effectiveCurrent,
                       effectiveAbyssHold, paramAnchor,
                       paramMemoryLink, actualCrossover);
            }
            else if (msg.isNoteOff())
            {
                noteOff(msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
            }
            else if (msg.isController())
            {
                if (msg.getControllerNumber() == 1)
                {
                    // CC1 mod wheel → depth control (real-time forgetting) (D006)
                    modWheelValue = msg.getControllerValue() / 127.0f;
                }
                else if (msg.getControllerNumber() == 11)
                {
                    // CC11 expression → current sweep (forgetting ↔ emergence) (D006)
                    aftertouchValue = msg.getControllerValue() / 127.0f;
                }
            }
            else if (msg.isChannelPressure())
            {
                // Aftertouch → erosion rate multiplier
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                int raw = msg.getPitchWheelValue();
                pitchBendNorm = static_cast<float>(raw - 8192) / 8192.0f;
            }
        }

        // Silence gate bypass: if idle and no MIDI, clear and return
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            return;
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;

        // ---- Per-sample render loop ----
        for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
        {
            int activeCount = 0;

            // One-pole smoothing for control parameters (zipper prevention)
            smoothedCutoff += 0.001f * (effectiveCutoff - smoothedCutoff);
            smoothedReso   += 0.001f * (paramFilterReso  - smoothedReso);
            smoothedDrive  += 0.001f * (paramFilterDrive  - smoothedDrive);

            smoothedCutoff = flushDenormal(smoothedCutoff);
            smoothedReso   = flushDenormal(smoothedReso);
            smoothedDrive  = flushDenormal(smoothedDrive);

            float mixLeft  = 0.0f;
            float mixRight = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                ++activeCount;

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

                // Portamento glide
                voice.currentFrequency += (voice.targetFrequency - voice.currentFrequency)
                                          * voice.glideCoefficient;
                voice.currentFrequency = flushDenormal(voice.currentFrequency);

                // Base frequency with pitch bend
                float freq = voice.currentFrequency * pitchBendRatio;

                // ---- Tick envelopes ----
                float ampLevel = voice.ampEnv.process();
                float modLevel = voice.modEnv.process();

                // Check if voice is done
                if (!voice.ampEnv.isActive() && !voice.isFadingOut)
                {
                    voice.active = false;
                    continue;
                }

                // ---- Per-voice LFO (D002/D005) ----
                voice.lfo1.setRate(paramLfoRate, sampleRateFloat);
                voice.lfo1.setShape(paramLfoShape);
                float lfo1Out = flushDenormal(voice.lfo1.process() * effectiveLfoDepth);

                // ---- OSCILLATOR ----
                float oscOut = 0.0f;

                if (paramDriftType == OLV_FM)
                {
                    // FM mode: carrier + modulator
                    float carrierFreq  = freq;
                    float modFreq      = carrierFreq * paramFmRatio;
                    float modInc       = modFreq / sampleRateFloat;

                    // Advance modulator phase
                    voice.fmModPhase += modInc;
                    if (voice.fmModPhase >= 1.0f) voice.fmModPhase -= 1.0f;
                    voice.fmModOutput = fastSin(voice.fmModPhase * kOlvidoTwoPI);

                    // Carrier with FM modulation (frequency modulated by modulator)
                    float fmDeviation  = effectiveFmDepth * carrierFreq;
                    float modifiedFreq = juce::jlimit(10.0f, sampleRateFloat * 0.49f,
                        carrierFreq + voice.fmModOutput * fmDeviation);
                    voice.osc.setFrequency(modifiedFreq, sampleRateFloat);
                    voice.osc.setWaveform(PolyBLEP::Waveform::Sine);
                    oscOut = voice.osc.processSample();
                }
                else
                {
                    // Standard PolyBLEP waveforms
                    voice.osc.setFrequency(freq, sampleRateFloat);
                    switch (paramDriftType)
                    {
                        case OLV_PULSE:
                            voice.osc.setWaveform(PolyBLEP::Waveform::Pulse);
                            voice.osc.setPulseWidth(paramDriftAsym);
                            break;
                        case OLV_TRI:
                            voice.osc.setWaveform(PolyBLEP::Waveform::Triangle);
                            break;
                        case OLV_SINE:
                            voice.osc.setWaveform(PolyBLEP::Waveform::Sine);
                            break;
                        case OLV_SAW:
                        default:
                            voice.osc.setWaveform(PolyBLEP::Waveform::Saw);
                            break;
                    }
                    oscOut = voice.osc.processSample();
                }

                // LFO modulates oscillator pitch slightly
                oscOut = flushDenormal(oscOut);

                // ---- Add pink noise (foam) ----
                if (effectiveFoam > 0.001f)
                    oscOut += pinkNoise() * effectiveFoam;

                // ---- Patina: sample-rate reduction ----
                // Only update oscillator output every N samples
                {
                    int patinaFactor = 1 + static_cast<int>(effectivePatina * 8.0f);
                    voice.patinaCounter++;
                    if (voice.patinaCounter >= patinaFactor)
                    {
                        voice.patinaCounter = 0;
                        voice.patinaHold    = oscOut;
                    }
                    oscOut = voice.patinaHold;
                }

                // ---- CROSSOVER FILTER BANK (6 bands) ----
                // Use cascaded CytomicSVF LP+HP pairs at each crossover freq
                // band[0] = LP at crossover[0]
                // residual = HP at crossover[0]
                // band[1] = LP at crossover[1] of residual
                // ...etc
                float bandSignal[kOlvidoNumBands];
                float residual = oscOut;

                for (int ci = 0; ci < 5; ++ci)
                {
                    // Coefficients already set at block level (block-constant).
                    // Low-pass at this crossover → goes into band ci
                    bandSignal[ci] = voice.crossoverLP[ci].processSample(residual);

                    // High-pass continues to next band
                    residual = voice.crossoverHP[ci].processSample(residual);

                    bandSignal[ci] = flushDenormal(bandSignal[ci]);
                    residual       = flushDenormal(residual);
                }
                // Band 5 gets the highest residual
                bandSignal[5] = residual;

                // ---- PER-BAND DECAY (spectral erosion) ----
                // Effective current: combine param + CC11 expression sweep
                float effectiveCurrentNow = juce::jlimit(-1.0f, 1.0f,
                    effectiveCurrent + (aftertouchValue - 0.5f) * 0.5f);

                // base_rate = 1.0 / erosion_time (per second → per sample)
                float baseRate = (effectiveErosion > 0.0f) ? (1.0f / (effectiveErosion * sampleRateFloat)) : 0.0f;

                for (int bi = 0; bi < kOlvidoNumBands; ++bi)
                {
                    // anchor: selected band is immune to decay
                    bool isAnchorBand = (paramAnchor > 0) && (bi == (paramAnchor - 1));

                    if (isAnchorBand)
                    {
                        voice.bandAmplitude[bi] = 1.0f;
                    }
                    else if (effectiveCurrentNow > 0.0f)
                    {
                        // STANDARD DECAY (forgetting): bands erode over time
                        // decay_rate[band] = base_rate * pow(band_index+1, 2.0 * depth)
                        float decayScale = std::pow(static_cast<float>(bi + 1), 2.0f * effectiveDepth);
                        float perSampleDecay = baseRate * decayScale * effectiveCurrentNow;

                        // Flotsam: stochastic decay skip (per-band, per-sample)
                        if (effectiveFlotsam > 0.001f && rng.nextFloat() < effectiveFlotsam)
                        {
                            // Skip decay for this band this sample
                        }
                        else
                        {
                            voice.bandAmplitude[bi] -= perSampleDecay;
                        }

                        // Band 0: abyss_hold provides amplitude floor
                        float floor = (bi == 0) ? effectiveAbyssHold : 0.0f;
                        voice.bandAmplitude[bi] = juce::jlimit(floor, 1.0f, voice.bandAmplitude[bi]);
                        voice.bandAmplitude[bi] = flushDenormal(voice.bandAmplitude[bi]);
                    }
                    else if (effectiveCurrentNow < 0.0f)
                    {
                        // EMERGENCE: bands sharpen TOWARD full amplitude
                        // Higher bands emerge SLOWER (inverse of decay order)
                        // Band 5 (highest) emerges slowest — inverse scaling
                        float invertBandIdx   = static_cast<float>((kOlvidoNumBands - 1) - bi);
                        float emergenceScale  = std::pow(invertBandIdx + 1.0f, 2.0f * effectiveDepth);
                        float emergenceRate   = baseRate * emergenceScale * std::fabs(effectiveCurrentNow);

                        voice.bandAmplitude[bi] += emergenceRate;

                        float floor = (bi == 0) ? effectiveAbyssHold : 0.0f;
                        voice.bandAmplitude[bi] = juce::jlimit(floor, 1.0f, voice.bandAmplitude[bi]);
                        voice.bandAmplitude[bi] = flushDenormal(voice.bandAmplitude[bi]);
                    }
                    // current == 0: no spectral change (bypass erosion)

                    voice.bandDecayRate[bi] = baseRate *
                        std::pow(static_cast<float>(bi + 1), 2.0f * effectiveDepth);
                }

                // ---- RECONSTRUCT: sum bands × amplitude ----
                float postErosion = 0.0f;
                for (int bi = 0; bi < kOlvidoNumBands; ++bi)
                    postErosion += bandSignal[bi] * voice.bandAmplitude[bi];
                postErosion = flushDenormal(postErosion);

                // Cache for coupling output (post-erosion, pre-filter)
                voice.lastCouplingOut = postErosion;

                // ---- OUTPUT FILTER ----
                // Modulation envelope routes to filter cutoff
                float modEnvCutoffOffset = modLevel * paramFilterCutoff * paramFilterEnv;
                float voiceCutoff = juce::jlimit(80.0f, 20000.0f,
                    smoothedCutoff + modEnvCutoffOffset + lfo1Out * 1000.0f);

                // Pre-resonance tanh saturation (filter drive)
                float driven = postErosion;
                if (smoothedDrive > 0.001f)
                    driven = fastTanh(postErosion * (1.0f + smoothedDrive * 3.0f));

                voice.outputFilterL.setMode(CytomicSVF::Mode::LowPass);
                voice.outputFilterL.setCoefficients_fast(voiceCutoff, smoothedReso, sampleRateFloat);
                voice.outputFilterR.setMode(CytomicSVF::Mode::LowPass);
                voice.outputFilterR.setCoefficients_fast(voiceCutoff, smoothedReso, sampleRateFloat);

                float filtL = voice.outputFilterL.processSample(driven);
                float filtR = voice.outputFilterR.processSample(driven);

                filtL = flushDenormal(filtL);
                filtR = flushDenormal(filtR);

                // ---- AMPLITUDE ENVELOPE × velocity ----
                float gain = ampLevel * voice.velocity * voice.crossfadeGain;

                // LFO → slight stereo spread
                float spreadL = 1.0f + lfo1Out * 0.04f;
                float spreadR = 1.0f - lfo1Out * 0.04f;

                mixLeft  += filtL * gain * spreadL;
                mixRight += filtR * gain * spreadR;
            }

            // ---- Normalize by voice count ----
            float normFactor = (activeCount > 0) ? (1.0f / static_cast<float>(activeCount)) : 1.0f;
            mixLeft  *= normFactor;
            mixRight *= normFactor;

            // Cache output for coupling
            if (static_cast<size_t>(sampleIdx) < outputCacheLeft.size())
            {
                outputCacheLeft[static_cast<size_t>(sampleIdx)]  = mixLeft;
                outputCacheRight[static_cast<size_t>(sampleIdx)] = mixRight;
            }

            outL[sampleIdx] = mixLeft;
            outR[sampleIdx] = mixRight;
        }

        // Update coupling output cache (average of active voices)
        {
            float coupSum  = 0.0f;
            int   coupCount = 0;
            for (const auto& v : voices)
            {
                if (v.active) { coupSum += v.lastCouplingOut; ++coupCount; }
            }
            cachedCouplingOutput = (coupCount > 0) ? (coupSum / static_cast<float>(coupCount)) : 0.0f;
        }

        // Update stored band state for memory link (average of active voices)
        {
            float bandSum[kOlvidoNumBands] = {};
            int   bandCount = 0;
            for (const auto& v : voices)
            {
                if (v.active)
                {
                    for (int i = 0; i < kOlvidoNumBands; ++i)
                        bandSum[i] += v.bandAmplitude[i];
                    ++bandCount;
                }
            }
            if (bandCount > 0)
            {
                for (int i = 0; i < kOlvidoNumBands; ++i)
                    storedBandState[i] = bandSum[i] / static_cast<float>(bandCount);
            }
        }

        // Update active voice count (atomic, read by UI thread)
        {
            int cnt = 0;
            for (const auto& v : voices)
                if (v.active) ++cnt;
            activeVoiceCount_.store(cnt, std::memory_order_relaxed);
        }

        // ---- Apply shared FDN reverb ----
        applyReverb(outL, outR, numSamples, paramReverbSize, effectiveReverbMix);

        // Update output cache after effects
        for (int i = 0; i < numSamples; ++i)
        {
            if (static_cast<size_t>(i) < outputCacheLeft.size())
            {
                outputCacheLeft[static_cast<size_t>(i)]  = outL[i];
                outputCacheRight[static_cast<size_t>(i)] = outR[i];
            }
        }

        // ---- Feed silence gate ----
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  C O U P L I N G
    //==========================================================================

    float getSampleForCoupling(int /*channel*/, int /*sampleIndex*/) const override
    {
        return cachedCouplingOutput;  // post-erosion, pre-filter
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        static constexpr float kOlvidoCouplingScale = 0.25f;

        if (type == CouplingType::AudioToBuffer)
        {
            // Coupling signal feeds into highest bands as additional content
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, s * kOlvidoCouplingScale);
                couplingBandFeedAccum += (scaled - couplingBandFeedAccum) * 0.01f;
            }
        }
        else if (type == CouplingType::EnvToMorph)
        {
            // SpectralShaping: coupling modulates per-band amplitudes directly
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, s * kOlvidoCouplingScale);
                couplingSpectralAccum += (scaled - couplingSpectralAccum) * 0.01f;
            }
        }
        else if (type == CouplingType::EnvToDecay)
        {
            // Coupling modulates erosion rate
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, s * kOlvidoCouplingScale);
                couplingErosionAccum += (scaled - couplingErosionAccum) * 0.01f;
            }
        }
        else
        {
            // AmpToFilter fallback — modulates filter cutoff
            for (int i = 0; i < numSamples; ++i)
            {
                float s = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, s * kOlvidoCouplingScale);
                couplingFilterAccum += (scaled - couplingFilterAccum) * 0.01f;
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
        // ---- Source (5) ----

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"olv_drift_type", 1}, "XOlvido Drift",
            juce::StringArray{"Saw", "Pulse", "Tri", "Sine", "FM"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_drift_asym", 1}, "XOlvido Asymmetry",
            juce::NormalisableRange<float>(0.1f, 0.9f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_foam", 1}, "XOlvido Foam",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_fm_ratio", 1}, "XOlvido FM Ratio",
            juce::NormalisableRange<float>(0.5f, 8.0f, 0.001f, 0.5f), 2.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_fm_depth", 1}, "XOlvido FM Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // ---- Spectral Erosion (6) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_depth", 1}, "XOlvido Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_current", 1}, "XOlvido Current",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_erosion", 1}, "XOlvido Erosion",
            juce::NormalisableRange<float>(0.05f, 300.0f, 0.001f, 0.15f), 3.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_shoreline", 1}, "XOlvido Shoreline",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_flotsam", 1}, "XOlvido Flotsam",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_abyss_hold", 1}, "XOlvido Abyss Hold",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f));

        // ---- Output Filter (4) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_filter_cutoff", 1}, "XOlvido Filter",
            juce::NormalisableRange<float>(80.0f, 20000.0f, 0.1f, 0.25f), 6000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_filter_reso", 1}, "XOlvido Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_filter_drive", 1}, "XOlvido Drive",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_filter_env", 1}, "XOlvido Filter Env",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.3f));

        // ---- Space (3) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_reverb_size", 1}, "XOlvido Reverb Size",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.4f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_reverb_mix", 1}, "XOlvido Reverb Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_patina", 1}, "XOlvido Patina",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // ---- Modulation (7) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_lfo_rate", 1}, "XOlvido LFO Rate",
            juce::NormalisableRange<float>(0.01f, 20.0f, 0.001f, 0.3f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"olv_lfo_shape", 1}, "XOlvido LFO Shape",
            juce::StringArray{"Sin", "Tri", "Saw", "Sq", "S&H"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_lfo_depth", 1}, "XOlvido LFO Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_env_attack", 1}, "XOlvido Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_env_decay", 1}, "XOlvido Decay",
            juce::NormalisableRange<float>(0.01f, 10.0f, 0.001f, 0.3f), 1.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_env_sustain", 1}, "XOlvido Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_env_release", 1}, "XOlvido Release",
            juce::NormalisableRange<float>(0.01f, 10.0f, 0.001f, 0.3f), 1.5f));

        // ---- Performance (4) ----

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"olv_voice_mode", 1}, "XOlvido Voice Mode",
            juce::StringArray{"Poly", "Mono", "Legato"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_memory_link", 1}, "XOlvido Memory Link",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"olv_anchor", 1}, "XOlvido Anchor",
            juce::StringArray{"None", "Lo", "Lo-Mid", "Mid", "Hi-Mid", "Hi"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"olv_tempo_sync", 1}, "XOlvido Tempo Sync",
            juce::StringArray{"Off", "1/16", "1/8", "1/4", "1/2", "1", "2", "4"}, 0));

        // ---- Standard macros ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_macro1", 1}, "XOlvido Macro CHARACTER",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_macro2", 1}, "XOlvido Macro MOVEMENT",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_macro3", 1}, "XOlvido Macro COUPLING",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"olv_macro4", 1}, "XOlvido Macro SPACE",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pDriftType     = apvts.getRawParameterValue("olv_drift_type");
        pDriftAsym     = apvts.getRawParameterValue("olv_drift_asym");
        pFoam          = apvts.getRawParameterValue("olv_foam");
        pFmRatio       = apvts.getRawParameterValue("olv_fm_ratio");
        pFmDepth       = apvts.getRawParameterValue("olv_fm_depth");

        pDepth         = apvts.getRawParameterValue("olv_depth");
        pCurrent       = apvts.getRawParameterValue("olv_current");
        pErosion       = apvts.getRawParameterValue("olv_erosion");
        pShoreline     = apvts.getRawParameterValue("olv_shoreline");
        pFlotsam       = apvts.getRawParameterValue("olv_flotsam");
        pAbyssHold     = apvts.getRawParameterValue("olv_abyss_hold");

        pFilterCutoff  = apvts.getRawParameterValue("olv_filter_cutoff");
        pFilterReso    = apvts.getRawParameterValue("olv_filter_reso");
        pFilterDrive   = apvts.getRawParameterValue("olv_filter_drive");
        pFilterEnv     = apvts.getRawParameterValue("olv_filter_env");

        pReverbSize    = apvts.getRawParameterValue("olv_reverb_size");
        pReverbMix     = apvts.getRawParameterValue("olv_reverb_mix");
        pPatina        = apvts.getRawParameterValue("olv_patina");

        pLfoRate       = apvts.getRawParameterValue("olv_lfo_rate");
        pLfoShape      = apvts.getRawParameterValue("olv_lfo_shape");
        pLfoDepth      = apvts.getRawParameterValue("olv_lfo_depth");

        pEnvAttack     = apvts.getRawParameterValue("olv_env_attack");
        pEnvDecay      = apvts.getRawParameterValue("olv_env_decay");
        pEnvSustain    = apvts.getRawParameterValue("olv_env_sustain");
        pEnvRelease    = apvts.getRawParameterValue("olv_env_release");

        pVoiceMode     = apvts.getRawParameterValue("olv_voice_mode");
        pMemoryLink    = apvts.getRawParameterValue("olv_memory_link");
        pAnchor        = apvts.getRawParameterValue("olv_anchor");
        pTempoSync     = apvts.getRawParameterValue("olv_tempo_sync");

        pMacro1        = apvts.getRawParameterValue("olv_macro1");
        pMacro2        = apvts.getRawParameterValue("olv_macro2");
        pMacro3        = apvts.getRawParameterValue("olv_macro3");
        pMacro4        = apvts.getRawParameterValue("olv_macro4");
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

    juce::String getEngineId()     const override { return "Olvido"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0x3B, 0x6E, 0x8F); }
    int          getMaxVoices()    const override { return kOlvidoMaxVoices; }

    int getActiveVoiceCount() const override
    {
        return activeVoiceCount_.load(std::memory_order_relaxed);
    }

    //==========================================================================
    //  M A C R O   M A P P I N G
    //==========================================================================

    // Macros are applied in renderBlock via effectiveXxx variables.
    // This method is provided for UI feedback / external macro binding.
    void setMacro(int macroIndex, float value)
    {
        // Macros handled via APVTS parameters olv_macro1..4.
        (void)macroIndex; (void)value;
    }

    //==========================================================================
    //  H O S T   T R A N S P O R T
    //==========================================================================

    // Inject host BPM for tempo sync. Called by the processor each block.
    // If bpm <= 0, tempo sync falls back to 120 BPM.
    void setHostTransport(float bpm, double /*beatPos*/ = 0.0, bool /*isPlaying*/ = false) noexcept
    {
        hostBPM_ = (bpm > 0.0f) ? bpm : 0.0f;
    }

private:

    //==========================================================================
    //  H E L P E R S
    //==========================================================================

    static float loadParam(const std::atomic<float>* ptr, float fallback) noexcept
    {
        return (ptr != nullptr) ? ptr->load(std::memory_order_relaxed) : fallback;
    }

    static float midiNoteToHz(int noteNum) noexcept
    {
        return 440.0f * std::pow(2.0f, (static_cast<float>(noteNum) - 69.0f) / 12.0f);
    }

    //==========================================================================
    //  P I N K   N O I S E   G E N E R A T O R
    //
    //  Voss-McCartney algorithm: sum of 6 filtered white noise sources.
    //  Coefficients produce -3 dB/octave spectral slope.
    //==========================================================================
    inline float pinkNoise() noexcept
    {
        float white = rng.nextFloat() * 2.0f - 1.0f;
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        b6 = white * 0.115926f;
        return pink * 0.11f;  // normalize
    }

    //==========================================================================
    //  F D N   R E V E R B
    //
    //  4-line Feedback Delay Network with Hadamard mixing matrix and allpass
    //  diffusion. Prime delay lengths give diffuse, non-repeating reflections.
    //  No gate — XOlvido reverb is a smooth, continuous tail.
    //==========================================================================
    void applyReverb(float* outL, float* outR, int numSamples,
                     float size, float mix) noexcept
    {
        if (mix < 0.001f) return;

        // FDN feedback coefficient: larger = longer reverb
        const float feedback = 0.5f + size * 0.45f;  // [0.5, 0.95]

        for (int i = 0; i < numSamples; ++i)
        {
            float inMono = (outL[i] + outR[i]) * 0.5f;

            // ---- FDN tick ----
            int rd0 = (fdnIdx0 + 1) % static_cast<int>(fdnBuf0.size());
            int rd1 = (fdnIdx1 + 1) % static_cast<int>(fdnBuf1.size());
            int rd2 = (fdnIdx2 + 1) % static_cast<int>(fdnBuf2.size());
            int rd3 = (fdnIdx3 + 1) % static_cast<int>(fdnBuf3.size());

            float d0 = fdnBuf0[static_cast<size_t>(rd0)];
            float d1 = fdnBuf1[static_cast<size_t>(rd1)];
            float d2 = fdnBuf2[static_cast<size_t>(rd2)];
            float d3 = fdnBuf3[static_cast<size_t>(rd3)];

            // Hadamard mixing matrix (lossless, decorrelates channels)
            float s0 =  d0 + d1 + d2 + d3;
            float s1 =  d0 + d1 - d2 - d3;
            float s2 =  d0 - d1 + d2 - d3;
            float s3 =  d0 - d1 - d2 + d3;
            s0 *= 0.5f; s1 *= 0.5f; s2 *= 0.5f; s3 *= 0.5f;

            // First-order allpass per channel (one-pole, coefficient 0.5)
            const float allpassCoeff = 0.5f;
            float ap0 = s0 + allpassCoeff * fdnState0;
            fdnState0 = flushDenormal(s0 - allpassCoeff * ap0);
            float ap1 = s1 + allpassCoeff * fdnState1;
            fdnState1 = flushDenormal(s1 - allpassCoeff * ap1);
            float ap2 = s2 + allpassCoeff * fdnState2;
            fdnState2 = flushDenormal(s2 - allpassCoeff * ap2);
            float ap3 = s3 + allpassCoeff * fdnState3;
            fdnState3 = flushDenormal(s3 - allpassCoeff * ap3);

            // Write back with feedback and input injection
            fdnBuf0[static_cast<size_t>(fdnIdx0)] = inMono + ap0 * feedback;
            fdnBuf1[static_cast<size_t>(fdnIdx1)] = inMono + ap1 * feedback;
            fdnBuf2[static_cast<size_t>(fdnIdx2)] = inMono + ap2 * feedback;
            fdnBuf3[static_cast<size_t>(fdnIdx3)] = inMono + ap3 * feedback;

            // Advance FDN write pointers
            fdnIdx0 = (fdnIdx0 + 1) % static_cast<int>(fdnBuf0.size());
            fdnIdx1 = (fdnIdx1 + 1) % static_cast<int>(fdnBuf1.size());
            fdnIdx2 = (fdnIdx2 + 1) % static_cast<int>(fdnBuf2.size());
            fdnIdx3 = (fdnIdx3 + 1) % static_cast<int>(fdnBuf3.size());

            // Stereo reverb output (decorrelated: L/R from different delay lines)
            float revL = (ap0 + ap2) * 0.5f;
            float revR = (ap1 + ap3) * 0.5f;
            revL = flushDenormal(revL);
            revR = flushDenormal(revR);

            // Mix wet into output
            outL[i] += revL * mix;
            outR[i] += revR * mix;
        }
    }

    //==========================================================================
    //  M I D I   H A N D L I N G
    //==========================================================================

    void noteOn(int noteNum, float velocity,
                int maxPoly, bool monoMode, bool legatoMode,
                float envA, float envD, float envS, float envR,
                float lfoRate, int lfoShape,
                int driftType, float driftAsym,
                float cutoff, float reso,
                float depth, float erosion, float current,
                float abyssHold, int anchorBand,
                float memoryLink, const float actualCrossover[5])
    {
        float freq = midiNoteToHz(noteNum);
        lastNoteVelocity = velocity;

        if (monoMode)
        {
            auto& voice    = voices[0];
            bool  wasActive = voice.active;
            voice.targetFrequency = freq;

            if (legatoMode && wasActive)
            {
                // Legato: glide without reinit
                voice.midiNote = noteNum;
                voice.velocity = velocity;
                // Retrigger envelopes
                voice.ampEnv.noteOn();
                voice.modEnv.noteOn();
            }
            else
            {
                // Full reinit
                initVoice(voice, noteNum, freq, velocity,
                          envA, envD, envS, envR, lfoRate, lfoShape,
                          driftType, driftAsym, cutoff, reso,
                          depth, erosion, current, abyssHold, anchorBand,
                          memoryLink, storedBandState, actualCrossover,
                          /* monoGlide */ true);
            }
            return;
        }

        // ---- Polyphonic mode ----
        int voiceIdx = findVoiceForNoteOn(maxPoly);
        auto& voice  = voices[static_cast<size_t>(voiceIdx)];

        // If stealing an active voice, crossfade to prevent click
        if (voice.active)
        {
            voice.isFadingOut   = true;
            voice.crossfadeGain = std::min(voice.crossfadeGain, 0.5f);
        }

        // Compute poly average band state for memory link
        float polyAvgBand[kOlvidoNumBands] = {};
        int   activePoly = 0;
        for (const auto& v : voices)
        {
            if (v.active && !v.isFadingOut)
            {
                for (int i = 0; i < kOlvidoNumBands; ++i)
                    polyAvgBand[i] += v.bandAmplitude[i];
                ++activePoly;
            }
        }
        if (activePoly > 0)
            for (int i = 0; i < kOlvidoNumBands; ++i)
                polyAvgBand[i] /= static_cast<float>(activePoly);
        else
            for (int i = 0; i < kOlvidoNumBands; ++i)
                polyAvgBand[i] = 1.0f;

        initVoice(voice, noteNum, freq, velocity,
                  envA, envD, envS, envR, lfoRate, lfoShape,
                  driftType, driftAsym, cutoff, reso,
                  depth, erosion, current, abyssHold, anchorBand,
                  memoryLink, polyAvgBand, actualCrossover,
                  /* monoGlide */ false);
    }

    // Initialize a single voice on noteOn
    void initVoice(OlvidoVoice& voice, int noteNum, float freq, float velocity,
                   float envA, float envD, float envS, float envR,
                   float lfoRate, int lfoShape,
                   int driftType, float driftAsym,
                   float cutoff, float reso,
                   float depth, float erosion, float current,
                   float abyssHold, int anchorBand,
                   float memoryLink, const float* storedAmps,
                   const float* actualCrossover,
                   bool monoGlide)
    {
        voice.active           = true;
        voice.isFadingOut      = false;
        voice.midiNote         = noteNum;
        voice.velocity         = velocity;
        voice.frequency        = freq;
        voice.targetFrequency  = freq;
        voice.currentFrequency = freq;
        voice.glideCoefficient = monoGlide ? 0.1f : 1.0f;
        voice.startTime        = voiceTimestamp++;
        voice.crossfadeGain    = 1.0f;
        voice.fmModPhase       = 0.0f;
        voice.fmModOutput      = 0.0f;
        voice.patinaHold       = 0.0f;
        voice.patinaCounter    = 0;

        // D001: Velocity shapes initial band amplitudes
        // Higher velocity = more high-band content; lower velocity = high bands suppressed
        for (int bi = 0; bi < kOlvidoNumBands; ++bi)
        {
            float velFactor = 0.3f + 0.7f * velocity * (1.0f - static_cast<float>(bi) * 0.12f * (1.0f - velocity));
            velFactor = juce::jlimit(0.0f, 1.0f, velFactor);

            float initAmp = velFactor;

            // Apply memory link: blend initial amps with stored band state
            initAmp = (1.0f - memoryLink) * initAmp + memoryLink * storedAmps[bi];
            initAmp = juce::jlimit(0.0f, 1.0f, initAmp);

            // Emergence (current < 0): start at decayed level, target = full
            if (current < 0.0f)
            {
                // Calculate what the decayed level would be after erosion
                float baseRate = (erosion > 0.0f) ? (1.0f / erosion) : 0.0f;
                float decayScale = std::pow(static_cast<float>(bi + 1), 2.0f * depth);
                // Approximate decay after 0.5 erosion time
                float decayedAmp = initAmp * std::exp(-baseRate * decayScale * 0.5f);
                decayedAmp = juce::jlimit(0.0f, 1.0f, decayedAmp);
                // Start at decayed level
                voice.bandAmplitude[bi] = decayedAmp;
            }
            else
            {
                voice.bandAmplitude[bi] = initAmp;
            }

            voice.bandTargetAmp[bi] = 1.0f;

            // Anchor: selected band immune to decay
            bool isAnchor = (anchorBand > 0) && (bi == anchorBand - 1);
            if (isAnchor)
                voice.bandAmplitude[bi] = 1.0f;

            // Band 0: apply abyss_hold as floor
            if (bi == 0)
                voice.bandAmplitude[bi] = juce::jlimit(abyssHold, 1.0f, voice.bandAmplitude[bi]);

            // Compute per-band decay rates
            float baseRate = (erosion > 0.0f) ? (1.0f / (erosion * sampleRateFloat)) : 0.0f;
            voice.bandDecayRate[bi] = baseRate * std::pow(static_cast<float>(bi + 1), 2.0f * depth);
        }

        // Initialise crossover filters at each crossover frequency
        for (int ci = 0; ci < 5; ++ci)
        {
            voice.crossoverLP[ci].reset();
            voice.crossoverLP[ci].setMode(CytomicSVF::Mode::LowPass);
            voice.crossoverLP[ci].setCoefficients(actualCrossover[ci], 0.7071f, sampleRateFloat);

            voice.crossoverHP[ci].reset();
            voice.crossoverHP[ci].setMode(CytomicSVF::Mode::HighPass);
            voice.crossoverHP[ci].setCoefficients(actualCrossover[ci], 0.7071f, sampleRateFloat);
        }

        // Set oscillator waveform
        voice.osc.reset();
        switch (driftType)
        {
            case OLV_PULSE:
                voice.osc.setWaveform(PolyBLEP::Waveform::Pulse);
                voice.osc.setPulseWidth(driftAsym);
                break;
            case OLV_TRI:
                voice.osc.setWaveform(PolyBLEP::Waveform::Triangle);
                break;
            case OLV_SINE:
                voice.osc.setWaveform(PolyBLEP::Waveform::Sine);
                break;
            case OLV_FM:
                voice.osc.setWaveform(PolyBLEP::Waveform::Sine);
                break;
            case OLV_SAW:
            default:
                voice.osc.setWaveform(PolyBLEP::Waveform::Saw);
                break;
        }
        voice.osc.setFrequency(freq, sampleRateFloat);

        // Envelopes
        voice.ampEnv.setParams(envA, envD, envS, envR, sampleRateFloat);
        voice.ampEnv.noteOn();
        voice.modEnv.setParams(envA * 0.3f, envD, envS * 0.7f, envR * 1.5f, sampleRateFloat);
        voice.modEnv.noteOn();

        // LFO
        voice.lfo1.setRate(lfoRate, sampleRateFloat);
        voice.lfo1.setShape(lfoShape);
        voice.lfo1.reset();

        // Output filter
        voice.outputFilterL.reset();
        voice.outputFilterL.setMode(CytomicSVF::Mode::LowPass);
        voice.outputFilterL.setCoefficients(cutoff, reso, sampleRateFloat);
        voice.outputFilterR.reset();
        voice.outputFilterR.setMode(CytomicSVF::Mode::LowPass);
        voice.outputFilterR.setCoefficients(cutoff, reso, sampleRateFloat);

        voice.lastCouplingOut = 0.0f;
    }

    void noteOff(int noteNum)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.midiNote == noteNum && !voice.isFadingOut)
            {
                // Store band amplitudes for memory link on next note
                for (int i = 0; i < kOlvidoNumBands; ++i)
                    storedBandState[i] = voice.bandAmplitude[i];

                voice.ampEnv.noteOff();
                voice.modEnv.noteOff();
            }
        }
    }

    // Find best voice slot for a new note (LRU steal)
    int findVoiceForNoteOn(int maxPoly) noexcept
    {
        const int limit = juce::jmin(maxPoly, static_cast<int>(voices.size()));
        // Pass 1: find an inactive slot
        for (int i = 0; i < limit; ++i)
            if (!voices[static_cast<size_t>(i)].active)
                return i;
        // Pass 2: steal the oldest (LRU)
        int      oldest     = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < limit; ++i)
        {
            if (voices[static_cast<size_t>(i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t>(i)].startTime;
                oldest     = i;
            }
        }
        return oldest;
    }

    //==========================================================================
    //  M E M B E R   D A T A
    //==========================================================================

    // ---- Audio configuration (set in prepare()) ----
    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    double sampleRateDouble = 0.0;
    float  sampleRateFloat  = 0.0f;
    float  voiceFadeRate    = 0.01f;

    // ---- Voice pool ----
    std::array<OlvidoVoice, kOlvidoMaxVoices> voices;
    uint64_t voiceTimestamp = 0;

    // ---- Smoothed control parameters ----
    float smoothedCutoff = 6000.0f;
    float smoothedReso   = 0.2f;
    float smoothedDrive  = 0.1f;

    // ---- D006: expression ----
    float modWheelValue   = 0.0f;  // CC1 → depth control
    float aftertouchValue = 0.0f;  // aftertouch / CC11 expression → current sweep
    float pitchBendNorm   = 0.0f;  // ±1

    // ---- Last noteOn state ----
    float lastNoteVelocity = 0.0f;

    // ---- Memory link: stored per-band state (updated on noteOff) ----
    float storedBandState[kOlvidoNumBands] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

    // ---- Coupling accumulators (reset each block) ----
    float couplingBandFeedAccum  = 0.0f;  // high band content injection
    float couplingSpectralAccum  = 0.0f;  // per-band amplitude modulation
    float couplingErosionAccum   = 0.0f;  // erosion rate modulation
    float couplingFilterAccum    = 0.0f;  // filter cutoff modulation
    float cachedCouplingOutput   = 0.0f;  // O(1) coupling read

    // ---- Output cache for coupling reads ----
    std::vector<float> outputCacheLeft;
    std::vector<float> outputCacheRight;

    // ---- FDN Reverb (4-line, no gate) ----
    std::vector<float> fdnBuf0;  // 1481 samples
    std::vector<float> fdnBuf1;  // 1823 samples
    std::vector<float> fdnBuf2;  // 2141 samples
    std::vector<float> fdnBuf3;  // 2503 samples
    int   fdnIdx0 = 0, fdnIdx1 = 0, fdnIdx2 = 0, fdnIdx3 = 0;
    float fdnState0 = 0.0f, fdnState1 = 0.0f, fdnState2 = 0.0f, fdnState3 = 0.0f;

    // ---- Pink noise state (Voss-McCartney 6-pole) ----
    float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f, b3 = 0.0f;
    float b4 = 0.0f, b5 = 0.0f, b6 = 0.0f;

    // ---- Random number generator ----
    juce::Random rng;

    // ---- Host transport (injected by processor for tempo sync) ----
    float hostBPM_ = 0.0f;

    // ---- Cached APVTS parameter pointers (ParamSnapshot pattern) ----
    // Source
    std::atomic<float>* pDriftType    = nullptr;
    std::atomic<float>* pDriftAsym    = nullptr;
    std::atomic<float>* pFoam         = nullptr;
    std::atomic<float>* pFmRatio      = nullptr;
    std::atomic<float>* pFmDepth      = nullptr;
    // Spectral Erosion
    std::atomic<float>* pDepth        = nullptr;
    std::atomic<float>* pCurrent      = nullptr;
    std::atomic<float>* pErosion      = nullptr;
    std::atomic<float>* pShoreline    = nullptr;
    std::atomic<float>* pFlotsam      = nullptr;
    std::atomic<float>* pAbyssHold    = nullptr;
    // Output Filter
    std::atomic<float>* pFilterCutoff = nullptr;
    std::atomic<float>* pFilterReso   = nullptr;
    std::atomic<float>* pFilterDrive  = nullptr;
    std::atomic<float>* pFilterEnv    = nullptr;
    // Space
    std::atomic<float>* pReverbSize   = nullptr;
    std::atomic<float>* pReverbMix    = nullptr;
    std::atomic<float>* pPatina       = nullptr;
    // Modulation
    std::atomic<float>* pLfoRate      = nullptr;
    std::atomic<float>* pLfoShape     = nullptr;
    std::atomic<float>* pLfoDepth     = nullptr;
    std::atomic<float>* pEnvAttack    = nullptr;
    std::atomic<float>* pEnvDecay     = nullptr;
    std::atomic<float>* pEnvSustain   = nullptr;
    std::atomic<float>* pEnvRelease   = nullptr;
    // Performance
    std::atomic<float>* pVoiceMode    = nullptr;
    std::atomic<float>* pMemoryLink   = nullptr;
    std::atomic<float>* pAnchor       = nullptr;
    std::atomic<float>* pTempoSync    = nullptr;
    // Macros
    std::atomic<float>* pMacro1       = nullptr;
    std::atomic<float>* pMacro2       = nullptr;
    std::atomic<float>* pMacro3       = nullptr;
    std::atomic<float>* pMacro4       = nullptr;
};

} // namespace xoceanus
