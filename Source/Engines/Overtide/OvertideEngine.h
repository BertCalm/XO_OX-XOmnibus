// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
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
//
//  O V E R T I D E   E N G I N E
//  Wavelet Multi-Resolution Synthesis
//
//  XO_OX Aquatic Identity: Overtides — harmonics of tidal frequencies
//  (M2 -> M4 -> M6). Multi-scale ocean phenomena. Each wavelet is a windowed
//  oscillation, localized in both time and frequency, at octave-related
//  scales from the fundamental. Low scales breathe slowly like ocean swells;
//  high scales shimmer rapidly like surface spray.
//
//  Gallery code: OVERTIDE | Accent: Tidal Cobalt #2A6BB5 | Prefix: ovt_
//
//  Inspired by (D003):
//    Wavelet-informed multi-scale synthesis — Gaussian-windowed oscillators
//    at octave-spaced frequencies, inspired by wavelet time-frequency
//    localization (Morlet 1982, Daubechies 1992, Gabor 1946).
//
//  Signal Flow:
//    8x WaveletGenerator (scales 1-8, octave spacing)
//        |
//    Sum + CytomicSVF Output Filter (stereo)
//        |
//    VCA (amp envelope x velocity)
//        |
//    Output (stereo, scale-spread panning)
//
//  Coupling:
//    Input:  AudioToFM      -> modulates wavelet carrier phases
//            AmpToFilter    -> filter cutoff
//            EnvToMorph     -> scaleBalance
//            RhythmToBlend  -> triggers wavelet retrigger burst
//    Output: ch0=L, ch1=R, ch2=current scaleBalance [0..1]
//
//==============================================================================

static constexpr int   kOvertideMaxVoices  = 8;
static constexpr int   kOvertideNumScales  = 8;
static constexpr float kOvertideTwoPi      = 6.28318530717958647692f;

//==============================================================================
// WaveletGenerator -- one wavelet oscillator at a specific octave scale.
//
// Produces a windowed oscillation (localized in time AND frequency).
// When the Gaussian envelope decays below threshold, envPhase resets
// automatically (continuous sound via periodic retrigger).
//==============================================================================
struct OvtWaveletGenerator
{
    float phase        = 0.0f;   // carrier phase [0, 1)
    float envPhase     = 0.0f;   // Gaussian envelope position (sweeps from 0 outward)
    float amplitude    = 1.0f;   // target per-scale amplitude
    float ampSmoothed  = 1.0f;   // one-pole smoothed amplitude (zipper prevention)
    float smoothCoeff  = 0.005f; // set from outer loop with sample-rate-scaled value
    float retrigOffset = 0.0f;   // Random mode: stochastic offset on retrig threshold
    uint32_t rng       = 12345u; // per-generator PRNG for phase randomization

    void reset() noexcept
    {
        phase        = 0.0f;
        envPhase     = 0.0f;
        amplitude    = 1.0f;
        ampSmoothed  = 1.0f;
        retrigOffset = 0.0f;
    }

    // Advance one sample and return wavelet output.
    //
    //   centerFreq   -- wavelet center frequency in Hz
    //   sigma        -- Gaussian window width (bandwidth param, [0.1, 2.0])
    //   envRate      -- envelope phase advance rate per sample
    //   waveletType  -- 0=Morlet, 1=Gabor, 2=MexicanHat, 3=Haar
    //   retrigMode   -- 0=Free, 1=Sync, 2=Random
    //   sampleRate   -- current sample rate
    //   fmMod        -- FM modulation Hz from AudioToFM coupling
    //   forceRetrig  -- Sync mode retrigger forced externally
    //   phaseRandom  -- [0,1] randomness on retrigger phase
    //
    inline float tick(float centerFreq, float sigma, float envRate,
                      int waveletType, int retrigMode,
                      float sampleRate, float fmMod,
                      bool forceRetrig, float phaseRandom) noexcept
    {
        // Smooth amplitude target (one-pole, per-sample -- zipper prevention)
        ampSmoothed += (amplitude - ampSmoothed) * smoothCoeff;
        ampSmoothed  = flushDenormal(ampSmoothed);
        if (ampSmoothed < 1e-6f) return 0.0f;

        // Effective carrier frequency with FM
        const float effFreq = std::max(10.0f, centerFreq + fmMod);

        // Advance carrier phase
        phase += effFreq / sampleRate;
        if (phase >= 1.0f) phase -= 1.0f;

        // Advance Gaussian envelope phase
        envPhase += envRate;
        envPhase  = flushDenormal(envPhase);

        // Gaussian window: exp(-0.5 * t^2 / sigma^2)
        const float sig2 = std::max(0.0025f, sigma * sigma);
        const float envVal = fastExp(-0.5f * envPhase * envPhase / sig2);

        // Retrigger when envelope decays below threshold
        const float retrigThresh = 0.001f + retrigOffset;
        bool doRetrig = forceRetrig || (flushDenormal(envVal) < retrigThresh);

        if (doRetrig)
        {
            envPhase = 0.0f;
            if (phaseRandom > 0.0f)
            {
                rng ^= rng << 13u;
                rng ^= rng >> 17u;
                rng ^= rng << 5u;
                const float rand01 = static_cast<float>(rng & 0xFFFF) / 65536.0f;
                phase += rand01 * phaseRandom;
                if (phase >= 1.0f) phase -= 1.0f;
            }
            if (retrigMode == 2) // Random: stagger retrig threshold
            {
                rng ^= rng << 13u;
                rng ^= rng >> 17u;
                rng ^= rng << 5u;
                retrigOffset = static_cast<float>(rng & 0x3FF) / 4096.0f * 0.03f;
            }
        }

        // Carrier waveform
        float carrier = 0.0f;
        const float phaseRad = phase * kOvertideTwoPi;

        switch (waveletType)
        {
        case 0: // Morlet: sin(2pift) x Gaussian
        default:
            carrier = fastSin(phaseRad);
            break;

        case 1: // Gabor: half-rectified cosine — punchier, more percussive atom
            carrier = std::max(0.0f, fastCos(phaseRad));
            break;

        case 2: // Mexican Hat (Ricker): purely the second derivative of Gaussian
        {
            const float t2s2 = envPhase * envPhase / sig2;
            carrier = (1.0f - t2s2) * 2.0f;  // scale factor for audibility
            break;
        }

        case 3: // Haar: localized bipolar step — positive then negative with silence
            if (phase < 0.25f)       carrier =  1.0f;
            else if (phase < 0.5f)   carrier = -1.0f;
            else                     carrier =  0.0f;  // silence in second half = localization
            break;
        }

        return carrier * flushDenormal(envVal) * ampSmoothed;
    }
};

//==============================================================================
// OvertideVoice -- one polyphonic voice (8 wavelet generators)
//==============================================================================
struct OvertideVoice
{
    bool  active    = false;
    bool  releasing = false;
    int   note      = -1;
    float velocity  = 0.0f;
    float keyTrack  = 0.0f;

    // Glide-smoothed fundamental frequency
    float glideFreq = 440.0f;

    // 8 wavelet generators (scales 1-8, octave spacing)
    OvtWaveletGenerator gens[kOvertideNumScales];

    // Output filter (stereo independent instances)
    CytomicSVF filterL;
    CytomicSVF filterR;

    // Envelopes
    StandardADSR ampEnv;
    StandardADSR filterEnv;

    // Per-voice LFOs
    StandardLFO lfo1;
    StandardLFO lfo2;

    // Last LFO outputs for mod matrix source population
    float lastLfo1Val = 0.0f;
    float lastLfo2Val = 0.0f;

    // Coupling FM per-sample accumulator
    float couplingFmAccum = 0.0f;

    // Filter coefficient update counter (update every 16 samples)
    int filterUpdateCounter = 0;

    // RhythmToBlend Sync retrigger request flag
    std::atomic<bool> syncRetrigRequest{false};

    void reset(float /*sr*/) noexcept
    {
        active    = false;
        releasing = false;
        note      = -1;
        velocity  = 0.0f;
        keyTrack  = 0.0f;
        glideFreq = 440.0f;

        for (int i = 0; i < kOvertideNumScales; ++i)
        {
            gens[i].reset();
            // Stagger initial env phases so all scales don't start in unison
            gens[i].envPhase = static_cast<float>(i) * 0.5f;
            gens[i].rng = 12345u
                + static_cast<uint32_t>(i) * 2654435761u;
        }

        filterL.reset();
        filterR.reset();
        ampEnv.reset();
        filterEnv.reset();
        lfo1.reset();
        lfo2.reset();
        lastLfo1Val         = 0.0f;
        lastLfo2Val         = 0.0f;
        couplingFmAccum     = 0.0f;
        filterUpdateCounter = 0;
        syncRetrigRequest.store(false, std::memory_order_relaxed);
    }
};

//==============================================================================
//
//  OvertideEngine -- Wavelet Multi-Resolution Synthesis
//
//  Implements SynthEngine for XOceanus integration.
//  All DSP is inline per project convention -- .cpp is a one-line stub.
//
//  Parameter prefix: ovt_
//  Gallery accent:   Tidal Cobalt #2A6BB5
//
//==============================================================================
class OvertideEngine : public SynthEngine
{
public:

    //==========================================================================
    //  P A R A M E T E R   R E G I S T R A T I O N
    //==========================================================================

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using AP  = juce::AudioParameterFloat;
        using APC = juce::AudioParameterChoice;
        using PID = juce::ParameterID;
        using NR  = juce::NormalisableRange<float>;

        // ---- A: Wavelet Core (7 params) ----
        params.push_back(std::make_unique<APC>(PID{"ovt_waveletType",1}, "Overtide Wavelet Type",
            juce::StringArray{"Morlet","Gabor","MexicanHat","Haar"}, 0));

        // scaleCount: index 0..4 -> 4..8 active scales
        params.push_back(std::make_unique<APC>(PID{"ovt_scaleCount",1}, "Overtide Scale Count",
            juce::StringArray{"4","5","6","7","8"}, 4));

        params.push_back(std::make_unique<AP>(PID{"ovt_bandwidth",1}, "Overtide Bandwidth",
            NR{0.1f, 2.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ovt_scaleBalance",1}, "Overtide Frequency Tilt",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<AP>(PID{"ovt_scaleSpread",1}, "Overtide Stereo Width",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<APC>(PID{"ovt_retrigMode",1}, "Overtide Retrig Mode",
            juce::StringArray{"Free","Sync","Random"}, 0));

        params.push_back(std::make_unique<AP>(PID{"ovt_brightness",1}, "Overtide Brightness",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        // ---- B: Wavelet Shape (4 params) ----
        params.push_back(std::make_unique<AP>(PID{"ovt_envDecay",1}, "Overtide Shimmer Rate",
            NR{0.01f, 1.0f, 0.001f}, 0.3f));

        params.push_back(std::make_unique<AP>(PID{"ovt_phaseRandom",1}, "Overtide Phase Random",
            NR{0.0f, 1.0f, 0.001f}, 0.2f));

        params.push_back(std::make_unique<AP>(PID{"ovt_pitchTrack",1}, "Overtide Pitch Track",
            NR{0.0f, 1.0f, 0.001f}, 1.0f));

        {
            NR r{20.0f, 800.0f, 0.1f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_baseFreq",1}, "Overtide Base Freq", r, 110.0f));
        }

        // ---- C: Filter + Filter Envelope (9 params) ----
        {
            NR r{20.0f, 20000.0f, 0.1f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_fltCutoff",1}, "Overtide Flt Cutoff", r, 10000.0f));
        }
        params.push_back(std::make_unique<AP>(PID{"ovt_fltReso",1}, "Overtide Flt Reso",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));

        params.push_back(std::make_unique<APC>(PID{"ovt_fltType",1}, "Overtide Flt Type",
            juce::StringArray{"LP","HP","BP","Notch"}, 0));

        params.push_back(std::make_unique<AP>(PID{"ovt_fltEnvAmt",1}, "Overtide Flt Env Amt",
            NR{-1.0f, 1.0f, 0.001f}, 0.2f));

        params.push_back(std::make_unique<AP>(PID{"ovt_fltKeyTrack",1}, "Overtide Flt Key Track",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_fltAtk",1}, "Overtide Flt Atk", r, 0.01f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_fltDec",1}, "Overtide Flt Dec", r, 0.3f));
        }
        params.push_back(std::make_unique<AP>(PID{"ovt_fltSus",1}, "Overtide Flt Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_fltRel",1}, "Overtide Flt Rel", r, 0.4f));
        }

        // ---- D: Amp Envelope (5 params) ----
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_ampAtk",1}, "Overtide Amp Atk", r, 0.01f));
        }
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_ampDec",1}, "Overtide Amp Dec", r, 0.4f));
        }
        params.push_back(std::make_unique<AP>(PID{"ovt_ampSus",1}, "Overtide Amp Sus",
            NR{0.0f, 1.0f, 0.001f}, 0.7f));
        {
            NR r{0.0f, 10.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_ampRel",1}, "Overtide Amp Rel", r, 0.5f));
        }
        // D001: velocity -> upper scales get more amplitude at high velocity
        params.push_back(std::make_unique<AP>(PID{"ovt_velTimbre",1}, "Overtide Vel Timbre",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        // ---- E: LFOs (8 params) ----
        static const juce::StringArray kLFOShapes   {"Sine","Tri","Saw","Square","S&H"};
        static const juce::StringArray kLFO1Targets {"Bandwidth","ScaleBalance","FilterCutoff","EnvDecay"};
        static const juce::StringArray kLFO2Targets {"Bandwidth","ScaleBalance","FilterCutoff","EnvDecay"};

        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_lfo1Rate",1}, "Overtide LFO1 Rate", r, 0.4f));
        }
        params.push_back(std::make_unique<AP>(PID{"ovt_lfo1Depth",1}, "Overtide LFO1 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"ovt_lfo1Shape",1}, "Overtide LFO1 Shape",
            kLFOShapes, 0));
        params.push_back(std::make_unique<APC>(PID{"ovt_lfo1Target",1}, "Overtide LFO1 Target",
            kLFO1Targets, 0));

        {
            NR r{0.01f, 20.0f, 0.001f};
            r.setSkewForCentre(0.3f);
            params.push_back(std::make_unique<AP>(PID{"ovt_lfo2Rate",1}, "Overtide LFO2 Rate", r, 0.12f));
        }
        params.push_back(std::make_unique<AP>(PID{"ovt_lfo2Depth",1}, "Overtide LFO2 Depth",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        params.push_back(std::make_unique<APC>(PID{"ovt_lfo2Shape",1}, "Overtide LFO2 Shape",
            kLFOShapes, 1));
        params.push_back(std::make_unique<APC>(PID{"ovt_lfo2Target",1}, "Overtide LFO2 Target",
            kLFO2Targets, 1));

        // ---- F: Mod Matrix (4 slots x 3 params = 12 params) ----
        static const juce::StringArray kOvertideModDests {
            "Off", "Filter Cutoff", "Bandwidth", "Scale Balance",
            "Env Decay", "Brightness", "Amp Level"
        };
        ModMatrix<4>::addParameters(params, "ovt_", "Overtide", kOvertideModDests);

        // ---- G: Macros + Voice (7 params) ----
        // M1=SWELL: bandwidth + scale balance -- ocean wave character
        params.push_back(std::make_unique<AP>(PID{"ovt_macro1",1}, "Overtide Macro1",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));
        // M2=CHOP: envDecay + brightness -- surface texture
        params.push_back(std::make_unique<AP>(PID{"ovt_macro2",1}, "Overtide Macro2",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M3=TIDE: stereo width via scale spread
        params.push_back(std::make_unique<AP>(PID{"ovt_macro3",1}, "Overtide Macro3",
            NR{0.0f, 1.0f, 0.001f}, 0.0f));
        // M4=SPACE: filter cutoff + width
        params.push_back(std::make_unique<AP>(PID{"ovt_macro4",1}, "Overtide Macro4",
            NR{0.0f, 1.0f, 0.001f}, 0.5f));

        params.push_back(std::make_unique<APC>(PID{"ovt_voiceMode",1}, "Overtide Voice Mode",
            juce::StringArray{"Mono","Legato","Poly4","Poly8"}, 2));

        {
            NR r{0.0f, 2.0f, 0.001f};
            r.setSkewForCentre(0.5f);
            params.push_back(std::make_unique<AP>(PID{"ovt_glide",1}, "Overtide Glide", r, 0.0f));
        }

        params.push_back(std::make_unique<APC>(PID{"ovt_glideMode",1}, "Overtide Glide Mode",
            juce::StringArray{"Legato","Always"}, 0));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        addParameters(params);
        return {params.begin(), params.end()};
    }

    //==========================================================================
    //  L I F E C Y C L E
    //==========================================================================

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sampleRateFloat = (sampleRate > 0.0) ? static_cast<float>(sampleRate) : 44100.0f;
        maxBlock = maxBlockSize;

        for (int i = 0; i < kOvertideMaxVoices; ++i)
        {
            voices[i].reset(sampleRateFloat);
            // Per-voice PRNG seed diversification
            for (int s = 0; s < kOvertideNumScales; ++s)
                voices[i].gens[s].rng = 12345u
                    + static_cast<uint32_t>(i) * 31337u
                    + static_cast<uint32_t>(s) * 2654435761u;
        }

        couplingFmBuf.assign(static_cast<size_t>(std::max(maxBlockSize, 1)), 0.0f);
        couplingAmpFilter = 0.0f;
        couplingEnvMorph  = 0.0f;
        couplingRhythm    = 0.0f;
        modWheelValue     = 0.0f;
        aftertouchValue   = 0.0f;
        lastSampleL       = 0.0f;
        lastSampleR       = 0.0f;
        lastScaleBalance  = 0.5f;

        activeVoiceCount_.store(0, std::memory_order_relaxed);

        // Wavelet tails can sustain; use 300ms hold
        prepareSilenceGate(sampleRate, maxBlockSize, 300.0f);
    }

    void releaseResources() override {}

    void reset() override
    {
        for (auto& v : voices)
            v.reset(sampleRateFloat);

        std::fill(couplingFmBuf.begin(), couplingFmBuf.end(), 0.0f);
        couplingAmpFilter = 0.0f;
        couplingEnvMorph  = 0.0f;
        couplingRhythm    = 0.0f;
        modWheelValue     = 0.0f;
        aftertouchValue   = 0.0f;
        pitchBendValue    = 0.0f;
        lastSampleL       = 0.0f;
        lastSampleR       = 0.0f;
        lastScaleBalance  = 0.5f;
        activeVoiceCount_.store(0, std::memory_order_relaxed);
    }

    //==========================================================================
    //  C O U P L I N G
    //==========================================================================

    void applyCouplingInput(CouplingType type, float amount,
                            const float* sourceBuffer, int numSamples) override
    {
        if (sourceBuffer == nullptr || numSamples <= 0)
            return;

        switch (type)
        {
        case CouplingType::AudioToFM:
        {
            const int copyLen = std::min(numSamples, maxBlock);
            for (int i = 0; i < copyLen; ++i)
                couplingFmBuf[static_cast<size_t>(i)] = sourceBuffer[i] * amount;
            for (auto& v : voices)
                if (v.active)
                    v.couplingFmAccum = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::AmpToFilter:
        {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter = rms * amount;
            break;
        }
        case CouplingType::EnvToMorph:
        {
            // Envelope modulates scaleBalance (low->high emphasis)
            couplingEnvMorph = sourceBuffer[numSamples - 1] * amount;
            break;
        }
        case CouplingType::RhythmToBlend:
        {
            couplingRhythm = sourceBuffer[numSamples - 1] * amount;
            // Strong hit triggers wavelet retrigger burst (Sync mode)
            if (std::fabs(couplingRhythm) > 0.5f)
                for (auto& v : voices)
                    if (v.active) v.syncRetrigRequest.store(true, std::memory_order_release);
            break;
        }
        default:
        {
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += std::fabs(sourceBuffer[i]);
            rms /= static_cast<float>(numSamples);
            couplingAmpFilter += rms * amount;
            break;
        }
        }
    }

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        if (channel == 0) return lastSampleL;
        if (channel == 1) return lastSampleR;
        if (channel == 2) return lastScaleBalance;
        return 0.0f;
    }

    //==========================================================================
    //  P A R A M E T E R   A T T A C H M E N T
    //==========================================================================

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        pWaveletType  = apvts.getRawParameterValue("ovt_waveletType");
        pScaleCount   = apvts.getRawParameterValue("ovt_scaleCount");
        pBandwidth    = apvts.getRawParameterValue("ovt_bandwidth");
        pScaleBalance = apvts.getRawParameterValue("ovt_scaleBalance");
        pScaleSpread  = apvts.getRawParameterValue("ovt_scaleSpread");
        pRetrigMode   = apvts.getRawParameterValue("ovt_retrigMode");
        pBrightness   = apvts.getRawParameterValue("ovt_brightness");

        pEnvDecay    = apvts.getRawParameterValue("ovt_envDecay");
        pPhaseRandom = apvts.getRawParameterValue("ovt_phaseRandom");
        pPitchTrack  = apvts.getRawParameterValue("ovt_pitchTrack");
        pBaseFreq    = apvts.getRawParameterValue("ovt_baseFreq");

        pFltCutoff   = apvts.getRawParameterValue("ovt_fltCutoff");
        pFltReso     = apvts.getRawParameterValue("ovt_fltReso");
        pFltType     = apvts.getRawParameterValue("ovt_fltType");
        pFltEnvAmt   = apvts.getRawParameterValue("ovt_fltEnvAmt");
        pFltKeyTrack = apvts.getRawParameterValue("ovt_fltKeyTrack");
        pFltAtk      = apvts.getRawParameterValue("ovt_fltAtk");
        pFltDec      = apvts.getRawParameterValue("ovt_fltDec");
        pFltSus      = apvts.getRawParameterValue("ovt_fltSus");
        pFltRel      = apvts.getRawParameterValue("ovt_fltRel");

        pAmpAtk    = apvts.getRawParameterValue("ovt_ampAtk");
        pAmpDec    = apvts.getRawParameterValue("ovt_ampDec");
        pAmpSus    = apvts.getRawParameterValue("ovt_ampSus");
        pAmpRel    = apvts.getRawParameterValue("ovt_ampRel");
        pVelTimbre = apvts.getRawParameterValue("ovt_velTimbre");

        pLfo1Rate   = apvts.getRawParameterValue("ovt_lfo1Rate");
        pLfo1Depth  = apvts.getRawParameterValue("ovt_lfo1Depth");
        pLfo1Shape  = apvts.getRawParameterValue("ovt_lfo1Shape");
        pLfo1Target = apvts.getRawParameterValue("ovt_lfo1Target");
        pLfo2Rate   = apvts.getRawParameterValue("ovt_lfo2Rate");
        pLfo2Depth  = apvts.getRawParameterValue("ovt_lfo2Depth");
        pLfo2Shape  = apvts.getRawParameterValue("ovt_lfo2Shape");
        pLfo2Target = apvts.getRawParameterValue("ovt_lfo2Target");

        modMatrix.attachParameters(apvts, "ovt_");

        pMacro1    = apvts.getRawParameterValue("ovt_macro1");
        pMacro2    = apvts.getRawParameterValue("ovt_macro2");
        pMacro3    = apvts.getRawParameterValue("ovt_macro3");
        pMacro4    = apvts.getRawParameterValue("ovt_macro4");
        pVoiceMode = apvts.getRawParameterValue("ovt_voiceMode");
        pGlide     = apvts.getRawParameterValue("ovt_glide");
        pGlideMode = apvts.getRawParameterValue("ovt_glideMode");
    }

    //==========================================================================
    //  I D E N T I T Y
    //==========================================================================

    juce::String  getEngineId()     const override { return "Overtide"; }
    juce::Colour  getAccentColour() const override { return juce::Colour(0xFF2A6BB5); }
    int           getMaxVoices()    const override { return kOvertideMaxVoices; }

    //==========================================================================
    //  R E N D E R   B L O C K
    //==========================================================================

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi, int numSamples) override
    {
        juce::ScopedNoDenormals noDenormals;
        if (sampleRateFloat <= 0.0f) return;
        if (numSamples <= 0) return;

        // SilenceGate: wake on note-on, bail if truly silent
        for (const auto& md : midi)
        {
            if (md.getMessage().isNoteOn())
            {
                wakeSilenceGate();
                break;
            }
        }
        if (isSilenceGateBypassed() && midi.isEmpty())
        {
            buffer.clear();
            return;
        }

        // ---- Snapshot all parameters once per block ----
        const float macro1 = pMacro1 ? pMacro1->load() : 0.5f;
        const float macro2 = pMacro2 ? pMacro2->load() : 0.0f;
        const float macro3 = pMacro3 ? pMacro3->load() : 0.0f;
        const float macro4 = pMacro4 ? pMacro4->load() : 0.5f;

        // Group A -- Wavelet Core
        const int waveletType   = pWaveletType  ? static_cast<int>(pWaveletType->load())  : 0;
        const int scaleCountIdx = pScaleCount   ? static_cast<int>(pScaleCount->load())   : 4;
        const int scaleCount    = std::clamp(scaleCountIdx + 4, 4, kOvertideNumScales);
        float bandwidth         = pBandwidth    ? pBandwidth->load()    : 0.5f;
        float scaleBalance      = pScaleBalance ? pScaleBalance->load() : 0.5f;
        float scaleSpread       = pScaleSpread  ? pScaleSpread->load()  : 0.0f;
        const int retrigMode    = pRetrigMode   ? static_cast<int>(pRetrigMode->load())   : 0;
        float brightness        = pBrightness   ? pBrightness->load()   : 0.5f;

        // M1=SWELL: bipolar — above 0.5 widens bandwidth + shifts to highs (swelling)
        bandwidth    = std::clamp(bandwidth    + (macro1 - 0.5f) * 0.6f, 0.1f, 2.0f);
        scaleBalance = std::clamp(scaleBalance + (macro1 - 0.5f) * 0.6f, 0.0f, 1.0f);

        // Group B -- Wavelet Shape
        float envDecay = pEnvDecay    ? pEnvDecay->load()    : 0.3f;
        // M2=CHOP: envDecay shorter + brightness lifts
        envDecay   = std::clamp(envDecay   - macro2 * 0.15f, 0.01f, 1.0f);
        brightness = std::clamp(brightness + macro2 * 0.4f,  0.0f,  1.0f);

        const float phaseRandom = pPhaseRandom ? pPhaseRandom->load() : 0.2f;
        const float pitchTrack  = pPitchTrack  ? pPitchTrack->load()  : 1.0f;
        const float baseFreq    = pBaseFreq    ? pBaseFreq->load()    : 110.0f;

        // M3=TIDE: stereo width widens
        scaleSpread = std::clamp(scaleSpread + macro3 * 0.8f, 0.0f, 1.0f);

        // Apply EnvToMorph coupling: shifts scaleBalance
        scaleBalance = std::clamp(scaleBalance + couplingEnvMorph, 0.0f, 1.0f);

        // Group C -- Filter
        float baseCutoff = pFltCutoff ? pFltCutoff->load() : 10000.0f;
        baseCutoff += couplingAmpFilter * 8000.0f;
        // M4=SPACE: macro opens filter
        baseCutoff = std::clamp(baseCutoff * (0.5f + macro4), 20.0f, 20000.0f);

        const float fltReso     = pFltReso     ? pFltReso->load()     : 0.0f;
        const int   fltType     = pFltType     ? static_cast<int>(pFltType->load()) : 0;
        const float fltEnvAmt   = pFltEnvAmt   ? pFltEnvAmt->load()   : 0.2f;
        const float fltKeyTrack = pFltKeyTrack ? pFltKeyTrack->load() : 0.5f;
        const float fltAtk      = pFltAtk      ? pFltAtk->load()      : 0.01f;
        const float fltDec      = pFltDec      ? pFltDec->load()      : 0.3f;
        const float fltSus      = pFltSus      ? pFltSus->load()      : 0.0f;
        const float fltRel      = pFltRel      ? pFltRel->load()      : 0.4f;

        // Group D -- Amp Envelope
        const float ampAtk    = pAmpAtk    ? pAmpAtk->load()    : 0.01f;
        const float ampDec    = pAmpDec    ? pAmpDec->load()    : 0.4f;
        const float ampSus    = pAmpSus    ? pAmpSus->load()    : 0.7f;
        const float ampRel    = pAmpRel    ? pAmpRel->load()    : 0.5f;
        const float velTimbre = pVelTimbre ? pVelTimbre->load() : 0.5f;

        // Group E -- LFOs (enforce 0.01 Hz floor, D005)
        const float lfo1Rate  = std::max(0.01f, pLfo1Rate  ? pLfo1Rate->load()  : 0.4f);
        const float lfo1Depth = pLfo1Depth ? pLfo1Depth->load() : 0.0f;
        const int   lfo1Shape = pLfo1Shape ? static_cast<int>(pLfo1Shape->load())  : 0;
        const int   lfo1Tgt   = pLfo1Target? static_cast<int>(pLfo1Target->load()) : 0;

        const float lfo2Rate  = std::max(0.01f, pLfo2Rate  ? pLfo2Rate->load()  : 0.12f);
        const float lfo2Depth = pLfo2Depth ? pLfo2Depth->load() : 0.0f;
        const int   lfo2Shape = pLfo2Shape ? static_cast<int>(pLfo2Shape->load())  : 1;
        const int   lfo2Tgt   = pLfo2Target? static_cast<int>(pLfo2Target->load()) : 1;

        // Voice / Glide
        const int   voiceMode = pVoiceMode ? static_cast<int>(pVoiceMode->load()) : 2;
        const float glideTime = pGlide     ? pGlide->load() : 0.0f;
        const int   glideMode = pGlideMode ? static_cast<int>(pGlideMode->load()) : 0;

        const bool  glideActive = (glideTime > 0.0001f) && (glideMode == 1 || voiceMode == 1);
        const float glideCoeff  = glideActive
            ? fastExp(-1.0f / (glideTime * sampleRateFloat))
            : 0.0f;

        // D006: CC1 modulates bandwidth (narrow=tonal, wide=transient)
        const float effectiveBandwidth   = std::clamp(bandwidth - modWheelValue * 0.3f, 0.1f, 2.0f);
        // D006: Aftertouch modulates scale balance (shifts emphasis low<->high)
        const float effectiveScaleBalance = std::clamp(scaleBalance + aftertouchValue * 0.4f, 0.0f, 1.0f);

        // ---- Build mod matrix sources from currently active voices ----
        {
            float lfo1Sum = 0.0f, lfo2Sum = 0.0f, envSum = 0.0f;
            float velSum  = 0.0f, ktSum   = 0.0f;
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
                const float inv = 1.0f / static_cast<float>(count);
                blockModSrc.lfo1     = lfo1Sum * inv;
                blockModSrc.lfo2     = lfo2Sum * inv;
                blockModSrc.env      = envSum  * inv;
                blockModSrc.velocity = velSum  * inv;
                blockModSrc.keyTrack = ktSum   * inv;
            }
            blockModSrc.modWheel   = modWheelValue;
            blockModSrc.aftertouch = aftertouchValue;
        }

        // ---- Apply mod matrix ----
        // Destinations: 0=Off, 1=Filter Cutoff, 2=Bandwidth, 3=Scale Balance,
        //               4=Env Decay, 5=Brightness, 6=Amp Level
        float modDestOffsets[7] = {};
        modMatrix.apply(blockModSrc, modDestOffsets);

        const float finalCutoff       = std::clamp(baseCutoff + modDestOffsets[1] * 8000.0f, 20.0f, 20000.0f);
        const float finalBandwidth    = std::clamp(effectiveBandwidth + modDestOffsets[2], 0.1f, 2.0f);
        const float finalScaleBalance = std::clamp(effectiveScaleBalance + modDestOffsets[3], 0.0f, 1.0f);
        const float finalEnvDecay     = std::clamp(envDecay + modDestOffsets[4] * 0.5f, 0.01f, 1.0f);
        const float finalBrightness   = std::clamp(brightness + modDestOffsets[5], 0.0f, 1.0f);
        const float modAmpLevel       = modDestOffsets[6];

        // ---- Output buffers ----
        auto* writeL = buffer.getWritePointer(0);
        auto* writeR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : writeL;
        buffer.clear();

        // ---- MIDI + render interleaved ----
        int midiSamplePos = 0;

        for (const auto& midiEvent : midi)
        {
            const auto& msg    = midiEvent.getMessage();
            const int   msgPos = std::min(midiEvent.samplePosition, numSamples - 1);

            renderVoicesRange(writeL, writeR, midiSamplePos, msgPos,
                              scaleCount, waveletType, finalBandwidth,
                              finalScaleBalance, scaleSpread, retrigMode,
                              finalBrightness, finalEnvDecay, phaseRandom,
                              pitchTrack, baseFreq,
                              ampAtk, ampDec, ampSus, ampRel,
                              finalCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                              fltAtk, fltDec, fltSus, fltRel,
                              lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                              lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                              velTimbre, glideCoeff, modAmpLevel);
            midiSamplePos = msgPos;

            if (msg.isNoteOn())
            {
                handleNoteOn(msg.getNoteNumber(), msg.getVelocity(),
                             ampAtk, ampDec, ampSus, ampRel,
                             fltAtk, fltDec, fltSus, fltRel,
                             voiceMode, glideTime);
            }
            else if (msg.isNoteOff())
            {
                handleNoteOff(msg.getNoteNumber());
            }
            else if (msg.isController() && msg.getControllerNumber() == 1)
            {
                modWheelValue = msg.getControllerValue() / 127.0f;
            }
            else if (msg.isChannelPressure())
            {
                aftertouchValue = msg.getChannelPressureValue() / 127.0f;
            }
            else if (msg.isAftertouch())
            {
                aftertouchValue = msg.getAfterTouchValue() / 127.0f;
            }
            else if (msg.isPitchWheel())
            {
                pitchBendValue = (msg.getPitchWheelValue() - 8192) / 8192.0f;
            }
        }

        // Render remaining samples after last MIDI event
        renderVoicesRange(writeL, writeR, midiSamplePos, numSamples,
                          scaleCount, waveletType, finalBandwidth,
                          finalScaleBalance, scaleSpread, retrigMode,
                          finalBrightness, finalEnvDecay, phaseRandom,
                          pitchTrack, baseFreq,
                          ampAtk, ampDec, ampSus, ampRel,
                          finalCutoff, fltReso, fltType, fltEnvAmt, fltKeyTrack,
                          fltAtk, fltDec, fltSus, fltRel,
                          lfo1Rate, lfo1Depth, lfo1Shape, lfo1Tgt,
                          lfo2Rate, lfo2Depth, lfo2Shape, lfo2Tgt,
                          velTimbre, glideCoeff, modAmpLevel);

        // Cache last output samples for coupling
        if (numSamples > 0)
        {
            lastSampleL      = writeL[numSamples - 1];
            lastSampleR      = writeR[numSamples - 1];
            lastScaleBalance = finalScaleBalance;
        }

        // Coupling accumulator decay — sample-rate and block-size independent (~1s time constant)
        const float couplingDecay = std::exp(-static_cast<float>(numSamples) / (sampleRateFloat * 1.0f));
        couplingAmpFilter *= couplingDecay;
        couplingEnvMorph  *= couplingDecay;
        couplingRhythm    *= couplingDecay;

        // Active voice count (atomic write)
        int av = 0;
        for (auto& v : voices)
            if (v.active) ++av;
        activeVoiceCount_.store(av, std::memory_order_relaxed);

        // SRO: SilenceGate analysis
        analyzeForSilenceGate(buffer, numSamples);
    }

private:

    //==========================================================================
    //  D S P   H E L P E R S
    //==========================================================================

    // Per-scale amplitude: D001 velocity -> upper scales get more shimmer at high velocity
    static float computeScaleAmplitude(int scaleIndex, int numScales,
                                       float scaleBalance, float brightness,
                                       float velocity, float velTimbre) noexcept
    {
        if (numScales <= 0) return 0.0f;
        const float t = static_cast<float>(scaleIndex)
            / static_cast<float>(std::max(1, numScales - 1));

        // scaleBalance=0 -> low scales loud; scaleBalance=1 -> high scales loud
        const float amp = lerp(1.0f - t, t, scaleBalance)
                        + brightness * t * 0.5f
                        + velocity * velTimbre * t * 0.8f; // D001
        return std::clamp(amp, 0.0f, 1.0f);
    }

    // Scale center frequency: scale 0 = fundamental, scale n = fundamental * 2^n
    static float computeScaleFreq(int scaleIndex, float fundamental,
                                  float pitchTrack, float baseFreq) noexcept
    {
        const float f = lerp(baseFreq, fundamental, pitchTrack);
        return f * fastPow2(static_cast<float>(scaleIndex));
    }

    // Envelope rate: inversely proportional to freq (natural wavelet behavior)
    static float computeEnvRate(int scaleIndex, float centerFreq,
                                float envDecay, float sampleRate) noexcept
    {
        // Low scales breathe slowly; high scales shimmer quickly
        const float scaleFactor = 0.02f + static_cast<float>(scaleIndex) * 0.015f;
        const float rate = (centerFreq / sampleRate) * scaleFactor / std::max(0.01f, envDecay);
        return flushDenormal(std::clamp(rate, 1e-7f, 0.1f));  // cap prevents sub-sample Gaussian collapse
    }

    // Stereo panning: scaleSpread=0 -> centered; scaleSpread=1 -> low=L, high=R
    static void computeScalePan(int scaleIndex, int numScales, float scaleSpread,
                                float& gainL, float& gainR) noexcept
    {
        if (numScales <= 1 || scaleSpread < 0.0001f)
        {
            gainL = gainR = 0.7071f;
            return;
        }
        const float t     = static_cast<float>(scaleIndex) / static_cast<float>(numScales - 1);
        const float pan   = lerp(-1.0f, 1.0f, t) * scaleSpread;
        const float angle = (pan + 1.0f) * 0.7853982f; // map [-1,1] to [0, pi/2]
        gainL = fastCos(angle);
        gainR = fastSin(angle);
    }

    //==========================================================================
    //  R E N D E R   V O I C E S   R A N G E
    //==========================================================================
    void renderVoicesRange(float* writeL, float* writeR,
                           int startSample, int endSample,
                           int scaleCount, int waveletType, float bandwidth,
                           float scaleBalance, float scaleSpread, int retrigMode,
                           float brightness, float envDecay, float phaseRandom,
                           float pitchTrack, float baseFreq,
                           float ampAtk, float ampDec, float ampSus, float ampRel,
                           float baseCutoff, float fltReso, int fltType,
                           float fltEnvAmt, float fltKeyTrack,
                           float fltAtk, float fltDec, float fltSus, float fltRel,
                           float lfo1Rate, float lfo1Depth, int lfo1Shape, int lfo1Tgt,
                           float lfo2Rate, float lfo2Depth, int lfo2Shape, int lfo2Tgt,
                           float velTimbre, float glideCoeff, float modAmpLevel) noexcept
    {
        if (startSample >= endSample) return;

        for (auto& v : voices)
        {
            if (!v.active) continue;

            // Set envelope params
            v.ampEnv.prepare(sampleRateFloat);
            v.ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
            v.filterEnv.prepare(sampleRateFloat);
            v.filterEnv.setADSR(fltAtk, fltDec, fltSus, fltRel);

            // LFO setup (block-rate)
            v.lfo1.setRate(lfo1Rate, sampleRateFloat);
            v.lfo1.setShape(lfo1Shape);
            v.lfo2.setRate(lfo2Rate, sampleRateFloat);
            v.lfo2.setShape(lfo2Shape);

            // Sample-rate-scaled amplitude smoother (~5ms smoothing time)
            const float ampSmoothCoeff = 1.0f - std::exp(-1.0f / (0.005f * sampleRateFloat));
            for (int sc = 0; sc < scaleCount; ++sc)
                v.gens[sc].smoothCoeff = ampSmoothCoeff;

            const float noteFundamental = midiToFreq(v.note);
            const float keyTrackHz = static_cast<float>(v.note - 60)
                * (baseCutoff * fltKeyTrack / 60.0f);

            const bool syncRetrig = v.syncRetrigRequest.load(std::memory_order_acquire);
            v.syncRetrigRequest.store(false, std::memory_order_relaxed);

            for (int s = startSample; s < endSample; ++s)
            {
                // Glide: smooth frequency
                if (glideCoeff > 0.0f)
                    v.glideFreq = v.glideFreq * glideCoeff + noteFundamental * (1.0f - glideCoeff);
                else
                    v.glideFreq = noteFundamental;
                v.glideFreq = flushDenormal(v.glideFreq);

                const float fundamental = std::max(10.0f, v.glideFreq
                    * fastPow2(pitchBendValue * 2.0f / 12.0f));  // ±2 semitones

                // LFO values (LIVE, per-sample, D002)
                const float lfo1Val = v.lfo1.process() * lfo1Depth;
                const float lfo2Val = v.lfo2.process() * lfo2Depth;
                v.lastLfo1Val = lfo1Val;
                v.lastLfo2Val = lfo2Val;

                // LFO target routing
                float lfoModBandwidth    = 0.0f;
                float lfoModScaleBalance = 0.0f;
                float lfoModCutoff       = 0.0f;
                float lfoModEnvDecay     = 0.0f;

                switch (lfo1Tgt)
                {
                case 0: lfoModBandwidth    += lfo1Val; break;
                case 1: lfoModScaleBalance += lfo1Val; break;
                case 2: lfoModCutoff       += lfo1Val; break;
                case 3: lfoModEnvDecay     += lfo1Val; break;
                default: break;
                }
                switch (lfo2Tgt)
                {
                case 0: lfoModBandwidth    += lfo2Val; break;
                case 1: lfoModScaleBalance += lfo2Val; break;
                case 2: lfoModCutoff       += lfo2Val; break;
                case 3: lfoModEnvDecay     += lfo2Val; break;
                default: break;
                }

                const float effBandwidth    = std::clamp(bandwidth + lfoModBandwidth,    0.1f, 2.0f);
                const float effScaleBalance = std::clamp(scaleBalance + lfoModScaleBalance, 0.0f, 1.0f);
                const float effEnvDecay     = std::clamp(envDecay + lfoModEnvDecay,       0.01f, 1.0f);

                // Amp envelope
                const float ampLevel = v.ampEnv.process();
                if (!v.ampEnv.isActive())
                {
                    v.active    = false;
                    v.releasing = false;
                    break;
                }

                // Filter envelope
                const float fltEnvLevel = v.filterEnv.process();

                // FM coupling for this sample
                const float fmMod = couplingFmBuf[std::min(static_cast<size_t>(s), static_cast<size_t>(maxBlock - 1))];

                // Sum all active wavelet generators
                float sumL = 0.0f;
                float sumR = 0.0f;
                float totalAmp = 0.0f;

                for (int sc = 0; sc < scaleCount; ++sc)
                {
                    const float centerFreq = computeScaleFreq(sc, fundamental, pitchTrack, baseFreq);
                    const float envRate    = computeEnvRate(sc, centerFreq, effEnvDecay, sampleRateFloat);

                    // Per-scale amplitude target (D001 velTimbre applies)
                    const float scaleAmp = computeScaleAmplitude(
                        sc, scaleCount, effScaleBalance, brightness, v.velocity, velTimbre);

                    v.gens[sc].amplitude = scaleAmp;

                    // Sync mode retrigger on RhythmToBlend
                    const bool forceRetrig = syncRetrig && (retrigMode == 1);

                    // FM mod attenuated for upper scales (physically motivated)
                    const float scaleFm = fmMod
                        * (1.0f - static_cast<float>(sc) / static_cast<float>(kOvertideNumScales));

                    const float waveSample = v.gens[sc].tick(
                        centerFreq, effBandwidth, envRate,
                        waveletType, retrigMode, sampleRateFloat,
                        scaleFm, forceRetrig, phaseRandom);

                    float gainL, gainR;
                    computeScalePan(sc, scaleCount, scaleSpread, gainL, gainR);

                    sumL += waveSample * gainL;
                    sumR += waveSample * gainR;
                    totalAmp += scaleAmp;
                }

                // Soft normalization to prevent clipping when many scales are active
                if (totalAmp > 0.001f)
                {
                    const float normFactor = 1.0f / (totalAmp + 0.5f);
                    sumL *= normFactor;
                    sumR *= normFactor;
                }

                sumL = flushDenormal(sumL);
                sumR = flushDenormal(sumR);

                // Filter coefficient update every 16 samples
                ++v.filterUpdateCounter;
                if (v.filterUpdateCounter >= 16)
                {
                    v.filterUpdateCounter = 0;

                    // Bipolar filter env mod: use != 0 check (negative sweeps downward)
                    float envContrib = 0.0f;
                    if (fltEnvAmt != 0.0f)
                        envContrib = fltEnvLevel * fltEnvAmt * 8000.0f;

                    const float effectiveCutoff = std::clamp(
                        baseCutoff + keyTrackHz + envContrib + lfoModCutoff * 4000.0f,
                        20.0f, 20000.0f);

                    CytomicSVF::Mode mode = CytomicSVF::Mode::LowPass;
                    switch (fltType)
                    {
                    case 1: mode = CytomicSVF::Mode::HighPass; break;
                    case 2: mode = CytomicSVF::Mode::BandPass; break;
                    case 3: mode = CytomicSVF::Mode::Notch;    break;
                    default: break;
                    }

                    v.filterL.setMode(mode);
                    v.filterR.setMode(mode);
                    v.filterL.setCoefficients_fast(effectiveCutoff, fltReso, sampleRateFloat);
                    v.filterR.setCoefficients_fast(effectiveCutoff, fltReso, sampleRateFloat);
                }

                sumL = v.filterL.processSample(sumL);
                sumR = v.filterR.processSample(sumR);

                sumL = flushDenormal(sumL);
                sumR = flushDenormal(sumR);

                // VCA: amp envelope x velocity x mod matrix amp
                const float vcaGain = ampLevel * v.velocity * (1.0f + modAmpLevel);
                sumL *= vcaGain;
                sumR *= vcaGain;

                writeL[s] += sumL;
                writeR[s] += sumR;
            }
        }
    }

    //==========================================================================
    //  N O T E   O N
    //==========================================================================
    void handleNoteOn(int note, int velocity,
                      float ampAtk, float ampDec, float ampSus, float ampRel,
                      float fltAtk, float fltDec, float fltSus, float fltRel,
                      int voiceMode, float glideTime) noexcept
    {
        const float vel01 = velocity / 127.0f;
        const int maxVoices = [voiceMode]()
        {
            switch (voiceMode)
            {
            case 0: return 1;
            case 1: return 1;
            case 2: return 4;
            default: return kOvertideMaxVoices;
            }
        }();

        // Find a free voice
        int voiceIdx = -1;
        for (int i = 0; i < maxVoices; ++i)
        {
            if (!voices[i].active)
            {
                voiceIdx = i;
                break;
            }
        }

        // Voice stealing: steal quietest releasing voice
        if (voiceIdx < 0)
        {
            // Pass 1: prefer quietest releasing voice
            voiceIdx = 0;
            float minLevel = 2.0f;
            for (int i = 0; i < maxVoices; ++i)
            {
                if (voices[i].releasing && voices[i].ampEnv.getLevel() < minLevel)
                {
                    minLevel = voices[i].ampEnv.getLevel();
                    voiceIdx = i;
                }
            }
            // Pass 2: if no releasing voice found, steal quietest overall
            if (minLevel >= 2.0f)
            {
                for (int i = 0; i < maxVoices; ++i)
                {
                    if (voices[i].ampEnv.getLevel() < minLevel)
                    {
                        minLevel = voices[i].ampEnv.getLevel();
                        voiceIdx = i;
                    }
                }
            }
        }

        auto& v = voices[voiceIdx];

        // Legato: preserve glideFreq from previous voice
        const float prevFreq = (voiceMode == 1 && v.active && glideTime > 0.0001f)
            ? v.glideFreq
            : midiToFreq(note);

        v.active    = true;
        v.releasing = false;
        v.note      = note;
        v.velocity  = vel01;
        v.keyTrack  = (static_cast<float>(note) - 60.0f) / 60.0f;
        v.glideFreq = prevFreq;

        // Reset wavelet envelope phases on new note (non-legato)
        for (int sc = 0; sc < kOvertideNumScales; ++sc)
        {
            v.gens[sc].amplitude = 1.0f;
            if (voiceMode != 1)
                v.gens[sc].envPhase = 0.0f;
        }

        v.filterUpdateCounter = 0;

        // Amp envelope
        if (voiceMode == 1 && v.ampEnv.isActive())
            v.ampEnv.retriggerFrom(v.ampEnv.getLevel(), ampAtk, ampDec, ampSus, ampRel);
        else
        {
            v.ampEnv.prepare(sampleRateFloat);
            v.ampEnv.setADSR(ampAtk, ampDec, ampSus, ampRel);
            v.ampEnv.noteOn();
        }

        // Filter envelope
        v.filterEnv.prepare(sampleRateFloat);
        v.filterEnv.setADSR(fltAtk, fltDec, fltSus, fltRel);
        v.filterEnv.noteOn();

        // LFO reset on new note (non-legato)
        if (voiceMode != 1)
        {
            v.lfo1.reset();
            v.lfo2.reset();
        }
    }

    //==========================================================================
    //  N O T E   O F F
    //==========================================================================
    void handleNoteOff(int note) noexcept
    {
        for (auto& v : voices)
        {
            if (v.active && !v.releasing && v.note == note)
            {
                v.releasing = true;
                v.ampEnv.noteOff();
                v.filterEnv.noteOff();
            }
        }
    }

    //==========================================================================
    //  S T A T E
    //==========================================================================

    // Do not default-init — must be set by prepare() on the live sample rate.
    // Sentinel 0.0 makes misuse before prepare() a crash instead of silent wrong-rate DSP.
    float sampleRateFloat = 0.0f;
    int   maxBlock        = 512;

    std::array<OvertideVoice, kOvertideMaxVoices> voices;

    // Coupling accumulators
    std::vector<float> couplingFmBuf;
    float couplingAmpFilter = 0.0f;
    float couplingEnvMorph  = 0.0f;
    float couplingRhythm    = 0.0f;

    // Expression
    float modWheelValue   = 0.0f;
    float aftertouchValue = 0.0f;
    float pitchBendValue  = 0.0f;  // [-1, +1] range, ±2 semitones

    // Coupling output cache
    float lastSampleL      = 0.0f;
    float lastSampleR      = 0.0f;
    float lastScaleBalance = 0.5f;

    // Mod matrix
    ModMatrix<4> modMatrix;
    ModMatrix<4>::Sources blockModSrc;

    //==========================================================================
    //  P A R A M E T E R   P O I N T E R S
    //==========================================================================

    std::atomic<float>* pWaveletType  = nullptr;
    std::atomic<float>* pScaleCount   = nullptr;
    std::atomic<float>* pBandwidth    = nullptr;
    std::atomic<float>* pScaleBalance = nullptr;
    std::atomic<float>* pScaleSpread  = nullptr;
    std::atomic<float>* pRetrigMode   = nullptr;
    std::atomic<float>* pBrightness   = nullptr;

    std::atomic<float>* pEnvDecay    = nullptr;
    std::atomic<float>* pPhaseRandom = nullptr;
    std::atomic<float>* pPitchTrack  = nullptr;
    std::atomic<float>* pBaseFreq    = nullptr;

    std::atomic<float>* pFltCutoff   = nullptr;
    std::atomic<float>* pFltReso     = nullptr;
    std::atomic<float>* pFltType     = nullptr;
    std::atomic<float>* pFltEnvAmt   = nullptr;
    std::atomic<float>* pFltKeyTrack = nullptr;
    std::atomic<float>* pFltAtk      = nullptr;
    std::atomic<float>* pFltDec      = nullptr;
    std::atomic<float>* pFltSus      = nullptr;
    std::atomic<float>* pFltRel      = nullptr;

    std::atomic<float>* pAmpAtk    = nullptr;
    std::atomic<float>* pAmpDec    = nullptr;
    std::atomic<float>* pAmpSus    = nullptr;
    std::atomic<float>* pAmpRel    = nullptr;
    std::atomic<float>* pVelTimbre = nullptr;

    std::atomic<float>* pLfo1Rate   = nullptr;
    std::atomic<float>* pLfo1Depth  = nullptr;
    std::atomic<float>* pLfo1Shape  = nullptr;
    std::atomic<float>* pLfo1Target = nullptr;
    std::atomic<float>* pLfo2Rate   = nullptr;
    std::atomic<float>* pLfo2Depth  = nullptr;
    std::atomic<float>* pLfo2Shape  = nullptr;
    std::atomic<float>* pLfo2Target = nullptr;

    std::atomic<float>* pMacro1    = nullptr;
    std::atomic<float>* pMacro2    = nullptr;
    std::atomic<float>* pMacro3    = nullptr;
    std::atomic<float>* pMacro4    = nullptr;
    std::atomic<float>* pVoiceMode = nullptr;
    std::atomic<float>* pGlide     = nullptr;
    std::atomic<float>* pGlideMode = nullptr;
};

} // namespace xoceanus
