#pragma once
#include <cmath>
#include <algorithm>
#include <array>
#include "../CytomicSVF.h"
#include "../ParameterSmoother.h"

namespace xoceanus {

//==============================================================================
// fXFormant — Membrane Collection formant filter.
//
// 4 parallel bandpass filters at vocal formant frequencies (F1-F4).
// Each band has independent frequency and gain. Shift knob moves all
// formant frequencies up/down together (vocal character change).
// On voice: male→female, age, size shifts without pitch change.
// On synth: vowel sweeps, talk-box territory, "speaking" synths.
//
// Vowel table from Hillenbrand et al. (1995) adult male averages.
//
// CPU budget: ~30 ops/sample (4x CytomicSVF bandpass)
// Zero CPU when mix = 0 (early return in processBlock).
//
// Parameters:
//   shift     — [-1, +1]  : moves all formants down/up by up to one octave
//   vowel     — [0, 4]    : interpolates between A(0)/E(1)/I(2)/O(3)/U(4)
//   resonance — [0.5, 20] : Q of each bandpass filter
//   mix       — [0, 1]    : wet/dry blend
//==============================================================================
class fXFormant
{
public:
    fXFormant() = default;

    //--------------------------------------------------------------------------
    void prepare (double sampleRate, int /*maxBlockSize*/)
    {
        sr_ = static_cast<float> (sampleRate);
        for (int b = 0; b < kNumBands; ++b)
        {
            bpL_[b].setMode (CytomicSVF::Mode::BandPass);
            bpR_[b].setMode (CytomicSVF::Mode::BandPass);
            bpL_[b].reset();
            bpR_[b].reset();
        }
        shiftSmoother_.prepare (sampleRate, 0.005f);
        mixSmoother_.prepare   (sampleRate, 0.005f);
        shiftSmoother_.snapTo (0.0f);
        mixSmoother_.snapTo   (0.0f);
        updateFormants (0.0f);
    }

    //--------------------------------------------------------------------------
    // Shift: -1.0 to +1.0 (shifts all formants down/up by up to one octave)
    void setShift (float shift) noexcept
    {
        shift_ = std::clamp (shift, -1.0f, 1.0f);
        shiftSmoother_.set (shift_);
    }

    // Vowel: 0-4 selects A/E/I/O/U base formant table (continuous interpolation)
    void setVowel (float vowel) noexcept
    {
        vowel_ = std::clamp (vowel, 0.0f, 4.0f);
    }

    // Resonance: Q of the bandpass filters (0.5 - 20.0)
    void setResonance (float q) noexcept
    {
        resonance_ = std::clamp (q, 0.5f, 20.0f);
    }

    void setMix (float m) noexcept
    {
        mix_ = std::clamp (m, 0.0f, 1.0f);
        mixSmoother_.set (mix_);
    }

    //--------------------------------------------------------------------------
    void processBlock (float* left, float* right, int numSamples)
    {
        if (mix_ < 0.001f) return;  // zero CPU at mix=0

        // Update formant coefficients once per block (not per sample)
        const float smoothedShift = shiftSmoother_.process();
        updateFormants (smoothedShift);

        for (int i = 0; i < numSamples; ++i)
        {
            const float smoothMix = mixSmoother_.process();

            const float dryL = left[i];
            const float dryR = right[i];

            float wetL = 0.0f, wetR = 0.0f;
            for (int b = 0; b < kNumBands; ++b)
            {
                wetL += bpL_[b].processSample (dryL) * kBandGain[b];
                wetR += bpR_[b].processSample (dryR) * kBandGain[b];
            }

            left[i]  = dryL + (wetL - dryL) * smoothMix;
            right[i] = dryR + (wetR - dryR) * smoothMix;
        }
    }

    //--------------------------------------------------------------------------
    void reset() noexcept
    {
        for (int b = 0; b < kNumBands; ++b)
        {
            bpL_[b].reset();
            bpR_[b].reset();
        }
        shiftSmoother_.snapTo (0.0f);
        mixSmoother_.snapTo   (0.0f);
    }

private:
    static constexpr int kNumBands = 4;

    // Per-band relative gain (F1 loudest, upper formants attenuated)
    static constexpr float kBandGain[kNumBands] = { 1.0f, 0.8f, 0.6f, 0.4f };

    // Vowel formant frequency tables (Hz) — F1, F2, F3, F4
    // Based on Hillenbrand et al. (1995), "Acoustic characteristics of American
    // English vowels", Journal of the Acoustical Society of America 97(5).
    // Values are adult male averages for the listed vowels.
    inline static constexpr float kVowelTable[5][4] = {
        { 730.0f, 1090.0f, 2440.0f, 3400.0f },  // A (father)
        { 530.0f, 1840.0f, 2480.0f, 3520.0f },  // E (bed)
        { 390.0f, 1990.0f, 2550.0f, 3600.0f },  // I (beet)
        { 570.0f,  840.0f, 2410.0f, 3400.0f },  // O (boat)
        { 440.0f, 1020.0f, 2240.0f, 3400.0f },  // U (boot)
    };

    float sr_        = 44100.0f;
    float shift_     = 0.0f;
    float vowel_     = 0.0f;   // 0=A, 1=E, 2=I, 3=O, 4=U
    float resonance_ = 8.0f;
    float mix_       = 0.0f;

    CytomicSVF bpL_[kNumBands], bpR_[kNumBands];

    ParameterSmoother shiftSmoother_;
    ParameterSmoother mixSmoother_;

    //--------------------------------------------------------------------------
    void updateFormants (float shift) noexcept
    {
        // Interpolate between adjacent vowel rows
        const int vowelA = std::clamp (static_cast<int> (vowel_), 0, 3);
        const int vowelB = vowelA + 1;  // always <= 4 because vowelA <= 3
        const float frac = vowel_ - static_cast<float> (vowelA);

        // Shift multiplier: shift=-1 → 0.5x freq (down one octave)
        //                   shift=0  → 1.0x (no change)
        //                   shift=+1 → 2.0x (up one octave)
        const float shiftMult = std::pow (2.0f, shift);

        for (int b = 0; b < kNumBands; ++b)
        {
            const float freqA = kVowelTable[vowelA][b];
            const float freqB = kVowelTable[vowelB][b];  // vowelB <= 4, always valid
            float freq = (freqA + (freqB - freqA) * frac) * shiftMult;

            // Clamp to safe audio range (avoid instability near Nyquist)
            freq = std::clamp (freq, 20.0f, sr_ * 0.45f);

            // Map resonance (0.5-20.0) to CytomicSVF resonance [0, 1]
            // resonance_=0.5 → SVF res ~0 (Butterworth); resonance_=20 → SVF res ~0.95
            const float svfRes = std::clamp ((resonance_ - 0.5f) / 20.5f, 0.0f, 0.95f);

            bpL_[b].setCoefficients (freq, svfRes, sr_);
            bpR_[b].setCoefficients (freq, svfRes, sr_);
        }
    }
};

} // namespace xoceanus
