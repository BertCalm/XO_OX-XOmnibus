import Foundation
import UIKit

struct WeeklyChallenge: Codable, Identifiable {
    let id: UUID
    let title: String
    let description: String
    let type: ChallengeType
    let target: Int
    var bestAttempt: Int
    var completed: Bool
    let weekStart: Date
    let reward: String

    enum ChallengeType: String, Codable {
        case scoreTarget       // Achieve score >= target
        case depthTarget       // Reach depth >= target
        case zoneTarget        // Reach specific zone (target = zone index)
        case interactionTarget // Maintain interaction % >= target
        case consecutiveDives  // Complete N dives in the week
    }

    var progressString: String {
        if completed { return "Completed!" }
        return "\(bestAttempt)/\(target)"
    }
}

final class WeeklyChallengeManager: ObservableObject {
    @Published var challenges: [WeeklyChallenge] = []

    private let storageKey = "obrix_weekly_challenges"

    init() { loadOrGenerate() }

    func loadOrGenerate() {
        if let data = UserDefaults.standard.data(forKey: storageKey),
           let saved = try? JSONDecoder().decode([WeeklyChallenge].self, from: data),
           let first = saved.first,
           isCurrentWeek(first.weekStart) {
            challenges = saved
            return
        }
        generateNewWeek()
    }

    func updateFromDive(score: Int, depth: Int, interactionPercent: Int) {
        for i in challenges.indices {
            guard !challenges[i].completed else { continue }

            switch challenges[i].type {
            case .scoreTarget:
                challenges[i].bestAttempt = max(challenges[i].bestAttempt, score)
                if score >= challenges[i].target { challenges[i].completed = true }
            case .depthTarget:
                challenges[i].bestAttempt = max(challenges[i].bestAttempt, depth)
                if depth >= challenges[i].target { challenges[i].completed = true }
            case .zoneTarget:
                let zoneIndex = depth >= 1000 ? 3 : depth >= 600 ? 2 : depth >= 200 ? 1 : 0
                challenges[i].bestAttempt = max(challenges[i].bestAttempt, zoneIndex)
                if zoneIndex >= challenges[i].target { challenges[i].completed = true }
            case .interactionTarget:
                challenges[i].bestAttempt = max(challenges[i].bestAttempt, interactionPercent)
                if interactionPercent >= challenges[i].target { challenges[i].completed = true }
            case .consecutiveDives:
                challenges[i].bestAttempt += 1
                if challenges[i].bestAttempt >= challenges[i].target { challenges[i].completed = true }
            }

            if challenges[i].completed {
                UINotificationFeedbackGenerator().notificationOccurred(.success)
            }
        }
        save()
    }

    var completedCount: Int { challenges.filter { $0.completed }.count }
    var allComplete: Bool { challenges.allSatisfy { $0.completed } }

    private func generateNewWeek() {
        let weekStart = Calendar.current.startOfDay(for: Date())

        // 3 challenges per week, varying difficulty
        challenges = [
            WeeklyChallenge(id: UUID(), title: "Deep Explorer",
                           description: "Reach the Midnight Zone (600m+)",
                           type: .depthTarget, target: 600,
                           bestAttempt: 0, completed: false,
                           weekStart: weekStart, reward: "+100 XP to all specimens"),
            WeeklyChallenge(id: UUID(), title: "High Scorer",
                           description: "Score 3,000+ in a single dive",
                           type: .scoreTarget, target: 3000,
                           bestAttempt: 0, completed: false,
                           weekStart: weekStart, reward: "+1 extra music catch"),
            WeeklyChallenge(id: UUID(), title: "Active Diver",
                           description: "Complete 5 dives this week",
                           type: .consecutiveDives, target: 5,
                           bestAttempt: 0, completed: false,
                           weekStart: weekStart, reward: "Unlock cosmetic: Bioluminescent"),
        ]
        save()
    }

    private func isCurrentWeek(_ date: Date) -> Bool {
        Calendar.current.isDate(date, equalTo: Date(), toGranularity: .weekOfYear)
    }

    private func save() {
        if let data = try? JSONEncoder().encode(challenges) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }
}
