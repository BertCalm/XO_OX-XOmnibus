import SwiftUI

struct OnboardingView: View {
    let onComplete: () -> Void
    @State private var currentPage = 0

    var body: some View {
        ZStack {
            DesignTokens.darkBackground.ignoresSafeArea()

            VStack(spacing: 0) {
                TabView(selection: $currentPage) {
                    // Page 1: Catch
                    onboardingPage(
                        icon: "antenna.radiowaves.left.and.right",
                        title: "Catch",
                        subtitle: "Find aquatic specimens in the wild.\nEach one is a unique synth module.",
                        color: DesignTokens.reefJade
                    ).tag(0)

                    // Page 2: Wire
                    onboardingPage(
                        icon: "point.3.connected.trianglepath.dotted",
                        title: "Wire",
                        subtitle: "Connect specimens on your reef.\nSources → Processors → Effects.\nBuild your signal chain.",
                        color: DesignTokens.sourceColor
                    ).tag(1)

                    // Page 3: Play
                    onboardingPage(
                        icon: "pianokeys",
                        title: "Play",
                        subtitle: "Your reef IS your instrument.\nEvery wiring creates a new sound.",
                        color: DesignTokens.xoGold
                    ).tag(2)

                    // Page 4: Grow
                    onboardingPage(
                        icon: "leaf.fill",
                        title: "Grow",
                        subtitle: "Specimens level up through play.\nYour instrument evolves with you.",
                        color: DesignTokens.modulatorColor
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

    private func onboardingPage(icon: String, title: String, subtitle: String, color: Color) -> some View {
        VStack(spacing: 24) {
            Spacer()

            // Icon
            Image(systemName: icon)
                .font(.system(size: 64, weight: .light))
                .foregroundColor(color.opacity(0.6))

            // Title
            Text(title)
                .font(.custom("SpaceGrotesk-Bold", size: 36, relativeTo: .title))
                .foregroundColor(.white)

            // Subtitle
            Text(subtitle)
                .font(.custom("Inter-Regular", size: 16, relativeTo: .body))
                .foregroundColor(.white.opacity(0.5))
                .multilineTextAlignment(.center)
                .padding(.horizontal, 40)

            Spacer()
            Spacer()
        }
    }
}
