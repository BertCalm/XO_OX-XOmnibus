import Foundation
import UIKit

/// Tracks mastery progression — the endgame after all specimens are leveled
final class MasteryManager: ObservableObject {
    @Published var masteryLevel: Int = 0
    @Published var masteryXP: Int = 0
    @Published var totalSpecimensMaxed: Int = 0
    @Published var isPrestigeUnlocked: Bool = false

    private let masteryKey = "obrix_mastery_level"
    private let masteryXPKey = "obrix_mastery_xp"
    private let prestigeKey = "obrix_prestige_unlocked"

    static let xpPerMasteryLevel = 5000

    /// Singleton — used by DiveTab and CatchTab to award XP without environment injection.
    static let shared = MasteryManager()

    init() {
        masteryLevel = UserDefaults.standard.integer(forKey: masteryKey)
        masteryXP = UserDefaults.standard.integer(forKey: masteryXPKey)
        isPrestigeUnlocked = UserDefaults.standard.bool(forKey: prestigeKey)
    }

    /// Check mastery state from reef
    func updateFromReef(reefStore: ReefStore) {
        let maxed = reefStore.specimens.compactMap { $0 }.filter { $0.level >= 10 }.count
        totalSpecimensMaxed = maxed

        // Prestige unlocks when all 16 reef slots have Lv.10 specimens
        if maxed >= 16 && !isPrestigeUnlocked {
            isPrestigeUnlocked = true
            UserDefaults.standard.set(true, forKey: prestigeKey)
            UINotificationFeedbackGenerator().notificationOccurred(.success)
        }
    }

    /// Award mastery XP (from dives, perfect catches, etc.)
    func awardMasteryXP(_ amount: Int) {
        guard isPrestigeUnlocked else { return }
        masteryXP += amount

        let newLevel = masteryXP / Self.xpPerMasteryLevel
        if newLevel > masteryLevel {
            masteryLevel = newLevel
            UINotificationFeedbackGenerator().notificationOccurred(.success)
        }

        save()
    }

    var progressToNextLevel: Float {
        let xpInLevel = masteryXP % Self.xpPerMasteryLevel
        return Float(xpInLevel) / Float(Self.xpPerMasteryLevel)
    }

    var masteryTitle: String {
        switch masteryLevel {
        case 0: return "Reef Warden"
        case 1...2: return "Reef Master"
        case 3...5: return "Reef Sage"
        case 6...9: return "Reef Elder"
        case 10...19: return "Reef Oracle"
        case 20...49: return "Reef Titan"
        default: return "Reef Legend"
        }
    }

    private func save() {
        UserDefaults.standard.set(masteryLevel, forKey: masteryKey)
        UserDefaults.standard.set(masteryXP, forKey: masteryXPKey)
    }
}
