#import "ObrixBridge.h"

// JUCE config — must be defined before any JUCE header is included
#include "JuceConfig.h"

// JUCE headers — only visible in this .mm translation unit
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

// STL headers for param cache (must come after JUCE to avoid macro conflicts)
#include <string>
#include <unordered_map>

// The engine
#include "../../Source/Engines/Obrix/ObrixEngine.h"

using namespace xoceanus;

// Suppress JUCE GUI — we only use audio modules
// JUCE_MODULE_AVAILABLE_juce_gui_basics is NOT defined

// ── Thin adapter: wraps ObrixEngine as a juce::AudioProcessor ────────
class ObrixProcessorAdapter : public juce::AudioProcessor
{
public:
    struct NoteEvent {
        int note;
        float velocity; // 0 = note off, >0 = note on
    };

    struct ParamEvent {
        std::atomic<float>* target;  // Pre-resolved pointer (resolved on UI thread)
        float value;
    };

    ObrixProcessorAdapter(ObrixEngine& engine,
                          std::atomic<float>& gainRef,
                          NoteEvent* queueRef,
                          std::atomic<int>& readPosRef,
                          std::atomic<int>& writePosRef,
                          ParamEvent* paramQueueRef,
                          std::atomic<int>& paramReadPosRef,
                          std::atomic<int>& paramWritePosRef,
                          std::atomic<bool>& allNotesOffRef,
                          std::atomic<bool>& tapActiveRef,
                          float* tapBufA,
                          float* tapBufB,
                          std::atomic<float*>& tapReadyBufferRef,
                          std::atomic<int>& tapReadyFramesRef,
                          std::atomic<int>& tapReadyChannelsRef)
        : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
        , eng(engine)
        , apvts(*this, nullptr, "ObrixPocketParams", engine.createParameterLayout())
        , _gainRef(gainRef)
        , _queueRef(queueRef)
        , _readPosRef(readPosRef)
        , _writePosRef(writePosRef)
        , _paramQueueRef(paramQueueRef)
        , _paramReadRef(paramReadPosRef)
        , _paramWriteRef(paramWritePosRef)
        , _allNotesOffRef(allNotesOffRef)
        , _tapActiveRef(tapActiveRef)
        , _tapBufARef(tapBufA)
        , _tapBufBRef(tapBufB)
        , _tapReadyBufferRef(tapReadyBufferRef)
        , _tapReadyFramesRef(tapReadyFramesRef)
        , _tapReadyChannelsRef(tapReadyChannelsRef)
    {
        // Wire engine's atomic parameter pointers to the APVTS
        eng.attachParameters(apvts);

        // Pre-cache all parameter pointers — eliminates juce::String on audio thread.
        // getRawParameterValue returns std::atomic<float>* which is stable for the
        // lifetime of the APVTS. Safe to cache once at construction.
        for (auto* param : apvts.processor.getParameters())
        {
            if (auto* withID = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
            {
                auto* raw = apvts.getRawParameterValue(withID->paramID);
                if (raw)
                    _paramCache[withID->paramID.toStdString()] = raw;
            }
        }
    }

    juce::AudioProcessorValueTreeState apvts;

    const juce::String getName() const override { return "ObrixPocket"; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        eng.prepare(sampleRate, samplesPerBlock);
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        // Drain parameter queue from touch/UI thread (before MIDI, before render).
        // Each event carries a pre-resolved std::atomic<float>* pointer — the UI
        // thread did the string→pointer lookup at enqueue time. ProcessBlock does
        // ZERO string operations, ZERO hash lookups, ZERO heap allocations.
        while (_paramReadRef.load(std::memory_order_acquire) != _paramWriteRef.load(std::memory_order_acquire))
        {
            int readPos = _paramReadRef.load(std::memory_order_relaxed);
            auto& evt = _paramQueueRef[readPos];

            if (evt.target)
                evt.target->store(evt.value, std::memory_order_relaxed);

            _paramReadRef.store((readPos + 1) % kParamQueueSize, std::memory_order_release);
        }

        // Drain MIDI queue from touch thread
        while (_readPosRef.load(std::memory_order_acquire) != _writePosRef.load(std::memory_order_acquire))
        {
            int readPos = _readPosRef.load(std::memory_order_relaxed);
            auto& evt = _queueRef[readPos];
            if (evt.velocity > 0.0f)
                midi.addEvent(juce::MidiMessage::noteOn(1, evt.note, evt.velocity), 0);
            else
                midi.addEvent(juce::MidiMessage::noteOff(1, evt.note), 0);
            _readPosRef.store((readPos + 1) % 256, std::memory_order_release);
        }

        // Atomic all-notes-off: a single CC123 message replaces 128 individual note-offs,
        // which could silently drop if the ring buffer was partially full.
        if (_allNotesOffRef.exchange(false, std::memory_order_acquire))
        {
            midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
        }

        // Clear the buffer before rendering — the engine uses addSample() which
        // accumulates. If JUCE doesn't zero the buffer (edge case in some hosting
        // configurations), stale data from the previous callback would bleed through.
        buffer.clear();

        eng.renderBlock(buffer, midi, buffer.getNumSamples());

        // === NaN/Infinity guard =============================================
        // If the engine produces any invalid samples (NaN, inf, or extreme
        // values), they propagate to CoreAudio's HAL and can corrupt the
        // hardware audio state on iOS, requiring a full device reboot.
        // This clamp is the last line of defense. If it triggers, the engine
        // has a bug — but the user's phone doesn't brick.
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (std::isnan(data[i]) || std::isinf(data[i]))
                    data[i] = 0.0f;
                else
                    data[i] = std::max(-4.0f, std::min(4.0f, data[i]));
            }
        }

        // Apply output gain (for ducking under other media)
        float gain = _gainRef.load(std::memory_order_relaxed);
        if (gain < 0.99f)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                juce::FloatVectorOperations::multiply(buffer.getWritePointer(ch), gain, buffer.getNumSamples());
        }

        // ── Output tap ──────────────────────────────────────────────────
        // Only execute if the main thread has enabled the tap.
        if (_tapActiveRef.load(std::memory_order_relaxed))
        {
            int frames   = buffer.getNumSamples();
            int channels = std::min(buffer.getNumChannels(), 2);
            int required = frames * channels;

            // Guard against oversized blocks (tap buffers are 8192 floats = 4096 frames × 2 ch)
            if (required <= 8192)
            {
                // Pick the write buffer: the one NOT currently published as ready.
                float* readyNow = _tapReadyBufferRef.load(std::memory_order_relaxed);
                float* writeBuf = (readyNow == _tapBufARef) ? _tapBufBRef : _tapBufARef;

                // Interleave: [L0 R0 L1 R1 ...]
                for (int i = 0; i < frames; ++i)
                    for (int ch = 0; ch < channels; ++ch)
                        writeBuf[i * channels + ch] = buffer.getReadPointer(ch)[i];

                _tapReadyFramesRef.store(frames,   std::memory_order_relaxed);
                _tapReadyChannelsRef.store(channels, std::memory_order_relaxed);
                // Publish — main thread reads this with acquire
                _tapReadyBufferRef.store(writeBuf, std::memory_order_release);
            }
        }
    }

    // Required overrides (minimal — we're not a plugin, just hosting internally)
    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    static constexpr int kParamQueueSize = 256;

    // Direct param accessors for main-thread callers (setParameterImmediate / getParameter).
    // String lookup happens on the calling thread (UI/main) — never on audio thread.
    void setParamDirect(const char* paramId, float value)
    {
        auto it = _paramCache.find(paramId);
        if (it != _paramCache.end())
            it->second->store(value, std::memory_order_relaxed);
    }

    float getParamDirect(const char* paramId) const
    {
        auto it = _paramCache.find(paramId);
        if (it != _paramCache.end())
            return it->second->load(std::memory_order_relaxed);
        return 0.0f;
    }

    /// Expose cache for setParameter: to resolve pointers on UI thread.
    /// The map is immutable after construction — safe to read from any thread.
    const std::unordered_map<std::string, std::atomic<float>*>& paramCache() const { return _paramCache; }

private:
    ObrixEngine& eng;
    std::atomic<float>& _gainRef;
    NoteEvent* _queueRef;
    std::atomic<int>& _readPosRef;
    std::atomic<int>& _writePosRef;
    ParamEvent* _paramQueueRef;
    std::atomic<int>& _paramReadRef;
    std::atomic<int>& _paramWriteRef;
    std::atomic<bool>& _allNotesOffRef;
    // Tap references — point into the ObrixBridge ivar storage
    std::atomic<bool>& _tapActiveRef;
    float* const _tapBufARef;
    float* const _tapBufBRef;
    std::atomic<float*>& _tapReadyBufferRef;
    std::atomic<int>& _tapReadyFramesRef;
    std::atomic<int>& _tapReadyChannelsRef;

    // Pre-cached parameter pointers — built once at construction, never reallocated.
    // Keyed by std::string so processBlock can look up params with zero heap allocation.
    std::unordered_map<std::string, std::atomic<float>*> _paramCache;
};

static const int kMidiQueueSize = 256;

@implementation ObrixBridge
{
    std::unique_ptr<ObrixEngine> _engine;
    std::unique_ptr<juce::AudioDeviceManager> _deviceManager;
    std::unique_ptr<juce::AudioProcessorPlayer> _player;
    std::unique_ptr<ObrixProcessorAdapter> _adapter;
    std::atomic<float> _outputGain;
    std::atomic<bool> _running;

    // Lock-free MIDI input queue (touch → audio thread)
    ObrixProcessorAdapter::NoteEvent _midiQueue[kMidiQueueSize];
    std::atomic<int> _midiWritePos;
    std::atomic<int> _midiReadPos;

    // Lock-free parameter queue (UI thread → audio thread)
    ObrixProcessorAdapter::ParamEvent _paramQueue[ObrixProcessorAdapter::kParamQueueSize];
    std::atomic<int> _paramWritePos;
    std::atomic<int> _paramReadPos;

    // Atomic flag: set by allNotesOff on the UI thread, consumed (exchanged) by processBlock.
    // Avoids flooding the 256-slot MIDI ring buffer with 128 individual note-offs.
    std::atomic<bool> _allNotesOffFlag;

    // ── Output tap (double-buffer, lock-free) ──────────────────────────────
    // Audio thread writes interleaved PCM into whichever buffer is NOT the
    // current _tapReadyBuffer, then atomically publishes it.
    // Main thread (AudioExporter) calls -drainTapInto: to swap the pointer.
    //
    // kTapBufSize: max frames × 2 channels. 4 096 frames × 2 = 8 192 floats.
    // At 48 kHz / 256-sample blocks this wraps ~187 times/sec — plenty of
    // headroom for the 60 Hz polling timer in AudioExporter.
    // (Plain array size — static constexpr not valid in ObjC ivar block)
    float _tapBufA[8192]; // 4 096 frames × 2 ch
    float _tapBufB[8192];
    std::atomic<bool> _tapActive;
    // Pointer to the last fully-written buffer (nullptr if none ready).
    // Written only on the audio thread; read and cleared on the main thread.
    std::atomic<float*> _tapReadyBuffer;
    std::atomic<int> _tapReadyFrames;
    std::atomic<int> _tapReadyChannels;
}

+ (instancetype)shared
{
    static ObrixBridge *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[ObrixBridge alloc] init];
    });
    return instance;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _outputGain.store(1.0f);
        _running.store(false);
        _midiWritePos.store(0);
        _midiReadPos.store(0);
        _paramWritePos.store(0);
        _paramReadPos.store(0);
        _allNotesOffFlag.store(false);
        _tapActive.store(false);
        _tapReadyBuffer.store(nullptr);
        _tapReadyFrames.store(0);
        _tapReadyChannels.store(0);

        // Ensure JUCE message manager exists (required for audio callbacks)
        juce::MessageManager::getInstance();

        // Create engine
        _engine = std::make_unique<ObrixEngine>();
    }
    return self;
}

- (void)startAudio
{
    if (_running.load()) return;

    _deviceManager = std::make_unique<juce::AudioDeviceManager>();

    // Initialize with stereo output, no input (mic handled separately via AVAudioEngine)
    auto error = _deviceManager->initialise(
        0,    // numInputChannels
        2,    // numOutputChannels
        nullptr, // savedState XML
        true     // selectDefaultDeviceOnFailure
    );

    if (error.isNotEmpty()) {
        NSLog(@"[ObrixBridge] AudioDeviceManager init error: %s", error.toRawUTF8());
        return;
    }

    // Create adapter and player, wire engine to audio output
    _adapter = std::make_unique<ObrixProcessorAdapter>(
        *_engine,
        _outputGain,
        _midiQueue,
        _midiReadPos,
        _midiWritePos,
        _paramQueue,
        _paramReadPos,
        _paramWritePos,
        _allNotesOffFlag,
        _tapActive,
        _tapBufA,
        _tapBufB,
        _tapReadyBuffer,
        _tapReadyFrames,
        _tapReadyChannels
    );
    _player = std::make_unique<juce::AudioProcessorPlayer>();
    _player->setProcessor(_adapter.get());

    // Set _running BEFORE adding the audio callback — processBlock may fire
    // immediately after addAudioCallback, and it should see a valid state.
    _running.store(true);

    _deviceManager->addAudioCallback(_player.get());

    if (auto* dev = _deviceManager->getCurrentAudioDevice())
        NSLog(@"[ObrixBridge] Audio started. SR=%.0f", dev->getCurrentSampleRate());
    else
        NSLog(@"[ObrixBridge] Audio started (device pending).");
}

- (void)stopAudio
{
    if (!_running.load()) return;

    if (_player)
        _deviceManager->removeAudioCallback(_player.get());

    _deviceManager->closeAudioDevice();
    _running.store(false);
    NSLog(@"[ObrixBridge] Audio stopped.");
}

- (BOOL)isRunning
{
    return _running.load();
}

// MARK: - Note Input (lock-free queue for touch → audio thread)

- (void)noteOn:(int)note velocity:(float)velocity
{
    int writePos = _midiWritePos.load(std::memory_order_relaxed);
    int nextPos = (writePos + 1) % kMidiQueueSize;
    if (nextPos == _midiReadPos.load(std::memory_order_acquire)) return; // queue full

    _midiQueue[writePos] = { note, velocity };
    _midiWritePos.store(nextPos, std::memory_order_release);
}

- (void)noteOff:(int)note
{
    int writePos = _midiWritePos.load(std::memory_order_relaxed);
    int nextPos = (writePos + 1) % kMidiQueueSize;
    if (nextPos == _midiReadPos.load(std::memory_order_acquire)) return;

    _midiQueue[writePos] = { note, 0.0f };
    _midiWritePos.store(nextPos, std::memory_order_release);
}

- (void)allNotesOff
{
    // Set the atomic flag — processBlock will exchange it and emit a single CC123
    // (all notes off) on the next audio callback. This avoids flooding the 256-slot
    // MIDI ring buffer with 128 individual note-offs, which would silently drop notes
    // if the queue was partially full, leaving stuck notes.
    _allNotesOffFlag.store(true, std::memory_order_release);
}

- (void)setParameter:(NSString *)paramId value:(float)value
{
    if (!_adapter) return;

    // Resolve the parameter pointer NOW on the UI thread — processBlock
    // will just dereference the pointer with zero string operations.
    auto it = _adapter->paramCache().find([paramId UTF8String]);
    if (it == _adapter->paramCache().end()) return; // Unknown param — ignore

    int writePos = _paramWritePos.load(std::memory_order_relaxed);
    int nextPos = (writePos + 1) % ObrixProcessorAdapter::kParamQueueSize;
    if (nextPos == _paramReadPos.load(std::memory_order_acquire)) return; // queue full

    _paramQueue[writePos].target = it->second;
    _paramQueue[writePos].value = value;
    _paramWritePos.store(nextPos, std::memory_order_release);
}

- (float)getParameter:(NSString *)paramId
{
    // Returns the denormalized (real-world) value — e.g. Hz for cutoff, 0-1 for mix.
    // Uses pre-cached pointer map — no juce::String allocation.
    if (!_adapter) return 0.0f;
    return _adapter->getParamDirect([paramId UTF8String]);
}

- (void)setParameterImmediate:(NSString *)paramId value:(float)value
{
    if (!_adapter) return;
    // Uses pre-cached pointer map — no juce::String allocation.
    // Values are DENORMALIZED (real-world): Hz for cutoff, seconds for attack, etc.
    // Callers (AudioEngineManager.swift) are responsible for pre-scaling their 0-1
    // specimen values to the engine's real-world range before calling this method.
    // Example: obrix_proc1Cutoff expects Hz (20-20000), not 0-1 normalized.
    // AudioEngineManager already does: scale: { v in 20.0 + v * (20000.0 - 20.0) }
    _adapter->setParamDirect([paramId UTF8String], value);
}

- (void)setOutputGain:(float)gain
{
    _outputGain.store(gain, std::memory_order_relaxed);
}

- (float)cpuLoad
{
    if (_deviceManager && _deviceManager->getCurrentAudioDevice())
        return (float)_deviceManager->getCpuUsage();
    return 0.0f;
}

- (int)activeVoiceCount
{
    // TODO Phase 0 Week 3: Query engine voice count
    return 0;
}

- (float)sampleRate
{
    if (_deviceManager && _deviceManager->getCurrentAudioDevice())
        return (float)_deviceManager->getCurrentAudioDevice()->getCurrentSampleRate();
    return 0.0f;
}

// MARK: - Output tap

- (void)startOutputTap
{
    // Clear any stale ready-buffer pointer so the first drain doesn't return
    // leftover data from before the tap started.
    _tapReadyBuffer.store(nullptr, std::memory_order_release);
    _tapReadyFrames.store(0, std::memory_order_relaxed);
    _tapReadyChannels.store(0, std::memory_order_relaxed);
    // Enable last — the audio thread checks this flag first.
    _tapActive.store(true, std::memory_order_release);
}

- (void)stopOutputTap
{
    // Disable first so the audio thread stops writing.
    _tapActive.store(false, std::memory_order_release);
    // Then clear the ready pointer so a late drain returns 0 frames.
    _tapReadyBuffer.store(nullptr, std::memory_order_release);
}

- (int)drainTapInto:(float *)dest
          maxFrames:(int)maxFrames
        outChannels:(int *)outChannels
{
    // Atomically claim the ready buffer (swap to nullptr).
    // This is the ONLY place the ready pointer is cleared on the reader side,
    // which ensures each captured block is returned exactly once.
    float* buf = _tapReadyBuffer.exchange(nullptr, std::memory_order_acquire);
    if (!buf) {
        *outChannels = 0;
        return 0;
    }

    int frames   = _tapReadyFrames.load(std::memory_order_relaxed);
    int channels = _tapReadyChannels.load(std::memory_order_relaxed);
    int toCopy   = std::min(frames, maxFrames);

    memcpy(dest, buf, (size_t)(toCopy * channels) * sizeof(float));
    *outChannels = channels;
    return toCopy;
}

@end
