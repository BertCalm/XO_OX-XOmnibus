// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../ParameterSmoother.h"
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
// OutbreakChain — Come As You Glitch FX Chain (4 stages)
//
// Source concept: Come As You Glitch (Lo-Fi/Industrial Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 2 Hyper-Scooped Octave Fuzz)
// Accent: Hot Pink #FF1493
//
// Stage 1: VHS Degradation (Chase Bliss Gen Loss)
//          FractionalDelay modulated by S&H→lowpass LFO (wow) + 60Hz sine (flutter).
//          Filtered white noise (tape hiss). CytomicSVF LP (bandwidth limit).
//          Dropout probability param.
// Stage 2: Hyper-Scooped Octave Fuzz (Boss FZ-2) — Mono → Stereo here.
//          Full-wave rectification (abs) + Saturator extreme drive +
//          Baxandall tone stack (LowShelf + HighShelf, mids carved out).
//          OversamplingProcessor<8>.
// Stage 3: Zero-Order Hold Decimator (Alesis Bitrman)
//          Sample-rate reduction (hold every Nth sample) +
//          bit-depth quantization (multiply→floor→divide).
// Stage 4: Industrial Concrete Reverb (Death By Audio Rooms)
//          8 FractionalDelay taps at prime spacings, cross-feeding.
//          No allpass diffusers — intentionally harsh discrete echoes.
//          CytomicSVF LP in feedback for slight darkening.
//
// Parameter prefix: outb_ (11 params)
//==============================================================================
class OutbreakChain
{
public:
    OutbreakChain() = default;

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
    double sr_        = 0.0;  // Sentinel: must be set by prepare() before use
    int    blockSize_ = 512;

    //==========================================================================
    // Stage 1 — VHS Degradation (Chase Bliss Gen Loss)
    //==========================================================================
    struct GenLossStage
    {
        static constexpr float kMaxWowMs = 5.0f;

        FractionalDelay wowDelay;
        StandardLFO     wowLFO;     // S&H shape → smoothed → slow wow
        StandardLFO     flutterLFO; // Sine at 60Hz → subtle flutter
        CytomicSVF      wowSmooth;  // Lowpass smoother for S&H output
        CytomicSVF      bwLimit;    // Bandwidth limiter
        double sr = 0.0;  // Sentinel: must be set by prepare() before use
        uint32_t rngState = 42321u; // For tape hiss

        float lastWowVal = 0.0f;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxWowMs * sampleRate / 1000.0) + 8;
            wowDelay.prepare(maxSamp);
            wowLFO.setShape(StandardLFO::SandH);
            wowLFO.reset();
            flutterLFO.setShape(StandardLFO::Sine);
            flutterLFO.setRate(60.0f, static_cast<float>(sampleRate));
            flutterLFO.reset();
            wowSmooth.reset();
            bwLimit.reset();
        }
        void reset()
        {
            wowDelay.clear();
            wowLFO.reset();
            flutterLFO.reset();
            wowSmooth.reset();
            bwLimit.reset();
            lastWowVal = 0.0f;
        }

        float nextHiss()
        {
            rngState = rngState * 1664525u + 1013904223u;
            return static_cast<float>(static_cast<int32_t>(rngState)) / static_cast<float>(0x7FFFFFFF);
        }

        float process(float in, float wowAmt, float flutterAmt,
                      float failureProb, float srF)
        {
            // Dropout: randomly mute signal based on failure probability
            {
                rngState = rngState * 1664525u + 1013904223u;
                float r = static_cast<float>(rngState & 0xFFFF) / 65535.0f;
                if (r < failureProb * 0.001f) // very sparse dropouts
                    in = 0.0f;
            }

            // S&H wow (rate: 0.3–2Hz), smoothed to slow tape pitch wander
            wowLFO.setRate(0.3f + wowAmt * 1.7f, srF);
            float rawWow = wowLFO.process();
            // Low-pass the S&H output at ~2Hz to get smooth wow
            wowSmooth.setMode(CytomicSVF::Mode::LowPass);
            wowSmooth.setCoefficients(2.0f, 0.5f, srF);
            float smoothWow = flushDenormal(wowSmooth.processSample(rawWow));

            // Flutter at 60Hz
            float flutter = flutterLFO.process() * flutterAmt;

            // Combined pitch modulation → delay time
            float modMs = (smoothWow * wowAmt * 2.0f + flutter * 0.3f);
            modMs = std::max(0.1f, kMaxWowMs * 0.5f + modMs);
            float delaySamp = modMs * srF / 1000.0f;
            delaySamp = std::max(1.0f, std::min(delaySamp, static_cast<float>(wowDelay.getSize()) - 2.0f));

            // Tape hiss: white noise scaled by failure
            float hiss = nextHiss() * failureProb * 0.02f;

            wowDelay.write(in + hiss);

            // Bandwidth limiting (VHS ~8kHz top)
            float bwFreq = 8000.0f - failureProb * 5000.0f; // worse tape = lower BW
            bwFreq = std::max(2000.0f, bwFreq);
            bwLimit.setMode(CytomicSVF::Mode::LowPass);
            bwLimit.setCoefficients(bwFreq, 0.6f, srF);
            return flushDenormal(bwLimit.processSample(wowDelay.read(delaySamp)));
        }
    } genLoss_;

    //==========================================================================
    // Stage 2 — Hyper-Scooped Octave Fuzz (Boss FZ-2) — Mono → Stereo
    //==========================================================================
    struct FZ2Stage
    {
        OversamplingProcessor<8> ovs;
        CytomicSVF  lowShelfL, lowShelfR;
        CytomicSVF  highShelfL, highShelfR;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate, int maxBlockSize)
        {
            sr = sampleRate;
            ovs.prepare(sampleRate, maxBlockSize);
            lowShelfL.reset(); lowShelfR.reset();
            highShelfL.reset(); highShelfR.reset();
        }
        void reset()
        {
            ovs.reset();
            lowShelfL.reset(); lowShelfR.reset();
            highShelfL.reset(); highShelfR.reset();
        }

        void processBlock(const float* monoIn, float* outL, float* outR,
                          int numSamples, float gain, int mode)
        {
            float srF = static_cast<float>(sr);
            // Copy mono to both channels
            std::copy(monoIn, monoIn + numSamples, outL);
            std::copy(monoIn, monoIn + numSamples, outR);

            // FuzzI = full-wave rect; FuzzII = half-wave rect + asymmetric drive
            float driveGain = 1.0f + gain * (mode == 0 ? 80.0f : 60.0f);

            // 8× OVS stereo
            ovs.processStereo(outL, outR, numSamples,
                [&](float* upL, float* upR, int upN)
                {
                    for (int i = 0; i < upN; ++i)
                    {
                        float xL = upL[i];
                        float xR = upR[i];

                        if (mode == 0)
                        {
                            // FuzzI: full-wave rectification (octave up character)
                            xL = std::abs(xL * driveGain);
                            xR = std::abs(xR * driveGain);
                        }
                        else
                        {
                            // FuzzII: asymmetric half-wave + hard clip
                            xL = xL * driveGain;
                            xR = xR * driveGain;
                            xL = (xL > 0.0f) ? xL : xL * 0.1f; // half-wave
                            xR = (xR > 0.0f) ? xR : xR * 0.1f;
                        }
                        // Hard clip
                        xL = std::clamp(xL, -1.0f, 1.0f);
                        xR = std::clamp(xR, -1.0f, 1.0f);
                        upL[i] = xL;
                        upR[i] = xR;
                    }
                });

            // Baxandall tone stack: bass boost + treble boost + implicit mid scoop
            float lowGainDb  =  8.0f; // +8dB low shelf
            float highGainDb = +6.0f; // +6dB high shelf
            // Mid scoop emerges from shelf overlap without explicit parametric notch

            // Coefficients are constant — set once per block, not per sample
            lowShelfL.setMode(CytomicSVF::Mode::LowShelf);
            lowShelfL.setCoefficients(300.0f, 0.7f, srF, lowGainDb);
            highShelfL.setMode(CytomicSVF::Mode::HighShelf);
            highShelfL.setCoefficients(3000.0f, 0.7f, srF, highGainDb);
            lowShelfR.setMode(CytomicSVF::Mode::LowShelf);
            lowShelfR.setCoefficients(300.0f, 0.7f, srF, lowGainDb);
            highShelfR.setMode(CytomicSVF::Mode::HighShelf);
            highShelfR.setCoefficients(3000.0f, 0.7f, srF, highGainDb);

            for (int i = 0; i < numSamples; ++i)
            {
                outL[i] = lowShelfL.processSample(outL[i]);
                outL[i] = highShelfL.processSample(outL[i]);
                outR[i] = lowShelfR.processSample(outR[i]);
                outR[i] = highShelfR.processSample(outR[i]);
            }
        }
    } fz2_;

    //==========================================================================
    // Stage 3 — Zero-Order Hold Decimator (Alesis Bitrman)
    //==========================================================================
    struct BitrmanStage
    {
        float holdValL = 0.0f;
        float holdValR = 0.0f;
        int   holdCount = 0;

        void reset()
        {
            holdValL = holdValR = 0.0f;
            holdCount = 0;
        }
        void processBlock(float* L, float* R, int numSamples,
                          float decimateAmt, float crushBits)
        {
            // Decimate: hold every N samples
            int holdN = 1 + static_cast<int>(decimateAmt * 31.0f); // 1 to 32

            // Bit crush: quantize to 2^bits levels
            float levels = fastPow2(std::max(1.0f, crushBits)); // 2–32767 levels
            float invLevels = (levels > 0.5f) ? (1.0f / levels) : 1.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                // Sample rate reduction
                if (holdCount <= 0)
                {
                    holdValL = L[i];
                    holdValR = R[i];
                    holdCount = holdN;
                }
                else
                {
                    L[i] = holdValL;
                    R[i] = holdValR;
                }
                --holdCount;

                // Bit crush
                L[i] = std::floor(L[i] * levels + 0.5f) * invLevels;
                R[i] = std::floor(R[i] * levels + 0.5f) * invLevels;
            }
        }
    } bitrman_;

    //==========================================================================
    // Stage 4 — Industrial Concrete Reverb (Death By Audio Rooms)
    // 8 taps at prime spacings, cross-feeding, NO allpass, harsh echoes
    //==========================================================================
    struct RoomsStage
    {
        static constexpr int kNumTaps = 8;
        // Prime spacings at 44.1kHz — scaled by sr at runtime
        static constexpr int kBaseTapsL[kNumTaps] = {1013, 1427, 1979, 2557, 3109, 3881, 4637, 5381};
        static constexpr int kBaseTapsR[kNumTaps] = {1103, 1531, 2081, 2663, 3191, 3967, 4733, 5477};

        std::array<FractionalDelay, kNumTaps> delL;
        std::array<FractionalDelay, kNumTaps> delR;
        CytomicSVF  darkFiltL;
        CytomicSVF  darkFiltR;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use
        float feedL = 0.0f;
        float feedR = 0.0f;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            float srScale = static_cast<float>(sampleRate / 44100.0);
            for (int i = 0; i < kNumTaps; ++i)
            {
                int capL = static_cast<int>(kBaseTapsL[i] * srScale) + 4;
                int capR = static_cast<int>(kBaseTapsR[i] * srScale) + 4;
                delL[i].prepare(capL);
                delR[i].prepare(capR);
            }
            darkFiltL.reset();
            darkFiltR.reset();
        }
        void reset()
        {
            for (int i = 0; i < kNumTaps; ++i) { delL[i].clear(); delR[i].clear(); }
            darkFiltL.reset(); darkFiltR.reset();
            feedL = feedR = 0.0f;
        }

        void processBlock(const float* inL, const float* inR,
                          float* outL, float* outR, int numSamples,
                          float decay, float size, float mix, float damp)
        {
            float srF = static_cast<float>(sr);
            float srScale = srF / 44100.0f;
            float fbGain = std::min(0.9f, decay * 0.9f);

            // Dark filter cutoff scales with damp
            float darkFreq = 6000.0f - damp * 4000.0f;
            darkFiltL.setMode(CytomicSVF::Mode::LowPass);
            darkFiltL.setCoefficients(darkFreq, 0.5f, srF);
            darkFiltR.setMode(CytomicSVF::Mode::LowPass);
            darkFiltR.setCoefficients(darkFreq, 0.5f, srF);

            for (int i = 0; i < numSamples; ++i)
            {
                // Write into all tap delays
                for (int t = 0; t < kNumTaps; ++t)
                {
                    // Cross-feed: L taps write L+R, R taps write R+L
                    float writeL = flushDenormal(inL[i] + feedR * fbGain * 0.5f);
                    float writeR = flushDenormal(inR[i] + feedL * fbGain * 0.5f);
                    delL[t].write(writeL);
                    delR[t].write(writeR);
                }

                // Read from all taps
                float wetL = 0.0f, wetR = 0.0f;
                for (int t = 0; t < kNumTaps; ++t)
                {
                    float delaySampL = static_cast<float>(kBaseTapsL[t]) * srScale * size;
                    float delaySampR = static_cast<float>(kBaseTapsR[t]) * srScale * size;
                    delaySampL = std::max(1.0f, delaySampL);
                    delaySampR = std::max(1.0f, delaySampR);
                    wetL += delL[t].read(delaySampL);
                    wetR += delR[t].read(delaySampR);
                }
                wetL /= static_cast<float>(kNumTaps);
                wetR /= static_cast<float>(kNumTaps);

                // Slight LP darkening in feedback path
                feedL = flushDenormal(darkFiltL.processSample(wetL));
                feedR = flushDenormal(darkFiltR.processSample(wetR));

                outL[i] = inL[i] + mix * (wetL - inL[i]);
                outR[i] = inR[i] + mix * (wetR - inR[i]);
            }
        }
    } rooms_;

    //==========================================================================
    // Temporary stereo work buffers
    //==========================================================================
    std::vector<float> tmpL_;
    std::vector<float> tmpR_;

    //==========================================================================
    // Cached parameter pointers (11 params)
    //==========================================================================
    std::atomic<float>* p_genlossWow     = nullptr;
    std::atomic<float>* p_genlossFlutter = nullptr;
    std::atomic<float>* p_genlossFailure = nullptr;
    std::atomic<float>* p_fz2Gain        = nullptr;
    std::atomic<float>* p_fz2Mode        = nullptr;
    std::atomic<float>* p_bitrDecimate   = nullptr;
    std::atomic<float>* p_bitrCrush      = nullptr;
    std::atomic<float>* p_roomsDecay     = nullptr;
    std::atomic<float>* p_roomsSize      = nullptr;
    std::atomic<float>* p_roomsMix       = nullptr;
    std::atomic<float>* p_roomsDamp      = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OutbreakChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    genLoss_.prepare(sampleRate);
    fz2_.prepare(sampleRate, maxBlockSize);
    bitrman_.reset();
    rooms_.prepare(sampleRate);
    tmpL_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    tmpR_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
}

inline void OutbreakChain::reset()
{
    genLoss_.reset();
    fz2_.reset();
    bitrman_.reset();
    rooms_.reset();
}

inline void OutbreakChain::processBlock(const float* monoIn, float* L, float* R,
                                         int numSamples,
                                         double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_genlossWow) return;

    // ParamSnapshot
    const float genlossWow     = p_genlossWow->load(std::memory_order_relaxed);
    const float genlossFlutter = p_genlossFlutter->load(std::memory_order_relaxed);
    const float genlossFailure = p_genlossFailure->load(std::memory_order_relaxed);
    const float fz2Gain        = p_fz2Gain->load(std::memory_order_relaxed);
    const int   fz2Mode        = static_cast<int>(p_fz2Mode->load(std::memory_order_relaxed));
    const float bitrDecimate   = p_bitrDecimate->load(std::memory_order_relaxed);
    const float bitrCrush      = p_bitrCrush->load(std::memory_order_relaxed);
    const float roomsDecay     = p_roomsDecay->load(std::memory_order_relaxed);
    const float roomsSize      = p_roomsSize->load(std::memory_order_relaxed);
    const float roomsMix       = p_roomsMix->load(std::memory_order_relaxed);
    const float roomsDamp      = p_roomsDamp->load(std::memory_order_relaxed);
    float srF = static_cast<float>(sr_);

    // Stage 1: VHS Degradation — mono
    for (int i = 0; i < numSamples; ++i)
        tmpL_[i] = genLoss_.process(monoIn[i], genlossWow, genlossFlutter,
                                     genlossFailure, srF);

    // Stage 2: FZ-2 Fuzz — mono → stereo
    fz2_.processBlock(tmpL_.data(), L, R, numSamples, fz2Gain, fz2Mode);

    // Stage 3: Bitrman decimator — stereo in-place
    bitrman_.processBlock(L, R, numSamples, bitrDecimate, bitrCrush);

    // Stage 4: Rooms reverb — stereo in-place
    // Copy L/R into tmps so we have separate in/out
    std::copy(L, L + numSamples, tmpL_.data());
    std::copy(R, R + numSamples, tmpR_.data());
    rooms_.processBlock(tmpL_.data(), tmpR_.data(), L, R, numSamples,
                        roomsDecay, roomsSize, roomsMix, roomsDamp);
}

inline void OutbreakChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "outb_";
    registerFloat(layout, p + "genlossWow",     p + "GenLoss Wow",     0.0f,  1.0f,  0.4f);
    registerFloat(layout, p + "genlossFlutter", p + "GenLoss Flutter", 0.0f,  1.0f,  0.3f);
    registerFloat(layout, p + "genlossFailure", p + "GenLoss Failure", 0.0f,  1.0f,  0.1f);
    registerFloat(layout, p + "fz2Gain",        p + "FZ2 Gain",        0.0f,  1.0f,  0.7f);
    registerChoice(layout, p + "fz2Mode", p + "FZ2 Mode",
                   {"FuzzI", "FuzzII"}, 0);
    registerFloat(layout, p + "bitrDecimate",   p + "Bitr Decimate",  0.0f,  1.0f,  0.0f);
    registerFloat(layout, p + "bitrCrush",      p + "Bitr Crush",     1.0f,  15.0f, 12.0f);
    registerFloat(layout, p + "roomsDecay",     p + "Rooms Decay",    0.1f,  1.0f,  0.5f);
    registerFloat(layout, p + "roomsSize",      p + "Rooms Size",     0.1f,  1.0f,  0.7f);
    registerFloat(layout, p + "roomsMix",       p + "Rooms Mix",      0.0f,  1.0f,  0.4f);
    registerFloat(layout, p + "roomsDamp",      p + "Rooms Damp",     0.0f,  1.0f,  0.5f);
}

inline void OutbreakChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "outb_";
    p_genlossWow     = cacheParam(apvts, p + "genlossWow");
    p_genlossFlutter = cacheParam(apvts, p + "genlossFlutter");
    p_genlossFailure = cacheParam(apvts, p + "genlossFailure");
    p_fz2Gain        = cacheParam(apvts, p + "fz2Gain");
    p_fz2Mode        = cacheParam(apvts, p + "fz2Mode");
    p_bitrDecimate   = cacheParam(apvts, p + "bitrDecimate");
    p_bitrCrush      = cacheParam(apvts, p + "bitrCrush");
    p_roomsDecay     = cacheParam(apvts, p + "roomsDecay");
    p_roomsSize      = cacheParam(apvts, p + "roomsSize");
    p_roomsMix       = cacheParam(apvts, p + "roomsMix");
    p_roomsDamp      = cacheParam(apvts, p + "roomsDamp");
}

} // namespace xoceanus
