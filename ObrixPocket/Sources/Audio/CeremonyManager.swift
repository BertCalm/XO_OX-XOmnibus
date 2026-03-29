import Foundation

/// Tracks and triggers one-time ceremonial moments in the reef's life.
/// Each ceremony fires once, is remembered, and produces a CeremonyEvent
/// that the UI layer consumes for animation/audio feedback.
///
/// Season-change ceremonies are the exception — they fire once per transition
/// (they can repeat each year), but all other ceremony types are one-time-only.
final class CeremonyManager: ObservableObject {

    // MARK: - Ceremony Types

    enum CeremonyType: String, Codable, CaseIterable {
        case offspringFirstNote    // Bred specimen placed on reef for the first time
        case elderTransition       // Specimen reaches Elder stage (60 days)
        case ancientAwakening      // Specimen reaches Ancient stage (180 days)
        case seasonChange          // Season transitions (repeatable)
        case firstBreeding         // First successful breeding
        case firstDive             // First dive completed
        case reefMilestone10       // 10 specimens on reef
        case reefMilestone25       // 25 specimens
        case hundredDays           // Reef is 100 days old
    }

    struct CeremonyEvent: Codable, Identifiable {
        let id: UUID
        let type: CeremonyType
        let timestamp: Date
        let specimenName: String?  // If ceremony is about a specific specimen
        let narrativeText: String  // Poetic text displayed during ceremony
        var hasBeenSeen: Bool = false
    }

    // MARK: - State

    @Published var pendingCeremonies: [CeremonyEvent] = []
    /// rawValues of CeremonyTypes that have been triggered (one-time gate)
    @Published var completedCeremonyTypes: Set<String> = []

    // MARK: - Trigger Methods

    /// Call when a bred specimen is placed on the reef for the first time
    func onOffspringPlaced(specimenName: String) {
        triggerOnce(.offspringFirstNote, specimenName: specimenName,
            text: "A new voice enters the reef. It carries the memory of its parents — and something entirely its own. Listen.")
    }

    /// Call when a specimen transitions to Elder (60 days)
    func onElderTransition(specimenName: String) {
        triggerOnce(.elderTransition, specimenName: specimenName,
            text: "\(specimenName) has become an Elder. Sixty days of listening have deepened its voice. It remembers everything you have played.")
    }

    /// Call when a specimen reaches the Ancient threshold (180 days).
    /// Note: SpecimenAging uses four stages (Spawn/Juvenile/Adult/Elder).
    /// Ancient is a ceremony milestone beyond Elder — specimens 180+ days old.
    func onAncientAwakening(specimenName: String) {
        triggerOnce(.ancientAwakening, specimenName: specimenName,
            text: "\(specimenName) has passed beyond Elder into Ancient. Its voice is unlike any other in the ocean. It has become the reef itself.")
    }

    /// Call on each season transition.
    /// Season changes can repeat (they fire each year), so this does NOT use triggerOnce.
    func onSeasonChange(from: String, to: String) {
        let event = CeremonyEvent(
            id: UUID(),
            type: .seasonChange,
            timestamp: Date(),
            specimenName: nil,
            narrativeText: seasonTransitionText(from: from, to: to)
        )
        pendingCeremonies.append(event)
        save()
    }

    /// Call on first successful breeding
    func onFirstBreeding(offspringName: String) {
        triggerOnce(.firstBreeding, specimenName: offspringName,
            text: "Life creates life. Two voices have produced a third. The reef is no longer just a collection — it is a family.")
    }

    /// Call when the player completes their first dive
    func onFirstDive() {
        triggerOnce(.firstDive, specimenName: nil,
            text: "The surface breaks. Below, the water hums with frequencies you have never heard. Your specimens descend. The Dive begins.")
    }

    /// Call whenever the reef specimen count crosses a milestone.
    /// Checks both the 10 and 25 thresholds — safe to call on every count change.
    func onReefMilestone(_ count: Int) {
        if count >= 10 {
            triggerOnce(.reefMilestone10, specimenName: nil,
                text: "Ten voices. The reef is beginning to sound like something no one has heard before. It is beginning to sound like you.")
        }
        if count >= 25 {
            triggerOnce(.reefMilestone25, specimenName: nil,
                text: "Twenty-five voices. The reef has developed its own acoustic signature — reverberant, complex, alive. Other reefs would recognize yours by sound alone.")
        }
    }

    /// Call when the reef reaches day 100
    func onHundredDays() {
        triggerOnce(.hundredDays, specimenName: nil,
            text: "One hundred days. The reef remembers every note you have ever played. Every specimen carries the echo of your music in its voice. The ocean is listening.")
    }

    // MARK: - Dismiss

    /// Mark a ceremony as seen. The UI calls this after the ceremony animation/display completes.
    func dismissCeremony(_ id: UUID) {
        if let index = pendingCeremonies.firstIndex(where: { $0.id == id }) {
            pendingCeremonies[index].hasBeenSeen = true
        }
        save()
    }

    // MARK: - Private

    private func triggerOnce(_ type: CeremonyType, specimenName: String?, text: String) {
        guard !completedCeremonyTypes.contains(type.rawValue) else { return }
        completedCeremonyTypes.insert(type.rawValue)
        let event = CeremonyEvent(
            id: UUID(),
            type: type,
            timestamp: Date(),
            specimenName: specimenName,
            narrativeText: text
        )
        pendingCeremonies.append(event)
        save()
    }

    private func seasonTransitionText(from: String, to: String) -> String {
        switch to {
        case "springBloom":
            return "The water warms. Currents shift. New specimens stir in the shallows. Spring Bloom has arrived — the reef is waking up."
        case "summerSurge":
            return "The light reaches deeper now. Every specimen feels it. The reef is at its brightest, its loudest, its most alive. Summer Surge."
        case "autumnDrift":
            return "The first cold current arrives. Some specimens settle deeper. The reef grows quieter, more reflective. Autumn Drift begins."
        case "winterDeep":
            return "Darkness. The surface light disappears. But below — below, the deeper frequencies emerge. Sounds you have never heard before. Winter Deep."
        default:
            return "The season has changed. The reef adjusts. New sounds emerge."
        }
    }

    // MARK: - Persistence

    private let pendingKey   = "obrix_pendingCeremonies"
    private let completedKey = "obrix_completedCeremonyTypes"

    func save() {
        if let data = try? JSONEncoder().encode(pendingCeremonies) {
            UserDefaults.standard.set(data, forKey: pendingKey)
        }
        if let data = try? JSONEncoder().encode(Array(completedCeremonyTypes)) {
            UserDefaults.standard.set(data, forKey: completedKey)
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: pendingKey),
           let decoded = try? JSONDecoder().decode([CeremonyEvent].self, from: data) {
            pendingCeremonies = decoded
        }
        if let data = UserDefaults.standard.data(forKey: completedKey),
           let decoded = try? JSONDecoder().decode([String].self, from: data) {
            completedCeremonyTypes = Set(decoded)
        }
    }

    init() { restore() }
}
