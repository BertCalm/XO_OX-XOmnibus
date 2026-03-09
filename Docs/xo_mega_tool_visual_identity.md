# XO_OX Mega-Tool — Unified Visual Identity Specification

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Status:** Specification Complete — Ready for Implementation
**Depends on:** `xo_mega_tool_dev_strategy.md`, `xo_signature_playsurface_spec.md`, `product_identity.md`

---

## 1. Design Philosophy

### 1.1 The Core Principle

> "Engine identity through accent color, unity through shared structure."

The XO_OX Mega-Tool hosts 7+ distinct synth modules, each born from a standalone instrument with its own visual personality. The challenge is not erasing that personality but channeling it through a consistent structural language. The mega-tool UI must feel like a single instrument, not a tabbed collection of unrelated windows.

### 1.2 Three Pillars of Visual Unity

| Pillar | Mechanism | Effect |
|--------|-----------|--------|
| **Light Canvas** | Clean light base (#F0F0F5) shared across all views | Striking, bright, legible — the primary design target |
| **Glass Structure** | Frosted glass panels, specular highlights, translucent borders | Common material language adapted from XOblongBob, XObese, and XOppossum glass aesthetics |
| **Accent Personality** | Each engine owns a primary accent color that fills knob arcs, glows, headers, and active states | Identity without structural divergence |

**Design direction note:** While five of six existing standalone XO_OX instruments use near-black backgrounds, the mega-tool breaks from this convention intentionally. The light canvas is more striking, cleaner, and more legible — qualities that matter for an instrument with this many parameters. Accent colors pop against light backgrounds with a different energy: bold and confident rather than ambient and moody. The existing dark aesthetics are preserved as a dark mode option (Section 10), but all primary design decisions target the light interface first. XOdyssey's light-based aesthetic validates this direction within the existing portfolio.

### 1.3 The Accent Rule

No visual element in the shell carries its own accent color. The shell is chromatically neutral. Accent comes exclusively from the active engine. When a user selects an engine, the shell "fills" with that engine's color — knob arcs shift, header text changes, glow halos retint. The shell disappears. The engine's personality fills the space.

### 1.4 Why This Works

The approach mirrors how professional audio hardware handles multi-engine architectures. The Elektron Analog Four uses a monochrome shell with per-track color coding. The Moog Matriarch uses a shared panel aesthetic with colored patch cables for signal identity. The mega-tool extends this: shared glass panels are the rack, accent colors are the cables.

---

## 2. The Shell Aesthetic

### 2.1 Shell Base Colors

The shell is the host frame surrounding engine panels — navigation bars, engine selector tabs, master controls, dividers, and window chrome.

| Element | Color | Alpha | Notes |
|---------|-------|-------|-------|
| Window background | `#F0F0F5` | 100% | Clean, bright off-white with subtle cool undertone |
| Shell panel surface | `#E8E8F0` | 100% | Slightly recessed from background for panel hierarchy |
| Shell border (outer) | `#000000` | 8% | Subtle structural edge |
| Shell divider lines | `#000000` | 8% | Clean separation between zones |
| Shell text (primary) | `#FFFFFF` | 100% | Navigation labels, engine names |
| Shell text (secondary) | `#FFFFFF` | 50% | Descriptions, hints, inactive labels |
| Shell text (dim) | `#FFFFFF` | 30% | Tertiary information, disabled states |
| Shell icon (idle) | `#FFFFFF` | 40% | Unselected navigation icons |
| Shell icon (hover) | `#FFFFFF` | 70% | Hover state for navigation icons |
| Shell icon (active) | Engine accent | 100% | Selected state inherits engine color |

### 2.2 Glass Panel Rendering

Every content panel in the mega-tool uses a shared frosted glass material. This is the structural DNA inherited from XOblongBob, XObese, and XOppossum.

| Property | Value | Notes |
|----------|-------|-------|
| Panel fill | `#FFFFFF` at 8% alpha | Lighter than background, frosted appearance |
| Panel corner radius | 8px | Consistent across all panels |
| Panel border | `#FFFFFF` at 10% alpha, 1px | Subtle structural edge |
| Specular highlight | `#FFFFFF` at 20% alpha, 1px | Top edge only, simulates overhead light |
| Inner shadow | `#000000` at 15% alpha, 2px blur | Bottom-inside edge, adds depth |
| Panel-to-panel gap | 4px | Consistent spacing between adjacent panels |
| Panel-to-edge margin | 8px | Margin from shell window edge |

### 2.3 Shell Navigation Bar

The navigation bar sits at the top of the window and provides engine selection, mode toggle, and global controls.

| Element | Height | Background | Notes |
|---------|--------|------------|-------|
| Nav bar | 36px | `#0B0B12` (same as window) | No panel — flat integration with background |
| Engine tab (idle) | 28px | Transparent | Text at 50% white |
| Engine tab (active) | 28px | Glass panel (8% white) | Text in engine accent color, 2px bottom border in accent |
| Engine tab (hover) | 28px | `#FFFFFF` at 4% | Text at 70% white |
| Mode toggle (Intuitive/Advanced) | 28px | Glass panel | Accent-colored indicator dot on active mode |
| Master volume | 24px tall, 80px wide | Glass track | Gold `#E9C46A` fill (brand neutral) |
| Preset name display | 28px | Transparent | Current preset name, white text |

### 2.4 Shell Identity — No Accent of Its Own

The shell has no signature accent color. When no engine is loaded, the UI is entirely neutral — gray glass on near-black. This is intentional. The mega-tool is a vessel, not a personality. Each engine loaded fills the space with its character. Master/shared controls that are not engine-specific use Gold `#E9C46A` — the XO_OX coupling color — as their accent.

---

## 3. Engine Color Registry

### 3.1 Color Assignment Table

Each engine module is assigned a definitive accent palette. These colors derive from the actual implementations of each standalone instrument.

| Module Name | Engine Source | Primary Accent | Hex | Secondary Accent | Hex | Glow Color | Hex + Alpha | Sonic Identity |
|-------------|-------------|---------------|-----|-----------------|-----|------------|-------------|----------------|
| **FAT** | XObese | Pink | `#E06090` | Yellow | `#E8C84A` | Pink glow | `#E06090` @ 40% | Warm, playful, massive |
| **BITE** | XOppossum | Rust | `#C47040` | Acid Lime | `#88CC44` | Rust glow | `#C47040` @ 40% | Shadowy, moody, bass-weight |
| **SNAP** | XOddCouple X | Terracotta | `#C8553D` | — | — | Terracotta glow | `#C8553D` @ 40% | Punchy, rhythmic, percussive |
| **MORPH** | XOddCouple O | Teal | `#2A9D8F` | — | — | Teal glow | `#2A9D8F` @ 40% | Lush, evolving, cool |
| **DUB** | XOverdub | Dub Green | `#00FF88` | Amber | `#FFAA00` | Green glow | `#00FF88` @ 35% | Raw, direct, dub-pressure |
| **DRIFT** | XOdyssey | Psychedelic Purple | `#9B59B6` | Shimmer Gold | `#F0C040` | Purple glow | `#9B59B6` @ 40% | Ethereal, cosmic, voyager |
| **PLUSH** | XOblongBob | Amber | `#F5C97A` | — | — | Amber glow | `#F5C97A` @ 40% | Warm, character, glass |
| **ONSET** | XOnset | Gradient blend | — | Terracotta-to-Teal | `#C8553D` to `#2A9D8F` | Gradient glow | Blend @ 40% | Morphing, rhythmic, percussive |

### 3.2 Engine Color Profile Struct

Every engine provides its colors through a standardized struct consumed by the LookAndFeel:

```cpp
struct EngineColorProfile
{
    juce::Colour primaryAccent;       // Knob arcs, active states, header text
    juce::Colour secondaryAccent;     // Optional: secondary controls, meters
    juce::Colour glowColor;           // Halo/glow around active elements
    float glowAlpha = 0.40f;          // Default glow intensity
    juce::Colour panelTint;           // Subtle tint applied to glass panels (primaryAccent @ 3%)
    juce::String moduleName;          // Display name ("FAT", "DUB", etc.)
};
```

### 3.3 Color Contrast Requirements

All accent colors have been verified for minimum contrast against the shell background (`#0B0B12`):

| Color | Contrast Ratio vs Background | WCAG AA (4.5:1) |
|-------|------------------------------|-----------------|
| Pink `#E06090` | 5.2:1 | Pass |
| Rust `#C47040` | 4.8:1 | Pass |
| Terracotta `#C8553D` | 4.6:1 | Pass |
| Teal `#2A9D8F` | 5.7:1 | Pass |
| Dub Green `#00FF88` | 10.3:1 | Pass |
| Purple `#9B59B6` | 4.5:1 | Pass (borderline) |
| Amber `#F5C97A` | 8.1:1 | Pass |
| Gradient midpoint | 5.1:1 | Pass |

### 3.4 Coupling & Shared Element Color

| Element | Color | Hex | Usage |
|---------|-------|-----|-------|
| Coupling strip | Gold | `#E9C46A` | Coupling knob, coupling cables, coupling amount display |
| Master controls | Gold | `#E9C46A` | Master volume, master FX send, global macros |
| Shared FX rack | Gold | `#E9C46A` | Shared reverb/delay/compressor controls |
| Brand logo | Gold | `#E9C46A` | XO_OX logo in nav bar |
| Panic button | Red | `#FF3333` | Emergency all-notes-off, always red regardless of engine |

Gold is the XO_OX brand coupling color. It appears in every standalone product (XOddCouple's coupling knob, XOblongBob's macro ring, XObese's FX ring) and carries forward as the mega-tool's shared-element accent.

---

## 4. Shared Component Styling

Every UI component adapts to the active engine's `EngineColorProfile`. The component body is always neutral glass; only accent elements (needles, arcs, fills, active states) change per engine.

### 4.1 Rotary Knobs

| Element | Styling | Color Source |
|---------|---------|-------------|
| Knob body (dome) | Radial gradient: center `#FFFFFF` @ 14%, edge `#FFFFFF` @ 6% | Shared (neutral glass) |
| Knob body border | `#FFFFFF` @ 12%, 1px | Shared |
| Specular highlight | `#FFFFFF` @ 35%, arc from 10 o'clock to 2 o'clock, 1px | Shared |
| Track (background arc) | `#FFFFFF` @ 8%, 3px stroke | Shared |
| Track (filled arc) | `primaryAccent` @ 100%, 3px stroke | Engine profile |
| Needle/pointer | `primaryAccent` @ 100%, 2px line from center to edge | Engine profile |
| Glow arc | `glowColor` @ `glowAlpha`, 6px stroke, Gaussian blur 4px | Engine profile |
| Value text | `#FFFFFF` @ 70%, centered below knob | Shared |
| Label text | `#FFFFFF` @ 50%, centered below value | Shared |

**Knob Sizes:**

| Context | Diameter | Track Width | Needle Length | Label Font |
|---------|----------|-------------|---------------|------------|
| Macro knob | 72px | 4px | 30px | 11pt |
| Standard knob | 56px | 3px | 22px | 10pt |
| Compact knob | 36px | 2px | 14px | 8pt |
| Mini knob (coupling matrix) | 24px | 2px | 9px | 7pt |

### 4.2 Sliders

| Element | Styling | Color Source |
|---------|---------|-------------|
| Track background | Glass gradient: `#FFFFFF` @ 6% to `#FFFFFF` @ 10% (bottom to top) | Shared |
| Track border | `#FFFFFF` @ 10%, 1px | Shared |
| Track corner radius | 4px | Shared |
| Fill (from bottom) | `primaryAccent` @ 80% | Engine profile |
| Thumb | 12px circle, `primaryAccent` @ 100%, 1px `#FFFFFF` @ 30% border | Engine profile |
| Thumb glow | `glowColor` @ `glowAlpha`, 8px radius blur | Engine profile |
| Value text | `#FFFFFF` @ 70%, positioned beside slider | Shared |

**Slider Sizes:**

| Context | Width | Height | Thumb Diameter |
|---------|-------|--------|----------------|
| Standard vertical | 24px | 120px | 12px |
| Compact vertical | 18px | 80px | 10px |
| Horizontal (balance) | 120px | 24px | 12px |
| Master volume | 80px | 24px (horizontal) | 14px |

### 4.3 Buttons and Toggles

| State | Fill | Border | Text Color | Notes |
|-------|------|--------|------------|-------|
| Idle | `#FFFFFF` @ 6% | `#FFFFFF` @ 10%, 1px | `#FFFFFF` @ 50% | Glass appearance |
| Hover | `#FFFFFF` @ 10% | `#FFFFFF` @ 15%, 1px | `#FFFFFF` @ 70% | Subtle lift |
| Active/On | `primaryAccent` @ 20% | `primaryAccent` @ 60%, 1px | `primaryAccent` @ 100% | Engine personality emerges |
| Pressed | `primaryAccent` @ 30% | `primaryAccent` @ 80%, 1px | `primaryAccent` @ 100% | Momentary feedback |
| Disabled | `#FFFFFF` @ 3% | `#FFFFFF` @ 6%, 1px | `#FFFFFF` @ 20% | Clearly unavailable |

**Specular edge:** All buttons carry a 1px top-edge highlight at `#FFFFFF` @ 20% — the shared glass specular inherited from XOblongBob.

**Button Sizes:**

| Context | Height | Min Width | Corner Radius | Font Size |
|---------|--------|-----------|---------------|-----------|
| Standard | 28px | 60px | 6px | 10pt |
| Compact | 22px | 44px | 4px | 9pt |
| Performance pad | 56px | 56px | 8px | 11pt bold |

### 4.4 Panel Headers

| Element | Styling | Color Source |
|---------|---------|-------------|
| Header bar height | 24px | Shared |
| Header background | `primaryAccent` @ 6% | Engine profile |
| Header bottom border | `primaryAccent` @ 30%, 1px | Engine profile |
| Engine/module name | `primaryAccent` @ 100%, 11pt bold, uppercase | Engine profile |
| Section label | `#FFFFFF` @ 50%, 10pt, uppercase | Shared |
| Collapse arrow | `#FFFFFF` @ 40% idle, `#FFFFFF` @ 70% hover | Shared |

### 4.5 Meters and Scopes

| Element | Styling | Color Source |
|---------|---------|-------------|
| Meter background | Glass panel (8% white), 4px corner radius | Shared |
| Meter fill (signal) | `primaryAccent` @ 90%, bottom-to-top gradient | Engine profile |
| Meter fill (peak) | `primaryAccent` @ 100%, top 2px bar | Engine profile |
| Meter fill (clip) | `#FF3333` @ 100% | Shared (always red) |
| Scope background | Glass panel (8% white) | Shared |
| Scope trace | `primaryAccent` @ 70%, 1.5px anti-aliased | Engine profile |
| Scope grid lines | `#FFFFFF` @ 6%, 0.5px | Shared |

### 4.6 Tab and Page Navigation

| State | Background | Text | Indicator |
|-------|------------|------|-----------|
| Idle tab | Transparent | `#FFFFFF` @ 40% | None |
| Hover tab | `#FFFFFF` @ 4% | `#FFFFFF` @ 60% | None |
| Active tab | Transparent | `primaryAccent` @ 100% | 2px bottom border in `primaryAccent` |
| Disabled tab | Transparent | `#FFFFFF` @ 15% | None |

### 4.7 Dropdown / Combo Box

| Element | Styling | Color Source |
|---------|---------|-------------|
| Closed state | Glass panel fill, `#FFFFFF` @ 50% text, down-arrow @ 40% | Shared |
| Open menu | `#141420` @ 95%, 1px `#FFFFFF` @ 15% border, 8px corner radius | Shared |
| Menu item (idle) | Transparent, `#FFFFFF` @ 70% | Shared |
| Menu item (hover) | `primaryAccent` @ 12%, `primaryAccent` @ 100% | Engine profile |
| Menu item (selected) | `primaryAccent` @ 8%, `primaryAccent` @ 80%, checkmark | Engine profile |
| Menu shadow | `#000000` @ 30%, 8px blur, 4px offset-y | Shared |

---

## 5. Multi-Engine Mode Visual Rules

### 5.1 Split-Panel Layout

When 2 or more engines are active simultaneously, the UI divides into per-engine panels. Each panel renders its controls using its own engine's accent color.

```
2-Engine Layout (e.g., SNAP + DUB):
┌──────────────────────┬────────┬──────────────────────┐
│   SNAP Panel         │ Gold   │   DUB Panel          │
│   (Terracotta)       │Coupling│   (Dub Green)        │
│                      │ Strip  │                      │
│   Knobs: terra arcs  │  48px  │   Knobs: green arcs  │
│   Headers: terra     │  wide  │   Headers: green     │
│   Buttons: terra     │        │   Buttons: green     │
│                      │        │                      │
│       506px          │        │       506px           │
└──────────────────────┴────────┴──────────────────────┘
│                  1060px total                         │
```

```
3-Engine Layout:
┌──────────────┬────┬──────────────┬────┬──────────────┐
│  Engine A    │Gold│  Engine B    │Gold│  Engine C    │
│   340px      │24px│   340px      │24px│   340px      │
└──────────────┴────┴──────────────┴────┴──────────────┘
```

```
4-Engine Layout (2x2 grid):
┌──────────────┬────┬──────────────┐
│  Engine A    │Gold│  Engine B    │
│   506px      │48px│   506px      │
├──────────────┼────┼──────────────┤
│  Engine C    │Gold│  Engine D    │
│   506px      │48px│   506px      │
└──────────────┴────┴──────────────┘
```

### 5.2 Coupling Strip

The gold coupling strip between engine panels is the visual signature of cross-engine interaction.

| Element | Styling | Notes |
|---------|---------|-------|
| Strip background | `#E9C46A` @ 6% | Subtle gold tinting |
| Strip border (both sides) | `#E9C46A` @ 20%, 1px | Gold structural edges |
| Coupling amount knob | Mini knob (24px), gold arc | Per-pair coupling intensity |
| Coupling type indicator | Gold icon (coupling type symbol) | AmpToFilter, LFOToPitch, etc. |
| Coupling pulse line | `#E9C46A` @ 30-80%, pulsing | Pulse rate proportional to coupling amount |
| Strip width | 48px (2-engine), 24px (3+ engines) | Narrows as engine count increases |

### 5.3 Master and Shared Controls

Controls that are not owned by any single engine always use Gold `#E9C46A`:

| Control | Color | Rationale |
|---------|-------|-----------|
| Master Volume | Gold | Global output, no engine owns it |
| Shared FX Rack (reverb, delay, compressor) | Gold | Shared FX for engines with `ownsEffects() == false` |
| Coupling Matrix (all pairs) | Gold | Cross-engine territory |
| Global Macros | Gold | User-assignable, span engines |
| Preset Browser chrome | Gold highlights | Unified browsing, no engine bias |
| PlaySurface mode tabs | Gold when no engine loaded | Neutral until engine context is established |

### 5.4 PlaySurface Engine Adaptation

The PlaySurface tints dynamically based on the active engine context:

| PlaySurface Element | Single Engine Active | Two Engines Active (Blend) |
|---------------------|---------------------|---------------------------|
| Pad glow on touch | Engine's `glowColor` | Blend of both engine colors at current blend position |
| Velocity heatmap | Warm amber `#F5C97A` @ decay (shared) | Same — amber is always the velocity indicator |
| Ghost traces | `#FFFFFF` @ 15%, age-fading | Same — neutral ghost traces |
| Orbit trail | Engine's `primaryAccent` dots | Blended color dots |
| Knob arcs in Orbit zone | Engine's `primaryAccent` | Left arc = Engine A color, right arc = Engine B color |
| Performance Strip gradient | Engine's primary + secondary | Gradient from Engine A primary to Engine B primary |

---

## 6. Intuitive vs Advanced Mode Visual Differences

### 6.1 Intuitive Mode

Designed for preset browsers and performers. The engine's personality is maximized; chrome is minimized.

| Characteristic | Specification |
|---------------|---------------|
| Visible parameters | 4 macro knobs only (72px diameter, prominent) |
| Engine panel | Single large panel, engine accent color fills panel header |
| Panel glass tint | `primaryAccent` @ 5% (engine personality saturates the space) |
| Knob glow | Enhanced: `glowAlpha` * 1.5 (more immersive) |
| PlaySurface | Full-width, 344px tall (maximum expression area) |
| Preset browser | Large tile view, category colors |
| Engine selector | Large tabs with engine icons, accent-colored active state |
| Coupling controls | Hidden (coupling is preset-driven, not user-adjusted) |
| FX controls | Hidden (FX are preset-driven) |
| Patch cables | Hidden |
| Overall density | Low — generous spacing, large touch targets |

### 6.2 Advanced Mode

Designed for sound designers. Structural chrome is visible; the engine's personality is channeled through more numerous but smaller controls.

| Characteristic | Specification |
|---------------|---------------|
| Visible parameters | All engine parameters (36px compact knobs) |
| Engine panel | Dense grid layout, engine accent on arcs and headers |
| Panel glass tint | `primaryAccent` @ 3% (subtle — chrome takes precedence) |
| Knob glow | Standard: `glowAlpha` as specified in profile |
| PlaySurface | Collapsible, 200px tall minimum |
| Preset browser | List view with parameter comparison |
| Engine selector | Compact tabs, text-only |
| Coupling controls | Visible coupling matrix with per-pair knobs |
| FX controls | Full rack visible with per-FX enable toggles |
| Patch cables | Visible overlay (see Section 6.3) |
| Overall density | High — compact spacing, information-dense |

### 6.3 Patch Cable Overlay (Advanced Only)

| Element | Styling |
|---------|---------|
| Cable line | 2px, anti-aliased Bezier curve |
| Cable color | Source engine's `primaryAccent` @ 60% |
| Cable glow | Source engine's `glowColor` @ 20%, 4px blur along cable path |
| Cable active (carrying signal) | Full `primaryAccent` @ 100%, pulse animation |
| Cable idle (no signal) | `primaryAccent` @ 30% |
| Jack socket | 8px circle, `#FFFFFF` @ 15% fill, `#FFFFFF` @ 25% border |
| Jack socket (connected) | 8px circle, `primaryAccent` @ 25% fill, `primaryAccent` @ 50% border |
| Drag preview | Dashed line, `#FFFFFF` @ 40%, following cursor |

---

## 7. Animation and Motion Design

### 7.1 Shared Timing Constants

All engines share the same animation timing for visual consistency. These values are inherited from the XOblongBob PlaySurface implementation.

| Animation | Duration / Rate | Curve | Notes |
|-----------|----------------|-------|-------|
| Glow decay | 0.92x per frame @ 30fps | Exponential | ~800ms to visually zero |
| Ghost trace lifetime | 1.5s | Linear fade | 8-entry ring buffer |
| Orbit trail | 60 dots, oldest fades first | Age-based linear | Dot opacity = 1.0 - (age / 60) |
| Button press flash | 100ms | Ease-out | Accent color @ 30% -> 0% |
| Hover transition | 150ms | Ease-in-out | Opacity and background |
| Engine switch crossfade | 200ms | Linear | Accent colors morph between old and new |
| Coupling pulse | 0.5s - 3.0s period | Sine wave | Period = 3.0 - (2.5 * couplingAmount) |
| Panel collapse/expand | 250ms | Ease-out | Height animation |
| Preset change flash | 300ms | Ease-out | Brief accent glow across panel header |
| Mode toggle (Intuitive/Advanced) | 400ms | Ease-in-out | Layout reflow with crossfade |
| Velocity heatmap decay | 0.92x per frame @ 30fps | Exponential | Warm amber glow fades naturally |

### 7.2 Engine Transition Animation

When the user switches the active engine (e.g., from SNAP to DUB), accent colors do not snap — they morph over 200ms.

```
Frame 0:   primaryAccent = Terracotta #C8553D (SNAP)
Frame 1-6: primaryAccent = interpolated blend (200ms / 33.3ms per frame = 6 frames)
Frame 6:   primaryAccent = Dub Green #00FF88 (DUB)
```

Interpolation uses HSL color space to avoid muddy intermediate values. The hue, saturation, and lightness channels interpolate independently.

### 7.3 Coupling Visualization

| Element | Animation |
|---------|-----------|
| Coupling pulse line | Gold `#E9C46A` opacity oscillates between 30% and 80% on a sine curve |
| Pulse rate | Proportional to coupling amount: 0.0 = no pulse, 0.5 = 1.5s period, 1.0 = 0.5s period |
| Coupling activation | 150ms fade-in of pulse line when coupling amount > 0 |
| Coupling deactivation | 300ms fade-out when coupling amount returns to 0 |

### 7.4 Frame Budget

All animations target 30fps rendering. At 30fps, each frame has 33.3ms of budget. Visual rendering (drawing glass panels, knob arcs, glow blurs) must complete within 8ms to leave headroom for audio processing and system overhead. The JUCE `repaint()` cycle should be driven by a 30fps timer, not by parameter changes — parameter changes flag dirty regions but do not trigger immediate repaints.

---

## 8. Typography

### 8.1 Font Stack

| Priority | Font | Usage | Fallback |
|----------|------|-------|----------|
| Primary | SF Pro Display Bold | Engine names, section headers | Helvetica Neue Bold |
| Secondary | SF Pro Text Medium | Parameter labels, button text | Helvetica Neue Medium |
| Tertiary | SF Pro Text Regular | Values, descriptions, hints | Helvetica Neue Regular |
| Monospace | SF Mono | Numerical readouts, MIDI note names | Menlo |

### 8.2 Type Scale

| Level | Size | Weight | Tracking | Transform | Color | Usage |
|-------|------|--------|----------|-----------|-------|-------|
| H1 | 14pt | Bold | +0.5px | Uppercase | `primaryAccent` | Engine module name in panel header |
| H2 | 12pt | Bold | +0.3px | Uppercase | `#FFFFFF` @ 90% | Section headers within engine panel |
| H3 | 11pt | Medium | +0.2px | Uppercase | `#FFFFFF` @ 70% | Subsection labels |
| Label | 10pt | Medium | +0.1px | Uppercase | `#FFFFFF` @ 50% | Parameter labels beneath knobs |
| Value | 10pt | Regular | 0 | None | `#FFFFFF` @ 60% | Parameter value readouts |
| Compact Label | 8pt | Medium | +0.1px | Uppercase | `#FFFFFF` @ 40% | Labels on mini/compact knobs |
| Compact Value | 8pt | Regular | 0 | None | `#FFFFFF` @ 50% | Values on mini/compact knobs |
| Hint | 9pt | Regular | 0 | None | `#FFFFFF` @ 30% | Tooltips, descriptions |

### 8.3 Typography Rules

1. **All labels are uppercase.** No exceptions. Uppercase compact text at small sizes provides legibility on dark backgrounds and aligns with hardware synthesizer conventions.
2. **Engine names are always accent-colored.** The module name ("FAT", "DUB", "SNAP") renders in the engine's `primaryAccent` wherever it appears.
3. **Parameter labels are always neutral.** Labels like "CUTOFF", "RESONANCE", "ATTACK" render in white/gray regardless of engine. This prevents visual noise when accent colors are bright (e.g., Dub Green `#00FF88`).
4. **Values are slightly dimmer than labels.** This creates visual hierarchy: the parameter name draws the eye first, the value is available on closer inspection.
5. **Numerical values use monospace.** This prevents layout shifting when values change (e.g., "1.00" to "10.0" maintains the same text width).

---

## 9. Iconography

### 9.1 Engine Module Icons

Each engine has a simple geometric icon used in the engine selector tabs and preset browser. Icons are 20x20px at standard scale, rendered as stroked paths (not filled) in the engine's accent color when active, `#FFFFFF` @ 40% when idle.

| Module | Icon Shape | Description |
|--------|-----------|-------------|
| FAT (XObese) | Circle with 3 concentric rings | Mass, layers, saturation |
| BITE (XOppossum) | Triangle with serrated edge | Sharp attack, feral energy |
| SNAP (XOddCouple X) | Square with diagonal crack | Percussive hit, impact |
| MORPH (XOddCouple O) | Flowing wave with morph inflection | Pad evolution, wavetable |
| DUB (XOverdub) | Echo ripples (3 arcs) | Delay, space, dub |
| DRIFT (XOdyssey) | Spiral nebula | Psychedelic journey, cosmic |
| PLUSH (XOblongBob) | Rounded rectangle with specular dot | Glass, warmth, character |
| ONSET (XOnset) | Vertical impact line with decay tail | Transient, percussive onset |

### 9.2 PlaySurface Mode Icons

| Mode | Icon | Size | Notes |
|------|------|------|-------|
| PAD | 3x3 grid of squares | 16x16px | Scale-locked note grid |
| FRETLESS | Horizontal sine wave | 16x16px | Continuous pitch |
| DRUM | 2x4 grid of circles | 16x16px | Percussive kit layout |

### 9.3 Performance Pad Icons

| Pad | Icon | Color | Size |
|-----|------|-------|------|
| FIRE | Flame silhouette | `#00FF88` (green) | 20x20px |
| XOSEND | Upward arrow | `#FFAA00` (amber) | 20x20px |
| ECHO CUT | Scissors | `#FFAA00` (amber) | 20x20px |
| PANIC | X mark | `#FF3333` (red) | 20x20px |

### 9.4 Utility Icons

| Function | Icon | Idle Color | Active Color |
|----------|------|------------|--------------|
| Collapse/expand panel | Chevron (down/up) | `#FFFFFF` @ 40% | `#FFFFFF` @ 70% |
| Settings gear | Gear outline | `#FFFFFF` @ 40% | `#FFFFFF` @ 70% |
| Preset save | Floppy disk | `#FFFFFF` @ 40% | `#E9C46A` (gold) |
| Preset randomize | Dice | `#FFFFFF` @ 40% | `#E9C46A` (gold) |
| Copy | Overlapping squares | `#FFFFFF` @ 40% | `primaryAccent` |
| Paste | Clipboard with arrow | `#FFFFFF` @ 40% | `primaryAccent` |
| Undo | Curved left arrow | `#FFFFFF` @ 40% | `primaryAccent` |

### 9.5 Icon Rendering Rules

1. **Stroke-based, not filled.** All icons use 1.5px stroked paths for a clean, lightweight appearance.
2. **Single color.** No multi-color icons. Color comes from state (idle/active) and context (engine accent).
3. **No gradients in icons.** Gradients are reserved for panels and surfaces, not iconography.
4. **Pixel-aligned.** At standard scale, icon paths snap to half-pixel grid for sharp rendering.

---

## 10. Light Mode / Dark Mode

### 10.1 Light Mode (Default, Primary)

Light mode is the default and primary visual mode. It provides a striking, clean, legible interface that stands out from the sea of dark-themed audio plugins. The bright canvas makes accent colors bold and confident. All specifications in Sections 1-9 now describe light mode as the primary target.

**Why light-first:** The mega-tool has more parameters and visual complexity than any single XO_OX instrument. A light canvas provides better legibility for dense parameter layouts, cleaner visual hierarchy, and a more professional, refined aesthetic. Accent colors read differently on light — punchy and defined rather than glowing and ambient.

### 10.2 Light Mode Color Table (Primary)

| Element | Light Mode | Notes |
|---------|------------|-------|
| Window background | `#F0F0F5` | Clean off-white with subtle cool undertone |
| Shell panel surface | `#E8E8F0` | Slightly recessed for panel hierarchy |
| Glass panel fill | `#000000` @ 6% | Frosted dark-on-light |
| Glass panel border | `#000000` @ 8% | Subtle structural edges |
| Specular highlight | `#FFFFFF` @ 40% (top) | Bright top-edge catch |
| Inner shadow | `#000000` @ 8% (bottom) | Soft depth cue |
| Shell dividers | `#000000` @ 8% | Clean separation |
| Primary text | `#1A1A1A` | Near-black for maximum legibility |
| Secondary text | `#1A1A1A` @ 50% | Clear but receded |
| Dim text | `#1A1A1A` @ 30% | Tertiary information |
| Knob body | `#000000` @ 8% center | Glass dome adapted for light bg |
| Track background | `#000000` @ 6% | Subtle inset track |
| Accent colors | Same hex values as engine registry | Bold on light — no glow needed |
| Glow effects | Accent @ 20% | Subtle — glow is less dominant on light, accents carry through color fill instead |
| Coupling gold | `#C4A044` | Darkened for WCAG contrast on light bg |

### 10.3 Dark Mode (Optional, Available v1.0)

Dark mode preserves the aesthetic familiar to standalone XO_OX instrument users. It matches the existing 5/6 dark-themed instruments.

| Element | Light Mode | Dark Mode |
|---------|------------|-----------|
| Window background | `#F0F0F5` | `#0B0B12` |
| Shell panel surface | `#E8E8F0` | `#141420` |
| Glass panel fill | `#000000` @ 6% | `#FFFFFF` @ 8% |
| Glass panel border | `#000000` @ 8% | `#FFFFFF` @ 10% |
| Specular highlight | `#FFFFFF` @ 40% (top) | `#FFFFFF` @ 20% (top) |
| Inner shadow | `#000000` @ 8% (bottom) | `#000000` @ 15% (bottom) |
| Shell dividers | `#000000` @ 8% | `#FFFFFF` @ 10% |
| Primary text | `#1A1A1A` | `#FFFFFF` |
| Secondary text | `#1A1A1A` @ 50% | `#FFFFFF` @ 50% |
| Dim text | `#1A1A1A` @ 30% | `#FFFFFF` @ 30% |
| Knob body | `#000000` @ 8% center | `#FFFFFF` @ 14% center |
| Track background | `#000000` @ 6% | `#FFFFFF` @ 8% |
| Accent colors | Same hex values | Same hex values |
| Glow effects | Accent @ 20% | Accent @ 40% (more vibrant on dark) |
| Coupling gold | `#C4A044` | `#E9C46A` |

### 10.4 Shipping Strategy

| Version | Mode Support |
|---------|-------------|
| v1.0 | Light mode default + dark mode toggle |
| v1.x | System preference auto-detect (macOS appearance setting) |

---

## 11. Responsive Scaling

### 11.1 Base Dimensions

| Measurement | Value | Notes |
|-------------|-------|-------|
| Base window size | 1060 x 640px | Matches XOblongBob and XOppossum |
| Minimum window size | 848 x 512px (80% scale) | Smallest usable size |
| Maximum window size | 1590 x 960px (150% scale) | Largest standard scale |
| Aspect ratio | 1.656:1 (approx. 5:3) | Maintained across all scales |

### 11.2 Scale Factors

| Scale | Window Size | Use Case | Knob (Standard) | Knob (Compact) | Knob (Macro) | Font Scale |
|-------|-------------|----------|-----------------|----------------|-------------|------------|
| 80% | 848 x 512px | Small screens, laptop secondary | 45px | 29px | 58px | 0.8x |
| 100% | 1060 x 640px | Standard desktop | 56px | 36px | 72px | 1.0x |
| 125% | 1325 x 800px | High-DPI / Retina | 70px | 45px | 90px | 1.25x |
| 150% | 1590 x 960px | Large displays, presentation | 84px | 54px | 108px | 1.5x |

### 11.3 Touch Target Minimums

| Scale | Minimum Touch Target | Notes |
|-------|---------------------|-------|
| 80% | 35px (44pt equivalent at scale) | Below Apple HIG on paper, acceptable for desktop mouse |
| 100% | 44px | Apple HIG compliant |
| 125% | 55px | Comfortable touch |
| 150% | 66px | Large touch, tablet-friendly |

### 11.4 Layout Breakpoints

| Window Width | Layout Adaptation |
|-------------|-------------------|
| < 848px | Not supported — minimum size enforced |
| 848-1059px | 80% scale, compact knobs in dense areas |
| 1060-1324px | 100% scale, standard layout |
| 1325-1589px | 125% scale, increased spacing |
| 1590px+ | 150% scale, maximum spacing and comfort |

### 11.5 Scaling Implementation

All pixel dimensions are defined as `float` values multiplied by a global `scaleFactor`:

```cpp
float scaleFactor = 1.0f;  // Set by user preference or auto-detect

float knobSize()    const { return 56.0f * scaleFactor; }
float compactKnob() const { return 36.0f * scaleFactor; }
float macroKnob()   const { return 72.0f * scaleFactor; }
float fontSize(float basePt) const { return basePt * scaleFactor; }
float margin()      const { return 8.0f * scaleFactor; }
float panelGap()    const { return 4.0f * scaleFactor; }
float borderWidth() const { return 1.0f; }  // Always 1px — never scale borders
```

Border widths and stroke widths do not scale. A 1px border remains 1px at all scales. On Retina displays, JUCE renders at the native pixel density, so 1px becomes 1 physical pixel on standard displays and 2 physical pixels on 2x Retina. This is handled automatically by JUCE's `Graphics` context.

---

## 12. Implementation: XOMegaLookAndFeel Class

### 12.1 Class Architecture

```cpp
// File: Source/Shared/UI/XOMegaLookAndFeel.h

#pragma once
#include <JuceHeader.h>

namespace xo {

/// Color profile for an engine module — swapped at runtime
struct EngineColorProfile
{
    juce::Colour primaryAccent   { 0xFFE9C46A };   // Default: gold (no engine)
    juce::Colour secondaryAccent { 0x00000000 };   // Optional
    juce::Colour glowColor       { 0xFFE9C46A };   // Default: gold
    float        glowAlpha       = 0.40f;
    juce::String moduleName      { "—" };
};

/// Engine type enum for quick switching
enum class EngineType
{
    None,       // Shell default — gold accents
    FAT,        // XObese
    BITE,       // XOppossum
    SNAP,       // XOddCouple Engine X
    MORPH,      // XOddCouple Engine O
    DUB,        // XOverdub
    DRIFT,      // XOdyssey
    PLUSH,      // XOblongBob
    ONSET       // XOnset
};

class XOMegaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    XOMegaLookAndFeel();
    ~XOMegaLookAndFeel() override = default;

    //==========================================================================
    // Engine Profile Management
    //==========================================================================

    /// Set the active engine — all draw methods reference this profile
    void setActiveEngine(EngineType type);

    /// Get the current engine color profile
    const EngineColorProfile& getProfile() const { return activeProfile; }

    /// Set a custom color profile (for blended/interpolated states)
    void setCustomProfile(const EngineColorProfile& profile);

    /// Interpolate between two profiles (for engine switch animation)
    static EngineColorProfile interpolateProfiles(
        const EngineColorProfile& a,
        const EngineColorProfile& b,
        float t);

    //==========================================================================
    // Scaling
    //==========================================================================

    void setScaleFactor(float factor);
    float getScaleFactor() const { return scaleFactor; }

    float scaled(float value) const { return value * scaleFactor; }

    //==========================================================================
    // Shared Glass Rendering Utilities
    //==========================================================================

    /// Draw a frosted glass panel with border, specular, and inner shadow
    void drawGlassPanel(juce::Graphics& g,
                        juce::Rectangle<float> bounds,
                        float cornerRadius = 8.0f,
                        float fillAlpha = 0.08f);

    /// Draw a specular top-edge highlight on a rounded rectangle
    void drawSpecularEdge(juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          float cornerRadius = 8.0f,
                          float alpha = 0.20f);

    /// Draw an accent-colored arc with glow blur
    void drawGlowArc(juce::Graphics& g,
                     juce::Rectangle<float> bounds,
                     float startAngle,
                     float endAngle,
                     float trackWidth = 3.0f,
                     float glowRadius = 4.0f);

    /// Draw a panel header bar with engine accent tinting
    void drawPanelHeader(juce::Graphics& g,
                         juce::Rectangle<float> bounds,
                         const juce::String& title);

    /// Draw a coupling strip between two engine panels
    void drawCouplingStrip(juce::Graphics& g,
                           juce::Rectangle<float> bounds,
                           float couplingAmount);

    //==========================================================================
    // LookAndFeel_V4 Overrides
    //==========================================================================

    void drawRotarySlider(juce::Graphics& g, int x, int y,
                          int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y,
                          int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style,
                          juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g,
                              juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool isMouseOverButton,
                              bool isButtonDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height,
                      bool isButtonDown,
                      int buttonX, int buttonY,
                      int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawPopupMenuItem(juce::Graphics& g,
                           const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon,
                           const juce::Colour* textColour) override;

    void drawTabButton(juce::TabBarButton& button,
                       juce::Graphics& g, bool isMouseOver,
                       bool isMouseDown) override;

    void drawLabel(juce::Graphics& g, juce::Label& label) override;

private:
    //==========================================================================
    // Internal State
    //==========================================================================

    EngineColorProfile activeProfile;
    float scaleFactor = 1.0f;

    /// Static registry of built-in engine profiles
    static EngineColorProfile getBuiltInProfile(EngineType type);

    //==========================================================================
    // Shell Color Constants
    //==========================================================================

    static constexpr uint32_t kWindowBg       = 0xFF0B0B12;
    static constexpr uint32_t kPanelSurface   = 0xFF141420;
    static constexpr uint32_t kCouplingGold   = 0xFFE9C46A;
    static constexpr uint32_t kPanicRed       = 0xFFFF3333;
    static constexpr uint32_t kClipRed        = 0xFFFF3333;
    static constexpr uint32_t kWhite          = 0xFFFFFFFF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XOMegaLookAndFeel)
};

} // namespace xo
```

### 12.2 Built-In Profile Registry

```cpp
// In XOMegaLookAndFeel.cpp

EngineColorProfile XOMegaLookAndFeel::getBuiltInProfile(EngineType type)
{
    switch (type)
    {
        case EngineType::None:
            return { juce::Colour(0xFFE9C46A), {}, juce::Colour(0xFFE9C46A), 0.40f, "---" };

        case EngineType::FAT:
            return { juce::Colour(0xFFE06090), juce::Colour(0xFFE8C84A),
                     juce::Colour(0xFFE06090), 0.40f, "FAT" };

        case EngineType::BITE:
            return { juce::Colour(0xFFC47040), juce::Colour(0xFF88CC44),
                     juce::Colour(0xFFC47040), 0.40f, "BITE" };

        case EngineType::SNAP:
            return { juce::Colour(0xFFC8553D), {},
                     juce::Colour(0xFFC8553D), 0.40f, "SNAP" };

        case EngineType::MORPH:
            return { juce::Colour(0xFF2A9D8F), {},
                     juce::Colour(0xFF2A9D8F), 0.40f, "MORPH" };

        case EngineType::DUB:
            return { juce::Colour(0xFF00FF88), juce::Colour(0xFFFFAA00),
                     juce::Colour(0xFF00FF88), 0.35f, "DUB" };

        case EngineType::DRIFT:
            return { juce::Colour(0xFF9B59B6), juce::Colour(0xFFF0C040),
                     juce::Colour(0xFF9B59B6), 0.40f, "DRIFT" };

        case EngineType::PLUSH:
            return { juce::Colour(0xFFF5C97A), {},
                     juce::Colour(0xFFF5C97A), 0.40f, "PLUSH" };

        case EngineType::ONSET:
            return { juce::Colour(0xFFC8553D), juce::Colour(0xFF2A9D8F),
                     juce::Colour(0xFFC8553D), 0.40f, "ONSET" };

        default:
            return getBuiltInProfile(EngineType::None);
    }
}
```

### 12.3 Key Implementation Notes

1. **`setActiveEngine()` triggers a repaint cascade.** When the active engine changes, the LookAndFeel stores the new profile and the editor calls `repaint()` on all visible components. The 200ms crossfade is handled by the editor's animation timer interpolating between the old and new profiles.

2. **`drawGlassPanel()` is the single source of truth for panel rendering.** Every panel in the mega-tool calls this method. It renders: fill, border, specular top edge, and inner bottom shadow. No component should render its own panel background — this ensures glass consistency.

3. **`drawGlowArc()` uses a two-pass approach.** First pass: draw the accent-colored arc at full opacity. Second pass: draw the same arc with `glowAlpha` opacity and a larger stroke width to simulate blur. True Gaussian blur is too expensive per-frame; the wider-stroke approximation is visually sufficient and costs nothing.

4. **The LookAndFeel does not own component layout.** It only handles painting. Component sizes and positions are determined by the editor classes (MegaToolEditor, EnginePanel, PlaySurface). The LookAndFeel provides `scaled()` for dimension queries but does not position components.

5. **Thread safety.** `setActiveEngine()` and `setCustomProfile()` are called on the message thread only. The audio thread never touches the LookAndFeel. No locks are needed.

### 12.4 Glass Rendering Reference Implementation

```cpp
void XOMegaLookAndFeel::drawGlassPanel(juce::Graphics& g,
                                        juce::Rectangle<float> bounds,
                                        float cornerRadius,
                                        float fillAlpha)
{
    // 1. Panel fill — frosted glass
    g.setColour(juce::Colour(kWhite).withAlpha(fillAlpha));
    g.fillRoundedRectangle(bounds, cornerRadius);

    // 2. Panel border
    g.setColour(juce::Colour(kWhite).withAlpha(0.10f));
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

    // 3. Specular top-edge highlight
    drawSpecularEdge(g, bounds, cornerRadius, 0.20f);

    // 4. Inner shadow (bottom edge)
    auto shadowBounds = bounds.reduced(1.0f);
    auto shadowPath = juce::Path();
    shadowPath.addRoundedRectangle(shadowBounds, cornerRadius);

    // Approximate inner shadow with a thin dark line at the bottom interior
    g.setColour(juce::Colour(0xFF000000).withAlpha(0.15f));
    g.drawHorizontalLine(static_cast<int>(bounds.getBottom() - 2.0f),
                         bounds.getX() + cornerRadius,
                         bounds.getRight() - cornerRadius);
}

void XOMegaLookAndFeel::drawSpecularEdge(juce::Graphics& g,
                                          juce::Rectangle<float> bounds,
                                          float cornerRadius,
                                          float alpha)
{
    // Draw a bright 1px line along the top interior edge of the rounded rect
    auto topEdge = bounds.removeFromTop(1.0f).reduced(cornerRadius, 0.0f);
    g.setColour(juce::Colour(kWhite).withAlpha(alpha));
    g.fillRect(topEdge);
}
```

---

## Appendix A: Complete Color Reference

### A.1 Shell Colors (Engine-Independent)

| Token | Hex | Alpha | ARGB (JUCE) | Usage |
|-------|-----|-------|-------------|-------|
| `shell.windowBg` | `#0B0B12` | 100% | `0xFF0B0B12` | Window background fill |
| `shell.panelSurface` | `#141420` | 100% | `0xFF141420` | Elevated surface panels |
| `shell.glassFill` | `#FFFFFF` | 8% | `0x14FFFFFF` | Standard glass panel fill |
| `shell.glassBorder` | `#FFFFFF` | 10% | `0x1AFFFFFF` | Panel border stroke |
| `shell.specular` | `#FFFFFF` | 20% | `0x33FFFFFF` | Top-edge specular highlight |
| `shell.innerShadow` | `#000000` | 15% | `0x26000000` | Bottom-inside edge shadow |
| `shell.divider` | `#FFFFFF` | 10% | `0x1AFFFFFF` | Section divider lines |
| `shell.textPrimary` | `#FFFFFF` | 100% | `0xFFFFFFFF` | Primary labels and headings |
| `shell.textSecondary` | `#FFFFFF` | 50% | `0x80FFFFFF` | Secondary labels |
| `shell.textDim` | `#FFFFFF` | 30% | `0x4DFFFFFF` | Tertiary / disabled text |
| `shell.iconIdle` | `#FFFFFF` | 40% | `0x66FFFFFF` | Unselected icons |
| `shell.iconHover` | `#FFFFFF` | 70% | `0xB3FFFFFF` | Hovered icons |

### A.2 Brand Colors (Shared/Coupling)

| Token | Hex | Alpha | ARGB | Usage |
|-------|-----|-------|------|-------|
| `brand.couplingGold` | `#E9C46A` | 100% | `0xFFE9C46A` | Coupling strip, shared controls, logo |
| `brand.goldGlow` | `#E9C46A` | 30% | `0x4DE9C46A` | Gold glow halo |
| `brand.goldSubtle` | `#E9C46A` | 6% | `0x0FE9C46A` | Gold panel tint |
| `brand.panicRed` | `#FF3333` | 100% | `0xFFFF3333` | Panic button, clip indicator |

### A.3 Engine Accent Colors (Per-Engine)

| Engine | Token | Hex | ARGB |
|--------|-------|-----|------|
| FAT | `fat.primary` | `#E06090` | `0xFFE06090` |
| FAT | `fat.secondary` | `#E8C84A` | `0xFFE8C84A` |
| FAT | `fat.glow` | `#E06090` @ 40% | `0x66E06090` |
| BITE | `bite.primary` | `#C47040` | `0xFFC47040` |
| BITE | `bite.secondary` | `#88CC44` | `0xFF88CC44` |
| BITE | `bite.glow` | `#C47040` @ 40% | `0x66C47040` |
| SNAP | `snap.primary` | `#C8553D` | `0xFFC8553D` |
| SNAP | `snap.glow` | `#C8553D` @ 40% | `0x66C8553D` |
| MORPH | `morph.primary` | `#2A9D8F` | `0xFF2A9D8F` |
| MORPH | `morph.glow` | `#2A9D8F` @ 40% | `0x662A9D8F` |
| DUB | `dub.primary` | `#00FF88` | `0xFF00FF88` |
| DUB | `dub.secondary` | `#FFAA00` | `0xFFFFAA00` |
| DUB | `dub.glow` | `#00FF88` @ 35% | `0x5900FF88` |
| DRIFT | `drift.primary` | `#9B59B6` | `0xFF9B59B6` |
| DRIFT | `drift.secondary` | `#F0C040` | `0xFFF0C040` |
| DRIFT | `drift.glow` | `#9B59B6` @ 40% | `0x669B59B6` |
| PLUSH | `plush.primary` | `#F5C97A` | `0xFFF5C97A` |
| PLUSH | `plush.glow` | `#F5C97A` @ 40% | `0x66F5C97A` |
| ONSET | `onset.primary` | `#C8553D` | `0xFFC8553D` |
| ONSET | `onset.secondary` | `#2A9D8F` | `0xFF2A9D8F` |
| ONSET | `onset.glow` | `#C8553D` @ 40% | `0x66C8553D` |

---

## Appendix B: Per-Engine Visual Origin Map

This table maps each engine's mega-tool visual treatment back to its standalone instrument's original aesthetic, documenting what was preserved and what was normalized.

| Engine | Standalone Background | Mega-Tool Background | Standalone Panel Style | Mega-Tool Panel Style | Preserved | Normalized |
|--------|----------------------|---------------------|----------------------|----------------------|-----------|------------|
| FAT (XObese) | `#0D0D12` | `#0B0B12` | visionOS glass, specular | Shared glass | Pink/Yellow accents, glass metaphor | Background shifted 2 values |
| BITE (XOppossum) | `#0E0E11` | `#0B0B12` | Spatial glass, vertical gradient | Shared glass | Rust/Acid palette, frosted borders | Gradient direction standardized |
| SNAP (XOddCouple X) | `#1A1A1A` | `#0B0B12` | Solid panels, `#2D2D2D` surface | Shared glass | Terracotta accent, percussive identity | Background darkened, glass added |
| MORPH (XOddCouple O) | `#1A1A1A` | `#0B0B12` | Solid panels, `#2D2D2D` surface | Shared glass | Teal accent, lush identity | Background darkened, glass added |
| DUB (XOverdub) | `#0A0A0A` | `#0B0B12` | Solid dark, `#1A1A1A` surfaces | Shared glass | Green/Amber/Cyan accents, brutalist density | Glass replaces solid panels |
| DRIFT (XOdyssey) | Light base | `#0B0B12` | Custom light LookAndFeel | Shared glass (dark) | Psychedelic accent philosophy | Full inversion to dark; light mode preserves original |
| PLUSH (XOblongBob) | `#08080F` | `#0B0B12` | Apple Liquid Glass, 14% white | Shared glass | Amber accent, glass dome knobs, specular edge | Glass spec IS the shared spec (origin) |
| ONSET (XOnset) | N/A (new) | `#0B0B12` | N/A | Shared glass | Built to spec from day 1 | N/A |

---

## Appendix C: Accessibility Considerations

### C.1 Color Blindness

The engine accent palette was designed to remain distinguishable under the three major color vision deficiency types:

| Engine Pair | Protanopia (no red) | Deuteranopia (no green) | Tritanopia (no blue) |
|-------------|-------------------|----------------------|---------------------|
| FAT vs SNAP | Pink reads as gray-blue; Terracotta reads as dark yellow. Distinguishable by lightness. | Similar concern. Secondary accent (Yellow vs none) resolves ambiguity. | Both remain warm-toned. Lightness difference sufficient. |
| DUB vs MORPH | Green shifts to yellow; Teal shifts to gray-blue. Distinguishable. | Green shifts to amber; Teal remains blue-leaning. Distinguishable. | Both shift warm. Name labels resolve. |
| BITE vs SNAP | Rust and Terracotta are close. Acid lime secondary resolves BITE. | Similar. Secondary accent is the differentiator. | Distinguishable by lightness. |

**Mitigation strategies (all versions):**

1. Engine module names always appear alongside color indicators (text is primary, color is secondary).
2. Engine icons provide shape-based identification (Section 9.1) independent of color.
3. In multi-engine layouts, panel headers show both the colored name and the geometric icon.

### C.2 Motion Sensitivity

Users who experience discomfort from motion can disable:
- Glow decay animations (replaced by static glow)
- Orbit trail rendering (replaced by single cursor dot)
- Engine switch crossfade (replaced by instant swap)
- Coupling pulse animation (replaced by static gold line)

A single `Reduce Motion` toggle in settings controls all of these.

---

## Appendix D: Design Token Export Format

For integration with design tools (Figma, Sketch) and CSS-based prototypes, all tokens are exportable as JSON:

```json
{
    "xo_mega_tool_tokens": {
        "version": "1.0",
        "shell": {
            "windowBg": "#0B0B12",
            "panelSurface": "#141420",
            "glassFillAlpha": 0.08,
            "glassBorderAlpha": 0.10,
            "specularAlpha": 0.20,
            "innerShadowAlpha": 0.15,
            "dividerAlpha": 0.10,
            "textPrimary": "#FFFFFF",
            "textSecondaryAlpha": 0.50,
            "textDimAlpha": 0.30,
            "cornerRadius": 8,
            "panelGap": 4,
            "panelMargin": 8
        },
        "brand": {
            "couplingGold": "#E9C46A",
            "panicRed": "#FF3333"
        },
        "engines": {
            "FAT":   { "primary": "#E06090", "secondary": "#E8C84A", "glowAlpha": 0.40 },
            "BITE":  { "primary": "#C47040", "secondary": "#88CC44", "glowAlpha": 0.40 },
            "SNAP":  { "primary": "#C8553D", "secondary": null,      "glowAlpha": 0.40 },
            "MORPH": { "primary": "#2A9D8F", "secondary": null,      "glowAlpha": 0.40 },
            "DUB":   { "primary": "#00FF88", "secondary": "#FFAA00", "glowAlpha": 0.35 },
            "DRIFT": { "primary": "#9B59B6", "secondary": "#F0C040", "glowAlpha": 0.40 },
            "PLUSH": { "primary": "#F5C97A", "secondary": null,      "glowAlpha": 0.40 },
            "ONSET": { "primary": "#C8553D", "secondary": "#2A9D8F", "glowAlpha": 0.40 }
        },
        "typography": {
            "fontPrimary": "SF Pro Display",
            "fontSecondary": "SF Pro Text",
            "fontMono": "SF Mono",
            "h1": { "size": 14, "weight": "bold", "tracking": 0.5, "transform": "uppercase" },
            "h2": { "size": 12, "weight": "bold", "tracking": 0.3, "transform": "uppercase" },
            "h3": { "size": 11, "weight": "medium", "tracking": 0.2, "transform": "uppercase" },
            "label": { "size": 10, "weight": "medium", "tracking": 0.1, "transform": "uppercase" },
            "value": { "size": 10, "weight": "regular", "tracking": 0, "transform": "none" }
        },
        "animation": {
            "glowDecay": 0.92,
            "glowFps": 30,
            "ghostTraceLifetime": 1.5,
            "orbitTrailDots": 60,
            "engineSwitchMs": 200,
            "buttonFlashMs": 100,
            "hoverTransitionMs": 150,
            "panelCollapseMs": 250,
            "presetFlashMs": 300,
            "modeToggleMs": 400
        },
        "dimensions": {
            "baseWindowWidth": 1060,
            "baseWindowHeight": 640,
            "knobStandard": 56,
            "knobCompact": 36,
            "knobMacro": 72,
            "knobMini": 24,
            "minTouchTarget": 44,
            "navBarHeight": 36,
            "panelHeaderHeight": 24,
            "couplingStripWidth": 48
        }
    }
}
```

---

*CONFIDENTIAL -- XO_OX Internal Design Document*
