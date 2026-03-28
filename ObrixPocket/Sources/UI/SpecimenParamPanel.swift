import SwiftUI

/// Parameter editing panel for a selected reef specimen.
/// Appears between the reef grid and keyboard. Real-time engine updates.
struct SpecimenParamPanel: View {
    let slotIndex: Int
    var onDismiss: (() -> Void)? = nil
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var audioEngine: AudioEngineManager
    @State private var activeParam: String? // Which param is being dragged (for hover label)
    @State private var showReleaseConfirm = false
    @State private var showSwapPicker = false
    @State private var showFusionPicker = false

    private var specimen: Specimen? { reefStore.specimens[slotIndex] }

    var body: some View {
        if let spec = specimen {
            VStack(spacing: 0) {
                // Header bar — distinct shade for section separation
                HStack(spacing: 8) {
                    // Category icon — teaches the user what role this specimen plays
                    Image(systemName: categoryIcon(spec.category))
                        .font(.system(size: 10))
                        .foregroundColor(catColor(spec.category))

                    // Category dot
                    Circle()
                        .fill(catColor(spec.category))
                        .frame(width: 8, height: 8)

                    Text(spec.creatureName)
                        .font(.custom("SpaceGrotesk-Bold", size: 15))
                        .foregroundColor(.white)

                    Text(categoryLabel(spec.category).uppercased())
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .tracking(1)
                        .foregroundColor(catColor(spec.category).opacity(0.6))

                    // Level badge
                    Text("Lv.\(spec.level)")
                        .font(.custom("JetBrainsMono-Bold", size: 10))
                        .foregroundColor(spec.level >= 5
                            ? Color(hex: "E9C46A")
                            : Color.white.opacity(0.4))
                        .padding(.horizontal, 5)
                        .padding(.vertical, 2)
                        .background(
                            RoundedRectangle(cornerRadius: 3)
                                .fill(Color.white.opacity(0.06))
                        )

                    Spacer()

                    // Favorite toggle button
                    Button(action: {
                        guard var spec = reefStore.specimens[slotIndex] else { return }
                        spec.isFavorite.toggle()
                        reefStore.specimens[slotIndex] = spec
                        reefStore.save()
                    }) {
                        Image(systemName: (reefStore.specimens[slotIndex]?.isFavorite ?? false) ? "heart.fill" : "heart")
                            .font(.system(size: 14))
                            .foregroundColor((reefStore.specimens[slotIndex]?.isFavorite ?? false) ? Color(hex: "FF4D4D") : .white.opacity(0.3))
                    }

                    // Close button
                    Button(action: { onDismiss?() }) {
                        Image(systemName: "xmark")
                            .font(.system(size: 10, weight: .bold))
                            .foregroundColor(.white.opacity(0.3))
                    }
                }
                .padding(.horizontal, 16)
                .padding(.vertical, 10)
                .background(Color.white.opacity(0.04)) // Subtle header separation

                // Category role description — one line teaching what this category does
                Text(categoryDescription(spec.category))
                    .font(.custom("Inter-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.3))
                    .padding(.horizontal, 16)
                    .padding(.top, 4)

                // Hover label — shows param name + value while dragging
                if let paramName = activeParam,
                   let value = reefStore.specimens[slotIndex]?.parameterState[paramName] {
                    Text("\(paramDisplayName(paramName)): \(String(format: "%.2f", value))")
                        .font(.custom("JetBrainsMono-Bold", size: 13))
                        .foregroundColor(catColor(spec.category))
                        .padding(.vertical, 4)
                        .transition(.opacity)
                }

                // Parameter sliders — category-specific
                VStack(spacing: 8) {
                    switch spec.category {
                    case .source:   sourceParams(spec)
                    case .processor: processorParams(spec)
                    case .modulator: modulatorParams(spec)
                    case .effect:    effectParams(spec)
                    }

                    // Envelope section — ALL specimens get envelope controls
                    sectionHeader("ENVELOPE")
                    envelopeParams(spec)

                    // XP Boost button — spend 10 energy to award +50 XP to this specimen
                    Button(action: {
                        if ReefEnergyManager.shared.spend(ReefEnergyManager.xpCost) {
                            audioEngine.awardBulkXP(slotIndex: slotIndex, amount: 50)
                        }
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: "bolt.fill")
                                .font(.system(size: 9))
                            Text("Boost +50 XP")
                                .font(.custom("Inter-Regular", size: 10))
                            Text("(\(ReefEnergyManager.xpCost)⚡)")
                                .font(.custom("JetBrainsMono-Regular", size: 9))
                        }
                        .foregroundColor(ReefEnergyManager.shared.canAfford(ReefEnergyManager.xpCost)
                            ? Color(hex: "E9C46A") : .white.opacity(0.15))
                    }
                    .disabled(!ReefEnergyManager.shared.canAfford(ReefEnergyManager.xpCost))
                    .padding(.top, 4)

                    // Move to Stasis button — preserves the specimen without occupying a reef slot
                    Button(action: {
                        reefStore.moveToStasis(at: slotIndex)
                        reefStore.save()
                        onDismiss?()
                    }) {
                        HStack(spacing: 4) {
                            Image(systemName: "archivebox")
                                .font(.system(size: 10))
                            Text("Move to Stasis")
                                .font(.custom("Inter-Regular", size: 11))
                        }
                        .foregroundColor(Color(hex: "E9C46A").opacity(0.5))
                    }
                    .padding(.top, 4)

                    // Swap button — open stasis picker to directly swap this specimen
                    Button(action: { showSwapPicker = true }) {
                        HStack(spacing: 4) {
                            Image(systemName: "arrow.triangle.2.circlepath")
                                .font(.system(size: 10))
                            Text("Swap")
                                .font(.custom("Inter-Regular", size: 11))
                        }
                        .foregroundColor(Color(hex: "3380FF").opacity(0.5))
                    }
                    .padding(.top, 4)

                    // Fusion button — requires level 3+
                    if let spec = specimen, spec.level >= 3 {
                        Button(action: { showFusionPicker = true }) {
                            HStack(spacing: 4) {
                                Image(systemName: "arrow.triangle.merge")
                                    .font(.system(size: 10))
                                Text("Fuse")
                                    .font(.custom("Inter-Regular", size: 11))
                            }
                            .foregroundColor(Color(hex: "B34DFF").opacity(0.5))
                        }
                        .padding(.top, 4)
                    }

                    // Release button — lets user free up a full reef slot (destructive)
                    Button(action: { showReleaseConfirm = true }) {
                        HStack(spacing: 4) {
                            Image(systemName: "arrow.uturn.backward")
                                .font(.system(size: 10))
                            Text("Release")
                                .font(.custom("Inter-Regular", size: 11))
                        }
                        .foregroundColor(Color(hex: "FF4D4D").opacity(0.5))
                    }
                    .padding(.top, 4)
                }
                .padding(.horizontal, 16)
                .padding(.vertical, 10)
            }
            .background(
                ZStack {
                    Color(hex: "0E0E10")
                    if let panelBg = UIImage(named: "UIPanelBg") {
                        Image(uiImage: panelBg)
                            .resizable()
                            .interpolation(.none)
                            .opacity(0.08) // Very subtle — just texture
                    }
                }
            )
            .overlay(
                RoundedRectangle(cornerRadius: 12)
                    .stroke(catColor(spec.category).opacity(0.15), lineWidth: 1)
            )
            .cornerRadius(12)
            .padding(.horizontal, 12)
            .sheet(isPresented: $showSwapPicker) {
                if let spec = specimen {
                    SwapPickerView(
                        currentSlot: slotIndex,
                        currentSpecimen: spec,
                        onSwap: { stasisSpecimenId in
                            performSwap(stasisSpecimenId: stasisSpecimenId)
                            showSwapPicker = false
                            onDismiss?()
                        },
                        onCancel: { showSwapPicker = false }
                    )
                    .environmentObject(reefStore)
                }
            }
            .sheet(isPresented: $showFusionPicker) {
                if let spec = specimen {
                    FusionPickerView(
                        sourceSlot: slotIndex,
                        sourceSpecimen: spec,
                        onFuse: { partnerSlot in
                            performFusion(partnerSlot: partnerSlot)
                            showFusionPicker = false
                            onDismiss?()
                        },
                        onCancel: { showFusionPicker = false }
                    )
                    .environmentObject(reefStore)
                }
            }
            .alert("Release \(specimen?.creatureName ?? "specimen")?", isPresented: $showReleaseConfirm) {
                Button("Release", role: .destructive) {
                    reefStore.releaseSpecimen(at: slotIndex)
                    reefStore.save()
                    ReefStatsTracker.shared.increment(.specimensReleased)
                    onDismiss?()
                }
                Button("Cancel", role: .cancel) {}
            } message: {
                Text("It will return to the wild. This cannot be undone.")
            }
        }
    }

    // MARK: - Fusion Logic

    private func performFusion(partnerSlot: Int) {
        guard let source = reefStore.specimens[slotIndex],
              let partner = reefStore.specimens[partnerSlot],
              let child = SpecimenFusion.fuse(source, partner) else { return }

        // Remove both parents
        reefStore.releaseSpecimen(at: slotIndex)
        reefStore.releaseSpecimen(at: partnerSlot)

        // Place child in the source's slot
        reefStore.specimens[slotIndex] = child
        reefStore.save()

        audioEngine.rebuildParamCache(reefStore: reefStore)

        HapticEngine.evolution() // Dramatic haptic for fusion
    }

    // MARK: - Swap Logic

    private func performSwap(stasisSpecimenId: UUID) {
        guard reefStore.specimens[slotIndex] != nil else { return }
        // Move current specimen to stasis
        reefStore.moveToStasis(at: slotIndex)
        // Move stasis specimen to this slot (slot is now empty after moveToStasis)
        reefStore.moveFromStasis(specimenId: stasisSpecimenId, toSlot: slotIndex)
        reefStore.save()
        // Rebuild param cache so the engine reflects the newly slotted specimen
        audioEngine.rebuildParamCache(reefStore: reefStore)
    }

    // MARK: - Section Header

    private func sectionHeader(_ title: String) -> some View {
        HStack {
            Text(title)
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))
            Rectangle()
                .fill(Color.white.opacity(0.06))
                .frame(height: 1)
        }
        .padding(.top, 4)
    }

    // MARK: - Source Parameters

    private func sourceParams(_ spec: Specimen) -> some View {
        VStack(spacing: 6) {
            sectionHeader("SOURCE")
            // Sonic character from catalog — tells user what this source actually produces
            if let entry = SpecimenCatalog.entry(for: spec.subtype) {
                Text(entry.sonicCharacter)
                    .font(.custom("Inter-Regular", size: 9))
                    .foregroundColor(catColor(spec.category).opacity(0.4))
                    .padding(.bottom, 2)
            }
            paramSlider("Tune", paramKey: "obrix_src1Tune", range: -24...24, unit: "st", spec: spec)
            paramSlider("Level", paramKey: "obrix_src1Level", range: 0...1, unit: "", spec: spec)
        }
    }

    // MARK: - Processor Parameters

    private func processorParams(_ spec: Specimen) -> some View {
        VStack(spacing: 6) {
            sectionHeader("FILTER")
            // Processor type + filter behavior hint from catalog
            if let entry = SpecimenCatalog.entry(for: spec.subtype) {
                Text(entry.sonicCharacter)
                    .font(.custom("Inter-Regular", size: 9))
                    .foregroundColor(catColor(spec.category).opacity(0.4))
                    .padding(.bottom, 2)
            }
            paramSlider("Cutoff", paramKey: "obrix_flt1Cutoff", range: 0...1, unit: "", spec: spec)
            paramSlider("Resonance", paramKey: "obrix_flt1Resonance", range: 0...1, unit: "", spec: spec)
            paramSlider("Env Depth", paramKey: "obrix_flt1EnvDepth", range: -0.5...0.5, unit: "", spec: spec)
        }
    }

    // MARK: - Modulator Parameters

    private func modulatorParams(_ spec: Specimen) -> some View {
        VStack(spacing: 6) {
            sectionHeader("MODULATOR")
            // Modulator character + static target hint
            if let entry = SpecimenCatalog.entry(for: spec.subtype) {
                Text(entry.sonicCharacter)
                    .font(.custom("Inter-Regular", size: 9))
                    .foregroundColor(catColor(spec.category).opacity(0.4))
            }
            Text("→ Filter Cutoff")
                .font(.custom("Inter-Regular", size: 9))
                .foregroundColor(catColor(spec.category).opacity(0.3))
                .padding(.bottom, 2)
            paramSlider("Rate", paramKey: "obrix_lfo1Rate", range: 0.01...10, unit: "Hz", spec: spec)
            paramSlider("Depth", paramKey: "obrix_lfo1Depth", range: 0...1, unit: "", spec: spec)
        }
    }

    // MARK: - Effect Parameters

    private func effectParams(_ spec: Specimen) -> some View {
        VStack(spacing: 6) {
            sectionHeader("EFFECT")
            // Effect type from catalog — names the effect and what it does to the output
            if let entry = SpecimenCatalog.entry(for: spec.subtype) {
                Text(entry.sonicCharacter)
                    .font(.custom("Inter-Regular", size: 9))
                    .foregroundColor(catColor(spec.category).opacity(0.4))
                    .padding(.bottom, 2)
            }
            paramSlider("Mix", paramKey: "obrix_fx1Mix", range: 0...1, unit: "", spec: spec)
            paramSlider("Tone", paramKey: "obrix_fx1Param1", range: 0...1, unit: "", spec: spec)
        }
    }

    // MARK: - Envelope (shared by all categories)

    private func envelopeParams(_ spec: Specimen) -> some View {
        VStack(spacing: 6) {
            paramSlider("Attack", paramKey: "obrix_env1Attack", range: 0.001...0.5, unit: "s", spec: spec)
            paramSlider("Decay", paramKey: "obrix_env1Decay", range: 0.01...2.0, unit: "s", spec: spec)
            paramSlider("Sustain", paramKey: "obrix_env1Sustain", range: 0...1, unit: "", spec: spec)
            paramSlider("Release", paramKey: "obrix_env1Release", range: 0.01...3.0, unit: "s", spec: spec)
        }
    }

    // MARK: - Parameter Slider

    private func paramSlider(_ label: String, paramKey: String, range: ClosedRange<Float>, unit: String, spec: Specimen) -> some View {
        let defaultVal = Float((range.lowerBound + range.upperBound) / 2)
        let color = catColor(spec.category)
        let liveValue = reefStore.specimens[slotIndex]?.parameterState[paramKey] ?? defaultVal

        return HStack(spacing: 10) {
            // Label
            Text(label)
                .font(.custom("Inter-Medium", size: 11))
                .foregroundColor(.white.opacity(0.55))
                .frame(width: 65, alignment: .trailing)

            // Slider with custom track
            ZStack(alignment: .leading) {
                // Track background
                RoundedRectangle(cornerRadius: 3)
                    .fill(Color.white.opacity(0.08))
                    .frame(height: 6)

                // Track fill
                GeometryReader { geo in
                    let pct = CGFloat((liveValue - range.lowerBound) / (range.upperBound - range.lowerBound))
                    RoundedRectangle(cornerRadius: 3)
                        .fill(color.opacity(0.5))
                        .frame(width: geo.size.width * pct, height: 6)
                }
                .frame(height: 6)

                // Invisible slider on top
                Slider(
                    value: Binding(
                        get: { liveValue },
                        set: { newValue in
                            activeParam = paramKey
                            updateParam(paramKey: paramKey, value: newValue)
                        }
                    ),
                    in: range,
                    onEditingChanged: { editing in
                        if !editing { activeParam = nil }
                    }
                )
                .tint(.clear) // Hide default tint — we draw our own track
                .accentColor(.clear)
            }

            // Value readout
            Text(formatValue(liveValue, unit: unit, range: range))
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(.white.opacity(activeParam == paramKey ? 0.8 : 0.35))
                .frame(width: 44, alignment: .trailing)
        }
        .frame(height: 28)
    }

    // MARK: - Update Logic

    private func updateParam(paramKey: String, value: Float) {
        guard var spec = reefStore.specimens[slotIndex] else { return }
        spec.parameterState[paramKey] = value
        reefStore.specimens[slotIndex] = spec

        // Push just this one parameter directly — don't reconfigure the whole chain
        audioEngine.pushSingleParam(specimenParam: paramKey, value: value)
    }

    // MARK: - Helpers

    private func formatValue(_ value: Float, unit: String, range: ClosedRange<Float>) -> String {
        let span = range.upperBound - range.lowerBound
        let formatted: String
        if span > 10 {
            formatted = String(format: "%.1f", value)
        } else if span > 1 {
            formatted = String(format: "%.2f", value)
        } else {
            formatted = String(format: "%.2f", value)
        }
        return unit.isEmpty ? formatted : "\(formatted)\(unit)"
    }

    private func paramDisplayName(_ key: String) -> String {
        let names: [String: String] = [
            "obrix_src1Tune": "Tune", "obrix_src1Level": "Level",
            "obrix_flt1Cutoff": "Cutoff", "obrix_flt1Resonance": "Resonance",
            "obrix_flt1EnvDepth": "Env Depth",
            "obrix_lfo1Rate": "Rate", "obrix_lfo1Depth": "Depth",
            "obrix_fx1Mix": "Mix", "obrix_fx1Param1": "Tone",
            "obrix_env1Attack": "Attack", "obrix_env1Decay": "Decay",
            "obrix_env1Sustain": "Sustain", "obrix_env1Release": "Release",
        ]
        return names[key] ?? key
    }

    private func categoryLabel(_ category: SpecimenCategory) -> String {
        switch category {
        case .source:    return "Source"
        case .processor: return "Processor"
        case .modulator: return "Modulator"
        case .effect:    return "Effect"
        }
    }

    /// SF Symbol name that visually communicates each category's role.
    private func categoryIcon(_ category: SpecimenCategory) -> String {
        switch category {
        case .source:    return "waveform"
        case .processor: return "slider.horizontal.3"
        case .modulator: return "arrow.triangle.2.circlepath"
        case .effect:    return "sparkles"
        }
    }

    /// One-line role description shown below the header to teach the user what each category does.
    private func categoryDescription(_ category: SpecimenCategory) -> String {
        switch category {
        case .source:    return "Generates the raw sound wave"
        case .processor: return "Shapes and transforms the sound"
        case .modulator: return "Moves parameters over time"
        case .effect:    return "Adds space, depth, or color"
        }
    }

    private func catColor(_ category: SpecimenCategory) -> Color {
        switch category {
        case .source:    return Color(hex: "3380FF")
        case .processor: return Color(hex: "FF4D4D")
        case .modulator: return Color(hex: "4DCC4D")
        case .effect:    return Color(hex: "B34DFF")
        }
    }
}

// MARK: - SwapPickerView

/// Sheet presented when the user taps "Swap" in the param panel.
/// Lists all stasis specimens so the user can pick one to swap in immediately.
struct SwapPickerView: View {
    let currentSlot: Int
    let currentSpecimen: Specimen
    let onSwap: (UUID) -> Void
    let onCancel: () -> Void
    @EnvironmentObject var reefStore: ReefStore

    @State private var stasisSpecimens: [Specimen] = []

    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Current specimen header — shows what will be moved out
                HStack(spacing: 8) {
                    SpecimenSprite(subtype: currentSpecimen.subtype,
                                  category: currentSpecimen.category,
                                  size: 32)
                    VStack(alignment: .leading, spacing: 2) {
                        Text(currentSpecimen.creatureName)
                            .font(.custom("SpaceGrotesk-Bold", size: 14))
                            .foregroundColor(.white)
                        Text("Currently in slot \(currentSlot + 1)")
                            .font(.custom("Inter-Regular", size: 10))
                            .foregroundColor(.white.opacity(0.3))
                    }
                    Spacer()
                    Image(systemName: "arrow.triangle.2.circlepath")
                        .foregroundColor(.white.opacity(0.2))
                }
                .padding(16)
                .background(Color.white.opacity(0.03))

                if stasisSpecimens.isEmpty {
                    VStack(spacing: 8) {
                        Spacer()
                        Text("No specimens in stasis")
                            .font(.custom("Inter-Regular", size: 13))
                            .foregroundColor(.white.opacity(0.3))
                        Text("Move specimens to stasis from the param panel to swap later")
                            .font(.custom("Inter-Regular", size: 10))
                            .foregroundColor(.white.opacity(0.2))
                            .multilineTextAlignment(.center)
                            .padding(.horizontal, 40)
                        Spacer()
                    }
                } else {
                    List(stasisSpecimens) { specimen in
                        Button(action: { onSwap(specimen.id) }) {
                            HStack(spacing: 10) {
                                SpecimenSprite(subtype: specimen.subtype,
                                              category: specimen.category,
                                              size: 28)
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(specimen.creatureName)
                                        .font(.custom("SpaceGrotesk-Bold", size: 13))
                                        .foregroundColor(.white)
                                    HStack(spacing: 6) {
                                        Text(specimen.rarity.rawValue.uppercased())
                                            .font(.custom("JetBrainsMono-Regular", size: 8))
                                            .foregroundColor(Color(hex: "E9C46A"))
                                        Text("Lv.\(specimen.level)")
                                            .font(.custom("JetBrainsMono-Regular", size: 8))
                                            .foregroundColor(.white.opacity(0.4))
                                    }
                                }
                                Spacer()
                                Image(systemName: "arrow.right.circle")
                                    .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
                            }
                            .padding(.vertical, 4)
                        }
                        .listRowBackground(Color(hex: "0E0E10"))
                    }
                    .listStyle(.plain)
                }
            }
            .background(Color(hex: "0E0E10"))
            .navigationTitle("Swap Specimen")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel", action: onCancel)
                }
            }
        }
        .onAppear {
            stasisSpecimens = reefStore.loadStasisSpecimens()
        }
    }
}

// MARK: - FusionPickerView

/// Sheet presented when the user taps "Fuse" in the param panel.
/// Lists all reef specimens compatible for fusion (same category, Lv.3+).
struct FusionPickerView: View {
    let sourceSlot: Int
    let sourceSpecimen: Specimen
    let onFuse: (Int) -> Void
    let onCancel: () -> Void
    @EnvironmentObject var reefStore: ReefStore
    @State private var showConfirm = false
    @State private var selectedPartner: Int?

    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Source specimen header — shows what will be consumed
                HStack(spacing: 8) {
                    SpecimenSprite(subtype: sourceSpecimen.subtype, category: sourceSpecimen.category, size: 32)
                    Text(sourceSpecimen.creatureName)
                        .font(.custom("SpaceGrotesk-Bold", size: 14))
                        .foregroundColor(.white)
                    Text("Lv.\(sourceSpecimen.level)")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(.white.opacity(0.4))
                    Spacer()
                    Text("× ?")
                        .font(.custom("SpaceGrotesk-Bold", size: 16))
                        .foregroundColor(Color(hex: "B34DFF").opacity(0.5))
                }
                .padding(16)
                .background(Color.white.opacity(0.03))

                // Compatible specimens list
                let compatible = reefStore.specimens.enumerated().compactMap { (idx, spec) -> (Int, Specimen)? in
                    guard let s = spec, idx != sourceSlot, SpecimenFusion.canFuse(sourceSpecimen, s) else { return nil }
                    return (idx, s)
                }

                if compatible.isEmpty {
                    VStack(spacing: 8) {
                        Spacer()
                        Text("No compatible specimens")
                            .font(.custom("Inter-Regular", size: 13))
                            .foregroundColor(.white.opacity(0.3))
                        Text("Need another \(sourceSpecimen.category.rawValue) at Lv.3+")
                            .font(.custom("Inter-Regular", size: 10))
                            .foregroundColor(.white.opacity(0.2))
                        Spacer()
                    }
                } else {
                    List(compatible, id: \.0) { (idx, spec) in
                        Button(action: {
                            selectedPartner = idx
                            showConfirm = true
                        }) {
                            HStack(spacing: 10) {
                                SpecimenSprite(subtype: spec.subtype, category: spec.category, size: 28)
                                VStack(alignment: .leading) {
                                    Text(spec.creatureName)
                                        .font(.custom("SpaceGrotesk-Bold", size: 13))
                                        .foregroundColor(.white)
                                    Text("Lv.\(spec.level) · \(spec.rarity.rawValue)")
                                        .font(.custom("JetBrainsMono-Regular", size: 9))
                                        .foregroundColor(.white.opacity(0.4))
                                }
                                Spacer()
                            }
                        }
                        .listRowBackground(Color(hex: "0E0E10"))
                    }
                    .listStyle(.plain)
                }
            }
            .background(Color(hex: "0E0E10"))
            .navigationTitle("Fuse Specimens")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel", action: onCancel)
                }
            }
            .alert("Fuse Specimens?", isPresented: $showConfirm) {
                Button("Fuse", role: .destructive) {
                    if let partner = selectedPartner { onFuse(partner) }
                }
                Button("Cancel", role: .cancel) {}
            } message: {
                Text("Both parent specimens will be consumed. The child inherits averaged traits from both.")
            }
        }
    }
}
