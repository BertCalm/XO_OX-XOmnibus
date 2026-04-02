// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/VoiceAllocator.h"
#include "../../DSP/GlideProcessor.h"
#include "../../DSP/ParameterSmoother.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/WavetableOscillator.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/Effects/Compressor.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace xoceanus {

//==============================================================================
// OrcaEngine — Apex Predator Synthesis.
//
// Maps the anatomy and behavior of the Orca (Orcinus orca) directly into
// synthesizer architecture. Five biological subsystems become five DSP modules
// that couple, collide, and hunt together.
//
// SUBSYSTEMS:
//
//   1. POD DIALECT — Wavetable oscillator scanning through metallic tables
//      with a 5-band formant filter network. Heavy portamento makes pitches
//      bend like whale song. Each "pod" (voice) has its own dialect (formant
//      configuration). LFO slowly scans the wavetable position.
//
//   2. ECHOLOCATION — Resonant comb filter pinged by microscopic noise bursts.
//      A rapid retriggering LFO fires short impulses into a comb filter with
//      very high resonance. As click rate increases, delay times merge into
//      ringing metallic tones that map the acoustic space.
//
//   3. APEX HUNT — Extreme macro modulation. A single HUNT macro controls
//      filter cutoff, FM amount, wave-folding, comb resonance, and bitcrush
//      depth simultaneously. Push it up and the entire patch moves as a
//      coordinated, aggressive, devastating unit.
//
//   4. BREACH — Sidechain displacement. A massive sine/triangle sub-bass
//      layer with an internal sidechain compressor. When triggered by
//      velocity spikes or external coupling, the audio is violently ducked —
//      an 8,000-pound animal displacing the mix.
//
//   5. COUNTERSHADING — Dynamic bitcrushing with band splitting. The low-end
//      "belly" stays clean and smooth. The high-end "dorsal" is violently
//      decimated with sample-rate reduction and bitcrushing. Stark contrast:
//      pristine below, jagged above.
//
// Coupling:
//   - Output: Post-process stereo audio, envelope level (for sidechain)
//   - Input: AudioToFM (modulate wavetable position), AmpToFilter (modulate
//            formant intensity), AmpToChoke (trigger breach), EnvToMorph
//            (modulate echolocation rate), AudioToRing (ring mod source)
//
// Accent Color: Orca Black & White — Deep Ocean #1B2838
//
//==============================================================================

//==============================================================================
// ADSR envelope generator — shared fleet implementation.
//==============================================================================
using OrcaADSR = StandardADSR;

//==============================================================================
// LFO with multiple shapes — shared fleet implementation.
//==============================================================================
using OrcaLFO = StandardLFO;

//==============================================================================
// Comb filter for echolocation — resonant delay line.
//==============================================================================
struct OrcaCombFilter
{
    static constexpr int kMaxDelaySamples = 8192;

    float buffer[kMaxDelaySamples] {};
    int writePos = 0;
    float delaySamples = 100.0f;
    float feedback = 0.9f;
    float dampingCoeff = 0.3f;
    float prevSample = 0.0f;

    void setDelay (float samples) noexcept
    {
        delaySamples = clamp (samples, 1.0f, static_cast<float> (kMaxDelaySamples - 1));
    }

    void setFeedback (float fb) noexcept
    {
        feedback = clamp (fb, 0.0f, 0.995f);
    }

    void setDamping (float damp) noexcept
    {
        dampingCoeff = clamp (damp, 0.0f, 0.99f);
    }

    float processSample (float input) noexcept
    {
        // Read with linear interpolation
        float readPos = static_cast<float> (writePos) - delaySamples;
        if (readPos < 0.0f) readPos += static_cast<float> (kMaxDelaySamples);

        int idx0 = static_cast<int> (readPos);
        int idx1 = (idx0 + 1) % kMaxDelaySamples;
        float frac = readPos - static_cast<float> (idx0);
        idx0 = idx0 % kMaxDelaySamples;

        float delayed = buffer[idx0] + frac * (buffer[idx1] - buffer[idx0]);

        // 1-pole damping in feedback path
        float damped = delayed + dampingCoeff * (prevSample - delayed);
        prevSample = flushDenormal (damped);

        // Write: input + feedback
        buffer[writePos] = flushDenormal (input + damped * feedback);

        writePos = (writePos + 1) % kMaxDelaySamples;

        return delayed;
    }

    void reset() noexcept
    {
        std::memset (buffer, 0, sizeof (buffer));
        writePos = 0;
        prevSample = 0.0f;
    }
};

//==============================================================================
// OrcaVoice — per-voice state.
//==============================================================================
struct OrcaVoice
{
    bool active = false;
    int noteNumber = -1;
    float velocity = 0.0f;
    uint64_t startTime = 0;

    // MPE per-voice expression state
    MPEVoiceExpression mpeExpression;

    // Glide (Pod Dialect — whale song portamento)
    GlideProcessor glide;                         // Shared utility: frequency-domain portamento

    // Wavetable oscillator (Pod Dialect)
    WavetableOscillator wtOsc;

    // Envelopes
    OrcaADSR ampEnv;
    OrcaADSR modEnv;     // modulation envelope for PD depth / echolocation

    // LFOs
    OrcaLFO lfo1;   // wavetable position scanner (Pod Dialect)
    OrcaLFO lfo2;   // echolocation click rate

    // Formant filters (Pod Dialect — 5-band vowel network)
    CytomicSVF formant[5];

    // Echolocation comb filters (stereo pair)
    OrcaCombFilter echoL;
    OrcaCombFilter echoR;

    // Echolocation click generator state
    float clickPhase = 0.0f;
    float clickPhaseInc = 0.0f;
    uint32_t clickRng = 54321u;

    // Main output filter
    CytomicSVF mainFilter;

    // Countershading band-split filters
    CytomicSVF bandSplitLP;   // belly (low, clean)
    CytomicSVF bandSplitHP;   // dorsal (high, crushed)

    // Countershading sample-rate reduction state (per-voice, NOT thread_local)
    float crushHold = 0.0f;
    float crushCounter = 0.0f;

    // Voice stealing crossfade
    float fadeGain = 1.0f;
    bool fadingOut = false;

    void reset() noexcept
    {
        active = false;
        noteNumber = -1;
        velocity = 0.0f;
        glide.snapTo (440.0f);
        fadeGain = 1.0f;
        fadingOut = false;
        crushHold = 0.0f;
        crushCounter = 0.0f;
        clickPhase = 0.0f;
        clickPhaseInc = 0.0f;
        ampEnv.reset();
        modEnv.reset();
        lfo1.reset();
        lfo2.reset();
        wtOsc.reset();
        for (auto& f : formant)
            f.reset();
        echoL.reset();
        echoR.reset();
        mainFilter.reset();
        bandSplitLP.reset();
        bandSplitHP.reset();
    }
};

//==============================================================================
// OrcaEngine — the main engine class.
//==============================================================================
class OrcaEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices = 16;
    static constexpr float kPI = 3.14159265358979323846f;
    static constexpr float kTwoPi = 6.28318530717958647692f;

    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srf = static_cast<float> (sr);
        silenceGate.prepare (sampleRate, maxBlockSize);

        crossfadeRate = 1.0f / (0.005f * srf);

        // Smoothing for control-rate parameters (shared ParameterSmoother, 5ms)
        smoothWTPos.prepare (srf);
        smoothFormant.prepare (srf);
        smoothEchoMix.prepare (srf);
        smoothCrushMix.prepare (srf);

        outputCacheL.resize (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.resize (static_cast<size_t> (maxBlockSize), 0.0f);

        // Build procedural metallic wavetable for Pod Dialect
        buildOrcaWavetable();

        // Initialize voices
        for (auto& v : voices)
        {
            v.reset();
            v.wtOsc.loadWavetable (orcaWavetable, kWTFrames, kWTFrameSize);

            // Init formant filters as bandpass
            for (int f = 0; f < 5; ++f)
            {
                v.formant[f].reset();
                v.formant[f].setMode (CytomicSVF::Mode::BandPass);
            }
            v.mainFilter.reset();
            v.mainFilter.setMode (CytomicSVF::Mode::LowPass);
            v.bandSplitLP.reset();
            v.bandSplitLP.setMode (CytomicSVF::Mode::LowPass);
            v.bandSplitHP.reset();
            v.bandSplitHP.setMode (CytomicSVF::Mode::HighPass);
        }

        // Init breach compressor
        breachComp.prepare (sr);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();

        envelopeOutput = 0.0f;
        couplingWTPosMod = 0.0f;
        couplingFormantMod = 0.0f;
        couplingBreachTrigger = 0.0f;
        couplingEchoRateMod = 0.0f;
        couplingRingModSrc = 0.0f;

        smoothWTPos.snapTo (0.0f);
        smoothFormant.snapTo (0.0f);
        smoothEchoMix.snapTo (0.0f);
        smoothCrushMix.snapTo (0.0f);

        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //==========================================================================
    // Audio
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (numSamples <= 0) return;

        // --- ParamSnapshot: read all parameters once per block ---
        const float pWTPos        = loadParam (paramWTPos, 0.0f);
        const float pWTScanRate   = loadParam (paramWTScanRate, 0.5f);
        const float pFormantInt   = loadParam (paramFormantIntensity, 0.5f);
        const float pFormantShift = loadParam (paramFormantShift, 0.5f);
        const float pGlideVal     = loadParam (paramGlide, 0.3f);

        const float pEchoRate     = loadParam (paramEchoRate, 5.0f);
        const float pEchoReso     = loadParam (paramEchoReso, 0.85f);
        const float pEchoDamp     = loadParam (paramEchoDamp, 0.3f);
        const float pEchoMix      = loadParam (paramEchoMix, 0.0f);

        const float pHuntMacro    = loadParam (paramHuntMacro, 0.0f);

        const float pBreachSub    = loadParam (paramBreachSub, 0.5f);
        const float pBreachShape  = loadParam (paramBreachShape, 0.0f);
        const float pBreachThresh = loadParam (paramBreachThresh, -18.0f);
        const float pBreachRatio  = loadParam (paramBreachRatio, 8.0f);

        const float pCrushBits    = loadParam (paramCrushBits, 16.0f);
        const float pCrushRate    = loadParam (paramCrushDownsample, 1.0f);
        const float pCrushMix     = loadParam (paramCrushMix, 0.0f);
        const float pCrushSplit   = loadParam (paramCrushSplitFreq, 800.0f);

        const float pCutoff       = loadParam (paramFilterCutoff, 8000.0f);
        const float pReso         = loadParam (paramFilterReso, 0.0f);
        const float pLevel        = loadParam (paramLevel, 0.8f);
        const float pVelCutoffAmt = loadParam (paramVelCutoffAmt, 0.5f);

        const float pAmpA         = loadParam (paramAmpAttack, 0.01f);
        const float pAmpD         = loadParam (paramAmpDecay, 0.1f);
        const float pAmpS         = loadParam (paramAmpSustain, 0.8f);
        const float pAmpR         = loadParam (paramAmpRelease, 0.3f);
        const float pModA         = loadParam (paramModAttack, 0.01f);
        const float pModD         = loadParam (paramModDecay, 0.3f);
        const float pModS         = loadParam (paramModSustain, 0.5f);
        const float pModR         = loadParam (paramModRelease, 0.5f);

        const float pLfo1Rate     = loadParam (paramLfo1Rate, 0.2f);
        const float pLfo1Depth    = loadParam (paramLfo1Depth, 0.5f);
        const int   pLfo1Shape    = static_cast<int> (loadParam (paramLfo1Shape, 0.0f));
        const float pLfo2Rate     = loadParam (paramLfo2Rate, 8.0f);
        const float pLfo2Depth    = loadParam (paramLfo2Depth, 0.0f);
        const int   pLfo2Shape    = static_cast<int> (loadParam (paramLfo2Shape, 0.0f));

        const int   voiceModeIdx  = static_cast<int> (loadParam (paramVoiceMode, 1.0f));

        const float macroChar     = loadParam (paramMacroCharacter, 0.0f);
        const float macroMove     = loadParam (paramMacroMovement, 0.0f);
        const float macroCoup     = loadParam (paramMacroCoupling, 0.0f);
        const float macroSpace    = loadParam (paramMacroSpace, 0.0f);

        // Voice mode
        int maxPoly = kMaxVoices;
        bool monoMode = false;
        bool legatoMode = false;
        switch (voiceModeIdx)
        {
            case 0: maxPoly = 1; monoMode = true; break;
            case 1: maxPoly = 1; monoMode = true; legatoMode = true; break;
            case 2: maxPoly = 8; break;
            case 3: maxPoly = 16; break;
            default: maxPoly = 8; break;
        }

        // Glide coefficient — heavy portamento for whale song
        float glideCoeff = 1.0f;
        if (pGlideVal > 0.001f)
            glideCoeff = 1.0f - std::exp (-1.0f / (pGlideVal * srf));

        // === APEX HUNT MACRO ===
        // Single control drives filter cutoff, echolocation resonance,
        // formant intensity, crush mix, and sub level simultaneously
        float huntAmount = clamp (pHuntMacro + macroChar * 0.3f, 0.0f, 1.0f);
        float effectiveCutoff  = clamp (pCutoff + huntAmount * 8000.0f + couplingFormantMod * 4000.0f, 20.0f, 20000.0f);
        float effectiveReso    = clamp (pReso + huntAmount * 0.4f, 0.0f, 1.0f);
        float effectiveFormant = clamp (pFormantInt + huntAmount * 0.5f + macroMove * 0.3f, 0.0f, 1.0f);
        float effectiveEchoRes = clamp (pEchoReso + huntAmount * 0.1f, 0.0f, 0.995f);
        float effectiveCrush   = clamp (pCrushMix + huntAmount * 0.6f, 0.0f, 1.0f);
        float effectiveBreachSub = clamp (pBreachSub + huntAmount * 0.3f, 0.0f, 1.0f);
        // Mod wheel (CC#1) scans the wavetable position — sweeps from sine/whale-call
        // through metallic partials to complex vocal textures (D006 compliance)
        float effectiveWTPos   = clamp (pWTPos + couplingWTPosMod + macroMove * 0.3f + modWheelAmount_ * 0.5f, 0.0f, 1.0f);
        float effectiveEchoRate = clamp (pEchoRate + couplingEchoRateMod * 10.0f + macroMove * 5.0f, 0.5f, 40.0f);
        float effectiveEchoMix = clamp (pEchoMix + macroCoup * 0.4f + macroSpace * 0.5f, 0.0f, 1.0f);

        // Reset coupling accumulators
        couplingWTPosMod = 0.0f;
        couplingFormantMod = 0.0f;
        couplingBreachTrigger = 0.0f;
        couplingEchoRateMod = 0.0f;
        couplingRingModSrc = 0.0f;

        // Breach compressor settings
        breachComp.setThreshold (pBreachThresh);
        breachComp.setRatio (pBreachRatio);
        breachComp.setAttack (0.1f);   // instant attack — the slam
        breachComp.setRelease (150.0f); // pump release
        breachComp.setKnee (0.0f);     // hard knee — violent displacement

        // Formant center frequencies — orca vocal tract model
        // Shifted by pFormantShift parameter [0..1] -> frequency multiplier [0.5..2.0]
        float formantMul = 0.5f + pFormantShift * 1.5f;
        float formantFreqs[5] = {
            270.0f  * formantMul,   // F1: low vowel
            730.0f  * formantMul,   // F2: mid vowel
            2300.0f * formantMul,   // F3: nasal
            3500.0f * formantMul,   // F4: presence
            4500.0f * formantMul    // F5: air / brilliance
        };
        float formantQ[5] = { 0.7f, 0.6f, 0.55f, 0.5f, 0.45f };

        // Countershading: split frequency
        float splitFreq = clamp (pCrushSplit, 100.0f, 4000.0f);

        // Bitcrusher resolution: map [1..16] bits, with HUNT pushing it lower
        float crushBits = clamp (pCrushBits - huntAmount * 12.0f, 1.0f, 16.0f);
        float crushStep = std::pow (2.0f, crushBits);
        // Downsample factor
        float crushDownsample = clamp (pCrushRate + huntAmount * 0.8f, 1.0f, 64.0f);

        // --- Process MIDI events ---
        for (const auto metadata : midi)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), msg.getChannel(), maxPoly, monoMode, legatoMode, glideCoeff,
                        pAmpA, pAmpD, pAmpS, pAmpR, pModA, pModD, pModS, pModR,
                        pLfo1Rate, pLfo1Depth, pLfo1Shape, pLfo2Rate, pLfo2Depth, pLfo2Shape,
                        effectiveCutoff, effectiveReso, formantFreqs, formantQ);
            }
            else if (msg.isNoteOff())
                noteOff (msg.getNoteNumber(), msg.getChannel());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                reset();
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelAmount_ = msg.getControllerValue() / 127.0f;
            else if (msg.isPitchWheel()) pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
        }

        // SilenceGate: skip all DSP if engine has been silent long enough
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
                if (!voice.active) continue;
                mpeManager->updateVoiceExpression(voice.mpeExpression);
            }
        }

        // --- Update per-voice filter coefficients once per block ---
        for (auto& voice : voices)
        {
            if (!voice.active) continue;

            // D001/D006 velocity → timbre: high velocity opens the filter (brighter
            // attack), giving each note a distinct timbral character proportional to
            // how hard it was struck.  maxCutoffOffset = 3000 Hz at full depth.
            static constexpr float kMaxCutoffOffset = 3000.0f;
            float velCutoff = clamp (effectiveCutoff + pVelCutoffAmt * voice.velocity * kMaxCutoffOffset,
                                     20.0f, 20000.0f);

            voice.mainFilter.setCoefficients (velCutoff, effectiveReso, srf);

            for (int f = 0; f < 5; ++f)
                voice.formant[f].setCoefficients (formantFreqs[f], formantQ[f], srf);

            // Band-split for countershading
            voice.bandSplitLP.setCoefficients (splitFreq, 0.5f, srf);
            voice.bandSplitHP.setCoefficients (splitFreq, 0.5f, srf);

            // Echolocation comb filter: delay time from note frequency
            // Comb filter delay = sampleRate / frequency → resonates at note pitch
            // D001: high velocity slightly compresses the comb delay → higher effective
            // click rate → more frantic echolocation hunting behaviour.
            float combDelay = srf / std::max (20.0f, voice.glide.getFreq());
            float velEchoDelay = combDelay * (1.0f - pVelCutoffAmt * voice.velocity * 0.3f);
            voice.echoL.setDelay (velEchoDelay);
            voice.echoR.setDelay (velEchoDelay * 1.003f); // slight stereo spread
            voice.echoL.setFeedback (effectiveEchoRes);
            voice.echoR.setFeedback (effectiveEchoRes);
            voice.echoL.setDamping (pEchoDamp);
            voice.echoR.setDamping (pEchoDamp);

            // Echolocation click rate
            voice.clickPhaseInc = effectiveEchoRate / srf;
        }

        // Set smoother targets for this block
        smoothWTPos.set (effectiveWTPos);
        smoothFormant.set (effectiveFormant);
        smoothEchoMix.set (effectiveEchoMix);
        smoothCrushMix.set (effectiveCrush);

        float peakEnv = 0.0f;

        // --- Render sample loop ---
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Advance smoothed control-rate parameters (shared ParameterSmoother)
            float smoothedWTPos   = smoothWTPos.process();
            float smoothedFormant = smoothFormant.process();
            float smoothedEchoMix = smoothEchoMix.process();
            float smoothedCrushMix = smoothCrushMix.process();

            float mixL = 0.0f, mixR = 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                // --- Voice-stealing crossfade ---
                if (voice.fadingOut)
                {
                    voice.fadeGain -= crossfadeRate;
                    voice.fadeGain = flushDenormal (voice.fadeGain);
                    if (voice.fadeGain <= 0.0f)
                    {
                        voice.fadeGain = 0.0f;
                        voice.active = false;
                        continue;
                    }
                }

                // --- Glide (whale song portamento) ---
                voice.glide.process();

                // --- Envelopes ---
                float ampLevel = voice.ampEnv.process();
                float modLevel = voice.modEnv.process();

                if (!voice.ampEnv.isActive())
                {
                    voice.active = false;
                    continue;
                }

                // --- LFO modulation ---
                float lfo1Val = voice.lfo1.process() * pLfo1Depth;
                float lfo2Val = voice.lfo2.process() * pLfo2Depth;

                // =====================================================
                // 1. POD DIALECT — Wavetable + Formant
                // =====================================================

                // LFO1 scans wavetable position slowly (the "dialect" evolving)
                float wtPos = clamp (smoothedWTPos + lfo1Val * 0.3f + modLevel * pWTScanRate * 0.5f, 0.0f, 1.0f);
                voice.wtOsc.setPosition (wtPos);
                float mpeFreqOrc = voice.glide.getFreq() * xoceanus::fastPow2 (voice.mpeExpression.pitchBendSemitones / 12.0f)
                                                         * PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);
                voice.wtOsc.setFrequency (mpeFreqOrc, srf);

                float wtSample = voice.wtOsc.processSample();

                // Formant filter network — gives the wavetable a "speaking" quality
                float formantOut = 0.0f;
                if (smoothedFormant > 0.001f)
                {
                    for (int f = 0; f < 5; ++f)
                        formantOut += voice.formant[f].processSample (wtSample);
                    formantOut *= 0.2f;  // normalize 5-band sum

                    wtSample = wtSample * (1.0f - smoothedFormant) + formantOut * smoothedFormant;
                }

                // =====================================================
                // 2. ECHOLOCATION — Comb filter pinged by clicks
                // =====================================================

                float echoOut = 0.0f;

                if (smoothedEchoMix > 0.001f)
                {
                    // Generate impulse click (microscopic noise burst)
                    float click = 0.0f;
                    voice.clickPhase += voice.clickPhaseInc;
                    if (voice.clickPhase >= 1.0f)
                    {
                        voice.clickPhase -= 1.0f;
                        // White noise burst — 1 sample impulse
                        voice.clickRng = voice.clickRng * 1664525u + 1013904223u;
                        click = (static_cast<float> (voice.clickRng & 0xFFFF) / 32768.0f - 1.0f);
                    }

                    // Feed click through resonant comb filters
                    float echoL = voice.echoL.processSample (click);
                    float echoR = voice.echoR.processSample (click);

                    // Stereo echolocation signal
                    echoOut = (echoL + echoR) * 0.5f;
                }

                // Mix wavetable and echolocation
                float voiceSignal = wtSample * (1.0f - smoothedEchoMix * 0.5f)
                                  + echoOut * smoothedEchoMix;

                // =====================================================
                // 5. COUNTERSHADING — Band-split bitcrushing
                //    (processed before main filter for maximum contrast)
                // =====================================================

                if (smoothedCrushMix > 0.001f)
                {
                    // Split signal into belly (low) and dorsal (high)
                    float belly  = voice.bandSplitLP.processSample (voiceSignal);
                    float dorsal = voice.bandSplitHP.processSample (voiceSignal);

                    // Bitcrush the dorsal (high-frequency) band ONLY
                    // Quantize to reduced bit depth
                    float crushed = std::round (dorsal * crushStep) / crushStep;

                    // Sample-rate reduction on dorsal (per-voice state — no thread_local)
                    voice.crushCounter += 1.0f;
                    if (voice.crushCounter >= crushDownsample)
                    {
                        voice.crushHold = crushed;
                        voice.crushCounter = 0.0f;
                    }

                    // Recombine: clean belly + crushed dorsal
                    float countershaded = belly + voice.crushHold;

                    // Mix with dry signal
                    voiceSignal = voiceSignal * (1.0f - smoothedCrushMix) + countershaded * smoothedCrushMix;
                }

                // --- Main filter (LFO2 modulates cutoff per sample) ---
                {
                    static constexpr float kMaxCutoffOffsetInner = 3000.0f;
                    float baseCutoff = clamp (effectiveCutoff + pVelCutoffAmt * voice.velocity * kMaxCutoffOffsetInner,
                                             20.0f, 20000.0f);
                    float lfo2Cutoff = clamp (baseCutoff + lfo2Val * 2000.0f, 20.0f, 20000.0f);
                    voice.mainFilter.setCoefficients (lfo2Cutoff, effectiveReso, srf);
                }
                voiceSignal = voice.mainFilter.processSample (voiceSignal);

                // --- AudioToRing coupling: ring-modulate voice signal ---
                // couplingRingModSrc is accumulated in applyCouplingInput() and
                // zeroed each block before the sample loop (line ~487).
                voiceSignal *= (1.0f + couplingRingModSrc);

                // --- Apply amplitude envelope, velocity, crossfade ---
                float gain = ampLevel * voice.velocity * voice.fadeGain;
                float outL = voiceSignal * gain;
                float outR = voiceSignal * gain;

                // Denormal protection
                outL = flushDenormal (outL);
                outR = flushDenormal (outR);

                mixL += outL;
                mixR += outR;

                peakEnv = std::max (peakEnv, ampLevel);
            }

            // =====================================================
            // 4. BREACH — Sub-bass layer with sidechain displacement
            //    (rendered globally, not per-voice, for massive weight)
            // =====================================================

            if (effectiveBreachSub > 0.001f && peakEnv > 0.001f)
            {
                // Generate sub-bass: pure sine or triangle based on shape
                float subPhaseInc = 0.0f;

                // Use lowest active voice frequency for sub
                float lowestFreq = 20000.0f;
                for (const auto& v : voices)
                {
                    if (v.active && v.glide.getFreq() < lowestFreq)
                        lowestFreq = v.glide.getFreq();
                }

                if (lowestFreq < 20000.0f)
                {
                    // Sub one octave below the lowest voice
                    float subFreq = lowestFreq * 0.5f;
                    subPhaseInc = subFreq / srf;

                    breachSubPhase += subPhaseInc;
                    if (breachSubPhase >= 1.0f) breachSubPhase -= 1.0f;

                    float subSample;
                    if (pBreachShape < 0.5f)
                    {
                        // Pure sine — massive, round sub
                        subSample = fastSin (breachSubPhase * kTwoPi);
                    }
                    else
                    {
                        // Triangle — slightly more presence
                        subSample = 4.0f * std::fabs (breachSubPhase - 0.5f) - 1.0f;
                    }

                    subSample *= effectiveBreachSub * peakEnv;

                    mixL += subSample;
                    mixR += subSample;
                }
            }

            // Apply master level
            float finalL = mixL * pLevel;
            float finalR = mixR * pLevel;

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

            // Cache for coupling
            if (sample < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sample)] = finalL;
                outputCacheR[static_cast<size_t> (sample)] = finalR;
            }
        }

        // Apply breach (sidechain) displacement to the output buffer
        // Uses the envelope peak as an internal trigger
        if (effectiveBreachSub > 0.001f && buffer.getNumChannels() >= 2)
        {
            float* left  = buffer.getWritePointer (0);
            float* right = buffer.getWritePointer (1);
            breachComp.processBlock (left, right, numSamples);
        }

        envelopeOutput = peakEnv;

        int count = 0;
        for (const auto& v : voices)
            if (v.active) ++count;
        activeVoices.store(count, std::memory_order_relaxed);

        // SilenceGate: analyze output level for next-block bypass decision
        silenceGate.analyzeBlock (buffer.getReadPointer (0),
                                  buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr,
                                  numSamples);
    }

    //==========================================================================
    // Coupling
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0) return 0.0f;
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
                couplingWTPosMod += amount * 0.5f;
                break;
            case CouplingType::AmpToFilter:
                couplingFormantMod += amount;
                break;
            case CouplingType::AmpToChoke:
                couplingBreachTrigger += amount;
                break;
            case CouplingType::EnvToMorph:
                couplingEchoRateMod += amount * 0.3f;
                break;
            case CouplingType::AudioToRing:
                couplingRingModSrc += amount;
                break;
            case CouplingType::LFOToPitch:
                // Modulate echolocation click rate
                couplingEchoRateMod += amount * 5.0f;
                break;
            default:
                break;
        }
    }

    //==========================================================================
    // Parameters
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
        // --- Pod Dialect (Wavetable + Formant) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_wtPosition", 1 }, "Orca Wavetable Position",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_wtScanRate", 1 }, "Orca WT Scan Rate",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_formantIntensity", 1 }, "Orca Formant Intensity",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_formantShift", 1 }, "Orca Formant Shift",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_glide", 1 }, "Orca Glide",
            juce::NormalisableRange<float> (0.0f, 5.0f, 0.001f, 0.5f), 0.3f));

        // --- Echolocation (Comb Filter + Pinging) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_echoRate", 1 }, "Orca Echo Click Rate",
            juce::NormalisableRange<float> (0.5f, 40.0f, 0.01f, 0.3f), 5.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_echoReso", 1 }, "Orca Echo Resonance",
            juce::NormalisableRange<float> (0.0f, 0.995f, 0.001f), 0.85f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_echoDamp", 1 }, "Orca Echo Damping",
            juce::NormalisableRange<float> (0.0f, 0.99f, 0.001f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_echoMix", 1 }, "Orca Echo Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // --- Apex Hunt (Master Macro) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_huntMacro", 1 }, "Orca Hunt",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        // --- Breach (Sub + Sidechain) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_breachSub", 1 }, "Orca Breach Sub Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_breachShape", 1 }, "Orca Breach Sub Shape",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_breachThreshold", 1 }, "Orca Breach Threshold",
            juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -18.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_breachRatio", 1 }, "Orca Breach Ratio",
            juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f), 8.0f));

        // --- Countershading (Bitcrusher) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_crushBits", 1 }, "Orca Crush Bits",
            juce::NormalisableRange<float> (1.0f, 16.0f, 0.1f), 16.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_crushDownsample", 1 }, "Orca Crush Downsample",
            juce::NormalisableRange<float> (1.0f, 64.0f, 0.1f, 0.3f), 1.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_crushMix", 1 }, "Orca Crush Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_crushSplitFreq", 1 }, "Orca Crush Split Freq",
            juce::NormalisableRange<float> (100.0f, 4000.0f, 1.0f, 0.3f), 800.0f));

        // --- Main Filter ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_filterCutoff", 1 }, "Orca Filter Cutoff",
            juce::NormalisableRange<float> (20.0f, 20000.0f, 0.1f, 0.3f), 8000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_filterReso", 1 }, "Orca Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        // --- Level ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_level", 1 }, "Orca Level",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        // --- Amp Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_ampAttack", 1 }, "Orca Amp Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_ampDecay", 1 }, "Orca Amp Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.1f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_ampSustain", 1 }, "Orca Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_ampRelease", 1 }, "Orca Amp Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.3f));

        // --- Mod Envelope ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_modAttack", 1 }, "Orca Mod Attack",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.01f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_modDecay", 1 }, "Orca Mod Decay",
            juce::NormalisableRange<float> (0.0f, 10.0f, 0.001f, 0.3f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_modSustain", 1 }, "Orca Mod Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_modRelease", 1 }, "Orca Mod Release",
            juce::NormalisableRange<float> (0.0f, 20.0f, 0.001f, 0.3f), 0.5f));

        // --- LFO 1 (Pod Dialect scanner) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_lfo1Rate", 1 }, "Orca LFO1 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 0.2f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_lfo1Depth", 1 }, "Orca LFO1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "orca_lfo1Shape", 1 }, "Orca LFO1 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- LFO 2 (Echolocation modulation) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_lfo2Rate", 1 }, "Orca LFO2 Rate",
            juce::NormalisableRange<float> (0.01f, 30.0f, 0.01f, 0.3f), 8.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_lfo2Depth", 1 }, "Orca LFO2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "orca_lfo2Shape", 1 }, "Orca LFO2 Shape",
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));

        // --- Voice Mode ---
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { "orca_polyphony", 1 }, "Orca Voice Mode",
            juce::StringArray { "Mono", "Legato", "Poly8", "Poly16" }, 1));

        // --- Velocity → Timbre (D001 / D006) ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_velCutoffAmt", 1 }, "Orca Velocity \xe2\x86\x92 Brightness",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // --- Macros ---
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_macroCharacter", 1 }, "Orca Macro CHARACTER",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_macroMovement", 1 }, "Orca Macro MOVEMENT",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_macroCoupling", 1 }, "Orca Macro COUPLING",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "orca_macroSpace", 1 }, "Orca Macro SPACE",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        paramWTPos             = apvts.getRawParameterValue ("orca_wtPosition");
        paramWTScanRate        = apvts.getRawParameterValue ("orca_wtScanRate");
        paramFormantIntensity  = apvts.getRawParameterValue ("orca_formantIntensity");
        paramFormantShift      = apvts.getRawParameterValue ("orca_formantShift");
        paramGlide             = apvts.getRawParameterValue ("orca_glide");

        paramEchoRate          = apvts.getRawParameterValue ("orca_echoRate");
        paramEchoReso          = apvts.getRawParameterValue ("orca_echoReso");
        paramEchoDamp          = apvts.getRawParameterValue ("orca_echoDamp");
        paramEchoMix           = apvts.getRawParameterValue ("orca_echoMix");

        paramHuntMacro         = apvts.getRawParameterValue ("orca_huntMacro");

        paramBreachSub         = apvts.getRawParameterValue ("orca_breachSub");
        paramBreachShape       = apvts.getRawParameterValue ("orca_breachShape");
        paramBreachThresh      = apvts.getRawParameterValue ("orca_breachThreshold");
        paramBreachRatio       = apvts.getRawParameterValue ("orca_breachRatio");

        paramCrushBits         = apvts.getRawParameterValue ("orca_crushBits");
        paramCrushDownsample   = apvts.getRawParameterValue ("orca_crushDownsample");
        paramCrushMix          = apvts.getRawParameterValue ("orca_crushMix");
        paramCrushSplitFreq    = apvts.getRawParameterValue ("orca_crushSplitFreq");

        paramFilterCutoff      = apvts.getRawParameterValue ("orca_filterCutoff");
        paramFilterReso        = apvts.getRawParameterValue ("orca_filterReso");
        paramLevel             = apvts.getRawParameterValue ("orca_level");

        paramAmpAttack         = apvts.getRawParameterValue ("orca_ampAttack");
        paramAmpDecay          = apvts.getRawParameterValue ("orca_ampDecay");
        paramAmpSustain        = apvts.getRawParameterValue ("orca_ampSustain");
        paramAmpRelease        = apvts.getRawParameterValue ("orca_ampRelease");

        paramModAttack         = apvts.getRawParameterValue ("orca_modAttack");
        paramModDecay          = apvts.getRawParameterValue ("orca_modDecay");
        paramModSustain        = apvts.getRawParameterValue ("orca_modSustain");
        paramModRelease        = apvts.getRawParameterValue ("orca_modRelease");

        paramLfo1Rate          = apvts.getRawParameterValue ("orca_lfo1Rate");
        paramLfo1Depth         = apvts.getRawParameterValue ("orca_lfo1Depth");
        paramLfo1Shape         = apvts.getRawParameterValue ("orca_lfo1Shape");
        paramLfo2Rate          = apvts.getRawParameterValue ("orca_lfo2Rate");
        paramLfo2Depth         = apvts.getRawParameterValue ("orca_lfo2Depth");
        paramLfo2Shape         = apvts.getRawParameterValue ("orca_lfo2Shape");

        paramVoiceMode         = apvts.getRawParameterValue ("orca_polyphony");

        paramVelCutoffAmt      = apvts.getRawParameterValue ("orca_velCutoffAmt");

        paramMacroCharacter    = apvts.getRawParameterValue ("orca_macroCharacter");
        paramMacroMovement     = apvts.getRawParameterValue ("orca_macroMovement");
        paramMacroCoupling     = apvts.getRawParameterValue ("orca_macroCoupling");
        paramMacroSpace        = apvts.getRawParameterValue ("orca_macroSpace");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String getEngineId() const override { return "Orca"; }

    // Deep Ocean — stark black/white contrast of orca markings
    juce::Colour getAccentColour() const override { return juce::Colour (0xFF1B2838); }

    int getMaxVoices() const override { return kMaxVoices; }

    int getActiveVoiceCount() const override { return activeVoices.load(std::memory_order_relaxed); }

private:
    SilenceGate silenceGate;

    //==========================================================================
    // Safe parameter load
    //==========================================================================

    static float loadParam (std::atomic<float>* p, float fallback) noexcept
    {
        return (p != nullptr) ? p->load() : fallback;
    }

    //==========================================================================
    // Wavetable generation — procedural metallic/whale-call textures
    //==========================================================================

    static constexpr int kWTFrames = 64;
    static constexpr int kWTFrameSize = 2048;
    float orcaWavetable[kWTFrames * kWTFrameSize] {};

    void buildOrcaWavetable()
    {
        // Build 64 frames morphing from pure sine through metallic harmonics
        // to complex, vocal-like textures — the sonic palette of orca vocalizations
        for (int frame = 0; frame < kWTFrames; ++frame)
        {
            float morph = static_cast<float> (frame) / static_cast<float> (kWTFrames - 1);

            for (int s = 0; s < kWTFrameSize; ++s)
            {
                float phase = static_cast<float> (s) / static_cast<float> (kWTFrameSize);
                float sample = 0.0f;

                // Frame 0: Pure sine (whale call fundamental)
                float sine = std::sin (kTwoPi * phase);

                // Frame progression: add increasingly inharmonic partials
                // These create the metallic, eerie quality of orca calls
                int numPartials = 1 + static_cast<int> (morph * 15.0f);
                float totalAmp = 0.0f;

                for (int p = 1; p <= numPartials; ++p)
                {
                    float n = static_cast<float> (p);

                    // Inharmonic stretching — creates metallic, bell-like quality
                    float stretchedRatio = n * (1.0f + morph * 0.03f * n);

                    // Amplitude falls off with partial number, but mid partials
                    // are boosted for vocal quality
                    float amp = 1.0f / (n * n * 0.5f + 0.5f);
                    if (p >= 3 && p <= 7)
                        amp *= 1.0f + morph * 2.0f;  // formant-like boost

                    // Alternate phase offsets for complexity
                    float phaseOffset = (p % 3 == 0) ? 0.25f : 0.0f;

                    sample += amp * std::sin (kTwoPi * (stretchedRatio * phase + phaseOffset));
                    totalAmp += amp;
                }

                // Normalize
                if (totalAmp > 0.0f)
                    sample /= totalAmp;

                // Crossfade: early frames are sine-dominant, later are complex
                sample = sine * (1.0f - morph * 0.7f) + sample * (0.3f + morph * 0.7f);

                // Late frames: add slight noise/breath component
                if (morph > 0.6f)
                {
                    // Deterministic "noise" from hashed phase
                    uint32_t hash = static_cast<uint32_t> (phase * 123456.789f + frame * 789.123f);
                    hash = hash * 2654435761u;
                    float noise = static_cast<float> (hash & 0xFFFF) / 32768.0f - 1.0f;
                    sample += noise * (morph - 0.6f) * 0.15f;
                }

                orcaWavetable[frame * kWTFrameSize + s] = sample;
            }
        }
    }

    //==========================================================================
    // MIDI note handling
    //==========================================================================

    void noteOn (int noteNumber, float velocity, int midiChannel, int maxPoly,
                 bool monoMode, bool legatoMode, float glideCoeff,
                 float ampA, float ampD, float ampS, float ampR,
                 float modA, float modD, float modS, float modR,
                 float lfo1Rate, float lfo1Depth, int lfo1Shape,
                 float lfo2Rate, float lfo2Depth, int lfo2Shape,
                 float cutoff, float reso,
                 const float formantFreqs[5], const float formantQ[5])
    {
        float freq = 440.0f * fastPow2 ((static_cast<float> (noteNumber) - 69.0f) / 12.0f);

        if (monoMode)
        {
            auto& voice = voices[0];
            bool wasActive = voice.active;

            voice.glide.setTarget (freq);
            voice.glide.setCoeff (glideCoeff);

            if (!wasActive || !legatoMode)
            {
                if (!wasActive)
                    voice.glide.snapTo (freq);

                voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
                voice.ampEnv.noteOn();
                voice.modEnv.setParams (modA, modD, modS, modR, srf);
                voice.modEnv.noteOn();
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

            voice.lfo1.setRate (lfo1Rate, srf);
            voice.lfo1.setShape (lfo1Shape);
            voice.lfo2.setRate (lfo2Rate, srf);
            voice.lfo2.setShape (lfo2Shape);

            voice.mainFilter.setCoefficients (cutoff, reso, srf);
            for (int f = 0; f < 5; ++f)
                voice.formant[f].setCoefficients (formantFreqs[f], formantQ[f], srf);

            return;
        }

        // Polyphonic — find free voice or steal oldest (LRU)
        int slot = VoiceAllocator::findFreeVoice (voices, std::min (maxPoly, kMaxVoices));

        if (voices[slot].active)
        {
            // Voice stealing: crossfade out the oldest
            voices[slot].fadingOut = true;
        }

        auto& voice = voices[slot];
        voice.reset();
        voice.active = true;
        voice.noteNumber = noteNumber;
        voice.velocity = velocity;
        voice.glide.snapTo (freq);         // No glide in poly mode (instant pitch)
        voice.startTime = ++voiceCounter;

        // Initialize MPE expression for this voice's channel
        voice.mpeExpression.reset();
        voice.mpeExpression.midiChannel = midiChannel;
        if (mpeManager != nullptr)
            mpeManager->updateVoiceExpression(voice.mpeExpression);

        voice.ampEnv.setParams (ampA, ampD, ampS, ampR, srf);
        voice.ampEnv.noteOn();
        voice.modEnv.setParams (modA, modD, modS, modR, srf);
        voice.modEnv.noteOn();

        voice.lfo1.setRate (lfo1Rate, srf);
        voice.lfo1.setShape (lfo1Shape);
        voice.lfo2.setRate (lfo2Rate, srf);
        voice.lfo2.setShape (lfo2Shape);

        voice.mainFilter.setCoefficients (cutoff, reso, srf);
        for (int f = 0; f < 5; ++f)
            voice.formant[f].setCoefficients (formantFreqs[f], formantQ[f], srf);
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
                voice.modEnv.noteOff();
            }
        }
    }

    //==========================================================================
    // State
    //==========================================================================

    double sr = 44100.0;
    float srf = 44100.0f;
    float crossfadeRate = 0.0f;
    uint64_t voiceCounter = 0;

    // MIDI expression
    float modWheelAmount_ = 0.0f;   // CC#1 — scans wavetable position (D006)
    float pitchBendNorm   = 0.0f;   // MIDI pitch wheel [-1, +1]; ±2 semitone range

    // Voices
    std::array<OrcaVoice, kMaxVoices> voices {};
    std::atomic<int> activeVoices{0};

    // Breach sub-bass state
    float breachSubPhase = 0.0f;
    Compressor breachComp;

    // Output cache for coupling
    std::vector<float> outputCacheL;
    std::vector<float> outputCacheR;
    float envelopeOutput = 0.0f;

    // Coupling modulation accumulators
    float couplingWTPosMod = 0.0f;
    float couplingFormantMod = 0.0f;
    float couplingBreachTrigger = 0.0f;
    float couplingEchoRateMod = 0.0f;
    float couplingRingModSrc = 0.0f;

    // Smoothed control values (shared ParameterSmoother, 5ms time constant)
    ParameterSmoother smoothWTPos;
    ParameterSmoother smoothFormant;
    ParameterSmoother smoothEchoMix;
    ParameterSmoother smoothCrushMix;

    // Parameter pointers
    std::atomic<float>* paramWTPos = nullptr;
    std::atomic<float>* paramWTScanRate = nullptr;
    std::atomic<float>* paramFormantIntensity = nullptr;
    std::atomic<float>* paramFormantShift = nullptr;
    std::atomic<float>* paramGlide = nullptr;

    std::atomic<float>* paramEchoRate = nullptr;
    std::atomic<float>* paramEchoReso = nullptr;
    std::atomic<float>* paramEchoDamp = nullptr;
    std::atomic<float>* paramEchoMix = nullptr;

    std::atomic<float>* paramHuntMacro = nullptr;

    std::atomic<float>* paramBreachSub = nullptr;
    std::atomic<float>* paramBreachShape = nullptr;
    std::atomic<float>* paramBreachThresh = nullptr;
    std::atomic<float>* paramBreachRatio = nullptr;

    std::atomic<float>* paramCrushBits = nullptr;
    std::atomic<float>* paramCrushDownsample = nullptr;
    std::atomic<float>* paramCrushMix = nullptr;
    std::atomic<float>* paramCrushSplitFreq = nullptr;

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

    std::atomic<float>* paramVelCutoffAmt = nullptr;

    std::atomic<float>* paramMacroCharacter = nullptr;
    std::atomic<float>* paramMacroMovement = nullptr;
    std::atomic<float>* paramMacroCoupling = nullptr;
    std::atomic<float>* paramMacroSpace = nullptr;
};

} // namespace xoceanus
