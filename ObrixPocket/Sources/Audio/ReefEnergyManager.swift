import Foundation
import UIKit

/// Manages Reef Energy — the primary currency earned through play.
///
/// Economy overview:
/// - Earn 1 energy per note played, capped at 50/day.
/// - Energy cap is 100 total (banked across days).
/// - Spending gates premium actions: extra music catch, XP boosts, spawn rerolls.
final class ReefEnergyManager: ObservableObject {
    /// Shared singleton — use for spending calls in views that don't own the energy state.
    static let shared = ReefEnergyManager()

    @Published var currentEnergy: Int = 0
    @Published var dailyEnergyEarned: Int = 0

    private let energyKey = "obrix_reef_energy"
    private let dailyEarnedKey = "obrix_daily_energy_earned"
    private let lastEnergyDateKey = "obrix_last_energy_date"

    static let maxEnergy = 100        // Banked energy cap
    static let dailyEarnCap = 50      // Max earnable per day
    static let xpCost = 10            // Energy cost to boost a specimen (+50 XP)
    static let musicCatchCost = 20    // Energy cost for an extra music catch
    static let rerollCost = 5         // Energy cost to reroll spawn pool

    init() {
        currentEnergy = UserDefaults.standard.integer(forKey: energyKey)
        dailyEnergyEarned = UserDefaults.standard.integer(forKey: dailyEarnedKey)
        resetDailyIfNeeded()
    }

    // MARK: - Earning

    /// Earn energy from playing (called per note).
    /// Silently ignored if the daily earn cap is already reached.
    func earnFromPlay(amount: Int = 1) {
        guard dailyEnergyEarned < Self.dailyEarnCap else { return }
        let earned = min(amount, Self.dailyEarnCap - dailyEnergyEarned)
        currentEnergy = min(Self.maxEnergy, currentEnergy + earned)
        dailyEnergyEarned += earned
        save()
    }

    // MARK: - Spending

    /// Spend energy for a premium action.
    /// Returns `true` on success, `false` if insufficient energy.
    @discardableResult
    func spend(_ amount: Int) -> Bool {
        guard currentEnergy >= amount else { return false }
        currentEnergy -= amount
        save()
        UIImpactFeedbackGenerator(style: .light).impactOccurred()
        return true
    }

    /// Returns `true` if the player has at least `amount` energy banked.
    func canAfford(_ amount: Int) -> Bool {
        currentEnergy >= amount
    }

    // MARK: - Convenience affordability checks

    var canAffordXPBoost: Bool { canAfford(Self.xpCost) }
    var canAffordMusicCatch: Bool { canAfford(Self.musicCatchCost) }
    var canAffordReroll: Bool { canAfford(Self.rerollCost) }

    // MARK: - Daily Reset

    /// Resets the daily earned counter when a new calendar day begins.
    /// Call once on init and on app foreground.
    func resetDailyIfNeeded() {
        guard let lastDate = UserDefaults.standard.object(forKey: lastEnergyDateKey) as? Date else {
            // First run — stamp today and return; no reset needed.
            UserDefaults.standard.set(Date(), forKey: lastEnergyDateKey)
            return
        }
        if !Calendar.current.isDateInToday(lastDate) {
            dailyEnergyEarned = 0
            UserDefaults.standard.set(Date(), forKey: lastEnergyDateKey)
            save()
        }
    }

    // MARK: - Persistence

    private func save() {
        UserDefaults.standard.set(currentEnergy, forKey: energyKey)
        UserDefaults.standard.set(dailyEnergyEarned, forKey: dailyEarnedKey)
    }
}
