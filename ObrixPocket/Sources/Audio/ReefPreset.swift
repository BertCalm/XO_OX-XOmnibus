import Foundation

/// A saved reef configuration — specimens, slots, and wiring
struct ReefPreset: Codable, Identifiable {
    let id: UUID
    var name: String
    let savedAt: Date
    let specimenSlots: [SlotSnapshot?]  // 16 slots
    let couplingRoutes: [RouteSnapshot]

    struct SlotSnapshot: Codable {
        let specimenId: UUID
        let name: String
        let subtype: String
        let category: String
        let rarity: String
        let parameterState: [String: Float]
        let level: Int
    }

    struct RouteSnapshot: Codable {
        let sourceIndex: Int  // Slot index, not specimen UUID
        let destIndex: Int
        let depth: Float
    }
}

// MARK: - ReefPresetManager

final class ReefPresetManager: ObservableObject {
    @Published var presets: [ReefPreset] = []

    private let storageKey = "obrix_reef_presets"

    init() { load() }

    func save(name: String, from reefStore: ReefStore) {
        var slots: [ReefPreset.SlotSnapshot?] = []
        for specimen in reefStore.specimens {
            if let spec = specimen {
                slots.append(ReefPreset.SlotSnapshot(
                    specimenId: spec.id,
                    name: spec.name,
                    subtype: spec.subtype,
                    category: spec.category.rawValue,
                    rarity: spec.rarity.rawValue,
                    parameterState: spec.parameterState,
                    level: spec.level
                ))
            } else {
                slots.append(nil)
            }
        }

        var routes: [ReefPreset.RouteSnapshot] = []
        for route in reefStore.couplingRoutes {
            if let srcIdx = reefStore.specimens.firstIndex(where: { $0?.id == route.sourceId }),
               let dstIdx = reefStore.specimens.firstIndex(where: { $0?.id == route.destId }) {
                routes.append(ReefPreset.RouteSnapshot(
                    sourceIndex: srcIdx,
                    destIndex: dstIdx,
                    depth: route.depth
                ))
            }
        }

        let preset = ReefPreset(
            id: UUID(),
            name: name,
            savedAt: Date(),
            specimenSlots: slots,
            couplingRoutes: routes
        )
        presets.append(preset)
        persist()
        ReefStatsTracker.shared.increment(.presetsSaved)
    }

    func restore(_ preset: ReefPreset, to reefStore: ReefStore) {
        // Clear current reef
        for i in 0..<ReefStore.maxSlots {
            reefStore.specimens[i] = nil
        }
        reefStore.couplingRoutes.removeAll()

        // Restore specimens by matching IDs from collection
        let allSpecimens = reefStore.loadAllSpecimens()
        for (index, slot) in preset.specimenSlots.enumerated() {
            guard index < ReefStore.maxSlots, let snapshot = slot else { continue }
            if let specimen = allSpecimens.first(where: { $0.id == snapshot.specimenId }) {
                reefStore.specimens[index] = specimen
            }
        }

        // Restore wiring
        for route in preset.couplingRoutes {
            guard route.sourceIndex < ReefStore.maxSlots,
                  route.destIndex < ReefStore.maxSlots,
                  let srcSpec = reefStore.specimens[route.sourceIndex],
                  let dstSpec = reefStore.specimens[route.destIndex] else { continue }
            reefStore.couplingRoutes.append(CouplingRoute(
                sourceId: srcSpec.id,
                destId: dstSpec.id,
                depth: route.depth,
                connectedSince: Date()
            ))
        }

        reefStore.save()
    }

    func delete(_ preset: ReefPreset) {
        presets.removeAll { $0.id == preset.id }
        persist()
    }

    func rename(_ preset: ReefPreset, to newName: String) {
        if let idx = presets.firstIndex(where: { $0.id == preset.id }) {
            presets[idx].name = newName
            persist()
        }
    }

    // MARK: - Persistence (UserDefaults)

    private func persist() {
        if let data = try? JSONEncoder().encode(presets) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }

    private func load() {
        guard let data = UserDefaults.standard.data(forKey: storageKey),
              let saved = try? JSONDecoder().decode([ReefPreset].self, from: data) else { return }
        presets = saved
    }
}
