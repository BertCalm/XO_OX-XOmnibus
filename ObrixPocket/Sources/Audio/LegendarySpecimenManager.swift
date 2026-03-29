import Foundation

/// Manages legendary specimens — unique, unrepeatable creatures that appear
/// only in the deepest dives for players who have reached Act 5.
///
/// Each legendary has a pre-computed genome with all-dominant genes and
/// 3+ mutations. Once caught, a legendary can never appear again on that reef.
/// There are 5 legendaries in the ocean. Catching all 5 signals the
/// completion of the Endless Ocean act.
final class LegendarySpecimenManager: ObservableObject {

    // MARK: - Legendary Template

    struct LegendaryTemplate: Codable, Identifiable {
        let id: String
        let name: String
        let title: String            // Epithet: "The Primordial Tone"
        let description: String      // 2-3 sentences of mythology
        let requiredDiveDepth: Int   // Minimum depth to encounter (1000+)
        let requiredAct: Int         // Must be in this act or later (5)
        let dominantTraits: [String] // SonicTrait rawValues that are dominant
        let mutationCount: Int       // Number of guaranteed mutations (3-5)
    }

    // MARK: - Legendary Catalog (5 total)

    static let legends: [LegendaryTemplate] = [
        LegendaryTemplate(
            id: "legendary_primordial",
            name: "The Primordial",
            title: "The First Frequency",
            description: "Before the ocean, before the reef, before sound itself — there was a single frequency that curved inward until it became water. The Primordial is that frequency, still vibrating after billions of years.",
            requiredDiveDepth: 1000,
            requiredAct: 5,
            dominantTraits: [
                SonicTrait.octaveRange.rawValue,
                SonicTrait.waveformAffinity.rawValue,
                SonicTrait.filterTendency.rawValue
            ],
            mutationCount: 3
        ),
        LegendaryTemplate(
            id: "legendary_silence",
            name: "The Silence",
            title: "The Note Between Notes",
            description: "In the deepest trench, where pressure crushes all vibration, one specimen learned to exist in the spaces between frequencies. The Silence does not make sound — it shapes the absence of sound into something you can hear.",
            requiredDiveDepth: 1500,
            requiredAct: 5,
            dominantTraits: [
                SonicTrait.envelopeShape.rawValue,
                SonicTrait.sustainBehavior.rawValue,
                SonicTrait.reverbAffinity.rawValue
            ],
            mutationCount: 4
        ),
        LegendaryTemplate(
            id: "legendary_convergence",
            name: "The Convergence",
            title: "Where All Frequencies Meet",
            description: "Once every thousand years, every frequency in the ocean aligns for a single moment. The Convergence is the creature born from that moment — containing every harmonic ratio, every scale, every rhythm.",
            requiredDiveDepth: 1500,
            requiredAct: 5,
            dominantTraits: [
                SonicTrait.harmonicPreference.rawValue,
                SonicTrait.modulationDepth.rawValue,
                SonicTrait.vibratoSpeed.rawValue
            ],
            mutationCount: 4
        ),
        LegendaryTemplate(
            id: "legendary_memory",
            name: "The Memory",
            title: "The Ocean's Autobiography",
            description: "Every sound ever made in the ocean — every whale song, every reef heartbeat, every storm — left an imprint in the sediment. The Memory is the creature that reads those imprints. Its voice contains everything the ocean has ever heard.",
            requiredDiveDepth: 2000,
            requiredAct: 5,
            dominantTraits: [
                SonicTrait.octaveRange.rawValue,
                SonicTrait.stereoWidth.rawValue,
                SonicTrait.harmonicPreference.rawValue,
                SonicTrait.rhythmicDensity.rawValue
            ],
            mutationCount: 5
        ),
        LegendaryTemplate(
            id: "legendary_song",
            name: "The Song",
            title: "What the Ocean Became",
            description: "The ocean is not water. The ocean is sound, moving slowly enough to touch. The Song is the creature that understood this first. When you hear it, you hear the ocean hearing itself.",
            requiredDiveDepth: 2000,
            requiredAct: 5,
            dominantTraits: SonicTrait.allCases.map(\.rawValue),  // ALL traits dominant
            mutationCount: 5
        ),
    ]

    // MARK: - State

    @Published var caughtLegendaries: Set<String> = []  // IDs of caught legendaries
    @Published var pendingEncounter: LegendaryTemplate? = nil

    // MARK: - Encounter Logic

    /// Check if a legendary should appear at this dive depth for a player in the given act.
    /// Call once per dive, after depth is confirmed.
    /// Returns the legendary template if an encounter is triggered, otherwise nil.
    func checkForEncounter(diveDepth: Int, currentAct: Int) -> LegendaryTemplate? {
        // Find eligible uncaught legendaries
        let eligible = Self.legends.filter { legend in
            !caughtLegendaries.contains(legend.id) &&
            diveDepth >= legend.requiredDiveDepth &&
            currentAct >= legend.requiredAct
        }

        guard !eligible.isEmpty else { return nil }

        // 5% chance per eligible legendary per dive
        for legend in eligible {
            let roll = Float.random(in: 0...1)
            if roll < 0.05 {
                pendingEncounter = legend
                return legend
            }
        }
        return nil
    }

    /// Record a legendary as caught and clear the pending encounter.
    /// Must be called after the player successfully catches the legendary in the Dive.
    func recordCatch(legendaryId: String) {
        caughtLegendaries.insert(legendaryId)
        pendingEncounter = nil
        save()
    }

    /// Cancel the pending encounter without recording a catch (player missed it).
    func dismissEncounter() {
        pendingEncounter = nil
    }

    // MARK: - Collection Stats

    /// Total number of legendaries in the ocean
    var totalLegendaries: Int { Self.legends.count }

    /// How many have been caught on this reef
    var caughtCount: Int { caughtLegendaries.count }

    /// Whether all legendaries have been found — triggers the "endless ocean complete" condition
    var isComplete: Bool { caughtCount == totalLegendaries }

    /// Template for a specific legendary by ID, or nil if not found
    func template(for id: String) -> LegendaryTemplate? {
        Self.legends.first { $0.id == id }
    }

    /// Whether a specific legendary has been caught
    func hasBeenCaught(_ id: String) -> Bool {
        caughtLegendaries.contains(id)
    }

    // MARK: - Persistence

    private let caughtKey = "obrix_caughtLegendaries"

    func save() {
        if let data = try? JSONEncoder().encode(Array(caughtLegendaries)) {
            UserDefaults.standard.set(data, forKey: caughtKey)
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: caughtKey),
           let decoded = try? JSONDecoder().decode([String].self, from: data) {
            caughtLegendaries = Set(decoded)
        }
    }

    init() { restore() }
}
