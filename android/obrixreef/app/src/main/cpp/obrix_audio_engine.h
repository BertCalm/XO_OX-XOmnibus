// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
//
// ObrixAudioEngine — Oboe AudioStreamCallback that drives ObrixSDKAdapter.
// This is the Android equivalent of ObrixPocket's ObrixProcessorAdapter + ObrixBridge.
//
// Architecture:
//   UI thread  --> noteOn/noteOff/setParameter --> lock-free queues
//   Audio thread <-- onAudioReady() <-- Oboe callback
//     1. Drain param queue
//     2. Drain MIDI queue
//     3. engine_.renderBlock()
//     4. Write to Oboe output buffer
#pragma once

#include "obrix_param_queue.h"
#include "obrix/ObrixSDKAdapter.h"

#include <oboe/Oboe.h>

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace obrix
{

class ObrixAudioEngine : public oboe::AudioStreamCallback
{
public:
    ObrixAudioEngine();
    ~ObrixAudioEngine();

    //--- Lifecycle (UI thread) ---
    bool start();
    void stop();
    bool isRunning() const { return running_.load(std::memory_order_relaxed); }

    //--- Input (UI thread, lock-free) ---
    void noteOn(int note, float velocity);
    void noteOff(int note);
    void allNotesOff();
    void setParameter(int paramIndex, float value);

    //--- Preset loading (UI thread) ---
    /// Apply a map of parameter ID -> value. Thread-safe via queue.
    void loadPresetParams(const std::vector<std::pair<std::string, float>>& params);

    //--- Queries (any thread) ---
    float getParameter(int paramIndex) const;
    int getActiveVoiceCount() const;
    double getSampleRate() const { return sampleRate_; }
    int getParameterCount() const;
    std::string getParameterIdAt(int index) const;

    //--- Oboe callback (audio thread) ---
    oboe::DataCallbackResult onAudioReady(
        oboe::AudioStream* stream,
        void* audioData,
        int32_t numFrames) override;

    void onErrorAfterClose(
        oboe::AudioStream* stream,
        oboe::Result error) override;

private:
    void openStream();
    void drainParamQueue();
    void drainMidiQueue();

    ObrixSDKAdapter engine_;

    // Lock-free queues (UI -> audio)
    SPSCQueue<NoteEvent, 256> midiQueue_;
    SPSCQueue<ParamEvent, 256> paramQueue_;
    std::atomic<bool> allNotesOff_{false};

    // Pre-allocated render buffers
    static constexpr int kMaxFrames = 1024;
    float leftBuf_[kMaxFrames] = {};
    float rightBuf_[kMaxFrames] = {};

    // MIDI event scratch buffer (audio thread only)
    static constexpr int kMaxMidiEvents = 64;
    MidiEvent midiEvents_[kMaxMidiEvents] = {};
    int numMidiEvents_ = 0;

    // Oboe stream
    std::shared_ptr<oboe::AudioStream> stream_;
    std::atomic<bool> running_{false};
    double sampleRate_ = 48000.0;

    // Parameter ID lookup (built once at construction)
    std::vector<std::string> paramIds_;
};

} // namespace obrix
