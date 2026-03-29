import Foundation
import QuartzCore

/// The ambient generative engine that gives the Reef its living voice.
///
/// When the app opens, the Reef produces a soft, evolving ambient loop
/// based on the current population of specimens. It's always-on, low-level
/// background music that reflects the Reef's mood and composition.
///
/// Design: "The Reef is alive. You hear it before you see it."
final class ReefHeartbeat {

    // MARK: - Configuration

    /// Base tempo range (BPM). Sparse reefs are slow, dense reefs are faster.
    static let tempoRange: ClosedRange<Double> = 40...80

    /// Volume level for ambient playback (0-1). Should be subtle.
    var ambientVolume: Float = 0.25

    /// Whether the heartbeat is active
    private(set) var isPlaying: Bool = false

    // MARK: - State

    /// Harmonic context for ambient generation
    private let harmonic = HarmonicContext(key: 0, scale: .pentatonic, progression: .dreamFloat)

    /// Current effective BPM (derived from population density)
    private(set) var currentBPM: Double = 50

    /// Specimens registered for ambient contribution
    private var registeredSpecimens: [(subtypeID: String, role: MusicalRole, profile: VoiceProfile)] = []

    /// Which specimen index plays next (round-robin)
    private var nextSpecimenIndex: Int = 0

    /// Beat counter for chord progression advancement
    private var beatCounter: Int = 0

    /// Phrase counter (resets every 16 beats for variation)
    private var phraseCounter: Int = 0

    /// Timer for heartbeat ticks
    private var timer: Timer?

    /// Last note played per specimen (for melodic memory)
    private var lastNotes: [Int: Int] = [:]

    /// Active ambient notes (for cleanup)
    private var activeNotes: [(note: Int, offTime: TimeInterval)] = []

    /// Mood drift: slowly shifts the harmonic character over time
    private var moodDriftPhase: Float = 0

    // MARK: - Sound Memory

    /// Sound memory manager — records exposure for active specimens on each tick.
    /// Set before calling start(). When nil, memory recording is silently skipped.
    weak var soundMemoryManager: SoundMemoryManager?

    /// Full Specimen objects for the active reef population.
    /// Set alongside start(specimenSubtypeIDs:) so recordExposure can use stable UUIDs.
    /// Parallel to `specimenSubtypeIDs` by index, but extra entries are ignored gracefully.
    var activeSpecimenObjects: [Specimen] = []

    // MARK: - Callbacks

    var onNoteOn: ((Int, Float) -> Void)?   // (midiNote, velocity)
    var onNoteOff: ((Int) -> Void)?

    // MARK: - Lifecycle

    /// Start the ambient heartbeat with the current reef population
    func start(specimenSubtypeIDs: [String]) {
        stop()

        // Register specimens
        registeredSpecimens = specimenSubtypeIDs.map { id in
            (subtypeID: id,
             role: SpecimenRoleMap.role(for: id),
             profile: VoiceProfileCatalog.profile(for: id))
        }

        guard !registeredSpecimens.isEmpty else { return }

        // Calculate tempo from population density
        let density = Float(registeredSpecimens.count) / 16.0  // 16 max slots
        currentBPM = Self.tempoRange.lowerBound +
            Double(density) * (Self.tempoRange.upperBound - Self.tempoRange.lowerBound)

        // Choose scale based on specimen composition
        configureHarmony()

        // Reset state
        nextSpecimenIndex = 0
        beatCounter = 0
        phraseCounter = 0
        moodDriftPhase = 0
        lastNotes.removeAll()
        activeNotes.removeAll()
        isPlaying = true

        // Note: activeSpecimenObjects is NOT cleared here — the caller sets it before
        // calling start() and it persists across restarts triggered by updatePopulation().
        // If the caller didn't set it, memory recording is silently skipped (see tick()).

        // Start tick timer at beat resolution
        let beatInterval = 60.0 / currentBPM
        timer = Timer.scheduledTimer(withTimeInterval: beatInterval, repeats: true) { [weak self] _ in
            self?.tick()
        }
    }

    /// Stop the ambient heartbeat
    func stop() {
        timer?.invalidate()
        timer = nil
        isPlaying = false

        // Release all active notes
        for (note, _) in activeNotes {
            onNoteOff?(note)
        }
        activeNotes.removeAll()
    }

    /// Update when the reef population changes (specimen added/removed)
    func updatePopulation(specimenSubtypeIDs: [String]) {
        if isPlaying {
            start(specimenSubtypeIDs: specimenSubtypeIDs)
        }
    }

    // MARK: - Harmony Configuration

    private func configureHarmony() {
        // Count roles to determine mood
        let roles = registeredSpecimens.map { $0.role }
        let bassCount = roles.filter { $0 == .bass }.count
        let melodyCount = roles.filter { $0 == .melody }.count
        let rhythmCount = roles.filter { $0 == .rhythm }.count

        // More bass = darker, more melody = brighter, more rhythm = energetic
        if bassCount > melodyCount {
            harmonic.transitionToZone(.twilight, specimenMood: 0.3)
        } else if rhythmCount > melodyCount {
            harmonic.transitionToZone(.sunlit, specimenMood: 0.7)
        } else {
            harmonic.transitionToZone(.sunlit, specimenMood: 0.5)
        }
    }

    // MARK: - Tick

    private func tick() {
        let now = CACurrentMediaTime()

        // Release expired notes
        let expired = activeNotes.filter { $0.offTime <= now }
        for (note, _) in expired {
            onNoteOff?(note)
        }
        activeNotes.removeAll { $0.offTime <= now }

        // Advance beat
        beatCounter += 1

        // Advance chord every 8 beats (slower than Dive — ambient is patient)
        if beatCounter % 8 == 0 {
            harmonic.forceNextChord()
        }

        // Phrase variation every 16 beats
        if beatCounter % 16 == 0 {
            phraseCounter += 1
            applyMoodDrift()
        }

        // Record musical exposure for all active specimens on every tick.
        // tickInterval = one beat duration (seconds). Scale name comes from the
        // harmonic context's current scale rawValue (matches SoundMemory's expected keys).
        if let smm = soundMemoryManager, !activeSpecimenObjects.isEmpty {
            let tickInterval: TimeInterval = 60.0 / currentBPM
            let scaleName = harmonic.scale.rawValue
            let register  = ambientOctave(for: .melody)  // Use melody octave as register proxy
            for specimen in activeSpecimenObjects {
                let playAge = SpecimenAge.from(playSeconds: specimen.totalPlaySeconds)
                smm.recordExposure(
                    specimenId: specimen.id,
                    scale:      scaleName,
                    tempo:      Float(currentBPM),
                    register:   register,
                    duration:   tickInterval,
                    genre:      "ambient",
                    age:        playAge
                )
            }
        }

        // Determine if a note should play this beat
        // Ambient is SPARSE — not every beat produces a note
        guard !registeredSpecimens.isEmpty else { return }

        // Select which specimen contributes this beat
        let specimen = registeredSpecimens[nextSpecimenIndex % registeredSpecimens.count]
        nextSpecimenIndex += 1

        // Use voice profile's beat pattern to decide if this specimen fires
        let subdivisionInBar = beatCounter % 16
        let probability = specimen.profile.firingProbability(at: subdivisionInBar)

        // Reduce probability for ambient (everything is quieter and sparser)
        let ambientProbability = probability * 0.4
        guard Float.random(in: 0...1) < ambientProbability else { return }

        // Only note-generating roles produce ambient sound
        guard specimen.role.generatesNotes else { return }

        // Select note from harmonic context
        let octave = ambientOctave(for: specimen.role)
        let availableNotes = harmonic.notesForRole(specimen.role, baseOctave: octave)
        guard !availableNotes.isEmpty else { return }

        // Filter to voice profile range
        let rangedNotes = availableNotes.filter {
            $0 >= specimen.profile.rangeLow && $0 <= specimen.profile.rangeHigh
        }
        let pitches = rangedNotes.isEmpty ? availableNotes : rangedNotes

        // Note selection with memory
        let note: Int
        if let last = lastNotes[nextSpecimenIndex - 1] {
            note = selectAmbientNote(from: pitches, lastNote: last, profile: specimen.profile)
        } else {
            // First note: pick from the middle of the range (gentle entry)
            note = pitches[pitches.count / 2]
        }
        lastNotes[nextSpecimenIndex - 1] = note

        // Ambient velocity — always soft
        let baseVelocity = ambientVolume * 0.6
        let variation = Float.random(in: -0.05...0.05)
        let velocity = max(0.08, min(0.4, baseVelocity + variation))

        // Fire the note
        onNoteOn?(note, velocity)

        // Long sustain for ambient notes (2-8 beats)
        let sustainBeats = Double.random(in: 2...8)
        let sustainSeconds = sustainBeats * (60.0 / currentBPM)
        activeNotes.append((note: note, offTime: now + sustainSeconds))
    }

    // MARK: - Helpers

    private func ambientOctave(for role: MusicalRole) -> Int {
        switch role {
        case .bass:    return 2  // Very low ambient bass
        case .melody:  return 4  // Mid-high ambient melody
        case .harmony: return 3  // Mid ambient chords
        case .rhythm:  return 3  // Mid percussion (muted in ambient)
        case .texture: return 3
        case .effect:  return 3
        }
    }

    private func selectAmbientNote(from pitches: [Int], lastNote: Int, profile: VoiceProfile) -> Int {
        guard !pitches.isEmpty else { return lastNote }

        // Ambient prefers very small intervals (stepwise motion)
        var bestPitch = pitches[0]
        var bestScore: Float = .infinity

        for pitch in pitches {
            let interval = abs(pitch - lastNote)
            let profileScore = profile.intervalScore(interval)
            // Strong gravity toward small intervals for ambient
            let score = profileScore + Float(interval) * 0.5
            if score < bestScore {
                bestScore = score
                bestPitch = pitch
            }
        }

        return bestPitch
    }

    /// Slowly drift the harmonic mood over time
    private func applyMoodDrift() {
        moodDriftPhase += 0.1
        // Every ~10 phrases, consider shifting the harmonic context
        if phraseCounter % 10 == 0 {
            // Randomly shift key by a perfect fifth (circle of fifths motion)
            // This creates gentle, natural modulation
            let shouldShift = Float.random(in: 0...1) < 0.3  // 30% chance
            if shouldShift {
                harmonic.forceNextChord()  // Extra chord change for variety
            }
        }
    }
}
