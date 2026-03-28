import Foundation
import SwiftUI

struct HelpTopic: Identifiable {
    let id: String
    let title: String
    let icon: String
    let content: String  // Markdown-ish plain text
}

enum HelpContent {
    static let topics: [HelpTopic] = [
        HelpTopic(id: "catch", title: "Catching Specimens", icon: "antenna.radiowaves.left.and.right",
                  content: "Find specimens on the map and match their musical pattern to catch them. Different categories have different mini-games: Sources use pattern match, Processors use frequency sweep, Modulators use rhythm tap, Effects use echo memory."),

        HelpTopic(id: "wiring", title: "Wiring Your Reef", icon: "link",
                  content: "Long-press a specimen, then drag to another to create a wire. Wires connect your signal chain: Source → Processor → Effect. Modulators can wire to anything. Green wires are natural, yellow are unusual, red are invalid."),

        HelpTopic(id: "playing", title: "Playing Music", icon: "pianokeys",
                  content: "The keyboard at the bottom plays through your active source's wired chain. Tap a source on the reef to select it. Use scale modes to stay in key. Try chord and arp modes for richer sound."),

        HelpTopic(id: "leveling", title: "Leveling Up", icon: "arrow.up.circle",
                  content: "Specimens earn XP from every note you play through them. At level 5, parameters widen. At level 10, they evolve into one of two forms based on your play style (aggressive vs gentle)."),

        HelpTopic(id: "energy", title: "Reef Energy", icon: "bolt.fill",
                  content: "Earn energy by playing notes (up to 50/day). Spend on XP boosts (10⚡), extra music catches (20⚡), or spawn rerolls (5⚡). Cap: 100 energy."),

        HelpTopic(id: "dive", title: "The Dive", icon: "arrow.down.to.line",
                  content: "A 60-second generative performance. Touch to steer pitch and velocity. Tilt for filter modulation. Deeper zones = different scales + harder difficulty + more points."),

        HelpTopic(id: "music", title: "Song of the Day", icon: "music.note",
                  content: "Once per day, pick a song from your library. The song's metadata is hashed into a unique specimen — same song always creates the same creature type. #WhatsYourSong"),

        HelpTopic(id: "stasis", title: "Stasis Storage", icon: "archivebox",
                  content: "Move specimens to stasis to free reef slots. Stasis preserves the specimen but removes it from the active reef. Tap an empty slot to browse stasis and place a specimen."),

        HelpTopic(id: "fusion", title: "Specimen Fusion", icon: "arrow.triangle.merge",
                  content: "Two same-category specimens at Lv.3+ can fuse into a hybrid child. The child inherits averaged traits from both parents. Parents are consumed — choose wisely."),

        HelpTopic(id: "export", title: "Sharing & Export", icon: "square.and.arrow.up",
                  content: "Share creature cards, 15-second audio clips, MIDI files, and .xoreef configurations. Export your reef for desktop import or send to friends."),

        HelpTopic(id: "microscope", title: "The Microscope", icon: "magnifyingglass",
                  content: "Toggle between Creature View (game stats) and Microscope View (synth params) to see how game stats map to synthesis. Warmth = Cutoff, Intensity = Resonance, etc."),

        HelpTopic(id: "categories", title: "Specimen Categories", icon: "square.grid.2x2",
                  content: "Sources (circles) generate sound. Processors (squares) shape it. Modulators (diamonds) move parameters. Effects (hexagons) add space and color."),
    ]
}

// MARK: - HelpView

struct HelpView: View {
    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 0) {
                    ForEach(HelpContent.topics) { topic in
                        VStack(alignment: .leading, spacing: 6) {
                            HStack(spacing: 8) {
                                Image(systemName: topic.icon)
                                    .font(.system(size: 14))
                                    .foregroundColor(Color(hex: "1E8B7E"))
                                    .frame(width: 24)
                                Text(topic.title)
                                    .font(.custom("SpaceGrotesk-Bold", size: 14))
                                    .foregroundColor(.white)
                            }

                            Text(topic.content)
                                .font(.custom("Inter-Regular", size: 12))
                                .foregroundColor(.white.opacity(0.5))
                                .lineSpacing(3)
                        }
                        .padding(.horizontal, 20)
                        .padding(.vertical, 12)

                        Divider().background(Color.white.opacity(0.04))
                    }
                }
            }
            .background(Color(hex: "0E0E10"))
            .navigationTitle("Help")
            .navigationBarTitleDisplayMode(.inline)
        }
    }
}
