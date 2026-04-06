import SwiftUI

/// Full-screen semi-transparent overlay displayed when a ceremony fires.
/// Poetic, minimal, cinematic — a moment of quiet, not a busy UI.
struct CeremonyOverlay: View {

    @ObservedObject var ceremonyManager: CeremonyManager

    @State private var opacity: Double = 0
    @State private var dismissTimer: Timer?

    private var ceremony: CeremonyManager.CeremonyEvent? {
        ceremonyManager.pendingCeremonies.first { !$0.hasBeenSeen }
    }

    var body: some View {
        if let event = ceremony {
            ZStack {
                // Backdrop
                Color.black.opacity(0.85)
                    .ignoresSafeArea()

                // Content — centered, compact, maximum atmosphere
                VStack(spacing: DesignTokens.spacing20) {

                    // Specimen name above the title when ceremony is specimen-specific
                    if let specimenName = event.specimenName {
                        Text(specimenName.uppercased())
                            .font(DesignTokens.mono(11))
                            .tracking(2.5)
                            .foregroundColor(DesignTokens.reefJade.opacity(0.7))
                    }

                    // Ceremony title
                    Text(ceremonyTitle(for: event.type))
                        .font(DesignTokens.heading(28))
                        .foregroundColor(.white)
                        .multilineTextAlignment(.center)

                    // Decorative hairline
                    Rectangle()
                        .fill(Color.white.opacity(0.12))
                        .frame(width: 48, height: 1)

                    // Narrative text — max 3 lines
                    Text(event.narrativeText)
                        .font(DesignTokens.body(15))
                        .foregroundColor(.white.opacity(0.55))
                        .multilineTextAlignment(.center)
                        .lineLimit(3)
                        .lineSpacing(4)
                        .padding(.horizontal, DesignTokens.spacing40)

                    // Dismiss hint
                    Text("TAP ANYWHERE")
                        .font(DesignTokens.mono(9))
                        .tracking(2)
                        .foregroundColor(.white.opacity(0.15))
                        .padding(.top, DesignTokens.spacing12)
                }
                .padding(.horizontal, DesignTokens.spacing24)
            }
            .opacity(opacity)
            .onAppear {
                // Fade in — skip animation when Reduce Motion is on
                if UIAccessibility.isReduceMotionEnabled {
                    opacity = 1
                } else {
                    withAnimation(.easeIn(duration: 0.8)) {
                        opacity = 1
                    }
                }
                // Auto-dismiss after 5 seconds
                scheduleDismiss(for: event.id)
            }
            .onChange(of: event.id) { newID in
                opacity = 0
                if UIAccessibility.isReduceMotionEnabled {
                    opacity = 1
                } else {
                    withAnimation(.easeIn(duration: 0.8)) {
                        opacity = 1
                    }
                }
                scheduleDismiss(for: newID)
            }
            .contentShape(Rectangle())
            .onTapGesture {
                dismiss(id: event.id)
            }
        }
    }

    // MARK: - Dismiss

    private func dismiss(id: UUID) {
        dismissTimer?.invalidate()
        dismissTimer = nil
        guard !UIAccessibility.isReduceMotionEnabled else {
            opacity = 0
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) {
                ceremonyManager.dismissCeremony(id)
            }
            return
        }
        withAnimation(.easeOut(duration: 0.4)) {
            opacity = 0
        }
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.4) {
            ceremonyManager.dismissCeremony(id)
        }
    }

    private func scheduleDismiss(for id: UUID) {
        dismissTimer?.invalidate()
        let t = Timer(timeInterval: 5, repeats: false) { [self] _ in
            dismiss(id: id)
        }
        RunLoop.main.add(t, forMode: .common)
        dismissTimer = t
    }

    // MARK: - Title Text

    private func ceremonyTitle(for type: CeremonyManager.CeremonyType) -> String {
        switch type {
        case .offspringFirstNote:  return "A New Voice"
        case .elderTransition:     return "Elder"
        case .ancientAwakening:    return "Ancient"
        case .seasonChange:        return "The Season Turns"
        case .firstBreeding:       return "Life Creates Life"
        case .firstDive:           return "The Surface Breaks"
        case .reefMilestone10:     return "Ten Voices"
        case .reefMilestone25:     return "Twenty-Five Voices"
        case .hundredDays:         return "One Hundred Days"
        }
    }
}

// MARK: - Preview

#if DEBUG
#Preview {
    ZStack {
        DesignTokens.background.ignoresSafeArea()
        Text("Reef")
            .foregroundColor(.white)
    }
    .overlay {
        let manager: CeremonyManager = {
            let m = CeremonyManager()
            m.onElderTransition(specimenName: "Sawfin")
            return m
        }()
        CeremonyOverlay(ceremonyManager: manager)
    }
}
#endif
