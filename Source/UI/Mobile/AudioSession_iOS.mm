//==============================================================================
// AudioSession_iOS.mm — AVAudioSession management bridge for XOceanus iOS.
//
// Standalone mode only. AUv3 mode: configure() detects the .appex bundle and
// exits immediately so the host retains full session control.
//
// Notification observers are registered and removed on the main thread.
// AVAudioSession notifications are delivered on the thread that registered the
// observer (main thread), so all callbacks fire on the main thread.
//
// No memory is allocated on the audio thread. All state access from the audio
// thread goes through std::atomic reads.
//
#include "AudioSession_iOS.h"

#if JUCE_IOS

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>
#include <atomic>
#include <functional>

//==============================================================================
// Internal state — translation-unit private.
//
namespace {

//-- Session state -------------------------------------------------------------
std::atomic<bool>  g_sessionActive    { false };
std::atomic<float> g_sampleRate       { 0.0f  };
std::atomic<int>   g_bufferSize       { 0     };

//-- User callbacks -----------------------------------------------------------
std::function<void(bool)> g_interruptionCallback;
std::function<void()>     g_routeChangeCallback;

//-- NSNotificationCenter observers (retained so we can remove them) ----------
id g_interruptionObserver = nil;
id g_routeChangeObserver  = nil;

//-- AUv3 detection -----------------------------------------------------------
// Extensions live in a .appex bundle. Check the main bundle path suffix.
// This check is intentionally conservative: if we cannot determine the bundle
// type for any reason, we assume standalone and proceed with configuration.
bool isRunningAsAUv3Extension()
{
    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    return [bundlePath hasSuffix:@".appex"];
}

//-- Internal helpers ---------------------------------------------------------
void updateHardwareMetrics()
{
    AVAudioSession* session = [AVAudioSession sharedInstance];
    g_sampleRate.store(static_cast<float>(session.sampleRate), std::memory_order_relaxed);

    // bufferDuration is in seconds; convert to frames using the hardware rate.
    double sr  = session.sampleRate;
    double dur = session.IOBufferDuration;
    int frames = (sr > 0.0) ? static_cast<int>(std::round(dur * sr)) : 0;
    g_bufferSize.store(frames, std::memory_order_relaxed);
}

} // anonymous namespace

//==============================================================================
// Public API implementation.
//
namespace xoceanus::audio_session {

void configure()
{
    // AUv3 guard — host owns the session, we must not touch it.
    if (isRunningAsAUv3Extension())
        return;

    AVAudioSession* session = [AVAudioSession sharedInstance];
    NSError* error = nil;

    // ------------------------------------------------------------------
    // Category: Playback.
    //   Use AVAudioSessionCategoryPlayback for standalone synth output.
    //   Switch to AVAudioSessionCategoryPlayAndRecord if the engine ever
    //   needs live microphone input (OSMOSIS external audio path).
    //   Options:
    //     DefaultToSpeaker   — route to speaker when no headphones present
    //     MixWithOthers      — intentionally NOT set; we want exclusive focus
    //     AllowBluetooth     — allow AirPods, A2DP receivers
    //     AllowAirPlay       — allow AirPlay 2 wireless output
    // ------------------------------------------------------------------
    BOOL ok = [session setCategory:AVAudioSessionCategoryPlayback
                       withOptions:  AVAudioSessionCategoryOptionAllowBluetooth
                                   | AVAudioSessionCategoryOptionAllowAirPlay
                                   | AVAudioSessionCategoryOptionDefaultToSpeaker
                             error:&error];
    if (!ok)
    {
        NSLog(@"[XOceanus] AVAudioSession setCategory failed: %@", error.localizedDescription);
        return;
    }

    // ------------------------------------------------------------------
    // Preferred sample rate — 48 kHz.
    // iOS may not honour this exactly (depends on connected hardware);
    // always read back session.sampleRate to get the actual value.
    // ------------------------------------------------------------------
    [session setPreferredSampleRate:48000.0 error:&error];
    if (error)
        NSLog(@"[XOceanus] AVAudioSession setPreferredSampleRate: %@", error.localizedDescription);

    // ------------------------------------------------------------------
    // Preferred I/O buffer duration — 128 frames at 48 kHz ≈ 2.67 ms.
    // iOS may round this to the nearest supported value.
    // ------------------------------------------------------------------
    NSTimeInterval preferredDuration = 128.0 / 48000.0;
    [session setPreferredIOBufferDuration:preferredDuration error:&error];
    if (error)
        NSLog(@"[XOceanus] AVAudioSession setPreferredIOBufferDuration: %@", error.localizedDescription);

    // ------------------------------------------------------------------
    // Activate the session.
    // ------------------------------------------------------------------
    ok = [session setActive:YES error:&error];
    if (!ok)
    {
        NSLog(@"[XOceanus] AVAudioSession setActive failed: %@", error.localizedDescription);
        return;
    }

    g_sessionActive.store(true, std::memory_order_release);
    updateHardwareMetrics();

    NSLog(@"[XOceanus] AVAudioSession active — rate: %.0f Hz, buffer: %d frames",
          (double)g_sampleRate.load(std::memory_order_relaxed),
          g_bufferSize.load(std::memory_order_relaxed));
}

//------------------------------------------------------------------------------
void handleInterruption(std::function<void(bool)> callback)
{
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

    // Remove any previous observer before registering a new one.
    if (g_interruptionObserver != nil)
    {
        [nc removeObserver:g_interruptionObserver];
        g_interruptionObserver = nil;
    }

    g_interruptionCallback = std::move(callback);

    if (!g_interruptionCallback)
        return;

    // Capture the callback by reference to g_interruptionCallback so we can
    // swap it later without removing the observer again.
    g_interruptionObserver =
        [nc addObserverForName:AVAudioSessionInterruptionNotification
                        object:[AVAudioSession sharedInstance]
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification* note)
    {
        NSNumber* typeValue = note.userInfo[AVAudioSessionInterruptionTypeKey];
        if (typeValue == nil)
            return;

        auto interruptionType =
            static_cast<AVAudioSessionInterruptionType>(typeValue.unsignedIntegerValue);

        if (interruptionType == AVAudioSessionInterruptionTypeBegan)
        {
            // Interruption started (call, Siri, alarm…). Mark session inactive
            // so audio-thread queries return the correct state.
            g_sessionActive.store(false, std::memory_order_release);

            if (g_interruptionCallback)
                g_interruptionCallback(true);
        }
        else if (interruptionType == AVAudioSessionInterruptionTypeEnded)
        {
            // Check whether we should actually resume.
            // Some interruptions (e.g., user starts Music app) end without
            // AVAudioSessionInterruptionOptionShouldResume — in that case we
            // do NOT call the callback and let the user re-engage explicitly.
            NSNumber* optionsValue = note.userInfo[AVAudioSessionInterruptionOptionKey];
            AVAudioSessionInterruptionOptions options =
                optionsValue ? optionsValue.unsignedIntegerValue : 0;

            if (options & AVAudioSessionInterruptionOptionShouldResume)
            {
                // Re-activate the session before resuming the engine.
                NSError* err = nil;
                [[AVAudioSession sharedInstance] setActive:YES error:&err];
                if (err)
                    NSLog(@"[XOceanus] Re-activate after interruption: %@", err.localizedDescription);

                g_sessionActive.store(true, std::memory_order_release);
                updateHardwareMetrics();

                if (g_interruptionCallback)
                    g_interruptionCallback(false);
            }
        }
    }];
}

//------------------------------------------------------------------------------
void handleRouteChange(std::function<void()> callback)
{
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

    if (g_routeChangeObserver != nil)
    {
        [nc removeObserver:g_routeChangeObserver];
        g_routeChangeObserver = nil;
    }

    g_routeChangeCallback = std::move(callback);

    if (!g_routeChangeCallback)
        return;

    g_routeChangeObserver =
        [nc addObserverForName:AVAudioSessionRouteChangeNotification
                        object:[AVAudioSession sharedInstance]
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification* note)
    {
        // Log the reason for diagnostic convenience; the callback itself is
        // unconditional — the engine decides how to react.
        NSNumber* reasonValue = note.userInfo[AVAudioSessionRouteChangeReasonKey];
        if (reasonValue)
        {
            auto reason = static_cast<AVAudioSessionRouteChangeReason>(
                              reasonValue.unsignedIntegerValue);
            switch (reason)
            {
                case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
                    NSLog(@"[XOceanus] Route change: new device available (headphones/BT connected)");
                    break;
                case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
                    NSLog(@"[XOceanus] Route change: old device unavailable (headphones unplugged)");
                    break;
                case AVAudioSessionRouteChangeReasonCategoryChange:
                    NSLog(@"[XOceanus] Route change: category changed");
                    break;
                case AVAudioSessionRouteChangeReasonOverride:
                    NSLog(@"[XOceanus] Route change: override");
                    break;
                case AVAudioSessionRouteChangeReasonWakeFromSleep:
                    NSLog(@"[XOceanus] Route change: wake from sleep");
                    break;
                default:
                    NSLog(@"[XOceanus] Route change: reason %lu",
                          static_cast<unsigned long>(reason));
                    break;
            }
        }

        // Update cached metrics — the hardware route may have changed its
        // supported sample rate or buffer size.
        updateHardwareMetrics();

        if (g_routeChangeCallback)
            g_routeChangeCallback();
    }];
}

//------------------------------------------------------------------------------
bool isActive()
{
    return g_sessionActive.load(std::memory_order_acquire);
}

//------------------------------------------------------------------------------
float currentSampleRate()
{
    return g_sampleRate.load(std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
int currentBufferSize()
{
    return g_bufferSize.load(std::memory_order_relaxed);
}

//------------------------------------------------------------------------------
void shutdown()
{
    NSNotificationCenter* nc = [NSNotificationCenter defaultCenter];

    if (g_interruptionObserver != nil)
    {
        [nc removeObserver:g_interruptionObserver];
        g_interruptionObserver = nil;
    }

    if (g_routeChangeObserver != nil)
    {
        [nc removeObserver:g_routeChangeObserver];
        g_routeChangeObserver = nil;
    }

    // Clear callbacks so retained blocks cannot fire after shutdown.
    g_interruptionCallback = nullptr;
    g_routeChangeCallback  = nullptr;

    // Deactivate only if we are the owner (standalone, not AUv3).
    if (!isRunningAsAUv3Extension() && g_sessionActive.load(std::memory_order_acquire))
    {
        NSError* error = nil;
        [[AVAudioSession sharedInstance] setActive:NO
                                       withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                             error:&error];
        if (error)
            NSLog(@"[XOceanus] AVAudioSession deactivate: %@", error.localizedDescription);
    }

    g_sessionActive.store(false, std::memory_order_release);
    g_sampleRate.store(0.0f, std::memory_order_relaxed);
    g_bufferSize.store(0,    std::memory_order_relaxed);
}

} // namespace xoceanus::audio_session

#endif // JUCE_IOS
