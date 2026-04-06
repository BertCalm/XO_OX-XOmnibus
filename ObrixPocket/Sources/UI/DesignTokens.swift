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

    /// Legendary Gold — pure gold, brighter + more saturated than xoGold; reserved for legendary rarity tier
    static let legendaryGold = Color(hex: "FFD700")

    /// Primary background — app canvas
    static let background = Color(hex: "0E0E10")

    /// Dark background — dive mode, modal underlays
    static let darkBackground = Color(hex: "0A0A0F")

    // MARK: - Category Colors

    /// Source (Shells) — oscillators
    static let sourceColor = Color(hex: "3380FF")

    /// Processor (Coral) — filters (coral-salmon, distinct from errorRed #FF4D4D)
    static let processorColor = Color(hex: "FF6B5E")

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
        case .legendary: return legendaryGold
        }
    }

    // MARK: - Semantic Aliases

    /// Error / alert red — destructive actions, health warnings
    static let errorRed = Color(hex: "FF4D4D")

    /// Muted text — secondary labels, timestamps, metadata
    static let mutedText = Color(hex: "8A8886")

    /// Streak orange — streak rewards, fire imagery
    static let streakOrange = Color(hex: "FF6B35")

    /// Panel background — drawers, overlays, param panels
    static let panelBackground = Color(hex: "141418")

    /// Deep accent — deep specimens section, locked content, evolution panel
    static let deepAccent = Color(hex: "7B5FD4")

    // MARK: - Cosmetic Tier Colors

    /// Bioluminescent cosmetic tier — teal glow
    static let cosmeticBioluminescent = Color(hex: "7FFFB2")

    /// Phantom cosmetic tier — soft purple
    static let cosmeticPhantom = Color(hex: "C8B8FF")

    /// Fossilized cosmetic tier — warm amber
    static let cosmeticFossilized = Color(hex: "C8A96E")

    /// Prismatic cosmetic tier — pink shimmer
    static let cosmeticPrismatic = Color(hex: "FF9EF7")

    /// Returns the display color for a cosmetic tier.
    static func cosmeticColor(_ tier: CosmeticTier) -> Color {
        switch tier {
        case .standard:       return .white.opacity(0.5)
        case .bioluminescent: return cosmeticBioluminescent
        case .phantom:        return cosmeticPhantom
        case .fossilized:     return cosmeticFossilized
        case .prismatic:      return cosmeticPrismatic
        }
    }

    // MARK: - Typography

    /// SpaceGrotesk-Bold — display headings
    /// The `relativeTo` parameter enables Dynamic Type scaling fleet-wide.
    static func heading(_ size: CGFloat, relativeTo style: Font.TextStyle = .headline) -> Font {
        .custom("SpaceGrotesk-Bold", size: size, relativeTo: style)
    }

    /// Inter-Regular — body copy, labels
    static func body(_ size: CGFloat, relativeTo style: Font.TextStyle = .body) -> Font {
        .custom("Inter-Regular", size: size, relativeTo: style)
    }

    /// Inter-Medium — semi-bold body text, CTA labels
    static func bodyMedium(_ size: CGFloat, relativeTo style: Font.TextStyle = .body) -> Font {
        .custom("Inter-Medium", size: size, relativeTo: style)
    }

    /// JetBrainsMono-Regular — numeric values, IDs, code
    static func mono(_ size: CGFloat, relativeTo style: Font.TextStyle = .caption) -> Font {
        .custom("JetBrainsMono-Regular", size: size, relativeTo: style)
    }

    /// JetBrainsMono-Bold — prominent numeric values
    static func monoBold(_ size: CGFloat, relativeTo style: Font.TextStyle = .caption) -> Font {
        .custom("JetBrainsMono-Bold", size: size, relativeTo: style)
    }

    // MARK: - Spacing Scale (4pt base grid)

    /// 2pt — hairline gaps, micro separators
    static let spacing2: CGFloat = 2
    /// 4pt — tight internal padding, small gaps
    static let spacing4: CGFloat = 4
    /// 6pt — compact padding
    static let spacing6: CGFloat = 6
    /// 8pt — standard small padding
    static let spacing8: CGFloat = 8
    /// 12pt — medium padding, section gaps
    static let spacing12: CGFloat = 12
    /// 16pt — standard content padding
    static let spacing16: CGFloat = 16
    /// 20pt — screen edge padding
    static let spacing20: CGFloat = 20
    /// 24pt — large section spacing
    static let spacing24: CGFloat = 24
    /// 40pt — hero spacing, large buttons
    static let spacing40: CGFloat = 40
}
