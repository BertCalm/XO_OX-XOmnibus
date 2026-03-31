import Foundation
import CoreLocation

/// A wild specimen visible on the radar — available for catching.
struct WildSpecimen: Identifiable {
    let id: UUID
    let category: SpecimenCategory
    let subtype: String
    let rarity: SpecimenRarity
    let biome: Biome
    let spawnSource: SpawnSource
    /// Compass bearing from the user (radians, 0 = north)
    let direction: Double
    /// Approximate distance from the user in meters
    let distance: Double
    /// Wall-clock expiry — specimens despawn after 4 hours
    let expiresAt: Date

    /// Catch-time spectral bias from the spawn biome
    var biomeBias: [Float] { biome.spectralBias }

    /// Standard init — generates a fresh UUID automatically.
    init(
        category: SpecimenCategory,
        subtype: String,
        rarity: SpecimenRarity,
        biome: Biome,
        spawnSource: SpawnSource,
        direction: Double,
        distance: Double,
        expiresAt: Date
    ) {
        self.id = UUID()
        self.category = category
        self.subtype = subtype
        self.rarity = rarity
        self.biome = biome
        self.spawnSource = spawnSource
        self.direction = direction
        self.distance = distance
        self.expiresAt = expiresAt
    }

    /// Refresh init — preserves the existing id so map annotations stay stable.
    init(preservingID id: UUID, from source: WildSpecimen, expiresAt: Date) {
        self.id = id
        self.category = source.category
        self.subtype = source.subtype
        self.rarity = source.rarity
        self.biome = source.biome
        self.spawnSource = source.spawnSource
        self.direction = source.direction
        self.distance = source.distance
        self.expiresAt = expiresAt
    }
}

/// Which spawn subsystem created this specimen (spec Section 4 + 7.2)
enum SpawnSource: String, Codable {
    case dailyDrift         // 1-2 Sources per day, duplicated every ~500m
    case loginMilestone     // Modulator on Day 3/7/14/30/60/100 streak
    case performanceReward  // Effect for surviving a Dive 3+ minutes (Phase 2)
    case weatherBonus       // Processor from real-time weather match (Phase 1)
    case couplingDiscovery  // Hybrid cross-category (Phase 4)
    case explorationBonus   // Any category Rare+ when entering a new 500m cell
    case timeOfDay          // Processor at golden hour (dawn/dusk)
    case communityEvent     // Special limited-edition variants (Phase 4)
}

/// Manages all specimen spawning logic (spec Sections 4 and 7.2).
///
/// Spawn source → category exclusivity (prevents cannibalization):
///   Daily Drift        → Sources only
///   Login Milestones   → Modulators only
///   Performance Rewards→ Effects only (Phase 2)
///   Weather Bonus      → Processors — boosted by real-time weather condition (Phase 2)
///   Time-of-Day        → Category rotates by time window (Phase 2)
///   Exploration Bonus  → Any category, Rare+ guaranteed
///   Coupling Discovery → Offspring category from BreedingSystem (Phase 2)
///   Community Events   → Seasonal event pool with rarity multiplier (Phase 2)
final class SpawnManager: ObservableObject {
    @Published var wildSpecimens: [WildSpecimen] = []

    /// Weak reference to ReefStore — used for persisting visited geohashes across launches.
    weak var reefStore: ReefStore?

    /// Injected by CatchTab so weather condition drives spawn probability boosts (Phase 2).
    weak var weatherService: WeatherService?

    private let biomeDetector: BiomeDetector
    private let defaults = UserDefaults.standard
    private let loginStreakKey = "obrix_login_streak"
    private let lastLoginDateKey = "obrix_last_login_date"
    private let lastDriftDateKey = "obrix_spawn_last_drift_date"

    var loginStreak: Int {
        get { defaults.integer(forKey: loginStreakKey) }
        set { defaults.set(newValue, forKey: loginStreakKey) }
    }

    var lastLoginDate: Date? {
        get { defaults.object(forKey: lastLoginDateKey) as? Date }
        set { defaults.set(newValue, forKey: lastLoginDateKey) }
    }

    /// Persisted to UserDefaults so the daily drift window survives app restarts.
    var lastDriftDate: Date? {
        get { defaults.object(forKey: lastDriftDateKey) as? Date }
        set { defaults.set(newValue, forKey: lastDriftDateKey) }
    }

    // MARK: - Specimen subtype rosters (spec Section 7.1)
    // Only CORE specimens are available through normal spawns.
    // Deep specimens are excluded — they unlock through special conditions.

    static let sourceSubtypes: [String] = [
        "polyblep-saw", "polyblep-square", "noise-white", "fm-basic"
    ]
    static let processorSubtypes: [String] = [
        "svf-lp", "svf-bp", "shaper-hard", "feedback"
    ]
    static let modulatorSubtypes: [String] = [
        "adsr-fast", "lfo-sine", "vel-map", "lfo-random"
    ]
    static let effectSubtypes: [String] = [
        "delay-stereo", "chorus-lush", "reverb-hall", "distortion-warm"
    ]

    // MARK: - Journey Override
    // Set by CatchTab when the tutorial journey is active. When non-nil, daily drift spawns
    // exactly this specimen instead of a random Source so the journey sequence is guaranteed.

    /// The catalog subtype ID to force next (e.g., "lfo-sine"). Nil = random mode.
    var forcedNextSubtype: String? = nil
    /// The category of the forced specimen (must match catalog entry).
    var forcedNextCategory: SpecimenCategory? = nil

    init(biomeDetector: BiomeDetector) {
        self.biomeDetector = biomeDetector
    }

    // MARK: - Daily Drift (Sources only — spec Section 7.2)

    /// Spawns specimens for today's drift window.
    ///
    /// Journey mode (forcedNextSubtype != nil): spawns exactly the next scripted specimen
    /// as a Common, close by, with a generous 24-hour window so new players can't miss it.
    ///
    /// Normal mode: spawns 1-2 random Source specimens as usual.
    func checkDailyDrift() {
        let today = Calendar.current.startOfDay(for: Date())
        guard lastDriftDate == nil || !Calendar.current.isDate(lastDriftDate!, inSameDayAs: today) else {
            return
        }
        lastDriftDate = today

        if let forcedSubtype = forcedNextSubtype, let forcedCategory = forcedNextCategory {
            // Journey mode: spawn the scripted next specimen — always Common, nearby, long window.
            wildSpecimens.append(WildSpecimen(
                category: forcedCategory,
                subtype: forcedSubtype,
                rarity: .common,
                biome: biomeDetector.currentBiome,
                spawnSource: .dailyDrift,
                direction: Double.random(in: 0...(2 * .pi)),
                distance: Double.random(in: 30...100),
                expiresAt: Date().addingTimeInterval(24 * 3600)
            ))
        } else {
            // Normal random spawns
            let count = Int.random(in: 1...2)
            for _ in 0..<count {
                let rarity: SpecimenRarity = Bool.random() ? .common : .uncommon
                wildSpecimens.append(generateWildSpecimen(
                    category: .source,
                    rarity: rarity,
                    source: .dailyDrift
                ))
            }
        }
    }

    // MARK: - Login Milestones (Modulators only — spec Section 7.2)

    /// Awards a Modulator when the player hits a login streak milestone (3/7/14/30/60/100 days).
    func checkLoginMilestone() {
        let today = Calendar.current.startOfDay(for: Date())

        // Guard: already logged in today
        if let lastLogin = lastLoginDate, Calendar.current.isDate(lastLogin, inSameDayAs: today) {
            return
        }

        // Update streak
        if let lastLogin = lastLoginDate {
            let daysBetween = Calendar.current.dateComponents([.day], from: lastLogin, to: today).day ?? 0
            loginStreak = daysBetween == 1 ? loginStreak + 1 : 1
        } else {
            loginStreak = 1
        }
        lastLoginDate = today

        // Spawn on milestone days only
        let milestones: Set<Int> = [3, 7, 14, 30, 60, 100]
        guard milestones.contains(loginStreak) else { return }

        let rarity: SpecimenRarity = loginStreak >= 30 ? .rare : .uncommon
        wildSpecimens.append(generateWildSpecimen(
            category: .modulator,
            rarity: rarity,
            source: .loginMilestone
        ))
    }

    // MARK: - Time of Day Bonus (Phase 2 — category rotates by window)

    /// Spawns a specimen whose category matches the current time-of-day window.
    ///
    /// Windows (Phase 2 spec):
    ///   Morning   (06–12): Source specimens boosted   — oscillators stir at dawn
    ///   Afternoon (12–18): Processor specimens boosted — filters peak in bright light
    ///   Evening   (18–24): Modulator specimens boosted — LFOs/envelopes surge at dusk
    ///   Night     (00–06): Effect specimens boosted    — delays/reverbs rule the dark
    ///
    /// 35% spawn probability per call to avoid flooding the map.
    func checkTimeOfDay() {
        guard Float.random(in: 0...1) < 0.35 else { return }

        let hour = Calendar.current.component(.hour, from: Date())
        let (category, rarity): (SpecimenCategory, SpecimenRarity)

        switch hour {
        case 6..<12:
            // Morning — Source specimens; rarity climbs toward noon
            category = .source
            rarity = hour >= 9 ? .uncommon : .common
        case 12..<18:
            // Afternoon — Processor specimens; peak brightness = Rare window 14–16
            category = .processor
            rarity = (hour >= 14 && hour < 16) ? .rare : .uncommon
        case 18..<24:
            // Evening — Modulator specimens; golden hour 18–20 bumps to Rare
            category = .modulator
            rarity = (hour >= 18 && hour < 20) ? .rare : .uncommon
        default:
            // Night (00–06) — Effect specimens; deepest hour (02–04) = Rare chance
            category = .effect
            rarity = (hour >= 2 && hour < 4) ? .rare : .uncommon
        }

        wildSpecimens.append(generateWildSpecimen(
            category: category,
            rarity: rarity,
            source: .timeOfDay
        ))
    }

    // MARK: - Performance Reward (Effects only — Phase 2)

    /// Spawns an Effect specimen after Dive completion. Score thresholds determine rarity:
    ///
    ///   < 1000         → Common  (80% chance to spawn at all)
    ///   1000 – 2999    → Uncommon (30% chance)
    ///   3000 – 5999    → Rare     (15% chance)
    ///   6000+          → Legendary (5% chance)
    ///
    /// Call this from DiveTab.endDive() with the final computed score.
    func checkPerformanceReward(diveScore: Int) {
        let spawnChance: Float
        let rarity: SpecimenRarity

        switch diveScore {
        case ..<1000:
            spawnChance = 0.80
            rarity = .common
        case 1000..<3000:
            spawnChance = 0.30
            rarity = .uncommon
        case 3000..<6000:
            spawnChance = 0.15
            rarity = .rare
        default: // 6000+
            spawnChance = 0.05
            rarity = .legendary
        }

        guard Float.random(in: 0...1) < spawnChance else { return }

        wildSpecimens.append(generateWildSpecimen(
            category: .effect,
            rarity: rarity,
            source: .performanceReward
        ))
    }

    // MARK: - Weather Bonus (Processors — Phase 2)

    /// Spawns a Processor whose rarity is boosted by the current weather condition.
    ///
    /// Weather → cosmetic/biome mapping (Phase 2 spec):
    ///   Clear / fair   → Sunlit biome  — Common/Uncommon at 40% probability
    ///   Rain / storm   → Thermocline   — Uncommon/Rare   at 50% probability
    ///   Fog / low vis  → Abyss         — Rare            at 20% probability
    ///   Night + any    → Bio cosmetic  — Uncommon        at 35% probability
    ///
    /// Falls back gracefully when WeatherService is unavailable (no spawn).
    func checkWeatherBonus() {
        guard let weather = weatherService?.bestAvailable else { return }

        let hour = Calendar.current.component(.hour, from: Date())
        let isNight = hour < 6 || hour >= 21
        let desc = weather.description.lowercased()

        let spawnChance: Float
        let rarity: SpecimenRarity

        if desc.contains("fog") || desc.contains("mist") || desc.contains("haze") {
            // Fog / low visibility — Abyss class, rare
            spawnChance = 0.20
            rarity = .rare
        } else if desc.contains("rain") || desc.contains("thunder") || desc.contains("storm")
                  || desc.contains("drizzle") || desc.contains("snow") || desc.contains("sleet") {
            // Rain / storm — Thermocline class, uncommon–rare
            spawnChance = 0.50
            rarity = Bool.random() ? .uncommon : .rare
        } else if isNight {
            // Night — Bio cosmetic class, uncommon
            spawnChance = 0.35
            rarity = .uncommon
        } else {
            // Clear / fair — Sunlit class, common–uncommon
            spawnChance = 0.40
            rarity = Bool.random() ? .common : .uncommon
        }

        guard Float.random(in: 0...1) < spawnChance else { return }

        wildSpecimens.append(generateWildSpecimen(
            category: .processor,
            rarity: rarity,
            source: .weatherBonus
        ))
    }

    // MARK: - Coupling Discovery (Hybrid category from BreedingSystem — Phase 2)

    /// Registers a bred offspring as a wild specimen available for catch.
    ///
    /// Called by the Nursery/GameCoordinator when an offspring graduates
    /// (formationEndDate is reached and the player has not yet placed it).
    /// The offspring appears on the radar near the player so they must "catch" it
    /// before it joins the reef — this keeps the catch loop central.
    ///
    /// - Parameters:
    ///   - offspringSubtype: The blended subtype ID from `InheritedTraits.offspringSubtypeID`.
    ///   - offspringCategory: The category string from `InheritedTraits.offspringCategory`.
    ///   - generation: The offspring's generation (Gen-2+). Higher gen = higher rarity floor.
    func registerCouplingDiscovery(
        offspringSubtype: String,
        offspringCategory: String,
        generation: Int
    ) {
        // Derive SpecimenCategory from the raw string; fall back to .source on unknown values
        let validCategories = Set(SpecimenCategory.allCases.map { $0.rawValue })
        let category: SpecimenCategory
        if validCategories.contains(offspringCategory),
           let parsed = SpecimenCategory(rawValue: offspringCategory) {
            category = parsed
        } else {
            category = .source
        }

        // Generation determines rarity floor: Gen-2 = uncommon, Gen-3 = rare, Gen-4+ = legendary
        let rarity: SpecimenRarity
        switch generation {
        case ..<2:   rarity = .common
        case 2:      rarity = .uncommon
        case 3:      rarity = .rare
        default:     rarity = .legendary
        }

        // Offspring spawn close to the player — they know where you live
        wildSpecimens.append(WildSpecimen(
            category: category,
            subtype: offspringSubtype,
            rarity: rarity,
            biome: biomeDetector.currentBiome,
            spawnSource: .couplingDiscovery,
            direction: Double.random(in: 0...(2 * .pi)),
            distance: Double.random(in: 30...150),  // Always nearby
            expiresAt: Date().addingTimeInterval(24 * 3600) // 24-hour window (generous)
        ))
    }

    // MARK: - Community Events (Seasonal event pool — Phase 2)

    /// Applies the active seasonal event's spawn pool and rarity multiplier to produce
    /// additional wild specimens. Call periodically (e.g., on app foreground or hourly tick).
    ///
    /// Event behaviour:
    ///   .bioluminescentStorm → Rare/Legendary specimens from all categories; 2× multiplier
    ///   .migrationWave       → Burst of up to 6 simultaneous specimens; 1.5× multiplier
    ///   .deepCurrent         → Deep-biome subtypes surface as Sources and Processors
    ///   .coralBloom          → No spawn change (breeding/nursery focus — handled elsewhere)
    ///
    /// No-op when no event is active.
    func checkCommunityEvent() {
        guard let event = SeasonalEventManager.activeEvent() else { return }

        switch event.type {

        case .bioluminescentStorm:
            // Doubled rare spawns — all categories, Rare or Legendary
            guard Float.random(in: 0...1) < 0.60 else { return }
            let category = SpecimenCategory.allCases.randomElement() ?? .source
            let rarity: SpecimenRarity = Float.random(in: 0...1) < 0.40 ? .legendary : .rare
            wildSpecimens.append(generateWildSpecimen(
                category: category,
                rarity: rarity,
                source: .communityEvent
            ))

        case .migrationWave:
            // Wave of 3–6 simultaneous specimens — any rarity, any category
            let count = Int.random(in: 3...6)
            for _ in 0..<count {
                let category = SpecimenCategory.allCases.randomElement() ?? .source
                let rarity: SpecimenRarity = Float.random(in: 0...1) < 0.30 ? .rare :
                             (Float.random(in: 0...1) < 0.50 ? .uncommon : .common)
                wildSpecimens.append(generateWildSpecimen(
                    category: category,
                    rarity: rarity,
                    source: .communityEvent
                ))
            }

        case .deepCurrent:
            // Deep-biome subtypes surface — Sources and Processors only, Rare floor
            guard Float.random(in: 0...1) < 0.50 else { return }
            let surfacedSubtypes = LimitedTimeEventType.deepCurrent.surfacedDeepSubtypes
            guard let chosenSubtype = surfacedSubtypes.randomElement() else { return }
            // Map deep subtypes to a category: filter-ish → processor, others → source
            let category: SpecimenCategory = chosenSubtype.contains("svf") || chosenSubtype.contains("feedback")
                ? .processor : .source
            wildSpecimens.append(WildSpecimen(
                category: category,
                subtype: chosenSubtype,
                rarity: .rare,
                biome: biomeDetector.currentBiome,
                spawnSource: .communityEvent,
                direction: Double.random(in: 0...(2 * .pi)),
                distance: Double.random(in: 50...300),
                expiresAt: Date().addingTimeInterval(4 * 3600)
            ))

        case .coralBloom:
            // Coral Bloom affects breeding/nursery speed only — no spawn change
            break
        }
    }

    // MARK: - Exploration Bonus (Any category, Rare+ — spec Section 7.2)

    private var visitedGeohashes: Set<String> = []

    /// Populate in-memory visitedGeohashes from the DB on launch.
    /// Call this once after setting reefStore, before the first checkExplorationBonus call.
    func loadPersistedGeohashes() {
        if let hashes = reefStore?.loadVisitedGeohashes() {
            visitedGeohashes = hashes
        }
    }

    /// Awards a Rare+ specimen of any category when entering a previously unvisited 500m cell.
    func checkExplorationBonus(at location: CLLocationCoordinate2D) {
        let hash = simpleGeohash(location, precision: 3) // ~600m cell (2^15 = 32768 buckets, 180/32768 ≈ 610m)
        guard !visitedGeohashes.contains(hash) else { return }
        visitedGeohashes.insert(hash)
        reefStore?.saveVisitedGeohash(hash)

        let category = SpecimenCategory.allCases.randomElement() ?? .source
        // Exploration guarantees Rare or better
        let rarity: SpecimenRarity = Bool.random() ? .rare : .legendary
        wildSpecimens.append(generateWildSpecimen(
            category: category,
            rarity: rarity,
            source: .explorationBonus
        ))
    }

    // MARK: - Prune Expired Specimens

    /// Removes specimens whose 4-hour window has closed.
    func pruneExpired() {
        wildSpecimens.removeAll { $0.expiresAt < Date() }
    }

    // MARK: - Expiry Reset (escape/retry mechanic)

    /// Replaces the wild specimen with the given id with an updated copy (e.g. refreshed expiry).
    /// Called by CatchTab after a failed catch so the specimen stays on the map.
    func updateSpecimen(_ updated: WildSpecimen, replacing oldID: UUID) {
        guard let idx = wildSpecimens.firstIndex(where: { $0.id == oldID }) else { return }
        wildSpecimens[idx] = updated
    }

    // MARK: - Generation

    private func generateWildSpecimen(
        category: SpecimenCategory,
        rarity: SpecimenRarity,
        source: SpawnSource
    ) -> WildSpecimen {
        let subtypes: [String]
        switch category {
        case .source:    subtypes = Self.sourceSubtypes
        case .processor: subtypes = Self.processorSubtypes
        case .modulator: subtypes = Self.modulatorSubtypes
        case .effect:    subtypes = Self.effectSubtypes
        }

        // Weight selection by biome affinity and seasonal event boosts
        let currentBiome = biomeDetector.currentBiome
        let weighted = subtypes.map { subtype -> (String, Float) in
            let catalogID = SpecimenCatalog.catalogSubtypeID(from: subtype)
            var weight: Float = 1.0

            // Biome affinity — preferred biome specimens are 3x more likely
            if let entry = SpecimenCatalog.entry(for: catalogID),
               entry.preferredBiomes.contains(currentBiome) {
                weight *= 3.0
            }

            // Seasonal event boost — applies SeasonalEventManager spawn rate multiplier
            // (handles season-exclusive blocking, season modifiers, and active event overrides)
            let eventMultiplier = SeasonalEventManager.shared.spawnRateMultiplier(for: catalogID)
            weight *= eventMultiplier

            return (subtype, weight)
        }

        let selectedSubtype = weightedRandom(from: weighted) ?? subtypes[0]

        return WildSpecimen(
            category: category,
            subtype: selectedSubtype,
            rarity: rarity,
            biome: currentBiome,
            spawnSource: source,
            direction: Double.random(in: 0...(2 * .pi)),
            distance: Double.random(in: 20...400),
            expiresAt: Date().addingTimeInterval(4 * 3600) // 4-hour window
        )
    }

    private func weightedRandom(from items: [(String, Float)]) -> String? {
        let totalWeight = items.map { $0.1 }.reduce(0, +)
        guard totalWeight > 0 else { return nil }
        var roll = Float.random(in: 0..<totalWeight)
        for (item, weight) in items {
            roll -= weight
            if roll < 0 { return item }
        }
        return items.last?.0
    }

    // MARK: - Simple Geohash (exploration tracking only)

    /// Encodes a coordinate into a coarse grid cell for 500m-resolution exploration tracking.
    private func simpleGeohash(_ coord: CLLocationCoordinate2D, precision: Int) -> String {
        let scale = pow(2.0, Double(precision * 5))
        let latBin = Int((coord.latitude + 90.0)  / 180.0 * scale)
        let lonBin = Int((coord.longitude + 180.0) / 360.0 * scale)
        return "\(latBin)_\(lonBin)"
    }
}
