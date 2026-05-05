// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

#include "../../Core/DNAModulationBus.h"
#include "../../Core/PartnerAudioBus.h"
#include "../ParameterSmoother.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

namespace xoceanus
{

//==============================================================================
// OblateChain — Spectral Gate (FX Pack 1, Sidechain Creative)
//
// STFT gate where each FFT bin is gated by the partner engine's spectrum.
// Partner brightness DNA tilts the threshold curve. Sidechain key driven by
// partner *spectrum*, not just amplitude.
//
// Pipeline (5 stages, per spec §3):
//   1. STFT Analyzer (FFT, selectable size 256/512/1024/2048)
//   2. Sidechain Key Extractor — partner spectrum → per-bin key magnitudes
//   3. Per-Band Gate Threshold Computer — DNA-tilted threshold per bin
//   4. Anti-Zip Smoothing — per-bin one-pole, attack/release plus extra pole
//   5. ISTFT Resynthesis (overlap-add, 50% hop, sqrt-Hann analysis+synthesis)
//
// Parameter prefix: obla_  (12 params: 11 base + obla_hqMode)
// Routing: Stereo In → Stereo Out (per-channel STFT analysis)
// Latency: fftSize samples (one frame). Host PDC is not currently propagated
//          by the FX slot controller; advisory only.
//
// Audio-thread safety:
//   - All FFT instances + buffers pre-allocated in prepare().
//   - Partner audio bus and DNA bus pointers set on the message thread before
//     audio starts; read lock-free per block (single-writer/single-reader).
//   - FFT-size switch is detected at block start and flushes overlap state
//     without allocation; juce::dsp::FFT instances exist for all 4 sizes.
//
// See: Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §3, §10 (A2)
//==============================================================================
class OblateChain
{
public:
    static constexpr int kMaxFFTOrder = 11;          // 2^11 = 2048
    static constexpr int kMaxFFTSize  = 1 << kMaxFFTOrder;
    static constexpr int kMaxNumBins  = kMaxFFTSize / 2 + 1;

    OblateChain() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = sampleRate;
        thresholdSmoothed_.reset(sampleRate, 0.02);
        mixSmoothed_.reset(sampleRate, 0.02);

        // Pre-allocate FFT instances at all 4 supported sizes (avoids any
        // audio-thread allocation when the user changes obla_fftSize).
        ffts_[0] = std::make_unique<juce::dsp::FFT>(8);   // 256
        ffts_[1] = std::make_unique<juce::dsp::FFT>(9);   // 512
        ffts_[2] = std::make_unique<juce::dsp::FFT>(10);  // 1024
        ffts_[3] = std::make_unique<juce::dsp::FFT>(11);  // 2048

        buildSqrtHann(windows_[0],  256);
        buildSqrtHann(windows_[1],  512);
        buildSqrtHann(windows_[2], 1024);
        buildSqrtHann(windows_[3], 2048);

        for (auto& ring : inputRing_)  ring.assign(kMaxFFTSize, 0.0f);
        for (auto& ring : outputRing_) ring.assign(kMaxFFTSize, 0.0f);
        for (auto& buf  : workBuf_)    buf.assign(2 * kMaxFFTSize, 0.0f);
        keyRing_.assign(kMaxFFTSize, 0.0f);
        keyWorkBuf_.assign(2 * kMaxFFTSize, 0.0f);
        keyMag_.assign(kMaxNumBins, 0.0f);
        for (auto& g : binGain_) g.assign(kMaxNumBins, 1.0f);

        currentFFTIndex_ = 2; // 1024 default (matches obla_fftSize default)
        resetSTFTState();
    }

    void reset()
    {
        thresholdSmoothed_.setCurrentAndTargetValue(-20.0f);
        mixSmoothed_.setCurrentAndTargetValue(1.0f);
        breathPhase_ = 0.0f;
        resetSTFTState();
    }

    // Inject DNA bus pointer (set once on message thread before audio starts).
    void setDNABus(const DNAModulationBus* bus) noexcept { dnaBus_ = bus; }

    // Inject partner audio bus pointer (set once on message thread before
    // audio starts). Read lock-free per block to pull the sidechain key.
    void setPartnerAudioBus(const PartnerAudioBus* bus) noexcept { partnerBus_ = bus; }

    // Stereo in, stereo out. Per-channel STFT analysis & resynthesis with a
    // shared mono key spectrum for the gate decision.
    void processBlock(float* L, float* R, int numSamples,
                      double /*bpm*/ = 0.0, double /*ppqPosition*/ = -1.0)
    {
        if (! pThreshold_ || ! pMix_) return;

        // ---- Block-rate parameter loads ------------------------------------
        thresholdSmoothed_.setTargetValue(pThreshold_->load(std::memory_order_relaxed));
        mixSmoothed_.setTargetValue(pMix_->load(std::memory_order_relaxed));

        const float ratio        = pRatio_       ? std::max(1.0f, pRatio_->load(std::memory_order_relaxed))      : 4.0f;
        const float attackMs     = pAttack_      ? std::max(0.1f, pAttack_->load(std::memory_order_relaxed))     : 2.0f;
        const float releaseMs    = pRelease_     ? std::max(1.0f, pRelease_->load(std::memory_order_relaxed))    : 100.0f;
        const int   keyEngine    = pKeyEngine_   ? clampSlot(pKeyEngine_->load(std::memory_order_relaxed))       : 0;
        const float tilt         = pTilt_        ? pTilt_->load(std::memory_order_relaxed)                       : 0.0f;
        const float dnaCoupling  = pDnaCoupling_ ? juce::jlimit(0.0f, 1.0f, pDnaCoupling_->load(std::memory_order_relaxed)) : 0.0f;
        const float smoothing    = pSmoothing_   ? juce::jlimit(0.0f, 1.0f, pSmoothing_->load(std::memory_order_relaxed))   : 0.5f;
        const float breathRate   = pBreathRate_  ? std::max(0.001f, pBreathRate_->load(std::memory_order_relaxed)) : 0.1f;
        const bool  hqMode       = pHqMode_      ? pHqMode_->load(std::memory_order_relaxed) >= 0.5f             : false;
        const int   fftChoice    = pFftSize_     ? clampFFTChoice(static_cast<int>(pFftSize_->load(std::memory_order_relaxed) + 0.5f), hqMode) : 2;

        // ---- FFT size change → flush overlap state (no allocation) --------
        if (fftChoice != currentFFTIndex_)
        {
            currentFFTIndex_ = fftChoice;
            resetSTFTState();
        }

        const int fftSize = 1 << (currentFFTIndex_ + 8); // 256 / 512 / 1024 / 2048
        const int hop     = fftSize / 2;
        const int numBins = fftSize / 2 + 1;

        // ---- Hop-rate envelope coefficients --------------------------------
        // Per-bin gain is updated once per hop. Convert ms to per-hop coefs.
        const float hopRate = (sr_ > 0.0) ? static_cast<float>(sr_) / static_cast<float>(hop) : 1.0f;
        const float atkCoef = std::exp(-1.0f / (attackMs  * 0.001f * hopRate));
        const float relCoef = std::exp(-1.0f / (releaseMs * 0.001f * hopRate));
        // Anti-zip smoothing: extra one-pole on the per-bin gain (smoothing
        // param 0..1 maps to coef 0..0.95 → strongest setting kills metallic
        // chatter at the cost of slower gate transients).
        const float zipCoef = 0.95f * smoothing;

        // ---- DNA-tilted threshold ------------------------------------------
        // Partner brightness (0..1) tilts the per-bin tilt curve toward the
        // top end. dnaCoupling=0 disables; dnaCoupling=1 doubles the tilt
        // when partner is bright, halves it when dark.
        float brightness = 0.5f;
        if (dnaBus_)
            brightness = dnaBus_->get(keyEngine, DNAModulationBus::Axis::Brightness);
        const float dnaScale     = 1.0f + dnaCoupling * (brightness - 0.5f) * 2.0f;
        const float effectiveTilt = juce::jlimit(-1.0f, 1.0f, tilt) * dnaScale;

        // Breathing LFO (D005, floor 0.001 Hz) — ±2 dB threshold modulation.
        const float twoPi   = juce::MathConstants<float>::twoPi;
        const float lfoInc  = (sr_ > 0.0) ? twoPi * breathRate / static_cast<float>(sr_) : 0.0f;

        // ---- Resolve partner key audio (nullptr → silent key) -------------
        const float* keyPtr  = partnerBus_ ? partnerBus_->getMono(keyEngine) : nullptr;
        const int    keyLen  = partnerBus_ ? partnerBus_->getNumSamples(keyEngine) : 0;

        // ---- Per-sample loop: ring-buffer push/pull, hop-triggered FFT ----
        for (int i = 0; i < numSamples; ++i)
        {
            const float dryL = L[i];
            const float dryR = R[i];
            const float keyS = (keyPtr && i < keyLen) ? keyPtr[i] : 0.0f;
            const float mix  = mixSmoothed_.getNextValue();
            const float threshDbBase = thresholdSmoothed_.getNextValue();

            // Push input
            inputRing_[0][static_cast<size_t>(writePos_)] = dryL;
            inputRing_[1][static_cast<size_t>(writePos_)] = dryR;
            keyRing_      [static_cast<size_t>(writePos_)] = keyS;

            // Pull output (read-then-clear so future overlap-adds start at 0)
            const float wetL = outputRing_[0][static_cast<size_t>(readPos_)];
            const float wetR = outputRing_[1][static_cast<size_t>(readPos_)];
            outputRing_[0][static_cast<size_t>(readPos_)] = 0.0f;
            outputRing_[1][static_cast<size_t>(readPos_)] = 0.0f;

            // Denormal protection on wet path before mix.
            const float dnSafeL = std::abs(wetL) < 1.0e-30f ? 0.0f : wetL;
            const float dnSafeR = std::abs(wetR) < 1.0e-30f ? 0.0f : wetR;

            L[i] = dryL * (1.0f - mix) + dnSafeL * mix;
            R[i] = dryR * (1.0f - mix) + dnSafeR * mix;

            writePos_ = (writePos_ + 1) % fftSize;
            readPos_  = (readPos_  + 1) % fftSize;
            ++hopCount_;

            // Advance breath LFO once per sample.
            breathPhase_ += lfoInc;
            if (breathPhase_ > twoPi) breathPhase_ -= twoPi;

            if (hopCount_ >= hop)
            {
                hopCount_ = 0;

                // ±2 dB breathing modulation around the user threshold.
                const float breathDb = 2.0f * std::sin(breathPhase_);
                const float threshDb = threshDbBase + breathDb;

                analyzeKey(fftSize, numBins);
                processChannel(0, fftSize, hop, numBins, threshDb,
                               effectiveTilt, ratio, atkCoef, relCoef, zipCoef);
                processChannel(1, fftSize, hop, numBins, threshDb,
                               effectiveTilt, ratio, atkCoef, relCoef, zipCoef);
            }
        }
    }

    //--------------------------------------------------------------------------
    // APVTS integration — 12 parameters per Pack 1 spec §3 + A2 (hqMode).
    static void addParameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                              const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "obla_";

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "threshold", 1), "OBLA Threshold",
            juce::NormalisableRange<float>(-60.0f, 0.0f), -20.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "ratio", 1), "OBLA Ratio",
            juce::NormalisableRange<float>(1.0f, 100.0f, 0.0f, 0.3f), 4.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "attack", 1), "OBLA Attack",
            juce::NormalisableRange<float>(0.1f, 50.0f, 0.0f, 0.5f), 2.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "release", 1), "OBLA Release",
            juce::NormalisableRange<float>(5.0f, 500.0f, 0.0f, 0.5f), 100.0f));
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(p + "keyEngine", 1), "OBLA Key Engine", 0, 3, 0));
        // A2 (locked 2026-04-27): default 1024. The 2048 option is gated
        // behind obla_hqMode; the runtime clamps fftSize to 1024 when hqMode
        // is false even if the parameter index reaches 2048.
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(p + "fftSize", 1), "OBLA FFT Size",
            juce::StringArray{"256", "512", "1024", "2048"}, 2));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "tilt", 1), "OBLA Tilt",
            juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "dnaCoupling", 1), "OBLA DNA Coupling",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "smoothing", 1), "OBLA Smoothing",
            juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
        // D005 floor: 0.001 Hz.
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "breathRate", 1), "OBLA Breath Rate",
            juce::NormalisableRange<float>(0.001f, 2.0f, 0.0f, 0.3f), 0.1f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(p + "mix", 1), "OBLA Mix",
            juce::NormalisableRange<float>(0.0f, 1.0f), 1.0f));
        // A2 (locked 2026-04-27): HQ Mode toggle gates the 2048 FFT option.
        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID(p + "hqMode", 1), "OBLA HQ Mode", false));
    }

    void cacheParameterPointers(juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& slotPrefix = "")
    {
        const juce::String p = slotPrefix + "obla_";
        pThreshold_   = apvts.getRawParameterValue(p + "threshold");
        pRatio_       = apvts.getRawParameterValue(p + "ratio");
        pAttack_      = apvts.getRawParameterValue(p + "attack");
        pRelease_     = apvts.getRawParameterValue(p + "release");
        pKeyEngine_   = apvts.getRawParameterValue(p + "keyEngine");
        pFftSize_     = apvts.getRawParameterValue(p + "fftSize");
        pTilt_        = apvts.getRawParameterValue(p + "tilt");
        pDnaCoupling_ = apvts.getRawParameterValue(p + "dnaCoupling");
        pSmoothing_   = apvts.getRawParameterValue(p + "smoothing");
        pBreathRate_  = apvts.getRawParameterValue(p + "breathRate");
        pMix_         = apvts.getRawParameterValue(p + "mix");
        pHqMode_      = apvts.getRawParameterValue(p + "hqMode");
    }

private:
    static int clampSlot(float v) noexcept
    {
        const int n = static_cast<int>(v + 0.5f);
        return n < 0 ? 0 : (n > 3 ? 3 : n);
    }

    // FFT choice index (0..3) → fftSize 256/512/1024/2048. 2048 (idx 3) is
    // gated behind hqMode; if hqMode is off, falls back to 1024 (idx 2).
    static int clampFFTChoice(int n, bool hqMode) noexcept
    {
        if (n < 0) n = 0;
        if (n > 3) n = 3;
        if (n == 3 && ! hqMode) n = 2;
        return n;
    }

    // Periodic Hann (denominator N, not N-1): sqrt-Hann analysis + sqrt-Hann
    // synthesis = Hann; sum of two Hann frames at hop=N/2 is exactly unity → COLA.
    static void buildSqrtHann(std::vector<float>& w, int N)
    {
        w.assign(static_cast<size_t>(N), 0.0f);
        const double twoPi = juce::MathConstants<double>::twoPi;
        for (int n = 0; n < N; ++n)
        {
            const double hann = 0.5 * (1.0 - std::cos(twoPi * n / N));
            w[static_cast<size_t>(n)] = static_cast<float>(std::sqrt(hann));
        }
    }

    void resetSTFTState() noexcept
    {
        writePos_  = 0;
        readPos_   = 0;
        hopCount_  = 0;
        for (auto& ring : inputRing_)  std::fill(ring.begin(), ring.end(), 0.0f);
        for (auto& ring : outputRing_) std::fill(ring.begin(), ring.end(), 0.0f);
        std::fill(keyRing_.begin(),    keyRing_.end(),    0.0f);
        std::fill(keyMag_.begin(),     keyMag_.end(),     0.0f);
        for (auto& g : binGain_)       std::fill(g.begin(), g.end(), 1.0f);
    }

    // Build the windowed mono key frame, FFT it, and store per-bin magnitudes.
    void analyzeKey(int fftSize, int numBins) noexcept
    {
        const auto& window = windows_[static_cast<size_t>(currentFFTIndex_)];
        // Read fftSize samples ending at writePos (newest sample at index N-1).
        for (int n = 0; n < fftSize; ++n)
        {
            const int idx = (writePos_ - fftSize + n + fftSize) % fftSize;
            keyWorkBuf_[static_cast<size_t>(n)] =
                keyRing_[static_cast<size_t>(idx)] * window[static_cast<size_t>(n)];
        }
        // Zero the imag scratch region (FFT expects 2*N floats).
        std::fill(keyWorkBuf_.begin() + fftSize,
                  keyWorkBuf_.begin() + 2 * fftSize, 0.0f);

        // Hint: we only need non-negative frequencies for the magnitude calc.
        ffts_[static_cast<size_t>(currentFFTIndex_)]
            ->performRealOnlyForwardTransform(keyWorkBuf_.data(), true);

        for (int k = 0; k < numBins; ++k)
        {
            const float re = keyWorkBuf_[static_cast<size_t>(2 * k)];
            const float im = keyWorkBuf_[static_cast<size_t>(2 * k + 1)];
            keyMag_[static_cast<size_t>(k)] = std::sqrt(re * re + im * im);
        }
    }

    // Per-channel STFT cycle: window → FFT → per-bin gate → IFFT → window
    // → overlap-add into outputRing_.
    void processChannel(int ch, int fftSize, int hop, int numBins,
                        float threshDb, float effectiveTilt, float ratio,
                        float atkCoef, float relCoef, float zipCoef) noexcept
    {
        const auto& window = windows_[static_cast<size_t>(currentFFTIndex_)];
        auto& work = workBuf_[static_cast<size_t>(ch)];
        auto& gain = binGain_[static_cast<size_t>(ch)];

        // 1. Window the input frame (ending at writePos).
        for (int n = 0; n < fftSize; ++n)
        {
            const int idx = (writePos_ - fftSize + n + fftSize) % fftSize;
            work[static_cast<size_t>(n)] =
                inputRing_[static_cast<size_t>(ch)][static_cast<size_t>(idx)]
                * window[static_cast<size_t>(n)];
        }
        std::fill(work.begin() + fftSize, work.begin() + 2 * fftSize, 0.0f);

        // 2. Forward FFT. We only modify and read the non-negative bins; the
        //    inverse transform only reads (size/2)+1 complex numbers anyway.
        ffts_[static_cast<size_t>(currentFFTIndex_)]
            ->performRealOnlyForwardTransform(work.data(), true);

        // 3. Per-bin gate, smoothed.
        const float thresholdLin0 = juce::Decibels::decibelsToGain(threshDb);
        const float floorGain     = 1.0f / std::max(1.0f, ratio);

        for (int k = 0; k < numBins; ++k)
        {
            // Per-bin threshold: tilt curves the gate across frequency.
            // binNorm in [0,1]; +tilt closes highs, -tilt opens highs.
            const float binNorm = (numBins > 1)
                                      ? static_cast<float>(k) / static_cast<float>(numBins - 1)
                                      : 0.0f;
            const float tiltDb = effectiveTilt * (binNorm - 0.5f) * 24.0f; // ±12 dB
            const float thrLin = thresholdLin0 * std::pow(10.0f, tiltDb * (1.0f / 20.0f));

            // Sidechain decision: openness = how far key magnitude exceeds threshold.
            const float keyM     = keyMag_[static_cast<size_t>(k)];
            const float openness = juce::jlimit(0.0f, 1.0f,
                                                (keyM - thrLin) / (thrLin + 1.0e-12f));
            const float target   = floorGain + (1.0f - floorGain) * openness;

            // Anti-zip: per-bin one-pole driven by attack/release; obla_smoothing
            // raises the effective coefficient toward 0.99 to kill metallic chatter.
            float& g = gain[static_cast<size_t>(k)];
            const float baseCoef = (target > g) ? atkCoef : relCoef;
            const float coef     = std::max(baseCoef, zipCoef);
            g = coef * g + (1.0f - coef) * target;

            // Apply gain to bin k. The inverse transform only reads bins
            // 0..numBins-1; the conjugate-symmetric upper bins are ignored.
            work[static_cast<size_t>(2 * k)]     *= g;
            work[static_cast<size_t>(2 * k + 1)] *= g;
        }

        // 4. Inverse FFT.
        ffts_[static_cast<size_t>(currentFFTIndex_)]
            ->performRealOnlyInverseTransform(work.data());

        // 5. Synthesis window (sqrt-Hann ✕ sqrt-Hann = Hann; sums to unity at
        //    50% hop, satisfying COLA).
        for (int n = 0; n < fftSize; ++n)
            work[static_cast<size_t>(n)] *= window[static_cast<size_t>(n)];

        // 6. Overlap-add into output ring at readPos.
        auto& out = outputRing_[static_cast<size_t>(ch)];
        for (int n = 0; n < fftSize; ++n)
        {
            const int idx = (readPos_ + n) % fftSize;
            out[static_cast<size_t>(idx)] += work[static_cast<size_t>(n)];
        }

        (void) hop; // hop currently implicit in writePos vs readPos sync
    }

    //--------------------------------------------------------------------------
    double sr_ = 0.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> thresholdSmoothed_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed_;

    // 4 FFT instances (256/512/1024/2048), one window per size — all
    // pre-allocated in prepare() so the audio thread never allocates.
    std::array<std::unique_ptr<juce::dsp::FFT>, 4> ffts_;
    std::array<std::vector<float>, 4>              windows_;
    int currentFFTIndex_ = 2; // 1024 default

    // Stereo ring buffers (one per channel, plus a mono key ring).
    std::array<std::vector<float>, 2> inputRing_{};
    std::array<std::vector<float>, 2> outputRing_{};
    std::vector<float>                keyRing_;

    // FFT scratch (2*N floats: N real input + N imag scratch / freq-domain).
    std::array<std::vector<float>, 2> workBuf_{};
    std::vector<float>                keyWorkBuf_;
    std::vector<float>                keyMag_;

    // Per-bin smoothed gate gain, per channel (anti-zip state).
    std::array<std::vector<float>, 2> binGain_{};

    // Ring positions; advance per sample modulo currentFFTSize.
    int writePos_ = 0;
    int readPos_  = 0;
    int hopCount_ = 0;

    // Breath LFO phase (D005 floor 0.001 Hz). ±2 dB threshold modulation.
    float breathPhase_ = 0.0f;

    // Cached parameter pointers.
    std::atomic<float>* pThreshold_   = nullptr;
    std::atomic<float>* pRatio_       = nullptr;
    std::atomic<float>* pAttack_      = nullptr;
    std::atomic<float>* pRelease_     = nullptr;
    std::atomic<float>* pKeyEngine_   = nullptr;
    std::atomic<float>* pFftSize_     = nullptr;
    std::atomic<float>* pTilt_        = nullptr;
    std::atomic<float>* pDnaCoupling_ = nullptr;
    std::atomic<float>* pSmoothing_   = nullptr;
    std::atomic<float>* pBreathRate_  = nullptr;
    std::atomic<float>* pMix_         = nullptr;
    std::atomic<float>* pHqMode_      = nullptr;

    // Set once via setDNABus()/setPartnerAudioBus() on the message thread
    // before audio starts; read lock-free on the audio thread.
    const DNAModulationBus*  dnaBus_     = nullptr;
    const PartnerAudioBus*   partnerBus_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OblateChain)
};

} // namespace xoceanus
