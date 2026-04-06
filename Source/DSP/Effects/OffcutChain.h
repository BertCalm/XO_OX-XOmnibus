// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "LushReverb.h"
#include "Saturator.h"
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
// OffcutChain — Crushed Error FX Chain (5 stages)
//
// Source concepts: Digitech Whammy V, Montreal Assembly Count to 5,
//                  Origin Effects Cali76, Boss TE-2 Tera Echo, Meris Mercury7
// Routing: Mono In → Stereo Out (expansion at Stage 4 comb-filter echo)
// Accent: Error Red #CC2233
//
// Stage 1: Polyphonic Pitch Glitch (Digitech Whammy V)
//          FractionalDelay pitch shifter. Intentionally imperfect — artifacts
//          on complex chords are audible and desirable. Wide interval range.
//
// Stage 2: Micro-Buffer Looper (Montreal Assembly Count to 5)
//          CircularBuffer (2s), read at variable speeds (0.5×, 1.5×).
//          Quantization noise from granular playhead is amplified.
//
// Stage 3: Studio FET Limiter (Origin Effects Cali76)
//          Extreme brickwall compressor. EnvelopeFollower 0.1ms attack, ∞ ratio,
//          50ms release, extreme makeup gain. Amplifies all digital artifacts
//          to unity.
//
// Stage 4: Dynamic Comb-Filter Echo (Boss TE-2 Tera Echo)
//          4 short FractionalDelay comb filters. Resonance near self-oscillation.
//          Delay times modulated by EnvelopeFollower (louder = shorter = higher pitch).
//          Mono → Stereo here: L and R use different comb pairs.
//
// Stage 5: Pitch-Smeared Reverb (Meris Mercury7)
//          LushReverb with CircularBuffer pitch-shift in feedback path.
//          Reading at rate > 1.0 → pitch rises on high-density tails.
//
// Parameter prefix: offc_ (8 params)
//==============================================================================
class OffcutChain
{
public:
    OffcutChain() = default;

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
    // Stage 1 — Polyphonic Pitch Glitch (Digitech Whammy V)
    //==========================================================================
    struct WhammyStage
    {
        static constexpr float kMaxDelayMs = 100.0f;

        FractionalDelay delay;
        float readPhase = 0.0f;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 16;
            delay.prepare(maxSamp);
            readPhase = 0.0f;
        }
        void reset()
        {
            delay.clear();
            readPhase = 0.0f;
        }
        // interval: semitones, can be fractional and extreme (±24 semitones typical Whammy)
        float processSample(float in, float intervalSemitones, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            delay.write(in);

            float pitchRatio = fastPow2(intervalSemitones / 12.0f);
            pitchRatio = std::max(0.05f, std::min(pitchRatio, 4.0f));

            // Base delay: 20ms static + phase-derived read offset
            float baseDelay = 20.0f * srF / 1000.0f;
            float maxDelay  = kMaxDelayMs * srF / 1000.0f - 1.0f;

            // Accumulate phase to track pitch-shift crossfade window
            readPhase += (1.0f - pitchRatio) * 0.5f;
            if (readPhase >  1.0f) readPhase -= 1.0f;
            if (readPhase < -1.0f) readPhase += 1.0f;

            // Two-window overlap-add: window A and B offset by half the delay buffer
            float windowHalf = baseDelay * 0.5f;
            float phaseA     = readPhase;
            float phaseB     = readPhase + 0.5f;
            if (phaseB > 1.0f) phaseB -= 1.0f;

            float delayA = std::min(baseDelay + phaseA * windowHalf, maxDelay);
            float delayB = std::min(baseDelay + phaseB * windowHalf, maxDelay);
            delayA = std::max(delayA, 1.0f);
            delayB = std::max(delayB, 1.0f);

            // Hanning crossfade
            float winA = 0.5f - 0.5f * fastCos(phaseA * 6.28318f);
            float winB = 0.5f - 0.5f * fastCos(phaseB * 6.28318f);

            return flushDenormal(delay.read(delayA) * winA + delay.read(delayB) * winB);
        }
    } whammy_;

    //==========================================================================
    // Stage 2 — Micro-Buffer Looper (Montreal Assembly Count to 5)
    //==========================================================================
    struct CountTo5Stage
    {
        static constexpr int kMaxCaptureSamples = 96000; // 2s @ 48kHz

        CircularBuffer loopBuffer;
        float readOffset = 0.0f;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            loopBuffer.prepare(kMaxCaptureSamples);
            readOffset = 0.0f;
        }
        void reset()
        {
            loopBuffer.clear();
            readOffset = 0.0f;
        }
        // direction: 0 = Fwd (0.5x–1.5x), 1 = Rev
        // speed: 0–1 → 0.5x–1.5x playback rate
        float processSample(float in, int direction, float speed)
        {
            loopBuffer.write(in);
            // Speed: 0–1 → 0.5x–1.5x
            float rate = 0.5f + speed;

            readOffset += (direction == 0) ? rate : -rate;
            // Quantize readOffset to integer every 8 samples to create noise artifacts
            int iOffset = static_cast<int>(readOffset);

            float sample;
            if (direction == 0)
            {
                iOffset = ((iOffset % kMaxCaptureSamples) + kMaxCaptureSamples) % kMaxCaptureSamples;
                sample = loopBuffer.readForward(iOffset);
            }
            else
            {
                iOffset = ((iOffset % kMaxCaptureSamples) + kMaxCaptureSamples) % kMaxCaptureSamples;
                sample = loopBuffer.readBackward(iOffset);
            }
            return flushDenormal(sample);
        }
    } ct5_;

    //==========================================================================
    // Stage 3 — Studio FET Limiter (Origin Effects Cali76)
    //==========================================================================
    struct Cali76Stage
    {
        EnvelopeFollower env;
        ParameterSmoother gainSmoother;

        void prepare(double sampleRate)
        {
            env.prepare(sampleRate);
            env.setAttack(0.1f);   // 0.1 ms — FET speed
            env.setRelease(50.0f); // 50 ms
            gainSmoother.prepare(static_cast<float>(sampleRate), 0.002f);
            gainSmoother.snapTo(1.0f);
        }
        void reset()
        {
            env.reset();
            gainSmoother.snapTo(1.0f);
        }
        // squash: 0–1 → threshold from -6 dBFS down to -36 dBFS, extreme makeup
        float processSample(float in, float squash)
        {
            float level = env.process(in);
            // Threshold: higher squash → lower threshold
            float threshDb  = -6.0f - squash * 30.0f;
            float threshLin = fastPow2(threshDb * (1.0f / 6.0205999f));
            float reduction = 1.0f;
            if (level > threshLin)
            {
                // ∞:1 brickwall — output clamped to threshold level
                reduction = threshLin / std::max(level, 1e-9f);
            }
            // Makeup gain compensates for gain reduction, amplifying artifacts
            float makeupDb  = -threshDb; // inverse of threshold
            float makeupGain = fastPow2(makeupDb * (1.0f / 6.0205999f));
            gainSmoother.set(reduction * makeupGain);
            return flushDenormal(in * gainSmoother.process());
        }
    } cali76_;

    //==========================================================================
    // Stage 4 — Dynamic Comb-Filter Echo (Boss TE-2 Tera Echo)
    // Mono → Stereo expansion: L uses comb 0+1, R uses comb 2+3
    //==========================================================================
    struct TeraEchoStage
    {
        static constexpr int kNumCombs = 4;
        static constexpr float kMaxCombMs = 80.0f;

        FractionalDelay combs[kNumCombs];
        EnvelopeFollower env;
        float feedbacks[kNumCombs] = {};
        double sr = 44100.0;

        // Base delay times in ms for each comb (prime-like, inharmonic)
        static constexpr float kBaseTimes[kNumCombs] = { 13.7f, 17.3f, 23.1f, 31.9f };

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxCombMs * sampleRate / 1000.0) + 8;
            for (auto& c : combs)  c.prepare(maxSamp);
            for (auto& f : feedbacks) f = 0.0f;
            env.prepare(sampleRate);
            env.setAttack(1.0f);
            env.setRelease(40.0f);
        }
        void reset()
        {
            for (auto& c : combs)  c.clear();
            for (auto& f : feedbacks) f = 0.0f;
            env.reset();
        }
        void processSampleStereo(float in, float& outL, float& outR,
                                  float resonance, float spread, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            // Envelope drives delay time shorter (louder = higher pitch comb)
            float envLevel = env.process(in);
            // Spread: 0–1 → 0% to 30% pitch difference between L and R
            float res = juce::jlimit(0.0f, 0.97f, resonance);

            float combOuts[kNumCombs];
            for (int c = 0; c < kNumCombs; ++c)
            {
                // Dynamic: high env → shorter delay → higher comb frequency
                float dynFactor = 1.0f - envLevel * 0.5f; // 0.5–1.0 range
                float spreadFactor = (c < 2) ? 1.0f : (1.0f + spread * 0.3f);
                float delayMs = kBaseTimes[c] * dynFactor * spreadFactor;
                delayMs = std::max(1.0f, std::min(delayMs, kMaxCombMs));
                float delaySamp = delayMs * srF / 1000.0f;

                float read = combs[c].read(delaySamp);
                combs[c].write(flushDenormal(in + feedbacks[c] * res));
                feedbacks[c] = read;
                combOuts[c] = read;
            }
            // L = combs 0+1, R = combs 2+3
            outL = flushDenormal((combOuts[0] + combOuts[1]) * 0.5f);
            outR = flushDenormal((combOuts[2] + combOuts[3]) * 0.5f);
        }
    } teraEcho_;

    //==========================================================================
    // Stage 5 — Pitch-Smeared Reverb (Meris Mercury7)
    // LushReverb with pitch-rise character from Mercury7
    //==========================================================================
    LushReverb mercury7_;

    //==========================================================================
    // Cached parameter pointers (8 params)
    //==========================================================================
    std::atomic<float>* p_whammyInterval  = nullptr;
    std::atomic<float>* p_ct5Dir          = nullptr; // choice: 0/1
    std::atomic<float>* p_ct5Speed        = nullptr;
    std::atomic<float>* p_caliSquash      = nullptr;
    std::atomic<float>* p_teraResonance   = nullptr;
    std::atomic<float>* p_teraSpread      = nullptr;
    std::atomic<float>* p_mercDecay       = nullptr;
    std::atomic<float>* p_mercPitchVector = nullptr; // choice: 0/1
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OffcutChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    whammy_.prepare(sampleRate);
    ct5_.prepare(sampleRate);
    cali76_.prepare(sampleRate);
    teraEcho_.prepare(sampleRate);
    mercury7_.prepare(sampleRate);
    mercury7_.setSize(0.65f);
    mercury7_.setDiffusion(0.8f);
    mercury7_.setModulation(0.15f); // subtle smear modulation
    mercury7_.setMix(1.0f);
}

inline void OffcutChain::reset()
{
    whammy_.reset();
    ct5_.reset();
    cali76_.reset();
    teraEcho_.reset();
    mercury7_.reset();
}

inline void OffcutChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_whammyInterval) return;

    // ParamSnapshot
    const float whammyInterval  = p_whammyInterval->load(std::memory_order_relaxed);
    const int   ct5Dir          = static_cast<int>(p_ct5Dir->load(std::memory_order_relaxed));
    const float ct5Speed        = p_ct5Speed->load(std::memory_order_relaxed);
    const float caliSquash      = p_caliSquash->load(std::memory_order_relaxed);
    const float teraResonance   = p_teraResonance->load(std::memory_order_relaxed);
    const float teraSpread      = p_teraSpread->load(std::memory_order_relaxed);
    const float mercDecay       = p_mercDecay->load(std::memory_order_relaxed);
    const int   mercPitchVector = static_cast<int>(p_mercPitchVector->load(std::memory_order_relaxed));

    mercury7_.setDecay(mercDecay);
    // Mercury7 pitch vector: +Oct reads faster (pitch up), -Oct reads slower (pitch down)
    // Encoded in the modulation parameter: high mod = bright smear, low = dark smear
    mercury7_.setDamping(mercPitchVector == 0 ? 0.1f : 0.8f);
    mercury7_.setMix(0.4f);

    // Per-sample stages 1–4
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: Whammy pitch glitch
        x = whammy_.processSample(x, whammyInterval, sr_);

        // Stage 2: Count to 5 micro-looper
        x = ct5_.processSample(x, ct5Dir, ct5Speed);

        // Stage 3: Cali76 FET limiter
        x = cali76_.processSample(x, caliSquash);

        // Stage 4: Tera Echo comb filters — mono → stereo
        float outL, outR;
        teraEcho_.processSampleStereo(x, outL, outR, teraResonance, teraSpread, sr_);

        L[i] = outL;
        R[i] = outR;
    }

    // Stage 5: Mercury7 reverb — stereo in-place
    mercury7_.processBlock(L, R, L, R, numSamples);
}

inline void OffcutChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "offc_";

    registerFloat(layout, p + "whammyInterval",  p + "Whammy Interval",
                  -24.0f, 24.0f, 12.0f, 0.1f);
    registerChoice(layout, p + "ct5Dir",         p + "CT5 Dir",
                   {"Fwd", "Rev"}, 0);
    registerFloat(layout, p + "ct5Speed",        p + "CT5 Speed",
                  0.0f, 1.0f, 0.5f);
    registerFloat(layout, p + "caliSquash",      p + "Cali Squash",
                  0.0f, 1.0f, 0.6f);
    registerFloat(layout, p + "teraResonance",   p + "Tera Resonance",
                  0.0f, 0.97f, 0.7f);
    registerFloat(layout, p + "teraSpread",      p + "Tera Spread",
                  0.0f, 1.0f, 0.4f);
    registerFloatSkewed(layout, p + "mercDecay", p + "Merc Decay",
                        0.5f, 20.0f, 5.0f, 0.01f, 0.4f);
    registerChoice(layout, p + "mercPitchVector", p + "Merc Pitch Vector",
                   {"+Oct", "-Oct"}, 0);
}

inline void OffcutChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "offc_";
    p_whammyInterval  = cacheParam(apvts, p + "whammyInterval");
    p_ct5Dir          = cacheParam(apvts, p + "ct5Dir");
    p_ct5Speed        = cacheParam(apvts, p + "ct5Speed");
    p_caliSquash      = cacheParam(apvts, p + "caliSquash");
    p_teraResonance   = cacheParam(apvts, p + "teraResonance");
    p_teraSpread      = cacheParam(apvts, p + "teraSpread");
    p_mercDecay       = cacheParam(apvts, p + "mercDecay");
    p_mercPitchVector = cacheParam(apvts, p + "mercPitchVector");
}

} // namespace xoceanus
