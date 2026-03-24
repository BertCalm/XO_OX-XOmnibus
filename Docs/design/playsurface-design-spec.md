# PlaySurface Design Specification v2.0

**Date**: 2026-03-23
**Status**: Design Complete — Ready for Implementation
**Participants**: The Visionary (L1-L5), UIX Design Studio (Ulf/Issea/Xavier/Lucy), Kai (MPC), Producers Guild, Synth Seance Ghosts
**Scope**: Three play surfaces + expression controllers for XOlokun (AU, Standalone, iPad)

---

## Table of Contents

1. [Existing Foundation](#1-existing-foundation)
2. [Surface 1: XOuija — Fretless Performance Surface](#2-surface-1-xouija--fretless-performance-surface)
3. [Surface 2: MPC 16-Pad Grid](#3-surface-2-mpc-16-pad-grid)
4. [Surface 3: Seaboard-Style Continuous Keyboard](#4-surface-3-seaboard-style-continuous-keyboard)
5. [Expression Controllers](#5-expression-controllers)
6. [Platform-Specific Adaptation](#6-platform-specific-adaptation)
7. [Default Layout and Surface Switching](#7-default-layout-and-surface-switching)
8. [JUCE Implementation Architecture](#8-juce-implementation-architecture)
9. [Visionary Escalation — The Full Dive](#9-visionary-escalation--the-full-dive)
10. [Producers Guild Review](#10-producers-guild-review)
11. [Synth Seance Ghost Council](#11-synth-seance-ghost-council)

---

## 1. Existing Foundation

The current PlaySurface (`Source/UI/PlaySurface/PlaySurface.h`, 1092 lines) implements a 4-zone layout:

| Zone | Component | Purpose |
|------|-----------|---------|
| Zone 1 | `NoteInputZone` | 4x4 pad grid OR fretless strip (modes: Pad/Fretless/Drum) |
| Zone 2 | `OrbitPathZone` | Circular XY expression (Free/Lock/Snap physics) |
| Zone 3 | `PerformanceStrip` | Full-width XY gestural controller (DubSpace/FilterSweep/Coupling/DubSiren) |
| Zone 4 | `PerformancePads` | FIRE, XOSEND, ECHO CUT, PANIC triggers |

Additionally, `ToucheExpression.h` (212 lines) implements an Ondes Martenot-inspired pressure/expression zone with Intensity/Timbral/Spatial/Expression modes.

**What exists**: Solid foundation with ocean-depth zone coloring, warm memory trails, orbit physics, scale quantization, and keyboard shortcuts. Built but not yet mounted in the editor.

**What this spec adds**: Three fully realized, platform-adapted play surfaces that replace the current single `NoteInputZone` with mode switching, plus a comprehensive expression controller suite.

---

## 2. Surface 1: XOuija — Fretless Performance Surface

### 2.1 Concept

A continuous 2D performance surface where finger/mouse position maps to pitch (X-axis) and expression (Y-axis). The defining feature is the **Planchette** — a translucent lens that drifts over the surface, revealing the note name, octave, frequency, and engine-specific parameter values as you explore. Named after the ouija board planchette because it feels like communing with the spirit of the sound — you don't push notes, you discover them.

The XOuija is not a traditional X/Y pad. It is a **continuous instrument** with:
- **Pitch** on X (left = low, right = high), quantizable to scales or fully fretless
- **Expression** on Y (bottom = dark/closed, top = bright/open), mappable per-engine
- **Pressure** (iPad) / **Click depth** (desktop) as a third dimension for dynamics
- **Glissando** as a first-class gesture — dragging between notes produces continuous pitch glide

### 2.2 The Planchette

The Planchette is the visual soul of XOuija. It is a translucent, softly glowing lens (approximately 80x60pt) that tracks the current touch/cursor position with slight momentum-based lag (2-3 frames at 60fps). Inside the Planchette:

```
+----------------------------------+
|  Eb4          220.0 Hz           |
|  ─────────────────────           |
|  Vel: 0.72    Expr: 0.45        |
|  ENGINE: OPERA (Aria Gold glow)  |
+----------------------------------+
```

**Planchette behaviors:**
- **Idle drift**: When no touch is active, the Planchette drifts slowly in a Lissajous pattern across the surface, like a spirit moving on its own. Speed: ~0.3 Hz cycle, amplitude: 15% of surface width. This creates life in the interface even when not being played.
- **Lock-on**: When a touch begins, the Planchette snaps to the touch point with a 150ms ease-in (cubic bezier). The drift stops. The lens becomes focused.
- **Release trail**: On touch release, the Planchette lingers for 400ms at the last position (warm memory), then slowly resumes drifting. During linger, the note name fades to 50% opacity.
- **Engine color**: The Planchette border tints to the active engine's accent color (e.g., Aria Gold `#D4AF37` for OPERA, Reef Jade `#1E8B7E` for OBRIX).
- **Scale overlay**: When a scale is active (non-chromatic), faint vertical "fret" lines appear on the surface showing quantized pitch positions. The Planchette snaps horizontally to these frets with configurable snap strength (0-100%).

### 2.3 Aquatic Expression

The surface background is the ocean depth gradient from the existing fretless implementation:
- **Bottom (low pitch)**: Midnight zone — deep violet `#150820` to `#7B2FBE`
- **Middle**: Twilight zone — deep blue `#0D2744` to `#0096C7`
- **Top (high pitch/bright expression)**: Sunlit zone — cyan `#48CAE4`

**Bioluminescent trails**: As the Planchette moves, it leaves a phosphorescent trail (8-12 points, fading over 1.5s) that glows in the engine's accent color. Fast gestures produce longer, brighter trails. Slow exploration produces gentle, dim ones. This is **kinetic empathy** — the surface responds to the energy of your gesture.

**Ripple on touch**: When a note triggers, a concentric ripple expands from the touch point (3 rings, 300ms, max radius 40pt). Ring color = engine accent at 30% opacity. This provides instant visual confirmation that a note fired without being distracting.

### 2.4 Input Mapping

| Axis | Parameter | Range | Notes |
|------|-----------|-------|-------|
| X | Pitch | C1-C7 (configurable) | Left=low, Right=high. 6 octaves default. |
| Y | Expression | 0.0-1.0 | Bottom=0 (dark), Top=1 (bright). Mappable to any parameter. Default: filter cutoff. |
| Pressure (iPad) | Velocity/Dynamics | 0.0-1.0 | 3D Touch / Force Touch. Mapped to note velocity on initial touch, then aftertouch while held. |
| Click depth (desktop) | Velocity | 0.35-1.0 | Simulated via mouse Y position within touch: lower click = softer, higher = harder. |

**Scale quantization**: Same scale library as existing code (Chromatic, Major, Minor, Dorian, Mixolydian, Pentatonic Minor/Major, Blues, Harmonic Minor). Root key selector. Snap strength: 0% = fully fretless, 100% = hard quantize, 50% = magnetic pull toward scale tones (pitch correction feel).

**Glissando**: When dragging across the X axis with a held note, pitch glides continuously. If scale quantization is active, the glide passes through scale tones with brief magnetism at each (portamento with scale awareness). Glide rate is configurable: 0ms (instant jump) to 500ms (slow slide).

### 2.5 UIX Studio Assessment

**Ulf (Layout & Logic):**
The XOuija demands generous space. On desktop, it should occupy at minimum 480x240pt (the existing Zone 1 footprint) but ideally expands to fill the full surface area when selected as the primary input. The Planchette provides immediate feedback without cluttering the surface — this is progressive disclosure done right. The note name inside the Planchette means the surface itself needs no permanent labels, keeping it clean.

**Recommendation [U]**: The XOuija should be the default surface on iPad, where the full screen width is available. On desktop AU, it shares space with expression controllers in a configurable split.

**Issea (Space & Restraint):**
The idle drift of the Planchette is *ma* — intentional motion in negative space. The surface is mostly empty ocean gradient, and that emptiness is the point. When you touch it, meaning arrives. When you release, meaning dissolves back into possibility. The bioluminescent trail is wabi-sabi: beautiful, impermanent, unrepeatable. Each gesture creates a unique trail that will never exist again.

**Recommendation [I]**: Resist the urge to add grid lines or permanent markers to the surface. The ocean gradient and the Planchette are sufficient. Scale fret lines should be extremely subtle (3-5% opacity) — suggestions, not walls.

**Xavier (Platform & Kinetics):**
- **iPad**: This is the star surface. Full-screen, multi-touch (up to 5 simultaneous Planchettes for polyphonic play). Each touch gets its own engine-colored Planchette. Touch velocity via initial contact speed (UITouch's `force` property on 3D Touch devices, `majorRadius` change rate on non-3D Touch). Haptic feedback via `UIImpactFeedbackGenerator(.light)` on scale-tone crossings.
- **Desktop AU**: Mouse hover shows a ghosted Planchette (20% opacity) previewing the note. Click-drag activates the full Planchette. Right-click for context menu (scale, root key, range).
- **Touch targets**: The entire surface is one hit target — no minimum size concern. But the Planchette text must be minimum 11pt for readability during fast gestures.

**Recommendation [X]**: On iPad, implement two-finger vertical swipe to shift octave range without lifting the playing finger. This is the instrument-feel gesture that separates a toy from a tool.

**Lucy (JUCE Implementation):**
```
class XOuijaComponent : public juce::Component, private juce::Timer
```
- **Single paint() pass**: Background gradient cached as `juce::Image` in `resized()`. Planchette, trails, and ripples painted live over the cached background. At 60fps, this is ~2ms paint budget.
- **Trail buffer**: Ring buffer of 12 `TrailPoint` structs (x, y, age, velocity). Painted as filled ellipses with alpha decay. No Path objects needed.
- **Ripple pool**: Fixed pool of 4 `RippleState` structs (center, radius, age). Reused round-robin. Each ripple is 3 `drawEllipse` calls.
- **Planchette**: Single `juce::Image` (80x60, regenerated only when engine changes). Drawn with `drawImageTransformed()` for smooth positioning.
- **Multi-touch (iPad)**: Use `juce::MouseInputSource` index to track up to 5 simultaneous touches. Each gets a Planchette instance from a fixed pool.
- **Performance budget**: <3% CPU on repaint. The background image cache is critical — gradient recomputation per frame would blow the budget.
- **Pitch calculation**: `float pitch = startNote + (x / width) * semitoneRange;` — computed in `mouseDrag`, converted to MIDI with `juce::MidiMessage::noteOn()`. Scale quantization uses the existing `quantizeToScale()` but returns a float for microtonality.

**Recommendation [L]**: Do NOT use `juce::Path` for trails. Individual `fillEllipse` calls are faster for small point counts (<16). Only switch to Path-based rendering if trail length exceeds 30 points, which it should not.

### 2.6 Kai — MPC Integration

The XOuija maps to MPC keygroup programs. When an XOlokun XPM program is loaded on MPC, the XOuija's X-axis pitch range corresponds to the keygroup's zone layout. The Planchette shows which MPC zone the current pitch falls into:

```
Planchette overlay when XPM mode active:
+----------------------------------+
|  Eb4     [Zone 3 / Layer 1]     |
|  MPC Pad: 7                      |
+----------------------------------+
```

On MPCe (MPC Live III), the XOuija maps to the XY pad on the hardware. The quad-corner system translates:
- X-axis on XOuija = Q-Link Knob 1 (mapped to pitch/morph)
- Y-axis on XOuija = Q-Link Knob 2 (mapped to expression)

**Critical**: The pitch range must align with the XPM's `<SampleStart>` to `<SampleEnd>` key range. Default C1-C7 covers the standard 73-note range. XPM programs narrower than this should visually indicate the active zone on the surface (brighter region = playable, dim = out of range).

---

## 3. Surface 2: MPC 16-Pad Grid

### 3.1 Concept

A 4x4 velocity-sensitive pad grid that is 1:1 compatible with MPC pad layouts. This is not a decorative representation — it IS the MPC pad bank rendered in the XOlokun visual language. Pad numbering, velocity curves, and note mapping match MPC conventions exactly.

### 3.2 Pad Layout

```
MPC Standard Layout (Pad Bank A):
+------+------+------+------+
| 13   | 14   | 15   | 16   |   Row 3 (top)    — Sunlit zone
+------+------+------+------+
|  9   | 10   | 11   | 12   |   Row 2           — Twilight upper
+------+------+------+------+
|  5   |  6   |  7   |  8   |   Row 1           — Twilight lower
+------+------+------+------+
|  1   |  2   |  3   |  4   |   Row 0 (bottom)  — Midnight zone
+------+------+------+------+
```

**Numbering**: MPC standard — bottom-left is Pad 1, top-right is Pad 16. NOT zero-indexed. This is critical for XPM correlation: Pad 1 = MIDI note 37 (C#2, MPC convention for kick), Pad 2 = note 36 (C2), etc. The default note map follows Akai's Program A Bank A assignment.

### 3.3 Pad Visual Design

Each pad has four visual states:

1. **Idle**: Dark background with subtle ocean-depth zone tinting (existing implementation). Pad number in zone-colored text. Engine accent color as a hairline border (1px, 12% opacity).

2. **Hover (desktop)**: Background brightens by 8%. Border opacity increases to 30%. Pad number text brightens. This is the "finger approaching" state.

3. **Hit**: Full velocity-dependent illumination. The pad fills with the engine accent color at opacity proportional to velocity (vel 0.3 = 30% fill, vel 1.0 = 100% fill). A brief flash pulse (50ms, white at 20% opacity) provides the "impact" feel. The pad number inverts to dark text on bright background at high velocities.

4. **Warm memory**: After release, the pad retains a fading ghost of the hit (existing warm memory system, 1.5s decay). This shows recent hit history — essential for beatmaking where you need to see the pattern you just played.

**Velocity heatmap mode** (toggle): All 16 pads simultaneously show their most recent velocity as a persistent color fill. This creates a visual "drum pattern" — you can see at a glance that pads 1, 5, 9, 13 are being hit hard (kick pattern) while pad 6 is light (ghost snare). The heatmap decays over 4 beats (synced to host tempo if available).

### 3.4 Velocity Curve

Four selectable velocity curves matching MPC hardware:

| Curve | Character | Formula | Use Case |
|-------|-----------|---------|----------|
| Linear | Even response | `v = input` | Neutral, predictable |
| Logarithmic | Sensitive at low vel | `v = 127 * pow(input/127, 0.6)` | Finger drumming, ghost notes |
| Exponential | Sensitive at high vel | `v = input^2` | Hard hitting, trap, aggressive |
| Fixed | Constant velocity | `v = fixedValue (default 0.8)` | Step sequencing, consistent levels |

**Velocity detection (desktop)**: Click speed determines velocity. Measured as the inverse of time from `mouseDown` to first significant movement threshold (2px). Fast click = high velocity. Slow press = low velocity. Range: 0.2 (slowest deliberate press) to 1.0 (fastest tap).

**Velocity detection (iPad)**: `UITouch.force` on 3D Touch devices. On non-3D Touch: initial contact `majorRadius` rate of change (finger landing flat = soft, fingertip jab = hard). Haptic feedback on pad hit: `UIImpactFeedbackGenerator(.medium)` scaled to velocity (low vel = `.light`, high vel = `.heavy`).

### 3.5 Pad Banks

Four banks (A, B, C, D) matching MPC convention. Bank switching via:
- Desktop: Buttons or keyboard shortcuts (1-4)
- iPad: Swipe left/right to switch banks, or tap bank indicator

Each bank can be independently configured:
- **Chromatic mode**: Pad 1 = root note, ascending by semitone/scale interval
- **Drum mode**: Custom note assignment per pad (matches XPM drum programs)
- **Choke groups**: Pads can be assigned to choke groups (1-4) where triggering one pad silences others in the group (hi-hat open/closed)

### 3.6 XPM Correlation

When an XPM drum program is loaded:
- Pad colors shift to indicate sample type: kick = blue tint, snare = red tint, hat = yellow tint, percussion = green tint
- Pad labels update to show the sample name (truncated to 8 chars)
- Velocity layers are shown as subtle horizontal dividers within each pad (2 layers = 1 divider at 50%, 4 layers = 3 dividers at 25/50/75%)
- Mute groups from the XPM are mapped to choke groups automatically

### 3.7 UIX Studio Assessment

**Ulf:**
The 4x4 grid is the most recognizable layout in electronic music production. The visual hierarchy must be: (1) which pad is being hit NOW, (2) which pads were hit RECENTLY, (3) pad identity/number. The velocity heatmap mode is excellent progressive disclosure — default off, but once discovered, it transforms the workflow.

**Recommendation [U]**: Pad spacing should be generous — minimum 4pt gap between pads. Cramped pads destroy the instrument feel. On desktop, the grid should be square (not stretched). Let the remaining space be used by expression controllers.

**Issea:**
The ocean depth zone coloring is the signature detail that distinguishes this from every other pad grid. It is not decoration — it maps low pads to deep water and high pads to sunlit surface, creating a synesthetic connection between pitch register and visual depth. This is tactile digitalism through color.

**Recommendation [I]**: The warm memory ghosts (white ring fade) must be the ONLY post-hit animation. No bouncing pads, no scaling effects, no gratuitous particles. The ghost ring says "I was here" and dissolves. That restraint is what makes it feel like craft instead of software.

**Xavier:**
- **iPad**: 44pt minimum per pad (16 pads in a 4x4 grid needs minimum 176pt + 12pt spacing = 188pt square). On iPad mini this is tight; on iPad Pro 12.9" there is room to breathe. Support two-finger gestures on individual pads for velocity-locked repeat (essential for finger drumming rolls).
- **Desktop AU**: Keyboard mapping: QWER (row 3), ASDF (row 2), ZXCV (row 1), 7890 (row 0). This matches the physical 4x4 layout of the keyboard. Visual indication of keyboard binding on each pad (small key letter in bottom-right corner, 30% opacity).

**Recommendation [X]**: On iPad, long-press on a pad should reveal a radial menu: assign note, set choke group, set velocity curve. This keeps the surface clean while providing depth on demand.

**Lucy:**
```
class MPCPadGrid : public juce::Component
```
- **16 child components**: Each pad is a lightweight `PadComponent` with its own `paint()` and hit testing. This isolates repaints — hitting pad 1 only repaints pad 1, not all 16.
- **Velocity animation**: Use a single shared `juce::Timer` at 30fps (existing tick system) rather than per-pad timers. The tick decrements each pad's velocity glow.
- **Velocity detection timing**: Use `juce::Time::getMillisecondCounterHiRes()` in `mouseDown` to start a timer, check delta in the first `mouseDrag` or after 50ms timeout (whichever comes first). This provides desktop velocity without perceptible delay.
- **Keyboard mapping**: Override `keyPressed()` in the parent surface, dispatch to the correct pad via lookup table. Use `keyStateChanged()` for key-up detection (note-off).
- **Image caching**: Each pad's idle state can be cached as a 4-state `juce::Image` set (idle/hover/hit/memory). Regenerate on engine change or resize. The velocity overlay is painted live (single `fillRoundedRectangle`).

**Recommendation [L]**: 16 `PadComponent` children is fine — JUCE handles this count efficiently. Do NOT try to paint all 16 pads in a single parent `paint()` to "optimize" — the dirty-rect benefit of individual components outweighs the component overhead.

### 3.8 Kai — MPC Integration (Critical Path)

This is the section that makes or breaks XPM compatibility.

**Pad-to-MIDI Note Mapping (MPC Standard Bank A):**

| Pad | MIDI Note | GM Drum | MPC Default |
|-----|-----------|---------|-------------|
| 1 | 37 | Side Stick | Kick A |
| 2 | 36 | Bass Drum | Kick B |
| 3 | 42 | Closed HH | Snare A |
| 4 | 82 | Shaker | Snare B |
| 5 | 40 | Electric Snare | Clap |
| 6 | 38 | Acoustic Snare | Hat Closed |
| 7 | 46 | Open HH | Hat Open |
| 8 | 44 | Pedal HH | Ride |
| 9 | 48 | Hi Mid Tom | Tom 1 |
| 10 | 47 | Low Mid Tom | Perc A |
| 11 | 45 | Low Tom | Perc B |
| 12 | 43 | High Floor Tom | Crash |
| 13 | 49 | Crash 1 | Fx 1 |
| 14 | 55 | Splash | Fx 2 |
| 15 | 57 | Crash 2 | Fx 3 |
| 16 | 51 | Ride 1 | Fx 4 |

**Velocity curves must match MPC exactly.** The MPC's logarithmic curve (called "Curve 2" in MPC software) uses `v = 127 * pow(input/127, 0.6)`. Our "Logarithmic" curve must produce identical output or drum programs will feel different when played on XOlokun vs. MPC hardware.

**MPCe quad-corner integration:**
On MPC Live III / MPC Key 61, each pad has four corners. XOlokun maps this to:
- **Top-left corner**: CHARACTER macro (M1)
- **Top-right corner**: MOVEMENT macro (M2)
- **Bottom-left corner**: COUPLING macro (M3)
- **Bottom-right corner**: SPACE macro (M4)

Pressing a pad corner on MPCe sends the note at full velocity PLUS a CC value for the corresponding macro. XOlokun receives both and routes accordingly. The visual pad in XOlokun shows the quad-corner zones as subtle diagonal lines when in "MPCe mode":

```
+----------+
|\ M1 / M2 |
| \   /    |
|  \ /     |
|  / \     |
| / M3\ M4 |
+----------+
```

**XPM program correlation:**
- Pad Bank A = MPC Program slots 1-16
- Pad Bank B = MPC Program slots 17-32
- The `<Layers>` element in XPM maps to velocity layers per pad
- The `<PadNote>` element in XPM maps directly to our pad-note assignment
- `<MuteGroup>` in XPM maps to our choke group system

**30-second test (Kai):**
Load an MPC XPM drum program into XOlokun. Play the 16-pad grid. Every pad should trigger the correct sample at the correct velocity response. Switch to Bank B. Pads should update. If a beatmaker can load their MPC kit and play it without reading documentation, the implementation is correct.

---

## 4. Surface 3: Seaboard-Style Continuous Keyboard

### 4.1 Concept

A continuous keyboard surface inspired by Roli Seaboard, Linnstrument, and Haken Continuum. Unlike a traditional piano keyboard, this surface supports per-note pitch bend (glide between keys), per-note slide (Y-axis expression), and per-note pressure (aftertouch). Full MPE compatibility.

This is NOT a visual piano keyboard with click targets. It is a continuous surface organized in a keyboard-like layout that supports three dimensions of expression per voice.

### 4.2 Visual Design

The keyboard spans 2 octaves visible at a time (scrollable). Keys are rendered as continuous regions rather than discrete buttons:

```
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|  |##|  |##|  |  |##|  |##|  |##|  |  |  |
|  |##|  |##|  |  |##|  |##|  |##|  |  |  |
|  |##|  |##|  |  |##|  |##|  |##|  |  |  |
|  +--+  +--+  |  +--+  +--+  +--+  |  |  |
|     |     |  |     |     |     |   |  |  |
|  C  | D   |E |  F  |  G  |  A  |B |C |  |
+-----+-----+--+-----+-----+-----+--+--+--+
```

**Key visual states:**
- **Idle**: Natural keys = warm white `#F0EDE8` (Gallery white family). Sharp keys = deep charcoal `#2A2A2A`.
- **Touch active**: Key fills with engine accent color at pressure-proportional opacity. The fill is not binary — it breathes with the pressure.
- **Pitch bend zone**: When a touch is active on a key, the horizontal edges of the key (left/right 15%) are the pitch bend zones. A subtle gradient indicates these zones: slightly warmer tone on left edge, slightly cooler on right. Dragging from center toward an edge bends the pitch smoothly.
- **Slide zone (Y-axis)**: Vertical position on each key controls the slide dimension. Bottom of key = 0 (dark/warm), top of key = 1 (bright/cold). A faint horizontal gradient indicates this mapping.
- **Glide trail**: When sliding from one key to another, a connecting trail (engine accent color, 40% opacity, 200ms fade) shows the glide path. This visualizes the portamento.

**Octave indicator**: A thin strip above the keyboard shows the current octave range with tick marks at each C. The active octaves are highlighted. Drag this strip to scroll the keyboard range.

### 4.3 MPE Implementation

The Seaboard surface generates MPE (MIDI Polyphonic Expression) messages:

| Dimension | MPE Channel Message | CC | Range |
|-----------|--------------------|----|-------|
| Pitch bend per note | Pitch Bend on per-note channel | — | +/- 48 semitones (configurable) |
| Slide (Y-axis) | CC 74 (Brightness) | 74 | 0-127 |
| Pressure | Channel Pressure | — | 0-127 |
| Strike velocity | Note On velocity | — | 1-127 |
| Lift velocity | Note Off velocity | — | 0-127 |

**MPE zone config**: Lower Zone (channels 2-8, 7 voices) is the default. Manager channel = 1. Configurable for Upper Zone or Full MPE.
> **Note**: Standard MPE allows channels 2-16 (15 voices). The 7-voice default (channels 2-8) is a CPU budget constraint, not an MPE limitation. Users with lower CPU pressure can expand to full MPE via the zone config.

**Per-note voice allocation**: Each new touch is assigned to the next available MPE channel. Voice stealing follows LRU (Least Recently Used) when all channels are occupied. The `VoiceAllocator` from `Source/DSP/VoiceAllocator.h` is used for consistency with the engine's internal allocation.

### 4.4 Pitch Bend Behavior

Three pitch bend modes:

| Mode | Behavior | Use Case |
|------|----------|----------|
| Absolute | Bend amount = horizontal distance from key center | Precise, predictable bends |
| Relative | Bend accumulates from initial touch position | Guitar-like string bending |
| Quantized | Bend snaps to semitones/scale tones with magnetic pull | Scale-locked legato |

**Bend range**: Configurable from +/-1 to +/-48 semitones. Default: +/-2 (standard pitch wheel range). When bend range exceeds +/-12, visual tick marks appear showing semitone boundaries.

**Glide between keys**: When a finger slides from one key to an adjacent key, the transition is a smooth pitch glide (not a discrete note change). This is the Seaboard behavior that makes it feel like a continuous instrument. Glide rate: 20ms default (configurable 0-200ms).

### 4.5 UIX Studio Assessment

**Ulf:**
The 2-octave visible window with scroll is the correct choice. A full 88-key keyboard crammed into a plugin window would be illegible. The octave strip scroll gives access to the full range while keeping the visible keys large enough to play expressively.

**Recommendation [U]**: The key width must be generous — minimum 28pt per natural key on desktop. This gives ~400pt for 2 octaves of natural keys, which fits the existing Zone 1 width. On iPad, 36pt minimum per key.

**Issea:**
The sharp keys as deep charcoal against warm white naturals creates the material contrast of ebony and ivory. The engine accent color filling the keys on press is the moment where the instrument responds to the musician. No other decoration is needed. The keyboard IS the visual statement.

**Recommendation [I]**: The pitch bend zones at key edges should be indicated ONLY when a touch is active on that key. At idle, the keys should be pure, clean, material surfaces. Revealing bend zones on touch is progressive disclosure at the tactile level.

**Xavier:**
- **iPad**: Multi-touch is essential — play chords with individual per-note expression. Two-finger vertical swipe on a held note to bend pitch (alternative to horizontal edge drag). Pinch on the keyboard to zoom in/out (change visible octave range from 1 to 4 octaves).
- **Desktop AU**: Mouse click = note on. Horizontal drag = pitch bend. Vertical position at click time = slide value. MIDI keyboard input overrides mouse — when an external MIDI keyboard is connected and playing, the visual keyboard shows the received notes with their MPE data rendered in real-time.
- **Key sizing**: Natural key width must scale with the component width. Below 24pt per key, switch to a reduced display (no sharp key visual separation, just a pitch strip with chromatic markers).

**Recommendation [X]**: When an external MIDI controller (especially a Seaboard or Linnstrument) is connected, the visual keyboard becomes a DISPLAY rather than an INPUT — it shows what the controller is sending with full MPE visualization. This dual mode (play/display) is critical for the "keyboard as instrument" concept.

**Lucy:**
```
class SeaboardKeyboard : public juce::Component, private juce::Timer
```
- **Key rendering**: Natural keys and sharp keys painted in a single `paint()` pass. No child components per key — the keyboard is too performance-critical for that. Hit testing uses coordinate math, not component bounds.
- **Touch tracking**: `std::array<TouchState, 8>` for up to 8 simultaneous touches. Each `TouchState` holds: sourceIndex, noteNumber, pitchBend, slide, pressure, channel, startTime.
- **MPE output**: Generate `juce::MidiMessage` objects and push to a lock-free queue read by the audio thread. Do NOT use `MidiKeyboardState` — it does not support MPE.
- **Smooth rendering**: Key press fill uses `juce::Colour::withAlpha(pressure)` — no gradients per key (too expensive for 24+ keys per frame). The pressure animation is just alpha modulation.
- **Scroll**: The octave strip uses `juce::Viewport` internally for the scroll gesture. The keyboard content component is wider than the visible area. Scroll is pixel-smooth, not octave-snapped.

**Recommendation [L]**: The BIGGEST performance trap is per-key gradient fills. Avoid them completely. Solid color fills with alpha modulation give the same visual impression at 1/10th the GPU cost. If material texture is needed, use a pre-rendered `juce::Image` texture applied once.

### 4.6 Kai — MPC Integration

The Seaboard keyboard maps to MPC keygroup programs:
- Key range matches the XPM's `<KeyRange>` element
- Velocity layers within each zone map to the pressure dimension
- When an XPM keygroup program is loaded, zone boundaries are shown as subtle vertical dividers on the keyboard (different zones = slightly different background tints)

On MPC hardware without a keyboard (MPC Live, MPC One), the pad grid is the primary input. The Seaboard surface is primarily for:
- MPC Key 61 users (full keyboard + pads)
- DAW users with external MIDI keyboards
- iPad users who want melodic expression beyond the pad grid

### 4.7 Correlation with MIDI Keyboard Input

When an external MIDI keyboard sends notes to XOlokun:
- The Seaboard surface shows each held note as a lit key
- If the external controller sends MPE data, all three expression dimensions are visualized (pitch bend shown as horizontal glow offset, slide as vertical fill gradient, pressure as color intensity)
- If the external controller sends standard MIDI (no MPE), only note-on/off and channel aftertouch are visualized
- The user can play the on-screen keyboard AND the external keyboard simultaneously — XOlokun merges both streams

---

## 5. Expression Controllers

### 5.1 Mod Wheel

**Visual**: Vertical strip (28pt wide, 160pt tall on desktop). A ridged surface texture (4 horizontal ridges, like a physical mod wheel). The fill rises from bottom to top in the engine's accent color.

**Behavior**:
- Spring-return OFF by default (stays where you leave it). Spring-return ON is a toggle (returns to bottom on release).
- Sends CC1 (Mod Wheel). Configurable CC assignment.
- Desktop: Click-drag up/down. Scroll wheel input also accepted.
- iPad: Touch drag. Haptic ticks at 0%, 25%, 50%, 75%, 100%.

**Visual feedback**: Current value displayed as small text above the wheel (e.g., "0.72"). A faint trail shows recent movement direction (ghost line fading over 500ms).

### 5.2 Pitch Bend Strip

**Visual**: Vertical strip (28pt wide, 160pt tall). Spring-return enabled by default (returns to center on release). Center line visible. Current position shown as a bright dot on the strip.

**Behavior**:
- Sends Pitch Bend messages. Range: +/-2 semitones default (configurable +/-1 to +/-24).
- Spring-return uses a critically damped spring (no oscillation): `pos += (0.5 - pos) * 0.15` per tick.
- Desktop: Click-drag. Center detent at 50% (slight resistance feel via cursor snap within 3pt zone).
- iPad: Touch drag. Haptic at center detent.

**Visual feedback**: Semitone markers shown as tick marks on the strip. The dot grows slightly when near a semitone boundary. Above/below center shown as warm (up) / cool (down) color shift.

### 5.3 XY Pad

**Visual**: Square pad (160x160pt on desktop). Configurable axis labels. Current position shown as a bright dot with trail (reuses OrbitPathZone trail system). Background shows faint quadrant grid.

**Behavior**:
- Maps to any 2 parameters (default: X = filter cutoff, Y = resonance).
- Three physics modes from existing OrbitPathZone: Free (momentum), Lock (stays put), Snap (spring-return to center).
- Desktop: Click-drag with trail. Right-click to assign parameters.
- iPad: Touch-drag. Two-finger pinch to zoom axis ranges.

**Visual feedback**: Axis labels at edges. Corner labels show parameter extremes (e.g., "Dark" at bottom-left, "Bright" at top-right). The existing orbit trail and glow system from `OrbitPathZone` is reused directly.

### 5.4 Breath/Expression Display

**Visual**: Vertical meter strip (24pt wide). NOT directly playable — it DISPLAYS incoming CC values from external controllers (breath controller CC2, expression CC11, or any assigned CC).

**Behavior**:
- Read-only display of incoming CC data.
- Shows current value as a fill bar + numeric readout.
- Smoothed display (50ms window) to prevent visual jitter from noisy controllers.
- When no data is being received, shows "---" and dims to 30% opacity.
- Can be clicked to manually set a value (override mode), with a small "M" indicator showing manual override is active.

**Visual feedback**: Color follows the CC value: low = cool blue, mid = warm amber, high = hot red. This creates an instant visual read of controller intensity.

### 5.5 Macro Strips (CHARACTER / MOVEMENT / COUPLING / SPACE)

**Visual**: Four vertical strips, each 20pt wide, grouped together (total ~96pt with 4pt spacing). Each strip labeled at top:

| Strip | Label | Color | Default Engine Mapping |
|-------|-------|-------|----------------------|
| M1 | CHAR | XO Gold `#E9C46A` | CHARACTER macro |
| M2 | MOVE | Phosphor Green `#00FF41` | MOVEMENT macro |
| M3 | COUP | Prism Violet `#BF40FF` | COUPLING macro |
| M4 | SPACE | Phosphorescent Teal `#00B4A0` | SPACE macro |

**Behavior**:
- Each strip is a vertical slider (0.0-1.0).
- No spring-return. Values persist.
- Sends to the engine's four macro parameters.
- Desktop: Click-drag. Shift+click for fine control (1/4 speed).
- iPad: Touch-drag. Long-press to type exact value.

**Visual feedback**: Fill from bottom in the strip's assigned color. Value shown at top as 2-digit number (e.g., "72"). When a macro is being modulated by coupling, a pulsing border indicates external modulation source.

### 5.6 Novel Expression Controllers

#### 5.6.1 Coupling Crossfader

A horizontal fader that crossfades between two coupled engines. When coupling is active between Engine A (slots 1+2) and Engine B (slots 3+4):
- **Left**: 100% Engine A
- **Center**: 50/50 blend
- **Right**: 100% Engine B

Visual: Horizontal strip with engine accent colors at each end, blended gradient in center. Current position shown as a vertical line with engine names on each side.

This is the coupling performance system's crossfader (from `CouplingPerformanceParams` in the existing Phase A implementation) made into a visible, draggable control.

#### 5.6.2 Tide Controller (Aquatic LFO)

A novel expression controller unique to XOlokun. A circular controller showing a simulated water surface that responds to touch:

- **Tap center**: Creates a ripple that modulates the assigned parameter in a decaying sine wave.
- **Drag**: Creates a continuous wave whose amplitude = distance from center and rate = drag speed.
- **Two-finger**: Creates interference patterns (two wave sources).
- **Tilt (iPad gyroscope)**: The water surface tilts with the device, creating directional bias.

This maps to any parameter but is designed for slow, organic modulation — the LFO-replacement for performers who want to feel their modulation rather than set it numerically.

**Visual**: Circular, 120pt diameter. Rendered as a simple height-field (16x16 grid, bilinear interpolated) with the engine accent color. Update at 30fps. The water simulation uses a 2D wave equation with damping:

```cpp
h[x][y] += velocity[x][y];
velocity[x][y] += propagation * (h[x-1][y] + h[x+1][y] + h[x][y-1] + h[x][y+1] - 4*h[x][y]);
velocity[x][y] *= damping; // 0.98
```

CPU cost: negligible (256 float operations per frame at 30fps).

#### 5.6.3 Engine Selector Dial

A rotary selector that scrolls through the 73 registered engines. Visual: circular dial showing the current engine name and accent color, with adjacent engine names fading in on each side. Turning the dial crossfades between engines using the existing 50ms hot-swap mechanism.

This is not strictly an expression controller — it is a performance tool for live engine switching. Placing it alongside the expression controllers means a performer can switch engines without leaving the performance view.

---

## 6. Platform-Specific Adaptation

### 6.1 Desktop AU / VST3 / Standalone (macOS)

**Window size**: Default 1060x680pt (existing PlaySurface width + expression controllers + header).

**Layout**:
```
+--[Header: Surface selector | Scale | Root | Octave | Strip modes]--+
|                                                                      |
|  [Active Play Surface]                    | [Expression Controllers] |
|  (XOuija / MPC Grid / Seaboard)          | Mod Wheel                |
|  480-720pt wide                           | Pitch Bend               |
|  (resizable)                              | Macro Strips (4)         |
|                                           | XY Pad                   |
|                                           | Breath/Expr Display      |
|                                           | Coupling Crossfader      |
|                                           | Tide Controller          |
|                                                                      |
+--[Performance Strip — full width]----------------------------------------+
|  DUB SPACE / FILTER SWEEP / COUPLING / DUB SIREN                       |
+-------------------------------------------------------------------------+
|  [FIRE] [XOSEND] [ECHO CUT] [PANIC] — keyboard shortcuts Z X C V     |
+-------------------------------------------------------------------------+
```

**Mouse interactions**:
- Left-click: Primary action (note trigger, slider drag)
- Right-click: Context menu (assign parameter, change mode, configure)
- Scroll wheel: Mod wheel control when hovering over mod wheel, octave shift when hovering over keyboard
- Shift+drag: Fine control on any slider/strip
- Cmd+click (macOS): Reset to default value

**Keyboard shortcuts**:
- Tab: Cycle between play surfaces
- 1/2/3: Select XOuija / MPC Grid / Seaboard directly
- Space: Toggle play surface visibility (collapse to expression-only view)
- Arrow keys: Octave up/down (left/right), scale selection (up/down)
- Q-P row: Top row of MPC pads (when MPC Grid active)
- A-L row: Second row of MPC pads
- Z-M row: Bottom row (ZXCV also doubles as performance pad triggers)

### 6.2 iPad (AUv3 / Standalone)

**Orientation**: Landscape only for the PlaySurface. Portrait mode shows a simplified expression-only view.

**Layout (landscape)**:
```
+--[Tab bar: XOuija | MPC Pads | Seaboard | Expression]--+
|                                                          |
|  [Full-screen Play Surface]                             |
|  Multi-touch enabled                                     |
|  Edge swipe: expression controllers slide in from right  |
|                                                          |
+--[Bottom bar: Macro strips (4) | Engine | Octave]-------+
```

**Touch interactions**:
- Single touch: Primary play
- Multi-touch: Polyphonic (up to 5 voices on XOuija, up to 10 pads simultaneously on MPC Grid, up to 8 notes on Seaboard)
- Edge swipe from right: Reveal expression controller panel (mod wheel, pitch bend, XY pad, breath display)
- Edge swipe from left: Reveal engine selector / preset browser
- Two-finger vertical swipe (on surface): Octave shift
- Two-finger pinch (on Seaboard): Zoom octave range
- Long press (on MPC pad): Pad configuration radial menu

**Haptics**:
- Pad hit: `UIImpactFeedbackGenerator` scaled to velocity
- Scale-tone crossing (XOuija): `UISelectionFeedbackGenerator` (light tick)
- Pitch bend center detent: `UINotificationFeedbackGenerator(.success)` (distinct bump)
- Octave boundary crossing: `UIImpactFeedbackGenerator(.heavy)` (strong thump)

**Safe areas**: All interactive elements respect `safeAreaInsets`. The bottom bar accounts for home indicator on devices without a home button. Touch targets maintain 44pt minimum.

**AUv3 constraints**: The AUv3 host provides the view container. The PlaySurface must handle resize gracefully. Use `viewDidLayoutSubviews()` to recalculate layout. Do not assume a fixed aspect ratio.

### 6.3 MPCe (MPC Live III / MPC Key 61)

XOlokun running as a plugin on MPC hardware:
- The 4x4 physical pads map directly to the MPC 16-Pad Grid surface
- The Q-Link knobs map to: Knob 1 = M1 (CHARACTER), Knob 2 = M2 (MOVEMENT), Knob 3 = M3 (COUPLING), Knob 4 = M4 (SPACE)
- The touch strip on MPC Live III maps to the Pitch Bend strip
- The MPC's XY pad (if present on MPC Key 61) maps to the XOlokun XY Pad
- The 7" touchscreen shows a simplified version of the play surface (no trails, no ripples — performance first)

---

## 7. Default Layout and Surface Switching

### 7.1 Defaults by Platform

| Platform | Default Surface | Expression Controllers Visible |
|----------|----------------|-------------------------------|
| Desktop AU/VST3 | MPC 16-Pad Grid | Mod Wheel + Pitch Bend + Macro Strips |
| Desktop Standalone | XOuija | Mod Wheel + Pitch Bend + Macro Strips + XY Pad |
| iPad Standalone | XOuija | Macro Strips (bottom bar). Others via edge swipe. |
| iPad AUv3 | MPC 16-Pad Grid | Macro Strips (bottom bar). Others via edge swipe. |
| MPCe | MPC 16-Pad Grid (hardware pads) | Q-Link knobs (hardware). Screen shows surface overlay. |

### 7.2 Surface Switching

**Tab bar** (desktop): Three tabs: `XOUIJA` | `MPC PADS` | `SEABOARD`. Active tab underlined with XO Gold. Switching is instant (no crossfade — the surfaces are preloaded, just visibility-toggled).

**Tab bar** (iPad): Same three tabs, plus `EXPRESSION` tab that shows a full-screen expression controller layout (all 6 controllers arranged in a 3x2 grid).

**State persistence**: The selected surface and all controller positions are saved with the preset. Loading a preset restores the last-used surface for that sound. This means:
- A drum preset activates MPC Grid on load
- A pad/synth preset activates XOuija on load
- A melodic preset activates Seaboard on load

The surface preference is stored in the `.xometa` preset JSON:
```json
{
  "playSurface": {
    "defaultSurface": "mpc",
    "xouija": { "scale": 5, "rootKey": 0, "snapStrength": 0.7 },
    "mpc": { "bank": 0, "velocityCurve": 1 },
    "seaboard": { "bendRange": 2, "visibleOctaves": 2, "startOctave": 3 }
  }
}
```

### 7.3 Split View (Desktop Only)

On desktop, when the window is wide enough (>1200pt), a split view is available:
- Left: Play surface (any of the three)
- Right: Expression controllers
- The split is adjustable via drag handle

When the window is narrow (<800pt), expression controllers collapse into a slide-out drawer (accessible via a disclosure button) and the play surface takes full width.

---

## 8. JUCE Implementation Architecture

### 8.1 Component Hierarchy

```
PlaySurfaceContainer : public juce::Component, private juce::Timer
  |
  +-- PlaySurfaceTabBar : public juce::Component
  |     (3 tabs: XOuija / MPC / Seaboard)
  |
  +-- XOuijaComponent : public juce::Component
  |     +-- PlanchetteOverlay (painted, not a child component)
  |     +-- TrailBuffer (ring buffer, painted in parent paint())
  |     +-- RipplePool (fixed 4-slot pool, painted in parent paint())
  |
  +-- MPCPadGrid : public juce::Component
  |     +-- PadComponent x16 (child components)
  |     +-- BankIndicator (painted in parent paint())
  |
  +-- SeaboardKeyboard : public juce::Component
  |     +-- OctaveScrollStrip (child component)
  |     +-- KeySurface (painted, not child components per key)
  |     +-- TouchStateDisplay (painted overlay)
  |
  +-- ExpressionPanel : public juce::Component
  |     +-- ModWheelStrip : public juce::Component
  |     +-- PitchBendStrip : public juce::Component
  |     +-- XYPadController : public juce::Component (wraps OrbitPathZone)
  |     +-- BreathDisplay : public juce::Component
  |     +-- MacroStripGroup : public juce::Component
  |     |     +-- MacroStrip x4 (child components)
  |     +-- CouplingCrossfader : public juce::Component
  |     +-- TideController : public juce::Component
  |
  +-- PerformanceStrip (existing, reused)
  +-- PerformancePads (existing, reused)
  +-- ToucheExpression (existing, reused)
```

### 8.2 Timer Strategy

**Single shared timer** at 30fps on the `PlaySurfaceContainer`. The `timerCallback()` dispatches `tick()` to all visible children:

```cpp
void timerCallback() override
{
    if (activeTab == Tab::XOuija)    xouija.tick();
    if (activeTab == Tab::MPC)       mpcGrid.tick();
    if (activeTab == Tab::Seaboard)  seaboard.tick();
    expressionPanel.tick();
    strip.tick();
}
```

Invisible surfaces do NOT tick. This saves ~1.5ms per frame when only one surface is active.

### 8.3 MIDI Output Pipeline

All play surfaces generate `juce::MidiMessage` objects and push them into a shared `MidiMessageCollector`:

```cpp
// In each surface's note trigger:
auto msg = juce::MidiMessage::noteOn(channel, note, velocity);
midiCollector.addMessageToQueue(msg);

// In processBlock():
midiCollector.removeNextBlockOfMessages(midiBuffer, numSamples);
```

This ensures the audio thread receives MIDI from the play surface at the correct timestamp without blocking.

### 8.4 Parameter Attachment

Expression controllers attach to `AudioProcessorValueTreeState` parameters:

```cpp
// In ExpressionPanel constructor:
modWheelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    apvts, "globalModWheel", modWheelStrip.getInternalSlider());
```

The macro strips attach to the engine's 4 macro parameters (CHARACTER, MOVEMENT, COUPLING, SPACE). When the active engine changes, the attachments are rebuilt to point to the new engine's macro parameter IDs.

### 8.5 Performance Budget

| Component | Target CPU (paint) | Strategy |
|-----------|--------------------|----------|
| XOuija background | 0% (cached Image) | Regenerate in `resized()` only |
| XOuija Planchette | <0.5% | Pre-rendered Image, `drawImageAt()` |
| XOuija trails + ripples | <0.5% | fillEllipse calls, ring buffer |
| MPC pad idle | 0% | Cached per-pad Images |
| MPC pad hit animation | <0.3% per pad | Single fillRoundedRectangle |
| Seaboard keyboard | <1% | Single paint pass, no child components per key |
| Seaboard MPE visualization | <0.5% | Alpha modulation on cached key images |
| Expression controllers (all) | <1% | Simple fill geometries |
| Tide Controller water sim | <0.2% | 256 float ops per frame |
| **Total** | **<4%** | — |

### 8.6 Accessibility

All surfaces support:
- `juce::AccessibilityHandler` with descriptive text for each zone
- Keyboard-only operation via shortcuts (Section 6.1)
- VoiceOver descriptions for each pad, key, and controller
- High-contrast mode toggle (increases border weights and opacity by 2x)
- Reduced motion mode (disables trails, ripples, and idle drift)

---

## 9. Visionary Escalation — The Full Dive

### 9.1 The Seed

Three play surfaces for a synthesizer. This is where everyone lives — the sunlit zone. Every synth has a keyboard view and maybe a pad grid. What makes this different?

### 9.2 Level 1: Logical Extension — The Instrument-Within-the-Instrument

**VISIONARY**: The play surface is not a UI component. It is a *second instrument* that lives inside XOlokun. The engine is one instrument (the sound generator). The play surface is another (the performance interface). When you couple them, you get a third thing that neither could produce alone.

Think about it: a Stradivarius and a bow are two separate instruments. Neither makes sound alone. The musician's body is a third instrument. XOlokun has the engine (Strad), the play surface (bow), and the performer (body). We are designing the bow.

The XOuija Planchette is not a cursor — it is the bow. It has its own physics, its own memory (warm trails), its own personality (idle drift). The bow affects the sound as much as the strings do.

**DA**: Yeah but — every synth says their UI is "an instrument." How is this materially different from a Roli Dashboard or an NI Maschine pad view?

**VISIONARY**: Because those are control surfaces for their sound engines. They translate gesture into parameter changes. The XOuija Planchette does something different: it has *memory*. The warm trails aren't visual decoration — they are performance data. The Planchette remembers where you've been, and that history could feed back into the sound. The trail IS a modulation source. "Where has this performer traveled on the surface in the last 4 bars?" is a question no other synth's play surface can answer.

### 9.3 Level 2: Cross-Domain Collision — Cartography Meets Synthesis

**VISIONARY**: What if the play surface is a MAP?

Not a metaphorical map. A literal one. Each engine creates a different "landscape" on the XOuija surface. OPERA's landscape has resonant peaks at the formant frequencies — the surface literally shows you where the vowels live. OBRIX's landscape has the coral reef topology — parameter ridges and valleys that correspond to the brick ecology's state. OCEANDEEP's landscape IS a bathymetric chart — depth contours that correspond to the filter ceiling.

The Planchette becomes an explorer navigating a sonic landscape unique to each engine. You don't just play notes — you explore terrain. The bioluminescent trail becomes your path through the territory. The warm memory shows where you've been. The engine's accent color lights up the regions that are "acoustically interesting" (high parameter gradient = bright, flat parameter space = dim).

**DA**: This assumes every engine can generate a meaningful 2D landscape. Can ODDFELIX's neon tetra filter produce a readable topography?

**VISIONARY**: Yes — because the "landscape" is not arbitrary. It is the engine's *parameter sensitivity map*. For any (x, y) position, compute the partial derivative of the output spectrum with respect to small movements in x and y. High sensitivity = ridge. Low sensitivity = plain. This can be precomputed per-preset and cached as a 64x64 texture. Cost: ~200 floats. Computation: offline, once per preset load.

**DA**: Okay, that's genuinely novel. But it adds complexity to every preset and engine. Is the payoff visible to a casual user?

**VISIONARY**: The casual user sees a surface that is bright where the sound changes and dim where it doesn't. They naturally gravitate toward the bright regions. They are being *guided by the instrument itself* toward the most expressive parts of the sound. That is the payoff: the instrument teaches you how to play it.

### 9.4 Level 3: Paradigm Inversion — The Surface Plays You

**VISIONARY**: What if the play surface is not just an input? What if it is an *output*?

Currently: performer touches surface -> surface generates MIDI -> engine makes sound. One direction.

Inversion: engine state -> surface state -> performer response. The engine's internal activity (LFO phases, coupling energy, envelope stages) is visualized on the play surface AS the terrain. The performer responds to what they SEE. The LFO creates a moving wave on the surface. The coupling energy between two engines creates a pulsing ridge. The performer "surfs" the wave — riding the engine's modulation rather than imposing their own.

This is what OPERA's Conductor arc was trying to be. The Conductor drives the drama, and the performer responds. The PlaySurface makes that two-way conversation VISIBLE. The performer sees the Conductor's arc as a rising tide on the XOuija surface. They can play with it (surfing the wave) or against it (paddling upstream). Both create valid musical results, but the surface makes the choice legible.

**DA**: This is dangerously close to a screen saver. How do you prevent the surface from becoming visual noise that the performer ignores?

**VISIONARY**: By making it OPTIONAL and LOW-FREQUENCY. The engine state visualization is a background layer at 5-10% opacity. It is literally below the performer's conscious attention. But subconsciously, it influences where they move on the surface. Studies in embodied cognition show that subtle environmental cues (lighting, terrain gradient) influence movement decisions even when subjects don't consciously perceive them. The surface becomes a subliminal collaborator.

The key is that the performer can turn this off (static background) or up (20-30% opacity for deliberate visual performance). The default is just enough to influence without demanding attention.

### 9.5 Level 4: Temporal Extrapolation — The PlaySurface as AI Instrument (2030-2035)

**VISIONARY**: By 2030, on-device ML inference will be fast enough to run a small generative model in the play surface itself. The Planchette doesn't just track your finger — it has its own musical intention. It suggests the NEXT move. A ghosted second Planchette shows where the ML model thinks you should go next, based on your playing history, the current preset, and the musical context (tempo, key, chord progression inferred from the engine's output).

The performer can follow the suggestion (play into the ghost position) or diverge (the model adapts). Over time, the model learns the performer's style. The ghost becomes less of a teacher and more of a duet partner.

This is the instrument-as-collaborator that Vangelis described but never built. The instrument doesn't just respond to the musician — it has opinions about where the music should go.

**DA**: On-device ML in an audio plugin in 2030? That's plausible but the latency budget for a generative model giving real-time suggestions...

**VISIONARY**: It doesn't need real-time inference. It needs BEAT-AHEAD inference. The model predicts the next bar's trajectory, not the next sample. At 120 BPM, that's 500ms of lookahead — an eternity for inference. A small transformer (4 layers, 128 dims) could run this on an M-series neural engine in under 10ms per bar. The ghost Planchette moves ahead of the beat, showing where the model suggests you'll want to be NEXT.

### 9.6 Level 5: The Omega Point — The Surface Is the Synth

**VISIONARY**: The final form of the PlaySurface is that it IS the synthesizer. Not a controller for a synth — the surface topology itself defines the synthesis. Moving on the surface doesn't send parameters to an engine. Moving on the surface IS the sound-generating act.

Every point on the surface has a unique spectral signature. The performer doesn't "play notes" — they "inhabit frequencies." The XOuija becomes a map of the entire audible spectrum, and the Planchette is a listener choosing what to hear. Dragging across the surface IS granular synthesis. Pressing harder IS additive synthesis (more harmonics revealed). Two touches IS ring modulation (the surface between the two points vibrates sympathetically).

At this depth, the distinction between "instrument," "interface," and "sound" dissolves. The surface is all three. The performer's gesture IS the synthesis algorithm. The XOuija has become what the Ondes Martenot almost was — a purely gestural instrument where the interface IS the instrument IS the sound.

---

## 10. Producers Guild Review

### 10.1 Genre Assessment

| Genre | Primary Surface | Why | 30-Second Test |
|-------|----------------|-----|----------------|
| Boom Bap / Hip-Hop | MPC 16-Pad Grid | 4x4 pads ARE the beatmaking paradigm. Period. | Load XOffering kit, tap out a 2-bar loop. Velocity should feel like MPC hardware. |
| Trap | MPC 16-Pad Grid | Hi-hat rolls need velocity sensitivity and fast pad response | Tap pad 6 (HH-C) rapidly. 16th note rolls at 140 BPM must be clean, no dropped hits. |
| Lo-Fi / Chill | XOuija | Smooth pitch exploration, organic expression curves | Drag slowly across the XOuija with OPERA loaded. The sound should evolve continuously without discrete steps. |
| Ambient / Drone | XOuija + Tide Controller | Long, slow gestures. The Tide Controller's wave sim is ideal for ambient modulation. | Create a Tide ripple and let it decay over 8 bars while OXBOW drones. Should feel like the sound is breathing. |
| EDM / House | Seaboard Keyboard | Chord stabs with per-note expression. Pitch bend for risers. | Play a minor 7th chord, bend the top note up 2 semitones while increasing pressure on the root. Each note should respond independently. |
| Neo-Soul / R&B | Seaboard Keyboard | MPE expression for vocal-like lead lines. Slide for vibrato. | Play a melody on the Seaboard with OPERA. Y-axis slide should open the vowel formants. |
| Film Score | XOuija + Expression Panel | Continuous, sweeping gestures mapped to spatial parameters. | Full-surface gesture from bottom-left to top-right with OCEANDEEP loaded. The sound should move from deep murk to bright shimmer continuously. |
| Finger Drumming | MPC 16-Pad Grid | This is the finger drummer's entire workflow. Velocity curves MUST match MPC or they won't switch. | 8-bar drum pattern using all 16 pads. Ghost notes (vel 20-40) must be reliably triggerable. Accents (vel 100+) must feel explosive. |
| Sound Design | All three + Tide | Sound designers explore every axis. They need all the surfaces and expression tools simultaneously. | 5 minutes of free exploration across all surfaces. Nothing should feel limited, broken, or laggy. |

### 10.2 The 30-Second Test (Universal)

For each surface, there is one test that determines if the implementation is correct:

**XOuija**: Drag your finger slowly from the bottom-left corner to the top-right corner. The pitch should rise continuously (if fretless) or step through scale tones (if quantized). The expression should open the filter from dark to bright. The Planchette should follow smoothly with no jitter. The trail should glow behind you. If any of these fail, the surface is not ready.

**MPC Grid**: Tap all 16 pads in sequence from Pad 1 (bottom-left) to Pad 16 (top-right). Each should trigger a distinct note with clear velocity response. The visual hit feedback should be immediate (no perceptible delay). Load an XPM drum program — pad assignments should match the MPC exactly.

**Seaboard**: Play a C major scale ascending, then glide from C to E by sliding your finger horizontally. The pitch should glide smoothly between notes. Press harder — the sound should get brighter/louder. Move your finger vertically on a held note — the timbre should shift.

---

## 11. Synth Seance Ghost Council

### 11.1 Buchla on the XOuija (Fretless Surface)

*Don (adjusting his banana cables)*: "Finally. A surface that doesn't assume I want to play twelve notes per octave. The fretless mode with zero snap strength is how I would have built the 200e touchplate if the technology existed. The continuous pitch axis is correct. The Y-axis expression is correct. But I want the pitch response to be optionally nonlinear — exponential at the low end (more resolution in the bass register, where tuning matters), linear in the middle, compressed at the top. The Planchette idle drift is brilliant — it's the system's own life force. Every instrument should have a heartbeat even when you're not touching it."

*The Planchette earned Buchla's attention: he sees it as a direct descendant of his touchplate philosophy. The idle drift is the feature he would have invented.*

### 11.2 Kakehashi on the MPC 16-Pad Grid

*Ikutaro (bowing slightly)*: "The 4x4 grid is sacred geometry. Sixteen pads is not arbitrary — it is the square of four, which is the number of voices in most drum machines throughout history (kick, snare, hat, clap). The pad numbering must be correct. Bottom-left is Pad 1 because that is where the kick lives — at the foundation. The velocity curve matching is not optional; it is a matter of respect. Millions of producers have muscle memory built on MPC velocity response. Change it by even 5% and they will feel it in their hands and reject the instrument. I approve the four-bank system. I approve the choke groups. The warm memory ghosts are a beautiful touch — they honor the ghost note, the lightest tap that still carries musical intention. I am satisfied."

*Kakehashi gave his clearest approval. The pad grid is sacred and the velocity matching is a matter of respect to the lineage.*

### 11.3 Ciani on the Seaboard Keyboard

*Suzanne (running her fingers along an imaginary keyboard)*: "The traditional keyboard is a brilliant and terrible invention. Brilliant because it organizes pitch into a grid that the hands can learn. Terrible because it forces discrete pitch on a continuous phenomenon. The Seaboard surface heals this wound. I want to feel the pitch BETWEEN the keys — not as an error but as a destination. The slide dimension is where the soul of a note lives. When I press deeper on a held note and the timbre opens, that is the moment where the electronic instrument becomes voice-like. The glide between keys must feel like moving through water — not clicking through slots. The MPE implementation is correct, but I have one demand: the Seaboard must respond to the INTENTION of the gesture, not just the coordinates. A fast slide across keys is a glissando. A slow slide is a portamento. The surface must know the difference."

*Ciani's feedback: the Seaboard must distinguish gesture intent (fast vs. slow slide) and respond musically, not just mechanically.*

**Implementation of Ciani's demand**: Measure touch velocity (dx/dt). Above a threshold (10 keys/second), switch to glissando mode (rapid note retriggering). Below the threshold, use portamento mode (continuous pitch glide). The threshold is configurable. Default: 10 keys/second.

---

## Appendix A: Color Reference

| Element | Hex | Usage |
|---------|-----|-------|
| Gallery White | `#F8F6F3` | Light mode background |
| Surface Background | `#1A1A1A` | Dark surface background |
| Surface Card | `#2D2D2D` | Elevated surface elements |
| XO Gold | `#E9C46A` | Brand constant, macro strips, Planchette ring |
| Midnight Zone | `#7B2FBE` | Low pitch / deep expression |
| Twilight Zone | `#0096C7` | Mid pitch / mid expression |
| Sunlit Zone | `#48CAE4` | High pitch / bright expression |
| Hit Flash | `#FFFFFF` at 20% | Pad impact feedback |
| Natural Key | `#F0EDE8` | Seaboard natural keys |
| Sharp Key | `#2A2A2A` | Seaboard sharp keys |
| Trail Alpha Start | 60% | Freshest trail point |
| Trail Alpha End | 0% | Oldest trail point |

## Appendix B: MIDI Mapping Reference

| Control | MIDI Message | Default CC | Notes |
|---------|-------------|------------|-------|
| Mod Wheel | CC | 1 | Configurable |
| Pitch Bend | Pitch Bend | — | Channel-wide or per-note (MPE) |
| XY Pad X | CC | 16 | Configurable |
| XY Pad Y | CC | 17 | Configurable |
| Breath Display | CC | 2 | Read-only display by default |
| Macro 1 (CHAR) | CC | 20 | Mapped to engine macro 1 |
| Macro 2 (MOVE) | CC | 21 | Mapped to engine macro 2 |
| Macro 3 (COUP) | CC | 22 | Mapped to engine macro 3 |
| Macro 4 (SPACE) | CC | 23 | Mapped to engine macro 4 |
| Seaboard Slide | CC | 74 | MPE standard (Brightness) |
| Seaboard Pressure | Channel Pressure | — | Per-note in MPE mode |

## Appendix C: File Inventory

| File | Lines | Status |
|------|-------|--------|
| `Source/UI/PlaySurface/PlaySurface.h` | 1092 | Existing — Zone 1-4 foundation |
| `Source/UI/PlaySurface/ToucheExpression.h` | 212 | Existing — Ondes Martenot expression |
| `Source/UI/PlaySurface/XOuijaComponent.h` | — | NEW — Fretless performance surface |
| `Source/UI/PlaySurface/MPCPadGrid.h` | — | NEW — 16-pad velocity grid |
| `Source/UI/PlaySurface/SeaboardKeyboard.h` | — | NEW — Continuous MPE keyboard |
| `Source/UI/PlaySurface/ExpressionPanel.h` | — | NEW — Expression controller suite |
| `Source/UI/PlaySurface/TideController.h` | — | NEW — Aquatic LFO controller |
| `Source/UI/PlaySurface/PlaySurfaceContainer.h` | — | NEW — Tab-based container + timer |

## Appendix D: Implementation Priority

| Phase | Components | Effort | Depends On |
|-------|-----------|--------|------------|
| Phase 1 | `PlaySurfaceContainer` + tab switching + `MPCPadGrid` | 3 days | Nothing (can start now) |
| Phase 2 | `XOuijaComponent` (basic: pitch/expression, no Planchette AI) | 3 days | Phase 1 |
| Phase 3 | `SeaboardKeyboard` (basic: notes + pitch bend, no full MPE) | 3 days | Phase 1 |
| Phase 4 | `ExpressionPanel` (all 7 controllers) | 2 days | Phase 1 |
| Phase 5 | Mount into `XOlokunEditor`, wire MIDI output | 1 day | Phase 1-4 |
| Phase 6 | `TideController` water sim | 1 day | Phase 4 |
| Phase 7 | Full MPE on Seaboard + external MIDI visualization | 2 days | Phase 3 |
| Phase 8 | Planchette idle drift + engine sensitivity map | 2 days | Phase 2 |
| Phase 9 | iPad adaptations + haptics | 3 days | Phase 1-4 |
| Phase 10 | MPCe hardware mapping | 2 days | Phase 1 |
| **Total** | | **~22 days** | |

---

*Design session closed 2026-03-23. All participants signed off. Ready for implementation.*
