# UI Patterns — Gallery Model Design System

Standards for XOlokun UI implementation following the Gallery Model aesthetic.

## Gallery Model Philosophy

The UI is a **gallery wall** — a warm white shell that frames the colorful engine accent colors. The frame doesn't compete with the art; it presents it.

- The shell is the constant: warm, clean, confident
- Engine accents are the variable: each engine brings its own color personality
- Controls feel physical: knobs turn, sliders slide, surfaces respond to touch
- Information is layered: essential first, details on demand

## Color System

### Constants

| Token | Value | Usage |
|-------|-------|-------|
| Shell White | `#F8F6F3` | Primary background, panel fill |
| XO Gold | `#E9C46A` | Brand accent — macros, coupling strip, active states |
| Text Primary | `#2C2C2C` | Headings, labels, values |
| Text Secondary | `#6B6B6B` | Descriptions, inactive labels |
| Surface Hover | `#F0EDE8` | Hover state for interactive elements |
| Surface Pressed | `#E8E0D8` | Pressed/active state |
| Divider | `#E0DCD6` | Separators, borders |

### Engine Accent Colors

Each engine has a signature accent color (see CLAUDE.md for full table). Usage:
- Active engine slot background tint
- Parameter knob accent ring
- Waveform/visualizer fills
- Engine name badge

**Rule:** Engine color is the accent, never the dominant color. The shell white should always occupy the most visual area.

### Dark Mode

- Swap shell to `#1A1A1A`
- Text to `#E8E0D8`
- Keep engine accent colors the same (they're designed to work on both)
- XO Gold stays `#E9C46A`
- Light mode is the **primary presentation** — dark mode is a toggle

## Typography

| Role | Font | Weight | Size |
|------|------|--------|------|
| Display / Headings | Space Grotesk | Bold (700) | 18-24px |
| Body / Labels | Inter | Regular (400), Medium (500) | 12-14px |
| Values / Numbers | JetBrains Mono | Regular (400) | 11-13px |

**Rules:**
- Numeric values always use monospace (JetBrains Mono) for alignment
- Parameter names use Inter Medium
- Engine names use Space Grotesk Bold
- Never use more than 2 weights of the same font in one view

## Layout Patterns

### Engine Slot Strip

```
┌─────────────────────────────────────────────────────┐
│  [Slot 1]    [Slot 2]    [Slot 3]    [Slot 4]      │
│  OddfeliX    Odyssey     Opal        ─ empty ─      │
│  ■ active    ■ active    ■ active    ○ empty        │
└─────────────────────────────────────────────────────┘
```

- 4 slots, always visible
- Active slot has engine accent color tint
- Empty slot shows dashed outline
- Drag to reorder, click to select

### Parameter Panel

```
┌──────────────────────────────┐
│  FILTER                      │  ← Section header (Space Grotesk)
│                              │
│  ○ Cutoff      ○ Resonance   │  ← Knobs with labels (Inter)
│  1.2 kHz       0.45         │  ← Values (JetBrains Mono)
│                              │
│  [───●─────────] Drive       │  ← Slider with label
│  0.32                        │  ← Value
└──────────────────────────────┘
```

### Macro Strip

```
┌──────────────────────────────────────────────────┐
│  M1 CHARACTER  M2 MOVEMENT  M3 COUPLING  M4 SPACE│
│  [====●======] [===●=======] [●==========] [===●]│
│    0.52          0.38          0.05         0.61  │
└──────────────────────────────────────────────────┘
```

- Always visible at bottom or top
- XO Gold accent for macro controls
- Values in JetBrains Mono

### Coupling Matrix View

```
┌────────────────────────────────────────┐
│  COUPLING MATRIX                        │
│                                         │
│  OddfeliX ──[phase mod]──→ Odyssey     │
│              intensity: 0.4             │
│                                         │
│  Opal ──[grain mod]──→ OddfeliX        │
│              intensity: 0.2             │
│                                         │
│  [+ Add Route]                          │
└────────────────────────────────────────┘
```

- Visual flow diagram (source → target)
- Coupling type as label on the connection
- XO Gold for active connections

## PlaySurface

4-zone unified playing interface:

| Zone | Mode | Interaction |
|------|------|------------|
| Pad | Pressure-sensitive pads | Tap, hold, velocity |
| Fretless | Continuous pitch surface | Slide, vibrato, glide |
| Drum | Percussive triggers | Hit, roll, flam |
| XY | 2D modulation surface | Touch position maps to params |

**Rules:**
- All zones must respond to touch input with < 10ms latency
- Visual feedback immediate on touch (don't wait for audio)
- Zone boundaries clearly marked but not heavy-handed
- Engine accent color used for touch feedback ripples

## Component Patterns

### Knobs
- Circular, 270° rotation range
- Engine accent color for the value arc
- Center detent at 12 o'clock for bipolar parameters
- Value tooltip on hover/touch (JetBrains Mono)
- Double-click/tap to reset to default

### Sliders
- Horizontal by default
- Thumb: rounded rectangle, filled with engine accent
- Track: thin line, Shell White → engine accent gradient
- Value label right-aligned or below

### Buttons/Toggles
- Rounded rectangle, subtle border
- Active state: engine accent fill
- Inactive: outline only
- Never use sharp corners (radius ≥ 4px)

### Dropdown/Selectors
- Appears as a button showing current selection
- Opens a clean list, no heavy shadows
- Selected item highlighted with engine accent

## Animation Guidelines

- Transitions: 150-200ms ease-out for state changes
- Knob rotation: immediate (no lag), smooth at all rates
- Engine swap: 50ms crossfade matches audio crossfade timing
- Visualizer: 60fps target, fall back gracefully on low-power devices
- Never animate something that doesn't need animation

## Responsive Behavior

- **macOS Standalone/AU:** Fixed size or resizable with aspect ratio lock
- **iOS AUv3:** Adaptive layout for different host sizes
- **iOS Standalone:** Full-screen, orientation-aware
- Engine panel scrolls if needed, macro strip and engine slots always visible

## Accessibility

- All controls keyboard-navigable (macOS)
- VoiceOver labels for all interactive elements (iOS)
- Sufficient contrast ratio (4.5:1 minimum for text)
- No information conveyed by color alone (supplement with labels/icons)
