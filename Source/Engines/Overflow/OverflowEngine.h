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

namespace xomnibus {

//==============================================================================
//
//  OVERFLOW ENGINE — Pressure Pad (Pressure Cooking)
//  XO_OX Designs | XOmnibus Multi-Engine Synthesizer
//
//  CREATURE IDENTITY:
//      XOverflow is a pressure cooker. Energy accumulates with playing density.
//      When threshold is exceeded, a valve release event fires — burst of
//      harmonics, brief silence, restart. The pad has consequences.
//
//  PHYSICS: Clausius-Clapeyron Equation
//      dP/dT = L / (T * deltaV)
//      Pressure accumulates from MIDI input density. The saturation curve
//      follows the steam table — threshold is not constant but depends on
//      the current temperature (intensity of ongoing input).
//
//  DSP ARCHITECTURE:
//      1. Multi-partial oscillator bank (up to 16 harmonics)
//      2. Pressure accumulator: tracks MIDI density, velocity, note count
//      3. Pre-release strain: as pressure approaches threshold, sound
//         hardens — high-freq grating, low-freq tightening, beating
//      4. Valve release: explosive harmonic burst on threshold crossing
//         - Gradual (0): slow pressure bleed with spectral expansion
//         - Explosive (1): full burst then silence then restart
//         - Whistle (2): pitched release (steam whistle FM burst)
//      5. Over-pressure catastrophic mode: if player never releases,
//         sound saturates, hardens, then shatters (explosive reverb burst)
//
//  TIME RELATIONSHIP: Potential Energy (phrase-scale, accumulation-based)
//
//  4 MACROS:
//      flow_macroCharacter — vessel material (affects timbre during pressure)
//      flow_macroMovement  — accumulation rate + LFO depth
//      flow_macroCoupling  — pressure sensitivity to coupling input
//      flow_macroSpace     — release reverb size + stereo spread
//
//  COOPERATIVE COUPLING (BROTH):
//      Reads XOverworn's concentrateDark — a concentrated broth builds
//      pressure faster (lower threshold). The flavor is intense.
//
//  ACCENT COLOR: Steam White #E8E8E8
//  PARAMETER PREFIX: flow_
//  ENGINE ID: "Overflow"
//
//  DOCTRINES: D001–D006 compliant
//
//==============================================================================

//==============================================================================
// PressureState — Internal pressure accumulator
//==============================================================================
struct PressureState
{
    float pressure       = 0.0f;   // 0.0 → threshold (then release)
    float temperature    = 0.0f;   // current input intensity (affects threshold)
    float strainLevel    = 0.0f;   // 0.0–1.0: how close to valve release
    bool  valveOpen      = false;  // true during release event
    float valveTimer     = 0.0f;   // seconds since valve opened
    float valveDuration  = 0.5f;   // how long the release event lasts
    bool  overPressure   = false;  // catastrophic over-pressure state
    float overPressureTimer = 0.0f;

    void reset() noexcept
    {
        pressure = 0.0f;
        temperature = 0.0f;
        strainLevel = 0.0f;
        valveOpen = false;
        valveTimer = 0.0f;
        overPressure = false;
        overPressureTimer = 0.0f;
    }
};

//==============================================================================
// OverflowVoice
//==============================================================================
struct OverflowVoice
{
    static constexpr int kNumPartials = 16;

    bool     active      = false;
    uint64_t startTime   = 0;
    int      currentNote = 60;
    float    velocity    = 0.0f;
    float    fundamental = 440.0f;

    StandardADSR ampEnv;
    FilterEnvelope filterEnv;

    float partialPhase[kNumPartials] = {};

    CytomicSVF voiceFilter;

    void reset() noexcept
    {
        active = false;
        ampEnv.reset();
        filterEnv.stage = FilterEnvelope::Stage::Idle;
        filterEnv.level = 0.0f;
        voiceFilter.reset();
        std::fill (std::begin (partialPhase), std::end (partialPhase), 0.0f);
    }

    void prepare (float sampleRate) noexcept
    {
        ampEnv.prepare (sampleRate);
        filterEnv.prepare (sampleRate);
        voiceFilter.setMode (CytomicSVF::Mode::LowPass);
        voiceFilter.reset();
    }

    void noteOn (int note, float vel, float sr, uint64_t time) noexcept
    {
        active = true;
        currentNote = note;
        velocity = vel;
        startTime = time;
        fundamental = 440.0f * fastPow2 ((static_cast<float> (note) - 69.0f) / 12.0f);

        ampEnv.noteOn();
        filterEnv.trigger();
    }

    void noteOff() noexcept
    {
        ampEnv.noteOff();
        filterEnv.release();
    }
};

//==============================================================================
// OverflowEngine
//==============================================================================
class OverflowEngine : public SynthEngine
{
public:
    OverflowEngine() = default;

    static constexpr int kMaxVoices = 8;

    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;
        srF = static_cast<float> (sampleRate);
        blockSize = maxBlockSize;

        for (auto& v : voices)
        {
            v.prepare (srF);
            v.reset();
        }

        lfo1.setShape (StandardLFO::Sine);
        lfo1.reset();
        lfo2.setShape (StandardLFO::Triangle);
        lfo2.reset();
        breathLfo.setRate (0.009f, srF);

        pressureState.reset();
        noteCounter = 0;

        // Release burst noise RNG
        noiseRng = 42u;

        silenceGate.prepare (sr, maxBlockSize);
        silenceGate.setHoldTime (500.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        lfo1.reset();
        lfo2.reset();
        pressureState.reset();
        lastSampleL = lastSampleR = 0.0f;
        extFilterMod = extRingMod = 0.0f;
    }

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;

        // 1. Parse MIDI — accumulate pressure from input events
        float midiPressureInput = 0.0f;
        for (const auto& meta : midi)
        {
            auto msg = meta.getMessage();
            if (msg.isNoteOn())
            {
                silenceGate.wake();
                int idx = VoiceAllocator::findFreeVoicePreferRelease (
                    voices, kMaxVoices,
                    [] (const OverflowVoice& v) { return v.ampEnv.getStage() == StandardADSR::Stage::Release; });
                voices[idx].noteOn (msg.getNoteNumber(), msg.getFloatVelocity(), srF, ++noteCounter);

                // Pressure accumulation from note-on
                midiPressureInput += msg.getFloatVelocity() * 0.1f;

                // Dissonant intervals add more pressure (interval tension)
                // Simple heuristic: notes closer than 3 semitones or tritone add extra
                for (int v2 = 0; v2 < kMaxVoices; ++v2)
                {
                    if (v2 == idx || !voices[v2].active) continue;
                    int interval = std::abs (voices[v2].currentNote - msg.getNoteNumber()) % 12;
                    if (interval == 1 || interval == 2 || interval == 6)
                        midiPressureInput += 0.05f;  // dissonance = more pressure
                }
            }
            else if (msg.isNoteOff())
            {
                int idx = VoiceAllocator::findVoiceForNote (voices, kMaxVoices, msg.getNoteNumber());
                if (idx >= 0) voices[idx].noteOff();
            }
            else if (msg.isAftertouch() || msg.isChannelPressure())
            {
                aftertouch = msg.isAftertouch()
                    ? msg.getAfterTouchValue() / 127.0f
                    : msg.getChannelPressureValue() / 127.0f;
                midiPressureInput += aftertouch * 0.02f;
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheel = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendNorm = PitchBendUtil::parsePitchWheel (msg.getPitchWheelValue());
            }
        }

        // 2. Bypass check
        if (silenceGate.isBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // 3. Read parameters
        float pThreshold   = pThresholdParam   ? pThresholdParam->load()   : 0.7f;
        float pAccumRate   = pAccumRateParam   ? pAccumRateParam->load()   : 0.5f;
        float pValveType   = pValveTypeParam   ? pValveTypeParam->load()   : 0.0f;
        float pVesselSize  = pVesselSizeParam  ? pVesselSizeParam->load()  : 0.5f;
        float pStrainColor = pStrainColorParam ? pStrainColorParam->load() : 0.5f;
        float pReleaseTime = pReleaseTimeParam ? pReleaseTimeParam->load() : 0.5f;
        float pWhistlePitch= pWhistlePitchParam? pWhistlePitchParam->load(): 2000.0f;
        float pStereoWidth = pStereoWidthParam ? pStereoWidthParam->load() : 0.5f;
        float pFilterCut   = pFilterCutParam   ? pFilterCutParam->load()   : 6000.0f;
        float pFilterRes   = pFilterResParam   ? pFilterResParam->load()   : 0.2f;
        float pFiltEnvAmt  = pFiltEnvAmtParam  ? pFiltEnvAmtParam->load()  : 0.3f;
        float pAmpA        = pAmpAParam        ? pAmpAParam->load()        : 0.2f;
        float pAmpD        = pAmpDParam        ? pAmpDParam->load()        : 0.5f;
        float pAmpS        = pAmpSParam        ? pAmpSParam->load()        : 0.8f;
        float pAmpR        = pAmpRParam        ? pAmpRParam->load()        : 1.5f;
        float pFiltA       = pFiltAParam       ? pFiltAParam->load()       : 0.1f;
        float pFiltD       = pFiltDParam       ? pFiltDParam->load()       : 0.3f;
        float pFiltS       = pFiltSParam       ? pFiltSParam->load()       : 0.5f;
        float pFiltR       = pFiltRParam       ? pFiltRParam->load()       : 0.8f;
        float pLfo1Rate    = pLfo1RateParam    ? pLfo1RateParam->load()    : 0.15f;
        float pLfo1Depth   = pLfo1DepthParam   ? pLfo1DepthParam->load()   : 0.2f;
        float pLfo2Rate    = pLfo2RateParam    ? pLfo2RateParam->load()    : 0.07f;
        float pLfo2Depth   = pLfo2DepthParam   ? pLfo2DepthParam->load()   : 0.15f;
        float pLevel       = pLevelParam       ? pLevelParam->load()       : 0.8f;

        // Macros
        float pMC = pMacroCharacterParam ? pMacroCharacterParam->load() : 0.0f;
        float pMM = pMacroMovementParam  ? pMacroMovementParam->load()  : 0.0f;
        float pMK = pMacroCouplingParam  ? pMacroCouplingParam->load()  : 0.0f;
        float pMS = pMacroSpaceParam     ? pMacroSpaceParam->load()     : 0.0f;

        // CHARACTER → vessel material + strain color
        pStrainColor = clamp (pStrainColor + pMC * 0.3f, 0.0f, 1.0f);
        // MOVEMENT → accumulation rate + LFO
        pAccumRate = clamp (pAccumRate + pMM * 0.3f, 0.0f, 1.0f);
        pLfo1Depth = clamp (pLfo1Depth + pMM * 0.2f, 0.0f, 1.0f);
        // COUPLING → pressure sensitivity
        pThreshold = clamp (pThreshold - pMK * 0.2f, 0.1f, 1.0f);  // lower threshold
        // SPACE → stereo + release reverb
        pStereoWidth = clamp (pStereoWidth + pMS * 0.3f, 0.0f, 1.0f);
        pReleaseTime = clamp (pReleaseTime + pMS * 0.3f, 0.1f, 2.0f);

        // D006: Mod wheel → vessel size (tone)
        pVesselSize = clamp (pVesselSize + modWheel * 0.4f, 0.0f, 1.0f);
        // D006: Aftertouch → adds pressure directly
        // (already accumulated in midiPressureInput above)

        // Coupling
        pFilterCut = clamp (pFilterCut + extFilterMod, 50.0f, 16000.0f);

        // BROTH cooperative: concentrated broth lowers threshold
        pThreshold = clamp (pThreshold - brothConcentrateDark * 0.2f, 0.1f, 1.0f);

        lfo1.setRate (pLfo1Rate, srF);
        lfo2.setRate (pLfo2Rate, srF);

        int valveTypeInt = static_cast<int> (pValveType + 0.5f);

        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1
                        ? buffer.getWritePointer (1) : nullptr;

        const float inverseSr = 1.0f / srF;
        const float pitchBendRatio = PitchBendUtil::semitonesToFreqRatio (pitchBendNorm * 2.0f);

        // Apply MIDI pressure input (spread across block)
        float pressurePerSample = midiPressureInput * pAccumRate / static_cast<float> (numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            float lfo1Val = lfo1.process();
            float lfo2Val = lfo2.process();
            float breathVal = breathLfo.process();

            // === UPDATE PRESSURE STATE ===
            pressureState.pressure = clamp (pressureState.pressure + pressurePerSample, 0.0f, 2.0f);

            // Natural pressure decay (slow bleed)
            pressureState.pressure *= (1.0f - 0.00001f * inverseSr * 44100.0f);
            pressureState.pressure = flushDenormal (pressureState.pressure);

            // Strain level: 0 at pressure=0, 1 at threshold
            pressureState.strainLevel = clamp (pressureState.pressure / pThreshold, 0.0f, 1.5f);

            // Check for valve release
            if (!pressureState.valveOpen && pressureState.strainLevel >= 1.0f)
            {
                pressureState.valveOpen = true;
                pressureState.valveTimer = 0.0f;
                pressureState.valveDuration = pReleaseTime;
            }

            // Check for over-pressure (strain > 1.3 without release)
            if (pressureState.strainLevel > 1.3f && !pressureState.overPressure)
            {
                pressureState.overPressure = true;
                pressureState.overPressureTimer = 0.0f;
            }

            // Process valve release
            float releaseGain = 0.0f;
            float releaseSample = 0.0f;
            if (pressureState.valveOpen)
            {
                pressureState.valveTimer += inverseSr;
                float releaseProgress = pressureState.valveTimer / pressureState.valveDuration;

                if (releaseProgress >= 1.0f)
                {
                    // Valve closes — pressure drops to zero, restart cycle
                    pressureState.valveOpen = false;
                    pressureState.pressure = 0.0f;
                    pressureState.overPressure = false;
                }
                else
                {
                    // Generate release sound based on valve type
                    releaseGain = 1.0f - releaseProgress;

                    switch (valveTypeInt)
                    {
                        case 0: // Gradual release
                            releaseGain *= 0.5f;
                            break;
                        case 1: // Explosive burst
                        {
                            releaseGain = (releaseProgress < 0.1f) ? 1.0f : (1.0f - releaseProgress) * 0.3f;
                            break;
                        }
                        case 2: // Whistle (pitched FM release)
                        {
                            float whistleFreq = pWhistlePitch * (1.0f + releaseProgress * 0.5f);
                            whistlePhase += whistleFreq * inverseSr;
                            if (whistlePhase >= 1.0f) whistlePhase -= 1.0f;
                            releaseSample = fastSin (whistlePhase * 6.28318530718f) * releaseGain;
                            break;
                        }
                    }

                    // Noise burst for explosive/gradual
                    if (valveTypeInt != 2)
                    {
                        noiseRng ^= noiseRng << 13; noiseRng ^= noiseRng >> 17; noiseRng ^= noiseRng << 5;
                        float noise = (static_cast<float> (noiseRng & 0xFFFF) / 32768.0f - 1.0f);
                        releaseSample = noise * releaseGain * 0.5f;
                    }
                }
            }

            // === SYNTHESIZE VOICES ===
            float sampleL = 0.0f;
            float sampleR = 0.0f;

            for (int v = 0; v < kMaxVoices; ++v)
            {
                auto& voice = voices[v];
                if (!voice.active) continue;

                voice.ampEnv.setADSR (pAmpA, pAmpD, pAmpS, pAmpR);
                voice.filterEnv.setADSR (pFiltA, pFiltD, pFiltS, pFiltR);

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

                // Synthesize partials with pressure-dependent modifications
                float voiceSample = 0.0f;
                int numPartials = static_cast<int> (8.0f + pVesselSize * 8.0f);

                for (int p = 0; p < numPartials; ++p)
                {
                    float harmonic = static_cast<float> (p + 1);
                    float freq = fundamental * harmonic;
                    if (freq > srF * 0.49f) break;

                    voice.partialPhase[p] += freq * inverseSr;
                    if (voice.partialPhase[p] >= 1.0f) voice.partialPhase[p] -= 1.0f;

                    float sine = fastSin (voice.partialPhase[p] * 6.28318530718f);
                    float partialAmp = (1.0f / harmonic) * velBright;

                    // Strain modification: as pressure builds, sound hardens
                    if (pressureState.strainLevel > 0.5f)
                    {
                        float strainAmount = (pressureState.strainLevel - 0.5f) * 2.0f;
                        strainAmount = clamp (strainAmount, 0.0f, 1.0f);

                        // High partials get grating — beating frequency modulation
                        if (p > 4)
                        {
                            float beatFreq = strainAmount * pStrainColor * 30.0f;
                            float beatMod = fastSin (voice.partialPhase[0] * beatFreq * 6.28318530718f);
                            partialAmp *= (1.0f + beatMod * strainAmount * 0.5f);
                        }

                        // Low partials tighten (reduce amplitude as pressure maxes)
                        if (p < 3)
                        {
                            partialAmp *= (1.0f - strainAmount * 0.3f);
                        }
                    }

                    // D005: breathing
                    partialAmp *= (1.0f + breathVal * 0.03f);
                    partialAmp *= (1.0f + lfo1Val * pLfo1Depth * 0.1f);

                    voiceSample += sine * partialAmp;
                }

                voiceSample *= (2.0f / static_cast<float> (numPartials));

                // Over-pressure saturation
                if (pressureState.overPressure)
                {
                    float saturate = 1.0f + pressureState.strainLevel * 2.0f;
                    voiceSample = fastTanh (voiceSample * saturate);
                }

                // During valve release: duck the main signal
                if (pressureState.valveOpen)
                {
                    float duck = 1.0f - clamp (pressureState.valveTimer / 0.05f, 0.0f, 1.0f);
                    if (valveTypeInt == 1) duck = 0.0f;  // explosive: full silence
                    voiceSample *= duck;
                }

                // Per-voice filter
                float voiceCutoff = pFilterCut
                    + pFiltEnvAmt * filtLevel * 4000.0f * voice.velocity
                    - pressureState.strainLevel * 2000.0f * pStrainColor;
                voiceCutoff = clamp (voiceCutoff, 50.0f, srF * 0.49f);
                voice.voiceFilter.setCoefficients_fast (voiceCutoff, pFilterRes, srF);
                voiceSample = voice.voiceFilter.processSample (voiceSample);

                voiceSample *= ampLevel;

                // Stereo
                float pan = 0.5f + (static_cast<float> (voice.currentNote - 60) * 0.02f
                            + lfo2Val * pLfo2Depth * 0.3f) * pStereoWidth;
                pan = clamp (pan, 0.0f, 1.0f);

                sampleL += voiceSample * (1.0f - pan);
                sampleR += voiceSample * pan;
            }

            // Mix in release sound
            sampleL += releaseSample * 0.7f;
            sampleR += releaseSample * 0.7f;

            sampleL *= pLevel;
            sampleR *= pLevel;

            if (std::fabs (extRingMod) > 0.001f)
            {
                sampleL *= (1.0f + extRingMod);
                sampleR *= (1.0f + extRingMod);
            }

            sampleL = softClip (sampleL);
            sampleR = softClip (sampleR);

            lastSampleL = sampleL;
            lastSampleR = sampleR;

            outL[i] += sampleL;
            if (outR) outR[i] += sampleR;
        }

        extFilterMod = 0.0f;
        extRingMod = 0.0f;

        const float* rL = buffer.getReadPointer (0);
        const float* rR = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : nullptr;
        silenceGate.analyzeBlock (rL, rR, numSamples);
    }

    //-- Coupling ---------------------------------------------------------------

    float getSampleForCoupling (int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int /*numSamples*/) override
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

    //-- BROTH Cooperative Coupling ---------------------------------------------
    void setBrothConcentrateDark (float dark) { brothConcentrateDark = clamp (dark, 0.0f, 1.0f); }
    float getPressureLevel() const { return pressureState.pressure; }
    float getStrainLevel() const { return pressureState.strainLevel; }
    bool isValveOpen() const { return pressureState.valveOpen; }

    //-- Parameters -------------------------------------------------------------

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParametersImpl (params);
        return { params.begin(), params.end() };
    }

    static void addParameters (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        addParametersImpl (params);
    }

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using PF = juce::AudioParameterFloat;

        // Core pressure parameters
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_threshold", 1 }, "Pressure Threshold",
            juce::NormalisableRange<float> (0.1f, 1.0f), 0.7f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_accumRate", 1 }, "Accumulation Rate",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_valveType", 1 }, "Valve Type",
            juce::NormalisableRange<float> (0.0f, 2.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_vesselSize", 1 }, "Vessel Size",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_strainColor", 1 }, "Strain Color",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_releaseTime", 1 }, "Release Duration",
            juce::NormalisableRange<float> (0.1f, 2.0f, 0.0f, 0.5f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_whistlePitch", 1 }, "Whistle Pitch",
            juce::NormalisableRange<float> (500.0f, 8000.0f, 0.0f, 0.3f), 2000.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_stereoWidth", 1 }, "Stereo Width",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        // Filter
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_filterCutoff", 1 }, "Filter Cutoff",
            juce::NormalisableRange<float> (50.0f, 16000.0f, 0.0f, 0.3f), 6000.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_filterRes", 1 }, "Filter Resonance",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_filtEnvAmount", 1 }, "Filter Env Amount",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.3f));

        // Amp ADSR
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_ampAttack", 1 }, "Amp Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.3f), 0.2f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_ampDecay", 1 }, "Amp Decay",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.3f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_ampSustain", 1 }, "Amp Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_ampRelease", 1 }, "Amp Release",
            juce::NormalisableRange<float> (0.01f, 15.0f, 0.0f, 0.3f), 1.5f));

        // Filter ADSR
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_filtAttack", 1 }, "Filter Attack",
            juce::NormalisableRange<float> (0.001f, 5.0f, 0.0f, 0.3f), 0.1f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_filtDecay", 1 }, "Filter Decay",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.3f), 0.3f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_filtSustain", 1 }, "Filter Sustain",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.5f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_filtRelease", 1 }, "Filter Release",
            juce::NormalisableRange<float> (0.01f, 10.0f, 0.0f, 0.3f), 0.8f));

        // LFOs
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_lfo1Rate", 1 }, "LFO 1 Rate",
            juce::NormalisableRange<float> (0.005f, 20.0f, 0.0f, 0.3f), 0.15f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_lfo1Depth", 1 }, "LFO 1 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.2f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_lfo2Rate", 1 }, "LFO 2 Rate",
            juce::NormalisableRange<float> (0.005f, 10.0f, 0.0f, 0.3f), 0.07f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_lfo2Depth", 1 }, "LFO 2 Depth",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.15f));

        // Output
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_level", 1 }, "Level",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.8f));

        // Macros
        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_macroCharacter", 1 }, "Character",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_macroMovement", 1 }, "Movement",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_macroCoupling", 1 }, "Coupling",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));

        params.push_back (std::make_unique<PF> (
            juce::ParameterID { "flow_macroSpace", 1 }, "Space",
            juce::NormalisableRange<float> (0.0f, 1.0f), 0.0f));
    }

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        pThresholdParam    = apvts.getRawParameterValue ("flow_threshold");
        pAccumRateParam    = apvts.getRawParameterValue ("flow_accumRate");
        pValveTypeParam    = apvts.getRawParameterValue ("flow_valveType");
        pVesselSizeParam   = apvts.getRawParameterValue ("flow_vesselSize");
        pStrainColorParam  = apvts.getRawParameterValue ("flow_strainColor");
        pReleaseTimeParam  = apvts.getRawParameterValue ("flow_releaseTime");
        pWhistlePitchParam = apvts.getRawParameterValue ("flow_whistlePitch");
        pStereoWidthParam  = apvts.getRawParameterValue ("flow_stereoWidth");
        pFilterCutParam    = apvts.getRawParameterValue ("flow_filterCutoff");
        pFilterResParam    = apvts.getRawParameterValue ("flow_filterRes");
        pFiltEnvAmtParam   = apvts.getRawParameterValue ("flow_filtEnvAmount");
        pAmpAParam         = apvts.getRawParameterValue ("flow_ampAttack");
        pAmpDParam         = apvts.getRawParameterValue ("flow_ampDecay");
        pAmpSParam         = apvts.getRawParameterValue ("flow_ampSustain");
        pAmpRParam         = apvts.getRawParameterValue ("flow_ampRelease");
        pFiltAParam        = apvts.getRawParameterValue ("flow_filtAttack");
        pFiltDParam        = apvts.getRawParameterValue ("flow_filtDecay");
        pFiltSParam        = apvts.getRawParameterValue ("flow_filtSustain");
        pFiltRParam        = apvts.getRawParameterValue ("flow_filtRelease");
        pLfo1RateParam     = apvts.getRawParameterValue ("flow_lfo1Rate");
        pLfo1DepthParam    = apvts.getRawParameterValue ("flow_lfo1Depth");
        pLfo2RateParam     = apvts.getRawParameterValue ("flow_lfo2Rate");
        pLfo2DepthParam    = apvts.getRawParameterValue ("flow_lfo2Depth");
        pLevelParam        = apvts.getRawParameterValue ("flow_level");
        pMacroCharacterParam = apvts.getRawParameterValue ("flow_macroCharacter");
        pMacroMovementParam  = apvts.getRawParameterValue ("flow_macroMovement");
        pMacroCouplingParam  = apvts.getRawParameterValue ("flow_macroCoupling");
        pMacroSpaceParam     = apvts.getRawParameterValue ("flow_macroSpace");
    }

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId() const override { return "Overflow"; }

    juce::Colour getAccentColour() const override
    {
        return juce::Colour (0xFFE8E8E8);  // Steam White
    }

    int getMaxVoices() const override { return kMaxVoices; }
    int getActiveVoiceCount() const override
    {
        return VoiceAllocator::countActive (voices, kMaxVoices);
    }

private:
    double sr = 44100.0;
    float srF = 44100.0f;
    int blockSize = 512;

    OverflowVoice voices[kMaxVoices];
    uint64_t noteCounter = 0;

    PressureState pressureState;

    StandardLFO lfo1, lfo2;
    BreathingLFO breathLfo;

    float whistlePhase = 0.0f;
    uint32_t noiseRng = 42u;

    float aftertouch    = 0.0f;
    float modWheel      = 0.0f;
    float pitchBendNorm = 0.0f;

    float extFilterMod = 0.0f;
    float extRingMod   = 0.0f;
    float lastSampleL  = 0.0f;
    float lastSampleR  = 0.0f;

    float brothConcentrateDark = 0.0f;

    // Parameter pointers (28 params)
    std::atomic<float>* pThresholdParam    = nullptr;
    std::atomic<float>* pAccumRateParam    = nullptr;
    std::atomic<float>* pValveTypeParam    = nullptr;
    std::atomic<float>* pVesselSizeParam   = nullptr;
    std::atomic<float>* pStrainColorParam  = nullptr;
    std::atomic<float>* pReleaseTimeParam  = nullptr;
    std::atomic<float>* pWhistlePitchParam = nullptr;
    std::atomic<float>* pStereoWidthParam  = nullptr;
    std::atomic<float>* pFilterCutParam    = nullptr;
    std::atomic<float>* pFilterResParam    = nullptr;
    std::atomic<float>* pFiltEnvAmtParam   = nullptr;
    std::atomic<float>* pAmpAParam         = nullptr;
    std::atomic<float>* pAmpDParam         = nullptr;
    std::atomic<float>* pAmpSParam         = nullptr;
    std::atomic<float>* pAmpRParam         = nullptr;
    std::atomic<float>* pFiltAParam        = nullptr;
    std::atomic<float>* pFiltDParam        = nullptr;
    std::atomic<float>* pFiltSParam        = nullptr;
    std::atomic<float>* pFiltRParam        = nullptr;
    std::atomic<float>* pLfo1RateParam     = nullptr;
    std::atomic<float>* pLfo1DepthParam    = nullptr;
    std::atomic<float>* pLfo2RateParam     = nullptr;
    std::atomic<float>* pLfo2DepthParam    = nullptr;
    std::atomic<float>* pLevelParam        = nullptr;
    std::atomic<float>* pMacroCharacterParam = nullptr;
    std::atomic<float>* pMacroMovementParam  = nullptr;
    std::atomic<float>* pMacroCouplingParam  = nullptr;
    std::atomic<float>* pMacroSpaceParam     = nullptr;
};

} // namespace xomnibus
