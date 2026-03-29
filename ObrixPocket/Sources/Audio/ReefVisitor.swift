import Foundation

// MARK: - Sprint 89: Reef Visiting
//
// Read-only window into other players' reefs.
// No central server — social layer is purely peer-to-peer via
// JSON cards shared through AirDrop, Messages, or clipboard.

// MARK: - Supporting Types

/// Lightweight specimen snapshot carried inside a ReefCard.
/// Does not include full DSP state — observation only.
struct SpecimenSummary: Codable {
    let subtypeId: String
    let displayName: String
    let generation: Int
    let level: Int
    let ageInDays: Int
    let isFavorite: Bool
    let hasBeenBred: Bool
}

/// Aggregate statistics about a reef at the moment the card was created.
struct ReefStats: Codable {
    let totalSpecimens: Int
    let daysActive: Int
    let divesCompleted: Int
    let arrangementsCreated: Int
    let maxBreedingGeneration: Int
}

/// Minimal acoustic flavour — no DSP state, just ambient character.
struct AcousticProfile: Codable {
    let reverbTime: Float    // 0.0–10.0 seconds
    let roomSize: Float      // 0.0–1.0 (small pocket → vast ocean)
    let ambience: Float      // 0.0–1.0 (silent tide → living chorus)
}

// MARK: - ReefCard

/// A shareable snapshot of a player's reef — the social business card.
///
/// Purposefully lightweight: it captures the *personality* of a reef
/// (what species live there, how long they've been around, what the
/// acoustic character feels like) without exposing raw DSP parameters
/// or enabling reconstruction. It's meant to be observed, not cloned.
struct ReefCard: Codable, Identifiable {

    // MARK: Identity
    let cardId: String          // 8 alphanumeric chars, unique to this export
    let playerName: String      // Display name, max 20 chars
    let reefName: String        // Custom reef name, max 30 chars
    let createdDate: Date

    // MARK: Content
    let specimenSummaries: [SpecimenSummary]
    let reefStats: ReefStats
    let acousticProfile: AcousticProfile
    let season: String                      // Season when card was created
    let featuredSpecimen: SpecimenSummary?  // Player's chosen showcase

    // MARK: Local bookkeeping (not serialised from sender — set on receipt)
    var isStarred: Bool = false             // Receiver's favourite flag
    var receivedDate: Date = Date()         // When this card landed locally

    // MARK: Identifiable conformance
    var id: String { cardId }

    // MARK: Derived helpers

    /// True when the card is older than 30 days and the reef may have changed.
    var isOutdated: Bool {
        Date().timeIntervalSince(createdDate) > 30 * 24 * 3600
    }

    /// Display-ready age string: "3 days ago", "1 month ago", etc.
    var ageDescription: String {
        let seconds = Date().timeIntervalSince(createdDate)
        switch seconds {
        case ..<120:        return "just now"
        case ..<3600:       return "\(Int(seconds / 60)) minutes ago"
        case ..<86400:      return "\(Int(seconds / 3600)) hours ago"
        case ..<2592000:    return "\(Int(seconds / 86400)) days ago"
        default:            return "\(Int(seconds / 2592000)) months ago"
        }
    }

    // MARK: CodingKeys — exclude local bookkeeping from the shared payload

    enum CodingKeys: String, CodingKey {
        case cardId, playerName, reefName, createdDate
        case specimenSummaries, reefStats, acousticProfile
        case season, featuredSpecimen
        // isStarred and receivedDate are NOT encoded — they are set locally
    }

    init(
        cardId: String,
        playerName: String,
        reefName: String,
        createdDate: Date,
        specimenSummaries: [SpecimenSummary],
        reefStats: ReefStats,
        acousticProfile: AcousticProfile,
        season: String,
        featuredSpecimen: SpecimenSummary?
    ) {
        self.cardId = cardId
        self.playerName = playerName
        self.reefName = reefName
        self.createdDate = createdDate
        self.specimenSummaries = specimenSummaries
        self.reefStats = reefStats
        self.acousticProfile = acousticProfile
        self.season = season
        self.featuredSpecimen = featuredSpecimen
    }

    init(from decoder: Decoder) throws {
        let c = try decoder.container(keyedBy: CodingKeys.self)
        cardId          = try c.decode(String.self,                  forKey: .cardId)
        playerName      = try c.decode(String.self,                  forKey: .playerName)
        reefName        = try c.decode(String.self,                  forKey: .reefName)
        createdDate     = try c.decode(Date.self,                    forKey: .createdDate)
        specimenSummaries = try c.decode([SpecimenSummary].self,     forKey: .specimenSummaries)
        reefStats       = try c.decode(ReefStats.self,               forKey: .reefStats)
        acousticProfile = try c.decode(AcousticProfile.self,         forKey: .acousticProfile)
        season          = try c.decode(String.self,                  forKey: .season)
        featuredSpecimen = try c.decodeIfPresent(SpecimenSummary.self, forKey: .featuredSpecimen)
        // Local-only fields — set to defaults on import
        isStarred    = false
        receivedDate = Date()
    }

    func encode(to encoder: Encoder) throws {
        var c = encoder.container(keyedBy: CodingKeys.self)
        try c.encode(cardId,           forKey: .cardId)
        try c.encode(playerName,       forKey: .playerName)
        try c.encode(reefName,         forKey: .reefName)
        try c.encode(createdDate,      forKey: .createdDate)
        try c.encode(specimenSummaries, forKey: .specimenSummaries)
        try c.encode(reefStats,        forKey: .reefStats)
        try c.encode(acousticProfile,  forKey: .acousticProfile)
        try c.encode(season,           forKey: .season)
        try c.encodeIfPresent(featuredSpecimen, forKey: .featuredSpecimen)
    }
}

// MARK: - Stats Comparison

/// Side-by-side comparison between the local reef and a visited card.
struct ReefCardComparison {
    let visited: ReefCard
    let mySpecimenCount: Int
    let myDaysActive: Int
    let myDivesCompleted: Int

    var specimenDifference: Int { mySpecimenCount - visited.reefStats.totalSpecimens }
    var daysActiveDifference: Int { myDaysActive - visited.reefStats.daysActive }
    var divesDifference: Int { myDivesCompleted - visited.reefStats.divesCompleted }

    /// Unique subtypes in the visited reef that the local player does not have.
    var subtypesTheyHaveThatYouDont: [String] {
        let mySubtypes = Set<String>() // Caller supplies their own list if needed
        return visited.specimenSummaries.map(\.subtypeId).filter { !mySubtypes.contains($0) }
    }

    /// One-line human-readable summary.
    var summaryLine: String {
        var parts: [String] = []
        if specimenDifference > 0 {
            parts.append("You have \(specimenDifference) more specimens")
        } else if specimenDifference < 0 {
            parts.append("They have \(abs(specimenDifference)) more specimens")
        } else {
            parts.append("Same specimen count")
        }
        if divesDifference > 0 {
            parts.append("\(divesDifference) more dives")
        } else if divesDifference < 0 {
            parts.append("\(abs(divesDifference)) fewer dives")
        }
        return parts.joined(separator: " · ")
    }
}

// MARK: - ReefVisitorManager

/// Manages the social "reef visiting" feature — purely read-only, purely P2P.
///
/// Players share ReefCards through system share sheets (AirDrop, Messages,
/// clipboard). Cards are stored locally in UserDefaults (max 50).
/// No network calls are ever made.
final class ReefVisitorManager: ObservableObject {

    // MARK: - Published State

    @Published var visitedReefs: [ReefCard] = []       // History, newest first
    @Published var myReefCard: ReefCard? = nil          // This player's own card

    // MARK: - Constants

    static let maxVisitedHistory = 50
    private let visitedStorageKey = "obrix_visited_reef_cards"
    private let myCardStorageKey  = "obrix_my_reef_card"

    // MARK: - Init

    init() {
        restoreVisited()
        restoreMyCard()
    }

    // MARK: - My Card

    /// Build and publish a new card from current reef state.
    /// Call this whenever the player wants to refresh their shareable card.
    @discardableResult
    func generateMyCard(
        playerName: String,
        reefStore: ReefStore,
        divesCompleted: Int,
        arrangementsCreated: Int,
        reefCreatedDate: Date,
        overrideAcousticProfile: AcousticProfile? = nil,
        featuredSpecimenId: UUID? = nil
    ) -> ReefCard {
        let allSpecimens = reefStore.specimens.compactMap { $0 }

        let summaries = allSpecimens.map { spec in
            SpecimenSummary(
                subtypeId: spec.subtype,
                displayName: spec.name,
                generation: spec.journal.filter { $0.type == .born }.count,
                level: spec.level,
                ageInDays: Int(Date().timeIntervalSince(spec.catchTimestamp) / 86400),
                isFavorite: spec.isFavorite,
                hasBeenBred: spec.journal.contains { $0.type == .born && $0.description.contains("Bred") }
            )
        }

        let maxGen = summaries.map(\.generation).max() ?? 0
        let daysActive = Int(Date().timeIntervalSince(reefCreatedDate) / 86400)

        let stats = ReefStats(
            totalSpecimens: allSpecimens.count,
            daysActive: max(daysActive, 0),
            divesCompleted: divesCompleted,
            arrangementsCreated: arrangementsCreated,
            maxBreedingGeneration: maxGen
        )

        let acoustic = overrideAcousticProfile ?? AcousticProfile(
            reverbTime: 2.4,
            roomSize: 0.5,
            ambience: 0.5
        )

        let featured: SpecimenSummary?
        if let featId = featuredSpecimenId,
           let spec = allSpecimens.first(where: { $0.id == featId }) {
            featured = SpecimenSummary(
                subtypeId: spec.subtype,
                displayName: spec.name,
                generation: spec.journal.filter { $0.type == .born }.count,
                level: spec.level,
                ageInDays: Int(Date().timeIntervalSince(spec.catchTimestamp) / 86400),
                isFavorite: spec.isFavorite,
                hasBeenBred: spec.journal.contains { $0.type == .born && $0.description.contains("Bred") }
            )
        } else {
            // Default: highest-level specimen in reef
            featured = summaries.max(by: { $0.level < $1.level })
        }

        let truncatedPlayerName = String(playerName.prefix(20))
        let truncatedReefName   = String(reefStore.reefName.prefix(30))

        let card = ReefCard(
            cardId: Self.generateCardId(),
            playerName: truncatedPlayerName,
            reefName: truncatedReefName,
            createdDate: Date(),
            specimenSummaries: summaries,
            reefStats: stats,
            acousticProfile: acoustic,
            season: Self.currentSeason(),
            featuredSpecimen: featured
        )

        myReefCard = card
        saveMyCard(card)
        return card
    }

    // MARK: - Export / Import

    /// Serialize a ReefCard to JSON Data for sharing.
    func exportCardAsJSON(_ card: ReefCard) -> Data? {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        encoder.dateEncodingStrategy = .iso8601
        return try? encoder.encode(card)
    }

    /// Deserialize a received ReefCard from JSON Data.
    /// Returns nil on decode failure — caller should show an error toast.
    func importCard(from data: Data) -> ReefCard? {
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        guard let card = try? decoder.decode(ReefCard.self, from: data) else {
            return nil
        }
        return card
    }

    /// Record a successfully imported card into the visit history.
    /// Deduplicates by cardId — importing the same card twice is a no-op
    /// except to refresh receivedDate.
    func recordVisit(_ card: ReefCard) {
        var updated = card
        updated.receivedDate = Date()

        if let existing = visitedReefs.firstIndex(where: { $0.cardId == card.cardId }) {
            // Refresh — bring to front
            visitedReefs.remove(at: existing)
        }
        visitedReefs.insert(updated, at: 0)

        // Enforce history cap
        if visitedReefs.count > Self.maxVisitedHistory {
            visitedReefs = Array(visitedReefs.prefix(Self.maxVisitedHistory))
        }

        saveVisited()
    }

    // MARK: - Starring (Favourites)

    func toggleStar(cardId: String) {
        guard let idx = visitedReefs.firstIndex(where: { $0.cardId == cardId }) else { return }
        visitedReefs[idx].isStarred.toggle()
        saveVisited()
    }

    var starredReefs: [ReefCard] {
        visitedReefs.filter(\.isStarred)
    }

    // MARK: - Outdated Filtering

    /// Visited reefs that still appear current (< 30 days old).
    var currentReefs: [ReefCard] {
        visitedReefs.filter { !$0.isOutdated }
    }

    /// Visited reefs older than 30 days (may no longer reflect actual reef).
    var outdatedReefs: [ReefCard] {
        visitedReefs.filter(\.isOutdated)
    }

    // MARK: - Stats Comparison

    /// Compare the local reef against a visited card.
    func compare(
        _ card: ReefCard,
        mySpecimenCount: Int,
        myDaysActive: Int,
        myDivesCompleted: Int
    ) -> ReefCardComparison {
        ReefCardComparison(
            visited: card,
            mySpecimenCount: mySpecimenCount,
            myDaysActive: myDaysActive,
            myDivesCompleted: myDivesCompleted
        )
    }

    // MARK: - Deletion

    func removeVisited(cardId: String) {
        visitedReefs.removeAll { $0.cardId == cardId }
        saveVisited()
    }

    func clearAllVisited() {
        visitedReefs.removeAll()
        saveVisited()
    }

    // MARK: - Persistence

    private func saveVisited() {
        let encoder = JSONEncoder()
        encoder.dateEncodingStrategy = .iso8601
        if let data = try? encoder.encode(visitedReefs) {
            UserDefaults.standard.set(data, forKey: visitedStorageKey)
        }
    }

    private func restoreVisited() {
        guard let data = UserDefaults.standard.data(forKey: visitedStorageKey) else { return }
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        if let decoded = try? decoder.decode([ReefCard].self, from: data) {
            visitedReefs = decoded
        }
    }

    private func saveMyCard(_ card: ReefCard) {
        let encoder = JSONEncoder()
        encoder.dateEncodingStrategy = .iso8601
        if let data = try? encoder.encode(card) {
            UserDefaults.standard.set(data, forKey: myCardStorageKey)
        }
    }

    private func restoreMyCard() {
        guard let data = UserDefaults.standard.data(forKey: myCardStorageKey) else { return }
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        myReefCard = try? decoder.decode(ReefCard.self, from: data)
    }

    // MARK: - Private Helpers

    /// 8-character alphanumeric ID unique to each card export.
    private static func generateCardId() -> String {
        let chars = "abcdefghijklmnopqrstuvwxyz0123456789"
        return String((0..<8).compactMap { _ in chars.randomElement() })
    }

    private static func currentSeason() -> String {
        let month = Calendar.current.component(.month, from: Date())
        switch month {
        case 3...5:  return "Spring"
        case 6...8:  return "Summer"
        case 9...11: return "Autumn"
        default:     return "Winter"
        }
    }
}
