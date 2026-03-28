import SwiftUI

/// Journey/energy/streak indicators and daily challenge bar.
/// challengeManager is passed in from ReefTab so PlayKeyboard callbacks can also call it.
/// Owns: streakManager. Reads energyManager singleton.
struct ReefIndicatorBar: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore

    // Passed in from ReefTab so the PlayKeyboard callback can also increment progress
    @ObservedObject var challengeManager: DailyChallengeManager

    @StateObject private var streakManager = StreakManager()
    @ObservedObject private var energyManager = ReefEnergyManager.shared

    @State private var showStreakReward = false
    @State private var lastReward: StreakReward = .none

    var body: some View {
        VStack(spacing: 0) {
            // Daily challenges bar
            HStack(spacing: 8) {
                Image(systemName: "star.fill")
                    .font(.system(size: 9))
                    .foregroundColor(DesignTokens.xoGold)

                ForEach(challengeManager.challenges) { challenge in
                    VStack(spacing: 2) {
                        Text(challenge.description)
                            .font(.custom("Inter-Regular", size: 8))
                            .foregroundColor(challenge.isComplete ? DesignTokens.reefJade : .white.opacity(0.4))
                            .strikethrough(challenge.isComplete)
                            .lineLimit(1)

                        // Mini progress bar
                        GeometryReader { geo in
                            ZStack(alignment: .leading) {
                                RoundedRectangle(cornerRadius: 1)
                                    .fill(Color.white.opacity(0.06))
                                RoundedRectangle(cornerRadius: 1)
                                    .fill(challenge.isComplete ? DesignTokens.reefJade : DesignTokens.xoGold.opacity(0.5))
                                    .frame(width: geo.size.width * CGFloat(challenge.progress) / CGFloat(challenge.target))
                            }
                        }
                        .frame(height: 2)
                    }
                }
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 4)

            // Reef Energy currency display
            HStack(spacing: 6) {
                Image(systemName: "bolt.fill")
                    .font(.system(size: 9))
                    .foregroundColor(DesignTokens.xoGold)
                Text("\(energyManager.currentEnergy)")
                    .font(.custom("JetBrainsMono-Bold", size: 11))
                    .foregroundColor(DesignTokens.xoGold)
                Text("/ \(ReefEnergyManager.maxEnergy)")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.2))
                // Daily earn progress pill — shows how close to the 50/day cap
                if energyManager.dailyEnergyEarned < ReefEnergyManager.dailyEarnCap {
                    GeometryReader { geo in
                        ZStack(alignment: .leading) {
                            RoundedRectangle(cornerRadius: 2)
                                .fill(Color.white.opacity(0.06))
                            RoundedRectangle(cornerRadius: 2)
                                .fill(DesignTokens.xoGold.opacity(0.35))
                                .frame(width: geo.size.width * CGFloat(energyManager.dailyEnergyEarned) / CGFloat(ReefEnergyManager.dailyEarnCap))
                        }
                    }
                    .frame(width: 40, height: 3)
                } else {
                    Image(systemName: "checkmark.circle.fill")
                        .font(.system(size: 8))
                        .foregroundColor(DesignTokens.reefJade.opacity(0.5))
                }
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 4)

            // Streak indicator
            HStack(spacing: 6) {
                Image(systemName: "flame.fill")
                    .font(.system(size: 10))
                    .foregroundColor(streakManager.currentStreak >= 7 ? DesignTokens.xoGold : DesignTokens.streakOrange.opacity(0.6))

                Text("\(streakManager.currentStreak)")
                    .font(.custom("JetBrainsMono-Bold", size: 12))
                    .foregroundColor(.white.opacity(0.7))

                if !streakManager.todayRewardClaimed {
                    Button(action: {
                        lastReward = streakManager.claimReward()
                        if lastReward.xpAmount > 0 {
                            // Distribute XP to all reef specimens
                            for (index, spec) in reefStore.specimens.enumerated() {
                                if spec != nil {
                                    audioEngine.awardBulkXP(slotIndex: index, amount: lastReward.xpAmount)
                                }
                            }
                            showStreakReward = true
                        }
                    }) {
                        Text("CLAIM")
                            .font(.custom("JetBrainsMono-Bold", size: 8))
                            .foregroundColor(.white)
                            .padding(.horizontal, 8)
                            .padding(.vertical, 3)
                            .background(RoundedRectangle(cornerRadius: 4).fill(DesignTokens.streakOrange))
                    }
                }

                Spacer()

                Text("next milestone: day \(streakManager.nextMilestone)")
                    .font(.custom("JetBrainsMono-Regular", size: 8))
                    .foregroundColor(.white.opacity(0.2))
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 4)
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
