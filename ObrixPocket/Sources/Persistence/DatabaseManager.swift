// DEPENDENCY: GRDB.swift (https://github.com/groue/GRDB.swift)
// Add via Xcode: File → Add Package Dependencies → https://github.com/groue/GRDB.swift
// Or add to project.yml packages section

import Foundation
import GRDB

/// Central database manager for OBRIX Pocket persistence
final class DatabaseManager {
    static let shared = DatabaseManager()

    private var dbQueue: DatabaseQueue?

    private init() {
        do {
            let databaseURL = try FileManager.default
                .url(for: .applicationSupportDirectory, in: .userDomainMask, appropriateFor: nil, create: true)
                .appendingPathComponent("obrix_pocket.sqlite")

            var config = Configuration()
            config.foreignKeysEnabled = true

            dbQueue = try DatabaseQueue(path: databaseURL.path, configuration: config)
            try migrate()
        } catch {
            print("[DatabaseManager] Failed to initialize: \(error)")
        }
    }

    var db: DatabaseQueue {
        guard let dbQueue else {
            fatalError("[DatabaseManager] Database not initialized")
        }
        return dbQueue
    }

    // MARK: - Migrations

    private func migrate() throws {
        var migrator = DatabaseMigrator()

        migrator.registerMigration("v1_create_tables") { db in
            // Specimens table
            try db.create(table: "specimen") { t in
                t.column("id", .text).primaryKey()
                t.column("name", .text).notNull()
                t.column("category", .text).notNull()
                t.column("subtype", .text).notNull()
                t.column("rarity", .text).notNull()
                t.column("health", .integer).notNull().defaults(to: 100)
                t.column("isPhantom", .boolean).notNull().defaults(to: false)
                t.column("phantomScar", .boolean).notNull().defaults(to: false)
                t.column("spectralDNA", .text).notNull() // JSON array of 64 floats
                t.column("parameterState", .text).notNull() // JSON dict
                t.column("catchLatitude", .double)
                t.column("catchLongitude", .double)
                t.column("catchTimestamp", .datetime).notNull()
                t.column("catchWeatherDescription", .text)
                t.column("catchAccelPattern", .text) // JSON array of floats
                t.column("provenance", .text).notNull().defaults(to: "[]") // JSON array
                t.column("creatureGenomeData", .blob)
                t.column("reefSlotIndex", .integer) // nil = in collection, not in reef
                t.column("stasisSlotIndex", .integer) // nil = not in stasis
                t.column("lastHealthUpdate", .datetime)
            }

            // Coupling routes table
            try db.create(table: "couplingRoute") { t in
                t.autoIncrementedPrimaryKey("id")
                t.column("sourceSpecimenId", .text).notNull()
                    .references("specimen", onDelete: .cascade)
                t.column("destSpecimenId", .text).notNull()
                    .references("specimen", onDelete: .cascade)
                t.column("depth", .double).notNull().defaults(to: 0.5)
            }

            // Reef metadata
            try db.create(table: "reefMeta") { t in
                t.column("key", .text).primaryKey()
                t.column("value", .text).notNull()
            }

            // Visited geohashes (for exploration bonus)
            try db.create(table: "visitedGeohash") { t in
                t.column("hash", .text).primaryKey()
                t.column("visitedAt", .datetime).notNull()
            }
        }

        try migrator.migrate(db)
    }
}
