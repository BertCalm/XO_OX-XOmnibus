#pragma once
#include <cmath>
#include <cstring>
#include "FastMath.h"

namespace xolokun {

//==============================================================================
// WavetableOscillator — Wavetable oscillator with crossfade morphing.
//
// Supports up to 256 frames of 2048 samples each. The position parameter
// morphs smoothly between adjacent frames using linear crossfading.
// Within each frame, sample-accurate linear interpolation is used for
// pitch-accurate playback at any frequency.
//
// Memory layout: All wavetable data is stored in a fixed-size internal buffer
// (256 * 2048 = 524288 floats, ~2 MB). No heap allocation occurs during
// processing. The loadWavetable() method copies data into the internal buffer
// and should be called from a non-audio thread before playback begins.
//
// Usage:
//   WavetableOscillator wt;
//   wt.loadWavetable(myWavetableData, 32, 2048);
//   wt.setFrequency(440.0f, 44100.0f);
//   wt.setPosition(0.5f);  // morph halfway through the table
//   float sample = wt.processSample();
//==============================================================================
class WavetableOscillator
{
public:
    static constexpr int kMaxFrames   = 256;
    static constexpr int kDefaultFrameSize = 2048;
    static constexpr int kMaxFrameSize     = 2048;
    static constexpr int kMaxTableSize     = kMaxFrames * kMaxFrameSize;  // 524288 samples

    WavetableOscillator()
    {
        // Zero-initialize the entire wavetable buffer
        std::memset (table, 0, sizeof (table));
    }

    //--------------------------------------------------------------------------
    /// Load wavetable data into the internal buffer.
    /// This copies the data, so the source pointer does not need to persist.
    /// Call from a non-audio thread only.
    ///
    /// @param data       Pointer to interleaved wavetable frames
    ///                   (frame0[0..frameSize-1], frame1[0..frameSize-1], ...).
    /// @param numFrames  Number of frames in the wavetable (1 to 256).
    /// @param frameSize  Samples per frame (default 2048, max 2048).
    void loadWavetable (const float* data, int numFrames, int frameSize = kDefaultFrameSize) noexcept
    {
        if (data == nullptr || numFrames <= 0 || frameSize <= 0)
            return;

        if (numFrames > kMaxFrames)
            numFrames = kMaxFrames;
        if (frameSize > kMaxFrameSize)
            frameSize = kMaxFrameSize;

        // frameSize must be power-of-2 for bitmask wrapping in readFrame().
        // Round up to next power of 2 if needed.
        if ((frameSize & (frameSize - 1)) != 0)
        {
            int p = 1;
            while (p < frameSize && p <= kMaxFrameSize) p <<= 1;
            frameSize = (p <= kMaxFrameSize) ? p : kMaxFrameSize;
        }

        this->numFrames = numFrames;
        this->frameSize = frameSize;

        int totalSamples = numFrames * frameSize;
        std::memcpy (table, data, static_cast<size_t> (totalSamples) * sizeof (float));

        // Zero out any remaining buffer space beyond loaded data
        if (totalSamples < kMaxTableSize)
            std::memset (table + totalSamples, 0,
                         static_cast<size_t> (kMaxTableSize - totalSamples) * sizeof (float));
    }

    //--------------------------------------------------------------------------
    /// Set the morph position within the wavetable.
    /// @param pos  Position in [0.0, 1.0]. 0.0 = first frame, 1.0 = last frame.
    void setPosition (float pos) noexcept
    {
        if (pos < 0.0f) pos = 0.0f;
        if (pos > 1.0f) pos = 1.0f;
        position = pos;
    }

    /// Get the current morph position.
    float getPosition() const noexcept { return position; }

    //--------------------------------------------------------------------------
    /// Set the playback frequency and sample rate.
    /// @param freqHz     Desired frequency in Hz.
    /// @param sampleRate Current sample rate in Hz.
    void setFrequency (float freqHz, float sampleRate) noexcept
    {
        if (sampleRate <= 0.0f) return;

        // Clamp to safe range
        float maxFreq = sampleRate * 0.5f;
        if (freqHz < 0.0f)    freqHz = 0.0f;
        if (freqHz > maxFreq) freqHz = maxFreq;

        // Phase increment = (frequency * frameSize) / sampleRate
        // This advances through one frame per period
        phaseIncrement = (freqHz * static_cast<float> (frameSize)) / sampleRate;
    }

    //--------------------------------------------------------------------------
    /// Generate and return the next output sample.
    /// Uses linear interpolation within frames and linear crossfade between frames.
    float processSample() noexcept
    {
        if (numFrames <= 0 || frameSize <= 0)
            return 0.0f;

        // Determine which two frames to crossfade between
        float framePos = position * static_cast<float> (numFrames - 1);
        int frameA = static_cast<int> (framePos);
        int frameB = frameA + 1;
        float frameFrac = framePos - static_cast<float> (frameA);

        // Clamp frame indices
        if (frameA < 0)             frameA = 0;
        if (frameA >= numFrames)    frameA = numFrames - 1;
        if (frameB >= numFrames)    frameB = numFrames - 1;

        // Read interpolated sample from frame A
        float sampleA = readFrame (frameA, phase);

        // Read interpolated sample from frame B and crossfade
        float out;
        if (frameA == frameB || frameFrac < 0.0001f)
        {
            out = sampleA;
        }
        else
        {
            float sampleB = readFrame (frameB, phase);
            out = sampleA + frameFrac * (sampleB - sampleA);
        }

        // Advance phase through the frame (bounded wrap to prevent runaway loops)
        phase += phaseIncrement;
        float fs = static_cast<float> (frameSize);
        if (fs > 0.0f)
        {
            phase = std::fmod (phase, fs);
            if (phase < 0.0f) phase += fs;
        }

        return out;
    }

    //--------------------------------------------------------------------------
    /// Reset the oscillator phase.
    void reset() noexcept
    {
        phase = 0.0f;
    }

    /// Set the phase directly. Value is in samples within the frame [0, frameSize).
    void setPhase (float p) noexcept
    {
        float fs = static_cast<float> (frameSize);
        if (fs > 0.0f)
        {
            phase = std::fmod (p, fs);
            if (phase < 0.0f) phase += fs;
        }
        else
        {
            phase = 0.0f;
        }
    }

    //--------------------------------------------------------------------------
    /// Process a block of samples into a buffer.
    /// @param output      Pointer to output buffer.
    /// @param numSamples  Number of samples to generate.
    void processBlock (float* output, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

    //--------------------------------------------------------------------------
    /// Check if a wavetable has been loaded.
    bool isLoaded() const noexcept { return numFrames > 0 && frameSize > 0; }

    /// Get the number of loaded frames.
    int getNumFrames() const noexcept { return numFrames; }

    /// Get the frame size.
    int getFrameSize() const noexcept { return frameSize; }

private:
    //--------------------------------------------------------------------------
    /// Read a linearly-interpolated sample from a specific frame at a given
    /// phase position (in samples, not normalized).
    float readFrame (int frameIndex, float samplePos) const noexcept
    {
        // Bounds-check frame index to prevent buffer overrun
        if (frameIndex < 0 || frameIndex >= numFrames)
            return 0.0f;

        int offset = frameIndex * frameSize;

        // Compute integer and fractional parts of sample position
        int idx0 = static_cast<int> (samplePos);
        int idx1 = idx0 + 1;
        float frac = samplePos - static_cast<float> (idx0);

        // Wrap indices within frame (power-of-2 masking for 2048)
        int mask = frameSize - 1;  // works for power-of-2 frame sizes
        idx0 &= mask;
        idx1 &= mask;

        // Final safety check against total table size
        int accessMax = offset + std::max (idx0, idx1);
        if (accessMax >= kMaxTableSize)
            return 0.0f;

        // Linear interpolation between adjacent samples
        float s0 = table[offset + idx0];
        float s1 = table[offset + idx1];
        return s0 + frac * (s1 - s0);
    }

    // Wavetable storage — fixed-size, no heap allocation
    float table[kMaxTableSize] {};

    // Table metadata
    int numFrames = 0;
    int frameSize = kDefaultFrameSize;

    // Playback state
    float phase = 0.0f;            // Current position within frame (in samples)
    float phaseIncrement = 0.0f;   // Samples to advance per output sample
    float position = 0.0f;         // Morph position [0, 1]
};

} // namespace xolokun
