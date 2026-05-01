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

namespace xoceanus
{

//==============================================================================
// OpusChain — Tomorrow's Microcosm FX Chain (5 stages)
//
// Source concept: Tomorrow's Microcosm (Boutique Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 3 Dual Optical Phaser)
// Accent: Synapse Violet #9B5DE5
//
// Stage 1: Bucket-Brigade Vibrato (Boss VB-2)
//          FractionalDelay (20ms max) + StandardLFO sine + EnvelopeFollower depth mod
// Stage 2: Granular Micro-Looper (Hologram Microcosm)
//          CircularBuffer (2s) + grain scheduler (50–200ms, Gaussian window,
//          0.5×/–1×/2× playback speeds, stochastic scheduling)
// Stage 3: Dual Optical Phaser (Mutron Bi-Phase)
//          Two CytomicSVF 6-stage allpass cascades, hard L/R pan,
//          StandardLFO per channel with slew-rate limiting. Mono→Stereo here.
// Stage 4: Memory Buffer Delay (Chase Bliss Habit)
//          CircularBuffer (2M samples), random scanner read-pointer,
//          FractionalDelay Hermite interpolation
// Stage 5: Pitch-Smeared Plate Reverb (Meris Mercury7)
//          LushReverb + CircularBuffer pitch-shift in feedback (+1 octave high)
//
// Parameter prefix: opus_ (11 params)
//==============================================================================
class OpusChain
{
public:
    OpusChain() = default;

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
    // Stage 1 — Bucket-Brigade Vibrato (Boss VB-2)
    //==========================================================================
    struct VB2Stage
    {
        static constexpr float kMaxDelayMs = 20.0f;

        FractionalDelay delay;
        StandardLFO     lfo;
        EnvelopeFollower envFollow;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamp = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 8;
            delay.prepare(maxSamp);
            lfo.setShape(StandardLFO::Sine);
            lfo.reset();
            envFollow.prepare(sampleRate);
            envFollow.setAttack(10.0f);
            envFollow.setRelease(200.0f);
        }
        void reset()
        {
            delay.clear();
            lfo.reset();
            envFollow.reset();
        }
        float process(float in, float rateHz, float depth)
        {
            // EnvelopeFollower modulates depth — louder signals = more depth
            float envGain = envFollow.process(in);
            float effDepth = depth * (0.5f + 0.5f * envGain);

            lfo.setRate(rateHz, static_cast<float>(sr));
            float lfoOut = lfo.process(); // [-1, +1]

            // Delay time oscillates ±depth ms around center 10ms
            float centreMs = kMaxDelayMs * 0.5f;
            float delayMs  = centreMs + lfoOut * effDepth * centreMs;
            delayMs = std::max(1.0f, std::min(delayMs, kMaxDelayMs - 0.5f));
            float delaySamp = delayMs * static_cast<float>(sr) / 1000.0f;

            delay.write(in);
            return delay.read(delaySamp);
        }
    } vb2_;

    //==========================================================================
    // Stage 2 — Granular Micro-Looper (Hologram Microcosm)
    //==========================================================================
    struct GrainLooperStage
    {
        static constexpr int   kBufSamples  = 96001; // 2s @ 48kHz
        static constexpr int   kMaxGrains   = 8;
        static constexpr float kMinGrainMs  = 50.0f;
        static constexpr float kMaxGrainMs  = 200.0f;

        CircularBuffer rec;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        // Grain state
        struct Grain
        {
            bool  active     = false;
            int   startOff   = 0;   // offset from write head when grain started
            int   lenSamp    = 0;   // grain length in samples
            int   pos        = 0;   // read position within grain
            float speed      = 1.0f; // +0.5, -1.0, or +2.0
        };
        std::array<Grain, kMaxGrains> grains;

        // Stochastic scheduler state
        float   schedCountdown  = 0.0f; // samples until next grain launch
        uint32_t rngState       = 99991u;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int bufSize = static_cast<int>(2.1 * sampleRate) + 4;
            rec.prepare(bufSize);
            for (auto& g : grains)
                g.active = false;
            schedCountdown = static_cast<float>(sampleRate) * 0.05f;
        }
        void reset()
        {
            rec.clear();
            for (auto& g : grains)
                g.active = false;
            schedCountdown = 2205.0f;
            rngState = 99991u;
        }

        // Simple LCG RNG
        float nextRand()
        {
            rngState = rngState * 1664525u + 1013904223u;
            return static_cast<float>(rngState & 0xFFFF) / 65535.0f; // [0,1]
        }

        // Gaussian window approximation (triangle blended with raised cosine)
        static float grainWindow(float t) // t in [0,1]
        {
            // Raised-cosine (Hann) window
            constexpr float kPi = 3.14159265358979323846f;
            return 0.5f * (1.0f - fastCos(kPi * (2.0f * t - 1.0f)));
        }

        float process(float in, float activity, int shapeMode)
        {
            // activity [0,1] — controls grain density / frequency
            rec.write(in);

            // Spawn new grain?
            schedCountdown -= 1.0f;
            if (schedCountdown <= 0.0f)
            {
                // Inter-grain interval proportional to 1/activity (more activity = more grains)
                float actClamped = std::max(0.01f, activity);
                float intervalSamp = static_cast<float>(sr) * 0.05f / actClamped;
                schedCountdown = intervalSamp * (0.5f + nextRand());

                // Find a free grain slot
                for (auto& g : grains)
                {
                    if (!g.active)
                    {
                        float lenMs = kMinGrainMs + nextRand() * (kMaxGrainMs - kMinGrainMs);
                        g.lenSamp  = static_cast<int>(lenMs * static_cast<float>(sr) / 1000.0f);
                        g.pos      = 0;
                        // Read from a random location in the last 2 seconds
                        int maxOff = rec.getSize() - 1;
                        g.startOff = static_cast<int>(nextRand() * static_cast<float>(maxOff));
                        // Speed selection: shapeMode 0=Fwd(0.5×), 1=Rev(-1×), 2=Fast(2×)
                        switch (shapeMode)
                        {
                        case 0:  g.speed =  0.5f; break;
                        case 1:  g.speed = -1.0f; break;
                        default: g.speed =  2.0f; break;
                        }
                        g.active = true;
                        break;
                    }
                }
            }

            // Mix active grains
            float out = 0.0f;
            int activeCount = 0;
            for (auto& g : grains)
            {
                if (!g.active) continue;
                ++activeCount;

                float t = static_cast<float>(g.pos) / static_cast<float>(std::max(1, g.lenSamp));
                float win = grainWindow(t);

                // Read position with speed (fractional)
                float readOff;
                if (g.speed >= 0.0f)
                    readOff = static_cast<float>(g.startOff) + static_cast<float>(g.pos) * g.speed;
                else
                    readOff = static_cast<float>(g.startOff + g.lenSamp) + static_cast<float>(g.pos) * g.speed;

                int iOff = static_cast<int>(readOff);
                iOff = iOff % std::max(1, rec.getSize());
                out += rec.readForward(iOff) * win;

                ++g.pos;
                if (g.pos >= g.lenSamp)
                    g.active = false;
            }
            if (activeCount > 0)
                out /= static_cast<float>(activeCount);

            out = flushDenormal(out);
            return out;
        }
    } grainLooper_;

    //==========================================================================
    // Stage 3 — Dual Optical Phaser (Mutron Bi-Phase) — Mono → Stereo
    //==========================================================================
    struct BiPhaseStage
    {
        // 6 allpass stages per channel
        static constexpr int kNumAP = 6;

        std::array<CytomicSVF, kNumAP> apL;
        std::array<CytomicSVF, kNumAP> apR;
        StandardLFO lfoL;
        StandardLFO lfoR;
        ParameterSmoother freqSmL;
        ParameterSmoother freqSmR;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            for (auto& f : apL) f.reset();
            for (auto& f : apR) f.reset();
            lfoL.setShape(StandardLFO::Sine);
            lfoR.setShape(StandardLFO::Sine);
            lfoR.setPhaseOffset(0.5f); // 180° offset for stereo spread
            lfoL.reset();
            lfoR.reset();
            freqSmL.prepare(static_cast<float>(sampleRate), 0.003f);
            freqSmR.prepare(static_cast<float>(sampleRate), 0.003f);
            freqSmL.snapTo(800.0f);
            freqSmR.snapTo(800.0f);
        }
        void reset()
        {
            for (auto& f : apL) f.reset();
            for (auto& f : apR) f.reset();
            lfoL.reset();
            lfoR.reset();
            freqSmL.snapTo(800.0f);
            freqSmR.snapTo(800.0f);
        }

        void process(float in, float& outL, float& outR,
                     float rateHz, float depth, float sweepCentreHz)
        {
            float srF = static_cast<float>(sr);
            lfoL.setRate(rateHz, srF);
            lfoR.setRate(rateHz, srF);

            float modL = lfoL.process(); // [-1,+1]
            float modR = lfoR.process();

            // Frequency sweep range: sweepCentreHz ± depth*octaves
            float fL = sweepCentreHz * fastPow2(modL * depth);
            float fR = sweepCentreHz * fastPow2(modR * depth);
            fL = std::max(20.0f, std::min(fL, srF * 0.45f));
            fR = std::max(20.0f, std::min(fR, srF * 0.45f));

            freqSmL.set(fL);
            freqSmR.set(fR);
            float smoothL = freqSmL.process();
            float smoothR = freqSmR.process();

            // Apply 6 allpass stages per channel
            float xL = in;
            float xR = in;
            for (int i = 0; i < kNumAP; ++i)
            {
                apL[i].setMode(CytomicSVF::Mode::AllPass);
                apL[i].setCoefficients(smoothL, 0.5f, srF);
                xL = apL[i].processSample(xL);

                apR[i].setMode(CytomicSVF::Mode::AllPass);
                apR[i].setCoefficients(smoothR, 0.5f, srF);
                xR = apR[i].processSample(xR);
            }

            // Mix phased signal with dry (classic phaser blend = 0.5)
            outL = (in + xL) * 0.5f;
            outR = (in + xR) * 0.5f;
        }
    } biphase_;

    //==========================================================================
    // Stage 4 — Memory Buffer Delay (Chase Bliss Habit)
    // Long CircularBuffer with random scanner read-pointer
    //==========================================================================
    struct HabitStage
    {
        static constexpr int kBufCap = 2000000; // ~41s @ 48kHz

        CircularBuffer  buf;
        FractionalDelay fracDel;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        // Scanner state
        float   scanPos      = 0.0f; // current fractional offset (samples)
        float   scanTarget   = 0.0f;
        float   scanSpeed    = 0.0f;
        uint32_t rngState    = 7654321u;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            buf.prepare(kBufCap);
            fracDel.prepare(kBufCap + 8);
            scanPos = scanTarget = static_cast<float>(sampleRate) * 0.5f;
        }
        void reset()
        {
            buf.clear();
            fracDel.clear();
            scanPos = scanTarget = static_cast<float>(sr) * 0.5f;
            rngState = 7654321u;
        }
        float nextRand()
        {
            rngState = rngState * 1664525u + 1013904223u;
            return static_cast<float>(rngState & 0xFFFF) / 65535.0f;
        }
        float process(float in, float scanRateHz, float spread)
        {
            // Write into both delay structures
            buf.write(in);
            fracDel.write(in);

            // Slowly drift scan target to a new random position
            scanSpeed = scanRateHz / static_cast<float>(sr);
            scanPos += (scanTarget - scanPos) * scanSpeed * 0.1f;
            scanPos = flushDenormal(scanPos);

            // Occasionally jump to a new random target
            if (std::abs(scanPos - scanTarget) < 10.0f)
            {
                float maxOff = static_cast<float>(kBufCap - 1) * spread;
                scanTarget = maxOff * nextRand();
                scanTarget = std::max(1.0f, std::min(scanTarget, static_cast<float>(kBufCap) - 2.0f));
            }

            float delaySamp = std::max(1.0f, std::min(scanPos, static_cast<float>(kBufCap) - 2.0f));
            return fracDel.read(delaySamp);
        }
    } habit_;

    //==========================================================================
    // Stage 5 — Pitch-Smeared Plate Reverb (Meris Mercury7)
    // LushReverb + CircularBuffer pitch-shift in feedback path
    //==========================================================================
    struct Mercury7Stage
    {
        LushReverb  reverb;
        CircularBuffer pitchBuf;
        double sr = 0.0;  // Sentinel: must be set by prepare() before use
        float  feedbackL = 0.0f;
        float  feedbackR = 0.0f;

        // Pitch shift: read at 2× speed = +1 octave
        float   pitchReadPosL = 0.0f;
        float   pitchReadPosR = 0.0f;
        static constexpr float kPitchBufMs = 50.0f; // 50ms pitch buffer

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            reverb.prepare(sampleRate);
            int pitchBufSamp = static_cast<int>(kPitchBufMs * sampleRate / 1000.0) + 8;
            pitchBuf.prepare(pitchBufSamp);
            feedbackL = feedbackR = 0.0f;
            pitchReadPosL = pitchReadPosR = 0.0f;
        }
        void reset()
        {
            reverb.reset();
            pitchBuf.clear();
            feedbackL = feedbackR = 0.0f;
            pitchReadPosL = pitchReadPosR = 0.0f;
        }

        // Read from circular buffer at given fractional speed (upward pitch shift)
        float pitchShiftRead(float& readPos, int bufSize)
        {
            // Advance read at 2× to get +1 octave
            readPos += 2.0f;
            if (readPos >= static_cast<float>(bufSize))
                readPos -= static_cast<float>(bufSize);
            int iR = static_cast<int>(readPos);
            return pitchBuf.readForward(iR % bufSize);
        }

        void processBlock(const float* inL, const float* inR,
                          float* outL, float* outR, int numSamples,
                          float decaySec, float pitchVector)
        {
            reverb.setDecay(decaySec);
            reverb.setSize(0.8f);
            reverb.setDamping(0.3f);
            reverb.setMix(1.0f); // wet only — caller handles dry/wet
            reverb.setDiffusion(0.7f);
            reverb.setModulation(0.3f);

            int pitchBufSize = pitchBuf.getSize();

            for (int i = 0; i < numSamples; ++i)
            {
                // Feed input + pitched feedback into reverb input
                float pitchFbL = pitchShiftRead(pitchReadPosL, pitchBufSize) * pitchVector;
                float pitchFbR = pitchShiftRead(pitchReadPosR, pitchBufSize) * pitchVector;

                float rvbInL = inL[i] + feedbackL * pitchFbL;
                float rvbInR = inR[i] + feedbackR * pitchFbR;

                // Process through reverb (single sample via block of 1)
                float rvbOutL = 0.0f, rvbOutR = 0.0f;
                reverb.processBlock(&rvbInL, &rvbInR, &rvbOutL, &rvbOutR, 1);

                outL[i] = rvbOutL;
                outR[i] = rvbOutR;

                // Store output for pitch feedback
                pitchBuf.write(rvbOutL);
                feedbackL = flushDenormal(rvbOutL);
                feedbackR = flushDenormal(rvbOutR);
            }
        }
    } mercury7_;

    //==========================================================================
    // Temporary work buffer (mono, allocated in prepare)
    //==========================================================================
    std::vector<float> tmpL_;
    std::vector<float> tmpR_;

    //==========================================================================
    // Cached parameter pointers (11 params)
    //==========================================================================
    std::atomic<float>* p_vb2Rate       = nullptr;
    std::atomic<float>* p_vb2Depth      = nullptr;
    std::atomic<float>* p_microActivity = nullptr;
    std::atomic<float>* p_microShape    = nullptr;
    std::atomic<float>* p_biphaseRate   = nullptr;
    std::atomic<float>* p_biphaseDepth  = nullptr;
    std::atomic<float>* p_biphaseSweep  = nullptr;
    std::atomic<float>* p_habitScanRate = nullptr;
    std::atomic<float>* p_habitSpread   = nullptr;
    std::atomic<float>* p_mercDecay     = nullptr;
    std::atomic<float>* p_mercPitchVec  = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OpusChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    vb2_.prepare(sampleRate);
    grainLooper_.prepare(sampleRate);
    biphase_.prepare(sampleRate);
    habit_.prepare(sampleRate);
    mercury7_.prepare(sampleRate);
    tmpL_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
    tmpR_.resize(static_cast<size_t>(maxBlockSize), 0.0f);
}

inline void OpusChain::reset()
{
    vb2_.reset();
    grainLooper_.reset();
    biphase_.reset();
    habit_.reset();
    mercury7_.reset();
}

inline void OpusChain::processBlock(const float* monoIn, float* L, float* R,
                                     int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_vb2Rate) return;

    // ParamSnapshot
    const float vb2Rate       = p_vb2Rate->load(std::memory_order_relaxed);
    const float vb2Depth      = p_vb2Depth->load(std::memory_order_relaxed);
    const float microActivity = p_microActivity->load(std::memory_order_relaxed);
    const int   microShape    = static_cast<int>(p_microShape->load(std::memory_order_relaxed));
    const float biphaseRate   = p_biphaseRate->load(std::memory_order_relaxed);
    const float biphaseDepth  = p_biphaseDepth->load(std::memory_order_relaxed);
    const float biphaseSweep  = p_biphaseSweep->load(std::memory_order_relaxed);
    const float habitScanRate = p_habitScanRate->load(std::memory_order_relaxed);
    const float habitSpread   = p_habitSpread->load(std::memory_order_relaxed);
    const float mercDecay     = p_mercDecay->load(std::memory_order_relaxed);
    const float mercPitchVec  = p_mercPitchVec->load(std::memory_order_relaxed);

    // Mono pipeline stages 1–2; stereo expansion at stage 3
    for (int i = 0; i < numSamples; ++i)
    {
        float x = monoIn[i];

        // Stage 1: VB-2 Vibrato
        x = vb2_.process(x, vb2Rate, vb2Depth);

        // Stage 2: Granular Micro-Looper
        x = grainLooper_.process(x, microActivity, microShape);

        // Stage 3: Dual Optical Phaser → stereo expansion
        float xL = 0.0f, xR = 0.0f;
        biphase_.process(x, xL, xR, biphaseRate, biphaseDepth, biphaseSweep);

        // Stage 4: Habit delay on each channel independently
        tmpL_[i] = habit_.process(xL, habitScanRate, habitSpread);
        tmpR_[i] = xR; // R goes dry into reverb (habit only on L to add interest)
    }

    // Stage 5: Mercury7 reverb on both channels
    mercury7_.processBlock(tmpL_.data(), tmpR_.data(), L, R, numSamples,
                           mercDecay, mercPitchVec);
}

inline void OpusChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "opus_";
    // D005 (must breathe): vb2Rate floor lowered 0.1 → 0.005 Hz, matching
    // StandardLFO::setRate's internal clamp at Source/DSP/StandardLFO.h:54
    // (going lower in the param range is illusory — caught on review). Now
    // uses registerFloatSkewed because the range spans 3+ decades; without
    // skew the bottom of the knob would be unreachable in practice. Default
    // 1.0 Hz unchanged. habitScanRate (below) is already at 0.01 Hz.
    registerFloatSkewed(layout, p + "vb2Rate", p + "VB2 Rate",
                        0.005f, 8.0f, 1.0f, 0.001f, 0.3f);
    registerFloat(layout, p + "vb2Depth",      p + "VB2 Depth",      0.0f,   1.0f,  0.5f);
    registerFloat(layout, p + "microActivity", p + "Micro Activity", 0.0f,   1.0f,  0.3f);
    registerChoice(layout, p + "microShape", p + "Micro Shape",
                   {"Fwd", "Rev", "Fast"}, 0);
    // D005: biphaseRate floor lowered 0.05 → 0.005 Hz on the dual optical
    // phaser. Same StandardLFO clamp + skew rationale as vb2Rate.
    registerFloatSkewed(layout, p + "biphaseRate", p + "Biphase Rate",
                        0.005f, 5.0f, 0.5f, 0.001f, 0.3f);
    registerFloat(layout, p + "biphaseDepth", p + "Biphase Depth",  0.0f,   3.0f,  1.5f);
    registerFloat(layout, p + "biphaseSweep", p + "Biphase Sweep",  200.0f, 4000.0f, 800.0f);
    registerFloat(layout, p + "habitScanRate",p + "Habit Scan Rate",0.01f,  2.0f,  0.2f);
    registerFloat(layout, p + "habitSpread",  p + "Habit Spread",   0.0f,   1.0f,  0.5f);
    registerFloat(layout, p + "mercDecay",    p + "Merc Decay",     0.5f,  20.0f,  4.0f);
    registerFloat(layout, p + "mercPitchVec", p + "Merc Pitch Vec", 0.0f,   1.0f,  0.3f);
}

inline void OpusChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "opus_";
    p_vb2Rate       = cacheParam(apvts, p + "vb2Rate");
    p_vb2Depth      = cacheParam(apvts, p + "vb2Depth");
    p_microActivity = cacheParam(apvts, p + "microActivity");
    p_microShape    = cacheParam(apvts, p + "microShape");
    p_biphaseRate   = cacheParam(apvts, p + "biphaseRate");
    p_biphaseDepth  = cacheParam(apvts, p + "biphaseDepth");
    p_biphaseSweep  = cacheParam(apvts, p + "biphaseSweep");
    p_habitScanRate = cacheParam(apvts, p + "habitScanRate");
    p_habitSpread   = cacheParam(apvts, p + "habitSpread");
    p_mercDecay     = cacheParam(apvts, p + "mercDecay");
    p_mercPitchVec  = cacheParam(apvts, p + "mercPitchVec");
}

} // namespace xoceanus
