import SwiftUI

struct OnboardingView: View {
    let onComplete: () -> Void
    @State private var currentPage = 0

    var body: some View {
        ZStack {
            DesignTokens.darkBackground.ignoresSafeArea()

            VStack(spacing: 0) {
                TabView(selection: $currentPage) {
                    // Page 1: Catch — show actual specimen sprites
                    onboardingPage(
                        title: "Catch",
                        subtitle: "Find aquatic specimens in the wild.\nEach one is a unique synth module.",
                        color: DesignTokens.reefJade,
                        content: {
                            HStack(spacing: 16) {
                                VStack(spacing: 4) {
                                    SpecimenSprite(subtype: "polyblep-saw", category: .source, size: 48)
                                    Text("Sawfin")
                                        .font(DesignTokens.mono(9))
                                        .foregroundColor(.white.opacity(0.4))
                                }
                                VStack(spacing: 4) {
                                    SpecimenSprite(subtype: "svf-lp", category: .processor, size: 48)
                                    Text("Curtain")
                                        .font(DesignTokens.mono(9))
                                        .foregroundColor(.white.opacity(0.4))
                                }
                                VStack(spacing: 4) {
                                    SpecimenSprite(subtype: "lfo-sine", category: .modulator, size: 48)
                                    Text("Tidepulse")
                                        .font(DesignTokens.mono(9))
                                        .foregroundColor(.white.opacity(0.4))
                                }
                                VStack(spacing: 4) {
                                    SpecimenSprite(subtype: "delay-stereo", category: .effect, size: 48)
                                    Text("Echocave")
                                        .font(DesignTokens.mono(9))
                                        .foregroundColor(.white.opacity(0.4))
                                }
                            }
                        }
                    ).tag(0)

                    // Page 2: Wire — show category shapes with signal chain
                    onboardingPage(
                        title: "Wire",
                        subtitle: "Connect specimens on your reef.\nSources → Processors → Effects.\nBuild your signal chain.",
                        color: DesignTokens.sourceColor,
                        content: {
                            VStack(spacing: 8) {
                                HStack(spacing: 24) {
                                    ZStack {
                                        Circle()
                                            .stroke(DesignTokens.sourceColor.opacity(0.5), lineWidth: 2)
                                            .frame(width: 40, height: 40)
                                        Text("SRC")
                                            .font(DesignTokens.mono(7))
                                            .foregroundColor(DesignTokens.sourceColor)
                                    }

                                    Image(systemName: "arrow.right")
                                        .foregroundColor(.white.opacity(0.2))

                                    ZStack {
                                        RoundedRectangle(cornerRadius: 6)
                                            .stroke(DesignTokens.processorColor.opacity(0.5), lineWidth: 2)
                                            .frame(width: 40, height: 40)
                                        Text("FLT")
                                            .font(DesignTokens.mono(7))
                                            .foregroundColor(DesignTokens.processorColor)
                                    }

                                    Image(systemName: "arrow.right")
                                        .foregroundColor(.white.opacity(0.2))

                                    ZStack {
                                        Circle()
                                            .stroke(DesignTokens.effectColor.opacity(0.5), lineWidth: 2)
                                            .frame(width: 40, height: 40)
                                        Text("FX")
                                            .font(DesignTokens.mono(7))
                                            .foregroundColor(DesignTokens.effectColor)
                                    }
                                }

                                Text("Source → Processor → Effect")
                                    .font(DesignTokens.mono(10))
                                    .foregroundColor(.white.opacity(0.3))
                            }
                        }
                    ).tag(1)

                    // Page 3: Play — piano icon + play mode hints
                    onboardingPage(
                        title: "Play",
                        subtitle: "Your reef IS your instrument.\nEvery wiring creates a new sound.",
                        color: DesignTokens.xoGold,
                        content: {
                            VStack(spacing: 8) {
                                Image(systemName: "pianokeys")
                                    .font(.system(size: 64, weight: .light))
                                    .foregroundColor(DesignTokens.xoGold.opacity(0.6))
                                Text("Scale modes · Chord mode · Arpeggiator")
                                    .font(DesignTokens.mono(9))
                                    .foregroundColor(DesignTokens.xoGold.opacity(0.4))
                            }
                        }
                    ).tag(2)

                    // Page 4: Grow — level progression visual
                    onboardingPage(
                        title: "Grow",
                        subtitle: "Specimens level up through play.\nYour instrument evolves with you.",
                        color: DesignTokens.modulatorColor,
                        content: {
                            VStack(spacing: 8) {
                                HStack(spacing: 4) {
                                    ForEach(1...10, id: \.self) { level in
                                        Circle()
                                            .fill(level <= 5 ? DesignTokens.reefJade : DesignTokens.xoGold.opacity(0.3))
                                            .frame(width: 8, height: 8)
                                    }
                                }
                                Text("Lv.1 → Lv.10 → Evolution")
                                    .font(DesignTokens.mono(10))
                                    .foregroundColor(.white.opacity(0.3))
                            }
                        }
                    ).tag(3)
                }
                .tabViewStyle(.page(indexDisplayMode: .never))

                // Page dots
                HStack(spacing: 8) {
                    ForEach(0..<4, id: \.self) { i in
                        Circle()
                            .fill(i == currentPage ? DesignTokens.reefJade : Color.white.opacity(0.2))
                            .frame(width: 8, height: 8)
                    }
                }
                .padding(.bottom, 20)

                // Button
                Button(action: {
                    if currentPage < 3 {
                        withAnimation { currentPage += 1 }
                    } else {
                        onComplete()
                    }
                }) {
                    Text(currentPage < 3 ? "NEXT" : "START")
                        .font(DesignTokens.heading(16))
                        .tracking(2)
                        .foregroundColor(.white)
                        .frame(maxWidth: .infinity)
                        .frame(height: 56)
                        .background(
                            RoundedRectangle(cornerRadius: 28)
                                .fill(DesignTokens.reefJade)
                        )
                }
                .padding(.horizontal, 40)
                .padding(.bottom, 20)

                // Skip
                if currentPage < 3 {
                    Button("Skip") {
                        onComplete()
                    }
                    .font(DesignTokens.body(13))
                    .foregroundColor(.white.opacity(0.3))
                    .padding(.bottom, 16)
                }
            }
        }
    }

    private func onboardingPage<Content: View>(
        title: String,
        subtitle: String,
        color: Color,
        @ViewBuilder content: () -> Content
    ) -> some View {
        VStack(spacing: 24) {
            Spacer()

            content()

            // Title
            Text(title)
                .font(DesignTokens.heading(36))
                .foregroundColor(.white)

            // Subtitle
            Text(subtitle)
                .font(DesignTokens.body(16))
                .foregroundColor(.white.opacity(0.5))
                .multilineTextAlignment(.center)
                .padding(.horizontal, 40)

            Spacer()
            Spacer()
        }
    }
}
