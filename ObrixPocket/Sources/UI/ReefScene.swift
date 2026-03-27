import SpriteKit

/// SpriteKit scene rendering the 4x4 reef grid.
/// Each slot is a touch target that triggers notes via the audio engine.
class ReefScene: SKScene {

    private let reefStore: ReefStore
    private let onNoteOn: (Int, Float) -> Void
    private let onNoteOff: (Int) -> Void

    // Grid geometry
    private let gridSize = 4
    private var slotNodes: [SKNode] = []
    private var couplingLines: [SKShapeNode] = []

    // Organic offsets (±8px, deterministic per slot)
    private let organicOffsets: [CGPoint] = {
        // Seeded pseudo-random offsets for visual organic feel
        let seed: [CGPoint] = [
            CGPoint(x: 3, y: -5), CGPoint(x: -6, y: 2), CGPoint(x: 4, y: 7), CGPoint(x: -3, y: -4),
            CGPoint(x: 7, y: 1), CGPoint(x: -2, y: -7), CGPoint(x: 5, y: 4), CGPoint(x: -8, y: 3),
            CGPoint(x: 1, y: -6), CGPoint(x: -4, y: 8), CGPoint(x: 6, y: -2), CGPoint(x: -5, y: 5),
            CGPoint(x: 2, y: 3), CGPoint(x: -7, y: -1), CGPoint(x: 8, y: -3), CGPoint(x: -1, y: 6)
        ]
        return seed
    }()

    // Touch tracking for velocity calculation
    private var touchDownTimes: [Int: TimeInterval] = [:]

    // Breathing animation phase
    private var breathPhase: Float = 0
    private var lastUpdateTime: TimeInterval = 0

    // Category colors
    private let categoryColors: [SpecimenCategory: SKColor] = [
        .source: SKColor(red: 0.2, green: 0.5, blue: 1.0, alpha: 1.0),     // Blue
        .processor: SKColor(red: 1.0, green: 0.3, blue: 0.3, alpha: 1.0),  // Red
        .modulator: SKColor(red: 0.3, green: 0.8, blue: 0.3, alpha: 1.0),  // Green
        .effect: SKColor(red: 0.7, green: 0.3, blue: 1.0, alpha: 1.0)      // Purple
    ]

    init(size: CGSize, reefStore: ReefStore, onNoteOn: @escaping (Int, Float) -> Void, onNoteOff: @escaping (Int) -> Void) {
        self.reefStore = reefStore
        self.onNoteOn = onNoteOn
        self.onNoteOff = onNoteOff
        super.init(size: size)

        self.backgroundColor = SKColor(red: 0.055, green: 0.055, blue: 0.063, alpha: 1.0) // #0E0E10
        self.isUserInteractionEnabled = true
    }

    required init?(coder: NSCoder) { fatalError("init(coder:) not implemented") }

    override func didMove(to view: SKView) {
        buildGrid()
    }

    // MARK: - Grid Construction

    private func buildGrid() {
        removeAllChildren()
        slotNodes.removeAll()

        let cellSize = size.width / CGFloat(gridSize)

        for row in 0..<gridSize {
            for col in 0..<gridSize {
                let index = row * gridSize + col
                let offset = organicOffsets[index]

                let x = CGFloat(col) * cellSize + cellSize / 2 + offset.x
                let y = size.height - (CGFloat(row) * cellSize + cellSize / 2) - offset.y // SpriteKit Y is flipped

                let slotNode = SKNode()
                slotNode.position = CGPoint(x: x, y: y)
                slotNode.name = "slot_\(index)"

                // Background circle (touch target)
                let bgRadius = cellSize * 0.4
                let bg = SKShapeNode(circleOfRadius: bgRadius)
                bg.name = "slotBg_\(index)"

                if let specimen = reefStore.specimens[index] {
                    // Occupied slot
                    let color = categoryColors[specimen.category] ?? .white
                    let healthAlpha = CGFloat(specimen.health) / 100.0

                    bg.fillColor = color.withAlphaComponent(specimen.isPhantom ? 0.15 : 0.25 * healthAlpha)
                    bg.strokeColor = color.withAlphaComponent(specimen.isPhantom ? 0.3 : 0.6)
                    bg.lineWidth = specimen.rarity == .legendary ? 3.0 : (specimen.rarity == .rare ? 2.0 : 1.0)

                    // Health glow (bioluminescent aura) — added first so it renders behind bg
                    if !specimen.isPhantom && specimen.health > 0 {
                        let glow = SKShapeNode(circleOfRadius: bgRadius * 1.2)
                        glow.fillColor = color.withAlphaComponent(0.05 * healthAlpha)
                        glow.strokeColor = .clear
                        glow.name = "glow_\(index)"
                        slotNode.addChild(glow)
                    }

                    // bg added after glow so it renders on top of the glow
                    slotNode.addChild(bg)

                    // Creature placeholder (Phase 1: replace with .xogenome renderer)
                    // Added last so it renders frontmost
                    let creatureLabel = SKLabelNode(text: String(specimen.name.prefix(2)).uppercased())
                    creatureLabel.fontSize = 14
                    creatureLabel.fontName = "JetBrainsMono-Bold"
                    creatureLabel.fontColor = specimen.isPhantom ? color.withAlphaComponent(0.4) : color
                    creatureLabel.verticalAlignmentMode = .center
                    slotNode.addChild(creatureLabel)
                } else {
                    // Empty slot — faint "+" affordance
                    bg.fillColor = SKColor.white.withAlphaComponent(0.02)
                    bg.strokeColor = SKColor.white.withAlphaComponent(0.08)
                    bg.lineWidth = 1.0

                    // bg first, then "+" label on top
                    slotNode.addChild(bg)

                    let plus = SKLabelNode(text: "+")
                    plus.fontSize = 20
                    plus.fontColor = SKColor.white.withAlphaComponent(0.15)
                    plus.verticalAlignmentMode = .center
                    slotNode.addChild(plus)
                }
                addChild(slotNode)
                slotNodes.append(slotNode)
            }
        }
    }

    // MARK: - Breathing Animation

    override func update(_ currentTime: TimeInterval) {
        let delta = lastUpdateTime > 0 ? currentTime - lastUpdateTime : 0
        lastUpdateTime = currentTime
        breathPhase += Float(delta) * 2.0 * .pi * 0.1 // 0.1Hz regardless of frame rate
        if breathPhase > .pi * 2 { breathPhase -= .pi * 2 }
        let breathScale = 1.0 + sin(breathPhase) * 0.015

        for (index, node) in slotNodes.enumerated() {
            if reefStore.specimens[index] != nil {
                if let glow = node.childNode(withName: "glow_\(index)") {
                    glow.setScale(CGFloat(breathScale))
                }
            }
        }
    }

    // MARK: - Touch Handling

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            if let slotIndex = slotIndex(at: location),
               reefStore.specimens[slotIndex] != nil {
                touchDownTimes[slotIndex] = touch.timestamp
            }
        }
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        for touch in touches {
            let location = touch.location(in: self)
            if let slotIndex = slotIndex(at: location),
               reefStore.specimens[slotIndex] != nil {

                // Calculate velocity from tap duration (fast tap = high velocity)
                let duration = touch.timestamp - (touchDownTimes[slotIndex] ?? touch.timestamp)
                let velocity = max(0.1, min(1.0, Float(1.0 - duration * 2.0))) // 0-0.5s maps to 1.0-0.0

                onNoteOn(slotIndex, velocity)

                // Visual feedback — brief flash
                if let bg = childNode(withName: "//slotBg_\(slotIndex)") as? SKShapeNode {
                    let originalColor = bg.fillColor
                    let flash = SKAction.sequence([
                        SKAction.run { bg.fillColor = bg.strokeColor.withAlphaComponent(0.5) },
                        SKAction.wait(forDuration: 0.15),
                        SKAction.run { bg.fillColor = originalColor }
                    ])
                    bg.run(flash)
                }

                // Auto note-off after brief hold (Phase 1: implement hold for sustained notes)
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) { [weak self] in
                    self?.onNoteOff(slotIndex)
                }

                touchDownTimes.removeValue(forKey: slotIndex)
            }
        }
    }

    // MARK: - Hit Testing

    private func slotIndex(at point: CGPoint) -> Int? {
        let cellSize = size.width / CGFloat(gridSize)

        for (index, node) in slotNodes.enumerated() {
            let dx = point.x - node.position.x
            let dy = point.y - node.position.y
            let distance = sqrt(dx * dx + dy * dy)

            if distance < cellSize * 0.45 { // Slightly larger than visual for easier tapping
                return index
            }
        }
        return nil
    }
}
