import Foundation
import CoreMotion

/// Maps device motion (tilt, shake) to synth parameter modulation.
/// Tilt = filter sweep. Shake = stutter/glitch.
///
/// Motion values are applied as **deltas** on top of the specimen's cached
/// parameter values, not as absolute writes.  This prevents the motion layer
/// from overriding the specimen cache: the cache is the source of truth for
/// the base value; motion adds a transient offset that evaporates when the
/// device returns to rest (tilt near zero, shake below threshold).
final class MotionController: ObservableObject {
    @Published var isEnabled = false
    @Published var tiltX: Float = 0   // -1 to 1 (left-right tilt)
    @Published var tiltY: Float = 0   // -1 to 1 (forward-back tilt)
    @Published var shakeIntensity: Float = 0  // 0 to 1

    private let motionManager = CMMotionManager()
    private let updateInterval: TimeInterval = 1.0 / 30.0 // 30Hz

    /// Maximum cutoff offset (Hz) when device is tilted fully left/right.
    private let cutoffDeltaRange: Float = 5000.0

    /// Maximum resonance offset when device is tilted fully forward/back.
    private let resoDeltaRange: Float = 0.4

    func start() {
        guard motionManager.isDeviceMotionAvailable else { return }
        isEnabled = true

        motionManager.deviceMotionUpdateInterval = updateInterval
        motionManager.startDeviceMotionUpdates(to: .main) { [weak self] motion, error in
            guard let self, let motion else { return }

            // Tilt: attitude roll/pitch mapped to -1...1 (±45° = full range)
            self.tiltX = Float(max(-1, min(1, motion.attitude.roll / (.pi / 4))))
            self.tiltY = Float(max(-1, min(1, motion.attitude.pitch / (.pi / 4))))

            // Shake detection: magnitude of user acceleration
            // threshold 0.5g, saturates at 2.5g
            let accel = motion.userAcceleration
            let magnitude = sqrt(accel.x * accel.x + accel.y * accel.y + accel.z * accel.z)
            self.shakeIntensity = Float(max(0, min(1, (magnitude - 0.5) / 2.0)))

            self.applyDeltasToEngine()
        }
    }

    func stop() {
        motionManager.stopDeviceMotionUpdates()
        isEnabled = false
        shakeIntensity = 0
        tiltX = 0
        tiltY = 0
        // Zero out any lingering modulation deltas so the engine returns to
        // the specimen cache's base values when motion input stops.
        applyDeltasToEngine()
    }

    /// Apply motion as **additive modulation deltas** via immediate parameter writes.
    ///
    /// ObrixBridge has no native delta API, so we read the current parameter value,
    /// add the motion offset, and write it back via `setParameterImmediate`.  The
    /// specimen cache layer re-establishes the base on the next `applyCachedParams`
    /// call, so these writes are inherently transient — when motion stops (tilt/shake
    /// near zero) the engine returns to the cached base on the next load cycle.
    private func applyDeltasToEngine() {
        guard let bridge = ObrixBridge.shared() else { return }

        // Tilt X → filter cutoff delta (±cutoffDeltaRange Hz around the base value)
        let cutoffDelta = tiltX * cutoffDeltaRange
        let currentCutoff = bridge.getParameter("obrix_proc1Cutoff")
        bridge.setParameterImmediate("obrix_proc1Cutoff", value: currentCutoff + cutoffDelta)

        // Tilt Y → resonance delta (±resoDeltaRange around the base value)
        let resoDelta = tiltY * resoDeltaRange
        let currentReso = bridge.getParameter("obrix_proc1Reso")
        bridge.setParameterImmediate("obrix_proc1Reso", value: currentReso + resoDelta)

        // Shake → stutter burst via LFO rate/depth deltas (only when shaking)
        if shakeIntensity > 0.1 {
            let currentRate  = bridge.getParameter("obrix_mod2Rate")
            let currentDepth = bridge.getParameter("obrix_mod2Depth")
            bridge.setParameterImmediate("obrix_mod2Rate",  value: currentRate  + shakeIntensity * 20.0)
            bridge.setParameterImmediate("obrix_mod2Depth", value: currentDepth + shakeIntensity * 0.8)
        }
        // When shake subsides (shakeIntensity ≤ 0.1) the next applyCachedParams
        // call from the audio engine will restore the base LFO values — no explicit
        // zero write needed.
    }
}
