import Foundation

// MARK: - CommunityEventType

enum CommunityEventType: String, Codable, CaseIterable {
    case catchathon       // Community catches X total specimens
    case breedathon       // Community breeds X total offspring
    case diveDeep         // Community reaches cumulative X meters of dive depth
    case seasonalHunt     // Catch X of a specific seasonal specimen
    case musicMarathon    // Community logs X total minutes of arrangement/dive play time
    case discoveryRush    // Community discovers X new subtype entries across all players

    var displayName: String {
        switch self {
        case .catchathon:     return "Catchathon"
        case .breedathon:     return "Breedathon"
        case .diveDeep:       return "Deep Descent"
        case .seasonalHunt:   return "Seasonal Hunt"
        case .musicMarathon:  return "Music Marathon"
        case .discoveryRush:  return "Discovery Rush"
        }
    }

    /// Base participation rate used when simulating global progress.
    var participationRate: Float {
        switch self {
        case .catchathon:     return 0.55
        case .breedathon:     return 0.40
        case .diveDeep:       return 0.60
        case .seasonalHunt:   return 0.45
        case .musicMarathon:  return 0.50
        case .discoveryRush:  return 0.35
        }
    }

    /// How many units a single dedicated player contributes per day (for target calibration).
    var singlePlayerDailyRate: Int {
        switch self {
        case .catchathon:     return 8    // ~8 catches per day
        case .breedathon:     return 3    // ~3 breeds per day
        case .diveDeep:       return 800  // ~800 meters per day
        case .seasonalHunt:   return 4    // ~4 seasonal specimens per day
        case .musicMarathon:  return 30   // ~30 minutes per day
        case .discoveryRush:  return 2    // ~2 new subtypes per day
        }
    }
}

// MARK: - CommunityReward

enum CommunityReward: String, Codable, CaseIterable {
    case reefDecoration        // Cosmetic reef decoration unlocked for all contributors
    case rareSpecimenVariant   // Rare specimen variant (palette) becomes available
    case bonusReefEnergy       // Bonus Reef Energy currency drop
    case exclusiveAchievement  // Achievement badge, contribution-gated
    case seasonalPresetPack    // A seasonal DSP preset pack

    var displayName: String {
        switch self {
        case .reefDecoration:       return "Reef Decoration"
        case .rareSpecimenVariant:  return "Rare Variant"
        case .bonusReefEnergy:      return "Bonus Reef Energy"
        case .exclusiveAchievement: return "Community Badge"
        case .seasonalPresetPack:   return "Seasonal Preset Pack"
        }
    }

    var description: String {
        switch self {
        case .reefDecoration:
            return "A new decoration grows in every contributor's reef."
        case .rareSpecimenVariant:
            return "A rare colour variant becomes catchable for all participants."
        case .bonusReefEnergy:
            return "A burst of Reef Energy appears in your collection."
        case .exclusiveAchievement:
            return "A community badge only participants of this event can earn."
        case .seasonalPresetPack:
            return "A fresh set of DSP presets tuned to this event's mood."
        }
    }
}

// MARK: - ContributionType

/// The player action types that count toward community event goals.
enum ContributionType: String, Codable, CaseIterable {
    case catchSpecimen  // Catching a specimen
    case breed      // Successfully breeding two specimens
    case dive       // Meters of dive depth accumulated
    case play       // Minutes of play time (arrangement or dive)
    case discover   // New subtype entry added to the player's catalog

    /// Whether this contribution type counts toward the given event type.
    func countsFor(_ eventType: CommunityEventType) -> Bool {
        switch (self, eventType) {
        case (.catchSpecimen, .catchathon):    return true
        case (.catchSpecimen, .seasonalHunt):  return true
        case (.breed,    .breedathon):    return true
        case (.dive,     .diveDeep):      return true
        case (.play,     .musicMarathon): return true
        case (.discover, .discoveryRush): return true
        default:                          return false
        }
    }
}

// MARK: - CommunityEvent

/// A community event running for 3-7 days. Since there is no server, events are
/// deterministic — the same calendar date produces the same event for all players.
/// Global progress is simulated to create the sensation of collective momentum.
struct CommunityEvent: Codable, Identifiable {
    let eventId: String
    let title: String
    let description: String
    let eventType: CommunityEventType
    let startDate: Date
    let endDate: Date
    let goalTarget: Int                    // Total goal units for the "community"
    var localContribution: Int             // This player's cumulative contribution
    var estimatedGlobalProgress: Float     // Simulated 0.0–1.0 — updated when contribution is recorded
    let rewards: [CommunityReward]

    var id: String { eventId }

    var isActive: Bool {
        let now = Date()
        return now >= startDate && now < endDate
    }

    var hasEnded: Bool { Date() >= endDate }

    var daysRemaining: Int {
        max(0, Calendar.current.dateComponents([.day], from: Date(), to: endDate).day ?? 0)
    }

    /// Local contribution as a fraction of the goal target (0–1, capped at 1).
    var localFraction: Float {
        guard goalTarget > 0 else { return 0 }
        return min(1.0, Float(localContribution) / Float(goalTarget))
    }

    /// True once simulated global progress crosses 80%.
    var communityGoalMet: Bool { estimatedGlobalProgress >= 0.8 }
}

// MARK: - Event Templates

/// Pre-written event templates — 2 per CommunityEventType (12 total).
/// The generator picks from these using the deterministic seed.
private struct EventTemplate {
    let title: String
    let description: String
    let type: CommunityEventType
    let durationDays: Int
    let rewards: [CommunityReward]
}

private let eventTemplates: [CommunityEventType: [EventTemplate]] = [
    .catchathon: [
        EventTemplate(
            title: "The Great Haul",
            description: "Reefs everywhere are teeming. Every hand in the water makes the count rise.",
            type: .catchathon,
            durationDays: 5,
            rewards: [.bonusReefEnergy, .exclusiveAchievement]
        ),
        EventTemplate(
            title: "Frenzy Season",
            description: "The current shifts and specimens flood the shallows. Catch while the tide is high.",
            type: .catchathon,
            durationDays: 4,
            rewards: [.rareSpecimenVariant, .bonusReefEnergy]
        ),
    ],
    .breedathon: [
        EventTemplate(
            title: "Coral Nursery Drive",
            description: "New life is the reef's greatest resource. Every pairing strengthens the whole.",
            type: .breedathon,
            durationDays: 6,
            rewards: [.reefDecoration, .exclusiveAchievement]
        ),
        EventTemplate(
            title: "The Pairing Event",
            description: "Match by match, the community builds something no single reef could.",
            type: .breedathon,
            durationDays: 5,
            rewards: [.rareSpecimenVariant, .seasonalPresetPack]
        ),
    ],
    .diveDeep: [
        EventTemplate(
            title: "Collective Descent",
            description: "Together we go deeper than any one diver could alone. Every meter counts.",
            type: .diveDeep,
            durationDays: 7,
            rewards: [.reefDecoration, .seasonalPresetPack]
        ),
        EventTemplate(
            title: "The Depth Record",
            description: "A community-wide run for cumulative depth. The abyss keeps count.",
            type: .diveDeep,
            durationDays: 5,
            rewards: [.exclusiveAchievement, .bonusReefEnergy]
        ),
    ],
    .seasonalHunt: [
        EventTemplate(
            title: "Bloom Seekers",
            description: "This season's rarest voices have been spotted. Find them before the window closes.",
            type: .seasonalHunt,
            durationDays: 4,
            rewards: [.rareSpecimenVariant, .exclusiveAchievement]
        ),
        EventTemplate(
            title: "The Migration Window",
            description: "They move with the current. Catch them now — they won't pass this way again for a season.",
            type: .seasonalHunt,
            durationDays: 3,
            rewards: [.reefDecoration, .bonusReefEnergy]
        ),
    ],
    .musicMarathon: [
        EventTemplate(
            title: "The Long Listen",
            description: "Play, arrange, dive. Every minute of music adds to the collective sound.",
            type: .musicMarathon,
            durationDays: 7,
            rewards: [.seasonalPresetPack, .exclusiveAchievement]
        ),
        EventTemplate(
            title: "Always Playing",
            description: "The reef never sleeps. Neither do we. Play as long as you can.",
            type: .musicMarathon,
            durationDays: 6,
            rewards: [.bonusReefEnergy, .reefDecoration]
        ),
    ],
    .discoveryRush: [
        EventTemplate(
            title: "The Catalog Sprint",
            description: "New entries, new voices. Each discovery lights up the collective map.",
            type: .discoveryRush,
            durationDays: 5,
            rewards: [.rareSpecimenVariant, .seasonalPresetPack]
        ),
        EventTemplate(
            title: "The Unnamed Things",
            description: "The deep still has its secrets. The community catalogs what was unknown.",
            type: .discoveryRush,
            durationDays: 4,
            rewards: [.exclusiveAchievement, .reefDecoration]
        ),
    ],
]

// MARK: - CommunityEventManager

/// Manages the community event calendar. Events are purely local and deterministic:
/// all players on the same date generate the same events from the same seed.
///
/// Since there is no server, "global progress" is simulated from the player's own
/// contribution scaled by estimated player count and participation rate. The goal
/// targets are calibrated so a dedicated single player contributes ~5-10%, and the
/// simulated community always reaches ~80%+ (events succeed — this is meant to be fun).
///
/// Two events can be active simultaneously. A new event fires every 5 days, with
/// a 2-day gap between events.
final class CommunityEventManager: ObservableObject {

    // MARK: - Published State

    @Published var activeEvents: [CommunityEvent] = []
    @Published var completedEvents: [CommunityEvent] = []
    @Published var localContributions: [String: Int] = [:]     // eventId → total contribution

    // MARK: - Constants

    /// Deterministic seed: 0x434F4D4D = ASCII "COMM"
    private static let baseSeed: UInt32 = 0x434F4D4D

    /// Max simultaneous active events.
    private static let maxActiveEvents = 2

    /// New event every 5 days, 2-day gap between events.
    private static let eventIntervalDays = 5
    private static let eventGapDays = 2

    /// Estimated player count tiers (hardcoded; scales optimistically over time).
    /// Year offsets from a base launch year — expand tiers as the game grows.
    private static let estimatedPlayerCount: Int = 100

    // MARK: - Persistence

    private let activeEventsKey    = "obrix_community_active_events"
    private let completedEventsKey = "obrix_community_completed_events"
    private let contributionsKey   = "obrix_community_contributions"

    // MARK: - Init

    init() {
        restore()
        refresh()
    }

    // MARK: - Public API

    /// Call when the player performs an action that may contribute to a community event.
    /// - Parameters:
    ///   - eventId: The event to credit. If nil, credits all matching active events.
    ///   - type: What action was performed.
    ///   - amount: How many units to add (e.g. 1 catch, 150 dive meters, 5 play minutes).
    func recordContribution(eventId: String? = nil, type: ContributionType, amount: Int) {
        let targets: [CommunityEvent]
        if let id = eventId {
            targets = activeEvents.filter { $0.eventId == id }
        } else {
            targets = activeEvents.filter { type.countsFor($0.eventType) && $0.isActive }
        }

        for var event in targets {
            event.localContribution += amount

            // Update persistent contribution total
            localContributions[event.eventId, default: 0] += amount

            // Recalculate simulated global progress
            event.estimatedGlobalProgress = simulatedGlobalProgress(
                localContribution: event.localContribution,
                goalTarget: event.goalTarget,
                eventType: event.eventType
            )

            // Write back into the array
            if let idx = activeEvents.firstIndex(where: { $0.eventId == event.eventId }) {
                activeEvents[idx] = event
            }
        }

        save()
    }

    /// Call from the app foreground / scene phase change to advance the event calendar.
    func refresh() {
        let calendar = Calendar.current
        let now = Date()

        // Move expired active events to completed
        let (stillActive, expired) = activeEvents.reduce(
            into: ([CommunityEvent](), [CommunityEvent]())
        ) { acc, event in
            if event.hasEnded { acc.1.append(event) } else { acc.0.append(event) }
        }
        activeEvents = stillActive
        completedEvents = (expired + completedEvents)
            .sorted { $0.endDate > $1.endDate }

        // Generate today's expected events and add any we don't already have
        let year = calendar.component(.year, from: now)
        let dayOfYear = calendar.ordinality(of: .day, in: .year, for: now) ?? 1
        let expectedEvents = generateEvents(year: year, dayOfYear: dayOfYear)

        for event in expectedEvents where !alreadyTracked(event) {
            if activeEvents.count < Self.maxActiveEvents && event.isActive {
                activeEvents.append(event)
            }
        }

        save()
    }

    // MARK: - Event Generation (Deterministic)

    /// Generate the expected events that should be active on a given calendar day.
    ///
    /// Each event window is seeded from its own start-day (year × 1000 + windowStartDayOfYear XOR baseSeed),
    /// NOT from the current day. This guarantees an event's type, duration, and template are identical
    /// regardless of which day `generateEvents` is called — Day 100 and Day 105 both produce the
    /// same event for a window starting on Day 100.
    func generateEvents(year: Int, dayOfYear: Int) -> [CommunityEvent] {
        // We generate a rolling window: look back 14 days and forward 14 days.
        // Events that fall within the active window are returned.
        let calendar = Calendar.current
        guard let windowStart = calendar.date(
            byAdding: .day, value: -14,
            to: calendar.startOfDay(for: Date())
        ) else { return [] }

        var events: [CommunityEvent] = []
        var cursor = windowStart
        let windowEnd = cursor.addingTimeInterval(TimeInterval(28 * 86400))

        while cursor < windowEnd {
            // Seed from this window's START day — stable across all calls within the window.
            let windowStartDayOfYear = calendar.ordinality(of: .day, in: .year, for: cursor) ?? 1
            let windowYear = calendar.component(.year, from: cursor)
            let windowSeed = UInt32(truncatingIfNeeded: windowYear &* 1000 &+ windowStartDayOfYear) ^ Self.baseSeed
            var rng = SeededRandom(seed: windowSeed)

            // Pick event type from seed
            let typeIndex = Int(rng.next() % UInt32(CommunityEventType.allCases.count))
            let type = CommunityEventType.allCases[typeIndex]

            // Duration: 3-7 days
            let duration = 3 + Int(rng.next() % 5)

            // Pick template
            let templates = eventTemplates[type] ?? []
            let templateIndex = templates.isEmpty ? 0 : Int(rng.next() % UInt32(templates.count))
            let template: EventTemplate? = templates.isEmpty ? nil : templates[templateIndex]

            // Goal target: single dedicated player contributes ~5-10% of the target over event duration
            let singlePlayerTotal = type.singlePlayerDailyRate * duration
            let targetMultiplier = 10 + Int(rng.next() % 11)  // 10x – 20x single player
            let goalTarget = singlePlayerTotal * targetMultiplier

            let eventId = "community_\(type.rawValue)_\(windowYear)_\(windowStartDayOfYear)"
            let endDate = cursor.addingTimeInterval(TimeInterval(duration * 86400))

            let event = CommunityEvent(
                eventId: eventId,
                title: template?.title ?? type.displayName,
                description: template?.description ?? "The community works together toward a shared goal.",
                eventType: type,
                startDate: cursor,
                endDate: endDate,
                goalTarget: goalTarget,
                localContribution: localContributions[eventId] ?? 0,
                estimatedGlobalProgress: simulatedGlobalProgress(
                    localContribution: localContributions[eventId] ?? 0,
                    goalTarget: goalTarget,
                    eventType: type
                ),
                rewards: template?.rewards ?? [.bonusReefEnergy]
            )

            events.append(event)

            // Advance cursor: event duration + 2-day gap
            cursor = endDate.addingTimeInterval(TimeInterval(Self.eventGapDays * 86400))
        }

        return events
    }

    // MARK: - Simulated Global Progress

    /// Estimates community-wide progress without a server.
    /// Formula: localContribution × estimatedPlayerCount × participationRate / goalTarget
    /// Clamped to 0.80–1.0 once the event has past its halfway mark (events always "succeed").
    func simulatedGlobalProgress(localContribution: Int,
                                 goalTarget: Int,
                                 eventType: CommunityEventType) -> Float {
        guard goalTarget > 0 else { return 0 }

        let participationRate = eventType.participationRate
        let estimatedTotal = Float(localContribution)
            * Float(Self.estimatedPlayerCount)
            * participationRate
        let rawProgress = estimatedTotal / Float(goalTarget)

        // Events always succeed at 80%+ once we have any meaningful contribution.
        // A single player's honest play puts the community over the line.
        if localContribution > 0 {
            return min(1.0, max(0.80, rawProgress))
        }
        return min(1.0, rawProgress)
    }

    // MARK: - Helpers

    private func alreadyTracked(_ event: CommunityEvent) -> Bool {
        activeEvents.contains(where: { $0.eventId == event.eventId })
            || completedEvents.contains(where: { $0.eventId == event.eventId })
    }

    // MARK: - Persistence

    func save() {
        let encoder = JSONEncoder()
        if let data = try? encoder.encode(activeEvents) {
            UserDefaults.standard.set(data, forKey: activeEventsKey)
        }
        if let data = try? encoder.encode(completedEvents) {
            UserDefaults.standard.set(data, forKey: completedEventsKey)
        }
        if let data = try? encoder.encode(localContributions) {
            UserDefaults.standard.set(data, forKey: contributionsKey)
        }
    }

    func restore() {
        let decoder = JSONDecoder()
        if let data = UserDefaults.standard.data(forKey: activeEventsKey),
           let saved = try? decoder.decode([CommunityEvent].self, from: data) {
            activeEvents = saved
        }
        if let data = UserDefaults.standard.data(forKey: completedEventsKey),
           let saved = try? decoder.decode([CommunityEvent].self, from: data) {
            completedEvents = saved
        }
        if let data = UserDefaults.standard.data(forKey: contributionsKey),
           let saved = try? decoder.decode([String: Int].self, from: data) {
            localContributions = saved
        }
    }
}

// MARK: - SeededRandom

/// A simple LCG PRNG seeded with a UInt32. Used for deterministic event generation.
/// All players on the same date with the same seed produce identical outputs.
private struct SeededRandom {
    private var state: UInt64

    init(seed: UInt32) {
        // Mix the 32-bit seed up to 64 bits with a fixed constant.
        state = UInt64(seed) &* 6364136223846793005 &+ 1442695040888963407
    }

    /// Next pseudo-random UInt32.
    mutating func next() -> UInt32 {
        state = state &* 6364136223846793005 &+ 1442695040888963407
        return UInt32(truncatingIfNeeded: (state >> 17) ^ state)
    }
}
