import Foundation

/// Musical role that determines how a specimen contributes to a Dive arrangement.
/// Each role has distinct behavior for octave range, note selection, and rhythmic density.
enum MusicalRole: String, CaseIterable, Codable {
    case bass       // Root + fifth, low octave, sustained legato
    case melody     // Scale degrees, mid-high octave, arpeggiated
    case harmony    // Chord tones, mid octave, pad-like sustain
    case rhythm     // Short percussive hits, syncopated patterns
    case texture    // Filter sweeps, modulation — doesn't fire notes directly
    case effect     // Spatial processing — echoes, reverb, color. No notes.

    /// Octave offset from the zone's base octave
    var octaveOffset: Int {
        switch self {
        case .bass:    return -1
        case .melody:  return 1
        case .harmony: return 0
        case .rhythm:  return 0
        case .texture: return 0
        case .effect:  return 0
        }
    }

    /// Whether this role generates notes in the Dive
    var generatesNotes: Bool {
        switch self {
        case .bass, .melody, .harmony, .rhythm: return true
        case .texture, .effect: return false
        }
    }

    /// Preferred scale degrees (0-indexed within the current scale).
    /// Bass prefers root+fifth, melody prefers all degrees, harmony prefers chord tones.
    var preferredDegrees: [Int]? {
        switch self {
        case .bass:    return [0, 4]          // Root and fifth
        case .melody:  return nil             // All scale degrees (nil = no restriction)
        case .harmony: return [0, 2, 4, 6]   // 1st, 3rd, 5th, 7th (chord tones)
        case .rhythm:  return [0, 4]          // Root and fifth (percussive hits lock to tonality)
        case .texture: return nil
        case .effect:  return nil
        }
    }

    /// Rhythmic density multiplier (1.0 = zone default, <1 = sparser, >1 = denser)
    var rhythmDensity: Float {
        switch self {
        case .bass:    return 0.5    // Half the zone's density — bass is sparse
        case .melody:  return 1.0    // Matches zone density
        case .harmony: return 0.75   // Slightly sparser than melody
        case .rhythm:  return 1.5    // Denser — syncopated fills
        case .texture: return 0.0    // No rhythm (texture is continuous)
        case .effect:  return 0.0    // No rhythm
        }
    }

    /// Note duration multiplier (relative to zone default)
    var durationMultiplier: Float {
        switch self {
        case .bass:    return 2.0    // Long sustained notes
        case .melody:  return 1.0    // Zone default
        case .harmony: return 1.5    // Pad-like sustain
        case .rhythm:  return 0.3    // Very short — percussive
        case .texture: return 0.0
        case .effect:  return 0.0
        }
    }

    /// Velocity range for this role
    var velocityRange: ClosedRange<Float> {
        switch self {
        case .bass:    return 0.5...0.8    // Solid, consistent
        case .melody:  return 0.4...0.9    // Most dynamic range
        case .harmony: return 0.3...0.6    // Background, softer
        case .rhythm:  return 0.6...0.95   // Punchy
        case .texture: return 0.2...0.5
        case .effect:  return 0.2...0.4
        }
    }
}

/// Maps specimen catalog entries to their musical roles.
/// The mapping is based on each specimen's sonic character and category.
enum SpecimenRoleMap {

    /// Get the musical role for a specimen by its subtype ID
    static func role(for subtypeID: String) -> MusicalRole {
        switch subtypeID {
        // Sources
        case "polyblep-saw":    return .bass       // Sawfin — warm, full foundation
        case "polyblep-square": return .harmony    // Boxjelly — hollow, woody chords
        case "noise-white":     return .rhythm     // Foamspray — percussive noise
        case "fm-basic":        return .melody     // Bellcrab — metallic arpeggios
        case "polyblep-tri":    return .melody     // Glider (deep) — pure flute melody
        case "noise-pink":      return .bass       // Siltsift (deep) — deep warm rumble
        case "wt-analog":       return .harmony    // Morpheel (deep) — evolving pads
        case "wt-vocal":        return .melody     // Chorale (deep) — vocal lead

        // Processors — shape sound, don't generate primary notes
        case "svf-lp":          return .texture    // Curtain — filter sweep contour
        case "shaper-hard":     return .effect     // Bonecrush — adds grit
        case "feedback":        return .texture    // Loopworm — resonance
        case "svf-bp":          return .texture    // Prism — wah character
        case "svf-hp":          return .texture    // Razorgill (deep) — airy shimmer
        case "shaper-soft":     return .texture    // Waxcoral (deep) — tape warmth

        // Modulators — control movement
        case "adsr-fast":       return .rhythm     // Snapper — percussive triggers
        case "lfo-sine":        return .texture    // Tidepulse — breathing LFO
        case "vel-map":         return .harmony    // Strikescale — velocity-responsive chords
        case "lfo-random":      return .effect     // Scramble — chaotic glitch
        case "adsr-slow":       return .texture    // Drifter (deep) — slow swell
        case "at-map":          return .texture    // Pressurewing (deep) — pressure control

        // Effects — spatial processing
        case "delay-stereo":    return .effect     // Echocave — rhythmic echo
        case "chorus-lush":     return .texture    // Shimmer — width/chorus
        case "reverb-hall":     return .texture    // Cathedral — space
        case "dist-warm":       return .effect     // Ember — warmth/saturation

        default:                return .harmony    // Fallback
        }
    }
}
