import SwiftUI

struct ContentView: View {
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager
    @EnvironmentObject var reefStore: ReefStore
    @State private var selectedTab = 0
    @State private var showMusicCatch = false
    @State private var dbErrorMessage: String? = nil
    @State private var showMusicCatchTooltip = false

    private let hasSeenMusicCatchTooltipKey = "obrix_pocket_seen_music_catch_tooltip"

    var body: some View {
        ZStack {
            TabView(selection: $selectedTab) {
                ReefTab()
                    .tabItem {
                        Image(systemName: "water.waves")
                        Text("Reef")
                    }
                    .tag(0)

                CatchTab()
                    .tabItem {
                        Image(systemName: "map")
                        Text("Catch")
                    }
                    .tag(1)

                DiveTab()
                    .tabItem {
                        Image(systemName: "arrow.down.to.line.compact")
                        Text("Dive")
                    }
                    .tag(2)

                CollectionTab()
                    .tabItem {
                        Image(systemName: "square.grid.2x2")
                        Text("Collection")
                    }
                    .tag(3)
            }
            .tint(DesignTokens.reefJade)

            // Floating music catch button — hidden on Reef tab (keyboard is there)
            if selectedTab != 0 {
                VStack {
                    Spacer()
                    HStack {
                        Spacer()
                        VStack(alignment: .trailing, spacing: 4) {
                            // First-appearance tooltip
                            if showMusicCatchTooltip {
                                Text("Catch a creature from any song in your library.")
                                    .font(DesignTokens.body(11))
                                    .foregroundColor(.white.opacity(0.85))
                                    .padding(.horizontal, 12)
                                    .padding(.vertical, 7)
                                    .background(
                                        RoundedRectangle(cornerRadius: 10)
                                            .fill(DesignTokens.panelBackground)
                                            .shadow(color: .black.opacity(0.4), radius: 6, y: 3)
                                    )
                                    .frame(maxWidth: 220, alignment: .trailing)
                                    .transition(.opacity.combined(with: .move(edge: .bottom)))
                            }

                            Button(action: {
                                showMusicCatchTooltip = false
                                showMusicCatch = true
                            }) {
                                VStack(spacing: 4) {
                                    Image(systemName: "music.note")
                                        .font(.system(size: 20, weight: .semibold))
                                        .foregroundColor(.white)
                                    Text("SONG")
                                        .font(DesignTokens.mono(8))
                                        .foregroundColor(.white.opacity(0.8))
                                }
                                .frame(width: 52, height: 52)
                                .background(musicCatchButtonBackground)
                            }
                            .accessibilityLabel("Song of the Day")
                            .accessibilityHint("Catch a specimen from your music library")
                            .opacity(firstLaunchManager.canMusicCatch ? 1.0 : 0.5)
                            .disabled(!firstLaunchManager.canMusicCatch)
                            .onAppear {
                                let seen = UserDefaults.standard.bool(forKey: hasSeenMusicCatchTooltipKey)
                                if !seen {
                                    withAnimation(.easeIn(duration: 0.3).delay(0.5)) {
                                        showMusicCatchTooltip = true
                                    }
                                    UserDefaults.standard.set(true, forKey: hasSeenMusicCatchTooltipKey)
                                    // Auto-hide tooltip after 5 seconds
                                    DispatchQueue.main.asyncAfter(deadline: .now() + 5.5) {
                                        withAnimation(.easeOut(duration: 0.3)) {
                                            showMusicCatchTooltip = false
                                        }
                                    }
                                }
                            }
                        }
                        .padding(.trailing, 20)
                        .padding(.bottom, 60)
                    }
                }
            }
        }
        .fullScreenCover(isPresented: $showMusicCatch) {
            MusicCatchFlow()
        }
        // Surface GRDB errors to the user — fixes #278 (print-only DB errors).
        .alert("Save Error", isPresented: Binding(
            get: { dbErrorMessage != nil },
            set: { if !$0 { dbErrorMessage = nil; reefStore.lastDBError = nil } }
        )) {
            Button("OK", role: .cancel) {
                dbErrorMessage = nil
                reefStore.lastDBError = nil
            }
        } message: {
            Text(dbErrorMessage ?? "An unknown error occurred saving your data.")
        }
        .onChange(of: reefStore.lastDBError) { error in
            if let error {
                dbErrorMessage = error
            }
        }
    }

    @ViewBuilder
    private var musicCatchButtonBackground: some View {
        if firstLaunchManager.canMusicCatch {
            Circle()
                .fill(DesignTokens.reefJade)
                .shadow(color: DesignTokens.reefJade.opacity(0.4), radius: 8, y: 4)
        } else {
            Circle()
                .fill(DesignTokens.background)
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
