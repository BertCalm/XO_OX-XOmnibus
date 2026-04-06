// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "FXChainHelpers.h"
#include "LushReverb.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include "../PolyBLEP.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OxymoronChain — OXYMORON Gated Choir FX Chain (4 stages)
//
// Source concept: Gated Choir (Broken Rule Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 2 Rotary Speaker)
// Accent: Contradiction Gold #D4A000
//
// Stage 1: Analog Ring Modulator (Moog MF-102) — PolyBLEP sine carrier × input.
//          Clanging, atonal, bell-like sidebands.
// Stage 2: Rotary Speaker (Boss RT-20) — two cascaded FractionalDelay stages
//          at slightly different StandardLFO rates (Leslie horn + drum simulation).
//          Hard L/R pan. Mono → Stereo here.
// Stage 3: Sidechained Metal Noise Gate (Fortin Zuul) — EnvelopeFollower tracks
//          the ORIGINAL dry input (pre-effects). SchmittTrigger gates Stage 2
//          output. Ultra-fast (<2ms attack/release). Chaos → instant silence.
// Stage 4: Resonant Shimmer Reverb (Strymon NightSky) — LushReverb with pitch
//          shift in feedback (CircularBuffer at 2× read speed = +12 semitones)
//          + CytomicSVF LP in feedback. Stretches gate remnants into crystalline choir.
//
// BROKEN RULE: Hard noise gate AFTER intense modulation — kills the swirl but
// preserves tiny shimmer remnants that Stage 4 then expands into a choir.
//
// Parameter prefix: oxym_ (9 params)
//==============================================================================
class OxymoronChain
{
public:
    OxymoronChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process mono input, writing stereo to L and R.
    // Caller must ensure L and R are separate output buffers.
    void processBlock(const float* monoIn, float* L, float* R, int numSamples);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_        = 44100.0;
    int    blockSize_ = 512;

    // Dry signal tap for sidechain gate
    std::vector<float> dryBuf_;
    // Mono working buffer
    std::vector<float> monoWork_;

    //==========================================================================
    // Stage 1 — Analog Ring Modulator (Moog MF-102)
    // PolyBLEP sine carrier × input signal. DC-blocked output.
    //==========================================================================
    struct RingModStage
    {
        PolyBLEP carrier;
        float    dcState_  = 0.0f;
        double   sr_       = 44100.0;

        void prepare(double sampleRate)
        {
            sr_ = sampleRate;
            carrier.setWaveform(PolyBLEP::Waveform::Sine);
            carrier.setFrequency(100.0f, static_cast<float>(sampleRate));
            dcState_ = 0.0f;
        }
        void reset() { dcState_ = 0.0f; }

        // freqHz: carrier frequency. mix: wet amount [0,1].
        float process(float in, float freqHz, float mix)
        {
            carrier.setFrequency(freqHz, static_cast<float>(sr_));
            const float modOut = in * carrier.processSample();
            // DC block: one-pole highpass at ~10Hz
            dcState_ = flushDenormal(dcState_ * 0.99994f + modOut - in * 0.99994f);
            const float wet = modOut - dcState_;
            return in * (1.0f - mix) + wet * mix;
        }
    } ringMod_;

    //==========================================================================
    // Stage 2 — Rotary Speaker (Boss RT-20)
    // Two cascaded FractionalDelay stages: horn (fast) + drum (slow).
    // Mono → Stereo: horn panned hard L, drum panned hard R; summed at output.
    //==========================================================================
    struct RotaryStage
    {
        // Leslie slow: ~40 rpm (~0.67 Hz), fast: ~400 rpm (~6.67 Hz)
        static constexpr float kSlowHz = 0.67f;
        static constexpr float kFastHz = 6.67f;
        static constexpr float kHornMaxMs  = 5.5f;
        static constexpr float kDrumMaxMs  = 9.0f;

        FractionalDelay hornDelL, hornDelR;
        FractionalDelay drumDelL, drumDelR;
        StandardLFO     hornLFO, drumLFO;
        ParameterSmoother depthSm;
        double sr_ = 44100.0;
        float  targetHz_ = kSlowHz;

        void prepare(double sampleRate)
        {
            sr_ = sampleRate;
            const int hornMax = static_cast<int>(kHornMaxMs * sampleRate / 1000.0) + 8;
            const int drumMax = static_cast<int>(kDrumMaxMs * sampleRate / 1000.0) + 8;
            hornDelL.prepare(hornMax); hornDelR.prepare(hornMax);
            drumDelL.prepare(drumMax); drumDelR.prepare(drumMax);

            hornLFO.setShape(StandardLFO::Sine);
            drumLFO.setShape(StandardLFO::Sine);
            hornLFO.setPhaseOffset(0.0f);
            drumLFO.setPhaseOffset(0.25f); // horn/drum out of phase

            depthSm.prepare(static_cast<float>(sampleRate), 0.010f);
            depthSm.snapTo(1.0f);

            hornLFO.setRate(kSlowHz, static_cast<float>(sampleRate));
            drumLFO.setRate(kSlowHz * 0.6f, static_cast<float>(sampleRate));
        }
        void reset()
        {
            hornDelL.clear(); hornDelR.clear();
            drumDelL.clear(); drumDelR.clear();
            hornLFO.reset();
            drumLFO.reset();
            depthSm.snapTo(1.0f);
        }

        // speed: 0=Slow, 1=Fast. depth [0,1].
        void processStereo(const float* in, float* outL, float* outR, int numSamples,
                           bool fast, float depth)
        {
            const float srF     = static_cast<float>(sr_);
            const float rateHz  = fast ? kFastHz : kSlowHz;
            // Drum is ~1/6 horn speed
            hornLFO.setRate(rateHz, srF);
            drumLFO.setRate(rateHz / 6.0f, srF);
            depthSm.set(depth);

            const float hornCentre = kHornMaxMs * srF / 1000.0f * 0.5f;
            const float drumCentre = kDrumMaxMs * srF / 1000.0f * 0.5f;
            const float hornRange  = hornCentre * 0.85f;
            const float drumRange  = drumCentre * 0.85f;

            for (int i = 0; i < numSamples; ++i)
            {
                const float d      = depthSm.process();
                const float hornM  = hornLFO.process();
                const float drumM  = drumLFO.process();

                const float hornDL = std::max(1.0f, hornCentre + hornM * hornRange * d);
                const float hornDR = std::max(1.0f, hornCentre - hornM * hornRange * d); // opposite phase R
                const float drumDL = std::max(1.0f, drumCentre + drumM * drumRange * d);
                const float drumDR = std::max(1.0f, drumCentre - drumM * drumRange * d);

                hornDelL.write(in[i]); hornDelR.write(in[i]);
                drumDelL.write(in[i]); drumDelR.write(in[i]);

                // Hard pan: horn left, drum right
                outL[i] = hornDelL.read(hornDL) * 0.8f + drumDelL.read(drumDL) * 0.2f;
                outR[i] = hornDelR.read(hornDR) * 0.2f + drumDelR.read(drumDR) * 0.8f;
            }
        }
    } rotary_;

    //==========================================================================
    // Stage 3 — Sidechained Metal Noise Gate (Fortin Zuul)
    // Sidechain = original DRY input (passed through separately).
    // Ultra-fast (<2ms) SchmittTrigger gate on Stage 2 stereo output.
    //==========================================================================
    struct NoiseGateStage
    {
        EnvelopeFollower scEnv;      // sidechain envelope
        SchmittTrigger   schmitt;
        ParameterSmoother gainSmL, gainSmR; // <2ms smoothing for click suppression

        void prepare(double sampleRate)
        {
            scEnv.prepare(sampleRate);
            scEnv.setAttack(0.5f);   // 0.5ms — very fast
            scEnv.setRelease(1.5f);  // 1.5ms
            // Photo-fast smoothing to avoid actual clicks while being perceptually abrupt
            gainSmL.prepare(static_cast<float>(sampleRate), 0.001f);
            gainSmR.prepare(static_cast<float>(sampleRate), 0.001f);
            gainSmL.snapTo(1.0f);
            gainSmR.snapTo(1.0f);
            schmitt.reset();
        }
        void reset()
        {
            scEnv.reset();
            schmitt.reset();
            gainSmL.snapTo(1.0f);
            gainSmR.snapTo(1.0f);
        }

        // thresh: gate threshold [0,1] (maps to signal level 0..0.5)
        // releaseMs: additional hold/release past the sidechain
        void processStereo(float* L, float* R, const float* drySidechain, int numSamples,
                           float thresh, float releaseMs)
        {
            // Map thresh [0,1] → Schmitt thresholds
            const float hi = std::max(0.001f, thresh * 0.5f);
            const float lo = hi * 0.6f;
            schmitt.setThresholds(lo, hi);
            scEnv.setRelease(releaseMs);

            for (int i = 0; i < numSamples; ++i)
            {
                const float scLevel = scEnv.process(drySidechain[i]);
                // SchmittTrigger: true when above threshold (gate open)
                const float env = scLevel;
                const bool  open = (env >= hi) || (!schmitt.process(drySidechain[i]) && env > lo);
                const float target = open ? 1.0f : 0.0f;

                gainSmL.set(target);
                gainSmR.set(target);
                L[i] *= gainSmL.process();
                R[i] *= gainSmR.process();
            }
        }
    } noisGate_;

    //==========================================================================
    // Stage 4 — Resonant Shimmer Reverb (Strymon NightSky)
    // LushReverb + pitch-shift in feedback (CircularBuffer at 2× = +12 semi)
    // + CytomicSVF LP in feedback. Crystalline choir from gate remnants.
    //==========================================================================
    struct ShimmerReverbStage
    {
        LushReverb  reverb;
        // Pitch-shift feedback via circular buffer: reading at 2× speed = +12 semi
        CircularBuffer pitchBufL, pitchBufR;
        CytomicSVF     fbLPL, fbLPR;
        float   fbStateL_   = 0.0f;
        float   fbStateR_   = 0.0f;
        float   pitchPhaseL_= 0.0f;
        float   pitchPhaseR_= 0.0f;
        int     pitchBufLen_= 4096;
        double  sr_         = 44100.0;

        void prepare(double sampleRate)
        {
            sr_ = sampleRate;
            reverb.prepare(sampleRate);
            reverb.setSize(0.7f);
            reverb.setDiffusion(0.7f);
            reverb.setDamping(0.2f);
            reverb.setModulation(0.2f);
            reverb.setWidth(1.0f);

            // Buffer for pitch-shift: 2048 samples @ 44.1kHz ≈ 46ms grain
            pitchBufLen_ = static_cast<int>(2048.0 * sampleRate / 44100.0) + 4;
            pitchBufL.prepare(pitchBufLen_);
            pitchBufR.prepare(pitchBufLen_);
            fbLPL.setMode(CytomicSVF::Mode::LowPass);
            fbLPR.setMode(CytomicSVF::Mode::LowPass);
            reset();
        }
        void reset()
        {
            reverb.reset();
            pitchBufL.clear();
            pitchBufR.clear();
            fbStateL_ = fbStateR_ = 0.0f;
            pitchPhaseL_ = pitchPhaseR_ = 0.0f;
            fbLPL.reset();
            fbLPR.reset();
        }

        // pitchSemitones: +0..+24 (default +12 = one octave up = 2× speed)
        // decaySec: reverb decay
        // mix: wet amount [0,1]
        void processStereo(float* L, float* R, int numSamples,
                           float decaySec, float pitchSemitones, float mix)
        {
            const float srF    = static_cast<float>(sr_);
            const float fb     = 0.55f; // fixed shimmer feedback level
            const float ratio  = fastPow2(pitchSemitones / 12.0f); // speed ratio

            reverb.setDecay(decaySec);
            reverb.setMix(1.0f); // 100% wet for shimmer tail

            // LP cutoff in feedback: prevents high-freq buildup
            fbLPL.setCoefficients(6000.0f, 0.4f, srF);
            fbLPR.setCoefficients(6000.0f, 0.4f, srF);

            // Temp stereo buffers (use in-place with feedback injection)
            for (int i = 0; i < numSamples; ++i)
            {
                // Write dry + feedback into pitch buffer
                pitchBufL.write(flushDenormal(L[i] + fbStateL_ * fb));
                pitchBufR.write(flushDenormal(R[i] + fbStateR_ * fb));

                // Read at ratio speed (2× for +12 semi)
                pitchPhaseL_ += ratio;
                pitchPhaseR_ += ratio;

                // Clamp phase to buffer size
                const float bufF = static_cast<float>(pitchBufLen_);
                while (pitchPhaseL_ >= bufF) pitchPhaseL_ -= bufF;
                while (pitchPhaseR_ >= bufF) pitchPhaseR_ -= bufF;

                const int idxL = static_cast<int>(pitchPhaseL_) % pitchBufLen_;
                const int idxR = static_cast<int>(pitchPhaseR_) % pitchBufLen_;
                float shiftL   = pitchBufL.readForward(idxL);
                float shiftR   = pitchBufR.readForward(idxR);

                // LP filter in feedback path
                shiftL = fbLPL.processSample(shiftL);
                shiftR = fbLPR.processSample(shiftR);
                fbStateL_ = flushDenormal(shiftL);
                fbStateR_ = flushDenormal(shiftR);

                L[i] = L[i] * (1.0f - mix) + shiftL * mix;
                R[i] = R[i] * (1.0f - mix) + shiftR * mix;
            }

            // Run LushReverb over pitch-shifted signal
            reverb.processBlock(L, R, L, R, numSamples);
        }
    } shimmer_;

    //==========================================================================
    // Cached parameter pointers (9 params)
    //==========================================================================
    std::atomic<float>* p_ringFreq       = nullptr;
    std::atomic<float>* p_ringMix        = nullptr;
    std::atomic<float>* p_rotarySpeed    = nullptr;
    std::atomic<float>* p_rotaryDepth    = nullptr;
    std::atomic<float>* p_gateThresh     = nullptr;
    std::atomic<float>* p_gateRelease    = nullptr;
    std::atomic<float>* p_shimmerDecay   = nullptr;
    std::atomic<float>* p_shimmerPitch   = nullptr;
    std::atomic<float>* p_shimmerMix     = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OxymoronChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    dryBuf_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    monoWork_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    ringMod_.prepare(sampleRate);
    rotary_.prepare(sampleRate);
    noisGate_.prepare(sampleRate);
    shimmer_.prepare(sampleRate);
}

inline void OxymoronChain::reset()
{
    ringMod_.reset();
    rotary_.reset();
    noisGate_.reset();
    shimmer_.reset();
}

inline void OxymoronChain::processBlock(const float* monoIn, float* L, float* R, int numSamples)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_ringFreq) return;

    // ParamSnapshot
    const float ringFreq       = p_ringFreq->load(std::memory_order_relaxed);
    const float ringMix        = p_ringMix->load(std::memory_order_relaxed);
    const bool  rotaryFast     = p_rotarySpeed->load(std::memory_order_relaxed) >= 0.5f;
    const float rotaryDepth    = p_rotaryDepth->load(std::memory_order_relaxed);
    const float gateThresh     = p_gateThresh->load(std::memory_order_relaxed);
    const float gateRelease    = p_gateRelease->load(std::memory_order_relaxed);
    const float shimmerDecay   = p_shimmerDecay->load(std::memory_order_relaxed);
    const float shimmerPitch   = p_shimmerPitch->load(std::memory_order_relaxed);
    const float shimmerMix     = p_shimmerMix->load(std::memory_order_relaxed);

    // Capture dry signal for sidechain BEFORE any processing
    std::copy(monoIn, monoIn + numSamples, dryBuf_.data());

    // Stage 1 — Ring Modulator → mono into monoWork_
    for (int i = 0; i < numSamples; ++i)
        monoWork_[i] = ringMod_.process(monoIn[i], ringFreq, ringMix * 0.01f);

    // Stage 2 — Rotary Speaker (mono→stereo split)
    rotary_.processStereo(monoWork_.data(), L, R, numSamples, rotaryFast, rotaryDepth);

    // Stage 3 — Noise Gate (sidechained to original dry input)
    noisGate_.processStereo(L, R, dryBuf_.data(), numSamples, gateThresh, gateRelease);

    // Stage 4 — Shimmer Reverb (expands gate remnants into crystalline choir)
    shimmer_.processStereo(L, R, numSamples, shimmerDecay, shimmerPitch, shimmerMix * 0.01f);
}

inline void OxymoronChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "oxym_";

    registerFloatSkewed(layout, p + "ringFreq",     "Ring Freq",      20.0f,  4000.0f,  220.0f, 1.0f,  0.4f);
    registerFloat      (layout, p + "ringMix",      "Ring Mix",        0.0f,   100.0f,   70.0f, 0.1f);
    registerFloat      (layout, p + "rotarySpeed",  "Rotary Speed",    0.0f,     1.0f,    0.0f, 1.0f); // 0=Slow,1=Fast
    registerFloat      (layout, p + "rotaryDepth",  "Rotary Depth",    0.0f,     1.0f,    0.7f, 0.001f);
    registerFloat      (layout, p + "gateThresh",   "Gate Thresh",     0.0f,     1.0f,    0.3f, 0.001f);
    registerFloatSkewed(layout, p + "gateRelease",  "Gate Release",    0.5f,   200.0f,   10.0f, 0.1f,  0.4f);
    registerFloatSkewed(layout, p + "shimmerDecay", "Shimmer Decay",   0.5f,    20.0f,    6.0f, 0.01f, 0.4f);
    registerFloat      (layout, p + "shimmerPitch", "Shimmer Pitch",   0.0f,    24.0f,   12.0f, 0.1f);
    registerFloat      (layout, p + "shimmerMix",   "Shimmer Mix",     0.0f,   100.0f,   60.0f, 0.1f);
}

inline void OxymoronChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "oxym_";
    p_ringFreq     = cacheParam(apvts, p + "ringFreq");
    p_ringMix      = cacheParam(apvts, p + "ringMix");
    p_rotarySpeed  = cacheParam(apvts, p + "rotarySpeed");
    p_rotaryDepth  = cacheParam(apvts, p + "rotaryDepth");
    p_gateThresh   = cacheParam(apvts, p + "gateThresh");
    p_gateRelease  = cacheParam(apvts, p + "gateRelease");
    p_shimmerDecay = cacheParam(apvts, p + "shimmerDecay");
    p_shimmerPitch = cacheParam(apvts, p + "shimmerPitch");
    p_shimmerMix   = cacheParam(apvts, p + "shimmerMix");
}

} // namespace xoceanus
