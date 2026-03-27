import SwiftUI
import SpriteKit

struct ReefTab: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @State private var reefScene: ReefScene?
    @State private var activeSourceSlot: Int?  // Which source the keyboard plays through

    var body: some View {
        VStack(spacing: 0) {
            // Reef name header
            HStack {
                Text(reefStore.reefName)
                    .font(.custom("SpaceGrotesk-Bold", size: 18))
                    .foregroundColor(.white)
                Spacer()
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
                                    // If tapped specimen is a Source, make it the active keyboard source
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
                    Text("Tap a source to switch")
                        .font(.custom("Inter-Regular", size: 9))
                        .foregroundColor(.white.opacity(0.25))
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
                    accentColor: Color(hex: "1E8B7E")
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
