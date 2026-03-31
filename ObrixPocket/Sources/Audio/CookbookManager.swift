import Foundation

// MARK: - CouplingRecipe

/// A discovered coupling recipe: two parent subtypes that were wired together
/// and produced offspring. Resonant pairs are those found in CouplingAffinity.highAffinityPairs.
///
/// There are exactly 12 resonant pairs in the game (matching CouplingAffinity.highAffinityPairs).
/// Collecting all 12 unlocks the Master Alchemist badge.
struct CouplingRecipe: Codable, Identifiable {
    let id: UUID
    let parent1Subtype: String      // e.g., "polyblep-saw"
    let parent2Subtype: String      // e.g., "svf-lp"
    let affinityTier: AffinityTier  // LOW, MEDIUM, HIGH
    let offspringSubtype: String    // Subtype ID of the bred offspring
    let discoveredDate: Date
    var communityName: String?      // Optional player-assigned name for the recipe

    /// Whether this pair is one of the 12 canonical resonant pairs from CouplingAffinity.
    let isResonantPair: Bool

    /// Total number of canonical resonant pairs in the game.
    static let resonantPairCount = 12

    // MARK: - AffinityTier

    /// Maps from CouplingAffinity cases to a Codable string enum.
    /// Stored as a string so the persisted JSON is readable without the CouplingAffinity type.
    enum AffinityTier: String, Codable, CaseIterable {
        case low     = "LOW"
        case medium  = "MEDIUM"
        case high    = "HIGH"

        /// Display label used in the Cookbook UI.
        var displayLabel: String {
            switch self {
            case .low:    return "Low"
            case .medium: return "Medium"
            case .high:   return "High"
            }
        }

        /// Accent color key used by DesignTokens — callers resolve via DesignTokens.
        var colorKey: String {
            switch self {
            case .low:    return "muted"
            case .medium: return "xoGold"
            case .high:   return "reefJade"
            }
        }
    }

    // MARK: - Canonical key

    /// Canonical key for deduplicating recipes: always sorts the two parent subtypes
    /// so that (A, B) and (B, A) resolve to the same key.
    var pairKey: String {
        let sorted = [parent1Subtype, parent2Subtype].sorted()
        return "\(sorted[0])|\(sorted[1])"
    }
}

// MARK: - CookbookManager

/// Tracks and persists all discovered coupling recipes in OBRIX Pocket.
///
/// A recipe is recorded each time `breed()` completes (via `BreedingManager`).
/// The caller (GameCoordinator or BreedingManager post-breed hook) is responsible
/// for calling `recordDiscovery(parent1:parent2:offspring:affinity:)` — this keeps
/// CookbookManager dependency-free and testable in isolation.
///
/// Resonant pairs mirror CouplingAffinity.highAffinityPairs exactly (12 pairs).
/// When all 12 resonant pairs are discovered, `isMasterAlchemist` returns true.
///
/// Persistence: UserDefaults under key "cookbookRecipes".
final class CookbookManager: ObservableObject {

    // MARK: - Shared singleton

    static let shared = CookbookManager()

    // MARK: - Published state

    /// All discovered recipes, deduplicated by pairKey — most recent first.
    @Published private(set) var discoveredRecipes: [CouplingRecipe] = []

    // MARK: - Computed metrics

    /// Number of resonant pairs (HIGH affinity) the player has discovered.
    var resonantPairsDiscovered: Int {
        discoveredRecipes.filter { $0.isResonantPair }.count
    }

    /// True when all 12 resonant pairs have been discovered.
    var isMasterAlchemist: Bool {
        resonantPairsDiscovered >= CouplingRecipe.resonantPairCount
    }

    /// Progress fraction (0–1) toward the Master Alchemist milestone.
    var masterAlchemistProgress: Float {
        Float(resonantPairsDiscovered) / Float(CouplingRecipe.resonantPairCount)
    }

    // MARK: - Init

    private init() {
        restore()
    }

    // MARK: - Recording discoveries

    /// Record a successful coupling recipe.
    ///
    /// Idempotent: if the same pair key already exists in discoveredRecipes,
    /// the existing record is **not** replaced — coupling recipes are unique
    /// per pair (the first discovery is the canonical one). Returns early if
    /// already discovered.
    ///
    /// - Parameters:
    ///   - parent1: Subtype ID of the first parent specimen (e.g., "polyblep-saw").
    ///   - parent2: Subtype ID of the second parent specimen (e.g., "svf-lp").
    ///   - offspring: Subtype ID that the breed() call produced.
    ///   - affinity: The CouplingAffinity value for this pair (converted to AffinityTier).
    @discardableResult
    func recordDiscovery(
        parent1: String,
        parent2: String,
        offspring: String,
        affinity: CouplingAffinity
    ) -> CouplingRecipe? {
        // Build a canonical pair key to check for duplicates.
        let sortedPair = [parent1, parent2].sorted()
        let key = "\(sortedPair[0])|\(sortedPair[1])"

        // Already discovered — return the existing entry.
        if let existing = discoveredRecipes.first(where: { $0.pairKey == key }) {
            return existing
        }

        let tier: CouplingRecipe.AffinityTier
        switch affinity {
        case .high:    tier = .high
        case .medium:  tier = .medium
        case .low:     tier = .low
        case .invalid: return nil   // Invalid pairs cannot be bred; ignore.
        }

        let recipe = CouplingRecipe(
            id: UUID(),
            parent1Subtype: sortedPair[0],
            parent2Subtype: sortedPair[1],
            affinityTier: tier,
            offspringSubtype: offspring,
            discoveredDate: Date(),
            communityName: nil,
            isResonantPair: tier == .high
        )

        // Prepend so the grid shows most-recent first.
        discoveredRecipes.insert(recipe, at: 0)
        save()
        return recipe
    }

    // MARK: - Querying

    /// Look up a discovered recipe by its two parent subtype IDs.
    /// Returns nil if this pair has not yet been discovered.
    func getRecipe(for parent1: String, and parent2: String) -> CouplingRecipe? {
        let sorted = [parent1, parent2].sorted()
        let key = "\(sorted[0])|\(sorted[1])"
        return discoveredRecipes.first { $0.pairKey == key }
    }

    /// Returns all resonant-pair recipes discovered so far.
    var discoveredResonantRecipes: [CouplingRecipe] {
        discoveredRecipes.filter { $0.isResonantPair }
    }

    // MARK: - Community naming

    /// Assign a player-supplied community name to a recipe.
    /// Silently no-ops if the recipe ID is not found.
    func nameRecipe(id: UUID, name: String) {
        guard let index = discoveredRecipes.firstIndex(where: { $0.id == id }) else { return }
        let trimmed = name.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else { return }
        discoveredRecipes[index].communityName = trimmed
        save()
    }

    // MARK: - Persistence

    private static let storageKey = "cookbookRecipes"

    func save() {
        guard let data = try? JSONEncoder().encode(discoveredRecipes) else { return }
        UserDefaults.standard.set(data, forKey: Self.storageKey)
    }

    func restore() {
        guard
            let data = UserDefaults.standard.data(forKey: Self.storageKey),
            let decoded = try? JSONDecoder().decode([CouplingRecipe].self, from: data)
        else { return }
        discoveredRecipes = decoded
    }
}
