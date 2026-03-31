import SwiftUI

/// Full-timeline biography view for a single specimen.
///
/// Shows every journal event in chronological order (newest first) with
/// colour-coded event type icons and optional filtering by event type.
/// Accessible from `MicroscopeView` via a "View Full Journal" button.
struct JournalView: View {

    let specimen: Specimen

    @State private var selectedFilter: JournalEntry.JournalEventType? = nil

    // Pre-computed on appear to avoid re-sorting on every render
    @State private var sortedEntries: [JournalEntry] = []

    private var displayedEntries: [JournalEntry] {
        if let filter = selectedFilter {
            return sortedEntries.filter { $0.type == filter }
        }
        return sortedEntries
    }

    var body: some View {
        VStack(spacing: 0) {
            filterBar
            Divider()
                .background(Color.white.opacity(0.06))
            entryList
        }
        .background(DesignTokens.background)
        .navigationTitle("Journal")
        .navigationBarTitleDisplayMode(.inline)
        .onAppear {
            sortedEntries = specimen.journal.reversed()
        }
    }

    // MARK: - Filter Bar

    private var filterBar: some View {
        ScrollView(.horizontal, showsIndicators: false) {
            HStack(spacing: DesignTokens.spacing8) {
                filterChip(label: "All", type: nil)
                ForEach(JournalEntry.JournalEventType.allCases, id: \.self) { eventType in
                    filterChip(label: JournalManager.label(for: eventType), type: eventType)
                }
            }
            .padding(.horizontal, DesignTokens.spacing16)
            .padding(.vertical, DesignTokens.spacing8)
        }
    }

    private func filterChip(label: String, type: JournalEntry.JournalEventType?) -> some View {
        let isSelected = selectedFilter == type
        return Button {
            withAnimation(.easeInOut(duration: 0.15)) {
                selectedFilter = isSelected ? nil : type
            }
        } label: {
            HStack(spacing: DesignTokens.spacing4) {
                if let type {
                    Image(systemName: JournalManager.icon(for: type))
                        .font(.system(size: 9))
                        .foregroundColor(isSelected ? DesignTokens.background : eventColor(type).opacity(0.7))
                }
                Text(label)
                    .font(DesignTokens.mono(10))
                    .foregroundColor(isSelected ? DesignTokens.background : .white.opacity(0.5))
            }
            .padding(.horizontal, DesignTokens.spacing8)
            .padding(.vertical, DesignTokens.spacing4)
            .background(
                RoundedRectangle(cornerRadius: 12)
                    .fill(isSelected
                          ? (type.map(eventColor) ?? DesignTokens.reefJade)
                          : Color.white.opacity(0.06))
            )
        }
        .buttonStyle(.plain)
    }

    // MARK: - Entry List

    private var entryList: some View {
        Group {
            if displayedEntries.isEmpty {
                emptyState
            } else {
                ScrollView {
                    LazyVStack(spacing: 0) {
                        ForEach(Array(displayedEntries.enumerated()), id: \.element.id) { idx, entry in
                            entryRow(entry: entry, isLast: idx == displayedEntries.count - 1)
                        }
                    }
                    .padding(.vertical, DesignTokens.spacing8)
                }
            }
        }
    }

    private var emptyState: some View {
        VStack(spacing: DesignTokens.spacing12) {
            Image(systemName: "book.closed")
                .font(.system(size: 32))
                .foregroundColor(.white.opacity(0.15))
            Text("No events recorded")
                .font(DesignTokens.body(13))
                .foregroundColor(.white.opacity(0.2))
                .italic()
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
        .padding(.top, 60)
    }

    // MARK: - Entry Row

    private func entryRow(entry: JournalEntry, isLast: Bool) -> some View {
        HStack(alignment: .top, spacing: DesignTokens.spacing12) {

            // Timeline line + icon
            VStack(spacing: 0) {
                ZStack {
                    Circle()
                        .fill(eventColor(entry.type).opacity(0.15))
                        .frame(width: 28, height: 28)
                    Image(systemName: JournalManager.icon(for: entry.type))
                        .font(.system(size: 12, weight: .medium))
                        .foregroundColor(eventColor(entry.type))
                }

                if !isLast {
                    Rectangle()
                        .fill(Color.white.opacity(0.06))
                        .frame(width: 1)
                        .frame(maxHeight: .infinity)
                        .padding(.vertical, DesignTokens.spacing4)
                }
            }
            .frame(width: 28)

            // Content
            VStack(alignment: .leading, spacing: DesignTokens.spacing2) {
                HStack(alignment: .firstTextBaseline) {
                    Text(JournalManager.label(for: entry.type))
                        .font(DesignTokens.bodyMedium(11))
                        .foregroundColor(eventColor(entry.type).opacity(0.85))

                    Spacer()

                    Text(entry.timestamp, style: .relative)
                        .font(DesignTokens.mono(9))
                        .foregroundColor(DesignTokens.mutedText)
                }

                Text(entry.description)
                    .font(DesignTokens.body(12))
                    .foregroundColor(.white.opacity(0.65))
                    .fixedSize(horizontal: false, vertical: true)

                Text(formattedDate(entry.timestamp))
                    .font(DesignTokens.mono(9))
                    .foregroundColor(DesignTokens.mutedText.opacity(0.6))
            }
            .padding(.bottom, DesignTokens.spacing16)
        }
        .padding(.horizontal, DesignTokens.spacing16)
    }

    // MARK: - Helpers

    private func eventColor(_ type: JournalEntry.JournalEventType) -> Color {
        switch type {
        case .born:    return DesignTokens.reefJade
        case .wired:   return DesignTokens.sourceColor
        case .unwired: return DesignTokens.errorRed
        case .levelUp: return DesignTokens.xoGold
        case .evolved: return DesignTokens.legendaryGold
        case .drifted: return DesignTokens.effectColor
        case .played:  return DesignTokens.modulatorColor
        case .traded:  return DesignTokens.xoGold
        }
    }

    private func formattedDate(_ date: Date) -> String {
        let fmt = DateFormatter()
        fmt.dateStyle = .medium
        fmt.timeStyle = .short
        return fmt.string(from: date)
    }
}

// MARK: - Extension: CaseIterable conformance

extension JournalEntry.JournalEventType: CaseIterable {
    static var allCases: [JournalEntry.JournalEventType] {
        [.born, .levelUp, .evolved, .wired, .unwired, .drifted, .played, .traded]
    }
}

// MARK: - MicroscopeView integration button

/// A compact "View Full Journal" button suitable for insertion at the bottom
/// of `MicroscopeView.journalSection`.  Navigates to `JournalView` when tapped.
struct JournalViewLink: View {
    let specimen: Specimen

    var body: some View {
        NavigationLink {
            JournalView(specimen: specimen)
        } label: {
            HStack(spacing: 6) {
                Image(systemName: "book.pages")
                    .font(.system(size: 10))
                Text("View Full Journal (\(specimen.journal.count))")
                    .font(DesignTokens.mono(10))
            }
            .foregroundColor(DesignTokens.reefJade.opacity(0.7))
        }
        .buttonStyle(.plain)
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    NavigationView {
        JournalView(
            specimen: Specimen(
                id: UUID(),
                name: "Prismatic Cascade",
                category: .source,
                rarity: .rare,
                health: 80,
                isPhantom: false,
                phantomScar: false,
                subtype: "polyblep-saw",
                catchAccelPattern: [],
                provenance: [],
                spectralDNA: (0..<64).map { i in
                    0.4 + 0.5 * sin(Float(i) * 0.4)
                },
                parameterState: [:],
                catchLatitude: 37.7749,
                catchLongitude: -122.4194,
                catchTimestamp: Date().addingTimeInterval(-86400 * 7),
                catchWeatherDescription: "Rain",
                creatureGenomeData: nil,
                cosmeticTier: .bioluminescent,
                morphIndex: 0,
                musicHash: nil,
                sourceTrackTitle: nil,
                xp: 350,
                level: 5,
                aggressiveScore: 3.2,
                gentleScore: 1.1,
                totalPlaySeconds: 7200,
                journal: [
                    JournalEntry(id: UUID(), timestamp: Date().addingTimeInterval(-86400 * 7),
                                 type: .born, description: "Caught in the wild"),
                    JournalEntry(id: UUID(), timestamp: Date().addingTimeInterval(-86400 * 6),
                                 type: .wired, description: "Connected to Curtain"),
                    JournalEntry(id: UUID(), timestamp: Date().addingTimeInterval(-86400 * 5),
                                 type: .levelUp, description: "Reached level 2"),
                    JournalEntry(id: UUID(), timestamp: Date().addingTimeInterval(-86400 * 4),
                                 type: .drifted, description: "Spectral drift 3% after 4.2h coupling"),
                    JournalEntry(id: UUID(), timestamp: Date().addingTimeInterval(-86400 * 3),
                                 type: .levelUp, description: "Reached level 3"),
                    JournalEntry(id: UUID(), timestamp: Date().addingTimeInterval(-86400 * 2),
                                 type: .played, description: "1 hour milestone reached"),
                    JournalEntry(id: UUID(), timestamp: Date().addingTimeInterval(-86400),
                                 type: .unwired, description: "Disconnected from Curtain"),
                    JournalEntry(id: UUID(), timestamp: Date().addingTimeInterval(-3600),
                                 type: .levelUp, description: "Reached level 5"),
                ],
                isFavorite: true
            )
        )
    }
    .preferredColorScheme(.dark)
}
#endif
