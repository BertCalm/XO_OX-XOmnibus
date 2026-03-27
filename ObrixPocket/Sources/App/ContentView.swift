import SwiftUI

struct ContentView: View {
    @State private var selectedTab = 0

    var body: some View {
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
        .tint(Color(hex: "1E8B7E")) // Reef Jade accent
    }
}

// Placeholder tabs for Phase 0
struct CatchTab: View {
    var body: some View {
        VStack {
            Image(systemName: "antenna.radiowaves.left.and.right")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            Text("Catch — Phase 1")
                .foregroundColor(.secondary)
        }
    }
}

struct DiveTab: View {
    var body: some View {
        VStack {
            Image(systemName: "arrow.down.to.line")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            Text("Dive — Phase 2")
                .foregroundColor(.secondary)
        }
    }
}

struct CollectionTab: View {
    var body: some View {
        VStack {
            Image(systemName: "square.grid.2x2.fill")
                .font(.system(size: 48))
                .foregroundColor(.secondary)
            Text("Collection — Phase 1")
                .foregroundColor(.secondary)
        }
    }
}

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
