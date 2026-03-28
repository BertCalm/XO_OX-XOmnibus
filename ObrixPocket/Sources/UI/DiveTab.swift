import SwiftUI

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

    enum DivePhase {
        case ready      // Before dive
        case diving     // Active dive
        case surfacing  // Dive complete, showing score
    }

    var body: some View {
        ZStack {
            // Background — gets darker as you dive deeper
            LinearGradient(
                colors: [
                    Color(hex: "0A0A0F"),
                    Color(hex: divePhase == .diving ? "020208" : "0A0A0F")
                ],
                startPoint: .top, endPoint: .bottom
            )
            .ignoresSafeArea()

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

    // MARK: - Diving

    private var divingView: some View {
        VStack(spacing: 16) {
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
        VStack(spacing: 16) {
            Text("SURFACED")
                .font(.custom("SpaceGrotesk-Bold", size: 24))
                .foregroundColor(.white)

            // Final stats
            VStack(spacing: 8) {
                statRow("Depth reached", value: "\(diveDepth)m")
                statRow("Score", value: "\(diveScore)")
                statRow("Duration", value: "60s")
            }
            .padding(.horizontal, 40)

            // XP reward
            let xpReward = diveScore / 10
            Text("+\(xpReward) XP to all reef specimens")
                .font(.custom("JetBrainsMono-Regular", size: 12))
                .foregroundColor(Color(hex: "E9C46A"))

            Button(action: {
                // Award XP to all reef specimens
                for (index, spec) in reefStore.specimens.enumerated() {
                    if spec != nil {
                        audioEngine.earnXP(slotIndex: index, amount: xpReward)
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
        divePhase = .diving
        diveProgress = 0
        diveDepth = 0
        diveScore = 0
        isDiving = true
        activeNotes = []

        // Configure engine with first source's chain
        if let sourceSlot = reefStore.specimens.firstIndex(where: { $0?.category == .source }) {
            audioEngine.applySlotChain(slotIndex: sourceSlot, reefStore: reefStore)
        }

        // Progress timer fires on main run loop — safe to mutate @State directly
        let startTime = Date()
        diveTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [self] _ in
            let elapsed = Date().timeIntervalSince(startTime)
            diveProgress = Float(min(elapsed / 60.0, 1.0))
            diveDepth = Int(elapsed * 20) // 20m per second = 1200m max

            if elapsed >= 60 {
                endDive()
            }
        }

        // Generative note player — plays random notes from the scale at increasing density
        scheduleNextNote(startTime: startTime)
    }

    private func scheduleNextNote(startTime: Date) {
        guard isDiving else { return }

        let elapsed = Date().timeIntervalSince(startTime)
        // Note density increases with depth: 1 note/sec -> 4 notes/sec
        let notesPerSecond = 1.0 + elapsed / 20.0
        let interval = 1.0 / notesPerSecond

        // Timer fires on main run loop — safe to mutate @State directly
        noteTimer = Timer.scheduledTimer(withTimeInterval: interval, repeats: false) { [self] _ in
            guard isDiving else { return }

            // Pick a random pentatonic note
            let pentatonic = [0, 2, 4, 7, 9] // Major pentatonic intervals
            let octave = Int.random(in: 3...5)
            let noteInScale = pentatonic.randomElement() ?? 0
            let midiNote = octave * 12 + noteInScale

            // Play note via bridge
            ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: Float.random(in: 0.5...0.9))
            activeNotes.insert(midiNote)
            diveScore += Int.random(in: 5...15)

            // Note-off after random duration
            let noteDuration = Double.random(in: 0.2...1.0)
            DispatchQueue.main.asyncAfter(deadline: .now() + noteDuration) {
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

        // All notes off
        ObrixBridge.shared()?.allNotesOff()
        activeNotes.removeAll()
        divePhase = .surfacing
        UINotificationFeedbackGenerator().notificationOccurred(.success)
    }
}
