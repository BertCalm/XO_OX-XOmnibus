import Foundation

/// A complete snapshot of a reef's state — the "preset system" for living ecosystems
struct ReefSnapshot: Identifiable {
    let id: UUID
    var name: String
    let createdDate: Date
    var lastLoadedDate: Date?

    /// Grid layout: slot index → specimen subtype ID (nil = empty slot)
    let slotContents: [Int: String]

    /// Wiring connections: [(from, to)]
    let wirings: [(Int, Int)]

    /// Specimen mutations (from SpecimenTweaker)
    let mutations: [Int: [String: Float]]   // slotIndex → {paramID: tweakedValue}

    /// Macro knob settings
    let macroSettings: [MacroKnobSnapshot]

    /// Scale and key
    let key: Int           // 0-11
    let scale: String      // ExtendedScale raw value

    /// Number of occupied slots
    var populationCount: Int {
        slotContents.count
    }

    /// Display string: "My Jazz Reef (8 specimens)"
    var displayString: String {
        "\(name) (\(populationCount) specimens)"
    }
}

/// Snapshot of a single macro knob
struct MacroKnobSnapshot: Codable {
    let index: Int
    let targetRawValue: String   // MacroTarget raw value
    let value: Float
}

/// Result of comparing two reef snapshots
struct ReefComparison {
    let snapshotA: ReefSnapshot
    let snapshotB: ReefSnapshot

    /// Specimens in A but not in B
    var onlyInA: [String] {
        let bTypes = Set(snapshotB.slotContents.values)
        return snapshotA.slotContents.values.filter { !bTypes.contains($0) }
    }

    /// Specimens in B but not in A
    var onlyInB: [String] {
        let aTypes = Set(snapshotA.slotContents.values)
        return snapshotB.slotContents.values.filter { !aTypes.contains($0) }
    }

    /// Specimens in both
    var shared: [String] {
        let aTypes = Set(snapshotA.slotContents.values)
        let bTypes = Set(snapshotB.slotContents.values)
        return Array(aTypes.intersection(bTypes))
    }

    /// Population difference
    var populationDiff: Int {
        snapshotB.populationCount - snapshotA.populationCount
    }

    /// Whether the key/scale changed
    var harmonyChanged: Bool {
        snapshotA.key != snapshotB.key || snapshotA.scale != snapshotB.scale
    }

    /// Number of wiring differences
    var wiringDifferences: Int {
        let aWires = Set(snapshotA.wirings.map { "\($0.0)-\($0.1)" })
        let bWires = Set(snapshotB.wirings.map { "\($0.0)-\($0.1)" })
        return aWires.symmetricDifference(bWires).count
    }

    /// Summary text for display
    var summary: String {
        var parts: [String] = []
        if populationDiff > 0 {
            parts.append("+\(populationDiff) specimens")
        } else if populationDiff < 0 {
            parts.append("\(populationDiff) specimens")
        }
        if !onlyInA.isEmpty { parts.append("\(onlyInA.count) removed") }
        if !onlyInB.isEmpty { parts.append("\(onlyInB.count) added") }
        if harmonyChanged { parts.append("key/scale changed") }
        if wiringDifferences > 0 { parts.append("\(wiringDifferences) wiring changes") }
        return parts.isEmpty ? "Identical" : parts.joined(separator: ", ")
    }
}

/// Manages reef snapshots — save, load, compare, and organize reef configurations.
///
/// Snapshots are the "preset system" for OBRIX Pocket, but instead of saving
/// synth patches, they save entire living ecosystems: grid layout, wiring,
/// mutations, macros, scale/key.
///
/// Design: "I can finally have multiple personalities for my instrument."
final class ReefSnapshotManager: ObservableObject {

    // MARK: - State

    @Published var snapshots: [ReefSnapshot] = []

    /// Currently active snapshot name (nil = unsaved state)
    @Published var activeSnapshotName: String? = nil

    /// Maximum number of snapshots
    static let maxSnapshots: Int = 20

    // MARK: - Save

    /// Capture the current reef state as a snapshot
    func saveSnapshot(
        name: String,
        slotContents: [Int: String],
        wirings: [(Int, Int)],
        mutations: [Int: [String: Float]],
        macros: [MacroKnobSnapshot],
        key: Int,
        scale: String
    ) -> ReefSnapshot? {
        // Enforce limit
        if snapshots.count >= Self.maxSnapshots {
            // Remove oldest (by creation date)
            if let oldest = snapshots.min(by: { $0.createdDate < $1.createdDate }) {
                snapshots.removeAll { $0.id == oldest.id }
            }
        }

        let snapshot = ReefSnapshot(
            id: UUID(),
            name: name,
            createdDate: Date(),
            lastLoadedDate: nil,
            slotContents: slotContents,
            wirings: wirings,
            mutations: mutations,
            macroSettings: macros,
            key: key,
            scale: scale
        )

        snapshots.append(snapshot)
        activeSnapshotName = name
        save()
        return snapshot
    }

    /// Overwrite an existing snapshot with current state
    func updateSnapshot(id: UUID, slotContents: [Int: String], wirings: [(Int, Int)],
                        mutations: [Int: [String: Float]], macros: [MacroKnobSnapshot],
                        key: Int, scale: String) {
        guard let index = snapshots.firstIndex(where: { $0.id == id }) else { return }
        let old = snapshots[index]
        snapshots[index] = ReefSnapshot(
            id: old.id,
            name: old.name,
            createdDate: old.createdDate,
            lastLoadedDate: Date(),
            slotContents: slotContents,
            wirings: wirings,
            mutations: mutations,
            macroSettings: macros,
            key: key,
            scale: scale
        )
        save()
    }

    // MARK: - Load

    /// Load a snapshot by ID. Returns the snapshot data for the caller to apply.
    func loadSnapshot(id: UUID) -> ReefSnapshot? {
        guard let index = snapshots.firstIndex(where: { $0.id == id }) else { return nil }
        snapshots[index].lastLoadedDate = Date()
        activeSnapshotName = snapshots[index].name
        save()
        return snapshots[index]
    }

    // MARK: - Compare

    /// Compare two snapshots
    func compare(_ idA: UUID, _ idB: UUID) -> ReefComparison? {
        guard let a = snapshots.first(where: { $0.id == idA }),
              let b = snapshots.first(where: { $0.id == idB }) else { return nil }
        return ReefComparison(snapshotA: a, snapshotB: b)
    }

    // MARK: - Management

    /// Rename a snapshot
    func rename(id: UUID, newName: String) {
        guard let index = snapshots.firstIndex(where: { $0.id == id }) else { return }
        snapshots[index].name = newName
        if activeSnapshotName == snapshots[index].name {
            activeSnapshotName = newName
        }
        save()
    }

    /// Delete a snapshot
    func delete(id: UUID) {
        snapshots.removeAll { $0.id == id }
        save()
    }

    /// Get snapshots sorted by last used (most recent first)
    var recentSnapshots: [ReefSnapshot] {
        snapshots.sorted { ($0.lastLoadedDate ?? $0.createdDate) > ($1.lastLoadedDate ?? $1.createdDate) }
    }

    /// Get snapshots sorted by name
    var sortedByName: [ReefSnapshot] {
        snapshots.sorted { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }
    }

    // MARK: - Persistence

    private let storageKey = "reefSnapshots"

    func save() {
        // Custom encoding to handle tuple arrays
        let encodable = snapshots.map { snapshot -> EncodableSnapshot in
            EncodableSnapshot(
                id: snapshot.id,
                name: snapshot.name,
                createdDate: snapshot.createdDate,
                lastLoadedDate: snapshot.lastLoadedDate,
                slotContents: snapshot.slotContents,
                wirings: snapshot.wirings.map { WiringPair(from: $0.0, to: $0.1) },
                mutations: snapshot.mutations,
                macroSettings: snapshot.macroSettings,
                key: snapshot.key,
                scale: snapshot.scale
            )
        }
        if let data = try? JSONEncoder().encode(encodable) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: storageKey),
           let decoded = try? JSONDecoder().decode([EncodableSnapshot].self, from: data) {
            snapshots = decoded.map { enc in
                ReefSnapshot(
                    id: enc.id,
                    name: enc.name,
                    createdDate: enc.createdDate,
                    lastLoadedDate: enc.lastLoadedDate,
                    slotContents: enc.slotContents,
                    wirings: enc.wirings.map { ($0.from, $0.to) },
                    mutations: enc.mutations,
                    macroSettings: enc.macroSettings,
                    key: enc.key,
                    scale: enc.scale
                )
            }
        }
    }
}

// MARK: - Encoding Helpers (tuples aren't Codable)

private struct WiringPair: Codable {
    let from: Int
    let to: Int
}

private struct EncodableSnapshot: Codable {
    let id: UUID
    let name: String
    let createdDate: Date
    let lastLoadedDate: Date?
    let slotContents: [Int: String]
    let wirings: [WiringPair]
    let mutations: [Int: [String: Float]]
    let macroSettings: [MacroKnobSnapshot]
    let key: Int
    let scale: String
}
