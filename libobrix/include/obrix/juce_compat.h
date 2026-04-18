// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
//
// juce_compat.h — Minimal JUCE type shims for compiling ObrixEngine.h
// without the JUCE framework. Only the API surface actually used by
// ObrixEngine is implemented. This is NOT a general JUCE replacement.
//
// Shimmed types:
//   juce::AudioBuffer<float>           — getWritePointer, addSample, clear, getNumChannels
//   juce::MidiBuffer + MidiMessage     — range-for iteration, note/CC/pitch queries
//   juce::ScopedNoDenormals            — ARM64 FPCR flush-to-zero
//   juce::jlimit                       — clamp
//   juce::String                       — thin std::string wrapper
//   juce::Colour                       — ARGB colour
//   juce::AudioProcessorValueTreeState — getRawParameterValue only
//   juce::RangedAudioParameter family  — stubs for addParameters() compilation
//   juce::NormalisableRange<float>     — range description
//   juce::StringArray                  — initializer-list string vector
//   juce::ParameterID                  — ID + version pair
//   juce::roundToInt                   — int rounding

#pragma once

// x86 intrinsics must be included before use in ScopedNoDenormals
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#include <xmmintrin.h>
#endif

#include <atomic>
#include <cmath>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace juce
{

//==============================================================================
// juce::String — thin wrapper around std::string
//==============================================================================
class String
{
public:
    String() = default;
    String(const char* s) : str_(s ? s : "") {}
    String(const std::string& s) : str_(s) {}
    String(std::string&& s) : str_(std::move(s)) {}

    operator const std::string&() const { return str_; }
    const char* toRawUTF8() const { return str_.c_str(); }
    bool isEmpty() const { return str_.empty(); }
    bool operator==(const String& o) const { return str_ == o.str_; }
    bool operator!=(const String& o) const { return str_ != o.str_; }

private:
    std::string str_;
};

//==============================================================================
// juce::StringArray
//==============================================================================
class StringArray
{
public:
    StringArray() = default;
    StringArray(std::initializer_list<const char*> init)
    {
        for (auto* s : init)
            strings_.emplace_back(s);
    }
    int size() const { return static_cast<int>(strings_.size()); }
    const String& operator[](int i) const { return strings_[static_cast<size_t>(i)]; }

private:
    std::vector<String> strings_;
};

//==============================================================================
// juce::Colour
//==============================================================================
class Colour
{
public:
    Colour() = default;
    explicit Colour(uint32_t argb)
        : r_(static_cast<uint8_t>((argb >> 16) & 0xFF)),
          g_(static_cast<uint8_t>((argb >> 8) & 0xFF)),
          b_(static_cast<uint8_t>(argb & 0xFF)),
          a_(static_cast<uint8_t>((argb >> 24) & 0xFF)) {}

    uint8_t getRed()   const { return r_; }
    uint8_t getGreen() const { return g_; }
    uint8_t getBlue()  const { return b_; }
    uint8_t getAlpha() const { return a_; }

private:
    uint8_t r_ = 0, g_ = 0, b_ = 0, a_ = 255;
};

//==============================================================================
// juce::NormalisableRange<float>
//==============================================================================
template <typename T>
struct NormalisableRange
{
    T start, end, interval;
    T skewFactor;

    NormalisableRange() : start(0), end(1), interval(0), skewFactor(1) {}

    NormalisableRange(T start_, T end_, T interval_ = T(0), T skew_ = T(1))
        : start(start_), end(end_), interval(interval_), skewFactor(skew_) {}
};

//==============================================================================
// juce::ParameterID
//==============================================================================
struct ParameterID
{
    std::string parameterID;
    int versionHint;

    ParameterID(const char* id, int version) : parameterID(id), versionHint(version) {}
    ParameterID(const std::string& id, int version) : parameterID(id), versionHint(version) {}

    const std::string& getParamID() const { return parameterID; }
};

//==============================================================================
// juce::RangedAudioParameter family — minimal stubs for addParameters()
//==============================================================================
class RangedAudioParameter
{
public:
    virtual ~RangedAudioParameter() = default;
    ParameterID paramID{"", 0};

    RangedAudioParameter(const ParameterID& id) : paramID(id) {}
};

class AudioParameterFloat : public RangedAudioParameter
{
public:
    AudioParameterFloat(const ParameterID& id, const char*, NormalisableRange<float>, float)
        : RangedAudioParameter(id) {}
    AudioParameterFloat(const ParameterID& id, const String&, NormalisableRange<float>, float)
        : RangedAudioParameter(id) {}
};

class AudioParameterChoice : public RangedAudioParameter
{
public:
    AudioParameterChoice(const ParameterID& id, const char*, const StringArray&, int)
        : RangedAudioParameter(id) {}
    AudioParameterChoice(const ParameterID& id, const String&, const StringArray&, int)
        : RangedAudioParameter(id) {}
};

class AudioParameterBool : public RangedAudioParameter
{
public:
    AudioParameterBool(const ParameterID& id, const char*, bool)
        : RangedAudioParameter(id) {}
    AudioParameterBool(const ParameterID& id, const String&, bool)
        : RangedAudioParameter(id) {}
};

//==============================================================================
// juce::AudioProcessorValueTreeState — minimal shim
//
// Only getRawParameterValue() is used by ObrixEngine::attachParameters().
// The actual storage is backed by ObrixParamStore (external).
//==============================================================================
class AudioProcessorValueTreeState
{
public:
    using ParameterLayout = int; // Unused stub type

    void registerParam(const std::string& id, std::atomic<float>* ptr)
    {
        params_[id] = ptr;
    }

    std::atomic<float>* getRawParameterValue(const char* id) const
    {
        auto it = params_.find(id);
        return it != params_.end() ? it->second : nullptr;
    }

    std::atomic<float>* getRawParameterValue(const std::string& id) const
    {
        return getRawParameterValue(id.c_str());
    }

private:
    std::unordered_map<std::string, std::atomic<float>*> params_;
};

//==============================================================================
// juce::AudioBuffer<float> — lightweight non-owning stereo buffer
//==============================================================================
template <typename SampleType>
class AudioBuffer
{
public:
    AudioBuffer() = default;

    void setDataToReferTo(SampleType** channelData, int numChannels, int numSamples)
    {
        channels_ = channelData;
        numChannels_ = numChannels;
        numSamples_ = numSamples;
    }

    int getNumChannels() const { return numChannels_; }
    int getNumSamples() const { return numSamples_; }

    SampleType* getWritePointer(int ch) { return channels_[ch]; }
    const SampleType* getReadPointer(int ch) const { return channels_[ch]; }

    void addSample(int channel, int sampleIndex, SampleType value)
    {
        channels_[channel][sampleIndex] += value;
    }

    void clear()
    {
        for (int ch = 0; ch < numChannels_; ++ch)
            std::memset(channels_[ch], 0, sizeof(SampleType) * static_cast<size_t>(numSamples_));
    }

    void clear(int startSample, int numSamplesToClear)
    {
        for (int ch = 0; ch < numChannels_; ++ch)
            std::memset(channels_[ch] + startSample, 0,
                        sizeof(SampleType) * static_cast<size_t>(numSamplesToClear));
    }

private:
    SampleType** channels_ = nullptr;
    int numChannels_ = 0;
    int numSamples_ = 0;
};

//==============================================================================
// juce::MidiMessage — read-only MIDI message
//==============================================================================
class MidiMessage
{
public:
    MidiMessage() = default;
    MidiMessage(uint8_t b0, uint8_t b1, uint8_t b2)
        : data_{b0, b1, b2}, size_(3) {}
    MidiMessage(uint8_t b0, uint8_t b1)
        : data_{b0, b1, 0}, size_(2) {}

    bool isNoteOn()  const { return size_ >= 3 && (data_[0] & 0xF0) == 0x90 && data_[2] > 0; }
    bool isNoteOff() const { return size_ >= 3 && ((data_[0] & 0xF0) == 0x80 || ((data_[0] & 0xF0) == 0x90 && data_[2] == 0)); }
    bool isController() const { return size_ >= 3 && (data_[0] & 0xF0) == 0xB0; }
    bool isChannelPressure() const { return size_ >= 2 && (data_[0] & 0xF0) == 0xD0; }
    bool isPitchWheel() const { return size_ >= 3 && (data_[0] & 0xF0) == 0xE0; }
    bool isAftertouch() const { return size_ >= 3 && (data_[0] & 0xF0) == 0xA0; }
    bool isAllNotesOff() const { return isController() && getControllerNumber() == 123; }
    bool isAllSoundOff() const { return isController() && getControllerNumber() == 120; }

    int getNoteNumber() const { return data_[1]; }
    int getVelocity() const { return data_[2]; }
    float getFloatVelocity() const { return static_cast<float>(data_[2]) / 127.0f; }
    int getChannel() const { return (data_[0] & 0x0F) + 1; }
    int getControllerNumber() const { return data_[1]; }
    int getControllerValue() const { return data_[2]; }
    int getChannelPressureValue() const { return data_[1]; }
    int getAfterTouchValue() const { return data_[2]; } // Polyphonic aftertouch: note=data1, value=data2

    int getPitchWheelValue() const
    {
        return data_[1] | (data_[2] << 7); // 14-bit, 0..16383
    }

private:
    uint8_t data_[3] = {};
    int size_ = 0;
};

//==============================================================================
// juce::MidiBuffer — iterable MIDI event container
//==============================================================================
class MidiBuffer
{
public:
    struct Event
    {
        MidiMessage message;
        int samplePosition;

        const MidiMessage& getMessage() const { return message; }
    };

    // Range-for support: `for (const auto md : midi)`
    class Iterator
    {
    public:
        Iterator(const Event* ptr) : ptr_(ptr) {}
        const Event& operator*() const { return *ptr_; }
        Iterator& operator++() { ++ptr_; return *this; }
        bool operator!=(const Iterator& o) const { return ptr_ != o.ptr_; }

    private:
        const Event* ptr_;
    };

    Iterator begin() const { return Iterator(events_.data()); }
    Iterator end() const { return Iterator(events_.data() + events_.size()); }

    bool isEmpty() const { return events_.empty(); }
    int getNumEvents() const { return static_cast<int>(events_.size()); }

    void clear() { events_.clear(); }

    void addEvent(const MidiMessage& msg, int samplePosition)
    {
        events_.push_back({msg, samplePosition});
    }

    // Convenience: add raw MIDI bytes
    void addNoteOn(int channel, int note, float velocity, int samplePos)
    {
        uint8_t vel = static_cast<uint8_t>(velocity * 127.0f);
        addEvent(MidiMessage(static_cast<uint8_t>(0x90 | (channel - 1)),
                             static_cast<uint8_t>(note), vel), samplePos);
    }

    void addNoteOff(int channel, int note, int samplePos)
    {
        addEvent(MidiMessage(static_cast<uint8_t>(0x80 | (channel - 1)),
                             static_cast<uint8_t>(note), 0), samplePos);
    }

    void addAllNotesOff(int channel, int samplePos)
    {
        addEvent(MidiMessage(static_cast<uint8_t>(0xB0 | (channel - 1)),
                             123, 0), samplePos);
    }

private:
    std::vector<Event> events_;
};

//==============================================================================
// juce::ScopedNoDenormals — platform-specific denormal flushing
//==============================================================================
class ScopedNoDenormals
{
public:
    ScopedNoDenormals()
    {
#if defined(__aarch64__) || defined(_M_ARM64)
        // ARM64: set FZ (flush-to-zero) bit in FPCR
        uint64_t fpcr;
        asm volatile("mrs %0, fpcr" : "=r"(fpcr));
        oldFpcr_ = fpcr;
        fpcr |= (1ULL << 24); // FZ bit
        asm volatile("msr fpcr, %0" : : "r"(fpcr));
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
        // x86: set FTZ and DAZ bits in MXCSR
        oldMxcsr_ = _mm_getcsr();
        _mm_setcsr(oldMxcsr_ | 0x8040); // FTZ (bit 15) + DAZ (bit 6)
#endif
    }

    ~ScopedNoDenormals()
    {
#if defined(__aarch64__) || defined(_M_ARM64)
        asm volatile("msr fpcr, %0" : : "r"(oldFpcr_));
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
        _mm_setcsr(oldMxcsr_);
#endif
    }

    ScopedNoDenormals(const ScopedNoDenormals&) = delete;
    ScopedNoDenormals& operator=(const ScopedNoDenormals&) = delete;

private:
#if defined(__aarch64__) || defined(_M_ARM64)
    uint64_t oldFpcr_ = 0;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
    unsigned int oldMxcsr_ = 0;
#endif
};

//==============================================================================
// Free functions
//==============================================================================

/// juce::jlimit — clamp value to [lower, upper]
template <typename T>
inline T jlimit(T lower, T upper, T value)
{
    return value < lower ? lower : (value > upper ? upper : value);
}

/// juce::roundToInt
inline int roundToInt(float value) { return static_cast<int>(std::round(value)); }
inline int roundToInt(double value) { return static_cast<int>(std::round(value)); }

} // namespace juce
