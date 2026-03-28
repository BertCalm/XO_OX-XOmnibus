import SwiftUI
import SpriteKit

// MARK: - Depth Zone

enum DepthZone: String {
    case sunlit = "Sunlit Zone"       // 0-200m — bright, simple, pentatonic
    case twilight = "Twilight Zone"    // 200-600m — darker, add minor notes
    case midnight = "Midnight Zone"   // 600-1000m — chromatic, dense
    case abyssal = "Abyssal Zone"     // 1000m+ — deep, sparse, mysterious

    static func at(depth: Int) -> DepthZone {
        switch depth {
        case 0..<200:    return .sunlit
        case 200..<600:  return .twilight
        case 600..<1000: return .midnight
        default:         return .abyssal
        }
    }

    /// Scale intervals for this zone
    var scaleIntervals: [Int] {
        switch self {
        case .sunlit:   return [0, 2, 4, 7, 9]                              // Major pentatonic
        case .twilight: return [0, 2, 3, 5, 7, 8, 10]                       // Natural minor
        case .midnight: return [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]      // Chromatic
        case .abyssal:  return [0, 3, 5, 7, 10]                             // Minor pentatonic (sparse)
        }
    }

    /// Octave range for this zone (deeper = lower)
    var octaveRange: ClosedRange<Int> {
        switch self {
        case .sunlit:   return 4...5
        case .twilight: return 3...5
        case .midnight: return 2...5
        case .abyssal:  return 1...3   // Very low
        }
    }

    /// Background gradient bottom color tint
    var bgColor: String {
        switch self {
        case .sunlit:   return "0A1018"
        case .twilight: return "080810"
        case .midnight: return "050508"
        case .abyssal:  return "020204"
        }
    }
}

// MARK: - DiveTab

struct DiveTab: View {
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var audioEngine: AudioEngineManager
    @State private var isDiving = false
    @State private var diveProgress: Float = 0  // 0-1
    @State private var diveDepth: Int = 0       // Meters
    @State private var diveScore: Int = 0
    @State private var divePhase: DivePhase = .ready
    @State private var activeNotes: Set<Int> = []
    @State private var diveToken: TickToken?
    @State private var lastZone: DepthZone = .sunlit
    @State private var reefBonus: Float = 1.0
    @State private var availableSources: [Int] = []
    @State private var currentSourceIndex = 0
    /// Incremented each time a dive starts; stale closures check this before firing.
    @State private var diveGeneration = 0
    @State private var isCollectingRewards = false

    // MARK: - SpriteKit + Composer

    @State private var diveScene: DiveScene?
    @State private var composer: DiveComposer?

    // Engagement tracking (per-tick frames)
    @State private var activeFrames = 0
    @State private var totalFrames = 0

    // Tilt modulation
    @StateObject private var motionController = MotionController()

    // Dive history & high score
    @StateObject private var diveHistory = DiveHistoryManager()
    @State private var isNewHighScore = false
    @State private var showHistory = false

    // Weekly challenges
    @StateObject private var weeklyChallenges = WeeklyChallengeManager()

    enum DivePhase {
        case ready      // Before dive
        case diving     // Active dive
        case surfacing  // Dive complete, showing score
    }

    var body: some View {
        ZStack {
            // Background — changes color as you dive into deeper zones
            let zone = DepthZone.at(depth: diveDepth)
            LinearGradient(
                colors: [DesignTokens.darkBackground, Color(hex: zone.bgColor)],
                startPoint: .top, endPoint: .bottom
            )
            .ignoresSafeArea()
            .animation(.easeInOut(duration: 2.0), value: zone.rawValue)

            VStack(spacing: 24) {
                // Header — hidden during active dive (SpriteKit HUD covers it)
                if divePhase != .diving {
                    HStack {
                        HStack(spacing: 8) {
                            Text("THE DIVE")
                                .font(DesignTokens.heading(18))
                                .foregroundColor(.white)
                            let sources = reefStore.specimens.compactMap { $0 }.filter { $0.category == .source }.count
                            let total = reefStore.diveEligibleCount
                            Text("\(sources) src · \(total) total")
                                .font(DesignTokens.mono(9))
                                .foregroundColor(.white.opacity(0.2))
                        }
                        Spacer()
                        Text("\(diveDepth)m")
                            .font(DesignTokens.monoBold(16))
                            .foregroundColor(DesignTokens.reefJade)
                    }
                    .padding(.horizontal, 20)
                    .padding(.top, 12)
                }

                Spacer()

                switch divePhase {
                case .ready:
                    readyView
                case .diving:
                    divingView
                case .surfacing:
                    surfacingView
                }

                Spacer()
            }
        }
        .onDisappear {
            if isDiving { endDive() }
        }
    }

    // MARK: - Ready

    private var readyView: some View {
        VStack(spacing: 20) {
            Image(systemName: "arrow.down.to.line")
                .font(.system(size: 48))
                .foregroundColor(DesignTokens.reefJade.opacity(0.4))

            Text("Dive into your reef")
                .font(DesignTokens.heading(20))
                .foregroundColor(.white)

            Text("60 seconds of generative performance.\nYour reef plays itself. Deeper = more complex.")
                .font(DesignTokens.body(13))
                .foregroundColor(.white.opacity(0.4))
                .multilineTextAlignment(.center)
                .padding(.horizontal, 40)

            // Requirements
            let sourceCount = reefStore.specimens.compactMap { $0 }.filter { $0.category == .source }.count
            let totalCount = reefStore.diveEligibleCount

            VStack(spacing: 4) {
                requirementRow("Sources", count: sourceCount, required: 1)
                requirementRow("Total specimens", count: totalCount, required: 4)
            }
            .padding(.horizontal, 40)

            if sourceCount < 1 {
                Text("Visit the Catch tab to find your first specimen")
                    .font(DesignTokens.body(11))
                    .foregroundColor(.white.opacity(0.3))
                    .multilineTextAlignment(.center)
                    .padding(.horizontal, 40)
            }

            Button(action: startDive) {
                Text("DIVE")
                    .font(DesignTokens.heading(18))
                    .tracking(3)
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity)
                    .frame(height: 56)
                    .background(
                        RoundedRectangle(cornerRadius: 28)
                            .fill(totalCount >= 4 ? DesignTokens.reefJade : Color(hex: "1A1A1C"))
                    )
            }
            .disabled(totalCount < 4)
            .padding(.horizontal, 40)

            // Dive stats (if any dives completed)
            if diveHistory.totalDives > 0 {
                VStack(spacing: 4) {
                    HStack(spacing: 16) {
                        statColumn("High Score", value: "\(diveHistory.highScore)")
                        statColumn("Deepest", value: "\(diveHistory.deepestDive)m")
                        statColumn("Dives", value: "\(diveHistory.totalDives)")
                    }
                }
                .padding(.horizontal, 40)
                .padding(.bottom, 8)
            }

            // History link
            if !diveHistory.records.isEmpty {
                Button(action: { showHistory = true }) {
                    Text("View History")
                        .font(DesignTokens.body(12))
                        .foregroundColor(.white.opacity(0.4))
                }
            }

            // Weekly challenges
            if !weeklyChallenges.challenges.isEmpty {
                VStack(alignment: .leading, spacing: 6) {
                    HStack {
                        Text("WEEKLY CHALLENGES")
                            .font(DesignTokens.mono(9))
                            .tracking(1.5)
                            .foregroundColor(DesignTokens.xoGold)
                        Spacer()
                        Text("\(weeklyChallenges.completedCount)/\(weeklyChallenges.challenges.count)")
                            .font(DesignTokens.mono(9))
                            .foregroundColor(.white.opacity(0.3))
                    }

                    ForEach(weeklyChallenges.challenges) { challenge in
                        HStack(spacing: 8) {
                            Image(systemName: challenge.completed ? "checkmark.circle.fill" : "circle")
                                .font(.system(size: 12))
                                .foregroundColor(challenge.completed ? DesignTokens.reefJade : .white.opacity(0.2))

                            VStack(alignment: .leading, spacing: 1) {
                                Text(challenge.title)
                                    .font(DesignTokens.bodyMedium(11))
                                    .foregroundColor(challenge.completed ? .white.opacity(0.4) : .white.opacity(0.7))
                                    .strikethrough(challenge.completed)
                                Text(challenge.description)
                                    .font(DesignTokens.body(9))
                                    .foregroundColor(.white.opacity(0.25))
                            }

                            Spacer()

                            Text(challenge.progressString)
                                .font(DesignTokens.mono(9))
                                .foregroundColor(.white.opacity(0.3))
                        }
                    }
                }
                .padding(.horizontal, 20)
                .padding(.top, 8)
            }
        }
        .sheet(isPresented: $showHistory) {
            DiveHistoryList(history: diveHistory)
        }
    }

    private func requirementRow(_ label: String, count: Int, required: Int) -> some View {
        HStack {
            Text(label)
                .font(DesignTokens.body(11))
                .foregroundColor(.white.opacity(0.4))
            Spacer()
            Text("\(count)/\(required)")
                .font(DesignTokens.mono(11))
                .foregroundColor(count >= required ? DesignTokens.reefJade : DesignTokens.errorRed)
            Image(systemName: count >= required ? "checkmark.circle.fill" : "xmark.circle")
                .font(.system(size: 10))
                .foregroundColor(count >= required ? DesignTokens.reefJade : DesignTokens.errorRed.opacity(0.5))
        }
    }

    private func statColumn(_ label: String, value: String) -> some View {
        VStack(spacing: 2) {
            Text(value)
                .font(DesignTokens.monoBold(16))
                .foregroundColor(DesignTokens.reefJade)
            Text(label)
                .font(DesignTokens.body(9))
                .foregroundColor(.white.opacity(0.3))
        }
        .frame(maxWidth: .infinity)
    }

    // MARK: - Diving

    private var divingView: some View {
        ZStack {
            // Full-screen SpriteKit game
            if let scene = diveScene {
                SpriteView(scene: scene)
                    .ignoresSafeArea()
            }

            // HUD overlay (non-interactive, passes touches through)
            VStack {
                // Top bar: zone name + depth + score
                HStack {
                    Text(DepthZone.at(depth: diveDepth).rawValue)
                        .font(DesignTokens.mono(10))
                        .foregroundColor(.white.opacity(0.5))
                    Spacer()
                    Text("\(diveDepth)m")
                        .font(DesignTokens.monoBold(14))
                        .foregroundColor(.white)
                    Spacer()
                    Text("\(diveScore)")
                        .font(DesignTokens.monoBold(14))
                        .foregroundColor(DesignTokens.xoGold)
                }
                .padding(.horizontal, 20)
                .padding(.top, 8)

                DegradationWaveform(
                    degradation: composer?.degradation ?? 0,
                    beat: composer?.currentBeat ?? 0
                )
                .opacity(isDiving ? 1 : 0)

                Spacer()

                // Bottom: progress bar + time remaining
                VStack(spacing: 4) {
                    // Degradation indicator (when > 0)
                    if composer?.degradation ?? 0 > 0 {
                        Text("SIGNAL DEGRADED")
                            .font(DesignTokens.mono(8))
                            .foregroundColor(DesignTokens.errorRed.opacity(0.6))
                    }

                    // Progress bar
                    GeometryReader { geo in
                        ZStack(alignment: .leading) {
                            RoundedRectangle(cornerRadius: 2)
                                .fill(Color.white.opacity(0.1))
                            RoundedRectangle(cornerRadius: 2)
                                .fill(DesignTokens.reefJade.opacity(0.6))
                                .frame(width: geo.size.width * CGFloat(diveProgress))
                        }
                    }
                    .frame(height: 4)
                    .padding(.horizontal, 40)

                    Text("\(Int((1.0 - diveProgress) * 60))s")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.3))
                }
                .padding(.bottom, 16)
            }
            .allowsHitTesting(false) // Touch passes through to SpriteKit
        }
    }

    // MARK: - Surfacing (Results)

    private var surfacingView: some View {
        let interactionPercent = totalFrames > 0
            ? Int(Float(activeFrames) / Float(totalFrames) * 100)
            : 0
        let interactionBonus = diveScore * interactionPercent / 200  // Up to 50% bonus at 100% engagement
        let finalScore = diveScore + interactionBonus

        return VStack(spacing: 16) {
            Text("SURFACED")
                .font(DesignTokens.heading(24))
                .foregroundColor(.white)

            // New high score badge
            if isNewHighScore {
                Text("NEW HIGH SCORE!")
                    .font(DesignTokens.heading(16))
                    .foregroundColor(DesignTokens.xoGold)
                    .padding(.bottom, 4)
            }

            // Final stats
            VStack(spacing: 8) {
                statRow("Depth reached", value: "\(diveDepth)m")
                statRow("Base score", value: "\(diveScore)")
                statRow("Interaction", value: "\(interactionPercent)%")
                statRow("Interaction Bonus", value: "+\(interactionBonus)")
                statRow("Final Score", value: "\(finalScore)")
                statRow("Duration", value: "60s")
            }
            .padding(.horizontal, 40)

            // XP reward
            let xpReward = finalScore / 10
            Text("+\(xpReward) XP to all reef specimens")
                .font(DesignTokens.mono(12))
                .foregroundColor(DesignTokens.xoGold)

            Button(action: {
                guard !isCollectingRewards else { return }
                isCollectingRewards = true

                // Award XP to all reef specimens
                for (index, spec) in reefStore.specimens.enumerated() {
                    if spec != nil {
                        audioEngine.awardBulkXP(slotIndex: index, amount: xpReward)
                    }
                }
                // Update total dive depth
                reefStore.totalDiveDepth += diveDepth
                reefStore.save()
                divePhase = .ready
                isCollectingRewards = false // Reset for next dive
            }) {
                Text("COLLECT REWARDS")
                    .font(DesignTokens.heading(16))
                    .tracking(2)
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity)
                    .frame(height: 50)
                    .background(RoundedRectangle(cornerRadius: 25).fill(DesignTokens.reefJade))
            }
            .disabled(isCollectingRewards)
            .padding(.horizontal, 40)
        }
    }

    private func statRow(_ label: String, value: String) -> some View {
        HStack {
            Text(label)
                .font(DesignTokens.body(12))
                .foregroundColor(.white.opacity(0.4))
            Spacer()
            Text(value)
                .font(DesignTokens.monoBold(14))
                .foregroundColor(.white)
        }
    }

    // MARK: - Dive Logic

    private func startDive() {
        guard !isDiving else { return }
        isDiving = true
        divePhase = .diving
        diveScore = 0
        diveDepth = 0
        diveProgress = 0
        diveGeneration += 1
        lastZone = .sunlit
        isNewHighScore = false
        activeFrames = 0
        totalFrames = 0

        // Start tilt modulation for dive expression
        motionController.start()

        // Calculate reef bonus
        let specimenCount = reefStore.specimens.compactMap { $0 }.count
        reefBonus = 1.0 + Float(specimenCount) * 0.15

        // Find available source specimens for the composer
        availableSources = reefStore.specimens.enumerated()
            .compactMap { $0.element?.category == .source ? $0.offset : nil }
        currentSourceIndex = 0

        // Get specimen stats for composer influence
        let stats = activeSpecimenStats()

        // Create composer
        let newComposer = DiveComposer()
        newComposer.specimenPulse = Float(stats.pulse) / 100.0
        newComposer.specimenWarmth = Float(stats.warmth) / 100.0
        newComposer.specimenReflexes = Float(stats.reflexes) / 100.0
        newComposer.specimenGrit = Float(stats.grit) / 100.0

        // Wire composer audio output to ObrixBridge
        newComposer.onNoteOn = { note, velocity in
            ObrixBridge.shared()?.note(on: Int32(note), velocity: velocity)
        }
        newComposer.onNoteOff = { note in
            ObrixBridge.shared()?.noteOff(Int32(note))
        }
        newComposer.onBeat = { [weak self] beat in
            guard let self else { return }
            HapticEngine.diveBeat()
            self.diveScene?.triggerBeatPulse()
        }
        newComposer.onDegradation = { [weak self] _ in
            // Trigger view update so HUD reflects current degradation level
            _ = self?.composer?.degradation
        }

        composer = newComposer

        // Configure engine with first source's chain
        if let sourceSlot = availableSources.first {
            audioEngine.applySlotChain(slotIndex: sourceSlot, reefStore: reefStore)
        }

        // Create SpriteKit scene
        let scene = DiveScene(size: CGSize(width: UIScreen.main.bounds.width,
                                            height: UIScreen.main.bounds.height * 0.85))
        scene.scaleMode = .aspectFill
        scene.currentZone = .sunlit

        // Wire scene callbacks to composer + scoring
        scene.onPlayerPositionChanged = { [weak newComposer] x in
            newComposer?.playerX = x
        }
        scene.onPlayerVelocityChanged = { [weak newComposer] v in
            newComposer?.playerDodgeVelocity = v
        }
        scene.onObstacleProximity = { [weak newComposer] p in
            newComposer?.obstacleProximity = p
        }
        scene.onHit = { [weak self, weak newComposer] in
            newComposer?.registerHit()
            self?.diveScore = max(0, (self?.diveScore ?? 0) - 50)
        }
        scene.onCollect = { [weak self, weak newComposer] in
            newComposer?.registerCollectible()
            let zone = DepthZone.at(depth: self?.diveDepth ?? 0)
            let zoneMultiplier: Int
            switch zone {
            case .sunlit:   zoneMultiplier = 1
            case .twilight: zoneMultiplier = 2
            case .midnight: zoneMultiplier = 3
            case .abyssal:  zoneMultiplier = 5
            }
            self?.diveScore += 100 * zoneMultiplier
        }

        diveScene = scene

        // Start composer
        newComposer.start(zone: .sunlit)

        // Progress timer (10Hz) — drives depth, zone transitions, and scoring
        let startTime = Date()
        diveToken = TickScheduler.shared.register(hz: 10) { [weak self] in
            guard let self, self.isDiving else { return }
            let elapsed = Date().timeIntervalSince(startTime)
            self.diveProgress = Float(min(elapsed / 60.0, 1.0))
            self.diveDepth = Int(elapsed * 20)

            // Engagement tracking
            self.totalFrames += 1

            // Zone transition
            let currentZone = DepthZone.at(depth: self.diveDepth)
            if currentZone != self.lastZone {
                self.lastZone = currentZone
                HapticEngine.diveDepthMilestone()
                self.diveScene?.currentZone = currentZone
                self.composer?.updateZone(currentZone)

                // Switch source specimen at zone boundary
                if !self.availableSources.isEmpty {
                    self.currentSourceIndex = (self.currentSourceIndex + 1) % self.availableSources.count
                    self.audioEngine.applyCachedParams(for: self.availableSources[self.currentSourceIndex])
                }
            }

            // Passive scoring
            let zoneMultiplier: Int
            switch currentZone {
            case .sunlit:   zoneMultiplier = 1
            case .twilight: zoneMultiplier = 2
            case .midnight: zoneMultiplier = 3
            case .abyssal:  zoneMultiplier = 5
            }
            self.diveScore += Int(Float(zoneMultiplier) * self.reefBonus)

            if elapsed >= 60 {
                self.endDive()
            }
        }
    }

    private func endDive() {
        guard isDiving else { return }
        isDiving = false
        diveToken = nil

        // Stop tilt modulation
        motionController.stop()

        // Stop composer and clear scene
        composer?.stop()
        composer = nil
        diveScene = nil

        // All notes off
        ObrixBridge.shared()?.allNotesOff()
        activeNotes.removeAll()

        // Lifetime stats
        ReefStatsTracker.shared.increment(.divesCompleted)
        BadgeManager.shared.award("first_dive")
        let divesCompleted = ReefStatsTracker.shared.value(for: .divesCompleted)
        if divesCompleted >= 100 { BadgeManager.shared.award("100_dives") }
        if diveDepth > ReefStatsTracker.shared.value(for: .deepestDive) {
            ReefStatsTracker.shared.increment(.deepestDive, by: diveDepth - ReefStatsTracker.shared.value(for: .deepestDive))
        }

        // Record the dive and check for new high score (check BEFORE recording)
        let previousHigh = diveHistory.highScore
        let interactionPct = totalFrames > 0
            ? Int(Float(activeFrames) / Float(totalFrames) * 100)
            : 0
        let interactionBonus = diveScore * interactionPct / 200
        let finalScore = diveScore + interactionBonus
        let record = DiveRecord(
            id: UUID(),
            date: Date(),
            maxDepth: diveDepth,
            score: finalScore,
            duration: 60,
            specimenCount: reefStore.specimens.compactMap { $0 }.count,
            wireCount: reefStore.couplingRoutes.count,
            deepestZone: DepthZone.at(depth: diveDepth).rawValue
        )
        diveHistory.record(record)
        isNewHighScore = finalScore > previousHigh
        let currentHighStat = ReefStatsTracker.shared.value(for: .highestDiveScore)
        if finalScore > currentHighStat {
            ReefStatsTracker.shared.increment(.highestDiveScore, by: finalScore - currentHighStat)
        }

        // Update weekly challenge progress
        weeklyChallenges.updateFromDive(
            score: finalScore,
            depth: diveDepth,
            interactionPercent: interactionPct
        )

        divePhase = .surfacing
        UINotificationFeedbackGenerator().notificationOccurred(.success)

        // Award mastery XP if prestige is unlocked
        MasteryManager.shared.awardMasteryXP(finalScore / 20)
    }

    // MARK: - Helpers

    private func activeSpecimenStats() -> GameStats {
        // Get stats from the first source specimen, or use defaults
        if let sourceSlot = availableSources.first,
           let specimen = reefStore.specimens[sourceSlot] {
            return GameStats.from(params: specimen.parameterState)
        }
        return GameStats.from(params: [:]) // Defaults
    }
}

// MARK: - Degradation Waveform

private struct DegradationWaveform: View {
    let degradation: Float  // 0 = clean, 1 = destroyed
    let beat: Int           // current beat for animation phase

    var body: some View {
        Canvas { context, size in
            let w = size.width
            let h = size.height
            let midY = h / 2
            let amplitude = h * 0.35

            var path = Path()
            path.move(to: CGPoint(x: 0, y: midY))

            for x in stride(from: 0.0, through: w, by: 2) {
                let normalized = x / w
                // Clean sine wave at degradation=0, noisy at degradation=1
                let cleanWave = sin(normalized * .pi * 6 + Double(beat) * 0.5)
                let noise = Double(degradation) * Double.random(in: -1...1)
                let combined = cleanWave * (1.0 - Double(degradation) * 0.7) + noise
                let y = midY + CGFloat(combined) * amplitude
                path.addLine(to: CGPoint(x: x, y: y))
            }

            // Color shifts from jade (clean) to red (degraded)
            let color: Color = degradation < 0.5
                ? DesignTokens.reefJade.opacity(0.6)
                : DesignTokens.errorRed.opacity(Double(0.4 + degradation * 0.4))

            context.stroke(path, with: .color(color), lineWidth: 1.5)
        }
        .frame(height: 20)
        .padding(.horizontal, 40)
    }
}
