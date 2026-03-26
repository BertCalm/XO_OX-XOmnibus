//==============================================================================
// DisplayLink_iOS.mm — CADisplayLink bridge for XOlokun iOS animation.
//
// Delivers per-frame callbacks from CADisplayLink into the JUCE message thread
// via juce::MessageManager::callAsync so that Component::repaint() and other
// UI mutations are always made from the correct thread context.
//
// Architecture
// ─────────────
// ┌───────────────────────────────────────────────────────┐
// │  CADisplayLink (main run loop, kCFRunLoopCommonModes) │
// │    ↓ -displayLinkFired: (Objective-C selector)        │
// │    ↓ captures timestamp / targetTimestamp             │
// │    ↓ juce::MessageManager::callAsync(lambda)          │
// │         ↓ (≈ 1 frame later, JUCE message thread)      │
// │         ↓ invoke stored std::function callback        │
// └───────────────────────────────────────────────────────┘
//
// The callAsync bounce is acknowledged in QDD LVL 4 as acceptable latency
// for UI animation purposes.  It is intentional: it keeps all JUCE Component
// access on the message thread regardless of how the CADisplayLink is
// configured.
//
// Thread safety
// ─────────────
// - g_running is a std::atomic<bool> — readable from any thread.
// - All other state (g_callback, g_displayLink, g_preferredFPS) is accessed
//   only from the main thread.  No locks are needed for those fields.
// - The callAsync lambda captures timestamp values by value, and tests
//   g_running (atomic read) before invoking the callback so that callbacks
//   queued before stop() was called are silently discarded.
//
// ProMotion (iOS 15+)
// ───────────────────
// CADisplayLink.preferredFrameRateRange (CAFrameRateRange) replaced the
// deprecated .preferredFramesPerSecond on iOS 15.  We use @available checks
// at runtime to select the right API, keeping backward compatibility with
// iOS 14 and below.
//
#include "DisplayLink_iOS.h"

#if JUCE_IOS

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#include <juce_events/juce_events.h>
#include <atomic>
#include <functional>
#include <mutex>

//==============================================================================
// XODisplayLinkTarget — Objective-C target object for CADisplayLink.
//
// CADisplayLink requires a target + selector pair.  This lightweight ObjC
// class bridges the selector callback into the C++ storage in the anonymous
// namespace below.
//
@interface XODisplayLinkTarget : NSObject
- (void)displayLinkFired:(CADisplayLink*)link;
@end

//==============================================================================
// Translation-unit private state
//
namespace
{

//-- Callback wrapper ----------------------------------------------------------
// Protected by the main-thread-only constraint: read/write only on main thread.
// The lambda inside callAsync tests g_running atomically before calling this.
std::function<void(double, double)> g_callback;

//-- CADisplayLink + ObjC target -----------------------------------------------
CADisplayLink* __strong g_displayLink = nil;
XODisplayLinkTarget* __strong g_target = nil;

//-- Running flag — read from any thread, written from main thread only --------
std::atomic<bool> g_running { false };

//-- Preferred FPS — stored so setPreferredFrameRate() works before start() ---
int g_preferredFPS = 0;  // 0 = system-native rate

//-- Apply the current g_preferredFPS to the live display link ----------------
// Must be called from the main thread with g_displayLink non-nil.
void applyFrameRatePreference()
{
    if (g_displayLink == nil)
        return;

    const int fps = g_preferredFPS;

    if (@available(iOS 15.0, *))
    {
        // CAFrameRateRange: min / max / preferred.
        // fps == 0 means "let the system decide" — we use maximumFramesPerSecond
        // as the ceiling, with preferred == max so iOS grants the full rate.
        const int maxFPS = (int)[UIScreen mainScreen].maximumFramesPerSecond;
        const int resolvedMax       = (fps > 0) ? fps : maxFPS;
        const int resolvedMin       = (fps > 0) ? std::max(1, fps / 2) : 1;
        const int resolvedPreferred = resolvedMax;

        g_displayLink.preferredFrameRateRange =
            CAFrameRateRangeMake((float)resolvedMin,
                                 (float)resolvedMax,
                                 (float)resolvedPreferred);
    }
    else
    {
        // iOS < 15: use deprecated preferredFramesPerSecond.
        // Assigning 0 tells CADisplayLink to match the display refresh rate.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        g_displayLink.preferredFramesPerSecond = fps;
#pragma clang diagnostic pop
    }
}

} // anonymous namespace

//==============================================================================
// XODisplayLinkTarget implementation
//
@implementation XODisplayLinkTarget

- (void)displayLinkFired:(CADisplayLink*)link
{
    @autoreleasepool
    {
        // Capture timestamps by value before the async bounce — the link
        // object must not be retained across threads.
        const double ts       = link.timestamp;
        const double targetTs = link.targetTimestamp;

        // juce::MessageManager::callAsync posts a lambda to the JUCE message
        // queue.  Even though CADisplayLink already fires on the main thread
        // (same thread as the JUCE message loop on iOS), the callAsync ensures
        // we are executing inside JUCE's own dispatch machinery, which is the
        // correct context for Component::repaint() calls and MessageManagerLock
        // assumptions elsewhere in the codebase.
        juce::MessageManager::callAsync([ts, targetTs]()
        {
            // Guard: if stop() was called between the display-link fire and
            // this lambda executing, discard the callback.
            if (!g_running.load(std::memory_order_acquire))
                return;

            if (g_callback)
                g_callback(ts, targetTs);
        });
    }
}

@end

//==============================================================================
// Public API implementation
//
namespace xolokun { namespace display_link {

//------------------------------------------------------------------------------
void start(std::function<void(double, double)> callback)
{
    // If already running, replace the callback without rebuilding the link.
    // The old callback will not be called again after this assignment because
    // -displayLinkFired: reads g_callback inside callAsync (main thread),
    // which happens after this assignment completes (also main thread).
    g_callback = std::move(callback);

    if (g_running.load(std::memory_order_acquire))
    {
        NSLog(@"[XOlokun] DisplayLink: callback replaced (link already running)");
        return;
    }

    // Create the ObjC target if it doesn't exist yet.
    if (g_target == nil)
        g_target = [[XODisplayLinkTarget alloc] init];

    // Create a new CADisplayLink targeting our selector.
    g_displayLink = [CADisplayLink displayLinkWithTarget:g_target
                                               selector:@selector(displayLinkFired:)];

    // Apply any frame rate preference that was set before start() was called.
    applyFrameRatePreference();

    // Schedule on the main run loop.  kCFRunLoopCommonModes ensures callbacks
    // fire during scroll views and UITrackingRunLoopMode, not just the default
    // run loop mode — important for animation during touch interaction.
    [g_displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                        forMode:NSRunLoopCommonModes];

    g_running.store(true, std::memory_order_release);

    NSLog(@"[XOlokun] DisplayLink: started (preferredFPS=%d, displayHz=%d)",
          g_preferredFPS, displayRefreshRate());
}

//------------------------------------------------------------------------------
void stop()
{
    if (!g_running.load(std::memory_order_acquire))
        return;

    // Set running = false BEFORE invalidating so that any callAsync lambdas
    // that are already queued will see false and discard themselves.
    g_running.store(false, std::memory_order_release);

    if (g_displayLink != nil)
    {
        [g_displayLink invalidate];
        g_displayLink = nil;
    }

    // Clear the callback to release any captured state (e.g., Component
    // pointers) immediately rather than waiting for the next GC cycle.
    g_callback = nullptr;

    NSLog(@"[XOlokun] DisplayLink: stopped");
}

//------------------------------------------------------------------------------
bool isRunning()
{
    return g_running.load(std::memory_order_acquire);
}

//------------------------------------------------------------------------------
void setPreferredFrameRate(int fps)
{
    g_preferredFPS = fps;

    // Apply immediately if a live display link exists.
    if (g_running.load(std::memory_order_acquire) && g_displayLink != nil)
        applyFrameRatePreference();
}

//------------------------------------------------------------------------------
int displayRefreshRate()
{
    // UIScreen.mainScreen.maximumFramesPerSecond is available on iOS 10.3+.
    // Returns 120 on ProMotion devices, 60 on all others.
    // Returns 60 as a safe default when UIKit is not yet fully initialised.
    UIScreen* screen = [UIScreen mainScreen];
    if (screen == nil)
        return 60;

    return (int)screen.maximumFramesPerSecond;
}

}} // namespace xolokun::display_link

#endif // JUCE_IOS
