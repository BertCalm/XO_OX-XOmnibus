// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
#include "../PolyBLEP.h"
#include "../ParameterSmoother.h"
#include "../StandardLFO.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OnrushChain — ONRUSH Expressive Lead FX Chain (5 stages)
//
// Source concept: PolySwell (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 5 BBD Delay)
// Accent: Molten Amber #FF6F00
//
// Stage 1: Auto-Swell (Boss SG-1) — one-pole IIR envelope + Schmitt trigger
// Stage 2: Ring Modulator (Fairfield Randy's Revenge) — dual PolyBLEP + fastTanh
// Stage 3: Hard Clip Distortion (Boss DS-1) — soft-knee + 4x OVS + tilt EQ
// Stage 4: Envelope Filter (Boss TW-1) — env follower → fastPow2 freq → CytomicSVF
// Stage 5: Pitch-Sequenced BBD Delay (Chase Bliss Thermae) — Hermite + 2-step seq
//
// Parameter prefix: onr_ (17 params)
//==============================================================================
class OnrushChain
{
public:
    OnrushChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process mono input, writing stereo to L and R.
    // Caller must ensure L != R (separate output buffers).
    void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                      double bpm = 0.0, double ppqPosition = -1.0);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_ = 0.0;  // Sentinel: must be set by prepare() before use
    int    blockSize_ = 512;

    //==========================================================================
    // Stage 1 — Auto-Swell (Boss SG-1)
    //==========================================================================
    struct AutoSwellStage
    {
        float envState    = 0.0f;
        float schmittHi   = 0.0f;
        float schmittLo   = 0.0f;
        bool  swellOpen   = true;
        float attackCoeff  = 0.0f;
        float releaseCoeff = 0.0f;
        ParameterSmoother gainSmoother;

        void prepare(double sampleRate)
        {
            gainSmoother.prepare(static_cast<float>(sampleRate), 0.005f);
            reset();
        }
        void reset()
        {
            envState  = 0.0f;
            swellOpen = true;
            gainSmoother.snapTo(1.0f);
        }
        void setCoeffs(float attackMs, float releaseMs, float threshDb, double sampleRate)
        {
            float sr = static_cast<float>(sampleRate);
            attackCoeff  = fastExp(-1.0f / (attackMs  * 0.001f * sr));
            releaseCoeff = fastExp(-1.0f / (releaseMs * 0.001f * sr));
            float threshLin = fastPow2(threshDb * (1.0f / 6.0205999f));
            schmittHi = threshLin * 1.06f;
            schmittLo = threshLin * 0.94f;
        }
        float process(float in)
        {
            float absIn = std::abs(in);
            float coeff = absIn > envState ? attackCoeff : releaseCoeff;
            envState = flushDenormal(envState * coeff + absIn * (1.0f - coeff));

            if ( swellOpen && envState < schmittLo) swellOpen = false;
            if (!swellOpen && envState > schmittHi) swellOpen = true;

            gainSmoother.set(swellOpen ? 1.0f : 0.0f);
            return in * gainSmoother.process();
        }
    } swell_;

    //==========================================================================
    // Stage 2 — Ring Modulator (Fairfield Randy's Revenge)
    //==========================================================================
    struct RingModStage
    {
        PolyBLEP sineCarrier;
        PolyBLEP squareCarrier;
        float dcOffset = 0.0f;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            sineCarrier.setWaveform(PolyBLEP::Waveform::Sine);
            squareCarrier.setWaveform(PolyBLEP::Waveform::Square);
            dcOffset = 0.0f;
        }
        void reset() { dcOffset = 0.0f; }

        void setFreq(float hz)
        {
            sineCarrier.setFrequency(hz, static_cast<float>(sr));
            squareCarrier.setFrequency(hz, static_cast<float>(sr));
        }

        float process(float in, float blend, float mix)
        {
            float sineSamp   = sineCarrier.processSample();
            float squareSamp = squareCarrier.processSample();
            float carrier = sineSamp * (1.0f - blend) + squareSamp * blend;
            carrier = fastTanh(carrier);
            float modOut = in * carrier;
            dcOffset = flushDenormal(dcOffset + 0.9997f * (modOut - dcOffset));
            modOut -= dcOffset;
            return in + mix * (modOut - in);
        }
    } ringMod_;

    //==========================================================================
    // Stage 3 — Hard Clip Distortion (Boss DS-1 Golden) — 4x oversampling
    //==========================================================================
    struct DistortionStage
    {
        float aaUp1 = 0.0f;
        float aaUp2 = 0.0f;
        CytomicSVF tiltSVF;

        void prepare(double /*sampleRate*/) { tiltSVF.reset(); reset(); }
        void reset()
        {
            aaUp1 = aaUp2 = 0.0f;
            tiltSVF.reset();
        }

        // Soft-knee saturator: x / (1 + |x|)
        static float softKnee(float x, float drive)
        {
            float d = x * drive;
            return d / (1.0f + std::abs(d));
        }

        float process(float in, float driveLinear, float tilt, float outGain, double sampleRate)
        {
            // 4x oversampling: 2x up via linear interp, process, 2x down via average
            float up1a = (in + aaUp1) * 0.5f;
            float up1b = in;
            aaUp1 = in;

            float up2a = (up1a + aaUp2) * 0.5f;
            float up2b = up1a;
            float up2c = (up1b + aaUp2) * 0.5f;
            float up2d = up1b;
            aaUp2 = up1b;

            up2a = softKnee(up2a, driveLinear);
            up2b = softKnee(up2b, driveLinear);
            up2c = softKnee(up2c, driveLinear);
            up2d = softKnee(up2d, driveLinear);

            float out = (up2a + up2b + up2c + up2d) * 0.25f;

            // Tilt EQ: CytomicSVF low-shelf
            float tiltGainDb = tilt * 6.0f;
            tiltSVF.setMode(CytomicSVF::Mode::LowShelf);
            tiltSVF.setCoefficients(800.0f, 0.707f, static_cast<float>(sampleRate), tiltGainDb);
            out = tiltSVF.processSample(out);

            return out * outGain;
        }
    } dist_;

    //==========================================================================
    // Stage 4 — Envelope Filter (Boss TW-1 T Wah)
    //==========================================================================
    struct EnvFilterStage
    {
        float envState    = 0.0f;
        float envAttack   = 0.0f;
        float envRelease  = 0.0f;
        CytomicSVF svf;
        ParameterSmoother freqSmoother;

        void prepare(double sampleRate)
        {
            freqSmoother.prepare(static_cast<float>(sampleRate), 0.001f);
            reset();
        }
        void reset()
        {
            envState = 0.0f;
            svf.reset();
            freqSmoother.snapTo(200.0f);
        }
        void setTimeConsts(double sampleRate)
        {
            envAttack  = fastExp(-1.0f / (0.001f * static_cast<float>(sampleRate)));
            envRelease = fastExp(-1.0f / (0.050f * static_cast<float>(sampleRate)));
        }
        float process(float in, float baseFreq, float depthOctaves, float resonanceQ,
                      double sampleRate)
        {
            float absIn = std::abs(in);
            float coeff = absIn > envState ? envAttack : envRelease;
            envState = flushDenormal(envState * coeff + absIn * (1.0f - coeff));

            float targetFreq = baseFreq * fastPow2(envState * depthOctaves);
            float srF = static_cast<float>(sampleRate);
            targetFreq = std::max(20.0f, std::min(targetFreq, srF * 0.45f));

            freqSmoother.set(targetFreq);
            float smoothedFreq = freqSmoother.process();

            // Map Q to CytomicSVF resonance (0–1 range)
            float res = 1.0f - (1.0f / resonanceQ);
            res = juce::jlimit(0.0f, 0.99f, res);
            svf.setMode(CytomicSVF::Mode::LowPass);
            svf.setCoefficients(smoothedFreq, res, srF);
            return svf.processSample(in);
        }
    } envFilter_;

    //==========================================================================
    // Stage 5 — Pitch-Sequenced BBD Delay (Chase Bliss Thermae)
    // Hermite fractional delay. Mono → Stereo split at this stage.
    //==========================================================================
    struct BBDDelayStage
    {
        static constexpr float kMaxDelayMs = 1500.0f;

        std::vector<float> delayBufL;
        std::vector<float> delayBufR;
        int writePosL = 0;
        int writePosR = 0;
        float feedbackL = 0.0f;
        float feedbackR = 0.0f;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            delayBufL.assign(maxSamples, 0.0f);
            delayBufR.assign(maxSamples, 0.0f);
            reset();
        }
        void reset()
        {
            std::fill(delayBufL.begin(), delayBufL.end(), 0.0f);
            std::fill(delayBufR.begin(), delayBufR.end(), 0.0f);
            writePosL = writePosR = 0;
            feedbackL = feedbackR = 0.0f;
        }

        // Hermite cubic interpolation
        static float hermite(const std::vector<float>& buf, int writePos, float delaySamples)
        {
            int size = static_cast<int>(buf.size());
            float readF = static_cast<float>(writePos) - delaySamples;
            while (readF < 0.0f) readF += static_cast<float>(size);
            while (readF >= static_cast<float>(size)) readF -= static_cast<float>(size);
            int i1 = static_cast<int>(readF);
            int i0 = (i1 - 1 + size) % size;
            int i2 = (i1 + 1) % size;
            int i3 = (i1 + 2) % size;
            float t  = readF - static_cast<float>(i1);
            float a  = buf[i0], b = buf[i1], c = buf[i2], d = buf[i3];
            return b + 0.5f * t * (c - a + t * (2.0f * a - 5.0f * b + 4.0f * c - d
                                                + t * (3.0f * (b - c) + d - a)));
        }

        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          float baseDelayMs, float semitone1, float semitone2,
                          float feedback, float mix, double /*bpm*/)
        {
            float srF = static_cast<float>(sr);
            float baseDelaySamples = std::max(1.0f, baseDelayMs * srF / 1000.0f);
            baseDelaySamples = std::min(baseDelaySamples,
                                        static_cast<float>(delayBufL.size()) - 2.0f);

            float ratioL = fastPow2(semitone1 / 12.0f);
            float ratioR = fastPow2(semitone2 / 12.0f);
            float delayL = baseDelaySamples / std::max(0.01f, ratioL);
            float delayR = baseDelaySamples / std::max(0.01f, ratioR);
            delayL = std::min(delayL, static_cast<float>(delayBufL.size()) - 2.0f);
            delayR = std::min(delayR, static_cast<float>(delayBufR.size()) - 2.0f);

            int bufSize = static_cast<int>(delayBufL.size());

            for (int i = 0; i < numSamples; ++i)
            {
                float in = monoIn[i];

                float wetL = hermite(delayBufL, writePosL, delayL);
                float wetR = hermite(delayBufR, writePosR, delayR);

                delayBufL[writePosL] = flushDenormal(in + feedbackL * feedback);
                delayBufR[writePosR] = flushDenormal(in + feedbackR * feedback);
                feedbackL = wetL;
                feedbackR = wetR;

                writePosL = (writePosL + 1) % bufSize;
                writePosR = (writePosR + 1) % bufSize;

                L[i] = in + mix * (wetL - in);
                R[i] = in + mix * (wetR - in);
            }
        }
    } bbd_;

    //==========================================================================
    // Cached parameter pointers (17 params)
    //==========================================================================
    std::atomic<float>* p_swellThresh  = nullptr;
    std::atomic<float>* p_swellAttack  = nullptr;
    std::atomic<float>* p_swellRelease = nullptr;
    std::atomic<float>* p_ringFreq     = nullptr;
    std::atomic<float>* p_ringWave     = nullptr;
    std::atomic<float>* p_ringMix      = nullptr;
    std::atomic<float>* p_distDrive    = nullptr;
    std::atomic<float>* p_distTilt     = nullptr;
    std::atomic<float>* p_distOut      = nullptr;
    std::atomic<float>* p_envBaseFreq  = nullptr;
    std::atomic<float>* p_envDepth     = nullptr;
    std::atomic<float>* p_envRes       = nullptr;
    std::atomic<float>* p_delayBase    = nullptr;
    std::atomic<float>* p_seqStep1     = nullptr;
    std::atomic<float>* p_seqStep2     = nullptr;
    std::atomic<float>* p_delayFb      = nullptr;
    std::atomic<float>* p_delayMix     = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OnrushChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    swell_.prepare(sampleRate);
    ringMod_.prepare(sampleRate);
    dist_.prepare(sampleRate);
    envFilter_.prepare(sampleRate);
    bbd_.prepare(sampleRate);
}

inline void OnrushChain::reset()
{
    swell_.reset();
    ringMod_.reset();
    dist_.reset();
    envFilter_.reset();
    bbd_.reset();
}

inline void OnrushChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double bpm, double /*ppqPosition*/)
{
    juce::ScopedNoDenormals noDenormals;
    if (!p_swellThresh) return;

    // ParamSnapshot: read all params once
    const float swellThresh  = p_swellThresh->load(std::memory_order_relaxed);
    const float swellAttack  = p_swellAttack->load(std::memory_order_relaxed);
    const float swellRelease = p_swellRelease->load(std::memory_order_relaxed);
    const float ringFreq     = p_ringFreq->load(std::memory_order_relaxed);
    const float ringWave     = p_ringWave->load(std::memory_order_relaxed);
    const float ringMix      = p_ringMix->load(std::memory_order_relaxed);
    const float distDrive    = p_distDrive->load(std::memory_order_relaxed);
    const float distTilt     = p_distTilt->load(std::memory_order_relaxed);
    const float distOut      = p_distOut->load(std::memory_order_relaxed);
    const float envBaseFreq  = p_envBaseFreq->load(std::memory_order_relaxed);
    const float envDepth     = p_envDepth->load(std::memory_order_relaxed);
    const float envRes       = p_envRes->load(std::memory_order_relaxed);
    const float delayBase    = p_delayBase->load(std::memory_order_relaxed);
    const float seqStep1     = p_seqStep1->load(std::memory_order_relaxed);
    const float seqStep2     = p_seqStep2->load(std::memory_order_relaxed);
    const float delayFb      = p_delayFb->load(std::memory_order_relaxed);
    const float delayMix     = p_delayMix->load(std::memory_order_relaxed);

    // distDrive is in dB (0–40) → linear
    float driveLinear    = fastPow2(distDrive * (1.0f / 6.0205999f));
    float outGainLinear  = fastPow2(distOut   * (1.0f / 6.0205999f));

    swell_.setCoeffs(swellAttack, swellRelease, swellThresh, sr_);
    ringMod_.setFreq(ringFreq);
    envFilter_.setTimeConsts(sr_);

    // Mono pipeline stages 1-4 (write to L as temp)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: Auto-Swell
        x = swell_.process(x);

        // Stage 2: Ring Modulator
        x = ringMod_.process(x, ringWave, ringMix * 0.01f);

        // Stage 3: Distortion
        x = dist_.process(x, driveLinear, distTilt, outGainLinear, sr_);

        // Stage 4: Envelope Filter
        x = envFilter_.process(x, envBaseFreq, envDepth, envRes, sr_);

        L[i] = x;
    }

    // Stage 5: BBD Delay — expands mono → stereo
    bbd_.processBlock(L, L, R, numSamples, delayBase, seqStep1, seqStep2,
                      delayFb * 0.01f, delayMix * 0.01f, bpm);
}

inline void OnrushChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    using AP = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    const juce::String p = slotPrefix + "onr_";

    layout.add(std::make_unique<AP>(juce::ParameterID{p + "swellThresh",  1}, p + "Swell Thresh",
        NR(-60.0f, 0.0f, 0.1f), -24.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "swellAttack",  1}, p + "Swell Attack",
        NR(1.0f, 500.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "swellRelease", 1}, p + "Swell Release",
        NR(10.0f, 2000.0f, 1.0f), 200.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "ringFreq",     1}, p + "Ring Freq",
        NR(20.0f, 2000.0f, 0.1f), 220.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "ringWave",     1}, p + "Ring Wave",
        NR(0.0f, 1.0f, 0.001f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "ringMix",      1}, p + "Ring Mix",
        NR(0.0f, 100.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "distDrive",    1}, p + "Dist Drive",
        NR(0.0f, 40.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "distTilt",     1}, p + "Dist Tilt",
        NR(-1.0f, 1.0f, 0.001f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "distOut",      1}, p + "Dist Out",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "envBaseFreq",  1}, p + "Env Base Freq",
        NR(50.0f, 2000.0f, 1.0f), 200.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "envDepth",     1}, p + "Env Depth",
        NR(0.0f, 4.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "envRes",       1}, p + "Env Res",
        NR(0.707f, 10.0f, 0.001f), 2.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "delayBase",    1}, p + "Delay Base",
        NR(1.0f, 1000.0f, 0.1f), 250.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "seqStep1",     1}, p + "Seq Step 1",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "seqStep2",     1}, p + "Seq Step 2",
        NR(-12.0f, 12.0f, 0.1f), 7.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "delayFb",      1}, p + "Delay Fb",
        NR(0.0f, 85.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "delayMix",     1}, p + "Delay Mix",
        NR(0.0f, 100.0f, 0.1f), 40.0f));
}

inline void OnrushChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "onr_";
    p_swellThresh  = apvts.getRawParameterValue(p + "swellThresh");
    p_swellAttack  = apvts.getRawParameterValue(p + "swellAttack");
    p_swellRelease = apvts.getRawParameterValue(p + "swellRelease");
    p_ringFreq     = apvts.getRawParameterValue(p + "ringFreq");
    p_ringWave     = apvts.getRawParameterValue(p + "ringWave");
    p_ringMix      = apvts.getRawParameterValue(p + "ringMix");
    p_distDrive    = apvts.getRawParameterValue(p + "distDrive");
    p_distTilt     = apvts.getRawParameterValue(p + "distTilt");
    p_distOut      = apvts.getRawParameterValue(p + "distOut");
    p_envBaseFreq  = apvts.getRawParameterValue(p + "envBaseFreq");
    p_envDepth     = apvts.getRawParameterValue(p + "envDepth");
    p_envRes       = apvts.getRawParameterValue(p + "envRes");
    p_delayBase    = apvts.getRawParameterValue(p + "delayBase");
    p_seqStep1     = apvts.getRawParameterValue(p + "seqStep1");
    p_seqStep2     = apvts.getRawParameterValue(p + "seqStep2");
    p_delayFb      = apvts.getRawParameterValue(p + "delayFb");
    p_delayMix     = apvts.getRawParameterValue(p + "delayMix");
}

} // namespace xoceanus
