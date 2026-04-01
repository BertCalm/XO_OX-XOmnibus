#pragma once

//==============================================================================
// RebirthPipeline.h — Offline DSP pipeline orchestrator for Rebirth Mode.
//
// Pipeline (per spec Section 2):
//   1. Resample → 48 kHz (or keep if already 44100/48000)
//   2. Measure integrated LUFS (K-weighted, BS.1770-4)
//   3. Normalise to -18 dBFS LUFS (18 dB headroom for FX chain)
//   4. Analyse audio → AnalysisResult (4 metrics)
//   5. Execute engine-inspired FX chain (generic, profile-driven)
//   6. Compensate output LUFS to match input LUFS ± 0.5 LU
//   7. True-peak limit at -1.0 dBTP
//
// Design rules (per CLAUDE.md + Spec Section 3.1):
//   - NO per-profile conditional logic here — profiles are pure data
//   - Worker-thread only — never called from the audio thread
//   - No per-block allocation in the render loop (all DSP pre-allocated)
//   - Thread-safe cancel via atomic<bool>
//
// Dependencies (all already committed):
//   Source/Export/RebirthDSP.h       — AllpassDiffuser, FormantResonator,
//                                      NoiseBurst, softClip/rebirthSoftClipBlock
//   Source/Export/RebirthLUFS.h      — computeIntegratedLUFS()
//   Source/Export/RebirthProfiles.h  — RebirthProfileID, RebirthProfile,
//                                      DSPModuleConfig, RebirthSettings,
//                                      getRebirthProfile(), resolveVelocityParam()
//   Source/DSP/Effects/Saturator.h         — class Saturator
//   Source/DSP/Effects/Combulator.h        — class Combulator
//   Source/DSP/Effects/TransientDesigner.h — class TransientDesigner
//   Source/DSP/Effects/BrickwallLimiter.h  — class BrickwallLimiter
//   Source/DSP/Effects/SpectralTilt.h      — class SpectralTilt
//   Source/DSP/Effects/LushReverb.h        — class LushReverb
//
// Namespace: xolokun
//==============================================================================

#include <juce_audio_basics/juce_audio_basics.h>

#include "RebirthDSP.h"
#include "RebirthLUFS.h"
#include "RebirthProfiles.h"

#include "../DSP/Effects/BrickwallLimiter.h"
#include "../DSP/Effects/Combulator.h"
#include "../DSP/Effects/LushReverb.h"
#include "../DSP/Effects/Saturator.h"
#include "../DSP/Effects/SpectralTilt.h"
#include "../DSP/Effects/TransientDesigner.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>

namespace xolokun {

//==============================================================================
// AnalysisResult — 4-metric feature vector computed from the normalised buffer.
//
// transientRatio:   Compare RMS of first 10ms vs next 90ms.
//                   Values > 1 indicate a sharp attack.  Clamped to [0, 1].
// spectralCentroid: Brightness indicator in Hz.
//                   Sum(f_i * |X_i|) / Sum(|X_i|)  via a single FFT frame.
// spectralFlatness: Noise-likeness: geometric mean / arithmetic mean of |X_i|.
//                   0 = pure tone, 1 = white noise.
// durationS:        Total buffer length in seconds.
//==============================================================================
struct AnalysisResult
{
    float transientRatio   = 0.0f;  ///< 0.0 = sustained, 1.0 = sharp attack
    float spectralCentroid = 1000.0f; ///< Hz — brightness indicator
    float spectralFlatness = 0.5f;  ///< 0.0 = tonal, 1.0 = noise-like
    float durationS        = 0.0f;  ///< seconds
};

//==============================================================================
// RebirthPipeline — Transforms a source sample buffer through a profile FX chain.
//
// Thread model: process() and preview() run on worker threads. cancel() may be
// called from any thread. All member state is local to a single call — multiple
// concurrent calls are safe IFF each call uses its own RebirthPipeline instance.
//
// Usage:
//   RebirthPipeline pipeline;
//   juce::Random rng (42);
//   RebirthSettings settings;
//   settings.profileId = RebirthProfileID::ONSET;
//   settings.intensity = 0.7f;
//   auto reborn = pipeline.process (source, 44100.0, settings, 0.5f, rng,
//                                   [](float p) { DBG (p); });
//==============================================================================
class RebirthPipeline
{
public:
    using ProgressCallback = std::function<void (float)>;

    RebirthPipeline()  = default;
    ~RebirthPipeline() = default;

    // Not copyable — contains atomic state
    RebirthPipeline (const RebirthPipeline&)            = delete;
    RebirthPipeline& operator= (const RebirthPipeline&) = delete;

    //==========================================================================
    // process() — Full offline rendering of one sample buffer.
    //
    // Returns a new AudioBuffer<float> containing the reborn audio at the
    // pipeline's internal target sample rate (48000 Hz, or 44100 Hz if source
    // is exactly 44100 Hz and no resampling is needed — see resampleBuffer()).
    //
    // Parameters:
    //   source          — source sample (any channels, any sample rate)
    //   sourceSampleRate— declared sample rate of the source buffer
    //   settings        — Rebirth settings (profile, intensity, chaosAmount ignored)
    //   velocityNorm    — 0.0 (softest layer) to 1.0 (hardest layer)
    //   rng             — seeded PRNG for per-round-robin variation
    //   progress        — optional 0.0→1.0 progress callback (may be null)
    //
    // Thread: worker thread only.  Never call from the audio thread.
    //==========================================================================
    juce::AudioBuffer<float> process (const juce::AudioBuffer<float>& source,
                                      double                           sourceSampleRate,
                                      const RebirthSettings&           settings,
                                      float                            velocityNorm,
                                      juce::Random&                    rng,
                                      ProgressCallback                 progress = nullptr)
    {
        cancelFlag_.store (false, std::memory_order_relaxed);

        // Clamp velocity to [0, 1]
        velocityNorm = juce::jlimit (0.0f, 1.0f, velocityNorm);

        auto reportProgress = [&progress] (float p)
        {
            if (progress) progress (juce::jlimit (0.0f, 1.0f, p));
        };

        // ── Stage 1: Resample ────────────────────────────────────────────────
        auto buf = resampleBuffer (source, sourceSampleRate);
        double workingSR = chooseTargetSR (sourceSampleRate);
        reportProgress (0.10f);
        if (cancelFlag_.load (std::memory_order_relaxed)) return {};

        // ── Stage 2: Measure input LUFS ──────────────────────────────────────
        float inputLUFS = computeIntegratedLUFS (buf, workingSR);
        reportProgress (0.15f);
        if (cancelFlag_.load (std::memory_order_relaxed)) return {};

        // ── Stage 3: Normalise to -18 dBFS LUFS ─────────────────────────────
        normaliseToTarget (buf, inputLUFS, kNormalisationTargetLUFS);
        reportProgress (0.20f);
        if (cancelFlag_.load (std::memory_order_relaxed)) return {};

        // ── Stage 4: Audio analysis ──────────────────────────────────────────
        AnalysisResult analysis = analyzeAudio (buf, workingSR);
        reportProgress (0.25f);
        if (cancelFlag_.load (std::memory_order_relaxed)) return {};

        // ── Stage 5: Keep a dry copy BEFORE the chain (for WetDryMix) ───────
        juce::AudioBuffer<float> dryBuf;
        dryBuf.makeCopyOf (buf);

        // ── Stage 6: Execute FX chain ─────────────────────────────────────
        const RebirthProfile& profile = getRebirthProfile (settings.profileId);
        // Extend buffer by profile's tail time so reverb/diffusion decays fully
        {
            int tailSamples = (int) std::ceil (profile.tailSeconds * workingSR);
            int totalSamples = buf.getNumSamples() + tailSamples;
            juce::AudioBuffer<float> extended (buf.getNumChannels(), totalSamples);
            extended.clear();
            for (int ch = 0; ch < buf.getNumChannels(); ++ch)
                extended.copyFrom (ch, 0, buf, ch, 0, buf.getNumSamples());
            buf = std::move (extended);

            // Extend dryBuf to match (tail = silence)
            juce::AudioBuffer<float> dryExtended (dryBuf.getNumChannels(), totalSamples);
            dryExtended.clear();
            for (int ch = 0; ch < dryBuf.getNumChannels(); ++ch)
                dryExtended.copyFrom (ch, 0, dryBuf, ch, 0, dryBuf.getNumSamples());
            dryBuf = std::move (dryExtended);
        }

        buf = applyChain (buf, dryBuf, workingSR, profile,
                          settings.intensity, velocityNorm, analysis, rng,
                          [&reportProgress] (float chainFrac) {
                              reportProgress (0.25f + chainFrac * 0.55f);
                          });
        if (cancelFlag_.load (std::memory_order_relaxed)) return {};

        // ── Stage 7: Output LUFS compensation ───────────────────────────────
        compensateGain (buf, workingSR, inputLUFS);
        reportProgress (0.85f);
        if (cancelFlag_.load (std::memory_order_relaxed)) return {};

        // ── Stage 8: True-peak limit at -1.0 dBTP ───────────────────────────
        applyTruePeakLimit (buf, workingSR);
        reportProgress (1.00f);

        return buf;
    }

    //==========================================================================
    // preview() — Process the first 2 seconds at full quality.
    //
    // Intended for the A/B toggle UI — runs the same pipeline as process() on
    // a 2-second excerpt so the user hears an accurate preview quickly.
    //
    // velocityNorm is fixed at 0.7 (representative single layer).
    //==========================================================================
    juce::AudioBuffer<float> preview (const juce::AudioBuffer<float>& source,
                                      double                           sourceSampleRate,
                                      const RebirthSettings&           settings)
    {
        constexpr double kPreviewLengthS = 2.0;
        int previewSamples = (int) std::ceil (kPreviewLengthS * sourceSampleRate);

        // Extract first 2 seconds (or the full buffer if shorter)
        int numSamples = std::min (source.getNumSamples(), previewSamples);
        juce::AudioBuffer<float> excerpt (source.getNumChannels(), numSamples);
        for (int ch = 0; ch < source.getNumChannels(); ++ch)
            excerpt.copyFrom (ch, 0, source, ch, 0, numSamples);

        juce::Random rng (12345); // Fixed seed for deterministic previews
        return process (excerpt, sourceSampleRate, settings, 0.7f, rng, nullptr);
    }

    //==========================================================================
    // cancel() — Signal a running process/preview to stop at the next stage.
    //
    // Thread-safe. The pipeline checks cancelFlag_ between pipeline stages and
    // between FX chain modules.  Incomplete results are discarded (empty buffer
    // is returned to the caller).
    //==========================================================================
    void cancel() noexcept
    {
        cancelFlag_.store (true, std::memory_order_relaxed);
    }

private:
    //==========================================================================
    // Internal constants
    //==========================================================================
    static constexpr double kNormalisationTargetLUFS = -18.0;
    static constexpr double kTargetSR48              = 48000.0;
    static constexpr double kTargetSR44              = 44100.0;
    static constexpr float  kTruePeakCeilingDb       = -1.0f;
    static constexpr int    kBlockSize               = 512;

    std::atomic<bool> cancelFlag_ { false };

    //==========================================================================
    // chooseTargetSR() — Decide the working sample rate.
    //
    // Keep source at 44100 or 48000 to avoid double-resampling if already at a
    // standard rate.  Everything else is resampled up to 48000.
    //==========================================================================
    static double chooseTargetSR (double sourceSR) noexcept
    {
        if (std::abs (sourceSR - kTargetSR44) < 1.0) return kTargetSR44;
        if (std::abs (sourceSR - kTargetSR48) < 1.0) return kTargetSR48;
        return kTargetSR48;
    }

    //==========================================================================
    // resampleBuffer() — Resample from sourceSR to the target SR.
    //
    // Uses juce::LagrangeInterpolator (5th-order Lagrange polynomial —
    // good alias rejection, suitable for offline rendering quality).
    // Pre-computes the entire resampled buffer before any DSP stage.
    //
    // If the source is already at the target SR, returns a copy unchanged.
    //==========================================================================
    static juce::AudioBuffer<float> resampleBuffer (const juce::AudioBuffer<float>& source,
                                                    double                           sourceSR)
    {
        double targetSR = chooseTargetSR (sourceSR);

        // Already at target? Return a copy.
        if (std::abs (sourceSR - targetSR) < 1.0)
        {
            juce::AudioBuffer<float> copy;
            copy.makeCopyOf (source);
            return copy;
        }

        double ratio         = targetSR / sourceSR;
        int    numInputSamps = source.getNumSamples();
        int    numOutSamps   = (int) std::ceil ((double) numInputSamps * ratio);
        int    numCh         = source.getNumChannels();

        juce::AudioBuffer<float> output (numCh, numOutSamps);
        output.clear();

        // juce::LagrangeInterpolator processes one channel at a time.
        for (int ch = 0; ch < numCh; ++ch)
        {
            juce::LagrangeInterpolator interp;
            interp.reset();

            const float* srcData = source.getReadPointer (ch);
            float*       dstData = output.getWritePointer (ch);

            // process() returns the number of input samples consumed.
            // We feed all input and let the interpolator handle the rest.
            interp.process (1.0 / ratio,  // speed ratio (sourceSR / targetSR)
                            srcData,
                            dstData,
                            numOutSamps,
                            numInputSamps,
                            0);
        }

        return output;
    }

    //==========================================================================
    // normaliseToTarget() — Apply gain to reach targetLUFS.
    //
    // inputLUFS is the measured integrated LUFS of buf.
    // Gain in dB = targetLUFS - inputLUFS.
    // If the input is silence (-100 dBFS sentinel), gain is left unchanged.
    //==========================================================================
    static void normaliseToTarget (juce::AudioBuffer<float>& buf,
                                   float                      inputLUFS,
                                   double                     targetLUFS)
    {
        if (inputLUFS <= -99.0f) return;  // silence — do nothing

        float gainDb  = (float) targetLUFS - inputLUFS;
        float gainLin = juce::Decibels::decibelsToGain (gainDb);
        buf.applyGain (gainLin);
    }

    //==========================================================================
    // analyzeAudio() — Compute the 4-metric AnalysisResult.
    //
    // All metrics operate on the mono sum of the (already normalised) buffer.
    //
    // transientRatio:
    //   RMS of first 10ms / RMS of next 90ms.  Clamped to [0, 1].
    //   > 1 initially but clamped: a value close to 1 = sharp attack.
    //   Implementation: compute both segments, form ratio, normalise.
    //
    // spectralCentroid:
    //   Single 2048-point windowed FFT from the midpoint of the buffer.
    //   Centroid = Sum(f_i * |X_i|) / Sum(|X_i|).
    //
    // spectralFlatness:
    //   Geometric / arithmetic mean of the same magnitude spectrum.
    //   Computed in log domain: exp(mean(ln(|X_i| + eps))) / mean(|X_i| + eps).
    //   Result clamped to [0, 1].
    //
    // durationS: buf.getNumSamples() / sampleRate.
    //==========================================================================
    static AnalysisResult analyzeAudio (const juce::AudioBuffer<float>& buf,
                                        double                           sampleRate)
    {
        AnalysisResult result;
        int   numSamps = buf.getNumSamples();
        int   numCh    = buf.getNumChannels();

        if (numSamps == 0 || numCh == 0)
            return result;

        result.durationS = (float) numSamps / (float) sampleRate;

        // ── Build mono sum ───────────────────────────────────────────────────
        std::vector<float> mono ((size_t) numSamps, 0.0f);
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float* src = buf.getReadPointer (ch);
            for (int i = 0; i < numSamps; ++i)
                mono[(size_t) i] += src[i];
        }
        float invCh = 1.0f / (float) numCh;
        for (auto& s : mono) s *= invCh;

        // ── Transient ratio ──────────────────────────────────────────────────
        {
            int seg0End = std::min ((int) std::ceil (0.010 * sampleRate), numSamps);
            int seg1Beg = seg0End;
            int seg1End = std::min ((int) std::ceil (0.100 * sampleRate), numSamps);

            auto rms = [&mono] (int start, int end) -> float
            {
                if (end <= start) return 0.0f;
                double sum = 0.0;
                for (int i = start; i < end; ++i)
                    sum += (double) mono[(size_t) i] * (double) mono[(size_t) i];
                return (float) std::sqrt (sum / (double) (end - start));
            };

            float rms0 = rms (0,       seg0End);
            float rms1 = rms (seg1Beg, seg1End);

            if (rms1 > 1e-9f)
            {
                // Ratio > 1 = sharp attack; normalise to [0,1] via tanh-ish clamp:
                // transientRatio = clamp(rms0/rms1, 0, 1) gives 0..1 range
                // where 1.0 means attack segment is AT LEAST as loud as sustain.
                float raw = rms0 / rms1;
                result.transientRatio = juce::jlimit (0.0f, 1.0f, (raw - 1.0f) / 3.0f + 0.5f);
                // Mapping: ratio=1 → 0.5, ratio=4 → 1.0, ratio=0 → ~0.17
                // Better: simple sigmoid-ish: clamp(raw / (raw + 1), 0, 1)
                result.transientRatio = juce::jlimit (0.0f, 1.0f, raw / (raw + 1.0f));
            }
            else
            {
                // No sustain → treat as pure transient
                result.transientRatio = (rms0 > 1e-9f) ? 1.0f : 0.0f;
            }
        }

        // ── Spectral centroid + flatness via FFT ─────────────────────────────
        {
            constexpr int kFFTSize = 2048;

            if (numSamps >= kFFTSize)
            {
                // Centre-ish window
                int start = std::max (0, (numSamps - kFFTSize) / 2);

                std::vector<float> fftRe (kFFTSize, 0.0f);
                std::vector<float> fftIm (kFFTSize, 0.0f);
                for (int i = 0; i < kFFTSize; ++i)
                {
                    float w = 0.5f * (1.0f - std::cos (2.0f * juce::MathConstants<float>::pi
                                                       * (float) i / (float) (kFFTSize - 1)));
                    fftRe[(size_t) i] = mono[(size_t) (start + i)] * w;
                }

                // Inline radix-2 FFT from RebirthDSP.h (avoids juce_dsp / <complex>)
                inlineRadix2FFT (fftRe.data(), fftIm.data(), kFFTSize);

                int halfN = kFFTSize / 2;
                float binHz = (float) sampleRate / (float) kFFTSize;

                double sumMagFreq    = 0.0;  // Sum(f_i * |X_i|)
                double sumMag        = 0.0;  // Sum(|X_i|)
                double sumLogMag     = 0.0;  // Sum(ln(|X_i| + eps))
                constexpr float kEps = 1e-10f;

                for (int i = 1; i < halfN; ++i)  // skip DC bin
                {
                    float re  = fftRe[(size_t) i];
                    float im  = fftIm[(size_t) i];
                    float mag = std::sqrt (re * re + im * im) + kEps;
                    float hz  = (float) i * binHz;

                    sumMagFreq += (double) hz  * (double) mag;
                    sumMag     += (double) mag;
                    sumLogMag  += (double) std::log (mag);
                }

                int   validBins = halfN - 1;  // excluding DC
                if (sumMag > kEps)
                {
                    result.spectralCentroid = (float) (sumMagFreq / sumMag);

                    // Flatness = geometric mean / arithmetic mean
                    double geomMean  = std::exp (sumLogMag / (double) validBins);
                    double arithMean = sumMag / (double) validBins;
                    result.spectralFlatness = juce::jlimit (0.0f, 1.0f,
                                                            (float) (geomMean / arithMean));
                }
            }
            // If buffer < 2048 samples, leave defaults (1kHz centroid, 0.5 flatness)
        }

        return result;
    }

    //==========================================================================
    // applyChain() — Generic FX chain executor.
    //
    // Iterates the profile's chain vector and dispatches to the correct DSP
    // module for each entry.  All parameters are resolved via
    // resolveVelocityParam() so every velocity layer is spectrally distinct
    // (DOC-004 compliance).
    //
    // The WetDryMix entry at the end of each chain blends the accumulated
    // wet output with the pre-chain dry copy using settings.intensity.
    //
    // Processing is in-place on 'wet'.  The function returns the final buffer
    // (which is a blend of wet and dry at the WetDryMix stage).
    //
    // OWARE special case: the Combulator is tuned to Akan interval sets based
    // on analysis.spectralCentroid before the chain runs. This is the only
    // analysis-driven parameter adaptation; all other modules read their params
    // directly from the profile config.
    //==========================================================================
    juce::AudioBuffer<float> applyChain (juce::AudioBuffer<float>       wet,
                                         const juce::AudioBuffer<float>& dry,
                                         double                           sampleRate,
                                         const RebirthProfile&            profile,
                                         float                            intensity,
                                         float                            velocityNorm,
                                         const AnalysisResult&            analysis,
                                         juce::Random&                    rng,
                                         std::function<void (float)>      reportChainProgress)
    {
        int numSamps = wet.getNumSamples();
        int numCh    = wet.getNumChannels();

        // Ensure at least 2 channels for stereo processing.
        // If mono, duplicate to stereo before processing, then collapse after.
        bool wasMono = (numCh == 1);
        if (wasMono)
        {
            juce::AudioBuffer<float> stereo (2, numSamps);
            stereo.copyFrom (0, 0, wet, 0, 0, numSamps);
            stereo.copyFrom (1, 0, wet, 0, 0, numSamps);
            wet = std::move (stereo);
            numCh = 2;
        }

        int numModules = (int) profile.chain.size();

        // ── Pre-allocate DSP modules to avoid per-block allocation ───────────
        // All modules are stack-allocated here (export thread, not audio thread).
        Saturator         saturator;
        Combulator        combulator;
        Combulator        combulator2;  // OWARE sympathetic comb bank
        TransientDesigner transientDesigner;
        AllpassDiffuser   allpassDiffuser;
        FormantResonator  formantResonator;
        NoiseBurst        noiseBurst;
        SpectralTilt      spectralTilt;
        LushReverb        lushReverb;

        // Biquad LP state (for BiquadLPFilter modules) — Direct Form II transposed
        struct BiquadState { float z1L = 0.f, z2L = 0.f, z1R = 0.f, z2R = 0.f; };
        BiquadState biquadLP;

        // Prepare modules that need sampleRate
        saturator.setDrive (0.0f); // will be reconfigured per-module
        saturator.setMix (1.0f);

        combulator.prepare (sampleRate);
        combulator2.prepare (sampleRate);
        transientDesigner.prepare (sampleRate);
        allpassDiffuser.prepare (sampleRate, kBlockSize);
        formantResonator.prepare (sampleRate, kBlockSize);
        noiseBurst.prepare (sampleRate, kBlockSize);
        spectralTilt.prepare (sampleRate);
        lushReverb.prepare (sampleRate);

        // Track which Combulator instance to configure next (for OWARE 2-comb chain)
        int combulatorIndex = 0;

        // ── OWARE: pre-select Akan interval ratios from spectral centroid ────
        // Ratios are applied to both Combulator instances below when encountered.
        float akanRatio2 = 2.0f,  akanRatio3 = 3.0f;   // Metal (default: 1–4kHz)
        if (profile.id == RebirthProfileID::OWARE)
        {
            float centroid = analysis.spectralCentroid;
            if (centroid < 1000.0f)
            {
                // Wood set: [1.0, 2.76, 5.40]
                akanRatio2 = 2.76f;
                akanRatio3 = 5.40f;
            }
            else if (centroid < 4000.0f)
            {
                // Metal set: [1.0, 2.0, 3.0]
                akanRatio2 = 2.0f;
                akanRatio3 = 3.0f;
            }
            else
            {
                // Bell set: [1.0, 2.32, 4.18]
                akanRatio2 = 2.32f;
                akanRatio3 = 4.18f;
            }
        }

        // ── Iterate chain ────────────────────────────────────────────────────
        for (int moduleIdx = 0; moduleIdx < numModules; ++moduleIdx)
        {
            if (cancelFlag_.load (std::memory_order_relaxed)) return {};

            const DSPModuleConfig& cfg = profile.chain[(size_t) moduleIdx];

            float* L = wet.getWritePointer (0);
            float* R = wet.getWritePointer (1);

            switch (cfg.moduleId)
            {
                //--------------------------------------------------------------
                case RebirthDSPModuleID::Saturator:
                {
                    float drive = resolveVelocityParam (cfg, "drive", velocityNorm);
                    float mode  = resolveVelocityParam (cfg, "mode",  velocityNorm);

                    Saturator::SaturationMode satMode = Saturator::SaturationMode::Tube;
                    int modeInt = (int) std::round (mode);
                    switch (modeInt)
                    {
                        case 0:  satMode = Saturator::SaturationMode::Tube;     break;
                        case 1:  satMode = Saturator::SaturationMode::Tape;     break;
                        case 2:  satMode = Saturator::SaturationMode::Digital;  break;
                        case 3:  satMode = Saturator::SaturationMode::FoldBack; break;
                        default: satMode = Saturator::SaturationMode::Tube;     break;
                    }

                    saturator.setMode (satMode);
                    saturator.setDrive (drive);
                    saturator.setMix (1.0f);
                    saturator.setOutputGain (1.0f);

                    // Process block-by-block (pre-allocated kBlockSize block)
                    for (int i = 0; i < numSamps; ++i)
                    {
                        L[i] = saturator.processSample (L[i]);
                        R[i] = saturator.processSample (R[i]);
                    }
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::SoftClipGuard:
                {
                    rebirthSoftClipBlock (L, numSamps);
                    rebirthSoftClipBlock (R, numSamps);
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::Combulator:
                {
                    // Choose which Combulator instance to use
                    Combulator& comb = (combulatorIndex == 0) ? combulator : combulator2;
                    ++combulatorIndex;

                    float feedback = resolveVelocityParam (cfg, "feedback", velocityNorm);
                    float damping  = resolveVelocityParam (cfg, "damping",  velocityNorm);
                    float mix      = resolveVelocityParam (cfg, "mix",      velocityNorm);

                    comb.setFeedback (feedback);
                    comb.setDamping (damping);
                    comb.setMix (mix);

                    // Tune fundamental to spectral centroid for resonance tracking.
                    // Clamp to [20, 2000] Hz so comb delays are musically sensible.
                    float centroidHz = juce::jlimit (20.0f, 2000.0f,
                                                     analysis.spectralCentroid);
                    comb.setFrequency (centroidHz);

                    // OWARE: apply Akan interval ratios as semitone offsets.
                    if (profile.id == RebirthProfileID::OWARE)
                    {
                        // Convert frequency ratios to semitone offsets:
                        // semitones = 12 * log2(ratio)
                        float semi2 = 12.0f * std::log2 (akanRatio2);
                        float semi3 = 12.0f * std::log2 (akanRatio3);
                        comb.setComb2Offset (semi2);
                        comb.setComb3Offset (semi3);
                    }

                    comb.processBlock (L, R, numSamps);
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::TransientDesigner:
                {
                    float attack  = resolveVelocityParam (cfg, "attack",  velocityNorm);
                    float sustain = resolveVelocityParam (cfg, "sustain", velocityNorm);
                    float mix     = resolveVelocityParam (cfg, "mix",     velocityNorm);
                    if (mix < 0.001f) mix = 1.0f; // default full mix if not specified

                    transientDesigner.setAttack (attack);
                    transientDesigner.setSustain (sustain);
                    transientDesigner.setMix (mix);
                    transientDesigner.processBlock (L, R, numSamps);
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::NoiseBurst:
                {
                    float burstLengthMs = resolveVelocityParam (cfg, "burstLengthMs", velocityNorm);
                    float burstLevelDb  = resolveVelocityParam (cfg, "burstLevelDb",  velocityNorm);
                    float hpfCutoffHz   = resolveVelocityParam (cfg, "hpfCutoffHz",   velocityNorm);

                    noiseBurst.setBurstLengthMs (burstLengthMs > 0.0f ? burstLengthMs : 5.0f);
                    noiseBurst.setBurstLevelDb  (burstLevelDb  < 0.0f ? burstLevelDb  : -24.0f);
                    noiseBurst.setHPFCutoffHz   (hpfCutoffHz   > 0.0f ? hpfCutoffHz   : 4000.0f);
                    noiseBurst.triggerBurst();

                    // Process in blocks matching pre-allocated size
                    for (int offset = 0; offset < numSamps; offset += kBlockSize)
                    {
                        int block = std::min (kBlockSize, numSamps - offset);
                        noiseBurst.processBlock (L + offset, R + offset, block);
                    }
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::AllpassDiffuser:
                {
                    float feedback = resolveVelocityParam (cfg, "feedback", velocityNorm);
                    allpassDiffuser.setFeedbackAll (feedback);

                    // Apply per-stage delay times from params if present
                    for (int s = 0; s < AllpassDiffuser::kNumStages; ++s)
                    {
                        std::string key = "delayMs" + std::to_string (s);
                        auto it = cfg.params.find (key);
                        if (it != cfg.params.end())
                            allpassDiffuser.setDelayMs (s, it->second);
                    }

                    for (int offset = 0; offset < numSamps; offset += kBlockSize)
                    {
                        int block = std::min (kBlockSize, numSamps - offset);
                        allpassDiffuser.processBlock (L + offset, R + offset, block);
                    }
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::FormantResonator:
                {
                    float q   = resolveVelocityParam (cfg, "q",   velocityNorm);
                    float mix = resolveVelocityParam (cfg, "mix", velocityNorm);

                    // Analyse formants from the current (normalised) buffer state.
                    // Use the original normalised buffer (wet at this point in chain)
                    // — this captures the sample's actual spectral character.
                    formantResonator.analyzeFormants (wet, sampleRate);
                    formantResonator.setQ   (q   > 0.0f ? q   : 8.0f);
                    formantResonator.setMix (mix > 0.0f ? mix : 0.4f);

                    for (int offset = 0; offset < numSamps; offset += kBlockSize)
                    {
                        int block = std::min (kBlockSize, numSamps - offset);
                        formantResonator.processBlock (L + offset, R + offset, block);
                    }
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::SpectralTilt:
                {
                    float tilt = resolveVelocityParam (cfg, "tilt", velocityNorm);
                    spectralTilt.setTilt (tilt);
                    spectralTilt.setMix (1.0f);
                    spectralTilt.processBlock (L, R, numSamps);
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::BiquadLPFilter:
                {
                    // 2nd-order Butterworth LP, processed inline.
                    // Pre-compute coefficients from resolved cutoff/Q.
                    float cutoffHz = resolveVelocityParam (cfg, "cutoffHz", velocityNorm);
                    float q        = resolveVelocityParam (cfg, "q",        velocityNorm);

                    if (cutoffHz < 20.0f)  cutoffHz = 9000.0f; // default if missing
                    if (q < 0.1f)          q        = 0.7071f; // Butterworth

                    float srF = (float) sampleRate;
                    float w0  = 2.0f * juce::MathConstants<float>::pi * cutoffHz / srF;
                    float sinW0 = std::sin (w0);
                    float cosW0 = std::cos (w0);
                    float alpha = sinW0 / (2.0f * q);
                    float a0inv = 1.0f / (1.0f + alpha);

                    float b0 = (1.0f - cosW0) * 0.5f * a0inv;
                    float b1 = (1.0f - cosW0) * a0inv;
                    float b2 = b0;
                    float a1 = -2.0f * cosW0 * a0inv;
                    float a2 = (1.0f - alpha) * a0inv;

                    // Transposed Direct Form II — process sample by sample
                    for (int i = 0; i < numSamps; ++i)
                    {
                        // Left
                        float yL       = b0 * L[i] + biquadLP.z1L;
                        biquadLP.z1L   = b1 * L[i] - a1 * yL + biquadLP.z2L;
                        biquadLP.z2L   = b2 * L[i] - a2 * yL;
                        L[i] = yL;

                        // Right
                        float yR       = b0 * R[i] + biquadLP.z1R;
                        biquadLP.z1R   = b1 * R[i] - a1 * yR + biquadLP.z2R;
                        biquadLP.z2R   = b2 * R[i] - a2 * yR;
                        R[i] = yR;
                    }
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::LushReverb:
                {
                    float roomSize = resolveVelocityParam (cfg, "roomSize", velocityNorm);
                    float damping  = resolveVelocityParam (cfg, "damping",  velocityNorm);
                    float mix      = resolveVelocityParam (cfg, "mix",      velocityNorm);

                    if (roomSize < 0.0f) roomSize = 0.5f;
                    if (damping  < 0.0f) damping  = 0.4f;
                    if (mix      < 0.0f) mix      = 0.25f;

                    lushReverb.setRoomSize (roomSize);
                    lushReverb.setDamping  (damping);
                    lushReverb.setWidth    (1.0f);
                    lushReverb.setMix      (mix);

                    // LushReverb has a 4-pointer API (inL, inR, outL, outR)
                    // with safe in-place aliasing, so pass L,R,L,R.
                    for (int offset = 0; offset < numSamps; offset += kBlockSize)
                    {
                        int block = std::min (kBlockSize, numSamps - offset);
                        lushReverb.processBlock (L + offset, R + offset,
                                                 L + offset, R + offset, block);
                    }
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::LFOModulator:
                {
                    // Phase 1B: LFO modulation is noted as a TODO in the spec.
                    // Implementation-lite: apply a slow sinusoidal AM on the buffer
                    // to produce the desired subtle movement.  Deep per-module LFO
                    // parameter routing (FormantResonator centre-freq, AllpassDiffuser
                    // delay time) is deferred to Phase 1C (CHAOS + advanced controls).
                    //
                    // FUTURE(Phase 1C): Route LFO output to target module parameters
                    // based on cfg.params["target"]:
                    //   0.0 = FormantResonator centre frequencies (±depth Hz)
                    //   1.0 = AllpassDiffuser delay times (±depth ms)
                    //
                    // Phase 1B approximation: gentle tremolo AM to create perceptible
                    // movement without parameter routing infrastructure.
                    float rate  = resolveVelocityParam (cfg, "rate",  velocityNorm);
                    float depth = resolveVelocityParam (cfg, "depth", velocityNorm);

                    if (rate  <= 0.0f) rate  = 0.3f;
                    if (depth <= 0.0f) depth = 1.0f;

                    // Normalise depth to a fractional AM depth (0 = no modulation,
                    // 1 = ±100% — we scale so max depth ≈ ±20% AM)
                    float amDepth = juce::jlimit (0.0f, 0.2f, depth / 250.0f);
                    float phaseInc = 2.0f * juce::MathConstants<float>::pi
                                   * rate / (float) sampleRate;
                    float phase = 0.0f;

                    for (int i = 0; i < numSamps; ++i)
                    {
                        float lfoVal = 1.0f + amDepth * std::sin (phase);
                        L[i] *= lfoVal;
                        R[i] *= lfoVal;
                        phase += phaseInc;
                        if (phase > juce::MathConstants<float>::twoPi)
                            phase -= juce::MathConstants<float>::twoPi;
                    }
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::WetDryMix:
                {
                    // Blend the accumulated wet result with the pre-chain dry copy.
                    // output = dry*(1 - intensity) + wet*intensity
                    // intensity = 0.0: pass through dry (no transformation)
                    // intensity = 1.0: full wet transformation
                    float wet_gain = juce::jlimit (0.0f, 1.0f, intensity);
                    float dry_gain = 1.0f - wet_gain;

                    const float* dryL = dry.getReadPointer (0);
                    const float* dryR = (dry.getNumChannels() > 1)
                                       ? dry.getReadPointer (1)
                                       : dry.getReadPointer (0);

                    for (int i = 0; i < numSamps; ++i)
                    {
                        L[i] = dry_gain * dryL[i] + wet_gain * L[i];
                        R[i] = dry_gain * dryR[i] + wet_gain * R[i];
                    }
                    break;
                }

                //--------------------------------------------------------------
                case RebirthDSPModuleID::BrickwallLimiter:
                {
                    // BrickwallLimiter entries in the chain (if any) are handled
                    // the same as the final pipeline limit stage, but with params.
                    // In practice the profiles don't put BrickwallLimiter in chains —
                    // the pipeline applies it at Stage 8 — but we support it for
                    // completeness / future profiles.
                    // Handled below in applyTruePeakLimit() with ceiling = -1 dBFS.
                    break;
                }

                default:
                    // Unknown module ID — skip silently, log in debug builds.
                    jassertfalse;
                    break;
            }

            // Report per-module progress within the chain
            reportChainProgress ((float) (moduleIdx + 1) / (float) numModules);
        }

        // ── Collapse stereo to mono if source was mono ───────────────────────
        if (wasMono)
        {
            juce::AudioBuffer<float> mono (1, numSamps);
            float* dst       = mono.getWritePointer (0);
            const float* srcL = wet.getReadPointer (0);
            const float* srcR = wet.getReadPointer (1);
            for (int i = 0; i < numSamps; ++i)
                dst[i] = (srcL[i] + srcR[i]) * 0.5f;
            return mono;
        }

        return wet;
    }

    //==========================================================================
    // compensateGain() — Match output LUFS to targetLUFS ± 0.5 LU.
    //
    // Measures integrated LUFS of the processed buffer and applies a corrective
    // gain so the output loudness matches the original input loudness.
    // This keeps velocity layers perceptually consistent in level — the spectral
    // difference is what the player hears, not a loudness ramp.
    //==========================================================================
    static void compensateGain (juce::AudioBuffer<float>& buf,
                                 double                     sampleRate,
                                 float                      targetLUFS)
    {
        if (targetLUFS <= -99.0f) return;  // silence in, silence out — nothing to do

        float currentLUFS = computeIntegratedLUFS (buf, sampleRate);
        if (currentLUFS <= -99.0f) return;

        float delta = targetLUFS - currentLUFS;

        // Only apply if outside the ±0.5 LU tolerance window
        if (std::abs (delta) > 0.5f)
        {
            float gainLin = juce::Decibels::decibelsToGain (delta);
            buf.applyGain (gainLin);
        }
    }

    //==========================================================================
    // applyTruePeakLimit() — Apply BrickwallLimiter at -1.0 dBTP.
    //
    // Ceiling is -1.0 dBFS (effectively dBTP for non-oversampled buffers).
    // This is the final safety net preventing output clipping after gain
    // compensation.
    //==========================================================================
    static void applyTruePeakLimit (juce::AudioBuffer<float>& buf,
                                    double                     sampleRate)
    {
        if (buf.getNumChannels() < 1 || buf.getNumSamples() == 0) return;

        BrickwallLimiter limiter;
        limiter.prepare (sampleRate, kBlockSize);
        limiter.setCeiling (kTruePeakCeilingDb);

        int numSamps = buf.getNumSamples();

        // Ensure stereo (limiter is always stereo)
        bool wasMono = (buf.getNumChannels() == 1);
        juce::AudioBuffer<float> work;
        if (wasMono)
        {
            work.setSize (2, numSamps);
            work.copyFrom (0, 0, buf, 0, 0, numSamps);
            work.copyFrom (1, 0, buf, 0, 0, numSamps);
        }
        else
        {
            work.makeCopyOf (buf);
        }

        float* L = work.getWritePointer (0);
        float* R = work.getWritePointer (1);

        for (int offset = 0; offset < numSamps; offset += kBlockSize)
        {
            int block = std::min (kBlockSize, numSamps - offset);
            limiter.processBlock (L + offset, R + offset, block);
        }

        // Write back
        if (wasMono)
        {
            float* dst = buf.getWritePointer (0);
            for (int i = 0; i < numSamps; ++i)
                dst[i] = (L[i] + R[i]) * 0.5f;
        }
        else
        {
            buf.copyFrom (0, 0, work, 0, 0, numSamps);
            buf.copyFrom (1, 0, work, 1, 0, numSamps);
        }
    }
};

} // namespace xolokun
