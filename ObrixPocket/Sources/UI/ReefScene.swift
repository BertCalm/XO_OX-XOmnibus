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

    // Deferred tap — slot recorded on touchesBegan, note played on touchesEnded (avoids spurious note on long-press/wiring)
    private var pendingTapSlot: Int?

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

    // Background texture cache — only re-render when scene size changes
    private var cachedBgTexture: SKTexture?
    private var cachedBgSize: CGSize?

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

    private func renderBackground() -> SKTexture {
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
        return SKTexture(image: gradientImage)
    }

    // MARK: - Category Shapes

    /// Returns a category-specific SKShapeNode for occupied reef slots.
    /// Source = circle (origin), Processor = rounded square (transforms),
    /// Modulator = diamond (dynamic, moving), Effect = hexagon (environmental, spatial).
    private func shapeForCategory(_ category: SpecimenCategory, radius: CGFloat) -> SKShapeNode {
        switch category {
        case .source:
            return SKShapeNode(circleOfRadius: radius)
        case .processor:
            let size = radius * 1.7
            let rect = CGRect(x: -size / 2, y: -size / 2, width: size, height: size)
            return SKShapeNode(rect: rect, cornerRadius: radius * 0.3)
        case .modulator:
            let path = UIBezierPath()
            path.move(to: CGPoint(x: 0, y: radius))
            path.addLine(to: CGPoint(x: radius, y: 0))
            path.addLine(to: CGPoint(x: 0, y: -radius))
            path.addLine(to: CGPoint(x: -radius, y: 0))
            path.close()
            return SKShapeNode(path: path.cgPath)
        case .effect:
            let path = UIBezierPath()
            for i in 0..<6 {
                let angle = CGFloat(i) * (.pi / 3) - .pi / 6
                let point = CGPoint(x: radius * cos(angle), y: radius * sin(angle))
                if i == 0 { path.move(to: point) }
                else { path.addLine(to: point) }
            }
            path.close()
            return SKShapeNode(path: path.cgPath)
        }
    }

    private func buildGrid() {
        removeAllChildren()
        slotNodes.removeAll()

        // Ocean floor gradient background — use cached texture unless scene size changed
        let bgSize = self.size
        if cachedBgTexture == nil || cachedBgSize != bgSize {
            cachedBgTexture = renderBackground()
            cachedBgSize = bgSize
        }
        let bgNode = SKSpriteNode(texture: cachedBgTexture!)
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
                // Occupied slots use category-specific shape; empty slots stay as neutral circles
                let bg: SKShapeNode
                if let specimen = reefStore.specimens[index] {
                    bg = shapeForCategory(specimen.category, radius: bgRadius)
                } else {
                    bg = SKShapeNode(circleOfRadius: bgRadius)
                }
                bg.name = "slotBg_\(index)"

                if let specimen = reefStore.specimens[index] {
                    let color = categoryColors[specimen.category] ?? .white
                    let healthAlpha = CGFloat(specimen.health) / 100.0

                    bg.fillColor = color.withAlphaComponent(specimen.isPhantom ? 0.15 : 0.25 * healthAlpha)
                    bg.strokeColor = color.withAlphaComponent(specimen.isPhantom ? 0.3 : 0.6)
                    bg.lineWidth = specimen.rarity == .legendary ? 3.0 : (specimen.rarity == .rare ? 2.0 : 1.0)

                    if !specimen.isPhantom && specimen.health > 0 {
                        // Glow radius scales with rarity — rarer = bigger halo
                        let glowRadiusMultiplier: CGFloat = {
                            switch specimen.rarity {
                            case .common:    return 1.2
                            case .uncommon:  return 1.3
                            case .rare:      return 1.5
                            case .legendary: return 1.8
                            }
                        }()
                        // Glow shape matches the specimen category shape
                        let glow = shapeForCategory(specimen.category, radius: bgRadius * glowRadiusMultiplier)
                        glow.fillColor = color.withAlphaComponent(0.05 * healthAlpha)
                        glow.strokeColor = .clear
                        glow.name = "glow_\(index)"
                        slotNode.addChild(glow)

                        // Legendary: add a second outer bioluminescent aura ring (category shape)
                        if specimen.rarity == .legendary {
                            let outerGlow = shapeForCategory(specimen.category, radius: bgRadius * 1.6)
                            outerGlow.fillColor = color.withAlphaComponent(0.03)
                            outerGlow.strokeColor = color.withAlphaComponent(0.15)
                            outerGlow.lineWidth = 0.5
                            outerGlow.name = "outerGlow_\(index)"
                            slotNode.addChild(outerGlow)
                        }
                    }

                    slotNode.addChild(bg)

                    // Creature sprite or text fallback
                    if let spriteImage = UIImage(named: specimen.subtype) {
                        let texture = SKTexture(image: spriteImage)
                        texture.filteringMode = .nearest
                        let sprite = SKSpriteNode(texture: texture)
                        sprite.size = CGSize(width: bgRadius * 1.2, height: bgRadius * 1.2)
                        sprite.position = CGPoint(x: 0, y: 4) // Nudge up to make room for label
                        if specimen.isPhantom { sprite.alpha = 0.4 }
                        slotNode.addChild(sprite)
                    } else {
                        let creatureLabel = SKLabelNode(text: String(specimen.name.prefix(2)).uppercased())
                        creatureLabel.fontSize = 14
                        creatureLabel.fontName = "JetBrainsMono-Bold"
                        creatureLabel.fontColor = specimen.isPhantom ? color.withAlphaComponent(0.4) : color
                        creatureLabel.verticalAlignmentMode = .center
                        creatureLabel.position = CGPoint(x: 0, y: 4)
                        slotNode.addChild(creatureLabel)
                    }

                    // Role label below sprite — shows musical function
                    let roleText = Self.roleLabel(for: specimen)
                    let roleLabel = SKLabelNode(text: roleText)
                    roleLabel.fontSize = 7
                    roleLabel.fontName = "JetBrainsMono-Regular"
                    roleLabel.fontColor = color.withAlphaComponent(0.7)
                    roleLabel.verticalAlignmentMode = .top
                    roleLabel.position = CGPoint(x: 0, y: -bgRadius * 0.55)
                    slotNode.addChild(roleLabel)

                    // Provenance label — shows autobiography context (source track, weather, or catch date)
                    let provText = Self.provenanceLabel(for: specimen)
                    let provLabel = SKLabelNode(text: provText)
                    provLabel.fontSize = 6
                    provLabel.fontName = "JetBrainsMono-Regular"
                    provLabel.fontColor = SKColor.white.withAlphaComponent(0.25)
                    provLabel.verticalAlignmentMode = .top
                    provLabel.position = CGPoint(x: 0, y: -bgRadius * 0.55 - 10)
                    slotNode.addChild(provLabel)

                    // Level badge — top-right corner of slot
                    let levelBadge = SKLabelNode(text: "Lv\(specimen.level)")
                    levelBadge.fontSize = 7
                    levelBadge.fontName = "JetBrainsMono-Bold"
                    levelBadge.fontColor = specimen.level >= 5
                        ? SKColor(red: 0.914, green: 0.769, blue: 0.416, alpha: 1) // XO Gold
                        : SKColor.white.withAlphaComponent(0.5)
                    levelBadge.verticalAlignmentMode = .top
                    levelBadge.horizontalAlignmentMode = .right
                    levelBadge.position = CGPoint(x: bgRadius * 0.7, y: bgRadius * 0.7)
                    slotNode.addChild(levelBadge)
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

    /// Returns a semantic color for a wire based on signal-flow validity between two categories.
    /// Green = natural flow, Yellow = unusual but possible, Red = invalid pairing.
    private func wireValidationColor(from srcCat: SpecimenCategory, to dstCat: SpecimenCategory) -> SKColor {
        switch (srcCat, dstCat) {
        case (.source, .processor), (.source, .effect), (.modulator, _):
            return SKColor(red: 0.3, green: 0.8, blue: 0.3, alpha: 1) // Green — natural flow
        case (.source, .source):
            return SKColor(red: 1.0, green: 0.3, blue: 0.3, alpha: 1) // Red — invalid chain
        default:
            return SKColor(red: 0.9, green: 0.7, blue: 0.2, alpha: 1) // Yellow — unusual
        }
    }

    private func drawCouplingWires() {
        // Remove old wires, wire labels, and flow dots
        children.filter {
            $0.name?.hasPrefix("wire_") == true ||
            $0.name?.hasPrefix("wireLabel_") == true ||
            $0.name?.hasPrefix("flowDot_") == true
        }.forEach { $0.removeFromParent() }

        for route in reefStore.couplingRoutes {
            guard let srcIndex = slotIndexForSpecimen(id: route.sourceId),
                  let dstIndex = slotIndexForSpecimen(id: route.destId),
                  srcIndex < slotNodes.count, dstIndex < slotNodes.count else { continue }

            let srcPos = slotNodes[srcIndex].position
            let dstPos = slotNodes[dstIndex].position
            let srcSpecimen = reefStore.specimens[srcIndex]
            let dstSpecimen = reefStore.specimens[dstIndex]
            // Use validation color instead of raw category color
            let color: SKColor
            if let src = srcSpecimen, let dst = dstSpecimen {
                color = wireValidationColor(from: src.category, to: dst.category)
            } else {
                color = categoryColors[srcSpecimen?.category ?? .source] ?? .white
            }

            let wire = createWirePath(from: srcPos, to: dstPos, color: color)
            wire.name = "wire_\(route.sourceId.uuidString)_\(route.destId.uuidString)"
            wire.zPosition = 0.5 // Between background and slots
            addChild(wire)

            // Wire label at midpoint — shows connection context
            let midX = (srcPos.x + dstPos.x) / 2
            let midY = (srcPos.y + dstPos.y) / 2
            // Offset slightly along the perpendicular (same direction as the bezier curve control point)
            let dx = dstPos.x - srcPos.x
            let dy = dstPos.y - srcPos.y
            let perpX = -dy * 0.1
            let perpY = dx * 0.1
            let labelText = Self.wireContextLabel(src: srcSpecimen, dst: dstSpecimen)
            let wireLabel = SKLabelNode(text: labelText)
            wireLabel.fontSize = 6
            wireLabel.fontName = "JetBrainsMono-Regular"
            wireLabel.fontColor = color.withAlphaComponent(0.4)
            wireLabel.verticalAlignmentMode = .center
            wireLabel.horizontalAlignmentMode = .center
            wireLabel.position = CGPoint(x: midX + perpX, y: midY + perpY)
            wireLabel.zPosition = 0.7
            wireLabel.name = "wireLabel_\(route.sourceId.uuidString)_\(route.destId.uuidString)"
            addChild(wireLabel)

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

    /// Determine the contextual label for a wire between two specimens
    private static func wireContextLabel(src: Specimen?, dst: Specimen?) -> String {
        guard let s = src, let d = dst else { return "Wired" }

        // Same source track → they share musical DNA
        if let srcTitle = s.sourceTrackTitle, let dstTitle = d.sourceTrackTitle,
           !srcTitle.isEmpty, !dstTitle.isEmpty, srcTitle == dstTitle {
            return "Same song"
        }

        // Same category → they're family
        if s.category == d.category {
            return "Same family"
        }

        // Natural coupling pair: Source → Processor, Source → Effect, Modulator → anything
        let naturalPairs: Set<[SpecimenCategory]> = [
            [.source, .processor], [.source, .effect],
            [.modulator, .source], [.modulator, .processor], [.modulator, .effect]
        ]
        let pair = [s.category, d.category]
        let pairReversed = [d.category, s.category]
        if naturalPairs.contains(pair) || naturalPairs.contains(pairReversed) {
            return "Natural pair"
        }

        return "Wired"
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

        for (index, node) in slotNodes.enumerated() {
            guard let specimen = reefStore.specimens[index] else { continue }

            // Breath amplitude scales with rarity — Legendary pulses visibly
            let rarityBreathAmplitude: Float = {
                switch specimen.rarity {
                case .common:    return 0.015
                case .uncommon:  return 0.025
                case .rare:      return 0.04
                case .legendary: return 0.06
                }
            }()
            let breathScale = 1.0 + sin(breathPhase) * rarityBreathAmplitude

            if let glow = node.childNode(withName: "glow_\(index)") {
                glow.setScale(CGFloat(breathScale))
            }
            // Legendary outer glow ring pulses slightly out of phase for shimmer effect
            if specimen.rarity == .legendary,
               let outerGlow = node.childNode(withName: "outerGlow_\(index)") {
                let outerBreath = 1.0 + sin(breathPhase + 0.4) * 0.04
                outerGlow.setScale(CGFloat(outerBreath))
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

        // Record the slot — note will be played in touchesEnded only if this turns out to be a tap (not a long-press/wiring)
        pendingTapSlot = slot

        // Start the long-press timer for wiring
        longPressTouch = touch
        touchDownTimes[slot] = touch.timestamp
        longPressTimer?.invalidate()
        longPressTimer = Timer.scheduledTimer(withTimeInterval: longPressDuration, repeats: false) { [weak self] _ in
            guard let self else { return }
            self.beginWiring(from: slot)
        }
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
            // Normal tap — play note preview + flash
            if let slot = pendingTapSlot, reefStore.specimens[slot] != nil {
                onNoteOn(slot, 0.8)
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.3) { [weak self] in
                    self?.onNoteOff(slot)
                }
                flashSlot(slot)
                touchDownTimes.removeValue(forKey: slot)
            }
        }

        pendingTapSlot = nil
        longPressTouch = nil
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        longPressTimer?.invalidate()
        longPressTimer = nil
        cancelWiring()
        pendingTapSlot = nil
        longPressTouch = nil
    }

    // MARK: - Wiring Gestures

    private func beginWiring(from slot: Int) {
        isWiring = true
        wireSourceSlot = slot
        pendingTapSlot = nil // No tap note will be fired — long-press won

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
        let srcCategory = reefStore.specimens[srcSlot]?.category ?? .source

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

        // Highlight potential target slot and update wire color based on target category
        var hoveredTargetCategory: SpecimenCategory? = nil
        for (index, node) in slotNodes.enumerated() {
            if let bg = node.childNode(withName: "slotBg_\(index)") as? SKShapeNode {
                if index != srcSlot && reefStore.specimens[index] != nil && isNear(point, to: node.position) {
                    bg.glowWidth = 3.0
                    hoveredTargetCategory = reefStore.specimens[index]?.category
                } else if index != srcSlot {
                    bg.glowWidth = 0.0
                }
            }
        }

        // Color the in-progress wire based on what we're hovering over
        if let dstCat = hoveredTargetCategory {
            let validationColor = wireValidationColor(from: srcCategory, to: dstCat)
            activeWireLine?.strokeColor = validationColor.withAlphaComponent(0.75)
        } else {
            // No hover target — revert to source category color
            let baseColor = categoryColors[srcCategory] ?? .white
            activeWireLine?.strokeColor = baseColor.withAlphaComponent(0.6)
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
                // Create the coupling route with connection timestamp for spectral drift
                let route = CouplingRoute(sourceId: srcSpec.id, destId: dstSpec.id, depth: 0.5, connectedSince: Date())
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

        // Apply spectral drift before removing the route — specimens co-evolve
        if let route = reefStore.couplingRoutes.first(where: {
            ($0.sourceId == srcId && $0.destId == dstId) ||
            ($0.sourceId == dstId && $0.destId == srcId)
        }) {
            reefStore.applySpectralDrift(sourceId: route.sourceId, destId: route.destId, connectedSince: route.connectedSince)
        }

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

    // MARK: - Role Labels

    /// Short musical function label for a specimen (e.g., "SRC: Saw", "FLT: LP", "MOD: LFO")
    private static func roleLabel(for specimen: Specimen) -> String {
        let subtypeLabels: [String: String] = [
            // Sources
            "polyblep-saw": "SRC: Saw", "polyblep-square": "SRC: Square", "polyblep-tri": "SRC: Tri",
            "noise-white": "SRC: Noise", "noise-pink": "SRC: Pink",
            "wt-analog": "SRC: WT", "wt-vocal": "SRC: Vocal", "fm-basic": "SRC: FM",
            // Processors
            "svf-lp": "FLT: LP", "svf-hp": "FLT: HP", "svf-bp": "FLT: BP",
            "shaper-soft": "SAT: Soft", "shaper-hard": "SAT: Hard", "feedback": "FB: Loop",
            // Modulators
            "adsr-fast": "ENV: Fast", "adsr-slow": "ENV: Slow",
            "lfo-sine": "LFO: Sine", "lfo-random": "LFO: S&H",
            "vel-map": "MOD: Vel", "at-map": "MOD: AT",
            // Effects
            "delay-stereo": "FX: Delay", "chorus-lush": "FX: Chorus",
            "reverb-hall": "FX: Reverb", "dist-warm": "FX: Dist",
        ]
        return subtypeLabels[specimen.subtype] ?? specimen.category.rawValue.uppercased()
    }

    /// Provenance text for a specimen — shows where it came from
    private static func provenanceLabel(for specimen: Specimen) -> String {
        if let title = specimen.sourceTrackTitle, !title.isEmpty {
            // Truncate to ~20 chars for readability
            return String(title.prefix(20)) + (title.count > 20 ? "..." : "")
        }
        if let weather = specimen.catchWeatherDescription, !weather.isEmpty {
            return String(weather.prefix(20)) + (weather.count > 20 ? "..." : "")
        }
        // Fallback: catch date
        let formatter = DateFormatter()
        formatter.dateFormat = "MMM d, HH:mm"
        return formatter.string(from: specimen.catchTimestamp)
    }

    private func isNear(_ point: CGPoint, to target: CGPoint) -> Bool {
        let cellSize = size.width / CGFloat(gridSize)
        let dx = point.x - target.x
        let dy = point.y - target.y
        return sqrt(dx * dx + dy * dy) < cellSize * 0.45
    }
}
