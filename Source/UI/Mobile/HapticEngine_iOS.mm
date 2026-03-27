/*
  ==============================================================================

    HapticEngine_iOS.mm
    XOlokun — Objective-C++ bridge for UIKit haptic feedback generators.

    Implements all functions declared in the xolokun::haptic_platform namespace
    (HapticEngine.h). Compiled only on iOS; the #if JUCE_IOS guard ensures this
    file produces no symbols on macOS.

    Design notes:
    - Four UIImpactFeedbackGenerator instances (light/medium/heavy/rigid) are
      pre-allocated and -prepare()'d once at module load so the Taptic Engine
      is warm before the first finger touches a pad.
    - Apple requires a call to -prepare after each -impactOccurred / -selectionChanged /
      -notificationOccurred to maintain low latency on subsequent fires.
    - All generators are wrapped in @autoreleasepool blocks to ensure prompt
      deallocation of temporary ObjC objects on the message thread.
    - On devices without a Taptic Engine (older iPads, Simulator) all
      UIFeedbackGenerator calls are silently ignored by UIKit — no guard needed,
      but we nil-check to be explicit.
    - These functions MUST be called from the message thread (JUCE main thread).
      Never call from the audio callback thread.

  ==============================================================================
*/

#include "HapticEngine.h"

#if JUCE_IOS

#import <UIKit/UIKit.h>

// ---------------------------------------------------------------------------
// Module-private generator storage
// ---------------------------------------------------------------------------
//
// Stored as __strong ObjC pointers inside a plain C++ anonymous namespace so
// they survive across multiple calls without ARC releasing them prematurely.
// Initialised lazily on first use inside an @autoreleasepool — safe because
// all call sites are on the message thread (single-threaded from JUCE's
// perspective).

namespace
{
    // Impact generators — indexed by style (matching HapticEngine.h convention):
    //   0 = light, 1 = medium, 2 = heavy, 3 = rigid
    UIImpactFeedbackGenerator* __strong  gImpact[4]  = { nil, nil, nil, nil };
    UINotificationFeedbackGenerator* __strong gNotify = nil;
    UISelectionFeedbackGenerator* __strong    gSelect = nil;

    // Lazily allocate + prepare a single impact generator at the given index.
    // Must be called on the message thread.
    inline UIImpactFeedbackGenerator* impactGenerator(int style)
    {
        if (style < 0 || style > 3)
            return nil;

        if (gImpact[style] == nil)
        {
            @autoreleasepool
            {
                UIImpactFeedbackStyle uiStyle;
                switch (style)
                {
                    case 0:  uiStyle = UIImpactFeedbackStyleLight;  break;
                    case 1:  uiStyle = UIImpactFeedbackStyleMedium; break;
                    case 2:  uiStyle = UIImpactFeedbackStyleHeavy;  break;
                    case 3:
                    default:
                        // UIImpactFeedbackStyleRigid is iOS 13+.
                        // Fall back to heavy on older OS versions gracefully.
                        if (@available(iOS 13.0, *))
                            uiStyle = UIImpactFeedbackStyleRigid;
                        else
                            uiStyle = UIImpactFeedbackStyleHeavy;
                        break;
                }
                gImpact[style] = [[UIImpactFeedbackGenerator alloc] initWithStyle:uiStyle];
            }
        }
        return gImpact[style];
    }

    // Lazily allocate notification generator.
    inline UINotificationFeedbackGenerator* notifyGenerator()
    {
        if (gNotify == nil)
        {
            @autoreleasepool
            {
                gNotify = [[UINotificationFeedbackGenerator alloc] init];
            }
        }
        return gNotify;
    }

    // Lazily allocate selection generator.
    inline UISelectionFeedbackGenerator* selectGenerator()
    {
        if (gSelect == nil)
        {
            @autoreleasepool
            {
                gSelect = [[UISelectionFeedbackGenerator alloc] init];
            }
        }
        return gSelect;
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// haptic_platform implementation
// ---------------------------------------------------------------------------

namespace xolokun::haptic_platform
{

// Warms up the Taptic Engine for the given impact style so the first fire
// has minimum latency. Call once per style at startup (HapticEngine ctor
// calls all four), and again after each fire (Apple requirement).
//
// style: 0 = light, 1 = medium, 2 = heavy, 3 = rigid
void prepareImpact(int style)
{
    @autoreleasepool
    {
        UIImpactFeedbackGenerator* gen = impactGenerator(style);
        if (gen != nil)
            [gen prepare];
    }
}

// Fire an impact haptic with a given normalised intensity.
//
// style:     0 = light, 1 = medium, 2 = heavy, 3 = rigid
// intensity: 0.0–1.0 (clamped). Uses -impactOccurredWithIntensity: on
//            iOS 13+ for velocity-sensitive events (PadStrike). Falls back
//            to -impactOccurred on older OS versions.
//
// After firing, the caller (HapticEngine::rePrepareForEvent) calls
// prepareImpact() again to keep the Taptic Engine warm.
void fireImpact(int style, float intensity)
{
    @autoreleasepool
    {
        UIImpactFeedbackGenerator* gen = impactGenerator(style);
        if (gen == nil)
            return;

        const CGFloat clampedIntensity = static_cast<CGFloat>(
            intensity < 0.0f ? 0.0f : (intensity > 1.0f ? 1.0f : intensity));

        if (@available(iOS 13.0, *))
        {
            [gen impactOccurredWithIntensity:clampedIntensity];
        }
        else
        {
            // Pre-iOS 13: intensity is not available; fire unconditionally.
            [gen impactOccurred];
        }
    }
}

// Fire a notification haptic.
//
// type: 0 = success, 1 = warning, 2 = error
// Maps to UINotificationFeedbackType values.
//
// UINotificationFeedbackGenerator auto-prepares between fires per Apple docs,
// but we keep the generator alive to avoid re-allocation overhead.
void fireNotification(int type)
{
    @autoreleasepool
    {
        UINotificationFeedbackGenerator* gen = notifyGenerator();
        if (gen == nil)
            return;

        UINotificationFeedbackType uiType;
        switch (type)
        {
            case 0:  uiType = UINotificationFeedbackTypeSuccess; break;
            case 1:  uiType = UINotificationFeedbackTypeWarning; break;
            case 2:  uiType = UINotificationFeedbackTypeError;   break;
            default: uiType = UINotificationFeedbackTypeSuccess; break;
        }

        [gen notificationOccurred:uiType];
    }
}

// Fire the selection haptic — a crisp, minimal tick used for knob detents.
//
// UISelectionFeedbackGenerator auto-prepares between fires per Apple docs.
void fireSelection()
{
    @autoreleasepool
    {
        UISelectionFeedbackGenerator* gen = selectGenerator();
        if (gen != nil)
            [gen selectionChanged];
    }
}

} // namespace xolokun::haptic_platform

// ---------------------------------------------------------------------------
// A11y platform bridge — UIAccessibility.isReduceMotionEnabled
// ---------------------------------------------------------------------------
// Declared as xolokun::a11y_platform::isReduceMotionEnabled() in GalleryColors.h.
// Reads the system-level "Reduce Motion" accessibility setting via UIAccessibility.
// Returns true when the user has enabled Settings > Accessibility > Reduce Motion.
namespace xolokun::a11y_platform {

bool isReduceMotionEnabled()
{
    @autoreleasepool
    {
        return UIAccessibilityIsReduceMotionEnabled();
    }
}

} // namespace xolokun::a11y_platform

#endif // JUCE_IOS
