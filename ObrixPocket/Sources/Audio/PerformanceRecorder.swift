import Foundation

/// Records note events (noteOn/noteOff with timing) for playback and sharing
final class PerformanceRecorder: ObservableObject {
    @Published var isRecording = false
    @Published var isPlaying = false
    @Published var duration: TimeInterval = 0
    @Published var hasRecording = false

    private var events: [NoteEvent] = []
    private var recordStartTime: Date?
    private var playbackTimer: Timer?
    private var playbackIndex = 0
    private let maxDuration: TimeInterval = 30

    struct NoteEvent: Codable {
        let timestamp: TimeInterval  // Seconds from recording start
        let midiNote: Int
        let velocity: Float          // 0 = noteOff, >0 = noteOn
    }

    // MARK: - Recording

    func startRecording() {
        events.removeAll()
        recordStartTime = Date()
        isRecording = true
        hasRecording = false
        duration = 0

        // Auto-stop after maxDuration
        DispatchQueue.main.asyncAfter(deadline: .now() + maxDuration) { [weak self] in
            guard let self, self.isRecording else { return }
            self.stopRecording()
        }
    }

    func stopRecording() {
        guard isRecording else { return }
        isRecording = false
        duration = events.last?.timestamp ?? 0
        hasRecording = !events.isEmpty
    }

    func recordNoteOn(midiNote: Int, velocity: Float) {
        guard isRecording, let start = recordStartTime else { return }
        let timestamp = Date().timeIntervalSince(start)
        guard timestamp <= maxDuration else { stopRecording(); return }
        events.append(NoteEvent(timestamp: timestamp, midiNote: midiNote, velocity: velocity))
        duration = timestamp
    }

    func recordNoteOff(midiNote: Int) {
        guard isRecording, let start = recordStartTime else { return }
        let timestamp = Date().timeIntervalSince(start)
        events.append(NoteEvent(timestamp: timestamp, midiNote: midiNote, velocity: 0))
    }

    // MARK: - Playback

    func play(noteOn: @escaping (Int, Float) -> Void, noteOff: @escaping (Int) -> Void) {
        guard hasRecording, !events.isEmpty, !isPlaying else { return }
        isPlaying = true
        playbackIndex = 0

        scheduleNextEvent(noteOn: noteOn, noteOff: noteOff)
    }

    func stopPlayback() {
        playbackTimer?.invalidate()
        playbackTimer = nil
        isPlaying = false
    }

    private func scheduleNextEvent(noteOn: @escaping (Int, Float) -> Void, noteOff: @escaping (Int) -> Void) {
        guard playbackIndex < events.count else {
            isPlaying = false
            return
        }

        let event = events[playbackIndex]
        let previousTime = playbackIndex > 0 ? events[playbackIndex - 1].timestamp : 0
        let delay = event.timestamp - previousTime

        playbackTimer = Timer.scheduledTimer(withTimeInterval: max(0, delay), repeats: false) { [weak self] _ in
            guard let self, self.isPlaying else { return }

            if event.velocity > 0 {
                noteOn(event.midiNote, event.velocity)
            } else {
                noteOff(event.midiNote)
            }

            self.playbackIndex += 1
            self.scheduleNextEvent(noteOn: noteOn, noteOff: noteOff)
        }
    }

    // MARK: - Export

    /// Export events as JSON data for sharing
    func exportJSON() -> Data? {
        try? JSONEncoder().encode(events)
    }

    /// Export the recorded events for MIDI export
    func exportEvents() -> [NoteEvent] {
        return events
    }

    /// Duration formatted as "0:15"
    var durationString: String {
        let seconds = Int(duration)
        return "\(seconds / 60):\(String(format: "%02d", seconds % 60))"
    }
}
