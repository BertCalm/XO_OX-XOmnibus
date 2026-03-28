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
    @StateObject private var ambientManager = ReefAmbientManager()
    @StateObject private var metronome = MetronomeManager()
    @StateObject private var loopRecorder = LoopRecorder()
    @StateObject private var streakManager = StreakManager()
    @StateObject private var audioExporter = AudioExporter()
    @StateObject private var energyManager = ReefEnergyManager()
    @State private var isAudioRecording = false
    @State private var showAudioShare = false
    @State private var isGeneratingClip = false
    @State private var showClipShare = false
    @State private var clipShareItems: [Any] = []
    @State private var showStreakReward = false
    @State private var lastReward: StreakReward = .none
    @State private var reefScene: ReefScene?
    @State private var gridRefreshTimer: Timer?
    @State private var activeSourceSlot: Int?  // Which source the keyboard plays through
    @State private var selectedSlot: Int?        // Which specimen's params are showing
    @State private var octaveOffset: Int = 0      // Keyboard octave shift (-2 to +2)
    @State private var keyboardScale: KeyboardScale = .pentatonic  // Default: hardest to sound bad
    @State private var keyboardMode: KeyboardMode = .single
    @State private var showPerformanceMode = false
    @State private var showSaveDialog = false
    @State private var showLoadSheet = false
    @State private var presetName = ""
    @State private var ambientEnabled = false
    @State private var ambientResumeTimer: Timer?

    // Reef rename
    @State private var showReefRename = false
    @State private var editingReefName = ""

    // Reef theme
    @State private var reefTheme: ReefTheme = .ocean

    // .xoreef export
    @State private var reefExportURL: URL?
    @State private var showReefExport = false

    // MIDI export
    @State private var midiExportURL: URL?
    @State private var showMIDIExport = false

    var body: some View {
        VStack(spacing: 0) {
            // Reef name header
            HStack {
                // Tappable reef name — opens rename alert
                Button(action: {
                    editingReefName = reefStore.reefName
                    showReefRename = true
                }) {
                    Text(reefStore.reefName)
                        .font(.custom("SpaceGrotesk-Bold", size: 18))
                        .foregroundColor(.white)
                }

                // Theme picker
                Menu {
                    ForEach(ReefTheme.allCases, id: \.self) { theme in
                        Button(action: {
                            reefTheme = theme
                            reefScene?.theme = theme
                            UserDefaults.standard.set(theme.rawValue, forKey: "obrix_reef_theme")
                        }) {
                            HStack {
                                Text(theme.rawValue)
                                if theme == reefTheme {
                                    Image(systemName: "checkmark")
                                }
                            }
                        }
                    }
                } label: {
                    Image(systemName: "paintpalette")
                        .font(.system(size: 12))
                        .foregroundColor(.white.opacity(0.3))
                }

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
                // Save / Load preset buttons + .xoreef export
                HStack(spacing: 8) {
                    Button(action: { showSaveDialog = true }) {
                        Image(systemName: "square.and.arrow.down")
                            .font(.system(size: 12))
                            .foregroundColor(Color(hex: "1E8B7E").opacity(0.6))
                    }
                    if !presetManager.presets.isEmpty {
                        Button(action: { showLoadSheet = true }) {
                            Image(systemName: "list.bullet")
                                .font(.system(size: 12))
                                .foregroundColor(Color(hex: "1E8B7E").opacity(0.6))
                        }
                    }
                    Button(action: {
                        if let url = XOReefExporter.exportToFile(reefStore: reefStore) {
                            reefExportURL = url
                            showReefExport = true
                        }
                    }) {
                        Image(systemName: "square.and.arrow.up")
                            .font(.system(size: 12))
                            .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
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

            // Underwater header decoration — subtle texture accent between header and reef grid
            if let headerDecor = UIImage(named: "UIHeader") {
                Image(uiImage: headerDecor)
                    .resizable()
                    .interpolation(.none)
                    .frame(height: 8)
                    .opacity(0.1) // Very subtle
                    .padding(.horizontal, 20)
            }

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

            // Reef Energy currency display
            HStack(spacing: 6) {
                Image(systemName: "bolt.fill")
                    .font(.system(size: 9))
                    .foregroundColor(Color(hex: "E9C46A"))
                Text("\(energyManager.currentEnergy)")
                    .font(.custom("JetBrainsMono-Bold", size: 11))
                    .foregroundColor(Color(hex: "E9C46A"))
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
                                .fill(Color(hex: "E9C46A").opacity(0.35))
                                .frame(width: geo.size.width * CGFloat(energyManager.dailyEnergyEarned) / CGFloat(ReefEnergyManager.dailyEarnCap))
                        }
                    }
                    .frame(width: 40, height: 3)
                } else {
                    Image(systemName: "checkmark.circle.fill")
                        .font(.system(size: 8))
                        .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
                }
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 4)

            // Streak indicator
            HStack(spacing: 6) {
                Image(systemName: "flame.fill")
                    .font(.system(size: 10))
                    .foregroundColor(streakManager.currentStreak >= 7 ? Color(hex: "E9C46A") : Color(hex: "FF6B35").opacity(0.6))

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
                            .background(RoundedRectangle(cornerRadius: 4).fill(Color(hex: "FF6B35")))
                    }
                }

                Spacer()

                Text("next milestone: day \(streakManager.nextMilestone)")
                    .font(.custom("JetBrainsMono-Regular", size: 8))
                    .foregroundColor(.white.opacity(0.2))
            }
            .padding(.horizontal, 20)
            .padding(.bottom, 4)

            // The Reef Grid (SpriteKit scene)
            GeometryReader { geometry in
                let gridSize = min(geometry.size.width * 0.9, geometry.size.height * 0.85)

                if let scene = reefScene {
                    SpriteView(scene: scene)
                        .frame(width: gridSize, height: gridSize)
                        .clipShape(RoundedRectangle(cornerRadius: 16))
                        .position(x: geometry.size.width / 2, y: geometry.size.height * 0.45)
                        .onReceive(reefStore.objectWillChange) { _ in
                            // Debounce: rapid @Published changes (e.g. slider drags)
                            // collapse into one rebuild per 150ms instead of rebuilding
                            // on every tick, which was causing frame drops.
                            gridRefreshTimer?.invalidate()
                            gridRefreshTimer = Timer.scheduledTimer(withTimeInterval: 0.15, repeats: false) { _ in
                                reefScene?.refreshGrid()
                            }
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
                    // Mode selector
                    Menu {
                        ForEach(KeyboardMode.allCases, id: \.self) { kbMode in
                            Button(action: { keyboardMode = kbMode }) {
                                HStack {
                                    Text(kbMode.rawValue)
                                    if kbMode == keyboardMode {
                                        Image(systemName: "checkmark")
                                    }
                                }
                            }
                        }
                    } label: {
                        Text(keyboardMode.rawValue)
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
                    // Performance mode — fullscreen keyboard
                    Button(action: { showPerformanceMode = true }) {
                        Image(systemName: "arrow.up.left.and.arrow.down.right")
                            .font(.system(size: 11))
                            .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
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
                    // Ambient toggle
                    Button(action: {
                        ambientEnabled.toggle()
                        if ambientEnabled {
                            ambientManager.start(reefStore: reefStore, audioEngine: audioEngine)
                        } else {
                            ambientManager.stop()
                        }
                    }) {
                        Image(systemName: ambientEnabled ? "speaker.wave.2.fill" : "speaker.slash")
                            .font(.system(size: 11))
                            .foregroundColor(ambientEnabled ? Color(hex: "1E8B7E") : .white.opacity(0.3))
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

                            // Share JSON button
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

                            // MIDI export button
                            Button(action: {
                                if let url = MIDIExporter.exportToFile(
                                    events: recorder.exportEvents(),
                                    name: "reef_performance",
                                    bpm: metronome.bpm
                                ) {
                                    midiExportURL = url
                                    showMIDIExport = true
                                }
                            }) {
                                HStack(spacing: 3) {
                                    Image(systemName: "waveform")
                                        .font(.system(size: 10))
                                    Text("MIDI")
                                        .font(.custom("JetBrainsMono-Regular", size: 8))
                                }
                                .foregroundColor(Color(hex: "E9C46A").opacity(0.5))
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

                // Audio recording button (records actual audio output via JUCE tap)
                HStack(spacing: 8) {
                    Button(action: {
                        if isAudioRecording {
                            audioExporter.stopLiveRecording()
                            isAudioRecording = false
                            if audioExporter.lastExportURL != nil {
                                showAudioShare = true
                            }
                        } else {
                            audioExporter.startLiveRecording()
                            isAudioRecording = true
                        }
                    }) {
                        HStack(spacing: 4) {
                            Circle()
                                .fill(isAudioRecording ? Color(hex: "FF4D4D") : Color(hex: "FF4D4D").opacity(0.3))
                                .frame(width: 10, height: 10)
                            Text(isAudioRecording ? "STOP" : "REC AUDIO")
                                .font(.custom("JetBrainsMono-Regular", size: 8))
                                .foregroundColor(isAudioRecording ? Color(hex: "FF4D4D") : .white.opacity(0.3))
                        }
                    }

                    if isAudioRecording {
                        Text(String(format: "%.1fs", audioExporter.recordingDuration))
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .foregroundColor(Color(hex: "FF4D4D").opacity(0.6))
                    }

                    if audioExporter.lastExportURL != nil && !isAudioRecording {
                        Button(action: { showAudioShare = true }) {
                            HStack(spacing: 3) {
                                Image(systemName: "waveform")
                                    .font(.system(size: 9))
                                Text("Share M4A")
                                    .font(.custom("JetBrainsMono-Regular", size: 8))
                            }
                            .foregroundColor(Color(hex: "E9C46A").opacity(0.5))
                        }
                    }

                    // 15-second social clip — auto-plays a pentatonic melody + records output
                    Button(action: {
                        guard !isGeneratingClip && !isAudioRecording else { return }
                        isGeneratingClip = true
                        audioExporter.generateSocialClip(
                            reefStore: reefStore,
                            audioEngine: audioEngine
                        ) { url in
                            isGeneratingClip = false
                            if let audioURL = url {
                                // Bundle audio + share card for maximum social impact
                                let firstSpec = reefStore.specimens.compactMap { $0 }.first
                                    ?? SpecimenFactory.createStarter()
                                let cardImage = ShareCardGenerator.generateCard(for: firstSpec)
                                clipShareItems = [audioURL, cardImage]
                                showClipShare = true
                            }
                        }
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: isGeneratingClip ? "hourglass" : "square.and.arrow.up")
                                .font(.system(size: 10))
                            Text(isGeneratingClip ? "Recording..." : "15s Clip")
                                .font(.custom("JetBrainsMono-Regular", size: 8))
                        }
                        .foregroundColor(Color(hex: "E9C46A").opacity(isGeneratingClip ? 0.3 : 0.5))
                    }
                    .disabled(isGeneratingClip || isAudioRecording)

                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 4)

                // Loop recorder controls
                HStack(spacing: 10) {
                    // Loop record / overdub button
                    Button(action: {
                        if loopRecorder.isRecording { loopRecorder.stopRecording() }
                        else { loopRecorder.startRecording() }
                    }) {
                        Circle()
                            .fill(loopRecorder.isRecording ? Color(hex: "FF4D4D") : Color(hex: "FF4D4D").opacity(0.4))
                            .frame(width: 20, height: 20)
                            .overlay(
                                loopRecorder.isRecording ?
                                AnyView(RoundedRectangle(cornerRadius: 2).fill(.white).frame(width: 8, height: 8)) :
                                AnyView(Circle().fill(.white).frame(width: 8, height: 8))
                            )
                    }

                    // Play / stop loop playback
                    Button(action: { loopRecorder.togglePlayback() }) {
                        Image(systemName: loopRecorder.isPlaying ? "stop.fill" : "play.fill")
                            .font(.system(size: 12))
                            .foregroundColor(Color(hex: "1E8B7E"))
                    }
                    .disabled(loopRecorder.layerCount == 0 && !loopRecorder.isRecording)

                    // Layer count badge
                    if loopRecorder.layerCount > 0 {
                        Text("\(loopRecorder.layerCount) layer\(loopRecorder.layerCount == 1 ? "" : "s")")
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.3))
                    }

                    // Loop progress bar
                    if loopRecorder.isPlaying {
                        GeometryReader { geo in
                            ZStack(alignment: .leading) {
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(Color.white.opacity(0.06))
                                RoundedRectangle(cornerRadius: 2)
                                    .fill(Color(hex: "1E8B7E").opacity(0.4))
                                    .frame(width: geo.size.width * CGFloat(loopRecorder.loopProgress))
                            }
                        }
                        .frame(width: 60, height: 4)
                    }

                    // Undo last layer
                    if loopRecorder.layerCount > 0 {
                        Button(action: { loopRecorder.undoLastLayer() }) {
                            Image(systemName: "arrow.uturn.backward")
                                .font(.system(size: 10))
                                .foregroundColor(.white.opacity(0.3))
                        }
                    }

                    // Clear all layers
                    if loopRecorder.layerCount > 0 {
                        Button(action: { loopRecorder.clearAllLayers() }) {
                            Image(systemName: "trash")
                                .font(.system(size: 10))
                                .foregroundColor(Color(hex: "FF4D4D").opacity(0.4))
                        }
                    }

                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 2)

                // Metronome controls
                HStack(spacing: 8) {
                    // Toggle
                    Button(action: { metronome.toggle() }) {
                        Image(systemName: metronome.isRunning ? "metronome.fill" : "metronome")
                            .font(.system(size: 12))
                            .foregroundColor(metronome.isRunning ? Color(hex: "E9C46A") : .white.opacity(0.3))
                    }

                    if metronome.isRunning {
                        // BPM stepper
                        Button(action: { metronome.bpm = max(60, metronome.bpm - 5) }) {
                            Text("−")
                                .font(.system(size: 14, weight: .bold))
                                .foregroundColor(.white.opacity(0.5))
                        }

                        Text("\(Int(metronome.bpm))")
                            .font(.custom("JetBrainsMono-Bold", size: 12))
                            .foregroundColor(Color(hex: "E9C46A"))
                            .frame(width: 32)

                        Button(action: { metronome.bpm = min(200, metronome.bpm + 5) }) {
                            Text("+")
                                .font(.system(size: 14, weight: .bold))
                                .foregroundColor(.white.opacity(0.5))
                        }

                        // Beat dots
                        HStack(spacing: 4) {
                            ForEach(0..<metronome.beatsPerBar, id: \.self) { beat in
                                Circle()
                                    .fill(beat == metronome.currentBeat ? Color(hex: "E9C46A") : Color.white.opacity(0.15))
                                    .frame(width: 6, height: 6)
                            }
                        }
                    }

                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 2)

                PlayKeyboard(
                    onNoteOn: { midiNote, velocity in
                        let sourceSlot = activeSourceSlot ?? firstSourceSlot
                        // I-08: Use cached params instead of the full applySlotChain walk.
                        // applySlotChain re-reads reefStore and walks wiring on every keypress;
                        // the cache is kept current by applyReefConfiguration on every wiring change.
                        if let s = sourceSlot {
                            audioEngine.applyCachedParams(for: s)
                        }
                        ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: velocity)
                        recorder.recordNoteOn(midiNote: midiNote, velocity: velocity)
                        loopRecorder.recordNoteOn(midiNote: midiNote, velocity: velocity)
                        challengeManager.incrementProgress(type: .playNotes)
                        milestoneManager.increment("play_100")
                        milestoneManager.increment("play_1000")
                        milestoneManager.increment("play_10000")
                        ReefStatsTracker.shared.increment(.notesPlayed)
                        // Reef Energy: earn 1 energy per note, capped at 50/day
                        energyManager.earnFromPlay(amount: 1)
                        // Pause ambient while user is playing; cancel any pending resume
                        ambientResumeTimer?.invalidate()
                        ambientResumeTimer = nil
                        if ambientEnabled { ambientManager.stop() }
                    },
                    onNoteOff: { midiNote in
                        ObrixBridge.shared()?.noteOff(Int32(midiNote))
                        recorder.recordNoteOff(midiNote: midiNote)
                        loopRecorder.recordNoteOff(midiNote: midiNote)
                        // Resume ambient 3 seconds after the last key release
                        ambientResumeTimer?.invalidate()
                        if ambientEnabled {
                            ambientResumeTimer = Timer.scheduledTimer(withTimeInterval: 3.0, repeats: false) { _ in
                                ambientManager.start(reefStore: reefStore, audioEngine: audioEngine)
                            }
                        }
                    },
                    accentColor: Color(hex: "1E8B7E"),
                    octaveOffset: $octaveOffset,
                    scale: keyboardScale,
                    mode: keyboardMode,
                    syncBPM: metronome.isRunning ? metronome.bpm : nil,
                    onExpression: { filterMod, pitchBend in
                        // Y position (0=bottom, 1=top) modulates filter cutoff: low touch = closed, high = open
                        ObrixBridge.shared()?.setParameterImmediate("obrix_proc1Cutoff",
                                                                     value: 2000 + filterMod * 10000)
                        // X position within key bends pitch ±2 semitones via src1 tuning
                        ObrixBridge.shared()?.setParameterImmediate("obrix_src1Tune",
                                                                     value: pitchBend * 2.0)
                    }
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
            // Reset daily energy counter if it's a new day
            energyManager.resetDailyIfNeeded()
            // Restore saved reef theme
            let savedRaw = UserDefaults.standard.string(forKey: "obrix_reef_theme") ?? "Ocean"
            let saved = ReefTheme(rawValue: savedRaw) ?? .ocean
            reefTheme = saved
            reefScene?.theme = saved
        }
        .onDisappear {
            motionController.stop()
            ambientManager.stop()
            metronome.stop()
            loopRecorder.stopPlayback() // Stop playback but KEEP layers
            ambientResumeTimer?.invalidate()
            ambientResumeTimer = nil
            gridRefreshTimer?.invalidate()
            gridRefreshTimer = nil
        }
        // Fire loop playback events at ~30 Hz from the main thread
        .onReceive(Timer.publish(every: 1.0 / 30.0, on: .main, in: .common).autoconnect()) { _ in
            guard loopRecorder.isPlaying else { return }
            loopRecorder.playEvents(
                noteOn: { note, vel in ObrixBridge.shared()?.note(on: Int32(note), velocity: vel) },
                noteOff: { note in ObrixBridge.shared()?.noteOff(Int32(note)) }
            )
        }
        // Sync loop duration to metronome BPM
        .onChange(of: metronome.bpm) { newBPM in
            loopRecorder.bpm = newBPM
        }
        // Auto-stop: AudioExporter hit maxDuration and stopped itself
        .onChange(of: audioExporter.isRecording) { recording in
            if !recording && isAudioRecording {
                isAudioRecording = false
                if audioExporter.lastExportURL != nil {
                    showAudioShare = true
                }
            }
        }
        .alert("Rename Reef", isPresented: $showReefRename) {
            TextField("Reef name", text: $editingReefName)
            Button("Save") {
                if !editingReefName.isEmpty {
                    reefStore.reefName = editingReefName
                    reefStore.save()
                }
            }
            Button("Cancel", role: .cancel) {}
        } message: {
            Text("Give your reef a name")
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
        .alert("Streak Reward!", isPresented: $showStreakReward) {
            Button("Nice!") {}
        } message: {
            Text("Day \(streakManager.currentStreak): \(lastReward.description)")
        }
        .sheet(isPresented: $showLoadSheet) {
            ReefPresetList(
                presetManager: presetManager,
                reefStore: reefStore,
                onDismiss: { showLoadSheet = false }
            )
        }
        .sheet(isPresented: $showReefExport) {
            if let url = reefExportURL {
                ShareSheet(items: [url])
            }
        }
        .sheet(isPresented: $showMIDIExport) {
            if let url = midiExportURL {
                ShareSheet(items: [url])
            }
        }
        .sheet(isPresented: $showAudioShare) {
            if let url = audioExporter.lastExportURL {
                ShareSheet(items: [url])
            }
        }
        .sheet(isPresented: $showClipShare) {
            ShareSheet(items: clipShareItems)
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
