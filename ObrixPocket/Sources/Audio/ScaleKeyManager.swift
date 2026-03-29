import Foundation
import Combine

/// Musical key (root note), 0-indexed from C
enum MusicalKey: Int, CaseIterable, Codable {
    case C = 0, Cs = 1, D = 2, Ds = 3, E = 4, F = 5
    case Fs = 6, G = 7, Gs = 8, A = 9, As = 10, B = 11

    var displayName: String {
        switch self {
        case .C: return "C"
        case .Cs: return "C#"
        case .D: return "D"
        case .Ds: return "D#"
        case .E: return "E"
        case .F: return "F"
        case .Fs: return "F#"
        case .G: return "G"
        case .Gs: return "G#"
        case .A: return "A"
        case .As: return "A#"
        case .B: return "B"
        }
    }

    /// Flat-name alternative for display
    var flatName: String {
        switch self {
        case .Cs: return "Db"
        case .Ds: return "Eb"
        case .Fs: return "Gb"
        case .Gs: return "Ab"
        case .As: return "Bb"
        default: return displayName
        }
    }
}

/// Extended scale system matching HarmonicContext.Scale
/// with additional display and filtering functionality.
enum ExtendedScale: String, CaseIterable, Codable {
    case major
    case minor
    case pentatonic
    case blues
    case dorian
    case phrygian
    case mixolydian
    case wholeTone
    case chromatic

    var displayName: String {
        switch self {
        case .major:      return "Major"
        case .minor:      return "Minor"
        case .pentatonic: return "Pentatonic"
        case .blues:      return "Blues"
        case .dorian:     return "Dorian"
        case .phrygian:   return "Phrygian"
        case .mixolydian: return "Mixolydian"
        case .wholeTone:  return "Whole Tone"
        case .chromatic:  return "Chromatic"
        }
    }

    /// Semitone intervals from root
    var intervals: [Int] {
        switch self {
        case .major:      return [0, 2, 4, 5, 7, 9, 11]
        case .minor:      return [0, 2, 3, 5, 7, 8, 10]
        case .pentatonic: return [0, 2, 4, 7, 9]
        case .blues:      return [0, 3, 5, 6, 7, 10]
        case .dorian:     return [0, 2, 3, 5, 7, 9, 10]
        case .phrygian:   return [0, 1, 3, 5, 7, 8, 10]
        case .mixolydian: return [0, 2, 4, 5, 7, 9, 10]
        case .wholeTone:  return [0, 2, 4, 6, 8, 10]
        case .chromatic:  return Array(0...11)
        }
    }

    /// Number of notes in the scale
    var noteCount: Int { intervals.count }

    /// Character description for UI
    var character: String {
        switch self {
        case .major:      return "Bright, happy"
        case .minor:      return "Dark, emotional"
        case .pentatonic: return "Simple, universal"
        case .blues:      return "Soulful, gritty"
        case .dorian:     return "Jazzy, modal"
        case .phrygian:   return "Spanish, tense"
        case .mixolydian: return "Funky, dominant"
        case .wholeTone:  return "Dreamy, floating"
        case .chromatic:  return "All notes, no filter"
        }
    }

    /// Convert to HarmonicContext.Scale
    var harmonicScale: Scale {
        switch self {
        case .major:      return .major
        case .minor:      return .minor
        case .pentatonic: return .pentatonic
        case .blues:      return .blues
        case .dorian:     return .dorian
        case .phrygian:   return .phrygian
        case .mixolydian: return .mixolydian
        case .wholeTone:  return .wholeTone
        case .chromatic:  return .chromatic
        }
    }

    /// Check if a MIDI note is in this scale for a given key
    func contains(midiNote: Int, key: MusicalKey) -> Bool {
        let semitone = ((midiNote - key.rawValue) % 12 + 12) % 12
        return intervals.contains(semitone)
    }

    /// Get all MIDI notes in this scale within a range
    func notesInRange(key: MusicalKey, low: Int, high: Int) -> [Int] {
        (low...high).filter { contains(midiNote: $0, key: key) }
    }
}

/// Roaming key mode — key shifts every N bars following the chord progression
enum RoamingMode: String, Codable {
    case off            // Fixed key
    case everyBar       // Key shifts every 4 beats
    case everyPhrase    // Key shifts every 16 beats (4 bars)
    case followChord    // Key follows the chord root (most dynamic)

    var displayName: String {
        switch self {
        case .off:          return "Fixed"
        case .everyBar:     return "Every Bar"
        case .everyPhrase:  return "Every Phrase"
        case .followChord:  return "Follow Chord"
        }
    }

    /// How many beats between key changes
    var beatsPerChange: Int? {
        switch self {
        case .off:          return nil
        case .everyBar:     return 4
        case .everyPhrase:  return 16
        case .followChord:  return 4  // Tied to chord changes
        }
    }
}

/// Manages scale and key selection for the entire app.
///
/// This is the single source of truth for "what key and scale are we in?"
/// Both the PlayKeyboard and the HarmonicContext read from this manager.
/// When the user changes key or scale, all musical systems update together.
///
/// Supports "roaming key" mode where the key shifts automatically following
/// the chord progression — creating natural modulations without wrong notes.
final class ScaleKeyManager: ObservableObject {

    // MARK: - Published State (SwiftUI-observable)

    /// Current musical key
    @Published var key: MusicalKey = .C

    /// Current scale
    @Published var scale: ExtendedScale = .major

    /// Roaming key mode
    @Published var roamingMode: RoamingMode = .off

    /// Octave offset for the keyboard (-2 to +2)
    @Published var octaveOffset: Int = 0

    // MARK: - Derived State

    /// Current key as an integer (for HarmonicContext)
    var keyValue: Int { key.rawValue }

    /// Display string: "C Major", "D# Phrygian", etc.
    var displayString: String {
        "\(key.displayName) \(scale.displayName)"
    }

    /// Display string with roaming indicator
    var fullDisplayString: String {
        if roamingMode != .off {
            return "\(displayString) (\(roamingMode.displayName))"
        }
        return displayString
    }

    /// All MIDI notes in the current scale for the keyboard range (2 octaves from offset)
    var keyboardNotes: [Int] {
        let baseNote = 48 + (octaveOffset * 12)
        return scale.notesInRange(key: key, low: baseNote, high: baseNote + 24)
    }

    // MARK: - Roaming Key

    /// Beat counter for roaming key changes
    private var roamingBeatCounter: Int = 0

    /// Progression for roaming key derivation
    private var roamingProgression: ChordProgression = .popBright
    private var roamingChordIndex: Int = 0

    /// Advance beat for roaming mode. Call on each beat from the tick scheduler.
    /// Returns true if the key changed.
    @discardableResult
    func advanceBeat() -> Bool {
        guard roamingMode != .off else { return false }

        roamingBeatCounter += 1

        guard let beatsPerChange = roamingMode.beatsPerChange else { return false }

        if roamingBeatCounter >= beatsPerChange {
            roamingBeatCounter = 0

            if roamingMode == .followChord {
                // Advance chord and derive key from chord root
                roamingChordIndex = (roamingChordIndex + 1) % roamingProgression.degrees.count
                let degree = roamingProgression.degrees[roamingChordIndex]
                let newKeyValue = (key.rawValue + scale.intervals[degree % scale.intervals.count]) % 12
                if let newKey = MusicalKey(rawValue: newKeyValue), newKey != key {
                    key = newKey
                    return true
                }
            } else {
                // Simple key shift: move up by a perfect fifth (circle of fifths)
                let newKeyValue = (key.rawValue + 7) % 12
                if let newKey = MusicalKey(rawValue: newKeyValue) {
                    key = newKey
                    return true
                }
            }
        }

        return false
    }

    /// Set the roaming progression (usually from HarmonicContext's current progression)
    func setRoamingProgression(_ progression: ChordProgression) {
        roamingProgression = progression
        roamingChordIndex = 0
        roamingBeatCounter = 0
    }

    // MARK: - Sync with HarmonicContext

    /// Push current state to a HarmonicContext
    func applyTo(_ context: HarmonicContext) {
        // HarmonicContext uses the same Scale enum, so we convert
        // Note: HarmonicContext.key is an Int, MusicalKey.rawValue is compatible
        // The context's key and scale are private(set), so we work through
        // transitionToZone or init. For manual override we'd need to add a setter.
        // For now, this method serves as a documentation point for the wiring.
    }

    // MARK: - Persistence

    /// Save current settings to UserDefaults
    func save() {
        UserDefaults.standard.set(key.rawValue, forKey: "scaleKey")
        UserDefaults.standard.set(scale.rawValue, forKey: "scaleType")
        UserDefaults.standard.set(roamingMode.rawValue, forKey: "roamingMode")
        UserDefaults.standard.set(octaveOffset, forKey: "octaveOffset")
    }

    /// Restore settings from UserDefaults
    func restore() {
        if let keyVal = MusicalKey(rawValue: UserDefaults.standard.integer(forKey: "scaleKey")) {
            key = keyVal
        }
        if let scaleStr = UserDefaults.standard.string(forKey: "scaleType"),
           let scaleVal = ExtendedScale(rawValue: scaleStr) {
            scale = scaleVal
        }
        if let roamStr = UserDefaults.standard.string(forKey: "roamingMode"),
           let roamVal = RoamingMode(rawValue: roamStr) {
            roamingMode = roamVal
        }
        octaveOffset = UserDefaults.standard.integer(forKey: "octaveOffset")
        octaveOffset = max(-2, min(2, octaveOffset))
    }

    // MARK: - Convenience

    /// Check if a specific MIDI note is in the current scale+key
    func isInScale(_ midiNote: Int) -> Bool {
        scale.contains(midiNote: midiNote, key: key)
    }

    /// Get the scale degree (0-based) of a MIDI note, or nil if not in scale
    func scaleDegree(of midiNote: Int) -> Int? {
        let semitone = ((midiNote - key.rawValue) % 12 + 12) % 12
        return scale.intervals.firstIndex(of: semitone)
    }

    /// Suggest a reef water color tint based on current key/scale
    /// Warm colors for major-like scales, cool for minor-like
    var reefColorTint: (red: Float, green: Float, blue: Float) {
        switch scale {
        case .major, .mixolydian:
            return (0.9, 0.85, 0.6)      // Warm golden
        case .minor, .phrygian:
            return (0.4, 0.5, 0.8)        // Cool blue
        case .pentatonic:
            return (0.6, 0.9, 0.7)        // Fresh green
        case .blues:
            return (0.6, 0.4, 0.7)        // Purple haze
        case .dorian:
            return (0.5, 0.7, 0.8)        // Teal jazz
        case .wholeTone:
            return (0.7, 0.6, 0.9)        // Lavender dream
        case .chromatic:
            return (0.7, 0.7, 0.7)        // Neutral
        }
    }
}
