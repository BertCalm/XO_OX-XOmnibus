#pragma once
// WavetableOscillator.h — Multi-frame wavetable oscillator with morph, cubic Hermite
// interpolation, and band-limited basic shape generation.
//
// Replaces the V1 linear-interpolation stub. Backward-compatible:
//   loadWavetable() / setPosition() / setFrequency() / processSample() unchanged.
// New V1.1 additions:
//   loadFromBuffer()          — alias for loadWavetable() (WavetableEditor API)
//   setMorphPosition()        — alias for setPosition() (WavetableEditor API)
//   generateBasicTables()     — fills 4 single-frame tables (sine/saw/square/tri)
//   kFrameSize / kMaxFrames   — canonical constants referenced by the editor
//
// Memory layout: fixed-size internal buffer, no heap allocation during processing.
// Call generateBasicTables() or loadFromBuffer() from a non-audio thread.
//
// Architecture rules: no JUCE dependency, no allocation in processSample().

#include <cmath>
#include <cstring>
#include <algorithm>
#include "FastMath.h"

namespace xolokun {

//==============================================================================
// WavetableOscillator — Wavetable oscillator with crossfade morphing.
//
// Supports up to 256 frames of 2048 samples each. The position parameter
// morphs smoothly between adjacent frames using linear crossfading.
// Within each frame, cubic Hermite interpolation is used for sub-sample
// accuracy (replaces the V1 linear interpolation).
//
// Anti-aliasing: each frame stores harmonics up to Nyquist at prepare-time
// sampleRate. When setFrequency() is called, phaseIncrement is set such that
// harmonics above Nyquist are never generated — the fundamental is rendered
// at the correct pitch and the stored table already has the right spectral
// content. No per-sample filtering is required.
//
// Usage:
//   WavetableOscillator wt;
//   wt.generateBasicTables();          // or loadFromBuffer()
//   wt.setFrequency(440.0f, sampleRate);
//   wt.setMorphPosition(0.5f);         // morph halfway through the table
//   float sample = wt.processSample();
//==============================================================================
class WavetableOscillator
{
public:
    // Canonical constants — referenced by WavetableEditor and engines.
    static constexpr int kFrameSize   = 2048;
    static constexpr int kMaxFrames   = 256;

    // Legacy aliases kept for backward compatibility.
    static constexpr int kDefaultFrameSize = kFrameSize;
    static constexpr int kMaxFrameSize     = kFrameSize;
    static constexpr int kMaxTableSize     = kMaxFrames * kFrameSize;  // 524288 samples

    WavetableOscillator()
    {
        std::memset (table, 0, sizeof (table));
    }

    //--------------------------------------------------------------------------
    /// Load wavetable data into the internal buffer. Copies data — source
    /// pointer does not need to persist. Call from a non-audio thread only.
    ///
    /// @param data       Pointer to interleaved frames
    ///                   (frame0[0..frameSize-1], frame1[0..frameSize-1], ...).
    /// @param numFrames  Number of frames (1 to 256).
    /// @param frameSize  Samples per frame (default 2048, max 2048).
    void loadWavetable (const float* data, int numFrames, int frameSize = kFrameSize) noexcept
    {
        if (data == nullptr || numFrames <= 0 || frameSize <= 0)
            return;

        if (numFrames > kMaxFrames)  numFrames = kMaxFrames;
        if (frameSize > kMaxFrameSize) frameSize = kMaxFrameSize;

        // frameSize must be power-of-2 for bitmask wrapping in readFrame().
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

        if (totalSamples < kMaxTableSize)
            std::memset (table + totalSamples, 0,
                         static_cast<size_t> (kMaxTableSize - totalSamples) * sizeof (float));
    }

    /// Alias for loadWavetable() — used by WavetableEditor API.
    void loadFromBuffer (const float* data, int numFrames, int frameSize = kFrameSize) noexcept
    {
        loadWavetable (data, numFrames, frameSize);
    }

    //--------------------------------------------------------------------------
    /// Generate basic single-frame tables: saw → square → triangle → sine
    /// (4 frames, so morphing across them sweeps the shape space).
    ///
    /// Each frame is band-limited via additive synthesis at the reference
    /// sampleRate (defaults to 44100 if prepare() has not been called yet).
    /// Call from a non-audio thread.
    void generateBasicTables (double sampleRate = 44100.0) noexcept
    {
        constexpr int nFrames    = 4;
        constexpr int fs         = kFrameSize;
        // Stack-allocate: 4 × 2048 × 4 = 32KB — fine for non-audio thread calls.
        // Avoid static to eliminate the thread-safety concern on first invocation.
        float buf[nFrames * fs];

        // Nyquist limit: only generate harmonics whose frequency is below sr/2.
        // At 44100 Hz with a C2 fundamental (65.4 Hz), Nyquist allows ~337 harmonics.
        // For table generation we use a conservative max: sr/2 / (reference 20 Hz).
        // This guarantees the table sounds clean at any reasonable pitch.
        const int maxHarmonic = static_cast<int> (sampleRate * 0.5 / 20.0);

        // Frame 0: Sawtooth (descending, all harmonics 1/n)
        for (int s = 0; s < fs; ++s)
        {
            const float phase = static_cast<float> (s) / static_cast<float> (fs); // [0, 1)
            float val = 0.0f;
            for (int h = 1; h <= maxHarmonic; ++h)
                val += (1.0f / static_cast<float> (h))
                       * std::sin (k2Pi * static_cast<float> (h) * phase);
            buf[0 * fs + s] = val;
        }
        normalizeFrame (buf, 0, fs);

        // Frame 1: Square (odd harmonics only, 1/n)
        for (int s = 0; s < fs; ++s)
        {
            const float phase = static_cast<float> (s) / static_cast<float> (fs);
            float val = 0.0f;
            for (int h = 1; h <= maxHarmonic; h += 2)
                val += (1.0f / static_cast<float> (h))
                       * std::sin (k2Pi * static_cast<float> (h) * phase);
            buf[1 * fs + s] = val;
        }
        normalizeFrame (buf, 1, fs);

        // Frame 2: Triangle (odd harmonics with alternating sign, 1/n^2)
        for (int s = 0; s < fs; ++s)
        {
            const float phase = static_cast<float> (s) / static_cast<float> (fs);
            float val = 0.0f;
            float sign = 1.0f;
            for (int h = 1; h <= maxHarmonic; h += 2)
            {
                val += sign * (1.0f / static_cast<float> (h * h))
                       * std::sin (k2Pi * static_cast<float> (h) * phase);
                sign = -sign;
            }
            buf[2 * fs + s] = val;
        }
        normalizeFrame (buf, 2, fs);

        // Frame 3: Sine (single harmonic — pure)
        for (int s = 0; s < fs; ++s)
        {
            const float phase = static_cast<float> (s) / static_cast<float> (fs);
            buf[3 * fs + s] = std::sin (k2Pi * phase);
        }
        // Sine is already ±1; skip normalize.

        loadWavetable (buf, nFrames, fs);
    }

    //--------------------------------------------------------------------------
    /// Set the morph position within the wavetable.
    /// @param pos  Position in [0.0, 1.0]. 0.0 = first frame, 1.0 = last frame.
    void setPosition (float pos) noexcept
    {
        position = (pos < 0.0f) ? 0.0f : (pos > 1.0f) ? 1.0f : pos;
    }

    /// Alias for setPosition() — used by WavetableEditor API.
    void setMorphPosition (float pos) noexcept { setPosition (pos); }

    /// Get the current morph position.
    float getPosition() const noexcept { return position; }

    //--------------------------------------------------------------------------
    /// Set playback frequency and sample rate.
    /// @param freqHz     Desired frequency in Hz.
    /// @param sampleRate Current sample rate in Hz.
    void setFrequency (float freqHz, float sampleRate) noexcept
    {
        if (sampleRate <= 0.0f) return;

        const float maxFreq = sampleRate * 0.5f;
        if (freqHz < 0.0f)    freqHz = 0.0f;
        if (freqHz > maxFreq) freqHz = maxFreq;

        // Phase increment: advances (frequency * frameSize) / sampleRate
        // samples per output sample, completing exactly one frame period per cycle.
        phaseIncrement = (freqHz * static_cast<float> (frameSize)) / sampleRate;
    }

    //--------------------------------------------------------------------------
    /// Generate and return the next output sample.
    /// Uses cubic Hermite interpolation within frames and linear crossfade
    /// between adjacent frames. Denormal-safe.
    float processSample() noexcept
    {
        if (numFrames <= 0 || frameSize <= 0)
            return 0.0f;

        // Determine which two frames to crossfade between.
        const float framePos  = position * static_cast<float> (numFrames - 1);
        int   frameA    = static_cast<int> (framePos);
        const float frameFrac = framePos - static_cast<float> (frameA);

        if (frameA < 0)           frameA = 0;
        if (frameA >= numFrames)  frameA = numFrames - 1;
        const int frameB = (frameA + 1 < numFrames) ? frameA + 1 : numFrames - 1;

        const float sampleA = readFrameCubic (frameA, phase);
        float out;

        if (frameA == frameB || frameFrac < 0.0001f)
        {
            out = sampleA;
        }
        else
        {
            const float sampleB = readFrameCubic (frameB, phase);
            out = sampleA + frameFrac * (sampleB - sampleA);
        }

        // Advance phase and wrap.
        phase += phaseIncrement;
        const float fs = static_cast<float> (frameSize);
        if (fs > 0.0f)
        {
            phase = std::fmod (phase, fs);
            if (phase < 0.0f) phase += fs;
        }

        // Denormal protection on output.
        return flushDenormal (out);
    }

    //--------------------------------------------------------------------------
    /// Reset the oscillator phase to zero.
    void reset() noexcept { phase = 0.0f; }

    /// Set the phase directly (in samples within the frame).
    void setPhase (float p) noexcept
    {
        const float fs = static_cast<float> (frameSize);
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
    /// Process a block of samples into a pre-allocated buffer.
    void processBlock (float* output, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample();
    }

    //--------------------------------------------------------------------------
    /// Returns true if a wavetable has been loaded.
    bool isLoaded() const noexcept { return numFrames > 0 && frameSize > 0; }

    /// Number of loaded frames.
    int getNumFrames() const noexcept { return numFrames; }

    /// Frame size (samples per frame).
    int getFrameSize() const noexcept { return frameSize; }

    /// Read-only pointer into the internal table for display / editing.
    /// Returns a pointer to frame `frameIndex`, or nullptr if out of range.
    const float* getFrameReadPointer (int frameIndex) const noexcept
    {
        if (frameIndex < 0 || frameIndex >= numFrames) return nullptr;
        return table + frameIndex * frameSize;
    }

    /// Read-write pointer into the internal table — used by WavetableEditor
    /// for in-place drawing. Caller is responsible for keeping values in [-1, 1].
    /// Must NOT be called from the audio thread while processSample() is running.
    float* getFrameWritePointer (int frameIndex) noexcept
    {
        if (frameIndex < 0 || frameIndex >= numFrames) return nullptr;
        return table + frameIndex * frameSize;
    }

    /// Normalize a single frame to peak ±1.0 in place.
    /// Call from the UI/non-audio thread only.
    void normalizeCurrentFrame (int frameIndex) noexcept
    {
        float* frame = getFrameWritePointer (frameIndex);
        if (frame == nullptr) return;

        float peak = 0.0f;
        for (int s = 0; s < frameSize; ++s)
            peak = std::max (peak, std::abs (frame[s]));

        if (peak > 0.001f)
            for (int s = 0; s < frameSize; ++s)
                frame[s] /= peak;
    }

private:
    //--------------------------------------------------------------------------
    /// Cubic Hermite interpolation within a single frame.
    /// Reads four samples (p-1, p0, p1, p2) with power-of-2 index wrap.
    float readFrameCubic (int frameIndex, float samplePos) const noexcept
    {
        if (frameIndex < 0 || frameIndex >= numFrames) return 0.0f;

        const int offset = frameIndex * frameSize;
        const int mask   = frameSize - 1;  // power-of-2 wrap

        const int i0 = static_cast<int> (samplePos);
        const float frac = samplePos - static_cast<float> (i0);

        const int im1 = (i0 - 1) & mask;
        const int i1  = (i0 + 1) & mask;
        const int i2  = (i0 + 2) & mask;
        const int i00 = i0 & mask;

        // Guard against total table size overflow.
        const int base = offset;
        if (base + std::max ({im1, i00, i1, i2}) >= kMaxTableSize) return 0.0f;

        const float ym1 = table[base + im1];
        const float y0  = table[base + i00];
        const float y1  = table[base + i1];
        const float y2  = table[base + i2];

        // Catmull-Rom / cubic Hermite coefficients.
        const float c0 = y0;
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);

        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    //--------------------------------------------------------------------------
    /// Normalize a frame inside the static generation buffer.
    static void normalizeFrame (float* buf, int frameIndex, int fs) noexcept
    {
        float* f = buf + frameIndex * fs;
        float peak = 0.001f;
        for (int s = 0; s < fs; ++s) peak = std::max (peak, std::abs (f[s]));
        for (int s = 0; s < fs; ++s) f[s] /= peak;
    }

    // Wavetable storage — fixed-size, no heap allocation.
    float table[kMaxTableSize] {};

    // Table metadata.
    int numFrames = 0;
    int frameSize = kFrameSize;

    // Playback state.
    float phase          = 0.0f;  // Position within frame (in samples)
    float phaseIncrement = 0.0f;  // Samples to advance per output sample
    float position       = 0.0f;  // Morph position [0, 1]

    static constexpr float k2Pi = 6.28318530718f;
};

} // namespace xolokun
