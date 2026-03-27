import Foundation
import AVFoundation
import Accelerate

/// Captures a 64-float ambient spectral DNA profile from the microphone during a catch encounter.
///
/// The capture lasts ~5 seconds. The result is a normalized magnitude spectrum binned into 64 bands.
/// No raw audio is ever stored — only the frequency distribution.
///
/// Offline/denied fallbacks (spec Section 3.4):
///   - Permission denied or undetermined → user is prompted once; refusal yields a neutral "Studio Caught" profile.
///   - AVAudioEngine start failure → neutral profile (never crashes the catch flow).
///
/// Audio session lifecycle (spec Section 3.4):
///   The caller (AudioEngineManager) owns .playback. SpectralCapture temporarily overrides
///   to .playAndRecord for the 5-second capture window, then restores .playback when done.
///   This is safe because JUCE audio is paused by the catch sheet presentation.
final class SpectralCapture: ObservableObject {
    @Published var isCapturing = false
    @Published var capturedProfile: [Float]?
    @Published var captureProgress: Float = 0

    private var audioEngine: AVAudioEngine?
    private var spectralAccumulator: [Float] = Array(repeating: 0, count: 64)
    private var frameCount: Int = 0

    /// Number of FFT frames to accumulate (~5 seconds at typical 256-sample callback rate).
    /// Actual duration may vary slightly with device sample rate and buffer size.
    private let targetFrames = 256

    // MARK: - Public API

    /// Start a ~5-second ambient spectral capture.
    /// Calls `completion` on the main thread with the 64-float normalized profile.
    func startCapture(completion: @escaping ([Float]) -> Void) {
        guard !isCapturing else { return }

        switch AVAudioSession.sharedInstance().recordPermission {
        case .granted:
            performCapture(completion: completion)

        case .undetermined:
            AVAudioSession.sharedInstance().requestRecordPermission { [weak self] granted in
                DispatchQueue.main.async {
                    if granted {
                        self?.performCapture(completion: completion)
                    } else {
                        // User declined — deliver neutral "Studio Caught" profile
                        completion(self?.neutralProfile() ?? Array(repeating: 0.5, count: 64))
                    }
                }
            }

        case .denied:
            // Microphone access previously denied — neutral profile, no retry prompt
            completion(neutralProfile())

        @unknown default:
            completion(neutralProfile())
        }
    }

    /// Cancel a capture in progress. Cleans up the engine and resets state.
    func cancelCapture() {
        stopEngine()
        isCapturing = false
        captureProgress = 0
        capturedProfile = nil
    }

    // MARK: - Private: Capture Flow

    private func performCapture(completion: @escaping ([Float]) -> Void) {
        isCapturing = true
        spectralAccumulator = Array(repeating: 0, count: 64)
        frameCount = 0
        captureProgress = 0

        // Temporarily upgrade audio session to playAndRecord for mic access.
        // defaultToSpeaker keeps JUCE output on the speaker while we capture.
        do {
            try AVAudioSession.sharedInstance().setCategory(.playAndRecord, options: [.defaultToSpeaker])
            try AVAudioSession.sharedInstance().setActive(true)
        } catch {
            print("[SpectralCapture] Session override failed: \(error)")
            isCapturing = false
            completion(neutralProfile())
            return
        }

        let engine = AVAudioEngine()
        let inputNode = engine.inputNode
        let format = inputNode.outputFormat(forBus: 0)

        // Install tap on the input bus — receives raw PCM frames
        inputNode.installTap(onBus: 0, bufferSize: 2048, format: format) { [weak self] buffer, _ in
            self?.processBuffer(buffer)
        }

        do {
            try engine.start()
            audioEngine = engine
        } catch {
            print("[SpectralCapture] Engine start failed: \(error)")
            isCapturing = false
            completion(neutralProfile())
            return
        }

        // Collect for 5 seconds then finalize
        DispatchQueue.main.asyncAfter(deadline: .now() + 5.0) { [weak self] in
            guard let self, self.isCapturing else { return }
            self.stopEngine()

            // Normalize: scale so the loudest band = 1.0
            let maxVal = self.spectralAccumulator.max() ?? 0.0
            let normalized: [Float]
            if maxVal > 0 {
                normalized = self.spectralAccumulator.map { $0 / maxVal }
            } else {
                // Silent environment — use neutral profile
                normalized = self.neutralProfile()
            }

            self.capturedProfile = normalized
            self.isCapturing = false
            self.captureProgress = 1.0

            // Restore playback-only session
            try? AVAudioSession.sharedInstance().setCategory(.playback)

            completion(normalized)
        }
    }

    // MARK: - Private: FFT Processing

    /// Processes one PCM buffer: runs FFT, bins magnitudes into 64 bands, accumulates.
    private func processBuffer(_ buffer: AVAudioPCMBuffer) {
        guard let channelData = buffer.floatChannelData?[0] else { return }
        let frameLength = Int(buffer.frameLength)
        guard frameLength >= 64 else { return } // Too short to FFT

        // frameLength must be a power of 2 for vDSP_fft_zrip
        let log2n = vDSP_Length(floor(log2(Float(frameLength))))
        let fftSize = 1 << Int(log2n) // largest power-of-2 ≤ frameLength
        guard fftSize >= 64 else { return }

        guard let fftSetup = vDSP_create_fftsetup(log2n, FFTRadix(kFFTRadix2)) else { return }
        defer { vDSP_destroy_fftsetup(fftSetup) }

        let halfSize = fftSize / 2

        var realp = [Float](repeating: 0, count: halfSize)
        var imagp = [Float](repeating: 0, count: halfSize)

        realp.withUnsafeMutableBufferPointer { realBuf in
            imagp.withUnsafeMutableBufferPointer { imagBuf in
                var splitComplex = DSPSplitComplex(
                    realp: realBuf.baseAddress!,
                    imagp: imagBuf.baseAddress!
                )

                // Pack interleaved real samples into split complex (imaginary = 0)
                channelData.withMemoryRebound(to: DSPComplex.self, capacity: halfSize) { complexPtr in
                    vDSP_ctoz(complexPtr, 2, &splitComplex, 1, vDSP_Length(halfSize))
                }

                vDSP_fft_zrip(fftSetup, &splitComplex, 1, log2n, FFTDirection(FFT_FORWARD))

                // Compute magnitude squared — cheap and monotonic with magnitude
                var magnitudes = [Float](repeating: 0, count: halfSize)
                vDSP_zvmags(&splitComplex, 1, &magnitudes, 1, vDSP_Length(halfSize))

                // Bin into 64 bands (linear frequency subdivision — fine for environmental capture)
                let binsPerBand = max(1, halfSize / 64)
                for band in 0..<64 {
                    let start = band * binsPerBand
                    let end   = min(start + binsPerBand, halfSize)
                    guard end > start else { continue }
                    let slice = magnitudes[start..<end]
                    let avg = slice.reduce(0, +) / Float(slice.count)
                    spectralAccumulator[band] += avg
                }
            }
        }

        frameCount += 1
        let progress = min(1.0, Float(frameCount) / Float(targetFrames))
        DispatchQueue.main.async { [weak self] in
            self?.captureProgress = progress
        }
    }

    // MARK: - Private: Engine Teardown

    private func stopEngine() {
        audioEngine?.inputNode.removeTap(onBus: 0)
        audioEngine?.stop()
        audioEngine = nil
    }

    // MARK: - Neutral Profile ("Studio Caught")

    /// A flat 0.5 profile used when the microphone is unavailable.
    /// Specimens tagged "Studio Caught" in provenance; gameplay and rarity are unaffected.
    private func neutralProfile() -> [Float] {
        Array(repeating: 0.5, count: 64)
    }
}
