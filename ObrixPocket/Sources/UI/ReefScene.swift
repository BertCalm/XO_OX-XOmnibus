import SpriteKit

/// SpriteKit scene rendering the 4x4 reef grid.
/// Tap to play notes. Long-press + drag to wire specimens together.
class ReefScene: SKScene {

    private let reefStore: ReefStore
    private let onNoteOn: (Int, Float) -> Void
    private let onNoteOff: (Int) -> Void
    private let onWiringChanged: (() -> Void)?

    // Grid geometry
    private let gridSize = 4
    private var slotNodes: [SKNode] = []

    // Wiring state
    private var wireSourceSlot: Int?          // Slot being dragged FROM
    private var activeWireLine: SKShapeNode?  // The in-progress drag line
    private var longPressTimer: Timer?
    private var longPressTouch: UITouch?
    private var isWiring = false
    private let longPressDuration: TimeInterval = 0.4

    // Organic offsets (±8px, deterministic per slot)
    private let organicOffsets: [CGPoint] = {
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
        .source: SKColor(red: 0.2, green: 0.5, blue: 1.0, alpha: 1.0),
        .processor: SKColor(red: 1.0, green: 0.3, blue: 0.3, alpha: 1.0),
        .modulator: SKColor(red: 0.3, green: 0.8, blue: 0.3, alpha: 1.0),
        .effect: SKColor(red: 0.7, green: 0.3, blue: 1.0, alpha: 1.0)
    ]

    init(size: CGSize, reefStore: ReefStore, onNoteOn: @escaping (Int, Float) -> Void, onNoteOff: @escaping (Int) -> Void, onWiringChanged: (() -> Void)? = nil) {
        self.reefStore = reefStore
        self.onNoteOn = onNoteOn
        self.onNoteOff = onNoteOff
        self.onWiringChanged = onWiringChanged
        super.init(size: size)

        self.backgroundColor = SKColor(red: 0.055, green: 0.055, blue: 0.063, alpha: 1.0)
        self.isUserInteractionEnabled = true
    }

    required init?(coder: NSCoder) { fatalError("init(coder:) not implemented") }

    override func didMove(to view: SKView) {
        buildGrid()
    }

    // MARK: - Grid Construction

    func refreshGrid() {
        buildGrid()
    }

    private func buildGrid() {
        removeAllChildren()
        slotNodes.removeAll()

        // Ocean floor gradient background
        let bgSize = self.size
        let renderer = UIGraphicsImageRenderer(size: bgSize)
        let gradientImage = renderer.image { ctx in
            let colors = [
                UIColor(red: 0.04, green: 0.06, blue: 0.12, alpha: 1.0).cgColor,
                UIColor(red: 0.06, green: 0.10, blue: 0.14, alpha: 1.0).cgColor,
                UIColor(red: 0.05, green: 0.08, blue: 0.11, alpha: 1.0).cgColor,
            ]
            let gradient = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(),
                                       colors: colors as CFArray,
                                       locations: [0.0, 0.5, 1.0])!
            ctx.cgContext.drawLinearGradient(gradient,
                                             start: CGPoint(x: 0, y: 0),
                                             end: CGPoint(x: 0, y: bgSize.height),
                                             options: [])
            // Sand speckles at the bottom
            let speckleY = bgSize.height * 0.7
            UIColor(red: 0.12, green: 0.14, blue: 0.10, alpha: 0.15).setFill()
            var rng = SystemRandomNumberGenerator()
            for _ in 0..<40 {
                let x = CGFloat.random(in: 0...bgSize.width, using: &rng)
                let y = CGFloat.random(in: speckleY...bgSize.height, using: &rng)
                let r = CGFloat.random(in: 1...3, using: &rng)
                ctx.cgContext.fillEllipse(in: CGRect(x: x, y: y, width: r, height: r))
            }
        }
        let bgTexture = SKTexture(image: gradientImage)
        let bgNode = SKSpriteNode(texture: bgTexture)
        bgNode.size = bgSize
        bgNode.position = CGPoint(x: bgSize.width / 2, y: bgSize.height / 2)
        bgNode.zPosition = -10
        addChild(bgNode)

        // Build slot nodes
        let cellSize = size.width / CGFloat(gridSize)

        for row in 0..<gridSize {
            for col in 0..<gridSize {
                let index = row * gridSize + col
                let offset = organicOffsets[index]

                let x = CGFloat(col) * cellSize + cellSize / 2 + offset.x
                let y = size.height - (CGFloat(row) * cellSize + cellSize / 2) - offset.y

                let slotNode = SKNode()
                slotNode.position = CGPoint(x: x, y: y)
                slotNode.name = "slot_\(index)"
                slotNode.zPosition = 1

                let bgRadius = cellSize * 0.4
                let bg = SKShapeNode(circleOfRadius: bgRadius)
                bg.name = "slotBg_\(index)"

                if let specimen = reefStore.specimens[index] {
                    let color = categoryColors[specimen.category] ?? .white
                    let healthAlpha = CGFloat(specimen.health) / 100.0

                    bg.fillColor = color.withAlphaComponent(specimen.isPhantom ? 0.15 : 0.25 * healthAlpha)
                    bg.strokeColor = color.withAlphaComponent(specimen.isPhantom ? 0.3 : 0.6)
                    bg.lineWidth = specimen.rarity == .legendary ? 3.0 : (specimen.rarity == .rare ? 2.0 : 1.0)

                    if !specimen.isPhantom && specimen.health > 0 {
                        let glow = SKShapeNode(circleOfRadius: bgRadius * 1.2)
                        glow.fillColor = color.withAlphaComponent(0.05 * healthAlpha)
                        glow.strokeColor = .clear
                        glow.name = "glow_\(index)"
                        slotNode.addChild(glow)
                    }

                    slotNode.addChild(bg)

                    if let spriteImage = UIImage(named: specimen.subtype) {
                        let texture = SKTexture(image: spriteImage)
                        texture.filteringMode = .nearest
                        let sprite = SKSpriteNode(texture: texture)
                        sprite.size = CGSize(width: bgRadius * 1.2, height: bgRadius * 1.2)
                        if specimen.isPhantom { sprite.alpha = 0.4 }
                        slotNode.addChild(sprite)
                    } else {
                        let creatureLabel = SKLabelNode(text: String(specimen.name.prefix(2)).uppercased())
                        creatureLabel.fontSize = 14
                        creatureLabel.fontName = "JetBrainsMono-Bold"
                        creatureLabel.fontColor = specimen.isPhantom ? color.withAlphaComponent(0.4) : color
                        creatureLabel.verticalAlignmentMode = .center
                        slotNode.addChild(creatureLabel)
                    }
                } else {
                    bg.fillColor = SKColor.white.withAlphaComponent(0.02)
                    bg.strokeColor = SKColor.white.withAlphaComponent(0.08)
                    bg.lineWidth = 1.0
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

        // Draw existing coupling wires
        drawCouplingWires()
    }

    // MARK: - Coupling Wire Rendering

    private func drawCouplingWires() {
        // Remove old wires
        children.filter { $0.name?.hasPrefix("wire_") == true }.forEach { $0.removeFromParent() }

        for route in reefStore.couplingRoutes {
            guard let srcIndex = slotIndexForSpecimen(id: route.sourceId),
                  let dstIndex = slotIndexForSpecimen(id: route.destId),
                  srcIndex < slotNodes.count, dstIndex < slotNodes.count else { continue }

            let srcPos = slotNodes[srcIndex].position
            let dstPos = slotNodes[dstIndex].position
            let srcSpecimen = reefStore.specimens[srcIndex]
            let color = categoryColors[srcSpecimen?.category ?? .source] ?? .white

            let wire = createWirePath(from: srcPos, to: dstPos, color: color)
            wire.name = "wire_\(route.sourceId.uuidString)_\(route.destId.uuidString)"
            wire.zPosition = 0.5 // Between background and slots
            addChild(wire)

            // Flow dots — small circles that pulse along the wire
            let flowDot = SKShapeNode(circleOfRadius: 3)
            flowDot.fillColor = color.withAlphaComponent(0.8)
            flowDot.strokeColor = .clear
            flowDot.zPosition = 0.6
            flowDot.name = "flowDot_\(route.sourceId.uuidString)"

            let pathAction = SKAction.follow(wireBezierPath(from: srcPos, to: dstPos).cgPath,
                                              asOffset: false, orientToPath: false,
                                              duration: 1.5)
            let loop = SKAction.repeatForever(SKAction.sequence([pathAction, SKAction.wait(forDuration: 0.3)]))
            addChild(flowDot)
            flowDot.run(loop)
        }
    }

    private func createWirePath(from src: CGPoint, to dst: CGPoint, color: SKColor) -> SKShapeNode {
        let path = wireBezierPath(from: src, to: dst)
        let wire = SKShapeNode(path: path.cgPath)
        wire.strokeColor = color.withAlphaComponent(0.5)
        wire.lineWidth = 2.5
        wire.lineCap = .round
        wire.fillColor = .clear
        wire.glowWidth = 1.0
        return wire
    }

    /// Create a curved bezier path between two points (gentle arc for visual clarity)
    private func wireBezierPath(from src: CGPoint, to dst: CGPoint) -> UIBezierPath {
        let path = UIBezierPath()
        path.move(to: src)

        // Control point offset perpendicular to the line, creating a gentle curve
        let midX = (src.x + dst.x) / 2
        let midY = (src.y + dst.y) / 2
        let dx = dst.x - src.x
        let dy = dst.y - src.y
        // Perpendicular offset (rotate 90°), scaled by 20% of distance
        let perpX = -dy * 0.2
        let perpY = dx * 0.2
        let controlPoint = CGPoint(x: midX + perpX, y: midY + perpY)

        path.addQuadCurve(to: dst, controlPoint: controlPoint)
        return path
    }

    private func slotIndexForSpecimen(id: UUID) -> Int? {
        reefStore.specimens.firstIndex(where: { $0?.id == id })
    }

    // MARK: - Breathing Animation

    override func update(_ currentTime: TimeInterval) {
        let delta = lastUpdateTime > 0 ? currentTime - lastUpdateTime : 0
        lastUpdateTime = currentTime
        breathPhase += Float(delta) * 2.0 * .pi * 0.1
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

    // MARK: - Touch Handling (Tap to play, Long-press to wire)

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        let location = touch.location(in: self)

        // If tap is NOT on a specimen slot, check for wire hit (deletion)
        if slotIndex(at: location) == nil || reefStore.specimens[slotIndex(at: location)!] == nil {
            if let wireName = wireHitTest(at: location) {
                deleteWire(named: wireName)
                return
            }
            return // Tapped empty space, no slot, no wire
        }

        // Tap IS on a specimen slot
        guard let slot = slotIndex(at: location),
              reefStore.specimens[slot] != nil else { return }

        // Start the long-press timer for wiring
        longPressTouch = touch
        touchDownTimes[slot] = touch.timestamp
        longPressTimer?.invalidate()
        longPressTimer = Timer.scheduledTimer(withTimeInterval: longPressDuration, repeats: false) { [weak self] _ in
            guard let self else { return }
            self.beginWiring(from: slot)
        }

        // Short tap plays a preview note (single specimen sound)
        onNoteOn(slot, 0.8)
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first, isWiring, wireSourceSlot != nil else { return }
        let location = touch.location(in: self)
        updateWireDrag(to: location)
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        let location = touch.location(in: self)

        // Cancel long-press timer
        longPressTimer?.invalidate()
        longPressTimer = nil

        if isWiring {
            // End wiring — check if released on a valid target
            endWiring(at: location)
        } else {
            // Normal tap — note off + flash
            if let slot = slotIndex(at: location), reefStore.specimens[slot] != nil {
                onNoteOff(slot)
                flashSlot(slot)
                touchDownTimes.removeValue(forKey: slot)
            }
        }

        longPressTouch = nil
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        longPressTimer?.invalidate()
        longPressTimer = nil
        cancelWiring()
        longPressTouch = nil
    }

    // MARK: - Wiring Gestures

    private func beginWiring(from slot: Int) {
        isWiring = true
        wireSourceSlot = slot

        // Stop the note that was triggered on touchesBegan
        onNoteOff(slot)

        // Highlight the source slot
        if let bg = childNode(withName: "//slotBg_\(slot)") as? SKShapeNode {
            bg.glowWidth = 4.0
        }

        // Create the in-progress wire line
        let srcPos = slotNodes[slot].position
        let line = SKShapeNode()
        line.strokeColor = (categoryColors[reefStore.specimens[slot]?.category ?? .source] ?? .white).withAlphaComponent(0.6)
        line.lineWidth = 2.5
        line.lineCap = .round
        line.zPosition = 2
        line.name = "activeWire"
        addChild(line)
        activeWireLine = line

        // Haptic feedback
        let generator = UIImpactFeedbackGenerator(style: .medium)
        generator.impactOccurred()
    }

    private func updateWireDrag(to point: CGPoint) {
        guard let srcSlot = wireSourceSlot, srcSlot < slotNodes.count else { return }
        let srcPos = slotNodes[srcSlot].position

        let path = UIBezierPath()
        path.move(to: srcPos)

        // Curve toward the drag point
        let midX = (srcPos.x + point.x) / 2
        let midY = (srcPos.y + point.y) / 2
        let dx = point.x - srcPos.x
        let dy = point.y - srcPos.y
        let perpX = -dy * 0.15
        let perpY = dx * 0.15
        let controlPoint = CGPoint(x: midX + perpX, y: midY + perpY)
        path.addQuadCurve(to: point, controlPoint: controlPoint)

        activeWireLine?.path = path.cgPath

        // Highlight potential target slot
        for (index, node) in slotNodes.enumerated() {
            if let bg = node.childNode(withName: "slotBg_\(index)") as? SKShapeNode {
                if index != srcSlot && reefStore.specimens[index] != nil && isNear(point, to: node.position) {
                    bg.glowWidth = 3.0
                } else if index != srcSlot {
                    bg.glowWidth = 0.0
                }
            }
        }
    }

    private func endWiring(at point: CGPoint) {
        guard let srcSlot = wireSourceSlot else {
            cancelWiring()
            return
        }

        // Find target slot
        let targetSlot = slotIndex(at: point)

        if let dstSlot = targetSlot,
           dstSlot != srcSlot,
           let srcSpec = reefStore.specimens[srcSlot],
           let dstSpec = reefStore.specimens[dstSlot] {

            // Check for existing route between these two
            let alreadyConnected = reefStore.couplingRoutes.contains {
                ($0.sourceId == srcSpec.id && $0.destId == dstSpec.id) ||
                ($0.sourceId == dstSpec.id && $0.destId == srcSpec.id)
            }

            if !alreadyConnected {
                // Create the coupling route
                let route = CouplingRoute(sourceId: srcSpec.id, destId: dstSpec.id, depth: 0.5)
                reefStore.couplingRoutes.append(route)
                reefStore.save()

                // Haptic success
                let generator = UINotificationFeedbackGenerator()
                generator.notificationOccurred(.success)
            }
        }

        cancelWiring()
        buildGrid() // Rebuild to show new wires
        onWiringChanged?() // Push updated params to engine
    }

    private func cancelWiring() {
        isWiring = false
        wireSourceSlot = nil
        activeWireLine?.removeFromParent()
        activeWireLine = nil

        // Remove all glow highlights
        for (index, _) in slotNodes.enumerated() {
            if let bg = childNode(withName: "//slotBg_\(index)") as? SKShapeNode {
                bg.glowWidth = 0.0
            }
        }
    }

    // MARK: - Wire Deletion

    private func wireHitTest(at point: CGPoint) -> String? {
        for child in children {
            guard let name = child.name, name.hasPrefix("wire_") else { continue }
            if let wire = child as? SKShapeNode, let path = wire.path {
                // Check if point is within ~15pt of the wire path
                let strokedPath = path.copy(strokingWithWidth: 30, lineCap: .round, lineJoin: .round, miterLimit: 0)
                if strokedPath.contains(point) {
                    return name
                }
            }
        }
        return nil
    }

    private func deleteWire(named wireName: String) {
        // Parse source/dest UUIDs from wire name: "wire_{srcUUID}_{dstUUID}"
        let parts = wireName.replacingOccurrences(of: "wire_", with: "").split(separator: "_", maxSplits: 1)
        guard parts.count == 2,
              let srcId = UUID(uuidString: String(parts[0])),
              let dstId = UUID(uuidString: String(parts[1])) else { return }

        reefStore.couplingRoutes.removeAll {
            ($0.sourceId == srcId && $0.destId == dstId) ||
            ($0.sourceId == dstId && $0.destId == srcId)
        }
        reefStore.save()

        // Haptic feedback
        let generator = UIImpactFeedbackGenerator(style: .light)
        generator.impactOccurred()

        buildGrid() // Rebuild to remove wire visuals
        onWiringChanged?() // Push updated params to engine
    }

    // MARK: - Visual Feedback

    private func flashSlot(_ slot: Int) {
        if let bg = childNode(withName: "//slotBg_\(slot)") as? SKShapeNode {
            let originalColor = bg.fillColor
            let flash = SKAction.sequence([
                SKAction.run { bg.fillColor = bg.strokeColor.withAlphaComponent(0.5) },
                SKAction.wait(forDuration: 0.15),
                SKAction.run { bg.fillColor = originalColor }
            ])
            bg.run(flash)
        }
    }

    // MARK: - Hit Testing

    private func slotIndex(at point: CGPoint) -> Int? {
        let cellSize = size.width / CGFloat(gridSize)
        for (index, node) in slotNodes.enumerated() {
            let dx = point.x - node.position.x
            let dy = point.y - node.position.y
            let distance = sqrt(dx * dx + dy * dy)
            if distance < cellSize * 0.45 {
                return index
            }
        }
        return nil
    }

    private func isNear(_ point: CGPoint, to target: CGPoint) -> Bool {
        let cellSize = size.width / CGFloat(gridSize)
        let dx = point.x - target.x
        let dy = point.y - target.y
        return sqrt(dx * dx + dy * dy) < cellSize * 0.45
    }
}
