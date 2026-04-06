// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
#include "../PolyBLEP.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include "FXChainHelpers.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OmenChain — Reverb-Driven PLL Synthesis FX Chain (5 stages)
//
// Source concepts: Catalinbread Topanga (spring reverb), EQD Data Corrupter (PLL),
//                  Ibanez CF7 Flanger, Strymon Deco (tape saturation)
// Routing: PARALLEL SPLIT — Mono In → Stereo Out
//
// SPECIAL ROUTING:
//   Stage 1  — Parallel Splitter: copy input to dryBuffer
//   Stage 2  — Drip Spring Reverb (Catalinbread Topanga): wet path only
//   Stage 3  — PLL Synthesizer (EQD Data Corrupter): fed by reverb tail
//   Stage 4  — Wack'd Flanger (Ibanez CF7): DRY + WET merged here via splitBlend;
//              mono → stereo
//   Stage 5  — Master Tape Saturation (Strymon Deco): glues parallel paths
//
// Stage 2: Custom spring sim — cascade of CytomicSVF AllPass + long FractionalDelay.
//          High dwell = chaotic metallic tail. 100% wet into PLL.
// Stage 3: EnvelopeFollower + zero-crossing pitch tracker fed by reverb tail.
//          PolyBLEP square at tracked freq + sub-octave dividers.
//          Glitches beautifully on spring chaos.
// Stage 4: Short FractionalDelay modulated by StandardLFO capable of audio rate.
//          splitBlend merges dryBuffer + PLL signal before flanger.
// Stage 5: Saturator Tape mode + CytomicSVF dynamic EQ. Glues parallel paths.
//
// Parameter prefix: omen_ (10 params)
//==============================================================================
class OmenChain
{
public:
    OmenChain() = default;

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
    double sr_        = 44100.0;
    int    blockSize_ = 512;

    // Scratch buffer for dry path (parallel split)
    std::vector<float> dryBuffer_;

    //==========================================================================
    // Stage 2 — Drip Spring Reverb (Catalinbread Topanga)
    // Spring simulation: AllPass cascade + long FractionalDelay feedback.
    //==========================================================================
    struct SpringReverbStage
    {
        static constexpr int   kNumAllPasses   = 8;
        static constexpr float kMaxDelayMs     = 800.0f;

        CytomicSVF allPasses[kNumAllPasses];
        FractionalDelay tank;
        float tankFeedback = 0.0f;
        CytomicSVF toneFilter;
        double sr = 44100.0;

        // Mutually irrational allpass delay scales (spring dispersion signature)
        static constexpr float kAPFreqs[kNumAllPasses] = {
            137.0f, 211.0f, 347.0f, 521.0f,
            823.0f, 1187.0f, 1733.0f, 2503.0f
        };

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            for (auto& ap : allPasses) ap.reset();
            int maxSamp = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            tank.prepare(maxSamp);
            toneFilter.reset();
            tankFeedback = 0.0f;
        }
        void reset()
        {
            for (auto& ap : allPasses) ap.reset();
            tank.clear();
            toneFilter.reset();
            tankFeedback = 0.0f;
        }
        float processSample(float in, float dwell, float tone, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            // Tone filter: 0 = dark (LP), 1 = bright (HP tilt)
            float toneFreq = 200.0f + tone * 3800.0f;
            toneFilter.setMode(CytomicSVF::Mode::LowPass);
            toneFilter.setCoefficients(toneFreq, 0.5f, srF);

            // AllPass cascade: spring dispersion character
            float x = in + tankFeedback * dwell * 0.8f;
            x = flushDenormal(x);
            for (int i = 0; i < kNumAllPasses; ++i)
            {
                allPasses[i].setMode(CytomicSVF::Mode::AllPass);
                // Use mutually irrational frequencies to avoid metallic resonances
                float freq = kAPFreqs[i];
                freq = std::min(freq, srF * 0.44f);
                allPasses[i].setCoefficients(freq, 0.6f, srF);
                x = allPasses[i].processSample(x);
            }

            // Long delay tank (~300ms base, scaled by dwell)
            float tankMs   = 100.0f + dwell * 400.0f;
            float tankSamp = std::min(tankMs * srF / 1000.0f,
                                      static_cast<float>(tank.getSize() - 2));
            tankSamp = std::max(tankSamp, 1.0f);

            tank.write(flushDenormal(x));
            float tankOut = tank.read(tankSamp);
            tankFeedback  = flushDenormal(toneFilter.processSample(tankOut));

            // 100% wet output into PLL
            return flushDenormal(tankOut);
        }
    } springReverb_;

    //==========================================================================
    // Stage 3 — Phase-Locked Loop Synthesizer (EQD Data Corrupter)
    // Zero-crossing pitch tracker from reverb tail → PolyBLEP square + sub-dividers
    //==========================================================================
    struct PLLStage
    {
        PolyBLEP squareOsc;
        PolyBLEP subOctave1; // ÷2
        PolyBLEP subOctave2; // ÷4

        EnvelopeFollower ampEnv;
        ParameterSmoother freqSmoother;

        float prevSample    = 0.0f;
        int   zeroCrossCount = 0;
        int   samplesSinceCross = 0;
        float trackedFreqHz = 110.0f;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            squareOsc.setWaveform(PolyBLEP::Waveform::Square);
            subOctave1.setWaveform(PolyBLEP::Waveform::Square);
            subOctave2.setWaveform(PolyBLEP::Waveform::Square);
            ampEnv.prepare(sampleRate);
            ampEnv.setAttack(5.0f);
            ampEnv.setRelease(200.0f);
            freqSmoother.prepare(static_cast<float>(sampleRate), 0.050f);
            freqSmoother.snapTo(110.0f);
            prevSample = 0.0f;
            samplesSinceCross = 0;
        }
        void reset()
        {
            prevSample = 0.0f;
            samplesSinceCross = 0;
            trackedFreqHz = 110.0f;
            ampEnv.reset();
            freqSmoother.snapTo(110.0f);
        }
        float processSample(float reverbIn, float pllGlide, float subOctaveBlend,
                             float squareLevel, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);

            // Zero-crossing pitch tracker (positive-going crossings)
            samplesSinceCross++;
            bool crossed = (prevSample <= 0.0f && reverbIn > 0.0f);
            prevSample = reverbIn;

            if (crossed && samplesSinceCross > 4)
            {
                // Period estimate from crossing interval
                float period = static_cast<float>(samplesSinceCross);
                float rawFreq = srF / (period * 2.0f); // x2 because we track half-periods
                rawFreq = std::max(20.0f, std::min(rawFreq, srF * 0.45f));

                // Glide: pllGlide 0→1 slows tracker response
                float glideMs = pllGlide * 200.0f + 5.0f;
                freqSmoother.prepare(srF, glideMs * 0.001f);
                freqSmoother.set(rawFreq);
                samplesSinceCross = 0;
            }

            float smoothFreq = freqSmoother.process();
            smoothFreq = std::max(20.0f, std::min(smoothFreq, srF * 0.45f));

            squareOsc.setFrequency(smoothFreq,           srF);
            subOctave1.setFrequency(smoothFreq * 0.5f,  srF);
            subOctave2.setFrequency(smoothFreq * 0.25f, srF);

            float sq  = squareOsc.processSample();
            float sub1 = subOctave1.processSample();
            float sub2 = subOctave2.processSample();

            // Mix: squareLevel controls fundamental vs sub-octave balance
            float fundamental = sq * squareLevel;
            float subBlend    = (sub1 + sub2 * 0.5f) * subOctaveBlend * (1.0f - squareLevel * 0.5f);

            // Amplitude envelope tracks reverb energy — PLL goes quiet when spring is quiet
            float envLevel = ampEnv.process(reverbIn);
            float output   = (fundamental + subBlend) * envLevel;

            return flushDenormal(output);
        }
    } pll_;

    //==========================================================================
    // Stage 4 — "Wack'd" Flanger (Ibanez CF7)
    // Short delay, LFO capable of audio rate (0.1–1000 Hz), mono→stereo.
    // DRY + WET merge happens here via splitBlend parameter.
    //==========================================================================
    struct CF7Stage
    {
        static constexpr float kMaxDelayMs = 15.0f;

        FractionalDelay delayL, delayR;
        StandardLFO lfoL, lfoR;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            delayL.prepare(maxSamp);
            delayR.prepare(maxSamp);
            // Slightly offset LFO phases for stereo width
            lfoL.setShape(StandardLFO::Sine);
            lfoR.setShape(StandardLFO::Sine);
            lfoR.phase = 0.25f; // 90° offset for quadrature stereo
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
            lfoL.reset();
            lfoR.reset();
        }
        void processSampleStereo(float in, float& outL, float& outR,
                                  float rate, float depth, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            // Rate: 0.1–1000 Hz (audio-rate flanger capable)
            lfoL.setRate(rate, srF);
            lfoR.setRate(rate, srF);
            float modL = lfoL.process();
            float modR = lfoR.process();

            // Depth: 0–1 → 0–7 ms modulation range
            float delL = 0.5f + (1.0f + modL) * 0.5f * depth * 7.0f;
            float delR = 0.5f + (1.0f + modR) * 0.5f * depth * 7.0f;
            float maxD = kMaxDelayMs * srF / 1000.0f - 1.0f;
            delL = std::max(0.5f, std::min(delL * srF / 1000.0f, maxD));
            delR = std::max(0.5f, std::min(delR * srF / 1000.0f, maxD));

            delayL.write(in);
            delayR.write(in);
            float wetL = delayL.read(delL);
            float wetR = delayR.read(delR);

            outL = flushDenormal(in + wetL); // comb-filter characteristic
            outR = flushDenormal(in + wetR);
        }
    } cf7_;

    //==========================================================================
    // Stage 5 — Master Tape Saturation (Strymon Deco)
    //==========================================================================
    struct DecoStage
    {
        Saturator satL, satR;
        CytomicSVF dynEqL, dynEqR;
        EnvelopeFollower envL, envR;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            satL.setMode(Saturator::SaturationMode::Tape);
            satR.setMode(Saturator::SaturationMode::Tape);
            satL.setMix(1.0f);
            satR.setMix(1.0f);
            dynEqL.reset();
            dynEqR.reset();
            envL.prepare(sampleRate);
            envR.prepare(sampleRate);
            envL.setAttack(3.0f);  envL.setRelease(80.0f);
            envR.setAttack(3.0f);  envR.setRelease(80.0f);
        }
        void reset()
        {
            dynEqL.reset();
            dynEqR.reset();
            envL.reset();
            envR.reset();
        }
        void processStereo(float* L, float* R, int numSamples,
                            float saturation, float volume, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            satL.setDrive(saturation);
            satR.setDrive(saturation);

            for (int i = 0; i < numSamples; ++i)
            {
                float xL = satL.processSample(L[i]);
                float xR = satR.processSample(R[i]);

                // Dynamic EQ: envelope-driven high-shelf rolloff to tame harsh artifacts
                float envLvlL = envL.process(xL);
                float envLvlR = envR.process(xR);
                float shelfGainL = -envLvlL * 6.0f; // 0 to -6 dB on loud peaks
                float shelfGainR = -envLvlR * 6.0f;

                dynEqL.setMode(CytomicSVF::Mode::HighShelf);
                dynEqL.setCoefficients(6000.0f, 0.707f, srF, shelfGainL);
                dynEqR.setMode(CytomicSVF::Mode::HighShelf);
                dynEqR.setCoefficients(6000.0f, 0.707f, srF, shelfGainR);

                L[i] = flushDenormal(dynEqL.processSample(xL) * volume);
                R[i] = flushDenormal(dynEqR.processSample(xR) * volume);
            }
        }
    } deco_;

    //==========================================================================
    // Cached parameter pointers (10 params)
    //==========================================================================
    std::atomic<float>* p_splitBlend    = nullptr;
    std::atomic<float>* p_springDwell   = nullptr;
    std::atomic<float>* p_springTone    = nullptr;
    std::atomic<float>* p_pllGlide      = nullptr;
    std::atomic<float>* p_pllSubOctave  = nullptr;
    std::atomic<float>* p_pllSquare     = nullptr;
    std::atomic<float>* p_cf7Rate       = nullptr;
    std::atomic<float>* p_cf7Depth      = nullptr;
    std::atomic<float>* p_decoSaturation = nullptr;
    std::atomic<float>* p_decoVolume    = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OmenChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;

    // Pre-allocate dry scratch buffer — no runtime allocation in processBlock
    dryBuffer_.assign(static_cast<size_t>(maxBlockSize), 0.0f);

    springReverb_.prepare(sampleRate);
    pll_.prepare(sampleRate);
    cf7_.prepare(sampleRate);
    deco_.prepare(sampleRate);
}

inline void OmenChain::reset()
{
    springReverb_.reset();
    pll_.reset();
    cf7_.reset();
    deco_.reset();
}

inline void OmenChain::processBlock(const float* monoIn, float* L, float* R,
                                     int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_splitBlend) return;

    // ParamSnapshot
    const float splitBlend    = p_splitBlend->load(std::memory_order_relaxed);
    const float springDwell   = p_springDwell->load(std::memory_order_relaxed);
    const float springTone    = p_springTone->load(std::memory_order_relaxed);
    const float pllGlide      = p_pllGlide->load(std::memory_order_relaxed);
    const float pllSubOctave  = p_pllSubOctave->load(std::memory_order_relaxed);
    const float pllSquare     = p_pllSquare->load(std::memory_order_relaxed);
    const float cf7Rate       = p_cf7Rate->load(std::memory_order_relaxed);
    const float cf7Depth      = p_cf7Depth->load(std::memory_order_relaxed);
    const float decoSaturation = p_decoSaturation->load(std::memory_order_relaxed);
    const float decoVolume    = p_decoVolume->load(std::memory_order_relaxed);

    // STAGE 1: Parallel splitter — copy input to dryBuffer
    //          dryBuffer_ was pre-allocated in prepare(); safe to copy here
    const int numToCopy = std::min(numSamples, blockSize_);
    std::copy(monoIn, monoIn + numToCopy, dryBuffer_.data());

    // STAGES 2–3: Process wet path per-sample (spring reverb → PLL)
    // Write PLL output into L[] as mono wet temp buffer
    for (int i = 0; i < numSamples; ++i)
    {
        // Stage 2: Spring reverb (100% wet)
        float springOut = springReverb_.processSample(monoIn[i], springDwell, springTone, sr_);

        // Stage 3: PLL synthesizer driven by reverb tail
        float pllOut = pll_.processSample(springOut, pllGlide, pllSubOctave, pllSquare, sr_);

        // Write wet signal to L as mono scratch
        L[i] = pllOut;
    }

    // STAGE 4: Blend dry + wet, then flanger expands to stereo
    // splitBlend: 0 = all dry, 1 = all PLL/wet
    for (int i = 0; i < numSamples; ++i)
    {
        float blended = dryBuffer_[i] * (1.0f - splitBlend) + L[i] * splitBlend;
        // CF7 flanger — mono → stereo expansion
        float outL, outR;
        cf7_.processSampleStereo(blended, outL, outR, cf7Rate, cf7Depth, sr_);
        L[i] = outL;
        R[i] = outR;
    }

    // STAGE 5: Strymon Deco tape saturation — stereo in-place
    deco_.processStereo(L, R, numSamples, decoSaturation, decoVolume, sr_);
}

inline void OmenChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "omen_";

    registerFloat(layout, p + "splitBlend",    p + "Split Blend",
                  0.0f, 1.0f, 0.5f);
    registerFloat(layout, p + "springDwell",   p + "Spring Dwell",
                  0.0f, 1.0f, 0.5f);
    registerFloat(layout, p + "springTone",    p + "Spring Tone",
                  0.0f, 1.0f, 0.6f);
    registerFloat(layout, p + "pllGlide",      p + "PLL Glide",
                  0.0f, 1.0f, 0.3f);
    registerFloat(layout, p + "pllSubOctave",  p + "PLL Sub Oct",
                  0.0f, 1.0f, 0.4f);
    registerFloat(layout, p + "pllSquare",     p + "PLL Square",
                  0.0f, 1.0f, 0.7f);
    registerFloatSkewed(layout, p + "cf7Rate", p + "CF7 Rate",
                        0.1f, 1000.0f, 1.5f, 0.001f, 0.25f);
    registerFloat(layout, p + "cf7Depth",      p + "CF7 Depth",
                  0.0f, 1.0f, 0.5f);
    registerFloat(layout, p + "decoSaturation", p + "Deco Saturation",
                  0.0f, 1.0f, 0.3f);
    registerFloat(layout, p + "decoVolume",    p + "Deco Volume",
                  0.0f, 1.5f, 0.9f);
}

inline void OmenChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "omen_";
    p_splitBlend     = cacheParam(apvts, p + "splitBlend");
    p_springDwell    = cacheParam(apvts, p + "springDwell");
    p_springTone     = cacheParam(apvts, p + "springTone");
    p_pllGlide       = cacheParam(apvts, p + "pllGlide");
    p_pllSubOctave   = cacheParam(apvts, p + "pllSubOctave");
    p_pllSquare      = cacheParam(apvts, p + "pllSquare");
    p_cf7Rate        = cacheParam(apvts, p + "cf7Rate");
    p_cf7Depth       = cacheParam(apvts, p + "cf7Depth");
    p_decoSaturation = cacheParam(apvts, p + "decoSaturation");
    p_decoVolume     = cacheParam(apvts, p + "decoVolume");
}

} // namespace xoceanus
