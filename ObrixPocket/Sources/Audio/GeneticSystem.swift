import Foundation

/// The 12 inheritable sonic traits, each with dominant/recessive expression
enum SonicTrait: String, CaseIterable, Codable {
    case octaveRange          // Preferred octave register
    case waveformAffinity     // Tendency toward specific waveform type
    case rhythmicDensity      // How many notes per beat
    case harmonicPreference   // Major vs minor vs chromatic tendency
    case envelopeShape        // Attack/decay character
    case filterTendency       // Bright vs dark default
    case modulationDepth      // How much LFO/modulation
    case vibratoSpeed         // Rate of pitch modulation
    case stereoWidth          // Mono vs wide
    case reverbAffinity       // Dry vs wet default
    case attackCharacter      // Pluck vs pad vs percussive
    case sustainBehavior      // Short staccato vs long legato

    var displayName: String {
        switch self {
        case .octaveRange:        return "Octave Range"
        case .waveformAffinity:   return "Waveform"
        case .rhythmicDensity:    return "Rhythm"
        case .harmonicPreference: return "Harmony"
        case .envelopeShape:      return "Envelope"
        case .filterTendency:     return "Filter"
        case .modulationDepth:    return "Modulation"
        case .vibratoSpeed:       return "Vibrato"
        case .stereoWidth:        return "Width"
        case .reverbAffinity:     return "Space"
        case .attackCharacter:    return "Attack"
        case .sustainBehavior:    return "Sustain"
        }
    }

    /// Parameter IDs this trait maps to
    var parameterIDs: [String] {
        switch self {
        case .octaveRange:        return ["obrix_src1Tune"]
        case .waveformAffinity:   return ["obrix_src1Type"]
        case .rhythmicDensity:    return []  // Affects VoiceProfile, not params
        case .harmonicPreference: return []  // Affects HarmonicContext scale choice
        case .envelopeShape:      return ["obrix_env1Attack", "obrix_env1Decay"]
        case .filterTendency:     return ["obrix_flt1Cutoff"]
        case .modulationDepth:    return ["obrix_lfo1Depth"]
        case .vibratoSpeed:       return ["obrix_lfo1Rate"]
        case .stereoWidth:        return []  // Affects pan/spread
        case .reverbAffinity:     return ["obrix_fx1Mix"]
        case .attackCharacter:    return ["obrix_env1Attack"]
        case .sustainBehavior:    return ["obrix_env1Sustain", "obrix_env1Release"]
        }
    }
}

/// Expression of a single trait — dominant or recessive from each parent
enum TraitExpression: String, Codable {
    case dominant    // Trait is strongly expressed
    case recessive   // Trait is present but subdued
    case codominant  // Both parents' traits blend equally

    /// Value multiplier for this expression type
    var multiplier: Float {
        switch self {
        case .dominant:   return 1.0
        case .recessive:  return 0.4
        case .codominant: return 0.7
        }
    }
}

/// A single inherited gene — one trait with its expression and source
struct Gene: Codable, Identifiable {
    let id: UUID
    let trait: SonicTrait
    let expression: TraitExpression
    let value: Float              // 0-1 normalized value
    let sourceParent: String      // Parent subtype ID that contributed this gene
    let isMutated: Bool           // Whether this gene mutated during inheritance

    /// The effective value (value × expression multiplier)
    var effectiveValue: Float {
        value * expression.multiplier
    }
}

/// Complete genetic makeup of a specimen
struct SpecimenGenome: Codable, Identifiable {
    let id: UUID
    let subtypeID: String
    let specimenName: String
    let generation: Int           // Gen-1 = caught, Gen-2 = bred, etc.
    let genes: [Gene]             // 12 genes (one per SonicTrait)
    let parentGenomeIDs: [UUID]   // Empty for Gen-1, 2 entries for bred
    let birthDate: Date

    /// Mutation window: higher generations allow wider parameter tweaks
    var mutationWindow: Float {
        switch generation {
        case 1:     return 0.20   // ±20% (standard catch)
        case 2:     return 0.25   // ±25%
        case 3:     return 0.30   // ±30%
        case 4:     return 0.35   // ±35%
        default:    return 0.40   // ±40% (Gen-5+: profoundly unique)
        }
    }

    /// Count of mutated genes
    var mutationCount: Int {
        genes.filter { $0.isMutated }.count
    }

    /// Overall genetic uniqueness score (0-1). Higher = more unique.
    var uniquenessScore: Float {
        guard !genes.isEmpty else { return 0 }
        let mutationFactor = Float(mutationCount) / Float(genes.count)
        let generationFactor = min(1, Float(generation) / 5.0)
        let expressionVariety = Set(genes.map { $0.expression }).count
        let varietyFactor = Float(expressionVariety) / 3.0
        return (mutationFactor + generationFactor + varietyFactor) / 3.0
    }

    /// Get the gene for a specific trait
    func gene(for trait: SonicTrait) -> Gene? {
        genes.first { $0.trait == trait }
    }

    /// Get effective values as a parameter dictionary
    func asParameterModifiers() -> [String: Float] {
        var mods: [String: Float] = [:]
        for gene in genes {
            for paramID in gene.trait.parameterIDs {
                mods[paramID] = gene.effectiveValue
            }
        }
        return mods
    }
}

/// Manages genetic inheritance, generational tracking, and genome storage.
///
/// Gen-1 specimens (caught) get a default genome derived from their catalog entry.
/// Gen-2+ specimens (bred) get a blended genome from Mendelian inheritance.
/// Each generation allows wider parameter mutation windows.
/// Gen-5+ specimens are profoundly unique — no two sound alike.
final class GeneticManager: ObservableObject {

    // MARK: - State

    @Published var genomes: [UUID: SpecimenGenome] = [:]

    /// Discovery log: tracks every unique trait combination ever bred
    @Published var discoveryLog: [GeneticDiscovery] = []

    // MARK: - Genome Creation

    /// Create a Gen-1 genome for a freshly caught specimen
    func createCaughtGenome(subtypeID: String) -> SpecimenGenome {
        let profile = VoiceProfileCatalog.profile(for: subtypeID)

        // Derive genes from the specimen's voice profile
        let genes = SonicTrait.allCases.map { trait -> Gene in
            Gene(
                id: UUID(),
                trait: trait,
                expression: .dominant,  // Caught specimens express all traits dominantly
                value: defaultGeneValue(trait: trait, profile: profile),
                sourceParent: subtypeID,
                isMutated: false
            )
        }

        let genome = SpecimenGenome(
            id: UUID(),
            subtypeID: subtypeID,
            specimenName: profile.specimenName,
            generation: 1,
            genes: genes,
            parentGenomeIDs: [],
            birthDate: Date()
        )

        genomes[genome.id] = genome
        save()
        return genome
    }

    /// Create a Gen-N genome through breeding (Mendelian inheritance)
    func breedGenome(parentGenomeA: SpecimenGenome, parentGenomeB: SpecimenGenome,
                     offspringSubtypeID: String) -> SpecimenGenome {
        let offspringGen = max(parentGenomeA.generation, parentGenomeB.generation) + 1
        let profile = VoiceProfileCatalog.profile(for: offspringSubtypeID)

        let genes = SonicTrait.allCases.map { trait -> Gene in
            let geneA = parentGenomeA.gene(for: trait)
            let geneB = parentGenomeB.gene(for: trait)

            // Mendelian inheritance: randomly select dominant/recessive
            let fromA = Bool.random()
            let selectedGene = fromA ? geneA : geneB
            let otherGene = fromA ? geneB : geneA

            // Determine expression
            let expression: TraitExpression
            if let sel = selectedGene, let other = otherGene {
                if sel.expression == .dominant && other.expression == .dominant {
                    expression = .codominant  // Both parents dominant = blend
                } else if sel.expression == .dominant {
                    expression = .dominant
                } else {
                    expression = .recessive
                }
            } else {
                expression = .dominant
            }

            // Value blending with generation-based variation
            let baseValue = selectedGene?.value ?? defaultGeneValue(trait: trait, profile: profile)
            let variation = Float.random(in: -0.1...0.1) * Float(offspringGen) / 3.0
            let value = max(0, min(1, baseValue + variation))

            // Mutation check (5% per gene)
            let isMutated = Float.random(in: 0...1) < 0.05
            let finalValue = isMutated ? Float.random(in: 0.1...0.9) : value

            return Gene(
                id: UUID(),
                trait: trait,
                expression: expression,
                value: finalValue,
                sourceParent: fromA ? parentGenomeA.subtypeID : parentGenomeB.subtypeID,
                isMutated: isMutated
            )
        }

        let genome = SpecimenGenome(
            id: UUID(),
            subtypeID: offspringSubtypeID,
            specimenName: "\(profile.specimenName) Gen-\(offspringGen)",
            generation: offspringGen,
            genes: genes,
            parentGenomeIDs: [parentGenomeA.id, parentGenomeB.id],
            birthDate: Date()
        )

        genomes[genome.id] = genome

        // Log any unique trait combinations
        logDiscoveries(genome: genome)

        save()
        return genome
    }

    // MARK: - Queries

    /// Get genome by ID
    func genome(for id: UUID) -> SpecimenGenome? {
        genomes[id]
    }

    /// Find all genomes of a specific generation
    func genomesOfGeneration(_ gen: Int) -> [SpecimenGenome] {
        Array(genomes.values.filter { $0.generation == gen })
    }

    /// Highest generation in the collection
    var highestGeneration: Int {
        genomes.values.map { $0.generation }.max() ?? 1
    }

    /// Total unique trait combinations discovered
    var totalDiscoveries: Int {
        discoveryLog.count
    }

    // MARK: - Helpers

    private func defaultGeneValue(trait: SonicTrait, profile: VoiceProfile) -> Float {
        switch trait {
        case .octaveRange:        return Float(profile.rangeLow) / 108.0
        case .waveformAffinity:   return 0.5
        case .rhythmicDensity:    return profile.swing
        case .harmonicPreference: return 0.5
        case .envelopeShape:      return profile.attackSharpness
        case .filterTendency:     return 0.5
        case .modulationDepth:    return profile.velocityVariation
        case .vibratoSpeed:       return 0.3
        case .stereoWidth:        return 0.5
        case .reverbAffinity:     return 0.4
        case .attackCharacter:    return profile.attackSharpness
        case .sustainBehavior:    return profile.sustainLength
        }
    }

    private func logDiscoveries(genome: SpecimenGenome) {
        // Check for unique expression combinations
        let expressionPattern = genome.genes.map { $0.expression.rawValue }.joined(separator: "-")
        let isNew = !discoveryLog.contains { $0.expressionPattern == expressionPattern }

        if isNew {
            let discovery = GeneticDiscovery(
                id: UUID(),
                date: Date(),
                genomeID: genome.id,
                generation: genome.generation,
                expressionPattern: expressionPattern,
                mutationCount: genome.mutationCount,
                description: "New genetic combination in Gen-\(genome.generation)"
            )
            discoveryLog.append(discovery)
            if discoveryLog.count > 500 {
                discoveryLog = Array(discoveryLog.suffix(500))
            }
        }
    }

    // MARK: - Persistence

    func save() {
        if let data = try? JSONEncoder().encode(Array(genomes.values)) {
            UserDefaults.standard.set(data, forKey: "specimenGenomes")
        }
        if let data = try? JSONEncoder().encode(discoveryLog) {
            UserDefaults.standard.set(data, forKey: "geneticDiscoveries")
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: "specimenGenomes"),
           let decoded = try? JSONDecoder().decode([SpecimenGenome].self, from: data) {
            genomes = [:]
            for genome in decoded {
                genomes[genome.id] = genome
            }
        }
        if let data = UserDefaults.standard.data(forKey: "geneticDiscoveries"),
           let decoded = try? JSONDecoder().decode([GeneticDiscovery].self, from: data) {
            discoveryLog = decoded
        }
    }
}

/// Record of a unique genetic discovery
struct GeneticDiscovery: Codable, Identifiable {
    let id: UUID
    let date: Date
    let genomeID: UUID
    let generation: Int
    let expressionPattern: String
    let mutationCount: Int
    let description: String
}
