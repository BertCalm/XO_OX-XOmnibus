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

                    // Release button — lets user free up a full reef slot
                    Button(action: { showReleaseConfirm = true }) {
                        HStack(spacing: 4) {
                            Image(systemName: "arrow.uturn.backward")
                                .font(.system(size: 10))
                            Text("Release")
                                .font(.custom("Inter-Regular", size: 11))
                        }
                        .foregroundColor(Color(hex: "FF4D4D").opacity(0.5))
                    }
                    .padding(.top, 8)
                }
                .padding(.horizontal, 16)
                .padding(.vertical, 10)
            }
            .background(Color(hex: "0E0E10"))
            .overlay(
                RoundedRectangle(cornerRadius: 12)
                    .stroke(catColor(spec.category).opacity(0.15), lineWidth: 1)
            )
            .cornerRadius(12)
            .padding(.horizontal, 12)
            .alert("Release \(specimen?.creatureName ?? "specimen")?", isPresented: $showReleaseConfirm) {
                Button("Release", role: .destructive) {
                    reefStore.releaseSpecimen(at: slotIndex)
                    reefStore.save()
                    onDismiss?()
                }
                Button("Cancel", role: .cancel) {}
            } message: {
                Text("It will return to the wild. This cannot be undone.")
            }
        }
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
