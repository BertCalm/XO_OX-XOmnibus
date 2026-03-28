import Foundation

enum AppConstants {
    static let appName = "OBRIX Pocket"
    static let version = "1.0.0"
    static let build = 75
    static let tagline = "The reef remembers."
    static let developer = "XO_OX Designs"
    static let website = "https://XO-OX.org"

    // Game constants summary
    static let coreSpecimenCount = 16
    static let deepSpecimenCount = 8
    static let reefSlots = 16
    static let maxLevel = 10
    static let evolutionBranches = 32
    static let catchGameTypes = 4
    static let diveZones = 4
    static let maxEnergy = 100
    static let dailyEnergyCap = 50

    // Feature flags
    static let isNFCEnabled = false          // Phase 2
    static let isMultiplayerEnabled = false  // Phase 2
    static let isCloudSyncEnabled = false    // Phase 2
}
