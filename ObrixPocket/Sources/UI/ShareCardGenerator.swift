import SwiftUI
import UIKit

/// Generates a shareable 1080x1350 card image for a specimen.
/// Used by the #WhatsYourSong viral content format.
enum ShareCardGenerator {

    static func generateCard(for specimen: Specimen) -> UIImage {
        let cardSize = CGSize(width: 1080, height: 1350)
        let renderer = UIGraphicsImageRenderer(size: cardSize)

        return renderer.image { ctx in
            let cgCtx = ctx.cgContext
            let rect = CGRect(origin: .zero, size: cardSize)

            // Background — near-black matching OBRIX Pocket dark UI
            UIColor(red: 0.04, green: 0.04, blue: 0.05, alpha: 1).setFill()
            ctx.fill(rect)

            let catColor = uiCategoryColor(specimen.category)

            // Category color accent bar at top (6px)
            catColor.withAlphaComponent(0.3).setFill()
            ctx.fill(CGRect(x: 0, y: 0, width: cardSize.width, height: 6))

            // MARK: Creature name (large, centered)
            let nameAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 72, weight: .bold),
                .foregroundColor: UIColor.white
            ]
            let nameStr = specimen.name as NSString
            let nameSize = nameStr.size(withAttributes: nameAttrs)
            nameStr.draw(
                at: CGPoint(x: (cardSize.width - nameSize.width) / 2, y: 200),
                withAttributes: nameAttrs
            )

            // MARK: Rarity — XO Gold
            let rarityAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.monospacedSystemFont(ofSize: 28, weight: .medium),
                .foregroundColor: UIColor(red: 0.914, green: 0.769, blue: 0.416, alpha: 1)
            ]
            let rarityStr = specimen.rarity.rawValue.uppercased() as NSString
            let raritySize = rarityStr.size(withAttributes: rarityAttrs)
            rarityStr.draw(
                at: CGPoint(x: (cardSize.width - raritySize.width) / 2, y: 290),
                withAttributes: rarityAttrs
            )

            // MARK: Creature sprite (centered, 300pt square)
            if let spriteImage = UIImage(named: specimen.subtype) {
                let spriteSize: CGFloat = 300
                let spriteRect = CGRect(
                    x: (cardSize.width - spriteSize) / 2,
                    y: 380,
                    width: spriteSize,
                    height: spriteSize
                )
                spriteImage.draw(in: spriteRect)
            }

            // MARK: Category + level label
            let roleText = "\(categoryLabel(specimen.category)) · Level \(specimen.level)"
            let roleAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.monospacedSystemFont(ofSize: 24, weight: .regular),
                .foregroundColor: catColor.withAlphaComponent(0.7)
            ]
            let roleStr = roleText as NSString
            let roleSize = roleStr.size(withAttributes: roleAttrs)
            roleStr.draw(
                at: CGPoint(x: (cardSize.width - roleSize.width) / 2, y: 720),
                withAttributes: roleAttrs
            )

            // MARK: Personality line (from catalog entry)
            if let entry = SpecimenCatalog.entry(for: specimen.subtype) {
                let persAttrs: [NSAttributedString.Key: Any] = [
                    .font: UIFont.italicSystemFont(ofSize: 28),
                    .foregroundColor: UIColor.white.withAlphaComponent(0.4)
                ]
                let persStr = entry.personalityLine as NSString
                let persSize = persStr.size(withAttributes: persAttrs)
                persStr.draw(
                    at: CGPoint(x: (cardSize.width - persSize.width) / 2, y: 770),
                    withAttributes: persAttrs
                )
            }

            // MARK: Top 3 game stats
            let stats = GameStats.from(params: specimen.parameterState)
            let statsText = stats.topThree
                .map { "\($0.0): \($0.1)" }
                .joined(separator: " · ")
            let statsAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.monospacedSystemFont(ofSize: 22, weight: .regular),
                .foregroundColor: catColor.withAlphaComponent(0.6)
            ]
            let statsStr = statsText as NSString
            let statsSize = statsStr.size(withAttributes: statsAttrs)
            statsStr.draw(
                at: CGPoint(x: (cardSize.width - statsSize.width) / 2, y: 850),
                withAttributes: statsAttrs
            )

            // MARK: Source track (if music catch)
            if let track = specimen.sourceTrackTitle {
                let trackAttrs: [NSAttributedString.Key: Any] = [
                    .font: UIFont.systemFont(ofSize: 22, weight: .regular),
                    .foregroundColor: UIColor.white.withAlphaComponent(0.3)
                ]
                let trackStr = "Born from: \(track)" as NSString
                let trackSize = trackStr.size(withAttributes: trackAttrs)
                trackStr.draw(
                    at: CGPoint(x: (cardSize.width - trackSize.width) / 2, y: 920),
                    withAttributes: trackAttrs
                )
            }

            // MARK: Spectral fingerprint ring
            drawSpectralRing(
                ctx: cgCtx,
                center: CGPoint(x: cardSize.width / 2, y: 1080),
                dna: specimen.spectralDNA,
                color: catColor,
                radius: 80
            )

            // MARK: OBRIX Pocket branding
            let brandAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 20, weight: .medium),
                .foregroundColor: UIColor.white.withAlphaComponent(0.15)
            ]
            let brandStr = "OBRIX Pocket · #WhatsYourSong" as NSString
            let brandSize = brandStr.size(withAttributes: brandAttrs)
            brandStr.draw(
                at: CGPoint(x: (cardSize.width - brandSize.width) / 2, y: 1290),
                withAttributes: brandAttrs
            )
        }
    }

    // MARK: - Private Helpers

    private static func drawSpectralRing(
        ctx: CGContext,
        center: CGPoint,
        dna: [Float],
        color: UIColor,
        radius: CGFloat
    ) {
        guard !dna.isEmpty else { return }

        color.withAlphaComponent(0.4).setStroke()

        let count = min(dna.count, 64)
        let path = UIBezierPath()

        for i in 0..<count {
            let angle = CGFloat(i) / CGFloat(count) * 2 * .pi - .pi / 2
            // Use `dna.indices.contains` + `?? 0` — 0.0 is a valid DNA value
            let mag = CGFloat(dna.indices.contains(i) ? dna[i] : 0)
            let r = radius * 0.5 + mag * radius * 0.5
            let pt = CGPoint(x: center.x + cos(angle) * r, y: center.y + sin(angle) * r)
            if i == 0 { path.move(to: pt) } else { path.addLine(to: pt) }
        }
        path.close()
        path.lineWidth = 2
        path.stroke()
    }

    private static func uiCategoryColor(_ cat: SpecimenCategory) -> UIColor {
        switch cat {
        case .source:    return UIColor(red: 0.2,  green: 0.5, blue: 1.0, alpha: 1)
        case .processor: return UIColor(red: 1.0,  green: 0.3, blue: 0.3, alpha: 1)
        case .modulator: return UIColor(red: 0.3,  green: 0.8, blue: 0.3, alpha: 1)
        case .effect:    return UIColor(red: 0.7,  green: 0.3, blue: 1.0, alpha: 1)
        }
    }

    private static func categoryLabel(_ cat: SpecimenCategory) -> String {
        switch cat {
        case .source:    return "Source"
        case .processor: return "Processor"
        case .modulator: return "Modulator"
        case .effect:    return "Effect"
        }
    }
}
