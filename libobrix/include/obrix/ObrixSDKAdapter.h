// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
//
// ObrixSDKAdapter — Public C++ interface for the Obrix engine.
// Uses pimpl to keep JUCE types completely out of the public header.
// The JNI bridge and test code include only this header.
//
// NO JUCE types, NO SDK types — just plain C++17.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace obrix
{

/// A single MIDI event for the render call.
struct MidiEvent
{
    int sampleOffset = 0;
    uint8_t data[3] = {};
    int numBytes = 0;
};

/// Parameter definition.
struct ParamInfo
{
    std::string id;
    float defaultValue;
};

/// Portable Obrix engine wrapper. Thread-safe parameter access.
class ObrixSDKAdapter
{
public:
    ObrixSDKAdapter();
    ~ObrixSDKAdapter();

    // Non-copyable, non-movable (owns audio state)
    ObrixSDKAdapter(const ObrixSDKAdapter&) = delete;
    ObrixSDKAdapter& operator=(const ObrixSDKAdapter&) = delete;

    //--- Lifecycle ---
    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    //--- Audio (called from audio thread) ---
    /// Render audio into left/right buffers (additive). Buffers must be pre-zeroed.
    /// midiEvents can be nullptr if numMidiEvents == 0.
    void renderBlock(float* left, float* right, int numSamples,
                     const MidiEvent* midiEvents, int numMidiEvents);

    //--- Parameters (thread-safe, lock-free) ---
    bool setParameter(const std::string& id, float value);
    float getParameter(const std::string& id) const;
    std::vector<ParamInfo> getParameterList() const;
    int getParameterCount() const;

    //--- Identity ---
    const char* getEngineId() const { return "Obrix"; }
    int getMaxVoices() const { return 8; }
    int getActiveVoiceCount() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace obrix
