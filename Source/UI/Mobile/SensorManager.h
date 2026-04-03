// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_core/juce_core.h>
#include <atomic>
#include <functional>

namespace xoceanus
{

//==============================================================================
// SensorManager — Accelerometer, gyroscope, and motion integration.
//
// Provides smoothed, dead-zone-filtered sensor data for musical modulation.
// All output values are normalized to -1.0 to 1.0.
//
// Design:
//   - Sensor updates arrive at up to 60Hz from the platform layer
//   - Each axis is smoothed with an exponential moving average
//   - Dead zone prevents jitter when the device is at rest
//   - Neutral position is calibratable (not hardcoded to flat)
//   - Output values are atomic — safe to read from audio thread
//
// Platform integration:
//   - On iOS: wraps CMMotionManager via Objective-C++ bridge (SensorManager_iOS.mm)
//   - On macOS: start()/stop() are no-ops; motion data is never delivered
//   - The bridge layer calls feedAccelerometer() and feedGyroscope()
//     from the CMMotionManager callback thread
//
// Lifecycle:
//   1. Construct SensorManager
//   2. Call setConfig() with the desired Config
//   3. Call start() — begins CMMotionManager updates on iOS
//   4. Call stop() before destroying the instance (or at plugin shutdown)
//
class SensorManager
{
public:
    //-- Configuration -----------------------------------------------------------

    struct Config
    {
        bool motionEnabled = false;         // User opt-in (Settings > Motion Control)
        float deadZoneDegrees = 3.0f;       // Dead zone around neutral (+/-)
        float smoothingMs = 50.0f;          // EMA time constant
        int updateRateHz = 60;              // Sensor polling rate cap
        float sensitivityMultiplier = 1.0f; // 0.25x to 4.0x user-configurable
    };

    enum class MotionMode
    {
        Off,           // No motion modulation
        Accelerometer, // Tilt → filter/pitch
        Gyroscope      // Rotation → Orbit Path XY
    };

    void setConfig(const Config& cfg)
    {
        config = cfg;
        // Recompute smoothing coefficient: alpha = 1 - exp(-dt / tau)
        // where dt = 1/updateRate, tau = smoothingMs/1000
        if (config.smoothingMs > 0.0f && config.updateRateHz > 0)
        {
            float dt = 1.0f / static_cast<float>(config.updateRateHz);
            float tau = config.smoothingMs * 0.001f;
            smoothAlpha = 1.0f - std::exp(-dt / tau);
        }
    }

    void setMotionMode(MotionMode mode) { motionMode = mode; }
    MotionMode getMotionMode() const { return motionMode; }

    //-- Lifecycle ---------------------------------------------------------------

    // Start motion sensor updates.
    // On iOS: delegates to sensor_platform::startMotionUpdates(), which allocates
    //   a CMMotionManager singleton and begins device-motion callbacks.  The
    //   callbacks will call feedAccelerometer() / feedGyroscope() on this instance.
    // On other platforms: no-op (no motion hardware available).
    //
    // May be called multiple times; each call replaces the previous session.
    // config.motionEnabled must be true for this to have any effect.
    void start()
    {
#if JUCE_IOS
        sensor_platform::startMotionUpdates(this, config);
#endif
    }

    // Stop motion sensor updates and release platform resources.
    // On iOS: delegates to sensor_platform::stopMotionUpdates(), which stops
    //   CMMotionManager and cancels any pending callbacks.
    // On other platforms: no-op.
    //
    // Idempotent. Must be called before this SensorManager instance is destroyed
    // whenever start() was previously called.
    void stop()
    {
#if JUCE_IOS
        sensor_platform::stopMotionUpdates();
#endif
    }

    //-- Calibration -------------------------------------------------------------

    // Capture the current raw sensor values as the new neutral position.
    // Call when the user taps "Set Neutral" in settings.
    void calibrateNeutral()
    {
        neutralX = rawAccelX;
        neutralY = rawAccelY;
        neutralZ = rawAccelZ;
    }

    //-- Sensor input (called from platform bridge at up to 60Hz) ----------------

    // Feed raw accelerometer data (in g-force units, typically -1 to +1 per axis).
    void feedAccelerometer(float ax, float ay, float az)
    {
        rawAccelX = ax;
        rawAccelY = ay;
        rawAccelZ = az;

        if (motionMode != MotionMode::Accelerometer || !config.motionEnabled)
            return;

        // Compute tilt in degrees relative to neutral
        float tiltXDeg = (ax - neutralX) * 90.0f; // Approximate: 1g ≈ 90°
        float tiltYDeg = (ay - neutralY) * 90.0f;

        // Apply dead zone
        tiltXDeg = applyDeadZone(tiltXDeg, config.deadZoneDegrees);
        tiltYDeg = applyDeadZone(tiltYDeg, config.deadZoneDegrees);

        // Normalize to -1..1 (±45° = full range)
        float normX = juce::jlimit(-1.0f, 1.0f, tiltXDeg / 45.0f * config.sensitivityMultiplier);
        float normY = juce::jlimit(-1.0f, 1.0f, tiltYDeg / 45.0f * config.sensitivityMultiplier);

        // Smooth
        smoothedTiltX += smoothAlpha * (normX - smoothedTiltX);
        smoothedTiltY += smoothAlpha * (normY - smoothedTiltY);

        // Publish (atomic for audio thread reads)
        tiltX.store(smoothedTiltX, std::memory_order_relaxed);
        tiltY.store(smoothedTiltY, std::memory_order_relaxed);
    }

    // Feed raw gyroscope data (in radians/second).
    void feedGyroscope(float rx, float ry, float rz)
    {
        if (motionMode != MotionMode::Gyroscope || !config.motionEnabled)
            return;

        // Integrate rotation for Orbit Path position
        float dt = 1.0f / static_cast<float>(config.updateRateHz);
        gyroAccumX += rx * dt * config.sensitivityMultiplier;
        gyroAccumY += ry * dt * config.sensitivityMultiplier;

        // Clamp accumulated rotation to ±1 (represents full Orbit Path range)
        gyroAccumX = juce::jlimit(-1.0f, 1.0f, gyroAccumX);
        gyroAccumY = juce::jlimit(-1.0f, 1.0f, gyroAccumY);

        // Apply smoothing
        smoothedGyroX += smoothAlpha * (gyroAccumX - smoothedGyroX);
        smoothedGyroY += smoothAlpha * (gyroAccumY - smoothedGyroY);

        // Auto-center decay (gentle pull toward zero when device is still)
        if (std::abs(rx) < 0.05f && std::abs(ry) < 0.05f)
        {
            gyroAccumX *= 0.995f;
            gyroAccumY *= 0.995f;
        }

        gyroX.store(smoothedGyroX, std::memory_order_relaxed);
        gyroY.store(smoothedGyroY, std::memory_order_relaxed);
    }

    //-- Output (safe to read from audio thread) ---------------------------------

    // Accelerometer: tilt modulation values, -1.0 to 1.0
    float getTiltX() const { return tiltX.load(std::memory_order_relaxed); }
    float getTiltY() const { return tiltY.load(std::memory_order_relaxed); }

    // Gyroscope: Orbit Path position values, -1.0 to 1.0
    float getGyroX() const { return gyroX.load(std::memory_order_relaxed); }
    float getGyroY() const { return gyroY.load(std::memory_order_relaxed); }

    // Reset gyroscope accumulator (e.g., when user re-centers)
    void resetGyroPosition()
    {
        gyroAccumX = 0.0f;
        gyroAccumY = 0.0f;
        smoothedGyroX = 0.0f;
        smoothedGyroY = 0.0f;
        gyroX.store(0.0f, std::memory_order_relaxed);
        gyroY.store(0.0f, std::memory_order_relaxed);
    }

    bool isMotionEnabled() const { return config.motionEnabled; }

private:
    Config config;
    MotionMode motionMode = MotionMode::Off;
    float smoothAlpha = 0.3f;

    // Raw sensor state
    float rawAccelX = 0.0f, rawAccelY = 0.0f, rawAccelZ = 0.0f;

    // Calibration neutral position
    float neutralX = 0.0f, neutralY = 0.0f, neutralZ = 0.0f;

    // Smoothed intermediate values (message thread only)
    float smoothedTiltX = 0.0f, smoothedTiltY = 0.0f;
    float smoothedGyroX = 0.0f, smoothedGyroY = 0.0f;
    float gyroAccumX = 0.0f, gyroAccumY = 0.0f;

    // Atomic outputs (read by audio thread)
    std::atomic<float> tiltX{0.0f};
    std::atomic<float> tiltY{0.0f};
    std::atomic<float> gyroX{0.0f};
    std::atomic<float> gyroY{0.0f};

    static float applyDeadZone(float valueDeg, float deadZoneDeg)
    {
        if (std::abs(valueDeg) < deadZoneDeg)
            return 0.0f;
        // Scale remaining range so output starts at 0 just outside dead zone
        float sign = valueDeg > 0.0f ? 1.0f : -1.0f;
        return (std::abs(valueDeg) - deadZoneDeg) * sign;
    }
};

} // namespace xoceanus

// ---------------------------------------------------------------------------
// sensor_platform — iOS platform bridge forward declarations
//
// Defined in SensorManager_iOS.mm (Objective-C++).
//
// Placed AFTER the full SensorManager class definition so that
// SensorManager::Config is a complete type when these signatures are parsed.
//
// Non-iOS translation units that include this header see only the declarations
// (under the #if guard).  They will never be called because start()/stop()
// are also guarded by #if JUCE_IOS.  The linker only pulls in the .mm on iOS
// targets, so there are no link errors on macOS/desktop builds.
// ---------------------------------------------------------------------------
#if JUCE_IOS
namespace sensor_platform
{

// Start motion updates feeding into |sm| using the supplied Config.
// |sm| must outlive the motion update session.
void startMotionUpdates(xoceanus::SensorManager* sm, const xoceanus::SensorManager::Config& config);

// Stop all motion updates and release CMMotionManager resources.
void stopMotionUpdates();

// Returns true if the current device has a gyroscope.
// Safe to call before startMotionUpdates().
bool isGyroscopeAvailable();

// Returns true if motion updates are currently active.
bool isRunning();

} // namespace sensor_platform
#endif // JUCE_IOS
