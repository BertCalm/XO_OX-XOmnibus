import SwiftUI
import SpriteKit

struct ReefTab: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @State private var reefScene: ReefScene?

    var body: some View {
        VStack(spacing: 0) {
            // Reef name header
            HStack {
                Text(reefStore.reefName)
                    .font(.custom("SpaceGrotesk-Bold", size: 18))
                    .foregroundColor(.white)
                Spacer()
                Text("Depth: \(reefStore.totalDiveDepth)m")
                    .font(.custom("JetBrainsMono-Regular", size: 11))
                    .foregroundColor(Color(hex: "7A7876"))
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
                } else {
                    Color.clear
                        .onAppear {
                            reefScene = ReefScene(
                                size: CGSize(width: gridSize, height: gridSize),
                                reefStore: reefStore,
                                onNoteOn: { slot, velocity in
                                    audioEngine.noteOn(slotIndex: slot, velocity: velocity)
                                },
                                onNoteOff: { slot in
                                    audioEngine.noteOff(slotIndex: slot)
                                }
                            )
                        }
                }
            }

            // Dive button
            Button(action: {
                // Phase 2: Navigate to Dive
            }) {
                Text("DIVE")
                    .font(.custom("SpaceGrotesk-Bold", size: 14))
                    .tracking(2)
                    .foregroundColor(reefStore.diveEligibleCount >= 4 ? .white : Color(hex: "7A7876"))
                    .frame(maxWidth: .infinity)
                    .frame(height: 44)
                    .background(
                        RoundedRectangle(cornerRadius: 22)
                            .fill(reefStore.diveEligibleCount >= 4
                                  ? Color(hex: "1E8B7E")
                                  : Color(hex: "1A1A1C"))
                    )
            }
            .disabled(reefStore.diveEligibleCount < 4)
            .padding(.horizontal, 40)
            .padding(.bottom, 16)
        }
        .background(Color(hex: "0E0E10"))
    }
}
