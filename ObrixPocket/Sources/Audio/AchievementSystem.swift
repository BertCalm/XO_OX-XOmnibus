import Foundation
import UIKit

// MARK: - AchievementCategory

enum AchievementCategory: String, Codable, CaseIterable {
    case collection  = "Collection"
    case breeding    = "Breeding"
    case performance = "Performance"
    case exploration = "Exploration"
    case mastery     = "Mastery"
    case social      = "Social"
    case seasonal    = "Seasonal"
    case secret      = "Secret"

    var displayName: String { rawValue }

    var symbolName: String {
        switch self {
        case .collection:  return "square.grid.2x2.fill"
        case .breeding:    return "leaf.fill"
        case .performance: return "music.note.list"
        case .exploration: return "map.fill"
        case .mastery:     return "crown.fill"
        case .social:      return "person.2.fill"
        case .seasonal:    return "sun.max.fill"
        case .secret:      return "questionmark.circle.fill"
        }
    }
}

// MARK: - AchievementTier

enum AchievementTier: String, Codable, CaseIterable, Comparable {
    case bronze   = "Bronze"
    case silver   = "Silver"
    case gold     = "Gold"
    case platinum = "Platinum"
    case mythic   = "Mythic"

    var pointValue: Int {
        switch self {
        case .bronze:   return 10
        case .silver:   return 25
        case .gold:     return 50
        case .platinum: return 100
        case .mythic:   return 500
        }
    }

    var symbolName: String {
        switch self {
        case .bronze:   return "medal.fill"
        case .silver:   return "star.fill"
        case .gold:     return "star.circle.fill"
        case .platinum: return "sparkles"
        case .mythic:   return "crown.fill"
        }
    }

    static func < (lhs: AchievementTier, rhs: AchievementTier) -> Bool {
        lhs.pointValue < rhs.pointValue
    }
}

// MARK: - AchievementRequirement

enum AchievementRequirement: Codable {
    case specimenCount(count: Int)
    case uniqueSubtypes(count: Int)
    case breedingGeneration(gen: Int)
    case breedingTotal(count: Int)
    case diveHighScore(score: Int)
    case consecutiveDays(days: Int)
    case couplingTypesUsed(count: Int)
    case arrangementLength(seconds: Int)
    case seasonalComplete(count: Int)
    case loreDiscovered(count: Int)
    case elderSpecimens(count: Int)
    case totalPlayTime(hours: Int)
    case exportedMIDI(count: Int)
    case tradesCompleted(count: Int)
    case diveDepth(depth: Int)
    case secretCondition(id: String)

    // MARK: Codable

    private enum CodingKeys: String, CodingKey {
        case type, count, gen, days, score, seconds, hours, id, depth
    }

    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        switch self {
        case .specimenCount(let count):
            try container.encode("specimenCount", forKey: .type)
            try container.encode(count, forKey: .count)
        case .uniqueSubtypes(let count):
            try container.encode("uniqueSubtypes", forKey: .type)
            try container.encode(count, forKey: .count)
        case .breedingGeneration(let gen):
            try container.encode("breedingGeneration", forKey: .type)
            try container.encode(gen, forKey: .gen)
        case .breedingTotal(let count):
            try container.encode("breedingTotal", forKey: .type)
            try container.encode(count, forKey: .count)
        case .diveHighScore(let score):
            try container.encode("diveHighScore", forKey: .type)
            try container.encode(score, forKey: .score)
        case .consecutiveDays(let days):
            try container.encode("consecutiveDays", forKey: .type)
            try container.encode(days, forKey: .days)
        case .couplingTypesUsed(let count):
            try container.encode("couplingTypesUsed", forKey: .type)
            try container.encode(count, forKey: .count)
        case .arrangementLength(let seconds):
            try container.encode("arrangementLength", forKey: .type)
            try container.encode(seconds, forKey: .seconds)
        case .seasonalComplete(let count):
            try container.encode("seasonalComplete", forKey: .type)
            try container.encode(count, forKey: .count)
        case .loreDiscovered(let count):
            try container.encode("loreDiscovered", forKey: .type)
            try container.encode(count, forKey: .count)
        case .elderSpecimens(let count):
            try container.encode("elderSpecimens", forKey: .type)
            try container.encode(count, forKey: .count)
        case .totalPlayTime(let hours):
            try container.encode("totalPlayTime", forKey: .type)
            try container.encode(hours, forKey: .hours)
        case .exportedMIDI(let count):
            try container.encode("exportedMIDI", forKey: .type)
            try container.encode(count, forKey: .count)
        case .tradesCompleted(let count):
            try container.encode("tradesCompleted", forKey: .type)
            try container.encode(count, forKey: .count)
        case .diveDepth(let depth):
            try container.encode("diveDepth", forKey: .type)
            try container.encode(depth, forKey: .depth)
        case .secretCondition(let id):
            try container.encode("secretCondition", forKey: .type)
            try container.encode(id, forKey: .id)
        }
    }

    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        let type = try container.decode(String.self, forKey: .type)
        switch type {
        case "specimenCount":      self = .specimenCount(count: try container.decode(Int.self, forKey: .count))
        case "uniqueSubtypes":     self = .uniqueSubtypes(count: try container.decode(Int.self, forKey: .count))
        case "breedingGeneration": self = .breedingGeneration(gen: try container.decode(Int.self, forKey: .gen))
        case "breedingTotal":      self = .breedingTotal(count: try container.decode(Int.self, forKey: .count))
        case "diveHighScore":      self = .diveHighScore(score: try container.decode(Int.self, forKey: .score))
        case "consecutiveDays":    self = .consecutiveDays(days: try container.decode(Int.self, forKey: .days))
        case "couplingTypesUsed":  self = .couplingTypesUsed(count: try container.decode(Int.self, forKey: .count))
        case "arrangementLength":  self = .arrangementLength(seconds: try container.decode(Int.self, forKey: .seconds))
        case "seasonalComplete":   self = .seasonalComplete(count: try container.decode(Int.self, forKey: .count))
        case "loreDiscovered":     self = .loreDiscovered(count: try container.decode(Int.self, forKey: .count))
        case "elderSpecimens":     self = .elderSpecimens(count: try container.decode(Int.self, forKey: .count))
        case "totalPlayTime":      self = .totalPlayTime(hours: try container.decode(Int.self, forKey: .hours))
        case "exportedMIDI":       self = .exportedMIDI(count: try container.decode(Int.self, forKey: .count))
        case "tradesCompleted":    self = .tradesCompleted(count: try container.decode(Int.self, forKey: .count))
        case "diveDepth":          self = .diveDepth(depth: try container.decode(Int.self, forKey: .depth))
        case "secretCondition":    self = .secretCondition(id: try container.decode(String.self, forKey: .id))
        default:                   self = .secretCondition(id: "unknown")
        }
    }
}

// MARK: - AchievementReward

struct AchievementReward: Codable {
    let description: String
    let bonusXP: Int
}

// MARK: - Achievement

struct Achievement: Identifiable, Codable {
    let id: String
    let title: String
    let description: String
    let category: AchievementCategory
    let tier: AchievementTier
    let requirement: AchievementRequirement
    var isCompleted: Bool
    var completedDate: Date?
    let reward: AchievementReward

    /// For secret achievements: title and description are hidden until completed
    let isSecret: Bool

    var displayTitle: String {
        isSecret && !isCompleted ? "???" : title
    }

    var displayDescription: String {
        isSecret && !isCompleted ? "A hidden achievement. Keep exploring." : description
    }
}

// MARK: - PlayerTier

enum PlayerTier: String, CaseIterable {
    case newcomer       = "Newcomer"
    case bronzeCollector = "Bronze Collector"
    case silverExplorer = "Silver Explorer"
    case goldMaster     = "Gold Master"
    case platinumLegend = "Platinum Legend"
    case mythic         = "Mythic"

    var pointThreshold: Int {
        switch self {
        case .newcomer:        return 0
        case .bronzeCollector: return 100
        case .silverExplorer:  return 500
        case .goldMaster:      return 2000
        case .platinumLegend:  return 5000
        case .mythic:          return 10000
        }
    }

    var symbolName: String {
        switch self {
        case .newcomer:        return "drop"
        case .bronzeCollector: return "medal.fill"
        case .silverExplorer:  return "star.fill"
        case .goldMaster:      return "star.circle.fill"
        case .platinumLegend:  return "sparkles"
        case .mythic:          return "crown.fill"
        }
    }

    static func tier(for points: Int) -> PlayerTier {
        PlayerTier.allCases.reversed().first { points >= $0.pointThreshold } ?? .newcomer
    }

    var nextTier: PlayerTier? {
        let all = PlayerTier.allCases
        guard let idx = all.firstIndex(of: self), idx + 1 < all.count else { return nil }
        return all[idx + 1]
    }
}

// MARK: - AchievementManager

final class AchievementManager: ObservableObject {

    static let shared = AchievementManager()

    @Published var achievements: [Achievement] = []
    @Published var achievementPoints: Int = 0
    @Published var pendingNotifications: [Achievement] = []

    private let storageKey = "obrix_achievements"
    private let pointsKey  = "obrix_achievement_points"

    // MARK: Secret Hints (appear after 10+ hours of play)

    let secretHints: [String: String] = [
        "sec_001": "Something extraordinary happens when the deepest voice meets the brightest.",
        "sec_002": "Time itself is a kind of collection. What have you let grow old?",
        "sec_003": "The Reef has a memory. Have you asked it what it remembers?",
    ]

    init() {
        load()
        if achievements.isEmpty { generateAchievements() }
        recalculatePoints()
    }

    // MARK: - Computed Properties

    var completedAchievements: [Achievement] { achievements.filter { $0.isCompleted } }
    var pendingAchievements: [Achievement]   { achievements.filter { !$0.isCompleted } }
    var playerTier: PlayerTier               { PlayerTier.tier(for: achievementPoints) }

    var completionPercentage: Double {
        guard !achievements.isEmpty else { return 0.0 }
        return Double(completedAchievements.count) / Double(achievements.count) * 100.0
    }

    var achievementsByCategory: [AchievementCategory: [Achievement]] {
        Dictionary(grouping: achievements, by: { $0.category })
    }

    func pointsUntilNextTier() -> Int? {
        guard let next = playerTier.nextTier else { return nil }
        return max(0, next.pointThreshold - achievementPoints)
    }

    func secretHint(for achievementId: String, playHours: Int) -> String? {
        guard playHours >= 10 else { return nil }
        return secretHints[achievementId]
    }

    // MARK: - Completion Triggers

    /// Call when total specimen count changes
    func onSpecimenCountChanged(_ count: Int) {
        complete(where: { if case .specimenCount(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when unique subtype count changes
    func onUniqueSubtypesChanged(_ count: Int) {
        complete(where: { if case .uniqueSubtypes(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when a new breeding generation is achieved
    func onBreedingGenerationReached(_ gen: Int) {
        complete(where: { if case .breedingGeneration(let req) = $0.requirement { return gen >= req }; return false })
    }

    /// Call when total breeding count changes
    func onBreedingTotalChanged(_ count: Int) {
        complete(where: { if case .breedingTotal(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when a new dive high score is set
    func onDiveHighScore(_ score: Int) {
        complete(where: { if case .diveHighScore(let req) = $0.requirement { return score >= req }; return false })
    }

    /// Call each day the user logs in with an updated streak
    func onConsecutiveDaysUpdated(_ days: Int) {
        complete(where: { if case .consecutiveDays(let req) = $0.requirement { return days >= req }; return false })
    }

    /// Call when the number of distinct coupling types used changes
    func onCouplingTypesUsed(_ count: Int) {
        complete(where: { if case .couplingTypesUsed(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when an arrangement length milestone is reached (in seconds)
    func onArrangementLength(_ seconds: Int) {
        complete(where: { if case .arrangementLength(let req) = $0.requirement { return seconds >= req }; return false })
    }

    /// Call when seasonal completion count changes
    func onSeasonalComplete(_ count: Int) {
        complete(where: { if case .seasonalComplete(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when lore discovered count changes
    func onLoreDiscovered(_ count: Int) {
        complete(where: { if case .loreDiscovered(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when elder specimen count changes
    func onElderSpecimensChanged(_ count: Int) {
        complete(where: { if case .elderSpecimens(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when total play time crosses an hour threshold
    func onTotalPlayTime(hours: Int) {
        complete(where: { if case .totalPlayTime(let req) = $0.requirement { return hours >= req }; return false })
    }

    /// Call when MIDI export count changes
    func onExportedMIDI(_ count: Int) {
        complete(where: { if case .exportedMIDI(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when trade count changes
    func onTradesCompleted(_ count: Int) {
        complete(where: { if case .tradesCompleted(let req) = $0.requirement { return count >= req }; return false })
    }

    /// Call when a dive completes — checks both dive high-score and depth achievements
    func onDiveDepthReached(_ depth: Int) {
        complete(where: { if case .diveDepth(let req) = $0.requirement { return depth >= req }; return false })
    }

    /// Trigger a named secret condition
    func triggerSecret(id: String) {
        complete(where: { if case .secretCondition(let reqId) = $0.requirement { return reqId == id }; return false })
    }

    /// Dismiss a pending notification (call after UI has displayed it)
    func dismissNotification(_ achievementId: String) {
        pendingNotifications.removeAll { $0.id == achievementId }
    }

    func clearAllNotifications() {
        pendingNotifications = []
    }

    // MARK: - Private

    private func complete(where predicate: (Achievement) -> Bool) {
        var changed = false
        for idx in achievements.indices {
            guard !achievements[idx].isCompleted, predicate(achievements[idx]) else { continue }
            achievements[idx].isCompleted = true
            achievements[idx].completedDate = Date()
            pendingNotifications.append(achievements[idx])
            achievementPoints += achievements[idx].tier.pointValue
            changed = true
            UINotificationFeedbackGenerator().notificationOccurred(.success)
        }
        if changed { save() }
    }

    private func recalculatePoints() {
        achievementPoints = completedAchievements.reduce(0) { $0 + $1.tier.pointValue }
    }

    // MARK: - Persistence

    private func save() {
        if let data = try? JSONEncoder().encode(achievements) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
        UserDefaults.standard.set(achievementPoints, forKey: pointsKey)
    }

    private func load() {
        if let data = UserDefaults.standard.data(forKey: storageKey),
           let saved = try? JSONDecoder().decode([Achievement].self, from: data) {
            achievements = saved
        }
        achievementPoints = UserDefaults.standard.integer(forKey: pointsKey)
    }

    // MARK: - Achievement Library (40 total)

    private func generateAchievements() {

        // MARK: Collection (8)

        let collection: [Achievement] = [
            Achievement(
                id: "first_catch",
                title: "First Catch",
                description: "Catch your first wild specimen from the Reef.",
                category: .collection, tier: .bronze,
                requirement: .specimenCount(count: 1),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Unlocks the Codex", bonusXP: 10),
                isSecret: false
            ),
            Achievement(
                id: "ten_unique",
                title: "Tide Pool Keeper",
                description: "Discover 10 unique specimen subtypes.",
                category: .collection, tier: .bronze,
                requirement: .uniqueSubtypes(count: 10),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "+2 reef slots", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "twenty_five_unique",
                title: "Reef Surveyor",
                description: "Discover 25 unique specimen subtypes.",
                category: .collection, tier: .silver,
                requirement: .uniqueSubtypes(count: 25),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Unlocks advanced filter in Reef view", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "fifty_unique",
                title: "Deep Archivist",
                description: "Discover 50 unique specimen subtypes.",
                category: .collection, tier: .gold,
                requirement: .uniqueSubtypes(count: 50),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Golden reef border in Collection tab", bonusXP: 100),
                isSecret: false
            ),
            Achievement(
                id: "all_common",
                title: "Shell Collector",
                description: "Collect all common-rarity specimen subtypes.",
                category: .collection, tier: .silver,
                requirement: .uniqueSubtypes(count: 16),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Unlocks the Twilight zone biome", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "all_rare",
                title: "Rare Currents",
                description: "Collect at least one specimen of every rare subtype.",
                category: .collection, tier: .gold,
                requirement: .uniqueSubtypes(count: 32),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Rare specimen spawn rate +10%", bonusXP: 100),
                isSecret: false
            ),
            Achievement(
                id: "complete_biome",
                title: "Biome Complete",
                description: "Collect every specimen subtype native to a single biome zone.",
                category: .collection, tier: .silver,
                requirement: .uniqueSubtypes(count: 8),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Biome theme unlocked in PlaySurface", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "full_catalog",
                title: "The Reef Complete",
                description: "Collect every specimen subtype in the full catalog.",
                category: .collection, tier: .mythic,
                requirement: .uniqueSubtypes(count: 24),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Mythic reef border + XOceanus title", bonusXP: 500),
                isSecret: false
            ),
        ]

        // MARK: Breeding (6)

        let breeding: [Achievement] = [
            Achievement(
                id: "first_offspring",
                title: "First Offspring",
                description: "Successfully breed two specimens for the first time.",
                category: .breeding, tier: .bronze,
                requirement: .breedingTotal(count: 1),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Unlocks the Breeding Pool", bonusXP: 10),
                isSecret: false
            ),
            Achievement(
                id: "gen_3",
                title: "Third Generation",
                description: "Breed a specimen that is three generations removed from wild-caught originals.",
                category: .breeding, tier: .silver,
                requirement: .breedingGeneration(gen: 3),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Generation indicator in Specimen Journal", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "gen_5",
                title: "Living Dynasty",
                description: "Breed a specimen that is five generations deep.",
                category: .breeding, tier: .gold,
                requirement: .breedingGeneration(gen: 5),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Dynasty crest visible in Reef view", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "gen_10",
                title: "Ancient Lineage",
                description: "Breed a specimen ten generations deep. Its DNA contains the memory of ten predecessors.",
                category: .breeding, tier: .platinum,
                requirement: .breedingGeneration(gen: 10),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Lineage map unlocked in Specimen Journal", bonusXP: 100),
                isSecret: false
            ),
            Achievement(
                id: "breed_all_types",
                title: "Cross-Pollinator",
                description: "Successfully breed at least one offspring from every combination of specimen category types.",
                category: .breeding, tier: .gold,
                requirement: .breedingTotal(count: 20),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Exotic biome access", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "discover_mutation",
                title: "Ghost Harmonic",
                description: "Breed a specimen with a mutation — a partial that exists in neither parent's spectral DNA.",
                category: .breeding, tier: .platinum,
                requirement: .secretCondition(id: "ghost_harmonic_triggered"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Mutation badge on specimen card", bonusXP: 100),
                isSecret: false
            ),
        ]

        // MARK: Performance (5)

        let performance: [Achievement] = [
            Achievement(
                id: "first_arrangement",
                title: "First Performance",
                description: "Create and play an arrangement of any length.",
                category: .performance, tier: .bronze,
                requirement: .arrangementLength(seconds: 1),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Unlocks arrangement recording", bonusXP: 10),
                isSecret: false
            ),
            Achievement(
                id: "five_min_arrangement",
                title: "Extended Set",
                description: "Play a continuous arrangement lasting at least 5 minutes.",
                category: .performance, tier: .silver,
                requirement: .arrangementLength(seconds: 300),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "+1 PlaySurface voice slot", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "export_midi",
                title: "The Bridge",
                description: "Export an arrangement as a MIDI file.",
                category: .performance, tier: .silver,
                requirement: .exportedMIDI(count: 1),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "MIDI export badge on arrangement", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "dive_score_1000",
                title: "Dive Master",
                description: "Achieve a score of 1,000 or higher in a single Dive session.",
                category: .performance, tier: .gold,
                requirement: .diveHighScore(score: 1000),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Gold aura in Dive lobby", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "perfect_dive_round",
                title: "Perfect Round",
                description: "Complete a Dive round without missing a single catch opportunity.",
                category: .performance, tier: .gold,
                requirement: .secretCondition(id: "perfect_dive_round_completed"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Perfect badge appears in Dive history", bonusXP: 50),
                isSecret: false
            ),
        ]

        // MARK: Exploration (5)

        let exploration: [Achievement] = [
            Achievement(
                id: "all_biomes",
                title: "Zone Mapper",
                description: "Explore all five water column zones: Surface, Twilight, Midnight, Abyss, and Hadal.",
                category: .exploration, tier: .gold,
                requirement: .uniqueSubtypes(count: 5),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Full water column map visible in Dive", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "deep_dive_500",
                title: "Pressure Tested",
                description: "Reach a dive depth of 500 meters.",
                category: .exploration, tier: .silver,
                requirement: .diveDepth(depth: 500),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Midnight zone unlocked in Dive", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "weather_all_types",
                title: "Storm Chaser",
                description: "Experience all available weather conditions in the Reef.",
                category: .exploration, tier: .silver,
                requirement: .seasonalComplete(count: 4),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Weather forecast unlocked in Reef view", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "night_dive",
                title: "Noctilucent",
                description: "Complete a Dive at night — after local midnight and before 4am.",
                category: .exploration, tier: .bronze,
                requirement: .secretCondition(id: "night_dive_completed"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Night filter in Dive visual mode", bonusXP: 10),
                isSecret: false
            ),
            Achievement(
                id: "hidden_zone",
                title: "The Hadal Cartographer",
                description: "Discover the hidden zone that lies beyond the labeled map.",
                category: .exploration, tier: .platinum,
                requirement: .diveDepth(depth: 2000),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Hadal zone unlocked permanently", bonusXP: 100),
                isSecret: false
            ),
        ]

        // MARK: Mastery (5)

        let mastery: [Achievement] = [
            Achievement(
                id: "first_elder",
                title: "Witness to Time",
                description: "Raise your first specimen to Elder status.",
                category: .mastery, tier: .silver,
                requirement: .elderSpecimens(count: 1),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Elder aura in Reef view", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "ten_elders",
                title: "The Patient Reef",
                description: "Raise 10 specimens to Elder status.",
                category: .mastery, tier: .gold,
                requirement: .elderSpecimens(count: 10),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Elder chorus effect in PlaySurface", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "max_level_specimen",
                title: "Peak Form",
                description: "Raise any specimen to its maximum possible level.",
                category: .mastery, tier: .gold,
                requirement: .secretCondition(id: "max_level_reached"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Level cap badge on specimen card", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "all_coupling_types",
                title: "Coupling Scholar",
                description: "Use all available coupling types at least once.",
                category: .mastery, tier: .platinum,
                requirement: .couplingTypesUsed(count: 15),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Full coupling matrix view unlocked", bonusXP: 100),
                isSecret: false
            ),
            Achievement(
                id: "100_hours",
                title: "The Long Game",
                description: "Accumulate 100 hours of total play time.",
                category: .mastery, tier: .platinum,
                requirement: .totalPlayTime(hours: 100),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Century badge on player profile", bonusXP: 100),
                isSecret: false
            ),
        ]

        // MARK: Social (4)

        let social: [Achievement] = [
            Achievement(
                id: "first_trade",
                title: "First Exchange",
                description: "Complete your first specimen trade with another player.",
                category: .social, tier: .bronze,
                requirement: .tradesCompleted(count: 1),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Trade history view unlocked", bonusXP: 10),
                isSecret: false
            ),
            Achievement(
                id: "trade_rare",
                title: "Rare Passage",
                description: "Trade a rare-rarity specimen with another player.",
                category: .social, tier: .silver,
                requirement: .tradesCompleted(count: 5),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Trade rarity filter unlocked", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "trade_gen5",
                title: "Living Heritage",
                description: "Trade a generation-5 or deeper specimen with another player.",
                category: .social, tier: .gold,
                requirement: .tradesCompleted(count: 10),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Provenance badge on traded specimen", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "help_new_player",
                title: "Tide Guide",
                description: "Gift a specimen to a player who has been in the Reef for fewer than 7 days.",
                category: .social, tier: .silver,
                requirement: .secretCondition(id: "new_player_gift_sent"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Tide Guide badge on player profile", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "collab_first",
                title: "Shared Waters",
                description: "Complete your first collaborative dive with another player.",
                category: .social, tier: .silver,
                requirement: .secretCondition(id: "collab_first"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Collaborative dive history unlocked", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "collab_ten",
                title: "Dive Partners",
                description: "Complete 10 collaborative dives.",
                category: .social, tier: .gold,
                requirement: .secretCondition(id: "collab_ten"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Partner aura in Dive lobby", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "collab_cross_coupling",
                title: "Cross-Pollination",
                description: "Discover a cross-player coupling during a collaborative dive.",
                category: .social, tier: .gold,
                requirement: .secretCondition(id: "collab_cross_coupling_first"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Cross-coupling badge visible in Microscope", bonusXP: 50),
                isSecret: false
            ),
        ]

        // MARK: Seasonal (4)

        let seasonal: [Achievement] = [
            Achievement(
                id: "complete_spring",
                title: "Spring Tide",
                description: "Complete all available Spring season events in a single year.",
                category: .seasonal, tier: .silver,
                requirement: .seasonalComplete(count: 1),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Spring palette in Reef view", bonusXP: 25),
                isSecret: false
            ),
            Achievement(
                id: "all_seasons",
                title: "Full Cycle",
                description: "Complete at least one seasonal event in all four seasons.",
                category: .seasonal, tier: .gold,
                requirement: .seasonalComplete(count: 4),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Seasonal cycle animation unlocked", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "catch_seasonal_exclusive",
                title: "Limited Edition",
                description: "Catch a specimen that only appears during a specific seasonal event.",
                category: .seasonal, tier: .gold,
                requirement: .secretCondition(id: "seasonal_exclusive_caught"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Seasonal exclusive badge on specimen card", bonusXP: 50),
                isSecret: false
            ),
            Achievement(
                id: "breed_coral_bloom",
                title: "Bloom Offspring",
                description: "Successfully breed a new specimen during the Coral Bloom event.",
                category: .seasonal, tier: .platinum,
                requirement: .secretCondition(id: "coral_bloom_breed_completed"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Bloom marking on offspring's specimen card", bonusXP: 100),
                isSecret: false
            ),
        ]

        // MARK: Secret (3)

        let secret: [Achievement] = [
            Achievement(
                id: "sec_001",
                title: "The Hadal Chord",
                description: "You found the chord that the ocean plays when it thinks no one is listening. It has seven notes, no octave, and no name in any human scale system. The Reef remembers you now.",
                category: .secret, tier: .mythic,
                requirement: .secretCondition(id: "hadal_chord_triggered"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Hadal Chord available in PlaySurface tuning modes", bonusXP: 500),
                isSecret: true
            ),
            Achievement(
                id: "sec_002",
                title: "The Elder's Silence",
                description: "You kept a specimen alive for over a year without wiring it to anything. In that time it developed timbral complexity no breeding line has ever produced. Some sounds require only patience.",
                category: .secret, tier: .mythic,
                requirement: .secretCondition(id: "year_old_unwired_specimen"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Silence mode unlocked in PlaySurface", bonusXP: 500),
                isSecret: true
            ),
            Achievement(
                id: "sec_003",
                title: "What the Reef Heard First",
                description: "You played the first note of the Song — the frequency the Reef was listening for before you arrived. It recognized you immediately. It has been waiting.",
                category: .secret, tier: .mythic,
                requirement: .secretCondition(id: "first_note_of_song"),
                isCompleted: false, completedDate: nil,
                reward: AchievementReward(description: "Origin Glyph unlocked in Reef decoration", bonusXP: 500),
                isSecret: true
            ),
        ]

        achievements = collection + breeding + performance + exploration + mastery + social + seasonal + secret
        save()
    }
}
