import SwiftUI

/// A playable keyboard strip for the reef.
/// Sends noteOn/noteOff through the wired engine chain.
/// 2 octaves (C3-B4), touch-responsive, category-colored.
struct PlayKeyboard: View {
    let onNoteOn: (Int, Float) -> Void  // (midiNote, velocity)
    let onNoteOff: (Int) -> Void
    let accentColor: Color

    // 2 octaves starting at C3 (MIDI 48)
    private let startNote = 48
    private let whiteNotes = [0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23] // C D E F G A B × 2
    private let blackNotes = [1, 3, 6, 8, 10, 13, 15, 18, 20, 22] // C# D# F# G# A# × 2

    @State private var activeNotes: Set<Int> = []

    var body: some View {
        GeometryReader { geometry in
            let whiteKeyWidth = geometry.size.width / CGFloat(whiteNotes.count)
            let blackKeyWidth = whiteKeyWidth * 0.6
            let whiteKeyHeight = geometry.size.height
            let blackKeyHeight = whiteKeyHeight * 0.6

            ZStack(alignment: .top) {
                // White keys
                HStack(spacing: 0) {
                    ForEach(whiteNotes, id: \.self) { offset in
                        let midiNote = startNote + offset
                        let isActive = activeNotes.contains(midiNote)

                        Rectangle()
                            .fill(isActive ? accentColor.opacity(0.4) : Color(hex: "1A1A1C"))
                            .frame(width: whiteKeyWidth, height: whiteKeyHeight)
                            .border(Color.white.opacity(0.08), width: 0.5)
                            .gesture(
                                DragGesture(minimumDistance: 0)
                                    .onChanged { _ in
                                        if !activeNotes.contains(midiNote) {
                                            activeNotes.insert(midiNote)
                                            onNoteOn(midiNote, 0.8)
                                        }
                                    }
                                    .onEnded { _ in
                                        activeNotes.remove(midiNote)
                                        onNoteOff(midiNote)
                                    }
                            )
                    }
                }

                // Black keys (overlaid)
                HStack(spacing: 0) {
                    ForEach(0..<whiteNotes.count, id: \.self) { whiteIndex in
                        let whiteOffset = whiteNotes[whiteIndex]
                        let hasBlack = blackNotes.contains(whiteOffset + 1)

                        ZStack {
                            Color.clear.frame(width: whiteKeyWidth)

                            if hasBlack {
                                let blackMidi = startNote + whiteOffset + 1
                                let isActive = activeNotes.contains(blackMidi)

                                Rectangle()
                                    .fill(isActive ? accentColor : Color(hex: "0A0A0F"))
                                    .frame(width: blackKeyWidth, height: blackKeyHeight)
                                    .cornerRadius(0, corners: [.bottomLeft, .bottomRight])
                                    .offset(x: whiteKeyWidth * 0.35)
                                    .gesture(
                                        DragGesture(minimumDistance: 0)
                                            .onChanged { _ in
                                                if !activeNotes.contains(blackMidi) {
                                                    activeNotes.insert(blackMidi)
                                                    onNoteOn(blackMidi, 0.9)
                                                }
                                            }
                                            .onEnded { _ in
                                                activeNotes.remove(blackMidi)
                                                onNoteOff(blackMidi)
                                            }
                                    )
                                    .zIndex(1)
                            }
                        }
                    }
                }
            }
        }
    }
}

// Corner radius helper
extension View {
    func cornerRadius(_ radius: CGFloat, corners: UIRectCorner) -> some View {
        clipShape(RoundedCorner(radius: radius, corners: corners))
    }
}

struct RoundedCorner: Shape {
    var radius: CGFloat = .infinity
    var corners: UIRectCorner = .allCorners

    func path(in rect: CGRect) -> Path {
        let path = UIBezierPath(
            roundedRect: rect,
            byRoundingCorners: corners,
            cornerRadii: CGSize(width: radius, height: radius)
        )
        return Path(path.cgPath)
    }
}
