/*
  ==============================================================================

    TouchForce_iOS.mm
    XOlokun — UITouch.force side-channel for MDTV Signal 3.

    Implements all functions declared in the xolokun::touch_force namespace
    (TouchForce_iOS.h). Compiled only on iOS; the #if JUCE_IOS guard ensures
    this file produces no symbols on macOS or Simulator.

    Architecture — why a UIGestureRecognizer, not method swizzling:
      Swizzling UIView's -touchesBegan:withEvent: is fragile: JUCE may itself
      swizzle the same methods, and the insertion order becomes load-order
      dependent. Any JUCE update can silently break the chain.

      A UIGestureRecognizer added to the view participates in UIKit's standard
      gesture pipeline with well-defined semantics:
        - cancelsTouchesInView = NO  → does NOT cancel touches going to JUCE
        - delaysTouchesBegan   = NO  → UIKit delivers Began to JUCE immediately
        - delaysTouchesEnded   = NO  → UIKit delivers Ended to JUCE immediately
      The recognizer stays in UIGestureRecognizerStatePossible permanently and
      never transitions to Recognized, so UIKit never cancels JUCE's touches.

    Data layout:
      - MAX_TOUCHES = 16 slots (UIKit supports up to 10 simultaneous touches;
        16 gives comfortable headroom and stays cache-friendly at 64 bytes)
      - Slot index = touch.hash % MAX_TOUCHES
      - Each slot holds an std::atomic<float> (force, -1.0 = no data)
      - A separate std::atomic<float> gLastForce tracks the most recent event
        for callers that only need single-value queries

    Division-by-zero guard:
      On iPads and iPhones without 3D Touch, UITouch.maximumPossibleForce is
      0.0 (or very close). We check maximumPossibleForce > 0.0 before dividing.
      If not positive, we store -1.0 (no data sentinel) in the slot.

    Thread safety:
      All state is std::atomic<float> with relaxed ordering (we only need
      atomicity, not happens-before ordering between threads). Reads from the
      audio/DSP thread are always safe.

    Memory:
      The XOTouchForceRecognizer instance is stored in a __strong ObjC pointer
      inside an anonymous C++ namespace so ARC does not release it after
      install() returns. uninstall() explicitly nils the pointer and calls
      -removeGestureRecognizer: to break the UIView retain cycle.

  ==============================================================================
*/

#include "TouchForce_iOS.h"

#if JUCE_IOS

#import  <UIKit/UIKit.h>
#include <atomic>
#include <juce_gui_basics/juce_gui_basics.h>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr int MAX_TOUCHES = 16;

// ---------------------------------------------------------------------------
// Module-private storage
// ---------------------------------------------------------------------------

namespace
{
    // Per-touch force slots, indexed by touch.hash % MAX_TOUCHES.
    // Initialised to -1.0 (no data).
    std::atomic<float> gForceSlots[MAX_TOUCHES];

    // Most recent force value from any touch, any finger.
    std::atomic<float> gLastForce { -1.0f };

    // Whether the current device reported UIForceTouchCapabilityAvailable.
    // Set once in install(); reads are always safe (relaxed).
    std::atomic<bool> gForceAvailable { false };

    // The gesture recognizer we added to the JUCE view.
    // Kept alive by this __strong reference.
    // NSObject* rather than the forward-declared class so the anonymous
    // namespace doesn't need to see the full @interface before it's defined.
    __strong UIGestureRecognizer* gRecognizer = nil;

    // Write normalised force for a single UITouch into the slot array.
    // Called from all three touch phase callbacks.
    inline void storeForce(UITouch* __unsafe_unretained touch)
    {
        const CGFloat maxForce = touch.maximumPossibleForce;
        float normalised = -1.0f;
        if (maxForce > 0.0)
            normalised = static_cast<float>(touch.force / maxForce);

        const int slot = static_cast<int>(touch.hash % static_cast<NSUInteger>(MAX_TOUCHES));
        gForceSlots[slot].store(normalised, std::memory_order_relaxed);
        gLastForce.store(normalised, std::memory_order_relaxed);
    }

    // Clear the slot for a touch that has ended or been cancelled.
    inline void clearSlot(UITouch* __unsafe_unretained touch)
    {
        const int slot = static_cast<int>(touch.hash % static_cast<NSUInteger>(MAX_TOUCHES));
        gForceSlots[slot].store(-1.0f, std::memory_order_relaxed);
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// XOTouchForceRecognizer — UIGestureRecognizer subclass
// ---------------------------------------------------------------------------
//
// Intercepts all touch phases to read UITouch.force before JUCE's own
// input pipeline converts it to a MouseEvent (which drops the force data).
//
// Key properties:
//   cancelsTouchesInView = NO   — JUCE's view still receives every touch
//   delaysTouchesBegan   = NO   — no added latency on note-on events
//   delaysTouchesEnded   = NO   — no added latency on note-off events
//
// The recognizer never calls [self setState:UIGestureRecognizerStateRecognized]
// so UIKit never sends -touchesCancelled to JUCE's view on our behalf.

@interface XOTouchForceRecognizer : UIGestureRecognizer
@end

@implementation XOTouchForceRecognizer

- (instancetype)init
{
    self = [super initWithTarget:nil action:nil];
    if (self)
    {
        self.cancelsTouchesInView = NO;
        self.delaysTouchesBegan   = NO;
        self.delaysTouchesEnded   = NO;
    }
    return self;
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    @autoreleasepool
    {
        for (UITouch* touch in touches)
            storeForce(touch);
    }
    // Do NOT call super — we are the terminal node for force data only.
    // Touches are still delivered to the view by UIKit (cancelsTouchesInView=NO).
    juce::ignoreUnused(event);
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    @autoreleasepool
    {
        for (UITouch* touch in touches)
            storeForce(touch);
    }
    juce::ignoreUnused(event);
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    @autoreleasepool
    {
        for (UITouch* touch in touches)
        {
            // Capture the final force reading before clearing the slot.
            storeForce(touch);
            clearSlot(touch);
        }
    }
    juce::ignoreUnused(event);
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event
{
    @autoreleasepool
    {
        for (UITouch* touch in touches)
            clearSlot(touch);
    }
    juce::ignoreUnused(event);
}

@end

// ---------------------------------------------------------------------------
// touch_force implementation
// ---------------------------------------------------------------------------

namespace xolokun { namespace touch_force {

void install()
{
    // Idempotent — safe to call multiple times.
    if (gRecognizer != nil)
        return;

    @autoreleasepool
    {
        // ----------------------------------------------------------------
        // 1. Determine force availability on this device.
        // ----------------------------------------------------------------
        //
        // UIForceTouchCapabilityAvailable requires iOS 9+ and a Taptic Engine
        // that supports pressure sensing. On all iPads and non-Pro iPhones 11+
        // this evaluates to UIForceTouchCapabilityUnavailable.
        //
        // We check once at install time. Device capabilities don't change at
        // runtime (you can't hot-plug a Taptic Engine).

        BOOL forceAvail = NO;
        if (@available(iOS 9.0, *))
        {
            UITraitCollection* traits = UIScreen.mainScreen.traitCollection;
            forceAvail = (traits.forceTouchCapability == UIForceTouchCapabilityAvailable);
        }
        gForceAvailable.store(forceAvail, std::memory_order_relaxed);

        // ----------------------------------------------------------------
        // 2. Initialise all force slots to -1.0 (no data).
        // ----------------------------------------------------------------
        for (int i = 0; i < MAX_TOUCHES; ++i)
            gForceSlots[i].store(-1.0f, std::memory_order_relaxed);
        gLastForce.store(-1.0f, std::memory_order_relaxed);

        // ----------------------------------------------------------------
        // 3. Locate the JUCE editor's UIView.
        // ----------------------------------------------------------------
        //
        // juce::ComponentPeer::getNativeHandle() returns a void* that on iOS
        // points to the UIView backing the component. We must cast through
        // __bridge to hand ownership to ARC correctly.
        //
        // We walk the JUCE component hierarchy to find the top-level editor.
        // Desktop::getInstance().getComponent(0) is the root Component on iOS
        // (the standalone app's main window or the AUv3 editor root). If it has
        // no peer yet (called too early), we bail and rely on the caller
        // to call install() again once the editor is on screen.

        juce::Component* rootComponent = nullptr;
        if (juce::Desktop::getNumInstances() > 0)
        {
            auto& desktop = juce::Desktop::getInstance();
            if (desktop.getNumComponents() > 0)
                rootComponent = desktop.getComponent(0);
        }

        if (rootComponent == nullptr)
            return;  // Editor not yet visible — caller should retry.

        juce::ComponentPeer* peer = rootComponent->getPeer();
        if (peer == nullptr)
            return;

        UIView* juceView = (__bridge UIView*) peer->getNativeHandle();
        if (juceView == nil)
            return;

        // ----------------------------------------------------------------
        // 4. Create and attach the recognizer.
        // ----------------------------------------------------------------

        XOTouchForceRecognizer* rec = [[XOTouchForceRecognizer alloc] init];
        [juceView addGestureRecognizer:rec];
        gRecognizer = rec;  // Retain via __strong.
    }
}

void uninstall()
{
    if (gRecognizer == nil)
        return;

    @autoreleasepool
    {
        UIView* view = gRecognizer.view;
        if (view != nil)
            [view removeGestureRecognizer:gRecognizer];

        gRecognizer = nil;

        // Clear all stored data.
        for (int i = 0; i < MAX_TOUCHES; ++i)
            gForceSlots[i].store(-1.0f, std::memory_order_relaxed);
        gLastForce.store(-1.0f, std::memory_order_relaxed);
    }
}

float getForceForTouch(int touchHash)
{
    if (!gForceAvailable.load(std::memory_order_relaxed))
        return -1.0f;

    const int slot = touchHash % MAX_TOUCHES;
    return gForceSlots[slot].load(std::memory_order_relaxed);
}

float getLastForce()
{
    return gLastForce.load(std::memory_order_relaxed);
}

bool isForceAvailable()
{
    return gForceAvailable.load(std::memory_order_relaxed);
}

}} // namespace xolokun::touch_force

#endif // JUCE_IOS
