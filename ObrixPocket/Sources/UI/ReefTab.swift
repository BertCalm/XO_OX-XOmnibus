import SwiftUI
import SpriteKit

struct ReefTab: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager
    @State private var reefScene: ReefScene?
    @State private var activeSourceSlot: Int?  // Which source the keyboard plays through
    @State private var selectedSlot: Int?        // Which specimen's params are showing
    @State private var octaveOffset: Int = 0      // Keyboard octave shift (-2 to +2)

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

            // Parameter panel — shows when a specimen is tapped
            if let slot = selectedSlot, reefStore.specimens[slot] != nil {
                SpecimenParamPanel(slotIndex: slot, onDismiss: { selectedSlot = nil })
                    .transition(.move(edge: .bottom).combined(with: .opacity))
                    .animation(.easeInOut(duration: 0.2), value: selectedSlot)
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
                }
                .padding(.horizontal, 20)
                .padding(.bottom, 2)

                PlayKeyboard(
                    onNoteOn: { midiNote, velocity in
                        let sourceSlot = activeSourceSlot ?? firstSourceSlot
                        if let s = sourceSlot {
                            audioEngine.applySlotChain(slotIndex: s, reefStore: reefStore)
                        }
                        ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: velocity)
                    },
                    onNoteOff: { midiNote in
                        ObrixBridge.shared()?.noteOff(Int32(midiNote))
                    },
                    accentColor: Color(hex: "1E8B7E"),
                    octaveOffset: $octaveOffset
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
    }

    private var firstSourceSlot: Int? {
        reefStore.specimens.firstIndex(where: { $0?.category == .source })
    }
}
