import Foundation
import CoreMotion

/// Maps device motion (tilt, shake) to synth parameter modulation.
/// Tilt = filter sweep. Shake = stutter/glitch.
final class MotionController: ObservableObject {
    @Published var isEnabled = false
    @Published var tiltX: Float = 0   // -1 to 1 (left-right tilt)
    @Published var tiltY: Float = 0   // -1 to 1 (forward-back tilt)
    @Published var shakeIntensity: Float = 0  // 0 to 1

    private let motionManager = CMMotionManager()
    private let updateInterval: TimeInterval = 1.0 / 30.0 // 30Hz

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

            self.applyToEngine()
        }
    }

    func stop() {
        motionManager.stopDeviceMotionUpdates()
        isEnabled = false
        shakeIntensity = 0
        tiltX = 0
        tiltY = 0
    }

    private func applyToEngine() {
        guard let bridge = ObrixBridge.shared() else { return }

        // Tilt X → filter cutoff modulation (0–10000 Hz offset)
        let cutoffMod = 5000.0 + Float(tiltX) * 5000.0
        bridge.setParameterImmediate("obrix_proc1Cutoff", value: cutoffMod)

        // Tilt Y → resonance modulation (0–0.7 range)
        let resoMod = max(0, min(1, 0.3 + tiltY * 0.4))
        bridge.setParameterImmediate("obrix_proc1Reso", value: resoMod)

        // Shake → stutter via rapid LFO rate burst
        if shakeIntensity > 0.1 {
            bridge.setParameterImmediate("obrix_mod2Rate", value: 10 + shakeIntensity * 20)
            bridge.setParameterImmediate("obrix_mod2Depth", value: shakeIntensity * 0.8)
        }
    }
}
