import Foundation

/// Exports note events as a Standard MIDI File (Type 0).
/// Spec Section 5.3: MIDI export path for sharing performances outside the app.
enum MIDIExporter {

    // MARK: - Public API

    /// Export PerformanceRecorder events to MIDI data.
    static func exportPerformance(events: [PerformanceRecorder.NoteEvent], bpm: Double = 120) -> Data {
        let mapped = events.map { MIDIEvent(timestamp: $0.timestamp, note: $0.midiNote, velocity: $0.velocity) }
        return buildMIDI(events: mapped, bpm: bpm)
    }

    /// Export LoopRecorder layers to MIDI data.
    /// All layers are merged and sorted by timestamp into a single Type-0 track.
    static func exportLoop(layers: [[LoopRecorder.NoteEvent]], bpm: Double = 120) -> Data {
        var allEvents: [MIDIEvent] = []
        for layer in layers {
            for event in layer {
                allEvents.append(MIDIEvent(timestamp: event.timestamp, note: event.midiNote, velocity: event.velocity))
            }
        }
        allEvents.sort {
            if $0.timestamp != $1.timestamp { return $0.timestamp < $1.timestamp }
            return $0.velocity > $1.velocity // note-on (velocity > 0) before note-off (velocity == 0)
        }
        return buildMIDI(events: allEvents, bpm: bpm)
    }

    /// Write a PerformanceRecorder export to a temp file and return the URL.
    /// Returns nil if the event list is empty or the write fails.
    static func exportToFile(events: [PerformanceRecorder.NoteEvent], name: String, bpm: Double = 120) -> URL? {
        guard !events.isEmpty else { return nil }
        let data = exportPerformance(events: events, bpm: bpm)
        return writeToTempFile(data: data, name: name)
    }

    /// Write a LoopRecorder export to a temp file and return the URL.
    /// Returns nil if all layers are empty or the write fails.
    static func exportLoopToFile(layers: [[LoopRecorder.NoteEvent]], name: String, bpm: Double = 120) -> URL? {
        let hasEvents = layers.contains(where: { !$0.isEmpty })
        guard hasEvents else { return nil }
        let data = exportLoop(layers: layers, bpm: bpm)
        return writeToTempFile(data: data, name: name)
    }

    // MARK: - Internal types

    private struct MIDIEvent {
        let timestamp: TimeInterval
        let note: Int
        let velocity: Float   // 0 = noteOff, >0 = noteOn
    }

    // MARK: - MIDI file builder

    /// Build a Standard MIDI File Type 0 from a sorted event list.
    private static func buildMIDI(events: [MIDIEvent], bpm: Double) -> Data {
        var data = Data()

        let ppq: UInt16 = 480  // Pulses per quarter note (industry standard)

        // ---- Header chunk: MThd ----
        data.append(contentsOf: [0x4D, 0x54, 0x68, 0x64])  // "MThd"
        data.append(contentsOf: uint32BE(6))                  // Header length always 6
        data.append(contentsOf: uint16BE(0))                  // Format 0: single track
        data.append(contentsOf: uint16BE(1))                  // 1 track
        data.append(contentsOf: uint16BE(ppq))                // 480 PPQ

        // ---- Track chunk: MTrk ----
        var trackData = Data()

        // Tempo meta event (delta-time 0)
        let safeBPM = max(1, bpm)  // Guard against divide-by-zero
        let microsPerBeat = UInt32(60_000_000.0 / safeBPM)
        trackData.append(0x00)                                             // Delta time = 0
        trackData.append(contentsOf: [0xFF, 0x51, 0x03])                  // Tempo meta event
        trackData.append(UInt8((microsPerBeat >> 16) & 0xFF))
        trackData.append(UInt8((microsPerBeat >> 8) & 0xFF))
        trackData.append(UInt8(microsPerBeat & 0xFF))

        // Convert events to delta-time MIDI messages
        var lastTick: UInt32 = 0
        let ticksPerSecond = Double(ppq) * (safeBPM / 60.0)

        for event in events {
            let tick = UInt32(max(0, event.timestamp * ticksPerSecond))
            // Guard: events should already be sorted ascending; clamp negative deltas to 0.
            let delta = tick >= lastTick ? tick - lastTick : 0
            trackData.append(contentsOf: variableLengthQuantity(delta))

            let noteNum = UInt8(max(0, min(127, event.note)))
            if event.velocity > 0 {
                // Note On — channel 0
                let vel = UInt8(max(1, min(127, Int(event.velocity * 127))))
                trackData.append(contentsOf: [0x90, noteNum, vel])
            } else {
                // Note Off — channel 0
                trackData.append(contentsOf: [0x80, noteNum, 0x00])
            }

            lastTick = tick
        }

        // End of track meta event
        trackData.append(contentsOf: [0x00, 0xFF, 0x2F, 0x00])

        data.append(contentsOf: [0x4D, 0x54, 0x72, 0x6B])  // "MTrk"
        data.append(contentsOf: uint32BE(UInt32(trackData.count)))
        data.append(trackData)

        return data
    }

    // MARK: - Helpers

    private static func writeToTempFile(data: Data, name: String) -> URL? {
        let tempDir = FileManager.default.temporaryDirectory
            .appendingPathComponent("MIDIExports", isDirectory: true)
        // createDirectory is a no-op if the directory already exists
        try? FileManager.default.createDirectory(at: tempDir, withIntermediateDirectories: true)
        let url = tempDir.appendingPathComponent("\(name).mid")
        do {
            try data.write(to: url)
            return url
        } catch {
            print("[MIDIExporter] Failed to write \(url.lastPathComponent): \(error)")
            return nil
        }
    }

    private static func uint32BE(_ value: UInt32) -> [UInt8] {
        [UInt8((value >> 24) & 0xFF),
         UInt8((value >> 16) & 0xFF),
         UInt8((value >> 8)  & 0xFF),
         UInt8( value        & 0xFF)]
    }

    private static func uint16BE(_ value: UInt16) -> [UInt8] {
        [UInt8((value >> 8) & 0xFF),
         UInt8( value       & 0xFF)]
    }

    /// Encode a 28-bit unsigned integer as a MIDI variable-length quantity (big-endian 7-bit groups).
    private static func variableLengthQuantity(_ value: UInt32) -> [UInt8] {
        if value < 128 { return [UInt8(value)] }
        var result: [UInt8] = []
        var v = value
        // LSB group first (no continuation bit), then remaining groups with continuation bits
        result.append(UInt8(v & 0x7F))
        v >>= 7
        while v > 0 {
            result.append(UInt8((v & 0x7F) | 0x80))
            v >>= 7
        }
        return result.reversed()
    }
}
