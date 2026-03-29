import SwiftUI

/// Accessibility modifiers for OBRIX Pocket's custom views
extension View {
    /// Makes a specimen card accessible with role and stats
    func specimenAccessibility(
        name: String,
        subtype: String,
        generation: Int,
        level: Int,
        isFavorite: Bool
    ) -> some View {
        self
            .accessibilityElement(children: .ignore)
            .accessibilityLabel("\(name), \(subtype)")
            .accessibilityValue("Generation \(generation), Level \(level)")
            .accessibilityHint(isFavorite ? "Favorited specimen" : "Double tap to view details")
            .accessibilityAddTraits(.isButton)
    }

    /// Makes a parameter bar accessible with current value
    func parameterAccessibility(name: String, value: Float, range: ClosedRange<Float> = 0...1) -> some View {
        let percentage = Int(((value - range.lowerBound) / (range.upperBound - range.lowerBound)) * 100)
        return self
            .accessibilityElement(children: .ignore)
            .accessibilityLabel(name)
            .accessibilityValue("\(percentage) percent")
            .accessibilityAddTraits(.updatesFrequently)
    }

    /// Makes a ceremony overlay accessible
    func ceremonyAccessibility(title: String, narrative: String) -> some View {
        self
            .accessibilityElement(children: .ignore)
            .accessibilityLabel("Ceremony: \(title)")
            .accessibilityValue(narrative)
            .accessibilityHint("Tap to dismiss")
            .accessibilityAddTraits(.isModal)
    }

    /// Makes a reef slot accessible
    func reefSlotAccessibility(index: Int, specimenName: String?, isEmpty: Bool) -> some View {
        self
            .accessibilityLabel(isEmpty ? "Empty reef slot \(index + 1)" : "Reef slot \(index + 1): \(specimenName ?? "Unknown")")
            .accessibilityHint(isEmpty ? "Double tap to place a specimen" : "Double tap to interact")
    }

    /// Makes an achievement card accessible
    func achievementAccessibility(title: String, isCompleted: Bool, tier: String) -> some View {
        self
            .accessibilityElement(children: .ignore)
            .accessibilityLabel("\(tier) achievement: \(title)")
            .accessibilityValue(isCompleted ? "Completed" : "Not completed")
    }
}

/// Dynamic Type support — ensure minimum readable sizes
struct ScaledFont: ViewModifier {
    let name: String
    let size: CGFloat
    let relativeTo: Font.TextStyle

    func body(content: Content) -> some View {
        content.font(.custom(name, size: size, relativeTo: relativeTo))
    }
}

extension View {
    func scaledFont(_ name: String, size: CGFloat, relativeTo: Font.TextStyle = .body) -> some View {
        modifier(ScaledFont(name: name, size: size, relativeTo: relativeTo))
    }
}
