import SwiftUI
import SpriteKit

struct ReefTab: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager
    @StateObject private var presetManager = ReefPresetManager()
    @State private var reefScene: ReefScene?
    @State private var activeSourceSlot: Int?  // Which source the keyboard plays through
    @State private var selectedSlot: Int?        // Which specimen's params are showing
    @State private var octaveOffset: Int = 0      // Keyboard octave shift (-2 to +2)
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
