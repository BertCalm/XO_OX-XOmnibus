import SwiftUI

/// Keyboard control header: source label, scale picker, mode picker, octave controls,
/// metronome, motion toggle, ambient toggle, MIDI indicator, performance mode button.
///
/// metronome is owned by ReefTab and passed in (shared with ReefRecordingBar + PlayKeyboard syncBPM).
/// motionController and midiInput are owned here — ReefTab only needs to call stop() on disappear,
/// done via the onDisappear modifier below.
/// ambientEnabled is a Binding from ReefTab so note-on/off callbacks can also read/write it.
struct ReefKeyboardHeader: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore

    @Binding var activeSourceSlot: Int?
    @Binding var keyboardScale: KeyboardScale
    @Binding var keyboardMode: KeyboardMode
    @Binding var octaveOffset: Int
    @Binding var ambientEnabled: Bool
    @Binding var showPerformanceMode: Bool

    @ObservedObject var metronome: MetronomeManager

    // Owned by this view — only accessed from the keyboard header UI
    @StateObject private var motionController = MotionController()
    @StateObject private var midiInput = MIDIInputManager()

    // Ambient manager passed in from ReefTab (shared with onNoteOn/onNoteOff callbacks)
    var ambientManager: ReefAmbientManager

    var body: some View {
        HStack {
            // Source label + subtle synth identity bridge
            let slot = activeSourceSlot ?? firstSourceSlot
            let sourceName = slot.flatMap { reefStore.specimens[$0]?.creatureName } ?? "—"
            let sourceSubtype = slot.flatMap { reefStore.specimens[$0]?.subtype }
            VStack(alignment: .leading, spacing: 1) {
                Text("Playing: \(sourceName)")
                    .font(DesignTokens.mono(10))
                    .foregroundColor(DesignTokens.reefJade.opacity(0.7))
                if let subtype = sourceSubtype,
                   let entry = SpecimenCatalog.entry(for: subtype) {
                    // Derive short synth type from "Synth Name — detail" sonicCharacter format
                    let synthType = entry.sonicCharacter
                        .components(separatedBy: "—").first?
                        .trimmingCharacters(in: .whitespaces) ?? ""
                    if !synthType.isEmpty {
                        Text("Source: \(synthType)")
                            .font(DesignTokens.mono(8))
                            .foregroundColor(.white.opacity(0.15))
                    }
                }
            }

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
                    .font(DesignTokens.mono(9))
                    .foregroundColor(DesignTokens.reefJade.opacity(0.6))
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
                    .font(DesignTokens.mono(9))
                    .foregroundColor(DesignTokens.reefJade.opacity(0.6))
            }

            // Octave controls — 44pt minimum tap targets
            Button(action: { if octaveOffset > -2 { octaveOffset -= 1 } }) {
                Text("−")
                    .font(DesignTokens.heading(16))
                    .foregroundColor(octaveOffset > -2 ? .white : .white.opacity(0.2))
                    .frame(width: 28, height: 28)
            }
            .frame(minWidth: 44, minHeight: 44)
            .contentShape(Rectangle())
            .accessibilityLabel("Lower octave")

            Text("C\(4 + octaveOffset)")
                .font(DesignTokens.mono(10))
                .foregroundColor(.white.opacity(0.5))
                .frame(width: 24)

            Button(action: { if octaveOffset < 2 { octaveOffset += 1 } }) {
                Text("+")
                    .font(DesignTokens.heading(16))
                    .foregroundColor(octaveOffset < 2 ? .white : .white.opacity(0.2))
                    .frame(width: 28, height: 28)
            }
            .frame(minWidth: 44, minHeight: 44)
            .contentShape(Rectangle())
            .accessibilityLabel("Higher octave")

            // Performance mode — fullscreen keyboard
            Button(action: { showPerformanceMode = true }) {
                Image(systemName: "arrow.up.left.and.arrow.down.right")
                    .font(.system(size: 11))
                    .foregroundColor(DesignTokens.reefJade.opacity(0.5))
            }

            // Motion toggle
            Button(action: {
                if motionController.isEnabled { motionController.stop() }
                else { motionController.start() }
            }) {
                Image(systemName: "gyroscope")
                    .font(.system(size: 12))
                    .foregroundColor(motionController.isEnabled ? DesignTokens.reefJade : .white.opacity(0.3))
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
                    .foregroundColor(ambientEnabled ? DesignTokens.reefJade : .white.opacity(0.3))
            }

            // Tilt position dot — only visible when motion is active
            if motionController.isEnabled {
                HStack(spacing: 4) {
                    Text("TILT")
                        .font(DesignTokens.mono(8))
                        .foregroundColor(DesignTokens.reefJade.opacity(0.5))
                    Circle()
                        .fill(DesignTokens.reefJade)
                        .frame(width: 4, height: 4)
                        .offset(x: CGFloat(motionController.tiltX) * 10,
                                y: CGFloat(-motionController.tiltY) * 10)
                }
            }

            // MIDI input indicator — visible when external sources are connected
            if midiInput.connectedSources > 0 {
                HStack(spacing: 3) {
                    Image(systemName: "pianokeys")
                        .font(.system(size: 9))
                        .foregroundColor(DesignTokens.reefJade.opacity(0.5))
                    Text("MIDI")
                        .font(DesignTokens.mono(8))
                        .foregroundColor(.white.opacity(0.3))
                    if let note = midiInput.lastNoteReceived {
                        Text("N\(note)")
                            .font(DesignTokens.mono(8))
                            .foregroundColor(DesignTokens.reefJade.opacity(0.4))
                    }
                }
            }

            // Metronome toggle + BPM controls
            Button(action: { metronome.toggle() }) {
                Image(systemName: metronome.isRunning ? "metronome.fill" : "metronome")
                    .font(.system(size: 12))
                    .foregroundColor(metronome.isRunning ? DesignTokens.xoGold : .white.opacity(0.3))
            }

            if metronome.isRunning {
                Button(action: { metronome.bpm = max(60, metronome.bpm - 5) }) {
                    Text("−")
                        .font(.system(size: 14, weight: .bold))
                        .foregroundColor(.white.opacity(0.5))
                }

                Text("\(Int(metronome.bpm))")
                    .font(DesignTokens.monoBold(12))
                    .foregroundColor(DesignTokens.xoGold)
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
                            .fill(beat == metronome.currentBeat ? DesignTokens.xoGold : Color.white.opacity(0.15))
                            .frame(width: 6, height: 6)
                    }
                }
            }
        }
        .padding(.horizontal, 20)
        .padding(.bottom, 2)
        .onAppear {
            // Wire MIDI input to play through the active source chain
            midiInput.onNoteOn = { midiNote, velocity in
                let sourceSlot = activeSourceSlot ?? firstSourceSlot
                if let s = sourceSlot {
                    audioEngine.applyCachedParams(for: s)
                }
                ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: velocity)
                HapticEngine.keyPress(velocity: velocity)
            }
            midiInput.onNoteOff = { midiNote in
                ObrixBridge.shared()?.noteOff(Int32(midiNote))
            }
        }
        .onDisappear {
            motionController.stop()
            metronome.stop()
        }
    }

    private var firstSourceSlot: Int? {
        reefStore.specimens.firstIndex(where: { $0?.category == .source })
    }
}
