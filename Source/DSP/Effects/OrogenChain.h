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
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OrogenChain — OROGEN Ringing Abyss FX Chain (4 stages)
//
// Concept: Lovetone Ring Stinger × Industrialectric RM-1N × Pladask Fabrikat
//          × Alesis Quadraverb
// Routing: Mono In → Stereo Out (scatter at Stage 3 Granular Time-Stretch)
//
// Stage 1: Transformer Ring-Mod/Fuzz (Lovetone Ring Stinger) — PolyBLEP sine
//          carrier × input + Saturator FoldBack + OversamplingProcessor<8>
// Stage 2: Distorted Plate Reverb (Industrialectric RM-1N) — Saturator Tube
//          pre-fuzz → small FDN reverb (4 delay lines) → Saturator Tube post
// Stage 3: Granular Time-Stretch (Pladask Fabrikat) — CircularBuffer + grain
//          scheduler with Gaussian windows → Mono→Stereo scatter
// Stage 4: Tuned Resonator (Alesis Quadraverb) — 4 parallel comb filters
//          tuned to chord frequencies, 95% feedback
//
// Parameter prefix: orog_ (12 params)
//==============================================================================
class OrogenChain
{
public:
    OrogenChain() = default;

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
    double sr_        = 0.0;  // Sentinel: must be set by prepare() before use
    int    blockSize_ = 512;

    //==========================================================================
    // Stage 1 — Transformer Ring-Mod/Fuzz (Lovetone Ring Stinger)
    // PolyBLEP sine carrier × input + Saturator FoldBack, 8x oversampled
    //==========================================================================
    struct RingStingerStage
    {
        PolyBLEP                 sineCarrier;
        OversamplingProcessor<8> ovs;
        Saturator                foldSat;

        void prepare(double sampleRate, int maxBlockSize)
        {
            sineCarrier.setWaveform(PolyBLEP::Waveform::Sine);
            ovs.prepare(sampleRate, maxBlockSize);
            foldSat.prepare(sampleRate);
            foldSat.setMode(Saturator::SaturationMode::FoldBack);
            foldSat.setMix(1.0f);
        }

        void reset()
        {
            ovs.reset();
            foldSat.reset();
        }

        // Process a full block in-place with 8x OVS on the foldback fuzz path.
        // monoIn/buf may be the same pointer — writes ring+fuzz output to buf.
        void processBlock(const float* monoIn, float* buf, int numSamples,
                          float carrierHz, float fuzzAmt, float ringMix, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            sineCarrier.setFrequency(carrierHz, srF);

            // First pass: ring-modulate into buf
            for (int i = 0; i < numSamples; ++i)
                buf[i] = monoIn[i] * sineCarrier.processSample();

            // FoldBack fuzz at 8x oversampled rate
            foldSat.setDrive(fuzzAmt);
            ovs.process(buf, numSamples, [&](float* upBuf, int upN)
            {
                for (int i = 0; i < upN; ++i)
                    upBuf[i] = foldSat.processSample(upBuf[i]);
            });

            // Blend: dry + ringMix * (fuzzed - dry)
            for (int i = 0; i < numSamples; ++i)
                buf[i] = monoIn[i] + ringMix * (buf[i] - monoIn[i]);
        }
    } ringStinger_;

    //==========================================================================
    // Stage 2 — Distorted Plate Reverb (Industrialectric RM-1N)
    // Saturator Tube pre → 4-line FDN reverb (Householder) → Saturator Tube post
    //==========================================================================
    struct PlateReverbStage
    {
        Saturator preFuzz;
        Saturator postFuzz;

        // 4-line FDN with Householder feedback matrix
        // Delay lengths (prime-spaced in samples for decorrelation)
        static constexpr int kNumLines = 4;
        static constexpr int kDelayLengthsMs[4] = { 29, 37, 53, 71 }; // prime ms values

        FractionalDelay lines[kNumLines];
        CytomicSVF      dampFilts[kNumLines]; // HF damping per delay line
        float           feedback[kNumLines] = {};

        void prepare(double sampleRate)
        {
            preFuzz.prepare(sampleRate);
            preFuzz.setMode(Saturator::SaturationMode::Tube);
            preFuzz.setMix(1.0f);
            preFuzz.setOutputGain(0.5f);

            postFuzz.prepare(sampleRate);
            postFuzz.setMode(Saturator::SaturationMode::Tube);
            postFuzz.setMix(1.0f);
            postFuzz.setOutputGain(0.5f);

            for (int i = 0; i < kNumLines; ++i)
            {
                int delaySamples = static_cast<int>(kDelayLengthsMs[i] * sampleRate / 1000.0) + 4;
                lines[i].prepare(delaySamples);
                dampFilts[i].setMode(CytomicSVF::Mode::LowPass);
                dampFilts[i].setCoefficients(3000.0f, 0.5f, static_cast<float>(sampleRate));
                feedback[i] = 0.0f;
            }
        }

        void reset()
        {
            preFuzz.reset();
            postFuzz.reset();
            for (int i = 0; i < kNumLines; ++i)
            {
                lines[i].clear();
                dampFilts[i].reset();
                feedback[i] = 0.0f;
            }
        }

        float process(float in, float preGainAmt, float verbSize, float postGainAmt,
                      double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);

            // Pre-fuzz
            preFuzz.setDrive(preGainAmt);
            float x = preFuzz.processSample(in);

            // Householder feedback: negate sum minus each component (N=4)
            float sum = feedback[0] + feedback[1] + feedback[2] + feedback[3];

            float fdnOut = 0.0f;
            for (int i = 0; i < kNumLines; ++i)
            {
                float delayMs = static_cast<float>(kDelayLengthsMs[i]) * (0.5f + verbSize * 0.5f);
                float delaySamples = delayMs * srF * 0.001f;
                delaySamples = std::max(1.0f, std::min(delaySamples, static_cast<float>(lines[i].getSize()) - 2.0f));

                // Read from delay line
                float delayOut = lines[i].read(delaySamples);

                // HF damping
                dampFilts[i].setCoefficients(3000.0f - verbSize * 2000.0f, 0.5f, srF);
                delayOut = dampFilts[i].processSample(delayOut);

                // Householder matrix: fb_i = (2/N * sum - fb_i) * fbGain
                float fbGain = 0.6f + verbSize * 0.3f; // 0.6–0.9
                feedback[i] = flushDenormal((sum * 0.5f - feedback[i]) * fbGain);

                // Write to delay line: input + feedback
                lines[i].write(x + feedback[i]);

                fdnOut += delayOut;
            }

            fdnOut *= 0.25f; // normalize 4 lines

            // Post-fuzz
            postFuzz.setDrive(postGainAmt * 0.5f);
            return postFuzz.processSample(fdnOut);
        }
    } plateReverb_;

    //==========================================================================
    // Stage 3 — Granular Time-Stretch (Pladask Fabrikat)
    // CircularBuffer capture + grain scheduler with Gaussian windows
    // Mono → Stereo scatter at this stage
    //==========================================================================
    struct GranularStage
    {
        static constexpr int kMaxGrains    = 8;
        static constexpr int kCaptureSecs  = 4; // 4 seconds of capture buffer

        CircularBuffer capture;
        int            captureSize  = 0;
        double         sr = 0.0;  // Sentinel: must be set by prepare() before use

        struct Grain
        {
            bool  active       = false;
            int   readOffset   = 0;     // offset into capture buffer at grain start
            int   samplePos    = 0;     // current position within grain
            int   grainLength  = 0;     // grain size in samples
            float panL         = 1.0f;
            float panR         = 1.0f;
        };

        Grain     grains[kMaxGrains] = {};
        int       nextGrainSlot      = 0;
        float     spawnAccum         = 0.0f;
        uint32_t  lcgState           = 13579u;

        float nextRandom() noexcept
        {
            lcgState = lcgState * 1664525u + 1013904223u;
            return static_cast<float>(lcgState & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
        }

        // Gaussian window approximation via 1 - (2t-1)^2 applied twice
        static float gaussWindow(float pos0to1) noexcept
        {
            float t = pos0to1 * 2.0f - 1.0f; // [-1, 1]
            float g = 1.0f - t * t;            // 0..1 parabola (approximates Gaussian)
            return g * g;                       // sharpen with second power
        }

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            captureSize = static_cast<int>(sampleRate * kCaptureSecs);
            capture.prepare(captureSize);
            spawnAccum = 0.0f;
            for (auto& g : grains) g.active = false;
        }

        void reset()
        {
            capture.clear();
            spawnAccum = 0.0f;
            for (auto& g : grains) g.active = false;
        }

        void process(float in, float& outL, float& outR,
                     float grainSizeMs, float stretch, float density)
        {
            capture.write(in);

            float srF = static_cast<float>(sr);
            float grainLengthSamples = grainSizeMs * srF * 0.001f;
            grainLengthSamples = std::max(64.0f, std::min(grainLengthSamples, static_cast<float>(captureSize / 2)));

            // Spawn grains based on density (grains per second)
            float spawnRate = density * srF * 0.001f; // density = grains per 1000 samples
            spawnAccum += spawnRate;
            if (spawnAccum >= 1.0f)
            {
                spawnAccum -= 1.0f;

                // Find an inactive grain slot
                for (int attempt = 0; attempt < kMaxGrains; ++attempt)
                {
                    int slot = (nextGrainSlot + attempt) % kMaxGrains;
                    if (!grains[slot].active)
                    {
                        // Spawn grain: random read position in capture buffer
                        float stretchOffset = (1.0f - stretch) * grainLengthSamples * 2.0f;
                        int maxOffset = std::min(static_cast<int>(stretchOffset) + static_cast<int>(grainLengthSamples),
                                                 captureSize - 1);
                        int readOffset = static_cast<int>(nextRandom() * static_cast<float>(maxOffset));
                        readOffset = std::max(1, std::min(readOffset, captureSize - 1));

                        grains[slot].active     = true;
                        grains[slot].readOffset = readOffset;
                        grains[slot].samplePos  = 0;
                        grains[slot].grainLength = static_cast<int>(grainLengthSamples);

                        // Stereo scatter: random pan
                        float pan = nextRandom(); // 0..1
                        grains[slot].panL = fastCos(pan * 1.5707963f); // half-power panning
                        grains[slot].panR = fastSin(pan * 1.5707963f);

                        nextGrainSlot = (slot + 1) % kMaxGrains;
                        break;
                    }
                }
            }

            // Accumulate active grains
            float sumL = 0.0f, sumR = 0.0f;
            int activeCount = 0;

            for (int g = 0; g < kMaxGrains; ++g)
            {
                if (!grains[g].active) continue;

                float t   = static_cast<float>(grains[g].samplePos) / static_cast<float>(grains[g].grainLength);
                float env = gaussWindow(t);
                float smp = capture.readForward(grains[g].readOffset + grains[g].samplePos);

                sumL += smp * env * grains[g].panL;
                sumR += smp * env * grains[g].panR;

                ++grains[g].samplePos;
                if (grains[g].samplePos >= grains[g].grainLength)
                    grains[g].active = false;

                ++activeCount;
            }

            // Normalize by grain overlap count (at least 1)
            float norm = 1.0f / static_cast<float>(std::max(1, activeCount));
            outL = flushDenormal(sumL * norm);
            outR = flushDenormal(sumR * norm);
        }
    } granular_;

    //==========================================================================
    // Stage 4 — Tuned Resonator (Alesis Quadraverb)
    // 4 parallel comb filters tuned to chord frequencies, 95% feedback
    // Chord modes: Maj/Min/Dim/Sus4 (ratio tables)
    //==========================================================================
    struct TunedResonatorStage
    {
        // 4 chord modes × 4 voices: semitone offsets from root
        static constexpr int kChordOffsets[4][4] = {
            {  0,  4,  7, 12 }, // Major
            {  0,  3,  7, 12 }, // Minor
            {  0,  3,  6, 12 }, // Diminished
            {  0,  5,  7, 12 }, // Sus4
        };

        FractionalDelay combs[4];
        float           combFeedback[4] = {};
        double          sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            // Max comb delay = 1 second (covers ~20 Hz fundamental)
            int maxDelay = static_cast<int>(sampleRate) + 4;
            for (int i = 0; i < 4; ++i)
            {
                combs[i].prepare(maxDelay);
                combFeedback[i] = 0.0f;
            }
        }

        void reset()
        {
            for (int i = 0; i < 4; ++i)
            {
                combs[i].clear();
                combFeedback[i] = 0.0f;
            }
        }

        void process(float in, float& outL, float& outR,
                     int chordMode, float resonance, float resMix)
        {
            float srF    = static_cast<float>(sr);
            float fbGain = 0.90f + resonance * 0.049f; // 0.90–0.949 (stays < 1.0)
            float sumL   = 0.0f, sumR = 0.0f;

            // Root fundamental: 110 Hz (A2), sensible default for resonators
            float rootHz = 110.0f;
            int   mode   = std::max(0, std::min(chordMode, 3));

            for (int i = 0; i < 4; ++i)
            {
                float semitones = static_cast<float>(kChordOffsets[mode][i]);
                float freq      = rootHz * fastPow2(semitones / 12.0f);
                float delaySmp  = srF / std::max(freq, 20.0f);
                delaySmp = std::max(1.0f, std::min(delaySmp, static_cast<float>(combs[i].getSize()) - 2.0f));

                combs[i].write(in + combFeedback[i] * fbGain);
                float combOut = combs[i].read(delaySmp);
                combFeedback[i] = flushDenormal(combOut);

                // Alternate L/R for stereo spread
                if (i % 2 == 0)
                    sumL += combOut;
                else
                    sumR += combOut;
            }

            sumL *= 0.5f;
            sumR *= 0.5f;

            outL = in + resMix * (sumL - in);
            outR = in + resMix * (sumR - in);
        }
    } resonator_;

    //==========================================================================
    // Cached parameter pointers (12 params)
    //==========================================================================
    std::atomic<float>* p_ringFreq  = nullptr;
    std::atomic<float>* p_ringFuzz  = nullptr;
    std::atomic<float>* p_ringMix   = nullptr;
    std::atomic<float>* p_preGain   = nullptr;
    std::atomic<float>* p_verbSize  = nullptr;
    std::atomic<float>* p_postGain  = nullptr;
    std::atomic<float>* p_grainSize = nullptr;
    std::atomic<float>* p_stretch   = nullptr;
    std::atomic<float>* p_density   = nullptr;
    std::atomic<float>* p_chord     = nullptr;
    std::atomic<float>* p_resonance = nullptr;
    std::atomic<float>* p_resMix    = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OrogenChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    ringStinger_.prepare(sampleRate, maxBlockSize);
    plateReverb_.prepare(sampleRate);
    granular_.prepare(sampleRate);
    resonator_.prepare(sampleRate);
}

inline void OrogenChain::reset()
{
    ringStinger_.reset();
    plateReverb_.reset();
    granular_.reset();
    resonator_.reset();
}

inline void OrogenChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_ringFreq) return;

    // ParamSnapshot: read all params once
    const float ringFreq  = p_ringFreq->load(std::memory_order_relaxed);
    const float ringFuzz  = p_ringFuzz->load(std::memory_order_relaxed);
    const float ringMix   = p_ringMix->load(std::memory_order_relaxed);
    const float preGain   = p_preGain->load(std::memory_order_relaxed);
    const float verbSize  = p_verbSize->load(std::memory_order_relaxed);
    const float postGain  = p_postGain->load(std::memory_order_relaxed);
    const float grainSize = p_grainSize->load(std::memory_order_relaxed);
    const float stretch   = p_stretch->load(std::memory_order_relaxed);
    const float density   = p_density->load(std::memory_order_relaxed);
    const int   chord     = static_cast<int>(p_chord->load(std::memory_order_relaxed));
    const float resonance = p_resonance->load(std::memory_order_relaxed);
    const float resMix    = p_resMix->load(std::memory_order_relaxed);

    // Stage 1: Ring Stinger — block-level with 8x OVS fuzz
    ringStinger_.processBlock(monoIn, L, numSamples, ringFreq, ringFuzz, ringMix, sr_);

    // Stage 2: Distorted Plate Reverb — per-sample
    for (int i = 0; i < numSamples; ++i)
        L[i] = plateReverb_.process(L[i], preGain, verbSize, postGain, sr_);

    // Stage 3: Granular Time-Stretch — Mono → Stereo scatter
    for (int i = 0; i < numSamples; ++i)
    {
        float gL, gR;
        granular_.process(L[i], gL, gR, grainSize, stretch, density);
        L[i] = gL;
        R[i] = gR;
    }

    // Stage 4: Tuned Resonator — operates on both L and R independently
    for (int i = 0; i < numSamples; ++i)
    {
        float rL, rR;
        float mono = (L[i] + R[i]) * 0.5f;
        resonator_.process(mono, rL, rR, chord, resonance, resMix);
        L[i] = rL;
        R[i] = rR;
    }
}

inline void OrogenChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "orog_";

    registerFloatSkewed(layout, p + "ringFreq",  p + "Ring Freq",
                        20.0f, 2000.0f, 220.0f, 0.1f, 0.4f);
    registerFloat(layout, p + "ringFuzz",  p + "Ring Fuzz",
                  0.0f, 1.0f, 0.3f, 0.001f);
    registerFloat(layout, p + "ringMix",   p + "Ring Mix",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "preGain",   p + "Pre Gain",
                  0.0f, 1.0f, 0.4f, 0.001f);
    registerFloat(layout, p + "verbSize",  p + "Verb Size",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "postGain",  p + "Post Gain",
                  0.0f, 1.0f, 0.3f, 0.001f);
    registerFloat(layout, p + "grainSize", p + "Grain Size",
                  10.0f, 500.0f, 100.0f, 0.1f);
    registerFloat(layout, p + "stretch",   p + "Stretch",
                  0.1f, 4.0f, 1.0f, 0.001f);
    registerFloat(layout, p + "density",   p + "Density",
                  0.1f, 5.0f, 2.0f, 0.01f);
    registerChoice(layout, p + "chord",    p + "Chord",
                  juce::StringArray{"Maj", "Min", "Dim", "Sus4"}, 0);
    registerFloat(layout, p + "resonance", p + "Resonance",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "resMix",    p + "Res Mix",
                  0.0f, 1.0f, 0.4f, 0.001f);
}

inline void OrogenChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "orog_";
    p_ringFreq  = cacheParam(apvts, p + "ringFreq");
    p_ringFuzz  = cacheParam(apvts, p + "ringFuzz");
    p_ringMix   = cacheParam(apvts, p + "ringMix");
    p_preGain   = cacheParam(apvts, p + "preGain");
    p_verbSize  = cacheParam(apvts, p + "verbSize");
    p_postGain  = cacheParam(apvts, p + "postGain");
    p_grainSize = cacheParam(apvts, p + "grainSize");
    p_stretch   = cacheParam(apvts, p + "stretch");
    p_density   = cacheParam(apvts, p + "density");
    p_chord     = cacheParam(apvts, p + "chord");
    p_resonance = cacheParam(apvts, p + "resonance");
    p_resMix    = cacheParam(apvts, p + "resMix");
}

} // namespace xoceanus
