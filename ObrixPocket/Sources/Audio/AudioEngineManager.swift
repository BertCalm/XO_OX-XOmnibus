import Foundation
import AVFoundation
import Combine

/// Manages the JUCE audio hosting layer for ObrixEngine.
/// Phase 0: Initializes audio session, creates JUCE bridge, routes touch events to notes.
final class AudioEngineManager: ObservableObject {
    @Published var isRunning = false
    @Published var cpuLoad: Float = 0.0
    @Published var hasPlayedFirstNote = false

    private var audioSessionConfigured = false
    private let midiOutput = MIDIOutputManager()

    init() {
        configureAudioSession()

        // Crash recovery: deactivate any stale audio session from a prior crash.
        // If a previous run crashed while audio was active, the session may still
        // be holding the audio route in a bad state (causing distortion in other apps).
        do {
            try AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
        } catch {
            // Expected on first launch — no session was active
        }
    }

    func start() {
        guard !isRunning else { return }

        // Activate audio session just before starting the bridge
        do {
            try AVAudioSession.sharedInstance().setActive(true)
        } catch {
            print("[ObrixPocket] Failed to activate audio session: \(error)")
        }

        // Phase 0: Start JUCE audio bridge
        ObrixBridge.shared()?.startAudio()
        isRunning = true
    }

    func stop() {
        ObrixBridge.shared()?.stopAudio()
        isRunning = false
    }

    // MARK: - Reef Configuration (push wired specimen params to engine)

    /// Apply the full reef config on launch (sets defaults).
    func applyReefConfiguration(_ reefStore: ReefStore) {
        // On launch, configure with the first occupied slot's chain
        if let firstOccupied = reefStore.specimens.firstIndex(where: { $0 != nil }) {
            applySlotChain(slotIndex: firstOccupied, reefStore: reefStore)
        }
    }

    /// Configure the OBRIX engine for a specific slot's wired chain.
    /// Called when a slot is tapped — reads the specimen + its wired connections.
    func applySlotChain(slotIndex: Int, reefStore: ReefStore) {
        guard let bridge = ObrixBridge.shared(),
              let specimen = reefStore.specimens[slotIndex] else { return }

        // Step 1: Reset engine to defaults (turn off unused modules) — immediate write
        bridge.setParameterImmediate("obrix_src1Type", value: 0)  // Off
        bridge.setParameterImmediate("obrix_src2Type", value: 0)  // Off
        bridge.setParameterImmediate("obrix_proc1Type", value: 0) // Off
        bridge.setParameterImmediate("obrix_proc2Type", value: 0) // Off
        bridge.setParameterImmediate("obrix_proc3Type", value: 0) // Off
        bridge.setParameterImmediate("obrix_fx1Type", value: 0)   // Off
        bridge.setParameterImmediate("obrix_fx2Type", value: 0)   // Off
        bridge.setParameterImmediate("obrix_fx3Type", value: 0)   // Off
        bridge.setParameterImmediate("obrix_mod1Depth", value: 0)
        bridge.setParameterImmediate("obrix_mod2Depth", value: 0)
        // Ensure envelope is audible
        bridge.setParameterImmediate("obrix_ampAttack", value: 0.01)
        bridge.setParameterImmediate("obrix_ampDecay", value: 0.3)
        bridge.setParameterImmediate("obrix_ampSustain", value: 0.7)
        bridge.setParameterImmediate("obrix_ampRelease", value: 0.5)

        // Step 2: Apply the tapped specimen
        applySpecimenToEngine(specimen, bridge: bridge)

        // Step 3: Walk wired connections and apply them too
        let wiredSpecimens = findWiredChain(from: slotIndex, reefStore: reefStore)
        for wired in wiredSpecimens {
            applySpecimenToEngine(wired, bridge: bridge)
        }
    }

    /// Find all specimens wired to/from a given slot (one level deep).
    private func findWiredChain(from slotIndex: Int, reefStore: ReefStore) -> [Specimen] {
        guard let specimen = reefStore.specimens[slotIndex] else { return [] }

        var chain: [Specimen] = []
        for route in reefStore.couplingRoutes {
            // Find specimens connected to this one (either direction)
            if route.sourceId == specimen.id {
                if let idx = reefStore.specimens.firstIndex(where: { $0?.id == route.destId }),
                   let connected = reefStore.specimens[idx] {
                    chain.append(connected)
                }
            } else if route.destId == specimen.id {
                if let idx = reefStore.specimens.firstIndex(where: { $0?.id == route.sourceId }),
                   let connected = reefStore.specimens[idx] {
                    chain.append(connected)
                }
            }
        }
        return chain
    }

    /// Push a single specimen's parameters to the engine based on its category.
    /// Uses setParameterImmediate for synchronous writes (no queue delay).
    private func applySpecimenToEngine(_ specimen: Specimen, bridge: ObrixBridge) {
        // First: enable the correct module type based on specimen category
        switch specimen.category {
        case .source:
            // Map specimen source type to engine source type
            let srcTypeMap: [String: Float] = [
                "polyblep-saw": 2, "polyblep-square": 3, "polyblep-tri": 4,
                "noise-white": 5, "noise-pink": 5,
                "wt-analog": 6, "wt-vocal": 6,
                "fm-basic": 7 // Pulse (closest to FM in OBRIX)
            ]
            let engineType = srcTypeMap[specimen.subtype] ?? 2 // Default saw
            bridge.setParameterImmediate("obrix_src1Type", value: engineType)

        case .processor:
            // Map specimen processor type to engine proc type
            let procTypeMap: [String: Float] = [
                "svf-lp": 1, "svf-hp": 2, "svf-bp": 3,
                "shaper-soft": 4, "shaper-hard": 4,
                "feedback": 5
            ]
            let engineType = procTypeMap[specimen.subtype] ?? 1 // Default LP
            bridge.setParameterImmediate("obrix_proc1Type", value: engineType)

        case .modulator:
            // Enable mod slot 2 as LFO
            bridge.setParameterImmediate("obrix_mod2Type", value: 2) // LFO
            bridge.setParameterImmediate("obrix_mod2Target", value: 2) // Filter Cutoff

        case .effect:
            // Map specimen effect type to engine FX type
            let fxTypeMap: [String: Float] = [
                "delay-stereo": 1, "chorus-lush": 2,
                "reverb-hall": 3, "dist-warm": 1
            ]
            let engineType = fxTypeMap[specimen.subtype] ?? 1
            bridge.setParameterImmediate("obrix_fx1Type", value: engineType)
        }

        // Then: apply all mapped parameter values
        for (specimenParam, value) in specimen.parameterState {
            if let mapping = Self.parameterMapping[specimenParam] {
                let scaledValue = mapping.scale(value)
                bridge.setParameterImmediate(mapping.engineParam, value: scaledValue)
            }
        }
    }

    // MARK: - Specimen → Engine Parameter Mapping

    private struct ParamMapping {
        let engineParam: String
        let scale: (Float) -> Float
    }

    /// Maps specimen parameter names to OBRIX engine parameter names with value scaling.
    /// Specimen params use simplified names (flt1, env1, lfo1) and 0-1 ranges.
    /// Engine params use full OBRIX names (proc1, amp, mod2) and actual ranges.
    private static let parameterMapping: [String: ParamMapping] = [
        // Sources — type index needs offset (specimen 0=saw → engine 2=saw)
        "obrix_src1Type": ParamMapping(engineParam: "obrix_src1Type",
            scale: { v in
                // Specimen: 0=saw,1=square,2=tri,3=noise,4=wt,5=fm
                // Engine:   0=off,1=sine,2=saw,3=square,4=tri,5=noise,6=wt,7=pulse,8=driftwood
                let map: [Int: Float] = [0: 2, 1: 3, 2: 4, 3: 5, 4: 6, 5: 7]
                return map[Int(v)] ?? 2
            }),
        "obrix_src1Tune": ParamMapping(engineParam: "obrix_src1Tune",
            scale: { $0 }), // Both -24 to 24, direct
        "obrix_src1Level": ParamMapping(engineParam: "obrix_srcMix",
            scale: { $0 }), // 0-1 → 0-1

        // Processors — name and range remapping
        "obrix_flt1Cutoff": ParamMapping(engineParam: "obrix_proc1Cutoff",
            scale: { v in 20.0 + v * (20000.0 - 20.0) }), // 0-1 → 20-20000 Hz
        "obrix_flt1Resonance": ParamMapping(engineParam: "obrix_proc1Reso",
            scale: { $0 }), // 0-1 → 0-1
        "obrix_flt1EnvDepth": ParamMapping(engineParam: "obrix_mod1Depth",
            scale: { $0 }), // -0.5 to 0.5 → -1 to 1 (close enough for now)

        // Envelope — name remapping + range scaling
        "obrix_env1Attack": ParamMapping(engineParam: "obrix_ampAttack",
            scale: { $0 }), // 0-0.3 → 0-10 (within range, just small values)
        "obrix_env1Decay": ParamMapping(engineParam: "obrix_ampDecay",
            scale: { $0 }), // 0-0.8 → 0-10
        "obrix_env1Sustain": ParamMapping(engineParam: "obrix_ampSustain",
            scale: { $0 }), // 0-1 → 0-1
        "obrix_env1Release": ParamMapping(engineParam: "obrix_ampRelease",
            scale: { $0 }), // 0-2 → 0-20

        // Modulators — map to mod slot 2 (LFO)
        "obrix_lfo1Rate": ParamMapping(engineParam: "obrix_mod2Rate",
            scale: { $0 }), // 0.01-10 → 0.01-30 (within range)
        "obrix_lfo1Depth": ParamMapping(engineParam: "obrix_mod2Depth",
            scale: { $0 }), // 0-1 → -1 to 1 (positive only for now)
        "obrix_lfo1Shape": ParamMapping(engineParam: "obrix_mod2Type",
            scale: { v in
                // Specimen: 0=env,1=lfo,2=s&h,3=random → Engine: 0=off,1=env,2=lfo,3=vel,4=at
                let map: [Int: Float] = [0: 1, 1: 2, 2: 2, 3: 2]
                return map[Int(v)] ?? 2
            }),

        // Effects — map to fx slot 1
        "obrix_fx1Mix": ParamMapping(engineParam: "obrix_fx1Mix",
            scale: { $0 }), // 0-1 → 0-1
        "obrix_fx1Param1": ParamMapping(engineParam: "obrix_fx1Param",
            scale: { $0 }), // 0-1 → 0-1
        "obrix_fx1Param2": ParamMapping(engineParam: "obrix_fx1Type",
            scale: { v in
                // Map continuous value to discrete FX type
                // 0=off, 1=delay, 2=chorus, 3=reverb
                if v < 0.25 { return 1 } // delay
                if v < 0.5  { return 2 } // chorus
                return 3 // reverb
            }),
    ]

    // MARK: - Note Input (from ReefGrid touch events)

    /// Reference to the reef store — set by ObrixPocketApp after launch
    weak var reefStoreRef: ReefStore?

    func noteOn(slotIndex: Int, velocity: Float) {
        // Configure the engine for this slot's specimen chain before playing
        if let reefStore = reefStoreRef {
            applySlotChain(slotIndex: slotIndex, reefStore: reefStore)
        }

        // Map slot index to MIDI note: C3 + slot index (gives C3-D#4 for 16 slots)
        let note = min(127, max(0, 60 + slotIndex))
        ObrixBridge.shared()?.note(on: Int32(note), velocity: velocity)
        midiOutput.sendNoteOn(note: UInt8(note), velocity: UInt8(velocity * 127), channel: UInt8(slotIndex % 16))
        if !hasPlayedFirstNote {
            hasPlayedFirstNote = true
        }
    }

    func noteOff(slotIndex: Int) {
        // Clamp to valid MIDI note range 0-127
        let note = min(127, max(0, 60 + slotIndex))
        ObrixBridge.shared()?.noteOff(Int32(note))
        midiOutput.sendNoteOff(note: UInt8(note), channel: UInt8(slotIndex % 16))
    }

    func allNotesOff() {
        ObrixBridge.shared()?.allNotesOff()
    }

    // MARK: - Audio Session

    private func configureAudioSession() {
        let session = AVAudioSession.sharedInstance()
        do {
            // Start with playback — switch to playAndRecord when mic is needed (catch)
            try session.setCategory(.playback)
            try session.setPreferredIOBufferDuration(0.005) // 5ms = 256 samples at 48kHz
            // setActive(true) is deferred to start() — called just before JUCE bridge starts
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
            // Headphones unplugged — pause briefly then resume on speaker
            stop()
            // Auto-resume on speaker after a brief delay
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { [weak self] in
                self?.start()
            }
        }
    }

    @objc private func handleSecondaryAudioHint(_ notification: Notification) {
        guard let info = notification.userInfo,
              let typeValue = info[AVAudioSessionSilenceSecondaryAudioHintTypeKey] as? UInt,
              let type = AVAudioSession.SilenceSecondaryAudioHintType(rawValue: typeValue) else { return }

        switch type {
        case .begin:
            // Other audio started — duck reef ambient to -12dB
            ObrixBridge.shared()?.setOutputGain(0.25) // -12dB
        case .end:
            // Other audio stopped — restore
            ObrixBridge.shared()?.setOutputGain(1.0)
        @unknown default:
            break
        }
    }

    deinit {
        NotificationCenter.default.removeObserver(self)
        // Ensure audio session is deactivated cleanly on teardown
        ObrixBridge.shared()?.stopAudio()
        try? AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
    }
}
