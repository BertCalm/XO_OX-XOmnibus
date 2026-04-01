import SwiftUI

@main
struct ObrixPocketApp: App {
    @StateObject private var audioEngine: AudioEngineManager
    @StateObject private var reefStore = ReefStore()
    @StateObject private var firstLaunchManager = FirstLaunchManager()
    @StateObject private var geneticManager: GeneticManager
    @StateObject private var soundMemoryManager: SoundMemoryManager
    @StateObject private var harmonicProfileStore: HarmonicProfileStore
    @StateObject private var breedingManager: BreedingManager
    @StateObject private var specimenAgingManager: SpecimenAgingManager
    @StateObject private var gameCoordinator: GameCoordinator
    @StateObject private var spawnManager = SpawnManager(biomeDetector: BiomeDetector())
    @Environment(\.scenePhase) var scenePhase

    @State private var importPreview: XOReefFile?
    @State private var showImportPreview = false

    init() {
        // Build the genetics/aging/memory graph in dependency order so all weak
        // back-references are wired before the first audio callback fires.
        let gm  = GeneticManager()
        let smm = SoundMemoryManager()
        let hps = HarmonicProfileStore()
        let bm  = BreedingManager()

        // Wire BreedingManager's optional weak references.
        bm.geneticManager       = gm
        bm.soundMemoryManager   = smm
        bm.harmonicProfileStore = hps

        let gc  = GameCoordinator(
            geneticManager:        gm,
            soundMemoryManager:    smm,
            harmonicProfileStore:  hps,
            breedingManager:       bm
        )
        let sam = SpecimenAgingManager()

        _audioEngine          = StateObject(wrappedValue: AudioEngineManager())
        _geneticManager       = StateObject(wrappedValue: gm)
        _soundMemoryManager   = StateObject(wrappedValue: smm)
        _harmonicProfileStore = StateObject(wrappedValue: hps)
        _breedingManager      = StateObject(wrappedValue: bm)
        _specimenAgingManager = StateObject(wrappedValue: sam)
        _gameCoordinator      = StateObject(wrappedValue: gc)

        // #382: Eagerly initialize singletons so UserDefaults state is loaded
        // before their owning views appear. Without this, lazy first-access inside
        // a view means the manager's init() (which calls load()) runs only when that
        // specific view is first rendered — any session where the user never visits
        // that view leaves the manager at default values, losing saved progress.
        _ = BadgeManager.shared
        _ = MasteryManager.shared
        _ = CookbookManager.shared
    }

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(audioEngine)
                .environmentObject(reefStore)
                .environmentObject(firstLaunchManager)
                .environmentObject(spawnManager)
                .onOpenURL { url in
                    handleIncomingReefFile(url)
                }
                .sheet(isPresented: $showImportPreview) {
                    if let reef = importPreview {
                        ImportPreviewView(reef: reef, onImport: {
                            XOReefImporter.importReef(reef, into: reefStore)
                            audioEngine.rebuildParamCache(reefStore: reefStore)
                            showImportPreview = false
                        }, onCancel: {
                            showImportPreview = false
                        })
                    }
                }
                .onAppear {
                    // Login milestone check runs on every launch so streak rewards
                    // are granted even if the user never visits the Catch tab.
                    spawnManager.checkLoginMilestone()

                    HapticEngine.prepare()
                    audioEngine.reefStoreRef = reefStore
                    // Wire the genetics/aging bridge so specimen traits reach the engine.
                    audioEngine.gameCoordinator      = gameCoordinator
                    audioEngine.specimenAgingManager = specimenAgingManager
                    // Let GameCoordinator resolve its own reef-store reference.
                    gameCoordinator.reefStore        = reefStore
                    audioEngine.start()
                    firstLaunchManager.resetDailyPlayIfNeeded()

                    // Request notification permission and schedule reef reminders
                    NotificationManager.shared.requestPermission()
                    NotificationManager.shared.resetDormancyTimer()
                    NotificationManager.shared.scheduleDailyEnergyReminder()
                    NotificationManager.shared.scheduleMusicCatchReminder()

                    if firstLaunchManager.isFirstLaunch {
                        // First launch: place starter specimen and mark launch complete
                        _ = firstLaunchManager.placeStarterSpecimen(in: reefStore)
                        reefStore.save()
                        firstLaunchManager.completeFirstLaunch()
                    } else {
                        // Returning user: restore persisted reef
                        reefStore.load()

                        // #378: Restore breeding pairs and nursery occupants persisted
                        // across app kills (pairs, nursery, parameter offsets, genome IDs).
                        breedingManager.restore()

                        // Apply health decay for days the app was closed
                        let daysSince = firstLaunchManager.daysSinceLastOpen
                        if daysSince > 0 {
                            reefStore.applyDormancyDecay(daysSinceLastOpen: daysSince)
                        }
                    }

                    // Push reef specimen parameters to OBRIX engine
                    audioEngine.applyReefConfiguration(reefStore)

                    firstLaunchManager.recordAppOpen()
                }
                // Save on background/inactive, manage audio on foreground
                .onChange(of: scenePhase) { newPhase in
                    switch newPhase {
                    case .inactive:
                        // Save immediately on inactive — covers sudden termination (e.g. swipe-kill,
                        // incoming call) before the process reaches .background or is killed outright.
                        reefStore.saveImmediately()
                        // #379: Persist breeding state on inactive so a swipe-kill doesn't
                        // discard in-progress breeding pairs or nursery occupants.
                        breedingManager.save()
                        CookbookManager.shared.save()
                    case .background:
                        // Force active notes off before saving so XP state is complete and the
                        // audio engine is clean on the next foreground restore (issue #443).
                        audioEngine.allNotesOff()
                        DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
                            reefStore.save()
                            // #379: Persist game manager state not covered by reefStore.save().
                            breedingManager.save()
                            CookbookManager.shared.save()
                        }
                        // Don't stop audio — reef ambient continues in background per spec (Section 5.2)
                        // UIBackgroundModes: audio in Info.plist permits this.
                        // The interruption handler manages stop/restart on phone calls, etc.
                    case .active:
                        audioEngine.start()
                    default:
                        break
                    }
                }
                // Place second specimen after the very first note is played
                .onChange(of: audioEngine.hasPlayedFirstNote) { played in
                    if played
                        && !firstLaunchManager.secondSpecimenPlaced
                    {
                        _ = firstLaunchManager.placeSecondSpecimen(in: reefStore)
                        reefStore.save()
                    }
                }
                // Show onboarding on first ever app open
                .fullScreenCover(isPresented: Binding(
                    get: { !firstLaunchManager.hasSeenIntro },
                    set: { if !$0 { firstLaunchManager.hasSeenIntro = true } }
                )) {
                    OnboardingView(onComplete: {
                        firstLaunchManager.hasSeenIntro = true
                    })
                }
        }
    }

    private func handleIncomingReefFile(_ url: URL) {
        guard url.pathExtension == "xoreef" else { return }
        guard let data = try? Data(contentsOf: url) else { return }
        guard let reef = XOReefImporter.parse(data: data) else { return }
        importPreview = reef
        showImportPreview = true
    }
}

// MARK: - Import Preview Sheet

struct ImportPreviewView: View {
    let reef: XOReefFile
    let onImport: () -> Void
    let onCancel: () -> Void

    var body: some View {
        VStack(spacing: 16) {
            Text("Import Reef")
                .font(DesignTokens.heading(20))
                .foregroundColor(.white)

            Text(reef.reefName)
                .font(DesignTokens.mono(14))
                .foregroundColor(DesignTokens.reefJade)

            let specCount = reef.specimens.compactMap { $0 }.count
            let routeCount = reef.routes.count

            Text("\(specCount) specimens · \(routeCount) wires · \(reef.totalDiveDepth)m depth")
                .font(DesignTokens.body(12))
                .foregroundColor(.white.opacity(0.4))

            Text("This will replace your current reef.")
                .font(DesignTokens.body(11))
                .foregroundColor(DesignTokens.errorRed.opacity(0.6))

            Button(action: onImport) {
                Text("IMPORT")
                    .font(DesignTokens.heading(16))
                    .tracking(2)
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity).frame(height: 50)
                    .background(RoundedRectangle(cornerRadius: 25).fill(DesignTokens.reefJade))
            }
            .padding(.horizontal, 40)

            Button("Cancel", action: onCancel)
                .foregroundColor(.white.opacity(0.3))
        }
        .padding(.vertical, 40)
        .background(DesignTokens.background.ignoresSafeArea())
    }
}
