// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../PolyBLEP.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <array>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// ObscurityChain — OBSCURITY Dark Ambient FX Chain (5 stages)
//
// Source concept: VoidAtmosphere (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 3 dimension chorus)
// Accent: Void Purple #2D1B69
//
// Stage 1: PLL Synthesizer (EQD Data Corrupter) — LP→Schmitt→period→PolyBLEP
// Stage 2: Asymmetric Diode OD (Boss PW-2) — positive/negative mismatched clip
// Stage 3: Dimension Chorus (Boss DC-2) — dual fractional delay + 180° phase split
// Stage 4: Degrading BBD Delay (Boss DM-1) — BBD bandwidth loss + tanh accumulation
// Stage 5: Industrial Multi-Tap Reverb (Death By Audio Rooms) — 8 prime taps
//
// Parameter prefix: obsc_ (20 params)
//==============================================================================
class ObscurityChain
{
public:
    ObscurityChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // monoIn: single channel. L/R: stereo output.
    void processBlock(const float* monoIn, float* L, float* R, int numSamples);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_ = 0.0;  // Sentinel: must be set by prepare() before use

    //==========================================================================
    // Stage 1 — PLL Synthesizer (EQD Data Corrupter)
    // Extreme LP → Schmitt trigger → period measurement → PolyBLEP square
    //==========================================================================
    struct PLLStage
    {
        float lpState = 0.0f;
        bool  schmittState = false;
        int   periodCounter = 0;
        int   lastPeriod    = 0;
        PolyBLEP squareOsc;
        PolyBLEP sub1Osc;
        PolyBLEP sub2Osc;
        ParameterSmoother freqGlide;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            squareOsc.setWaveform(PolyBLEP::Waveform::Square);
            sub1Osc.setWaveform(PolyBLEP::Waveform::Square);
            sub2Osc.setWaveform(PolyBLEP::Waveform::Square);
            freqGlide.prepare(static_cast<float>(sampleRate), 0.100f);
            freqGlide.snapTo(100.0f); // initial frequency guess
            reset();
        }
        void reset()
        {
            lpState = 0.0f;
            schmittState = false;
            periodCounter = lastPeriod = 0;
        }

        float process(float in, float glideMs, float sub1Db, float sub2Db, float squareDb,
                      double sampleRate)
        {
            // Update glide time constant
            freqGlide.prepare(static_cast<float>(sampleRate), glideMs * 0.001f);

            // Extreme LP (fc ~200Hz)
            float lpCoeff = fastExp(-2.0f * 3.14159f * 200.0f / static_cast<float>(sampleRate));
            lpState = flushDenormal(lpState * lpCoeff + in * (1.0f - lpCoeff));

            // Schmitt trigger (hysteresis ±0.05)
            bool prevState = schmittState;
            if ( schmittState && lpState < -0.05f) schmittState = false;
            if (!schmittState && lpState >  0.05f) schmittState = true;

            periodCounter++;
            if (schmittState && !prevState) // rising edge → period detected
            {
                if (periodCounter > 0)
                {
                    lastPeriod = periodCounter;
                    periodCounter = 0;
                    float freq = static_cast<float>(sampleRate) / static_cast<float>(lastPeriod);
                    freq = std::max(20.0f, std::min(freq, 2000.0f));
                    freqGlide.set(freq);
                }
            }

            float smoothedFreq = freqGlide.process();
            float srF = static_cast<float>(sampleRate);
            squareOsc.setFrequency(smoothedFreq,              srF);
            sub1Osc.setFrequency(smoothedFreq  * 0.5f,        srF);
            sub2Osc.setFrequency(smoothedFreq  * 0.25f,       srF);

            float sq  = squareOsc.processSample();
            float s1  = sub1Osc.processSample();
            float s2  = sub2Osc.processSample();

            float sq_lin = fastPow2(squareDb * (1.0f / 6.0205999f));
            float s1_lin = fastPow2(sub1Db   * (1.0f / 6.0205999f));
            float s2_lin = fastPow2(sub2Db   * (1.0f / 6.0205999f));

            return in + sq * sq_lin + s1 * s1_lin + s2 * s2_lin;
        }
    } pll_;

    //==========================================================================
    // Stage 2 — Asymmetric Diode Overdrive (Boss PW-2)
    // Positive: 1 - exp(-x * drive) | Negative: -1 + exp(x * drive * 0.5)
    // 4x oversampling + pre-clip LP shelf ("Fat") + post-clip peak ("Muscle")
    //==========================================================================
    struct AsymDiodeStage
    {
        CytomicSVF fatSVF;
        CytomicSVF muscleSVF;
        float aaState = 0.0f;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate) { sr = sampleRate; reset(); }
        void reset() { fatSVF.reset(); muscleSVF.reset(); aaState = 0.0f; }

        static float diodeClip(float x, float drive)
        {
            if (x >= 0.0f)
                return 1.0f - fastExp(-x * drive);
            else
                return -1.0f + fastExp(x * drive * 0.5f);
        }

        float process(float in, float drive, float fatFreq, float fatRes,
                      float muscleFreq, float muscleGain, float outDb)
        {
            // Pre-clip "Fat" low-shelf
            fatSVF.setMode(CytomicSVF::Mode::LowShelf);
            fatSVF.setCoefficients(fatFreq, juce::jlimit(0.0f, 0.99f, fatRes - 1.0f) * 0.2f,
                                   static_cast<float>(sr), 6.0f);
            float preEQ = fatSVF.processSample(in);

            // 4x oversample + asymmetric clip
            float samples[4];
            float prev = aaState;
            for (int k = 0; k < 4; ++k)
            {
                float t = static_cast<float>(k) / 4.0f;
                samples[k] = prev + t * (preEQ - prev);
            }
            aaState = preEQ;

            for (int k = 0; k < 4; ++k)
                samples[k] = diodeClip(samples[k], drive);

            float out = 0.0f;
            for (int k = 0; k < 4; ++k) out += samples[k];
            out *= 0.25f;

            // Post-clip "Muscle" wide-Q peak
            muscleSVF.setMode(CytomicSVF::Mode::Peak);
            muscleSVF.setCoefficients(muscleFreq, 0.3f, static_cast<float>(sr), muscleGain);
            out = muscleSVF.processSample(out);

            return out * fastPow2(outDb * (1.0f / 6.0205999f));
        }
    } diode_;

    //==========================================================================
    // Stage 3 — Dimension Chorus (Boss DC-2)
    // 2 parallel fractional delay lines + 180° LFO offset + hard-pan L/R
    //==========================================================================
    struct DimensionChorusStage
    {
        // Mode depth/rate presets (DC-2 feel)
        static constexpr float kModeDepth[4] = { 2.0f, 4.0f, 6.5f, 10.0f }; // ms
        static constexpr float kModeRate[4]  = { 0.5f, 0.8f, 1.2f, 1.8f  }; // Hz

        std::vector<float> delay1, delay2;
        int write1 = 0, write2 = 0;
        StandardLFO lfo1, lfo2;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(15.0 * sampleRate / 1000.0) + 4;
            delay1.assign(maxSamples, 0.0f);
            delay2.assign(maxSamples, 0.0f);
            lfo1.setShape(StandardLFO::Shape::Sine);
            lfo2.setShape(StandardLFO::Shape::Sine);
            lfo2.setPhaseOffset(0.5f); // 180° offset
            reset();
        }
        void reset()
        {
            std::fill(delay1.begin(), delay1.end(), 0.0f);
            std::fill(delay2.begin(), delay2.end(), 0.0f);
            write1 = write2 = 0;
        }

        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          int activeMode)
        {
            activeMode = juce::jlimit(0, 3, activeMode);
            float depthMs = kModeDepth[activeMode];
            float rateHz  = kModeRate[activeMode];
            lfo1.setRate(rateHz, static_cast<float>(sr));
            lfo2.setRate(rateHz, static_cast<float>(sr));

            float srF = static_cast<float>(sr);
            int bufSize = static_cast<int>(delay1.size());

            for (int i = 0; i < numSamples; ++i)
            {
                float mod1 = (lfo1.process() * 0.5f + 0.5f) * depthMs * srF / 1000.0f;
                float mod2 = (lfo2.process() * 0.5f + 0.5f) * depthMs * srF / 1000.0f;
                mod1 = std::min(mod1, static_cast<float>(bufSize) - 2.0f);
                mod2 = std::min(mod2, static_cast<float>(bufSize) - 2.0f);

                delay1[write1] = monoIn[i];
                delay2[write2] = monoIn[i];

                auto readLerp = [&](const std::vector<float>& buf, int wp, float d) -> float {
                    int sz = static_cast<int>(buf.size());
                    float rF = static_cast<float>(wp) - d;
                    while (rF < 0.0f) rF += static_cast<float>(sz);
                    int r0 = static_cast<int>(rF) % sz;
                    int r1 = (r0 + 1) % sz;
                    float fr = rF - static_cast<float>(r0);
                    return buf[r0] * (1.0f - fr) + buf[r1] * fr;
                };

                L[i] = readLerp(delay1, write1, mod1);
                R[i] = readLerp(delay2, write2, mod2);

                write1 = (write1 + 1) % bufSize;
                write2 = (write2 + 1) % bufSize;
            }
        }
    } dimChorus_;

    //==========================================================================
    // Stage 4 — Degrading BBD Delay (Boss DM-1)
    // LP filter + fastTanh saturation in feedback path
    //==========================================================================
    struct BBDDegradeStage
    {
        static constexpr float kMaxDelayMs = 600.0f;
        std::vector<float> delayBuf;
        int writePos = 0;
        CytomicSVF feedbackLP;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 4;
            delayBuf.assign(maxSamples, 0.0f);
            reset();
        }
        void reset()
        {
            std::fill(delayBuf.begin(), delayBuf.end(), 0.0f);
            writePos = 0;
            feedbackLP.reset();
        }

        float processSample(float in, float delayMs, float fbAmount,
                            float compound, float gritHz)
        {
            int bufSize = static_cast<int>(delayBuf.size());
            float delaySamples = std::min(delayMs * static_cast<float>(sr) / 1000.0f,
                                          static_cast<float>(bufSize) - 2.0f);
            delaySamples = std::max(1.0f, delaySamples);

            float rF = static_cast<float>(writePos) - delaySamples;
            while (rF < 0.0f) rF += static_cast<float>(bufSize);
            int r0 = static_cast<int>(rF) % bufSize;
            int r1 = (r0 + 1) % bufSize;
            float fr = rF - static_cast<float>(r0);
            float wet = delayBuf[r0] * (1.0f - fr) + delayBuf[r1] * fr;

            feedbackLP.setMode(CytomicSVF::Mode::LowPass);
            feedbackLP.setCoefficients(gritHz, 0.707f, static_cast<float>(sr));
            float fbProcessed = feedbackLP.processSample(wet);
            fbProcessed = fastTanh(fbProcessed * (1.0f + compound));

            delayBuf[writePos] = flushDenormal(in + fbProcessed * fbAmount);
            writePos = (writePos + 1) % bufSize;

            return wet;
        }
    } bbd_;

    //==========================================================================
    // Stage 5 — Industrial Multi-Tap Reverb (Death By Audio Rooms)
    // 8 prime-spaced taps + cross-feedback + hard clipper at input
    //==========================================================================
    struct IndustrialReverbStage
    {
        static constexpr int kNumTaps = 8;
        static constexpr int kPrimeLengths[kNumTaps] = { 557, 743, 1021, 1327, 1847, 2503, 3251, 4127 };

        std::array<std::vector<float>, kNumTaps> tapBufs;
        std::array<int, kNumTaps> writePos{};
        std::array<float, kNumTaps> tapState{};
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            float srScale = static_cast<float>(sampleRate) / 48000.0f;
            for (int t = 0; t < kNumTaps; ++t)
            {
                int len = static_cast<int>(kPrimeLengths[t] * srScale) + 2;
                tapBufs[t].assign(len, 0.0f);
                writePos[t] = 0;
                tapState[t] = 0.0f;
            }
        }
        void reset()
        {
            for (int t = 0; t < kNumTaps; ++t)
            {
                std::fill(tapBufs[t].begin(), tapBufs[t].end(), 0.0f);
                writePos[t] = 0;
                tapState[t] = 0.0f;
            }
        }

        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          float decay, float mix)
        {
            if (mix < 0.001f)
            {
                for (int i = 0; i < numSamples; ++i) { L[i] = monoIn[i]; R[i] = monoIn[i]; }
                return;
            }

            float decayCoeff = fastExp(-1.0f / (decay * static_cast<float>(sr)));
            float srScale = static_cast<float>(sr) / 48000.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                float clippedIn = juce::jlimit(-0.95f, 0.95f, monoIn[i]);

                float tapOut[kNumTaps];
                float crossFeedback = 0.0f;
                for (int t = 0; t < kNumTaps; ++t)
                {
                    int len = static_cast<int>(tapBufs[t].size());
                    int effectiveDelay = static_cast<int>(kPrimeLengths[t] * srScale);
                    effectiveDelay = juce::jlimit(1, len - 1, effectiveDelay);
                    int rp = ((writePos[t] - effectiveDelay) + len) % len;
                    tapState[t] = flushDenormal(
                        tapState[t] * decayCoeff + tapBufs[t][rp] * (1.0f - decayCoeff));
                    tapOut[t] = tapState[t];
                    crossFeedback += tapOut[t];
                }
                crossFeedback /= static_cast<float>(kNumTaps);

                for (int t = 0; t < kNumTaps; ++t)
                {
                    int len = static_cast<int>(tapBufs[t].size());
                    tapBufs[t][writePos[t]] = flushDenormal(
                        clippedIn + crossFeedback * 0.1f * decayCoeff);
                    writePos[t] = (writePos[t] + 1) % len;
                }

                // Even taps → L, odd taps → R
                float wetL = 0.0f, wetR = 0.0f;
                for (int t = 0; t < kNumTaps; ++t)
                {
                    if (t % 2 == 0) wetL += tapOut[t];
                    else            wetR += tapOut[t];
                }
                wetL /= static_cast<float>(kNumTaps / 2);
                wetR /= static_cast<float>(kNumTaps / 2);

                L[i] = monoIn[i] + mix * (wetL - monoIn[i]);
                R[i] = monoIn[i] + mix * (wetR - monoIn[i]);
            }
        }
    } indRev_;

    //==========================================================================
    // Cached parameter pointers (20 params)
    //==========================================================================
    std::atomic<float>* p_pllGlide      = nullptr;
    std::atomic<float>* p_pllSub1       = nullptr;
    std::atomic<float>* p_pllSub2       = nullptr;
    std::atomic<float>* p_pllSquare     = nullptr;
    std::atomic<float>* p_odDrive       = nullptr;
    std::atomic<float>* p_odFatFreq     = nullptr;
    std::atomic<float>* p_odFatRes      = nullptr;
    std::atomic<float>* p_odMuscleFreq  = nullptr;
    std::atomic<float>* p_odMuscleGain  = nullptr;
    std::atomic<float>* p_odLevel       = nullptr;
    std::atomic<float>* p_dimMode1      = nullptr;
    std::atomic<float>* p_dimMode2      = nullptr;
    std::atomic<float>* p_dimMode3      = nullptr;
    std::atomic<float>* p_dimMode4      = nullptr;
    std::atomic<float>* p_dlvTime       = nullptr;
    std::atomic<float>* p_dlvFeedback   = nullptr;
    std::atomic<float>* p_dlvCompound   = nullptr;
    std::atomic<float>* p_dlvGrit       = nullptr;
    std::atomic<float>* p_revSize       = nullptr;
    std::atomic<float>* p_revMix        = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void ObscurityChain::prepare(double sampleRate, int /*maxBlockSize*/)
{
    sr_ = sampleRate;
    pll_.prepare(sampleRate);
    diode_.prepare(sampleRate);
    dimChorus_.prepare(sampleRate);
    bbd_.prepare(sampleRate);
    indRev_.prepare(sampleRate);
}

inline void ObscurityChain::reset()
{
    pll_.reset();
    diode_.reset();
    dimChorus_.reset();
    bbd_.reset();
    indRev_.reset();
}

inline void ObscurityChain::processBlock(const float* monoIn, float* L, float* R, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (!p_pllGlide) return;

    const float pllGlide     = p_pllGlide->load(std::memory_order_relaxed);
    const float pllSub1      = p_pllSub1->load(std::memory_order_relaxed);
    const float pllSub2      = p_pllSub2->load(std::memory_order_relaxed);
    const float pllSquare    = p_pllSquare->load(std::memory_order_relaxed);
    const float odDrive      = p_odDrive->load(std::memory_order_relaxed);
    const float odFatFreq    = p_odFatFreq->load(std::memory_order_relaxed);
    const float odFatRes     = p_odFatRes->load(std::memory_order_relaxed);
    const float odMuscleFreq = p_odMuscleFreq->load(std::memory_order_relaxed);
    const float odMuscleGain = p_odMuscleGain->load(std::memory_order_relaxed);
    const float odLevel      = p_odLevel->load(std::memory_order_relaxed);
    const float dimMode2     = p_dimMode2->load(std::memory_order_relaxed);
    const float dimMode3     = p_dimMode3->load(std::memory_order_relaxed);
    const float dlvTime      = p_dlvTime->load(std::memory_order_relaxed);
    const float dlvFeedback  = p_dlvFeedback->load(std::memory_order_relaxed);
    const float dlvCompound  = p_dlvCompound->load(std::memory_order_relaxed);
    const float dlvGrit      = p_dlvGrit->load(std::memory_order_relaxed);
    const float revSize      = p_revSize->load(std::memory_order_relaxed);
    const float revMix       = p_revMix->load(std::memory_order_relaxed);

    // Resolve active DC-2 dimension mode (exclusive bool → int 0-3)
    int activeMode = 0;
    if (dimMode2 > 0.5f) activeMode = 1;
    if (dimMode3 > 0.5f) activeMode = 2;
    if (p_dimMode4 && p_dimMode4->load(std::memory_order_relaxed) > 0.5f) activeMode = 3;

    float odDriveLin = fastPow2(odDrive * (1.0f / 6.0205999f));

    // Stage 1: PLL + Stage 2: Diode OD (mono pipeline)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = pll_.process(monoIn[i], pllGlide, pllSub1, pllSub2, pllSquare, sr_);
        x = diode_.process(x, odDriveLin, odFatFreq, odFatRes,
                           odMuscleFreq, odMuscleGain, odLevel);
        L[i] = x;
    }

    // Stage 3: Dimension Chorus — Mono → Stereo
    dimChorus_.processBlock(L, L, R, numSamples, activeMode);

    // Stage 4: BBD Delay (both channels separately)
    for (int i = 0; i < numSamples; ++i)
    {
        float wetL = bbd_.processSample(L[i], dlvTime, dlvFeedback * 0.01f,
                                         dlvCompound, dlvGrit);
        float wetR = bbd_.processSample(R[i], dlvTime, dlvFeedback * 0.01f,
                                         dlvCompound, dlvGrit);
        L[i] = wetL; R[i] = wetR;
    }

    // Stage 5: Industrial Reverb (per-sample mono sum → stereo)
    // Temporarily use a single-sample array per call to stay allocation-free
    float monoMix[1], lArr[1], rArr[1];
    for (int i = 0; i < numSamples; ++i)
    {
        monoMix[0] = (L[i] + R[i]) * 0.5f;
        indRev_.processBlock(monoMix, lArr, rArr, 1, revSize, revMix * 0.01f);
        L[i] = lArr[0]; R[i] = rArr[0];
    }
}

inline void ObscurityChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    using AP = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    const juce::String p = slotPrefix + "obsc_";

    layout.add(std::make_unique<AP>(juce::ParameterID{p + "pllGlide",     1}, p + "PLL Glide",
        NR(0.0f, 200.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "pllSub1",      1}, p + "PLL Sub1",
        NR(-60.0f, 0.0f, 0.1f), -12.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "pllSub2",      1}, p + "PLL Sub2",
        NR(-60.0f, 0.0f, 0.1f), -18.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "pllSquare",    1}, p + "PLL Square",
        NR(-60.0f, 0.0f, 0.1f), -6.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odDrive",      1}, p + "OD Drive",
        NR(0.0f, 40.0f, 0.1f), 12.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odFatFreq",    1}, p + "OD Fat Freq",
        NR(50.0f, 2000.0f, 1.0f), 200.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odFatRes",     1}, p + "OD Fat Res",
        NR(0.5f, 5.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odMuscleFreq", 1}, p + "OD Muscle Freq",
        NR(500.0f, 8000.0f, 1.0f), 2000.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odMuscleGain", 1}, p + "OD Muscle Gain",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "odLevel",      1}, p + "OD Level",
        NR(-18.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dimMode1",     1}, p + "Dim Mode1",
        NR(0.0f, 1.0f, 1.0f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dimMode2",     1}, p + "Dim Mode2",
        NR(0.0f, 1.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dimMode3",     1}, p + "Dim Mode3",
        NR(0.0f, 1.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dimMode4",     1}, p + "Dim Mode4",
        NR(0.0f, 1.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dlvTime",      1}, p + "DLV Time",
        NR(10.0f, 500.0f, 1.0f), 200.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dlvFeedback",  1}, p + "DLV Feedback",
        NR(0.0f, 85.0f, 0.1f), 40.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dlvCompound",  1}, p + "DLV Compound",
        NR(0.0f, 4.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "dlvGrit",      1}, p + "DLV Grit",
        NR(200.0f, 8000.0f, 1.0f), 1500.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revSize",      1}, p + "Rev Size",
        NR(0.0f, 2.0f, 0.001f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revMix",       1}, p + "Rev Mix",
        NR(0.0f, 100.0f, 0.1f), 0.0f));
}

inline void ObscurityChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "obsc_";
    p_pllGlide      = apvts.getRawParameterValue(p + "pllGlide");
    p_pllSub1       = apvts.getRawParameterValue(p + "pllSub1");
    p_pllSub2       = apvts.getRawParameterValue(p + "pllSub2");
    p_pllSquare     = apvts.getRawParameterValue(p + "pllSquare");
    p_odDrive       = apvts.getRawParameterValue(p + "odDrive");
    p_odFatFreq     = apvts.getRawParameterValue(p + "odFatFreq");
    p_odFatRes      = apvts.getRawParameterValue(p + "odFatRes");
    p_odMuscleFreq  = apvts.getRawParameterValue(p + "odMuscleFreq");
    p_odMuscleGain  = apvts.getRawParameterValue(p + "odMuscleGain");
    p_odLevel       = apvts.getRawParameterValue(p + "odLevel");
    p_dimMode1      = apvts.getRawParameterValue(p + "dimMode1");
    p_dimMode2      = apvts.getRawParameterValue(p + "dimMode2");
    p_dimMode3      = apvts.getRawParameterValue(p + "dimMode3");
    p_dimMode4      = apvts.getRawParameterValue(p + "dimMode4");
    p_dlvTime       = apvts.getRawParameterValue(p + "dlvTime");
    p_dlvFeedback   = apvts.getRawParameterValue(p + "dlvFeedback");
    p_dlvCompound   = apvts.getRawParameterValue(p + "dlvCompound");
    p_dlvGrit       = apvts.getRawParameterValue(p + "dlvGrit");
    p_revSize       = apvts.getRawParameterValue(p + "revSize");
    p_revMix        = apvts.getRawParameterValue(p + "revMix");
}

} // namespace xoceanus
