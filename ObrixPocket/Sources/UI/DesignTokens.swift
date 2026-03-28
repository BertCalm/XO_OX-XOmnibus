import SwiftUI
import SpriteKit

/// Single source of truth for all colors, fonts, and design constants.
/// All hardcoded hex strings in the project should reference these tokens.
enum DesignTokens {

    // MARK: - Brand

    /// Reef Jade — primary accent (OBRIX accent color, tint, CTA backgrounds)
    static let reefJade = Color(hex: "1E8B7E")

    /// XO Gold — macro strip, active states, reward text
    static let xoGold = Color(hex: "E9C46A")

    /// Primary background — app canvas
    static let background = Color(hex: "0E0E10")

    /// Dark background — dive mode, modal underlays
    static let darkBackground = Color(hex: "0A0A0F")

    // MARK: - Category Colors

    /// Source (Shells) — oscillators
    static let sourceColor = Color(hex: "3380FF")

    /// Processor (Coral) — filters
    static let processorColor = Color(hex: "FF4D4D")

    /// Modulator (Currents) — LFOs / envelopes
    static let modulatorColor = Color(hex: "4DCC4D")

    /// Effect (Tide Pools) — delay / chorus / reverb
    static let effectColor = Color(hex: "B34DFF")

    /// Returns the SwiftUI category color for a given specimen category.
    static func color(for category: SpecimenCategory) -> Color {
        switch category {
        case .source:    return sourceColor
        case .processor: return processorColor
        case .modulator: return modulatorColor
        case .effect:    return effectColor
        }
    }

    // MARK: - SpriteKit Versions

    /// Returns the SpriteKit category color for use in ReefScene nodes.
    static func skColor(for category: SpecimenCategory) -> SKColor {
        switch category {
        case .source:    return SKColor(red: 0.2,  green: 0.5, blue: 1.0, alpha: 1)
        case .processor: return SKColor(red: 1.0,  green: 0.3, blue: 0.3, alpha: 1)
        case .modulator: return SKColor(red: 0.3,  green: 0.8, blue: 0.3, alpha: 1)
        case .effect:    return SKColor(red: 0.7,  green: 0.3, blue: 1.0, alpha: 1)
        }
    }

    // MARK: - Rarity Colors

    /// Returns the display color for a rarity tier.
    static func rarityColor(_ rarity: SpecimenRarity) -> Color {
        switch rarity {
        case .common:    return .white.opacity(0.5)
        case .uncommon:  return .white.opacity(0.75)
        case .rare:      return xoGold
        case .legendary: return xoGold
        }
    }

    // MARK: - Typography

    /// SpaceGrotesk-Bold — display headings
    static func heading(_ size: CGFloat) -> Font {
        .custom("SpaceGrotesk-Bold", size: size)
    }

    /// Inter-Regular — body copy, labels
    static func body(_ size: CGFloat) -> Font {
        .custom("Inter-Regular", size: size)
    }

    /// JetBrainsMono-Regular — numeric values, IDs, code
    static func mono(_ size: CGFloat) -> Font {
        .custom("JetBrainsMono-Regular", size: size)
    }

    /// JetBrainsMono-Bold — prominent numeric values
    static func monoBold(_ size: CGFloat) -> Font {
        .custom("JetBrainsMono-Bold", size: size)
    }
}
