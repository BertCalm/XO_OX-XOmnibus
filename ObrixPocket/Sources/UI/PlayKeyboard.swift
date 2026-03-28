import SwiftUI
import UIKit

// MARK: - KeyboardScale

enum KeyboardScale: String, CaseIterable {
    case chromatic  = "Chromatic"
    case major      = "Major"
    case minor      = "Minor"
    case pentatonic = "Pentatonic"

    /// Semitone offsets within one octave that belong to this scale
    var intervals: [Int] {
        switch self {
        case .chromatic:  return [0,1,2,3,4,5,6,7,8,9,10,11]
        case .major:      return [0,2,4,5,7,9,11]         // W W H W W W H
        case .minor:      return [0,2,3,5,7,8,10]          // W H W W H W W
        case .pentatonic: return [0,2,4,7,9]               // Major pentatonic
        }
    }

    /// Check if a MIDI note is in this scale (relative to C)
    func contains(midiNote: Int) -> Bool {
        let semitone = ((midiNote % 12) + 12) % 12  // Always positive
        return intervals.contains(semitone)
    }
}

// MARK: - KeyboardView (UIKit, multi-touch native)

/// A UIView that handles keyboard rendering and multi-touch directly.
/// Supports simultaneous note presses, velocity from 3D Touch, and glissando.
final class KeyboardView: UIView {

    // MARK: Callbacks
    var onNoteOn: ((Int, Float) -> Void)?
    var onNoteOff: ((Int) -> Void)?
    var accentColor: UIColor = UIColor(red: 0.118, green: 0.545, blue: 0.494, alpha: 1)
    var octaveOffset: Int = 0 {
        didSet { setNeedsDisplay() }
    }
    var scale: KeyboardScale = .chromatic {
        didSet { setNeedsDisplay() }
    }

    // MARK: Layout constants
    private let whiteNoteOffsets = [0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23]
    private let blackNoteOffsets: Set<Int> = [1, 3, 6, 8, 10, 13, 15, 18, 20, 22]

    /// Map from white-key index → black-key note offset (offset from startNote), or nil if no black key
    /// A black key is positioned to the right of the white key at the same index.
    private let blackKeyForWhiteIndex: [Int: Int] = [
        0: 1,   // C → C#
        1: 3,   // D → D#
        3: 6,   // F → F#
        4: 8,   // G → G#
        5: 10,  // A → A#
        7: 13,  // C → C#
        8: 15,  // D → D#
        10: 18, // F → F#
        11: 20, // G → G#
        12: 22  // A → A#
    ]

    private var startNote: Int { 48 + (octaveOffset * 12) }
    private let whiteKeyCount = 14

    // MARK: State
    /// Maps UITouch identity → MIDI note currently sounding for that finger
    private var activeTouches: [ObjectIdentifier: Int] = [:]

    // MARK: Colors
    private let whiteKeyColor   = UIColor(red: 0.102, green: 0.102, blue: 0.110, alpha: 1)   // #1A1A1C
    private let blackKeyColor   = UIColor(red: 0.039, green: 0.039, blue: 0.059, alpha: 1)   // #0A0A0F
    private let borderColor     = UIColor.white.withAlphaComponent(0.08)

    // MARK: Init

    override init(frame: CGRect) {
        super.init(frame: frame)
        isMultipleTouchEnabled = true  // CRITICAL — without this only one finger is tracked
        backgroundColor = UIColor(red: 0.055, green: 0.055, blue: 0.063, alpha: 1)
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)
        isMultipleTouchEnabled = true
    }

    // MARK: Drawing

    override func draw(_ rect: CGRect) {
        guard let ctx = UIGraphicsGetCurrentContext() else { return }

        let activeNotes = Set(activeTouches.values)
        let whiteKeyWidth  = bounds.width / CGFloat(whiteKeyCount)
        let blackKeyWidth  = whiteKeyWidth * 0.6
        let blackKeyHeight = bounds.height * 0.6

        // Draw white keys first
        let dimWhiteKeyColor = UIColor(red: 0.055, green: 0.055, blue: 0.063, alpha: 1) // Nearly background
        for i in 0..<whiteKeyCount {
            let note    = startNote + whiteNoteOffsets[i]
            let x       = CGFloat(i) * whiteKeyWidth
            let keyRect = CGRect(x: x, y: 0, width: whiteKeyWidth, height: bounds.height)
            let inScale = scale.contains(midiNote: note)

            let fill: UIColor
            if activeNotes.contains(note) {
                fill = accentColor.withAlphaComponent(0.40)
            } else if inScale || scale == .chromatic {
                fill = whiteKeyColor
            } else {
                fill = dimWhiteKeyColor
            }
            ctx.setFillColor(fill.cgColor)
            ctx.fill(keyRect)

            // Right-edge border
            ctx.setStrokeColor(borderColor.cgColor)
            ctx.setLineWidth(0.5)
            ctx.stroke(CGRect(x: x, y: 0, width: whiteKeyWidth, height: bounds.height))
        }

        // Draw black keys on top
        let dimBlackKeyColor = UIColor(red: 0.024, green: 0.024, blue: 0.032, alpha: 1) // Nearly invisible
        for (whiteIndex, blackOffset) in blackKeyForWhiteIndex {
            let note    = startNote + blackOffset
            let inScale = scale.contains(midiNote: note)
            // Black key is centered over the right edge of its white key
            let whiteX = CGFloat(whiteIndex) * whiteKeyWidth
            let blackX = whiteX + whiteKeyWidth - blackKeyWidth * 0.5
            let keyRect = CGRect(x: blackX, y: 0, width: blackKeyWidth, height: blackKeyHeight)

            let fill: UIColor
            if activeNotes.contains(note) {
                fill = accentColor
            } else if inScale || scale == .chromatic {
                fill = blackKeyColor
            } else {
                fill = dimBlackKeyColor
            }
            ctx.setFillColor(fill.cgColor)

            // Draw with rounded bottom corners
            let path = UIBezierPath(
                roundedRect: keyRect,
                byRoundingCorners: [.bottomLeft, .bottomRight],
                cornerRadii: CGSize(width: 3, height: 3)
            )
            ctx.addPath(path.cgPath)
            ctx.fillPath()

            // Border
            ctx.setStrokeColor(borderColor.cgColor)
            ctx.setLineWidth(0.5)
            ctx.addPath(path.cgPath)
            ctx.strokePath()
        }
    }

    // MARK: Hit-testing

    /// Return the MIDI note at `point`, checking black keys first (they are on top).
    /// Returns nil for out-of-scale notes when a non-chromatic scale is active.
    func keyAt(point: CGPoint) -> Int? {
        guard bounds.contains(point) else { return nil }

        let whiteKeyWidth  = bounds.width / CGFloat(whiteKeyCount)
        let blackKeyWidth  = whiteKeyWidth * 0.6
        let blackKeyHeight = bounds.height * 0.6

        // Check black keys first
        if point.y < blackKeyHeight {
            for (whiteIndex, blackOffset) in blackKeyForWhiteIndex {
                let whiteX = CGFloat(whiteIndex) * whiteKeyWidth
                let blackX = whiteX + whiteKeyWidth - blackKeyWidth * 0.5
                let keyRect = CGRect(x: blackX, y: 0, width: blackKeyWidth, height: blackKeyHeight)
                if keyRect.contains(point) {
                    let note = startNote + blackOffset
                    // Scale filter — skip out-of-scale notes
                    if scale != .chromatic && !scale.contains(midiNote: note) { return nil }
                    return note
                }
            }
        }

        // Fall through to white keys
        let whiteIndex = Int(point.x / whiteKeyWidth)
        guard whiteIndex >= 0 && whiteIndex < whiteKeyCount else { return nil }
        let note = startNote + whiteNoteOffsets[whiteIndex]
        // Scale filter — skip out-of-scale notes
        if scale != .chromatic && !scale.contains(midiNote: note) { return nil }
        return note
    }

    // MARK: Velocity

    func velocityForTouch(_ touch: UITouch) -> Float {
        if touch.maximumPossibleForce > 0 {
            let raw = Float(touch.force / touch.maximumPossibleForce)
            return max(0.3, min(1.0, raw))
        }
        return 0.8  // Fallback for devices without 3D Touch
    }

    // MARK: Touch Handling

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            guard let note = keyAt(point: touch.location(in: self)) else { continue }
            let id       = ObjectIdentifier(touch)
            let velocity = velocityForTouch(touch)
            activeTouches[id] = note
            onNoteOn?(note, velocity)
            HapticEngine.keyPress(velocity: velocity)
        }
        setNeedsDisplay()
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        var changed = false
        for touch in touches {
            let id = ObjectIdentifier(touch)
            guard let newNote = keyAt(point: touch.location(in: self)) else {
                // Finger moved off the keyboard — release whatever was held
                if let oldNote = activeTouches.removeValue(forKey: id) {
                    onNoteOff?(oldNote)
                    changed = true
                }
                continue
            }
            let oldNote = activeTouches[id]
            if oldNote != newNote {
                // Glissando: finger slid to a different key
                if let prev = oldNote {
                    onNoteOff?(prev)
                }
                let velocity = velocityForTouch(touch)
                onNoteOn?(newNote, velocity)
                activeTouches[id] = newNote
                changed = true
            }
        }
        if changed { setNeedsDisplay() }
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let id = ObjectIdentifier(touch)
            if let note = activeTouches.removeValue(forKey: id) {
                onNoteOff?(note)
            }
        }
        setNeedsDisplay()
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        // Treat cancellation identically to ending — clean up all outstanding notes
        touchesEnded(touches, with: event)
    }

    // MARK: Cleanup

    /// Release all currently held notes (call when the view disappears).
    func allNotesOff() {
        for note in activeTouches.values {
            onNoteOff?(note)
        }
        activeTouches.removeAll()
        setNeedsDisplay()
    }
}

// MARK: - PlayKeyboard (SwiftUI wrapper)

/// SwiftUI wrapper around KeyboardView.
/// API is identical to the old DragGesture version — drop-in replacement.
struct PlayKeyboard: UIViewRepresentable {
    let onNoteOn:     (Int, Float) -> Void  // (midiNote, velocity)
    let onNoteOff:    (Int) -> Void
    let accentColor:  Color
    @Binding var octaveOffset: Int
    var scale: KeyboardScale = .chromatic

    func makeUIView(context: Context) -> KeyboardView {
        let view           = KeyboardView()
        view.onNoteOn      = onNoteOn
        view.onNoteOff     = onNoteOff
        view.accentColor   = UIColor(accentColor)
        view.octaveOffset  = octaveOffset
        view.scale         = scale
        return view
    }

    func updateUIView(_ uiView: KeyboardView, context: Context) {
        // Push any SwiftUI-side changes down to the UIKit view
        uiView.onNoteOn     = onNoteOn
        uiView.onNoteOff    = onNoteOff
        uiView.accentColor  = UIColor(accentColor)
        uiView.scale        = scale

        if uiView.octaveOffset != octaveOffset {
            // Release held notes before jumping octave — avoids stuck notes
            uiView.allNotesOff()
            uiView.octaveOffset = octaveOffset
        }
    }
}

// MARK: - RoundedCorner shape helper (retained for other views)

extension View {
    func cornerRadius(_ radius: CGFloat, corners: UIRectCorner) -> some View {
        clipShape(RoundedCorner(radius: radius, corners: corners))
    }
}

struct RoundedCorner: Shape {
    var radius: CGFloat    = .infinity
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
