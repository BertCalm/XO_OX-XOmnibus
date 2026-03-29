import Foundation

/// Sections of a Dive arrangement — each has distinct musical behavior
enum ArrangementSection: String, CaseIterable {
    case intro      // 0-10s: One voice emerges from silence
    case build      // 10-30s: Voices layer in progressively
    case peak       // 30-45s: Full arrangement, all voices active
    case outro      // 45-60s: Voices thin, return to simplicity

    /// Time range within a 60-second Dive (in seconds)
    var timeRange: ClosedRange<Double> {
        switch self {
        case .intro: return 0...10
        case .build: return 10...30
        case .peak:  return 30...45
        case .outro: return 45...60
        }
    }

    /// Maximum number of note-generating voices active in this section
    var maxActiveVoices: Int {
        switch self {
        case .intro: return 1
        case .build: return 3
        case .peak:  return 6    // All voices if available
        case .outro: return 2
        }
    }

    /// Overall energy level (0-1) for this section
    var energy: Float {
        switch self {
        case .intro: return 0.2
        case .build: return 0.6
        case .peak:  return 1.0
        case .outro: return 0.3
        }
    }

    /// Velocity multiplier for this section
    var velocityMultiplier: Float {
        switch self {
        case .intro: return 0.5
        case .build: return 0.75
        case .peak:  return 1.0
        case .outro: return 0.6
        }
    }

    /// Whether chord changes are allowed in this section
    var allowsChordChanges: Bool {
        switch self {
        case .intro: return false   // Stay on one chord during intro
        case .build: return true
        case .peak:  return true
        case .outro: return true
        }
    }
}

/// Manages the musical arrangement structure of a Dive.
///
/// The ArrangementEngine tracks elapsed time and determines:
/// - Which section of the arrangement we're in (intro/build/peak/outro)
/// - How many and which voices should be active
/// - Energy level and density targets
/// - When voices should enter and exit
///
/// It does NOT generate notes — it tells DiveComposer which roles are allowed
/// to fire at any given moment.
final class ArrangementEngine {

    // MARK: - Properties

    /// Current arrangement section
    private(set) var currentSection: ArrangementSection = .intro

    /// Total dive duration in seconds
    let diveDuration: Double

    /// Elapsed time since dive start
    private(set) var elapsed: Double = 0

    /// All available roles for this dive (set from reef specimens)
    private var allRoles: [MusicalRole] = []

    /// Currently active roles (subset of allRoles, managed by arrangement)
    private(set) var activeRoles: [MusicalRole] = []

    /// Voice entry schedule: at what elapsed time each role enters
    private var entrySchedule: [(time: Double, role: MusicalRole)] = []

    /// Voice exit schedule: at what elapsed time each role exits (for outro)
    private var exitSchedule: [(time: Double, role: MusicalRole)] = []

    /// Current arrangement tension (0-1). Rises through build, peaks, descends in outro.
    private(set) var tension: Float = 0

    /// Density target: how many subdivisions should have notes (0-1)
    private(set) var densityTarget: Float = 0.2

    /// Callbacks
    var onSectionChange: ((ArrangementSection) -> Void)?
    var onVoiceEnter: ((MusicalRole) -> Void)?
    var onVoiceExit: ((MusicalRole) -> Void)?

    // MARK: - Init

    init(diveDuration: Double = 60.0) {
        self.diveDuration = diveDuration
    }

    // MARK: - Setup

    /// Configure the arrangement for a set of musical roles.
    /// Call before the dive starts. Builds the entry/exit schedule.
    func configure(roles: [MusicalRole]) {
        // Only track note-generating roles for entry/exit scheduling
        let noteRoles = roles.filter { $0.generatesNotes }
        allRoles = noteRoles
        activeRoles = []
        entrySchedule = []
        exitSchedule = []

        guard !noteRoles.isEmpty else { return }

        // Sort roles by priority: anchor voices first, then by musical importance
        let sorted = noteRoles.sorted { rolePriority($0) < rolePriority($1) }

        // Build entry schedule: stagger voice entries through intro and build
        // First voice enters at 2s (give the player a moment of silence)
        // Subsequent voices enter every 4-8 seconds through the build
        let entryStart = 2.0
        let entryEnd = 28.0  // All voices should be in by late build
        let entrySpacing = min(8.0, (entryEnd - entryStart) / Double(max(1, sorted.count)))

        for (i, role) in sorted.enumerated() {
            let entryTime = entryStart + Double(i) * entrySpacing
            entrySchedule.append((time: entryTime, role: role))
        }

        // Build exit schedule: voices drop out during outro (reverse priority order)
        let exitStart = 46.0
        let exitEnd = 56.0
        let exitSpacing = min(5.0, (exitEnd - exitStart) / Double(max(1, sorted.count - 1)))

        // Keep the highest-priority voice (bass/anchor) until the very end
        let exitOrder = sorted.reversed()
        for (i, role) in exitOrder.enumerated() {
            // Always keep at least one voice
            if i < exitOrder.count - 1 {
                let exitTime = exitStart + Double(i) * exitSpacing
                exitSchedule.append((time: exitTime, role: role))
            }
        }
    }

    /// Role priority for entry order (lower = enters first)
    private func rolePriority(_ role: MusicalRole) -> Int {
        switch role {
        case .bass:    return 0  // Bass enters first (foundation)
        case .harmony: return 1  // Harmony second (establishes chords)
        case .melody:  return 2  // Melody third (the hook)
        case .rhythm:  return 3  // Rhythm last (fills in the groove)
        case .texture: return 4
        case .effect:  return 5
        }
    }

    // MARK: - Update

    /// Update the arrangement state. Call every frame or every beat.
    /// Returns the current set of active roles.
    @discardableResult
    func update(elapsed: Double) -> [MusicalRole] {
        self.elapsed = elapsed

        // Determine current section
        let newSection = sectionForTime(elapsed)
        if newSection != currentSection {
            currentSection = newSection
            onSectionChange?(newSection)
        }

        // Process voice entries
        for entry in entrySchedule {
            if elapsed >= entry.time && !activeRoles.contains(entry.role) {
                activeRoles.append(entry.role)
                onVoiceEnter?(entry.role)
            }
        }

        // Process voice exits (only during outro)
        if currentSection == .outro {
            for exit in exitSchedule {
                if elapsed >= exit.time {
                    activeRoles.removeAll { $0 == exit.role }
                    onVoiceExit?(exit.role)
                }
            }
        }

        // Update tension curve
        tension = tensionForTime(elapsed)

        // Update density target
        densityTarget = densityForSection(currentSection, progress: sectionProgress(elapsed))

        return activeRoles
    }

    // MARK: - Queries

    /// Whether a specific role is allowed to generate notes right now
    func isRoleActive(_ role: MusicalRole) -> Bool {
        activeRoles.contains(role)
    }

    /// Get velocity multiplier for the current arrangement position
    var currentVelocityMultiplier: Float {
        let base = currentSection.velocityMultiplier
        // Smooth transitions: lerp based on progress within section
        let progress = sectionProgress(elapsed)

        switch currentSection {
        case .intro:
            return base * (0.3 + progress * 0.7)  // Fade in
        case .build:
            return base + progress * 0.25           // Gradual increase
        case .peak:
            return base                              // Full power
        case .outro:
            return base * (1.0 - progress * 0.5)   // Fade down
        }
    }

    /// Suggested tempo modification (1.0 = no change)
    var tempoMultiplier: Float {
        switch currentSection {
        case .intro: return 0.9    // Slightly slower intro
        case .build: return 1.0
        case .peak:  return 1.05   // Slight urgency at peak
        case .outro: return 0.95   // Gentle deceleration
        }
    }

    // MARK: - Helpers

    private func sectionForTime(_ t: Double) -> ArrangementSection {
        let ratio = t / diveDuration
        if ratio <= 0.167 { return .intro }       // 0-10s of 60s
        if ratio <= 0.5   { return .build }       // 10-30s
        if ratio <= 0.75  { return .peak }        // 30-45s
        return .outro                              // 45-60s
    }

    /// Progress within the current section (0-1)
    private func sectionProgress(_ t: Double) -> Float {
        let range = currentSection.timeRange
        let duration = range.upperBound - range.lowerBound
        guard duration > 0 else { return 0 }
        return Float(max(0, min(1, (t - range.lowerBound) / duration)))
    }

    /// Tension curve: S-shaped rise through build, plateau at peak, descent in outro
    private func tensionForTime(_ t: Double) -> Float {
        let ratio = Float(t / diveDuration)

        if ratio < 0.167 {
            // Intro: minimal tension, gentle rise
            return ratio / 0.167 * 0.15
        } else if ratio < 0.5 {
            // Build: S-curve from 0.15 to 0.85
            let buildProgress = (ratio - 0.167) / 0.333
            let sCurve = buildProgress * buildProgress * (3.0 - 2.0 * buildProgress) // smoothstep
            return 0.15 + sCurve * 0.7
        } else if ratio < 0.75 {
            // Peak: high tension with slight pulsing
            let peakProgress = (ratio - 0.5) / 0.25
            let pulse = sin(peakProgress * .pi * 2) * 0.05  // Subtle breathing
            return 0.85 + pulse
        } else {
            // Outro: gradual descent
            let outroProgress = (ratio - 0.75) / 0.25
            return 0.85 * (1.0 - outroProgress * 0.7)
        }
    }

    /// Density target for a section (how many notes per subdivision)
    private func densityForSection(_ section: ArrangementSection, progress: Float) -> Float {
        switch section {
        case .intro: return 0.1 + progress * 0.1   // Very sparse
        case .build: return 0.2 + progress * 0.4   // Gradually filling
        case .peak:  return 0.6 + progress * 0.15  // Dense
        case .outro: return 0.5 * (1.0 - progress * 0.6) // Thinning
        }
    }

    // MARK: - Reset

    func reset() {
        elapsed = 0
        currentSection = .intro
        activeRoles = []
        tension = 0
        densityTarget = 0.2
    }
}
