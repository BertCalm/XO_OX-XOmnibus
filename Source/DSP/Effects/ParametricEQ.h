#pragma once
#include <cmath>
#include <algorithm>
#include "../CytomicSVF.h"
#include "../ParameterSmoother.h"
#include "../FastMath.h"

namespace xoceanus {

//==============================================================================
// ParametricEQ — 4-band master parametric equaliser.
//
// Four independent bands, each implemented as a stereo CytomicSVF (Cytomic
// topology-preserving SVF with trapezoidal integration — unconditionally
// stable at all settings):
//
//   Band 1:  Low shelf    20 – 500 Hz    ±12 dB   Q 0.5 – 5.0
//   Band 2:  Peak / bell  100 – 5000 Hz  ±12 dB   Q 0.5 – 10.0
//   Band 3:  Peak / bell  500 – 15000 Hz ±12 dB   Q 0.5 – 10.0
//   Band 4:  High shelf   2000 – 20000 Hz ±12 dB  Q 0.5 – 5.0
//
// Features:
//   - Per-band one-pole coefficient smoothing via ParameterSmoother (5 ms
//     default): eliminates zipper noise on knob automation
//   - Zero CPU when all four gain parameters are within ±0.05 dB of 0 dB
//     (early-return guard in processBlock)
//   - Denormal protection inherited from CytomicSVF state updates
//   - Stereo in-place processing (separate L / R filter instances per band
//     share identical coefficients — avoids per-sample stereo divergence)
//   - Always pass sampleRate — never hardcodes 44100
//   - setBand() clamps inputs to the documented ranges so callers cannot
//     produce unstable coefficients by accident
//
// Coefficient smoothing strategy:
//   The smoothers track target freq / gainDb / Q and recompute SVF
//   coefficients once per SAMPLE only when any smoother is still converging.
//   Once all smoothers have settled the inner loop skips the coefficient
//   recalculation for zero overhead in the steady state.
//
// Usage:
//   ParametricEQ eq;
//   eq.prepare (sampleRate);
//   eq.setBand (0, 100.0f, -3.0f, 0.7f);   // low shelf: 100 Hz, -3 dB
//   eq.processBlock (L, R, numSamples);
//==============================================================================
class ParametricEQ
{
public:
    static constexpr int kNumBands = 4;

    ParametricEQ() = default;

    //--------------------------------------------------------------------------
    /// Call once before the first processBlock (or after a sample-rate change).
    void prepare (double sampleRate)
    {
        sr = static_cast<float> (sampleRate);

        // Initialise smoothers — 5 ms is the XOceanus fleet standard
        for (int b = 0; b < kNumBands; ++b)
        {
            smoothFreq[b].prepare (sr, 0.005f);
            smoothGain[b].prepare (sr, 0.005f);
            smoothQ[b].prepare    (sr, 0.005f);

            // Snap to defaults (no transient on first block)
            smoothFreq[b].snapTo (kDefaultFreq[b]);
            smoothGain[b].snapTo (0.0f);
            smoothQ[b].snapTo    (kDefaultQ[b]);

            // Reset filter state for both channels
            svfL[b].reset();
            svfR[b].reset();

            // Store current targets so isSettled() check is correct from the start
            currentFreq[b] = kDefaultFreq[b];
            currentGain[b] = 0.0f;
            currentQ[b]    = kDefaultQ[b];
        }

        // Set filter modes (fixed per band)
        svfL[0].setMode (CytomicSVF::Mode::LowShelf);
        svfR[0].setMode (CytomicSVF::Mode::LowShelf);
        svfL[1].setMode (CytomicSVF::Mode::Peak);
        svfR[1].setMode (CytomicSVF::Mode::Peak);
        svfL[2].setMode (CytomicSVF::Mode::Peak);
        svfR[2].setMode (CytomicSVF::Mode::Peak);
        svfL[3].setMode (CytomicSVF::Mode::HighShelf);
        svfR[3].setMode (CytomicSVF::Mode::HighShelf);

        // Push initial coefficients to the SVFs
        for (int b = 0; b < kNumBands; ++b)
            applyCoefficients (b, currentFreq[b], currentGain[b], currentQ[b]);

        settled = false;  // run one block to flush smoother state
    }

    //--------------------------------------------------------------------------
    /// Set parameters for one band.  Call from the audio thread or parameter
    /// changed callback — both are safe because ParameterSmoother is atomic-free
    /// (the chain is always called from a single audio thread).
    ///
    /// @param band    0 = low shelf, 1 = low-mid peak, 2 = high-mid peak, 3 = high shelf
    /// @param freqHz  Cutoff/centre frequency, clamped to band-safe range
    /// @param gainDb  Gain in dB, clamped to [-12, +12]
    /// @param q       Q / bandwidth, clamped to band-safe range
    void setBand (int band, float freqHz, float gainDb, float q) noexcept
    {
        if (band < 0 || band >= kNumBands) return;

        const float safeFreq = clamp (freqHz, kFreqMin[band], kFreqMax[band]);
        const float safeGain = clamp (gainDb, -12.0f, 12.0f);
        const float safeQ    = clamp (q,      kQMin[band],   kQMax[band]);

        smoothFreq[band].set (safeFreq);
        smoothGain[band].set (safeGain);
        smoothQ[band].set    (safeQ);

        settled = false;  // wake up the smoothing loop
    }

    //--------------------------------------------------------------------------
    /// Enable / disable the EQ entirely.  When disabled processBlock is a no-op.
    void setEnabled (bool e) noexcept { enabled = e; }
    bool isEnabled()   const noexcept { return enabled; }

    //--------------------------------------------------------------------------
    /// Process stereo audio in-place.
    void processBlock (float* L, float* R, int numSamples) noexcept
    {
        if (!enabled || numSamples <= 0) return;

        // Zero-CPU early-return: all bands flat (gain within ±0.05 dB)
        // Check settled flag — set to false by setBand(), set back to true
        // here once all smoothers have converged.
        if (settled)
        {
            // All smoothers have converged — verify gains are truly flat
            bool allFlat = true;
            for (int b = 0; b < kNumBands; ++b)
            {
                if (std::fabs (currentGain[b]) > 0.05f)
                {
                    allFlat = false;
                    break;
                }
            }
            if (allFlat) return;
        }

        // Process each sample, advancing smoothers and recomputing coefficients
        // only while they are still converging.
        for (int i = 0; i < numSamples; ++i)
        {
            // Advance all smoothers by one sample
            bool anyMoving = false;
            for (int b = 0; b < kNumBands; ++b)
            {
                const float f = smoothFreq[b].process();
                const float g = smoothGain[b].process();
                const float q = smoothQ[b].process();

                // Detect movement — only recompute when something changed
                if (std::fabs (f - currentFreq[b]) > 0.5f  ||
                    std::fabs (g - currentGain[b]) > 0.001f ||
                    std::fabs (q - currentQ[b])    > 0.001f)
                {
                    currentFreq[b] = f;
                    currentGain[b] = g;
                    currentQ[b]    = q;
                    applyCoefficients (b, f, g, q);
                    anyMoving = true;
                }
            }

            if (!anyMoving && i == 0)
                settled = true;  // will skip smoothing loop next block

            // Run signal through all 4 bands in series (both channels)
            float sL = L[i];
            float sR = R[i];

            for (int b = 0; b < kNumBands; ++b)
            {
                sL = svfL[b].processSample (sL);
                sR = svfR[b].processSample (sR);
            }

            L[i] = sL;
            R[i] = sR;
        }
    }

    //--------------------------------------------------------------------------
    /// Reset all filter state (call on silence / stream-start).
    void reset() noexcept
    {
        for (int b = 0; b < kNumBands; ++b)
        {
            svfL[b].reset();
            svfR[b].reset();
        }
        settled = false;
    }

    //--------------------------------------------------------------------------
    /// Direct setters for use from the processor when APVTS is not used.
    void setBand1 (float freqHz, float gainDb, float q) noexcept { setBand (0, freqHz, gainDb, q); }
    void setBand2 (float freqHz, float gainDb, float q) noexcept { setBand (1, freqHz, gainDb, q); }
    void setBand3 (float freqHz, float gainDb, float q) noexcept { setBand (2, freqHz, gainDb, q); }
    void setBand4 (float freqHz, float gainDb, float q) noexcept { setBand (3, freqHz, gainDb, q); }

private:
    //--------------------------------------------------------------------------
    // Push current freq / gain / Q into both L and R SVFs for one band.
    void applyCoefficients (int b, float freqHz, float gainDb, float q) noexcept
    {
        // CytomicSVF resonance parameter maps [0,1] where 1 = self-oscillation.
        // We convert Q to resonance via:   resonance = 1 - 1/(2*Q)
        // This gives 0.5 at Q=1 (Butterworth) and approaches 1 at high Q.
        const float resonance = clamp (1.0f - 1.0f / (2.0f * q), 0.0f, 0.9999f);

        svfL[b].setCoefficients (freqHz, resonance, sr, gainDb);
        svfR[b].setCoefficients (freqHz, resonance, sr, gainDb);
    }

    //--------------------------------------------------------------------------
    // Band-range constants (frequency Hz, Q)
    static constexpr float kFreqMin[kNumBands] = {  20.0f,  100.0f,  500.0f, 2000.0f };
    static constexpr float kFreqMax[kNumBands] = { 500.0f, 5000.0f, 15000.0f, 20000.0f };
    static constexpr float kQMin[kNumBands]    = { 0.5f, 0.5f, 0.5f, 0.5f };
    static constexpr float kQMax[kNumBands]    = { 5.0f, 10.0f, 10.0f, 5.0f };

    // Default centre frequencies (musically sensible starting points)
    static constexpr float kDefaultFreq[kNumBands] = { 100.0f, 400.0f, 3000.0f, 10000.0f };
    static constexpr float kDefaultQ[kNumBands]    = { 0.707f, 1.0f, 1.0f, 0.707f };

    //--------------------------------------------------------------------------
    // Per-band stereo SVF instances
    CytomicSVF svfL[kNumBands];  // left-channel filters
    CytomicSVF svfR[kNumBands];  // right-channel filters

    // Parameter smoothers — one triple (freq, gain, Q) per band
    ParameterSmoother smoothFreq[kNumBands];
    ParameterSmoother smoothGain[kNumBands];
    ParameterSmoother smoothQ[kNumBands];

    // Last settled coefficient values (used to detect movement)
    float currentFreq[kNumBands] {};
    float currentGain[kNumBands] {};
    float currentQ[kNumBands]    {};

    float sr      = 44100.0f;
    bool  enabled = true;
    bool  settled = false;  // true once all smoothers have converged
};

} // namespace xoceanus
