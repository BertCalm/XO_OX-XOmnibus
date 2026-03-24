#pragma once
//==============================================================================
//
//  OfferingCollage.h — Layer Stacking, Chop Sim, Time-Stretch, Ring Mod
//  XO_OX Designs | XOlokun Multi-Engine Synthesizer
//
//  Simulates the creative act of sample manipulation:
//    Layer stacking  — 1-4 transient variations per trigger
//    Chop sim        — Rhythmic amplitude gating
//    Time-stretch    — Speed/pitch manipulation of the hit
//    Ring modulation — Inter-layer metallic textures
//
//  Controlled by the FLIP macro (M3).
//  CPU safety: effective polyphony capped at voices × layers ≤ 16.
//
//  Accent: Crate Wax Yellow #E5B80B | Prefix: ofr_
//
//==============================================================================

#include "../../DSP/FastMath.h"
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace xolokun {

//==============================================================================
// OfferingCollage — Per-voice collage processor applied after texture layer.
//
// Works on a per-voice basis: stores a short buffer of the current hit,
// then applies chop/stretch/ring-mod effects.
//==============================================================================
class OfferingCollage
{
public:
    void prepare (float sampleRate) noexcept
    {
        sr_ = sampleRate;
        reset();
    }

    //--------------------------------------------------------------------------
    // Trigger collage processing for a new hit.
    //
    // layers: number of stacked variations [1, 4]
    // chop: chop simulation intensity [0, 1]
    // stretch: time-stretch factor [0.5, 2.0]
    // ringMod: inter-layer ring mod depth [0, 1]
    //--------------------------------------------------------------------------
    void trigger (int layers, float chop, float stretch, float ringMod) noexcept
    {
        layers_ = std::max (1, std::min (layers, kMaxLayers));
        chop_ = chop;
        stretch_ = std::max (0.5f, std::min (stretch, 2.0f));
        ringMod_ = ringMod;
        chopPhase_ = 0.0f;
        stretchReadPos_ = 0.0f;
        writePos_ = 0;
        bufferFilled_ = false;
        sampleCount_ = 0;

        // Seed layer offsets for variation
        for (int i = 0; i < kMaxLayers; ++i)
        {
            layerPhaseOffsets_[i] = static_cast<float> (i) * 0.17f; // prime-ish spacing
            layerGains_[i] = (i < layers_) ? (1.0f / static_cast<float> (layers_)) : 0.0f;
        }
    }

    //--------------------------------------------------------------------------
    // Process one sample through the collage chain.
    // Input is the post-texture transient sample.
    //--------------------------------------------------------------------------
    float process (float input) noexcept
    {
        // Store input in circular buffer for stretch/chop
        if (writePos_ < kBufferSize)
        {
            buffer_[writePos_] = input;
            writePos_++;
            if (writePos_ >= kBufferSize) bufferFilled_ = true;
        }
        sampleCount_++;

        float out = input;

        // Apply time-stretch (only when stretch != 1.0)
        if (std::abs (stretch_ - 1.0f) > 0.01f && bufferFilled_)
            out = processStretch (input);

        // Apply chop simulation
        if (chop_ > 0.01f)
            out = processChop (out);

        // Apply layer stacking + ring mod (only for layers > 1)
        if (layers_ > 1)
            out = processLayers (out);

        return out;
    }

    void reset() noexcept
    {
        writePos_ = 0;
        bufferFilled_ = false;
        chopPhase_ = 0.0f;
        stretchReadPos_ = 0.0f;
        sampleCount_ = 0;
        std::fill (buffer_, buffer_ + kBufferSize, 0.0f);
    }

private:
    //--------------------------------------------------------------------------
    // Time-stretch: reads from buffer at altered rate.
    // stretch < 1.0 = compressed (faster playback), > 1.0 = stretched (slower).
    //--------------------------------------------------------------------------
    float processStretch (float input) noexcept
    {
        // Read position advances at 1/stretch rate
        stretchReadPos_ += 1.0f / stretch_;
        int readIdx = static_cast<int> (stretchReadPos_) % kBufferSize;
        float frac = stretchReadPos_ - std::floor (stretchReadPos_);

        // Linear interpolation for smooth stretch
        int nextIdx = (readIdx + 1) % kBufferSize;
        float stretched = buffer_[readIdx] * (1.0f - frac) + buffer_[nextIdx] * frac;

        // Crossfade between original and stretched
        float stretchAmount = std::abs (stretch_ - 1.0f);
        return input * (1.0f - stretchAmount) + stretched * stretchAmount;
    }

    //--------------------------------------------------------------------------
    // Chop simulation: rhythmic amplitude gating.
    // Creates the "chopped break" aesthetic of sample-based production.
    //--------------------------------------------------------------------------
    float processChop (float input) noexcept
    {
        // Chop rate: 4-32 Hz (faster chop at higher intensity)
        float chopRate = 4.0f + chop_ * 28.0f;
        chopPhase_ += chopRate / sr_;
        if (chopPhase_ >= 1.0f) chopPhase_ -= 1.0f;

        // Square-ish gate with smooth edges to avoid clicks
        float gate;
        float duty = 0.3f + (1.0f - chop_) * 0.4f; // 30-70% duty cycle
        if (chopPhase_ < duty)
        {
            // Gate open — with smooth attack
            float attackPhase = chopPhase_ / (duty * 0.1f);
            gate = std::min (1.0f, attackPhase);
        }
        else
        {
            // Gate closing — with smooth release
            float releasePhase = (chopPhase_ - duty) / ((1.0f - duty) * 0.1f);
            gate = std::max (0.0f, 1.0f - releasePhase);
        }

        // Mix: chop=0 is bypass, chop=1 is full chop effect
        return input * (1.0f - chop_ + chop_ * gate);
    }

    //--------------------------------------------------------------------------
    // Layer stacking: creates variation by reading buffer at offset positions
    // and optionally ring-modulating between layers.
    //--------------------------------------------------------------------------
    float processLayers (float input) noexcept
    {
        float sum = input * layerGains_[0]; // Layer 0 = original

        for (int i = 1; i < layers_; ++i)
        {
            // Read from buffer at offset position (creates temporal displacement)
            int offset = static_cast<int> (layerPhaseOffsets_[i] * sr_ * 0.005f); // 0-5ms offsets
            int readIdx = (sampleCount_ - offset) % kBufferSize;
            if (readIdx < 0) readIdx += kBufferSize;
            float layerSample = buffer_[readIdx];

            // Apply ring modulation between layers if enabled
            if (ringMod_ > 0.01f)
            {
                float ringModded = input * layerSample;
                layerSample = layerSample * (1.0f - ringMod_) + ringModded * ringMod_;
            }

            sum += layerSample * layerGains_[i];
        }

        return sum;
    }

    //--------------------------------------------------------------------------
    // State
    //--------------------------------------------------------------------------
    float sr_ = 44100.0f;
    int layers_ = 1;
    float chop_ = 0.0f;
    float stretch_ = 1.0f;
    float ringMod_ = 0.0f;

    // Circular buffer for stretch/layer operations
    static constexpr int kBufferSize = 4096; // ~93ms at 44.1kHz
    float buffer_[kBufferSize] = {};
    int writePos_ = 0;
    bool bufferFilled_ = false;
    int sampleCount_ = 0;

    // Chop state
    float chopPhase_ = 0.0f;

    // Stretch state
    float stretchReadPos_ = 0.0f;

    // Layer state
    static constexpr int kMaxLayers = 4;
    float layerPhaseOffsets_[kMaxLayers] = {};
    float layerGains_[kMaxLayers] = {};
};

} // namespace xolokun
