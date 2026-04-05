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
#include <cstdlib>

namespace xoceanus
{

//==============================================================================
// OublietteChain — OUBLIETTE Memory Slicer FX Chain (4 stages)
//
// Concept: Chase Bliss Habit × Montreal Assembly PurPLL × Boss SL-20 × Yamaha SPX90
// Routing: Mono In → Stereo Out (expansion at Stage 4 SPX Widener)
//
// Stage 1: Echo Collector (Chase Bliss Habit) — 3-minute circular buffer,
//          random scanner playhead with Hermite crossfade
// Stage 2: PLL Synth (Montreal Assembly PurPLL) — envelope-tracked PolyBLEP
//          square + sub-octaves + XOR ring-mod
// Stage 3: Rhythmic Audio Slicer (Boss SL-20) — 16-step VCA gate sequencer
//          synced to host BPM
// Stage 4: Symphonic Stereo Widener (Yamaha SPX90) — dual FractionalDelay
//          modulated by staggered LFOs + 12-bit colour + stereo expansion
//
// Parameter prefix: oubl_ (12 params)
//==============================================================================
class OublietteChain
{
public:
    OublietteChain() = default;

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
    // Stage 1 — Echo Collector (Chase Bliss Habit)
    // 3-minute circular buffer + random scanner playhead with Hermite crossfade
    //==========================================================================
    struct EchoCollectorStage
    {
        // 3-minute max @ 96kHz = 17,280,000 samples — pre-allocated in prepare()
        CircularBuffer history;
        int   bufferSize      = 0;

        // Scanner playhead state
        float scanPhase       = 0.0f;   // 0..1 normalised position into history
        float scanTarget      = 0.2f;   // current random jump target
        float scanCurrent     = 0.2f;   // smoothed position
        float scanSmooth      = 0.0f;   // one-pole IIR coefficient
        float xfadeGain       = 1.0f;   // crossfade window for playhead jumps
        float xfadeDir        = 0.0f;   // +1 fading in, -1 fading out
        int   xfadeCounter    = 0;
        static constexpr int kXfadeLength = 512;

        // LCG for reproducible-ish random scanner positions
        uint32_t lcgState = 12345678u;

        float nextRandom() noexcept
        {
            lcgState = lcgState * 1664525u + 1013904223u;
            return static_cast<float>(lcgState & 0x00FFFFFFu) / static_cast<float>(0x01000000u);
        }

        void prepare(double sampleRate)
        {
            int maxSamples = static_cast<int>(sampleRate * 180.0); // 3 minutes
            history.prepare(maxSamples);
            bufferSize = maxSamples;
            scanSmooth = fastExp(-1.0f / (static_cast<float>(sampleRate) * 0.05f)); // 50ms smooth
            reset();
        }

        void reset()
        {
            history.clear();
            scanPhase   = 0.0f;
            scanCurrent = 0.2f;
            scanTarget  = 0.2f;
            xfadeGain   = 1.0f;
            xfadeDir    = 0.0f;
            xfadeCounter = 0;
        }

        // Process one sample. Returns blended scanner output.
        float process(float in, float scanRate, float scanSpread)
        {
            history.write(in);

            // Smooth toward target position
            scanCurrent = flushDenormal(scanCurrent * scanSmooth
                          + scanTarget * (1.0f - scanSmooth));

            // Advance scan phase at scanRate Hz (relative speed through history)
            scanPhase += scanRate / static_cast<float>(bufferSize);
            if (scanPhase >= 1.0f)
            {
                scanPhase -= 1.0f;
                // Pick a new random target on wrap — constrained by spread
                float center = scanCurrent;
                float half   = scanSpread * 0.5f;
                float rnd    = nextRandom();
                scanTarget   = center + (rnd - 0.5f) * 2.0f * half;
                scanTarget   = std::max(0.0f, std::min(1.0f, scanTarget));

                // Initiate crossfade on jump
                xfadeCounter = kXfadeLength;
                xfadeDir     = 1.0f;
            }

            // Crossfade windowing
            if (xfadeCounter > 0)
            {
                float t = static_cast<float>(xfadeCounter) / static_cast<float>(kXfadeLength);
                xfadeGain = flushDenormal(t); // ramp 1→0 over xfadeLength
                --xfadeCounter;
            }
            else
            {
                xfadeGain = 1.0f;
            }

            // Read from history at scanner position
            int readOffset = static_cast<int>(scanCurrent * static_cast<float>(bufferSize - 1));
            readOffset = std::max(0, std::min(readOffset, bufferSize - 1));
            float scanned = history.readForward(readOffset);

            return scanned * xfadeGain;
        }
    } echo_;

    //==========================================================================
    // Stage 2 — PLL Synth (Montreal Assembly PurPLL)
    // EnvelopeFollower pitch-track → PolyBLEP square + sub-octaves + XOR ring-mod
    //==========================================================================
    struct PLLSynthStage
    {
        EnvelopeFollower envFollow;
        PolyBLEP        pllOsc;
        float           pllFreq      = 440.0f;
        float           pllFreqSmooth = 0.0f;
        float           pllPhaseAcc  = 0.0f;  // sub-octave phase (manual)
        float           subPhase     = 0.0f;
        double          sr           = 44100.0;

        // Zero-crossing detector for pitch tracking
        float lastSample     = 0.0f;
        int   zeroCrossCount = 0;
        int   periodSamples  = 100;
        int   samplesSinceCross = 0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            envFollow.prepare(sampleRate);
            envFollow.setAttack(1.0f);
            envFollow.setRelease(20.0f);
            pllOsc.setWaveform(PolyBLEP::Waveform::Square);
            pllFreq       = 220.0f;
            pllFreqSmooth = 220.0f;
            pllPhaseAcc   = 0.0f;
            subPhase      = 0.0f;
            lastSample    = 0.0f;
            zeroCrossCount = 0;
            samplesSinceCross = 0;
        }

        void reset()
        {
            envFollow.reset();
            pllPhaseAcc = 0.0f;
            subPhase    = 0.0f;
            lastSample  = 0.0f;
            samplesSinceCross = 0;
        }

        float process(float in, float trackSpeed, float subAmount, float pllMult, float dryAmt)
        {
            float srF = static_cast<float>(sr);
            envFollow.process(in);

            // Zero-crossing pitch detection
            ++samplesSinceCross;
            if ((in >= 0.0f) != (lastSample >= 0.0f))
            {
                if (samplesSinceCross > 4 && samplesSinceCross < static_cast<int>(srF / 20.0f))
                    periodSamples = samplesSinceCross * 2; // half-period → full period
                samplesSinceCross = 0;
            }
            lastSample = in;

            // Derive tracked frequency from period
            float detectedFreq = srF / static_cast<float>(std::max(1, periodSamples));
            detectedFreq = std::max(20.0f, std::min(detectedFreq, srF * 0.45f));
            detectedFreq *= pllMult;
            detectedFreq = std::max(20.0f, std::min(detectedFreq, srF * 0.45f));

            // Smooth PLL frequency (trackSpeed controls time constant)
            float smoothCoeff = fastExp(-1.0f / (trackSpeed * 0.001f * srF));
            pllFreqSmooth = flushDenormal(pllFreqSmooth * smoothCoeff
                            + detectedFreq * (1.0f - smoothCoeff));

            pllOsc.setFrequency(pllFreqSmooth, srF);
            float pllOut = pllOsc.processSample();

            // Sub-octave: simple period-divide via separate phase
            subPhase = flushDenormal(subPhase + pllFreqSmooth / (srF * 2.0f));
            if (subPhase >= 1.0f) subPhase -= 1.0f;
            float subOut = (subPhase < 0.5f) ? 1.0f : -1.0f;

            // Mix sub into PLL output
            float combined = pllOut + subOut * subAmount;

            // XOR ring-mod: multiply dry and PLL sign bits
            // Integer XOR-style: compare signs to create bipolar ±1 ring product
            int drySign = (in >= 0.0f)       ? 1 : -1;
            int pllSign = (combined >= 0.0f) ? 1 : -1;
            float xorOut = static_cast<float>(drySign * pllSign) * std::abs(combined);

            return in * dryAmt + xorOut * (1.0f - dryAmt);
        }
    } pll_;

    //==========================================================================
    // Stage 3 — Rhythmic Audio Slicer (Boss SL-20)
    // 16-step VCA gate sequencer synced to host BPM
    //==========================================================================
    struct SlicerStage
    {
        static constexpr int kMaxSteps = 16;

        // 20 hardcoded gate patterns (each is a 16-bit bitmask)
        static constexpr uint16_t kPatterns[20] = {
            0xFFFF, // 0: all open
            0xAAAA, // 1: every other (8th notes)
            0xCCCC, // 2: pairs
            0x8888, // 3: quarter notes
            0x8080, // 4: half notes
            0xF0F0, // 5: bar halves
            0xFFF0, // 6: 3/4 open
            0x0FFF, // 7: last 3/4
            0xF888, // 8: accent pattern
            0xAAAA, // 9: syncopated
            0x9999, // 10: dotted
            0xEEEE, // 11: triplet-ish
            0xE4E4, // 12: drum-machine
            0xD5D5, // 13: busy
            0x8484, // 14: sparse
            0xA0A0, // 15: sparser
            0x9090, // 16: offbeats
            0xB5B5, // 17: complex
            0xFF00, // 18: first half
            0x00FF, // 19: second half
        };

        float gateGain    = 1.0f;
        float attackCoeff = 0.0f;
        int   currentStep = 0;
        float stepAccum   = 0.0f;  // fractional step accumulator
        double sr         = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            attackCoeff = fastExp(-1.0f / (2.0f * static_cast<float>(sampleRate) * 0.001f)); // 2ms attack
            reset();
        }

        void reset()
        {
            gateGain   = 1.0f;
            currentStep = 0;
            stepAccum  = 0.0f;
        }

        float process(float in, int patternIndex, float attackMs, float dutyFrac,
                      double bpm, int numSamplesElapsed)
        {
            // Compute step duration in samples (16th notes at bpm)
            float srF        = static_cast<float>(sr);
            float beatsPerSec = static_cast<float>(bpm) / 60.0f;
            float stepSamples = srF / (beatsPerSec * 4.0f); // 16th note = beat/4
            stepSamples = std::max(stepSamples, 64.0f);

            // Advance step accumulator
            stepAccum += 1.0f;
            if (stepAccum >= stepSamples)
            {
                stepAccum -= stepSamples;
                currentStep = (currentStep + 1) % kMaxSteps;
            }

            // Check gate state for current step
            int   patIdx   = std::max(0, std::min(patternIndex - 1, 19));
            bool  gateOpen = ((kPatterns[patIdx] >> currentStep) & 1u) != 0;

            // Duty cycle: gate closes after duty fraction of step
            float stepPos = stepAccum / stepSamples;
            if (stepPos > dutyFrac) gateOpen = false;

            // Attack coefficient per attackMs parameter
            float attCoeff = fastExp(-1.0f / (attackMs * 0.001f * srF));

            // Smooth gate with attack
            float targetGain = gateOpen ? 1.0f : 0.0f;
            gateGain = flushDenormal(gateGain * attCoeff + targetGain * (1.0f - attCoeff));

            return in * gateGain;
        }
    } slicer_;

    //==========================================================================
    // Stage 4 — Symphonic Stereo Widener (Yamaha SPX90)
    // Dual FractionalDelay (1–10ms) modulated by staggered StandardLFOs
    // + 12-bit quantization + Mono→Stereo split
    //==========================================================================
    struct SPXWidenerStage
    {
        FractionalDelay delayL;
        FractionalDelay delayR;
        StandardLFO     lfoL;
        StandardLFO     lfoR;
        double          sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxDelaySamples = static_cast<int>(sampleRate * 0.015); // 15ms max
            delayL.prepare(maxDelaySamples);
            delayR.prepare(maxDelaySamples);

            lfoL.setShape(StandardLFO::Sine);
            lfoR.setShape(StandardLFO::Sine);
            lfoL.setPhaseOffset(0.0f);
            lfoR.setPhaseOffset(0.25f); // 90-degree offset for width
        }

        void reset()
        {
            delayL.clear();
            delayR.clear();
        }

        void process(float in, float& outL, float& outR,
                     float widthMs, float colorAmt, double sr_)
        {
            float srF  = static_cast<float>(sr_);
            float lfoL_out = lfoL.process();
            float lfoR_out = lfoR.process();

            // Delay times: 1ms center ± (widthMs/2) modulation
            float baseDelayL = (1.0f + widthMs * 0.5f * (1.0f + lfoL_out)) * srF * 0.001f;
            float baseDelayR = (1.0f + widthMs * 0.5f * (1.0f + lfoR_out)) * srF * 0.001f;
            baseDelayL = std::max(1.0f, std::min(baseDelayL, static_cast<float>(delayL.getSize()) - 2.0f));
            baseDelayR = std::max(1.0f, std::min(baseDelayR, static_cast<float>(delayR.getSize()) - 2.0f));

            // 12-bit quantization colour effect
            float quantized = in;
            if (colorAmt > 0.01f)
            {
                float scale    = 4096.0f;
                float quantRaw = std::floor(in * scale) / scale;
                quantized      = in + colorAmt * (quantRaw - in);
            }

            delayL.write(quantized);
            delayR.write(quantized);

            outL = delayL.read(baseDelayL);
            outR = delayR.read(baseDelayR);
        }
    } widener_;

    //==========================================================================
    // Cached parameter pointers (12 params)
    //==========================================================================
    std::atomic<float>* p_scanRate     = nullptr;
    std::atomic<float>* p_scanSpread   = nullptr;
    std::atomic<float>* p_scanMix      = nullptr;
    std::atomic<float>* p_pllTrack     = nullptr;
    std::atomic<float>* p_pllSub       = nullptr;
    std::atomic<float>* p_pllMult      = nullptr;
    std::atomic<float>* p_pllDry       = nullptr;
    std::atomic<float>* p_slicePattern = nullptr;
    std::atomic<float>* p_sliceAttack  = nullptr;
    std::atomic<float>* p_sliceDuty    = nullptr;
    std::atomic<float>* p_spxWidth     = nullptr;
    std::atomic<float>* p_spxColor     = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OublietteChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    echo_.prepare(sampleRate);
    pll_.prepare(sampleRate);
    slicer_.prepare(sampleRate);
    widener_.prepare(sampleRate);
}

inline void OublietteChain::reset()
{
    echo_.reset();
    pll_.reset();
    slicer_.reset();
    widener_.reset();
}

inline void OublietteChain::processBlock(const float* monoIn, float* L, float* R,
                                          int numSamples, double bpm, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_scanRate) return;

    // ParamSnapshot: read all params once
    const float scanRate     = p_scanRate->load(std::memory_order_relaxed);
    const float scanSpread   = p_scanSpread->load(std::memory_order_relaxed);
    const float scanMix      = p_scanMix->load(std::memory_order_relaxed) * 0.01f;
    const float pllTrack     = p_pllTrack->load(std::memory_order_relaxed);
    const float pllSub       = p_pllSub->load(std::memory_order_relaxed);
    const float pllMult      = p_pllMult->load(std::memory_order_relaxed);
    const float pllDry       = p_pllDry->load(std::memory_order_relaxed);
    const int   slicePattern = static_cast<int>(p_slicePattern->load(std::memory_order_relaxed));
    const float sliceAttack  = p_sliceAttack->load(std::memory_order_relaxed);
    const float sliceDuty    = p_sliceDuty->load(std::memory_order_relaxed);
    const float spxWidth     = p_spxWidth->load(std::memory_order_relaxed);
    const float spxColor     = p_spxColor->load(std::memory_order_relaxed);

    // LFO rates for widener — slightly offset for natural chorus
    float srF = static_cast<float>(sr_);
    widener_.lfoL.setRate(0.83f, srF);
    widener_.lfoR.setRate(0.97f, srF);

    if (bpm <= 0.0) bpm = 120.0;

    // Mono pipeline stages 1-3 (write to L as temp mono)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: Echo Collector
        float echoed = echo_.process(x, scanRate, scanSpread);
        x = x + scanMix * (echoed - x);

        // Stage 2: PLL Synth
        x = pll_.process(x, pllTrack, pllSub, pllMult, pllDry);

        // Stage 3: Rhythmic Slicer
        x = slicer_.process(x, slicePattern, sliceAttack, sliceDuty, bpm, i);

        L[i] = x;
    }

    // Stage 4: SPX Widener — Mono → Stereo
    for (int i = 0; i < numSamples; ++i)
    {
        float wL, wR;
        widener_.process(L[i], wL, wR, spxWidth, spxColor, sr_);
        L[i] = wL;
        R[i] = wR;
    }
}

inline void OublietteChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "oubl_";

    registerFloat(layout, p + "scanRate",     p + "Scan Rate",
                  0.01f, 10.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "scanSpread",   p + "Scan Spread",
                  0.0f, 1.0f, 0.4f, 0.001f);
    registerFloat(layout, p + "scanMix",      p + "Scan Mix",
                  0.0f, 100.0f, 40.0f, 0.1f);
    registerFloat(layout, p + "pllTrackSpeed", p + "PLL Track Speed",
                  1.0f, 500.0f, 50.0f, 0.1f);
    registerFloat(layout, p + "pllSub",       p + "PLL Sub",
                  0.0f, 1.0f, 0.3f, 0.001f);
    registerFloat(layout, p + "pllMult",      p + "PLL Mult",
                  0.25f, 4.0f, 1.0f, 0.001f);
    registerFloat(layout, p + "pllDry",       p + "PLL Dry",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "slicePattern", p + "Slice Pattern",
                  1.0f, 20.0f, 1.0f, 1.0f);
    registerFloat(layout, p + "sliceAttack",  p + "Slice Attack",
                  0.1f, 50.0f, 5.0f, 0.1f);
    registerFloat(layout, p + "sliceDuty",    p + "Slice Duty",
                  0.05f, 1.0f, 0.7f, 0.001f);
    registerFloat(layout, p + "spxWidth",     p + "SPX Width",
                  0.0f, 10.0f, 3.0f, 0.01f);
    registerFloat(layout, p + "spxColor",     p + "SPX Color",
                  0.0f, 1.0f, 0.2f, 0.001f);
}

inline void OublietteChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "oubl_";
    p_scanRate     = cacheParam(apvts, p + "scanRate");
    p_scanSpread   = cacheParam(apvts, p + "scanSpread");
    p_scanMix      = cacheParam(apvts, p + "scanMix");
    p_pllTrack     = cacheParam(apvts, p + "pllTrackSpeed");
    p_pllSub       = cacheParam(apvts, p + "pllSub");
    p_pllMult      = cacheParam(apvts, p + "pllMult");
    p_pllDry       = cacheParam(apvts, p + "pllDry");
    p_slicePattern = cacheParam(apvts, p + "slicePattern");
    p_sliceAttack  = cacheParam(apvts, p + "sliceAttack");
    p_sliceDuty    = cacheParam(apvts, p + "sliceDuty");
    p_spxWidth     = cacheParam(apvts, p + "spxWidth");
    p_spxColor     = cacheParam(apvts, p + "spxColor");
}

} // namespace xoceanus
