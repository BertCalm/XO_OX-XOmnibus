import Foundation

enum SpecimenFusion {

    /// Check if two specimens can fuse
    static func canFuse(_ a: Specimen, _ b: Specimen) -> Bool {
        // Must be same category
        guard a.category == b.category else { return false }
        // Both must be level 3+ (investment required)
        guard a.level >= 3 && b.level >= 3 else { return false }
        // Can't fuse with self
        guard a.id != b.id else { return false }
        return true
    }

    /// Create a fused specimen from two parents
    static func fuse(_ a: Specimen, _ b: Specimen) -> Specimen? {
        guard canFuse(a, b) else { return nil }

        // Rarity: higher of the two parents
        let rarity: SpecimenRarity = {
            let order: [SpecimenRarity] = [.common, .uncommon, .rare, .legendary]
            let aIdx = order.firstIndex(of: a.rarity) ?? 0
            let bIdx = order.firstIndex(of: b.rarity) ?? 0
            return order[max(aIdx, bIdx)]
        }()

        // Subtype: randomly pick one parent's subtype
        let subtype = Bool.random() ? a.subtype : b.subtype

        // Parameters: average of both parents
        var params: [String: Float] = [:]
        let allKeys = Set(a.parameterState.keys).union(b.parameterState.keys)
        for key in allKeys {
            let aVal = a.parameterState[key] ?? 0.5
            let bVal = b.parameterState[key] ?? 0.5
            params[key] = (aVal + bVal) / 2.0
        }

        // Spectral DNA: blend
        let dnaCount = max(a.spectralDNA.count, b.spectralDNA.count)
        var blendedDNA = [Float](repeating: 0.5, count: dnaCount)
        for i in 0..<dnaCount {
            let aVal = i < a.spectralDNA.count ? a.spectralDNA[i] : 0.5
            let bVal = i < b.spectralDNA.count ? b.spectralDNA[i] : 0.5
            // Weighted blend with slight randomness
            let weight = Float.random(in: 0.3...0.7)
            blendedDNA[i] = aVal * weight + bVal * (1 - weight)
        }

        // Name: "Parent1 × Parent2"
        let name = "\(a.creatureName) × \(b.creatureName)"

        // Level: average of parents, minimum 1
        let level = max(1, (a.level + b.level) / 2)

        let child = Specimen(
            id: UUID(),
            name: name,
            category: a.category, // Same category guaranteed
            rarity: rarity,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: subtype,
            catchAccelPattern: [],
            provenance: [],
            spectralDNA: blendedDNA,
            parameterState: params,
            catchLatitude: nil,
            catchLongitude: nil,
            catchTimestamp: Date(),
            catchWeatherDescription: "Fused",
            creatureGenomeData: nil,
            cosmeticTier: .standard,
            morphIndex: 0,
            musicHash: nil,
            sourceTrackTitle: nil,
            xp: 0,
            level: level,
            aggressiveScore: (a.aggressiveScore + b.aggressiveScore) / 2,
            gentleScore: (a.gentleScore + b.gentleScore) / 2,
            totalPlaySeconds: 0,
            journal: [
                JournalEntry(id: UUID(), timestamp: Date(), type: .born,
                           description: "Fused from \(a.creatureName) (Lv.\(a.level)) + \(b.creatureName) (Lv.\(b.level))")
            ],
            isFavorite: false
        )

        return child
    }
}
