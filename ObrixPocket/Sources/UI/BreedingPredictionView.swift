import SwiftUI

/// Pre-breeding preview sheet: shows trait predictions, compatibility ring,
/// and a confirm button before the breed() call is made.
///
/// Present as a sheet when the player examines an eligible pair:
///   .sheet(isPresented: …) { BreedingPredictionView(prediction: …, onBreed: …) }
struct BreedingPredictionView: View {

    let prediction: BreedingPrediction
    /// Called when the player taps "Begin Breeding". The caller is responsible
    /// for dismissing the sheet and invoking BreedingManager.breed().
    let onBreed: () -> Void

    @Environment(\.dismiss) private var dismiss

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(spacing: DesignTokens.spacing24) {
                    parentHeader
                    compatibilityRing
                    predictionGrid
                    mutationRow
                    metaRow
                    breedButton
                }
                .padding(.horizontal, DesignTokens.spacing20)
                .padding(.vertical, DesignTokens.spacing24)
            }
            .background(DesignTokens.panelBackground.ignoresSafeArea())
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .cancellationAction) {
                    Button("Cancel") { dismiss() }
                        .font(DesignTokens.bodyMedium(14))
                        .foregroundColor(.white.opacity(0.5))
                }
            }
        }
    }

    // MARK: - Parent Header

    private var parentHeader: some View {
        VStack(spacing: DesignTokens.spacing8) {
            Text("Breeding Preview")
                .font(DesignTokens.heading(20))
                .foregroundColor(.white)

            HStack(spacing: DesignTokens.spacing8) {
                Text(prediction.parentNameA)
                    .font(DesignTokens.bodyMedium(13))
                    .foregroundColor(.white.opacity(0.75))

                Text("x")
                    .font(DesignTokens.mono(12))
                    .foregroundColor(.white.opacity(0.3))

                Text(prediction.parentNameB)
                    .font(DesignTokens.bodyMedium(13))
                    .foregroundColor(.white.opacity(0.75))
            }
        }
    }

    // MARK: - Compatibility Ring

    private var compatibilityRing: some View {
        VStack(spacing: DesignTokens.spacing12) {
            ZStack {
                // Track ring
                Circle()
                    .stroke(Color.white.opacity(0.08), lineWidth: 10)
                    .frame(width: 110, height: 110)

                // Filled arc
                Circle()
                    .trim(from: 0, to: CGFloat(compatScore))
                    .stroke(compatColor, style: StrokeStyle(lineWidth: 10, lineCap: .round))
                    .frame(width: 110, height: 110)
                    .rotationEffect(.degrees(-90))

                VStack(spacing: 2) {
                    Text("\(Int(compatScore * 100))%")
                        .font(DesignTokens.monoBold(22))
                        .foregroundColor(.white)
                    Text("Compatibility")
                        .font(DesignTokens.mono(9))
                        .foregroundColor(.white.opacity(0.35))
                        .tracking(1)
                }
            }

            Text(compatLabel)
                .font(DesignTokens.body(12))
                .foregroundColor(compatColor.opacity(0.85))
                .italic()
        }
        .padding(.vertical, DesignTokens.spacing8)
    }

    // MARK: - Trait Prediction Grid

    private var predictionGrid: some View {
        VStack(spacing: DesignTokens.spacing2) {
            sectionLabel("TRAIT PREDICTIONS")

            VStack(spacing: 0) {
                ForEach(Array(prediction.traitPredictions.enumerated()), id: \.offset) { _, trait in
                    TraitPredictionRow(prediction: trait)

                    if trait.trait != prediction.traitPredictions.last?.trait {
                        Divider()
                            .background(Color.white.opacity(0.05))
                    }
                }
            }
            .background(
                RoundedRectangle(cornerRadius: 10)
                    .fill(Color.white.opacity(0.04))
            )
            .overlay(
                RoundedRectangle(cornerRadius: 10)
                    .strokeBorder(Color.white.opacity(0.07), lineWidth: 1)
            )
        }
    }

    // MARK: - Mutation Row

    private var mutationRow: some View {
        HStack(spacing: DesignTokens.spacing12) {
            VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
                Text("Mutation Chance")
                    .font(DesignTokens.mono(10))
                    .tracking(1)
                    .foregroundColor(.white.opacity(0.3))
                Text("\(Int(prediction.mutationProbability * 100))%")
                    .font(DesignTokens.monoBold(18))
                    .foregroundColor(mutationColor)
            }

            Spacer()

            if prediction.mutationProbability > 0.1 {
                Text("Mutation possible")
                    .font(DesignTokens.body(11))
                    .foregroundColor(DesignTokens.xoGold.opacity(0.7))
                    .italic()
            }
        }
        .padding(DesignTokens.spacing16)
        .background(
            RoundedRectangle(cornerRadius: 10)
                .fill(Color.white.opacity(0.04))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .strokeBorder(Color.white.opacity(0.07), lineWidth: 1)
        )
    }

    // MARK: - Generation / Confidence Meta Row

    private var metaRow: some View {
        HStack(spacing: DesignTokens.spacing12) {
            metaTile(
                label: "GENERATION",
                value: "Gen-\(prediction.predictedGeneration)",
                color: DesignTokens.xoGold
            )
            metaTile(
                label: "CONFIDENCE",
                value: "\(Int(prediction.confidence * 100))%",
                color: .white.opacity(0.65)
            )
        }
    }

    private func metaTile(label: String, value: String, color: Color) -> some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing4) {
            Text(label)
                .font(DesignTokens.mono(9))
                .tracking(1)
                .foregroundColor(.white.opacity(0.25))
            Text(value)
                .font(DesignTokens.monoBold(20))
                .foregroundColor(color)
        }
        .frame(maxWidth: .infinity, alignment: .leading)
        .padding(DesignTokens.spacing16)
        .background(
            RoundedRectangle(cornerRadius: 10)
                .fill(Color.white.opacity(0.04))
        )
        .overlay(
            RoundedRectangle(cornerRadius: 10)
                .strokeBorder(Color.white.opacity(0.07), lineWidth: 1)
        )
    }

    // MARK: - Breed Button

    private var breedButton: some View {
        Button(action: {
            onBreed()
            dismiss()
        }) {
            Text("Begin Breeding")
                .font(DesignTokens.bodyMedium(16))
                .foregroundColor(.black)
                .frame(maxWidth: .infinity)
                .padding(.vertical, DesignTokens.spacing16)
                .background(
                    RoundedRectangle(cornerRadius: 12)
                        .fill(DesignTokens.reefJade)
                )
        }
        .padding(.top, DesignTokens.spacing8)
    }

    // MARK: - Helpers

    private func sectionLabel(_ text: String) -> some View {
        HStack {
            Text(text)
                .font(DesignTokens.mono(9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))
            Spacer()
        }
        .padding(.bottom, DesignTokens.spacing4)
    }

    /// Score normalized from prediction traits' uncertainties.
    /// We use whatever the first trait prediction carries for compat; if
    /// the caller has a HarmonicCompatibility score, they should compute
    /// BreedingPrediction with that value surfaced. For now, derive from
    /// average uncertainty (less uncertainty = higher compat displayed).
    private var compatScore: Float {
        // Use confidence as the compat proxy — it's meaningful and available.
        prediction.confidence
    }

    private var compatColor: Color {
        switch compatScore {
        case 0..<0.4: return DesignTokens.errorRed
        case 0.4..<0.6: return DesignTokens.xoGold
        default: return DesignTokens.modulatorColor
        }
    }

    private var compatLabel: String {
        switch compatScore {
        case 0..<0.4: return "High tension — dissonant pairing"
        case 0.4..<0.6: return "Mixed character — interesting contrast"
        case 0.6..<0.8: return "Harmonious pairing"
        default: return "Strong consonance — well matched"
        }
    }

    private var mutationColor: Color {
        prediction.mutationProbability > 0.15 ? DesignTokens.xoGold : .white.opacity(0.55)
    }
}

// MARK: - Trait Prediction Row

private struct TraitPredictionRow: View {
    let prediction: TraitPrediction

    var body: some View {
        HStack(spacing: DesignTokens.spacing8) {
            // Trait name
            Text(prediction.trait.displayName)
                .font(DesignTokens.bodyMedium(11))
                .foregroundColor(.white.opacity(0.55))
                .frame(width: 72, alignment: .trailing)

            // Min-max range bar
            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 2)
                        .fill(Color.white.opacity(0.07))
                    RoundedRectangle(cornerRadius: 2)
                        .fill(expressionColor.opacity(0.5))
                        .offset(x: geo.size.width * CGFloat(prediction.minValue))
                        .frame(width: geo.size.width * CGFloat(prediction.maxValue - prediction.minValue))
                }
            }
            .frame(height: 6)

            // Expression badge
            Text(expressionLabel)
                .font(DesignTokens.mono(9))
                .foregroundColor(expressionColor)
                .frame(width: 60)

            // Parent icon (A vs B)
            Text(prediction.fromParent.prefix(1).uppercased())
                .font(DesignTokens.monoBold(9))
                .foregroundColor(.white.opacity(0.35))
                .frame(width: 14)
        }
        .padding(.horizontal, DesignTokens.spacing12)
        .padding(.vertical, DesignTokens.spacing8)
    }

    private var expressionLabel: String {
        switch prediction.likelyExpression {
        case .dominant:   return "Dominant"
        case .recessive:  return "Recessive"
        case .codominant: return "Blend"
        }
    }

    private var expressionColor: Color {
        switch prediction.likelyExpression {
        case .dominant:   return DesignTokens.sourceColor
        case .recessive:  return DesignTokens.mutedText
        case .codominant: return DesignTokens.modulatorColor
        }
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    BreedingPredictionView(
        prediction: BreedingPrediction(
            parentNameA: "Sawfin",
            parentNameB: "Curtain",
            traitPredictions: SonicTrait.allCases.map { trait in
                TraitPrediction(
                    trait: trait,
                    minValue: Float.random(in: 0.1...0.4),
                    maxValue: Float.random(in: 0.5...0.9),
                    likelyExpression: [.dominant, .recessive, .codominant].randomElement()!,
                    fromParent: Bool.random() ? "Sawfin" : "Curtain"
                )
            },
            mutationProbability: 0.07,
            predictedGeneration: 2,
            confidence: 0.78
        ),
        onBreed: {}
    )
}
#endif
