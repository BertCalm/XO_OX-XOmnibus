import SwiftUI

struct OnboardingView: View {
    let onComplete: () -> Void

    @State private var showTitle = false
    @State private var showSubtitle = false
    @State private var showButton = false

    var body: some View {
        ZStack {
            DesignTokens.darkBackground.ignoresSafeArea()

            VStack(spacing: DesignTokens.spacing24) {
                Spacer()

                // App title
                if showTitle {
                    VStack(spacing: 8) {
                        Text("OBRIX")
                            .font(DesignTokens.heading(40))
                            .foregroundColor(DesignTokens.reefJade)

                        Text("POCKET")
                            .font(DesignTokens.heading(18))
                            .foregroundColor(.white.opacity(0.6))
                            .tracking(8)
                    }
                    .transition(.opacity)
                }

                Spacer()

                // Subtitle
                if showSubtitle {
                    Text("Your reef. Your instrument.")
                        .font(DesignTokens.bodyMedium(16))
                        .foregroundColor(.white.opacity(0.75))
                        .transition(.opacity)
                }

                // Begin button
                if showButton {
                    Button(action: {
                        onComplete()
                    }) {
                        Text("BEGIN")
                            .font(DesignTokens.heading(16))
                            .tracking(2)
                            .foregroundColor(.white)
                            .frame(width: 200, height: 50)
                            .background(DesignTokens.reefJade)
                            .cornerRadius(25)
                    }
                    .padding(.bottom, DesignTokens.spacing40)
                    .transition(.opacity)
                }
            }
        }
        .onAppear {
            // Play a preview note immediately — this is the first sound the user ever hears
            playIntroNote()

            // Stagger title / subtitle / button appearance
            let reducedMotion = UIAccessibility.isReduceMotionEnabled
            if reducedMotion {
                showTitle = true
                showSubtitle = true
                showButton = true
            } else {
                withAnimation(.easeIn(duration: 0.8)) {
                    showTitle = true
                }
                DispatchQueue.main.asyncAfter(deadline: .now() + 1.2) {
                    withAnimation(.easeIn(duration: 0.6)) {
                        showSubtitle = true
                    }
                }
                DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
                    withAnimation(.easeIn(duration: 0.4)) {
                        showButton = true
                    }
                }
            }
        }
    }

    private func playIntroNote() {
        // C3 (MIDI 48) at low velocity — warm, welcoming, unhurried
        ObrixBridge.shared()?.note(on: 48, velocity: 0.4)
        DispatchQueue.main.asyncAfter(deadline: .now() + 3.0) {
            ObrixBridge.shared()?.noteOff(48)
        }
    }
}
