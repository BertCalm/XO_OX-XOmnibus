#pragma once
#include "../DSP/Effects/Saturator.h"
#include "../DSP/Effects/MasterDelay.h"
#include "../DSP/Effects/LushReverb.h"
#include "../DSP/Effects/MasterModulation.h"
#include "../DSP/Effects/Compressor.h"
#include "MasterFXSequencer.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace xomnibus {

//==============================================================================
// MasterFXChain — Post-mix master effects for XOmnibus.
//
// Signal chain (6 stages, fixed order):
//   1. Tape Saturation  — warmth/glue via Saturator (Tape mode)
//   2. Stereo Delay     — spatial echo via MasterDelay (with diffusion + autoclear)
//   3. Space Reverb     — global ambience via LushReverb
//   4. Modulation FX    — Chorus/Flanger/Ensemble/Drift via MasterModulation
//   5. Bus Compressor   — output glue via Compressor (with wet/dry blend)
//   6. Sequenced Mod    — rhythmic parameter animation via MasterFXSequencer (non-audio)
//
// 26 total parameters, all APVTS-sourced via cached raw pointers.
// Sequencer modulates stages 1-5 parameters rhythmically.
// All stages bypass at zero CPU when their mix/depth/enable = 0.
//
// Design note: parameters are read via raw pointers cached in prepare().
// No APVTS lookups inside processBlock — one read per block per param.
//==============================================================================
class MasterFXChain
{
public:
    MasterFXChain() = default;

    //--------------------------------------------------------------------------
    /// Prepare all processors and cache APVTS parameter pointers.
    void prepare (double sampleRate, int samplesPerBlock,
                  juce::AudioProcessorValueTreeState& apvts)
    {
        sr = sampleRate;
        blockSize = samplesPerBlock;

        // --- Stage 1: Saturator ---
        saturator.setMode (Saturator::SaturationMode::Tape);
        saturator.setOutputGain (0.85f);
        saturator.reset();

        // --- Stage 2: Delay ---
        delay.prepare (sampleRate, samplesPerBlock);

        // --- Stage 3: Reverb ---
        reverb.prepare (sampleRate);
        reverb.setWidth (1.0f);

        // --- Stage 4: Modulation ---
        modulation.prepare (sampleRate, samplesPerBlock);

        // --- Stage 5: Compressor ---
        compressor.prepare (sampleRate);
        compressor.setThreshold (-12.0f);
        compressor.setKnee (6.0f);
        compressor.setMakeupGain (0.0f);

        // --- Stage 6: Sequencer ---
        sequencer.prepare (sampleRate);

        // Pre-allocate dry copy buffer for compressor wet/dry blend
        dryBuffer.setSize (2, samplesPerBlock);

        // Cache all parameter pointers
        cacheParameterPointers (apvts);
    }

    //--------------------------------------------------------------------------
    /// Process the master buffer in-place. Buffer must be stereo (2 channels).
    /// Called from XOmnibusProcessor::processBlock() after all engines mix.
    /// @param ppqPosition  Current PPQ from AudioPlayHead (for sequencer sync)
    /// @param bpm          Current BPM from host transport
    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples,
                       double ppqPosition = -1.0, double bpm = 0.0)
    {
        if (!pSatDrive || buffer.getNumChannels() < 2)
            return;

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);

        // ====================================================================
        // Read ALL params once per block (ParamSnapshot pattern)
        // ====================================================================
        const float satDrive     = pSatDrive->load();

        const float delayTime    = pDelayTime    ? pDelayTime->load()    : 375.0f;
        const float delayFB      = pDelayFB      ? pDelayFB->load()      : 0.3f;
        const float delayMix     = pDelayMix     ? pDelayMix->load()     : 0.0f;
        const float delayPP      = pDelayPP      ? pDelayPP->load()      : 0.5f;
        const float delayDamp    = pDelayDamp    ? pDelayDamp->load()    : 0.3f;
        const float delayDiff    = pDelayDiff    ? pDelayDiff->load()    : 0.0f;
        const float delaySync    = pDelaySync    ? pDelaySync->load()    : 0.0f;

        const float reverbSize   = pReverbSize->load();
        const float reverbMix    = pReverbMix->load();

        const float modRate      = pModRate      ? pModRate->load()      : 0.8f;
        const float modDepth     = pModDepth     ? pModDepth->load()     : 0.0f;
        const float modMix       = pModMix       ? pModMix->load()       : 0.0f;
        const float modMode      = pModMode      ? pModMode->load()      : 0.0f;
        const float modFeedback  = pModFB        ? pModFB->load()        : 0.0f;

        const float compRatio    = pCompRatio->load();
        const float compAttack   = pCompAttack->load();
        const float compRelease  = pCompRelease->load();
        const float compMix      = pCompMix->load();

        const float seqEnabled   = pSeqEnabled   ? pSeqEnabled->load()   : 0.0f;
        const float seqRate      = pSeqRate      ? pSeqRate->load()      : 2.0f;
        const float seqSteps     = pSeqSteps     ? pSeqSteps->load()     : 8.0f;
        const float seqDepth     = pSeqDepth     ? pSeqDepth->load()     : 0.5f;
        const float seqSmooth    = pSeqSmooth    ? pSeqSmooth->load()    : 0.3f;
        const float seqTarget1   = pSeqTarget1   ? pSeqTarget1->load()   : 0.0f;
        const float seqTarget2   = pSeqTarget2   ? pSeqTarget2->load()   : 0.0f;
        const float seqPattern   = pSeqPattern   ? pSeqPattern->load()   : 0.0f;
        const float seqEnvFollow = pSeqEnvFollow ? pSeqEnvFollow->load() : 0.0f;
        const float seqEnvAmt    = pSeqEnvAmt    ? pSeqEnvAmt->load()    : 0.5f;

        // ====================================================================
        // Stage 6: Sequencer (non-audio, runs first to compute mod offsets)
        // ====================================================================
        float seqMod1 = 0.0f;
        float seqMod2 = 0.0f;

        if (seqEnabled > 0.5f)
        {
            sequencer.setEnabled (true);
            sequencer.setPattern (static_cast<MasterFXSequencer::Pattern> (static_cast<int> (seqPattern)));
            sequencer.setSteps (static_cast<int> (seqSteps));
            sequencer.setDepth (seqDepth);
            sequencer.setSmooth (seqSmooth);
            sequencer.setRate (static_cast<MasterFXSequencer::ClockDiv> (static_cast<int> (seqRate)));
            sequencer.setTarget1 (static_cast<MasterFXSequencer::Target> (static_cast<int> (seqTarget1)));
            sequencer.setTarget2 (static_cast<MasterFXSequencer::Target> (static_cast<int> (seqTarget2)));
            sequencer.setEnvFollowEnabled (seqEnvFollow > 0.5f);
            sequencer.setEnvFollowAmount (seqEnvAmt);

            // Compute input RMS for envelope follower
            float rms = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                rms += L[i] * L[i] + R[i] * R[i];
            rms = std::sqrt (rms / static_cast<float> (numSamples * 2));

            sequencer.updateBlock (ppqPosition, bpm, numSamples, rms);
            seqMod1 = sequencer.getModValue1();
            seqMod2 = sequencer.getModValue2();
        }
        else
        {
            sequencer.setEnabled (false);
            sequencer.updateBlock (ppqPosition, bpm, numSamples, 0.0f);
        }

        // Helper: apply sequencer modulation to a parameter
        auto applySeqMod = [&] (float baseValue, MasterFXSequencer::Target paramTarget,
                                float maxOffset) -> float
        {
            float offset = 0.0f;
            if (sequencer.getTarget1() == paramTarget) offset += seqMod1;
            if (sequencer.getTarget2() == paramTarget) offset += seqMod2;
            return clamp (baseValue + offset * maxOffset, 0.0f, 1.0f);
        };

        // ====================================================================
        // Stage 1: Tape Saturation
        // ====================================================================
        float effectiveSatDrive = applySeqMod (satDrive,
            MasterFXSequencer::Target::SatDrive, 0.5f);

        if (effectiveSatDrive > 0.001f)
        {
            saturator.setDrive (effectiveSatDrive);
            saturator.setMix (effectiveSatDrive);
            saturator.processBlock (L, numSamples);
            saturator.processBlock (R, numSamples);
        }

        // ====================================================================
        // Stage 2: Stereo Delay
        // ====================================================================
        float effectiveDelayMix = applySeqMod (delayMix,
            MasterFXSequencer::Target::DelayMix, 0.8f);
        float effectiveDelayFB = applySeqMod (delayFB,
            MasterFXSequencer::Target::DelayFeedback, 0.4f);

        if (effectiveDelayMix > 0.001f)
        {
            delay.setDelayTime (delayTime);
            delay.setFeedback (effectiveDelayFB);
            delay.setMix (effectiveDelayMix);
            delay.setPingPong (delayPP);
            delay.setDamping (delayDamp);
            delay.setDiffusion (delayDiff);
            delay.setSyncDiv (static_cast<MasterDelay::SyncDiv> (static_cast<int> (delaySync)));
            delay.setBPM (bpm);
            delay.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 3: Space Reverb
        // ====================================================================
        float effectiveReverbMix = applySeqMod (reverbMix,
            MasterFXSequencer::Target::ReverbMix, 0.6f);

        if (effectiveReverbMix > 0.001f)
        {
            reverb.setRoomSize (reverbSize);
            reverb.setDamping (0.4f);
            reverb.setMix (effectiveReverbMix);
            reverb.processBlock (L, R, L, R, numSamples);
        }

        // ====================================================================
        // Stage 4: Modulation FX
        // ====================================================================
        float effectiveModDepth = applySeqMod (modDepth,
            MasterFXSequencer::Target::ModDepth, 0.7f);
        float effectiveModRate = modRate;
        {
            // Apply rate modulation if targeted
            float rateOffset = 0.0f;
            if (sequencer.getTarget1() == MasterFXSequencer::Target::ModRate)
                rateOffset += seqMod1;
            if (sequencer.getTarget2() == MasterFXSequencer::Target::ModRate)
                rateOffset += seqMod2;
            effectiveModRate = clamp (modRate + rateOffset * 5.0f, 0.01f, 15.0f);
        }

        if (effectiveModDepth > 0.001f && modMix > 0.001f)
        {
            modulation.setMode (static_cast<MasterModulation::Mode> (static_cast<int> (modMode)));
            modulation.setRate (effectiveModRate);
            modulation.setDepth (effectiveModDepth);
            modulation.setMix (modMix);
            modulation.setFeedback (modFeedback);
            modulation.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 5: Bus Compressor (with wet/dry blend)
        // ====================================================================
        float effectiveCompMix = applySeqMod (compMix,
            MasterFXSequencer::Target::CompMix, 0.5f);

        if (effectiveCompMix > 0.001f)
        {
            compressor.setRatio (compRatio);
            compressor.setAttack (compAttack);
            compressor.setRelease (compRelease);

            if (effectiveCompMix >= 0.999f)
            {
                compressor.processBlock (L, R, numSamples);
            }
            else
            {
                const int safeSamples = juce::jmin (numSamples, dryBuffer.getNumSamples());
                dryBuffer.copyFrom (0, 0, buffer, 0, 0, safeSamples);
                dryBuffer.copyFrom (1, 0, buffer, 1, 0, safeSamples);
                compressor.processBlock (L, R, numSamples);
                buffer.applyGain (effectiveCompMix);
                const float dryGain = 1.0f - effectiveCompMix;
                buffer.addFrom (0, 0, dryBuffer, 0, 0, safeSamples, dryGain);
                buffer.addFrom (1, 0, dryBuffer, 1, 0, safeSamples, dryGain);
            }
        }
    }

    //--------------------------------------------------------------------------
    /// Legacy processBlock for backward compatibility (no transport info)
    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        processBlock (buffer, numSamples, -1.0, 0.0);
    }

    //--------------------------------------------------------------------------
    /// Get current compressor gain reduction in dB (for UI metering).
    float getCompGainReduction() const { return compressor.getGainReduction(); }

    /// Get current sequencer step (for UI display).
    int getSeqCurrentStep() const { return sequencer.getCurrentStep(); }

    /// Is sequencer active?
    bool isSeqEnabled() const { return sequencer.isEnabled(); }

    /// Access delay for autoclear trigger from note-on handler
    MasterDelay& getDelay() { return delay; }

    //--------------------------------------------------------------------------
    /// Reset all processor state.
    void reset()
    {
        saturator.reset();
        delay.reset();
        reverb.reset();
        modulation.reset();
        compressor.reset();
        sequencer.reset();
    }

private:
    void cacheParameterPointers (juce::AudioProcessorValueTreeState& apvts)
    {
        // Stage 1: Saturation
        pSatDrive     = apvts.getRawParameterValue ("master_satDrive");

        // Stage 2: Delay
        pDelayTime    = apvts.getRawParameterValue ("master_delayTime");
        pDelayFB      = apvts.getRawParameterValue ("master_delayFeedback");
        pDelayMix     = apvts.getRawParameterValue ("master_delayMix");
        pDelayPP      = apvts.getRawParameterValue ("master_delayPingPong");
        pDelayDamp    = apvts.getRawParameterValue ("master_delayDamping");
        pDelayDiff    = apvts.getRawParameterValue ("master_delayDiffusion");
        pDelaySync    = apvts.getRawParameterValue ("master_delaySync");

        // Stage 3: Reverb
        pReverbSize   = apvts.getRawParameterValue ("master_reverbSize");
        pReverbMix    = apvts.getRawParameterValue ("master_reverbMix");

        // Stage 4: Modulation
        pModRate      = apvts.getRawParameterValue ("master_modRate");
        pModDepth     = apvts.getRawParameterValue ("master_modDepth");
        pModMix       = apvts.getRawParameterValue ("master_modMix");
        pModMode      = apvts.getRawParameterValue ("master_modMode");
        pModFB        = apvts.getRawParameterValue ("master_modFeedback");

        // Stage 5: Compressor
        pCompRatio    = apvts.getRawParameterValue ("master_compRatio");
        pCompAttack   = apvts.getRawParameterValue ("master_compAttack");
        pCompRelease  = apvts.getRawParameterValue ("master_compRelease");
        pCompMix      = apvts.getRawParameterValue ("master_compMix");

        // Stage 6: Sequencer
        pSeqEnabled   = apvts.getRawParameterValue ("master_seqEnabled");
        pSeqRate      = apvts.getRawParameterValue ("master_seqRate");
        pSeqSteps     = apvts.getRawParameterValue ("master_seqSteps");
        pSeqDepth     = apvts.getRawParameterValue ("master_seqDepth");
        pSeqSmooth    = apvts.getRawParameterValue ("master_seqSmooth");
        pSeqTarget1   = apvts.getRawParameterValue ("master_seqTarget1");
        pSeqTarget2   = apvts.getRawParameterValue ("master_seqTarget2");
        pSeqPattern   = apvts.getRawParameterValue ("master_seqPattern");
        pSeqEnvFollow = apvts.getRawParameterValue ("master_seqEnvFollow");
        pSeqEnvAmt    = apvts.getRawParameterValue ("master_seqEnvAmount");
    }

    //--------------------------------------------------------------------------
    // DSP processors
    Saturator         saturator;
    MasterDelay       delay;
    LushReverb        reverb;
    MasterModulation  modulation;
    Compressor        compressor;
    MasterFXSequencer sequencer;

    juce::AudioBuffer<float> dryBuffer;
    double sr = 44100.0;
    int blockSize = 512;

    // Cached APVTS raw pointers (null until prepare() called)
    // Stage 1
    std::atomic<float>* pSatDrive    = nullptr;
    // Stage 2
    std::atomic<float>* pDelayTime   = nullptr;
    std::atomic<float>* pDelayFB     = nullptr;
    std::atomic<float>* pDelayMix    = nullptr;
    std::atomic<float>* pDelayPP     = nullptr;
    std::atomic<float>* pDelayDamp   = nullptr;
    std::atomic<float>* pDelayDiff   = nullptr;
    std::atomic<float>* pDelaySync   = nullptr;
    // Stage 3
    std::atomic<float>* pReverbSize  = nullptr;
    std::atomic<float>* pReverbMix   = nullptr;
    // Stage 4
    std::atomic<float>* pModRate     = nullptr;
    std::atomic<float>* pModDepth    = nullptr;
    std::atomic<float>* pModMix      = nullptr;
    std::atomic<float>* pModMode     = nullptr;
    std::atomic<float>* pModFB       = nullptr;
    // Stage 5
    std::atomic<float>* pCompRatio   = nullptr;
    std::atomic<float>* pCompAttack  = nullptr;
    std::atomic<float>* pCompRelease = nullptr;
    std::atomic<float>* pCompMix     = nullptr;
    // Stage 6
    std::atomic<float>* pSeqEnabled  = nullptr;
    std::atomic<float>* pSeqRate     = nullptr;
    std::atomic<float>* pSeqSteps    = nullptr;
    std::atomic<float>* pSeqDepth    = nullptr;
    std::atomic<float>* pSeqSmooth   = nullptr;
    std::atomic<float>* pSeqTarget1  = nullptr;
    std::atomic<float>* pSeqTarget2  = nullptr;
    std::atomic<float>* pSeqPattern  = nullptr;
    std::atomic<float>* pSeqEnvFollow = nullptr;
    std::atomic<float>* pSeqEnvAmt   = nullptr;
};

} // namespace xomnibus
