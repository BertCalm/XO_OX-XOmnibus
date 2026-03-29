import SwiftUI

/// Achievement collection view with tier progression.
/// Two-column grid, tier-colored borders, secret achievements hidden until earned.
struct AchievementGalleryView: View {

    @ObservedObject private var manager = AchievementManager.shared

    @State private var selectedCategory: AchievementCategory? = nil
    @State private var selectedAchievement: Achievement? = nil
    @State private var showDetailSheet = false

    private let gridColumns = [
        GridItem(.flexible(), spacing: DesignTokens.spacing12),
        GridItem(.flexible(), spacing: DesignTokens.spacing12)
    ]

    private var filteredAchievements: [Achievement] {
        guard let cat = selectedCategory else { return manager.achievements }
        return manager.achievements.filter { $0.category == cat }
    }

    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 0) {

                    // Player tier + points header
                    tierHeader
                        .padding(.horizontal, DesignTokens.spacing20)
                        .padding(.top, DesignTokens.spacing16)
                        .padding(.bottom, DesignTokens.spacing24)

                    // Category segmented control
                    categorySegment
                        .padding(.horizontal, DesignTokens.spacing20)
                        .padding(.bottom, DesignTokens.spacing16)

                    // Achievement grid
                    LazyVGrid(columns: gridColumns, spacing: DesignTokens.spacing12) {
                        ForEach(filteredAchievements) { achievement in
                            achievementCard(achievement)
                                .onTapGesture {
                                    selectedAchievement = achievement
                                    showDetailSheet = true
                                }
                        }
                    }
                    .padding(.horizontal, DesignTokens.spacing20)

                    Spacer(minLength: DesignTokens.spacing40)
                }
            }
            .background(DesignTokens.background)
            .navigationTitle("Achievements")
            .navigationBarTitleDisplayMode(.large)
        }
        .preferredColorScheme(.dark)
        .sheet(isPresented: $showDetailSheet) {
            if let achievement = selectedAchievement {
                AchievementDetailSheet(achievement: achievement)
            }
        }
    }

    // MARK: - Tier Header

    private var tierHeader: some View {
        VStack(spacing: DesignTokens.spacing12) {

            HStack(spacing: DesignTokens.spacing12) {
                // Tier badge
                ZStack {
                    Circle()
                        .fill(tierColor(manager.playerTier).opacity(0.15))
                        .frame(width: 52, height: 52)
                    Image(systemName: manager.playerTier.symbolName)
                        .font(.system(size: 22, weight: .semibold))
                        .foregroundColor(tierColor(manager.playerTier))
                }

                VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                    Text(manager.playerTier.rawValue.uppercased())
                        .font(DesignTokens.heading(16))
                        .foregroundColor(.white)
                    HStack(spacing: DesignTokens.spacing4) {
                        Text("\(manager.achievementPoints) pts")
                            .font(DesignTokens.monoBold(12))
                            .foregroundColor(DesignTokens.xoGold)
                        if let pointsNeeded = manager.pointsUntilNextTier() {
                            Text("— \(pointsNeeded) to next tier")
                                .font(DesignTokens.body(11))
                                .foregroundColor(.white.opacity(0.3))
                        } else {
                            Text("— Maximum tier")
                                .font(DesignTokens.body(11))
                                .foregroundColor(.white.opacity(0.3))
                        }
                    }
                }

                Spacer()

                // Completion percentage
                VStack(alignment: .trailing, spacing: 2) {
                    Text("\(Int(manager.completionPercentage))%")
                        .font(DesignTokens.monoBold(14))
                        .foregroundColor(DesignTokens.xoGold)
                    Text("complete")
                        .font(DesignTokens.body(10))
                        .foregroundColor(.white.opacity(0.3))
                }
            }

            // Progress bar to next tier
            if let next = manager.playerTier.nextTier {
                let currentFloor = manager.playerTier.pointThreshold
                let nextFloor    = next.pointThreshold
                let progress     = Double(manager.achievementPoints - currentFloor) / Double(nextFloor - currentFloor)

                VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                    GeometryReader { geo in
                        ZStack(alignment: .leading) {
                            RoundedRectangle(cornerRadius: 3)
                                .fill(Color.white.opacity(0.06))
                            RoundedRectangle(cornerRadius: 3)
                                .fill(tierColor(manager.playerTier).opacity(0.7))
                                .frame(width: geo.size.width * CGFloat(min(progress, 1.0)))
                                .animation(.easeInOut(duration: 0.5), value: progress)
                        }
                    }
                    .frame(height: 4)

                    HStack {
                        Text(manager.playerTier.rawValue)
                            .font(DesignTokens.mono(9))
                            .foregroundColor(.white.opacity(0.25))
                        Spacer()
                        Text(next.rawValue)
                            .font(DesignTokens.mono(9))
                            .foregroundColor(.white.opacity(0.25))
                    }
                }
            }
        }
    }

    // MARK: - Category Segment

    private var categorySegment: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: DesignTokens.spacing6) {
                categoryChip(label: "All", symbol: nil, isSelected: selectedCategory == nil) {
                    selectedCategory = nil
                }
                ForEach(AchievementCategory.allCases, id: \.self) { cat in
                    let count = manager.achievementsByCategory[cat]?.filter { $0.isCompleted }.count ?? 0
                    categoryChip(
                        label: cat.displayName,
                        symbol: cat.symbolName,
                        badge: count > 0 ? "\(count)" : nil,
                        isSelected: selectedCategory == cat
                    ) {
                        selectedCategory = (selectedCategory == cat) ? nil : cat
                    }
                }
            }
        }
    }

    @ViewBuilder
    private func categoryChip(
        label: String,
        symbol: String?,
        badge: String? = nil,
        isSelected: Bool,
        action: @escaping () -> Void
    ) -> some View {
        Button(action: action) {
            HStack(spacing: DesignTokens.spacing4) {
                if let sym = symbol {
                    Image(systemName: sym)
                        .font(.system(size: 9))
                }
                Text(label)
                    .font(DesignTokens.bodyMedium(11))
                if let badge {
                    Text(badge)
                        .font(DesignTokens.mono(8))
                        .foregroundColor(isSelected ? DesignTokens.background.opacity(0.6) : .white.opacity(0.3))
                }
            }
            .foregroundColor(isSelected ? DesignTokens.background : .white.opacity(0.55))
            .padding(.horizontal, DesignTokens.spacing12)
            .padding(.vertical, DesignTokens.spacing6)
            .background(isSelected ? DesignTokens.reefJade : Color.white.opacity(0.06))
            .clipShape(Capsule())
        }
    }

    // MARK: - Achievement Card

    private func achievementCard(_ achievement: Achievement) -> some View {
        let borderColor = achievement.isCompleted
            ? tierColor(achievement.tier)
            : Color.white.opacity(0.07)
        let fgOpacity: Double = achievement.isCompleted ? 1.0 : 0.35

        return VStack(alignment: .leading, spacing: DesignTokens.spacing8) {
            HStack(spacing: DesignTokens.spacing6) {
                // Tier icon
                Image(systemName: achievement.tier.symbolName)
                    .font(.system(size: 14))
                    .foregroundColor(achievement.isCompleted ? tierColor(achievement.tier) : .white.opacity(0.2))

                Spacer()

                // Category icon (small, secondary)
                Image(systemName: achievement.category.symbolName)
                    .font(.system(size: 10))
                    .foregroundColor(.white.opacity(0.2))
            }

            Text(achievement.displayTitle)
                .font(DesignTokens.heading(12))
                .foregroundColor(.white.opacity(fgOpacity))
                .lineLimit(2)
                .fixedSize(horizontal: false, vertical: true)

            Text(achievement.displayDescription)
                .font(DesignTokens.body(10))
                .foregroundColor(.white.opacity(fgOpacity * 0.55))
                .lineLimit(2)
                .fixedSize(horizontal: false, vertical: true)

            Spacer(minLength: 0)

            // Completed date or points value
            if achievement.isCompleted, let date = achievement.completedDate {
                Text(DateFormatter.localizedString(from: date, dateStyle: .short, timeStyle: .none))
                    .font(DesignTokens.mono(8))
                    .foregroundColor(tierColor(achievement.tier).opacity(0.6))
            } else {
                Text("+\(achievement.tier.pointValue) pts")
                    .font(DesignTokens.mono(8))
                    .foregroundColor(.white.opacity(0.2))
            }
        }
        .padding(DesignTokens.spacing12)
        .frame(minHeight: 110)
        .background(achievement.isCompleted ? tierColor(achievement.tier).opacity(0.06) : DesignTokens.panelBackground)
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .stroke(borderColor, lineWidth: achievement.isCompleted ? 1.5 : 1)
        )
        .clipShape(RoundedRectangle(cornerRadius: 10))
    }

    // MARK: - Tier Color

    private func tierColor(_ tier: AchievementTier) -> Color {
        switch tier {
        case .bronze:   return DesignTokens.cosmeticFossilized
        case .silver:   return Color(hex: "C0C0C0")
        case .gold:     return DesignTokens.xoGold
        case .platinum: return DesignTokens.cosmeticPhantom
        case .mythic:   return DesignTokens.legendaryGold
        }
    }

    private func tierColor(_ playerTier: PlayerTier) -> Color {
        switch playerTier {
        case .newcomer:        return .white.opacity(0.4)
        case .bronzeCollector: return DesignTokens.cosmeticFossilized
        case .silverExplorer:  return Color(hex: "C0C0C0")
        case .goldMaster:      return DesignTokens.xoGold
        case .platinumLegend:  return DesignTokens.cosmeticPhantom
        case .mythic:          return DesignTokens.legendaryGold
        }
    }
}

// MARK: - AchievementDetailSheet

/// Bottom sheet showing full requirement + progress for a single achievement.
struct AchievementDetailSheet: View {

    let achievement: Achievement
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        NavigationView {
            ScrollView {
                VStack(alignment: .leading, spacing: DesignTokens.spacing20) {

                    // Icon + tier
                    HStack(spacing: DesignTokens.spacing12) {
                        ZStack {
                            Circle()
                                .fill(tierColor.opacity(0.12))
                                .frame(width: 56, height: 56)
                            Image(systemName: achievement.tier.symbolName)
                                .font(.system(size: 24, weight: .semibold))
                                .foregroundColor(tierColor)
                        }

                        VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                            Text(achievement.tier.rawValue.uppercased())
                                .font(DesignTokens.mono(9))
                                .tracking(2)
                                .foregroundColor(tierColor.opacity(0.7))
                            Text(achievement.displayTitle)
                                .font(DesignTokens.heading(18))
                                .foregroundColor(.white)
                            Text(achievement.category.displayName)
                                .font(DesignTokens.body(11))
                                .foregroundColor(.white.opacity(0.35))
                        }
                    }

                    // Description
                    Text(achievement.displayDescription)
                        .font(DesignTokens.body(14))
                        .foregroundColor(.white.opacity(0.65))
                        .lineSpacing(3)
                        .fixedSize(horizontal: false, vertical: true)

                    // Requirement summary
                    VStack(alignment: .leading, spacing: DesignTokens.spacing6) {
                        Text("REQUIREMENT")
                            .font(DesignTokens.mono(9))
                            .tracking(2)
                            .foregroundColor(.white.opacity(0.2))
                        Text(requirementText)
                            .font(DesignTokens.bodyMedium(13))
                            .foregroundColor(.white.opacity(0.6))
                    }
                    .padding(DesignTokens.spacing12)
                    .background(DesignTokens.panelBackground)
                    .clipShape(RoundedRectangle(cornerRadius: 8))

                    // Reward
                    VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                        Text("REWARD")
                            .font(DesignTokens.mono(9))
                            .tracking(2)
                            .foregroundColor(.white.opacity(0.2))
                        HStack(spacing: DesignTokens.spacing8) {
                            Text("+\(achievement.tier.pointValue)")
                                .font(DesignTokens.monoBold(16))
                                .foregroundColor(DesignTokens.xoGold)
                            Text("points")
                                .font(DesignTokens.body(13))
                                .foregroundColor(.white.opacity(0.4))
                        }
                        if !achievement.reward.description.isEmpty {
                            Text(achievement.reward.description)
                                .font(DesignTokens.body(12))
                                .foregroundColor(.white.opacity(0.4))
                        }
                    }

                    // Completed date
                    if achievement.isCompleted, let date = achievement.completedDate {
                        HStack(spacing: DesignTokens.spacing6) {
                            Image(systemName: "checkmark.circle.fill")
                                .font(.system(size: 13))
                                .foregroundColor(tierColor)
                            Text("Completed \(DateFormatter.localizedString(from: date, dateStyle: .long, timeStyle: .short))")
                                .font(DesignTokens.body(12))
                                .foregroundColor(.white.opacity(0.4))
                        }
                    }

                    Spacer(minLength: DesignTokens.spacing40)
                }
                .padding(DesignTokens.spacing20)
            }
            .background(DesignTokens.background)
            .navigationTitle("")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Done") { dismiss() }
                        .font(DesignTokens.bodyMedium(15))
                        .foregroundColor(DesignTokens.reefJade)
                }
            }
        }
        .preferredColorScheme(.dark)
    }

    private var tierColor: Color {
        switch achievement.tier {
        case .bronze:   return DesignTokens.cosmeticFossilized
        case .silver:   return Color(hex: "C0C0C0")
        case .gold:     return DesignTokens.xoGold
        case .platinum: return DesignTokens.cosmeticPhantom
        case .mythic:   return DesignTokens.legendaryGold
        }
    }

    private var requirementText: String {
        switch achievement.requirement {
        case .specimenCount(let n):      return "Own \(n) specimens total."
        case .uniqueSubtypes(let n):     return "Discover \(n) unique specimen types."
        case .breedingGeneration(let g): return "Reach breeding generation \(g)."
        case .breedingTotal(let n):      return "Breed \(n) specimens."
        case .diveHighScore(let s):      return "Achieve a Dive score of \(s)."
        case .consecutiveDays(let d):    return "Visit the reef \(d) days in a row."
        case .couplingTypesUsed(let n):  return "Use \(n) different coupling types."
        case .arrangementLength(let s):  return "Record an arrangement of \(s) seconds."
        case .seasonalComplete(let n):   return "Complete \(n) seasonal challenges."
        case .loreDiscovered(let n):     return "Discover \(n) lore fragments."
        case .elderSpecimens(let n):     return "Raise \(n) Elder specimens."
        case .totalPlayTime(let h):      return "Play for \(h) hours total."
        case .exportedMIDI(let n):       return "Export \(n) MIDI recordings."
        case .tradesCompleted(let n):    return "Complete \(n) reef trades."
        case .diveDepth(let d):          return "Reach \(d)m dive depth."
        case .secretCondition:           return "A hidden condition. Keep exploring."
        }
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    AchievementGalleryView()
        .background(DesignTokens.background.ignoresSafeArea())
}
#endif
