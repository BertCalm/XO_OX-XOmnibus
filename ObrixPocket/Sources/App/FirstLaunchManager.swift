import Foundation

/// Manages first-launch state and the starter specimen flow
final class FirstLaunchManager: ObservableObject {
    @Published var isFirstLaunch: Bool
    @Published var hasSeenIntro = false
    @Published var starterSpecimenPlaced = false

    private let userDefaults = UserDefaults.standard
    private let firstLaunchKey = "obrix_pocket_first_launch_complete"
    private let secondSpecimenKey = "obrix_pocket_second_specimen_placed"

    var secondSpecimenPlaced: Bool {
        get { userDefaults.bool(forKey: secondSpecimenKey) }
        set { userDefaults.set(newValue, forKey: secondSpecimenKey) }
    }

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

    /// Create and place the second specimen (auto-spawned after first play) — always a Common Curtain
    func placeSecondSpecimen(in reefStore: ReefStore) -> Int? {
        let second = Specimen(
            id: UUID(),
            name: "Curtain",
            category: .processor,
            rarity: .common,
            health: 100,
            isPhantom: false,
            phantomScar: false,
            subtype: "svf-lp",
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
            creatureGenomeData: nil,
            cosmeticTier: .standard,
            morphIndex: 0,
            musicHash: nil,
            sourceTrackTitle: nil
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

    // MARK: - Daily Music Catch Limit

    private let lastMusicCatchKey = "obrix_pocket_last_music_catch_date"

    /// Returns true if the user has not yet caught a song today.
    var canMusicCatch: Bool {
        guard let lastCatch = userDefaults.object(forKey: lastMusicCatchKey) as? Date else {
            return true // Never caught — allow
        }
        return !Calendar.current.isDateInToday(lastCatch)
    }

    /// Records that the user has used their daily music catch (call after specimen is added to reef).
    func recordMusicCatch() {
        userDefaults.set(Date(), forKey: lastMusicCatchKey)
    }

    /// Returns the start of tomorrow if the catch has already been used today, nil otherwise.
    var nextMusicCatchDate: Date? {
        guard !canMusicCatch else { return nil }
        return Calendar.current.startOfDay(for: Date().addingTimeInterval(86_400))
    }
}
