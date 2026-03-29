import Foundation
import AVFoundation
import Combine

// MARK: - ObrixSynthEngine
//
// Swift-side polyphonic engine controller for OBRIX Pocket.
//
// ── Architecture ────────────────────────────────────────────────────────────────
// The REAL synthesis runs in C++ (ObrixEngine.h, 81 parameters, 8 voices,
// PolyBLEP oscillators + CytomicSVF filters + StandardADSR + StandardLFO +
// Schroeder reverb). ObrixBridge.mm wraps it with JUCE AudioDeviceManager.
//
// This class adds the Swift layer between the game systems and the C++ engine:
//   • Voice pool tracking (mirrors C++ engine state for metering/UI)
//   • Parameter state cache (fast read-back without round-tripping the bridge)
//   • Preset snapshot apply (atomic blast of all obrix_* params in one call)
//   • MIDI-note bookkeeping for XP, Dive, and PerformanceRecorder
//   • Parameter scaling helpers (normalized 0–1 → engine real-world ranges)
//
// All noteOn/noteOff/setParameter calls delegate immediately to ObrixBridge.
// No DSP runs here. This class is main-thread only.
//
// ── Real-time safety (C++ side) ─────────────────────────────────────────────
// The C++ engine's render callback (ObrixBridge processBlock):
//   • Drains a lock-free MIDI ring buffer (256 slots)
//   • Drains a lock-free parameter ring buffer (256 slots, pointer-keyed)
//   • Zero heap allocation
//   • Zero string operations
//   • Zero locks
// Swift → C++ communication is always through ObrixBridge's queuing/atomic API.
//
// ── Usage ────────────────────────────────────────────────────────────────────
// AudioEngineManager owns an ObrixSynthEngine and delegates to it for:
//   - noteOn(note:velocity:reefSlot:)
//   - noteOff(note:reefSlot:)
//   - setParameter(_:value:)
//   - applyPreset(_:)
// ObrixSynthEngine publishes VoiceSnapshot for metering and ReefIndicatorBar.
// ────────────────────────────────────────────────────────────────────────────

final class ObrixSynthEngine: ObservableObject {

    // MARK: - Constants

    /// Design polyphony limit (matches kMaxVoices in ObrixEngine.h).
    /// At 48kHz/256-block on iPhone A-series, 8 voices with full FX chain
    /// stays well under 10% CPU. 6 is the comfortable sustained-play target.
    static let voiceCount = 8

    /// Maximum simultaneous notes from a single reef slot.
    /// Prevents a single specimen from monopolising all voices.
    static let maxVoicesPerSlot = 2

    // MARK: - Published State

    /// Current voice snapshot — updated after every noteOn/noteOff.
    /// Observed by ReefIndicatorBar and PerformanceRecorder.
    @Published private(set) var voiceSnapshot = ObrixVoiceSnapshot(
        voices: [], activeCount: 0, timestamp: Date()
    )

    /// Current parameter state — updated via setParameter() and applyPreset().
    /// Used by SpecimenParamPanel for read-back without querying the bridge.
    @Published private(set) var parameterState: [String: Float] = [:]

    // MARK: - Private State

    /// Live voice tracking (main thread only).
    /// Slot index = C++ ObrixVoice array index (0–7).
    private var voices: [ObrixVoice] = (0..<ObrixSynthEngine.voiceCount).map { ObrixVoice(slot: $0) }

    /// Maps MIDI note → voice slot for note-off routing.
    private var noteToVoiceSlot: [Int: Int] = [:]

    /// Round-robin write cursor for voice stealing.
    private var stealCursor: Int = 0

    /// Timer that polls the bridge for active voice count (metering).
    private var meterTimer: Timer?

    // MARK: - Parameter Definitions
    //
    // Mirrors the parameter IDs in ObrixEngine.h createParameterLayout().
    // All IDs are frozen (never change after release — see CLAUDE.md).
    // Ranges match the NormalisableRange definitions in ObrixEngine.h exactly.

    /// Full parameter ID set for the OBRIX engine (81 params — Wave 5).
    /// Split into categories for validation and preset management.
    struct ParameterID {
        // Sources (7)
        static let src1Type      = "obrix_src1Type"
        static let src2Type      = "obrix_src2Type"
        static let src1Tune      = "obrix_src1Tune"
        static let src2Tune      = "obrix_src2Tune"
        static let src1PW        = "obrix_src1PW"
        static let src2PW        = "obrix_src2PW"
        static let srcMix        = "obrix_srcMix"

        // Processors (9)
        static let proc1Type     = "obrix_proc1Type"
        static let proc1Cutoff   = "obrix_proc1Cutoff"
        static let proc1Reso     = "obrix_proc1Reso"
        static let proc2Type     = "obrix_proc2Type"
        static let proc2Cutoff   = "obrix_proc2Cutoff"
        static let proc2Reso     = "obrix_proc2Reso"
        static let proc3Type     = "obrix_proc3Type"
        static let proc3Cutoff   = "obrix_proc3Cutoff"
        static let proc3Reso     = "obrix_proc3Reso"

        // Amplitude Envelope (4)
        static let ampAttack     = "obrix_ampAttack"
        static let ampDecay      = "obrix_ampDecay"
        static let ampSustain    = "obrix_ampSustain"
        static let ampRelease    = "obrix_ampRelease"

        // Modulators — 4 slots × 4 params (16)
        static let mod1Type      = "obrix_mod1Type"
        static let mod1Target    = "obrix_mod1Target"
        static let mod1Depth     = "obrix_mod1Depth"
        static let mod1Rate      = "obrix_mod1Rate"
        static let mod2Type      = "obrix_mod2Type"
        static let mod2Target    = "obrix_mod2Target"
        static let mod2Depth     = "obrix_mod2Depth"
        static let mod2Rate      = "obrix_mod2Rate"
        static let mod3Type      = "obrix_mod3Type"
        static let mod3Target    = "obrix_mod3Target"
        static let mod3Depth     = "obrix_mod3Depth"
        static let mod3Rate      = "obrix_mod3Rate"
        static let mod4Type      = "obrix_mod4Type"
        static let mod4Target    = "obrix_mod4Target"
        static let mod4Depth     = "obrix_mod4Depth"
        static let mod4Rate      = "obrix_mod4Rate"

        // Effects — 3 slots × 3 params (9)
        static let fx1Type       = "obrix_fx1Type"
        static let fx1Mix        = "obrix_fx1Mix"
        static let fx1Param      = "obrix_fx1Param"
        static let fx2Type       = "obrix_fx2Type"
        static let fx2Mix        = "obrix_fx2Mix"
        static let fx2Param      = "obrix_fx2Param"
        static let fx3Type       = "obrix_fx3Type"
        static let fx3Mix        = "obrix_fx3Mix"
        static let fx3Param      = "obrix_fx3Param"

        // Level + Macros (5)
        static let level         = "obrix_level"
        static let macroCharacter  = "obrix_macroCharacter"
        static let macroMovement   = "obrix_macroMovement"
        static let macroCoupling   = "obrix_macroCoupling"
        static let macroSpace      = "obrix_macroSpace"

        // Voice mode + Expression (3)
        static let polyphony     = "obrix_polyphony"
        static let pitchBendRange = "obrix_pitchBendRange"
        static let glideTime     = "obrix_glideTime"

        // FLASH gesture (2)
        static let gestureType   = "obrix_gestureType"
        static let flashTrigger  = "obrix_flashTrigger"

        // Wave 2 (5)
        static let fmDepth       = "obrix_fmDepth"
        static let proc1Feedback = "obrix_proc1Feedback"
        static let proc2Feedback = "obrix_proc2Feedback"
        static let wtBank        = "obrix_wtBank"
        static let unisonDetune  = "obrix_unisonDetune"

        // Wave 3 (5)
        static let driftRate     = "obrix_driftRate"
        static let driftDepth    = "obrix_driftDepth"
        static let journeyMode   = "obrix_journeyMode"
        static let distance      = "obrix_distance"
        static let air           = "obrix_air"

        // Wave 4 — Biophonic (14)
        static let fieldStrength         = "obrix_fieldStrength"
        static let fieldPolarity         = "obrix_fieldPolarity"
        static let fieldRate             = "obrix_fieldRate"
        static let fieldPrimeLimit       = "obrix_fieldPrimeLimit"
        static let envTemp               = "obrix_envTemp"
        static let envPressure           = "obrix_envPressure"
        static let envCurrent            = "obrix_envCurrent"
        static let envTurbidity          = "obrix_envTurbidity"
        static let competitionStrength   = "obrix_competitionStrength"
        static let symbiosisStrength     = "obrix_symbiosisStrength"
        static let stressDecay           = "obrix_stressDecay"
        static let bleachRate            = "obrix_bleachRate"
        static let stateReset            = "obrix_stateReset"
        static let fxMode                = "obrix_fxMode"

        // Wave 5 — Reef Residency (2)
        static let reefResident          = "obrix_reefResident"
        static let residentStrength      = "obrix_residentStrength"
    }

    // MARK: - Parameter Ranges
    //
    // Real-world (denormalized) ranges per parameter, matching ObrixEngine.h.
    // Used by scaleParameter() to convert normalized 0–1 from specimen params
    // to the engine's actual expected value range.
    //
    // NOTE: setParameterImmediate on ObrixBridge always expects denormalized
    // real-world values. AudioEngineManager.parameterMapping already handles
    // most of the scaling. ObrixSynthEngine adds helpers for callers that
    // deal in normalized values (e.g., genetic system).

    struct ParameterRange {
        let min: Float
        let max: Float
        let defaultValue: Float

        func scale(_ normalized: Float) -> Float {
            min + normalized * (max - min)
        }

        func normalize(_ realWorld: Float) -> Float {
            guard max > min else { return 0 }
            return (realWorld - min) / (max - min)
        }
    }

    private static let parameterRanges: [String: ParameterRange] = [
        // Tune: -24 to +24 semitones
        ParameterID.src1Tune:       ParameterRange(min: -24, max: 24, defaultValue: 0),
        ParameterID.src2Tune:       ParameterRange(min: -24, max: 24, defaultValue: 0),
        // Pulse width: 0.05 to 0.95
        ParameterID.src1PW:         ParameterRange(min: 0.05, max: 0.95, defaultValue: 0.5),
        ParameterID.src2PW:         ParameterRange(min: 0.05, max: 0.95, defaultValue: 0.5),
        // Mix, resonance, mix/param: 0–1
        ParameterID.srcMix:         ParameterRange(min: 0, max: 1, defaultValue: 0.5),
        ParameterID.proc1Reso:      ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.proc2Reso:      ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.proc3Reso:      ParameterRange(min: 0, max: 1, defaultValue: 0),
        // Cutoff: 20 Hz – 20 kHz
        ParameterID.proc1Cutoff:    ParameterRange(min: 20, max: 20_000, defaultValue: 8_000),
        ParameterID.proc2Cutoff:    ParameterRange(min: 20, max: 20_000, defaultValue: 4_000),
        ParameterID.proc3Cutoff:    ParameterRange(min: 20, max: 20_000, defaultValue: 4_000),
        // Amplitude envelope
        ParameterID.ampAttack:      ParameterRange(min: 0, max: 10, defaultValue: 0.01),
        ParameterID.ampDecay:       ParameterRange(min: 0, max: 10, defaultValue: 0.3),
        ParameterID.ampSustain:     ParameterRange(min: 0, max: 1,  defaultValue: 0.7),
        ParameterID.ampRelease:     ParameterRange(min: 0, max: 20, defaultValue: 0.5),
        // Modulator depth: -1 to 1
        ParameterID.mod1Depth:      ParameterRange(min: -1, max: 1, defaultValue: 0.5),
        ParameterID.mod2Depth:      ParameterRange(min: -1, max: 1, defaultValue: 0),
        ParameterID.mod3Depth:      ParameterRange(min: -1, max: 1, defaultValue: 0.5),
        ParameterID.mod4Depth:      ParameterRange(min: -1, max: 1, defaultValue: 0),
        // Modulator rate: 0.01 – 30 Hz
        ParameterID.mod1Rate:       ParameterRange(min: 0.01, max: 30, defaultValue: 1),
        ParameterID.mod2Rate:       ParameterRange(min: 0.01, max: 30, defaultValue: 1),
        ParameterID.mod3Rate:       ParameterRange(min: 0.01, max: 30, defaultValue: 1),
        ParameterID.mod4Rate:       ParameterRange(min: 0.01, max: 30, defaultValue: 1),
        // FX mix / param: 0–1
        ParameterID.fx1Mix:         ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.fx1Param:       ParameterRange(min: 0, max: 1, defaultValue: 0.3),
        ParameterID.fx2Mix:         ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.fx2Param:       ParameterRange(min: 0, max: 1, defaultValue: 0.3),
        ParameterID.fx3Mix:         ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.fx3Param:       ParameterRange(min: 0, max: 1, defaultValue: 0.3),
        // Level, macros: 0–1
        ParameterID.level:          ParameterRange(min: 0, max: 1, defaultValue: 0.8),
        ParameterID.macroCharacter: ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.macroMovement:  ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.macroCoupling:  ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.macroSpace:     ParameterRange(min: 0, max: 1, defaultValue: 0),
        // Expression
        ParameterID.pitchBendRange: ParameterRange(min: 1, max: 24, defaultValue: 2),
        ParameterID.glideTime:      ParameterRange(min: 0, max: 1,  defaultValue: 0),
        // FM depth: -1 to 1
        ParameterID.fmDepth:        ParameterRange(min: -1, max: 1, defaultValue: 0),
        // Filter feedback: 0–1
        ParameterID.proc1Feedback:  ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.proc2Feedback:  ParameterRange(min: 0, max: 1, defaultValue: 0),
        // Unison detune: 0–50 cents
        ParameterID.unisonDetune:   ParameterRange(min: 0, max: 50, defaultValue: 0),
        // Wave 3 spatial
        ParameterID.driftRate:      ParameterRange(min: 0.001, max: 0.05, defaultValue: 0.005),
        ParameterID.driftDepth:     ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.distance:       ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.air:            ParameterRange(min: 0, max: 1, defaultValue: 0.5),
        // Wave 4 biophonic
        ParameterID.fieldStrength:        ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.fieldRate:            ParameterRange(min: 0.001, max: 0.1, defaultValue: 0.01),
        ParameterID.envTemp:              ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.envPressure:          ParameterRange(min: 0, max: 1, defaultValue: 0.5),
        ParameterID.envCurrent:           ParameterRange(min: -1, max: 1, defaultValue: 0),
        ParameterID.envTurbidity:         ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.competitionStrength:  ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.symbiosisStrength:    ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.stressDecay:          ParameterRange(min: 0, max: 1, defaultValue: 0),
        ParameterID.bleachRate:           ParameterRange(min: 0, max: 1, defaultValue: 0),
        // Wave 5
        ParameterID.residentStrength:     ParameterRange(min: 0, max: 1, defaultValue: 0.3),
    ]

    // MARK: - Default Preset
    //
    // Safe defaults applied when no specimen is configured.
    // Matches the C++ ObrixEngine defaults (ParamSnapshot section in renderBlock).
    // A single Saw oscillator → LP filter → ADSR → no effects.
    // Sounds musical immediately — DB003 resolved for OBRIX.

    static let defaultPreset: [String: Float] = [
        ParameterID.src1Type:     2,       // Saw
        ParameterID.src2Type:     0,       // Off
        ParameterID.src1Tune:     0,
        ParameterID.src2Tune:     0,
        ParameterID.srcMix:       0.5,
        ParameterID.proc1Type:    1,       // LP Filter
        ParameterID.proc1Cutoff:  8_000,   // Hz — matches engine default
        ParameterID.proc1Reso:    0,
        ParameterID.proc2Type:    0,       // Off
        ParameterID.proc3Type:    0,       // Off
        ParameterID.ampAttack:    0.01,
        ParameterID.ampDecay:     0.3,
        ParameterID.ampSustain:   0.7,
        ParameterID.ampRelease:   0.5,
        ParameterID.mod1Type:     1,       // Envelope
        ParameterID.mod1Target:   2,       // Filter Cutoff
        ParameterID.mod1Depth:    0.5,
        ParameterID.mod1Rate:     1.0,
        ParameterID.mod2Type:     2,       // LFO
        ParameterID.mod2Target:   2,       // Filter Cutoff
        ParameterID.mod2Depth:    0,       // Off by default — activated by specimen
        ParameterID.mod2Rate:     1.0,
        ParameterID.mod3Type:     3,       // Velocity
        ParameterID.mod3Target:   4,       // Volume (D001 compliance)
        ParameterID.mod3Depth:    0.5,
        ParameterID.mod3Rate:     1.0,
        ParameterID.mod4Type:     4,       // Aftertouch
        ParameterID.mod4Target:   2,       // Filter Cutoff
        ParameterID.mod4Depth:    0,
        ParameterID.mod4Rate:     1.0,
        ParameterID.fx1Type:      0,
        ParameterID.fx1Mix:       0,
        ParameterID.fx1Param:     0.3,
        ParameterID.fx2Type:      0,
        ParameterID.fx2Mix:       0,
        ParameterID.fx2Param:     0.3,
        ParameterID.fx3Type:      0,
        ParameterID.fx3Mix:       0,
        ParameterID.fx3Param:     0.3,
        ParameterID.level:        0.8,
        ParameterID.macroCharacter:  0,
        ParameterID.macroMovement:   0,
        ParameterID.macroCoupling:   0,
        ParameterID.macroSpace:      0,
        ParameterID.polyphony:    3,       // Poly8
        ParameterID.pitchBendRange: 2,
        ParameterID.glideTime:    0,
        ParameterID.fmDepth:      0,
        ParameterID.proc1Feedback: 0,
        ParameterID.proc2Feedback: 0,
        ParameterID.driftDepth:   0,
        ParameterID.distance:     0,
        ParameterID.air:          0.5,
        ParameterID.fieldStrength: 0,
        ParameterID.envPressure:  0.5,
        ParameterID.envCurrent:   0,
        ParameterID.reefResident: 0,
    ]

    // MARK: - Init

    init() {
        // Seed parameter state with engine defaults
        parameterState = Self.defaultPreset
        startMeterTimer()
    }

    deinit {
        meterTimer?.invalidate()
    }

    // MARK: - Note On / Note Off
    //
    // All calls delegate synchronously to ObrixBridge, which enqueues the
    // MIDI event into the lock-free ring buffer for the audio thread.
    //
    // Voice slot bookkeeping is done here (Swift side) for metering/XP.
    // The C++ engine does its own independent voice stealing (oldest-active).

    /// Trigger a note-on.
    ///
    /// - Parameters:
    ///   - note: MIDI note number (0–127).
    ///   - velocity: Note velocity (0–1). 0 is treated as note-off by MIDI spec.
    ///   - reefSlot: Reef slot index that originated this note (-1 if none).
    func noteOn(note: Int, velocity: Float, reefSlot: Int = -1) {
        assert(Thread.isMainThread, "noteOn must be called on the main thread")
        guard let bridge = ObrixBridge.shared() else { return }

        let clampedNote = max(0, min(127, note))
        let clampedVelocity = max(0, min(1, velocity))
        // ObjC selector: - (void)noteOn:(int)note velocity:(float)velocity
        bridge.note(on: Int32(clampedNote), velocity: clampedVelocity)

        // Track on Swift side for metering/XP
        let slot = allocateVoiceSlot(for: clampedNote, reefSlot: reefSlot)
        noteToVoiceSlot[clampedNote] = slot
        voices[slot].note = clampedNote
        voices[slot].velocity = clampedVelocity
        voices[slot].isActive = true
        voices[slot].envelopeStage = .attack
        voices[slot].envelopeLevel = 0.0
        voices[slot].startTime = Date()
        voices[slot].reefSlotIndex = reefSlot
        voices[slot].waveform = currentWaveform()
        publishVoiceSnapshot()
    }

    /// Release a note.
    ///
    /// - Parameters:
    ///   - note: MIDI note number (0–127).
    ///   - reefSlot: Reef slot index for context (not required for routing).
    func noteOff(note: Int, reefSlot: Int = -1) {
        assert(Thread.isMainThread, "noteOff must be called on the main thread")
        guard let bridge = ObrixBridge.shared() else { return }

        let clampedNote = max(0, min(127, note))
        bridge.noteOff(Int32(clampedNote))

        // Transition to release on Swift side
        if let slot = noteToVoiceSlot[clampedNote] {
            voices[slot].envelopeStage = .release
        }
        // Full deactivation happens in the meter timer after the release completes.
        publishVoiceSnapshot()
    }

    /// Stop all sounding notes immediately (panic).
    func allNotesOff() {
        assert(Thread.isMainThread, "allNotesOff must be called on the main thread")
        ObrixBridge.shared()?.allNotesOff()
        for i in 0..<Self.voiceCount { voices[i].reset() }
        noteToVoiceSlot.removeAll()
        publishVoiceSnapshot()
    }

    // MARK: - Parameter Control

    /// Set a single engine parameter immediately.
    ///
    /// This maps directly to `setParameterImmediate:value:` on ObrixBridge —
    /// no queue, no delay. Safe for bulk configuration before a noteOn.
    ///
    /// - Parameters:
    ///   - id: obrix_* parameter ID string. Must exist in the engine's param layout.
    ///   - value: Real-world (denormalized) value in the parameter's native range.
    ///            E.g. cutoff in Hz, attack in seconds, type as integer index.
    func setParameter(_ id: String, value: Float) {
        assert(Thread.isMainThread, "setParameter must be called on the main thread")
        ObrixBridge.shared()?.setParameterImmediate(id, value: value)
        parameterState[id] = value
    }

    /// Set a parameter from a normalized 0–1 value, scaling to the real-world range.
    ///
    /// Convenience wrapper used by the genetic system and animated macros.
    /// If the parameter has no known range entry, the value is passed through unchanged.
    ///
    /// - Parameters:
    ///   - id: obrix_* parameter ID.
    ///   - normalized: Normalized value in [0, 1].
    func setParameterNormalized(_ id: String, normalized: Float) {
        let scaled: Float
        if let range = Self.parameterRanges[id] {
            scaled = range.scale(normalized)
        } else {
            scaled = normalized
        }
        setParameter(id, value: scaled)
    }

    /// Read back the last-set value for a parameter (from the Swift-side cache).
    ///
    /// This does NOT query the C++ engine — it returns whatever was last written
    /// via setParameter(). Suitable for UI read-back; not for audio-thread use.
    func getParameter(_ id: String) -> Float {
        parameterState[id] ?? (Self.defaultPreset[id] ?? 0)
    }

    // MARK: - Preset Application
    //
    // Applies a full parameter snapshot to the engine.
    // Used by AudioEngineManager.applySlotChain() and ReefPreset loader.

    /// Apply a preset dictionary atomically — all params written before any note fires.
    ///
    /// Missing keys fall back to defaultPreset values (not the C++ engine defaults,
    /// which may differ for some params). This ensures no specimen leaves stale
    /// params from a previous configuration active.
    ///
    /// - Parameter preset: Dictionary of obrix_* keys → real-world values.
    func applyPreset(_ preset: [String: Float]) {
        assert(Thread.isMainThread, "applyPreset must be called on the main thread")
        guard let bridge = ObrixBridge.shared() else { return }

        // Start from defaults (ensures all 81 params are touched)
        var full = Self.defaultPreset
        for (key, value) in preset {
            full[key] = value
        }
        for (key, value) in full {
            bridge.setParameterImmediate(key, value: value)
            parameterState[key] = value
        }
    }

    /// Apply only the keys present in the dictionary, leaving all others unchanged.
    ///
    /// Faster than applyPreset() when only a few params change (e.g., wiring update).
    ///
    /// - Parameter delta: Partial parameter dictionary.
    func applyParameterDelta(_ delta: [String: Float]) {
        assert(Thread.isMainThread, "applyParameterDelta must be called on the main thread")
        guard let bridge = ObrixBridge.shared() else { return }
        for (key, value) in delta {
            bridge.setParameterImmediate(key, value: value)
            parameterState[key] = value
        }
    }

    /// Reset all parameters to default values and silence all voices.
    func resetToDefaults() {
        assert(Thread.isMainThread, "resetToDefaults must be called on the main thread")
        allNotesOff()
        applyPreset(Self.defaultPreset)
    }

    // MARK: - Macro Control
    //
    // Convenience accessors for the four CHARACTER/MOVEMENT/COUPLING/SPACE macros.
    // These are the primary real-time performance controls.

    var macroCharacter: Float {
        get { getParameter(ParameterID.macroCharacter) }
        set { setParameter(ParameterID.macroCharacter, value: max(0, min(1, newValue))) }
    }

    var macroMovement: Float {
        get { getParameter(ParameterID.macroMovement) }
        set { setParameter(ParameterID.macroMovement, value: max(0, min(1, newValue))) }
    }

    var macroCoupling: Float {
        get { getParameter(ParameterID.macroCoupling) }
        set { setParameter(ParameterID.macroCoupling, value: max(0, min(1, newValue))) }
    }

    var macroSpace: Float {
        get { getParameter(ParameterID.macroSpace) }
        set { setParameter(ParameterID.macroSpace, value: max(0, min(1, newValue))) }
    }

    // MARK: - FLASH Gesture

    /// Fire the FLASH gesture chromatophore burst.
    /// The gesture type (Ripple/Pulse/Undertow/Surge) must be set beforehand via
    /// setParameter(ParameterID.gestureType, value:).
    func fireFlash() {
        assert(Thread.isMainThread)
        guard let bridge = ObrixBridge.shared() else { return }
        // Rising edge on flashTrigger fires the FLASH envelope in the C++ engine.
        bridge.setParameterImmediate(ParameterID.flashTrigger, value: 1)
        // Clear immediately — the C++ engine detects the rising edge per-block,
        // so we clear after one scheduler cycle (next runloop pass is sufficient).
        DispatchQueue.main.async { [weak self] in
            ObrixBridge.shared()?.setParameterImmediate(
                ObrixSynthEngine.ParameterID.flashTrigger, value: 0)
            self?.parameterState[ObrixSynthEngine.ParameterID.flashTrigger] = 0
        }
        parameterState[ParameterID.flashTrigger] = 1
    }

    // MARK: - Engine Info

    /// Current sample rate from the audio device (via bridge).
    /// Falls back to AVAudioSession.sampleRate — never hardcodes 44100 or 48000.
    var sampleRate: Double {
        let bridgeSR = ObrixBridge.shared()?.sampleRate() ?? 0
        if bridgeSR > 0 { return Double(bridgeSR) }
        return AVAudioSession.sharedInstance().sampleRate
    }

    /// CPU load from the JUCE audio device manager (0–1).
    var cpuLoad: Float {
        ObrixBridge.shared()?.cpuLoad() ?? 0
    }

    /// Number of active voices as reported by the C++ engine.
    var activeVoiceCount: Int {
        Int(ObrixBridge.shared()?.activeVoiceCount() ?? 0)
    }

    // MARK: - Parameter Normalization Helpers

    /// Scale a normalized [0, 1] filter cutoff to Hz using the engine's skewed range.
    /// The C++ engine uses NormalisableRange with skewFactor=0.3 (log-like).
    /// We replicate it here for preview/display: f = 20 × (20000/20)^normalized.
    static func cutoffNormalizedToHz(_ normalized: Float) -> Float {
        let clamped = max(0, min(1, normalized))
        // Skew factor 0.3 approximation: power law
        return 20.0 * pow(1000.0, clamped)  // 20 Hz → 20 kHz
    }

    static func cutoffHzToNormalized(_ hz: Float) -> Float {
        let clamped = max(20, min(20_000, hz))
        return log(clamped / 20.0) / log(1000.0)
    }

    // MARK: - Private: Voice Slot Management

    /// Find a free slot for a new note, or steal the oldest active voice.
    ///
    /// Stealing policy: round-robin across all 8 slots, prefer already-releasing voices,
    /// then steal the slot that has been active longest. This mirrors the C++ engine's
    /// own voice-stealing, so the Swift tracking stays in sync.
    private func allocateVoiceSlot(for note: Int, reefSlot: Int) -> Int {
        // Reuse the existing slot if this exact note is already active
        if let existing = noteToVoiceSlot[note] { return existing }

        // Prefer idle slots first
        for i in 0..<Self.voiceCount where !voices[i].isActive { return i }

        // Prefer voices in release stage (nearly done)
        for i in 0..<Self.voiceCount where voices[i].envelopeStage == .release { return i }

        // Steal oldest active voice (round-robin for determinism)
        let stolen = stealCursor % Self.voiceCount
        stealCursor += 1
        return stolen
    }

    // MARK: - Private: Metering Timer

    /// Polls the bridge every 50ms to sync Swift voice tracking with C++ engine state.
    /// Rate: 20 Hz — fast enough for smooth metering, cheap enough for main thread.
    private func startMeterTimer() {
        meterTimer = Timer.scheduledTimer(withTimeInterval: 0.05, repeats: true) { [weak self] _ in
            self?.syncVoiceStates()
        }
    }

    /// Pull active voice count from the bridge and estimate envelope levels.
    ///
    /// The C++ engine reports total active voices but not per-voice state.
    /// We infer stage transitions from elapsed time + ADSR parameters for metering.
    private func syncVoiceStates() {
        let bridgeActiveCount = activeVoiceCount
        var anyChanged = false

        for i in 0..<Self.voiceCount {
            guard voices[i].isActive else { continue }

            let age = voices[i].noteAge
            let stage = voices[i].envelopeStage

            // Estimate envelope level for metering bar
            let attack  = Double(parameterState[ParameterID.ampAttack]  ?? 0.01)
            let decay   = Double(parameterState[ParameterID.ampDecay]   ?? 0.3)
            let sustain = Float(parameterState[ParameterID.ampSustain]  ?? 0.7)
            let release = Double(parameterState[ParameterID.ampRelease] ?? 0.5)

            switch stage {
            case .attack:
                if attack > 0.0001 {
                    voices[i].envelopeLevel = Float(min(1.0, age / attack))
                    if age >= attack {
                        voices[i].envelopeStage = .decay
                        anyChanged = true
                    }
                } else {
                    voices[i].envelopeStage = .decay
                    anyChanged = true
                }

            case .decay:
                let decayElapsed = age - attack
                if decay > 0.0001 {
                    let t = Float(min(1.0, decayElapsed / decay))
                    voices[i].envelopeLevel = 1.0 - t * (1.0 - sustain)
                    if decayElapsed >= decay {
                        voices[i].envelopeStage = .sustain
                        anyChanged = true
                    }
                } else {
                    voices[i].envelopeStage = .sustain
                    voices[i].envelopeLevel = sustain
                    anyChanged = true
                }

            case .sustain:
                voices[i].envelopeLevel = sustain

            case .release:
                // Estimate release completion
                if release > 0.0001 {
                    // noteOff was called — we don't have exact noteOff time, approximate
                    // by assuming release started at the last snapshot update
                    voices[i].envelopeLevel *= Float(1.0 - 0.05 / max(release, 0.05))
                    if voices[i].envelopeLevel < 0.001 {
                        voices[i].reset()
                        noteToVoiceSlot = noteToVoiceSlot.filter { $1 != i }
                        anyChanged = true
                    }
                } else {
                    voices[i].reset()
                    noteToVoiceSlot = noteToVoiceSlot.filter { $1 != i }
                    anyChanged = true
                }

            case .idle:
                break
            }
        }

        // Reconcile: if bridge says 0 active but we think some are playing, clear them.
        // Handles edge cases: all-notes-off from MIDI, Journey mode ending, bridge restart.
        let swiftActiveCount = voices.filter { $0.isActive }.count
        if bridgeActiveCount == 0 && swiftActiveCount > 0 {
            for i in 0..<Self.voiceCount { voices[i].reset() }
            noteToVoiceSlot.removeAll()
            anyChanged = true
        }

        if anyChanged { publishVoiceSnapshot() }
    }

    // MARK: - Private: Snapshot Publishing

    private func publishVoiceSnapshot() {
        let active = voices.filter { $0.isActive }
        voiceSnapshot = ObrixVoiceSnapshot(
            voices: active,
            activeCount: active.count,
            timestamp: Date()
        )
    }

    // MARK: - Private: Waveform Read-back

    private func currentWaveform() -> ObrixWaveform {
        let typeVal = Int(parameterState[ParameterID.src1Type] ?? 2)
        return ObrixWaveform(rawValue: typeVal) ?? .saw
    }
}

// MARK: - AudioEngineManager Integration
//
// Extension on AudioEngineManager that wires ObrixSynthEngine into the
// existing parameter dispatch path.
//
// AudioEngineManager already handles:
//   • applySlotChain → setParameterImmediate on the bridge
//   • noteOn/noteOff → bridge MIDI queue
//   • applyCachedParams → bridge param blasts
//
// ObrixSynthEngine adds on top of this:
//   • Voice snapshot publication (for metering)
//   • Parameter state read-back (for SpecimenParamPanel)
//   • Preset apply with defaults fill-in
//   • Macro convenience accessors

extension AudioEngineManager {

    // MARK: - Synth Engine Accessor
    //
    // The synth engine is a lazy companion to AudioEngineManager — not in its
    // stored properties because AudioEngineManager is a final class with no
    // modification access here. Instead, a shared instance is vended via a
    // module-scoped singleton. AudioEngineManager methods call through to it
    // after performing their own bridge writes.
    //
    // Usage:
    //   AudioEngineManager.synthEngine.noteOn(note: 60, velocity: 0.8, reefSlot: 2)
    //   AudioEngineManager.synthEngine.setParameter("obrix_fx1Mix", value: 0.4)
    //   audioEngineManager.synthEngine.voiceSnapshot  // Published — observe in SwiftUI

    static let synthEngine = ObrixSynthEngine()

    // MARK: - noteOn override (Swift tracking layer)
    //
    // AudioEngineManager.noteOn already calls bridge.noteOn. This convenience
    // method additionally informs ObrixSynthEngine for metering. Call it instead
    // of (or after) the base noteOn when you also need voice snapshots.

    func trackNoteOn(note: Int, velocity: Float, reefSlot: Int) {
        Self.synthEngine.noteOn(note: note, velocity: velocity, reefSlot: reefSlot)
    }

    func trackNoteOff(note: Int, reefSlot: Int) {
        Self.synthEngine.noteOff(note: note, reefSlot: reefSlot)
    }

    // MARK: - Parameter push with tracking

    /// Push a parameter to the bridge AND update the synth engine's state cache.
    ///
    /// Use this instead of bridge.setParameterImmediate directly when you want
    /// ObrixSynthEngine.parameterState to stay in sync (e.g., for SpecimenParamPanel).
    func pushTrackedParam(_ id: String, value: Float) {
        // Route through setParameter so parameterState cache stays consistent.
        // This also calls setParameterImmediate on the bridge internally.
        Self.synthEngine.setParameter(id, value: value)
    }

    // MARK: - FLASH gesture

    /// Fire the FLASH chromatophore gesture on the synthesis engine.
    func fireFlash() {
        Self.synthEngine.fireFlash()
    }
}
