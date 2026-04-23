// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/SharedTransport.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/StandardADSR.h"
#include "../../DSP/StandardLFO.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/ModMatrix.h"
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <vector>

namespace xoceanus
{

//==============================================================================
// OrreryEngine — Fleet Navigation Vector Synthesis
//
// Blends 4 sound sources via a 2D vector pad with animated orbital paths.
// Sources can be built-in oscillators or coupling audio inputs from other engines.
// An orbit envelope traces paths (Circle, Ellipse, Figure-8, Custom Ephemeris)
// over the pad surface, with libration LFOs and gravity wells driven by coupling
// amplitude or spectral centroid.
//
// Gallery code: ORRERY | Accent: Orrery Brass #C8A84B | Prefix: orry_
//==============================================================================

static constexpr int kOrryMaxVoices = 12;
static constexpr int kOrryEphemerisSize = 256; // circular buffer for gesture capture

//------------------------------------------------------------------------------
// Ephemeris waypoint — {x, y} pair recorded at ~30 Hz
//------------------------------------------------------------------------------
struct EphemerisPoint
{
    float x = 0.5f;
    float y = 0.5f;
};

//------------------------------------------------------------------------------
// OrreryVoice — 4-source vector synthesis voice
//------------------------------------------------------------------------------
struct OrreryVoice
{
    bool active = false;
    bool releasing = false;
    int  note = -1;
    float velocity = 0.0f;   // normalized 0..1
    float keyTrack = 0.0f;   // (note - 60) / 60, bipolar

    // ---- per-corner oscillator phase accumulators ----
    float oscPhase[4]{};     // NW, NE, SW, SE
    float oscFreq[4]{};      // in Hz

    // ---- per-corner coupling audio accumulation buffers ----
    // These are filled by applyCouplingInput() before renderBlock() runs
    float couplingAccum[4]{};  // one sample accumulator per corner

    // ---- filter (stereo, one per voice) ----
    CytomicSVF filterL;
    CytomicSVF filterR;
    int prevFltType = -1; // sentinel: -1 forces IC reset on first block

    // ---- envelopes ----
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // ---- libration LFOs (per-voice for polyphonic independence) ----
    StandardLFO lfo1;
    StandardLFO lfo2;

    // ---- orbit state ----
    float orbitPhase = 0.0f;  // 0..1, advances at orbitSpeed Hz

    // ---- current vector position ----
    float currentX = 0.5f;
    float currentY = 0.5f;

    // ---- smoothed gravity response ----
    float smoothedGravEnergy = 0.0f;

    // ---- crossfade gain (post-VCA) ----
    float crossfadeGain = 1.0f;

    // ---- per-corner crossfade on wave-type switch (50ms) ----
    int   prevWave[4]{};
    float waveXfadeGain[4]{1.0f, 1.0f, 1.0f, 1.0f};  // 1=new-src fully in

    // ---- noise state ----
    uint32_t noiseRng[4]{12345u, 67890u, 11111u, 22222u};

    // ---- legato glide ----
    float glideFreq[4]{};    // current glide frequency per corner

    // ---- last LFO output values (for mod matrix sources) ----
    float lastLfo1Val = 0.0f;
    float lastLfo2Val = 0.0f;

    void reset() noexcept
    {
        active = false;
        releasing = false;
        note = -1;
        velocity = 0.0f;
        keyTrack = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            oscPhase[i] = 0.0f;
            oscFreq[i] = 440.0f;
            glideFreq[i] = 440.0f;
            couplingAccum[i] = 0.0f;
            prevWave[i] = 0;
            waveXfadeGain[i] = 1.0f;
            noiseRng[i] = 12345u + static_cast<uint32_t>(i * 1337);
        }
        filterL.reset();
        filterR.reset();
        prevFltType = -1;
        ampEnv.reset();
        filterEnv.reset();
        lfo1.reset();
        lfo2.reset();
        orbitPhase = 0.0f;
        currentX = 0.5f;
        currentY = 0.5f;
        smoothedGravEnergy = 0.0f;
        crossfadeGain = 1.0f;
        lastLfo1Val = 0.0f;
        lastLfo2Val = 0.0f;
    }
};

//==============================================================================
// OrreryEngine
//==============================================================================
class OrreryEngine : public SynthEngine
{
public:
    //==========================================================================
    // Lifecycle
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateFloat = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 0.0f;
        maxBlock = maxBlockSize;

        // Pre-compute 50ms xfade step
        waveXfadeStep = 1.0f / std::max(1.0f, 0.050f * sampleRateFloat);

        for (auto& v : voices)
            v.reset();

        couplingEnergyAccum = 0.0f;
        couplingEnergyCount = 0;
        lastCouplingEnergy  = 0.0f;
        spectralCentroidEst = 0.0f;

        // Mod wheel / aftertouch
        modWheelValue = 0.0f;
        aftertouchValue = 0.0f;

        // Coupling corner assignment
        nextCouplingCorner = 0;

        activeVoices.store(0, std::memory_order_relaxed);

        // SilenceGate — vector synth with moderate tail
        prepareSilenceGate(sampleRate, maxBlockSize, 300.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset();
        couplingEnergyAccum = 0.0f;
        couplingEnergyCount = 0;
        lastCouplingEnergy  = 0.0f;
        spectralCentroidEst = 0.0f;
        modWheelValue = 0.0f;
        aftertouchValue = 0.0f;
        nextCouplingCorner = 0;
        lastSampleL = lastSampleR = 0.0f;
        ephRecordTimer = 0.0f;
        activeVoices.store(0, std::memory_order_relaxed);
    }

    //==========================================================================
    // Coupling — applyCouplingInput (called BEFORE renderBlock)
    //==========================================================================

    void applyCouplingInput(CouplingType type, float amount, const float* sourceBuffer, int numSamples) override
    {
        if (sourceBuffer == nullptr || numSamples <= 0)
            return;

        switch (type)
        {
        case CouplingType::AudioToFM:
        case CouplingType::AudioToRing:
        {
            // Route coupling audio to the next available corner (round-robin)
            // This is also used to fill coupling audio accumulators for voices
            // that have wave=6 (Coupled) on that corner.
            int corner = nextCouplingCorner % 4;
            for (auto& v : voices)
            {
                if (v.active)
                {
                    // Accumulate last sample of the block into the corner buffer
                    v.couplingAccum[corner] = sourceBuffer[numSamples - 1] * amount;
                }
            }
            // Also track RMS energy for gravity wells
            float sumSq = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                sumSq += sourceBuffer[i] * sourceBuffer[i];
            couplingEnergyAccum += std::sqrt(sumSq / static_cast<float>(numSamples));
            couplingEnergyCount++;
            // Spectral centroid: zero-crossing rate of first channel
            for (int i = 1; i < numSamples; ++i)
            {
                float prev = sourceBuffer[i - 1];
                float cur  = sourceBuffer[i];
                if ((prev >= 0.0f && cur < 0.0f) || (prev < 0.0f && cur >= 0.0f))
                    spectralCentroidEst += 1.0f;
            }
            nextCouplingCorner = (nextCouplingCorner + 1) % 4;
            break;
        }

        case CouplingType::AmpToFilter:
        {
            // Amplitude of source drives filter cutoff
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingEnergyAccum += rms * amount;
            couplingEnergyCount++;
            break;
        }

        case CouplingType::EnvToMorph:
        {
            // Coupling envelope pushes vector X position
            float envLevel = sourceBuffer[numSamples - 1] * amount;
            couplingEnvToMorphX = clamp(envLevel, -1.0f, 1.0f);
            break;
        }

        case CouplingType::RhythmToBlend:
        {
            // Rhythm pushes vector Y position
            float rhythmLevel = sourceBuffer[numSamples - 1] * amount;
            couplingRhythmToY = clamp(rhythmLevel, -1.0f, 1.0f);
            break;
        }

        default:
            // General energy accumulation for gravity wells
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingEnergyAccum += rms * amount;
            couplingEnergyCount++;
            break;
        }
    }

    //==========================================================================
    // getSampleForCoupling — O(1) cached output
    //==========================================================================

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return lastSampleL;
        if (channel == 1) return lastSampleR;
        if (channel == 2) return lastOrbitPhase;  // orbit phase 0..1
        return 0.0f;
    }

    //==========================================================================
    // Parameters
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using NRange = juce::NormalisableRange<float>;
        using AP = juce::AudioParameterFloat;
        using APC = juce::AudioParameterChoice;
        using PID = juce::ParameterID;

        // ---- Group A: Sources (4 corners × 5 = 20 params) ----
        static const juce::StringArray kWaveNames {"Sine","Saw","Square","Tri","Pulse","Noise","Coupled"};
        static const char* kCorners[4] = {"nw","ne","sw","se"};

        for (int c = 0; c < 4; ++c)
        {
            juce::String pfx = juce::String("orry_") + kCorners[c] + "_";
            params.push_back(std::make_unique<APC>(PID{pfx + "wave", 1}, pfx + "wave", kWaveNames, 0));
            params.push_back(std::make_unique<AP>(PID{pfx + "char", 1}, pfx + "char",
                NRange{0.0f, 1.0f, 0.001f}, 0.0f));
            params.push_back(std::make_unique<AP>(PID{pfx + "pitch", 1}, pfx + "pitch",
                NRange{-24.0f, 24.0f, 0.01f}, 0.0f));
            params.push_back(std::make_unique<AP>(PID{pfx + "level", 1}, pfx + "level",
                NRange{0.0f, 1.0f, 0.001f}, 0.8f));
        }

        // ---- Group B: Vector Pad ----
        params.push_back(std::make_unique<AP>(PID{"orry_posX", 1}, "orry_posX",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        params.push_back(std::make_unique<AP>(PID{"orry_posY", 1}, "orry_posY",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        params.push_back(std::make_unique<AP>(PID{"orry_blendCurve", 1}, "orry_blendCurve",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        params.push_back(std::make_unique<AP>(PID{"orry_centerWeight", 1}, "orry_centerWeight",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));

        // ---- Group C: Orbit ----
        static const juce::StringArray kOrbitShapes {"Circle","Ellipse","Figure8","Custom"};
        static const juce::StringArray kOrbitLoops  {"Free","OneShot","Pendulum"};
        static const juce::StringArray kOrbitSync   {"Free","TempoSync"};

        params.push_back(std::make_unique<APC>(PID{"orry_orbitShape", 1}, "orry_orbitShape", kOrbitShapes, 0));
        {
            NRange r{0.01f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_orbitSpeed", 1}, "orry_orbitSpeed", r, 0.25f));
        }
        params.push_back(std::make_unique<AP>(PID{"orry_orbitDepth", 1}, "orry_orbitDepth",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        params.push_back(std::make_unique<AP>(PID{"orry_orbitPhase", 1}, "orry_orbitPhase",
            NRange{0.0f, 360.0f, 0.1f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"orry_orbitLoop", 1}, "orry_orbitLoop", kOrbitLoops, 0));
        params.push_back(std::make_unique<AP>(PID{"orry_orbitTilt", 1}, "orry_orbitTilt",
            NRange{-1.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"orry_orbitSync", 1}, "orry_orbitSync", kOrbitSync, 0));

        // ---- Group D: Libration LFOs ----
        static const juce::StringArray kLFOShapes {"Sine","Tri","Saw","Square","S&H"};
        static const juce::StringArray kLFOTargets {"X","Y","Both","OrbitSpeed"};

        {
            NRange r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_lfo1Rate", 1}, "orry_lfo1Rate", r, 0.3f));
        }
        params.push_back(std::make_unique<AP>(PID{"orry_lfo1Depth", 1}, "orry_lfo1Depth",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"orry_lfo1Shape", 1}, "orry_lfo1Shape", kLFOShapes, 0));
        params.push_back(std::make_unique<APC>(PID{"orry_lfo1Target", 1}, "orry_lfo1Target", kLFOTargets, 0));

        {
            NRange r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_lfo2Rate", 1}, "orry_lfo2Rate", r, 0.185f));
        }
        params.push_back(std::make_unique<AP>(PID{"orry_lfo2Depth", 1}, "orry_lfo2Depth",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"orry_lfo2Shape", 1}, "orry_lfo2Shape", kLFOShapes, 1));
        params.push_back(std::make_unique<APC>(PID{"orry_lfo2Target", 1}, "orry_lfo2Target", kLFOTargets, 1));

        // ---- Group E: Gravity Wells ----
        params.push_back(std::make_unique<AP>(PID{"orry_gravX", 1}, "orry_gravX",
            NRange{-1.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<AP>(PID{"orry_gravY", 1}, "orry_gravY",
            NRange{-1.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<AP>(PID{"orry_gravSmooth", 1}, "orry_gravSmooth",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        static const juce::StringArray kGravSrc {"Amplitude","SpectralCentroid"};
        params.push_back(std::make_unique<APC>(PID{"orry_gravSource", 1}, "orry_gravSource", kGravSrc, 0));

        // ---- Group F: Filter (5) + Filter Envelope (4) ----
        {
            NRange r{20.0f, 20000.0f, 0.1f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_fltCutoff", 1}, "orry_fltCutoff", r, 8000.0f));
        }
        params.push_back(std::make_unique<AP>(PID{"orry_fltReso", 1}, "orry_fltReso",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        static const juce::StringArray kFltTypes {"LP","HP","BP","Notch"};
        params.push_back(std::make_unique<APC>(PID{"orry_fltType", 1}, "orry_fltType", kFltTypes, 0));
        params.push_back(std::make_unique<AP>(PID{"orry_fltEnvAmt", 1}, "orry_fltEnvAmt",
            NRange{-1.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<AP>(PID{"orry_fltKeyTrack", 1}, "orry_fltKeyTrack",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_fltAtk", 1}, "orry_fltAtk", r, 0.01f));
        }
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_fltDec", 1}, "orry_fltDec", r, 0.2f));
        }
        params.push_back(std::make_unique<AP>(PID{"orry_fltSus", 1}, "orry_fltSus",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        {
            NRange r{0.0f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_fltRel", 1}, "orry_fltRel", r, 0.3f));
        }

        // ---- Group G: Amp Envelope ----
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_ampAtk", 1}, "orry_ampAtk", r, 0.01f));
        }
        {
            NRange r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_ampDec", 1}, "orry_ampDec", r, 0.3f));
        }
        params.push_back(std::make_unique<AP>(PID{"orry_ampSus", 1}, "orry_ampSus",
            NRange{0.0f, 1.0f, 0.001f}, 0.8f));
        {
            NRange r{0.0f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"orry_ampRel", 1}, "orry_ampRel", r, 0.5f));
        }

        // ---- Group H: Mod Matrix (4 slots × 3 params = 12 params) ----
        static const juce::StringArray kModDests {
            "Off", "Filter Cutoff", "Pos X", "Pos Y", "Orbit Speed", "Orbit Depth", "Amp Level"
        };
        ModMatrix<4>::addParameters(params, "orry_", "Orrery", kModDests);

        // ---- Group I: Macros + Voice + Ephemeris ----
        params.push_back(std::make_unique<AP>(PID{"orry_macro1", 1}, "orry_macro1",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));
        params.push_back(std::make_unique<AP>(PID{"orry_macro2", 1}, "orry_macro2",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<AP>(PID{"orry_macro3", 1}, "orry_macro3",
            NRange{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<AP>(PID{"orry_macro4", 1}, "orry_macro4",
            NRange{0.0f, 1.0f, 0.001f}, 0.5f));

        static const juce::StringArray kVoiceModes {"Mono","Legato","Poly8","Poly12"};
        params.push_back(std::make_unique<APC>(PID{"orry_voiceMode", 1}, "orry_voiceMode", kVoiceModes, 2));

        {
            NRange r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"orry_glide", 1}, "orry_glide", r, 0.0f));
        }

        static const juce::StringArray kEphRec {"Off","Recording","Playback"};
        params.push_back(std::make_unique<APC>(PID{"orry_ephRecord", 1}, "orry_ephRecord", kEphRec, 0));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return {params.begin(), params.end()};
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        // ---- Group A: sources ----
        static const char* kCorners[4] = {"nw","ne","sw","se"};
        for (int c = 0; c < 4; ++c)
        {
            juce::String pfx = juce::String("orry_") + kCorners[c] + "_";
            pWave[c]  = apvts.getRawParameterValue(pfx + "wave");
            pChar[c]  = apvts.getRawParameterValue(pfx + "char");
            pPitch[c] = apvts.getRawParameterValue(pfx + "pitch");
            pLevel[c] = apvts.getRawParameterValue(pfx + "level");
        }

        // ---- Group B ----
        pPosX         = apvts.getRawParameterValue("orry_posX");
        pPosY         = apvts.getRawParameterValue("orry_posY");
        pBlendCurve   = apvts.getRawParameterValue("orry_blendCurve");
        pCenterWeight = apvts.getRawParameterValue("orry_centerWeight");

        // ---- Group C ----
        pOrbitShape = apvts.getRawParameterValue("orry_orbitShape");
        pOrbitSpeed = apvts.getRawParameterValue("orry_orbitSpeed");
        pOrbitDepth = apvts.getRawParameterValue("orry_orbitDepth");
        pOrbitPhaseParam = apvts.getRawParameterValue("orry_orbitPhase");
        pOrbitLoop  = apvts.getRawParameterValue("orry_orbitLoop");
        pOrbitTilt  = apvts.getRawParameterValue("orry_orbitTilt");
        pOrbitSync  = apvts.getRawParameterValue("orry_orbitSync");

        // ---- Group D ----
        pLfo1Rate   = apvts.getRawParameterValue("orry_lfo1Rate");
        pLfo1Depth  = apvts.getRawParameterValue("orry_lfo1Depth");
        pLfo1Shape  = apvts.getRawParameterValue("orry_lfo1Shape");
        pLfo1Target = apvts.getRawParameterValue("orry_lfo1Target");
        pLfo2Rate   = apvts.getRawParameterValue("orry_lfo2Rate");
        pLfo2Depth  = apvts.getRawParameterValue("orry_lfo2Depth");
        pLfo2Shape  = apvts.getRawParameterValue("orry_lfo2Shape");
        pLfo2Target = apvts.getRawParameterValue("orry_lfo2Target");

        // ---- Group E ----
        pGravX      = apvts.getRawParameterValue("orry_gravX");
        pGravY      = apvts.getRawParameterValue("orry_gravY");
        pGravSmooth = apvts.getRawParameterValue("orry_gravSmooth");
        pGravSource = apvts.getRawParameterValue("orry_gravSource");

        // ---- Group F ----
        pFltCutoff    = apvts.getRawParameterValue("orry_fltCutoff");
        pFltReso      = apvts.getRawParameterValue("orry_fltReso");
        pFltType      = apvts.getRawParameterValue("orry_fltType");
        pFltEnvAmt    = apvts.getRawParameterValue("orry_fltEnvAmt");
        pFltKeyTrack  = apvts.getRawParameterValue("orry_fltKeyTrack");
        pFltAtk       = apvts.getRawParameterValue("orry_fltAtk");
        pFltDec       = apvts.getRawParameterValue("orry_fltDec");
        pFltSus       = apvts.getRawParameterValue("orry_fltSus");
        pFltRel       = apvts.getRawParameterValue("orry_fltRel");

        // ---- Group G ----
        pAmpAtk = apvts.getRawParameterValue("orry_ampAtk");
        pAmpDec = apvts.getRawParameterValue("orry_ampDec");
        pAmpSus = apvts.getRawParameterValue("orry_ampSus");
        pAmpRel = apvts.getRawParameterValue("orry_ampRel");

        // ---- Group H ----
        modMatrix.attachParameters(apvts, "orry_");

        // ---- Group I ----
        pMacro1    = apvts.getRawParameterValue("orry_macro1");
        pMacro2    = apvts.getRawParameterValue("orry_macro2");
        pMacro3    = apvts.getRawParameterValue("orry_macro3");
        pMacro4    = apvts.getRawParameterValue("orry_macro4");
        pVoiceMode = apvts.getRawParameterValue("orry_voiceMode");
        pGlide     = apvts.getRawParameterValue("orry_glide");
        pEphRecord = apvts.getRawParameterValue("orry_ephRecord");
    }

    //==========================================================================
    // Identity
    //==========================================================================

    juce::String  getEngineId()       const override { return "Orrery"; }
    juce::Colour  getAccentColour()   const override { return juce::Colour(0xFFC8A84B); } // Orrery Brass
    int           getMaxVoices()      const override { return kOrryMaxVoices; }
    int           getActiveVoiceCount() const override { return activeVoices.load(std::memory_order_relaxed); }

    //==========================================================================
    // Transport
    //==========================================================================

    // Called by the processor to give Orrery access to SharedTransport.
    // Must be called before the first renderBlock().
    void setSharedTransport(const SharedTransport* transport) noexcept { sharedTransport = transport; }

    //==========================================================================
    // renderBlock
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (sampleRateFloat <= 0.0f) return;
        if (numSamples <= 0) return;

        // SRO: wake on note-on, then bail if silent
        for (const auto& md : midi)
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            return;
        }

        // ---- Snapshot parameters (block-rate) ----
        const float macro1 = pMacro1 ? pMacro1->load() : 0.5f;
        const float macro2 = pMacro2 ? pMacro2->load() : 0.0f;
        const float macro3 = pMacro3 ? pMacro3->load() : 0.0f;
        const float macro4 = pMacro4 ? pMacro4->load() : 0.5f;

        // Macros modify underlying parameters
        // M1=ORBIT: scales orbit depth + speed
        const float macroOrbitMult   = macro1 * 2.0f;  // [0..2], 1.0 = unity
        // M2=LIBRATION: scales both LFO depths
        const float macroLFODepthMult = macro2 * 2.0f;
        // M3=GRAVITY: scales gravX + gravY
        const float macroGravMult     = macro3 * 2.0f;
        // M4=SPACE: scales filter cutoff
        const float macroSpaceMult    = 0.5f + macro4; // [0.5..1.5]

        const float posX = pPosX ? pPosX->load() : 0.5f;
        const float posY = pPosY ? pPosY->load() : 0.5f;
        const float blendCurve   = pBlendCurve   ? pBlendCurve->load()   : 0.5f;
        const float centerWeight = pCenterWeight ? pCenterWeight->load() : 0.0f;

        const int   orbitShape = pOrbitShape ? static_cast<int>(pOrbitShape->load()) : 0;
        float orbitSpeed       = pOrbitSpeed ? pOrbitSpeed->load() : 0.25f;
        const int   orbitSync  = pOrbitSync  ? static_cast<int>(pOrbitSync->load())  : 0;
        if (orbitSync >= 1)
        {
            // TempoSync: treat speed as beat division multiplier at host BPM.
            // Read BPM from SharedTransport if available, fall back to 120 BPM.
            const float bpm = (sharedTransport != nullptr)
                ? static_cast<float>(std::max(sharedTransport->getBPM(), 1.0))
                : 120.0f;
            orbitSpeed *= (bpm / 60.0f);
        }
        orbitSpeed = std::max(0.01f, orbitSpeed) * macroOrbitMult;
        const float orbitDepth = clamp((pOrbitDepth ? pOrbitDepth->load() : 0.5f) * macroOrbitMult, 0.0f, 0.5f);
        const float orbitPhaseOffset = pOrbitPhaseParam ? (pOrbitPhaseParam->load() / 360.0f) : 0.0f;
        const int   orbitLoop  = pOrbitLoop ? static_cast<int>(pOrbitLoop->load()) : 0;
        const float orbitTilt  = pOrbitTilt ? pOrbitTilt->load() : 0.0f;

        const float lfo1Rate   = std::max(0.01f, pLfo1Rate  ? pLfo1Rate->load()  : 0.3f);
        const float lfo1Depth  = clamp((pLfo1Depth  ? pLfo1Depth->load()  : 0.0f) * macroLFODepthMult, 0.0f, 1.0f);
        const int   lfo1Shape  = pLfo1Shape  ? static_cast<int>(pLfo1Shape->load())  : 0;
        const int   lfo1Target = pLfo1Target ? static_cast<int>(pLfo1Target->load()) : 0;

        const float lfo2Rate   = std::max(0.01f, pLfo2Rate  ? pLfo2Rate->load()  : 0.185f);
        const float lfo2Depth  = clamp((pLfo2Depth  ? pLfo2Depth->load()  : 0.0f) * macroLFODepthMult, 0.0f, 1.0f);
        const int   lfo2Shape  = pLfo2Shape  ? static_cast<int>(pLfo2Shape->load())  : 1;
        const int   lfo2Target = pLfo2Target ? static_cast<int>(pLfo2Target->load()) : 1;

        const float gravX        = (pGravX ? pGravX->load() : 0.0f) * macroGravMult;
        const float gravY        = (pGravY ? pGravY->load() : 0.0f) * macroGravMult;
        const float gravSmoothP  = pGravSmooth ? pGravSmooth->load() : 0.5f;
        const int   gravSource   = pGravSource ? static_cast<int>(pGravSource->load()) : 0;

        float baseCutoff  = (pFltCutoff ? pFltCutoff->load() : 8000.0f) * macroSpaceMult;
        baseCutoff = clamp(baseCutoff, 20.0f, 20000.0f);
        const float fltReso     = pFltReso     ? pFltReso->load()     : 0.0f;
        const int   fltType     = pFltType     ? static_cast<int>(pFltType->load()) : 0;
        const float fltEnvAmt   = pFltEnvAmt   ? pFltEnvAmt->load()   : 0.0f;
        const float fltKeyTrack = pFltKeyTrack ? pFltKeyTrack->load() : 0.5f;
        const float fltAtk = pFltAtk ? pFltAtk->load() : 0.01f;
        const float fltDec = pFltDec ? pFltDec->load() : 0.2f;
        const float fltSus = pFltSus ? pFltSus->load() : 0.0f;
        const float fltRel = pFltRel ? pFltRel->load() : 0.3f;

        const float ampAtk = pAmpAtk ? pAmpAtk->load() : 0.01f;
        const float ampDec = pAmpDec ? pAmpDec->load() : 0.3f;
        const float ampSus = pAmpSus ? pAmpSus->load() : 0.8f;
        const float ampRel = pAmpRel ? pAmpRel->load() : 0.5f;

        const int   voiceMode = pVoiceMode ? static_cast<int>(pVoiceMode->load()) : 2;
        const float glideTime = pGlide     ? pGlide->load() : 0.0f;
        const int   ephRecord = pEphRecord ? static_cast<int>(pEphRecord->load()) : 0;

        // ---- Compute gravity smooth coefficient from gravSmoothP ----
        // Map [0,1] to time constant [0.01s .. 1.0s]
        float gravTau = 0.01f + gravSmoothP * 0.99f;
        float gravSmoothCoeff = 1.0f - fastExp(-1.0f / (gravTau * sampleRateFloat));

        // ---- Accumulate coupling energy into smoothed energy ----
        float couplingEnergy = 0.0f;
        if (couplingEnergyCount > 0)
        {
            couplingEnergy = couplingEnergyAccum / static_cast<float>(couplingEnergyCount);
            couplingEnergyAccum = 0.0f;
            couplingEnergyCount = 0;
            lastCouplingEnergy = couplingEnergy;
        }
        else
        {
            couplingEnergy = lastCouplingEnergy;
        }

        // ---- Spectral centroid estimate (ZCR normalized to 0..1) ----
        // ZCR-based approximation: normalize by theoretical max (sr/2 crossings per sample)
        const float zcNormFactor = 1.0f / std::max(1.0f, sampleRateFloat * 0.5f * static_cast<float>(numSamples));
        float zcNorm = clamp(spectralCentroidEst * zcNormFactor, 0.0f, 1.0f);
        spectralCentroidEst = 0.0f; // reset for next block

        float gravEnergy = (gravSource == 1) ? zcNorm : couplingEnergy;
        // Clamp to [0..1]
        gravEnergy = clamp(gravEnergy, 0.0f, 1.0f);

        // ---- Per-corner waveform / character ----
        int   cornerWave[4]{};
        float cornerChar[4]{};
        float cornerPitch[4]{};
        float cornerLevel[4]{};
        for (int c = 0; c < 4; ++c)
        {
            cornerWave[c]  = pWave[c]  ? static_cast<int>(pWave[c]->load())  : 0;
            cornerChar[c]  = pChar[c]  ? pChar[c]->load()  : 0.0f;
            cornerPitch[c] = pPitch[c] ? pPitch[c]->load() : 0.0f;
            cornerLevel[c] = pLevel[c] ? pLevel[c]->load() : 0.8f;
        }

        // ---- Glide coefficient ----
        float glideCoeff = (glideTime > 0.0001f)
            ? fastExp(-1.0f / (glideTime * sampleRateFloat))
            : 0.0f;

        // ---- MIDI processing ----
        auto* writeL = buffer.getWritePointer(0);
        auto* writeR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : writeL;
        // ADDITIVE: do not clear — engine adds to existing buffer (slot chain convention)

        int midiSamplePos = 0;
        for (const auto& midiEvent : midi)
        {
            const auto& msg = midiEvent.getMessage();
            const int msgPos = std::min(midiEvent.samplePosition, numSamples - 1);

            // Render any samples before this MIDI event
            renderVoicesToBuffer(writeL, writeR, midiSamplePos, msgPos,
                                 posX, posY, blendCurve, centerWeight,
                                 orbitShape, orbitSpeed, orbitDepth, orbitPhaseOffset, orbitLoop, orbitTilt,
                                 lfo1Rate, lfo1Depth, lfo1Shape, lfo1Target,
                                 lfo2Rate, lfo2Depth, lfo2Shape, lfo2Target,
                                 gravX, gravY, gravSmoothCoeff, gravEnergy,
                                 baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                                 fltAtk, fltDec, fltSus, fltRel,
                                 ampAtk, ampDec, ampSus, ampRel,
                                 cornerWave, cornerChar, cornerPitch, cornerLevel,
                                 glideCoeff, ephRecord);
            midiSamplePos = msgPos;

            if (msg.isNoteOn())
                handleNoteOn(msg.getNoteNumber(), msg.getVelocity(),
                             ampAtk, ampDec, ampSus, ampRel,
                             fltAtk, fltDec, fltSus, fltRel,
                             cornerPitch, voiceMode, glideCoeff);
            else if (msg.isNoteOff())
                handleNoteOff(msg.getNoteNumber());
            else if (msg.isController() && msg.getControllerNumber() == 1)
                modWheelValue = msg.getControllerValue() / 127.0f;
            else if (msg.isChannelPressure())
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            else if (msg.isAftertouch())
                aftertouchValue = msg.getAfterTouchValue() / 127.0f;
        }

        // Render remaining samples
        renderVoicesToBuffer(writeL, writeR, midiSamplePos, numSamples,
                             posX, posY, blendCurve, centerWeight,
                             orbitShape, orbitSpeed, orbitDepth, orbitPhaseOffset, orbitLoop, orbitTilt,
                             lfo1Rate, lfo1Depth, lfo1Shape, lfo1Target,
                             lfo2Rate, lfo2Depth, lfo2Shape, lfo2Target,
                             gravX, gravY, gravSmoothCoeff, gravEnergy,
                             baseCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                             fltAtk, fltDec, fltSus, fltRel,
                             ampAtk, ampDec, ampSus, ampRel,
                             cornerWave, cornerChar, cornerPitch, cornerLevel,
                             glideCoeff, ephRecord);

        // ---- Ephemeris recording at ~30 Hz ----
        if (ephRecord == 1 && pPosX && pPosY)
        {
            ephRecordTimer += static_cast<float>(numSamples) / sampleRateFloat;
            if (ephRecordTimer >= (1.0f / 30.0f))
            {
                ephRecordTimer -= (1.0f / 30.0f);
                EphemerisPoint pt;
                pt.x = pPosX->load();
                pt.y = pPosY->load();
                ephemeris[ephWritePos % kOrryEphemerisSize] = pt;
                ephWritePos = (ephWritePos + 1) % kOrryEphemerisSize;
                if (ephSize < kOrryEphemerisSize) ++ephSize;
            }
        }

        // ---- Mod matrix (block-rate application — affects base parameters) ----
        // Sources populated from active voices (averaged)
        {
            float lfo1Sum = 0.0f, lfo2Sum = 0.0f, envSum = 0.0f;
            float velSum = 0.0f, ktSum = 0.0f;
            int count = 0;
            for (auto& v : voices)
            {
                if (v.active)
                {
                    lfo1Sum += v.lastLfo1Val;
                    lfo2Sum += v.lastLfo2Val;
                    envSum  += v.ampEnv.getLevel();
                    velSum  += v.velocity;
                    ktSum   += v.keyTrack;
                    ++count;
                }
            }
            if (count > 0)
            {
                float inv = 1.0f / static_cast<float>(count);
                blockModSrc.lfo1      = lfo1Sum * inv;
                blockModSrc.lfo2      = lfo2Sum * inv;
                blockModSrc.env       = envSum * inv;
                blockModSrc.velocity  = velSum * inv;
                blockModSrc.keyTrack  = ktSum  * inv;
            }
            blockModSrc.modWheel  = modWheelValue;
            blockModSrc.aftertouch = aftertouchValue;
        }

        // Mod matrix destinations (offsets applied per-block)
        float modDestOffsets[7] = {};
        modMatrix.apply(blockModSrc, modDestOffsets);
        // dest 1 = Filter Cutoff offset (±1 maps to ±8000 Hz)
        // dest 2 = Pos X offset
        // dest 3 = Pos Y offset
        // dest 4 = Orbit Speed offset
        // dest 5 = Orbit Depth offset
        // dest 6 = Amp Level offset
        // (These are consumed in next block's render — block-rate)
        // Store for next-block use
        modCutoffOffset    = modDestOffsets[1] * 8000.0f;
        modPosXOffset      = modDestOffsets[2];
        modPosYOffset      = modDestOffsets[3];
        modOrbitSpeedOffset= modDestOffsets[4];
        modOrbitDepthOffset= modDestOffsets[5];
        modAmpLevelOffset  = modDestOffsets[6];

        // ---- Coupling audio for output ----
        if (numSamples > 0)
        {
            lastSampleL = writeL[numSamples - 1];
            lastSampleR = writeR[numSamples - 1];
        }

        // ---- Update active voice count ----
        int av = 0;
        for (auto& v : voices)
            if (v.active) ++av;
        activeVoices.store(av, std::memory_order_relaxed);

        // SRO SilenceGate analysis
        analyzeForSilenceGate(buffer, numSamples);

        // Reset per-block coupling corner counter
        nextCouplingCorner = 0;
    }

private:

    //==========================================================================
    // renderVoicesToBuffer — core per-sample loop
    //==========================================================================

    void renderVoicesToBuffer(float* writeL, float* writeR,
                              int startSample, int endSample,
                              float posX, float posY,
                              float blendCurve, float centerWeight,
                              int orbitShape, float orbitSpeed, float orbitDepth,
                              float orbitPhaseOffset, int orbitLoop, float orbitTilt,
                              float lfo1Rate, float lfo1Depth, int lfo1Shape, int lfo1Target,
                              float lfo2Rate, float lfo2Depth, int lfo2Shape, int lfo2Target,
                              float gravX, float gravY, float gravSmoothCoeff, float gravEnergy,
                              float baseCutoff, float fltReso, int fltType,
                              float fltEnvAmt, float fltKeyTrack,
                              float fltAtk, float fltDec, float fltSus, float fltRel,
                              float ampAtk, float ampDec, float ampSus, float ampRel,
                              const int cornerWave[4], const float cornerChar[4],
                              const float cornerPitch[4], const float cornerLevel[4],
                              float glideCoeff, int /*ephRecord*/) noexcept
    {
        if (startSample >= endSample) return;

        // Apply mod matrix offsets to parameters
        const float effectivePosX  = clamp(posX + modPosXOffset, 0.0f, 1.0f);
        const float effectivePosY  = clamp(posY + modPosYOffset, 0.0f, 1.0f);
        const float effectiveOrbitSpeed = std::max(0.01f, orbitSpeed + modOrbitSpeedOffset);
        const float effectiveOrbitDepth = clamp(orbitDepth + modOrbitDepthOffset, 0.0f, 0.5f);

        const float orbitPhaseInc = effectiveOrbitSpeed / sampleRateFloat;
        const float kTwoPi = 6.28318530718f;

        // Set up LFO configs and envelope params for all voices (block-rate)
        // Hoisted per-block (was incorrectly per-sample — fix 2026-04-19): setParams fires once per
        // block, not per sample. ampAtk/Dec/Sus/Rel and fltAtk/Dec/Sus/Rel are block-constant atomic
        // loads already captured above; recomputing envelope coefficients numSamples× per block was
        // wasteful and could cause zipper noise if params changed mid-block.
        for (auto& v : voices)
        {
            if (!v.active) continue;
            v.lfo1.setRate(lfo1Rate, sampleRateFloat);
            v.lfo1.setShape(lfo1Shape);
            v.lfo2.setRate(lfo2Rate, sampleRateFloat);
            v.lfo2.setShape(lfo2Shape);
            // Hoist envelope setParams out of per-sample loop — ADSR knobs are
            // block-rate, but setParams() internally recomputes exp() coefficients
            // on every call. Was V × N × 2 std::exp calls per block.
            v.ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
            v.filterEnv.setParams(fltAtk, fltDec, fltSus, fltRel, sampleRateFloat);
        }

        // Tilt rotation coefficients are block-constant (orbitTilt loaded at block
        // start). Computing fastCos/fastSin(tiltAngle) per sample was wasted work
        // since every voice at every sample uses the same tilt rotation matrix.
        const float tiltAngle = orbitTilt * (kTwoPi * 0.5f); // ±π
        const float cosTilt = fastCos(tiltAngle);
        const float sinTilt = fastSin(tiltAngle);

        for (int s = startSample; s < endSample; ++s)
        {
            // Update filter coefficients every 16 samples (sub-block rate)

            float outL = 0.0f, outR = 0.0f;

            for (auto& v : voices)
            {
                if (!v.active) continue;

                // ---- Amp envelope (setParams hoisted to per-block voice loop) ----
                const float ampLevel = v.ampEnv.process();

                // If envelope finished, deactivate voice
                if (!v.ampEnv.isActive())
                {
                    v.active = false;
                    continue;
                }

                // ---- Filter envelope (setParams hoisted to per-block voice loop) ----
                [[maybe_unused]] const float fltEnvLevel = v.filterEnv.process();

                // ---- LFO values (per-sample) ----
                const float lfo1Val = v.lfo1.process();  // bipolar [-1, +1]
                const float lfo2Val = v.lfo2.process();
                v.lastLfo1Val = lfo1Val;
                v.lastLfo2Val = lfo2Val;

                // ---- LFO → X/Y/OrbitSpeed routing (computed before orbit advance) ----
                float lfoXMod = 0.0f, lfoYMod = 0.0f, lfoOrbitMod = 0.0f;
                if (lfo1Depth != 0.0f)
                {
                    float l1 = lfo1Val * lfo1Depth * 0.5f;
                    switch (lfo1Target)
                    {
                    case 0: lfoXMod += l1; break;
                    case 1: lfoYMod += l1; break;
                    case 2: lfoXMod += l1; lfoYMod += l1; break;
                    case 3: lfoOrbitMod += l1; break;
                    }
                }
                if (lfo2Depth != 0.0f)
                {
                    float l2 = lfo2Val * lfo2Depth * 0.5f;
                    switch (lfo2Target)
                    {
                    case 0: lfoXMod += l2; break;
                    case 1: lfoYMod += l2; break;
                    case 2: lfoXMod += l2; lfoYMod += l2; break;
                    case 3: lfoOrbitMod += l2; break;
                    }
                }

                // ---- Orbit position (per-sample) ----
                // Phase advance with loop mode (lfoOrbitMod applied per-sample)
                float sampleOrbitPhaseInc = orbitPhaseInc * (1.0f + lfoOrbitMod * 2.0f);
                sampleOrbitPhaseInc = std::max(0.0f, sampleOrbitPhaseInc);

                if (orbitLoop == 0) // Free
                {
                    v.orbitPhase += sampleOrbitPhaseInc;
                    if (v.orbitPhase >= 1.0f) v.orbitPhase -= 1.0f;
                }
                else if (orbitLoop == 1) // OneShot — stop at 1.0
                {
                    if (v.orbitPhase < 1.0f)
                        v.orbitPhase += sampleOrbitPhaseInc;
                    if (v.orbitPhase > 1.0f) v.orbitPhase = 1.0f;
                }
                else // Pendulum
                {
                    v.orbitPhase += sampleOrbitPhaseInc;
                    if (v.orbitPhase >= 2.0f) v.orbitPhase -= 2.0f;
                }

                // Map orbit phase to angle in [0, 2π]
                float orbitAngle;
                if (orbitLoop == 2) // pendulum: 0..1 forward, 1..2 backward
                {
                    float t = v.orbitPhase;
                    if (t > 1.0f) t = 2.0f - t;
                    orbitAngle = (t + orbitPhaseOffset) * kTwoPi;
                }
                else
                {
                    orbitAngle = (v.orbitPhase + orbitPhaseOffset) * kTwoPi;
                }

                // Tilt rotation: rotate (cosA, sinA) by tiltAngle.
                // cosTilt/sinTilt hoisted to block setup (tiltAngle is block-constant).
                const float cosA = fastCos(orbitAngle);
                const float sinA = fastSin(orbitAngle);

                float orbitX, orbitY;
                switch (orbitShape)
                {
                case 0: // Circle
                    orbitX = 0.5f + effectiveOrbitDepth * (cosA * cosTilt - sinA * sinTilt);
                    orbitY = 0.5f + effectiveOrbitDepth * (cosA * sinTilt + sinA * cosTilt);
                    break;

                case 1: // Ellipse
                    orbitX = 0.5f + effectiveOrbitDepth * (cosA * cosTilt - sinA * 0.5f * sinTilt);
                    orbitY = 0.5f + effectiveOrbitDepth * (cosA * sinTilt + sinA * 0.5f * cosTilt);
                    break;

                case 2: // Figure-8
                    orbitX = 0.5f + effectiveOrbitDepth * fastSin(orbitAngle);
                    orbitY = 0.5f + effectiveOrbitDepth * fastSin(2.0f * orbitAngle) * 0.5f;
                    break;

                case 3: // Custom Ephemeris — cubic interpolation from recorded buffer
                    if (ephSize >= 2)
                    {
                        float ephPos = v.orbitPhase * static_cast<float>(ephSize);
                        int   idx0 = static_cast<int>(ephPos) % ephSize;
                        int   idx1 = (idx0 + 1) % ephSize;
                        int   idx2 = (idx0 + 2) % ephSize;
                        int   idxm1 = (idx0 + ephSize - 1) % ephSize;
                        float t = ephPos - std::floor(ephPos);

                        // Cubic Catmull-Rom interpolation
                        auto cubicInterp = [&](float pm1, float p0, float p1, float p2) -> float {
                            return 0.5f * ((2.0f * p0) +
                                           (-pm1 + p1) * t +
                                           (2.0f * pm1 - 5.0f * p0 + 4.0f * p1 - p2) * t * t +
                                           (-pm1 + 3.0f * p0 - 3.0f * p1 + p2) * t * t * t);
                        };
                        orbitX = cubicInterp(ephemeris[idxm1].x, ephemeris[idx0].x, ephemeris[idx1].x, ephemeris[idx2].x);
                        orbitY = cubicInterp(ephemeris[idxm1].y, ephemeris[idx0].y, ephemeris[idx1].y, ephemeris[idx2].y);
                    }
                    else
                    {
                        orbitX = effectivePosX;
                        orbitY = effectivePosY;
                    }
                    break;

                default:
                    orbitX = 0.5f;
                    orbitY = 0.5f;
                    break;
                }

                // ---- Gravity well influence (per-sample) ----
                v.smoothedGravEnergy += (gravEnergy - v.smoothedGravEnergy) * gravSmoothCoeff;
                v.smoothedGravEnergy = flushDenormal(v.smoothedGravEnergy);

                float gravPushX = v.smoothedGravEnergy * gravX;
                float gravPushY = v.smoothedGravEnergy * gravY;
                // Clamp gravity displacement to ±0.5
                gravPushX = clamp(gravPushX, -0.5f, 0.5f);
                gravPushY = clamp(gravPushY, -0.5f, 0.5f);

                // Coupling modulation from coupling types
                const float couplingMorphX = couplingEnvToMorphX;
                const float couplingRhythmY = couplingRhythmToY;
                couplingEnvToMorphX *= 0.999f;
                couplingRhythmToY   *= 0.999f;

                // ---- Compose final X/Y position ----
                float finalX = effectivePosX + (orbitX - 0.5f) + gravPushX + lfoXMod + couplingMorphX * 0.5f;
                float finalY = effectivePosY + (orbitY - 0.5f) + gravPushY + lfoYMod + couplingRhythmY * 0.5f;

                // Center weight: lerp toward 0.5
                finalX = lerp(finalX, 0.5f, centerWeight);
                finalY = lerp(finalY, 0.5f, centerWeight);

                finalX = clamp(finalX, 0.0f, 1.0f);
                finalY = clamp(finalY, 0.0f, 1.0f);
                v.currentX = finalX;
                v.currentY = finalY;

                // ---- 2D Equal-Power Crossfade with blendCurve blend ----
                // blendCurve=0: linear, blendCurve=1: cosine (equal-power)
                float eastGainEP  = std::sqrt(finalX);
                float westGainEP  = std::sqrt(1.0f - finalX);
                float northGainEP = std::sqrt(finalY);
                float southGainEP = std::sqrt(1.0f - finalY);

                float eastGainLin  = finalX;
                float westGainLin  = 1.0f - finalX;
                float northGainLin = finalY;
                float southGainLin = 1.0f - finalY;

                float eastGain  = lerp(eastGainLin,  eastGainEP,  blendCurve);
                float westGain  = lerp(westGainLin,  westGainEP,  blendCurve);
                float northGain = lerp(northGainLin, northGainEP, blendCurve);
                float southGain = lerp(southGainLin, southGainEP, blendCurve);

                float gainNW = westGain * northGain;
                float gainNE = eastGain * northGain;
                float gainSW = westGain * southGain;
                float gainSE = eastGain * southGain;

                // ---- Per-corner oscillator synthesis ----
                float cornerSig[4]{};
                for (int c = 0; c < 4; ++c)
                {
                    const int wave   = cornerWave[c];
                    const float ch   = cornerChar[c];
                    const float ptch = cornerPitch[c];
                    const float lvl  = cornerLevel[c];

                    // Glide: smooth frequency toward target
                    float targetFreq = midiToFreqTune(v.note, ptch);
                    if (glideCoeff > 0.0f)
                        v.glideFreq[c] += (targetFreq - v.glideFreq[c]) * (1.0f - glideCoeff);
                    else
                        v.glideFreq[c] = targetFreq;

                    float freq = (glideCoeff > 0.0f) ? v.glideFreq[c] : targetFreq;

                    float phaseInc = freq / sampleRateFloat;

                    // Coupled source: use accumulated coupling audio
                    float coupledSample = v.couplingAccum[c];

                    float rawSig = 0.0f;
                    if (wave == 6) // Coupled
                    {
                        rawSig = coupledSample;
                    }
                    else
                    {
                        rawSig = generateOscSample(wave, v.oscPhase[c], phaseInc, ch,
                                                   v.noiseRng[c], sampleRateFloat);
                        v.oscPhase[c] += phaseInc;
                        if (v.oscPhase[c] >= 1.0f) v.oscPhase[c] -= 1.0f;
                    }

                    // 50ms wave-type crossfade
                    if (wave != v.prevWave[c])
                    {
                        v.waveXfadeGain[c] = 0.0f;
                        v.prevWave[c] = wave;
                    }
                    if (v.waveXfadeGain[c] < 1.0f)
                    {
                        v.waveXfadeGain[c] += waveXfadeStep;
                        if (v.waveXfadeGain[c] > 1.0f) v.waveXfadeGain[c] = 1.0f;
                    }

                    cornerSig[c] = rawSig * lvl * v.waveXfadeGain[c];
                }

                // ---- 2D blend ----
                float blended = cornerSig[0] * gainNW +
                                cornerSig[1] * gainNE +
                                cornerSig[2] * gainSW +
                                cornerSig[3] * gainSE;

                blended = flushDenormal(blended);

                // ---- Filter ----

                float filteredL = v.filterL.processSample(blended);
                float filteredR = v.filterR.processSample(blended);

                // ---- Mod wheel: timbral effect — modulates filter cutoff slightly per-sample ----
                // (handled via block-rate modMatrix cutoff offset above)

                // ---- VCA + velocity ----
                // Velocity scales both amplitude AND filter (D001 compliance)
                float velAmp = v.velocity;
                float ampOut = ampLevel * velAmp * (1.0f + modAmpLevelOffset);
                ampOut = clamp(ampOut, 0.0f, 1.5f);

                // Aftertouch adds amplitude and timbral shimmer (D006 compliance)
                ampOut *= (1.0f + aftertouchValue * 0.3f);

                filteredL *= ampOut;
                filteredR *= ampOut;

                outL += filteredL;
                outR += filteredR;
            } // end voice loop

            writeL[s] += outL;
            writeR[s] += outR;

            // Cache orbit phase from first active voice for coupling output
            for (auto& v : voices)
            {
                if (v.active)
                {
                    lastOrbitPhase = v.orbitPhase;
                    break;
                }
            }
        } // end sample loop
    }

    //==========================================================================
    // generateOscSample — all waveforms
    //==========================================================================

    static float generateOscSample(int wave, float phase, float phaseInc,
                                   float character, uint32_t& rng,
                                   float sampleRate) noexcept
    {
        (void)sampleRate;
        switch (wave)
        {
        case 0: // Sine — character folds (wavefold for warmth)
        {
            float s = fastSin(phase * 6.28318530718f);
            if (character > 0.0f)
            {
                float foldAmt = 1.0f + character * 3.0f;
                s = fastSin(s * foldAmt * 1.5707963f);
            }
            return s;
        }

        case 1: // Saw — PolyBLEP correction
        {
            float saw = 2.0f * phase - 1.0f;
            // PolyBLEP at discontinuity
            if (phaseInc > 0.0f)
            {
                float t = phase / phaseInc;
                if (t < 1.0f)
                    saw += t * t - 2.0f * t + 1.0f; // rising edge correction
                t = (phase - 1.0f) / phaseInc;
                if (t > -1.0f && t < 0.0f)
                    saw -= t * t + 2.0f * t + 1.0f;
            }
            // Character = sync ratio: re-trigger oscillator at sync phase
            if (character > 0.0f)
            {
                float syncPhase = phase * (1.0f + character * 3.0f);
                syncPhase -= std::floor(syncPhase);
                saw = 2.0f * syncPhase - 1.0f;
            }
            return saw;
        }

        case 2: // Square
        {
            float sq = (phase < 0.5f) ? 1.0f : -1.0f;
            // PolyBLEP
            if (phaseInc > 0.0f)
            {
                float t = phase / phaseInc;
                if (t < 1.0f) sq += t * t - 2.0f * t + 1.0f;
                t = (phase - 0.5f) / phaseInc;
                if (t > -1.0f && t < 0.0f) sq -= t * t + 2.0f * t + 1.0f;
            }
            return sq;
        }

        case 3: // Triangle — character folds
        {
            float tri = 4.0f * std::fabs(phase - 0.5f) - 1.0f;
            if (character > 0.0f)
            {
                float foldAmt = 1.0f + character * 2.0f;
                tri = fastSin(tri * foldAmt * 1.5707963f);
            }
            return tri;
        }

        case 4: // Pulse — character controls pulse width
        {
            float pw = 0.05f + character * 0.9f;  // [5%..95%]
            float pulse = (phase < pw) ? 1.0f : -1.0f;
            // PolyBLEP corrections at both edges
            if (phaseInc > 0.0f)
            {
                float t1 = phase / phaseInc;
                if (t1 < 1.0f) pulse += t1 * t1 - 2.0f * t1 + 1.0f;
                float t2 = (phase - pw) / phaseInc;
                if (t2 > -1.0f && t2 < 0.0f) pulse -= t2 * t2 + 2.0f * t2 + 1.0f;
            }
            return pulse;
        }

        case 5: // White noise — character = saturation
        {
            rng = rng * 1664525u + 1013904223u;
            float n = static_cast<float>(static_cast<int32_t>(rng)) * (1.0f / 2147483648.0f);
            if (character > 0.0f)
                n = softClip(n * (1.0f + character * 4.0f));
            return n;
        }

        default:
            return 0.0f;
        }
    }

    //==========================================================================
    // handleNoteOn
    //==========================================================================

    void handleNoteOn(int note, int velocity,
                      float ampAtk, float ampDec, float ampSus, float ampRel,
                      float fltAtk, float fltDec, float fltSus, float fltRel,
                      const float cornerPitch[4], int voiceMode, float /*glideCoeff*/)
    {
        float velNorm = velocity / 127.0f;
        float kt = (static_cast<float>(note) - 60.0f) / 60.0f;

        // Find a free or releasable voice
        OrreryVoice* target = nullptr;
        int maxVoices = (voiceMode == 3) ? kOrryMaxVoices : (voiceMode == 2) ? 8 : 1;

        // For mono/legato, always use voice 0
        if (voiceMode == 0 || voiceMode == 1)
        {
            target = &voices[0];
            if (voiceMode == 1 && target->active)
            {
                // Legato: retrigger from current level
                target->note     = note;
                target->velocity = velNorm;
                target->keyTrack = kt;
                for (int c = 0; c < 4; ++c)
                    target->oscFreq[c] = midiToFreqTune(note, cornerPitch[c]);
                target->ampEnv.retriggerFrom(target->ampEnv.getLevel(), ampAtk, ampDec, ampSus, ampRel);
                target->filterEnv.noteOn();
                return;
            }
        }
        else
        {
            // Find free voice
            for (int i = 0; i < maxVoices && i < kOrryMaxVoices; ++i)
            {
                if (!voices[i].active)
                {
                    target = &voices[i];
                    break;
                }
            }
            // Voice steal: take oldest releasing voice, then oldest active voice
            if (target == nullptr)
            {
                for (int i = 0; i < maxVoices && i < kOrryMaxVoices; ++i)
                {
                    if (voices[i].releasing)
                    {
                        target = &voices[i];
                        break;
                    }
                }
            }
            if (target == nullptr)
                target = &voices[0]; // ultimate fallback
        }

        // Initialize voice
        target->reset();
        target->active    = true;
        target->releasing = false;
        target->note      = note;
        target->velocity  = velNorm;
        target->keyTrack  = kt;

        for (int c = 0; c < 4; ++c)
        {
            target->oscFreq[c]   = midiToFreqTune(note, cornerPitch[c]);
            target->glideFreq[c] = target->oscFreq[c];
        }

        target->ampEnv.setParams(ampAtk, ampDec, ampSus, ampRel, sampleRateFloat);
        target->ampEnv.noteOn();

        target->filterEnv.setParams(fltAtk, fltDec, fltSus, fltRel, sampleRateFloat);
        target->filterEnv.noteOn();

        // LFOs free-running (no reset per note for smoother texture)
        target->lfo1.setRate(0.3f, sampleRateFloat);
        target->lfo2.setRate(0.185f, sampleRateFloat);

    }

    //==========================================================================
    // handleNoteOff
    //==========================================================================

    void handleNoteOff(int note)
    {
        for (auto& v : voices)
        {
            if (v.active && v.note == note && !v.releasing)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
                break;
            }
        }
    }

    //==========================================================================
    // Static helpers
    //==========================================================================
    static inline float clamp(float v, float lo, float hi) noexcept
    {
        return (v < lo) ? lo : ((v > hi) ? hi : v);
    }

    //==========================================================================
    // Members — Parameters
    //==========================================================================

    // Group A
    std::atomic<float>* pWave[4]  = {};
    std::atomic<float>* pChar[4]  = {};
    std::atomic<float>* pPitch[4] = {};
    std::atomic<float>* pLevel[4] = {};

    // Group B
    std::atomic<float>* pPosX         = nullptr;
    std::atomic<float>* pPosY         = nullptr;
    std::atomic<float>* pBlendCurve   = nullptr;
    std::atomic<float>* pCenterWeight = nullptr;

    // Group C
    std::atomic<float>* pOrbitShape      = nullptr;
    std::atomic<float>* pOrbitSpeed      = nullptr;
    std::atomic<float>* pOrbitDepth      = nullptr;
    std::atomic<float>* pOrbitPhaseParam = nullptr;
    std::atomic<float>* pOrbitLoop       = nullptr;
    std::atomic<float>* pOrbitTilt       = nullptr;
    std::atomic<float>* pOrbitSync       = nullptr;

    // Group D
    std::atomic<float>* pLfo1Rate   = nullptr;
    std::atomic<float>* pLfo1Depth  = nullptr;
    std::atomic<float>* pLfo1Shape  = nullptr;
    std::atomic<float>* pLfo1Target = nullptr;
    std::atomic<float>* pLfo2Rate   = nullptr;
    std::atomic<float>* pLfo2Depth  = nullptr;
    std::atomic<float>* pLfo2Shape  = nullptr;
    std::atomic<float>* pLfo2Target = nullptr;

    // Group E
    std::atomic<float>* pGravX      = nullptr;
    std::atomic<float>* pGravY      = nullptr;
    std::atomic<float>* pGravSmooth = nullptr;
    std::atomic<float>* pGravSource = nullptr;

    // Group F
    std::atomic<float>* pFltCutoff   = nullptr;
    std::atomic<float>* pFltReso     = nullptr;
    std::atomic<float>* pFltType     = nullptr;
    std::atomic<float>* pFltEnvAmt   = nullptr;
    std::atomic<float>* pFltKeyTrack = nullptr;
    std::atomic<float>* pFltAtk      = nullptr;
    std::atomic<float>* pFltDec      = nullptr;
    std::atomic<float>* pFltSus      = nullptr;
    std::atomic<float>* pFltRel      = nullptr;

    // Group G
    std::atomic<float>* pAmpAtk = nullptr;
    std::atomic<float>* pAmpDec = nullptr;
    std::atomic<float>* pAmpSus = nullptr;
    std::atomic<float>* pAmpRel = nullptr;

    // Group H — mod matrix
    ModMatrix<4> modMatrix;

    // Group I
    std::atomic<float>* pMacro1    = nullptr;
    std::atomic<float>* pMacro2    = nullptr;
    std::atomic<float>* pMacro3    = nullptr;
    std::atomic<float>* pMacro4    = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide     = nullptr;
    std::atomic<float>* pEphRecord = nullptr;

    //==========================================================================
    // Members — Engine State
    //==========================================================================

    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    float sampleRateFloat = 0.0f;
    int   maxBlock        = 0;

    std::array<OrreryVoice, kOrryMaxVoices> voices;
    std::atomic<int> activeVoices{0};

    // Coupling accumulation
    float couplingEnergyAccum = 0.0f;
    int   couplingEnergyCount = 0;
    float lastCouplingEnergy  = 0.0f;
    float couplingEnvToMorphX = 0.0f;
    float couplingRhythmToY   = 0.0f;
    int   nextCouplingCorner  = 0;

    // Spectral centroid (ZCR approximation)
    float spectralCentroidEst = 0.0f;

    // Cached output samples for coupling
    float lastSampleL    = 0.0f;
    float lastSampleR    = 0.0f;
    float lastOrbitPhase = 0.0f;

    // Mod wheel / aftertouch (D006 compliance)
    float modWheelValue   = 0.0f;
    float aftertouchValue = 0.0f;

    // Per-block mod matrix output offsets
    ModMatrix<4>::Sources blockModSrc{};
    float modCutoffOffset     = 0.0f;
    float modPosXOffset       = 0.0f;
    float modPosYOffset       = 0.0f;
    float modOrbitSpeedOffset = 0.0f;
    float modOrbitDepthOffset = 0.0f;
    float modAmpLevelOffset   = 0.0f;

    // Wave crossfade step (50ms)
    float waveXfadeStep = 0.01f;

    // Ephemeris
    EphemerisPoint ephemeris[kOrryEphemerisSize]{};
    int   ephWritePos = 0;
    int   ephSize     = 0;
    float ephRecordTimer = 0.0f;

    // SharedTransport — host BPM for TempoSync orbit speed
    const SharedTransport* sharedTransport = nullptr;
};

} // namespace xoceanus
