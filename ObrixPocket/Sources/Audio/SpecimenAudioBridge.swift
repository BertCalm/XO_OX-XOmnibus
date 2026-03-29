import Foundation

/// The bridge between OBRIX Pocket's game systems and the audio engine.
///
/// Collects modifiers from four sources, in priority order:
///   1. Genetic modifiers (dominant/recessive trait expression via `genome.asParameterModifiers()`)
///   2. Aging modifiers (LifeStage-based multipliers from `AgingModifiers.forStage`)
///   3. Sound memory influence (accumulated musical exposure via `SoundMemoryManager.getInfluence`)
///   4. Nursery parameter offsets (formation-period influence via `BreedingManager.nurseryOffsets`)
///
/// The final dictionary is consumed by the audio engine when a specimen is placed on
/// the reef or enters a Dive. HarmonicProfile is returned separately so DiveComposer
/// can configure HarmonicContext (scale/key selection) independently.
///
/// All injected managers are weak references — the bridge holds no ownership.
final class SpecimenAudioBridge {

    // MARK: - Injected Managers

    weak var geneticManager: GeneticManager?
    weak var soundMemoryManager: SoundMemoryManager?
    weak var harmonicProfileStore: HarmonicProfileStore?
    weak var reefAcousticsManager: ReefAcousticsManager?
    weak var breedingManager: BreedingManager?

    // MARK: - Init

    /// Designated initialiser. Pass all managers from the app's environment.
    init(
        geneticManager: GeneticManager,
        soundMemoryManager: SoundMemoryManager,
        harmonicProfileStore: HarmonicProfileStore,
        reefAcousticsManager: ReefAcousticsManager,
        breedingManager: BreedingManager
    ) {
        self.geneticManager = geneticManager
        self.soundMemoryManager = soundMemoryManager
        self.harmonicProfileStore = harmonicProfileStore
        self.reefAcousticsManager = reefAcousticsManager
        self.breedingManager = breedingManager
    }

    // MARK: - Compute Effective Parameters

    /// Gathers all modifiers for a specimen and returns a unified parameter dictionary.
    ///
    /// Called when a specimen is placed on the reef or enters a Dive.
    ///
    /// - Parameters:
    ///   - specimenId:  UUID that identifies this specimen across all subsystems
    ///   - subtypeId:   Subtype string (e.g. "polyblep-saw"), used as genome fallback
    ///   - lifeStage:   Current `LifeStage` (spawn/juvenile/adult/elder) from `SpecimenAge.stage`
    ///   - playAge:     `SpecimenAge` enum derived from total play seconds
    ///                  (used by SoundMemoryManager for saturation scaling)
    /// - Returns: Merged parameter dictionary to overlay on the specimen's base preset.
    func effectiveParameters(
        specimenId: UUID,
        subtypeId: String,
        lifeStage: LifeStage,
        playAge: SpecimenAge
    ) -> [String: Float] {
        var params: [String: Float] = [:]

        // 1. Genetic modifiers (dominant/recessive trait expression)
        if let gm = geneticManager {
            let genome = gm.genome(for: specimenId)
                ?? gm.createCaughtGenome(subtypeID: subtypeId)
            let geneticMods = genome.asParameterModifiers()
            for (key, value) in geneticMods {
                params[key] = (params[key] ?? 0) + value
            }
        }

        // 2. Aging modifiers — AgingModifiers.forStage returns a struct; use its apply() method
        //    to transform the params built so far (multiplies/offsets each known key).
        let agingMods = AgingModifiers.forStage(lifeStage)
        params = agingMods.apply(to: params)

        // 3. Sound memory influence (accumulated musical exposure)
        if let smm = soundMemoryManager {
            let influence = smm.getInfluence(for: specimenId)
            applyMemoryInfluence(influence, to: &params)
        }

        // 4. Nursery parameter offsets from formation period
        if let bm = breedingManager {
            let nurseryOffsets = bm.nurseryOffsets(for: specimenId)
            for (key, value) in nurseryOffsets {
                params[key] = (params[key] ?? 0) + value
            }
        }

        return params
    }

    // MARK: - Harmonic Profile Access

    /// Returns the harmonic profile for a specimen.
    /// Used by DiveComposer for scale/key selection.
    ///
    /// Derives from the genome if not yet cached. Falls back to the default
    /// major profile if the specimen has no genome registered.
    func harmonicProfile(for specimenId: UUID) -> HarmonicProfile {
        guard let store = harmonicProfileStore else {
            return HarmonicDNA.defaultProfile()
        }
        if let gm = geneticManager,
           let genome = gm.genome(for: specimenId) {
            return store.profile(for: genome)
        }
        return HarmonicDNA.defaultProfile()
    }

    /// Returns the blended harmonic profile for a set of active specimens.
    /// Used to configure HarmonicContext when multiple specimens play together.
    ///
    /// The blend is computed iteratively using `HarmonicCompatibility.harmonicBlend(a:b:)`.
    /// Dominant-gravity profiles win; tritone conflicts add tension affinity.
    func blendedHarmonicProfile(specimenIds: [UUID]) -> HarmonicProfile? {
        guard !specimenIds.isEmpty,
              let store = harmonicProfileStore,
              let gm = geneticManager
        else { return nil }

        // Resolve each specimen UUID → genome → profile, skipping unknowns
        let profiles: [HarmonicProfile] = specimenIds.compactMap { id in
            guard let genome = gm.genome(for: id) else { return nil }
            return store.profile(for: genome)
        }

        guard let first = profiles.first else { return nil }

        // Iteratively blend: dominant specimen's profile absorbs each successive one
        return profiles.dropFirst().reduce(first) { accumulated, next in
            HarmonicCompatibility.harmonicBlend(a: accumulated, b: next)
        }
    }

    // MARK: - Reef Acoustics

    /// Returns reef acoustic parameters (used by the audio engine for reverb/space).
    /// Delegates directly to ReefAcousticsManager.getAudioParameters().
    func reefAcousticParameters() -> [String: Float] {
        reefAcousticsManager?.getAudioParameters() ?? [:]
    }

    // MARK: - Private: Memory Influence Application

    /// Maps a MemoryInfluence to parameter offsets/multipliers and merges into params.
    ///
    /// Property names confirmed from SoundMemory.swift:
    ///   - `attackSharpnessDelta`   ±0.15 range; positive = brighter/sharper
    ///   - `sustainLengthDelta`     ±0.15 range; positive = longer sustain
    ///   - `velocityVariationDelta` ±0.15 range (not mapped to a single param; reserved)
    ///   - `swingDelta`             ±0.15 range (affects ArrangementEngine, not raw params)
    ///   - `registerBias`           ±0.15 range; maps to `obrix_src1Tune` offset in semitones
    private func applyMemoryInfluence(_ influence: MemoryInfluence, to params: inout [String: Float]) {
        // Attack sharpness → envelope attack time
        // Negative delta (darker exposure) = longer attack; positive = shorter
        if influence.attackSharpnessDelta != 0 {
            // ±0.15 attack delta → ±7.5% attack time multiplier (subtract: sharper = shorter)
            let attackMod = 1.0 - (influence.attackSharpnessDelta * 0.5)
            params["obrix_env1Attack"] = (params["obrix_env1Attack"] ?? 1.0) * attackMod
        }

        // Sustain length → envelope sustain + release multiplier
        if influence.sustainLengthDelta != 0 {
            let sustainMod = 1.0 + (influence.sustainLengthDelta * 0.5)
            params["obrix_env1Sustain"] = (params["obrix_env1Sustain"] ?? 1.0) * sustainMod
            params["obrix_env1Release"] = (params["obrix_env1Release"] ?? 1.0) * sustainMod
        }

        // Register bias → octave tuning offset on the primary oscillator
        // ±0.15 bias → ±1.8 semitones (subtle — never more than a whole step)
        if influence.registerBias != 0 {
            params["obrix_src1Tune"] = (params["obrix_src1Tune"] ?? 0) + influence.registerBias * 12.0
        }

        // velocityVariationDelta and swingDelta affect higher-level musical behaviour
        // (ArrangementEngine note velocity spread, DiveComposer swing timing) —
        // not raw obrix_ parameter IDs. They are exposed via dedicated accessors
        // on SoundMemoryManager when those systems query influence directly.
    }
}
