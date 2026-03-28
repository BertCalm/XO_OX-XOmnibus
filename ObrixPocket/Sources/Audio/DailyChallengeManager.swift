import Foundation
import UIKit

struct DailyChallenge: Codable, Identifiable {
    let id: UUID
    let description: String
    let type: ChallengeType
    let target: Int  // Target count/value
    var progress: Int
    let reward: ChallengeReward
    let date: Date  // The day this challenge is for

    var isComplete: Bool { progress >= target }

    enum ChallengeType: String, Codable {
        case catchCount        // Catch N specimens
        case playNotes         // Play N notes
        case wireSpecimens     // Create N wires
        case sustainedPlay     // Hold notes for N total seconds
        case perfectCatch      // Get N perfect catches
    }

    enum ChallengeReward: String, Codable {
        case bonusXP           // +50 XP to all reef specimens
        case rarityBoost       // Next catch is guaranteed Uncommon+
        case extraMusicCatch   // Extra music catch today
    }
}

final class DailyChallengeManager: ObservableObject {
    @Published var challenges: [DailyChallenge] = []

    private let storageKey = "obrix_daily_challenges"

    init() {
        loadOrGenerate()
    }

    /// Generate 3 random daily challenges if today's don't exist yet.
    func loadOrGenerate() {
        if let data = UserDefaults.standard.data(forKey: storageKey),
           let saved = try? JSONDecoder().decode([DailyChallenge].self, from: data),
           let first = saved.first,
           Calendar.current.isDateInToday(first.date) {
            challenges = saved
            return
        }

        // Generate fresh challenges for today
        challenges = [
            generateChallenge(.catchCount, desc: "Catch 2 specimens", target: 2, reward: .bonusXP),
            generateChallenge(.playNotes, desc: "Play 50 notes", target: 50, reward: .extraMusicCatch),
            generateChallenge(.wireSpecimens, desc: "Create 3 wires", target: 3, reward: .rarityBoost),
        ]
        save()
    }

    func incrementProgress(type: DailyChallenge.ChallengeType, by amount: Int = 1) {
        for i in challenges.indices {
            if challenges[i].type == type && !challenges[i].isComplete {
                challenges[i].progress = min(challenges[i].target, challenges[i].progress + amount)
                if challenges[i].isComplete {
                    UINotificationFeedbackGenerator().notificationOccurred(.success)
                }
            }
        }
        save()
    }

    var allComplete: Bool { challenges.allSatisfy { $0.isComplete } }
    var completedCount: Int { challenges.filter { $0.isComplete }.count }

    private func generateChallenge(
        _ type: DailyChallenge.ChallengeType,
        desc: String,
        target: Int,
        reward: DailyChallenge.ChallengeReward
    ) -> DailyChallenge {
        DailyChallenge(id: UUID(), description: desc, type: type, target: target, progress: 0, reward: reward, date: Date())
    }

    private func save() {
        if let data = try? JSONEncoder().encode(challenges) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }
}
