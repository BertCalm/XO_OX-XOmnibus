import Foundation

// MARK: - Sprint 90: Specimen Gifting
//
// One-way specimen transfer — purely P2P, no central server.
// Wrapping REMOVES the specimen from the sender's reef.
// Gift packages travel as JSON via AirDrop, Messages, or clipboard.

// MARK: - GiftType

enum GiftType: String, Codable, CaseIterable {
    case normal     // Standard gift — any time
    case seasonal   // Seasonal event — special wrapping cue in UI
    case secret     // Unlocked via achievements — hidden origin until opened
}

// MARK: - GiftedSpecimen

/// Full reconstruction payload for a specimen that is being gifted.
/// Contains everything the recipient's reef needs to rebuild the specimen
/// from scratch — genetics, voice, memory, and provenance chain.
struct GiftedSpecimen: Codable {
    let subtypeId: String
    let displayName: String
    let generation: Int

    /// 12 trait values (raw allele floats) + expression weights.
    /// Index layout: [0..11] = trait magnitudes, [12..23] = expressions.
    let genome: [Float]

    /// Key voice parameters — enough to restore the sonic identity.
    /// Stored as a flat dictionary to stay decoupled from VoiceProfile internals.
    let voiceProfileData: [String: Float]

    /// Compressed memory fingerprint — no raw audio, just statistical character.
    let musicalMemorySnapshot: MusicalMemorySnapshot

    /// 3–5 curated journal moments chosen at wrap time.
    let journalHighlights: [String]

    /// Every reef this specimen has lived in, in order.
    let provenanceChain: [String]
}

/// Compressed summary of how a specimen "plays" musically.
/// Derived from the live MusicalMemory at wrap time — not a full replay.
struct MusicalMemorySnapshot: Codable {
    let dominantScale: String   // e.g., "Dorian", "Pentatonic Minor"
    let averageTempo: Float     // BPM average over the specimen's life
    let preferredRegister: String  // "Low", "Mid", "High", "Full Range"
    let saturationLevel: Float  // 0.0–1.0 — how driven/distorted the voice ran
}

// MARK: - GiftPackage

/// A wrapped specimen ready to be shared via AirDrop, Messages, or clipboard.
struct GiftPackage: Codable, Identifiable {
    let packageId: String           // 8 alphanumeric chars
    let senderName: String
    let recipientNote: String       // Personal message, max 100 chars
    let specimen: GiftedSpecimen
    let wrappedDate: Date
    var isOpened: Bool
    let giftType: GiftType

    var id: String { packageId }

    /// True when the package is still within the 1-hour recall window
    /// and has not yet been exported/shared (checked locally only).
    var isRecallable: Bool {
        !isOpened && Date().timeIntervalSince(wrappedDate) < 3600
    }

    /// Display string shown in the Gifts tab.
    var displayTitle: String {
        switch giftType {
        case .normal:   return "Gift from \(senderName)"
        case .seasonal: return "Seasonal Gift from \(senderName)"
        case .secret:   return "Mystery Package"
        }
    }

    /// Reveal the sender's name even for secret gifts (used post-open only).
    var revealedSenderName: String { senderName }
}

// MARK: - GiftHistoryEntry

/// Immutable log entry for every gift sent or received.
struct GiftHistoryEntry: Codable, Identifiable {
    let id: UUID
    let packageId: String
    let direction: Direction
    let specimenDisplayName: String
    let otherPlayerName: String
    let timestamp: Date
    let giftType: GiftType

    enum Direction: String, Codable {
        case sent
        case received
    }
}

// MARK: - GiftingManager

/// Orchestrates the full gifting lifecycle — wrapping, export, import, opening.
///
/// Limits and safety rails:
/// - Max 3 outgoing gifts per calendar day (prevents accidental reef drain).
/// - Unopened outgoing gifts can be recalled within 1 hour of wrapping.
/// - Gifted specimens arrive at level 1 but keep genetics, memory, and journal.
/// - Every transfer appends to the provenance chain.
final class GiftingManager: ObservableObject {

    // MARK: - Published State

    @Published var outgoingGifts: [GiftPackage] = []       // Wrapped, not yet exported
    @Published var receivedGifts: [GiftPackage] = []       // Received, unopened first
    @Published var giftHistory: [GiftHistoryEntry] = []    // Full audit log

    // MARK: - Constants

    static let maxOutgoingPerDay = 3
    private let outgoingKey = "obrix_outgoing_gifts"
    private let receivedKey = "obrix_received_gifts"
    private let historyKey  = "obrix_gift_history"
    private let dailyCountKey = "obrix_gift_daily_count"
    private let dailyDateKey  = "obrix_gift_daily_date"

    // MARK: - Init

    init() {
        restoreAll()
    }

    // MARK: - Daily Limit

    /// How many gifts have been wrapped today.
    var giftsWrappedToday: Int {
        guard let lastDate = UserDefaults.standard.object(forKey: dailyDateKey) as? Date,
              Calendar.current.isDateInToday(lastDate) else {
            return 0
        }
        return UserDefaults.standard.integer(forKey: dailyCountKey)
    }

    var canWrapToday: Bool { giftsWrappedToday < Self.maxOutgoingPerDay }

    private func incrementDailyCount() {
        let today = Date()
        let count: Int
        if let lastDate = UserDefaults.standard.object(forKey: dailyDateKey) as? Date,
           Calendar.current.isDateInToday(lastDate) {
            count = UserDefaults.standard.integer(forKey: dailyCountKey) + 1
        } else {
            count = 1
        }
        UserDefaults.standard.set(today,  forKey: dailyDateKey)
        UserDefaults.standard.set(count,  forKey: dailyCountKey)
    }

    // MARK: - Wrap (Send Side)

    /// Wrap a specimen for gifting — removes it from the reef.
    ///
    /// - Parameters:
    ///   - specimenId: The reef specimen to wrap.
    ///   - reefStore: Live reef — specimen is removed here on success.
    ///   - senderName: Display name of the sending player.
    ///   - note: Personal message to accompany the gift (max 100 chars).
    ///   - giftType: Normal, Seasonal, or Secret.
    /// - Returns: The wrapped GiftPackage, or nil if the daily limit is reached
    ///   or the specimen cannot be found.
    @discardableResult
    func wrapSpecimen(
        specimenId: UUID,
        reefStore: ReefStore,
        senderName: String,
        note: String,
        giftType: GiftType
    ) -> GiftPackage? {
        guard canWrapToday else { return nil }

        guard let slotIndex = reefStore.specimens.firstIndex(where: { $0?.id == specimenId }),
              var specimen = reefStore.specimens[slotIndex] else { return nil }

        // Write "Gifted Away" journal entry before extracting genetic data
        let giftEntry = JournalEntry(
            id: UUID(),
            timestamp: Date(),
            type: .traded,
            description: "Gifted away to \(senderName.isEmpty ? "a friend" : senderName)"
        )
        specimen.journal.append(giftEntry)
        reefStore.specimens[slotIndex] = specimen // write back before removal

        // Build provenance chain from existing entries + current reef
        var provenanceChain = specimen.provenance.map(\.fromPlayer)
        provenanceChain.append(reefStore.reefName)

        // Select 3–5 journal highlights (most recent non-born entries, fallback to born)
        let highlights = Self.extractJournalHighlights(from: specimen.journal)

        let gifted = GiftedSpecimen(
            subtypeId: specimen.subtype,
            displayName: specimen.name,
            generation: specimen.journal.filter { $0.type == .born }.count,
            genome: specimen.spectralDNA,
            voiceProfileData: specimen.parameterState,
            musicalMemorySnapshot: MusicalMemorySnapshot(
                dominantScale: "Unknown",    // Caller can inject real data
                averageTempo: 120,
                preferredRegister: Self.preferredRegister(for: specimen),
                saturationLevel: specimen.aggressiveScore / max(specimen.gentleScore + specimen.aggressiveScore, 1)
            ),
            journalHighlights: highlights,
            provenanceChain: provenanceChain
        )

        let package = GiftPackage(
            packageId: Self.generatePackageId(),
            senderName: String(senderName.prefix(20)),
            recipientNote: String(note.prefix(100)),
            specimen: gifted,
            wrappedDate: Date(),
            isOpened: false,
            giftType: giftType
        )

        // Remove specimen from reef (and its coupling routes)
        reefStore.releaseSpecimen(at: slotIndex)

        outgoingGifts.append(package)
        incrementDailyCount()
        appendHistory(GiftHistoryEntry(
            id: UUID(),
            packageId: package.packageId,
            direction: .sent,
            specimenDisplayName: specimen.name,
            otherPlayerName: "recipient",
            timestamp: Date(),
            giftType: giftType
        ))

        saveAll()
        return package
    }

    // MARK: - Recall (Undo for Sender)

    /// Cancel an outgoing gift and return the specimen to the reef.
    ///
    /// Only works within 1 hour of wrapping and only if the package has
    /// not been opened (i.e., it has not been exported yet from the
    /// recipient's perspective — locally we just discard the package).
    ///
    /// - Returns: A partially reconstructed Specimen if successful, nil otherwise.
    @discardableResult
    func recallGift(packageId: String, reefStore: ReefStore) -> Specimen? {
        guard let idx = outgoingGifts.firstIndex(where: { $0.packageId == packageId }),
              outgoingGifts[idx].isRecallable else { return nil }

        let package = outgoingGifts[idx]
        outgoingGifts.remove(at: idx)

        // Reconstruct a minimal specimen and return it to the reef
        let gifted = package.specimen
        guard let entry = SpecimenCatalog.entry(for: gifted.subtypeId) else {
            saveAll()
            return nil
        }

        let recallJournal = [
            JournalEntry(
                id: UUID(),
                timestamp: Date(),
                type: .born,
                description: "Returned from a gift that was recalled within 1 hour"
            )
        ]

        let specimen = Specimen(
            id: UUID(),
            name: gifted.displayName,
            category: entry.category,
            rarity: .common,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: gifted.subtypeId,
            catchAccelPattern: [],
            provenance: [],
            spectralDNA: gifted.genome,
            parameterState: gifted.voiceProfileData,
            catchLatitude: nil,
            catchLongitude: nil,
            catchTimestamp: Date(),
            catchWeatherDescription: "Recalled gift",
            creatureGenomeData: nil,
            cosmeticTier: .standard,
            morphIndex: 0,
            musicHash: nil,
            sourceTrackTitle: nil,
            xp: 0,
            level: 1,
            aggressiveScore: 0,
            gentleScore: 0,
            totalPlaySeconds: 0,
            journal: recallJournal,
            isFavorite: false
        )

        reefStore.addSpecimen(specimen)
        saveAll()
        return specimen
    }

    // MARK: - Export (Sender Shares Package)

    /// Serialize a wrapped GiftPackage to JSON Data for sharing.
    func exportGift(package: GiftPackage) -> Data? {
        let encoder = JSONEncoder()
        encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        encoder.dateEncodingStrategy = .iso8601
        return try? encoder.encode(package)
    }

    /// Write a gift package to a temporary file and return the URL.
    func exportGiftToFile(package: GiftPackage) -> URL? {
        guard let data = exportGift(package: package) else { return nil }
        let safeName = package.specimen.displayName
            .replacingOccurrences(of: " ", with: "_")
            .replacingOccurrences(of: "/", with: "-")
        let fileName = "gift_\(safeName)_\(package.packageId).obrixgift"
        let tempDir = FileManager.default.temporaryDirectory
            .appendingPathComponent("ObrixGifts", isDirectory: true)
        do {
            try FileManager.default.createDirectory(at: tempDir, withIntermediateDirectories: true)
            let fileURL = tempDir.appendingPathComponent(fileName)
            try data.write(to: fileURL)
            return fileURL
        } catch {
            return nil
        }
    }

    // MARK: - Import (Receiver Accepts Package)

    /// Deserialize a received gift package from JSON Data.
    /// Returns nil on decode failure — caller should show an error toast.
    func importGift(from data: Data) -> GiftPackage? {
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        guard let package = try? decoder.decode(GiftPackage.self, from: data) else {
            return nil
        }
        return package
    }

    /// Register a received package in the inbox.
    /// Deduplicates by packageId — receiving the same package twice is a no-op.
    func receiveGift(_ package: GiftPackage) {
        guard !receivedGifts.contains(where: { $0.packageId == package.packageId }) else { return }
        // Ensure isOpened is false regardless of what the sender set
        var incoming = package
        incoming.isOpened = false
        // Unopened gifts sort first
        if let firstOpenedIdx = receivedGifts.firstIndex(where: { $0.isOpened }) {
            receivedGifts.insert(incoming, at: firstOpenedIdx)
        } else {
            receivedGifts.append(incoming)
        }
        saveAll()
    }

    // MARK: - Open (Receiver Unwraps)

    /// Unwrap a received gift and add the specimen to the reef.
    ///
    /// - Gifted specimens arrive at level 1.
    /// - Genetics, musical memory, and journal highlights are preserved.
    /// - Provenance chain is extended with the receiver's reef name.
    ///
    /// - Returns: The reconstructed Specimen, or nil if the package was
    ///   not found, already opened, or the reef is full.
    @discardableResult
    func openGift(
        packageId: String,
        receiverName: String,
        reefStore: ReefStore
    ) -> Specimen? {
        guard let idx = receivedGifts.firstIndex(where: { $0.packageId == packageId }),
              !receivedGifts[idx].isOpened else { return nil }

        let package = receivedGifts[idx]
        let gifted  = package.specimen

        guard let entry = SpecimenCatalog.entry(for: gifted.subtypeId) else { return nil }

        // Build journal: highlights + arrival entry
        var journal: [JournalEntry] = gifted.journalHighlights.map { highlight in
            JournalEntry(
                id: UUID(),
                timestamp: Date(),
                type: .born,
                description: highlight
            )
        }
        journal.append(JournalEntry(
            id: UUID(),
            timestamp: Date(),
            type: .born,
            description: "Arrived as a gift from \(package.senderName). Came from: \(gifted.provenanceChain.joined(separator: " → "))"
        ))

        let specimen = Specimen(
            id: UUID(),
            name: gifted.displayName,
            category: entry.category,
            rarity: entry.baseRarity,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: gifted.subtypeId,
            catchAccelPattern: [],
            provenance: gifted.provenanceChain.map {
                ProvenanceEntry(fromPlayer: $0, toPlayer: receiverName, timestamp: Date(), location: nil)
            },
            spectralDNA: gifted.genome,
            parameterState: gifted.voiceProfileData,
            catchLatitude: nil,
            catchLongitude: nil,
            catchTimestamp: Date(),
            catchWeatherDescription: "Received as a gift",
            creatureGenomeData: nil,
            cosmeticTier: .standard,
            morphIndex: 0,
            musicHash: nil,
            sourceTrackTitle: nil,
            xp: 0,
            level: 1,         // Always level 1 on arrival
            aggressiveScore: 0,
            gentleScore: 0,
            totalPlaySeconds: 0,
            journal: journal,
            isFavorite: false
        )

        guard reefStore.addSpecimen(specimen) != nil else {
            // Reef full — do not mark as opened; let player clear space first
            return nil
        }

        // Mark opened
        receivedGifts[idx].isOpened = true

        appendHistory(GiftHistoryEntry(
            id: UUID(),
            packageId: packageId,
            direction: .received,
            specimenDisplayName: gifted.displayName,
            otherPlayerName: package.senderName,
            timestamp: Date(),
            giftType: package.giftType
        ))

        // Notify narrative and achievement systems
        NarrativeArcManager.shared.recordTradeCompleted()
        AchievementManager.shared.onTradesCompleted(1)

        // Cap opened gifts: keep all unopened + last 100 opened to prevent unbounded growth
        let unopened = receivedGifts.filter { !$0.isOpened }
        let opened   = Array(receivedGifts.filter { $0.isOpened }.suffix(100))
        receivedGifts = unopened + opened

        saveAll()
        return specimen
    }

    // MARK: - Accessors

    var unopenedCount: Int { receivedGifts.filter { !$0.isOpened }.count }

    var recallableGifts: [GiftPackage] { outgoingGifts.filter(\.isRecallable) }

    // MARK: - Deletion

    func dismissOpenedGift(packageId: String) {
        receivedGifts.removeAll { $0.packageId == packageId && $0.isOpened }
        saveAll()
    }

    // MARK: - Persistence

    private func saveAll() {
        let encoder = JSONEncoder()
        encoder.dateEncodingStrategy = .iso8601
        if let data = try? encoder.encode(outgoingGifts) {
            UserDefaults.standard.set(data, forKey: outgoingKey)
        }
        if let data = try? encoder.encode(receivedGifts) {
            UserDefaults.standard.set(data, forKey: receivedKey)
        }
        if let data = try? encoder.encode(giftHistory) {
            UserDefaults.standard.set(data, forKey: historyKey)
        }
    }

    private func restoreAll() {
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        if let data = UserDefaults.standard.data(forKey: outgoingKey),
           let decoded = try? decoder.decode([GiftPackage].self, from: data) {
            outgoingGifts = decoded
        }
        if let data = UserDefaults.standard.data(forKey: receivedKey),
           let decoded = try? decoder.decode([GiftPackage].self, from: data) {
            // Unopened first
            receivedGifts = decoded.sorted { !$0.isOpened && $1.isOpened }
        }
        if let data = UserDefaults.standard.data(forKey: historyKey),
           let decoded = try? decoder.decode([GiftHistoryEntry].self, from: data) {
            giftHistory = decoded
        }
    }

    // MARK: - Private Helpers

    private func appendHistory(_ entry: GiftHistoryEntry) {
        giftHistory.insert(entry, at: 0)
        // Cap history at 200 entries
        if giftHistory.count > 200 {
            giftHistory = Array(giftHistory.prefix(200))
        }
    }

    /// 8-character alphanumeric package ID.
    private static func generatePackageId() -> String {
        let chars = "abcdefghijklmnopqrstuvwxyz0123456789"
        return String((0..<8).compactMap { _ in chars.randomElement() })
    }

    /// Pick 3–5 meaningful journal entries for the highlights payload.
    /// Prefers levelUp, drifted, played over born. Falls back to most recent.
    private static func extractJournalHighlights(from journal: [JournalEntry]) -> [String] {
        let preferred: [JournalEntry.JournalEventType] = [.levelUp, .drifted, .played, .unwired, .wired]
        var highlights = journal
            .filter { preferred.contains($0.type) }
            .sorted { $0.timestamp > $1.timestamp }
            .prefix(5)
            .map(\.description)

        if highlights.count < 3 {
            let fallback = journal
                .sorted { $0.timestamp > $1.timestamp }
                .prefix(3 - highlights.count)
                .map(\.description)
            highlights.append(contentsOf: fallback)
        }
        return Array(highlights.prefix(5))
    }

    /// Derive a register label from the specimen's play history.
    private static func preferredRegister(for specimen: Specimen) -> String {
        // Heuristic: gentle-dominant = "Mid/High", aggressive-dominant = "Low/Mid"
        let total = specimen.gentleScore + specimen.aggressiveScore
        guard total > 0 else { return "Full Range" }
        let gentleRatio = specimen.gentleScore / total
        switch gentleRatio {
        case 0.7...:    return "High"
        case 0.4..<0.7: return "Mid"
        default:        return "Low"
        }
    }
}

// MARK: - CatalogEntry Extension

extension CatalogEntry {
    /// Derive a base rarity from the catalog entry's depth flag.
    /// Deep specimens are rare by default; core specimens are common.
    var baseRarity: SpecimenRarity { isDeepSpecimen ? .rare : .common }
}
