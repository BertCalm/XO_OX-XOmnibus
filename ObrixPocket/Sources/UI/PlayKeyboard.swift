import SwiftUI
import UIKit

// MARK: - KeyboardMode

enum KeyboardMode: String, CaseIterable {
    case single = "Single"
    case chord  = "Chord"
    case arp    = "Arp"
}

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
    var mode: KeyboardMode = .single {
        didSet { stopArp() }
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

    // MARK: Expression Tracking

    private struct TouchExpression {
        var midiNote: Int
        var yExpression: Float  // 0 = bottom of key, 1 = top — maps to filter mod
        var xExpression: Float  // 0 = left edge, 1 = right edge — maps to pitch bend
    }

    /// Fires on every finger-move within the same key: (filterMod 0–1, pitchBend –1 to +1)
    var onExpression: ((Float, Float) -> Void)?

    private var activeTouchExpressions: [ObjectIdentifier: TouchExpression] = [:]

    // MARK: State
    /// Maps UITouch identity → MIDI note currently sounding for that finger
    private var activeTouches: [ObjectIdentifier: Int] = [:]

    // MARK: Chord Mode State
    /// Maps UITouch identity → all MIDI notes triggered by that finger in chord mode
    private var chordNotes: [ObjectIdentifier: [Int]] = [:]

    // MARK: Arp Mode State
    private var arpTimer: Timer?
    private var arpNotes: [Int] = []
    private var arpIndex = 0
    private var arpBPM: Double = 120
    var syncBPM: Double? = nil  // External BPM sync (from metronome)
    private var currentArpNote: Int?

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

        // Draw expression indicators for active white keys (horizontal line at touch Y)
        for i in 0..<whiteKeyCount {
            let note    = startNote + whiteNoteOffsets[i]
            guard activeNotes.contains(note) else { continue }
            guard let expr = activeTouchExpressions.values.first(where: { $0.midiNote == note }) else { continue }
            let x       = CGFloat(i) * whiteKeyWidth
            let keyRect = CGRect(x: x, y: 0, width: whiteKeyWidth, height: bounds.height)
            let indicatorY = CGFloat(1.0 - expr.yExpression) * bounds.height
            ctx.setStrokeColor(UIColor.white.withAlphaComponent(0.35).cgColor)
            ctx.setLineWidth(1.0)
            ctx.move(to: CGPoint(x: keyRect.minX + 2, y: indicatorY))
            ctx.addLine(to: CGPoint(x: keyRect.maxX - 2, y: indicatorY))
            ctx.strokePath()
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

            // Expression indicator for active black key
            if activeNotes.contains(note),
               let expr = activeTouchExpressions.values.first(where: { $0.midiNote == note }) {
                let indicatorY = CGFloat(1.0 - expr.yExpression) * blackKeyHeight
                ctx.setStrokeColor(UIColor.white.withAlphaComponent(0.4).cgColor)
                ctx.setLineWidth(1.0)
                ctx.move(to: CGPoint(x: keyRect.minX + 2, y: indicatorY))
                ctx.addLine(to: CGPoint(x: keyRect.maxX - 2, y: indicatorY))
                ctx.strokePath()
            }
        }
    }

    // MARK: Key Rect Lookup

    /// Return the bounding rect for a given MIDI note, or nil if the note is not visible on this keyboard.
    func rectForNote(_ note: Int) -> CGRect? {
        let offset = note - startNote
        let whiteKeyWidth  = bounds.width / CGFloat(whiteKeyCount)
        let blackKeyWidth  = whiteKeyWidth * 0.6
        let blackKeyHeight = bounds.height * 0.6

        // Check black keys
        for (whiteIndex, blackOffset) in blackKeyForWhiteIndex {
            if blackOffset == offset {
                let whiteX = CGFloat(whiteIndex) * whiteKeyWidth
                let blackX = whiteX + whiteKeyWidth - blackKeyWidth * 0.5
                return CGRect(x: blackX, y: 0, width: blackKeyWidth, height: blackKeyHeight)
            }
        }
        // Check white keys
        for (i, whiteOffset) in whiteNoteOffsets.enumerated() {
            if whiteOffset == offset {
                return CGRect(x: CGFloat(i) * whiteKeyWidth, y: 0,
                              width: whiteKeyWidth, height: bounds.height)
            }
        }
        return nil
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

            if mode == .chord {
                // Play major triad: root + major 3rd (+4 semitones) + perfect 5th (+7 semitones)
                let root  = note
                let third = root + 4
                let fifth = root + 7

                onNoteOn?(root, velocity)
                if third <= 127 { onNoteOn?(third, velocity * 0.9) }
                if fifth <= 127 { onNoteOn?(fifth, velocity * 0.85) }

                activeTouches[id] = root
                chordNotes[id] = [root, third, fifth].filter { $0 <= 127 }
            } else if mode == .arp {
                // Build arp pattern from the active scale's intervals so the arp
                // stays in key regardless of which scale is selected.
                var pattern: [Int] = [note]  // Root always first
                let scaleIntervals = scale.intervals
                let rootChroma = ((note % 12) + 12) % 12
                var added = 0
                for i in 1...12 {
                    let candidate = rootChroma + i
                    let normalizedCandidate = candidate % 12
                    if scaleIntervals.contains(normalizedCandidate) {
                        pattern.append(note + i)
                        added += 1
                        if added >= 3 { break }  // Root + 3 more = 4-note pattern
                    }
                }
                arpNotes = pattern.filter { $0 <= 127 }
                arpIndex = 0
                startArp()
                activeTouches[id] = note
            } else {
                activeTouches[id] = note
                onNoteOn?(note, velocity)

                // Capture initial expression position for single-mode touches
                let loc = touch.location(in: self)
                let yNorm = max(0, min(1, Float(1.0 - (loc.y / bounds.height))))
                let xNorm: Float
                if let keyRect = rectForNote(note), keyRect.width > 0 {
                    xNorm = max(0, min(1, Float((loc.x - keyRect.minX) / keyRect.width)))
                } else {
                    xNorm = 0.5
                }
                activeTouchExpressions[id] = TouchExpression(
                    midiNote: note,
                    yExpression: yNorm,
                    xExpression: xNorm
                )
                let filterMod = yNorm
                let pitchBend = (xNorm - 0.5) * 2.0
                onExpression?(filterMod, pitchBend)
            }

            HapticEngine.keyPress(velocity: velocity)
        }
        setNeedsDisplay()
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        var changed = false
        for touch in touches {
            let id = ObjectIdentifier(touch)

            // Chord and arp modes do not support glissando or expression — skip movement handling
            if mode == .chord || mode == .arp { continue }

            let loc = touch.location(in: self)

            guard let newNote = keyAt(point: loc) else {
                // Finger moved off the keyboard — release whatever was held
                if let oldNote = activeTouches.removeValue(forKey: id) {
                    onNoteOff?(oldNote)
                    activeTouchExpressions.removeValue(forKey: id)
                    changed = true
                }
                continue
            }
            let oldNote = activeTouches[id]
            if oldNote == newNote {
                // Same key — update expression in real time
                let yNorm = max(0, min(1, Float(1.0 - (loc.y / bounds.height))))
                let xNorm: Float
                if let keyRect = rectForNote(newNote), keyRect.width > 0 {
                    xNorm = max(0, min(1, Float((loc.x - keyRect.minX) / keyRect.width)))
                } else {
                    xNorm = 0.5
                }
                let oldExpr = activeTouchExpressions[id]
                let yChanged = abs(yNorm - (oldExpr?.yExpression ?? 0)) > 0.005
                let xChanged = abs(xNorm - (oldExpr?.xExpression ?? 0)) > 0.005

                if yChanged || xChanged {
                    activeTouchExpressions[id]?.yExpression = yNorm
                    activeTouchExpressions[id]?.xExpression = xNorm
                    onExpression?(yNorm, (xNorm - 0.5) * 2.0)
                    setNeedsDisplay()  // Redraw expression indicator
                }
            } else if oldNote != newNote {
                // Glissando: finger slid to a different key
                if let prev = oldNote {
                    onNoteOff?(prev)
                }
                let velocity = velocityForTouch(touch)
                onNoteOn?(newNote, velocity)
                activeTouches[id] = newNote

                // Reset expression for new key
                let yNorm = max(0, min(1, Float(1.0 - (loc.y / bounds.height))))
                let xNorm: Float
                if let keyRect = rectForNote(newNote), keyRect.width > 0 {
                    xNorm = max(0, min(1, Float((loc.x - keyRect.minX) / keyRect.width)))
                } else {
                    xNorm = 0.5
                }
                activeTouchExpressions[id] = TouchExpression(
                    midiNote: newNote,
                    yExpression: yNorm,
                    xExpression: xNorm
                )
                let filterMod = yNorm
                let pitchBend = (xNorm - 0.5) * 2.0
                onExpression?(filterMod, pitchBend)
                changed = true
            }
        }
        if changed { setNeedsDisplay() }
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let id = ObjectIdentifier(touch)

            if mode == .chord, let notes = chordNotes.removeValue(forKey: id) {
                activeTouches.removeValue(forKey: id)
                activeTouchExpressions.removeValue(forKey: id)
                for n in notes { onNoteOff?(n) }
            } else if mode == .arp {
                activeTouches.removeValue(forKey: id)
                activeTouchExpressions.removeValue(forKey: id)
                stopArp()
            } else {
                if let note = activeTouches.removeValue(forKey: id) {
                    onNoteOff?(note)
                }
                activeTouchExpressions.removeValue(forKey: id)
            }
        }
        setNeedsDisplay()
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        // Treat cancellation identically to ending — clean up all outstanding notes
        touchesEnded(touches, with: event)
    }

    // MARK: Arpeggiator

    private func startArp() {
        stopArp()
        let interval = 60.0 / (syncBPM ?? arpBPM)  // Quarter note interval; prefer external sync
        let timer = Timer(timeInterval: interval, repeats: true) { [weak self] _ in
            guard let self, !self.arpNotes.isEmpty else { return }

            // Note off previous
            if let prev = self.currentArpNote {
                self.onNoteOff?(prev)
            }

            // Note on next
            let note = self.arpNotes[self.arpIndex % self.arpNotes.count]
            self.onNoteOn?(note, 0.7)
            self.currentArpNote = note
            self.arpIndex += 1
        }
        RunLoop.main.add(timer, forMode: .common)
        arpTimer = timer
    }

    private func stopArp() {
        arpTimer?.invalidate()
        arpTimer = nil
        if let prev = currentArpNote {
            onNoteOff?(prev)
            currentArpNote = nil
        }
        arpNotes = []
        arpIndex = 0
    }

    // MARK: Cleanup

    /// Release all currently held notes (call when the view disappears).
    func allNotesOff() {
        stopArp()
        // Release any chord notes
        for notes in chordNotes.values {
            for n in notes { onNoteOff?(n) }
        }
        chordNotes.removeAll()
        for note in activeTouches.values {
            onNoteOff?(note)
        }
        activeTouches.removeAll()
        activeTouchExpressions.removeAll()
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
    var mode: KeyboardMode = .single
    var syncBPM: Double? = nil  // External BPM sync (from metronome)
    var onExpression: ((Float, Float) -> Void)? = nil  // (filterMod 0–1, pitchBend –1 to +1)

    func makeUIView(context: Context) -> KeyboardView {
        let view           = KeyboardView()
        view.onNoteOn      = onNoteOn
        view.onNoteOff     = onNoteOff
        view.accentColor   = UIColor(accentColor)
        view.octaveOffset  = octaveOffset
        view.scale         = scale
        view.mode          = mode
        view.onExpression  = onExpression
        return view
    }

    func updateUIView(_ uiView: KeyboardView, context: Context) {
        // Push any SwiftUI-side changes down to the UIKit view
        uiView.onNoteOn      = onNoteOn
        uiView.onNoteOff     = onNoteOff
        uiView.accentColor   = UIColor(accentColor)
        uiView.scale         = scale
        uiView.mode          = mode
        uiView.syncBPM       = syncBPM
        uiView.onExpression  = onExpression

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
