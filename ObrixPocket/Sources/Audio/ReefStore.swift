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

/// Cosmetic tier — orthogonal to rarity (visual axis, not stat axis)
enum CosmeticTier: String, Codable, CaseIterable {
    case standard       // 85% — base art
    case bioluminescent // 10% — glowing edges, pulsing light (night spawns)
    case phantom        // 3%  — inverted palette, translucent, spectral trail
    case fossilized     // 1.5% — stone texture, ancient markings (music hash ONLY)
    case prismatic      // 0.5% — rainbow chromatic aberration (coupling discovery ONLY)
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
    var subtype: String // e.g., "polyblep-saw", "svf-lp" (catalog subtype ID)
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

    // Monster Rancher DNA system (A+B milestone)
    var cosmeticTier: CosmeticTier
    var morphIndex: Int          // 0 = Reef morph, 1 = Drift morph
    var musicHash: String?       // SHA256 hex if born from music library
    var sourceTrackTitle: String? // "Artist — Title" of the song that birthed it

    // Leveling system
    var xp: Int                  // Experience points earned
    var level: Int               // Current level (1-10)
    var aggressiveScore: Float   // Cumulative aggressive play (high velocity, fast notes)
    var gentleScore: Float       // Cumulative gentle play (low velocity, sustained notes)
    var totalPlaySeconds: Double // Total seconds of play through this specimen

    static let slotCount = 16

    /// The catalog creature name for this specimen's subtype
    var creatureName: String {
        SpecimenCatalog.entry(for: subtype)?.creatureName ?? subtype
    }
}

/// Coupling route between two specimens
struct CouplingRoute: Codable {
    let sourceId: UUID
    let destId: UUID
    var depth: Float // -1.0 to 1.0
    var connectedSince: Date? // When the wire was created — used for spectral drift on disconnect
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

    // MARK: - Spectral Drift on Disconnect

    /// Apply spectral DNA drift to two specimens that were wired together.
    /// Drift magnitude is proportional to connection duration — small but cumulative.
    /// Each of the 64 spectral DNA bands shifts toward the other specimen's value.
    /// Cap: min(0.05, hours * 0.002) per band per disconnect event.
    func applySpectralDrift(sourceId: UUID, destId: UUID, connectedSince: Date?) {
        guard let since = connectedSince else { return }

        let durationHours = Date().timeIntervalSince(since) / 3600.0
        guard durationHours > 0 else { return }

        let driftAmount = Float(min(0.05, durationHours * 0.002))

        guard let srcIdx = specimens.firstIndex(where: { $0?.id == sourceId }),
              let dstIdx = specimens.firstIndex(where: { $0?.id == destId }),
              var srcSpec = specimens[srcIdx],
              var dstSpec = specimens[dstIdx] else { return }

        let bandCount = min(srcSpec.spectralDNA.count, dstSpec.spectralDNA.count)
        guard bandCount > 0 else { return }

        for i in 0..<bandCount {
            let srcVal = srcSpec.spectralDNA[i]
            let dstVal = dstSpec.spectralDNA[i]
            // Shift each toward the other by driftAmount
            srcSpec.spectralDNA[i] = srcVal + (dstVal - srcVal) * driftAmount
            dstSpec.spectralDNA[i] = dstVal + (srcVal - dstVal) * driftAmount
        }

        specimens[srcIdx] = srcSpec
        specimens[dstIdx] = dstSpec
    }

}
// MARK: - Persistence implemented in ReefPersistence.swift (GRDB-backed)

