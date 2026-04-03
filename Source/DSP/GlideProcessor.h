// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <cmath>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// GlideProcessor — Shared portamento/glide for the XOceanus fleet.
//
// Consolidates the frequency-domain glide pattern found across 35+ engines.
// Operates in Hz space rather than MIDI note space, matching the analog VCO
// convention: a one-octave glide upward feels naturally slower than the same
// interval downward, because the Hz distance is twice as large. This is the
// behavior players expect from a Minimoog or Juno — not from a MIDI sequencer.
//
// The coefficient is derived from a time constant (τ): the time to close 63%
// of the gap to the target each second. coeff = 1.0 means instant arrival
// (no portamento). Lower values produce longer, smoother glides.
//
// Usage:
//   GlideProcessor glide;
//
//   // On prepare():
//   glide.setTime (0.1f, sampleRate);  // 100ms portamento
//
//   // On noteOn:
//   glide.setTargetOrSnap (targetFreqHz);  // snap on first note, glide thereafter
//
//   // Per-sample in render loop:
//   float freq = glide.process();
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
    void setTime(float timeSec, float sampleRate) noexcept
    {
        if (timeSec <= 0.0f || sampleRate <= 0.0f)
            coeff = 1.0f; // instant
        else
            coeff = 1.0f - std::exp(-1.0f / (timeSec * sampleRate));
    }

    /// Set glide coefficient directly (for engines that compute it themselves).
    /// coeff=1.0 → instant, coeff→0 → infinitely slow.
    void setCoeff(float c) noexcept { coeff = c; }

    //--------------------------------------------------------------------------
    // Target management
    //--------------------------------------------------------------------------

    /// Set a new target frequency. The glide will smoothly approach this value.
    void setTarget(float freqHz) noexcept { targetFreq = freqHz; }

    /// Set target and snap current frequency immediately (no glide).
    /// Use for the first note or poly voice allocation (no portamento on fresh voices).
    void snapTo(float freqHz) noexcept
    {
        targetFreq = freqHz;
        currentFreq = freqHz;
    }

    /// Set target with conditional snap: if current frequency is uninitialized
    /// (< 1 Hz), snap immediately; otherwise glide. This is the common pattern
    /// for voice noteOn — first note snaps, subsequent notes glide.
    void setTargetOrSnap(float freqHz) noexcept
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

        // Float32 convergence guard: at high frequencies the ULP gap can
        // exceed the step size, leaving the glide permanently 0.2 Hz short
        // of its target. That's inaudible (~0.4 cents at 880 Hz) — snap and move on.
        if (std::fabs(currentFreq - targetFreq) < 0.2f)
            currentFreq = targetFreq;

        return currentFreq;
    }

    /// Get current frequency without advancing. Useful for readback.
    float getFreq() const noexcept { return currentFreq; }

    /// Check if glide has effectively reached the target (within 0.01 Hz).
    bool isSettled() const noexcept { return std::fabs(currentFreq - targetFreq) < 0.01f; }

    //--------------------------------------------------------------------------
    // State management
    //--------------------------------------------------------------------------

    void reset() noexcept
    {
        currentFreq = 0.0f;
        targetFreq = 0.0f;
        coeff = 1.0f;
    }

    //--------------------------------------------------------------------------
    // State — public for snapshot/restore
    //--------------------------------------------------------------------------
    float currentFreq = 0.0f;
    float targetFreq = 0.0f;
    float coeff = 1.0f; // 1.0 = instant (no glide)
};

} // namespace xoceanus
