import Foundation

/// Defines evolution paths for each core specimen at level 10
struct EvolutionPath {
    let aggressiveName: String    // Path A — high aggressiveScore
    let aggressiveSubtype: String // New subtype identifier
    let gentleName: String        // Path B — high gentleScore
    let gentleSubtype: String
}

enum EvolutionCatalog {
    static let paths: [String: EvolutionPath] = [
        // Sources
        "polyblep-saw":    EvolutionPath(aggressiveName: "Sawshark", aggressiveSubtype: "evo-sawshark",
                                          gentleName: "Sawdrift", gentleSubtype: "evo-sawdrift"),
        "polyblep-square": EvolutionPath(aggressiveName: "Ironjelly", aggressiveSubtype: "evo-ironjelly",
                                          gentleName: "Glassjelly", gentleSubtype: "evo-glassjelly"),
        "noise-white":     EvolutionPath(aggressiveName: "Stormspray", aggressiveSubtype: "evo-stormspray",
                                          gentleName: "Mistspray", gentleSubtype: "evo-mistspray"),
        "fm-basic":        EvolutionPath(aggressiveName: "Bellstrike", aggressiveSubtype: "evo-bellstrike",
                                          gentleName: "Bellchime", gentleSubtype: "evo-bellchime"),
        // Processors
        "svf-lp":          EvolutionPath(aggressiveName: "Razortide", aggressiveSubtype: "evo-razortide",
                                          gentleName: "Silkveil", gentleSubtype: "evo-silkveil"),
        "shaper-hard":     EvolutionPath(aggressiveName: "Skullcrush", aggressiveSubtype: "evo-skullcrush",
                                          gentleName: "Velvetcrush", gentleSubtype: "evo-velvetcrush"),
        "feedback":        EvolutionPath(aggressiveName: "Razorworm", aggressiveSubtype: "evo-razorworm",
                                          gentleName: "Silkworm", gentleSubtype: "evo-silkworm"),
        "svf-bp":          EvolutionPath(aggressiveName: "Shardprism", aggressiveSubtype: "evo-shardprism",
                                          gentleName: "Moonprism", gentleSubtype: "evo-moonprism"),
        // Modulators
        "adsr-fast":       EvolutionPath(aggressiveName: "Thundersnap", aggressiveSubtype: "evo-thundersnap",
                                          gentleName: "Gentlesnap", gentleSubtype: "evo-gentlesnap"),
        "lfo-sine":        EvolutionPath(aggressiveName: "Stormpulse", aggressiveSubtype: "evo-stormpulse",
                                          gentleName: "Moonpulse", gentleSubtype: "evo-moonpulse"),
        "vel-map":         EvolutionPath(aggressiveName: "Ironscale", aggressiveSubtype: "evo-ironscale",
                                          gentleName: "Shimmerscale", gentleSubtype: "evo-shimmerscale"),
        "lfo-random":      EvolutionPath(aggressiveName: "Chaosscramble", aggressiveSubtype: "evo-chaosscramble",
                                          gentleName: "Dreamscramble", gentleSubtype: "evo-dreamscramble"),
        // Effects
        "delay-stereo":    EvolutionPath(aggressiveName: "Abyssal Echo", aggressiveSubtype: "evo-abyssecho",
                                          gentleName: "Tidepool Echo", gentleSubtype: "evo-tidepoolecho"),
        "chorus-lush":     EvolutionPath(aggressiveName: "Tempest", aggressiveSubtype: "evo-tempest",
                                          gentleName: "Aurora", gentleSubtype: "evo-aurora"),
        "reverb-hall":     EvolutionPath(aggressiveName: "Void", aggressiveSubtype: "evo-void",
                                          gentleName: "Sanctuary", gentleSubtype: "evo-sanctuary"),
        "dist-warm":       EvolutionPath(aggressiveName: "Inferno", aggressiveSubtype: "evo-inferno",
                                          gentleName: "Hearth", gentleSubtype: "evo-hearth"),
    ]

    /// Determine which evolution path a specimen qualifies for.
    /// Returns nil if the specimen is below level 10 or has no registered path.
    static func evolvedForm(for specimen: Specimen) -> (name: String, subtype: String)? {
        guard specimen.level >= 10,
              let path = paths[specimen.subtype] else { return nil }

        if specimen.aggressiveScore > specimen.gentleScore {
            return (path.aggressiveName, path.aggressiveSubtype)
        } else {
            return (path.gentleName, path.gentleSubtype)
        }
    }
}
