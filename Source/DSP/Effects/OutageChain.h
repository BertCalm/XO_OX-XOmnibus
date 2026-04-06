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
// OutageChain — OUTAGE Lo-Fi Cinema FX Chain (5 stages)
//
// Source concept: Lo-Fi Cinema (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 4 Vintage Stereo Chorus)
// Accent: Blown Filament Amber #CC8800
//
// Stage 1: Telephone/Radio Filter (Ibanez LF7)
//          2x cascaded CytomicSVF HPF + LPF, Saturator Tube for lo-fi character
// Stage 2: K-Field Modulator / LPG (Fairfield Shallow Water)
//          FractionalDelay + StandardLFO S&H→lowpass drift, EnvelopeFollower
//          drives CytomicSVF LP (Vactrol LPG sim)
// Stage 3: Multi-Head Buffer (Montreal Assembly Count to 5)
//          CircularBuffer with 3 FractionalDelay read heads at 0.5x, -1.0x, 2.0x
// Stage 4: Vintage Stereo Chorus (Arion SCH-1)
//          Dual FractionalDelay, StandardLFO at 180° phase offset. Mono → Stereo.
// Stage 5: Spectral Reverb (Pladask Draume)
//          LushReverb + StandardLFO AM (20–100 Hz), post-reverb sample-rate reduction
//
// Parameter prefix: outg_ (17 params)
//==============================================================================
class OutageChain
{
public:
    OutageChain() = default;

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
    // Stage 1 — Telephone/Radio Filter (Ibanez LF7)
    //==========================================================================
    struct TelephoneFilterStage
    {
        CytomicSVF hpf1, hpf2;   // cascaded HPF
        CytomicSVF lpf1, lpf2;   // cascaded LPF
        Saturator  tubeSat;

        void prepare(double /*sampleRate*/)
        {
            hpf1.reset(); hpf2.reset();
            lpf1.reset(); lpf2.reset();
            tubeSat.setMode(Saturator::SaturationMode::Tube);
            tubeSat.setDrive(0.3f);
            tubeSat.setMix(0.5f);
            tubeSat.setOutputGain(0.9f);
        }
        void reset()
        {
            hpf1.reset(); hpf2.reset();
            lpf1.reset(); lpf2.reset();
        }
        float process(float in, float lowCutHz, float highCutHz, float driveAmt, float srF)
        {
            // HPF cascade
            hpf1.setMode(CytomicSVF::Mode::HighPass);
            hpf1.setCoefficients(lowCutHz, 0.6f, srF);
            hpf2.setMode(CytomicSVF::Mode::HighPass);
            hpf2.setCoefficients(lowCutHz, 0.6f, srF);

            // LPF cascade
            lpf1.setMode(CytomicSVF::Mode::LowPass);
            lpf1.setCoefficients(highCutHz, 0.6f, srF);
            lpf2.setMode(CytomicSVF::Mode::LowPass);
            lpf2.setCoefficients(highCutHz, 0.6f, srF);

            float x = hpf1.processSample(in);
            x = hpf2.processSample(x);
            x = lpf1.processSample(x);
            x = lpf2.processSample(x);

            // Tube saturation for lo-fi character
            tubeSat.setDrive(driveAmt);
            x = tubeSat.processSample(x);
            return x;
        }
    } teleFilter_;

    //==========================================================================
    // Stage 2 — K-Field Modulator / LPG (Fairfield Shallow Water)
    //==========================================================================
    struct KFieldStage
    {
        FractionalDelay modDelay;
        StandardLFO     sandHLfo;
        CytomicSVF      smoothLpf;   // smooths the S&H output (Perlin-like drift)
        EnvelopeFollower envFollow;
        CytomicSVF       lpg;         // Vactrol LPG filter
        float            smoothedMod = 0.0f;
        double           sr          = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxDelay = static_cast<int>(sampleRate * 0.04) + 8; // 40ms max
            modDelay.prepare(maxDelay);
            sandHLfo.setShape(StandardLFO::SandH);
            sandHLfo.setRate(2.0f, static_cast<float>(sampleRate));
            smoothLpf.reset();
            envFollow.prepare(sampleRate);
            envFollow.setAttack(5.0f);
            envFollow.setRelease(80.0f);
            lpg.reset();
            smoothedMod = 0.0f;
        }
        void reset()
        {
            modDelay.clear();
            envFollow.reset();
            lpg.reset();
            smoothedMod = 0.0f;
        }
        float process(float in, float rate, float depth, float lpgSens, float srF)
        {
            // S&H LFO → low-pass smoothed for Perlin-like drift
            sandHLfo.setRate(rate, srF);
            float sandH = sandHLfo.process();
            smoothLpf.setMode(CytomicSVF::Mode::LowPass);
            smoothLpf.setCoefficients(rate * 4.0f + 0.5f, 0.3f, srF);
            smoothedMod = flushDenormal(smoothLpf.processSample(sandH));

            // Fractional delay modulation (wobble effect)
            float modSamples = (1.0f + smoothedMod * depth) * static_cast<float>(sr) * 0.003f + 1.0f;
            modDelay.write(in);
            float modOut = modDelay.read(modSamples);

            // Envelope follower → LPG cutoff (Vactrol sim)
            float envLevel = envFollow.process(in);
            float lpgCutoff = 200.0f + envLevel * lpgSens * 8000.0f;
            lpgCutoff = std::max(80.0f, std::min(lpgCutoff, srF * 0.45f));
            lpg.setMode(CytomicSVF::Mode::LowPass);
            lpg.setCoefficients(lpgCutoff, 0.5f, srF);
            return lpg.processSample(modOut);
        }
    } kField_;

    //==========================================================================
    // Stage 3 — Multi-Head Buffer (Montreal Assembly Count to 5)
    //==========================================================================
    struct MultiHeadStage
    {
        CircularBuffer  circBuf;
        double          sr        = 44100.0;
        // 3 read-head speed multipliers: 0.5x, -1.0x (backward), 2.0x
        float           head1Phase = 0.0f; // forward 0.5x
        float           head3Phase = 0.0f; // forward 2.0x
        float           head2Pos   = 0.0f; // backward 1.0x

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int bufSize = static_cast<int>(sampleRate * 0.5) + 4; // 500ms
            circBuf.prepare(bufSize);
            head1Phase = 0.0f;
            head2Pos   = 0.0f;
            head3Phase = 0.0f;
        }
        void reset()
        {
            circBuf.clear();
            head1Phase = head2Pos = head3Phase = 0.0f;
        }

        float process(float in, int dir1Choice, int dir2Choice, int dir3Choice, float mix)
        {
            circBuf.write(in);
            int bufSize = circBuf.getSize();
            if (bufSize <= 0) return in;

            // Head 1: forward 0.5x (reads half speed)
            head1Phase += 0.5f;
            if (head1Phase >= static_cast<float>(bufSize)) head1Phase -= static_cast<float>(bufSize);
            float h1 = circBuf.readForward(static_cast<int>(head1Phase));

            // Head 2: backward 1.0x
            head2Pos += 1.0f;
            if (head2Pos >= static_cast<float>(bufSize)) head2Pos -= static_cast<float>(bufSize);
            float h2 = circBuf.readBackward(static_cast<int>(head2Pos));

            // Head 3: forward 2.0x
            head3Phase += 2.0f;
            if (head3Phase >= static_cast<float>(bufSize)) head3Phase -= static_cast<float>(bufSize);
            float h3 = circBuf.readForward(static_cast<int>(head3Phase));

            // dir choice: 0=forward, 1=backward, 2=double
            auto pickHead = [&](int choice) -> float {
                switch (choice) {
                    case 0:  return h1;
                    case 1:  return h2;
                    default: return h3;
                }
            };

            float blend = (pickHead(dir1Choice) + pickHead(dir2Choice) + pickHead(dir3Choice)) * (1.0f / 3.0f);
            return in + mix * (blend - in);
        }
    } multiHead_;

    //==========================================================================
    // Stage 4 — Vintage Stereo Chorus (Arion SCH-1)
    // Mono → Stereo expansion here
    //==========================================================================
    struct ChorusStage
    {
        FractionalDelay delayL, delayR;
        StandardLFO     lfoL, lfoR;
        CytomicSVF      highShelfL, highShelfR;

        void prepare(double sampleRate)
        {
            int maxDelay = static_cast<int>(sampleRate * 0.03) + 8; // 30ms
            delayL.prepare(maxDelay);
            delayR.prepare(maxDelay);
            lfoL.setShape(StandardLFO::Sine);
            lfoR.setShape(StandardLFO::Sine);
            lfoR.setPhaseOffset(0.5f); // 180° phase for SCH-1 stereo spread
            highShelfL.reset();
            highShelfR.reset();
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
        }
        void process(float in, float& outL, float& outR,
                     float rate, float depth, float tone, float srF)
        {
            lfoL.setRate(rate, srF);
            lfoR.setRate(rate, srF);

            float modL = lfoL.process();
            float modR = lfoR.process();

            float baseDelaySamples = srF * 0.007f; // 7ms center
            float depthSamples     = srF * 0.006f * depth;

            float dL = baseDelaySamples + modL * depthSamples;
            float dR = baseDelaySamples + modR * depthSamples;
            dL = std::max(1.0f, dL);
            dR = std::max(1.0f, dR);

            delayL.write(in);
            delayR.write(in);
            float wetL = delayL.read(dL);
            float wetR = delayR.read(dR);

            // High-shelf boost in wet path for SCH-1 brightness
            float shelfGainDb = tone * 6.0f;
            highShelfL.setMode(CytomicSVF::Mode::HighShelf);
            highShelfL.setCoefficients(3000.0f, 0.707f, srF, shelfGainDb);
            highShelfR.setMode(CytomicSVF::Mode::HighShelf);
            highShelfR.setCoefficients(3000.0f, 0.707f, srF, shelfGainDb);
            wetL = highShelfL.processSample(wetL);
            wetR = highShelfR.processSample(wetR);

            outL = (in + wetL) * 0.5f;
            outR = (in + wetR) * 0.5f;
        }
    } chorus_;

    //==========================================================================
    // Stage 5 — Spectral Reverb (Pladask Draume)
    //==========================================================================
    struct SpectralReverbStage
    {
        LushReverb    reverb;
        StandardLFO   amLfo;
        float         srHoldCounter = 0.0f;
        float         srHoldL       = 0.0f;
        float         srHoldR       = 0.0f;
        float         srReduceFactor = 1.0f;
        std::vector<float> inL_, inR_, rvL_, rvR_;

        void prepare(double sampleRate, int maxBlockSize)
        {
            reverb.prepare(sampleRate);
            amLfo.setShape(StandardLFO::Sine);
            amLfo.setRate(40.0f, static_cast<float>(sampleRate));
            inL_.resize(maxBlockSize, 0.0f);
            inR_.resize(maxBlockSize, 0.0f);
            rvL_.resize(maxBlockSize, 0.0f);
            rvR_.resize(maxBlockSize, 0.0f);
        }
        void reset()
        {
            reverb.reset();
            srHoldCounter = 0.0f;
            srHoldL = srHoldR = 0.0f;
            std::fill(inL_.begin(), inL_.end(), 0.0f);
            std::fill(inR_.begin(), inR_.end(), 0.0f);
            std::fill(rvL_.begin(), rvL_.end(), 0.0f);
            std::fill(rvR_.begin(), rvR_.end(), 0.0f);
        }
        void processBlock(float* L, float* R, int numSamples,
                          float decay, float amRate, float mix, float srF)
        {
            // Configure reverb
            reverb.setDecay(decay);
            reverb.setSize(0.7f);
            reverb.setDamping(0.3f);
            reverb.setDiffusion(0.6f);
            reverb.setMix(1.0f); // full wet, we mix externally

            // Copy for reverb input (using pre-allocated member buffers)
            float* inL = inL_.data();
            float* inR = inR_.data();
            float* rvL = rvL_.data();
            float* rvR = rvR_.data();
            std::copy(L, L + numSamples, inL);
            std::copy(R, R + numSamples, inR);
            std::fill(rvL, rvL + numSamples, 0.0f);
            std::fill(rvR, rvR + numSamples, 0.0f);

            reverb.processBlock(inL, inR, rvL, rvR, numSamples);

            // AM modulation + sample-rate reduction on reverb output
            amLfo.setRate(amRate, srF);
            // srReduceFactor from mix: more mix = slightly more reduction
            float srStep = 1.0f + mix * 3.0f; // hold up to 4 samples

            for (int i = 0; i < numSamples; ++i)
            {
                // AM
                float amMod = 1.0f + amLfo.process() * 0.08f;
                rvL[i] *= amMod;
                rvR[i] *= amMod;

                // Zero-order hold (sample rate reduction)
                srHoldCounter += 1.0f;
                if (srHoldCounter >= srStep)
                {
                    srHoldCounter = 0.0f;
                    srHoldL = rvL[i];
                    srHoldR = rvR[i];
                }
                rvL[i] = srHoldL;
                rvR[i] = srHoldR;
            }

            // Wet/dry blend
            for (int i = 0; i < numSamples; ++i)
            {
                L[i] = L[i] + mix * (rvL[i] - L[i]);
                R[i] = R[i] + mix * (rvR[i] - R[i]);
            }
        }
    } spectralReverb_;

    //==========================================================================
    // Cached parameter pointers (17 params)
    //==========================================================================
    std::atomic<float>* p_lowCut       = nullptr;
    std::atomic<float>* p_highCut      = nullptr;
    std::atomic<float>* p_drive        = nullptr;
    std::atomic<float>* p_kFieldRate   = nullptr;
    std::atomic<float>* p_kFieldDepth  = nullptr;
    std::atomic<float>* p_lpgSens      = nullptr;
    std::atomic<float>* p_ct5Len       = nullptr;
    std::atomic<float>* p_ct5Dir1      = nullptr;
    std::atomic<float>* p_ct5Dir2      = nullptr;
    std::atomic<float>* p_ct5Dir3      = nullptr;
    std::atomic<float>* p_ct5Mix       = nullptr;
    std::atomic<float>* p_schRate      = nullptr;
    std::atomic<float>* p_schDepth     = nullptr;
    std::atomic<float>* p_schTone      = nullptr;
    std::atomic<float>* p_draumeDecay  = nullptr;
    std::atomic<float>* p_draumeAmRate = nullptr;
    std::atomic<float>* p_draumeMix    = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OutageChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    teleFilter_.prepare(sampleRate);
    kField_.prepare(sampleRate);
    multiHead_.prepare(sampleRate);
    chorus_.prepare(sampleRate);
    spectralReverb_.prepare(sampleRate, maxBlockSize);
}

inline void OutageChain::reset()
{
    teleFilter_.reset();
    kField_.reset();
    multiHead_.reset();
    chorus_.reset();
    spectralReverb_.reset();
}

inline void OutageChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_lowCut) return;

    // ParamSnapshot: read all params once per block
    const float lowCut       = p_lowCut->load(std::memory_order_relaxed);
    const float highCut      = p_highCut->load(std::memory_order_relaxed);
    const float drive        = p_drive->load(std::memory_order_relaxed);
    const float kFieldRate   = p_kFieldRate->load(std::memory_order_relaxed);
    const float kFieldDepth  = p_kFieldDepth->load(std::memory_order_relaxed);
    const float lpgSens      = p_lpgSens->load(std::memory_order_relaxed);
    const float ct5Mix       = p_ct5Mix->load(std::memory_order_relaxed) * 0.01f;
    const int   ct5Dir1      = static_cast<int>(p_ct5Dir1->load(std::memory_order_relaxed));
    const int   ct5Dir2      = static_cast<int>(p_ct5Dir2->load(std::memory_order_relaxed));
    const int   ct5Dir3      = static_cast<int>(p_ct5Dir3->load(std::memory_order_relaxed));
    const float schRate      = p_schRate->load(std::memory_order_relaxed);
    const float schDepth     = p_schDepth->load(std::memory_order_relaxed);
    const float schTone      = p_schTone->load(std::memory_order_relaxed);
    const float draumeDecay  = p_draumeDecay->load(std::memory_order_relaxed);
    const float draumeAmRate = p_draumeAmRate->load(std::memory_order_relaxed);
    const float draumeMix    = p_draumeMix->load(std::memory_order_relaxed) * 0.01f;

    const float srF = static_cast<float>(sr_);
    const float driveNorm = drive * 0.01f;

    // Mono pipeline: stages 1-3, write to L as temp buffer
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: Telephone/Radio Filter
        x = teleFilter_.process(x, lowCut, highCut, driveNorm, srF);

        // Stage 2: K-Field Modulator / LPG
        x = kField_.process(x, kFieldRate, kFieldDepth, lpgSens, srF);

        // Stage 3: Multi-Head Buffer
        x = multiHead_.process(x, ct5Dir1, ct5Dir2, ct5Dir3, ct5Mix);

        L[i] = x;
    }

    // Stage 4: Vintage Stereo Chorus — Mono → Stereo
    for (int i = 0; i < numSamples; ++i)
    {
        float outL, outR;
        chorus_.process(L[i], outL, outR, schRate, schDepth, schTone, srF);
        L[i] = outL;
        R[i] = outR;
    }

    // Stage 5: Spectral Reverb (operates on stereo)
    spectralReverb_.processBlock(L, R, numSamples, draumeDecay, draumeAmRate, draumeMix, srF);
}

inline void OutageChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "outg_";

    registerFloatSkewed(layout, p + "lowCut",       p + "LF7 Low Cut",
                        20.0f,   2000.0f, 300.0f,  1.0f, 0.3f);
    registerFloatSkewed(layout, p + "highCut",      p + "LF7 High Cut",
                        800.0f, 18000.0f, 3400.0f, 1.0f, 0.3f);
    registerFloat      (layout, p + "drive",        p + "LF7 Drive",
                        0.0f,   100.0f,  20.0f);
    registerFloatSkewed(layout, p + "kFieldRate",   p + "K-Field Rate",
                        0.05f,   10.0f,  0.8f,  0.001f, 0.3f);
    registerFloat      (layout, p + "kFieldDepth",  p + "K-Field Depth",
                        0.0f,    1.0f,   0.4f);
    registerFloat      (layout, p + "lpgSens",      p + "LPG Sens",
                        0.0f,    1.0f,   0.5f);
    registerFloat      (layout, p + "ct5Len",       p + "CT5 Length",
                        0.0f,  100.0f,  50.0f);
    registerChoice     (layout, p + "ct5Dir1",      p + "CT5 Head 1",
                        {"Forward 0.5x", "Backward 1x", "Forward 2x"}, 0);
    registerChoice     (layout, p + "ct5Dir2",      p + "CT5 Head 2",
                        {"Forward 0.5x", "Backward 1x", "Forward 2x"}, 1);
    registerChoice     (layout, p + "ct5Dir3",      p + "CT5 Head 3",
                        {"Forward 0.5x", "Backward 1x", "Forward 2x"}, 2);
    registerFloat      (layout, p + "ct5Mix",       p + "CT5 Mix",
                        0.0f,  100.0f,  35.0f);
    registerFloatSkewed(layout, p + "schRate",      p + "SCH-1 Rate",
                        0.1f,    5.0f,  0.6f,  0.001f, 0.4f);
    registerFloat      (layout, p + "schDepth",     p + "SCH-1 Depth",
                        0.0f,    1.0f,  0.5f);
    registerFloat      (layout, p + "schTone",      p + "SCH-1 Tone",
                        -1.0f,   1.0f,  0.2f);
    registerFloatSkewed(layout, p + "draumeDecay",  p + "Draume Decay",
                        0.5f,   20.0f,  4.0f,  0.01f, 0.4f);
    registerFloatSkewed(layout, p + "draumeAmRate", p + "Draume AM Rate",
                        20.0f, 100.0f, 45.0f,  0.1f, 0.5f);
    registerFloat      (layout, p + "draumeMix",    p + "Draume Mix",
                        0.0f,  100.0f, 30.0f);
}

inline void OutageChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "outg_";
    p_lowCut       = cacheParam(apvts, p + "lowCut");
    p_highCut      = cacheParam(apvts, p + "highCut");
    p_drive        = cacheParam(apvts, p + "drive");
    p_kFieldRate   = cacheParam(apvts, p + "kFieldRate");
    p_kFieldDepth  = cacheParam(apvts, p + "kFieldDepth");
    p_lpgSens      = cacheParam(apvts, p + "lpgSens");
    p_ct5Len       = cacheParam(apvts, p + "ct5Len");
    p_ct5Dir1      = cacheParam(apvts, p + "ct5Dir1");
    p_ct5Dir2      = cacheParam(apvts, p + "ct5Dir2");
    p_ct5Dir3      = cacheParam(apvts, p + "ct5Dir3");
    p_ct5Mix       = cacheParam(apvts, p + "ct5Mix");
    p_schRate      = cacheParam(apvts, p + "schRate");
    p_schDepth     = cacheParam(apvts, p + "schDepth");
    p_schTone      = cacheParam(apvts, p + "schTone");
    p_draumeDecay  = cacheParam(apvts, p + "draumeDecay");
    p_draumeAmRate = cacheParam(apvts, p + "draumeAmRate");
    p_draumeMix    = cacheParam(apvts, p + "draumeMix");
}

} // namespace xoceanus
