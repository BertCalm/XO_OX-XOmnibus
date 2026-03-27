import SwiftUI

@main
struct ObrixPocketApp: App {
    @StateObject private var audioEngine = AudioEngineManager()
    @StateObject private var reefStore = ReefStore()
    @StateObject private var firstLaunchManager = FirstLaunchManager()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(audioEngine)
                .environmentObject(reefStore)
                .environmentObject(firstLaunchManager)
                .onAppear {
                    audioEngine.start()
                }
        }
    }
}
