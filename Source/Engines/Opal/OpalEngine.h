#pragma once
#include "../../Core/SynthEngine.h"
#include "OpalDSP.h"

#include <juce_audio_basics/juce_audio_basics.h>

namespace xomnibus {

//==============================================================================
// OpalEngine — XOpal granular synthesis engine for XOmnibus
//
// Wraps XOpal's granular DSP (ring buffer, grain pool, cloud voices) behind
// the SynthEngine interface. Primary coupling: AudioToWavetable — any engine's
// audio written into OPAL's grain buffer becomes the grain source.
//
// Signal chain:
//   OSC / AudioToWavetable → GrainBuffer (4s ring)
//   MIDI → CloudPool (12) → GrainPool (32) → Cloud Mix
//   Cloud Mix → SVFilter → Shimmer/Frost → Amp → Reverb → Out
//
// Coupling inputs:
//   AudioToWavetable → writes source audio into grain buffer (the core coupling)
//   AmpToFilter      → offsets filter cutoff (drum hits open the filter)
//   EnvToMorph       → offsets grain position (envelope shapes scan position)
//   LFOToPitch       → offsets pitch scatter (cross-engine scatter modulation)
//
// Coupling output:
//   getSampleForCoupling() → post-filter cloud stereo, feeds DUB / MORPH / BITE
//
class OpalEngine : public SynthEngine
{
public:
    OpalEngine()  = default;
    ~OpalEngine() = default;

    //-- Identity ---------------------------------------------------------------

    juce::String getEngineId()     const override { return "Opal"; }
    juce::Colour getAccentColour() const override { return juce::Colour(0xFFA78BFA); } // Iridescent Lavender
    int          getMaxVoices()    const override { return XOpal::MAX_CLOUD_COUNT; }

    //-- Lifecycle --------------------------------------------------------------

    void prepare(double sampleRate, int maxBlockSize) override
    {
        sr = sampleRate;

        grainBuffer.clear();
        grainPool.stopAll();
        cloudPool.prepare(sampleRate);
        oscBank.prepare(sampleRate);
        postFilter.reset();

        // Pre-allocate work buffers — no audio-thread allocation
        grainWorkBuf.setSize   (2, maxBlockSize, false, true, false);
        reverbWorkBuf.setSize  (2, maxBlockSize, false, true, false);
        couplingInputBuf.setSize(2, maxBlockSize, false, true, false);
        couplingInputBuf.clear();

        reverb.setSampleRate(sampleRate);
        resetReverbParams();
    }

    void releaseResources() override
    {
        cloudPool.stopAll();
        grainPool.stopAll();
    }

    void reset() override
    {
        grainBuffer.clear();
        cloudPool.stopAll();
        grainPool.stopAll();
        postFilter.reset();
        reverb.reset();
        couplingInputBuf.clear();
        externalFilterMod  = 0.0f;
        externalPosMod     = 0.0f;
        externalPitchMod   = 0.0f;
        lastSampleL        = 0.0f;
        lastSampleR        = 0.0f;
    }

    //-- Parameters ------------------------------------------------------------

    static void addParameters(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params)
    {
        XOpal::addParameters(params);
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout() override
    {
        return XOpal::createParameterLayout();
    }

    void attachParameters(juce::AudioProcessorValueTreeState& apvts) override
    {
        namespace ID = XOpal::ParamID;
        p_source        = apvts.getRawParameterValue(ID::SOURCE);
        p_grainSize     = apvts.getRawParameterValue(ID::GRAIN_SIZE);
        p_density       = apvts.getRawParameterValue(ID::DENSITY);
        p_position      = apvts.getRawParameterValue(ID::POSITION);
        p_posScatter    = apvts.getRawParameterValue(ID::POS_SCATTER);
        p_pitchScatter  = apvts.getRawParameterValue(ID::PITCH_SCATTER);
        p_panScatter    = apvts.getRawParameterValue(ID::PAN_SCATTER);
        p_window        = apvts.getRawParameterValue(ID::WINDOW);
        p_freeze        = apvts.getRawParameterValue(ID::FREEZE);
        p_couplingLevel = apvts.getRawParameterValue(ID::COUPLING_LEVEL);
        p_pulseWidth    = apvts.getRawParameterValue(ID::PULSE_WIDTH);
        p_detuneCents   = apvts.getRawParameterValue(ID::DETUNE_CENTS);
        p_filterCutoff  = apvts.getRawParameterValue(ID::FILTER_CUTOFF);
        p_filterReso    = apvts.getRawParameterValue(ID::FILTER_RESO);
        p_filterMode    = apvts.getRawParameterValue(ID::FILTER_MODE);
        p_shimmer       = apvts.getRawParameterValue(ID::SHIMMER);
        p_frost         = apvts.getRawParameterValue(ID::FROST);
        p_smear         = apvts.getRawParameterValue(ID::SMEAR);
        p_reverbMix     = apvts.getRawParameterValue(ID::REVERB_MIX);
        p_ampAttack     = apvts.getRawParameterValue(ID::AMP_ATTACK);
        p_ampDecay      = apvts.getRawParameterValue(ID::AMP_DECAY);
        p_ampSustain    = apvts.getRawParameterValue(ID::AMP_SUSTAIN);
        p_ampRelease    = apvts.getRawParameterValue(ID::AMP_RELEASE);
        p_macroScatter  = apvts.getRawParameterValue(ID::MACRO_SCATTER);
        p_macroDrift    = apvts.getRawParameterValue(ID::MACRO_DRIFT);
        p_macroCoupling = apvts.getRawParameterValue(ID::MACRO_COUPLING);
        p_macroSpace    = apvts.getRawParameterValue(ID::MACRO_SPACE);
        p_masterVolume  = apvts.getRawParameterValue(ID::MASTER_VOLUME);
    }

    //-- Coupling --------------------------------------------------------------

    float getSampleForCoupling(int channel, int /*sampleIndex*/) const override
    {
        return (channel == 0) ? lastSampleL : lastSampleR;
    }

    void applyCouplingInput(CouplingType type,
                            float amount,
                            const float* sourceBuffer,
                            int numSamples) override
    {
        switch (type)
        {
            case CouplingType::AudioToWavetable:
                // Store incoming audio; consumed in renderBlock's per-sample loop
                if (sourceBuffer != nullptr)
                {
                    float* cL = couplingInputBuf.getWritePointer(0);
                    float* cR = couplingInputBuf.getWritePointer(1);
                    for (int n = 0; n < numSamples; ++n)
                        cL[n] = cR[n] = sourceBuffer[n] * amount;
                }
                break;

            case CouplingType::AmpToFilter:
                // Source amplitude opens filter cutoff (±8000 Hz)
                externalFilterMod = amount * 8000.0f;
                break;

            case CouplingType::EnvToMorph:
                // Source envelope scans grain position (0 → moves position forward)
                externalPosMod = amount * 0.3f;
                break;

            case CouplingType::LFOToPitch:
                // Source LFO modulates pitch scatter (±12 semitones)
                externalPitchMod = amount * 12.0f;
                break;

            default:
                break;
        }
    }

    //-- Audio -----------------------------------------------------------------

    void renderBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midi,
                     int numSamples) override
    {
        if (numSamples <= 0) return;

        // Guard against rendering before attachParameters() has been called.
        if (p_source == nullptr)
            return;

        juce::ScopedNoDenormals noDenormals;

        buffer.clear();
        float* outL = buffer.getWritePointer(0);
        float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;

        // --- Read parameters ---
        int   sourceMode    = static_cast<int>(p_source->load());
        float grainSizeMs   = p_grainSize->load();
        float density       = p_density->load();
        float position      = p_position->load() + externalPosMod;
        float posScatter    = p_posScatter->load();
        float pitchScatter  = p_pitchScatter->load() + externalPitchMod;
        float panScatter    = p_panScatter->load();
        int   windowShape   = static_cast<int>(p_window->load());
        float freezeAmt     = p_freeze->load();
        float couplingLevel = p_couplingLevel->load();
        float pulseWidth    = p_pulseWidth->load();
        float detuneCents   = p_detuneCents->load();
        float filterCutoff  = juce::jlimit(20.0f, 20000.0f,
                                           p_filterCutoff->load() + externalFilterMod);
        float filterReso    = p_filterReso->load();
        int   filterMode    = static_cast<int>(p_filterMode->load());
        float shimmerAmt    = p_shimmer->load();
        float frostAmt      = p_frost->load();
        float smearAmt      = p_smear->load();
        float reverbMix     = p_reverbMix->load();
        float ampAttack     = p_ampAttack->load();
        float ampDecay      = p_ampDecay->load();
        float ampSustain    = p_ampSustain->load();
        float ampRelease    = p_ampRelease->load();
        float masterVolume  = p_masterVolume->load();

        // Clamp external mods to valid ranges
        position     = juce::jlimit(0.0f, 1.0f, position);
        pitchScatter = juce::jlimit(0.0f, 24.0f, pitchScatter);

        // --- Apply macros ---
        applyMacros(grainSizeMs, density, posScatter, pitchScatter, panScatter,
                    couplingLevel, freezeAmt, reverbMix, smearAmt);

        // --- Freeze ---
        grainBuffer.setFrozen(freezeAmt > 0.5f);

        // --- Filter ---
        auto fMode = (filterMode == 1) ? XOpal::FilterMode::BandPass
                   : (filterMode == 2) ? XOpal::FilterMode::HighPass
                                       : XOpal::FilterMode::LowPass;
        postFilter.setParams(sr, filterCutoff, filterReso, fMode);

        // --- Build shared grain params ---
        XOpal::GrainParams grainParams;
        grainParams.position      = position;
        grainParams.posScatter    = posScatter;
        grainParams.grainSizeMs   = grainSizeMs;
        grainParams.density       = density;
        grainParams.pitchScatter  = pitchScatter;
        grainParams.panScatter    = panScatter;
        grainParams.windowShape   = windowShape;
        grainParams.peakAmplitude = 1.0f;

        // --- MIDI ---
        for (const auto& msg : midi)
        {
            auto m = msg.getMessage();
            if (m.isNoteOn())
            {
                auto* cloud = cloudPool.allocate();
                cloud->noteOn(m.getNoteNumber(), m.getFloatVelocity(),
                              ampAttack, ampDecay, ampSustain, ampRelease);
            }
            else if (m.isNoteOff())
            {
                // Sustain pedal holds clouds active until pedal released
                if (!sustainPedalDown)
                {
                    auto* cloud = cloudPool.findNote(m.getNoteNumber());
                    if (cloud) cloud->noteOff();
                }
            }
            else if (m.isAllNotesOff() || m.isAllSoundOff())
            {
                cloudPool.stopAll();
                sustainPedalDown = false;
            }
            else if (m.isController() && m.getControllerNumber() == 64)
            {
                bool wasDown = sustainPedalDown;
                sustainPedalDown = (m.getControllerValue() >= 64);
                // On pedal release: trigger release on all active clouds
                if (wasDown && !sustainPedalDown)
                    cloudPool.stopAll();
            }
        }

        // --- Update shared grain params in all active clouds ---
        cloudPool.setSharedParams(grainParams);

        // --- Per-sample: write grain source + tick cloud schedulers ---
        bool isCouplingMode = (sourceMode == 5);
        const float* cInL = couplingInputBuf.getReadPointer(0);
        const float* cInR = couplingInputBuf.getReadPointer(1);

        for (int n = 0; n < numSamples; ++n)
        {
            float srcL, srcR;
            if (isCouplingMode)
            {
                srcL = cInL[n] * couplingLevel;
                srcR = cInR[n] * couplingLevel;
            }
            else
            {
                auto oscMode = static_cast<XOpal::OscSource>(sourceMode);
                oscBank.generateSample(oscMode, 261.63f, pulseWidth, detuneCents, srcL, srcR);
                srcL *= 0.5f;
                srcR *= 0.5f;
            }
            grainBuffer.write(srcL, srcR);

            cloudPool.tickOneSample(grainBuffer, windows, grainPool);
        }

        // Clear coupling buffer for next block
        couplingInputBuf.clear();
        externalFilterMod = 0.0f;
        externalPosMod    = 0.0f;
        externalPitchMod  = 0.0f;

        // --- Render all active grains into work buffer ---
        grainWorkBuf.clear();
        float* gL = grainWorkBuf.getWritePointer(0);
        float* gR = grainWorkBuf.getWritePointer(1);
        grainPool.processAll(grainBuffer, windows, gL, gR, numSamples);

        // --- Copy grain output to main buffer ---
        for (int n = 0; n < numSamples; ++n)
        {
            outL[n] = gL[n];
            outR[n] = gR[n];
        }

        // --- Post-cloud filter ---
        for (int n = 0; n < numSamples; ++n)
            postFilter.process(outL[n], outR[n]);

        // --- Character stages ---
        if (shimmerAmt > 0.001f || frostAmt > 0.001f)
        {
            for (int n = 0; n < numSamples; ++n)
            {
                outL[n] = applyShimmer(outL[n], shimmerAmt);
                outR[n] = applyShimmer(outR[n], shimmerAmt);
                outL[n] = applyFrost(outL[n], frostAmt);
                outR[n] = applyFrost(outR[n], frostAmt);
            }
        }

        // --- Reverb ---
        if (reverbMix > 0.001f && buffer.getNumChannels() >= 2)
        {
            reverbWorkBuf.copyFrom(0, 0, outL, numSamples);
            reverbWorkBuf.copyFrom(1, 0, outR, numSamples);
            reverb.processStereo(reverbWorkBuf.getWritePointer(0),
                                 reverbWorkBuf.getWritePointer(1),
                                 numSamples);
            for (int n = 0; n < numSamples; ++n)
            {
                outL[n] = outL[n] * (1.0f - reverbMix) + reverbWorkBuf.getSample(0, n) * reverbMix;
                outR[n] = outR[n] * (1.0f - reverbMix) + reverbWorkBuf.getSample(1, n) * reverbMix;
            }
        }

        // --- Master volume ---
        for (int n = 0; n < numSamples; ++n)
        {
            outL[n] *= masterVolume;
            outR[n] *= masterVolume;
        }

        // --- Cache last sample for coupling output ---
        if (numSamples > 0)
        {
            lastSampleL = outL[numSamples - 1];
            lastSampleR = outR[numSamples - 1];
        }
    }

private:
    //-- DSP members -----------------------------------------------------------

    double sr = 44100.0;

    XOpal::GrainBuffer                       grainBuffer;
    XOpal::WindowFunctions                   windows;
    XOpal::GrainPool<XOpal::MAX_GRAIN_COUNT> grainPool;
    XOpal::CloudPool                         cloudPool;
    XOpal::OscillatorBank                    oscBank;
    XOpal::SVFilterStereo                    postFilter;
    juce::Reverb                             reverb;

    // Pre-allocated work buffers (no audio-thread allocation)
    juce::AudioBuffer<float> grainWorkBuf;
    juce::AudioBuffer<float> reverbWorkBuf;
    juce::AudioBuffer<float> couplingInputBuf;

    // CC64 sustain pedal
    bool sustainPedalDown = false;

    // Coupling modulation accumulators (reset each block)
    float externalFilterMod = 0.0f;  // Hz offset on filter cutoff
    float externalPosMod    = 0.0f;  // position offset [0..1]
    float externalPitchMod  = 0.0f;  // semitone offset on pitch scatter

    // Cached coupling output
    float lastSampleL = 0.0f;
    float lastSampleR = 0.0f;

    //-- Parameter pointers ---------------------------------------------------

    std::atomic<float>* p_source        = nullptr;
    std::atomic<float>* p_grainSize     = nullptr;
    std::atomic<float>* p_density       = nullptr;
    std::atomic<float>* p_position      = nullptr;
    std::atomic<float>* p_posScatter    = nullptr;
    std::atomic<float>* p_pitchScatter  = nullptr;
    std::atomic<float>* p_panScatter    = nullptr;
    std::atomic<float>* p_window        = nullptr;
    std::atomic<float>* p_freeze        = nullptr;
    std::atomic<float>* p_couplingLevel = nullptr;
    std::atomic<float>* p_pulseWidth    = nullptr;
    std::atomic<float>* p_detuneCents   = nullptr;
    std::atomic<float>* p_filterCutoff  = nullptr;
    std::atomic<float>* p_filterReso    = nullptr;
    std::atomic<float>* p_filterMode    = nullptr;
    std::atomic<float>* p_shimmer       = nullptr;
    std::atomic<float>* p_frost         = nullptr;
    std::atomic<float>* p_smear         = nullptr;
    std::atomic<float>* p_reverbMix     = nullptr;
    std::atomic<float>* p_ampAttack     = nullptr;
    std::atomic<float>* p_ampDecay      = nullptr;
    std::atomic<float>* p_ampSustain    = nullptr;
    std::atomic<float>* p_ampRelease    = nullptr;
    std::atomic<float>* p_macroScatter  = nullptr;
    std::atomic<float>* p_macroDrift    = nullptr;
    std::atomic<float>* p_macroCoupling = nullptr;
    std::atomic<float>* p_macroSpace    = nullptr;
    std::atomic<float>* p_masterVolume  = nullptr;

    //-- Helpers ---------------------------------------------------------------

    void resetReverbParams() noexcept
    {
        juce::Reverb::Parameters rp;
        rp.roomSize   = 0.8f;
        rp.damping    = 0.5f;
        rp.wetLevel   = 1.0f;
        rp.dryLevel   = 0.0f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters(rp);
    }

    void applyMacros(float& grainSize, float& density,
                     float& posScatter, float& pitchScatter, float& panScatter,
                     float& couplingLevel, float& freeze,
                     float& reverbMix, float& smear) noexcept
    {
        float scatter  = p_macroScatter->load();
        float drift    = p_macroDrift->load();
        float coupling = p_macroCoupling->load();
        float space    = p_macroSpace->load();

        // M1 SCATTER: small grains + dense @ 0, large grains + sparse @ 1
        if (scatter > 0.001f)
        {
            grainSize = 10.0f + scatter * 790.0f;
            density   = 120.0f - scatter * 110.0f;
        }

        // M2 DRIFT: 0 = focused, 1 = dissolved
        if (drift > 0.001f)
        {
            posScatter   = std::max(posScatter,   drift * 0.8f);
            pitchScatter = std::max(pitchScatter, drift * 20.0f);
            panScatter   = std::max(panScatter,   drift);
        }

        // M3 COUPLING: coupling level + freeze above 0.5
        if (coupling > 0.001f)
        {
            couplingLevel = std::max(couplingLevel, coupling);
            float extraFreeze = (coupling - 0.5f) * 2.0f;
            if (extraFreeze > 0.0f)
                freeze = std::max(freeze, extraFreeze);
        }

        // M4 SPACE: reverb + smear
        if (space > 0.001f)
        {
            reverbMix = std::max(reverbMix, space * 0.8f);
            smear     = std::max(smear,     space * 0.6f);
        }
    }

    inline float applyShimmer(float x, float amount) noexcept
    {
        float threshold = 1.0f - amount * 0.7f;
        if (std::abs(x) <= threshold) return x;
        float excess = x - threshold * (x > 0 ? 1.0f : -1.0f);
        return threshold * (x > 0 ? 1.0f : -1.0f)
             + std::tanh(excess * 2.0f) * (1.0f - threshold) * amount;
    }

    inline float applyFrost(float x, float amount) noexcept
    {
        float ceiling = 1.0f - amount * 0.5f;
        if (std::abs(x) <= ceiling) return x;
        return ceiling * (x > 0 ? 1.0f : -1.0f);
    }
};

} // namespace xomnibus
