// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
#include "FXChainHelpers.h"
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
// OvershootChain — OVERSHOOT Error Cascade FX Chain (4 stages)
//
// Source concept: Error Cascade (Broken Rule Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 3 Flanger)
// Accent: Glitch Cyan #00E5FF
//
// Stage 1: Pristine Digital Delay (Strymon TimeLine) — clean FractionalDelay,
//          dotted-8th or user-set time. 50/50 mix creates overlapping
//          "polyphony" that intentionally confuses Stage 2.
// Stage 2: Monophonic Guitar Synth (Korg X-911) — EnvelopeFollower +
//          zero-crossing pitch tracker with intentionally low hysteresis.
//          Tracks erratically on the polyphonic Stage 1 output.
//          PolyBLEP square at tracked pitch with simple ADSR envelope.
// Stage 3: Jet-Plane Flanger (A/DA Flanger) — short FractionalDelay,
//          StandardLFO modulation, extreme feedback (~95%). CytomicSVF
//          comb sweeps. Mono → Stereo here (opposite LFO phases L/R).
// Stage 4: Granular Freeze (Hologram Microcosm) — CircularBuffer capture,
//          100ms Gaussian-windowed grains at 50% playback speed.
//          Freeze bool param locks the capture buffer.
//
// BROKEN RULE: Delay BEFORE the pitch tracker — polyphonic input confuses
// the monophonic tracker, creating systematic pitch errors as feature.
//
// Parameter prefix: ovsh_ (10 params)
//==============================================================================
class OvershootChain
{
public:
    OvershootChain() = default;

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

    // Mono working buffer (allocated in prepare)
    std::vector<float> monoWork_;

    //==========================================================================
    // Stage 1 — Pristine Digital Delay (Strymon TimeLine)
    // Clean FractionalDelay, 50/50 wet mix.
    //==========================================================================
    struct DigitalDelayStage
    {
        static constexpr float kMaxDelayMs = 2000.0f;
        FractionalDelay del;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            del.prepare(maxSamples);
        }
        void reset() { del.clear(); }

        void process(const float* in, float* out, int numSamples,
                     float delayMs, float feedback)
        {
            const float srF   = static_cast<float>(sr);
            const float maxD  = static_cast<float>(del.getSize()) - 4.0f;
            const float dSmp  = std::max(1.0f, std::min(delayMs * srF / 1000.0f, maxD));
            const float fb    = juce::jlimit(0.0f, 0.90f, feedback);

            for (int i = 0; i < numSamples; ++i)
            {
                const float wet = del.read(dSmp);
                del.write(flushDenormal(in[i] + wet * fb));
                // 50/50 mix: wet mixed with dry to create "polyphony"
                out[i] = in[i] * 0.5f + wet * 0.5f;
            }
        }
    } digitalDelay_;

    //==========================================================================
    // Stage 2 — Monophonic Guitar Synth (Korg X-911)
    // EnvelopeFollower + zero-crossing pitch tracker (low hysteresis = erratic
    // tracking on polyphonic input). PolyBLEP square at tracked pitch.
    //==========================================================================
    struct GuitarSynthStage
    {
        EnvelopeFollower env;
        PolyBLEP         osc;

        // Pitch tracker state
        float  lastSample_     = 0.0f;
        float  trackedHz_      = 110.0f;
        int    zeroCrossCount_ = 0;
        int    periodSamples_  = 400;
        float  trackingError_  = 0.5f; // hysteresis reduction factor (0=normal, 1=max chaos)
        double sr_             = 44100.0;

        // Simple ADSR (linear)
        float envLevel_ = 0.0f;
        bool  noteOn_   = false;

        void prepare(double sampleRate)
        {
            sr_ = sampleRate;
            env.prepare(sampleRate);
            env.setAttack(1.0f);
            env.setRelease(80.0f);
            osc.setWaveform(PolyBLEP::Waveform::Square);
            osc.setFrequency(110.0f, static_cast<float>(sampleRate));
            reset();
        }
        void reset()
        {
            env.reset();
            osc.reset();
            lastSample_ = 0.0f;
            trackedHz_  = 110.0f;
            zeroCrossCount_ = 0;
            periodSamples_  = 400;
            envLevel_   = 0.0f;
            noteOn_     = false;
        }

        void setTrackingError(float errorAmount)
        {
            // errorAmount [0,1]: 0 = minimal hysteresis (most chaotic), 1 = full hysteresis
            trackingError_ = errorAmount;
        }

        float process(float in, float wave, double sampleRate)
        {
            const float srF = static_cast<float>(sampleRate);
            // Envelope
            const float level = env.process(in);

            // Zero-crossing period measurement with intentional hysteresis reduction
            // Hysteresis threshold is inversely proportional to trackingError_
            const float hysteresis = 0.005f * (1.0f - trackingError_) + 0.0001f;
            const bool zeroCross = (lastSample_ < -hysteresis && in >= hysteresis)
                                || (lastSample_ >  hysteresis && in <= -hysteresis);
            lastSample_ = in;

            if (zeroCross)
            {
                if (zeroCrossCount_ > 0)
                {
                    // Update tracked period from half-cycle (× 2 for full period)
                    periodSamples_ = std::max(4, zeroCrossCount_ * 2);
                    trackedHz_ = srF / static_cast<float>(periodSamples_);
                    trackedHz_ = juce::jlimit(20.0f, 4000.0f, trackedHz_);
                    osc.setFrequency(trackedHz_, srF);
                }
                zeroCrossCount_ = 0;
            }
            else
            {
                ++zeroCrossCount_;
            }

            // wave param selects between square (0) and triangle (1)
            osc.setWaveform(wave < 0.5f ? PolyBLEP::Waveform::Square : PolyBLEP::Waveform::Triangle);
            const float oscOut = osc.processSample();
            return oscOut * level;
        }
    } guitarSynth_;

    //==========================================================================
    // Stage 3 — Jet-Plane Flanger (A/DA Flanger)
    // Short FractionalDelay, StandardLFO, extreme feedback.
    // Mono → Stereo: opposite LFO phases for L and R.
    //==========================================================================
    struct FlangerStage
    {
        static constexpr float kMaxFlangeMs = 12.0f;
        FractionalDelay delL, delR;
        StandardLFO     lfoL, lfoR;
        CytomicSVF      combSVFL, combSVFR;
        ParameterSmoother depthSmootherL, depthSmootherR;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxFlangeMs * sampleRate / 1000.0) + 8;
            delL.prepare(maxSamples);
            delR.prepare(maxSamples);

            lfoL.setShape(StandardLFO::Sine);
            lfoR.setShape(StandardLFO::Sine);
            lfoL.setPhaseOffset(0.0f);
            lfoR.setPhaseOffset(0.5f); // opposite phase for stereo image

            combSVFL.setMode(CytomicSVF::Mode::AllPass);
            combSVFR.setMode(CytomicSVF::Mode::AllPass);

            depthSmootherL.prepare(static_cast<float>(sampleRate), 0.005f);
            depthSmootherR.prepare(static_cast<float>(sampleRate), 0.005f);
            depthSmootherL.snapTo(4.0f);
            depthSmootherR.snapTo(4.0f);

            reset();
        }
        void reset()
        {
            delL.clear();
            delR.clear();
            combSVFL.reset();
            combSVFR.reset();
            depthSmootherL.snapTo(4.0f);
            depthSmootherR.snapTo(4.0f);
        }

        void processStereo(const float* in, float* outL, float* outR, int numSamples,
                           float rate, float depth, float resonance, double sampleRate)
        {
            const float srF    = static_cast<float>(sampleRate);
            const float maxD   = kMaxFlangeMs * srF / 1000.0f - 1.0f;
            const float fb     = juce::jlimit(0.0f, 0.97f, resonance); // extreme feedback
            const float centre = maxD * 0.5f;

            lfoL.setRate(rate, srF);
            lfoR.setRate(rate, srF);
            depthSmootherL.set(depth);
            depthSmootherR.set(depth);

            for (int i = 0; i < numSamples; ++i)
            {
                const float dL = depthSmootherL.process();
                const float dR = depthSmootherR.process();
                const float modL = lfoL.process();
                const float modR = lfoR.process();

                float delSmpL = std::max(1.0f, centre + modL * dL * srF / 1000.0f);
                float delSmpR = std::max(1.0f, centre + modR * dR * srF / 1000.0f);
                delSmpL = std::min(delSmpL, maxD);
                delSmpR = std::min(delSmpR, maxD);

                const float wetL = delL.read(delSmpL);
                const float wetR = delR.read(delSmpR);

                delL.write(flushDenormal(in[i] + wetL * fb));
                delR.write(flushDenormal(in[i] + wetR * fb));

                // AllPass comb for extra spectral sweep
                combSVFL.setCoefficients(srF / (delSmpL * 2.0f), 0.6f, srF);
                combSVFR.setCoefficients(srF / (delSmpR * 2.0f), 0.6f, srF);
                outL[i] = combSVFL.processSample(in[i] + wetL);
                outR[i] = combSVFR.processSample(in[i] + wetR);
            }
        }
    } flanger_;

    //==========================================================================
    // Stage 4 — Granular Freeze (Hologram Microcosm)
    // CircularBuffer capture, 100ms Gaussian-windowed grains, 50% speed.
    // Freeze bool locks capture buffer.
    //==========================================================================
    struct GranularFreezeStage
    {
        static constexpr int kGrainSamples = 4410; // ~100ms @ 44.1kHz (scaled at prepare)
        static constexpr int kBufSamples   = 88200; // ~2s

        CircularBuffer captureBufL, captureBufR;
        bool   frozen_        = false;
        float  grainPhaseL_   = 0.0f;
        float  grainPhaseR_   = 0.0f;
        float  grainInc_      = 0.5f; // 50% playback speed
        int    grainLen_      = kGrainSamples;
        int    captureLenL_   = kBufSamples;
        int    captureLenR_   = kBufSamples;
        double sr_            = 44100.0;

        void prepare(double sampleRate)
        {
            sr_ = sampleRate;
            const float scale = static_cast<float>(sampleRate / 44100.0);
            grainLen_    = std::max(4, static_cast<int>(kGrainSamples * scale));
            captureLenL_ = std::max(8, static_cast<int>(kBufSamples  * scale));
            captureLenR_ = captureLenL_;
            captureBufL.prepare(captureLenL_);
            captureBufR.prepare(captureLenR_);
            reset();
        }
        void reset()
        {
            captureBufL.clear();
            captureBufR.clear();
            grainPhaseL_ = grainPhaseR_ = 0.0f;
            frozen_ = false;
        }

        // Gaussian window approximation: 0.5*(1 - cos(2π*t/N))  (raised-cosine Hann)
        static float gaussianWin(float phase)
        {
            constexpr float kTwoPi = 6.28318530717958647692f;
            return 0.5f * (1.0f - fastCos(kTwoPi * phase));
        }

        void processStereo(float* L, float* R, int numSamples,
                           bool freeze, float pitchRatio, float mix)
        {
            frozen_ = freeze;
            const float inc = std::max(0.01f, std::min(pitchRatio, 4.0f)) * 0.5f; // half-speed base

            for (int i = 0; i < numSamples; ++i)
            {
                // Only write to capture buffer when not frozen
                if (!frozen_)
                {
                    captureBufL.write(L[i]);
                    captureBufR.write(R[i]);
                }

                // Grain read position (interpolated)
                const float normPhL = grainPhaseL_;
                const float normPhR = grainPhaseR_;
                const float winL    = gaussianWin(normPhL);
                const float winR    = gaussianWin(normPhR);

                // Read offset into capture buffer
                const int readOffL = static_cast<int>(normPhL * static_cast<float>(grainLen_));
                const int readOffR = static_cast<int>(normPhR * static_cast<float>(grainLen_));
                const float grainL = captureBufL.readForward(readOffL) * winL;
                const float grainR = captureBufR.readForward(readOffR) * winR;

                // Advance grain phase
                grainPhaseL_ += inc / static_cast<float>(grainLen_);
                grainPhaseR_ += inc / static_cast<float>(grainLen_);
                if (grainPhaseL_ >= 1.0f) grainPhaseL_ -= 1.0f;
                if (grainPhaseR_ >= 1.0f) grainPhaseR_ -= 1.0f;

                L[i] = L[i] * (1.0f - mix) + grainL * mix;
                R[i] = R[i] * (1.0f - mix) + grainR * mix;
            }
        }
    } granFreeze_;

    //==========================================================================
    // Cached parameter pointers (10 params)
    //==========================================================================
    std::atomic<float>* p_delayTime          = nullptr;
    std::atomic<float>* p_delayFdbk          = nullptr;
    std::atomic<float>* p_synthTrackingError = nullptr;
    std::atomic<float>* p_synthWave          = nullptr;
    std::atomic<float>* p_flangeRate         = nullptr;
    std::atomic<float>* p_flangeDepth        = nullptr;
    std::atomic<float>* p_flangeResonance    = nullptr;
    std::atomic<float>* p_granFreeze         = nullptr;
    std::atomic<float>* p_granPitch          = nullptr;
    std::atomic<float>* p_granMix            = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OvershootChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    monoWork_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    digitalDelay_.prepare(sampleRate);
    guitarSynth_.prepare(sampleRate);
    flanger_.prepare(sampleRate);
    granFreeze_.prepare(sampleRate);
}

inline void OvershootChain::reset()
{
    digitalDelay_.reset();
    guitarSynth_.reset();
    flanger_.reset();
    granFreeze_.reset();
}

inline void OvershootChain::processBlock(const float* monoIn, float* L, float* R, int numSamples)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_delayTime) return;

    // ParamSnapshot
    const float delayTime          = p_delayTime->load(std::memory_order_relaxed);
    const float delayFdbk          = p_delayFdbk->load(std::memory_order_relaxed);
    const float synthTrackingError = p_synthTrackingError->load(std::memory_order_relaxed);
    const float synthWave          = p_synthWave->load(std::memory_order_relaxed);
    const float flangeRate         = p_flangeRate->load(std::memory_order_relaxed);
    const float flangeDepth        = p_flangeDepth->load(std::memory_order_relaxed);
    const float flangeResonance    = p_flangeResonance->load(std::memory_order_relaxed);
    const bool  granFreeze         = p_granFreeze->load(std::memory_order_relaxed) >= 0.5f;
    const float granPitch          = p_granPitch->load(std::memory_order_relaxed);
    const float granMix            = p_granMix->load(std::memory_order_relaxed);

    // Stage 1 — Digital Delay → mono output into monoWork_
    digitalDelay_.process(monoIn, monoWork_.data(), numSamples,
                          delayTime, delayFdbk * 0.01f);

    // Stage 2 — Guitar Synth (monophonic tracker on polyphonic Stage 1 output)
    guitarSynth_.setTrackingError(synthTrackingError);
    for (int i = 0; i < numSamples; ++i)
        monoWork_[i] = guitarSynth_.process(monoWork_[i], synthWave, sr_);

    // Stage 3 — Jet Flanger (mono→stereo split)
    flanger_.processStereo(monoWork_.data(), L, R, numSamples,
                           flangeRate, flangeDepth, flangeResonance, sr_);

    // Stage 4 — Granular Freeze
    granFreeze_.processStereo(L, R, numSamples, granFreeze, granPitch, granMix * 0.01f);
}

inline void OvershootChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "ovsh_";

    registerFloatSkewed(layout, p + "delayTime",          "Delay Time",     10.0f,  2000.0f, 375.0f, 1.0f,  0.4f);
    registerFloat      (layout, p + "delayFdbk",          "Delay Fdbk",      0.0f,   90.0f,  40.0f,  0.1f);
    registerFloat      (layout, p + "synthTrackingError", "Synth Track Err", 0.0f,    1.0f,   0.7f,  0.001f);
    registerFloat      (layout, p + "synthWave",          "Synth Wave",      0.0f,    1.0f,   0.0f,  0.001f);
    registerFloatSkewed(layout, p + "flangeRate",         "Flange Rate",     0.05f,   8.0f,   0.5f,  0.001f, 0.4f);
    registerFloat      (layout, p + "flangeDepth",        "Flange Depth",    0.1f,    5.0f,   3.0f,  0.01f);
    registerFloat      (layout, p + "flangeResonance",    "Flange Res",      0.0f,    0.97f,  0.85f, 0.001f);
    registerBool       (layout, p + "granFreeze",         "Gran Freeze",     false);
    registerFloat      (layout, p + "granPitch",          "Gran Pitch",      0.25f,   4.0f,   1.0f,  0.01f);
    registerFloat      (layout, p + "granMix",            "Gran Mix",        0.0f,  100.0f,  40.0f,  0.1f);
}

inline void OvershootChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "ovsh_";
    p_delayTime          = cacheParam(apvts, p + "delayTime");
    p_delayFdbk          = cacheParam(apvts, p + "delayFdbk");
    p_synthTrackingError = cacheParam(apvts, p + "synthTrackingError");
    p_synthWave          = cacheParam(apvts, p + "synthWave");
    p_flangeRate         = cacheParam(apvts, p + "flangeRate");
    p_flangeDepth        = cacheParam(apvts, p + "flangeDepth");
    p_flangeResonance    = cacheParam(apvts, p + "flangeResonance");
    p_granFreeze         = cacheParam(apvts, p + "granFreeze");
    p_granPitch          = cacheParam(apvts, p + "granPitch");
    p_granMix            = cacheParam(apvts, p + "granMix");
}

} // namespace xoceanus
