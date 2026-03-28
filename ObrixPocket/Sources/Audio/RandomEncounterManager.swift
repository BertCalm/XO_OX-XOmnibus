import Foundation
import UIKit

struct RandomEncounter: Identifiable {
    let id = UUID()
    let type: EncounterType
    let title: String
    let description: String
    let reward: String

    enum EncounterType {
        case rareSpawn         // A rare specimen appears nearby
        case energySurge       // Double energy earning for 30 minutes
        case xpStorm           // 3x XP for 10 minutes
        case ghostVisitor      // A "ghost" specimen visits the reef temporarily
        case goldenHour        // Special golden hour — all catches are Uncommon+
    }
}

final class RandomEncounterManager: ObservableObject {
    @Published var activeEncounter: RandomEncounter?
    @Published var encounterExpiry: Date?

    private let lastEncounterKey = "obrix_last_encounter"

    /// Check if a random encounter should trigger (call on app open)
    func checkForEncounter() {
        // Max one encounter per day
        if let last = UserDefaults.standard.object(forKey: lastEncounterKey) as? Date,
           Calendar.current.isDateInToday(last) {
            return
        }

        // 20% chance per app open
        guard Float.random(in: 0...1) < 0.2 else { return }

        let encounters: [RandomEncounter] = [
            RandomEncounter(type: .energySurge, title: "Energy Surge!",
                            description: "Double energy earning for 30 minutes",
                            reward: "2x energy"),
            RandomEncounter(type: .xpStorm, title: "XP Storm!",
                            description: "Triple XP for the next 10 minutes",
                            reward: "3x XP"),
            RandomEncounter(type: .goldenHour, title: "Golden Hour!",
                            description: "All catches are Uncommon or better for 1 hour",
                            reward: "Uncommon+ catches"),
            RandomEncounter(type: .rareSpawn, title: "Rare Sighting!",
                            description: "A rare specimen has appeared nearby",
                            reward: "Guaranteed Rare spawn"),
            RandomEncounter(type: .ghostVisitor, title: "Ghost Visitor",
                            description: "A phantom specimen visits your reef briefly",
                            reward: "Temporary phantom specimen"),
        ]

        activeEncounter = encounters.randomElement()
        encounterExpiry = Date().addingTimeInterval(activeEncounter?.type == .xpStorm ? 600 : 1800) // 10 or 30 min

        UserDefaults.standard.set(Date(), forKey: lastEncounterKey)
        UINotificationFeedbackGenerator().notificationOccurred(.success)
    }

    /// Check if the active encounter has expired
    func checkExpiry() {
        if let expiry = encounterExpiry, Date() > expiry {
            activeEncounter = nil
            encounterExpiry = nil
        }
    }

    /// XP multiplier from active encounter
    var xpMultiplier: Float {
        guard let enc = activeEncounter, enc.type == .xpStorm else { return 1.0 }
        return 3.0
    }

    /// Energy multiplier from active encounter
    var energyMultiplier: Int {
        guard let enc = activeEncounter, enc.type == .energySurge else { return 1 }
        return 2
    }

    /// Minimum rarity from active encounter
    var minimumRarity: SpecimenRarity? {
        guard let enc = activeEncounter, enc.type == .goldenHour else { return nil }
        return .uncommon
    }
}
