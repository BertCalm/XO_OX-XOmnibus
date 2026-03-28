import Foundation

/// Static catalog of OBRIX Pocket specimen types.
/// 16 core specimens (normal spawns) + 8 deep specimens (special unlock conditions).
/// Each entry maps a DSP subtype ID to its creature identity.
struct CatalogEntry {
    let subtypeID: String        // e.g., "polyblep-saw"
    let creatureName: String     // e.g., "Sawfin"
    let category: SpecimenCategory
    let sonicCharacter: String   // One-line description for UI tooltip
    let creatureConcept: String  // Visual identity hint
    let personalityLine: String  // Short personality tagline shown on catch + collection
    let isDeepSpecimen: Bool     // true = not available in normal spawns
    let unlockCondition: String  // Human-readable unlock hint (empty for core specimens)
    let defaultParams: [String: Float]  // Curated parameter preset for this specimen type

    init(
        subtypeID: String,
        creatureName: String,
        category: SpecimenCategory,
        sonicCharacter: String,
        creatureConcept: String,
        personalityLine: String = "",
        isDeepSpecimen: Bool = false,
        unlockCondition: String = "",
        defaultParams: [String: Float] = [:]
    ) {
        self.subtypeID = subtypeID
        self.creatureName = creatureName
        self.category = category
        self.sonicCharacter = sonicCharacter
        self.creatureConcept = creatureConcept
        self.personalityLine = personalityLine
        self.isDeepSpecimen = isDeepSpecimen
        self.unlockCondition = unlockCondition
        self.defaultParams = defaultParams
    }

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

/// The canonical specimen catalog — 16 core types + 8 deep types across 4 categories.
enum SpecimenCatalog {

    // MARK: - Sources (Shells)
    // 4 core + 4 deep

    static let sources: [CatalogEntry] = [
        // Core (normal spawns)
        // Sawfin — warm, full, classic subtractive starting point
        CatalogEntry(subtypeID: "polyblep-saw",    creatureName: "Sawfin",    category: .source,
                     sonicCharacter: "Classic sawtooth — warm, buzzy, detunable for supersaw",
                     creatureConcept: "Long-finned fish with serrated dorsal fin",
                     personalityLine: "Reliable. Warm. The reef's foundation.",
                     defaultParams: [
                         "obrix_src1Type": 0, "obrix_src1Tune": 0, "obrix_src1Level": 0.8,
                         "obrix_flt1Cutoff": 0.65, "obrix_flt1Resonance": 0.15,
                         "obrix_env1Attack": 0.01, "obrix_env1Decay": 0.3,
                         "obrix_env1Sustain": 0.7, "obrix_env1Release": 0.4,
                         "obrix_lfo1Rate": 0.3, "obrix_lfo1Depth": 0.1,
                     ]),
        // Boxjelly — hollow, woody, pulse-width modulated
        CatalogEntry(subtypeID: "polyblep-square",  creatureName: "Boxjelly",  category: .source,
                     sonicCharacter: "Hollow square wave — woody, clarinet-like",
                     creatureConcept: "Translucent cubic jellyfish that pulses",
                     personalityLine: "Hollow. Woody. Pulses in the dark.",
                     defaultParams: [
                         "obrix_src1Type": 1, "obrix_src1Tune": 0, "obrix_src1Level": 0.7,
                         "obrix_flt1Cutoff": 0.5, "obrix_flt1Resonance": 0.25,
                         "obrix_env1Attack": 0.02, "obrix_env1Decay": 0.4,
                         "obrix_env1Sustain": 0.5, "obrix_env1Release": 0.6,
                         "obrix_lfo1Rate": 1.5, "obrix_lfo1Depth": 0.2,
                     ]),
        // Foamspray — noisy, percussive, explosive
        CatalogEntry(subtypeID: "noise-white",      creatureName: "Foamspray", category: .source,
                     sonicCharacter: "Full-spectrum noise — ocean spray, breath, transients",
                     creatureConcept: "Sea urchin constantly shedding tiny spines",
                     personalityLine: "Chaos incarnate. The sound of everything at once.",
                     defaultParams: [
                         "obrix_src1Type": 3, "obrix_src1Tune": 0, "obrix_src1Level": 0.9,
                         "obrix_flt1Cutoff": 0.8, "obrix_flt1Resonance": 0.0,
                         "obrix_env1Attack": 0.001, "obrix_env1Decay": 0.15,
                         "obrix_env1Sustain": 0.1, "obrix_env1Release": 0.1,
                         "obrix_lfo1Rate": 0.0, "obrix_lfo1Depth": 0.0,
                     ]),
        // Bellcrab — metallic, bell-like, long sustain
        CatalogEntry(subtypeID: "fm-basic",         creatureName: "Bellcrab",  category: .source,
                     sonicCharacter: "FM synthesis — metallic bells, electric piano",
                     creatureConcept: "Hermit crab whose shell rings when struck",
                     personalityLine: "Metallic. Unpredictable. Rings like a struck bell.",
                     defaultParams: [
                         "obrix_src1Type": 5, "obrix_src1Tune": 0, "obrix_src1Level": 0.6,
                         "obrix_flt1Cutoff": 0.9, "obrix_flt1Resonance": 0.1,
                         "obrix_env1Attack": 0.001, "obrix_env1Decay": 0.8,
                         "obrix_env1Sustain": 0.3, "obrix_env1Release": 1.5,
                         "obrix_lfo1Rate": 0.1, "obrix_lfo1Depth": 0.05,
                     ]),

        // Deep (unlock required)
        CatalogEntry(subtypeID: "polyblep-tri",     creatureName: "Glider",    category: .source,
                     sonicCharacter: "Pure triangle — flute-like, gentle",
                     creatureConcept: "Manta ray with smooth triangular wings",
                     personalityLine: "Silent wings. Pure tone.",
                     isDeepSpecimen: true,
                     unlockCondition: "Evolve Sawfin to Lv.10"),
        CatalogEntry(subtypeID: "noise-pink",       creatureName: "Siltsift",  category: .source,
                     sonicCharacter: "Warm noise — rumble, wind, deep water",
                     creatureConcept: "Bottom-dwelling flatfish that stirs sediment",
                     personalityLine: "The sound beneath the sound.",
                     isDeepSpecimen: true,
                     unlockCondition: "Evolve Foamspray to Lv.10"),
        CatalogEntry(subtypeID: "wt-analog",        creatureName: "Morpheel",  category: .source,
                     sonicCharacter: "Wavetable scanning — evolving, shifting timbres",
                     creatureConcept: "Eel that changes color as it swims",
                     personalityLine: "Never the same twice.",
                     isDeepSpecimen: true,
                     unlockCondition: "Dive reward at 600m+"),
        CatalogEntry(subtypeID: "wt-vocal",         creatureName: "Chorale",   category: .source,
                     sonicCharacter: "Vocal formant wavetable — ahs, oohs, choir",
                     creatureConcept: "Cluster of tube worms that sing in harmony",
                     personalityLine: "The reef sings when Chorale appears.",
                     isDeepSpecimen: true,
                     unlockCondition: "Dive reward at 800m+"),
    ]

    // MARK: - Processors (Coral)
    // 4 core + 2 deep

    static let processors: [CatalogEntry] = [
        // Core (normal spawns)
        // Curtain — warm low-pass, moderate resonance, slow sweep
        CatalogEntry(subtypeID: "svf-lp",           creatureName: "Curtain",    category: .processor,
                     sonicCharacter: "Low-pass filter — warmth, classic subtractive sweep",
                     creatureConcept: "Flowing sea curtain anemone",
                     personalityLine: "Gentle gatekeeper. Only the worthy frequencies pass.",
                     defaultParams: [
                         "obrix_flt1Cutoff": 0.45, "obrix_flt1Resonance": 0.3,
                         "obrix_flt1EnvDepth": 0.3,
                         "obrix_env1Attack": 0.05, "obrix_env1Decay": 0.5,
                         "obrix_env1Sustain": 0.4, "obrix_env1Release": 0.5,
                     ]),
        // Bonecrush — aggressive, driven, fast attack
        CatalogEntry(subtypeID: "shaper-hard",      creatureName: "Bonecrush",  category: .processor,
                     sonicCharacter: "Hard clipping — aggressive distortion",
                     creatureConcept: "Mantis shrimp with devastating strike force",
                     personalityLine: "Devastation with purpose. Every harmonic earned.",
                     defaultParams: [
                         "obrix_flt1Cutoff": 0.85, "obrix_flt1Resonance": 0.0,
                         "obrix_flt1EnvDepth": -0.2,
                         "obrix_env1Attack": 0.001, "obrix_env1Decay": 0.2,
                         "obrix_env1Sustain": 0.8, "obrix_env1Release": 0.3,
                     ]),
        // Loopworm — plucky, feedback-rich, Karplus-Strong character
        CatalogEntry(subtypeID: "feedback",         creatureName: "Loopworm",   category: .processor,
                     sonicCharacter: "Feedback path — Karplus-Strong pluck, metallic tones",
                     creatureConcept: "Tube worm that recirculates water endlessly",
                     personalityLine: "What goes in, comes back changed. And changed again.",
                     defaultParams: [
                         "obrix_flt1Cutoff": 0.6, "obrix_flt1Resonance": 0.5,
                         "obrix_flt1EnvDepth": 0.4,
                         "obrix_env1Attack": 0.001, "obrix_env1Decay": 0.6,
                         "obrix_env1Sustain": 0.0, "obrix_env1Release": 0.8,
                     ]),
        // Prism — vocal, wah-like, narrow bandpass
        CatalogEntry(subtypeID: "svf-bp",           creatureName: "Prism",      category: .processor,
                     sonicCharacter: "Band-pass filter — wah-like, vocal",
                     creatureConcept: "Translucent prism shrimp that refracts light",
                     personalityLine: "Isolates the voice within the noise.",
                     defaultParams: [
                         "obrix_flt1Cutoff": 0.35, "obrix_flt1Resonance": 0.6,
                         "obrix_flt1EnvDepth": 0.5,
                         "obrix_env1Attack": 0.03, "obrix_env1Decay": 0.3,
                         "obrix_env1Sustain": 0.6, "obrix_env1Release": 0.4,
                     ]),

        // Deep (unlock required)
        CatalogEntry(subtypeID: "svf-hp",           creatureName: "Razorgill",  category: .processor,
                     sonicCharacter: "High-pass filter — airiness, thin metallic character",
                     creatureConcept: "Sharp-edged fish with crystalline gill slits",
                     personalityLine: "Cuts away everything but the edge.",
                     isDeepSpecimen: true,
                     unlockCondition: "Evolve Curtain to Lv.10"),
        CatalogEntry(subtypeID: "shaper-soft",      creatureName: "Waxcoral",   category: .processor,
                     sonicCharacter: "Soft saturation — tape warmth, gentle harmonics",
                     creatureConcept: "Soft coral that gently bends incoming current",
                     personalityLine: "Softens the sharp. Warms the cold.",
                     isDeepSpecimen: true,
                     unlockCondition: "Evolve Bonecrush to Lv.10"),
    ]

    // MARK: - Modulators (Currents)
    // 4 core + 2 deep

    static let modulators: [CatalogEntry] = [
        // Core (normal spawns)
        // Snapper — fast, percussive, punchy
        CatalogEntry(subtypeID: "adsr-fast",        creatureName: "Snapper",       category: .modulator,
                     sonicCharacter: "Fast attack/decay — percussive, plucky",
                     creatureConcept: "Pistol shrimp that fires instantly",
                     personalityLine: "Strikes first. Asks questions never.",
                     defaultParams: [
                         "obrix_lfo1Rate": 0.0, "obrix_lfo1Depth": 0.0,
                         "obrix_env1Attack": 0.001, "obrix_env1Decay": 0.08,
                         "obrix_env1Sustain": 0.0, "obrix_env1Release": 0.05,
                     ]),
        // Tidepulse — slow, breathing, meditative
        CatalogEntry(subtypeID: "lfo-sine",         creatureName: "Tidepulse",     category: .modulator,
                     sonicCharacter: "Smooth sine LFO — vibrato, tremolo",
                     creatureConcept: "Sea anemone that pulses with the tide",
                     personalityLine: "Breathes. The reef breathes with it.",
                     defaultParams: [
                         "obrix_lfo1Rate": 0.2, "obrix_lfo1Depth": 0.4,
                         "obrix_env1Attack": 0.1, "obrix_env1Decay": 0.5,
                         "obrix_env1Sustain": 0.7, "obrix_env1Release": 1.0,
                     ]),
        // Strikescale — velocity-responsive, dynamic
        CatalogEntry(subtypeID: "vel-map",          creatureName: "Strikescale",   category: .modulator,
                     sonicCharacter: "Velocity mapping — harder = brighter/louder",
                     creatureConcept: "Fish with scales that flash under pressure",
                     personalityLine: "Harder you push, brighter it gets.",
                     defaultParams: [
                         "obrix_lfo1Rate": 0.0, "obrix_lfo1Depth": 0.0,
                         "obrix_env1Attack": 0.01, "obrix_env1Decay": 0.2,
                         "obrix_env1Sustain": 0.5, "obrix_env1Release": 0.3,
                     ]),
        // Scramble — chaotic, unpredictable, glitchy
        CatalogEntry(subtypeID: "lfo-random",       creatureName: "Scramble",      category: .modulator,
                     sonicCharacter: "Random/S&H LFO — unpredictable, glitchy",
                     creatureConcept: "Octopus flashing random chromatophore patterns",
                     personalityLine: "Nobody knows what happens next. Not even Scramble.",
                     defaultParams: [
                         "obrix_lfo1Rate": 5.0, "obrix_lfo1Depth": 0.6,
                         "obrix_env1Attack": 0.01, "obrix_env1Decay": 0.15,
                         "obrix_env1Sustain": 0.3, "obrix_env1Release": 0.2,
                     ]),

        // Deep (unlock required)
        CatalogEntry(subtypeID: "adsr-slow",        creatureName: "Drifter",       category: .modulator,
                     sonicCharacter: "Slow pad envelope — gradual swell, long sustain",
                     creatureConcept: "Moon jellyfish that floats on currents",
                     personalityLine: "Patience. The long fade.",
                     isDeepSpecimen: true,
                     unlockCondition: "Evolve Snapper to Lv.10"),
        CatalogEntry(subtypeID: "at-map",           creatureName: "Pressurewing",  category: .modulator,
                     sonicCharacter: "Aftertouch/pressure mapping — squeeze for expression",
                     creatureConcept: "Stingray responsive to water pressure",
                     personalityLine: "Squeeze harder. It responds.",
                     isDeepSpecimen: true,
                     unlockCondition: "Evolve Strikescale to Lv.10"),
    ]

    // MARK: - Effects (Tide Pools)
    // 4 core + 0 deep

    static let effects: [CatalogEntry] = [
        // Core (normal spawns — all 4 effects are core)
        // Echocave — rhythmic echoes, moderate feedback
        CatalogEntry(subtypeID: "delay-stereo",     creatureName: "Echocave",   category: .effect,
                     sonicCharacter: "Stereo delay — rhythmic echoes, ping-pong",
                     creatureConcept: "Cave-dwelling shrimp in a resonant hollow",
                     personalityLine: "Every sound returns. Eventually.",
                     defaultParams: [
                         "obrix_fx1Mix": 0.4, "obrix_fx1Param1": 0.5,
                         "obrix_env1Attack": 0.01, "obrix_env1Decay": 0.3,
                         "obrix_env1Sustain": 0.6, "obrix_env1Release": 0.5,
                     ]),
        // Shimmer — lush, wide, chorus-rich
        CatalogEntry(subtypeID: "chorus-lush",      creatureName: "Shimmer",    category: .effect,
                     sonicCharacter: "Rich chorus — doubles, detunes, widens",
                     creatureConcept: "School of bioluminescent fish",
                     personalityLine: "Makes one voice sound like a choir.",
                     defaultParams: [
                         "obrix_fx1Mix": 0.5, "obrix_fx1Param1": 0.6,
                         "obrix_env1Attack": 0.05, "obrix_env1Decay": 0.5,
                         "obrix_env1Sustain": 0.8, "obrix_env1Release": 0.8,
                     ]),
        // Cathedral — vast, long tail, immersive
        CatalogEntry(subtypeID: "reverb-hall",      creatureName: "Cathedral",  category: .effect,
                     sonicCharacter: "Large hall reverb — vast, oceanic, immersive",
                     creatureConcept: "Massive underwater cavern",
                     personalityLine: "The space between the notes.",
                     defaultParams: [
                         "obrix_fx1Mix": 0.6, "obrix_fx1Param1": 0.8,
                         "obrix_env1Attack": 0.1, "obrix_env1Decay": 0.8,
                         "obrix_env1Sustain": 0.7, "obrix_env1Release": 2.0,
                     ]),
        // Ember — warm, gritty, saturated
        CatalogEntry(subtypeID: "dist-warm",        creatureName: "Ember",      category: .effect,
                     sonicCharacter: "Warm analog distortion — tubes, tape, heat",
                     creatureConcept: "Hydrothermal vent creature",
                     personalityLine: "Warmth with teeth.",
                     defaultParams: [
                         "obrix_fx1Mix": 0.35, "obrix_fx1Param1": 0.7,
                         "obrix_env1Attack": 0.01, "obrix_env1Decay": 0.3,
                         "obrix_env1Sustain": 0.7, "obrix_env1Release": 0.4,
                     ]),
    ]

    // MARK: - Unified Access

    /// All 24 catalog entries in order (16 core + 8 deep)
    static let all: [CatalogEntry] = sources + processors + modulators + effects

    /// The 16 core specimens available through normal spawns
    static var coreSpecimens: [CatalogEntry] { all.filter { !$0.isDeepSpecimen } }

    /// The 8 deep specimens that require special unlock conditions
    static var deepSpecimens: [CatalogEntry] { all.filter { $0.isDeepSpecimen } }

    /// Count of core specimens (should always be 16)
    static var coreCount: Int { coreSpecimens.count }

    /// Look up a catalog entry by its subtype ID (case-insensitive)
    static func entry(for subtypeID: String) -> CatalogEntry? {
        let normalized = subtypeID.lowercased()
        return all.first { $0.subtypeID == normalized }
    }

    /// Look up by index within the CORE specimens (0-15). Used by hash generator.
    static func entry(at index: Int) -> CatalogEntry? {
        guard index >= 0, index < coreSpecimens.count else { return nil }
        return coreSpecimens[index]
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
