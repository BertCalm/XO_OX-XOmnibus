import Foundation

/// Records and plays back fixed-length loops with overdub layering.
/// Each call to startRecording() captures one new layer on top of existing layers.
final class LoopRecorder: ObservableObject {
    @Published var isRecording = false
    @Published var isPlaying = false
    @Published var currentBeat: Int = 0     // 0-based beat position within the loop (0 to beatsPerLoop-1)
    @Published var layerCount: Int = 0
    @Published var loopProgress: Float = 0  // 0-1 within the loop

    private var layers: [[NoteEvent]] = []
    private var currentLayer: [NoteEvent] = []
    private var loopDuration: TimeInterval = 8.0  // default: 4 bars at 120 BPM
    private var loopStartTime: Date?
    private var playbackTimer: Timer?
    private var playbackStartTime: Date?
    private var autoStopWorkItem: DispatchWorkItem?
    private var lastPlaybackPos: TimeInterval = -1

    struct NoteEvent: Codable {
        let timestamp: TimeInterval  // Seconds from loop start (0 to loopDuration)
        let midiNote: Int
        let velocity: Float          // 0 = noteOff, >0 = noteOn
    }

    /// Beats per loop (always 4 bars × 4 beats = 16)
    var beatsPerLoop: Int { 16 }

    /// Sync to metronome BPM; recomputes loopDuration but does NOT restart in-progress recording/playback.
    var bpm: Double = 120 {
        didSet { loopDuration = (60.0 / bpm) * 4 * 4 }  // 4 beats × 4 bars
    }

    // MARK: - Recording

    func startRecording() {
        // Cancel any previous auto-stop that hasn't fired yet
        autoStopWorkItem?.cancel()
        autoStopWorkItem = nil

        currentLayer = []
        loopStartTime = Date()
        isRecording = true

        // Start playback of existing layers simultaneously so the performer hears them while overdubbing
        if !isPlaying { startPlayback() }

        // Auto-stop recording at loop end; use a captured duration so a mid-record BPM change doesn't stretch it
        let capturedDuration = loopDuration
        let workItem = DispatchWorkItem { [weak self] in
            guard let self, self.isRecording else { return }
            self.stopRecording()
        }
        autoStopWorkItem = workItem
        DispatchQueue.main.asyncAfter(deadline: .now() + capturedDuration, execute: workItem)
    }

    func stopRecording() {
        guard isRecording else { return }
        autoStopWorkItem?.cancel()
        autoStopWorkItem = nil
        isRecording = false

        if !currentLayer.isEmpty {
            layers.append(currentLayer)
            layerCount = layers.count
        }
        currentLayer = []
    }

    func recordNoteOn(midiNote: Int, velocity: Float) {
        guard isRecording, let start = loopStartTime else { return }
        let timestamp = Date().timeIntervalSince(start).truncatingRemainder(dividingBy: loopDuration)
        currentLayer.append(NoteEvent(timestamp: timestamp, midiNote: midiNote, velocity: velocity))
    }

    func recordNoteOff(midiNote: Int) {
        guard isRecording, let start = loopStartTime else { return }
        let timestamp = Date().timeIntervalSince(start).truncatingRemainder(dividingBy: loopDuration)
        currentLayer.append(NoteEvent(timestamp: timestamp, midiNote: midiNote, velocity: 0))
    }

    // MARK: - Playback

    func startPlayback() {
        // Need at least one committed layer, or recording must be active (new layer in progress)
        guard !layers.isEmpty || isRecording else { return }
        isPlaying = true
        playbackStartTime = Date()
        lastPlaybackPos = -1

        playbackTimer = Timer.scheduledTimer(withTimeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            guard let self, self.isPlaying, let start = self.playbackStartTime else { return }
            let elapsed = Date().timeIntervalSince(start)
            let posInLoop = elapsed.truncatingRemainder(dividingBy: self.loopDuration)
            self.loopProgress = Float(posInLoop / self.loopDuration)
            self.currentBeat = Int(posInLoop / (60.0 / self.bpm))
        }
    }

    func stopPlayback() {
        playbackTimer?.invalidate()
        playbackTimer = nil
        isPlaying = false
        loopProgress = 0
        currentBeat = 0
        lastPlaybackPos = -1

        ObrixBridge.shared()?.allNotesOff()
    }

    func togglePlayback() {
        if isPlaying { stopPlayback() } else { startPlayback() }
    }

    /// Fire all layer events that fall within the current ~33 ms frame window.
    /// Call this from a 30 Hz timer on the main thread (e.g. via `.onReceive` in ReefTab).
    func playEvents(noteOn: (Int, Float) -> Void, noteOff: (Int) -> Void) {
        guard isPlaying, let start = playbackStartTime else { return }
        let elapsed = Date().timeIntervalSince(start)
        let posInLoop = elapsed.truncatingRemainder(dividingBy: loopDuration)

        // Detect loop restart (position jumped backwards by more than jitter tolerance)
        let loopRestarted = posInLoop < lastPlaybackPos - 0.1

        let frameWindow: TimeInterval = 1.0 / 30.0

        for layer in layers {
            for event in layer {
                let diff = event.timestamp - posInLoop

                // Normal case: event falls in current frame window
                let inWindow = diff >= 0 && diff < frameWindow

                // Boundary case: we just looped and the event is near the start
                let atBoundary = loopRestarted && event.timestamp < frameWindow

                if inWindow || atBoundary {
                    if event.velocity > 0 { noteOn(event.midiNote, event.velocity) }
                    else { noteOff(event.midiNote) }
                }
            }
        }

        lastPlaybackPos = posInLoop
    }

    // MARK: - Export

    /// Export all layers' events for MIDI export
    func exportLayers() -> [[NoteEvent]] {
        return layers
    }

    // MARK: - Layer management

    func undoLastLayer() {
        guard !layers.isEmpty else { return }
        layers.removeLast()
        layerCount = layers.count
        // If no layers remain and we're not recording, stop playback too
        if layers.isEmpty && !isRecording {
            stopPlayback()
        }
    }

    func clearAllLayers() {
        autoStopWorkItem?.cancel()
        autoStopWorkItem = nil
        stopPlayback()
        if isRecording {
            isRecording = false
            currentLayer = []
        }
        layers.removeAll()
        layerCount = 0
    }
}
