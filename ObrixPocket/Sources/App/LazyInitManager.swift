import Foundation

/// Defers initialization of non-critical managers until they're needed.
/// Critical path (audio, reef, specimens) loads immediately.
/// Non-critical (lore, achievements, ceremonies, community) loads lazily.
final class LazyInitManager {

    enum InitPhase: Int, Comparable {
        case immediate = 0   // Audio, Reef, Specimens
        case firstFrame = 1  // UI managers, themes
        case deferred = 2    // Lore, Achievements, Ceremonies, Community, Legendaries
        case background = 3  // Sound Memory pruning, migration checks

        static func < (lhs: InitPhase, rhs: InitPhase) -> Bool {
            lhs.rawValue < rhs.rawValue
        }
    }

    static let shared = LazyInitManager()

    private var completedPhases: Set<Int> = []

    /// Run deferred initialization after first frame renders
    func runDeferredInit(completion: @escaping () -> Void) {
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            // Phase 1: First frame managers
            // (Achievement, Narrative, Lore singletons access triggers lazy init)
            _ = AchievementManager.shared
            _ = LoreCodex.shared

            self.completedPhases.insert(InitPhase.firstFrame.rawValue)

            DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
                // Phase 2: Background work
                // - Sound memory pruning
                // - Persistence migration check
                // - Performance monitor start
                #if DEBUG
                PerformanceMonitor.shared.startMonitoring()
                #endif

                self.completedPhases.insert(InitPhase.deferred.rawValue)
                completion()
            }
        }
    }

    func hasCompleted(_ phase: InitPhase) -> Bool {
        completedPhases.contains(phase.rawValue)
    }
}
