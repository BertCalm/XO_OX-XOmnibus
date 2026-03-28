import Foundation
import Combine
import GRDB

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

    // Append-only biography — the soul of the specimen
    var journal: [JournalEntry]

    // Favorite / pin status — marked by the player for quick access
    var isFavorite: Bool

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

    /// Tracks which slot was most recently filled (for "NEW!" badge in ReefScene).
    var lastAddedSlot: Int? = nil
    var lastAddedTime: Date? = nil

    /// Monotonic counter incremented at the start of every load() call.
    /// The async completion block checks this before writing to guard against
    /// double-call races where an older load() resolves after a newer one.
    var loadGeneration: Int = 0

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
        lastAddedSlot = emptyIndex
        lastAddedTime = Date()
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

        guard let srcIdx = specimens.firstIndex(where: { $0?.id == sourceId }),
              let dstIdx = specimens.firstIndex(where: { $0?.id == destId }),
              var srcSpec = specimens[srcIdx],
              var dstSpec = specimens[dstIdx] else { return }

        let affinity = CouplingAffinity.between(srcSpec, dstSpec)
        let baseDrift = Float(min(0.05, durationHours * 0.002))
        let driftAmount = baseDrift * affinity.driftMultiplier

        let bandCount = min(srcSpec.spectralDNA.count, dstSpec.spectralDNA.count)
        guard bandCount > 0 else { return }

        for i in 0..<bandCount {
            let srcVal = srcSpec.spectralDNA[i]
            let dstVal = dstSpec.spectralDNA[i]
            // Shift each toward the other by driftAmount
            srcSpec.spectralDNA[i] = srcVal + (dstVal - srcVal) * driftAmount
            dstSpec.spectralDNA[i] = dstVal + (srcVal - dstVal) * driftAmount
        }

        // Journal: spectral drift event for both specimens
        let driftPct = Int((driftAmount * 100).rounded())
        let driftDesc = "Spectral drift \(driftPct)% after \(String(format: "%.1f", durationHours))h coupling"
        let srcDriftEntry = JournalEntry(id: UUID(), timestamp: Date(), type: .drifted, description: driftDesc)
        let dstDriftEntry = JournalEntry(id: UUID(), timestamp: Date(), type: .drifted, description: driftDesc)
        srcSpec.journal.append(srcDriftEntry)
        dstSpec.journal.append(dstDriftEntry)

        specimens[srcIdx] = srcSpec
        specimens[dstIdx] = dstSpec
    }

    // MARK: - Stasis Management

    /// Move a specimen from reef to stasis (preserve it without active slot)
    func moveToStasis(at slotIndex: Int) {
        guard slotIndex >= 0, slotIndex < Self.maxSlots,
              let specimen = specimens[slotIndex] else { return }
        // Remove coupling routes involving this specimen
        couplingRoutes.removeAll { $0.sourceId == specimen.id || $0.destId == specimen.id }
        // Save to stasis in DB
        saveSpecimenToStasis(specimen)
        // Clear reef slot
        specimens[slotIndex] = nil
    }

    /// Move a specimen from stasis to a specific reef slot
    func moveFromStasis(specimenId: UUID, toSlot slotIndex: Int) {
        guard slotIndex >= 0, slotIndex < Self.maxSlots,
              specimens[slotIndex] == nil else { return }
        // Load from stasis
        guard let specimen = loadStasisSpecimen(id: specimenId) else { return }
        specimens[slotIndex] = specimen
        // Remove from stasis in DB
        removeFromStasis(specimenId: specimenId)
    }

    /// Get all stasis specimens (those with stasisSlotIndex set and reefSlotIndex nil)
    func loadStasisSpecimens() -> [Specimen] {
        guard let db = DatabaseManager.shared.db else { return [] }
        do {
            return try db.read { db in
                try SpecimenRecord
                    .filter(Column("stasisSlotIndex") != nil)
                    .filter(Column("reefSlotIndex") == nil)
                    .fetchAll(db)
                    .compactMap { $0.toSpecimen() }
            }
        } catch {
            print("[ReefStore] Stasis load failed: \(error)")
            return []
        }
    }

    // MARK: - Stasis Persistence Helpers

    func saveSpecimenToStasis(_ specimen: Specimen) {
        guard let db = DatabaseManager.shared.db else { return }
        do {
            try db.write { db in
                var record = SpecimenRecord(from: specimen, stasisSlotIndex: 0)
                record.reefSlotIndex = nil // Not in reef
                try record.save(db)
            }
        } catch {
            print("[ReefStore] Stasis save failed: \(error)")
        }
    }

    private func removeFromStasis(specimenId: UUID) {
        guard let db = DatabaseManager.shared.db else { return }
        do {
            try db.write { db in
                try SpecimenRecord
                    .filter(Column("id") == specimenId.uuidString)
                    .deleteAll(db)
            }
        } catch {
            print("[ReefStore] Stasis remove failed: \(error)")
        }
    }

    private func loadStasisSpecimen(id: UUID) -> Specimen? {
        guard let db = DatabaseManager.shared.db else { return nil }
        do {
            return try db.read { db in
                try SpecimenRecord
                    .filter(Column("id") == id.uuidString)
                    .fetchOne(db)?
                    .toSpecimen()
            }
        } catch {
            print("[ReefStore] Stasis load single failed: \(error)")
            return nil
        }
    }

    // MARK: - Journal

    /// Append a journal event to the specimen in the given reef slot.
    func addJournalEntry(to slotIndex: Int, type: JournalEntry.JournalEventType, description: String) {
        guard slotIndex >= 0, slotIndex < Self.maxSlots,
              var specimen = specimens[slotIndex] else { return }
        let entry = JournalEntry(id: UUID(), timestamp: Date(), type: type, description: description)
        specimen.journal.append(entry)
        specimens[slotIndex] = specimen
    }

}
// MARK: - Persistence implemented in ReefPersistence.swift (GRDB-backed)

