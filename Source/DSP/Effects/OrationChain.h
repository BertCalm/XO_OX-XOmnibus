// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "LushReverb.h"
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
// OrationChain — Post-Delay Auto-Wah FX Chain (4 stages)
//
// Source concepts: Meris Hedra, Chase Bliss Warped Vinyl,
//                  DOD FX25 Envelope Filter, Neunaber Immerse Reverberator
// Routing: Mono In → Stereo Out (expansion at Stage 3 envelope filter)
// Accent: Sermon Amber #D4881A
//
// Stage 1: 3-Voice Rhythmic Pitch Shifter (Meris Hedra)
//          3 FractionalDelay lines with pitch shifting via read-rate manipulation.
//          Delay 1 = 8th note (+5th, ×1.5 freq ratio), Delay 2 = dotted 8th
//          (+Octave, ×2.0), Delay 3 = quarter note (−4th, ×0.75).
//          Host-synced via bpm parameter.
//
// Stage 2: Analog Pitch Vibrato (Chase Bliss Warped Vinyl)
//          Short FractionalDelay modulated by triangle StandardLFO for
//          subtle pitch wobble and chorus shimmer.
//
// Stage 3: Sub-Thump Envelope Filter (DOD FX25)
//          EnvelopeFollower drives CytomicSVF LP cutoff. Each pitch-shifted
//          tap triggers its own EnvelopeFollower, creating cascading vowel sweeps.
//          Mono → Stereo expansion here (L = taps 1+2, R = taps 2+3).
//
// Stage 4: Immersive Reverb (Neunaber Immerse)
//          LushReverb clean FDN — no modulation, pure space.
//
// Parameter prefix: orat_ (10 params)
//==============================================================================
class OrationChain
{
public:
    OrationChain() = default;

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
    // Stage 1 — 3-Voice Rhythmic Pitch Shifter (Meris Hedra)
    // Each tap read-head uses a different pitch ratio and delay time.
    //==========================================================================
    struct HedraStage
    {
        static constexpr float kMaxDelayMs = 2000.0f;
        static constexpr int   kNumTaps    = 3;

        FractionalDelay delayLine; // shared write buffer, 3 read heads
        double sr = 44100.0;

        // Pitch ratios per tap (in semitones for storage, converted per block)
        // Tap 0: +5th   (7 semitones  → ×1.4983…  ≈ ×1.5)
        // Tap 1: +Oct   (12 semitones → ×2.0)
        // Tap 2: -4th   (-5 semitones → ×0.7491…  ≈ ×0.75)
        static constexpr float kDefaultSemitones[kNumTaps] = { 7.0f, 12.0f, -5.0f };

        // Per-tap state for pitch-shifting via variable read rate
        float readPhase[kNumTaps] = { 0.0f, 0.0f, 0.0f };
        float tapOut[kNumTaps]    = { 0.0f, 0.0f, 0.0f };

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 16;
            delayLine.prepare(maxSamp);
            for (auto& p : readPhase) p = 0.0f;
            for (auto& o : tapOut)   o = 0.0f;
        }
        void reset()
        {
            delayLine.clear();
            for (auto& p : readPhase) p = 0.0f;
            for (auto& o : tapOut)   o = 0.0f;
        }

        // Returns output in tapOut[0..2]. Call before reading tapOut.
        // semitones[] = per-tap pitch shift overrides (from params or defaults).
        // bpm used to calculate note-division delay times.
        void processSample(float in, double bpm,
                           float semi0, float semi1, float semi2,
                           double sampleRate)
        {
            // Write input into shared delay line
            delayLine.write(in);

            double safeBpm = (bpm > 0.0) ? bpm : 120.0;
            double beatSecs   = 60.0 / safeBpm;
            double eighthSecs = beatSecs * 0.5;
            double dotEighth  = beatSecs * 0.75;
            double quarterSec = beatSecs;

            // Delay times for each tap (in samples)
            float delayTimes[kNumTaps] = {
                static_cast<float>(eighthSecs * sampleRate),
                static_cast<float>(dotEighth  * sampleRate),
                static_cast<float>(quarterSec * sampleRate)
            };
            float semis[kNumTaps] = { semi0, semi1, semi2 };

            float maxDelay = static_cast<float>(delayLine.getSize() - 4);

            for (int t = 0; t < kNumTaps; ++t)
            {
                float pitchRatio = fastPow2(semis[t] / 12.0f);
                float dt = std::min(delayTimes[t], maxDelay);
                dt = std::max(dt, 1.0f);
                // Simple crossfading pitch shift: two read heads overlap-add
                // modulate read position to shift pitch
                float basePos = dt;
                float shiftedPos = dt / std::max(pitchRatio, 0.01f);
                shiftedPos = std::min(shiftedPos, maxDelay);
                shiftedPos = std::max(shiftedPos, 1.0f);
                // Cross-fade between base and shifted reads via phase
                readPhase[t] += (pitchRatio - 1.0f) * 0.002f;
                if (readPhase[t] > 1.0f) readPhase[t] -= 1.0f;
                if (readPhase[t] < 0.0f) readPhase[t] += 1.0f;
                float xf = 0.5f - 0.5f * fastSin(readPhase[t] * 6.28318f);
                float a  = delayLine.read(basePos);
                float b  = delayLine.read(shiftedPos);
                tapOut[t] = flushDenormal(a * xf + b * (1.0f - xf));
            }
        }
    } hedra_;

    //==========================================================================
    // Stage 2 — Analog Pitch Vibrato (Chase Bliss Warped Vinyl)
    //==========================================================================
    struct WarpedVinylStage
    {
        static constexpr float kMaxDelayMs = 30.0f;
        FractionalDelay delayL, delayR;
        StandardLFO lfo;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            delayL.prepare(maxSamp);
            delayR.prepare(maxSamp);
            lfo.setShape(StandardLFO::Triangle);
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
        }
        float processSampleMono(float in, float rate, float depth, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            lfo.setRate(rate, srF);
            float mod = lfo.process();
            // Depth: 0–1 → 0–10 ms of modulation range
            float delayMs   = 5.0f + mod * depth * 5.0f;
            float delaySamp = delayMs * srF / 1000.0f;
            delaySamp = std::max(1.0f, std::min(delaySamp,
                                 kMaxDelayMs * srF / 1000.0f - 1.0f));
            delayL.write(in);
            return flushDenormal(delayL.read(delaySamp));
        }
    } warpedVinyl_;

    //==========================================================================
    // Stage 3 — Sub-Thump Envelope Filter (DOD FX25)
    // One EnvelopeFollower per tap. Mono → Stereo here.
    //==========================================================================
    struct FX25Stage
    {
        static constexpr int kNumTaps = 3;
        EnvelopeFollower envFollowers[kNumTaps];
        CytomicSVF filters[kNumTaps];
        ParameterSmoother freqSmootherL, freqSmootherR;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            for (int t = 0; t < kNumTaps; ++t)
            {
                envFollowers[t].prepare(sampleRate);
                envFollowers[t].setAttack(2.0f);
                envFollowers[t].setRelease(80.0f);
                filters[t].reset();
            }
            freqSmootherL.prepare(static_cast<float>(sampleRate), 0.005f);
            freqSmootherR.prepare(static_cast<float>(sampleRate), 0.005f);
            freqSmootherL.snapTo(200.0f);
            freqSmootherR.snapTo(200.0f);
        }
        void reset()
        {
            for (int t = 0; t < kNumTaps; ++t)
            {
                envFollowers[t].reset();
                filters[t].reset();
            }
        }
        void processSampleStereo(const float tapIn[kNumTaps],
                                  float& outL, float& outR,
                                  float sensitivity, float range, float resonance,
                                  double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);
            // Base cutoff: 80 Hz; sensitivity drives how far up in freq
            float baseCutoff = 80.0f;
            // range: 0–1 → up to 8 octaves of sweep
            float maxCutoff  = baseCutoff * fastPow2(range * 8.0f);
            maxCutoff = std::min(maxCutoff, srF * 0.44f);

            float res = juce::jlimit(0.0f, 0.95f, resonance);

            for (int t = 0; t < kNumTaps; ++t)
            {
                float env = envFollowers[t].process(tapIn[t] * sensitivity * 4.0f);
                float cutoff = baseCutoff + env * (maxCutoff - baseCutoff);
                filters[t].setMode(CytomicSVF::Mode::LowPass);
                filters[t].setCoefficients(cutoff, res, srF);
            }

            // L = tap 0 + tap 1 (8th + dotted-8th)
            // R = tap 1 + tap 2 (dotted-8th + quarter)
            // filters[1] is stateful (CytomicSVF) — call exactly once and cache
            float filtL0  = filters[0].processSample(tapIn[0]);
            float filt1   = filters[1].processSample(tapIn[1]); // cached; shared by L and R
            float filtR2  = filters[2].processSample(tapIn[2]);

            outL = flushDenormal((filtL0 + filt1) * 0.5f);
            outR = flushDenormal((filt1  + filtR2) * 0.5f);
        }
    } fx25_;

    //==========================================================================
    // Stage 4 — Immersive Reverb (Neunaber Immerse)
    //==========================================================================
    LushReverb reverb_;

    //==========================================================================
    // Cached parameter pointers (10 params)
    //==========================================================================
    std::atomic<float>* p_hedraPitch1  = nullptr;
    std::atomic<float>* p_hedraPitch2  = nullptr;
    std::atomic<float>* p_hedraPitch3  = nullptr;
    std::atomic<float>* p_warpRate     = nullptr;
    std::atomic<float>* p_warpDepth    = nullptr;
    std::atomic<float>* p_fx25Sens     = nullptr;
    std::atomic<float>* p_fx25Range    = nullptr;
    std::atomic<float>* p_fx25Res      = nullptr;
    std::atomic<float>* p_verbDecay    = nullptr;
    std::atomic<float>* p_verbMix      = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OrationChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    hedra_.prepare(sampleRate);
    warpedVinyl_.prepare(sampleRate);
    fx25_.prepare(sampleRate);
    reverb_.prepare(sampleRate);
    reverb_.setSize(0.6f);
    reverb_.setDiffusion(0.7f);
    reverb_.setModulation(0.0f); // no modulation — Neunaber Immerse is clean
    reverb_.setMix(1.0f);        // full wet from stage; caller controls dry/wet
}

inline void OrationChain::reset()
{
    hedra_.reset();
    warpedVinyl_.reset();
    fx25_.reset();
}

inline void OrationChain::processBlock(const float* monoIn, float* L, float* R,
                                        int numSamples, double bpm, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_hedraPitch1) return;

    // ParamSnapshot
    const float hedraPitch1 = p_hedraPitch1->load(std::memory_order_relaxed);
    const float hedraPitch2 = p_hedraPitch2->load(std::memory_order_relaxed);
    const float hedraPitch3 = p_hedraPitch3->load(std::memory_order_relaxed);
    const float warpRate    = p_warpRate->load(std::memory_order_relaxed);
    const float warpDepth   = p_warpDepth->load(std::memory_order_relaxed);
    const float fx25Sens    = p_fx25Sens->load(std::memory_order_relaxed);
    const float fx25Range   = p_fx25Range->load(std::memory_order_relaxed);
    const float fx25Res     = p_fx25Res->load(std::memory_order_relaxed);
    const float verbDecay   = p_verbDecay->load(std::memory_order_relaxed);
    const float verbMix     = p_verbMix->load(std::memory_order_relaxed);

    reverb_.setDecay(verbDecay);
    reverb_.setMix(verbMix);

    // Per-sample pipeline: Stages 1–3
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: Hedra pitch shifter — fills tapOut[0..2]
        hedra_.processSample(x, bpm, hedraPitch1, hedraPitch2, hedraPitch3, sr_);

        // Stage 2: Warped Vinyl vibrato — apply to each tap output
        float tapsVibed[3];
        for (int t = 0; t < 3; ++t)
            tapsVibed[t] = warpedVinyl_.processSampleMono(hedra_.tapOut[t], warpRate, warpDepth, sr_);

        // Stage 3: FX25 envelope filter — stereo expansion
        float outL, outR;
        fx25_.processSampleStereo(tapsVibed, outL, outR, fx25Sens, fx25Range, fx25Res, sr_);

        L[i] = outL;
        R[i] = outR;
    }

    // Stage 4: LushReverb — stereo in-place
    reverb_.processBlock(L, R, L, R, numSamples);
}

inline void OrationChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "orat_";

    registerFloat(layout, p + "hedraPitch1", p + "Hedra Pitch 1",
                  -24.0f, 24.0f, 7.0f, 0.1f);
    registerFloat(layout, p + "hedraPitch2", p + "Hedra Pitch 2",
                  -24.0f, 24.0f, 12.0f, 0.1f);
    registerFloat(layout, p + "hedraPitch3", p + "Hedra Pitch 3",
                  -24.0f, 24.0f, -5.0f, 0.1f);
    registerFloatSkewed(layout, p + "warpRate",  p + "Warp Rate",
                        0.05f, 5.0f, 0.3f, 0.001f, 0.35f);
    registerFloat(layout, p + "warpDepth",   p + "Warp Depth",
                  0.0f, 1.0f, 0.25f);
    registerFloat(layout, p + "fx25Sens",    p + "FX25 Sens",
                  0.0f, 1.0f, 0.5f);
    registerFloat(layout, p + "fx25Range",   p + "FX25 Range",
                  0.0f, 1.0f, 0.6f);
    registerFloat(layout, p + "fx25Res",     p + "FX25 Res",
                  0.0f, 0.95f, 0.4f);
    registerFloatSkewed(layout, p + "verbDecay", p + "Verb Decay",
                        0.5f, 20.0f, 3.5f, 0.01f, 0.4f);
    registerFloat(layout, p + "verbMix",     p + "Verb Mix",
                  0.0f, 1.0f, 0.35f);
}

inline void OrationChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "orat_";
    p_hedraPitch1 = cacheParam(apvts, p + "hedraPitch1");
    p_hedraPitch2 = cacheParam(apvts, p + "hedraPitch2");
    p_hedraPitch3 = cacheParam(apvts, p + "hedraPitch3");
    p_warpRate    = cacheParam(apvts, p + "warpRate");
    p_warpDepth   = cacheParam(apvts, p + "warpDepth");
    p_fx25Sens    = cacheParam(apvts, p + "fx25Sens");
    p_fx25Range   = cacheParam(apvts, p + "fx25Range");
    p_fx25Res     = cacheParam(apvts, p + "fx25Res");
    p_verbDecay   = cacheParam(apvts, p + "verbDecay");
    p_verbMix     = cacheParam(apvts, p + "verbMix");
}

} // namespace xoceanus
