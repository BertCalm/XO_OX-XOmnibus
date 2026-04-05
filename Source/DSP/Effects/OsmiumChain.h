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

namespace xoceanus
{

//==============================================================================
// OsmiumChain — OSMIUM Sub-Harmonic Collapse FX Chain (4 stages)
//
// Concept: DOD FX32 Meat Box × Korg SDD-3000 × Kinotone Ribbons × Chase Bliss Gen Loss
// Routing: Mono In → Stereo Out (expansion at Stage 4 VHS Degradation)
//
// Stage 1: Sub-Harmonic Synthesizer (DOD FX32 Meat Box) — pitch detect via
//          zero-crossing → two PolyBLEP sines at f/2 and f/4 → CytomicSVF LP
// Stage 2: JRC4558 Preamp (Korg SDD-3000) — CytomicSVF high-shelf boost →
//          Saturator Tube mode → attenuator
// Stage 3: 4-Track Tape Compression (Kinotone Ribbons) — EnvelopeFollower
//          compressor + Saturator Tape mode + dynamic HF rolloff via SVF LP
// Stage 4: VHS Degradation & Flutter (Chase Bliss Gen Loss) — FractionalDelay
//          + S&H LFO wow + 60Hz motor hum + filtered tape hiss → Mono→Stereo
//
// Parameter prefix: osmi_ (12 params)
//==============================================================================
class OsmiumChain
{
public:
    OsmiumChain() = default;

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
    // Stage 1 — Sub-Harmonic Synthesizer (DOD FX32 Meat Box)
    // Zero-crossing pitch detection → PolyBLEP sines at f/2 and f/4
    // 4th-order CytomicSVF LP at 80Hz on the sub output
    //==========================================================================
    struct MeatBoxStage
    {
        PolyBLEP    sub2Osc;     // f/2 sine
        PolyBLEP    sub4Osc;     // f/4 sine
        CytomicSVF  lpFilt1;     // first-order cascade LP (stage 1 of 4th-order)
        CytomicSVF  lpFilt2;     // stage 2
        CytomicSVF  lpFilt3;     // stage 3
        CytomicSVF  lpFilt4;     // stage 4

        float  lastSample       = 0.0f;
        int    samplesSinceCross = 0;
        int    periodSamples    = 100;
        double sr               = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            sub2Osc.setWaveform(PolyBLEP::Waveform::Sine);
            sub4Osc.setWaveform(PolyBLEP::Waveform::Sine);
            lpFilt1.setMode(CytomicSVF::Mode::LowPass);
            lpFilt2.setMode(CytomicSVF::Mode::LowPass);
            lpFilt3.setMode(CytomicSVF::Mode::LowPass);
            lpFilt4.setMode(CytomicSVF::Mode::LowPass);
            // Pre-set 80Hz LP coefficients (will be refreshed each processBlock)
            float srF = static_cast<float>(sampleRate);
            lpFilt1.setCoefficients(80.0f, 0.5f, srF);
            lpFilt2.setCoefficients(80.0f, 0.5f, srF);
            lpFilt3.setCoefficients(80.0f, 0.5f, srF);
            lpFilt4.setCoefficients(80.0f, 0.5f, srF);
            reset();
        }

        void reset()
        {
            lastSample = 0.0f;
            samplesSinceCross = 0;
            periodSamples = 100;
            lpFilt1.reset(); lpFilt2.reset();
            lpFilt3.reset(); lpFilt4.reset();
        }

        // Returns mixed dry + sub signal
        float process(float in, float sub30Hz, float sub60Hz, float cleanAmt)
        {
            float srF = static_cast<float>(sr);

            // Zero-crossing detection for pitch tracking
            ++samplesSinceCross;
            if ((in >= 0.0f) != (lastSample >= 0.0f))
            {
                if (samplesSinceCross > 4 && samplesSinceCross < static_cast<int>(srF / 20.0f))
                    periodSamples = samplesSinceCross * 2;
                samplesSinceCross = 0;
            }
            lastSample = in;

            float detectedHz = srF / static_cast<float>(std::max(1, periodSamples));
            detectedHz = std::max(20.0f, std::min(detectedHz, srF * 0.45f));

            // Sub at f/2 and f/4
            float sub2Freq = std::max(10.0f, detectedHz * 0.5f);
            float sub4Freq = std::max(10.0f, detectedHz * 0.25f);

            sub2Osc.setFrequency(sub2Freq, srF);
            sub4Osc.setFrequency(sub4Freq, srF);

            float sub2 = sub2Osc.processSample() * sub60Hz;
            float sub4 = sub4Osc.processSample() * sub30Hz;

            // 4th-order LP at 80Hz on sub sum (coefficients set in prepare / processBlock)
            float subSum = sub2 + sub4;
            subSum = lpFilt1.processSample(subSum);
            subSum = lpFilt2.processSample(subSum);
            subSum = lpFilt3.processSample(subSum);
            subSum = lpFilt4.processSample(subSum);

            return flushDenormal(in * cleanAmt + subSum);
        }
    } meatBox_;

    //==========================================================================
    // Stage 2 — JRC4558 Preamp (Korg SDD-3000)
    // CytomicSVF high-shelf boost → Saturator Tube mode → attenuator
    //==========================================================================
    struct SDD3000Stage
    {
        CytomicSVF highShelf;
        Saturator  tubeSat;

        void prepare(double sampleRate)
        {
            highShelf.setMode(CytomicSVF::Mode::HighShelf);
            tubeSat.prepare(sampleRate);
            tubeSat.setMode(Saturator::SaturationMode::Tube);
            tubeSat.setMix(1.0f);
            reset();
        }

        void reset()
        {
            highShelf.reset();
            tubeSat.reset();
        }

        float process(float in, float driveAmt, float toneAmt, double sampleRate)
        {
            float srF = static_cast<float>(sampleRate);

            // High-shelf boost: toneAmt controls shelf gain (+12dB max)
            float shelfGainDb = toneAmt * 12.0f;
            highShelf.setCoefficients(3000.0f, 0.707f, srF, shelfGainDb);
            float x = highShelf.processSample(in);

            // Tube saturation
            tubeSat.setDrive(driveAmt);
            x = tubeSat.processSample(x);

            // Attenuate to compensate drive gain
            float attenuate = 1.0f / (1.0f + driveAmt * 3.0f);
            return x * attenuate;
        }
    } preamp_;

    //==========================================================================
    // Stage 3 — 4-Track Tape Compression (Kinotone Ribbons)
    // EnvelopeFollower compressor + Saturator Tape + dynamic HF rolloff
    //==========================================================================
    struct TapeCompStage
    {
        EnvelopeFollower env;
        Saturator        tapeSat;
        CytomicSVF       hfRolloff;
        float            gainReduction = 1.0f;

        void prepare(double sampleRate)
        {
            env.prepare(sampleRate);
            env.setAttack(5.0f);
            env.setRelease(80.0f);
            tapeSat.prepare(sampleRate);
            tapeSat.setMode(Saturator::SaturationMode::Tape);
            tapeSat.setMix(1.0f);
            hfRolloff.setMode(CytomicSVF::Mode::LowPass);
            hfRolloff.setCoefficients(18000.0f, 0.5f, static_cast<float>(sampleRate));
            gainReduction = 1.0f;
        }

        void reset()
        {
            env.reset();
            tapeSat.reset();
            hfRolloff.reset();
            gainReduction = 1.0f;
        }

        float process(float in, float compAmt, float satAmt, float ageAmt, double sampleRate)
        {
            float srF  = static_cast<float>(sampleRate);
            float level = env.process(in);

            // Simple compressor: gain reduction proportional to level above threshold
            float threshold = 0.3f;
            if (level > threshold)
            {
                float over = level - threshold;
                gainReduction = 1.0f / (1.0f + over * compAmt * 4.0f);
            }
            else
            {
                gainReduction = flushDenormal(gainReduction + (1.0f - gainReduction) * 0.001f);
            }

            float x = in * gainReduction;

            // Tape saturation for hysteresis
            tapeSat.setDrive(satAmt);
            x = tapeSat.processSample(x);

            // Dynamic HF rolloff: more rolloff at high levels (tape saturation)
            float rolloffFreq = 18000.0f - level * ageAmt * 14000.0f;
            rolloffFreq = std::max(1000.0f, std::min(rolloffFreq, srF * 0.45f));
            hfRolloff.setCoefficients(rolloffFreq, 0.5f, srF);
            return flushDenormal(hfRolloff.processSample(x));
        }
    } tapeComp_;

    //==========================================================================
    // Stage 4 — VHS Degradation & Flutter (Chase Bliss Gen Loss)
    // FractionalDelay + S&H LFO wow + 60Hz motor hum + tape hiss → Stereo
    //==========================================================================
    struct VHSStage
    {
        FractionalDelay wowDelay;
        StandardLFO     wowLFO;    // S&H shape for wow
        StandardLFO     hissLFO;   // SandH for noise texture
        CytomicSVF      hissFilt;
        CytomicSVF      hissFiltR;
        float           motorPhase  = 0.0f;  // 60Hz motor hum
        float           noiseState  = 0.0f;
        double          sr          = 44100.0;

        // LCG for tape hiss
        uint32_t lcgState = 9876543u;
        float nextNoise() noexcept
        {
            lcgState = lcgState * 1664525u + 1013904223u;
            return static_cast<float>(static_cast<int32_t>(lcgState)) / 2147483648.0f;
        }

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxDelay = static_cast<int>(sampleRate * 0.1); // 100ms max flutter
            wowDelay.prepare(maxDelay);

            wowLFO.setShape(StandardLFO::SandH);
            wowLFO.setRate(3.5f, static_cast<float>(sampleRate));  // S&H chopper for wow

            hissLFO.setShape(StandardLFO::Sine);
            hissLFO.setRate(0.2f, static_cast<float>(sampleRate)); // slow noise modulation

            hissFilt.setMode(CytomicSVF::Mode::LowPass);
            hissFiltR.setMode(CytomicSVF::Mode::LowPass);
            motorPhase = 0.0f;
            noiseState = 0.0f;
        }

        void reset()
        {
            wowDelay.clear();
            motorPhase = 0.0f;
            noiseState = 0.0f;
            hissFilt.reset();
            hissFiltR.reset();
        }

        void process(float in, float& outL, float& outR,
                     float wowAmt, float flutterAmt, float noiseAmt, float filtAmt)
        {
            float srF = static_cast<float>(sr);

            // Wow: S&H LFO → low-pass filtered → modulates delay time
            float wowRaw = wowLFO.process();
            noiseState = flushDenormal(noiseState * 0.999f + wowRaw * 0.001f); // extreme LP
            float wowSamples = noiseState * wowAmt * srF * 0.001f * 5.0f; // ±5ms wow
            wowSamples = std::max(1.0f, std::min(wowSamples + 2.0f, static_cast<float>(wowDelay.getSize()) - 2.0f));

            // Flutter: fast sine wobble modulating delay time
            float flutterHz = 8.0f + flutterAmt * 4.0f;
            motorPhase = flushDenormal(motorPhase + flutterHz / srF);
            if (motorPhase >= 1.0f) motorPhase -= 1.0f;
            float flutter = fastSin(motorPhase * 6.28318f) * flutterAmt * srF * 0.00003f;

            wowDelay.write(in);
            float delayed = wowDelay.read(wowSamples + std::abs(flutter));

            // Motor hum: 60Hz sine additive
            float motorHum = fastSin(motorPhase * 6.28318f * 60.0f / flutterHz) * flutterAmt * 0.015f;

            // Filtered tape hiss
            float hiss = nextNoise() * noiseAmt * 0.03f;
            float hissFilterFreq = 4000.0f + hissLFO.process() * 2000.0f * filtAmt;
            hissFilterFreq = std::max(1000.0f, std::min(hissFilterFreq, srF * 0.45f));
            hissFilt.setCoefficients(hissFilterFreq, 0.5f, srF);
            hissFiltR.setCoefficients(hissFilterFreq + 200.0f, 0.5f, srF); // slight channel difference
            float hissL = hissFilt.processSample(hiss);
            float hissR = hissFiltR.processSample(nextNoise() * noiseAmt * 0.03f);

            outL = flushDenormal(delayed + motorHum + hissL);
            outR = flushDenormal(delayed * 0.97f + motorHum * 0.96f + hissR);
        }
    } vhs_;

    //==========================================================================
    // Cached parameter pointers (12 params)
    //==========================================================================
    std::atomic<float>* p_meat30hz   = nullptr;
    std::atomic<float>* p_meat60hz   = nullptr;
    std::atomic<float>* p_meatClean  = nullptr;
    std::atomic<float>* p_sddDrive   = nullptr;
    std::atomic<float>* p_sddTone    = nullptr;
    std::atomic<float>* p_tapeComp   = nullptr;
    std::atomic<float>* p_tapeSat    = nullptr;
    std::atomic<float>* p_tapeAge    = nullptr;
    std::atomic<float>* p_vhsWow     = nullptr;
    std::atomic<float>* p_vhsFlutter = nullptr;
    std::atomic<float>* p_vhsNoise   = nullptr;
    std::atomic<float>* p_vhsFilter  = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OsmiumChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    meatBox_.prepare(sampleRate);
    preamp_.prepare(sampleRate);
    tapeComp_.prepare(sampleRate);
    vhs_.prepare(sampleRate);
}

inline void OsmiumChain::reset()
{
    meatBox_.reset();
    preamp_.reset();
    tapeComp_.reset();
    vhs_.reset();
}

inline void OsmiumChain::processBlock(const float* monoIn, float* L, float* R,
                                       int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_meat30hz) return;

    // ParamSnapshot: read all params once
    const float meat30hz   = p_meat30hz->load(std::memory_order_relaxed);
    const float meat60hz   = p_meat60hz->load(std::memory_order_relaxed);
    const float meatClean  = p_meatClean->load(std::memory_order_relaxed);
    const float sddDrive   = p_sddDrive->load(std::memory_order_relaxed);
    const float sddTone    = p_sddTone->load(std::memory_order_relaxed);
    const float tapeComp   = p_tapeComp->load(std::memory_order_relaxed);
    const float tapeSat    = p_tapeSat->load(std::memory_order_relaxed);
    const float tapeAge    = p_tapeAge->load(std::memory_order_relaxed);
    const float vhsWow     = p_vhsWow->load(std::memory_order_relaxed);
    const float vhsFlutter = p_vhsFlutter->load(std::memory_order_relaxed);
    const float vhsNoise   = p_vhsNoise->load(std::memory_order_relaxed);
    const float vhsFilter  = p_vhsFilter->load(std::memory_order_relaxed);

    // Mono pipeline stages 1-3 (write to L as temp mono)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: Sub-Harmonic Synthesizer (Meat Box)
        x = meatBox_.process(x, meat30hz, meat60hz, meatClean);

        // Stage 2: JRC4558 Preamp (SDD-3000)
        x = preamp_.process(x, sddDrive, sddTone, sr_);

        // Stage 3: 4-Track Tape Compression
        x = tapeComp_.process(x, tapeComp, tapeSat, tapeAge, sr_);

        L[i] = x;
    }

    // Stage 4: VHS Degradation — Mono → Stereo
    for (int i = 0; i < numSamples; ++i)
    {
        float wL, wR;
        vhs_.process(L[i], wL, wR, vhsWow, vhsFlutter, vhsNoise, vhsFilter);
        L[i] = wL;
        R[i] = wR;
    }
}

inline void OsmiumChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "osmi_";

    registerFloat(layout, p + "meat30hz",   p + "Sub 30Hz Level",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "meat60hz",   p + "Sub 60Hz Level",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "meatClean",  p + "Meat Clean",
                  0.0f, 1.0f, 0.7f, 0.001f);
    registerFloat(layout, p + "sddDrive",   p + "SDD Drive",
                  0.0f, 1.0f, 0.3f, 0.001f);
    registerFloat(layout, p + "sddTone",    p + "SDD Tone",
                  0.0f, 1.0f, 0.5f, 0.001f);
    registerFloat(layout, p + "tapeComp",   p + "Tape Comp",
                  0.0f, 1.0f, 0.4f, 0.001f);
    registerFloat(layout, p + "tapeSat",    p + "Tape Sat",
                  0.0f, 1.0f, 0.3f, 0.001f);
    registerFloat(layout, p + "tapeAge",    p + "Tape Age",
                  0.0f, 1.0f, 0.4f, 0.001f);
    registerFloat(layout, p + "vhsWow",     p + "VHS Wow",
                  0.0f, 1.0f, 0.3f, 0.001f);
    registerFloat(layout, p + "vhsFlutter", p + "VHS Flutter",
                  0.0f, 1.0f, 0.2f, 0.001f);
    registerFloat(layout, p + "vhsNoise",   p + "VHS Noise",
                  0.0f, 1.0f, 0.2f, 0.001f);
    registerFloat(layout, p + "vhsFilter",  p + "VHS Filter",
                  0.0f, 1.0f, 0.4f, 0.001f);
}

inline void OsmiumChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "osmi_";
    p_meat30hz   = cacheParam(apvts, p + "meat30hz");
    p_meat60hz   = cacheParam(apvts, p + "meat60hz");
    p_meatClean  = cacheParam(apvts, p + "meatClean");
    p_sddDrive   = cacheParam(apvts, p + "sddDrive");
    p_sddTone    = cacheParam(apvts, p + "sddTone");
    p_tapeComp   = cacheParam(apvts, p + "tapeComp");
    p_tapeSat    = cacheParam(apvts, p + "tapeSat");
    p_tapeAge    = cacheParam(apvts, p + "tapeAge");
    p_vhsWow     = cacheParam(apvts, p + "vhsWow");
    p_vhsFlutter = cacheParam(apvts, p + "vhsFlutter");
    p_vhsNoise   = cacheParam(apvts, p + "vhsNoise");
    p_vhsFilter  = cacheParam(apvts, p + "vhsFilter");
}

} // namespace xoceanus
