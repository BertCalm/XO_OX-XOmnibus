// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "../CytomicSVF.h"
#include "../FastMath.h"
#include "Saturator.h"
#include "../StandardLFO.h"
#include "../ParameterSmoother.h"
#include "LushReverb.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <cmath>

namespace xoceanus
{

//==============================================================================
// OmnistereoChain — OMNISTEREO Stereo Widener FX Chain (5 stages)
//
// Source concept: ChromaSpace (Gemini Pedalboard Series)
// Routing: Stereo In → Stereo Out (true stereo throughout)
// Accent: Prismatic Silver #B0C4DE
//
// Stage 1: Tape Saturation & Hysteresis (Strymon Deco V2)
// Stage 2: Precision Parametric EQ (Boss SP-1 Spectrum)
// Stage 3: BBD Vibrato (Boss VB-2)
// Stage 4: BBD Chorus Ensemble (Boss CE-1)
// Stage 5: FDN Reverb (Meris MercuryX) — LushReverb reuse
//
// Parameter prefix: omni_ (16 params)
//==============================================================================
class OmnistereoChain
{
public:
    OmnistereoChain() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void processBlock(float* L, float* R, int numSamples);

    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "");
    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "");

private:
    double sr_ = 44100.0;

    //==========================================================================
    // Stage 1 — Tape Saturation & Hysteresis (Strymon Deco V2)
    //==========================================================================
    struct TapeStage
    {
        Saturator satL, satR;
        float jaStateL = 0.0f, jaStateR = 0.0f;
        // Wow/flutter: StandardLFO S&H → one-pole LP (ParameterSmoother ~2Hz)
        StandardLFO wowLFO_L, wowLFO_R;
        ParameterSmoother wowSmooth_L, wowSmooth_R;
        std::vector<float> wowDelayL, wowDelayR;
        int wowWriteL = 0, wowWriteR = 0;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            satL.prepare(sampleRate); satL.setMode(Saturator::SaturationMode::Tape);
            satR.prepare(sampleRate); satR.setMode(Saturator::SaturationMode::Tape);
            // S&H LFOs for wow drift (slightly different rates for organic feel)
            wowLFO_L.setShape(StandardLFO::Shape::SandH);
            wowLFO_L.setRate(1.3f, static_cast<float>(sampleRate));
            wowLFO_R.setShape(StandardLFO::Shape::SandH);
            wowLFO_R.setRate(0.9f, static_cast<float>(sampleRate));
            // ~2Hz effective LP (500ms time constant)
            wowSmooth_L.prepare(static_cast<float>(sampleRate), 0.5f);
            wowSmooth_R.prepare(static_cast<float>(sampleRate), 0.5f);
            int maxWowSamples = static_cast<int>(0.005 * sampleRate) + 4;
            wowDelayL.assign(maxWowSamples, 0.0f);
            wowDelayR.assign(maxWowSamples, 0.0f);
            reset();
        }
        void reset()
        {
            jaStateL = jaStateR = 0.0f;
            wowWriteL = wowWriteR = 0;
            std::fill(wowDelayL.begin(), wowDelayL.end(), 0.0f);
            std::fill(wowDelayR.begin(), wowDelayR.end(), 0.0f);
            wowSmooth_L.snapTo(0.0f); wowSmooth_R.snapTo(0.0f);
            satL.reset(); satR.reset();
        }

        void processBlock(float* L, float* R, int numSamples,
                          float drive, float bias, float wowDepth)
        {
            float srF = static_cast<float>(sr);
            float maxWowSamplesF = wowDepth * 0.005f * srF;
            int bufSize = static_cast<int>(wowDelayL.size());

            for (int i = 0; i < numSamples; ++i)
            {
                // Wow/flutter: S&H → LP smoothed → fractional delay modulation
                wowSmooth_L.set(wowLFO_L.process());
                wowSmooth_R.set(wowLFO_R.process());
                float smoothWowL = wowSmooth_L.process();
                float smoothWowR = wowSmooth_R.process();

                float delaySamplesL = std::max(0.0f, (smoothWowL * 0.5f + 0.5f) * maxWowSamplesF);
                float delaySamplesR = std::max(0.0f, (smoothWowR * 0.5f + 0.5f) * maxWowSamplesF);

                wowDelayL[wowWriteL] = L[i];
                wowDelayR[wowWriteR] = R[i];

                // Linear interpolation read
                auto readLerp = [&](const std::vector<float>& buf, int wp, float d) -> float {
                    int sz = static_cast<int>(buf.size());
                    float rF = static_cast<float>(wp) - d;
                    while (rF < 0.0f) rF += static_cast<float>(sz);
                    int r0 = static_cast<int>(rF) % sz;
                    int r1 = (r0 + 1) % sz;
                    float fr = rF - static_cast<float>(r0);
                    return buf[r0] * (1.0f - fr) + buf[r1] * fr;
                };
                float wL = readLerp(wowDelayL, wowWriteL, delaySamplesL);
                float wR = readLerp(wowDelayR, wowWriteR, delaySamplesR);
                wowWriteL = (wowWriteL + 1) % bufSize;
                wowWriteR = (wowWriteR + 1) % bufSize;

                // Tape saturation + JA hysteresis (simplified one-pole memory feedback)
                float satInL = wL + jaStateL * 0.1f + bias;
                float satInR = wR + jaStateR * 0.1f + bias;
                satL.setDrive(drive); satR.setDrive(drive);
                float satOutL = satL.processSample(satInL);
                float satOutR = satR.processSample(satInR);
                jaStateL = flushDenormal(jaStateL * 0.99f + (satOutL - wL) * 0.5f);
                jaStateR = flushDenormal(jaStateR * 0.99f + (satOutR - wR) * 0.5f);

                L[i] = satOutL;
                R[i] = satOutR;
            }
        }
    } tape_;

    //==========================================================================
    // Stage 2 — Precision Parametric EQ (Boss SP-1 Spectrum)
    //==========================================================================
    struct ParamEQStage
    {
        CytomicSVF svfL, svfR;
        float lastFreq = 0.0f, lastQ = 0.0f, lastGain = 0.0f;
        double sr = 44100.0;

        void prepare(double sampleRate) { sr = sampleRate; reset(); }
        void reset() { svfL.reset(); svfR.reset(); lastFreq = lastQ = lastGain = 0.0f; }

        void processBlock(float* L, float* R, int numSamples,
                          float freq, float q, float gainDb)
        {
            if (std::abs(gainDb) < 0.01f) return;
            if (freq != lastFreq || q != lastQ || gainDb != lastGain)
            {
                // Map Q to SVF resonance (0-1): resonance = 1 - 1/Q approx
                float res = juce::jlimit(0.0f, 0.99f, 1.0f - (1.0f / std::max(0.1f, q)));
                svfL.setMode(CytomicSVF::Mode::Peak);
                svfL.setCoefficients(freq, res, static_cast<float>(sr), gainDb);
                svfR.setMode(CytomicSVF::Mode::Peak);
                svfR.setCoefficients(freq, res, static_cast<float>(sr), gainDb);
                lastFreq = freq; lastQ = q; lastGain = gainDb;
            }
            for (int i = 0; i < numSamples; ++i)
            {
                L[i] = svfL.processSample(L[i]);
                R[i] = svfR.processSample(R[i]);
            }
        }
    } peq_;

    //==========================================================================
    // Stage 3 — BBD Vibrato (Boss VB-2)
    //==========================================================================
    struct VibratoStage
    {
        static constexpr float kMaxDelayMs = 20.0f;
        std::vector<float> delayL, delayR;
        int writeL = 0, writeR = 0;
        StandardLFO lfo;
        CytomicSVF chebLP1, chebLP2;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 4;
            delayL.assign(maxSamples, 0.0f);
            delayR.assign(maxSamples, 0.0f);
            lfo.setShape(StandardLFO::Shape::Sine);
            reset();
        }
        void reset()
        {
            std::fill(delayL.begin(), delayL.end(), 0.0f);
            std::fill(delayR.begin(), delayR.end(), 0.0f);
            writeL = writeR = 0;
            chebLP1.reset(); chebLP2.reset();
        }

        void processBlock(float* L, float* R, int numSamples,
                          float rateHz, float depthMs, float /*unlatchMs*/)
        {
            lfo.setRate(rateHz, static_cast<float>(sr));
            int bufSize = static_cast<int>(delayL.size());
            float maxDepthSamples = depthMs * static_cast<float>(sr) / 1000.0f;
            float lpCutoff = std::max(1000.0f, 8000.0f - depthMs * 600.0f);
            chebLP1.setMode(CytomicSVF::Mode::LowPass);
            chebLP1.setCoefficients(lpCutoff, 0.8f, static_cast<float>(sr));
            chebLP2.setMode(CytomicSVF::Mode::LowPass);
            chebLP2.setCoefficients(lpCutoff, 0.6f, static_cast<float>(sr));

            for (int i = 0; i < numSamples; ++i)
            {
                float mod = (lfo.process() * 0.5f + 0.5f) * maxDepthSamples;
                mod = std::max(0.0f, std::min(mod, static_cast<float>(bufSize) - 2.0f));

                delayL[writeL] = L[i];
                delayR[writeR] = R[i];

                float rF = static_cast<float>(writeL) - mod;
                while (rF < 0.0f) rF += static_cast<float>(bufSize);
                int r0 = static_cast<int>(rF) % bufSize;
                int r1 = (r0 + 1) % bufSize;
                float fr = rF - static_cast<float>(r0);
                float wetL = delayL[r0] * (1.0f - fr) + delayL[r1] * fr;
                float wetR = delayR[r0] * (1.0f - fr) + delayR[r1] * fr;

                wetL = chebLP1.processSample(wetL);
                wetL = chebLP2.processSample(wetL);
                wetR = chebLP1.processSample(wetR);
                wetR = chebLP2.processSample(wetR);

                writeL = (writeL + 1) % bufSize;
                writeR = (writeR + 1) % bufSize;

                L[i] = wetL;
                R[i] = wetR;
            }
        }
    } vib_;

    //==========================================================================
    // Stage 4 — BBD Chorus Ensemble (Boss CE-1)
    //==========================================================================
    struct ChorusStage
    {
        static constexpr float kMaxDelayMs = 30.0f;
        std::vector<float> delayL, delayR;
        int writeL = 0, writeR = 0;
        StandardLFO lfoL, lfoR;
        float compStateL = 0.0f, compStateR = 0.0f;
        double sr = 44100.0;

        void prepare(double sampleRate)
        {
            sr = sampleRate;
            int maxSamples = static_cast<int>(kMaxDelayMs * sampleRate / 1000.0) + 4;
            delayL.assign(maxSamples, 0.0f);
            delayR.assign(maxSamples, 0.0f);
            lfoL.setShape(StandardLFO::Shape::Sine);
            lfoR.setShape(StandardLFO::Shape::Sine);
            lfoR.setPhaseOffset(0.5f); // 180° offset
            reset();
        }
        void reset()
        {
            std::fill(delayL.begin(), delayL.end(), 0.0f);
            std::fill(delayR.begin(), delayR.end(), 0.0f);
            writeL = writeR = 0;
            compStateL = compStateR = 0.0f;
        }

        void processBlock(float* L, float* R, int numSamples,
                          float rateHz, float depthMs, float compander)
        {
            lfoL.setRate(rateHz, static_cast<float>(sr));
            lfoR.setRate(rateHz, static_cast<float>(sr));
            int bufSize = static_cast<int>(delayL.size());
            float maxDepthSamples = depthMs * static_cast<float>(sr) / 1000.0f;

            float rmsCoeffAtt = fastExp(-1.0f / (0.005f * static_cast<float>(sr)));
            float rmsCoeffRel = fastExp(-1.0f / (0.100f * static_cast<float>(sr)));

            for (int i = 0; i < numSamples; ++i)
            {
                // Compander: compress before delay
                auto updateRMS = [&](float& state, float x) -> float {
                    float e = x * x;
                    float c = e > state ? rmsCoeffAtt : rmsCoeffRel;
                    state = flushDenormal(state * c + e * (1.0f - c));
                    return std::sqrt(std::max(state, 1e-12f));
                };
                float rmsL = updateRMS(compStateL, L[i]);
                float rmsR = updateRMS(compStateR, R[i]);

                float normL = rmsL * compander + (1.0f - compander);
                float normR = rmsR * compander + (1.0f - compander);
                float compIn_L = L[i] / std::max(0.01f, normL);
                float compIn_R = R[i] / std::max(0.01f, normR);

                float modL = (lfoL.process() * 0.5f + 0.5f) * maxDepthSamples;
                float modR = (lfoR.process() * 0.5f + 0.5f) * maxDepthSamples;
                modL = std::min(modL, static_cast<float>(bufSize) - 2.0f);
                modR = std::min(modR, static_cast<float>(bufSize) - 2.0f);

                delayL[writeL] = compIn_L;
                delayR[writeR] = compIn_R;

                auto readLerp = [&](const std::vector<float>& buf, int wp, float d) -> float {
                    int sz = static_cast<int>(buf.size());
                    float rF = static_cast<float>(wp) - d;
                    while (rF < 0.0f) rF += static_cast<float>(sz);
                    int r0 = static_cast<int>(rF) % sz;
                    int r1 = (r0 + 1) % sz;
                    float fr = rF - static_cast<float>(r0);
                    return buf[r0] * (1.0f - fr) + buf[r1] * fr;
                };
                float wetL = readLerp(delayL, writeL, modL);
                float wetR = readLerp(delayR, writeR, modR);

                // Expander: expand after delay
                wetL *= normL;
                wetR *= normR;

                writeL = (writeL + 1) % bufSize;
                writeR = (writeR + 1) % bufSize;

                // 50% dry blend (fixed, per CE-1 spec)
                L[i] = L[i] * 0.5f + wetL * 0.5f;
                R[i] = R[i] * 0.5f + wetR * 0.5f;
            }
        }
    } chorus_;

    //==========================================================================
    // Stage 5 — FDN Reverb (Meris MercuryX) — LushReverb reuse
    //==========================================================================
    LushReverb reverb_;

    //==========================================================================
    // Cached parameter pointers (16 params)
    //==========================================================================
    std::atomic<float>* p_tapeDrive    = nullptr;
    std::atomic<float>* p_tapeBias     = nullptr;
    std::atomic<float>* p_tapeWow      = nullptr;
    std::atomic<float>* p_eqFreq       = nullptr;
    std::atomic<float>* p_eqQ          = nullptr;
    std::atomic<float>* p_eqGain       = nullptr;
    std::atomic<float>* p_vibRate      = nullptr;
    std::atomic<float>* p_vibDepth     = nullptr;
    std::atomic<float>* p_vibUnlatch   = nullptr;
    std::atomic<float>* p_choRate      = nullptr;
    std::atomic<float>* p_choDepth     = nullptr;
    std::atomic<float>* p_choCompander = nullptr;
    std::atomic<float>* p_revSize      = nullptr;
    std::atomic<float>* p_revDamp      = nullptr;
    std::atomic<float>* p_revMod       = nullptr;
    std::atomic<float>* p_revMix       = nullptr;
};

//==============================================================================
// Inline implementations
//==============================================================================

inline void OmnistereoChain::prepare(double sampleRate, int /*maxBlockSize*/)
{
    sr_ = sampleRate;
    tape_.prepare(sampleRate);
    peq_.prepare(sampleRate);
    vib_.prepare(sampleRate);
    chorus_.prepare(sampleRate);
    reverb_.prepare(sampleRate);
}

inline void OmnistereoChain::reset()
{
    tape_.reset(); peq_.reset(); vib_.reset(); chorus_.reset(); reverb_.reset();
}

inline void OmnistereoChain::processBlock(float* L, float* R, int numSamples)
{
    juce::ScopedNoDenormals noDenormals;
    if (!p_tapeDrive) return;

    const float tapeDrive    = p_tapeDrive->load(std::memory_order_relaxed);
    const float tapeBias     = p_tapeBias->load(std::memory_order_relaxed);
    const float tapeWow      = p_tapeWow->load(std::memory_order_relaxed);
    const float eqFreq       = p_eqFreq->load(std::memory_order_relaxed);
    const float eqQ          = p_eqQ->load(std::memory_order_relaxed);
    const float eqGain       = p_eqGain->load(std::memory_order_relaxed);
    const float vibRate      = p_vibRate->load(std::memory_order_relaxed);
    const float vibDepth     = p_vibDepth->load(std::memory_order_relaxed);
    const float vibUnlatch   = p_vibUnlatch->load(std::memory_order_relaxed);
    const float choRate      = p_choRate->load(std::memory_order_relaxed);
    const float choDepth     = p_choDepth->load(std::memory_order_relaxed);
    const float choCompander = p_choCompander->load(std::memory_order_relaxed);
    const float revSize      = p_revSize->load(std::memory_order_relaxed);
    const float revDamp      = p_revDamp->load(std::memory_order_relaxed);
    const float revMod       = p_revMod->load(std::memory_order_relaxed);
    const float revMix       = p_revMix->load(std::memory_order_relaxed);

    tape_.processBlock(L, R, numSamples, tapeDrive, tapeBias, tapeWow * 0.01f);
    peq_.processBlock(L, R, numSamples, eqFreq, eqQ, eqGain);
    vib_.processBlock(L, R, numSamples, vibRate, vibDepth, vibUnlatch);
    chorus_.processBlock(L, R, numSamples, choRate, choDepth, choCompander * 0.01f);

    if (revMix > 0.001f)
    {
        reverb_.setSize(revSize);
        // revDamp is in Hz (500-16000), normalize to 0-1 for LushReverb
        float dampNorm = juce::jlimit(0.0f, 1.0f,
            1.0f - (revDamp - 500.0f) / (16000.0f - 500.0f));
        reverb_.setDamping(dampNorm);
        reverb_.setModulation(revMod * 0.01f);
        reverb_.setMix(revMix * 0.01f);
        reverb_.processBlock(L, R, L, R, numSamples);
    }
}

inline void OmnistereoChain::addParameters(
    juce::AudioProcessorValueTreeState::ParameterLayout& layout,
    const juce::String& slotPrefix)
{
    using AP = juce::AudioParameterFloat;
    using NR = juce::NormalisableRange<float>;
    const juce::String p = slotPrefix + "omni_";

    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tapeDrive",    1}, p + "Tape Drive",
        NR(0.0f, 24.0f, 0.1f), 6.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tapeBias",     1}, p + "Tape Bias",
        NR(-0.5f, 0.5f, 0.001f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "tapeWow",      1}, p + "Tape Wow",
        NR(0.0f, 100.0f, 0.1f), 20.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "eqFreq",       1}, p + "EQ Freq",
        NR(100.0f, 10000.0f, 1.0f), 1000.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "eqQ",          1}, p + "EQ Q",
        NR(0.1f, 10.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "eqGain",       1}, p + "EQ Gain",
        NR(-15.0f, 15.0f, 0.1f), 0.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "vibRate",      1}, p + "Vib Rate",
        NR(0.1f, 10.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "vibDepth",     1}, p + "Vib Depth",
        NR(0.0f, 5.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "vibUnlatch",   1}, p + "Vib Unlatch",
        NR(1.0f, 200.0f, 1.0f), 50.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "choRate",      1}, p + "Cho Rate",
        NR(0.1f, 5.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "choDepth",     1}, p + "Cho Depth",
        NR(0.0f, 15.0f, 0.01f), 5.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "choCompander", 1}, p + "Cho Compander",
        NR(0.0f, 100.0f, 0.1f), 30.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revSize",      1}, p + "Rev Size",
        NR(0.0f, 1.0f, 0.001f), 0.5f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revDamp",      1}, p + "Rev Damp",
        NR(500.0f, 16000.0f, 1.0f), 5000.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revMod",       1}, p + "Rev Mod",
        NR(0.0f, 100.0f, 0.1f), 20.0f));
    layout.add(std::make_unique<AP>(juce::ParameterID{p + "revMix",       1}, p + "Rev Mix",
        NR(0.0f, 100.0f, 0.1f), 0.0f));
}

inline void OmnistereoChain::cacheParameterPointers(
    juce::AudioProcessorValueTreeState& apvts,
    const juce::String& slotPrefix)
{
    const juce::String p = slotPrefix + "omni_";
    p_tapeDrive    = apvts.getRawParameterValue(p + "tapeDrive");
    p_tapeBias     = apvts.getRawParameterValue(p + "tapeBias");
    p_tapeWow      = apvts.getRawParameterValue(p + "tapeWow");
    p_eqFreq       = apvts.getRawParameterValue(p + "eqFreq");
    p_eqQ          = apvts.getRawParameterValue(p + "eqQ");
    p_eqGain       = apvts.getRawParameterValue(p + "eqGain");
    p_vibRate      = apvts.getRawParameterValue(p + "vibRate");
    p_vibDepth     = apvts.getRawParameterValue(p + "vibDepth");
    p_vibUnlatch   = apvts.getRawParameterValue(p + "vibUnlatch");
    p_choRate      = apvts.getRawParameterValue(p + "choRate");
    p_choDepth     = apvts.getRawParameterValue(p + "choDepth");
    p_choCompander = apvts.getRawParameterValue(p + "choCompander");
    p_revSize      = apvts.getRawParameterValue(p + "revSize");
    p_revDamp      = apvts.getRawParameterValue(p + "revDamp");
    p_revMod       = apvts.getRawParameterValue(p + "revMod");
    p_revMix       = apvts.getRawParameterValue(p + "revMix");
}

} // namespace xoceanus
