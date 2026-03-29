import Foundation

/// The chemical relationship between two specimens
enum ChemistryType: String, Codable {
    case affinity    // They harmonize — consonance, complementary
    case tension     // They clash — dissonance, competing for sonic space
    case neutral     // No special relationship
    case symbiosis   // One feeds the other (modulator → source)
    case resonance   // They amplify each other's characteristics
}

/// A chemical bond between two specimen slots
struct ChemistryBond: Codable, Identifiable {
    let id: UUID
    let slotA: Int
    let slotB: Int
    let type: ChemistryType
    let strength: Float       // 0-1, how strong the interaction is
    let gridDistance: Int      // Manhattan distance on the 4×4 grid

    /// Interaction multiplier based on distance (closer = stronger)
    var distanceMultiplier: Float {
        switch gridDistance {
        case 1:  return 1.0    // Adjacent — full interaction
        case 2:  return 0.6    // One apart — moderate
        case 3:  return 0.3    // Two apart — weak
        default: return 0.15   // Far apart — minimal
        }
    }

    /// Effective strength (base strength × distance)
    var effectiveStrength: Float {
        strength * distanceMultiplier
    }
}

/// Chemistry rules defining which specimen categories interact
enum ChemistryRules {

    /// Determine the chemistry type between two specimens
    static func chemistry(categoryA: SpecimenCategory, roleA: MusicalRole,
                          categoryB: SpecimenCategory, roleB: MusicalRole) -> ChemistryType {
        // Same role = tension (competing for sonic space)
        if roleA == roleB {
            return .tension
        }

        // Source + Processor = symbiosis (natural pairing)
        if (categoryA == .source && categoryB == .processor) ||
           (categoryA == .processor && categoryB == .source) {
            return .symbiosis
        }

        // Source + Modulator = resonance (modulator amplifies source character)
        if (categoryA == .source && categoryB == .modulator) ||
           (categoryA == .modulator && categoryB == .source) {
            return .resonance
        }

        // Bass + Melody = affinity (classic harmonic pairing)
        if (roleA == .bass && roleB == .melody) || (roleA == .melody && roleB == .bass) {
            return .affinity
        }

        // Harmony + anything melodic = affinity
        if (roleA == .harmony && roleB.generatesNotes) || (roleB == .harmony && roleA.generatesNotes) {
            return .affinity
        }

        // Rhythm + Bass = affinity (groove section)
        if (roleA == .rhythm && roleB == .bass) || (roleA == .bass && roleB == .rhythm) {
            return .affinity
        }

        // Effect + Effect = tension (too much processing)
        if categoryA == .effect && categoryB == .effect {
            return .tension
        }

        // Texture + Texture = neutral (they coexist peacefully)
        if roleA == .texture && roleB == .texture {
            return .neutral
        }

        return .neutral
    }

    /// Base interaction strength for a chemistry type
    static func baseStrength(for type: ChemistryType) -> Float {
        switch type {
        case .affinity:  return 0.8
        case .tension:   return 0.7
        case .neutral:   return 0.3
        case .symbiosis: return 0.9
        case .resonance: return 0.85
        }
    }

    /// Calculate Manhattan distance between two slots on a 4×4 grid
    static func gridDistance(slotA: Int, slotB: Int) -> Int {
        let rowA = slotA / 4, colA = slotA % 4
        let rowB = slotB / 4, colB = slotB % 4
        return abs(rowA - rowB) + abs(colA - colB)
    }
}

/// Sonic effects produced by different chemistry types
struct ChemistryEffect: Codable {
    /// Filter cutoff modifier (applied to both specimens)
    let filterMod: Float

    /// Detune amount between the pair (creates beating/chorus)
    let detuneCents: Float

    /// Velocity correlation (positive = they accent together, negative = alternate)
    let velocityCorrelation: Float

    /// Rhythmic sync tendency (1 = lock to same grid, 0 = independent, -1 = syncopate against)
    let rhythmSync: Float

    /// Reverb/space send modifier
    let spaceMod: Float

    static func forType(_ type: ChemistryType, strength: Float) -> ChemistryEffect {
        switch type {
        case .affinity:
            return ChemistryEffect(
                filterMod: 0.05 * strength,        // Slightly brighter together
                detuneCents: 0,                      // No beating — consonance
                velocityCorrelation: 0.7 * strength, // Accent together
                rhythmSync: 0.5 * strength,          // Tend to lock rhythms
                spaceMod: 0.1 * strength             // Slightly more reverb (blending)
            )
        case .tension:
            return ChemistryEffect(
                filterMod: -0.1 * strength,        // Slightly darker (competing)
                detuneCents: 5 * strength,           // Slight beating — dissonance
                velocityCorrelation: -0.3 * strength, // Alternate accents
                rhythmSync: -0.4 * strength,         // Syncopate against each other
                spaceMod: -0.05 * strength           // Drier (in-your-face clash)
            )
        case .neutral:
            return ChemistryEffect(
                filterMod: 0, detuneCents: 0,
                velocityCorrelation: 0, rhythmSync: 0, spaceMod: 0
            )
        case .symbiosis:
            return ChemistryEffect(
                filterMod: 0.1 * strength,         // Brighter (processor opens source)
                detuneCents: 0,
                velocityCorrelation: 0.9 * strength, // Tightly coupled dynamics
                rhythmSync: 0.8 * strength,          // Rhythmically locked
                spaceMod: 0.05 * strength
            )
        case .resonance:
            return ChemistryEffect(
                filterMod: 0.08 * strength,
                detuneCents: 2 * strength,           // Slight chorus (amplified character)
                velocityCorrelation: 0.5 * strength,
                rhythmSync: 0.3 * strength,
                spaceMod: 0.15 * strength            // More space (resonance needs room)
            )
        }
    }
}

/// Manages chemical relationships across the entire reef.
///
/// When specimens are placed on the reef grid, their positions and
/// categories determine chemistry bonds. These bonds affect how
/// specimens sound together — affinity pairs harmonize, tension
/// pairs create dissonance, symbiotic pairs feed each other.
///
/// Design: "Tension isn't BAD — it's dissonance. Let the player
/// choose consonance vs. dissonance by placement. Jazz players
/// live in the tension."
final class ReefChemistryManager: ObservableObject {

    // MARK: - State

    @Published var bonds: [ChemistryBond] = []

    /// Cached effects per slot (aggregated from all bonds involving that slot)
    private var slotEffects: [Int: ChemistryEffect] = [:]

    // MARK: - Configuration

    /// Recalculate all chemistry bonds for the current reef layout.
    /// Call when specimens are added, removed, or moved.
    func recalculate(slots: [(index: Int, subtypeID: String, category: SpecimenCategory)]) {
        bonds.removeAll()
        slotEffects.removeAll()

        // Calculate bonds between every pair of occupied slots
        for i in 0..<slots.count {
            for j in (i+1)..<slots.count {
                let slotA = slots[i]
                let slotB = slots[j]

                let roleA = SpecimenRoleMap.role(for: slotA.subtypeID)
                let roleB = SpecimenRoleMap.role(for: slotB.subtypeID)

                let type = ChemistryRules.chemistry(
                    categoryA: slotA.category, roleA: roleA,
                    categoryB: slotB.category, roleB: roleB
                )

                let distance = ChemistryRules.gridDistance(slotA: slotA.index, slotB: slotB.index)
                let strength = ChemistryRules.baseStrength(for: type)

                let bond = ChemistryBond(
                    id: UUID(),
                    slotA: slotA.index,
                    slotB: slotB.index,
                    type: type,
                    strength: strength,
                    gridDistance: distance
                )

                bonds.append(bond)
            }
        }

        // Cache aggregated effects per slot
        for slot in slots {
            let slotBonds = bonds.filter { $0.slotA == slot.index || $0.slotB == slot.index }
            slotEffects[slot.index] = aggregateEffects(bonds: slotBonds)
        }
    }

    // MARK: - Queries

    /// Get the aggregated chemistry effect for a slot
    func effect(for slotIndex: Int) -> ChemistryEffect {
        slotEffects[slotIndex] ?? ChemistryEffect(filterMod: 0, detuneCents: 0,
                                                    velocityCorrelation: 0,
                                                    rhythmSync: 0, spaceMod: 0)
    }

    /// Get all bonds involving a specific slot
    func bonds(for slotIndex: Int) -> [ChemistryBond] {
        bonds.filter { $0.slotA == slotIndex || $0.slotB == slotIndex }
    }

    /// Get the strongest bond for a slot (for UI highlight)
    func strongestBond(for slotIndex: Int) -> ChemistryBond? {
        bonds(for: slotIndex).max(by: { $0.effectiveStrength < $1.effectiveStrength })
    }

    /// Overall reef harmony score (0-1). Higher = more affinity, lower = more tension.
    var harmonyScore: Float {
        guard !bonds.isEmpty else { return 0.5 }
        let affinityStrength = bonds.filter { $0.type == .affinity || $0.type == .symbiosis || $0.type == .resonance }
            .reduce(0) { $0 + $1.effectiveStrength }
        let tensionStrength = bonds.filter { $0.type == .tension }
            .reduce(0) { $0 + $1.effectiveStrength }
        let total = affinityStrength + tensionStrength
        guard total > 0 else { return 0.5 }
        return affinityStrength / total
    }

    // MARK: - Visualization Data

    /// Get particle flow data for affinity bonds (gentle flow between specimens)
    var affinityFlows: [(from: Int, to: Int, strength: Float)] {
        bonds.filter { $0.type == .affinity || $0.type == .symbiosis || $0.type == .resonance }
            .map { ($0.slotA, $0.slotB, $0.effectiveStrength) }
    }

    /// Get spark data for tension bonds (jagged sparks between specimens)
    var tensionSparks: [(from: Int, to: Int, strength: Float)] {
        bonds.filter { $0.type == .tension }
            .map { ($0.slotA, $0.slotB, $0.effectiveStrength) }
    }

    // MARK: - Helpers

    private func aggregateEffects(bonds: [ChemistryBond]) -> ChemistryEffect {
        guard !bonds.isEmpty else {
            return ChemistryEffect(filterMod: 0, detuneCents: 0,
                                   velocityCorrelation: 0, rhythmSync: 0, spaceMod: 0)
        }

        var totalFilter: Float = 0
        var totalDetune: Float = 0
        var totalVelCorr: Float = 0
        var totalRhythm: Float = 0
        var totalSpace: Float = 0

        for bond in bonds {
            let effect = ChemistryEffect.forType(bond.type, strength: bond.effectiveStrength)
            totalFilter += effect.filterMod
            totalDetune += effect.detuneCents
            totalVelCorr += effect.velocityCorrelation
            totalRhythm += effect.rhythmSync
            totalSpace += effect.spaceMod
        }

        let count = Float(bonds.count)
        return ChemistryEffect(
            filterMod: totalFilter / count,
            detuneCents: totalDetune / count,
            velocityCorrelation: totalVelCorr / count,
            rhythmSync: totalRhythm / count,
            spaceMod: totalSpace / count
        )
    }
}
