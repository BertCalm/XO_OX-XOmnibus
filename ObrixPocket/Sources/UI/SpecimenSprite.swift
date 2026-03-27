import SwiftUI

// MARK: - SpecimenSprite

/// Renders the sprite for a caught specimen.
///
/// Looks up the image asset by the specimen's subtype ID (e.g. "polyblep-saw", "svf-lp").
/// If no asset is found, falls back to a category-colored circle with 3-letter initials.
///
/// Usage:
/// ```swift
/// SpecimenSprite(subtype: specimen.subtype, category: specimen.category, size: 80)
/// ```
struct SpecimenSprite: View {
    let subtype: String
    let category: SpecimenCategory
    let size: CGFloat

    var body: some View {
        if let uiImage = UIImage(named: subtype) {
            Image(uiImage: uiImage)
                .resizable()
                .interpolation(.none) // Pixel art — no smoothing
                .aspectRatio(contentMode: .fit)
                .frame(width: size, height: size)
        } else {
            // Fallback: category-colored circle with 3-letter initials
            ZStack {
                Circle()
                    .fill(categoryColor(for: category).opacity(0.2))
                    .frame(width: size, height: size)
                Text(String(subtype.prefix(3)).uppercased())
                    .font(.custom("JetBrainsMono-Bold", size: size * 0.3))
                    .foregroundColor(categoryColor(for: category))
            }
        }
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    VStack(spacing: 20) {
        // Known sprite — should show the Tuna image
        SpecimenSprite(subtype: "polyblep-saw", category: .source, size: 80)

        // Known sprite — should show the Anglerfish image
        SpecimenSprite(subtype: "dist-warm", category: .effect, size: 80)

        // Unknown subtype — should show fallback circle
        SpecimenSprite(subtype: "unknown-type", category: .modulator, size: 80)
    }
    .padding()
    .background(Color(hex: "080810"))
}
#endif
