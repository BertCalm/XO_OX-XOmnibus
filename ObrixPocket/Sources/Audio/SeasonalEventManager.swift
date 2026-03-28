import Foundation

struct SeasonalEvent: Codable {
    let id: String
    let name: String
    let description: String
    let startDate: Date
    let endDate: Date
    let boostedSubtypes: [String]   // Subtypes with increased spawn rate during event
    let bonusCosmeticTier: String?  // Special cosmetic tier available during event
    let bonusXPMultiplier: Float    // XP bonus during event

    var isActive: Bool {
        let now = Date()
        return now >= startDate && now <= endDate
    }

    var daysRemaining: Int {
        max(0, Calendar.current.dateComponents([.day], from: Date(), to: endDate).day ?? 0)
    }
}

final class SeasonalEventManager: ObservableObject {
    @Published var currentEvent: SeasonalEvent?

    init() {
        currentEvent = Self.activeEvent()
    }

    /// Check for currently active seasonal event
    static func activeEvent() -> SeasonalEvent? {
        let now = Date()
        let calendar = Calendar.current
        let month = calendar.component(.month, from: now)
        let year = calendar.component(.year, from: now)

        // Define events by month
        switch month {
        case 1: // January — Deep Freeze
            return SeasonalEvent(
                id: "deep_freeze_\(year)",
                name: "Deep Freeze",
                description: "Ice-cold specimens from the Arctic depths",
                startDate: calendar.date(from: DateComponents(year: year, month: 1, day: 1))!,
                endDate: calendar.date(from: DateComponents(year: year, month: 1, day: 31))!,
                boostedSubtypes: ["svf-lp", "reverb-hall", "lfo-sine"],
                bonusCosmeticTier: "bioluminescent",
                bonusXPMultiplier: 1.25
            )
        case 3: // March — Spring Tide
            return SeasonalEvent(
                id: "spring_tide_\(year)",
                name: "Spring Tide",
                description: "The reef awakens with new life",
                startDate: calendar.date(from: DateComponents(year: year, month: 3, day: 1))!,
                endDate: calendar.date(from: DateComponents(year: year, month: 3, day: 31))!,
                boostedSubtypes: ["polyblep-saw", "adsr-fast", "chorus-lush"],
                bonusCosmeticTier: nil,
                bonusXPMultiplier: 1.5
            )
        case 6: // June — Solstice Bloom
            return SeasonalEvent(
                id: "solstice_bloom_\(year)",
                name: "Solstice Bloom",
                description: "Longest day brings the rarest specimens",
                startDate: calendar.date(from: DateComponents(year: year, month: 6, day: 15))!,
                endDate: calendar.date(from: DateComponents(year: year, month: 6, day: 30))!,
                boostedSubtypes: ["fm-basic", "shaper-hard", "lfo-random"],
                bonusCosmeticTier: "prismatic",
                bonusXPMultiplier: 1.0
            )
        case 10: // October — Abyssal Night
            return SeasonalEvent(
                id: "abyssal_night_\(year)",
                name: "Abyssal Night",
                description: "The deep rises. Phantom specimens stir.",
                startDate: calendar.date(from: DateComponents(year: year, month: 10, day: 15))!,
                endDate: calendar.date(from: DateComponents(year: year, month: 10, day: 31))!,
                boostedSubtypes: ["feedback", "delay-stereo", "noise-white"],
                bonusCosmeticTier: "phantom",
                bonusXPMultiplier: 1.0
            )
        case 12: // December — Reef Glow
            return SeasonalEvent(
                id: "reef_glow_\(year)",
                name: "Reef Glow",
                description: "Bioluminescence lights the winter reef",
                startDate: calendar.date(from: DateComponents(year: year, month: 12, day: 1))!,
                endDate: calendar.date(from: DateComponents(year: year, month: 12, day: 31))!,
                boostedSubtypes: ["polyblep-square", "svf-bp", "dist-warm"],
                bonusCosmeticTier: "bioluminescent",
                bonusXPMultiplier: 1.25
            )
        default:
            return nil
        }
    }
}
