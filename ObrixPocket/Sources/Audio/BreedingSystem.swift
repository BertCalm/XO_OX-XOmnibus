import Foundation

// MARK: - Manager References (injected at init; access via shared app environment)
// BreedingManager coordinates with GeneticManager, SoundMemoryManager,
// HarmonicProfileStore, NurseryManager, and SeasonalEventManager.
// In the full app these are injected; references below use a lightweight
// environment accessor pattern consistent with the rest of the codebase.

/// Status of a breeding attempt
enum BreedingStatus: String, Codable {
    case notEligible       // Pair doesn't meet requirements
    case eligible          // Ready to breed (7+ days wired, high affinity)
    case incubating        // Breeding in progress (in Nursery)
    case complete          // Offspring ready to move to reef
    case cooldown          // Recently bred, waiting 14 days
}

/// Record of a breeding pair's history
struct BreedingPair: Codable, Identifiable {
    let id: UUID
    let parentSlotA: Int
    let parentSlotB: Int
    let parentSubtypeA: String
    let parentSubtypeB: String
    let wiredDate: Date            // When they were first wired together
    var lastBreedingDate: Date?    // When they last produced offspring
    var offspringCount: Int = 0    // Total offspring from this pair

    /// Days the pair has been wired together
    var wiredDays: Int {
        Calendar.current.dateComponents([.day], from: wiredDate, to: Date()).day ?? 0
    }

    /// Whether the pair meets the minimum wiring duration (7 days)
    var meetsWiringRequirement: Bool {
        wiredDays >= 7
    }

    /// Whether the pair is in cooldown from recent breeding.
    ///
    /// Cooldown state is derived from `lastBreedingDate` (a `Date`) compared against
    /// `Date()` — it is purely computed and requires no separate persistence.
    ///
    /// Correctness depends on `BreedingManager.restore()` being called at app launch
    /// so that `lastBreedingDate` is populated from the persisted `pairs` array.
    /// Without that call the pairs array would be empty and all cooldowns would be lost.
    /// `restore()` is wired in `ObrixPocketApp.onAppear` for returning users (#378/#384).
    var isInCooldown: Bool {
        guard let lastBreeding = lastBreedingDate else { return false }
        let daysSince = Calendar.current.dateComponents([.day], from: lastBreeding, to: Date()).day ?? 0
        return daysSince < 14
    }

    /// Days remaining in cooldown (0 if not in cooldown)
    var cooldownDaysRemaining: Int {
        guard let lastBreeding = lastBreedingDate else { return 0 }
        let daysSince = Calendar.current.dateComponents([.day], from: lastBreeding, to: Date()).day ?? 0
        return max(0, 14 - daysSince)
    }

    /// Current breeding status
    var status: BreedingStatus {
        if isInCooldown { return .cooldown }
        if meetsWiringRequirement { return .eligible }
        return .notEligible
    }
}

/// A bred offspring waiting in the Nursery
struct NurseryOccupant: Codable, Identifiable {
    let id: UUID
    let parentSubtypeA: String
    let parentSubtypeB: String
    let parentNameA: String
    let parentNameB: String
    let conceptionDate: Date
    let formationEndDate: Date     // When the offspring is ready (24 hours later)
    var inheritedTraits: InheritedTraits
    let generation: Int            // Gen-2 if both parents are Gen-1, etc.

    /// Whether the formation period is complete
    var isReady: Bool {
        Date() >= formationEndDate
    }

    /// Progress through formation (0-1)
    var formationProgress: Float {
        let total = formationEndDate.timeIntervalSince(conceptionDate)
        let elapsed = Date().timeIntervalSince(conceptionDate)
        return Float(max(0, min(1, elapsed / total)))
    }

    /// Hours remaining until ready
    var hoursRemaining: Int {
        let remaining = formationEndDate.timeIntervalSince(Date())
        return max(0, Int(remaining / 3600))
    }
}

/// Traits inherited from parents during breeding
struct InheritedTraits: Codable {
    /// Which parent each trait came from (true = parentA, false = parentB)
    var traitSources: [String: Bool]

    /// The blended voice profile values
    var octavePreference: Int         // Inherited octave tendency
    var preferredIntervals: [Int]     // Blended from both parents
    var attackSharpness: Float        // Blended
    var sustainLength: Float          // Blended
    var swing: Float                  // Blended
    var velocityVariation: Float      // Blended

    /// Mutation flag: 5% chance of a completely unexpected trait
    var hasMutation: Bool
    var mutationDescription: String?  // e.g., "Anomalous harmonic overtone"

    /// The blended subtype ID for the offspring
    /// Offspring inherits the category of one parent (random) with traits of both
    var offspringSubtypeID: String
    var offspringCategory: String     // SpecimenCategory raw value
}

/// Manages the breeding system for OBRIX Pocket.
///
/// When two specimens are wired together with high affinity for 7+ days,
/// they become eligible to breed. Breeding produces a Gen-2+ offspring
/// that inherits blended traits from both parents.
///
/// Breeding IS sound design — you design new synth voices through
/// ecology, not knobs. Each breeding is unique.
///
/// Pipeline on `breed()`:
///   1. InheritedTraits blending (Mendelian voice profile blend)
///   2. Genome creation via GeneticManager.breedGenome
///   3. Sound memory inheritance via SoundMemoryManager.inheritMemory
///   4. Harmonic DNA derivation via HarmonicDNA + HarmonicProfileStore
///   5. Seasonal event notification via SeasonalEventManager.recordBreedingSuccess
///
/// Pipeline on `graduateOffspring()`:
///   6. Nursery musical influences applied to genome via NurseryManager.applyInfluences
///      (returns parameter offsets stored on the occupant's inheritedTraits)
final class BreedingManager: ObservableObject {

    // MARK: - State

    @Published var pairs: [BreedingPair] = []
    @Published var nursery: [NurseryOccupant] = []

    /// Maximum nursery capacity
    static let nurseryCapacity: Int = 3

    /// Minimum affinity strength for breeding eligibility
    static let affinityThreshold: Float = 0.5

    /// Mutation probability
    static let mutationChance: Float = 0.05

    // MARK: - Injected Managers
    // These are weak references to avoid retain cycles with ObservableObject graphs.
    // Callers must inject these before using breed() / graduateOffspring().

    weak var geneticManager: GeneticManager?
    weak var soundMemoryManager: SoundMemoryManager?
    weak var harmonicProfileStore: HarmonicProfileStore?
    weak var nurseryManager: NurseryManager?
    weak var seasonalEventManager: SeasonalEventManager?

    /// Specimen UUIDs for the current reef slots (needed to look up parent genomes by UUID).
    /// Key = slot index, value = specimen UUID. Set by the coordinator before breeding.
    var specimenUUIDs: [Int: UUID] = [:]

    // MARK: - Pair Management

    /// Register a new wiring between two specimens
    func registerWiring(slotA: Int, slotB: Int, subtypeA: String, subtypeB: String) {
        // Check if pair already exists
        if pairExists(slotA: slotA, slotB: slotB) { return }

        let pair = BreedingPair(
            id: UUID(),
            parentSlotA: slotA,
            parentSlotB: slotB,
            parentSubtypeA: subtypeA,
            parentSubtypeB: subtypeB,
            wiredDate: Date()
        )
        pairs.append(pair)
        save()
    }

    /// Remove a wiring (specimens unwired)
    func removeWiring(slotA: Int, slotB: Int) {
        pairs.removeAll { ($0.parentSlotA == slotA && $0.parentSlotB == slotB) ||
                           ($0.parentSlotA == slotB && $0.parentSlotB == slotA) }
        save()
    }

    /// Check if a pair already exists
    func pairExists(slotA: Int, slotB: Int) -> Bool {
        pairs.contains { ($0.parentSlotA == slotA && $0.parentSlotB == slotB) ||
                          ($0.parentSlotA == slotB && $0.parentSlotB == slotA) }
    }

    // MARK: - Breeding

    /// Get all eligible pairs (ready to breed)
    var eligiblePairs: [BreedingPair] {
        pairs.filter { $0.status == .eligible }
    }

    /// Attempt to breed a pair. Returns the nursery occupant if successful.
    ///
    /// Full pipeline (FIX 1–5):
    ///   1. InheritedTraits voice-profile blend (existing Mendelian logic)
    ///   2. Genome breeding via GeneticManager (FIX 1)
    ///   3. Sound memory inheritance via SoundMemoryManager (FIX 2)
    ///   4. Harmonic DNA derivation + store (FIX 5)
    ///   5. Seasonal event notification (FIX 3)
    func breed(pairID: UUID, affinityStrength: Float) -> NurseryOccupant? {
        guard let index = pairs.firstIndex(where: { $0.id == pairID }) else { return nil }
        let pair = pairs[index]

        // Check eligibility
        guard pair.status == .eligible else { return nil }
        guard affinityStrength >= Self.affinityThreshold else { return nil }
        guard nursery.count < Self.nurseryCapacity else { return nil }

        // Create offspring traits (existing Mendelian voice-profile blend)
        let traits = blendTraits(parentA: pair.parentSubtypeA, parentB: pair.parentSubtypeB)

        let profileA = VoiceProfileCatalog.profile(for: pair.parentSubtypeA)
        let profileB = VoiceProfileCatalog.profile(for: pair.parentSubtypeB)

        // Determine generation from parent genomes if available, otherwise default Gen-2
        let genomeA = geneticManager.flatMap { gm -> SpecimenGenome? in
            guard let idA = specimenUUIDs[pair.parentSlotA] else { return nil }
            return gm.genome(for: idA) ?? {
                let g = gm.createCaughtGenome(subtypeID: pair.parentSubtypeA)
                return g
            }()
        }
        let genomeB = geneticManager.flatMap { gm -> SpecimenGenome? in
            guard let idB = specimenUUIDs[pair.parentSlotB] else { return nil }
            return gm.genome(for: idB) ?? {
                let g = gm.createCaughtGenome(subtypeID: pair.parentSubtypeB)
                return g
            }()
        }

        let gen: Int
        if let gA = genomeA, let gB = genomeB {
            gen = max(gA.generation, gB.generation) + 1
        } else {
            gen = 2  // Default Gen-2 when genomes unavailable
        }

        let occupantID = UUID()
        let occupant = NurseryOccupant(
            id: occupantID,
            parentSubtypeA: pair.parentSubtypeA,
            parentSubtypeB: pair.parentSubtypeB,
            parentNameA: profileA.specimenName,
            parentNameB: profileB.specimenName,
            conceptionDate: Date(),
            formationEndDate: Date().addingTimeInterval(24 * 3600),  // 24 hours
            inheritedTraits: traits,
            generation: gen
        )

        nursery.append(occupant)

        // Update pair
        pairs[index].lastBreedingDate = Date()
        pairs[index].offspringCount += 1

        // FIX 1: Wire genetics — breed a new genome from both parent genomes
        if let gm = geneticManager, let gA = genomeA, let gB = genomeB {
            let offspringGenome = gm.breedGenome(
                parentGenomeA: gA,
                parentGenomeB: gB,
                offspringSubtypeID: traits.offspringSubtypeID
            )
            // offspringGenome is already stored inside breedGenome via gm.genomes[id] = genome
            // We associate it with the occupant ID so graduation can look it up by occupant.id

            // FIX 5: Build harmonic DNA from the new offspring genome
            if let store = harmonicProfileStore {
                // profile(for:) both derives and caches — call it now to pre-warm the cache
                _ = store.profile(for: offspringGenome)
                // Store under the occupant UUID so SpecimenAudioBridge can retrieve it
                // by specimen UUID after graduation.
                // (The genome's own UUID is stored inside GeneticManager.genomes;
                //  we also maintain a nursery→genome mapping for the graduation step.)
                nurseryGenomeIDs[occupantID] = offspringGenome.id
            }
        }

        // FIX 2: Inherit 30% of each parent's musical memory
        if let smm = soundMemoryManager,
           let idA = specimenUUIDs[pair.parentSlotA],
           let idB = specimenUUIDs[pair.parentSlotB] {
            smm.inheritMemory(specimenId: occupantID, parentA: idA, parentB: idB)
        }

        // FIX 3: Notify seasonal event manager of a successful breeding
        seasonalEventManager?.recordBreedingSuccess()

        // Persist immediately after a successful breed so that `lastBreedingDate`
        // (which drives cooldown) survives a force-quit between breed() and the
        // next .background save.  This is the defence-in-depth complement to
        // restore() being called at launch (#384).
        save()
        return occupant
    }

    /// Maps nursery occupant IDs → offspring genome IDs so graduation can wire them up.
    /// Persisted alongside nursery occupants so that an app kill mid-formation
    /// does not lose the genome association (FIX 5).
    private(set) var nurseryGenomeIDs: [UUID: UUID] = [:]

    /// Check if any nursery occupants are ready to graduate
    var readyOffspring: [NurseryOccupant] {
        nursery.filter { $0.isReady }
    }

    /// Graduate an offspring from the nursery (player places it on the reef).
    ///
    /// FIX 4: Applies nursery musical influences to the offspring's genome before
    /// returning the occupant. The caller places the graduated specimen on the reef
    /// using the occupant's `id` as the specimen UUID.
    ///
    /// Returns the graduated NurseryOccupant, or nil if not ready / not found.
    func graduateOffspring(id: UUID) -> NurseryOccupant? {
        guard let index = nursery.firstIndex(where: { $0.id == id && $0.isReady }) else { return nil }
        let graduated = nursery.remove(at: index)

        // FIX 4: Apply nursery musical influences to the offspring's genome.
        // applyInfluences returns a [String: Float] offset dictionary that callers
        // route to the audio engine on top of the base genome parameters.
        // We also store the result as a side-channel on the genome store
        // so SpecimenAudioBridge can pick it up without a separate lookup.
        if let nm = nurseryManager,
           let gm = geneticManager,
           let offspringGenomeID = nurseryGenomeIDs[id],
           let offspringGenome = gm.genome(for: offspringGenomeID) {
            let nurseryMods = nm.applyInfluences(to: offspringGenome, occupantID: id)
            // nurseryMods is a [String: Float] of parameter offsets from formation.
            // Persist them so SpecimenAudioBridge can blend them in at play time.
            nurseryParameterOffsets[id] = nurseryMods

            // FIX 3: Register the offspring genome under the graduated specimen's UUID
            // (the occupant.id becomes the specimen.id on the reef) so that
            // SpecimenAudioBridge.effectiveParameters can look up the genome by
            // specimen UUID rather than requiring the caller to know genome.id.
            gm.storeForSpecimen(genome: offspringGenome, specimenId: id)

            // Invalidate the harmonic profile cache — nursery exposure may have
            // shifted the genome's effective harmonic character if influences included
            // a harmonicPreference override. Re-derive on next access.
            harmonicProfileStore?.invalidate(genomeID: offspringGenomeID)

            // Clean up nursery influence data now that graduation is complete
            nm.clearInfluences(occupantID: id)
        }

        // Clean up the nursery→genome mapping now that the occupant has graduated
        nurseryGenomeIDs.removeValue(forKey: id)

        save()
        return graduated
    }

    /// Nursery parameter offsets from formation exposure, keyed by specimen UUID.
    /// Callers (e.g. SpecimenAudioBridge) blend these additive offsets on top of
    /// the base genome parameters when producing effective audio parameters.
    /// Persisted in UserDefaults alongside the rest of the breeding state.
    private(set) var nurseryParameterOffsets: [UUID: [String: Float]] = [:]

    /// Returns the nursery parameter offsets accumulated during formation for a given
    /// graduated specimen. Returns an empty dictionary if none recorded.
    func nurseryOffsets(for specimenId: UUID) -> [String: Float] {
        nurseryParameterOffsets[specimenId] ?? [:]
    }

    // MARK: - Trait Blending

    private func blendTraits(parentA: String, parentB: String) -> InheritedTraits {
        let profileA = VoiceProfileCatalog.profile(for: parentA)
        let profileB = VoiceProfileCatalog.profile(for: parentB)
        let roleA = SpecimenRoleMap.role(for: parentA)
        let roleB = SpecimenRoleMap.role(for: parentB)

        // Each trait randomly comes from one parent (Mendelian)
        var sources: [String: Bool] = [:]

        // Octave: inherit from one parent
        let octaveFromA = Bool.random()
        sources["octave"] = octaveFromA
        let octave = octaveFromA ? profileA.rangeLow / 12 : profileB.rangeLow / 12

        // Intervals: blend both parents' preferences
        var intervals = Array(Set(profileA.preferredIntervals + profileB.preferredIntervals))
        intervals.sort()
        sources["intervals"] = Bool.random()

        // Attack: weighted average with random bias
        let attackBias = Float.random(in: 0.3...0.7)
        let attack = profileA.attackSharpness * attackBias + profileB.attackSharpness * (1 - attackBias)
        sources["attack"] = attackBias > 0.5

        // Sustain: weighted average
        let sustainBias = Float.random(in: 0.3...0.7)
        let sustain = profileA.sustainLength * sustainBias + profileB.sustainLength * (1 - sustainBias)
        sources["sustain"] = sustainBias > 0.5

        // Swing: inherit from one parent
        let swingFromA = Bool.random()
        sources["swing"] = swingFromA
        let swing = swingFromA ? profileA.swing : profileB.swing

        // Velocity variation: average
        let velVar = (profileA.velocityVariation + profileB.velocityVariation) / 2
        sources["velocity"] = Bool.random()

        // Mutation check
        let hasMutation = Float.random(in: 0...1) < Self.mutationChance
        let mutationDesc: String? = hasMutation ? generateMutationDescription() : nil

        // Offspring inherits category from random parent
        let categoryFromA = Bool.random()
        let offspringSubtype = categoryFromA ? parentA : parentB
        let offspringCategory = categoryFromA ? "\(roleA)" : "\(roleB)"

        return InheritedTraits(
            traitSources: sources,
            octavePreference: octave,
            preferredIntervals: intervals,
            attackSharpness: attack,
            sustainLength: sustain,
            swing: swing,
            velocityVariation: velVar,
            hasMutation: hasMutation,
            mutationDescription: mutationDesc,
            offspringSubtypeID: offspringSubtype,
            offspringCategory: offspringCategory
        )
    }

    private func generateMutationDescription() -> String {
        let mutations = [
            "Anomalous harmonic overtone emerged",
            "Unusual rhythmic displacement pattern",
            "Inverted envelope characteristics",
            "Cross-spectrum resonance affinity",
            "Spectral drift in upper partials",
            "Sub-harmonic reinforcement tendency",
            "Phase-locked vibrato synchronization",
            "Micro-tonal pitch bias detected",
        ]
        return mutations.randomElement() ?? "Unknown mutation"
    }

    // MARK: - Queries

    /// Get breeding status for a specific pair of slots
    func statusForPair(slotA: Int, slotB: Int) -> BreedingStatus {
        guard let pair = pairs.first(where: {
            ($0.parentSlotA == slotA && $0.parentSlotB == slotB) ||
            ($0.parentSlotA == slotB && $0.parentSlotB == slotA)
        }) else {
            return .notEligible
        }
        return pair.status
    }

    /// Days until a pair becomes eligible (0 if already eligible)
    func daysUntilEligible(slotA: Int, slotB: Int) -> Int {
        guard let pair = pairs.first(where: {
            ($0.parentSlotA == slotA && $0.parentSlotB == slotB) ||
            ($0.parentSlotA == slotB && $0.parentSlotB == slotA)
        }) else {
            return 7  // Not even wired yet
        }
        return max(0, 7 - pair.wiredDays)
    }

    // MARK: - Persistence

    func save() {
        if let pairData = try? JSONEncoder().encode(pairs) {
            UserDefaults.standard.set(pairData, forKey: "breedingPairs")
        }
        if let nurseryData = try? JSONEncoder().encode(nursery) {
            UserDefaults.standard.set(nurseryData, forKey: "nurseryOccupants")
        }
        // Persist nursery parameter offsets so they survive app kill during formation
        if let offsetData = try? JSONEncoder().encode(nurseryParameterOffsets) {
            UserDefaults.standard.set(offsetData, forKey: "nurseryParameterOffsets")
        }
        // FIX 5: Persist nurseryGenomeIDs so graduation can still locate the correct
        // genome after an app restart that occurs mid-formation.
        if let genomeIDData = try? JSONEncoder().encode(nurseryGenomeIDs) {
            UserDefaults.standard.set(genomeIDData, forKey: "nurseryGenomeIDs")
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: "breedingPairs"),
           let decoded = try? JSONDecoder().decode([BreedingPair].self, from: data) {
            pairs = decoded
        }
        if let data = UserDefaults.standard.data(forKey: "nurseryOccupants"),
           let decoded = try? JSONDecoder().decode([NurseryOccupant].self, from: data) {
            nursery = decoded
        }
        if let data = UserDefaults.standard.data(forKey: "nurseryParameterOffsets"),
           let decoded = try? JSONDecoder().decode([UUID: [String: Float]].self, from: data) {
            nurseryParameterOffsets = decoded
        }
        // FIX 5: Restore nurseryGenomeIDs so in-flight nursery occupants can still
        // graduate correctly after an app kill during formation.
        if let data = UserDefaults.standard.data(forKey: "nurseryGenomeIDs"),
           let decoded = try? JSONDecoder().decode([UUID: UUID].self, from: data) {
            nurseryGenomeIDs = decoded
        }
    }
}
