import SwiftUI

struct CompareView: View {
    let specimenA: Specimen
    let specimenB: Specimen

    var body: some View {
        ScrollView {
            VStack(spacing: 16) {
                // Header
                Text("COMPARE")
                    .font(.custom("SpaceGrotesk-Bold", size: 18))
                    .foregroundColor(.white)
                    .padding(.top, 16)

                // Side by side sprites
                HStack(spacing: 24) {
                    specimenColumn(specimenA)

                    // VS divider
                    Text("vs")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(.white.opacity(0.2))

                    specimenColumn(specimenB)
                }
                .padding(.horizontal, 20)

                // Stat comparison bars
                statComparison

                // Provenance comparison
                provenanceComparison
            }
        }
        .background(Color(hex: "0E0E10").ignoresSafeArea())
    }

    private func specimenColumn(_ specimen: Specimen) -> some View {
        VStack(spacing: 6) {
            SpecimenSprite(subtype: specimen.subtype, category: specimen.category, size: 56)

            Text(specimen.creatureName)
                .font(.custom("SpaceGrotesk-Bold", size: 14))
                .foregroundColor(.white)
                .lineLimit(1)

            Text(specimen.rarity.rawValue.uppercased())
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .foregroundColor(Color(hex: "E9C46A"))

            Text("Lv.\(specimen.level)")
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .foregroundColor(.white.opacity(0.4))
        }
        .frame(maxWidth: .infinity)
    }

    private var statComparison: some View {
        let statsA = GameStats.from(params: specimenA.parameterState)
        let statsB = GameStats.from(params: specimenB.parameterState)

        let pairs: [(String, Int, Int)] = [
            ("Warmth",     statsA.warmth,     statsB.warmth),
            ("Intensity",  statsA.intensity,  statsB.intensity),
            ("Reflexes",   statsA.reflexes,   statsB.reflexes),
            ("Stamina",    statsA.stamina,    statsB.stamina),
            ("Presence",   statsA.presence,   statsB.presence),
            ("Complexity", statsA.complexity, statsB.complexity),
            ("Pulse",      statsA.pulse,      statsB.pulse),
            ("Grit",       statsA.grit,       statsB.grit),
            ("Aura",       statsA.aura,       statsB.aura),
        ]

        return VStack(spacing: 4) {
            Text("STATS")
                .font(.custom("JetBrainsMono-Regular", size: 9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))

            ForEach(pairs, id: \.0) { name, valA, valB in
                HStack(spacing: 4) {
                    // A value
                    Text("\(valA)")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(valA > valB ? Color(hex: "1E8B7E") : .white.opacity(0.4))
                        .frame(width: 28, alignment: .trailing)

                    // A bar (right-aligned, grows left)
                    GeometryReader { geo in
                        HStack {
                            Spacer()
                            RoundedRectangle(cornerRadius: 2)
                                .fill(categoryColor(for: specimenA.category).opacity(0.5))
                                .frame(width: geo.size.width * CGFloat(valA) / 100, height: 6)
                        }
                    }
                    .frame(height: 6)

                    // Label
                    Text(name)
                        .font(.custom("Inter-Regular", size: 8))
                        .foregroundColor(.white.opacity(0.3))
                        .frame(width: 55)

                    // B bar (left-aligned, grows right)
                    GeometryReader { geo in
                        RoundedRectangle(cornerRadius: 2)
                            .fill(categoryColor(for: specimenB.category).opacity(0.5))
                            .frame(width: geo.size.width * CGFloat(valB) / 100, height: 6)
                    }
                    .frame(height: 6)

                    // B value
                    Text("\(valB)")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(valB > valA ? Color(hex: "1E8B7E") : .white.opacity(0.4))
                        .frame(width: 28, alignment: .leading)
                }
                .frame(height: 16)
            }
        }
        .padding(.horizontal, 20)
    }

    private var provenanceComparison: some View {
        HStack(alignment: .top, spacing: 20) {
            VStack(alignment: .leading, spacing: 2) {
                Text("XP: \(specimenA.xp)")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.3))
                Text("Play: \(formatTime(specimenA.totalPlaySeconds))")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.3))
                if let track = specimenA.sourceTrackTitle {
                    Text(track)
                        .font(.custom("Inter-Regular", size: 8))
                        .foregroundColor(.white.opacity(0.2))
                        .lineLimit(1)
                }
            }
            .frame(maxWidth: .infinity, alignment: .leading)

            VStack(alignment: .leading, spacing: 2) {
                Text("XP: \(specimenB.xp)")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.3))
                Text("Play: \(formatTime(specimenB.totalPlaySeconds))")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.3))
                if let track = specimenB.sourceTrackTitle {
                    Text(track)
                        .font(.custom("Inter-Regular", size: 8))
                        .foregroundColor(.white.opacity(0.2))
                        .lineLimit(1)
                }
            }
            .frame(maxWidth: .infinity, alignment: .leading)
        }
        .padding(.horizontal, 20)
        .padding(.bottom, 24)
    }

    private func formatTime(_ seconds: Double) -> String {
        if seconds < 60 { return "\(Int(seconds))s" }
        if seconds < 3600 { return "\(Int(seconds / 60))m" }
        return String(format: "%.1fh", seconds / 3600)
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    let specimenA = Specimen(
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
        parameterState: [
            "obrix_flt1Cutoff": 0.78,
            "obrix_flt1Resonance": 0.35,
            "obrix_env1Attack": 0.05,
            "obrix_env1Decay": 0.4,
            "obrix_src1Level": 0.9,
            "obrix_src1Type": 1.0,
            "obrix_lfo1Rate": 3.2,
            "obrix_fx1Param1": 0.2,
            "obrix_fx1Mix": 0.4,
        ],
        catchLatitude: nil,
        catchLongitude: nil,
        catchTimestamp: Date(),
        catchWeatherDescription: nil,
        creatureGenomeData: nil,
        cosmeticTier: .standard,
        morphIndex: 0,
        musicHash: nil,
        sourceTrackTitle: "John Coltrane — A Love Supreme",
        xp: 85,
        level: 3,
        aggressiveScore: 2.1,
        gentleScore: 0.8,
        totalPlaySeconds: 420,
        journal: [],
        isFavorite: false
    )
    let specimenB = Specimen(
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
        parameterState: [
            "obrix_flt1Cutoff": 0.45,
            "obrix_flt1Resonance": 0.6,
            "obrix_env1Attack": 0.2,
            "obrix_env1Decay": 0.7,
            "obrix_src1Level": 0.5,
            "obrix_src1Type": 3.0,
            "obrix_lfo1Rate": 0.8,
            "obrix_fx1Param1": 0.65,
            "obrix_fx1Mix": 0.8,
        ],
        catchLatitude: nil,
        catchLongitude: nil,
        catchTimestamp: Date(),
        catchWeatherDescription: nil,
        creatureGenomeData: nil,
        cosmeticTier: .standard,
        morphIndex: 0,
        musicHash: nil,
        sourceTrackTitle: nil,
        xp: 42,
        level: 2,
        aggressiveScore: 0.5,
        gentleScore: 1.8,
        totalPlaySeconds: 190,
        journal: [],
        isFavorite: false
    )
    return CompareView(specimenA: specimenA, specimenB: specimenB)
}
#endif
