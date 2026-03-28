import UIKit

/// Generates a 1080x1080 reef gallery image showing all specimens and wiring
/// suitable for sharing as a "reef portrait."
enum ReefGalleryGenerator {

    static func generate(reefStore: ReefStore) -> UIImage {
        let size = CGSize(width: 1080, height: 1080)
        let renderer = UIGraphicsImageRenderer(size: size)

        return renderer.image { ctx in
            // Background gradient — deep ocean dark
            let colors = [
                UIColor(red: 0.04, green: 0.06, blue: 0.12, alpha: 1).cgColor,
                UIColor(red: 0.06, green: 0.10, blue: 0.14, alpha: 1).cgColor,
            ]
            let gradient = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(),
                                       colors: colors as CFArray, locations: [0, 1])!
            ctx.cgContext.drawLinearGradient(gradient,
                                             start: .zero,
                                             end: CGPoint(x: 0, y: size.height),
                                             options: [])

            // Grid layout (4x4)
            let gridSize = 4
            let cellSize = size.width * 0.8 / CGFloat(gridSize)
            let offsetX = size.width * 0.1
            let offsetY = size.height * 0.15

            // Title
            let titleAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 42, weight: .bold),
                .foregroundColor: UIColor.white
            ]
            let title = reefStore.reefName as NSString
            let titleSize = title.size(withAttributes: titleAttrs)
            title.draw(at: CGPoint(x: (size.width - titleSize.width) / 2, y: 40),
                       withAttributes: titleAttrs)

            // Depth subtitle
            let depthAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.monospacedSystemFont(ofSize: 20, weight: .regular),
                .foregroundColor: UIColor(red: 0.118, green: 0.545, blue: 0.494, alpha: 0.7)
            ]
            let depthStr = "\(reefStore.totalDiveDepth)m depth" as NSString
            let depthSize = depthStr.size(withAttributes: depthAttrs)
            depthStr.draw(at: CGPoint(x: (size.width - depthSize.width) / 2, y: 90),
                          withAttributes: depthAttrs)

            // Draw wires first (behind specimens)
            for route in reefStore.couplingRoutes {
                guard let srcIdx = reefStore.specimens.firstIndex(where: { $0?.id == route.sourceId }),
                      let dstIdx = reefStore.specimens.firstIndex(where: { $0?.id == route.destId }) else { continue }

                let srcPos = slotCenter(index: srcIdx, cellSize: cellSize, offsetX: offsetX, offsetY: offsetY, gridSize: gridSize)
                let dstPos = slotCenter(index: dstIdx, cellSize: cellSize, offsetX: offsetX, offsetY: offsetY, gridSize: gridSize)

                let wirePath = UIBezierPath()
                wirePath.move(to: srcPos)
                let midX = (srcPos.x + dstPos.x) / 2
                let midY = (srcPos.y + dstPos.y) / 2
                let dx = dstPos.x - srcPos.x
                let dy = dstPos.y - srcPos.y
                let control = CGPoint(x: midX - dy * 0.15, y: midY + dx * 0.15)
                wirePath.addQuadCurve(to: dstPos, controlPoint: control)

                UIColor(red: 0.3, green: 0.8, blue: 0.3, alpha: 0.3).setStroke()
                wirePath.lineWidth = 3
                wirePath.stroke()
            }

            // Draw specimens
            for row in 0..<gridSize {
                for col in 0..<gridSize {
                    let index = row * gridSize + col
                    let center = slotCenter(index: index, cellSize: cellSize, offsetX: offsetX, offsetY: offsetY, gridSize: gridSize)
                    let radius = cellSize * 0.35

                    if let specimen = reefStore.specimens[index] {
                        let catColor = uiCategoryColor(specimen.category)

                        // Outer glow
                        catColor.withAlphaComponent(0.1).setFill()
                        UIBezierPath(ovalIn: CGRect(x: center.x - radius * 1.2,
                                                    y: center.y - radius * 1.2,
                                                    width: radius * 2.4,
                                                    height: radius * 2.4)).fill()

                        // Background circle
                        catColor.withAlphaComponent(0.25).setFill()
                        catColor.withAlphaComponent(0.5).setStroke()
                        let circle = UIBezierPath(ovalIn: CGRect(x: center.x - radius,
                                                                  y: center.y - radius,
                                                                  width: radius * 2,
                                                                  height: radius * 2))
                        circle.fill()
                        circle.lineWidth = 2
                        circle.stroke()

                        // Creature sprite
                        if let sprite = UIImage(named: specimen.subtype) {
                            let spriteSize = radius * 1.0
                            sprite.draw(in: CGRect(x: center.x - spriteSize / 2,
                                                   y: center.y - spriteSize / 2 - 8,
                                                   width: spriteSize,
                                                   height: spriteSize))
                        }

                        // Creature name
                        let nameAttrs: [NSAttributedString.Key: Any] = [
                            .font: UIFont.systemFont(ofSize: 14, weight: .medium),
                            .foregroundColor: UIColor.white.withAlphaComponent(0.7)
                        ]
                        let name = specimen.creatureName as NSString
                        let nameSize = name.size(withAttributes: nameAttrs)
                        name.draw(at: CGPoint(x: center.x - nameSize.width / 2,
                                              y: center.y + radius * 0.5),
                                  withAttributes: nameAttrs)

                        // Level badge — gold for Lv.5+
                        let lvAttrs: [NSAttributedString.Key: Any] = [
                            .font: UIFont.monospacedSystemFont(ofSize: 11, weight: .regular),
                            .foregroundColor: specimen.level >= 5
                                ? UIColor(red: 0.914, green: 0.769, blue: 0.416, alpha: 0.6)
                                : UIColor.white.withAlphaComponent(0.3)
                        ]
                        ("Lv.\(specimen.level)" as NSString).draw(
                            at: CGPoint(x: center.x - 12, y: center.y + radius * 0.5 + 18),
                            withAttributes: lvAttrs
                        )
                    } else {
                        // Empty slot — faint circle placeholder
                        UIColor.white.withAlphaComponent(0.03).setFill()
                        UIBezierPath(ovalIn: CGRect(x: center.x - radius,
                                                    y: center.y - radius,
                                                    width: radius * 2,
                                                    height: radius * 2)).fill()
                    }
                }
            }

            // Watermark
            let wmAttrs: [NSAttributedString.Key: Any] = [
                .font: UIFont.systemFont(ofSize: 18, weight: .medium),
                .foregroundColor: UIColor.white.withAlphaComponent(0.12)
            ]
            ("OBRIX Pocket" as NSString).draw(
                at: CGPoint(x: (size.width - 140) / 2, y: size.height - 50),
                withAttributes: wmAttrs
            )
        }
    }

    // MARK: - Private Helpers

    private static func slotCenter(index: Int, cellSize: CGFloat, offsetX: CGFloat, offsetY: CGFloat, gridSize: Int) -> CGPoint {
        let row = index / gridSize
        let col = index % gridSize
        return CGPoint(
            x: offsetX + CGFloat(col) * cellSize + cellSize / 2,
            y: offsetY + CGFloat(row) * cellSize + cellSize / 2
        )
    }

    private static func uiCategoryColor(_ cat: SpecimenCategory) -> UIColor {
        switch cat {
        case .source:    return UIColor(red: 0.2, green: 0.5, blue: 1.0, alpha: 1)
        case .processor: return UIColor(red: 1.0, green: 0.3, blue: 0.3, alpha: 1)
        case .modulator: return UIColor(red: 0.3, green: 0.8, blue: 0.3, alpha: 1)
        case .effect:    return UIColor(red: 0.7, green: 0.3, blue: 1.0, alpha: 1)
        }
    }
}
