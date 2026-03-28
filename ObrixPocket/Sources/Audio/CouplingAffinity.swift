import Foundation

enum CouplingAffinity {
    case high    // Natural pair — bonus spectral drift + faster XP
    case medium  // Compatible — normal behavior
    case low     // Unusual — works but no bonus
    case invalid // Can't connect (Source→Source)

    /// Determine affinity between two specimen types
    static func between(_ a: Specimen, _ b: Specimen) -> CouplingAffinity {
        // Source → Source = invalid
        if a.category == .source && b.category == .source { return .invalid }

        // High affinity pairs (from the specimen catalog lore)
        let pair: Set<String> = [a.subtype, b.subtype]
        if highAffinityPairs.contains(pair) { return .high }

        // Source → Processor or Source → Effect = medium (natural flow)
        if (a.category == .source && (b.category == .processor || b.category == .effect)) ||
           (b.category == .source && (a.category == .processor || a.category == .effect)) {
            return .medium
        }

        // Modulator → anything = medium
        if a.category == .modulator || b.category == .modulator { return .medium }

        return .low
    }

    /// High affinity pairs — from the specimen catalog lore
    private static let highAffinityPairs: Set<Set<String>> = [
        ["polyblep-saw",    "svf-lp"],       // Sawfin + Curtain = classic subtractive
        ["fm-basic",        "shaper-hard"],   // Bellcrab + Bonecrush = metallic aggression
        ["noise-white",     "feedback"],      // Foamspray + Loopworm = Karplus-Strong percussion
        ["polyblep-square", "svf-bp"],        // Boxjelly + Prism = vowel sculpting
        ["lfo-sine",        "svf-lp"],        // Tidepulse + Curtain = classic filter sweep
        ["adsr-fast",       "delay-stereo"],  // Snapper + Echocave = rhythmic echoes
        ["lfo-random",      "shaper-hard"],   // Scramble + Bonecrush = chaos + destruction
        ["vel-map",         "fm-basic"],      // Strikescale + Bellcrab = velocity-driven FM
    ]

    /// Bonus multiplier for spectral drift
    var driftMultiplier: Float {
        switch self {
        case .high:    return 2.0  // Double drift — they evolve together faster
        case .medium:  return 1.0
        case .low:     return 0.5  // Slow drift
        case .invalid: return 0.0
        }
    }

    /// Bonus XP when playing through this pair
    var xpBonus: Int {
        switch self {
        case .high:    return 2  // Extra XP per note
        case .medium:  return 0
        case .low:     return 0
        case .invalid: return 0
        }
    }

    /// Return all subtype IDs that have high affinity with the given subtype
    static func affinitySubtypes(for subtype: String) -> [String] {
        let pairs: [String: [String]] = [
            "polyblep-saw":    ["svf-lp"],
            "svf-lp":          ["polyblep-saw", "lfo-sine"],
            "fm-basic":        ["shaper-hard", "vel-map"],
            "shaper-hard":     ["fm-basic", "lfo-random"],
            "noise-white":     ["feedback"],
            "feedback":        ["noise-white"],
            "polyblep-square": ["svf-bp"],
            "svf-bp":          ["polyblep-square"],
            "lfo-sine":        ["svf-lp"],
            "adsr-fast":       ["delay-stereo"],
            "delay-stereo":    ["adsr-fast"],
            "lfo-random":      ["shaper-hard"],
            "vel-map":         ["fm-basic"],
        ]
        return pairs[subtype] ?? []
    }
}
