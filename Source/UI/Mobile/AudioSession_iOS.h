// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <functional>

//==============================================================================
// AudioSession_iOS — AVAudioSession management bridge for XOceanus iOS.
//
// Standalone mode only. In AUv3 mode the host owns the audio session and
// configure() is a no-op (detected via .appex bundle path).
//
// Usage:
//   // On app launch (main thread):
//   xoceanus::audio_session::configure();
//   xoceanus::audio_session::handleInterruption([](bool began) {
//       if (began) myEngine.pause(); else myEngine.resume();
//   });
//   xoceanus::audio_session::handleRouteChange([]() {
//       myEngine.updateOutputRouting();
//   });
//
//   // Query at any time:
//   float sr  = xoceanus::audio_session::currentSampleRate();
//   int   buf = xoceanus::audio_session::currentBufferSize();
//
//   // On app termination:
//   xoceanus::audio_session::shutdown();
//
// Thread safety:
//   All functions must be called from the main thread.
//   Notification handlers are dispatched on the main thread (the thread on
//   which the observers are registered).  The registered callbacks are
//   therefore also invoked on the main thread — do not call audio-thread-
//   only APIs from inside them.
//
#if JUCE_IOS

namespace xoceanus::audio_session {

//==============================================================================
// configure() — Activate the AVAudioSession for standalone synth use.
//
// Sets category to AVAudioSessionCategoryPlayback, preferred sample rate to
// 48 000 Hz, and preferred I/O buffer duration to 128/48000 ≈ 2.67 ms.
//
// Skips all configuration when running inside an AUv3 (.appex) bundle because
// in that context the host is responsible for the session.
//
// Must be called once from the main thread before the audio engine starts.
//
void configure();

//==============================================================================
// handleInterruption() — Register a callback for audio interruptions.
//
// The callback receives `true` when an interruption begins (phone call,
// alarm, Siri, etc.) and `false` when the interruption ends.
//
// On ended, the callback is only invoked when AVAudioSession signals that
// the session should resume (AVAudioSessionInterruptionOptionShouldResume).
// If the session should not resume (e.g., the user started another audio
// app) the callback is not called.
//
// Pass nullptr to unregister a previously registered handler.
//
void handleInterruption(std::function<void(bool began)> callback);

//==============================================================================
// handleRouteChange() — Register a callback for audio route changes.
//
// Called whenever the hardware audio route changes: headphone plug/unplug,
// Bluetooth connect/disconnect, AirPlay, dock connector, etc.
//
// The callback carries no parameters — the caller should re-query the current
// route from AVAudioSession if it needs to know what changed.
//
// Pass nullptr to unregister a previously registered handler.
//
void handleRouteChange(std::function<void()> callback);

//==============================================================================
// isActive() — Returns true if the AVAudioSession is currently active.
//
bool isActive();

//==============================================================================
// currentSampleRate() — Hardware sample rate in Hz (e.g. 48000.0, 44100.0).
//
// Returns 0.0 if the session has not been configured.
//
float currentSampleRate();

//==============================================================================
// currentBufferSize() — Hardware I/O buffer size in frames.
//
// Returns 0 if the session has not been configured.
//
int currentBufferSize();

//==============================================================================
// shutdown() — Remove all notification observers and deactivate the session.
//
// Call from applicationWillTerminate / sceneDidDisconnect.
//
void shutdown();

} // namespace xoceanus::audio_session

#endif // JUCE_IOS
