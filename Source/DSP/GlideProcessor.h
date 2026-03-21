#pragma once
#include <cmath>
#include <algorithm>

namespace xomnibus {

//==============================================================================
// GlideProcessor — Shared portamento/glide for the XOmnibus fleet.
//
// Consolidates the identical frequency-domain glide pattern found in 35+
// engines. Operates in Hz space (analog synth convention — upward glides
// feel naturally slower than downward, matching VCO behavior).
//
// The glide coefficient is computed from a time constant: the time it takes
// for the frequency to reach ~63% of the way to the target. At coeff=1.0
// the glide is instant (no portamento).
//
// Usage:
//   // In voice struct:
//   GlideProcessor glide;
//
//   // On noteOn:
//   glide.setTarget (targetFreqHz);
//
//   // Per-sample in render loop:
//   float freq = glide.process();
//
//   // When glide time changes (once per block):
//   glide.setTime (glideTimeSec, sampleRate);
//
// All methods are noexcept and allocation-free for real-time safety.
//==============================================================================
struct GlideProcessor
{
    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------

    /// Set glide time in seconds. 0 = instant (no glide).
    /// Call once per block when the parameter changes, not per-sample.
    void setTime (float timeSec, float sampleRate) noexcept
    {
        if (timeSec <= 0.0f || sampleRate <= 0.0f)
            coeff = 1.0f;  // instant
        else
            coeff = 1.0f - std::exp (-1.0f / (timeSec * sampleRate));
    }

    /// Set glide coefficient directly (for engines that compute it themselves).
    /// coeff=1.0 → instant, coeff→0 → infinitely slow.
    void setCoeff (float c) noexcept
    {
        coeff = c;
    }

    //--------------------------------------------------------------------------
    // Target management
    //--------------------------------------------------------------------------

    /// Set a new target frequency. The glide will smoothly approach this value.
    void setTarget (float freqHz) noexcept
    {
        targetFreq = freqHz;
    }

    /// Set target and snap current frequency immediately (no glide).
    /// Use for the first note or poly voice allocation (no portamento on fresh voices).
    void snapTo (float freqHz) noexcept
    {
        targetFreq  = freqHz;
        currentFreq = freqHz;
    }

    /// Set target with conditional snap: if current frequency is uninitialized
    /// (< 1 Hz), snap immediately; otherwise glide. This is the common pattern
    /// for voice noteOn — first note snaps, subsequent notes glide.
    void setTargetOrSnap (float freqHz) noexcept
    {
        targetFreq = freqHz;
        if (currentFreq < 1.0f)
            currentFreq = freqHz;
    }

    //--------------------------------------------------------------------------
    // Processing
    //--------------------------------------------------------------------------

    /// Advance one sample and return the current (gliding) frequency in Hz.
    float process() noexcept
    {
        float delta = (targetFreq - currentFreq) * coeff;
        currentFreq += delta;
        // Snap to target when convergence stalls due to float32 precision.
        // At high frequencies (e.g., 880 Hz), the ULP gap is ~0.00006 Hz,
        // so a small coeff can't produce a delta large enough to advance.
        // The 0.2 Hz threshold is inaudible (~0.4 cents at 880 Hz) and
        // prevents the glide from permanently "almost there."
        if (std::fabs (currentFreq - targetFreq) < 0.2f)
            currentFreq = targetFreq;
        return currentFreq;
    }

    /// Get current frequency without advancing. Useful for readback.
    float getFreq() const noexcept { return currentFreq; }

    /// Check if glide has effectively reached the target (within 0.01 Hz).
    bool isSettled() const noexcept
    {
        return std::fabs (currentFreq - targetFreq) < 0.01f;
    }

    //--------------------------------------------------------------------------
    // State management
    //--------------------------------------------------------------------------

    void reset() noexcept
    {
        currentFreq = 0.0f;
        targetFreq  = 0.0f;
        coeff       = 1.0f;
    }

    //--------------------------------------------------------------------------
    // State — public for snapshot/restore
    //--------------------------------------------------------------------------
    float currentFreq = 0.0f;
    float targetFreq  = 0.0f;
    float coeff       = 1.0f;   // 1.0 = instant (no glide)
};

} // namespace xomnibus
