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
///   Weather/Time Bonus → Processors
///   Exploration Bonus  → Any category, Rare+ guaranteed
///   Coupling Discovery → Hybrid cross-category (Phase 4)
///   Community Events   → Special limited-edition (Phase 4)
final class SpawnManager: ObservableObject {
    @Published var wildSpecimens: [WildSpecimen] = []

    /// Weak reference to ReefStore — used for persisting visited geohashes across launches.
    weak var reefStore: ReefStore?

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
        "PolyBLEP-Saw", "PolyBLEP-Square", "Noise-White", "FM-Basic"
    ]
    static let processorSubtypes: [String] = [
        "SVF-LP", "SVF-BP", "Waveshaper-Hard", "Feedback-Path"
    ]
    static let modulatorSubtypes: [String] = [
        "ADSR-Fast", "LFO-Sine", "Velocity-Map", "LFO-Random"
    ]
    static let effectSubtypes: [String] = [
        "Delay-Stereo", "Chorus-Lush", "Reverb-Hall", "Distortion-Warm"
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

    // MARK: - Time of Day Bonus (Processors — spec Section 7.2)

    /// Spawns a Processor during golden hour (dawn 5-7 AM, dusk 5-7 PM) with 30% probability.
    func checkTimeOfDay() {
        let hour = Calendar.current.component(.hour, from: Date())
        let isGoldenHour = (hour >= 5 && hour < 7) || (hour >= 17 && hour < 19)
        guard isGoldenHour, Float.random(in: 0...1) < 0.3 else { return }

        wildSpecimens.append(generateWildSpecimen(
            category: .processor,
            rarity: .uncommon,
            source: .timeOfDay
        ))
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
        let activeEvent = SeasonalEventManager.activeEvent()
        let weighted = subtypes.map { subtype -> (String, Float) in
            let catalogID = SpecimenCatalog.catalogSubtypeID(from: subtype)
            var weight: Float = 1.0

            // Biome affinity — preferred biome specimens are 3x more likely
            if let entry = SpecimenCatalog.entry(for: catalogID),
               entry.preferredBiomes.contains(currentBiome) {
                weight *= 3.0
            }

            // Seasonal event boost — reserved for future LimitedTimeEvent.boostedSubtypes support
            _ = activeEvent  // suppress unused-variable warning

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
