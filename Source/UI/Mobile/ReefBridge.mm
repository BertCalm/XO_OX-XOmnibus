/*
  ==============================================================================

    ReefBridge.mm
    XOlokun — Objective-C++ implementation of the SpriteKit reef companion
    layer for OBRIX Pocket.

    Architecture:
      The reef occupies the top ~60% of the screen as a sibling UIView to
      the JUCE component view.  Both are children of the same UIWindow
      (or host UIView in AUv3 mode).  JUCE never renders into the reef
      region; the SKView never handles pad input.

    Scene structure:
      ReefScene (SKScene)
        camera (SKCameraNode)
        backgroundLayer  (SKNode) — slowest parallax (0.1x scroll rate)
        midgroundLayer   (SKNode) — mid parallax (0.4x scroll rate)
        foregroundLayer  (SKNode) — fastest parallax (1.0x scroll rate)
          creature nodes (SKSpriteNode, keyed by creatureId)
        particleLayer    (SKNode) — always at screen-space position (above fg)

    Creature animation:
      Each creature has a set of SKActions stored in an NSDictionary keyed by
      CreatureState. setCreatureState swaps the running action on the node.

    Scroll model:
      scrollReef(offsetX) updates gScrollX (clamped to [0, kSceneWidth]).
      The camera's position is set to (gScrollX, gReefHeight/2).
      Background layer xScale is adjusted via SKAction moveBy at differential
      rates relative to the camera.

    Thread safety:
      Public API is called on the JUCE message thread.
      setCreatureState dispatches to the main queue so SKNode mutations happen
      on the right thread even if the timer fires on a background queue.

  ==============================================================================
*/

#include "ReefBridge.h"

#if JUCE_IOS

#import <UIKit/UIKit.h>
#import <SpriteKit/SpriteKit.h>
#include <juce_core/juce_core.h>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr CGFloat kSceneWidth  = 2048.0;   // Wider than any screen; parallax room
static constexpr CGFloat kScrollMin   = 0.0;
static constexpr CGFloat kScrollMax   = kSceneWidth - 414.0; // 414 = widest iPhone pt width

// Parallax scroll multipliers per layer (foreground scrolls 1:1 with camera naturally)
static constexpr double kParallaxBackground  = 0.10;
static constexpr double kParallaxMidground   = 0.40;

// ---------------------------------------------------------------------------
// ReefScene — private Objective-C class
// ---------------------------------------------------------------------------

@interface ReefScene : SKScene

@property (nonatomic, strong) SKCameraNode* reefCamera;
@property (nonatomic, strong) SKNode*       backgroundLayer;
@property (nonatomic, strong) SKNode*       midgroundLayer;
@property (nonatomic, strong) SKNode*       foregroundLayer;
@property (nonatomic, strong) SKNode*       particleLayer;

// Creature nodes: @(creatureId) -> SKSpriteNode*
@property (nonatomic, strong) NSMutableDictionary<NSNumber*, SKSpriteNode*>* creatureNodes;

// Per-creature action dictionaries: @(creatureId) -> NSDictionary<NSNumber*, SKAction*>*
// Inner dict key is CreatureState raw int value.
@property (nonatomic, strong) NSMutableDictionary<NSNumber*, NSDictionary*>* creatureActions;

@end

@implementation ReefScene

- (instancetype)initWithSize:(CGSize)size
{
    self = [super initWithSize:size];
    if (!self)
        return nil;

    self.scaleMode      = SKSceneSizeScaleModeFill;
    self.backgroundColor = [UIColor colorWithRed:0.04 green:0.12 blue:0.22 alpha:1.0]; // Deep ocean

    self.creatureNodes   = [NSMutableDictionary dictionary];
    self.creatureActions = [NSMutableDictionary dictionary];

    [self buildLayers];
    [self buildCamera];

    return self;
}

- (void)buildLayers
{
    // Background — distant water column gradient plane
    _backgroundLayer = [SKNode node];
    _backgroundLayer.name = @"background";
    [self addChild:_backgroundLayer];

    SKSpriteNode* backdrop = [SKSpriteNode spriteNodeWithColor:
        [UIColor colorWithRed:0.06 green:0.17 blue:0.30 alpha:1.0]
        size:CGSizeMake(kSceneWidth * 2.0, self.size.height)];
    backdrop.position = CGPointMake(kSceneWidth / 2.0, self.size.height / 2.0);
    backdrop.zPosition = -10.0;
    [_backgroundLayer addChild:backdrop];

    // Midground — mid-water coral formations
    _midgroundLayer = [SKNode node];
    _midgroundLayer.name = @"midground";
    [self addChild:_midgroundLayer];

    // Foreground — creatures + close reef detail
    _foregroundLayer = [SKNode node];
    _foregroundLayer.name = @"foreground";
    [self addChild:_foregroundLayer];

    // Particle layer — always rendered above all parallax layers
    _particleLayer = [SKNode node];
    _particleLayer.name = @"particles";
    _particleLayer.zPosition = 100.0;
    [self addChild:_particleLayer];
}

- (void)buildCamera
{
    _reefCamera = [SKCameraNode node];
    _reefCamera.position = CGPointMake(self.size.width / 2.0,
                                       self.size.height / 2.0);
    [self addChild:_reefCamera];
    self.camera = _reefCamera;
}

// Update parallax layers to match the given camera X position.
// Called from scrollReef after clamping.
- (void)applyParallaxForCameraX:(CGFloat)cameraX
{
    const CGFloat originX = self.size.width / 2.0;
    const CGFloat delta   = cameraX - originX;

    _backgroundLayer.position = CGPointMake((CGFloat)(-delta * kParallaxBackground), 0.0);
    _midgroundLayer.position  = CGPointMake((CGFloat)(-delta * kParallaxMidground),  0.0);
    // Foreground moves with the camera (1x), so no adjustment needed — the
    // camera already transforms foreground children correctly.
}

// Build or return the action set for a newly added creature.
// Actions are deliberately simple for the scaffold; flesh out textures later.
- (NSDictionary*)defaultActionsForCreature
{
    // Sleeping: slow vertical bob, faded out
    SKAction* sleepBob = [SKAction sequence:@[
        [SKAction moveByX:0 y:3 duration:2.5],
        [SKAction moveByX:0 y:-3 duration:2.5]
    ]];
    SKAction* sleepFade  = [SKAction fadeAlphaTo:0.45 duration:0.6];
    SKAction* sleepGroup = [SKAction group:@[
        [SKAction repeatActionForever:sleepBob],
        sleepFade
    ]];

    // Idle: gentle swaying
    SKAction* idleSway = [SKAction sequence:@[
        [SKAction rotateByAngle:0.05  duration:1.2],
        [SKAction rotateByAngle:-0.05 duration:1.2]
    ]];
    SKAction* idleFade  = [SKAction fadeAlphaTo:0.75 duration:0.3];
    SKAction* idleGroup = [SKAction group:@[
        [SKAction repeatActionForever:idleSway],
        idleFade
    ]];

    // Curious: lean forward, full opacity
    SKAction* curiousLean = [SKAction sequence:@[
        [SKAction rotateByAngle: 0.12 duration:0.5],
        [SKAction rotateByAngle:-0.12 duration:0.5]
    ]];
    SKAction* curiousFade  = [SKAction fadeAlphaTo:0.90 duration:0.2];
    SKAction* curiousGroup = [SKAction group:@[
        [SKAction repeatActionForever:curiousLean],
        curiousFade
    ]];

    // Excited: fast bounce
    SKAction* excitedBounce = [SKAction sequence:@[
        [SKAction moveByX:0 y:8  duration:0.18],
        [SKAction moveByX:0 y:-8 duration:0.18]
    ]];
    SKAction* excitedFade  = [SKAction fadeAlphaTo:1.0 duration:0.15];
    SKAction* excitedGroup = [SKAction group:@[
        [SKAction repeatActionForever:excitedBounce],
        excitedFade
    ]];

    // Singing: fast scale pulse with full brightness
    SKAction* singPulse = [SKAction sequence:@[
        [SKAction scaleBy:1.12 duration:0.12],
        [SKAction scaleBy:(1.0/1.12) duration:0.12]
    ]];
    SKAction* singFade  = [SKAction fadeAlphaTo:1.0 duration:0.1];
    SKAction* singGroup = [SKAction group:@[
        [SKAction repeatActionForever:singPulse],
        singFade
    ]];

    return @{
        @(0) : sleepGroup,
        @(1) : idleGroup,
        @(2) : curiousGroup,
        @(3) : excitedGroup,
        @(4) : singGroup
    };
}

@end // ReefScene

// ---------------------------------------------------------------------------
// Module-private state
// ---------------------------------------------------------------------------

namespace
{
    SKView*     gReefView   = nil;
    ReefScene*  gReefScene  = nil;
    bool        gInitialized = false;
    CGFloat     gScrollX     = 0.0;
    CGFloat     gReefHeight  = 0.0;

    // Retrieve the root UIViewController / UIView that JUCE uses.
    // JUCE's Desktop::getInstance() returns the global Desktop which holds
    // a list of TopLevelWindow wrappers. On iOS, the underlying UIView is
    // accessible via the first component peer.
    UIView* juceRootView()
    {
        @autoreleasepool
        {
            // Walk all UIWindows and return the first non-nil keyWindow view.
            // This is robust for both Standalone and AUv3 contexts.
            NSArray<UIWindowScene*>* scenes =
                (NSArray<UIWindowScene*>*)[UIApplication sharedApplication].connectedScenes.allObjects;
            for (UIWindowScene* scene in scenes)
            {
                for (UIWindow* window in scene.windows)
                {
                    if (!window.isHidden)
                        return window;
                }
            }
        }
        return nil;
    }

    // Clamp gScrollX and update the camera + parallax.
    void applyScroll()
    {
        if (!gReefScene || !gReefScene.reefCamera)
            return;

        // Clamp
        if (gScrollX < kScrollMin) gScrollX = kScrollMin;
        const CGFloat scrollMax = kSceneWidth - gReefView.bounds.size.width;
        if (gScrollX > scrollMax) gScrollX = scrollMax;

        const CGFloat cameraCentreX = gReefView.bounds.size.width / 2.0 + gScrollX;
        gReefScene.reefCamera.position = CGPointMake(cameraCentreX,
                                                     gReefHeight / 2.0);
        [gReefScene applyParallaxForCameraX:cameraCentreX];
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// reef_bridge implementation
// ---------------------------------------------------------------------------

namespace xolokun::reef_bridge {

void initialize(float reefViewHeight)
{
    if (gInitialized)
        return;

    @autoreleasepool
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            UIView* rootView = juceRootView();
            if (!rootView)
            {
                juce::Logger::writeToLog("ReefBridge: initialize() — could not locate JUCE root UIView");
                return;
            }

            gReefHeight = static_cast<CGFloat>(reefViewHeight);

            // Size the SKView to the full width of the screen, capped to reefViewHeight.
            const CGRect screenBounds = [UIScreen mainScreen].bounds;
            const CGRect reefFrame = CGRectMake(0.0, 0.0,
                                                screenBounds.size.width,
                                                gReefHeight);

            gReefView = [[SKView alloc] initWithFrame:reefFrame];
            gReefView.ignoresSiblingOrder = YES;
            gReefView.allowsTransparency  = NO;
            gReefView.paused              = NO;

            // Insert as a sibling BEHIND the JUCE view (index 0 = back).
            // The JUCE component sits at a higher Z index and handles its
            // own region (bottom ~40%). The reef occupies top ~60%.
            [rootView insertSubview:gReefView atIndex:0];

            // Create scene with physical pixel size so retina looks correct.
            const CGSize sceneSize = CGSizeMake(kSceneWidth, gReefHeight);
            gReefScene = [[ReefScene alloc] initWithSize:sceneSize];
            [gReefView presentScene:gReefScene];

            gScrollX     = 0.0;
            gInitialized = true;

            juce::Logger::writeToLog("ReefBridge: initialized — reef height " +
                                     juce::String(reefViewHeight) + " pt");
        });
    }
}

void shutdown()
{
    if (!gInitialized)
        return;

    @autoreleasepool
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            [gReefView removeFromSuperview];
            [gReefView presentScene:nil];
            gReefView    = nil;
            gReefScene   = nil;
            gInitialized = false;
        });
    }
}

void addCreature(int creatureId, const char* spriteSheetName,
                 float reefX, float reefY)
{
    if (!spriteSheetName)
        return;

    NSString* sheetName = [NSString stringWithUTF8String:spriteSheetName];
    const int capturedId = creatureId;
    const CGPoint pos    = CGPointMake(static_cast<CGFloat>(reefX),
                                       static_cast<CGFloat>(reefY));

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!gInitialized || !gReefScene)
            return;

        @autoreleasepool
        {
            // Attempt to load the first frame from the named atlas.
            // Falls back to a coloured placeholder rectangle if the asset
            // is absent (expected during scaffold / prototype phase).
            SKTexture* tex = nil;
            SKTextureAtlas* atlas = [SKTextureAtlas atlasNamed:sheetName];
            if (atlas && atlas.textureNames.count > 0)
                tex = [atlas textureNamed:atlas.textureNames.firstObject];

            SKSpriteNode* sprite;
            if (tex)
            {
                sprite = [SKSpriteNode spriteNodeWithTexture:tex];
            }
            else
            {
                // Placeholder: 48×48 Reef Jade square
                sprite = [SKSpriteNode spriteNodeWithColor:
                    [UIColor colorWithRed:0.12 green:0.55 blue:0.49 alpha:1.0]
                    size:CGSizeMake(48.0, 48.0)];
            }

            sprite.name     = [NSString stringWithFormat:@"creature_%d", capturedId];
            sprite.position = pos;
            sprite.zPosition = 10.0;

            [gReefScene.foregroundLayer addChild:sprite];
            gReefScene.creatureNodes[@(capturedId)] = sprite;

            // Assign default idle animation
            NSDictionary* actions = [gReefScene defaultActionsForCreature];
            gReefScene.creatureActions[@(capturedId)] = actions;

            // Start in Idle state
            SKAction* idleAction = actions[@((int)CreatureState::Idle)];
            if (idleAction)
                [sprite runAction:idleAction withKey:@"behavior"];
        }
    });
}

void removeCreature(int creatureId)
{
    const int capturedId = creatureId;
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!gInitialized || !gReefScene)
            return;

        @autoreleasepool
        {
            SKSpriteNode* node = gReefScene.creatureNodes[@(capturedId)];
            if (node)
            {
                [node removeAllActions];
                [node removeFromParent];
                [gReefScene.creatureNodes removeObjectForKey:@(capturedId)];
                [gReefScene.creatureActions removeObjectForKey:@(capturedId)];
            }
        }
    });
}

void setCreatureState(int creatureId, CreatureState state)
{
    const int capturedId    = creatureId;
    const int capturedState = static_cast<int>(state);

    // Dispatch to main thread — safe to call from JUCE timer thread.
    dispatch_async(dispatch_get_main_queue(), ^{
        if (!gInitialized || !gReefScene)
            return;

        @autoreleasepool
        {
            SKSpriteNode* node = gReefScene.creatureNodes[@(capturedId)];
            if (!node)
                return;

            NSDictionary* actions = gReefScene.creatureActions[@(capturedId)];
            SKAction* action = actions ? actions[@(capturedState)] : nil;
            if (!action)
                return;

            // Only swap if actually changing (avoid restarting the same action).
            SKAction* running = [node actionForKey:@"behavior"];
            if (running == action)
                return;

            [node removeActionForKey:@"behavior"];
            [node runAction:action withKey:@"behavior"];
        }
    });
}

void scrollReef(float offsetX)
{
    if (!gInitialized)
        return;

    @autoreleasepool
    {
        dispatch_async(dispatch_get_main_queue(), ^{
            gScrollX += static_cast<CGFloat>(offsetX);
            applyScroll();
        });
    }
}

void emitParticles(ParticleType type, float x, float y)
{
    if (!gInitialized)
        return;

    const int capturedType = static_cast<int>(type);
    const CGPoint pos      = CGPointMake(static_cast<CGFloat>(x),
                                         static_cast<CGFloat>(y));

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!gInitialized || !gReefScene)
            return;

        @autoreleasepool
        {
            // Attempt to load a named .sks particle file from the bundle.
            // Expected naming convention: reef_particles_<TypeName>.sks
            // Falls back to a programmatic emitter if the asset is absent.
            NSString* particleNames[] = {
                @"reef_particles_TreasureChestShimmer",
                @"reef_particles_CreatureDiscoveryBubbles",
                @"reef_particles_DepthZoneTransition",
                @"reef_particles_PadStrikeRipple",
                @"reef_particles_BrickUnlockGlow"
            };

            if (capturedType < 0 || capturedType > 4)
                return;

            NSString* assetName = particleNames[capturedType];
            SKEmitterNode* emitter = nil;

            NSString* path = [[NSBundle mainBundle] pathForResource:assetName
                                                             ofType:@"sks"];
            if (path)
            {
                emitter = [NSKeyedUnarchiver unarchiveObjectWithFile:path];
            }

            // Fallback: simple programmatic bubble emitter for all types
            if (!emitter)
            {
                emitter = [[SKEmitterNode alloc] init];
                emitter.particleTexture    = [SKTexture textureWithImageNamed:@"spark"]; // system asset
                emitter.particleBirthRate  = 40.0;
                emitter.numParticlesToEmit = 30;
                emitter.particleLifetime   = 0.8;
                emitter.emissionAngle      = M_PI_2;           // upward
                emitter.emissionAngleRange = M_PI / 3.0;
                emitter.particleSpeed      = 80.0;
                emitter.particleSpeedRange = 40.0;
                emitter.particleAlpha      = 0.8;
                emitter.particleAlphaSpeed = -0.8;
                emitter.particleScale      = 0.3;
                emitter.particleScaleRange = 0.2;
                emitter.particleColor      = [UIColor colorWithRed:0.12
                                                             green:0.85
                                                              blue:0.78
                                                             alpha:1.0]; // Reef teal
                emitter.particleColorBlendFactor = 1.0;
            }

            emitter.position   = pos;
            emitter.zPosition  = 50.0;
            [gReefScene.particleLayer addChild:emitter];

            // Auto-remove after all particles expire
            const double removeDelay = emitter.particleLifetime
                                     + emitter.particleLifetimeRange
                                     + 0.1;
            dispatch_after(
                dispatch_time(DISPATCH_TIME_NOW,
                              (int64_t)(removeDelay * NSEC_PER_SEC)),
                dispatch_get_main_queue(), ^{
                    [emitter removeFromParent];
                }
            );
        }
    });
}

void setDepthZone(int zoneIndex)
{
    if (!gInitialized)
        return;

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!gInitialized || !gReefScene)
            return;

        @autoreleasepool
        {
            // Zone color palettes: background tint transitions
            UIColor* zoneColors[] = {
                // 0: Sunlit
                [UIColor colorWithRed:0.06 green:0.35 blue:0.55 alpha:1.0],
                // 1: Twilight
                [UIColor colorWithRed:0.04 green:0.18 blue:0.38 alpha:1.0],
                // 2: Midnight
                [UIColor colorWithRed:0.02 green:0.08 blue:0.22 alpha:1.0],
                // 3: Abyss
                [UIColor colorWithRed:0.01 green:0.02 blue:0.08 alpha:1.0]
            };

            const int clampedZone = (zoneIndex < 0) ? 0
                                  : (zoneIndex > 3) ? 3
                                  : zoneIndex;

            SKAction* fadeBackground = [SKAction colorizeWithColor:zoneColors[clampedZone]
                                                  colorBlendFactor:1.0
                                                          duration:1.2];
            [gReefScene runAction:fadeBackground];

            // Emit a full-screen zone transition particle wash
            emitParticles(ParticleType::DepthZoneTransition,
                          (float)(gReefScene.size.width  / 2.0),
                          (float)(gReefScene.size.height / 2.0));
        }
    });
}

bool captureReefSnapshot(const char* outputPath)
{
    if (!outputPath || !gInitialized || !gReefView)
        return false;

    __block bool success = false;
    NSString* path = [NSString stringWithUTF8String:outputPath];

    // captureReefSnapshot may be called from a background thread.
    // UIGraphicsImageRenderer requires the main thread.
    dispatch_sync(dispatch_get_main_queue(), ^{
        @autoreleasepool
        {
            UIGraphicsImageRenderer* renderer =
                [[UIGraphicsImageRenderer alloc] initWithBounds:gReefView.bounds];

            UIImage* image = [renderer imageWithActions:^(UIGraphicsImageRendererContext* ctx) {
                [gReefView drawViewHierarchyInRect:gReefView.bounds afterScreenUpdates:NO];
                (void)ctx;
            }];

            NSData* pngData = UIImagePNGRepresentation(image);
            if (pngData)
                success = [pngData writeToFile:path atomically:YES];
        }
    });

    return success;
}

bool isInitialized()
{
    return gInitialized;
}

} // namespace xolokun::reef_bridge

#endif // JUCE_IOS
