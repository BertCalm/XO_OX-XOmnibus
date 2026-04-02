// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <functional>

//==============================================================================
// DisplayLink_iOS — CADisplayLink bridge for display-synchronized animation
//                   callbacks in XOceanus JUCE Components.
//
// Wraps CADisplayLink (iOS) in a C++ API that delivers per-frame callbacks on
// the JUCE message thread, making it safe to call repaint() or manipulate
// Component state without message-thread guards.
//
// Usage:
//   // On the message thread — start callbacks at the native display rate:
//   xoceanus::display_link::start([](double ts, double targetTs)
//   {
//       myComponent.repaint();
//   });
//
//   // Request ProMotion rate (iPad Pro / iPhone 15 Pro):
//   xoceanus::display_link::setPreferredFrameRate(120);
//
//   // Query the actual hardware refresh rate:
//   int hz = xoceanus::display_link::displayRefreshRate();  // 60 or 120
//
//   // Stop when the component is hidden or destroyed:
//   xoceanus::display_link::stop();
//
// Threading model:
//   CADisplayLink fires on the main run loop (main thread). The bridge bounces
//   the callback into JUCE's message loop via juce::MessageManager::callAsync.
//   This adds approximately one display frame of latency — acknowledged in
//   QDD LVL 4 and acceptable for UI animation.  Audio-thread callers must not
//   call any function in this namespace.
//
// ProMotion (iOS 15+):
//   setPreferredFrameRate() uses CAFrameRateRange (min/max/preferred) when
//   running on iOS 15 or later.  On older iOS the deprecated
//   preferredFramesPerSecond property is used as a fallback.
//   Pass 0 to let the system choose the display's native refresh rate.
//
// macOS stub:
//   On non-iOS targets all functions are no-ops so shared UI code compiles
//   without conditional guards at every call site.
//
#if JUCE_IOS

namespace xoceanus { namespace display_link {

//==============================================================================
// start() — Attach a CADisplayLink to the main run loop and begin delivering
//            per-frame callbacks.
//
// The callback receives:
//   timestamp       — CFTimeInterval at which the current frame began (seconds
//                     since system boot, matches CADisplayLink.timestamp)
//   targetTimestamp — CFTimeInterval at which the frame will be displayed
//                     (CADisplayLink.targetTimestamp, iOS 10+)
//
// Calling start() while a display link is already running replaces the
// existing callback without stopping and re-creating the native link.  The
// frame rate preference is preserved across callback replacements.
//
// Must be called from the message thread (JUCE main thread).
//
void start(std::function<void(double /*timestamp*/, double /*targetTimestamp*/)> callback);

//==============================================================================
// stop() — Invalidate and release the CADisplayLink.
//
// Safe to call when no display link is running (no-op).  After stop(), no
// further callbacks will be delivered even if a callAsync is already queued —
// the bridge checks the running flag before invoking the stored callback.
//
// Must be called from the message thread (JUCE main thread).
//
void stop();

//==============================================================================
// isRunning() — Returns true if a CADisplayLink is currently active.
//
// Thread-safe (reads a std::atomic<bool>).
//
bool isRunning();

//==============================================================================
// setPreferredFrameRate() — Request a target frame rate from the system.
//
// fps: desired frames per second.
//   0   — system chooses (matches display's native refresh rate)
//   60  — standard rate (all devices)
//   120 — ProMotion rate (iPad Pro, iPhone 15 Pro and later)
//
// On iOS 15+  sets CADisplayLink.preferredFrameRateRange with
//   min      = max(1, fps / 2)
//   max      = fps  (0 → maximumFramesPerSecond)
//   preferred = fps (0 → maximumFramesPerSecond)
//
// On iOS < 15 sets CADisplayLink.preferredFramesPerSecond (deprecated API).
//
// Can be called before or after start().  If called before start() the
// preference is stored and applied when start() is invoked.
//
// Must be called from the message thread (JUCE main thread).
//
void setPreferredFrameRate(int fps);

//==============================================================================
// displayRefreshRate() — Query the hardware display's maximum refresh rate.
//
// Returns UIScreen.mainScreen.maximumFramesPerSecond:
//   120 on ProMotion devices (iPad Pro, iPhone 15 Pro and later)
//   60  on all other devices
//
// Returns 60 if queried before UIKit is initialised (safe default).
//
// Thread-safe (UIScreen is main-thread-only; call from the message thread).
//
int displayRefreshRate();

}} // namespace xoceanus::display_link

#else  // !JUCE_IOS — no-op stubs for macOS builds
//==============================================================================
// macOS stubs — all functions compile to nothing so shared Component code
// does not need #if JUCE_IOS guards at every call site.
//
#include <functional>

namespace xoceanus { namespace display_link {

inline void start(std::function<void(double, double)>) {}
inline void stop()                                      {}
inline bool isRunning()                                 { return false; }
inline void setPreferredFrameRate(int)                  {}
inline int  displayRefreshRate()                        { return 60; }

}} // namespace xoceanus::display_link

#endif // JUCE_IOS
