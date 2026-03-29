import SwiftUI

/// Full-screen keyboard for performance — reef grid hidden, maximum key area.
/// Enters landscape orientation automatically; restores portrait on dismiss.
struct PerformanceMode: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @Environment(\.dismiss) private var dismiss

    @State private var octaveOffset: Int = 0
    @State private var keyboardScale: KeyboardScale = .pentatonic
    @State private var keyboardMode: KeyboardMode = .single
    @StateObject private var audioExporter = AudioExporter()
    @State private var isAudioRecording = false

    // Playback source
    let activeSourceSlot: Int?

    var body: some View {
        ZStack {
            DesignTokens.darkBackground.ignoresSafeArea()

            VStack(spacing: 0) {
                // Compact header strip — source name + controls + close
                HStack(spacing: 12) {
                    // Close button — leading edge in landscape
                    Button(action: { dismiss() }) {
                        Image(systemName: "xmark.circle.fill")
                            .font(.system(size: 20))
                            .foregroundColor(.white.opacity(0.3))
                    }
                    .accessibilityLabel("Close performance mode")

                    // Source info
                    if let slot = activeSourceSlot, let spec = reefStore.specimens[slot] {
                        HStack(spacing: 6) {
                            SpecimenSprite(subtype: spec.subtype, category: spec.category, size: 20)
                            Text(spec.creatureName)
                                .font(DesignTokens.mono(10))
                                .foregroundColor(DesignTokens.reefJade.opacity(0.7))
                                .lineLimit(1)
                        }
                    }

                    Spacer()

                    // Scale picker
                    Menu {
                        ForEach(KeyboardScale.allCases, id: \.self) { scale in
                            Button(scale.rawValue) { keyboardScale = scale }
                        }
                    } label: {
                        Text(keyboardScale.rawValue)
                            .font(DesignTokens.mono(9))
                            .foregroundColor(.white.opacity(0.4))
                    }

                    // Mode picker
                    Menu {
                        ForEach(KeyboardMode.allCases, id: \.self) { mode in
                            Button(mode.rawValue) { keyboardMode = mode }
                        }
                    } label: {
                        Text(keyboardMode.rawValue)
                            .font(DesignTokens.mono(9))
                            .foregroundColor(.white.opacity(0.4))
                    }

                    // Octave controls
                    Button("−") { if octaveOffset > -2 { octaveOffset -= 1 } }
                        .font(.system(size: 14, weight: .bold))
                        .foregroundColor(.white.opacity(0.5))
                        .accessibilityLabel("Lower octave")
                    Text("C\(4 + octaveOffset)")
                        .font(DesignTokens.monoBold(14))
                        .foregroundColor(.white.opacity(0.6))
                        .frame(width: 32)
                    Button("+") { if octaveOffset < 2 { octaveOffset += 1 } }
                        .font(.system(size: 14, weight: .bold))
                        .foregroundColor(.white.opacity(0.5))
                        .accessibilityLabel("Higher octave")

                    // Audio record
                    Button(action: {
                        if isAudioRecording {
                            audioExporter.stopLiveRecording()
                            isAudioRecording = false
                        } else {
                            audioExporter.startLiveRecording()
                            isAudioRecording = true
                        }
                    }) {
                        Circle()
                            .fill(isAudioRecording ? DesignTokens.errorRed : DesignTokens.errorRed.opacity(0.3))
                            .frame(width: 16, height: 16)
                    }
                }
                .padding(.horizontal, 12)
                .padding(.vertical, 6)

                // FULL WIDTH KEYBOARD — fills all remaining vertical space in landscape
                PlayKeyboard(
                    onNoteOn: { midiNote, velocity in
                        if let s = activeSourceSlot {
                            audioEngine.applyCachedParams(for: s)
                        }
                        ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: velocity)
                        HapticEngine.keyPress(velocity: velocity)
                    },
                    onNoteOff: { midiNote in
                        ObrixBridge.shared()?.noteOff(Int32(midiNote))
                    },
                    accentColor: DesignTokens.reefJade,
                    octaveOffset: $octaveOffset,
                    scale: keyboardScale,
                    mode: keyboardMode,
                    onExpression: { filterMod, pitchBend in
                        ObrixBridge.shared()?.setParameterImmediate("obrix_proc1Cutoff", value: 2000 + filterMod * 10000)
                        ObrixBridge.shared()?.setParameterImmediate("obrix_src1Tune", value: pitchBend * 2.0)
                    }
                )
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .ignoresSafeArea(.container, edges: .bottom)
            }
        }
        // Force landscape orientation while this view is on screen
        .background(LandscapeLock().frame(width: 0, height: 0))
        .statusBarHidden()
        .onDisappear {
            ObrixBridge.shared()?.allNotesOff()
            // Stop any active recording to prevent silent tail accumulation
            if isAudioRecording {
                audioExporter.stopLiveRecording()
                isAudioRecording = false
            }
        }
    }
}

// MARK: - Landscape Orientation Lock

private struct LandscapeLock: UIViewControllerRepresentable {
    func makeUIViewController(context: Context) -> LandscapeLockController {
        LandscapeLockController()
    }
    func updateUIViewController(_ uiViewController: LandscapeLockController, context: Context) {}
}

private class LandscapeLockController: UIViewController {
    override var supportedInterfaceOrientations: UIInterfaceOrientationMask {
        .landscape
    }
    override var preferredInterfaceOrientationForPresentation: UIInterfaceOrientation {
        .landscapeRight
    }
    override func viewDidAppear(_ animated: Bool) {
        super.viewDidAppear(animated)
        // Force rotation to landscape
        if #available(iOS 16.0, *) {
            setNeedsUpdateOfSupportedInterfaceOrientations()
            guard let windowScene = view.window?.windowScene else { return }
            let prefs = UIWindowScene.GeometryPreferences.iOS(interfaceOrientations: .landscape)
            windowScene.requestGeometryUpdate(prefs)
        } else {
            UIDevice.current.setValue(UIInterfaceOrientation.landscapeRight.rawValue, forKey: "orientation")
        }
    }
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        // Restore portrait when dismissing
        if #available(iOS 16.0, *) {
            guard let windowScene = view.window?.windowScene else { return }
            let prefs = UIWindowScene.GeometryPreferences.iOS(interfaceOrientations: .portrait)
            windowScene.requestGeometryUpdate(prefs)
        } else {
            UIDevice.current.setValue(UIInterfaceOrientation.portrait.rawValue, forKey: "orientation")
        }
    }
}
