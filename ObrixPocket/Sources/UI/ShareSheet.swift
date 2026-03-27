import SwiftUI
import UIKit

// MARK: - UIActivityViewController wrapper

/// SwiftUI wrapper for UIActivityViewController (native iOS share sheet).
struct ShareSheet: UIViewControllerRepresentable {
    let items: [Any]

    func makeUIViewController(context: Context) -> UIActivityViewController {
        UIActivityViewController(activityItems: items, applicationActivities: nil)
    }

    func updateUIViewController(_ uiViewController: UIActivityViewController, context: Context) {
        // No dynamic updates needed — sheet is presented modally and dismissed by the OS
    }
}

// MARK: - Audio Share Button

/// Share button for reef audio exports (M4A files from AudioExporter).
struct ShareButton: View {
    let url: URL
    @State private var showingShare = false

    var body: some View {
        Button(action: { showingShare = true }) {
            HStack(spacing: 6) {
                Image(systemName: "square.and.arrow.up")
                Text("Share")
            }
            .font(.custom("Inter-Medium", size: 13))
            .foregroundColor(Color(hex: "1E8B7E")) // Reef Jade
        }
        .sheet(isPresented: $showingShare) {
            ShareSheet(items: [url])
        }
    }
}

// MARK: - Specimen Card Share Button

/// Share button that generates a PNG specimen card and presents the share sheet.
struct SpecimenCardShareButton: View {
    let specimen: Specimen
    @State private var showingShare = false
    @State private var cardImage: UIImage?

    var body: some View {
        Button(action: {
            cardImage = generateSpecimenCard()
            showingShare = true
        }) {
            HStack(spacing: 6) {
                Image(systemName: "photo.on.rectangle.angled")
                Text("Share Card")
            }
            .font(.custom("Inter-Medium", size: 13))
            .foregroundColor(Color(hex: "E9C46A")) // XO Gold
        }
        .sheet(isPresented: $showingShare) {
            if let image = cardImage {
                ShareSheet(items: [image])
            }
        }
    }

    // MARK: - Card Rendering

    /// Generate a 400x500 PNG specimen card using UIGraphicsImageRenderer.
    private func generateSpecimenCard() -> UIImage? {
        let cardSize = CGSize(width: 400, height: 500)
        let renderer = UIGraphicsImageRenderer(size: cardSize)

        return renderer.image { _ in
            let rect = CGRect(origin: .zero, size: cardSize)

            // Background — dark slate matching OBRIX Pocket dark UI
            UIColor(red: 0.055, green: 0.055, blue: 0.063, alpha: 1).setFill()
            UIRectFill(rect)

            let catColor = categoryColor(for: specimen.category)

            // Rarity ring (oval behind creature icon area)
            let ringRect = CGRect(x: 125, y: 40, width: 150, height: 150)
            let ring = UIBezierPath(ovalIn: ringRect)
            catColor.withAlphaComponent(0.3).setStroke()
            ring.lineWidth = specimen.rarity == .legendary ? 4 : 2
            ring.stroke()

            // Specimen name
            let nameAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 28, weight: .bold),
                .foregroundColor: UIColor.white
            ]
            let nameStr = specimen.name as NSString
            let nameSize = nameStr.size(withAttributes: nameAttrs)
            nameStr.draw(
                at: CGPoint(x: (cardSize.width - nameSize.width) / 2, y: 220),
                withAttributes: nameAttrs
            )

            // Subtype label
            let subtypeAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.monospacedSystemFont(ofSize: 12, weight: .regular),
                .foregroundColor: catColor
            ]
            let subtypeStr = specimen.subtype as NSString
            let subtypeSize = subtypeStr.size(withAttributes: subtypeAttrs)
            subtypeStr.draw(
                at: CGPoint(x: (cardSize.width - subtypeSize.width) / 2, y: 258),
                withAttributes: subtypeAttrs
            )

            // Rarity badge — XO Gold
            let rarityAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 11, weight: .medium),
                .foregroundColor: UIColor(red: 0.913, green: 0.769, blue: 0.416, alpha: 1)
            ]
            let rarityStr = specimen.rarity.rawValue.uppercased() as NSString
            let raritySize = rarityStr.size(withAttributes: rarityAttrs)
            rarityStr.draw(
                at: CGPoint(x: (cardSize.width - raritySize.width) / 2, y: 285),
                withAttributes: rarityAttrs
            )

            // Provenance line (weather + catch date)
            let provenanceText = buildProvenanceText()
            let provAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 10, weight: .regular),
                .foregroundColor: UIColor(white: 1, alpha: 0.4)
            ]
            let provStr = provenanceText as NSString
            let provSize = provStr.size(withAttributes: provAttrs)
            provStr.draw(
                at: CGPoint(x: (cardSize.width - provSize.width) / 2, y: 320),
                withAttributes: provAttrs
            )

            // Spectral fingerprint — simplified radial waveform from spectralDNA
            drawSpectralFingerprint(center: CGPoint(x: 200, y: 420), color: catColor)

            // OBRIX Pocket watermark
            let watermarkAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 9, weight: .medium),
                .foregroundColor: UIColor(white: 1, alpha: 0.2)
            ]
            ("OBRIX Pocket" as NSString).draw(
                at: CGPoint(x: 155, y: 478),
                withAttributes: watermarkAttrs
            )
        }
    }

    // MARK: - Drawing Helpers

    private func categoryColor(for category: SpecimenCategory) -> UIColor {
        switch category {
        case .source:    return UIColor(red: 0.2,  green: 0.5, blue: 1.0, alpha: 1) // Blue — Shells
        case .processor: return UIColor(red: 1.0,  green: 0.3, blue: 0.3, alpha: 1) // Red — Coral
        case .modulator: return UIColor(red: 0.3,  green: 0.8, blue: 0.3, alpha: 1) // Green — Currents
        case .effect:    return UIColor(red: 0.7,  green: 0.3, blue: 1.0, alpha: 1) // Purple — Tide Pools
        }
    }

    private func buildProvenanceText() -> String {
        var parts: [String] = []
        if let weather = specimen.catchWeatherDescription, !weather.isEmpty {
            parts.append(weather)
        }
        let fmt = DateFormatter()
        fmt.dateFormat = "MMM d, yyyy 'at' h:mm a"
        parts.append(fmt.string(from: specimen.catchTimestamp))
        return parts.joined(separator: " · ")
    }

    /// Draw a radial waveform using the specimen's 64-float spectralDNA.
    private func drawSpectralFingerprint(center: CGPoint, color: UIColor) {
        let dna = specimen.spectralDNA
        guard !dna.isEmpty else { return }

        color.withAlphaComponent(0.5).setStroke()

        let sampleCount = min(dna.count, 64)
        let path = UIBezierPath()

        for i in 0..<sampleCount {
            let angle = CGFloat(i) / CGFloat(sampleCount) * 2.0 * .pi - .pi / 2
            // Use `?? 0` not `|| 0` — 0.0 is a valid DNA value
            let magnitude = CGFloat(dna.indices.contains(i) ? dna[i] : 0)
            let radius: CGFloat = 30 + magnitude * 35
            let point = CGPoint(
                x: center.x + cos(angle) * radius,
                y: center.y + sin(angle) * radius
            )
            if i == 0 {
                path.move(to: point)
            } else {
                path.addLine(to: point)
            }
        }
        path.close()
        path.lineWidth = 1.5
        path.stroke()
    }
}
