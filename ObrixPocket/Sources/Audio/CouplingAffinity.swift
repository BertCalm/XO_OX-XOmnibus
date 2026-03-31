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
        // Original 8 pairs
        ["polyblep-saw",    "svf-lp"],       // Sawfin + Curtain = classic subtractive / warm filtered lead
        ["fm-basic",        "shaper-hard"],   // Bellcrab + Bonecrush = metallic aggression
        ["noise-white",     "feedback"],      // Foamspray + Loopworm = Karplus-Strong percussion
        ["polyblep-square", "svf-bp"],        // Boxjelly + Prism = vowel sculpting / spectral pad
        ["lfo-sine",        "svf-lp"],        // Tidepulse + Curtain = classic filter sweep
        ["adsr-fast",       "delay-stereo"],  // Snapper + Echocave = rhythmic echoes
        ["lfo-random",      "shaper-hard"],   // Scramble + Bonecrush = chaos + destruction
        ["vel-map",         "fm-basic"],      // Strikescale + Bellcrab = velocity-driven FM
        // Sprint 34 — 4 new resonant pairs (8 → 12 total)
        // Note: Sawfin+Curtain and Boxjelly+Prism were already paired above;
        // their Sprint 34 descriptions ("warm filtered lead", "spectral pad") are
        // captured in the comments above. The two genuinely new pairs below are
        // Glider+Drifter and Foamspray+Echocave, plus two additional pairs that
        // bring the set to a clean 12.
        ["polyblep-tri",    "adsr-slow"],     // Glider + Drifter = evolving texture
        ["noise-white",     "delay-stereo"],  // Foamspray + Echocave = ambient wash
        ["wt-analog",       "chorus-lush"],   // Morpheel + Shimmer = wide evolving pad
        ["lfo-sine",        "reverb-hall"],   // Tidepulse + Cathedral = breathing ambience
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
            // Original 8 pairs
            "polyblep-saw":    ["svf-lp"],
            "svf-lp":          ["polyblep-saw", "lfo-sine"],
            "fm-basic":        ["shaper-hard", "vel-map"],
            "shaper-hard":     ["fm-basic", "lfo-random"],
            "noise-white":     ["feedback", "delay-stereo"],   // +Foamspray+Echocave (Sprint 34)
            "feedback":        ["noise-white"],
            "polyblep-square": ["svf-bp"],
            "svf-bp":          ["polyblep-square"],
            "lfo-sine":        ["svf-lp", "reverb-hall"],      // +Tidepulse+Cathedral (Sprint 34)
            "adsr-fast":       ["delay-stereo"],
            "delay-stereo":    ["adsr-fast", "noise-white"],   // +Echocave+Foamspray (Sprint 34)
            "lfo-random":      ["shaper-hard"],
            "vel-map":         ["fm-basic"],
            // Sprint 34 — 4 new pairs
            "polyblep-tri":    ["adsr-slow"],                  // Glider + Drifter = evolving texture
            "adsr-slow":       ["polyblep-tri"],               // Drifter + Glider (reverse)
            "wt-analog":       ["chorus-lush"],                // Morpheel + Shimmer = wide evolving pad
            "chorus-lush":     ["wt-analog"],                  // Shimmer + Morpheel (reverse)
            "reverb-hall":     ["lfo-sine"],                   // Cathedral + Tidepulse = breathing ambience
        ]
        return pairs[subtype] ?? []
    }
}
