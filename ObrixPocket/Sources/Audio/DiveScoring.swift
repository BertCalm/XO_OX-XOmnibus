import Foundation

/// Individual scoring dimensions for a Dive performance
struct DiveScoreBreakdown: Codable {
    /// How well notes fit the harmonic context (0-100)
    var harmonicCoherence: Float = 0

    /// How much the player interacted (taps, tilts, swipes) (0-100)
    var playerEngagement: Float = 0

    /// Variety of expression used (tap + hold + tilt + swipe = more variety) (0-100)
    var expressionVariety: Float = 0

    /// How clean the run was (inverse of obstacle hits) (0-100)
    var navigationalSkill: Float = 0

    /// Arrangement quality (did voices enter/exit smoothly?) (0-100)
    var arrangementFlow: Float = 0

    /// Overall composite score (weighted average)
    var composite: Float {
        let weights: [Float] = [0.30, 0.20, 0.15, 0.15, 0.20]
        let scores = [harmonicCoherence, playerEngagement, expressionVariety,
                      navigationalSkill, arrangementFlow]
        return zip(weights, scores).reduce(0) { $0 + $1.0 * $1.1 }
    }

    /// Letter grade from composite score
    var grade: String {
        switch composite {
        case 90...:   return "S"
        case 80..<90: return "A"
        case 70..<80: return "B"
        case 60..<70: return "C"
        case 50..<60: return "D"
        default:      return "F"
        }
    }

    /// Star rating (1-5) for quick UI display
    var stars: Int {
        switch composite {
        case 90...:   return 5
        case 75..<90: return 4
        case 60..<75: return 3
        case 40..<60: return 2
        default:      return 1
        }
    }
}

/// A saved memory from a high-scoring Dive
struct DiveMemory: Codable, Identifiable {
    let id: UUID
    let date: Date
    let score: DiveScoreBreakdown
    let durationSeconds: Double
    let depthReached: Float          // Deepest zone reached (0-1000+)
    let specimenNames: [String]      // Specimens that participated
    let sectionReached: String       // Highest arrangement section

    /// File path for the 15-sec audio clip (relative to documents directory)
    let audioClipPath: String?

    /// Whether this memory qualifies as a "best performance" for any specimen
    let isPersonalBest: Bool
}

/// Tracks scoring metrics throughout a Dive session in real-time.
///
/// DiveScoring accumulates events during the Dive and produces a final score
/// breakdown when the Dive ends. It also determines whether to save a "Dive Memory"
/// (high-scoring performances get 15-sec audio clips saved to the specimen journal).
final class DiveScoring {

    // MARK: - Configuration

    /// Minimum composite score to save a Dive Memory
    static let memoryThreshold: Float = 70.0

    /// Maximum memories stored per specimen
    static let maxMemoriesPerSpecimen: Int = 5

    // MARK: - Accumulation State

    /// Total notes played during the Dive
    private var totalNotes: Int = 0

    /// Notes that were in-key (consonant with harmonic context)
    private var inKeyNotes: Int = 0

    /// Notes that were tension notes (dissonant, from obstacles)
    private var tensionNotes: Int = 0

    /// Total player interactions (taps, swipes, tilts beyond threshold)
    private var interactions: Int = 0

    /// Set of distinct interaction types used
    private var interactionTypes: Set<String> = []

    /// Total obstacle hits
    private var obstacleHits: Int = 0

    /// Total obstacles encountered (hits + dodges)
    private var totalObstacles: Int = 0

    /// Section transitions (how many times the arrangement changed section)
    private var sectionTransitions: Int = 0

    /// Number of voices that entered during the Dive
    private var voiceEntries: Int = 0

    /// Deepest zone reached
    private var deepestDepth: Float = 0

    /// Peak number of simultaneous active voices
    private var peakActiveVoices: Int = 0

    /// Elapsed time
    private var elapsed: Double = 0

    /// Names of participating specimens
    private var specimenNames: [String] = []

    /// Highest arrangement section reached
    private var highestSection: ArrangementSection = .intro

    // MARK: - Computed Score

    /// Current live score (updates in real-time for UI display)
    var liveScore: Float {
        guard totalNotes > 0 else { return 0 }
        // Simplified real-time estimate
        let harmony = Float(inKeyNotes) / Float(totalNotes) * 100
        let engagement = min(100, Float(interactions) / max(1, Float(elapsed)) * 50)
        return (harmony * 0.5 + engagement * 0.5)
    }

    // MARK: - Event Registration

    /// Register a note being played. Called by DiveComposer on each note-on.
    func registerNote(midiNote: Int, wasInKey: Bool, wasTension: Bool) {
        totalNotes += 1
        if wasInKey { inKeyNotes += 1 }
        if wasTension { tensionNotes += 1 }
    }

    /// Register a player interaction
    func registerInteraction(type: String) {
        interactions += 1
        interactionTypes.insert(type)
    }

    /// Register an obstacle hit
    func registerObstacleHit() {
        obstacleHits += 1
        totalObstacles += 1
    }

    /// Register an obstacle dodged (passed without hitting)
    func registerObstacleDodged() {
        totalObstacles += 1
    }

    /// Register a section transition
    func registerSectionChange(_ section: ArrangementSection) {
        sectionTransitions += 1
        if sectionPriority(section) > sectionPriority(highestSection) {
            highestSection = section
        }
    }

    /// Register a voice entering the arrangement
    func registerVoiceEntry() {
        voiceEntries += 1
    }

    /// Update depth (called by DiveTab)
    func updateDepth(_ depth: Float) {
        deepestDepth = max(deepestDepth, depth)
    }

    /// Update active voice count
    func updateActiveVoices(_ count: Int) {
        peakActiveVoices = max(peakActiveVoices, count)
    }

    /// Update elapsed time
    func updateElapsed(_ t: Double) {
        elapsed = t
    }

    /// Set participating specimen names (call once at dive start)
    func setSpecimens(_ names: [String]) {
        specimenNames = names
    }

    // MARK: - Final Score Calculation

    /// Calculate the final score breakdown. Call when the Dive ends.
    func calculateFinalScore() -> DiveScoreBreakdown {
        var score = DiveScoreBreakdown()

        // 1. Harmonic Coherence (30% weight)
        // High in-key ratio = high score. Some tension is good (shows obstacle engagement).
        if totalNotes > 0 {
            let inKeyRatio = Float(inKeyNotes) / Float(totalNotes)
            let tensionRatio = Float(tensionNotes) / Float(totalNotes)
            // Perfect score: >90% in-key with 5-15% tension (shows musical navigation)
            let baseHarmony = inKeyRatio * 100
            let tensionBonus = tensionRatio > 0.05 && tensionRatio < 0.2 ? 10.0 : 0.0
            score.harmonicCoherence = min(100, baseHarmony + Float(tensionBonus))
        }

        // 2. Player Engagement (20% weight)
        // Interactions per second, normalized. 1+ interaction/sec = good.
        if elapsed > 0 {
            let ips = Float(interactions) / Float(elapsed)
            score.playerEngagement = min(100, ips * 60)  // 1.67 ips = 100
        }

        // 3. Expression Variety (15% weight)
        // How many different interaction types used
        let maxTypes: Float = 5  // tap, hold, swipeUp, swipeDown, tilt
        let varietyRatio = Float(interactionTypes.count) / maxTypes
        score.expressionVariety = varietyRatio * 100

        // 4. Navigational Skill (15% weight)
        // Dodge ratio
        if totalObstacles > 0 {
            let dodgeRatio = Float(totalObstacles - obstacleHits) / Float(totalObstacles)
            score.navigationalSkill = dodgeRatio * 100
        } else {
            score.navigationalSkill = 100  // No obstacles = perfect (shouldn't happen)
        }

        // 5. Arrangement Flow (20% weight)
        // Did the arrangement build properly? Did voices enter?
        let expectedTransitions: Float = 3  // intro→build→peak→outro
        let transitionScore = min(1, Float(sectionTransitions) / expectedTransitions)
        let voiceScore = min(1, Float(voiceEntries) / max(1, Float(peakActiveVoices)))
        score.arrangementFlow = (transitionScore * 50 + voiceScore * 50)

        return score
    }

    /// Determine if this Dive qualifies for a Dive Memory
    func shouldSaveMemory(score: DiveScoreBreakdown) -> Bool {
        score.composite >= Self.memoryThreshold
    }

    /// Create a DiveMemory record from the final score
    func createMemory(score: DiveScoreBreakdown, audioClipPath: String?, isPersonalBest: Bool) -> DiveMemory {
        DiveMemory(
            id: UUID(),
            date: Date(),
            score: score,
            durationSeconds: elapsed,
            depthReached: deepestDepth,
            specimenNames: specimenNames,
            sectionReached: highestSection.rawValue,
            audioClipPath: audioClipPath,
            isPersonalBest: isPersonalBest
        )
    }

    // MARK: - Reset

    func reset() {
        totalNotes = 0
        inKeyNotes = 0
        tensionNotes = 0
        interactions = 0
        interactionTypes = []
        obstacleHits = 0
        totalObstacles = 0
        sectionTransitions = 0
        voiceEntries = 0
        deepestDepth = 0
        peakActiveVoices = 0
        elapsed = 0
        specimenNames = []
        highestSection = .intro
    }

    // MARK: - Helpers

    private func sectionPriority(_ section: ArrangementSection) -> Int {
        switch section {
        case .intro: return 0
        case .build: return 1
        case .peak:  return 2
        case .outro: return 3
        }
    }
}
