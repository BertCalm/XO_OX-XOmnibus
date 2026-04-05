// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "FXChainHelpers.h"
#include "Saturator.h"
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../PolyBLEP.h"
#include "../ParameterSmoother.h"
#include "../StandardLFO.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OculusChain — OCULUS Sentient Grid FX Chain (4 stages)
//
// Concept: Gamechanger Audio Plasma × Line 6 FM4 × Mu-Tron Bi-Phase × Meris Polymoon
// Routing: Mono In → Stereo Out (scatter at Stage 4 Polymoon multi-tap delay)
//
// Stage 1: Plasma Distortion (Gamechanger Audio Plasma) — SchmittTrigger hard
//          gate → extreme hard-clipping, OversamplingProcessor<8>
// Stage 2: Sequenced Formant Filter (Line 6 FM4) — 3 parallel CytomicSVF
//          BandPass at vowel formant frequencies, step-sequenced to host BPM
// Stage 3: Dual Optical Phaser (Mu-Tron Bi-Phase) — two 6-stage CytomicSVF
//          AllPass cascades, L/R hard-pan, independent LFOs with slew limiting
// Stage 4: Cascading Multi-Tap Delay (Meris Polymoon) — FractionalDelay with
//          up to 6 prime-spaced read pointers + AllPass diffusion → Stereo
//
// Parameter prefix: ocul_ (13 params)
//==============================================================================
class OculusChain
{
public:
    OculusChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process mono input, writing stereo to L and R.
    // Caller must ensure L != R (separate output buffers).
    void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                      double bpm = 120.0, double ppqPosition = -1.0);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_        = 44100.0;
    int    blockSize_ = 512;

    //==========================================================================
    // Stage 1 — Plasma Distortion (Gamechanger Audio Plasma)
    // SchmittTrigger hard gate → extreme hard-clipping at low threshold
    // OversamplingProcessor<8> for alias-free square-wave bursts
    //==========================================================================
    struct PlasmaStage
    {
        SchmittTrigger           trigger;
        OversamplingProcessor<8> ovs;
        bool                     gateOpen = false;

        void prepare(double sampleRate, int maxBlockSize)
        {
            ovs.prepare(sampleRate, maxBlockSize);
            trigger.reset();
            gateOpen = false;
        }

        void reset()
        {
            ovs.reset();
            trigger.reset();
            gateOpen = false;
        }

        // Apply plasma to a block in-place via 8x OVS for alias-free square bursts.
        // voltageAmt: 0=gentle, 1=full plasma (maps clip threshold 0.5→0.02)
        void processBlock(float* buf, int numSamples, float voltageAmt)
        {
            float thresh = 0.5f - voltageAmt * 0.48f; // 0.5→0.02
            float threshLow  = thresh * 0.7f;
            float threshHigh = thresh;
            trigger.setThresholds(threshLow, threshHigh);

            float clipThresh = 0.05f + (1.0f - voltageAmt) * 0.5f;
            float invClip    = (clipThresh > 1e-6f) ? (1.0f / clipThresh) : 0.0f;

            // Apply Schmitt trigger + hard clip at 8x oversampled rate
            ovs.process(buf, numSamples, [&](float* upBuf, int upN)
            {
                for (int i = 0; i < upN; ++i)
                {
                    bool fired = trigger.process(upBuf[i]);
                    if (fired) gateOpen = !gateOpen;
                    float gated = gateOpen ? upBuf[i] : 0.0f;
                    float clipped = std::max(-clipThresh, std::min(clipThresh, gated));
                    upBuf[i] = clipped * invClip;
                }
            });
        }
    } plasma_;

    //==========================================================================
    // Stage 2 — Sequenced Formant Filter (Line 6 FM4)
    // 3 parallel BandPass SVFs at vowel formant frequencies
    // Step-sequencer cycles through vowel sets synced to BPM
    //==========================================================================
    struct FormantFilterStage
    {
        // 5 vowels × 3 formants (F1, F2, F3) in Hz
        // A, E, I, O, U (approximate average male/female)
        static constexpr float kVowelFormants[5][3] = {
            {  800.0f, 1200.0f, 2800.0f }, // A
            {  400.0f, 1800.0f, 2500.0f }, // E
            {  300.0f, 2200.0f, 3000.0f }, // I
            {  500.0f,  900.0f, 2700.0f }, // O
            {  350.0f,  850.0f, 2200.0f }, // U
        };

        CytomicSVF   bandF1;
        CytomicSVF   bandF2;
        CytomicSVF   bandF3;

        // Step sequencer state
        int   currentVowelA    = 0;
        int   currentVowelB    = 1;  // next vowel for morphing
        float vowelMorph       = 0.0f;  // 0=A, 1=B
        float stepAccum        = 0.0f;
        int   numSteps         = 4;     // 4, 6, 8, or 16
        double sr              = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            bandF1.setMode(CytomicSVF::Mode::BandPass);
            bandF2.setMode(CytomicSVF::Mode::BandPass);
            bandF3.setMode(CytomicSVF::Mode::BandPass);
            // Initial coefficients
            float srF = static_cast<float>(sampleRate);
            bandF1.setCoefficients(kVowelFormants[0][0], 0.7f, srF);
            bandF2.setCoefficients(kVowelFormants[0][1], 0.6f, srF);
            bandF3.setCoefficients(kVowelFormants[0][2], 0.5f, srF);
            stepAccum = 0.0f;
            currentVowelA = 0;
            currentVowelB = 1;
            vowelMorph    = 0.0f;
        }

        void reset()
        {
            bandF1.reset();
            bandF2.reset();
            bandF3.reset();
            stepAccum = 0.0f;
        }

        float process(float in, int steps, float seekSpeed, float morphAmt, double bpm)
        {
            float srF = static_cast<float>(sr);
            numSteps = steps;

            // Advance step at seek speed (in BPM-sync'd 16th notes)
            // seekSpeed multiplies the step rate
            float beatsPerSec   = static_cast<float>(bpm) / 60.0f;
            float stepSamples   = srF / (beatsPerSec * 4.0f * seekSpeed);
            stepSamples = std::max(stepSamples, 32.0f);

            stepAccum += 1.0f;
            if (stepAccum >= stepSamples)
            {
                stepAccum -= stepSamples;
                currentVowelA = (currentVowelA + 1) % numSteps;
                currentVowelA = std::min(currentVowelA, 4); // clamp to valid vowel index
                currentVowelB = (currentVowelA + 1) % 5;
                vowelMorph    = 0.0f; // reset morph on step advance
            }

            // Advance morph continuously within step
            vowelMorph = flushDenormal(vowelMorph + (1.0f / stepSamples));
            vowelMorph = std::min(vowelMorph, 1.0f);

            // Interpolate between vowel A and vowel B formant frequencies
            int vA = std::min(currentVowelA, 4);
            int vB = std::min(currentVowelB, 4);
            float mt = vowelMorph * morphAmt;

            float f1 = kVowelFormants[vA][0] + mt * (kVowelFormants[vB][0] - kVowelFormants[vA][0]);
            float f2 = kVowelFormants[vA][1] + mt * (kVowelFormants[vB][1] - kVowelFormants[vA][1]);
            float f3 = kVowelFormants[vA][2] + mt * (kVowelFormants[vB][2] - kVowelFormants[vA][2]);

            bandF1.setCoefficients(f1, 0.7f, srF);
            bandF2.setCoefficients(f2, 0.6f, srF);
            bandF3.setCoefficients(f3, 0.5f, srF);

            float out1 = bandF1.processSample(in);
            float out2 = bandF2.processSample(in);
            float out3 = bandF3.processSample(in);

            return flushDenormal((out1 + out2 * 0.8f + out3 * 0.5f) / 2.3f);
        }
    } formant_;

    //==========================================================================
    // Stage 3 — Dual Optical Phaser (Mu-Tron Bi-Phase)
    // Two 6-stage CytomicSVF AllPass cascades, L/R hard-pan
    // Independent LFOs with slew-rate limiting via ParameterSmoother
    //==========================================================================
    struct BiphaseStage
    {
        static constexpr int kStages = 6;

        CytomicSVF     apL[kStages];   // Left channel allpass cascade
        CytomicSVF     apR[kStages];   // Right channel allpass cascade
        StandardLFO    lfoL;
        StandardLFO    lfoR;
        ParameterSmoother freqSmootherL;
        ParameterSmoother freqSmootherR;
        double         sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            float srF = static_cast<float>(sampleRate);
            for (int i = 0; i < kStages; ++i)
            {
                apL[i].setMode(CytomicSVF::Mode::AllPass);
                apL[i].setCoefficients(1000.0f, 0.5f, srF);
                apR[i].setMode(CytomicSVF::Mode::AllPass);
                apR[i].setCoefficients(1000.0f, 0.5f, srF);
            }
            lfoL.setShape(StandardLFO::Sine);
            lfoR.setShape(StandardLFO::Sine);
            lfoL.setPhaseOffset(0.0f);
            lfoR.setPhaseOffset(0.5f); // 180° offset for bi-phase effect
            freqSmootherL.prepare(srF, 0.010f);
            freqSmootherR.prepare(srF, 0.010f);
            freqSmootherL.snapTo(1000.0f);
            freqSmootherR.snapTo(1000.0f);
        }

        void reset()
        {
            for (int i = 0; i < kStages; ++i)
            {
                apL[i].reset();
                apR[i].reset();
            }
            freqSmootherL.snapTo(1000.0f);
            freqSmootherR.snapTo(1000.0f);
        }

        void process(float in, float& outL, float& outR,
                     float rateA, float rateB, bool synced, float depth)
        {
            float srF = static_cast<float>(sr);

            float lfoRateA = rateA;
            float lfoRateB = synced ? rateA * 1.618f : rateB; // golden ratio offset if synced

            lfoL.setRate(lfoRateA, srF);
            lfoR.setRate(lfoRateB, srF);

            // LFO output → sweep allpass center frequency
            // Range: 100Hz to 4kHz
            float modL = lfoL.process() * depth;
            float modR = lfoR.process() * depth;

            float centerFreq = 1000.0f;
            float sweepRange = 1500.0f;

            float targetL = centerFreq + modL * sweepRange;
            float targetR = centerFreq + modR * sweepRange;
            targetL = std::max(100.0f, std::min(targetL, srF * 0.4f));
            targetR = std::max(100.0f, std::min(targetR, srF * 0.4f));

            // Slew-rate limit via ParameterSmoother
            freqSmootherL.set(targetL);
            freqSmootherR.set(targetR);
            float smoothedL = freqSmootherL.process();
            float smoothedR = freqSmootherR.process();

            // Cascade 6 allpass stages per channel
            float xL = in;
            float xR = in;
            for (int i = 0; i < kStages; ++i)
            {
                apL[i].setCoefficients(smoothedL + static_cast<float>(i) * 200.0f, 0.5f, srF);
                apR[i].setCoefficients(smoothedR + static_cast<float>(i) * 200.0f, 0.5f, srF);
                xL = apL[i].processSample(xL);
                xR = apR[i].processSample(xR);
            }

            // Hard-pan: L only gets left cascade, R only gets right cascade
            outL = flushDenormal(in * 0.5f + xL * 0.5f);
            outR = flushDenormal(in * 0.5f + xR * 0.5f);
        }
    } biphase_;

    //==========================================================================
    // Stage 4 — Cascading Multi-Tap Delay (Meris Polymoon)
    // Single FractionalDelay with up to 6 prime-spaced read pointers
    // AllPass diffusion on each tap + alternating L/R pan per tap → Stereo
    //==========================================================================
    struct PolymoonStage
    {
        static constexpr float kMaxDelayMs = 2000.0f;
        static constexpr int   kMaxTaps    = 6;

        // Prime multiplier offsets for the 6 taps (relative to base delay)
        // Each tap is at base × prime/7 to avoid combing between taps
        static constexpr int kPrimeMults[6] = { 1, 2, 3, 5, 7, 11 };

        FractionalDelay mainDelay;
        CytomicSVF      tapDiffusers[kMaxTaps]; // allpass per tap
        float           feedback    = 0.0f;
        double          sr          = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            mainDelay.prepare(maxSamples);
            feedback = 0.0f;

            float srF = static_cast<float>(sampleRate);
            for (int i = 0; i < kMaxTaps; ++i)
            {
                tapDiffusers[i].setMode(CytomicSVF::Mode::AllPass);
                tapDiffusers[i].setCoefficients(500.0f + static_cast<float>(i) * 300.0f, 0.5f, srF);
            }
        }

        void reset()
        {
            mainDelay.clear();
            feedback = 0.0f;
            for (int i = 0; i < kMaxTaps; ++i)
                tapDiffusers[i].reset();
        }

        void process(float in, float& outL, float& outR,
                     float baseTimeMs, int numTaps, float dimension, float fbAmt)
        {
            float srF = static_cast<float>(sr);

            // Write with feedback
            mainDelay.write(in + feedback * fbAmt);

            float sumL = 0.0f;
            float sumR = 0.0f;
            int   activeTaps = std::max(1, std::min(numTaps, kMaxTaps));

            for (int t = 0; t < activeTaps; ++t)
            {
                // Each tap at prime-spaced offset
                float tapTimeMs = baseTimeMs * static_cast<float>(kPrimeMults[t]) * (1.0f / 7.0f);
                tapTimeMs = std::max(1.0f, std::min(tapTimeMs, kMaxDelayMs));
                float tapSamples = tapTimeMs * srF * 0.001f;
                tapSamples = std::max(1.0f, std::min(tapSamples, static_cast<float>(mainDelay.getSize()) - 2.0f));

                float tapOut = mainDelay.read(tapSamples);

                // AllPass diffusion
                float srFi = srF;
                tapDiffusers[t].setCoefficients(500.0f + static_cast<float>(t) * 300.0f * dimension,
                                                0.5f, srFi);
                tapOut = tapDiffusers[t].processSample(tapOut);
                tapOut = flushDenormal(tapOut);

                // Gain envelope: each tap slightly quieter
                float tapGain = 1.0f / static_cast<float>(t + 1);

                // Alternating L/R pan
                if (t % 2 == 0)
                    sumL += tapOut * tapGain;
                else
                    sumR += tapOut * tapGain;
            }

            // Normalize
            float normL = (activeTaps > 1) ? 2.0f / static_cast<float>(activeTaps) : 1.0f;
            float normR = normL;
            sumL *= normL;
            sumR *= normR;

            feedback = flushDenormal((sumL + sumR) * 0.5f);

            outL = sumL;
            outR = sumR;
        }
    } polymoon_;

    //==========================================================================
    // Cached parameter pointers (13 params)
    //==========================================================================
    std::atomic<float>* p_plasmaVoltage   = nullptr;
    std::atomic<float>* p_plasmaBlend     = nullptr;
    std::atomic<float>* p_seekSteps       = nullptr;
    std::atomic<float>* p_seekSpeed       = nullptr;
    std::atomic<float>* p_seekVowelMorph  = nullptr;
    std::atomic<float>* p_biphaseRateA    = nullptr;
    std::atomic<float>* p_biphaseRateB    = nullptr;
    std::atomic<float>* p_biphaseSync     = nullptr;
    std::atomic<float>* p_biphaseDepth    = nullptr;
    std::atomic<float>* p_polyTime        = nullptr;
    std::atomic<float>* p_polyTaps        = nullptr;
    std::atomic<float>* p_polyDimension   = nullptr;
    std::atomic<float>* p_polyFeedback    = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OculusChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    plasma_.prepare(sampleRate, maxBlockSize);
    formant_.prepare(sampleRate);
    biphase_.prepare(sampleRate);
    polymoon_.prepare(sampleRate);
}

inline void OculusChain::reset()
{
    plasma_.reset();
    formant_.reset();
    biphase_.reset();
    polymoon_.reset();
}

inline void OculusChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double bpm, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_plasmaVoltage) return;

    // ParamSnapshot: read all params once
    const float plasmaVoltage  = p_plasmaVoltage->load(std::memory_order_relaxed);
    const float plasmaBlend    = p_plasmaBlend->load(std::memory_order_relaxed);
    const int   seekSteps      = static_cast<int>(p_seekSteps->load(std::memory_order_relaxed));
    const float seekSpeed      = p_seekSpeed->load(std::memory_order_relaxed);
    const float seekVowelMorph = p_seekVowelMorph->load(std::memory_order_relaxed);
    const float biphaseRateA   = p_biphaseRateA->load(std::memory_order_relaxed);
    const float biphaseRateB   = p_biphaseRateB->load(std::memory_order_relaxed);
    const bool  biphaseSync    = p_biphaseSync->load(std::memory_order_relaxed) >= 0.5f;
    const float biphaseDepth   = p_biphaseDepth->load(std::memory_order_relaxed);
    const float polyTime       = p_polyTime->load(std::memory_order_relaxed);
    const int   polyTaps       = static_cast<int>(p_polyTaps->load(std::memory_order_relaxed));
    const float polyDimension  = p_polyDimension->load(std::memory_order_relaxed);
    const float polyFeedback   = p_polyFeedback->load(std::memory_order_relaxed);

    if (bpm <= 0.0) bpm = 120.0;

    // Map seekSteps choice index to actual step count
    // Choice order: 0=4, 1=6, 2=8, 3=16
    static const int kStepChoices[4] = { 4, 6, 8, 16 };
    int actualSteps = kStepChoices[std::max(0, std::min(seekSteps, 3))];

    // Map polyTaps choice (0=1 tap, 5=6 taps) — stored as 1-indexed float
    int actualTaps = std::max(1, std::min(polyTaps, 6));

    // Stage 1: Plasma Distortion — copy to L buffer, process block via OVS
    for (int i = 0; i < numSamples; ++i)
        L[i] = monoIn[i];

    // Apply plasma in-place on L (uses OVS internally for 8x oversampling)
    plasma_.processBlock(L, numSamples, plasmaVoltage);

    // Blend plasma with dry and run stage 2 per-sample
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i] + plasmaBlend * (L[i] - monoIn[i]);

        // Stage 2: Sequenced Formant Filter
        x = formant_.process(x, actualSteps, seekSpeed, seekVowelMorph, bpm);

        L[i] = x;
    }

    // Stage 3: Dual Optical Phaser — still mono pass-through to L, but computes stereo
    for (int i = 0; i < numSamples; ++i)
    {
        float pL, pR;
        biphase_.process(L[i], pL, pR, biphaseRateA, biphaseRateB, biphaseSync, biphaseDepth);
        L[i] = pL;
        R[i] = pR;
    }

    // Stage 4: Multi-Tap Delay — processes stereo to stereo via mono sum input
    for (int i = 0; i < numSamples; ++i)
    {
        float monoMix = (L[i] + R[i]) * 0.5f;
        float dL, dR;
        polymoon_.process(monoMix, dL, dR, polyTime, actualTaps, polyDimension, polyFeedback);
        // Blend delay output with phaser output
        L[i] = L[i] * 0.5f + dL * 0.5f;
        R[i] = R[i] * 0.5f + dR * 0.5f;
    }
}

inline void OculusChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "ocul_";

    registerFloat(layout, p + "plasmaVoltage",  p + "Plasma Voltage",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "plasmaBlend",    p + "Plasma Blend",
                  0.0f, 1.0f, 0.6f, 0.001f);
    registerChoice(layout, p + "seekSteps",     p + "Seek Steps",
                  juce::StringArray{"4", "6", "8", "16"}, 0);
    registerFloat(layout, p + "seekSpeed",      p + "Seek Speed",
                  0.1f, 4.0f, 1.0f, 0.001f);
    registerFloat(layout, p + "seekVowelMorph", p + "Seek Vowel Morph",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloatSkewed(layout, p + "biphaseRateA", p + "Biphase Rate A",
                  0.01f, 10.0f, 0.5f, 0.001f, 0.4f);
    registerFloatSkewed(layout, p + "biphaseRateB", p + "Biphase Rate B",
                  0.01f, 10.0f, 0.7f, 0.001f, 0.4f);
    registerBool(layout,  p + "biphaseSync",    p + "Biphase Sync", false);
    registerFloat(layout, p + "biphaseDepth",   p + "Biphase Depth",
                  0.0f, 1.0f, 0.7f, 0.001f);
    registerFloatSkewed(layout, p + "polyTime", p + "Poly Time",
                  1.0f, 2000.0f, 300.0f, 1.0f, 0.4f);
    registerChoice(layout, p + "polyTaps",      p + "Poly Taps",
                  juce::StringArray{"1", "2", "3", "4", "5", "6"}, 3);
    registerFloat(layout, p + "polyDimension",  p + "Poly Dimension",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "polyFeedback",   p + "Poly Feedback",
                  0.0f, 0.94f, 0.4f, 0.001f);
}

inline void OculusChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "ocul_";
    p_plasmaVoltage  = cacheParam(apvts, p + "plasmaVoltage");
    p_plasmaBlend    = cacheParam(apvts, p + "plasmaBlend");
    p_seekSteps      = cacheParam(apvts, p + "seekSteps");
    p_seekSpeed      = cacheParam(apvts, p + "seekSpeed");
    p_seekVowelMorph = cacheParam(apvts, p + "seekVowelMorph");
    p_biphaseRateA   = cacheParam(apvts, p + "biphaseRateA");
    p_biphaseRateB   = cacheParam(apvts, p + "biphaseRateB");
    p_biphaseSync    = cacheParam(apvts, p + "biphaseSync");
    p_biphaseDepth   = cacheParam(apvts, p + "biphaseDepth");
    p_polyTime       = cacheParam(apvts, p + "polyTime");
    p_polyTaps       = cacheParam(apvts, p + "polyTaps");
    p_polyDimension  = cacheParam(apvts, p + "polyDimension");
    p_polyFeedback   = cacheParam(apvts, p + "polyFeedback");
}

} // namespace xoceanus
