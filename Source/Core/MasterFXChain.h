#pragma once
#include "../DSP/Effects/Saturator.h"
#include "../DSP/Effects/Corroder.h"
#include "../DSP/Effects/MasterDelay.h"
#include "../DSP/Effects/Combulator.h"
#include "../DSP/Effects/LushReverb.h"
#include "../DSP/Effects/FrequencyShifter.h"
#include "../DSP/Effects/MasterModulation.h"
#include "../DSP/Effects/MultibandCompressor.h"
#include "../DSP/Effects/Compressor.h"
#include "MasterFXSequencer.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace xomnibus {

//==============================================================================
// MasterFXChain — Post-mix master effects for XOmnibus.
//
// Signal chain (10 stages, fixed order):
//   1.  Saturation     — Tape/Tube/Digital/FoldBack via Saturator (mode select)
//   2.  Corroder       — Digital erosion: bitcrush + SR reduce + FM distortion
//   3.  Stereo Delay   — Spatial echo with diffusion, autoclear, tape feedback
//   4.  Combulator     — Tuned comb bank with noise exciter (pitched resonance)
//   5.  Space Reverb   — Algorithmic reverb (Schroeder-Moorer)
//   6.  Freq Shifter   — Hilbert transform frequency shifter (metallic/alien)
//   7.  Modulation FX  — Chorus/Flanger/Ensemble/Drift
//   8.  Multiband OTT  — 3-band upward+downward compression
//   9.  Bus Compressor — Single-band output glue (parallel blend)
//   10. Sequenced Mod  — Rhythmic parameter animation (non-audio)
//
// All stages bypass at zero CPU when their mix/depth/enable = 0.
// Sequencer modulates stages 1-9 parameters rhythmically.
//==============================================================================
class MasterFXChain
{
public:
    MasterFXChain() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate, int samplesPerBlock,
                  juce::AudioProcessorValueTreeState& apvts)
    {
        sr = sampleRate;
        blockSize = samplesPerBlock;

        // Stage 1: Saturator
        saturator.setMode (Saturator::SaturationMode::Tape);
        saturator.setOutputGain (0.85f);
        saturator.reset();

        // Stage 2: Corroder
        corroder.prepare (sampleRate);

        // Stage 3: Delay
        delay.prepare (sampleRate, samplesPerBlock);

        // Stage 4: Combulator
        combulator.prepare (sampleRate);

        // Stage 5: Reverb
        reverb.prepare (sampleRate);
        reverb.setWidth (1.0f);

        // Stage 6: Frequency Shifter
        freqShifter.prepare (sampleRate);

        // Stage 7: Modulation
        modulation.prepare (sampleRate, samplesPerBlock);

        // Stage 8: Multiband OTT
        multibandComp.prepare (sampleRate);

        // Stage 9: Bus Compressor
        compressor.prepare (sampleRate);
        compressor.setThreshold (-12.0f);
        compressor.setKnee (6.0f);
        compressor.setMakeupGain (0.0f);

        // Stage 10: Sequencer
        sequencer.prepare (sampleRate);

        // Dry buffer for compressor parallel blend
        dryBuffer.setSize (2, samplesPerBlock);

        cacheParameterPointers (apvts);
    }

    //--------------------------------------------------------------------------
    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples,
                       double ppqPosition = -1.0, double bpm = 0.0)
    {
        if (!pSatDrive || buffer.getNumChannels() < 2)
            return;

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);

        // ====================================================================
        // ParamSnapshot: read ALL params once per block
        // ====================================================================

        // Stage 1: Saturation
        const float satDrive     = pSatDrive->load();
        const float satMode      = pSatMode      ? pSatMode->load()      : 1.0f;

        // Stage 2: Corroder
        const float corrMix      = pCorrMix      ? pCorrMix->load()      : 0.0f;
        const float corrBits     = pCorrBits     ? pCorrBits->load()     : 24.0f;
        const float corrSR       = pCorrSR       ? pCorrSR->load()       : 44100.0f;
        const float corrFM       = pCorrFM       ? pCorrFM->load()       : 0.0f;
        const float corrTone     = pCorrTone     ? pCorrTone->load()     : 1.0f;

        // Stage 3: Delay
        const float delayTime    = pDelayTime    ? pDelayTime->load()    : 375.0f;
        const float delayFB      = pDelayFB      ? pDelayFB->load()      : 0.3f;
        const float delayMix     = pDelayMix     ? pDelayMix->load()     : 0.0f;
        const float delayPP      = pDelayPP      ? pDelayPP->load()      : 0.5f;
        const float delayDamp    = pDelayDamp    ? pDelayDamp->load()    : 0.3f;
        const float delayDiff    = pDelayDiff    ? pDelayDiff->load()    : 0.0f;
        const float delaySync    = pDelaySync    ? pDelaySync->load()    : 0.0f;

        // Stage 4: Combulator
        const float combMix      = pCombMix      ? pCombMix->load()      : 0.0f;
        const float combFreq     = pCombFreq     ? pCombFreq->load()     : 220.0f;
        const float combFB       = pCombFB       ? pCombFB->load()       : 0.85f;
        const float combDamp     = pCombDamp     ? pCombDamp->load()     : 0.3f;
        const float combNoise    = pCombNoise    ? pCombNoise->load()    : 0.0f;
        const float combSpread   = pCombSpread   ? pCombSpread->load()   : 0.5f;
        const float combOff2     = pCombOff2     ? pCombOff2->load()     : 7.0f;
        const float combOff3     = pCombOff3     ? pCombOff3->load()     : 12.0f;

        // Stage 5: Reverb
        const float reverbSize   = pReverbSize->load();
        const float reverbMix    = pReverbMix->load();

        // Stage 6: Frequency Shifter
        const float fshiftHz     = pFShiftHz     ? pFShiftHz->load()     : 0.0f;
        const float fshiftMix    = pFShiftMix    ? pFShiftMix->load()    : 0.0f;
        const float fshiftMode   = pFShiftMode   ? pFShiftMode->load()   : 0.0f;
        const float fshiftFB     = pFShiftFB     ? pFShiftFB->load()     : 0.0f;

        // Stage 7: Modulation
        const float modRate      = pModRate      ? pModRate->load()      : 0.8f;
        const float modDepth     = pModDepth     ? pModDepth->load()     : 0.0f;
        const float modMix       = pModMix       ? pModMix->load()       : 0.0f;
        const float modMode      = pModMode      ? pModMode->load()      : 0.0f;
        const float modFeedback  = pModFB        ? pModFB->load()        : 0.0f;

        // Stage 8: Multiband OTT
        const float ottMix       = pOttMix       ? pOttMix->load()       : 0.0f;
        const float ottDepth     = pOttDepth     ? pOttDepth->load()     : 0.7f;

        // Stage 9: Bus Compressor
        const float compRatio    = pCompRatio->load();
        const float compAttack   = pCompAttack->load();
        const float compRelease  = pCompRelease->load();
        const float compMix      = pCompMix->load();

        // Stage 10: Sequencer
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
        // Stage 10: Sequencer (non-audio, runs first to compute mod offsets)
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

        // Helper: apply sequencer modulation offset to a parameter
        auto applySeqMod = [&] (float baseValue, MasterFXSequencer::Target paramTarget,
                                float maxOffset) -> float
        {
            float offset = 0.0f;
            if (sequencer.getTarget1() == paramTarget) offset += seqMod1;
            if (sequencer.getTarget2() == paramTarget) offset += seqMod2;
            return clamp (baseValue + offset * maxOffset, 0.0f, 1.0f);
        };

        // ====================================================================
        // Stage 1: Saturation (with mode select)
        // ====================================================================
        float effectiveSatDrive = applySeqMod (satDrive,
            MasterFXSequencer::Target::SatDrive, 0.5f);

        if (effectiveSatDrive > 0.001f)
        {
            // Mode: 0=Tube, 1=Tape, 2=Digital, 3=FoldBack
            auto mode = static_cast<Saturator::SaturationMode> (
                clamp (static_cast<int> (satMode), 0, 3));
            saturator.setMode (mode);
            saturator.setOutputGain (mode == Saturator::SaturationMode::FoldBack ? 0.7f : 0.85f);
            saturator.setDrive (effectiveSatDrive);
            saturator.setMix (effectiveSatDrive);
            saturator.processBlock (L, numSamples);
            saturator.processBlock (R, numSamples);
        }

        // ====================================================================
        // Stage 2: Corroder (digital erosion)
        // ====================================================================
        if (corrMix > 0.001f)
        {
            corroder.setBitDepth (corrBits);
            corroder.setSampleRate (corrSR);
            corroder.setFMDepth (corrFM);
            corroder.setTone (corrTone);
            corroder.setMix (corrMix);
            corroder.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 3: Stereo Delay
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
        // Stage 4: Combulator (tuned comb bank)
        // ====================================================================
        if (combMix > 0.001f)
        {
            combulator.setFrequency (combFreq);
            combulator.setFeedback (combFB);
            combulator.setDamping (combDamp);
            combulator.setNoiseLevel (combNoise);
            combulator.setStereoSpread (combSpread);
            combulator.setComb2Offset (combOff2);
            combulator.setComb3Offset (combOff3);
            combulator.setMix (combMix);
            combulator.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 5: Space Reverb
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
        // Stage 6: Frequency Shifter
        // ====================================================================
        if (fshiftMix > 0.001f)
        {
            freqShifter.setShift (fshiftHz);
            freqShifter.setMix (fshiftMix);
            freqShifter.setMode (static_cast<FrequencyShifter::Mode> (
                clamp (static_cast<int> (fshiftMode), 0, 2)));
            freqShifter.setFeedback (fshiftFB);
            freqShifter.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 7: Modulation FX
        // ====================================================================
        float effectiveModDepth = applySeqMod (modDepth,
            MasterFXSequencer::Target::ModDepth, 0.7f);
        float effectiveModRate = modRate;
        {
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
        // Stage 8: Multiband OTT Compressor
        // ====================================================================
        if (ottMix > 0.001f)
        {
            multibandComp.setDepth (ottDepth);
            multibandComp.setMix (ottMix);
            multibandComp.processBlock (L, R, numSamples);
        }

        // ====================================================================
        // Stage 9: Bus Compressor (with wet/dry blend)
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
    /// Legacy processBlock (no transport info)
    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        processBlock (buffer, numSamples, -1.0, 0.0);
    }

    //--------------------------------------------------------------------------
    float getCompGainReduction() const { return compressor.getGainReduction(); }
    int getSeqCurrentStep() const { return sequencer.getCurrentStep(); }
    bool isSeqEnabled() const { return sequencer.isEnabled(); }
    MasterDelay& getDelay() { return delay; }

    //--------------------------------------------------------------------------
    void reset()
    {
        saturator.reset();
        corroder.reset();
        delay.reset();
        combulator.reset();
        reverb.reset();
        freqShifter.reset();
        modulation.reset();
        multibandComp.reset();
        compressor.reset();
        sequencer.reset();
    }

private:
    void cacheParameterPointers (juce::AudioProcessorValueTreeState& apvts)
    {
        // Stage 1: Saturation
        pSatDrive     = apvts.getRawParameterValue ("master_satDrive");
        pSatMode      = apvts.getRawParameterValue ("master_satMode");

        // Stage 2: Corroder
        pCorrMix      = apvts.getRawParameterValue ("master_corrMix");
        pCorrBits     = apvts.getRawParameterValue ("master_corrBits");
        pCorrSR       = apvts.getRawParameterValue ("master_corrSR");
        pCorrFM       = apvts.getRawParameterValue ("master_corrFM");
        pCorrTone     = apvts.getRawParameterValue ("master_corrTone");

        // Stage 3: Delay
        pDelayTime    = apvts.getRawParameterValue ("master_delayTime");
        pDelayFB      = apvts.getRawParameterValue ("master_delayFeedback");
        pDelayMix     = apvts.getRawParameterValue ("master_delayMix");
        pDelayPP      = apvts.getRawParameterValue ("master_delayPingPong");
        pDelayDamp    = apvts.getRawParameterValue ("master_delayDamping");
        pDelayDiff    = apvts.getRawParameterValue ("master_delayDiffusion");
        pDelaySync    = apvts.getRawParameterValue ("master_delaySync");

        // Stage 4: Combulator
        pCombMix      = apvts.getRawParameterValue ("master_combMix");
        pCombFreq     = apvts.getRawParameterValue ("master_combFreq");
        pCombFB       = apvts.getRawParameterValue ("master_combFeedback");
        pCombDamp     = apvts.getRawParameterValue ("master_combDamping");
        pCombNoise    = apvts.getRawParameterValue ("master_combNoise");
        pCombSpread   = apvts.getRawParameterValue ("master_combSpread");
        pCombOff2     = apvts.getRawParameterValue ("master_combOffset2");
        pCombOff3     = apvts.getRawParameterValue ("master_combOffset3");

        // Stage 5: Reverb
        pReverbSize   = apvts.getRawParameterValue ("master_reverbSize");
        pReverbMix    = apvts.getRawParameterValue ("master_reverbMix");

        // Stage 6: Frequency Shifter
        pFShiftHz     = apvts.getRawParameterValue ("master_fshiftHz");
        pFShiftMix    = apvts.getRawParameterValue ("master_fshiftMix");
        pFShiftMode   = apvts.getRawParameterValue ("master_fshiftMode");
        pFShiftFB     = apvts.getRawParameterValue ("master_fshiftFeedback");

        // Stage 7: Modulation
        pModRate      = apvts.getRawParameterValue ("master_modRate");
        pModDepth     = apvts.getRawParameterValue ("master_modDepth");
        pModMix       = apvts.getRawParameterValue ("master_modMix");
        pModMode      = apvts.getRawParameterValue ("master_modMode");
        pModFB        = apvts.getRawParameterValue ("master_modFeedback");

        // Stage 8: Multiband OTT
        pOttMix       = apvts.getRawParameterValue ("master_ottMix");
        pOttDepth     = apvts.getRawParameterValue ("master_ottDepth");

        // Stage 9: Bus Compressor
        pCompRatio    = apvts.getRawParameterValue ("master_compRatio");
        pCompAttack   = apvts.getRawParameterValue ("master_compAttack");
        pCompRelease  = apvts.getRawParameterValue ("master_compRelease");
        pCompMix      = apvts.getRawParameterValue ("master_compMix");

        // Stage 10: Sequencer
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
    Saturator            saturator;
    Corroder             corroder;
    MasterDelay          delay;
    Combulator           combulator;
    LushReverb           reverb;
    FrequencyShifter     freqShifter;
    MasterModulation     modulation;
    MultibandCompressor  multibandComp;
    Compressor           compressor;
    MasterFXSequencer    sequencer;

    juce::AudioBuffer<float> dryBuffer;
    double sr = 44100.0;
    int blockSize = 512;

    // Cached APVTS raw pointers (null until prepare() called)
    // Stage 1: Saturation
    std::atomic<float>* pSatDrive    = nullptr;
    std::atomic<float>* pSatMode     = nullptr;
    // Stage 2: Corroder
    std::atomic<float>* pCorrMix     = nullptr;
    std::atomic<float>* pCorrBits    = nullptr;
    std::atomic<float>* pCorrSR      = nullptr;
    std::atomic<float>* pCorrFM      = nullptr;
    std::atomic<float>* pCorrTone    = nullptr;
    // Stage 3: Delay
    std::atomic<float>* pDelayTime   = nullptr;
    std::atomic<float>* pDelayFB     = nullptr;
    std::atomic<float>* pDelayMix    = nullptr;
    std::atomic<float>* pDelayPP     = nullptr;
    std::atomic<float>* pDelayDamp   = nullptr;
    std::atomic<float>* pDelayDiff   = nullptr;
    std::atomic<float>* pDelaySync   = nullptr;
    // Stage 4: Combulator
    std::atomic<float>* pCombMix     = nullptr;
    std::atomic<float>* pCombFreq    = nullptr;
    std::atomic<float>* pCombFB      = nullptr;
    std::atomic<float>* pCombDamp    = nullptr;
    std::atomic<float>* pCombNoise   = nullptr;
    std::atomic<float>* pCombSpread  = nullptr;
    std::atomic<float>* pCombOff2    = nullptr;
    std::atomic<float>* pCombOff3    = nullptr;
    // Stage 5: Reverb
    std::atomic<float>* pReverbSize  = nullptr;
    std::atomic<float>* pReverbMix   = nullptr;
    // Stage 6: Frequency Shifter
    std::atomic<float>* pFShiftHz    = nullptr;
    std::atomic<float>* pFShiftMix   = nullptr;
    std::atomic<float>* pFShiftMode  = nullptr;
    std::atomic<float>* pFShiftFB    = nullptr;
    // Stage 7: Modulation
    std::atomic<float>* pModRate     = nullptr;
    std::atomic<float>* pModDepth    = nullptr;
    std::atomic<float>* pModMix      = nullptr;
    std::atomic<float>* pModMode     = nullptr;
    std::atomic<float>* pModFB       = nullptr;
    // Stage 8: Multiband OTT
    std::atomic<float>* pOttMix      = nullptr;
    std::atomic<float>* pOttDepth    = nullptr;
    // Stage 9: Bus Compressor
    std::atomic<float>* pCompRatio   = nullptr;
    std::atomic<float>* pCompAttack  = nullptr;
    std::atomic<float>* pCompRelease = nullptr;
    std::atomic<float>* pCompMix     = nullptr;
    // Stage 10: Sequencer
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
