import Foundation

// MARK: - XOReef File Format

/// .xoreef file format — captures a reef configuration for sharing/export.
/// Used for player-to-player sharing and desktop XOceanus import.
struct XOReefFile: Codable {
    let format: String
    let version: Int
    let exportDate: Date
    let reefName: String
    let totalDiveDepth: Int
    let specimens: [XOReefSpecimen?]  // 16 slots; nil = empty slot
    let routes: [XOReefRoute]

    init(
        exportDate: Date,
        reefName: String,
        totalDiveDepth: Int,
        specimens: [XOReefSpecimen?],
        routes: [XOReefRoute]
    ) {
        self.format = "xoreef"
        self.version = 1
        self.exportDate = exportDate
        self.reefName = reefName
        self.totalDiveDepth = totalDiveDepth
        self.specimens = specimens
        self.routes = routes
    }

    struct XOReefSpecimen: Codable {
        let name: String
        let subtype: String
        let category: String
        let rarity: String
        let level: Int
        let cosmeticTier: String
        let parameterState: [String: Float]
        let spectralDNA: [Float]
        let sourceTrackTitle: String?
        let personalityLine: String?
    }

    struct XOReefRoute: Codable {
        let sourceSlot: Int
        let destSlot: Int
        let depth: Float
    }
}

// MARK: - XOReefImporter

enum XOReefImporter {

    /// Parse a .xoreef JSON file
    static func parse(data: Data) -> XOReefFile? {
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        return try? decoder.decode(XOReefFile.self, from: data)
    }

    /// Import a parsed reef into ReefStore (replaces current reef)
    static func importReef(_ file: XOReefFile, into reefStore: ReefStore) {
        // Clear current reef
        for i in 0..<ReefStore.maxSlots {
            reefStore.specimens[i] = nil
        }
        reefStore.couplingRoutes.removeAll()

        // Import specimens into slots
        for (index, specData) in file.specimens.enumerated() {
            guard let data = specData, index < ReefStore.maxSlots else { continue }
            guard let category = SpecimenCategory(rawValue: data.category),
                  let rarity = SpecimenRarity(rawValue: data.rarity) else { continue }

            let specimen = Specimen(
                id: UUID(), // New UUID for imported specimens
                name: data.name,
                category: category,
                rarity: rarity,
                health: 100,
                isPhantom: false,
                phantomScar: false,
                subtype: data.subtype,
                catchAccelPattern: [],
                provenance: [],
                spectralDNA: data.spectralDNA,
                parameterState: data.parameterState,
                catchLatitude: nil,
                catchLongitude: nil,
                catchTimestamp: Date(),
                catchWeatherDescription: "Imported",
                creatureGenomeData: nil,
                cosmeticTier: CosmeticTier(rawValue: data.cosmeticTier) ?? .standard,
                morphIndex: 0,
                musicHash: nil,
                sourceTrackTitle: data.sourceTrackTitle,
                xp: 0,       // Imported specimens start fresh
                level: data.level, // But keep their level
                aggressiveScore: 0,
                gentleScore: 0,
                totalPlaySeconds: 0,
                journal: [JournalEntry(id: UUID(), timestamp: Date(), type: .born, description: "Imported from \(file.reefName)")],
                isFavorite: false
            )
            reefStore.specimens[index] = specimen
        }

        // Import coupling routes
        for route in file.routes {
            guard route.sourceSlot < ReefStore.maxSlots,
                  route.destSlot < ReefStore.maxSlots,
                  let srcSpec = reefStore.specimens[route.sourceSlot],
                  let dstSpec = reefStore.specimens[route.destSlot] else { continue }

            reefStore.couplingRoutes.append(CouplingRoute(
                sourceId: srcSpec.id,
                destId: dstSpec.id,
                depth: route.depth,
                connectedSince: Date()
            ))
        }

        reefStore.reefName = file.reefName
        reefStore.save()
    }
}

// MARK: - XOReefExporter

enum XOReefExporter {

    /// Encode the current reef to .xoreef JSON data.
    static func export(reefStore: ReefStore) -> Data? {
        var specimens: [XOReefFile.XOReefSpecimen?] = []

        for spec in reefStore.specimens {
            if let s = spec {
                let entry = SpecimenCatalog.entry(for: s.subtype)
                specimens.append(XOReefFile.XOReefSpecimen(
                    name: s.name,
                    subtype: s.subtype,
                    category: s.category.rawValue,
                    rarity: s.rarity.rawValue,
                    level: s.level,
                    cosmeticTier: s.cosmeticTier.rawValue,
                    parameterState: s.parameterState,
                    spectralDNA: s.spectralDNA,
                    sourceTrackTitle: s.sourceTrackTitle,
                    personalityLine: entry?.personalityLine
                ))
            } else {
                specimens.append(nil)
            }
        }

        var routes: [XOReefFile.XOReefRoute] = []
        for route in reefStore.couplingRoutes {
            if let srcIdx = reefStore.specimens.firstIndex(where: { $0?.id == route.sourceId }),
               let dstIdx = reefStore.specimens.firstIndex(where: { $0?.id == route.destId }) {
                routes.append(XOReefFile.XOReefRoute(
                    sourceSlot: srcIdx,
                    destSlot: dstIdx,
                    depth: route.depth
                ))
            }
        }

        let file = XOReefFile(
            exportDate: Date(),
            reefName: reefStore.reefName,
            totalDiveDepth: reefStore.totalDiveDepth,
            specimens: specimens,
            routes: routes
        )

        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        encoder.dateEncodingStrategy = .iso8601
        return try? encoder.encode(file)
    }

    /// Write to a temporary file and return the URL for sharing.
    /// Returns nil if encoding or file write fails.
    static func exportToFile(reefStore: ReefStore) -> URL? {
        guard let data = export(reefStore: reefStore) else { return nil }

        let safeName = reefStore.reefName
            .replacingOccurrences(of: " ", with: "_")
            .replacingOccurrences(of: "/", with: "-")
        let fileName = "\(safeName).xoreef"
        let tempDir = FileManager.default.temporaryDirectory
            .appendingPathComponent("XOReefExports", isDirectory: true)

        do {
            try FileManager.default.createDirectory(at: tempDir, withIntermediateDirectories: true)
            let fileURL = tempDir.appendingPathComponent(fileName)
            try data.write(to: fileURL)
            return fileURL
        } catch {
            return nil
        }
    }
}
