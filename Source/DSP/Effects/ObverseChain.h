// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "FXChainHelpers.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// ObverseChain — OBVERSE Reverse Gravity FX Chain (4 stages)
//
// Source concept: Reverse Gravity (Broken Rule Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 2 Reverse Reverb)
// Accent: Anti-Gravity Violet #6A0DAD
//
// Stage 1: Extreme FET Compression (Origin Effects Cali76) — EnvelopeFollower,
//          0.1ms attack, 20:1 ratio, hard knee. Flattens transient completely,
//          creating a constant-level input for the reverse reverb.
// Stage 2: Gated Reverse Reverb (Yamaha SPX90) — CircularBuffer (500ms), read
//          backward with exponential quiet→loud envelope. Mono → Stereo here
//          (L/R use slightly offset read positions for decorrelation).
// Stage 3: Envelope Filter / Auto-Wah (Mutron III) — EnvelopeFollower reads
//          Stage 2 output → CytomicSVF BandPass cutoff modulation. Backward
//          swell triggers the "vowel" AFTER you play. Direction param: Up/Down.
// Stage 4: Optical Tremolo (Spaceman Voyager I) — StandardLFO triangle driving
//          gain, ParameterSmoother for photo-cell smoothness.
//
// BROKEN RULE: Envelope filter AFTER reverb — the filter reacts to the
// reverse swell, not the original transient. Backwards causality as sound.
//
// Parameter prefix: obvr_ (9 params)
//==============================================================================
class ObverseChain
{
public:
    ObverseChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process mono input, writing stereo to L and R.
    // Caller must ensure L and R are separate output buffers.
    void processBlock(const float* monoIn, float* L, float* R, int numSamples);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_        = 0.0;  // Sentinel: must be set by prepare() before use
    int    blockSize_ = 512;

    // Working mono buffer
    std::vector<float> monoWork_;

    //==========================================================================
    // Stage 1 — Extreme FET Compression (Origin Effects Cali76)
    // Attack 0.1ms, 20:1 ratio, hard knee, makeup gain.
    //==========================================================================
    struct FETCompressorStage
    {
        EnvelopeFollower follower;
        ParameterSmoother gainSmoother;
        float threshold_ = 0.1f;
        float ratio_     = 20.0f;

        void prepare(double sampleRate)
        {
            follower.prepare(sampleRate);
            follower.setAttack(0.1f);  // 0.1ms — extremely fast, kills transients
            follower.setRelease(60.0f);
            gainSmoother.prepare(static_cast<float>(sampleRate), 0.003f);
            gainSmoother.snapTo(1.0f);
        }
        void reset()
        {
            follower.reset();
            gainSmoother.snapTo(1.0f);
        }

        // squash [0,1]: 0=no compression, 1=full 20:1 compression
        // makeup: linear gain after compression
        float process(float in, float squash, float makeup)
        {
            threshold_ = 0.05f + (1.0f - squash) * 0.45f; // harder squash → lower threshold
            const float env = follower.process(in);
            float gain = 1.0f;
            if (env > threshold_)
            {
                // Hard knee: gain = threshold + (env - threshold) / ratio
                float compressed = threshold_ + (env - threshold_) / ratio_;
                gain = compressed / std::max(env, 1e-6f);
            }
            gainSmoother.set(gain * makeup);
            return in * gainSmoother.process();
        }
    } comp_;

    //==========================================================================
    // Stage 2 — Gated Reverse Reverb (Yamaha SPX90)
    // CircularBuffer 500ms, backward read with exponential quiet→loud envelope.
    // Mono → Stereo: L reads slightly earlier in the buffer than R.
    //==========================================================================
    struct ReverseReverbStage
    {
        static constexpr float kSwellMaxMs  = 500.0f;
        static constexpr float kDecorOffMs  = 8.0f; // L/R decorrelation offset

        CircularBuffer capL, capR;
        float swellPhase_  = 0.0f; // 0→1 over swellTime samples
        float swellInc_    = 0.0f;
        int   swellLen_    = 0;
        double sr_         = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr_ = sampleRate;
            const int maxSmp = static_cast<int>(kSwellMaxMs * sampleRate / 1000.0) + 8;
            capL.prepare(maxSmp);
            capR.prepare(maxSmp);
            swellLen_  = static_cast<int>(kSwellMaxMs * sampleRate / 1000.0);
            swellInc_  = 1.0f / static_cast<float>(std::max(1, swellLen_));
            reset();
        }
        void reset()
        {
            capL.clear();
            capR.clear();
            swellPhase_ = 0.0f;
        }

        // swellTime: [50ms, 500ms] — length of reverse swell
        // mix: wet amount [0,1]
        void processStereo(const float* in, float* outL, float* outR, int numSamples,
                           float swellTimeMs, float mix)
        {
            const float srF        = static_cast<float>(sr_);
            const int   swellSmp   = std::max(4, static_cast<int>(swellTimeMs * srF / 1000.0f));
            const float inc        = 1.0f / static_cast<float>(swellSmp);
            const int   decorOff   = std::max(0, static_cast<int>(kDecorOffMs * srF / 1000.0f));
            const int   capSize    = capL.getSize();

            for (int i = 0; i < numSamples; ++i)
            {
                capL.write(in[i]);
                capR.write(in[i]);

                // Advance swell phase
                swellPhase_ += inc;
                if (swellPhase_ > 1.0f) swellPhase_ -= 1.0f;

                // Exponential quiet→loud: env = exp(-5 * (1 - phase)) (rising)
                const float env = fastExp(-5.0f * (1.0f - swellPhase_));

                // Read backward: oldest end of swell window
                const int readOffset = static_cast<int>((1.0f - swellPhase_) * static_cast<float>(swellSmp));
                const int readOffL   = std::max(0, std::min(readOffset,         capSize - 1));
                const int readOffR   = std::max(0, std::min(readOffset + decorOff, capSize - 1));

                const float wetL = capL.readBackward(readOffL) * env;
                const float wetR = capR.readBackward(readOffR) * env;

                outL[i] = in[i] * (1.0f - mix) + wetL * mix;
                outR[i] = in[i] * (1.0f - mix) + wetR * mix;
            }
        }
    } revReverb_;

    //==========================================================================
    // Stage 3 — Envelope Filter / Auto-Wah (Mutron III)
    // EnvelopeFollower reads Stage 2 (reverb swell) output.
    // CytomicSVF BandPass. Direction: 0=Up (rising filter), 1=Down (falling).
    //==========================================================================
    struct EnvFilterStage
    {
        EnvelopeFollower envL, envR;
        CytomicSVF       svfL, svfR;
        ParameterSmoother freqSmL, freqSmR;

        void prepare(double sampleRate)
        {
            envL.prepare(sampleRate);
            envR.prepare(sampleRate);
            envL.setAttack(3.0f);
            envL.setRelease(120.0f);
            envR.setAttack(3.0f);
            envR.setRelease(120.0f);
            svfL.setMode(CytomicSVF::Mode::BandPass);
            svfR.setMode(CytomicSVF::Mode::BandPass);
            freqSmL.prepare(static_cast<float>(sampleRate), 0.005f);
            freqSmR.prepare(static_cast<float>(sampleRate), 0.005f);
            freqSmL.snapTo(300.0f);
            freqSmR.snapTo(300.0f);
        }
        void reset()
        {
            envL.reset();
            envR.reset();
            svfL.reset();
            svfR.reset();
            freqSmL.snapTo(300.0f);
            freqSmR.snapTo(300.0f);
        }

        // sens: envelope sensitivity [0,1]
        // peak: resonance/peak [0.2, 1.0]
        // dir: 0=Up, 1=Down
        void processStereo(float* L, float* R, int numSamples,
                           float sens, float peak, bool dirDown, double sampleRate)
        {
            const float srF    = static_cast<float>(sampleRate);
            const float minHz  = 80.0f;
            const float maxHz  = 5000.0f;
            const float range  = maxHz - minHz;
            const float res    = juce::jlimit(0.0f, 0.95f, peak);

            for (int i = 0; i < numSamples; ++i)
            {
                const float lvlL = envL.process(L[i]);
                const float lvlR = envR.process(R[i]);

                float normL = juce::jlimit(0.0f, 1.0f, lvlL * sens * 8.0f);
                float normR = juce::jlimit(0.0f, 1.0f, lvlR * sens * 8.0f);
                if (dirDown) { normL = 1.0f - normL; normR = 1.0f - normR; }

                freqSmL.set(minHz + normL * range);
                freqSmR.set(minHz + normR * range);
                const float fL = flushDenormal(freqSmL.process());
                const float fR = flushDenormal(freqSmR.process());

                svfL.setCoefficients(fL, res, srF);
                svfR.setCoefficients(fR, res, srF);
                L[i] = svfL.processSample(L[i]);
                R[i] = svfR.processSample(R[i]);
            }
        }
    } envFilter_;

    //==========================================================================
    // Stage 4 — Optical Tremolo (Spaceman Voyager I)
    // StandardLFO triangle driving gain. ParameterSmoother for photo-cell lag.
    //==========================================================================
    struct TremoloStage
    {
        StandardLFO lfoL, lfoR;
        ParameterSmoother gainSmL, gainSmR;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            lfoL.setShape(StandardLFO::Triangle);
            lfoR.setShape(StandardLFO::Triangle);
            lfoL.setPhaseOffset(0.0f);
            lfoR.setPhaseOffset(0.0f);
            // Photo-cell smoothing time ~8ms
            gainSmL.prepare(static_cast<float>(sampleRate), 0.008f);
            gainSmR.prepare(static_cast<float>(sampleRate), 0.008f);
            gainSmL.snapTo(1.0f);
            gainSmR.snapTo(1.0f);
        }
        void reset()
        {
            lfoL.reset();
            lfoR.reset();
            gainSmL.snapTo(1.0f);
            gainSmR.snapTo(1.0f);
        }

        void processStereo(float* L, float* R, int numSamples,
                           float rateHz, float depth)
        {
            const float srF = static_cast<float>(sr);
            lfoL.setRate(rateHz, srF);
            lfoR.setRate(rateHz, srF);

            const float d = juce::jlimit(0.0f, 1.0f, depth);

            for (int i = 0; i < numSamples; ++i)
            {
                // LFO output is [-1,+1]; map to gain [1-d, 1]
                const float modL = lfoL.process(); // [-1,+1]
                const float modR = lfoR.process();
                gainSmL.set(1.0f - d * (1.0f - (modL * 0.5f + 0.5f)));
                gainSmR.set(1.0f - d * (1.0f - (modR * 0.5f + 0.5f)));
                L[i] *= gainSmL.process();
                R[i] *= gainSmR.process();
            }
        }
    } tremolo_;

    //==========================================================================
    // Cached parameter pointers (9 params)
    //==========================================================================
    std::atomic<float>* p_compSquash    = nullptr;
    std::atomic<float>* p_compMakeup   = nullptr;
    std::atomic<float>* p_revSwellTime = nullptr;
    std::atomic<float>* p_revMix       = nullptr;
    std::atomic<float>* p_mutronSens   = nullptr;
    std::atomic<float>* p_mutronPeak   = nullptr;
    std::atomic<float>* p_mutronDir    = nullptr;
    std::atomic<float>* p_tremRate     = nullptr;
    std::atomic<float>* p_tremDepth    = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void ObverseChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    monoWork_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    comp_.prepare(sampleRate);
    revReverb_.prepare(sampleRate);
    envFilter_.prepare(sampleRate);
    tremolo_.prepare(sampleRate);
}

inline void ObverseChain::reset()
{
    comp_.reset();
    revReverb_.reset();
    envFilter_.reset();
    tremolo_.reset();
}

inline void ObverseChain::processBlock(const float* monoIn, float* L, float* R, int numSamples)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_compSquash) return;

    // ParamSnapshot
    const float compSquash   = p_compSquash->load(std::memory_order_relaxed);
    const float compMakeup   = p_compMakeup->load(std::memory_order_relaxed);
    const float revSwellTime = p_revSwellTime->load(std::memory_order_relaxed);
    const float revMix       = p_revMix->load(std::memory_order_relaxed);
    const float mutronSens   = p_mutronSens->load(std::memory_order_relaxed);
    const float mutronPeak   = p_mutronPeak->load(std::memory_order_relaxed);
    const bool  mutronDir    = p_mutronDir->load(std::memory_order_relaxed) >= 0.5f;
    const float tremRate     = p_tremRate->load(std::memory_order_relaxed);
    const float tremDepth    = p_tremDepth->load(std::memory_order_relaxed);

    // Linear makeup gain from dB
    const float makeupGain = fastPow2(compMakeup / 6.0205999f);

    // Stage 1 — FET Compression → mono into monoWork_
    for (int i = 0; i < numSamples; ++i)
        monoWork_[i] = comp_.process(monoIn[i], compSquash, makeupGain);

    // Stage 2 — Reverse Reverb (mono→stereo split)
    revReverb_.processStereo(monoWork_.data(), L, R, numSamples,
                             revSwellTime, revMix * 0.01f);

    // Stage 3 — Envelope Filter (reads swell output; vowel AFTER the note)
    envFilter_.processStereo(L, R, numSamples, mutronSens, mutronPeak, mutronDir, sr_);

    // Stage 4 — Optical Tremolo
    tremolo_.processStereo(L, R, numSamples, tremRate, tremDepth);
}

inline void ObverseChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "obvr_";

    registerFloat      (layout, p + "compSquash",   "Comp Squash",    0.0f,  1.0f,   0.7f, 0.001f);
    registerFloat      (layout, p + "compMakeup",   "Comp Makeup",    0.0f, 24.0f,   6.0f, 0.1f);
    registerFloatSkewed(layout, p + "revSwellTime", "Rev Swell Time", 50.0f, 500.0f, 250.0f, 1.0f, 0.4f);
    registerFloat      (layout, p + "revMix",       "Rev Mix",        0.0f, 100.0f,  70.0f, 0.1f);
    registerFloat      (layout, p + "mutronSens",   "Mutron Sens",    0.0f,   1.0f,   0.5f, 0.001f);
    registerFloat      (layout, p + "mutronPeak",   "Mutron Peak",    0.0f,   1.0f,   0.6f, 0.001f);
    registerFloat      (layout, p + "mutronDir",    "Mutron Dir",     0.0f,   1.0f,   0.0f, 1.0f); // 0=Up, 1=Down
    registerFloatSkewed(layout, p + "tremRate",     "Trem Rate",      0.05f,  15.0f,  4.0f, 0.01f, 0.4f);
    registerFloat      (layout, p + "tremDepth",    "Trem Depth",     0.0f,   1.0f,   0.5f, 0.001f);
}

inline void ObverseChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "obvr_";
    p_compSquash   = cacheParam(apvts, p + "compSquash");
    p_compMakeup   = cacheParam(apvts, p + "compMakeup");
    p_revSwellTime = cacheParam(apvts, p + "revSwellTime");
    p_revMix       = cacheParam(apvts, p + "revMix");
    p_mutronSens   = cacheParam(apvts, p + "mutronSens");
    p_mutronPeak   = cacheParam(apvts, p + "mutronPeak");
    p_mutronDir    = cacheParam(apvts, p + "mutronDir");
    p_tremRate     = cacheParam(apvts, p + "tremRate");
    p_tremDepth    = cacheParam(apvts, p + "tremDepth");
}

} // namespace xoceanus
