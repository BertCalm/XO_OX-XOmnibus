import SwiftUI

/// Full-screen legendary encounter — once-in-a-lifetime moment.
///
/// Present as a full-screen cover:
///   .fullScreenCover(item: $legendaryManager.pendingEncounter) { legend in
///     LegendaryEncounterView(
///       template: legend,
///       onEncounter: { legendaryManager.pendingEncounter = nil; startCatchFlow(legend) },
///       onDismiss: { legendaryManager.dismissEncounter() }
///     )
///   }
struct LegendaryEncounterView: View {

    let template: LegendarySpecimenManager.LegendaryTemplate
    /// Called when the player taps "Encounter" — transitions to the catch flow.
    let onEncounter: () -> Void
    /// Called if the player somehow exits without catching (back gesture, dismiss).
    let onDismiss: () -> Void

    // Staged reveal timings
    @State private var backgroundVisible = false
    @State private var particlesVisible = false
    @State private var nameVisible = false
    @State private var descriptionVisible = false
    @State private var buttonVisible = false

    var body: some View {
        ZStack {
            // Full black canvas — no background color bleed
            Color.black.ignoresSafeArea()

            // Bioluminescent particle field
            if particlesVisible {
                BioluminescentParticleField()
                    .transition(.opacity)
            }

            // Central content
            VStack(spacing: DesignTokens.spacing24) {
                Spacer()

                if nameVisible {
                    legendaryNameBlock
                        .transition(.opacity.combined(with: .move(edge: .bottom)))
                }

                if descriptionVisible {
                    descriptionBlock
                        .transition(.opacity)
                }

                Spacer()

                if buttonVisible {
                    encounterButton
                        .transition(.opacity.combined(with: .scale(scale: 0.94)))
                }
            }
            .padding(.horizontal, DesignTokens.spacing24)
            .padding(.bottom, DesignTokens.spacing40)
        }
        .opacity(backgroundVisible ? 1 : 0)
        .onAppear { runRevealSequence() }
        .onDisappear { onDismiss() }
    }

    // MARK: - Name Block

    private var legendaryNameBlock: some View {
        VStack(spacing: DesignTokens.spacing8) {
            Text(template.name)
                .font(DesignTokens.heading(40, relativeTo: .largeTitle))
                .foregroundColor(DesignTokens.legendaryGold)
                .multilineTextAlignment(.center)
                .shadow(color: DesignTokens.legendaryGold.opacity(0.4), radius: 20, x: 0, y: 0)

            Text(template.title)
                .font(DesignTokens.body(16, relativeTo: .body))
                .foregroundColor(DesignTokens.xoGold.opacity(0.7))
                .italic()
                .multilineTextAlignment(.center)
        }
    }

    // MARK: - Description

    private var descriptionBlock: some View {
        Text(template.description)
            .font(DesignTokens.body(14))
            .foregroundColor(.white.opacity(0.65))
            .multilineTextAlignment(.center)
            .lineSpacing(4)
            .padding(.horizontal, DesignTokens.spacing8)
    }

    // MARK: - Encounter Button

    private var encounterButton: some View {
        Button(action: onEncounter) {
            Text("Encounter")
                .font(DesignTokens.bodyMedium(18))
                .foregroundColor(.black)
                .frame(maxWidth: .infinity)
                .padding(.vertical, DesignTokens.spacing20)
                .background(
                    RoundedRectangle(cornerRadius: 14)
                        .fill(DesignTokens.legendaryGold)
                        .shadow(
                            color: DesignTokens.legendaryGold.opacity(0.35),
                            radius: 24, x: 0, y: 0
                        )
                )
        }
    }

    // MARK: - Reveal Sequence

    private func runRevealSequence() {
        // Step 1: fade in the black canvas
        withAnimation(.easeIn(duration: 2.0)) {
            backgroundVisible = true
        }

        // Step 2: particles emerge at 0.6s
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.6) {
            withAnimation(.easeIn(duration: 1.2)) {
                particlesVisible = true
            }
        }

        // Step 3: name appears at 1.0s
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) {
            withAnimation(.easeOut(duration: 0.8)) {
                nameVisible = true
            }
        }

        // Step 4: description fades in at 2.0s
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
            withAnimation(.easeIn(duration: 0.7)) {
                descriptionVisible = true
            }
        }

        // Step 5: button appears after 3.0s — don't rush the moment
        DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) {
            withAnimation(.spring(response: 0.5, dampingFraction: 0.75)) {
                buttonVisible = true
            }
        }
    }
}

// MARK: - Bioluminescent Particle Field

/// Subtle floating dot field — bioluminescent glow effect.
/// Uses a TimelineView-driven Canvas for performance; no SpriteKit dependency.
private struct BioluminescentParticleField: View {

    private struct Particle: Identifiable {
        let id: Int
        let x: CGFloat        // 0-1 normalized
        let baseY: CGFloat    // 0-1 normalized
        let size: CGFloat
        let opacity: Double
        let speed: Double     // Drift cycles per second
        let phase: Double     // Initial phase offset
        let hue: Double       // Slight hue variation around teal
    }

    private let particles: [Particle] = (0..<38).map { i in
        Particle(
            id: i,
            x: Double.random(in: 0...1),
            baseY: Double.random(in: 0...1),
            size: CGFloat.random(in: 2...5),
            opacity: Double.random(in: 0.3...0.7),
            speed: Double.random(in: 0.04...0.12),
            phase: Double.random(in: 0...Double.pi * 2),
            hue: Double.random(in: 0.48...0.58)   // Teal-to-cyan band
        )
    }

    var body: some View {
        TimelineView(.animation) { timeline in
            Canvas { context, size in
                let t = timeline.date.timeIntervalSinceReferenceDate
                for particle in particles {
                    let currentY = particle.baseY - CGFloat(t * particle.speed).truncatingRemainder(dividingBy: 1)
                    let driftedY = currentY < 0 ? currentY + 1 : currentY
                    let x = particle.x * size.width
                    let y = driftedY * size.height

                    // Subtle horizontal drift via sine
                    let sineX = sin(t * particle.speed * 1.3 + particle.phase) * 10

                    let rect = CGRect(
                        x: x + sineX - particle.size / 2,
                        y: y - particle.size / 2,
                        width: particle.size,
                        height: particle.size
                    )

                    // Fade at top/bottom edges
                    let edgeFade = min(driftedY / 0.1, (1 - driftedY) / 0.1, 1.0)

                    context.opacity = particle.opacity * edgeFade
                    context.fill(
                        Path(ellipseIn: rect),
                        with: .color(Color(hue: particle.hue, saturation: 0.8, brightness: 1.0))
                    )
                }
            }
        }
        .allowsHitTesting(false)
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    LegendaryEncounterView(
        template: LegendarySpecimenManager.legends[0],
        onEncounter: {},
        onDismiss: {}
    )
}
#endif
