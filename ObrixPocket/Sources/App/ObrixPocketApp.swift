import SwiftUI

@main
struct ObrixPocketApp: App {
    @StateObject private var audioEngine = AudioEngineManager()
    @StateObject private var reefStore = ReefStore()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(audioEngine)
                .environmentObject(reefStore)
                .onAppear {
                    audioEngine.start()
                }
        }
    }
}
