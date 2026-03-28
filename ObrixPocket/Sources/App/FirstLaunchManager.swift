import Foundation

/// Manages first-launch state, the starter specimen flow, and the guided tutorial journey.
final class FirstLaunchManager: ObservableObject {
    @Published var isFirstLaunch: Bool
    @Published var hasSeenIntro = false
    @Published var starterSpecimenPlaced = false

    private let userDefaults = UserDefaults.standard
    private let firstLaunchKey = "obrix_pocket_first_launch_complete"
    private let secondSpecimenKey = "obrix_pocket_second_specimen_placed"
    private let journeyStepKey = "obrix_pocket_journey_step"

    var secondSpecimenPlaced: Bool {
        get { userDefaults.bool(forKey: secondSpecimenKey) }
        set { userDefaults.set(newValue, forKey: secondSpecimenKey) }
    }

    // MARK: - Guided Tutorial Journey

    /// Ordered subtype IDs for the 8-specimen starter journey (catalog format, lowercase kebab).
    ///
    /// Index 0 (Sawfin) and 1 (Curtain) are placed automatically via placeStarterSpecimen /
    /// placeSecondSpecimen. Indices 2-7 are the 6 guaranteed wild catches.
    private static let journeySequence: [String] = [
        "polyblep-saw",    // 0: starter — Sawfin (Source)    "This is your first voice."
        "svf-lp",          // 1: Curtain (Processor)          "This shapes the sound."
        "lfo-sine",        // 2: first wild catch — Tidepulse (Modulator)  "This makes it breathe."
        "delay-stereo",    // 3: second wild catch — Echocave (Effect)     "This gives it space."
        "polyblep-square", // 4: third wild catch — Boxjelly (Source)      "A different voice. Wire it in."
        "adsr-fast",       // 5: fourth wild catch — Snapper (Modulator)   "Fast and percussive."
        "shaper-hard",     // 6: fifth wild catch — Bonecrush (Processor)  "Distortion. Handle with care."
        "chorus-lush",     // 7: sixth wild catch — Shimmer (Effect)       "Everything sounds bigger now."
    ]

    /// Current journey step.
    /// - 0: initial state before starter is placed
    /// - 1: starter placed (Sawfin)
    /// - 2: second specimen placed (Curtain, after first note)
    /// - 3-8: wild catches 1-6 completed
    /// - 8+: journey complete — spawns become fully random
    var journeyStep: Int {
        get { userDefaults.integer(forKey: journeyStepKey) }
        set { userDefaults.set(newValue, forKey: journeyStepKey) }
    }

    var isJourneyComplete: Bool { journeyStep >= Self.journeySequence.count }

    /// The catalog subtype ID that should spawn next in the journey (nil if journey is complete).
    var nextJourneySubtype: String? {
        guard journeyStep < Self.journeySequence.count else { return nil }
        return Self.journeySequence[journeyStep]
    }

    /// The category for the next journey specimen (nil if journey is complete).
    var nextJourneyCategory: SpecimenCategory? {
        guard let subtype = nextJourneySubtype else { return nil }
        return SpecimenCatalog.entry(for: subtype)?.category
    }

    /// Advance the journey by one step. Call after each scripted specimen is obtained.
    func advanceJourney() {
        journeyStep += 1
    }

    init() {
        isFirstLaunch = !userDefaults.bool(forKey: firstLaunchKey)
    }

    func completeFirstLaunch() {
        userDefaults.set(true, forKey: firstLaunchKey)
        isFirstLaunch = false
    }

    /// Create and place the starter specimen.
    /// Advances journey to step 1 (Sawfin placed; Curtain is next).
    func placeStarterSpecimen(in reefStore: ReefStore) -> Int? {
        let starter = SpecimenFactory.createStarter()
        let index = reefStore.addSpecimen(starter)
        if index != nil {
            starterSpecimenPlaced = true
            journeyStep = 1
        }
        return index
    }

    /// Create and place the second specimen (auto-spawned after first play) — always a Common Curtain.
    /// Advances journey to step 2 (Curtain placed; first wild catch — Tidepulse — is next).
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
            sourceTrackTitle: nil,
            xp: 0,
            level: 1,
            aggressiveScore: 0,
            gentleScore: 0,
            totalPlaySeconds: 0
        )
        let index = reefStore.addSpecimen(second)
        if index != nil {
            secondSpecimenPlaced = true
            journeyStep = 2
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
