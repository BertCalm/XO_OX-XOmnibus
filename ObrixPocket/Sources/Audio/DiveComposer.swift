import Foundation
import QuartzCore

/// Generative music engine for the Dive — turns player movement into music.
///
/// The Dive is an underwater obstacle-avoidance game where the player's swimming
/// controls the melody. DiveComposer provides the harmonic structure (chord progressions,
/// beat grid, phrase contours) while the player's position provides the note selection.
///
/// Design: Player X position (0-1) maps to pitch within the current chord voicing.
/// Obstacle proximity adds tension/dissonance. Specimen GameStats shape timbre and rhythm.
/// Everything snaps to a beat grid — that's what makes it music, not noise.
final class DiveComposer {

    // MARK: - Configuration

    /// Tempo in BPM — constant through a dive (could be specimen-influenced later)
    let bpm: Double = 110.0

    /// Seconds per beat
    var beatDuration: Double { 60.0 / bpm }

    /// Current beat within the phrase (0-based, wraps at phraseLength)
    private(set) var currentBeat: Int = 0

    /// Beats per phrase (4 bars of 4/4 = 16 beats)
    let phraseLength: Int = 16

    /// Current chord index within the zone's progression
    private(set) var currentChordIndex: Int = 0

    /// How degraded the sound is (0 = pristine, 1 = destroyed). Rises on hit, decays over time.
    private(set) var degradation: Float = 0.0

    /// Accumulated clean streak — beats without a hit
    private(set) var cleanStreak: Int = 0

    // MARK: - Zone Chord Progressions

    /// Each zone has a chord progression that loops. Chords are arrays of semitone intervals
    /// relative to the root (C). The progression advances every 4 beats (1 bar).
    ///
    /// These progressions are designed so that ANY note within the chord sounds consonant.
    /// The player's position selects which chord tone to play — not whether it sounds good.
    /// The structure IS the safety net.
    static let zoneProgressions: [DepthZone: [[Int]]] = [
        // Sunlit: I - V - vi - IV (C - G - Am - F) — universally pleasant, bright
        .sunlit: [
            [0, 4, 7, 12, 16],     // C major (C E G C' E')
            [7, 11, 14, 19, 23],    // G major (G B D G' B')
            [9, 12, 16, 21, 24],    // A minor (A C E A' C')
            [5, 9, 12, 17, 21],     // F major (F A C F' A')
        ],
        // Twilight: i - VII - VI - V (Cm - Bb - Ab - G) — cinematic, descending
        .twilight: [
            [0, 3, 7, 12, 15],     // C minor (C Eb G C' Eb')
            [10, 14, 17, 22, 26],   // Bb major (Bb D F Bb' D')
            [8, 12, 15, 20, 24],    // Ab major (Ab C Eb Ab' C')
            [7, 11, 14, 19, 23],    // G major (G B D G' B')
        ],
        // Midnight: chromatic tension — diminished, augmented, tritones
        .midnight: [
            [0, 3, 6, 9, 12],      // C diminished (C Eb Gb A C')
            [1, 5, 8, 11, 13],     // Db aug cluster (Db F Ab B Db')
            [2, 5, 8, 11, 14],     // D half-dim (D F Ab B D')
            [0, 4, 7, 10, 12],     // C7 (C E G Bb C') — resolves tension slightly
        ],
        // Abyssal: open fifths and drones — vast, empty, low
        .abyssal: [
            [0, 7, 12, 19, 24],    // C power chord stack (C G C' G' C'')
            [5, 12, 17, 24, 29],   // F power chord stack (F C' F' C'' F'')
            [0, 7, 12, 19, 24],    // C again — minimal movement
            [3, 10, 15, 22, 27],   // Eb power chord stack (Eb Bb Eb' Bb' Eb'')
        ],
    ]

    // MARK: - Rhythm Patterns

    /// Subdivision patterns per zone. true = note fires on this subdivision.
    /// Each beat is divided into 4 subdivisions (16th notes).
    /// The pattern repeats every beat.
    static let zoneRhythms: [DepthZone: [Bool]] = [
        // Sunlit: quarter notes + occasional eighth (simple, bright)
        .sunlit:   [true, false, false, false],         // 1 note per beat
        // Twilight: eighth notes with syncopation
        .twilight: [true, false, true, false],           // 2 notes per beat
        // Midnight: sixteenth note density (frantic)
        .midnight: [true, true, true, false],            // 3 notes per beat
        // Abyssal: whole notes (sparse, haunting)
        .abyssal:  [true, false, false, false],          // 1 note per beat, but very long
    ]

    /// Note duration multiplier per zone (in beats)
    static let zoneNoteDuration: [DepthZone: ClosedRange<Double>] = [
        .sunlit:   0.3...0.8,    // Short to medium
        .twilight: 0.5...1.2,    // Medium
        .midnight: 0.1...0.3,    // Very short (staccato bursts)
        .abyssal:  2.0...4.0,    // Very long (drones)
    ]

    /// Base octave per zone (deeper = lower)
    static let zoneBaseOctave: [DepthZone: Int] = [
        .sunlit:   4,   // C4 range
        .twilight: 3,   // C3 range
        .midnight: 3,   // C3 range (but chromatic = wild)
        .abyssal:  2,   // C2 range (deep drones)
    ]

    /// Tension intervals per zone — used when obstacles are near.
    /// Instead of random chromatic neighbors, these intervals are musically coherent
    /// within each zone's harmonic language.
    static let zoneTensionIntervals: [DepthZone: [Int]] = [
        .sunlit:   [2, -2, 5],     // Major 2nd, down major 2nd, perfect 4th (bright suspension)
        .twilight: [1, -2, 3],     // Minor 2nd up, whole step down, minor 3rd (dark but scalar)
        .midnight: [1, -1, 6],     // Chromatic neighbors + tritone (dissonance IS the zone)
        .abyssal:  [5, 7, -5],     // Perfect 4th/5th (open, vast intervals for drones)
    ]

    // MARK: - State

    private var subdivisionCount: Int = 0   // Global subdivision counter
    private var tickToken: TickToken?
    private var zone: DepthZone = .sunlit
    private var lastNoteOnBeat: Int = -1
    private var activeNotes: [(note: Int, offTime: TimeInterval)] = []
    private var startTime: CFTimeInterval?
    private var lastNote: Int? = nil

    // Specimen influence (set before dive starts)
    var specimenPulse: Float = 0.5      // LFO rate → rhythm density modifier
    var specimenWarmth: Float = 0.5     // Cutoff → prefer lower/higher chord tones
    var specimenReflexes: Float = 0.5   // Attack → note duration modifier
    var specimenGrit: Float = 0.3       // Drive → velocity intensity
    var specimenComplexity: Float = 0.3  // Src type → chord enrichment + phrase contour
    var specimenPresence: Float = 0.3    // Unison detune → octave doubling probability
    var specimenStamina: Float = 0.5     // Decay+Release → note sustain extension

    // Player input (updated every frame by DiveScene)
    var playerX: Float = 0.5            // 0 = far left, 1 = far right
    var obstacleProximity: Float = 0.0  // 0 = no obstacle near, 1 = touching
    var playerDodgeVelocity: Float = 0.0 // How fast the player is moving laterally

    // MARK: - Callbacks

    var onNoteOn: ((Int, Float) -> Void)?   // (midiNote, velocity)
    var onNoteOff: ((Int) -> Void)?
    var onBeat: ((Int) -> Void)?            // Fires on each beat (for haptics/visual sync)
    var onChordChange: (() -> Void)?        // Fires when chord advances
    var onDegradation: ((Float) -> Void)?   // Current degradation level changed

    // MARK: - Lifecycle

    /// Start the composition engine. Call once when the dive begins.
    func start(zone: DepthZone) {
        self.zone = zone
        self.startTime = CACurrentMediaTime()
        self.currentBeat = 0
        self.currentChordIndex = 0
        self.subdivisionCount = 0
        self.degradation = 0.0
        self.cleanStreak = 0
        self.activeNotes = []
        self.lastNote = nil

        // Tick at 16th-note resolution: 4 subdivisions per beat
        // At 110 BPM: beat = 545ms, subdivision = 136ms → ~7.3 Hz
        // Use TickScheduler at 15Hz (close enough, latency < 1 subdivision)
        // Actually, compute exact: subdivisions per second = (bpm / 60) * 4
        let subsPerSecond = (bpm / 60.0) * 4.0
        let hz = max(10, min(60, Int(subsPerSecond.rounded(.up))))

        tickToken = TickScheduler.shared.register(hz: hz) { [weak self] in
            self?.tick()
        }
    }

    /// Stop the composition engine. Call when the dive ends.
    func stop() {
        tickToken = nil
        // Release all active notes
        for (note, _) in activeNotes {
            onNoteOff?(note)
        }
        activeNotes.removeAll()
        lastNote = nil
    }

    /// Update the current zone (called by DiveTab when depth changes)
    func updateZone(_ newZone: DepthZone) {
        if newZone != zone {
            zone = newZone
            // Reset chord progression for new zone
            currentChordIndex = 0
        }
    }

    /// Register a hit — degrades musical fidelity
    func registerHit() {
        degradation = min(1.0, degradation + 0.25)
        cleanStreak = 0
        onDegradation?(degradation)
    }

    /// Register collecting an orb — triggers chord change + filter bloom
    func registerCollectible() {
        // Force a chord change on next beat
        // advanceChord() already calls onChordChange?() internally — do not call again
        advanceChord()

        // Filter bloom: open up
        degradation = max(0, degradation - 0.15)
        onDegradation?(degradation)
    }

    // MARK: - Tick (fires at subdivision resolution)

    private func tick() {
        guard let start = startTime else { return }
        let elapsed = CACurrentMediaTime() - start

        // Release expired notes
        releaseExpiredNotes(at: elapsed)

        // Determine if this tick aligns with a subdivision
        let subdivisionDuration = beatDuration / 4.0
        let expectedSub = Int(elapsed / subdivisionDuration)

        guard expectedSub > subdivisionCount else { return }
        subdivisionCount = expectedSub

        // Which subdivision within the beat? (0-3)
        let subInBeat = subdivisionCount % 4

        // Beat boundary detection
        if subInBeat == 0 {
            let newBeat = (subdivisionCount / 4) % phraseLength
            if newBeat != currentBeat {
                currentBeat = newBeat
                onBeat?(currentBeat)

                // Advance chord every 4 beats (1 bar)
                if currentBeat % 4 == 0 {
                    advanceChord()
                }

                // Decay degradation on clean beats
                if degradation > 0 {
                    cleanStreak += 1
                    if cleanStreak >= 4 {
                        degradation = max(0, degradation - 0.05)
                        onDegradation?(degradation)
                    }
                }
            }
        }

        // Should a note fire on this subdivision?
        let pattern = Self.zoneRhythms[zone] ?? [true, false, false, false]

        // Specimen Pulse modifies rhythm: high pulse adds extra subdivisions
        var shouldFire = pattern[subInBeat % pattern.count]
        if !shouldFire && specimenPulse > 0.7 {
            // High-pulse specimens add syncopated 16ths
            shouldFire = subInBeat == 2 && (subdivisionCount % 8 < 4)
        }

        guard shouldFire else { return }

        // Select note with melodic coherence — specimen stats shape the harmonic language
        let chord = currentChord()
        let octave = Self.zoneBaseOctave[zone] ?? 3
        var pitches = chord.map { max(24, min(108, octave * 12 + $0)) }

        // Complexity enrichment: high-complexity specimens hear passing tones
        if specimenComplexity > 0.5 {
            let passing = pitches.flatMap { [$0 + 2, $0 - 2] }
                .filter { p in p >= 24 && p <= 108 && !pitches.contains(p) }
            pitches += passing
            pitches.sort()
        }

        // Phrase contour: beats 0-7 tend upward, 8-15 tend downward
        let phrasePos = Float(currentBeat) / Float(phraseLength)
        let contourBias = phrasePos < 0.5 ? Float(0.15) : Float(-0.15)
        let direction = playerX - 0.5 + contourBias * specimenComplexity

        // Note Memory: prefer notes close to the last played note
        let clampedNote: Int
        if let last = lastNote {
            clampedNote = selectCoherentNote(from: pitches, lastNote: last, direction: direction)
        } else {
            let noteIndex = noteIndexFromPlayerPosition(chordSize: pitches.count)
            clampedNote = pitches[max(0, min(pitches.count - 1, noteIndex))]
        }

        // Velocity: base from zone + dodge velocity adds accent + degradation reduces
        var velocity: Float = 0.5 + specimenGrit * 0.3
        velocity += abs(playerDodgeVelocity) * 0.3      // Fast dodging = louder accent
        velocity *= (1.0 - degradation * 0.5)            // Degradation quiets
        velocity = max(0.15, min(0.95, velocity))

        // Obstacle proximity adds zone-appropriate tension intervals
        var finalNote = clampedNote
        if obstacleProximity > 0.6 {
            let intervals = Self.zoneTensionIntervals[zone] ?? [1, -1]
            finalNote += intervals.randomElement() ?? 1
        }

        // Fire the note
        onNoteOn?(finalNote, velocity)
        lastNote = finalNote

        // Schedule note-off
        let durationRange = Self.zoneNoteDuration[zone] ?? 0.3...0.8
        var duration = Double.random(in: durationRange) * beatDuration
        // Specimen reflexes: high reflexes = shorter notes (staccato)
        duration *= Double(1.2 - specimenReflexes * 0.6)
        duration = max(0.05, duration)
        // Stamina extends sustain (legato tendency)
        duration *= Double(1.0 + specimenStamina * 0.4)

        activeNotes.append((note: finalNote, offTime: elapsed + duration))

        // Presence → octave doubling (wide, big sound)
        if specimenPresence > 0.5 && Float.random(in: 0...1) < (specimenPresence - 0.5) * 0.6 {
            let doubled = finalNote + (Bool.random() ? 12 : -12)
            if doubled >= 24 && doubled <= 108 {
                onNoteOn?(doubled, velocity * 0.55)
                activeNotes.append((note: doubled, offTime: elapsed + duration))
            }
        }
    }

    // MARK: - Helpers

    private func currentChord() -> [Int] {
        let progression = Self.zoneProgressions[zone] ?? [[0, 4, 7, 12, 16]]
        return progression[currentChordIndex % progression.count]
    }

    private func advanceChord() {
        let progression = Self.zoneProgressions[zone] ?? [[0, 4, 7, 12, 16]]
        currentChordIndex = (currentChordIndex + 1) % progression.count
        onChordChange?()
    }

    /// Select a note that's melodically coherent with the previous note.
    /// Prefers small intervals while respecting player direction + phrase contour.
    private func selectCoherentNote(from pitches: [Int], lastNote: Int, direction: Float) -> Int {
        guard !pitches.isEmpty else { return lastNote }

        var bestPitch = pitches[0]
        var bestScore: Float = .infinity

        for pitch in pitches {
            let interval = Float(abs(pitch - lastNote))

            // Direction penalty: penalize notes opposite to player steering
            let dirPenalty: Float
            if direction > 0.1 && pitch < lastNote {
                dirPenalty = interval * 0.4
            } else if direction < -0.1 && pitch > lastNote {
                dirPenalty = interval * 0.4
            } else {
                dirPenalty = 0
            }

            // Memory gravity: warmth controls preference for small intervals
            let gravity = 0.3 + specimenWarmth * 0.7
            let score = interval * gravity + dirPenalty

            if score < bestScore {
                bestScore = score
                bestPitch = pitch
            }
        }

        return bestPitch
    }

    /// Map player X position (0-1) to a chord tone index.
    /// Left = lower tones, right = higher tones.
    /// Specimen warmth biases toward lower (warm) or higher (bright) tones.
    private func noteIndexFromPlayerPosition(chordSize: Int) -> Int {
        guard chordSize > 0 else { return 0 }

        // Bias the position by specimen warmth (high warmth pulls toward lower tones)
        var biasedX = playerX
        biasedX += (0.5 - specimenWarmth) * 0.3  // Warm specimens shift left (lower)
        biasedX = max(0, min(1, biasedX))

        let index = Int(biasedX * Float(chordSize - 1))
        return max(0, min(chordSize - 1, index))
    }

    private func releaseExpiredNotes(at currentTime: TimeInterval) {
        let expired = activeNotes.filter { $0.offTime <= currentTime }
        for (note, _) in expired {
            onNoteOff?(note)
        }
        activeNotes.removeAll { $0.offTime <= currentTime }
    }
}
