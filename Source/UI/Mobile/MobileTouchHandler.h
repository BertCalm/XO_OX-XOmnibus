#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <unordered_map>
#include <functional>
#if JUCE_IOS
#include "TouchForce_iOS.h"
#endif

namespace xoceanus {

//==============================================================================
// Touch phase mirrors UITouch phases but is platform-agnostic.
enum class TouchPhase {
    Began,
    Moved,
    Ended,
    Cancelled
};

//==============================================================================
// A single touch event carrying all information needed for musical input.
// This is the currency of the mobile input pipeline — every zone receives these.
struct TouchEvent {
    int touchId;             // Unique per finger (stable across Moved events)
    float x, y;              // Position in points (component-local coordinates)
    float normalizedX;       // 0-1 within the receiving zone
    float normalizedY;       // 0-1 within the receiving zone
    float force;             // 0.0 - 1.0 (normalized from UITouch.force or radius fallback)
    float radius;            // Touch major radius in points (fallback velocity source)
    TouchPhase phase;
    int64_t timestampMs;     // Monotonic timestamp
    float velocityX;         // Points/second (computed from consecutive Moved events)
    float velocityY;         // Points/second
};

//==============================================================================
// Callback type for zones that consume touch events.
using TouchCallback = std::function<void(const TouchEvent&)>;

//==============================================================================
// MobileTouchHandler — Multi-touch abstraction layer.
//
// Converts JUCE mouse events (which on iOS carry touch information) into
// per-finger TouchEvent structs dispatched to registered zones.
//
// Key design decisions:
//   - Each zone maintains its own active touch set (no cross-zone interference)
//   - Touch ownership is determined at Began phase and held until Ended/Cancelled
//   - Force is normalized: UITouch.force / UITouch.maximumPossibleForce
//   - On devices without force sensing, touch radius is used as velocity proxy
//   - Velocity is computed from consecutive Moved events with 60Hz sampling
//
// Thread safety:
//   - All methods called on the message thread (JUCE component callbacks)
//   - No audio-thread interaction — zones bridge to audio via atomics/FIFOs
//
class MobileTouchHandler {
public:
    static constexpr int MaxSimultaneousTouches = 10;
    static constexpr float MinTouchTargetPt = 44.0f;  // Apple HIG minimum
    static constexpr float DefaultForce = 0.5f;       // When force not available (legacy; MDTV replaces this)
    static constexpr float VelocitySmoothingFactor = 0.3f;  // EMA for touch velocity

    // MDTV tuning constants
    static constexpr int64_t MDTVApproachWindowMs = 200;   // Look back this far for approach speed
    static constexpr float   MDTVApproachRadiusPt = 20.0f; // Proximity threshold for approach detection
    static constexpr float   MDTVContactGradientPt = 10.0f; // Distance at which gradient saturates to 0

    //-- Zone registration -------------------------------------------------------

    struct TouchZone {
        juce::Rectangle<float> bounds;  // In parent component coordinates
        TouchCallback callback;
        juce::String name;              // For debugging
        bool active = true;
    };

    // Register a zone that receives touch events.
    // Returns a zone ID for later removal or bounds update.
    int addZone(const juce::String& name,
                juce::Rectangle<float> bounds,
                TouchCallback callback)
    {
        int id = nextZoneId++;
        zones[id] = { bounds, std::move(callback), name, true };
        return id;
    }

    void removeZone(int zoneId)
    {
        zones.erase(zoneId);
        // Release any touches owned by this zone
        for (auto it = activeTouches.begin(); it != activeTouches.end(); )
        {
            if (it->second.owningZoneId == zoneId)
                it = activeTouches.erase(it);
            else
                ++it;
        }
    }

    void updateZoneBounds(int zoneId, juce::Rectangle<float> newBounds)
    {
        auto it = zones.find(zoneId);
        if (it != zones.end())
            it->second.bounds = newBounds;
    }

    void setZoneActive(int zoneId, bool active)
    {
        auto it = zones.find(zoneId);
        if (it != zones.end())
            it->second.active = active;
    }

    //-- JUCE mouse event handlers -----------------------------------------------
    // Call these from your Component's mouse callbacks.

    void handleMouseDown(const juce::MouseEvent& e)
    {
        const int touchId = e.source.getIndex();
        const float px = static_cast<float>(e.position.x);
        const float py = static_cast<float>(e.position.y);
        const int64_t nowMs = juce::Time::currentTimeMillis();

        // Find the zone that contains this touch point
        int owningZone = -1;
        for (const auto& [zid, zone] : zones)
        {
            if (zone.active && zone.bounds.contains(px, py))
            {
                owningZone = zid;
                break;
            }
        }

        if (owningZone < 0)
            return;  // Touch outside all zones — ignore

        // Evict stale recently-ended records before using them for approach speed
        for (auto it = recentlyEndedTouches.begin(); it != recentlyEndedTouches.end(); )
        {
            if ((nowMs - it->second.timestampMs) > MDTVApproachWindowMs)
                it = recentlyEndedTouches.erase(it);
            else
                ++it;
        }

        // MDTV Signal 1: approach speed from any recently-ended nearby touch
        const float approachSpd = computeApproachSpeed(px, py, nowMs);

        // MDTV Signal 3: hardware force (available on 3D Touch iPhones)
        const float hwForce = queryHardwareForce(e);

        // Initial MDTV estimate using only Signal 1 + Signal 3.
        // Signal 2 (contact gradient) is added in handleMouseDrag on first drag.
        // If hardware force IS available, we already have a strong estimate.
        // contactGradient defaults to 0.5 (mid-range neutral) for the initial event.
        const float initialContactGradient = 0.5f;
        const float initialMDTV = computeMDTVVelocity(approachSpd, initialContactGradient, hwForce);

        const auto& zone = zones[owningZone];
        float nx = (px - zone.bounds.getX()) / zone.bounds.getWidth();
        float ny = (py - zone.bounds.getY()) / zone.bounds.getHeight();
        nx = juce::jlimit(0.0f, 1.0f, nx);
        ny = juce::jlimit(0.0f, 1.0f, ny);

        TouchEvent evt {
            touchId, px, py, nx, ny,
            initialMDTV,                    // MDTV-derived force replaces DefaultForce
            static_cast<float>(e.pressure),
            TouchPhase::Began,
            nowMs,
            0.0f, 0.0f
        };

        ActiveTouch at;
        at.owningZoneId      = owningZone;
        at.lastX             = px;
        at.lastY             = py;
        at.lastTimestamp     = nowMs;
        at.smoothedVx        = 0.0f;
        at.smoothedVy        = 0.0f;
        // MDTV fields
        at.beginX                = px;
        at.beginY                = py;
        at.beganTimestampMs      = nowMs;
        at.firstDragDistancePt   = 0.0f;
        at.firstDragTimestampMs  = 0;       // 0 = first drag not yet seen
        at.mdtvResolved          = false;
        at.mdtvVelocity          = initialMDTV;
        at.approachSpeed         = approachSpd;

        activeTouches[touchId] = at;

        zone.callback(evt);
    }

    void handleMouseDrag(const juce::MouseEvent& e)
    {
        const int touchId = e.source.getIndex();
        auto it = activeTouches.find(touchId);
        if (it == activeTouches.end())
            return;

        auto& at = it->second;
        auto zoneIt = zones.find(at.owningZoneId);
        if (zoneIt == zones.end())
            return;

        const auto& zone = zoneIt->second;
        const float px = static_cast<float>(e.position.x);
        const float py = static_cast<float>(e.position.y);
        const int64_t now = juce::Time::currentTimeMillis();
        const float dt = static_cast<float>(now - at.lastTimestamp);

        // Compute touch velocity (points/second) with EMA smoothing
        float vx = 0.0f, vy = 0.0f;
        if (dt > 0.0f)
        {
            float rawVx = (px - at.lastX) / (dt * 0.001f);
            float rawVy = (py - at.lastY) / (dt * 0.001f);
            vx = at.smoothedVx + VelocitySmoothingFactor * (rawVx - at.smoothedVx);
            vy = at.smoothedVy + VelocitySmoothingFactor * (rawVy - at.smoothedVy);
            at.smoothedVx = vx;
            at.smoothedVy = vy;
        }

        // MDTV Signal 2: contact duration gradient — first drag only
        if (!at.mdtvResolved && at.firstDragTimestampMs == 0)
        {
            at.firstDragTimestampMs = now;

            float ddx = px - at.beginX;
            float ddy = py - at.beginY;
            at.firstDragDistancePt = std::sqrt(ddx * ddx + ddy * ddy);

            // contactGradient: 1.0 = decisive hard strike (tiny movement on first drag)
            //                  0.0 = soft/exploring touch (large early movement)
            float contactGradient = 1.0f - juce::jlimit(0.0f, 1.0f,
                                        at.firstDragDistancePt / MDTVContactGradientPt);

            // Scale by reaction time: short dt (quick first drag) → hard strike
            // Long dt (finger settled before moving) → soft touch
            // Normalise: 0–150 ms maps to 1.0–0.0 scaling factor
            const float kMaxReactionMs = 150.0f;
            float elapsed = static_cast<float>(now - at.beganTimestampMs);
            float timeFactor = 1.0f - juce::jlimit(0.0f, 1.0f, elapsed / kMaxReactionMs);
            contactGradient *= (0.5f + 0.5f * timeFactor);  // blend: pure distance at slow end

            // MDTV Signal 3: hardware force
            const float hwForce = queryHardwareForce(e);

            at.mdtvVelocity = computeMDTVVelocity(at.approachSpeed, contactGradient, hwForce);
            at.mdtvResolved = true;
        }

        at.lastX = px;
        at.lastY = py;
        at.lastTimestamp = now;

        float nx = (px - zone.bounds.getX()) / zone.bounds.getWidth();
        float ny = (py - zone.bounds.getY()) / zone.bounds.getHeight();
        nx = juce::jlimit(0.0f, 1.0f, nx);
        ny = juce::jlimit(0.0f, 1.0f, ny);

        // Use the resolved MDTV velocity for Moved events; this lets zones that
        // re-read force on drag (e.g., aftertouch emulation) see the corrected value.
        float forceFinal = at.mdtvResolved ? at.mdtvVelocity : extractForce(e);

        TouchEvent evt {
            touchId, px, py, nx, ny,
            forceFinal, static_cast<float>(e.pressure),
            TouchPhase::Moved,
            now, vx, vy
        };

        zone.callback(evt);
    }

    void handleMouseUp(const juce::MouseEvent& e)
    {
        dispatchEndEvent(e, TouchPhase::Ended);
    }

    // Called if the system cancels a touch (e.g., incoming call)
    void handleTouchCancelled(const juce::MouseEvent& e)
    {
        dispatchEndEvent(e, TouchPhase::Cancelled);
    }

    //-- Query -------------------------------------------------------------------

    int getActiveTouchCount() const { return static_cast<int>(activeTouches.size()); }

    bool isTouchActiveInZone(int zoneId) const
    {
        for (const auto& [_, at] : activeTouches)
            if (at.owningZoneId == zoneId)
                return true;
        return false;
    }

    // Cancel all active touches (used by PANIC, orientation change, etc.)
    void cancelAllTouches()
    {
        for (auto& [touchId, at] : activeTouches)
        {
            auto zoneIt = zones.find(at.owningZoneId);
            if (zoneIt != zones.end())
            {
                TouchEvent evt {
                    touchId, at.lastX, at.lastY, 0.0f, 0.0f,
                    0.0f, 0.0f,
                    TouchPhase::Cancelled,
                    juce::Time::currentTimeMillis(),
                    0.0f, 0.0f
                };
                zoneIt->second.callback(evt);
            }
        }
        activeTouches.clear();
    }

private:
    struct ActiveTouch {
        int owningZoneId;
        float lastX, lastY;
        int64_t lastTimestamp;
        float smoothedVx, smoothedVy;

        // MDTV — Signal tracking
        float beginX, beginY;             // Touch-down position
        int64_t beganTimestampMs;         // When mouseDown fired
        float firstDragDistancePt;        // Distance of first drag from begin position
        int64_t firstDragTimestampMs;     // When first drag arrived (0 = not yet seen)
        bool mdtvResolved;                // Whether MDTV velocity has been computed
        float mdtvVelocity;               // Computed strike velocity (0.05–1.0)
        float approachSpeed;              // Signal 1: spatial speed estimate (0.0–1.0)
    };

    std::unordered_map<int, TouchZone> zones;
    std::unordered_map<int, ActiveTouch> activeTouches;

    // Ring buffer of recently-ended touch positions for Signal 1 approach speed.
    // Keyed by touchId; entries older than MDTVApproachWindowMs are ignored.
    struct EndedTouchRecord {
        float x, y;
        int64_t timestampMs;
    };
    std::unordered_map<int, EndedTouchRecord> recentlyEndedTouches;

    int nextZoneId = 0;

    // --------------------------------------------------------------------------
    // MDTV Signal 3: UITouch.force via side-channel
    // Returns the raw hardware force or -1.0 if unavailable.
    float queryHardwareForce(const juce::MouseEvent& e) const
    {
#if JUCE_IOS
        if (xoceanus::touch_force::isForceAvailable())
            return xoceanus::touch_force::getForceForTouch(e.source.getIndex());
#else
        juce::ignoreUnused(e);
#endif
        return -1.0f;
    }

    // --------------------------------------------------------------------------
    // MDTV Signal 1: Approach speed
    // Looks for a recently-ended touch within MDTVApproachRadiusPt of (px, py)
    // that ended within MDTVApproachWindowMs of now. If found, estimates
    // approach speed from the ending velocity of that touch (0.0–1.0 normalized).
    // Returns 0.0 if no qualifying approach is found.
    float computeApproachSpeed(float px, float py, int64_t nowMs) const
    {
        float bestSpeed = 0.0f;
        for (const auto& [id, rec] : recentlyEndedTouches)
        {
            int64_t age = nowMs - rec.timestampMs;
            if (age < 0 || age > MDTVApproachWindowMs)
                continue;

            float dx = px - rec.x;
            float dy = py - rec.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > MDTVApproachRadiusPt)
                continue;

            // Recency factor: touches that ended right before this one score higher
            float recency = 1.0f - (static_cast<float>(age) / static_cast<float>(MDTVApproachWindowMs));
            // Proximity factor: closer approach point scores higher
            float proximity = 1.0f - juce::jlimit(0.0f, 1.0f, dist / MDTVApproachRadiusPt);
            float speed = recency * proximity;
            if (speed > bestSpeed)
                bestSpeed = speed;
        }
        return bestSpeed;
    }

    // --------------------------------------------------------------------------
    // MDTV combiner
    // Blends Signal 1 (approachSpeed), Signal 2 (contactGradient), Signal 3
    // (touchForce). Force takes priority when available; otherwise the iPad path
    // runs with contactGradient as primary.
    static float computeMDTVVelocity(float approachSpeed, float contactGradient, float touchForce)
    {
        float v;
        if (touchForce >= 0.0f)
        {
            // Device has force sensing — weight it heavily
            v = touchForce * 0.6f + contactGradient * 0.3f + approachSpeed * 0.1f;
        }
        else
        {
            // No force sensing (all iPads) — contact gradient is primary
            v = contactGradient * 0.7f + approachSpeed * 0.3f;
        }
        return juce::jlimit(0.05f, 1.0f, v);
    }

    // --------------------------------------------------------------------------
    // extractForce — now MDTV-aware.
    // For a Began event, returns an early MDTV estimate using Signal 1 + Signal 3
    // only (Signal 2 is not yet available). Once the ActiveTouch has been resolved
    // via the first Drag event, callers should prefer ActiveTouch::mdtvVelocity.
    float extractForce(const juce::MouseEvent& e) const
    {
        // JUCE exposes pressure from iOS UITouch.force
        float pressure = static_cast<float>(e.pressure);
        if (pressure > 0.0f)
            return juce::jlimit(0.0f, 1.0f, pressure);

        // Fallback: no force sensing — use a fixed default.
        // The caller replaces this with the MDTV value once it is resolved.
        return DefaultForce;
    }

    void dispatchEndEvent(const juce::MouseEvent& e, TouchPhase phase)
    {
        const int touchId = e.source.getIndex();
        auto it = activeTouches.find(touchId);
        if (it == activeTouches.end())
            return;

        auto& at = it->second;
        const int64_t nowMs = juce::Time::currentTimeMillis();

        auto zoneIt = zones.find(at.owningZoneId);
        if (zoneIt != zones.end())
        {
            const auto& zone = zoneIt->second;
            const float px = static_cast<float>(e.position.x);
            const float py = static_cast<float>(e.position.y);
            float nx = (px - zone.bounds.getX()) / zone.bounds.getWidth();
            float ny = (py - zone.bounds.getY()) / zone.bounds.getHeight();
            nx = juce::jlimit(0.0f, 1.0f, nx);
            ny = juce::jlimit(0.0f, 1.0f, ny);

            TouchEvent evt {
                touchId, px, py, nx, ny,
                0.0f, 0.0f,
                phase,
                nowMs,
                at.smoothedVx, at.smoothedVy
            };

            zone.callback(evt);

            // Record tombstone for MDTV Signal 1: the next touch-down near this
            // position within MDTVApproachWindowMs can measure approach speed.
            recentlyEndedTouches[touchId] = { px, py, nowMs };
        }

        activeTouches.erase(it);
    }
};

} // namespace xoceanus
