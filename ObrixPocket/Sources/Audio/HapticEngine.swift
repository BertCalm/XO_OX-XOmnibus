import UIKit
import CoreHaptics

/// Category-specific haptic patterns for specimen interactions.
/// Provides two layers:
///   - `GameHaptic` / `play(_:)` — rich CoreHaptics patterns for key game events
///   - Legacy static methods — UIFeedbackGenerator shortcuts for high-frequency micro-events
enum HapticEngine {

    // MARK: - GameHaptic (CoreHaptics — Sprint 38)

    /// Distinct haptic events for every meaningful game moment.
    enum GameHaptic {
        case catchSource       // single strong tap       — shell snaps into existence
        case catchProcessor    // rolling vibration       — filter processing hum
        case catchModulator    // rhythmic pulse pattern  — LFO rhythm made tangible
        case catchEffect       // expanding bloom         — reverb tail radiating outward
        case diveBeat          // quantized light tap     — metronome tick underwater
        case zoneTransition    // deep rumble             — crossing into new depth zone
        case chordResolution   // satisfying click        — harmonic lock-in
        case affinityLow       // single tap              — weak coupling spark
        case affinityMedium    // double tap              — moderate bond pulse
        case affinityHigh      // triple crescendo        — strong coupling resonance
        case prismatic         // continuous shimmer      — cosmetic tier glow
    }

    /// Shared CoreHaptics engine instance (lazy so it only starts when first needed).
    private static var _chEngine: CHHapticEngine?

    /// Returns a live CHHapticEngine, starting it if needed. Returns nil on unsupported hardware.
    private static var chEngine: CHHapticEngine? {
        if let e = _chEngine { return e }
        guard CHHapticEngine.capabilitiesForHardware().supportsHaptics else { return nil }
        do {
            let e = try CHHapticEngine()
            e.isAutoShutdownEnabled = true
            try e.start()
            e.stoppedHandler = { _ in _chEngine = nil }
            e.resetHandler = { try? _chEngine?.start() }
            _chEngine = e
            return e
        } catch {
            return nil
        }
    }

    /// Play a rich `GameHaptic` pattern. Falls back to UIFeedbackGenerator if CoreHaptics unavailable.
    static func play(_ haptic: GameHaptic) {
        guard let engine = chEngine else {
            playFallback(haptic)
            return
        }
        do {
            let pattern = try makePattern(haptic)
            let player = try engine.makePlayer(with: pattern)
            try player.start(atTime: CHHapticTimeImmediate)
        } catch {
            playFallback(haptic)
        }
    }

    // MARK: - Pattern Construction

    private static func makePattern(_ haptic: GameHaptic) throws -> CHHapticPattern {
        switch haptic {

        case .catchSource:
            // Single strong rigid tap — a shell snapping shut
            let sharpness = CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.9)
            let intensity = CHHapticEventParameter(parameterID: .hapticIntensity, value: 1.0)
            let event = CHHapticEvent(eventType: .hapticTransient, parameters: [sharpness, intensity], relativeTime: 0)
            return try CHHapticPattern(events: [event], parameters: [])

        case .catchProcessor:
            // Rolling vibration — filter processing hum that winds up then decays
            let rampUp = CHHapticParameterCurve(
                parameterID: .hapticIntensityControl,
                controlPoints: [
                    .init(relativeTime: 0,    value: 0.1),
                    .init(relativeTime: 0.15, value: 0.85),
                    .init(relativeTime: 0.4,  value: 0.6),
                    .init(relativeTime: 0.6,  value: 0.0)
                ],
                relativeTime: 0
            )
            let sharpCurve = CHHapticParameterCurve(
                parameterID: .hapticSharpnessControl,
                controlPoints: [
                    .init(relativeTime: 0,    value: 0.1),
                    .init(relativeTime: 0.15, value: 0.6),
                    .init(relativeTime: 0.6,  value: 0.2)
                ],
                relativeTime: 0
            )
            let continuous = CHHapticEvent(
                eventType: .hapticContinuous,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.7),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.3)
                ],
                relativeTime: 0,
                duration: 0.6
            )
            return try CHHapticPattern(events: [continuous], parameterCurves: [rampUp, sharpCurve])

        case .catchModulator:
            // Rhythmic pulse pattern — 4 even beats like an LFO ticking
            var events: [CHHapticEvent] = []
            for i in 0..<4 {
                let t = Double(i) * 0.12
                let intensity = Float(0.5 + Double(i) * 0.1) // crescendo
                let sharpness = CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.4)
                let intensityParam = CHHapticEventParameter(parameterID: .hapticIntensity, value: intensity)
                events.append(CHHapticEvent(eventType: .hapticTransient, parameters: [sharpness, intensityParam], relativeTime: t))
            }
            return try CHHapticPattern(events: events, parameters: [])

        case .catchEffect:
            // Expanding bloom — soft continuous burst that swells and fades like a reverb tail
            let intensityCurve = CHHapticParameterCurve(
                parameterID: .hapticIntensityControl,
                controlPoints: [
                    .init(relativeTime: 0,    value: 0.3),
                    .init(relativeTime: 0.1,  value: 0.8),
                    .init(relativeTime: 0.35, value: 0.4),
                    .init(relativeTime: 0.7,  value: 0.0)
                ],
                relativeTime: 0
            )
            let sharpnessCurve = CHHapticParameterCurve(
                parameterID: .hapticSharpnessControl,
                controlPoints: [
                    .init(relativeTime: 0,   value: 0.05),
                    .init(relativeTime: 0.7, value: 0.0)
                ],
                relativeTime: 0
            )
            let bloom = CHHapticEvent(
                eventType: .hapticContinuous,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.6),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.05)
                ],
                relativeTime: 0,
                duration: 0.7
            )
            return try CHHapticPattern(events: [bloom], parameterCurves: [intensityCurve, sharpnessCurve])

        case .diveBeat:
            // Quantized light tap — precise underwater metronome click, low sharpness
            let sharpness = CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.2)
            let intensity = CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.3)
            let event = CHHapticEvent(eventType: .hapticTransient, parameters: [sharpness, intensity], relativeTime: 0)
            return try CHHapticPattern(events: [event], parameters: [])

        case .zoneTransition:
            // Deep rumble — low-frequency heavy continuous burst as pressure increases
            let intensityCurve = CHHapticParameterCurve(
                parameterID: .hapticIntensityControl,
                controlPoints: [
                    .init(relativeTime: 0,    value: 0.0),
                    .init(relativeTime: 0.05, value: 0.9),
                    .init(relativeTime: 0.3,  value: 0.7),
                    .init(relativeTime: 0.5,  value: 0.0)
                ],
                relativeTime: 0
            )
            let rumble = CHHapticEvent(
                eventType: .hapticContinuous,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.9),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.0)
                ],
                relativeTime: 0,
                duration: 0.5
            )
            // Accent transient at the peak
            let accent = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.8),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.1)
                ],
                relativeTime: 0.05
            )
            return try CHHapticPattern(events: [rumble, accent], parameterCurves: [intensityCurve])

        case .chordResolution:
            // Satisfying click — sharp transient followed immediately by a soft confirm buzz
            let click = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.85),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 1.0)
                ],
                relativeTime: 0
            )
            let settle = CHHapticEvent(
                eventType: .hapticContinuous,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.3),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.2)
                ],
                relativeTime: 0.03,
                duration: 0.08
            )
            return try CHHapticPattern(events: [click, settle], parameters: [])

        case .affinityLow:
            // Single soft tap — weak coupling spark, barely there
            let event = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.35),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.4)
                ],
                relativeTime: 0
            )
            return try CHHapticPattern(events: [event], parameters: [])

        case .affinityMedium:
            // Double tap — two even pulses, moderate coupling acknowledgement
            let tap1 = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.55),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5)
                ],
                relativeTime: 0
            )
            let tap2 = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.65),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5)
                ],
                relativeTime: 0.12
            )
            return try CHHapticPattern(events: [tap1, tap2], parameters: [])

        case .affinityHigh:
            // Triple crescendo — three building taps, final is heaviest
            let t1 = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.45),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.5)
                ],
                relativeTime: 0
            )
            let t2 = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.7),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.6)
                ],
                relativeTime: 0.12
            )
            let t3 = CHHapticEvent(
                eventType: .hapticTransient,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 1.0),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.7)
                ],
                relativeTime: 0.25
            )
            // Warm buzz tail after the final tap
            let tail = CHHapticEvent(
                eventType: .hapticContinuous,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.4),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.1)
                ],
                relativeTime: 0.28,
                duration: 0.15
            )
            return try CHHapticPattern(events: [t1, t2, t3, tail], parameters: [])

        case .prismatic:
            // Continuous shimmer — soft undulating buzz, low sharpness, irregular amplitude
            let shimmerCurve = CHHapticParameterCurve(
                parameterID: .hapticIntensityControl,
                controlPoints: [
                    .init(relativeTime: 0.0,  value: 0.2),
                    .init(relativeTime: 0.1,  value: 0.5),
                    .init(relativeTime: 0.2,  value: 0.25),
                    .init(relativeTime: 0.3,  value: 0.55),
                    .init(relativeTime: 0.4,  value: 0.2),
                    .init(relativeTime: 0.5,  value: 0.45),
                    .init(relativeTime: 0.65, value: 0.15),
                    .init(relativeTime: 0.8,  value: 0.0)
                ],
                relativeTime: 0
            )
            let shimmer = CHHapticEvent(
                eventType: .hapticContinuous,
                parameters: [
                    CHHapticEventParameter(parameterID: .hapticIntensity, value: 0.35),
                    CHHapticEventParameter(parameterID: .hapticSharpness, value: 0.05)
                ],
                relativeTime: 0,
                duration: 0.8
            )
            return try CHHapticPattern(events: [shimmer], parameterCurves: [shimmerCurve])
        }
    }

    // MARK: - UIFeedbackGenerator Fallback

    /// Called when CoreHaptics is unavailable (older devices / simulator).
    private static func playFallback(_ haptic: GameHaptic) {
        switch haptic {
        case .catchSource:
            rigidImpact.impactOccurred(intensity: 1.0)
        case .catchProcessor:
            mediumImpact.impactOccurred(intensity: 0.7)
        case .catchModulator:
            for i in 0..<4 {
                let delay = Double(i) * 0.12
                DispatchQueue.main.asyncAfter(deadline: .now() + delay) {
                    lightImpact.impactOccurred(intensity: CGFloat(0.4 + Double(i) * 0.15))
                }
            }
        case .catchEffect:
            softImpact.impactOccurred(intensity: 0.6)
        case .diveBeat:
            lightImpact.impactOccurred(intensity: 0.3)
        case .zoneTransition:
            heavyImpact.impactOccurred()
        case .chordResolution:
            notification.notificationOccurred(.success)
        case .affinityLow:
            lightImpact.impactOccurred(intensity: 0.35)
        case .affinityMedium:
            mediumImpact.impactOccurred(intensity: 0.55)
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.12) {
                mediumImpact.impactOccurred(intensity: 0.65)
            }
        case .affinityHigh:
            notification.notificationOccurred(.success)
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.12) {
                heavyImpact.impactOccurred()
            }
        case .prismatic:
            softImpact.impactOccurred(intensity: 0.4)
        }
    }

    // MARK: - UIFeedbackGenerator Pool (legacy + fallback)

    // Pre-prepared generators for lower latency
    private static let lightImpact = UIImpactFeedbackGenerator(style: .light)
    private static let mediumImpact = UIImpactFeedbackGenerator(style: .medium)
    private static let heavyImpact = UIImpactFeedbackGenerator(style: .heavy)
    private static let softImpact = UIImpactFeedbackGenerator(style: .soft)
    private static let rigidImpact = UIImpactFeedbackGenerator(style: .rigid)
    private static let notification = UINotificationFeedbackGenerator()
    private static let selection = UISelectionFeedbackGenerator()

    /// Prepare all generators (call on app launch)
    static func prepare() {
        lightImpact.prepare()
        mediumImpact.prepare()
        heavyImpact.prepare()
        softImpact.prepare()
        rigidImpact.prepare()
        notification.prepare()
        selection.prepare()
    }

    /// Haptic for playing a note on a specimen
    static func noteOn(category: SpecimenCategory, velocity: Float) {
        switch category {
        case .source:
            // Sources: sharp attack, strength = velocity
            rigidImpact.impactOccurred(intensity: CGFloat(velocity))
        case .processor:
            // Processors: soft, filtered feel
            softImpact.impactOccurred(intensity: CGFloat(velocity * 0.7))
        case .modulator:
            // Modulators: light, rhythmic
            lightImpact.impactOccurred(intensity: CGFloat(velocity * 0.5))
        case .effect:
            // Effects: medium, spacious
            mediumImpact.impactOccurred(intensity: CGFloat(velocity * 0.6))
        }
    }

    /// Haptic for keyboard key press
    static func keyPress(velocity: Float) {
        lightImpact.impactOccurred(intensity: CGFloat(max(0.3, velocity)))
    }

    /// Haptic for specimen selection on reef
    static func specimenSelect() {
        selection.selectionChanged()
    }

    /// Haptic for wire creation
    static func wireCreated() {
        notification.notificationOccurred(.success)
    }

    /// Haptic for wire deletion
    static func wireDeleted() {
        lightImpact.impactOccurred(intensity: 0.5)
    }

    /// Haptic for level up
    static func levelUp() {
        notification.notificationOccurred(.success)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
            heavyImpact.impactOccurred()
        }
    }

    /// Haptic for evolution
    static func evolution() {
        notification.notificationOccurred(.success)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) {
            heavyImpact.impactOccurred()
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
            rigidImpact.impactOccurred()
        }
    }

    /// Haptic for catch success
    static func catchSuccess() {
        notification.notificationOccurred(.success)
    }

    /// Haptic for catch failure
    static func catchFailure() {
        notification.notificationOccurred(.error)
    }

    /// Haptic for perfect catch
    static func perfectCatch() {
        notification.notificationOccurred(.success)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) {
            heavyImpact.impactOccurred()
        }
    }

    /// Haptic for daily energy collected
    static func energyCollected() {
        notification.notificationOccurred(.success)
    }

    /// Haptic for milestone unlock
    static func milestoneUnlocked() {
        notification.notificationOccurred(.success)
    }

    /// Haptic for slider drag (param panel)
    static func sliderTick() {
        selection.selectionChanged()
    }

    /// Haptic for chest ceremony phase transitions
    static func chestPhase(_ phase: Int) {
        switch phase {
        case 1: lightImpact.impactOccurred(intensity: 0.4)
        case 2: mediumImpact.impactOccurred()
        case 3: heavyImpact.impactOccurred()
        default: break
        }
    }

    /// Haptic for dive depth milestones (every 200m)
    static func diveDepthMilestone() {
        mediumImpact.impactOccurred()
    }

    /// Haptic for dive beat pulse (subtle tick on each beat)
    static func diveBeat() {
        lightImpact.impactOccurred(intensity: 0.3)
    }
}
