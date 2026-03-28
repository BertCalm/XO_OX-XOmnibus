import SwiftUI

struct ContentView: View {
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager
    @State private var selectedTab = 0
    @State private var showMusicCatch = false

    var body: some View {
        ZStack {
            TabView(selection: $selectedTab) {
                ReefTab()
                    .tabItem {
                        Image(systemName: "waveform.circle.fill")
                        Text("Reef")
                    }
                    .tag(0)

                CatchTab()
                    .tabItem {
                        Image(systemName: "antenna.radiowaves.left.and.right")
                        Text("Catch")
                    }
                    .tag(1)

                DiveTab()
                    .tabItem {
                        Image(systemName: "arrow.down.to.line")
                        Text("Dive")
                    }
                    .tag(2)

                CollectionTab()
                    .tabItem {
                        Image(systemName: "square.grid.2x2.fill")
                        Text("Collection")
                    }
                    .tag(3)
            }
            .tint(Color(hex: "1E8B7E"))

            // Floating music catch button — hidden on Reef tab (keyboard is there)
            if selectedTab != 0 {
                VStack {
                    Spacer()
                    HStack {
                        Spacer()
                        Button(action: { showMusicCatch = true }) {
                            Image(systemName: "music.note")
                                .font(.system(size: 20, weight: .semibold))
                                .foregroundColor(.white)
                                .frame(width: 52, height: 52)
                                .background(musicCatchButtonBackground)
                        }
                        .opacity(firstLaunchManager.canMusicCatch ? 1.0 : 0.5)
                        .padding(.trailing, 20)
                        .padding(.bottom, 60)
                    }
                }
            }
        }
        .fullScreenCover(isPresented: $showMusicCatch) {
            MusicCatchFlow()
        }
    }

    @ViewBuilder
    private var musicCatchButtonBackground: some View {
        if firstLaunchManager.canMusicCatch {
            Circle()
                .fill(Color(hex: "1E8B7E"))
                .shadow(color: Color(hex: "1E8B7E").opacity(0.4), radius: 8, y: 4)
        } else {
            Circle()
                .fill(Color(hex: "0E0E10"))
                .overlay(
                    Circle()
                        .stroke(Color.white.opacity(0.15), lineWidth: 1)
                )
        }
    }
}

// CatchTab is defined in Sources/Catch/CatchTab.swift

// DiveTab is defined in Sources/UI/DiveTab.swift

// CollectionTab is defined in Sources/UI/CollectionTab.swift

// Hex color extension
extension Color {
    init(hex: String) {
        let hex = hex.trimmingCharacters(in: CharacterSet.alphanumerics.inverted)
        var int: UInt64 = 0
        Scanner(string: hex).scanHexInt64(&int)
        let r = Double((int >> 16) & 0xFF) / 255.0
        let g = Double((int >> 8) & 0xFF) / 255.0
        let b = Double(int & 0xFF) / 255.0
        self.init(red: r, green: g, blue: b)
    }
}
