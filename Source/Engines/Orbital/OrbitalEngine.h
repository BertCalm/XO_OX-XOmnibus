// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../../Core/SynthEngine.h"
#include "../../Core/PolyAftertouch.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/PitchBendUtil.h"
#include "../../DSP/SRO/SilenceGate.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/Effects/Saturator.h"
#include "../../DSP/StandardLFO.h"
#include <array>
#include <cmath>
#include <vector>

namespace xoceanus {

//==============================================================================
//
//  ORBITAL ENGINE — 64-Partial Additive Synthesis
//  XOceanus Engine Module | Accent: Warm Red #FF6B6B
//
//  Creature: The Circling Current
//  Habitat:  Open Water — the vast middle of the XO_OX water column
//
//  XOrbital lives in Open Water alongside Odyssey, Ostinato, and Obese. It
//  is a school of harmonic overtones circling like fish — 64 sine partials
//  per voice, each on its own orbit, collectively forming spectral shapes
//  that morph, breathe, and evolve. Where Obese achieves mass through
//  oscillator stacking, Orbital achieves complexity through harmonic control.
//
//  Historical Lineage:
//  Orbital descends from the additive synthesis tradition pioneered by the
//  Kawai K5 (1987) and Technos Acxel (1987), reaching its modern peak in
//  the Camel Audio Alchemy (2009) and Native Instruments Razor (2011).
//  The per-partial FM owes a debt to the Yamaha FS1R (1998), the only
//  commercial synth to marry additive and FM in the frequency domain.
//  The inharmonicity model draws from piano string physics — the same
//  stiffness equation used in physical modeling since Karplus-Strong (1983).
//
//  Architecture:
//    64 sine partials per voice x 6 voices = 384 max simultaneous partials.
//    Spectral profile morph (A <-> B), formant filter, 4-group envelopes,
//    optional per-partial FM, post-SVF filter, tube saturator.
//
//  Coupling (in the XO_OX ecosystem, coupling is symbiosis):
//    In:  AudioToWavetable (spectral DNA transfer from another species),
//         AmpToFilter, EnvToMorph, AudioToFM, AudioToRing,
//         LFOToPitch, PitchToPitch, EnvToDecay, RhythmToBlend
//    Out: post-filter stereo via outputCacheL/R (per-sample, tight coupling)
//
//==============================================================================


//==============================================================================
// FormantFilter
//
// A 64-point spectral envelope applied as per-partial amplitude multipliers.
// Two modes:
//   - Tilt + odd/even balance (vowelIndex == 0): sculpts the harmonic series
//     by applying a spectral rolloff slope and fading even or odd partials.
//   - Vowel formant (vowelIndex 1-5): three Gaussian peaks at F1/F2/F3
//     formant frequencies, simulating vowel resonances (A, E, I, O, U).
//
// Rebuilt only when parameters change (dirty flag). Zero per-sample cost.
//
// The formant frequencies below come from Peterson & Barney (1952), the
// foundational study of American English vowel formants — the same data
// used by Klatt's formant synthesizer and most vocal synthesis since.
//==============================================================================
struct FormantFilter
{
    std::array<float, 64> envelope {};

    void build (int vowelIndex, float fundamentalHz, float brightness,
                float oddEven, float shiftSemitones) noexcept
    {
        const float shiftedFundamentalHz = fundamentalHz
                                         * std::pow (2.0f, shiftSemitones / 12.0f);

        if (vowelIndex == 0)
        {
            //-- Spectral tilt + odd/even balance mode ---------------------------
            for (int partialIdx = 0; partialIdx < 64; ++partialIdx)
            {
                float partialNumber = static_cast<float> (partialIdx);

                // Spectral tilt: higher partials roll off faster as brightness
                // decreases. Exponent of 2.0 gives a natural 1/f^2 slope at
                // minimum brightness, matching the rolloff of a triangle wave.
                float tilt = std::pow (1.0f / (partialNumber + 1.0f),
                                       (1.0f - brightness) * 2.0f);

                // Odd/even balance: at 0.5 all partials equal; below 0.5 fades
                // evens (approaching square wave); above 0.5 fades odds
                // (keeping fundamental, approaching octave-rich timbre).
                float oddEvenGain = 1.0f;
                const bool isOddPartial = ((partialIdx + 1) % 2 == 1);
                if (oddEven < 0.5f && !isOddPartial)
                    oddEvenGain = oddEven * 2.0f;                           // fade even partials
                else if (oddEven > 0.5f && isOddPartial && partialIdx > 0)
                    oddEvenGain = 2.0f - (oddEven - 0.5f) * 2.0f;          // fade odd (keep fundamental)
                oddEvenGain = juce::jlimit (0.0f, 1.0f, oddEvenGain);

                envelope[partialIdx] = tilt * oddEvenGain;
            }
        }
        else
        {
            //-- Vowel formant mode: Gaussian peaks at F1, F2, F3 ---------------
            // Formant center frequencies (Hz) from Peterson & Barney (1952).
            // Each row = one vowel [F1, F2, F3], representing the three lowest
            // resonances of the vocal tract for that vowel sound.
            static const float kFormantCenterHz[5][3] = {
                { 730.0f, 1090.0f, 2440.0f },   // A — as in "father"
                { 530.0f, 1840.0f, 2480.0f },   // E — as in "bed"
                { 270.0f, 2290.0f, 3010.0f },   // I — as in "beet"
                { 570.0f,  840.0f, 2410.0f },   // O — as in "boat"
                { 300.0f,  870.0f, 2240.0f },   // U — as in "boot"
            };

            // Bandwidth (Hz) of each formant's Gaussian bell curve. Wider BW
            // on higher formants matches the natural acoustic behavior of the
            // vocal tract, where higher resonances are less sharply tuned.
            static const float kFormantBandwidthHz[3] = { 120.0f, 150.0f, 200.0f };

            // Peak amplitude for each formant. F1 loudest, F3 quietest —
            // matches the natural spectral energy distribution of speech.
            static const float kFormantPeakAmplitude[3] = { 1.0f, 0.7f, 0.4f };

            const int vowelArrayIdx = juce::jlimit (0, 4, vowelIndex - 1);
            const float* formantCenters = kFormantCenterHz[vowelArrayIdx];

            for (int partialIdx = 0; partialIdx < 64; ++partialIdx)
            {
                float partialFreq = static_cast<float> (partialIdx + 1) * shiftedFundamentalHz;
                float gain = 0.0f;

                for (int formantIdx = 0; formantIdx < 3; ++formantIdx)
                {
                    // Gaussian bell: exp(-0.5 * ((f - center) / bandwidth)^2)
                    float deviation = (partialFreq - formantCenters[formantIdx])
                                    / kFormantBandwidthHz[formantIdx];
                    gain += kFormantPeakAmplitude[formantIdx]
                          * std::exp (-0.5f * deviation * deviation);
                }

                envelope[partialIdx] = juce::jlimit (0.0f, 1.0f, gain);
            }
        }
    }

    float apply (int partialIdx, float amplitude) const noexcept
    {
        return amplitude * envelope[partialIdx];
    }
};


//==============================================================================
// OrbitalVoice — Per-voice state for a 64-partial additive synthesis voice.
//
// Each voice holds 64 phase accumulators (double precision to prevent pitch
// drift at low frequencies over long sustains), per-partial stereo pan
// positions, and two envelope systems:
//   - Global ADSR: controls the overall voice amplitude
//   - 4 group envelopes: independent AD envelopes for partial bands
//     (Group 0 = partials 0-7 "body", Group 1 = 8-15 "presence",
//      Group 2 = 16-31 "air", Group 3 = 32-63 "shimmer")
//==============================================================================
struct OrbitalVoice
{
    //-- Voice identity ----------------------------------------------------------
    bool     active        = false;
    int      noteNumber    = -1;
    float    velocity      = 0.0f;
    uint64_t startTime     = 0;
    float    fundamentalHz = 261.63f;   // Middle C default

    //-- Phase accumulators (double precision) ------------------------------------
    // Double precision is essential here: at 20 Hz fundamental, a float phase
    // accumulator loses ~0.1 cents of pitch accuracy after just 10 seconds.
    // Double precision stays sub-cent accurate for >24 hours of continuous play.
    double phase[64]       = {};
    double fmPhase[64]     = {};

    //-- Phase increments (cached at noteOn) -------------------------------------
    // Pre-computed once per note, incorporating inharmonic ratios. Avoids
    // per-sample sqrt and multiply in the hot render loop.
    float phaseIncrement[64]   = {};
    float fmPhaseIncrement[64] = {};

    //-- Per-partial stereo pan (constant-power, cached at noteOn) ----------------
    float panGainL[64] = {};
    float panGainR[64] = {};

    //-- Group envelopes (AD with sustain floor) ---------------------------------
    // 4 bands of partials get independent attack/decay envelopes, allowing
    // higher partials to bloom and decay at different rates than lower ones.
    // This is the key to Orbital's "living spectrum" character.
    //   Group 0: partials  0-7   (body — warm, slow, fundamental weight)
    //   Group 1: partials  8-15  (presence — vocal range, midrange definition)
    //   Group 2: partials 16-31  (air — breathy upper harmonics)
    //   Group 3: partials 32-63  (shimmer — the highest, most delicate partials)
    float groupEnvLevel[4]       = {};
    float groupAttackCoeff[4]    = {};
    float groupDecayCoeff[4]     = {};
    enum class GroupEnvStage { Attack, Decay, Sustain, Off };
    GroupEnvStage groupStage[4]  = {
        GroupEnvStage::Off, GroupEnvStage::Off,
        GroupEnvStage::Off, GroupEnvStage::Off
    };

    //-- Global ADSR envelope ----------------------------------------------------
    float envLevel          = 0.0f;
    float envAttackCoeff    = 0.0f;
    float envDecayCoeff     = 0.0f;
    float envSustainLevel   = 0.8f;
    float envReleaseCoeff   = 0.0f;
    enum class EnvStage { Attack, Decay, Sustain, Release, Off };
    EnvStage envStage       = EnvStage::Off;

    //-- Voice-steal crossfade ---------------------------------------------------
    float fadeOutLevel = 0.0f;
};


//==============================================================================
// OrbitalEngine
//==============================================================================
class OrbitalEngine : public SynthEngine
{
public:
    //-- Constants ---------------------------------------------------------------
    static constexpr int   kMaxVoices    = 6;
    static constexpr int   kNumPartials  = 64;
    static constexpr float kInvPartialCount = 1.0f / static_cast<float> (kNumPartials - 1);

    //-- Identity ----------------------------------------------------------------
    juce::String getEngineId()     const override { return "Orbital"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFFF6B6B); }   // Warm Red
    int          getMaxVoices()    const override { return kMaxVoices; }


    //==========================================================================
    //  LIFECYCLE
    //==========================================================================

    void prepare (double sampleRate, int maxBlockSize) override
    {
        cachedSampleRate       = sampleRate;
        cachedSampleRateFloat  = static_cast<float> (sampleRate);

        for (auto& voice : voices)
        {
            voice.active = false;
            std::fill (std::begin (voice.phase),   std::end (voice.phase),   0.0);
            std::fill (std::begin (voice.fmPhase), std::end (voice.fmPhase), 0.0);
        }

        // Voice-steal crossfade: 5ms fade-out prevents clicks when stealing
        // voices. At 44.1kHz this is ~220 samples — fast enough to be
        // inaudible but long enough to avoid discontinuity artifacts.
        fadeOutStepPerSample = 1.0f / (0.005f * cachedSampleRateFloat);

        outputCacheL.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.assign (static_cast<size_t> (maxBlockSize), 0.0f);

        aftertouch.prepare (sampleRate);

        silenceGate.prepare (sampleRate, maxBlockSize);

        couplingAudioBuffer.setSize (2, maxBlockSize, false, true, false);
        couplingRingBuffer .setSize (2, maxBlockSize, false, true, false);

        postFilterL.reset();
        postFilterR.reset();
        saturatorL.reset();
        saturatorR.reset();
        saturatorL.setMode (Saturator::SaturationMode::Tube);
        saturatorR.setMode (Saturator::SaturationMode::Tube);

        spectralCouplingOffset.fill (0.0f);
        formantDirty        = true;
        phaseWrapCounter    = 0;
        envelopeOutput      = 0.0f;
        voiceAllocationAge  = 0;
    }

    void releaseResources() override
    {
        for (auto& voice : voices)
        {
            voice.active   = false;
            voice.envStage = OrbitalVoice::EnvStage::Off;
        }
    }

    void reset() override
    {
        for (auto& voice : voices)
        {
            voice.active   = false;
            voice.envStage = OrbitalVoice::EnvStage::Off;
            voice.envLevel = 0.0f;
            for (int groupIdx = 0; groupIdx < 4; ++groupIdx)
            {
                voice.groupEnvLevel[groupIdx] = 0.0f;
                voice.groupStage[groupIdx]    = OrbitalVoice::GroupEnvStage::Off;
            }
            std::fill (std::begin (voice.phase),   std::end (voice.phase),   0.0);
            std::fill (std::begin (voice.fmPhase), std::end (voice.fmPhase), 0.0);
        }
        postFilterL.reset();
        postFilterR.reset();
        saturatorL.reset();
        saturatorR.reset();
        couplingAudioBuffer.clear();
        couplingRingBuffer .clear();
        spectralCouplingOffset.fill (0.0f);
        hasAudioCoupling = false;
        hasRingCoupling  = false;
        externalFilterMod = externalMorphMod = externalPitchMod = 0.0f;
        externalFmMod = externalDecayMod = externalBlendMod = 0.0f;
        envelopeOutput   = 0.0f;
        phaseWrapCounter = 0;
        formantDirty     = true;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }


    //==========================================================================
    //  PARAMETERS
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

    void attachParameters (juce::AudioProcessorValueTreeState& apvts) override
    {
        p_profileA           = apvts.getRawParameterValue ("orb_profileA");
        p_profileB           = apvts.getRawParameterValue ("orb_profileB");
        p_morph              = apvts.getRawParameterValue ("orb_morph");
        p_brightness         = apvts.getRawParameterValue ("orb_brightness");
        p_oddEven            = apvts.getRawParameterValue ("orb_oddEven");
        p_formantShape       = apvts.getRawParameterValue ("orb_formantShape");
        p_formantShift       = apvts.getRawParameterValue ("orb_formantShift");
        p_inharmonicity      = apvts.getRawParameterValue ("orb_inharm");
        p_fmIndex            = apvts.getRawParameterValue ("orb_fmIndex");
        p_fmRatio            = apvts.getRawParameterValue ("orb_fmRatio");
        p_groupAttack1       = apvts.getRawParameterValue ("orb_groupAttack1");
        p_groupDecay1        = apvts.getRawParameterValue ("orb_groupDecay1");
        p_groupAttack2       = apvts.getRawParameterValue ("orb_groupAttack2");
        p_groupDecay2        = apvts.getRawParameterValue ("orb_groupDecay2");
        p_groupAttack3       = apvts.getRawParameterValue ("orb_groupAttack3");
        p_groupDecay3        = apvts.getRawParameterValue ("orb_groupDecay3");
        p_groupAttack4       = apvts.getRawParameterValue ("orb_groupAttack4");
        p_groupDecay4        = apvts.getRawParameterValue ("orb_groupDecay4");
        p_filterCutoff       = apvts.getRawParameterValue ("orb_filterCutoff");
        p_filterResonance    = apvts.getRawParameterValue ("orb_filterReso");
        p_filterType         = apvts.getRawParameterValue ("orb_filterType");
        p_filterEnvDepth     = apvts.getRawParameterValue ("orb_filterEnvDepth");
        p_stereoSpread       = apvts.getRawParameterValue ("orb_stereoSpread");
        p_saturation         = apvts.getRawParameterValue ("orb_saturation");
        p_ampAttack          = apvts.getRawParameterValue ("orb_ampAttack");
        p_ampDecay           = apvts.getRawParameterValue ("orb_ampDecay");
        p_ampSustain         = apvts.getRawParameterValue ("orb_ampSustain");
        p_ampRelease         = apvts.getRawParameterValue ("orb_ampRelease");
        p_volume             = apvts.getRawParameterValue ("orb_volume");
        p_macroSpectrum      = apvts.getRawParameterValue ("orb_macroSpectrum");
        p_macroEvolve        = apvts.getRawParameterValue ("orb_macroEvolve");
        p_macroCoupling      = apvts.getRawParameterValue ("orb_macroCoupling");
        p_macroSpace         = apvts.getRawParameterValue ("orb_macroSpace");
        p_voiceMode          = apvts.getRawParameterValue ("orb_voiceMode");
    }


    //==========================================================================
    //  COUPLING — Symbiosis with the XO_OX ecosystem
    //==========================================================================

    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        if (sampleIndex < 0) return 0.0f;
        auto sampleIdx = static_cast<size_t> (sampleIndex);
        if (channel == 0 && sampleIdx < outputCacheL.size()) return outputCacheL[sampleIdx];
        if (channel == 1 && sampleIdx < outputCacheR.size()) return outputCacheR[sampleIdx];
        if (channel == 2) return envelopeOutput;   // envelope follower for cross-engine mod
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
            //-- AudioToWavetable: another engine's audio becomes spectral DNA ---
            // The source audio's RMS energy lifts upper partials, transferring
            // the timbral "fingerprint" of one species into Orbital's harmonic grid.
            case CouplingType::AudioToWavetable:
                if (sourceBuffer != nullptr)
                {
                    float* couplingWrite = couplingAudioBuffer.getWritePointer (0);
                    for (int n = 0; n < numSamples; ++n)
                        couplingWrite[n] = sourceBuffer[n] * amount;
                    hasAudioCoupling = true;
                }
                break;

            //-- AudioToFM: another engine's audio modulates partial frequencies -
            case CouplingType::AudioToFM:
                if (sourceBuffer != nullptr)
                {
                    float* couplingWrite = couplingAudioBuffer.getWritePointer (1);
                    for (int n = 0; n < numSamples; ++n)
                        couplingWrite[n] = sourceBuffer[n] * amount;
                    externalFmMod = amount;
                }
                break;

            //-- AudioToRing: ring modulation — metallic, bell-like coupling -----
            case CouplingType::AudioToRing:
                if (sourceBuffer != nullptr)
                {
                    float* couplingWrite = couplingRingBuffer.getWritePointer (0);
                    for (int n = 0; n < numSamples; ++n)
                        couplingWrite[n] = sourceBuffer[n] * amount;
                    hasRingCoupling = true;
                }
                break;

            //-- AmpToFilter: external amplitude drives filter cutoff ------------
            // Scale factor of 8000 Hz maps full coupling amount to roughly a
            // 3-octave sweep from a typical cutoff starting point — enough to
            // create dramatic filter pumping without overshooting Nyquist.
            case CouplingType::AmpToFilter:
                externalFilterMod = amount * 8000.0f;
                break;

            //-- EnvToMorph: external envelope drives spectral morph position ----
            // Scale of 0.5 means full coupling shifts morph halfway, preventing
            // external sources from completely overriding the user's morph setting.
            case CouplingType::EnvToMorph:
                externalMorphMod = amount * 0.5f;
                break;

            //-- LFOToPitch: slow modulation for vibrato/drift effects -----------
            // Scale of 2 semitones at full amount — musical vibrato range.
            case CouplingType::LFOToPitch:
                externalPitchMod += amount * 2.0f;
                break;

            //-- PitchToPitch: hard pitch tracking/transposition ------------------
            // Scale of 12 semitones (one octave) at full amount.
            case CouplingType::PitchToPitch:
                externalPitchMod += amount * 12.0f;
                break;

            //-- EnvToDecay: external envelope damps group envelope levels --------
            case CouplingType::EnvToDecay:
                externalDecayMod = juce::jlimit (-1.0f, 1.0f, amount);
                break;

            //-- RhythmToBlend: rhythmic pattern drives morph position -----------
            case CouplingType::RhythmToBlend:
                externalBlendMod = juce::jlimit (0.0f, 1.0f, amount);
                break;

            default:
                break;
        }
    }


    //==========================================================================
    //  AUDIO RENDER — The heart of the Circling Current
    //==========================================================================

    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (p_profileA == nullptr || numSamples <= 0) return;

        // Denormal protection: additive synthesis with 384 simultaneous sine
        // waves generates many near-zero values during envelope release tails.
        // Without this guard, the CPU spends 10-100x longer processing denormal
        // floats through the FPU's microcode fallback path, causing audio dropouts.
        juce::ScopedNoDenormals noDenormals;

        buffer.clear();
        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

        //-- ParamSnapshot: cache all parameter values once per block -----------
        // This is the XOceanus ParamSnapshot pattern. Loading atomic floats
        // once per block (not per sample) eliminates cache-line contention
        // and gives the compiler freedom to optimize the inner loop.
        const int   profileAIndex     = static_cast<int> (p_profileA->load());
        const int   profileBIndex     = static_cast<int> (p_profileB->load());
        float morphPosition           = p_morph->load();
        float brightness              = p_brightness->load();
        float oddEven                 = p_oddEven->load();
        const int   vowelIndex        = static_cast<int> (p_formantShape->load());
        float formantShift            = p_formantShift->load();
        const float inharmonicity     = p_inharmonicity->load();
        const float fmIndex           = p_fmIndex->load();
        const float fmRatio           = p_fmRatio->load();
        float groupAttackTimes[4]     = { p_groupAttack1->load(), p_groupAttack2->load(),
                                          p_groupAttack3->load(), p_groupAttack4->load() };
        float groupDecayTimes[4]      = { p_groupDecay1->load(), p_groupDecay2->load(),
                                          p_groupDecay3->load(), p_groupDecay4->load() };
        const float filterCutoff      = p_filterCutoff->load();
        const float filterResonance   = p_filterResonance->load();
        const int   filterTypeIndex   = static_cast<int> (p_filterType->load());
        float stereoSpread            = p_stereoSpread->load();
        const float saturation        = p_saturation->load();
        const float ampAttack         = p_ampAttack->load();
        const float ampDecay          = p_ampDecay->load();
        const float ampSustain        = p_ampSustain->load();
        const float ampRelease        = p_ampRelease->load();
        const float volume            = p_volume->load();
        // Round 11D: voice mode (0=Poly, 1=Mono, 2=Legato)
        const int   voiceMode         = (p_voiceMode != nullptr)
                                        ? static_cast<int> (p_voiceMode->load()) : 0;

        //-- Apply macros -------------------------------------------------------
        applyMacros (brightness, oddEven, morphPosition, stereoSpread,
                     formantShift, groupAttackTimes, groupDecayTimes);

        //-- Rebuild formant filter if parameters have changed ------------------
        // Thresholds for dirty detection: 0.005 for normalized params avoids
        // rebuilding on sub-perceptual jitter; 0.25 semitones for formant shift
        // avoids rebuilding on inaudible micro-changes.
        if (formantDirty
            || vowelIndex      != lastVowelIndex
            || std::abs (brightness   - lastBrightness)   > 0.005f
            || std::abs (oddEven      - lastOddEven)      > 0.005f
            || std::abs (formantShift - lastFormantShift)  > 0.25f)
        {
            formantFilter.build (vowelIndex, lastFundamentalHz,
                                 brightness, oddEven, formantShift);
            lastVowelIndex   = vowelIndex;
            lastBrightness   = brightness;
            lastOddEven      = oddEven;
            lastFormantShift = formantShift;
            formantDirty     = false;
        }

        //-- Spectral coupling offset (AudioToWavetable) ------------------------
        // When another engine feeds audio into Orbital, we compute the RMS
        // energy and use it to gently lift upper partials. The weighting
        // factor (0.3 at the top partial) is subtle — coupling should flavor
        // the sound, not dominate it. This is commensalism, not parasitism.
        if (hasAudioCoupling)
        {
            float rmsAccumulator = 0.0f;
            const float* couplingRead = couplingAudioBuffer.getReadPointer (0);
            for (int n = 0; n < numSamples; ++n)
                rmsAccumulator += couplingRead[n] * couplingRead[n];
            float rmsLevel = std::sqrt (rmsAccumulator / static_cast<float> (numSamples));
            rmsLevel = juce::jlimit (0.0f, 1.0f, rmsLevel);

            for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
            {
                // Weight increases linearly with partial index: upper partials
                // get more coupling influence, creating brightness transfer.
                float partialWeight = static_cast<float> (partialIdx) * kInvPartialCount;
                // 0.3 max lift: enough to add shimmer without destabilizing the spectrum
                spectralCouplingOffset[partialIdx] = rmsLevel * partialWeight * 0.3f;
            }
            couplingAudioBuffer.clear();
            hasAudioCoupling = false;
        }
        else
        {
            spectralCouplingOffset.fill (0.0f);
        }

        //-- Consume + reset coupling accumulators ------------------------------
        const float pitchOffset  = externalPitchMod + pitchBendNorm * 2.0f;
        const float morphOffset  = externalMorphMod;
        const float filterOffset = externalFilterMod;
        const float fmAudioAmount = externalFmMod;
        externalPitchMod = externalMorphMod = externalFilterMod = 0.0f;
        externalFmMod = externalDecayMod = externalBlendMod = 0.0f;

        // D005 fix: minimal LFO added — 0.03 Hz spectral morph breathing (±0.05)
        // D006: mod wheel increases drift rate up to +0.3 Hz at full wheel (sensitivity 0.3)
        // Replaced inline radians accumulator with shared BreathingLFO (Source/DSP/StandardLFO.h).
        const float spectralDriftRate = 0.03f + modWheelValue * 0.3f;
        spectralDriftLFO.setRate (spectralDriftRate, cachedSampleRateFloat);
        // D006: aftertouch added below — atPressure resolved after MIDI loop
        float effectiveMorph  = juce::jlimit (0.0f, 1.0f,
            morphPosition + morphOffset + 0.05f * spectralDriftLFO.process());
        // D001: compute peak velocity × envLevel across all active voices this block.
        // The post-mix SVF filter receives a boost proportional to how hard the notes
        // are playing — louder/harder hits open the filter, satisfying D001 (velocity
        // must shape timbre). kFilterEnvMaxHz = 7000 Hz sweep range.
        static constexpr float kFilterEnvMaxHz = 7000.0f;
        const float filterEnvDepth = (p_filterEnvDepth != nullptr) ? p_filterEnvDepth->load() : 0.25f;
        float peakVelEnv = 0.0f;
        for (const auto& voice : voices)
        {
            if (voice.active)
                peakVelEnv = std::max (peakVelEnv, voice.envLevel * voice.velocity);
        }
        const float filterEnvBoost = filterEnvDepth * peakVelEnv * kFilterEnvMaxHz;

        const float effectiveCutoff = juce::jlimit (20.0f, 20000.0f,
                                                    filterCutoff + filterOffset + filterEnvBoost);

        //-- Process MIDI -------------------------------------------------------
        for (const auto& meta : midi)
        {
            auto message = meta.getMessage();
            if (message.isNoteOn())
            {
                silenceGate.wake();
                // Round 11D: Legato mode — if a voice is gate-open (active and not
                // in release), slide its pitch to the new note without retriggering
                // the envelope. This preserves the spectral bloom during legato lines.
                // In Poly (0) or Mono (1), always retrigger (triggerVoice path).
                // In Legato (2), retrigger only if no gate-open voice exists.
                if (voiceMode == 2) // Legato
                {
                    bool legatoHandled = false;
                    for (auto& voice : voices)
                    {
                        if (voice.active
                            && voice.envStage != OrbitalVoice::EnvStage::Release
                            && voice.envStage != OrbitalVoice::EnvStage::Off)
                        {
                            // Slide pitch: recompute phase increments for new note.
                            // Phase accumulators are preserved — no reset — so the
                            // partial waves continue unbroken (legato character).
                            legatoSlideVoice (voice, message.getNoteNumber(),
                                              inharmonicity, fmRatio);
                            legatoHandled = true;
                            break;
                        }
                    }
                    if (!legatoHandled)
                        triggerVoice (message.getNoteNumber(), message.getFloatVelocity(),
                                      inharmonicity, fmRatio, stereoSpread,
                                      ampAttack, ampDecay, ampSustain, ampRelease,
                                      groupAttackTimes, groupDecayTimes);
                }
                else
                {
                    triggerVoice (message.getNoteNumber(), message.getFloatVelocity(),
                                  inharmonicity, fmRatio, stereoSpread,
                                  ampAttack, ampDecay, ampSustain, ampRelease,
                                  groupAttackTimes, groupDecayTimes);
                }
            }
            else if (message.isNoteOff())
                noteOff (message.getNoteNumber());
            else if (message.isAllNotesOff() || message.isAllSoundOff())
                for (auto& voice : voices)
                {
                    voice.active   = false;
                    voice.envStage = OrbitalVoice::EnvStage::Off;
                }
            // D006: channel pressure → aftertouch (applied to morph position below)
            else if (message.isChannelPressure())
                aftertouch.setChannelPressure (message.getChannelPressureValue() / 127.0f);
            // D006: CC1 mod wheel → spectral morph drift rate (faster drift with wheel; sensitivity 0.3)
            else if (message.isController() && message.getControllerNumber() == 1)
                modWheelValue = message.getControllerValue() / 127.0f;
            else if (message.isPitchWheel()) pitchBendNorm = PitchBendUtil::parsePitchWheel(message.getPitchWheelValue());
        }

        if (silenceGate.isBypassed() && midi.isEmpty()) { buffer.clear(); return; }

        // D006: smooth aftertouch and apply to morph — pressure pushes toward profile B
        aftertouch.updateBlock (numSamples);
        const float atPressure = aftertouch.getSmoothedPressure (0);
        // Sensitivity 0.3: full pressure moves morph up to +0.3 toward profile B
        effectiveMorph = juce::jlimit (0.0f, 1.0f, effectiveMorph + atPressure * 0.3f);

        //-- Cache filter coefficients once per block ---------------------------
        // Computing filter coefficients per block (not per sample) saves ~3%
        // CPU. The audible difference is negligible for cutoff movements slower
        // than the block rate (~1.4ms at 512 samples / 44.1kHz).
        const auto filterMode = (filterTypeIndex == 1) ? CytomicSVF::Mode::BandPass
                              : (filterTypeIndex == 2) ? CytomicSVF::Mode::HighPass
                                                       : CytomicSVF::Mode::LowPass;
        postFilterL.setMode (filterMode);
        postFilterR.setMode (filterMode);
        postFilterL.setCoefficients (effectiveCutoff, filterResonance, cachedSampleRateFloat);
        postFilterR.setCoefficients (effectiveCutoff, filterResonance, cachedSampleRateFloat);

        // Saturation bypass threshold: values below 0.001 are inaudible,
        // so skip the tanh processing entirely to save CPU.
        const bool saturationActive = saturation > 0.001f;
        if (saturationActive)
        {
            // Drive scaled by 0.5: maps the 0-1 user range to 0-0.5 internal
            // drive, keeping the tube saturation warm rather than aggressive.
            saturatorL.setDrive (saturation * 0.5f);
            saturatorL.setMix   (saturation);
            saturatorR.setDrive (saturation * 0.5f);
            saturatorR.setMix   (saturation);
        }

        const bool fmActive         = fmIndex > 0.001f;
        const bool externalFmActive = fmAudioAmount > 0.001f;
        const bool pitchModActive   = std::abs (pitchOffset) > 0.01f;

        // Pitch ratio from semitone offset: 2^(semitones/12) = e^(semitones * ln2/12).
        // The constant 0.0578 = ln(2)/12, converting semitones to the natural
        // exponential domain. Using fastExp avoids std::pow per sample.
        const float pitchRatio = pitchModActive
                               ? fastExp (pitchOffset * 0.0578f) : 1.0f;

        const float* profileAmplitudesA = sProfiles[juce::jlimit (0, 7, profileAIndex)].data();
        const float* profileAmplitudesB = sProfiles[juce::jlimit (0, 7, profileBIndex)].data();
        const float* fmAudioPointer = externalFmActive
                                    ? couplingAudioBuffer.getReadPointer (1) : nullptr;
        const float* ringModPointer = hasRingCoupling
                                    ? couplingRingBuffer.getReadPointer (0)  : nullptr;


        //======================================================================
        //  PER-SAMPLE RENDER LOOP
        //  64 partials x 6 voices = up to 384 sine oscillators per sample
        //======================================================================

        float envelopePeak = 0.0f;

        for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
        {
            float mixL = 0.0f, mixR = 0.0f;
            const float externalFmSample = externalFmActive
                                         ? fmAudioPointer[sampleIdx] * fmAudioAmount
                                         : 0.0f;

            for (auto& voice : voices)
            {
                if (!voice.active) continue;

                //-- Tick global ADSR envelope ---------------------------------
                switch (voice.envStage)
                {
                    case OrbitalVoice::EnvStage::Attack:
                        voice.envLevel += voice.envAttackCoeff;
                        if (voice.envLevel >= 1.0f)
                        {
                            voice.envLevel = 1.0f;
                            voice.envStage = OrbitalVoice::EnvStage::Decay;
                        }
                        break;

                    case OrbitalVoice::EnvStage::Decay:
                        voice.envLevel -= voice.envDecayCoeff;
                        // Flush denormals: during decay, the level approaches
                        // sustain asymptotically. Without flushing, the FPU
                        // hits denormal territory and performance collapses.
                        voice.envLevel = flushDenormal (voice.envLevel);
                        if (voice.envLevel <= voice.envSustainLevel)
                        {
                            voice.envLevel = voice.envSustainLevel;
                            voice.envStage = OrbitalVoice::EnvStage::Sustain;
                        }
                        break;

                    case OrbitalVoice::EnvStage::Sustain:
                        break;

                    case OrbitalVoice::EnvStage::Release:
                        voice.envLevel -= voice.envReleaseCoeff;
                        // Flush denormals: release tails approach zero and are
                        // the primary source of denormal values in additive synths.
                        voice.envLevel = flushDenormal (voice.envLevel);
                        if (voice.envLevel <= 0.0f)
                        {
                            voice.envLevel = 0.0f;
                            voice.envStage = OrbitalVoice::EnvStage::Off;
                            voice.active   = false;
                            continue;
                        }
                        break;

                    case OrbitalVoice::EnvStage::Off:
                        voice.active = false;
                        continue;
                }

                //-- Tick group envelopes (AD with sustain floor) ---------------
                for (int groupIdx = 0; groupIdx < 4; ++groupIdx)
                {
                    switch (voice.groupStage[groupIdx])
                    {
                        case OrbitalVoice::GroupEnvStage::Attack:
                            voice.groupEnvLevel[groupIdx] += voice.groupAttackCoeff[groupIdx];
                            if (voice.groupEnvLevel[groupIdx] >= 1.0f)
                            {
                                voice.groupEnvLevel[groupIdx] = 1.0f;
                                voice.groupStage[groupIdx] = OrbitalVoice::GroupEnvStage::Decay;
                            }
                            break;

                        case OrbitalVoice::GroupEnvStage::Decay:
                            voice.groupEnvLevel[groupIdx] -= voice.groupDecayCoeff[groupIdx];
                            voice.groupEnvLevel[groupIdx] = flushDenormal (voice.groupEnvLevel[groupIdx]);
                            if (voice.groupEnvLevel[groupIdx] <= kGroupSustainFloor[groupIdx])
                            {
                                voice.groupEnvLevel[groupIdx] = kGroupSustainFloor[groupIdx];
                                voice.groupStage[groupIdx] = OrbitalVoice::GroupEnvStage::Sustain;
                            }
                            break;

                        case OrbitalVoice::GroupEnvStage::Sustain:
                        case OrbitalVoice::GroupEnvStage::Off:
                            break;
                    }

                    // External decay modulation: gently damps group levels when
                    // another engine's envelope is coupled via EnvToDecay.
                    // The 0.001 scale keeps the damping subtle per sample — it
                    // accumulates over hundreds of samples for a smooth effect.
                    if (externalDecayMod != 0.0f)
                        voice.groupEnvLevel[groupIdx] = std::max (kGroupSustainFloor[groupIdx],
                            voice.groupEnvLevel[groupIdx] * (1.0f - externalDecayMod * 0.001f));

                    voice.groupEnvLevel[groupIdx] = flushDenormal (voice.groupEnvLevel[groupIdx]);
                }

                //-- 64-partial additive synthesis (the circling school of fish) -
                float voiceL = 0.0f, voiceR = 0.0f;

                for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
                {
                    // Map partial index to group: 0-7=body, 8-15=presence,
                    // 16-31=air, 32-63=shimmer. Logarithmic grouping gives
                    // finer control over the perceptually important lower partials.
                    const int groupIdx = (partialIdx < 8)  ? 0
                                       : (partialIdx < 16) ? 1
                                       : (partialIdx < 32) ? 2 : 3;

                    // Morph between Profile A and B amplitudes, then apply
                    // formant shaping and group envelope
                    float amplitude = lerp (profileAmplitudesA[partialIdx],
                                            profileAmplitudesB[partialIdx],
                                            effectiveMorph)
                                    * formantFilter.envelope[partialIdx]
                                    * voice.groupEnvLevel[groupIdx];

                    // Add spectral coupling offset (energy from another engine)
                    amplitude = std::max (0.0f, amplitude + spectralCouplingOffset[partialIdx]);

                    // Skip silent partials (amplitude below -60dB) for performance
                    if (amplitude < 0.001f) continue;

                    //-- Phase accumulation with optional FM ---------------------
                    if (fmActive || externalFmActive)
                    {
                        float fmModulation = externalFmSample;
                        if (fmActive)
                            fmModulation += fmIndex * fastSin (static_cast<float> (voice.fmPhase[partialIdx]));

                        voice.phase[partialIdx]   += static_cast<double> (
                            voice.phaseIncrement[partialIdx] * pitchRatio * (1.0f + fmModulation));
                        voice.fmPhase[partialIdx] += static_cast<double> (
                            voice.fmPhaseIncrement[partialIdx]);
                    }
                    else
                    {
                        voice.phase[partialIdx] += static_cast<double> (
                            voice.phaseIncrement[partialIdx] * pitchRatio);
                    }

                    // Synthesize sine partial and pan to stereo
                    const float sineSample = amplitude
                                           * fastSin (static_cast<float> (voice.phase[partialIdx]));
                    voiceL += sineSample * voice.panGainL[partialIdx];
                    voiceR += sineSample * voice.panGainR[partialIdx];
                }

                //-- Voice-stealing crossfade ----------------------------------
                float stealFade = 1.0f;
                if (voice.fadeOutLevel > 0.0f)
                {
                    voice.fadeOutLevel -= fadeOutStepPerSample;
                    voice.fadeOutLevel = flushDenormal (voice.fadeOutLevel);
                    if (voice.fadeOutLevel < 0.0f) voice.fadeOutLevel = 0.0f;
                    stealFade = 1.0f - voice.fadeOutLevel;
                }

                const float voiceGain = voice.envLevel * voice.velocity * stealFade;
                mixL += voiceL * voiceGain;
                mixR += voiceR * voiceGain;
                envelopePeak = std::max (envelopePeak, voice.envLevel);
            }

            //-- Ring modulation coupling --------------------------------------
            if (ringModPointer != nullptr)
            {
                mixL *= ringModPointer[sampleIdx];
                mixR *= ringModPointer[sampleIdx];
            }

            //-- Post filter + saturator + volume (all per-sample) -------------
            float outputSampleL = postFilterL.processSample (mixL);
            float outputSampleR = postFilterR.processSample (mixR);
            if (saturationActive)
            {
                outputSampleL = saturatorL.processSample (outputSampleL);
                outputSampleR = saturatorR.processSample (outputSampleR);
            }
            outputSampleL *= volume;
            outputSampleR *= volume;

            outL[sampleIdx] = outputSampleL;
            outR[sampleIdx] = outputSampleR;

            //-- Cache output for coupling reads from other engines -------------
            if (sampleIdx < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (sampleIdx)] = outputSampleL;
                outputCacheR[static_cast<size_t> (sampleIdx)] = outputSampleR;
            }
        } // end per-sample loop

        envelopeOutput = envelopePeak;

        //-- Phase wrap: prevent double-precision overflow ----------------------
        // Even with double precision, phase accumulators will eventually lose
        // precision after ~10^15 increments. Wrapping every 1024 samples
        // (= ~23ms at 44.1kHz) keeps phases small. The cost is negligible
        // compared to the 384-partial inner loop.
        phaseWrapCounter += numSamples;
        if (phaseWrapCounter >= 1024)
        {
            wrapPhases();
            phaseWrapCounter &= 0x3FF;   // Equivalent to modulo 1024
        }

        //-- Reset ring coupling for next block --------------------------------
        if (hasRingCoupling)
        {
            couplingRingBuffer.clear();
            hasRingCoupling = false;
        }
        if (externalFmActive)
            couplingAudioBuffer.clear (1, 0, numSamples);

        silenceGate.analyzeBlock (buffer.getReadPointer (0), buffer.getReadPointer (1), numSamples);
    }


private:

    SilenceGate silenceGate;

    //==========================================================================
    //  VOICE MANAGEMENT
    //==========================================================================

    void triggerVoice (int note, float vel,
                       float inharmonicity, float fmRatio, float stereoSpread,
                       float attackSeconds, float decaySeconds,
                       float sustainLevel, float releaseSeconds,
                       const float groupAttackTimes[4], const float groupDecayTimes[4])
    {
        const int voiceIdx = findFreeVoice();
        auto& voice = voices[static_cast<size_t> (voiceIdx)];

        // Crossfade: if stealing an active voice, fade out from current level
        voice.fadeOutLevel  = voice.active ? voice.envLevel : 0.0f;
        voice.active        = true;
        voice.noteNumber    = note;
        voice.velocity      = vel;
        voice.startTime     = voiceAllocationAge++;
        voice.fundamentalHz = midiToFreq (note);

        lastFundamentalHz = voice.fundamentalHz;
        formantDirty      = true;   // rebuild formant for new fundamental

        //-- Inharmonic frequency ratios (piano string stiffness model) ---------
        // B coefficient from Fletcher's piano string equation:
        //   f_n = n * f_1 * sqrt(1 + B * n^2)
        // where B depends on string stiffness. The 0.001 scale maps the
        // user-friendly 0-1 range to physically plausible B values.
        // At B=0, partials are perfectly harmonic (ideal string).
        // At B=0.001, upper partials are stretched ~50 cents (like a real piano).
        // The sqrt is expensive but only computed once per noteOn, not per sample.
        const float stiffnessCoeff = inharmonicity * 0.001f;
        constexpr double twoPi = 6.283185307179586;   // 2 * pi
        const double inverseSampleRate = 1.0 / cachedSampleRate;

        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
        {
            const float partialNumber = static_cast<float> (partialIdx + 1);

            // Fletcher's inharmonicity formula: ratio = sqrt(1 + B * n^2)
            const float inharmonicRatio = std::sqrt (1.0f + stiffnessCoeff
                                                           * partialNumber
                                                           * partialNumber);

            const double partialFreq = static_cast<double> (voice.fundamentalHz
                                                           * partialNumber
                                                           * inharmonicRatio);

            // Phase increment = freq * 2pi / sampleRate (radians per sample)
            voice.phaseIncrement[partialIdx]   = static_cast<float> (
                partialFreq * twoPi * inverseSampleRate);
            voice.fmPhaseIncrement[partialIdx] = static_cast<float> (
                partialFreq * static_cast<double> (fmRatio) * twoPi * inverseSampleRate);

            voice.phase[partialIdx]   = 0.0;
            voice.fmPhase[partialIdx] = 0.0;
        }

        //-- Per-partial stereo pan (constant-power spread) ---------------------
        // Higher partials spread wider in the stereo field, alternating L/R.
        // This creates the immersive "orbital" quality — the school of partials
        // circling around the listener's head.
        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
        {
            float spreadAmount = stereoSpread
                               * static_cast<float> (partialIdx) * kInvPartialCount;

            // Alternate even partials left, odd partials right
            float panPosition = 0.5f + ((partialIdx % 2 == 0) ? -spreadAmount : spreadAmount) * 0.5f;
            panPosition = juce::jlimit (0.0f, 1.0f, panPosition);

            // Constant-power panning: cos/sin ensures equal energy regardless
            // of pan position. 1.5707963 = pi/2, mapping 0-1 to 0-90 degrees.
            voice.panGainL[partialIdx] = fastCos (panPosition * 1.5707963f);
            voice.panGainR[partialIdx] = fastSin (panPosition * 1.5707963f);
        }

        //-- Global ADSR coefficients -------------------------------------------
        // Linear increment/decrement per sample. Min attack of 1ms prevents
        // division by zero; min decay/release of 50ms prevents clicks.
        voice.envLevel        = 0.0f;
        voice.envStage        = OrbitalVoice::EnvStage::Attack;
        voice.envAttackCoeff  = 1.0f / (std::max (0.001f, attackSeconds)  * cachedSampleRateFloat);
        voice.envDecayCoeff   = 1.0f / (std::max (0.05f,  decaySeconds)   * cachedSampleRateFloat);
        voice.envSustainLevel = sustainLevel;
        voice.envReleaseCoeff = 1.0f / (std::max (0.05f,  releaseSeconds) * cachedSampleRateFloat);

        //-- Group envelope coefficients ----------------------------------------
        for (int groupIdx = 0; groupIdx < 4; ++groupIdx)
        {
            voice.groupEnvLevel[groupIdx]    = 0.0f;
            voice.groupStage[groupIdx]       = OrbitalVoice::GroupEnvStage::Attack;
            voice.groupAttackCoeff[groupIdx] = 1.0f / (std::max (0.001f, groupAttackTimes[groupIdx])
                                                       * cachedSampleRateFloat);
            voice.groupDecayCoeff[groupIdx]  = 1.0f / (std::max (0.01f,  groupDecayTimes[groupIdx])
                                                       * cachedSampleRateFloat);
        }
    }

    void noteOff (int noteNumber)
    {
        for (auto& voice : voices)
        {
            if (voice.active && voice.noteNumber == noteNumber
                && voice.envStage != OrbitalVoice::EnvStage::Release
                && voice.envStage != OrbitalVoice::EnvStage::Off)
            {
                voice.envStage = OrbitalVoice::EnvStage::Release;
            }
        }
    }

    // Round 11D: legatoSlideVoice — retarget pitch without envelope retrigger.
    //
    // Updates fundamentalHz and recomputes all 64 phase increments for the new note.
    // Phase accumulators are deliberately NOT reset — the partial waves continue
    // unbroken, creating a smooth "glide" from one pitch to another rather than
    // a percussive attack. This is the additive synthesis equivalent of portamento:
    // the spectrum transforms continuously rather than snapping.
    //
    // The inharmonicity and fmRatio values are unchanged (inherited from the
    // previous renderBlock ParamSnapshot), keeping the spectral character stable.
    void legatoSlideVoice (OrbitalVoice& voice, int newNote,
                           float inharmonicity, float fmRatio) noexcept
    {
        voice.noteNumber    = newNote;
        voice.fundamentalHz = midiToFreq (newNote);
        lastFundamentalHz   = voice.fundamentalHz;
        formantDirty        = true;   // rebuild formant for new fundamental

        const float stiffnessCoeff = inharmonicity * 0.001f;
        constexpr double twoPi = 6.283185307179586;
        const double inverseSampleRate = 1.0 / cachedSampleRate;

        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
        {
            const float partialNumber = static_cast<float> (partialIdx + 1);
            const float inharmonicRatio = std::sqrt (1.0f + stiffnessCoeff
                                                           * partialNumber
                                                           * partialNumber);
            const double partialFreq = static_cast<double> (voice.fundamentalHz
                                                           * partialNumber
                                                           * inharmonicRatio);
            voice.phaseIncrement[partialIdx]   = static_cast<float> (
                partialFreq * twoPi * inverseSampleRate);
            voice.fmPhaseIncrement[partialIdx] = static_cast<float> (
                partialFreq * static_cast<double> (fmRatio) * twoPi * inverseSampleRate);
            // NOTE: voice.phase[partialIdx] and voice.fmPhase[partialIdx] are intentionally
            // left unchanged — continuous phase = smooth legato slide, no click.
        }
    }

    int findFreeVoice()
    {
        // First pass: find an inactive voice
        for (int i = 0; i < kMaxVoices; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // No free voices: steal the oldest (LRU — Least Recently Used)
        int      oldestIndex = 0;
        uint64_t oldestAge   = UINT64_MAX;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestAge)
            {
                oldestAge   = voices[static_cast<size_t> (i)].startTime;
                oldestIndex = i;
            }
        }
        return oldestIndex;
    }


    //==========================================================================
    //  MACRO SYSTEM — Four knobs that reshape the Circling Current
    //==========================================================================

    void applyMacros (float& brightness, float& oddEven, float& morph,
                      float& stereoSpread, float& formantShift,
                      float groupAttackTimes[4], float groupDecayTimes[4]) noexcept
    {
        if (p_macroSpectrum == nullptr) return;

        const float spectrum  = p_macroSpectrum->load();
        const float evolve    = p_macroEvolve->load();
        const float coupling  = p_macroCoupling->load();
        const float space     = p_macroSpace->load();

        // M1 SPECTRUM: dark fundamental -> all harmonics blazing
        // Directly maps to brightness (spectral tilt) and odd/even balance.
        // At 0: only fundamental. At 1: full harmonic series.
        brightness = spectrum;
        oddEven    = 0.3f + spectrum * 0.4f;   // Range 0.3-0.7: subtle odd/even shift

        // M2 EVOLVE: Profile A static -> Profile B morphing + slower envelopes
        // The envelope time multiplier (1x to 5x) makes partials bloom more
        // slowly as Evolve increases — the school of fish spreads apart.
        if (evolve > 0.001f)
        {
            morph = std::max (morph, evolve);
            const float envelopeTimeScale = 1.0f + evolve * 4.0f;   // 1x to 5x slowdown
            for (int groupIdx = 0; groupIdx < 4; ++groupIdx)
            {
                groupAttackTimes[groupIdx] *= envelopeTimeScale;
                groupDecayTimes[groupIdx]  *= envelopeTimeScale;
            }
        }

        // M3 COUPLING: lift formant shift for spectral contrast when coupled
        // 12 semitones (one octave) maximum shift at full coupling.
        if (coupling > 0.001f)
            formantShift = std::max (formantShift, coupling * 12.0f);

        // M4 SPACE: stereo spread — partials orbit wider around the listener
        if (space > 0.001f)
            stereoSpread = std::max (stereoSpread, space);
    }


    //==========================================================================
    //  PHASE MANAGEMENT
    //==========================================================================

    void wrapPhases() noexcept
    {
        // Wrap phase accumulators to prevent loss of double-precision
        // significance. We wrap at 2pi * 10^6 (about 6.28 million radians)
        // rather than at 2pi to avoid wrapping every single sample — the
        // modulo operation has a cost, and wrapping less frequently amortizes it.
        constexpr double twoPi  = 6.283185307179586;
        constexpr double wrapThreshold = twoPi * 1.0e6;

        for (auto& voice : voices)
        {
            if (!voice.active) continue;
            for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
            {
                if (voice.phase[partialIdx] > wrapThreshold)
                    voice.phase[partialIdx] -= twoPi
                        * std::floor (voice.phase[partialIdx] / twoPi);
                if (voice.fmPhase[partialIdx] > wrapThreshold)
                    voice.fmPhase[partialIdx] -= twoPi
                        * std::floor (voice.fmPhase[partialIdx] / twoPi);
            }
        }
    }


    //==========================================================================
    //  SPECTRAL PROFILES — The shapes of the Circling Current
    //
    //  Each profile is a 64-element amplitude array defining the harmonic
    //  signature of a waveform. Two profiles morph continuously via the
    //  orb_morph parameter. These are computed once at program startup
    //  and never change — they are the DNA of each spectral shape.
    //
    //  The profiles follow the Fourier series of their respective waveforms:
    //    Sawtooth: a_k = 1/k          (all harmonics, linear rolloff)
    //    Square:   a_k = 1/k for odds (only odd harmonics)
    //    Triangle: a_k = 1/k^2 for odds (only odd, steeper rolloff)
    //    Bell:     exponential decay with inharmonic ripple
    //    Organ:    Hammond drawbar registration (discrete harmonics)
    //    Glass:    rising then falling spectrum (upper-partial emphasis)
    //    Vocal:    1/f rolloff with formant-like peak near partial 5
    //    Custom:   flat spectrum — a blank canvas for the formant filter
    //==========================================================================

    static std::array<float, kNumPartials> buildSawtooth()
    {
        // Fourier series of a sawtooth wave: a_k = 1/k
        // Rich in all harmonics — the "buzzsaw" of additive synthesis.
        std::array<float, kNumPartials> amplitudes {};
        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
            amplitudes[partialIdx] = 1.0f / static_cast<float> (partialIdx + 1);
        return amplitudes;
    }

    static std::array<float, kNumPartials> buildSquare()
    {
        // Fourier series of a square wave: a_k = 1/k for odd harmonics only.
        // Hollow, clarinet-like timbre. Even harmonics are exactly zero.
        std::array<float, kNumPartials> amplitudes {};
        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
            amplitudes[partialIdx] = ((partialIdx + 1) % 2 == 1)
                                   ? 1.0f / static_cast<float> (partialIdx + 1)
                                   : 0.0f;
        return amplitudes;
    }

    static std::array<float, kNumPartials> buildTriangle()
    {
        // Fourier series of a triangle wave: a_k = 1/k^2 for odd harmonics.
        // The 1/k^2 rolloff (vs sawtooth's 1/k) gives a mellow, flute-like tone.
        std::array<float, kNumPartials> amplitudes {};
        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
        {
            const float partialNumber = static_cast<float> (partialIdx + 1);
            amplitudes[partialIdx] = ((partialIdx + 1) % 2 == 1)
                                   ? 1.0f / (partialNumber * partialNumber)
                                   : 0.0f;
        }
        return amplitudes;
    }

    static std::array<float, kNumPartials> buildBell()
    {
        // Bell spectrum: exponential decay with inharmonic ripple.
        // The 0.12 decay rate gives ~20dB attenuation by partial 20 — similar
        // to a struck bell or tubular chime. The 1.1 ripple frequency creates
        // the characteristic non-uniform spacing of bell partials (related to
        // the Bessel function zeros of a vibrating disc).
        std::array<float, kNumPartials> amplitudes {};
        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
        {
            const float partialNumber = static_cast<float> (partialIdx);
            const float exponentialDecay = std::exp (-0.12f * partialNumber);
            const float inharmonicRipple = 1.0f + 0.25f * fastSin (partialNumber * 1.1f);
            amplitudes[partialIdx] = juce::jlimit (0.0f, 1.0f,
                                                   exponentialDecay * inharmonicRipple);
        }
        return amplitudes;
    }

    // -------------------------------------------------------------------------
    // NOTE: Alternative drawbar registrations considered — current values are final.
    // Rock Hammond registration (8' heavy, 4' strong) defines ORBITAL's organ character.
    // Each element corresponds to one drawbar partial; all un-set partials remain silent.
    // -------------------------------------------------------------------------
    static std::array<float, kNumPartials> buildOrgan()
    {
        // Hammond B3 drawbar registration (rock preset).
        // Drawbar footages map to harmonic numbers:
        //   16'=sub, 8'=fundamental, 5-1/3'=3rd, 4'=2nd, 2-2/3'=3rd,
        //   2'=4th, 1-3/5'=5th, 1-1/3'=6th, 1'=8th
        std::array<float, kNumPartials> amplitudes {};
        amplitudes[0]  = 1.0f;    // 8'    -- fundamental
        amplitudes[1]  = 0.8f;    // 4'    -- 2nd harmonic (one octave up)
        amplitudes[2]  = 0.4f;    // 2-2/3' -- 3rd harmonic (octave + fifth)
        amplitudes[3]  = 0.7f;    // 2'    -- 4th harmonic (two octaves up)
        amplitudes[4]  = 0.3f;    // 1-3/5' -- 5th harmonic (two octaves + major third)
        amplitudes[5]  = 0.2f;    // 1-1/3' -- 6th harmonic (two octaves + fifth)
        amplitudes[7]  = 0.15f;   // 1'    -- 8th harmonic (three octaves up)
        return amplitudes;
    }

    static std::array<float, kNumPartials> buildGlass()
    {
        // Glass spectrum: rising low partials then exponential decay.
        // The initial 0.082 rise rate per partial creates a "scooped fundamental"
        // characteristic of struck glass, where the lowest modes are weak and
        // the mid-range modes dominate. The 0.07 decay rate above partial 8
        // simulates the rapid high-frequency damping of glass resonance.
        std::array<float, kNumPartials> amplitudes {};
        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
        {
            const float partialNumber = static_cast<float> (partialIdx);
            amplitudes[partialIdx] = (partialIdx < 8)
                 ? juce::jlimit (0.0f, 1.0f, 0.05f + 0.082f * partialNumber)
                 : 0.7f * std::exp (-0.07f * (partialNumber - 8.0f));
        }
        return amplitudes;
    }

    static std::array<float, kNumPartials> buildVocal()
    {
        // Vocal spectrum: 1/f rolloff with formant-like resonance peak.
        // The Gaussian boost centered at partial 5 (~1300 Hz at C4) simulates
        // the first formant region of the human voice. The 3.0 sigma width
        // creates a broad, natural-sounding resonance rather than a sharp peak.
        std::array<float, kNumPartials> amplitudes {};
        for (int partialIdx = 0; partialIdx < kNumPartials; ++partialIdx)
        {
            const float partialNumber = static_cast<float> (partialIdx);
            const float baseRolloff   = 1.0f / (1.0f + 0.5f * partialNumber);
            const float formantBoost  = 0.4f * std::exp (-0.5f
                * std::pow ((partialNumber - 4.0f) / 3.0f, 2.0f));
            amplitudes[partialIdx] = juce::jlimit (0.0f, 1.0f, baseRolloff + formantBoost);
        }
        return amplitudes;
    }

    static std::array<float, kNumPartials> buildCustom()
    {
        // Flat spectrum: all partials at equal amplitude.
        // Acts as a blank canvas — the brightness, odd/even, and formant
        // controls do all the shaping. Useful for experimental sound design
        // where you want full spectral control from the parameter knobs.
        std::array<float, kNumPartials> amplitudes {};
        amplitudes.fill (1.0f);
        return amplitudes;
    }

    static std::array<std::array<float, kNumPartials>, 8> buildAllProfiles()
    {
        return { buildSawtooth(), buildSquare(), buildTriangle(), buildBell(),
                 buildOrgan(),    buildGlass(),  buildVocal(),    buildCustom() };
    }

    //-- Static profile data (built once at program startup, shared by all instances)
    inline static const std::array<std::array<float, kNumPartials>, 8> sProfiles
        = buildAllProfiles();


    //==========================================================================
    //  PARAMETER LAYOUT
    //==========================================================================

    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P  = juce::ParameterID;
        using NR = juce::NormalisableRange<float>;

        //-- Spectral profile selection -----------------------------------------
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            P { "orb_profileA", 1 }, "Orbital Profile A",
            juce::StringArray { "Sawtooth","Square","Triangle","Bell",
                                "Organ","Glass","Vocal","Custom" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            P { "orb_profileB", 1 }, "Orbital Profile B",
            juce::StringArray { "Sawtooth","Square","Triangle","Bell",
                                "Organ","Glass","Vocal","Custom" }, 3));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_morph", 1 }, "Orbital Morph",
            NR (0.0f, 1.0f, 0.001f), 0.0f));

        //-- Spectral shaping ---------------------------------------------------
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_brightness", 1 }, "Orbital Brightness",
            NR (0.0f, 1.0f, 0.01f), 0.7f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_oddEven", 1 }, "Orbital Odd/Even",
            NR (0.0f, 1.0f, 0.01f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            P { "orb_formantShape", 1 }, "Orbital Formant",
            juce::StringArray { "Off","A","E","I","O","U" }, 0));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_formantShift", 1 }, "Orbital Formant Shift",
            NR (-24.0f, 24.0f, 0.1f), 0.0f));

        //-- Inharmonicity + FM -------------------------------------------------
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_inharm", 1 }, "Orbital Inharmonicity",
            NR (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_fmIndex", 1 }, "Orbital FM Index",
            NR (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_fmRatio", 1 }, "Orbital FM Ratio",
            NR (0.5f, 8.0f, 0.01f, 0.4f), 2.0f));

        //-- Group envelopes (4 partial bands, each with attack + decay) --------
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_groupAttack1", 1 }, "Orbital Group1 Attack",
            NR (0.001f, 4.0f, 0.001f, 0.4f), 0.01f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_groupDecay1", 1 }, "Orbital Group1 Decay",
            NR (0.01f, 4.0f, 0.001f, 0.4f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_groupAttack2", 1 }, "Orbital Group2 Attack",
            NR (0.001f, 4.0f, 0.001f, 0.4f), 0.01f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_groupDecay2", 1 }, "Orbital Group2 Decay",
            NR (0.01f, 4.0f, 0.001f, 0.4f), 0.4f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_groupAttack3", 1 }, "Orbital Group3 Attack",
            NR (0.001f, 2.0f, 0.001f, 0.4f), 0.005f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_groupDecay3", 1 }, "Orbital Group3 Decay",
            NR (0.01f, 2.0f, 0.001f, 0.4f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_groupAttack4", 1 }, "Orbital Group4 Attack",
            NR (0.001f, 1.0f, 0.001f, 0.4f), 0.002f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_groupDecay4", 1 }, "Orbital Group4 Decay",
            NR (0.01f, 1.0f, 0.001f, 0.4f), 0.2f));

        //-- Post filter --------------------------------------------------------
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_filterCutoff", 1 }, "Orbital Filter Cutoff",
            NR (20.0f, 20000.0f, 0.1f, 0.3f), 20000.0f));
        // D001: filter envelope depth — velocity × envelope level sweeps filter cutoff.
        // Default 0.25: at full velocity and attack peak, adds +1750 Hz above base cutoff.
        // Connects Orbital's group-envelope system to spectral brightness: harder hits
        // open the post-SVF filter, mapping touch dynamics to timbral character.
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_filterEnvDepth", 1 }, "Orbital Filter Env Depth",
            NR (0.0f, 1.0f, 0.01f), 0.25f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_filterReso", 1 }, "Orbital Filter Reso",
            NR (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            P { "orb_filterType", 1 }, "Orbital Filter Type",
            juce::StringArray { "LP","BP","HP" }, 0));

        //-- Output shaping -----------------------------------------------------
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_stereoSpread", 1 }, "Orbital Stereo Spread",
            NR (0.0f, 1.0f, 0.01f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_saturation", 1 }, "Orbital Saturation",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        //-- Global ADSR envelope -----------------------------------------------
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_ampAttack", 1 }, "Orbital Amp Attack",
            NR (0.001f, 8.0f, 0.001f, 0.4f), 0.1f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_ampDecay", 1 }, "Orbital Amp Decay",
            NR (0.05f, 4.0f, 0.001f, 0.4f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_ampSustain", 1 }, "Orbital Amp Sustain",
            NR (0.0f, 1.0f, 0.01f), 0.8f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_ampRelease", 1 }, "Orbital Amp Release",
            NR (0.05f, 8.0f, 0.001f, 0.4f), 1.0f));

        //-- Master volume ------------------------------------------------------
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_volume", 1 }, "Orbital Volume",
            NR (0.0f, 1.0f, 0.01f), 0.8f));

        //-- Voice mode --------------------------------------------------------
        // Round 11D: voice mode for legato playing.
        // 0=Poly (default — always retrigger envelope, full polyphony),
        // 1=Mono (single voice, always retrigger),
        // 2=Legato (single voice, suppress retrigger when gate open — slide pitch).
        // Default Poly preserves existing behavior for all existing presets.
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            P { "orb_voiceMode", 1 }, "Orbital Voice Mode",
            juce::StringArray { "Poly", "Mono", "Legato" }, 0));

        //-- Macros (4 performance knobs) ---------------------------------------
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_macroSpectrum", 1 }, "Orbital Spectrum",
            NR (0.0f, 1.0f, 0.001f), 0.5f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_macroEvolve", 1 }, "Orbital Evolve",
            NR (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_macroCoupling", 1 }, "Orbital Coupling",
            NR (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_macroSpace", 1 }, "Orbital Space",
            NR (0.0f, 1.0f, 0.001f), 0.3f));
    }


    //==========================================================================
    //  GROUP ENVELOPE SUSTAIN FLOORS
    //
    //  Each group decays to a different minimum level, not to zero. This
    //  ensures the fundamental "body" partials (group 0) always stay present
    //  while the highest "shimmer" partials (group 3) can fully silence.
    //  The musical effect: sustained notes retain warmth while losing sparkle.
    //
    //  Group 0 (body,     partials  0-7):  floor 0.5 — always warm
    //  Group 1 (presence, partials  8-15): floor 0.3 — retains definition
    //  Group 2 (air,      partials 16-31): floor 0.1 — fades to breath
    //  Group 3 (shimmer,  partials 32-63): floor 0.0 — can fully silence
    //==========================================================================
    static constexpr float kGroupSustainFloor[4] = { 0.5f, 0.3f, 0.1f, 0.0f };


    //==========================================================================
    //  DSP STATE
    //==========================================================================

    // D006: aftertouch — pressure increases morph position (push toward state B)
    PolyAftertouch aftertouch;

    // D006: mod wheel — CC1 increases spectral morph drift rate (faster animated texture; sensitivity 0.3)
    float modWheelValue = 0.0f;

    //-- Sample rate (cached at prepare()) --------------------------------------
    double cachedSampleRate      = 44100.0;
    float  cachedSampleRateFloat = 44100.0f;

    //-- Voice pool -------------------------------------------------------------
    std::array<OrbitalVoice, kMaxVoices> voices {};
    uint64_t voiceAllocationAge = 0;       // monotonic counter for LRU voice stealing
    int      phaseWrapCounter   = 0;       // counts samples since last phase wrap
    float    fadeOutStepPerSample = 220.5f; // voice-steal crossfade rate (recalculated in prepare)
    float    lastFundamentalHz  = 261.63f;  // last triggered note's frequency (for formant rebuild)
    float    envelopeOutput     = 0.0f;     // peak envelope level this block (for coupling output)

    // D005 fix: minimal LFO added — spectral morph drift at 0.03 Hz
    // Replaced inline double-precision radians accumulator with shared BreathingLFO.
    BreathingLFO spectralDriftLFO;

    //-- Spectral state ---------------------------------------------------------
    FormantFilter formantFilter;
    bool  formantDirty       = true;
    int   lastVowelIndex     = -1;
    float lastBrightness     = -1.0f;
    float lastOddEven        = -1.0f;
    float lastFormantShift   = -999.0f;
    std::array<float, kNumPartials> spectralCouplingOffset {};

    //-- Post-processing (Cytomic SVF + tube saturator) -------------------------
    CytomicSVF postFilterL, postFilterR;
    Saturator  saturatorL,  saturatorR;

    //-- Coupling output cache (per-sample, for tight coupling reads) -----------
    std::vector<float> outputCacheL, outputCacheR;

    //-- Coupling input buffers -------------------------------------------------
    // Channel 0 = AudioToWavetable (spectral DNA), Channel 1 = AudioToFM
    juce::AudioBuffer<float> couplingAudioBuffer;
    juce::AudioBuffer<float> couplingRingBuffer;
    bool  hasAudioCoupling  = false;
    bool  hasRingCoupling   = false;

    float pitchBendNorm = 0.0f;  // MIDI pitch wheel [-1, +1]; ±2 semitone range

    //-- Block-level coupling accumulators (reset at top of each renderBlock) ----
    float externalFilterMod = 0.0f;   // Hz offset from AmpToFilter coupling
    float externalMorphMod  = 0.0f;   // morph position offset from EnvToMorph
    float externalPitchMod  = 0.0f;   // semitone offset from LFOToPitch/PitchToPitch
    float externalFmMod     = 0.0f;   // FM depth from AudioToFM coupling
    float externalDecayMod  = 0.0f;   // decay damping from EnvToDecay
    float externalBlendMod  = 0.0f;   // blend offset from RhythmToBlend

    //-- Parameter pointers (32 total, set in attachParameters) -----------------
    std::atomic<float>* p_profileA        = nullptr;
    std::atomic<float>* p_profileB        = nullptr;
    std::atomic<float>* p_morph           = nullptr;
    std::atomic<float>* p_brightness      = nullptr;
    std::atomic<float>* p_oddEven         = nullptr;
    std::atomic<float>* p_formantShape    = nullptr;
    std::atomic<float>* p_formantShift    = nullptr;
    std::atomic<float>* p_inharmonicity   = nullptr;
    std::atomic<float>* p_fmIndex         = nullptr;
    std::atomic<float>* p_fmRatio         = nullptr;
    std::atomic<float>* p_groupAttack1    = nullptr;
    std::atomic<float>* p_groupDecay1     = nullptr;
    std::atomic<float>* p_groupAttack2    = nullptr;
    std::atomic<float>* p_groupDecay2     = nullptr;
    std::atomic<float>* p_groupAttack3    = nullptr;
    std::atomic<float>* p_groupDecay3     = nullptr;
    std::atomic<float>* p_groupAttack4    = nullptr;
    std::atomic<float>* p_groupDecay4     = nullptr;
    std::atomic<float>* p_filterCutoff    = nullptr;
    std::atomic<float>* p_filterResonance = nullptr;
    std::atomic<float>* p_filterType      = nullptr;
    std::atomic<float>* p_filterEnvDepth  = nullptr;   // D001: filter env depth (orb_filterEnvDepth)
    std::atomic<float>* p_stereoSpread    = nullptr;
    std::atomic<float>* p_saturation      = nullptr;
    std::atomic<float>* p_ampAttack       = nullptr;
    std::atomic<float>* p_ampDecay        = nullptr;
    std::atomic<float>* p_ampSustain      = nullptr;
    std::atomic<float>* p_ampRelease      = nullptr;
    std::atomic<float>* p_volume          = nullptr;
    std::atomic<float>* p_macroSpectrum   = nullptr;
    std::atomic<float>* p_macroEvolve     = nullptr;
    std::atomic<float>* p_macroCoupling   = nullptr;
    std::atomic<float>* p_macroSpace      = nullptr;
    // Round 11D: voice mode (0=Poly, 1=Mono, 2=Legato)
    std::atomic<float>* p_voiceMode       = nullptr;
};

} // namespace xoceanus
