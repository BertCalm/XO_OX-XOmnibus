import Foundation
import Combine

/// Predefined macro targets that can be mapped to macro knobs
enum MacroTarget: String, CaseIterable, Codable {
    // Cross-cutting parameters
    case density        // How many voices/notes are active
    case brightness     // Filter cutoff across all specimens
    case space          // Reverb/delay send across all specimens
    case chaos          // Randomization/detuning amount

    // Specimen-specific (for remapping)
    case filterCutoff   // Direct filter cutoff
    case filterResonance // Direct filter resonance
    case lfoRate        // LFO rate across specimens
    case lfoDepth       // LFO depth across specimens
    case attackTime     // Envelope attack
    case releaseTime    // Envelope release
    case driveAmount    // Distortion/saturation
    case detuneAmount   // Unison detune spread

    var displayName: String {
        switch self {
        case .density:         return "Density"
        case .brightness:      return "Brightness"
        case .space:           return "Space"
        case .chaos:           return "Chaos"
        case .filterCutoff:    return "Filter"
        case .filterResonance: return "Resonance"
        case .lfoRate:         return "LFO Rate"
        case .lfoDepth:        return "LFO Depth"
        case .attackTime:      return "Attack"
        case .releaseTime:     return "Release"
        case .driveAmount:     return "Drive"
        case .detuneAmount:    return "Detune"
        }
    }

    /// The OBRIX parameter IDs this macro affects (applied to all active slots)
    var parameterIDs: [String] {
        switch self {
        case .density:         return []  // Handled by ArrangementEngine density target
        case .brightness:      return ["obrix_flt1Cutoff"]
        case .space:           return ["obrix_fx1Mix"]
        case .chaos:           return ["obrix_lfo1Depth", "obrix_src1Tune"]
        case .filterCutoff:    return ["obrix_flt1Cutoff"]
        case .filterResonance: return ["obrix_flt1Resonance"]
        case .lfoRate:         return ["obrix_lfo1Rate"]
        case .lfoDepth:        return ["obrix_lfo1Depth"]
        case .attackTime:      return ["obrix_env1Attack"]
        case .releaseTime:     return ["obrix_env1Release"]
        case .driveAmount:     return ["obrix_flt1Resonance"]  // Overdriven resonance as "drive"
        case .detuneAmount:    return ["obrix_src1Tune"]
        }
    }

    /// How this macro maps its 0-1 value to the parameter range
    var mapping: MacroMapping {
        switch self {
        case .density:         return .linear(min: 0.1, max: 1.0)
        case .brightness:      return .exponential(min: 0.1, max: 0.95)
        case .space:           return .linear(min: 0.0, max: 0.8)
        case .chaos:           return .exponential(min: 0.0, max: 0.7)
        case .filterCutoff:    return .exponential(min: 0.05, max: 0.95)
        case .filterResonance: return .linear(min: 0.0, max: 0.8)
        case .lfoRate:         return .exponential(min: 0.01, max: 10.0)
        case .lfoDepth:        return .linear(min: 0.0, max: 1.0)
        case .attackTime:      return .exponential(min: 0.001, max: 2.0)
        case .releaseTime:     return .exponential(min: 0.01, max: 5.0)
        case .driveAmount:     return .linear(min: 0.0, max: 0.9)
        case .detuneAmount:    return .linear(min: -0.5, max: 0.5)
        }
    }
}

/// How a macro value maps to a parameter range
enum MacroMapping: Codable {
    case linear(min: Float, max: Float)
    case exponential(min: Float, max: Float)

    /// Convert a 0-1 macro value to the mapped parameter value
    func apply(_ value: Float) -> Float {
        let clamped = max(0, min(1, value))
        switch self {
        case .linear(let min, let max):
            return min + clamped * (max - min)
        case .exponential(let min, let max):
            // Exponential mapping for perceptual parameters (filter, rate, time)
            let logMin = log(max(0.001, min))
            let logMax = log(max(0.001, max))
            return exp(logMin + clamped * (logMax - logMin))
        }
    }
}

/// A single macro knob with its current state
struct MacroKnob: Codable, Identifiable {
    let id: Int  // 0-3 (macro index)
    var target: MacroTarget
    var value: Float  // 0-1
    var label: String { target.displayName }

    /// Default macro assignments
    static let defaults: [MacroKnob] = [
        MacroKnob(id: 0, target: .density, value: 0.5),
        MacroKnob(id: 1, target: .brightness, value: 0.5),
        MacroKnob(id: 2, target: .space, value: 0.3),
        MacroKnob(id: 3, target: .chaos, value: 0.1),
    ]
}

/// Manages 4 macro knobs that cross-cut specimen parameters.
///
/// Each macro maps a 0-1 value to one or more OBRIX engine parameters
/// applied across all active specimen slots simultaneously. Macros can be
/// remapped by the user via long-press assignment.
///
/// Macros save per-reef-configuration — different setups for different moods.
final class MacroController: ObservableObject {

    // MARK: - Published State

    @Published var knobs: [MacroKnob] = MacroKnob.defaults

    // MARK: - Callbacks

    /// Called when a macro value changes. (slotIndex, paramID, value) for each affected parameter.
    var onParameterChange: ((String, Float) -> Void)?

    /// Called when density macro changes (handled by ArrangementEngine, not audio params)
    var onDensityChange: ((Float) -> Void)?

    // MARK: - Macro Control

    /// Set a macro value (0-1). Fires parameter changes to all affected params.
    func setMacro(_ index: Int, value: Float) {
        guard index >= 0 && index < knobs.count else { return }
        knobs[index].value = max(0, min(1, value))

        let target = knobs[index].target
        let mapped = target.mapping.apply(value)

        if target == .density {
            onDensityChange?(mapped)
        } else {
            for paramID in target.parameterIDs {
                onParameterChange?(paramID, mapped)
            }
        }
    }

    /// Get current macro value
    func macroValue(_ index: Int) -> Float {
        guard index >= 0 && index < knobs.count else { return 0.5 }
        return knobs[index].value
    }

    /// Remap a macro to a different target
    func remapMacro(_ index: Int, to target: MacroTarget) {
        guard index >= 0 && index < knobs.count else { return }
        knobs[index].target = target
        // Re-apply current value to the new target
        setMacro(index, value: knobs[index].value)
    }

    /// Get the mapped parameter value for a macro
    func mappedValue(_ index: Int) -> Float {
        guard index >= 0 && index < knobs.count else { return 0 }
        return knobs[index].target.mapping.apply(knobs[index].value)
    }

    // MARK: - Preset Management

    /// Save current macro state as a named preset
    func savePreset(name: String) -> MacroPreset {
        MacroPreset(name: name, knobs: knobs, date: Date())
    }

    /// Load a macro preset
    func loadPreset(_ preset: MacroPreset) {
        knobs = preset.knobs
        // Re-apply all values
        for (i, knob) in knobs.enumerated() {
            setMacro(i, value: knob.value)
        }
    }

    /// Reset all macros to defaults
    func resetToDefaults() {
        knobs = MacroKnob.defaults
        for (i, knob) in knobs.enumerated() {
            setMacro(i, value: knob.value)
        }
    }

    // MARK: - Persistence

    /// Save to UserDefaults (per-reef snapshots would use savePreset instead)
    func save() {
        if let data = try? JSONEncoder().encode(knobs) {
            UserDefaults.standard.set(data, forKey: "macroKnobs")
        }
    }

    /// Restore from UserDefaults
    func restore() {
        if let data = UserDefaults.standard.data(forKey: "macroKnobs"),
           let decoded = try? JSONDecoder().decode([MacroKnob].self, from: data) {
            knobs = decoded
        }
    }
}

/// A saved macro configuration
struct MacroPreset: Codable, Identifiable {
    let id: UUID
    let name: String
    let knobs: [MacroKnob]
    let date: Date

    init(name: String, knobs: [MacroKnob], date: Date) {
        self.id = UUID()
        self.name = name
        self.knobs = knobs
        self.date = date
    }
}
