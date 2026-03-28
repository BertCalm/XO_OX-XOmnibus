import SpriteKit
import UIKit

// MARK: - Physics Categories

private let playerCategory:      UInt32 = 0x1 << 0
private let obstacleCategory:    UInt32 = 0x1 << 1
private let collectibleCategory: UInt32 = 0x1 << 2
private let currentCategory:     UInt32 = 0x1 << 3

// MARK: - Obstacle Type

private enum ObstacleType {
    case coral
    case jellyfish
    case shark
    case current
}

// MARK: - DiveScene

/// The Dive — an underwater obstacle-avoidance game where swimming IS music.
///
/// Player's horizontal position drives note selection in DiveComposer.
/// Obstacle proximity adds dissonance. Hits degrade musical fidelity.
/// Collectibles trigger chord changes and filter blooms.
///
/// The scene does NOT own the DiveComposer or the 60-second timer.
/// It reports player state upward via callbacks.
final class DiveScene: SKScene, SKPhysicsContactDelegate {

    // MARK: - Callbacks (read by DiveTab / DiveComposer)

    /// Normalized player X position (0 = left edge, 1 = right edge).
    var onPlayerPositionChanged: ((Float) -> Void)?

    /// Signed dodge speed — positive = moving right, negative = moving left.
    var onPlayerVelocityChanged: ((Float) -> Void)?

    /// Proximity of the nearest obstacle (0 = touching, 1 = far away).
    var onObstacleProximity: ((Float) -> Void)?

    /// Called when the player collides with an obstacle.
    var onHit: (() -> Void)?

    /// Called when the player collects a bioluminescent orb.
    var onCollect: (() -> Void)?

    /// Called externally on each beat — triggers a subtle visual pulse.
    var onBeatPulse: (() -> Void)?

    // MARK: - Public State

    /// Current depth zone — set externally by DiveTab. Changing it updates obstacle rates.
    var currentZone: DepthZone = .sunlit {
        didSet {
            guard currentZone != oldValue else { return }
            applyZoneTransition()
        }
    }

    // MARK: - Private Nodes

    private var playerNode: SKShapeNode!
    private var bubbleEmitter: SKEmitterNode?
    private var screenEdgeFlash: SKSpriteNode!
    private var beatPulseNode: SKSpriteNode!

    // Background layers for parallax (per zone, two copies each for seamless scroll)
    private var bgLayerNodes: [[SKSpriteNode]] = []

    // MARK: - Obstacle / Collectible Pools

    private var obstacleSpawnTimer: TimeInterval = 0
    private var collectibleSpawnTimer: TimeInterval = 0
    private var obstacleSpawnInterval: TimeInterval = 2.0
    private let collectibleSpawnInterval: TimeInterval = 5.0

    // Obstacle node cache — maintained in parallel with the scene graph so that
    // reportObstacleProximity() iterates a tight typed set rather than all children.
    private var obstacleNodes: Set<SKNode> = []

    // Jellyfish sway tracking
    private var jellyfishPhases: [SKNode: Double] = [:]

    // MARK: - Touch / Physics

    private var touchTargetX: CGFloat = 0            // X the player is moving toward
    private var lastPlayerX: CGFloat = 0             // X at end of previous frame
    private var lastUpdateTime: TimeInterval = -1    // For delta-time calculation
    private var playerVelocityX: Float = 0           // Per-frame velocity (normalised)

    // Invulnerability window after a hit
    private var isInvulnerable = false
    private var invulnerabilityTimer: TimeInterval = 0
    private let invulnerabilityDuration: TimeInterval = 0.5

    // Current (zone push) force tracking
    private var activeCurrentForce: CGFloat = 0

    // MARK: - Zone Background Colors (fallback gradient tints)

    private let zoneBgColors: [DepthZone: SKColor] = [
        .sunlit:   SKColor(red: 0.040, green: 0.063, blue: 0.094, alpha: 1.0),
        .twilight: SKColor(red: 0.031, green: 0.031, blue: 0.063, alpha: 1.0),
        .midnight: SKColor(red: 0.020, green: 0.020, blue: 0.031, alpha: 1.0),
        .abyssal:  SKColor(red: 0.008, green: 0.008, blue: 0.016, alpha: 1.0),
    ]

    // DesignTokens colours mapped to SKColor
    private static let skReefJade = SKColor(red: 0.118, green: 0.545, blue: 0.494, alpha: 1.0)
    private static let skXoGold   = SKColor(red: 0.914, green: 0.769, blue: 0.416, alpha: 1.0)
    private static let skDark     = SKColor(red: 0.039, green: 0.039, blue: 0.059, alpha: 1.0)

    // MARK: - Player Geometry Constants

    private let playerRadius: CGFloat  = 16.0
    private let playerYFraction: CGFloat = 0.25   // 25 % from bottom
    private let playerMinXFraction: CGFloat = 0.10
    private let playerMaxXFraction: CGFloat = 0.90

    // MARK: - Scene Setup

    override func didMove(to view: SKView) {
        backgroundColor = Self.skDark
        isUserInteractionEnabled = true

        physicsWorld.gravity = .zero   // Underwater — no gravity
        physicsWorld.contactDelegate = self

        touchTargetX  = size.width * 0.5
        lastPlayerX   = size.width * 0.5

        buildBackground()
        buildScreenFlash()
        buildPlayer()
        buildBubbleTrail()
        updateObstacleSpawnInterval()
    }

    // MARK: - Build: Background

    private func buildBackground() {
        // Simple gradient fallback. We lay two large sprites per zone layer
        // so we can scroll them seamlessly. For now all layers use the same tint;
        // pixel-art backgrounds will be swapped in during the asset polish pass.

        let zoneCount = 4
        bgLayerNodes = Array(repeating: [], count: zoneCount)

        // Single-colour fallback texture (1×1 pixel, scaled up)
        let bgTexture = SKTexture(imageNamed: "")  // blank — colour is applied via SKColor
        _ = bgTexture  // suppress warning; we colour nodes directly below

        let color = zoneBgColors[currentZone] ?? Self.skDark
        for i in 0..<2 {
            let node = SKSpriteNode(color: color, size: CGSize(width: size.width, height: size.height))
            node.position = CGPoint(x: size.width * 0.5,
                                    y: size.height * 0.5 + CGFloat(i) * size.height)
            node.zPosition = -10
            node.name = "bg_\(i)"
            addChild(node)
        }
    }

    private func applyZoneTransition() {
        let newColor = zoneBgColors[currentZone] ?? Self.skDark
        let darken = SKAction.sequence([
            SKAction.colorize(with: newColor, colorBlendFactor: 1.0, duration: 2.0)
        ])
        children
            .filter { $0.name?.hasPrefix("bg_") == true }
            .compactMap { $0 as? SKSpriteNode }
            .forEach { $0.run(darken) }

        updateObstacleSpawnInterval()
    }

    // MARK: - Build: Screen Flash Overlay

    private func buildScreenFlash() {
        let edgeFlash = SKSpriteNode(color: .red, size: size)
        edgeFlash.position = CGPoint(x: size.width * 0.5, y: size.height * 0.5)
        edgeFlash.zPosition = 100
        edgeFlash.alpha = 0
        edgeFlash.name = "edgeFlash"
        addChild(edgeFlash)
        screenEdgeFlash = edgeFlash

        let pulse = SKSpriteNode(color: Self.skReefJade, size: size)
        pulse.position = CGPoint(x: size.width * 0.5, y: size.height * 0.5)
        pulse.zPosition = 99
        pulse.alpha = 0
        pulse.name = "beatPulse"
        addChild(pulse)
        beatPulseNode = pulse
    }

    // MARK: - Build: Player

    private func buildPlayer() {
        let node = SKShapeNode(circleOfRadius: playerRadius)
        node.fillColor = Self.skReefJade
        node.strokeColor = Self.skReefJade.withAlphaComponent(0.6)
        node.lineWidth = 1.5
        node.position = CGPoint(x: size.width * 0.5,
                                y: size.height * playerYFraction)
        node.zPosition = 10
        node.name = "player"

        // Physics
        let body = SKPhysicsBody(circleOfRadius: playerRadius - 2)
        body.isDynamic = true
        body.affectedByGravity = false
        body.allowsRotation = false
        body.linearDamping = 8.0
        body.categoryBitMask    = playerCategory
        body.contactTestBitMask = obstacleCategory | collectibleCategory | currentCategory
        body.collisionBitMask   = 0   // We handle response manually
        node.physicsBody = body

        addChild(node)
        playerNode = node

        // Subtle bobbing animation
        let bobUp   = SKAction.moveBy(x: 0, y: 4,  duration: 0.9)
        let bobDown = SKAction.moveBy(x: 0, y: -4, duration: 0.9)
        bobUp.timingMode   = .easeInEaseOut
        bobDown.timingMode = .easeInEaseOut
        playerNode.run(.repeatForever(.sequence([bobUp, bobDown])), withKey: "bob")

        // Inner glow ring
        let glowRing = SKShapeNode(circleOfRadius: playerRadius + 4)
        glowRing.fillColor   = .clear
        glowRing.strokeColor = Self.skReefJade.withAlphaComponent(0.25)
        glowRing.lineWidth   = 2
        glowRing.name        = "glowRing"
        playerNode.addChild(glowRing)

        let glowPulse = SKAction.sequence([
            SKAction.fadeAlpha(to: 0.5, duration: 0.6),
            SKAction.fadeAlpha(to: 0.1, duration: 0.6),
        ])
        glowRing.run(.repeatForever(glowPulse))
    }

    // MARK: - Build: Bubble Trail

    private func buildBubbleTrail() {
        // Programmatic emitter — no .sks file dependency
        let emitter = SKEmitterNode()
        emitter.particleBirthRate        = 8
        emitter.particleLifetime         = 1.2
        emitter.particleLifetimeRange    = 0.4
        emitter.particlePositionRange    = CGVector(dx: playerRadius * 0.8, dy: 0)
        emitter.particleSpeed            = 22
        emitter.particleSpeedRange       = 10
        emitter.emissionAngle            = .pi * 1.5    // Upward (270°)
        emitter.emissionAngleRange       = .pi * 0.3
        emitter.particleScale            = 0.04
        emitter.particleScaleRange       = 0.02
        emitter.particleScaleSpeed       = -0.02
        emitter.particleAlpha            = 0.6
        emitter.particleAlphaSpeed       = -0.5
        emitter.particleColor            = SKColor(red: 0.7, green: 0.9, blue: 1.0, alpha: 1.0)
        emitter.particleColorBlendFactor = 1.0
        emitter.targetNode               = self
        emitter.zPosition                = 9
        emitter.name                     = "bubbles"
        playerNode.addChild(emitter)
        bubbleEmitter = emitter
    }

    // MARK: - Obstacle Spawn Rate

    private func updateObstacleSpawnInterval() {
        switch currentZone {
        case .sunlit:   obstacleSpawnInterval = 2.0
        case .twilight: obstacleSpawnInterval = 1.2
        case .midnight: obstacleSpawnInterval = 0.7
        case .abyssal:  obstacleSpawnInterval = 1.5
        }
    }

    // MARK: - Spawners

    private func spawnObstacle() {
        // Weight obstacle type by zone
        let type: ObstacleType
        switch currentZone {
        case .sunlit:
            type = [.coral, .coral, .jellyfish].randomElement()!
        case .twilight:
            type = [.coral, .jellyfish, .jellyfish, .current].randomElement()!
        case .midnight:
            type = [.coral, .jellyfish, .shark, .shark].randomElement()!
        case .abyssal:
            // Fewer but bigger obstacles in the abyss
            type = [.coral, .current, .shark].randomElement()!
        }

        switch type {
        case .coral:     spawnCoral()
        case .jellyfish: spawnJellyfish()
        case .shark:     spawnShark()
        case .current:   spawnCurrent()
        }
    }

    private func spawnCoral() {
        let radius = currentZone == .abyssal
            ? CGFloat.random(in: 22...38)
            : CGFloat.random(in: 12...24)

        let node = SKShapeNode(circleOfRadius: radius)
        node.fillColor   = SKColor(red: 0.85, green: 0.25, blue: 0.35, alpha: 0.9)
        node.strokeColor = SKColor(red: 1.0,  green: 0.4,  blue: 0.5,  alpha: 0.6)
        node.lineWidth   = 1.5
        node.name        = "obstacle_coral"
        node.zPosition   = 5

        let xRange = size.width * playerMinXFraction ... size.width * playerMaxXFraction
        node.position = CGPoint(
            x: CGFloat.random(in: xRange),
            y: size.height + radius + 20
        )

        let body = SKPhysicsBody(circleOfRadius: radius - 2)
        body.isDynamic           = false
        body.categoryBitMask     = obstacleCategory
        body.contactTestBitMask  = playerCategory
        body.collisionBitMask    = 0
        node.physicsBody = body

        addChild(node)
        obstacleNodes.insert(node)
        animateObstacleDown(node: node, speed: scrollSpeedForZone())
    }

    private func spawnJellyfish() {
        let size2: CGFloat = CGFloat.random(in: 14...22)
        let node = SKShapeNode(ellipseOf: CGSize(width: size2, height: size2 * 0.75))
        node.fillColor   = SKColor(red: 0.6, green: 0.4, blue: 0.9, alpha: 0.7)
        node.strokeColor = SKColor(red: 0.8, green: 0.6, blue: 1.0, alpha: 0.5)
        node.lineWidth   = 1
        node.name        = "obstacle_jellyfish"
        node.zPosition   = 5

        let xRange = size.width * 0.15 ... size.width * 0.85
        node.position = CGPoint(
            x: CGFloat.random(in: xRange),
            y: self.size.height + size2 + 20
        )

        let body = SKPhysicsBody(ellipseOf: CGSize(width: size2 - 4, height: size2 * 0.75 - 4))
        body.isDynamic          = false
        body.categoryBitMask    = obstacleCategory
        body.contactTestBitMask = playerCategory
        body.collisionBitMask   = 0
        node.physicsBody = body

        addChild(node)
        obstacleNodes.insert(node)
        jellyfishPhases[node] = Double.random(in: 0 ..< .pi * 2)
        animateObstacleDown(node: node, speed: scrollSpeedForZone() * 0.7)
    }

    private func spawnShark() {
        let w: CGFloat = 50
        let h: CGFloat = 20

        let node = SKShapeNode(rectOf: CGSize(width: w, height: h), cornerRadius: 6)
        node.fillColor   = SKColor(red: 0.3, green: 0.3, blue: 0.4, alpha: 0.9)
        node.strokeColor = SKColor(red: 0.5, green: 0.5, blue: 0.7, alpha: 0.6)
        node.lineWidth   = 1.5
        node.name        = "obstacle_shark"
        node.zPosition   = 5

        // Sharks cross horizontally at a random Y in the upper 60% of the scene
        let yPos = CGFloat.random(in: size.height * 0.4 ... size.height * 0.9)
        let goingRight = Bool.random()

        let startX: CGFloat = goingRight ? -(w * 0.5 + 10) : size.width + w * 0.5 + 10
        let endX:   CGFloat = goingRight ? size.width + w * 0.5 + 10 : -(w * 0.5 + 10)

        node.position = CGPoint(x: startX, y: yPos)
        node.xScale   = goingRight ? 1 : -1   // Flip facing direction

        let body = SKPhysicsBody(rectangleOf: CGSize(width: w - 6, height: h - 4))
        body.isDynamic          = false
        body.categoryBitMask    = obstacleCategory
        body.contactTestBitMask = playerCategory
        body.collisionBitMask   = 0
        node.physicsBody = body

        addChild(node)
        obstacleNodes.insert(node)

        let crossDuration = Double.random(in: 2.5...4.5)
        let move   = SKAction.moveTo(x: endX, duration: crossDuration)
        let cleanup = SKAction.run { [weak self, weak node] in
            guard let node else { return }
            self?.obstacleNodes.remove(node)
        }
        let remove = SKAction.removeFromParent()
        node.run(.sequence([move, cleanup, remove]))

        // Tail indicator lines (visual cue a shark is incoming)
        addSharkWarningIndicator(atY: yPos, goingRight: goingRight)
    }

    private func addSharkWarningIndicator(atY y: CGFloat, goingRight: Bool) {
        let lineLength: CGFloat = 30
        let edgeX: CGFloat = goingRight ? 0 : size.width

        let line = SKShapeNode()
        let path = CGMutablePath()
        path.move(to: CGPoint(x: edgeX, y: y - 6))
        path.addLine(to: CGPoint(x: edgeX + (goingRight ? lineLength : -lineLength), y: y - 6))
        path.move(to: CGPoint(x: edgeX, y: y + 6))
        path.addLine(to: CGPoint(x: edgeX + (goingRight ? lineLength : -lineLength), y: y + 6))
        line.path        = path
        line.strokeColor = SKColor(red: 1.0, green: 0.3, blue: 0.3, alpha: 0.7)
        line.lineWidth   = 2
        line.zPosition   = 6
        line.name        = "shark_warning"
        addChild(line)

        line.run(.sequence([
            .wait(forDuration: 1.2),
            .fadeOut(withDuration: 0.5),
            .removeFromParent()
        ]))
    }

    private func spawnCurrent() {
        let w: CGFloat = size.width * CGFloat.random(in: 0.3...0.55)
        let h: CGFloat = CGFloat.random(in: 60...120)

        let node = SKShapeNode(rectOf: CGSize(width: w, height: h))
        // Subtle visual — just faint horizontal lines
        node.fillColor   = SKColor(red: 0.2, green: 0.6, blue: 0.9, alpha: 0.06)
        node.strokeColor = SKColor(red: 0.4, green: 0.7, blue: 1.0, alpha: 0.15)
        node.lineWidth   = 1
        node.name        = "obstacle_current"
        node.zPosition   = 4

        // Horizontal stripe lines inside the current band
        let stripeNode = SKShapeNode()
        let sp = CGMutablePath()
        for yOff: CGFloat in stride(from: -h * 0.4, through: h * 0.4, by: 12) {
            sp.move(to: CGPoint(x: -w * 0.45, y: yOff))
            sp.addLine(to: CGPoint(x: w * 0.45,  y: yOff))
        }
        stripeNode.path        = sp
        stripeNode.strokeColor = SKColor(red: 0.5, green: 0.8, blue: 1.0, alpha: 0.12)
        stripeNode.lineWidth   = 1
        node.addChild(stripeNode)

        let goRight = Bool.random()
        node.position = CGPoint(
            x: goRight
                ? CGFloat.random(in: size.width * 0.1 ... size.width * 0.45)
                : CGFloat.random(in: size.width * 0.55 ... size.width * 0.9),
            y: self.size.height + h * 0.5 + 20
        )

        let body = SKPhysicsBody(rectangleOf: CGSize(width: w - 4, height: h - 4))
        body.isDynamic          = false
        body.categoryBitMask    = currentCategory
        body.contactTestBitMask = playerCategory
        body.collisionBitMask   = 0
        node.physicsBody        = body
        node.userData           = NSMutableDictionary()
        node.userData?["pushRight"] = goRight

        addChild(node)
        obstacleNodes.insert(node)
        animateObstacleDown(node: node, speed: scrollSpeedForZone() * 0.8)
    }

    private func spawnCollectibleOrb() {
        let radius: CGFloat = 8

        let node = SKShapeNode(circleOfRadius: radius)
        node.fillColor   = SKColor(red: 0.49, green: 1.0, blue: 0.7, alpha: 0.9)
        node.strokeColor = SKColor(red: 0.7,  green: 1.0, blue: 0.85, alpha: 0.6)
        node.lineWidth   = 1.5
        node.name        = "collectible_orb"
        node.zPosition   = 6

        let xRange = size.width * 0.15 ... size.width * 0.85
        node.position = CGPoint(
            x: CGFloat.random(in: xRange),
            y: self.size.height + radius + 20
        )

        // Physics
        let body = SKPhysicsBody(circleOfRadius: radius + 4)  // Slightly generous hitbox
        body.isDynamic          = false
        body.categoryBitMask    = collectibleCategory
        body.contactTestBitMask = playerCategory
        body.collisionBitMask   = 0
        node.physicsBody = body

        addChild(node)

        // Outer glow ring
        let glow = SKShapeNode(circleOfRadius: radius + 6)
        glow.fillColor   = .clear
        glow.strokeColor = SKColor(red: 0.49, green: 1.0, blue: 0.7, alpha: 0.3)
        glow.lineWidth   = 2
        node.addChild(glow)

        // Pulse animation
        let scaleUp   = SKAction.scale(to: 1.2, duration: 0.6)
        let scaleDown = SKAction.scale(to: 0.9, duration: 0.6)
        scaleUp.timingMode   = .easeInEaseOut
        scaleDown.timingMode = .easeInEaseOut
        node.run(.repeatForever(.sequence([scaleUp, scaleDown])))

        // Scroll down with obstacles
        animateObstacleDown(node: node, speed: scrollSpeedForZone() * 0.75)
    }

    // MARK: - Obstacle Motion Helpers

    private func scrollSpeedForZone() -> CGFloat {
        // Points per second at which obstacles move downward
        switch currentZone {
        case .sunlit:   return CGFloat.random(in: 110...150)
        case .twilight: return CGFloat.random(in: 150...210)
        case .midnight: return CGFloat.random(in: 220...300)
        case .abyssal:  return CGFloat.random(in: 160...200)   // Slower but bigger
        }
    }

    private func animateObstacleDown(node: SKNode, speed: CGFloat) {
        // Distance to travel: from spawn Y to -50 (off bottom)
        let distance = node.position.y + 50
        guard speed > 0 else { return }
        let duration = TimeInterval(distance / speed)
        let move    = SKAction.moveTo(y: -50, duration: duration)
        let cleanup = SKAction.run { [weak self, weak node] in
            guard let node else { return }
            self?.obstacleNodes.remove(node)
        }
        let remove  = SKAction.removeFromParent()
        node.run(.sequence([move, cleanup, remove]), withKey: "scroll")
    }

    // MARK: - Touch Handling

    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        handleTouchPosition(touch.location(in: self))
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        guard let touch = touches.first else { return }
        handleTouchPosition(touch.location(in: self))
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        // No special action needed on release
    }

    private func handleTouchPosition(_ location: CGPoint) {
        let minX = size.width * playerMinXFraction
        let maxX = size.width * playerMaxXFraction
        touchTargetX = min(max(location.x, minX), maxX)

        // Smooth-move the player node towards the finger
        playerNode.removeAction(forKey: "moveToTouch")
        let move = SKAction.moveTo(x: touchTargetX, duration: 0.08)
        move.timingMode = .easeOut
        playerNode.run(move, withKey: "moveToTouch")
    }

    // MARK: - Physics Contact

    func didBegin(_ contact: SKPhysicsContact) {
        let a = contact.bodyA
        let b = contact.bodyB

        let isPlayerA = a.categoryBitMask == playerCategory
        let isPlayerB = b.categoryBitMask == playerCategory
        guard isPlayerA || isPlayerB else { return }

        let otherBody = isPlayerA ? b : a
        let otherNode = isPlayerA ? b.node : a.node

        if otherBody.categoryBitMask == obstacleCategory {
            handleHit(obstacleNode: otherNode)
        } else if otherBody.categoryBitMask == collectibleCategory {
            handleCollect(orbNode: otherNode)
        } else if otherBody.categoryBitMask == currentCategory {
            handleCurrentEntry(currentNode: otherNode)
        }
    }

    func didEnd(_ contact: SKPhysicsContact) {
        // Current exit — remove push force
        let a = contact.bodyA
        let b = contact.bodyB
        let other = a.categoryBitMask == currentCategory ? a : b
        if other.categoryBitMask == currentCategory {
            activeCurrentForce = 0
        }
    }

    private func handleHit(obstacleNode: SKNode?) {
        guard !isInvulnerable else { return }
        isInvulnerable = true
        invulnerabilityTimer = invulnerabilityDuration

        // Flash player red
        let flashRed  = SKAction.colorize(with: .red, colorBlendFactor: 0.8, duration: 0.1)
        let restoreColor = SKAction.colorize(with: Self.skReefJade, colorBlendFactor: 0.0, duration: 0.3)
        playerNode.run(.sequence([flashRed, restoreColor]))

        // Screen edge flash
        screenEdgeFlash.alpha = 0.35
        screenEdgeFlash.run(.fadeOut(withDuration: 0.4))

        // Bubble burst
        bubbleEmitter?.particleBirthRate = 40
        let resetBirthRate = SKAction.sequence([
            .wait(forDuration: 0.25),
            .run { [weak self] in self?.bubbleEmitter?.particleBirthRate = 8 }
        ])
        run(resetBirthRate)

        onHit?()
    }

    private func handleCollect(orbNode: SKNode?) {
        guard let orb = orbNode else { return }

        // Expand golden ring effect at orb position
        let ring = SKShapeNode(circleOfRadius: 8)
        ring.fillColor   = .clear
        ring.strokeColor = Self.skXoGold
        ring.lineWidth   = 2
        ring.position    = orb.position
        ring.zPosition   = 20
        addChild(ring)

        ring.run(.sequence([
            .group([
                .scale(to: 3.5, duration: 0.4),
                .fadeOut(withDuration: 0.4)
            ]),
            .removeFromParent()
        ]))

        // Scatter micro-particles
        for _ in 0..<6 {
            let spark = SKShapeNode(circleOfRadius: 2)
            spark.fillColor = Self.skXoGold
            spark.position  = orb.position
            spark.zPosition = 21
            addChild(spark)
            let angle = CGFloat.random(in: 0 ..< .pi * 2)
            let dist  = CGFloat.random(in: 30...60)
            let dx = cos(angle) * dist
            let dy = sin(angle) * dist
            spark.run(.sequence([
                .group([
                    .moveBy(x: dx, y: dy, duration: 0.5),
                    .fadeOut(withDuration: 0.5)
                ]),
                .removeFromParent()
            ]))
        }

        orb.removeFromParent()
        onCollect?()
    }

    private func handleCurrentEntry(currentNode: SKNode?) {
        guard let node = currentNode else { return }
        let pushRight = node.userData?["pushRight"] as? Bool ?? true
        // Apply a steady lateral push while overlapping
        let pushStrength: CGFloat = 45
        activeCurrentForce = pushRight ? pushStrength : -pushStrength
    }

    // MARK: - Beat Pulse (called externally)

    func triggerBeatPulse() {
        beatPulseNode.alpha = 0.08
        beatPulseNode.run(.fadeOut(withDuration: 0.15))
        onBeatPulse?()

        // Subtle bubble rate bump on beat
        let prev = bubbleEmitter?.particleBirthRate ?? 8
        bubbleEmitter?.particleBirthRate = prev + 4
        let restore = SKAction.sequence([
            .wait(forDuration: 0.12),
            .run { [weak self] in self?.bubbleEmitter?.particleBirthRate = prev }
        ])
        run(restore)
    }

    // MARK: - Update Loop

    override func update(_ currentTime: TimeInterval) {
        // Compute delta time
        let dt: TimeInterval = lastUpdateTime < 0 ? 0 : min(currentTime - lastUpdateTime, 0.05)
        lastUpdateTime = currentTime

        guard dt > 0 else { return }

        // Invulnerability countdown
        if isInvulnerable {
            invulnerabilityTimer -= dt
            if invulnerabilityTimer <= 0 {
                isInvulnerable = false
                playerNode.alpha = 1.0
                playerNode.removeAction(forKey: "blink")
            } else {
                // Blink the player during invulnerability
                let blinkAlpha: CGFloat = sin(CGFloat(invulnerabilityTimer) * 30) > 0 ? 1.0 : 0.35
                playerNode.alpha = blinkAlpha
            }
        }

        // Apply current (zone push) force to the player's target X
        if activeCurrentForce != 0 {
            let pushed = touchTargetX + activeCurrentForce * CGFloat(dt)
            let minX   = size.width * playerMinXFraction
            let maxX   = size.width * playerMaxXFraction
            touchTargetX = min(max(pushed, minX), maxX)
            let move = SKAction.moveTo(x: touchTargetX, duration: 0.06)
            playerNode.run(move, withKey: "currentPush")
        }

        // Compute velocity from position change
        let currentX = playerNode.position.x
        let dxPixels = currentX - lastPlayerX
        playerVelocityX = Float(dxPixels / CGFloat(dt)) / Float(size.width)
        lastPlayerX = currentX

        // Report normalized position (0-1)
        let normalizedX = Float((currentX - size.width * playerMinXFraction) /
                                (size.width * (playerMaxXFraction - playerMinXFraction)))
        onPlayerPositionChanged?(max(0, min(1, normalizedX)))
        onPlayerVelocityChanged?(playerVelocityX)

        // Update jellyfish sinusoidal drift
        updateJellyfish(dt: dt)

        // Prune off-screen and stale jellyfish phase entries
        pruneOffScreenNodes()

        // Obstacle proximity reporting
        reportObstacleProximity()

        // Obstacle spawn timer
        obstacleSpawnTimer += dt
        if obstacleSpawnTimer >= obstacleSpawnInterval {
            obstacleSpawnTimer = 0
            spawnObstacle()
        }

        // Collectible spawn timer
        collectibleSpawnTimer += dt
        if collectibleSpawnTimer >= collectibleSpawnInterval {
            collectibleSpawnTimer = 0
            spawnCollectibleOrb()
        }
    }

    // MARK: - Jellyfish Drift

    private func updateJellyfish(dt: TimeInterval) {
        let driftAmplitude: CGFloat = 28
        let driftFrequency: Double  = 0.8    // Cycles per second

        for (node, phase) in jellyfishPhases {
            guard node.parent != nil else {
                jellyfishPhases.removeValue(forKey: node)
                continue
            }
            let newPhase = phase + dt * .pi * 2 * driftFrequency
            jellyfishPhases[node] = newPhase

            // Lateral drift: add to the node's current X on top of its scroll action
            // We do this by offsetting xScale (keeps physics body centred)
            let xOffset = driftAmplitude * CGFloat(sin(newPhase))
            let baseX   = node.userData?["baseX"] as? CGFloat ?? node.position.x
            if node.userData == nil { node.userData = NSMutableDictionary() }
            if node.userData?["baseX"] == nil { node.userData?["baseX"] = node.position.x }
            node.position.x = baseX + xOffset
        }
    }

    // MARK: - Proximity Reporting

    private func reportObstacleProximity() {
        let playerPos  = playerNode.position
        var minDist: CGFloat = .infinity

        for node in obstacleNodes {
            let d = hypot(node.position.x - playerPos.x, node.position.y - playerPos.y)
            if d < minDist { minDist = d }
        }

        // Normalise: 0 = very close (within 40pt), 1 = 250pt or farther
        let proximity = Float(min(max((minDist - 40) / 210, 0), 1))
        onObstacleProximity?(proximity)
    }

    // MARK: - Off-Screen Cleanup

    private func pruneOffScreenNodes() {
        let bottomEdge: CGFloat = -60
        children
            .filter { node in
                guard let name = node.name else { return false }
                let isObstacle = name.hasPrefix("obstacle_") ||
                                 name == "collectible_orb" ||
                                 name == "shark_warning"
                return isObstacle && node.position.y < bottomEdge
            }
            .forEach { node in
                jellyfishPhases.removeValue(forKey: node)
                obstacleNodes.remove(node)
                node.removeFromParent()
            }
    }

    // MARK: - Required Init

    required init?(coder: NSCoder) {
        fatalError("DiveScene must be initialised programmatically")
    }

    override init(size: CGSize) {
        super.init(size: size)
    }
}
