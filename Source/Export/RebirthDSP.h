#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "../DSP/FastMath.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <vector>

namespace xolokun {

//==============================================================================
// SoftClipGuard — Inline inter-stage headroom limiter.
// Soft-limits signal to ±1.0 using tanh shape. Gain-neutral at low levels
// (tanh(0.7)/tanh(0.7) = 1.0 at the normalization point).
// Insert between any two stages that add gain to prevent hard clipping.
//==============================================================================
inline float softClip (float x) noexcept
{
    constexpr float kDrive = 0.7f;
    constexpr float kNorm  = 0.6043677f; // 1.0f / tanh(0.7f)
    return std::tanh (x * kDrive) * kNorm;
}

inline void softClipBlock (float* data, int numSamples) noexcept
{
    for (int i = 0; i < numSamples; ++i)
        data[i] = softClip (data[i]);
}

//==============================================================================
// AllpassDiffuser — 4-stage cascaded allpass filter for temporal diffusion.
//
// Each stage implements: y[n] = -g*x[n] + x[n-D] + g*y[n-D]
// Delay times are set in milliseconds; feedback per stage is configurable.
// Prime-ish delay ratios (7.1, 11.3, 17.9, 23.7 ms) avoid spectral coloration.
//
// OVERWASH profile uses this for "deep water" diffusion of transient content.
//==============================================================================
class AllpassDiffuser
{
public:
    static constexpr int kNumStages = 4;

    // Default delay times (ms) — prime-ish ratios avoid comb resonance artifacts
    static constexpr float kDefaultDelayMs[kNumStages] = { 7.1f, 11.3f, 17.9f, 23.7f };

    AllpassDiffuser() = default;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = sampleRate;
        for (int s = 0; s < kNumStages; ++s)
        {
            delayMs_[s]   = kDefaultDelayMs[s];
            feedback_[s]  = 0.6f;
        }
        allocateBuffers();
        reset();
    }

    void setDelayMs (int stage, float ms)
    {
        jassert (stage >= 0 && stage < kNumStages);
        delayMs_[stage] = juce::jlimit (0.5f, 500.0f, ms);
        allocateBuffers();
    }

    void setFeedback (int stage, float g)
    {
        jassert (stage >= 0 && stage < kNumStages);
        feedback_[stage] = juce::jlimit (-0.99f, 0.99f, g);
    }

    // Set all-stage feedback uniformly (convenience for profile parameter application)
    void setFeedbackAll (float g)
    {
        for (int s = 0; s < kNumStages; ++s)
            feedback_[s] = juce::jlimit (-0.99f, 0.99f, g);
    }

    void reset()
    {
        for (int s = 0; s < kNumStages; ++s)
        {
            if (!bufL_[s].empty()) std::fill (bufL_[s].begin(), bufL_[s].end(), 0.0f);
            if (!bufR_[s].empty()) std::fill (bufR_[s].begin(), bufR_[s].end(), 0.0f);
            posL_[s] = posR_[s] = 0;
        }
    }

    // Process a stereo block in-place.
    void processBlock (float* left, float* right, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float xL = left[i];
            float xR = right[i];

            for (int s = 0; s < kNumStages; ++s)
            {
                int delaySamps = delaySamples_[s];
                auto& bL = bufL_[s];
                auto& bR = bufR_[s];
                int& pL = posL_[s];
                int& pR = posR_[s];
                float g = feedback_[s];

                // Allpass: y[n] = -g*x[n] + x[n-D] + g*y[n-D]
                // The delay line stores y[n], so x[n-D] is retrieved from D samples ago
                // and g*y[n-D] is already embedded in the delay line content.
                // Schroeder allpass implementation:
                //   delayed = buf[pos]         (this is x[n-D] in the input delay view,
                //                               but we store the mixed signal)
                // Standard implementation: store input, read delayed, compute output, write output.
                // Use dual-buffer: inputBuf for x[n-D], outputBuf for y[n-D].
                // For simplicity and correctness use the one-buffer form:
                //   v     = x - g * buf[pos]
                //   y     = buf[pos] + g * v
                //   buf[pos] = v
                float vL = xL - g * bL[pL];
                float yL = bL[pL] + g * vL;
                bL[pL]   = vL;
                pL = (pL + 1) % delaySamps;

                float vR = xR - g * bR[pR];
                float yR = bR[pR] + g * vR;
                bR[pR]   = vR;
                pR = (pR + 1) % delaySamps;

                // Denormal protection
                yL += 1e-25f; yL -= 1e-25f;
                yR += 1e-25f; yR -= 1e-25f;

                xL = yL;
                xR = yR;
            }

            left[i]  = xL;
            right[i] = xR;
        }
    }

private:
    void allocateBuffers()
    {
        for (int s = 0; s < kNumStages; ++s)
        {
            int samps = juce::jmax (1, (int) std::ceil (delayMs_[s] * 0.001 * sr_)) + 2;
            delaySamples_[s] = samps;
            bufL_[s].assign ((size_t) samps, 0.0f);
            bufR_[s].assign ((size_t) samps, 0.0f);
        }
    }

    double sr_ = 44100.0;
    float  delayMs_[kNumStages]     = { 7.1f, 11.3f, 17.9f, 23.7f };
    float  feedback_[kNumStages]    = { 0.6f, 0.6f, 0.6f, 0.6f };
    int    delaySamples_[kNumStages] = { 0, 0, 0, 0 };

    std::vector<float> bufL_[kNumStages];
    std::vector<float> bufR_[kNumStages];
    int posL_[kNumStages] = {};
    int posR_[kNumStages] = {};
};

//==============================================================================
// NoiseBurst — Short filtered white-noise transient injector.
//
// Generates a brief burst of HP-filtered white noise mixed into the signal.
// Fired once at block start if triggerBurst() was called. The burst length,
// HP filter cutoff, and level are all configurable.
//
// ONSET profile uses this to add "crack" to the attack of percussive material.
//==============================================================================
class NoiseBurst
{
public:
    NoiseBurst() = default;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr_          = sampleRate;
        burstPhase_  = 0;
        burstActive_ = false;
        updateHPF();
        reset();
    }

    /// Length of the noise burst in milliseconds.
    void setBurstLengthMs (float ms)
    {
        burstLengthMs_ = juce::jlimit (0.5f, 200.0f, ms);
    }

    /// Level of the burst in dB (negative = attenuated).
    void setBurstLevelDb (float db)
    {
        burstLevel_ = juce::Decibels::decibelsToGain (juce::jlimit (-60.0f, 0.0f, db));
    }

    /// High-pass cutoff frequency for the noise burst.
    void setHPFCutoffHz (float hz)
    {
        hpfCutoffHz_ = juce::jlimit (20.0f, 20000.0f, hz);
        updateHPF();
    }

    /// Call once to schedule a burst that will be injected on the next processBlock() call.
    void triggerBurst() noexcept { burstPhase_ = 0; burstActive_ = true; }

    void reset()
    {
        burstPhase_  = 0;
        burstActive_ = false;
        hpfZ1L_ = hpfZ1R_ = 0.0f;
    }

    // Process stereo block in-place. Burst is mixed additively into the signal.
    void processBlock (float* left, float* right, int numSamples)
    {
        if (!burstActive_) return;

        int burstLenSamps = (int) std::ceil (burstLengthMs_ * 0.001 * sr_);

        for (int i = 0; i < numSamples && burstActive_; ++i)
        {
            if (burstPhase_ >= burstLenSamps)
            {
                burstActive_ = false;
                break;
            }

            // White noise via LCG
            rngState_ = rngState_ * 6364136223846793005ULL + 1442695040888963407ULL;
            float noise = (float) (int32_t) (rngState_ >> 33) * (1.0f / 1073741824.0f);

            // 1-pole HP filter: y[n] = x[n] - x[n-1] + coeff * y[n-1]
            float hpL = noise - hpfZ1L_ + hpfCoeff_ * hpfZ1L_;
            hpfZ1L_  = noise;
            float hpR = noise - hpfZ1R_ + hpfCoeff_ * hpfZ1R_;
            hpfZ1R_  = noise;

            // Amplitude envelope: linear decay over burst length
            float env = 1.0f - (float) burstPhase_ / (float) burstLenSamps;

            left[i]  += hpL * env * burstLevel_;
            right[i] += hpR * env * burstLevel_;

            ++burstPhase_;
        }
    }

private:
    void updateHPF()
    {
        // 1-pole matched-Z HP coefficient
        hpfCoeff_ = std::exp (-2.0f * juce::MathConstants<float>::pi * hpfCutoffHz_ / (float) sr_);
    }

    double   sr_            = 44100.0;
    float    burstLengthMs_ = 5.0f;
    float    burstLevel_    = juce::Decibels::decibelsToGain (-24.0f);
    float    hpfCutoffHz_   = 4000.0f;
    float    hpfCoeff_      = 0.0f;
    float    hpfZ1L_        = 0.0f;
    float    hpfZ1R_        = 0.0f;
    uint64_t rngState_      = 0xDEADBEEFCAFEBABEULL;

    int  burstPhase_  = 0;
    bool burstActive_ = false;
};

//==============================================================================
// FormantResonator — FFT formant estimator + parallel bandpass resonator bank.
//
// Step 1: Analyze the entire source buffer with a windowed 2048-point FFT.
//         Pick the top 4 spectral peaks as formant center frequencies.
// Step 2: Build 4 parallel second-order bandpass filters (biquads) tuned to
//         those frequencies with configurable Q.
// Step 3: processBlock() applies the resonator bank: dry signal passes through
//         each resonator in parallel, outputs are summed and blended with dry.
//
// Implementation note: NOT LPC. LPC is too expensive for <3s preview target.
// Windowed FFT peak picking is sufficient for perceptual formant capture.
//
// OPERA profile uses this to give samples vocal/choral shimmer quality.
//==============================================================================
class FormantResonator
{
public:
    static constexpr int kNumFormants = 4;
    static constexpr int kFFTSize     = 2048;

    FormantResonator() = default;

    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr_  = sampleRate;
        q_   = 8.0f;
        mix_ = 0.4f;
        // Default formant frequencies (vowel "ah" approximation)
        formantHz_[0] = 800.0f;
        formantHz_[1] = 1200.0f;
        formantHz_[2] = 2500.0f;
        formantHz_[3] = 3500.0f;
        rebuildFilters();
        reset();
    }

    /// Q of all resonators (4.0 = wide/warm, 16.0 = narrow/bright).
    void setQ (float q)
    {
        q_ = juce::jlimit (0.5f, 50.0f, q);
        rebuildFilters();
    }

    /// Mix of resonator bank (0 = dry, 1 = full resonator output).
    void setMix (float m) { mix_ = juce::jlimit (0.0f, 1.0f, m); }

    /// Analyze a buffer to extract formant frequencies.
    /// Call this once before processBlock(). Uses the mono sum of the buffer.
    void analyzeFormants (const juce::AudioBuffer<float>& buf, double /*sr*/)
    {
        int totalSamps = buf.getNumSamples();
        int numCh      = buf.getNumChannels();
        if (totalSamps < kFFTSize) return;

        // Build mono sum into FFT input (use first kFFTSize samples from center-ish)
        int startSamp = std::max (0, (totalSamps - kFFTSize) / 2);
        std::vector<float> window (kFFTSize, 0.0f);
        for (int i = 0; i < kFFTSize; ++i)
        {
            float s = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                s += buf.getSample (ch, startSamp + i);
            s /= (float) numCh;
            // Hann window
            float w = 0.5f * (1.0f - std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (kFFTSize - 1)));
            window[i] = s * w;
        }

        // Real FFT using juce::dsp::FFT
        // juce::dsp::FFT requires a buffer of 2*N floats (interleaved real/imag)
        std::vector<float> fftBuf (kFFTSize * 2, 0.0f);
        for (int i = 0; i < kFFTSize; ++i) fftBuf[i * 2] = window[i];

        juce::dsp::FFT fft (11); // 2^11 = 2048
        fft.performRealOnlyForwardTransform (fftBuf.data());

        // Compute magnitude spectrum (first N/2 bins)
        int halfN = kFFTSize / 2;
        std::vector<float> mag (halfN, 0.0f);
        for (int i = 0; i < halfN; ++i)
        {
            float re = fftBuf[i * 2];
            float im = fftBuf[i * 2 + 1];
            mag[i]   = std::sqrt (re * re + im * im);
        }

        // Simple peak picking: find top kNumFormants local maxima.
        // Min distance between peaks: 100 Hz in bin space.
        float binHz     = (float) sr_ / (float) kFFTSize;
        int   minBinGap = std::max (1, (int) (100.0f / binHz));

        // Ignore DC and sub-100Hz bins
        int minBin = std::max (1, (int) (100.0f / binHz));
        int maxBin = halfN - 1;

        struct Peak { int bin; float mag; };
        std::vector<Peak> peaks;

        for (int i = minBin + 1; i < maxBin - 1; ++i)
        {
            if (mag[i] > mag[i - 1] && mag[i] > mag[i + 1])
                peaks.push_back ({ i, mag[i] });
        }

        // Sort by magnitude descending
        std::sort (peaks.begin(), peaks.end(),
                   [] (const Peak& a, const Peak& b) { return a.mag > b.mag; });

        // Select top kNumFormants peaks with minimum bin gap enforcement
        std::vector<int> selectedBins;
        for (const auto& p : peaks)
        {
            bool tooClose = false;
            for (int sel : selectedBins)
                if (std::abs (p.bin - sel) < minBinGap) { tooClose = true; break; }
            if (!tooClose)
            {
                selectedBins.push_back (p.bin);
                if ((int) selectedBins.size() >= kNumFormants) break;
            }
        }

        // Pad with default frequencies if fewer than kNumFormants peaks found
        static const float kDefaults[kNumFormants] = { 800.0f, 1200.0f, 2500.0f, 3500.0f };
        for (int i = 0; i < kNumFormants; ++i)
        {
            if (i < (int) selectedBins.size())
                formantHz_[i] = (float) selectedBins[i] * binHz;
            else
                formantHz_[i] = kDefaults[i];
        }

        // Sort formants ascending (perceptual convention)
        std::sort (formantHz_, formantHz_ + kNumFormants);
        rebuildFilters();
    }

    void reset()
    {
        for (int f = 0; f < kNumFormants; ++f)
            for (int ch = 0; ch < 2; ++ch)
                z1_[f][ch] = z2_[f][ch] = 0.0f;
    }

    // Process stereo block in-place. Each sample is fed through the parallel
    // resonator bank; outputs are summed and blended with the dry signal.
    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix_ < 0.001f) return;

        for (int i = 0; i < numSamples; ++i)
        {
            float dryL = left[i];
            float dryR = right[i];
            float wetL = 0.0f;
            float wetR = 0.0f;

            // Parallel bandpass resonators
            for (int f = 0; f < kNumFormants; ++f)
            {
                // Transposed Direct Form II biquad
                float yL = b0_[f] * dryL + z1_[f][0];
                z1_[f][0] = b1_[f] * dryL - a1_[f] * yL + z2_[f][0];
                z2_[f][0] = b2_[f] * dryL - a2_[f] * yL;
                wetL += yL;

                float yR = b0_[f] * dryR + z1_[f][1];
                z1_[f][1] = b1_[f] * dryR - a1_[f] * yR + z2_[f][1];
                z2_[f][1] = b2_[f] * dryR - a2_[f] * yR;
                wetR += yR;
            }

            // Normalize parallel sum (kNumFormants resonators)
            wetL /= (float) kNumFormants;
            wetR /= (float) kNumFormants;

            left[i]  = dryL + mix_ * (wetL - dryL);
            right[i] = dryR + mix_ * (wetR - dryR);
        }
    }

private:
    void rebuildFilters()
    {
        // Constant-peak-gain bandpass (BPF) biquad coefficients.
        // At resonance: gain = 1.0 (0 dB). Two zeros at DC and Nyquist.
        for (int f = 0; f < kNumFormants; ++f)
        {
            float fc    = formantHz_[f];
            float w0    = 2.0f * juce::MathConstants<float>::pi * fc / (float) sr_;
            float alpha = std::sin (w0) / (2.0f * q_);

            float cosW0  = std::cos (w0);
            float a0inv  = 1.0f / (1.0f + alpha);

            b0_[f] =  alpha * a0inv;
            b1_[f] =  0.0f;
            b2_[f] = -alpha * a0inv;
            a1_[f] = -2.0f * cosW0 * a0inv;
            a2_[f] =  (1.0f - alpha) * a0inv;
        }
    }

    double sr_  = 44100.0;
    float  q_   = 8.0f;
    float  mix_ = 0.4f;

    float formantHz_[kNumFormants] = { 800.0f, 1200.0f, 2500.0f, 3500.0f };

    // Biquad coefficients per formant
    float b0_[kNumFormants] = {};
    float b1_[kNumFormants] = {};
    float b2_[kNumFormants] = {};
    float a1_[kNumFormants] = {};
    float a2_[kNumFormants] = {};

    // State: z1[formant][channel], z2[formant][channel]
    float z1_[kNumFormants][2] = {};
    float z2_[kNumFormants][2] = {};
};

} // namespace xolokun
