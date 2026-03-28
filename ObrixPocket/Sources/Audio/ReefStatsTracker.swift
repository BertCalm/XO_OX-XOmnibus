import Foundation

/// Tracks cumulative lifetime stats across the entire app
final class ReefStatsTracker: ObservableObject {
    static let shared = ReefStatsTracker()

    private let prefix = "obrix_stats_"

    /// Increment a stat counter
    func increment(_ stat: StatKey, by amount: Int = 1) {
        let key = prefix + stat.rawValue
        let current = UserDefaults.standard.integer(forKey: key)
        UserDefaults.standard.set(current + amount, forKey: key)
    }

    /// Read a stat
    func value(for stat: StatKey) -> Int {
        UserDefaults.standard.integer(forKey: prefix + stat.rawValue)
    }

    /// All stats as a dictionary for display
    var allStats: [(String, String, String)] { // (label, value, icon)
        [
            ("Notes Played", formatNumber(value(for: .notesPlayed)), "music.note"),
            ("Total Play Time", formatTime(Double(value(for: .playSeconds))), "clock"),
            ("Specimens Caught", "\(value(for: .specimensCaught))", "leaf.fill"),
            ("Music Catches", "\(value(for: .musicCatches))", "music.quarternote.3"),
            ("Wires Created", "\(value(for: .wiresCreated))", "link"),
            ("Wires Deleted", "\(value(for: .wiresDeleted))", "link.badge.plus"),
            ("Dives Completed", "\(value(for: .divesCompleted))", "arrow.down.to.line"),
            ("Deepest Dive", "\(value(for: .deepestDive))m", "arrow.down"),
            ("Highest Dive Score", formatNumber(value(for: .highestDiveScore)), "star.fill"),
            ("Level Ups", "\(value(for: .levelUps))", "arrow.up.circle"),
            ("Evolutions", "\(value(for: .evolutions))", "sparkles"),
            ("Perfect Catches", "\(value(for: .perfectCatches))", "bolt.fill"),
            ("Specimens Released", "\(value(for: .specimensReleased))", "arrow.uturn.backward"),
            ("Presets Saved", "\(value(for: .presetsSaved))", "square.and.arrow.down"),
            ("Loops Recorded", "\(value(for: .loopsRecorded))", "repeat"),
            ("Cards Shared", "\(value(for: .cardsShared))", "square.and.arrow.up"),
        ]
    }

    enum StatKey: String {
        case notesPlayed
        case playSeconds
        case specimensCaught
        case musicCatches
        case wiresCreated
        case wiresDeleted
        case divesCompleted
        case deepestDive
        case highestDiveScore
        case levelUps
        case evolutions
        case perfectCatches
        case specimensReleased
        case presetsSaved
        case loopsRecorded
        case cardsShared
    }

    private func formatNumber(_ n: Int) -> String {
        if n < 1000 { return "\(n)" }
        if n < 1_000_000 { return String(format: "%.1fK", Double(n) / 1000) }
        return String(format: "%.1fM", Double(n) / 1_000_000)
    }

    private func formatTime(_ seconds: Double) -> String {
        if seconds < 60 { return "\(Int(seconds))s" }
        if seconds < 3600 { return "\(Int(seconds / 60))m" }
        if seconds < 86400 { return String(format: "%.1fh", seconds / 3600) }
        return String(format: "%.1fd", seconds / 86400)
    }
}
