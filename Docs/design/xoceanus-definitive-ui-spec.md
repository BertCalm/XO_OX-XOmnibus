# XOceanus — The Definitive UI/UX Specification

**Version**: 1.0 — 2026-03-23
**Authors**: The Visionary (L1-L5+) + UIX Design Studio (Ulf, Issea, Xavier, JUCE-y Lucy)
**Status**: THE document. Everything before this is history. Everything after this is implementation.
**Target**: A JUCE developer reads this and builds the entire interface. Every pixel. Every animation. Every interaction.

> "XOceanus — for all."

---

## TABLE OF CONTENTS

1. [Part 1: THE VISION](#part-1-the-vision)
2. [Part 2: DESKTOP LAYOUT](#part-2-desktop-layout)
3. [Part 3: iOS LAYOUT](#part-3-ios-layout)
4. [Part 4: ACCESSIBILITY](#part-4-accessibility)
5. [Part 5: THE 25% + 25%](#part-5-the-25--25)
6. [Appendix A: Complete Color Reference](#appendix-a-complete-color-reference)
7. [Appendix B: Complete Font Reference](#appendix-b-complete-font-reference)
8. [Appendix C: Asset Cross-Reference](#appendix-c-asset-cross-reference)
9. [Appendix D: JUCE Implementation Guide](#appendix-d-juce-implementation-guide)
10. [Appendix E: Animation Timing Bible](#appendix-e-animation-timing-bible)

---

# PART 1: THE VISION

## 1.1 What Has Never Been Done

Every synthesizer interface ever made has treated the UI as a **control surface** — a collection of knobs, sliders, and screens that manipulate an audio engine running underneath. Vital is gorgeous. Serum is precise. Pigments is friendly. They are all dashboards for sound engines.

XOceanus is not a dashboard.

XOceanus is an **aquarium**. A living, breathing, responsive environment that you inhabit rather than operate. The 86 engines are not modules in a rack — they are creatures in a water column. The coupling connections between them are not patch cables — they are ecological relationships. The performer does not "use" XOceanus. The performer enters it.

This is the fundamental inversion that no synthesizer has attempted: **the mythology IS the interface**.

### 1.1.1 The Five Principles That Break the Ceiling

**Principle 1: The Instrument Teaches You How to Play It**

The XOuija Planchette is not a cursor. It is a divining lens. When you move over the performance surface, the surface itself communicates where the interesting sounds live. High parameter sensitivity regions glow brighter. Dead zones dim. The instrument is literally guiding the performer toward expression. No other synthesizer does this. No other instrument has EVER done this — not even acoustic ones. A violin does not glow where the harmonics are sweetest. XOceanus does.

Implementation: Per-preset parameter sensitivity maps cached as 64x64 textures. Precomputed on preset load (cost: ~200 floats, <1ms). Rendered as a background luminance layer at 8% opacity on XOuija surface.

**Principle 2: Dark Cockpit — Only the Living Speaks**

Borrowed from maritime bridge design (OpenBridge 6.0 reference): in a Dark Cockpit, all controls are dimmed to near-invisibility until they are active. The performer's attention is never split. The only bright elements are:
- The engine that is currently sounding
- The control that is currently moving
- The coupling arc that is currently modulating

Everything else exists at 15-20% opacity — present but silent. This is the opposite of every synth that shows 200 knobs at full brightness. XOceanus shows you exactly what matters RIGHT NOW.

This is not minimalism. Every control is present. This is **attentional design** — the interface respects the performer's cognitive bandwidth.

**Principle 3: The Surface Plays You Back**

The PlaySurface is bidirectional. Engine state (LFO phases, coupling energy, envelope stages) is visualized as terrain on the XOuija surface. The LFO creates a moving wave. The coupling energy creates a pulsing ridge between two engines. The performer "surfs" the wave — riding the engine's own motion rather than imposing external control. The play surface becomes a subliminal collaborator.

Default opacity: 5-8% (subconscious influence). User-adjustable to 30% (deliberate visual performance). Toggleable to 0% (static background).

**Principle 4: Sound on First Launch**

When XOceanus opens for the first time, it is already making sound. Not a sustained drone — a gentle, evolving pad preset (OXBOW "First Breath") that responds to mouse hover over ANY control. Moving the cursor over a knob produces a subtle pitch shift. Moving over the coupling strip produces a harmonic swell. The plugin is alive before the performer touches a single control.

This is the 12-year-old test: a child opens the plugin and hears something beautiful within 0.5 seconds. They move their mouse and it responds. They are hooked before they understand anything about synthesis. Every synth competitor shows a silent init patch and a wall of controls. XOceanus greets you with music.

**Principle 5: 73 Colors Are 73 Languages**

Every engine has a unique accent color. This is not decoration — it is a complete visual language system. When OPERA (Aria Gold `#D4AF37`) is active, the macro knob arcs, the coupling strip, the PlaySurface trails, the preset browser highlights, and the knob indicators ALL shift to Aria Gold. When you switch to OBRIX (Reef Jade `#1E8B7E`), the entire interface shifts. The color IS the engine's voice.

No other synthesizer uses an 88-color system as a functional communication layer. Most use 2-3 accent colors for UI states. XOceanus's color system means a performer can identify the active engine from across the room, from a blurred screenshot, from peripheral vision while looking at a piano keyboard.

### 1.1.2 The Emotional Arc

The interface has three emotional registers that the performer moves between:

| Register | State | Visual Treatment | Emotional Quality |
|----------|-------|-----------------|-------------------|
| **Gallery** | Browsing, selecting, configuring | Light mode shell (`#F8F6F3`), full control visibility, clean typography | Calm, curious, exploratory |
| **Performance** | Playing, performing live | Dark Cockpit dims inactive controls, engine accent color dominant, trails active | Focused, expressive, in-flow |
| **Coupling** | Engines interacting, mutual modulation | Gold corridor illuminated, coupling arcs pulsing, crossfader active | Connected, emergent, collaborative |

The transition between registers is automatic based on user activity:
- Gallery mode: default when mouse is in preset browser or header
- Performance mode: activates when PlaySurface receives input (mouse down, MIDI note-on)
- Coupling mode: activates when 2+ engines are coupled and coupling amount > 0.1

Transition: 400ms ease-in-out. All non-active controls dim to 20% opacity. Active controls brighten to 100%.

---

## 1.2 The XOuija Planchette

The Planchette is the single most distinctive UI element in XOceanus. It is a translucent lens approximately 80x60pt that floats over the XOuija performance surface. It is simultaneously:
- A note display (shows current pitch, octave, Hz)
- An engine identifier (border tinted to engine accent color)
- A performance indicator (velocity, expression values)
- A navigation aid (reveals engine-specific parameter information)
- An autonomous entity (drifts in a Lissajous pattern when idle)

### Planchette Visual Specification

```
Outer shape: Rounded rectangle, 80x60pt, corner radius 12pt
Border: 2px, engine accent color at 70% opacity
Background: rgba(14, 14, 16, 0.65) — frosted glass effect
  (reference: Cubic Glass Gradient pack, Melatonin Blur JUCE library)
Inner glow: engine accent color at 8% opacity, 4px inset blur
```

**Text layout inside Planchette:**

```
+----------------------------------+
| [Note]  Eb4      [Hz]  311.1 Hz |  <- Space Grotesk SemiBold 13pt / JetBrains Mono 11pt
| ─────────────────────────────── |  <- 1px rule, engine accent at 20%
| Vel: 0.72    Expr: 0.45        |  <- JetBrains Mono Regular 10pt, text-muted
| OPERA                            |  <- Space Grotesk SemiBold 9pt ALL CAPS, engine accent
+----------------------------------+
```

Font: Note name = Space Grotesk SemiBold 13pt. Hz value = JetBrains Mono Regular 11pt. Vel/Expr = JetBrains Mono Regular 10pt, `rgba(240, 237, 232, 0.45)`. Engine name = Space Grotesk SemiBold 9pt ALL CAPS, engine accent color.

### Planchette Behaviors

**Idle Drift** (no touch active):
- Lissajous path: `x = cx + A*sin(a*t + delta)`, `y = cy + B*sin(b*t)`
- Frequency: a=0.3Hz, b=0.2Hz, delta=PI/2
- Amplitude: 15% of surface dimensions
- Opacity during drift: 40% (ghosted)
- Purpose: The interface has a heartbeat. It is alive even when you are not touching it.

**Lock-On** (touch begins):
- Snap to touch position: 150ms cubic-bezier(0.34, 1.56, 0.64, 1)
- Drift ceases immediately
- Opacity: 100%
- Border brightens to engine accent at 90%
- Inner glow intensifies to 15%
- Haptic (iPad): `UIImpactFeedbackGenerator(.light)`

**Active Tracking** (touch held):
- Follows touch with 2-3 frame momentum lag (spring constant k=0.85)
- Note name and Hz update in real-time
- Engine accent glow pulses with note velocity (breathing at 0.5Hz, amplitude = velocity * 5%)

**Release Trail** (touch ends):
- Planchette holds position for 400ms (warm memory)
- Note name fades to 50% opacity over 400ms
- Border fades to 40% opacity
- After 400ms, resumes idle drift from current position
- Drift resumes with a 1s ease-in (no sudden snap to pattern)

**Bioluminescent Trail**:
- Ring buffer: 12 `TrailPoint` structs `{float x, y, age, velocity}`
- Each point rendered as `fillEllipse(x, y, radius, radius)` where radius = 3 + velocity * 4
- Color: engine accent at `alpha = 0.6 * (1.0 - age / 1.5)`  (fade over 1.5s)
- Fast gestures: longer, brighter trails (velocity > 0.5 increases radius)
- Slow gestures: gentle, dim trails
- Paint cost: 12 `fillEllipse` calls per frame = negligible

**Ripple on Touch**:
- 3 concentric rings expand from touch point on note-on
- Max radius: 40pt. Duration: 300ms.
- Ring stroke: 1.5px, engine accent at `30% * (1 - t/0.3)` where t = age in seconds
- Painted as 3 `drawEllipse` calls from a fixed pool of 4 `RippleState` structs

---

## 1.3 The Tide Controller

A novel expression controller unique to XOceanus. A circular water-surface simulation (120pt diameter) that responds to touch with actual fluid dynamics.

**Physics**: 2D wave equation on a 16x16 grid:
```cpp
h[x][y] += velocity[x][y];
velocity[x][y] += propagation * (h[x-1][y] + h[x+1][y] + h[x][y-1] + h[x][y+1] - 4*h[x][y]);
velocity[x][y] *= damping; // 0.98
```

**Visual**: Height-field rendered with engine accent color. Peaks = bright, troughs = dark. Bilinear interpolated between grid points. 30fps update.

**Interactions**:
- Tap center: single ripple, decaying sine wave modulation
- Drag: continuous wave, amplitude = distance from center, rate = drag speed
- Two-finger (iPad): interference patterns from two wave sources
- Tilt (iPad gyroscope via `CMMotionManager`): water surface tilts with device
  - `import CoreMotion; let motionManager = CMMotionManager()`

**Parameter mapping**: Assignable to any parameter. Default: filter cutoff. The center height value h[8][8] is the output value, normalized 0-1. This creates organic, naturally decaying modulation that no LFO shape can replicate.

CPU cost: 256 float operations per frame at 30fps. Negligible.

**JUCE**: `TideController : public juce::Component, private juce::Timer`. 16x16 `float` arrays for height and velocity. `paint()` renders as filled rectangles with color interpolation. No OpenGL required.

---

## 1.4 The Coupling Visualizer

When 2+ engines are coupled, their relationship is visible as living arcs in the coupling strip. This is not a static diagram — it is a real-time visualization of the coupling energy flowing between engines.

**Arc rendering**:
- Each coupling connection draws a cubic Bezier curve between engine nodes
- Arc thickness: `1.5 + couplingAmount * 3.0` pixels (range 1.5-4.5px)
- Arc color: blend of both engines' accent colors (50/50 mix in Oklch color space)
- Arc animation: Perlin/Simplex noise displaces control points at 0.5Hz, amplitude 3px
- When LFO modulates coupling amount: arc visually pulses at LFO rate
- Data crosses audio/UI boundary via lock-free FIFO (never poll audio thread)

**The Living Gold Corridor**:
- Coupling strip header: `#E9C46A` (XO Gold) horizontal band, 24px height
- When coupling is active, the gold band pulses with the coupling energy: `alpha = 0.7 + 0.3 * couplingRMS`
- Gold glow extends 4px above and below the band via `juce::GlowEffect` or manual radial gradient
- This is the "golden corridor between exhibitions" — the gallery's most distinctive feature

**Engine nodes**:
- Circle, 24px diameter, filled with engine accent color
- Active: 100% opacity + subtle 2px glow in accent color
- Inactive: 25% opacity, no glow
- Label: engine short name, Space Grotesk Regular 10pt, below node

---

## 1.5 The Depth-Zone Dial

A custom control for navigating the water column. This replaces a traditional engine selector dropdown.

**Visual**: Circular dial, 80px diameter. Background is a radial gradient:
- Center: creature icon (engine-specific, outline-only, 1.5px stroke)
- Inner ring: engine accent color at 40%
- Outer ring: depth-zone gradient:
  - Top arc: `#48CAE4` (Sunlit zone)
  - Middle arc: `#0096C7` (Twilight zone)
  - Bottom arc: `#7B2FBE` (Midnight zone)

**Interaction**: Drag clockwise/counterclockwise to scroll through engines in water-column order (surface engines first, abyss engines last). Engine name displays below the dial in Space Grotesk Regular 13pt. The 50ms hot-swap crossfade triggers on each engine change.

**Gradient source**: Galactic Gradients pack (blue-purple cosmic gradients map directly to ocean depth zones). Extract hex stops from `~/Downloads/Galactic Gradients/` reference images.

---

## 1.6 The Sonic DNA Hexagons

Presets are visualized as generative hexagonal shapes — not bar charts. Each preset's 6D Sonic DNA (brightness, warmth, movement, density, space, aggression) generates a unique procedural form.

**Generation algorithm**:
- Start with a regular hexagon (6 vertices)
- Each vertex is displaced outward/inward by the corresponding DNA dimension value (0-1)
- High "Movement" (`#2A9D8F`) adds animated ripple to the outline (Perlin displacement at 0.3Hz)
- High "Space" (`#4A3680`) adds a breathing scale oscillation (scale: `1.0 + 0.02 * sin(t * 0.8)`)
- High "Aggression" (`#C0392B`) adds angular distortion to vertices (sharp peaks)
- High "Warmth" (`#E9A84A`) softens corners (increased corner radius on the shape)
- High "Brightness" (`#FFD700`) increases fill opacity
- High "Density" (`#7B2D8B`) adds concentric inner hexagons

**Visual**: Filled with gradient from center (white at 10%) to edge (DNA dimension colors blended). Outline: 1px, white at 20%. Size: 48x48 in preset cards, 120x120 in detail view.

**Performance**: Vertices computed once per preset load. Only the animation (ripple, breathing) updates per frame. At 30fps, cost is 6 vertex updates + 6 line draws = negligible.

---

# PART 2: DESKTOP LAYOUT

## 2.1 Master Dimensions

**Window size**: 1100x700pt (default, resizable)
**Minimum**: 960x600pt
**Maximum**: 1920x1080pt (scales proportionally)

**Golden Ratio grid**: The primary layout uses phi (1.618) proportions:
- Engine panels width: 680pt (61.8% of 1100)
- Right sidebar width: 420pt (38.2% of 1100)
- Header height: 52pt
- PlaySurface height: 264pt (37.7% of 700, close to 1 - 1/phi)
- Engine panel height: 332pt (47.4%, close to 1/phi of remaining after header)
- Bottom status bar: 52pt

---

## 2.2 Complete Layout Map

```
+==============================================================================+
|                           HEADER BAR (52pt)                                   |
|  [XO_OX Logo 32x32] [Depth-Zone Dial 48x48] [ENGINE NAME, Space Grotesk    |
|   SemiBold 16pt] [<] [Preset Name, Inter Medium 14pt] [>] [heart] [DNA hex] |
|                              [M1][M2][M3][M4] macro knobs 36x36             |
|  .............................................................................|
|  [DARK/LIGHT toggle] [CPU %] [MIDI indicator] [Settings gear]                |
+==============================================================================+
|                                                                               |
|  LEFT: ENGINE PANEL (680pt wide)              | RIGHT: SIDEBAR (420pt wide)  |
|  +------------------------------------------+ | +------------------------+   |
|  |                                          | | |  SIDEBAR TABS:         |   |
|  |  ENGINE PANEL A (when 1 engine)          | | |  [PRESET] [COUPLE]     |   |
|  |  or                                      | | |  [FX] [PLAY]           |   |
|  |  ENGINE PANEL A + B (when 2 engines)     | | |                        |   |
|  |  340pt each side-by-side                 | | |  ---- PRESET TAB ---- |   |
|  |                                          | | |  Mood filter row       |   |
|  |  Contains:                               | | |  Search field          |   |
|  |  - Engine nameplate                      | | |  Preset list/grid      |   |
|  |  - 4 macro knobs (redundant w/ header)   | | |  DNA hexagon detail    |   |
|  |  - Parameter groups (progressive disc.)  | | |                        |   |
|  |  - Oscilloscope / waveform view          | | |  ---- COUPLE TAB ----  |   |
|  |  - ADSR envelope display                 | | |  CouplingVisualizer    |   |
|  |                                          | | |  Coupling crossfader   |   |
|  |  332pt tall                              | | |  Type selector         |   |
|  |                                          | | |  Amount knob           |   |
|  +------------------------------------------+ | |                        |   |
|                                               | |  ---- FX TAB ----     |   |
|                                               | |  FX chain slots       |   |
|                                               | |  Wet/dry per slot     |   |
|                                               | |                        |   |
|                                               | |  ---- PLAY TAB ----   |   |
|                                               | |  Expression ctrls     |   |
|                                               | |  (when sidebar-only)  |   |
|                                               | +------------------------+   |
|                                                                               |
+===============================================================================+
|                        PLAYSURFACE (264pt tall)                               |
|  +--- Tab Bar (28pt) ----+                                                    |
|  | [XOUIJA] [MPC] [KEYS] |  <- Space Grotesk SemiBold 11pt ALL CAPS         |
|  +------------------------+     Active tab: XO Gold underline (2px)           |
|  |                                                                            |
|  |  ACTIVE SURFACE                     | EXPRESSION CONTROLLERS (172pt wide) |
|  |  (XOuija / MPC Pads / Seaboard)    | +-- Mod Wheel (28x160pt) --------+  |
|  |  888pt wide                         | +-- Pitch Bend (28x160pt) ------+  |
|  |  236pt tall                         | +-- Macro Strips (4x 20pt) -----+  |
|  |                                     | +-- Tide Controller (120pt dia) -+  |
|  |                                     |                                     |
|  +--------------------------------------------------------------------------- +
+===============================================================================+
|                        STATUS BAR (52pt)                                       |
|  [FIRE][XOSEND][ECHO CUT][PANIC]  |  [BPM: 120]  |  [Voices: 4/8]  | [CPU]  |
+===============================================================================+
```

---

## 2.3 Header Bar — Complete Specification

**Dimensions**: Full width x 52pt. Fixed, never scrolls.

**Background**:
- Light mode: `#F8F6F3` (Gallery shell white)
- Dark mode: `#1A1A1A`
- Bottom border: 1px, `rgba(221, 218, 213, 0.5)` light / `rgba(255, 255, 255, 0.07)` dark

### 2.3.1 Logo

- **Position**: Left edge, 16pt from left, vertically centered
- **Size**: 32x32pt
- **Asset**: SVG coupling symbol (interlocking X/O with gold bridge)
- **Bridge color**: Always `#E9C46A` (brand invariant)
- **Symbol color**: `#1A1A1A` (light mode) / `#F0EDE8` (dark mode)
- **Click action**: Opens About panel
- **JUCE**: `juce::Drawable::createFromSVG()`, cached as `std::unique_ptr<juce::Drawable>`, tinted via `setColour()`

### 2.3.2 Depth-Zone Dial (Engine Selector)

- **Position**: 56pt from left, vertically centered
- **Size**: 48x48pt
- **See**: Section 1.5 for full specification
- **Font**: Engine name below dial in Space Grotesk Regular 11pt, engine accent color
- **JUCE**: Custom `juce::Component` with `drawArc()` for depth gradient, `drawImage()` for creature icon

### 2.3.3 Preset Navigator

- **Position**: 120pt from left, vertically centered
- **Components**:
  - Left arrow button: 20x20pt, `#DDDAD5` stroke, 1.5px, round caps
  - Preset name: Inter Medium 14pt, `#1A1A1A` / `#F0EDE8`, max 200pt width, truncated with ellipsis
  - Right arrow button: 20x20pt
  - Heart (favorite) toggle: 20x20pt, outline when off (`#DDDAD5`), filled `#E9C46A` when on
  - DNA hexagon mini: 24x24pt, preset's 6D DNA visualization
- **Spacing**: 8pt between each element
- **Click on preset name**: Opens preset browser (sidebar switches to PRESET tab)
- **JUCE**: `juce::TextButton` for arrows, custom `juce::Button` for heart, custom paint for DNA hex

### 2.3.4 Macro Knobs (M1-M4)

- **Position**: Right-aligned, 16pt from right edge, vertically centered
- **Size**: 36x36pt each, 8pt spacing between
- **Knob style**: Filmstrip from Knob-Set-09 (JUCE-ready filmstrips at `~/Downloads/Audio UI/KNOBS-SET/knob-set-09/`)
  - Use Knob-01-Filmstrip.png for M1 (CHARACTER)
  - Use Knob-02-Filmstrip.png for M2 (MOVEMENT)
  - Use Knob-03-Filmstrip.png for M3 (COUPLING)
  - Knob-01 again for M4 (SPACE) — or Knob-03 from same set for differentiation
- **Arc overlay**: 3px arc, XO Gold `#E9C46A`, swept from 7 o'clock to current value position
- **Label**: Below knob, Space Grotesk SemiBold 8pt ALL CAPS, letter-spacing +0.08em
  - M1: "CHAR" in XO Gold
  - M2: "MOVE" in `#00FF41` (Phosphor Green)
  - M3: "COUP" in `#BF40FF` (Prism Violet)
  - M4: "SPACE" in `#00B4A0` (Phosphorescent Teal)
- **Value readout**: JetBrains Mono Regular 9pt, centered below label, shows 0-100
- **Animation**: When modulated by coupling, outer ring pulses with coupling energy (breathing: `alpha = 0.6 + 0.4 * sin(t * couplingRate)`, period matches coupling LFO)
- **Interaction**:
  - Drag up/down to change value
  - Shift+drag for fine control (1/4 speed)
  - Cmd+click to reset to 0.5
  - Double-click to type exact value
- **JUCE**: `juce::Slider` with `RotaryHorizontalVerticalDrag` style, custom `LookAndFeel::drawRotarySlider()` using filmstrip frames. Arc painted manually via `juce::Path::addArc()`. Spring physics for the "Weighted Brass" feel: `value += (target - value) * 0.12` per frame (critically damped, no oscillation for macro knobs).

### 2.3.5 Utility Controls (Far Right)

- **Position**: Right edge, 16pt margin, after macro knobs
- **Dark/Light toggle**: 20x20pt, sun/moon icon (HeroIcons), `juce::ToggleButton`
- **CPU meter**: JetBrains Mono 10pt, "CPU: 4.2%", green < 30%, amber 30-70%, red > 70%
- **MIDI indicator**: 8x8pt circle, dims when no MIDI, flashes engine accent on note-on (50ms flash, 200ms decay)
- **Settings gear**: 20x20pt, HeroIcons cog, opens settings panel

---

## 2.4 Engine Panel — Complete Specification

The engine panel is the "exhibition" inside the gallery. Each engine transforms this panel completely.

**Dimensions**: 680pt wide x 332pt tall (single engine). When 2 engines: 336pt each with 8pt gap.

### 2.4.1 Panel Background

- **Light mode**: `#FCFBF9` (slotBg) with 1px `#DDDAD5` border, 16pt corner radius
- **Dark mode**: `#2D2D2D` with 1px `rgba(255, 255, 255, 0.07)` border
- **Materiality layer** (from Master Spec v2):
  - Surface/Light engines (feliX energy): Frosted Etched Glass effect. Use background blur (Melatonin Blur JUCE library or GLSL shader). Panel appears to float with translucent depth.
  - Deep/Abyss engines (Oscar energy): Brushed gunmetal texture. Subtle bioluminescent under-lighting in engine accent color at 3-5% opacity along bottom edge.
- **JUCE**: For glass effect without OpenGL: paint a cached blurred copy of the background behind the panel using `juce::Image::rescaled()` at 25% then back to 100% (cheap blur approximation), tinted with `fillAll(Colour(0x55FCFBF9))`. For full fidelity: attach `OpenGLContext`, use GLSL Gaussian blur shader.

### 2.4.2 Engine Nameplate

- **Position**: Top-left of panel, 16pt margin
- **Font**: Space Grotesk SemiBold 16pt (or Nebulica Bold 16pt for premium feel — see asset registry 1.2)
- **Color**: Engine accent color at 100%
- **Identity band**: 4px vertical stripe at left edge of panel, full height, engine accent color at 60%
- **Creature icon**: 24x24pt, outline-only, 1.5px stroke, engine accent color, positioned right of nameplate text

### 2.4.3 Progressive Disclosure Control Layout

The Fab Five mandate: 16-24 controls visible at any time. XOceanus achieves this through three disclosure levels:

**Level 1 — Macro View (default, 4 controls visible)**:
- 4 macro knobs: CHARACTER, MOVEMENT, COUPLING, SPACE
- Each 48x48pt, arranged in a 2x2 grid
- Knob style: Kit-07 illuminated knobs (`~/Downloads/Audio UI/KITS/kit-07/`) — the "Light Knob" variant
- Arc: 3px, engine accent color
- Label below: Inter Medium 10pt, sentence case
- This is the "I just want to turn knobs and make sound" view

**Level 2 — Group View (click "EXPAND" or scroll down, 12-16 controls visible)**:
- Macros collapse to a horizontal strip (4 x 32x32pt knobs)
- Below: Parameter groups appear as collapsible sections:
  - OSCILLATOR (osc type, pitch, detune, mix)
  - FILTER (cutoff, resonance, env amount, type)
  - ENVELOPE (ADSR display — interactive, drag points)
  - MODULATION (LFO rate, depth, shape, target)
- Each group header: Space Grotesk SemiBold 10pt ALL CAPS, engine accent at 60%
- Knobs within groups: 28x28pt, Knob-Set-08 filmstrips (`~/Downloads/Audio UI/KNOBS-SET/knob-set-08/`)
- Spacing: 8pt between knobs within group, 16pt between groups
- Section collapse: Click group header to toggle. Smooth height animation, 200ms ease-out.

**Level 3 — Full Parameter View (double-click panel header or "ALL PARAMS")**:
- All engine parameters visible in a scrollable grid
- Mini knobs: 24x24pt, from any individual KNOBS/ entry
- Labels to the right of each knob: Inter Regular 10pt
- Values below each knob: JetBrains Mono Regular 9pt
- This mode is for sound designers who need every parameter visible

**Dark Cockpit integration**: In Performance mode (PlaySurface receiving input), Level 2/3 params dim to 20% opacity. Only the macro knobs remain at full brightness. Parameters being actively modulated by coupling or LFO remain at 80% opacity.

### 2.4.4 Oscilloscope / Waveform Display

- **Position**: Top-right of engine panel, 200x80pt
- **Style**: CRT aesthetic for OPTIC engine (phosphor green `#00FF41` on `#050A05` background with scanlines). All other engines: clean waveform in engine accent color on panel background.
- **Render**: `juce::Path` from audio buffer samples. 256 samples displayed. Stroke: 1.5px. Anti-aliased.
- **Update rate**: 30Hz via `AsyncUpdater` bridge from audio thread. Lock-free FIFO for waveform data.
- **Reduced motion**: Falls to 10Hz, static waveform snapshot.

### 2.4.5 ADSR Envelope Display

- **Position**: Below oscilloscope, 200x60pt
- **Interactive**: Drag points to adjust A/D/S/R values directly
- **Visual**: 4 connected line segments. Fill below the line: engine accent at 8%.
- **Points**: 4 draggable circles (8pt diameter), engine accent color, white center dot (4pt)
- **Stage labels**: Inter Regular 8pt, "A" "D" "S" "R" below each segment
- **Value readout**: JetBrains Mono 8pt below each point, shows ms/level
- **JUCE**: `juce::Path` with 4 line segments. Drag detection via `mouseDown()` proximity to point centers. Slew rate clamped for smooth drag. Parameters attached via `AudioProcessorValueTreeState::SliderAttachment` proxy.

---

## 2.5 Sidebar — Complete Specification

**Dimensions**: 420pt wide x 584pt tall (full height minus header and status bar)

### 2.5.1 Sidebar Tab Bar

- **Position**: Top of sidebar, full width, 36pt height
- **Tabs**: PRESET | COUPLE | FX | PLAY
- **Font**: Space Grotesk SemiBold 10pt ALL CAPS, letter-spacing +0.06em
- **Active tab**: Engine accent color text, 2px underline in engine accent
- **Inactive tab**: `rgba(240, 237, 232, 0.45)` text
- **Spacing**: Equal distribution across width

### 2.5.2 Preset Tab

**Mood Filter Row**:
- Horizontal scroll of mood pills: FOUNDATION, ATMOSPHERE, ENTANGLED, PRISM, FLUX, AETHER, FAMILY, SUBMERGED, COUPLING, CRYSTALLINE, DEEP, ETHEREAL, KINETIC, LUMINOUS, ORGANIC
- Each pill: Inter Medium 9pt ALL CAPS, letter-spacing +0.1em, 6px corner radius
- Active: mood color fill, white text
- Inactive: transparent, mood color text at 60%
- Height: 28pt, spacing 6pt

**Search Field**:
- Full width minus 16pt margins, 36pt height
- Background: `#EAE8E4` light / `#363636` dark, 8pt corner radius
- Placeholder: Inter Regular 12pt, "Search 19,859+ presets...", `#777570`
- Search icon: HeroIcons magnifying glass, 16x16pt, left-aligned inside field
- Focus ring: 2px `#E9C46A` outline-offset 2px

**Preset List**:
- Scrollable list, full remaining height
- Each preset card: full width x 72pt
- Layout per card:
  ```
  +----------------------------------------------+
  | [DNA hex 32x32] [Preset Name, Inter Med 13pt]|
  |                 [Engine badge, 8pt caps]      |
  |                 [mood dot] [tags, Inter 10pt] |
  | [heart 16x16]                                 |
  +----------------------------------------------+
  ```
- Hover: background `#F0EDE8` light / `#2D2D2D` dark
- Selected: left border 3px engine accent, background `#F8F6F3` light / `#1F1F24` dark
- Transition: 100ms ease-out on hover, instant on click

### 2.5.3 Coupling Tab

**CouplingVisualizer**:
- 380pt wide x 240pt tall, centered in tab
- Background: `#0E0E10` (deep) with subtle radial gradient to `#141416` at edges
- Engine nodes: 28pt circles, accent color fill, positioned by slot (1-4 arranged in a diamond)
- Coupling arcs: See Section 1.4
- Coupling type labels: JetBrains Mono 9pt, centered on arc midpoint
- All non-active arcs: 15% opacity (Dark Cockpit)

**Coupling Crossfader**:
- Full width x 32pt, horizontal
- Track: 2px, `rgba(255, 255, 255, 0.07)`
- Thumb: 16x24pt rounded rectangle, XO Gold fill
- Left label: Engine A name, Space Grotesk Regular 10pt, Engine A accent
- Right label: Engine B name, Space Grotesk Regular 10pt, Engine B accent
- Center: blended gradient of both engine accent colors
- **JUCE**: `juce::Slider` with `LinearHorizontal`, custom `LookAndFeel`

**Coupling Type Selector**:
- Dropdown: Inter Medium 11pt, current type displayed
- 15 coupling types: Amplitude, Filter, Pitch, Ring, FM, Sync, Waveshape, Granular, Spectral, Formant, Reverb, Delay, Knot, Triangular, Adversarial
- Each type has an icon (8x8pt, 1.5px stroke)

**Coupling Amount Knob**:
- 48x48pt, Kit-07 illuminated knob
- Arc: 3px, XO Gold
- Label: "AMOUNT", Space Grotesk SemiBold 9pt ALL CAPS
- Value: JetBrains Mono 10pt, 0-100%

### 2.5.4 FX Tab

**FX Chain Slots** (4 slots):
- Each slot: full width x 56pt
- Contains: FX name dropdown (Inter Medium 11pt), bypass toggle (12x12pt circle), wet/dry knob (24x24pt mini)
- Active slot: full opacity, accent-colored left border (3px)
- Bypassed: 30% opacity
- Drag to reorder (drag handle: 3 horizontal dots, 12x8pt)

**Chain modes**: Serial / Parallel toggle at top (Space Grotesk SemiBold 9pt ALL CAPS)

### 2.5.5 Play Tab

When the sidebar is in Play mode, expression controllers that normally appear next to the PlaySurface are shown here instead (useful when the window is narrow).

Contains: Mod Wheel, Pitch Bend, XY Pad (160x160pt), Breath Display, Macro Strips, Tide Controller — arranged vertically, scrollable.

---

## 2.6 PlaySurface — Complete Specification

**Dimensions**: 1100pt wide x 264pt tall (full width, fixed height)

### 2.6.1 Tab Bar

- **Height**: 28pt
- **Background**: `#1A1A1A` (dark stage, even in light mode — per design guidelines Section 3.6)
- **Tabs**: XOUIJA | MPC PADS | KEYS
- **Font**: Space Grotesk SemiBold 10pt ALL CAPS
- **Active**: `#F0EDE8` text, 2px XO Gold underline
- **Inactive**: `rgba(240, 237, 232, 0.35)`
- **Switching**: Instant — surfaces are preloaded, visibility-toggled only

### 2.6.2 XOuija Surface

- **Dimensions**: 888pt x 236pt (full surface width minus expression controller panel)
- **Background**: Ocean depth gradient, cached as `juce::Image` in `resized()`:
  - Top: `#48CAE4` (Sunlit zone — high pitch, bright expression)
  - Middle: `#0096C7` (Twilight zone)
  - Bottom: `#7B2FBE` to `#150820` (Midnight zone — low pitch, dark expression)
  - Reference: Galactic Gradients pack for blue-purple stops
- **Scale fret lines** (when scale active):
  - Vertical lines at quantized pitch positions
  - Stroke: 1px, `rgba(240, 237, 232, 0.04)` — EXTREMELY subtle (Issea's mandate: suggestions, not walls)
- **Parameter sensitivity map** (background luminance layer):
  - 64x64 texture, precomputed per preset
  - White at 8% opacity where sensitivity is high
  - Black (transparent) where sensitivity is low
  - Rendered as a `juce::Image` overlay using `drawImageWithin()` with `Graphics::lowResamplingQuality`
- **Engine state visualization** (bidirectional surface — Principle 3):
  - LFO phase rendered as a sine-wave displacement on the gradient
  - Coupling energy as brightness pulse between coupled engine positions
  - Default: 5% opacity. Adjustable. Toggleable off.
- **Planchette**: See Section 1.2
- **Trails + Ripples**: See Section 1.2
- **JUCE class**: `XOuijaComponent : public juce::Component, private juce::Timer`
- **Performance**: Background image cached. Planchette as pre-rendered `juce::Image`. Trails as `fillEllipse`. Ripples as `drawEllipse`. Total paint budget: <2ms at 60fps.

### 2.6.3 MPC 16-Pad Grid

- **Dimensions**: 888pt x 236pt (same as XOuija — fill available space)
- **Grid**: 4x4, minimum 4pt gap between pads
- **Pad size**: approximately 216x55pt each (scales to fill)
- **Numbering**: MPC standard — Pad 1 bottom-left, Pad 16 top-right
- **See**: PlaySurface Design Spec Section 3 for complete pad visual states, velocity curves, and MPC integration

**Pad visual states**:
1. **Idle**: Dark background `#1A1A1A`, ocean-depth zone tinting (row 0 = midnight violet, row 3 = sunlit cyan). Pad number: zone-colored, JetBrains Mono 14pt, center. 1px engine accent border at 12%.
2. **Hover**: Background +8% brightness. Border 30% opacity.
3. **Hit**: Engine accent fill at `opacity = velocity`. White flash: 20% opacity, 50ms duration. Pad number inverts at high velocity.
4. **Warm Memory**: Ghost ring in engine accent at `alpha = 0.4 * (1 - age/1.5)`, 1.5s decay.

**Velocity Heatmap Mode** (toggle in tab bar):
- All 16 pads show persistent velocity as color fill
- Decay synced to host tempo (4 beats at current BPM)
- Creates visual drum pattern — essential for beatmaking

**Keyboard mapping**:
```
Row 3 (top):    Q  W  E  R    → Pads 13-16
Row 2:          A  S  D  F    → Pads 9-12
Row 1:          Z  X  C  V    → Pads 5-8
Row 0 (bottom): 7  8  9  0    → Pads 1-4
```
Keyboard bindings shown as small key letter, bottom-right of each pad, JetBrains Mono 8pt at 30% opacity.

**JUCE class**: `MPCPadGrid : public juce::Component`. 16 child `PadComponent` instances (isolated repaints). Shared 30fps timer. Velocity via `juce::Time::getMillisecondCounterHiRes()` in `mouseDown`.

### 2.6.4 Seaboard-Style Continuous Keyboard

- **Dimensions**: 888pt x 236pt
- **Visible range**: 2 octaves (24 keys), scrollable
- **Key width**: ~37pt per natural key (888pt / 24 = 37pt)
- **Natural keys**: `#F0EDE8` (Gallery warm white)
- **Sharp keys**: `#2A2A2A` (deep charcoal), shorter (60% of key height)
- **Touch active**: Engine accent color fill at `pressure` opacity
- **Pitch bend zones**: Left/right 15% of each key, revealed only on touch (progressive disclosure)
- **Glide trail**: Engine accent at 40%, 200ms fade
- **Octave scroll strip**: 16pt tall above keyboard, tick marks at each C, drag to scroll
- **MPE**: Generates per-note Pitch Bend, CC74 (Slide), Channel Pressure
- **See**: PlaySurface Design Spec Section 4 for complete specification

**JUCE class**: `SeaboardKeyboard : public juce::Component, private juce::Timer`. Single `paint()` pass — no child components per key. `std::array<TouchState, 8>` for up to 8 simultaneous touches.

### 2.6.5 Expression Controller Panel

- **Position**: Right side of PlaySurface area, 172pt wide x 236pt tall
- **Background**: `#141416` (darker than surface background)

**Layout (top to bottom)**:

| Controller | Size | Position |
|-----------|------|----------|
| Mod Wheel | 28x160pt | Top-left |
| Pitch Bend | 28x160pt | Top-right (next to Mod Wheel) |
| Macro Strips (x4) | 20pt wide each, 120pt tall | Below wheels, 4pt spacing |
| Tide Controller | 100pt diameter | Below macros, centered |

**Mod Wheel**:
- Vertical strip with ridged texture (4 horizontal ridges, reference Kit-28 wheel filmstrip `~/Downloads/Audio UI/KITS/kit-28/`)
- Fill rises from bottom: engine accent color
- Value display: JetBrains Mono 9pt above strip
- Spring-return: OFF by default (toggle)
- Sends CC1

**Pitch Bend**:
- Vertical strip, spring-return ON (snaps to center)
- Center line: 1px, `rgba(255, 255, 255, 0.2)`
- Current position: bright dot, 8pt, engine accent
- Semitone ticks visible when range > +/-12
- Spring physics: `pos += (0.5 - pos) * 0.15` per tick (critically damped)
- Sends Pitch Bend

**Macro Strips**:
- 4 vertical sliders, 20pt wide each, 4pt spacing
- Fill from bottom in macro colors:
  - M1: `#E9C46A` (XO Gold)
  - M2: `#00FF41` (Phosphor Green)
  - M3: `#BF40FF` (Prism Violet)
  - M4: `#00B4A0` (Phosphorescent Teal)
- Labels: Space Grotesk SemiBold 7pt ALL CAPS at top
- Value: JetBrains Mono 8pt at top

**Tide Controller**: See Section 1.3

---

## 2.7 Status Bar — Complete Specification

**Dimensions**: Full width x 52pt

### 2.7.1 Performance Pads (Left Section)

4 trigger pads, each 48x48pt, 8pt spacing:
- **FIRE**: `#4ADE80` fill, Space Grotesk Bold 9pt ALL CAPS, keyboard shortcut Z
- **XOSEND**: `#F5C97A` fill, keyboard shortcut X
- **ECHO CUT**: `#F5C97A` fill, keyboard shortcut C
- **PANIC**: `#EF4444` fill, keyboard shortcut V

Pads use spring physics compression on click (Hooke's Law): visual depth shift of 2px downward over 50ms, bounce back over 150ms. Haptic on iPad: `.medium` impact.

### 2.7.2 Transport Info (Center)

- BPM: JetBrains Mono 12pt, synced to host or internal clock
- Voice count: JetBrains Mono 10pt, "Voices: 4/8"
- CPU: JetBrains Mono 10pt, color-coded (green/amber/red at 30%/70% thresholds)

### 2.7.3 Engine Slot Indicators (Right)

- 4 circles (12pt diameter), one per engine slot
- Active: engine accent color fill
- Empty: `#EAE8E4` light / `#363636` dark
- Click: selects that engine slot for editing

---

## 2.8 Dark Cockpit Behavior — Complete Specification

Dark Cockpit is the system that manages attentional focus. It is NOT a "dark mode" — it operates within both light and dark modes.

### 2.8.1 Trigger Conditions

| Trigger | Dim Target | Timing |
|---------|-----------|--------|
| PlaySurface receives mouse/touch input | Engine panel params (Level 2+3) | 400ms ease-in |
| No input for 3 seconds after performance | Restore all controls to full | 600ms ease-out |
| Coupling amount > 0.1 | Non-coupled engine controls | 300ms ease-in |
| Preset browser opened | Engine panels | 200ms ease-in |
| Settings panel opened | Everything except settings | 200ms ease-in |

### 2.8.2 Opacity Levels

| State | Opacity | Description |
|-------|---------|-------------|
| Active-primary | 100% | Currently being interacted with |
| Active-modulated | 80% | Being modulated by coupling/LFO (not directly touched) |
| Ambient | 45% | Visible but not currently relevant |
| Dimmed | 20% | Background — present but cognitively invisible |
| Hidden | 0% | Collapsed/closed elements |

### 2.8.3 Exceptions (Never Dim)

- XO_OX logo (always visible for brand identity)
- Active engine nameplate
- PANIC button (safety — always visible and accessible)
- Current preset name
- CPU indicator (safety — always monitor system health)
- Accessibility focus ring (WCAG requirement)

---

## 2.9 Sound on First Launch

### 2.9.1 Behavior

When XOceanus loads for the first time (detected via `localStorage` flag):
1. Engine slot 1 loads OXBOW with preset "First Breath" (a gentle, evolving pad)
2. The preset begins playing automatically at velocity 0.4
3. Mouse hover over ANY control produces subtle parameter modulation:
   - Hover over a knob: value shifts ±5% toward cursor direction
   - Hover over coupling strip: coupling amount increases by 0.1 temporarily
   - Hover over PlaySurface: gentle pitch shift based on cursor X position
4. After 30 seconds, the automatic sound fades to silence over 3 seconds
5. A toast notification appears: "Welcome to XOceanus. Touch anything." (Inter Regular 13pt, XO Gold text)

### 2.9.2 Subsequent Launches

On all subsequent launches, XOceanus loads silently with the last-used preset and engine configuration. The "hover modulation" feature is OFF by default but can be re-enabled in Settings (toggle: "Interactive Hover").

---

# PART 3: iOS LAYOUT

## 3.1 iPad Layout — Landscape (Primary)

**Target**: iPad Pro 11" (2388x1668 @ 2x = 1194x834pt) and iPad Pro 12.9" (2732x2048 @ 2x = 1366x1024pt)

```
+=========================================================================+
|  NAV BAR (44pt — iOS standard)                                          |
|  [Back] [XOceanus Logo 28x28] [Engine Name] [Preset ◄ ►] [M1 M2 M3 M4]|
+=========================================================================+
|                                                                          |
|  FULL-SCREEN PLAY SURFACE                                               |
|  (XOuija default, tab bar at top to switch)                             |
|                                                                          |
|  Multi-touch: up to 5 Planchettes (XOuija), 10 pads (MPC), 8 keys     |
|                                                                          |
|  Full width x (screen height - nav bar - bottom bar - safe area)        |
|  ~660pt on iPad Pro 11", ~850pt on iPad Pro 12.9"                       |
|                                                                          |
|  Edge swipe RIGHT → Expression Controllers slide in (280pt drawer)      |
|  Edge swipe LEFT → Engine Selector + Preset Browser slide in (320pt)    |
|                                                                          |
+=========================================================================+
|  BOTTOM BAR (64pt + safe area inset)                                    |
|  [CHAR 36x36] [MOVE 36x36] [COUP 36x36] [SPACE 36x36]  | [Engine] |  |
|  [FIRE] [XOSEND] [ECHO] [PANIC]                          | [Octave] |  |
+=========================================================================+
```

### 3.1.1 Touch Targets

- All interactive elements: minimum 44x44pt hit area (Apple HIG)
- Visual elements may be smaller (24pt knob) but expand to 44pt touch target
- Macro knobs in nav bar and bottom bar: 36pt visual, 44pt touch
- Performance pads in bottom bar: 44x44pt visual and touch

### 3.1.2 Multi-Touch on XOuija

- Each simultaneous touch spawns its own Planchette instance (from a fixed pool of 5)
- Each Planchette colored with engine accent
- Independent note tracking per Planchette
- Polyphonic: 5-voice limit on XOuija
- Two-finger vertical swipe: octave shift (without lifting playing fingers)
- `UITouch.force` (3D Touch devices): velocity/aftertouch mapping
- Non-3D Touch: `majorRadius` rate of change for velocity estimation

### 3.1.3 Haptic Feedback

| Event | Generator | Intensity |
|-------|-----------|-----------|
| Pad hit (MPC) | `UIImpactFeedbackGenerator` | `.light` (vel < 0.3), `.medium` (0.3-0.7), `.heavy` (> 0.7) |
| Scale-tone crossing (XOuija) | `UISelectionFeedbackGenerator` | Default (light tick) |
| Pitch bend center detent | `UINotificationFeedbackGenerator` | `.success` (distinct bump) |
| Octave boundary | `UIImpactFeedbackGenerator` | `.heavy` (strong thump) |
| Engine switch | `UINotificationFeedbackGenerator` | `.warning` (double tap) |
| Macro knob at 0/50/100 | `UISelectionFeedbackGenerator` | Default tick |

### 3.1.4 Edge Swipe Drawers

**Right drawer (Expression Controllers)**:
- Width: 280pt
- Contains: Mod Wheel, Pitch Bend, XY Pad (200x200pt), Breath Display, Tide Controller
- Swipe threshold: 20pt from right edge, 40pt minimum swipe distance
- Animation: 300ms spring curve (UISpringTimingParameters, dampingRatio 0.85)
- Background: `#141416` with 8pt left border radius
- Dismiss: swipe left or tap outside

**Left drawer (Engine + Presets)**:
- Width: 320pt
- Top half: Engine selector (grid of 90 engine icons, each 44x44pt, colored by accent)
- Bottom half: Preset browser (same as desktop sidebar PRESET tab, touch-optimized)
- Dismiss: swipe right or tap outside

### 3.1.5 The XOuija as Primary iPad Instrument

On iPad, the XOuija is the star. The entire screen becomes a continuous instrument surface. The Planchette is the performer's bow. The ocean gradient is the stage. The bioluminescent trails are the performer's signature.

This is the "XOceanus Pocket" experience that makes a tween fall in love with synthesis: open the app, see a beautiful ocean surface, touch it, and hear something magical. Move your fingers and the sound responds. See trails of light follow your gestures. This is the 12-year-old test, passed.

---

## 3.2 iPad Layout — Portrait

**Layout**: Single-column, scrollable

```
+================================+
|  NAV BAR (44pt)                |
|  [Logo] [Engine] [Preset ◄ ►] |
+================================+
|                                 |
|  PLAY SURFACE                   |
|  Full width x 400pt            |
|  (XOuija or MPC Pads)          |
|                                 |
+================================+
|  MACRO STRIP (horizontal)      |
|  [CHAR] [MOVE] [COUP] [SPACE] |
|  48pt height                   |
+================================+
|  ENGINE PANEL (scrollable)     |
|  Level 1 macros visible        |
|  Expand for Level 2/3          |
|                                 |
+================================+
|  BOTTOM TAB BAR (49pt + safe)  |
|  [Play] [Engine] [Presets] [FX]|
+================================+
```

Portrait mode uses a standard iOS tab bar (49pt) with 4 tabs for navigation. The PlaySurface is always visible at the top. Engine panel scrolls below.

---

## 3.3 iPhone Layout

**Target**: iPhone 15 Pro (393x852pt), iPhone 15 Pro Max (430x932pt)

iPhone is a simplified, single-purpose view at a time.

**Landscape**: Full-screen PlaySurface only. Performance-focused. Bottom bar with FIRE/XOSEND/ECHO/PANIC. No engine editing — connect to DAW and use desktop for sound design.

**Portrait**: Tab-based navigation:
- Tab 1 (Play): PlaySurface (XOuija or simplified 4x4 pad grid, 4 pads per row)
- Tab 2 (Sound): Engine macros (4 large knobs, 80x80pt) + preset navigator
- Tab 3 (Browse): Preset browser (full screen list)
- Tab 4 (FX): FX chain controls

All touch targets: 44pt minimum. Font sizes: minimum 11pt (iOS Dynamic Type support).

---

# PART 4: ACCESSIBILITY

## 4.1 WCAG 2.1 Compliance Target

**Minimum**: AA across all components
**Target**: AAA for text, AA for non-text
**Verification**: All contrast ratios computed per WCAG relative luminance formula

### 4.1.1 Color Contrast Matrix (Verified)

| Foreground | Background | Ratio | Grade | Usage |
|-----------|-----------|-------|-------|-------|
| `#E9C46A` XO Gold | `#1A1A1A` dark bg | 7.2:1 | AAA | Primary accent on dark |
| `#E9C46A` XO Gold | `#F8F6F3` light shell | 2.4:1 | FAIL normal / AA large | **Use `#7A5C1E` (XO Gold Text) for text on light bg** |
| `#7A5C1E` XO Gold Text | `#F8F6F3` light shell | 5.0:1 | AA | Gold text on light mode |
| `#1A1A1A` text | `#F8F6F3` light shell | 14.7:1 | AAA | Primary text, light mode |
| `#F0EDE8` text | `#1A1A1A` dark bg | 13.9:1 | AAA | Primary text, dark mode |
| `#F0EDE8` text | `#2D2D2D` card bg | 10.1:1 | AAA | Card text, dark mode |
| `#6B6965` secondary | `#F8F6F3` light shell | 5.0:1 | AA | Secondary text, light mode |
| `#A0A0A0` secondary | `#1A1A1A` dark bg | 6.0:1 | AA | Secondary text, dark mode |
| `#A0A0A0` secondary | `#2E2E2E` card bg | 4.6:1 | AA | Card secondary, dark mode |

### 4.1.2 Engine Accent Color Accessibility

Several engine accent colors on dark backgrounds:

| Engine | Accent | vs `#1A1A1A` | Grade | Notes |
|--------|--------|-------------|-------|-------|
| ODDFELIX | `#00A6D6` | 5.4:1 | AA | Safe for text and controls |
| ORCA | `#1B2838` | 1.1:1 | FAIL | **Never use as text on dark bg. Use outline/stroke only.** |
| OCEANDEEP | `#2D0A4E` | 1.1:1 | FAIL | **Same — outline/stroke only on dark bg** |
| OBSIDIAN | `#E8E0D8` | 12.4:1 | AAA | Near-white; relies on stroke for state |
| OGRE | `#0D0D0D` | 1.0:1 | FAIL | **Darkest engine. Use brightened variant `#4D4D4D` for UI elements.** |

**Mitigation for low-contrast engines**: When an engine's accent color fails AA against the background, the UI automatically:
1. Uses a brightened variant (+40 lightness in Oklch) for text and arc fills
2. Adds a 1px contrasting outline (white at 20% on dark, black at 20% on light)
3. Uses the original accent color only for large fills (>24pt elements) where 3:1 ratio suffices

### 4.1.3 Color-Blind Safe Engine Differentiation

86 engines cannot be differentiated by color alone. The system provides multiple redundant channels:

**Channel 1 (Color)**: Engine accent color — unique per engine but NOT relied upon as sole identifier

**Channel 2 (Shape)**: Engine creature icons — 73 unique outline shapes (1.5px stroke, 24x24pt). Shape recognition does not require color perception.

**Channel 3 (Text)**: Engine short name always displayed alongside icon/color (Space Grotesk Regular, minimum 10pt)

**Channel 4 (Position)**: Water column depth position (surface/reef/open/deep/abyss) provides categorical grouping

**Channel 5 (Pattern)**: In coupling visualizer, each coupling TYPE uses a distinct stroke pattern:
- Amplitude: solid
- Filter: dashed (4px dash, 4px gap)
- Pitch: dotted (2px dot, 4px gap)
- Ring Mod: double line
- FM: wave pattern
- Others: unique dash patterns

**Color-blind simulation testing**: All UI screenshots must pass Deuteranopia (green-blind), Protanopia (red-blind), and Tritanopia (blue-blind) simulation with all information remaining discernible.

---

## 4.2 VoiceOver / Screen Reader Support

### 4.2.1 Component Accessibility Map

| Component | AccessibilityRole | Label Pattern | Value Pattern |
|-----------|------------------|---------------|---------------|
| Macro Knob | `slider` | "Character macro" | "72 percent" |
| Engine Selector | `comboBox` | "Engine selector, currently OPERA" | "OPERA, engine 45 of 88" |
| Preset Navigator | `group` | "Preset navigator" | — |
| Preset Name | `staticText` | "Current preset: Velvet Morning" | — |
| Prev/Next Preset | `button` | "Previous preset" / "Next preset" | — |
| Favorite Toggle | `toggleButton` | "Favorite" | "On" / "Off" |
| MPC Pad | `button` | "Pad 1, velocity 72 percent" | — |
| Coupling Amount | `slider` | "Coupling amount, OPERA to OBRIX" | "45 percent" |
| PlaySurface Tab | `button` + `tab` | "XOuija surface" | "Selected" / — |
| Tide Controller | `slider` | "Tide controller, filter cutoff" | "Wave height 63 percent" |
| FIRE button | `button` | "Fire, performance trigger" | — |
| PANIC button | `button` | "Panic, stop all sound" | — |

### 4.2.2 JUCE Implementation

Every interactive component overrides `createAccessibilityHandler()`:

```cpp
std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override
{
    return std::make_unique<juce::AccessibilityHandler>(
        *this,
        juce::AccessibilityRole::slider,
        juce::AccessibilityActions()
            .addAction(juce::AccessibilityActionType::showMenu, [this] { showContextMenu(); })
    );
}
```

All icon-only buttons have `setDescription()` with human-readable text.
All value controls implement `AccessibilityValueInterface` returning formatted strings.

### 4.2.3 VoiceOver Rotor Groups (iOS)

Group related controls into accessibility containers:
- "Macro controls" (M1-M4)
- "Play surface" (current surface + expression)
- "Preset browser" (search, mood filter, preset list)
- "Coupling controls" (visualizer, crossfader, type, amount)
- "Effects chain" (4 FX slots)

---

## 4.3 Keyboard Navigation

### 4.3.1 Tab Order (Desktop)

Tab order follows visual layout, left-to-right, top-to-bottom:

1. Header: Logo → Engine Selector → Preset Nav (prev, name, next, heart) → M1 → M2 → M3 → M4 → Settings
2. Engine Panel: Parameter groups, left-to-right within each group, top-to-bottom between groups
3. Sidebar: Active tab → tab content controls
4. PlaySurface: Surface tab bar → active surface → expression controllers
5. Status Bar: FIRE → XOSEND → ECHO CUT → PANIC

### 4.3.2 Keyboard Shortcuts (Non-Tab)

| Key | Action | Context |
|-----|--------|---------|
| Tab / Shift+Tab | Navigate between controls | Global |
| Enter / Space | Activate focused button/toggle | Global |
| Arrow Up/Down | Adjust focused slider/knob | Slider focused |
| Arrow Left/Right | Fine adjust (0.1% increments) | Slider focused |
| Home / End | Set to minimum / maximum | Slider focused |
| 1 / 2 / 3 | Select XOuija / MPC / Keys surface | PlaySurface focused |
| Q-P, A-L, Z-M, 7-0 | MPC pad triggers | MPC surface active |
| Escape | Close modal/drawer/browser | Any modal open |
| Cmd+Z / Cmd+Shift+Z | Undo / Redo | Global |

### 4.3.3 Focus Ring

- 2px solid `#E9C46A` (XO Gold), offset 2px from element border
- Visible only on keyboard focus (`:focus-visible` equivalent)
- Never suppressed, even in Dark Cockpit mode
- JUCE: Paint in `focusGained()`, clear in `focusLost()`:
```cpp
void paintOverChildren(juce::Graphics& g) override
{
    if (hasKeyboardFocus(false))
    {
        g.setColour(juce::Colour(0xFFE9C46A));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1), cornerRadius, 2.0f);
    }
}
```

---

## 4.4 Reduced Motion

When the OS reduced-motion preference is active (`prefers-reduced-motion: reduce` on web, `UIAccessibility.isReduceMotionEnabled` on iOS):

| Feature | Normal | Reduced Motion |
|---------|--------|----------------|
| Planchette idle drift | Lissajous at 0.3Hz | Static at center |
| Bioluminescent trails | 12-point trail, 1.5s decay | No trail (instant fade) |
| Ripple on touch | 3 expanding rings, 300ms | Single flash, 50ms |
| Coupling arc animation | Perlin noise displacement | Static arc |
| DNA hexagon animation | Ripple + breathing | Static shape |
| Dark Cockpit transitions | 400ms ease | Instant opacity change |
| Preset switch | Crossfade 150ms | Instant |
| Panel expand/collapse | Height animation 200ms | Instant |
| OpticVisualizer | 30Hz update | 10Hz, Scope mode only |
| Sound on First Launch | Hover modulation active | Disabled (silent until touched) |
| Macro knob coupling pulse | Breathing animation | Static arc |

**Detection in JUCE**:
```cpp
// macOS
bool reducedMotion = [[NSWorkspace sharedWorkspace] accessibilityDisplayShouldReduceMotion];

// iOS
bool reducedMotion = UIAccessibility.isReduceMotionEnabled;

// Fallback: user preference in XOceanus settings
bool reducedMotion = userPrefs.getBoolValue("reducedMotion", false);
```

---

## 4.5 Screen Magnification Compatibility

- All text renders as vector (no rasterized text)
- All icons are SVG-based (`juce::Drawable`), scale cleanly to any size
- Layout uses proportional sizing (percentage of parent) rather than absolute pixels
- Font sizes compute from component height: `fontSize = getHeight() * factor`
- Minimum font size: 9pt at 1x, scales with OS text size preference
- Knob filmstrips embed at highest available resolution; JUCE scales down at runtime

---

# PART 5: THE 25% + 25%

## 5.1 The First 25% — What We Held Back

### 5.1.1 Gesture Memory — The Trail as Modulation Source

The bioluminescent trail that follows the Planchette is not just visual feedback. It IS a modulation source. The trail's path over the last 4 bars is recorded as a sequence of (x, y, velocity, time) tuples. This sequence can be:

1. **Looped** — the trail replays as an LFO-like modulation source. The X path modulates one parameter, the Y path modulates another. The performer literally "draws" their modulation shape by moving on the XOuija.
2. **Frozen** — a single gesture is captured and held as a static modulation shape. Click the trail to freeze it. The frozen trail becomes a visible "ghost path" on the surface.
3. **Coupled** — two performers' trails (multi-touch on iPad) can be COUPLED. The interference pattern between two gesture paths creates modulation that neither performer intended alone.

No other synthesizer turns the performer's gesture history into a reusable modulation source. LFOs approximate what the performer does. This IS what the performer did.

**Implementation**: Ring buffer of 256 `(float x, float y, float vel, double time)` tuples. Circular read at tempo-synced rate. Output normalized 0-1. Attached to any modulatable parameter via the mod matrix.

**JUCE**: `TrailModulationSource : public ModulationSource`. Reads from the same ring buffer that the visual trail uses. Zero additional computation — the data already exists.

### 5.1.2 Spectral Silhouette — Engine Identity as Visual DNA

Each engine has a unique spectral fingerprint — the average frequency content of its presets. This fingerprint can be rendered as a "silhouette" behind the engine nameplate: a wavy outline that represents the engine's characteristic frequency curve.

- OCEANDEEP's silhouette is a low, dark blob (bass-heavy, filter ceiling at 800Hz)
- OPENSKY's silhouette is tall and wide (full-spectrum shimmer)
- ONSET's silhouette has sharp transient spikes (percussive)

The silhouette is precomputed per engine (not per preset) and rendered as a filled `juce::Path` at 5% opacity behind the engine panel. It provides instant visual identification of the engine's character without reading any text.

**Implementation**: 128-bin average spectral magnitude per engine, normalized, rendered as a smooth path. Computed offline once per engine. Stored as a 128-float array in engine metadata.

### 5.1.3 Breathing Animation Formula

All "living" animations in XOceanus use the same organic breathing function (from Gemini research):

```
f(t) = (e^sin(t) - 1/e) / (e - 1/e)
```

This produces a value that oscillates between 0 and 1 with a natural, organic feel — inhale is faster than exhale, matching biological respiration. Use this for:
- Planchette idle drift amplitude modulation
- Coupling arc glow pulsing
- Engine activity indicator
- DNA hexagon breathing

**Period**: Default 4 seconds (t advances by `2*PI / (4 * frameRate)` per frame).

---

## 5.2 The Second 25% — What Has Never Been Attempted

### 5.2.1 Synaptic Preset Morphing — The Preset IS the Interface

Current state of the art: presets are loaded discretely. You click "Velvet Morning" and all parameters jump. Some synths (Pigments, Massive X) offer A/B morphing between two presets.

XOceanus does something no software has attempted: **Spatial Preset Navigation**.

The preset browser is not a list — it is a territory. Presets are plotted on a 2D map whose axes are two of the six Sonic DNA dimensions (default Brightness × Warmth, user-selectable). The map fills the sidebar's PRESET tab. This is honest spatial organization by DNA + mood — not dimensionality reduction. An actual learned projection (PCA/UMAP) is a future enhancement and requires a versioned schema; it is not what ships today.

The performer does not SELECT a preset. They NAVIGATE to a location on the map. As they move, XOceanus continuously interpolates between the nearest presets. Every point on the map produces a unique sound — not just the points where presets live, but the SPACES BETWEEN presets.

This means XOceanus has not 19,859 presets but INFINITE sounds. The preset map is a continuous sound landscape where every position is a valid, musical sound.

**Implementation**:
- Presets embedded as 6D vectors (already computed — Sonic DNA)
- 2D map coordinates are two user-selected DNA axes (Brightness, Warmth, Movement, Density, Space, Aggression) — no projection, no learned embedding
- Real-time: find K-nearest presets to cursor position (k=4, using spatial index)
- Interpolate all parameters using inverse-distance weighting
- Result: smooth, continuous parameter space traversal

**Visual**: Map rendered as a density plot — brighter where presets cluster, darker where space is empty. Mood colors tint regions. Current position shown as the DNA hexagon. Trail shows navigation history.

**CPU cost**: KD-tree lookup for 4 nearest presets: <0.1ms. Parameter interpolation: negligible (weighted average of 4 float arrays).

This is not theoretical — it is immediately implementable with existing data and standard algorithms. It just requires the courage to replace the preset list with a map.

### 5.2.2 The Living Manual — Embedded, Contextual, Beautiful

No synthesizer has ever made its documentation beautiful enough that users WANT to read it. XOceanus embeds the documentation INTO the interface.

When a performer hovers over ANY control for 2 seconds, a gentle tooltip appears — not a grey rectangle with text, but a contextual miniature of the feature's documentation. For a filter knob, it shows:
- The filter curve visualization (live, responding to the current cutoff value)
- One sentence explaining what the parameter does (Inter Regular 11pt)
- The engine-specific behavior note ("In OPERA, cutoff controls formant frequency")
- A "Learn more" link that opens the full field guide entry

This tooltip is glass-morphism: blurred background, engine accent border, 12pt corner radius. It uses the Cubic Glass Gradient aesthetic. It is beautiful enough that a performer might hover over controls just to see the tooltips.

**Implementation**: Tooltip data stored as JSON metadata per parameter. Renderer: custom `juce::BubbleComponent` subclass with glass effect. Trigger: 2-second hover timer. Dismisses on mouse move.

### 5.2.3 Emotion-Responsive UI — The Interface Reads the Music

The most ambitious proposal: XOceanus's interface subtly adapts to the EMOTION of the sound being produced.

When the audio output is analyzed in real-time (spectral centroid, RMS, spectral flatness — data that already exists for the OpticVisualizer), the UI environment shifts:

| Audio Character | UI Response |
|----------------|-------------|
| Bright, loud (high centroid + high RMS) | Background subtly warms (1-2% toward coral) |
| Dark, quiet (low centroid + low RMS) | Background subtly cools (1-2% toward indigo) |
| Complex, noisy (high spectral flatness) | Subtle texture noise appears in panel backgrounds (1% opacity) |
| Pure, tonal (low flatness) | Backgrounds become perfectly clean |
| Rhythmic (transient density > threshold) | Status bar subtle pulse at detected tempo |

These shifts are at 1-3% magnitude — below conscious perception but above subliminal threshold. The performer does not notice that the interface is responding to the music. They notice that the interface FEELS right. The environment matches the sound.

This is embodied cognition applied to software design. It has never been attempted in any application, let alone a synthesizer.

**Implementation**: Audio analysis runs on the audio thread (cheap: centroid, RMS, flatness are O(n) over the buffer). Results stored in `std::atomic<float>` values. UI reads them in `timerCallback()`. Background tint computed as a weighted blend toward target colors. Transition: 2-second smoothing (no sudden shifts).

**CPU cost**: Audio analysis: included in existing OpticVisualizer budget. UI tint: one `fillAll()` with modified color per frame. Negligible.

**Disable**: Settings toggle "Adaptive UI" (default: ON). Reduced motion mode: OFF.

### 5.2.4 The Constellation View — Seeing All 88 at Once

Every synth forces you to look at one engine at a time. XOceanus has 86 engines. What if you could see ALL of them?

The Constellation View is a full-window overlay (triggered by a button in the header or Cmd+Shift+A) that shows all 86 engines as stars in a constellation map. The map layout follows the water column (surface engines at top, abyss engines at bottom). Each star:
- Size: proportional to preset count (popular engines are larger stars)
- Color: engine accent color
- Brightness: proportional to current activity (if the engine is making sound, its star is bright)
- Connections: coupling relationships shown as golden lines between stars

The performer can click any star to load that engine. They can drag between two stars to create a coupling connection. The constellation IS the coupling matrix, rendered as a star chart.

This gives the performer a god's-eye view of the entire XOceanus universe. It is both navigational (find and load engines) and creative (design coupling networks by connecting stars).

No synthesizer has ever shown its entire capability space in a single, beautiful, interactive view.

**Implementation**: 88 positioned circles with SVG creature icons. Layout: hardcoded positions following the water column arrangement from the design guidelines (Section 10.2). Click: loads engine into active slot. Drag between: creates coupling route. Golden lines: same Bezier rendering as coupling visualizer.

**JUCE**: `ConstellationOverlay : public juce::Component`. Full-window overlay, modal. 88 `juce::Component` children (engine stars). Lines painted in parent `paint()`. Fade-in: 300ms.

---

## 5.3 Summary of Innovations

| Innovation | Category | Precedent | Implementation Effort |
|-----------|----------|-----------|----------------------|
| Parameter sensitivity maps | Interface teaches you | None in synths | 2 days (precompute + render) |
| Gesture trail as mod source | Performance → modulation | None in any software | 1 day (data already exists) |
| Spectral silhouettes | Visual engine identity | None | 1 day (offline computation) |
| Spatial preset navigation | Continuous sound space | None (discrete A/B morph exists) | 3 days (DNA-axis map + interpolation + map render) |
| Living manual tooltips | Embedded documentation | None this beautiful | 2 days (tooltip renderer + content) |
| Emotion-responsive UI | Embodied cognition | None in any application | 1 day (audio analysis exists, UI tint trivial) |
| Constellation view | God's-eye navigation | None in synths | 2 days (overlay + star layout) |
| Sound on first launch | Zero-friction onboarding | None in plugin synths | 0.5 day (preset autoload + hover routing) |
| Bidirectional PlaySurface | Surface plays you back | None | 1 day (LFO/coupling → surface visualization) |

---

# APPENDIX A: COMPLETE COLOR REFERENCE

## A.1 Shell Colors

| Token | Light Mode | Dark Mode | Usage |
|-------|-----------|-----------|-------|
| `shellWhite` | `#F8F6F3` | `#1A1A1A` | Main background |
| `surface` | `#FFFFFF` | `#2D2D2D` | Cards, panels |
| `slotBg` | `#FCFBF9` | `#2D2D2D` | Engine slot backgrounds |
| `emptySlot` | `#EAE8E4` | `#363636` | Empty engine slots |
| `borderGray` | `#DDDAD5` | `#4A4A4A` | Subtle borders |
| `textPrimary` | `#1A1A1A` | `#F0EDE8` | Primary text |
| `textSecondary` | `#6B6965` | `#A0A0A0` | Labels, secondary |
| `textDim` | `#BDBBB6` | `rgba(240,237,232,0.20)` | Disabled, placeholder |

## A.2 Brand Constants (Mode-Invariant)

| Token | Value | Usage |
|-------|-------|-------|
| `xoGold` | `#E9C46A` | THE brand color. Coupling strip, macros, actions |
| `xoGoldLight` | `#F4DFA0` | Hover state (light mode) |
| `xoGoldDark` | `#C4A24E` | Pressed state |
| `xoGoldText` | `#7A5C1E` | Gold text on light backgrounds (AA compliant, 5.0:1 on `#F8F6F3`) |

## A.3 Depth Zone Colors

| Zone | Color | Hex | Depth Range |
|------|-------|-----|-------------|
| Sunlit | Bright cyan | `#48CAE4` | 0-55% |
| Twilight | Deep blue | `#0096C7` | 55-80% |
| Midnight | Purple | `#7B2FBE` | 80-95% |
| Abyss | Deep violet | `#150820` | 95-100% |

## A.4 Mood Colors

| Mood | Color | Hex |
|------|-------|-----|
| Foundation | Terracotta | `#C8553D` |
| Atmosphere | Teal | `#2A9D8F` |
| Entangled | Gold | `#E9C46A` |
| Prism | Silver | `#B8C4CC` |
| Flux | Crimson | `#C0392B` |
| Aether | Indigo | `#4A3680` |
| Family | Warm Amber | `#D4A574` |
| Submerged | Deep Marine | `#1B4F72` |
| Coupling | XO Gold | `#E9C46A` |
| Crystalline | Ice Blue | `#B2D8F0` |
| Deep | Trench Violet | `#2D0A4E` |
| Ethereal | Lavender | `#C9B8DB` |
| Kinetic | Electric Orange | `#FF6B35` |
| Luminous | Solar Yellow | `#FFD700` |
| Organic | Moss Green | `#6B8E4E` |

## A.5 Sonic DNA Dimension Colors

| Dimension | Color | Hex |
|-----------|-------|-----|
| Brightness | Gold | `#FFD700` |
| Warmth | Amber | `#E9A84A` |
| Movement | Teal | `#2A9D8F` |
| Density | Violet | `#7B2D8B` |
| Space | Indigo | `#4A3680` |
| Aggression | Crimson | `#C0392B` |

## A.6 Semantic Colors

| Purpose | Color | Hex |
|---------|-------|-----|
| Error | Red | `#E74C3C` |
| Success | Green | `#27AE60` |
| Warning | Amber | `#F39C12` |
| Info | Teal | `#4ECDC4` |

## A.7 PlaySurface Palette

| Element | Hex | Notes |
|---------|-----|-------|
| Surface background | `#1A1A1A` | Dark even in light mode |
| Surface card | `#2D2D2D` | Elevated elements |
| Surface text light | `#EEEEEE` | Primary on dark |
| Surface text dim | `#A0A0A0` | Secondary on dark |
| FIRE pad | `#4ADE80` | Green |
| XOSEND pad | `#F5C97A` | Amber |
| ECHO CUT pad | `#F5C97A` | Amber |
| PANIC pad | `#EF4444` | Red |
| Natural key | `#F0EDE8` | Seaboard white keys |
| Sharp key | `#2A2A2A` | Seaboard black keys |
| Hit flash | `#FFFFFF` at 20% | Pad impact feedback |

## A.8 All 88 Engine Accent Colors

(Full table — see CLAUDE.md Engine Modules table for complete listing. All 86 engines with hex values are defined there and are the canonical reference.)

---

# APPENDIX B: COMPLETE FONT REFERENCE

## B.1 Production Font Stack

| Font | Weights | Usage | Size Reference | Embedding |
|------|---------|-------|----------------|-----------|
| **Space Grotesk** | SemiBold (600), Regular (400) | Window titles (16pt SB), section headers (10pt SB ALL CAPS), tab labels (10-11pt SB), engine names (13pt R) | See type scale below | BinaryData TTF from Google Fonts |
| **Inter** | Regular (400), Medium (500), SemiBold (600) | Body copy (12pt R), labels (10-11pt M), descriptions, tooltips, preset names (13-14pt M) | See type scale below | BinaryData TTF from Google Fonts |
| **JetBrains Mono** | Regular (400) | All numeric values (9-11pt), Hz, dB, BPM, percentages | See type scale below | BinaryData TTF from Google Fonts |

## B.2 Type Scale

| Token | Font | Weight | Size (CSS px) | Size (JUCE pt) | Line Height | Letter Spacing | Usage |
|-------|------|--------|---------------|-----------------|-------------|----------------|-------|
| `display` | Space Grotesk | SemiBold | 28px | 21pt | 32px | 0 | Logo, splash |
| `h1` | Space Grotesk | SemiBold | 20px | 15pt | 24px | 0 | Window title |
| `h2` | Space Grotesk | SemiBold | 16px | 12pt | 20px | 0 | Engine nameplate |
| `h3-caps` | Space Grotesk | SemiBold | 11px | 8.25pt | 14px | +0.08em | Section headers (ALL CAPS) |
| `tab` | Space Grotesk | SemiBold | 11px | 8.25pt | 14px | +0.06em | Tab labels (ALL CAPS) |
| `body` | Inter | Regular | 13px | 9.75pt | 18px | 0 | Descriptions, body text |
| `label` | Inter | Medium | 11px | 8.25pt | 14px | 0 | Form labels, metadata |
| `preset` | Inter | Medium | 14px | 10.5pt | 18px | 0 | Preset names |
| `caption` | Inter | Regular | 11px | 8.25pt | 14px | 0 | Tags, secondary info |
| `micro` | Inter | Medium | 9px | 6.75pt | 12px | +0.1em | Mood badges (ALL CAPS) |
| `numeric` | JetBrains Mono | Regular | 11px | 8.25pt | 14px | 0 | All numeric readouts |
| `numeric-sm` | JetBrains Mono | Regular | 9px | 6.75pt | 12px | 0 | Small value displays |

## B.3 Display Fonts (Site/iOS)

| Font | Usage | Location |
|------|-------|----------|
| **Nebulica** Bold/ExtraBold | XO-OX.org hero headings, engine nameplate alt | `~/Downloads/nebulica_.../OTF/` |
| **Chrys** Regular/Distorted | Avant-garde headings | `~/Downloads/chrys_.../OTF/` |
| **Fonzy** (iOS candidate) | iOS body text — warm, approachable | `~/Downloads/fonzy_.../OTF/` |
| **Polly Rounded** (iOS candidate) | iOS secondary text — friendly | `~/Downloads/Polly_Rounded/` |

## B.4 Case Rules

| Context | Case | Example |
|---------|------|---------|
| Parameter labels | Sentence case | `Filter cutoff` |
| Macro knobs | ALL CAPS | `CHARACTER` |
| Preset names | Title case | `Velvet Morning` |
| Engine names | Exact brand spelling | `XOblong`, `OddfeliX` |
| Gallery codes | ALL CAPS | `OVERDUB`, `OPERA` |
| Mood badges | ALL CAPS + 1.5px spacing | `FOUNDATION` |
| Section headers | ALL CAPS | `OSCILLATOR` |
| Tab labels | ALL CAPS | `XOUIJA` |

---

# APPENDIX C: ASSET CROSS-REFERENCE

This table maps every UI element to its source purchased asset.

| UI Element | Asset | Location | Notes |
|-----------|-------|----------|-------|
| Macro knobs (header) | Knob-Set-09 Filmstrip | `~/Downloads/Audio UI/KNOBS-SET/knob-set-09/` | JUCE-ready filmstrips, 3 sizes |
| Engine param knobs (L2) | Knob-Set-08 Filmstrip | `~/Downloads/Audio UI/KNOBS-SET/knob-set-08/` | Standard size, proper filmstrips |
| Mini param knobs (L3) | Individual KNOBS entries | `~/Downloads/Audio UI/KNOBS/knob-93/` (or similar) | 24x24pt compact |
| Mod wheel texture | Kit-28 Wheel | `~/Downloads/Audio UI/KITS/kit-28/` | Ridged wheel component |
| Illuminated knobs (L1) | Kit-07 Light Knob | `~/Downloads/Audio UI/KITS/kit-07/` | Lit/unlit states for active/inactive |
| Button states | Button-01 (Nickel/Silver) | `~/Downloads/Audio UI/BUTTONS/Button-01/` | Toggle/action buttons |
| Slider tracks | Metal Slider Set 02 | `~/Downloads/Audio UI/SLIDERS/metal-slider-set-02/` | Pitch bend, macro strips |
| VU/level meter | VU Meter 03 (LED segment) | `~/Downloads/Audio UI/VU-METER/vu-meter-03/` | CPU meter, level display |
| Light indicators | Light Set 01 | `~/Downloads/Audio UI/LIGHT/Light-Set-01/` | MIDI indicator, engine slot active |
| Glass panel effect | Cubic Glass Gradient | `~/Downloads/Cubic Glass Gradient/` | Tooltip backgrounds, frosted panels |
| Depth gradients | Galactic Gradients | `~/Downloads/Galactic Gradients/` | Ocean depth zone backgrounds |
| Hero backgrounds | Hero Gradients v1 + v2 | `~/Downloads/Hero Gradients v1/` | Constellation view background |
| Mobile controls ref | Mobile Audio GUI Kit | `~/Downloads/Audio UI/FIGMA-KITS/Mobile_Application_Audio_GUI/` | iOS control sizing/spacing |
| LED display | LEDboard character set | `~/Downloads/LEDboard/` | BPM display, XOverworld engine UI |
| Dashboard patterns | AiDEA SaaS Dashboard | `~/Downloads/AiDEA – Smart SaaS Dashboard UI Kit.fig` | Coupling data visualization |
| Design system ref | Game UX Kit | Figma: `xo643maNsuCFslvd9DaZ7k` | Button state matrix, color methodology |
| Form inputs ref | Lean Mantine Library | Figma: `3jtGAzg3jCdmcgAIhJblYc` | 7-state input system |
| Toast patterns ref | FLOW V.4.0 | Figma: `yzqQkVf2alXAsF4jweZ3Rl` | Notification patterns |
| iOS layout ref | iOS App Wireframes | Figma: `Ch2tom2TeEb73GKDmr7z9j` | Mobile layout patterns |
| HUD overlay ref | UI HUD Kit v1.2.1 | `~/Downloads/UI HUD/` | Performance overlay patterns |
| Holographic texture | Holographic Series 13-24 | `~/Downloads/templify_holographic_13-24_.../` | Premium engine panel backgrounds |
| Glass surfaces ref | Kit-14 | `~/Downloads/Audio UI/KITS/kit-14/` | Glass/translucent surface panels |
| Futuristic UI ref | Samolevsky 200 elements | `~/Downloads/samolevskycom-futuristic-ui-kit-200...` | XOsmosis, XOptic engine chrome |
| Animation icons | Uicon Animated Vol 4 | `~/Downloads/Uicon Animated Icons Vol4/` | Loading, success micro-interactions (web only) |
| Primary icons | HeroIcons (160+) | Wireframing Starter Kit Figma | All UI icons |
| Fallback icons | Feather Icons (280+) | Helio Wireframe Kit Figma | Coverage gaps |
| Studio UI ref | Recording Studio UI Pack | `~/Downloads/ezyzip_.../Recording Studio UI Pack.fig` | Transport controls, meter styling |

---

# APPENDIX D: JUCE IMPLEMENTATION GUIDE

## D.1 Component Hierarchy

```
XOceanusEditor : public juce::AudioProcessorEditor
  |
  +-- HeaderBar : public juce::Component
  |     +-- LogoComponent (SVG Drawable)
  |     +-- DepthZoneDial (custom, see Section 1.5)
  |     +-- PresetNavigator (prev/name/next/heart/DNA)
  |     +-- MacroKnobStrip (4x juce::Slider with custom LookAndFeel)
  |     +-- UtilityControls (dark/light, CPU, MIDI, settings)
  |
  +-- MainContent : public juce::Component
  |     +-- EnginePanel(s) : public juce::Component (1-4 panels)
  |     |     +-- EngineNameplate (Drawable icon + styled label)
  |     |     +-- MacroView (Level 1: 4 large knobs)
  |     |     +-- GroupView (Level 2: collapsible param groups)
  |     |     +-- FullParamView (Level 3: scrollable grid)
  |     |     +-- OscilloscopeView (waveform display)
  |     |     +-- ADSRView (interactive envelope)
  |     |
  |     +-- Sidebar : public juce::Component
  |           +-- SidebarTabBar (PRESET/COUPLE/FX/PLAY)
  |           +-- PresetBrowserPanel
  |           +-- CouplingPanel
  |           |     +-- CouplingVisualizer
  |           |     +-- CouplingCrossfader
  |           |     +-- CouplingTypeSelector
  |           |     +-- CouplingAmountKnob
  |           +-- FXPanel
  |           +-- PlayPanel (sidebar expression controllers)
  |
  +-- PlaySurfaceContainer : public juce::Component, private juce::Timer
  |     +-- PlaySurfaceTabBar
  |     +-- XOuijaComponent
  |     +-- MPCPadGrid
  |     |     +-- PadComponent x16
  |     +-- SeaboardKeyboard
  |     +-- ExpressionPanel
  |     |     +-- ModWheelStrip
  |     |     +-- PitchBendStrip
  |     |     +-- MacroStripGroup (4x MacroStrip)
  |     |     +-- TideController
  |     +-- PerformanceStrip (existing)
  |     +-- PerformancePads (existing)
  |
  +-- StatusBar : public juce::Component
  |     +-- PerformancePadTriggers (FIRE/XOSEND/ECHO/PANIC)
  |     +-- TransportInfo (BPM/voices/CPU)
  |     +-- EngineSlotIndicators (4 dots)
  |
  +-- ConstellationOverlay : public juce::Component (modal, full-window)
```

## D.2 LookAndFeel Architecture

```cpp
class XOceanusLookAndFeel : public juce::LookAndFeel_V4
{
public:
    // === KNOBS ===

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        // Determine knob type from slider properties
        auto* knobType = slider.getProperties()["knobType"].toString();

        if (knobType == "macro")
            drawMacroKnob(g, x, y, w, h, sliderPos, slider);
        else if (knobType == "standard")
            drawStandardKnob(g, x, y, w, h, sliderPos, slider);
        else
            drawMiniKnob(g, x, y, w, h, sliderPos, slider);
    }

private:
    void drawMacroKnob(/* ... */)
    {
        // 1. Draw filmstrip frame from Knob-Set-09
        auto& filmstrip = getMacroFilmstrip();
        int numFrames = 100;
        int frameH = filmstrip.getHeight() / numFrames;
        int frameIdx = juce::roundToInt(sliderPos * (numFrames - 1));
        g.drawImage(filmstrip, x, y, w, h,
                    0, frameIdx * frameH, filmstrip.getWidth(), frameH);

        // 2. Draw XO Gold arc overlay
        juce::Path arc;
        float startRad = juce::degreesToRadians(225.0f); // 7 o'clock
        float endRad = startRad + sliderPos * juce::degreesToRadians(270.0f);
        float cx = x + w * 0.5f, cy = y + h * 0.5f;
        float radius = juce::jmin(w, h) * 0.42f;
        arc.addArc(cx - radius, cy - radius, radius * 2, radius * 2,
                   startRad, endRad, true);
        g.setColour(juce::Colour(0xFFE9C46A));
        g.strokePath(arc, juce::PathStrokeType(3.0f));
    }

    // Standard knob: Knob-Set-08 filmstrip + engine accent arc
    // Mini knob: individual KNOBS/ filmstrip, 24x24, accent arc 2px
};
```

## D.3 OpenGL Context Setup

```cpp
// In XOceanusEditor constructor:
openGLContext.setComponentPaintingEnabled(true);
openGLContext.setContinuousRepainting(false); // We control repaint timing
openGLContext.attachTo(*this);

// GLSL Shaders (compile once in initialise()):
// 1. frostedGlassShader — Gaussian blur + tint for engine panels (feliX energy)
// 2. bioluminescenceShader — radial glow for engine panels (Oscar energy)
// 3. dnaGenerativeShader — procedural hexagon for Sonic DNA visualization
```

## D.4 Thread Safety — Audio to UI Bridge

```cpp
// Pattern: Lock-free FIFO for all audio-reactive visualization

// Audio thread writes:
struct VisualizationData {
    float waveform[256];
    float couplingRMS;
    float spectralCentroid;
    float rms;
    float lfoPhases[4];
};

juce::AbstractFifo fifo { 4 };  // 4-deep ring buffer
VisualizationData data[4];

// In processBlock():
int start1, size1, start2, size2;
fifo.prepareToWrite(1, start1, size1, start2, size2);
if (size1 > 0) {
    data[start1] = currentVisualizationData;
    fifo.finishedWrite(1);
}

// UI thread reads (in timerCallback, 30fps):
fifo.prepareToRead(1, start1, size1, start2, size2);
if (size1 > 0) {
    auto& vizData = data[start1];
    // Update oscilloscope, coupling arcs, emotion-responsive tint
    fifo.finishedRead(1);
}
```

## D.5 Performance Budget

| Component | CPU Target (paint) | Strategy |
|-----------|--------------------|----------|
| Header bar | <0.5ms | Static layout, cached images |
| Engine panel (1 engine) | <1ms | Filmstrip knobs, cached background |
| Engine panel (2 engines) | <1.5ms | Same, two panels |
| Sidebar | <0.5ms | List virtualization for presets |
| XOuija surface | <2ms | Cached gradient background, live Planchette + trails |
| MPC pad grid | <0.5ms | Per-pad dirty-rect repaints |
| Seaboard keyboard | <1ms | Single paint pass, alpha fills |
| Expression controllers | <0.5ms | Simple geometry |
| Coupling visualizer | <0.5ms | Bezier arcs, cached node positions |
| Status bar | <0.2ms | Static layout, periodic text updates |
| Dark Cockpit transitions | 0ms (opacity, GPU) | CSS-like opacity transition |
| **Total per frame** | **<8ms** | Budget: 16.7ms at 60fps, 50% headroom |

## D.6 Resize Strategy

```cpp
void XOceanusEditor::resized()
{
    auto bounds = getLocalBounds();
    int w = bounds.getWidth();
    int h = bounds.getHeight();

    // Golden ratio proportions
    constexpr float phi = 1.618f;
    int headerH = 52;
    int statusH = 52;
    int remainH = h - headerH - statusH;
    int playH = (int)(remainH / (1.0f + phi)); // ~37% for PlaySurface
    int mainH = remainH - playH;                // ~63% for engine + sidebar

    int engineW = (int)(w / phi);    // ~62% for engine panel
    int sidebarW = w - engineW;      // ~38% for sidebar

    headerBar.setBounds(0, 0, w, headerH);
    mainContent.setBounds(0, headerH, w, mainH);
    playSurface.setBounds(0, headerH + mainH, w, playH);
    statusBar.setBounds(0, h - statusH, w, statusH);

    // Within mainContent:
    enginePanel.setBounds(0, 0, engineW, mainH);
    sidebar.setBounds(engineW, 0, sidebarW, mainH);

    // Fonts scale with component height
    // e.g., nameplateFontSize = enginePanel.getHeight() * 0.048f
}
```

---

# APPENDIX E: ANIMATION TIMING BIBLE

Every animation in XOceanus is specified here. No animation exists outside this list. If it is not here, it does not animate.

## E.1 UI Transitions

| Element | Trigger | Duration | Easing | Reduced Motion |
|---------|---------|----------|--------|----------------|
| Engine panel load | Engine switch | 200ms | ease-out | Instant |
| Preset switch | Preset load | 150ms | ease-out (crossfade values) | Instant |
| Coupling strip appear | 2nd engine loaded | 250ms | ease-out (height expand from 0) | Instant |
| Mood tab switch | Mood pill click | 200ms | ease-out (slide) | Instant |
| Sidebar tab switch | Tab click | 150ms | ease-out | Instant |
| Panel expand/collapse | Group header click | 200ms | ease-out | Instant |
| Dark Cockpit dim | Performance input | 400ms | ease-in | Instant |
| Dark Cockpit restore | No input 3s | 600ms | ease-out | Instant |
| Modal open | Button click | 300ms | spring (damping 0.85) | Instant |
| Modal close | Escape/click outside | 200ms | ease-in | Instant |
| Constellation overlay | Cmd+Shift+A | 300ms | ease-out (fade + scale from 0.95) | Instant |
| Toast appear | Event trigger | 200ms | ease-out (slide up + fade) | Instant |
| Toast dismiss | Auto (4s) or swipe | 300ms | ease-in (slide down + fade) | Instant |

## E.2 Performance Animations (30fps timer-driven)

| Element | Behavior | Duration/Rate | Reduced Motion |
|---------|----------|---------------|----------------|
| Planchette idle drift | Lissajous path | 0.3Hz / 0.2Hz | Static at center |
| Planchette lock-on | Snap to touch | 150ms, cubic-bezier(0.34, 1.56, 0.64, 1) | Instant |
| Planchette release | Hold + resume drift | 400ms hold, 1s ease-in to drift | Instant fade |
| Bioluminescent trail | Point trail decay | 1.5s per point | Disabled |
| Touch ripple | 3 expanding rings | 300ms | Single flash, 50ms |
| Pad hit flash | White flash + fill | 50ms flash, 200ms decay | 50ms flash only |
| Pad warm memory | Ghost ring decay | 1.5s | Disabled |
| Velocity heatmap | Persistent fill decay | 4 beats at host tempo | Persistent, no decay |
| Seaboard key press | Accent fill at pressure | Immediate (per-frame alpha) | Immediate |
| Seaboard glide trail | Connecting trail | 200ms fade | Disabled |
| Mod wheel ghost trail | Direction trail | 500ms fade | Disabled |
| Pitch bend spring | Return to center | Per frame: `pos += (0.5 - pos) * 0.15` | Instant return |
| Tide Controller water | Wave equation | 30fps, 256 ops/frame | Static center |

## E.3 Continuous Animations (Global Timer)

| Element | Formula | Rate | Reduced Motion |
|---------|---------|------|----------------|
| Breathing function | `(e^sin(t) - 1/e) / (e - 1/e)` | 4s period (t = 2*PI / (4 * fps) per frame) | Static (midpoint value) |
| Coupling arc glow | `alpha = 0.7 + 0.3 * couplingRMS` | Tied to audio analysis | Static 0.7 |
| Coupling arc displacement | Perlin noise, 0.5Hz | Per frame | Disabled |
| Macro pulse (when modulated) | `alpha = 0.6 + 0.4 * sin(t * couplingRate)` | Matches mod rate | Static 0.8 |
| DNA hexagon ripple | Perlin vertex displacement, 0.3Hz | Per frame | Static shape |
| DNA hexagon breathing | `scale = 1.0 + 0.02 * sin(t * 0.8)` | Per frame | Scale = 1.0 |
| MIDI indicator flash | Engine accent, decay | 50ms on, 200ms decay to off | 50ms on, instant off |
| Engine slot active glow | Accent color + subtle pulse | 2s breathing cycle | No pulse |
| Emotion-responsive tint | Background color shift, 1-3% | 2s smoothing | Disabled |

## E.4 Spring Physics Parameters

| Element | Stiffness (k) | Damping (d) | Mass (m) | Notes |
|---------|--------------|-------------|----------|-------|
| Macro knob tracking | 0.12 | 0.85 | 1.0 | Weighted brass feel — smooth follow |
| Performance pad hit | 0.35 | 0.70 | 0.6 | Rubberized compression, 2px depth |
| Pitch bend return | 0.15 | 0.95 | 1.0 | Critically damped, no oscillation |
| Planchette follow | 0.85 | 0.92 | 0.3 | Light mass, responsive with slight lag |
| Drawer slide | spring damping 0.85 | — | — | `UISpringTimingParameters` (iOS) |

All spring physics: `velocity += (target - current) * stiffness; velocity *= damping; current += velocity;` computed per frame at 30fps (PlaySurface timer) or 60fps (UI timer).

---

# APPENDIX F: IMPLEMENTATION PRIORITY

| Phase | Components | Effort | Impact |
|-------|-----------|--------|--------|
| Phase 0 | `XOceanusLookAndFeel` (knob filmstrips, fonts, colors) | 2 days | Foundation for everything |
| Phase 1 | Header bar + Status bar | 2 days | Frame establishes identity |
| Phase 2 | Engine panel (Level 1 macro view) | 2 days | Sound manipulation works |
| Phase 3 | Sidebar (Preset tab only) | 3 days | Preset browsing works |
| Phase 4 | PlaySurface container + MPC Pad Grid | 3 days | Playing works |
| Phase 5 | XOuija with Planchette | 3 days | Signature feature |
| Phase 6 | Expression controllers (mod wheel, pitch bend, macros) | 2 days | Expression works |
| Phase 7 | Dark Cockpit system | 1 day | Attentional design activates |
| Phase 8 | Coupling tab + CouplingVisualizer | 2 days | Coupling visible |
| Phase 9 | Seaboard keyboard | 3 days | MPE works |
| Phase 10 | Tide Controller | 1 day | Novel expression |
| Phase 11 | Sonic DNA hexagons | 1 day | Visual preset identity |
| Phase 12 | Progressive disclosure (Level 2, Level 3) | 2 days | Full depth |
| Phase 13 | FX tab | 1 day | FX management |
| Phase 14 | Sound on First Launch | 0.5 day | Onboarding magic |
| Phase 15 | Constellation View | 2 days | God's-eye navigation |
| Phase 16 | Gesture trail as mod source | 1 day | Innovation #1 |
| Phase 17 | Spatial preset navigation | 3 days | Innovation #2 |
| Phase 18 | Emotion-responsive UI | 1 day | Innovation #3 |
| Phase 19 | Living manual tooltips | 2 days | Innovation #4 |
| Phase 20 | iPad adaptation | 3 days | Mobile platform |
| Phase 21 | iPhone adaptation | 2 days | Mobile simplification |
| **Total** | | **~40 days** | |

---

*This document supersedes all previous UI specifications for XOceanus. It is the single source of truth for interface implementation. Every pixel described here has been considered in context of the purchased assets, the existing codebase, the aquatic mythology, the 88-engine fleet, and the mandate to surpass every synthesizer interface ever created.*

*The interface is the instrument. The instrument is the interface. The mythology is the medium.*

*XOceanus — for all.*
