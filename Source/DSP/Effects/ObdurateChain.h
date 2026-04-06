// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include "FXChainHelpers.h"
#include "LushReverb.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// ObdurateChain — OBDURATE Oscillating Drone Wall FX Chain (5 stages)
//
// Source concept: OscillatingDroneWall (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 4 Caroline Megabyte)
// Accent: Corroded Iron #6B5050
//
// Stage 1: Self-Oscillating Fuzz Gate (Zoom UF-01 Ultra Fuzz)
//          Saturator Digital + 1-sample feedback loop
//          CytomicSVF BandPass narrow Q — self-oscillates at high gain
// Stage 2: Multi-Stage VCA Phaser (Empress Phaser)
//          CytomicSVF allpass cascade (2/4/8 stages selectable)
//          StandardLFO
// Stage 3: Reverse Delay Trails (Danelectro Back Talk)
//          CircularBuffer 500ms backward chunks, Hann window crossfade
//          CytomicSVF LP at 2.5kHz post-reverse
// Stage 4: Lo-Fi Havoc Delay (Caroline Megabyte)
//          FractionalDelay + Saturator Tube in feedback
//          Havoc mode: feedback → 1.05, pitch rises. Mono → Stereo split here.
// Stage 5: DSP Plate & Width Expansion (EQD Avalanche Run)
//          LushReverb (Dattorro-style FDN) + mid/side width expansion
//
// Parameter prefix: obdr_ (14 params)
//==============================================================================
class ObdurateChain
{
public:
    ObdurateChain() = default;

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
    // Stage 1 — Self-Oscillating Fuzz Gate (Zoom UF-01 Ultra Fuzz)
    //==========================================================================
    struct UF01Stage
    {
        Saturator   hardClip;
        CytomicSVF  bpFilter;
        float       fbState     = 0.0f; // 1-sample feedback
        float       gateEnv     = 0.0f;
        float       gateCoeff   = 0.0f;

        void prepare(double sampleRate)
        {
            hardClip.setMode(Saturator::SaturationMode::Digital);
            hardClip.setDrive(1.0f);
            hardClip.setMix(1.0f);
            hardClip.setOutputGain(0.5f);
            bpFilter.reset();
            fbState   = 0.0f;
            gateEnv   = 0.0f;
            // Gate release coefficient ~20ms
            gateCoeff = fastExp(-1.0f / (0.020f * static_cast<float>(sampleRate)));
        }
        void reset()
        {
            bpFilter.reset();
            fbState = gateEnv = 0.0f;
        }
        float process(float in, float gain, float gate, float oscPitchHz, float srF)
        {
            // Narrow bandpass — resonant pitch for self-oscillation
            float pitchClamped = std::max(30.0f, std::min(oscPitchHz, srF * 0.45f));
            bpFilter.setMode(CytomicSVF::Mode::BandPass);
            bpFilter.setCoefficients(pitchClamped, 0.97f, srF); // very high Q

            // 1-sample feedback loop (self-oscillation at high gain)
            float x = in + fbState * gain;
            hardClip.setDrive(gain);
            x = hardClip.processSample(x);
            x = bpFilter.processSample(x);
            fbState = flushDenormal(x * 0.9f);

            // Gate: envelope follower on input, attenuate output below threshold
            float envInput = std::abs(in);
            gateEnv = flushDenormal(gateEnv * gateCoeff + envInput * (1.0f - gateCoeff));
            float gateGain = gateEnv > gate ? 1.0f : (gateEnv / (gate + 1e-10f));
            gateGain = std::min(gateGain, 1.0f);

            return x * gateGain;
        }
    } uf01_;

    //==========================================================================
    // Stage 2 — Multi-Stage VCA Phaser (Empress Phaser)
    //==========================================================================
    struct EmphasserPhaserStage
    {
        // Up to 8 allpass stages, selectable 2/4/8
        static constexpr int kMaxStages = 8;
        CytomicSVF apf[kMaxStages];
        StandardLFO lfo;

        void prepare(double sampleRate)
        {
            for (int s = 0; s < kMaxStages; ++s)
                apf[s].reset();
            lfo.setShape(StandardLFO::Sine);
            lfo.setRate(0.3f, static_cast<float>(sampleRate));
        }
        void reset()
        {
            for (int s = 0; s < kMaxStages; ++s)
                apf[s].reset();
        }
        float process(float in, int numStages, float rate, float depth, float srF)
        {
            lfo.setRate(rate, srF);
            float modLfo = lfo.process();

            float baseFreq  = 400.0f;
            float freqRange = 3000.0f;
            float freq = baseFreq + (modLfo * 0.5f + 0.5f) * freqRange * depth;
            freq = std::max(20.0f, std::min(freq, srF * 0.45f));

            // Clamp numStages to {2,4,8}
            int stages = 2;
            if (numStages == 1) stages = 4;
            else if (numStages == 2) stages = 8;

            float x = in;
            for (int s = 0; s < stages; ++s)
            {
                apf[s].setMode(CytomicSVF::Mode::AllPass);
                apf[s].setCoefficients(freq * (1.0f + s * 0.3f), 0.5f, srF);
                x = apf[s].processSample(x);
            }

            // Mix phased signal back with original for comb filtering
            return (in + x) * 0.5f;
        }
    } empPhaser_;

    //==========================================================================
    // Stage 3 — Reverse Delay Trails (Danelectro Back Talk)
    //==========================================================================
    struct BackTalkStage
    {
        static constexpr int kChunkSamples = 4096; // ~92ms @ 44.1kHz
        CircularBuffer  recordBuf;
        std::vector<float> revChunk;     // current reversed chunk
        int             chunkReadPos = 0;
        int             chunkFillPos = 0;
        bool            chunkReady   = false;
        CytomicSVF      postLP;
        double          sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int chunkLen = static_cast<int>(sampleRate * 0.5) + 4; // 500ms
            recordBuf.prepare(chunkLen);
            revChunk.assign(static_cast<size_t>(chunkLen), 0.0f);
            chunkReadPos = 0;
            chunkFillPos = 0;
            chunkReady   = false;
            postLP.reset();
        }
        void reset()
        {
            recordBuf.clear();
            std::fill(revChunk.begin(), revChunk.end(), 0.0f);
            chunkReadPos = 0;
            chunkFillPos = 0;
            chunkReady   = false;
            postLP.reset();
        }
        float process(float in, float mix, float srF)
        {
            recordBuf.write(in);
            ++chunkFillPos;

            int chunkLen = static_cast<int>(srF * 0.5f);
            chunkLen = std::max(2, std::min(chunkLen, static_cast<int>(revChunk.size())));

            // When a full chunk is collected, fill the reverse buffer (Hann windowed)
            if (chunkFillPos >= chunkLen)
            {
                chunkFillPos = 0;
                for (int j = 0; j < chunkLen; ++j)
                {
                    float hannT  = static_cast<float>(j) / static_cast<float>(chunkLen - 1);
                    float window = 0.5f - 0.5f * fastSin(
                        (hannT - 0.5f) * 6.28318530718f); // Hann
                    revChunk[j] = recordBuf.readForward(chunkLen - 1 - j) * window;
                }
                chunkReadPos = 0;
                chunkReady   = true;
            }

            float revOut = 0.0f;
            if (chunkReady && chunkReadPos < chunkLen)
            {
                revOut = revChunk[static_cast<size_t>(chunkReadPos++)];
            }

            // Post LP at 2.5kHz
            postLP.setMode(CytomicSVF::Mode::LowPass);
            postLP.setCoefficients(2500.0f, 0.5f, srF);
            revOut = postLP.processSample(revOut);

            return in + mix * (revOut - in);
        }
    } backTalk_;

    //==========================================================================
    // Stage 4 — Lo-Fi Havoc Delay (Caroline Megabyte)
    // Mono → Stereo split here (L/R get slightly different delay times)
    //==========================================================================
    struct MegabyteStage
    {
        FractionalDelay delayL, delayR;
        Saturator       tubeSatL, tubeSatR;
        float           fbL      = 0.0f;
        float           fbR      = 0.0f;
        float           delaySampL = 0.0f;
        float           delaySampR = 0.0f;
        double          sr       = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxBuf = static_cast<int>(sampleRate * 1.5) + 8;
            delayL.prepare(maxBuf);
            delayR.prepare(maxBuf);
            tubeSatL.setMode(Saturator::SaturationMode::Tube);
            tubeSatL.setDrive(0.4f);
            tubeSatL.setMix(0.6f);
            tubeSatL.setOutputGain(0.8f);
            tubeSatR.setMode(Saturator::SaturationMode::Tube);
            tubeSatR.setDrive(0.4f);
            tubeSatR.setMix(0.6f);
            tubeSatR.setOutputGain(0.8f);
            fbL = fbR = 0.0f;
            delaySampL = delaySampR = static_cast<float>(sampleRate) * 0.25f;
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
            fbL = fbR = 0.0f;
        }
        void processBlock(float in, float& outL, float& outR,
                          float delayMs, float degrade, bool havoc, float srF)
        {
            float targetDelaySamples = delayMs * srF / 1000.0f;
            targetDelaySamples = std::max(1.0f, targetDelaySamples);

            float targetFb = 0.6f + degrade * 0.3f; // normal feedback
            if (havoc)
            {
                // Havoc: feedback exceeds unity, delay time decreases (pitch rises)
                targetFb = 1.05f;
                targetDelaySamples *= 0.7f; // shrink delay → pitch up
            }

            float maxDelSamp = static_cast<float>(delayL.getSize()) - 4.0f;
            targetDelaySamples = std::min(targetDelaySamples, maxDelSamp);

            // Smooth delay time (prevents clicks)
            float dtCoeff = fastExp(-1.0f / (0.005f * srF));
            delaySampL = flushDenormal(delaySampL * dtCoeff + targetDelaySamples * (1.0f - dtCoeff));
            delaySampR = flushDenormal(delaySampR * dtCoeff + (targetDelaySamples * 1.02f) * (1.0f - dtCoeff)); // slight R offset for stereo

            // Tube sat in feedback path (degrade = tape wear character)
            tubeSatL.setDrive(degrade);
            tubeSatR.setDrive(degrade);

            float fbInL = flushDenormal(tubeSatL.processSample(fbL * targetFb));
            float fbInR = flushDenormal(tubeSatR.processSample(fbR * targetFb));

            delayL.write(in + fbInL);
            delayR.write(in + fbInR);

            float wetL = delayL.read(delaySampL);
            float wetR = delayR.read(delaySampR);
            fbL = wetL;
            fbR = wetR;

            outL = (in + wetL) * 0.5f;
            outR = (in + wetR) * 0.5f;
        }
    } megabyte_;

    //==========================================================================
    // Stage 5 — DSP Plate & Width Expansion (EQD Avalanche Run)
    //==========================================================================
    struct AvalancheRunStage
    {
        LushReverb plate;

        void prepare(double sampleRate)
        {
            plate.prepare(sampleRate);
        }
        void reset()
        {
            plate.reset();
        }
        void processBlock(float* L, float* R, int numSamples,
                          float decay, float tone, float width)
        {
            // Configure reverb (plate-like: bright, medium diffusion)
            plate.setDecay(decay);
            plate.setDamping(1.0f - tone);
            plate.setSize(0.6f);
            plate.setDiffusion(0.7f);
            plate.setModulation(0.2f);
            plate.setMix(0.35f);
            plate.setWidth(width);

            // Process reverb (in-place safe)
            plate.processBlock(L, R, L, R, numSamples);

            // Mid/side width expansion
            // Boost Side channel by widthGain
            float widthGain = 1.0f + width * 1.5f;
            for (int i = 0; i < numSamples; ++i)
            {
                float mid  = (L[i] + R[i]) * 0.5f;
                float side = (L[i] - R[i]) * 0.5f * widthGain;
                L[i] = mid + side;
                R[i] = mid - side;
            }
        }
    } avalancheRun_;

    //==========================================================================
    // Cached parameter pointers (14 params)
    //==========================================================================
    std::atomic<float>* p_ufGain          = nullptr;
    std::atomic<float>* p_ufGate          = nullptr;
    std::atomic<float>* p_ufOscPitch      = nullptr;
    std::atomic<float>* p_empStages       = nullptr;
    std::atomic<float>* p_empRate         = nullptr;
    std::atomic<float>* p_empDepth        = nullptr;
    std::atomic<float>* p_backtalkTime    = nullptr;
    std::atomic<float>* p_backtalkMix     = nullptr;
    std::atomic<float>* p_megaTime        = nullptr;
    std::atomic<float>* p_megaDegrade     = nullptr;
    std::atomic<float>* p_megaHavoc       = nullptr;
    std::atomic<float>* p_avalancheDecay  = nullptr;
    std::atomic<float>* p_avalancheTone   = nullptr;
    std::atomic<float>* p_avalancheWidth  = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void ObdurateChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    uf01_.prepare(sampleRate);
    empPhaser_.prepare(sampleRate);
    backTalk_.prepare(sampleRate);
    megabyte_.prepare(sampleRate);
    avalancheRun_.prepare(sampleRate);
}

inline void ObdurateChain::reset()
{
    uf01_.reset();
    empPhaser_.reset();
    backTalk_.reset();
    megabyte_.reset();
    avalancheRun_.reset();
}

inline void ObdurateChain::processBlock(const float* monoIn, float* L, float* R,
                                         int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_ufGain) return;

    // ParamSnapshot
    const float ufGain         = p_ufGain->load(std::memory_order_relaxed);
    const float ufGate         = p_ufGate->load(std::memory_order_relaxed);
    const float ufOscPitch     = p_ufOscPitch->load(std::memory_order_relaxed);
    const int   empStages      = static_cast<int>(p_empStages->load(std::memory_order_relaxed));
    const float empRate        = p_empRate->load(std::memory_order_relaxed);
    const float empDepth       = p_empDepth->load(std::memory_order_relaxed);
    const float backtalkTime   = p_backtalkTime->load(std::memory_order_relaxed) * 0.01f;
    (void)backtalkTime; // reserved for future BackTalk trail-time DSP
    const float backtalkMix    = p_backtalkMix->load(std::memory_order_relaxed);
    const float megaTime       = p_megaTime->load(std::memory_order_relaxed);
    const float megaDegrade    = p_megaDegrade->load(std::memory_order_relaxed);
    const bool  megaHavoc      = p_megaHavoc->load(std::memory_order_relaxed) >= 0.5f;
    const float avalancheDecay = p_avalancheDecay->load(std::memory_order_relaxed);
    const float avalancheTone  = p_avalancheTone->load(std::memory_order_relaxed);
    const float avalancheWidth = p_avalancheWidth->load(std::memory_order_relaxed);

    const float srF = static_cast<float>(sr_);

    // Mono pipeline stages 1-3 (write to L as temp buffer)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: UF-01 Self-Oscillating Fuzz Gate
        x = uf01_.process(x, ufGain, ufGate, ufOscPitch, srF);

        // Stage 2: Empress Multi-Stage Phaser
        x = empPhaser_.process(x, empStages, empRate, empDepth, srF);

        // Stage 3: Back Talk Reverse Delay Trails
        x = backTalk_.process(x, backtalkMix, srF);

        L[i] = x;
    }

    // Stage 4: Megabyte Lo-Fi Delay — Mono → Stereo
    for (int i = 0; i < numSamples; ++i)
    {
        float outL, outR;
        megabyte_.processBlock(L[i], outL, outR, megaTime, megaDegrade, megaHavoc, srF);
        L[i] = outL;
        R[i] = outR;
    }

    // Stage 5: Avalanche Run Plate & Width
    avalancheRun_.processBlock(L, R, numSamples, avalancheDecay, avalancheTone, avalancheWidth);
}

inline void ObdurateChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "obdr_";

    registerFloat      (layout, p + "ufGain",         p + "UF-01 Gain",
                        0.0f,    1.0f,   0.6f);
    registerFloat      (layout, p + "ufGate",         p + "UF-01 Gate",
                        0.0f,    0.5f,   0.02f);
    registerFloatSkewed(layout, p + "ufOscPitch",     p + "UF-01 Osc Pitch",
                        30.0f, 1200.0f, 150.0f, 1.0f, 0.3f);
    registerChoice     (layout, p + "empStages",      p + "Empress Stages",
                        {"2 Stage", "4 Stage", "8 Stage"}, 1);
    registerFloatSkewed(layout, p + "empRate",        p + "Empress Rate",
                        0.01f,   10.0f,  0.3f,  0.001f, 0.3f);
    registerFloat      (layout, p + "empDepth",       p + "Empress Depth",
                        0.0f,    1.0f,   0.6f);
    registerFloat      (layout, p + "backtalkTime",   p + "Back Talk Time",
                        0.0f,  100.0f,  50.0f);
    registerFloat      (layout, p + "backtalkMix",    p + "Back Talk Mix",
                        0.0f,    1.0f,   0.4f);
    registerFloatSkewed(layout, p + "megaTime",       p + "Megabyte Time",
                        10.0f, 1200.0f, 400.0f, 1.0f, 0.3f);
    registerFloat      (layout, p + "megaDegrade",    p + "Megabyte Degrade",
                        0.0f,    1.0f,   0.3f);
    registerBool       (layout, p + "megaHavoc",      p + "Megabyte Havoc", false);
    registerFloatSkewed(layout, p + "avalancheDecay", p + "Avalanche Decay",
                        0.5f,   20.0f,   4.0f, 0.01f, 0.4f);
    registerFloat      (layout, p + "avalancheTone",  p + "Avalanche Tone",
                        0.0f,    1.0f,   0.5f);
    registerFloat      (layout, p + "avalancheWidth", p + "Avalanche Width",
                        0.0f,    1.0f,   0.7f);
}

inline void ObdurateChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "obdr_";
    p_ufGain          = cacheParam(apvts, p + "ufGain");
    p_ufGate          = cacheParam(apvts, p + "ufGate");
    p_ufOscPitch      = cacheParam(apvts, p + "ufOscPitch");
    p_empStages       = cacheParam(apvts, p + "empStages");
    p_empRate         = cacheParam(apvts, p + "empRate");
    p_empDepth        = cacheParam(apvts, p + "empDepth");
    p_backtalkTime    = cacheParam(apvts, p + "backtalkTime");
    p_backtalkMix     = cacheParam(apvts, p + "backtalkMix");
    p_megaTime        = cacheParam(apvts, p + "megaTime");
    p_megaDegrade     = cacheParam(apvts, p + "megaDegrade");
    p_megaHavoc       = cacheParam(apvts, p + "megaHavoc");
    p_avalancheDecay  = cacheParam(apvts, p + "avalancheDecay");
    p_avalancheTone   = cacheParam(apvts, p + "avalancheTone");
    p_avalancheWidth  = cacheParam(apvts, p + "avalancheWidth");
}

} // namespace xoceanus
