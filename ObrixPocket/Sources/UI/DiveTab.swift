import SwiftUI

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
    @State private var diveTimer: Timer?
    @State private var noteTimer: Timer?
    @State private var lastZone: DepthZone = .sunlit
    @State private var reefBonus: Float = 1.0
    @State private var availableSources: [Int] = []
    @State private var currentSourceIndex = 0
    /// Incremented each time a dive starts; stale note-off closures check this before firing.
    @State private var diveGeneration = 0

    // MARK: - Player Interaction State

    @State private var divePlayerActive = false
    @State private var divePlayerPitch: Float = 0.5   // 0=low, 1=high
    @State private var divePlayerVelocity: Float = 0.5

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
                colors: [Color(hex: "0A0A0F"), Color(hex: zone.bgColor)],
                startPoint: .top, endPoint: .bottom
            )
            .ignoresSafeArea()
            .animation(.easeInOut(duration: 2.0), value: zone.rawValue)

            VStack(spacing: 24) {
                // Header
                HStack {
                    Text("THE DIVE")
                        .font(.custom("SpaceGrotesk-Bold", size: 18))
                        .foregroundColor(.white)
                    Spacer()
                    Text("\(diveDepth)m")
                        .font(.custom("JetBrainsMono-Bold", size: 16))
                        .foregroundColor(Color(hex: "1E8B7E"))
                }
                .padding(.horizontal, 20)
                .padding(.top, 12)

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
                .foregroundColor(Color(hex: "1E8B7E").opacity(0.4))

            Text("Dive into your reef")
                .font(.custom("SpaceGrotesk-Bold", size: 20))
                .foregroundColor(.white)

            Text("60 seconds of generative performance.\nYour reef plays itself. Deeper = more complex.")
                .font(.custom("Inter-Regular", size: 13))
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

            Button(action: startDive) {
                Text("DIVE")
                    .font(.custom("SpaceGrotesk-Bold", size: 18))
                    .tracking(3)
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity)
                    .frame(height: 56)
                    .background(
                        RoundedRectangle(cornerRadius: 28)
                            .fill(totalCount >= 4 ? Color(hex: "1E8B7E") : Color(hex: "1A1A1C"))
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
                        .font(.custom("Inter-Regular", size: 12))
                        .foregroundColor(.white.opacity(0.4))
                }
            }

            // Weekly challenges
            if !weeklyChallenges.challenges.isEmpty {
                VStack(alignment: .leading, spacing: 6) {
                    HStack {
                        Text("WEEKLY CHALLENGES")
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .tracking(1.5)
                            .foregroundColor(Color(hex: "E9C46A"))
                        Spacer()
                        Text("\(weeklyChallenges.completedCount)/\(weeklyChallenges.challenges.count)")
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.3))
                    }

                    ForEach(weeklyChallenges.challenges) { challenge in
                        HStack(spacing: 8) {
                            Image(systemName: challenge.completed ? "checkmark.circle.fill" : "circle")
                                .font(.system(size: 12))
                                .foregroundColor(challenge.completed ? Color(hex: "1E8B7E") : .white.opacity(0.2))

                            VStack(alignment: .leading, spacing: 1) {
                                Text(challenge.title)
                                    .font(.custom("Inter-Medium", size: 11))
                                    .foregroundColor(challenge.completed ? .white.opacity(0.4) : .white.opacity(0.7))
                                    .strikethrough(challenge.completed)
                                Text(challenge.description)
                                    .font(.custom("Inter-Regular", size: 9))
                                    .foregroundColor(.white.opacity(0.25))
                            }

                            Spacer()

                            Text(challenge.progressString)
                                .font(.custom("JetBrainsMono-Regular", size: 9))
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
                .font(.custom("Inter-Regular", size: 11))
                .foregroundColor(.white.opacity(0.4))
            Spacer()
            Text("\(count)/\(required)")
                .font(.custom("JetBrainsMono-Regular", size: 11))
                .foregroundColor(count >= required ? Color(hex: "1E8B7E") : Color(hex: "FF4D4D"))
            Image(systemName: count >= required ? "checkmark.circle.fill" : "xmark.circle")
                .font(.system(size: 10))
                .foregroundColor(count >= required ? Color(hex: "1E8B7E") : Color(hex: "FF4D4D").opacity(0.5))
        }
    }

    private func statColumn(_ label: String, value: String) -> some View {
        VStack(spacing: 2) {
            Text(value)
                .font(.custom("JetBrainsMono-Bold", size: 16))
                .foregroundColor(Color(hex: "1E8B7E"))
            Text(label)
                .font(.custom("Inter-Regular", size: 9))
                .foregroundColor(.white.opacity(0.3))
        }
        .frame(maxWidth: .infinity)
    }

    // MARK: - Diving

    private var divingView: some View {
        VStack(spacing: 16) {
            // Current zone name
            let zone = DepthZone.at(depth: diveDepth)
            Text(zone.rawValue.uppercased())
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .tracking(2)
                .foregroundColor(.white.opacity(0.3))

            // Current source indicator
            if !availableSources.isEmpty {
                let currentSlot = availableSources[currentSourceIndex]
                if let spec = reefStore.specimens[currentSlot] {
                    HStack(spacing: 6) {
                        SpecimenSprite(subtype: spec.subtype, category: spec.category, size: 20)
                        Text(spec.creatureName)
                            .font(.custom("JetBrainsMono-Regular", size: 10))
                            .foregroundColor(Color(hex: "1E8B7E").opacity(0.6))
                    }
                }
            }

            // Depth counter (large, centered)
            Text("\(diveDepth)m")
                .font(.custom("JetBrainsMono-Bold", size: 48))
                .foregroundColor(Color(hex: "1E8B7E"))

            // Progress bar
            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 3)
                        .fill(Color.white.opacity(0.06))
                    RoundedRectangle(cornerRadius: 3)
                        .fill(Color(hex: "1E8B7E").opacity(0.5))
                        .frame(width: geo.size.width * CGFloat(diveProgress))
                }
            }
            .frame(height: 6)
            .padding(.horizontal, 40)

            // Active note indicators
            HStack(spacing: 8) {
                ForEach(Array(activeNotes.sorted()), id: \.self) { _ in
                    Circle()
                        .fill(Color(hex: "1E8B7E").opacity(0.6))
                        .frame(width: 8, height: 8)
                }
            }

            // Score
            Text("Score: \(diveScore)")
                .font(.custom("JetBrainsMono-Regular", size: 14))
                .foregroundColor(.white.opacity(0.5))

            // Touch interaction zone
            GeometryReader { geo in
                ZStack {
                    Color.clear
                        .contentShape(Rectangle())
                        .gesture(
                            DragGesture(minimumDistance: 0)
                                .onChanged { value in
                                    // X position → pitch range (left=low, right=high)
                                    let xNorm = Float(value.location.x / geo.size.width)
                                    divePlayerPitch = max(0, min(1, xNorm))

                                    // Y position → velocity (top=loud, bottom=soft)
                                    let yNorm = 1.0 - Float(value.location.y / geo.size.height)
                                    divePlayerVelocity = max(0, min(1, yNorm))

                                    divePlayerActive = true
                                }
                                .onEnded { _ in
                                    divePlayerActive = false
                                }
                        )

                    // Finger cursor
                    if divePlayerActive {
                        Circle()
                            .fill(Color(hex: "1E8B7E"))
                            .frame(width: 12, height: 12)
                            .offset(
                                x: CGFloat(divePlayerPitch - 0.5) * 100,
                                y: CGFloat(0.5 - divePlayerVelocity) * 60
                            )
                    }

                    // Label
                    VStack(spacing: 2) {
                        if divePlayerActive {
                            Text("STEERING")
                                .font(.custom("JetBrainsMono-Regular", size: 8))
                                .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
                        } else {
                            Text("TOUCH TO STEER")
                                .font(.custom("Inter-Regular", size: 9))
                                .foregroundColor(.white.opacity(0.15))
                        }
                    }
                }
            }
            .frame(height: 150)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(Color.white.opacity(divePlayerActive ? 0.04 : 0.02))
            )
            .padding(.horizontal, 20)

            // Abort button
            Button(action: endDive) {
                Text("SURFACE")
                    .font(.custom("Inter-Regular", size: 12))
                    .foregroundColor(.white.opacity(0.3))
            }
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
                .font(.custom("SpaceGrotesk-Bold", size: 24))
                .foregroundColor(.white)

            // New high score badge
            if isNewHighScore {
                Text("NEW HIGH SCORE!")
                    .font(.custom("SpaceGrotesk-Bold", size: 16))
                    .foregroundColor(Color(hex: "E9C46A"))
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
                .font(.custom("JetBrainsMono-Regular", size: 12))
                .foregroundColor(Color(hex: "E9C46A"))

            Button(action: {
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
            }) {
                Text("COLLECT REWARDS")
                    .font(.custom("SpaceGrotesk-Bold", size: 16))
                    .tracking(2)
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity)
                    .frame(height: 50)
                    .background(RoundedRectangle(cornerRadius: 25).fill(Color(hex: "1E8B7E")))
            }
            .padding(.horizontal, 40)
        }
    }

    private func statRow(_ label: String, value: String) -> some View {
        HStack {
            Text(label)
                .font(.custom("Inter-Regular", size: 12))
                .foregroundColor(.white.opacity(0.4))
            Spacer()
            Text(value)
                .font(.custom("JetBrainsMono-Bold", size: 14))
                .foregroundColor(.white)
        }
    }

    // MARK: - Dive Logic

    private func startDive() {
        diveGeneration += 1
        divePhase = .diving
        diveProgress = 0
        diveDepth = 0
        diveScore = 0
        isDiving = true
        activeNotes = []
        lastZone = .sunlit

        // Reset engagement counters and player state
        activeFrames = 0
        totalFrames = 0
        divePlayerActive = false
        divePlayerPitch = 0.5
        divePlayerVelocity = 0.5

        // Start tilt modulation for dive expression
        motionController.start()

        // Compute reef quality bonus from specimen count and coupling wires
        let specimenCount = reefStore.specimens.compactMap { $0 }.count
        let wireCount = reefStore.couplingRoutes.count
        reefBonus = max(1.0, Float(specimenCount) * 0.1 + Float(wireCount) * 0.15)

        // Collect all source slots for zone-based switching
        availableSources = reefStore.specimens.enumerated()
            .compactMap { (index, spec) -> Int? in
                spec?.category == .source ? index : nil
            }
        currentSourceIndex = 0

        // Configure engine with first source's chain
        if let sourceSlot = availableSources.first {
            audioEngine.applySlotChain(slotIndex: sourceSlot, reefStore: reefStore)
        } else if let sourceSlot = reefStore.specimens.firstIndex(where: { $0?.category == .source }) {
            audioEngine.applySlotChain(slotIndex: sourceSlot, reefStore: reefStore)
        }

        // Progress timer fires on main run loop — safe to mutate @State directly
        let startTime = Date()
        diveTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [self] _ in
            let elapsed = Date().timeIntervalSince(startTime)
            diveProgress = Float(min(elapsed / 60.0, 1.0))
            diveDepth = Int(elapsed * 20) // 20m per second = 1200m max

            // Track engagement frames (10 ticks/sec)
            totalFrames += 1
            if divePlayerActive { activeFrames += 1 }

            // Detect zone transitions and fire haptic + switch source
            let currentZone = DepthZone.at(depth: diveDepth)
            if currentZone != lastZone {
                lastZone = currentZone
                HapticEngine.diveDepthMilestone()

                // Switch to next source in the reef
                if !availableSources.isEmpty {
                    currentSourceIndex = (currentSourceIndex + 1) % availableSources.count
                    let nextSlot = availableSources[currentSourceIndex]
                    audioEngine.applyCachedParams(for: nextSlot)
                }
            }

            if elapsed >= 60 {
                endDive()
            }
        }

        // Generative note player — zone-aware density and pitch content
        scheduleNextNote(startTime: startTime)
    }

    private func scheduleNextNote(startTime: Date) {
        guard isDiving else { return }

        let elapsed = Date().timeIntervalSince(startTime)
        let zone = DepthZone.at(depth: diveDepth)

        // Note density varies by zone
        let notesPerSecond: Double
        switch zone {
        case .sunlit:   notesPerSecond = 1.0 + elapsed / 30.0  // Gentle buildup
        case .twilight: notesPerSecond = 2.0 + elapsed / 20.0  // Moderate
        case .midnight: notesPerSecond = 3.0 + elapsed / 15.0  // Dense
        case .abyssal:  notesPerSecond = 1.5                    // Sparse again — eerie
        }

        let interval = 1.0 / notesPerSecond

        // Timer fires on main run loop — safe to mutate @State directly
        noteTimer = Timer.scheduledTimer(withTimeInterval: interval, repeats: false) { [self] _ in
            guard isDiving else { return }

            // Re-read zone at the moment the note fires (depth has advanced)
            let currentZone = DepthZone.at(depth: diveDepth)
            let scale = currentZone.scaleIntervals
            let noteInScale = scale.randomElement() ?? 0

            // Zone multiplier (used for both scoring and player bonus)
            let zoneMultiplier: Int
            switch currentZone {
            case .sunlit:   zoneMultiplier = 1
            case .twilight: zoneMultiplier = 2
            case .midnight: zoneMultiplier = 3
            case .abyssal:  zoneMultiplier = 5
            }

            // Octave and velocity: player steers when active, auto-play otherwise
            let octave: Int
            let velocity: Float
            if divePlayerActive {
                // Player is steering — use their X/Y input
                let octaveRange = currentZone.octaveRange
                let octaveSpan = octaveRange.upperBound - octaveRange.lowerBound
                octave = octaveRange.lowerBound + Int(divePlayerPitch * Float(octaveSpan))
                velocity = max(0.15, divePlayerVelocity * 0.9)

                // Bonus points for active engagement
                diveScore += 5 * zoneMultiplier
            } else {
                // Auto-play: existing random behavior
                octave = Int.random(in: currentZone.octaveRange)
                switch currentZone {
                case .sunlit:   velocity = Float.random(in: 0.5...0.9)
                case .twilight: velocity = Float.random(in: 0.4...0.7)
                case .midnight: velocity = Float.random(in: 0.3...0.6)
                case .abyssal:  velocity = Float.random(in: 0.15...0.4)
                }
            }

            let midiNote = octave * 12 + noteInScale
            ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: velocity)
            activeNotes.insert(midiNote)

            // Score: more points in deeper zones, scaled by reef quality bonus
            diveScore += Int(Float(Int.random(in: 5...15) * zoneMultiplier) * reefBonus)

            // Note duration varies by zone (deeper = longer, more sustained)
            let noteDuration: Double
            switch currentZone {
            case .sunlit:   noteDuration = Double.random(in: 0.2...0.8)
            case .twilight: noteDuration = Double.random(in: 0.3...1.2)
            case .midnight: noteDuration = Double.random(in: 0.1...0.5)  // Quick staccato
            case .abyssal:  noteDuration = Double.random(in: 1.0...4.0)  // Long, haunting
            }

            let gen = diveGeneration
            DispatchQueue.main.asyncAfter(deadline: .now() + noteDuration) {
                guard diveGeneration == gen else { return }
                ObrixBridge.shared()?.noteOff(Int32(midiNote))
                activeNotes.remove(midiNote)
            }

            // Schedule next note
            scheduleNextNote(startTime: startTime)
        }
    }

    private func endDive() {
        guard isDiving else { return }
        isDiving = false
        diveTimer?.invalidate()
        diveTimer = nil
        noteTimer?.invalidate()
        noteTimer = nil

        // Stop tilt modulation
        motionController.stop()

        // All notes off
        ObrixBridge.shared()?.allNotesOff()
        activeNotes.removeAll()

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

        // Update weekly challenge progress
        weeklyChallenges.updateFromDive(
            score: finalScore,
            depth: diveDepth,
            interactionPercent: interactionPct
        )

        divePhase = .surfacing
        UINotificationFeedbackGenerator().notificationOccurred(.success)
    }
}
