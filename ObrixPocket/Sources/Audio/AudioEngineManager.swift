import Foundation
import AVFoundation
import Combine

/// Manages the JUCE audio hosting layer for ObrixEngine.
/// Phase 0: Initializes audio session, creates JUCE bridge, routes touch events to notes.
final class AudioEngineManager: ObservableObject {
    @Published var isRunning = false
    @Published var cpuLoad: Float = 0.0

    private var audioSessionConfigured = false

    init() {
        configureAudioSession()
    }

    func start() {
        guard !isRunning else { return }

        // Phase 0: Start JUCE audio bridge
        ObrixBridge.shared.startAudio()
        isRunning = true
    }

    func stop() {
        ObrixBridge.shared.stopAudio()
        isRunning = false
    }

    // MARK: - Note Input (from ReefGrid touch events)

    func noteOn(slotIndex: Int, velocity: Float) {
        // Map slot index to MIDI note: C3 + slot index (gives C3-D#4 for 16 slots)
        let note = 60 + slotIndex
        ObrixBridge.shared.noteOn(Int32(note), velocity: velocity)
    }

    func noteOff(slotIndex: Int) {
        let note = 60 + slotIndex
        ObrixBridge.shared.noteOff(Int32(note))
    }

    func allNotesOff() {
        ObrixBridge.shared.allNotesOff()
    }

    // MARK: - Audio Session

    private func configureAudioSession() {
        let session = AVAudioSession.sharedInstance()
        do {
            // Start with playback — switch to playAndRecord when mic is needed (catch)
            try session.setCategory(.playback)
            try session.setPreferredIOBufferDuration(0.005) // 5ms = 256 samples at 48kHz
            try session.setActive(true)
            audioSessionConfigured = true

            // Interruption handling
            NotificationCenter.default.addObserver(
                self,
                selector: #selector(handleInterruption),
                name: AVAudioSession.interruptionNotification,
                object: session
            )

            // Secondary audio hint (for ducking reef ambient under other media)
            NotificationCenter.default.addObserver(
                self,
                selector: #selector(handleSecondaryAudioHint),
                name: AVAudioSession.silenceSecondaryAudioHintNotification,
                object: session
            )

            NotificationCenter.default.addObserver(
                self,
                selector: #selector(handleRouteChange),
                name: AVAudioSession.routeChangeNotification,
                object: session
            )
        } catch {
            print("[ObrixPocket] Audio session config failed: \(error)")
        }
    }

    @objc private func handleInterruption(_ notification: Notification) {
        guard let info = notification.userInfo,
              let typeValue = info[AVAudioSessionInterruptionTypeKey] as? UInt,
              let type = AVAudioSession.InterruptionType(rawValue: typeValue) else { return }

        switch type {
        case .began:
            stop()
        case .ended:
            guard let optionsValue = info[AVAudioSessionInterruptionOptionKey] as? UInt else { return }
            let options = AVAudioSession.InterruptionOptions(rawValue: optionsValue)
            if options.contains(.shouldResume) {
                do {
                    try AVAudioSession.sharedInstance().setActive(true)
                } catch {
                    print("[ObrixPocket] Failed to reactivate audio session: \(error)")
                }
                start()
            }
        @unknown default:
            break
        }
    }

    @objc private func handleRouteChange(_ notification: Notification) {
        guard let info = notification.userInfo,
              let reasonValue = info[AVAudioSessionRouteChangeReasonKey] as? UInt,
              let reason = AVAudioSession.RouteChangeReason(rawValue: reasonValue) else { return }

        if reason == .oldDeviceUnavailable {
            // Headphones unplugged — pause to prevent unexpected speaker output
            stop()
        }
    }

    @objc private func handleSecondaryAudioHint(_ notification: Notification) {
        guard let info = notification.userInfo,
              let typeValue = info[AVAudioSessionSilenceSecondaryAudioHintTypeKey] as? UInt,
              let type = AVAudioSession.SilenceSecondaryAudioHintType(rawValue: typeValue) else { return }

        switch type {
        case .begin:
            // Other audio started — duck reef ambient to -12dB
            ObrixBridge.shared.setOutputGain(0.25) // -12dB
        case .end:
            // Other audio stopped — restore
            ObrixBridge.shared.setOutputGain(1.0)
        @unknown default:
            break
        }
    }

    deinit {
        NotificationCenter.default.removeObserver(self)
    }
}
