import Foundation

/// How a note's pitch contour tends to move over a phrase
enum ContourShape: String, Codable {
    case ascending   // Tends upward over the phrase
    case descending  // Tends downward
    case arch        // Up then down (melodic arc)
    case valley      // Down then up
    case wave        // Oscillates
    case static_     // Stays in a narrow range (renamed to avoid keyword)
}

/// Defines the unique musical personality of a specimen in the Dive.
/// Each specimen has distinct rhythmic feel, melodic tendencies, and articulation.
struct VoiceProfile: Codable {

    // MARK: - Identity
    let specimenName: String

    // MARK: - Rhythmic Character

    /// 16-step probability pattern (one bar of 16th notes). 0.0 = never fires, 1.0 = always fires.
    /// This is a PROBABILITY, not a binary gate — creates natural variation.
    let beatPattern: [Float]

    /// Swing amount: 0 = perfectly straight, 1.0 = full triplet shuffle.
    /// Applied by delaying even-numbered 16th notes.
    let swing: Float

    // MARK: - Melodic Character

    /// Preferred melodic intervals in semitones. The note selection algorithm
    /// biases toward these intervals when choosing the next note.
    let preferredIntervals: [Int]

    /// How the melody tends to move across a phrase
    let contour: ContourShape

    /// MIDI note range for this voice
    let rangeLow: Int
    let rangeHigh: Int

    // MARK: - Articulation

    /// Note attack sharpness: 0.0 = soft pad onset, 1.0 = hard percussive strike
    let attackSharpness: Float

    /// Sustain tendency: 0.0 = very short (staccato), 1.0 = holds until next note (legato)
    let sustainLength: Float

    /// Velocity variation: 0.0 = constant velocity, 1.0 = highly dynamic
    let velocityVariation: Float

    /// Accent pattern: which beats get emphasis (indices into beatPattern that get +20% velocity)
    let accentBeats: [Int]

    // MARK: - Interaction

    /// If true, this voice listens to other voices and adjusts (e.g., avoids clashing)
    let responsive: Bool

    /// If true, this voice is an anchor that other voices reference
    let anchor: Bool

    // MARK: - Convenience

    /// Get the probability that a note fires on a given subdivision (0-15)
    func firingProbability(at subdivision: Int) -> Float {
        guard !beatPattern.isEmpty else { return 0.5 }
        return beatPattern[subdivision % beatPattern.count]
    }

    /// Whether a given subdivision should be accented
    func isAccented(_ subdivision: Int) -> Bool {
        accentBeats.contains(subdivision % 16)
    }

    /// Clamp a MIDI note to this voice's range
    func clampToRange(_ note: Int) -> Int {
        max(rangeLow, min(rangeHigh, note))
    }

    /// Score how well a proposed interval matches this voice's preferences.
    /// Lower score = better match. Used by note selection to bias toward character.
    func intervalScore(_ semitones: Int) -> Float {
        let absSemitones = abs(semitones)
        // Check if this interval (or its inversion) is preferred
        if preferredIntervals.contains(absSemitones) || preferredIntervals.contains(semitones) {
            return 0.0  // Perfect match
        }
        // Score by distance from nearest preferred interval
        let nearest = preferredIntervals.map { abs($0 - absSemitones) }.min() ?? absSemitones
        return Float(nearest) * 0.3
    }
}

// MARK: - Voice Profile Catalog

/// Maps each specimen to its unique VoiceProfile.
/// These profiles define the musical personality of each creature in the Dive.
enum VoiceProfileCatalog {

    /// Look up a voice profile by specimen subtype ID
    static func profile(for subtypeID: String) -> VoiceProfile {
        switch subtypeID {

        // ═══════════════════════════════════════
        // SOURCES (Shells) — Primary note generators
        // ═══════════════════════════════════════

        case "polyblep-saw":  // Sawfin — BASS: The Reef's Foundation
            return VoiceProfile(
                specimenName: "Sawfin",
                beatPattern: [1.0, 0, 0, 0,  0, 0, 0, 0,  0.7, 0, 0, 0,  0, 0, 0, 0],  // Whole + dotted half
                swing: 0.0,
                preferredIntervals: [0, 5, 7, -5, -7],  // Roots, 4ths, 5ths (bass walks)
                contour: .descending,
                rangeLow: 36, rangeHigh: 55,   // C2-G3 (deep bass)
                attackSharpness: 0.3,
                sustainLength: 0.9,
                velocityVariation: 0.15,
                accentBeats: [0, 8],            // Beat 1 and beat 3
                responsive: false,
                anchor: true                     // Bass anchors everything
            )

        case "polyblep-square":  // Boxjelly — HARMONY: Hollow pulsing chords
            return VoiceProfile(
                specimenName: "Boxjelly",
                beatPattern: [0, 0, 0.8, 0,  0, 0, 0.8, 0,  0, 0, 0.8, 0,  0, 0, 0.8, 0],  // Off-beat stabs
                swing: 0.15,
                preferredIntervals: [3, 4, 7, -3, -4],  // Thirds and fourths (chord voicings)
                contour: .static_,
                rangeLow: 48, rangeHigh: 72,   // C3-C5 (mid register)
                attackSharpness: 0.2,
                sustainLength: 0.85,
                velocityVariation: 0.1,
                accentBeats: [2, 6, 10, 14],    // Every off-beat
                responsive: true,
                anchor: false
            )

        case "noise-white":  // Foamspray — RHYTHM: Percussive chaos
            return VoiceProfile(
                specimenName: "Foamspray",
                beatPattern: [1.0, 0.2, 0.1, 0.8,  0.3, 0.9, 0, 0.4,  1.0, 0.1, 0.7, 0.2,  0, 0.5, 0.1, 0.6],  // Syncopated
                swing: 0.6,
                preferredIntervals: [0, 0, 7, 12],  // Repeats + octave leaps (percussive)
                contour: .static_,
                rangeLow: 48, rangeHigh: 67,   // C3-G4 (mid, tight range)
                attackSharpness: 1.0,
                sustainLength: 0.08,           // Extremely short — clicks and pops
                velocityVariation: 0.85,
                accentBeats: [0, 3, 6, 10, 13], // Syncopated accents
                responsive: false,
                anchor: false
            )

        case "fm-basic":  // Bellcrab — MELODY: Sparkling arpeggios
            return VoiceProfile(
                specimenName: "Bellcrab",
                beatPattern: [1.0, 0, 0.9, 0,  0.3, 0.8, 0, 0.7,  1.0, 0, 0.5, 0.9,  0, 0.6, 0, 0.3],  // Arpeggiated
                swing: 0.25,
                preferredIntervals: [2, 3, 4, 5, 7],  // Steps and skips (melodic)
                contour: .arch,
                rangeLow: 60, rangeHigh: 84,   // C4-C6 (bright, high)
                attackSharpness: 0.7,
                sustainLength: 0.4,
                velocityVariation: 0.7,
                accentBeats: [0, 4, 8, 12],     // On the beats
                responsive: true,
                anchor: false
            )

        // Deep Sources
        case "polyblep-tri":  // Glider — MELODY: Pure flute-like phrases
            return VoiceProfile(
                specimenName: "Glider",
                beatPattern: [0.9, 0, 0, 0.4,  0, 0.7, 0, 0,  0.8, 0, 0.3, 0,  0.6, 0, 0, 0],  // Gentle, breathing
                swing: 0.1,
                preferredIntervals: [2, 3, 5, -2, -3],  // Stepwise motion (flute-like)
                contour: .wave,
                rangeLow: 60, rangeHigh: 84,
                attackSharpness: 0.15,
                sustainLength: 0.7,
                velocityVariation: 0.5,
                accentBeats: [0, 8],
                responsive: true,
                anchor: false
            )

        case "noise-pink":  // Siltsift — BASS: Deep warm rumble
            return VoiceProfile(
                specimenName: "Siltsift",
                beatPattern: [1.0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0.5],  // Very sparse (drones)
                swing: 0.0,
                preferredIntervals: [0, 5, -5, 7],
                contour: .static_,
                rangeLow: 28, rangeHigh: 48,   // Very low (sub-bass territory)
                attackSharpness: 0.05,
                sustainLength: 1.0,            // Infinite sustain — drone
                velocityVariation: 0.05,
                accentBeats: [0],
                responsive: false,
                anchor: true
            )

        case "wt-analog":  // Morpheel — HARMONY: Shifting timbral pads
            return VoiceProfile(
                specimenName: "Morpheel",
                beatPattern: [0.7, 0, 0, 0,  0, 0, 0, 0.4,  0, 0, 0, 0,  0.5, 0, 0, 0],  // Sparse, evolving
                swing: 0.0,
                preferredIntervals: [3, 4, 5, 7, -3, -4],
                contour: .wave,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.1,
                sustainLength: 0.95,
                velocityVariation: 0.3,
                accentBeats: [0],
                responsive: true,
                anchor: false
            )

        case "wt-vocal":  // Chorale — MELODY: Vocal choir leads
            return VoiceProfile(
                specimenName: "Chorale",
                beatPattern: [1.0, 0, 0, 0,  0, 0, 0.8, 0,  0, 0, 0, 0,  0.6, 0, 0, 0],  // Slow, hymn-like
                swing: 0.0,
                preferredIntervals: [2, 3, 5, -2, -1],  // Stepwise (vocal)
                contour: .arch,
                rangeLow: 55, rangeHigh: 79,   // G3-G5 (vocal range)
                attackSharpness: 0.1,
                sustainLength: 0.9,
                velocityVariation: 0.4,
                accentBeats: [0, 6],
                responsive: true,
                anchor: false
            )

        // ═══════════════════════════════════════
        // PROCESSORS (Coral) — Sound shapers (texture/effect roles)
        // ═══════════════════════════════════════

        case "svf-lp":  // Curtain — TEXTURE: Slow filter sweeps
            return VoiceProfile(
                specimenName: "Curtain",
                beatPattern: [0.6, 0, 0, 0,  0, 0, 0, 0,  0.4, 0, 0, 0,  0, 0, 0, 0],
                swing: 0.0,
                preferredIntervals: [0, 2, -2],
                contour: .wave,
                rangeLow: 36, rangeHigh: 72,
                attackSharpness: 0.05,
                sustainLength: 1.0,
                velocityVariation: 0.1,
                accentBeats: [],
                responsive: true,
                anchor: false
            )

        case "shaper-hard":  // Bonecrush — EFFECT: Grit on accents
            return VoiceProfile(
                specimenName: "Bonecrush",
                beatPattern: [1.0, 0, 0, 0.5,  0, 0, 0, 0,  0.8, 0, 0, 0,  0, 0, 0.6, 0],
                swing: 0.0,
                preferredIntervals: [0, 7, 12],
                contour: .static_,
                rangeLow: 36, rangeHigh: 60,
                attackSharpness: 0.9,
                sustainLength: 0.2,
                velocityVariation: 0.8,
                accentBeats: [0, 8],
                responsive: false,
                anchor: false
            )

        case "feedback":  // Loopworm — TEXTURE: Karplus-Strong resonance
            return VoiceProfile(
                specimenName: "Loopworm",
                beatPattern: [0.8, 0, 0, 0,  0, 0.5, 0, 0,  0, 0, 0.7, 0,  0, 0, 0, 0.4],
                swing: 0.2,
                preferredIntervals: [7, 12, 5, -7],  // Wide intervals (resonant)
                contour: .descending,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.8,
                sustainLength: 0.6,
                velocityVariation: 0.3,
                accentBeats: [0, 10],
                responsive: true,
                anchor: false
            )

        case "svf-bp":  // Prism — TEXTURE: Wah-like vocal sweeps
            return VoiceProfile(
                specimenName: "Prism",
                beatPattern: [0, 0.7, 0, 0,  0.6, 0, 0, 0.5,  0, 0.8, 0, 0,  0.4, 0, 0, 0],
                swing: 0.3,
                preferredIntervals: [2, 3, -2, -3],
                contour: .wave,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.3,
                sustainLength: 0.5,
                velocityVariation: 0.6,
                accentBeats: [1, 9],
                responsive: true,
                anchor: false
            )

        // Deep Processors
        case "svf-hp":  // Razorgill — TEXTURE: Airy crystalline shimmer
            return VoiceProfile(
                specimenName: "Razorgill",
                beatPattern: [0.5, 0.3, 0.5, 0.3,  0.5, 0.3, 0.5, 0.3,  0.5, 0.3, 0.5, 0.3,  0.5, 0.3, 0.5, 0.3],
                swing: 0.0,
                preferredIntervals: [7, 12, -5],
                contour: .ascending,
                rangeLow: 60, rangeHigh: 96,
                attackSharpness: 0.4,
                sustainLength: 0.3,
                velocityVariation: 0.2,
                accentBeats: [0, 4, 8, 12],
                responsive: true,
                anchor: false
            )

        case "shaper-soft":  // Waxcoral — TEXTURE: Tape warmth
            return VoiceProfile(
                specimenName: "Waxcoral",
                beatPattern: [0.6, 0, 0, 0,  0, 0, 0, 0,  0.5, 0, 0, 0,  0, 0, 0, 0],
                swing: 0.0,
                preferredIntervals: [0, 2, 3, -2],
                contour: .static_,
                rangeLow: 36, rangeHigh: 72,
                attackSharpness: 0.05,
                sustainLength: 0.95,
                velocityVariation: 0.05,
                accentBeats: [],
                responsive: true,
                anchor: false
            )

        // ═══════════════════════════════════════
        // MODULATORS (Currents) — Movement controllers
        // ═══════════════════════════════════════

        case "adsr-fast":  // Snapper — RHYTHM: Percussive ghost notes
            return VoiceProfile(
                specimenName: "Snapper",
                beatPattern: [0.9, 0.4, 0.2, 0.7,  0.3, 0.8, 0.1, 0.5,  0.9, 0.3, 0.6, 0.2,  0.4, 0.7, 0.1, 0.8],  // Dense ghost notes
                swing: 0.4,
                preferredIntervals: [0, 0, 5, 7],
                contour: .static_,
                rangeLow: 48, rangeHigh: 60,
                attackSharpness: 1.0,
                sustainLength: 0.05,           // Instant decay — pure transient
                velocityVariation: 0.9,        // Ghost notes are quiet, accents are loud
                accentBeats: [0, 4, 8, 12],
                responsive: false,
                anchor: false
            )

        case "lfo-sine":  // Tidepulse — TEXTURE: Slow breathing modulation
            return VoiceProfile(
                specimenName: "Tidepulse",
                beatPattern: [0.3, 0, 0, 0,  0, 0, 0, 0,  0.3, 0, 0, 0,  0, 0, 0, 0],
                swing: 0.0,
                preferredIntervals: [0, 2, -2],
                contour: .wave,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.0,
                sustainLength: 1.0,
                velocityVariation: 0.0,
                accentBeats: [],
                responsive: true,
                anchor: false
            )

        case "vel-map":  // Strikescale — HARMONY: Velocity-responsive chord stabs
            return VoiceProfile(
                specimenName: "Strikescale",
                beatPattern: [1.0, 0, 0, 0,  0, 0, 1.0, 0,  0, 0, 0, 0,  1.0, 0, 0, 0],  // Strong chord stabs
                swing: 0.0,
                preferredIntervals: [3, 4, 7, -3, -4],
                contour: .static_,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.6,
                sustainLength: 0.5,
                velocityVariation: 1.0,        // Maximum dynamic range
                accentBeats: [0, 6, 12],
                responsive: true,
                anchor: false
            )

        case "lfo-random":  // Scramble — EFFECT: Random glitch bursts
            return VoiceProfile(
                specimenName: "Scramble",
                beatPattern: [0.3, 0.3, 0.3, 0.3,  0.3, 0.3, 0.3, 0.3,  0.3, 0.3, 0.3, 0.3,  0.3, 0.3, 0.3, 0.3],  // Random uniform
                swing: 0.5,
                preferredIntervals: [1, 6, 11, -1, -6],  // Chromatic + tritones (chaotic)
                contour: .static_,
                rangeLow: 36, rangeHigh: 96,   // Very wide range (unpredictable)
                attackSharpness: 0.85,
                sustainLength: 0.15,
                velocityVariation: 0.95,
                accentBeats: [],               // No predictable accents
                responsive: false,
                anchor: false
            )

        // Deep Modulators
        case "adsr-slow":  // Drifter — TEXTURE: Glacial pad swell
            return VoiceProfile(
                specimenName: "Drifter",
                beatPattern: [0.4, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0],  // One note per bar max
                swing: 0.0,
                preferredIntervals: [0, 2, 3, -2],
                contour: .wave,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.0,
                sustainLength: 1.0,
                velocityVariation: 0.0,
                accentBeats: [],
                responsive: true,
                anchor: false
            )

        case "at-map":  // Pressurewing — TEXTURE: Pressure-responsive expression
            return VoiceProfile(
                specimenName: "Pressurewing",
                beatPattern: [0.5, 0, 0, 0,  0, 0.4, 0, 0,  0.5, 0, 0, 0,  0, 0, 0.3, 0],
                swing: 0.0,
                preferredIntervals: [2, 5, -2, -5],
                contour: .arch,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.2,
                sustainLength: 0.7,
                velocityVariation: 0.8,
                accentBeats: [0, 8],
                responsive: true,
                anchor: false
            )

        // ═══════════════════════════════════════
        // EFFECTS (Tide Pools) — Spatial processing
        // ═══════════════════════════════════════

        case "delay-stereo":  // Echocave — EFFECT: Rhythmic echo timing
            return VoiceProfile(
                specimenName: "Echocave",
                beatPattern: [0.8, 0, 0, 0.6,  0, 0, 0.4, 0,  0, 0.3, 0, 0,  0.2, 0, 0, 0],  // Decaying echo
                swing: 0.0,
                preferredIntervals: [0, 7, 12],
                contour: .descending,
                rangeLow: 48, rangeHigh: 84,
                attackSharpness: 0.5,
                sustainLength: 0.4,
                velocityVariation: 0.5,
                accentBeats: [0],
                responsive: true,
                anchor: false
            )

        case "chorus-lush":  // Shimmer — TEXTURE: Chorus swells
            return VoiceProfile(
                specimenName: "Shimmer",
                beatPattern: [0.5, 0, 0, 0,  0, 0, 0, 0,  0.4, 0, 0, 0,  0, 0, 0, 0],
                swing: 0.0,
                preferredIntervals: [3, 4, 7, 12],
                contour: .ascending,
                rangeLow: 55, rangeHigh: 84,
                attackSharpness: 0.05,
                sustainLength: 0.95,
                velocityVariation: 0.1,
                accentBeats: [],
                responsive: true,
                anchor: false
            )

        case "reverb-hall":  // Cathedral — TEXTURE: Vast reverb washes
            return VoiceProfile(
                specimenName: "Cathedral",
                beatPattern: [0.3, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0],  // Extremely sparse
                swing: 0.0,
                preferredIntervals: [7, 12, 5],
                contour: .static_,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.0,
                sustainLength: 1.0,
                velocityVariation: 0.0,
                accentBeats: [],
                responsive: true,
                anchor: false
            )

        case "dist-warm":  // Ember — EFFECT: Warm saturation blooms
            return VoiceProfile(
                specimenName: "Ember",
                beatPattern: [0.7, 0, 0, 0.3,  0, 0, 0.5, 0,  0.6, 0, 0, 0,  0, 0.4, 0, 0],
                swing: 0.2,
                preferredIntervals: [0, 3, 5, 7],
                contour: .static_,
                rangeLow: 36, rangeHigh: 60,
                attackSharpness: 0.6,
                sustainLength: 0.5,
                velocityVariation: 0.5,
                accentBeats: [0, 8],
                responsive: false,
                anchor: false
            )

        default:
            // Fallback — neutral profile
            return VoiceProfile(
                specimenName: "Unknown",
                beatPattern: [0.5, 0, 0, 0,  0, 0, 0.5, 0,  0, 0, 0, 0,  0.5, 0, 0, 0],
                swing: 0.0,
                preferredIntervals: [2, 3, 5, 7],
                contour: .static_,
                rangeLow: 48, rangeHigh: 72,
                attackSharpness: 0.5,
                sustainLength: 0.5,
                velocityVariation: 0.5,
                accentBeats: [0, 8],
                responsive: true,
                anchor: false
            )
        }
    }
}
