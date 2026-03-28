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
