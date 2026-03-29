import Foundation

// MARK: - ScaleType

/// Full scale vocabulary for harmonic DNA — superset of HarmonicContext.Scale,
/// including all modes and exotic scales a specimen's genetics can express.
enum ScaleType: String, CaseIterable, Codable {
    case major
    case naturalMinor
    case harmonicMinor
    case dorian
    case mixolydian
    case pentatonicMajor
    case pentatonicMinor
    case blues
    case chromatic
    case wholeTone
    case lydian
    case phrygian

    var displayName: String {
        switch self {
        case .major:           return "Major"
        case .naturalMinor:    return "Natural Minor"
        case .harmonicMinor:   return "Harmonic Minor"
        case .dorian:          return "Dorian"
        case .mixolydian:      return "Mixolydian"
        case .pentatonicMajor: return "Pentatonic Major"
        case .pentatonicMinor: return "Pentatonic Minor"
        case .blues:           return "Blues"
        case .chromatic:       return "Chromatic"
        case .wholeTone:       return "Whole Tone"
        case .lydian:          return "Lydian"
        case .phrygian:        return "Phrygian"
        }
    }

    /// Semitone intervals from root
    var intervals: [Int] {
        switch self {
        case .major:           return [0, 2, 4, 5, 7, 9, 11]
        case .naturalMinor:    return [0, 2, 3, 5, 7, 8, 10]
        case .harmonicMinor:   return [0, 2, 3, 5, 7, 8, 11]
        case .dorian:          return [0, 2, 3, 5, 7, 9, 10]
        case .mixolydian:      return [0, 2, 4, 5, 7, 9, 10]
        case .pentatonicMajor: return [0, 2, 4, 7, 9]
        case .pentatonicMinor: return [0, 3, 5, 7, 10]
        case .blues:           return [0, 3, 5, 6, 7, 10]
        case .chromatic:       return Array(0...11)
        case .wholeTone:       return [0, 2, 4, 6, 8, 10]
        case .lydian:          return [0, 2, 4, 6, 7, 9, 11]
        case .phrygian:        return [0, 1, 3, 5, 7, 8, 10]
        }
    }

    /// Relative brightness — 0=darkest (Locrian-end), 1=brightest (Lydian-end)
    var modalBrightness: Float {
        switch self {
        case .lydian:          return 1.0
        case .major:           return 0.83
        case .mixolydian:      return 0.67
        case .dorian:          return 0.5
        case .naturalMinor:    return 0.33
        case .phrygian:        return 0.17
        case .harmonicMinor:   return 0.28
        case .pentatonicMajor: return 0.72
        case .pentatonicMinor: return 0.38
        case .blues:           return 0.35
        case .wholeTone:       return 0.6      // Ambiguous — lands in middle
        case .chromatic:       return 0.5      // No modal character
        }
    }

    /// Whether this scale is considered pentatonic/simple (fewer notes = simpler)
    var isSimple: Bool {
        switch self {
        case .pentatonicMajor, .pentatonicMinor: return true
        default: return false
        }
    }

    /// Whether this scale is chromatic or ambiguous in tonal center
    var isAtonal: Bool {
        switch self {
        case .chromatic, .wholeTone: return true
        default: return false
        }
    }
}

// MARK: - IntervalAffinity

/// A specimen's affinity for a specific musical interval (measured in semitones)
struct IntervalAffinity: Codable, Equatable {
    /// Interval in semitones (1-12)
    let interval: Int

    /// Affinity strength (0 = avoids this interval, 1 = strongly prefers it)
    let strength: Float

    /// Human-readable interval name
    var intervalName: String {
        switch interval {
        case 1:  return "Minor 2nd"
        case 2:  return "Major 2nd"
        case 3:  return "Minor 3rd"
        case 4:  return "Major 3rd"
        case 5:  return "Perfect 4th"
        case 6:  return "Tritone"
        case 7:  return "Perfect 5th"
        case 8:  return "Minor 6th"
        case 9:  return "Major 6th"
        case 10: return "Minor 7th"
        case 11: return "Major 7th"
        case 12: return "Octave"
        default: return "Interval \(interval)"
        }
    }

    /// Whether this interval is considered consonant
    var isConsonant: Bool {
        [4, 5, 7, 9, 12].contains(interval)
    }
}

// MARK: - HarmonicProfile

/// The complete harmonic identity of a specimen — what it sounds like
/// harmonically, what scales it prefers, and how strongly it clings to its tonal center.
struct HarmonicProfile: Codable {

    // MARK: - Scale Identity

    /// The specimen's preferred scale type
    let preferredScale: ScaleType

    /// Preferred root pitch class (0=C, 1=C#, ... 11=B)
    let preferredRoot: Int

    // MARK: - Interval Preferences

    /// Ranked list of preferred intervals (strongest first)
    let intervalAffinity: [IntervalAffinity]

    // MARK: - Tonal Character

    /// How strongly the specimen resists leaving its preferred key.
    /// 0 = atonal wanderer (freely explores all pitches),
    /// 1 = strongly tonal (gravitates back to home key like a magnet)
    let tonalGravity: Float

    /// Willingness to use out-of-scale (chromatic) notes.
    /// 0 = diatonic purist (only scale tones),
    /// 1 = chromatic explorer (passing tones, borrowed notes freely used)
    let chromaticTolerance: Float

    /// Harmonic complexity preference.
    /// 0 = simple triads and open intervals,
    /// 1 = extended jazz chords, clusters, and dense harmonics
    let harmonicComplexity: Float

    /// Modal character on the Lydian-to-Locrian brightness axis.
    /// 0 = bright (Lydian), 1 = dark (Locrian / Phrygian end)
    let modalCharacter: Float

    // MARK: - Convenience

    /// The strongest single interval preference
    var dominantInterval: IntervalAffinity? {
        intervalAffinity.max(by: { $0.strength < $1.strength })
    }

    /// Whether this profile is fundamentally tonal (gravity > 0.5)
    var isTonal: Bool { tonalGravity > 0.5 }

    /// Root as a displayable note name
    var rootName: String {
        let names = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
        let idx = max(0, min(11, preferredRoot))
        return names[idx]
    }

    /// Display string for UI
    var displayName: String {
        "\(rootName) \(preferredScale.displayName)"
    }
}

// MARK: - HarmonicDNA

/// Converts a specimen's raw genetic value for the `harmonicPreference` trait
/// into a fully realized HarmonicProfile. This is the layer that makes each
/// specimen's genetics musically meaningful.
///
/// Gene value mapping (0-1):
///   0.0 – 0.2  → Pentatonic territory: simple, strongly tonal
///   0.2 – 0.4  → Diatonic territory: major/minor, triadic
///   0.4 – 0.6  → Modal territory: dorian, mixolydian, moderate complexity
///   0.6 – 0.8  → Extended territory: lydian, phrygian, jazz intervals
///   0.8 – 1.0  → Chromatic territory: whole-tone, complex, atonal tendency
enum HarmonicDNA {

    // MARK: - Profile Derivation

    /// Derive a HarmonicProfile from a specimen's genome.
    /// Uses the `harmonicPreference` gene's effective value plus expression type.
    static func derivedFromGenetics(genome: SpecimenGenome) -> HarmonicProfile {
        guard let gene = genome.gene(for: .harmonicPreference) else {
            return defaultProfile()
        }
        return buildProfile(from: gene)
    }

    /// Build a profile from a single harmonicPreference Gene
    static func buildProfile(from gene: Gene) -> HarmonicProfile {
        let baseValue = gene.value
        let effectiveValue = gene.effectiveValue

        // Mutated genes shift the entire profile by ±0.2 on all axes
        let mutationShift: Float = gene.isMutated ? Float.random(in: -0.2...0.2) : 0.0

        // Dominant expression = stronger tonal gravity
        // Recessive = weaker gravity, more atonal tendency
        let expressionGravityBoost: Float
        switch gene.expression {
        case .dominant:   expressionGravityBoost = 0.15
        case .codominant: expressionGravityBoost = 0.0
        case .recessive:  expressionGravityBoost = -0.15
        }

        // Determine scale and interval affinities from gene value
        let (scale, intervals) = scaleAndIntervals(for: baseValue)

        // Determine tonal character from effective value + expression
        let rawGravity     = tonalGravity(for: baseValue) + expressionGravityBoost
        let rawChromatic   = chromaticTolerance(for: baseValue)
        let rawComplexity  = harmonicComplexity(for: effectiveValue)
        let rawModal       = modalCharacter(for: scale)

        // Apply mutation shift to all axes, clamped to 0-1
        let finalGravity    = (rawGravity + mutationShift).clamped(to: 0...1)
        let finalChromatic  = (rawChromatic + abs(mutationShift)).clamped(to: 0...1)
        let finalComplexity = (rawComplexity + mutationShift * 0.5).clamped(to: 0...1)
        let finalModal      = (rawModal + mutationShift).clamped(to: 0...1)

        // Root selection: derived from gene value modulo 12 for deterministic variety
        let rootSeed = Int(baseValue * 100) % 12
        let preferredRoot = rootSeed

        return HarmonicProfile(
            preferredScale: scale,
            preferredRoot: preferredRoot,
            intervalAffinity: intervals,
            tonalGravity: finalGravity,
            chromaticTolerance: finalChromatic,
            harmonicComplexity: finalComplexity,
            modalCharacter: finalModal
        )
    }

    // MARK: - Private Mapping Helpers

    private static func scaleAndIntervals(for value: Float) -> (ScaleType, [IntervalAffinity]) {
        switch value {
        case 0.0..<0.2:
            // Pentatonic zone: simple, universal, strong 5th and 4th affinity
            let scale: ScaleType = value < 0.1 ? .pentatonicMajor : .pentatonicMinor
            let intervals: [IntervalAffinity] = [
                IntervalAffinity(interval: 7, strength: 0.95),   // P5 — open and pure
                IntervalAffinity(interval: 5, strength: 0.90),   // P4
                IntervalAffinity(interval: 12, strength: 0.85),  // Octave
                IntervalAffinity(interval: 4, strength: 0.60),   // M3
                IntervalAffinity(interval: 3, strength: 0.55),   // m3
            ]
            return (scale, intervals)

        case 0.2..<0.4:
            // Diatonic zone: major/minor, triadic focus
            let scale: ScaleType = value < 0.3 ? .major : .naturalMinor
            let intervals: [IntervalAffinity] = [
                IntervalAffinity(interval: 4, strength: 0.90),   // M3 — chord identity
                IntervalAffinity(interval: 7, strength: 0.88),   // P5
                IntervalAffinity(interval: 3, strength: 0.85),   // m3
                IntervalAffinity(interval: 5, strength: 0.75),   // P4
                IntervalAffinity(interval: 9, strength: 0.55),   // M6
            ]
            return (scale, intervals)

        case 0.4..<0.6:
            // Modal zone: dorian/mixolydian, moderate color
            let scale: ScaleType = value < 0.5 ? .dorian : .mixolydian
            let intervals: [IntervalAffinity] = [
                IntervalAffinity(interval: 10, strength: 0.85),  // m7 — modal color
                IntervalAffinity(interval: 7, strength: 0.80),   // P5
                IntervalAffinity(interval: 4, strength: 0.75),   // M3
                IntervalAffinity(interval: 9, strength: 0.65),   // M6
                IntervalAffinity(interval: 2, strength: 0.55),   // M2 (passing)
            ]
            return (scale, intervals)

        case 0.6..<0.8:
            // Extended zone: lydian/phrygian, jazz intervals, color tones
            let scale: ScaleType = value < 0.7 ? .lydian : .phrygian
            let intervals: [IntervalAffinity] = [
                IntervalAffinity(interval: 11, strength: 0.85),  // M7 — jazz sophistication
                IntervalAffinity(interval: 6, strength: 0.75),   // Tritone — tension/release
                IntervalAffinity(interval: 9, strength: 0.70),   // M6
                IntervalAffinity(interval: 2, strength: 0.65),   // M2
                IntervalAffinity(interval: 1, strength: 0.50),   // m2 (phrygian color)
            ]
            return (scale, intervals)

        default:
            // Chromatic zone: whole-tone and chromatic, atonal tendency
            let scale: ScaleType = value < 0.9 ? .wholeTone : .chromatic
            let intervals: [IntervalAffinity] = [
                IntervalAffinity(interval: 6, strength: 0.88),   // Tritone — atonal anchor
                IntervalAffinity(interval: 1, strength: 0.80),   // m2 — chromatic bite
                IntervalAffinity(interval: 11, strength: 0.75),  // M7
                IntervalAffinity(interval: 2, strength: 0.65),   // M2
                IntervalAffinity(interval: 10, strength: 0.60),  // m7
            ]
            return (scale, intervals)
        }
    }

    private static func tonalGravity(for value: Float) -> Float {
        // Invert: low gene value = high tonal gravity (strongly tonal)
        // High gene value = low gravity (atonal tendency)
        return 1.0 - (value * 0.9)
    }

    private static func chromaticTolerance(for value: Float) -> Float {
        // Higher gene value = more chromatic tolerance
        return value * 0.85
    }

    private static func harmonicComplexity(for effectiveValue: Float) -> Float {
        // Effective value (gene × expression multiplier) drives complexity
        return effectiveValue * 0.9
    }

    private static func modalCharacter(for scale: ScaleType) -> Float {
        // Derive from scale's inherent brightness, inverted (1 = dark)
        return 1.0 - scale.modalBrightness
    }

    // MARK: - Default

    static func defaultProfile() -> HarmonicProfile {
        HarmonicProfile(
            preferredScale: .major,
            preferredRoot: 0,
            intervalAffinity: [
                IntervalAffinity(interval: 7, strength: 0.9),
                IntervalAffinity(interval: 4, strength: 0.8),
                IntervalAffinity(interval: 5, strength: 0.7),
            ],
            tonalGravity: 0.7,
            chromaticTolerance: 0.2,
            harmonicComplexity: 0.3,
            modalCharacter: 0.17   // Major = relatively bright
        )
    }
}

// MARK: - HarmonicCompatibility

/// Measures how well two harmonic profiles work together and blends
/// them into a combined identity when specimens are wired.
///
/// Design: compatibility isn't a quality judgment — high tension (low compatibility)
/// creates interesting dissonance. Players may deliberately wire conflicting
/// specimens for a clashing, dissonant character.
enum HarmonicCompatibility {

    // MARK: - Compatibility Score

    /// Returns 0-1. High = consonant pairing. Low = dissonant / conflicting.
    /// Both extremes are musically interesting — 0.5 is the least interesting.
    static func compatibility(a: HarmonicProfile, b: HarmonicProfile) -> Float {
        var score: Float = 0.0
        var weight: Float = 0.0

        // 1. Root relationship (most impactful — 40% weight)
        let rootScore = rootCompatibility(rootA: a.preferredRoot, rootB: b.preferredRoot)
        score += rootScore * 0.40
        weight += 0.40

        // 2. Scale overlap — share scale tones (30% weight)
        let scaleScore = scaleOverlap(a: a.preferredScale, b: b.preferredScale)
        score += scaleScore * 0.30
        weight += 0.30

        // 3. Tonal gravity alignment (15% weight)
        // Two strongly tonal specimens with different roots fight. Similar gravity = easier pairing.
        let gravityDiff = abs(a.tonalGravity - b.tonalGravity)
        let gravityScore = 1.0 - (gravityDiff * 0.6)
        score += gravityScore * 0.15
        weight += 0.15

        // 4. Interval affinity overlap (15% weight)
        let intervalScore = intervalOverlap(a: a.intervalAffinity, b: b.intervalAffinity)
        score += intervalScore * 0.15
        weight += 0.15

        return weight > 0 ? (score / weight).clamped(to: 0...1) : 0.5
    }

    // MARK: - Harmonic Blend

    /// Creates the combined harmonic identity when two specimens play together.
    /// The blend is not a simple average — dominant profiles win, and the
    /// combination can produce emergent qualities neither had alone.
    static func harmonicBlend(a: HarmonicProfile, b: HarmonicProfile) -> HarmonicProfile {
        // Dominant parent wins on scale (higher tonal gravity = more assertive)
        let (dominant, recessive) = a.tonalGravity >= b.tonalGravity ? (a, b) : (b, a)
        let blendScale = dominant.preferredScale

        // Root: dominant parent's root, unless compatibility is very low (tritone tension)
        let rootInterval = abs(a.preferredRoot - b.preferredRoot)
        let isTritoneConflict = rootInterval == 6
        let blendRoot = isTritoneConflict
            ? dominant.preferredRoot   // Keep dominant root — tension is deliberate
            : dominant.preferredRoot

        // Blend the continuous axes — weighted toward dominant
        let domWeight: Float = 0.65
        let recWeight: Float = 0.35

        let blendGravity     = (dominant.tonalGravity * domWeight + recessive.tonalGravity * recWeight)
        let blendChromatic   = max(dominant.chromaticTolerance, recessive.chromaticTolerance) * 0.9 + 0.05
        let blendComplexity  = (dominant.harmonicComplexity * domWeight + recessive.harmonicComplexity * recWeight)
        let blendModal       = (dominant.modalCharacter * domWeight + recessive.modalCharacter * recWeight)

        // Merge interval affinities: take up to top 5 from each, weighted
        var mergedIntervals = mergeIntervalAffinities(
            dominant: dominant.intervalAffinity,
            recessive: recessive.intervalAffinity
        )

        // Tritone conflict adds tritone affinity to the blend (interesting tension)
        if isTritoneConflict {
            let existingTritone = mergedIntervals.first { $0.interval == 6 }
            if existingTritone == nil {
                mergedIntervals.append(IntervalAffinity(interval: 6, strength: 0.65))
            }
            mergedIntervals.sort { $0.strength > $1.strength }
        }

        return HarmonicProfile(
            preferredScale: blendScale,
            preferredRoot: blendRoot,
            intervalAffinity: Array(mergedIntervals.prefix(6)),
            tonalGravity: blendGravity.clamped(to: 0...1),
            chromaticTolerance: blendChromatic.clamped(to: 0...1),
            harmonicComplexity: blendComplexity.clamped(to: 0...1),
            modalCharacter: blendModal.clamped(to: 0...1)
        )
    }

    // MARK: - Breeding Inheritance

    /// Produce the offspring's harmonic profile from two parent profiles.
    /// The dominant parent's scale is inherited; continuous values are blended
    /// with possible mutation.
    static func inheritedProfile(from parentA: HarmonicProfile, parentB: HarmonicProfile,
                                  isMutated: Bool) -> HarmonicProfile {
        // Dominant parent (higher tonal gravity) contributes scale
        let (dominant, recessive) = parentA.tonalGravity >= parentB.tonalGravity
            ? (parentA, parentB) : (parentB, parentA)

        let mutationAmount: Float = isMutated ? Float.random(in: -0.2...0.2) : 0.0

        let blendGravity    = ((dominant.tonalGravity + recessive.tonalGravity) / 2.0 + mutationAmount)
        let blendChromatic  = ((dominant.chromaticTolerance + recessive.chromaticTolerance) / 2.0 + abs(mutationAmount) * 0.5)
        let blendComplexity = ((dominant.harmonicComplexity + recessive.harmonicComplexity) / 2.0 + mutationAmount * 0.5)
        let blendModal      = ((dominant.modalCharacter + recessive.modalCharacter) / 2.0 + mutationAmount * 0.3)

        // Root: mutated offspring may drift to a new root
        let offspringRoot: Int
        if isMutated {
            offspringRoot = (dominant.preferredRoot + Int.random(in: 0...11)) % 12
        } else {
            offspringRoot = dominant.preferredRoot
        }

        return HarmonicProfile(
            preferredScale: dominant.preferredScale,
            preferredRoot: offspringRoot,
            intervalAffinity: mergeIntervalAffinities(
                dominant: dominant.intervalAffinity,
                recessive: recessive.intervalAffinity
            ),
            tonalGravity: blendGravity.clamped(to: 0...1),
            chromaticTolerance: blendChromatic.clamped(to: 0...1),
            harmonicComplexity: blendComplexity.clamped(to: 0...1),
            modalCharacter: blendModal.clamped(to: 0...1)
        )
    }

    // MARK: - Private Helpers

    private static func rootCompatibility(rootA: Int, rootB: Int) -> Float {
        let interval = min(abs(rootA - rootB), 12 - abs(rootA - rootB))
        // Consonant root relationships (P5=7, P4=5, M3=4, m3=3)
        // Dissonant (tritone=6, m2=1) score low — but not zero; tension has value
        switch interval {
        case 0:  return 1.00   // Unison — perfect agreement
        case 7:  return 0.90   // Perfect 5th — strongest consonance
        case 5:  return 0.85   // Perfect 4th
        case 4:  return 0.75   // Major 3rd
        case 3:  return 0.70   // Minor 3rd
        case 9:  return 0.65   // Major 6th
        case 8:  return 0.60   // Minor 6th
        case 2:  return 0.45   // Major 2nd — mild dissonance
        case 10: return 0.40   // Minor 7th
        case 11: return 0.30   // Major 7th — strong dissonance
        case 1:  return 0.20   // Minor 2nd — maximum dissonance
        case 6:  return 0.15   // Tritone — maximum tension
        default: return 0.50
        }
    }

    private static func scaleOverlap(a: ScaleType, b: ScaleType) -> Float {
        if a == b { return 1.0 }

        let setA = Set(a.intervals)
        let setB = Set(b.intervals)
        let shared = setA.intersection(setB).count
        let union  = setA.union(setB).count

        guard union > 0 else { return 0.5 }
        return Float(shared) / Float(union)   // Jaccard similarity
    }

    private static func intervalOverlap(a: [IntervalAffinity], b: [IntervalAffinity]) -> Float {
        let setA = Set(a.map { $0.interval })
        let setB = Set(b.map { $0.interval })
        let shared = setA.intersection(setB).count
        let total  = max(setA.count, setB.count)
        guard total > 0 else { return 0.5 }
        return Float(shared) / Float(total)
    }

    private static func mergeIntervalAffinities(dominant: [IntervalAffinity],
                                                  recessive: [IntervalAffinity]) -> [IntervalAffinity] {
        var merged: [Int: Float] = [:]

        // Dominant intervals at full strength
        for affinity in dominant {
            merged[affinity.interval] = affinity.strength
        }

        // Recessive intervals at reduced strength; promote if already present
        for affinity in recessive {
            if let existing = merged[affinity.interval] {
                merged[affinity.interval] = max(existing, affinity.strength * 0.7)
            } else {
                merged[affinity.interval] = affinity.strength * 0.6
            }
        }

        return merged
            .sorted { $0.value > $1.value }
            .map { IntervalAffinity(interval: $0.key, strength: $0.value) }
    }
}

// MARK: - HarmonicProfileStore

/// Lightweight cache so HarmonicProfiles are not recomputed on every render.
/// Invalidated when the genome changes (breeding, mutation).
final class HarmonicProfileStore: ObservableObject {

    // MARK: - State

    @Published private(set) var profiles: [UUID: HarmonicProfile] = [:]

    // MARK: - Access

    /// Get (or compute and cache) the harmonic profile for a genome
    func profile(for genome: SpecimenGenome) -> HarmonicProfile {
        if let cached = profiles[genome.id] {
            return cached
        }
        let derived = HarmonicDNA.derivedFromGenetics(genome: genome)
        profiles[genome.id] = derived
        return derived
    }

    /// Invalidate a cached profile (call after breeding or mutation)
    func invalidate(genomeID: UUID) {
        profiles.removeValue(forKey: genomeID)
    }

    /// Invalidate all profiles (call after bulk genome changes)
    func invalidateAll() {
        profiles.removeAll()
    }

    // MARK: - Wiring

    /// Get the blended harmonic profile for two wired specimens
    func blendedProfile(genomeA: SpecimenGenome, genomeB: SpecimenGenome) -> HarmonicProfile {
        let profileA = profile(for: genomeA)
        let profileB = profile(for: genomeB)
        return HarmonicCompatibility.harmonicBlend(a: profileA, b: profileB)
    }

    /// Compatibility score between two wired specimens (0-1)
    func compatibilityScore(genomeA: SpecimenGenome, genomeB: SpecimenGenome) -> Float {
        let profileA = profile(for: genomeA)
        let profileB = profile(for: genomeB)
        return HarmonicCompatibility.compatibility(a: profileA, b: profileB)
    }

    // MARK: - Persistence

    func save() {
        if let data = try? JSONEncoder().encode(profiles) {
            UserDefaults.standard.set(data, forKey: "harmonicProfiles")
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: "harmonicProfiles"),
           let decoded = try? JSONDecoder().decode([UUID: HarmonicProfile].self, from: data) {
            profiles = decoded
        }
    }
}

// MARK: - Float Clamping Extension

private extension Float {
    func clamped(to range: ClosedRange<Float>) -> Float {
        Swift.max(range.lowerBound, Swift.min(range.upperBound, self))
    }
}
