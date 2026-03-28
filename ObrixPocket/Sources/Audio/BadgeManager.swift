import Foundation

struct Badge: Identifiable, Codable {
    let id: String
    let name: String
    let icon: String      // SF Symbol
    let description: String
    let tier: BadgeTier
    var earned: Bool
    var earnedDate: Date?

    enum BadgeTier: String, Codable {
        case bronze, silver, gold, diamond
    }
}

final class BadgeManager: ObservableObject {
    static let shared = BadgeManager()

    @Published var badges: [Badge] = []

    private let storageKey = "obrix_badges"

    init() {
        load()
        if badges.isEmpty { generateBadges() }
    }

    private func generateBadges() {
        badges = [
            // Bronze
            Badge(id: "first_note", name: "First Note", icon: "music.note", description: "Play your first note", tier: .bronze, earned: false),
            Badge(id: "first_catch", name: "First Catch", icon: "leaf", description: "Catch your first specimen", tier: .bronze, earned: false),
            Badge(id: "first_wire", name: "First Wire", icon: "link", description: "Create your first wire", tier: .bronze, earned: false),
            Badge(id: "first_dive", name: "First Dive", icon: "arrow.down.to.line", description: "Complete your first dive", tier: .bronze, earned: false),
            Badge(id: "first_song", name: "Song Catcher", icon: "music.quarternote.3", description: "Catch from a song", tier: .bronze, earned: false),

            // Silver
            Badge(id: "100_notes", name: "Melodist", icon: "music.note.list", description: "Play 100 notes", tier: .silver, earned: false),
            Badge(id: "all_categories", name: "Collector", icon: "square.grid.2x2", description: "Catch one of each category", tier: .silver, earned: false),
            Badge(id: "chain_4", name: "Chain Builder", icon: "point.3.connected.trianglepath.dotted", description: "Wire a chain of 4+ specimens", tier: .silver, earned: false),
            Badge(id: "perfect_catch", name: "Perfect", icon: "bolt.fill", description: "Get a perfect catch", tier: .silver, earned: false),
            Badge(id: "week_streak", name: "Dedicated", icon: "flame.fill", description: "7-day streak", tier: .silver, earned: false),

            // Gold
            Badge(id: "1000_notes", name: "Musician", icon: "pianokeys", description: "Play 1,000 notes", tier: .gold, earned: false),
            Badge(id: "full_roster", name: "Reef Warden", icon: "star.fill", description: "Discover all 16 types", tier: .gold, earned: false),
            Badge(id: "level_10", name: "Evolver", icon: "sparkles", description: "Evolve a specimen", tier: .gold, earned: false),
            Badge(id: "deep_dive", name: "Abyssal", icon: "arrow.down", description: "Reach 1000m depth", tier: .gold, earned: false),
            Badge(id: "month_streak", name: "Devoted", icon: "heart.fill", description: "30-day streak", tier: .gold, earned: false),

            // Diamond
            Badge(id: "10000_notes", name: "Virtuoso", icon: "medal.fill", description: "Play 10,000 notes", tier: .diamond, earned: false),
            Badge(id: "all_evolved", name: "Reef Master", icon: "crown.fill", description: "Evolve all 16 specimens", tier: .diamond, earned: false),
            Badge(id: "mastery", name: "Legend", icon: "laurel.leading", description: "Reach Mastery Lv.10", tier: .diamond, earned: false),
            Badge(id: "fusion_5", name: "Alchemist", icon: "arrow.triangle.merge", description: "Fuse 5 specimens", tier: .diamond, earned: false),
            Badge(id: "100_dives", name: "Deep One", icon: "water.waves", description: "Complete 100 dives", tier: .diamond, earned: false),
        ]
        save()
    }

    func award(_ badgeId: String) {
        guard let idx = badges.firstIndex(where: { $0.id == badgeId && !$0.earned }) else { return }
        badges[idx].earned = true
        badges[idx].earnedDate = Date()
        save()
    }

    var earnedCount: Int { badges.filter { $0.earned }.count }

    private func save() {
        if let data = try? JSONEncoder().encode(badges) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }

    private func load() {
        guard let data = UserDefaults.standard.data(forKey: storageKey),
              let saved = try? JSONDecoder().decode([Badge].self, from: data) else { return }
        badges = saved
    }
}
