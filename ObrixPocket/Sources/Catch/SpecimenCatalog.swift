import Foundation

/// Static catalog of the 24 OBRIX Pocket specimen types.
/// Each entry maps a DSP subtype ID to its creature identity.
struct CatalogEntry {
    let subtypeID: String        // e.g., "polyblep-saw"
    let creatureName: String     // e.g., "Sawfin"
    let category: SpecimenCategory
    let sonicCharacter: String   // One-line description for UI tooltip
    let creatureConcept: String  // Visual identity hint

    /// Display name with rarity prefix
    func displayName(rarity: SpecimenRarity, morphIndex: Int = 0) -> String {
        let morphPrefix = morphIndex == 1 ? "Drift " : ""
        switch rarity {
        case .common:    return "\(morphPrefix)\(creatureName)"
        case .uncommon:  return "Deep \(morphPrefix)\(creatureName)"
        case .rare:      return "Spectral \(morphPrefix)\(creatureName)"
        case .legendary: return "Abyssal \(morphPrefix)\(creatureName)"
        }
    }
}

/// The canonical specimen catalog — 24 types across 4 categories.
enum SpecimenCatalog {

    // MARK: - Sources (Shells) — 8 subtypes

    static let sources: [CatalogEntry] = [
        CatalogEntry(subtypeID: "polyblep-saw",    creatureName: "Sawfin",    category: .source,
                     sonicCharacter: "Classic sawtooth — warm, buzzy, detunable for supersaw",
                     creatureConcept: "Long-finned fish with serrated dorsal fin"),
        CatalogEntry(subtypeID: "polyblep-square",  creatureName: "Boxjelly",  category: .source,
                     sonicCharacter: "Hollow square wave — woody, clarinet-like",
                     creatureConcept: "Translucent cubic jellyfish that pulses"),
        CatalogEntry(subtypeID: "polyblep-tri",     creatureName: "Glider",    category: .source,
                     sonicCharacter: "Pure triangle — flute-like, gentle",
                     creatureConcept: "Manta ray with smooth triangular wings"),
        CatalogEntry(subtypeID: "noise-white",      creatureName: "Foamspray", category: .source,
                     sonicCharacter: "Full-spectrum noise — ocean spray, breath, transients",
                     creatureConcept: "Sea urchin constantly shedding tiny spines"),
        CatalogEntry(subtypeID: "noise-pink",       creatureName: "Siltsift",  category: .source,
                     sonicCharacter: "Warm noise — rumble, wind, deep water",
                     creatureConcept: "Bottom-dwelling flatfish that stirs sediment"),
        CatalogEntry(subtypeID: "wt-analog",        creatureName: "Morpheel",  category: .source,
                     sonicCharacter: "Wavetable scanning — evolving, shifting timbres",
                     creatureConcept: "Eel that changes color as it swims"),
        CatalogEntry(subtypeID: "wt-vocal",         creatureName: "Chorale",   category: .source,
                     sonicCharacter: "Vocal formant wavetable — ahs, oohs, choir",
                     creatureConcept: "Cluster of tube worms that sing in harmony"),
        CatalogEntry(subtypeID: "fm-basic",         creatureName: "Bellcrab",  category: .source,
                     sonicCharacter: "FM synthesis — metallic bells, electric piano",
                     creatureConcept: "Hermit crab whose shell rings when struck"),
    ]

    // MARK: - Processors (Coral) — 6 subtypes

    static let processors: [CatalogEntry] = [
        CatalogEntry(subtypeID: "svf-lp",           creatureName: "Curtain",    category: .processor,
                     sonicCharacter: "Low-pass filter — warmth, classic subtractive sweep",
                     creatureConcept: "Flowing sea curtain anemone"),
        CatalogEntry(subtypeID: "svf-hp",           creatureName: "Razorgill",  category: .processor,
                     sonicCharacter: "High-pass filter — airiness, thin metallic character",
                     creatureConcept: "Sharp-edged fish with crystalline gill slits"),
        CatalogEntry(subtypeID: "svf-bp",           creatureName: "Prism",      category: .processor,
                     sonicCharacter: "Band-pass filter — wah-like, vocal",
                     creatureConcept: "Translucent prism shrimp that refracts light"),
        CatalogEntry(subtypeID: "shaper-soft",      creatureName: "Waxcoral",   category: .processor,
                     sonicCharacter: "Soft saturation — tape warmth, gentle harmonics",
                     creatureConcept: "Soft coral that gently bends incoming current"),
        CatalogEntry(subtypeID: "shaper-hard",      creatureName: "Bonecrush",  category: .processor,
                     sonicCharacter: "Hard clipping — aggressive distortion",
                     creatureConcept: "Mantis shrimp with devastating strike force"),
        CatalogEntry(subtypeID: "feedback",         creatureName: "Loopworm",   category: .processor,
                     sonicCharacter: "Feedback path — Karplus-Strong pluck, metallic tones",
                     creatureConcept: "Tube worm that recirculates water endlessly"),
    ]

    // MARK: - Modulators (Currents) — 6 subtypes

    static let modulators: [CatalogEntry] = [
        CatalogEntry(subtypeID: "adsr-fast",        creatureName: "Snapper",       category: .modulator,
                     sonicCharacter: "Fast attack/decay — percussive, plucky",
                     creatureConcept: "Pistol shrimp that fires instantly"),
        CatalogEntry(subtypeID: "adsr-slow",        creatureName: "Drifter",       category: .modulator,
                     sonicCharacter: "Slow pad envelope — gradual swell, long sustain",
                     creatureConcept: "Moon jellyfish that floats on currents"),
        CatalogEntry(subtypeID: "lfo-sine",         creatureName: "Tidepulse",     category: .modulator,
                     sonicCharacter: "Smooth sine LFO — vibrato, tremolo",
                     creatureConcept: "Sea anemone that pulses with the tide"),
        CatalogEntry(subtypeID: "lfo-random",       creatureName: "Scramble",      category: .modulator,
                     sonicCharacter: "Random/S&H LFO — unpredictable, glitchy",
                     creatureConcept: "Octopus flashing random chromatophore patterns"),
        CatalogEntry(subtypeID: "vel-map",          creatureName: "Strikescale",   category: .modulator,
                     sonicCharacter: "Velocity mapping — harder = brighter/louder",
                     creatureConcept: "Fish with scales that flash under pressure"),
        CatalogEntry(subtypeID: "at-map",           creatureName: "Pressurewing",  category: .modulator,
                     sonicCharacter: "Aftertouch/pressure mapping — squeeze for expression",
                     creatureConcept: "Stingray responsive to water pressure"),
    ]

    // MARK: - Effects (Tide Pools) — 4 subtypes

    static let effects: [CatalogEntry] = [
        CatalogEntry(subtypeID: "delay-stereo",     creatureName: "Echocave",   category: .effect,
                     sonicCharacter: "Stereo delay — rhythmic echoes, ping-pong",
                     creatureConcept: "Cave-dwelling shrimp in a resonant hollow"),
        CatalogEntry(subtypeID: "chorus-lush",      creatureName: "Shimmer",    category: .effect,
                     sonicCharacter: "Rich chorus — doubles, detunes, widens",
                     creatureConcept: "School of bioluminescent fish"),
        CatalogEntry(subtypeID: "reverb-hall",      creatureName: "Cathedral",  category: .effect,
                     sonicCharacter: "Large hall reverb — vast, oceanic, immersive",
                     creatureConcept: "Massive underwater cavern"),
        CatalogEntry(subtypeID: "dist-warm",        creatureName: "Ember",      category: .effect,
                     sonicCharacter: "Warm analog distortion — tubes, tape, heat",
                     creatureConcept: "Hydrothermal vent creature"),
    ]

    // MARK: - Unified Access

    /// All 24 catalog entries in order
    static let all: [CatalogEntry] = sources + processors + modulators + effects

    /// Look up a catalog entry by its subtype ID (case-insensitive)
    static func entry(for subtypeID: String) -> CatalogEntry? {
        let normalized = subtypeID.lowercased()
        return all.first { $0.subtypeID == normalized }
    }

    /// Look up by index (0-23). Used by hash generator.
    static func entry(at index: Int) -> CatalogEntry? {
        guard index >= 0, index < all.count else { return nil }
        return all[index]
    }

    /// Map legacy SpawnManager subtype strings to catalog subtype IDs
    static func catalogSubtypeID(from spawnSubtype: String) -> String {
        // SpawnManager uses "PolyBLEP-Saw" format; catalog uses "polyblep-saw"
        let mapping: [String: String] = [
            "PolyBLEP-Saw":      "polyblep-saw",
            "PolyBLEP-Square":   "polyblep-square",
            "PolyBLEP-Tri":      "polyblep-tri",
            "Noise-White":       "noise-white",
            "Noise-Pink":        "noise-pink",
            "Wavetable-Analog":  "wt-analog",
            "Wavetable-Vocal":   "wt-vocal",
            "FM-Basic":          "fm-basic",
            "SVF-LP":            "svf-lp",
            "SVF-HP":            "svf-hp",
            "SVF-BP":            "svf-bp",
            "Waveshaper-Soft":   "shaper-soft",
            "Waveshaper-Hard":   "shaper-hard",
            "Feedback-Path":     "feedback",
            "ADSR-Fast":         "adsr-fast",
            "ADSR-Slow":         "adsr-slow",
            "LFO-Sine":          "lfo-sine",
            "LFO-Random":        "lfo-random",
            "Velocity-Map":      "vel-map",
            "Aftertouch-Map":    "at-map",
            "Delay-Stereo":      "delay-stereo",
            "Chorus-Lush":       "chorus-lush",
            "Reverb-Hall":       "reverb-hall",
            "Distortion-Warm":   "dist-warm",
        ]
        return mapping[spawnSubtype] ?? spawnSubtype.lowercased()
    }
}
