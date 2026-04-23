// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

// =============================================================================
// OstraconEngine.h — XOstracon: Corpus-Buffer Synthesis (#88)
// "The Remembering Engine"
//
// A shared circular buffer records your playing onto virtual tape.
// Multiple read heads recall fragments with Mellotron-style degradation:
// oxide (bandwidth loss), flutter (pitch instability), print-through
// (ghost bleed from previous revolution). Memory as instrument.
//
// Architecture: Shared buffer (all voices write to one communal tape),
// per-voice read heads (1-4), per-head oxide filtering (distance-scaled).
//
// Physics: Mellotron M400 tape mechanics, Camras 1988 oxide shedding,
// Jorgensen tape frequency response, Pressnitzer & McAdams flutter threshold.
//
// Water Column: Surface Zone | Creature: Hermit Crab
// Accent: Shard Terracotta #C0785A
// Prefix: ostr_ | Gallery: OSTR
// =============================================================================

#include "../../Core/SynthEngine.h"
#include "../../DSP/PolyBLEP.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/Effects/DCBlocker.h"
#include "../../DSP/SRO/SilenceGate.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <vector>

namespace xoceanus
{

//==============================================================================
// Engine constants
//==============================================================================
static constexpr int   kOstraconMaxVoices  = 8;
static constexpr float kOstraconMaxReelSec = 8.0f;   // max reel_size parameter
static constexpr float kOstraconPI         = 3.14159265358979323846f;
static constexpr float kOstraconTwoPI      = 6.28318530717958647692f;

//==============================================================================
// Waveform enum
//==============================================================================
enum OstraconWaveform { OSTR_SAW = 0, OSTR_SQUARE, OSTR_TRI, OSTR_NOISE };

//==============================================================================
// Record mode enum
//==============================================================================
enum OstraconRecordMode { OSTR_ALWAYS = 0, OSTR_HELD, OSTR_TRIGGERED };

//==============================================================================
// LFO target enum
//==============================================================================
enum OstraconLfoTarget
{
    OSTR_LFO_FLUTTER = 0,
    OSTR_LFO_SPREAD,
    OSTR_LFO_FILTER,
    OSTR_LFO_OXIDE,
    OSTR_LFO_BIAS,
    OSTR_LFO_PRINT
};

//==============================================================================
// OstraconVoice — Per-Voice State
//==============================================================================
struct OstraconVoice
{
    bool     active    = false;
    bool     releasing = false;
    int      midiNote  = -1;
    float    velocity  = 0.0f;
    float    frequency = 440.0f;

    // Source oscillator
    PolyBLEP oscillator;

    // Read heads (max 4)
    static constexpr int kMaxHeads = 4;
    float readPos[kMaxHeads] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Per-head oxide LP filter
    CytomicSVF oxideFilter[kMaxHeads];

    // Flutter & wow phases (per-voice offset for natural variation)
    float flutterPhase = 0.0f;
    float wowPhase     = 0.0f;

    // Amplitude envelope
    StandardADSR ampEnv;

    // LFO (per-voice for phase diversity)
    StandardLFO lfo;

    // Output filter (L/R independent for stereo spread)
    CytomicSVF outputFilterL;
    CytomicSVF outputFilterR;

    // Glide
    GlideProcessor glide;

    // Coupling output cache
    float lastCouplingOut = 0.0f;

    // Voice stealing crossfade
    float crossfadeGain = 1.0f;
    float crossfadeRate = 0.0f;

    // Record mode: Triggered countdown (100ms at 48kHz = 4800 samples)
    int triggeredSamplesRemaining = 0;

    // Source output cached for write-to-buffer pass
    float sourceOutput = 0.0f;

    void reset() noexcept
    {
        active    = false;
        releasing = false;
        midiNote  = -1;
        velocity  = 0.0f;
        frequency = 440.0f;

        oscillator.reset();
        for (int h = 0; h < kMaxHeads; ++h)
        {
            readPos[h] = 0.0f;
            oxideFilter[h].reset();
            oxideFilter[h].setMode(CytomicSVF::Mode::LowPass);  // fixed — no need to set per-update
        }
        flutterPhase = 0.0f;
        wowPhase     = 0.0f;
        ampEnv.reset();
        lfo.reset();
        outputFilterL.reset();
        outputFilterR.reset();
        glide.reset();
        lastCouplingOut              = 0.0f;
        crossfadeGain                = 1.0f;
        crossfadeRate                = 0.0f;
        triggeredSamplesRemaining    = 0;
        sourceOutput                 = 0.0f;
    }
};

//==============================================================================
//
//  OstraconEngine
//
//  Corpus-buffer synthesis with Mellotron tape physics.
//  All DSP is inline per project convention — the .cpp is a one-line stub.
//
//==============================================================================
class OstraconEngine : public SynthEngine
{
public:

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        currentSampleRate = static_cast<float>(sampleRate);

        // 5ms crossfade rate for voice stealing
        voiceFadeRate = 1.0f / (0.005f * currentSampleRate);

        // Sample-rate-correct one-pole smoother coefficient (5ms, angular-frequency form).
        // Matches ParameterSmoother: 1 − exp(−2π / (T · sr))
        smootherCoeff = 1.0f - std::exp(-kOstraconTwoPI / (0.005f * currentSampleRate));

        // Allocate the shared circular reel buffer.
        // kOstraconMaxReelSec seconds of mono audio — no allocation during render.
        reelBufferSize = static_cast<int>(kOstraconMaxReelSec * currentSampleRate) + 4;
        reelBuffer.assign(static_cast<size_t>(reelBufferSize), 0.0f);
        writePos       = 0;
        reelSizeSamples = static_cast<int>(2.0f * currentSampleRate);  // default 2s

        // Output cache for coupling reads (stereo, one vector each)
        outputCacheL.assign(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.assign(static_cast<size_t>(maxBlockSize), 0.0f);

        // Reset silence gate — 400ms hold (tape has a natural tail)
        prepareSilenceGate(sampleRate, maxBlockSize, 400.0f);

        // Reset all voices
        for (auto& v : voices)
            v.reset();

        // Reset smoothed parameters
        smoothedCutoff  = 12000.0f;
        smoothedReso    = 0.15f;

        // Reset coupling accumulators
        couplingFilterAccum = 0.0f;
        couplingAudioAccum  = 0.0f;
        couplingMorphAccum  = 0.0f;

        // Reset cached coupling output
        cachedCouplingOutput[0] = 0.0f;
        cachedCouplingOutput[1] = 0.0f;

        // MIDI expression state
        modWheelValue   = 0.0f;
        expressionValue = 0.0f;
        aftertouchValue = 0.0f;
        pitchBendNorm   = 0.0f;

        // Noise LCG seed
        noiseState = 12345u;

        frozen = false;
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        // Clear the reel buffer — preset change wipes memory
        if (!reelBuffer.empty())
            std::fill(reelBuffer.begin(), reelBuffer.end(), 0.0f);
        writePos = 0;

        couplingFilterAccum = 0.0f;
        couplingAudioAccum  = 0.0f;
        couplingMorphAccum  = 0.0f;
        cachedCouplingOutput[0] = 0.0f;
        cachedCouplingOutput[1] = 0.0f;

        modWheelValue   = 0.0f;
        expressionValue = 0.0f;
        aftertouchValue = 0.0f;
        pitchBendNorm   = 0.0f;
        frozen          = false;

        smoothedCutoff = 12000.0f;
        smoothedReso   = 0.15f;

        if (!outputCacheL.empty())
            std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        if (!outputCacheR.empty())
            std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  A U D I O
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0 || currentSampleRate <= 0.0f)
            return;

        // ---- ParamSnapshot: read all parameter values ONCE per block ----
        const int   paramWaveform      = static_cast<int>(loadParam(pWaveform,     0.0f));
        const float paramTune          = loadParam(pTune,          0.0f);
        const float paramFine          = loadParam(pFine,          -3.0f);
        const float paramSourceMix     = loadParam(pSourceMix,     0.15f);

        const float paramReelSize      = loadParam(pReelSize,      2.0f);
        const int   paramHeadCount     = juce::jlimit(1, 4, static_cast<int>(loadParam(pHeadCount, 2.0f)));
        const float paramHeadSpread    = loadParam(pHeadSpread,    0.35f);
        const float paramBias          = loadParam(pBias,          0.5f);
        const int   paramRecordMode    = static_cast<int>(loadParam(pRecordMode,   0.0f));
        const float paramFreeze        = loadParam(pFreeze,        0.0f);

        const float paramOxide         = loadParam(pOxide,         0.25f);
        const float paramFlutter       = loadParam(pFlutter,       0.15f);
        const float paramFlutterRate   = loadParam(pFlutterRate,   4.2f);
        const float paramWow           = loadParam(pWow,           0.15f);
        const float paramPrint         = loadParam(pPrint,         0.1f);

        const float paramFilterCutoff  = loadParam(pFilterCutoff,  12000.0f);
        const float paramFilterReso    = loadParam(pFilterReso,    0.15f);
        const int   paramFilterType    = static_cast<int>(loadParam(pFilterType,   0.0f));

        const float paramAttack        = loadParam(pAttack,        0.02f);
        const float paramDecay         = loadParam(pDecay,         0.3f);
        const float paramSustain       = loadParam(pSustain,       0.8f);
        const float paramRelease       = loadParam(pRelease,       1.2f);

        const float paramLfoRate       = loadParam(pLfoRate,       0.15f);
        const int   paramLfoShape      = static_cast<int>(loadParam(pLfoShape,     0.0f));
        const float paramLfoDepth      = loadParam(pLfoDepth,      0.3f);
        const int   paramLfoTarget     = static_cast<int>(loadParam(pLfoTarget,    0.0f));

        const float paramSpeed         = loadParam(pSpeed,         1.0f);
        const float paramReverse       = loadParam(pReverse,       0.0f);

        const float macroChar  = loadParam(pMacro1, 0.0f);   // CHARACTER
        const float macroMove  = loadParam(pMacro2, 0.0f);   // MOVEMENT
        const float macroCoupl = loadParam(pMacro3, 0.0f);   // COUPLING
        const float macroSpace = loadParam(pMacro4, 0.0f);   // SPACE

        // ---- Macro application ----
        // M1 CHARACTER: oxide + source richness (waveform complexity via source_mix)
        const float effectiveOxide      = juce::jlimit(0.0f, 1.0f, paramOxide      + macroChar  * 0.5f);
        const float effectiveSourceMix  = juce::jlimit(0.0f, 1.0f, paramSourceMix  + macroChar  * 0.3f);

        // M2 MOVEMENT: flutter + head_spread + lfo_depth
        const float effectiveFlutter    = juce::jlimit(0.0f, 1.0f, paramFlutter    + macroMove  * 0.4f);
        const float effectiveSpread     = juce::jlimit(0.0f, 1.0f, paramHeadSpread + macroMove  * 0.3f);
        const float effectiveLfoDepth   = juce::jlimit(0.0f, 1.0f, paramLfoDepth   + macroMove  * 0.3f);

        // M3 COUPLING: bias (toward memory) + print
        const float effectiveBias       = juce::jlimit(0.0f, 1.0f, paramBias       - macroCoupl * 0.4f);
        const float effectivePrint      = juce::jlimit(0.0f, 1.0f, paramPrint      + macroCoupl * 0.3f);

        // M4 SPACE: wow + reel_size
        const float effectiveWow        = juce::jlimit(0.0f, 1.0f, paramWow        + macroSpace * 0.4f);
        const float effectiveReelSize   = juce::jlimit(0.5f, kOstraconMaxReelSec,
                                                         paramReelSize + macroSpace * 2.0f);

        // Derived effective values
        const float effectiveFlutterRate = juce::jlimit(2.0f, 8.0f, paramFlutterRate);
        const float effectiveSpeed       = juce::jlimit(0.85f, 1.15f, paramSpeed);
        const bool  effectiveReverse     = (paramReverse > 0.5f);

        // Freeze: >0.5 = frozen write head
        frozen = (paramFreeze > 0.5f);

        // Reel size in samples (update if changed)
        const int newReelSizeSamples = juce::jlimit(
            static_cast<int>(0.5f * currentSampleRate),
            reelBufferSize - 4,
            static_cast<int>(effectiveReelSize * currentSampleRate));

        if (newReelSizeSamples != reelSizeSamples)
            reelSizeSamples = newReelSizeSamples;

        // Expression/CC offsets — mod wheel routes to flutter (PASS 3), not filter
        float effectiveCutoff = juce::jlimit(80.0f, 20000.0f,
            paramFilterCutoff
            + couplingFilterAccum * 4000.0f);

        // Pitch bend ratio (±2 semitones) — fastPow2 avoids std::pow in block prep
        const float pitchBendRatio = fastPow2(pitchBendNorm * 2.0f / 12.0f);

        // Semitone + fine tune multiplier — fastPow2 avoids std::pow in block prep
        const float tuneMult = fastPow2((paramTune + paramFine * 0.01f) / 12.0f);

        // Aftertouch → effective oxide
        const float oxideWithAT = juce::jlimit(0.0f, 1.0f, effectiveOxide + aftertouchValue * 0.3f);

        // Capture coupling morph before it gets zeroed later
        const float couplingMorphIn = couplingMorphAccum;

        // Expression (CC11) + coupling morph → effective bias
        const float biasWithExpr = juce::jlimit(0.0f, 1.0f,
            effectiveBias + expressionValue * 0.5f - 0.25f + couplingMorphIn * 0.5f);

        // ---- Process MIDI ----
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                wakeSilenceGate();
                doNoteOn(msg.getNoteNumber(), msg.getFloatVelocity(),
                         paramAttack, paramDecay, paramSustain, paramRelease,
                         paramLfoRate, paramLfoShape,
                         paramHeadCount, effectiveSpread);
            }
            else if (msg.isNoteOff())
            {
                doNoteOff(msg.getNoteNumber());
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                reset();
            }
            else if (msg.isController())
            {
                const int cc  = msg.getControllerNumber();
                const int val = msg.getControllerValue();
                switch (cc)
                {
                    case 0x01: modWheelValue   = static_cast<float>(val) / 127.0f; break; // D006 flutter
                    case 0x0B: expressionValue = static_cast<float>(val) / 127.0f; break; // D006 bias
                    default: break;
                }
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue = static_cast<float>(msg.getChannelPressureValue()) / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                // Standard MIDI pitch wheel: [0, 16383], center = 8192.
                // Divide by 8191 (not 8192) to reach exactly +1.0 at full positive.
                int raw       = msg.getPitchWheelValue();
                pitchBendNorm = juce::jlimit(-1.0f, 1.0f,
                    static_cast<float>(raw - 8192) / 8191.0f);
            }
        }

        // Silence gate bypass — if silent and no MIDI events, zero buffer and skip
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            return;
        }

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;

        // Consume coupling accumulators — CAPTURE before zeroing (P25)
        const float couplingAudioIn  = couplingAudioAccum;
        // couplingMorphIn already captured above (before MIDI loop, needed for biasWithExpr)
        couplingFilterAccum = 0.0f;
        couplingAudioAccum  = 0.0f;
        couplingMorphAccum  = 0.0f;

        // Determine filter mode — applied to all voices once per block
        CytomicSVF::Mode filterMode = CytomicSVF::Mode::LowPass;
        if      (paramFilterType == 1) filterMode = CytomicSVF::Mode::HighPass;
        else if (paramFilterType == 2) filterMode = CytomicSVF::Mode::BandPass;

        // Propagate filter mode and waveform per-block (block-level params, not per-sample)
        {
            PolyBLEP::Waveform wfEnum = PolyBLEP::Waveform::Triangle;
            if      (paramWaveform == OSTR_SAW)    wfEnum = PolyBLEP::Waveform::Saw;
            else if (paramWaveform == OSTR_SQUARE)  wfEnum = PolyBLEP::Waveform::Square;

            for (auto& v : voices)
            {
                if (!v.active) continue;
                v.outputFilterL.setMode(filterMode);
                v.outputFilterR.setMode(filterMode);
                if (paramWaveform != OSTR_NOISE)
                    v.oscillator.setWaveform(wfEnum);
            }
        }

        // ---- Per-sample render loop ----
        for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
        {
            const bool updateFilter = ((sampleIdx & 15) == 0);
            int activeCount = 0;

            // One-pole smoothing — sample-rate-correct coefficient (5ms, see prepare())
            smoothedCutoff += smootherCoeff * (effectiveCutoff - smoothedCutoff);
            smoothedReso   += smootherCoeff * (paramFilterReso  - smoothedReso);
            smoothedCutoff  = flushDenormal(smoothedCutoff);
            smoothedReso    = flushDenormal(smoothedReso);

            float mixLeft  = 0.0f;
            float mixRight = 0.0f;

            // ---- PASS 1: Source oscillators → sourceOutput, crossfade fades ----
            for (auto& voice : voices)
            {
                if (!voice.active)
                {
                    voice.sourceOutput = 0.0f;
                    continue;
                }

                // Voice stealing crossfade
                if (voice.crossfadeRate > 0.0f)
                {
                    voice.crossfadeGain -= voice.crossfadeRate;
                    voice.crossfadeGain  = flushDenormal(voice.crossfadeGain);
                    if (voice.crossfadeGain <= 0.0f)
                    {
                        voice.crossfadeGain = 0.0f;
                        voice.active        = false;
                        voice.sourceOutput  = 0.0f;
                        continue;
                    }
                }

                ++activeCount;

                // Glide
                float freq = voice.glide.process();
                freq  = flushDenormal(freq);
                freq *= pitchBendRatio * tuneMult;
                freq  = juce::jlimit(20.0f, currentSampleRate * 0.49f, freq);
                voice.frequency = freq;

                // ---- Source oscillator ----
                // Waveform enum is a block-level param — set once, not per-sample.
                // setFrequency() must remain per-sample (freq changes via glide/pitch bend).
                float srcOut = 0.0f;
                if (paramWaveform == OSTR_NOISE)
                {
                    srcOut = nextNoise() * 0.7f;
                }
                else
                {
                    voice.oscillator.setFrequency(freq, currentSampleRate);
                    srcOut = voice.oscillator.processSample();
                }

                // Mix in noise (source_mix: 0=pure oscillator, 1=pure noise)
                if (effectiveSourceMix > 0.001f)
                    srcOut = srcOut * (1.0f - effectiveSourceMix)
                           + nextNoise() * effectiveSourceMix;

                srcOut = flushDenormal(srcOut);

                // Scale by velocity amplitude (D001: velocity shapes write gain)
                float velGain = voice.velocity * voice.velocity;  // vel² — harder = louder recording
                voice.sourceOutput = srcOut * velGain * voice.crossfadeGain;
            }

            // ---- PASS 2: Write to shared buffer ----
            {
                float writeSample     = couplingAudioIn;  // coupling audio also recorded
                int   writeVoiceCount = 0;

                for (auto& voice : voices)
                {
                    if (!voice.active) continue;

                    bool shouldWrite = true;
                    if (paramRecordMode == OSTR_HELD     && voice.releasing)             shouldWrite = false;
                    if (paramRecordMode == OSTR_TRIGGERED && voice.triggeredSamplesRemaining <= 0) shouldWrite = false;

                    if (shouldWrite)
                    {
                        writeSample += voice.sourceOutput;
                        ++writeVoiceCount;
                    }

                    if (voice.triggeredSamplesRemaining > 0)
                        --voice.triggeredSamplesRemaining;
                }

                if (!frozen)
                {
                    if (writeVoiceCount > 0)
                        writeSample /= static_cast<float>(writeVoiceCount);

                    // Tape saturation on write path: oxide increases drive (D003)
                    float writeDrive = 1.0f + oxideWithAT * 0.4f;
                    writeSample = fastTanh(writeSample * writeDrive);
                    writeSample = flushDenormal(writeSample);

                    reelBuffer[static_cast<size_t>(writePos)] = writeSample;
                }

                writePos = (writePos + 1) % reelSizeSamples;
            }

            // ---- PASS 3: Per-voice read heads → final output ----
            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // ---- LFO (D002/D005) ----
                if (updateFilter)
                {
                    voice.lfo.setRate(paramLfoRate, currentSampleRate);
                    voice.lfo.setShape(paramLfoShape);
                }
                float lfoOut = voice.lfo.process() * effectiveLfoDepth;
                lfoOut = flushDenormal(lfoOut);

                // ---- Flutter & Wow (D003: physics) ----
                // Mod wheel (CC1) → flutter depth
                float flutterDepth = juce::jlimit(0.0f, 1.0f, effectiveFlutter + modWheelValue * 0.3f);

                voice.flutterPhase += effectiveFlutterRate / currentSampleRate;
                if (voice.flutterPhase >= 1.0f) voice.flutterPhase -= 1.0f;
                float flutterMod = fastSin(voice.flutterPhase * kOstraconTwoPI) * flutterDepth * 0.005f;
                // max flutter ±0.5% pitch deviation at depth=1

                voice.wowPhase += 0.3f / currentSampleRate;   // ~0.3 Hz internal rate
                if (voice.wowPhase >= 1.0f) voice.wowPhase -= 1.0f;
                float wowMod = fastSin(voice.wowPhase * kOstraconTwoPI) * effectiveWow * 0.008f;
                // max wow ±0.8% pitch deviation at depth=1

                // LFO → flutter if target == OSTR_LFO_FLUTTER
                if (paramLfoTarget == OSTR_LFO_FLUTTER)
                    flutterMod += lfoOut * 0.003f;

                // Combined read speed multiplier
                float readSpeed = effectiveSpeed * (1.0f + flutterMod + wowMod);
                if (effectiveReverse) readSpeed = -readSpeed;

                // LFO → spread
                float spreadMod = 0.0f;
                if (paramLfoTarget == OSTR_LFO_SPREAD)
                    spreadMod = lfoOut * 0.1f;

                // LFO → oxide
                float oxideMod = 0.0f;
                if (paramLfoTarget == OSTR_LFO_OXIDE)
                    oxideMod = lfoOut * 0.3f;

                // LFO → bias
                float biasMod = 0.0f;
                if (paramLfoTarget == OSTR_LFO_BIAS)
                    biasMod = lfoOut * 0.3f;

                // LFO → print
                float printMod = 0.0f;
                if (paramLfoTarget == OSTR_LFO_PRINT)
                    printMod = lfoOut * 0.2f;

                // LFO → filter
                float filterMod = 0.0f;
                if (paramLfoTarget == OSTR_LFO_FILTER)
                    filterMod = lfoOut * 4000.0f;

                const float effectiveOxideVoice  = juce::jlimit(0.0f, 1.0f, oxideWithAT + oxideMod);
                const float effectiveBiasVoice   = juce::jlimit(0.0f, 1.0f, biasWithExpr + biasMod);
                const float effectivePrintVoice  = juce::jlimit(0.0f, 1.0f, effectivePrint + printMod);
                // D001: velocity shapes timbre — harder hits read brighter (less oxide attenuation)
                // velTimbre ∈ [-0.25, 0]: soft=−0.25 (more oxide/darker), hard=0 (no attenuation)
                const float velTimbre  = (voice.velocity - 1.0f) * 0.25f;
                const float finalCutoff = juce::jlimit(80.0f, 20000.0f,
                    smoothedCutoff + filterMod + velTimbre * 4000.0f);

                // ---- Amplitude envelope ----
                float ampLevel = voice.ampEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active    = false;
                    voice.releasing = false;
                    continue;
                }

                // ---- Read heads accumulation ----
                float headMixL  = 0.0f;
                float headMixR  = 0.0f;
                float printMixL = 0.0f;
                float printMixR = 0.0f;

                // Print-through level (quadratic: more control at low values)
                float printLevel = effectivePrintVoice * effectivePrintVoice;

                for (int h = 0; h < paramHeadCount; ++h)
                {
                    // Advance read position — spreadMod makes heads diverge/converge
                    float headCenter = 0.5f * static_cast<float>(paramHeadCount - 1);
                    float headReadSpeed = readSpeed * (1.0f + spreadMod * (static_cast<float>(h) - headCenter));
                    voice.readPos[h] += headReadSpeed;

                    // Wrap within reel
                    while (voice.readPos[h] >= static_cast<float>(reelSizeSamples))
                        voice.readPos[h] -= static_cast<float>(reelSizeSamples);
                    while (voice.readPos[h] < 0.0f)
                        voice.readPos[h] += static_cast<float>(reelSizeSamples);

                    // Cubic Hermite read from buffer
                    float rawSample = readBufferHermite(voice.readPos[h]);
                    rawSample = flushDenormal(rawSample);

                    // Per-head oxide LP filter.
                    // Cutoff scales with head distance from write head:
                    // closer heads (smaller distance) = brighter; older = more degraded.
                    {
                        float headDist = voice.readPos[h] - static_cast<float>(writePos);
                        if (headDist < 0.0f) headDist += static_cast<float>(reelSizeSamples);
                        // Normalized 0-1 (0 = right at write head, 1 = one full revolution behind)
                        float normDist = headDist / static_cast<float>(reelSizeSamples);

                        float oxideDepth = effectiveOxideVoice * (1.0f + normDist * 0.5f);
                        if (updateFilter)
                        {
                            // setMode(LowPass) omitted here — set once in reset() / doNoteOn()
                            float oxideCutoff = 20000.0f * fastExp(-oxideDepth * 4.0f);
                            oxideCutoff = juce::jlimit(80.0f, 20000.0f, oxideCutoff);
                            voice.oxideFilter[h].setMode(CytomicSVF::Mode::LowPass);
                            voice.oxideFilter[h].setCoefficients_fast(oxideCutoff, 0.3f, currentSampleRate);
                        }
                        rawSample = voice.oxideFilter[h].processSample(rawSample);
                        rawSample = flushDenormal(rawSample);
                    }

                    // Print-through: ghost from PREVIOUS revolution
                    float printPos = voice.readPos[h] - static_cast<float>(reelSizeSamples);
                    if (printPos < 0.0f) printPos += static_cast<float>(reelBufferSize);
                    float printSample = readBufferHermite(printPos) * printLevel;
                    printSample = flushDenormal(printSample);

                    // Stereo spread: even heads → L, odd heads → R
                    // Phase offset between heads creates natural stereo image
                    if ((h & 1) == 0)
                    {
                        headMixL  += rawSample;
                        printMixL += printSample;
                    }
                    else
                    {
                        headMixR  += rawSample;
                        printMixR += printSample;
                    }
                }

                // Normalize head mix per-channel to preserve equal power across head counts.
                // With odd head counts (1 or 3), L gets more heads than R. Normalize
                // each channel by its actual head count to prevent energy imbalance.
                const int headsL = (paramHeadCount + 1) / 2;   // ceil(n/2) — even indices → L
                const int headsR = paramHeadCount / 2;          // floor(n/2) — odd indices → R

                if (headsL > 0) { headMixL /= static_cast<float>(headsL); printMixL /= static_cast<float>(headsL); }
                if (headsR > 0) { headMixR /= static_cast<float>(headsR); printMixR /= static_cast<float>(headsR); }

                // If mono heads (all land in L), copy to R for true mono-center
                if (paramHeadCount == 1)
                {
                    headMixR  = headMixL;
                    printMixR = printMixL;
                }

                // Include print-through
                float memoryL = headMixL + printMixL;
                float memoryR = headMixR + printMixR;

                // ---- Bias crossfade: live source ↔ memory ----
                // At bias=0: pure live oscillator. At bias=1: pure memory.
                float liveL = voice.sourceOutput;
                float liveR = voice.sourceOutput;
                float outSampleL = liveL * (1.0f - effectiveBiasVoice) + memoryL * effectiveBiasVoice;
                float outSampleR = liveR * (1.0f - effectiveBiasVoice) + memoryR * effectiveBiasVoice;

                outSampleL = flushDenormal(outSampleL);
                outSampleR = flushDenormal(outSampleR);

                // ---- Output filter (per-voice L/R) — coeff refresh decimated ----
                // setMode() is applied once per block above; only coefficients need per-16 refresh.
                if (updateFilter)
                {
                    voice.outputFilterL.setMode(filterMode);
                    voice.outputFilterR.setMode(filterMode);
                    voice.outputFilterL.setCoefficients_fast(finalCutoff, smoothedReso, currentSampleRate);
                    voice.outputFilterR.setCoefficients_fast(finalCutoff, smoothedReso, currentSampleRate);
                }

                outSampleL = voice.outputFilterL.processSample(outSampleL);
                outSampleR = voice.outputFilterR.processSample(outSampleR);
                outSampleL = flushDenormal(outSampleL);
                outSampleR = flushDenormal(outSampleR);

                // ---- Amplitude envelope × velocity ----
                float envGain = ampLevel * voice.crossfadeGain;
                outSampleL *= envGain;
                outSampleR *= envGain;

                // Cache for coupling
                voice.lastCouplingOut = (outSampleL + outSampleR) * 0.5f;

                mixLeft  += outSampleL;
                mixRight += outSampleR;
            }

            // Write to output buffer
            outL[sampleIdx] = mixLeft;
            outR[sampleIdx] = mixRight;

            // Cache for coupling reads
            if (static_cast<size_t>(sampleIdx) < outputCacheL.size())
            {
                outputCacheL[static_cast<size_t>(sampleIdx)] = mixLeft;
                outputCacheR[static_cast<size_t>(sampleIdx)] = mixRight;
            }

            // Update cached coupling output (post-filter, both channels)
            cachedCouplingOutput[0] = mixLeft;
            cachedCouplingOutput[1] = mixRight;
        }

        // Update active voice count for UI
        {
            int count = 0;
            for (const auto& v : voices)
                if (v.active) ++count;
            activeVoiceCount_.store(count, std::memory_order_relaxed);
        }

        // Feed silence gate
        analyzeForSilenceGate(buffer, numSamples);
    }

    //==========================================================================
    //  S Y N T H   E N G I N E   I N T E R F A C E  —  C O U P L I N G
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return cachedCouplingOutput[channel & 1];
    }

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        static constexpr float kCouplingScale = 0.25f;

        if (type == CouplingType::AudioToBuffer)
        {
            // Coupling audio is injected into the write path (recorded to tape)
            for (int i = 0; i < numSamples; ++i)
            {
                float src    = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, src * kCouplingScale);
                couplingAudioAccum += 0.01f * (scaled - couplingAudioAccum);
            }
        }
        else if (type == CouplingType::EnvToMorph)
        {
            // Morphs the bias parameter (live ↔ memory crossfade)
            for (int i = 0; i < numSamples; ++i)
            {
                float src    = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, src * kCouplingScale);
                couplingMorphAccum += 0.01f * (scaled - couplingMorphAccum);
            }
        }
        else
        {
            // AmpToFilter fallback — modulates output filter cutoff
            for (int i = 0; i < numSamples; ++i)
            {
                float src    = (sourceBuffer != nullptr ? sourceBuffer[i] : 0.0f) * amount;
                float scaled = juce::jlimit(-1.0f, 1.0f, src * kCouplingScale);
                couplingFilterAccum += 0.01f * (scaled - couplingFilterAccum);
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
        // ---- Source (4) ----

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ostr_waveform", 1}, "XOstracon Waveform",
            juce::StringArray{"Saw", "Square", "Triangle", "Noise"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"ostr_tune", 1}, "XOstracon Tune",
            -24, 24, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_fine", 1}, "XOstracon Fine",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), -3.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_source_mix", 1}, "XOstracon Source Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

        // ---- Reel (6) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_reel_size", 1}, "XOstracon Reel Size",
            juce::NormalisableRange<float>(0.5f, 8.0f, 0.001f, 0.5f), 2.0f));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"ostr_head_count", 1}, "XOstracon Heads",
            1, 4, 2));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_head_spread", 1}, "XOstracon Spread",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_bias", 1}, "XOstracon Bias",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ostr_record_mode", 1}, "XOstracon Record Mode",
            juce::StringArray{"Always", "Held", "Triggered"}, 0));

        // Freeze is a binary toggle — AudioParameterBool avoids misleading float range.
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"ostr_freeze", 1}, "XOstracon Freeze", false));

        // ---- Tape Physics (5) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_oxide", 1}, "XOstracon Oxide",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_flutter", 1}, "XOstracon Flutter",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_flutter_rate", 1}, "XOstracon Flutter Rate",
            juce::NormalisableRange<float>(2.0f, 8.0f, 0.01f), 4.2f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_wow", 1}, "XOstracon Wow",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_print", 1}, "XOstracon Print",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));

        // ---- Output Filter (3) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_filter_cutoff", 1}, "XOstracon Filter",
            juce::NormalisableRange<float>(80.0f, 20000.0f, 0.1f, 0.3f), 12000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_filter_reso", 1}, "XOstracon Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.15f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ostr_filter_type", 1}, "XOstracon Filter Type",
            juce::StringArray{"LP", "HP", "BP"}, 0));

        // ---- Envelope (4) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_attack", 1}, "XOstracon Attack",
            juce::NormalisableRange<float>(0.001f, 5.0f, 0.001f, 0.4f), 0.02f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_decay", 1}, "XOstracon Decay",
            juce::NormalisableRange<float>(0.01f, 10.0f, 0.001f, 0.4f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_sustain", 1}, "XOstracon Sustain",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_release", 1}, "XOstracon Release",
            juce::NormalisableRange<float>(0.01f, 10.0f, 0.001f, 0.4f), 1.2f));

        // ---- Modulation (4) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_lfo_rate", 1}, "XOstracon LFO Rate",
            juce::NormalisableRange<float>(0.005f, 12.0f, 0.001f, 0.35f), 0.15f));

        // StandardLFO shape enum: Sine=0, Triangle=1, Saw=2, Square=3, S&H=4.
        // Expose all 5 shapes so indices match 1:1 — previously "S&H" was at index 3
        // which silently mapped to Square, making S&H inaccessible (D004 violation).
        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ostr_lfo_shape", 1}, "XOstracon LFO Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_lfo_depth", 1}, "XOstracon LFO Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"ostr_lfo_target", 1}, "XOstracon LFO Target",
            juce::StringArray{"Flutter", "Spread", "Filter", "Oxide", "Bias", "Print"}, 0));

        // ---- Performance (2) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_speed", 1}, "XOstracon Speed",
            juce::NormalisableRange<float>(0.85f, 1.15f, 0.001f), 1.0f));

        // Reverse is a binary toggle — AudioParameterBool avoids misleading float range.
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"ostr_reverse", 1}, "XOstracon Reverse", false));

        // ---- Standard macros (4) ----

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_macro1", 1}, "XOstracon Macro CHARACTER",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_macro2", 1}, "XOstracon Macro MOVEMENT",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_macro3", 1}, "XOstracon Macro COUPLING",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"ostr_macro4", 1}, "XOstracon Macro SPACE",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pWaveform     = apvts.getRawParameterValue("ostr_waveform");
        pTune         = apvts.getRawParameterValue("ostr_tune");
        pFine         = apvts.getRawParameterValue("ostr_fine");
        pSourceMix    = apvts.getRawParameterValue("ostr_source_mix");

        pReelSize     = apvts.getRawParameterValue("ostr_reel_size");
        pHeadCount    = apvts.getRawParameterValue("ostr_head_count");
        pHeadSpread   = apvts.getRawParameterValue("ostr_head_spread");
        pBias         = apvts.getRawParameterValue("ostr_bias");
        pRecordMode   = apvts.getRawParameterValue("ostr_record_mode");
        pFreeze       = apvts.getRawParameterValue("ostr_freeze");

        pOxide        = apvts.getRawParameterValue("ostr_oxide");
        pFlutter      = apvts.getRawParameterValue("ostr_flutter");
        pFlutterRate  = apvts.getRawParameterValue("ostr_flutter_rate");
        pWow          = apvts.getRawParameterValue("ostr_wow");
        pPrint        = apvts.getRawParameterValue("ostr_print");

        pFilterCutoff = apvts.getRawParameterValue("ostr_filter_cutoff");
        pFilterReso   = apvts.getRawParameterValue("ostr_filter_reso");
        pFilterType   = apvts.getRawParameterValue("ostr_filter_type");

        pAttack       = apvts.getRawParameterValue("ostr_attack");
        pDecay        = apvts.getRawParameterValue("ostr_decay");
        pSustain      = apvts.getRawParameterValue("ostr_sustain");
        pRelease      = apvts.getRawParameterValue("ostr_release");

        pLfoRate      = apvts.getRawParameterValue("ostr_lfo_rate");
        pLfoShape     = apvts.getRawParameterValue("ostr_lfo_shape");
        pLfoDepth     = apvts.getRawParameterValue("ostr_lfo_depth");
        pLfoTarget    = apvts.getRawParameterValue("ostr_lfo_target");

        pSpeed        = apvts.getRawParameterValue("ostr_speed");
        pReverse      = apvts.getRawParameterValue("ostr_reverse");

        pMacro1       = apvts.getRawParameterValue("ostr_macro1");
        pMacro2       = apvts.getRawParameterValue("ostr_macro2");
        pMacro3       = apvts.getRawParameterValue("ostr_macro3");
        pMacro4       = apvts.getRawParameterValue("ostr_macro4");
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

    juce::String getEngineId()     const override { return "Ostracon"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xC0, 0x78, 0x5A); }
    int          getMaxVoices()    const override { return kOstraconMaxVoices; }

    int getActiveVoiceCount() const override
    {
        return activeVoiceCount_.load(std::memory_order_relaxed);
    }

    //==========================================================================
    //  M A C R O   M A P P I N G
    //==========================================================================

    void setMacro(int /*macroIndex*/, float /*value*/) noexcept
    {
        // Macros handled via APVTS parameters ostr_macro1..4.
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
        return 440.0f * fastPow2((static_cast<float>(noteNum) - 69.0f) / 12.0f);
    }

    //--------------------------------------------------------------------------
    // Cubic Hermite interpolation from the shared reel buffer.
    // Produces smooth sub-sample reads without the tonal coloration of linear
    // interpolation, critical for realistic tape-head simulation.
    //--------------------------------------------------------------------------
    inline float readBufferHermite(float pos) const noexcept
    {
        if (reelBufferSize < 4) return 0.0f;
        const int sz = reelSizeSamples;

        int   i1   = static_cast<int>(pos);
        float frac = pos - static_cast<float>(i1);

        i1       = ((i1 % sz) + sz) % sz;
        int i0   = (i1 - 1 + sz) % sz;
        int i2   = (i1 + 1)       % sz;
        int i3   = (i1 + 2)       % sz;

        float y0 = reelBuffer[static_cast<size_t>(i0)];
        float y1 = reelBuffer[static_cast<size_t>(i1)];
        float y2 = reelBuffer[static_cast<size_t>(i2)];
        float y3 = reelBuffer[static_cast<size_t>(i3)];

        // Catmull-Rom / Hermite coefficients
        float c0 =  y1;
        float c1 =  0.5f * (y2 - y0);
        float c2 =  y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 =  0.5f * (y3 - y0) + 1.5f * (y1 - y2);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    //--------------------------------------------------------------------------
    // Simple LCG noise generator — fast, adequate for tape noise / source mix.
    //--------------------------------------------------------------------------
    inline float nextNoise() noexcept
    {
        noiseState = noiseState * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(noiseState)) / 2147483648.0f;
    }

    //==========================================================================
    //  M I D I   H A N D L I N G
    //==========================================================================

    void doNoteOn(int noteNum, float velocity,
                  float envA, float envD, float envS, float envR,
                  float lfoRate, int lfoShape,
                  int headCount, float headSpread) noexcept
    {
        float freq = midiNoteToHz(noteNum);

        // Find a free voice (simple oldest-first LRU stealing)
        int voiceIdx = findFreeVoice();
        auto& voice  = voices[static_cast<size_t>(voiceIdx)];

        // If stealing an active voice, preserve crossfade state for a brief fade-out.
        // We clone the voice DSP state into a spare slot here if available; otherwise
        // hard-kill at low gain to avoid a click. The unconditional reset() that
        // previously followed immediately undid any crossfade setup (bug).
        if (voice.active && voice.crossfadeGain >= 0.1f)
        {
            // Attempt to hand-off fade to a free second slot so this slot can accept
            // the new note cleanly. If no free slot exists, accept the small click.
            int freeSlot = -1;
            for (int fi = 0; fi < kOstraconMaxVoices; ++fi)
            {
                if (!voices[static_cast<size_t>(fi)].active)
                {
                    freeSlot = fi;
                    break;
                }
            }
            if (freeSlot >= 0)
            {
                // Move stolen voice's state into the free slot for a 5ms fade-out
                voices[static_cast<size_t>(freeSlot)] = voice;
                voices[static_cast<size_t>(freeSlot)].crossfadeRate = voiceFadeRate;
            }
            // Now the original slot is free to be reset for the new note
        }

        voice.reset();
        voice.active    = true;
        voice.releasing = false;
        voice.midiNote  = noteNum;
        voice.velocity  = velocity;
        voice.frequency = freq;

        // Glide: snap to target (single voice)
        voice.glide.setTargetOrSnap(freq);

        // Amp envelope
        voice.ampEnv.setParams(envA, envD, envS, envR, currentSampleRate);
        voice.ampEnv.noteOn();

        // LFO — random phase offset per voice for ensemble texture
        {
            // Use voice index to spread phases 0, 0.125, 0.25 ...
            float phaseOff = static_cast<float>(voiceIdx) / static_cast<float>(kOstraconMaxVoices);
            voice.lfo.setPhaseOffset(phaseOff);
            voice.lfo.setRate(lfoRate, currentSampleRate);
            voice.lfo.setShape(lfoShape);
        }

        // Flutter & wow: offset phases for natural variation across voices
        voice.flutterPhase = static_cast<float>(voiceIdx) * 0.137f;
        voice.wowPhase     = static_cast<float>(voiceIdx) * 0.251f;

        // Triggered record mode: 100ms countdown
        voice.triggeredSamplesRemaining = static_cast<int>(0.1f * currentSampleRate);

        // Crossfade gain — fresh voice starts at full
        voice.crossfadeGain = 1.0f;
        voice.crossfadeRate = 0.0f;

        // ---- Initialize read head positions ----
        // Heads are placed backward from the current writePos, spaced by head_spread.
        // This means each head reads slightly different history from the buffer.
        // The spread determines how much older history later heads access.
        //
        // Maximum spread at 1.0 = heads span up to 50% of the reel size.
        float maxSpread = static_cast<float>(reelSizeSamples) * 0.5f * headSpread;
        for (int h = 0; h < OstraconVoice::kMaxHeads; ++h)
        {
            if (h < headCount)
            {
                float offset = (headCount > 1)
                    ? (static_cast<float>(h) / static_cast<float>(headCount - 1)) * maxSpread
                    : 0.0f;
                float pos = static_cast<float>(writePos) - offset;
                if (pos < 0.0f) pos += static_cast<float>(reelSizeSamples);
                voice.readPos[h] = pos;
            }
            else
            {
                voice.readPos[h] = static_cast<float>(writePos);
            }
        }
    }

    void doNoteOff(int noteNum) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && !v.releasing && v.midiNote == noteNum)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
            }
        }
    }

    // Find the best free voice slot.
    // Returns: a free (inactive) slot if available, otherwise the voice with
    // the lowest envelope level (quietest candidate for stealing).
    int findFreeVoice() const noexcept
    {
        // First pass: find truly free slot
        for (int i = 0; i < kOstraconMaxVoices; ++i)
            if (!voices[i].active)
                return i;

        // Second pass: find releasing voice with lowest envelope
        int    bestIdx   = 0;
        float  bestLevel = 2.0f;
        for (int i = 0; i < kOstraconMaxVoices; ++i)
        {
            if (voices[i].releasing)
            {
                float lvl = voices[i].ampEnv.getLevel();
                if (lvl < bestLevel)
                {
                    bestLevel = lvl;
                    bestIdx   = i;
                }
            }
        }
        return bestIdx;
    }

    //==========================================================================
    //  S H A R E D   R E E L   B U F F E R   ( engine-level )
    //==========================================================================

    std::vector<float> reelBuffer;
    int                writePos       = 0;
    int                reelBufferSize = 0;    // total allocated size (maxReelSize * SR)
    int                reelSizeSamples = 0;   // current active reel size in samples
    bool               frozen          = false;
    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    float              currentSampleRate = 0.0f;

    //==========================================================================
    //  V O I C E   A R R A Y
    //==========================================================================

    std::array<OstraconVoice, kOstraconMaxVoices> voices;

    //==========================================================================
    //  O U T P U T   C A C H E
    //==========================================================================

    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;
    float cachedCouplingOutput[2] = { 0.0f, 0.0f };

    //==========================================================================
    //  S M O O T H E D   C O N T R O L S
    //==========================================================================

    float smoothedCutoff    = 12000.0f;
    float smoothedReso      = 0.15f;
    float voiceFadeRate     = 200.0f;
    float smootherCoeff     = 0.001f;   // computed in prepare(): 1−exp(−2π/(0.005·sr))

    //==========================================================================
    //  M I D I   E X P R E S S I O N   S T A T E
    //==========================================================================

    float modWheelValue   = 0.0f;  // CC1  → flutter depth
    float expressionValue = 0.0f;  // CC11 → bias
    float aftertouchValue = 0.0f;  // channel pressure → oxide
    float pitchBendNorm   = 0.0f;  // pitch bend (-1 to +1)

    //==========================================================================
    //  N O I S E   G E N E R A T O R
    //==========================================================================

    uint32_t noiseState = 12345u;

    //==========================================================================
    //  C O U P L I N G   A C C U M U L A T O R S
    //==========================================================================

    float couplingFilterAccum = 0.0f;
    float couplingAudioAccum  = 0.0f;
    float couplingMorphAccum  = 0.0f;

    //==========================================================================
    //  P A R A M E T E R   P O I N T E R S
    //==========================================================================

    // Source
    const std::atomic<float>* pWaveform    = nullptr;
    const std::atomic<float>* pTune        = nullptr;
    const std::atomic<float>* pFine        = nullptr;
    const std::atomic<float>* pSourceMix   = nullptr;

    // Reel
    const std::atomic<float>* pReelSize    = nullptr;
    const std::atomic<float>* pHeadCount   = nullptr;
    const std::atomic<float>* pHeadSpread  = nullptr;
    const std::atomic<float>* pBias        = nullptr;
    const std::atomic<float>* pRecordMode  = nullptr;
    const std::atomic<float>* pFreeze      = nullptr;

    // Tape physics
    const std::atomic<float>* pOxide       = nullptr;
    const std::atomic<float>* pFlutter     = nullptr;
    const std::atomic<float>* pFlutterRate = nullptr;
    const std::atomic<float>* pWow         = nullptr;
    const std::atomic<float>* pPrint       = nullptr;

    // Output filter
    const std::atomic<float>* pFilterCutoff = nullptr;
    const std::atomic<float>* pFilterReso   = nullptr;
    const std::atomic<float>* pFilterType   = nullptr;

    // Envelope
    const std::atomic<float>* pAttack   = nullptr;
    const std::atomic<float>* pDecay    = nullptr;
    const std::atomic<float>* pSustain  = nullptr;
    const std::atomic<float>* pRelease  = nullptr;

    // Modulation
    const std::atomic<float>* pLfoRate   = nullptr;
    const std::atomic<float>* pLfoShape  = nullptr;
    const std::atomic<float>* pLfoDepth  = nullptr;
    const std::atomic<float>* pLfoTarget = nullptr;

    // Performance
    const std::atomic<float>* pSpeed   = nullptr;
    const std::atomic<float>* pReverse = nullptr;

    // Macros
    const std::atomic<float>* pMacro1 = nullptr;
    const std::atomic<float>* pMacro2 = nullptr;
    const std::atomic<float>* pMacro3 = nullptr;
    const std::atomic<float>* pMacro4 = nullptr;
};

} // namespace xoceanus
