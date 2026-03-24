#pragma once
#include <juce_core/juce_core.h>
#include <atomic>

#if XO_MOBILE
// Forward declarations — implemented in HapticEngine_iOS.mm
namespace xolokun::haptic_platform {
    void prepareImpact(int style);
    void fireImpact(int style, float intensity);
    void fireNotification(int type);
    void fireSelection();
}
#endif

namespace xolokun {

//==============================================================================
// HapticEngine — Contextual tactile feedback for musical events.
//
// Provides a typed API for haptic events so that each musical action
// has a distinct feel. Pre-prepares haptic patterns for minimum latency.
//
// Haptic language:
//   - Pad strike: light impact proportional to velocity
//   - Knob detent: selection tick (crisp, minimal)
//   - Strip snap-back: medium impact (physical rebound)
//   - Preset load: success notification (satisfying confirmation)
//   - Boundary hit: rigid impact (Orbit Path edge contact)
//   - PANIC: heavy impact (emergency stop)
//
// Latency strategy:
//   Pre-prepared patterns fire on the touch callback thread,
//   BEFORE the audio callback returns sound. This creates the
//   illusion that the instrument responds instantly.
//
// Platform:
//   - iOS: UIImpactFeedbackGenerator, UINotificationFeedbackGenerator,
//          UISelectionFeedbackGenerator via Objective-C++ bridge
//   - macOS: no-op (no Taptic Engine)
//
class HapticEngine {
public:
    // Haptic event types — each maps to a specific tactile pattern
    enum class Event {
        PadStrike,          // Note on — light impact, velocity-scaled
        PadRelease,         // Note off — ultra-light (optional, subtle)
        KnobDetent,         // Knob hits a detent position — selection tick
        StripSnapBack,      // Performance Strip spring return — medium impact
        PresetLoaded,       // Preset change complete — success notification
        OrbitBoundary,      // Orbit Path hits circle edge — rigid impact
        Panic,              // PANIC pad — heavy impact
        ModeSwitch,         // PlaySurface mode change — light impact
        DrawerSnap          // Parameter drawer snaps to position — medium
    };

    HapticEngine()
    {
        #if XO_MOBILE
        // Pre-prepare all impact generators for minimum latency.
        // UIImpactFeedbackGenerator.prepare() pre-spins the Taptic Engine
        // so the first fire is as fast as subsequent ones.
        haptic_platform::prepareImpact(0);  // light
        haptic_platform::prepareImpact(1);  // medium
        haptic_platform::prepareImpact(2);  // heavy
        haptic_platform::prepareImpact(3);  // rigid
        #endif
    }

    // Fire a haptic event. Call from the message thread.
    // velocity: 0.0-1.0, used for velocity-sensitive events (PadStrike)
    void fire(Event event, float velocity = 1.0f)
    {
        if (!enabled.load(std::memory_order_relaxed))
            return;

        #if XO_MOBILE
        switch (event)
        {
            case Event::PadStrike:
                haptic_platform::fireImpact(0, juce::jlimit(0.3f, 1.0f, velocity));
                break;
            case Event::PadRelease:
                if (velocity > 0.5f)  // Only on strong releases
                    haptic_platform::fireImpact(0, 0.3f);
                break;
            case Event::KnobDetent:
                haptic_platform::fireSelection();
                break;
            case Event::StripSnapBack:
                haptic_platform::fireImpact(1, 0.6f);
                break;
            case Event::PresetLoaded:
                haptic_platform::fireNotification(0);  // success
                break;
            case Event::OrbitBoundary:
                haptic_platform::fireImpact(3, 0.8f);  // rigid
                break;
            case Event::Panic:
                haptic_platform::fireImpact(2, 1.0f);  // heavy, max
                break;
            case Event::ModeSwitch:
                haptic_platform::fireImpact(0, 0.5f);
                break;
            case Event::DrawerSnap:
                haptic_platform::fireImpact(1, 0.5f);
                break;
        }

        // Re-prepare the generator for the next fire (maintains low latency)
        rePrepareForEvent(event);
        #else
        juce::ignoreUnused(event, velocity);
        #endif
    }

    // Global enable/disable (Settings > Haptic Feedback toggle)
    void setEnabled(bool shouldEnable)
    {
        enabled.store(shouldEnable, std::memory_order_relaxed);
    }

    bool isEnabled() const
    {
        return enabled.load(std::memory_order_relaxed);
    }

private:
    std::atomic<bool> enabled { true };

    #if XO_MOBILE
    void rePrepareForEvent(Event event)
    {
        switch (event)
        {
            case Event::PadStrike:
            case Event::PadRelease:
            case Event::ModeSwitch:
                haptic_platform::prepareImpact(0);  // light
                break;
            case Event::StripSnapBack:
            case Event::DrawerSnap:
                haptic_platform::prepareImpact(1);  // medium
                break;
            case Event::Panic:
                haptic_platform::prepareImpact(2);  // heavy
                break;
            case Event::OrbitBoundary:
                haptic_platform::prepareImpact(3);  // rigid
                break;
            case Event::KnobDetent:
            case Event::PresetLoaded:
                break;  // Notification/selection generators auto-prepare
        }
    }
    #endif
};

} // namespace xolokun
