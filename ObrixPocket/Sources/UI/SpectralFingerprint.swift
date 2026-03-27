import SwiftUI

/// Renders a radial waveform glyph from a 64-float spectral profile.
/// This is the specimen's sonic identity — a visual thumbprint of where it was caught.
struct SpectralFingerprint: View {
    let spectralDNA: [Float]
    let color: Color
    let size: CGFloat
    var interactive: Bool = false
    var onTapBand: ((Int) -> Void)?

    var body: some View {
        Canvas { context, canvasSize in
            let center = CGPoint(x: canvasSize.width / 2, y: canvasSize.height / 2)
            let maxRadius = min(canvasSize.width, canvasSize.height) / 2 * 0.85
            let minRadius = maxRadius * 0.3

            // Draw the radial waveform
            var path = Path()
            let bands = min(spectralDNA.count, 64)

            for i in 0..<bands {
                let angle = CGFloat(i) / CGFloat(bands) * 2.0 * .pi - .pi / 2
                let magnitude = CGFloat(spectralDNA.indices.contains(i) ? spectralDNA[i] : 0.5)
                let radius = minRadius + magnitude * (maxRadius - minRadius)
                let point = CGPoint(
                    x: center.x + cos(angle) * radius,
                    y: center.y + sin(angle) * radius
                )

                if i == 0 {
                    path.move(to: point)
                } else {
                    // Smooth curve between points
                    let prevAngle = CGFloat(i - 1) / CGFloat(bands) * 2.0 * .pi - .pi / 2
                    let prevMag = CGFloat(spectralDNA[i - 1])
                    let prevRadius = minRadius + prevMag * (maxRadius - minRadius)
                    let midAngle = (prevAngle + angle) / 2
                    let midRadius = (prevRadius + radius) / 2
                    let control = CGPoint(
                        x: center.x + cos(midAngle) * midRadius * 1.05,
                        y: center.y + sin(midAngle) * midRadius * 1.05
                    )
                    path.addQuadCurve(to: point, control: control)
                }
            }
            path.closeSubpath()

            // Fill with low opacity
            context.fill(path, with: .color(color.opacity(0.08)))

            // Stroke
            context.stroke(path, with: .color(color.opacity(0.5)), lineWidth: 1.5)

            // Inner glow ring
            let glowPath = Circle().path(in: CGRect(
                x: center.x - minRadius,
                y: center.y - minRadius,
                width: minRadius * 2,
                height: minRadius * 2
            ))
            context.stroke(glowPath, with: .color(color.opacity(0.15)), lineWidth: 0.5)
        }
        .frame(width: size, height: size)
        .contentShape(Circle())
        .onTapGesture { location in
            guard interactive, let onTap = onTapBand else { return }
            // Calculate which band was tapped
            let center = CGPoint(x: size / 2, y: size / 2)
            let dx = location.x - center.x
            let dy = location.y - center.y
            var angle = atan2(dy, dx) + .pi / 2
            if angle < 0 { angle += 2 * .pi }
            let band = Int(angle / (2 * .pi) * 64) % 64
            onTap(band)
        }
    }
}

/// Small inline fingerprint for reef grid slots
struct MiniFingerprint: View {
    let spectralDNA: [Float]
    let color: Color

    var body: some View {
        SpectralFingerprint(
            spectralDNA: spectralDNA,
            color: color,
            size: 60
        )
        .opacity(0.3) // Subtle background texture
    }
}

/// Large interactive fingerprint for Collection detail card
struct DetailFingerprint: View {
    let specimen: Specimen
    let color: Color
    @State private var highlightedBand: Int?

    var body: some View {
        VStack(spacing: 8) {
            SpectralFingerprint(
                spectralDNA: specimen.spectralDNA,
                color: color,
                size: 200,
                interactive: true,
                onTapBand: { band in
                    highlightedBand = band
                    // Phase 2: Play isolated frequency band from the specimen
                }
            )

            if let band = highlightedBand {
                Text("Band \(band): \(String(format: "%.1f", specimen.spectralDNA.indices.contains(band) ? specimen.spectralDNA[band] * 100 : 0))%")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(color.opacity(0.7))
            }
        }
    }
}
