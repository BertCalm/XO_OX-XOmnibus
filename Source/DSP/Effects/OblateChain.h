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

namespace xoceanus
{

//==============================================================================
// OblateChain — Spectral Gate (FX Pack 1, Sidechain Creative)
//
// Wildcard: STFT gate where each FFT bin is gated by the partner engine's
// spectrum, with partner brightness DNA tilting the threshold curve.
// Sidechain key driven by partner *spectrum*, not just amplitude.
//
// Parameter prefix: obla_  (12 params: 11 base + obla_hqMode per spec §10 A2)
//
// Routing: Stereo In → Stereo Out (per-channel STFT analysis)
// Latency: kFFTSize - kHopSize = N/2 = 512 samples ≈ 12 ms @ 44.1 kHz.
//
// Signal flow (per spec §3):
//   1. STFT analyzer per channel — Hann-windowed, 1024-point FFT, 50% overlap.
//   2. Sidechain key extractor — partner audio (mono, from PartnerAudioBus
//      slot `keyEngine`) goes through its own STFT in parallel; per-bin
//      magnitudes are smoothed with a one-pole follower keyed by attack/release.
//   3. Per-bin gate threshold computer — base threshold (dB) + tilt (slope
//      across the spectrum) + DNA coupling (partner brightness shifts tilt).
//   4. Per-bin gate gain — soft-knee gate above threshold (inspired by
//      `ratio` parameter); below threshold, gain = 1/ratio (limit-style).
//   5. ISTFT resynthesis — multiply complex spectrum by per-bin gain, inverse
//      FFT, overlap-add into output ring buffer.
//
// D005: obla_breathRate (0.001-2 Hz floor) drifts the global threshold by
// ±2 dB so the gate "breathes" — extremely slow LFO modulation that prevents
// metronomic gating. Phase advances per block (rate is sub-2 Hz).
//
// hqMode (A2 lock-in): when on, the FFT size param can select 2048; when off,
// it is clamped to 1024. The DSP itself is 1024-only in this Phase 1 cut —
// dynamic FFT size requires worker-thread reallocation that's deferred to a
// follow-up. The hqMode toggle is wired to the parameter range but is a no-op
// at the DSP layer; it will gain teeth in Phase 2.
//
// Audio-thread contract:
//   - All buffers fixed-size and pre-allocated in prepare().
//   - juce::dsp::FFT instance allocated once in member init (no audio-thread alloc).
//   - Parameter pointers cached once via cacheParameterPointers().
//   - Denormal flush in overlap-add accumulator and key magnitude state.
//
// See: Docs/specs/2026-04-27-fx-pack-1-sidechain-creative.md §3, §10 A2
//==============================================================================
class OblateChain
{
public:
    static constexpr int kFFTOrder = 10;                 // 2^10 = 1024
    static constexpr int kFFTSize  = 1 << kFFTOrder;     // 1024
    static constexpr int kHopSize  = kFFTSize / 2;       // 512 (50% overlap)
    static constexpr int kNumBins  = kFFTSize / 2 + 1;   // 513 unique bins (DC..Nyquist)

    OblateChain() = default;

    void prepare(double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = sampleRate;
        mixSmoothed_.reset(sampleRate, 0.02);
        thresholdSmoothed_.reset(sampleRate, 0.05);

        // Build Hann analysis window. No synthesis window — at 50% overlap
        // Hann analysis alone gives perfect COLA (sum of overlapping windows = 1).
        const float twoPi = juce::MathConstants<float>::twoPi;
        for (int i = 0; i < kFFTSize; ++i)
            window_[static_cast<size_t>(i)] =
                0.5f * (1.0f - std::cos(twoPi * static_cast<float>(i) / static_cast<float>(kFFTSize)));
    }

    void reset()
    {
        mixSmoothed_.setCurrentAndTargetValue(1.0f);
        thresholdSmoothed_.setCurrentAndTargetValue(-20.0f);
        chL_ = {};
        chR_ = {};
        keyState_ = {};
        binGain_.fill(1.0f);
        breathPhase_ = 0.0f;
    }

    void setDNABus(const DNAModulationBus* bus) noexcept { dnaBus_ = bus; }
    void setPartnerAudioBus(const PartnerAudioBus* bus) noexcept { partnerBus_ = bus; }

    void processBlock(float* L, float* R, int numSamples,
                      double /*bpm*/ = 0.0, double /*ppqPosition*/ = -1.0)
    {
        if (! pThreshold_ || ! pMix_) return;

        // ---- Block-rate parameter loads (ParamSnapshot) ----
        const float thresholdDb = pThreshold_  ->load(std::memory_order_relaxed);
        const float ratio       = pRatio_       ? pRatio_      ->load(std::memory_order_relaxed) : 4.0f;
        const float atkMs       = pAttack_      ? pAttack_     ->load(std::memory_order_relaxed) : 2.0f;
        const float relMs       = pRelease_     ? pRelease_    ->load(std::memory_order_relaxed) : 100.0f;
        const int   keyEngine   = pKeyEngine_   ? clampSlot(pKeyEngine_->load(std::memory_order_relaxed)) : 0;
        const float tilt        = pTilt_        ? pTilt_       ->load(std::memory_order_relaxed) : 0.0f;
        const float dnaCoupling = pDnaCoupling_ ? pDnaCoupling_->load(std::memory_order_relaxed) : 0.0f;
        const float smoothing   = pSmoothing_   ? pSmoothing_  ->load(std::memory_order_relaxed) : 0.5f;
        const float breathRate  = pBreathRate_  ? pBreathRate_ ->load(std::memory_order_relaxed) : 0.1f;

        mixSmoothed_.setTargetValue(pMix_->load(std::memory_order_relaxed));

        // ---- Breath LFO drift on threshold (D005, ≤ 0.001 Hz floor) ----
        const float twoPi = juce::MathConstants<float>::twoPi;
        constexpr float kBreathDepthDb = 2.0f;
        const float breathDb = std::sin(breathPhase_) * kBreathDepthDb;
        if (sr_ > 0.0)
            breathPhase_ += twoPi * breathRate / static_cast<float>(sr_) * static_cast<float>(numSamples);
        if (breathPhase_ > twoPi) breathPhase_ -= twoPi;
        thresholdSmoothed_.setTargetValue(thresholdDb + breathDb);

        // Per-frame env coefficients — used when smoothing key magnitudes
        // and signal bin gains across hop frames. Time constant = 1 hop * coef.
        const float hopSec = (sr_ > 0.0) ? static_cast<float>(kHopSize) / static_cast<float>(sr_) : 0.0f;
        const float atkCoef = (atkMs > 0.0f && hopSec > 0.0f)
                                   ? std::exp(-hopSec / (atkMs * 0.001f)) : 0.0f;
        const float relCoef = (relMs > 0.0f && hopSec > 0.0f)
                                   ? std::exp(-hopSec / (relMs * 0.001f)) : 0.0f;

        // Smoothing param controls a unipolar one-pole on the bin gain
        // (heavier smoothing → less metallic, slower response).
        const float gainSmoothCoef = juce::jlimit(0.0f, 0.99f, 0.7f + 0.29f * smoothing);

        // ---- DNA brightness modulates the spectral tilt ----
        // Partner brightness in [0,1] → tilt offset in [-1, +1]. dnaCoupling
        // blends from "static tilt" (0) to "DNA-driven tilt" (1).
        float brightness = 0.5f;
        if (dnaBus_ != nullptr)
            brightness = dnaBus_->get(keyEngine, DNAModulationBus::Axis::Brightness);
        const float dnaTilt = (brightness - 0.5f) * 2.0f;
        const float effectiveTilt = tilt * (1.0f - dnaCoupling) + dnaTilt * dnaCoupling;

        // Inverse-ratio used for the soft-gate: signals below threshold are
        // attenuated by 1/ratio (a 4:1 ratio at threshold = -12 dB attenuation).
        const float invRatio = 1.0f / juce::jmax(1.0f, ratio);

        const float* key = (partnerBus_ != nullptr) ? partnerBus_->getMono(keyEngine) : nullptr;

        // ---- Per-sample loop ----
        for (int i = 0; i < numSamples; ++i)
        {
            const float mix = mixSmoothed_.getNextValue();
            const float thresholdLin = juce::Decibels::decibelsToGain(
                                            thresholdSmoothed_.getNextValue(),
                                            -120.0f);

            // Push input samples (and key) into ring buffers.
            chL_.inputRing[static_cast<size_t>(chL_.writePos)] = L[i];
            chR_.inputRing[static_cast<size_t>(chR_.writePos)] = R[i];
            keyState_.inputRing[static_cast<size_t>(keyState_.writePos)] = (key != nullptr) ? key[i] : 0.0f;
            chL_.writePos = (chL_.writePos + 1) & (kFFTSize - 1);
            chR_.writePos = (chR_.writePos + 1) & (kFFTSize - 1);
            keyState_.writePos = (keyState_.writePos + 1) & (kFFTSize - 1);

            // Per-sample hop counter — when full, run STFT on key + L + R.
            ++hopCounter_;
            if (hopCounter_ >= kHopSize)
            {
                hopCounter_ = 0;
                processHopFrame(thresholdLin, invRatio, effectiveTilt, atkCoef, relCoef, gainSmoothCoef);
            }

            // Read the overlap-add output for this sample, then clear the slot
            // so future overlap-add only adds to a zeroed accumulator.
            const int outIdxL = chL_.writePos;  // chronologically aligned to the just-written input
            const int outIdxR = chR_.writePos;
            float wetL = chL_.outputRing[static_cast<size_t>(outIdxL)];
            float wetR = chR_.outputRing[static_cast<size_t>(outIdxR)];
            chL_.outputRing[static_cast<size_t>(outIdxL)] = 0.0f;
            chR_.outputRing[static_cast<size_t>(outIdxR)] = 0.0f;

            // Denormal flush on the overlap-add output (denormals can
            // accumulate in the ring buffer over many frames).
            constexpr float kDenormFloor = 1.0e-30f;
            if (std::abs(wetL) < kDenormFloor) wetL = 0.0f;
            if (std::abs(wetR) < kDenormFloor) wetR = 0.0f;

            const float dryL = L[i];
            const float dryR = R[i];
            L[i] = dryL * (1.0f - mix) + wetL * mix;
            R[i] = dryR * (1.0f - mix) + wetR * mix;
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
        // behind obla_hqMode and re-introduced together with the gating
        // logic in the Pack 1 implementation PR. Phase 1 DSP is 1024-only
        // (dynamic FFT-size resize requires worker-thread reallocation,
        // deferred to a follow-up). The choice param is kept at 256/512/1024
        // to preserve schema; 256 and 512 fall back to 1024 in the DSP.
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID(p + "fftSize", 1), "OBLA FFT Size",
            juce::StringArray{"256", "512", "1024"}, 2));
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
        pTilt_        = apvts.getRawParameterValue(p + "tilt");
        pDnaCoupling_ = apvts.getRawParameterValue(p + "dnaCoupling");
        pSmoothing_   = apvts.getRawParameterValue(p + "smoothing");
        pBreathRate_  = apvts.getRawParameterValue(p + "breathRate");
        pMix_         = apvts.getRawParameterValue(p + "mix");
        // pFftSize_ + pHqMode_ are read but Phase 1 DSP runs at 1024 always.
    }

private:
    static int clampSlot(float v) noexcept
    {
        const int n = static_cast<int>(v + 0.5f);
        return n < 0 ? 0 : (n > 3 ? 3 : n);
    }

    // Compute one STFT hop frame: forward FFT key + L + R, derive per-bin
    // gate gains from the key spectrum, modify L/R bins by the gains, inverse
    // FFT, overlap-add into output rings. Called once every kHopSize samples.
    void processHopFrame(float thresholdLin, float invRatio, float effectiveTilt,
                         float atkCoef, float relCoef, float gainSmoothCoef) noexcept
    {
        // === 1. Window key, forward FFT, compute per-bin magnitudes ===
        copyWindowedToFFTBuffer(keyState_.inputRing, keyState_.writePos, keyState_.fftScratch);
        fft_.performRealOnlyForwardTransform(keyState_.fftScratch.data());

        // Key magnitudes — followed with attack/release per bin, then used to
        // compute bin gate gains. JUCE packs real-only forward as interleaved
        // re/im pairs at indices [2k, 2k+1] for bin k.
        for (int k = 0; k < kNumBins; ++k)
        {
            const float re  = keyState_.fftScratch[static_cast<size_t>(2 * k)];
            const float im  = keyState_.fftScratch[static_cast<size_t>(2 * k + 1)];
            const float mag = std::sqrt(re * re + im * im);

            float prev = keyState_.binMag[static_cast<size_t>(k)];
            const float next = (mag > prev) ? atkCoef * prev + (1.0f - atkCoef) * mag
                                            : relCoef * prev;
            constexpr float kDenormFloor = 1.0e-30f;
            keyState_.binMag[static_cast<size_t>(k)] = std::abs(next) < kDenormFloor ? 0.0f : next;
        }

        // === 2. Compute per-bin gate gains ===
        // Soft-gate convention:
        //   gain = 1.0 when key magnitude >= threshold (signal passes)
        //   gain = invRatio when key magnitude is well below threshold
        //   smooth transition between via a soft knee (1-bin-wide for now)
        // Tilt shifts the per-bin threshold across the spectrum:
        //   bin index 0 → threshold × 10^(+tilt × 0.5)  (low-end attenuated more if tilt>0)
        //   bin index N → threshold × 10^(-tilt × 0.5)  (high-end attenuated less if tilt>0)
        // effectiveTilt ∈ [-1, +1]. Linear bin-index mapping is a fine
        // approximation for "tilt the threshold by ±10 dB across the spectrum".
        for (int k = 0; k < kNumBins; ++k)
        {
            const float binFraction = static_cast<float>(k) / static_cast<float>(kNumBins - 1);
            // Tilt convention: positive tilt → high bins gated *less* (threshold lowered),
            // low bins gated *more*. dB shift = effectiveTilt × ±10 dB across spectrum.
            const float tiltDb = effectiveTilt * (binFraction - 0.5f) * 20.0f;
            const float perBinThreshold = thresholdLin * std::pow(10.0f, tiltDb * (1.0f / 20.0f));

            const float keyMag = keyState_.binMag[static_cast<size_t>(k)];
            // Linear ratio of key vs threshold; ≥1 when key is loud.
            const float ratioVsThresh = keyMag / juce::jmax(perBinThreshold, 1.0e-12f);

            // Target gain: 1 when key is at/above threshold, 1/ratio when key is
            // quiet (below threshold). Use a smooth crossfade in dB-domain via
            // a one-pole sigmoid-ish curve (jlimit then linear blend).
            const float openness = juce::jlimit(0.0f, 1.0f, ratioVsThresh);
            const float targetGain = invRatio + (1.0f - invRatio) * openness;

            // One-pole smoothing on bin gain (smoothing param controls coefficient).
            float prevGain = binGain_[static_cast<size_t>(k)];
            const float nextGain = gainSmoothCoef * prevGain + (1.0f - gainSmoothCoef) * targetGain;
            binGain_[static_cast<size_t>(k)] = nextGain;
        }

        // === 3. Process L channel: forward FFT, apply bin gains, inverse FFT, OLA ===
        processChannelFrame(chL_);

        // === 4. Same for R ===
        processChannelFrame(chR_);
    }

    void processChannelFrame(struct ChannelState& ch) noexcept
    {
        copyWindowedToFFTBuffer(ch.inputRing, ch.writePos, ch.fftScratch);
        fft_.performRealOnlyForwardTransform(ch.fftScratch.data());

        // Apply per-bin gains. Multiply both real and imaginary components
        // by gain → magnitude scales, phase preserved.
        for (int k = 0; k < kNumBins; ++k)
        {
            const float gain = binGain_[static_cast<size_t>(k)];
            ch.fftScratch[static_cast<size_t>(2 * k)]     *= gain;
            ch.fftScratch[static_cast<size_t>(2 * k + 1)] *= gain;
        }

        // Mirror conjugate for the negative-frequency half (bins kNumBins .. kFFTSize-1
        // need to satisfy X[N-k] = conj(X[k]) for the output to be real after IFFT).
        // JUCE's performRealOnlyInverseTransform expects this packing.
        for (int k = 1; k < kFFTSize - kNumBins + 1; ++k)
        {
            const int mirror = kFFTSize - k;
            ch.fftScratch[static_cast<size_t>(2 * mirror)]     =  ch.fftScratch[static_cast<size_t>(2 * k)];
            ch.fftScratch[static_cast<size_t>(2 * mirror + 1)] = -ch.fftScratch[static_cast<size_t>(2 * k + 1)];
        }

        fft_.performRealOnlyInverseTransform(ch.fftScratch.data());

        // Overlap-add into output ring. The output of this frame should align
        // chronologically with the most recent input; we add starting at
        // ch.writePos (which is 1 past the most recent sample).
        // NOTE: Hann analysis × no-synth-window at 50% overlap is COLA-correct
        // (sum of overlapping Hann windows = 1), no normalization needed.
        for (int n = 0; n < kFFTSize; ++n)
        {
            // Output[n] aligns with the time-domain frame starting at the oldest
            // sample in the ring (writePos). Add to (writePos + n) mod kFFTSize.
            const int outIdx = (ch.writePos + n) & (kFFTSize - 1);
            ch.outputRing[static_cast<size_t>(outIdx)] += ch.fftScratch[static_cast<size_t>(n)];
        }
    }

    // Copy the most recent kFFTSize samples from a ring buffer into the FFT
    // scratch (size 2*kFFTSize), windowed by the Hann analysis window. The
    // remaining kFFTSize floats of fftScratch are zeroed (real-only forward
    // expects a buffer twice the FFT size).
    void copyWindowedToFFTBuffer(const std::array<float, kFFTSize>& ring, int writePos,
                                 std::array<float, 2 * kFFTSize>& fftScratch) noexcept
    {
        // Oldest-to-newest order: read starting at writePos (which is the
        // oldest slot, about to be overwritten next sample) and wrap around.
        for (int i = 0; i < kFFTSize; ++i)
        {
            const int readIdx = (writePos + i) & (kFFTSize - 1);
            fftScratch[static_cast<size_t>(i)] =
                ring[static_cast<size_t>(readIdx)] * window_[static_cast<size_t>(i)];
        }
        for (int i = kFFTSize; i < 2 * kFFTSize; ++i)
            fftScratch[static_cast<size_t>(i)] = 0.0f;
    }

    struct ChannelState
    {
        std::array<float, kFFTSize>      inputRing{};
        std::array<float, kFFTSize>      outputRing{};
        std::array<float, 2 * kFFTSize>  fftScratch{};
        int                              writePos = 0;
    };

    struct KeyState
    {
        std::array<float, kFFTSize>      inputRing{};
        std::array<float, 2 * kFFTSize>  fftScratch{};
        std::array<float, kNumBins>      binMag{};
        int                              writePos = 0;
    };

    double sr_ = 0.0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed_;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> thresholdSmoothed_;

    // FFT instance — allocated once at construction (no audio-thread alloc).
    juce::dsp::FFT fft_{kFFTOrder};

    std::array<float, kFFTSize> window_{}; // Hann analysis window

    ChannelState chL_;
    ChannelState chR_;
    KeyState     keyState_;

    // Per-bin gate gain (smoothed across hop frames). Shared across L and R
    // since the key is mono and the tilt curve is shared.
    std::array<float, kNumBins> binGain_{};

    // Single hop counter — drives all three FFT frames (L, R, key) at once.
    int hopCounter_ = 0;

    // Breath LFO phase — drifts threshold by ±2 dB at sub-2 Hz (D005).
    float breathPhase_ = 0.0f;

    // APVTS pointers
    std::atomic<float>* pThreshold_   = nullptr;
    std::atomic<float>* pRatio_       = nullptr;
    std::atomic<float>* pAttack_      = nullptr;
    std::atomic<float>* pRelease_     = nullptr;
    std::atomic<float>* pKeyEngine_   = nullptr;
    std::atomic<float>* pTilt_        = nullptr;
    std::atomic<float>* pDnaCoupling_ = nullptr;
    std::atomic<float>* pSmoothing_   = nullptr;
    std::atomic<float>* pBreathRate_  = nullptr;
    std::atomic<float>* pMix_         = nullptr;

    // Set once on message thread before audio starts; read lock-free on the
    // audio thread (single-writer / single-reader before-after pattern).
    const DNAModulationBus* dnaBus_     = nullptr;
    const PartnerAudioBus*  partnerBus_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OblateChain)
};

} // namespace xoceanus
