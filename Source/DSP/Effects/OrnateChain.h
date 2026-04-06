// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
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
// OrnateChain — Granular Exciter FX Chain (5 stages)
//
// Source concepts: Boss AC-2, Fairfield Accountant, Mutable Clouds/Tomkat Cloudy,
//                  Lovetone Doppelganger, Strymon Volante
// Routing: Mono In → Stereo Out (expansion at Stage 3 granular scatter)
// Accent: Gilt Bronze #A87540
//
// Stage 1: Artificial Acoustic Exciter (Boss AC-2)
//          CytomicSVF Notch at 500 Hz (deep body cut) + HighShelf at 4 kHz
//          (resonant presence boost). EnvelopeFollower-gated shelf gain enhances
//          transients.
//
// Stage 2: JFET Smasher (Fairfield Accountant)
//          EnvelopeFollower with fast attack / extreme ratio brickwall compressor.
//          Squashes dynamics flat with makeup gain to restore loudness.
//
// Stage 3: High-Density Granular (Mutable Clouds / Tomkat Cloudy)
//          CircularBuffer capture. Grain scheduler: 10–30 ms grains,
//          up to 100 grains/sec density, Gaussian window envelopes,
//          independent stereo scatter per grain. Mono → Stereo here.
//
// Stage 4: Dual-LFO Optical Phaser (Lovetone Doppelganger)
//          Two CytomicSVF AllPass cascades in stereo. Two independent-rate
//          StandardLFOs with slew-rate limiting via ParameterSmoother.
//
// Stage 5: Multi-Head Magnetic Echo (Strymon Volante)
//          4 FractionalDelay read-heads at Even / Triplet / GoldenRatio spacing.
//          Saturator Tape in feedback path. StandardLFO wow/flutter modulation.
//
// Parameter prefix: orna_ (11 params)
//==============================================================================
class OrnateChain
{
public:
    OrnateChain() = default;

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

    //==========================================================================
    // Stage 1 — Artificial Acoustic Exciter (Boss AC-2)
    //==========================================================================
    struct ExciterStage
    {
        CytomicSVF notchFilter;   // body cut at 500 Hz
        CytomicSVF shelfFilter;   // presence boost at 4 kHz
        EnvelopeFollower envFollow;
        float shelfGainBase = 0.0f; // dB from orna_exciterTop

        void prepare(double sampleRate)
        {
            notchFilter.reset();
            shelfFilter.reset();
            envFollow.prepare(sampleRate);
            envFollow.setAttack(1.0f);   // 1 ms attack — transient responsive
            envFollow.setRelease(30.0f); // 30 ms release
        }
        void reset()
        {
            notchFilter.reset();
            shelfFilter.reset();
            envFollow.reset();
        }
        float process(float in, float exciterTop, float exciterBody, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            // Notch at 500 Hz — exciterBody controls depth (0 = no cut, 1 = deep notch)
            float notchQ = 0.5f + exciterBody * 4.5f; // Q range 0.5–5.0
            notchFilter.setMode(CytomicSVF::Mode::Notch);
            notchFilter.setCoefficients(500.0f, notchQ * 0.1f, srF);
            float x = notchFilter.processSample(in);

            // High-shelf boost at 4 kHz, gate-enhanced by envelope
            float envLevel = envFollow.process(in);
            float transientBoost = envLevel * 6.0f; // up to +6 dB transient lift
            float shelfDb = exciterTop * 18.0f + transientBoost; // 0–18 dB base + transient
            shelfFilter.setMode(CytomicSVF::Mode::HighShelf);
            shelfFilter.setCoefficients(4000.0f, 0.707f, srF, shelfDb);
            x = shelfFilter.processSample(x);
            return x;
        }
    } exciter_;

    //==========================================================================
    // Stage 2 — JFET Smasher (Fairfield Accountant)
    //==========================================================================
    struct SmasherStage
    {
        EnvelopeFollower env;
        ParameterSmoother gainSmoother;

        void prepare(double sampleRate)
        {
            env.prepare(sampleRate);
            env.setAttack(0.1f);   // 0.1 ms — ultra-fast JFET attack
            env.setRelease(60.0f); // 60 ms release
            gainSmoother.prepare(static_cast<float>(sampleRate), 0.003f);
            gainSmoother.snapTo(1.0f);
        }
        void reset()
        {
            env.reset();
            gainSmoother.snapTo(1.0f);
        }
        // squashRatio: 0 = 4:1, 1 = 10:1, 2 = Limit (∞:1)
        float process(float in, int squashRatio)
        {
            float level = env.process(in);
            // Compute gain reduction based on ratio
            float threshold = 0.25f; // -12 dBFS threshold
            float reduction = 1.0f;
            if (level > threshold)
            {
                float ratio;
                switch (squashRatio)
                {
                    case 0:  ratio = 4.0f;    break;
                    case 1:  ratio = 10.0f;   break;
                    default: ratio = 1000.0f; break; // Limit
                }
                // Gain reduction = threshold * (level/threshold)^(1/ratio) / level
                float compressed = threshold * std::pow(level / threshold, 1.0f / ratio);
                reduction = compressed / std::max(level, 1e-9f);
            }
            // Makeup gain: compensate to approximately unity at threshold
            float makeupGain;
            switch (squashRatio)
            {
                case 0:  makeupGain = 2.0f;  break; // +6 dB
                case 1:  makeupGain = 4.0f;  break; // +12 dB
                default: makeupGain = 8.0f;  break; // +18 dB
            }
            gainSmoother.set(reduction * makeupGain);
            return flushDenormal(in * gainSmoother.process());
        }
    } smasher_;

    //==========================================================================
    // Stage 3 — High-Density Granular (Mutable Clouds)
    // Mono → Stereo expansion happens here via stereo scatter.
    //==========================================================================
    struct GranularStage
    {
        static constexpr int kMaxCaptureSamples = 131072; // ~3s @ 44.1kHz
        static constexpr int kMaxGrains         = 64;

        CircularBuffer capture;
        double   sr       = 44100.0;
        uint32_t rngSeed_ = 0xDEADBEEFu;

        struct Grain
        {
            bool   active     = false;
            float  readPos    = 0.0f; // fractional read position into capture buffer
            float  readRate   = 1.0f; // always 1.0 for no pitch shift
            int    lengthSamp = 1000; // grain length in samples
            int    elapsed    = 0;    // samples processed since grain start
            float  panL       = 1.0f;
            float  panR       = 1.0f;
        };
        std::array<Grain, kMaxGrains> grains;
        int   grainWriteIdx = 0;
        float spawnAccum    = 0.0f;   // accumulates until next grain spawn

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            capture.prepare(kMaxCaptureSamples);
            for (auto& g : grains)
                g.active = false;
            grainWriteIdx = 0;
            spawnAccum    = 0.0f;
        }
        void reset()
        {
            capture.clear();
            for (auto& g : grains)
                g.active = false;
            spawnAccum = 0.0f;
            rngSeed_   = 0xDEADBEEFu;
        }

        // Simple pseudo-random: LCG, not cryptographic
        static float lcgRand(uint32_t& seed)
        {
            seed = seed * 1664525u + 1013904223u;
            return static_cast<float>(seed >> 1) / static_cast<float>(0x7FFFFFFF);
        }

        void processBlock(const float* monoIn, float* L, float* R, int numSamples,
                          float grainSizeMs, float grainDensity, float grainSpread)
        {
            float srF = static_cast<float>(sr);
            // grainDensity: 0–1 → 5–100 grains/sec
            float grainsPerSec    = 5.0f + grainDensity * 95.0f;
            float samplesPerGrain = srF / grainsPerSec;
            // grainSizeMs: 0–1 → 10–30 ms
            float grainLenMs   = 10.0f + grainSizeMs * 20.0f;
            int   grainLenSamp = std::max(1, static_cast<int>(grainLenMs * srF / 1000.0f));

            // Use per-instance seed (non-static to avoid data race across chain instances)
            uint32_t& seed = rngSeed_;

            for (int i = 0; i < numSamples; ++i)
            {
                // Write into circular capture buffer
                capture.write(monoIn[i]);

                // Spawn new grains
                spawnAccum += 1.0f;
                if (spawnAccum >= samplesPerGrain)
                {
                    spawnAccum -= samplesPerGrain;
                    // Find a free grain slot
                    for (int g = 0; g < kMaxGrains; ++g)
                    {
                        int idx = (grainWriteIdx + g) % kMaxGrains;
                        if (!grains[idx].active)
                        {
                            Grain& gr    = grains[idx];
                            gr.active    = true;
                            gr.elapsed   = 0;
                            gr.lengthSamp = grainLenSamp;
                            // Random start position within capture buffer (up to 200ms back)
                            float maxBack = std::min(0.2f * srF,
                                                     static_cast<float>(kMaxCaptureSamples - 2));
                            gr.readPos   = maxBack * lcgRand(seed);
                            gr.readRate  = 1.0f;
                            // Stereo scatter: 0 spread = mono, 1 = full random pan
                            float pan    = (lcgRand(seed) * 2.0f - 1.0f) * grainSpread;
                            gr.panL      = std::sqrt(std::max(0.0f, 0.5f - pan * 0.5f));
                            gr.panR      = std::sqrt(std::max(0.0f, 0.5f + pan * 0.5f));
                            grainWriteIdx = (idx + 1) % kMaxGrains;
                            break;
                        }
                    }
                }

                // Sum active grains
                float sumL = 0.0f, sumR = 0.0f;
                for (auto& gr : grains)
                {
                    if (!gr.active) continue;

                    // Gaussian window envelope
                    float t = static_cast<float>(gr.elapsed) / static_cast<float>(gr.lengthSamp);
                    float phase = t * 2.0f - 1.0f; // -1 to +1
                    float window = fastExp(-phase * phase * 4.5f); // Gaussian, σ = 1/3

                    // Read from capture buffer
                    int readOffset = static_cast<int>(gr.readPos);
                    readOffset = std::min(readOffset, kMaxCaptureSamples - 2);
                    float grainSample = capture.readForward(readOffset);
                    grainSample = flushDenormal(grainSample * window);

                    sumL += grainSample * gr.panL;
                    sumR += grainSample * gr.panR;

                    gr.readPos += gr.readRate;
                    gr.elapsed++;
                    if (gr.elapsed >= gr.lengthSamp)
                        gr.active = false;
                }
                // Normalise by sqrt of max concurrent grains
                constexpr float kNorm = 1.0f / 8.0f;
                L[i] = flushDenormal(sumL * kNorm);
                R[i] = flushDenormal(sumR * kNorm);
            }
        }
    } granular_;

    //==========================================================================
    // Stage 4 — Dual-LFO Optical Phaser (Lovetone Doppelganger)
    // Two AllPass cascades with independent LFO rates and slew-limited depth.
    //==========================================================================
    struct PhaserStage
    {
        static constexpr int kAllPassStages = 4;

        std::array<CytomicSVF, kAllPassStages> apL;
        std::array<CytomicSVF, kAllPassStages> apR;
        StandardLFO lfoL, lfoR;
        ParameterSmoother freqSmootherL, freqSmootherR;

        void prepare(double sampleRate)
        {
            for (auto& f : apL) f.reset();
            for (auto& f : apR) f.reset();
            freqSmootherL.prepare(static_cast<float>(sampleRate), 0.010f);
            freqSmootherR.prepare(static_cast<float>(sampleRate), 0.010f);
            freqSmootherL.snapTo(500.0f);
            freqSmootherR.snapTo(700.0f);
        }
        void reset()
        {
            for (auto& f : apL) f.reset();
            for (auto& f : apR) f.reset();
        }
        void processBlock(float* L, float* R, int numSamples,
                          float rate1, float rate2, float color, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            lfoL.setRate(rate1, srF);
            lfoL.setShape(StandardLFO::Sine);
            lfoR.setRate(rate2, srF);
            lfoR.setShape(StandardLFO::Sine);

            // color: 0–1 maps base phaser freq from 200 Hz to 2 kHz
            float baseFreq = 200.0f + color * 1800.0f;
            float depth    = 800.0f; // ±800 Hz sweep

            for (int i = 0; i < numSamples; ++i)
            {
                float modL = lfoL.process();
                float modR = lfoR.process();

                float freqL = baseFreq + modL * depth;
                float freqR = baseFreq + modR * depth;
                freqL = std::max(50.0f, std::min(freqL, srF * 0.45f));
                freqR = std::max(50.0f, std::min(freqR, srF * 0.45f));

                freqSmootherL.set(freqL);
                freqSmootherR.set(freqR);
                float sFL = freqSmootherL.process();
                float sFR = freqSmootherR.process();

                float xL = L[i];
                float xR = R[i];
                for (int s = 0; s < kAllPassStages; ++s)
                {
                    apL[s].setMode(CytomicSVF::Mode::AllPass);
                    apL[s].setCoefficients(sFL * (1.0f + 0.3f * static_cast<float>(s)), 0.5f, srF);
                    xL = apL[s].processSample(xL);

                    apR[s].setMode(CytomicSVF::Mode::AllPass);
                    apR[s].setCoefficients(sFR * (1.0f + 0.3f * static_cast<float>(s)), 0.5f, srF);
                    xR = apR[s].processSample(xR);
                }
                L[i] = flushDenormal(xL);
                R[i] = flushDenormal(xR);
            }
        }
    } phaser_;

    //==========================================================================
    // Stage 5 — Multi-Head Magnetic Echo (Strymon Volante)
    // 4 read heads, Saturator tape feedback, LFO wow/flutter.
    //==========================================================================
    struct VolanteStage
    {
        static constexpr float kMaxDelayMs = 2000.0f;
        static constexpr int   kNumHeads   = 4;

        FractionalDelay delayL, delayR;
        Saturator   tapeSat;
        StandardLFO wowFlutter;
        float feedbackL = 0.0f;
        float feedbackR = 0.0f;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 16;
            delayL.prepare(maxSamp);
            delayR.prepare(maxSamp);
            tapeSat.setMode(Saturator::SaturationMode::Tape);
            tapeSat.setDrive(0.3f);
            tapeSat.setMix(1.0f);
            wowFlutter.setRate(0.8f, static_cast<float>(sampleRate));
            wowFlutter.setShape(StandardLFO::Sine);
            feedbackL = feedbackR = 0.0f;
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
            feedbackL = feedbackR = 0.0f;
        }

        // headSpacing: 0 = Even, 1 = Triplet, 2 = Golden Ratio
        void processBlock(float* L, float* R, int numSamples,
                          float drumWear, int headSpacing, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            // Base delay = 250 ms
            float baseMs = 250.0f;
            float baseSamp = baseMs * srF / 1000.0f;

            // Head ratios by spacing mode
            float headRatios[kNumHeads];
            switch (headSpacing)
            {
                case 0: // Even
                    headRatios[0] = 0.25f; headRatios[1] = 0.5f;
                    headRatios[2] = 0.75f; headRatios[3] = 1.0f;
                    break;
                case 1: // Triplet (1/3, 2/3, 1, 4/3)
                    headRatios[0] = 0.333f; headRatios[1] = 0.667f;
                    headRatios[2] = 1.0f;   headRatios[3] = 1.333f;
                    break;
                default: // Golden Ratio (φ = 1.618…)
                    headRatios[0] = 0.382f; headRatios[1] = 0.618f;
                    headRatios[2] = 1.0f;   headRatios[3] = 1.618f;
                    break;
            }

            // drumWear: tape saturation drive 0–1
            tapeSat.setDrive(drumWear);

            const float maxSamp = static_cast<float>(delayL.getSize() - 4);
            const float feedback = 0.4f;

            for (int i = 0; i < numSamples; ++i)
            {
                float wowMod = wowFlutter.process() * 2.0f; // ±2 samples flutter

                float wetL = 0.0f, wetR = 0.0f;
                for (int h = 0; h < kNumHeads; ++h)
                {
                    float delSamp = std::min(baseSamp * headRatios[h] + wowMod, maxSamp);
                    delSamp = std::max(delSamp, 1.0f);
                    float weight = 1.0f / static_cast<float>(kNumHeads);
                    wetL += delayL.read(delSamp) * weight;
                    wetR += delayR.read(delSamp) * weight;
                }

                float fbL = tapeSat.processSample(L[i] + feedbackL * feedback);
                float fbR = tapeSat.processSample(R[i] + feedbackR * feedback);
                delayL.write(flushDenormal(fbL));
                delayR.write(flushDenormal(fbR));
                feedbackL = wetL;
                feedbackR = wetR;

                L[i] = flushDenormal(L[i] * 0.5f + wetL * 0.5f);
                R[i] = flushDenormal(R[i] * 0.5f + wetR * 0.5f);
            }
        }
    } volante_;

    //==========================================================================
    // Cached parameter pointers (11 params)
    //==========================================================================
    std::atomic<float>* p_exciterTop        = nullptr;
    std::atomic<float>* p_exciterBody       = nullptr;
    std::atomic<float>* p_squashRatio       = nullptr; // choice: 0/1/2
    std::atomic<float>* p_grainSize         = nullptr;
    std::atomic<float>* p_grainDensity      = nullptr;
    std::atomic<float>* p_grainSpread       = nullptr;
    std::atomic<float>* p_phaseRate1        = nullptr;
    std::atomic<float>* p_phaseRate2        = nullptr;
    std::atomic<float>* p_phaseColor        = nullptr;
    std::atomic<float>* p_drumWear          = nullptr;
    std::atomic<float>* p_drumHeadSpacing   = nullptr; // choice: 0/1/2
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OrnateChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    exciter_.prepare(sampleRate);
    smasher_.prepare(sampleRate);
    granular_.prepare(sampleRate);
    phaser_.prepare(sampleRate);
    volante_.prepare(sampleRate);
}

inline void OrnateChain::reset()
{
    exciter_.reset();
    smasher_.reset();
    granular_.reset();
    phaser_.reset();
    volante_.reset();
}

inline void OrnateChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_exciterTop) return;

    // ParamSnapshot
    const float exciterTop      = p_exciterTop->load(std::memory_order_relaxed);
    const float exciterBody     = p_exciterBody->load(std::memory_order_relaxed);
    const int   squashRatio     = static_cast<int>(p_squashRatio->load(std::memory_order_relaxed));
    const float grainSize       = p_grainSize->load(std::memory_order_relaxed);
    const float grainDensity    = p_grainDensity->load(std::memory_order_relaxed);
    const float grainSpread     = p_grainSpread->load(std::memory_order_relaxed);
    const float phaseRate1      = p_phaseRate1->load(std::memory_order_relaxed);
    const float phaseRate2      = p_phaseRate2->load(std::memory_order_relaxed);
    const float phaseColor      = p_phaseColor->load(std::memory_order_relaxed);
    const float drumWear        = p_drumWear->load(std::memory_order_relaxed);
    const int   drumHeadSpacing = static_cast<int>(p_drumHeadSpacing->load(std::memory_order_relaxed));

    // Stages 1 & 2 — mono, write into L as temp buffer
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];
        // Stage 1: Acoustic Exciter
        x = exciter_.process(x, exciterTop, exciterBody, sr_);
        // Stage 2: JFET Smasher
        x = smasher_.process(x, squashRatio);
        L[i] = x;
    }

    // Stage 3: Granular — expands mono → stereo
    granular_.processBlock(L, L, R, numSamples, grainSize, grainDensity, grainSpread);

    // Stage 4: Dual-LFO Phaser — stereo in-place
    phaser_.processBlock(L, R, numSamples, phaseRate1, phaseRate2, phaseColor, sr_);

    // Stage 5: Magnetic Echo — stereo in-place
    volante_.processBlock(L, R, numSamples, drumWear, drumHeadSpacing, sr_);
}

inline void OrnateChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "orna_";

    registerFloat(layout, p + "exciterTop",      p + "Exciter Top",
                  0.0f, 1.0f, 0.4f);
    registerFloat(layout, p + "exciterBody",     p + "Exciter Body",
                  0.0f, 1.0f, 0.3f);
    registerChoice(layout, p + "squashRatio",    p + "Squash Ratio",
                   {"4:1", "10:1", "Limit"}, 1);
    registerFloat(layout, p + "grainSize",       p + "Grain Size",
                  0.0f, 1.0f, 0.5f);
    registerFloat(layout, p + "grainDensity",    p + "Grain Density",
                  0.0f, 1.0f, 0.5f);
    registerFloat(layout, p + "grainSpread",     p + "Grain Spread",
                  0.0f, 1.0f, 0.5f);
    registerFloatSkewed(layout, p + "phaseRate1", p + "Phase Rate 1",
                        0.05f, 10.0f, 0.5f, 0.001f, 0.35f);
    registerFloatSkewed(layout, p + "phaseRate2", p + "Phase Rate 2",
                        0.05f, 10.0f, 0.8f, 0.001f, 0.35f);
    registerFloat(layout, p + "phaseColor",      p + "Phase Color",
                  0.0f, 1.0f, 0.4f);
    registerFloat(layout, p + "drumWear",        p + "Drum Wear",
                  0.0f, 1.0f, 0.3f);
    registerChoice(layout, p + "drumHeadSpacing", p + "Head Spacing",
                   {"Even", "Triplet", "GoldenRatio"}, 0);
}

inline void OrnateChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "orna_";
    p_exciterTop      = cacheParam(apvts, p + "exciterTop");
    p_exciterBody     = cacheParam(apvts, p + "exciterBody");
    p_squashRatio     = cacheParam(apvts, p + "squashRatio");
    p_grainSize       = cacheParam(apvts, p + "grainSize");
    p_grainDensity    = cacheParam(apvts, p + "grainDensity");
    p_grainSpread     = cacheParam(apvts, p + "grainSpread");
    p_phaseRate1      = cacheParam(apvts, p + "phaseRate1");
    p_phaseRate2      = cacheParam(apvts, p + "phaseRate2");
    p_phaseColor      = cacheParam(apvts, p + "phaseColor");
    p_drumWear        = cacheParam(apvts, p + "drumWear");
    p_drumHeadSpacing = cacheParam(apvts, p + "drumHeadSpacing");
}

} // namespace xoceanus
