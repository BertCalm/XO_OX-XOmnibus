import Foundation
import CoreLocation
import CoreMotion

/// Creates Specimen objects from catch environment data
final class SpecimenFactory {

    /// Create a specimen from a wild specimen catch
    static func create(
        from wild: WildSpecimen,
        spectralDNA: [Float],
        location: CLLocationCoordinate2D?,
        weather: WeatherSnapshot?,
        accelerometer: [Float]
    ) -> Specimen {

        // Generate OBRIX parameter state based on specimen type
        let params = generateParameters(category: wild.category, subtype: wild.subtype, rarity: wild.rarity, biome: wild.biome)

        // Generate creature name
        let name = generateName(category: wild.category, biome: wild.biome)

        // Resolve catalog name from subtype
        let catalogID = SpecimenCatalog.catalogSubtypeID(from: wild.subtype)
        let catalogEntry = SpecimenCatalog.entry(for: catalogID)
        let displayName = catalogEntry?.displayName(rarity: wild.rarity) ?? name

        let bornEntry = JournalEntry(
            id: UUID(),
            timestamp: Date(),
            type: .born,
            description: "Caught in the wild"
        )

        return Specimen(
            id: UUID(),
            name: displayName,
            category: wild.category,
            rarity: wild.rarity,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: catalogID,
            catchAccelPattern: accelerometer,
            provenance: [],
            spectralDNA: spectralDNA,
            parameterState: params,
            catchLatitude: location?.latitude,
            catchLongitude: location?.longitude,
            catchTimestamp: Date(),
            catchWeatherDescription: weather?.description,
            creatureGenomeData: nil,
            cosmeticTier: .standard,
            morphIndex: 0,
            musicHash: nil,
            sourceTrackTitle: nil,
            xp: 0,
            level: 1,
            aggressiveScore: 0,
            gentleScore: 0,
            totalPlaySeconds: 0,
            journal: [bornEntry]
        )
    }

    /// Create the starter specimen (first launch) — always a Common Sawfin
    static func createStarter() -> Specimen {
        let bornEntry = JournalEntry(
            id: UUID(),
            timestamp: Date(),
            type: .born,
            description: "Born on first launch"
        )
        return Specimen(
            id: UUID(),
            name: "Sawfin",
            category: .source,
            rarity: .common,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: "polyblep-saw",
            catchAccelPattern: Array(repeating: 0, count: 16),
            provenance: [],
            spectralDNA: Array(repeating: 0.5, count: 64),
            parameterState: [
                "obrix_src1Type": 0,
                "obrix_src1Tune": 0,
                "obrix_src1Level": 0.8,
                "obrix_flt1Cutoff": 0.7,
                "obrix_flt1Resonance": 0.2,
                "obrix_env1Attack": 0.01,
                "obrix_env1Decay": 0.3,
                "obrix_env1Sustain": 0.6,
                "obrix_env1Release": 0.4,
                "obrix_lfo1Rate": 0.3,
                "obrix_lfo1Depth": 0.15,
            ],
            catchLatitude: nil,
            catchLongitude: nil,
            catchTimestamp: Date(),
            catchWeatherDescription: "First Launch",
            creatureGenomeData: nil,
            cosmeticTier: .standard,
            morphIndex: 0,
            musicHash: nil,
            sourceTrackTitle: nil,
            xp: 0,
            level: 1,
            aggressiveScore: 0,
            gentleScore: 0,
            totalPlaySeconds: 0,
            journal: [bornEntry]
        )
    }

    // MARK: - Parameter Generation

    private static func generateParameters(category: SpecimenCategory, subtype: String, rarity: SpecimenRarity, biome: Biome) -> [String: Float] {
        var params: [String: Float] = [:]

        // Base parameters every specimen has
        params["obrix_env1Attack"] = Float.random(in: 0.001...0.3)
        params["obrix_env1Decay"] = Float.random(in: 0.05...0.8)
        params["obrix_env1Sustain"] = Float.random(in: 0.2...0.9)
        params["obrix_env1Release"] = Float.random(in: 0.1...2.0)

        // Resolve to catalog ID for matching
        let catalogID = SpecimenCatalog.catalogSubtypeID(from: subtype)

        switch category {
        case .source:
            let srcTypeMap: [String: Float] = [
                "polyblep-saw": 0, "polyblep-square": 1, "polyblep-tri": 2,
                "noise-white": 3, "noise-pink": 3,
                "wt-analog": 4, "wt-vocal": 4,
                "fm-basic": 5
            ]
            let srcType: Float = srcTypeMap[catalogID] ?? 0
            params["obrix_src1Type"] = srcType
            params["obrix_src1Tune"] = Float.random(in: -12...12)
            params["obrix_src1Level"] = Float.random(in: 0.5...1.0)

        case .processor:
            params["obrix_flt1Cutoff"] = Float.random(in: 0.1...0.95)
            params["obrix_flt1Resonance"] = Float.random(in: 0.0...0.8)
            params["obrix_flt1EnvDepth"] = Float.random(in: -0.5...0.5)

        case .modulator:
            params["obrix_lfo1Rate"] = Float.random(in: 0.01...10.0)
            params["obrix_lfo1Depth"] = Float.random(in: 0.1...0.8)
            params["obrix_lfo1Shape"] = Float.random(in: 0...3).rounded()

        case .effect:
            params["obrix_fx1Mix"] = Float.random(in: 0.2...0.8)
            params["obrix_fx1Param1"] = Float.random(in: 0.1...0.9)
            params["obrix_fx1Param2"] = Float.random(in: 0.1...0.9)
        }

        // Apply catalog defaults as the base, with small random variation (±10%) layered on top.
        // This ensures each specimen type has a distinct sonic character regardless of catch randomness.
        if let entry = SpecimenCatalog.entry(for: catalogID) {
            for (key, defaultVal) in entry.defaultParams {
                let variation = Float.random(in: -0.1...0.1)
                params[key] = defaultVal * (1.0 + variation)
            }
        }

        // Rarity affects parameter extremes
        if rarity == .rare || rarity == .legendary {
            // Rare specimens have more extreme/interesting parameter values.
            // Guard: only push params that are normalized 0-1; skip raw values like tune (-12…12)
            // or shape (0…3) to avoid clamping them into nonsensical values.
            for key in params.keys {
                if let val = params[key], val >= 0.0, val <= 1.0, val > 0.3, val < 0.7 {
                    // Push away from center — more character
                    params[key] = val < 0.5 ? val * 0.5 : val + (1.0 - val) * 0.5
                }
            }
        }

        return params
    }

    // MARK: - Name Generation

    private static let adjectives = [
        "Luminous", "Deep", "Crystalline", "Phantom", "Prism",
        "Ethereal", "Kinetic", "Submerged", "Flux", "Aether",
        "Twilight", "Dawn", "Storm", "Drift", "Pulse",
        "Echo", "Coral", "Tide", "Mist", "Ember"
    ]

    private static let nouns = [
        "Shell", "Current", "Polyp", "Tendril", "Wave",
        "Reef", "Spiral", "Bloom", "Cascade", "Ripple",
        "Surge", "Glow", "Fathom", "Spire", "Veil",
        "Arc", "Plume", "Shard", "Core", "Crest"
    ]

    private static func generateName(category: SpecimenCategory, biome: Biome) -> String {
        let adj = adjectives.randomElement() ?? "Luminous"
        let noun = nouns.randomElement() ?? "Shell"
        return "\(adj) \(noun)"
    }
}

/// Weather data snapshot for spectral DNA
struct WeatherSnapshot: Codable {
    let temperature: Float  // Celsius
    let humidity: Float     // 0-1
    let pressure: Float     // hPa
    let description: String // "Rain", "Clear", etc.

    /// Fallback for offline/API failure
    static let fairWeather = WeatherSnapshot(
        temperature: 20, humidity: 0.5, pressure: 1013, description: "Fair Weather"
    )
}
