import SwiftUI

/// Player profile — a summary of the reef builder's identity and journey
struct ProfileView: View {
    @EnvironmentObject var reefStore: ReefStore

    // Use a single StreakManager instance for the whole view lifetime
    @StateObject private var streakManager = StreakManager()
    @ObservedObject private var masteryManager = MasteryManager.shared

    var body: some View {
        ScrollView {
            VStack(spacing: 20) {
                // Profile header
                VStack(spacing: 8) {
                    // Reef name as identity
                    Text(reefStore.reefName)
                        .font(.custom("SpaceGrotesk-Bold", size: 24))
                        .foregroundColor(.white)

                    // Mastery title
                    Text(masteryManager.masteryTitle)
                        .font(.custom("JetBrainsMono-Regular", size: 12))
                        .foregroundColor(Color(hex: "E9C46A"))

                    // Total depth
                    Text("\(reefStore.totalDiveDepth)m total depth")
                        .font(.custom("JetBrainsMono-Regular", size: 11))
                        .foregroundColor(.white.opacity(0.4))
                }
                .padding(.top, 20)

                // Stats grid
                LazyVGrid(columns: [
                    GridItem(.flexible()), GridItem(.flexible()),
                    GridItem(.flexible()), GridItem(.flexible())
                ], spacing: 12) {
                    profileStat(
                        value: "\(ReefStatsTracker.shared.value(for: .notesPlayed))",
                        label: "Notes",
                        icon: "music.note"
                    )
                    profileStat(
                        value: "\(ReefStatsTracker.shared.value(for: .specimensCaught))",
                        label: "Caught",
                        icon: "leaf.fill"
                    )
                    profileStat(
                        value: "\(ReefStatsTracker.shared.value(for: .divesCompleted))",
                        label: "Dives",
                        icon: "arrow.down.to.line"
                    )
                    profileStat(
                        value: "\(ReefStatsTracker.shared.value(for: .wiresCreated))",
                        label: "Wires",
                        icon: "link"
                    )
                }
                .padding(.horizontal, 20)

                // Favorite category
                favoriteCategorySection

                // Most played specimen
                mostPlayedSection

                // Streak info
                streakSection

                // Journey progress
                journeySection

                Spacer(minLength: 40)
            }
        }
        .background(Color(hex: "0E0E10").ignoresSafeArea())
    }

    // MARK: - Subviews

    private func profileStat(value: String, label: String, icon: String) -> some View {
        VStack(spacing: 4) {
            Image(systemName: icon)
                .font(.system(size: 14))
                .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
            Text(value)
                .font(.custom("JetBrainsMono-Bold", size: 16))
                .foregroundColor(.white)
            Text(label)
                .font(.custom("Inter-Regular", size: 9))
                .foregroundColor(.white.opacity(0.3))
        }
    }

    private var favoriteCategorySection: some View {
        let specimens = reefStore.specimens.compactMap { $0 }
        let categoryCounts = Dictionary(grouping: specimens, by: { $0.category })
        let favorite = categoryCounts.max(by: { $0.value.count < $1.value.count })

        return VStack(spacing: 4) {
            Text("FAVORITE CATEGORY")
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))
            if let fav = favorite {
                Text(fav.key.rawValue.capitalized)
                    .font(.custom("SpaceGrotesk-Bold", size: 16))
                    .foregroundColor(categoryColor(for: fav.key))
                Text("\(fav.value.count) in reef")
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.3))
            } else {
                Text("None yet")
                    .font(.custom("SpaceGrotesk-Bold", size: 16))
                    .foregroundColor(.white.opacity(0.3))
            }
        }
    }

    private var mostPlayedSection: some View {
        let specimens = reefStore.specimens.compactMap { $0 }
        let mostPlayed = specimens.max(by: { $0.totalPlaySeconds < $1.totalPlaySeconds })

        return VStack(spacing: 4) {
            Text("MOST PLAYED")
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))
            if let spec = mostPlayed, spec.totalPlaySeconds > 0 {
                HStack(spacing: 8) {
                    SpecimenSprite(subtype: spec.subtype, category: spec.category, size: 32)
                    VStack(alignment: .leading, spacing: 2) {
                        Text(spec.creatureName)
                            .font(.custom("SpaceGrotesk-Bold", size: 14))
                            .foregroundColor(.white)
                        Text(formatTime(spec.totalPlaySeconds))
                            .font(.custom("JetBrainsMono-Regular", size: 10))
                            .foregroundColor(.white.opacity(0.4))
                    }
                }
            } else {
                Text("Play a specimen to track time")
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.3))
            }
        }
    }

    private var streakSection: some View {
        VStack(spacing: 4) {
            Text("STREAK")
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))
            HStack(spacing: 4) {
                Image(systemName: "flame.fill")
                    .foregroundColor(Color(hex: "FF6B35"))
                Text("\(streakManager.currentStreak) days")
                    .font(.custom("JetBrainsMono-Bold", size: 14))
                    .foregroundColor(.white)
            }
            Text("Longest: \(streakManager.longestStreak) days")
                .font(.custom("Inter-Regular", size: 10))
                .foregroundColor(.white.opacity(0.3))
        }
    }

    private var journeySection: some View {
        let allMaxed = reefStore.specimens.compactMap { $0 }.filter { $0.level >= 10 }.count
        return VStack(spacing: 4) {
            Text("JOURNEY")
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))
            Text("\(allMaxed)/16 evolved")
                .font(.custom("JetBrainsMono-Regular", size: 12))
                .foregroundColor(.white.opacity(0.5))
        }
    }

    // MARK: - Helpers

    private func formatTime(_ seconds: Double) -> String {
        if seconds < 60 { return "\(Int(seconds))s" }
        if seconds < 3600 { return "\(Int(seconds / 60))m" }
        return String(format: "%.1fh", seconds / 3600)
    }

    private func categoryColor(for category: SpecimenCategory) -> Color {
        switch category {
        case .source:    return Color(hex: "3380FF")   // Shells — blue
        case .processor: return Color(hex: "FF4D4D")   // Coral — red
        case .modulator: return Color(hex: "4DCC4D")   // Currents — green
        case .effect:    return Color(hex: "B34DFF")   // Tide Pools — purple
        }
    }
}
