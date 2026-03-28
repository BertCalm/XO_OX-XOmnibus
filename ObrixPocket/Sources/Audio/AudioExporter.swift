import Foundation
import AVFoundation

/// Records reef audio output to M4A files for sharing.
/// Spec Section 5.3: Record reef audio to M4A + 15-sec clip mode for social sharing.
final class AudioExporter: ObservableObject {
    @Published var isRecording = false
    @Published var recordingDuration: TimeInterval = 0
    @Published var lastExportURL: URL?

    private var audioFile: AVAudioFile?
    private var recordStartTime: Date?
    private var recordTimer: Timer?
    private let maxDuration: TimeInterval = 60 // 60-second max per spec

    // Ring buffer queue: audio tap enqueues here; writer timer drains on background queue.
    private var pendingBuffers: [AVAudioPCMBuffer] = []
    private let bufferLock = NSLock()
    private var writerTimer: Timer?

    // Tap polling: a Timer that drains -drainTapInto: at ~60 Hz and feeds queueBuffer.
    private var tapPollTimer: Timer?
    // Scratch buffer for the raw interleaved float data from the bridge tap.
    // Pre-allocated once: 4 096 frames × 2 ch = 8 192 floats.
    private let tapScratch = UnsafeMutablePointer<Float>.allocate(capacity: 8192)
    // Reuse a single AVAudioFormat for the tap to avoid per-tick allocation.
    private var tapFormat: AVAudioFormat?

    // MARK: - Recording

    /// Start recording reef audio output to M4A.
    /// Safe to call multiple times — no-op if already recording.
    func startRecording() {
        guard !isRecording else { return }

        let url = exportURL(name: "reef_\(dateString())")

        do {
            let settings: [String: Any] = [
                AVFormatIDKey: kAudioFormatMPEG4AAC,
                AVSampleRateKey: 48000.0,
                AVNumberOfChannelsKey: 2,
                AVEncoderBitRateKey: 256000
            ]
            audioFile = try AVAudioFile(forWriting: url, settings: settings)
            isRecording = true
            recordStartTime = Date()

            // Phase 1: ObrixBridge needs an output tap method to feed PCM buffers
            // into audioFile via AVAudioFile.write(from:). The file and timers are
            // set up here; the bridge tap calls queueBuffer(_:) on each render cycle.

            recordTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
                guard let self, let start = self.recordStartTime else { return }
                DispatchQueue.main.async {
                    self.recordingDuration = Date().timeIntervalSince(start)
                }
                if self.recordingDuration >= self.maxDuration {
                    self.stopRecording()
                }
            }

            // Drain pending buffers on a background queue every 50ms.
            // This keeps AVAudioFile.write(from:) off the audio render thread.
            writerTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] _ in
                self?.drainBuffers()
            }
        } catch {
            print("[AudioExporter] Failed to start recording: \(error)")
        }
    }

    /// Enqueue a PCM buffer from the audio engine tap.
    /// Called from the render callback — copies the buffer immediately and returns,
    /// so it is safe to use on the real-time audio thread.
    func queueBuffer(_ buffer: AVAudioPCMBuffer) {
        // Copy the buffer — the audio thread gives us a borrowed, short-lived buffer.
        guard let copy = AVAudioPCMBuffer(pcmFormat: buffer.format, frameCapacity: buffer.frameLength) else { return }
        copy.frameLength = buffer.frameLength
        if let dst = copy.floatChannelData?[0], let src = buffer.floatChannelData?[0] {
            memcpy(dst, src, Int(buffer.frameLength) * MemoryLayout<Float>.size)
        }
        if buffer.format.channelCount > 1,
           let dst = copy.floatChannelData?[1],
           let src = buffer.floatChannelData?[1] {
            memcpy(dst, src, Int(buffer.frameLength) * MemoryLayout<Float>.size)
        }

        bufferLock.lock()
        pendingBuffers.append(copy)
        bufferLock.unlock()
    }

    /// Drain all queued buffers into the open AVAudioFile.
    /// Called from the writer timer on the main/background RunLoop — never on the audio thread.
    private func drainBuffers() {
        bufferLock.lock()
        let buffers = pendingBuffers
        pendingBuffers.removeAll()
        bufferLock.unlock()

        guard let file = audioFile else { return }
        for buffer in buffers {
            try? file.write(from: buffer)
        }
    }

    /// Stop recording and finalize the M4A file.
    /// Safe to call multiple times — no-op if not recording.
    func stopRecording() {
        guard isRecording else { return }

        recordTimer?.invalidate()
        recordTimer = nil
        writerTimer?.invalidate()
        writerTimer = nil
        isRecording = false
        recordingDuration = 0
        recordStartTime = nil

        // Flush any buffers that arrived between the last drain tick and now.
        drainBuffers()

        if let file = audioFile {
            // AVAudioFile finalizes (closes) automatically on dealloc.
            // Capture the URL before releasing the reference.
            DispatchQueue.main.async {
                self.lastExportURL = file.url
            }
            audioFile = nil
        }
    }

    // MARK: - Live Recording via Bridge Output Tap

    /// Start recording reef audio by tapping the JUCE bridge output.
    /// Combines startRecording() (opens the M4A file) with an output tap on ObrixBridge
    /// polled at ~60 Hz to drain interleaved PCM into queueBuffer().
    func startLiveRecording() {
        guard !isRecording else { return }

        // Determine the live sample rate from the bridge (default 48 kHz).
        let sr = Double(ObrixBridge.shared()?.sampleRate() ?? 48000)
        tapFormat = AVAudioFormat(commonFormat: .pcmFormatFloat32,
                                  sampleRate: sr,
                                  channels: 2,
                                  interleaved: false)

        startRecording()

        ObrixBridge.shared()?.startOutputTap()

        // Poll the bridge tap at ~60 Hz and feed decoded buffers into the write queue.
        tapPollTimer = Timer.scheduledTimer(withTimeInterval: 1.0 / 60.0, repeats: true) { [weak self] _ in
            self?.pollTap()
        }
    }

    /// Stop live recording, flush, and finalise the M4A file.
    func stopLiveRecording() {
        tapPollTimer?.invalidate()
        tapPollTimer = nil
        ObrixBridge.shared()?.stopOutputTap()
        // Drain any final buffers that landed between the last poll and now.
        pollTap()
        stopRecording()
    }

    /// Drain one tap block from the bridge and forward it to queueBuffer().
    /// Called from tapPollTimer — runs on the main RunLoop.
    private func pollTap() {
        guard let fmt = tapFormat else { return }

        var channels: Int32 = 0
        let frames = ObrixBridge.shared()?.drainTap(into: tapScratch,
                                                    maxFrames: 4096,
                                                    outChannels: &channels) ?? 0
        guard frames > 0, channels > 0 else { return }

        // Build an AVAudioPCMBuffer from the interleaved raw floats.
        guard let pcm = AVAudioPCMBuffer(pcmFormat: fmt, frameCapacity: AVAudioFrameCount(frames)) else { return }
        pcm.frameLength = AVAudioFrameCount(frames)

        // De-interleave into AVAudioPCMBuffer's planar channel data.
        let ch = Int(channels)
        for c in 0 ..< min(ch, 2) {
            guard let plane = pcm.floatChannelData?[c] else { continue }
            for f in 0 ..< Int(frames) {
                plane[f] = tapScratch[f * ch + c]
            }
        }
        // If mono source, copy L → R.
        if ch == 1, let r = pcm.floatChannelData?[1], let l = pcm.floatChannelData?[0] {
            memcpy(r, l, Int(frames) * MemoryLayout<Float>.size)
        }

        queueBuffer(pcm)
    }

    // MARK: - 15-Second Clip (spec Section 5.3)

    /// Record a 15-second ambient clip for social sharing.
    ///
    /// Phase 1 implementation: records live output for exactly 15 seconds.
    /// Phase 2 will use offline render via OfflineAudioContext for background export.
    ///
    /// - Parameter completion: Called on the main thread with the exported URL, or nil on failure.
    func generateClip(completion: @escaping (URL?) -> Void) {
        guard !isRecording else {
            // Don't interrupt an existing long recording
            completion(nil)
            return
        }

        // Capture the URL before starting — stopRecording sets lastExportURL asynchronously,
        // so reading it in the asyncAfter closure would race against the async dispatch.
        let clipURL = exportURL(name: "reef_clip_\(dateString())")

        // Write to the pre-determined URL by initialising audioFile directly.
        do {
            let settings: [String: Any] = [
                AVFormatIDKey: kAudioFormatMPEG4AAC,
                AVSampleRateKey: 48000.0,
                AVNumberOfChannelsKey: 2,
                AVEncoderBitRateKey: 256000
            ]
            audioFile = try AVAudioFile(forWriting: clipURL, settings: settings)
            isRecording = true
            recordStartTime = Date()

            writerTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] _ in
                self?.drainBuffers()
            }
        } catch {
            print("[AudioExporter] generateClip: failed to open file: \(error)")
            completion(nil)
            return
        }

        DispatchQueue.main.asyncAfter(deadline: .now() + 15.0) { [weak self] in
            guard let self else {
                completion(nil)
                return
            }
            self.stopRecording()
            completion(clipURL) // Use the known URL directly, not lastExportURL
        }
    }

    // MARK: - 15-Second Social Clip (spec Section 5.3 — #WhatsYourSong)

    /// Generate a 15-second clip by auto-playing a pentatonic melody through the reef chain
    /// and recording the audio output via the JUCE tap.
    ///
    /// Combines startLiveRecording() with an auto-play timer so no manual performance is needed.
    /// The melody loops a C major pentatonic pattern at 0.5s per note for the full 15-second window.
    ///
    /// - Parameters:
    ///   - reefStore: Used to locate the first source slot and configure the engine.
    ///   - audioEngine: Used to apply cached params for the source's chain before playback starts.
    ///   - completion: Called on the main thread with the M4A URL on success, or nil on failure.
    func generateSocialClip(
        reefStore: ReefStore,
        audioEngine: AudioEngineManager,
        completion: @escaping (URL?) -> Void
    ) {
        guard !isRecording else {
            // Don't interrupt an active long recording.
            completion(nil)
            return
        }

        // Find the first source slot to configure the engine chain.
        guard let sourceSlot = reefStore.specimens.firstIndex(where: { $0?.category == .source }) else {
            completion(nil)
            return
        }

        // Apply cached params for the source's wired chain.
        audioEngine.applyCachedParams(for: sourceSlot)

        // Start capturing the live JUCE tap output.
        startLiveRecording()

        // C major pentatonic loop: C4 D E G A C5 G4 E D C A3 D4
        let notes = [60, 62, 64, 67, 69, 72, 67, 64, 62, 60, 57, 62]
        var noteIndex = 0
        var clipTimer: Timer?

        clipTimer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: true) { [weak self] timer in
            guard self != nil else { timer.invalidate(); return }

            let note = notes[noteIndex % notes.count]
            let velocity = Float.random(in: 0.55...0.80)

            ObrixBridge.shared()?.note(on: Int32(note), velocity: velocity)

            // Note off after 300ms — leaves a small gap before the next note.
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) {
                ObrixBridge.shared()?.noteOff(Int32(note))
            }

            noteIndex += 1
        }

        // Stop recording after exactly 15 seconds and deliver the URL.
        DispatchQueue.main.asyncAfter(deadline: .now() + 15.0) { [weak self] in
            clipTimer?.invalidate()
            ObrixBridge.shared()?.allNotesOff()

            guard let self else {
                completion(nil)
                return
            }

            // Capture the URL before stopLiveRecording clears the file reference.
            // stopLiveRecording sets lastExportURL asynchronously via DispatchQueue.main.async,
            // so we read it after the call completes on this same main-thread dispatch.
            self.stopLiveRecording()

            // lastExportURL is set synchronously inside the main-thread stopRecording path
            // (the DispatchQueue.main.async inside stopRecording executes after we return here).
            // Use asyncAfter with a negligible delay to let that assignment land.
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
                completion(self.lastExportURL)
            }
        }
    }

    /// Returns the URL of the last completed export for sharing, or nil if none exists.
    func shareURL() -> URL? {
        return lastExportURL
    }

    // MARK: - Helpers

    private func exportURL(name: String) -> URL {
        let dir = FileManager.default.temporaryDirectory
            .appendingPathComponent("ObrixExports", isDirectory: true)
        // Ignore error — if the directory already exists this is a no-op
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        return dir.appendingPathComponent("\(name).m4a")
    }

    private func dateString() -> String {
        let fmt = DateFormatter()
        fmt.dateFormat = "yyyyMMdd_HHmmss"
        return fmt.string(from: Date())
    }
}
