import SwiftUI

/// Performance recorder controls, audio recorder controls, loop recorder, and 15-second clip generator.
///
/// recorder and loopRecorder are owned by ReefTab and passed in as @ObservedObject so that
/// PlayKeyboard callbacks in ReefTab can also call recordNoteOn/Off on the same instances.
/// metronome is also owned by ReefTab (shared with ReefKeyboardHeader + PlayKeyboard syncBPM).
/// audioExporter is owned here — it is only accessed from this view.
struct ReefRecordingBar: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore

    @ObservedObject var recorder: PerformanceRecorder
    @ObservedObject var loopRecorder: LoopRecorder
    @ObservedObject var metronome: MetronomeManager

    @StateObject private var audioExporter = AudioExporter()

    @State private var isAudioRecording = false
    @State private var showAudioShare = false
    @State private var isGeneratingClip = false
    @State private var showClipShare = false
    @State private var clipShareItems: [Any] = []
    @State private var midiExportURL: URL?
    @State private var showMIDIExport = false

    var body: some View {
        VStack(spacing: 0) {
            // Performance recorder controls
            if recorder.hasRecording || recorder.isRecording {
                HStack(spacing: 12) {
                    // Record button
                    Button(action: {
                        if recorder.isRecording { recorder.stopRecording() }
                        else { recorder.startRecording() }
                    }) {
                        Circle()
                            .fill(recorder.isRecording ? DesignTokens.errorRed : DesignTokens.errorRed.opacity(0.5))
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
                                .foregroundColor(DesignTokens.reefJade)
                        }

                        Text(recorder.durationString)
                            .font(DesignTokens.mono(10))
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
                                .foregroundColor(DesignTokens.xoGold.opacity(0.6))
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
                                    .font(DesignTokens.mono(8))
                            }
                            .foregroundColor(DesignTokens.xoGold.opacity(0.5))
                        }
                    }

                    // Recording indicator — duration readout while active
                    if recorder.isRecording {
                        HStack(spacing: 6) {
                            Circle()
                                .fill(DesignTokens.errorRed)
                                .frame(width: 8, height: 8)
                            Text("REC \(recorder.durationString)")
                                .font(DesignTokens.mono(10))
                                .foregroundColor(DesignTokens.errorRed.opacity(0.7))
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
                                .fill(DesignTokens.errorRed.opacity(0.5))
                                .frame(width: 12, height: 12)
                            Text("REC")
                                .font(DesignTokens.mono(9))
                                .foregroundColor(.white.opacity(0.3))
                        }
                    }
                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 4)
            }

            // Audio recording (records actual audio output via JUCE tap)
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
                            .fill(isAudioRecording ? DesignTokens.errorRed : DesignTokens.errorRed.opacity(0.3))
                            .frame(width: 10, height: 10)
                        Text(isAudioRecording ? "STOP" : "REC AUDIO")
                            .font(DesignTokens.mono(8))
                            .foregroundColor(isAudioRecording ? DesignTokens.errorRed : .white.opacity(0.3))
                    }
                }

                if isAudioRecording {
                    Text(String(format: "%.1fs", audioExporter.recordingDuration))
                        .font(DesignTokens.mono(9))
                        .foregroundColor(DesignTokens.errorRed.opacity(0.6))
                }

                if audioExporter.lastExportURL != nil && !isAudioRecording {
                    Button(action: { showAudioShare = true }) {
                        HStack(spacing: 3) {
                            Image(systemName: "waveform")
                                .font(.system(size: 9))
                            Text("Share M4A")
                                .font(DesignTokens.mono(8))
                        }
                        .foregroundColor(DesignTokens.xoGold.opacity(0.5))
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
                            .font(DesignTokens.mono(8))
                    }
                    .foregroundColor(DesignTokens.xoGold.opacity(isGeneratingClip ? 0.3 : 0.5))
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
                        .fill(loopRecorder.isRecording ? DesignTokens.errorRed : DesignTokens.errorRed.opacity(0.4))
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
                        .foregroundColor(DesignTokens.reefJade)
                }
                .disabled(loopRecorder.layerCount == 0 && !loopRecorder.isRecording)

                // Layer count badge
                if loopRecorder.layerCount > 0 {
                    Text("\(loopRecorder.layerCount) layer\(loopRecorder.layerCount == 1 ? "" : "s")")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.3))
                }

                // Loop progress bar
                if loopRecorder.isPlaying {
                    GeometryReader { geo in
                        ZStack(alignment: .leading) {
                            RoundedRectangle(cornerRadius: 2)
                                .fill(Color.white.opacity(0.06))
                            RoundedRectangle(cornerRadius: 2)
                                .fill(DesignTokens.reefJade.opacity(0.4))
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
                            .foregroundColor(DesignTokens.errorRed.opacity(0.4))
                    }
                }

                Spacer()
            }
            .padding(.horizontal, 20)
            .padding(.bottom, DesignTokens.spacing4)
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
        .onDisappear {
            loopRecorder.stopPlayback() // Stop playback but KEEP layers
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
    }
}
