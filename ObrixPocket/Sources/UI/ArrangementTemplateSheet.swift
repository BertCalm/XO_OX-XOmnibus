import SwiftUI

/// Template browser sheet for multi-specimen arrangements.
///
/// Present as a sheet from the Reef or Performance tab:
///   .sheet(isPresented: …) {
///     ArrangementTemplateSheet(
///       templateManager: arrangementManager,
///       availableSpecimens: […],
///       onApply: { template in … }
///     )
///   }
struct ArrangementTemplateSheet: View {

    @ObservedObject var templateManager: ArrangementTemplateManager

    /// Available specimens for auto-assignment previews.
    /// Each element carries the specimen UUID and its VoiceProfile subtype ID.
    let availableSpecimens: [(id: UUID, subtypeID: String, name: String)]

    /// Called when the player confirms "Apply Template". Passes the template
    /// and the auto-assigned slot mapping (slotIndex → specimenID).
    let onApply: (ArrangementTemplate, [Int: UUID]) -> Void

    @Environment(\.dismiss) private var dismiss
    @State private var selectedCategory: TemplateCategory = .starter
    @State private var selectedTemplate: ArrangementTemplate? = nil
    @State private var expandedSlotAssignments: [Int: UUID] = [:]
    @State private var matchMap: [Int: [SlotMatchScore]] = [:]

    private var templates: [ArrangementTemplate] {
        templateManager.allTemplates.filter { $0.category == selectedCategory }
    }

    var body: some View {
        NavigationStack {
            VStack(spacing: 0) {
                categoryTabs
                Divider().background(Color.white.opacity(0.06))

                if templates.isEmpty {
                    emptyState
                } else {
                    templateList
                }
            }
            .background(DesignTokens.panelBackground.ignoresSafeArea())
            .navigationTitle("Arrangement Templates")
            .navigationBarTitleDisplayMode(.inline)
            .toolbarBackground(DesignTokens.panelBackground, for: .navigationBar)
            .toolbarColorScheme(.dark, for: .navigationBar)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { dismiss() }
                        .font(DesignTokens.bodyMedium(14))
                        .foregroundColor(.white.opacity(0.5))
                }
            }
        }
    }

    // MARK: - Category Tabs

    private var categoryTabs: some View {
        HStack(spacing: 0) {
            ForEach(TemplateCategory.allCases, id: \.self) { category in
                Button(action: {
                    selectedCategory = category
                    selectedTemplate = nil
                }) {
                    VStack(spacing: DesignTokens.spacing4) {
                        Text(categoryLabel(category))
                            .font(DesignTokens.bodyMedium(12))
                            .foregroundColor(selectedCategory == category ? .white : .white.opacity(0.3))
                        Rectangle()
                            .fill(selectedCategory == category ? DesignTokens.reefJade : Color.clear)
                            .frame(height: 2)
                    }
                }
                .frame(maxWidth: .infinity)
                .padding(.vertical, DesignTokens.spacing12)
            }
        }
        .padding(.horizontal, DesignTokens.spacing16)
        .background(DesignTokens.panelBackground)
    }

    private func categoryLabel(_ c: TemplateCategory) -> String {
        switch c {
        case .starter:   return "Starter"
        case .ensemble:  return "Ensemble"
        case .orchestra: return "Orchestra"
        case .genre:     return "Genre"
        }
    }

    // MARK: - Template List

    private var templateList: some View {
        ScrollView {
            LazyVStack(spacing: DesignTokens.spacing12) {
                ForEach(templates) { template in
                    TemplateCard(
                        template: template,
                        isSelected: selectedTemplate?.id == template.id,
                        availableSpecimens: availableSpecimens,
                        templateManager: templateManager,
                        onSelect: {
                            selectTemplate(template)
                        },
                        onApply: {
                            onApply(template, expandedSlotAssignments)
                            dismiss()
                        },
                        assignments: expandedSlotAssignments,
                        matchMap: matchMap
                    )
                }
            }
            .padding(.horizontal, DesignTokens.spacing20)
            .padding(.vertical, DesignTokens.spacing16)
        }
    }

    // MARK: - Empty State

    private var emptyState: some View {
        VStack(spacing: DesignTokens.spacing8) {
            Text("No templates in this category")
                .font(DesignTokens.body(13))
                .foregroundColor(.white.opacity(0.3))
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }

    // MARK: - Selection Logic

    private func selectTemplate(_ template: ArrangementTemplate) {
        if selectedTemplate?.id == template.id {
            // Deselect
            selectedTemplate = nil
            expandedSlotAssignments = [:]
            matchMap = [:]
            return
        }
        selectedTemplate = template
        // Auto-assign and compute match map
        let specs = availableSpecimens.map { (id: $0.id, subtypeID: $0.subtypeID) }
        matchMap = templateManager.matchSpecimensToTemplate(template, specimens: specs)
        expandedSlotAssignments = templateManager.autoAssign(template: template, specimens: specs)
    }
}

// MARK: - Template Card

private struct TemplateCard: View {

    let template: ArrangementTemplate
    let isSelected: Bool
    let availableSpecimens: [(id: UUID, subtypeID: String, name: String)]
    let templateManager: ArrangementTemplateManager
    let onSelect: () -> Void
    let onApply: () -> Void
    let assignments: [Int: UUID]
    let matchMap: [Int: [SlotMatchScore]]

    var body: some View {
        VStack(alignment: .leading, spacing: 0) {
            cardHeader
            if isSelected {
                Divider().background(Color.white.opacity(0.06))
                slotAssignments
                couplingHints
                applyButton
            }
        }
        .background(
            RoundedRectangle(cornerRadius: 14)
                .fill(isSelected ? Color.white.opacity(0.06) : Color.white.opacity(0.03))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 14)
                .strokeBorder(
                    isSelected ? DesignTokens.reefJade.opacity(0.4) : Color.white.opacity(0.07),
                    lineWidth: 1
                )
        )
        .animation(.easeInOut(duration: 0.18), value: isSelected)
    }

    // MARK: Card Header

    private var cardHeader: some View {
        Button(action: onSelect) {
            VStack(alignment: .leading, spacing: DesignTokens.spacing8) {
                HStack(spacing: DesignTokens.spacing8) {
                    Text(template.name)
                        .font(DesignTokens.heading(15))
                        .foregroundColor(.white)

                    Spacer()

                    slotCountBadge
                    difficultyBadge
                }

                Text(template.description)
                    .font(DesignTokens.body(11))
                    .foregroundColor(.white.opacity(0.45))
                    .lineLimit(2)

                Text(template.performanceNote)
                    .font(DesignTokens.body(10))
                    .foregroundColor(DesignTokens.xoGold.opacity(0.6))
                    .italic()
                    .lineLimit(2)
            }
            .padding(DesignTokens.spacing16)
        }
        .buttonStyle(.plain)
    }

    private var slotCountBadge: some View {
        Text("\(template.requiredSlotCount) slots")
            .font(DesignTokens.mono(9))
            .foregroundColor(.white.opacity(0.5))
            .padding(.horizontal, DesignTokens.spacing6)
            .padding(.vertical, 2)
            .background(
                RoundedRectangle(cornerRadius: 4)
                    .fill(Color.white.opacity(0.07))
            )
    }

    private var difficultyBadge: some View {
        HStack(spacing: 2) {
            ForEach(1...5, id: \.self) { i in
                Circle()
                    .fill(i <= template.difficultyLevel ? DesignTokens.xoGold : Color.white.opacity(0.1))
                    .frame(width: 5, height: 5)
            }
        }
    }

    // MARK: Slot Assignments

    private var slotAssignments: some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
            Text("SLOT ASSIGNMENTS")
                .font(DesignTokens.mono(9))
                .tracking(1.2)
                .foregroundColor(.white.opacity(0.2))
                .padding(.horizontal, DesignTokens.spacing16)
                .padding(.top, DesignTokens.spacing12)

            ForEach(template.slots, id: \.index) { slot in
                slotRow(slot)
            }
        }
        .padding(.bottom, DesignTokens.spacing8)
    }

    private func slotRow(_ slot: TemplateSlot) -> some View {
        let assignedID = assignments[slot.index]
        let assignedName = availableSpecimens.first { $0.id == assignedID }?.name ?? "Unassigned"
        let score = matchMap[slot.index]?.first?.score ?? 0

        return HStack(spacing: DesignTokens.spacing10) {
            // Slot index circle
            ZStack {
                Circle()
                    .fill(DesignTokens.reefJade.opacity(0.15))
                    .frame(width: 24, height: 24)
                Text("\(slot.index + 1)")
                    .font(DesignTokens.monoBold(10))
                    .foregroundColor(DesignTokens.reefJade)
            }

            VStack(alignment: .leading, spacing: 2) {
                HStack(spacing: DesignTokens.spacing4) {
                    Text(roleName(slot.role))
                        .font(DesignTokens.bodyMedium(11))
                        .foregroundColor(.white.opacity(0.65))
                    Text("—")
                        .font(DesignTokens.mono(10))
                        .foregroundColor(.white.opacity(0.2))
                    Text(assignedID != nil ? assignedName : slot.hint)
                        .font(DesignTokens.body(11))
                        .foregroundColor(assignedID != nil ? .white.opacity(0.75) : .white.opacity(0.3))
                        .italic(assignedID == nil)
                        .lineLimit(1)
                }
                if assignedID != nil {
                    Text("Match: \(Int(score * 100))%")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(matchScoreColor(score))
                }
            }

            Spacer()

            // Volume hint bar
            volumeHintBar(slot.volumeHint)
        }
        .padding(.horizontal, DesignTokens.spacing16)
        .padding(.vertical, DesignTokens.spacing6)
    }

    private func volumeHintBar(_ hint: Float) -> some View {
        HStack(spacing: DesignTokens.spacing2) {
            Text("vol")
                .font(DesignTokens.mono(8))
                .foregroundColor(.white.opacity(0.2))
            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 2)
                        .fill(Color.white.opacity(0.07))
                    RoundedRectangle(cornerRadius: 2)
                        .fill(Color.white.opacity(0.25))
                        .frame(width: geo.size.width * CGFloat(hint))
                }
            }
            .frame(width: 36, height: 5)
        }
    }

    private func roleName(_ role: MusicalRole) -> String {
        switch role {
        case .bass:    return "Bass"
        case .melody:  return "Melody"
        case .rhythm:  return "Rhythm"
        case .harmony: return "Harmony"
        case .texture: return "Texture"
        case .effect:  return "Effect"
        }
    }

    private func matchScoreColor(_ score: Float) -> Color {
        switch score {
        case 0..<0.4: return DesignTokens.errorRed.opacity(0.7)
        case 0.4..<0.7: return DesignTokens.xoGold.opacity(0.7)
        default: return DesignTokens.modulatorColor.opacity(0.7)
        }
    }

    // MARK: Coupling Hints

    @ViewBuilder
    private var couplingHints: some View {
        if !template.couplingHints.isEmpty {
            VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                Text("SUGGESTED WIRINGS")
                    .font(DesignTokens.mono(9))
                    .tracking(1.2)
                    .foregroundColor(.white.opacity(0.2))
                    .padding(.horizontal, DesignTokens.spacing16)

                ForEach(Array(template.couplingHints.enumerated()), id: \.offset) { _, hint in
                    HStack(spacing: DesignTokens.spacing8) {
                        Text("Slot \(hint.fromSlot + 1)")
                            .font(DesignTokens.mono(10))
                            .foregroundColor(.white.opacity(0.5))

                        // Dotted line
                        Path { path in
                            path.move(to: CGPoint(x: 0, y: 6))
                            path.addLine(to: CGPoint(x: 40, y: 6))
                        }
                        .stroke(style: StrokeStyle(lineWidth: 1, dash: [3, 3]))
                        .foregroundColor(.white.opacity(0.2))
                        .frame(width: 40, height: 12)

                        Image(systemName: "arrow.right")
                            .font(.system(size: 8))
                            .foregroundColor(.white.opacity(0.3))

                        Text("Slot \(hint.toSlot + 1)")
                            .font(DesignTokens.mono(10))
                            .foregroundColor(.white.opacity(0.5))

                        Text(hint.reason)
                            .font(DesignTokens.body(10))
                            .foregroundColor(.white.opacity(0.35))
                            .lineLimit(1)
                            .italic()
                    }
                    .padding(.horizontal, DesignTokens.spacing16)
                }
            }
            .padding(.bottom, DesignTokens.spacing8)
        }
    }

    // MARK: Apply Button

    private var applyButton: some View {
        Button(action: onApply) {
            Text("Apply Template")
                .font(DesignTokens.bodyMedium(15))
                .foregroundColor(.black)
                .frame(maxWidth: .infinity)
                .padding(.vertical, DesignTokens.spacing14)
                .background(
                    RoundedRectangle(cornerRadius: 10)
                        .fill(DesignTokens.reefJade)
                )
        }
        .padding(.horizontal, DesignTokens.spacing16)
        .padding(.bottom, DesignTokens.spacing16)
    }
}

// MARK: - Spacing convenience

private extension DesignTokens {
    static let spacing10: CGFloat = 10
    static let spacing14: CGFloat = 14
}

// MARK: - Preview

#if DEBUG
#Preview {
    ArrangementTemplateSheet(
        templateManager: ArrangementTemplateManager(),
        availableSpecimens: [
            (id: UUID(), subtypeID: "polyblep-saw", name: "Sawfin"),
            (id: UUID(), subtypeID: "lowpass-ladder", name: "Curtain"),
            (id: UUID(), subtypeID: "fm-bell", name: "Chime"),
            (id: UUID(), subtypeID: "noise-perc", name: "Snap"),
        ],
        onApply: { _, _ in }
    )
}
#endif
