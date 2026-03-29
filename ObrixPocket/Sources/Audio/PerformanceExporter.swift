import Foundation
import QuartzCore

/// A single recorded performance event
struct PerformanceEvent: Codable {
    let timestamp: TimeInterval     // Seconds from recording start
    let type: EventType
    let slotIndex: Int?             // Which specimen slot (nil for global events)
    let midiNote: Int?              // MIDI note number (nil for non-note events)
    let velocity: Float?            // 0-1 (nil for note-off)
    let parameterID: String?        // For parameter change events
    let parameterValue: Float?      // For parameter change events

    enum EventType: String, Codable {
        case noteOn
        case noteOff
        case macroChange           // Macro knob moved
        case tiltExpression        // Tilt gesture
        case tapAccent             // Player accent tap
        case sectionChange         // Arrangement section transition
        case chordChange           // Harmonic progression change
    }
}

/// A complete recorded performance
struct PerformanceRecording: Codable, Identifiable {
    let id: UUID
    let date: Date
    let durationSeconds: Double
    let events: [PerformanceEvent]
    let metadata: PerformanceMetadata

    var eventCount: Int { events.count }
    var noteCount: Int { events.filter { $0.type == .noteOn }.count }
}

/// Metadata about a performance recording
struct PerformanceMetadata: Codable {
    let bpm: Double
    let key: Int                    // 0-11 (C-B)
    let scale: String               // Scale name
    let specimenNames: [String]     // Participating specimens
    let reefSnapshotName: String?   // Which reef configuration was active
    let diveMode: Bool              // Was this during a Dive or freeplay?
}

/// Records performance events and exports to multiple formats.
///
/// Captures every note, macro change, and expression event during
/// a performance session. The recording can be exported as:
/// - MIDI file (.mid) — notes and timing only
/// - Audio clip (.m4a) — rendered audio (requires separate audio recorder)
/// - Replay format (.xodive) — full event stream for replay
/// - Social clip (.m4a, 15 sec) — short shareable excerpt
final class PerformanceExporter {

    // MARK: - State

    private var events: [PerformanceEvent] = []
    private var startTime: TimeInterval?
    private var isRecording = false
    private var metadata: PerformanceMetadata?

    // MARK: - Recording Control

    /// Start recording
    func startRecording(bpm: Double, key: Int, scale: String,
                        specimenNames: [String], reefSnapshot: String? = nil,
                        diveMode: Bool = false) {
        events.removeAll()
        startTime = CACurrentMediaTime()
        isRecording = true
        metadata = PerformanceMetadata(
            bpm: bpm,
            key: key,
            scale: scale,
            specimenNames: specimenNames,
            reefSnapshotName: reefSnapshot,
            diveMode: diveMode
        )
    }

    /// Stop recording and return the complete recording
    func stopRecording() -> PerformanceRecording? {
        guard isRecording, let start = startTime, let meta = metadata else { return nil }
        isRecording = false

        let duration = CACurrentMediaTime() - start
        let recording = PerformanceRecording(
            id: UUID(),
            date: Date(),
            durationSeconds: duration,
            events: events,
            metadata: meta
        )
        return recording
    }

    /// Whether recording is active
    var recording: Bool { isRecording }

    // MARK: - Event Registration

    /// Record a note-on event
    func noteOn(slot: Int, note: Int, velocity: Float) {
        guard isRecording, let start = startTime else { return }
        events.append(PerformanceEvent(
            timestamp: CACurrentMediaTime() - start,
            type: .noteOn,
            slotIndex: slot,
            midiNote: note,
            velocity: velocity,
            parameterID: nil,
            parameterValue: nil
        ))
    }

    /// Record a note-off event
    func noteOff(slot: Int, note: Int) {
        guard isRecording, let start = startTime else { return }
        events.append(PerformanceEvent(
            timestamp: CACurrentMediaTime() - start,
            type: .noteOff,
            slotIndex: slot,
            midiNote: note,
            velocity: nil,
            parameterID: nil,
            parameterValue: nil
        ))
    }

    /// Record a macro change event
    func macroChange(paramID: String, value: Float) {
        guard isRecording, let start = startTime else { return }
        events.append(PerformanceEvent(
            timestamp: CACurrentMediaTime() - start,
            type: .macroChange,
            slotIndex: nil,
            midiNote: nil,
            velocity: nil,
            parameterID: paramID,
            parameterValue: value
        ))
    }

    /// Record an expression event (tilt, tap accent)
    func expressionEvent(type: PerformanceEvent.EventType) {
        guard isRecording, let start = startTime else { return }
        events.append(PerformanceEvent(
            timestamp: CACurrentMediaTime() - start,
            type: type,
            slotIndex: nil,
            midiNote: nil,
            velocity: nil,
            parameterID: nil,
            parameterValue: nil
        ))
    }

    // MARK: - Export: MIDI

    /// Export as a Standard MIDI File (Type 0, single track)
    /// Returns raw MIDI file data ready to write to disk
    func exportMIDI(recording: PerformanceRecording) -> Data {
        var midi = Data()
        let ticksPerBeat: UInt16 = 480
        let microsecondsPerBeat = UInt32(60_000_000 / recording.metadata.bpm)

        // MIDI Header: MThd
        midi.append(contentsOf: [0x4D, 0x54, 0x68, 0x64])  // "MThd"
        midi.append(contentsOf: uint32Bytes(6))               // Header length
        midi.append(contentsOf: uint16Bytes(0))                // Format 0 (single track)
        midi.append(contentsOf: uint16Bytes(1))                // 1 track
        midi.append(contentsOf: uint16Bytes(ticksPerBeat))     // Ticks per beat

        // Track data
        var track = Data()

        // Tempo meta event
        track.append(contentsOf: [0x00])                       // Delta time 0
        track.append(contentsOf: [0xFF, 0x51, 0x03])           // Tempo meta event
        track.append(contentsOf: uint24Bytes(microsecondsPerBeat))

        // Convert events to MIDI track events
        var lastTick: UInt32 = 0
        let noteEvents = recording.events.filter { $0.type == .noteOn || $0.type == .noteOff }

        for event in noteEvents {
            let tick = UInt32(event.timestamp * Double(ticksPerBeat) * recording.metadata.bpm / 60.0)
            let delta = tick > lastTick ? tick - lastTick : 0
            lastTick = tick

            // Variable-length delta time
            track.append(contentsOf: variableLengthBytes(delta))

            if event.type == .noteOn, let note = event.midiNote, let vel = event.velocity {
                let channel: UInt8 = UInt8(min(15, event.slotIndex ?? 0))
                track.append(0x90 | channel)                    // Note On
                track.append(UInt8(max(0, min(127, note))))
                track.append(UInt8(max(1, min(127, Int(vel * 127)))))
            } else if event.type == .noteOff, let note = event.midiNote {
                let channel: UInt8 = UInt8(min(15, event.slotIndex ?? 0))
                track.append(0x80 | channel)                    // Note Off
                track.append(UInt8(max(0, min(127, note))))
                track.append(0x40)                               // Release velocity
            }
        }

        // End of track
        track.append(contentsOf: [0x00, 0xFF, 0x2F, 0x00])

        // Track header: MTrk
        midi.append(contentsOf: [0x4D, 0x54, 0x72, 0x6B])  // "MTrk"
        midi.append(contentsOf: uint32Bytes(UInt32(track.count)))
        midi.append(track)

        return midi
    }

    // MARK: - Export: Replay (.xodive)

    /// Export as an .xodive replay file (JSON-based, includes all events + metadata)
    func exportReplay(recording: PerformanceRecording) -> Data? {
        try? JSONEncoder().encode(recording)
    }

    // MARK: - Export: Social Clip

    /// Extract a 15-second excerpt from the recording (the highest-energy section)
    func socialClipRange(recording: PerformanceRecording) -> ClosedRange<TimeInterval> {
        let clipDuration: TimeInterval = 15.0
        guard recording.durationSeconds > clipDuration else {
            return 0...recording.durationSeconds
        }

        // Find the 15-second window with the most note events
        let windowStep: TimeInterval = 1.0
        var bestStart: TimeInterval = 0
        var bestCount = 0

        var windowStart: TimeInterval = 0
        while windowStart + clipDuration <= recording.durationSeconds {
            let windowEnd = windowStart + clipDuration
            let count = recording.events.filter {
                $0.type == .noteOn && $0.timestamp >= windowStart && $0.timestamp < windowEnd
            }.count
            if count > bestCount {
                bestCount = count
                bestStart = windowStart
            }
            windowStart += windowStep
        }

        return bestStart...(bestStart + clipDuration)
    }

    // MARK: - File Operations

    /// Save a MIDI file to the documents directory
    func saveMIDI(recording: PerformanceRecording, filename: String) -> URL? {
        let data = exportMIDI(recording: recording)
        let url = documentsURL(filename: "\(filename).mid")
        do {
            try data.write(to: url)
            return url
        } catch {
            return nil
        }
    }

    /// Save a replay file to the documents directory
    func saveReplay(recording: PerformanceRecording, filename: String) -> URL? {
        guard let data = exportReplay(recording: recording) else { return nil }
        let url = documentsURL(filename: "\(filename).xodive")
        do {
            try data.write(to: url)
            return url
        } catch {
            return nil
        }
    }

    // MARK: - MIDI Encoding Helpers

    private func uint16Bytes(_ value: UInt16) -> [UInt8] {
        [UInt8(value >> 8), UInt8(value & 0xFF)]
    }

    private func uint24Bytes(_ value: UInt32) -> [UInt8] {
        [UInt8((value >> 16) & 0xFF), UInt8((value >> 8) & 0xFF), UInt8(value & 0xFF)]
    }

    private func uint32Bytes(_ value: UInt32) -> [UInt8] {
        [UInt8((value >> 24) & 0xFF), UInt8((value >> 16) & 0xFF),
         UInt8((value >> 8) & 0xFF), UInt8(value & 0xFF)]
    }

    private func variableLengthBytes(_ value: UInt32) -> [UInt8] {
        if value < 0x80 { return [UInt8(value)] }
        var result: [UInt8] = []
        var v = value
        result.append(UInt8(v & 0x7F))
        v >>= 7
        while v > 0 {
            result.append(UInt8((v & 0x7F) | 0x80))
            v >>= 7
        }
        return result.reversed()
    }

    private func documentsURL(filename: String) -> URL {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
            .appendingPathComponent(filename)
    }
}
