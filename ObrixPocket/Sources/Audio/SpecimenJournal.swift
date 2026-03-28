import Foundation

/// A single event in a specimen's life
struct JournalEntry: Codable, Identifiable {
    let id: UUID
    let timestamp: Date
    let type: JournalEventType
    let description: String

    enum JournalEventType: String, Codable {
        case born      // Specimen created (catch or music hash)
        case wired     // Connected to another specimen
        case unwired   // Disconnected from another specimen
        case levelUp   // Reached a new level
        case evolved   // Evolved at level 10
        case drifted   // Spectral DNA shifted from coupling
        case played    // Milestone play time reached (1h, 10h, 100h)
        case traded    // Traded to/from another player (future)
    }
}
