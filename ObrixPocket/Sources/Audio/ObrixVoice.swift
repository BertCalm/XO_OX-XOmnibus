import Foundation

// MARK: - ObrixVoice
//
// Swift-side representation of a single synthesis voice.
//
// Architecture note: actual DSP (PolyBLEP oscillators, CytomicSVF filters, ADSR,
// LFO, reverb/chorus/delay) runs in the C++ ObrixEngine (ObrixEngine.h, 8 voices).
// This struct mirrors the observable state of one C++ voice for Swift consumers:
// the Dive system (VoiceProfile-driven note scheduling), the metering bar
// (ReefIndicatorBar), and performance recording (PerformanceRecorder).
//
// No memory allocation, no locks — all fields are value types.
// Thread safety: mutated only on the main thread via ObrixSynthEngine.

// MARK: - Envelope Stage
enum ObrixEnvelopeStage: UInt8, CustomStringConvertible {
    case idle    = 0  // Silent — not active
    case attack  = 1  // Rising amplitude
    case decay   = 2  // Falling toward sustain
    case sustain = 3  // Held at sustain level
    case release = 4  // Falling to zero after note-off

    var description: String {
        switch self {
        case .idle:    return "idle"
        case .attack:  return "attack"
        case .decay:   return "decay"
        case .sustain: return "sustain"
        case .release: return "release"
        }
    }

    /// True when the voice is generating audio (attack through release).
    var isAudible: Bool { self != .idle }
}

// MARK: - Waveform Type
/// Maps to ObrixEngine's obrix_src1Type parameter (0–8).
/// Used by the Microscope view and DiveComposer for timbral labelling.
enum ObrixWaveform: Int, Codable, CaseIterable {
    case off       = 0
    case sine      = 1
    case saw       = 2
    case square    = 3
    case triangle  = 4
    case noise     = 5
    case wavetable = 6
    case pulse     = 7
    case driftwood = 8  // LoFiSaw

    var displayName: String {
        switch self {
        case .off:       return "Off"
        case .sine:      return "Sine"
        case .saw:       return "Saw"
        case .square:    return "Square"
        case .triangle:  return "Triangle"
        case .noise:     return "Noise"
        case .wavetable: return "Wavetable"
        case .pulse:     return "Pulse"
        case .driftwood: return "Driftwood"
        }
    }
}

// MARK: - ObrixVoice
/// Observable state of one synthesis voice.
///
/// - slot: Index into the C++ ObrixEngine voice array (0–7).
/// - note: MIDI note number (0–127). -1 when idle.
/// - velocity: Note-on velocity (0–1). 0 when idle.
/// - envelopeStage: Current ADSR stage — polled at metering rate (not audio rate).
/// - envelopeLevel: Estimated output amplitude (0–1) for the metering bar.
///   Derived from stage + elapsed time; approximate, not DSP-accurate.
/// - waveform: Active source waveform type (mirrors obrix_src1Type).
/// - startTime: When this note was triggered — used for duration XP and Dive scoring.
/// - slotIndex: Which reef slot triggered this voice (for XP routing back to the
///   correct specimen).
struct ObrixVoice {

    // MARK: - Identity
    let slot: Int           // C++ engine voice index (0–7)

    // MARK: - Note State
    var note: Int = -1
    var velocity: Float = 0.0
    var isActive: Bool = false

    // MARK: - Envelope (approximate — metering use only)
    var envelopeStage: ObrixEnvelopeStage = .idle
    var envelopeLevel: Float = 0.0  // 0–1

    // MARK: - Timbre
    var waveform: ObrixWaveform = .saw

    // MARK: - Timing
    var startTime: Date = Date()

    // MARK: - Reef Context
    /// Which reef slot triggered this voice (for XP routing).
    var reefSlotIndex: Int = -1

    // MARK: - Convenience

    /// Elapsed time since note-on.
    var noteAge: TimeInterval { Date().timeIntervalSince(startTime) }

    /// True when the voice has been playing long enough to be considered "sustained"
    /// (> 2 seconds — matches AudioEngineManager's XP sustain threshold).
    var isSustained: Bool { isActive && noteAge > 2.0 }

    /// Reset to idle state.
    mutating func reset() {
        note = -1
        velocity = 0.0
        isActive = false
        envelopeStage = .idle
        envelopeLevel = 0.0
        reefSlotIndex = -1
    }
}

// MARK: - ObrixVoiceSnapshot
/// Lightweight copy of all active voice states for metering and UI.
/// Published by ObrixSynthEngine — safe to read on the main thread.
struct ObrixVoiceSnapshot {
    let voices: [ObrixVoice]
    let activeCount: Int
    let timestamp: Date

    /// True if any voice is currently sounding.
    var isAnySounding: Bool { activeCount > 0 }

    /// Estimated polyphony load as a 0–1 fraction.
    /// 6 concurrent voices = 1.0 (design limit for real-time safety on iPhone).
    var polyphonyLoad: Float {
        Float(activeCount) / Float(ObrixSynthEngine.voiceCount)
    }
}
