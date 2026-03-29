import SwiftUI

/// Expandable Sound Memory panel — inserted inside MicroscopeView's ScrollView
/// between the Spectral Fingerprint and the Provenance section.
///
/// Usage:
///   SoundMemoryPanel(specimenID: specimen.id, memoryManager: soundMemoryManager)
struct SoundMemoryPanel: View {

    let specimenID: UUID
    @ObservedObject var memoryManager: SoundMemoryManager

    private var memory: MusicalMemory { memoryManager.getMemory(for: specimenID) }
    private var influence: MemoryInfluence { memoryManager.getInfluence(for: specimenID) }

    var body: some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing12) {
            panelHeader
            characterSummaryRow
            influenceBars
            exposureSummary
            footerLine
        }
        .padding(.horizontal, DesignTokens.spacing20)
    }

    // MARK: - Panel Header

    private var panelHeader: some View {
        HStack(alignment: .firstTextBaseline, spacing: DesignTokens.spacing8) {
            Text("SOUND MEMORY")
                .font(DesignTokens.mono(9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))

            Spacer()

            saturationIndicator
        }
    }

    private var saturationIndicator: some View {
        HStack(spacing: DesignTokens.spacing4) {
            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 2)
                        .fill(Color.white.opacity(0.07))
                    RoundedRectangle(cornerRadius: 2)
                        .fill(saturationColor)
                        .frame(width: geo.size.width * CGFloat(memory.memorySaturation))
                }
            }
            .frame(width: 60, height: 4)

            Text("\(Int(memory.memorySaturation * 100))%")
                .font(DesignTokens.mono(9))
                .foregroundColor(saturationColor.opacity(0.7))
                .frame(width: 28, alignment: .trailing)
        }
    }

    private var saturationColor: Color {
        switch memory.memorySaturation {
        case 0..<0.2: return .white.opacity(0.3)
        case 0.2..<0.6: return DesignTokens.xoGold
        default: return DesignTokens.reefJade
        }
    }

    // MARK: - Character Summary

    private var characterSummaryRow: some View {
        Text(influence.characterSummary)
            .font(DesignTokens.body(11))
            .foregroundColor(.white.opacity(memory.hasMeaningfulData ? 0.55 : 0.25))
            .italic()
            .lineLimit(2)
    }

    // MARK: - Influence Bars (5 axes)

    private var influenceBars: some View {
        VStack(spacing: DesignTokens.spacing6) {
            influenceBar(
                label: "Attack",
                leftLabel: "soft",
                rightLabel: "sharp",
                value: influence.attackSharpnessDelta
            )
            influenceBar(
                label: "Sustain",
                leftLabel: "staccato",
                rightLabel: "legato",
                value: influence.sustainLengthDelta
            )
            influenceBar(
                label: "Velocity",
                leftLabel: "flat",
                rightLabel: "dynamic",
                value: influence.velocityVariationDelta
            )
            influenceBar(
                label: "Swing",
                leftLabel: "straight",
                rightLabel: "groovy",
                value: influence.swingDelta
            )
            influenceBar(
                label: "Register",
                leftLabel: "low",
                rightLabel: "high",
                value: influence.registerBias
            )
        }
        .padding(DesignTokens.spacing12)
        .background(
            RoundedRectangle(cornerRadius: 10)
                .fill(Color.white.opacity(0.03))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .strokeBorder(Color.white.opacity(0.06), lineWidth: 1)
        )
    }

    /// Bipolar bar: center is 0. Positive extends right (toward rightLabel),
    /// negative extends left (toward leftLabel). Range is -0.15…+0.15.
    private func influenceBar(label: String, leftLabel: String, rightLabel: String, value: Float) -> some View {
        HStack(spacing: DesignTokens.spacing8) {
            Text(label)
                .font(DesignTokens.bodyMedium(10))
                .foregroundColor(.white.opacity(0.4))
                .frame(width: 56, alignment: .trailing)

            GeometryReader { geo in
                let maxMag: CGFloat = 0.15
                let normalized = CGFloat(value) / maxMag           // -1…+1
                let centerX = geo.size.width / 2

                ZStack(alignment: .center) {
                    // Track
                    RoundedRectangle(cornerRadius: 2)
                        .fill(Color.white.opacity(0.07))

                    // Center tick mark
                    Rectangle()
                        .fill(Color.white.opacity(0.15))
                        .frame(width: 1, height: 10)
                        .position(x: centerX, y: geo.size.height / 2)

                    // Filled bar
                    if abs(normalized) > 0.02 {
                        let barWidth = abs(normalized) * (geo.size.width / 2)
                        let xPos: CGFloat = normalized > 0
                            ? centerX + barWidth / 2
                            : centerX - barWidth / 2

                        RoundedRectangle(cornerRadius: 2)
                            .fill(barColor(value: value))
                            .frame(width: barWidth)
                            .position(x: xPos, y: geo.size.height / 2)
                    }
                }
            }
            .frame(height: 8)

            HStack(spacing: DesignTokens.spacing4) {
                Text(leftLabel)
                    .font(DesignTokens.mono(8))
                    .foregroundColor(.white.opacity(0.2))
                    .frame(width: 36, alignment: .trailing)
                    .opacity(value < -0.02 ? 0.8 : 0.3)

                Text(rightLabel)
                    .font(DesignTokens.mono(8))
                    .foregroundColor(.white.opacity(0.2))
                    .frame(width: 36, alignment: .leading)
                    .opacity(value > 0.02 ? 0.8 : 0.3)
            }
        }
    }

    private func barColor(value: Float) -> Color {
        abs(value) > 0.1 ? DesignTokens.xoGold : DesignTokens.reefJade
    }

    // MARK: - Exposure Summary

    private var exposureSummary: some View {
        HStack(spacing: DesignTokens.spacing12) {
            exposureTile("Scale", value: scaleDisplayName(memory.dominantScale))
            exposureTile("Tempo", value: "\(Int(memory.dominantTempo)) BPM")
            exposureTile("Total", value: minutesLabel(memory.totalExposureMinutes))
        }
    }

    private func exposureTile(_ label: String, value: String) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(label)
                .font(DesignTokens.mono(8))
                .tracking(1)
                .foregroundColor(.white.opacity(0.2))
            Text(value)
                .font(DesignTokens.mono(10))
                .foregroundColor(memory.hasMeaningfulData ? .white.opacity(0.6) : .white.opacity(0.2))
                .lineLimit(1)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }

    private func scaleDisplayName(_ scale: String) -> String {
        switch scale {
        case "major":          return "Major"
        case "minor":          return "Minor"
        case "dorian":         return "Dorian"
        case "pentatonic":     return "Penta."
        case "blues":          return "Blues"
        case "chromatic":      return "Chrom."
        case "wholeTone":      return "Whole Tone"
        case "pentatonicMajor": return "Penta.Maj"
        case "pentatonicMinor": return "Penta.Min"
        case "harmonicMinor":  return "Harm.Min"
        case "mixolydian":     return "Mixolyd."
        case "phrygian":       return "Phrygian"
        case "lydian":         return "Lydian"
        default:               return scale.capitalized
        }
    }

    private func minutesLabel(_ minutes: Float) -> String {
        let m = Int(minutes)
        if m < 1 { return "<1 min" }
        if m < 60 { return "\(m) min" }
        let h = m / 60
        let rem = m % 60
        return rem == 0 ? "\(h)h" : "\(h)h \(rem)m"
    }

    // MARK: - Footer

    private var footerLine: some View {
        let minutes = Int(memory.totalExposureMinutes)
        let text: String
        if !memory.hasMeaningfulData {
            text = "This voice has heard no music yet"
        } else if minutes == 1 {
            text = "This voice has been shaped by 1 minute of music"
        } else {
            text = "This voice has been shaped by \(minutesLabel(memory.totalExposureMinutes)) of music"
        }

        return Text(text)
            .font(DesignTokens.body(10))
            .foregroundColor(.white.opacity(0.2))
            .italic()
            .frame(maxWidth: .infinity, alignment: .leading)
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    let manager = SoundMemoryManager()
    let specimenID = UUID()

    // Seed some data so the preview is non-trivial
    manager.recordExposure(
        specimenId: specimenID,
        scale: "major",
        tempo: 100,
        register: 5,
        duration: 800,
        genre: "funk"
    )
    manager.recordExposure(
        specimenId: specimenID,
        scale: "pentatonicMajor",
        tempo: 80,
        register: 4,
        duration: 400,
        genre: "ambient"
    )

    return ScrollView {
        SoundMemoryPanel(specimenID: specimenID, memoryManager: manager)
            .padding(.vertical, DesignTokens.spacing24)
    }
    .background(DesignTokens.background.ignoresSafeArea())
}
#endif
