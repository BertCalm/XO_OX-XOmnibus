// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/WavetableOscillator.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/Effects/LushReverb.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xoceanus
{

//==============================================================================
// OctopusEngine — Decentralized Alien Intelligence Synthesis.
//
// Maps the bizarre anatomy and behavior of the octopus directly into
// synthesizer architecture. Five biological subsystems become five DSP modules
// that morph, camouflage, ink, and grip independently.
//
// SUBSYSTEMS:
//
//   1. ARMS — 8 independent LFOs running at prime-ratio-related rates,
//      each modulating a different parameter target. Two-thirds of an
//      octopus's neurons are in its arms; they think independently. The
//      result is generative polyrhythmic modulation that never exactly
//      repeats. armSpread controls how different the rates are;
//      armBaseRate sets the fundamental tempo. Each arm targets a
//      different sonic dimension (pitch, filter, wavetable, pan, etc.)
//
//   2. CHROMATOPHORES — Envelope follower feeding a morphing filter
//      bank. The octopus can instantly change skin color, pattern, and
//      texture to camouflage. An envelope follower tracks the input
//      amplitude and maps it to a continuously blending filter topology
//      (lowpass → bandpass → highpass → notch). chromaSpeed controls
//      adaptation rate; chromaDepth controls morphing intensity.
//
//   3. INK CLOUD — Velocity-triggered noise burst routed into a freeze
//      buffer with infinite hold and slow decay. When you hit a key at
//      maximum velocity, the dry synth signal mutes and an explosive
//      wall of dark, saturated noise erupts. The noise hangs in the
//      stereo field as a massive, blinding wash that slowly dissolves
//      into silence.
//
//   4. SHAPESHIFTER — Microtonal detuning, extreme portamento (up to
//      10 seconds), and random pitch drift. The octopus has no bones;
//      it can pour itself through any gap. The synth refuses to lock
//      to standard Western notes, continuously squeezing and slithering
//      through the frequencies between keys. Boneless pitch.
//
//   5. SUCKERS — Ultra-fast filter envelope (~1ms attack) with high
//      resonance bandpass filter. Creates a distinct "plonk" or sticky,
//      wet, bubbling transient — mimicking the suction and release of
//      tentacle suckers physically grabbing onto the audio spectrum.
//      The sucker envelope triggers independently on each note-on.
//
// Coupling:
//   - Output: Post-process stereo audio, envelope level
//   - Input: AudioToFM (modulate wavetable), AmpToFilter (chromatophore
//            sensitivity), EnvToMorph (arm rate modulation), AudioToRing
//            (ring mod source), LFOToPitch (shapeshifter pitch mod)
//
// Accent Color: Chromatophore Magenta #E040FB
//
// Inspirations: Tiptop Audio OCTOPUS (8 independent voice channels),
// Moog Labyrinth (generative sequencing), OBNE Procession (sci-fi
// freeze reverb), Chromaplane (spatial morphing), Home Bake CHAOS
// (shift register randomness)
//
//==============================================================================

//==============================================================================
// OctoADSR replaced by shared StandardADSR (Source/DSP/StandardADSR.h).
// API-compatible: setParams(), noteOn(), noteOff(), process(), reset(), isActive() all match.
//==============================================================================
using OctoADSR = StandardADSR;

//==============================================================================
// OctoLFO replaced by shared StandardLFO (Source/DSP/StandardLFO.h).
// API-compatible: setRate(), setShape(), process(), reset() all match.
// Used by both main LFOs and the 8-arm system.
//==============================================================================
using OctoLFO = StandardLFO;

//==============================================================================
// Freeze buffer for Ink Cloud — captures and holds a noise burst.
//==============================================================================
struct OctoFreezeBuffer
{
    static constexpr int kBufferSize = 8192;

    float buffer[kBufferSize]{};
    int writePos = 0;
    bool frozen = false;
    float frozenGain = 0.0f;   // current gain of the frozen cloud
    float decayRate = 0.0001f; // how fast the cloud dissolves
    int readPos = 0;

    void freeze(float density, float sampleRate) noexcept
    {
        // Fill buffer with shaped noise burst
        uint32_t rng = 98765u;
        for (int i = 0; i < kBufferSize; ++i)
        {
            rng = rng * 1664525u + 1013904223u;
            float noise = static_cast<float>(rng & 0xFFFF) / 32768.0f - 1.0f;
            // Shape: dark noise — lowpass-ish via averaging adjacent samples
            float prev = (i > 0) ? buffer[i - 1] : 0.0f;
            buffer[i] = noise * density * 0.7f + prev * 0.3f;
        }
        frozen = true;
        frozenGain = 1.0f;
        readPos = 0;
    }

    void setDecay(float decayTimeSec, float sampleRate) noexcept
    {
        if (decayTimeSec > 0.01f)
            decayRate = 1.0f / (decayTimeSec * std::max(1.0f, sampleRate));
        else
            decayRate = 1.0f;
    }

    float process() noexcept
    {
        if (!frozen || frozenGain <= 0.0001f)
        {
            frozen = false;
            frozenGain = 0.0f;
            return 0.0f;
        }

        // Read from circular buffer with slow fade
        float sample = buffer[readPos] * frozenGain;
        readPos = (readPos + 1) % kBufferSize;

        // Slow dissolve
        frozenGain -= decayRate;
        frozenGain = flushDenormal(frozenGain);
        if (frozenGain < 0.0f)
            frozenGain = 0.0f;

        return sample;
    }

    bool isActive() const noexcept { return frozen && frozenGain > 0.0001f; }

    void reset() noexcept
    {
        std::memset(buffer, 0, sizeof(buffer));
        writePos = 0;
        readPos = 0;
        frozen = false;
        frozenGain = 0.0f;
    }
};

//==============================================================================
// OctoVoice — per-voice state.
//==============================================================================
struct OctoVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // MPE per-voice expression state
    MPEVoiceExpression mpeExpression;

    // Shapeshifter — boneless pitch
    float currentFreq = 440.0f;
    float targetFreq = 440.0f;
    float glideCoeff = 1.0f;
    float microtonalOffset = 0.0f; // cents offset from standard pitch
    float pitchDriftPhase = 0.0f;  // slow random drift oscillator

    // Wavetable oscillator (core sound source)
    WavetableOscillator wtOsc;

    // Envelopes
    OctoADSR ampEnv;
    OctoADSR modEnv;

    // Sucker envelope — ultra-fast transient
    OctoADSR suckerEnv;

    // Main LFOs
    OctoLFO lfo1; // general purpose
    OctoLFO lfo2; // general purpose

    // Arms — 8 independent modulation LFOs
    OctoLFO arms[8];

    // Chromatophore filter (morphing SVF)
    CytomicSVF chromaFilter;
    float envFollower = 0.0f; // envelope follower state

    // Sucker filter (high-Q bandpass for pluck transient)
    CytomicSVF suckerFilter;

    // Main output filter
    CytomicSVF mainFilter;

    // Formant filters — arm #8 (ArmFormantShift) modulates their center frequencies
    // F1: 300–800 Hz (first vocal formant region)
    // F2: 800–2500 Hz (second vocal formant region)
    CytomicSVF formant1;
    CytomicSVF formant2;

    // Ink cloud (per-voice freeze buffer)
    OctoFreezeBuffer inkCloud;
    bool inkTriggered = false;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    // Per-voice RNG for drift
    uint32_t driftRng = 0u;

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        currentFreq = 440.0f;
        targetFreq = 440.0f;
        fadeGain = 1.0f;
        fadingOut = false;
        microtonalOffset = 0.0f;
        pitchDriftPhase = 0.0f;
        envFollower = 0.0f;
        inkTriggered = false;
        ampEnv.reset();
        modEnv.reset();
        suckerEnv.reset();
        lfo1.reset();
        lfo2.reset();
        for (auto& arm : arms)
            arm.reset();
        wtOsc.reset();
        chromaFilter.reset();
        suckerFilter.reset();
        mainFilter.reset();
        formant1.reset();
        formant2.reset();
        inkCloud.reset();
    }
};

//==============================================================================
// OctopusEngine — the main engine class.
//==============================================================================
class OctopusEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    // Prime ratios for the 8 arms — these ensure polyrhythmic independence
    // Ratios relative to base rate: each arm runs at baseRate * primeRatio[i]
    // Using primes/near-primes guarantees the arms never phase-lock
    static constexpr float kArmPrimeRatios[8] = {
        1.0f,       1.618034f,  2.236068f,  3.14159f, // 1, phi, sqrt(5), pi
        0.7071068f, 1.4142136f, 2.6457513f, 0.381966f // 1/sqrt(2), sqrt(2), sqrt(7), 1-1/phi
    };

    // Arm target indices — each arm modulates a different parameter dimension
    enum ArmTarget
    {
        ArmFilterCutoff = 0,
        ArmWavetablePos = 1,
        ArmPitch = 2,
        ArmPanSpread = 3,
        ArmChromaFreq = 4,
        ArmSuckerFreq = 5,
        ArmLevel = 6,
        ArmFormantShift = 7
    };

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float>(sr);

        smoothCoeff = 1.0f - std::exp(-kTwoPi * (1.0f / 0.005f) / srf);
        crossfadeRate = 1.0f / (0.005f * srf);

        outputCacheL.resize(static_cast<size_t>(maxBlockSize), 0.0f);
        outputCacheR.resize(static_cast<size_t>(maxBlockSize), 0.0f);

        silenceGate.prepare(sampleRate, maxBlockSize);

        // Build procedural organic wavetable
        buildOctopusWavetable();

        // Initialize voices
        for (int v = 0; v < kMaxVoices; ++v)
        {
            auto& voice = voices[v];
            voice.reset();
            voice.driftRng = static_cast<uint32_t>(v * 777 + 13579);
            voice.wtOsc.loadWavetable(octopusWavetable, kWTFrames, kWTFrameSize);

            voice.chromaFilter.reset();
            voice.chromaFilter.setMode(CytomicSVF::Mode::LowPass);
            voice.suckerFilter.reset();
            voice.suckerFilter.setMode(CytomicSVF::Mode::BandPass);
            voice.mainFilter.reset();
            voice.mainFilter.setMode(CytomicSVF::Mode::LowPass);
            voice.formant1.reset();
            voice.formant1.setMode(CytomicSVF::Mode::BandPass);
            voice.formant2.reset();
            voice.formant2.setMode(CytomicSVF::Mode::BandPass);
        }
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingWTPosMod = 0.0f;
        couplingChromaMod = 0.0f;
        couplingArmRateMod = 0.0f;
        couplingRingModSrc = 0.0f;
        couplingPitchMod = 0.0f;

        smoothedWTPos = 0.0f;
        smoothedChromaDepth = 0.0f;
        smoothedInkMix = 0.0f;
        smoothedSuckerMix = 0.0f;

        std::fill(outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill(outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // Audio
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0)
            return;

        // --- ParamSnapshot: read all parameters once per block ---

        // Arms
        const int pArmCount = std::max(1, std::min(8, static_cast<int>(loadParam(paramArmCount, 4.0f))));
        const float pArmSpread = loadParam(paramArmSpread, 0.5f);
        const float pArmBaseRate = loadParam(paramArmBaseRate, 1.0f);
        const float pArmDepth = loadParam(paramArmDepth, 0.5f);

        // Chromatophores
        const float pChromaSens = loadParam(paramChromaSens, 0.5f);
        const float pChromaSpeed = loadParam(paramChromaSpeed, 0.5f);
        const float pChromaMorph = loadParam(paramChromaMorph, 0.0f);
        const float pChromaDepth = loadParam(paramChromaDepth, 0.5f);
        const float pChromaFreq = loadParam(paramChromaFreq, 2000.0f);

        // Ink Cloud
        const float pInkThreshold = loadParam(paramInkThreshold, 0.9f);
        const float pInkDensity = loadParam(paramInkDensity, 0.8f);
        const float pInkDecay = loadParam(paramInkDecay, 5.0f);
        const float pInkMix = loadParam(paramInkMix, 0.0f);

        // Shapeshifter
        const float pShiftMicro = loadParam(paramShiftMicro, 0.0f);
        const float pShiftGlide = loadParam(paramShiftGlide, 0.5f);
        const float pShiftDrift = loadParam(paramShiftDrift, 0.0f);

        // Suckers
        const float pSuckerReso = loadParam(paramSuckerReso, 0.8f);
        const float pSuckerFreq = loadParam(paramSuckerFreq, 2000.0f);
        const float pSuckerDecay = loadParam(paramSuckerDecay, 0.05f);
        const float pSuckerMix = loadParam(paramSuckerMix, 0.0f);

        // Core
        const float pWTPos = loadParam(paramWTPos, 0.0f);
        const float pWTScanRate = loadParam(paramWTScanRate, 0.3f);
        const float pCutoff = loadParam(paramFilterCutoff, 8000.0f);
        const float pReso = loadParam(paramFilterReso, 0.0f);
        const float pLevel = loadParam(paramLevel, 0.8f);

        const float pAmpA = loadParam(paramAmpAttack, 0.01f);
        const float pAmpD = loadParam(paramAmpDecay, 0.3f);
        const float pAmpS = loadParam(paramAmpSustain, 0.7f);
        const float pAmpR = loadParam(paramAmpRelease, 0.5f);
        const float pModA = loadParam(paramModAttack, 0.01f);
        const float pModD = loadParam(paramModDecay, 0.5f);
        const float pModS = loadParam(paramModSustain, 0.5f);
        const float pModR = loadParam(paramModRelease, 0.5f);

        const float pLfo1Rate = loadParam(paramLfo1Rate, 0.5f);
        const float pLfo1Depth = loadParam(paramLfo1Depth, 0.3f);
        const int pLfo1Shape = static_cast<int>(loadParam(paramLfo1Shape, 0.0f));
        const float pLfo2Rate = loadParam(paramLfo2Rate, 2.0f);
        const float pLfo2Depth = loadParam(paramLfo2Depth, 0.0f);
        const int pLfo2Shape = static_cast<int>(loadParam(paramLfo2Shape, 0.0f));

        const int voiceModeIdx = static_cast<int>(loadParam(paramVoiceMode, 2.0f));

        const float macroChar = loadParam(paramMacroCharacter, 0.0f);
        const float macroMove = loadParam(paramMacroMovement, 0.0f);
        const float macroCoup = loadParam(paramMacroCoupling, 0.0f);
        const float macroSpace = loadParam(paramMacroSpace, 0.0f);

        // Voice mode
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIdx)
        {
        case 0:
            maxPoly = 1;
            monoMode = true;
            break;
        case 1:
            maxPoly = 1;
            monoMode = true;
            legatoMode = true;
            break;
        case 2:
            maxPoly = 8;
            break;
        case 3:
            maxPoly = 16;
            break;
        default:
            maxPoly = 8;
            break;
        }

        // Glide coefficient — shapeshifter extreme portamento
        float glideCoeff = 1.0f;
        if (pShiftGlide > 0.001f)
            glideCoeff = 1.0f - std::exp(-1.0f / (pShiftGlide * srf));

        // === MACRO MODULATION ===
        // M1 CHARACTER: arms depth + sucker intensity
        // M2 MOVEMENT: arm rate + chromatophore speed + wavetable scan
        // M3 COUPLING: chromatophore depth + ink sensitivity
        // M4 SPACE: ink decay time + pitch drift

        float effectiveArmDepth = clamp(pArmDepth + macroChar * 0.4f, 0.0f, 1.0f);
        float effectiveArmRate = clamp(pArmBaseRate + couplingArmRateMod + macroMove * 3.0f, 0.05f, 20.0f);
        // D006: mod wheel intensifies chromatophore skin-shift — adds up to +0.4
        // chroma depth, causing the filter topology to morph more aggressively.
        float effectiveChromaDepth =
            clamp(pChromaDepth + macroCoup * 0.4f + couplingChromaMod + modWheelAmount_ * 0.4f, 0.0f, 1.0f);
        float effectiveWTPos = clamp(pWTPos + couplingWTPosMod + macroMove * 0.2f, 0.0f, 1.0f);
        float effectiveCutoff = clamp(pCutoff + macroChar * 4000.0f, 20.0f, 20000.0f);
        float effectiveReso = clamp(pReso + macroChar * 0.2f, 0.0f, 1.0f);
        float effectiveInkMix = clamp(pInkMix + macroCoup * 0.3f, 0.0f, 1.0f);
        float effectiveSuckerMix = clamp(pSuckerMix + macroChar * 0.3f, 0.0f, 1.0f);
        float effectiveDrift = clamp(pShiftDrift + macroSpace * 0.3f, 0.0f, 1.0f);
        float effectiveInkDecay = clamp(pInkDecay + macroSpace * 10.0f, 0.5f, 30.0f);

        // Chromatophore filter morph target: 0=LP, 0.33=BP, 0.66=HP, 1.0=Notch
        float chromaMorphTarget = clamp(pChromaMorph + macroMove * 0.3f, 0.0f, 1.0f);

        // Snapshot pitch + ring-mod coupling before reset (#1118).
        const float blockCouplingPitchMod   = couplingPitchMod;
        const float blockCouplingRingModSrc = couplingRingModSrc;
        // Reset coupling accumulators
        couplingWTPosMod = 0.0f;
        couplingChromaMod = 0.0f;
        couplingArmRateMod = 0.0f;
        couplingRingModSrc = 0.0f;
        couplingPitchMod = 0.0f;

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn(msg.getNoteNumber(), msg.getFloatVelocity(), msg.getChannel(), maxPoly, monoMode, legatoMode,
                       glideCoeff, pAmpA, pAmpD, pAmpS, pAmpR, pModA, pModD, pModS, pModR, pSuckerDecay, pLfo1Rate,
                       pLfo1Depth, pLfo1Shape, pLfo2Rate, pLfo2Depth, pLfo2Shape, effectiveCutoff, effectiveReso,
                       effectiveArmRate, pArmSpread, pArmCount, pInkThreshold, pInkDensity, effectiveInkDecay,
                       pShiftMicro);
            }
            else if (msg.isNoteOff())
                noteOff(msg.getNoteNumber(), msg.getChannel());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount_ = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel())
                pitchBendNorm = PitchBendUtil::parsePitchWheel(msg.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // --- Update per-voice MPE expression from MPEManager ---
        if (mpeManager != nullptr)
        {
            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;
                mpeManager->updateVoiceExpression(voice.mpeExpression);
            }
        }

        // --- Update per-voice filter coefficients once per block ---
        for (auto& voice : voices)
        {
            if (!voice.active)
                continue;

            voice.mainFilter.setCoefficients(effectiveCutoff, effectiveReso, srf);
            voice.suckerFilter.setCoefficients(clamp(pSuckerFreq, 200.0f, 8000.0f), pSuckerReso, srf);

            // Set arm LFO rates per voice
            for (int a = 0; a < 8; ++a)
            {
                float armRate = effectiveArmRate * kArmPrimeRatios[a];
                // Spread: 0=all same rate, 1=full prime ratio diversity
                armRate = effectiveArmRate + (armRate - effectiveArmRate) * pArmSpread;
                voice.arms[a].setRate(armRate, srf);
                // Alternate shapes for variety
                voice.arms[a].setShape(a % 5);
            }

            // Ink cloud decay
            voice.inkCloud.setDecay(effectiveInkDecay, srf);
        }

        float peakEnv = 0.0f;

        // Hoist block-constant chromatophore envelope follower coefficient.
        // pChromaSpeed and srf are both block-rate values — computing this once
        // per block instead of per-sample saves one std::exp call per active voice
        // per sample (up to 16x calls at max polyphony).
        const float chromaCoeff = 1.0f - std::exp(-kTwoPi * (pChromaSpeed * 20.0f + 1.0f) / srf);

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const bool updateFilter = ((sample & 15) == 0);
            // Smooth control-rate parameters
            smoothedWTPos += (effectiveWTPos - smoothedWTPos) * smoothCoeff;
            smoothedChromaDepth += (effectiveChromaDepth - smoothedChromaDepth) * smoothCoeff;
            smoothedInkMix += (effectiveInkMix - smoothedInkMix) * smoothCoeff;
            smoothedSuckerMix += (effectiveSuckerMix - smoothedSuckerMix) * smoothCoeff;
            smoothedWTPos = flushDenormal(smoothedWTPos);
            smoothedChromaDepth = flushDenormal(smoothedChromaDepth);
            smoothedInkMix = flushDenormal(smoothedInkMix);
            smoothedSuckerMix = flushDenormal(smoothedSuckerMix);

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active)
                    continue;

                // --- Voice-stealing crossfade ---
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    voice.fadeGain = flushDenormal(voice.fadeGain);
                    if (voice.fadeGain <= 0.0f)
                    {
                        voice.fadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // =====================================================
                // 1. ARMS — 8 independent polyrhythmic modulation LFOs
                // =====================================================

                float armMods[8]{};
                for (int a = 0; a < pArmCount; ++a)
                    armMods[a] = voice.arms[a].process() * effectiveArmDepth;

                // =====================================================
                // 4. SHAPESHIFTER — Boneless pitch, microtonal drift
                // =====================================================

                // Pitch drift: slow random walk
                if (effectiveDrift > 0.001f)
                {
                    voice.pitchDriftPhase += 0.0001f;
                    if (voice.pitchDriftPhase >= 1.0f)
                        voice.pitchDriftPhase -= 1.0f;
                    voice.driftRng = voice.driftRng * 1664525u + 1013904223u;
                    float driftNoise = static_cast<float>(voice.driftRng & 0xFFFF) / 65536.0f - 0.5f;
                    voice.microtonalOffset += driftNoise * effectiveDrift * 0.1f;
                    voice.microtonalOffset = clamp(voice.microtonalOffset, -100.0f, 100.0f);
                }

                // Glide (boneless portamento)
                voice.currentFreq += (voice.targetFreq - voice.currentFreq) * voice.glideCoeff;
                voice.currentFreq = flushDenormal(voice.currentFreq);

                // Apply microtonal offset + arm pitch modulation + coupling pitch + MPE + pitch bend
                float pitchCents = pShiftMicro + voice.microtonalOffset +
                                   armMods[ArmPitch] * 50.0f // arm 3 modulates pitch +/-50 cents
                                   + blockCouplingPitchMod * 100.0f +
                                   voice.mpeExpression.pitchBendSemitones * 100.0f // MPE pitch bend in cents
                                   + pitchBendNorm * 200.0f; // channel pitch bend in cents (±2 semitones)
                float freqMod = voice.currentFreq * fastPow2(pitchCents / 1200.0f);

                // --- Envelopes ---
                float ampLevel = voice.ampEnv.process();
                float modLevel = voice.modEnv.process();

                if (!voice.ampEnv.isActive() && !voice.inkCloud.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Val = voice.lfo1.process() * pLfo1Depth;
                // LFO2 modulates chromatophore morph position (skin-color shift rate)
                float lfo2Val = voice.lfo2.process() * pLfo2Depth;

                // =====================================================
                // Core oscillator — wavetable with arm modulation
                // =====================================================

                float wtPos = clamp(smoothedWTPos + lfo1Val * 0.3f + modLevel * pWTScanRate * 0.5f +
                                        armMods[ArmWavetablePos] * 0.3f,
                                    0.0f, 1.0f);
                voice.wtOsc.setPosition(wtPos);
                voice.wtOsc.setFrequency(freqMod, srf);

                float voiceSignal = voice.wtOsc.processSample();

                // AudioToRing coupling: ring modulate the carrier with the incoming signal
                // (pre-reset snapshot used; the reset wipes couplingRingModSrc upstream)
                if (blockCouplingRingModSrc != 0.0f)
                    voiceSignal *= (1.0f + blockCouplingRingModSrc);

                // =====================================================
                // 5. SUCKERS — Ultra-fast transient pluck
                // =====================================================

                if (smoothedSuckerMix > 0.001f)
                {
                    float suckerLevel = voice.suckerEnv.process();
                    if (suckerLevel > 0.001f)
                    {
                        // Ultra-fast envelope into high-Q bandpass = sticky plonk (coeff refresh decimated)
                        if (updateFilter)
                        {
                            float suckerFreqMod = clamp(pSuckerFreq + armMods[ArmSuckerFreq] * 2000.0f, 200.0f, 8000.0f);
                            voice.suckerFilter.setCoefficients(suckerFreqMod, pSuckerReso, srf);
                        }
                        float suckerSig = voice.suckerFilter.processSample(voiceSignal) * suckerLevel * 2.0f;
                        voiceSignal =
                            voiceSignal * (1.0f - smoothedSuckerMix) + (voiceSignal + suckerSig) * smoothedSuckerMix;
                    }
                }

                // =====================================================
                // 2. CHROMATOPHORES — Adaptive morphing filter
                // =====================================================

                if (smoothedChromaDepth > 0.001f)
                {
                    // Envelope follower — tracks signal amplitude
                    // chromaCoeff is hoisted above the sample loop (block-constant)
                    float envIn = std::fabs(voiceSignal);
                    voice.envFollower += (envIn - voice.envFollower) * chromaCoeff;
                    voice.envFollower = flushDenormal(voice.envFollower);

                    // Map envelope to filter frequency modulation
                    float chromaFreqMod = clamp(pChromaFreq + voice.envFollower * pChromaSens * 4000.0f +
                                                    armMods[ArmChromaFreq] * 2000.0f,
                                                100.0f, 16000.0f);

                    // Morph filter type: LP → BP → HP → Notch
                    // We process through LP and HP, then blend
                    // LFO2 continuously shifts the chromatophore morph position
                    float voiceMorphTarget = clamp(chromaMorphTarget + lfo2Val * 0.5f, 0.0f, 1.0f);
                    if (updateFilter)
                    {
                        voice.chromaFilter.setMode(CytomicSVF::Mode::LowPass);
                        voice.chromaFilter.setCoefficients(chromaFreqMod, 0.5f + smoothedChromaDepth * 0.4f, srf);
                    }
                    float lpOut = voice.chromaFilter.processSample(voiceSignal);

                    // Use morph to blend between filter types
                    float morphedFilter;
                    if (voiceMorphTarget < 0.33f)
                    {
                        // LP dominant
                        float t = voiceMorphTarget / 0.33f;
                        morphedFilter = lpOut * (1.0f - t) + voiceSignal * t; // LP → dry (towards BP)
                    }
                    else if (voiceMorphTarget < 0.66f)
                    {
                        // BP zone — use bandpass-like response
                        float t = (voiceMorphTarget - 0.33f) / 0.33f;
                        float bpLike = voiceSignal - lpOut; // crude HP
                        morphedFilter = lpOut * (1.0f - t) + bpLike * t;
                    }
                    else
                    {
                        // HP → Notch zone
                        float t = (voiceMorphTarget - 0.66f) / 0.34f;
                        float hpLike = voiceSignal - lpOut;
                        float notchLike = voiceSignal - lpOut * 0.5f;
                        morphedFilter = hpLike * (1.0f - t) + notchLike * t;
                    }

                    voiceSignal = voiceSignal * (1.0f - smoothedChromaDepth) + morphedFilter * smoothedChromaDepth;
                }

                // --- Main filter with arm modulation ---
                // D001: continuous velocity→timbre — higher velocity opens the filter further
                if (updateFilter)
                {
                    float filterCutoffMod =
                        clamp(effectiveCutoff + armMods[ArmFilterCutoff] * 4000.0f + voice.velocity * 0.3f * 3000.0f, 20.0f,
                              20000.0f);
                    voice.mainFilter.setCoefficients(filterCutoffMod, effectiveReso, srf);
                }
                voiceSignal = voice.mainFilter.processSample(voiceSignal);

                // =====================================================
                // ArmFormantShift (arm #8) — 2-formant bandpass filter
                // armMods[7] shifts F1 (300–800 Hz) and F2 (800–2500 Hz)
                // in opposite directions, creating a vowel-shift effect.
                // =====================================================
                {
                    // arm mod spans -1..+1 → shift F1 down/up by up to 250 Hz, F2 up/down by up to 850 Hz
                    float fShift = armMods[ArmFormantShift]; // -1..+1, already depth-scaled
                    float f1Freq = clamp(550.0f + fShift * 250.0f, 200.0f, 900.0f);
                    float f2Freq = clamp(1650.0f - fShift * 850.0f, 600.0f, 3000.0f);
                    if (updateFilter)
                    {
                        voice.formant1.setCoefficients(f1Freq, 0.6f, srf);
                        voice.formant2.setCoefficients(f2Freq, 0.5f, srf);
                    }
                    float formantSig = voice.formant1.processSample(voiceSignal) * 0.6f +
                                       voice.formant2.processSample(voiceSignal) * 0.4f;
                    // Blend at a modest level — more pronounced when arm depth is high
                    float formantBlend = clamp(std::fabs(fShift) * 0.5f + effectiveArmDepth * 0.15f, 0.0f, 0.3f);
                    voiceSignal = voiceSignal * (1.0f - formantBlend) + formantSig * formantBlend;
                }

                // =====================================================
                // 3. INK CLOUD — Freeze reverb noise burst
                // =====================================================

                float inkSample = 0.0f;
                float dryMute = 1.0f; // 1.0 = dry passes, 0.0 = dry muted (ink escape)
                if (voice.inkCloud.isActive() && smoothedInkMix > 0.001f)
                {
                    inkSample = voice.inkCloud.process() * smoothedInkMix;
                    // Mute dry signal proportionally — the escape
                    dryMute = 1.0f - smoothedInkMix * voice.inkCloud.frozenGain * 0.8f;
                    dryMute = clamp(dryMute, 0.0f, 1.0f);
                }

                // --- Apply amplitude envelope, velocity, crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;

                // Arm level modulation (subtle)
                float levelMod = 1.0f + armMods[ArmLevel] * 0.15f;
                gain *= clamp(levelMod, 0.5f, 1.5f);

                // Pan spread from arm modulation
                float panMod = armMods[ArmPanSpread] * 0.5f; // -0.5 to +0.5
                float panL = clamp(0.5f - panMod, 0.0f, 1.0f);
                float panR = clamp(0.5f + panMod, 0.0f, 1.0f);

                float outL = (voiceSignal * dryMute * gain + inkSample) * panL;
                float outR = (voiceSignal * dryMute * gain + inkSample) * panR;

                // Denormal protection
                outL = flushDenormal(outL);
                outR = flushDenormal(outR);

                mixL += outL;
                mixR += outR;

                peakEnv = std::max(peakEnv, ampLevel);
            }

            // Apply master level
            float finalL = mixL * pLevel;
            float finalR = mixR * pLevel;

            // Write to output buffer
            if (buffer.getNumChannels() >= 2)
            {
                buffer.addSample(0, sample, finalL);
                buffer.addSample(1, sample, finalR);
            }
            else if (buffer.getNumChannels() == 1)
            {
                buffer.addSample(0, sample, (finalL + finalR) * 0.5f);
            }

            // Cache for coupling
            if (sample < static_cast<int>(outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t>(sample)] = finalL;
                outputCacheR[static_cast<size_t>(sample)] = finalR;
            }
        }

        envelopeOutput = peakEnv;

        int count = 0;
        for (const auto& v : voices)
            if (v.active)
                ++count;
        activeVoices.store(count, std::memory_order_relaxed);

        silenceGate.analyzeBlock(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling(int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0)
            return 0.0f;
        auto si = static_cast<size_t>(sampleIndex);
        if (channel == 0 && si < outputCacheL.size())
            return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size())
            return outputCacheR[si];
        if (channel == 2)
            return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput(CouplingType type, float amount, const float* /*sourceBuffer*/, int /*numSamples*/) override
    {
        switch (type)
        {
        case CouplingType::AudioToFM:
            couplingWTPosMod += amount * 0.5f;
            break;
        case CouplingType::AmpToFilter:
            couplingChromaMod += amount;
            break;
        case CouplingType::EnvToMorph:
            couplingArmRateMod += amount * 2.0f;
            break;
        case CouplingType::AudioToRing:
            couplingRingModSrc += amount;
            break;
        case CouplingType::LFOToPitch:
            couplingPitchMod += amount * 0.5f;
            break;
        default:
            break;
        }
    }

    //==========================================================================
    // Parameters
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl(params);
    }

    static void addParametersImpl(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        // --- Arms (8-lane polyrhythmic modulation) ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_armCount", 1}, "Octopus Arm Count",
                                                        juce::NormalisableRange<float>(1.0f, 8.0f, 1.0f), 4.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_armSpread", 1}, "Octopus Arm Spread",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_armBaseRate", 1}, "Octopus Arm Base Rate",
            juce::NormalisableRange<float>(0.05f, 20.0f, 0.01f, 0.3f), 1.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_armDepth", 1}, "Octopus Arm Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        // --- Chromatophores (Envelope Follower + Morphing Filter) ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_chromaSens", 1}, "Octopus Chroma Sensitivity",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_chromaSpeed", 1}, "Octopus Chroma Speed",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_chromaMorph", 1}, "Octopus Chroma Filter Morph",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_chromaDepth", 1}, "Octopus Chroma Depth",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_chromaFreq", 1}, "Octopus Chroma Frequency",
            juce::NormalisableRange<float>(100.0f, 16000.0f, 1.0f, 0.3f), 2000.0f));

        // --- Ink Cloud (Noise Burst + Freeze) ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_inkThreshold", 1}, "Octopus Ink Velocity Threshold",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.9f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_inkDensity", 1}, "Octopus Ink Density",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.8f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_inkDecay", 1}, "Octopus Ink Decay",
                                                        juce::NormalisableRange<float>(0.5f, 30.0f, 0.1f, 0.3f), 5.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_inkMix", 1}, "Octopus Ink Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // --- Shapeshifter (Microtonal + Glide + Drift) ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_shiftMicro", 1}, "Octopus Microtonal Offset",
            juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_shiftGlide", 1}, "Octopus Glide",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.5f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_shiftDrift", 1}, "Octopus Pitch Drift",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // --- Suckers (Sticky Transient Plucks) ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_suckerReso", 1}, "Octopus Sucker Resonance",
            juce::NormalisableRange<float>(0.0f, 0.995f, 0.001f), 0.8f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_suckerFreq", 1}, "Octopus Sucker Frequency",
            juce::NormalisableRange<float>(200.0f, 8000.0f, 1.0f, 0.3f), 2000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_suckerDecay", 1}, "Octopus Sucker Decay",
            juce::NormalisableRange<float>(0.005f, 0.5f, 0.001f, 0.3f), 0.05f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_suckerMix", 1}, "Octopus Sucker Mix",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // --- Core Oscillator ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_wtPosition", 1}, "Octopus Wavetable Position",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_wtScanRate", 1}, "Octopus WT Scan Rate",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        // --- Main Filter ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_filterCutoff", 1}, "Octopus Filter Cutoff",
            juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_filterReso", 1}, "Octopus Filter Resonance",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        // --- Level ---
        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_level", 1}, "Octopus Level",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.8f));

        // --- Amp Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_ampAttack", 1}, "Octopus Amp Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_ampDecay", 1}, "Octopus Amp Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_ampSustain", 1}, "Octopus Amp Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.7f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_ampRelease", 1}, "Octopus Amp Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- Mod Envelope ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_modAttack", 1}, "Octopus Mod Attack",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_modDecay", 1}, "Octopus Mod Decay",
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.001f, 0.3f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_modSustain", 1}, "Octopus Mod Sustain",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_modRelease", 1}, "Octopus Mod Release",
            juce::NormalisableRange<float>(0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- LFO 1 ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_lfo1Rate", 1}, "Octopus LFO1 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 0.5f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_lfo1Depth", 1}, "Octopus LFO1 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"octo_lfo1Shape", 1}, "Octopus LFO1 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // --- LFO 2 ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_lfo2Rate", 1}, "Octopus LFO2 Rate",
            juce::NormalisableRange<float>(0.01f, 30.0f, 0.01f, 0.3f), 2.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_lfo2Depth", 1}, "Octopus LFO2 Depth",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"octo_lfo2Shape", 1}, "Octopus LFO2 Shape",
            juce::StringArray{"Sine", "Triangle", "Saw", "Square", "S&H"}, 0));

        // --- Voice Mode ---
        params.push_back(
            std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"octo_polyphony", 1}, "Octopus Voice Mode",
                                                         juce::StringArray{"Mono", "Legato", "Poly8", "Poly16"}, 2));

        // --- Macros ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_macroCharacter", 1}, "Octopus Macro CHARACTER",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_macroMovement", 1}, "Octopus Macro MOVEMENT",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"octo_macroCoupling", 1}, "Octopus Macro COUPLING",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back(
            std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"octo_macroSpace", 1}, "Octopus Macro SPACE",
                                                        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // Arms
        paramArmCount = apvts.getRawParameterValue("octo_armCount");
        paramArmSpread = apvts.getRawParameterValue("octo_armSpread");
        paramArmBaseRate = apvts.getRawParameterValue("octo_armBaseRate");
        paramArmDepth = apvts.getRawParameterValue("octo_armDepth");

        // Chromatophores
        paramChromaSens = apvts.getRawParameterValue("octo_chromaSens");
        paramChromaSpeed = apvts.getRawParameterValue("octo_chromaSpeed");
        paramChromaMorph = apvts.getRawParameterValue("octo_chromaMorph");
        paramChromaDepth = apvts.getRawParameterValue("octo_chromaDepth");
        paramChromaFreq = apvts.getRawParameterValue("octo_chromaFreq");

        // Ink Cloud
        paramInkThreshold = apvts.getRawParameterValue("octo_inkThreshold");
        paramInkDensity = apvts.getRawParameterValue("octo_inkDensity");
        paramInkDecay = apvts.getRawParameterValue("octo_inkDecay");
        paramInkMix = apvts.getRawParameterValue("octo_inkMix");

        // Shapeshifter
        paramShiftMicro = apvts.getRawParameterValue("octo_shiftMicro");
        paramShiftGlide = apvts.getRawParameterValue("octo_shiftGlide");
        paramShiftDrift = apvts.getRawParameterValue("octo_shiftDrift");

        // Suckers
        paramSuckerReso = apvts.getRawParameterValue("octo_suckerReso");
        paramSuckerFreq = apvts.getRawParameterValue("octo_suckerFreq");
        paramSuckerDecay = apvts.getRawParameterValue("octo_suckerDecay");
        paramSuckerMix = apvts.getRawParameterValue("octo_suckerMix");

        // Core
        paramWTPos = apvts.getRawParameterValue("octo_wtPosition");
        paramWTScanRate = apvts.getRawParameterValue("octo_wtScanRate");
        paramFilterCutoff = apvts.getRawParameterValue("octo_filterCutoff");
        paramFilterReso = apvts.getRawParameterValue("octo_filterReso");
        paramLevel = apvts.getRawParameterValue("octo_level");

        paramAmpAttack = apvts.getRawParameterValue("octo_ampAttack");
        paramAmpDecay = apvts.getRawParameterValue("octo_ampDecay");
        paramAmpSustain = apvts.getRawParameterValue("octo_ampSustain");
        paramAmpRelease = apvts.getRawParameterValue("octo_ampRelease");

        paramModAttack = apvts.getRawParameterValue("octo_modAttack");
        paramModDecay = apvts.getRawParameterValue("octo_modDecay");
        paramModSustain = apvts.getRawParameterValue("octo_modSustain");
        paramModRelease = apvts.getRawParameterValue("octo_modRelease");

        paramLfo1Rate = apvts.getRawParameterValue("octo_lfo1Rate");
        paramLfo1Depth = apvts.getRawParameterValue("octo_lfo1Depth");
        paramLfo1Shape = apvts.getRawParameterValue("octo_lfo1Shape");
        paramLfo2Rate = apvts.getRawParameterValue("octo_lfo2Rate");
        paramLfo2Depth = apvts.getRawParameterValue("octo_lfo2Depth");
        paramLfo2Shape = apvts.getRawParameterValue("octo_lfo2Shape");

        paramVoiceMode = apvts.getRawParameterValue("octo_polyphony");

        paramMacroCharacter = apvts.getRawParameterValue("octo_macroCharacter");
        paramMacroMovement = apvts.getRawParameterValue("octo_macroMovement");
        paramMacroCoupling = apvts.getRawParameterValue("octo_macroCoupling");
        paramMacroSpace = apvts.getRawParameterValue("octo_macroSpace");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Octopus"; }

    // Chromatophore Magenta — vivid alien color-shifting pigment
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFE040FB); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices.load(std::memory_order_relaxed); }

private:
    SilenceGate silenceGate;

    //==========================================================================
    // Safe parameter load
    //==========================================================================

    static float loadParam(std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // Wavetable generation — procedural organic/fluid textures
    //==========================================================================

    static constexpr int kWTFrames = 64;
    static constexpr int kWTFrameSize = 2048;
    float octopusWavetable[kWTFrames * kWTFrameSize]{};

    void buildOctopusWavetable()
    {
        // Build 64 frames morphing from fluid sine through tentacular
        // undulations to complex, alien-textured waveforms.
        // The octopus wavetable emphasizes organic movement and non-harmonic
        // overtones — the sound of something boneless and fluid.
        for (int frame = 0; frame < kWTFrames; ++frame)
        {
            float morph = static_cast<float>(frame) / static_cast<float>(kWTFrames - 1);

            for (int s = 0; s < kWTFrameSize; ++s)
            {
                float phase = static_cast<float>(s) / static_cast<float>(kWTFrameSize);
                float sample = 0.0f;

                // Frame 0: Pure sine — calm, resting octopus
                float sine = std::sin(kTwoPi * phase);

                // Progression: add fluid, undulating overtones
                // Unlike Orca's metallic/inharmonic stretch, Octopus uses
                // sub-harmonic folding and waveshaping for organic texture
                int numPartials = 1 + static_cast<int>(morph * 12.0f);
                float totalAmp = 0.0f;

                for (int p = 1; p <= numPartials; ++p)
                {
                    float n = static_cast<float>(p);

                    // Tentacular ratios: mix of sub-octave and odd harmonics
                    // Creates a "reaching, gripping" quality
                    float ratio;
                    if (p <= 3)
                        ratio = n; // fundamental harmonics
                    else if (p % 2 == 0)
                        ratio = n * 0.75f + morph * 0.5f; // slightly flat — fluid
                    else
                        ratio = n * 1.1f + morph * 0.3f; // slightly sharp — alien

                    // Amplitude: emphasize mid-range (body of the creature)
                    float amp = 1.0f / (n * 0.7f + 0.3f);
                    if (p >= 2 && p <= 5)
                        amp *= 1.0f + morph * 1.5f; // body resonance

                    // Phase rotation per partial — creates swirling motion
                    float phaseRot = morph * kPI * 0.5f * n;

                    sample += amp * std::sin(kTwoPi * ratio * phase + phaseRot);
                    totalAmp += amp;
                }

                if (totalAmp > 0.0f)
                    sample /= totalAmp;

                // Crossfade: early = sine, later = complex tentacular
                sample = sine * (1.0f - morph * 0.8f) + sample * (0.2f + morph * 0.8f);

                // Late frames: add organic "breathing" texture via wavefold
                if (morph > 0.5f)
                {
                    float foldAmount = (morph - 0.5f) * 2.0f;
                    // Gentle wavefold — creates the bubbling, sucking quality
                    float folded = std::sin(sample * kPI * (1.0f + foldAmount));
                    sample = sample * (1.0f - foldAmount * 0.4f) + folded * foldAmount * 0.4f;
                }

                // Very late frames: micro-fluctuations (chromatophore shimmer)
                if (morph > 0.75f)
                {
                    uint32_t hash = static_cast<uint32_t>(phase * 234567.891f + frame * 891.234f);
                    hash = hash * 2654435761u;
                    float shimmer = static_cast<float>(hash & 0xFFFF) / 32768.0f - 1.0f;
                    sample += shimmer * (morph - 0.75f) * 0.1f;
                }

                octopusWavetable[frame * kWTFrameSize + s] = sample;
            }
        }
    }

    //==========================================================================
    // MIDI note handling
    //==========================================================================

    void noteOn(int noteNumber, float velocity, int midiChannel, int maxPoly, bool monoMode, bool legatoMode,
                float glideCoeff, float ampA, float ampD, float ampS, float ampR, float modA, float modD, float modS,
                float modR, float suckerDecay, float lfo1Rate, float lfo1Depth, int lfo1Shape, float lfo2Rate,
                float lfo2Depth, int lfo2Shape, float cutoff, float reso, float armBaseRate, float armSpread,
                int armCount, float inkThreshold, float inkDensity, float inkDecay, float microOffset)
    {
        float freq = 440.0f * fastPow2((static_cast<float>(noteNumber) - 69.0f) / 12.0f);

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.targetFreq = freq;
            voice.glideCoeff = glideCoeff;

            if (!wasActive || !legatoMode)
            {
                if (!wasActive)
                    voice.currentFreq = freq;

                voice.ampEnv.setParams(ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.modEnv.setParams(modA, modD, modS, modR, srf);
                voice.modEnv.noteOn();

                // Sucker envelope — ultra-fast attack, short decay
                voice.suckerEnv.setParams(0.001f, suckerDecay, 0.0f, suckerDecay, srf);
                voice.suckerEnv.noteOn();
            }

            voice.active = true;
            voice.noteNumber = noteNumber;
            voice.velocity = velocity;
            voice.fadingOut = false;
            voice.fadeGain = 1.0f;
            voice.startTime = ++voiceCounter;

            // Initialize MPE expression for this voice's channel
            voice.mpeExpression.reset();
            voice.mpeExpression.midiChannel = midiChannel;
            if (mpeManager != nullptr)
                mpeManager->updateVoiceExpression(voice.mpeExpression);

            // Add random microtonal offset per note (shapeshifter)
            voice.driftRng = voice.driftRng * 1664525u + 1013904223u;
            voice.microtonalOffset =
                microOffset + (static_cast<float>(voice.driftRng & 0xFFFF) / 65536.0f - 0.5f) * 10.0f;

            voice.lfo1.setRate(lfo1Rate, srf);
            voice.lfo1.setShape(lfo1Shape);
            voice.lfo2.setRate(lfo2Rate, srf);
            voice.lfo2.setShape(lfo2Shape);

            // Initialize arms
            for (int a = 0; a < 8; ++a)
            {
                float rate = armBaseRate + (armBaseRate * kArmPrimeRatios[a] - armBaseRate) * armSpread;
                voice.arms[a].setRate(rate, srf);
                voice.arms[a].setShape(a % 5);
            }

            voice.mainFilter.setCoefficients(cutoff, reso, srf);

            // Ink cloud trigger — maximum velocity = threat response
            if (velocity >= inkThreshold)
            {
                voice.inkCloud.freeze(inkDensity, srf);
                voice.inkCloud.setDecay(inkDecay, srf);
                voice.inkTriggered = true;
            }

            return;
        }

        // Polyphonic — find free voice or steal oldest (LRU)
        // findFreeVoice() replaced by VoiceAllocator::findFreeVoice() (Source/DSP/VoiceAllocator.h)
        int slot = VoiceAllocator::findFreeVoice(voices, std::min(maxPoly, kMaxVoices));

        if (voices[slot].active)
            voices[slot].fadingOut = true;

        auto& voice = voices[slot];
        voice.reset();
        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.currentFreq = freq;
        voice.targetFreq = freq;
        voice.glideCoeff = glideCoeff;
        voice.startTime = ++voiceCounter;
        voice.driftRng = static_cast<uint32_t>(slot * 777 + noteNumber * 31 + voiceCounter);

        // Initialize MPE expression for this voice's channel
        voice.mpeExpression.reset();
        voice.mpeExpression.midiChannel = midiChannel;
        if (mpeManager != nullptr)
            mpeManager->updateVoiceExpression(voice.mpeExpression);

        // Random microtonal offset per note
        voice.driftRng = voice.driftRng * 1664525u + 1013904223u;
        voice.microtonalOffset = microOffset + (static_cast<float>(voice.driftRng & 0xFFFF) / 65536.0f - 0.5f) * 10.0f;

        voice.ampEnv.setParams(ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();
        voice.modEnv.setParams(modA, modD, modS, modR, srf);
        voice.modEnv.noteOn();

        // Sucker — ultra-fast transient
        voice.suckerEnv.setParams(0.001f, suckerDecay, 0.0f, suckerDecay, srf);
        voice.suckerEnv.noteOn();

        voice.lfo1.setRate(lfo1Rate, srf);
        voice.lfo1.setShape(lfo1Shape);
        voice.lfo2.setRate(lfo2Rate, srf);
        voice.lfo2.setShape(lfo2Shape);

        // Initialize arms with per-voice phase offset
        for (int a = 0; a < 8; ++a)
        {
            float rate = armBaseRate + (armBaseRate * kArmPrimeRatios[a] - armBaseRate) * armSpread;
            voice.arms[a].setRate(rate, srf);
            voice.arms[a].setShape(a % 5);
            // Offset each voice's arm phases for maximum independence
            voice.arms[a].phase = static_cast<float>(slot * 0.1f + a * 0.125f);
            if (voice.arms[a].phase >= 1.0f)
                voice.arms[a].phase -= 1.0f;
        }

        voice.mainFilter.setCoefficients(cutoff, reso, srf);

        // Ink cloud trigger
        if (velocity >= inkThreshold)
        {
            voice.inkCloud.freeze(inkDensity, srf);
            voice.inkCloud.setDecay(inkDecay, srf);
            voice.inkTriggered = true;
        }
    }

    void noteOff(int noteNumber, int midiChannel = 0)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber && !voice.fadingOut)
            {
                // In MPE mode, match by channel too
                if (midiChannel > 0 && voice.mpeExpression.midiChannel > 0 &&
                    voice.mpeExpression.midiChannel != midiChannel)
                    continue;

                voice.ampEnv.noteOff();
                voice.modEnv.noteOff();
            }
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr = 0.0;  // Sentinel: must be set by prepare() before use
    float srf = 0.0f;  // Sentinel: must be set by prepare() before use
    float smoothCoeff = 0.0f;
    float crossfadeRate = 0.0f;
    uint64_t voiceCounter = 0;

    // Voices
    std::array<OctoVoice, kMaxVoices> voices{};
    std::atomic<int> activeVoices{0};

    // Output cache for coupling
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;
    float envelopeOutput = 0.0f;

    // D006: mod wheel (CC#1) — intensifies chromatophore skin-shift depth,
    // pushing the filter topology morph harder on each note. Full wheel
    // adds up to +0.4 to effectiveChromaDepth — the octopus flares its
    // full chromatophore palette in response to the performer's expression.
    float modWheelAmount_ = 0.0f;
    float pitchBendNorm = 0.0f; // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Coupling modulation accumulators
    float couplingWTPosMod = 0.0f;
    float couplingChromaMod = 0.0f;
    float couplingArmRateMod = 0.0f;
    float couplingRingModSrc = 0.0f;
    float couplingPitchMod = 0.0f;

    // Smoothed control values
    float smoothedWTPos = 0.0f;
    float smoothedChromaDepth = 0.0f;
    float smoothedInkMix = 0.0f;
    float smoothedSuckerMix = 0.0f;

    // Parameter pointers
    // Arms
    std::atomic<float>* paramArmCount = nullptr;
    std::atomic<float>* paramArmSpread = nullptr;
    std::atomic<float>* paramArmBaseRate = nullptr;
    std::atomic<float>* paramArmDepth = nullptr;

    // Chromatophores
    std::atomic<float>* paramChromaSens = nullptr;
    std::atomic<float>* paramChromaSpeed = nullptr;
    std::atomic<float>* paramChromaMorph = nullptr;
    std::atomic<float>* paramChromaDepth = nullptr;
    std::atomic<float>* paramChromaFreq = nullptr;

    // Ink Cloud
    std::atomic<float>* paramInkThreshold = nullptr;
    std::atomic<float>* paramInkDensity = nullptr;
    std::atomic<float>* paramInkDecay = nullptr;
    std::atomic<float>* paramInkMix = nullptr;

    // Shapeshifter
    std::atomic<float>* paramShiftMicro = nullptr;
    std::atomic<float>* paramShiftGlide = nullptr;
    std::atomic<float>* paramShiftDrift = nullptr;

    // Suckers
    std::atomic<float>* paramSuckerReso = nullptr;
    std::atomic<float>* paramSuckerFreq = nullptr;
    std::atomic<float>* paramSuckerDecay = nullptr;
    std::atomic<float>* paramSuckerMix = nullptr;

    // Core
    std::atomic<float>* paramWTPos = nullptr;
    std::atomic<float>* paramWTScanRate = nullptr;
    std::atomic<float>* paramFilterCutoff = nullptr;
    std::atomic<float>* paramFilterReso = nullptr;
    std::atomic<float>* paramLevel = nullptr;

    std::atomic<float>* paramAmpAttack = nullptr;
    std::atomic<float>* paramAmpDecay = nullptr;
    std::atomic<float>* paramAmpSustain = nullptr;
    std::atomic<float>* paramAmpRelease = nullptr;

    std::atomic<float>* paramModAttack = nullptr;
    std::atomic<float>* paramModDecay = nullptr;
    std::atomic<float>* paramModSustain = nullptr;
    std::atomic<float>* paramModRelease = nullptr;

    std::atomic<float>* paramLfo1Rate = nullptr;
    std::atomic<float>* paramLfo1Depth = nullptr;
    std::atomic<float>* paramLfo1Shape = nullptr;
    std::atomic<float>* paramLfo2Rate = nullptr;
    std::atomic<float>* paramLfo2Depth = nullptr;
    std::atomic<float>* paramLfo2Shape = nullptr;

    std::atomic<float>* paramVoiceMode = nullptr;

    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
};

} // namespace xoceanus
