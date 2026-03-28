import SwiftUI

/// Journey/energy/streak indicators and daily challenge bar.
/// Default state: compact single row (~24pt) showing bolt/energy, flame/streak, star/challenges.
/// Tap the row to expand and reveal full progress bars, CLAIM button, and challenge descriptions.
/// challengeManager is passed in from ReefTab so PlayKeyboard callbacks can also call it.
/// Owns: streakManager. Reads energyManager singleton.
struct ReefIndicatorBar: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore

    // Passed in from ReefTab so the PlayKeyboard callback can also increment progress
    @ObservedObject var challengeManager: DailyChallengeManager

    @ObservedObject private var streakManager = StreakManager.shared
    @ObservedObject private var energyManager = ReefEnergyManager.shared

    @State private var showStreakReward = false
    @State private var lastReward: StreakReward = .none
    @State private var isExpanded = false

    var body: some View {
        VStack(spacing: 0) {
            // ── Compact row — always visible, ~24pt tall ──────────────────────
            Button(action: {
                withAnimation(.easeInOut(duration: 0.2)) {
                    isExpanded.toggle()
                }
            }) {
                HStack(spacing: 12) {
                    // Energy
                    HStack(spacing: 3) {
                        Image(systemName: "bolt.fill")
                            .font(.system(size: 8))
                            .foregroundColor(DesignTokens.xoGold)
                        Text("\(energyManager.currentEnergy)")
                            .font(DesignTokens.mono(9))
                            .foregroundColor(DesignTokens.xoGold)
                    }

                    // Streak
                    HStack(spacing: 3) {
                        Image(systemName: "flame.fill")
                            .font(.system(size: 8))
                            .foregroundColor(
                                streakManager.currentStreak >= 7
                                    ? DesignTokens.xoGold
                                    : DesignTokens.streakOrange.opacity(0.6)
                            )
                        Text("\(streakManager.currentStreak)")
                            .font(DesignTokens.mono(9))
                            .foregroundColor(.white.opacity(0.5))
                        // Unclaimed-reward dot
                        if !streakManager.todayRewardClaimed {
                            Circle()
                                .fill(DesignTokens.streakOrange)
                                .frame(width: 4, height: 4)
                        }
                    }

                    // Challenges progress
                    HStack(spacing: 3) {
                        Image(systemName: "star.fill")
                            .font(.system(size: 8))
                            .foregroundColor(DesignTokens.xoGold.opacity(0.5))
                        Text("\(challengeManager.completedCount)/\(challengeManager.challenges.count)")
                            .font(DesignTokens.mono(9))
                            .foregroundColor(.white.opacity(0.3))
                    }

                    Spacer()

                    // Expand chevron
                    Image(systemName: isExpanded ? "chevron.up" : "chevron.down")
                        .font(.system(size: 8))
                        .foregroundColor(.white.opacity(0.2))
                }
            }
            .buttonStyle(.plain)
            .padding(.horizontal, 20)
            .padding(.vertical, 6)   // 6+6 = 12pt vertical + content ≈ 24pt total

            // ── Expanded section — tap to reveal, adds ~80pt ──────────────────
            if isExpanded {
                VStack(spacing: 6) {
                    // Full energy bar
                    HStack(spacing: 6) {
                        Image(systemName: "bolt.fill")
                            .font(.system(size: 9))
                            .foregroundColor(DesignTokens.xoGold)
                        Text("\(energyManager.currentEnergy)")
                            .font(DesignTokens.monoBold(11))
                            .foregroundColor(DesignTokens.xoGold)
                        Text("/ \(ReefEnergyManager.maxEnergy)")
                            .font(DesignTokens.mono(9))
                            .foregroundColor(.white.opacity(0.2))
                        if energyManager.dailyEnergyEarned < ReefEnergyManager.dailyEarnCap {
                            GeometryReader { geo in
                                ZStack(alignment: .leading) {
                                    RoundedRectangle(cornerRadius: 2)
                                        .fill(Color.white.opacity(0.06))
                                    RoundedRectangle(cornerRadius: 2)
                                        .fill(DesignTokens.xoGold.opacity(0.35))
                                        .frame(
                                            width: geo.size.width
                                                * CGFloat(energyManager.dailyEnergyEarned)
                                                / CGFloat(ReefEnergyManager.dailyEarnCap)
                                        )
                                }
                            }
                            .frame(width: 40, height: 3)
                        } else {
                            Image(systemName: "checkmark.circle.fill")
                                .font(.system(size: 8))
                                .foregroundColor(DesignTokens.reefJade.opacity(0.5))
                        }
                    }

                    // Full streak row + CLAIM button
                    HStack(spacing: 6) {
                        Image(systemName: "flame.fill")
                            .font(.system(size: 10))
                            .foregroundColor(
                                streakManager.currentStreak >= 7
                                    ? DesignTokens.xoGold
                                    : DesignTokens.streakOrange.opacity(0.6)
                            )
                        Text("\(streakManager.currentStreak)")
                            .font(DesignTokens.monoBold(12))
                            .foregroundColor(.white.opacity(0.7))

                        if !streakManager.todayRewardClaimed {
                            Button(action: {
                                lastReward = streakManager.claimReward()
                                if lastReward.xpAmount > 0 {
                                    for (index, spec) in reefStore.specimens.enumerated() {
                                        if spec != nil {
                                            audioEngine.awardBulkXP(slotIndex: index, amount: lastReward.xpAmount)
                                        }
                                    }
                                    showStreakReward = true
                                }
                            }) {
                                Text("CLAIM")
                                    .font(DesignTokens.monoBold(8))
                                    .foregroundColor(.white)
                                    .padding(.horizontal, 8)
                                    .padding(.vertical, 3)
                                    .background(RoundedRectangle(cornerRadius: 4).fill(DesignTokens.streakOrange))
                            }
                        }

                        Spacer()

                        Text("next milestone: day \(streakManager.nextMilestone)")
                            .font(DesignTokens.mono(8))
                            .foregroundColor(.white.opacity(0.2))
                    }

                    // Challenge descriptions + mini progress bars
                    HStack(spacing: 8) {
                        Image(systemName: "star.fill")
                            .font(.system(size: 9))
                            .foregroundColor(DesignTokens.xoGold)

                        ForEach(challengeManager.challenges) { challenge in
                            VStack(spacing: 2) {
                                Text(challenge.description)
                                    .font(DesignTokens.body(8))
                                    .foregroundColor(
                                        challenge.isComplete
                                            ? DesignTokens.reefJade
                                            : .white.opacity(0.4)
                                    )
                                    .strikethrough(challenge.isComplete)
                                    .lineLimit(1)

                                GeometryReader { geo in
                                    ZStack(alignment: .leading) {
                                        RoundedRectangle(cornerRadius: 1)
                                            .fill(Color.white.opacity(0.06))
                                        RoundedRectangle(cornerRadius: 1)
                                            .fill(
                                                challenge.isComplete
                                                    ? DesignTokens.reefJade
                                                    : DesignTokens.xoGold.opacity(0.5)
                                            )
                                            .frame(
                                                width: geo.size.width
                                                    * CGFloat(challenge.progress)
                                                    / CGFloat(challenge.target)
                                            )
                                    }
                                }
                                .frame(height: 2)
                            }
                        }
                    }
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 6)
                .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
        .onAppear {
            energyManager.resetDailyIfNeeded()
        }
        .alert("Streak Reward!", isPresented: $showStreakReward) {
            Button("Nice!") {}
        } message: {
            Text("Day \(streakManager.currentStreak): \(lastReward.description)")
        }
    }
}
