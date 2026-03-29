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

    // Sprint 61: Harmonic engine
    private(set) var harmonicContext: HarmonicContext = HarmonicContext()

    /// Musical roles for active specimens in this Dive (set before start)
    var specimenRoles: [MusicalRole] = [.melody]  // Default: single melody

    /// Which role index is currently "active" for note generation.
    /// Rotates each subdivision to give each voice its turn.
    private var activeRoleIndex: Int = 0

    // Sprint 62: Voice profiles for active specimens
    var specimenProfiles: [String: VoiceProfile] = [:]  // subtypeID → profile

    // Sprint 63: Arrangement engine
    let arrangement = ArrangementEngine()

    // Sprint 64: Player expression
    let expression = PlayerExpressionHandler()

    // Sprint 65: Scoring
    let scoring = DiveScoring()

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

    // MARK: - Configuration

    /// Configure the Dive with the player's reef specimens.
    /// Call before start(). Reads specimens and sets up roles, profiles, and arrangement.
    func configure(specimenSubtypeIDs: [String]) {
        // Map specimens to roles
        specimenRoles = specimenSubtypeIDs.map { SpecimenRoleMap.role(for: $0) }

        // Load voice profiles
        specimenProfiles = [:]
        for id in specimenSubtypeIDs {
            specimenProfiles[id] = VoiceProfileCatalog.profile(for: id)
        }

        // Configure arrangement with the roles
        arrangement.configure(roles: specimenRoles)

        // Set specimen names for scoring
        scoring.setSpecimens(specimenSubtypeIDs.compactMap { id in
            specimenProfiles[id]?.specimenName
        })
    }

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

        // Initialize harmonic context for this zone
        let avgMood = (specimenWarmth + (1.0 - specimenGrit)) / 2.0
        harmonicContext.transitionToZone(zone, specimenMood: avgMood)
        activeRoleIndex = 0

        // Start subsystems
        arrangement.reset()
        arrangement.onSectionChange = { [weak self] section in
            self?.scoring.registerSectionChange(section)
        }
        arrangement.onVoiceEnter = { [weak self] _ in
            self?.scoring.registerVoiceEntry()
        }
        expression.startTracking()
        scoring.reset()

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
        expression.stopTracking()
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
            currentChordIndex = 0
            let avgMood = (specimenWarmth + (1.0 - specimenGrit)) / 2.0
            harmonicContext.transitionToZone(newZone, specimenMood: avgMood)
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
        harmonicContext.forceNextChord()
        currentChordIndex = harmonicContext.chordIndex
        onChordChange?()

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
                if currentBeat % 4 == 0 && arrangement.currentSection.allowsChordChanges {
                    let chordChanged = harmonicContext.advanceBeat()
                    if chordChanged {
                        currentChordIndex = harmonicContext.chordIndex
                        onChordChange?()
                    }
                } else {
                    harmonicContext.advanceBeat()
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

        // ═══════════════════════════════════════════════════════
        // INTEGRATED NOTE GENERATION (Sprints 61-65)
        // ═══════════════════════════════════════════════════════

        // Sprint 63: Update arrangement state
        let currentActiveRoles = arrangement.update(elapsed: elapsed)
        scoring.updateElapsed(elapsed)
        scoring.updateActiveVoices(currentActiveRoles.count)

        // Filter to note-generating roles that are active in the arrangement
        let noteRoles = currentActiveRoles.filter { $0.generatesNotes }
        guard !noteRoles.isEmpty else { return }

        // Rotate through active roles each subdivision
        activeRoleIndex = subdivisionCount % noteRoles.count
        let currentRole = noteRoles[activeRoleIndex]

        // Sprint 62: Use voice profile for firing decision
        // Find a profile that matches this role (use first matching specimen)
        let matchingProfile = specimenProfiles.values.first { profile in
            SpecimenRoleMap.role(for: specimenProfiles.first(where: { $0.value.specimenName == profile.specimenName })?.key ?? "") == currentRole
        }

        // Check firing probability from voice profile (or fall back to role density)
        let subdivisionInBar = subdivisionCount % 16
        if let profile = matchingProfile {
            let probability = profile.firingProbability(at: subdivisionInBar)
            guard Float.random(in: 0...1) < probability else { return }
        } else {
            let roleDensityRoll = Float.random(in: 0...1)
            guard roleDensityRoll < currentRole.rhythmDensity else { return }
        }

        // Sprint 61: Get harmonically appropriate notes for this role
        let octave = Self.zoneBaseOctave[zone] ?? 3
        let availableNotes = harmonicContext.notesForRole(currentRole, baseOctave: octave)
        guard !availableNotes.isEmpty else { return }

        // Complexity enrichment: passing tones for melody
        var pitches = availableNotes
        if specimenComplexity > 0.5 && currentRole == .melody {
            let passing = pitches.flatMap { [$0 + 2, $0 - 2] }
                .filter { p in p >= 24 && p <= 108 && !pitches.contains(p) }
            pitches += passing
            pitches.sort()
        }

        // Clamp to voice profile range if available
        if let profile = matchingProfile {
            pitches = pitches.filter { $0 >= profile.rangeLow && $0 <= profile.rangeHigh }
            guard !pitches.isEmpty else { return }
        }

        // Phrase contour + player direction
        let phrasePos = Float(currentBeat) / Float(phraseLength)
        let contourBias = phrasePos < 0.5 ? Float(0.15) : Float(-0.15)
        let direction = playerX - 0.5 + contourBias * specimenComplexity

        // Note selection with melodic memory + voice profile interval preference
        let selectedNote: Int
        if let last = lastNote {
            // Sprint 62: Bias toward voice profile's preferred intervals
            if let profile = matchingProfile {
                selectedNote = selectProfileAwareNote(from: pitches, lastNote: last,
                                                       direction: direction, profile: profile)
            } else {
                selectedNote = selectCoherentNote(from: pitches, lastNote: last, direction: direction)
            }
        } else {
            let noteIndex = noteIndexFromPlayerPosition(chordSize: pitches.count)
            selectedNote = pitches[max(0, min(pitches.count - 1, noteIndex))]
        }

        // Velocity: role range + arrangement + expression + specimen influence
        let velRange = currentRole.velocityRange
        var velocity = Float.random(in: velRange)
        velocity *= arrangement.currentVelocityMultiplier         // Sprint 63
        velocity *= expression.velocityModifier                    // Sprint 64
        velocity += specimenGrit * 0.1
        velocity += abs(playerDodgeVelocity) * 0.15
        velocity *= (1.0 - degradation * 0.4)
        // Sprint 62: Voice profile accent
        if let profile = matchingProfile, profile.isAccented(subdivisionInBar) {
            velocity *= 1.2
        }
        velocity = max(0.15, min(0.95, velocity))

        // Tension from obstacle proximity
        var finalNote = selectedNote
        var wasTension = false
        if obstacleProximity > 0.6 {
            finalNote = harmonicContext.tensionNote(near: selectedNote, intensity: obstacleProximity)
            wasTension = true
        }

        // Sprint 64: Call-and-response — if player tapped a zone, accent it
        if let _ = expression.state.tappedZoneIndex {
            velocity = min(0.95, velocity + 0.2)
            scoring.registerInteraction(type: "tap")
        }

        // Fire the note
        onNoteOn?(finalNote, velocity)
        lastNote = finalNote

        // Sprint 65: Register with scoring
        scoring.registerNote(midiNote: finalNote, wasInKey: !wasTension, wasTension: wasTension)

        // Duration: role + profile + arrangement + expression + specimen stats
        let durationRange = Self.zoneNoteDuration[zone] ?? 0.3...0.8
        var duration = Double.random(in: durationRange) * beatDuration
        duration *= Double(currentRole.durationMultiplier)
        duration *= Double(expression.durationModifier)            // Sprint 64
        if let profile = matchingProfile {
            duration *= Double(0.5 + profile.sustainLength)        // Sprint 62
        }
        duration *= Double(1.2 - specimenReflexes * 0.6)
        duration = max(0.05, duration)
        duration *= Double(1.0 + specimenStamina * 0.4)

        activeNotes.append((note: finalNote, offTime: elapsed + duration))

        // Presence → octave doubling
        if specimenPresence > 0.5 && Float.random(in: 0...1) < (specimenPresence - 0.5) * 0.6 {
            let doubled = finalNote + (Bool.random() ? 12 : -12)
            if doubled >= 24 && doubled <= 108 {
                onNoteOn?(doubled, velocity * 0.55)
                activeNotes.append((note: doubled, offTime: elapsed + duration))
            }
        }
    }

    // MARK: - Helpers

    /// Legacy chord lookup — now delegates to HarmonicContext
    private func currentChord() -> [Int] {
        let octave = Self.zoneBaseOctave[zone] ?? 3
        return harmonicContext.currentChordNotes(octave: octave)
    }

    /// Legacy chord advance — now delegates to HarmonicContext
    private func advanceChord() {
        harmonicContext.forceNextChord()
        currentChordIndex = harmonicContext.chordIndex
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

    /// Select a note biased by the voice profile's preferred intervals.
    /// Combines melodic coherence with the specimen's unique personality.
    private func selectProfileAwareNote(from pitches: [Int], lastNote: Int,
                                         direction: Float, profile: VoiceProfile) -> Int {
        guard !pitches.isEmpty else { return lastNote }

        var bestPitch = pitches[0]
        var bestScore: Float = .infinity

        for pitch in pitches {
            let interval = pitch - lastNote
            let absInterval = abs(interval)

            // Voice profile preference score (lower = more preferred)
            let profileScore = profile.intervalScore(absInterval)

            // Direction penalty
            let dirPenalty: Float
            if direction > 0.1 && pitch < lastNote {
                dirPenalty = Float(absInterval) * 0.3
            } else if direction < -0.1 && pitch > lastNote {
                dirPenalty = Float(absInterval) * 0.3
            } else {
                dirPenalty = 0
            }

            // Gravity toward small intervals (weighted by warmth)
            let gravity = (0.2 + specimenWarmth * 0.5) * Float(absInterval)

            let score = profileScore + gravity + dirPenalty

            if score < bestScore {
                bestScore = score
                bestPitch = pitch
            }
        }

        return bestPitch
    }

    private func releaseExpiredNotes(at currentTime: TimeInterval) {
        let expired = activeNotes.filter { $0.offTime <= currentTime }
        for (note, _) in expired {
            onNoteOff?(note)
        }
        activeNotes.removeAll { $0.offTime <= currentTime }
    }
}
