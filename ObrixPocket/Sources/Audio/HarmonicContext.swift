import Foundation

/// Represents a musical scale by its intervals from the root.
enum Scale: String, CaseIterable, Codable {
    case major       // W W H W W W H
    case minor       // W H W W H W W (natural minor)
    case pentatonic  // W W WH W WH
    case blues       // WH W H H WH W
    case dorian      // W H W W W H W
    case phrygian    // H W W W H W W
    case mixolydian  // W W H W W H W
    case wholeTone   // W W W W W W
    case chromatic   // H H H H H H H H H H H H

    /// Semitone intervals from root (0)
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
}

/// Named chord progressions with emotional character.
/// Each progression is an array of scale-degree roots (0-indexed).
enum ChordProgression: String, CaseIterable {
    case popBright      // I-V-vi-IV — universally pleasant
    case cinematicDark  // i-VII-VI-V — cinematic descending
    case jazzii_V_I     // ii-V-I — jazz resolution
    case folkAmbient    // I-IV-I-V — pastoral simplicity
    case epicRise       // vi-IV-I-V — epic, anthem-like
    case minorBlues     // i-iv-i-V — bluesy, soulful
    case tension        // i-bVI-bIII-bVII — dark, modal
    case dreamFloat     // I-iii-vi-IV — dreamy, floating
    case powerDescent   // I-bVII-bVI-bVII — heavy, descending

    /// Scale degree roots for each chord in the progression (0-indexed).
    /// 0 = I/i, 1 = II/ii, 2 = III/iii, etc.
    var degrees: [Int] {
        switch self {
        case .popBright:     return [0, 4, 5, 3]    // I V vi IV
        case .cinematicDark: return [0, 6, 5, 4]    // i VII VI V
        case .jazzii_V_I:    return [1, 4, 0, 0]    // ii V I I
        case .folkAmbient:   return [0, 3, 0, 4]    // I IV I V
        case .epicRise:      return [5, 3, 0, 4]    // vi IV I V
        case .minorBlues:    return [0, 3, 0, 4]    // i iv i V
        case .tension:       return [0, 5, 2, 6]    // i bVI bIII bVII
        case .dreamFloat:    return [0, 2, 5, 3]    // I iii vi IV
        case .powerDescent:  return [0, 6, 5, 6]    // I bVII bVI bVII
        }
    }

    /// Emotional character for UI display and specimen mood matching
    var mood: String {
        switch self {
        case .popBright:     return "bright"
        case .cinematicDark: return "dark"
        case .jazzii_V_I:    return "sophisticated"
        case .folkAmbient:   return "peaceful"
        case .epicRise:      return "epic"
        case .minorBlues:    return "soulful"
        case .tension:       return "tense"
        case .dreamFloat:    return "dreamy"
        case .powerDescent:  return "heavy"
        }
    }
}

/// Maintains the harmonic state for a Dive session.
/// Manages key, scale, chord progression, and provides notes-in-key for each musical role.
final class HarmonicContext {

    // MARK: - Properties

    /// Current key (0 = C, 1 = C#, 2 = D, ... 11 = B)
    private(set) var key: Int

    /// Current scale
    private(set) var scale: Scale

    /// Current chord progression
    private(set) var progression: ChordProgression

    /// Current chord index within the progression (0-3)
    private(set) var chordIndex: Int = 0

    /// Beats per chord (how many beats before advancing to next chord)
    let beatsPerChord: Int = 4

    /// Beat counter within current chord
    private var beatInChord: Int = 0

    // MARK: - Init

    init(key: Int = 0, scale: Scale = .major, progression: ChordProgression = .popBright) {
        self.key = max(0, min(11, key))
        self.scale = scale
        self.progression = progression
    }

    // MARK: - Progression Selection

    /// Select a chord progression based on the depth zone and specimen mood weights.
    /// Called when the Dive starts or when the zone changes.
    static func progressionForZone(_ zone: DepthZone, specimenMood: Float = 0.5) -> ChordProgression {
        // Each zone has weighted preferences for progressions
        switch zone {
        case .sunlit:
            // Bright, pleasant — favor major progressions
            let options: [(ChordProgression, Float)] = [
                (.popBright, 0.35), (.folkAmbient, 0.25), (.dreamFloat, 0.2),
                (.epicRise, 0.15), (.jazzii_V_I, 0.05)
            ]
            return weightedSelect(options, bias: specimenMood)
        case .twilight:
            // Cinematic, darker — favor minor/descending
            let options: [(ChordProgression, Float)] = [
                (.cinematicDark, 0.3), (.epicRise, 0.2), (.minorBlues, 0.2),
                (.tension, 0.15), (.dreamFloat, 0.15)
            ]
            return weightedSelect(options, bias: specimenMood)
        case .midnight:
            // Tense, chromatic — favor tension/dark progressions
            let options: [(ChordProgression, Float)] = [
                (.tension, 0.35), (.cinematicDark, 0.25), (.powerDescent, 0.2),
                (.minorBlues, 0.15), (.jazzii_V_I, 0.05)
            ]
            return weightedSelect(options, bias: specimenMood)
        case .abyssal:
            // Vast, minimal — favor simple/powerful progressions
            let options: [(ChordProgression, Float)] = [
                (.powerDescent, 0.3), (.folkAmbient, 0.25), (.tension, 0.2),
                (.cinematicDark, 0.15), (.dreamFloat, 0.1)
            ]
            return weightedSelect(options, bias: specimenMood)
        }
    }

    /// Weighted random selection with a bias value (0-1) that shifts preference
    private static func weightedSelect(_ options: [(ChordProgression, Float)], bias: Float) -> ChordProgression {
        var weights = options.map { $0.1 }
        // Bias shifts weight toward the first option (bias=1) or last (bias=0)
        for i in weights.indices {
            let position = Float(i) / Float(max(1, weights.count - 1))
            weights[i] *= (1.0 + (1.0 - bias) * position * 0.5)
        }
        let total = weights.reduce(0, +)
        var roll = Float.random(in: 0..<total)
        for (i, w) in weights.enumerated() {
            roll -= w
            if roll <= 0 { return options[i].0 }
        }
        return options[0].0
    }

    // MARK: - Beat Advancement

    /// Advance the beat counter. Returns true if the chord changed.
    @discardableResult
    func advanceBeat() -> Bool {
        beatInChord += 1
        if beatInChord >= beatsPerChord {
            beatInChord = 0
            chordIndex = (chordIndex + 1) % progression.degrees.count
            return true
        }
        return false
    }

    /// Force advance to the next chord (e.g., when collecting an orb)
    func forceNextChord() {
        beatInChord = 0
        chordIndex = (chordIndex + 1) % progression.degrees.count
    }

    /// Reset to the beginning of the progression
    func reset() {
        chordIndex = 0
        beatInChord = 0
    }

    // MARK: - Note Generation

    /// Get the current chord's root as a scale degree (0-indexed)
    var currentChordRoot: Int {
        progression.degrees[chordIndex % progression.degrees.count]
    }

    /// Build a chord (triad + optional 7th) from the current progression position.
    /// Returns MIDI note numbers for the chord tones in the specified octave.
    func currentChordNotes(octave: Int) -> [Int] {
        let scaleIntervals = scale.intervals
        let root = currentChordRoot

        // Build triad: root, third, fifth (stacked in scale degrees)
        var degrees = [root, root + 2, root + 4]

        // Add 7th for richness in some contexts
        degrees.append(root + 6)

        // Convert scale degrees to semitone offsets, then to MIDI notes
        return degrees.map { degree in
            let wrappedDegree = degree % scaleIntervals.count
            let octaveShift = degree / scaleIntervals.count
            let semitone = scaleIntervals[wrappedDegree] + (octaveShift * 12)
            return key + semitone + (octave * 12)
        }
    }

    /// Get all notes in the current scale for a given octave range.
    /// Used by melody roles that can play any scale degree.
    func scaleNotes(octave: Int, range: Int = 2) -> [Int] {
        var notes: [Int] = []
        for oct in octave..<(octave + range) {
            for interval in scale.intervals {
                let midi = key + interval + (oct * 12)
                if midi >= 24 && midi <= 108 {
                    notes.append(midi)
                }
            }
        }
        return notes
    }

    /// Get notes appropriate for a given musical role at the current harmonic position.
    /// This is the core of the role-based note selection system.
    func notesForRole(_ role: MusicalRole, baseOctave: Int) -> [Int] {
        let octave = baseOctave + role.octaveOffset

        switch role {
        case .bass:
            // Bass: root and fifth of current chord, low octave
            let chordNotes = currentChordNotes(octave: octave)
            // Keep only root and fifth (indices 0 and 2)
            return [chordNotes[0], chordNotes.count > 2 ? chordNotes[2] : chordNotes[0]]

        case .melody:
            // Melody: all scale notes across 2 octaves for maximum expression
            return scaleNotes(octave: octave, range: 2)

        case .harmony:
            // Harmony: full chord tones (triad + 7th)
            return currentChordNotes(octave: octave)

        case .rhythm:
            // Rhythm: root and fifth (percussive hits lock to tonality)
            let chordNotes = currentChordNotes(octave: octave)
            return [chordNotes[0], chordNotes.count > 2 ? chordNotes[2] : chordNotes[0]]

        case .texture, .effect:
            // These roles don't generate notes — they modify sound
            return []
        }
    }

    /// Get a tension note that resolves musically (not random!)
    /// Returns a note that creates dissonance but relates to the current harmony.
    func tensionNote(near midiNote: Int, intensity: Float) -> Int {
        // Low intensity: neighbor tones (half step away — will resolve)
        // Medium intensity: tritone substitution
        // High intensity: chromatic approach from below
        if intensity < 0.3 {
            return midiNote + (Bool.random() ? 1 : -1)    // Half-step neighbor
        } else if intensity < 0.7 {
            return midiNote + 6                             // Tritone
        } else {
            return midiNote - 1                             // Chromatic approach from below
        }
    }

    // MARK: - Zone Transitions

    /// Update harmonic context when entering a new depth zone
    func transitionToZone(_ zone: DepthZone, specimenMood: Float = 0.5) {
        // Select new progression appropriate for this zone
        progression = Self.progressionForZone(zone, specimenMood: specimenMood)

        // Adjust scale for zone character
        switch zone {
        case .sunlit:   scale = .major
        case .twilight: scale = .minor
        case .midnight: scale = .phrygian   // Dark, Spanish-tinged
        case .abyssal:  scale = .pentatonic // Simple, vast
        }

        // Reset progression position
        reset()
    }

    // MARK: - Profile-Driven Configuration

    /// Configure this context from a blended HarmonicProfile (called before a Dive begins).
    /// The profile encodes the wired specimen pair's combined harmonic identity — key, scale,
    /// tonal gravity, and complexity — and this method translates that into live playback state.
    ///
    /// Zone transitions can override scale afterwards; call this before `transitionToZone`.
    func configureFromProfile(_ profile: HarmonicProfile) {
        // Clamp root to valid pitch-class range
        key = max(0, min(11, profile.preferredRoot))

        // If tonal gravity is very low (< 0.3) the specimen is an atonal wanderer —
        // override the genetic scale preference with chromatic for maximum freedom.
        if profile.tonalGravity < 0.3 {
            scale = .chromatic
        } else {
            scale = profile.preferredScale.asScale
        }

        // Select a progression compatible with the profile's modal character.
        // modalCharacter: 0 = bright (Lydian-end), 1 = dark (Phrygian/Locrian-end).
        // harmonicComplexity: 0 = triadic, 1 = extended/jazz.
        progression = Self.progressionForProfile(profile)

        // Reset beat position so the new harmonic state takes effect immediately
        reset()
    }

    /// Choose a starting chord progression from a HarmonicProfile.
    /// Internal helper — keeps configureFromProfile readable.
    private static func progressionForProfile(_ profile: HarmonicProfile) -> ChordProgression {
        let dark       = profile.modalCharacter        // 0 = bright, 1 = dark
        let complexity = profile.harmonicComplexity    // 0 = simple, 1 = complex

        switch (dark > 0.6, complexity > 0.5) {
        case (true,  true):  return .tension          // Dark + complex: modal tension
        case (true,  false): return .cinematicDark    // Dark + simple: cinematic descend
        case (false, true):  return .jazzii_V_I       // Bright + complex: jazz ii-V-I
        case (false, false): return .popBright        // Bright + simple: universal major
        }
    }
}
