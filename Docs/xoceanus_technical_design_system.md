# XOceanus — Technical Design System & UI Philosophy

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Purpose:** Define the visual language, UI architecture, and technical design standards for XOceanus

---

## 1. Design Philosophy

### 1.1 The Gallery Model

XOceanus is a **gallery**. Each engine is an **exhibition**.

The platform provides a clean, neutral space — warm white walls, consistent typography, predictable navigation — that frames whatever art is on display. When a producer loads the OddfeliX/OddOscar engine, the gallery fills with terracotta and teal. Switch to XOdyssey, and violet gradients take over. Couple two engines, and their colors meet in the coupling strip like two exhibitions connected by a golden corridor.

This metaphor drives every design decision:
- **The gallery walls (shell)** never change — they're always warm white, always the same layout
- **The art (engine panels)** changes completely — color, texture, even control density
- **The coupling strip** is the corridor between exhibitions — golden, visible, connecting
- **The preset browser** is the exhibition catalog — organized by mood, searchable by DNA
- **Community presets** are guest exhibitions — credited to the artist, displayed in the same gallery

### 1.2 Core Principle: Light Creates Contrast

XOceanus defaults to **light mode**. This is a deliberate choice that serves the gallery model — a bright, clean gallery wall makes the colored engine art pop harder. When every other synth is a dark rectangle, XOceanus is the one you notice.

Dark mode is available as a toggle — some producers work at 3AM and need it. But the marketing, screenshots, and first-launch experience are all light. The gallery is open during daylight hours by default.

### 1.3 The XO_OX Visual DNA

Every XO_OX instrument has had its own personality. XOceanus doesn't flatten these — each engine is a distinct exhibition within the gallery:

| Engine (Exhibition) | Visual Character | Palette | Material/Texture |
|-----------|-----------|-------|----------|
| OddfeliX/OddOscar | Dual-tone panels, coupling visualization | Neon Tetra Blue `#00A6D6` + Axolotl Gill Pink `#E8839B` | Brushed metal |
| XOverdub | Vintage mixing desk, VU meters, tape reels | Warm Cream `#F5E6D0` + Olive `#6B7B3A` | Worn wood grain |
| XOdyssey | Psychedelic gradients, journey visualization | Violet `#7B2D8B` + Cyan `#00BCD4` | Holographic sheen |
| XOblong | Rounded corners, soft shadows, tactile | Amber `#E9A84A` + Soft White `#F0EDE8` | Apple Liquid Glass |
| XObese | Bold ALL_CAPS, high contrast, industrial | Black `#0A0A0A` + Hot Pink `#FF1493` | Concrete |
| XOnset | Grid precision, circuit traces, LED indicators | Electric Blue `#0066FF` + Steel `#B8C4CC` | PCB green |

### 1.4 The Unifying Frame

The gallery shell wraps all engines in a **neutral, warm white frame** with engine-specific color appearing only inside the active engine panel(s). This creates:
- **Separation** — the platform (gallery) is visually distinct from the instruments (exhibitions)
- **Recognition** — users always know they're in XOceanus, regardless of which engine is active
- **Identity** — each engine retains its full visual personality within its panel
- **Coupling as connection** — when two engines are active, the golden coupling strip between them is the gallery corridor linking two exhibitions

---

## 2. Color System

### 2.1 Platform Colors (The Shell)

```
Background:         #F8F6F3   (warm paper white)
Background Dark:    #1A1A1A   (near-black, dark mode)
Surface:            #FFFFFF   (cards, panels)
Surface Dark:       #2D2D2D   (cards, dark mode)
Border:             #E0DCD6   (subtle warm grey)
Text Primary:       #1A1A1A   (near-black on light)
Text Secondary:     #7A7670   (warm grey)
Text Primary Dark:  #EEEEEE   (off-white on dark)
```

### 2.2 Brand Colors

```
XO Gold:            #E9C46A   (the brand constant — logo, macro knobs, active states)
XO Gold Light:      #F4DFA0   (hover state, light mode)
XO Gold Dark:       #C4A24E   (pressed state)
```

### 2.3 Mood Colors (Preset Browser)

```
Foundation:         #C8553D   (Terracotta)
Atmosphere:         #2A9D8F   (Teal)
Entangled:          #E9C46A   (Gold)
Prism:              #B8C4CC   (Silver)
Flux:               #C0392B   (Crimson)
Aether:             #4A3680   (Indigo)
```

### 2.4 Engine Accent Colors

Each engine panel uses its accent as the knob fill, active indicator, and section header:

```
OddfeliX/OddOscar:    #C8553D (terracotta) + #2A9D8F (teal) — dual-panel split
XOverdub:      #6B7B3A (olive)
XOdyssey:      #7B2D8B (violet)
XOblong:    #E9A84A (amber)
XObese:        #FF1493 (hot pink)
XOnset:        #0066FF (electric blue)
```

### 2.5 Semantic Colors

```
Active/On:          Engine accent color at 100% opacity
Inactive/Off:       Engine accent color at 20% opacity
Error:              #E74C3C
Success:            #27AE60
Warning:            #F39C12
DNA Brightness:     #FFD700
DNA Warmth:         #E9A84A
DNA Movement:       #2A9D8F
DNA Density:        #7B2D8B
DNA Space:          #4A3680
DNA Aggression:     #C0392B
```

---

## 3. Typography

### 3.1 Font Stack

```
Display/Logo:       Space Grotesk Bold (or Inter Black as fallback)
Headings:           Inter Semi-Bold
Body/Labels:        Inter Regular
Values/Numbers:     JetBrains Mono (monospace for parameter values)
Preset Names:       Inter Medium, 14px
Mood Badges:        Inter Semi-Bold, 10px, UPPERCASE, letter-spacing 1.5px
```

### 3.2 Type Scale

```
Display:    28px / 32px line-height  (logo, hero text)
H1:         20px / 24px             (section headers)
H2:         16px / 20px             (panel titles)
Body:       13px / 18px             (labels, descriptions)
Caption:    11px / 14px             (secondary info, tags)
Micro:      9px  / 12px             (mood badges, engine tags)
```

### 3.3 Rules

- All parameter labels: sentence case ("Filter cutoff" not "FILTER CUTOFF")
- Macro labels: UPPERCASE (they're physical controls — "CHARACTER", "MOVEMENT")
- Preset names: title case as designed ("Velvet Morning", "Tape Chaos")
- Engine names: exact brand spelling ("XOblong" not "Xoblongbob")

---

## 4. Layout Architecture

### 4.1 Master Layout

```
┌─────────────────────────────────────────────────────┐
│  HEADER BAR (fixed)                                 │
│  [XO_OX logo] [Preset name ◄ ►] [M1][M2][M3][M4]  │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ENGINE PANEL(S)                                    │
│  ┌──────────────┐ ┌──────────────┐                  │
│  │  Engine A     │ │  Engine B     │                 │
│  │  (accent clr) │ │  (accent clr) │                │
│  │              │ │              │                  │
│  └──────────────┘ └──────────────┘                  │
│                                                     │
│  COUPLING STRIP (visible when 2+ engines active)    │
│  [coupling visualization — signal flow diagram]     │
│                                                     │
├─────────────────────────────────────────────────────┤
│  BOTTOM BAR                                         │
│  [Mood tabs] [Preset browser / DNA map / Sequencer] │
└─────────────────────────────────────────────────────┘
```

### 4.2 Responsive Engine Panels

| Engines Active | Layout |
|---------------|--------|
| 1 engine | Full-width panel, all controls visible |
| 2 engines | 50/50 horizontal split, coupling strip between |
| 3 engines | 33/33/33 split, compact controls, coupling strip |

### 4.3 Spacing System

```
Base unit:      4px
xs:             4px   (inner padding)
sm:             8px   (between related controls)
md:             16px  (between control groups)
lg:             24px  (between sections)
xl:             32px  (between engine panels)
```

### 4.4 Corner Radius

```
Buttons:        6px
Cards/Panels:   12px
Engine frames:  16px
Modal dialogs:  20px
Knob indicators: 50% (circular)
```

---

## 5. Component Library

### 5.1 Knobs

**Standard Knob** (engine parameters):
- 32×32px, circular track with filled arc
- Track color: border color at 30% opacity
- Fill color: engine accent color
- Value indicator: small dot at arc endpoint, engine accent

**Macro Knob** (header bar):
- 40×40px, thicker arc (3px vs 2px)
- Always XO Gold fill
- Label below in UPPERCASE
- Subtle gold glow on hover (light mode) / gold ring on dark mode

**Mini Knob** (compact mode, 3-engine layout):
- 24×24px, same visual language scaled down
- Label to the right instead of below

### 5.2 Buttons

**Toggle Button:**
- Rounded rectangle, 6px radius
- Off: border color outline, transparent fill
- On: engine accent fill, white text
- Transition: 150ms ease-out

**Action Button:**
- XO Gold fill for primary actions (Save, Export)
- Surface fill + border for secondary actions (Load, Cancel)

### 5.3 Preset Browser Card

```
┌──────────────────────────────┐
│ ● Foundation   [XOverdub]    │  ← mood dot + engine badge
│                              │
│  Classic Dub Bass            │  ← preset name (Inter Medium 14px)
│                              │
│  ▁▃▅▇▅▃  DNA sparkline       │  ← 6-bar mini chart of DNA vector
│                              │
│  dub · warm · subby     ★    │  ← tags + favorite star
└──────────────────────────────┘
```

### 5.4 DNA Visualization

**Sparkline** (in preset card): 6 tiny vertical bars, one per DNA dimension, colored by dimension.

**Radar Chart** (in detail view): Hexagonal radar with 6 axes. Filled area = preset DNA. Overlaid second preset for comparison during morphing/breeding.

**Mood Map** (full browser view): 2D scatter plot of all presets. X = PCA component 1, Y = PCA component 2. Each dot colored by mood. Click a dot to load the preset. Zoom and pan. Cluster labels auto-generated.

### 5.5 Coupling Visualization

When 2+ engines are active, a horizontal strip between panels shows:
- Signal flow arrows from Engine A → Engine B
- Arrow thickness = coupling amount
- Arrow color = blend of both engine accent colors
- Coupling type label at midpoint ("Amp→Filter")
- Draggable amount control on the arrow itself

---

## 6. Animation & Motion

### 6.1 Principles

- Animations are **functional**, not decorative
- Every animation communicates state change
- Max duration: 300ms for UI transitions, 1s for page transitions
- Easing: ease-out for entrances, ease-in for exits

### 6.2 Key Animations

| Element | Animation | Duration |
|---------|-----------|----------|
| Engine panel load | Fade in + slide up 8px | 200ms |
| Preset switch | Crossfade values (no flash) | 150ms |
| Coupling strip appear | Height expand from 0 | 250ms |
| Mood tab switch | Slide preset grid horizontally | 200ms |
| DNA sparkline update | Bar heights animate | 300ms |
| Macro knob glow | Pulse on automation | 800ms loop |
| Breeding animation | DNA bars merge + randomize | 500ms |

### 6.3 Performance Rules

- No animations during audio processing
- GPU-accelerated transforms only (translate, scale, opacity)
- Reduce motion mode: all animations instant (accessibility)

---

## 7. Accessibility

### 7.1 Requirements

- All knobs keyboard-navigable (arrow keys for value, tab for focus)
- Color is never the only indicator — always paired with shape or text
- Minimum contrast ratio 4.5:1 for body text, 3:1 for large text
- Screen reader labels for all interactive elements
- Focus ring visible on all focusable elements (2px XO Gold outline)

### 7.2 Color Blindness

The mood colors were tested against deuteranopia, protanopia, and tritanopia:
- Foundation (terracotta) and Flux (crimson) are distinguishable by luminance
- All mood badges include text labels, never color-only

---

## 8. Dark Mode

### 8.1 Translation Rules

| Element | Light | Dark |
|---------|-------|------|
| Background | `#F8F6F3` | `#1A1A1A` |
| Surface | `#FFFFFF` | `#2D2D2D` |
| Border | `#E0DCD6` | `#4A4A4A` |
| Text primary | `#1A1A1A` | `#EEEEEE` |
| Text secondary | `#7A7670` | `#B0B0B0` |
| Engine accents | Same | Same (unchanged) |
| XO Gold | Same | Same (unchanged) |
| Knob tracks | 30% accent | 40% accent (slightly brighter) |
| Shadows | Subtle drop shadow | None (use border instead) |

### 8.2 Rule

Engine accent colors and XO Gold **never change** between modes. They are brand constants. Only the neutral shell adapts.

---

## 9. Iconography

### 9.1 Engine Icons

Each engine has a minimal icon (16×16, 24×24, 32×32) for use in preset browser badges, engine selectors, and documentation:

| Engine | Icon Concept |
|--------|-------------|
| OddfeliX/OddOscar | Two interlocking circles (X and O) |
| XOverdub | Tape reel (single circle with inner hub) |
| XOdyssey | Spiral/journey path |
| XOblong | Rounded rectangle (the "oblong") |
| XObese | Bold square with weight |
| XOnset | Grid/matrix of dots |

### 9.2 UI Icons

Minimal line icons, 1.5px stroke, engine accent or text secondary color:
- Waveform shapes (sine, saw, square, triangle)
- Filter types (LP, HP, BP curves)
- Mod sources (envelope, LFO wave)
- Actions (save, load, export, favorite, similar, opposite, breed)

### 9.3 Style Rules

- No filled icons — all outline/stroke
- Consistent 1.5px stroke weight at all sizes
- Round line caps and joins
- Icons are monochrome — colored by context, not by design

---

## 10. Implementation Notes

### 10.1 JUCE Integration

The design system maps to JUCE's `LookAndFeel` system:

```cpp
// XOceanusLookAndFeel extends juce::LookAndFeel_V4
// Each engine registers its accent color on activation
// Shell colors come from a ThemeManager (light/dark toggle)

struct Theme {
    juce::Colour background;
    juce::Colour surface;
    juce::Colour border;
    juce::Colour textPrimary;
    juce::Colour textSecondary;
    bool isDark;
};

// Engine panels create child LookAndFeel overrides
// that set rotarySliderFillColour to their accent
```

### 10.2 Asset Pipeline

```
assets/
├── icons/
│   ├── engines/           (SVG, 16/24/32px variants)
│   ├── ui/                (SVG, action/state icons)
│   └── mood/              (SVG, mood category icons)
├── fonts/
│   ├── Inter-*.ttf
│   ├── SpaceGrotesk-Bold.ttf
│   └── JetBrainsMono-Regular.ttf
└── images/
    ├── logo-light.svg
    ├── logo-dark.svg
    └── splash.png
```

### 10.3 CSS Variables (for web-based preset browser or documentation site)

```css
:root {
  --xo-bg:             #F8F6F3;
  --xo-surface:        #FFFFFF;
  --xo-border:         #E0DCD6;
  --xo-text:           #1A1A1A;
  --xo-text-secondary: #7A7670;
  --xo-gold:           #E9C46A;
  --xo-foundation:     #C8553D;
  --xo-atmosphere:     #2A9D8F;
  --xo-entangled:      #E9C46A;
  --xo-prism:          #B8C4CC;
  --xo-flux:           #C0392B;
  --xo-aether:         #4A3680;
}

[data-theme="dark"] {
  --xo-bg:             #1A1A1A;
  --xo-surface:        #2D2D2D;
  --xo-border:         #4A4A4A;
  --xo-text:           #EEEEEE;
  --xo-text-secondary: #B0B0B0;
}
```

---

## 11. Export & Screenshot Standards

### 11.1 Marketing Screenshots

- Always light mode (default brand presentation)
- 2x retina resolution (3840×2160 for 4K)
- Show a cross-engine preset with coupling visualization active
- Mood Map view as the hero screenshot
- DNA sparklines visible in preset cards

### 11.2 Social Media Templates

- Square (1080×1080) for Instagram/threads
- 16:9 (1920×1080) for YouTube thumbnails
- Dark background variant for video overlays
- Always include XO_OX logo + "XOceanus" wordmark

---

*This document defines the visual contract. All UI implementation should reference these specs.*
