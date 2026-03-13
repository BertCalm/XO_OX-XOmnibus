#pragma once
#include "../../Core/SynthEngine.h"
#include "../../DSP/FastMath.h"
#include "../../DSP/CytomicSVF.h"
#include "../../DSP/Effects/Saturator.h"
#include <array>
#include <cmath>
#include <vector>

namespace xomnibus {

//==============================================================================
// FormantFilter — 64-point spectral envelope, applied as per-partial amplitude
// multipliers. Modes: tilt+odd/even (vowelIndex=0) or vowel formant (1-5).
// Rebuilt only when parameters change (dirty flag). No per-sample cost.
//==============================================================================
struct FormantFilter
{
    std::array<float, 64> envelope {};

    void build (int vowelIndex, float fundamentalHz, float brightness,
                float oddEven, float shiftSemitones) noexcept
    {
        const float shiftedFundHz = fundamentalHz
                                  * std::pow (2.0f, shiftSemitones / 12.0f);

        if (vowelIndex == 0)
        {
            // Spectral tilt + odd/even balance
            for (int k = 0; k < 64; ++k)
            {
                float kf  = static_cast<float> (k);
                float tilt = std::pow (1.0f / (kf + 1.0f), (1.0f - brightness) * 2.0f);
                float oe   = 1.0f;
                const bool isOdd = ((k + 1) % 2 == 1);
                if (oddEven < 0.5f && !isOdd)
                    oe = oddEven * 2.0f;                // fade even partials
                else if (oddEven > 0.5f && isOdd && k > 0)
                    oe = 2.0f - (oddEven - 0.5f) * 2.0f; // fade odd (keep fundamental)
                oe  = juce::jlimit (0.0f, 1.0f, oe);
                envelope[k] = tilt * oe;
            }
        }
        else
        {
            // Vowel formant: Gaussian peaks at F1, F2, F3
            static const float kFormants[5][3] = {
                { 730.0f, 1090.0f, 2440.0f },   // A
                { 530.0f, 1840.0f, 2480.0f },   // E
                { 270.0f, 2290.0f, 3010.0f },   // I
                { 570.0f,  840.0f, 2410.0f },   // O
                { 300.0f,  870.0f, 2240.0f },   // U
            };
            static const float kBW[3]     = { 120.0f, 150.0f, 200.0f };
            static const float kPeakAmp[3] = { 1.0f, 0.7f, 0.4f };

            const int vi = juce::jlimit (0, 4, vowelIndex - 1);
            const float* fn = kFormants[vi];

            for (int k = 0; k < 64; ++k)
            {
                float freq = static_cast<float> (k + 1) * shiftedFundHz;
                float gain = 0.0f;
                for (int f = 0; f < 3; ++f)
                {
                    float d = (freq - fn[f]) / kBW[f];
                    gain   += kPeakAmp[f] * std::exp (-0.5f * d * d);
                }
                envelope[k] = juce::jlimit (0.0f, 1.0f, gain);
            }
        }
    }

    float apply (int k, float amp) const noexcept { return amp * envelope[k]; }
};

//==============================================================================
// OrbitalVoice — per-voice state for a 64-partial additive synthesis voice.
//==============================================================================
struct OrbitalVoice
{
    bool     active     = false;
    int      noteNumber = -1;
    float    velocity   = 0.0f;
    uint64_t startTime  = 0;
    float    fundamentalHz = 261.63f;

    // Phase accumulators — double precision prevents drift at low frequencies
    double phase[64]    = {};
    double fmPhase[64]  = {};

    // Phase increments cached at noteOn (inharmonic ratios pre-baked in)
    float phaseIncF[64]  = {};
    float fmPhaseInc[64] = {};

    // Per-partial stereo pan (constant-power, cached at noteOn)
    float panL[64] = {};
    float panR[64] = {};

    // Group envelopes (0=partials 0-7, 1=8-15, 2=16-31, 3=32-63)
    float groupEnvLevel[4]     = {};
    float groupAttackCoeff[4]  = {};
    float groupDecayCoeff[4]   = {};
    enum class GroupEnvStage { Attack, Decay, Sustain, Off };
    GroupEnvStage groupStage[4] = {
        GroupEnvStage::Off, GroupEnvStage::Off,
        GroupEnvStage::Off, GroupEnvStage::Off
    };

    // Global ADSR
    float envLevel       = 0.0f;
    float envAttackCoeff = 0.0f;
    float envDecayCoeff  = 0.0f;
    float envSustain     = 0.8f;
    float envReleaseCoeff = 0.0f;
    enum class EnvStage { Attack, Decay, Sustain, Release, Off };
    EnvStage envStage = EnvStage::Off;

    float fadeOutLevel = 0.0f;  // voice-steal crossfade

    // MPE per-voice expression state
    MPEVoiceExpression mpeExpression;
};

//==============================================================================
// OrbitalEngine — 64-partial additive synthesis engine for XOmnibus.
//
// Architecture:
//   64 sine partials per voice × 6 voices = 384 max simultaneous partials.
//   Spectral profile morph (A↔B), formant filter, 4-group envelopes,
//   optional per-partial FM, post-SVF filter, tube saturator.
//
// Coupling:
//   In:  AudioToWavetable (spectral DNA transfer), AmpToFilter, EnvToMorph,
//        AudioToFM, AudioToRing, LFOToPitch, PitchToPitch, EnvToDecay,
//        RhythmToBlend
//   Out: post-filter stereo via outputCacheL/R (per-sample, tight coupling)
//==============================================================================
class OrbitalEngine : public SynthEngine
{
public:
    static constexpr int kMaxVoices  = 6;
    static constexpr int kNumPartials = 64;
    static constexpr float kInvPartialCount = 1.0f / static_cast<float> (kNumPartials - 1);

    //-- Identity ---------------------------------------------------------------
    juce::String getEngineId()     const override { return "Orbital"; }
    juce::Colour getAccentColour() const override { return juce::Colour (0xFFFF6B6B); }
    int          getMaxVoices()    const override { return kMaxVoices; }

    //-- Lifecycle --------------------------------------------------------------
    void prepare (double sampleRate, int maxBlockSize) override
    {
        sr  = sampleRate;
        srf = static_cast<float> (sampleRate);

        for (auto& v : voices)
        {
            v.active = false;
            std::fill (std::begin (v.phase),   std::end (v.phase),   0.0);
            std::fill (std::begin (v.fmPhase), std::end (v.fmPhase), 0.0);
        }

        fadeOutStep = 1.0f / (0.005f * srf);   // 5 ms crossfade step rate

        outputCacheL.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        outputCacheR.assign (static_cast<size_t> (maxBlockSize), 0.0f);

        couplingAudioBuf.setSize (2, maxBlockSize, false, true, false);
        couplingRingBuf .setSize (2, maxBlockSize, false, true, false);

        postFilterL.reset();
        postFilterR.reset();
        saturatorL.reset();
        saturatorR.reset();
        saturatorL.setMode (Saturator::SaturationMode::Tube);
        saturatorR.setMode (Saturator::SaturationMode::Tube);

        spectralCouplingOffset.fill (0.0f);
        formantDirty   = true;
        sampleCounter  = 0;
        envelopeOutput = 0.0f;
        voiceCounter   = 0;
    }

    void releaseResources() override
    {
        for (auto& v : voices) { v.active = false; v.envStage = OrbitalVoice::EnvStage::Off; }
    }

    void reset() override
    {
        for (auto& v : voices)
        {
            v.active   = false;
            v.envStage = OrbitalVoice::EnvStage::Off;
            v.envLevel = 0.0f;
            for (int g = 0; g < 4; ++g)
            {
                v.groupEnvLevel[g] = 0.0f;
                v.groupStage[g]    = OrbitalVoice::GroupEnvStage::Off;
            }
            std::fill (std::begin (v.phase),   std::end (v.phase),   0.0);
            std::fill (std::begin (v.fmPhase), std::end (v.fmPhase), 0.0);
        }
        postFilterL.reset();
        postFilterR.reset();
        saturatorL.reset();
        saturatorR.reset();
        couplingAudioBuf.clear();
        couplingRingBuf .clear();
        spectralCouplingOffset.fill (0.0f);
        hasAudioCoupling = false;
        hasRingCoupling  = false;
        externalFilterMod = externalMorphMod = externalPitchMod = 0.0f;
        externalFmMod = externalDecayMod = externalBlendMod = 0.0f;
        envelopeOutput = 0.0f;
        sampleCounter  = 0;
        formantDirty   = true;
        std::fill (outputCacheL.begin(), outputCacheL.end(), 0.0f);
        std::fill (outputCacheR.begin(), outputCacheR.end(), 0.0f);
    }

    //-- Parameters ------------------------------------------------------------
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
        p_profileA     = apvts.getRawParameterValue ("orb_profileA");
        p_profileB     = apvts.getRawParameterValue ("orb_profileB");
        p_morph        = apvts.getRawParameterValue ("orb_morph");
        p_brightness   = apvts.getRawParameterValue ("orb_brightness");
        p_oddEven      = apvts.getRawParameterValue ("orb_oddEven");
        p_formantShape = apvts.getRawParameterValue ("orb_formantShape");
        p_formantShift = apvts.getRawParameterValue ("orb_formantShift");
        p_inharm       = apvts.getRawParameterValue ("orb_inharm");
        p_fmIndex      = apvts.getRawParameterValue ("orb_fmIndex");
        p_fmRatio      = apvts.getRawParameterValue ("orb_fmRatio");
        p_grpAtk1      = apvts.getRawParameterValue ("orb_groupAttack1");
        p_grpDec1      = apvts.getRawParameterValue ("orb_groupDecay1");
        p_grpAtk2      = apvts.getRawParameterValue ("orb_groupAttack2");
        p_grpDec2      = apvts.getRawParameterValue ("orb_groupDecay2");
        p_grpAtk3      = apvts.getRawParameterValue ("orb_groupAttack3");
        p_grpDec3      = apvts.getRawParameterValue ("orb_groupDecay3");
        p_grpAtk4      = apvts.getRawParameterValue ("orb_groupAttack4");
        p_grpDec4      = apvts.getRawParameterValue ("orb_groupDecay4");
        p_filterCutoff = apvts.getRawParameterValue ("orb_filterCutoff");
        p_filterReso   = apvts.getRawParameterValue ("orb_filterReso");
        p_filterType   = apvts.getRawParameterValue ("orb_filterType");
        p_stereoSpread = apvts.getRawParameterValue ("orb_stereoSpread");
        p_saturation   = apvts.getRawParameterValue ("orb_saturation");
        p_ampAttack    = apvts.getRawParameterValue ("orb_ampAttack");
        p_ampDecay     = apvts.getRawParameterValue ("orb_ampDecay");
        p_ampSustain   = apvts.getRawParameterValue ("orb_ampSustain");
        p_ampRelease   = apvts.getRawParameterValue ("orb_ampRelease");
        p_volume       = apvts.getRawParameterValue ("orb_volume");
        p_macroSpectrum = apvts.getRawParameterValue ("orb_macroSpectrum");
        p_macroEvolve   = apvts.getRawParameterValue ("orb_macroEvolve");
        p_macroCoupling = apvts.getRawParameterValue ("orb_macroCoupling");
        p_macroSpace    = apvts.getRawParameterValue ("orb_macroSpace");
    }

    //-- Coupling --------------------------------------------------------------
    float getSampleForCoupling (int channel, int sampleIndex) const override
    {
        auto si = static_cast<size_t> (sampleIndex);
        if (channel == 0 && si < outputCacheL.size()) return outputCacheL[si];
        if (channel == 1 && si < outputCacheR.size()) return outputCacheR[si];
        if (channel == 2) return envelopeOutput;
        return 0.0f;
    }

    void applyCouplingInput (CouplingType type, float amount,
                             const float* sourceBuffer, int numSamples) override
    {
        switch (type)
        {
            case CouplingType::AudioToWavetable:
                if (sourceBuffer != nullptr)
                {
                    float* c = couplingAudioBuf.getWritePointer (0);
                    for (int n = 0; n < numSamples; ++n)
                        c[n] = sourceBuffer[n] * amount;
                    hasAudioCoupling = true;
                }
                break;

            case CouplingType::AudioToFM:
                if (sourceBuffer != nullptr)
                {
                    float* c = couplingAudioBuf.getWritePointer (1);
                    for (int n = 0; n < numSamples; ++n)
                        c[n] = sourceBuffer[n] * amount;
                    externalFmMod = amount;
                }
                break;

            case CouplingType::AudioToRing:
                if (sourceBuffer != nullptr)
                {
                    float* c = couplingRingBuf.getWritePointer (0);
                    for (int n = 0; n < numSamples; ++n)
                        c[n] = sourceBuffer[n] * amount;
                    hasRingCoupling = true;
                }
                break;

            case CouplingType::AmpToFilter:
                externalFilterMod = amount * 8000.0f;
                break;

            case CouplingType::EnvToMorph:
                externalMorphMod = amount * 0.5f;
                break;

            case CouplingType::LFOToPitch:
                externalPitchMod += amount * 2.0f;
                break;

            case CouplingType::PitchToPitch:
                externalPitchMod += amount * 12.0f;
                break;

            case CouplingType::EnvToDecay:
                externalDecayMod = juce::jlimit (0.0f, 1.0f, amount);
                break;

            case CouplingType::RhythmToBlend:
                externalBlendMod = juce::jlimit (0.0f, 1.0f, amount);
                break;

            default:
                break;
        }
    }

    //-- Audio -----------------------------------------------------------------
    void renderBlock (juce::AudioBuffer<float>& buffer,
                      juce::MidiBuffer& midi,
                      int numSamples) override
    {
        if (p_profileA == nullptr || numSamples <= 0) return;

        juce::ScopedNoDenormals noDenormals;

        buffer.clear();
        float* outL = buffer.getWritePointer (0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : outL;

        //-- ParamSnapshot -------------------------------------------------------
        const int   profileAIdx   = static_cast<int> (p_profileA->load());
        const int   profileBIdx   = static_cast<int> (p_profileB->load());
        float morphPos      = p_morph->load();
        float brightness    = p_brightness->load();
        float oddEven       = p_oddEven->load();
        const int   vowelIdx      = static_cast<int> (p_formantShape->load());
        float formantShift  = p_formantShift->load();
        const float inharm        = p_inharm->load();
        const float fmIndex       = p_fmIndex->load();
        const float fmRatio       = p_fmRatio->load();
        float grpAtk[4] = { p_grpAtk1->load(), p_grpAtk2->load(),
                            p_grpAtk3->load(), p_grpAtk4->load() };
        float grpDec[4] = { p_grpDec1->load(), p_grpDec2->load(),
                            p_grpDec3->load(), p_grpDec4->load() };
        const float filterCutoff  = p_filterCutoff->load();
        const float filterReso    = p_filterReso->load();
        const int   filterTypeIdx = static_cast<int> (p_filterType->load());
        float stereoSpread  = p_stereoSpread->load();
        const float saturation    = p_saturation->load();
        const float ampAttack     = p_ampAttack->load();
        const float ampDecay      = p_ampDecay->load();
        const float ampSustain    = p_ampSustain->load();
        const float ampRelease    = p_ampRelease->load();
        const float volume        = p_volume->load();

        //-- Apply macros --------------------------------------------------------
        applyMacros (brightness, oddEven, morphPos, stereoSpread,
                     formantShift, grpAtk, grpDec);

        //-- Rebuild formant if dirty --------------------------------------------
        if (formantDirty
            || vowelIdx      != lastVowelIndex
            || std::abs (brightness   - lastBrightness)   > 0.005f
            || std::abs (oddEven      - lastOddEven)       > 0.005f
            || std::abs (formantShift - lastFormantShift)  > 0.25f)
        {
            formantFilter.build (vowelIdx, lastFundamentalHz,
                                 brightness, oddEven, formantShift);
            lastVowelIndex   = vowelIdx;
            lastBrightness   = brightness;
            lastOddEven      = oddEven;
            lastFormantShift = formantShift;
            formantDirty     = false;
        }

        //-- Spectral coupling offset (AudioToWavetable) ------------------------
        if (hasAudioCoupling)
        {
            float rms = 0.0f;
            const float* c = couplingAudioBuf.getReadPointer (0);
            for (int n = 0; n < numSamples; ++n) rms += c[n] * c[n];
            rms = std::sqrt (rms / static_cast<float> (numSamples));
            rms = juce::jlimit (0.0f, 1.0f, rms);
            for (int k = 0; k < kNumPartials; ++k)
            {
                float w = static_cast<float> (k) * kInvPartialCount;
                spectralCouplingOffset[k] = rms * w * 0.3f;   // subtle additive lift on uppers
            }
            couplingAudioBuf.clear();
            hasAudioCoupling = false;
        }
        else
        {
            spectralCouplingOffset.fill (0.0f);
        }

        //-- Consume + reset coupling accumulators ------------------------------
        const float pitchOffset = externalPitchMod;
        const float morphOffset = externalMorphMod;
        const float filterOffset = externalFilterMod;
        const float fmAudioAmt   = externalFmMod;
        externalPitchMod = externalMorphMod = externalFilterMod = 0.0f;
        externalFmMod = externalDecayMod = externalBlendMod = 0.0f;

        const float effectiveMorph  = juce::jlimit (0.0f, 1.0f, morphPos + morphOffset);
        const float effectiveCutoff = juce::jlimit (20.0f, 20000.0f,
                                                    filterCutoff + filterOffset);

        //-- Process MIDI -------------------------------------------------------
        for (const auto& meta : midi)
        {
            auto m = meta.getMessage();
            if (m.isNoteOn())
                triggerVoice (m.getNoteNumber(), m.getFloatVelocity(), m.getChannel(),
                              inharm, fmRatio, stereoSpread,
                              ampAttack, ampDecay, ampSustain, ampRelease,
                              grpAtk, grpDec);
            else if (m.isNoteOff())
                noteOff (m.getNoteNumber(), m.getChannel());
            else if (m.isAllNotesOff() || m.isAllSoundOff())
                for (auto& v : voices) { v.active = false; v.envStage = OrbitalVoice::EnvStage::Off; }
        }

        // --- Update per-voice MPE expression from MPEManager ---
        if (mpeManager != nullptr)
        {
            for (auto& v : voices)
            {
                if (!v.active) continue;
                mpeManager->updateVoiceExpression(v.mpeExpression);
            }
        }

        //-- Cache filter coefficients once per block ---------------------------
        const auto filterMode = (filterTypeIdx == 1) ? CytomicSVF::Mode::BandPass
                              : (filterTypeIdx == 2) ? CytomicSVF::Mode::HighPass
                                                     : CytomicSVF::Mode::LowPass;
        postFilterL.setMode (filterMode);
        postFilterR.setMode (filterMode);
        postFilterL.setCoefficients (effectiveCutoff, filterReso, srf);
        postFilterR.setCoefficients (effectiveCutoff, filterReso, srf);

        const bool satActive = saturation > 0.001f;
        if (satActive)
        {
            saturatorL.setDrive (saturation * 0.5f);
            saturatorL.setMix   (saturation);
            saturatorR.setDrive (saturation * 0.5f);
            saturatorR.setMix   (saturation);
        }

        const bool   fmActive      = fmIndex > 0.001f;
        const bool   extFmActive   = fmAudioAmt > 0.001f;
        const bool   pitchActive   = std::abs (pitchOffset) > 0.01f;
        const float  pitchRatio    = pitchActive
                                   ? fastExp (pitchOffset * 0.0578f) : 1.0f;

        const float* pA = sProfiles[juce::jlimit (0, 7, profileAIdx)].data();
        const float* pB = sProfiles[juce::jlimit (0, 7, profileBIdx)].data();
        const float* fmAudioPtr = extFmActive
                                ? couplingAudioBuf.getReadPointer (1) : nullptr;
        const float* ringPtr    = hasRingCoupling
                                ? couplingRingBuf.getReadPointer (0)  : nullptr;

        //-- Per-sample render loop ---------------------------------------------
        float envelopePeak = 0.0f;

        for (int n = 0; n < numSamples; ++n)
        {
            float mixL = 0.0f, mixR = 0.0f;
            const float extFmSample = extFmActive ? fmAudioPtr[n] * fmAudioAmt : 0.0f;

            for (auto& v : voices)
            {
                if (!v.active) continue;

                //-- Tick global ADSR ------------------------------------------
                switch (v.envStage)
                {
                    case OrbitalVoice::EnvStage::Attack:
                        v.envLevel += v.envAttackCoeff;
                        if (v.envLevel >= 1.0f) { v.envLevel = 1.0f; v.envStage = OrbitalVoice::EnvStage::Decay; }
                        break;
                    case OrbitalVoice::EnvStage::Decay:
                        v.envLevel -= v.envDecayCoeff;
                        v.envLevel = flushDenormal (v.envLevel);
                        if (v.envLevel <= v.envSustain) { v.envLevel = v.envSustain; v.envStage = OrbitalVoice::EnvStage::Sustain; }
                        break;
                    case OrbitalVoice::EnvStage::Sustain:
                        break;
                    case OrbitalVoice::EnvStage::Release:
                        v.envLevel -= v.envReleaseCoeff;
                        v.envLevel = flushDenormal (v.envLevel);
                        if (v.envLevel <= 0.0f)
                        {
                            v.envLevel = 0.0f;
                            v.envStage = OrbitalVoice::EnvStage::Off;
                            v.active   = false;
                            continue;
                        }
                        break;
                    case OrbitalVoice::EnvStage::Off:
                        v.active = false;
                        continue;
                }

                //-- Tick group envelopes (AD, hold at floor) ------------------
                for (int g = 0; g < 4; ++g)
                {
                    switch (v.groupStage[g])
                    {
                        case OrbitalVoice::GroupEnvStage::Attack:
                            v.groupEnvLevel[g] += v.groupAttackCoeff[g];
                            if (v.groupEnvLevel[g] >= 1.0f)
                            {
                                v.groupEnvLevel[g] = 1.0f;
                                v.groupStage[g] = OrbitalVoice::GroupEnvStage::Decay;
                            }
                            break;
                        case OrbitalVoice::GroupEnvStage::Decay:
                            v.groupEnvLevel[g] -= v.groupDecayCoeff[g];
                            v.groupEnvLevel[g] = flushDenormal (v.groupEnvLevel[g]);
                            if (v.groupEnvLevel[g] <= kGroupFloor[g])
                            {
                                v.groupEnvLevel[g] = kGroupFloor[g];
                                v.groupStage[g] = OrbitalVoice::GroupEnvStage::Sustain;
                            }
                            break;
                        case OrbitalVoice::GroupEnvStage::Sustain:
                        case OrbitalVoice::GroupEnvStage::Off:
                            break;
                    }
                    // Apply external decay mod: gently damps group levels
                    if (externalDecayMod > 0.0f)
                        v.groupEnvLevel[g] = std::max (kGroupFloor[g],
                            v.groupEnvLevel[g] * (1.0f - externalDecayMod * 0.001f));
                    v.groupEnvLevel[g] = flushDenormal (v.groupEnvLevel[g]);
                }

                //-- 64-partial synthesis --------------------------------------
                float voiceL = 0.0f, voiceR = 0.0f;
                const float mpePitchRatio = std::pow(2.0f, v.mpeExpression.pitchBendSemitones / 12.0f);

                for (int k = 0; k < kNumPartials; ++k)
                {
                    const int g = (k < 8) ? 0 : (k < 16) ? 1 : (k < 32) ? 2 : 3;
                    float amp = lerp (pA[k], pB[k], effectiveMorph)
                              * formantFilter.envelope[k]
                              * v.groupEnvLevel[g];
                    amp = std::max (0.0f, amp + spectralCouplingOffset[k]);
                    if (amp < 0.001f) continue;

                    if (fmActive || extFmActive)
                    {
                        float fmMod = extFmSample;
                        if (fmActive)
                            fmMod += fmIndex * fastSin (static_cast<float> (v.fmPhase[k]));
                        v.phase[k]   += static_cast<double> (v.phaseIncF[k] * pitchRatio * mpePitchRatio * (1.0f + fmMod));
                        v.fmPhase[k] += static_cast<double> (v.fmPhaseInc[k]);
                    }
                    else
                    {
                        v.phase[k] += static_cast<double> (v.phaseIncF[k] * pitchRatio * mpePitchRatio);
                    }

                    const float s = amp * fastSin (static_cast<float> (v.phase[k]));
                    voiceL += s * v.panL[k];
                    voiceR += s * v.panR[k];
                }

                //-- Voice stealing crossfade ----------------------------------
                float stealFade = 1.0f;
                if (v.fadeOutLevel > 0.0f)
                {
                    v.fadeOutLevel -= fadeOutStep;
                    v.fadeOutLevel = flushDenormal (v.fadeOutLevel);
                    if (v.fadeOutLevel < 0.0f) v.fadeOutLevel = 0.0f;
                    stealFade = 1.0f - v.fadeOutLevel;
                }

                const float vGain = v.envLevel * v.velocity * stealFade;
                mixL += voiceL * vGain;
                mixR += voiceR * vGain;
                envelopePeak = std::max (envelopePeak, v.envLevel);
            }

            //-- Ring modulation coupling --------------------------------------
            if (ringPtr != nullptr)
            {
                mixL *= ringPtr[n];
                mixR *= ringPtr[n];
            }

            //-- Post filter + saturator + volume (all per-sample) -------------
            float sL = postFilterL.processSample (mixL);
            float sR = postFilterR.processSample (mixR);
            if (satActive)
            {
                sL = saturatorL.processSample (sL);
                sR = saturatorR.processSample (sR);
            }
            sL *= volume;
            sR *= volume;

            outL[n] = sL;
            outR[n] = sR;

            if (n < static_cast<int> (outputCacheL.size()))
            {
                outputCacheL[static_cast<size_t> (n)] = sL;
                outputCacheR[static_cast<size_t> (n)] = sR;
            }
        } // end per-sample loop

        envelopeOutput = envelopePeak;

        //-- Phase wrap every 1024 samples (prevents double precision drift) ---
        sampleCounter += numSamples;
        if (sampleCounter >= 1024)
        {
            wrapPhases();
            sampleCounter &= 0x3FF;
        }

        //-- Reset ring coupling for next block --------------------------------
        if (hasRingCoupling)
        {
            couplingRingBuf.clear();
            hasRingCoupling = false;
        }
        if (extFmActive)
            couplingAudioBuf.clear (1, 0, numSamples);
    }

private:
    //-- triggerVoice ----------------------------------------------------------
    void triggerVoice (int note, float vel, int midiChannel,
                       float inharm, float fmRatio, float stereoSpread,
                       float attackS, float decayS, float sustainLvl, float releaseS,
                       const float grpAtk[4], const float grpDec[4])
    {
        const int idx = findFreeVoice();
        auto& v = voices[static_cast<size_t> (idx)];

        v.fadeOutLevel = v.active ? v.envLevel : 0.0f;
        v.active       = true;
        v.noteNumber   = note;
        v.velocity     = vel;
        v.startTime    = voiceCounter++;

        // Initialize MPE expression for this voice's channel
        v.mpeExpression.reset();
        v.mpeExpression.midiChannel = midiChannel;
        if (mpeManager != nullptr)
            mpeManager->updateVoiceExpression(v.mpeExpression);
        v.fundamentalHz = midiToFreq (note);

        lastFundamentalHz = v.fundamentalHz;
        formantDirty      = true;   // rebuild formant for new fundamental

        // Inharmonic frequency ratios (expensive sqrt — done here, not per-sample)
        const float B = inharm * 0.001f;
        constexpr double twoPi = 6.283185307179586;
        const double invSr = 1.0 / sr;

        for (int k = 0; k < kNumPartials; ++k)
        {
            const float kf   = static_cast<float> (k + 1);
            const float ratio = std::sqrt (1.0f + B * kf * kf);
            const double freq  = static_cast<double> (v.fundamentalHz * kf * ratio);
            v.phaseIncF[k]  = static_cast<float> (freq * twoPi * invSr);
            v.fmPhaseInc[k] = static_cast<float> (freq * static_cast<double> (fmRatio) * twoPi * invSr);
            v.phase[k]    = 0.0;
            v.fmPhase[k]  = 0.0;
        }

        // Per-partial stereo pan (constant-power spread)
        for (int k = 0; k < kNumPartials; ++k)
        {
            float spread = stereoSpread * static_cast<float> (k) * kInvPartialCount;
            float pan    = 0.5f + ((k % 2 == 0) ? -spread : spread) * 0.5f;
            pan = juce::jlimit (0.0f, 1.0f, pan);
            v.panL[k] = fastCos (pan * 1.5707963f);
            v.panR[k] = fastSin (pan * 1.5707963f);
        }

        // Global ADSR coefficients
        v.envLevel        = 0.0f;
        v.envStage        = OrbitalVoice::EnvStage::Attack;
        v.envAttackCoeff  = 1.0f / (std::max (0.001f, attackS)  * srf);
        v.envDecayCoeff   = 1.0f / (std::max (0.05f,  decayS)   * srf);
        v.envSustain      = sustainLvl;
        v.envReleaseCoeff = 1.0f / (std::max (0.05f,  releaseS) * srf);

        // Group envelope coefficients
        for (int g = 0; g < 4; ++g)
        {
            v.groupEnvLevel[g]    = 0.0f;
            v.groupStage[g]       = OrbitalVoice::GroupEnvStage::Attack;
            v.groupAttackCoeff[g] = 1.0f / (std::max (0.001f, grpAtk[g]) * srf);
            v.groupDecayCoeff[g]  = 1.0f / (std::max (0.01f,  grpDec[g]) * srf);
        }
    }

    void noteOff (int noteNumber, int midiChannel = 0)
    {
        for (auto& v : voices)
        {
            if (v.active && v.noteNumber == noteNumber
                && v.envStage != OrbitalVoice::EnvStage::Release
                && v.envStage != OrbitalVoice::EnvStage::Off)
            {
                // In MPE mode, match by channel too
                if (midiChannel > 0 && v.mpeExpression.midiChannel > 0
                    && v.mpeExpression.midiChannel != midiChannel)
                    continue;

                v.envStage = OrbitalVoice::EnvStage::Release;
            }
        }
    }

    int findFreeVoice()
    {
        for (int i = 0; i < kMaxVoices; ++i)
            if (!voices[static_cast<size_t> (i)].active)
                return i;

        // LRU steal
        int      oldest     = 0;
        uint64_t oldestTime = UINT64_MAX;
        for (int i = 0; i < kMaxVoices; ++i)
        {
            if (voices[static_cast<size_t> (i)].startTime < oldestTime)
            {
                oldestTime = voices[static_cast<size_t> (i)].startTime;
                oldest     = i;
            }
        }
        return oldest;
    }

    void applyMacros (float& brightness, float& oddEven, float& morph,
                      float& stereoSpread, float& formantShift,
                      float grpAtk[4], float grpDec[4]) noexcept
    {
        if (p_macroSpectrum == nullptr) return;

        const float spectrum  = p_macroSpectrum->load();
        const float evolve    = p_macroEvolve->load();
        const float coupling  = p_macroCoupling->load();
        const float space     = p_macroSpace->load();

        // M1 SPECTRUM: dark fundamental → all harmonics blazing
        brightness = spectrum;
        oddEven    = 0.3f + spectrum * 0.4f;

        // M2 EVOLVE: Profile A static → Profile B morphing + slower groups
        if (evolve > 0.001f)
        {
            morph = std::max (morph, evolve);
            const float envScale = 1.0f + evolve * 4.0f;
            for (int g = 0; g < 4; ++g) { grpAtk[g] *= envScale; grpDec[g] *= envScale; }
        }

        // M3 COUPLING: lift formant shift → spectral contrast when coupled
        if (coupling > 0.001f)
            formantShift = std::max (formantShift, coupling * 12.0f);

        // M4 SPACE: stereo spread
        if (space > 0.001f)
            stereoSpread = std::max (stereoSpread, space);
    }

    void wrapPhases() noexcept
    {
        constexpr double twoPi   = 6.283185307179586;
        constexpr double wrapAt  = twoPi * 1.0e6;
        for (auto& v : voices)
        {
            if (!v.active) continue;
            for (int k = 0; k < kNumPartials; ++k)
            {
                if (v.phase[k] > wrapAt)
                    v.phase[k] -= twoPi * std::floor (v.phase[k] / twoPi);
                if (v.fmPhase[k] > wrapAt)
                    v.fmPhase[k] -= twoPi * std::floor (v.fmPhase[k] / twoPi);
            }
        }
    }

    //-- Spectral profiles — built once at program startup ---------------------
    // Each profile is a 64-element amplitude array. Two profiles morph
    // continuously via the orb_morph parameter.

    static std::array<float, kNumPartials> buildSawtooth()
    {
        std::array<float, kNumPartials> a {};
        for (int k = 0; k < kNumPartials; ++k)
            a[k] = 1.0f / static_cast<float> (k + 1);
        return a;
    }

    static std::array<float, kNumPartials> buildSquare()
    {
        std::array<float, kNumPartials> a {};
        for (int k = 0; k < kNumPartials; ++k)
            a[k] = ((k + 1) % 2 == 1) ? 1.0f / static_cast<float> (k + 1) : 0.0f;
        return a;
    }

    static std::array<float, kNumPartials> buildTriangle()
    {
        std::array<float, kNumPartials> a {};
        for (int k = 0; k < kNumPartials; ++k)
        {
            const float kf = static_cast<float> (k + 1);
            a[k] = ((k + 1) % 2 == 1) ? 1.0f / (kf * kf) : 0.0f;
        }
        return a;
    }

    static std::array<float, kNumPartials> buildBell()
    {
        std::array<float, kNumPartials> a {};
        for (int k = 0; k < kNumPartials; ++k)
        {
            const float kf     = static_cast<float> (k);
            const float decay  = std::exp (-0.12f * kf);
            const float ripple = 1.0f + 0.25f * fastSin (kf * 1.1f);
            a[k] = juce::jlimit (0.0f, 1.0f, decay * ripple);
        }
        return a;
    }

    // -------------------------------------------------------------------------
    // TODO (Tuning Decision): Organ drawbar levels — this defines ORBITAL's
    // organ character across the entire Organ preset family.
    //
    // Current values emulate a rock Hammond registration (8' heavy, 4' strong).
    // Each line is one drawbar partial; all un-set partials stay 0 (silent).
    // Blueprint §3.3 maps drawbar footages → partial indices.
    //
    // Tune a[0]–a[7] to match the organ character you want for XOmnibus:
    //   Jazz Hammond: a[0]=1.0, a[1]=0.4, a[3]=0.8, a[7]=0.6 (sparse, punchy)
    //   Cathedral:    a[0]=0.8, a[2]=0.6, a[3]=0.5, a[5]=0.4, a[7]=0.3 (rich)
    //   Rock Hammond: values below (bright mid, heavy fundamental)
    // -------------------------------------------------------------------------
    static std::array<float, kNumPartials> buildOrgan()
    {
        std::array<float, kNumPartials> a {};
        a[0]  = 1.0f;    // 8'   — fundamental
        a[1]  = 0.8f;    // 4'   — 2nd harmonic
        a[2]  = 0.4f;    // 2⅔' — 3rd harmonic
        a[3]  = 0.7f;    // 2'   — 4th harmonic
        a[4]  = 0.3f;    // 1⅗' — 5th harmonic
        a[5]  = 0.2f;    // 1⅓' — 6th harmonic
        a[7]  = 0.15f;   // 1'   — 8th harmonic
        return a;
    }

    static std::array<float, kNumPartials> buildGlass()
    {
        std::array<float, kNumPartials> a {};
        for (int k = 0; k < kNumPartials; ++k)
        {
            const float kf = static_cast<float> (k);
            a[k] = (k < 8)
                 ? juce::jlimit (0.0f, 1.0f, 0.05f + 0.082f * kf)
                 : 0.7f * std::exp (-0.07f * (kf - 8.0f));
        }
        return a;
    }

    static std::array<float, kNumPartials> buildVocal()
    {
        std::array<float, kNumPartials> a {};
        for (int k = 0; k < kNumPartials; ++k)
        {
            const float kf = static_cast<float> (k);
            // Gentle rolloff with formant-like peak around partial 5
            const float base  = 1.0f / (1.0f + 0.5f * kf);
            const float boost = 0.4f * std::exp (-0.5f * std::pow ((kf - 4.0f) / 3.0f, 2.0f));
            a[k] = juce::jlimit (0.0f, 1.0f, base + boost);
        }
        return a;
    }

    static std::array<float, kNumPartials> buildCustom()
    {
        std::array<float, kNumPartials> a {};
        // Equal amplitude — acts as a blank canvas; brightness/formant controls shape it
        a.fill (1.0f);
        return a;
    }

    static std::array<std::array<float, kNumPartials>, 8> buildAllProfiles()
    {
        return { buildSawtooth(), buildSquare(), buildTriangle(), buildBell(),
                 buildOrgan(),    buildGlass(),  buildVocal(),    buildCustom() };
    }

    //-- Static profile data (built once at program startup) -------------------
    inline static const std::array<std::array<float, kNumPartials>, 8> sProfiles
        = buildAllProfiles();

    //-- Parameter layout ------------------------------------------------------
    static void addParametersImpl (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        using P  = juce::ParameterID;
        using NR = juce::NormalisableRange<float>;

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

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_inharm", 1 }, "Orbital Inharmonicity",
            NR (0.0f, 1.0f, 0.001f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_fmIndex", 1 }, "Orbital FM Index",
            NR (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_fmRatio", 1 }, "Orbital FM Ratio",
            NR (0.5f, 8.0f, 0.01f, 0.4f), 2.0f));

        // Group envelopes
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

        // Post filter
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_filterCutoff", 1 }, "Orbital Filter Cutoff",
            NR (20.0f, 20000.0f, 0.1f, 0.3f), 20000.0f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_filterReso", 1 }, "Orbital Filter Reso",
            NR (0.0f, 1.0f, 0.01f), 0.0f));
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            P { "orb_filterType", 1 }, "Orbital Filter Type",
            juce::StringArray { "LP","BP","HP" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_stereoSpread", 1 }, "Orbital Stereo Spread",
            NR (0.0f, 1.0f, 0.01f), 0.3f));
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_saturation", 1 }, "Orbital Saturation",
            NR (0.0f, 1.0f, 0.01f), 0.0f));

        // Global ADSR
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

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            P { "orb_volume", 1 }, "Orbital Volume",
            NR (0.0f, 1.0f, 0.01f), 0.8f));

        // Macros
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

    //-- Group envelope decay floors (per-group minimum sustain level) ---------
    // Group 0 (body): stays warm. Group 3 (air): can fully silence.
    static constexpr float kGroupFloor[4] = { 0.5f, 0.3f, 0.1f, 0.0f };

    //-- DSP state -------------------------------------------------------------
    double sr  = 44100.0;
    float  srf = 44100.0f;
    std::array<OrbitalVoice, kMaxVoices> voices {};
    uint64_t voiceCounter   = 0;
    int      sampleCounter  = 0;
    float    fadeOutStep    = 220.5f;
    float    lastFundamentalHz = 261.63f;
    float    envelopeOutput    = 0.0f;

    // Spectral state
    FormantFilter formantFilter;
    bool  formantDirty     = true;
    int   lastVowelIndex   = -1;
    float lastBrightness   = -1.0f;
    float lastOddEven      = -1.0f;
    float lastFormantShift = -999.0f;
    std::array<float, kNumPartials> spectralCouplingOffset {};

    // Post-processing
    CytomicSVF postFilterL, postFilterR;
    Saturator  saturatorL,  saturatorR;

    // Coupling output cache (per-sample, for tight coupling reads)
    std::vector<float> outputCacheL, outputCacheR;

    // Coupling input buffers (ch0 = wavetable/audio, ch1 = FM audio)
    juce::AudioBuffer<float> couplingAudioBuf;
    juce::AudioBuffer<float> couplingRingBuf;
    bool  hasAudioCoupling = false;
    bool  hasRingCoupling  = false;

    // Block-level coupling accumulators (all reset at top of renderBlock)
    float externalFilterMod = 0.0f;
    float externalMorphMod  = 0.0f;
    float externalPitchMod  = 0.0f;
    float externalFmMod     = 0.0f;
    float externalDecayMod  = 0.0f;
    float externalBlendMod  = 0.0f;

    // Parameter pointers (35 total, set in attachParameters)
    std::atomic<float>* p_profileA     = nullptr;
    std::atomic<float>* p_profileB     = nullptr;
    std::atomic<float>* p_morph        = nullptr;
    std::atomic<float>* p_brightness   = nullptr;
    std::atomic<float>* p_oddEven      = nullptr;
    std::atomic<float>* p_formantShape = nullptr;
    std::atomic<float>* p_formantShift = nullptr;
    std::atomic<float>* p_inharm       = nullptr;
    std::atomic<float>* p_fmIndex      = nullptr;
    std::atomic<float>* p_fmRatio      = nullptr;
    std::atomic<float>* p_grpAtk1      = nullptr;
    std::atomic<float>* p_grpDec1      = nullptr;
    std::atomic<float>* p_grpAtk2      = nullptr;
    std::atomic<float>* p_grpDec2      = nullptr;
    std::atomic<float>* p_grpAtk3      = nullptr;
    std::atomic<float>* p_grpDec3      = nullptr;
    std::atomic<float>* p_grpAtk4      = nullptr;
    std::atomic<float>* p_grpDec4      = nullptr;
    std::atomic<float>* p_filterCutoff = nullptr;
    std::atomic<float>* p_filterReso   = nullptr;
    std::atomic<float>* p_filterType   = nullptr;
    std::atomic<float>* p_stereoSpread = nullptr;
    std::atomic<float>* p_saturation   = nullptr;
    std::atomic<float>* p_ampAttack    = nullptr;
    std::atomic<float>* p_ampDecay     = nullptr;
    std::atomic<float>* p_ampSustain   = nullptr;
    std::atomic<float>* p_ampRelease   = nullptr;
    std::atomic<float>* p_volume       = nullptr;
    std::atomic<float>* p_macroSpectrum = nullptr;
    std::atomic<float>* p_macroEvolve   = nullptr;
    std::atomic<float>* p_macroCoupling = nullptr;
    std::atomic<float>* p_macroSpace    = nullptr;
};

} // namespace xomnibus
