import Foundation
import QuartzCore

/// A performance event generated when a player interacts with the reef grid.
/// Richer than a raw MIDI note — carries specimen context and harmonic awareness.
struct ReefPerformanceEvent {
    let slotIndex: Int
    let midiNote: Int
    let velocity: Float
    let role: MusicalRole
    let specimenName: String
    let isChordTone: Bool       // Was this note from the current chord?
    let timestamp: TimeInterval
}

/// Transforms the reef grid from a static display into a playable performance surface.
///
/// When the player taps a specimen on the reef, ReefPerformanceSurface:
/// 1. Looks up the specimen's MusicalRole and VoiceProfile
/// 2. Selects an appropriate note from HarmonicContext based on the role
/// 3. Applies velocity scaling from the VoiceProfile
/// 4. Fires the note through the audio engine
///
/// This enables "playing the reef" — each specimen responds musically to touch
/// based on its identity. A bass specimen plays low root notes. A melody specimen
/// plays high arpeggiated phrases. A rhythm specimen fires short staccato hits.
///
/// Design (Phantom Circuit): "Each pad IS a creature, and pressing it makes
/// that creature sing. Can I play this with my eyes closed? Now yes."
final class ReefPerformanceSurface {

    // MARK: - Configuration

    /// Harmonic context shared with the Dive (or standalone)
    var harmonicContext: HarmonicContext

    /// Specimen data per slot: (subtypeID, role, profile)
    private var slotData: [Int: SlotPerformanceData] = [:]

    /// Last note played per slot (for melodic coherence across taps)
    private var lastNotePerSlot: [Int: Int] = [:]

    /// Active notes per slot (for note-off tracking)
    private var activeNotesPerSlot: [Int: [Int]] = [:]

    /// Performance event log (for recording)
    private(set) var eventLog: [ReefPerformanceEvent] = []

    /// Whether to log events (enable for recording mode)
    var isRecording: Bool = false

    // MARK: - Callbacks

    var onNoteOn: ((Int, Int, Float) -> Void)?   // (slotIndex, midiNote, velocity)
    var onNoteOff: ((Int, Int) -> Void)?          // (slotIndex, midiNote)
    var onPerformanceEvent: ((ReefPerformanceEvent) -> Void)?

    // MARK: - Init

    init(harmonicContext: HarmonicContext = HarmonicContext()) {
        self.harmonicContext = harmonicContext
    }

    // MARK: - Setup

    /// Register a specimen in a reef slot for performance
    func registerSlot(_ slotIndex: Int, subtypeID: String) {
        let role = SpecimenRoleMap.role(for: subtypeID)
        let profile = VoiceProfileCatalog.profile(for: subtypeID)
        slotData[slotIndex] = SlotPerformanceData(
            subtypeID: subtypeID,
            role: role,
            profile: profile
        )
        lastNotePerSlot[slotIndex] = nil
        activeNotesPerSlot[slotIndex] = []
    }

    /// Clear all slot registrations
    func clearSlots() {
        slotData.removeAll()
        lastNotePerSlot.removeAll()
        activeNotesPerSlot.removeAll()
    }

    // MARK: - Performance Input

    /// Player tapped a reef slot. Generates a musically appropriate note.
    /// - Parameters:
    ///   - slotIndex: Which reef slot was tapped (0-15)
    ///   - tapForce: Force of the tap (0-1), used for velocity
    ///   - tapX: Horizontal position within the slot (0-1), used for note selection in melody roles
    func tap(slotIndex: Int, tapForce: Float = 0.5, tapX: Float = 0.5) {
        guard let data = slotData[slotIndex] else { return }

        // Release any currently active notes on this slot
        releaseSlot(slotIndex)

        // Get appropriate notes for this specimen's role
        let baseOctave = octaveForRole(data.role)
        let availableNotes = harmonicContext.notesForRole(data.role, baseOctave: baseOctave)

        guard !availableNotes.isEmpty else { return }

        // Filter to voice profile's range
        let rangedNotes = availableNotes.filter {
            $0 >= data.profile.rangeLow && $0 <= data.profile.rangeHigh
        }
        let pitches = rangedNotes.isEmpty ? availableNotes : rangedNotes

        // Select note based on role behavior
        let selectedNote: Int
        switch data.role {
        case .bass:
            // Bass: always play the root (lowest available note)
            selectedNote = pitches.min() ?? pitches[0]

        case .melody:
            // Melody: use tapX position to select from the scale
            if let lastNote = lastNotePerSlot[slotIndex] {
                // Melodic coherence: prefer small intervals from last note
                let direction = tapX - 0.5
                selectedNote = selectMelodicNote(from: pitches, lastNote: lastNote,
                                                  direction: direction, profile: data.profile)
            } else {
                let index = Int(tapX * Float(pitches.count - 1))
                selectedNote = pitches[max(0, min(pitches.count - 1, index))]
            }

        case .harmony:
            // Harmony: play a chord (multiple notes simultaneously)
            let chordNotes = Array(pitches.prefix(min(3, pitches.count)))
            for (i, note) in chordNotes.enumerated() {
                let chordVelocity = tapForce * (1.0 - Float(i) * 0.15) // Slight velocity cascade
                fireNote(slotIndex: slotIndex, note: note, velocity: chordVelocity,
                         data: data, isChordTone: true)
            }
            lastNotePerSlot[slotIndex] = chordNotes.first
            return  // Already fired notes

        case .rhythm:
            // Rhythm: always the root, short and punchy
            selectedNote = pitches[0]

        case .texture, .effect:
            // These roles trigger parameter changes, not notes
            // For performance surface, play a quiet pad note
            selectedNote = pitches[pitches.count / 2]  // Middle of range
        }

        // Calculate velocity from tap force + voice profile
        var velocity = tapForce
        let velRange = data.role.velocityRange
        velocity = velRange.lowerBound + velocity * (velRange.upperBound - velRange.lowerBound)

        // Voice profile attack sharpness affects velocity curve
        velocity *= (0.7 + data.profile.attackSharpness * 0.3)
        velocity = max(0.15, min(0.95, velocity))

        // Fire the note
        fireNote(slotIndex: slotIndex, note: selectedNote, velocity: velocity,
                 data: data, isChordTone: true)
        lastNotePerSlot[slotIndex] = selectedNote
    }

    /// Player is holding a reef slot (sustain behavior)
    func holdStart(slotIndex: Int) {
        // Holding extends the current note — don't release on holdEnd,
        // just let it ring until the next tap
    }

    /// Player released a reef slot
    func holdEnd(slotIndex: Int) {
        guard let data = slotData[slotIndex] else { return }

        // Short-sustain roles release immediately
        if data.profile.sustainLength < 0.3 {
            releaseSlot(slotIndex)
        }
        // Long-sustain roles keep ringing (released on next tap)
    }

    /// Release all active notes on a slot
    func releaseSlot(_ slotIndex: Int) {
        if let notes = activeNotesPerSlot[slotIndex] {
            for note in notes {
                onNoteOff?(slotIndex, note)
            }
        }
        activeNotesPerSlot[slotIndex] = []
    }

    /// Release all active notes on all slots
    func releaseAll() {
        for slot in activeNotesPerSlot.keys {
            releaseSlot(slot)
        }
    }

    // MARK: - Recording

    /// Clear the event log
    func clearEventLog() {
        eventLog.removeAll()
    }

    /// Get events within a time range
    func events(from start: TimeInterval, to end: TimeInterval) -> [ReefPerformanceEvent] {
        eventLog.filter { $0.timestamp >= start && $0.timestamp <= end }
    }

    // MARK: - Helpers

    private func fireNote(slotIndex: Int, note: Int, velocity: Float,
                          data: SlotPerformanceData, isChordTone: Bool) {
        onNoteOn?(slotIndex, note, velocity)

        var slotNotes = activeNotesPerSlot[slotIndex] ?? []
        slotNotes.append(note)
        activeNotesPerSlot[slotIndex] = slotNotes

        let event = ReefPerformanceEvent(
            slotIndex: slotIndex,
            midiNote: note,
            velocity: velocity,
            role: data.role,
            specimenName: data.profile.specimenName,
            isChordTone: isChordTone,
            timestamp: CACurrentMediaTime()
        )

        onPerformanceEvent?(event)

        if isRecording {
            eventLog.append(event)
        }

        // Auto-release for short-sustain voices
        if data.profile.sustainLength < 0.2 {
            let releaseDelay = Double(0.05 + data.profile.sustainLength * 0.3)
            DispatchQueue.main.asyncAfter(deadline: .now() + releaseDelay) { [weak self] in
                self?.onNoteOff?(slotIndex, note)
                self?.activeNotesPerSlot[slotIndex]?.removeAll { $0 == note }
            }
        }
    }

    private func selectMelodicNote(from pitches: [Int], lastNote: Int,
                                    direction: Float, profile: VoiceProfile) -> Int {
        guard !pitches.isEmpty else { return lastNote }

        var bestPitch = pitches[0]
        var bestScore: Float = .infinity

        for pitch in pitches {
            let interval = abs(pitch - lastNote)
            let profileScore = profile.intervalScore(interval)

            let dirPenalty: Float
            if direction > 0.1 && pitch < lastNote {
                dirPenalty = Float(interval) * 0.3
            } else if direction < -0.1 && pitch > lastNote {
                dirPenalty = Float(interval) * 0.3
            } else {
                dirPenalty = 0
            }

            let score = profileScore + Float(interval) * 0.2 + dirPenalty
            if score < bestScore {
                bestScore = score
                bestPitch = pitch
            }
        }

        return bestPitch
    }

    private func octaveForRole(_ role: MusicalRole) -> Int {
        switch role {
        case .bass:    return 2
        case .melody:  return 4
        case .harmony: return 3
        case .rhythm:  return 3
        case .texture: return 3
        case .effect:  return 3
        }
    }
}

// MARK: - Supporting Types

private struct SlotPerformanceData {
    let subtypeID: String
    let role: MusicalRole
    let profile: VoiceProfile
}
