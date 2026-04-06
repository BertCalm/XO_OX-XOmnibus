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
            // Deep blue ocean — cool, dark, expansive
            return [
                UIColor(red: 0.03, green: 0.06, blue: 0.15, alpha: 1),
                UIColor(red: 0.05, green: 0.10, blue: 0.20, alpha: 1),
                UIColor(red: 0.02, green: 0.04, blue: 0.10, alpha: 1)
            ]
        case .volcanic:
            // Warm amber-rust — thermal vents, magma glow
            return [
                UIColor(red: 0.18, green: 0.06, blue: 0.02, alpha: 1),
                UIColor(red: 0.22, green: 0.09, blue: 0.03, alpha: 1),
                UIColor(red: 0.12, green: 0.03, blue: 0.01, alpha: 1)
            ]
        case .arctic:
            // Pale ice-blue — cold, bright, crystalline
            return [
                UIColor(red: 0.08, green: 0.12, blue: 0.18, alpha: 1),
                UIColor(red: 0.12, green: 0.18, blue: 0.26, alpha: 1),
                UIColor(red: 0.05, green: 0.08, blue: 0.14, alpha: 1)
            ]
        case .deep:
            // True black with bioluminescent hints — abyssal
            return [
                UIColor(red: 0.02, green: 0.02, blue: 0.04, alpha: 1),
                UIColor(red: 0.03, green: 0.04, blue: 0.08, alpha: 1),
                UIColor(red: 0.01, green: 0.01, blue: 0.02, alpha: 1)
            ]
        case .coral:
            // Warm pink-orange — shallow reef, sunset light
            return [
                UIColor(red: 0.16, green: 0.06, blue: 0.06, alpha: 1),
                UIColor(red: 0.20, green: 0.09, blue: 0.07, alpha: 1),
                UIColor(red: 0.10, green: 0.04, blue: 0.03, alpha: 1)
            ]
        }
    }
}
