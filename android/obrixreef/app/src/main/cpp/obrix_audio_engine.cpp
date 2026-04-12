// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#include "obrix_audio_engine.h"

#include <android/log.h>
#include <cmath>
#include <cstring>

#define LOG_TAG "ObrixAudioEngine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace obrix
{

ObrixAudioEngine::ObrixAudioEngine()
{
    // Cache parameter IDs for index-based JNI access
    auto params = engine_.getParameterList();
    paramIds_.reserve(params.size());
    for (const auto& p : params)
        paramIds_.push_back(p.id);
}

ObrixAudioEngine::~ObrixAudioEngine()
{
    stop();
}

bool ObrixAudioEngine::start()
{
    if (running_.load()) return true;

    openStream();

    if (!stream_)
    {
        LOGE("Failed to open audio stream");
        return false;
    }

    sampleRate_ = stream_->getSampleRate();
    engine_.prepare(sampleRate_, stream_->getFramesPerCallback());

    auto result = stream_->requestStart();
    if (result != oboe::Result::OK)
    {
        LOGE("Failed to start stream: %s", oboe::convertToText(result));
        stream_.reset();
        return false;
    }

    running_.store(true, std::memory_order_relaxed);
    LOGI("Audio started: %.0f Hz, %d frames/callback, %s mode",
         sampleRate_,
         stream_->getFramesPerCallback(),
         stream_->getSharingMode() == oboe::SharingMode::Exclusive ? "Exclusive" : "Shared");
    return true;
}

void ObrixAudioEngine::stop()
{
    running_.store(false, std::memory_order_relaxed);
    if (stream_)
    {
        stream_->requestStop();
        stream_->close();
        stream_.reset();
    }
    LOGI("Audio stopped");
}

void ObrixAudioEngine::openStream()
{
    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
           .setPerformanceMode(oboe::PerformanceMode::LowLatency)
           .setSharingMode(oboe::SharingMode::Exclusive)
           .setSampleRate(48000)      // MANDATORY — 44100 adds ~140ms penalty
           .setChannelCount(2)
           .setFormat(oboe::AudioFormat::Float)
           .setCallback(this)
           .setUsage(oboe::Usage::Game);

    auto result = builder.openStream(stream_);
    if (result != oboe::Result::OK)
    {
        LOGE("openStream failed: %s", oboe::convertToText(result));
        stream_.reset();
    }
}

void ObrixAudioEngine::noteOn(int note, float velocity)
{
    midiQueue_.push({note, velocity});
}

void ObrixAudioEngine::noteOff(int note)
{
    midiQueue_.push({note, 0.0f});
}

void ObrixAudioEngine::allNotesOff()
{
    allNotesOff_.store(true, std::memory_order_relaxed);
}

void ObrixAudioEngine::setParameter(int paramIndex, float value)
{
    paramQueue_.push({paramIndex, value});
}

void ObrixAudioEngine::loadPresetParams(
    const std::vector<std::pair<std::string, float>>& params)
{
    for (const auto& [id, value] : params)
    {
        // Find index
        for (int i = 0; i < static_cast<int>(paramIds_.size()); ++i)
        {
            if (paramIds_[i] == id)
            {
                paramQueue_.push({i, value});
                break;
            }
        }
    }
}

float ObrixAudioEngine::getParameter(int paramIndex) const
{
    if (paramIndex < 0 || paramIndex >= static_cast<int>(paramIds_.size()))
        return 0.0f;
    return engine_.getParameter(paramIds_[paramIndex]);
}

int ObrixAudioEngine::getActiveVoiceCount() const
{
    return engine_.getActiveVoiceCount();
}

int ObrixAudioEngine::getParameterCount() const
{
    return static_cast<int>(paramIds_.size());
}

std::string ObrixAudioEngine::getParameterIdAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(paramIds_.size()))
        return {};
    return paramIds_[index];
}

//==============================================================================
// Audio thread callback
//==============================================================================

oboe::DataCallbackResult ObrixAudioEngine::onAudioReady(
    oboe::AudioStream* /*stream*/,
    void* audioData,
    int32_t numFrames)
{
    auto* output = static_cast<float*>(audioData);

    // Clamp to our pre-allocated buffer size
    if (numFrames > kMaxFrames)
        numFrames = kMaxFrames;

    // 1. Drain parameter queue
    drainParamQueue();

    // 2. Drain MIDI queue
    drainMidiQueue();

    // 3. Clear render buffers
    std::memset(leftBuf_, 0, sizeof(float) * numFrames);
    std::memset(rightBuf_, 0, sizeof(float) * numFrames);

    // 4. Render
    engine_.renderBlock(leftBuf_, rightBuf_, numFrames, midiEvents_, numMidiEvents_);
    numMidiEvents_ = 0;

    // 5. Interleave stereo to Oboe output (LRLRLR...)
    for (int i = 0; i < numFrames; ++i)
    {
        float L = leftBuf_[i];
        float R = rightBuf_[i];

        // NaN/Inf safety (belt + suspenders with adapter's own check)
        if (!std::isfinite(L)) L = 0.0f;
        if (!std::isfinite(R)) R = 0.0f;

        output[i * 2]     = L;
        output[i * 2 + 1] = R;
    }

    return oboe::DataCallbackResult::Continue;
}

void ObrixAudioEngine::onErrorAfterClose(
    oboe::AudioStream* /*stream*/,
    oboe::Result error)
{
    LOGE("Audio stream error: %s — restarting", oboe::convertToText(error));

    // Oboe recommends restarting on error
    if (running_.load(std::memory_order_relaxed))
    {
        stream_.reset();
        openStream();
        if (stream_)
        {
            sampleRate_ = stream_->getSampleRate();
            engine_.prepare(sampleRate_, stream_->getFramesPerCallback());
            stream_->requestStart();
        }
    }
}

void ObrixAudioEngine::drainParamQueue()
{
    ParamEvent pe;
    while (paramQueue_.pop(pe))
    {
        if (pe.paramIndex >= 0 && pe.paramIndex < static_cast<int>(paramIds_.size()))
            engine_.setParameter(paramIds_[pe.paramIndex], pe.value);
    }
}

void ObrixAudioEngine::drainMidiQueue()
{
    numMidiEvents_ = 0;

    // Handle all-notes-off flag
    if (allNotesOff_.exchange(false, std::memory_order_relaxed))
    {
        if (numMidiEvents_ < kMaxMidiEvents)
        {
            auto& e = midiEvents_[numMidiEvents_++];
            e.sampleOffset = 0;
            e.data[0] = 0xB0; // CC on channel 1
            e.data[1] = 123;  // All Notes Off
            e.data[2] = 0;
            e.numBytes = 3;
        }
    }

    // Drain note events
    NoteEvent ne;
    while (midiQueue_.pop(ne) && numMidiEvents_ < kMaxMidiEvents)
    {
        auto& e = midiEvents_[numMidiEvents_++];
        e.sampleOffset = 0;
        e.numBytes = 3;

        if (ne.velocity > 0.0f)
        {
            // Note on
            e.data[0] = 0x90;
            e.data[1] = static_cast<uint8_t>(ne.note & 0x7F);
            e.data[2] = static_cast<uint8_t>(ne.velocity * 127.0f);
        }
        else
        {
            // Note off
            e.data[0] = 0x80;
            e.data[1] = static_cast<uint8_t>(ne.note & 0x7F);
            e.data[2] = 0;
        }
    }
}

} // namespace obrix
