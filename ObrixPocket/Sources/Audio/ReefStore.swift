import Foundation
import Combine

/// Specimen rarity tiers
enum SpecimenRarity: String, Codable, CaseIterable {
    case common, uncommon, rare, legendary
}

/// Specimen category (matches OBRIX DSP modules)
enum SpecimenCategory: String, Codable, CaseIterable {
    case source      // Shells — oscillators
    case processor   // Coral — filters
    case modulator   // Currents — LFOs/envelopes
    case effect      // Tide Pools — delay/chorus/reverb
}

struct ProvenanceEntry: Codable {
    let fromPlayer: String
    let toPlayer: String
    let timestamp: Date
    let location: String?
}

/// A single specimen in the reef
struct Specimen: Identifiable, Codable {
    let id: UUID
    var name: String
    var category: SpecimenCategory
    var rarity: SpecimenRarity
    var health: Int // 0-100
    var isPhantom: Bool
    var phantomScar: Bool
    var subtype: String // e.g., "PolyBLEP", "CytomicSVF", "StandardLFO"
    var catchAccelPattern: [Float] // Movement signature from accelerometer at catch time

    // Trade provenance chain
    var provenance: [ProvenanceEntry]

    // Spectral DNA (64-float profile from catch environment)
    var spectralDNA: [Float]

    // OBRIX parameter state
    var parameterState: [String: Float]

    // Provenance
    var catchLatitude: Double?
    var catchLongitude: Double?
    var catchTimestamp: Date
    var catchWeatherDescription: String?

    // Creature visual
    var creatureGenomeData: Data?

    static let slotCount = 16
}

/// Coupling route between two specimens
struct CouplingRoute: Codable {
    let sourceId: UUID
    let destId: UUID
    var depth: Float // -1.0 to 1.0
}

/// The reef — 16 specimen slots + coupling
final class ReefStore: ObservableObject {
    static let maxSlots = 16

    @Published var specimens: [Specimen?] = Array(repeating: nil, count: maxSlots)
    @Published var couplingRoutes: [CouplingRoute] = []
    @Published var reefName: String = "My Reef"
    @Published var totalDiveDepth: Int = 0

    /// Count of non-nil, non-phantom specimens (for Dive eligibility)
    var diveEligibleCount: Int {
        specimens.compactMap { $0 }.filter { !$0.isPhantom }.count
    }

    /// Add a specimen to the first empty slot
    func addSpecimen(_ specimen: Specimen) -> Int? {
        guard let emptyIndex = specimens.firstIndex(where: { $0 == nil }) else {
            return nil // Reef is full — must release one first
        }
        specimens[emptyIndex] = specimen
        return emptyIndex
    }

    /// Release a specimen (economy sink)
    func releaseSpecimen(at index: Int) {
        guard index >= 0, index < Self.maxSlots else { return }
        // Remove coupling routes involving this specimen
        if let specId = specimens[index]?.id {
            couplingRoutes.removeAll { $0.sourceId == specId || $0.destId == specId }
        }
        specimens[index] = nil
    }
}
