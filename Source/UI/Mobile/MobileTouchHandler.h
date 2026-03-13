#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <unordered_map>
#include <functional>

namespace xomnibus {

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
    static constexpr float DefaultForce = 0.5f;       // When force not available
    static constexpr float VelocitySmoothingFactor = 0.3f;  // EMA for touch velocity

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

        const auto& zone = zones[owningZone];
        float nx = (px - zone.bounds.getX()) / zone.bounds.getWidth();
        float ny = (py - zone.bounds.getY()) / zone.bounds.getHeight();
        nx = juce::jlimit(0.0f, 1.0f, nx);
        ny = juce::jlimit(0.0f, 1.0f, ny);

        float force = extractForce(e);

        TouchEvent evt {
            touchId, px, py, nx, ny,
            force, static_cast<float>(e.pressure),
            TouchPhase::Began,
            juce::Time::currentTimeMillis(),
            0.0f, 0.0f
        };

        ActiveTouch at { owningZone, px, py, juce::Time::currentTimeMillis(), 0.0f, 0.0f };
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

        at.lastX = px;
        at.lastY = py;
        at.lastTimestamp = now;

        float nx = (px - zone.bounds.getX()) / zone.bounds.getWidth();
        float ny = (py - zone.bounds.getY()) / zone.bounds.getHeight();
        nx = juce::jlimit(0.0f, 1.0f, nx);
        ny = juce::jlimit(0.0f, 1.0f, ny);

        TouchEvent evt {
            touchId, px, py, nx, ny,
            extractForce(e), static_cast<float>(e.pressure),
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
    };

    std::unordered_map<int, TouchZone> zones;
    std::unordered_map<int, ActiveTouch> activeTouches;
    int nextZoneId = 0;

    float extractForce(const juce::MouseEvent& e) const
    {
        // JUCE exposes pressure from iOS UITouch.force
        float pressure = static_cast<float>(e.pressure);
        if (pressure > 0.0f)
            return juce::jlimit(0.0f, 1.0f, pressure);

        // Fallback: no force sensing — use a fixed default
        return DefaultForce;
    }

    void dispatchEndEvent(const juce::MouseEvent& e, TouchPhase phase)
    {
        const int touchId = e.source.getIndex();
        auto it = activeTouches.find(touchId);
        if (it == activeTouches.end())
            return;

        auto& at = it->second;
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
                juce::Time::currentTimeMillis(),
                at.smoothedVx, at.smoothedVy
            };

            zone.callback(evt);
        }

        activeTouches.erase(it);
    }
};

} // namespace xomnibus
