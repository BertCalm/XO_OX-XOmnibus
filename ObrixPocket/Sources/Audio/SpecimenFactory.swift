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

        return Specimen(
            id: UUID(),
            name: name,
            category: wild.category,
            rarity: wild.rarity,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: wild.subtype,
            catchAccelPattern: accelerometer,
            provenance: [],
            spectralDNA: spectralDNA,
            parameterState: params,
            catchLatitude: location?.latitude,
            catchLongitude: location?.longitude,
            catchTimestamp: Date(),
            catchWeatherDescription: weather?.description,
            creatureGenomeData: nil // Phase 1: .xogenome renderer
        )
    }

    /// Create the starter specimen (first launch)
    static func createStarter() -> Specimen {
        Specimen(
            id: UUID(),
            name: "First Light",
            category: .source,
            rarity: .common,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: "PolyBLEP-Saw",
            catchAccelPattern: Array(repeating: 0, count: 16),
            provenance: [],
            spectralDNA: Array(repeating: 0.5, count: 64), // Neutral
            parameterState: [
                "obrix_src1Type": 0,       // PolyBLEP Saw
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
            creatureGenomeData: nil
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

        switch category {
        case .source:
            let srcType: Float = subtype.contains("Saw") ? 0 :
                                  subtype.contains("Square") ? 1 :
                                  subtype.contains("Tri") ? 2 :
                                  subtype.contains("Noise") ? 3 :
                                  subtype.contains("Wavetable") ? 4 :
                                  subtype.contains("FM") ? 5 : 0
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

        // Rarity affects parameter extremes
        if rarity == .rare || rarity == .legendary {
            // Rare specimens have more extreme/interesting parameter values
            for key in params.keys {
                if let val = params[key], val > 0.3 && val < 0.7 {
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
        let adj = adjectives.randomElement()!
        let noun = nouns.randomElement()!
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
