import Foundation
import CoreLocation

/// A wild specimen visible on the radar — available for catching.
struct WildSpecimen: Identifiable {
    let id = UUID()
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
    @Published var lastDriftDate: Date?
    @Published var loginStreak: Int = 0
    @Published var lastLoginDate: Date?

    private let biomeDetector: BiomeDetector

    // MARK: - Specimen subtype rosters (spec Section 7.1)

    static let sourceSubtypes: [String] = [
        "PolyBLEP-Saw", "PolyBLEP-Square", "PolyBLEP-Tri", "Noise-White",
        "Noise-Pink", "Wavetable-Analog", "Wavetable-Vocal", "FM-Basic"
    ]
    static let processorSubtypes: [String] = [
        "SVF-LP", "SVF-HP", "SVF-BP", "Waveshaper-Soft",
        "Waveshaper-Hard", "Feedback-Path"
    ]
    static let modulatorSubtypes: [String] = [
        "ADSR-Fast", "ADSR-Slow", "LFO-Sine", "LFO-Random",
        "Velocity-Map", "Aftertouch-Map"
    ]
    static let effectSubtypes: [String] = [
        "Delay-Stereo", "Chorus-Lush", "Reverb-Hall", "Distortion-Warm"
    ]

    init(biomeDetector: BiomeDetector) {
        self.biomeDetector = biomeDetector
    }

    // MARK: - Daily Drift (Sources only — spec Section 7.2)

    /// Spawns 1-2 Source specimens if none have been drifted today.
    func checkDailyDrift() {
        let today = Calendar.current.startOfDay(for: Date())
        guard lastDriftDate == nil || !Calendar.current.isDate(lastDriftDate!, inSameDayAs: today) else {
            return
        }
        lastDriftDate = today
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

    /// Awards a Rare+ specimen of any category when entering a previously unvisited 500m cell.
    func checkExplorationBonus(at location: CLLocationCoordinate2D) {
        let hash = simpleGeohash(location, precision: 3) // ~600m cell (2^15 = 32768 buckets, 180/32768 ≈ 610m)
        guard !visitedGeohashes.contains(hash) else { return }
        visitedGeohashes.insert(hash)

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

        return WildSpecimen(
            category: category,
            subtype: subtypes.randomElement() ?? subtypes[0],
            rarity: rarity,
            biome: biomeDetector.currentBiome,
            spawnSource: source,
            direction: Double.random(in: 0...(2 * .pi)),
            distance: Double.random(in: 20...400),
            expiresAt: Date().addingTimeInterval(4 * 3600) // 4-hour window
        )
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
