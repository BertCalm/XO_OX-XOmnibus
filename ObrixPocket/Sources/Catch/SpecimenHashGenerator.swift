import Foundation
import CryptoKit

/// Deterministic specimen generation from music library metadata.
/// SHA256(artist|title|album|duration) → specimen seed → full specimen.
/// Same metadata = same specimen type everywhere. "Alchemy, not gacha."
enum SpecimenHashGenerator {

    /// Intermediate seed extracted from a SHA256 hash
    struct SpecimenSeed {
        let subtypeIndex: Int       // 0-15: which of the 16 CORE specimen types
        let rarityRoll: Float       // 0-1: determines rarity tier
        let spectralSeeds: [Float]  // 6 values: spectral DNA dimensions
        let morphIndex: Int         // 0 or 1 (V1 morphs)
        let cosmeticRoll: Float     // 0-1: determines cosmetic tier
        let hashHex: String         // Full SHA256 hex for provenance
    }

    // MARK: - Hash Generation

    /// Generate a deterministic specimen seed from song metadata.
    /// The hash is DETERMINISTIC: same metadata → same seed → same specimen type.
    static func generateSeed(
        artist: String,
        title: String,
        album: String,
        duration: TimeInterval
    ) -> SpecimenSeed {
        // Canonical input: trimmed, lowercased for consistency across devices
        let input = [
            artist.trimmingCharacters(in: .whitespacesAndNewlines).lowercased(),
            title.trimmingCharacters(in: .whitespacesAndNewlines).lowercased(),
            album.trimmingCharacters(in: .whitespacesAndNewlines).lowercased(),
            String(Int(duration)) // Truncate to whole seconds for consistency
        ].joined(separator: "|")

        let hash = SHA256.hash(data: Data(input.utf8))
        let bytes = Array(hash)
        let hashHex = bytes.map { String(format: "%02x", $0) }.joined()

        // Extract deterministic values from hash bytes
        // Bytes 0:    subtype index (0-15, core specimens only — deep unlocked separately)
        // Byte  1:    rarity roll
        // Bytes 2-7:  spectral DNA seeds (6 dimensions)
        // Byte  8:    morph index
        // Byte  9:    cosmetic roll
        let subtypeIndex = Int(bytes[0]) % SpecimenCatalog.coreCount
        let rarityRoll = Float(bytes[1]) / 255.0
        let spectralSeeds = (0..<6).map { Float(bytes[2 + $0]) / 255.0 }
        let morphIndex = Int(bytes[8]) % 2
        let cosmeticRoll = Float(bytes[9]) / 255.0

        return SpecimenSeed(
            subtypeIndex: subtypeIndex,
            rarityRoll: rarityRoll,
            spectralSeeds: spectralSeeds,
            morphIndex: morphIndex,
            cosmeticRoll: cosmeticRoll,
            hashHex: hashHex
        )
    }

    // MARK: - Rarity from Roll

    /// Rarity probability distribution (spec Section 7.2)
    static func rarity(from roll: Float) -> SpecimenRarity {
        switch roll {
        case 0..<0.70: return .common      // 70%
        case 0.70..<0.90: return .uncommon // 20%
        case 0.90..<0.98: return .rare     //  8%
        default: return .legendary          //  2%
        }
    }

    // MARK: - Cosmetic Tier from Roll (Music Hash variant)

    /// Music library hash catches unlock Fossilized tier (not available elsewhere).
    /// Prismatic is NOT available from music hash (coupling discovery only).
    static func cosmeticTier(from roll: Float) -> CosmeticTier {
        switch roll {
        case 0..<0.855: return .standard       // 85.5%
        case 0.855..<0.955: return .bioluminescent // 10%
        case 0.955..<0.985: return .phantom    // 3%
        default: return .fossilized             // 1.5%
        }
    }

    // MARK: - Spectral DNA Expansion

    /// Expand 6 seed dimensions into full 64-float spectral DNA profile.
    /// The 6 seeds map to broad frequency regions; cubic interpolation fills the gaps.
    static func expandSpectralDNA(seeds: [Float]) -> [Float] {
        guard seeds.count >= 6 else {
            return Array(repeating: 0.5, count: 64)
        }

        var dna = [Float](repeating: 0, count: 64)
        let anchors = [0, 12, 24, 36, 48, 63] // Frequency anchor points

        for i in 0..<64 {
            // Find surrounding anchors
            var lowerIdx = 0
            for (idx, anchor) in anchors.enumerated() {
                if anchor <= i { lowerIdx = idx }
            }
            let upperIdx = min(lowerIdx + 1, anchors.count - 1)

            if lowerIdx == upperIdx {
                dna[i] = seeds[lowerIdx]
            } else {
                // Linear interpolation between anchor points
                let lowerAnchor = Float(anchors[lowerIdx])
                let upperAnchor = Float(anchors[upperIdx])
                let t = (Float(i) - lowerAnchor) / (upperAnchor - lowerAnchor)
                dna[i] = seeds[lowerIdx] * (1 - t) + seeds[upperIdx] * t
            }
        }

        return dna
    }

    // MARK: - Full Specimen from Seed

    /// Create a complete Specimen from a hash seed.
    /// This is the Monster Rancher moment: song → creature.
    static func createSpecimen(
        from seed: SpecimenSeed,
        trackTitle: String,
        trackArtist: String
    ) -> Specimen {
        guard let catalogEntry = SpecimenCatalog.entry(at: seed.subtypeIndex) else {
            // Fallback: Sawfin
            return SpecimenFactory.createStarter()
        }

        let specimenRarity = rarity(from: seed.rarityRoll)
        let cosmetic = cosmeticTier(from: seed.cosmeticRoll)
        let spectralDNA = expandSpectralDNA(seeds: seed.spectralSeeds)
        let displayName = catalogEntry.displayName(rarity: specimenRarity, morphIndex: seed.morphIndex)

        // Generate parameters based on catalog type + rarity + spectral seeds
        let params = generateParametersFromSeed(
            catalogEntry: catalogEntry,
            rarity: specimenRarity,
            spectralSeeds: seed.spectralSeeds
        )

        let bornEntry = JournalEntry(
            id: UUID(),
            timestamp: Date(),
            type: .born,
            description: "Born from \(trackArtist) — \(trackTitle)"
        )

        return Specimen(
            id: UUID(),
            name: displayName,
            category: catalogEntry.category,
            rarity: specimenRarity,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: catalogEntry.subtypeID,
            catchAccelPattern: Array(repeating: 0, count: 16),
            provenance: [],
            spectralDNA: spectralDNA,
            parameterState: params,
            catchLatitude: nil,
            catchLongitude: nil,
            catchTimestamp: Date(),
            catchWeatherDescription: "Music Library",
            creatureGenomeData: nil,
            cosmeticTier: cosmetic,
            morphIndex: seed.morphIndex,
            musicHash: seed.hashHex,
            sourceTrackTitle: "\(trackArtist) — \(trackTitle)",
            xp: 0,
            level: 1,
            aggressiveScore: 0,
            gentleScore: 0,
            totalPlaySeconds: 0,
            journal: [bornEntry]
        )
    }

    // MARK: - Parameter Generation from Seed

    /// Generate deterministic OBRIX parameters from catalog entry + spectral seeds.
    /// Uses seed values (not random) so the same song always produces the same sound.
    private static func generateParametersFromSeed(
        catalogEntry: CatalogEntry,
        rarity: SpecimenRarity,
        spectralSeeds: [Float]
    ) -> [String: Float] {
        var params: [String: Float] = [:]

        // Base envelope from seeds (deterministic, not random)
        params["obrix_env1Attack"]  = spectralSeeds[0] * 0.3
        params["obrix_env1Decay"]   = 0.05 + spectralSeeds[1] * 0.75
        params["obrix_env1Sustain"] = 0.2 + spectralSeeds[2] * 0.7
        params["obrix_env1Release"] = 0.1 + spectralSeeds[3] * 1.9

        switch catalogEntry.category {
        case .source:
            let srcTypeMap: [String: Float] = [
                "polyblep-saw": 0, "polyblep-square": 1, "polyblep-tri": 2,
                "noise-white": 3, "noise-pink": 3,
                "wt-analog": 4, "wt-vocal": 4,
                "fm-basic": 5
            ]
            params["obrix_src1Type"]  = srcTypeMap[catalogEntry.subtypeID] ?? 0
            params["obrix_src1Tune"]  = (spectralSeeds[4] - 0.5) * 24  // ±12 semitones
            params["obrix_src1Level"] = 0.5 + spectralSeeds[5] * 0.5

        case .processor:
            params["obrix_flt1Cutoff"]   = 0.1 + spectralSeeds[4] * 0.85
            params["obrix_flt1Resonance"] = spectralSeeds[5] * 0.8
            params["obrix_flt1EnvDepth"] = (spectralSeeds[0] - 0.5)

        case .modulator:
            params["obrix_lfo1Rate"]  = 0.01 + spectralSeeds[4] * 9.99
            params["obrix_lfo1Depth"] = 0.1 + spectralSeeds[5] * 0.7
            params["obrix_lfo1Shape"] = (spectralSeeds[0] * 3).rounded()

        case .effect:
            params["obrix_fx1Mix"]    = 0.2 + spectralSeeds[4] * 0.6
            params["obrix_fx1Param1"] = 0.1 + spectralSeeds[5] * 0.8
            params["obrix_fx1Param2"] = 0.1 + spectralSeeds[0] * 0.8
        }

        // Rarity pushes parameters toward extremes (same logic as SpecimenFactory)
        if rarity == .rare || rarity == .legendary {
            for key in params.keys {
                if let val = params[key], val >= 0.0, val <= 1.0, val > 0.3, val < 0.7 {
                    params[key] = val < 0.5 ? val * 0.5 : val + (1.0 - val) * 0.5
                }
            }
        }

        return params
    }
}
