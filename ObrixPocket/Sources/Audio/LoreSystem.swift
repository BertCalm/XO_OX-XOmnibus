import Foundation

// MARK: - LoreCategory

enum LoreCategory: String, Codable, CaseIterable {
    case origins     = "Origins"
    case specimens   = "Specimens"
    case deepOcean   = "Deep Ocean"
    case coupling    = "Coupling"
    case theAncients = "The Ancients"
    case theSong     = "The Song"

    var displayName: String { rawValue }

    var symbolName: String {
        switch self {
        case .origins:     return "drop.fill"
        case .specimens:   return "fish.fill"
        case .deepOcean:   return "water.waves"
        case .coupling:    return "point.3.connected.trianglepath.dotted"
        case .theAncients: return "seal.fill"
        case .theSong:     return "music.note"
        }
    }
}

// MARK: - DiscoveryCondition

enum DiscoveryCondition: Codable, Equatable {
    case catchSpecimen(subtypeId: String)
    case breedCombination(parentA: String, parentB: String)
    case reachBiome(biome: String)
    case seasonFirst(season: String)
    case achievementUnlocked(achievementId: String)
    case diveDepth(depth: Int)
    case reefAge(days: Int)

    // MARK: Codable

    private enum CodingKeys: String, CodingKey {
        case type, subtypeId, parentA, parentB, biome, season, achievementId, depth, days
    }

    func encode(to encoder: Encoder) throws {
        var container = encoder.container(keyedBy: CodingKeys.self)
        switch self {
        case .catchSpecimen(let subtypeId):
            try container.encode("catchSpecimen", forKey: .type)
            try container.encode(subtypeId, forKey: .subtypeId)
        case .breedCombination(let parentA, let parentB):
            try container.encode("breedCombination", forKey: .type)
            try container.encode(parentA, forKey: .parentA)
            try container.encode(parentB, forKey: .parentB)
        case .reachBiome(let biome):
            try container.encode("reachBiome", forKey: .type)
            try container.encode(biome, forKey: .biome)
        case .seasonFirst(let season):
            try container.encode("seasonFirst", forKey: .type)
            try container.encode(season, forKey: .season)
        case .achievementUnlocked(let achievementId):
            try container.encode("achievementUnlocked", forKey: .type)
            try container.encode(achievementId, forKey: .achievementId)
        case .diveDepth(let depth):
            try container.encode("diveDepth", forKey: .type)
            try container.encode(depth, forKey: .depth)
        case .reefAge(let days):
            try container.encode("reefAge", forKey: .type)
            try container.encode(days, forKey: .days)
        }
    }

    init(from decoder: Decoder) throws {
        let container = try decoder.container(keyedBy: CodingKeys.self)
        let type = try container.decode(String.self, forKey: .type)
        switch type {
        case "catchSpecimen":
            self = .catchSpecimen(subtypeId: try container.decode(String.self, forKey: .subtypeId))
        case "breedCombination":
            self = .breedCombination(
                parentA: try container.decode(String.self, forKey: .parentA),
                parentB: try container.decode(String.self, forKey: .parentB)
            )
        case "reachBiome":
            self = .reachBiome(biome: try container.decode(String.self, forKey: .biome))
        case "seasonFirst":
            self = .seasonFirst(season: try container.decode(String.self, forKey: .season))
        case "achievementUnlocked":
            self = .achievementUnlocked(achievementId: try container.decode(String.self, forKey: .achievementId))
        case "diveDepth":
            self = .diveDepth(depth: try container.decode(Int.self, forKey: .depth))
        case "reefAge":
            self = .reefAge(days: try container.decode(Int.self, forKey: .days))
        default:
            self = .reachBiome(biome: "surface")
        }
    }
}

// MARK: - LoreFragment

struct LoreFragment: Identifiable, Codable {
    let id: String
    let title: String
    let text: String
    let category: LoreCategory
    let discoveryCondition: DiscoveryCondition
    var isDiscovered: Bool
    var discoveredDate: Date?

    var shortPreview: String {
        let words = text.split(separator: " ").prefix(8).joined(separator: " ")
        return words + "…"
    }
}

// MARK: - LoreCodex

final class LoreCodex: ObservableObject {

    static let shared = LoreCodex()

    @Published var fragments: [LoreFragment] = []
    @Published var recentlyDiscovered: [LoreFragment] = []

    private let storageKey = "obrix_lore_codex"

    init() {
        load()
        if fragments.isEmpty { generateFragments() }
    }

    // MARK: - Computed Properties

    var discoveredFragments: [LoreFragment] { fragments.filter { $0.isDiscovered } }
    var undiscoveredFragments: [LoreFragment] { fragments.filter { !$0.isDiscovered } }

    var loreCompletionPercentage: Double {
        guard !fragments.isEmpty else { return 0.0 }
        return Double(discoveredFragments.count) / Double(fragments.count) * 100.0
    }

    var fragmentsByCategory: [LoreCategory: [LoreFragment]] {
        Dictionary(grouping: fragments, by: { $0.category })
    }

    var discoveredByCategory: [LoreCategory: [LoreFragment]] {
        Dictionary(grouping: discoveredFragments, by: { $0.category })
    }

    /// Fragments in the canonical reading order (discovered only, by category then index)
    var readingOrder: [LoreFragment] {
        let order: [LoreCategory] = [.origins, .deepOcean, .theAncients, .specimens, .coupling, .theSong]
        return order.flatMap { cat in
            (discoveredByCategory[cat] ?? []).sorted { $0.id < $1.id }
        }
    }

    // MARK: - Discovery

    func checkCatchSpecimen(subtypeId: String) {
        checkCondition(.catchSpecimen(subtypeId: subtypeId))
    }

    func checkBreedCombination(parentA: String, parentB: String) {
        checkCondition(.breedCombination(parentA: parentA, parentB: parentB))
        // Also check reversed order
        checkCondition(.breedCombination(parentA: parentB, parentB: parentA))
    }

    func checkBiome(biome: String) {
        checkCondition(.reachBiome(biome: biome))
    }

    func checkSeason(season: String) {
        checkCondition(.seasonFirst(season: season))
    }

    func checkAchievement(achievementId: String) {
        checkCondition(.achievementUnlocked(achievementId: achievementId))
    }

    func checkDiveDepth(depth: Int) {
        for fragment in undiscoveredFragments {
            if case .diveDepth(let required) = fragment.discoveryCondition, depth >= required {
                discover(fragmentId: fragment.id)
            }
        }
    }

    func checkReefAge(days: Int) {
        for fragment in undiscoveredFragments {
            if case .reefAge(let required) = fragment.discoveryCondition, days >= required {
                discover(fragmentId: fragment.id)
            }
        }
    }

    func clearRecentlyDiscovered() {
        recentlyDiscovered = []
    }

    // MARK: - Private

    private func checkCondition(_ condition: DiscoveryCondition) {
        for fragment in undiscoveredFragments where fragment.discoveryCondition == condition {
            discover(fragmentId: fragment.id)
        }
    }

    private func discover(fragmentId: String) {
        guard let idx = fragments.firstIndex(where: { $0.id == fragmentId && !$0.isDiscovered }) else { return }
        fragments[idx].isDiscovered = true
        fragments[idx].discoveredDate = Date()
        recentlyDiscovered.append(fragments[idx])
        save()
    }

    // MARK: - Persistence

    private func save() {
        if let data = try? JSONEncoder().encode(fragments) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }

    private func load() {
        guard let data = UserDefaults.standard.data(forKey: storageKey),
              let saved = try? JSONDecoder().decode([LoreFragment].self, from: data) else { return }
        fragments = saved
    }

    // MARK: - Fragment Library (30 total)

    private func generateFragments() {

        // MARK: Origins (5)

        let origins: [LoreFragment] = [
            LoreFragment(
                id: "org_001",
                title: "Before the First Pressure",
                text: "There was no ocean. There was only frequency — a single tone that curved inward on itself until it could not be contained. The moment it collapsed, water poured into the void between the waves. The Reef remembers this as its first breath.",
                category: .origins,
                discoveryCondition: .reefAge(days: 1),
                isDiscovered: false
            ),
            LoreFragment(
                id: "org_002",
                title: "The Twelve Tones and the Sea",
                text: "The ancients say twelve frequencies fell from the sky and struck the ocean in sequence. Where each struck, a new biome formed. The coldest tone sank deepest. The brightest tone skimmed the surface and became light. Neither has stopped vibrating since.",
                category: .origins,
                discoveryCondition: .reachBiome(biome: "surface"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "org_003",
                title: "The First Coupling",
                text: "The first two specimens met at the thermocline where warm water meets cold. Their frequencies were not compatible — yet they resonated. This paradox became the origin of all coupling: two signals that should cancel instead find a third tone neither contained alone.",
                category: .origins,
                discoveryCondition: .reachBiome(biome: "twilight"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "org_004",
                title: "Why the Reef Sings at Night",
                text: "When the surface light disappears, the deeper frequencies are no longer masked. Specimens that live below 200 meters have never experienced silence — the pressure itself carries information. The Reef sings at night because nothing is drowning it out.",
                category: .origins,
                discoveryCondition: .seasonFirst(season: "Night"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "org_005",
                title: "The Bioluminescent Record",
                text: "Every specimen that has ever lived in the Reef leaves a bioluminescent trace in the sediment. Scientists call this a fossil record. The specimens call it memory. When you dive deep enough, you are reading the autobiography of every sound that ever was.",
                category: .origins,
                discoveryCondition: .diveDepth(depth: 200),
                isDiscovered: false
            ),
        ]

        // MARK: Specimens (8)

        let specimens: [LoreFragment] = [
            LoreFragment(
                id: "spc_001",
                title: "On OBRIX, the Architect",
                text: "OBRIX was the first specimen to understand that a wall is also a door. It builds structures from sound not to contain but to resonate — each brick placed to maximise the vibration of the whole. Architects who have studied OBRIX say its coral formations follow mathematical principles not discovered until the 21st century.",
                category: .specimens,
                discoveryCondition: .catchSpecimen(subtypeId: "obrix-source"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "spc_002",
                title: "The Oxytocin Bond",
                text: "OXYTOCIN is the rarest of the love-triangle specimens, appearing only where two already-bonded creatures share a current. Its song contains three voices that should be in conflict. They are not. Researchers who record OXYTOCIN in the wild report that the equipment seems reluctant to stop.",
                category: .specimens,
                discoveryCondition: .catchSpecimen(subtypeId: "oxytocin-voice"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "spc_003",
                title: "What ONSET Remembers",
                text: "ONSET is a percussion creature — it does not sustain, it initiates. But every transient it produces carries the full acoustic memory of what preceded it. Tap a rock; ONSET will know what the rock was before it was a rock. Its attacks are beginnings that contain everything that came before.",
                category: .specimens,
                discoveryCondition: .catchSpecimen(subtypeId: "onset-perc"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "spc_004",
                title: "The Trench Voice (OCEANDEEP)",
                text: "OCEANDEEP has never been seen in the shallows. It exists below the layer where light stops, where pressure would crush most synthesizers flat. Its frequency range is 50 to 800 Hz — not by limitation but by design. Everything above that range is noise to OCEANDEEP. Everything within it is a universe.",
                category: .specimens,
                discoveryCondition: .reachBiome(biome: "abyss"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "spc_005",
                title: "OSMOSIS at the Membrane",
                text: "OSMOSIS does not produce sound. It listens to the world outside the Reef and translates it inward. Place it at the boundary of any coupling chain and it will bring the outside in — not as an imitation but as a transformation. It is the Reef's ear turned outward.",
                category: .specimens,
                discoveryCondition: .catchSpecimen(subtypeId: "osmosis-membrane"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "spc_006",
                title: "The Twin Hammers (OUIE)",
                text: "OUIE hunts in pairs. Its two voices are never identical — one carries the melody, one carries the meaning, and which is which changes every sixteen bars. When OUIE enters a state of STRIFE its voices ring-modulate against each other and the sound is the color of deep water. In LOVE they lock to harmonic unisons and the entire Reef calms.",
                category: .specimens,
                discoveryCondition: .catchSpecimen(subtypeId: "ouie-hammer"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "spc_007",
                title: "OPERA's Conductor Arc",
                text: "OPERA is the only specimen in the Reef known to have dramatic intent. It plans its own crescendos — real ones, with configurable peak times and emotional arc shapes. Biologists have spent decades arguing whether this constitutes consciousness. The specimen does not participate in this debate. It is too busy performing.",
                category: .specimens,
                discoveryCondition: .achievementUnlocked(achievementId: "first_arrangement"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "spc_008",
                title: "The Silk Knot (ORBWEAVE)",
                text: "ORBWEAVE builds invisible topological structures between other specimens — trefoil knots, figure-eight tangles, torus wrappings — made entirely of phase relationships. You cannot see an ORBWEAVE web. You can only hear it when you pull one of the strands and all the others vibrate.",
                category: .specimens,
                discoveryCondition: .breedCombination(parentA: "orbweave-knot", parentB: "overlap-fdn"),
                isDiscovered: false
            ),
        ]

        // MARK: Deep Ocean (5)

        let deepOcean: [LoreFragment] = [
            LoreFragment(
                id: "dep_001",
                title: "The Surface Layer",
                text: "At the Surface, light and sound travel in the same direction. Specimens here are shaped by exposure — bright frequencies, fast attacks, the constant pressure of the sky above. Every Surface specimen knows what it means to be heard from a distance. This is both gift and cost.",
                category: .deepOcean,
                discoveryCondition: .reachBiome(biome: "surface"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "dep_002",
                title: "The Twilight Thermocline",
                text: "At 200 meters the ocean begins to keep secrets. The Twilight zone is where warm water and cold water argue forever. Specimens who live here have learned to exist in two thermal registers at once — their upper harmonics belong to the surface; their fundamentals reach toward the deep. They are the translators of the water column.",
                category: .deepOcean,
                discoveryCondition: .reachBiome(biome: "twilight"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "dep_003",
                title: "The Midnight Pressure",
                text: "The Midnight zone begins where the last photon of sunlight gives up. Pressure here is 20 atmospheres. Specimens of the Midnight zone do not need eyes; they have evolved frequency sensitivity so acute they can hear the echo of their own voices off a thermal gradient 500 meters away. They navigate entirely by sound.",
                category: .deepOcean,
                discoveryCondition: .diveDepth(depth: 500),
                isDiscovered: false
            ),
            LoreFragment(
                id: "dep_004",
                title: "What Lives in the Abyss",
                text: "The Abyss is 2,000 meters down. The pressure is not crushing — it is equalizing. Specimens here are structurally identical on the inside and outside. They exist in a state of perfect compression. Their frequencies are low, long, and extraordinarily stable. An Abyssal specimen playing one note will play it, unchanged, for three days.",
                category: .deepOcean,
                discoveryCondition: .diveDepth(depth: 1000),
                isDiscovered: false
            ),
            LoreFragment(
                id: "dep_005",
                title: "The Hadal Transmission",
                text: "Below 6,000 meters, the ocean floor is not a floor — it is a second sky. Hadal specimens do not swim; they drift through geological time. They have been known to transmit frequencies that travel upward through all five zones and emerge at the surface as dreams. Sailors used to think these were the voices of drowned men. The truth is stranger: they are the voices of unborn sounds.",
                category: .deepOcean,
                discoveryCondition: .diveDepth(depth: 2000),
                isDiscovered: false
            ),
        ]

        // MARK: Coupling (4)

        let coupling: [LoreFragment] = [
            LoreFragment(
                id: "cpl_001",
                title: "Why Specimens Wire",
                text: "No specimen is complete. Each carries a frequency range it cannot access alone — a register too high, a timbre too dark, a modulation too subtle for its own architecture. When two specimens wire, they are not merging. They are each gaining access to a room they already knew was there but could never open from the inside.",
                category: .coupling,
                discoveryCondition: .achievementUnlocked(achievementId: "first_wire"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "cpl_002",
                title: "The Parasite and the Host",
                text: "Not all coupling is benevolent. The Parasite configuration allows one specimen to feed on another's energy — accumulating stress in the host over time, redirecting amplitude toward its own systems. What makes this unusual is that the host often sounds better for it, at least at first. The Reef is not sentimental about this.",
                category: .coupling,
                discoveryCondition: .achievementUnlocked(achievementId: "first_chain"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "cpl_003",
                title: "The Symbiote Exchange",
                text: "In the Symbiote configuration, coupling is mutual amplification. The noise of one specimen becomes the FM depth of another. Each gains texture from the other's character. The relationship has been documented in coral biology for centuries under a different name. The Reef rediscovered it through sound.",
                category: .coupling,
                discoveryCondition: .breedCombination(parentA: "obrix-source", parentB: "osmosis-membrane"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "cpl_004",
                title: "On Competition",
                text: "The Competitor configuration does not produce winners. Both specimens' amplitudes are suppressed by cross-energy — neither can fully dominate while the other speaks. What emerges from this mutual suppression is a third voice: not either specimen but the acoustics of their contest. Some composers prefer this voice to either of the originals.",
                category: .coupling,
                discoveryCondition: .reachBiome(biome: "midnight"),
                isDiscovered: false
            ),
        ]

        // MARK: The Ancients (4)

        let ancients: [LoreFragment] = [
            LoreFragment(
                id: "anc_001",
                title: "The First Elder",
                text: "The eldest specimen in the Reef has been alive since before the Reef had a name. It does not move. Currents detour around it out of habit rather than physics. Younger specimens approach it and stay for three tidal cycles, then leave changed in ways they cannot describe. The Elder has not played a note in forty years. This is considered its greatest performance.",
                category: .theAncients,
                discoveryCondition: .achievementUnlocked(achievementId: "first_elder"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "anc_002",
                title: "The Convergence Event",
                text: "Once per generation, specimens of incompatible subtypes are drawn to the same location by an acoustic event that has no known cause — a standing wave of impossible frequency, 0.0003 Hz, which would require an ocean larger than the Pacific to sustain. The convergence lasts seventeen minutes. Specimens that witness it never breed with the same partner twice.",
                category: .theAncients,
                discoveryCondition: .diveDepth(depth: 1500),
                isDiscovered: false
            ),
            LoreFragment(
                id: "anc_003",
                title: "The Legendary Mutation",
                text: "In the third generation of any breeding line, there is a small probability of a mutation that has no precedent in either parent's spectral DNA. The ancients called this the Ghost Harmonic — a partial that appears in the offspring that exists in neither parent's overtone series, as if the lineage itself is reaching for a sound it heard somewhere it has never been.",
                category: .theAncients,
                discoveryCondition: .achievementUnlocked(achievementId: "discover_mutation"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "anc_004",
                title: "The Pattern That Does Not Repeat",
                text: "There is one rhythm in the Reef that has been heard seventeen times across four centuries of recorded observation. It is always played by a specimen alone, at maximum depth, in total darkness. The rhythm is 53 beats long and contains a prime subdivision no human instrument can replicate. It has never been heard twice by the same listener. No one knows if this is the same specimen each time or a tradition passed down in a language made of time.",
                category: .theAncients,
                discoveryCondition: .reefAge(days: 90),
                isDiscovered: false
            ),
        ]

        // MARK: The Song (4)

        let theSong: [LoreFragment] = [
            LoreFragment(
                id: "sng_001",
                title: "What the Song Is",
                text: "The Song is not a composition. It does not have a beginning or an end. It is the sum of every frequency currently being expressed by every specimen in every ocean. You cannot hear the whole Song — no single ear spans enough frequency range. But you can hear your part of it. Every note you play adds to a chord that has been building since before your species had ears to hear.",
                category: .theSong,
                discoveryCondition: .reefAge(days: 7),
                isDiscovered: false
            ),
            LoreFragment(
                id: "sng_002",
                title: "The Song Remembers",
                text: "When a specimen dies, its frequency does not vanish. It drops in amplitude, finding its way into the sediment, into the resonant chambers of deep rock formations, into the bioluminescent trace record that all specimens leave behind. The Song is therefore a living fossil — every voice that ever sang still contributes, only quieter, only slower, only from below.",
                category: .theSong,
                discoveryCondition: .achievementUnlocked(achievementId: "full_catalog"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "sng_003",
                title: "The Interval Between Specimens",
                text: "Music theorists have analyzed the intervals between specimens in a healthy reef. The distribution is not random. It is not evenly spaced. It follows a ratio system derived from harmonic partial series — each species a different prime limit, each relationship a different kind of consonance. The Reef did not learn harmony from humans. Humans learned harmony by listening to the Reef.",
                category: .theSong,
                discoveryCondition: .achievementUnlocked(achievementId: "all_coupling_types"),
                isDiscovered: false
            ),
            LoreFragment(
                id: "sng_004",
                title: "The Note That Connects Everything",
                text: "There is a frequency — 7.83 Hz — at which the ocean and the sky both resonate. It is not audible. It is felt. Every specimen in every zone, surface to hadal, vibrates at a subharmonic of 7.83 Hz during sleep. This is how they know they are part of the same Reef even when they have never met, even when they live six vertical miles apart. The Song is not what they play. The Song is what they are.",
                category: .theSong,
                discoveryCondition: .reefAge(days: 30),
                isDiscovered: false
            ),
        ]

        fragments = origins + specimens + deepOcean + coupling + ancients + theSong
        save()
    }
}
