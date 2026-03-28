import UIKit

/// Visual color themes for the reef grid background.
enum ReefTheme: String, CaseIterable, Codable {
    case ocean    = "Ocean"     // Default dark blue-green
    case volcanic = "Volcanic"  // Dark red-orange
    case arctic   = "Arctic"    // Cool blue-white
    case deep     = "Deep"      // Near-black with bioluminescence
    case coral    = "Coral"     // Warm pink-orange

    /// The three gradient stop colors used in renderBackground().
    var gradientColors: [UIColor] {
        switch self {
        case .ocean:
            return [
                UIColor(red: 0.04, green: 0.06, blue: 0.12, alpha: 1),
                UIColor(red: 0.06, green: 0.10, blue: 0.14, alpha: 1),
                UIColor(red: 0.05, green: 0.08, blue: 0.11, alpha: 1)
            ]
        case .volcanic:
            return [
                UIColor(red: 0.12, green: 0.04, blue: 0.04, alpha: 1),
                UIColor(red: 0.14, green: 0.06, blue: 0.04, alpha: 1),
                UIColor(red: 0.10, green: 0.05, blue: 0.04, alpha: 1)
            ]
        case .arctic:
            return [
                UIColor(red: 0.06, green: 0.08, blue: 0.14, alpha: 1),
                UIColor(red: 0.08, green: 0.12, blue: 0.18, alpha: 1),
                UIColor(red: 0.06, green: 0.10, blue: 0.16, alpha: 1)
            ]
        case .deep:
            return [
                UIColor(red: 0.02, green: 0.02, blue: 0.04, alpha: 1),
                UIColor(red: 0.03, green: 0.03, blue: 0.06, alpha: 1),
                UIColor(red: 0.02, green: 0.02, blue: 0.05, alpha: 1)
            ]
        case .coral:
            return [
                UIColor(red: 0.10, green: 0.05, blue: 0.06, alpha: 1),
                UIColor(red: 0.14, green: 0.08, blue: 0.08, alpha: 1),
                UIColor(red: 0.10, green: 0.06, blue: 0.06, alpha: 1)
            ]
        }
    }
}
