// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <random>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// ObliterateChain — OBLITERATE Heavy Stutter FX Chain (5 stages)
//
// Source concept: GritGlitch (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 4 granular scatter)
// Accent: Apocalypse Red #8B0000
//
// Stage 1: Shimmer Reverb (OBNE Dark Star) — mini 4-tap FDN + PSOLA pitch shifter
// Stage 2: Octave Fuzz (Boss FZ-2) — abs(x) rectification + 8x OVS + Baxandall EQ
// Stage 3: Reverse Pitch Delay (Boss PS-3) — dual circular buffers + Hann crossfade
// Stage 4: Granular Looper (Hologram Microcosm) — GrainScheduler + stochastic pan
// Stage 5: Hard-Chopped Tremolo (Boss PN-2) — shape-select LFO + ping-pong L/R
//
// Parameter prefix: oblt_ (19 params)
//==============================================================================
class ObliterateChain
{
public:
    ObliterateChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // monoIn: single channel input. L/R: stereo output.
    void processBlock(const float* monoIn, float* L, float* R, int numSamples);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_ = 0.0;  // Sentinel: must be set by prepare() before use
    // NOTE: Stage tap lengths reference 48kHz as the intentional scaling target.
    // kPrimeTaps in ShimmerReverbStage are at 48kHz reference, scaled at prepare time.

    //==========================================================================
    // Stage 1 — Shimmer Reverb (OBNE Dark Star)
    // 4-tap FDN + PSOLA pitch shifter in feedback
    //==========================================================================
    struct ShimmerReverbStage
    {
        static constexpr int kNumTaps = 4;
        // Prime tap lengths at 48kHz reference (scaled at prepare time)
        static constexpr int kPrimeTaps[kNumTaps] = { 1021, 1327, 1847, 2503 };

        std::array<std::vector<float>, kNumTaps> fdnBufs;
        std::array<int, kNumTaps> fdnWritePos{};
        std::array<float, kNumTaps> fdnState{};

        std::vector<float> psola1, psola2;
        int   psolaWrite1 = 0, psolaWrite2 = 0;
        float psolaPhase  = 0.0f;
        bool  psolaToggle = false;
        float psolaGrain1 = 1.0f;
        float psolaGrain2 = 0.0f;
        double sr = 0.0;  // Sentinel: set by prepare()

        void prepare(double sampleRate);
        void reset();
        float processSample(float in, float decaySec, int semitones, double sampleRate);
    } shimmer_;

    //==========================================================================
    // Stage 2 — Octave Fuzz (Boss FZ-2 Hyper Fuzz)
    // Full-wave rect + 8x oversampling + Baxandall tone stack
    //==========================================================================
    struct OctaveFuzzStage
    {
        CytomicSVF baxTreble, baxBass;
        float aaState = 0.0f;
        double sr = 0.0;  // Sentinel: set by prepare()

        void prepare(double sampleRate) { sr = sampleRate; reset(); }
        void reset() { baxTreble.reset(); baxBass.reset(); aaState = 0.0f; }

        float process(float in, float gainLinear, float trebleDb, float bassDb)
        {
            // 8x oversampling via linear interpolation
            float samples[8];
            float prev = aaState;
            for (int k = 0; k < 8; ++k)
            {
                float t = static_cast<float>(k) / 8.0f;
                samples[k] = prev + t * (in - prev);
            }
            aaState = in;

            // Full-wave rectification at 8x rate
            for (int k = 0; k < 8; ++k)
                samples[k] = std::abs(samples[k]) * gainLinear;

            // Downsample 8→1 (averaging)
            float out = 0.0f;
            for (int k = 0; k < 8; ++k) out += samples[k];
            out *= (1.0f / 8.0f);

            // Baxandall tone stack via CytomicSVF
            baxBass.setMode(CytomicSVF::Mode::LowShelf);
            baxBass.setCoefficients(200.0f, 0.707f, static_cast<float>(sr), bassDb);
            out = baxBass.processSample(out);

            baxTreble.setMode(CytomicSVF::Mode::HighShelf);
            baxTreble.setCoefficients(3000.0f, 0.707f, static_cast<float>(sr), trebleDb);
            out = baxTreble.processSample(out);

            return out;
        }
    } fuzz_;

    //==========================================================================
    // Stage 3 — Reverse Pitch Delay (Boss PS-3 Mode 7)
    // Dual circular buffers: forward write, backward read with Hann window
    //==========================================================================
    struct ReversePitchDelayStage
    {
        static constexpr float kMaxDelayMs = 2000.0f;
        std::vector<float> bufA, bufB;
        int   writeA = 0, writeB = 0;
        int   readBPos = 0;
        float hannPhase = 0.0f;
        double sr = 0.0;  // Sentinel: set by prepare()

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 4;
            bufA.assign(maxSamples, 0.0f);
            bufB.assign(maxSamples, 0.0f);
            reset();
        }
        void reset()
        {
            std::fill(bufA.begin(), bufA.end(), 0.0f);
            std::fill(bufB.begin(), bufB.end(), 0.0f);
            writeA = writeB = 0;
            readBPos = 0;
            hannPhase = 0.0f;
        }

        static float hann(float phase) { return 0.5f * (1.0f - std::cos(6.28318f * phase)); }

        float process(float in, float delayMs, float pitchRatio, float mix)
        {
            int bufSize = static_cast<int>(bufA.size());
            int loopLen = std::max(1, static_cast<int>(delayMs * static_cast<float>(sr) / 1000.0f));
            loopLen = std::min(loopLen, bufSize - 1);

            bufA[writeA] = in;
            bufB[writeB] = in;
            writeA = (writeA + 1) % bufSize;
            writeB = (writeB + 1) % bufSize;

            int backreadPos = ((writeB - readBPos + bufSize) % bufSize);
            float wetRaw  = bufB[backreadPos];
            float wetHann = wetRaw * hann(hannPhase);

            readBPos = static_cast<int>(readBPos + pitchRatio) % loopLen;
            hannPhase = std::fmod(hannPhase + (pitchRatio / static_cast<float>(loopLen)), 1.0f);

            return in + mix * (wetHann - in);
        }
    } revDelay_;

    //==========================================================================
    // Stage 4 — Granular Looper (Hologram Microcosm)
    // GrainScheduler + stochastic pan → Mono → Stereo
    //==========================================================================
    struct GranularStage
    {
        static constexpr int kMaxGrains = 24;
        static constexpr float kCaptureMs = 500.0f;

        struct Grain
        {
            int   startIdx = 0;
            int   length   = 0;
            int   pos      = 0;
            float ampL     = 1.0f;
            float ampR     = 1.0f;
            bool  active   = false;
        };

        std::array<Grain, kMaxGrains> grains{};
        std::vector<float> captureBuf;
        int captureWrite = 0;
        int captureLen   = 0;

        float spawnAccum = 0.0f;
        juce::Random rng{};
        double sr = 0.0;  // Sentinel: set by prepare()

        void prepare(double sampleRate, int /*maxBlockSize*/)
        {
            sr = sampleRate;
            captureLen = static_cast<int>(kCaptureMs * sampleRate / 1000.0);
            captureBuf.assign(captureLen, 0.0f);
            reset();
        }
        void reset()
        {
            std::fill(captureBuf.begin(), captureBuf.end(), 0.0f);
            captureWrite = 0;
            for (auto& g : grains) g.active = false;
            spawnAccum = 0.0f;
        }

        static float gaussianEnv(int pos, int length)
        {
            float t = static_cast<float>(pos) / static_cast<float>(std::max(1, length));
            return 0.5f * (1.0f - std::cos(6.28318f * t));
        }

        void spawnGrain(float grainSizeMs, float scatter)
        {
            for (auto& g : grains)
            {
                if (!g.active)
                {
                    int len = static_cast<int>(grainSizeMs * static_cast<float>(sr) / 1000.0f);
                    len = std::max(1, std::min(len, captureLen - 1));
                    int maxStart = std::max(1, captureLen - len);
                    g.startIdx = rng.nextInt(maxStart);
                    g.length   = len;
                    g.pos      = 0;
                    float pan = (rng.nextFloat() * 2.0f - 1.0f) * scatter;
                    float angle = (pan + 1.0f) * 0.5f * 1.5707963f;
                    g.ampL = std::cos(angle);
                    g.ampR = std::sin(angle);
                    g.active = true;
                    break;
                }
            }
        }

        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          float densityHz, float grainSizeMs, float scatter, float mix)
        {
            if (densityHz < 0.001f || mix < 0.001f)
            {
                for (int i = 0; i < numSamples; ++i) { L[i] = monoIn[i]; R[i] = monoIn[i]; }
                return;
            }

            float samplesPerGrain = static_cast<float>(sr) / std::max(0.01f, densityHz);

            for (int i = 0; i < numSamples; ++i)
            {
                captureBuf[captureWrite] = monoIn[i];
                captureWrite = (captureWrite + 1) % captureLen;

                spawnAccum += 1.0f;
                if (spawnAccum >= samplesPerGrain)
                {
                    spawnAccum -= samplesPerGrain;
                    spawnGrain(grainSizeMs, scatter);
                }

                float sumL = 0.0f, sumR = 0.0f;
                for (auto& g : grains)
                {
                    if (!g.active) continue;
                    int readIdx = (g.startIdx + g.pos) % captureLen;
                    float sample = captureBuf[readIdx] * gaussianEnv(g.pos, g.length);
                    sumL += sample * g.ampL;
                    sumR += sample * g.ampR;
                    g.pos++;
                    if (g.pos >= g.length) g.active = false;
                }

                float normFactor = 1.0f / std::max(1.0f, static_cast<float>(kMaxGrains) * 0.5f);
                L[i] = monoIn[i] + mix * (sumL * normFactor - monoIn[i]);
                R[i] = monoIn[i] + mix * (sumR * normFactor - monoIn[i]);
            }
        }
    } granular_;

    //==========================================================================
    // Stage 5 — Hard-Chopped Tremolo (Boss PN-2)
    // Shape-select LFO + slew limiter + ping-pong L/R phase offset
    //==========================================================================
    struct TremoloStage
    {
        StandardLFO lfoL, lfoR;
        ParameterSmoother gainSmoothL, gainSmoothR;

        void prepare(double sampleRate)
        {
            lfoL.setShape(StandardLFO::Shape::Triangle);
            lfoR.setShape(StandardLFO::Shape::Triangle);
            gainSmoothL.prepare(static_cast<float>(sampleRate), 0.002f);
            gainSmoothR.prepare(static_cast<float>(sampleRate), 0.002f);
        }
        void reset()
        {
            gainSmoothL.snapTo(1.0f);
            gainSmoothR.snapTo(1.0f);
        }

        void processBlock(float* L, float* R, int numSamples,
                          float rateHz, float shape, float width,
                          float phaseOffsetDeg, float mix, float srF)
        {
            auto wform = (shape > 0.5f) ? StandardLFO::Shape::Square
                                        : StandardLFO::Shape::Triangle;
            lfoL.setShape(wform); lfoR.setShape(wform);
            lfoL.setRate(rateHz, srF); lfoR.setRate(rateHz, srF);
            lfoR.setPhaseOffset(phaseOffsetDeg / 360.0f);

            float widthN = width * 0.01f;

            for (int i = 0; i < numSamples; ++i)
            {
                float lfoOutL = (lfoL.process() * 0.5f + 0.5f);
                float lfoOutR = (lfoR.process() * 0.5f + 0.5f);
                float gainL = 1.0f - widthN * (1.0f - lfoOutL);
                float gainR = 1.0f - widthN * (1.0f - lfoOutR);
                gainSmoothL.set(gainL); gainSmoothR.set(gainR);
                float slewL = gainSmoothL.process();
                float slewR = gainSmoothR.process();

                float wetL = L[i] * slewL;
                float wetR = R[i] * slewR;
                L[i] = L[i] * (1.0f - mix) + wetL * mix;
                R[i] = R[i] * (1.0f - mix) + wetR * mix;
            }
        }
    } trem_;

    //==========================================================================
    // Cached parameter pointers (19 params)
    //==========================================================================
    std::atomic<float>* p_shimInterval  = nullptr;
    std::atomic<float>* p_shimDecay     = nullptr;
    std::atomic<float>* p_shimMix       = nullptr;
    std::atomic<float>* p_fuzzGain      = nullptr;
    std::atomic<float>* p_fuzzRectMix   = nullptr;
    std::atomic<float>* p_fuzzTreble    = nullptr;
    std::atomic<float>* p_fuzzBass      = nullptr;
    std::atomic<float>* p_revDelayTime  = nullptr;
    std::atomic<float>* p_revDelayPitch = nullptr;
    std::atomic<float>* p_revDelayMix   = nullptr;
    std::atomic<float>* p_granDensity   = nullptr;
    std::atomic<float>* p_granSize      = nullptr;
    std::atomic<float>* p_granScatter   = nullptr;
    std::atomic<float>* p_granMix       = nullptr;
    std::atomic<float>* p_tremRate      = nullptr;
    std::atomic<float>* p_tremShape     = nullptr;
    std::atomic<float>* p_tremWidth     = nullptr;
    std::atomic<float>* p_tremPhase     = nullptr;
    std::atomic<float>* p_tremMix       = nullptr;
};

//==============================================================================
// ShimmerReverbStage inline implementations (separate from class due to forward-declared methods)
//==============================================================================

inline void ObliterateChain::ShimmerReverbStage::prepare(double sampleRate)
{
    sr = sampleRate;
    float srScale = static_cast<float>(sampleRate) / 48000.0f;
    for (int t = 0; t < kNumTaps; ++t)
    {
        int len = static_cast<int>(kPrimeTaps[t] * srScale) + 2;
        fdnBufs[t].assign(len, 0.0f);
        fdnWritePos[t] = 0;
        fdnState[t]    = 0.0f;
    }
    int psolaLen = static_cast<int>(kPrimeTaps[kNumTaps - 1] * srScale * 2) + 4;
    psola1.assign(psolaLen, 0.0f);
    psola2.assign(psolaLen, 0.0f);
    psolaWrite1 = psolaWrite2 = 0;
    psolaPhase  = 0.0f;
    psolaToggle = false;
    psolaGrain1 = 1.0f; psolaGrain2 = 0.0f;
}

inline void ObliterateChain::ShimmerReverbStage::reset()
{
    for (int t = 0; t < kNumTaps; ++t)
    {
        std::fill(fdnBufs[t].begin(), fdnBufs[t].end(), 0.0f);
        fdnWritePos[t] = 0;
        fdnState[t]    = 0.0f;
    }
    std::fill(psola1.begin(), psola1.end(), 0.0f);
    std::fill(psola2.begin(), psola2.end(), 0.0f);
    psolaWrite1 = psolaWrite2 = 0;
    psolaPhase  = 0.0f;
    psolaToggle = false;
    psolaGrain1 = 1.0f; psolaGrain2 = 0.0f;
}

inline float ObliterateChain::ShimmerReverbStage::processSample(
    float in, float decaySec, int semitones, double sampleRate)
{
    float srF = static_cast<float>(sampleRate);
    float srScale = srF / 48000.0f;
    float decayCoeff = fastExp(-1.0f / (decaySec * srF));
    float pitchRatio = fastPow2(static_cast<float>(semitones) / 12.0f);

    float fdnSum = 0.0f;
    for (int t = 0; t < kNumTaps; ++t)
    {
        int len = static_cast<int>(fdnBufs[t].size());
        int effectiveDelay = static_cast<int>(kPrimeTaps[t] * srScale);
        effectiveDelay = juce::jlimit(1, len - 1, effectiveDelay);
        int rp = ((fdnWritePos[t] - effectiveDelay) + len) % len;
        fdnState[t] = flushDenormal(fdnState[t] * decayCoeff + fdnBufs[t][rp] * (1.0f - decayCoeff));
        fdnSum += fdnState[t];
    }
    fdnSum /= static_cast<float>(kNumTaps);

    // PSOLA pitch-shifted feedback
    float grainLen = srF * 0.05f; // 50ms grains
    psolaPhase += pitchRatio / grainLen;
    if (psolaPhase >= 1.0f)
    {
        psolaPhase -= 1.0f;
        psolaToggle = !psolaToggle;
        if (psolaToggle) { psolaGrain1 = 0.0f; psolaGrain2 = 1.0f; }
        else             { psolaGrain1 = 1.0f; psolaGrain2 = 0.0f; }
    }
    float grainFade1 = 0.5f * (1.0f - std::cos(6.28318f * std::min(psolaPhase, 1.0f)));
    float psolaOut = 0.0f;
    int p1Len = static_cast<int>(psola1.size());
    int p2Len = static_cast<int>(psola2.size());
    if (p1Len > 0) psolaOut += psola1[psolaWrite1 % p1Len] * grainFade1;
    if (p2Len > 0) psolaOut += psola2[psolaWrite2 % p2Len] * (1.0f - grainFade1);

    float feedback = fdnSum * 0.5f + psolaOut * 0.5f;
    float fdnIn = in + feedback * 0.5f;
    for (int t = 0; t < kNumTaps; ++t)
    {
        int len = static_cast<int>(fdnBufs[t].size());
        fdnBufs[t][fdnWritePos[t]] = flushDenormal(fdnIn);
        fdnWritePos[t] = (fdnWritePos[t] + 1) % len;
    }
    if (p1Len > 0) psola1[psolaWrite1 % p1Len] = flushDenormal(fdnSum);
    if (p2Len > 0) psola2[psolaWrite2 % p2Len] = flushDenormal(fdnSum);
    psolaWrite1 = (psolaWrite1 + 1) % std::max(1, p1Len);
    psolaWrite2 = (psolaWrite2 + 1) % std::max(1, p2Len);

    return fdnSum;
}

//==============================================================================
// ObliterateChain inline implementations
//==============================================================================

inline void ObliterateChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_ = sampleRate;
    shimmer_.prepare(sampleRate);
    fuzz_.prepare(sampleRate);
    revDelay_.prepare(sampleRate);
    granular_.prepare(sampleRate, maxBlockSize);
    trem_.prepare(sampleRate);
}

inline void ObliterateChain::reset()
{
    shimmer_.reset();
    fuzz_.reset();
    revDelay_.reset();
    granular_.reset();
    trem_.reset();
}

inline void ObliterateChain::processBlock(const float* monoIn, float* L, float* R, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (!p_shimInterval) return;

    const float shimInterval  = p_shimInterval->load(std::memory_order_relaxed);
    const float shimDecay     = p_shimDecay->load(std::memory_order_relaxed);
    const float shimMix       = p_shimMix->load(std::memory_order_relaxed);
    const float fuzzGain      = p_fuzzGain->load(std::memory_order_relaxed);
    const float fuzzRectMix   = p_fuzzRectMix->load(std::memory_order_relaxed);
    const float fuzzTreble    = p_fuzzTreble->load(std::memory_order_relaxed);
    const float fuzzBass      = p_fuzzBass->load(std::memory_order_relaxed);
    const float revDelayTime  = p_revDelayTime->load(std::memory_order_relaxed);
    const float revDelayPitch = p_revDelayPitch->load(std::memory_order_relaxed);
    const float revDelayMix   = p_revDelayMix->load(std::memory_order_relaxed);
    const float granDensity   = p_granDensity->load(std::memory_order_relaxed);
    const float granSize      = p_granSize->load(std::memory_order_relaxed);
    const float granScatter   = p_granScatter->load(std::memory_order_relaxed);
    const float granMix       = p_granMix->load(std::memory_order_relaxed);
    const float tremRate      = p_tremRate->load(std::memory_order_relaxed);
    const float tremShape     = p_tremShape->load(std::memory_order_relaxed);
    const float tremWidth     = p_tremWidth->load(std::memory_order_relaxed);
    const float tremPhase     = p_tremPhase->load(std::memory_order_relaxed);
    const float tremMix       = p_tremMix->load(std::memory_order_relaxed);

    float fuzzGainLin = fastPow2(fuzzGain * (1.0f / 6.0205999f));

    // Temp mono buffer (use L as scratch through stages 1-3)
    for (int i = 0; i < numSamples; ++i) L[i] = monoIn[i];

    // Stage 1: Shimmer Reverb
    if (shimMix > 0.001f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float shimOut = shimmer_.processSample(L[i], shimDecay,
                                                   static_cast<int>(shimInterval), sr_);
            L[i] = L[i] + shimMix * 0.01f * (shimOut - L[i]);
        }
    }

    // Stage 2: Octave Fuzz
    for (int i = 0; i < numSamples; ++i)
    {
        float fuzzOut = fuzz_.process(L[i], fuzzGainLin, fuzzTreble, fuzzBass);
        L[i] = L[i] + fuzzRectMix * 0.01f * (fuzzOut - L[i]);
    }

    // Stage 3: Reverse Pitch Delay
    for (int i = 0; i < numSamples; ++i)
        L[i] = revDelay_.process(L[i], revDelayTime, revDelayPitch, revDelayMix * 0.01f);

    // Stage 4: Granular Looper — Mono → Stereo expansion
    granular_.processBlock(L, L, R, numSamples, granDensity, granSize,
                           granScatter, granMix * 0.01f);

    // Stage 5: Hard-Chopped Tremolo (stereo)
    trem_.processBlock(L, R, numSamples, tremRate, tremShape, tremWidth,
                       tremPhase, tremMix * 0.01f, static_cast<float>(sr_));
}

inline void ObliterateChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    using AP = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    const juce::String p = slotPrefix + "oblt_";

    layout.add(std::make_unique<AP>(juce::ParameterID{p + "shimInterval",  1}, p + "Shim Interval",
        NR(-12.0f, 12.0f, 1.0f), 12.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "shimDecay",     1}, p + "Shim Decay",
        NR(0.1f, 10.0f, 0.01f), 2.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "shimMix",       1}, p + "Shim Mix",
        NR(0.0f, 100.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "fuzzGain",      1}, p + "Fuzz Gain",
        NR(0.0f, 40.0f, 0.1f), 12.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "fuzzRectMix",   1}, p + "Fuzz Rect Mix",
        NR(0.0f, 100.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "fuzzTreble",    1}, p + "Fuzz Treble",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "fuzzBass",      1}, p + "Fuzz Bass",
        NR(-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revDelayTime",  1}, p + "Rev Delay Time",
        NR(50.0f, 2000.0f, 1.0f), 500.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revDelayPitch", 1}, p + "Rev Delay Pitch",
        NR(0.25f, 4.0f, 0.001f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revDelayMix",   1}, p + "Rev Delay Mix",
        NR(0.0f, 100.0f, 0.1f), 40.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "granDensity",   1}, p + "Gran Density",
        NR(0.5f, 50.0f, 0.1f), 8.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "granSize",      1}, p + "Gran Size",
        NR(10.0f, 100.0f, 1.0f), 40.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "granScatter",   1}, p + "Gran Scatter",
        NR(0.0f, 100.0f, 0.1f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "granMix",       1}, p + "Gran Mix",
        NR(0.0f, 100.0f, 0.1f), 40.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremRate",      1}, p + "Trem Rate",
        NR(0.1f, 20.0f, 0.01f), 4.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremShape",     1}, p + "Trem Shape",
        NR(0.0f, 1.0f, 1.0f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremWidth",     1}, p + "Trem Width",
        NR(0.0f, 100.0f, 0.1f), 80.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremPhase",     1}, p + "Trem Phase",
        NR(0.0f, 360.0f, 1.0f), 180.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tremMix",       1}, p + "Trem Mix",
        NR(0.0f, 100.0f, 0.1f), 60.0f));
}

inline void ObliterateChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "oblt_";
    p_shimInterval  = apvts.getRawParameterValue(p + "shimInterval");
    p_shimDecay     = apvts.getRawParameterValue(p + "shimDecay");
    p_shimMix       = apvts.getRawParameterValue(p + "shimMix");
    p_fuzzGain      = apvts.getRawParameterValue(p + "fuzzGain");
    p_fuzzRectMix   = apvts.getRawParameterValue(p + "fuzzRectMix");
    p_fuzzTreble    = apvts.getRawParameterValue(p + "fuzzTreble");
    p_fuzzBass      = apvts.getRawParameterValue(p + "fuzzBass");
    p_revDelayTime  = apvts.getRawParameterValue(p + "revDelayTime");
    p_revDelayPitch = apvts.getRawParameterValue(p + "revDelayPitch");
    p_revDelayMix   = apvts.getRawParameterValue(p + "revDelayMix");
    p_granDensity   = apvts.getRawParameterValue(p + "granDensity");
    p_granSize      = apvts.getRawParameterValue(p + "granSize");
    p_granScatter   = apvts.getRawParameterValue(p + "granScatter");
    p_granMix       = apvts.getRawParameterValue(p + "granMix");
    p_tremRate      = apvts.getRawParameterValue(p + "tremRate");
    p_tremShape     = apvts.getRawParameterValue(p + "tremShape");
    p_tremWidth     = apvts.getRawParameterValue(p + "tremWidth");
    p_tremPhase     = apvts.getRawParameterValue(p + "tremPhase");
    p_tremMix       = apvts.getRawParameterValue(p + "tremMix");
}

} // namespace xoceanus
