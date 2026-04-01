import SwiftUI

/// Parameter editing panel for a selected reef specimen.
/// Appears between the reef grid and keyboard. Real-time engine updates.
struct SpecimenParamPanel: View {
    let slotIndex: Int
    var onDismiss: (() -> Void)? = nil
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var audioEngine: AudioEngineManager
    @State private var activeParam: String? // Which param is being dragged (for hover label)
    @State private var showAdvanced = false
    @State private var showReleaseConfirm = false
    @State private var showSwapPicker = false
    @State private var showFusionPicker = false
    @State private var isPreviewPlaying = false
    @State private var previewNote: Int32 = 60

    private var specimen: Specimen? { reefStore.specimens[slotIndex] }

    var body: some View {
        if let spec = specimen {
            VStack(spacing: 0) {
                // Drag handle — visual cue for swipe-to-dismiss
                RoundedRectangle(cornerRadius: 2)
                    .fill(Color.white.opacity(0.15))
                    .frame(width: 36, height: 4)
                    .padding(.top, 6)

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
                        .font(DesignTokens.heading(15))
                        .foregroundColor(.white)

                    Text(categoryLabel(spec.category).uppercased())
                        .font(DesignTokens.mono(10))
                        .tracking(1)
                        .foregroundColor(catColor(spec.category).opacity(0.6))

                    Text(synthDescription(spec.category))
                        .font(DesignTokens.mono(8))
                        .foregroundColor(.white.opacity(0.15))

                    // Level badge
                    Text("Lv.\(spec.level)")
                        .font(DesignTokens.monoBold(10))
                        .foregroundColor(spec.level >= 5
                            ? DesignTokens.xoGold
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
                            .foregroundColor((reefStore.specimens[slotIndex]?.isFavorite ?? false) ? DesignTokens.errorRed : .white.opacity(0.3))
                    }
                    .accessibilityLabel((reefStore.specimens[slotIndex]?.isFavorite ?? false) ? "Remove from favorites" : "Add to favorites")

                    // Close button — 28pt visible area, 44pt tap target
                    Button(action: { onDismiss?() }) {
                        Image(systemName: "xmark")
                            .font(.system(size: 12, weight: .bold))
                            .foregroundColor(.white.opacity(0.4))
                            .frame(width: 28, height: 28)
                            .background(Color.white.opacity(0.06))
                            .clipShape(Circle())
                    }
                    .accessibilityLabel("Close parameter panel")
                    .frame(minWidth: 44, minHeight: 44)
                    .contentShape(Rectangle())
                }
                .padding(.horizontal, 16)
                .padding(.vertical, 10)
                .background(Color.white.opacity(0.04)) // Subtle header separation

                // Category role description — one line teaching what this category does
                Text(categoryDescription(spec.category))
                    .font(DesignTokens.body(9))
                    .foregroundColor(.white.opacity(0.3))
                    .padding(.horizontal, 16)
                    .padding(.top, 4)

                // Hover label — shows param name + value while dragging
                if let paramName = activeParam,
                   let value = reefStore.specimens[slotIndex]?.parameterState[paramName] {
                    Text("\(paramDisplayName(paramName)): \(String(format: "%.2f", value))")
                        .font(DesignTokens.monoBold(13))
                        .foregroundColor(catColor(spec.category))
                        .padding(.vertical, 4)
                        .transition(.opacity)
                }

                // Preview note — lets user hear parameter changes in real time
                HStack(spacing: 12) {
                    Button(action: {
                        if isPreviewPlaying {
                            ObrixBridge.shared()?.noteOff(previewNote)
                            isPreviewPlaying = false
                        } else {
                            // Apply this specimen's chain to the engine first
                            audioEngine.applySlotChain(slotIndex: slotIndex, reefStore: reefStore)
                            previewNote = 60
                            ObrixBridge.shared()?.note(on: previewNote, velocity: 0.7)
                            isPreviewPlaying = true
                        }
                    }) {
                        HStack(spacing: 6) {
                            Image(systemName: isPreviewPlaying ? "stop.fill" : "play.fill")
                                .font(.system(size: 11))
                            Text(isPreviewPlaying ? "STOP" : "PREVIEW")
                                .font(DesignTokens.mono(10))
                                .tracking(1)
                        }
                        .foregroundColor(isPreviewPlaying ? DesignTokens.errorRed : catColor(spec.category))
                        .padding(.horizontal, 14)
                        .padding(.vertical, 8)
                        .background(
                            RoundedRectangle(cornerRadius: 8)
                                .fill(isPreviewPlaying ? DesignTokens.errorRed.opacity(0.1) : catColor(spec.category).opacity(0.1))
                        )
                    }

                    // Octave buttons for preview note range
                    HStack(spacing: 4) {
                        ForEach([48, 60, 72], id: \.self) { note in
                            Button(action: {
                                if isPreviewPlaying {
                                    ObrixBridge.shared()?.noteOff(previewNote)
                                }
                                audioEngine.applySlotChain(slotIndex: slotIndex, reefStore: reefStore)
                                previewNote = Int32(note)
                                ObrixBridge.shared()?.note(on: previewNote, velocity: 0.7)
                                isPreviewPlaying = true
                            }) {
                                Text(note == 48 ? "C3" : note == 60 ? "C4" : "C5")
                                    .font(DesignTokens.mono(9))
                                    .foregroundColor(.white.opacity(0.4))
                                    .frame(width: 32, height: 28)
                                    .background(Color.white.opacity(0.06))
                                    .clipShape(RoundedRectangle(cornerRadius: 4))
                            }
                        }
                    }

                    Spacer()
                }
                .padding(.horizontal, 16)
                .padding(.vertical, 6)

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

                    // Advanced parameters toggle
                    Button(action: { withAnimation { showAdvanced.toggle() } }) {
                        HStack(spacing: 8) {
                            Image(systemName: showAdvanced ? "chevron.down" : "chevron.right")
                                .font(.system(size: 10, weight: .medium))
                            Text("ADVANCED (\(showAdvanced ? "48" : "48") params)")
                                .font(DesignTokens.mono(10))
                                .tracking(1)
                            Spacer()
                            Text(showAdvanced ? "HIDE" : "SHOW")
                                .font(DesignTokens.mono(8))
                                .tracking(1.5)
                        }
                        .foregroundColor(showAdvanced ? DesignTokens.reefJade.opacity(0.6) : .white.opacity(0.4))
                        .padding(.horizontal, 12)
                        .padding(.vertical, 8)
                        .background(
                            RoundedRectangle(cornerRadius: 8)
                                .fill(showAdvanced ? DesignTokens.reefJade.opacity(0.06) : Color.white.opacity(0.03))
                        )
                    }
                    .padding(.top, 8)

                    if showAdvanced {
                        VStack(spacing: 8) {
                            sectionHeader("OSCILLATOR")
                            paramSlider("Waveform", paramKey: "obrix_src1Type", range: 0...5, unit: "", spec: spec)
                            paramSlider("Wavetable", paramKey: "obrix_wtBank", range: 0...3, unit: "", spec: spec)
                            paramSlider("Unison Detune", paramKey: "obrix_unisonDetune", range: 0...50, unit: "", spec: spec)

                            sectionHeader("FILTER MODE")
                            paramSlider("Filter Type", paramKey: "obrix_proc1Type", range: 0...4, unit: "", spec: spec)
                            paramSlider("Feedback", paramKey: "obrix_proc1Feedback", range: 0...1, unit: "", spec: spec)

                            sectionHeader("ECOLOGY")
                            paramSlider("Reef Mode", paramKey: "obrix_reefResident", range: 0...3, unit: "", spec: spec)
                            paramSlider("Resident Str", paramKey: "obrix_residentStrength", range: 0...1, unit: "", spec: spec)
                            paramSlider("Competition", paramKey: "obrix_competitionStrength", range: 0...1, unit: "", spec: spec)
                            paramSlider("Symbiosis", paramKey: "obrix_symbiosisStrength", range: 0...1, unit: "", spec: spec)

                            sectionHeader("ENVIRONMENT")
                            paramSlider("Temperature", paramKey: "obrix_envTemp", range: 0...1, unit: "", spec: spec)
                            paramSlider("Pressure", paramKey: "obrix_envPressure", range: 0...1, unit: "", spec: spec)
                            paramSlider("Current", paramKey: "obrix_envCurrent", range: -1...1, unit: "", spec: spec)
                            paramSlider("Turbidity", paramKey: "obrix_envTurbidity", range: 0...1, unit: "", spec: spec)

                            sectionHeader("SPATIAL")
                            paramSlider("Distance", paramKey: "obrix_distance", range: 0...1, unit: "", spec: spec)
                            paramSlider("Air", paramKey: "obrix_air", range: 0...1, unit: "", spec: spec)
                            paramSlider("Drift Rate", paramKey: "obrix_driftRate", range: 0.001...0.05, unit: "", spec: spec)
                            paramSlider("Drift Depth", paramKey: "obrix_driftDepth", range: 0...1, unit: "", spec: spec)

                            sectionHeader("SOURCE 2")
                            paramSlider("Src 2 Type", paramKey: "obrix_src2Type", range: 0...5, unit: "", spec: spec)
                            paramSlider("Src 2 Tune", paramKey: "obrix_src2Tune", range: -24...24, unit: "st", spec: spec)
                            paramSlider("Src 2 PW", paramKey: "obrix_src2PW", range: 0.05...0.95, unit: "", spec: spec)
                            paramSlider("Source Mix", paramKey: "obrix_srcMix", range: 0...1, unit: "", spec: spec)

                            sectionHeader("HARMONIC FIELD")
                            paramSlider("Field Str", paramKey: "obrix_fieldStrength", range: 0...1, unit: "", spec: spec)
                            paramSlider("Polarity", paramKey: "obrix_fieldPolarity", range: -1...1, unit: "", spec: spec)
                            paramSlider("Field Rate", paramKey: "obrix_fieldRate", range: 0.01...10, unit: "Hz", spec: spec)
                            paramSlider("Prime Limit", paramKey: "obrix_fieldPrimeLimit", range: 0...2, unit: "", spec: spec)

                            sectionHeader("MODULATION")
                            paramSlider("LFO Shape", paramKey: "obrix_lfo1Shape", range: 0...3, unit: "", spec: spec)
                            paramSlider("Mod 1 Depth", paramKey: "obrix_mod1Depth", range: -1...1, unit: "", spec: spec)
                            paramSlider("Mod 1 Rate", paramKey: "obrix_mod1Rate", range: 0.01...30, unit: "Hz", spec: spec)
                            paramSlider("FM Depth", paramKey: "obrix_fmDepth", range: 0...1, unit: "", spec: spec)

                            sectionHeader("STRESS / STATE")
                            paramSlider("Stress Decay", paramKey: "obrix_stressDecay", range: 0.01...2, unit: "s", spec: spec)
                            paramSlider("Bleach Rate", paramKey: "obrix_bleachRate", range: 0...0.01, unit: "", spec: spec)
                            paramSlider("State Reset", paramKey: "obrix_stateReset", range: 0...1, unit: "", spec: spec)

                            sectionHeader("VOICE")
                            paramSlider("Voice Mode", paramKey: "obrix_polyphony", range: 0...3, unit: "", spec: spec)
                            paramSlider("Glide Time", paramKey: "obrix_glideTime", range: 0...1, unit: "s", spec: spec)
                            paramSlider("Bend Range", paramKey: "obrix_pitchBendRange", range: 1...24, unit: "st", spec: spec)

                            sectionHeader("MACROS")
                            paramSlider("CHARACTER", paramKey: "obrix_macroCharacter", range: 0...1, unit: "", spec: spec)
                            paramSlider("MOVEMENT", paramKey: "obrix_macroMovement", range: 0...1, unit: "", spec: spec)
                            paramSlider("COUPLING", paramKey: "obrix_macroCoupling", range: 0...1, unit: "", spec: spec)
                            paramSlider("SPACE", paramKey: "obrix_macroSpace", range: 0...1, unit: "", spec: spec)

                            sectionHeader("FX CHAIN")
                            paramSlider("FX Mode", paramKey: "obrix_fxMode", range: 0...1, unit: "", spec: spec)
                            paramSlider("FX 2 Mix", paramKey: "obrix_fx2Mix", range: 0...1, unit: "", spec: spec)
                            paramSlider("FX 2 Tone", paramKey: "obrix_fx2Param", range: 0...1, unit: "", spec: spec)
                            paramSlider("FX 3 Mix", paramKey: "obrix_fx3Mix", range: 0...1, unit: "", spec: spec)
                            paramSlider("FX 3 Tone", paramKey: "obrix_fx3Param", range: 0...1, unit: "", spec: spec)
                        }
                        .transition(.opacity.combined(with: .move(edge: .top)))
                    }

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
                                .font(DesignTokens.body(10))
                            Text("(\(ReefEnergyManager.xpCost)⚡)")
                                .font(DesignTokens.mono(9))
                        }
                        .foregroundColor(ReefEnergyManager.shared.canAfford(ReefEnergyManager.xpCost)
                            ? DesignTokens.xoGold : .white.opacity(0.15))
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
                                .font(DesignTokens.body(11))
                        }
                        .foregroundColor(DesignTokens.xoGold.opacity(0.5))
                    }
                    .padding(.top, 4)

                    // Swap button — open stasis picker to directly swap this specimen
                    Button(action: { showSwapPicker = true }) {
                        HStack(spacing: 4) {
                            Image(systemName: "arrow.triangle.2.circlepath")
                                .font(.system(size: 10))
                            Text("Swap")
                                .font(DesignTokens.body(11))
                        }
                        .foregroundColor(DesignTokens.sourceColor.opacity(0.5))
                    }
                    .padding(.top, 4)

                    // Fusion button — requires level 3+
                    if let spec = specimen, spec.level >= 3 {
                        Button(action: { showFusionPicker = true }) {
                            HStack(spacing: 4) {
                                Image(systemName: "arrow.triangle.merge")
                                    .font(.system(size: 10))
                                Text("Fuse")
                                    .font(DesignTokens.body(11))
                            }
                            .foregroundColor(DesignTokens.effectColor.opacity(0.5))
                        }
                        .padding(.top, 4)
                    }

                    // Release button — lets user free up a full reef slot (destructive)
                    Button(action: { showReleaseConfirm = true }) {
                        HStack(spacing: 4) {
                            Image(systemName: "arrow.uturn.backward")
                                .font(.system(size: 10))
                            Text("Release")
                                .font(DesignTokens.body(11))
                        }
                        .foregroundColor(DesignTokens.errorRed.opacity(0.5))
                    }
                    .padding(.top, 4)
                }
                .padding(.horizontal, 16)
                .padding(.vertical, 10)
            }
            .onDisappear {
                if isPreviewPlaying {
                    ObrixBridge.shared()?.noteOff(previewNote)
                    ObrixBridge.shared()?.noteOff(48)
                    ObrixBridge.shared()?.noteOff(72)
                    isPreviewPlaying = false
                }
            }
            .background(
                ZStack {
                    DesignTokens.background
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
            .gesture(
                DragGesture().onEnded { value in
                    if value.translation.height > 50 { onDismiss?() }
                }
            )
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
                .font(DesignTokens.mono(9))
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
                    .font(DesignTokens.body(9))
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
                    .font(DesignTokens.body(9))
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
                    .font(DesignTokens.body(9))
                    .foregroundColor(catColor(spec.category).opacity(0.4))
            }
            Text("→ Filter Cutoff")
                .font(DesignTokens.body(9))
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
                    .font(DesignTokens.body(9))
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
                .font(DesignTokens.bodyMedium(11))
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
                .accessibilityLabel(label)
                .accessibilityValue(formatValue(liveValue, unit: unit, range: range))
                .accessibilityAddTraits(.updatesFrequently)
            }

            // Value readout
            Text(formatValue(liveValue, unit: unit, range: range))
                .font(DesignTokens.mono(10))
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

    /// Subtle synth vocabulary bridge — 8pt at 15% opacity in the header.
    /// Bridges game category names to their synth equivalents for curious users.
    private func synthDescription(_ category: SpecimenCategory) -> String {
        switch category {
        case .source:    return "oscillator"
        case .processor: return "filter / shaper"
        case .modulator: return "LFO / envelope"
        case .effect:    return "delay / reverb / distortion"
        }
    }

    private func catColor(_ category: SpecimenCategory) -> Color {
        DesignTokens.color(for: category)
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
                            .font(DesignTokens.heading(14))
                            .foregroundColor(.white)
                        Text("Currently in slot \(currentSlot + 1)")
                            .font(DesignTokens.body(10))
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
                            .font(DesignTokens.body(13))
                            .foregroundColor(.white.opacity(0.3))
                        Text("Move specimens to stasis from the param panel to swap later")
                            .font(DesignTokens.body(10))
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
                                        .font(DesignTokens.heading(13))
                                        .foregroundColor(.white)
                                    HStack(spacing: 6) {
                                        Text(specimen.rarity.rawValue.uppercased())
                                            .font(DesignTokens.mono(8))
                                            .foregroundColor(DesignTokens.xoGold)
                                        Text("Lv.\(specimen.level)")
                                            .font(DesignTokens.mono(8))
                                            .foregroundColor(.white.opacity(0.4))
                                    }
                                }
                                Spacer()
                                Image(systemName: "arrow.right.circle")
                                    .foregroundColor(DesignTokens.reefJade.opacity(0.5))
                            }
                            .padding(.vertical, 4)
                        }
                        .listRowBackground(DesignTokens.background)
                    }
                    .listStyle(.plain)
                }
            }
            .background(DesignTokens.background)
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
    @EnvironmentObject var audioEngine: AudioEngineManager
    @State private var selectedPartner: Int?
    @State private var showPreview = false
    @State private var blendAmount: Float = 0.5

    var body: some View {
        NavigationView {
            VStack(spacing: 0) {
                // Source specimen header — shows what will be consumed
                HStack(spacing: 8) {
                    SpecimenSprite(subtype: sourceSpecimen.subtype, category: sourceSpecimen.category, size: 32)
                    Text(sourceSpecimen.creatureName)
                        .font(DesignTokens.heading(14))
                        .foregroundColor(.white)
                    Text("Lv.\(sourceSpecimen.level)")
                        .font(DesignTokens.mono(10))
                        .foregroundColor(.white.opacity(0.4))
                    Spacer()
                    Text("× ?")
                        .font(DesignTokens.heading(16))
                        .foregroundColor(DesignTokens.effectColor.opacity(0.5))
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
                            .font(DesignTokens.body(13))
                            .foregroundColor(.white.opacity(0.3))
                        Text("Need another \(sourceSpecimen.category.rawValue) at Lv.3+")
                            .font(DesignTokens.body(10))
                            .foregroundColor(.white.opacity(0.2))
                        Spacer()
                    }
                } else {
                    List(compatible, id: \.0) { (idx, spec) in
                        Button(action: {
                            selectedPartner = idx
                            showPreview = true
                        }) {
                            HStack(spacing: 10) {
                                SpecimenSprite(subtype: spec.subtype, category: spec.category, size: 28)
                                VStack(alignment: .leading) {
                                    Text(spec.creatureName)
                                        .font(DesignTokens.heading(13))
                                        .foregroundColor(.white)
                                    Text("Lv.\(spec.level) · \(spec.rarity.rawValue)")
                                        .font(DesignTokens.mono(9))
                                        .foregroundColor(.white.opacity(0.4))
                                }
                                Spacer()
                            }
                        }
                        .listRowBackground(DesignTokens.background)
                    }
                    .listStyle(.plain)
                }
            }
            .background(DesignTokens.background)
            .navigationTitle("Fuse Specimens")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel", action: onCancel)
                }
            }
            .sheet(isPresented: $showPreview) {
                if let partnerIdx = selectedPartner,
                   let partner = reefStore.specimens[partnerIdx] {
                    FusionPreviewView(
                        sourceSpecimen: sourceSpecimen,
                        partnerSpecimen: partner,
                        blendAmount: $blendAmount,
                        onPreview: { blend in
                            previewFusionAudio(partnerSlot: partnerIdx, blend: blend)
                        },
                        onFuse: {
                            showPreview = false
                            onFuse(partnerIdx)
                        },
                        onCancel: {
                            showPreview = false
                            // Restore original params
                            if let activeSlot = reefStore.specimens.firstIndex(where: { $0 != nil }) {
                                audioEngine.applyCachedParams(for: activeSlot)
                            }
                        }
                    )
                }
            }
        }
    }

    private func previewFusionAudio(partnerSlot: Int, blend: Float) {
        guard let partner = reefStore.specimens[partnerSlot] else { return }

        // Interpolate parameters between source and partner
        let allKeys = Set(sourceSpecimen.parameterState.keys).union(partner.parameterState.keys)
        for key in allKeys {
            let aVal = sourceSpecimen.parameterState[key] ?? 0.5
            let bVal = partner.parameterState[key] ?? 0.5
            let blended = aVal * (1.0 - blend) + bVal * blend
            ObrixBridge.shared()?.setParameterImmediate(key, value: blended)
        }

        // Play preview chord
        ObrixBridge.shared()?.note(on: 60, velocity: 0.7)
        ObrixBridge.shared()?.note(on: 67, velocity: 0.55)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) {
            ObrixBridge.shared()?.noteOff(60)
            ObrixBridge.shared()?.noteOff(67)
        }
    }
}

// MARK: - FusionPreviewView

/// Preview panel shown before committing a fusion.
/// Blend slider interpolates between parent timbres in real-time.
struct FusionPreviewView: View {
    let sourceSpecimen: Specimen
    let partnerSpecimen: Specimen
    @Binding var blendAmount: Float
    let onPreview: (Float) -> Void
    let onFuse: () -> Void
    let onCancel: () -> Void

    var body: some View {
        VStack(spacing: 20) {
            Text("FUSION ORACLE")
                .font(DesignTokens.heading(16))
                .tracking(2)
                .foregroundColor(DesignTokens.effectColor)
                .padding(.top, 24)

            // Parent specimens
            HStack(spacing: 24) {
                VStack(spacing: 6) {
                    SpecimenSprite(subtype: sourceSpecimen.subtype, category: sourceSpecimen.category, size: 40)
                    Text(sourceSpecimen.creatureName)
                        .font(DesignTokens.mono(10))
                        .foregroundColor(.white.opacity(0.7))
                    Text("Lv.\(sourceSpecimen.level)")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.3))
                }

                Image(systemName: "arrow.triangle.merge")
                    .font(.system(size: 20))
                    .foregroundColor(DesignTokens.effectColor.opacity(0.5))

                VStack(spacing: 6) {
                    SpecimenSprite(subtype: partnerSpecimen.subtype, category: partnerSpecimen.category, size: 40)
                    Text(partnerSpecimen.creatureName)
                        .font(DesignTokens.mono(10))
                        .foregroundColor(.white.opacity(0.7))
                    Text("Lv.\(partnerSpecimen.level)")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.3))
                }
            }

            // Blend slider
            VStack(spacing: 8) {
                Text("BLEND")
                    .font(DesignTokens.mono(9))
                    .tracking(1.5)
                    .foregroundColor(.white.opacity(0.4))

                HStack(spacing: 12) {
                    Text(sourceSpecimen.creatureName.prefix(8))
                        .font(DesignTokens.mono(8))
                        .foregroundColor(.white.opacity(0.3))

                    Slider(value: Binding(
                        get: { Double(blendAmount) },
                        set: { blendAmount = Float($0) }
                    ), in: 0...1)
                    .tint(DesignTokens.effectColor)

                    Text(partnerSpecimen.creatureName.prefix(8))
                        .font(DesignTokens.mono(8))
                        .foregroundColor(.white.opacity(0.3))
                }
                .padding(.horizontal, 20)

                Text("\(Int(blendAmount * 100))% blend")
                    .font(DesignTokens.mono(10))
                    .foregroundColor(DesignTokens.effectColor.opacity(0.6))
            }

            // Preview button
            Button(action: { onPreview(blendAmount) }) {
                HStack(spacing: 6) {
                    Image(systemName: "speaker.wave.2")
                    Text("PREVIEW")
                }
                .font(DesignTokens.heading(14))
                .foregroundColor(.white)
                .frame(maxWidth: .infinity)
                .frame(height: 44)
                .background(RoundedRectangle(cornerRadius: 22).fill(DesignTokens.effectColor.opacity(0.4)))
            }
            .padding(.horizontal, 40)

            // Warning
            Text("Both parents will be consumed")
                .font(DesignTokens.body(10))
                .foregroundColor(DesignTokens.errorRed.opacity(0.5))

            // Action buttons
            HStack(spacing: 16) {
                Button(action: onCancel) {
                    Text("Cancel")
                        .font(DesignTokens.heading(14))
                        .foregroundColor(.white.opacity(0.5))
                        .frame(maxWidth: .infinity)
                        .frame(height: 44)
                        .background(RoundedRectangle(cornerRadius: 22).stroke(Color.white.opacity(0.15)))
                }

                Button(action: onFuse) {
                    Text("FUSE")
                        .font(DesignTokens.heading(14))
                        .tracking(2)
                        .foregroundColor(.white)
                        .frame(maxWidth: .infinity)
                        .frame(height: 44)
                        .background(RoundedRectangle(cornerRadius: 22).fill(DesignTokens.effectColor))
                }
            }
            .padding(.horizontal, 40)

            Spacer()
        }
        .background(DesignTokens.darkBackground.ignoresSafeArea())
    }
}
