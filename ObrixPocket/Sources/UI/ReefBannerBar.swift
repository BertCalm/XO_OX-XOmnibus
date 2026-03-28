import SwiftUI

/// Random encounter banner + seasonal event banner.
/// Receives encounterManager from ReefTab (ReefTab owns it so onNoteOn can read energyMultiplier).
/// Owns: seasonalEvent.
struct ReefBannerBar: View {
    @ObservedObject var encounterManager: RandomEncounterManager
    @StateObject private var seasonalEvent = SeasonalEventManager()

    var body: some View {
        VStack(spacing: 0) {
            // Random encounter banner — shown while an encounter is active
            if let encounter = encounterManager.activeEncounter {
                HStack(spacing: 8) {
                    Image(systemName: "sparkle")
                        .font(.system(size: 12))
                        .foregroundColor(DesignTokens.effectColor)
                    VStack(alignment: .leading, spacing: 1) {
                        Text(encounter.title)
                            .font(DesignTokens.heading(11))
                            .foregroundColor(DesignTokens.effectColor)
                        if let expiry = encounterManager.encounterExpiry {
                            Text("\(encounter.description) · \(timeRemaining(expiry))")
                                .font(DesignTokens.body(8))
                                .foregroundColor(.white.opacity(0.3))
                        }
                    }
                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.vertical, 4)
                .background(DesignTokens.effectColor.opacity(0.04))
            }

            // Seasonal event banner — shown when an event is active
            if let event = seasonalEvent.currentEvent {
                HStack(spacing: 8) {
                    Image(systemName: "sparkle")
                        .font(.system(size: 10))
                        .foregroundColor(DesignTokens.xoGold)
                    VStack(alignment: .leading, spacing: 1) {
                        Text(event.name.uppercased())
                            .font(DesignTokens.monoBold(9))
                            .tracking(1)
                            .foregroundColor(DesignTokens.xoGold)
                        Text("\(event.daysRemaining) days left · \(event.description)")
                            .font(DesignTokens.body(8))
                            .foregroundColor(.white.opacity(0.3))
                    }
                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.vertical, 4)
                .background(DesignTokens.xoGold.opacity(0.03))
            }
        }
        .onReceive(Timer.publish(every: 60, on: .main, in: .common).autoconnect()) { _ in
            encounterManager.checkExpiry()
        }
    }

    private func timeRemaining(_ date: Date) -> String {
        let remaining = max(0, date.timeIntervalSince(Date()))
        let minutes = Int(remaining / 60)
        return "\(minutes)m left"
    }
}
