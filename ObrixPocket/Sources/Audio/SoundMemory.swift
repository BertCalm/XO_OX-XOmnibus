import Foundation

// MARK: - MusicalMemory

/// A compressed record of all musical exposure a specimen has accumulated.
/// Memory shapes the specimen's voice in subtle, cumulative ways —
/// the sound of having lived through thousands of notes.
struct MusicalMemory: Codable {

    // MARK: Core Exposure

    /// Total minutes this specimen has been present during active music
    var totalExposureMinutes: Float = 0

    /// The scale this specimen has heard most (e.g. "major", "minor")
    var dominantScale: String = "major"

    /// The average BPM this specimen has been exposed to most
    var dominantTempo: Float = 100

    /// The MIDI octave (0-8) this specimen hears most notes played in
    var preferredRegister: Int = 4

    // MARK: Exposure Distributions

    /// Minutes heard in each scale/key (e.g. "major": 12.5, "minor": 4.2)
    var harmonicExposure: [String: Float] = [:]

    /// Minutes heard in each tempo band (e.g. "slow_60-80": 8.0, "mid_90-120": 14.0)
    var rhythmicExposure: [String: Float] = [:]

    /// Minutes heard associated with each genre tag (e.g. "ambient": 20.0, "funk": 3.5)
    var genreExposure: [String: Float] = [:]

    // MARK: Saturation

    /// How full this specimen's memory is (asymptotic toward the ceiling, never reaches 1.0).
    /// Derived from totalExposureMinutes via saturationCurve().
    var memorySaturation: Float = 0

    // MARK: Temporal Markers

    var oldestMemory: Date = Date()
    var newestMemory: Date = Date()

    // MARK: - Helpers

    /// Human-readable summary for the Microscope UI
    var summary: String {
        let hours = Int(totalExposureMinutes / 60)
        let mins = Int(totalExposureMinutes) % 60
        if hours > 0 {
            return "\(hours)h \(mins)m total exposure"
        }
        return "\(mins)m total exposure"
    }

    /// True if the memory has enough data to meaningfully influence the voice
    var hasMeaningfulData: Bool { totalExposureMinutes >= 5.0 }
}

// MARK: - MemoryInfluence

/// Subtle parameter adjustments derived from a specimen's MusicalMemory.
/// These are applied on top of the base VoiceProfile — they flavor, not replace.
///
/// All modifier values are in the range -0.15...+0.15 (±15% max shift).
struct MemoryInfluence {

    // MARK: VoiceProfile Modifiers

    /// +: brighter, sharper attacks from major-key exposure
    /// -: darker, softer attacks from minor/chromatic exposure
    let attackSharpnessDelta: Float

    /// +: longer notes from slow-tempo exposure
    /// -: shorter, more staccato from fast-tempo exposure
    let sustainLengthDelta: Float

    /// +: more dynamic from high-energy exposure (funk, techno)
    /// -: flatter dynamics from ambient/drone exposure
    let velocityVariationDelta: Float

    /// Swing adjustment: +: more swing from blues/jazz exposure
    ///                   -: straighter from techno/ambient exposure
    let swingDelta: Float

    /// Register shift: positive means preferred to play higher, negative = lower
    /// Range: -1 to +1 (mapped from register preference over time)
    let registerBias: Float

    /// A short description of what has shaped this specimen (for Microscope UI)
    let characterSummary: String

    // MARK: - No-influence constant

    static let none = MemoryInfluence(
        attackSharpnessDelta: 0,
        sustainLengthDelta: 0,
        velocityVariationDelta: 0,
        swingDelta: 0,
        registerBias: 0,
        characterSummary: "No musical memory yet"
    )
}

// MARK: - SoundMemoryManager

/// Manages the sound memory system — recording musical exposure for each specimen
/// and computing how accumulated memory shapes their voice over time.
///
/// Specimens are not passive. They listen. They remember. They change.
final class SoundMemoryManager: ObservableObject {

    // MARK: - Published State

    @Published var memories: [UUID: MusicalMemory] = [:]

    // MARK: - Constants

    /// Memory saturation ceiling multiplier for elder/ancient specimens (2x normal)
    private static let elderSaturationMultiplier: Float = 2.0

    /// Half-life for memory decay in days (~90 days)
    private static let decayHalfLifeDays: Float = 90.0

    /// Fraction of each parent's memory inherited by a bred specimen
    private static let inheritanceFraction: Float = 0.30

    /// Maximum influence magnitude per parameter (±15%)
    private static let maxInfluenceMagnitude: Float = 0.15

    // MARK: - Persistence

    private let storageKey = "obrix_sound_memories"

    // MARK: - Init

    init() {
        load()
    }

    // MARK: - Recording Exposure

    /// Record a musical exposure event for a specimen.
    ///
    /// Call during: Dives, Arrangements, and Reef Heartbeat ticks.
    ///
    /// - Parameters:
    ///   - specimenId: The specimen that was present during music playback
    ///   - scale: The scale active during this exposure (e.g. "major", "dorian")
    ///   - tempo: BPM of the music during this exposure
    ///   - register: MIDI octave of the dominant note range (0-8)
    ///   - duration: How long this exposure lasted in seconds
    ///   - genre: Optional genre tag (e.g. "ambient", "funk")
    ///   - age: The specimen's age tier — elders have higher saturation ceiling
    func recordExposure(
        specimenId: UUID,
        scale: String,
        tempo: Float,
        register: Int,
        duration: TimeInterval,
        genre: String? = nil,
        age: SpecimenAge = .mature
    ) {
        let minutes = Float(duration / 60.0)
        guard minutes > 0 else { return }

        var memory = memories[specimenId] ?? MusicalMemory(
            oldestMemory: Date(),
            newestMemory: Date()
        )

        // Accumulate total exposure
        memory.totalExposureMinutes += minutes

        // Accumulate harmonic exposure
        memory.harmonicExposure[scale, default: 0] += minutes

        // Accumulate rhythmic exposure
        let tempoBand = Self.tempoBandKey(tempo)
        memory.rhythmicExposure[tempoBand, default: 0] += minutes

        // Accumulate genre exposure
        if let genre = genre {
            memory.genreExposure[genre, default: 0] += minutes
        }

        // Update register preference (exponential moving average)
        let alpha: Float = 0.05  // Slow update — register preference changes very gradually
        memory.preferredRegister = Int(
            (1 - alpha) * Float(memory.preferredRegister) + alpha * Float(register)
        )

        // Update dominants
        memory.dominantScale = Self.dominantKey(in: memory.harmonicExposure)
        memory.dominantTempo = Self.dominantTempo(in: memory.rhythmicExposure)

        // Update saturation
        memory.memorySaturation = Self.saturationCurve(
            exposureMinutes: memory.totalExposureMinutes,
            age: age
        )

        // Update temporal markers
        if memory.totalExposureMinutes <= minutes {
            memory.oldestMemory = Date()  // First record
        }
        memory.newestMemory = Date()

        memories[specimenId] = memory
        save()
    }

    // MARK: - Memory Access

    func getMemory(for specimenId: UUID) -> MusicalMemory {
        memories[specimenId] ?? MusicalMemory()
    }

    // MARK: - Influence Computation

    /// Compute how a specimen's accumulated memory modifies its VoiceProfile.
    /// Returns MemoryInfluence.none if no meaningful data exists.
    func getInfluence(for specimenId: UUID) -> MemoryInfluence {
        let memory = getMemory(for: specimenId)
        guard memory.hasMeaningfulData else { return .none }

        let sat = memory.memorySaturation  // 0-1 scale factor for all influences

        // MARK: Attack sharpness delta
        // Major/pentatonic exposure → brighter attacks
        // Minor/phrygian/blues/chromatic exposure → darker attacks
        let brightScales: Set<String> = ["major", "pentatonic", "mixolydian", "wholeTone"]
        let darkScales:   Set<String> = ["minor", "phrygian", "blues", "chromatic"]
        let brightMinutes = memory.harmonicExposure
            .filter { brightScales.contains($0.key) }
            .values.reduce(0, +)
        let darkMinutes = memory.harmonicExposure
            .filter { darkScales.contains($0.key) }
            .values.reduce(0, +)
        let totalHarmonic = max(1, brightMinutes + darkMinutes)
        let harmonicBias = (brightMinutes - darkMinutes) / totalHarmonic  // -1...+1
        let attackDelta = clampInfluence(harmonicBias * 0.5 * sat)

        // MARK: Sustain length delta
        // Slow tempo exposure → longer sustain (more legato)
        // Fast tempo exposure → shorter sustain (more staccato)
        let slowMinutes = (memory.rhythmicExposure["slow_40-70", default: 0]
                         + memory.rhythmicExposure["slow_60-80", default: 0])
        let fastMinutes = (memory.rhythmicExposure["fast_128-145", default: 0]
                         + memory.rhythmicExposure["fast_110-130", default: 0])
        let totalRhythmic = max(1, memory.rhythmicExposure.values.reduce(0, +))
        let tempoBias = (slowMinutes - fastMinutes) / totalRhythmic  // -1...+1
        let sustainDelta = clampInfluence(tempoBias * 0.4 * sat)

        // MARK: Velocity variation delta
        // High-energy genre exposure → more dynamic
        // Ambient/drone exposure → flatter dynamics
        let energyGenres:  Set<String> = ["funk", "techno", "boom_bap", "jazz"]
        let calmGenres:    Set<String> = ["ambient", "drone", "lo_fi", "classical"]
        let energyMinutes = memory.genreExposure
            .filter { energyGenres.contains($0.key) }
            .values.reduce(0, +)
        let calmMinutes = memory.genreExposure
            .filter { calmGenres.contains($0.key) }
            .values.reduce(0, +)
        let totalGenre = max(1, energyMinutes + calmMinutes)
        let energyBias = (energyMinutes - calmMinutes) / totalGenre
        let velVariationDelta = clampInfluence(energyBias * 0.4 * sat)

        // MARK: Swing delta
        // Jazz/blues/lo-fi exposure → more swing
        // Techno/ambient exposure → straighter timing
        let swingGenres:     Set<String> = ["jazz", "blues", "lo_fi", "funk"]
        let straightGenres:  Set<String> = ["techno", "ambient", "classical"]
        let swingGenreMinutes = memory.genreExposure
            .filter { swingGenres.contains($0.key) }
            .values.reduce(0, +)
        let straightGenreMinutes = memory.genreExposure
            .filter { straightGenres.contains($0.key) }
            .values.reduce(0, +)
        let totalSwingGenre = max(1, swingGenreMinutes + straightGenreMinutes)
        let swingBias = (swingGenreMinutes - straightGenreMinutes) / totalSwingGenre
        let swingDelta = clampInfluence(swingBias * 0.35 * sat)

        // MARK: Register bias
        // Normalized: octave 4 is center (0), lower octaves = negative, higher = positive
        let centerOctave: Float = 4.0
        let registerBias = clampInfluence(
            (Float(memory.preferredRegister) - centerOctave) / 4.0 * sat
        )

        // MARK: Build character summary
        let summary = buildCharacterSummary(
            memory: memory,
            attackDelta: attackDelta,
            sustainDelta: sustainDelta,
            swingDelta: swingDelta,
            registerBias: registerBias
        )

        return MemoryInfluence(
            attackSharpnessDelta: attackDelta,
            sustainLengthDelta: sustainDelta,
            velocityVariationDelta: velVariationDelta,
            swingDelta: swingDelta,
            registerBias: registerBias,
            characterSummary: summary
        )
    }

    // MARK: - Memory Reset

    /// Destructively clears all memory for a specimen.
    /// Caller is responsible for confirming with the player first.
    func resetMemory(for specimenId: UUID) {
        memories.removeValue(forKey: specimenId)
        save()
    }

    // MARK: - Breeding Inheritance

    /// Create a bred specimen's initial memory by blending 30% of each parent's memory.
    /// Call this immediately after breeding, before the new specimen accumulates its own exposure.
    func inheritMemory(specimenId: UUID, parentA: UUID, parentB: UUID) {
        let memA = getMemory(for: parentA)
        let memB = getMemory(for: parentB)

        var inherited = MusicalMemory()
        let f = Self.inheritanceFraction

        inherited.totalExposureMinutes = (memA.totalExposureMinutes + memB.totalExposureMinutes) * f

        // Blend harmonic exposure
        inherited.harmonicExposure = blendExposureDicts(memA.harmonicExposure, memB.harmonicExposure, fraction: f)
        inherited.rhythmicExposure = blendExposureDicts(memA.rhythmicExposure, memB.rhythmicExposure, fraction: f)
        inherited.genreExposure    = blendExposureDicts(memA.genreExposure,    memB.genreExposure,    fraction: f)

        // Average register preference
        inherited.preferredRegister = Int(
            (Float(memA.preferredRegister) + Float(memB.preferredRegister)) / 2
        )

        inherited.dominantScale = inherited.harmonicExposure.isEmpty ? "major"
            : Self.dominantKey(in: inherited.harmonicExposure)
        inherited.dominantTempo = inherited.rhythmicExposure.isEmpty ? 100
            : Self.dominantTempo(in: inherited.rhythmicExposure)

        inherited.memorySaturation = Self.saturationCurve(
            exposureMinutes: inherited.totalExposureMinutes,
            age: .young
        )

        inherited.oldestMemory = Date()
        inherited.newestMemory = Date()

        memories[specimenId] = inherited
        save()
    }

    // MARK: - Memory Decay

    /// Apply temporal decay to all memories. Call once per day (or on app launch).
    /// Very old memories fade slowly — half-life ~90 days.
    func applyDecay() {
        let halfLifeSeconds: Double = Double(Self.decayHalfLifeDays) * 86400.0
        let now = Date()
        var changed = false

        for (specimenId, var memory) in memories {
            let ageSeconds = now.timeIntervalSince(memory.newestMemory)
            guard ageSeconds > 0 else { continue }

            // Decay factor: how much memory remains after ageSeconds
            let decayFactor = Float(pow(0.5, ageSeconds / halfLifeSeconds))
            guard decayFactor < 0.9999 else { continue }  // Skip trivial decay

            memory.totalExposureMinutes     *= decayFactor
            memory.harmonicExposure          = memory.harmonicExposure.mapValues { $0 * decayFactor }
            memory.rhythmicExposure          = memory.rhythmicExposure.mapValues { $0 * decayFactor }
            memory.genreExposure             = memory.genreExposure.mapValues { $0 * decayFactor }
            memory.memorySaturation          = Self.saturationCurve(
                exposureMinutes: memory.totalExposureMinutes,
                age: .mature
            )

            // Prune keys that have decayed to negligible values (< 0.1 minutes)
            memory.harmonicExposure = memory.harmonicExposure.filter { $0.value >= 0.1 }
            memory.rhythmicExposure = memory.rhythmicExposure.filter { $0.value >= 0.1 }
            memory.genreExposure    = memory.genreExposure.filter { $0.value >= 0.1 }

            if !memory.harmonicExposure.isEmpty {
                memory.dominantScale = Self.dominantKey(in: memory.harmonicExposure)
            }

            memories[specimenId] = memory
            changed = true
        }

        if changed { save() }
    }

    // MARK: - Saturation Curve

    /// Asymptotic saturation: approaches ceiling but never reaches 1.0.
    /// Elder/ancient specimens have 2x the effective ceiling.
    static func saturationCurve(exposureMinutes: Float, age: SpecimenAge) -> Float {
        // Base ceiling is 0.95 (never quite saturates)
        let baseCeiling: Float = 0.95
        let ceiling = age == .elder || age == .ancient
            ? baseCeiling * elderSaturationMultiplier
            : baseCeiling

        // Logistic-inspired curve: fast early gain, slow later
        // 60 minutes → ~0.5 saturation for a mature specimen
        let k: Float = 0.02   // Rate constant
        let raw = 1.0 - exp(-k * exposureMinutes)
        return min(ceiling, raw * ceiling)
    }

    // MARK: - Persistence

    private func save() {
        if let data = try? JSONEncoder().encode(memories) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }

    private func load() {
        guard let data = UserDefaults.standard.data(forKey: storageKey),
              let saved = try? JSONDecoder().decode([UUID: MusicalMemory].self, from: data)
        else { return }
        memories = saved
    }

    // MARK: - Private Helpers

    /// Clamp a computed influence value to ±maxInfluenceMagnitude
    private func clampInfluence(_ value: Float) -> Float {
        max(-Self.maxInfluenceMagnitude, min(Self.maxInfluenceMagnitude, value))
    }

    /// Find the key with the highest value in a [String: Float] dictionary.
    private static func dominantKey(in dict: [String: Float]) -> String {
        dict.max(by: { $0.value < $1.value })?.key ?? "major"
    }

    /// Derive the dominant tempo from a rhythmic exposure dictionary.
    /// Returns the midpoint BPM of the most-heard tempo band.
    private static func dominantTempo(in rhythmicExposure: [String: Float]) -> Float {
        guard let topBand = rhythmicExposure.max(by: { $0.value < $1.value })?.key else {
            return 100
        }
        return tempoBandMidpoint(topBand)
    }

    /// Classify a BPM into a named tempo band key.
    static func tempoBandKey(_ bpm: Float) -> String {
        switch bpm {
        case ..<60:     return "slow_40-60"
        case 60..<80:   return "slow_60-80"
        case 80..<100:  return "mid_80-100"
        case 100..<120: return "mid_100-120"
        case 120..<140: return "fast_120-140"
        case 140..<160: return "fast_140-160"
        default:        return "extreme_160+"
        }
    }

    /// Return the midpoint BPM for a tempo band key.
    private static func tempoBandMidpoint(_ band: String) -> Float {
        switch band {
        case "slow_40-60":   return 50
        case "slow_60-80":   return 70
        case "mid_80-100":   return 90
        case "mid_100-120":  return 110
        case "fast_120-140": return 130
        case "fast_140-160": return 150
        default:             return 170
        }
    }

    /// Blend two exposure dictionaries: take fraction * (A + B) / 2 for each key.
    private func blendExposureDicts(
        _ a: [String: Float],
        _ b: [String: Float],
        fraction: Float
    ) -> [String: Float] {
        var result: [String: Float] = [:]
        let allKeys = Set(a.keys).union(Set(b.keys))
        for key in allKeys {
            let avg = ((a[key] ?? 0) + (b[key] ?? 0)) / 2
            result[key] = avg * fraction
        }
        return result
    }

    /// Build a concise human-readable character summary for the Microscope UI.
    private func buildCharacterSummary(
        memory: MusicalMemory,
        attackDelta: Float,
        sustainDelta: Float,
        swingDelta: Float,
        registerBias: Float
    ) -> String {
        guard memory.hasMeaningfulData else {
            return "No musical memory yet"
        }

        var traits: [String] = []

        if memory.memorySaturation > 0.6 {
            traits.append("deeply formed by \(memory.dominantScale) music")
        } else if memory.memorySaturation > 0.2 {
            traits.append("shaped by \(memory.dominantScale) harmonies")
        }

        if sustainDelta > 0.05 {
            traits.append("grown legato from slow tempos")
        } else if sustainDelta < -0.05 {
            traits.append("sharpened by fast music")
        }

        if swingDelta > 0.05 {
            traits.append("swings naturally")
        }

        if registerBias > 0.05 {
            traits.append("prefers high registers")
        } else if registerBias < -0.05 {
            traits.append("gravitates low")
        }

        if traits.isEmpty {
            return "Memory forming — \(memory.summary)"
        }
        return traits.joined(separator: ", ").capitalizedFirst + ". " + memory.summary
    }
}

// MARK: - String Helper

private extension String {
    var capitalizedFirst: String {
        guard let first = first else { return self }
        return first.uppercased() + dropFirst()
    }
}
