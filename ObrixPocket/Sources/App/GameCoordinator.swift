import Foundation
import Combine

/// Central coordinator that wires Act II game systems together.
/// Created once in ObrixPocketApp and injected as an environment object.
/// Routes game events to the appropriate managers.
final class GameCoordinator: ObservableObject {

    // MARK: - Owned Managers

    let reefAcousticsManager: ReefAcousticsManager
    let narrativeArcManager: NarrativeArcManager
    let audioBridge: SpecimenAudioBridge

    // MARK: - Injected References (owned elsewhere)

    weak var reefStore: ReefStore?
    weak var breedingManager: BreedingManager?
    weak var geneticManager: GeneticManager?
    weak var soundMemoryManager: SoundMemoryManager?
    weak var seasonalEventManager: SeasonalEventManager?

    // MARK: - Init

    init(
        geneticManager: GeneticManager,
        soundMemoryManager: SoundMemoryManager,
        harmonicProfileStore: HarmonicProfileStore,
        breedingManager: BreedingManager
    ) {
        self.reefAcousticsManager = ReefAcousticsManager()
        self.narrativeArcManager = NarrativeArcManager.shared

        self.audioBridge = SpecimenAudioBridge(
            geneticManager: geneticManager,
            soundMemoryManager: soundMemoryManager,
            harmonicProfileStore: harmonicProfileStore,
            reefAcousticsManager: self.reefAcousticsManager,
            breedingManager: breedingManager
        )

        self.geneticManager = geneticManager
        self.soundMemoryManager = soundMemoryManager
        self.breedingManager = breedingManager
    }

    // MARK: - Game Event Routing

    /// Call when a specimen is caught for the first time
    func onSpecimenCaught(subtypeId: String, isFirst: Bool) {
        narrativeArcManager.recordFirstCatch()
        if isFirst {
            LoreCodex.shared.checkCatchSpecimen(subtypeId: subtypeId)
        }
        AchievementManager.shared.onSpecimenCountChanged(
            reefStore?.specimens.compactMap { $0 }.count ?? 0
        )
    }

    /// Call when the player plays their first note
    func onFirstNote() {
        narrativeArcManager.recordFirstNote()
    }

    /// Call when two specimens are wired together
    func onSpecimensWired(typeDescription: String) {
        narrativeArcManager.recordFirstWire()
        narrativeArcManager.recordCouplingType(typeDescription)
    }

    /// Call when a dive completes
    func onDiveCompleted(score: Int, maxDepth: Int) {
        narrativeArcManager.recordDiveScore(score)
        AchievementManager.shared.onDiveHighScore(score)
        AchievementManager.shared.onDiveDepthReached(maxDepth)
    }

    /// Call when a biome/depth zone is discovered
    func onBiomeDiscovered(biome: String) {
        narrativeArcManager.recordBiomeDiscovery()
        LoreCodex.shared.checkBiome(biome: biome)
    }

    /// Call when a specimen reaches Elder status
    func onElderAchieved() {
        narrativeArcManager.recordElderAchieved()
    }

    /// Call when a streak is reached
    func onStreakReached(days: Int) {
        narrativeArcManager.recordStreakReached(days)
    }

    /// Call when an arrangement is created
    func onArrangementCreated() {
        narrativeArcManager.recordArrangementCreated()
    }

    /// Call when a rare specimen is caught
    func onRareCaught() {
        narrativeArcManager.recordRareCaught()
    }

    /// Call when the reef population changes (add/remove/breed)
    func onReefPopulationChanged() {
        guard let rs = reefStore else { return }
        let liveSpecimens = rs.specimens.compactMap { $0 }

        // Recalculate reef acoustics — convert live specimens to acoustic snapshots
        let acousticData: [SpecimenAcousticData] = liveSpecimens.map { spec in
            let ageDays = Calendar.current.dateComponents([.day],
                from: spec.catchTimestamp, to: Date()).day ?? 0
            let couplingCount = rs.couplingRoutes.filter {
                $0.sourceId == spec.id || $0.destId == spec.id
            }.count
            let role = SpecimenRoleMap.role(for: spec.subtype)
            let isInNursery = breedingManager?.nursery.contains {
                $0.parentSubtypeA == spec.subtype || $0.parentSubtypeB == spec.subtype
            } ?? false
            return SpecimenAcousticData(
                subtypeId: spec.subtype,
                ageDays: ageDays,
                couplingCount: couplingCount,
                musicalRole: role,
                isInNursery: isInNursery
            )
        }

        reefAcousticsManager.recalculate(
            specimens: acousticData,
            season: seasonalEventManager?.currentSeason,
            weather: nil
        )

        // Update narrative specimen count
        narrativeArcManager.recordSpecimenCount(liveSpecimens.count)

        // Prune orphaned sound memories
        let activeIds = Set(liveSpecimens.map { $0.id })
        soundMemoryManager?.pruneOrphanedMemories(activeSpecimenIDs: activeIds)
    }

    /// Get effective audio parameters for a specimen (the bridge call)
    func effectiveAudioParameters(
        specimenId: UUID,
        subtypeId: String,
        lifeStage: LifeStage,
        playAge: SpecimenAge
    ) -> [String: Float] {
        return audioBridge.effectiveParameters(
            specimenId: specimenId,
            subtypeId: subtypeId,
            lifeStage: lifeStage,
            playAge: playAge
        )
    }
}
