#import "ObrixBridge.h"

// JUCE headers — only visible in this .mm translation unit
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>

// The engine
#include "../../Source/Engines/Obrix/ObrixEngine.h"

// Suppress JUCE GUI — we only use audio modules
// JUCE_MODULE_AVAILABLE_juce_gui_basics is NOT defined

@implementation ObrixBridge
{
    std::unique_ptr<ObrixEngine> _engine;
    std::unique_ptr<juce::AudioDeviceManager> _deviceManager;
    std::unique_ptr<juce::AudioProcessorPlayer> _player;
    std::atomic<float> _outputGain;
    std::atomic<bool> _running;

    // Lock-free MIDI input queue (touch → audio thread)
    // Simple ring buffer for note events
    struct NoteEvent {
        int note;
        float velocity; // 0 = note off, >0 = note on
    };
    static constexpr int kMidiQueueSize = 256;
    NoteEvent _midiQueue[kMidiQueueSize];
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

        // Initialize JUCE
        juce::initialiseJuce_GUI(); // Required even for audio-only on iOS

        // Create engine
        _engine = std::make_unique<ObrixEngine>();
    }
    return self;
}

- (void)startAudio
{
    if (_running.load()) return;

    // TODO Phase 0 Week 2-3: Set up AudioDeviceManager for iOS
    // This requires JUCE's iOS audio device type to be available.
    // For now, create the device manager and attempt to open the default device.

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

    // Create processor player (hosts the engine as an AudioProcessor)
    // Note: ObrixEngine implements SynthEngine which extends AudioProcessor-like interface
    // TODO: Wrap ObrixEngine in a juce::AudioProcessor adapter for AudioProcessorPlayer

    _running.store(true);
    NSLog(@"[ObrixBridge] Audio started. SR=%.0f", _deviceManager->getCurrentAudioDevice()->getCurrentSampleRate());
}

- (void)stopAudio
{
    if (!_running.load()) return;

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
    // Send note-off for all 128 MIDI notes would flood the queue.
    // Instead, the engine should support an all-notes-off message.
    // For Phase 0: send CC 123 (All Notes Off) — TODO wire to engine
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
