# XOceanus — Mobile Implementation Strategy

**Version:** 1.0
**Date:** 2026-03-13
**Status:** Active — Phase 1 In Progress
**Depends On:** `xoceanus_mobile_and_midi_spec.md`, `xo_signature_playsurface_spec.md`

---

## Executive Summary

XOceanus mobile is not a compromise — it is the instrument's natural home. Touch is superior to mouse for expressive performance. The phone becomes the instrument itself: tilt it, press it, play it with all ten fingers. This strategy defines how we get there in three phases, shipping playable software at each milestone.

---

## Design Principles

### 1. Playability Above All Else

Every design decision is filtered through one question: **does this make the instrument more playable?** Latency, touch responsiveness, gesture recognition, haptic feedback — these are not features, they are the instrument's soul. A synth that sounds incredible but plays like a web form is worthless.

### 2. Same Sound, Adapted Interface

The DSP is identical on all platforms. The same engine, the same coupling matrix, the same presets. What changes is how you reach the controls:

| Platform | Interaction Model |
|----------|------------------|
| Desktop | Mouse + keyboard. All panels visible. |
| iPad | Touch + optional keyboard. Near-desktop layout. |
| iPhone | Touch + motion sensors. Focused single-zone carousel. |

### 3. Progressive Disclosure, Not Feature Hiding

Mobile doesn't hide features — it reveals them when you're ready. Intuitive mode (4 macros) is the default. Advanced mode (full parameter access) is one swipe away. Every preset sounds right through macros alone.

### 4. Latency Budget

| Component | Budget | Strategy |
|-----------|--------|----------|
| Touch → MIDI event | < 8ms | Direct UITouch → MIDI on touch thread |
| Audio buffer | 5.8ms (256 samples @ 44.1kHz) | Default buffer, configurable down to 128 |
| Haptic trigger | < 16ms | Pre-prepared haptic patterns |
| Visual feedback | < 16ms (60fps) | Metal-backed JUCE rendering |
| **Total touch-to-sound** | **< 14ms** | Competitive with hardware instruments |

### 5. Graceful Degradation

When CPU budget tightens, degrade in this order (least audible impact first):

1. Reduce visual animation frame rate (60fps → 30fps)
2. Halve mod matrix update rate
3. Switch oversampling from 2x → 1x
4. Simplify FX (shorter reverb tails, linear interpolation)
5. Reduce voice count (oldest-note stealing with 5ms fade)
6. **Never** reduce audio sample rate or bit depth

---

## Phase 1: Touch Foundation (Current)

**Goal:** Playable instrument with touch input, adaptive layout, and haptic feedback. Ship as TestFlight beta.

### Deliverables

| Component | File | Purpose |
|-----------|------|---------|
| Touch Handler | `Source/UI/Mobile/MobileTouchHandler.h` | Multi-touch abstraction: UITouch → platform-agnostic TouchEvent |
| Sensor Manager | `Source/UI/Mobile/SensorManager.h` | Accelerometer, gyroscope input with smoothing and dead zones |
| Haptic Engine | `Source/UI/Mobile/HapticEngine.h` | Taptic feedback for musical events |
| Layout Manager | `Source/UI/Mobile/MobileLayoutManager.h` | Breakpoint detection, adaptive layout coordination |
| CPU Monitor | `Source/UI/Mobile/CPUMonitor.h` | Real-time CPU tracking and automatic quality mitigation |
| Mobile PlaySurface | `Source/UI/Mobile/MobilePlaySurface.h` | Touch-optimized PlaySurface with carousel, drawer, gesture zones |
| MIDI Learn | `Source/Core/MIDILearnManager.h` | Cross-platform MIDI mapping (shared desktop + mobile) |
| Parameter Drawer | `Source/UI/Mobile/ParameterDrawer.h` | iPhone bottom drawer for engine parameters |

### Architecture Decisions

**Touch handling is a first-class input pipeline, not a mouse emulation layer.**

The desktop PlaySurface uses JUCE's `mouseDown`/`mouseDrag`/`mouseUp`. On mobile, we do NOT convert touch to mouse events. Instead:

```
UITouch → MobileTouchHandler → TouchEvent (per-finger) → Zone dispatch
```

Each zone maintains its own active touch set. Cross-zone interference is impossible. Multi-touch is native, not simulated.

**Layout is determined by available width, not device type.**

```cpp
if (width < 430)       → iPhonePortrait
if (height < 430)      → iPhoneLandscape
if (width < 600)       → iPadCompact
if (width < 1024)      → iPadRegular
else                   → iPadFull / Desktop
```

This means AUv3 hosted in a narrow panel automatically gets iPhone layout, even on iPad. Multitasking just works.

---

## Phase 2: Expression & Intelligence

**Goal:** Sensor modulation, MPE support, intelligent CPU management, visual feedback mode.

### Deliverables

- Motion-to-modulation mapping (accelerometer → filter, gyro → Orbit Path)
- MPE input/output with per-note expression
- Visual Feedback Mode (external keyboard illuminates on-screen pads)
- CPU-aware voice management (automatic voice count scaling)
- Eco quality mode toggle
- AUv3 state save/restore

---

## Phase 3: Polish & Ecosystem

**Goal:** Controller presets, offline rendering, preset sharing, App Store launch.

### Deliverables

- Hardware controller presets (MPD, KeyStep, Launchpad, Seaboard)
- Mobile offline rendering pipeline (WAV/XPN export)
- Preset sharing via Share Sheet / AirDrop
- App Store metadata, screenshots, review guidelines compliance
- Accessibility audit (VoiceOver, Dynamic Type, Reduce Motion)

---

## Playability Innovation Targets

These are the features that will differentiate XOceanus from every mobile synth on the market:

### 1. Zero-Latency Haptic Pad Response
Pre-prepared haptic patterns fire on touch-down, not on audio callback return. The player feels the instrument respond before the sound arrives. This creates the illusion of zero latency.

### 2. Fretless Mode with True Pitch Tracking
Per-finger pitch tracking with configurable portamento. Not stepped, not quantized — continuous pitch like a real fretless instrument. Vibrato from finger wiggle, not an LFO button.

### 3. Orbit Path with Physics
The XY controller has real physics: momentum, friction, spring-back, boundary reflection. Release your finger and the modulation continues, decaying naturally. No other mobile synth does this.

### 4. Cross-Engine Modulation in Your Hands
The coupling matrix works on mobile. Two engines modulating each other, controlled by touch. Swipe X on a pad to blend between coupled engines. This is XOceanus's killer feature and it must be fully playable on a phone.

### 5. Motion as Expression
Tilt the phone to sweep a filter. Rock it to add vibrato. The phone IS the mod wheel. Opt-in, configurable, with sensible dead zones so you can play on a bus without your filter going crazy.

### 6. Contextual Haptic Language
Different musical events produce different haptic patterns. A pad strike feels different from a knob detent. Orbit Path boundary contact feels different from a preset load. The instrument develops a tactile vocabulary that the player's hands learn unconsciously.

---

## Trade-Off Matrix

| Feature | Desktop | iPad | iPhone | Rationale |
|---------|---------|------|--------|-----------|
| Max simultaneous engines | 4 | 3 (Pro) / 2 | 2 | CPU budget reality |
| All PlaySurface zones visible | Yes | Yes | No (carousel) | Screen real estate |
| Parameter access | All visible | Bottom section | Drawer (swipe up) | Progressive disclosure |
| Oversampling | Up to 4x | Up to 2x | 1x (Eco available) | CPU budget |
| Offline render | Unlimited | Full | 1 at a time, foreground | Battery + thermal |
| Motion sensors | No | Yes | Yes | Hardware availability |
| Haptic feedback | No | Limited | Full (Taptic) | Hardware availability |

**Key principle:** Audio capability differences are quantitative (fewer voices, lower oversampling), never qualitative (missing engines, missing coupling types, missing FX). The sound palette is identical. The performance interface adapts.

---

## File Organization

```
Source/UI/Mobile/
├── MobileTouchHandler.h      // Touch → TouchEvent abstraction
├── SensorManager.h           // Accelerometer, gyroscope
├── HapticEngine.h            // Taptic feedback
├── MobileLayoutManager.h     // Breakpoint detection + layout coordination
├── MobilePlaySurface.h       // Touch-optimized PlaySurface
├── ParameterDrawer.h         // iPhone parameter drawer
└── CPUMonitor.h              // Performance monitoring

Source/Core/
├── MIDILearnManager.h        // Cross-platform MIDI learn (new)
├── SynthEngine.h             // (existing)
├── EngineRegistry.h          // (existing)
├── MegaCouplingMatrix.h      // (existing)
└── PresetManager.h           // (existing)
```

---

*This strategy prioritizes playability over feature completeness. Phase 1 ships a better-playing instrument than most mobile synths ship at v3. Phases 2 and 3 add depth without breaking the foundation.*
