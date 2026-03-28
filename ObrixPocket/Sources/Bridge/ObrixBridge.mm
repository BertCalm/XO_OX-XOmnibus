#import "ObrixBridge.h"

// JUCE config — must be defined before any JUCE header is included
#include "JuceConfig.h"

// JUCE headers — only visible in this .mm translation unit
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

// The engine
#include "../../Source/Engines/Obrix/ObrixEngine.h"

using namespace xolokun;

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
        char paramId[64];
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
                          std::atomic<bool>& allNotesOffRef)
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
    {
        // Wire engine's atomic parameter pointers to the APVTS
        eng.attachParameters(apvts);
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
        // Drain parameter queue from touch/UI thread (before MIDI, before render)
        while (_paramReadRef.load(std::memory_order_acquire) != _paramWriteRef.load(std::memory_order_acquire))
        {
            int readPos = _paramReadRef.load(std::memory_order_relaxed);
            auto& evt = _paramQueueRef[readPos];

            // Write directly to the atomic denormalized parameter value (audio-thread safe).
            // getRawParameterValue returns std::atomic<float>* holding the real-world value
            // (e.g. Hz for cutoff, seconds for attack). Callers must pre-scale to actual range.
            if (auto* rawValue = apvts.getRawParameterValue(juce::String(evt.paramId)))
            {
                rawValue->store(evt.value, std::memory_order_relaxed);
            }

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

        eng.renderBlock(buffer, midi, buffer.getNumSamples());

        // Apply output gain (for ducking under other media)
        float gain = _gainRef.load(std::memory_order_relaxed);
        if (gain < 0.99f)
        {
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                juce::FloatVectorOperations::multiply(buffer.getWritePointer(ch), gain, buffer.getNumSamples());
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
        _allNotesOffFlag
    );
    _player = std::make_unique<juce::AudioProcessorPlayer>();
    _player->setProcessor(_adapter.get());
    _deviceManager->addAudioCallback(_player.get());

    _running.store(true);

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
    // Lock-free enqueue to param queue; drained in adapter's processBlock before render
    int writePos = _paramWritePos.load(std::memory_order_relaxed);
    int nextPos = (writePos + 1) % ObrixProcessorAdapter::kParamQueueSize;
    if (nextPos == _paramReadPos.load(std::memory_order_acquire)) return; // queue full — drop silently

    strncpy(_paramQueue[writePos].paramId, [paramId UTF8String], 63);
    _paramQueue[writePos].paramId[63] = '\0';
    _paramQueue[writePos].value = value;
    _paramWritePos.store(nextPos, std::memory_order_release);
    // TODO Phase 2: Map paramId string to engine parameter in processBlock drain loop
}

- (float)getParameter:(NSString *)paramId
{
    // Returns the denormalized (real-world) value — e.g. Hz for cutoff, 0-1 for mix.
    // Matches what the engine reads in its renderBlock via loadP().
    if (_adapter) {
        if (auto* raw = _adapter->apvts.getRawParameterValue(juce::String([paramId UTF8String])))
            return raw->load(std::memory_order_relaxed);
    }
    return 0.0f;
}

- (void)setParameterImmediate:(NSString *)paramId value:(float)value
{
    if (!_adapter) return;
    // getRawParameterValue returns a pointer to the DENORMALIZED (real-world) value.
    // Verified in JUCE source: AudioProcessorValueTreeState::getRawParameterValue()
    // returns &adapter->getRawDenormalisedValue(), which stores the actual Hz/dB/etc.
    // value — NOT a 0-1 normalized float.
    //
    // Callers (AudioEngineManager.swift) are responsible for scaling their 0-1 specimen
    // values to the engine's real-world range before calling this method.
    // Example: obrix_proc1Cutoff expects Hz (20-20000), not 0-1 normalized.
    // AudioEngineManager already does: scale: { v in 20.0 + v * (20000.0 - 20.0) }
    //
    // Writing value directly here is correct — no additional normalization needed.
    if (auto* raw = _adapter->apvts.getRawParameterValue(juce::String([paramId UTF8String])))
    {
        raw->store(value, std::memory_order_relaxed);
    }
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
    return 48000.0f;
}

@end
