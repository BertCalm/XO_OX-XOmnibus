import SwiftUI

// MARK: - CookbookView

/// Displays all 12 resonant-pair recipe slots in a 3×4 grid.
/// Discovered recipes show parent icons, offspring, affinity badge, and discovery date.
/// Undiscovered slots show a locked silhouette with "???".
/// Tapping a discovered recipe opens RecipeDetailSheet.
///
/// Access via ProfileView / CollectionTab toolbar or as a standalone sheet.
struct CookbookView: View {

    @ObservedObject private var cookbook = CookbookManager.shared

    @State private var selectedRecipe: CouplingRecipe?
    @State private var showDetail = false

    // 3-column grid
    private let columns = [
        GridItem(.flexible(), spacing: DesignTokens.spacing12),
        GridItem(.flexible(), spacing: DesignTokens.spacing12),
        GridItem(.flexible(), spacing: DesignTokens.spacing12),
    ]

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: DesignTokens.spacing24) {
                    progressHeader
                    recipeGrid
                    if cookbook.isMasterAlchemist {
                        masterAlchemistBadge
                            .transition(.scale.combined(with: .opacity))
                    }
                    Spacer(minLength: DesignTokens.spacing40)
                }
                .padding(.horizontal, DesignTokens.spacing20)
                .padding(.top, DesignTokens.spacing16)
            }
            .background(DesignTokens.background.ignoresSafeArea())
            .navigationTitle("Coupling Cookbook")
            .navigationBarTitleDisplayMode(.large)
            .preferredColorScheme(.dark)
        }
        .sheet(isPresented: $showDetail) {
            if let recipe = selectedRecipe {
                RecipeDetailSheet(recipe: recipe) { id, name in
                    cookbook.nameRecipe(id: id, name: name)
                }
            }
        }
    }

    // MARK: - Progress Header

    private var progressHeader: some View {
        VStack(spacing: DesignTokens.spacing12) {

            // Count label
            HStack {
                VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                    Text("RESONANT PAIRS")
                        .font(DesignTokens.mono(9))
                        .tracking(1.5)
                        .foregroundColor(.white.opacity(0.25))
                    Text("\(cookbook.resonantPairsDiscovered) / \(CouplingRecipe.resonantPairCount)")
                        .font(DesignTokens.monoBold(28))
                        .foregroundColor(.white)
                }
                Spacer()
                if cookbook.resonantPairsDiscovered > 0 {
                    Text("\(Int(cookbook.masterAlchemistProgress * 100))%")
                        .font(DesignTokens.monoBold(18))
                        .foregroundColor(DesignTokens.reefJade)
                }
            }

            // Progress bar
            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 3)
                        .fill(Color.white.opacity(0.07))
                        .frame(height: 6)
                    RoundedRectangle(cornerRadius: 3)
                        .fill(
                            cookbook.isMasterAlchemist
                            ? DesignTokens.xoGold
                            : DesignTokens.reefJade
                        )
                        .frame(
                            width: geo.size.width * CGFloat(cookbook.masterAlchemistProgress),
                            height: 6
                        )
                        .animation(.easeInOut(duration: 0.4), value: cookbook.resonantPairsDiscovered)
                }
            }
            .frame(height: 6)

            // Sub-label
            Text(cookbook.isMasterAlchemist
                 ? "All resonant pairs discovered — Master Alchemist unlocked"
                 : "\(CouplingRecipe.resonantPairCount - cookbook.resonantPairsDiscovered) pairs remaining to unlock Master Alchemist"
            )
            .font(DesignTokens.body(11))
            .foregroundColor(.white.opacity(0.35))
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding(DesignTokens.spacing16)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(Color.white.opacity(0.04))
                .overlay(
                    RoundedRectangle(cornerRadius: 12)
                        .strokeBorder(Color.white.opacity(0.07), lineWidth: 1)
                )
        )
    }

    // MARK: - Recipe Grid

    private var recipeGrid: some View {
        LazyVGrid(columns: columns, spacing: DesignTokens.spacing12) {
            ForEach(0 ..< CouplingRecipe.resonantPairCount, id: \.self) { index in
                let recipe = cookbook.discoveredResonantRecipes.indices.contains(index)
                    ? cookbook.discoveredResonantRecipes[index]
                    : nil

                RecipeSlot(recipe: recipe, slotIndex: index)
                    .onTapGesture {
                        guard let r = recipe else { return }
                        selectedRecipe = r
                        showDetail = true
                    }
            }
        }
    }

    // MARK: - Master Alchemist Badge

    private var masterAlchemistBadge: some View {
        VStack(spacing: DesignTokens.spacing12) {
            ZStack {
                Circle()
                    .fill(DesignTokens.xoGold.opacity(0.12))
                    .frame(width: 72, height: 72)
                    .overlay(
                        Circle()
                            .strokeBorder(DesignTokens.xoGold.opacity(0.4), lineWidth: 1.5)
                    )
                Image(systemName: "flask.fill")
                    .font(.system(size: 28, weight: .semibold))
                    .foregroundColor(DesignTokens.xoGold)
                    .shadow(color: DesignTokens.xoGold.opacity(0.5), radius: 6)
            }

            Text("Master Alchemist")
                .font(DesignTokens.heading(18))
                .foregroundColor(DesignTokens.xoGold)
                .shadow(color: DesignTokens.xoGold.opacity(0.4), radius: 4)

            Text("All 12 resonant coupling pairs discovered.\nYou understand the deep grammar of the reef.")
                .font(DesignTokens.body(12))
                .foregroundColor(.white.opacity(0.45))
                .multilineTextAlignment(.center)
                .lineSpacing(3)
        }
        .padding(DesignTokens.spacing24)
        .frame(maxWidth: .infinity)
        .background(
            RoundedRectangle(cornerRadius: 16)
                .fill(DesignTokens.xoGold.opacity(0.06))
                .overlay(
                    RoundedRectangle(cornerRadius: 16)
                        .strokeBorder(DesignTokens.xoGold.opacity(0.22), lineWidth: 1)
                )
        )
    }
}

// MARK: - RecipeSlot

/// One cell in the 3×4 grid. Shows a discovered recipe or a locked placeholder.
private struct RecipeSlot: View {
    let recipe: CouplingRecipe?
    let slotIndex: Int

    private var isDiscovered: Bool { recipe != nil }

    var body: some View {
        ZStack {
            if let r = recipe {
                discoveredContent(r)
            } else {
                lockedContent
            }
        }
        .frame(minHeight: 130)
        .background(
            RoundedRectangle(cornerRadius: 12)
                .fill(isDiscovered
                    ? Color.white.opacity(0.05)
                    : Color.white.opacity(0.025)
                )
                .overlay(
                    RoundedRectangle(cornerRadius: 12)
                        .strokeBorder(
                            isDiscovered
                                ? affinityColor(recipe?.affinityTier).opacity(0.28)
                                : Color.white.opacity(0.06),
                            lineWidth: 1
                        )
                )
        )
    }

    // MARK: - Discovered

    private func discoveredContent(_ r: CouplingRecipe) -> some View {
        VStack(spacing: DesignTokens.spacing6) {

            // Affinity badge (top-right)
            HStack {
                Spacer()
                affinityBadge(r.affinityTier)
            }
            .padding(.horizontal, DesignTokens.spacing8)
            .padding(.top, DesignTokens.spacing8)

            // Parent icons with + between them
            HStack(spacing: DesignTokens.spacing4) {
                specimenIcon(subtype: r.parent1Subtype, size: 30)
                Text("+")
                    .font(DesignTokens.mono(10))
                    .foregroundColor(.white.opacity(0.3))
                specimenIcon(subtype: r.parent2Subtype, size: 30)
            }

            // Arrow
            Image(systemName: "arrow.down")
                .font(.system(size: 9, weight: .semibold))
                .foregroundColor(.white.opacity(0.2))

            // Offspring icon
            specimenIcon(subtype: r.offspringSubtype, size: 28)

            // Community name or offspring name
            let label = r.communityName
                ?? SpecimenCatalog.entry(for: r.offspringSubtype)?.creatureName
                ?? r.offspringSubtype
            Text(label)
                .font(DesignTokens.mono(9))
                .foregroundColor(.white.opacity(0.55))
                .lineLimit(1)
                .minimumScaleFactor(0.7)
                .padding(.horizontal, DesignTokens.spacing6)

            // Discovery date
            Text(shortDate(r.discoveredDate))
                .font(DesignTokens.mono(8))
                .foregroundColor(.white.opacity(0.22))
                .padding(.bottom, DesignTokens.spacing8)
        }
    }

    // MARK: - Locked

    private var lockedContent: some View {
        VStack(spacing: DesignTokens.spacing8) {
            Spacer()

            // Locked silhouette icon
            ZStack {
                Circle()
                    .fill(Color.white.opacity(0.04))
                    .frame(width: 44, height: 44)
                Image(systemName: "lock.fill")
                    .font(.system(size: 16, weight: .semibold))
                    .foregroundColor(.white.opacity(0.14))
            }

            Text("???")
                .font(DesignTokens.monoBold(13))
                .foregroundColor(.white.opacity(0.12))
                .tracking(2)

            Text("Undiscovered")
                .font(DesignTokens.mono(8))
                .foregroundColor(.white.opacity(0.12))

            Spacer()
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, DesignTokens.spacing8)
    }

    // MARK: - Helpers

    private func specimenIcon(subtype: String, size: CGFloat) -> some View {
        let category = SpecimenCatalog.entry(for: subtype)?.category ?? .source
        return SpecimenSprite(subtype: subtype, category: category, size: size)
    }

    private func affinityBadge(_ tier: CouplingRecipe.AffinityTier?) -> some View {
        let t = tier ?? .low
        return Text(t.displayLabel.uppercased())
            .font(DesignTokens.mono(7))
            .tracking(0.8)
            .foregroundColor(affinityColor(tier))
            .padding(.horizontal, 5)
            .padding(.vertical, 2)
            .background(
                RoundedRectangle(cornerRadius: 3)
                    .fill(affinityColor(tier).opacity(0.12))
            )
    }

    private func affinityColor(_ tier: CouplingRecipe.AffinityTier?) -> Color {
        switch tier {
        case .high:    return DesignTokens.reefJade
        case .medium:  return DesignTokens.xoGold
        case .low:     return DesignTokens.mutedText
        case nil:      return DesignTokens.mutedText
        }
    }

    private func shortDate(_ date: Date) -> String {
        let f = DateFormatter()
        f.dateStyle = .short
        f.timeStyle = .none
        return f.string(from: date)
    }
}

// MARK: - RecipeDetailSheet

/// Full-screen detail view for a single discovered coupling recipe.
/// Shows parent info, offspring stats, affinity explanation, and a
/// community name editor.
struct RecipeDetailSheet: View {

    let recipe: CouplingRecipe
    let onName: (UUID, String) -> Void

    @Environment(\.dismiss) private var dismiss
    @State private var nameInput: String = ""
    @State private var isEditingName = false

    private var parent1Entry: CatalogEntry? { SpecimenCatalog.entry(for: recipe.parent1Subtype) }
    private var parent2Entry: CatalogEntry? { SpecimenCatalog.entry(for: recipe.parent2Subtype) }
    private var offspringEntry: CatalogEntry? { SpecimenCatalog.entry(for: recipe.offspringSubtype) }

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: DesignTokens.spacing24) {
                    pairingHeader
                    affinityPanel
                    offspringPanel
                    communityNamePanel
                }
                .padding(.horizontal, DesignTokens.spacing20)
                .padding(.vertical, DesignTokens.spacing24)
            }
            .background(DesignTokens.panelBackground.ignoresSafeArea())
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Close") { dismiss() }
                        .font(DesignTokens.bodyMedium(14))
                        .foregroundColor(.white.opacity(0.5))
                }
            }
        }
        .preferredColorScheme(.dark)
        .onAppear {
            nameInput = recipe.communityName ?? ""
        }
    }

    // MARK: - Pairing Header

    private var pairingHeader: some View {
        VStack(spacing: DesignTokens.spacing16) {

            // Title
            Text(recipe.communityName ?? "Unnamed Recipe")
                .font(DesignTokens.heading(22))
                .foregroundColor(.white)
                .multilineTextAlignment(.center)

            // Resonant badge if applicable
            if recipe.isResonantPair {
                HStack(spacing: DesignTokens.spacing4) {
                    Image(systemName: "sparkles")
                        .font(.system(size: 10, weight: .semibold))
                    Text("Resonant Pair")
                        .font(DesignTokens.monoBold(11))
                        .tracking(0.8)
                }
                .foregroundColor(DesignTokens.xoGold)
                .padding(.horizontal, DesignTokens.spacing12)
                .padding(.vertical, DesignTokens.spacing4)
                .background(
                    Capsule()
                        .fill(DesignTokens.xoGold.opacity(0.12))
                        .overlay(
                            Capsule()
                                .strokeBorder(DesignTokens.xoGold.opacity(0.3), lineWidth: 0.5)
                        )
                )
            }

            // Parent icons
            HStack(spacing: DesignTokens.spacing20) {
                parentTile(entry: parent1Entry, subtype: recipe.parent1Subtype)

                VStack(spacing: DesignTokens.spacing4) {
                    Image(systemName: "link")
                        .font(.system(size: 12, weight: .semibold))
                        .foregroundColor(.white.opacity(0.25))
                    Text("×")
                        .font(DesignTokens.mono(14))
                        .foregroundColor(.white.opacity(0.2))
                }

                parentTile(entry: parent2Entry, subtype: recipe.parent2Subtype)
            }

            // Discovery date
            Text("Discovered \(formattedDate(recipe.discoveredDate))")
                .font(DesignTokens.body(11))
                .foregroundColor(.white.opacity(0.3))
        }
    }

    private func parentTile(entry: CatalogEntry?, subtype: String) -> some View {
        let category = entry?.category ?? .source
        return VStack(spacing: DesignTokens.spacing6) {
            SpecimenSprite(subtype: subtype, category: category, size: 52)
            Text(entry?.creatureName ?? subtype)
                .font(DesignTokens.bodyMedium(11))
                .foregroundColor(.white.opacity(0.75))
            Text(entry?.sonicCharacter ?? "")
                .font(DesignTokens.body(9))
                .foregroundColor(.white.opacity(0.35))
                .multilineTextAlignment(.center)
                .lineLimit(2)
                .frame(width: 90)
        }
    }

    // MARK: - Affinity Panel

    private var affinityPanel: some View {
        VStack(spacing: DesignTokens.spacing12) {
            sectionLabel("COUPLING AFFINITY")

            HStack(spacing: DesignTokens.spacing16) {
                // Tier badge
                VStack(spacing: DesignTokens.spacing4) {
                    Text(recipe.affinityTier.displayLabel.uppercased())
                        .font(DesignTokens.monoBold(18))
                        .foregroundColor(affinityColor)
                    Text("Affinity")
                        .font(DesignTokens.mono(9))
                        .tracking(1)
                        .foregroundColor(.white.opacity(0.25))
                }
                .frame(maxWidth: .infinity)

                Divider()
                    .background(Color.white.opacity(0.07))
                    .frame(height: 40)

                // XP bonus
                VStack(spacing: DesignTokens.spacing4) {
                    Text(recipe.affinityTier == .high ? "+2 XP" : "+0 XP")
                        .font(DesignTokens.monoBold(18))
                        .foregroundColor(recipe.affinityTier == .high ? DesignTokens.xoGold : .white.opacity(0.35))
                    Text("Per Note")
                        .font(DesignTokens.mono(9))
                        .tracking(1)
                        .foregroundColor(.white.opacity(0.25))
                }
                .frame(maxWidth: .infinity)

                Divider()
                    .background(Color.white.opacity(0.07))
                    .frame(height: 40)

                // Drift multiplier
                VStack(spacing: DesignTokens.spacing4) {
                    let mult = driftMultiplier
                    Text("\(String(format: "%.1f", mult))×")
                        .font(DesignTokens.monoBold(18))
                        .foregroundColor(.white.opacity(0.7))
                    Text("Drift")
                        .font(DesignTokens.mono(9))
                        .tracking(1)
                        .foregroundColor(.white.opacity(0.25))
                }
                .frame(maxWidth: .infinity)
            }
            .padding(DesignTokens.spacing16)
            .background(
                RoundedRectangle(cornerRadius: 10)
                    .fill(Color.white.opacity(0.04))
                    .overlay(
                        RoundedRectangle(cornerRadius: 10)
                            .strokeBorder(affinityColor.opacity(0.2), lineWidth: 1)
                    )
            )
        }
    }

    // MARK: - Offspring Panel

    private var offspringPanel: some View {
        VStack(spacing: DesignTokens.spacing12) {
            sectionLabel("OFFSPRING")

            if let entry = offspringEntry {
                HStack(spacing: DesignTokens.spacing16) {
                    let category = entry.category
                    SpecimenSprite(subtype: recipe.offspringSubtype, category: category, size: 60)

                    VStack(alignment: .leading, spacing: DesignTokens.spacing6) {
                        Text(entry.creatureName)
                            .font(DesignTokens.heading(16))
                            .foregroundColor(.white)

                        Text(entry.sonicCharacter)
                            .font(DesignTokens.body(11))
                            .foregroundColor(.white.opacity(0.5))
                            .lineLimit(2)
                            .lineSpacing(2)

                        if !entry.personalityLine.isEmpty {
                            Text("\u{201C}\(entry.personalityLine)\u{201D}")
                                .font(DesignTokens.body(11))
                                .foregroundColor(.white.opacity(0.55))
                                .italic()
                        }

                        // Category pill
                        Text(entry.category.rawValue.uppercased())
                            .font(DesignTokens.mono(9))
                            .tracking(1)
                            .foregroundColor(DesignTokens.color(for: entry.category))
                            .padding(.horizontal, DesignTokens.spacing6)
                            .padding(.vertical, 2)
                            .background(
                                RoundedRectangle(cornerRadius: 3)
                                    .fill(DesignTokens.color(for: entry.category).opacity(0.1))
                            )
                    }

                    Spacer()
                }
                .padding(DesignTokens.spacing16)
                .background(
                    RoundedRectangle(cornerRadius: 10)
                        .fill(Color.white.opacity(0.04))
                        .overlay(
                            RoundedRectangle(cornerRadius: 10)
                                .strokeBorder(Color.white.opacity(0.07), lineWidth: 1)
                        )
                )
            } else {
                Text(recipe.offspringSubtype)
                    .font(DesignTokens.mono(13))
                    .foregroundColor(.white.opacity(0.55))
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(DesignTokens.spacing16)
                    .background(
                        RoundedRectangle(cornerRadius: 10)
                            .fill(Color.white.opacity(0.04))
                    )
            }
        }
    }

    // MARK: - Community Name Panel

    private var communityNamePanel: some View {
        VStack(spacing: DesignTokens.spacing12) {
            sectionLabel("COMMUNITY NAME")

            VStack(spacing: DesignTokens.spacing8) {
                if isEditingName {
                    TextField("Name this recipe…", text: $nameInput)
                        .font(DesignTokens.bodyMedium(14))
                        .foregroundColor(.white)
                        .tint(DesignTokens.reefJade)
                        .padding(DesignTokens.spacing12)
                        .background(
                            RoundedRectangle(cornerRadius: 8)
                                .fill(Color.white.opacity(0.06))
                                .overlay(
                                    RoundedRectangle(cornerRadius: 8)
                                        .strokeBorder(DesignTokens.reefJade.opacity(0.4), lineWidth: 1)
                                )
                        )

                    HStack(spacing: DesignTokens.spacing8) {
                        Button("Cancel") {
                            nameInput = recipe.communityName ?? ""
                            isEditingName = false
                        }
                        .font(DesignTokens.bodyMedium(13))
                        .foregroundColor(.white.opacity(0.45))

                        Spacer()

                        Button("Save") {
                            onName(recipe.id, nameInput)
                            isEditingName = false
                        }
                        .font(DesignTokens.bodyMedium(13))
                        .foregroundColor(nameInput.trimmingCharacters(in: .whitespaces).isEmpty
                                        ? .white.opacity(0.2)
                                        : DesignTokens.reefJade)
                        .disabled(nameInput.trimmingCharacters(in: .whitespaces).isEmpty)
                    }

                } else {
                    HStack(spacing: DesignTokens.spacing12) {
                        VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                            if let name = recipe.communityName, !name.isEmpty {
                                Text(name)
                                    .font(DesignTokens.bodyMedium(15))
                                    .foregroundColor(.white.opacity(0.85))
                            } else {
                                Text("No name set")
                                    .font(DesignTokens.body(13))
                                    .foregroundColor(.white.opacity(0.25))
                                    .italic()
                            }
                            Text("Name your pairing discovery")
                                .font(DesignTokens.body(10))
                                .foregroundColor(.white.opacity(0.2))
                        }

                        Spacer()

                        Button(action: { isEditingName = true }) {
                            Image(systemName: "pencil")
                                .font(.system(size: 13, weight: .semibold))
                                .foregroundColor(DesignTokens.reefJade.opacity(0.75))
                                .padding(DesignTokens.spacing8)
                                .background(
                                    Circle()
                                        .fill(DesignTokens.reefJade.opacity(0.1))
                                )
                        }
                        .buttonStyle(.plain)
                    }
                    .padding(DesignTokens.spacing16)
                    .background(
                        RoundedRectangle(cornerRadius: 10)
                            .fill(Color.white.opacity(0.04))
                            .overlay(
                                RoundedRectangle(cornerRadius: 10)
                                    .strokeBorder(Color.white.opacity(0.07), lineWidth: 1)
                            )
                    )
                }
            }
        }
    }

    // MARK: - Helpers

    private func sectionLabel(_ text: String) -> some View {
        HStack {
            Text(text)
                .font(DesignTokens.mono(9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))
            Spacer()
        }
        .padding(.bottom, DesignTokens.spacing4)
    }

    private var affinityColor: Color {
        switch recipe.affinityTier {
        case .high:   return DesignTokens.reefJade
        case .medium: return DesignTokens.xoGold
        case .low:    return DesignTokens.mutedText
        }
    }

    private var driftMultiplier: Float {
        switch recipe.affinityTier {
        case .high:   return 2.0
        case .medium: return 1.0
        case .low:    return 0.5
        }
    }

    private func formattedDate(_ date: Date) -> String {
        let f = DateFormatter()
        f.dateStyle = .long
        f.timeStyle = .none
        return f.string(from: date)
    }
}

// MARK: - Preview

#if DEBUG
#Preview("Cookbook — partial progress") {
    CookbookView()
        .onAppear {
            CookbookManager.shared.recordDiscovery(parent1: "polyblep-saw", parent2: "svf-lp", offspring: "svf-lp", affinity: .high)
            CookbookManager.shared.recordDiscovery(parent1: "fm-basic", parent2: "shaper-hard", offspring: "shaper-hard", affinity: .high)
            CookbookManager.shared.recordDiscovery(parent1: "noise-white", parent2: "feedback", offspring: "polyblep-saw", affinity: .high)
            CookbookManager.shared.recordDiscovery(parent1: "lfo-sine", parent2: "delay-stereo", offspring: "lfo-sine", affinity: .medium)
        }
}

#Preview("Cookbook — Master Alchemist") {
    CookbookView()
        .onAppear {
            let pairs: [(String, String)] = [
                ("polyblep-saw", "svf-lp"), ("fm-basic", "shaper-hard"),
                ("noise-white", "feedback"), ("lfo-sine", "delay-stereo"),
                ("polyblep-square", "svf-bp"), ("adsr-fast", "chorus-lush"),
                ("vel-map", "reverb-hall"), ("lfo-random", "dist-warm"),
                ("polyblep-tri", "svf-hp"), ("noise-pink", "shaper-soft"),
                ("adsr-slow", "at-map"), ("wt-analog", "wt-vocal"),
            ]
            for (a, b) in pairs {
                CookbookManager.shared.recordDiscovery(parent1: a, parent2: b, offspring: a, affinity: .high)
            }
        }
}
#endif
