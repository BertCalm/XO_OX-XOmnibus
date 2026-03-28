import Foundation
import GRDB

/// GRDB record for the Specimen model
struct SpecimenRecord: Codable, FetchableRecord, PersistableRecord {
    static let databaseTableName = "specimen"

    var id: String
    var name: String
    var category: String
    var subtype: String
    var rarity: String
    var health: Int
    var isPhantom: Bool
    var phantomScar: Bool
    var spectralDNA: String // JSON-encoded [Float]
    var parameterState: String // JSON-encoded [String: Float]
    var catchLatitude: Double?
    var catchLongitude: Double?
    var catchTimestamp: Date
    var catchWeatherDescription: String?
    var catchAccelPattern: String? // JSON-encoded [Float]
    var provenance: String // JSON-encoded [ProvenanceEntry]
    var creatureGenomeData: Data?
    var reefSlotIndex: Int?
    var stasisSlotIndex: Int?
    var lastHealthUpdate: Date?

    // A+B milestone fields
    var cosmeticTier: String
    var morphIndex: Int
    var musicHash: String?
    var sourceTrackTitle: String?

    // Leveling system fields
    var xp: Int
    var level: Int
    var aggressiveScore: Double
    var gentleScore: Double
    var totalPlaySeconds: Double

    // MARK: - Conversion to/from Specimen

    init(from specimen: Specimen, reefSlotIndex: Int? = nil, stasisSlotIndex: Int? = nil) {
        self.id = specimen.id.uuidString
        self.name = specimen.name
        self.category = specimen.category.rawValue
        self.subtype = specimen.subtype
        self.rarity = specimen.rarity.rawValue
        self.health = specimen.health
        self.isPhantom = specimen.isPhantom
        self.phantomScar = specimen.phantomScar
        self.spectralDNA = Self.encodeFloats(specimen.spectralDNA)
        self.parameterState = Self.encodeDict(specimen.parameterState)
        self.catchLatitude = specimen.catchLatitude
        self.catchLongitude = specimen.catchLongitude
        self.catchTimestamp = specimen.catchTimestamp
        self.catchWeatherDescription = specimen.catchWeatherDescription
        self.catchAccelPattern = Self.encodeFloats(specimen.catchAccelPattern)
        self.provenance = Self.encodeProvenance(specimen.provenance)
        self.creatureGenomeData = specimen.creatureGenomeData
        self.reefSlotIndex = reefSlotIndex
        self.stasisSlotIndex = stasisSlotIndex
        self.lastHealthUpdate = Date()
        self.cosmeticTier = specimen.cosmeticTier.rawValue
        self.morphIndex = specimen.morphIndex
        self.musicHash = specimen.musicHash
        self.sourceTrackTitle = specimen.sourceTrackTitle
        self.xp = specimen.xp
        self.level = specimen.level
        self.aggressiveScore = Double(specimen.aggressiveScore)
        self.gentleScore = Double(specimen.gentleScore)
        self.totalPlaySeconds = specimen.totalPlaySeconds
    }

    func toSpecimen() -> Specimen? {
        guard let uuid = UUID(uuidString: id),
              let cat = SpecimenCategory(rawValue: category),
              let rar = SpecimenRarity(rawValue: rarity) else { return nil }

        return Specimen(
            id: uuid,
            name: name,
            category: cat,
            rarity: rar,
            health: health,
            isPhantom: isPhantom,
            phantomScar: phantomScar,
            subtype: subtype,
            catchAccelPattern: Self.decodeFloats(catchAccelPattern ?? "[]"),
            provenance: Self.decodeProvenance(provenance),
            spectralDNA: Self.decodeFloats(spectralDNA),
            parameterState: Self.decodeDict(parameterState),
            catchLatitude: catchLatitude,
            catchLongitude: catchLongitude,
            catchTimestamp: catchTimestamp,
            catchWeatherDescription: catchWeatherDescription,
            creatureGenomeData: creatureGenomeData,
            cosmeticTier: CosmeticTier(rawValue: cosmeticTier) ?? .standard,
            morphIndex: morphIndex,
            musicHash: musicHash,
            sourceTrackTitle: sourceTrackTitle,
            xp: xp,
            level: level,
            aggressiveScore: Float(aggressiveScore),
            gentleScore: Float(gentleScore),
            totalPlaySeconds: totalPlaySeconds
        )
    }

    // MARK: - JSON Encoding Helpers

    private static func encodeFloats(_ floats: [Float]) -> String {
        (try? JSONEncoder().encode(floats)).flatMap { String(data: $0, encoding: .utf8) } ?? "[]"
    }

    private static func decodeFloats(_ json: String) -> [Float] {
        (try? JSONDecoder().decode([Float].self, from: Data(json.utf8))) ?? []
    }

    private static func encodeDict(_ dict: [String: Float]) -> String {
        (try? JSONEncoder().encode(dict)).flatMap { String(data: $0, encoding: .utf8) } ?? "{}"
    }

    private static func decodeDict(_ json: String) -> [String: Float] {
        (try? JSONDecoder().decode([String: Float].self, from: Data(json.utf8))) ?? [:]
    }

    private static func encodeProvenance(_ entries: [ProvenanceEntry]) -> String {
        (try? JSONEncoder().encode(entries)).flatMap { String(data: $0, encoding: .utf8) } ?? "[]"
    }

    private static func decodeProvenance(_ json: String) -> [ProvenanceEntry] {
        (try? JSONDecoder().decode([ProvenanceEntry].self, from: Data(json.utf8))) ?? []
    }
}

/// GRDB record for coupling routes
struct CouplingRouteRecord: Codable, FetchableRecord, PersistableRecord {
    static let databaseTableName = "couplingRoute"

    var id: Int64?
    var sourceSpecimenId: String
    var destSpecimenId: String
    var depth: Double
    var connectedSince: Date?

    init(from route: CouplingRoute) {
        self.sourceSpecimenId = route.sourceId.uuidString
        self.destSpecimenId = route.destId.uuidString
        self.depth = Double(route.depth)
        self.connectedSince = route.connectedSince
    }

    func toCouplingRoute() -> CouplingRoute? {
        guard let srcId = UUID(uuidString: sourceSpecimenId),
              let dstId = UUID(uuidString: destSpecimenId) else { return nil }
        return CouplingRoute(sourceId: srcId, destId: dstId, depth: Float(depth), connectedSince: connectedSince)
    }
}
