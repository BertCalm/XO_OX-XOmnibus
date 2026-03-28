import SwiftUI

// MARK: - CollectionTab

/// Pokédex-style view of all specimen types.
/// Shows 16 core specimens (discoverable through normal spawns) and
/// 8 deep specimens (locked, showing unlock conditions).
struct CollectionTab: View {
    @EnvironmentObject var reefStore: ReefStore

    @State private var selectedSpecimen: Specimen?
    @State private var showingCard = false
    @State private var deepSectionExpanded = false
    @StateObject private var milestoneManager = MilestoneManager()

    // Compute the set of discovered subtype IDs once per render.
    // A type is "discovered" if found in the reef OR in the full collection.
    private var discoveredSubtypes: Set<String> {
        // Reef specimens (currently equipped)
        let reefSubtypes = reefStore.specimens
            .compactMap { $0?.subtype }

        // All specimens from persistent collection (reef + stasis + collection)
        let allSubtypes = reefStore.loadAllSpecimens().map { $0.subtype }

        return Set(reefSubtypes + allSubtypes)
    }

    private var discoveredCoreCount: Int {
        let subtypes = discoveredSubtypes
        return SpecimenCatalog.coreSpecimens.filter { subtypes.contains($0.subtypeID) }.count
    }

    private var discoveredDeepCount: Int {
        let subtypes = discoveredSubtypes
        return SpecimenCatalog.deepSpecimens.filter { subtypes.contains($0.subtypeID) }.count
    }

    /// Find the best matching Specimen for a catalog entry's subtypeID.
    /// Checks the live reef first, then falls back to the full persistent collection.
    private func specimen(for entry: CatalogEntry) -> Specimen? {
        reefStore.specimens.compactMap { $0 }.first { $0.subtype == entry.subtypeID }
            ?? reefStore.loadAllSpecimens().first { $0.subtype == entry.subtypeID }
    }

    // Split core entries by category (only core specimens for normal sections)
    private var coreSources: [CatalogEntry] {
        SpecimenCatalog.sources.filter { !$0.isDeepSpecimen }
    }
    private var coreProcessors: [CatalogEntry] {
        SpecimenCatalog.processors.filter { !$0.isDeepSpecimen }
    }
    private var coreModulators: [CatalogEntry] {
        SpecimenCatalog.modulators.filter { !$0.isDeepSpecimen }
    }
    private var coreEffects: [CatalogEntry] {
        SpecimenCatalog.effects.filter { !$0.isDeepSpecimen }
    }

    var body: some View {
        NavigationView {
            ZStack {
                Color(hex: "0E0E10")
                    .ignoresSafeArea()

                ScrollView {
                    VStack(alignment: .leading, spacing: 24) {
                        // Header
                        collectionHeader

                        let subtypes = discoveredSubtypes

                        // MARK: Core Sections
                        Text("CORE SPECIMENS")
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.35))
                            .tracking(1.5)

                        CollectionSection(
                            title: "Shells",
                            categoryColor: Color(hex: "3380FF"),
                            entries: coreSources,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in
                                selectedSpecimen = specimen(for: entry)
                                showingCard = selectedSpecimen != nil
                            }
                        )

                        CollectionSection(
                            title: "Coral",
                            categoryColor: Color(hex: "FF4D4D"),
                            entries: coreProcessors,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in
                                selectedSpecimen = specimen(for: entry)
                                showingCard = selectedSpecimen != nil
                            }
                        )

                        CollectionSection(
                            title: "Currents",
                            categoryColor: Color(hex: "4DCC4D"),
                            entries: coreModulators,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in
                                selectedSpecimen = specimen(for: entry)
                                showingCard = selectedSpecimen != nil
                            }
                        )

                        CollectionSection(
                            title: "Tide Pools",
                            categoryColor: Color(hex: "B34DFF"),
                            entries: coreEffects,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in
                                selectedSpecimen = specimen(for: entry)
                                showingCard = selectedSpecimen != nil
                            }
                        )

                        // MARK: Deep Specimens Section (collapsible)
                        deepSpecimensSection(discoveredSubtypes: subtypes)

                        // MARK: Milestones Section
                        milestonesSection
                    }
                    .padding(.horizontal, 16)
                    .padding(.bottom, 32)
                }
            }
            .navigationBarHidden(true)
            .sheet(isPresented: $showingCard) {
                if let specimen = selectedSpecimen {
                    ScrollView {
                        CreatureCard(specimen: specimen)
                            .padding(20)
                    }
                    .background(Color(hex: "0E0E10").ignoresSafeArea())
                }
            }
        }
        .navigationViewStyle(.stack)
    }

    // MARK: - Header

    private var collectionHeader: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack(alignment: .lastTextBaseline, spacing: 8) {
                Text("Collection")
                    .font(.custom("SpaceGrotesk-Bold", size: 20))
                    .foregroundColor(.white)
                Spacer()
            }
            HStack(spacing: 12) {
                // Core count
                HStack(spacing: 4) {
                    Text("Core")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(.white.opacity(0.4))
                    Text("\(discoveredCoreCount) / 16")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(Color(hex: "1E8B7E"))
                }
                // Deep count
                HStack(spacing: 4) {
                    Text("Deep")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(.white.opacity(0.4))
                    Text("\(discoveredDeepCount) / 8")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(Color(hex: "7B5FD4"))
                }
                Spacer()
            }
        }
        .padding(.top, 16)
    }

    // MARK: - Milestones Section

    private var milestonesSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("MILESTONES")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .tracking(1.5)
                    .foregroundColor(Color(hex: "E9C46A"))
                Spacer()
                Text("\(milestoneManager.unlockedCount)/\(milestoneManager.totalCount)")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.3))
            }
            .padding(.horizontal, 4)

            ForEach(milestoneManager.milestones) { milestone in
                HStack(spacing: 10) {
                    Image(systemName: milestone.icon)
                        .font(.system(size: 14))
                        .foregroundColor(milestone.unlocked ? Color(hex: "E9C46A") : .white.opacity(0.2))
                        .frame(width: 24)

                    VStack(alignment: .leading, spacing: 2) {
                        Text(milestone.title)
                            .font(.custom("SpaceGrotesk-Bold", size: 12))
                            .foregroundColor(milestone.unlocked ? .white : .white.opacity(0.5))
                        Text(milestone.description)
                            .font(.custom("Inter-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.3))
                    }

                    Spacer()

                    if milestone.unlocked {
                        Image(systemName: "checkmark.circle.fill")
                            .foregroundColor(Color(hex: "1E8B7E"))
                    } else {
                        Text("\(milestone.progress)/\(milestone.requirement)")
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.3))
                    }
                }
                .padding(.horizontal, 4)
                .padding(.vertical, 4)
            }
        }
    }

    // MARK: - Deep Specimens Section

    @ViewBuilder
    private func deepSpecimensSection(discoveredSubtypes: Set<String>) -> some View {
        VStack(alignment: .leading, spacing: 10) {
            // Collapsible header
            Button(action: {
                withAnimation(.easeInOut(duration: 0.2)) {
                    deepSectionExpanded.toggle()
                }
            }) {
                HStack(spacing: 8) {
                    Image(systemName: "lock.fill")
                        .font(.system(size: 9, weight: .medium))
                        .foregroundColor(Color(hex: "7B5FD4"))

                    Text("DEEP SPECIMENS")
                        .font(.custom("JetBrainsMono-Regular", size: 9))
                        .foregroundColor(Color(hex: "7B5FD4"))
                        .tracking(1.5)

                    Text("(\(discoveredDeepCount)/8 unlocked)")
                        .font(.custom("JetBrainsMono-Regular", size: 9))
                        .foregroundColor(.white.opacity(0.3))
                        .tracking(0.8)

                    Spacer()

                    Image(systemName: deepSectionExpanded ? "chevron.up" : "chevron.down")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.white.opacity(0.3))
                }
            }
            .buttonStyle(.plain)

            if deepSectionExpanded {
                DeepSpecimensGrid(
                    entries: SpecimenCatalog.deepSpecimens,
                    discoveredSubtypes: discoveredSubtypes,
                    onDiscoveredTap: { entry in
                        selectedSpecimen = specimen(for: entry)
                        showingCard = selectedSpecimen != nil
                    }
                )
                .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
    }
}

// MARK: - CollectionSection

/// A single category section with a colored header and 4-column card grid.
private struct CollectionSection: View {
    let title: String
    let categoryColor: Color
    let entries: [CatalogEntry]
    let discoveredSubtypes: Set<String>
    /// Called when a discovered card is tapped. Passes the tapped entry for specimen lookup.
    var onDiscoveredTap: ((CatalogEntry) -> Void)? = nil

    private let columns = Array(repeating: GridItem(.flexible(), spacing: 8), count: 4)

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            // Section header
            Text(title.uppercased())
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(categoryColor)
                .tracking(1.2)

            LazyVGrid(columns: columns, spacing: 8) {
                ForEach(entries, id: \.subtypeID) { entry in
                    let isDiscovered = discoveredSubtypes.contains(entry.subtypeID)
                    CollectionCard(
                        entry: entry,
                        discovered: isDiscovered
                    )
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.white.opacity(0.03))
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .strokeBorder(
                                        isDiscovered
                                            ? categoryColor.opacity(0.30)
                                            : Color.white.opacity(0.06),
                                        lineWidth: 1
                                    )
                            )
                    )
                    .onTapGesture {
                        guard isDiscovered else { return }
                        onDiscoveredTap?(entry)
                    }
                }
            }
        }
    }
}

// MARK: - DeepSpecimensGrid

/// Grid showing all 8 deep specimens. Unlocked ones show normally;
/// locked ones show a lock icon and unlock condition instead of "???".
private struct DeepSpecimensGrid: View {
    let entries: [CatalogEntry]
    let discoveredSubtypes: Set<String>
    var onDiscoveredTap: ((CatalogEntry) -> Void)? = nil

    private let columns = Array(repeating: GridItem(.flexible(), spacing: 8), count: 4)

    var body: some View {
        LazyVGrid(columns: columns, spacing: 8) {
            ForEach(entries, id: \.subtypeID) { entry in
                let isDiscovered = discoveredSubtypes.contains(entry.subtypeID)
                DeepSpecimenCard(entry: entry, discovered: isDiscovered)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.white.opacity(0.02))
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .strokeBorder(
                                        isDiscovered
                                            ? Color(hex: "7B5FD4").opacity(0.40)
                                            : Color(hex: "7B5FD4").opacity(0.12),
                                        lineWidth: 1
                                    )
                            )
                    )
                    .onTapGesture {
                        guard isDiscovered else { return }
                        onDiscoveredTap?(entry)
                    }
            }
        }
    }
}

// MARK: - DeepSpecimenCard

/// Card for deep specimens. Unlocked = sprite + name; locked = lock icon + unlock condition.
private struct DeepSpecimenCard: View {
    let entry: CatalogEntry
    let discovered: Bool

    var body: some View {
        VStack(spacing: 4) {
            if discovered {
                SpecimenSprite(subtype: entry.subtypeID, category: entry.category, size: 48)
                Text(entry.creatureName)
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.7))
                    .lineLimit(1)
            } else {
                // Lock icon + unlock condition
                ZStack {
                    Circle()
                        .fill(Color(hex: "7B5FD4").opacity(0.08))
                        .frame(width: 48, height: 48)
                    Image(systemName: "lock.fill")
                        .font(.system(size: 16, weight: .medium))
                        .foregroundColor(Color(hex: "7B5FD4").opacity(0.5))
                }
                Text(entry.unlockCondition)
                    .font(.custom("Inter-Regular", size: 8))
                    .foregroundColor(Color(hex: "7B5FD4").opacity(0.55))
                    .lineLimit(2)
                    .multilineTextAlignment(.center)
                    .minimumScaleFactor(0.7)
            }
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 8)
        .padding(.horizontal, 4)
    }
}

// MARK: - CollectionCard

/// A single specimen-type card. Discovered = sprite + name; undiscovered = silhouette + "???".
struct CollectionCard: View {
    let entry: CatalogEntry
    let discovered: Bool

    var body: some View {
        VStack(spacing: 6) {
            if discovered {
                SpecimenSprite(subtype: entry.subtypeID, category: entry.category, size: 48)
            } else {
                // Dark silhouette
                ZStack {
                    Circle()
                        .fill(Color.white.opacity(0.04))
                        .frame(width: 48, height: 48)
                    Text("?")
                        .font(.custom("SpaceGrotesk-Bold", size: 20))
                        .foregroundColor(.white.opacity(0.15))
                }
            }

            Text(discovered ? entry.creatureName : "???")
                .font(.custom("Inter-Regular", size: 10))
                .foregroundColor(discovered ? .white.opacity(0.7) : .white.opacity(0.2))
                .lineLimit(1)

            if discovered {
                Text(entry.personalityLine)
                    .font(.custom("Inter-Regular", size: 8))
                    .foregroundColor(.white.opacity(0.25))
                    .italic()
                    .lineLimit(1)
                    .minimumScaleFactor(0.8)
            }
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 8)
    }
}

// MARK: - Preview

#if DEBUG
private func makePreviewStore() -> ReefStore {
    let store = ReefStore()
    store.specimens[0] = Specimen(
        id: UUID(),
        name: "Sawfin",
        category: .source,
        rarity: .common,
        health: 80,
        isPhantom: false,
        phantomScar: false,
        subtype: "polyblep-saw",
        catchAccelPattern: [],
        provenance: [],
        spectralDNA: [],
        parameterState: [:],
        catchLatitude: nil,
        catchLongitude: nil,
        catchTimestamp: Date(),
        catchWeatherDescription: nil,
        creatureGenomeData: nil,
        cosmeticTier: .standard,
        morphIndex: 0,
        musicHash: nil,
        sourceTrackTitle: nil,
        xp: 0,
        level: 1,
        aggressiveScore: 0,
        gentleScore: 0,
        totalPlaySeconds: 0
    )
    store.specimens[1] = Specimen(
        id: UUID(),
        name: "Echocave",
        category: .effect,
        rarity: .uncommon,
        health: 65,
        isPhantom: false,
        phantomScar: false,
        subtype: "delay-stereo",
        catchAccelPattern: [],
        provenance: [],
        spectralDNA: [],
        parameterState: [:],
        catchLatitude: nil,
        catchLongitude: nil,
        catchTimestamp: Date(),
        catchWeatherDescription: nil,
        creatureGenomeData: nil,
        cosmeticTier: .standard,
        morphIndex: 0,
        musicHash: nil,
        sourceTrackTitle: nil,
        xp: 0,
        level: 1,
        aggressiveScore: 0,
        gentleScore: 0,
        totalPlaySeconds: 0
    )
    return store
}

#Preview {
    CollectionTab()
        .environmentObject(makePreviewStore())
}
#endif
