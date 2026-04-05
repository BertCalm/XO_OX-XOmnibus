// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../ParameterSmoother.h"
#include "../PolyBLEP.h"
#include "../StandardLFO.h"
#include "FXChainHelpers.h"
#include "Saturator.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// OutlawChain — Cybernetic Child FX Chain (5 stages)
//
// Source concept: Cybernetic Child (Experimental Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 4 Hard VCA Panner)
// Accent: Chromatophore Amber #CC6600
//
// Stage 1: PLL Synth (EQD Data Corrupter)
//          EnvelopeFollower + zero-crossing pitch tracker → PolyBLEP square.
//          Intentional glitch via reduced SchmittTrigger hysteresis.
//          Sub-octave via period bitshift (÷2).
// Stage 2: Touch-Sensitive Envelope Filter (Boss TW-1)
//          EnvelopeFollower → CytomicSVF BandPass cutoff.
//          Sensitivity and peak (resonance) params.
// Stage 3: Plasma Distortion (Gamechanger Audio Plasma)
//          SchmittTrigger noise gate + extreme hard-clipping (std::clamp),
//          OversamplingProcessor<8>. Zero attack/release.
// Stage 4: Hard VCA Panner (Boss PN-2) — Mono → Stereo here.
//          StandardLFO square driving L/R gain. 180° offset ping-pong.
//          Host BPM sync.
// Stage 5: Magnetic Drum Echo (Strymon Volante)
//          4 FractionalDelay read heads + Saturator Tape feedback.
//          StandardLFO wow/flutter. CytomicSVF LP darkening.
//
// Parameter prefix: outl_ (12 params)
//==============================================================================
class OutlawChain
{
public:
    OutlawChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    /// Process mono input, writing stereo to L and R.
    /// Caller must ensure L != R (separate output buffers).
    void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                      double bpm = 0.0, double ppqPosition = -1.0);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_        = 44100.0;
    int    blockSize_ = 512;

    //==========================================================================
    // Stage 1 — PLL Synth (EQD Data Corrupter)
    //==========================================================================
    struct PLLStage
    {
        EnvelopeFollower env;
        SchmittTrigger   zc; // zero-crossing detector (low hysteresis = glitchy)
        PolyBLEP         squarePLL;
        PolyBLEP         squareSub;
        double sr = 44100.0;

        // Pitch tracking state
        float   lastSample      = 0.0f;
        int     periodCount     = 0;
        int     lastZeroCross   = 0;
        float   trackedFreqHz   = 220.0f;
        int     halfPeriodSamp  = 100;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            env.prepare(sampleRate);
            env.setAttack(1.0f);
            env.setRelease(20.0f);
            // Reduced hysteresis = intentional glitch behaviour
            zc.setThresholds(0.02f, 0.04f);
            zc.reset();
            squarePLL.setWaveform(PolyBLEP::Waveform::Square);
            squareSub.setWaveform(PolyBLEP::Waveform::Square);
            trackedFreqHz = 220.0f;
        }
        void reset()
        {
            env.reset();
            zc.reset();
            lastSample = 0.0f;
            periodCount = lastZeroCross = 0;
            trackedFreqHz = 220.0f;
            halfPeriodSamp = static_cast<int>(sr / (2.0 * 220.0));
        }

        float process(float in, float glide, float subVol, float squareVol)
        {
            float srF = static_cast<float>(sr);
            float envLevel = env.process(in);

            // Zero crossing detection (positive edge)
            bool crossed = (lastSample < 0.0f && in >= 0.0f);
            lastSample = in;
            ++periodCount;

            if (crossed)
            {
                if (periodCount > 2) // debounce
                {
                    // Track half-period
                    float measuredFreq = srF / static_cast<float>(std::max(1, periodCount * 2));
                    // Glide: lerp toward measured freq
                    float glideCoeff = fastExp(-1.0f / std::max(1.0f, glide * srF * 0.001f));
                    trackedFreqHz = trackedFreqHz * glideCoeff + measuredFreq * (1.0f - glideCoeff);
                    trackedFreqHz = std::max(20.0f, std::min(trackedFreqHz, srF * 0.45f));
                }
                periodCount = 0;
            }

            // Sub octave: half the PLL frequency
            float subFreq = std::max(10.0f, trackedFreqHz * 0.5f);
            squarePLL.setFrequency(trackedFreqHz, srF);
            squareSub.setFrequency(subFreq, srF);

            float pllOut = squarePLL.processSample();
            float subOut = squareSub.processSample();

            // Gate by envelope: quieter input = less PLL output
            float gate = std::min(1.0f, envLevel * 8.0f);
            return (pllOut * squareVol + subOut * subVol) * gate;
        }
    } pll_;

    //==========================================================================
    // Stage 2 — Touch-Sensitive Envelope Filter (Boss TW-1)
    //==========================================================================
    struct TWahStage
    {
        EnvelopeFollower env;
        CytomicSVF       svf;
        ParameterSmoother freqSmoother;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            env.prepare(sampleRate);
            svf.reset();
            freqSmoother.prepare(static_cast<float>(sampleRate), 0.002f);
            freqSmoother.snapTo(400.0f);
        }
        void reset()
        {
            env.reset();
            svf.reset();
            freqSmoother.snapTo(400.0f);
        }
        float process(float in, float sensitivity, float peak)
        {
            float srF = static_cast<float>(sr);
            env.setAttack(1.0f);
            env.setRelease(80.0f);
            float envLevel = env.process(in);

            // Sensitivity scales how much envelope moves the filter
            float targetFreq = 200.0f + envLevel * sensitivity * 4000.0f;
            targetFreq = std::max(80.0f, std::min(targetFreq, srF * 0.45f));

            freqSmoother.set(targetFreq);
            float smoothedFreq = freqSmoother.process();

            // peak maps to resonance (BandPass)
            float res = juce::jlimit(0.0f, 0.95f, peak * 0.95f);
            svf.setMode(CytomicSVF::Mode::BandPass);
            svf.setCoefficients(smoothedFreq, res, srF);
            return svf.processSample(in);
        }
    } twah_;

    //==========================================================================
    // Stage 3 — Plasma Distortion (Gamechanger Audio Plasma) — 8x OVS
    //==========================================================================
    struct PlasmaStage
    {
        SchmittTrigger      gate;
        OversamplingProcessor<8> ovs;
        bool gateOpen = false;

        void prepare(double sampleRate, int maxBlockSize)
        {
            ovs.prepare(sampleRate, maxBlockSize);
            // Tight hysteresis = almost-zero gate threshold (plasma-style)
            gate.setThresholds(0.005f, 0.015f);
            gate.reset();
            gateOpen = true;
        }
        void reset()
        {
            ovs.reset();
            gate.reset();
            gateOpen = true;
        }
        void processBlock(float* buf, int numSamples, float voltage)
        {
            // Hard clipping pre-pass at native rate: SchmittTrigger sets gating
            for (int i = 0; i < numSamples; ++i)
            {
                // Gate: if signal falls below noise floor, open gate (plasma noise floor)
                if (gate.process(buf[i]))
                    gateOpen = true;
                else if (std::abs(buf[i]) < 0.003f)
                    gateOpen = false;
            }

            // Apply extreme gain + hard clip inside 8x oversampled context
            float clipLevel = 1.0f / std::max(0.01f, voltage); // voltage → clip ceiling
            ovs.process(buf, numSamples, [&](float* upBuf, int upN)
            {
                for (int i = 0; i < upN; ++i)
                {
                    float x = upBuf[i] * voltage * 40.0f; // extreme gain
                    x = std::clamp(x, -clipLevel, clipLevel);
                    upBuf[i] = gateOpen ? x : 0.0f;
                }
            });
        }
    } plasma_;

    //==========================================================================
    // Stage 4 — Hard VCA Panner (Boss PN-2) — Mono → Stereo
    //==========================================================================
    struct PannerStage
    {
        StandardLFO lfoL;
        StandardLFO lfoR;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            lfoL.setShape(StandardLFO::Square);
            lfoR.setShape(StandardLFO::Square);
            lfoR.setPhaseOffset(0.5f); // 180° = ping-pong
            lfoL.reset();
            lfoR.reset();
        }
        void reset()
        {
            lfoL.reset();
            lfoR.reset();
        }
        void process(float in, float& outL, float& outR,
                     float rateHz, float depth, double bpm)
        {
            float srF = static_cast<float>(sr);
            // BPM sync: snap to nearest beat division if BPM is provided
            float effRate = rateHz;
            if (bpm > 1.0)
            {
                float beatHz = static_cast<float>(bpm) / 60.0f;
                // Find nearest power-of-two division
                float divs[6] = {4.0f, 2.0f, 1.0f, 0.5f, 0.25f, 0.125f};
                float bestDiff = 1e9f;
                for (float d : divs)
                {
                    float candidate = beatHz * d;
                    float diff = std::abs(candidate - rateHz);
                    if (diff < bestDiff)
                    {
                        bestDiff = diff;
                        effRate  = candidate;
                    }
                }
            }
            lfoL.setRate(effRate, srF);
            lfoR.setRate(effRate, srF);

            float modL = lfoL.process(); // [-1,+1], square → 0 or 1
            float modR = lfoR.process();
            // Square LFO: map to [0,1] unipolar gain
            float gainL = (modL > 0.0f) ? (0.5f + 0.5f * depth) : (0.5f - 0.5f * depth);
            float gainR = (modR > 0.0f) ? (0.5f + 0.5f * depth) : (0.5f - 0.5f * depth);
            outL = in * gainL;
            outR = in * gainR;
        }
    } panner_;

    //==========================================================================
    // Stage 5 — Magnetic Drum Echo (Strymon Volante)
    // 4 read heads + Tape saturation + wow/flutter + LP darkening
    //==========================================================================
    struct VolanteStage
    {
        static constexpr int kNumHeads  = 4;
        static constexpr float kMaxMs   = 1200.0f;

        FractionalDelay  delayL;
        FractionalDelay  delayR;
        StandardLFO      wowFlutter;
        CytomicSVF       darkFilter;
        Saturator        tapeSat;
        double sr = 44100.0;

        // Spacing ratios for even / triplet / golden-ratio modes
        static constexpr float kEvenRatios[kNumHeads]      = {0.25f, 0.5f, 0.75f, 1.0f};
        static constexpr float kTripletRatios[kNumHeads]   = {0.333f, 0.5f, 0.666f, 1.0f};
        static constexpr float kGoldenRatios[kNumHeads]    = {0.236f, 0.382f, 0.618f, 1.0f};

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxMs * sampleRate / 1000.0) + 16;
            delayL.prepare(maxSamp);
            delayR.prepare(maxSamp);
            wowFlutter.setShape(StandardLFO::Sine);
            wowFlutter.setRate(0.8f, static_cast<float>(sampleRate));
            wowFlutter.reset();
            darkFilter.reset();
            tapeSat.setMode(Saturator::SaturationMode::Tape);
            tapeSat.setDrive(0.4f);
            tapeSat.setMix(1.0f);
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
            wowFlutter.reset();
            darkFilter.reset();
        }

        void processBlock(const float* inL, const float* inR,
                          float* outL, float* outR, int numSamples,
                          float baseMs, float feedback, int spacingMode, float wear)
        {
            float srF = static_cast<float>(sr);
            float baseSamp = std::max(1.0f, baseMs * srF / 1000.0f);
            baseSamp = std::min(baseSamp, static_cast<float>(delayL.getSize()) - 4.0f);

            const float* ratios = kEvenRatios;
            if      (spacingMode == 1) ratios = kTripletRatios;
            else if (spacingMode == 2) ratios = kGoldenRatios;

            // Wow/flutter LFO: ~0.5–2 Hz
            wowFlutter.setRate(0.5f + wear * 1.5f, srF);

            for (int i = 0; i < numSamples; ++i)
            {
                float wow = wowFlutter.process() * wear * 5.0f; // ±samples pitch wobble
                float wetL = 0.0f, wetR = 0.0f;

                for (int h = 0; h < kNumHeads; ++h)
                {
                    float headSamp = baseSamp * ratios[h] + wow;
                    headSamp = std::max(1.0f, std::min(headSamp, baseSamp));
                    wetL += delayL.read(headSamp);
                    wetR += delayR.read(headSamp);
                }
                wetL /= static_cast<float>(kNumHeads);
                wetR /= static_cast<float>(kNumHeads);

                // Tape saturation in feedback
                float fbL = tapeSat.processSample(wetL) * feedback;
                float fbR = tapeSat.processSample(wetR) * feedback;

                // LP darkening on feedback
                darkFilter.setMode(CytomicSVF::Mode::LowPass);
                darkFilter.setCoefficients(3000.0f - wear * 1500.0f, 0.5f, srF);
                fbL = darkFilter.processSample(fbL);
                fbR = flushDenormal(fbR);

                delayL.write(flushDenormal(inL[i] + fbL));
                delayR.write(flushDenormal(inR[i] + fbR));

                outL[i] = inL[i] + wetL;
                outR[i] = inR[i] + wetR;
            }
        }
    } volante_;

    //==========================================================================
    // Temporary stereo work buffers
    //==========================================================================
    std::vector<float> tmpL_;
    std::vector<float> tmpR_;

    //==========================================================================
    // Cached parameter pointers (12 params)
    //==========================================================================
    std::atomic<float>* p_pllGlide      = nullptr;
    std::atomic<float>* p_pllSubVol     = nullptr;
    std::atomic<float>* p_pllSquareVol  = nullptr;
    std::atomic<float>* p_twahSens      = nullptr;
    std::atomic<float>* p_twahPeak      = nullptr;
    std::atomic<float>* p_plasmaVoltage = nullptr;
    std::atomic<float>* p_plasmaBlend   = nullptr;
    std::atomic<float>* p_panRate       = nullptr;
    std::atomic<float>* p_panDepth      = nullptr;
    std::atomic<float>* p_drumFeedback  = nullptr;
    std::atomic<float>* p_drumSpacing   = nullptr;
    std::atomic<float>* p_drumWear      = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OutlawChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    pll_.prepare(sampleRate);
    twah_.prepare(sampleRate);
    plasma_.prepare(sampleRate, maxBlockSize);
    panner_.prepare(sampleRate);
    volante_.prepare(sampleRate);
    tmpL_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    tmpR_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
}

inline void OutlawChain::reset()
{
    pll_.reset();
    twah_.reset();
    plasma_.reset();
    panner_.reset();
    volante_.reset();
}

inline void OutlawChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double bpm, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_pllGlide) return;

    // ParamSnapshot
    const float pllGlide      = p_pllGlide->load(std::memory_order_relaxed);
    const float pllSubVol     = p_pllSubVol->load(std::memory_order_relaxed);
    const float pllSquareVol  = p_pllSquareVol->load(std::memory_order_relaxed);
    const float twahSens      = p_twahSens->load(std::memory_order_relaxed);
    const float twahPeak      = p_twahPeak->load(std::memory_order_relaxed);
    const float plasmaVoltage = std::max(0.1f, p_plasmaVoltage->load(std::memory_order_relaxed));
    const float plasmaBlend   = p_plasmaBlend->load(std::memory_order_relaxed);
    const float panRate       = p_panRate->load(std::memory_order_relaxed);
    const float panDepth      = p_panDepth->load(std::memory_order_relaxed);
    const float drumFeedback  = p_drumFeedback->load(std::memory_order_relaxed);
    const int   drumSpacing   = static_cast<int>(p_drumSpacing->load(std::memory_order_relaxed));
    const float drumWear      = p_drumWear->load(std::memory_order_relaxed);

    // Mono pipeline stages 1–3; stereo expansion at stage 4
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: PLL Synth — blend PLL output with dry
        float pllOut = pll_.process(x, pllGlide, pllSubVol, pllSquareVol);
        x = x + pllOut * plasmaBlend; // plasmaBlend also drives PLL blend depth

        // Stage 2: Touch Wah
        x = twah_.process(x, twahSens, twahPeak);

        // Stage 3: Plasma distortion — uses block-level OVS, write to tmpL_ first
        tmpL_[i] = x;
    }
    plasma_.processBlock(tmpL_.data(), numSamples, plasmaVoltage);

    // Stage 4: Panner → stereo
    for (int i = 0; i < numSamples; ++i)
    {
        float xL = 0.0f, xR = 0.0f;
        panner_.process(tmpL_[i], xL, xR, panRate, panDepth, bpm);
        tmpL_[i] = xL;
        tmpR_[i] = xR;
    }

    // Stage 5: Drum Echo (stereo)
    volante_.processBlock(tmpL_.data(), tmpR_.data(), L, R, numSamples,
                          400.0f, drumFeedback, drumSpacing, drumWear);
}

inline void OutlawChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "outl_";
    registerFloat(layout, p + "pllGlide",      p + "PLL Glide",      0.0f,  200.0f,  20.0f);
    registerFloat(layout, p + "pllSubVol",     p + "PLL Sub Vol",    0.0f,  1.0f,    0.5f);
    registerFloat(layout, p + "pllSquareVol",  p + "PLL Square Vol", 0.0f,  1.0f,    0.7f);
    registerFloat(layout, p + "twahSens",      p + "T-Wah Sens",     0.0f,  1.0f,    0.6f);
    registerFloat(layout, p + "twahPeak",      p + "T-Wah Peak",     0.0f,  1.0f,    0.5f);
    registerFloat(layout, p + "plasmaVoltage", p + "Plasma Voltage", 0.1f,  1.0f,    0.7f);
    registerFloat(layout, p + "plasmaBlend",   p + "Plasma Blend",   0.0f,  1.0f,    0.5f);
    registerFloat(layout, p + "panRate",       p + "Pan Rate",       0.1f,  8.0f,    1.0f);
    registerFloat(layout, p + "panDepth",      p + "Pan Depth",      0.0f,  1.0f,    0.8f);
    registerFloat(layout, p + "drumFeedback",  p + "Drum Feedback",  0.0f,  0.9f,    0.4f);
    registerChoice(layout, p + "drumSpacing", p + "Drum Spacing",
                   {"Even", "Triplet", "GoldenRatio"}, 0);
    registerFloat(layout, p + "drumWear",      p + "Drum Wear",      0.0f,  1.0f,    0.3f);
}

inline void OutlawChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "outl_";
    p_pllGlide      = cacheParam(apvts, p + "pllGlide");
    p_pllSubVol     = cacheParam(apvts, p + "pllSubVol");
    p_pllSquareVol  = cacheParam(apvts, p + "pllSquareVol");
    p_twahSens      = cacheParam(apvts, p + "twahSens");
    p_twahPeak      = cacheParam(apvts, p + "twahPeak");
    p_plasmaVoltage = cacheParam(apvts, p + "plasmaVoltage");
    p_plasmaBlend   = cacheParam(apvts, p + "plasmaBlend");
    p_panRate       = cacheParam(apvts, p + "panRate");
    p_panDepth      = cacheParam(apvts, p + "panDepth");
    p_drumFeedback  = cacheParam(apvts, p + "drumFeedback");
    p_drumSpacing   = cacheParam(apvts, p + "drumSpacing");
    p_drumWear      = cacheParam(apvts, p + "drumWear");
}

} // namespace xoceanus
