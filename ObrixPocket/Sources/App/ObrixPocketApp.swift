import SwiftUI

@main
struct ObrixPocketApp: App {
    @StateObject private var audioEngine = AudioEngineManager()
    @StateObject private var reefStore = ReefStore()
    @StateObject private var firstLaunchManager = FirstLaunchManager()
    @Environment(\.scenePhase) var scenePhase

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(audioEngine)
                .environmentObject(reefStore)
                .environmentObject(firstLaunchManager)
                .onAppear {
                    audioEngine.reefStoreRef = reefStore
                    audioEngine.start()
                    firstLaunchManager.resetDailyPlayIfNeeded()

                    if firstLaunchManager.isFirstLaunch {
                        // First launch: place starter specimen and mark launch complete
                        _ = firstLaunchManager.placeStarterSpecimen(in: reefStore)
                        reefStore.save()
                        firstLaunchManager.completeFirstLaunch()
                    } else {
                        // Returning user: restore persisted reef
                        reefStore.load()

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
                // Save on background, manage audio on foreground
                .onChange(of: scenePhase) { newPhase in
                    switch newPhase {
                    case .background:
                        reefStore.save()
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
}
