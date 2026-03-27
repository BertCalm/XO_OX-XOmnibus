import SwiftUI

// MARK: - CollectionTab

/// Pokédex-style view of all 24 specimen types.
/// Cards show discovered creatures with sprite + name;
/// undiscovered creatures show a silhouette with "???".
struct CollectionTab: View {
    @EnvironmentObject var reefStore: ReefStore

    @State private var selectedSpecimen: Specimen?
    @State private var showingCard = false

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

    private var discoveredCount: Int {
        let subtypes = discoveredSubtypes
        return SpecimenCatalog.all.filter { subtypes.contains($0.subtypeID) }.count
    }

    /// Find the best matching Specimen for a catalog entry's subtypeID.
    /// Checks the live reef first, then falls back to the full persistent collection.
    private func specimen(for entry: CatalogEntry) -> Specimen? {
        reefStore.specimens.compactMap { $0 }.first { $0.subtype == entry.subtypeID }
            ?? reefStore.loadAllSpecimens().first { $0.subtype == entry.subtypeID }
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

                        // Sections by category
                        let subtypes = discoveredSubtypes

                        CollectionSection(
                            title: "Shells",
                            categoryColor: Color(hex: "3380FF"),
                            entries: SpecimenCatalog.sources,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in
                                selectedSpecimen = specimen(for: entry)
                                showingCard = selectedSpecimen != nil
                            }
                        )

                        CollectionSection(
                            title: "Coral",
                            categoryColor: Color(hex: "FF4D4D"),
                            entries: SpecimenCatalog.processors,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in
                                selectedSpecimen = specimen(for: entry)
                                showingCard = selectedSpecimen != nil
                            }
                        )

                        CollectionSection(
                            title: "Currents",
                            categoryColor: Color(hex: "4DCC4D"),
                            entries: SpecimenCatalog.modulators,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in
                                selectedSpecimen = specimen(for: entry)
                                showingCard = selectedSpecimen != nil
                            }
                        )

                        CollectionSection(
                            title: "Tide Pools",
                            categoryColor: Color(hex: "B34DFF"),
                            entries: SpecimenCatalog.effects,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in
                                selectedSpecimen = specimen(for: entry)
                                showingCard = selectedSpecimen != nil
                            }
                        )
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
        HStack(alignment: .lastTextBaseline, spacing: 8) {
            Text("Collection")
                .font(.custom("SpaceGrotesk-Bold", size: 20))
                .foregroundColor(.white)

            Spacer()

            Text("\(discoveredCount) / 24")
                .font(.custom("JetBrainsMono-Regular", size: 12))
                .foregroundColor(Color(hex: "1E8B7E"))
        }
        .padding(.top, 16)
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
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 8)
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    // Build a mock reef store with a couple of discovered specimens
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
        sourceTrackTitle: nil
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
        sourceTrackTitle: nil
    )

    return CollectionTab()
        .environmentObject(store)
}
#endif
