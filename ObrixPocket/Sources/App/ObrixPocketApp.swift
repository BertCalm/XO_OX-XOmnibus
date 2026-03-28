import SwiftUI

@main
struct ObrixPocketApp: App {
    @StateObject private var audioEngine = AudioEngineManager()
    @StateObject private var reefStore = ReefStore()
    @StateObject private var firstLaunchManager = FirstLaunchManager()
    @Environment(\.scenePhase) var scenePhase

    @State private var importPreview: XOReefFile?
    @State private var showImportPreview = false

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(audioEngine)
                .environmentObject(reefStore)
                .environmentObject(firstLaunchManager)
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
                    HapticEngine.prepare()
                    audioEngine.reefStoreRef = reefStore
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
