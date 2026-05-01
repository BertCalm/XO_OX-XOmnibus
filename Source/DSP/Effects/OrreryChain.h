// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "../ParameterSmoother.h"
#include "../StandardLFO.h"
#include "FXChainHelpers.h"
#include "LushReverb.h"
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
// OrreryChain — Frozen Diamond FX Chain (5 stages)
//
// Source concept: Frozen Diamond (Ambient/Shimmer Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 3 DC-2 Chorus)
// Accent: Spectral Ice #A8D8EA
//
// Stage 1: Optical Audio Freeze (Gamechanger Plus)
//          CircularBuffer continuously recording. Bool freeze param triggers
//          crossfade lock of last 200ms into seamless loop.
// Stage 2: Synth/Reverse Swell (Digitech XP300)
//          CircularBuffer (1s), read backward at 2× speed (+12 semitones).
//          Creates shimmering reverse swells. Mix with dry/frozen signal.
// Stage 3: Spatial Dimension Chorus (Boss DC-2) — Mono → Stereo here.
//          Two FractionalDelay modulated by two StandardLFOs at 180° offset.
//          Hard L/R pan. No feedback. 4 exclusive mode buttons.
// Stage 4: Cascading Multi-Tap Delay (Meris Polymoon)
//          Single long FractionalDelay with 6 read pointers.
//          CytomicSVF allpass diffusers per tap for smear.
//          Pitch-shifting in feedback via read-rate manipulation.
// Stage 5: Resonant Synthesizer Reverb (Strymon NightSky)
//          LushReverb + CytomicSVF LP in feedback loop with
//          StandardLFO modulating cutoff (slow breathing, exposed via
//          orry_breathRate at 0.005–1 Hz; default 0.1 Hz).
//
// Parameter prefix: orry_ (11 params)
//==============================================================================
class OrreryChain
{
public:
    OrreryChain() = default;

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
    // Stage 1 — Optical Audio Freeze (Gamechanger Plus)
    //==========================================================================
    struct FreezeStage
    {
        static constexpr float kLoopMs = 200.0f;

        CircularBuffer rec;        // Continuously recording input
        CircularBuffer frozenLoop; // Snapshot of the frozen 200ms loop
        bool   frozen_      = false;
        bool   wasFrozen_   = false;
        float  xfadePos_    = 0.0f; // 0 = fully live, 1 = fully frozen
        float  xfadeSpeed_  = 0.0f;
        int    loopPos_     = 0;
        int    loopLen_     = 0;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int recCap = static_cast<int>(kLoopMs * 2.0f * sampleRate / 1000.0) + 4;
            rec.prepare(recCap);
            loopLen_ = static_cast<int>(kLoopMs * sampleRate / 1000.0);
            frozenLoop.prepare(loopLen_ + 4);
            xfadeSpeed_ = static_cast<float>(1.0 / (sampleRate * 0.020)); // 20ms crossfade
        }
        void reset()
        {
            rec.clear();
            frozenLoop.clear();
            frozen_    = false;
            wasFrozen_ = false;
            xfadePos_  = 0.0f;
            loopPos_   = 0;
        }

        float process(float in, bool freezeOn, float decayTail)
        {
            rec.write(in);

            // Detect freeze toggle: snapshot into frozenLoop on rising edge
            if (freezeOn && !wasFrozen_)
            {
                // Copy last loopLen_ samples into frozenLoop
                for (int j = 0; j < loopLen_; ++j)
                    frozenLoop.write(rec.readForward(loopLen_ - 1 - j));
                loopPos_ = 0;
            }
            wasFrozen_ = freezeOn;
            frozen_    = freezeOn;

            // Crossfade xfadePos_: 0 = live, 1 = frozen
            float target = frozen_ ? 1.0f : 0.0f;
            xfadePos_ += (target - xfadePos_) * xfadeSpeed_ * 64.0f;
            xfadePos_ = flushDenormal(xfadePos_);
            xfadePos_ = std::max(0.0f, std::min(xfadePos_, 1.0f));

            // Read from frozen loop (seamless looping)
            float frozenSamp = frozenLoop.readForward(loopPos_);
            loopPos_ = (loopPos_ + 1) % std::max(1, loopLen_);

            // Decay tail: when not fully frozen, frozen signal fades out
            float effectiveFrozen = frozenSamp * xfadePos_ * (0.5f + decayTail * 0.5f);
            float live = in * (1.0f - xfadePos_);
            return live + effectiveFrozen;
        }
    } freeze_;

    //==========================================================================
    // Stage 2 — Synth/Reverse Swell (Digitech XP300)
    // Read backward at 2× speed = +12 semitones upward shift
    //==========================================================================
    struct XP300Stage
    {
        static constexpr float kBufSec = 1.0f;

        CircularBuffer buf;
        float readPos_ = 0.0f;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use
        int    bufSize_ = 0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            bufSize_ = static_cast<int>(kBufSec * sampleRate) + 4;
            buf.prepare(bufSize_);
            readPos_ = 0.0f;
        }
        void reset()
        {
            buf.clear();
            readPos_ = 0.0f;
        }
        float process(float in, float reverseMix, float swellTimeMs)
        {
            buf.write(in);

            // swellTimeMs (50–800 ms) controls the reverse loop window length.
            // readPos_ wraps within this window so shorter values create tighter,
            // faster-cycling reverse swells; longer values give slower, wider sweeps.
            float loopSamples = std::max(1.0f,
                std::min(swellTimeMs * static_cast<float>(sr) / 1000.0f,
                         static_cast<float>(bufSize_)));

            // Read backward at 2× speed (creates +12 semitone pitch with reverse characteristic)
            readPos_ -= 2.0f;
            if (readPos_ < 0.0f)
                readPos_ += loopSamples;

            int iPos = static_cast<int>(readPos_) % std::max(1, bufSize_);
            float reversed = buf.readBackward(iPos);

            return in * (1.0f - reverseMix) + reversed * reverseMix;
        }
    } xp300_;

    //==========================================================================
    // Stage 3 — Spatial Dimension Chorus (Boss DC-2) — Mono → Stereo
    // 4 preset modes (hardcoded depth/rate pairs), 180° phase offset between channels
    //==========================================================================
    struct DC2Stage
    {
        static constexpr float kMaxDelayMs = 15.0f;

        FractionalDelay delayL;
        FractionalDelay delayR;
        StandardLFO     lfoL;
        StandardLFO     lfoR;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        // DC-2 mode presets: {rate Hz, depth ms}
        static constexpr float kModeRates[4]  = {0.5f, 0.9f, 1.5f, 2.5f};
        static constexpr float kModeDepths[4] = {2.0f, 3.5f, 5.5f, 7.5f};

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            delayL.prepare(maxSamp);
            delayR.prepare(maxSamp);
            lfoL.setShape(StandardLFO::Sine);
            lfoR.setShape(StandardLFO::Sine);
            lfoR.setPhaseOffset(0.5f); // 180° offset for spatial width
            lfoL.reset();
            lfoR.reset();
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
            lfoL.reset();
            lfoR.reset();
        }

        void process(float in, float& outL, float& outR, int mode)
        {
            float srF = static_cast<float>(sr);
            int m = std::max(0, std::min(mode, 3));
            float rateHz  = kModeRates[m];
            float depthMs = kModeDepths[m];

            lfoL.setRate(rateHz, srF);
            lfoR.setRate(rateHz, srF);

            float modL = lfoL.process(); // [-1,+1]
            float modR = lfoR.process();

            float centreMs   = kMaxDelayMs * 0.5f;
            float delayMsL   = centreMs + modL * depthMs;
            float delayMsR   = centreMs + modR * depthMs;
            float delaySampL = std::max(1.0f, delayMsL * srF / 1000.0f);
            float delaySampR = std::max(1.0f, delayMsR * srF / 1000.0f);
            delaySampL = std::min(delaySampL, static_cast<float>(delayL.getSize()) - 2.0f);
            delaySampR = std::min(delaySampR, static_cast<float>(delayR.getSize()) - 2.0f);

            delayL.write(in);
            delayR.write(in);

            // No feedback — pure chorus
            float wetL = delayL.read(delaySampL);
            float wetR = delayR.read(delaySampR);

            // Hard L/R pan: left channel gets left chorus, right channel gets right
            outL = (in + wetL) * 0.5f;
            outR = (in + wetR) * 0.5f;
        }
    } dc2_;

    //==========================================================================
    // Stage 4 — Cascading Multi-Tap Delay (Meris Polymoon)
    // 6 read pointers with allpass diffusion per tap, pitch shift in feedback
    //==========================================================================
    struct PolymoonStage
    {
        static constexpr float kMaxMs   = 2000.0f;
        static constexpr int   kMaxTaps = 6;

        FractionalDelay delayL;
        FractionalDelay delayR;
        // Allpass diffusers per tap
        std::array<CytomicSVF, kMaxTaps> diffL;
        std::array<CytomicSVF, kMaxTaps> diffR;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        // Pitch shift via read-rate manipulation
        // (read slightly faster = pitch up; slower = pitch down)
        float feedbackPitchPhaseL = 0.0f;
        float feedbackPitchPhaseR = 0.0f;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxMs * sampleRate / 1000.0) + 16;
            delayL.prepare(maxSamp);
            delayR.prepare(maxSamp);
            for (auto& d : diffL) d.reset();
            for (auto& d : diffR) d.reset();
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
            for (auto& d : diffL) d.reset();
            for (auto& d : diffR) d.reset();
            feedbackPitchPhaseL = feedbackPitchPhaseR = 0.0f;
        }

        void processBlock(const float* inL, const float* inR,
                          float* outL, float* outR, int numSamples,
                          float delayMs, int numTaps, float smear)
        {
            float srF   = static_cast<float>(sr);
            float baseS = std::max(1.0f, delayMs * srF / 1000.0f);
            baseS = std::min(baseS, static_cast<float>(delayL.getSize()) - 4.0f);
            int nTaps = std::max(1, std::min(numTaps, kMaxTaps));

            // Spacing: taps at baseS/nTaps, 2*baseS/nTaps, ... baseS
            float tapSpacing = baseS / static_cast<float>(nTaps);

            for (int i = 0; i < numSamples; ++i)
            {
                // Write input
                // Pitch shift in feedback: read at slightly different rate
                // Accumulate phase offset to shift pitch ±small amount
                feedbackPitchPhaseL += 0.01f; // ~+small pitch nudge
                feedbackPitchPhaseR += 0.01f;
                if (feedbackPitchPhaseL > baseS) feedbackPitchPhaseL -= baseS;
                if (feedbackPitchPhaseR > baseS) feedbackPitchPhaseR -= baseS;

                delayL.write(flushDenormal(inL[i]));
                delayR.write(flushDenormal(inR[i]));

                float wetL = 0.0f, wetR = 0.0f;
                for (int t = 0; t < nTaps; ++t)
                {
                    float tapDelaySL = tapSpacing * static_cast<float>(t + 1);
                    float tapDelaySR = tapSpacing * static_cast<float>(t + 1);
                    // L channel: slight pitch offset via phase
                    tapDelaySL += feedbackPitchPhaseL * 0.002f;
                    tapDelaySL = std::max(1.0f, std::min(tapDelaySL, baseS));
                    tapDelaySR = std::max(1.0f, std::min(tapDelaySR, baseS));

                    float tapL = delayL.read(tapDelaySL);
                    float tapR = delayR.read(tapDelaySR);

                    // Allpass diffuser smears each tap
                    float diffFreq = 400.0f + static_cast<float>(t) * 300.0f;
                    diffL[t].setMode(CytomicSVF::Mode::AllPass);
                    diffL[t].setCoefficients(diffFreq, smear * 0.7f, srF);
                    diffR[t].setMode(CytomicSVF::Mode::AllPass);
                    diffR[t].setCoefficients(diffFreq, smear * 0.7f, srF);
                    tapL = diffL[t].processSample(tapL);
                    tapR = diffR[t].processSample(tapR);

                    // Cascade: each tap feeds the next via amplitude taper
                    float taper = 1.0f / static_cast<float>(t + 1);
                    wetL += tapL * taper;
                    wetR += tapR * taper;
                }

                outL[i] = inL[i] + wetL * (1.0f / static_cast<float>(nTaps));
                outR[i] = inR[i] + wetR * (1.0f / static_cast<float>(nTaps));
            }
        }
    } polymoon_;

    //==========================================================================
    // Stage 5 — Resonant Synthesizer Reverb (Strymon NightSky)
    // LushReverb + slow LP breathing via LFO (rate exposed as
    // orry_breathRate, 0.005–1 Hz; default 0.1 Hz)
    //==========================================================================
    struct NightSkyStage
    {
        LushReverb   reverb;
        CytomicSVF   resonantFilter;
        StandardLFO  breathLFO;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            reverb.prepare(sampleRate);
            resonantFilter.reset();
            breathLFO.setShape(StandardLFO::Sine);
            breathLFO.setRate(0.1f, static_cast<float>(sampleRate));
            breathLFO.reset();
        }
        void reset()
        {
            reverb.reset();
            resonantFilter.reset();
            breathLFO.reset();
        }

        void processBlock(const float* inL, const float* inR,
                          float* outL, float* outR, int numSamples,
                          float decaySec, float filterCentre, float breathRateHz)
        {
            float srF = static_cast<float>(sr);
            reverb.setDecay(decaySec);
            reverb.setSize(0.9f);
            reverb.setDamping(0.2f);
            reverb.setDiffusion(0.8f);
            reverb.setModulation(0.4f);
            reverb.setMix(1.0f); // wet only — caller handles blend

            // Process reverb
            reverb.processBlock(inL, inR, outL, outR, numSamples);

            // Apply resonant LP with LFO breathing in the output
            // LFO sweeps the filter cutoff for the "resonant synthesizer" quality
            // D005: breathRateHz is now exposed via orry_breathRate (default
            // 0.1 Hz preserves the original 10s cycle; floor 0.005 Hz lets
            // user dial up to 200s for long-form drift). Per-block setRate
            // (LFO advances per-sample inside the inner loop, so the rate
            // applies correctly).
            breathLFO.setRate(breathRateHz, srF);
            for (int i = 0; i < numSamples; ++i)
            {
                float lfoMod = breathLFO.process(); // [-1, +1]
                float cutoff = filterCentre * fastPow2(lfoMod * 0.8f); // ±0.8 octave sweep
                cutoff = std::max(200.0f, std::min(cutoff, srF * 0.4f));

                resonantFilter.setMode(CytomicSVF::Mode::LowPass);
                resonantFilter.setCoefficients(cutoff, 0.5f, srF);
                outL[i] = flushDenormal(resonantFilter.processSample(outL[i]));
                outR[i] = flushDenormal(resonantFilter.processSample(outR[i]));
            }
        }
    } nightSky_;

    //==========================================================================
    // Temporary stereo work buffers
    //==========================================================================
    std::vector<float> tmpL_;
    std::vector<float> tmpR_;
    std::vector<float> tmp2L_;
    std::vector<float> tmp2R_;

    //==========================================================================
    // Cached parameter pointers
    //==========================================================================
    std::atomic<float>* p_freezeSustain  = nullptr;
    std::atomic<float>* p_freezeDecay    = nullptr;
    std::atomic<float>* p_xpRevMix       = nullptr;
    std::atomic<float>* p_xpSwellTime    = nullptr;
    std::atomic<float>* p_dimMode        = nullptr;
    std::atomic<float>* p_polyTime       = nullptr;
    std::atomic<float>* p_polyTaps       = nullptr;
    std::atomic<float>* p_polySmear      = nullptr;
    std::atomic<float>* p_nightskyDecay  = nullptr;
    std::atomic<float>* p_nightskyFilter = nullptr;
    std::atomic<float>* p_breathRate     = nullptr; // D005: exposed slow-mod rate (0.005–1 Hz)
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OrreryChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    freeze_.prepare(sampleRate);
    xp300_.prepare(sampleRate);
    dc2_.prepare(sampleRate);
    polymoon_.prepare(sampleRate);
    nightSky_.prepare(sampleRate);
    tmpL_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    tmpR_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    tmp2L_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    tmp2R_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
}

inline void OrreryChain::reset()
{
    freeze_.reset();
    xp300_.reset();
    dc2_.reset();
    polymoon_.reset();
    nightSky_.reset();
}

inline void OrreryChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples,
                                       double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_freezeSustain) return;

    // ParamSnapshot
    const bool  freezeSustain  = p_freezeSustain->load(std::memory_order_relaxed) > 0.5f;
    const float freezeDecay    = p_freezeDecay->load(std::memory_order_relaxed);
    const float xpRevMix       = p_xpRevMix->load(std::memory_order_relaxed);
    const float xpSwellTime    = p_xpSwellTime->load(std::memory_order_relaxed);
    const int   dimMode        = static_cast<int>(p_dimMode->load(std::memory_order_relaxed));
    const float polyTime       = p_polyTime->load(std::memory_order_relaxed);
    const int   polyTaps       = static_cast<int>(p_polyTaps->load(std::memory_order_relaxed));
    const float polySmear      = p_polySmear->load(std::memory_order_relaxed);
    const float nightskyDecay  = p_nightskyDecay->load(std::memory_order_relaxed);
    const float nightskyFilter = p_nightskyFilter->load(std::memory_order_relaxed);
    const float breathRate     = p_breathRate->load(std::memory_order_relaxed);

    // Stage 1: Freeze — mono, writes to tmpL_
    for (int i = 0; i < numSamples; ++i)
        tmpL_[i] = freeze_.process(monoIn[i], freezeSustain, freezeDecay);

    // Stage 2: Reverse Swell — mono
    for (int i = 0; i < numSamples; ++i)
        tmpL_[i] = xp300_.process(tmpL_[i], xpRevMix, xpSwellTime);

    // Stage 3: DC-2 Chorus → stereo expansion
    for (int i = 0; i < numSamples; ++i)
    {
        float xL = 0.0f, xR = 0.0f;
        dc2_.process(tmpL_[i], xL, xR, dimMode);
        tmp2L_[i] = xL;
        tmp2R_[i] = xR;
    }

    // Stage 4: Polymoon multi-tap delay (stereo)
    polymoon_.processBlock(tmp2L_.data(), tmp2R_.data(), tmpL_.data(), tmpR_.data(),
                           numSamples, polyTime, polyTaps, polySmear);

    // Stage 5: NightSky resonant reverb (stereo)
    nightSky_.processBlock(tmpL_.data(), tmpR_.data(), L, R, numSamples,
                           nightskyDecay, nightskyFilter, breathRate);
}

inline void OrreryChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "orry_";
    registerBool(layout,  p + "freezeSustain",  p + "Freeze Sustain",  false);
    registerFloat(layout, p + "freezeDecay",    p + "Freeze Decay Tail", 0.0f,  1.0f,  0.7f);
    registerFloat(layout, p + "xpRevMix",       p + "XP Reverse Mix",  0.0f,  1.0f,  0.4f);
    registerFloat(layout, p + "xpSwellTime",    p + "XP Swell Time",  50.0f, 800.0f, 300.0f);
    registerChoice(layout, p + "dimMode", p + "Dim Mode",
                   {"1", "2", "3", "4"}, 1);
    registerFloat(layout, p + "polyTime",       p + "Poly Time",      50.0f, 1500.0f, 400.0f);
    registerChoice(layout, p + "polyTaps", p + "Poly Taps",
                   {"1", "2", "3", "4", "5", "6"}, 3);
    registerFloat(layout, p + "polySmear",      p + "Poly Smear",     0.0f,  1.0f,  0.5f);
    registerFloat(layout, p + "nightskyDecay",  p + "NightSky Decay", 0.5f, 20.0f,  6.0f);
    registerFloat(layout, p + "nightskyFilter", p + "NightSky Filter",400.0f, 8000.0f, 2000.0f);
    // D005 (must breathe): exposes the NightSky reverb's breath LFO rate.
    // Default 0.1 Hz preserves the original 10-second cycle; floor 0.005 Hz
    // matches StandardLFO::setRate's internal clamp. Skewed because the
    // range spans 2+ decades.
    registerFloatSkewed(layout, p + "breathRate", p + "NightSky Breath Rate",
                        0.005f, 1.0f, 0.1f, 0.001f, 0.3f);
}

inline void OrreryChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "orry_";
    p_freezeSustain  = cacheParam(apvts, p + "freezeSustain");
    p_freezeDecay    = cacheParam(apvts, p + "freezeDecay");
    p_xpRevMix       = cacheParam(apvts, p + "xpRevMix");
    p_xpSwellTime    = cacheParam(apvts, p + "xpSwellTime");
    p_dimMode        = cacheParam(apvts, p + "dimMode");
    p_polyTime       = cacheParam(apvts, p + "polyTime");
    p_polyTaps       = cacheParam(apvts, p + "polyTaps");
    p_polySmear      = cacheParam(apvts, p + "polySmear");
    p_nightskyDecay  = cacheParam(apvts, p + "nightskyDecay");
    p_nightskyFilter = cacheParam(apvts, p + "nightskyFilter");
    p_breathRate     = cacheParam(apvts, p + "breathRate");
}

} // namespace xoceanus
