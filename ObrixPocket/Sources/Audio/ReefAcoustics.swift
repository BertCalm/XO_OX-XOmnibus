import Foundation
import Combine

// MARK: - SpecimenAcousticData

/// Lightweight data snapshot of a specimen's acoustic-relevant properties.
/// Only the fields ReefAcousticsManager needs — avoids pulling in full model types.
struct SpecimenAcousticData {
    /// Specimen subtype identifier (e.g. "polyblep-saw")
    let subtypeId: String

    /// Age in days (real-time, from catch date)
    let ageDays: Int

    /// Number of active wiring connections this specimen is part of
    let couplingCount: Int

    /// Musical role this specimen plays in the Dive
    let musicalRole: MusicalRole

    /// Whether the specimen is currently part of an active nursery (breeding in progress)
    let isInNursery: Bool
}

// MARK: - ReefAcousticProfile

/// The complete acoustic character of a reef at a point in time.
/// This profile is computed from the current population and fed to the audio engine.
struct ReefAcousticProfile: Codable {

    // MARK: - Reverb Parameters

    /// Decay time of the reverb tail in seconds (0.1–8.0)
    var reverbTime: Float

    /// High-frequency absorption coefficient (0=open water, 1=dense coral)
    var reverbDamping: Float

    /// Density of early reflections (0=anechoic, 1=extremely dense)
    var earlyReflections: Float

    /// Perceived room/space size (0=intimate, 1=vast)
    var roomSize: Float

    // MARK: - Underwater Character

    /// Frequency-dependent absorption simulating underwater acoustics.
    /// Higher values roll off the high-frequency content more aggressively.
    var waterAbsorption: Float

    /// Dominant resonant frequencies present in the space (3–5 values, Hz)
    var resonantFrequencies: [Float]

    // MARK: - Ambience

    /// Background ocean ambience level (0=silent, 1=loud ocean bed)
    var ambientLevel: Float

    /// Low-pass filtering depth that increases with perceived depth of the reef.
    /// 0=shallow and bright, 1=abyssal, heavily filtered
    var depthEffect: Float

    // MARK: - Computed Properties

    /// A single brightness estimate for UI display (inverse of damping + water absorption)
    var perceivedBrightness: Float {
        let rawBrightness = 1.0 - ((reverbDamping + waterAbsorption) / 2.0)
        return rawBrightness.reefClamped
    }

    /// Estimated space classification for UI
    var spaceLabel: String {
        switch roomSize {
        case 0.0..<0.25: return "Intimate"
        case 0.25..<0.5: return "Chamber"
        case 0.5..<0.75: return "Hall"
        default:         return "Vast"
        }
    }
}

// MARK: - Built-in Presets

extension ReefAcousticProfile {

    /// Shallow Lagoon — bright, intimate, lively early reflections
    static let shallowLagoon = ReefAcousticProfile(
        reverbTime: 0.6,
        reverbDamping: 0.55,        // Warm but not over-absorbed
        earlyReflections: 0.35,     // Moderate reflections from sand bed
        roomSize: 0.18,
        waterAbsorption: 0.20,      // Clear, shallow water = minimal absorption
        resonantFrequencies: [320, 640, 1280, 2560],
        ambientLevel: 0.30,
        depthEffect: 0.10
    )

    /// Coral Cathedral — long reverb, dense reflections, massive warmth
    static let coralCathedral = ReefAcousticProfile(
        reverbTime: 4.2,
        reverbDamping: 0.72,        // Coral absorbs high frequencies heavily
        earlyReflections: 0.80,     // Dense coral structures = many reflections
        roomSize: 0.78,
        waterAbsorption: 0.45,
        resonantFrequencies: [120, 240, 480, 960, 1920],
        ambientLevel: 0.25,
        depthEffect: 0.35
    )

    /// The Deep Shelf — massive space, heavy water absorption, low-pass dominant
    static let deepShelf = ReefAcousticProfile(
        reverbTime: 7.5,
        reverbDamping: 0.30,        // Deep water = less damping (cold, dense)
        earlyReflections: 0.20,     // Few reflections in open deep water
        roomSize: 0.95,
        waterAbsorption: 0.85,      // Extreme HF absorption in deep water
        resonantFrequencies: [55, 110, 220],
        ambientLevel: 0.60,
        depthEffect: 0.90
    )

    /// Kelp Forest — medium reverb, high early reflection density (kelp scatters sound)
    static let kelpForest = ReefAcousticProfile(
        reverbTime: 1.8,
        reverbDamping: 0.65,        // Kelp fronds absorb mids
        earlyReflections: 0.85,     // Kelp canopy = dense scattering
        roomSize: 0.45,
        waterAbsorption: 0.38,
        resonantFrequencies: [200, 400, 800, 1600],
        ambientLevel: 0.40,
        depthEffect: 0.28
    )

    /// All named built-in presets
    static var builtInPresets: [(name: String, profile: ReefAcousticProfile)] {
        [
            ("Shallow Lagoon",   .shallowLagoon),
            ("Coral Cathedral",  .coralCathedral),
            ("The Deep Shelf",   .deepShelf),
            ("Kelp Forest",      .kelpForest),
        ]
    }
}

// MARK: - ReefAcousticFactor

/// An intermediate calculation structure that breaks the acoustic
/// computation into named contributing factors before they're summed.
private struct ReefAcousticFactor {
    let specimenCount: Int
    let specimenDiversity: Float         // 0-1: proportion of unique subtypes
    let averageAgeDays: Float
    let couplingDensity: Float           // 0-1: wired pairs / max possible pairs
    let breedingActivityLevel: Float     // 0-1: proportion currently in nursery
    let season: Season?
    let weather: WeatherCondition?

    /// Reverb time contribution from population size
    var populationReverbBoost: Float {
        let normalized = Float(min(specimenCount, 16)) / 16.0
        return normalized * 0.8   // Up to +0.8s from population
    }

    /// Early reflection density from population size
    var populationReflectionDensity: Float {
        let normalized = Float(min(specimenCount, 16)) / 16.0
        return normalized * 0.6   // More specimens = more reflective surfaces
    }

    /// Damping increase from age — older reefs = warmer (more HF absorbed)
    var ageBasedDamping: Float {
        let normalized = min(averageAgeDays / 180.0, 1.0)   // 180 days = max aging effect
        return normalized * 0.25
    }

    /// Resonant frequency spread from diversity
    var diversityResonanceSpread: Float {
        specimenDiversity   // 1.0 diversity = full spread of resonances
    }

    /// Seasonal reverb modifier — winter = longer (cold water), summer = shorter
    var seasonalReverbMod: Float {
        switch season {
        case .winterDeep:   return +0.8    // Cold water carries sound farther
        case .springBloom:  return +0.2
        case .autumnDrift:  return -0.1
        case .summerSurge:  return -0.4    // Warm water = shorter reverb
        case .none:         return 0.0
        }
    }

    /// Seasonal damping modifier — winter = brighter (less biological absorption)
    var seasonalDampingMod: Float {
        switch season {
        case .winterDeep:   return -0.10   // Cold = less biological material = brighter
        case .summerSurge:  return +0.08   // Warm = more biology = warmer/darker
        case .springBloom:  return +0.04
        case .autumnDrift:  return +0.03
        case .none:         return 0.0
        }
    }

    /// Weather room size boost — storms expand the sense of space
    var weatherRoomMod: Float {
        switch weather {
        case .storm:         return +0.20
        case .nightDeep:     return +0.10
        case .eveningGlow:   return +0.05
        case .morningLight:  return -0.05
        case .warmAfternoon: return 0.0
        case .calm:          return 0.0
        case .none:          return 0.0
        }
    }

    /// Weather ambient level boost — storms increase ocean bed noise
    var weatherAmbientMod: Float {
        switch weather {
        case .storm:         return +0.30
        case .nightDeep:     return +0.10
        case .morningLight:  return -0.05
        case .warmAfternoon: return 0.0
        case .eveningGlow:   return -0.02
        case .calm:          return -0.05
        case .none:          return 0.0
        }
    }

    /// Coupling density adds resonant richness
    var couplingResonanceBoost: Float {
        couplingDensity * 0.35
    }

    /// Breeding activity adds subtle modulation (captured as ambient level micro-bump)
    var breedingAmbientBump: Float {
        breedingActivityLevel * 0.08
    }
}

// MARK: - ReefAcousticsManager

/// Manages the acoustic properties of the reef as a living system.
///
/// The reef's sound space evolves based on who lives in it:
///   - More specimens = denser early reflections, slightly longer reverb
///   - Diverse population = wider resonant frequency spread
///   - Older reef = warmer, more damped reverb (accumulated biological growth)
///   - Storms expand the room; winter deepens the reverb
///   - Wired pairs enrich the resonant frequency base
///   - Active nursery adds subtle modulation to the reverb tail
///
/// Changes are smoothly interpolated over 2 seconds to prevent jarring jumps.
final class ReefAcousticsManager: ObservableObject {

    // MARK: - Published State

    @Published private(set) var currentProfile: ReefAcousticProfile = .shallowLagoon

    // MARK: - Private State

    /// Baseline profile saved when compare mode is entered
    private var compareBaseline: ReefAcousticProfile?

    /// History of profiles over time (kept lightweight — stores last 50 snapshots)
    private var acousticHistory: [AcousticHistoryEntry] = []
    private let maxHistoryCount = 50

    /// Interpolation state for smooth crossfades
    private var targetProfile: ReefAcousticProfile = .shallowLagoon
    private var interpolationProgress: Float = 1.0   // 1.0 = at target
    private var interpolationTimer: Timer?
    private let crossfadeDuration: TimeInterval = 2.0
    private let interpolationStepInterval: TimeInterval = 0.05   // 20 fps update rate

    // MARK: - Lifecycle

    deinit {
        interpolationTimer?.invalidate()
    }

    // MARK: - Recalculation

    /// Recompute the acoustic profile from the current reef state.
    /// Call this whenever population changes, season transitions, or weather changes.
    func recalculate(specimens: [SpecimenAcousticData],
                     season: Season? = nil,
                     weather: WeatherCondition? = nil) {
        let newProfile = compute(specimens: specimens, season: season, weather: weather)
        beginCrossfade(to: newProfile)

        // Record history
        let entry = AcousticHistoryEntry(date: Date(), profile: newProfile,
                                          specimenCount: specimens.count)
        acousticHistory.append(entry)
        if acousticHistory.count > maxHistoryCount {
            acousticHistory.removeFirst()
        }
    }

    // MARK: - Computation

    private func compute(specimens: [SpecimenAcousticData],
                          season: Season?,
                          weather: WeatherCondition?) -> ReefAcousticProfile {
        guard !specimens.isEmpty else {
            // Empty reef defaults toward Shallow Lagoon with added silence
            var empty = ReefAcousticProfile.shallowLagoon
            empty.ambientLevel = 0.10
            empty.reverbTime = 0.4
            return empty
        }

        // Build factor inputs
        let diversity = specimenDiversityScore(specimens: specimens)
        let avgAge = specimens.map { Float($0.ageDays) }.reduce(0, +) / Float(specimens.count)
        let totalPairs = specimens.count * (specimens.count - 1) / 2
        let wiredPairs = specimens.reduce(0) { $0 + $1.couplingCount } / 2  // Each pair counted twice
        let couplingDensity = totalPairs > 0
            ? Float(min(wiredPairs, totalPairs)) / Float(totalPairs)
            : 0.0
        let breedingActivity = Float(specimens.filter { $0.isInNursery }.count) / Float(specimens.count)

        let factor = ReefAcousticFactor(
            specimenCount: specimens.count,
            specimenDiversity: diversity,
            averageAgeDays: avgAge,
            couplingDensity: couplingDensity,
            breedingActivityLevel: breedingActivity,
            season: season,
            weather: weather
        )

        // Base profile: starts from Shallow Lagoon and scales up
        var reverbTime = 0.5
            + factor.populationReverbBoost
            + factor.seasonalReverbMod

        var reverbDamping = 0.40
            + factor.ageBasedDamping
            + factor.seasonalDampingMod

        var earlyReflections = 0.20
            + factor.populationReflectionDensity

        var roomSize = 0.20
            + (Float(specimens.count) / 32.0) * 0.50
            + factor.weatherRoomMod

        var waterAbsorption = 0.15
            + (avgAge / 365.0 * 0.20).clamped(to: 0...0.20)

        let resonantFreqs = computeResonantFrequencies(
            specimens: specimens,
            diversitySpread: factor.diversityResonanceSpread,
            couplingBoost: factor.couplingResonanceBoost
        )

        var ambientLevel = 0.15
            + factor.weatherAmbientMod
            + factor.breedingAmbientBump

        var depthEffect = 0.10
            + waterAbsorption * 0.4

        // Clamp all values to valid ranges
        reverbTime      = reverbTime.clamped(to: 0.1...8.0)
        reverbDamping   = reverbDamping.clamped(to: 0...1)
        earlyReflections = earlyReflections.clamped(to: 0...1)
        roomSize        = roomSize.clamped(to: 0...1)
        waterAbsorption = waterAbsorption.clamped(to: 0...1)
        ambientLevel    = ambientLevel.clamped(to: 0...1)
        depthEffect     = depthEffect.clamped(to: 0...1)

        return ReefAcousticProfile(
            reverbTime: reverbTime,
            reverbDamping: reverbDamping,
            earlyReflections: earlyReflections,
            roomSize: roomSize,
            waterAbsorption: waterAbsorption,
            resonantFrequencies: resonantFreqs,
            ambientLevel: ambientLevel,
            depthEffect: depthEffect
        )
    }

    // MARK: - Resonant Frequency Computation

    private func computeResonantFrequencies(specimens: [SpecimenAcousticData],
                                              diversitySpread: Float,
                                              couplingBoost: Float) -> [Float] {
        // Base frequencies derived from specimen roles
        var baseFreqs: [Float] = []

        // Each musical role contributes a characteristic frequency range
        for specimen in specimens {
            let roleFreq = characteristicFrequency(for: specimen.musicalRole)
            baseFreqs.append(roleFreq)
        }

        // Deduplicate nearby frequencies (within a major 3rd = 1.26x)
        let sorted = baseFreqs.sorted()
        var deduped: [Float] = []
        for freq in sorted {
            if let last = deduped.last, freq < last * 1.20 {
                continue   // Too close to previous — skip
            }
            deduped.append(freq)
        }

        // More diverse reef = wider spread; coupling = extra resonances at harmonics
        var resonances = Array(deduped.prefix(4))

        if couplingBoost > 0.3 && resonances.count >= 2 {
            // Coupling creates sum/difference resonances
            let sumFreq = (resonances[0] + resonances[1]) / 2.0
            resonances.append(sumFreq.clamped(to: 80...8000))
        }

        // Ensure at least 3, at most 5 resonances — use deterministic fill values
        let fillFreqs: [Float] = [330, 660, 990]
        var fillIndex = 0
        while resonances.count < 3 {
            resonances.append(fillFreqs[fillIndex % fillFreqs.count])
            fillIndex += 1
        }

        return Array(resonances.prefix(5))
    }

    private func characteristicFrequency(for role: MusicalRole) -> Float {
        // Deterministic: return the midpoint of each role's frequency range.
        // The diversity spread and coupling boost add variation without randomness.
        switch role {
        case .bass:    return 120.0
        case .rhythm:  return 300.0
        case .harmony: return 450.0
        case .melody:  return 900.0
        case .texture: return 2000.0
        case .effect:  return 4000.0
        }
    }

    // MARK: - Diversity Score

    private func specimenDiversityScore(specimens: [SpecimenAcousticData]) -> Float {
        guard !specimens.isEmpty else { return 0.0 }
        let uniqueSubtypes = Set(specimens.map { $0.subtypeId }).count
        return Float(uniqueSubtypes) / Float(specimens.count)
    }

    // MARK: - Smooth Crossfade

    private func beginCrossfade(to newProfile: ReefAcousticProfile) {
        targetProfile = newProfile
        interpolationProgress = 0.0

        interpolationTimer?.invalidate()
        let stepSize = Float(interpolationStepInterval / crossfadeDuration)

        interpolationTimer = Timer.scheduledTimer(
            withTimeInterval: interpolationStepInterval,
            repeats: true
        ) { [weak self] timer in
            guard let self else { timer.invalidate(); return }

            self.interpolationProgress = min(1.0, self.interpolationProgress + stepSize)
            let newProfile = interpolate(
                from: self.currentProfile,
                to: self.targetProfile,
                t: smoothstep(self.interpolationProgress)
            )
            let atTarget = self.interpolationProgress >= 1.0
            let finalProfile = atTarget ? self.targetProfile : newProfile

            DispatchQueue.main.async { [weak self] in
                guard let self else { return }
                self.currentProfile = finalProfile
                if atTarget {
                    timer.invalidate()
                    self.interpolationTimer = nil
                }
            }
        }
    }

    // MARK: - Audio Parameter Export

    /// Translate the current reef acoustic profile into OBRIX engine parameters.
    ///
    /// `getAudioParameters()` returns `reef_*` keys that describe the acoustic state.
    /// This method converts those into the `obrix_*` parameter IDs that ObrixBridge
    /// accepts, applying the appropriate normalizations for each engine parameter range.
    ///
    /// Parameter mapping rationale:
    /// - `reef_reverbTime` (0.1–8.0 s) → `obrix_fx1Param` (0–1): reverb decay knob.
    ///   Normalized to 8s ceiling; values above 8s are clamped to 1.0.
    /// - `reef_reverbDamping` (0–1) → `obrix_fx1Mix` (0–1): inverted so high damping
    ///   (dense coral = short HF tail) corresponds to a drier mix.
    /// - `reef_earlyReflections` (0–1) → combined with roomSize as a secondary
    ///   contribution to `obrix_fx1Param`; denser reflections push the reverb param up.
    /// - `reef_waterAbsorption` (0–1) → `obrix_proc1Cutoff` (0–1 pre-scale): higher
    ///   absorption rolls off high frequencies — maps to a lower cutoff.
    /// - `reef_depthEffect` (0–1) → further dims `obrix_proc1Cutoff` for deep-water darkness.
    /// - `reef_ambientLevel` (0–1) → `obrix_mod2Depth` (0–1): ambient motion drives the
    ///   modulator depth so deeper reefs have more subtle background movement.
    func getObrixParameters() -> [String: Float] {
        let p = currentProfile
        var obrix: [String: Float] = [:]

        // Reverb type: always target FX slot 1 as reverb (type 3 = reverb-hall).
        // Setting this every time ensures the FX slot is the right type even when
        // the engine was previously configured with a different FX mode.
        obrix["obrix_fx1Type"] = 3

        // Reverb decay time → FX1 parameter knob.
        // Normalize 0.1–8.0 s range to 0–1. Long reverb times produce values near 1.0.
        obrix["obrix_fx1Param"] = min(1.0, p.reverbTime / 8.0)

        // Reverb damping → FX1 wet mix (inverted).
        // High damping = coral absorbs sound = shorter effective tail = less wet needed.
        // Scale to 0.3–0.85 so the reverb is never fully dry or fully wet from reef alone.
        let baseMix = 1.0 - (p.reverbDamping * 0.55)   // 1.0 → 0.45 as damping increases
        obrix["obrix_fx1Mix"] = baseMix.clamped(to: 0.3...0.85)

        // Water absorption → processor cutoff (pre-scale 0–1 value; AudioEngineManager
        // scales 0–1 to 20–20000 Hz via parameterMapping "obrix_flt1Cutoff" → "obrix_proc1Cutoff").
        // Higher absorption = darker water = lower cutoff.
        var cutoff: Float = 1.0 - (p.waterAbsorption * 0.60)

        // Depth effect compounds the cutoff reduction for abyssal environments.
        cutoff *= (1.0 - p.depthEffect * 0.30)

        obrix["obrix_proc1Cutoff"] = cutoff.clamped(to: 0.05...1.0)

        // Ambient level → modulator depth.
        // Drives subtle ongoing movement in the sound to match reef background noise.
        // Scaled to 0–0.35 so it never overwhelms the specimen's own modulation.
        obrix["obrix_mod2Depth"] = (p.ambientLevel * 0.35).clamped(to: 0...0.35)

        return obrix
    }

    /// Returns a flat dictionary of parameter key/value pairs the audio engine consumes.
    /// Keys follow the `obrix_` prefix convention for easy routing to OBRIX parameters.
    func getAudioParameters() -> [String: Float] {
        let p = currentProfile
        var params: [String: Float] = [
            "reef_reverbTime":        p.reverbTime,
            "reef_reverbDamping":     p.reverbDamping,
            "reef_earlyReflections":  p.earlyReflections,
            "reef_roomSize":          p.roomSize,
            "reef_waterAbsorption":   p.waterAbsorption,
            "reef_ambientLevel":      p.ambientLevel,
            "reef_depthEffect":       p.depthEffect,
        ]

        // Resonant frequencies as indexed params
        for (i, freq) in p.resonantFrequencies.enumerated() {
            params["reef_resonance\(i + 1)"] = freq
        }

        return params
    }

    // MARK: - Compare Mode

    /// Take a snapshot of the current profile as a baseline for comparison.
    func enterCompareMode() {
        compareBaseline = currentProfile
    }

    /// Clear the compare baseline.
    func exitCompareMode() {
        compareBaseline = nil
    }

    /// Whether compare mode is active
    var isInCompareMode: Bool { compareBaseline != nil }

    /// Difference between current and baseline profiles, for UI visualization.
    /// Positive = current is "larger" (more reverb, more space, etc.)
    func compareDeltas() -> ReefAcousticDelta? {
        guard let baseline = compareBaseline else { return nil }
        let current = currentProfile
        return ReefAcousticDelta(
            reverbTimeDelta:       current.reverbTime - baseline.reverbTime,
            reverbDampingDelta:    current.reverbDamping - baseline.reverbDamping,
            earlyReflectionsDelta: current.earlyReflections - baseline.earlyReflections,
            roomSizeDelta:         current.roomSize - baseline.roomSize,
            waterAbsorptionDelta:  current.waterAbsorption - baseline.waterAbsorption,
            ambientLevelDelta:     current.ambientLevel - baseline.ambientLevel,
            depthEffectDelta:      current.depthEffect - baseline.depthEffect
        )
    }

    // MARK: - Persistence

    func save() {
        if let data = try? JSONEncoder().encode(currentProfile) {
            UserDefaults.standard.set(data, forKey: "reefAcousticProfile")
        }
        if let data = try? JSONEncoder().encode(acousticHistory) {
            UserDefaults.standard.set(data, forKey: "reefAcousticHistory")
        }
    }

    func restore() {
        if let data = UserDefaults.standard.data(forKey: "reefAcousticProfile"),
           let decoded = try? JSONDecoder().decode(ReefAcousticProfile.self, from: data) {
            currentProfile = decoded
            targetProfile = decoded
            interpolationProgress = 1.0
        }
        if let data = UserDefaults.standard.data(forKey: "reefAcousticHistory"),
           let decoded = try? JSONDecoder().decode([AcousticHistoryEntry].self, from: data) {
            acousticHistory = Array(decoded.suffix(maxHistoryCount))
        }
    }
}

// MARK: - ReefAcousticDelta

/// The difference between two acoustic profiles — used by compare mode.
struct ReefAcousticDelta {
    let reverbTimeDelta: Float
    let reverbDampingDelta: Float
    let earlyReflectionsDelta: Float
    let roomSizeDelta: Float
    let waterAbsorptionDelta: Float
    let ambientLevelDelta: Float
    let depthEffectDelta: Float

    /// True if the current reef is acoustically "larger" than the baseline
    var isLarger: Bool { roomSizeDelta > 0.05 || reverbTimeDelta > 0.3 }

    /// True if the current reef is acoustically "warmer" than the baseline
    var isWarmer: Bool { reverbDampingDelta > 0.05 }
}

// MARK: - AcousticHistoryEntry

/// One time-stamped acoustic state record, for tracking the reef's evolution.
struct AcousticHistoryEntry: Codable {
    let date: Date
    let profile: ReefAcousticProfile
    let specimenCount: Int
}

// MARK: - Math Helpers

/// Smooth interpolation between two ReefAcousticProfiles
private func interpolate(from a: ReefAcousticProfile,
                          to b: ReefAcousticProfile,
                          t: Float) -> ReefAcousticProfile {
    func lerp(_ x: Float, _ y: Float) -> Float { x + (y - x) * t }

    let freqCount = max(a.resonantFrequencies.count, b.resonantFrequencies.count)
    var blendedFreqs: [Float] = []
    for i in 0..<freqCount {
        let aFreq = i < a.resonantFrequencies.count ? a.resonantFrequencies[i] : a.resonantFrequencies.last ?? 440
        let bFreq = i < b.resonantFrequencies.count ? b.resonantFrequencies[i] : b.resonantFrequencies.last ?? 440
        blendedFreqs.append(lerp(aFreq, bFreq))
    }

    return ReefAcousticProfile(
        reverbTime:          lerp(a.reverbTime, b.reverbTime),
        reverbDamping:       lerp(a.reverbDamping, b.reverbDamping),
        earlyReflections:    lerp(a.earlyReflections, b.earlyReflections),
        roomSize:            lerp(a.roomSize, b.roomSize),
        waterAbsorption:     lerp(a.waterAbsorption, b.waterAbsorption),
        resonantFrequencies: blendedFreqs,
        ambientLevel:        lerp(a.ambientLevel, b.ambientLevel),
        depthEffect:         lerp(a.depthEffect, b.depthEffect)
    )
}

/// Ken Perlin smoothstep for organic-feeling crossfades (no linear snap)
private func smoothstep(_ t: Float) -> Float {
    let clamped = t.clamped(to: 0...1)
    return clamped * clamped * (3 - 2 * clamped)
}

// MARK: - Float Extensions

private extension Float {
    func clamped(to range: ClosedRange<Float>) -> Float {
        Swift.max(range.lowerBound, Swift.min(range.upperBound, self))
    }

    /// Convenience for reef-specific 0-1 clamping
    var reefClamped: Float { clamped(to: 0...1) }
}
