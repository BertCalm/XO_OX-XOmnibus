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
#include "LushReverb.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OcclusionChain — OCCLUSION Spatiotemporal Collapse FX Chain (5 stages)
//
// Source concept: SpatiotemporalCollapse (Gemini Pedalboard Series)
// Routing: Stereo In → Stereo Out (stereo throughout)
// Accent: Collapsed Indigo #3A0070
//
// Stage 1: Synth Swell / Reverse Pitch (Digitech XP300)
//          EnvelopeFollower → gain + CircularBuffer (1s) backward at 2x speed (+1 oct)
//          Mix with dry
// Stage 2: Wide-Spectrum Fuzz (Death By Audio Fuzz War)
//          Saturator Tube (fastTanh x100), OversamplingProcessor<8>
//          CytomicSVF crossfade tone stack (resonant LP/HP blend)
// Stage 3: Pristine Poly-Echo (Vongon Polyphrase)
//          Dual FractionalDelay cross-feeding (L→R, R→L) up to 3000ms
//          Independent time multipliers per channel
// Stage 4: Decimator / Frequency Shifter (Alesis Bitrman)
//          Zero-order hold + bit quantization + Hilbert Transform freq shift
//          (2x CytomicSVF allpass 90° phase split + PolyBLEP sine/cosine)
// Stage 5: Spatial Micro-Looper (Chase Bliss MOOD)
//          CircularBuffer (2s continuous record), freeze trigger crossfades to
//          looped playback + LushReverb at 100% wet
//
// Parameter prefix: occl_ (14 params)
//==============================================================================
class OcclusionChain
{
public:
    OcclusionChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Process stereo input/output. inL/inR → outL/outR.
    // In-place (inL==outL, inR==outR) is safe.
    void processBlock(const float* inL, const float* inR, float* outL, float* outR,
                      int numSamples, double bpm = 0.0, double ppqPosition = -1.0);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_        = 0.0;  // Sentinel: must be set by prepare() before use
    int    blockSize_ = 512;

    //==========================================================================
    // Stage 1 — Synth Swell / Reverse Pitch (Digitech XP300)
    //==========================================================================
    struct XP300Stage
    {
        CircularBuffer  cbL, cbR;
        EnvelopeFollower envL, envR;
        float            revPosL = 0.0f;
        float            revPosR = 0.0f;
        double           sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int bufSize = static_cast<int>(sampleRate) + 4; // 1s
            cbL.prepare(bufSize);
            cbR.prepare(bufSize);
            envL.prepare(sampleRate);
            envL.setAttack(20.0f);
            envL.setRelease(500.0f);
            envR.prepare(sampleRate);
            envR.setAttack(20.0f);
            envR.setRelease(500.0f);
            revPosL = revPosR = 0.0f;
        }
        void reset()
        {
            cbL.clear(); cbR.clear();
            envL.reset(); envR.reset();
            revPosL = revPosR = 0.0f;
        }
        void process(float inL, float inR, float& outL, float& outR,
                     float swellTime, float reverseMix)
        {
            cbL.write(inL);
            cbR.write(inR);

            // Envelope-driven gain swell
            float envLvlL = envL.process(inL);
            float envLvlR = envR.process(inR);
            float gainL   = std::min(envLvlL * swellTime * 10.0f, 1.0f);
            float gainR   = std::min(envLvlR * swellTime * 10.0f, 1.0f);

            int bufSize = cbL.getSize();
            if (bufSize <= 0) { outL = inL; outR = inR; return; }

            // Read backward at 2x speed (+1 octave)
            revPosL += 2.0f;
            revPosR += 2.0f;
            if (revPosL >= static_cast<float>(bufSize)) revPosL -= static_cast<float>(bufSize);
            if (revPosR >= static_cast<float>(bufSize)) revPosR -= static_cast<float>(bufSize);

            float rvL = cbL.readBackward(static_cast<int>(revPosL));
            float rvR = cbR.readBackward(static_cast<int>(revPosR));

            // Mix: swell gain controls how much reverse enters
            float swellMixL = gainL * reverseMix;
            float swellMixR = gainR * reverseMix;
            outL = inL + swellMixL * (rvL - inL);
            outR = inR + swellMixR * (rvR - inR);
        }
    } xp300_;

    //==========================================================================
    // Stage 2 — Wide-Spectrum Fuzz (Death By Audio Fuzz War)
    //==========================================================================
    struct FuzzWarStage
    {
        OversamplingProcessor<8> ovs;
        CytomicSVF               toneLP, toneHP;

        void prepare(double sampleRate, int maxBlockSize)
        {
            ovs.prepare(sampleRate, maxBlockSize);
            toneLP.reset();
            toneHP.reset();
        }
        void reset()
        {
            ovs.reset();
            toneLP.reset();
            toneHP.reset();
        }
        void processBlock(float* L, float* R, int numSamples,
                          float fuzz, float tone, float vol, float srF)
        {
            // 8x OVS fuzz: extreme tanh(x*100) saturation
            ovs.processStereo(L, R, numSamples, [&](float* upL, float* upR, int upN)
            {
                float driveAmt = 2.0f + fuzz * 98.0f; // 2–100
                for (int i = 0; i < upN; ++i)
                {
                    upL[i] = fastTanh(upL[i] * driveAmt);
                    upR[i] = fastTanh(upR[i] * driveAmt);
                }
            });

            // Tone stack: crossfade resonant LP → HP
            float lpCutoff = 8000.0f - tone * 6000.0f; // 2k–8k
            float hpCutoff = tone * 2000.0f + 40.0f;   // 40–2040 Hz
            toneLP.setMode(CytomicSVF::Mode::LowPass);
            toneLP.setCoefficients(lpCutoff, 0.5f, srF);
            toneHP.setMode(CytomicSVF::Mode::HighPass);
            toneHP.setCoefficients(hpCutoff, 0.5f, srF);

            for (int i = 0; i < numSamples; ++i)
            {
                float lpL = toneLP.processSample(L[i]);
                float hpL = toneHP.processSample(L[i]);
                L[i] = (lpL * (1.0f - tone) + hpL * tone) * vol;

                float lpR = toneLP.processSample(R[i]);
                float hpR = toneHP.processSample(R[i]);
                R[i] = (lpR * (1.0f - tone) + hpR * tone) * vol;
            }
        }
    } fuzzWar_;

    //==========================================================================
    // Stage 3 — Pristine Poly-Echo (Vongon Polyphrase)
    //==========================================================================
    struct PolyphraseStage
    {
        FractionalDelay delayL, delayR;
        float           fbL = 0.0f;
        float           fbR = 0.0f;
        double          sr = 0.0;  // Sentinel: must be set by prepare() before use

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxBuf = static_cast<int>(sampleRate * 3.1) + 8; // 3.1s max
            delayL.prepare(maxBuf);
            delayR.prepare(maxBuf);
            fbL = fbR = 0.0f;
        }
        void reset()
        {
            delayL.clear();
            delayR.clear();
            fbL = fbR = 0.0f;
        }
        void processBlock(float* L, float* R, int numSamples,
                          float timeL_ms, float timeR_ms, float feedback, float srF)
        {
            float sampL = std::max(1.0f, std::min(timeL_ms * srF / 1000.0f,
                                                   static_cast<float>(delayL.getSize()) - 4.0f));
            float sampR = std::max(1.0f, std::min(timeR_ms * srF / 1000.0f,
                                                   static_cast<float>(delayR.getSize()) - 4.0f));

            for (int i = 0; i < numSamples; ++i)
            {
                // Cross-feed: L input + R feedback, R input + L feedback
                delayL.write(L[i] + fbR * feedback);
                delayR.write(R[i] + fbL * feedback);

                float wetL = delayL.read(sampL);
                float wetR = delayR.read(sampR);
                fbL = flushDenormal(wetL);
                fbR = flushDenormal(wetR);

                L[i] = (L[i] + wetL) * 0.5f;
                R[i] = (R[i] + wetR) * 0.5f;
            }
        }
    } polyphrase_;

    //==========================================================================
    // Stage 4 — Decimator / Frequency Shifter (Alesis Bitrman)
    //==========================================================================
    struct BitrmanStage
    {
        // Hilbert transform approximation: 2x CytomicSVF allpass at different phases
        CytomicSVF hilbertAP1L, hilbertAP2L; // L channel 90° pair
        CytomicSVF hilbertAP1R, hilbertAP2R; // R channel 90° pair
        PolyBLEP   shiftOscSine;
        PolyBLEP   shiftOscCosine;
        // Zero-order hold state
        float      zohL      = 0.0f;
        float      zohR      = 0.0f;
        float      zohCounter = 0.0f;

        void prepare(double sampleRate)
        {
            hilbertAP1L.reset(); hilbertAP2L.reset();
            hilbertAP1R.reset(); hilbertAP2R.reset();
            shiftOscSine.setWaveform(PolyBLEP::Waveform::Sine);
            shiftOscCosine.setWaveform(PolyBLEP::Waveform::Sine);
            shiftOscSine.setFrequency(100.0f, static_cast<float>(sampleRate));
            shiftOscCosine.setFrequency(100.0f, static_cast<float>(sampleRate));
            shiftOscCosine.setPhase(0.25f); // 90° offset = cosine
            zohL = zohR = 0.0f;
            zohCounter = 0.0f;
        }
        void reset()
        {
            hilbertAP1L.reset(); hilbertAP2L.reset();
            hilbertAP1R.reset(); hilbertAP2R.reset();
            zohL = zohR = zohCounter = 0.0f;
        }
        void processBlock(float* L, float* R, int numSamples,
                          float decimateRate, float crushBits, float freqShiftHz, float srF)
        {
            // Sample rate reduction: hold samples for zohStep samples
            float zohStep = std::max(1.0f, decimateRate);

            // Bit crushing: quantize to crushBits bits
            float crushLevels = fastPow2(std::max(1.0f, crushBits));

            shiftOscSine.setFrequency(freqShiftHz, srF);
            shiftOscCosine.setFrequency(freqShiftHz, srF);

            // Hilbert allpass pair frequencies (Regalia-Mitra design approx)
            float apFreq1 = srF * 0.1f;
            float apFreq2 = srF * 0.4f;
            hilbertAP1L.setMode(CytomicSVF::Mode::AllPass);
            hilbertAP1L.setCoefficients(apFreq1, 0.7f, srF);
            hilbertAP2L.setMode(CytomicSVF::Mode::AllPass);
            hilbertAP2L.setCoefficients(apFreq2, 0.7f, srF);
            hilbertAP1R.setMode(CytomicSVF::Mode::AllPass);
            hilbertAP1R.setCoefficients(apFreq1, 0.7f, srF);
            hilbertAP2R.setMode(CytomicSVF::Mode::AllPass);
            hilbertAP2R.setCoefficients(apFreq2, 0.7f, srF);

            for (int i = 0; i < numSamples; ++i)
            {
                // Zero-order hold (sample-rate reduction)
                zohCounter += 1.0f;
                if (zohCounter >= zohStep)
                {
                    zohCounter = 0.0f;
                    // Bit crush before hold
                    zohL = std::round(L[i] * crushLevels) / crushLevels;
                    zohR = std::round(R[i] * crushLevels) / crushLevels;
                }
                float heldL = zohL;
                float heldR = zohR;

                // Hilbert transform (analytic signal)
                float hilbL_real = hilbertAP1L.processSample(heldL);
                float hilbL_imag = hilbertAP2L.processSample(heldL);
                float hilbR_real = hilbertAP1R.processSample(heldR);
                float hilbR_imag = hilbertAP2R.processSample(heldR);

                // Frequency shift: multiply by complex exponential e^(j*2π*f*t)
                float sineSamp   = shiftOscSine.processSample();
                float cosineSamp = shiftOscCosine.processSample();

                L[i] = hilbL_real * cosineSamp - hilbL_imag * sineSamp;
                R[i] = hilbR_real * cosineSamp - hilbR_imag * sineSamp;
            }
        }
    } bitrman_;

    //==========================================================================
    // Stage 5 — Spatial Micro-Looper (Chase Bliss MOOD)
    //==========================================================================
    struct MOODStage
    {
        CircularBuffer loopBufL, loopBufR;
        LushReverb     moodVerb;
        float          loopPosL    = 0.0f;
        float          loopPosR    = 0.0f;
        float          freezeXfade = 0.0f;  // 0 = live, 1 = frozen loop
        bool           wasFrozen   = false;
        double         sr          = 0.0;  // Sentinel: must be set by prepare() before use
        std::vector<float> loopL_, loopR_, rvL_, rvR_;

        void prepare(double sampleRate, int maxBlockSize)
        {
            sr = sampleRate;
            int bufSize = static_cast<int>(sampleRate * 2.0) + 4; // 2s
            loopBufL.prepare(bufSize);
            loopBufR.prepare(bufSize);
            moodVerb.prepare(sampleRate);
            loopPosL = loopPosR = 0.0f;
            freezeXfade = 0.0f;
            wasFrozen   = false;
            loopL_.resize(maxBlockSize, 0.0f);
            loopR_.resize(maxBlockSize, 0.0f);
            rvL_.resize(maxBlockSize, 0.0f);
            rvR_.resize(maxBlockSize, 0.0f);
        }
        void reset()
        {
            loopBufL.clear();
            loopBufR.clear();
            moodVerb.reset();
            loopPosL = loopPosR = 0.0f;
            freezeXfade = 0.0f;
            wasFrozen   = false;
            std::fill(loopL_.begin(), loopL_.end(), 0.0f);
            std::fill(loopR_.begin(), loopR_.end(), 0.0f);
            std::fill(rvL_.begin(), rvL_.end(), 0.0f);
            std::fill(rvR_.begin(), rvR_.end(), 0.0f);
        }
        void processBlock(float* L, float* R, int numSamples,
                          float clockRate, bool freeze, float verbSize, float srF)
        {
            // Always record to buffer (continuously)
            for (int i = 0; i < numSamples; ++i)
                loopBufL.write(L[i]);
            for (int i = 0; i < numSamples; ++i)
                loopBufR.write(R[i]);

            // Target crossfade: freeze → 1.0, thaw → 0.0
            float xfadeTarget = freeze ? 1.0f : 0.0f;
            float xfadeCoeff  = fastExp(-1.0f / (0.010f * srF)); // 10ms crossfade

            int bufSize = loopBufL.getSize();

            // Use pre-allocated member buffers (no heap allocation on audio thread)
            float* loopL = loopL_.data();
            float* loopR = loopR_.data();

            // Loop playback: read at clockRate speed.
            // D005: internal clamp matches the addParameters range (0.005–4 Hz)
            // — clamping at 0.1 Hz here would have made the lowered param floor
            // illusory.
            float loopStep = std::max(0.005f, std::min(clockRate, 4.0f));

            for (int i = 0; i < numSamples; ++i)
            {
                freezeXfade = flushDenormal(freezeXfade + (xfadeTarget - freezeXfade) * (1.0f - xfadeCoeff));

                loopPosL += loopStep;
                loopPosR += loopStep;
                if (loopPosL >= static_cast<float>(bufSize)) loopPosL -= static_cast<float>(bufSize);
                if (loopPosR >= static_cast<float>(bufSize)) loopPosR -= static_cast<float>(bufSize);

                loopL[i] = loopBufL.readForward(static_cast<int>(loopPosL));
                loopR[i] = loopBufR.readForward(static_cast<int>(loopPosR));

                // Crossfade live vs frozen loop
                L[i] = L[i] * (1.0f - freezeXfade) + loopL[i] * freezeXfade;
                R[i] = R[i] * (1.0f - freezeXfade) + loopR[i] * freezeXfade;
            }

            // 100% wet reverb on frozen content
            if (freezeXfade > 0.01f)
            {
                moodVerb.setSize(verbSize);
                moodVerb.setDecay(10.0f);
                moodVerb.setDamping(0.2f);
                moodVerb.setDiffusion(0.8f);
                moodVerb.setMix(freezeXfade); // scale verb wet with freeze crossfade

                float* rvL = rvL_.data();
                float* rvR = rvR_.data();
                moodVerb.processBlock(L, R, rvL, rvR, numSamples);
                std::copy(rvL, rvL + numSamples, L);
                std::copy(rvR, rvR + numSamples, R);
            }
        }
    } mood_;

    //==========================================================================
    // Cached parameter pointers (14 params)
    //==========================================================================
    std::atomic<float>* p_xpSwellTime    = nullptr;
    std::atomic<float>* p_xpReverseMix   = nullptr;
    std::atomic<float>* p_fuzzWarFuzz    = nullptr;
    std::atomic<float>* p_fuzzWarTone    = nullptr;
    std::atomic<float>* p_fuzzWarVol     = nullptr;
    std::atomic<float>* p_polyTimeL      = nullptr;
    std::atomic<float>* p_polyTimeR      = nullptr;
    std::atomic<float>* p_polyFeedback   = nullptr;
    std::atomic<float>* p_bitrDecimate   = nullptr;
    std::atomic<float>* p_bitrCrush      = nullptr;
    std::atomic<float>* p_bitrFreqShift  = nullptr;
    std::atomic<float>* p_moodClock      = nullptr;
    std::atomic<float>* p_moodFreeze     = nullptr;
    std::atomic<float>* p_moodVerbSize   = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OcclusionChain::prepare(double sampleRate, int maxBlockSize)
{
    sr_        = sampleRate;
    blockSize_ = maxBlockSize;
    xp300_.prepare(sampleRate);
    fuzzWar_.prepare(sampleRate, maxBlockSize);
    polyphrase_.prepare(sampleRate);
    bitrman_.prepare(sampleRate);
    mood_.prepare(sampleRate, maxBlockSize);
}

inline void OcclusionChain::reset()
{
    xp300_.reset();
    fuzzWar_.reset();
    polyphrase_.reset();
    bitrman_.reset();
    mood_.reset();
}

inline void OcclusionChain::processBlock(const float* inL, const float* inR,
                                          float* outL, float* outR,
                                          int numSamples, double /*bpm*/, double /*ppqPosition*/)
{
    FX_CHAIN_PROCESS_GUARD;
    if (!p_xpSwellTime) return;

    // ParamSnapshot
    const float xpSwellTime   = p_xpSwellTime->load(std::memory_order_relaxed);
    const float xpReverseMix  = p_xpReverseMix->load(std::memory_order_relaxed);
    const float fuzzWarFuzz   = p_fuzzWarFuzz->load(std::memory_order_relaxed);
    const float fuzzWarTone   = p_fuzzWarTone->load(std::memory_order_relaxed);
    const float fuzzWarVol    = p_fuzzWarVol->load(std::memory_order_relaxed);
    const float polyTimeL     = p_polyTimeL->load(std::memory_order_relaxed);
    const float polyTimeR     = p_polyTimeR->load(std::memory_order_relaxed);
    const float polyFeedback  = p_polyFeedback->load(std::memory_order_relaxed) * 0.01f;
    const float bitrDecimate  = p_bitrDecimate->load(std::memory_order_relaxed);
    const float bitrCrush     = p_bitrCrush->load(std::memory_order_relaxed);
    const float bitrFreqShift = p_bitrFreqShift->load(std::memory_order_relaxed);
    const float moodClock     = p_moodClock->load(std::memory_order_relaxed);
    const bool  moodFreeze    = p_moodFreeze->load(std::memory_order_relaxed) >= 0.5f;
    const float moodVerbSize  = p_moodVerbSize->load(std::memory_order_relaxed);

    const float srF = static_cast<float>(sr_);

    // Copy input to output buffers for in-place pipeline
    if (inL != outL) std::copy(inL, inL + numSamples, outL);
    if (inR != outR) std::copy(inR, inR + numSamples, outR);

    // Stage 1: XP300 Synth Swell / Reverse Pitch
    for (int i = 0; i < numSamples; ++i)
    {
        float oL, oR;
        xp300_.process(outL[i], outR[i], oL, oR, xpSwellTime, xpReverseMix);
        outL[i] = oL;
        outR[i] = oR;
    }

    // Stage 2: Fuzz War — stereo block
    fuzzWar_.processBlock(outL, outR, numSamples, fuzzWarFuzz, fuzzWarTone, fuzzWarVol, srF);

    // Stage 3: Polyphrase Poly-Echo — stereo block
    polyphrase_.processBlock(outL, outR, numSamples, polyTimeL, polyTimeR, polyFeedback, srF);

    // Stage 4: Bitrman Decimator / Frequency Shifter — stereo block
    bitrman_.processBlock(outL, outR, numSamples, bitrDecimate, bitrCrush, bitrFreqShift, srF);

    // Stage 5: MOOD Micro-Looper — stereo block
    mood_.processBlock(outL, outR, numSamples, moodClock, moodFreeze, moodVerbSize, srF);
}

inline void OcclusionChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "occl_";

    registerFloatSkewed(layout, p + "xpSwellTime",   p + "XP300 Swell Time",
                        0.01f,   2.0f,  0.3f, 0.001f, 0.4f);
    registerFloat      (layout, p + "xpReverseMix",  p + "XP300 Reverse Mix",
                        0.0f,    1.0f,  0.4f);
    registerFloat      (layout, p + "fuzzWarFuzz",   p + "Fuzz War Fuzz",
                        0.0f,    1.0f,  0.5f);
    registerFloat      (layout, p + "fuzzWarTone",   p + "Fuzz War Tone",
                        0.0f,    1.0f,  0.5f);
    registerFloat      (layout, p + "fuzzWarVol",    p + "Fuzz War Vol",
                        0.0f,    1.0f,  0.7f);
    registerFloatSkewed(layout, p + "polyTimeL",     p + "Poly Echo Time L",
                        10.0f, 3000.0f, 500.0f, 1.0f, 0.3f);
    registerFloatSkewed(layout, p + "polyTimeR",     p + "Poly Echo Time R",
                        10.0f, 3000.0f, 750.0f, 1.0f, 0.3f);
    registerFloat      (layout, p + "polyFeedback",  p + "Poly Echo Feedback",
                        0.0f,   80.0f, 40.0f);
    registerFloat      (layout, p + "bitrDecimate",  p + "Bitrman Decimate",
                        1.0f,   32.0f,  1.0f, 0.1f);
    registerFloat      (layout, p + "bitrCrush",     p + "Bitrman Crush Bits",
                        2.0f,   16.0f, 16.0f, 0.1f);
    registerFloatSkewed(layout, p + "bitrFreqShift", p + "Bitrman Freq Shift",
                        0.0f, 2000.0f, 50.0f,  0.1f, 0.2f);
    // D005 (must breathe): MOOD Clock Rate is the chain's only exposed
    // sub-Hz-capable rate. Floor lowered 0.1 → 0.005 Hz, matching
    // StandardLFO::setRate's internal clamp at Source/DSP/StandardLFO.h:54.
    // Switched to registerFloatSkewed because the new range spans 3+
    // decades. Default 1.0 Hz unchanged.
    registerFloatSkewed(layout, p + "moodClock",     p + "MOOD Clock Rate",
                        0.005f,  4.0f,  1.0f, 0.001f, 0.3f);
    registerBool       (layout, p + "moodFreeze",    p + "MOOD Freeze", false);
    registerFloat      (layout, p + "moodVerbSize",  p + "MOOD Verb Size",
                        0.0f,    1.0f,  0.6f);
}

inline void OcclusionChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "occl_";
    p_xpSwellTime   = cacheParam(apvts, p + "xpSwellTime");
    p_xpReverseMix  = cacheParam(apvts, p + "xpReverseMix");
    p_fuzzWarFuzz   = cacheParam(apvts, p + "fuzzWarFuzz");
    p_fuzzWarTone   = cacheParam(apvts, p + "fuzzWarTone");
    p_fuzzWarVol    = cacheParam(apvts, p + "fuzzWarVol");
    p_polyTimeL     = cacheParam(apvts, p + "polyTimeL");
    p_polyTimeR     = cacheParam(apvts, p + "polyTimeR");
    p_polyFeedback  = cacheParam(apvts, p + "polyFeedback");
    p_bitrDecimate  = cacheParam(apvts, p + "bitrDecimate");
    p_bitrCrush     = cacheParam(apvts, p + "bitrCrush");
    p_bitrFreqShift = cacheParam(apvts, p + "bitrFreqShift");
    p_moodClock     = cacheParam(apvts, p + "moodClock");
    p_moodFreeze    = cacheParam(apvts, p + "moodFreeze");
    p_moodVerbSize  = cacheParam(apvts, p + "moodVerbSize");
}

} // namespace xoceanus
