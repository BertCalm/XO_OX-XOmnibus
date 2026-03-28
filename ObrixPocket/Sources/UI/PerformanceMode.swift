import SwiftUI

/// Full-screen keyboard for performance — reef grid hidden, maximum key area.
struct PerformanceMode: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @Environment(\.dismiss) private var dismiss

    @State private var octaveOffset: Int = 0
    @State private var keyboardScale: KeyboardScale = .pentatonic
    @State private var keyboardMode: KeyboardMode = .single

    // Playback source
    let activeSourceSlot: Int?

    var body: some View {
        ZStack {
            Color(hex: "0A0A0F").ignoresSafeArea()

            VStack(spacing: 0) {
                // Minimal header — source name + close button
                HStack {
                    // Source info
                    if let slot = activeSourceSlot, let spec = reefStore.specimens[slot] {
                        HStack(spacing: 6) {
                            SpecimenSprite(subtype: spec.subtype, category: spec.category, size: 24)
                            Text(spec.creatureName)
                                .font(.custom("JetBrainsMono-Regular", size: 11))
                                .foregroundColor(Color(hex: "1E8B7E").opacity(0.7))
                        }
                    }

                    Spacer()

                    // Controls
                    HStack(spacing: 12) {
                        // Scale
                        Menu {
                            ForEach(KeyboardScale.allCases, id: \.self) { scale in
                                Button(scale.rawValue) { keyboardScale = scale }
                            }
                        } label: {
                            Text(keyboardScale.rawValue)
                                .font(.custom("JetBrainsMono-Regular", size: 9))
                                .foregroundColor(.white.opacity(0.4))
                        }

                        // Mode
                        Menu {
                            ForEach(KeyboardMode.allCases, id: \.self) { mode in
                                Button(mode.rawValue) { keyboardMode = mode }
                            }
                        } label: {
                            Text(keyboardMode.rawValue)
                                .font(.custom("JetBrainsMono-Regular", size: 9))
                                .foregroundColor(.white.opacity(0.4))
                        }

                        // Octave
                        Button("−") { if octaveOffset > -2 { octaveOffset -= 1 } }
                            .font(.system(size: 14, weight: .bold))
                            .foregroundColor(.white.opacity(0.5))
                        Text("C\(4 + octaveOffset)")
                            .font(.custom("JetBrainsMono-Regular", size: 10))
                            .foregroundColor(.white.opacity(0.4))
                        Button("+") { if octaveOffset < 2 { octaveOffset += 1 } }
                            .font(.system(size: 14, weight: .bold))
                            .foregroundColor(.white.opacity(0.5))

                        // Close
                        Button(action: { dismiss() }) {
                            Image(systemName: "xmark.circle.fill")
                                .font(.system(size: 20))
                                .foregroundColor(.white.opacity(0.3))
                        }
                    }
                }
                .padding(.horizontal, 16)
                .padding(.vertical, 8)

                // FULL SCREEN KEYBOARD
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
                    accentColor: Color(hex: "1E8B7E"),
                    octaveOffset: $octaveOffset,
                    scale: keyboardScale,
                    mode: keyboardMode,
                    onExpression: { filterMod, pitchBend in
                        ObrixBridge.shared()?.setParameterImmediate("obrix_proc1Cutoff", value: 2000 + filterMod * 10000)
                        ObrixBridge.shared()?.setParameterImmediate("obrix_src1Tune", value: pitchBend * 2.0)
                    }
                )
                .ignoresSafeArea(.container, edges: .bottom)
            }
        }
        .statusBarHidden()
    }
}
