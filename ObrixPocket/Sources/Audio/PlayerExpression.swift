import Foundation
import CoreMotion

/// Types of player expression gestures recognized in the Dive
enum ExpressionGesture {
    case tap            // Quick touch — accent/trigger
    case hold           // Sustained touch — sustain notes
    case swipeUp        // Upward swipe — increase intensity
    case swipeDown      // Downward swipe — decrease intensity/pull back
    case tilt           // Device tilt — filter sweep (gyroscope)
    case shake          // Quick shake — randomize/glitch
}

/// Processed expression state from player input, ready for DiveComposer consumption.
/// Updated every frame by the DiveScene, read by DiveComposer on each tick.
struct ExpressionState {
    /// Filter cutoff modifier (0 = fully closed, 1 = fully open). Driven by tilt.
    var filterSweep: Float = 0.5

    /// Intensity level (0 = pulled back, 1 = maximum energy). Driven by swipe up/down.
    var intensity: Float = 0.5

    /// Whether the player is actively holding (sustain pedal effect)
    var isHolding: Bool = false

    /// Accent strength from most recent tap (decays over time, 0-1)
    var accentStrength: Float = 0.0

    /// Call-and-response: which zone index was tapped (nil = none). DiveComposer
    /// uses this to trigger a phrase from the tapped specimen's voice.
    var tappedZoneIndex: Int? = nil

    /// Conductance: overall modifier on arrangement energy (0.5 = neutral, 0 = suppress, 1 = boost)
    var conductance: Float = 0.5

    /// Shake detected this frame
    var shakeDetected: Bool = false
}

/// Processes raw touch and motion input into musical expression parameters.
///
/// The handler smooths raw input, applies musical curves (perceptual, not linear),
/// and produces an ExpressionState that DiveComposer reads every tick.
///
/// Design principle (Phantom Circuit): "Touch-to-sound latency must be below 10ms perceived.
/// Player input should shape the music meaningfully — you're a conductor, not a pianist."
final class PlayerExpressionHandler {

    // MARK: - Output

    /// Current processed expression state — read by DiveComposer
    private(set) var state = ExpressionState()

    // MARK: - Internal State

    /// Motion manager for gyroscope/accelerometer
    private let motionManager = CMMotionManager()

    /// Whether motion tracking is active
    private var motionActive = false

    /// Smoothed tilt values (exponential moving average)
    private var smoothedPitch: Float = 0
    private var smoothedRoll: Float = 0

    /// Intensity momentum — swipes add to this, it decays toward 0.5
    private var intensityMomentum: Float = 0

    /// Accent decay timer
    private var accentDecayTimer: Float = 0

    /// Shake detection: recent acceleration magnitudes
    private var recentAccelerations: [Float] = []
    private let shakeThreshold: Float = 2.5

    /// Smoothing coefficient for tilt (0 = no smoothing, 1 = frozen)
    private let tiltSmoothing: Float = 0.85

    /// How fast intensity decays back to neutral (per second)
    private let intensityDecay: Float = 0.3

    /// How fast accents decay (per second)
    private let accentDecay: Float = 3.0

    // MARK: - Lifecycle

    /// Start tracking motion input. Call when dive begins.
    func startTracking() {
        state = ExpressionState()
        smoothedPitch = 0
        smoothedRoll = 0
        intensityMomentum = 0
        recentAccelerations = []

        // Start gyroscope at 60Hz (matches display refresh)
        if motionManager.isDeviceMotionAvailable {
            motionManager.deviceMotionUpdateInterval = 1.0 / 60.0
            motionManager.startDeviceMotionUpdates()
            motionActive = true
        }
    }

    /// Stop tracking. Call when dive ends.
    func stopTracking() {
        if motionActive {
            motionManager.stopDeviceMotionUpdates()
            motionActive = false
        }
    }

    // MARK: - Input Events (called by DiveScene)

    /// Player tapped a zone on the screen
    func registerTap(zoneIndex: Int) {
        state.accentStrength = 1.0
        accentDecayTimer = 0
        state.tappedZoneIndex = zoneIndex

        // Tap also bumps intensity slightly
        intensityMomentum = min(0.5, intensityMomentum + 0.1)
    }

    /// Player started holding (touch down without immediate release)
    func registerHoldStart() {
        state.isHolding = true
    }

    /// Player released hold
    func registerHoldEnd() {
        state.isHolding = false
    }

    /// Player swiped upward — boost intensity
    func registerSwipeUp(velocity: Float) {
        let boost = min(0.5, velocity * 0.003)  // Normalize touch velocity
        intensityMomentum = min(0.5, intensityMomentum + boost)
    }

    /// Player swiped downward — pull back intensity
    func registerSwipeDown(velocity: Float) {
        let reduction = min(0.5, velocity * 0.003)
        intensityMomentum = max(-0.5, intensityMomentum - reduction)
    }

    // MARK: - Frame Update

    /// Update expression state. Call every frame (60Hz) from DiveScene.
    func update(deltaTime: Float) {
        updateTilt()
        updateIntensity(deltaTime: deltaTime)
        updateAccent(deltaTime: deltaTime)
        updateShakeDetection()
        updateConductance()

        // Clear single-frame events
        state.tappedZoneIndex = nil
        state.shakeDetected = false
    }

    // MARK: - Internal Processing

    private func updateTilt() {
        guard let motion = motionManager.deviceMotion else { return }

        // Pitch: forward/back tilt → filter sweep (0-1)
        // Neutral = device held at ~45° angle (comfortable playing position)
        let rawPitch = Float(motion.attitude.pitch)  // radians, 0 = flat, π/2 = vertical
        let normalizedPitch = (rawPitch - 0.4) / 0.8  // Map ~25°-70° to 0-1

        // Exponential smoothing to prevent jitter
        smoothedPitch = smoothedPitch * tiltSmoothing + normalizedPitch * (1 - tiltSmoothing)
        state.filterSweep = max(0, min(1, smoothedPitch))

        // Roll: left/right tilt → stereo pan (not used yet, but tracked)
        let rawRoll = Float(motion.attitude.roll)
        smoothedRoll = smoothedRoll * tiltSmoothing + rawRoll * (1 - tiltSmoothing)
    }

    private func updateIntensity(deltaTime: Float) {
        // Decay momentum toward neutral
        if intensityMomentum > 0.01 {
            intensityMomentum -= intensityDecay * deltaTime
            intensityMomentum = max(0, intensityMomentum)
        } else if intensityMomentum < -0.01 {
            intensityMomentum += intensityDecay * deltaTime
            intensityMomentum = min(0, intensityMomentum)
        } else {
            intensityMomentum = 0
        }

        // Hold boosts intensity slightly (sustain = more energy)
        let holdBoost: Float = state.isHolding ? 0.1 : 0

        state.intensity = max(0, min(1, 0.5 + intensityMomentum + holdBoost))
    }

    private func updateAccent(deltaTime: Float) {
        if state.accentStrength > 0 {
            state.accentStrength -= accentDecay * deltaTime
            state.accentStrength = max(0, state.accentStrength)
        }
    }

    private func updateShakeDetection() {
        guard let motion = motionManager.deviceMotion else { return }

        let acc = motion.userAcceleration
        let magnitude = Float(sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z))

        recentAccelerations.append(magnitude)
        if recentAccelerations.count > 10 {
            recentAccelerations.removeFirst()
        }

        // Shake = sustained high acceleration
        let avgAccel = recentAccelerations.reduce(0, +) / Float(recentAccelerations.count)
        state.shakeDetected = avgAccel > shakeThreshold
    }

    private func updateConductance() {
        // Conductance is a composite of all expression inputs
        // It modifies overall arrangement energy
        var c: Float = 0.5

        // Intensity shifts conductance
        c += (state.intensity - 0.5) * 0.4

        // Filter sweep at extremes affects conductance
        if state.filterSweep < 0.2 {
            c -= 0.1  // Closed filter = pull back
        } else if state.filterSweep > 0.8 {
            c += 0.1  // Open filter = push forward
        }

        // Active accent boosts
        c += state.accentStrength * 0.15

        // Shake = chaos boost
        if state.shakeDetected {
            c += 0.2
        }

        state.conductance = max(0, min(1, c))
    }

    // MARK: - Convenience for DiveComposer

    /// Whether any expression input is actively modifying the music right now
    var isExpressing: Bool {
        state.isHolding ||
        abs(state.intensity - 0.5) > 0.1 ||
        state.accentStrength > 0.1 ||
        state.shakeDetected
    }

    /// Velocity modifier based on current expression state.
    /// Applied on top of role velocity and arrangement velocity.
    var velocityModifier: Float {
        var mod: Float = 1.0
        mod *= (0.7 + state.intensity * 0.6)          // Intensity: 0.7x to 1.3x
        mod += state.accentStrength * 0.3               // Accent: up to +0.3
        return max(0.3, min(1.5, mod))
    }

    /// Duration modifier: holding extends notes, high intensity shortens them
    var durationModifier: Float {
        var mod: Float = 1.0
        if state.isHolding { mod *= 1.5 }              // Hold = longer notes
        mod *= (1.3 - state.intensity * 0.6)            // High intensity = shorter (energetic)
        return max(0.3, min(2.0, mod))
    }
}
