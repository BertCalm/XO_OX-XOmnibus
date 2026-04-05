// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// FXChainHelpers — Shared utility header for XOceanus FX chain implementations.
//
// Eliminates boilerplate that repeats across 20+ chain headers:
//   - Parameter registration (registerFloat / registerBool / registerChoice)
//   - Parameter pointer caching (cacheParam)
//   - Dry/wet mix application (applyMix / applyMixMono)
//   - Lightweight oversampling wrapper (OversamplingProcessor<Factor>)
//   - Common DSP primitives:
//       SchmittTrigger    — hysteresis-based level detection
//       EnvelopeFollower  — attack/release one-pole follower
//       FractionalDelay   — Hermite interpolated delay line
//       CircularBuffer    — bidirectional read/write ring buffer
//   - Process-block guard macro (FX_CHAIN_PROCESS_GUARD)
//
// ALL code is inline — no .cpp linkage required beyond the stub.
// NO memory allocation in any process function.
// fastExp() used for envelope coefficients (matched-Z convention).
//==============================================================================

#include "../FastMath.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>

namespace xoceanus
{

//==============================================================================
// FX_CHAIN_PROCESS_GUARD
// Place at the very top of every processBlock() implementation.
// Prevents denormal numbers from triggering CPU microcode fallback inside
// the callback. Equivalent to juce::ScopedNoDenormals but self-documenting.
//==============================================================================
#define FX_CHAIN_PROCESS_GUARD juce::ScopedNoDenormals _noDenormals


//==============================================================================
// Parameter Registration Helpers
//
// These reduce a 4-line AudioParameterFloat creation + layout.add() call to a
// single readable line. All helpers use ParameterID{id, 1} version tagging
// matching the codebase convention. The ParameterLayout overload is used
// everywhere new chains are written (APVTS-first workflow).
//==============================================================================

/// Register a continuous float parameter (AudioParameterFloat) with a
/// NormalisableRange constructed from [min, max] with the given interval step.
/// Use for the vast majority of chain parameters.
inline void registerFloat(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                          const juce::String& id,
                          const juce::String& name,
                          float min, float max, float defaultVal,
                          float interval = 0.001f)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{id, 1},
        name,
        juce::NormalisableRange<float>(min, max, interval),
        defaultVal));
}

/// Register a continuous float parameter with a skew factor (for logarithmic
/// controls such as frequency and time). skewFactor < 1.0 biases the range
/// toward the low end (useful for cutoff / ms / Hz controls).
inline void registerFloatSkewed(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                                const juce::String& id,
                                const juce::String& name,
                                float min, float max, float defaultVal,
                                float interval = 0.001f,
                                float skewFactor = 0.3f)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{id, 1},
        name,
        juce::NormalisableRange<float>(min, max, interval, skewFactor),
        defaultVal));
}

/// Register a boolean toggle parameter (AudioParameterBool).
/// Use for bypass switches, freeze toggles, on/off modes, etc.
inline void registerBool(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                         const juce::String& id,
                         const juce::String& name,
                         bool defaultVal)
{
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{id, 1},
        name,
        defaultVal));
}

/// Register a choice/enum parameter (AudioParameterChoice).
/// Use for waveform selectors, mode switches, algorithm pickers, etc.
/// defaultIndex is 0-based into the choices StringArray.
inline void registerChoice(juce::AudioProcessorValueTreeState::ParameterLayout& layout,
                           const juce::String& id,
                           const juce::String& name,
                           const juce::StringArray& choices,
                           int defaultIndex)
{
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{id, 1},
        name,
        choices,
        defaultIndex));
}


//==============================================================================
// Parameter Caching Helper
//
// Fetches and returns the raw atomic<float>* from APVTS for a given paramId.
// Wraps the getRawParameterValue() call — lets cacheParameterPointers() stay
// readable even with 17+ params per chain.
//
// Usage:
//   p_drive = cacheParam(apvts, prefix + "drive");
//==============================================================================
inline std::atomic<float>* cacheParam(juce::AudioProcessorValueTreeState& apvts,
                                       const juce::String& paramId)
{
    return apvts.getRawParameterValue(paramId);
}


//==============================================================================
// Dry/Wet Mix Application
//
// Applies wet/dry mix in-place. mix = 0 → fully dry, mix = 1 → fully wet.
// The formula used is: out = dry + mix * (wet - dry)
//   = (1 - mix)*dry + mix*wet
// This is the constant-power-friendly linear crossfade used across the fleet.
// No allocation, no branches in the hot loop.
//==============================================================================

/// Stereo in-place wet/dry mix. dryL/dryR are both read (dry input) and
/// written (blended output). wetL/wetR supply the processed signal.
/// dryL must not alias wetL; dryR must not alias wetR.
inline void applyMix(float* dryL, float* dryR,
                     const float* wetL, const float* wetR,
                     float mix, int numSamples)
{
    const float dry = 1.0f - mix;
    for (int i = 0; i < numSamples; ++i)
    {
        dryL[i] = dry * dryL[i] + mix * wetL[i];
        dryR[i] = dry * dryR[i] + mix * wetR[i];
    }
}

/// Mono in-place wet/dry mix. dry is both read and written.
inline void applyMixMono(float* dry, const float* wet, float mix, int numSamples)
{
    const float dryGain = 1.0f - mix;
    for (int i = 0; i < numSamples; ++i)
        dry[i] = dryGain * dry[i] + mix * wet[i];
}


//==============================================================================
// OversamplingProcessor<Factor>
//
// Lightweight wrapper around juce::dsp::Oversampling<float> that:
//   - Pre-allocates upsampled buffers in prepare() (no runtime allocation)
//   - Exposes process() and processStereo() accepting a lambda for the DSP
//     running at the oversampled rate
//
// Factor must be 2, 4, or 8. Use the minimum factor that suppresses aliasing
// for your nonlinearity — more oversampling = more CPU.
//
// Template parameter is an int order: Factor = 2^order (2=1, 4=2, 8=3).
// juce::dsp::Oversampling uses the order count, so:
//   OversamplingProcessor<2>  → 2x (order 1)
//   OversamplingProcessor<4>  → 4x (order 2)
//   OversamplingProcessor<8>  → 8x (order 3)
//
// Usage:
//   OversamplingProcessor<4> ovs;
//   ovs.prepare(sampleRate, maxBlockSize);
//
//   // In processBlock:
//   ovs.processStereo(L, R, numSamples, [&](float* upL, float* upR, int upN)
//   {
//       for (int i = 0; i < upN; ++i)
//       {
//           upL[i] = softClip(upL[i] * drive);
//           upR[i] = softClip(upR[i] * drive);
//       }
//   });
//==============================================================================
template<int Factor>
class OversamplingProcessor
{
public:
    static_assert(Factor == 2 || Factor == 4 || Factor == 8,
                  "OversamplingProcessor Factor must be 2, 4, or 8.");

    OversamplingProcessor()
        : oversampler_(2,               // numChannels (stereo)
                       factorToOrder(), // oversamplingOrder (1=2x, 2=4x, 3=8x)
                       juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
                       true)            // isMaximumQuality
    {}

    /// Call from prepare(). Initialises the polyphase filter bank.
    /// Must be called before process() or processStereo().
    void prepare(double sampleRate, int maxBlockSize)
    {
        oversampler_.initProcessing(static_cast<size_t>(maxBlockSize));
        oversampler_.reset();
        (void)sampleRate; // juce::dsp::Oversampling derives from the JUCE block spec
    }

    /// Reset filter bank state (call from chain's reset()).
    void reset()
    {
        oversampler_.reset();
    }

    /// Process a mono buffer through fn at the oversampled rate.
    /// fn signature: void(float* upBuf, int upNumSamples)
    /// The buffer is modified in-place: input is upsampled → fn runs → downsampled back.
    template<typename ProcessFn>
    void process(float* buffer, int numSamples, ProcessFn&& fn)
    {
        // Wrap input buffer in a juce::dsp::AudioBlock<float>
        float* chans[2] = { buffer, buffer };
        juce::dsp::AudioBlock<float> inputBlock(chans, 1, static_cast<size_t>(numSamples));

        // Upsample
        auto upBlock = oversampler_.processSamplesUp(inputBlock);
        float* upBuf = upBlock.getChannelPointer(0);
        int    upN   = static_cast<int>(upBlock.getNumSamples());

        // Process at oversampled rate
        fn(upBuf, upN);

        // Downsample back into original buffer
        juce::dsp::AudioBlock<float> outputBlock(chans, 1, static_cast<size_t>(numSamples));
        oversampler_.processSamplesDown(outputBlock);
    }

    /// Process a stereo buffer pair through fn at the oversampled rate.
    /// fn signature: void(float* upL, float* upR, int upNumSamples)
    template<typename ProcessFn>
    void processStereo(float* bufferL, float* bufferR, int numSamples, ProcessFn&& fn)
    {
        float* chans[2] = { bufferL, bufferR };
        juce::dsp::AudioBlock<float> inputBlock(chans, 2, static_cast<size_t>(numSamples));

        auto upBlock = oversampler_.processSamplesUp(inputBlock);
        float* upL = upBlock.getChannelPointer(0);
        float* upR = upBlock.getChannelPointer(1);
        int    upN = static_cast<int>(upBlock.getNumSamples());

        fn(upL, upR, upN);

        juce::dsp::AudioBlock<float> outputBlock(chans, 2, static_cast<size_t>(numSamples));
        oversampler_.processSamplesDown(outputBlock);
    }

    /// Returns the oversampled sample rate (host rate × Factor).
    /// Use this when computing filter coefficients inside the lambda.
    double getOversampledRate(double hostSampleRate) const noexcept
    {
        return hostSampleRate * static_cast<double>(Factor);
    }

private:
    static constexpr int factorToOrder()
    {
        if (Factor == 2) return 1;
        if (Factor == 4) return 2;
        return 3; // Factor == 8
    }

    juce::dsp::Oversampling<float> oversampler_;
};


//==============================================================================
// SchmittTrigger
//
// Hysteresis-based level comparator. Avoids rapid toggling (chattering) near
// the threshold by requiring the signal to cross a lower threshold before it
// can re-trigger.
//
// Used by: AutoSwell (OnrushChain Stage 1), transient gates, beat detectors.
//
// Usage:
//   SchmittTrigger st;
//   st.setThresholds(0.1f, 0.2f);   // low=0.1, high=0.2
//   bool fired = st.process(sample); // true on rising-edge cross of high thresh
//==============================================================================
class SchmittTrigger
{
public:
    SchmittTrigger() = default;

    /// Set hysteresis thresholds. low < high always; caller must enforce.
    /// Signal must fall below low before a new high-crossing can trigger.
    void setThresholds(float low, float high) noexcept
    {
        threshLow_  = low;
        threshHigh_ = high;
    }

    /// Process one sample. Returns true on the rising edge (low→high crossing).
    /// Uses absolute value so the trigger responds to both polarities.
    bool process(float input) noexcept
    {
        const float level = std::abs(input);
        if (!armed_ && level < threshLow_)
            armed_ = true;
        if (armed_ && level >= threshHigh_)
        {
            armed_   = false;
            state_   = true;
            return true;
        }
        state_ = false;
        return false;
    }

    /// Returns the most recent trigger state without advancing.
    bool getState() const noexcept { return state_; }

    /// Reset to initial unarmed state.
    void reset() noexcept
    {
        armed_ = true; // armed = ready to detect a rising edge
        state_ = false;
    }

private:
    float threshLow_  = 0.1f;
    float threshHigh_ = 0.2f;
    bool  armed_      = true;
    bool  state_      = false;
};


//==============================================================================
// EnvelopeFollower
//
// Classic one-pole attack/release RMS-like follower for dynamics processing
// and envelope-controlled filters.
//
// Coefficients use matched-Z (fastExp(-1/(τ·sr))) — the XOceanus fleet
// standard per the CLAUDE.md critical patterns. Never use the Euler
// approximation τ/(τ+1) here.
//
// Usage:
//   EnvelopeFollower env;
//   env.prepare(sampleRate);
//   env.setAttack(5.0f);    // ms
//   env.setRelease(50.0f);  // ms
//   float level = env.process(sample); // 0..+∞ (abs envelope)
//==============================================================================
class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;

    /// Must be called before use. Resets state and pre-computes coefficients.
    void prepare(double sampleRate) noexcept
    {
        sr_      = static_cast<float>(sampleRate);
        state_   = 0.0f;
        // default: 5ms attack, 50ms release
        setAttack(5.0f);
        setRelease(50.0f);
    }

    /// Set attack time in milliseconds. Updates coefficient immediately.
    void setAttack(float ms) noexcept
    {
        attackCoeff_ = (ms > 0.0f && sr_ > 0.0f)
            ? fastExp(-1.0f / (ms * 0.001f * sr_))
            : 0.0f;
    }

    /// Set release time in milliseconds. Updates coefficient immediately.
    void setRelease(float ms) noexcept
    {
        releaseCoeff_ = (ms > 0.0f && sr_ > 0.0f)
            ? fastExp(-1.0f / (ms * 0.001f * sr_))
            : 0.0f;
    }

    /// Process one sample. Returns the envelope value (always ≥ 0).
    /// State is maintained between calls — do not interleave channels.
    float process(float input) noexcept
    {
        const float absIn = std::abs(input);
        const float coeff = (absIn > state_) ? attackCoeff_ : releaseCoeff_;
        state_ = flushDenormal(state_ * coeff + absIn * (1.0f - coeff));
        return state_;
    }

    /// Peek at current envelope level without advancing state.
    float getLevel() const noexcept { return state_; }

    /// Hard-reset the envelope state to zero.
    void reset() noexcept { state_ = 0.0f; }

private:
    float sr_           = 44100.0f;
    float state_        = 0.0f;
    float attackCoeff_  = 0.0f;
    float releaseCoeff_ = 0.0f;
};


//==============================================================================
// FractionalDelay
//
// Single-channel delay line with Hermite cubic interpolation for sub-sample
// accuracy. Suitable for pitch-shifting readheads, chorus, vibrato, and
// BBD-style tape delay.
//
// Memory is pre-allocated in prepare() — no runtime heap calls in write/read.
// All operations wrap correctly via modular arithmetic.
//
// Usage:
//   FractionalDelay del;
//   del.prepare(static_cast<int>(sampleRate * 2.0)); // 2s max
//   del.write(sample);
//   float out = del.read(delaySamples); // fractional okay
//==============================================================================
class FractionalDelay
{
public:
    FractionalDelay() = default;

    /// Allocate the internal buffer. Call from prepare(), not from the audio thread.
    void prepare(int maxDelaySamples)
    {
        buf_.assign(static_cast<size_t>(maxDelaySamples + 4), 0.0f);
        writePos_ = 0;
        size_     = maxDelaySamples + 4;
    }

    /// Write one sample into the buffer and advance the write pointer.
    void write(float sample) noexcept
    {
        buf_[writePos_] = flushDenormal(sample);
        if (++writePos_ >= size_)
            writePos_ = 0;
    }

    /// Read with Hermite cubic interpolation.
    /// delaySamples must be in [1, size-2] for safe four-tap access.
    float read(float delaySamples) const noexcept
    {
        // Compute read position relative to current writePos
        float readF = static_cast<float>(writePos_) - delaySamples;
        while (readF <  0.0f) readF += static_cast<float>(size_);
        while (readF >= static_cast<float>(size_)) readF -= static_cast<float>(size_);

        const int i1 = static_cast<int>(readF);
        const int i0 = (i1 - 1 + size_) % size_;
        const int i2 = (i1 + 1) % size_;
        const int i3 = (i1 + 2) % size_;
        const float t  = readF - static_cast<float>(i1);
        const float a  = buf_[i0];
        const float b  = buf_[i1];
        const float c  = buf_[i2];
        const float d  = buf_[i3];
        // Standard Hermite cubic: (Catmull-Rom variant)
        return b + 0.5f * t * (c - a + t * (2.0f * a - 5.0f * b + 4.0f * c - d
                                            + t * (3.0f * (b - c) + d - a)));
    }

    /// Read at an integer offset with no interpolation (faster path).
    float readInteger(int delaySamples) const noexcept
    {
        int pos = writePos_ - delaySamples;
        while (pos <  0)     pos += size_;
        while (pos >= size_) pos -= size_;
        return buf_[pos];
    }

    /// Zero all buffer contents and reset the write position.
    void clear() noexcept
    {
        std::fill(buf_.begin(), buf_.end(), 0.0f);
        writePos_ = 0;
    }

    /// Returns the allocated buffer size (including guard samples).
    int getSize() const noexcept { return size_; }

private:
    std::vector<float> buf_;
    int writePos_ = 0;
    int size_     = 0;
};


//==============================================================================
// CircularBuffer
//
// Bidirectional single-channel ring buffer for reverse playback, granular
// windows, and freeze loops. Supports forward and backward reads from any
// offset relative to the write head.
//
// readForward(0)  returns the most recently written sample.
// readForward(n)  returns the sample written n steps ago (older).
// readBackward(n) reads going in the opposite direction from the head —
//                 useful for reverse granular and reverse delay effects.
//
// Usage:
//   CircularBuffer cb;
//   cb.prepare(static_cast<int>(sampleRate)); // 1s window
//   cb.write(sample);
//   float fwd  = cb.readForward(512);   // 512 samples back in time
//   float bkwd = cb.readBackward(512);  // 512 samples in reverse direction
//==============================================================================
class CircularBuffer
{
public:
    CircularBuffer() = default;

    /// Allocate the buffer. Call from prepare(), not the audio thread.
    void prepare(int maxSamples)
    {
        buf_.assign(static_cast<size_t>(maxSamples), 0.0f);
        writePos_ = 0;
        size_     = maxSamples;
    }

    /// Write one sample and advance the write pointer.
    void write(float sample) noexcept
    {
        if (size_ <= 0) return;
        buf_[writePos_] = flushDenormal(sample);
        if (++writePos_ >= size_)
            writePos_ = 0;
    }

    /// Read offset samples behind the write head (forward chronological order).
    /// offset=0 → most recently written sample.
    float readForward(int offset) const noexcept
    {
        if (size_ <= 0) return 0.0f;
        int pos = writePos_ - 1 - offset;
        while (pos <  0)     pos += size_;
        while (pos >= size_) pos -= size_;
        return buf_[pos];
    }

    /// Read in the reverse direction from the write head.
    /// offset=0 → oldest sample in the most-recently-filled half of the buffer.
    float readBackward(int offset) const noexcept
    {
        if (size_ <= 0) return 0.0f;
        int pos = writePos_ + offset;
        while (pos >= size_) pos -= size_;
        while (pos <  0)     pos += size_;
        return buf_[pos];
    }

    /// Returns the logical buffer size (number of samples available).
    int getSize() const noexcept { return size_; }

    /// Returns the current write position (useful for phase calculations).
    int getWritePos() const noexcept { return writePos_; }

    /// Zero all samples and reset the write pointer.
    void clear() noexcept
    {
        std::fill(buf_.begin(), buf_.end(), 0.0f);
        writePos_ = 0;
    }

private:
    std::vector<float> buf_;
    int writePos_ = 0;
    int size_     = 0;
};

} // namespace xoceanus
