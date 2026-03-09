# XO Signature PlaySurface — Unified Specification

**Version:** 1.0
**Author:** XO_OX Designs
**Scope:** Brand-wide unified playing surface for all XO_OX instruments and the Mega-Tool
**Status:** Specification Complete — Ready for Implementation
**Date:** 2026-03-08

---

## 1. Vision & Design Philosophy

### 1.1 The Core Principle

> "Separate what you play from how it sounds from what happens."

The XO Signature PlaySurface is the single, unified interaction layer for every XO_OX instrument. It decouples three concerns that traditional instruments entangle:

1. **What you play** — Note Input (Zone 1) handles pitch, scale, chord, and drum voice selection.
2. **How it sounds** — Orbit Path (Zone 2) shapes timbre, effects, and coupling in real time.
3. **What happens** — Performance Pads (Zone 4) and Performance Strip (Zone 3) trigger system-level actions and gestural FX control.

### 1.2 One Surface, All Instruments

The PlaySurface presents the same spatial layout, the same interaction model, and the same visual language across every XO_OX product. An artist who learns the surface on XOblongBob can sit down at XOnset, XOverdub, or the Mega-Tool and immediately perform. The zones adapt their parameter mappings per instrument, but the gestural vocabulary is universal.

### 1.3 Progressive Disclosure

The surface satisfies three user tiers without compromise:

| Tier | Experience | Surface Usage |
|------|-----------|---------------|
| **Beginner** | Tap pads, hear notes. Scale Lock prevents wrong notes. Orbit is optional. | 30 seconds to first music |
| **Performer** | Use Orbit for real-time expression, Performance Strip for dub gestures, FIRE/XOSEND for FX control. | 5 minutes to full performance |
| **Sound Designer** | Fretless mode for microtonal exploration, custom Orbit axis mapping, per-pad engine blend in Mega-Tool context, drum mode with per-hit synthesis morphing. | Unlimited depth |

### 1.4 Sources Merged

This specification unifies interaction patterns proven across five XO_OX projects:

- **XOblongBob** PlaySurface: 4x4 pad grid, Scale Lock, Chord Mode, Velocity Heatmap, Warm Memory ghost traces
- **XOblongBob** Orbit Path design: circular XY with physics, configurable axis mapping, orbit recipes
- **XOverdub** PadEngine: FIRE, XOSEND, ECHO CUT, PANIC performance pads with keyboard shortcuts
- **XOddCouple** Performance Strip: 4-mode XY strip with spring-back, gesture trail, Dub Siren
- **XOddCouple** XO Pad: per-pad engine blend (terracotta/gold/teal gradient)
- **XObese** NotePad: fretless continuous pitch, magnetic scale snapping, glide/portamento, X-axis mod routing
- **XObese** FXPad: dual-axis assignable XY, glow rings on touch
- **XOnset** Drum Engine: 8-voice percussive synthesis with per-pad Circuit/Algorithm blend

---

## 2. Layout Overview

### 2.1 Desktop Layout (Minimum 800x300px Surface Area)

```
┌────────────────────────────────────────────────────────────────────────────────┐
│  [PAD|FRETLESS|DRUM] mode toggles     [SCALE ▼] [ROOT ▼]  [OCT-] OCT 3 [OCT+]│
├────────────────────────────────────┬──────────────────┬────────────────────────┤
│                                    │                  │                        │
│           ZONE 1                   │     ZONE 2       │      ZONE 4            │
│       NOTE INPUT                   │   ORBIT PATH     │  PERFORMANCE PADS      │
│                                    │                  │                        │
│   ┌────┬────┬────┬────┐           │     ╭──────╮     │  ┌──────────────────┐  │
│   │ 13 │ 14 │ 15 │ 16 │           │   ╭─┤      ├─╮  │  │      FIRE        │  │
│   ├────┼────┼────┼────┤           │   │ │  ·   │ │  │  │    (green)       │  │
│   │  9 │ 10 │ 11 │ 12 │           │   │ │ cursor│ │  │  ├──────────────────┤  │
│   ├────┼────┼────┼────┤  480×240  │   ╰─┤      ├─╯  │  │     XOSEND       │  │
│   │  5 │  6 │  7 │  8 │    px     │     ╰──────╯    │  │    (amber)       │  │
│   ├────┼────┼────┼────┤           │    180×180 px    │  ├──────────────────┤  │
│   │  1 │  2 │  3 │  4 │           │                  │  │    ECHO CUT      │  │
│   └────┴────┴────┴────┘           │   Free|Lock|Snap │  │    (amber)       │  │
│                                    │  [recipe select] │  ├──────────────────┤  │
│   bottom-left origin (MPC-style)   │                  │  │      PANIC       │  │
│                                    │                  │  │     (red)        │  │
│                                    │                  │  └──────────────────┘  │
├────────────────────────────────────┴──────────────────┴────────────────────────┤
│                              ZONE 3: PERFORMANCE STRIP                         │
│  [DUB SPACE] [FILTER SWEEP] [COUPLING] [DUB SIREN]  [Spring ⟲|Latch ⊙]      │
│ ┌────────────────────────────────────────────────────────────────────────────┐ │
│ │                                                                            │ │
│ │  Y-Top ─────────── · ─────────── cursor ─────────── · ─────────── Y-Top   │ │
│ │  X-Left                          (gesture trail)                   X-Right │ │
│ │  Y-Bot ─────────── · ─────────── · ─────────── · ─────────── Y-Bot       │ │
│ │                                                                            │ │
│ └────────────────────────────────────────────────────────────────────────────┘ │
│                                 full-width × 80px                              │
└────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Pixel Dimensions (Desktop — 1060px plugin width)

| Zone | Width | Height | Position |
|------|-------|--------|----------|
| Header Row | 1060px | 24px | Top |
| Zone 1: Note Input | 480px | 240px | Left, below header |
| Zone 2: Orbit Path | 200px (180 circle + 10 margin each side) | 240px | Center, below header |
| Zone 4: Performance Pads | 100px | 240px | Right, below header |
| Zone 3: Performance Strip | 1060px | 80px | Bottom, full width |
| **Total Surface** | **1060px** | **344px** | |

### 2.3 Mobile Adaptation (Portrait)

```
┌──────────────────────────────┐
│  [PAD|FRETLESS|DRUM]         │
│  [SCALE ▼] [ROOT ▼] [OCT±]  │
├──────────────────────────────┤
│                              │
│         ZONE 1               │
│       NOTE INPUT             │
│       4×4 grid               │
│       full width × 200px     │
│                              │
├──────────────┬───────────────┤
│   ZONE 2     │   ZONE 4      │
│  ORBIT PATH  │  PERF PADS    │
│  150×150 px  │  80px wide    │
│              │               │
├──────────────┴───────────────┤
│     ZONE 3: PERF STRIP       │
│     full width × 100px       │
└──────────────────────────────┘
```

Minimum touch targets: 44pt (Apple HIG compliance). On phone screens, pad grid may reduce to 3x3 (9 pads) to maintain touch target size.

---

## 3. Zone 1: Note Input — PAD Mode

### 3.1 Grid Layout

- **Grid**: 4 rows x 4 columns = 16 pads
- **Origin**: Bottom-left (pad index 1 = bottom-left, MPC-style)
- **Note mapping**: Chromatic ascending, left-to-right then bottom-to-top
- **Base note**: C3 (MIDI 48) at octave offset 0
- **Pad channel**: MIDI channel 1

```
     Col 0    Col 1    Col 2    Col 3
    ┌────────┬────────┬────────┬────────┐
R3  │ E4     │ F4     │ F#4    │ G4     │  index 13-16
    ├────────┼────────┼────────┼────────┤
R2  │ C4     │ C#4    │ D4     │ D#4    │  index 9-12
    ├────────┼────────┼────────┼────────┤
R1  │ G#3    │ A3     │ A#3    │ B3     │  index 5-8
    ├────────┼────────┼────────┼────────┤
R0  │ C3     │ C#3    │ D3     │ D#3    │  index 1-4
    └────────┴────────┴────────┴────────┘
         ↑ bottom-left = root note
```

Note calculation: `midiNote = baseNote + (octaveOffset * 12) + (row * 4) + col`

### 3.2 Scale Lock

When Scale Lock is engaged, every pad note is quantized to the nearest note in the selected scale. The algorithm searches +-1 octave from the raw chromatic note and selects the scale tone with minimum semitone distance.

**Supported Scales (9+):**

| Index | Scale | Intervals (semitones from root) | Degree Count |
|-------|-------|-------------------------------|-------------|
| 0 | Chromatic | 0,1,2,3,4,5,6,7,8,9,10,11 | 12 |
| 1 | Major | 0,2,4,5,7,9,11 | 7 |
| 2 | Minor (Natural) | 0,2,3,5,7,8,10 | 7 |
| 3 | Dorian | 0,2,3,5,7,9,10 | 7 |
| 4 | Mixolydian | 0,2,4,5,7,9,10 | 7 |
| 5 | Pentatonic Minor | 0,3,5,7,10 | 5 |
| 6 | Pentatonic Major | 0,2,4,7,9 | 5 |
| 7 | Blues | 0,3,5,6,7,10 | 6 |
| 8 | Harmonic Minor | 0,2,3,5,7,8,11 | 7 |
| 9 | Whole Tone | 0,2,4,6,8,10 | 6 |

**Root Key Selector**: C through B (12 options). Root key shifts scale intervals. Default: C.

### 3.3 Chord Mode

When Chord Mode is active, each pad triggers a diatonic triad built from the selected scale. Chord quality is determined automatically:

| Scale | Quality per Degree | Example (C root) |
|-------|-------------------|-------------------|
| Major | Maj, min, min, Maj, Maj, min, dim | C, Dm, Em, F, G, Am, Bdim |
| Minor | min, dim, Maj, min, min, Maj, Maj | Cm, Ddim, Eb, Fm, Gm, Ab, Bb |
| Dorian | min, min, Maj, Maj, min, dim, Maj | Dm, Em, F, G, Am, Bdim, C |
| Pent Minor | min, min, Maj, min, min | Cm, Ebm, F, Gm, Bbm |

**Triad construction:**

| Quality | Intervals | Semitone offsets |
|---------|-----------|-----------------|
| Major | Root, M3, P5 | +0, +4, +7 |
| Minor | Root, m3, P5 | +0, +3, +7 |
| Diminished | Root, m3, dim5 | +0, +3, +6 |

**Chord name display**: Floating label (e.g., "Cmaj", "Dm", "Bdim") appears centered above the pad grid on strike, fades over 2 seconds. Font: 13pt, amber color. Alpha decay: `-0.0165/frame` at 30fps.

**Fallback**: If Scale Lock is OFF and Chord Mode is ON, notes that don't fall on a scale degree default to major triads — a neutral, predictable result.

### 3.4 Per-Pad Expression Axes

#### 3.4.1 X-Axis: Engine Blend (Mega-Tool Context)

In the XO_OX Mega-Tool, each pad's horizontal touch position controls the blend between active engines:

| Position | Blend Value | Effect |
|----------|------------|--------|
| Left edge | 0.0 | Engine A only (e.g., EngineX — percussive) |
| Center | 0.5 | Both engines coupled |
| Right edge | 1.0 | Engine B only (e.g., EngineO — pad/lush) |

**Visual**: Per-pad horizontal gradient reflecting blend position:
- Left zone: terracotta `#C8553D`
- Center zone: gold `#F5C97A`
- Right zone: teal `#2A9D8F`

In single-engine instruments, the X-axis is assignable (default: filter cutoff modulation, matching XObese NotePad behavior).

#### 3.4.2 Y-Axis: Configurable Vertical Expression

The vertical position within each pad is an assignable expression axis, selected via a "PAD Y" dropdown in the header row:

| PAD Y Mode | Top of Pad | Bottom of Pad | Default For |
|-----------|-----------|--------------|-------------|
| **VELOCITY** (default) | Soft (0.3) | Hard (1.0) | General playing |
| **FILTER** | Bright (open cutoff) | Dark (closed cutoff) | Timbral control |
| **FX SEND** | Wet (full send) | Dry (no send) | Dub performance |
| **REPEAT** | Slow rate (~450ms) | Fast rate (~50ms) | Rhythmic effects |
| **PITCH BEND** | +2 semitones | -2 semitones | Pitch expression |

Visual feedback: A fill bar renders from the bottom of each pad proportional to the Y-axis value.

### 3.5 Velocity Heatmap

Each pad maintains a glow level (0.0 to 1.0) representing recent strike intensity.

- **Trigger**: Set to 1.0 on pad strike
- **Decay**: Multiply by 0.92 each frame at 30fps
- **Time to zero**: ~800ms (26 frames at 0.92^26 = 0.0047)
- **Color**: Warm amber `rgb(255, 180, 60)` with alpha = `glowLevel * 0.6`
- **Rendering**: Filled rounded rectangle overlay on pad surface

### 3.6 Warm Memory Ghost Traces

A ring buffer of 8 entries records recent pad strikes. Each entry stores a pad index and an age counter.

- **Ring buffer size**: 8 entries
- **Age increment**: +0.033 per frame at 30fps (1.0 per second)
- **Visible duration**: 1.5 seconds (entries with `age < 1.5` are rendered)
- **Alpha calculation**: `(1.5 - age) / 1.5 * 0.25` (max alpha 0.25, linear fade)
- **Visual**: White circle outline at pad center, radius = 38% of pad width, stroke width 1.5px
- **Purpose**: Creates visual history of playing pattern, reinforces muscle memory

### 3.7 Octave Shift

- **Range**: -3 to +3 octaves from base (C0 to C6)
- **Base octave**: 3 (C3 = MIDI 48)
- **Display**: "OCT 3" label between [-] and [+] buttons
- **Buttons**: Glass-styled, 36px wide, 16px tall

### 3.8 Scale Highlight on Keyboard Strip

When Scale Lock is active and a piano keyboard strip is present (below the pad grid in standalone mode), in-scale keys receive a warm amber tint overlay:

- **White key highlight**: `rgba(255, 180, 60, 0.22)` on lower 60% of key
- **Black key highlight**: `rgba(255, 180, 60, 0.35)` on full key height
- **Keyboard range**: C2-C6 (MIDI 36-96)
- **Key width**: 22px

---

## 4. Zone 1: Note Input — FRETLESS Mode

### 4.1 Concept

Fretless mode transforms Zone 1 into a continuous pitch ribbon — the expressive solo mode. Think theremin, fretless bass, slide guitar. The entire zone becomes a single large touchable area where the Y-axis maps to pitch and the X-axis maps to a modulation target.

### 4.2 Pitch Mapping (Y-Axis)

| Parameter | Value |
|-----------|-------|
| MIDI range | 24-96 (C1 to C7) |
| Total range | 6 octaves (72 semitones) |
| Direction | Bottom = low pitch (C1), Top = high pitch (C7) |
| Resolution | Continuous float, snapped to nearest scale note when Magnetic Snap is active |
| Glide time | 0-500ms, configurable via `p_glide_time` parameter |
| Glide filter | IIR smooth interpolation between consecutive notes |

### 4.3 Magnetic Scale Snapping

Uses the same 9+ scale definitions as PAD mode (Section 3.2). When enabled, the continuous Y-position snaps to the nearest scale note. When the Chromatic scale is selected or snap is disabled, notes round to the nearest semitone.

**Snap algorithm**: For scale modes, the pad area is divided into equal-height bands per scale note in the visible range. Each band maps to one scale degree. This guarantees every scale note is equally accessible regardless of interval spacing.

### 4.4 X-Axis Modulation Targets

The horizontal position maps to a configurable modulation target via a dropdown selector:

| Index | Target | Range | Default |
|-------|--------|-------|---------|
| 1 | **Filter Cutoff** (default) | 20 Hz - 20 kHz | 0.5 |
| 2 | Filter Resonance | 0.0 - 1.0 | 0.5 |
| 3 | Morph / Wavetable Position | 0.0 - 1.0 | 0.5 |
| 4 | Drive | 0.0 - 1.0 | 0.5 |
| 5 | Chorus Depth | 0.0 - 1.0 | 0.5 |
| 6 | Vibrato Rate/Depth | 0.0 - 1.0 | 0.5 |
| 7 | Volume | 0.0 - 1.0 | 0.5 |

In Mega-Tool context, additional targets are appended per-engine (e.g., `tape_wow`, `coupling_amount`, `blend`).

### 4.5 Hold Modes

| Mode | Behavior | Use Case |
|------|----------|----------|
| **Drag** (default) | Note sounds on mouse/touch down, releases on up. Dragging changes pitch in real time. | Solo performance, pitch bends |
| **Timed Hold** | Note sustains for a configurable duration (100ms - 5000ms) after touch release. Timer managed at 20Hz. | Layering multiple held notes |
| **Infinite Hold** | Note sustains indefinitely until the same note is tapped again (toggle behavior). | Building chords, drone notes |

**Multi-note tracking**: In Timed and Infinite modes, a `std::vector<HeldNote>` tracks all active notes. Each entry stores `{midiNote, expiryTime}`. Expired notes are cleaned up in the timer callback.

### 4.6 Fret Line Visualization

**Chromatic mode**: Horizontal lines at every semitone. Octave C notes labeled (e.g., "C1", "C2", ..., "C7"). Active note lines drawn thicker (2.0px vs 1.0px).

**Scale mode**: Equal-height bands per scale note. Alternating band fill: `0x10FFFFFF` / `0x08FFFFFF`. Active note bands highlighted with pink tint at 30% alpha. Note name labels (e.g., "D3", "F#4") at left edge, font size = `min(bandHeight * 0.6, 11.0)`.

### 4.7 Crosshair Indicators

When touched/pressed:

| Indicator | Color | Alpha | Width | Direction |
|-----------|-------|-------|-------|-----------|
| **Pitch line** | Pink `#FF69B4` (accentPink) | 0.6 | 2.0px | Horizontal, full width |
| **Mod line** | Yellow `#FFD700` (accentYellow) | 0.4 | 1.5px | Vertical, full height |
| **Touch dot** | Pink `#FF69B4` | 1.0 | 16px diameter (filled) | At intersection |

Touch dot has a glass-border outline ring (1.0px, `glassBorder` color).

### 4.8 Glide / Portamento

- **Parameter**: `p_glide_time`, range 0-500ms
- **Interpolation**: IIR (infinite impulse response) smooth filter
- **Coefficient calculation**: Based on sample rate and glide time
- **Behavior**: When dragging between notes, pitch slides smoothly at the configured rate
- **Bypass**: At glide_time = 0, pitch changes are instantaneous

---

## 5. Zone 1: Note Input — DRUM Mode

### 5.1 Pad Layout

When XOnset (or any drum engine) is active in the Mega-Tool, Zone 1 switches to Drum Mode with a labeled 4x4 grid:

```
     Col 0      Col 1      Col 2      Col 3
    ┌──────────┬──────────┬──────────┬──────────┐
R3  │   (user  │  (user   │  (user   │  (user   │  Row 4: Assignable / Pattern
    │  assign) │  assign) │  assign) │  assign) │  control
    ├──────────┼──────────┼──────────┼──────────┤
R2  │   Tom    │  Perc A  │  Perc B  │ (accent) │  Row 3: Auxiliary voices
    │   V6     │   V7     │   V8     │          │
    ├──────────┼──────────┼──────────┼──────────┤
R1  │   Clap   │  HH-C   │  HH-O   │ (accent) │  Row 2: Upper kit
    │   V5     │   V3     │   V4     │          │
    ├──────────┼──────────┼──────────┼──────────┤
R0  │   Kick   │  Snare   │  Kick   │  Snare   │  Row 1: Core kit (doubled)
    │   V1     │   V2     │   V1    │   V2     │  for finger drumming
    └──────────┴──────────┴──────────┴──────────┘
```

Bottom row doubles Kick and Snare for two-handed finger drumming (left hand: kick-snare, right hand: kick-snare). This follows Akai MPC drum pad conventions.

### 5.2 Default MIDI Mapping (GM Drum Map)

| Voice | Name | Default Note | GM Standard |
|-------|------|-------------|-------------|
| V1 | Kick | C1 (36) | Bass Drum 1 |
| V2 | Snare | D1 (38) | Acoustic Snare |
| V3 | Hat Closed | F#1 (42) | Closed Hi-Hat |
| V4 | Hat Open | A#1 (46) | Open Hi-Hat |
| V5 | Clap | D#1 (39) | Hand Clap |
| V6 | Tom | A1 (45) | Low Tom |
| V7 | Perc A | C#1 (37) | Side Stick |
| V8 | Perc B | G#1 (44) | Pedal Hi-Hat |

### 5.3 Per-Pad X-Axis: Circuit / Algorithm Blend

Each pad's horizontal touch position controls the XOnset `v{n}_blend` parameter for that voice:

| Position | Blend | Synthesis Character |
|----------|-------|-------------------|
| Left edge | 0.0 | Circuit-only (analog: bridged-T, noise burst, 6-osc metallic) |
| Center | 0.5 | Hybrid (equal-power crossfade of both paradigms) |
| Right edge | 1.0 | Algorithm-only (FM, modal resonator, K-S, phase distortion) |

**Visual**: Terracotta-to-teal gradient per pad. The gradient position shifts to highlight the current blend zone. A thin vertical indicator line tracks the touch X-position.

| Blend Region | Color |
|-------------|-------|
| Circuit dominant (0.0-0.3) | Terracotta `#C8553D` |
| Hybrid zone (0.3-0.7) | Gold `#F5C97A` |
| Algorithm dominant (0.7-1.0) | Teal `#2A9D8F` |

### 5.4 Per-Pad Y-Axis: Decay Time

Vertical touch position within each pad controls the `v{n}_decay` parameter:

| Position | Decay | Character |
|----------|-------|-----------|
| Bottom | Short (1ms - 50ms) | Tight, clipped, punchy |
| Top | Long (200ms - 8s, voice-dependent) | Open, ringing, sustaining |

### 5.5 Velocity-to-Snap Mapping

Strike velocity maps to the `v{n}_snap` parameter (transient sharpness):

| Velocity | Snap | Transient Character |
|----------|------|-------------------|
| Soft (< 0.3) | 0.0 - 0.1 | Muted, ghosted, gentle |
| Medium (0.3 - 0.7) | 0.1 - 0.5 | Natural, musical |
| Hard (> 0.7) | 0.5 - 1.0 | Sharp click, dramatic pitch sweep, aggressive |

---

## 6. Zone 2: Orbit Path

### 6.1 Core Concept

A circular XY expression zone with physics-based cursor behavior. The Orbit Path is the signature XO_OX differentiator — no other instrument uses a physics-based circle for real-time parameter modulation.

### 6.2 Geometry

| Property | Value |
|----------|-------|
| Shape | Circle |
| Diameter | 180px (desktop), 150px (mobile) |
| Center position | Center of Zone 2 bounds |
| Boundary ring | 1px stroke, `rgba(255, 255, 255, 0.1)` |
| Center crosshair | Faint lines at 50%/50%, `rgba(255, 255, 255, 0.06)`, 1px |
| Usable area | Full circle interior, cursor constrained to boundary |

### 6.3 Cursor

| Property | Value |
|----------|-------|
| Shape | Filled circle |
| Diameter | 12px |
| Color | Amber `#F5C97A` |
| Outer glow | Radial gradient, 10px spread, `rgba(245, 201, 122, 0.4)` fading to transparent |
| Default position | Center (0.5, 0.5) = no modulation |

### 6.4 Trail

| Property | Value |
|----------|-------|
| Buffer size | 60 points (ring buffer) |
| Point size | 6px at newest, 2px at oldest (linear interpolation) |
| Point color | Amber `rgba(245, 201, 122, 0.6)` at newest, fading to `rgba(245, 201, 122, 0.0)` at oldest |
| Update rate | One point added per frame at 30fps (2 seconds of history) |
| Fade model | Linear opacity decrease based on age index: `alpha = (1.0 - index/60) * 0.6` |

### 6.5 Physics Engine

#### 6.5.1 Free Mode (Default)

The cursor behaves like a ball inside a circular boundary:

```
// Per-frame physics update (30fps)
velocityX += 0.0;  // no gravity
velocityY += 0.0;

// Friction (2% per frame)
velocityX *= 0.98;
velocityY *= 0.98;

// Position update
posX += velocityX;
posY += velocityY;

// Boundary collision (circle)
float dist = sqrt(posX*posX + posY*posY);
if (dist > radius) {
    // Reflect velocity off circle normal
    float nx = posX / dist;
    float ny = posY / dist;
    float dot = velocityX * nx + velocityY * ny;
    velocityX -= 2.0 * dot * nx;
    velocityY -= 2.0 * dot * ny;
    // Push back inside
    posX = nx * radius * 0.99;
    posY = ny * radius * 0.99;
}
```

**Result**: A single drag gesture creates self-evolving modulation patterns. The sound continues to change organically after release, decaying to stillness over several seconds.

#### 6.5.2 Lock Mode

Cursor stays exactly where placed. No physics simulation after release. Parameters hold their last-set values until the next touch.

- Cursor remains visible at last position
- Double-tap to reset to center

#### 6.5.3 Snap Mode

Cursor returns to center on release using exponential ease-out:

```
// Per-frame spring-back
float springSpeed = 0.08;  // configurable
posX += (0.0 - posX) * springSpeed;
posY += (0.0 - posY) * springSpeed;
```

Center position = no modulation applied. This is traditional XY pad behavior.

### 6.6 Configurable Axis Mapping

Both X and Y axes are independently assignable to parameter targets:

| Parameter ID | Label | Low (left/bottom) | High (right/top) | Scale |
|-------------|-------|-------------------|-------------------|-------|
| `cutoff` | CUTOFF | 20 Hz (closed) | 20 kHz (open) | Logarithmic |
| `resonance` | RESONANCE | 0.0 (flat) | 1.0 (peak) | Linear |
| `fxsend` | FX SEND | 0.0 (dry) | 1.0 (wet) | Linear |
| `delaytime` | DELAY TIME | 10ms (short) | 2.0s (long) | Linear |
| `detune` | DETUNE | 0 cents (unison) | 50 cents (wide) | Linear |
| `stereo` | STEREO | 0.0 (mono) | 1.0 (wide) | Linear |
| `drive` | DRIVE | 0.0 (clean) | 1.0 (grit) | Linear |
| `reverbmix` | REVERB MIX | 0.0 (dry) | 1.0 (wash) | Linear |

In Mega-Tool context, axes can additionally map to cross-engine coupling parameters:

| Parameter ID | Label | Purpose |
|-------------|-------|---------|
| `coupling_xo` | X→O PUMP | Engine X amplitude ducks Engine O filter |
| `coupling_ox` | O→X DRIFT | Engine O amplitude modulates Engine X pitch |
| `blend` | ENGINE BLEND | Global balance between Engine A and Engine B |
| `machine` | MACHINE | XOnset: all-voice blend (analog → digital) |

Each instrument extends the parameter list with its own targets (e.g., XOverdub adds `tape_wow`, XOblongBob adds `curiosity_depth`).

### 6.7 Orbit Recipes (Axis Presets)

Pre-configured X/Y combinations for common use cases:

| Recipe | X Axis | Y Axis | Default For |
|--------|--------|--------|-------------|
| **FILTER SWEEP** | Cutoff | Resonance | XOblongBob, XOdyssey |
| **SPACE DUB** | FX Send | Delay Time | XOverdub |
| **DRIFT** | Detune | Stereo | General |
| **GRIT** | Drive | Cutoff | General |
| **WASH** | Reverb Mix | FX Send | General |
| **CUSTOM** | (user choice) | (user choice) | Auto-selected when user changes individual axis |

Selecting an individual X or Y parameter automatically switches the recipe to CUSTOM. Each instrument ships with a default recipe appropriate to its character.

### 6.8 Integration API

```cpp
struct OrbitParamDef {
    juce::String id;            // e.g., "cutoff"
    juce::String label;         // e.g., "CUTOFF"
    juce::String loLabel;       // e.g., "CLOSED"
    juce::String hiLabel;       // e.g., "OPEN"
    juce::String apvtsParamId;  // e.g., "flt_cutoff" — maps to APVTS
    float range[2];             // e.g., {20.0f, 20000.0f}
    bool exponential = false;   // true for frequency-domain params
};

struct OrbitRecipe {
    juce::String name;          // e.g., "FILTER SWEEP"
    juce::String xParamId;      // e.g., "cutoff"
    juce::String yParamId;      // e.g., "resonance"
};
```

---

## 7. Zone 3: Performance Strip

### 7.1 Layout

A full-width horizontal XY strip positioned at the bottom of the PlaySurface. Functions as a dedicated gestural performance controller.

| Property | Value |
|----------|-------|
| Width | Full parent width (1060px at desktop) |
| Height | 80px (desktop), 100px (mobile portrait) |
| Touch area | 4px inset from all edges |
| Mode button row | 24px height, above strip |

```
┌───────────────────────────────────────────────────────────────────┐
│  [DUB SPACE] [FILTER SWEEP] [COUPLING] [DUB SIREN]  [⟲ | ⊙]    │  24px mode row
├───────────────────────────────────────────────────────────────────┤
│  X-Left                                              X-Right     │
│  Label          ╳ cursor + trail                     Label       │  56px touch area
│  Y-Bottom                                            Y-Bottom    │
└───────────────────────────────────────────────────────────────────┘
```

### 7.2 Mode Specifications

#### 7.2.1 Dub Space Mode

The signature mode. Real-time control over the dub effect chain.

| Axis | Parameter | Range | Spring Target |
|------|-----------|-------|---------------|
| X | DubDelay Feedback | 0.0 - 0.95 | 0.3 |
| Y | LushReverb Mix | 0.0 - 1.0 | 0.2 |

**Secondary parameters derived from position:**
- Delay Time: `0.1s + X * 1.8s`
- Tape Wobble: `X * 100%`
- Reverb Size: `0.3 + Y * 1.7`
- Reverb Damping: `(1 - Y) * 100%`

**Musical behavior:**
- Center = moderate delay + reverb ("default dub" position)
- Bottom-left = bone dry
- Top-right = maximum space (deep echo + full reverb)
- Circular motion = hypnotic swirl (delay + reverb modulate together)

**Background gradient**: Terracotta (left) to gold (center) to teal (right). Intensity tracks cursor engagement.

#### 7.2.2 Filter Sweep Mode

Classic filter performance surface.

| Axis | Parameter | Range | Spring Target |
|------|-----------|-------|---------------|
| X | Filter Cutoff | 20 Hz - 18 kHz (logarithmic) | ~1200 Hz |
| Y | Filter Resonance | 0.0 - 1.0 | 0.3 |

**Logarithmic X mapping**: `frequency = 20 * pow(900, x)`

**Secondary**: LoFi Mix = `X * Y * 100%` (LoFi increases in high-resonance, high-cutoff corner)

**Background gradient**: Gold radial glow following cursor position.

#### 7.2.3 Coupling Mode

Direct control over XO_OX cross-engine interaction. Available only in Mega-Tool context or dual-engine instruments.

| Axis | Parameter | Range | Spring Target |
|------|-----------|-------|---------------|
| X | X→O Filter Pump | 0.0 - 1.0 | 0.3 |
| Y | O→X Pitch Drift | 0.0 - 1.0 | 0.15 |

**Secondary**: Coupling Amount = `max(X, Y)`, O Drift Range = `±(Y * 0.5) semitones`

**Musical behavior:**
- Bottom-left = engines independent
- Top-right = maximum entanglement
- Sweep right only = rhythmic ducking (dub pump)
- Sweep up only = pitch wobble / organic detuning

**Background gradient**: Terracotta/Teal split with gold convergence zone at center.

#### 7.2.4 Dub Siren Mode

The strip becomes a playable instrument. Touch fires a held note through the effect chain.

| Axis | Parameter | Range | Spring Target |
|------|-----------|-------|---------------|
| X | Delay/Echo Depth | 0.0 - 1.0 | 0.5 |
| Y | Pitch Bend | -24 to +24 semitones | 0 (center) |

**Siren voice:**
- Waveform: Sawtooth
- Base pitch: Selectable (C2, C3, C4, C5 — default C3)
- Routes through full FX chain (DubDelay + LushReverb)
- Touch down = note starts (velocity 100)
- Touch up = note stops (unless Latch is ON)

**Musical behavior:**
- Drag up/down = classic dub siren pitch sweep
- Drag left/right = echo depth modulation
- Circular motion = the classic dub siren swirl
- Quick tap = short stab with delay tail

**Background gradient**: Warm radial from cursor position, intensity tracks pitch distance from center.

### 7.3 Spring-Back

When the performer releases the strip, parameters smoothly return to their spring target values.

| Property | Value |
|----------|-------|
| Speed | 0.08 (exponential ease-out per frame) |
| Interpolation | `value += (target - value) * speed` per frame at 30fps |
| Settle time | ~1.5s to within 1% of target |
| Override | Latch mode disables spring-back |

### 7.4 Latch Toggle

When Latch is ON:
- Cursor stays at last position after release
- Parameters hold their values
- In Dub Siren mode: note sustains after touch lift
- Tap to update position (cursor jumps to new touch point)
- Double-tap to release latch (returns to spring target)

### 7.5 Gesture Trail

| Property | Value |
|----------|-------|
| Dot color | Gold `rgba(245, 201, 122, 0.5)` |
| Dot size | 4px diameter |
| Trail duration | ~1.5s fade |
| Buffer | Ring buffer, one dot per frame at 30fps (45 dots visible max) |
| Fade model | Linear age-based opacity: `alpha = (1.0 - age/1.5) * 0.5` |

### 7.6 MIDI CC Integration

| Input | Strip Axis | Behavior |
|-------|-----------|----------|
| CC 1 (Mod Wheel) | X Axis | CC value 0-127 maps to 0.0-1.0 |
| CC 74 (Brightness) | Y Axis | CC value 0-127 maps to 0.0-1.0 |
| Channel Aftertouch | Y Axis (override) | Pressure maps to Y position |

---

## 8. Zone 4: Performance Pads

### 8.1 Layout

A vertical column of 4 performance pads on the right side of the PlaySurface. These are ALWAYS visible regardless of Note Input mode.

```
┌──────────────────┐
│                  │
│      FIRE        │  60px tall
│    (green)       │
│                  │
├──────────────────┤
│                  │
│     XOSEND       │  60px tall
│    (amber)       │
│    ┃ ← meter     │
├──────────────────┤
│                  │
│    ECHO CUT      │  60px tall
│    (amber)       │
│                  │
├──────────────────┤
│                  │
│      PANIC       │  60px tall
│     (red)        │
│                  │
└──────────────────┘
   100px wide
```

### 8.2 Pad Specifications

#### 8.2.1 FIRE (Green)

| Property | Value |
|----------|-------|
| Color | Green `#4ADE80` |
| Behavior | Retrieves last MIDI note, triggers noteOn at velocity 0.8 |
| Trigger | Rising-edge detection (fires once on press, not continuously) |
| Use cases | Retrigger last note, fire siren voice, accent hit |
| Keyboard shortcut | `Z` |

#### 8.2.2 XOSEND (Amber)

| Property | Value |
|----------|-------|
| Color | Amber `#F5C97A` |
| Behavior | Gates signal through send/return FX chain (Drive → Tape Delay → Spring Reverb) |
| Trigger | Momentary (held = active, release = inactive) |
| Send Meter | 16px vertical bar, amber, right side of pad, smoothed 15%/frame |
| Send Glow | Amber wash over content area when held, target alpha 0.6 |
| Vertical Send Gesture | Y-position within pad controls FX send depth (bottom=dry, top=wet) |
| Color interpolation | Green → Amber based on Y-position within pad |
| Keyboard shortcut | `X` |

#### 8.2.3 ECHO CUT (Amber)

| Property | Value |
|----------|-------|
| Color | Amber `#F5C97A` |
| Behavior | Zeros delay feedback without clearing delay buffer |
| Trigger | Momentary (feedback returns on release) |
| Effect | Current echoes die naturally but no new repeats are generated |
| Keyboard shortcut | `C` |

#### 8.2.4 PANIC (Red)

| Property | Value |
|----------|-------|
| Color | Red `#EF4444` |
| Behavior | Clears ALL state: allNotesOff + clear FX buffers + reset Orbit to center |
| Trigger | Momentary (one-shot on press) |
| Keyboard shortcut | `V` |

### 8.3 Instrument-Specific Override

The Performance Pads follow a 2+2 pattern: FIRE and PANIC are universal; XOSEND and ECHO CUT are the defaults but can be overridden per instrument:

| Instrument | Pad 2 | Pad 3 |
|-----------|-------|-------|
| **XOverdub** (default) | XOSEND (send VCA gate) | ECHO CUT (kill delay feedback) |
| **XOblongBob** | REPEAT (note repeat) | CURIOSITY (cycle CuriosityEngine mode) |
| **XOdyssey** | REPEAT (note repeat) | CLIMAX (trigger Climax bloom) |
| **XOnset** | REPEAT (note repeat) | CHAOS (random Orbit launch) |
| **Mega-Tool** | XOSEND | ECHO CUT |

### 8.4 Visual States

| State | Background | Border | Text |
|-------|-----------|--------|------|
| **Idle** | `padColor.withAlpha(0.15)` | `padColor.withAlpha(0.5)`, 2px | `padColor` |
| **Pressed** | `padColor` (full) | `padColor` (full), 2px | Black `#000000` |

Corner radius: 6px. Font: 14pt bold.

### 8.5 Keyboard Shortcut Mapping

| Key | Pad | Action |
|-----|-----|--------|
| `Z` | FIRE | Retrigger / siren trigger |
| `X` | XOSEND | Gate send/return FX |
| `C` | ECHO CUT | Kill delay feedback |
| `V` | PANIC | Clear all state |

When using keyboard shortcuts, the pad's `setPressedState(true/false)` is called to synchronize visual feedback with key press/release events.

---

## 9. Visual Design System

### 9.1 Color Palette

| Element | Color | Hex | Alpha |
|---------|-------|-----|-------|
| Background (base) | Near-black | `#0A0A10` | 1.0 |
| Glass panel fill | White | `#FFFFFF` | 0.09 (9%) |
| Glass panel fill (active) | White | `#FFFFFF` | 0.14 (14%) |
| Specular top-edge highlight | White | `#FFFFFF` | 0.30 (30%) |
| Glass border (inactive) | White | `#FFFFFF` | 0.12 (12%) |
| Glass border (active) | White | `#FFFFFF` | 0.20 (20%) |
| Amber accent (primary) | Warm amber | `#F5C97A` | varies |
| Terracotta (Engine X) | Terracotta | `#C8553D` | varies |
| Gold (Coupled/center) | Gold | `#F5C97A` | varies |
| Teal (Engine O) | Teal | `#2A9D8F` | varies |
| FIRE pad | Green | `#4ADE80` | varies |
| XOSEND pad | Amber | `#F5C97A` | varies |
| ECHO CUT pad | Amber | `#F5C97A` | varies |
| PANIC pad | Red | `#EF4444` | varies |
| LATCH pad (alt) | Cyan | `#22D3EE` | varies |
| REPEAT pad (alt) | Green | `#4ADE80` | varies |
| Crosshair pitch line | Pink | `#FF69B4` | 0.6 |
| Crosshair mod line | Yellow | `#FFD700` | 0.4 |
| Text (primary) | White | `#FFFFFF` | 0.80 |
| Text (secondary) | White | `#FFFFFF` | 0.50 |
| Text (dim) | White | `#FFFFFF` | 0.30 |
| Heatmap glow | Warm amber | `rgb(255,180,60)` | `level * 0.6` |
| Ghost trace | White | `#FFFFFF` | `(1.5-age)/1.5 * 0.25` |

### 9.2 Engine-Adaptive Color

When multiple engines are available in the Mega-Tool, the active engine's accent color tints:
- Knob arcs and needles
- Velocity heatmap glow
- Orbit cursor and trail
- Performance Strip background gradient

| Active Engine | Accent Color |
|--------------|-------------|
| Engine X / Percussive | Terracotta `#C8553D` |
| Engine O / Pad | Teal `#2A9D8F` |
| Both / Coupled | Gold `#F5C97A` |
| Drum (XOnset) | Amber `#F5C97A` |

### 9.3 Typography

| Context | Font | Size | Weight | Case |
|---------|------|------|--------|------|
| Zone labels ("PADS", "ORBIT") | System monospace | 10pt | Regular | UPPERCASE |
| Pad note names | System monospace | 9pt | Regular | Mixed |
| Chord labels | System default | 13pt | Regular | Mixed |
| Button labels | System default | 10pt | Regular | UPPERCASE |
| Performance Pad labels | System default | 14pt | Bold | UPPERCASE |
| Scale name | System default | 11pt | Italic | Mixed |
| Header controls | System default | 10pt | Regular | UPPERCASE |

### 9.4 Animation Specifications

| Animation | Trigger | Behavior | Duration |
|-----------|---------|----------|----------|
| Pad velocity heatmap | Pad strike | Set glow to 1.0, decay 0.92x/frame | ~800ms |
| Warm Memory ghost | Pad strike | White circle, linear alpha fade | 1.5s |
| Chord name label | Chord strike | Set alpha 1.0, decay -0.0165/frame | 2.0s |
| Orbit trail | Continuous | 60-point ring buffer, age-based alpha | 2.0s |
| Perf Strip trail | Touch move | Gold dots, age-based alpha | 1.5s |
| Spring-back | Touch release | Exponential ease-out, speed 0.08 | ~1.5s |
| Pad press glow | Pad press | Radial amber glow, 20px spread, instant on | Instant |
| Pad release ghost | Pad release | 800ms fade to ghost, 400ms fade to inactive | 1.2s total |
| Send Meter | XOSEND active | Smooth 15%/frame to target level | ~200ms |
| Send Glow | XOSEND held | Amber wash, smooth to alpha 0.6 | ~200ms |

---

## 10. Parameter Routing Table

### 10.1 Zone 1 Output Parameters

| Parameter | Source | PAD Mode | FRETLESS Mode | DRUM Mode |
|-----------|--------|----------|---------------|-----------|
| **MIDI Note** | Touch position | Chromatic grid (quantized if Scale Lock) | Y-axis continuous (snapped if Magnetic Snap) | Voice trigger note (GM drum map) |
| **Velocity** | PAD Y=Velocity: Y-pos; else: fixed 0.8 | Fixed 0.8 (or touch pressure on mobile) | Touch pressure / fixed 0.8 |
| **X Expression** | Touch X within pad | Engine Blend (mega-tool) or filter mod (single engine) | Mod target (filter cutoff default) | Circuit/Algorithm Blend |
| **Y Expression** | Touch Y within pad | Configurable (velocity/filter/send/repeat/bend) | Pitch (primary) | Decay time |
| **Chord Tones** | Chord Mode toggle | Multiple note-on (triad) | N/A | N/A |
| **Glide** | Drag between notes | N/A | `p_glide_time` IIR interpolation | N/A |
| **Snap** | Strike velocity | N/A | N/A | Velocity → transient sharpness |

### 10.2 Zone 2 Output Parameters (Orbit)

| Parameter | Source | Mapping |
|-----------|--------|---------|
| **X-axis value** | Cursor X position (0.0-1.0) | Mapped to selected X parameter (default: cutoff) |
| **Y-axis value** | Cursor Y position (0.0-1.0) | Mapped to selected Y parameter (default: resonance) |
| **Distance from center** | `sqrt(x^2 + y^2) / radius` | Available as "intensity" mod source |

### 10.3 Zone 3 Output Parameters (Performance Strip)

| Parameter | Source | Dub Space | Filter Sweep | Coupling | Dub Siren |
|-----------|--------|-----------|-------------|----------|-----------|
| **X primary** | Touch X | Delay Feedback | Cutoff (log) | X→O Pump | Echo Depth |
| **Y primary** | Touch Y | Reverb Mix | Resonance | O→X Drift | Pitch Bend |
| **X secondary** | Derived | Delay Time, Wobble | — | Pump Depth | Delay Time |
| **Y secondary** | Derived | Reverb Size, Damping | — | Drift Range | Reverb Mix |
| **X*Y combined** | Derived | — | LoFi Mix | Coupling Amount | — |
| **Siren note** | Touch down/up | — | — | — | Note On/Off |

### 10.4 Zone 4 Output Parameters (Performance Pads)

| Parameter | Source | Type | Target |
|-----------|--------|------|--------|
| **FIRE trigger** | Rising edge detect | Boolean pulse | Last MIDI note re-trigger |
| **XOSEND gate** | Press/release | Boolean hold | Send VCA enable (0.0/1.0) |
| **XOSEND depth** | Y-position in pad | Float 0.0-1.0 | Send VCA level |
| **ECHO CUT** | Press/release | Boolean hold | Delay feedback = 0.0 |
| **PANIC** | Rising edge detect | Boolean pulse | allNotesOff + FX clear |

### 10.5 Mega-Tool Engine Mapping

When the PlaySurface is used in the Mega-Tool context with multiple engines active:

| Surface Output | Engine A (e.g., XOddCouple EngineX) | Engine B (e.g., XOddCouple EngineO) | Drum Engine (XOnset) |
|---------------|--------------------------------------|--------------------------------------|---------------------|
| Pad X-axis blend | `masterBalance` per-note | (inverse of A) | `v{n}_blend` per-voice |
| Pad Y-axis | Assignable (velocity default) | Assignable | Decay time |
| Orbit X | `flt_cutoff` (or recipe) | `flt_cutoff` (or recipe) | Global parameter |
| Orbit Y | `flt_resonance` (or recipe) | `flt_resonance` (or recipe) | Global parameter |
| Strip Coupling X | `couplingMatrix.X→O` | (receives) | Cross-voice coupling |
| Strip Coupling Y | (receives) | `couplingMatrix.O→X` | Cross-voice coupling |
| XOSEND | Send VCA in current engine's FX chain | | |
| FIRE | Re-trigger in active engine context | | |

---

## 11. MIDI Integration

### 11.1 Note Output

| Mode | MIDI Channel | Note Source | Velocity Source |
|------|-------------|-------------|----------------|
| PAD | Channel 1 | Grid position + octave + scale quantize | PAD Y (if VELOCITY mode) or fixed 0.8 |
| FRETLESS | Channel 1 | Y-axis position, continuous | Fixed 0.8 (or touch pressure) |
| DRUM | Channel 10 (GM) | Voice-mapped notes (see Section 5.2) | Touch pressure / fixed 0.8 |

### 11.2 Drum Mode MIDI Mapping

Default: GM Drum Map (notes 35-81). Customizable per-voice via MIDI Learn or configuration.

| Pad Position | Default Note | GM Name |
|-------------|-------------|---------|
| R0-C0 (Kick) | 36 | Bass Drum 1 |
| R0-C1 (Snare) | 38 | Acoustic Snare |
| R0-C2 (Kick dup) | 36 | Bass Drum 1 |
| R0-C3 (Snare dup) | 38 | Acoustic Snare |
| R1-C0 (Clap) | 39 | Hand Clap |
| R1-C1 (HH-C) | 42 | Closed Hi-Hat |
| R1-C2 (HH-O) | 46 | Open Hi-Hat |
| R1-C3 (Accent) | Configurable | — |
| R2-C0 (Tom) | 45 | Low Tom |
| R2-C1 (Perc A) | 37 | Side Stick |
| R2-C2 (Perc B) | 44 | Pedal Hi-Hat |
| R2-C3 (Accent) | Configurable | — |
| R3-C0..C3 | Configurable | User-assignable |

### 11.3 Orbit MIDI

- **Default**: Internal parameter routing only (no MIDI CC output)
- **CC Mappable**: User can assign Orbit X and Y to any CC number for external gear control
- **CC Input**: CC1 (Mod Wheel) and CC74 (Brightness) accepted as Orbit position inputs

### 11.4 Performance Strip MIDI

- **Default**: Internal parameter routing
- **CC Input**: CC1 maps to X-axis, CC74 maps to Y-axis
- **Dub Siren**: Generates standard MIDI note on/off on touch down/up

### 11.5 Aftertouch

- **Touch devices**: Channel aftertouch maps to Y-axis expression (override configurable)
- **MIDI input**: Polyphonic aftertouch supported per-pad in PAD mode (future: MPE)

---

## 12. Accessibility & Responsiveness

### 12.1 Touch Targets

| Element | Minimum Size | Platform |
|---------|-------------|----------|
| Pad (PAD/DRUM mode) | 44 x 44 pt | All (Apple HIG) |
| Performance Pad | 44 x 44 pt | All |
| Mode toggle button | 44 x 24 pt | All (width exceeds minimum) |
| Octave button | 36 x 16 pt (desktop only) | Desktop: mouse precision adequate |
| Scale combo box | 88 x 16 pt (desktop only) | Desktop: mouse precision adequate |

### 12.2 Input Methods

| Platform | Primary Input | Secondary | Expression |
|----------|-------------|-----------|------------|
| **Desktop (plugin)** | Mouse click + drag | QWERTY keyboard | — |
| **Desktop (standalone)** | Mouse click + drag | QWERTY keyboard | — |
| **iPad** | Multi-touch (10 points) | — | Force/area → velocity |
| **iPhone** | Multi-touch (5 points) | — | Force/area → velocity |

### 12.3 Desktop Keyboard Mapping

| Keys | Function | Notes |
|------|----------|-------|
| A-F (row 1) | Pad row 1 notes | May conflict with DAW shortcuts in plugin mode |
| Q-R (row 2) | Pad row 2 notes | — |
| 1-4 (row 3) | Pad row 3 notes | — |
| Z, X, C, V | Performance Pads | FIRE, XOSEND, ECHO CUT, PANIC |
| `[` / `]` | Octave down / up | Standard music software convention |
| Enter | LATCH toggle | Alternate pad 1 function |
| Space | Pad 2 action | REPEAT or XOSEND |
| Tab | Pad 3 action | CHAOS or instrument-specific |
| Backspace | PANIC | Alternate PANIC trigger |

### 12.4 Resize Behavior

| Resize Strategy | Behavior |
|----------------|----------|
| **Proportional scaling** | All zones scale proportionally when parent component resizes |
| **Minimum surface area** | 800 x 300 px (below this, hide keyboard strip) |
| **Maximum** | No upper limit; zones grow proportionally |
| **Aspect preservation** | Orbit Path remains circular (square bounding box) |
| **Priority order** | Zone 1 (Note Input) gets width priority; Zone 4 (Perf Pads) stays fixed width |

**Layout algorithm:**
1. Zone 4 width = fixed 100px (right edge)
2. Zone 3 height = fixed 80px (bottom edge)
3. Zone 2 width = `min(availableHeight, 200px)` (square constraint for circle)
4. Zone 1 width = `totalWidth - zone2Width - zone4Width`
5. Zone 1 and Zone 2 height = `totalHeight - headerHeight - zone3Height`

---

## 13. Animation & Timer System

### 13.1 Timer Architecture

A single 30fps timer drives all visual animations across all zones. This follows the proven XOblongBob pattern.

```cpp
void XOSignaturePlaySurface::timerCallback()
{
    bool needsRepaint = false;

    // Zone 1: Pad glow decay
    for (auto& g : padGlows) {
        if (g.level > 0.005f) {
            g.level *= 0.92f;
            needsRepaint = true;
        }
    }

    // Zone 1: Warm Memory aging
    for (auto& m : padMemory) {
        if (m.padIdx >= 0 && m.age < 1.5f) {
            m.age += 0.033f;
            needsRepaint = true;
        }
    }

    // Zone 1: Chord label fade
    if (chordLabelAlpha > 0.0f) {
        chordLabelAlpha -= 0.0165f;
        if (chordLabelAlpha < 0.0f) chordLabelAlpha = 0.0f;
        needsRepaint = true;
    }

    // Zone 2: Orbit physics
    if (orbitMode == Free && (orbitVelX != 0 || orbitVelY != 0)) {
        updateOrbitPhysics();
        needsRepaint = true;
    }

    // Zone 2: Orbit trail aging
    if (orbitTrailCount > 0) {
        needsRepaint = true;
    }

    // Zone 3: Strip spring-back
    if (stripSpringActive) {
        updateStripSpringBack();
        needsRepaint = true;
    }

    // Zone 3: Gesture trail aging
    if (stripTrailCount > 0) {
        needsRepaint = true;
    }

    // Zone 4: Send meter smoothing
    if (sendMeterActive) {
        updateSendMeter();
        needsRepaint = true;
    }

    if (needsRepaint)
        repaint();
}
```

### 13.2 Decay Rates Summary

| Element | Rate | Formula | Duration to < 1% |
|---------|------|---------|------------------|
| Pad velocity glow | 0.92x / frame | `level *= 0.92` | 26 frames (~867ms) |
| Warm Memory ghost | +0.033 age / frame | `age += 0.033` | 45 frames (1.5s) |
| Chord label alpha | -0.0165 / frame | `alpha -= 0.0165` | 60 frames (2.0s) |
| Orbit trail point | Age-based (60 slots) | `alpha = (1 - idx/60) * 0.6` | 60 frames (2.0s) |
| Strip gesture trail | Age-based | `alpha = (1 - age/1.5) * 0.5` | 45 frames (1.5s) |
| Strip spring-back | 0.08 ease-out | `val += (target - val) * 0.08` | ~45 frames (1.5s) |
| Send meter | 15% smooth / frame | `meter += (target - meter) * 0.15` | ~6 frames (200ms) |
| Send glow | To alpha 0.6 | `alpha += (0.6 - alpha) * 0.15` | ~6 frames (200ms) |
| Orbit friction (Free) | 0.98x velocity / frame | `vel *= 0.98` | ~230 frames (7.7s to < 1%) |

### 13.3 Repaint Optimization

- **Skip repaint**: If no animated element has changed by more than a threshold value (0.001 for floats, 1px for positions), skip the repaint call
- **Dirty region**: Each zone tracks its own dirty flag; only repaint zones that have changed
- **CPU budget**: At 30fps with all animations active, target < 2% CPU for rendering

---

## 14. Implementation Notes

### 14.1 Class Architecture

```cpp
// Top-level component — drop-in for any XO_OX instrument
class XOSignaturePlaySurface : public juce::Component,
                                private juce::Timer
{
public:
    XOSignaturePlaySurface(juce::MidiKeyboardState& keyboardState,
                           juce::AudioProcessorValueTreeState& apvts,
                           const XOSurfaceConfig& config);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Subcomponents
    std::unique_ptr<NoteInputZone>    noteInput;
    std::unique_ptr<OrbitPath>        orbit;
    std::unique_ptr<PerformanceStrip> strip;
    std::unique_ptr<PerformancePads>  pads;

    // Mode state
    enum NoteInputMode { PAD, FRETLESS, DRUM };
    NoteInputMode currentMode = PAD;

    // Timer
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOSignaturePlaySurface)
};
```

### 14.2 Subcomponent Responsibilities

| Class | Responsibility | Key State |
|-------|---------------|-----------|
| `NoteInputZone` | PAD/FRETLESS/DRUM mode rendering and input | Scale, octave, chord mode, pad glows, memory buffer, held notes |
| `OrbitPath` | Circular XY with physics, trail, axis mapping | Cursor position, velocity, trail buffer, mode, recipe |
| `PerformanceStrip` | Full-width XY strip with modes, spring-back, trail | Position, mode, spring target, latch state, trail buffer |
| `PerformancePads` | 4 vertical pads with keyboard shortcuts | Pressed states, parameter attachments |

### 14.3 Mode Switching

```cpp
enum class NoteInputMode { PAD, FRETLESS, DRUM };

void setNoteInputMode(NoteInputMode mode) {
    // Release all active notes before switching
    noteInput->releaseAllNotes();

    currentMode = mode;

    // Reconfigure zone 1
    noteInput->setMode(mode);

    // Update header controls visibility
    // PAD: show scale/chord/octave controls
    // FRETLESS: show scale/hold mode/glide controls
    // DRUM: show voice labels, hide scale/chord

    resized();
    repaint();
}
```

### 14.4 Integration with AudioProcessorValueTreeState

```cpp
// All surface-generated parameters bind to APVTS
// via ParameterAttachment for thread-safe, undo-compatible operation

// Example: Orbit X-axis binding
orbitXAttach = std::make_unique<juce::ParameterAttachment>(
    *apvts.getParameter(config.orbitXParamId),
    [this](float v) { orbit->setXPosition(v); repaint(); },
    nullptr);

// On orbit drag:
orbitXAttach->beginGesture();
orbitXAttach->setValueAsPartOfGesture(newXValue);
// On release:
orbitXAttach->endGesture();
```

### 14.5 Integration with MidiKeyboardState

```cpp
// Note input routes through MidiKeyboardState for:
// - Automatic note-on/note-off tracking
// - Compatibility with JUCE's built-in keyboard component
// - External MIDI input merging

// PAD mode:
keyboardState.noteOn(kPadChannel, midiNote, velocity);
keyboardState.noteOff(kPadChannel, midiNote, 0.0f);

// FRETLESS mode (via MidiMessageCollector for timestamped messages):
auto msg = juce::MidiMessage::noteOn(1, note, velocity);
msg.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001);
midiCollector->addMessageToQueue(msg);
```

### 14.6 Configuration Struct

```cpp
struct XOSurfaceConfig {
    // Grid
    int gridRows = 4;
    int gridCols = 4;

    // Available Note Input modes (instrument can disable some)
    bool enablePadMode     = true;
    bool enableFretlessMode = true;
    bool enableDrumMode    = false;  // only when drum engine present

    // Pad Y-axis modes
    std::vector<juce::String> padYModes = {
        "velocity", "filter", "fxsend", "repeat", "pitchbend"
    };

    // Orbit
    std::vector<OrbitParamDef> orbitParams;       // extends built-in 8
    std::vector<OrbitRecipe>   orbitRecipes;       // extends built-in 5
    juce::String defaultRecipe = "FILTER SWEEP";
    juce::String orbitXParamId = "flt_cutoff";
    juce::String orbitYParamId = "flt_resonance";

    // Performance Strip
    bool enableDubSpaceMode  = true;
    bool enableFilterMode    = true;
    bool enableCouplingMode  = false;  // only in dual-engine context
    bool enableDubSirenMode  = true;

    // Performance Pads (override slots 2 and 3)
    juce::String pad2Label = "XOSEND";
    juce::String pad3Label = "ECHO CUT";
    std::function<void()> pad2Action;  // nullptr = built-in XOSEND
    std::function<void()> pad3Action;  // nullptr = built-in ECHO CUT

    // Repeat engine (for instruments that use REPEAT pad)
    bool repeatSyncToHost = true;
    float defaultBpm = 120.0f;
    int defaultDivisionIndex = 1;  // 0=1/4, 1=1/8, 2=1/8T, 3=1/16, 4=1/16T, 5=1/32

    // Drum mode voice layout (8 entries)
    struct DrumVoiceDef {
        juce::String name;
        int midiNote;
        float defaultBlend;
    };
    std::vector<DrumVoiceDef> drumVoices;

    // Scale system
    int defaultScaleIndex = 1;  // Major
    int defaultRootKey = 0;     // C
};
```

### 14.7 Example Integration: XOverdub

```cpp
XOSurfaceConfig config;
config.enableDrumMode = false;
config.enableCouplingMode = false;
config.defaultRecipe = "SPACE DUB";
config.orbitXParamId = "fxsend";
config.orbitYParamId = "delay_time";
config.pad2Label = "XOSEND";
config.pad3Label = "ECHO CUT";
config.pad2Action = [this]() { padEngine.toggleSend(); };
config.pad3Action = [this]() { padEngine.toggleEchoCut(); };

// Add instrument-specific orbit parameters
config.orbitParams.push_back({
    "tape_wow", "TAPE WOW", "CLEAN", "WARPED", "delayWow",
    {0.0f, 1.0f}, false
});

auto surface = std::make_unique<XOSignaturePlaySurface>(
    keyboardState, apvts, config);
```

### 14.8 Example Integration: XOnset (Mega-Tool Drum Engine)

```cpp
XOSurfaceConfig config;
config.enableFretlessMode = false;
config.enableDrumMode = true;
config.enableCouplingMode = true;
config.defaultRecipe = "GRIT";
config.pad2Label = "REPEAT";
config.pad3Label = "CHAOS";

config.drumVoices = {
    {"Kick",    36, 0.2f},
    {"Snare",   38, 0.5f},
    {"HH-C",    42, 0.7f},
    {"HH-O",    46, 0.7f},
    {"Clap",    39, 0.4f},
    {"Tom",     45, 0.3f},
    {"Perc A",  37, 0.6f},
    {"Perc B",  44, 0.8f},
};

auto surface = std::make_unique<XOSignaturePlaySurface>(
    keyboardState, apvts, config);
```

---

## 15. Design Rationale

### 15.1 Why Not a Piano Keyboard?

- Piano layout has no transpositional invariance (chord shapes change per key)
- Piano layout wastes space on unreachable keys
- Piano layout provides no expression beyond velocity
- Piano layout requires knowledge of key signatures
- The PlaySurface eliminates wrong notes, enables multi-axis expression, and works on any screen size

### 15.2 Why a Circular Orbit Instead of a Rectangular XY Pad?

- The circular boundary creates natural bounce physics (reflection off a circle is more musical than hitting a wall — the angle of reflection creates unpredictable, organic trajectories)
- The distance-from-center metric provides a natural "intensity" reading
- Circular is visually distinctive — it becomes the instant visual signature of XO_OX instruments
- No other instrument uses a physics-based circle for expression

### 15.3 Why Physics on the Orbit?

- Creates evolving modulation from a single gesture (one drag produces minutes of movement)
- Makes the instrument feel "alive" — sound continues changing after you stop touching
- Turns a simple control into a generative modulation source
- Aligns with XO_OX brand identity: character and personality in every interaction

### 15.4 Why 4 Performance Pads?

- 4 is the maximum that works on phone screens without crowding
- 4 maps to 4 keyboard shortcuts (Z/X/C/V — easy to memorize)
- 2 universal + 2 instrument-specific gives brand consistency while allowing personality
- More than 4 dilutes focus — every pad must be essential

### 15.5 Why Three Note Input Modes?

- **PAD**: Fast, forgiving, chord-capable — for performers
- **FRETLESS**: Continuous, expressive, microtonal — for soloists and sound designers
- **DRUM**: Voice-labeled, blend-per-hit, finger-drumming optimized — for rhythm
- One mode cannot serve all three use cases without compromise

---

## Appendix A: Complete Color Reference (Copy-Paste Ready)

```cpp
// XOSignaturePlaySurface Color Constants
namespace PlaySurfaceColors {
    // Base
    static constexpr uint32_t Background      = 0xFF0A0A10;
    static constexpr uint32_t GlassFill       = 0x17FFFFFF;  // 9%
    static constexpr uint32_t GlassFillActive = 0x24FFFFFF;  // 14%
    static constexpr uint32_t GlassBorder     = 0x20FFFFFF;  // 12%
    static constexpr uint32_t SpecularEdge    = 0x4DFFFFFF;  // 30%

    // Accent
    static constexpr uint32_t Amber           = 0xFFF5C97A;
    static constexpr uint32_t Terracotta      = 0xFFC8553D;
    static constexpr uint32_t Gold            = 0xFFF5C97A;
    static constexpr uint32_t Teal            = 0xFF2A9D8F;

    // Performance Pads
    static constexpr uint32_t PadFire         = 0xFF4ADE80;
    static constexpr uint32_t PadSend         = 0xFFF5C97A;
    static constexpr uint32_t PadEchoCut      = 0xFFF5C97A;
    static constexpr uint32_t PadPanic        = 0xFFEF4444;
    static constexpr uint32_t PadLatch        = 0xFF22D3EE;
    static constexpr uint32_t PadRepeat       = 0xFF4ADE80;

    // Crosshairs
    static constexpr uint32_t CrosshairPitch  = 0x99FF69B4;  // pink, 60%
    static constexpr uint32_t CrosshairMod    = 0x66FFD700;  // yellow, 40%

    // Text
    static constexpr uint32_t TextPrimary     = 0xCCFFFFFF;  // 80%
    static constexpr uint32_t TextSecondary   = 0x80FFFFFF;  // 50%
    static constexpr uint32_t TextDim         = 0x4DFFFFFF;  // 30%

    // Heatmap
    static constexpr uint32_t HeatmapGlow     = 0xFFFFB43C;  // rgb(255,180,60)
}
```

---

## Appendix B: Glossary

| Term | Definition |
|------|-----------|
| **Engine Blend** | The per-note balance between two synthesis engines (e.g., Circuit vs Algorithm, Engine X vs Engine O) |
| **Magnetic Snap** | Scale quantization that pulls continuous pitch to the nearest scale degree |
| **Orbit Recipe** | A preset combination of X and Y axis parameter assignments for the Orbit Path |
| **Spring-back** | The behavior where parameters smoothly return to their default values after touch release |
| **Warm Memory** | Visual ghost traces showing recently-played pads, creating a history of interaction |
| **Velocity Heatmap** | Per-pad amber glow that decays after each strike, showing recent activity intensity |
| **Send Gesture** | Vertical movement within XOSEND pad controlling FX send depth |
| **Circuit Layer** | Layer X — analog circuit-modeled synthesis (bridged-T, noise burst, 6-osc metallic) |
| **Algorithm Layer** | Layer O — mathematical synthesis models (FM, modal, Karplus-Strong, phase distortion) |
| **Coupling** | Cross-engine modulation where one engine's output influences another's parameters |

---

## Changelog

### v1.0 (2026-03-08)
- Initial unified specification combining 5 XO_OX project designs
- Full Zone 1 specification: PAD, FRETLESS, DRUM modes
- Full Zone 2 specification: Orbit Path with physics, configurable axes, recipes
- Full Zone 3 specification: Performance Strip with 4 modes, spring-back, gesture trail
- Full Zone 4 specification: Performance Pads (FIRE, XOSEND, ECHO CUT, PANIC)
- Visual Design System with complete color reference
- Parameter Routing Table covering all zones and modes
- MIDI integration specification
- Accessibility and responsiveness requirements
- Animation and timer system with exact decay rates
- Implementation architecture with configuration struct and integration examples
- Design rationale documenting key decisions

---

*CONFIDENTIAL -- XO_OX Internal Design Document*
