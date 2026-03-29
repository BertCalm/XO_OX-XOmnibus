import Foundation

/// Prediction of possible offspring traits before breeding
struct BreedingPrediction {
    let parentNameA: String
    let parentNameB: String

    /// Predicted trait ranges (each trait shows min-max possible values)
    let traitPredictions: [TraitPrediction]

    /// Estimated mutation probability
    let mutationProbability: Float

    /// Predicted generation of offspring
    let predictedGeneration: Int

    /// Confidence level (how accurate these predictions are)
    let confidence: Float  // 0.6-0.9 (never 100% — surprises happen)

    /// Summary text for UI
    var summary: String {
        let mutationRisk = mutationProbability > 0.1 ? " (mutation possible!)" : ""
        return "Gen-\(predictedGeneration) offspring\(mutationRisk)"
    }
}

/// Prediction for a single trait
struct TraitPrediction {
    let trait: SonicTrait
    let minValue: Float
    let maxValue: Float
    let likelyExpression: TraitExpression
    let fromParent: String  // Which parent this trait likely comes from

    /// Range width — wider = less predictable
    var uncertainty: Float {
        maxValue - minValue
    }

    var displayRange: String {
        let low = Int(minValue * 100)
        let high = Int(maxValue * 100)
        return "\(low)%-\(high)%"
    }
}

/// Influence event during the nursery formation period
struct NurseryInfluence: Codable, Identifiable {
    let id: UUID
    let timestamp: Date
    let type: InfluenceType
    let strength: Float           // 0-1, how strongly this influenced the offspring

    enum InfluenceType: String, Codable {
        case musicPlayed          // Player played music near nursery
        case reefAmbient          // Reef heartbeat was active during formation
        case weatherEvent         // A weather event occurred during formation
        case divePerformed        // Player did a Dive during formation
        case specimenInteraction  // Player interacted with another specimen
    }
}

/// Manages the Nursery — the 24-hour formation period for bred offspring.
///
/// During formation, the player can influence the offspring by playing
/// music near the Nursery. The music the player plays shapes the
/// offspring's voice — the Nursery is the most powerful sound design
/// tool in the game.
///
/// Design (The Chemist): "You're literally SHAPING a new voice by
/// what you play near it."
final class NurseryManager: ObservableObject {

    // MARK: - State

    /// Active influences recorded during current formation(s)
    @Published var influences: [UUID: [NurseryInfluence]] = [:]  // occupantID → influences

    /// Accumulated musical characteristics from player's nursery activity
    private var musicalImprints: [UUID: MusicalImprint] = [:]

    // MARK: - Predictions

    /// Generate a breeding prediction for a potential pair
    func predict(parentA: SpecimenGenome, parentB: SpecimenGenome) -> BreedingPrediction {
        let predictions = SonicTrait.allCases.map { trait -> TraitPrediction in
            let geneA = parentA.gene(for: trait)
            let geneB = parentB.gene(for: trait)

            let valA = geneA?.value ?? 0.5
            let valB = geneB?.value ?? 0.5

            // Prediction range: blend ± uncertainty
            let mid = (valA + valB) / 2
            let spread = abs(valA - valB) / 2 + 0.1  // Minimum uncertainty of 0.1
            let minVal = Swift.max(0, mid - spread)
            let maxVal = Swift.min(1, mid + spread)

            // Likely expression based on parent expressions
            let exprA = geneA?.expression ?? .dominant
            let exprB = geneB?.expression ?? .dominant
            let likelyExpr: TraitExpression
            if exprA == .dominant && exprB == .dominant {
                likelyExpr = .codominant
            } else if exprA == .dominant || exprB == .dominant {
                likelyExpr = .dominant
            } else {
                likelyExpr = .recessive
            }

            let fromParent = valA > valB ? parentA.specimenName : parentB.specimenName

            return TraitPrediction(
                trait: trait,
                minValue: minVal,
                maxValue: maxVal,
                likelyExpression: likelyExpr,
                fromParent: fromParent
            )
        }

        let gen = Swift.max(parentA.generation, parentB.generation) + 1

        // Mutation probability increases slightly with generation
        let baseMutation: Float = 0.05
        let genBonus = Float(gen - 2) * 0.02
        let mutProb = Swift.min(0.25, baseMutation + genBonus)

        // Confidence decreases with generation (more variables)
        let confidence = Swift.max(0.6, 0.9 - Float(gen - 2) * 0.05)

        return BreedingPrediction(
            parentNameA: parentA.specimenName,
            parentNameB: parentB.specimenName,
            traitPredictions: predictions,
            mutationProbability: mutProb,
            predictedGeneration: gen,
            confidence: confidence
        )
    }

    // MARK: - Nursery Influence

    /// Record that the player played music near the nursery
    func recordMusicPlayed(occupantID: UUID, intensity: Float) {
        addInfluence(occupantID: occupantID,
                     type: .musicPlayed,
                     strength: Swift.min(1, intensity * 0.8))
    }

    /// Record that the reef ambient heartbeat was active
    func recordReefAmbient(occupantID: UUID) {
        addInfluence(occupantID: occupantID,
                     type: .reefAmbient,
                     strength: 0.3)  // Passive influence
    }

    /// Record a weather event during formation
    func recordWeatherEvent(occupantID: UUID, eventStrength: Float) {
        addInfluence(occupantID: occupantID,
                     type: .weatherEvent,
                     strength: eventStrength)
    }

    /// Record a Dive performed during formation
    func recordDive(occupantID: UUID, diveScore: Float) {
        addInfluence(occupantID: occupantID,
                     type: .divePerformed,
                     strength: diveScore / 100.0)
    }

    /// Record interaction with another specimen
    func recordSpecimenInteraction(occupantID: UUID) {
        addInfluence(occupantID: occupantID,
                     type: .specimenInteraction,
                     strength: 0.4)
    }

    private func addInfluence(occupantID: UUID, type: NurseryInfluence.InfluenceType, strength: Float) {
        let influence = NurseryInfluence(
            id: UUID(),
            timestamp: Date(),
            type: type,
            strength: strength
        )
        var list = influences[occupantID] ?? []
        list.append(influence)
        influences[occupantID] = list
    }

    // MARK: - Musical Imprint

    /// Record the musical characteristics of what was played near the nursery.
    /// This shapes the offspring's taste.
    func recordMusicalImprint(occupantID: UUID, key: Int, scale: String,
                               averageVelocity: Float, noteDensity: Float) {
        var imprint = musicalImprints[occupantID] ?? MusicalImprint()
        imprint.exposureCount += 1
        imprint.totalVelocity += averageVelocity
        imprint.totalDensity += noteDensity
        imprint.keysExposed.insert(key)
        imprint.scalesExposed.insert(scale)
        musicalImprints[occupantID] = imprint
    }

    /// Get the accumulated musical imprint for an offspring
    func imprint(for occupantID: UUID) -> MusicalImprint {
        musicalImprints[occupantID] ?? MusicalImprint()
    }

    /// Apply nursery influences to modify the offspring's genetic traits.
    /// Called when the offspring graduates from the nursery.
    func applyInfluences(to genome: SpecimenGenome, occupantID: UUID) -> [String: Float] {
        let influences = self.influences[occupantID] ?? []
        let imprint = self.imprint(for: occupantID)

        var modifiers: [String: Float] = [:]

        // Music played = influence on filter/brightness
        let musicInfluences = influences.filter { $0.type == .musicPlayed }
        if !musicInfluences.isEmpty {
            let avgStrength = musicInfluences.reduce(0) { $0 + $1.strength } / Float(musicInfluences.count)
            modifiers["obrix_flt1Cutoff"] = avgStrength * 0.1  // Offset
        }

        // Imprint affects preferred velocity and density
        if imprint.exposureCount > 0 {
            let avgVel = imprint.totalVelocity / Float(imprint.exposureCount)
            modifiers["velocity_bias"] = avgVel

            let avgDensity = imprint.totalDensity / Float(imprint.exposureCount)
            modifiers["density_bias"] = avgDensity
        }

        // Weather events during formation affect modulation depth
        let weatherInfluences = influences.filter { $0.type == .weatherEvent }
        if !weatherInfluences.isEmpty {
            let maxStrength = weatherInfluences.map { $0.strength }.max() ?? 0
            modifiers["obrix_lfo1Depth"] = maxStrength * 0.15
        }

        return modifiers
    }

    // MARK: - Cleanup

    /// Remove influence data for a graduated or abandoned offspring
    func clearInfluences(occupantID: UUID) {
        influences.removeValue(forKey: occupantID)
        musicalImprints.removeValue(forKey: occupantID)
    }

    // MARK: - Persistence

    func save() {
        // Influences are transient — only persist active occupant data
        if let data = try? JSONEncoder().encode(influences) {
            UserDefaults.standard.set(data, forKey: "nurseryInfluences")
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: "nurseryInfluences"),
           let decoded = try? JSONDecoder().decode([UUID: [NurseryInfluence]].self, from: data) {
            influences = decoded
        }
    }
}

/// Accumulated musical characteristics from nursery exposure
struct MusicalImprint: Codable {
    var exposureCount: Int = 0
    var totalVelocity: Float = 0      // Sum of average velocities per exposure
    var totalDensity: Float = 0       // Sum of note densities per exposure
    var keysExposed: Set<Int> = []    // Which musical keys the offspring heard
    var scalesExposed: Set<String> = [] // Which scales the offspring heard

    /// Average velocity the offspring was exposed to
    var averageVelocity: Float {
        guard exposureCount > 0 else { return 0.5 }
        return totalVelocity / Float(exposureCount)
    }

    /// Average note density the offspring was exposed to
    var averageDensity: Float {
        guard exposureCount > 0 else { return 0.5 }
        return totalDensity / Float(exposureCount)
    }

    /// How diverse the musical exposure was (0-1)
    var diversity: Float {
        let keyDiversity = Float(keysExposed.count) / 12.0
        let scaleDiversity = Float(scalesExposed.count) / 9.0
        return (keyDiversity + scaleDiversity) / 2.0
    }
}
