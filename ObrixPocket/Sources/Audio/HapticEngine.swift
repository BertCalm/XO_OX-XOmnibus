import UIKit
import CoreHaptics

/// Category-specific haptic patterns for specimen interactions
enum HapticEngine {

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
