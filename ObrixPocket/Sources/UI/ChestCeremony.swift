// ChestCeremony.swift
// OBRIX Pocket — iOS 16+
//
// 3-second chest opening ceremony. Governed by the Suzuki ghost condition:
//   - First encounter of a specimen type: NEVER skippable for the full 3 seconds.
//   - Subsequent encounters: tappable to skip after 1.5 seconds.

import SwiftUI

// Color(hex:) extension is defined in ContentView.swift (module-scoped)

// MARK: - Animation phase

private enum CeremonyPhase: Int {
    case idle       = 0  // before start
    case orbPulse   = 1  // 0-1s  — orb scales in
    case crackOpen  = 2  // 1-2s  — light rays expand, orb bursts
    case reveal     = 3  // 2-3s  — creature name / badge / subtype fade-in
    case complete   = 4  // after 3s
}

// MARK: - ChestCeremony

struct ChestCeremony: View {

    // MARK: Inputs

    let creatureName: String
    let category: SpecimenCategory
    let rarity: SpecimenRarity
    let cosmeticTier: CosmeticTier
    let subtypeID: String
    let isFirstEncounter: Bool
    let onComplete: () -> Void

    // MARK: State

    @State private var phase: CeremonyPhase = .idle

    // Phase 1
    @State private var orbScale: CGFloat = 0.3
    @State private var orbOpacity: Double = 0.0
    @State private var backgroundOpacity: Double = 0.0

    // Phase 2
    @State private var rayScale: CGFloat = 0.0
    @State private var rayOpacity: Double = 0.0
    @State private var orbBurst: Bool = false
    @State private var orbBurstOpacity: Double = 0.0

    // Phase 3
    @State private var nameOpacity: Double = 0.0
    @State private var badgeOpacity: Double = 0.0
    @State private var subtypeOpacity: Double = 0.0
    @State private var cosmeticLabelOpacity: Double = 0.0

    // Skip eligibility (for non-first encounters)
    @State private var skipEligible: Bool = false

    // Rotating ray animation (continuous in phase 2-3)
    @State private var rayRotation: Double = 0.0

    // Timers — held so they can be invalidated on skip
    @State private var phaseTimers: [Timer] = []

    // Chest sprite pair extracted from the CraftPix spritesheet (32x32 @1x)
    // Sheet layout: 10 cols × 4 rows. Row 0, col 0 = closed; col 1 = open (brown wooden chest).
    @State private var chestPair: (closed: UIImage, open: UIImage)? = nil

    // MARK: Derived helpers

    private var categoryColor: Color {
        DesignTokens.color(for: category)
    }

    private var rayCount: Int {
        switch rarity {
        case .common:    return 4
        case .uncommon:  return 6
        case .rare:      return 8
        case .legendary: return 12
        }
    }

    private var cosmeticLabel: String? {
        switch cosmeticTier {
        case .standard:       return nil
        case .bioluminescent: return "bioluminescent"
        case .phantom:        return "phantom"
        case .fossilized:     return "fossilized"
        case .prismatic:      return "prismatic"
        }
    }

    private var rarityLabel: String {
        switch rarity {
        case .common:    return "COMMON"
        case .uncommon:  return "UNCOMMON"
        case .rare:      return "RARE"
        case .legendary: return "LEGENDARY"
        }
    }

    // MARK: Body

    var body: some View {
        ZStack {
            // Background
            DesignTokens.darkBackground
                .ignoresSafeArea()
                .opacity(backgroundOpacity)

            // Light rays (Phase 2+)
            RayBurst(count: rayCount, color: categoryColor)
                .scaleEffect(rayScale)
                .opacity(rayOpacity)
                .rotationEffect(.degrees(rayRotation))
                .frame(width: 340, height: 340)
                .animation(
                    .linear(duration: 6).repeatForever(autoreverses: false),
                    value: rayRotation
                )

            // Orb (Phase 1, burst on Phase 2 transition)
            ZStack {
                // Outer glow
                Circle()
                    .fill(
                        RadialGradient(
                            gradient: Gradient(colors: [
                                categoryColor.opacity(0.6),
                                categoryColor.opacity(0.0)
                            ]),
                            center: .center,
                            startRadius: 0,
                            endRadius: 70
                        )
                    )
                    .frame(width: 140, height: 140)
                    .scaleEffect(orbBurst ? 3.5 : 1.0)
                    .opacity(orbBurstOpacity)

                // Core orb
                Circle()
                    .fill(
                        RadialGradient(
                            gradient: Gradient(colors: [
                                Color.white.opacity(0.9),
                                categoryColor.opacity(0.85),
                                categoryColor.opacity(0.5)
                            ]),
                            center: .center,
                            startRadius: 0,
                            endRadius: 40
                        )
                    )
                    .frame(width: 80, height: 80)
                    .shadow(color: categoryColor.opacity(0.9), radius: 24, x: 0, y: 0)
                    .shadow(color: categoryColor.opacity(0.5), radius: 48, x: 0, y: 0)
                    .scaleEffect(orbBurst ? 0.0 : orbScale)
                    .opacity(orbBurst ? 0.0 : orbOpacity)

                // Chest sprite — closed during orbPulse, open from crackOpen onward
                if let pair = chestPair {
                    let isOpen = phase == .crackOpen || phase == .reveal || phase == .complete
                    Image(uiImage: isOpen ? pair.open : pair.closed)
                        .resizable()
                        .interpolation(.none)   // pixel-art — no anti-aliasing
                        .frame(width: 80, height: 80)
                        .scaleEffect(orbBurst ? 0.0 : orbScale)
                        .opacity(orbBurst ? 0.0 : orbOpacity)
                }
            }

            // Reveal layer (Phase 3)
            VStack(spacing: 12) {
                // Creature sprite — fades in with the name
                SpecimenSprite(subtype: subtypeID, category: category, size: 80)
                    .opacity(nameOpacity)

                // Creature name
                Text(creatureName)
                    .font(DesignTokens.heading(24))
                    .foregroundColor(.white)
                    .opacity(nameOpacity)

                // Rarity badge
                Text(rarityLabel)
                    .font(.system(size: 13, weight: .semibold))
                    .foregroundColor(DesignTokens.darkBackground)
                    .padding(.horizontal, 14)
                    .padding(.vertical, 5)
                    .background(
                        Capsule()
                            .fill(DesignTokens.xoGold)
                    )
                    .opacity(badgeOpacity)

                // Subtype ID
                Text(subtypeID)
                    .font(DesignTokens.mono(12))
                    .foregroundColor(categoryColor)
                    .opacity(subtypeOpacity)

                // Personality line
                if let entry = SpecimenCatalog.entry(for: subtypeID) {
                    Text(entry.personalityLine)
                        .font(DesignTokens.body(12))
                        .foregroundColor(.white.opacity(0.4))
                        .italic()
                        .multilineTextAlignment(.center)
                        .opacity(subtypeOpacity)
                }

                // Cosmetic tier (only when non-standard)
                if let label = cosmeticLabel {
                    Text("[ \(label) ]")
                        .font(DesignTokens.mono(11))
                        .foregroundColor(cosmeticTierColor)
                        .opacity(cosmeticLabelOpacity)
                }
            }
            .offset(y: 20)
        }
        .onAppear {
            loadChestSprites()
            startCeremony()
        }
        .onDisappear {
            cancelAllTimers()
        }
        .onTapGesture(perform: handleTap)
        .accessibilityAddTraits(.isButton)
        .accessibilityLabel("Skip catch ceremony")
        .accessibilityHint(skipEligible ? "Tap to skip" : "Cannot skip yet")
    }

    // MARK: - Cosmetic tier color

    private var cosmeticTierColor: Color {
        switch cosmeticTier {
        case .standard: return .white
        default:        return DesignTokens.cosmeticColor(cosmeticTier)
        }
    }

    // MARK: - Chest sprite loading

    /// Extracts the closed (col 0) and open (col 1) frames from the CraftPix chest spritesheet.
    /// Sheet: 320×128 px @1x → 10 cols × 4 rows of 32×32 tiles.
    /// Row 0 = brown wooden chest variants; col 0 = closed, col 1 = open.
    private func loadChestSprites() {
        guard let sheet = UIImage(named: "ChestSheet"),
              let cg = sheet.cgImage else { return }

        // UIImage.scale reflects @2x/@3x asset loading; the spritesheet is @1x so scale == 1.
        // Using CGImage pixel dimensions directly avoids any scale confusion.
        let tileW = cg.width / 10   // 10 columns
        let tileH = cg.height / 4   // 4 rows

        let closedRect = CGRect(x: 0,      y: 0, width: tileW, height: tileH)
        let openRect   = CGRect(x: tileW,  y: 0, width: tileW, height: tileH)

        guard let closedCG = cg.cropping(to: closedRect),
              let openCG   = cg.cropping(to: openRect) else { return }

        chestPair = (
            closed: UIImage(cgImage: closedCG),
            open:   UIImage(cgImage: openCG)
        )
    }

    // MARK: - Ceremony orchestration

    private func startCeremony() {
        phase = .orbPulse
        runPhaseOne()
    }

    // Phase 1 — 0 to 1 second
    private func runPhaseOne() {
        // Fade in background
        withAnimation(.easeIn(duration: 0.3)) {
            backgroundOpacity = 1.0
        }
        // Orb scales from 0.3 to 1.0 with a spring bounce
        withAnimation(.interpolatingSpring(stiffness: 90, damping: 14).delay(0.05)) {
            orbScale = 1.0
            orbOpacity = 1.0
        }

        // Enable skip eligibility for non-first encounters at 1.5 s
        if !isFirstEncounter {
            schedule(after: 1.5) {
                skipEligible = true
            }
        }

        // Advance to phase 2 at 1.0 s
        schedule(after: 1.0, block: runPhaseTwo)
    }

    // Phase 2 — 1 to 2 seconds
    private func runPhaseTwo() {
        phase = .crackOpen

        // Start continuous ray rotation — skipped when reduce motion is enabled
        if !UIAccessibility.isReduceMotionEnabled {
            withAnimation(.linear(duration: 6).repeatForever(autoreverses: false)) {
                rayRotation = 360
            }
        }

        // Rays expand outward
        withAnimation(.easeOut(duration: 0.55)) {
            rayScale = 1.0
            rayOpacity = 0.85
        }

        // Orb burst — scale up flash then fade
        withAnimation(.easeOut(duration: 0.3).delay(0.1)) {
            orbBurst = true
            orbBurstOpacity = 1.0
        }
        withAnimation(.easeIn(duration: 0.5).delay(0.3)) {
            orbBurstOpacity = 0.0
        }

        // Advance to phase 3 at 2.0 s
        schedule(after: 1.0, block: runPhaseThree)
    }

    // Phase 3 — 2 to 3 seconds
    private func runPhaseThree() {
        phase = .reveal

        // Staggered reveal of text elements
        withAnimation(.easeInOut(duration: 0.4)) {
            nameOpacity = 1.0
        }
        withAnimation(.easeInOut(duration: 0.35).delay(0.15)) {
            badgeOpacity = 1.0
        }
        withAnimation(.easeInOut(duration: 0.3).delay(0.28)) {
            subtypeOpacity = 1.0
        }
        withAnimation(.easeInOut(duration: 0.3).delay(0.40)) {
            cosmeticLabelOpacity = 1.0
        }

        // Rays gently fade to a lower opacity as creature is revealed
        withAnimation(.easeInOut(duration: 0.6)) {
            rayOpacity = 0.35
        }

        // Complete at 3.0 s (1 s after entering phase 3)
        schedule(after: 1.0, block: completeCeremony)
    }

    private func completeCeremony() {
        phase = .complete
        cancelAllTimers()
        onComplete()
    }

    // MARK: - Skip handling

    private func handleTap() {
        guard skipEligible, phase != .complete else { return }
        // Jump immediately to reveal then complete
        cancelAllTimers()
        skipEligible = false
        showRevealInstantly()
        schedule(after: 0.25, block: completeCeremony)
    }

    private func showRevealInstantly() {
        phase = .reveal
        withAnimation(.easeInOut(duration: 0.2)) {
            nameOpacity = 1.0
            badgeOpacity = 1.0
            subtypeOpacity = 1.0
            cosmeticLabelOpacity = 1.0
            rayOpacity = 0.35
            orbBurstOpacity = 0.0
        }
    }

    // MARK: - Timer helpers

    private func schedule(after delay: TimeInterval, block: @escaping () -> Void) {
        let t = Timer.scheduledTimer(withTimeInterval: delay, repeats: false) { _ in
            block()
        }
        phaseTimers.append(t)
    }

    private func cancelAllTimers() {
        phaseTimers.forEach { $0.invalidate() }
        phaseTimers.removeAll()
    }
}

// MARK: - RayBurst shape

/// Draws `count` thin triangular rays radiating from the center.
private struct RayBurst: View {
    let count: Int
    let color: Color

    var body: some View {
        Canvas { context, size in
            drawRays(context: context, size: size)
        }
    }

    private func drawRays(context: GraphicsContext, size: CGSize) {
        let cx: CGFloat = size.width / 2
        let cy: CGFloat = size.height / 2
        let center = CGPoint(x: cx, y: cy)
        let outerR: CGFloat = min(size.width, size.height) / 2
        let innerR: CGFloat = outerR * 0.08
        let step: Double = 2.0 * .pi / Double(max(count, 1))
        let halfAngle: Double = step * 0.22

        let gradColors: [Color] = [color.opacity(0.9), color.opacity(0.0)]
        let gradient = Gradient(colors: gradColors)

        for i in 0..<count {
            let baseAngle: Double = step * Double(i) - .pi / 2
            let leftAngle: Double = baseAngle - halfAngle
            let rightAngle: Double = baseAngle + halfAngle

            let p0 = CGPoint(x: cx + innerR * CGFloat(cos(leftAngle)), y: cy + innerR * CGFloat(sin(leftAngle)))
            let p1 = CGPoint(x: cx + outerR * CGFloat(cos(baseAngle)), y: cy + outerR * CGFloat(sin(baseAngle)))
            let p2 = CGPoint(x: cx + innerR * CGFloat(cos(rightAngle)), y: cy + innerR * CGFloat(sin(rightAngle)))

            var path = Path()
            path.move(to: p0)
            path.addLine(to: p1)
            path.addLine(to: p2)
            path.closeSubpath()

            let shading: GraphicsContext.Shading = .linearGradient(gradient, startPoint: center, endPoint: p1)
            context.fill(path, with: shading)
        }
    }
}

// MARK: - Preview

#if DEBUG
private struct ChestCeremony_PreviewWrapper: View {
    @State private var done = false

    var body: some View {
        if done {
            Text("Ceremony complete.")
                .foregroundColor(.white)
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .background(Color.black)
        } else {
            ChestCeremony(
                creatureName: "Vellichor",
                category: .source,
                rarity: .legendary,
                cosmeticTier: .bioluminescent,
                subtypeID: "SRC-042-A",
                isFirstEncounter: true,
                onComplete: { done = true }
            )
        }
    }
}

#Preview("Legendary Source — First Encounter") {
    ChestCeremony_PreviewWrapper()
}

#Preview("Rare Effect — Skip Eligible") {
    ChestCeremony(
        creatureName: "Pelagion",
        category: .effect,
        rarity: .rare,
        cosmeticTier: .phantom,
        subtypeID: "EFX-017-B",
        isFirstEncounter: false,
        onComplete: {}
    )
}
#endif
