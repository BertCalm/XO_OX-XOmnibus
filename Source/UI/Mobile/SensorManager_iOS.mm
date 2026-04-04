// SensorManager_iOS.mm
// XOceanus — iOS Objective-C++ bridge for CMMotionManager.
//
// Provides the sensor_platform namespace that SensorManager.h expects on iOS.
// Called by SensorManager on iOS only; all other platforms are no-ops via the
// #if JUCE_IOS guard.
//
// Design notes:
//   - CMMotionManager is allocated once per process and must not be duplicated.
//   - Motion callbacks fire on a dedicated NSOperationQueue (never the main queue)
//     so the C++ call into SensorManager is off the UI thread — this is safe
//     because SensorManager's feed* methods only write atomic<float> outputs and
//     non-atomic intermediate values that are only touched from this single
//     callback thread.
//   - Actual dt is derived from consecutive CMDeviceMotion timestamps rather than
//     assuming a fixed 1/updateRateHz interval.  CMMotionManager timestamps are
//     high-resolution NSTimeInterval values (seconds since device boot).
//   - feedGyroscope() inside SensorManager applies a fixed dt = 1/updateRateHz
//     when integrating rad/s into accumulated rotation.  To honour the *actual*
//     elapsed time, we pre-scale the raw rad/s values:
//
//         scaledRate = rawRate * (actualDt * nominalHz)
//
//     so that feedGyroscope()'s internal division by nominalHz cancels out and
//     the net effect is integration over actualDt.  This is the correct fix for
//     the QDD finding that a fixed dt assumption caused gyro drift errors under
//     jitter or background throttling.
//   - Gyroscope availability is checked at start time; on devices without a
//     gyroscope only accelerometer updates are requested.
//   - @autoreleasepool is placed around each callback body to prevent any
//     Objective-C temporary objects from accumulating on the worker thread.

#if JUCE_IOS

#import <CoreMotion/CoreMotion.h>
#include "SensorManager.h"

// ---------------------------------------------------------------------------
// Internal Objective-C helper — keeps CMMotionManager state off the C++ side.
// ---------------------------------------------------------------------------

@interface XOMotionManagerHelper : NSObject

@property (nonatomic, strong) CMMotionManager*      motionManager;
@property (nonatomic, strong) NSOperationQueue*     motionQueue;
@property (nonatomic, assign) xoceanus::SensorManager* sensorManager;  // non-owning
@property (nonatomic, assign) NSTimeInterval        lastTimestamp;     // previous callback ts
@property (nonatomic, assign) int                   nominalHz;         // mirrors Config::updateRateHz
@property (nonatomic, assign) BOOL                  gyroAvailable;

- (instancetype)initWithSensorManager:(xoceanus::SensorManager*)sm
                           nominalHz:(int)hz;

- (void)startUpdates;
- (void)stopUpdates;

@end

@implementation XOMotionManagerHelper

- (instancetype)initWithSensorManager:(xoceanus::SensorManager*)sm
                           nominalHz:(int)hz
{
    self = [super init];
    if (self) {
        _sensorManager  = sm;
        _nominalHz      = hz;
        _lastTimestamp  = -1.0;  // sentinel: "no previous sample yet"
        _motionManager  = [[CMMotionManager alloc] init];
        _motionQueue    = [[NSOperationQueue alloc] init];
        _motionQueue.name = @"com.xo-ox.xoceanus.motionQueue";
        _motionQueue.maxConcurrentOperationCount = 1;

        // Prefer device-motion (fused gyro + accel) when available.
        // Gyroscope is only available on devices that actually have the hardware.
        _gyroAvailable  = _motionManager.gyroAvailable;

        NSTimeInterval interval = 1.0 / static_cast<double>(hz);
        _motionManager.deviceMotionUpdateInterval  = interval;
        _motionManager.accelerometerUpdateInterval = interval;
    }
    return self;
}

- (void)startUpdates
{
    if (!_sensorManager)
        return;

    // Reset timestamp so the first callback doesn't produce a spurious large dt.
    _lastTimestamp = -1.0;

    if (_motionManager.deviceMotionAvailable) {
        // Device-motion fuses gyroscope + accelerometer through the reference frame.
        // We use CMAttitudeReferenceFrameXArbitraryZVertical so Z-up is meaningful
        // across device orientations without requiring magnetometer calibration.
        __weak XOMotionManagerHelper* weakSelf = self;

        [_motionManager startDeviceMotionUpdatesUsingReferenceFrame:
                            CMAttitudeReferenceFrameXArbitraryZVertical
                                                           toQueue:_motionQueue
                                                       withHandler:^(CMDeviceMotion* _Nullable motion,
                                                                     NSError* _Nullable  error)
        {
            @autoreleasepool {
                XOMotionManagerHelper* strongSelf = weakSelf;
                if (!strongSelf || !motion || error)
                    return;

                xoceanus::SensorManager* sm = strongSelf.sensorManager;
                if (!sm)
                    return;

                // ---- Compute actual dt from consecutive timestamps ----
                NSTimeInterval ts = motion.timestamp;
                double actualDt;

                if (strongSelf.lastTimestamp < 0.0) {
                    // First sample: assume nominal interval; no prior reference.
                    actualDt = 1.0 / static_cast<double>(strongSelf.nominalHz);
                } else {
                    actualDt = ts - strongSelf.lastTimestamp;
                    // Guard against pathological values (timer wrap, background wakeup gap).
                    // Clamp to [0.5 * nominal, 4 * nominal].
                    double nominalDt = 1.0 / static_cast<double>(strongSelf.nominalHz);
                    if (actualDt < nominalDt * 0.5)  actualDt = nominalDt * 0.5;
                    if (actualDt > nominalDt * 4.0)  actualDt = nominalDt * 4.0;
                }
                strongSelf.lastTimestamp = ts;

                // ---- Accelerometer (g-force, gravity-referenced) ----
                CMAcceleration ua = motion.userAcceleration;  // device accel without gravity
                CMAcceleration g  = motion.gravity;           // gravity component only

                // SensorManager expects total g-force per axis (gravity + user accel).
                // gravity vector gives the static tilt; userAcceleration gives movement.
                // For tilt control, gravity is the meaningful signal.  We pass gravity
                // for tilt axes (X, Y) and the full vector for Z.
                float ax = static_cast<float>(g.x  + ua.x);
                float ay = static_cast<float>(g.y  + ua.y);
                float az = static_cast<float>(g.z  + ua.z);

                sm->feedAccelerometer(ax, ay, az);

                // ---- Gyroscope (rotation rate in rad/s) ----
                // feedGyroscope() integrates:  gyroAccum += rate * (1/nominalHz)
                // To use actualDt instead, pre-scale rates so the net integration is:
                //   gyroAccum += (rate * actualDt * nominalHz) * (1/nominalHz)
                //              = rate * actualDt                     ✓
                if (strongSelf.gyroAvailable) {
                    CMRotationRate rr = motion.rotationRate;
                    double scale = actualDt * static_cast<double>(strongSelf.nominalHz);

                    float rx = static_cast<float>(rr.x * scale);
                    float ry = static_cast<float>(rr.y * scale);
                    float rz = static_cast<float>(rr.z * scale);

                    sm->feedGyroscope(rx, ry, rz);
                }
            }
        }];
    }
    else if (_motionManager.accelerometerAvailable) {
        // Fallback: raw accelerometer only (no gyroscope path).
        __weak XOMotionManagerHelper* weakSelf = self;

        [_motionManager startAccelerometerUpdatesToQueue:_motionQueue
                                             withHandler:^(CMAccelerometerData* _Nullable data,
                                                           NSError* _Nullable             error)
        {
            @autoreleasepool {
                XOMotionManagerHelper* strongSelf = weakSelf;
                if (!strongSelf || !data || error)
                    return;

                xoceanus::SensorManager* sm = strongSelf.sensorManager;
                if (!sm)
                    return;

                CMAcceleration a = data.acceleration;
                sm->feedAccelerometer(static_cast<float>(a.x),
                                      static_cast<float>(a.y),
                                      static_cast<float>(a.z));
                // No gyroscope data available on this device; gyroX/gyroY remain 0.
            }
        }];
    }
}

- (void)stopUpdates
{
    if (_motionManager.deviceMotionActive) {
        [_motionManager stopDeviceMotionUpdates];
    } else if (_motionManager.accelerometerActive) {
        [_motionManager stopAccelerometerUpdates];
    }
    // Drain any pending ops and reset timestamp sentinel.
    [_motionQueue cancelAllOperations];
    _lastTimestamp = -1.0;
}

@end

// ---------------------------------------------------------------------------
// sensor_platform C++ interface
// ---------------------------------------------------------------------------
//
// SensorManager.h declares that the platform bridge is called from its own
// start/stop methods.  We expose a plain C++ namespace so SensorManager.h
// can call into this file without knowing about Objective-C.
//
// Lifetime contract:
//   - sensor_platform::startMotionUpdates() may be called multiple times;
//     each call replaces any previous session.
//   - sensor_platform::stopMotionUpdates() is idempotent.
//   - The SensorManager pointer must outlive the motion update session.
//     Calling stopMotionUpdates() before the SensorManager is destroyed is
//     the caller's responsibility (typically in SensorManager's destructor or
//     in the iOS plugin shutdown path).

namespace sensor_platform {

// One global helper instance per process (CMMotionManager must be a singleton).
static XOMotionManagerHelper* s_helper = nil;

// Start motion updates feeding into |sm|.
// |sm|    — the SensorManager instance to receive feed* calls.
// |config| — the Config struct; updateRateHz and motionEnabled are read here.
void startMotionUpdates(xoceanus::SensorManager* sm,
                        const xoceanus::SensorManager::Config& config)
{
    if (!config.motionEnabled || !sm)
        return;

    // If there is an active session with a different config, stop it first.
    if (s_helper)
        [s_helper stopUpdates];

    s_helper = [[XOMotionManagerHelper alloc] initWithSensorManager:sm
                                                          nominalHz:config.updateRateHz];
    [s_helper startUpdates];
}

// Stop all motion updates and release CMMotionManager resources.
void stopMotionUpdates()
{
    if (s_helper) {
        [s_helper stopUpdates];
        s_helper = nil;
    }
}

// Returns true if the current device has a gyroscope.
// Safe to call before startMotionUpdates().
bool isGyroscopeAvailable()
{
    // Lightweight: CMMotionManager allocation is cheap for a capability query.
    // In practice the caller usually checks this before constructing s_helper.
    CMMotionManager* probe = [[CMMotionManager alloc] init];
    return probe.gyroAvailable == YES;
}

// Returns true if motion updates are currently active.
bool isRunning()
{
    if (!s_helper)
        return false;
    return (s_helper.motionManager.deviceMotionActive ||
            s_helper.motionManager.accelerometerActive);
}

} // namespace sensor_platform

#endif // JUCE_IOS
