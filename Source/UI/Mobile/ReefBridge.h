// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once

//==============================================================================
// ReefBridge.h
// XOceanus — C++ interface to the SpriteKit reef scene for OBRIX Pocket.
//
// Architecture:
//   JUCE handles audio + controls (bottom ~40% of screen).
//   SpriteKit handles reef + creatures + particles (top ~60%).
//   They are SIBLING UIViews — the SKView is inserted alongside the JUCE
//   component view, not inside it. JUCE never renders into the reef region.
//
// Call sequence:
//   1. Initialize(reefViewHeight) — after the JUCE editor is visible
//   2. addCreature(...) — register creatures from CreatureDriverMap.json
//   3. start CreatureBehaviorTimer — it calls setCreatureState periodically
//   4. scrollReef / emitParticles from touch handlers as needed
//   5. shutdown() — in the JUCE editor destructor
//
// Thread safety:
//   All functions must be called from the JUCE message thread.
//   setCreatureState dispatches internally to the main thread via
//   dispatch_async so it is safe to call from timerCallback().
//
// Platform gating:
//   The full API is compiled only when JUCE_IOS is set.
//   On macOS, every function is a no-op stub so non-iOS builds compile
//   without modification and link without SpriteKit.
//
#include <cstdint>

#if JUCE_IOS

namespace xoceanus { namespace reef_bridge {

//==============================================================================
// Creature state enum matching CreatureDriverMap.json state labels.
//
enum class CreatureState : int {
    Sleeping  = 0,  // No recent param activity; creature rests/hides
    Idle      = 1,  // Low param values; gentle ambient animation
    Curious   = 2,  // Mid param values; creature investigates
    Excited   = 3,  // High param values; energetic animation
    Singing   = 4   // Note active — overrides all param-driven states
};

//==============================================================================
// initialize() — Create the SKView and anchor it above the JUCE view.
//
// Must be called after the JUCE editor component has been added to a UIWindow
// so that Desktop::getInstance() can locate the underlying UIView.
//
// reefViewHeight:  Height in points from the top of the screen allocated to
//                  the reef. Typically ~60% of the screen height. The JUCE
//                  view sits below this region (the caller is responsible for
//                  sizing the JUCE component to the remaining height).
//
void initialize(float reefViewHeight);

//==============================================================================
// shutdown() — Remove the SKView from the view hierarchy and release all
// scene resources. Safe to call even if initialize() was never called.
//
void shutdown();

//==============================================================================
// Creature management
//==============================================================================

// addCreature() — Register a creature and insert its sprite node into the
// reef scene at the given reef-space position.
//
// creatureId:      Unique integer ID from CreatureDriverMap.json.
// spriteSheetName: Base name of the sprite atlas asset (without extension).
//                  The atlas must be present in the iOS app bundle.
// reefX / reefY:   Position within the reef scene's coordinate space.
//                  (0,0) = bottom-left of the scene; reefX increases rightward.
//
void addCreature(int creatureId, const char* spriteSheetName,
                 float reefX, float reefY);

// removeCreature() — Remove the creature node from the scene and release its
// sprite resources.
//
void removeCreature(int creatureId);

//==============================================================================
// setCreatureState() — Transition a creature to a new behavioural state.
//
// Dispatches to the main thread internally — safe to call from the JUCE
// timer thread (timerCallback). Ignores the call if creatureId is not found
// or if the state is unchanged (no-op on same state for efficiency).
//
void setCreatureState(int creatureId, CreatureState state);

//==============================================================================
// scrollReef() — Offset the reef camera by offsetX scene points.
//
// Uses differential parallax rates per layer so the background moves slower
// than the foreground.  Called from the JUCE touch handler when the user
// swipes the reef region.
//
// offsetX: Positive = scroll right, negative = scroll left.
//           The implementation clamps to scene boundaries.
//
void scrollReef(float offsetX);

//==============================================================================
// Particle effects
//==============================================================================

enum class ParticleType : int {
    TreasureChestShimmer    = 0,  // Gold shimmer burst from chest
    CreatureDiscoveryBubbles = 1, // Bubble trail on first creature reveal
    DepthZoneTransition     = 2,  // Full-screen gradient wash on zone change
    PadStrikeRipple         = 3,  // Underwater ripple from pad hit
    BrickUnlockGlow         = 4   // Radial glow when a brick preset unlocks
};

// emitParticles() — Spawn a particle effect at a reef-space position.
//
// x / y: Position in reef scene coordinate space.
//         For PadStrikeRipple, pass the pad's centre projected into the reef.
//
void emitParticles(ParticleType type, float x, float y);

//==============================================================================
// Depth zones
//==============================================================================

// setDepthZone() — Transition the reef to a different depth zone.
//
// Changes parallax tinting, ambient light color, and creature visibility
// according to the zone's visual spec.
//
// zoneIndex:  0 = Sunlit (bright, high contrast)
//             1 = Twilight (desaturated, cooler tones)
//             2 = Midnight (near-monochrome, bioluminescence visible)
//             3 = Abyss (pitch black, creatures are their own light sources)
//
void setDepthZone(int zoneIndex);

//==============================================================================
// Share card capture
//==============================================================================

// captureReefSnapshot() — Render the SKView to a PNG at outputPath.
//
// Uses UIGraphicsImageRenderer at the screen's native scale factor.
// Blocking on the calling thread — call from a background thread if latency
// is a concern, but ensure the SKView is not being deallocated concurrently.
//
// Returns true on success.  outputPath must point to a writable location
// (e.g. the app's temporary directory).
//
bool captureReefSnapshot(const char* outputPath);

//==============================================================================
// Query
//==============================================================================

// Returns true after a successful initialize() and before shutdown().
bool isInitialized();

}} // namespace xoceanus::reef_bridge

#else // !JUCE_IOS  — no-op stubs for macOS builds

namespace xoceanus { namespace reef_bridge {

enum class CreatureState : int {
    Sleeping = 0, Idle = 1, Curious = 2, Excited = 3, Singing = 4
};

enum class ParticleType : int {
    TreasureChestShimmer    = 0,
    CreatureDiscoveryBubbles = 1,
    DepthZoneTransition     = 2,
    PadStrikeRipple         = 3,
    BrickUnlockGlow         = 4
};

inline void initialize([[maybe_unused]] float reefViewHeight) {}
inline void shutdown() {}
inline void addCreature([[maybe_unused]] int creatureId,
                        [[maybe_unused]] const char* spriteSheetName,
                        [[maybe_unused]] float reefX,
                        [[maybe_unused]] float reefY) {}
inline void removeCreature([[maybe_unused]] int creatureId) {}
inline void setCreatureState([[maybe_unused]] int creatureId,
                             [[maybe_unused]] CreatureState state) {}
inline void scrollReef([[maybe_unused]] float offsetX) {}
inline void emitParticles([[maybe_unused]] ParticleType type,
                          [[maybe_unused]] float x,
                          [[maybe_unused]] float y) {}
inline void setDepthZone([[maybe_unused]] int zoneIndex) {}
inline bool captureReefSnapshot([[maybe_unused]] const char* outputPath)
{ return false; }
inline bool isInitialized() { return false; }

}} // namespace xoceanus::reef_bridge

#endif // JUCE_IOS
