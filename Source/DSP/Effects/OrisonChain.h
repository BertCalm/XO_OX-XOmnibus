// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
#include "LushReverb.h"
#include "FXChainHelpers.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OrisonChain — ORISON Shattered Cathedral FX Chain (4 stages)
//
// Source concept: Shattered Cathedral (Broken Rule Pedalboard Series)
// Routing: Mono In → Stereo Out (expansion at Stage 2 Unstable Fuzz)
// Accent: Cathedral Stone Grey #8B8682
//
// Stage 1: 1980s Hall Reverb (Lexicon 224) — LushReverb at 100% wet, long
//          decay (up to 10s). Removes all transients; creates pure harmonic
//          wash before any dynamics-sensitive stage.
// Stage 2: Unstable Oscillating Fuzz (Zvex Fuzz Factory) — Saturator extreme
//          drive + 1-sample FractionalDelay feedback self-oscillation loop.
//          SchmittTrigger gates on reverb tail phase-cancellations.
//          Mono → Stereo split here (different feedback seeds L/R).
// Stage 3: 8-Bit Crusher + Filter (Oto Machines Biscuit) — bit-depth
//          reduction (quantize to 2^N levels) + CytomicSVF LP sweeping the
//          crushed artefacts. Applied independently to L and R.
// Stage 4: Multi-Head Tape Delay (Roland RE-201) — 3 FractionalDelay read
//          heads, Saturator::Tape in feedback, StandardLFO wow/flutter,
//          CytomicSVF LP darkening per repeat.
//
// BROKEN RULE: Hall reverb at 100% wet runs BEFORE the transient-dependent
// fuzz stage — all transients are stripped; the fuzz reacts to the reverb tail.
//
// Parameter prefix: oris_ (10 params)
//==============================================================================
class OrisonChain
{
public:
    OrisonChain() = default;

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
    double sr_        = 0.0;  // Sentinel: must be set by prepare() before use
    int    blockSize_ = 512;

    // Temporary mono working buffer (allocated in prepare, size = maxBlockSize).
    std::vector<float> monoWork_;

    //==========================================================================
    // Stage 1 — 1980s Hall Reverb (Lexicon 224) — 100% wet
    //==========================================================================
    struct HallReverbStage
    {
        LushReverb reverb;

        void prepare(double sampleRate)
        {
            reverb.prepare(sampleRate);
            // Default: large hall feel with very long decay
            reverb.setSize(0.9f);
            reverb.setDecay(5.0f);
            reverb.setDamping(0.3f);
            reverb.setDiffusion(0.8f);
            reverb.setModulation(0.1f);
            reverb.setMix(1.0f);   // 100% wet — Broken Rule
            reverb.setWidth(0.9f);
        }
        void reset() { reverb.reset(); }

        // Returns mono sum of L+R reverb output (used as the single mono path).
        // outL / outR receive separate channels so Stage 2 can split stereo.
        void process(const float* monoIn, float* outL, float* outR, int numSamples,
                     float decaySec, float predelayMs)
        {
            reverb.setDecay(decaySec);
            reverb.setPreDelay(predelayMs);
            reverb.setMix(1.0f);
            reverb.processBlock(monoIn, monoIn, outL, outR, numSamples);
        }
    } hall_;

    //==========================================================================
    // Stage 2 — Unstable Oscillating Fuzz (Zvex Fuzz Factory)
    // Extreme Saturator drive + 1-sample self-oscillation feedback loop.
    // SchmittTrigger detects phase-cancellation nulls in the reverb tail.
    // Mono → Stereo split: L/R get slightly different feedback seeds.
    //==========================================================================
    struct FuzzStage
    {
        Saturator satL, satR;
        float     feedbackL = 0.0f;
        float     feedbackR = 0.0f;
        SchmittTrigger schmittL, schmittR;

        void prepare(double sampleRate)
        {
            satL.prepare(sampleRate);
            satR.prepare(sampleRate);
            satL.setMode(Saturator::SaturationMode::FoldBack);
            satR.setMode(Saturator::SaturationMode::FoldBack);
            schmittL.setThresholds(0.04f, 0.08f);
            schmittR.setThresholds(0.03f, 0.07f);
            reset();
        }
        void reset()
        {
            feedbackL = feedbackR = 0.0f;
            schmittL.reset();
            schmittR.reset();
        }

        void processStereo(const float* inL, const float* inR,
                           float* outL, float* outR, int numSamples,
                           float drive, float stabAmount)
        {
            // stabAmount: 0=fully unstable (max feedback), 1=stable (no feedback).
            // Invert so high param value = more self-oscillation.
            const float fbGain = (1.0f - stabAmount) * 0.92f;

            satL.setDrive(drive);
            satR.setDrive(drive);
            satL.setOutputGain(1.0f / (1.0f + drive * 10.0f));
            satR.setOutputGain(1.0f / (1.0f + drive * 10.0f));
            satL.setMix(1.0f);
            satR.setMix(1.0f);

            for (int i = 0; i < numSamples; ++i)
            {
                float xL = inL[i] + feedbackL * fbGain;
                float xR = inR[i] + feedbackR * fbGain;

                // SchmittTrigger on phase-cancellation nulls → gate fuzz momentarily
                bool gateL = !schmittL.process(xL);
                bool gateR = !schmittR.process(xR);

                xL = gateL ? satL.processSample(xL) : xL * 0.1f;
                xR = gateR ? satR.processSample(xR) : xR * 0.1f;

                feedbackL = flushDenormal(xL);
                feedbackR = flushDenormal(xR);

                outL[i] = xL;
                outR[i] = xR;
            }
        }
    } fuzz_;

    //==========================================================================
    // Stage 3 — 8-Bit Crusher + Filter (Oto Machines Biscuit)
    // Bit-depth reduction to 2^N levels + CytomicSVF LP on artifacts.
    // Applied to both L and R channels.
    //==========================================================================
    struct BitCrusherStage
    {
        CytomicSVF svfL, svfR;
        ParameterSmoother cutoffSmootherL, cutoffSmootherR;

        void prepare(double sampleRate)
        {
            svfL.setMode(CytomicSVF::Mode::LowPass);
            svfR.setMode(CytomicSVF::Mode::LowPass);
            cutoffSmootherL.prepare(static_cast<float>(sampleRate), 0.005f);
            cutoffSmootherR.prepare(static_cast<float>(sampleRate), 0.005f);
            cutoffSmootherL.snapTo(8000.0f);
            cutoffSmootherR.snapTo(8000.0f);
        }
        void reset()
        {
            svfL.reset();
            svfR.reset();
            cutoffSmootherL.snapTo(8000.0f);
            cutoffSmootherR.snapTo(8000.0f);
        }

        static float crush(float in, int bits)
        {
            // bits in [1,8]. Quantise to 2^bits levels in [-1,+1].
            bits = std::max(1, std::min(8, bits));
            const float levels = static_cast<float>(1 << bits); // 2..256
            return std::round(in * levels) / levels;
        }

        void processStereo(float* L, float* R, int numSamples,
                           int bits, float cutoffHz, float resonance, double sampleRate)
        {
            const float srF = static_cast<float>(sampleRate);
            const float res = juce::jlimit(0.0f, 0.97f, resonance);

            cutoffSmootherL.set(cutoffHz);
            cutoffSmootherR.set(cutoffHz);

            for (int i = 0; i < numSamples; ++i)
            {
                float cL = cutoffSmootherL.process();
                float cR = cutoffSmootherR.process();

                svfL.setCoefficients(cL, res, srF);
                svfR.setCoefficients(cR, res, srF);

                L[i] = svfL.processSample(crush(L[i], bits));
                R[i] = svfR.processSample(crush(R[i], bits));
            }
        }
    } bitCrusher_;

    //==========================================================================
    // Stage 4 — Multi-Head Tape Delay (Roland RE-201)
    // 3 FractionalDelay read heads, Saturator::Tape in feedback,
    // StandardLFO wow/flutter modulation, CytomicSVF LP darkening per repeat.
    //==========================================================================
    struct TapeDelayStage
    {
        static constexpr float kMaxDelayMs    = 1800.0f;
        static constexpr float kMaxFeedbackMs = kMaxDelayMs;

        // Head tap ratios relative to base delay time (classic RE-201 spacing)
        static constexpr float kHeadRatios[3] = { 1.0f, 1.5f, 2.25f };

        FractionalDelay delL, delR;
        Saturator       tapeSatL, tapeSatR;
        CytomicSVF      darkSVFL, darkSVFR;
        StandardLFO     wowLFO;
        ParameterSmoother wearSmoother;

        double sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 16;
            delL.prepare(maxSamples);
            delR.prepare(maxSamples);

            tapeSatL.prepare(sampleRate);
            tapeSatR.prepare(sampleRate);
            tapeSatL.setMode(Saturator::SaturationMode::Tape);
            tapeSatR.setMode(Saturator::SaturationMode::Tape);
            tapeSatL.setMix(1.0f);
            tapeSatR.setMix(1.0f);

            darkSVFL.setMode(CytomicSVF::Mode::LowPass);
            darkSVFR.setMode(CytomicSVF::Mode::LowPass);

            wowLFO.setShape(StandardLFO::Sine);
            wowLFO.setRate(0.55f, static_cast<float>(sampleRate)); // ~0.55 Hz wow

            wearSmoother.prepare(static_cast<float>(sampleRate), 0.02f);
            wearSmoother.snapTo(0.5f);

            reset();
        }
        void reset()
        {
            delL.clear();
            delR.clear();
            darkSVFL.reset();
            darkSVFR.reset();
            tapeSatL.reset();
            tapeSatR.reset();
            wowLFO.reset();
            wearSmoother.snapTo(0.5f);
        }

        void processStereo(const float* inL, const float* inR,
                           float* outL, float* outR, int numSamples,
                           float baseDelayMs, float feedback, float wear,
                           double sampleRate)
        {
            const float srF    = static_cast<float>(sampleRate);
            const float fb     = juce::jlimit(0.0f, 0.85f, feedback);
            const float maxD   = static_cast<float>(delL.getSize()) - 4.0f;

            tapeSatL.setDrive(wear * 0.6f);
            tapeSatR.setDrive(wear * 0.6f);
            tapeSatL.setOutputGain(1.0f / (1.0f + wear * 5.0f));
            tapeSatR.setOutputGain(1.0f / (1.0f + wear * 5.0f));

            wearSmoother.set(wear);

            // Dark filter cutoff: wear reduces high-frequency content
            // Range 20kHz (no wear) → 800Hz (max wear)
            for (int i = 0; i < numSamples; ++i)
            {
                const float wearSmoothed = wearSmoother.process();
                const float darkCutoff = 20000.0f * fastPow2(-wearSmoothed * 4.64f); // 2^-4.64≈0.04
                darkSVFL.setCoefficients(darkCutoff, 0.5f, srF);
                darkSVFR.setCoefficients(darkCutoff, 0.5f, srF);

                // Wow/flutter: LFO modulates delay time ±3ms
                const float wowMod  = wowLFO.process() * 3.0f; // ±3ms
                const float baseSmp = std::max(1.0f, std::min(baseDelayMs * srF / 1000.0f, maxD));

                // Sum 3 heads (attenuate each: 0.5, 0.35, 0.2)
                float tapSumL = 0.0f, tapSumR = 0.0f;
                const float tapGains[3] = { 0.5f, 0.35f, 0.2f };
                for (int h = 0; h < 3; ++h)
                {
                    float dSmp = std::max(1.0f, std::min(baseSmp * kHeadRatios[h] + wowMod * srF / 1000.0f, maxD));
                    tapSumL += delL.read(dSmp) * tapGains[h];
                    tapSumR += delR.read(dSmp) * tapGains[h];
                }

                // Tape saturation + darkening in feedback path
                float fbInL = tapeSatL.processSample(tapSumL);
                float fbInR = tapeSatR.processSample(tapSumR);
                fbInL = darkSVFL.processSample(fbInL);
                fbInR = darkSVFR.processSample(fbInR);

                delL.write(flushDenormal(inL[i] + fbInL * fb));
                delR.write(flushDenormal(inR[i] + fbInR * fb));

                outL[i] = inL[i] + tapSumL;
                outR[i] = inR[i] + tapSumR;
            }
        }
    } tapeDelay_;

    //==========================================================================
    // Cached parameter pointers (10 params)
    //==========================================================================
    std::atomic<float>* p_verbDecay      = nullptr;
    std::atomic<float>* p_verbPredelay   = nullptr;
    std::atomic<float>* p_fuzzGate       = nullptr;
    std::atomic<float>* p_fuzzStab       = nullptr;
    std::atomic<float>* p_biscuitBits    = nullptr;
    std::atomic<float>* p_biscuitCutoff  = nullptr;
    std::atomic<float>* p_biscuitRes     = nullptr;
    std::atomic<float>* p_spaceTime      = nullptr;
    std::atomic<float>* p_spaceFeedback  = nullptr;
    std::atomic<float>* p_spaceWear      = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OrisonChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    monoWork_.assign(static_cast<size_t>(maxBlockSize), 0.0f);
    hall_.prepare(sampleRate);
    fuzz_.prepare(sampleRate);
    bitCrusher_.prepare(sampleRate);
    tapeDelay_.prepare(sampleRate);
}

inline void OrisonChain::reset()
{
    hall_.reset();
    fuzz_.reset();
    bitCrusher_.reset();
    tapeDelay_.reset();
}

inline void OrisonChain::processBlock(const float* monoIn, float* L, float* R, int numSamples)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_verbDecay) return;

    // ParamSnapshot — read all params once per block
    const float verbDecay     = p_verbDecay->load(std::memory_order_relaxed);
    const float verbPredelay  = p_verbPredelay->load(std::memory_order_relaxed);
    const float fuzzGate      = p_fuzzGate->load(std::memory_order_relaxed);
    const float fuzzStab      = p_fuzzStab->load(std::memory_order_relaxed);
    const int   biscuitBits   = static_cast<int>(p_biscuitBits->load(std::memory_order_relaxed) + 0.5f);
    const float biscuitCutoff = p_biscuitCutoff->load(std::memory_order_relaxed);
    const float biscuitRes    = p_biscuitRes->load(std::memory_order_relaxed);
    const float spaceTime     = p_spaceTime->load(std::memory_order_relaxed);
    const float spaceFeedback = p_spaceFeedback->load(std::memory_order_relaxed);
    const float spaceWear     = p_spaceWear->load(std::memory_order_relaxed);

    // Stage 1 — Hall Reverb → stereo outputs (used as temporary L/R buffers)
    hall_.process(monoIn, L, R, numSamples, verbDecay, verbPredelay);

    // Stage 2 — Unstable Fuzz (mono→stereo split happens here)
    // fuzzGate drives the SchmittTrigger threshold bias (0=fully open, 1=tight gate)
    const float drive = juce::jlimit(0.0f, 1.0f, fuzzGate);
    fuzz_.processStereo(L, R, L, R, numSamples, drive, fuzzStab);

    // Stage 3 — Bit Crusher + Filter (applied to both channels)
    bitCrusher_.processStereo(L, R, numSamples, biscuitBits, biscuitCutoff, biscuitRes, sr_);

    // Stage 4 — Multi-Head Tape Delay
    tapeDelay_.processStereo(L, R, L, R, numSamples, spaceTime, spaceFeedback * 0.01f, spaceWear, sr_);
}

inline void OrisonChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "oris_";

    registerFloatSkewed(layout, p + "verbDecay",    "Verb Decay",    0.5f,  10.0f,  5.0f,  0.01f, 0.4f);
    registerFloatSkewed(layout, p + "verbPredelay", "Verb Predelay", 0.0f,  200.0f, 20.0f, 0.1f,  0.4f);
    registerFloat      (layout, p + "fuzzGate",     "Fuzz Gate",     0.0f,  1.0f,   0.5f,  0.001f);
    registerFloat      (layout, p + "fuzzStab",     "Fuzz Stab",     0.0f,  1.0f,   0.3f,  0.001f);
    registerFloat      (layout, p + "biscuitBits",  "Biscuit Bits",  1.0f,  8.0f,   6.0f,  1.0f);
    registerFloatSkewed(layout, p + "biscuitCutoff","Biscuit Cutoff",200.0f,18000.0f,4000.0f,1.0f, 0.3f);
    registerFloat      (layout, p + "biscuitRes",   "Biscuit Res",   0.0f,  0.95f,  0.5f,  0.001f);
    registerFloatSkewed(layout, p + "spaceTime",    "Space Time",    20.0f, 1200.0f,300.0f, 1.0f,  0.4f);
    registerFloat      (layout, p + "spaceFeedback","Space Feedback",0.0f,  85.0f,  35.0f, 0.1f);
    registerFloat      (layout, p + "spaceWear",    "Space Wear",    0.0f,  1.0f,   0.4f,  0.001f);
}

inline void OrisonChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "oris_";
    p_verbDecay     = cacheParam(apvts, p + "verbDecay");
    p_verbPredelay  = cacheParam(apvts, p + "verbPredelay");
    p_fuzzGate      = cacheParam(apvts, p + "fuzzGate");
    p_fuzzStab      = cacheParam(apvts, p + "fuzzStab");
    p_biscuitBits   = cacheParam(apvts, p + "biscuitBits");
    p_biscuitCutoff = cacheParam(apvts, p + "biscuitCutoff");
    p_biscuitRes    = cacheParam(apvts, p + "biscuitRes");
    p_spaceTime     = cacheParam(apvts, p + "spaceTime");
    p_spaceFeedback = cacheParam(apvts, p + "spaceFeedback");
    p_spaceWear     = cacheParam(apvts, p + "spaceWear");
}

} // namespace xoceanus
