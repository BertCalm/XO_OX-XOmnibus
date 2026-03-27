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
            // into audioFile via AVAudioFile.write(from:). The file and timer are
            // set up here; the bridge tap calls writeBuffer(_:) on each render cycle.

            recordTimer = Timer.scheduledTimer(withTimeInterval: 0.1, repeats: true) { [weak self] _ in
                guard let self, let start = self.recordStartTime else { return }
                DispatchQueue.main.async {
                    self.recordingDuration = Date().timeIntervalSince(start)
                }
                if self.recordingDuration >= self.maxDuration {
                    self.stopRecording()
                }
            }
        } catch {
            print("[AudioExporter] Failed to start recording: \(error)")
        }
    }

    /// Write a PCM buffer from the audio engine tap into the open M4A file.
    /// Called from the render cycle — must not block.
    func writeBuffer(_ buffer: AVAudioPCMBuffer) {
        guard isRecording, let file = audioFile else { return }
        do {
            try file.write(from: buffer)
        } catch {
            // Non-fatal: one missed buffer is acceptable; log for diagnostics
            print("[AudioExporter] Buffer write failed: \(error)")
        }
    }

    /// Stop recording and finalize the M4A file.
    /// Safe to call multiple times — no-op if not recording.
    func stopRecording() {
        guard isRecording else { return }

        recordTimer?.invalidate()
        recordTimer = nil
        isRecording = false
        recordingDuration = 0
        recordStartTime = nil

        if let file = audioFile {
            // AVAudioFile finalizes (closes) automatically on dealloc.
            // Capture the URL before releasing the reference.
            DispatchQueue.main.async {
                self.lastExportURL = file.url
            }
            audioFile = nil
        }
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

        startRecording()

        DispatchQueue.main.asyncAfter(deadline: .now() + 15.0) { [weak self] in
            guard let self else {
                completion(nil)
                return
            }
            self.stopRecording()
            completion(self.lastExportURL)
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
