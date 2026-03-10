#pragma once
#include "../DSP/Effects/Saturator.h"
#include "../DSP/Effects/LushReverb.h"
#include "../DSP/Effects/Compressor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace xomnibus {

//==============================================================================
// MasterFXChain — Post-mix master effects for XOmnibus.
//
// Signal chain (fixed order):
//   1. Tape Saturation — warmth/glue via Saturator (Tape mode)
//   2. Space Reverb    — global ambience via LushReverb
//   3. Bus Compressor  — output glue via Compressor (with wet/dry blend)
//
// All 7 parameters are APVTS-sourced:
//   master_satDrive      [0..1]         — saturation amount (0 = bypass)
//   master_reverbSize    [0..1]         — room size
//   master_reverbMix     [0..1]         — reverb wet amount
//   master_compRatio     [1..20]        — compression ratio
//   master_compAttack    [0.1..100 ms]  — attack time
//   master_compRelease   [10..1000 ms]  — release time
//   master_compMix       [0..1]         — compressor wet/dry blend
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
    /// Call this from prepareToPlay() after the APVTS is fully constructed.
    void prepare (double sampleRate, int samplesPerBlock,
                  juce::AudioProcessorValueTreeState& apvts)
    {
        sr = sampleRate;

        saturator.setMode (Saturator::SaturationMode::Tape);
        saturator.setOutputGain (0.85f);  // compensate for tape drive gain
        saturator.reset();

        reverb.prepare (sampleRate);
        reverb.setWidth (1.0f);

        compressor.prepare (sampleRate);
        compressor.setThreshold (-12.0f);
        compressor.setKnee (6.0f);
        compressor.setMakeupGain (0.0f);

        // Pre-allocate dry copy buffer for compressor wet/dry blend
        dryBuffer.setSize (2, samplesPerBlock);

        // Cache parameter pointers — eliminates per-sample APVTS lookups
        pSatDrive     = apvts.getRawParameterValue ("master_satDrive");
        pReverbSize   = apvts.getRawParameterValue ("master_reverbSize");
        pReverbMix    = apvts.getRawParameterValue ("master_reverbMix");
        pCompRatio    = apvts.getRawParameterValue ("master_compRatio");
        pCompAttack   = apvts.getRawParameterValue ("master_compAttack");
        pCompRelease  = apvts.getRawParameterValue ("master_compRelease");
        pCompMix      = apvts.getRawParameterValue ("master_compMix");
    }

    //--------------------------------------------------------------------------
    /// Process the master buffer in-place. Buffer must be stereo (2 channels).
    /// Called from XOmnibusProcessor::processBlock() after all engines mix.
    void processBlock (juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (!pSatDrive || buffer.getNumChannels() < 2)
            return;

        // Read params once per block (ParamSnapshot pattern)
        const float satDrive    = pSatDrive->load();
        const float reverbSize  = pReverbSize->load();
        const float reverbMix   = pReverbMix->load();
        const float compRatio   = pCompRatio->load();
        const float compAttack  = pCompAttack->load();
        const float compRelease = pCompRelease->load();
        const float compMix     = pCompMix->load();

        float* L = buffer.getWritePointer (0);
        float* R = buffer.getWritePointer (1);

        // --- Stage 1: Tape Saturation ---
        if (satDrive > 0.001f)
        {
            saturator.setDrive (satDrive);
            saturator.setMix (satDrive);  // drive knob also controls wet amount
            saturator.processBlock (L, numSamples);
            saturator.processBlock (R, numSamples);
        }

        // --- Stage 2: Space Reverb ---
        if (reverbMix > 0.001f)
        {
            reverb.setRoomSize (reverbSize);
            reverb.setDamping (0.4f);
            reverb.setMix (reverbMix);
            reverb.processBlock (L, R, L, R, numSamples);
        }

        // --- Stage 3: Bus Compressor (with wet/dry blend) ---
        if (compMix > 0.001f)
        {
            compressor.setRatio (compRatio);
            compressor.setAttack (compAttack);
            compressor.setRelease (compRelease);

            if (compMix >= 0.999f)
            {
                // Fully wet — skip blend math
                compressor.processBlock (L, R, numSamples);
            }
            else
            {
                // Parallel compression blend: out = dry*(1-mix) + wet*mix
                // 1) Save dry signal
                dryBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
                dryBuffer.copyFrom (1, 0, buffer, 1, 0, numSamples);
                // 2) Compress in-place (wet path)
                compressor.processBlock (L, R, numSamples);
                // 3) Scale wet by mix
                buffer.applyGain (compMix);
                // 4) Add dry scaled by (1 - mix)
                const float dryGain = 1.0f - compMix;
                buffer.addFrom (0, 0, dryBuffer, 0, 0, numSamples, dryGain);
                buffer.addFrom (1, 0, dryBuffer, 1, 0, numSamples, dryGain);
            }
        }
    }

    //--------------------------------------------------------------------------
    /// Get current compressor gain reduction in dB (for UI metering).
    float getCompGainReduction() const
    {
        return compressor.getGainReduction();
    }

    //--------------------------------------------------------------------------
    /// Reset all processor state (called on releaseResources or preset change).
    void reset()
    {
        saturator.reset();
        reverb.reset();
        compressor.reset();
    }

private:
    Saturator    saturator;
    LushReverb   reverb;
    Compressor   compressor;

    juce::AudioBuffer<float> dryBuffer;
    double sr = 44100.0;

    // Cached APVTS raw pointers (null until prepare() called)
    std::atomic<float>* pSatDrive    = nullptr;
    std::atomic<float>* pReverbSize  = nullptr;
    std::atomic<float>* pReverbMix   = nullptr;
    std::atomic<float>* pCompRatio   = nullptr;
    std::atomic<float>* pCompAttack  = nullptr;
    std::atomic<float>* pCompRelease = nullptr;
    std::atomic<float>* pCompMix     = nullptr;
};

} // namespace xomnibus
