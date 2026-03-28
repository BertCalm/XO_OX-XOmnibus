import Foundation
import GRDB

/// Persistence extension for ReefStore
extension ReefStore {

    /// Save the entire reef state to database
    func save() {
        guard let db = DatabaseManager.shared.db else { return }
        do {
            try db.write { db in
                // Clear existing reef specimens and routes
                try SpecimenRecord.filter(Column("reefSlotIndex") != nil).deleteAll(db)
                try CouplingRouteRecord.deleteAll(db)

                // Save reef specimens with slot indices
                for (index, specimen) in specimens.enumerated() {
                    guard let spec = specimen else { continue }
                    var record = SpecimenRecord(from: spec, reefSlotIndex: index)
                    try record.save(db)
                }

                // Save coupling routes
                for route in couplingRoutes {
                    var record = CouplingRouteRecord(from: route)
                    try record.save(db)
                }

                // Save reef metadata
                try saveMetadata(db, key: "reefName", value: reefName)
                try saveMetadata(db, key: "totalDiveDepth", value: "\(totalDiveDepth)")
            }
        } catch {
            print("[ReefPersistence] Save failed: \(error)")
        }
    }

    /// Load reef state from database.
    /// A generation counter guards against double-call races: if two load() calls
    /// overlap, only the most-recent one's async write is applied.
    func load() {
        loadGeneration += 1
        let currentGeneration = loadGeneration

        let db = DatabaseManager.shared.db
        guard let db else { return }
        do {
            try db.read { db in
                // Load reef specimens into their slots
                let records = try SpecimenRecord
                    .filter(Column("reefSlotIndex") != nil)
                    .order(Column("reefSlotIndex"))
                    .fetchAll(db)

                // Reset specimens array
                var loadedSpecimens: [Specimen?] = Array(repeating: nil, count: Self.maxSlots)
                for record in records {
                    if let slot = record.reefSlotIndex,
                       slot >= 0, slot < Self.maxSlots,
                       let specimen = record.toSpecimen() {
                        loadedSpecimens[slot] = specimen
                    }
                }

                // Load coupling routes
                let routeRecords = try CouplingRouteRecord.fetchAll(db)
                let loadedRoutes = routeRecords.compactMap { $0.toCouplingRoute() }

                // Load metadata
                let reefNameValue = try loadMetadata(db, key: "reefName") ?? "My Reef"
                let diveDepthValue = Int(try loadMetadata(db, key: "totalDiveDepth") ?? "0") ?? 0

                // Publish on main thread — bail if a newer load() has already started
                DispatchQueue.main.async { [weak self] in
                    guard let self, self.loadGeneration == currentGeneration else { return }
                    self.specimens = loadedSpecimens
                    self.couplingRoutes = loadedRoutes
                    self.reefName = reefNameValue
                    self.totalDiveDepth = diveDepthValue
                }
            }
        } catch {
            print("[ReefPersistence] Load failed: \(error)")
        }
    }

    /// Save a single specimen to the collection (not in reef)
    func saveSpecimenToCollection(_ specimen: Specimen) {
        guard let db = DatabaseManager.shared.db else { return }
        do {
            try db.write { db in
                var record = SpecimenRecord(from: specimen)
                try record.save(db)
            }
        } catch {
            print("[ReefPersistence] Save specimen failed: \(error)")
        }
    }

    /// Load all specimens (reef + collection + stasis)
    func loadAllSpecimens() -> [Specimen] {
        guard let db = DatabaseManager.shared.db else { return [] }
        do {
            return try db.read { db in
                try SpecimenRecord.fetchAll(db).compactMap { $0.toSpecimen() }
            }
        } catch {
            print("[ReefPersistence] Load all failed: \(error)")
            return []
        }
    }

    /// Apply dormancy health decay (7-day grace window per spec)
    func applyDormancyDecay(daysSinceLastOpen: Int) {
        guard daysSinceLastOpen > 7 else { return } // 7-day grace window

        let decayDays = daysSinceLastOpen - 7 // Only decay past the grace window
        guard let db = DatabaseManager.shared.db else { return }

        do {
            try db.write { db in
                // Decay all non-stasis specimens
                let activeSpecimens = try SpecimenRecord
                    .filter(Column("stasisSlotIndex") == nil)
                    .filter(Column("health") > 0)
                    .fetchAll(db)

                for var record in activeSpecimens {
                    let newHealth = max(0, record.health - decayDays)
                    record.health = newHealth

                    // Bleach to phantom at 0 health
                    if newHealth == 0 && !record.isPhantom {
                        record.isPhantom = true
                    }

                    record.lastHealthUpdate = Date()
                    try record.update(db)
                }
            }

            // Reload reef on main thread to reflect changes
            DispatchQueue.main.async { [weak self] in
                self?.load()
            }
        } catch {
            print("[ReefPersistence] Dormancy decay failed: \(error)")
        }
    }

    /// Save exploration geohash
    func saveVisitedGeohash(_ hash: String) {
        guard let db = DatabaseManager.shared.db else { return }
        do {
            try db.write { db in
                try db.execute(sql: """
                    INSERT OR IGNORE INTO visitedGeohash (hash, visitedAt) VALUES (?, ?)
                """, arguments: [hash, Date()])
            }
        } catch {
            print("[ReefPersistence] Save geohash failed: \(error)")
        }
    }

    /// Load all visited geohashes
    func loadVisitedGeohashes() -> Set<String> {
        guard let db = DatabaseManager.shared.db else { return [] }
        do {
            return try db.read { db in
                let rows = try Row.fetchAll(db, sql: "SELECT hash FROM visitedGeohash")
                return Set(rows.map { $0["hash"] as String })
            }
        } catch {
            return []
        }
    }

    // MARK: - Metadata Helpers

    private func saveMetadata(_ db: Database, key: String, value: String) throws {
        try db.execute(sql: """
            INSERT OR REPLACE INTO reefMeta (key, value) VALUES (?, ?)
        """, arguments: [key, value])
    }

    private func loadMetadata(_ db: Database, key: String) throws -> String? {
        try String.fetchOne(db, sql: "SELECT value FROM reefMeta WHERE key = ?", arguments: [key])
    }
}
