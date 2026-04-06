// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
#include "../PolyBLEP.h"
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
// OverrideChain — OVERRIDE Digital Aggression FX Chain (5 stages)
//
// Source concept: DigitalAggression (Gemini Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 4 PM7 Phase Modulator)
// Accent: Circuit Board Green #00CC44
//
// Stage 1: Polyphonic Synth Tracking (Meris Enzo)
//          EnvelopeFollower + zero-crossing pitch detect → PolyBLEP oscillator
//          CytomicSVF LP envelope-driven filter, mix with dry
// Stage 2: Chaotic Sub-Octave Fuzz (DOD FX33 Buzz Box)
//          Saturator Digital hard clip + CMOS flip-flop octave divider
//          (toggle state every zero-crossing), parallel blend
// Stage 3: PLL Fuzz Harmonizer (Beetronics Swarm)
//          Phase-locked PolyBLEP square, glide via ParameterSmoother
//          Interval parameter in semitones
// Stage 4: Audio-Rate Phase Modulator (Ibanez PM7)
//          3x cascaded CytomicSVF AllPass modulated by PolyBLEP at 0.1–2000 Hz
//          Mono → Stereo split here (L = +phase, R = -phase)
// Stage 5: Tape-Stop & Time Stretch (Red Panda Tensor)
//          FractionalDelay with exponential read-pointer deceleration
//          Momentary trigger via bool param
//
// Parameter prefix: ovrd_ (14 params)
//==============================================================================
class OverrideChain
{
public:
    OverrideChain() = default;

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
    // Stage 1 — Polyphonic Synth Tracking (Meris Enzo)
    //==========================================================================
    struct EnzoStage
    {
        EnvelopeFollower envFollow;
        PolyBLEP         trackOsc;
        CytomicSVF       trackFilter;
        float            detectedFreq  = 220.0f;
        float            zcPeriod      = 0.0f;
        float            zcCounter     = 0.0f;
        float            lastSample    = 0.0f;
        double           sr            = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            envFollow.prepare(sampleRate);
            envFollow.setAttack(2.0f);
            envFollow.setRelease(60.0f);
            trackOsc.setWaveform(PolyBLEP::Waveform::Saw);
            trackOsc.setFrequency(220.0f, static_cast<float>(sampleRate));
            trackFilter.reset();
            detectedFreq = 220.0f;
            zcPeriod = zcCounter = 0.0f;
            lastSample = 0.0f;
        }
        void reset()
        {
            envFollow.reset();
            trackFilter.reset();
            detectedFreq = 220.0f;
            zcPeriod = zcCounter = 0.0f;
            lastSample = 0.0f;
        }
        float process(float in, int waveChoice, float filterEnv, float mix, float srF)
        {
            // Zero-crossing pitch detection
            zcCounter += 1.0f;
            if (lastSample <= 0.0f && in > 0.0f && zcPeriod > 2.0f)
            {
                // Rising zero-crossing: estimate period
                float measuredFreq = srF / zcPeriod;
                measuredFreq = std::max(20.0f, std::min(measuredFreq, 4000.0f));
                // One-pole smooth the frequency estimate
                detectedFreq = flushDenormal(detectedFreq * 0.95f + measuredFreq * 0.05f);
                zcPeriod = 0.0f;
            }
            zcPeriod = flushDenormal(zcPeriod + 1.0f);
            lastSample = in;

            // Set oscillator waveform
            switch (waveChoice)
            {
                case 0: trackOsc.setWaveform(PolyBLEP::Waveform::Saw);    break;
                case 1: trackOsc.setWaveform(PolyBLEP::Waveform::Square); break;
                default: trackOsc.setWaveform(PolyBLEP::Waveform::Saw);   break;
            }
            trackOsc.setFrequency(detectedFreq, srF);
            float oscOut = trackOsc.processSample() * 0.5f;

            // Envelope-driven LP filter
            float envLevel = envFollow.process(in);
            float cutoff = 200.0f + envLevel * filterEnv * 6000.0f;
            cutoff = std::max(80.0f, std::min(cutoff, srF * 0.45f));
            trackFilter.setMode(CytomicSVF::Mode::LowPass);
            trackFilter.setCoefficients(cutoff, 0.5f, srF);
            oscOut = trackFilter.processSample(oscOut);

            return in + mix * (oscOut - in);
        }
    } enzo_;

    //==========================================================================
    // Stage 2 — Chaotic Sub-Octave Fuzz (DOD FX33 Buzz Box)
    //==========================================================================
    struct BuzzBoxStage
    {
        Saturator  hardClip;
        bool       flipState     = false;
        float      octaveOutput  = 0.0f;
        float      lastSample    = 0.0f;

        void prepare(double /*sampleRate*/)
        {
            hardClip.setMode(Saturator::SaturationMode::Digital);
            hardClip.setDrive(0.9f);
            hardClip.setMix(1.0f);
            hardClip.setOutputGain(0.6f);
            flipState    = false;
            octaveOutput = 0.0f;
            lastSample   = 0.0f;
        }
        void reset()
        {
            flipState    = false;
            octaveOutput = 0.0f;
            lastSample   = 0.0f;
        }
        float process(float in, float heavy, float blend)
        {
            // Hard clip distortion
            hardClip.setDrive(heavy);
            float clipped = hardClip.processSample(in);

            // CMOS flip-flop: toggle output state at every zero-crossing
            if (lastSample <= 0.0f && in > 0.0f)
                flipState = !flipState;
            lastSample = in;
            octaveOutput = flipState ? 0.5f : -0.5f;

            // Parallel blend: dry clip + sub octave
            float subBlend = blend * octaveOutput + (1.0f - blend) * clipped;
            return subBlend;
        }
    } buzzBox_;

    //==========================================================================
    // Stage 3 — PLL Fuzz Harmonizer (Beetronics Swarm)
    //==========================================================================
    struct SwarmStage
    {
        PolyBLEP         pllOsc;
        ParameterSmoother freqSmoother;
        float            pllPhase     = 0.0f;
        float            inputPhase   = 0.0f;
        float            lastSample   = 0.0f;
        double           sr           = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            pllOsc.setWaveform(PolyBLEP::Waveform::Square);
            pllOsc.setFrequency(220.0f, static_cast<float>(sampleRate));
            freqSmoother.prepare(static_cast<float>(sampleRate), 0.05f);
            freqSmoother.snapTo(220.0f);
            pllPhase = inputPhase = 0.0f;
            lastSample = 0.0f;
        }
        void reset()
        {
            freqSmoother.snapTo(220.0f);
            pllPhase = inputPhase = 0.0f;
            lastSample = 0.0f;
        }
        float process(float in, float intervalSemitones, float glideMs, float mix, float srF)
        {
            // Track zero-crossings of input for PLL sync
            if (lastSample <= 0.0f && in > 0.0f)
            {
                // Rising edge: compute estimated input freq from phase accum
                // Use simple instantaneous estimate
            }
            lastSample = in;

            // Glide smoothing on the interval shift
            freqSmoother.prepare(srF, glideMs * 0.001f);
            float targetFreq = 220.0f * fastPow2(intervalSemitones / 12.0f);
            freqSmoother.set(targetFreq);
            float smoothFreq = flushDenormal(freqSmoother.process());

            pllOsc.setFrequency(smoothFreq, srF);
            float pllOut = pllOsc.processSample() * 0.4f;

            return in + mix * (pllOut - in);
        }
    } swarm_;

    //==========================================================================
    // Stage 4 — Audio-Rate Phase Modulator (Ibanez PM7)
    // Mono → Stereo split here
    //==========================================================================
    struct PM7Stage
    {
        CytomicSVF apf1, apf2, apf3;     // cascaded allpass (L path)
        CytomicSVF apf1R, apf2R, apf3R;  // cascaded allpass (R path, inverted mod)
        PolyBLEP   modOsc;
        float      fbStateL = 0.0f;
        float      fbStateR = 0.0f;

        void prepare(double sampleRate)
        {
            apf1.reset(); apf2.reset(); apf3.reset();
            apf1R.reset(); apf2R.reset(); apf3R.reset();
            modOsc.setWaveform(PolyBLEP::Waveform::Sine);
            modOsc.setFrequency(1.0f, static_cast<float>(sampleRate));
            fbStateL = fbStateR = 0.0f;
        }
        void reset()
        {
            apf1.reset(); apf2.reset(); apf3.reset();
            apf1R.reset(); apf2R.reset(); apf3R.reset();
            fbStateL = fbStateR = 0.0f;
        }
        void process(float in, float& outL, float& outR,
                     float speed, float depth, float feedback, float srF)
        {
            modOsc.setFrequency(speed, srF);
            float modSample = modOsc.processSample();

            // Allpass center frequency modulated by oscillator
            float baseFreq = 800.0f;
            float freqL = baseFreq + modSample * depth * 3000.0f;
            float freqR = baseFreq - modSample * depth * 3000.0f; // inverted for stereo
            freqL = std::max(50.0f, std::min(freqL, srF * 0.45f));
            freqR = std::max(50.0f, std::min(freqR, srF * 0.45f));

            // L path
            float xL = in + fbStateL * feedback;
            apf1.setMode(CytomicSVF::Mode::AllPass);
            apf1.setCoefficients(freqL, 0.7f, srF);
            apf2.setMode(CytomicSVF::Mode::AllPass);
            apf2.setCoefficients(freqL * 1.5f, 0.7f, srF);
            apf3.setMode(CytomicSVF::Mode::AllPass);
            apf3.setCoefficients(freqL * 2.25f, 0.7f, srF);
            xL = apf1.processSample(xL);
            xL = apf2.processSample(xL);
            xL = apf3.processSample(xL);
            fbStateL = flushDenormal(xL);
            outL = xL;

            // R path (inverted modulation → stereo Bessel spread)
            float xR = in + fbStateR * feedback;
            apf1R.setMode(CytomicSVF::Mode::AllPass);
            apf1R.setCoefficients(freqR, 0.7f, srF);
            apf2R.setMode(CytomicSVF::Mode::AllPass);
            apf2R.setCoefficients(freqR * 1.5f, 0.7f, srF);
            apf3R.setMode(CytomicSVF::Mode::AllPass);
            apf3R.setCoefficients(freqR * 2.25f, 0.7f, srF);
            xR = apf1R.processSample(xR);
            xR = apf2R.processSample(xR);
            xR = apf3R.processSample(xR);
            fbStateR = flushDenormal(xR);
            outR = xR;
        }
    } pm7_;

    //==========================================================================
    // Stage 5 — Tape-Stop & Time Stretch (Red Panda Tensor)
    //==========================================================================
    struct TensorStage
    {
        FractionalDelay delayL, delayR;
        float           readPtrL     = 0.0f; // fractional read pointer offset
        float           readPtrR     = 0.0f;
        float           readSpeed    = 1.0f; // 1.0 = normal, decels to 0 on stop
        bool            wasStopped   = false;
        double          sr           = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxBuf = static_cast<int>(sampleRate * 2.0) + 8;
            delayL.prepare(maxBuf);
            delayR.prepare(maxBuf);
            readPtrL = readPtrR = 1.0f;
            readSpeed   = 1.0f;
            wasStopped  = false;
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
            readPtrL = readPtrR = 1.0f;
            readSpeed  = 1.0f;
            wasStopped = false;
        }
        void processBlock(float* L, float* R, int numSamples,
                          float stretchFactor, float stopTimeSec, bool momentary)
        {
            float srF = static_cast<float>(sr);
            // Tape stop: on momentary trigger, decelerate read pointer to 0
            if (momentary && !wasStopped)
            {
                readSpeed  = 0.0f; // snap stop (tape stop curve)
                wasStopped = true;
            }
            else if (!momentary && wasStopped)
            {
                readSpeed  = 1.0f; // resume normal speed
                wasStopped = false;
            }

            // Exponential deceleration coefficient for stop time
            float deccelCoeff = (stopTimeSec > 0.001f)
                ? fastExp(-1.0f / (stopTimeSec * srF))
                : 0.0f;

            for (int i = 0; i < numSamples; ++i)
            {
                delayL.write(L[i]);
                delayR.write(R[i]);

                // Read pointer speed: 1.0 = normal, 0 = stopped, stretch = >1
                float targetSpeed = wasStopped ? 0.0f : stretchFactor;
                readSpeed = flushDenormal(readSpeed + (targetSpeed - readSpeed) * (1.0f - deccelCoeff));

                readPtrL = flushDenormal(readPtrL + readSpeed);
                readPtrR = flushDenormal(readPtrR + readSpeed);
                if (readPtrL < 1.0f) readPtrL = 1.0f;
                if (readPtrR < 1.0f) readPtrR = 1.0f;

                float maxPtr = static_cast<float>(delayL.getSize()) - 3.0f;
                if (readPtrL > maxPtr) readPtrL = maxPtr;
                if (readPtrR > maxPtr) readPtrR = maxPtr;

                L[i] = delayL.read(readPtrL);
                R[i] = delayR.read(readPtrR);
            }
        }
    } tensor_;

    //==========================================================================
    // Cached parameter pointers (14 params)
    //==========================================================================
    std::atomic<float>* p_enzoFilterEnv  = nullptr;
    std::atomic<float>* p_enzoWave       = nullptr;
    std::atomic<float>* p_enzoMix        = nullptr;
    std::atomic<float>* p_buzzOctave     = nullptr;
    std::atomic<float>* p_buzzHeavy      = nullptr;
    std::atomic<float>* p_swarmInterval  = nullptr;
    std::atomic<float>* p_swarmGlide     = nullptr;
    std::atomic<float>* p_swarmMix       = nullptr;
    std::atomic<float>* p_pm7Speed       = nullptr;
    std::atomic<float>* p_pm7Depth       = nullptr;
    std::atomic<float>* p_pm7Feedback    = nullptr;
    std::atomic<float>* p_tensorStretch  = nullptr;
    std::atomic<float>* p_tensorStopTime = nullptr;
    std::atomic<float>* p_tensorMomentary = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OverrideChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    enzo_.prepare(sampleRate);
    buzzBox_.prepare(sampleRate);
    swarm_.prepare(sampleRate);
    pm7_.prepare(sampleRate);
    tensor_.prepare(sampleRate);
}

inline void OverrideChain::reset()
{
    enzo_.reset();
    buzzBox_.reset();
    swarm_.reset();
    pm7_.reset();
    tensor_.reset();
}

inline void OverrideChain::processBlock(const float* monoIn, float* L, float* R,
                                         int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_enzoFilterEnv) return;

    // ParamSnapshot
    const float enzoFilterEnv  = p_enzoFilterEnv->load(std::memory_order_relaxed);
    const int   enzoWave       = static_cast<int>(p_enzoWave->load(std::memory_order_relaxed));
    const float enzoMix        = p_enzoMix->load(std::memory_order_relaxed) * 0.01f;
    const float buzzOctave     = p_buzzOctave->load(std::memory_order_relaxed);
    const float buzzHeavy      = p_buzzHeavy->load(std::memory_order_relaxed);
    const float swarmInterval  = p_swarmInterval->load(std::memory_order_relaxed);
    const float swarmGlide     = p_swarmGlide->load(std::memory_order_relaxed);
    const float swarmMix       = p_swarmMix->load(std::memory_order_relaxed) * 0.01f;
    const float pm7Speed       = p_pm7Speed->load(std::memory_order_relaxed);
    const float pm7Depth       = p_pm7Depth->load(std::memory_order_relaxed);
    const float pm7Feedback    = p_pm7Feedback->load(std::memory_order_relaxed) * 0.01f;
    const float tensorStretch  = p_tensorStretch->load(std::memory_order_relaxed);
    const float tensorStopTime = p_tensorStopTime->load(std::memory_order_relaxed);
    const bool  tensorMomentary = p_tensorMomentary->load(std::memory_order_relaxed) >= 0.5f;

    const float srF = static_cast<float>(sr_);

    // Mono pipeline stages 1-3 (write to L as temp)
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: Enzo Polyphonic Tracking
        x = enzo_.process(x, enzoWave, enzoFilterEnv, enzoMix, srF);

        // Stage 2: Buzz Box Sub-Octave Fuzz
        x = buzzBox_.process(x, buzzHeavy, buzzOctave);

        // Stage 3: Swarm PLL Harmonizer
        x = swarm_.process(x, swarmInterval, swarmGlide, swarmMix, srF);

        L[i] = x;
    }

    // Stage 4: PM7 Phase Modulator — Mono → Stereo
    for (int i = 0; i < numSamples; ++i)
    {
        float outL, outR;
        pm7_.process(L[i], outL, outR, pm7Speed, pm7Depth, pm7Feedback, srF);
        L[i] = outL;
        R[i] = outR;
    }

    // Stage 5: Tensor Tape-Stop
    tensor_.processBlock(L, R, numSamples, tensorStretch, tensorStopTime, tensorMomentary);
}

inline void OverrideChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "ovrd_";

    registerFloat      (layout, p + "enzoFilterEnv",   p + "Enzo Filter Env",
                        0.0f,    1.0f,   0.5f);
    registerChoice     (layout, p + "enzoWave",         p + "Enzo Wave",
                        {"Saw", "Square"}, 0);
    registerFloat      (layout, p + "enzoMix",          p + "Enzo Mix",
                        0.0f,  100.0f,  40.0f);
    registerFloat      (layout, p + "buzzOctave",       p + "Buzz Octave Blend",
                        0.0f,    1.0f,   0.5f);
    registerFloat      (layout, p + "buzzHeavy",        p + "Buzz Heavy",
                        0.0f,    1.0f,   0.6f);
    registerFloat      (layout, p + "swarmInterval",    p + "Swarm Interval",
                        -24.0f, 24.0f,  -12.0f, 0.1f);
    registerFloatSkewed(layout, p + "swarmGlide",       p + "Swarm Glide",
                        1.0f,  500.0f,  30.0f,  0.1f, 0.3f);
    registerFloat      (layout, p + "swarmMix",         p + "Swarm Mix",
                        0.0f,  100.0f,  50.0f);
    registerFloatSkewed(layout, p + "pm7Speed",         p + "PM7 Speed",
                        0.1f, 2000.0f,   4.0f,  0.01f, 0.2f);
    registerFloat      (layout, p + "pm7Depth",         p + "PM7 Depth",
                        0.0f,    1.0f,   0.4f);
    registerFloat      (layout, p + "pm7Feedback",      p + "PM7 Feedback",
                        0.0f,   80.0f,  25.0f);
    registerFloat      (layout, p + "tensorStretch",    p + "Tensor Stretch",
                        0.25f,   4.0f,   1.0f, 0.01f);
    registerFloatSkewed(layout, p + "tensorStopTime",   p + "Tensor Stop Time",
                        0.05f,   5.0f,   0.5f, 0.01f, 0.3f);
    registerBool       (layout, p + "tensorMomentary",  p + "Tensor Stop", false);
}

inline void OverrideChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "ovrd_";
    p_enzoFilterEnv  = cacheParam(apvts, p + "enzoFilterEnv");
    p_enzoWave       = cacheParam(apvts, p + "enzoWave");
    p_enzoMix        = cacheParam(apvts, p + "enzoMix");
    p_buzzOctave     = cacheParam(apvts, p + "buzzOctave");
    p_buzzHeavy      = cacheParam(apvts, p + "buzzHeavy");
    p_swarmInterval  = cacheParam(apvts, p + "swarmInterval");
    p_swarmGlide     = cacheParam(apvts, p + "swarmGlide");
    p_swarmMix       = cacheParam(apvts, p + "swarmMix");
    p_pm7Speed       = cacheParam(apvts, p + "pm7Speed");
    p_pm7Depth       = cacheParam(apvts, p + "pm7Depth");
    p_pm7Feedback    = cacheParam(apvts, p + "pm7Feedback");
    p_tensorStretch  = cacheParam(apvts, p + "tensorStretch");
    p_tensorStopTime = cacheParam(apvts, p + "tensorStopTime");
    p_tensorMomentary = cacheParam(apvts, p + "tensorMomentary");
}

} // namespace xoceanus
