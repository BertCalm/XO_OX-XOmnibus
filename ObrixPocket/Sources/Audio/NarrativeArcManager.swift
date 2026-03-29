import Foundation

// MARK: - Narrative Act

/// The five acts of the reef's living story.
/// Acts are time-gated by approximate player-days AND milestone completion —
/// both conditions must be met before the act advances.
enum NarrativeAct: Int, CaseIterable, Codable, Comparable {
    case theFirstNote    = 1   // Day 1–7:    Tutorial — first sound, first catch, first wire
    case theGrowingReef  = 2   // Day 8–30:   Collection — 10 specimens, first breeding, first season
    case theDeepCall     = 3   // Day 31–90:  Exploration — all biomes, first rare, Gen-3+ breeding
    case theLivingSong   = 4   // Day 91–180: Mastery — 50 specimens, Elder, full arrangement
    case theEndlessOcean = 5   // Day 180+:   Endgame — complete collection, Gen-10, legendary

    var displayName: String {
        switch self {
        case .theFirstNote:    return "The First Note"
        case .theGrowingReef:  return "The Growing Reef"
        case .theDeepCall:     return "The Deep Call"
        case .theLivingSong:   return "The Living Song"
        case .theEndlessOcean: return "The Endless Ocean"
        }
    }

    var subtitle: String {
        switch self {
        case .theFirstNote:    return "Day 1–7"
        case .theGrowingReef:  return "Day 8–30"
        case .theDeepCall:     return "Day 31–90"
        case .theLivingSong:   return "Day 91–180"
        case .theEndlessOcean: return "Day 180+"
        }
    }

    /// Minimum player-days required to enter this act (gating condition 1 of 2).
    var minimumDays: Int {
        switch self {
        case .theFirstNote:    return 1
        case .theGrowingReef:  return 8
        case .theDeepCall:     return 31
        case .theLivingSong:   return 91
        case .theEndlessOcean: return 181
        }
    }

    /// Poetic description of this act's theme. Displayed at act entry and in the journal.
    var themeText: String {
        switch self {
        case .theFirstNote:
            return "The reef was silent once. You changed that. A single note became the beginning of everything."
        case .theGrowingReef:
            return "Others have answered. The reef fills with voices — some in harmony, some still searching. It is no longer just yours."
        case .theDeepCall:
            return "Something deeper stirs. The shallow water is not the whole story. Below the light, the real reef waits."
        case .theLivingSong:
            return "You have built something alive. The voices know each other now. The reef remembers what you have made."
        case .theEndlessOcean:
            return "The ocean was never something to finish. It is a way of listening. You have learned to listen."
        }
    }

    /// What mechanic or reef feature unlocks when this act is completed.
    var completionRewardDescription: String {
        switch self {
        case .theFirstNote:
            return "The Dive unlocks. Take your specimens into the deep for the first time."
        case .theGrowingReef:
            return "The Nursery opens. Bred specimens carry the voices of their parents."
        case .theDeepCall:
            return "The Arrangement Engine activates. Multiple voices can play together as a living song."
        case .theLivingSong:
            return "The Reef Archive unlocks. Your reef's history becomes navigable, replayable, shareable."
        case .theEndlessOcean:
            return "Legendary Specimens surface. Unique, unrepeatable, once per lifetime of the reef."
        }
    }

    /// Visual reef transformation token. UI reads this to apply a theme shift.
    var reefTransformation: ReefTransformation {
        switch self {
        case .theFirstNote:
            return ReefTransformation(
                id: "act1_first_note",
                stageName: "Bare Reef",
                description: "A single coral — small, new, mostly quiet.",
                ambientIntensity: 0.15,
                creatureActivityLevel: 0.1,
                lightingShift: "dawn"
            )
        case .theGrowingReef:
            return ReefTransformation(
                id: "act2_growing",
                stageName: "Young Reef",
                description: "Coral reaches in all directions. A few voices call back.",
                ambientIntensity: 0.35,
                creatureActivityLevel: 0.3,
                lightingShift: "morning"
            )
        case .theDeepCall:
            return ReefTransformation(
                id: "act3_deep_call",
                stageName: "Living Reef",
                description: "The structure is dense now. Shadows suggest more below.",
                ambientIntensity: 0.55,
                creatureActivityLevel: 0.55,
                lightingShift: "midday"
            )
        case .theLivingSong:
            return ReefTransformation(
                id: "act4_living_song",
                stageName: "Singing Reef",
                description: "The reef breathes in concert. Every voice knows its place.",
                ambientIntensity: 0.75,
                creatureActivityLevel: 0.80,
                lightingShift: "golden_hour"
            )
        case .theEndlessOcean:
            return ReefTransformation(
                id: "act5_endless_ocean",
                stageName: "Ancient Reef",
                description: "Time has layered itself into the stone. The reef is geology now.",
                ambientIntensity: 0.95,
                creatureActivityLevel: 1.0,
                lightingShift: "bioluminescent_night"
            )
        }
    }

    /// The next act, or nil if already at the final act.
    var next: NarrativeAct? {
        NarrativeAct(rawValue: rawValue + 1)
    }

    static func < (lhs: NarrativeAct, rhs: NarrativeAct) -> Bool {
        lhs.rawValue < rhs.rawValue
    }
}

// MARK: - Reef Transformation

/// Data-only description of a visual state the reef takes on at each narrative act.
/// UI layers read this and apply the corresponding visual theme.
struct ReefTransformation: Codable, Equatable {
    let id: String
    let stageName: String
    let description: String
    /// 0–1 scale for ambient particle/bubble intensity.
    let ambientIntensity: Float
    /// 0–1 scale for how active background creatures are.
    let creatureActivityLevel: Float
    /// Named lighting preset string the UI maps to a concrete color grading.
    let lightingShift: String
}

// MARK: - Narrative Milestone Type

/// The categories of actions that can trigger narrative milestones.
enum NarrativeMilestoneType: String, Codable, CaseIterable {
    case specimenCount          // Reached N total specimens
    case breedingGeneration     // Bred a Gen-N or higher specimen
    case biomeDiscovery         // Discovered all specimens in a biome
    case seasonExperienced      // Lived through a full season
    case couplingType           // Used a specific coupling type for the first time
    case arrangementCreated     // Created and saved an arrangement
    case diveScore              // Achieved a Dive score of N or higher
    case elderAchieved          // A specimen reached Elder tier
    case tradeCompleted         // Completed an NFC trade
    case streakReached          // Maintained a daily streak of N days
    case firstNote              // Played the very first note
    case firstWire              // Created the first wire between specimens
    case firstCatch             // Caught the very first specimen
    case rareSpecimenCaught     // Caught a rare-rarity specimen

    var displayName: String {
        switch self {
        case .specimenCount:       return "Collection Threshold"
        case .breedingGeneration:  return "Breeding Milestone"
        case .biomeDiscovery:      return "Biome Completed"
        case .seasonExperienced:   return "Season Lived"
        case .couplingType:        return "New Coupling"
        case .arrangementCreated:  return "Arrangement Saved"
        case .diveScore:           return "Dive Score"
        case .elderAchieved:       return "Elder Reached"
        case .tradeCompleted:      return "Trade Completed"
        case .streakReached:       return "Streak Milestone"
        case .firstNote:           return "First Note"
        case .firstWire:           return "First Wire"
        case .firstCatch:          return "First Catch"
        case .rareSpecimenCaught:  return "Rare Caught"
        }
    }
}

// MARK: - Narrative Milestone

/// A milestone definition tied to a narrative act with a poetic passage.
struct NarrativeMilestone: Codable, Identifiable {
    let id: String
    let act: NarrativeAct
    let type: NarrativeMilestoneType
    /// Target value (e.g., count=10 for specimenCount, generation=3 for breedingGeneration).
    let targetValue: Int
    /// Short display title.
    let title: String
    /// Poetic 2–3 sentence passage revealed when the milestone is hit.
    let narrativeText: String
    /// What the player unlocks or gains (1 sentence, practical).
    let rewardDescription: String
    /// Whether this milestone must be completed to advance the act.
    let isActRequired: Bool

    var isCompleted: Bool = false
    var completedDate: Date? = nil
}

// MARK: - Narrative Event

/// A single fired narrative moment: timestamp, act, milestone, text, and read status.
/// Unread events surface in the UI as an indicator.
struct NarrativeEvent: Codable, Identifiable {
    let id: UUID
    let timestamp: Date
    let act: NarrativeAct
    let milestoneID: String
    let milestoneTitle: String
    let narrativeText: String
    var hasBeenRead: Bool

    init(milestone: NarrativeMilestone) {
        self.id             = UUID()
        self.timestamp      = Date()
        self.act            = milestone.act
        self.milestoneID    = milestone.id
        self.milestoneTitle = milestone.title
        self.narrativeText  = milestone.narrativeText
        self.hasBeenRead    = false
    }
}

// MARK: - Act Progress

/// Persistent progress snapshot for a single act.
struct ActProgress: Codable {
    let act: NarrativeAct
    var completedMilestoneIDs: Set<String>
    var isActComplete: Bool
    var completedDate: Date?

    init(act: NarrativeAct) {
        self.act                  = act
        self.completedMilestoneIDs = []
        self.isActComplete        = false
        self.completedDate        = nil
    }

    mutating func markMilestone(_ id: String) {
        completedMilestoneIDs.insert(id)
    }

    func hasCompleted(_ milestoneID: String) -> Bool {
        completedMilestoneIDs.contains(milestoneID)
    }
}

// MARK: - NarrativeArcManager

/// Manages the five-act narrative progression of the reef.
///
/// Narrative events fire when the player crosses a milestone threshold.
/// Unread events are surfaced via `unreadCount` — the UI shows a quiet indicator,
/// never a notification. The story is discovered, not pushed.
///
/// Persistence: `UserDefaults` for the progress record; survives app restart.
final class NarrativeArcManager: ObservableObject {

    // MARK: - Published State

    @Published var currentAct: NarrativeAct = .theFirstNote
    @Published var actProgress: [NarrativeAct: ActProgress] = [:]
    @Published var recentEvents: [NarrativeEvent] = []
    @Published var unreadCount: Int = 0

    // MARK: - Milestone Library

    /// The complete library of narrative milestones across all five acts.
    let milestones: [NarrativeMilestone] = NarrativeArcManager.buildMilestoneLibrary()

    // MARK: - Storage Keys

    private let actProgressKey    = "obrix_narrative_act_progress"
    private let eventsKey         = "obrix_narrative_events"
    private let currentActKey     = "obrix_narrative_current_act"
    private let firstLaunchKey    = "obrix_narrative_first_launch_date"

    // MARK: - Init

    init() {
        // Initialize empty progress for all acts
        for act in NarrativeAct.allCases {
            actProgress[act] = ActProgress(act: act)
        }
        restore()
        recomputeUnreadCount()
    }

    // MARK: - Public API — Milestone Triggers

    /// Record that the player has played their very first note.
    func recordFirstNote() {
        fire(milestoneType: .firstNote, value: 1)
    }

    /// Record that the player has caught their very first specimen.
    func recordFirstCatch() {
        fire(milestoneType: .firstCatch, value: 1)
    }

    /// Record that the player has created their first wire.
    func recordFirstWire() {
        fire(milestoneType: .firstWire, value: 1)
    }

    /// Record total specimen count crossing a threshold.
    func recordSpecimenCount(_ count: Int) {
        fire(milestoneType: .specimenCount, value: count)
    }

    /// Record a breeding event with the resulting generation number.
    func recordBreeding(generation: Int) {
        fire(milestoneType: .breedingGeneration, value: generation)
    }

    /// Record completing a biome (all specimens of a biome discovered).
    func recordBiomeDiscovery() {
        fire(milestoneType: .biomeDiscovery, value: 1)
    }

    /// Record living through a season (called by SeasonalEventManager on season change).
    func recordSeasonExperienced(count: Int) {
        fire(milestoneType: .seasonExperienced, value: count)
    }

    /// Record a coupling type used for the first time.
    func recordCouplingType(_ type: String) {
        // Use a hash of the coupling type name as a stable int for comparison
        fire(milestoneType: .couplingType, value: abs(type.hashValue) % 1_000_000)
    }

    /// Record an arrangement being created and saved.
    func recordArrangementCreated() {
        fire(milestoneType: .arrangementCreated, value: 1)
    }

    /// Record a Dive score.
    func recordDiveScore(_ score: Int) {
        fire(milestoneType: .diveScore, value: score)
    }

    /// Record a specimen reaching Elder tier.
    func recordElderAchieved() {
        fire(milestoneType: .elderAchieved, value: 1)
    }

    /// Record a completed NFC trade.
    func recordTradeCompleted() {
        fire(milestoneType: .tradeCompleted, value: 1)
    }

    /// Record a daily streak crossing a threshold.
    func recordStreakReached(_ days: Int) {
        fire(milestoneType: .streakReached, value: days)
    }

    /// Record catching a rare specimen.
    func recordRareCaught() {
        fire(milestoneType: .rareSpecimenCaught, value: 1)
    }

    // MARK: - Public API — Reading

    /// Mark all unread narrative events as read.
    func markAllEventsRead() {
        for i in recentEvents.indices {
            recentEvents[i].hasBeenRead = true
        }
        unreadCount = 0
        saveEvents()
    }

    /// Mark a single narrative event as read by its ID.
    func markEventRead(id: UUID) {
        guard let idx = recentEvents.firstIndex(where: { $0.id == id }) else { return }
        recentEvents[idx].hasBeenRead = true
        recomputeUnreadCount()
        saveEvents()
    }

    /// All milestones belonging to the current act, with their completion status.
    var currentActMilestones: [NarrativeMilestone] {
        milestonesForAct(currentAct)
    }

    /// Fraction of required milestones completed in the current act (0–1).
    var currentActCompletionFraction: Float {
        let required = milestonesForAct(currentAct).filter { $0.isActRequired }
        guard !required.isEmpty else { return 1.0 }
        let progress = actProgress[currentAct]
        let done = required.filter { progress?.hasCompleted($0.id) ?? false }.count
        return Float(done) / Float(required.count)
    }

    /// Whether all required milestones for the current act are done and the day requirement is met.
    func canAdvanceAct(playerDays: Int) -> Bool {
        guard let next = currentAct.next else { return false }  // Already at final act
        guard playerDays >= next.minimumDays else { return false }
        return currentActCompletionFraction >= 1.0
    }

    /// Advance to the next act. Call after confirming `canAdvanceAct` returns true.
    func advanceAct(playerDays: Int) {
        guard canAdvanceAct(playerDays: playerDays), let next = currentAct.next else { return }

        // Mark current act complete
        actProgress[currentAct]?.isActComplete  = true
        actProgress[currentAct]?.completedDate  = Date()

        currentAct = next

        // Fire the act-entry narrative event
        let entryMilestone = NarrativeMilestone(
            id:                  "act_entry_\(next.rawValue)",
            act:                 next,
            type:                .firstNote,   // Sentinel type — act-entry events reuse this slot
            targetValue:         0,
            title:               next.displayName,
            narrativeText:       next.themeText,
            rewardDescription:   next.completionRewardDescription,
            isActRequired:       false
        )
        appendNarrativeEvent(NarrativeEvent(milestone: entryMilestone))
        save()
    }

    // MARK: - Internal — Milestone Firing

    /// Evaluate all milestones of the given type; fire any whose target is newly met.
    private func fire(milestoneType: NarrativeMilestoneType, value: Int) {
        let candidates = milestones.filter {
            $0.type == milestoneType &&
            $0.act <= currentAct &&
            !(actProgress[$0.act]?.hasCompleted($0.id) ?? false)
        }

        for milestone in candidates {
            guard valueMeetsTarget(value: value, milestone: milestone) else { continue }
            completeMilestone(milestone)
        }
    }

    /// True when `value` satisfies the milestone's target (exact match or exceeds it for counts).
    private func valueMeetsTarget(value: Int, milestone: NarrativeMilestone) -> Bool {
        switch milestone.type {
        case .specimenCount, .breedingGeneration, .diveScore, .streakReached:
            return value >= milestone.targetValue
        case .firstNote, .firstCatch, .firstWire, .arrangementCreated,
             .biomeDiscovery, .seasonExperienced, .elderAchieved,
             .tradeCompleted, .rareSpecimenCaught, .couplingType:
            return value >= milestone.targetValue
        }
    }

    private func completeMilestone(_ milestone: NarrativeMilestone) {
        actProgress[milestone.act]?.markMilestone(milestone.id)
        appendNarrativeEvent(NarrativeEvent(milestone: milestone))
        save()
    }

    // MARK: - Internal — Event Queue

    private func appendNarrativeEvent(_ event: NarrativeEvent) {
        recentEvents.insert(event, at: 0)        // Newest first
        if recentEvents.count > 100 {
            recentEvents = Array(recentEvents.prefix(100))  // Cap at 100 entries
        }
        recomputeUnreadCount()
    }

    private func recomputeUnreadCount() {
        unreadCount = recentEvents.filter { !$0.hasBeenRead }.count
    }

    // MARK: - Milestone Library (Static Builder)

    static func buildMilestoneLibrary() -> [NarrativeMilestone] {
        var list: [NarrativeMilestone] = []

        // MARK: Act 1 — The First Note (Day 1–7)

        list.append(NarrativeMilestone(
            id: "a1_first_note",
            act: .theFirstNote,
            type: .firstNote,
            targetValue: 1,
            title: "The Ocean Hears You",
            narrativeText: "The note left your hands and entered the water. The reef went still for a moment — then answered. Something remembered you were here.",
            rewardDescription: "The reef's ambient sound layer becomes active.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a1_first_catch",
            act: .theFirstNote,
            type: .firstCatch,
            targetValue: 1,
            title: "A Voice Comes Home",
            narrativeText: "It struggled at first. New voices always do. But the reef is patient, and so you must be. It settled, and you heard what it was.",
            rewardDescription: "The specimen's synth voice activates on the PlaySurface.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a1_first_wire",
            act: .theFirstNote,
            type: .firstWire,
            targetValue: 1,
            title: "The First Connection",
            narrativeText: "Wire is the wrong word for it. It is more like an introduction — two voices noticing each other for the first time, deciding whether to listen.",
            rewardDescription: "Coupling chemistry becomes visible on the reef map.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a1_three_specimens",
            act: .theFirstNote,
            type: .specimenCount,
            targetValue: 3,
            title: "Three Voices",
            narrativeText: "Three is the smallest number that makes a world. Below three, it is call and response. At three, something shifts — the harmony becomes its own thing.",
            rewardDescription: "The three-specimen wiring chain becomes available.",
            isActRequired: false
        ))

        // MARK: Act 2 — The Growing Reef (Day 8–30)

        list.append(NarrativeMilestone(
            id: "a2_ten_specimens",
            act: .theGrowingReef,
            type: .specimenCount,
            targetValue: 10,
            title: "Ten Voices",
            narrativeText: "You stopped counting at some point. The reef took over — new voices arriving, settling, learning the others. What you built is no longer just yours to control.",
            rewardDescription: "The Reef Overview map unlocks a secondary view layer.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a2_first_breeding",
            act: .theGrowingReef,
            type: .breedingGeneration,
            targetValue: 2,
            title: "Something New",
            narrativeText: "It carries both of them but sounds like neither. That is the point. Generation two is not an improvement — it is a question the reef is asking itself.",
            rewardDescription: "The Nursery's second slot unlocks.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a2_first_season",
            act: .theGrowingReef,
            type: .seasonExperienced,
            targetValue: 1,
            title: "The Reef Turns",
            narrativeText: "The water changed before anything else did. A different temperature, a different light. The specimens noticed before you did. They always do.",
            rewardDescription: "Seasonal exclusive specimens begin appearing.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a2_first_dive",
            act: .theGrowingReef,
            type: .diveScore,
            targetValue: 1,
            title: "Into the Deep",
            narrativeText: "You took them with you for the first time. They played differently down there — the pressure changes everything. They sounded older, somehow.",
            rewardDescription: "Dive depth increases to Twilight Zone.",
            isActRequired: false
        ))

        list.append(NarrativeMilestone(
            id: "a2_twenty_specimens",
            act: .theGrowingReef,
            type: .specimenCount,
            targetValue: 20,
            title: "A Chorus",
            narrativeText: "Twenty is not a number — it is a density. Past this threshold the reef stops being a collection of specimens and starts being an ecosystem. You can feel the difference.",
            rewardDescription: "The reef ambient layer gains a second voice of its own.",
            isActRequired: false
        ))

        // MARK: Act 3 — The Deep Call (Day 31–90)

        list.append(NarrativeMilestone(
            id: "a3_all_biomes",
            act: .theDeepCall,
            type: .biomeDiscovery,
            targetValue: 4,
            title: "All the Zones",
            narrativeText: "Each zone has a different silence. That is the true discovery — not the specimens but the quiet between them, each zone's version of nothing.",
            rewardDescription: "Depth-zone-specific rare specimens begin appearing.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a3_first_rare",
            act: .theDeepCall,
            type: .rareSpecimenCaught,
            targetValue: 1,
            title: "The Rare One",
            narrativeText: "There was no warning. Just suddenly it was there — a voice so specific you would know it anywhere. You understood why rarity exists. Some sounds cannot be common.",
            rewardDescription: "Rare specimens gain a secondary coupling output.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a3_gen3_breeding",
            act: .theDeepCall,
            type: .breedingGeneration,
            targetValue: 3,
            title: "Third Generation",
            narrativeText: "By the third generation, the family resemblance is abstract. You can trace it if you listen carefully — a preference for certain intervals, a timing habit, a color in the low frequencies.",
            rewardDescription: "Generation-3+ specimens can accept three concurrent wire connections.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a3_dive_500",
            act: .theDeepCall,
            type: .diveScore,
            targetValue: 500,
            title: "Depth Memory",
            narrativeText: "The deep does not forget what passes through it. Your specimens carry a record of this dive — not in data, but in the way they respond to certain notes now.",
            rewardDescription: "Dive replay unlocks: you can re-enter a past dive as ambient background.",
            isActRequired: false
        ))

        list.append(NarrativeMilestone(
            id: "a3_seven_day_streak",
            act: .theDeepCall,
            type: .streakReached,
            targetValue: 7,
            title: "Seven Days",
            narrativeText: "Seven days is enough to establish a rhythm. The reef has learned when to expect you. The ambient voice is different now — tuned to your return.",
            rewardDescription: "The reef plays a unique welcome tone when you return each day.",
            isActRequired: false
        ))

        // MARK: Act 4 — The Living Song (Day 91–180)

        list.append(NarrativeMilestone(
            id: "a4_fifty_specimens",
            act: .theLivingSong,
            type: .specimenCount,
            targetValue: 50,
            title: "Fifty Voices",
            narrativeText: "Fifty is past the point of curation. You are not choosing anymore — you are tending. The reef makes its own decisions about who lives near whom. Trust it.",
            rewardDescription: "The Mass Arrangement mode activates: all 50 specimens can play together.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a4_elder_achieved",
            act: .theLivingSong,
            type: .elderAchieved,
            targetValue: 1,
            title: "An Elder Speaks",
            narrativeText: "Age changed it. Not degraded — deepened. The elder carries the memory of every note it has ever played. You can hear it in the decay of each sound: the weight of history.",
            rewardDescription: "Elder specimens unlock a fourth wire connection and a unique coupling type.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a4_first_arrangement",
            act: .theLivingSong,
            type: .arrangementCreated,
            targetValue: 1,
            title: "The Arrangement",
            narrativeText: "You made something that will outlast the session. The voices are locked into a shape — not frozen, still alive, but arranged. The reef learned to hold still long enough to be heard.",
            rewardDescription: "Arrangements can be exported as audio recordings.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a4_all_seasons",
            act: .theLivingSong,
            type: .seasonExperienced,
            targetValue: 4,
            title: "A Full Year",
            narrativeText: "The fourth season closed, and the reef had been through all of them. You understood then that the reef has a year, the way a person does — patterns that repeat, but are never quite the same.",
            rewardDescription: "The seasonal archive unlocks: past season palettes can be applied to the reef.",
            isActRequired: false
        ))

        list.append(NarrativeMilestone(
            id: "a4_trade_completed",
            act: .theLivingSong,
            type: .tradeCompleted,
            targetValue: 1,
            title: "An Exchange",
            narrativeText: "Someone else made something. You made something. For a moment, the two reefs were connected. A voice you have never heard before arrived, and yours went somewhere you will never see.",
            rewardDescription: "Traded specimens gain a unique 'exchange' marking visible in the Microscope.",
            isActRequired: false
        ))

        // MARK: Act 5 — The Endless Ocean (Day 181+)

        list.append(NarrativeMilestone(
            id: "a5_gen10_breeding",
            act: .theEndlessOcean,
            type: .breedingGeneration,
            targetValue: 10,
            title: "The Tenth Generation",
            narrativeText: "Ten generations back, there was a first voice. You can still hear it — distilled, filtered through nine iterations of choice. What remains of the original is what was most essential.",
            rewardDescription: "Generation-10 specimens unlock a unique synth voice unavailable any other way.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a5_thirty_day_streak",
            act: .theEndlessOcean,
            type: .streakReached,
            targetValue: 30,
            title: "Thirty Days",
            narrativeText: "A month. The reef has had a month of you. It no longer sounds the same in your absence — you can tell when you return, by the quality of the waiting.",
            rewardDescription: "The reef generates a daily tonal greeting unique to your collection.",
            isActRequired: false
        ))

        list.append(NarrativeMilestone(
            id: "a5_dive_score_1000",
            act: .theEndlessOcean,
            type: .diveScore,
            targetValue: 1000,
            title: "A Thousand Fathoms",
            narrativeText: "There is no bottom. You understood this at a thousand fathoms — not as a disappointment but as a relief. The ocean does not end. Neither does the listening.",
            rewardDescription: "The abyssal dive tier unlocks: legendary specimens are encountered here.",
            isActRequired: true
        ))

        list.append(NarrativeMilestone(
            id: "a5_full_biome_complete",
            act: .theEndlessOcean,
            type: .biomeDiscovery,
            targetValue: 6,
            title: "The Whole Map",
            narrativeText: "You have named every zone, heard every silence. The map is complete. And yet — the ocean keeps offering something unmapped. It always will.",
            rewardDescription: "The complete collection badge unlocks. The reef displays your full history.",
            isActRequired: true
        ))

        return list
    }

    // MARK: - Persistence

    func save() {
        saveActProgress()
        saveEvents()
        UserDefaults.standard.set(currentAct.rawValue, forKey: currentActKey)
    }

    func restore() {
        restoreActProgress()
        restoreEvents()
        if let rawAct = UserDefaults.standard.value(forKey: currentActKey) as? Int,
           let act = NarrativeAct(rawValue: rawAct) {
            currentAct = act
        }
    }

    private func saveActProgress() {
        let encoder = JSONEncoder()
        let values  = Array(actProgress.values)
        if let data = try? encoder.encode(values) {
            UserDefaults.standard.set(data, forKey: actProgressKey)
        }
    }

    private func restoreActProgress() {
        guard let data    = UserDefaults.standard.data(forKey: actProgressKey),
              let records = try? JSONDecoder().decode([ActProgress].self, from: data) else { return }
        for record in records {
            actProgress[record.act] = record
        }
    }

    private func saveEvents() {
        let encoder = JSONEncoder()
        if let data = try? encoder.encode(recentEvents) {
            UserDefaults.standard.set(data, forKey: eventsKey)
        }
    }

    private func restoreEvents() {
        guard let data   = UserDefaults.standard.data(forKey: eventsKey),
              let events = try? JSONDecoder().decode([NarrativeEvent].self, from: data) else { return }
        recentEvents = events
    }

    // MARK: - Helpers

    private func milestonesForAct(_ act: NarrativeAct) -> [NarrativeMilestone] {
        milestones.filter { $0.act == act }
    }
}
