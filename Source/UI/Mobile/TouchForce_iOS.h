#pragma once

#if JUCE_IOS

#include <atomic>

namespace xolokun { namespace touch_force {

//==============================================================================
// TouchForce_iOS — UITouch.force side-channel for MDTV Signal 3.
//
// Problem:
//   JUCE's mouse event abstraction normalises platform input into a single
//   MouseEvent struct. On iOS, UITouch.force (3D Touch / Force Touch pressure
//   data, 0.0–maximumPossibleForce) is silently discarded before JUCE constructs
//   its MouseInputSource. MobileTouchHandler's TouchEvent.force field therefore
//   has no data source — it falls back to a constant 0.5.
//
// Solution:
//   A custom UIGestureRecognizer subclass (XOTouchForceRecognizer) is attached
//   to the JUCE editor's UIView. It fires alongside JUCE's own gesture pipeline,
//   reads UITouch.force from every touchesBegan/touchesMoved/touchesEnded call,
//   and writes normalised force values into a lock-free atomic array indexed by
//   touch hash. MobileTouchHandler calls getForceForTouch() on each JUCE mouse
//   event to populate TouchEvent.force from this side-channel.
//
// MDTV context:
//   This is Signal 3 of the Multi-Dimensional Temporal Velocity proposal from
//   the QDD audit. Signals 1 (velocity) and 2 (radius) are already populated by
//   MobileTouchHandler. Signal 3 (force) closes the MDTV triangle.
//
// Thread safety:
//   All storage is std::atomic<float>. Reads and writes from any thread are safe.
//   The gesture recognizer callbacks fire on the main thread; callers on the
//   audio/DSP thread may read via getForceForTouch() without acquiring a lock.
//
// Device support:
//   - iPhone 6s, 6s Plus, 7–8 (all), SE 2nd/3rd gen, X–15 Pro: force available.
//   - iPhone 11–15 (non-Pro): UIForceTouchCapabilityUnavailable (no 3D Touch).
//     maximumPossibleForce is 0.0 on these; we return -1.0 from getForceForTouch().
//   - All iPads: no 3D Touch. isForceAvailable() returns false.
//   - Simulator: no force hardware. isForceAvailable() returns false.
//   MobileTouchHandler must check isForceAvailable() and fall back to radius-
//   derived proxy when this returns false.
//
// Installation:
//   Call install() once after the JUCE plugin editor is created and visible.
//   Call uninstall() in the editor destructor.

// ---------------------------------------------------------------------------
// Lifecycle

/// Install the UITouch.force gesture recognizer on the JUCE plugin editor's
/// UIView. Call once after the editor component is created and visible. Safe
/// to call multiple times — subsequent calls are no-ops.
///
/// Internally, this function:
///   1. Obtains the native UIView via juce::Component::getPeer()->getNativeHandle()
///   2. Allocates an XOTouchForceRecognizer with cancelsTouchesInView=NO
///   3. Adds it to the view so it runs alongside JUCE's own recognizers
///   4. Queries UIScreen.mainScreen.traitCollection.forceTouchCapability and
///      caches the result for isForceAvailable()
void install();

/// Remove the force recognizer and clear all stored force data.
/// Call in the JUCE editor destructor before the UIView is torn down.
void uninstall();

// ---------------------------------------------------------------------------
// Queries (thread-safe; call from any thread)

/// Return the normalised force (0.0–1.0) for the touch identified by
/// touchHash, or -1.0 if:
///   - the device does not support force sensing, or
///   - no touch with this hash is currently tracked.
///
/// touchHash should be UITouch.hash (an NSUInteger). Because JUCE exposes
/// the touch hash via MouseInputSource::getIndex() mapped through an internal
/// table, callers should pass the raw ObjC hash retrieved via the side-channel
/// rather than a JUCE source index. See MobileTouchHandler for the bridge.
float getForceForTouch(int touchHash);

/// Return the normalised force of the most recent touch event from any finger.
/// Useful when you only need "how hard is the active touch?" without tracking
/// per-finger identity. Returns -1.0 if force data is unavailable.
float getLastForce();

/// Returns true if the current device reports UIForceTouchCapabilityAvailable.
/// On devices without 3D Touch (all iPads, iPhone 11 non-Pro, Simulator) this
/// returns false; MobileTouchHandler should then derive force from touch radius.
bool isForceAvailable();

}} // namespace xolokun::touch_force

#endif // JUCE_IOS
