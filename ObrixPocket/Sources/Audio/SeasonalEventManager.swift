import Foundation

// MARK: - Season

/// The four real-calendar seasons that govern the reef's rhythm.
/// Seasons transition on northern-hemisphere meteorological boundaries.
enum Season: String, CaseIterable, Codable {
    case springBloom    // March 1 – May 31
    case summerSurge    // June 1 – August 31
    case autumnDrift    // September 1 – November 30
    case winterDeep     // December 1 – February 28/29

    var displayName: String {
        switch self {
        case .springBloom:  return "Spring Bloom"
        case .summerSurge:  return "Summer Surge"
        case .autumnDrift:  return "Autumn Drift"
        case .winterDeep:   return "Winter Deep"
        }
    }

    var sonicDescription: String {
        switch self {
        case .springBloom:  return "New voices stir from dormancy. Bright tones pierce cold water."
        case .summerSurge:  return "The reef sings at full voice. Energy crests like a breaking wave."
        case .autumnDrift:  return "Warmth recedes. Tones grow amber, complex, searching."
        case .winterDeep:   return "Silence thickens. What remains glows all the brighter."
        }
    }

    /// Color palette tokens the UI reads to tint the reef. RGBA components 0–1.
    var palette: SeasonalPalette {
        switch self {
        case .springBloom:
            return SeasonalPalette(
                primary:    RGBA(r: 0.55, g: 0.90, b: 0.60, a: 1),  // fresh green
                secondary:  RGBA(r: 0.90, g: 0.70, b: 0.85, a: 1),  // blossom pink
                ambient:    RGBA(r: 0.75, g: 0.93, b: 0.88, a: 0.5), // teal mist
                waterTint:  RGBA(r: 0.40, g: 0.72, b: 0.85, a: 0.6)  // clear blue
            )
        case .summerSurge:
            return SeasonalPalette(
                primary:    RGBA(r: 1.00, g: 0.80, b: 0.20, a: 1),  // sunburst gold
                secondary:  RGBA(r: 1.00, g: 0.45, b: 0.25, a: 1),  // coral orange
                ambient:    RGBA(r: 0.90, g: 0.78, b: 0.40, a: 0.5), // warm glow
                waterTint:  RGBA(r: 0.12, g: 0.60, b: 0.75, a: 0.6)  // vivid cyan
            )
        case .autumnDrift:
            return SeasonalPalette(
                primary:    RGBA(r: 0.85, g: 0.50, b: 0.15, a: 1),  // amber
                secondary:  RGBA(r: 0.70, g: 0.25, b: 0.12, a: 1),  // deep rust
                ambient:    RGBA(r: 0.65, g: 0.45, b: 0.30, a: 0.5), // warm smoke
                waterTint:  RGBA(r: 0.22, g: 0.40, b: 0.55, a: 0.6)  // grey-blue
            )
        case .winterDeep:
            return SeasonalPalette(
                primary:    RGBA(r: 0.55, g: 0.75, b: 0.95, a: 1),  // ice blue
                secondary:  RGBA(r: 0.80, g: 0.85, b: 1.00, a: 1),  // frost white
                ambient:    RGBA(r: 0.15, g: 0.20, b: 0.40, a: 0.5), // deep indigo
                waterTint:  RGBA(r: 0.08, g: 0.12, b: 0.35, a: 0.7)  // abyssal night
            )
        }
    }

    /// Specimen subtypes that spawn exclusively in this season.
    var exclusiveSubtypes: [String] {
        switch self {
        case .springBloom:  return ["polyblep-tri", "adsr-fast", "chorus-lush"]   // Glider, Snapper, Shimmer
        case .summerSurge:  return ["polyblep-saw", "fm-basic", "shaper-hard"]    // Sawfin, Bellcrab, Bonecrush
        case .autumnDrift:  return ["feedback", "lfo-random", "dist-warm"]        // Loopworm, Scramble, Ember
        case .winterDeep:   return ["noise-pink", "svf-hp", "reverb-hall"]        // Siltsift, Razorgill, Cathedral
        }
    }

    /// Spawn-rate modifier for specimen subtypes (multiplier, >1 = boosted, <1 = suppressed).
    var spawnRateModifiers: [String: Float] {
        switch self {
        case .springBloom:
            return ["adsr-fast": 1.8, "polyblep-tri": 1.6, "lfo-sine": 1.4,
                    "noise-white": 0.6, "reverb-hall": 0.7]
        case .summerSurge:
            return ["polyblep-saw": 1.8, "fm-basic": 1.6, "shaper-hard": 1.5,
                    "noise-pink": 0.5, "svf-hp": 0.6]
        case .autumnDrift:
            return ["feedback": 1.7, "lfo-random": 1.6, "dist-warm": 1.5,
                    "adsr-fast": 0.6, "chorus-lush": 0.7]
        case .winterDeep:
            return ["reverb-hall": 1.8, "noise-pink": 1.7, "svf-hp": 1.5,
                    "fm-basic": 0.5, "polyblep-saw": 0.6]
        }
    }

    /// Which coupling types receive the +20% affinity bonus this season.
    var couplingAffinityBonuses: [String] {
        switch self {
        case .springBloom:  return ["HARMONIC", "UNISON", "OCTAVE"]
        case .summerSurge:  return ["RHYTHM", "PULSE", "VELOCITY"]
        case .autumnDrift:  return ["RESONANCE", "TENSION", "FEEDBACK"]
        case .winterDeep:   return ["SUSTAIN", "REVERB", "DEEP"]
        }
    }

    /// Seasonal challenge description (full text for UI).
    var challenge: SeasonalChallenge {
        switch self {
        case .springBloom:
            return SeasonalChallenge(
                id: "spring_challenge",
                title: "The First Bloom",
                description: "Catch 3 Spring-exclusive specimens and breed one pair during the season.",
                catchTarget: 3,
                breedRequired: true,
                exclusiveCatchRequired: true
            )
        case .summerSurge:
            return SeasonalChallenge(
                id: "summer_challenge",
                title: "Peak Current",
                description: "Catch 5 Summer-exclusive specimens and wire a chain of 4 during the season.",
                catchTarget: 5,
                breedRequired: false,
                exclusiveCatchRequired: true
            )
        case .autumnDrift:
            return SeasonalChallenge(
                id: "autumn_challenge",
                title: "The Amber Migration",
                description: "Catch 4 Autumn-exclusive specimens and reach a Dive score of 800+.",
                catchTarget: 4,
                breedRequired: false,
                exclusiveCatchRequired: true
            )
        case .winterDeep:
            return SeasonalChallenge(
                id: "winter_challenge",
                title: "The Deep Vigil",
                description: "Catch 3 Winter-exclusive specimens. Tend your reef every day for 7 consecutive days.",
                catchTarget: 3,
                breedRequired: false,
                exclusiveCatchRequired: true
            )
        }
    }

    /// Which weather conditions become more probable during this season.
    var preferredWeatherConditions: [String] {
        switch self {
        case .springBloom:  return ["morningLight", "calm", "warmAfternoon"]
        case .summerSurge:  return ["warmAfternoon", "storm", "morningLight"]
        case .autumnDrift:  return ["eveningGlow", "calm", "nightDeep"]
        case .winterDeep:   return ["nightDeep", "calm", "storm"]
        }
    }

    /// Determine the current season from a Calendar date.
    static func current(on date: Date = Date()) -> Season {
        let month = Calendar.current.component(.month, from: date)
        switch month {
        case 3...5:  return .springBloom
        case 6...8:  return .summerSurge
        case 9...11: return .autumnDrift
        default:     return .winterDeep  // 12, 1, 2
        }
    }

    /// Number of calendar days remaining in this season from a given date.
    func daysRemaining(from date: Date = Date()) -> Int {
        let calendar = Calendar.current
        let year  = calendar.component(.year,  from: date)
        let month = calendar.component(.month, from: date)

        // Season end dates (last day of the final month at 23:59)
        let endComponents: DateComponents
        switch self {
        case .springBloom:
            endComponents = DateComponents(year: year, month: 5, day: 31, hour: 23, minute: 59)
        case .summerSurge:
            endComponents = DateComponents(year: year, month: 8, day: 31, hour: 23, minute: 59)
        case .autumnDrift:
            endComponents = DateComponents(year: year, month: 11, day: 30, hour: 23, minute: 59)
        case .winterDeep:
            // Winter spans year boundary — if month is 12, end is Feb of next year; else same year
            let endYear = month == 12 ? year + 1 : year
            // Compute the last day of February dynamically to handle leap years
            let lastDayOfFeb: Int
            if let febRange = calendar.range(of: .day, in: .month,
                                              for: calendar.date(from: DateComponents(year: endYear, month: 2))!) {
                lastDayOfFeb = febRange.upperBound - 1
            } else {
                lastDayOfFeb = 28
            }
            endComponents = DateComponents(year: endYear, month: 2, day: lastDayOfFeb, hour: 23, minute: 59)
        }
        guard let endDate = calendar.date(from: endComponents) else { return 0 }
        return max(0, calendar.dateComponents([.day], from: date, to: endDate).day ?? 0)
    }
}

// MARK: - Supporting Value Types

/// Simple RGBA color data. UI layers convert this to SwiftUI Color or UIColor.
struct RGBA: Codable, Equatable {
    let r: Float
    let g: Float
    let b: Float
    let a: Float
}

/// Seasonal color palette stored as data; UI reads and applies.
struct SeasonalPalette: Codable, Equatable {
    let primary:   RGBA
    let secondary: RGBA
    let ambient:   RGBA
    let waterTint: RGBA
}

/// A seasonal challenge the player must complete before the season ends.
struct SeasonalChallenge: Codable, Identifiable {
    let id: String
    let title: String
    let description: String
    let catchTarget: Int            // Number of seasonal exclusives to catch
    let breedRequired: Bool         // Must breed at least once
    let exclusiveCatchRequired: Bool

    var catchProgress: Int = 0
    var breedCompleted: Bool = false

    var isComplete: Bool {
        let catchMet = catchProgress >= catchTarget
        let breedMet = !breedRequired || breedCompleted
        return catchMet && breedMet
    }

    var completionFraction: Float {
        let catchFrac = Float(min(catchProgress, catchTarget)) / Float(max(catchTarget, 1))
        let breedFrac: Float = breedRequired ? (breedCompleted ? 1.0 : 0.0) : 1.0
        return breedRequired ? (catchFrac + breedFrac) / 2.0 : catchFrac
    }
}

// MARK: - Limited-Time Events

/// A short-duration event that fires within a season.
/// Events are deterministic: given the same seed + date, all players see the same schedule.
enum LimitedTimeEventType: String, CaseIterable, Codable {
    case bioluminescentStorm    // 3 days — doubled rare spawns, glowing catch conditions
    case migrationWave          // 1 day  — 10 simultaneous specimens, narrow catch window
    case deepCurrent            // 2 days — deep-biome specimens surface
    case coralBloom             // 4 days — breeding +50% success, nursery time halved

    var displayName: String {
        switch self {
        case .bioluminescentStorm: return "Bioluminescent Storm"
        case .migrationWave:       return "Migration Wave"
        case .deepCurrent:         return "Deep Current"
        case .coralBloom:          return "Coral Bloom"
        }
    }

    var description: String {
        switch self {
        case .bioluminescentStorm:
            return "The reef ignites. Rare voices glow to the surface, twice as often, for three nights."
        case .migrationWave:
            return "Ten specimens appear at once. The window is narrow. Move fast."
        case .deepCurrent:
            return "An upwelling from the deep. Abyssal specimens drift into shallower water."
        case .coralBloom:
            return "The reef is fertile. Breeding succeeds more often; the nursery works faster."
        }
    }

    var durationDays: Int {
        switch self {
        case .bioluminescentStorm: return 3
        case .migrationWave:       return 1
        case .deepCurrent:         return 2
        case .coralBloom:          return 4
        }
    }

    /// Rare spawn multiplier during this event (1.0 = no change).
    var rareSpawnMultiplier: Float {
        switch self {
        case .bioluminescentStorm: return 2.0
        case .migrationWave:       return 1.5
        case .deepCurrent:         return 1.0
        case .coralBloom:          return 1.0
        }
    }

    /// Breeding success rate bonus (additive fraction, 0 = no change).
    var breedingBonus: Float {
        switch self {
        case .coralBloom:          return 0.50
        default:                   return 0.0
        }
    }

    /// Nursery time multiplier (1.0 = no change, 0.5 = half time).
    var nurseryTimeMultiplier: Float {
        switch self {
        case .coralBloom:          return 0.5
        default:                   return 1.0
        }
    }

    /// Which subtypes surface from the deep during a Deep Current.
    var surfacedDeepSubtypes: [String] {
        guard self == .deepCurrent else { return [] }
        return ["noise-pink", "wt-analog", "wt-vocal", "svf-hp", "adsr-slow", "at-map", "shaper-soft", "polyblep-tri"]
    }

    /// Number of simultaneous specimens during a Migration Wave (0 if not applicable).
    var simultaneousSpecimenCount: Int {
        self == .migrationWave ? 10 : 0
    }
}

/// A scheduled instance of a LimitedTimeEventType with concrete start/end dates.
struct LimitedTimeEvent: Codable, Identifiable {
    let id: String
    let type: LimitedTimeEventType
    let season: Season
    let startDate: Date
    let endDate: Date

    var isActive: Bool {
        let now = Date()
        return now >= startDate && now <= endDate
    }

    var daysRemaining: Int {
        max(0, Calendar.current.dateComponents([.day], from: Date(), to: endDate).day ?? 0)
    }

    var hasStarted: Bool { Date() >= startDate }
    var hasEnded:   Bool { Date() > endDate }
}

// MARK: - Seasonal Reward

/// Rewards earned by completing seasonal challenges or experiencing events.
enum SeasonalReward: String, Codable, CaseIterable {
    // Cosmetic reef decorations
    case springCoralArch
    case summerKelpForest
    case autumnAmberBoulders
    case winterIceGrotto

    // Seasonal specimen variants (palette tints on standard specimens)
    case bloomVariant      // Spring-tinted specimen skin
    case surgeVariant      // Summer-tinted specimen skin
    case driftVariant      // Autumn-tinted specimen skin
    case deepVariant       // Winter-tinted specimen skin

    // Seasonal presets (DSP preset packs that unlock in ArrangementEngine)
    case springPresetPack
    case summerPresetPack
    case autumnPresetPack
    case winterPresetPack

    var displayName: String {
        switch self {
        case .springCoralArch:    return "Coral Arch"
        case .summerKelpForest:   return "Kelp Forest"
        case .autumnAmberBoulders: return "Amber Boulders"
        case .winterIceGrotto:    return "Ice Grotto"
        case .bloomVariant:       return "Bloom Tint"
        case .surgeVariant:       return "Surge Tint"
        case .driftVariant:       return "Drift Tint"
        case .deepVariant:        return "Deep Tint"
        case .springPresetPack:   return "Spring Sounds"
        case .summerPresetPack:   return "Summer Sounds"
        case .autumnPresetPack:   return "Autumn Sounds"
        case .winterPresetPack:   return "Winter Sounds"
        }
    }

    var description: String {
        switch self {
        case .springCoralArch:    return "A living arch of spring coral grows at the reef entrance."
        case .summerKelpForest:   return "Golden kelp sways in your reef's shallows."
        case .autumnAmberBoulders: return "Ancient boulders, amber with age, anchor the reef floor."
        case .winterIceGrotto:    return "A frozen grotto of ice-blue stone, luminous in the dark."
        case .bloomVariant:       return "A fresh-green tint — specimens carry spring wherever they go."
        case .surgeVariant:       return "Sunburst gold shimmers on every scale."
        case .driftVariant:       return "Amber warmth settles into their colors."
        case .deepVariant:        return "Ice-blue and silver mark a winter-born specimen."
        case .springPresetPack:   return "DSP presets tuned to spring: bright filters, fast attacks."
        case .summerPresetPack:   return "DSP presets tuned to summer: saturated, full, and present."
        case .autumnPresetPack:   return "DSP presets tuned to autumn: warm, decaying, resonant."
        case .winterPresetPack:   return "DSP presets tuned to winter: deep space, slow envelopes."
        }
    }

    /// The season this reward belongs to.
    var season: Season {
        switch self {
        case .springCoralArch, .bloomVariant, .springPresetPack:     return .springBloom
        case .summerKelpForest, .surgeVariant, .summerPresetPack:    return .summerSurge
        case .autumnAmberBoulders, .driftVariant, .autumnPresetPack: return .autumnDrift
        case .winterIceGrotto, .deepVariant, .winterPresetPack:      return .winterDeep
        }
    }
}

// MARK: - Seasonal Progression Record

/// Persistent record of everything the player has experienced season by season.
struct SeasonalProgressRecord: Codable {
    /// Seasons the player has lived through at least one day of.
    var experiencedSeasons: Set<String> = []

    /// Per-season challenge progress, keyed by challenge.id.
    var challengeProgress: [String: SeasonalChallenge] = [:]

    /// Rewards the player has unlocked, keyed by SeasonalReward.rawValue.
    var unlockedRewards: Set<String> = []

    /// The seasonal event IDs the player has witnessed (event was active while app was open).
    var witnessedEventIDs: Set<String> = []

    mutating func markSeasonExperienced(_ season: Season) {
        experiencedSeasons.insert(season.rawValue)
    }

    func hasExperienced(_ season: Season) -> Bool {
        experiencedSeasons.contains(season.rawValue)
    }

    var experiencedSeasonCount: Int { experiencedSeasons.count }

    mutating func unlockReward(_ reward: SeasonalReward) {
        unlockedRewards.insert(reward.rawValue)
    }

    func hasReward(_ reward: SeasonalReward) -> Bool {
        unlockedRewards.contains(reward.rawValue)
    }
}

// MARK: - SeasonalEventManager

/// Manages the full lifecycle of seasons, limited-time events, challenges, and seasonal rewards.
///
/// Events are scheduled deterministically using a fixed global seed combined with
/// the calendar year + week number — every player on the same date sees the same events.
/// Persistence is via UserDefaults for simplicity; swap to DatabaseManager if size grows.
final class SeasonalEventManager: ObservableObject {

    // MARK: - Published State

    @Published var currentSeason: Season = .springBloom
    @Published var activeEvent: LimitedTimeEvent? = nil
    @Published var upcomingEvent: LimitedTimeEvent? = nil
    @Published var challengeProgress: SeasonalChallenge
    @Published var progressRecord: SeasonalProgressRecord = SeasonalProgressRecord()

    // MARK: - Internal State

    /// Deterministic seed for event scheduling — same value for all players.
    /// 0x4F42523158 = ASCII "OBR1X" — memorable and stable across builds.
    private let scheduleSeed: UInt64 = 0x4F42523158

    private let storageKey       = "obrix_seasonal_state"
    private let progressKey      = "obrix_seasonal_progress"

    // MARK: - Init

    init() {
        let season = Season.current()
        self.currentSeason     = season
        self.challengeProgress = season.challenge

        restore()
        refreshActiveEvent()
        markCurrentSeasonExperienced()
    }

    // MARK: - Public API

    /// Call when a new seasonal exclusive specimen is caught.
    func recordSeasonalCatch(subtypeID: String) {
        guard currentSeason.exclusiveSubtypes.contains(subtypeID) else { return }
        challengeProgress.catchProgress = min(
            challengeProgress.catchTarget,
            challengeProgress.catchProgress + 1
        )
        progressRecord.challengeProgress[challengeProgress.id] = challengeProgress
        checkChallengeCompletion()
        save()
    }

    /// Call when the player successfully breeds a pair.
    func recordBreedingSuccess() {
        guard challengeProgress.breedRequired, !challengeProgress.breedCompleted else { return }
        challengeProgress.breedCompleted = true
        progressRecord.challengeProgress[challengeProgress.id] = challengeProgress
        checkChallengeCompletion()
        save()
    }

    /// Call when the app enters foreground. Refreshes season, events, and experienced record.
    func refresh() {
        let newSeason = Season.current()
        if newSeason != currentSeason {
            currentSeason     = newSeason
            challengeProgress = restoredChallenge(for: newSeason)
        }
        refreshActiveEvent()
        markCurrentSeasonExperienced()
    }

    /// Spawn rate multiplier for a given specimen subtype given current season + active event.
    func spawnRateMultiplier(for subtypeID: String) -> Float {
        var multiplier: Float = 1.0

        // Season base modifier
        if let seasonMod = currentSeason.spawnRateModifiers[subtypeID] {
            multiplier *= seasonMod
        }

        // Exclusive subtype: only spawns during its season, blocked otherwise
        for season in Season.allCases where season != currentSeason {
            if season.exclusiveSubtypes.contains(subtypeID) {
                return 0.0   // Not this season — blocked
            }
        }

        // Active event override
        if let event = activeEvent {
            switch event.type {
            case .bioluminescentStorm:
                // All rare subtypes doubled
                multiplier *= event.type.rareSpawnMultiplier
            case .deepCurrent:
                // Deep subtypes surface
                if event.type.surfacedDeepSubtypes.contains(subtypeID) {
                    multiplier *= 2.5
                }
            case .migrationWave:
                // All subtypes slightly boosted
                multiplier *= 1.3
            case .coralBloom:
                break // No spawn change during Coral Bloom
            }
        }

        return multiplier
    }

    /// Breeding success rate bonus from the active event (0 if none).
    var breedingBonus: Float {
        activeEvent?.type.breedingBonus ?? 0.0
    }

    /// Nursery time multiplier from the active event (1.0 if none).
    var nurseryTimeMultiplier: Float {
        activeEvent?.type.nurseryTimeMultiplier ?? 1.0
    }

    /// Coupling affinity bonus for a given coupling type this season (+0.20 if applicable).
    func couplingAffinityBonus(for couplingType: String) -> Float {
        currentSeason.couplingAffinityBonuses.contains(couplingType) ? 0.20 : 0.0
    }

    // MARK: - Event Scheduling

    /// Rebuild the active + upcoming event from the deterministic schedule for this year.
    func refreshActiveEvent() {
        let year      = Calendar.current.component(.year, from: Date())
        let scheduled = scheduledEvents(year: year)

        activeEvent   = scheduled.first { $0.isActive }
        upcomingEvent = scheduled.first { !$0.hasStarted }

        if let active = activeEvent {
            progressRecord.witnessedEventIDs.insert(active.id)
            save()
        }
    }

    /// Build the full calendar of limited-time events for a given year.
    /// Uses a deterministic pseudo-random sequence seeded by (seed XOR year) —
    /// stable within a year, different between years. All players on the same date
    /// see the same schedule.
    func scheduledEvents(year: Int) -> [LimitedTimeEvent] {
        var events: [LimitedTimeEvent] = []
        let calendar = Calendar.current

        let yearMix = UInt64(bitPattern: Int64(year)) ^ scheduleSeed

        // (season, start month, start year offset, approx total days)
        // Winter starts in December of `year` but we only schedule its events in the
        // December portion; the January–February tail is handled by next year's call.
        let seasonDefs: [(Season, Int, Int, Int)] = [
            (.springBloom, 3, 0, 92),
            (.summerSurge, 6, 0, 92),
            (.autumnDrift, 9, 0, 91),
            (.winterDeep, 12, 0, 31),   // December only — keeps events within the year
        ]

        for (season, startMonth, yearOffset, totalDays) in seasonDefs {
            let seasonYear = year + yearOffset
            guard let seasonStart = calendar.date(from: DateComponents(
                year: seasonYear, month: startMonth, day: 1)) else { continue }

            let types = eventsForSeason(season)
            var dayOffset = Int((yearMix >> 3) % UInt64(max(1, totalDays / 2))) + 7

            for type in types {
                let eventStart = seasonStart.addingTimeInterval(TimeInterval(dayOffset * 86400))
                let eventEnd   = eventStart.addingTimeInterval(TimeInterval(type.durationDays * 86400))

                events.append(LimitedTimeEvent(
                    id:        "\(season.rawValue)_\(type.rawValue)_\(year)",
                    type:      type,
                    season:    season,
                    startDate: eventStart,
                    endDate:   eventEnd
                ))

                dayOffset += type.durationDays + Int((yearMix >> 7) % 10) + 5
                if dayOffset >= totalDays - type.durationDays { break }
            }
        }

        return events.sorted { $0.startDate < $1.startDate }
    }

    /// Which limited-time event types can fire within each season.
    private func eventsForSeason(_ season: Season) -> [LimitedTimeEventType] {
        switch season {
        case .springBloom:  return [.coralBloom, .migrationWave]
        case .summerSurge:  return [.bioluminescentStorm, .migrationWave]
        case .autumnDrift:  return [.deepCurrent, .bioluminescentStorm]
        case .winterDeep:   return [.deepCurrent, .coralBloom]
        }
    }

    // MARK: - Challenge Completion

    private func checkChallengeCompletion() {
        guard challengeProgress.isComplete else { return }
        let reward = completionReward(for: currentSeason)
        progressRecord.unlockReward(reward)
        save()
    }

    private func completionReward(for season: Season) -> SeasonalReward {
        switch season {
        case .springBloom:  return .springCoralArch
        case .summerSurge:  return .summerKelpForest
        case .autumnDrift:  return .autumnAmberBoulders
        case .winterDeep:   return .winterIceGrotto
        }
    }

    // MARK: - Season Tracking

    private func markCurrentSeasonExperienced() {
        progressRecord.markSeasonExperienced(currentSeason)
        save()
    }

    // MARK: - Persistence

    func save() {
        let encoder = JSONEncoder()
        if let data = try? encoder.encode(progressRecord) {
            UserDefaults.standard.set(data, forKey: progressKey)
        }

        // Save active event ID for quick restore
        if let event = activeEvent {
            UserDefaults.standard.set(event.id, forKey: storageKey)
        } else {
            UserDefaults.standard.removeObject(forKey: storageKey)
        }
    }

    func restore() {
        let decoder = JSONDecoder()
        if let data   = UserDefaults.standard.data(forKey: progressKey),
           let record = try? decoder.decode(SeasonalProgressRecord.self, from: data) {
            progressRecord = record
        }

        // Restore challenge progress for current season
        challengeProgress = restoredChallenge(for: currentSeason)
    }

    private func restoredChallenge(for season: Season) -> SeasonalChallenge {
        let template = season.challenge
        return progressRecord.challengeProgress[template.id] ?? template
    }
}

