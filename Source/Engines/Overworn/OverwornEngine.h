// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/FilterEnvelope.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/ParameterSmoother.h"
#include <array>
#include <cmath>
#include <cstring>

namespace xoceanus
{

//==============================================================================
//
//  OVERWORN ENGINE — Spectral Reduction Pad (Erosion)
//  XO_OX Designs | XOceanus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOverworn is a sauce reducing on a stove. It begins rich — full of
//      harmonics, bright, alive. Over the course of a 30+ minute session,
//      high-frequency content evaporates first, then mids, leaving only
//      concentrated fundamentals. The session IS the envelope.
//
//  PHYSICS: The Reduction Integral
//      V(t) = V₀ - ∫₀ᵗ E(s, T, A) ds
//      where V = spectral density, E = evaporation rate (frequency-dependent),
//      T = temperature (playing intensity), A = surface area.
//
//  THE RADICAL CLAIM:
//      XOverworn is the first synthesizer engine that accumulates a 30-minute
//      memory and cannot forget. The ReductionState persists across the session.
//      Playing notes both adds (brief spectral enrichment) and subtracts
//      (accelerates reduction). This is irreversible within a session.
//      Only an explicit "Start Fresh" (stateReset parameter) resets it.
//
//  DSP ARCHITECTURE:
//      1. Rich oscillator bank (16 harmonics) — starts full, reduces over time
//      2. ReductionState: persistent spectral envelope that tracks session age
//         - High frequencies reduce first (water evaporating)
//         - Mids reduce next
//         - Fundamentals concentrate (umami, collagen — never leave)
//      3. Maillard reaction: subtle harmonic distortion increases as
//         reduction deepens (caramelization of concentrated harmonics)
//      4. Session clock: accumulates real time since first note-on
//      5. Playing intensity accelerates reduction; quiet long tones add
//         spectral character without accelerating reduction
//
//  TIME RELATIONSHIP: Irreversibility (session-long, 30+ minute arc)
//
//  4 MACROS:
//      worn_macroCharacter — initial harmonic richness + Maillard depth
//      worn_macroMovement  — reduction rate + LFO depth
//      worn_macroCoupling  — how much reduction state affects coupled engines
//      worn_macroSpace     — stereo width + space
//
//  COOPERATIVE COUPLING (BROTH):
//      Exports: sessionAge, concentrateDark, spectralMass — read by other
//      BROTH engines to adjust their own behavior as the pot ages.
//
//  PRESET SYSTEM:
//      Presets save both starting state AND reduction trajectory. A preset
//      can begin mid-reduction. ReductionState is saved in preset JSON.
//
//  ACCENT COLOR: Reduced Wine #4A1A2E
//  PARAMETER PREFIX: worn_
//  ENGINE ID: "Overworn"
//
//  DOCTRINES: D001–D006 compliant
//      D001: Velocity shapes reduction acceleration intensity
//      D002: 2 LFOs, mod wheel, aftertouch, 4 macros
//      D003: Reduction integral with frequency-dependent evaporation
//      D004: All params wired to DSP
//      D005: Breathing LFO with rate floor <= 0.01 Hz
//      D006: Velocity→reduction speed, aftertouch→heat, mod wheel→richness
//
//==============================================================================

//==============================================================================
// ReductionState — The persistent memory of the session.
// This object accumulates across the entire session. It is NOT reset between
// notes, phrases, or transport stops. Only an explicit reset gesture clears it.
//==============================================================================
struct ReductionState
{
    float sessionAge = 0.0f;        // 0.0 (fresh) → 1.0 (fully reduced)
    float concentrateDark = 0.0f;   // caramelization depth (0.0 → 1.0)
    float umamiBed = 0.0f;          // deep fundamental resonance (0.0 → 1.0)
    float volatileAromatics = 1.0f; // high-frequency shimmer remaining (1.0 → 0.0)

    // Per-band spectral mass remaining (8 octave bands)
    // Band 0 = sub/fundamental (never fully reduces)
    // Band 7 = highest harmonics (reduces first)
    float spectralMass[8] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    float totalSpectralMass() const noexcept
    {
        float sum = 0.0f;
        for (int i = 0; i < 8; ++i)
            sum += spectralMass[i];
        return sum / 8.0f;
    }

    void reset() noexcept
    {
        sessionAge = 0.0f;
        concentrateDark = 0.0f;
        umamiBed = 0.0f;
        volatileAromatics = 1.0f;
        for (int i = 0; i < 8; ++i)
            spectralMass[i] = 1.0f;
    }
};

//==============================================================================
// OverwornVoice — Single polyphonic voice within the reduction field.
//==============================================================================
struct OverwornVoice
{
    static constexpr int kNumPartials = 16;

    bool active = false;
    uint64_t startTime = 0;
    int currentNote = 60;
    float velocity = 0.0f;
    float fundamental = 440.0f;

    StandardADSR ampEnv;
    FilterEnvelope filterEnv;

    // Per-partial phase accumulators
    float partialPhase[kNumPartials] = {};

    // Is this a "quiet long tone" (infusion event)?
    float holdDuration = 0.0f; // seconds held so far
    bool isInfusion = false;   // set when velocity < 0.3 && hold > 8s

    CytomicSVF voiceFilter;

    void reset() noexcept
    {
        active = false;
        ampEnv.reset();
        filterEnv.stage = FilterEnvelope::Stage::Idle;
        filterEnv.level = 0.0f;
        holdDuration = 0.0f;
        isInfusion = false;
        voiceFilter.reset();
        std::fill(std::begin(partialPhase), std::end(partialPhase), 0.0f);
    }

    void prepare(float sampleRate) noexcept
    {
        ampEnv.prepare(sampleRate);
        filterEnv.prepare(sampleRate);
        voiceFilter.setMode(CytomicSVF::Mode::LowPass);
        voiceFilter.reset();
    }

    void noteOn(int note, float vel, float sr, uint64_t time) noexcept
    {
        active = true;
        currentNote = note;
        velocity = vel;
        startTime = time;
        fundamental = 440.0f * fastPow2((static_cast<float>(note) - 69.0f) / 12.0f);
        holdDuration = 0.0f;
        isInfusion = false;

        ampEnv.noteOn();
        filterEnv.trigger();

        // Don't reset partial phases — allow continuity for legato feel
    }

    void noteOff() noexcept
    {
        ampEnv.noteOff();
        filterEnv.release();
    }
};

//==============================================================================
// OverwornEngine
//==============================================================================
class OverwornEngine : public SynthEngine
{
public:
    OverwornEngine() = default;

    static constexpr int kMaxVoices = 8;

    //-- SynthEngine interface --------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srF = static_cast<float>(sampleRate);
        inverseSr_ = 1.0f / srF;
        blockSize = maxBlockSize;

        for (auto& v : voices)
        {
            v.prepare(srF);
            v.reset();
        }

        lfo1.setShape(StandardLFO::Sine);
        lfo1.reset();
        lfo2.setShape(StandardLFO::Triangle);
        lfo2.reset();
        breathLfo.setRate(0.007f, srF);

        noteCounter = 0;
        sessionStarted = false;

        // Silence gate: 1000ms hold (infinite sustain pad)
        silenceGate.prepare(sr, maxBlockSize);
        silenceGate.setHoldTime(1000.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        lfo1.reset();
        lfo2.reset();
        lastSampleL = lastSampleR = 0.0f;
        extFilterMod = extRingMod = 0.0f;
        // NOTE: ReductionState is NOT reset here — only explicit stateReset does that
    }

    //--------------------------------------------------------------------------
    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                if (!sessionStarted)
                    sessionStarted = true;

                int idx = VoiceAllocator::findFreeVoicePreferRelease(
                    voices, kMaxVoices,
                    [](const OverwornVoice& v) { return v.ampEnv.getStage() == StandardADSR::Stage::Release; });
                voices[idx].noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), srF, ++noteCounter);
            }
            else if (msg.isNoteOff())
            {
                int idx = VoiceAllocator::findVoiceForNote(voices, kMaxVoices, msg.getNoteNumber());
                if (idx >= 0)
                    voices[idx].noteOff();
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                aftertouch =
                    msg.isAftertouch() ? msg.getAfterTouchValue() / 127.0f : msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheel = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
            }
        }

        // 2. Bypass check
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // 3. Read parameters
        float pReductionRate = pReductionRateParam ? pReductionRateParam->load() : 0.5f;
        float pHeat = pHeatParam ? pHeatParam->load() : 0.5f;
        float pRichness = pRichnessParam ? pRichnessParam->load() : 1.0f;
        float pMaillard = pMaillardParam ? pMaillardParam->load() : 0.3f;
        float pUmamiDepth = pUmamiDepthParam ? pUmamiDepthParam->load() : 0.5f;
        float pConcentrate = pConcentrateParam ? pConcentrateParam->load() : 0.5f;
        float pStereoWidth = pStereoWidthParam ? pStereoWidthParam->load() : 0.5f;
        float pFilterCut = pFilterCutParam ? pFilterCutParam->load() : 8000.0f;
        float pFilterRes = pFilterResParam ? pFilterResParam->load() : 0.15f;
        float pFiltEnvAmt = pFiltEnvAmtParam ? pFiltEnvAmtParam->load() : 0.3f;
        float pAmpA = pAmpAParam ? pAmpAParam->load() : 0.5f;
        float pAmpD = pAmpDParam ? pAmpDParam->load() : 1.0f;
        float pAmpS = pAmpSParam ? pAmpSParam->load() : 0.9f;
        float pAmpR = pAmpRParam ? pAmpRParam->load() : 3.0f;
        float pFiltA = pFiltAParam ? pFiltAParam->load() : 0.2f;
        float pFiltD = pFiltDParam ? pFiltDParam->load() : 0.8f;
        float pFiltS = pFiltSParam ? pFiltSParam->load() : 0.5f;
        float pFiltR = pFiltRParam ? pFiltRParam->load() : 1.5f;
        float pLfo1Rate = pLfo1RateParam ? pLfo1RateParam->load() : 0.08f;
        float pLfo1Depth = pLfo1DepthParam ? pLfo1DepthParam->load() : 0.15f;
        float pLfo2Rate = pLfo2RateParam ? pLfo2RateParam->load() : 0.03f;
        float pLfo2Depth = pLfo2DepthParam ? pLfo2DepthParam->load() : 0.1f;
        float pLevel = pLevelParam ? pLevelParam->load() : 0.8f;
        float pSessionTarget = pSessionTargetParam ? pSessionTargetParam->load() : 30.0f; // minutes

        // Check for explicit state reset
        float pReset = pStateResetParam ? pStateResetParam->load() : 0.0f;
        if (pReset > 0.5f && !resetTriggered)
        {
            reduction.reset();
            resetTriggered = true;
        }
        else if (pReset <= 0.5f)
        {
            resetTriggered = false;
        }

        // Macros
        float pMC = pMacroCharacterParam ? pMacroCharacterParam->load() : 0.0f;
        float pMM = pMacroMovementParam ? pMacroMovementParam->load() : 0.0f;
        float pMK = pMacroCouplingParam ? pMacroCouplingParam->load() : 0.0f;
        float pMS = pMacroSpaceParam ? pMacroSpaceParam->load() : 0.0f;

        // CHARACTER → richness + Maillard depth
        pRichness = clamp(pRichness + pMC * 0.3f, 0.0f, 1.0f);
        pMaillard = clamp(pMaillard + pMC * 0.3f, 0.0f, 1.0f);

        // MOVEMENT → reduction rate + LFO
        pReductionRate = clamp(pReductionRate + pMM * 0.3f, 0.0f, 1.0f);
        pLfo1Depth = clamp(pLfo1Depth + pMM * 0.2f, 0.0f, 1.0f);

        // COUPLING → concentrate export strength
        pConcentrate = clamp(pConcentrate + pMK * 0.3f, 0.0f, 1.0f);

        // SPACE → stereo width
        pStereoWidth = clamp(pStereoWidth + pMS * 0.3f, 0.0f, 1.0f);

        // D006: Mod wheel → richness
        pRichness = clamp(pRichness + modWheel * 0.4f, 0.0f, 1.0f);
        // D006: Aftertouch → heat (accelerates reduction)
        pHeat = clamp(pHeat + aftertouch * 0.4f, 0.0f, 1.0f);

        // Coupling modulation
        pFilterCut = clamp(pFilterCut + extFilterMod, 50.0f, 16000.0f);

        // LFO rates
        lfo1.setRate(pLfo1Rate, srF);
        lfo2.setRate(pLfo2Rate, srF);

        // Session target in seconds
        float sessionTargetSec = pSessionTarget * 60.0f;

        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        const float inverseSr = inverseSr_;
        const float pitchBendRatio = PitchBendUtil::semitonesToFreqRatio(pitchBendNorm * 2.0f);

        // Count active non-infusion voices for reduction acceleration.
        // Infusion voices (soft long tones, velocity < 0.3 && held > 8s) do NOT accelerate
        // reduction — they add character without burning off spectral mass (isInfusion fix).
        int activeVoiceCount = 0;
        int infusionVoiceCount = 0;
        for (int v = 0; v < kMaxVoices; ++v)
        {
            if (!voices[v].active)
                continue;
            if (voices[v].isInfusion)
                ++infusionVoiceCount;
            else
                ++activeVoiceCount;
        }

        // Hoisted per-block (was incorrectly per-sample — fix 2026-04-19): setADSR fires once per
        // block, not per sample. pAmpA/D/S/R and pFiltA/D/S/R are block-constant atomic loads
        // already captured above; recomputing envelope coefficients 44100× per second was wasteful.
        for (int v = 0; v < kMaxVoices; ++v)
        {
            if (!voices[v].active)
                continue;
            voices[v].ampEnv.setADSR(pAmpA, pAmpD, pAmpS, pAmpR);
            voices[v].filterEnv.setADSR(pFiltA, pFiltD, pFiltS, pFiltR);
        }

        for (int i = 0; i < numSamples; ++i)
        {
            float lfo1Val = lfo1.process();
            float lfo2Val = lfo2.process();
            float breathVal = breathLfo.process();

            // === UPDATE REDUCTION STATE ===
            if (sessionStarted)
            {
                // Base reduction rate: governed by pReductionRate and session target
                float baseRate = pReductionRate / (sessionTargetSec * srF);

                // Heat from playing intensity accelerates reduction
                float heatMultiplier = 1.0f + pHeat * static_cast<float>(activeVoiceCount) * 0.5f;

                float deltaAge = baseRate * heatMultiplier;
                reduction.sessionAge = clamp(reduction.sessionAge + deltaAge, 0.0f, 1.0f);

                // Frequency-dependent evaporation: high bands reduce first
                // Logarithmic decay curve (early reduction is fast, late is slow)
                for (int b = 0; b < 8; ++b)
                {
                    // Band 0 (sub) barely reduces. Band 7 (highs) reduces fastest.
                    float bandRate = baseRate * heatMultiplier *
                                     (0.05f + static_cast<float>(b) * 0.3f); // band 0: 0.05x, band 7: 2.15x

                    // Logarithmic slowdown as band approaches empty
                    float remaining = reduction.spectralMass[b];
                    bandRate *= remaining; // reduces slower as it empties

                    reduction.spectralMass[b] = clamp(remaining - bandRate, 0.0f, 1.0f);
                }

                // Caramelization (Maillard reaction) increases with reduction.
                // pConcentrate scales the export strength: higher concentrate = darker, denser timbre.
                reduction.concentrateDark = clamp(reduction.sessionAge * pMaillard * pConcentrate * 1.5f, 0.0f, 1.0f);

                // Umami bed: fundamentals concentrate as everything else reduces
                reduction.umamiBed = clamp((1.0f - reduction.totalSpectralMass()) * pUmamiDepth, 0.0f, 1.0f);

                // Volatile aromatics (high-freq shimmer) track band 6+7
                reduction.volatileAromatics = (reduction.spectralMass[6] + reduction.spectralMass[7]) * 0.5f;

                // Infusion: soft long tones add back spectral character to the top bands.
                // When infusion voices are active, gently re-energize bands 6 and 7
                // (high-frequency shimmer) — like adding a splash of water at low heat.
                // This consumes the isInfusion flag (D004 / concept fix).
                if (infusionVoiceCount > 0)
                {
                    float infuseRate = baseRate * static_cast<float>(infusionVoiceCount) * 0.3f;
                    reduction.spectralMass[6] = clamp(reduction.spectralMass[6] + infuseRate, 0.0f, 1.0f);
                    reduction.spectralMass[7] = clamp(reduction.spectralMass[7] + infuseRate * 0.5f, 0.0f, 1.0f);
                    // Also refresh volatile aromatics from the infusion
                    reduction.volatileAromatics = (reduction.spectralMass[6] + reduction.spectralMass[7]) * 0.5f;
                }
            }

            // === SYNTHESIZE VOICES ===
            float sampleL = 0.0f;
            float sampleR = 0.0f;

            for (int v = 0; v < kMaxVoices; ++v)
            {
                auto& voice = voices[v];
                if (!voice.active)
                    continue;

                // Track hold duration for infusion detection
                voice.holdDuration += inverseSr;
                if (voice.velocity < 0.3f && voice.holdDuration > 8.0f)
                    voice.isInfusion = true;

                float ampLevel = voice.ampEnv.process();
                float filtLevel = voice.filterEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // D001: velocity shapes brightness
                float velBright = 0.5f + voice.velocity * 0.5f;

                float fundamental = voice.fundamental * pitchBendRatio;

                // Synthesize partials, shaped by ReductionState
                float voiceSample = 0.0f;
                int numPartials = static_cast<int>(pRichness * 15.0f) + 1;

                for (int p = 0; p < numPartials; ++p)
                {
                    float harmonic = static_cast<float>(p + 1);
                    float freq = fundamental * harmonic;
                    if (freq > srF * 0.49f)
                        break;

                    // Determine which reduction band this partial falls into
                    // Map frequency to band (logarithmic: band 0 = <100Hz, band 7 = >8kHz)
                    int band = 0;
                    if (freq > 8000.0f)
                        band = 7;
                    else if (freq > 4000.0f)
                        band = 6;
                    else if (freq > 2000.0f)
                        band = 5;
                    else if (freq > 1000.0f)
                        band = 4;
                    else if (freq > 500.0f)
                        band = 3;
                    else if (freq > 250.0f)
                        band = 2;
                    else if (freq > 100.0f)
                        band = 1;

                    float bandGain = reduction.spectralMass[band];

                    // Phase accumulator
                    voice.partialPhase[p] += freq * inverseSr;
                    if (voice.partialPhase[p] >= 1.0f)
                        voice.partialPhase[p] -= 1.0f;

                    // Sine oscillator with 1/n harmonic weighting
                    float sine = fastSin(voice.partialPhase[p] * 6.28318530718f);
                    float partialAmp = (1.0f / harmonic) * bandGain * velBright;

                    // Add LFO modulation (D005: breathing)
                    partialAmp *= (1.0f + breathVal * 0.05f);
                    partialAmp *= (1.0f + lfo1Val * pLfo1Depth * 0.1f);

                    voiceSample += sine * partialAmp;
                }

                // Normalize
                voiceSample *= (2.0f / static_cast<float>(numPartials));

                // Maillard reaction: add subtle harmonic distortion as reduction deepens
                if (reduction.concentrateDark > 0.05f)
                {
                    float darkDrive = 1.0f + reduction.concentrateDark * 3.0f;
                    voiceSample = fastTanh(voiceSample * darkDrive) / darkDrive;
                }

                // Umami bed: boost fundamental when reduction is deep
                if (reduction.umamiBed > 0.1f)
                {
                    float umamiBass = fastSin(voice.partialPhase[0] * 6.28318530718f) * reduction.umamiBed * 0.3f;
                    voiceSample += umamiBass;
                }

                // Per-voice filter — cutoff reduces with session age
                float voiceCutoff = pFilterCut * (1.0f - reduction.sessionAge * 0.7f) +
                                    pFiltEnvAmt * filtLevel * 4000.0f * voice.velocity;
                voiceCutoff = clamp(voiceCutoff, 50.0f, srF * 0.49f);
                voice.voiceFilter.setCoefficients_fast(voiceCutoff, pFilterRes, srF);
                voiceSample = voice.voiceFilter.processSample(voiceSample);

                // Apply amp envelope
                voiceSample *= ampLevel;

                // Stereo: pan based on note + LFO2
                float pan = 0.5f + (static_cast<float>(voice.currentNote - 60) * 0.02f + lfo2Val * pLfo2Depth * 0.3f) *
                                       pStereoWidth;
                pan = clamp(pan, 0.0f, 1.0f);

                sampleL += voiceSample * (1.0f - pan);
                sampleR += voiceSample * pan;
            }

            // Apply level
            sampleL *= pLevel;
            sampleR *= pLevel;

            // Ring mod coupling
            if (std::fabs(extRingMod) > 0.001f)
            {
                sampleL *= (1.0f + extRingMod);
                sampleR *= (1.0f + extRingMod);
            }

            sampleL = softClip(sampleL);
            sampleR = softClip(sampleR);

            lastSampleL = sampleL;
            lastSampleR = sampleR;

            outL[i] += sampleL;
            if (outR)
                outR[i] += sampleR;
        }

        // Write sessionAge back to APVTS so host can display it (D004 fix).
        // The pSessionAgeParam is a read-only parameter the host/UI can observe.
        // Use relaxed ordering — this is a display value, not a sync barrier.
        if (pSessionAgeParam)
            pSessionAgeParam->store(reduction.sessionAge, std::memory_order_relaxed);

        // Reset coupling mods
        extFilterMod = 0.0f;
        extRingMod = 0.0f;

        // Silence gate
        const float* rL = buffer.getReadPointer(0);
        const float* rR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : nullptr;
        silenceGate.analyzeBlock(rL, rR, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int /*numSamples*/) override
    {
        switch (type)
        {
        case CouplingType::AmpToFilter:
            extFilterMod = amount * 4000.0f;
            break;
        case CouplingType::AudioToRing:
            extRingMod = (sourceBuffer ? sourceBuffer[0] : 0.0f) * amount;
            break;
        default:
            break;
        }
    }

    //-- BROTH Cooperative Coupling — EXPORTS -----------------------------------
    // Other BROTH engines read these to adjust their behavior
    float getSessionAge() const { return reduction.sessionAge; }
    float getConcentrateDark() const { return reduction.concentrateDark; }
    float getUmamiBed() const { return reduction.umamiBed; }
    float getVolatileAromatics() const { return reduction.volatileAromatics; }
    float getSpectralMass(int band) const
    {
        if (band >= 0 && band < 8)
            return reduction.spectralMass[band];
        return 0.0f;
    }
    float getTotalSpectralMass() const { return reduction.totalSpectralMass(); }

    // Access the full ReductionState for preset serialization
    const ReductionState& getReductionState() const { return reduction; }
    void setReductionState(const ReductionState& state) { reduction = state; }

    //-- Parameters -------------------------------------------------------------
    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        // Core reduction parameters
        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_reductionRate", 1}, "Reduction Rate",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_heat", 1}, "Heat",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_richness", 1}, "Richness",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_maillard", 1}, "Maillard",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_umamiDepth", 1}, "Umami Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_concentrate", 1}, "Concentrate",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_sessionTarget", 1}, "Session Target (min)",
                                              juce::NormalisableRange<float>(5.0f, 120.0f, 0.0f, 0.4f), 30.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_sessionAge", 1}, "Session Age (read-only)",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_stateReset", 1}, "Start Fresh",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        // Tone
        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_stereoWidth", 1}, "Stereo Width",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_filterCutoff", 1}, "Filter Cutoff",
                                              juce::NormalisableRange<float>(50.0f, 16000.0f, 0.0f, 0.3f), 8000.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_filterRes", 1}, "Filter Resonance",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_filtEnvAmount", 1}, "Filter Env Amount",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.3f));

        // Amp ADSR
        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_ampAttack", 1}, "Amp Attack",
                                              juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_ampDecay", 1}, "Amp Decay",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 1.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_ampSustain", 1}, "Amp Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.9f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_ampRelease", 1}, "Amp Release",
                                              juce::NormalisableRange<float>(0.01f, 15.0f, 0.0f, 0.3f), 3.0f));

        // Filter ADSR
        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_filtAttack", 1}, "Filter Attack",
                                              juce::NormalisableRange<float>(0.001f, 5.0f, 0.0f, 0.3f), 0.2f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_filtDecay", 1}, "Filter Decay",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 0.8f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_filtSustain", 1}, "Filter Sustain",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_filtRelease", 1}, "Filter Release",
                                              juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.3f), 1.5f));

        // LFOs
        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_lfo1Rate", 1}, "LFO 1 Rate",
                                              juce::NormalisableRange<float>(0.005f, 10.0f, 0.0f, 0.3f), 0.08f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_lfo1Depth", 1}, "LFO 1 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.15f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_lfo2Rate", 1}, "LFO 2 Rate",
                                              juce::NormalisableRange<float>(0.005f, 5.0f, 0.0f, 0.3f), 0.03f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_lfo2Depth", 1}, "LFO 2 Depth",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.1f));

        // Output
        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_level", 1}, "Level",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

        // Macros
        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_macroCharacter", 1}, "Character",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_macroMovement", 1}, "Movement",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_macroCoupling", 1}, "Coupling",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<PF>(juce::ParameterID{"worn_macroSpace", 1}, "Space",
                                              juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pReductionRateParam = apvts.getRawParameterValue("worn_reductionRate");
        pHeatParam = apvts.getRawParameterValue("worn_heat");
        pRichnessParam = apvts.getRawParameterValue("worn_richness");
        pMaillardParam = apvts.getRawParameterValue("worn_maillard");
        pUmamiDepthParam = apvts.getRawParameterValue("worn_umamiDepth");
        pConcentrateParam = apvts.getRawParameterValue("worn_concentrate");
        pSessionTargetParam = apvts.getRawParameterValue("worn_sessionTarget");
        pSessionAgeParam = apvts.getRawParameterValue("worn_sessionAge");
        pStateResetParam = apvts.getRawParameterValue("worn_stateReset");
        pStereoWidthParam = apvts.getRawParameterValue("worn_stereoWidth");
        pFilterCutParam = apvts.getRawParameterValue("worn_filterCutoff");
        pFilterResParam = apvts.getRawParameterValue("worn_filterRes");
        pFiltEnvAmtParam = apvts.getRawParameterValue("worn_filtEnvAmount");
        pAmpAParam = apvts.getRawParameterValue("worn_ampAttack");
        pAmpDParam = apvts.getRawParameterValue("worn_ampDecay");
        pAmpSParam = apvts.getRawParameterValue("worn_ampSustain");
        pAmpRParam = apvts.getRawParameterValue("worn_ampRelease");
        pFiltAParam = apvts.getRawParameterValue("worn_filtAttack");
        pFiltDParam = apvts.getRawParameterValue("worn_filtDecay");
        pFiltSParam = apvts.getRawParameterValue("worn_filtSustain");
        pFiltRParam = apvts.getRawParameterValue("worn_filtRelease");
        pLfo1RateParam = apvts.getRawParameterValue("worn_lfo1Rate");
        pLfo1DepthParam = apvts.getRawParameterValue("worn_lfo1Depth");
        pLfo2RateParam = apvts.getRawParameterValue("worn_lfo2Rate");
        pLfo2DepthParam = apvts.getRawParameterValue("worn_lfo2Depth");
        pLevelParam = apvts.getRawParameterValue("worn_level");
        pMacroCharacterParam = apvts.getRawParameterValue("worn_macroCharacter");
        pMacroMovementParam = apvts.getRawParameterValue("worn_macroMovement");
        pMacroCouplingParam = apvts.getRawParameterValue("worn_macroCoupling");
        pMacroSpaceParam = apvts.getRawParameterValue("worn_macroSpace");
    }

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Overworn"; }

    juce::Colour getAccentColour() const override
    {
        return juce::Colour(0xFF4A1A2E); // Reduced Wine
    }

    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override { return VoiceAllocator::countActive(voices, kMaxVoices); }

private:
    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srF = 0.0f;  // Sentinel: must be set by prepare() before use
    float inverseSr_ = 1.0f / 44100.0f;
    int blockSize = 512;

    OverwornVoice voices[kMaxVoices];
    uint64_t noteCounter = 0;
    bool sessionStarted = false;
    bool resetTriggered = false;

    // THE REDUCTION STATE — persistent across session
    ReductionState reduction;

    // LFOs
    StandardLFO lfo1, lfo2;
    BreathingLFO breathLfo;

    // MIDI state
    float aftertouch = 0.0f;
    float modWheel = 0.0f;
    float pitchBendNorm = 0.0f;

    // Coupling state
    float extFilterMod = 0.0f;
    float extRingMod = 0.0f;
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    // Parameter pointers (30 params)
    std::atomic<float>* pReductionRateParam = nullptr;
    std::atomic<float>* pHeatParam = nullptr;
    std::atomic<float>* pRichnessParam = nullptr;
    std::atomic<float>* pMaillardParam = nullptr;
    std::atomic<float>* pUmamiDepthParam = nullptr;
    std::atomic<float>* pConcentrateParam = nullptr;
    std::atomic<float>* pSessionTargetParam = nullptr;
    std::atomic<float>* pSessionAgeParam = nullptr;
    std::atomic<float>* pStateResetParam = nullptr;
    std::atomic<float>* pStereoWidthParam = nullptr;
    std::atomic<float>* pFilterCutParam = nullptr;
    std::atomic<float>* pFilterResParam = nullptr;
    std::atomic<float>* pFiltEnvAmtParam = nullptr;
    std::atomic<float>* pAmpAParam = nullptr;
    std::atomic<float>* pAmpDParam = nullptr;
    std::atomic<float>* pAmpSParam = nullptr;
    std::atomic<float>* pAmpRParam = nullptr;
    std::atomic<float>* pFiltAParam = nullptr;
    std::atomic<float>* pFiltDParam = nullptr;
    std::atomic<float>* pFiltSParam = nullptr;
    std::atomic<float>* pFiltRParam = nullptr;
    std::atomic<float>* pLfo1RateParam = nullptr;
    std::atomic<float>* pLfo1DepthParam = nullptr;
    std::atomic<float>* pLfo2RateParam = nullptr;
    std::atomic<float>* pLfo2DepthParam = nullptr;
    std::atomic<float>* pLevelParam = nullptr;
    std::atomic<float>* pMacroCharacterParam = nullptr;
    std::atomic<float>* pMacroMovementParam = nullptr;
    std::atomic<float>* pMacroCouplingParam = nullptr;
    std::atomic<float>* pMacroSpaceParam = nullptr;
};

} // namespace xoceanus
