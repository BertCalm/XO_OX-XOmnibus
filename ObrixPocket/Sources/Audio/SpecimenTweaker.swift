import Foundation
import Combine

/// A single parameter tweak applied to a specimen
struct ParameterTweak: Codable, Identifiable {
    let id: UUID
    let parameterID: String       // e.g., "obrix_flt1Cutoff"
    let defaultValue: Float       // Original value from catalog
    var tweakedValue: Float        // Player's modified value
    let minValue: Float            // Minimum allowed (default - 20%)
    let maxValue: Float            // Maximum allowed (default + 20%)

    /// How far from default this tweak is (-1 to +1, 0 = at default)
    var deviation: Float {
        guard maxValue > minValue else { return 0 }
        let range = maxValue - minValue
        return (tweakedValue - defaultValue) / (range / 2)
    }

    /// Whether this tweak has been modified from the default
    var isModified: Bool {
        abs(tweakedValue - defaultValue) > 0.001
    }

    /// Reset to default value
    mutating func reset() {
        tweakedValue = defaultValue
    }

    /// Human-readable parameter name (strips "obrix_" prefix)
    var displayName: String {
        let name = parameterID.replacingOccurrences(of: "obrix_", with: "")
        // Convert camelCase to Title Case
        var result = ""
        for (i, char) in name.enumerated() {
            if char.isUppercase && i > 0 {
                result += " "
            }
            result += i == 0 ? String(char).uppercased() : String(char)
        }
        return result
    }
}

/// Tracks all parameter tweaks for a single specimen.
/// Tweaks are limited to ±20% from the catalog default values.
struct SpecimenTweakSet: Codable, Identifiable {
    let id: UUID
    let slotIndex: Int
    let subtypeID: String
    let specimenName: String
    var tweaks: [ParameterTweak]
    let createdDate: Date
    var lastModifiedDate: Date

    /// Whether any parameter has been modified from default
    var isMutated: Bool {
        tweaks.contains { $0.isModified }
    }

    /// Count of modified parameters
    var mutationCount: Int {
        tweaks.filter { $0.isModified }.count
    }

    /// Overall mutation intensity (average absolute deviation, 0-1)
    var mutationIntensity: Float {
        let modified = tweaks.filter { $0.isModified }
        guard !modified.isEmpty else { return 0 }
        return modified.reduce(0) { $0 + abs($1.deviation) } / Float(modified.count)
    }

    /// Reset all tweaks to defaults
    mutating func resetAll() {
        for i in tweaks.indices {
            tweaks[i].reset()
        }
        lastModifiedDate = Date()
    }

    /// Get the tweaked value for a parameter, or nil if not tracked
    func value(for paramID: String) -> Float? {
        tweaks.first { $0.parameterID == paramID }?.tweakedValue
    }

    /// Update a specific parameter tweak
    mutating func setTweak(paramID: String, value: Float) {
        guard let index = tweaks.firstIndex(where: { $0.parameterID == paramID }) else { return }
        tweaks[index].tweakedValue = max(tweaks[index].minValue,
                                          min(tweaks[index].maxValue, value))
        lastModifiedDate = Date()
    }
}

/// Manages specimen parameter tweaking across the entire reef.
///
/// When a player enters Microscope edit mode and adjusts a parameter,
/// the tweak is stored here and applied to the audio engine. Tweaks
/// persist across sessions and appear as "mutations" on specimen cards.
///
/// Design: Intentionally limited to ±20% from defaults. This isn't a synth
/// programmer — it's personalization. The specimen's identity stays intact,
/// but YOUR version sounds slightly different from everyone else's.
final class SpecimenTweaker: ObservableObject {

    // MARK: - State

    /// Active tweak sets per slot index
    @Published var tweakSets: [Int: SpecimenTweakSet] = [:]

    /// Currently editing slot (nil = not in edit mode)
    @Published var editingSlot: Int? = nil

    /// The tweak range as a percentage (±20%)
    static let tweakRange: Float = 0.20

    // MARK: - Callbacks

    /// Called when a parameter value changes. (parameterID, newValue)
    var onParameterChange: ((String, Float) -> Void)?

    // MARK: - Setup

    /// Initialize tweak tracking for a specimen in a slot.
    /// Creates a tweak set from the catalog defaults with ±20% ranges.
    func initializeSlot(_ slotIndex: Int, subtypeID: String) {
        guard let entry = SpecimenCatalog.entry(for: subtypeID) else { return }

        let tweaks = entry.defaultParams.map { (paramID, defaultValue) -> ParameterTweak in
            let range = abs(defaultValue) * Self.tweakRange
            // For parameters near 0, use absolute range of 0.1
            let effectiveRange = max(range, 0.1)
            return ParameterTweak(
                id: UUID(),
                parameterID: paramID,
                defaultValue: defaultValue,
                tweakedValue: defaultValue,  // Start at default
                minValue: max(0, defaultValue - effectiveRange),
                maxValue: min(1, defaultValue + effectiveRange)
            )
        }

        tweakSets[slotIndex] = SpecimenTweakSet(
            id: UUID(),
            slotIndex: slotIndex,
            subtypeID: subtypeID,
            specimenName: entry.creatureName,
            tweaks: tweaks,
            createdDate: Date(),
            lastModifiedDate: Date()
        )
    }

    /// Remove tweak tracking for a slot (when specimen is released)
    func removeSlot(_ slotIndex: Int) {
        tweakSets.removeValue(forKey: slotIndex)
    }

    // MARK: - Edit Mode

    /// Enter edit mode for a slot
    func startEditing(slot: Int) {
        editingSlot = slot
    }

    /// Exit edit mode
    func stopEditing() {
        editingSlot = nil
    }

    /// Adjust a parameter for the currently editing slot
    func adjustParameter(_ paramID: String, value: Float) {
        guard let slot = editingSlot else { return }
        tweakSets[slot]?.setTweak(paramID: paramID, value: value)
        onParameterChange?(paramID, value)
    }

    /// Reset a single parameter to default
    func resetParameter(_ paramID: String) {
        guard let slot = editingSlot,
              var tweakSet = tweakSets[slot] else { return }
        if let index = tweakSet.tweaks.firstIndex(where: { $0.parameterID == paramID }) {
            let defaultVal = tweakSet.tweaks[index].defaultValue
            tweakSet.tweaks[index].reset()
            tweakSets[slot] = tweakSet
            onParameterChange?(paramID, defaultVal)
        }
    }

    /// Reset all parameters for the editing slot to defaults
    func resetAll() {
        guard let slot = editingSlot else { return }
        tweakSets[slot]?.resetAll()
        // Re-apply all defaults
        if let tweakSet = tweakSets[slot] {
            for tweak in tweakSet.tweaks {
                onParameterChange?(tweak.parameterID, tweak.defaultValue)
            }
        }
    }

    // MARK: - Query

    /// Whether a slot has any mutations
    func isMutated(slot: Int) -> Bool {
        tweakSets[slot]?.isMutated ?? false
    }

    /// Get mutation intensity for a slot (0-1, for visual display)
    func mutationIntensity(slot: Int) -> Float {
        tweakSets[slot]?.mutationIntensity ?? 0
    }

    /// Get all tweaked parameter values for a slot (for applying to audio engine)
    func tweakedParams(for slot: Int) -> [String: Float] {
        guard let tweakSet = tweakSets[slot] else { return [:] }
        var result: [String: Float] = [:]
        for tweak in tweakSet.tweaks {
            result[tweak.parameterID] = tweak.tweakedValue
        }
        return result
    }

    // MARK: - Persistence

    /// Save all tweak sets to UserDefaults
    func save() {
        if let data = try? JSONEncoder().encode(Array(tweakSets.values)) {
            UserDefaults.standard.set(data, forKey: "specimenTweaks")
        }
    }

    /// Restore tweak sets from UserDefaults
    func restore() {
        if let data = UserDefaults.standard.data(forKey: "specimenTweaks"),
           let sets = try? JSONDecoder().decode([SpecimenTweakSet].self, from: data) {
            tweakSets = [:]
            for set in sets {
                tweakSets[set.slotIndex] = set
            }
        }
    }

    // MARK: - Export

    /// Export tweak set as a shareable dictionary
    func exportTweaks(slot: Int) -> [String: Any]? {
        guard let tweakSet = tweakSets[slot] else { return nil }
        var dict: [String: Any] = [
            "subtypeID": tweakSet.subtypeID,
            "specimenName": tweakSet.specimenName,
            "mutationCount": tweakSet.mutationCount,
            "mutationIntensity": tweakSet.mutationIntensity,
        ]
        var params: [String: Float] = [:]
        for tweak in tweakSet.tweaks where tweak.isModified {
            params[tweak.parameterID] = tweak.tweakedValue
        }
        dict["tweakedParams"] = params
        return dict
    }
}
