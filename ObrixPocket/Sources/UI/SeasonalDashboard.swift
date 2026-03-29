import SwiftUI

/// Current season info, active limited-time events, seasonal challenges,
/// and community event participation — all in one scrollable view.
struct SeasonalDashboard: View {

    @ObservedObject private var seasonalManager = SeasonalEventManager.shared
    @StateObject private var communityManager = CommunityEventManager()

    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: DesignTokens.spacing20) {

                    // Current season hero card
                    seasonHeroCard
                        .padding(.horizontal, DesignTokens.spacing20)

                    // Active limited-time event
                    if let event = seasonalManager.activeEvent {
                        activeLimitedEventCard(event)
                            .padding(.horizontal, DesignTokens.spacing20)
                    }

                    // Upcoming event preview
                    if let upcoming = seasonalManager.upcomingEvent {
                        upcomingEventRow(upcoming)
                            .padding(.horizontal, DesignTokens.spacing20)
                    }

                    // Seasonal challenge
                    seasonalChallengeCard
                        .padding(.horizontal, DesignTokens.spacing20)

                    // Coupling bonus indicators
                    couplingBonusSection
                        .padding(.horizontal, DesignTokens.spacing20)

                    // Community events
                    if !communityManager.activeEvents.isEmpty {
                        communitySection
                            .padding(.horizontal, DesignTokens.spacing20)
                    }

                    Spacer(minLength: DesignTokens.spacing40)
                }
                .padding(.top, DesignTokens.spacing16)
            }
            .background(DesignTokens.background)
            .navigationTitle("Seasons")
            .navigationBarTitleDisplayMode(.large)
        }
        .preferredColorScheme(.dark)
        .onAppear {
            seasonalManager.refresh()
            communityManager.refresh()
        }
    }

    // MARK: - Season Hero Card

    private var seasonHeroCard: some View {
        let season = seasonalManager.currentSeason
        let palette = season.palette
        let primary = Color(palette.primary)
        let secondary = Color(palette.secondary)

        return VStack(alignment: .leading, spacing: DesignTokens.spacing12) {

            HStack(alignment: .top) {
                VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                    Text("CURRENT SEASON")
                        .font(DesignTokens.mono(9))
                        .tracking(2)
                        .foregroundColor(.white.opacity(0.3))
                    Text(season.displayName)
                        .font(DesignTokens.heading(26))
                        .foregroundColor(.white)
                }

                Spacer()

                // Days remaining badge
                VStack(alignment: .trailing, spacing: 2) {
                    Text("\(season.daysRemaining())")
                        .font(DesignTokens.monoBold(20))
                        .foregroundColor(primary)
                    Text("days left")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.3))
                }
            }

            // Sonic description
            Text(season.sonicDescription)
                .font(DesignTokens.body(13))
                .foregroundColor(.white.opacity(0.5))
                .lineSpacing(2)

            // Color palette swatches
            HStack(spacing: DesignTokens.spacing6) {
                Text("PALETTE")
                    .font(DesignTokens.mono(8))
                    .tracking(1.5)
                    .foregroundColor(.white.opacity(0.2))
                paletteSwatchRow(palette: palette)
            }
        }
        .padding(DesignTokens.spacing16)
        .background(
            ZStack {
                DesignTokens.panelBackground
                LinearGradient(
                    colors: [primary.opacity(0.12), secondary.opacity(0.06), .clear],
                    startPoint: .topLeading,
                    endPoint: .bottomTrailing
                )
            }
        )
        .clipShape(RoundedRectangle(cornerRadius: 14))
        .overlay(
            RoundedRectangle(cornerRadius: 14)
                .stroke(primary.opacity(0.25), lineWidth: 1)
        )
    }

    @ViewBuilder
    private func paletteSwatchRow(palette: SeasonalPalette) -> some View {
        let colors: [(Color, String)] = [
            (Color(palette.primary),   "primary"),
            (Color(palette.secondary), "secondary"),
            (Color(palette.ambient),   "ambient"),
            (Color(palette.waterTint), "water"),
        ]
        HStack(spacing: DesignTokens.spacing4) {
            ForEach(colors, id: \.1) { color, _ in
                RoundedRectangle(cornerRadius: 3)
                    .fill(color)
                    .frame(width: 24, height: 10)
            }
        }
    }

    // MARK: - Active Limited Event Card

    private func activeLimitedEventCard(_ event: LimitedTimeEvent) -> some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing12) {

            HStack {
                Image(systemName: eventSymbol(event.type))
                    .font(.system(size: 14))
                    .foregroundColor(DesignTokens.xoGold)

                Text("ACTIVE EVENT")
                    .font(DesignTokens.mono(9))
                    .tracking(2)
                    .foregroundColor(DesignTokens.xoGold.opacity(0.7))

                Spacer()

                // Time remaining
                HStack(spacing: 4) {
                    Image(systemName: "timer")
                        .font(.system(size: 10))
                    Text(event.daysRemaining == 0 ? "Ends today" : "\(event.daysRemaining)d remaining")
                        .font(DesignTokens.mono(10))
                }
                .foregroundColor(.white.opacity(0.4))
            }

            Text(event.type.displayName)
                .font(DesignTokens.heading(16))
                .foregroundColor(.white)

            Text(event.type.description)
                .font(DesignTokens.body(12))
                .foregroundColor(.white.opacity(0.5))
                .lineSpacing(2)
                .fixedSize(horizontal: false, vertical: true)
        }
        .padding(DesignTokens.spacing16)
        .background(DesignTokens.panelBackground)
        .clipShape(RoundedRectangle(cornerRadius: 12))
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(DesignTokens.xoGold.opacity(0.3), lineWidth: 1)
        )
    }

    // MARK: - Upcoming Event Row

    private func upcomingEventRow(_ event: LimitedTimeEvent) -> some View {
        HStack(spacing: DesignTokens.spacing12) {
            Image(systemName: "clock")
                .font(.system(size: 12))
                .foregroundColor(.white.opacity(0.25))
                .frame(width: 20)

            VStack(alignment: .leading, spacing: 2) {
                Text("COMING SOON")
                    .font(DesignTokens.mono(8))
                    .tracking(1.5)
                    .foregroundColor(.white.opacity(0.2))
                Text(event.type.displayName)
                    .font(DesignTokens.bodyMedium(12))
                    .foregroundColor(.white.opacity(0.5))
            }

            Spacer()

            let formatter = DateFormatter()
            let _ = { formatter.dateStyle = .short; formatter.timeStyle = .none }()
            Text(formatter.string(from: event.startDate))
                .font(DesignTokens.mono(10))
                .foregroundColor(.white.opacity(0.3))
        }
        .padding(.vertical, DesignTokens.spacing8)
        .padding(.horizontal, DesignTokens.spacing12)
        .background(Color.white.opacity(0.03))
        .clipShape(RoundedRectangle(cornerRadius: 10))
    }

    // MARK: - Seasonal Challenge Card

    private var seasonalChallengeCard: some View {
        let challenge = seasonalManager.challengeProgress
        let fraction  = CGFloat(challenge.completionFraction)

        return VStack(alignment: .leading, spacing: DesignTokens.spacing12) {

            HStack {
                Text("SEASONAL CHALLENGE")
                    .font(DesignTokens.mono(9))
                    .tracking(2)
                    .foregroundColor(.white.opacity(0.25))
                Spacer()
                if challenge.isComplete {
                    HStack(spacing: 4) {
                        Image(systemName: "checkmark.circle.fill")
                            .font(.system(size: 11))
                        Text("Complete")
                            .font(DesignTokens.mono(9))
                    }
                    .foregroundColor(DesignTokens.reefJade)
                }
            }

            Text(challenge.title)
                .font(DesignTokens.heading(16))
                .foregroundColor(.white)

            Text(challenge.description)
                .font(DesignTokens.body(12))
                .foregroundColor(.white.opacity(0.5))
                .lineSpacing(2)
                .fixedSize(horizontal: false, vertical: true)

            // Progress breakdown
            VStack(spacing: DesignTokens.spacing6) {
                // Catch progress
                progressRow(
                    label: "Catches",
                    current: challenge.catchProgress,
                    target: challenge.catchTarget,
                    color: DesignTokens.reefJade
                )

                // Breed requirement (if applicable)
                if challenge.breedRequired {
                    HStack(spacing: DesignTokens.spacing8) {
                        Image(systemName: challenge.breedCompleted ? "checkmark.circle.fill" : "circle")
                            .font(.system(size: 11))
                            .foregroundColor(challenge.breedCompleted ? DesignTokens.reefJade : .white.opacity(0.25))
                        Text("Breed during season")
                            .font(DesignTokens.body(11))
                            .foregroundColor(challenge.breedCompleted ? .white.opacity(0.7) : .white.opacity(0.35))
                        Spacer()
                    }
                }

                // Overall progress bar
                GeometryReader { geo in
                    ZStack(alignment: .leading) {
                        RoundedRectangle(cornerRadius: 3)
                            .fill(Color.white.opacity(0.06))
                        RoundedRectangle(cornerRadius: 3)
                            .fill(challenge.isComplete ? DesignTokens.reefJade : DesignTokens.xoGold.opacity(0.7))
                            .frame(width: geo.size.width * min(fraction, 1.0))
                            .animation(.easeInOut(duration: 0.4), value: fraction)
                    }
                }
                .frame(height: 3)
                .padding(.top, DesignTokens.spacing4)
            }
        }
        .padding(DesignTokens.spacing16)
        .background(DesignTokens.panelBackground)
        .clipShape(RoundedRectangle(cornerRadius: 12))
    }

    // MARK: - Coupling Bonus Section

    private var couplingBonusSection: some View {
        let bonuses = seasonalManager.currentSeason.couplingAffinityBonuses
        guard !bonuses.isEmpty else { return AnyView(EmptyView()) }

        return AnyView(
            VStack(alignment: .leading, spacing: DesignTokens.spacing8) {
                Text("SEASONAL COUPLING BONUS")
                    .font(DesignTokens.mono(9))
                    .tracking(2)
                    .foregroundColor(.white.opacity(0.2))

                FlowLayout(spacing: DesignTokens.spacing6) {
                    ForEach(bonuses, id: \.self) { couplingType in
                        HStack(spacing: DesignTokens.spacing4) {
                            Image(systemName: "arrow.triangle.2.circlepath")
                                .font(.system(size: 9))
                            Text(couplingType)
                                .font(DesignTokens.mono(9))
                            Text("+20%")
                                .font(DesignTokens.mono(9))
                                .foregroundColor(DesignTokens.reefJade)
                        }
                        .foregroundColor(.white.opacity(0.55))
                        .padding(.horizontal, DesignTokens.spacing8)
                        .padding(.vertical, DesignTokens.spacing4)
                        .background(DesignTokens.reefJade.opacity(0.08))
                        .clipShape(Capsule())
                    }
                }
            }
        )
    }

    // MARK: - Community Section

    private var communitySection: some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing12) {
            Text("COMMUNITY EVENTS")
                .font(DesignTokens.mono(9))
                .tracking(2)
                .foregroundColor(.white.opacity(0.2))

            ForEach(communityManager.activeEvents) { event in
                communityEventCard(event)
            }
        }
    }

    private func communityEventCard(_ event: CommunityEvent) -> some View {
        let globalFraction = CGFloat(event.estimatedGlobalProgress)
        let localFraction  = CGFloat(event.localFraction)

        return VStack(alignment: .leading, spacing: DesignTokens.spacing12) {

            HStack(alignment: .top) {
                VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                    Text(event.eventType.displayName.uppercased())
                        .font(DesignTokens.mono(8))
                        .tracking(1.5)
                        .foregroundColor(DesignTokens.deepAccent.opacity(0.8))
                    Text(event.title)
                        .font(DesignTokens.heading(14))
                        .foregroundColor(.white)
                }

                Spacer()

                VStack(alignment: .trailing, spacing: 2) {
                    Text("\(event.daysRemaining)d")
                        .font(DesignTokens.monoBold(13))
                        .foregroundColor(.white.opacity(0.5))
                    Text("left")
                        .font(DesignTokens.mono(8))
                        .foregroundColor(.white.opacity(0.2))
                }
            }

            Text(event.description)
                .font(DesignTokens.body(11))
                .foregroundColor(.white.opacity(0.4))
                .lineSpacing(2)
                .fixedSize(horizontal: false, vertical: true)

            // Global progress bar
            VStack(spacing: DesignTokens.spacing4) {
                HStack {
                    Text("COMMUNITY")
                        .font(DesignTokens.mono(8))
                        .tracking(1.2)
                        .foregroundColor(.white.opacity(0.2))
                    Spacer()
                    Text("\(Int(event.estimatedGlobalProgress * 100))%")
                        .font(DesignTokens.monoBold(10))
                        .foregroundColor(DesignTokens.deepAccent)
                }

                GeometryReader { geo in
                    ZStack(alignment: .leading) {
                        RoundedRectangle(cornerRadius: 3)
                            .fill(Color.white.opacity(0.06))
                        RoundedRectangle(cornerRadius: 3)
                            .fill(DesignTokens.deepAccent.opacity(0.6))
                            .frame(width: geo.size.width * min(globalFraction, 1.0))
                            .animation(.easeInOut(duration: 0.5), value: globalFraction)
                    }
                }
                .frame(height: 4)
            }

            // Local contribution row
            HStack(spacing: DesignTokens.spacing6) {
                Image(systemName: "person.fill")
                    .font(.system(size: 9))
                    .foregroundColor(.white.opacity(0.25))
                Text("Your contribution:")
                    .font(DesignTokens.body(10))
                    .foregroundColor(.white.opacity(0.3))
                Text("\(event.localContribution)")
                    .font(DesignTokens.monoBold(10))
                    .foregroundColor(.white.opacity(0.6))

                if event.localContribution > 0 {
                    GeometryReader { geo in
                        ZStack(alignment: .leading) {
                            RoundedRectangle(cornerRadius: 2)
                                .fill(Color.white.opacity(0.04))
                            RoundedRectangle(cornerRadius: 2)
                                .fill(DesignTokens.reefJade.opacity(0.5))
                                .frame(width: geo.size.width * min(localFraction, 1.0))
                        }
                    }
                    .frame(width: 50, height: 3)
                }

                Spacer()
            }

            // Rewards row
            if !event.rewards.isEmpty {
                HStack(spacing: DesignTokens.spacing6) {
                    Text("REWARDS")
                        .font(DesignTokens.mono(8))
                        .tracking(1.2)
                        .foregroundColor(.white.opacity(0.2))
                    ForEach(event.rewards, id: \.rawValue) { reward in
                        Text(reward.displayName)
                            .font(DesignTokens.mono(8))
                            .foregroundColor(.white.opacity(0.4))
                            .padding(.horizontal, DesignTokens.spacing6)
                            .padding(.vertical, 2)
                            .background(Color.white.opacity(0.05))
                            .clipShape(Capsule())
                    }
                }
            }
        }
        .padding(DesignTokens.spacing16)
        .background(DesignTokens.panelBackground)
        .clipShape(RoundedRectangle(cornerRadius: 12))
        .overlay(
            RoundedRectangle(cornerRadius: 12)
                .stroke(DesignTokens.deepAccent.opacity(0.25), lineWidth: 1)
        )
    }

    // MARK: - Helpers

    private func progressRow(label: String, current: Int, target: Int, color: Color) -> some View {
        HStack(spacing: DesignTokens.spacing8) {
            Text(label)
                .font(DesignTokens.body(11))
                .foregroundColor(.white.opacity(0.4))
                .frame(width: 60, alignment: .trailing)

            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 2)
                        .fill(Color.white.opacity(0.06))
                    RoundedRectangle(cornerRadius: 2)
                        .fill(color.opacity(0.6))
                        .frame(width: geo.size.width * CGFloat(min(current, target)) / CGFloat(max(target, 1)))
                }
            }
            .frame(height: 4)

            Text("\(current)/\(target)")
                .font(DesignTokens.mono(10))
                .foregroundColor(.white.opacity(0.35))
                .frame(width: 36, alignment: .trailing)
        }
    }

    private func eventSymbol(_ type: LimitedTimeEventType) -> String {
        switch type {
        case .bioluminescentStorm: return "sparkles"
        case .migrationWave:       return "arrow.forward.circle"
        case .deepCurrent:         return "water.waves"
        case .coralBloom:          return "leaf.fill"
        }
    }
}

// MARK: - FlowLayout

/// Simple wrapping flow layout for coupling bonus pills.
/// Wraps items to a new row when the current row overflows.
struct FlowLayout: Layout {
    var spacing: CGFloat = 8

    func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
        let width = proposal.width ?? 0
        var height: CGFloat = 0
        var rowWidth: CGFloat = 0
        var rowHeight: CGFloat = 0

        for subview in subviews {
            let size = subview.sizeThatFits(.unspecified)
            if rowWidth + size.width > width, rowWidth > 0 {
                height += rowHeight + spacing
                rowWidth = 0
                rowHeight = 0
            }
            rowWidth += size.width + spacing
            rowHeight = max(rowHeight, size.height)
        }
        height += rowHeight

        return CGSize(width: width, height: height)
    }

    func placeSubviews(in bounds: CGRect, proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) {
        var x = bounds.minX
        var y = bounds.minY
        var rowHeight: CGFloat = 0

        for subview in subviews {
            let size = subview.sizeThatFits(.unspecified)
            if x + size.width > bounds.maxX, x > bounds.minX {
                y += rowHeight + spacing
                x = bounds.minX
                rowHeight = 0
            }
            subview.place(at: CGPoint(x: x, y: y), proposal: ProposedViewSize(size))
            x += size.width + spacing
            rowHeight = max(rowHeight, size.height)
        }
    }
}

// MARK: - RGBA → Color Extension

private extension Color {
    init(_ rgba: RGBA) {
        self.init(red: Double(rgba.r), green: Double(rgba.g), blue: Double(rgba.b), opacity: Double(rgba.a))
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    SeasonalDashboard()
        .background(DesignTokens.background.ignoresSafeArea())
}
#endif
