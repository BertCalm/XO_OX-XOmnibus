#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Core/PresetManager.h"
#include "../Core/EngineRegistry.h"

#include <atomic>
#include <mutex>
#include <vector>
#include <thread>
#include <cmath>

namespace xomnibus {

//==============================================================================
// XDrip — Lightweight background preview renderer for the ExportDialog.
//
// Renders a 2-second mono preview of a preset at C3 (MIDI 60), velocity 100,
// on a low-priority background thread. Returns a small float buffer suitable
// for waveform thumbnail display and quick playback.
//
// Architecture notes:
//   - NEVER allocates on the audio thread (runs entirely on worker thread)
//   - Uses the same offline rendering path as XOriginate
//   - Thread-safe: atomic state, mutex on buffer access
//   - Cancellable mid-render
//   - DSP inline in .h per project convention
//
class XDrip
{
public:
    enum class State { Idle, Rendering, Ready, Error };

    static constexpr float kPreviewDurationS  = 2.0f;
    static constexpr int   kPreviewSampleRate  = 44100;
    static constexpr int   kPreviewNote        = 60;  // C3
    static constexpr int   kPreviewVelocity    = 100;
    static constexpr int   kThumbnailPoints    = 100;
    static constexpr int   kBlockSize          = 512;

    XDrip() = default;

    ~XDrip()
    {
        cancel();
        joinWorker();
    }

    //==========================================================================
    // Public API
    //==========================================================================

    /// Request a 2-second preview render for the given preset.
    /// Cancels any in-progress render before starting the new one.
    /// apvts may be nullptr (produces silent preview).
    void requestPreview(const PresetData& preset,
                        juce::AudioProcessorValueTreeState* apvts)
    {
        // Cancel any existing render
        cancel();
        joinWorker();

        // Snapshot the preset data on the calling thread
        pendingPreset_ = preset;
        pendingApvts_  = apvts;

        state_.store(State::Rendering);
        shouldCancel_.store(false);

        workerThread_ = std::thread([this] { renderWorker(); });
    }

    /// Cancel any in-progress render.
    void cancel()
    {
        shouldCancel_.store(true);
    }

    /// Current render state (lock-free read).
    State getState() const { return state_.load(); }

    /// Get the waveform thumbnail (kThumbnailPoints values, normalized 0..1).
    /// Returns empty vector if state != Ready.
    std::vector<float> getThumbnail() const
    {
        std::lock_guard<std::mutex> lock(bufferMutex_);
        return thumbnail_;
    }

    /// Get the full rendered buffer for playback.
    /// Returns an empty buffer if state != Ready.
    juce::AudioBuffer<float> getPreviewBuffer() const
    {
        std::lock_guard<std::mutex> lock(bufferMutex_);
        if (state_.load() != State::Ready || renderBuffer_.getNumSamples() == 0)
            return {};
        juce::AudioBuffer<float> copy(renderBuffer_.getNumChannels(),
                                       renderBuffer_.getNumSamples());
        for (int ch = 0; ch < renderBuffer_.getNumChannels(); ++ch)
            copy.copyFrom(ch, 0, renderBuffer_, ch, 0, renderBuffer_.getNumSamples());
        return copy;
    }

private:

    //==========================================================================
    // State
    //==========================================================================

    std::atomic<State> state_ { State::Idle };
    std::atomic<bool>  shouldCancel_ { false };

    mutable std::mutex           bufferMutex_;
    juce::AudioBuffer<float>     renderBuffer_;
    std::vector<float>           thumbnail_;

    // Snapshot of the request (written on message thread before worker starts)
    PresetData                                pendingPreset_;
    juce::AudioProcessorValueTreeState*       pendingApvts_ = nullptr;

    std::thread workerThread_;

    //==========================================================================
    // Worker thread
    //==========================================================================

    void joinWorker()
    {
        if (workerThread_.joinable())
            workerThread_.join();
    }

    void renderWorker()
    {
        // Lower thread priority so we don't compete with audio
#if JUCE_MAC || JUCE_IOS
        pthread_setschedparam(pthread_self(), SCHED_OTHER, nullptr);
#endif

        const int totalSamples = static_cast<int>(kPreviewDurationS * kPreviewSampleRate);
        const int holdSamples  = static_cast<int>(1.5f * kPreviewSampleRate); // 1.5s hold, 0.5s tail
        const int numChannels  = 1; // mono preview

        // Build offline engine context
        auto ctx = buildPreviewContext(pendingPreset_, kPreviewSampleRate);
        if (!ctx.valid)
        {
            state_.store(State::Error);
            return;
        }

        // Allocate render buffer on the worker thread (never on audio thread)
        juce::AudioBuffer<float> buffer(numChannels, totalSamples);
        buffer.clear();

        // Prepare MIDI note-on
        const uint8_t velocityByte = static_cast<uint8_t>(
            juce::jlimit(0, 127, kPreviewVelocity));

        int rendered = 0;
        bool noteOffSent = false;

        while (rendered < totalSamples)
        {
            if (shouldCancel_.load())
            {
                state_.store(State::Idle);
                return;
            }

            int blockSize = juce::jmin(kBlockSize, totalSamples - rendered);

            juce::MidiBuffer midi;
            if (rendered == 0)
            {
                midi.addEvent(
                    juce::MidiMessage::noteOn(1, kPreviewNote, velocityByte), 0);
            }
            else if (!noteOffSent && rendered >= holdSamples)
            {
                midi.addEvent(
                    juce::MidiMessage::noteOff(1, kPreviewNote), 0);
                noteOffSent = true;
            }

            juce::AudioBuffer<float> engineBuf(numChannels, blockSize);

            for (auto& engine : ctx.engines)
            {
                engineBuf.clear();
                engine->renderBlock(engineBuf, midi, blockSize);

                // Mix into accumulator
                buffer.addFrom(0, rendered, engineBuf, 0, 0, blockSize);
            }

            rendered += blockSize;
        }

        // Normalize
        normalizeBuffer(buffer);

        // Generate thumbnail
        auto thumb = generateThumbnail(buffer);

        // Commit results under lock
        {
            std::lock_guard<std::mutex> lock(bufferMutex_);
            renderBuffer_ = std::move(buffer);
            thumbnail_    = std::move(thumb);
        }

        state_.store(State::Ready);
    }

    //==========================================================================
    // Offline render context (mirrors XOriginate pattern)
    //==========================================================================

    struct PreviewContext
    {
        std::vector<std::unique_ptr<SynthEngine>> engines;
        bool valid = false;
    };

    PreviewContext buildPreviewContext(const PresetData& preset, double sampleRate)
    {
        PreviewContext ctx;

        // Apply preset parameters to shared APVTS if available
        if (pendingApvts_)
        {
            for (const auto& [engName, paramsVar] : preset.parametersByEngine)
            {
                if (auto* obj = paramsVar.getDynamicObject())
                {
                    for (const auto& prop : obj->getProperties())
                    {
                        juce::String paramId = prop.name.toString();
                        auto canonical = resolveEngineAlias(engName).equalsIgnoreCase("OddfeliX")
                            ? resolveSnapParamAlias(paramId) : paramId;
                        if (canonical.isEmpty()) continue;

                        if (auto* raw = pendingApvts_->getRawParameterValue(canonical))
                            raw->store(static_cast<float>(prop.value));
                    }
                }
            }
        }

        // Create engine instances
        for (const auto& engName : preset.engines)
        {
            auto canonical = resolveEngineAlias(engName);
            auto engine = EngineRegistry::instance().createEngine(canonical.toStdString());
            if (!engine) continue;

            if (pendingApvts_)
                engine->attachParameters(*pendingApvts_);

            engine->prepare(sampleRate, kBlockSize);
            engine->prepareSilenceGate(sampleRate, kBlockSize, 500.0f);
            engine->reset();
            ctx.engines.push_back(std::move(engine));
        }

        ctx.valid = !ctx.engines.empty();
        return ctx;
    }

    //==========================================================================
    // Normalization — peak-scan + gain to -0.3 dBFS
    //==========================================================================

    static void normalizeBuffer(juce::AudioBuffer<float>& buffer)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto range = buffer.findMinMax(ch, 0, buffer.getNumSamples());
            float chPeak = juce::jmax(std::abs(range.getStart()),
                                       std::abs(range.getEnd()));
            if (chPeak > peak) peak = chPeak;
        }

        if (peak < 1e-8f) return; // silence

        static constexpr float kCeilingDb = -0.3f;
        float targetGain = std::pow(10.0f, kCeilingDb / 20.0f);
        float gain = targetGain / peak;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.applyGain(ch, 0, buffer.getNumSamples(), gain);
    }

    //==========================================================================
    // Thumbnail generation — downsample to kThumbnailPoints peak values
    //==========================================================================

    static std::vector<float> generateThumbnail(const juce::AudioBuffer<float>& buffer)
    {
        std::vector<float> thumbnail(kThumbnailPoints, 0.0f);
        if (buffer.getNumSamples() == 0) return thumbnail;

        const int totalSamples = buffer.getNumSamples();
        const float samplesPerPoint = static_cast<float>(totalSamples) / kThumbnailPoints;
        const float* data = buffer.getReadPointer(0);

        // Find global peak for normalization
        float globalPeak = 0.0f;

        for (int i = 0; i < kThumbnailPoints; ++i)
        {
            int start = static_cast<int>(i * samplesPerPoint);
            int end   = static_cast<int>((i + 1) * samplesPerPoint);
            end = juce::jmin(end, totalSamples);

            float peak = 0.0f;
            for (int s = start; s < end; ++s)
            {
                float absVal = std::abs(data[s]);
                if (absVal > peak) peak = absVal;
            }

            thumbnail[static_cast<size_t>(i)] = peak;
            if (peak > globalPeak) globalPeak = peak;
        }

        // Normalize to 0..1
        if (globalPeak > 1e-8f)
        {
            float invPeak = 1.0f / globalPeak;
            for (auto& v : thumbnail)
                v *= invPeak;
        }

        return thumbnail;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XDrip)
};

} // namespace xomnibus
