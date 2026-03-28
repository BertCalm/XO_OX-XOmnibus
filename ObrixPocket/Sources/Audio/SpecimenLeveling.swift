import Foundation

/// Leveling system constants and helpers for specimen progression (1–10).
enum SpecimenLeveling {

    /// XP thresholds per level — how much total XP is required to reach each level.
    static let xpForLevel: [Int: Int] = [
        1: 0,    2: 50,   3: 120,  4: 220,  5: 350,
        6: 520,  7: 730,  8: 1000, 9: 1350, 10: 1800
    ]

    /// Derive the current level from total accumulated XP.
    /// Returns the highest level whose threshold has been met, capped at 10.
    static func checkLevelUp(xp: Int) -> Int {
        var level = 1
        for (lvl, threshold) in xpForLevel.sorted(by: { $0.key < $1.key }) {
            if xp >= threshold { level = lvl }
        }
        return min(level, 10)
    }

    /// Parameter range multiplier for a given level — wider at 5 and 10.
    /// Used by the engine to allow evolved specimens to push parameters further.
    static func paramRangeMultiplier(level: Int) -> Float {
        switch level {
        case 1...4: return 1.0
        case 5...6: return 1.2  // 20% wider
        case 7...9: return 1.4  // 40% wider
        case 10:    return 1.6  // 60% wider (evolved form)
        default:    return 1.0
        }
    }

    /// Human-readable XP progress string for display (e.g. "350/520 XP").
    static func xpProgressString(xp: Int, level: Int) -> String {
        let nextLevel = min(level + 1, 10)
        guard let nextThreshold = xpForLevel[nextLevel] else { return "\(xp) XP" }
        if level >= 10 { return "MAX" }
        return "\(xp)/\(nextThreshold) XP"
    }

    /// XP progress as a 0–1 fraction for progress bar rendering.
    static func xpFraction(xp: Int, level: Int) -> Double {
        guard level < 10 else { return 1.0 }
        let currentThreshold = xpForLevel[level] ?? 0
        let nextThreshold = xpForLevel[level + 1] ?? currentThreshold + 1
        let span = nextThreshold - currentThreshold
        guard span > 0 else { return 0 }
        return Double(xp - currentThreshold) / Double(span)
    }
}
