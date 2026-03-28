import SwiftUI
import SpriteKit

struct ReefTab: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager
    @StateObject private var presetManager = ReefPresetManager()
    @StateObject private var recorder = PerformanceRecorder()
    @StateObject private var motionController = MotionController()
    @StateObject private var challengeManager = DailyChallengeManager()
    @StateObject private var milestoneManager = MilestoneManager()
    @State private var reefScene: ReefScene?
    @State private var activeSourceSlot: Int?  // Which source the keyboard plays through
    @State private var selectedSlot: Int?        // Which specimen's params are showing
    @State private var octaveOffset: Int = 0      // Keyboard octave shift (-2 to +2)
    @State private var keyboardScale: KeyboardScale = .pentatonic  // Default: hardest to sound bad
    @State private var showSaveDialog = false
    @State private var showLoadSheet = false
    @State private var presetName = ""

    var body: some View {
        VStack(spacing: 0) {
            // Reef name header
            HStack {
                Text(reefStore.reefName)
                    .font(.custom("SpaceGrotesk-Bold", size: 18))
                    .foregroundColor(.white)
                Spacer()
                // Journey progress replaces depth counter until the tutorial is complete
                if !firstLaunchManager.isJourneyComplete {
                    Text("Journey: \(firstLaunchManager.journeyStep)/8")
                        .font(.custom("JetBrainsMono-Regular", size: 11))
                        .foregroundColor(Color(hex: "E9C46A"))
                } else {
                    Text("Depth: \(reefStore.totalDiveDepth)m")
                        .font(.custom("JetBrainsMono-Regular", size: 11))
                        .foregroundColor(Color(hex: "7A7876"))
                }
                // Save / Load preset buttons
                HStack(spacing: 8) {
                    Button(action: { showSaveDialog = true }) {
                        Image(systemName: "square.and.arrow.down")
                            .font(.system(size: 12))
                            .foregroundColor(Color(hex: "1E8B7E").opacity(0.6))
                    }
                    if !presetManager.presets.isEmpty {
                        Button(action: { showLoadSheet = true }) {
                            Image(systemName: "square.and.arrow.up")
                                .font(.system(size: 12))
                                .foregroundColor(Color(hex: "1E8B7E").opacity(0.6))
                        }
                    }
                }
                if !reefStore.couplingRoutes.isEmpty {
                    Button(action: {
                        reefStore.couplingRoutes.removeAll()
                        reefStore.save()
                        audioEngine.applyReefConfiguration(reefStore)
                    }) {
                        Text("Clear Wires")
                            .font(.custom("Inter-Regular", size: 11))
                            .foregroundColor(Color(hex: "FF4D4D").opacity(0.6))
                    }
                }
            }
            .padding(.horizontal, 20)
            .padding(.top, 12)
            .padding(.bottom, 8)

            // Daily challenges bar
            HStack(spacing: 8) {
                Image(systemName: "star.fill")
                    .font(.system(size: 9))
                    .foregroundColor(Color(hex: "E9C46A"))

                ForEach(challengeManager.challenges) { challenge in
                    VStack(spacing: 2) {
                        Text(challenge.description)
                            .font(.custom("Inter-Regular", size: 8))
                            .foregroundColor(challenge.isComplete ? Color(hex: "1E8B7E") : .white.opacity(0.4))
                            .strikethrough(challenge.isComplete)
                            .lineLimit(1)

                        // Mini progress bar
                        GeometryReader { geo in
                            ZStack(alignment: .leading) {
                                RoundedRectangle(cornerRadius: 1)
                                    .fill(Color.white.opacity(0.06))
                                RoundedRectangle(cornerRadius: 1)
                                    .fill(challenge.isComplete ? Color(hex: "1E8B7E") : Color(hex: "E9C46A").opacity(0.5))
                                    .frame(width: geo.size.width * CGFloat(challenge.progress) / CGFloat(challenge.target))
                            }
                        }
                        .frame(height: 2)
                    }
                }
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 4)

            // Daily energy progress bar
            if !firstLaunchManager.energyDistributedToday {
                HStack(spacing: 6) {
                    Image(systemName: "bolt.fill")
                        .font(.system(size: 9))
                        .foregroundColor(Color(hex: "E9C46A").opacity(0.5))

                    GeometryReader { geo in
                        ZStack(alignment: .leading) {
                            RoundedRectangle(cornerRadius: 2)
                                .fill(Color.white.opacity(0.06))
                            RoundedRectangle(cornerRadius: 2)
                                .fill(Color(hex: "E9C46A").opacity(0.4))
                                .frame(width: geo.size.width * CGFloat(firstLaunchManager.energyProgress))
                        }
                    }
                    .frame(width: 60, height: 4)

                    Text("Daily Energy")
                        .font(.custom("JetBrainsMono-Regular", size: 8))
                        .foregroundColor(.white.opacity(0.2))
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 4)
            } else {
                HStack(spacing: 4) {
                    Image(systemName: "checkmark.circle.fill")
                        .font(.system(size: 9))
                        .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
                    Text("Daily Energy collected")
                        .font(.custom("JetBrainsMono-Regular", size: 8))
                        .foregroundColor(.white.opacity(0.2))
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 4)
            }

            // The Reef Grid (SpriteKit scene)
            GeometryReader { geometry in
                let gridSize = min(geometry.size.width * 0.9, geometry.size.height * 0.85)

                if let scene = reefScene {
                    SpriteView(scene: scene)
                        .frame(width: gridSize, height: gridSize)
                        .clipShape(RoundedRectangle(cornerRadius: 16))
                        .position(x: geometry.size.width / 2, y: geometry.size.height * 0.45)
                        .onReceive(reefStore.objectWillChange) { _ in
                            reefScene?.refreshGrid()
                        }
                } else {
                    Color.clear
                        .onAppear {
                            reefScene = ReefScene(
                                size: CGSize(width: gridSize, height: gridSize),
                                reefStore: reefStore,
                                onNoteOn: { slot, velocity in
                                    // Select this slot for parameter editing
                                    selectedSlot = slot
                                    // If it's a Source, make it the active keyboard source
                                    if let spec = reefStore.specimens[slot], spec.category == .source {
                                        activeSourceSlot = slot
                                    }
                                    audioEngine.noteOn(slotIndex: slot, velocity: velocity)
                                },
                                onNoteOff: { slot in
                                    audioEngine.noteOff(slotIndex: slot)
                                },
                                onWiringChanged: {
                                    audioEngine.applyReefConfiguration(reefStore)
                                }
                            )
                        }
                }
            }

            // Contextual hint for new users (journey steps 0–2)
            if firstLaunchManager.journeyStep <= 2 {
                Text("Tap a specimen to preview its sound. Long-press to wire.")
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.25))
                    .padding(.horizontal, 20)
                    .padding(.bottom, 4)
            }

            // Parameter panel OR stasis browser — shows when a slot is tapped
            if let slot = selectedSlot {
                if reefStore.specimens[slot] != nil {
                    // Occupied slot — show param panel
                    SpecimenParamPanel(slotIndex: slot, onDismiss: { selectedSlot = nil })
                        .transition(.move(edge: .bottom).combined(with: .opacity))
                        .animation(.easeInOut(duration: 0.2), value: selectedSlot)
                } else {
                    // Empty slot — show stasis browser to place a specimen from stasis
                    StasisBrowser(targetSlot: slot, onDismiss: { selectedSlot = nil })
                        .environmentObject(reefStore)
                        .transition(.move(edge: .bottom).combined(with: .opacity))
                        .animation(.easeInOut(duration: 0.2), value: selectedSlot)
                }
            }

            // Play keyboard — plays through the active source's wired chain
            if reefStore.diveEligibleCount >= 1 {
                // Show which source is active
                let slot = activeSourceSlot ?? firstSourceSlot
                let sourceName = slot.flatMap { reefStore.specimens[$0]?.creatureName } ?? "—"

                HStack {
                    Text("Playing: \(sourceName)")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(Color(hex: "1E8B7E").opacity(0.7))
                    Spacer()
                    // Scale selector
                    Menu {
                        ForEach(KeyboardScale.allCases, id: \.self) { scale in
                            Button(action: { keyboardScale = scale }) {
                                HStack {
                                    Text(scale.rawValue)
                                    if scale == keyboardScale {
                                        Image(systemName: "checkmark")
                                    }
                                }
                            }
                        }
                    } label: {
                        Text(keyboardScale.rawValue)
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .foregroundColor(Color(hex: "1E8B7E").opacity(0.6))
                    }
                    // Octave controls
                    Button(action: { if octaveOffset > -2 { octaveOffset -= 1 } }) {
                        Text("−")
                            .font(.custom("SpaceGrotesk-Bold", size: 16))
                            .foregroundColor(octaveOffset > -2 ? .white : .white.opacity(0.2))
                            .frame(width: 28, height: 28)
                    }
                    Text("C\(4 + octaveOffset)")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(.white.opacity(0.5))
                        .frame(width: 24)
                    Button(action: { if octaveOffset < 2 { octaveOffset += 1 } }) {
                        Text("+")
                            .font(.custom("SpaceGrotesk-Bold", size: 16))
                            .foregroundColor(octaveOffset < 2 ? .white : .white.opacity(0.2))
                            .frame(width: 28, height: 28)
                    }
                    // Motion toggle
                    Button(action: {
                        if motionController.isEnabled { motionController.stop() }
                        else { motionController.start() }
                    }) {
                        Image(systemName: "gyroscope")
                            .font(.system(size: 12))
                            .foregroundColor(motionController.isEnabled ? Color(hex: "1E8B7E") : .white.opacity(0.3))
                    }
                    // Tilt position dot — only visible when motion is active
                    if motionController.isEnabled {
                        HStack(spacing: 4) {
                            Text("TILT")
                                .font(.custom("JetBrainsMono-Regular", size: 8))
                                .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
                            Circle()
                                .fill(Color(hex: "1E8B7E"))
                                .frame(width: 4, height: 4)
                                .offset(x: CGFloat(motionController.tiltX) * 10,
                                        y: CGFloat(-motionController.tiltY) * 10)
                        }
                    }
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 2)

                // Record controls
                if recorder.hasRecording || recorder.isRecording {
                    HStack(spacing: 12) {
                        // Record button
                        Button(action: {
                            if recorder.isRecording { recorder.stopRecording() }
                            else { recorder.startRecording() }
                        }) {
                            Circle()
                                .fill(recorder.isRecording ? Color(hex: "FF4D4D") : Color(hex: "FF4D4D").opacity(0.5))
                                .frame(width: 24, height: 24)
                                .overlay(
                                    recorder.isRecording ?
                                    AnyView(RoundedRectangle(cornerRadius: 3).fill(.white).frame(width: 10, height: 10)) :
                                    AnyView(Circle().fill(.white).frame(width: 10, height: 10))
                                )
                        }

                        if recorder.hasRecording && !recorder.isRecording {
                            // Play button
                            Button(action: {
                                if recorder.isPlaying {
                                    recorder.stopPlayback()
                                } else {
                                    recorder.play(
                                        noteOn: { note, vel in ObrixBridge.shared()?.note(on: Int32(note), velocity: vel) },
                                        noteOff: { note in ObrixBridge.shared()?.noteOff(Int32(note)) }
                                    )
                                }
                            }) {
                                Image(systemName: recorder.isPlaying ? "stop.fill" : "play.fill")
                                    .font(.system(size: 12))
                                    .foregroundColor(Color(hex: "1E8B7E"))
                            }

                            Text(recorder.durationString)
                                .font(.custom("JetBrainsMono-Regular", size: 10))
                                .foregroundColor(.white.opacity(0.4))

                            // Share button
                            Button(action: {
                                guard let data = recorder.exportJSON() else { return }
                                let url = FileManager.default.temporaryDirectory
                                    .appendingPathComponent("reef-performance.json")
                                try? data.write(to: url)
                                let av = UIActivityViewController(activityItems: [url], applicationActivities: nil)
                                if let windowScene = UIApplication.shared.connectedScenes.first as? UIWindowScene,
                                   let rootVC = windowScene.windows.first?.rootViewController {
                                    rootVC.present(av, animated: true)
                                }
                            }) {
                                Image(systemName: "square.and.arrow.up")
                                    .font(.system(size: 12))
                                    .foregroundColor(Color(hex: "E9C46A").opacity(0.6))
                            }
                        }

                        // Recording indicator — duration readout while active
                        if recorder.isRecording {
                            HStack(spacing: 6) {
                                Circle()
                                    .fill(Color(hex: "FF4D4D"))
                                    .frame(width: 8, height: 8)
                                Text("REC \(recorder.durationString)")
                                    .font(.custom("JetBrainsMono-Regular", size: 10))
                                    .foregroundColor(Color(hex: "FF4D4D").opacity(0.7))
                            }
                        }

                        Spacer()
                    }
                    .padding(.horizontal, 20)
                    .padding(.bottom, 4)
                } else {
                    // Show record button when no recording exists yet
                    HStack {
                        Button(action: { recorder.startRecording() }) {
                            HStack(spacing: 4) {
                                Circle()
                                    .fill(Color(hex: "FF4D4D").opacity(0.5))
                                    .frame(width: 12, height: 12)
                                Text("REC")
                                    .font(.custom("JetBrainsMono-Regular", size: 9))
                                    .foregroundColor(.white.opacity(0.3))
                            }
                        }
                        Spacer()
                    }
                    .padding(.horizontal, 20)
                    .padding(.bottom, 4)
                }

                PlayKeyboard(
                    onNoteOn: { midiNote, velocity in
                        let sourceSlot = activeSourceSlot ?? firstSourceSlot
                        if let s = sourceSlot {
                            audioEngine.applySlotChain(slotIndex: s, reefStore: reefStore)
                        }
                        ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: velocity)
                        recorder.recordNoteOn(midiNote: midiNote, velocity: velocity)
                        challengeManager.incrementProgress(type: .playNotes)
                        milestoneManager.increment("play_100")
                        milestoneManager.increment("play_1000")
                        milestoneManager.increment("play_10000")
                        // Reef Energy: approximate ~0.5s per note press
                        firstLaunchManager.dailyPlaySeconds += 0.5
                        // Distribute daily energy once threshold is reached
                        if firstLaunchManager.dailyEnergyEarned && !firstLaunchManager.energyDistributedToday {
                            for (index, spec) in reefStore.specimens.enumerated() {
                                if spec != nil {
                                    audioEngine.earnXP(slotIndex: index, amount: 25)
                                }
                            }
                            firstLaunchManager.recordEnergyDistribution()
                            HapticEngine.energyCollected()
                        }
                    },
                    onNoteOff: { midiNote in
                        ObrixBridge.shared()?.noteOff(Int32(midiNote))
                        recorder.recordNoteOff(midiNote: midiNote)
                    },
                    accentColor: Color(hex: "1E8B7E"),
                    octaveOffset: $octaveOffset,
                    scale: keyboardScale
                )
                .frame(height: 80)
                .clipShape(RoundedRectangle(cornerRadius: 12))
                .padding(.horizontal, 16)
                .padding(.bottom, 8)
            } else {
                Text("Add a specimen to play")
                    .font(.custom("Inter-Regular", size: 12))
                    .foregroundColor(Color(hex: "7A7876"))
                    .padding(.bottom, 16)
            }
        }
        .background(Color(hex: "0E0E10"))
        .onAppear {
            // Default to the first Source in the reef
            if activeSourceSlot == nil {
                activeSourceSlot = firstSourceSlot
            }
        }
        .onDisappear {
            motionController.stop()
        }
        .alert("Save Reef Preset", isPresented: $showSaveDialog) {
            TextField("Preset name", text: $presetName)
            Button("Save") {
                if !presetName.isEmpty {
                    presetManager.save(name: presetName, from: reefStore)
                    presetName = ""
                }
            }
            Button("Cancel", role: .cancel) { presetName = "" }
        }
        .sheet(isPresented: $showLoadSheet) {
            ReefPresetList(
                presetManager: presetManager,
                reefStore: reefStore,
                onDismiss: { showLoadSheet = false }
            )
        }
    }

    private var firstSourceSlot: Int? {
        reefStore.specimens.firstIndex(where: { $0?.category == .source })
    }
}

// MARK: - StasisBrowser

/// Shown when an empty reef slot is tapped — lets the player place a stasis specimen into it.
struct StasisBrowser: View {
    let targetSlot: Int
    var onDismiss: (() -> Void)?
    @EnvironmentObject var reefStore: ReefStore

    @State private var stasisSpecimens: [Specimen] = []

    var body: some View {
        VStack(spacing: 8) {
            HStack {
                Text("STASIS")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .tracking(1.5)
                    .foregroundColor(Color(hex: "E9C46A"))
                Text("— tap to place in slot \(targetSlot + 1)")
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.3))
                Spacer()
                Button(action: { onDismiss?() }) {
                    Image(systemName: "xmark")
                        .font(.system(size: 10, weight: .bold))
                        .foregroundColor(.white.opacity(0.3))
                }
            }
            .padding(.horizontal, 16)

            if stasisSpecimens.isEmpty {
                Text("No specimens in stasis")
                    .font(.custom("Inter-Regular", size: 11))
                    .foregroundColor(.white.opacity(0.3))
                    .padding(.vertical, 12)
            } else {
                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: 12) {
                        ForEach(stasisSpecimens) { specimen in
                            Button(action: {
                                reefStore.moveFromStasis(specimenId: specimen.id, toSlot: targetSlot)
                                reefStore.save()
                                onDismiss?()
                            }) {
                                VStack(spacing: 4) {
                                    SpecimenSprite(subtype: specimen.subtype,
                                                  category: specimen.category,
                                                  size: 36)
                                    Text(specimen.creatureName)
                                        .font(.custom("Inter-Regular", size: 8))
                                        .foregroundColor(.white.opacity(0.6))
                                        .lineLimit(1)
                                    Text("Lv.\(specimen.level)")
                                        .font(.custom("JetBrainsMono-Regular", size: 7))
                                        .foregroundColor(.white.opacity(0.3))
                                }
                                .frame(width: 56)
                            }
                        }
                    }
                    .padding(.horizontal, 16)
                }
            }
        }
        .padding(.vertical, 8)
        .background(Color(hex: "141418"))
        .cornerRadius(12)
        .padding(.horizontal, 12)
        .onAppear {
            stasisSpecimens = reefStore.loadStasisSpecimens()
        }
    }
}

// MARK: - ReefPresetList

struct ReefPresetList: View {
    @ObservedObject var presetManager: ReefPresetManager
    let reefStore: ReefStore
    let onDismiss: () -> Void

    var body: some View {
        NavigationView {
            List {
                ForEach(presetManager.presets) { preset in
                    Button(action: {
                        presetManager.restore(preset, to: reefStore)
                        onDismiss()
                    }) {
                        VStack(alignment: .leading, spacing: 4) {
                            Text(preset.name)
                                .font(.custom("SpaceGrotesk-Bold", size: 14))
                                .foregroundColor(.white)
                            Text(preset.savedAt, style: .date)
                                .font(.custom("JetBrainsMono-Regular", size: 10))
                                .foregroundColor(.white.opacity(0.4))
                            Text("\(preset.specimenSlots.compactMap { $0 }.count) specimens, \(preset.couplingRoutes.count) wires")
                                .font(.custom("Inter-Regular", size: 10))
                                .foregroundColor(.white.opacity(0.3))
                        }
                    }
                }
                .onDelete { indexSet in
                    for index in indexSet {
                        presetManager.delete(presetManager.presets[index])
                    }
                }
            }
            .listStyle(.plain)
            .navigationTitle("Reef Presets")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Done") { onDismiss() }
                }
            }
            .background(Color(hex: "0E0E10"))
        }
    }
}
