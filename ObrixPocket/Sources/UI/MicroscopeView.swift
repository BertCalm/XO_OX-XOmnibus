import SwiftUI

/// Dual-language specimen detail: game stats ↔ synth params.
/// The Rosetta Stone that teaches synthesis through play.
struct MicroscopeView: View {
    let specimen: Specimen
    @State private var showSynthParams = false // Toggle between game/synth view

    var body: some View {
        ScrollView {
            VStack(spacing: 16) {
                // Header
                header

                // Toggle
                Picker("View", selection: $showSynthParams) {
                    Text("Creature").tag(false)
                    Text("Microscope").tag(true)
                }
                .pickerStyle(.segmented)
                .padding(.horizontal, 20)

                // Dual-language stats
                statsSection

                // Spectral DNA visualization
                SpectralFingerprint(
                    spectralDNA: specimen.spectralDNA,
                    color: catColor,
                    size: 180,
                    interactive: true
                )
                .padding(.vertical, 8)

                // Provenance
                provenanceSection

                // Evolution info
                if specimen.level >= 10 {
                    evolutionSection
                }
            }
            .padding(.vertical, 16)
        }
        .background(Color(hex: "0E0E10"))
    }

    // MARK: - Header

    private var header: some View {
        VStack(spacing: 6) {
            SpecimenSprite(subtype: specimen.subtype, category: specimen.category, size: 80)

            Text(specimen.name)
                .font(.custom("SpaceGrotesk-Bold", size: 22))
                .foregroundColor(.white)

            HStack(spacing: 8) {
                Text(specimen.rarity.rawValue.uppercased())
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(Color(hex: "E9C46A"))

                Text("Lv.\(specimen.level)")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(specimen.level >= 5 ? Color(hex: "E9C46A") : .white.opacity(0.5))

                Text(categoryLabel(specimen.category))
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(catColor.opacity(0.6))
            }

            // Personality
            if let entry = SpecimenCatalog.entry(for: specimen.subtype) {
                Text(entry.personalityLine)
                    .font(.custom("Inter-Regular", size: 12))
                    .foregroundColor(.white.opacity(0.35))
                    .italic()
            }
        }
    }

    // MARK: - Dual-Language Stats

    private var statsSection: some View {
        let stats = GameStats.from(params: specimen.parameterState)

        return VStack(spacing: 2) {
            dualRow("Warmth",     gameValue: stats.warmth,     synthParam: "Cutoff",       synthValue: formatHz(specimen.parameterState["obrix_flt1Cutoff"]))
            dualRow("Intensity",  gameValue: stats.intensity,  synthParam: "Resonance",    synthValue: formatPercent(specimen.parameterState["obrix_flt1Resonance"]))
            dualRow("Reflexes",   gameValue: stats.reflexes,   synthParam: "Attack",       synthValue: formatTime(specimen.parameterState["obrix_env1Attack"]))
            dualRow("Stamina",    gameValue: stats.stamina,    synthParam: "Decay+Release", synthValue: formatTime(specimen.parameterState["obrix_env1Decay"]))
            dualRow("Presence",   gameValue: stats.presence,   synthParam: "Level",        synthValue: formatPercent(specimen.parameterState["obrix_src1Level"]))
            dualRow("Complexity", gameValue: stats.complexity, synthParam: "Osc Type",     synthValue: formatOscType(specimen.parameterState["obrix_src1Type"]))
            dualRow("Pulse",      gameValue: stats.pulse,      synthParam: "LFO Rate",     synthValue: formatHz(specimen.parameterState["obrix_lfo1Rate"]))
            dualRow("Grit",       gameValue: stats.grit,       synthParam: "FX Param",     synthValue: formatPercent(specimen.parameterState["obrix_fx1Param1"]))
            dualRow("Aura",       gameValue: stats.aura,       synthParam: "FX Mix",       synthValue: formatPercent(specimen.parameterState["obrix_fx1Mix"]))
        }
        .padding(.horizontal, 20)
    }

    private func dualRow(_ gameName: String, gameValue: Int, synthParam: String, synthValue: String) -> some View {
        HStack(spacing: 0) {
            // Game stat (always visible)
            HStack(spacing: 6) {
                Text(gameName)
                    .font(.custom("Inter-Medium", size: 11))
                    .foregroundColor(.white.opacity(0.6))
                    .frame(width: 70, alignment: .trailing)

                // Mini bar
                GeometryReader { geo in
                    ZStack(alignment: .leading) {
                        RoundedRectangle(cornerRadius: 2)
                            .fill(Color.white.opacity(0.06))
                        RoundedRectangle(cornerRadius: 2)
                            .fill(catColor.opacity(0.5))
                            .frame(width: geo.size.width * CGFloat(gameValue) / 100)
                    }
                }
                .frame(width: 60, height: 6)

                Text("\(gameValue)")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.4))
                    .frame(width: 24, alignment: .trailing)
            }

            // Synth param (visible in Microscope mode)
            if showSynthParams {
                HStack(spacing: 4) {
                    Text("→")
                        .font(.system(size: 10))
                        .foregroundColor(.white.opacity(0.2))
                    Text(synthParam)
                        .font(.custom("JetBrainsMono-Regular", size: 9))
                        .foregroundColor(catColor.opacity(0.5))
                    Text(synthValue)
                        .font(.custom("JetBrainsMono-Regular", size: 9))
                        .foregroundColor(.white.opacity(0.3))
                }
                .transition(.opacity.combined(with: .move(edge: .trailing)))
            }

            Spacer()
        }
        .frame(height: 22)
        .animation(.easeInOut(duration: 0.2), value: showSynthParams)
    }

    // MARK: - Provenance

    private var provenanceSection: some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("PROVENANCE")
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))

            if let track = specimen.sourceTrackTitle {
                provenanceLine("Born from", value: track)
            }
            provenanceLine("Caught", value: DateFormatter.localizedString(from: specimen.catchTimestamp, dateStyle: .medium, timeStyle: .short))
            if let weather = specimen.catchWeatherDescription {
                provenanceLine("Weather", value: weather)
            }
            provenanceLine("Play time", value: formatPlayTime(specimen.totalPlaySeconds))
            provenanceLine("XP", value: "\(specimen.xp)")

            if specimen.aggressiveScore > 0 || specimen.gentleScore > 0 {
                let total = specimen.aggressiveScore + specimen.gentleScore
                let aggPct = total > 0 ? Int(specimen.aggressiveScore / total * 100) : 50
                provenanceLine("Play style", value: "\(aggPct)% aggressive / \(100 - aggPct)% gentle")
            }
        }
        .padding(.horizontal, 20)
    }

    private func provenanceLine(_ label: String, value: String) -> some View {
        HStack {
            Text(label)
                .font(.custom("Inter-Regular", size: 10))
                .foregroundColor(.white.opacity(0.3))
                .frame(width: 70, alignment: .trailing)
            Text(value)
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(.white.opacity(0.5))
                .lineLimit(1)
        }
    }

    // MARK: - Evolution

    private var evolutionSection: some View {
        Group {
            if let path = EvolutionCatalog.paths[specimen.subtype] {
                let isAggressive = specimen.aggressiveScore > specimen.gentleScore
                VStack(spacing: 4) {
                    HStack(spacing: 4) {
                        Image(systemName: isAggressive ? "bolt.fill" : "leaf.fill")
                        Text(isAggressive ? "Aggressive Evolution" : "Gentle Evolution")
                    }
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(Color(hex: "E9C46A").opacity(0.6))

                    Text("Could have been: \(isAggressive ? path.gentleName : path.aggressiveName)")
                        .font(.custom("Inter-Regular", size: 9))
                        .foregroundColor(.white.opacity(0.2))
                        .italic()
                }
            }
        }
    }

    // MARK: - Formatting

    private func formatHz(_ value: Float?) -> String {
        guard let v = value else { return "—" }
        if v < 1 { return String(format: "%.2f Hz", v) }
        if v < 1000 { return String(format: "%.0f Hz", v) }
        return String(format: "%.1f kHz", v / 1000)
    }

    private func formatPercent(_ value: Float?) -> String {
        guard let v = value else { return "—" }
        return "\(Int(v * 100))%"
    }

    private func formatTime(_ value: Float?) -> String {
        guard let v = value else { return "—" }
        if v < 0.01 { return "<1ms" }
        if v < 1 { return String(format: "%.0fms", v * 1000) }
        return String(format: "%.1fs", v)
    }

    private func formatOscType(_ value: Float?) -> String {
        guard let v = value else { return "—" }
        let types = ["Saw", "Square", "Tri", "Noise", "WT", "FM"]
        let idx = Int(v)
        return idx < types.count ? types[idx] : "Type \(idx)"
    }

    private func formatPlayTime(_ seconds: Double) -> String {
        if seconds < 60 { return "\(Int(seconds))s" }
        if seconds < 3600 { return "\(Int(seconds / 60))m" }
        return String(format: "%.1fh", seconds / 3600)
    }

    private func categoryLabel(_ cat: SpecimenCategory) -> String {
        switch cat {
        case .source:    return "Source"
        case .processor: return "Processor"
        case .modulator: return "Modulator"
        case .effect:    return "Effect"
        }
    }

    private var catColor: Color {
        categoryColor(for: specimen.category)
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    MicroscopeView(
        specimen: Specimen(
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
            spectralDNA: (0..<64).map { i in
                0.4 + 0.5 * sin(Float(i) * 0.4) * cos(Float(i) * 0.15)
            },
            parameterState: [
                "obrix_flt1Cutoff":    0.78,
                "obrix_flt1Resonance": 0.35,
                "obrix_env1Attack":    0.05,
                "obrix_env1Decay":     0.4,
                "obrix_env1Release":   0.5,
                "obrix_src1Level":     0.9,
                "obrix_src1Type":      1.0,
                "obrix_lfo1Rate":      3.2,
                "obrix_fx1Param1":     0.2,
                "obrix_fx1Mix":        0.4,
            ],
            catchLatitude: 37.7749,
            catchLongitude: -122.4194,
            catchTimestamp: Date().addingTimeInterval(-86400 * 3),
            catchWeatherDescription: "clear sky",
            creatureGenomeData: nil,
            cosmeticTier: .standard,
            morphIndex: 0,
            musicHash: nil,
            sourceTrackTitle: "John Coltrane — A Love Supreme",
            xp: 85,
            level: 3,
            aggressiveScore: 2.1,
            gentleScore: 0.8,
            totalPlaySeconds: 420
        )
    )
    .background(Color(hex: "0E0E10").ignoresSafeArea())
}
#endif
