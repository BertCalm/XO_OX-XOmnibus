#import "ObrixBridge.h"

// JUCE headers — only visible in this .mm translation unit
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

// The engine
#include "../../Source/Engines/Obrix/ObrixEngine.h"

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

    ObrixProcessorAdapter(ObrixEngine& engine,
                          std::atomic<float>& gainRef,
                          NoteEvent* queueRef,
                          std::atomic<int>& readPosRef,
                          std::atomic<int>& writePosRef)
        : eng(engine)
        , _gainRef(gainRef)
        , _queueRef(queueRef)
        , _readPosRef(readPosRef)
        , _writePosRef(writePosRef)
    {}

    const juce::String getName() const override { return "ObrixPocket"; }

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        eng.prepare(sampleRate, samplesPerBlock);
    }

    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
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

private:
    ObrixEngine& eng;
    std::atomic<float>& _gainRef;
    NoteEvent* _queueRef;
    std::atomic<int>& _readPosRef;
    std::atomic<int>& _writePosRef;
};

@implementation ObrixBridge
{
    std::unique_ptr<ObrixEngine> _engine;
    std::unique_ptr<juce::AudioDeviceManager> _deviceManager;
    std::unique_ptr<juce::AudioProcessorPlayer> _player;
    std::unique_ptr<ObrixProcessorAdapter> _adapter;
    std::atomic<float> _outputGain;
    std::atomic<bool> _running;

    // Lock-free MIDI input queue (touch → audio thread)
    // Simple ring buffer for note events
    static constexpr int kMidiQueueSize = 256;
    ObrixProcessorAdapter::NoteEvent _midiQueue[kMidiQueueSize];
    std::atomic<int> _midiWritePos;
    std::atomic<int> _midiReadPos;
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
        _midiWritePos
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
    // Send note-off for all 128 possible MIDI notes via the lock-free queue.
    for (int note = 0; note < 128; ++note)
    {
        [self noteOff:note];
    }
}

- (void)setParameter:(NSString *)paramId value:(float)value
{
    // TODO Phase 0 Week 3: Wire to engine's parameter system
    // _engine->setParameter(juce::String([paramId UTF8String]), value);
}

- (float)getParameter:(NSString *)paramId
{
    // TODO Phase 0 Week 3
    return 0.0f;
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
