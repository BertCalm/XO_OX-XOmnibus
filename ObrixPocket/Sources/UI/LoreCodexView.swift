import SwiftUI

/// Scrollable reading view for discovered lore fragments.
/// Book-like aesthetic — warm parchment for discovered text, mystery placeholders for locked entries.
struct LoreCodexView: View {

    @ObservedObject private var codex = LoreCodex.shared
    @State private var selectedCategory: LoreCategory? = nil
    @State private var newlyRevealedIDs: Set<String> = []

    // Category display order matches LoreCodex.readingOrder
    private let categoryOrder: [LoreCategory] = [
        .origins, .deepOcean, .theAncients, .specimens, .coupling, .theSong
    ]

    // Filtered categories based on sidebar selection
    private var displayedCategories: [LoreCategory] {
        if let sel = selectedCategory { return [sel] }
        return categoryOrder
    }

    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 0) {

                    // Completion header
                    completionHeader
                        .padding(.horizontal, DesignTokens.spacing20)
                        .padding(.top, DesignTokens.spacing16)
                        .padding(.bottom, DesignTokens.spacing24)

                    // Category filter pills
                    categoryPills
                        .padding(.bottom, DesignTokens.spacing16)

                    // Fragment sections
                    ForEach(displayedCategories, id: \.self) { category in
                        if let fragments = codex.fragmentsByCategory[category], !fragments.isEmpty {
                            sectionView(category: category, fragments: fragments)
                                .padding(.bottom, DesignTokens.spacing24)
                        }
                    }

                    Spacer(minLength: DesignTokens.spacing40)
                }
            }
            .background(DesignTokens.background)
            .navigationTitle("The Codex")
            .navigationBarTitleDisplayMode(.large)
        }
        .preferredColorScheme(.dark)
        .onAppear {
            // Snapshot which fragments are new so we can animate reveals
            let recent = codex.recentlyDiscovered.map { $0.id }
            newlyRevealedIDs = Set(recent)
            codex.clearRecentlyDiscovered()
        }
    }

    // MARK: - Completion Header

    private var completionHeader: some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing8) {
            HStack {
                Text("LORE DISCOVERED")
                    .font(DesignTokens.mono(9))
                    .tracking(2)
                    .foregroundColor(.white.opacity(0.2))
                Spacer()
                Text("\(Int(codex.loreCompletionPercentage))%")
                    .font(DesignTokens.monoBold(12))
                    .foregroundColor(DesignTokens.xoGold)
            }

            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 2)
                        .fill(Color.white.opacity(0.06))
                    RoundedRectangle(cornerRadius: 2)
                        .fill(DesignTokens.reefJade.opacity(0.7))
                        .frame(width: geo.size.width * CGFloat(codex.loreCompletionPercentage / 100))
                        .animation(.easeInOut(duration: 0.6), value: codex.loreCompletionPercentage)
                }
            }
            .frame(height: 3)

            Text("\(codex.discoveredFragments.count) of \(codex.fragments.count) fragments")
                .font(DesignTokens.body(11))
                .foregroundColor(.white.opacity(0.3))
        }
    }

    // MARK: - Category Pills

    private var categoryPills: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: DesignTokens.spacing8) {
                // "All" pill
                categoryPill(label: "All", symbol: "book.fill", isSelected: selectedCategory == nil) {
                    selectedCategory = nil
                }

                ForEach(categoryOrder, id: \.self) { cat in
                    let discovered = codex.discoveredByCategory[cat]?.count ?? 0
                    let total = codex.fragmentsByCategory[cat]?.count ?? 0
                    categoryPill(
                        label: cat.displayName,
                        symbol: cat.symbolName,
                        badge: "\(discovered)/\(total)",
                        isSelected: selectedCategory == cat
                    ) {
                        selectedCategory = (selectedCategory == cat) ? nil : cat
                    }
                }
            }
            .padding(.horizontal, DesignTokens.spacing20)
        }
    }

    @ViewBuilder
    private func categoryPill(
        label: String,
        symbol: String,
        badge: String? = nil,
        isSelected: Bool,
        action: @escaping () -> Void
    ) -> some View {
        Button(action: action) {
            HStack(spacing: DesignTokens.spacing4) {
                Image(systemName: symbol)
                    .font(.system(size: 10))
                Text(label)
                    .font(DesignTokens.bodyMedium(11))
                if let badge {
                    Text(badge)
                        .font(DesignTokens.mono(8))
                        .foregroundColor(.white.opacity(0.3))
                }
            }
            .foregroundColor(isSelected ? DesignTokens.background : .white.opacity(0.6))
            .padding(.horizontal, DesignTokens.spacing12)
            .padding(.vertical, DesignTokens.spacing6)
            .background(isSelected ? DesignTokens.reefJade : Color.white.opacity(0.06))
            .clipShape(Capsule())
        }
    }

    // MARK: - Section

    private func sectionView(category: LoreCategory, fragments: [LoreFragment]) -> some View {
        let discovered = fragments.filter { $0.isDiscovered }.count
        return VStack(alignment: .leading, spacing: 0) {
            // Section header
            HStack(spacing: DesignTokens.spacing8) {
                Image(systemName: category.symbolName)
                    .font(.system(size: 14))
                    .foregroundColor(DesignTokens.reefJade)
                Text(category.displayName.uppercased())
                    .font(DesignTokens.heading(13))
                    .tracking(1.5)
                    .foregroundColor(.white.opacity(0.8))
                Spacer()
                Text("\(discovered)/\(fragments.count)")
                    .font(DesignTokens.mono(10))
                    .foregroundColor(.white.opacity(0.3))
            }
            .padding(.horizontal, DesignTokens.spacing20)
            .padding(.bottom, DesignTokens.spacing12)

            VStack(spacing: DesignTokens.spacing12) {
                ForEach(fragments) { fragment in
                    fragmentCard(fragment: fragment)
                        .padding(.horizontal, DesignTokens.spacing20)
                }
            }
        }
    }

    // MARK: - Fragment Card

    @ViewBuilder
    private func fragmentCard(fragment: LoreFragment) -> some View {
        if fragment.isDiscovered {
            discoveredCard(fragment: fragment)
                .transition(.opacity.combined(with: .scale(scale: 0.97)))
        } else {
            undiscoveredCard(fragment: fragment)
        }
    }

    private func discoveredCard(fragment: LoreFragment) -> some View {
        let isNew = newlyRevealedIDs.contains(fragment.id)

        return VStack(alignment: .leading, spacing: DesignTokens.spacing8) {
            HStack(alignment: .top) {
                Text(fragment.title)
                    .font(DesignTokens.heading(14))
                    .foregroundColor(.white.opacity(0.9))
                Spacer()
                if isNew {
                    Text("NEW")
                        .font(DesignTokens.mono(8))
                        .tracking(1.5)
                        .foregroundColor(DesignTokens.reefJade)
                        .padding(.horizontal, DesignTokens.spacing6)
                        .padding(.vertical, 2)
                        .background(DesignTokens.reefJade.opacity(0.12))
                        .clipShape(Capsule())
                }
            }

            Text(fragment.text)
                .font(DesignTokens.body(13))
                .foregroundColor(Color(hex: "B8A98A").opacity(0.85))
                .lineSpacing(3)
                .fixedSize(horizontal: false, vertical: true)

            if let date = fragment.discoveredDate {
                Text("Discovered \(DateFormatter.localizedString(from: date, dateStyle: .medium, timeStyle: .none))")
                    .font(DesignTokens.mono(9))
                    .foregroundColor(.white.opacity(0.18))
                    .padding(.top, DesignTokens.spacing2)
            }
        }
        .padding(DesignTokens.spacing16)
        .background(Color(hex: "1A1610"))  // Warm parchment-dark background
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .stroke(isNew ? DesignTokens.reefJade.opacity(0.4) : Color(hex: "3A3020").opacity(0.8), lineWidth: 1)
        )
        .clipShape(RoundedRectangle(cornerRadius: 10))
    }

    private func undiscoveredCard(fragment: LoreFragment) -> some View {
        HStack(spacing: DesignTokens.spacing12) {
            Image(systemName: "lock.fill")
                .font(.system(size: 12))
                .foregroundColor(.white.opacity(0.12))
                .frame(width: 20)

            VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                Text("???")
                    .font(DesignTokens.heading(13))
                    .foregroundColor(.white.opacity(0.18))

                Text(discoveryHint(for: fragment.discoveryCondition))
                    .font(DesignTokens.body(10))
                    .foregroundColor(.white.opacity(0.2))
                    .italic()
            }

            Spacer()
        }
        .padding(DesignTokens.spacing12)
        .background(Color.white.opacity(0.02))
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .stroke(Color.white.opacity(0.05), lineWidth: 1)
        )
        .clipShape(RoundedRectangle(cornerRadius: 10))
    }

    // MARK: - Discovery Hint

    private func discoveryHint(for condition: DiscoveryCondition) -> String {
        switch condition {
        case .catchSpecimen:
            return "Catch a new specimen to unlock."
        case .breedCombination:
            return "A breeding combination holds the key."
        case .reachBiome(let biome):
            return "Explore the \(biome) zone to reveal."
        case .seasonFirst(let season):
            return "Experience \(seasonDisplayName(season)) for the first time."
        case .achievementUnlocked:
            return "Complete a specific achievement to unlock."
        case .diveDepth(let depth):
            return "Dive to \(depth)m depth."
        case .reefAge(let days):
            return "Tend your reef for \(days) days."
        }
    }

    private func seasonDisplayName(_ raw: String) -> String {
        switch raw {
        case "springBloom":  return "Spring Bloom"
        case "summerSurge":  return "Summer Surge"
        case "autumnDrift":  return "Autumn Drift"
        case "winterDeep":   return "Winter Deep"
        default:             return raw
        }
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    LoreCodexView()
        .background(DesignTokens.background.ignoresSafeArea())
}
#endif
