import SwiftUI

// Color(hex:) extension is defined in ContentView.swift (module-scoped)

// MARK: - Category Color Helper

/// Maps a specimen category to its canonical accent color.
func categoryColor(for category: SpecimenCategory) -> Color {
    switch category {
    case .source:    return Color(hex: "3380FF")
    case .processor: return Color(hex: "FF4D4D")
    case .modulator: return Color(hex: "4DCC4D")
    case .effect:    return Color(hex: "B34DFF")
    }
}

// MARK: - Cosmetic Tier Badge View

private struct CosmeticTierBadge: View {
    let tier: CosmeticTier

    var body: some View {
        Text(label.uppercased())
            .font(.custom("JetBrainsMono-Regular", size: 9))
            .foregroundColor(foregroundColor)
            .padding(.horizontal, 6)
            .padding(.vertical, 2)
            .background(
                RoundedRectangle(cornerRadius: 3)
                    .fill(badgeBackground)
            )
    }

    private var label: String {
        switch tier {
        case .standard:       return "Standard"
        case .bioluminescent: return "Bioluminescent"
        case .phantom:        return "Phantom"
        case .fossilized:     return "Fossilized"
        case .prismatic:      return "Prismatic"
        }
    }

    private var foregroundColor: Color {
        switch tier {
        case .standard:       return .white.opacity(0.5)
        case .bioluminescent: return Color(hex: "00FFFF")
        case .phantom:        return Color(hex: "C8C8D0").opacity(0.85)
        case .fossilized:     return Color(hex: "D4954A")
        case .prismatic:      return .white
        }
    }

    /// Background fill — prismatic uses a rainbow gradient shimmer effect.
    private var badgeBackground: AnyShapeStyle {
        switch tier {
        case .standard:
            return AnyShapeStyle(Color.white.opacity(0.06))
        case .bioluminescent:
            return AnyShapeStyle(Color(hex: "00FFFF").opacity(0.12))
        case .phantom:
            return AnyShapeStyle(Color.white.opacity(0.08))
        case .fossilized:
            return AnyShapeStyle(Color(hex: "D4954A").opacity(0.15))
        case .prismatic:
            // Rainbow gradient cycling through spectrum hues
            return AnyShapeStyle(
                LinearGradient(
                    colors: [
                        Color(hex: "FF4D4D"),
                        Color(hex: "FFB84D"),
                        Color(hex: "4DFF91"),
                        Color(hex: "4DCCFF"),
                        Color(hex: "B34DFF"),
                        Color(hex: "FF4DCC"),
                    ],
                    startPoint: .leading,
                    endPoint: .trailing
                )
                .opacity(0.7)
            )
        }
    }
}

// MARK: - Spectral Fingerprint Placeholder

/// A category-colored circle standing in for the full SpectralFingerprint renderer
/// when a compact card layout is needed.
private struct FingerprintPlaceholder: View {
    let color: Color
    let spectralDNA: [Float]
    let size: CGFloat

    var body: some View {
        ZStack {
            // Outer glow ring
            Circle()
                .fill(color.opacity(0.06))
                .frame(width: size + 20, height: size + 20)

            // Main disc
            Circle()
                .fill(color.opacity(0.12))
                .frame(width: size, height: size)
                .overlay(
                    Circle()
                        .strokeBorder(color.opacity(0.35), lineWidth: 1.5)
                )

            // Simplified radial hint using the spectralDNA average per quadrant
            SpectralFingerprint(
                spectralDNA: spectralDNA,
                color: color,
                size: size * 0.88
            )
        }
        .frame(width: size + 20, height: size + 20)
    }
}

// MARK: - Top Traits Row

private struct TopTraitsRow: View {
    let topThree: [(String, Int)]
    let color: Color

    var body: some View {
        let text = topThree
            .map { "\($0.0) \($0.1)" }
            .joined(separator: " · ")

        Text(text)
            .font(.custom("JetBrainsMono-Regular", size: 11))
            .foregroundColor(color)
            .lineLimit(2)
            .multilineTextAlignment(.center)
    }
}

// MARK: - CreatureCard

/// A self-contained card that displays a caught specimen's identity, spectral fingerprint,
/// top game traits, and an expandable full stats panel.
///
/// Usage:
/// ```swift
/// CreatureCard(specimen: mySpecimen)
/// CreatureCard(specimen: mySpecimen, showFullStats: true)
/// ```
struct CreatureCard: View {
    let specimen: Specimen
    var showFullStats: Bool = false

    @State private var isStatsExpanded: Bool = false

    // MARK: - Computed helpers

    private var color: Color {
        categoryColor(for: specimen.category)
    }

    private var stats: GameStats {
        GameStats.from(params: specimen.parameterState)
    }

    private var formattedCatchDate: String {
        let formatter = DateFormatter()
        formatter.dateStyle = .medium
        formatter.timeStyle = .none
        return formatter.string(from: specimen.catchTimestamp)
    }

    // MARK: - Body

    var body: some View {
        VStack(alignment: .center, spacing: 0) {
            topSection
            Divider()
                .background(Color.white.opacity(0.06))
                .padding(.vertical, 12)
            middleSection
            Divider()
                .background(Color.white.opacity(0.06))
                .padding(.vertical, 12)
            statsSection
            bottomSection
        }
        .padding(.horizontal, 20)
        .padding(.vertical, 20)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(Color(hex: "0E0E10"))
                .overlay(
                    RoundedRectangle(cornerRadius: 16)
                        .strokeBorder(color.opacity(0.18), lineWidth: 1)
                )
        )
        .onAppear {
            if showFullStats {
                isStatsExpanded = true
            }
        }
    }

    // MARK: - Top Section: name + rarity + cosmetic tier

    private var topSection: some View {
        VStack(spacing: 4) {
            Text(specimen.name)
                .font(.custom("SpaceGrotesk-Bold", size: 22))
                .foregroundColor(.white)
                .multilineTextAlignment(.center)

            Text(specimen.rarity.rawValue.uppercased())
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(Color(hex: "E9C46A"))
                .tracking(1.5)

            if specimen.cosmeticTier != .standard {
                CosmeticTierBadge(tier: specimen.cosmeticTier)
                    .padding(.top, 2)
            }
        }
    }

    // MARK: - Middle Section: spectral fingerprint + source track

    private var middleSection: some View {
        VStack(spacing: 8) {
            ZStack {
                // SpectralFingerprint as subtle background aura
                FingerprintPlaceholder(
                    color: color,
                    spectralDNA: specimen.spectralDNA,
                    size: 120
                )

                // Sprite overlaid on top of the fingerprint
                SpecimenSprite(subtype: specimen.subtype, category: specimen.category, size: 100)
            }

            if let trackTitle = specimen.sourceTrackTitle {
                Text("Born from: \(trackTitle)")
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.40))
                    .multilineTextAlignment(.center)
                    .lineLimit(2)
                    .padding(.horizontal, 8)
            }
        }
    }

    // MARK: - Stats Section: top traits + expandable full panel

    private var statsSection: some View {
        VStack(spacing: 10) {
            TopTraitsRow(topThree: stats.topThree, color: color)

            statsToggleButton

            if isStatsExpanded {
                StatsPanel(stats: stats, categoryColor: color)
                    .padding(.top, 4)
                    .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
        .animation(.easeInOut(duration: 0.22), value: isStatsExpanded)
    }

    private var statsToggleButton: some View {
        Button(action: { isStatsExpanded.toggle() }) {
            HStack(spacing: 4) {
                Text(isStatsExpanded ? "Hide Stats" : "View All Stats")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(color.opacity(0.75))
                Image(systemName: isStatsExpanded ? "chevron.up" : "chevron.down")
                    .font(.system(size: 9, weight: .semibold))
                    .foregroundColor(color.opacity(0.55))
            }
        }
        .buttonStyle(.plain)
    }

    // MARK: - Bottom Section: catch date

    private var bottomSection: some View {
        Text("Caught \(formattedCatchDate)")
            .font(.custom("Inter-Regular", size: 10))
            .foregroundColor(.white.opacity(0.30))
            .padding(.top, 12)
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    ScrollView {
        VStack(spacing: 20) {
            // Common source specimen
            CreatureCard(
                specimen: Specimen(
                    id: UUID(),
                    name: "Sawfin",
                    category: .source,
                    rarity: .common,
                    health: 82,
                    isPhantom: false,
                    phantomScar: false,
                    subtype: "polyblep-saw",
                    catchAccelPattern: [],
                    provenance: [],
                    spectralDNA: (0..<64).map { i in
                        0.4 + 0.5 * sin(Float(i) * 0.4) * cos(Float(i) * 0.15)
                    },
                    parameterState: [
                        "obrix_flt1Cutoff": 0.78,
                        "obrix_flt1Resonance": 0.35,
                        "obrix_env1Attack": 0.05,
                        "obrix_env1Decay": 0.4,
                        "obrix_env1Release": 0.5,
                        "obrix_src1Level": 0.9,
                        "obrix_src1Type": 1.0,
                        "obrix_lfo1Rate": 3.2,
                        "obrix_fx1Param1": 0.2,
                        "obrix_fx1Mix": 0.4,
                    ],
                    catchLatitude: 37.7749,
                    catchLongitude: -122.4194,
                    catchTimestamp: Date().addingTimeInterval(-86400 * 3),
                    catchWeatherDescription: "clear sky",
                    creatureGenomeData: nil,
                    cosmeticTier: .standard,
                    morphIndex: 0,
                    musicHash: nil,
                    sourceTrackTitle: nil
                )
            )

            // Legendary effect specimen, bioluminescent, born from music
            CreatureCard(
                specimen: Specimen(
                    id: UUID(),
                    name: "Abyssal Cathedral",
                    category: .effect,
                    rarity: .legendary,
                    health: 95,
                    isPhantom: false,
                    phantomScar: false,
                    subtype: "reverb-hall",
                    catchAccelPattern: [],
                    provenance: [],
                    spectralDNA: (0..<64).map { i in
                        0.5 + 0.45 * sin(Float(i) * 0.25)
                    },
                    parameterState: [
                        "obrix_flt1Cutoff": 0.55,
                        "obrix_flt1Resonance": 0.6,
                        "obrix_env1Attack": 0.3,
                        "obrix_env1Decay": 0.7,
                        "obrix_env1Release": 0.9,
                        "obrix_src1Level": 0.85,
                        "obrix_src1Type": 3.0,
                        "obrix_lfo1Rate": 0.8,
                        "obrix_fx1Param1": 0.65,
                        "obrix_fx1Mix": 0.8,
                    ],
                    catchLatitude: nil,
                    catchLongitude: nil,
                    catchTimestamp: Date().addingTimeInterval(-86400 * 10),
                    catchWeatherDescription: nil,
                    creatureGenomeData: nil,
                    cosmeticTier: .bioluminescent,
                    morphIndex: 0,
                    musicHash: "a3f9b12c45de67890abc",
                    sourceTrackTitle: "John Coltrane — A Love Supreme"
                ),
                showFullStats: true
            )

            // Rare modulator, prismatic cosmetic tier
            CreatureCard(
                specimen: Specimen(
                    id: UUID(),
                    name: "Spectral Tidepulse",
                    category: .modulator,
                    rarity: .rare,
                    health: 71,
                    isPhantom: false,
                    phantomScar: true,
                    subtype: "lfo-sine",
                    catchAccelPattern: [],
                    provenance: [],
                    spectralDNA: (0..<64).map { i in
                        abs(sin(Float(i) * 0.5))
                    },
                    parameterState: [
                        "obrix_flt1Cutoff": 0.42,
                        "obrix_flt1Resonance": 0.28,
                        "obrix_env1Attack": 0.15,
                        "obrix_env1Decay": 0.3,
                        "obrix_env1Release": 0.6,
                        "obrix_src1Level": 0.6,
                        "obrix_src1Type": 2.0,
                        "obrix_lfo1Rate": 5.5,
                        "obrix_fx1Param1": 0.45,
                        "obrix_fx1Mix": 0.55,
                    ],
                    catchLatitude: 51.5074,
                    catchLongitude: -0.1278,
                    catchTimestamp: Date(),
                    catchWeatherDescription: "light rain",
                    creatureGenomeData: nil,
                    cosmeticTier: .prismatic,
                    morphIndex: 1,
                    musicHash: nil,
                    sourceTrackTitle: "Boards of Canada — Roygbiv"
                )
            )
        }
        .padding()
    }
    .background(Color(hex: "080810"))
}
#endif
