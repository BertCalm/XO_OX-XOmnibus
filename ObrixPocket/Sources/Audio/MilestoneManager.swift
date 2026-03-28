import Foundation
import UIKit

struct Milestone: Codable, Identifiable {
    let id: String
    let title: String
    let description: String
    let icon: String           // SF Symbol name
    let requirement: Int       // Target value
    var progress: Int          // Current value
    var unlocked: Bool
    let reward: String         // Description of what unlocks

    var progressFraction: Float { Float(min(progress, requirement)) / Float(requirement) }
}

final class MilestoneManager: ObservableObject {
    @Published var milestones: [Milestone] = []

    private let storageKey = "obrix_milestones"

    init() {
        load()
        if milestones.isEmpty { generateDefaults() }
    }

    private func generateDefaults() {
        milestones = [
            // Collection
            Milestone(id: "first_catch", title: "First Catch", description: "Catch your first wild specimen", icon: "star.fill", requirement: 1, progress: 0, unlocked: false, reward: "+25 XP to starter"),
            Milestone(id: "full_roster", title: "Full Roster", description: "Collect all 16 core specimen types", icon: "square.grid.4x3.fill", requirement: 16, progress: 0, unlocked: false, reward: "Unlock Deep Specimens"),
            Milestone(id: "catch_25", title: "Reef Builder", description: "Catch 25 total specimens", icon: "leaf.fill", requirement: 25, progress: 0, unlocked: false, reward: "+2 music catches per day"),

            // Wiring
            Milestone(id: "first_wire", title: "First Connection", description: "Create your first wire", icon: "link", requirement: 1, progress: 0, unlocked: false, reward: "Wire context labels"),
            Milestone(id: "chain_4", title: "Full Chain", description: "Wire a chain of 4+ specimens", icon: "point.3.connected.trianglepath.dotted", requirement: 4, progress: 0, unlocked: false, reward: "Unlock preset saving"),
            Milestone(id: "high_affinity", title: "Natural Pair", description: "Wire a high-affinity pair", icon: "heart.fill", requirement: 1, progress: 0, unlocked: false, reward: "+50% drift for that pair"),

            // Playing
            Milestone(id: "play_100", title: "First Notes", description: "Play 100 notes", icon: "music.note", requirement: 100, progress: 0, unlocked: false, reward: "Unlock scale modes"),
            Milestone(id: "play_1000", title: "Musician", description: "Play 1,000 notes", icon: "music.note.list", requirement: 1000, progress: 0, unlocked: false, reward: "Unlock recording"),
            Milestone(id: "play_10000", title: "Virtuoso", description: "Play 10,000 notes", icon: "medal.fill", requirement: 10000, progress: 0, unlocked: false, reward: "Unlock gesture control"),

            // Leveling
            Milestone(id: "level_5", title: "Growing", description: "Level any specimen to 5", icon: "arrow.up.circle.fill", requirement: 5, progress: 0, unlocked: false, reward: "Specimen glows gold"),
            Milestone(id: "level_10", title: "Evolution", description: "Evolve a specimen to level 10", icon: "sparkles", requirement: 10, progress: 0, unlocked: false, reward: "Evolution ceremony"),
            Milestone(id: "all_level_5", title: "Reef Master", description: "Level all 16 reef specimens to 5+", icon: "crown.fill", requirement: 16, progress: 0, unlocked: false, reward: "Master reef badge"),

            // Music
            Milestone(id: "music_catch_1", title: "Song of the Day", description: "Catch a specimen from a song", icon: "music.quarternote.3", requirement: 1, progress: 0, unlocked: false, reward: "Music hash animation"),
            Milestone(id: "music_catch_10", title: "DJ", description: "Catch 10 specimens from songs", icon: "opticaldisc.fill", requirement: 10, progress: 0, unlocked: false, reward: "Source track on reef"),
            Milestone(id: "perfect_catch", title: "Perfect", description: "Get a perfect catch (rarity upgrade)", icon: "bolt.fill", requirement: 1, progress: 0, unlocked: false, reward: "Bonus round glow"),
        ]
        save()
    }

    func increment(_ milestoneId: String, by amount: Int = 1) {
        guard let idx = milestones.firstIndex(where: { $0.id == milestoneId && !$0.unlocked }) else { return }
        milestones[idx].progress = min(milestones[idx].requirement, milestones[idx].progress + amount)
        if milestones[idx].progress >= milestones[idx].requirement {
            milestones[idx].unlocked = true
            UINotificationFeedbackGenerator().notificationOccurred(.success)
        }
        save()
    }

    func setProgress(_ milestoneId: String, to value: Int) {
        guard let idx = milestones.firstIndex(where: { $0.id == milestoneId && !$0.unlocked }) else { return }
        milestones[idx].progress = min(milestones[idx].requirement, value)
        if milestones[idx].progress >= milestones[idx].requirement {
            milestones[idx].unlocked = true
            UINotificationFeedbackGenerator().notificationOccurred(.success)
        }
        save()
    }

    var unlockedCount: Int { milestones.filter { $0.unlocked }.count }
    var totalCount: Int { milestones.count }

    private func save() {
        if let data = try? JSONEncoder().encode(milestones) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }

    private func load() {
        guard let data = UserDefaults.standard.data(forKey: storageKey),
              let saved = try? JSONDecoder().decode([Milestone].self, from: data) else { return }
        milestones = saved
    }
}
