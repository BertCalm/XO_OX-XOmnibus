import Foundation
import UIKit

struct TradeOffer: Identifiable {
    let id = UUID()
    let requestedSubtype: String    // What the trader wants
    let requestedMinLevel: Int      // Minimum level
    let offeredSubtype: String      // What they'll give you
    let offeredRarity: SpecimenRarity
    let offeredLevel: Int
    let expiresAt: Date

    var isExpired: Bool { Date() > expiresAt }
}

final class TradePostManager: ObservableObject {
    @Published var offers: [TradeOffer] = []

    private let refreshKey = "obrix_trade_last_refresh"

    init() {
        refreshIfNeeded()
    }

    /// Generate 3 random trade offers (refresh daily)
    func refreshIfNeeded() {
        if let lastRefresh = UserDefaults.standard.object(forKey: refreshKey) as? Date,
           Calendar.current.isDateInToday(lastRefresh),
           !offers.isEmpty {
            return // Already refreshed today
        }
        generateOffers()
        UserDefaults.standard.set(Date(), forKey: refreshKey)
    }

    func generateOffers() {
        let allSubtypes = SpecimenCatalog.coreSpecimens.map { $0.subtypeID }
        let expiry = Calendar.current.startOfDay(for: Date().addingTimeInterval(86400))

        offers = (0..<3).map { _ in
            let requested = allSubtypes.randomElement()!
            var offered = allSubtypes.randomElement()!
            while offered == requested { offered = allSubtypes.randomElement()! }

            let rarities: [SpecimenRarity] = [.common, .common, .uncommon, .uncommon, .rare]

            return TradeOffer(
                requestedSubtype: requested,
                requestedMinLevel: Int.random(in: 1...5),
                offeredSubtype: offered,
                offeredRarity: rarities.randomElement()!,
                offeredLevel: Int.random(in: 1...7),
                expiresAt: expiry
            )
        }
    }

    /// Execute a trade: give a specimen, receive a new one
    @discardableResult
    func executeTrade(offer: TradeOffer, givingSpecimen: Specimen, reefStore: ReefStore) -> Specimen? {
        // Validate
        guard givingSpecimen.subtype == offer.requestedSubtype,
              givingSpecimen.level >= offer.requestedMinLevel else { return nil }

        // Create the received specimen
        guard let entry = SpecimenCatalog.entry(for: offer.offeredSubtype) else { return nil }

        let received = Specimen(
            id: UUID(),
            name: entry.displayName(rarity: offer.offeredRarity),
            category: entry.category,
            rarity: offer.offeredRarity,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: offer.offeredSubtype,
            catchAccelPattern: [],
            provenance: [ProvenanceEntry(fromPlayer: "Trader", toPlayer: "You", timestamp: Date(), location: nil)],
            spectralDNA: (0..<64).map { _ in Float.random(in: 0.2...0.8) },
            parameterState: entry.defaultParams,
            catchLatitude: nil,
            catchLongitude: nil,
            catchTimestamp: Date(),
            catchWeatherDescription: "Traded",
            creatureGenomeData: nil,
            cosmeticTier: .standard,
            morphIndex: 0,
            musicHash: nil,
            sourceTrackTitle: nil,
            xp: 0,
            level: offer.offeredLevel,
            aggressiveScore: 0,
            gentleScore: 0,
            totalPlaySeconds: 0,
            journal: [JournalEntry(id: UUID(), timestamp: Date(), type: .traded, description: "Received from Trader for a \(givingSpecimen.creatureName)")],
            isFavorite: false
        )

        // Remove the offer
        offers.removeAll { $0.id == offer.id }

        UINotificationFeedbackGenerator().notificationOccurred(.success)

        return received
    }
}
