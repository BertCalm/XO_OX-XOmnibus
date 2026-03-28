import SwiftUI
import SpriteKit

struct ReefTab: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager

    // Owned here because they bridge to PlayKeyboard callbacks or are needed by multiple children
    @StateObject private var metronome = MetronomeManager()
    @StateObject private var recorder = PerformanceRecorder()
    @StateObject private var loopRecorder = LoopRecorder()
    @StateObject private var challengeManager = DailyChallengeManager()
    @StateObject private var milestoneManager = MilestoneManager()
    @StateObject private var ambientManager = ReefAmbientManager()
    @StateObject private var encounterManager = RandomEncounterManager()

    @State private var reefScene: ReefScene?
    @State private var gridRefreshTimer: Timer?
    @State private var activeSourceSlot: Int?
    @State private var selectedSlot: Int?

    // Passed as @Binding to ReefKeyboardHeader
    @State private var octaveOffset: Int = 0
    @State private var keyboardScale: KeyboardScale = .pentatonic
    @State private var keyboardMode: KeyboardMode = .single
    @State private var showPerformanceMode = false
    @State private var ambientEnabled = false
    @State private var ambientResumeTimer: Timer?
    @State private var showRecordingControls = false

    var body: some View {
        VStack(spacing: 0) {
            ReefHeaderView(onThemeChanged: { theme in
                reefScene?.theme = theme
            })

            ReefBannerBar(encounterManager: encounterManager)

            ReefIndicatorBar(challengeManager: challengeManager)

            // SpriteKit reef grid
            GeometryReader { geometry in
                let gridSize = min(geometry.size.width * 0.9, geometry.size.height * 0.85)

                if let scene = reefScene {
                    SpriteView(scene: scene)
                        .frame(width: gridSize, height: gridSize)
                        .clipShape(RoundedRectangle(cornerRadius: 16))
                        .overlay(
                            ReefAccessibilityOverlay(reefStore: reefStore, gridSize: gridSize)
                        )
                        .position(x: geometry.size.width / 2, y: geometry.size.height * 0.45)
                        .onReceive(reefStore.objectWillChange) { _ in
                            gridRefreshTimer?.invalidate()
                            let t = Timer(timeInterval: 0.15, repeats: false) { [self] _ in
                                reefScene?.refreshGrid()
                            }
                            RunLoop.main.add(t, forMode: .common)
                            gridRefreshTimer = t
                        }
                } else {
                    VStack(spacing: 8) {
                        ProgressView()
                            .tint(DesignTokens.reefJade)
                        Text("Loading reef...")
                            .font(DesignTokens.mono(10))
                            .foregroundColor(.white.opacity(0.3))
                    }
                    .frame(width: gridSize, height: gridSize)
                    .onAppear {
                        reefScene = ReefScene(
                            size: CGSize(width: gridSize, height: gridSize),
                            reefStore: reefStore,
                            onNoteOn: { slot, velocity in
                                selectedSlot = slot
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
                    .font(DesignTokens.body(10))
                    .foregroundColor(.white.opacity(0.25))
                    .padding(.horizontal, 20)
            }

            // Parameter panel OR stasis browser — shows when a slot is tapped
            if let slot = selectedSlot {
                if reefStore.specimens[slot] != nil {
                    SpecimenParamPanel(slotIndex: slot, onDismiss: { selectedSlot = nil })
                        .transition(.move(edge: .bottom).combined(with: .opacity))
                        .animation(.easeInOut(duration: 0.2), value: selectedSlot)
                } else {
                    StasisBrowser(targetSlot: slot, onDismiss: { selectedSlot = nil })
                        .environmentObject(reefStore)
                        .transition(.move(edge: .bottom).combined(with: .opacity))
                        .animation(.easeInOut(duration: 0.2), value: selectedSlot)
                }
            }

            if reefStore.diveEligibleCount >= 1 {
                // Compact record toggle — replaces always-visible recording bar
                if !showRecordingControls {
                    Button(action: { withAnimation { showRecordingControls = true } }) {
                        HStack(spacing: 4) {
                            Image(systemName: "record.circle")
                                .font(.system(size: 10))
                            Text("REC")
                                .font(DesignTokens.mono(9))
                        }
                        .foregroundColor(.white.opacity(0.3))
                        .padding(.horizontal, 12)
                        .padding(.vertical, 4)
                        .background(Color.white.opacity(0.04))
                        .clipShape(Capsule())
                    }
                    .padding(.top, 2)
                }

                if showRecordingControls {
                    HStack {
                        Spacer()
                        Button(action: { withAnimation { showRecordingControls = false } }) {
                            Image(systemName: "chevron.down.circle.fill")
                                .font(.system(size: 14))
                                .foregroundColor(.white.opacity(0.3))
                        }
                    }
                    .padding(.horizontal, 16)

                    ReefRecordingBar(
                        recorder: recorder,
                        loopRecorder: loopRecorder,
                        metronome: metronome
                    )
                    .transition(.move(edge: .bottom).combined(with: .opacity))
                }

                ReefKeyboardHeader(
                    activeSourceSlot: $activeSourceSlot,
                    keyboardScale: $keyboardScale,
                    keyboardMode: $keyboardMode,
                    octaveOffset: $octaveOffset,
                    ambientEnabled: $ambientEnabled,
                    showPerformanceMode: $showPerformanceMode,
                    metronome: metronome,
                    ambientManager: ambientManager
                )

                PlayKeyboard(
                    onNoteOn: { midiNote, velocity in
                        let sourceSlot = activeSourceSlot ?? firstSourceSlot
                        if let s = sourceSlot {
                            audioEngine.applyCachedParams(for: s)
                        }
                        ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: velocity)
                        recorder.recordNoteOn(midiNote: midiNote, velocity: velocity)
                        loopRecorder.recordNoteOn(midiNote: midiNote, velocity: velocity)
                        challengeManager.incrementProgress(type: .playNotes)
                        ReefStatsTracker.shared.increment(.notesPlayed)
                        let notesPlayed = ReefStatsTracker.shared.value(for: .notesPlayed)
                        // Fire milestones exactly once at each threshold
                        if notesPlayed == 100  { milestoneManager.increment("play_100") }
                        else if notesPlayed == 1000  { milestoneManager.increment("play_1000") }
                        else if notesPlayed == 10000 { milestoneManager.increment("play_10000") }
                        // Award badges at exact thresholds only
                        if notesPlayed == 1     { BadgeManager.shared.award("first_note") }
                        if notesPlayed == 100   { BadgeManager.shared.award("100_notes") }
                        if notesPlayed == 1000  { BadgeManager.shared.award("1000_notes") }
                        if notesPlayed == 10000 { BadgeManager.shared.award("10000_notes") }
                        ReefEnergyManager.shared.earnFromPlay(amount: encounterManager.energyMultiplier)
                        ambientResumeTimer?.invalidate()
                        ambientResumeTimer = nil
                        if ambientEnabled { ambientManager.stop() }
                        if OSCSender.shared.isConnected {
                            OSCSender.shared.sendNoteOn(note: midiNote, velocity: velocity)
                        }
                    },
                    onNoteOff: { midiNote in
                        ObrixBridge.shared()?.noteOff(Int32(midiNote))
                        recorder.recordNoteOff(midiNote: midiNote)
                        loopRecorder.recordNoteOff(midiNote: midiNote)
                        ambientResumeTimer?.invalidate()
                        if ambientEnabled {
                            let t = Timer(timeInterval: 3.0, repeats: false) { [self] _ in
                                ambientManager.start(reefStore: reefStore, audioEngine: audioEngine)
                            }
                            RunLoop.main.add(t, forMode: .common)
                            ambientResumeTimer = t
                        }
                        if OSCSender.shared.isConnected {
                            OSCSender.shared.sendNoteOff(note: midiNote)
                        }
                    },
                    accentColor: DesignTokens.reefJade,
                    octaveOffset: $octaveOffset,
                    scale: keyboardScale,
                    mode: keyboardMode,
                    syncBPM: metronome.isRunning ? metronome.bpm : nil,
                    onExpression: { filterMod, pitchBend in
                        ObrixBridge.shared()?.setParameterImmediate("obrix_proc1Cutoff",
                                                                     value: 2000 + filterMod * 10000)
                        ObrixBridge.shared()?.setParameterImmediate("obrix_src1Tune",
                                                                     value: pitchBend * 2.0)
                        if OSCSender.shared.isConnected {
                            OSCSender.shared.sendExpression(filterMod: filterMod, pitchBend: pitchBend)
                        }
                    }
                )
                .frame(height: 64)
                .clipShape(RoundedRectangle(cornerRadius: 12))
                .padding(.horizontal, 16)
                .padding(.bottom, 8)
                .transition(.move(edge: .bottom).combined(with: .opacity))
            } else {
                Text("Add a specimen to play")
                    .font(DesignTokens.body(12))
                    .foregroundColor(DesignTokens.mutedText)
                    .padding(.bottom, 16)
            }
        }
        .animation(.easeInOut(duration: 0.3), value: reefStore.diveEligibleCount)
        .animation(.easeInOut(duration: 0.2), value: showRecordingControls)
        .background(DesignTokens.background)
        .onChange(of: recorder.isRecording) { isRec in
            if isRec { withAnimation { showRecordingControls = true } }
        }
        .onAppear {
            if activeSourceSlot == nil {
                activeSourceSlot = firstSourceSlot
            }
            encounterManager.checkForEncounter()
        }
        .onDisappear {
            ambientManager.stop()
            ambientResumeTimer?.invalidate()
            ambientResumeTimer = nil
            gridRefreshTimer?.invalidate()
            gridRefreshTimer = nil
        }
        .fullScreenCover(isPresented: $showPerformanceMode) {
            PerformanceMode(activeSourceSlot: activeSourceSlot ?? firstSourceSlot)
                .environmentObject(audioEngine)
                .environmentObject(reefStore)
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
                    .font(DesignTokens.mono(10))
                    .tracking(1.5)
                    .foregroundColor(DesignTokens.xoGold)
                Text("— tap to place in slot \(targetSlot + 1)")
                    .font(DesignTokens.body(10))
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
                    .font(DesignTokens.body(11))
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
                                        .font(DesignTokens.body(8))
                                        .foregroundColor(.white.opacity(0.6))
                                        .lineLimit(1)
                                    Text("Lv.\(specimen.level)")
                                        .font(DesignTokens.mono(7))
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
        .background(DesignTokens.panelBackground)
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
                                .font(DesignTokens.heading(14))
                                .foregroundColor(.white)
                            Text(preset.savedAt, style: .date)
                                .font(DesignTokens.mono(10))
                                .foregroundColor(.white.opacity(0.4))
                            Text("\(preset.specimenSlots.compactMap { $0 }.count) specimens, \(preset.couplingRoutes.count) wires")
                                .font(DesignTokens.body(10))
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
            .background(DesignTokens.background)
        }
    }
}

// MARK: - ReefAccessibilityOverlay

/// Transparent VoiceOver overlay for the SpriteKit reef grid.
/// SpriteKit nodes are invisible to accessibility; this SwiftUI layer provides
/// one accessible hit-target per reef slot so VoiceOver users can navigate
/// and interact with every specimen without touching SpriteKit directly.
struct ReefAccessibilityOverlay: View {
    let reefStore: ReefStore
    let gridSize: CGFloat

    private let gridCount = 4

    var body: some View {
        let cellSize = gridSize / CGFloat(gridCount)

        ZStack {
            ForEach(0..<16, id: \.self) { index in
                let row = index / gridCount
                let col = index % gridCount
                let x = CGFloat(col) * cellSize + cellSize / 2
                let y = CGFloat(row) * cellSize + cellSize / 2

                Color.clear
                    .frame(width: cellSize * 0.9, height: cellSize * 0.9)
                    .position(x: x, y: y)
                    .accessibilityElement()
                    .accessibilityLabel(slotLabel(index))
                    .accessibilityHint(slotHint(index))
                    .accessibilityAddTraits(.isButton)
            }
        }
        .accessibilityElement(children: .contain)
    }

    private func slotLabel(_ index: Int) -> String {
        if let spec = reefStore.specimens[index] {
            return "\(spec.creatureName), \(spec.category.rawValue), level \(spec.level)"
        }
        return "Empty slot \(index + 1)"
    }

    private func slotHint(_ index: Int) -> String {
        if reefStore.specimens[index] != nil {
            return "Tap to preview sound. Long press to wire."
        }
        return "Tap to place a specimen from stasis."
    }
}
