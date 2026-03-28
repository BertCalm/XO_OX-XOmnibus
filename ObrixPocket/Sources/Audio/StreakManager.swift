import Foundation
import UIKit

final class StreakManager: ObservableObject {
    @Published var currentStreak: Int = 0
    @Published var longestStreak: Int = 0
    @Published var todayRewardClaimed: Bool = false

    private let streakKey = "obrix_current_streak"
    private let longestKey = "obrix_longest_streak"
    private let lastClaimKey = "obrix_last_streak_claim"

    init() {
        currentStreak = UserDefaults.standard.integer(forKey: streakKey)
        longestStreak = UserDefaults.standard.integer(forKey: longestKey)
        checkStreak()
    }

    /// Check and update streak on app open
    func checkStreak() {
        let lastClaim = UserDefaults.standard.object(forKey: lastClaimKey) as? Date

        if let last = lastClaim {
            if Calendar.current.isDateInToday(last) {
                todayRewardClaimed = true
                return
            }
            if Calendar.current.isDateInYesterday(last) {
                // Consecutive day — streak continues
                todayRewardClaimed = false
            } else {
                // Streak broken — reset
                currentStreak = 0
                UserDefaults.standard.set(0, forKey: streakKey)
                todayRewardClaimed = false
            }
        } else {
            todayRewardClaimed = false
        }
    }

    /// Claim today's streak reward
    func claimReward() -> StreakReward {
        guard !todayRewardClaimed else { return .none }

        currentStreak += 1
        todayRewardClaimed = true

        if currentStreak > longestStreak {
            longestStreak = currentStreak
            UserDefaults.standard.set(longestStreak, forKey: longestKey)
        }

        UserDefaults.standard.set(currentStreak, forKey: streakKey)
        UserDefaults.standard.set(Date(), forKey: lastClaimKey)

        UINotificationFeedbackGenerator().notificationOccurred(.success)

        return reward(for: currentStreak)
    }

    func reward(for day: Int) -> StreakReward {
        switch day {
        case 1:  return .xp(10)
        case 2:  return .xp(15)
        case 3:  return .xp(25)
        case 4:  return .xp(30)
        case 5:  return .xp(40)
        case 6:  return .xp(50)
        case 7:  return .milestone("Week Warrior", bonus: 100)
        case 14: return .milestone("Fortnight", bonus: 200)
        case 30: return .milestone("Month Master", bonus: 500)
        case 60: return .milestone("Reef Elder", bonus: 1000)
        case 100: return .milestone("Centurion", bonus: 2000)
        default:
            if day % 7 == 0 { return .xp(75) }  // Weekly bonus
            return .xp(10 + min(day, 30))        // Escalating daily
        }
    }

    /// Next milestone day
    var nextMilestone: Int {
        let milestones = [7, 14, 30, 60, 100]
        return milestones.first { $0 > currentStreak } ?? (currentStreak + 1)
    }
}

enum StreakReward {
    case none
    case xp(Int)
    case milestone(String, bonus: Int)

    var xpAmount: Int {
        switch self {
        case .none: return 0
        case .xp(let amount): return amount
        case .milestone(_, let bonus): return bonus
        }
    }

    var description: String {
        switch self {
        case .none: return ""
        case .xp(let amount): return "+\(amount) XP"
        case .milestone(let name, let bonus): return "\(name)! +\(bonus) XP"
        }
    }
}
