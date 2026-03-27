import Foundation

/// Manages first-launch state and the starter specimen flow
final class FirstLaunchManager: ObservableObject {
    @Published var isFirstLaunch: Bool
    @Published var hasSeenIntro = false
    @Published var starterSpecimenPlaced = false
    @Published var secondSpecimenPlaced = false

    private let userDefaults = UserDefaults.standard
    private let firstLaunchKey = "obrix_pocket_first_launch_complete"

    init() {
        isFirstLaunch = !userDefaults.bool(forKey: firstLaunchKey)
    }

    func completeFirstLaunch() {
        userDefaults.set(true, forKey: firstLaunchKey)
        isFirstLaunch = false
    }

    /// Create and place the starter specimen
    func placeStarterSpecimen(in reefStore: ReefStore) -> Int? {
        let starter = SpecimenFactory.createStarter()
        let index = reefStore.addSpecimen(starter)
        if index != nil {
            starterSpecimenPlaced = true
        }
        return index
    }

    /// Create and place the second specimen (auto-spawned after first play)
    func placeSecondSpecimen(in reefStore: ReefStore) -> Int? {
        let second = Specimen(
            id: UUID(),
            name: "Warm Coral",
            category: .processor,
            rarity: .common,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: "SVF-LP",
            catchAccelPattern: Array(repeating: 0, count: 16),
            provenance: [],
            spectralDNA: Array(repeating: 0.4, count: 64),
            parameterState: [
                "obrix_flt1Cutoff": 0.65,
                "obrix_flt1Resonance": 0.3,
                "obrix_flt1EnvDepth": 0.25,
            ],
            catchLatitude: nil,
            catchLongitude: nil,
            catchTimestamp: Date(),
            catchWeatherDescription: "First Launch",
            creatureGenomeData: nil
        )
        let index = reefStore.addSpecimen(second)
        if index != nil {
            secondSpecimenPlaced = true
        }
        return index
    }

    // MARK: - Login Tracking (for dormancy/health system)

    private let lastOpenKey = "obrix_pocket_last_open"

    func recordAppOpen() {
        userDefaults.set(Date(), forKey: lastOpenKey)
    }

    var daysSinceLastOpen: Int {
        guard let last = userDefaults.object(forKey: lastOpenKey) as? Date else { return 0 }
        return Calendar.current.dateComponents([.day], from: last, to: Date()).day ?? 0
    }
}
