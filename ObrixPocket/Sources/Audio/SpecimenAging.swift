import Foundation

/// Life stages of a specimen, progressing through real-time aging
enum LifeStage: String, Codable, CaseIterable, Comparable {
    case spawn      // Day 1-3: Simple, pure tones
    case juvenile   // Day 3-14: Developing complexity
    case adult      // Day 14-60: Full voice
    case elder      // Day 60+: Rich, harmonically complex, wise

    /// Minimum age in days to reach this stage
    var minimumDays: Int {
        switch self {
        case .spawn:    return 0
        case .juvenile: return 3
        case .adult:    return 14
        case .elder:    return 60
        }
    }

    /// Display name for UI
    var displayName: String {
        switch self {
        case .spawn:    return "Spawn"
        case .juvenile: return "Juvenile"
        case .adult:    return "Adult"
        case .elder:    return "Elder"
        }
    }

    /// Voice complexity multiplier. Spawn = simple, Elder = rich.
    var complexityMultiplier: Float {
        switch self {
        case .spawn:    return 0.3
        case .juvenile: return 0.6
        case .adult:    return 1.0
        case .elder:    return 1.4
        }
    }

    /// Harmonic richness: how many overtones/passing tones this stage allows
    var harmonicRichness: Float {
        switch self {
        case .spawn:    return 0.1   // Very few harmonics — pure
        case .juvenile: return 0.4   // Some development
        case .adult:    return 0.7   // Full voice
        case .elder:    return 1.0   // Maximum richness
        }
    }

    /// Filter openness: how bright the voice is
    var filterOpenness: Float {
        switch self {
        case .spawn:    return 0.3   // Dark, muffled (new to the world)
        case .juvenile: return 0.5   // Opening up
        case .adult:    return 0.7   // Clear
        case .elder:    return 0.85  // Bright, full spectrum wisdom
        }
    }

    /// Velocity stability: how consistent the volume is
    var velocityStability: Float {
        switch self {
        case .spawn:    return 0.4   // Unpredictable, learning
        case .juvenile: return 0.6   // Finding its voice
        case .adult:    return 0.85  // Consistent
        case .elder:    return 0.95  // Rock solid
        }
    }

    /// Whether this stage can teach younger specimens through wiring
    var canTeach: Bool {
        switch self {
        case .spawn, .juvenile: return false
        case .adult, .elder:    return true
        }
    }

    /// Visual border color indicator
    var borderColorHex: String {
        switch self {
        case .spawn:    return "#AAAAAA"  // Silver
        case .juvenile: return "#7BC8A4"  // Green
        case .adult:    return "#4A90D9"  // Blue
        case .elder:    return "#E9C46A"  // Gold (XO Gold)
        }
    }

    static func < (lhs: LifeStage, rhs: LifeStage) -> Bool {
        lhs.minimumDays < rhs.minimumDays
    }
}

/// Aging record for a single specimen
struct SpecimenAgeRecord: Codable, Identifiable {
    let id: UUID
    let slotIndex: Int
    let subtypeID: String
    let birthDate: Date
    var lastInteractionDate: Date
    var totalPlayMinutes: Double       // Cumulative time this specimen has been played
    var totalDiveParticipations: Int   // How many Dives this specimen has been in
    var teachingEvents: Int            // Times this specimen taught a younger one

    /// Age in real-time days since birth
    var ageDays: Int {
        Calendar.current.dateComponents([.day], from: birthDate, to: Date()).day ?? 0
    }

    /// Current life stage based on age
    var stage: LifeStage {
        let days = ageDays
        if days >= LifeStage.elder.minimumDays { return .elder }
        if days >= LifeStage.adult.minimumDays { return .adult }
        if days >= LifeStage.juvenile.minimumDays { return .juvenile }
        return .spawn
    }

    /// Progress within current stage (0-1)
    var stageProgress: Float {
        let days = ageDays
        let currentMin = stage.minimumDays
        let nextStage = LifeStage.allCases.first { $0.minimumDays > currentMin }
        let nextMin = nextStage?.minimumDays ?? (currentMin + 60)
        let range = nextMin - currentMin
        guard range > 0 else { return 1.0 }
        return Float(days - currentMin) / Float(range)
    }

    /// Whether this specimen has been interacted with recently (last 24 hours)
    var isActive: Bool {
        let hours = Calendar.current.dateComponents([.hour], from: lastInteractionDate, to: Date()).hour ?? 0
        return hours < 24
    }

    /// Display string: "3 days old (Spawn)"
    var ageDisplayString: String {
        let days = ageDays
        let dayStr = days == 1 ? "1 day" : "\(days) days"
        return "\(dayStr) old (\(stage.displayName))"
    }
}

/// Parameter modifications applied based on a specimen's life stage.
/// These are multipliers/offsets on top of the specimen's base parameters.
struct AgingModifiers {
    let filterCutoffMultiplier: Float   // Multiply base filter cutoff
    let resonanceOffset: Float          // Add to base resonance
    let lfoDepthMultiplier: Float       // Multiply LFO depth (more modulation with age)
    let attackMultiplier: Float         // Modify attack time
    let releaseMultiplier: Float        // Modify release time
    let detuneOffset: Float             // Slight detune that develops with age (character)

    /// Generate modifiers for a given life stage
    static func forStage(_ stage: LifeStage) -> AgingModifiers {
        switch stage {
        case .spawn:
            return AgingModifiers(
                filterCutoffMultiplier: 0.7,   // Darker
                resonanceOffset: -0.05,
                lfoDepthMultiplier: 0.5,        // Less modulation
                attackMultiplier: 1.3,           // Slightly slower attack
                releaseMultiplier: 0.8,          // Shorter release
                detuneOffset: 0.0
            )
        case .juvenile:
            return AgingModifiers(
                filterCutoffMultiplier: 0.85,
                resonanceOffset: 0.0,
                lfoDepthMultiplier: 0.75,
                attackMultiplier: 1.1,
                releaseMultiplier: 0.9,
                detuneOffset: 0.01             // Slight character developing
            )
        case .adult:
            return AgingModifiers(
                filterCutoffMultiplier: 1.0,    // Full brightness
                resonanceOffset: 0.0,
                lfoDepthMultiplier: 1.0,        // Full modulation
                attackMultiplier: 1.0,
                releaseMultiplier: 1.0,
                detuneOffset: 0.015
            )
        case .elder:
            return AgingModifiers(
                filterCutoffMultiplier: 1.15,   // Brighter than base (wisdom)
                resonanceOffset: 0.05,          // Slight resonance peak (character)
                lfoDepthMultiplier: 1.3,        // Rich modulation
                attackMultiplier: 0.9,           // Slightly faster (confident)
                releaseMultiplier: 1.3,          // Longer tail (gravitas)
                detuneOffset: 0.02              // Distinct character
            )
        }
    }

    /// Apply modifiers to a set of parameters, returning modified values
    func apply(to params: [String: Float]) -> [String: Float] {
        var result = params
        if let cutoff = result["obrix_flt1Cutoff"] {
            result["obrix_flt1Cutoff"] = min(0.95, cutoff * filterCutoffMultiplier)
        }
        if let res = result["obrix_flt1Resonance"] {
            result["obrix_flt1Resonance"] = max(0, min(0.9, res + resonanceOffset))
        }
        if let depth = result["obrix_lfo1Depth"] {
            result["obrix_lfo1Depth"] = min(1.0, depth * lfoDepthMultiplier)
        }
        if let attack = result["obrix_env1Attack"] {
            result["obrix_env1Attack"] = max(0.001, attack * attackMultiplier)
        }
        if let release = result["obrix_env1Release"] {
            result["obrix_env1Release"] = min(5.0, release * releaseMultiplier)
        }
        if let tune = result["obrix_src1Tune"] {
            result["obrix_src1Tune"] = tune + detuneOffset
        }
        return result
    }
}

/// Manages aging across all specimens on the reef.
///
/// Tracks birth dates, life stages, play time, and generates
/// parameter modifiers that change how specimens sound as they age.
/// Spawn = simple/pure. Elder = complex/harmonically rich.
///
/// Elders can "teach" younger specimens through wiring:
/// when an Elder is wired to a Spawn, the Spawn gains a small
/// boost to its harmonic richness (accelerated maturation).
final class SpecimenAgingManager: ObservableObject {

    // MARK: - State

    @Published var ages: [Int: SpecimenAgeRecord] = [:]  // slotIndex → age record

    // MARK: - Registration

    /// Register a newly caught specimen
    func registerBirth(slotIndex: Int, subtypeID: String) {
        ages[slotIndex] = SpecimenAgeRecord(
            id: UUID(),
            slotIndex: slotIndex,
            subtypeID: subtypeID,
            birthDate: Date(),
            lastInteractionDate: Date(),
            totalPlayMinutes: 0,
            totalDiveParticipations: 0,
            teachingEvents: 0
        )
    }

    /// Remove a specimen's age record (released/traded)
    func removeSpecimen(slotIndex: Int) {
        ages.removeValue(forKey: slotIndex)
    }

    // MARK: - Interaction Tracking

    /// Record that a specimen was played
    func recordPlay(slotIndex: Int, durationMinutes: Double) {
        ages[slotIndex]?.lastInteractionDate = Date()
        ages[slotIndex]?.totalPlayMinutes += durationMinutes
    }

    /// Record that a specimen participated in a Dive
    func recordDiveParticipation(slotIndex: Int) {
        ages[slotIndex]?.lastInteractionDate = Date()
        ages[slotIndex]?.totalDiveParticipations += 1
    }

    /// Record a teaching event (Elder wired to younger specimen)
    func recordTeaching(elderSlot: Int) {
        ages[elderSlot]?.teachingEvents += 1
    }

    // MARK: - Queries

    /// Get the life stage for a slot
    func stage(for slotIndex: Int) -> LifeStage {
        ages[slotIndex]?.stage ?? .spawn
    }

    /// Get aging modifiers for a slot's current life stage
    func modifiers(for slotIndex: Int) -> AgingModifiers {
        AgingModifiers.forStage(stage(for: slotIndex))
    }

    /// Get modified parameters for a slot (base params + aging modifiers)
    func agedParams(for slotIndex: Int, baseParams: [String: Float]) -> [String: Float] {
        modifiers(for: slotIndex).apply(to: baseParams)
    }

    /// Find all Elder specimens on the reef
    var elders: [Int] {
        ages.filter { $0.value.stage == .elder }.map { $0.key }
    }

    /// Find all Spawn specimens on the reef
    var spawns: [Int] {
        ages.filter { $0.value.stage == .spawn }.map { $0.key }
    }

    /// Check if teaching is possible between two wired slots
    func canTeach(from elderSlot: Int, to youngSlot: Int) -> Bool {
        guard let elder = ages[elderSlot], let young = ages[youngSlot] else { return false }
        return elder.stage.canTeach && young.stage < .adult
    }

    // MARK: - Persistence

    func save() {
        if let data = try? JSONEncoder().encode(Array(ages.values)) {
            UserDefaults.standard.set(data, forKey: "specimenAges")
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: "specimenAges"),
           let records = try? JSONDecoder().decode([SpecimenAgeRecord].self, from: data) {
            ages = [:]
            for record in records {
                ages[record.slotIndex] = record
            }
        }
    }
}
