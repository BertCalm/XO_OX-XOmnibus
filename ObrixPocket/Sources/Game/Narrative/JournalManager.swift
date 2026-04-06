import Foundation
import Combine

/// Cross-specimen journal query layer.
///
/// The journal is the append-only biography embedded on each `Specimen` and persisted
/// through GRDB via `SpecimenRecord`. `JournalManager` does not maintain a separate
/// copy of that data — it reads from `ReefStore.specimens` (the live source of truth)
/// and provides conveniences for multi-specimen queries, fleet-wide recent events, and
/// consistent entry construction.
///
/// Usage — add an entry to a reef specimen:
/// ```swift
/// JournalManager.shared.addEntry(
///     to: slotIndex,
///     in: reefStore,
///     type: .wired,
///     description: "Connected to Curtain"
/// )
/// ```
final class JournalManager: ObservableObject {
    static let shared = JournalManager()

    private init() {}

    // MARK: - Writing

    /// Append a journal event to the specimen in the given reef slot.
    /// No-op if the slot is empty.  Mutates `reefStore.specimens` in-place
    /// so callers should follow up with `reefStore.save()` when appropriate.
    func addEntry(
        to slotIndex: Int,
        in reefStore: ReefStore,
        type: JournalEntry.JournalEventType,
        description: String
    ) {
        reefStore.addJournalEntry(to: slotIndex, type: type, description: description)
    }

    /// Convenience: append a journal event directly to a `Specimen` value.
    /// The caller is responsible for writing the mutated specimen back to the store.
    func makeEntry(
        specimenId: UUID,
        type: JournalEntry.JournalEventType,
        description: String
    ) -> JournalEntry {
        JournalEntry(id: UUID(), timestamp: Date(), type: type, description: description)
    }

    // MARK: - Reading — single specimen

    /// All journal entries for a specific specimen, newest first.
    func entries(for specimen: Specimen) -> [JournalEntry] {
        specimen.journal.reversed()
    }

    /// Journal entries for a specific specimen, newest first, capped at `limit`.
    func recentEntries(for specimen: Specimen, limit: Int = 10) -> [JournalEntry] {
        Array(specimen.journal.suffix(limit).reversed())
    }

    /// Filter entries for a specimen to a specific event type.
    func entries(for specimen: Specimen, type: JournalEntry.JournalEventType) -> [JournalEntry] {
        specimen.journal.filter { $0.type == type }.reversed()
    }

    // MARK: - Reading — fleet wide

    /// All journal entries across every occupied reef slot, newest first, capped at `limit`.
    /// Useful for a global "recent activity" feed.
    func recentFleetEntries(in reefStore: ReefStore, limit: Int = 20) -> [(specimen: Specimen, entry: JournalEntry)] {
        var pairs: [(specimen: Specimen, entry: JournalEntry)] = []
        for specimen in reefStore.specimens.compactMap({ $0 }) {
            for entry in specimen.journal {
                pairs.append((specimen, entry))
            }
        }
        // Sort newest first, truncate
        pairs.sort { $0.entry.timestamp > $1.entry.timestamp }
        return Array(pairs.prefix(limit))
    }

    /// Total number of journal entries across all reef specimens.
    func totalEntryCount(in reefStore: ReefStore) -> Int {
        reefStore.specimens.compactMap { $0 }.reduce(0) { $0 + $1.journal.count }
    }

    // MARK: - Event Type Metadata

    /// SF Symbol name for a given event type (mirrors MicroscopeView logic as the single source).
    static func icon(for type: JournalEntry.JournalEventType) -> String {
        switch type {
        case .born:    return "sparkle"
        case .wired:   return "link"
        case .unwired: return "link.badge.plus"
        case .levelUp: return "arrow.up.circle"
        case .evolved: return "sparkles"
        case .drifted: return "waveform.path.ecg"
        case .played:  return "music.note"
        case .traded:  return "arrow.left.arrow.right"
        }
    }

    /// Human-readable label for a given event type (used in filter chips).
    static func label(for type: JournalEntry.JournalEventType) -> String {
        switch type {
        case .born:    return "Born"
        case .wired:   return "Wired"
        case .unwired: return "Unwired"
        case .levelUp: return "Level Up"
        case .evolved: return "Evolved"
        case .drifted: return "Drifted"
        case .played:  return "Played"
        case .traded:  return "Traded"
        }
    }
}
