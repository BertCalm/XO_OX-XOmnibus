import SwiftUI

/// Formation progress view for all nursery occupants.
/// Embedded inside the Reef tab when nursery.count > 0, or presented
/// as a sheet from a dedicated "Nursery" button in ReefHeaderView.
///
/// This is the most important tutorial moment — the player learns that
/// music played near the nursery shapes the offspring's voice.
struct NurseryFormationView: View {

    @ObservedObject var breedingManager: BreedingManager
    @ObservedObject var nurseryManager: NurseryManager

    /// Fires when the player taps "Graduate" on a ready occupant.
    /// Passes the occupant ID — caller invokes breedingManager.graduateOffspring(id:)
    /// and places the specimen on the reef.
    let onGraduate: (UUID) -> Void

    // Local tick timer so progress rings update in real time.
    @State private var tick = Date()
    private let timer = Timer.publish(every: 60, on: .main, in: .common).autoconnect()

    var body: some View {
        VStack(spacing: 0) {
            sectionHeader

            if breedingManager.nursery.isEmpty {
                emptyState
            } else {
                VStack(spacing: DesignTokens.spacing16) {
                    ForEach(breedingManager.nursery) { occupant in
                        NurseryOccupantCard(
                            occupant: occupant,
                            influences: nurseryManager.influences[occupant.id] ?? [],
                            onGraduate: { onGraduate(occupant.id) }
                        )
                    }
                }
                .padding(.horizontal, DesignTokens.spacing20)
                .padding(.bottom, DesignTokens.spacing24)
            }
        }
        .onReceive(timer) { date in tick = date }
    }

    // MARK: - Section Header

    private var sectionHeader: some View {
        HStack {
            VStack(alignment: .leading, spacing: DesignTokens.spacing2) {
                Text("NURSERY")
                    .font(DesignTokens.mono(10))
                    .tracking(1.5)
                    .foregroundColor(.white.opacity(0.2))
                Text("Formation in progress")
                    .font(DesignTokens.body(12))
                    .foregroundColor(.white.opacity(0.4))
            }
            Spacer()
            capacityBadge
        }
        .padding(.horizontal, DesignTokens.spacing20)
        .padding(.vertical, DesignTokens.spacing16)
    }

    private var capacityBadge: some View {
        HStack(spacing: DesignTokens.spacing4) {
            Text("\(breedingManager.nursery.count)")
                .font(DesignTokens.monoBold(12))
                .foregroundColor(.white)
            Text("/ \(BreedingManager.nurseryCapacity)")
                .font(DesignTokens.mono(12))
                .foregroundColor(.white.opacity(0.3))
        }
        .padding(.horizontal, DesignTokens.spacing8)
        .padding(.vertical, DesignTokens.spacing4)
        .background(
            RoundedRectangle(cornerRadius: 6)
                .fill(Color.white.opacity(0.06))
        )
    }

    private var emptyState: some View {
        VStack(spacing: DesignTokens.spacing8) {
            Text("The nursery is quiet")
                .font(DesignTokens.body(13))
                .foregroundColor(.white.opacity(0.3))
            Text("Wire two specimens for 7 days to begin breeding.")
                .font(DesignTokens.body(11))
                .foregroundColor(.white.opacity(0.2))
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, DesignTokens.spacing40)
        .padding(.horizontal, DesignTokens.spacing20)
    }
}

// MARK: - Occupant Card

private struct NurseryOccupantCard: View {

    let occupant: NurseryOccupant
    let influences: [NurseryInfluence]
    let onGraduate: () -> Void

    @State private var showInfluenceLog = false

    var body: some View {
        VStack(spacing: 0) {
            cardHeader
            Divider().background(Color.white.opacity(0.06))
            musicalInfluenceSection
            if occupant.isReady {
                graduateButton
            }
        }
        .background(
            RoundedRectangle(cornerRadius: 14)
                .fill(DesignTokens.panelBackground)
        )
        .overlay(
            RoundedRectangle(cornerRadius: 14)
                .strokeBorder(
                    occupant.isReady ? DesignTokens.reefJade.opacity(0.5) : Color.white.opacity(0.07),
                    lineWidth: 1
                )
        )
    }

    // MARK: Card Header

    private var cardHeader: some View {
        HStack(spacing: DesignTokens.spacing16) {
            progressRing

            VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                HStack(spacing: DesignTokens.spacing6) {
                    generationBadge
                    if occupant.isReady {
                        Text("READY")
                            .font(DesignTokens.mono(9))
                            .tracking(1)
                            .foregroundColor(DesignTokens.reefJade)
                            .padding(.horizontal, DesignTokens.spacing6)
                            .padding(.vertical, 2)
                            .background(
                                RoundedRectangle(cornerRadius: 4)
                                    .fill(DesignTokens.reefJade.opacity(0.15))
                            )
                    }
                }

                HStack(spacing: DesignTokens.spacing4) {
                    Text("From")
                        .font(DesignTokens.body(11))
                        .foregroundColor(.white.opacity(0.3))
                    Text(occupant.parentNameA)
                        .font(DesignTokens.bodyMedium(11))
                        .foregroundColor(.white.opacity(0.65))
                    Text("x")
                        .font(DesignTokens.mono(10))
                        .foregroundColor(.white.opacity(0.2))
                    Text(occupant.parentNameB)
                        .font(DesignTokens.bodyMedium(11))
                        .foregroundColor(.white.opacity(0.65))
                }

                if !occupant.isReady {
                    Text(hoursRemainingLabel)
                        .font(DesignTokens.mono(10))
                        .foregroundColor(.white.opacity(0.35))
                }
            }

            Spacer()
        }
        .padding(DesignTokens.spacing16)
    }

    private var progressRing: some View {
        ZStack {
            Circle()
                .stroke(Color.white.opacity(0.08), lineWidth: 7)
                .frame(width: 56, height: 56)

            Circle()
                .trim(from: 0, to: CGFloat(occupant.formationProgress))
                .stroke(
                    occupant.isReady ? DesignTokens.reefJade : DesignTokens.xoGold,
                    style: StrokeStyle(lineWidth: 7, lineCap: .round)
                )
                .frame(width: 56, height: 56)
                .rotationEffect(.degrees(-90))

            Text("\(Int(occupant.formationProgress * 100))%")
                .font(DesignTokens.monoBold(11))
                .foregroundColor(.white.opacity(0.8))
        }
    }

    private var generationBadge: some View {
        Text("Gen-\(occupant.generation)")
            .font(DesignTokens.mono(10))
            .foregroundColor(DesignTokens.xoGold)
            .padding(.horizontal, DesignTokens.spacing6)
            .padding(.vertical, 2)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(DesignTokens.xoGold.opacity(0.12))
            )
    }

    private var hoursRemainingLabel: String {
        let h = occupant.hoursRemaining
        if h <= 0 { return "Almost ready" }
        if h == 1 { return "1 hour remaining" }
        return "\(h) hours remaining"
    }

    // MARK: Musical Influence Section

    private var musicalInfluenceSection: some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing8) {
            HStack {
                VStack(alignment: .leading, spacing: DesignTokens.spacing2) {
                    Text("What it's hearing")
                        .font(DesignTokens.mono(9))
                        .tracking(1.2)
                        .foregroundColor(.white.opacity(0.2))
                    Text(ambientSummary)
                        .font(DesignTokens.body(11))
                        .foregroundColor(.white.opacity(0.45))
                }
                Spacer()
                Button(action: { withAnimation { showInfluenceLog.toggle() } }) {
                    Text(showInfluenceLog ? "Hide log" : "Show log")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.3))
                }
            }

            if !influences.isEmpty {
                Text("Play music near the Nursery to shape this voice")
                    .font(DesignTokens.body(10))
                    .foregroundColor(DesignTokens.xoGold.opacity(0.5))
                    .italic()
            } else {
                Text("No influences recorded yet — play music to begin shaping this voice")
                    .font(DesignTokens.body(10))
                    .foregroundColor(.white.opacity(0.2))
                    .italic()
            }

            if showInfluenceLog && !influences.isEmpty {
                influenceLog
            }
        }
        .padding(.horizontal, DesignTokens.spacing16)
        .padding(.vertical, DesignTokens.spacing12)
    }

    private var ambientSummary: String {
        let musicCount = influences.filter { $0.type == .musicPlayed }.count
        let ambientCount = influences.filter { $0.type == .reefAmbient }.count

        if influences.isEmpty { return "Silence" }
        if musicCount > 0 && ambientCount > 0 {
            return "Active reef + player music (\(musicCount) sessions)"
        }
        if musicCount > 0 {
            return "Player music (\(musicCount) sessions)"
        }
        return "Reef ambient heartbeat"
    }

    private var influenceLog: some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
            let recent = Array(influences.suffix(8).reversed())
            ForEach(recent) { influence in
                HStack(spacing: DesignTokens.spacing6) {
                    Circle()
                        .fill(influenceColor(influence.type))
                        .frame(width: 6, height: 6)

                    Text(influenceLabel(influence.type))
                        .font(DesignTokens.body(10))
                        .foregroundColor(.white.opacity(0.45))

                    Spacer()

                    Text(influence.timestamp, style: .relative)
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.2))

                    Text("\(Int(influence.strength * 100))%")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.2))
                        .frame(width: 28, alignment: .trailing)
                }
            }
        }
        .padding(DesignTokens.spacing8)
        .background(
            RoundedRectangle(cornerRadius: 8)
                .fill(Color.white.opacity(0.03))
        )
        .transition(.opacity.combined(with: .move(edge: .top)))
    }

    private func influenceLabel(_ type: NurseryInfluence.InfluenceType) -> String {
        switch type {
        case .musicPlayed:         return "Music played"
        case .reefAmbient:         return "Reef heartbeat"
        case .weatherEvent:        return "Weather event"
        case .divePerformed:       return "Dive performed"
        case .specimenInteraction: return "Specimen interaction"
        }
    }

    private func influenceColor(_ type: NurseryInfluence.InfluenceType) -> Color {
        switch type {
        case .musicPlayed:         return DesignTokens.reefJade
        case .reefAmbient:         return DesignTokens.sourceColor
        case .weatherEvent:        return DesignTokens.xoGold
        case .divePerformed:       return DesignTokens.deepAccent
        case .specimenInteraction: return DesignTokens.modulatorColor
        }
    }

    // MARK: Graduate Button

    private var graduateButton: some View {
        Button(action: onGraduate) {
            HStack(spacing: DesignTokens.spacing8) {
                Text("Graduate to Reef")
                    .font(DesignTokens.bodyMedium(15))
                    .foregroundColor(.black)
            }
            .frame(maxWidth: .infinity)
            .padding(.vertical, DesignTokens.spacing14)
            .background(
                RoundedRectangle(cornerRadius: 10)
                    .fill(DesignTokens.reefJade)
            )
        }
        .padding(.horizontal, DesignTokens.spacing16)
        .padding(.bottom, DesignTokens.spacing16)
    }
}

// MARK: - Spacing convenience

private extension DesignTokens {
    static let spacing14: CGFloat = 14
}

// MARK: - Preview

#if DEBUG
#Preview {
    ScrollView {
        NurseryFormationView(
            breedingManager: {
                let m = BreedingManager()
                let occupant = NurseryOccupant(
                    id: UUID(),
                    parentSubtypeA: "polyblep-saw",
                    parentSubtypeB: "lowpass-ladder",
                    parentNameA: "Sawfin",
                    parentNameB: "Curtain",
                    conceptionDate: Date().addingTimeInterval(-12 * 3600),
                    formationEndDate: Date().addingTimeInterval(12 * 3600),
                    inheritedTraits: InheritedTraits(
                        traitSources: [:],
                        octavePreference: 4,
                        preferredIntervals: [4, 7],
                        attackSharpness: 0.6,
                        sustainLength: 0.5,
                        swing: 0.3,
                        velocityVariation: 0.4,
                        hasMutation: false,
                        mutationDescription: nil,
                        offspringSubtypeID: "polyblep-saw",
                        offspringCategory: "source"
                    ),
                    generation: 2
                )
                m.nursery = [occupant]
                return m
            }(),
            nurseryManager: NurseryManager(),
            onGraduate: { _ in }
        )
    }
    .background(DesignTokens.background.ignoresSafeArea())
}
#endif
