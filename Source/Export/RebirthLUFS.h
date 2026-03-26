#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

namespace xolokun {

//==============================================================================
// RebirthLUFS — ITU-R BS.1770-4 K-weighted integrated loudness measurement.
//
// Algorithm:
//   1. Pre-filter stage 1: 2nd-order high-shelf (+4 dB above ~1.5kHz)
//   2. Pre-filter stage 2: 2nd-order high-pass (fc ≈ 38Hz, RLB weighting)
//   3. Mean-square measurement on K-weighted signal
//   4. Integration: -0.691 + 10*log10(sum_of_mean_square) LUFS
//
// Coefficients are for 48kHz reference and are rescaled using bilinear
// transform prewarp for non-48kHz sample rates — never hardcoded.
//
// Returns integrated loudness in LUFS (negative value; -14 = loud, -23 = quiet).
//==============================================================================

struct KWeightingFilter
{
    // Transposed Direct Form II biquad state (stereo)
    float z1[2] = {};
    float z2[2] = {};

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;

    void setCoeffs (float b0_, float b1_, float b2_, float a1_, float a2_)
    {
        b0 = b0_; b1 = b1_; b2 = b2_; a1 = a1_; a2 = a2_;
        z1[0] = z1[1] = z2[0] = z2[1] = 0.0f;
    }

    float processSample (float x, int ch)
    {
        float y = b0 * x + z1[ch];
        z1[ch]  = b1 * x - a1 * y + z2[ch];
        z2[ch]  = b2 * x - a2 * y;
        return y;
    }
};

//------------------------------------------------------------------------------
// Build K-weighting filter pair for a given sample rate.
// Stage 1 = high-shelf pre-filter; Stage 2 = high-pass RLB weighting.
// Coefficients derived from BS.1770 reference (48kHz) via bilinear prewarp.
//------------------------------------------------------------------------------
inline void buildKWeightingFilters (double sampleRate,
                                    KWeightingFilter& stage1,
                                    KWeightingFilter& stage2)
{
    // --- Stage 1: High-shelf pre-filter ---
    // BS.1770 reference @ 48kHz:
    //   b0 =  1.53512485958697, b1 = -2.69169618940638, b2 =  1.19839281085285
    //   a1 = -1.69065929318241, a2 =  0.73248077421585
    // Prewarp to target sample rate using the pole/zero mapping via fc.
    // The filter's characteristic frequency is approximately 1681.97 Hz.
    // We use bilinear transform: compute analog prototype poles/zeros, then map.
    // For correctness across sample rates, apply the standard bilinear frequency
    // prewarp: Wc_d = 2*sr*tan(pi*fc/sr), where fc ≈ 1681.97 Hz for stage 1.
    {
        constexpr double fc1   = 1681.97;
        double wc  = 2.0 * sampleRate * std::tan (juce::MathConstants<double>::pi * fc1 / sampleRate);
        // Analog high-shelf (gain ≈ +3.999 dB at high frequencies):
        // H(s) = (s^2 + Vg * wc/Q * s + wc^2) / (s^2 + wc/Q * s + wc^2)
        // Q = 0.5, Vg = sqrt(10^(3.999/20)) ≈ 1.5848
        constexpr double Q    = 0.5;
        constexpr double Vg   = 1.58489319; // 10^(4/20)
        double wc2 = wc * wc;
        double sr2 = 4.0 * sampleRate * sampleRate;
        double a0  = sr2 + wc / Q * sampleRate * 2.0 + wc2;

        // Bilinear transform of analog high-shelf
        double b0  = (sr2 + Vg * wc / Q * sampleRate * 2.0 + wc2) / a0;
        double b1  = 2.0 * (wc2 - sr2) / a0;
        double b2  = (sr2 - Vg * wc / Q * sampleRate * 2.0 + wc2) / a0;
        double a1  = 2.0 * (wc2 - sr2) / a0;
        double a2  = (sr2 - wc / Q * sampleRate * 2.0 + wc2) / a0;

        stage1.setCoeffs ((float) b0, (float) b1, (float) b2, (float) a1, (float) a2);
    }

    // --- Stage 2: RLB high-pass weighting ---
    // BS.1770 reference: 2nd-order Butterworth HP, fc = 38.135 Hz
    {
        constexpr double fc2  = 38.135;
        double wc  = 2.0 * sampleRate * std::tan (juce::MathConstants<double>::pi * fc2 / sampleRate);
        double wc2 = wc * wc;
        double sr2 = 4.0 * sampleRate * sampleRate;
        constexpr double Q = juce::MathConstants<double>::sqrt2 * 0.5; // Butterworth 2nd-order
        double a0  = sr2 + wc / Q * sampleRate * 2.0 + wc2;

        // Butterworth HP: b0 = sr2, b1 = -2*sr2, b2 = sr2
        double b0  = sr2 / a0;
        double b1  = -2.0 * sr2 / a0;
        double b2  = sr2 / a0;
        double a1  = 2.0 * (wc2 - sr2) / a0;
        double a2  = (sr2 - wc / Q * sampleRate * 2.0 + wc2) / a0;

        stage2.setCoeffs ((float) b0, (float) b1, (float) b2, (float) a1, (float) a2);
    }
}

//------------------------------------------------------------------------------
// computeIntegratedLUFS() — measure integrated loudness of a buffer.
// Returns LUFS value (typically -40.0 to 0.0). Returns -100.0 for silence.
//------------------------------------------------------------------------------
inline float computeIntegratedLUFS (const juce::AudioBuffer<float>& buffer,
                                    double sampleRate)
{
    int numCh    = buffer.getNumChannels();
    int numSamps = buffer.getNumSamples();
    if (numSamps == 0 || numCh == 0) return -100.0f;

    KWeightingFilter s1, s2;
    buildKWeightingFilters (sampleRate, s1, s2);

    double sumMeanSquare = 0.0;
    int    count         = 0;

    // Process in 100ms blocks for gating (absolute gate = -70 LUFS threshold)
    constexpr double kGateThresholdLUFS = -70.0;
    int blockSamps = std::max (1, (int) (sampleRate * 0.1));

    for (int blockStart = 0; blockStart < numSamps; blockStart += blockSamps)
    {
        int blockEnd = std::min (blockStart + blockSamps, numSamps);
        double blockMeanSquare = 0.0;

        for (int i = blockStart; i < blockEnd; ++i)
        {
            float kw = 0.0f;
            // Average over channels (BS.1770 per-channel mean square then average)
            for (int ch = 0; ch < numCh; ++ch)
            {
                float x = buffer.getSample (ch, i);
                float y = s1.processSample (x, ch % 2);
                float z = s2.processSample (y, ch % 2);
                kw += z * z;
            }
            kw /= (float) numCh;
            blockMeanSquare += kw;
        }
        blockMeanSquare /= (double) (blockEnd - blockStart);

        // Absolute gate: ignore blocks below -70 LUFS
        double blockLUFS = -0.691 + 10.0 * std::log10 (std::max (1e-30, blockMeanSquare));
        if (blockLUFS > kGateThresholdLUFS)
        {
            sumMeanSquare += blockMeanSquare;
            ++count;
        }
    }

    if (count == 0) return -100.0f;

    double integrated = -0.691 + 10.0 * std::log10 (sumMeanSquare / (double) count);
    return (float) integrated;
}

} // namespace xolokun
