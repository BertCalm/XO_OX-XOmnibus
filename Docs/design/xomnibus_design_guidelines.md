# XOlokun — Designer Brief & Visual Design System

**Prepared for:** External UI/Brand Designer
**Version:** 1.0
**Date:** 2026-03-17
**Source of truth docs:**
- `Docs/xolokun_technical_design_system.md` — authoritative component/color/typography spec
- `Docs/xolokun_brand_identity_and_launch.md` — logo, voice, brand rules
- `Source/UI/XOlokunEditor.h` — live color constants (GalleryColors namespace)

---

## 1. What XOlokun Is

XOlokun is a **free, open-source multi-engine synthesizer platform** by XO_OX Designs. Thirty-four character synthesis engines exist inside a single unified shell. The signature feature is **cross-engine coupling** — one engine's output modulates another's filter, pitch, amplitude, or any other parameter.

Tagline: **"Couple everything."**

The platform ships as:
- macOS AU + Standalone (primary)
- macOS VST3 (V2 scope)
- iOS AUv3 + Standalone (active spec, shared JUCE codebase)

---

## 2. The Gallery Model — Core Design Metaphor

XOlokun is a **gallery**. This metaphor drives every design decision.

| Gallery Concept | XOlokun Equivalent |
|----------------|---------------------|
| Gallery walls | The warm white shell — header bar, preset browser, coupling strip frame |
| Exhibition | An engine panel — colored, textured, unique to that instrument |
| Golden corridor between exhibitions | The coupling strip — XO Gold bridge connecting two engines |
| Exhibition catalog | The preset browser — organized by mood, searchable by 6D Sonic DNA |
| Guest exhibition | Community presets — credited, displayed in the same gallery |

**The organizing rule:**
> "The shell is the same gallery for every exhibition. The art inside changes completely."

The shell never changes color. Engine panels change completely. Users always know they're in XOlokun, but immediately know which engine is active.

### Why Light Mode is Default

XOlokun defaults to light mode. A bright, clean gallery wall makes the colored engine art pop harder. When every other synth is a dark rectangle, XOlokun is the one you notice. Dark mode is available as a toggle, but all marketing, screenshots, and first-launch experience are light.

---

## 3. Color System

### 3.1 Shell Colors (The Gallery Walls)

These are the neutral frame. They never change between engines.

| Token | Light Mode | Dark Mode | Usage |
|-------|-----------|----------|-------|
| `shellWhite` | `#F8F6F3` | `#1A1A1A` | Main background — warm paper white |
| `surface` | `#FFFFFF` | `#2D2D2D` | Cards, panels, floating surfaces |
| `slotBg` | `#FCFBF9` | `#2D2D2D` | Engine slot backgrounds |
| `emptySlot` | `#EAE8E4` | `#363636` | Empty/inactive engine slots |
| `borderGray` | `#DDDAD5` | `#4A4A4A` | Subtle warm grey borders |
| `textPrimary` | `#1A1A1A` | `#EEEEEE` | Primary text |
| `textSecondary` | `#6B6965` | `#C8C8C8` | Labels, secondary info |

Implementation note: In `Source/UI/XOlokunEditor.h`, these are live in the `GalleryColors` namespace with theme-aware accessor functions. The `darkMode()` bool is the toggle. Do not hard-code hex values in component code — use the accessors.

### 3.2 Brand Constant — XO Gold

```
XO Gold:       #E9C46A
XO Gold Light: #F4DFA0   (hover state, light mode)
XO Gold Dark:  #C4A24E   (pressed state)
XO Gold Text:  #7A5C1E   (WCAG AA on shell white — 5.0:1 on #F8F6F3 — used for text)
```

XO Gold is **the only color constant** across both modes. It never adapts. It appears on:
- The coupling strip background header
- Macro knob fills (M1–M4: CHARACTER, MOVEMENT, COUPLING, SPACE)
- Primary action buttons (Save, Export)
- Active/selected states in the preset browser
- The coupling bridge in the logo
- Focus rings (2px XO Gold outline for accessibility)

### 3.3 Engine Accent Colors — All 34 Engines

Each engine has one accent color. It fills knob arcs, active indicators, section headers, and the engine panel's identity band. The coupling strip uses a blend of both active engines' accent colors.

| Engine (Gallery Code) | Source Name | Accent Color | Hex |
|----------------------|-------------|--------------|-----|
| ODDFELIX | OddfeliX | Neon Tetra Blue | `#00A6D6` |
| ODDOSCAR | OddOscar | Axolotl Gill Pink | `#E8839B` |
| OVERDUB | XOverdub | Olive | `#6B7B3A` |
| ODYSSEY | XOdyssey | Violet | `#7B2D8B` |
| OBLONG | XOblong | Amber | `#E9A84A` |
| OBESE | XObese | Hot Pink | `#FF1493` |
| ONSET | XOnset | Electric Blue | `#0066FF` |
| OVERWORLD | XOverworld | Neon Green | `#39FF14` |
| OPAL | XOpal | Lavender | `#A78BFA` |
| ORGANON | XOrganon | Bioluminescent Cyan | `#00CED1` |
| OUROBOROS | XOuroboros | Strange Attractor Red | `#FF2D2D` |
| OBSIDIAN | XObsidian | Crystal White | `#E8E0D8` |
| OVERBITE | XOverbite | Fang White | `#F0EDE8` |
| ORIGAMI | XOrigami | Vermillion Fold | `#E63946` |
| ORACLE | XOracle | Prophecy Indigo | `#4B0082` |
| OBSCURA | XObscura | Daguerreotype Silver | `#8A9BA8` |
| OCEANIC | XOceanic | Phosphorescent Teal | `#00B4A0` |
| OCELOT | XOcelot | Ocelot Tawny | `#C5832B` |
| ORBITAL | XOrbital | Warm Red | `#FF6B6B` |
| OPTIC | XOptic | Phosphor Green | `#00FF41` |
| OBLIQUE | XOblique | Prism Violet | `#BF40FF` |
| OSPREY | XOsprey | Azulejo Blue | `#1B4F8A` |
| OSTERIA | XOsteria | Porto Wine | `#722F37` |
| OWLFISH | XOwlfish | Abyssal Gold | `#B8860B` |
| OHM | XOhm | Sage | `#87AE73` |
| ORPHICA | XOrphica | Siren Seafoam | `#7FDBCA` |
| OBBLIGATO | XObbligato | Rascal Coral | `#FF8A7A` |
| OTTONI | XOttoni | Patina | `#5B8A72` |
| OLE | XOlé | Hibiscus | `#C9377A` |
| OMBRE | XOmbre | Shadow Mauve | `#7B6B8A` |
| ORCA | XOrca | Deep Ocean | `#1B2838` |
| OCTOPUS | XOctopus | Chromatophore Magenta | `#E040FB` |
| OVERLAP | XOverlap | Bioluminescent Cyan-Green | `#00FFB4` |
| OUTWIT | XOutwit | Chromatophore Amber | `#CC6600` |

**V1 Concept Engines (pending DSP — accent colors locked):**

| Engine | Accent Color | Hex |
|--------|-------------|-----|
| OSTINATO | Firelight Orange | `#E8701A` |
| OPENSKY | Sunburst | `#FF8C00` |
| OCEANDEEP | Trench Violet | `#2D0A4E` |
| OUIE | Hammerhead Steel | `#708090` |

**Design notes on accent usage:**
- `Active/On` state: accent at 100% opacity
- `Inactive/Off` state: accent at 20% opacity
- Knob track (unfilled arc): border color at 30% opacity
- Knob fill (filled arc): accent color
- In dark mode, knob tracks use 40% accent (slightly brighter for visibility)
- Two very light accents (OBSIDIAN `#E8E0D8`, OVERBITE `#F0EDE8`) need contrast handling — they are intentionally near-white; use them for texture/identity but rely on stroke/outline rather than fill for state indication

### 3.4 Mood Colors (Preset Browser)

| Mood | Color | Hex |
|------|-------|-----|
| Foundation | Terracotta | `#C8553D` |
| Atmosphere | Teal | `#2A9D8F` |
| Entangled | Gold | `#E9C46A` |
| Prism | Silver | `#B8C4CC` |
| Flux | Crimson | `#C0392B` |
| Aether | Indigo | `#4A3680` |
| Family | (emerging — warm ensemble tones) | TBD |
| Submerged | (emerging — deep water palette) | TBD |

### 3.5 Semantic Colors

```
Error:              #E74C3C
Success:            #27AE60
Warning:            #F39C12

6D Sonic DNA dimensions:
  Brightness:       #FFD700
  Warmth:           #E9A84A
  Movement:         #2A9D8F
  Density:          #7B2D8B
  Space:            #4A3680
  Aggression:       #C0392B
```

### 3.6 The PlaySurface Color Subset

The PlaySurface (performance interface) uses a **dark background** even in light mode — it is a stage, not a gallery wall. Its internal constants:

```
Surface background:  #1A1A1A
Surface card:        #2D2D2D
Text light:          #EEEEEE
Text dim:            #A0A0A0
Amber (XOSEND pad):  #F5C97A
Terracotta (accent): #E07A5F
Teal (OddOscar ref): #2A9D8F
FIRE pad green:      #4ADE80
PANIC pad red:       #EF4444
```

### 3.7 OpticVisualizer Color Subset

The OpticVisualizer (Winamp-style audio-reactive display for the OPTIC engine) uses a dedicated CRT aesthetic palette:

```
Phosphor Green:  #00FF41
Phosphor Dim:    #004D13
CRT Background:  #050A05
Scanline alpha:  0x18 (very faint overlay)
```

### 3.8 Site / Web Palette (XO-OX.org)

The website uses a **distinct dark ocean aesthetic** — different from the plugin's light mode gallery. This is intentional. The site is approved as HT Ammell creative direction ("deep gallery / ocean midnight blues"):

```
Background deep:  #060A10
Background warm:  #0A1018
Card background:  #0E1620
Surface:          #131D28
Gold:             #E9C46A   (same XO Gold — brand constant)
Cream text:       #E4EAF0
Body text:        #B0BEC8
Dim text:         #6B7F8E
Teal accent:      #2AA19B
Coral accent:     #C4636E
Border:           #152330
```

Web typography uses `Outfit` (sans-serif) + `Cormorant Garamond` (serif for editorial display). This is separate from the plugin's font stack — the site is a different medium.

---

## 4. Typography

### 4.1 Plugin Font Stack

Three fonts cover all plugin UI contexts:

| Font | Weight | Use |
|------|--------|-----|
| **Space Grotesk Bold** | Bold | Display/logo — wordmark, hero text, section titles |
| **Inter** | Regular, Medium, Semi-Bold | Body text, labels, preset names, descriptions |
| **JetBrains Mono** | Regular | Parameter values, numeric displays, code |

### 4.2 Type Scale

```
Display:   28px / 32px line-height  — logo, hero text
H1:        20px / 24px              — major section headers
H2:        16px / 20px              — panel titles, zone labels
Body:      13px / 18px              — parameter labels, descriptions
Caption:   11px / 14px              — secondary info, tags
Micro:      9px / 12px              — mood badges, engine tags (uppercase)
```

### 4.3 Case Rules

- Parameter labels: sentence case → `Filter cutoff`, not `FILTER CUTOFF`
- Macro knobs: UPPERCASE → `CHARACTER`, `MOVEMENT`, `COUPLING`, `SPACE` (physical controls)
- Preset names: title case → `Velvet Morning`, `Tape Chaos`
- Engine names: exact brand spelling → `XOblong`, `XOverdub`, `OddfeliX`
- Mood badges: UPPERCASE with 1.5px letter-spacing → `FOUNDATION`, `FLUX`
- Gallery codes: ALLCAPS → `OVERDUB`, `ONSET`, `OPAL`

---

## 5. Layout Architecture

### 5.1 Master Plugin Layout

```
┌─────────────────────────────────────────────────────┐
│  HEADER BAR (fixed — always shell colors)           │
│  [XO_OX logo] [Preset name ◄ ►] [M1][M2][M3][M4]  │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ENGINE PANEL(S)                                    │
│  ┌──────────────┐ ┌──────────────┐                  │
│  │  Engine A     │ │  Engine B     │                 │
│  │  (accent clr) │ │  (accent clr) │                │
│  └──────────────┘ └──────────────┘                  │
│                                                     │
│  COUPLING STRIP (appears when 2+ engines active)    │
│  [engine nodes + coupling arcs — signal flow]       │
│                                                     │
├─────────────────────────────────────────────────────┤
│  PLAYSURFACE                                        │
│  [Zone 1: 4×4 pad grid] [Zone 2: Orbit Path]       │
│  [Zone 3: Performance Strip] [Zone 4: FIRE/SEND/etc]│
├─────────────────────────────────────────────────────┤
│  BOTTOM BAR                                         │
│  [Mood tabs] [Preset browser / DNA Map]             │
└─────────────────────────────────────────────────────┘
```

### 5.2 Engine Panel Layouts by Slot Count

| Active Engines | Layout |
|---------------|--------|
| 1 | Full-width panel, all controls visible, maximum density |
| 2 | 50/50 horizontal split, coupling strip between panels |
| 3 | 33/33/33 split, compact controls (Mini Knobs 24×24), coupling strip |
| 4 | Maximum density — future capacity target |

### 5.3 Spacing System (4px grid)

```
xs:   4px   — inner component padding
sm:   8px   — between related controls
md:  16px   — between control groups
lg:  24px   — between sections
xl:  32px   — between engine panels
```

### 5.4 Corner Radius

```
Buttons:          6px
Cards / panels:  12px
Engine frames:   16px
Modal dialogs:   20px
Knobs:           50% (circular)
```

### 5.5 PlaySurface Desktop Dimensions

From `Source/UI/PlaySurface/PlaySurface.h`:

```
Total surface:   1060 × 344 px
Header strip:       × 32 px
Zone 1 (pads):   480 × 240 px (4×4 grid, bottom-left origin MPC-style)
Zone 2 (orbit):  200 × 180 px (circular XY, 180px diameter)
Zone 3 (strip):  full width × 80 px (horizontal performance strip)
Zone 4 (pads):   100 px wide (4 vertical performance pads)
```

---

## 6. Component Inventory

### 6.1 Knobs (Three Sizes)

**Standard Knob** — engine parameters
- 32×32px, circular track with filled arc
- Track: border color at 30% opacity
- Fill: engine accent color
- Indicator: small dot at arc endpoint, engine accent

**Macro Knob** — header bar (M1–M4)
- 40×40px, thicker 3px arc
- Always XO Gold fill (never engine accent)
- Label below in UPPERCASE
- Subtle gold glow on hover (light mode); gold ring on dark mode

**Mini Knob** — 3-engine compact mode
- 24×24px, same visual language scaled down
- Label to the right instead of below

### 6.2 Buttons

**Toggle Button:**
- Rounded rectangle, 6px radius
- Off: border color outline, transparent fill
- On: engine accent fill, white text
- Transition: 150ms ease-out

**Action Button:**
- Primary: XO Gold fill (Save, Export)
- Secondary: Surface fill + border (Load, Cancel)

**Performance Pads (PlaySurface Zone 4):**
- FIRE: green `#4ADE80`
- XOSEND: amber `#F5C97A`
- ECHO CUT: amber `#F5C97A`
- PANIC: red `#EF4444`

### 6.3 Preset Browser Card

```
┌──────────────────────────────┐
│ ● Foundation   [XOverdub]    │  ← mood dot + engine badge
│                              │
│  Classic Dub Bass            │  ← preset name (Inter Medium 14px)
│                              │
│  ▁▃▅▇▅▃  DNA sparkline       │  ← 6-bar mini chart
│                              │
│  dub · warm · subby     ★    │  ← tags + favorite star
└──────────────────────────────┘
```

### 6.4 DNA Visualization Components

**Sparkline** (in preset card): 6 tiny vertical bars, one per DNA dimension, colored per dimension (see semantic color table in section 3.5).

**Radar Chart** (detail view): Hexagonal radar, 6 axes. Filled area = preset DNA. Second overlay for comparison (morphing/breeding).

**Mood Map** (full browser): 2D scatter plot of all presets. X/Y = PCA components 1/2. Each dot colored by mood. Click to load. Zoom and pan. Auto cluster labels.

### 6.5 Coupling Strip

When 2+ engines active, horizontal strip between panels shows:
- Engine nodes rendered as colored circles (engine accent color)
- Connection arcs between coupled nodes (arc thickness = coupling amount)
- Arc color = blend of both engine accent colors
- Coupling type label at midpoint (e.g. "Amp→Filter")
- Draggable amount control on the arc

The strip background is a **dark background** `#1E1E1E` with an XO Gold header label — one of the few places inside the shell that deliberately breaks light mode to create the "golden corridor" effect.

### 6.6 OpticVisualizer

Winamp/Milkdrop-inspired audio-reactive display for the OPTIC engine. Four modes:
- **Scope** — oscilloscope waveform with phosphor trail
- **Spectrum** — 8-band analyzer with peak hold
- **Milkdrop** — psychedelic particle field with feedback
- **Particles** — granular particle cloud driven by spectral energy

Runs at 30 Hz on UI thread. Falls back to Scope + 10 Hz in reduced motion mode.

### 6.7 Mobile UI Components (iOS)

From `Source/UI/Mobile/`:
- `MobileLayoutManager` — handles iPhone portrait / landscape / iPad layout switching
- `MobilePlaySurface` — touch-optimized play surface
- `MobileTouchHandler` — multi-touch gesture processing
- `ParameterDrawer` — bottom drawer for parameter access (slides up on demand)
- `HapticEngine` — haptic feedback on FIRE/PANIC actions
- `CMMotionManager` — accelerometer/gyroscope routing to parameters (Core Motion; `import CoreMotion`)

**Minimum touch target: 44pt** (Apple HIG compliance). Even 24pt visual knobs expand to 44pt hit areas.

**Mobile layout hierarchy (Gallery Model scaling):**
- Desktop: full museum — all exhibitions visible
- iPad: gallery wing — most zones visible
- iPhone portrait: single room — one exhibition at a time, swipe carousel
- iPhone landscape: performance hall — maximum PlaySurface, minimal preset UI

---

## 7. Animation & Motion

### 7.1 Principles

- Animations are functional, not decorative
- Every animation communicates a state change
- Max duration: 300ms for UI transitions, 1s for page transitions
- Easing: ease-out for entrances, ease-in for exits

### 7.2 Key Animations

| Element | Animation | Duration |
|---------|-----------|----------|
| Engine panel load | Fade in + slide up 8px | 200ms |
| Preset switch | Crossfade parameter values (no flash) | 150ms |
| Coupling strip appear | Height expand from 0 | 250ms |
| Mood tab switch | Slide preset grid horizontally | 200ms |
| DNA sparkline update | Bar heights animate | 300ms |
| Macro knob glow | Pulse on automation | 800ms loop |
| Preset breeding | DNA bars merge + randomize | 500ms |

### 7.3 Performance Constraints

- No animations during audio processing
- GPU-accelerated transforms only (translate, scale, opacity)
- Reduced motion mode: all animations instant (WCAG 2.3.3 compliance)
- OpticVisualizer reduces to 10 Hz in reduced motion mode and falls back to Scope

---

## 8. Iconography

### 8.1 Engine Icons

Each engine has a minimal icon (16×16, 24×24, 32×32) for preset browser badges and engine selectors. All icons are outline-only, 1.5px stroke, colored by context.

| Engine | Icon Concept |
|--------|-------------|
| OddfeliX/OddOscar | Two interlocking circles (X and O) |
| XOverdub | Tape reel |
| XOdyssey | Spiral/journey path |
| XOblong | Rounded rectangle |
| XObese | Bold square with weight |
| XOnset | Grid/matrix of dots |

### 8.2 UI Icons

Minimal line icons, 1.5px stroke weight, round line caps and joins. Monochrome — colored by context.
- Waveform shapes (sine, saw, square, triangle)
- Filter type curves (LP, HP, BP)
- Mod sources (envelope shape, LFO wave)
- Actions (save, load, export, favorite, breed, find similar, find opposite)

### 8.3 Icon Rules

- No filled icons — all outline/stroke only
- Consistent 1.5px stroke at all sizes
- Round caps and joins (no sharp ends)
- Icons are monochrome; context provides color

---

## 9. Logo System

### 9.1 Primary Mark — The Coupling Symbol

Two interlocking rings (X shape + O shape) connected by a visible bridge. The bridge represents coupling. **The bridge is always XO Gold — this never changes.**

```
Variations:
  Logomark only          — Coupled X/O symbol (app icon, favicon)
  Logomark + wordmark    — Symbol left, "XOlokun" right (header bar, marketing)
  Wordmark only          — "XOlokun" in Space Grotesk Bold (tight spaces)
  Full brand             — "XO_OX" above, coupling symbol, "XOlokun" below (splash)
```

### 9.2 Logo Colors

```
Primary mark:       #1A1A1A on light backgrounds
                    #F8F6F3 on dark backgrounds
Coupling bridge:    #E9C46A — ALWAYS gold, regardless of background
Monochrome:         Single color (black or white) for print/merch
```

### 9.3 Logo Don'ts

- Never rotate the logo
- Never change the coupling bridge color (always gold)
- Never separate the X and O symbols
- Never use engine sub-marks as the product logo
- Never place on busy backgrounds without a backing panel

### 9.4 Clear Space

Minimum clear space = width of the "O" in "XOlokun" on all sides.

---

## 10. Brand Identity & Aquatic Mythology

Understanding the mythology is important for visual decisions — engine UI, color choices, and promotional materials all draw from this world.

### 10.1 The Origin Story

In the beginning there were two creatures:

**feliX** — the neon tetra. X energy. Surface, transient, bright, rhythmic, electric. Every note is an arrival.

**Oscar** — the axolotl. O energy. Depth, sustain, warm, patient, regenerative. Every note is a journey.

When feliX and Oscar coupled — X met O — the XO_OX universe was born. Every engine is a species that evolved from that primordial coupling. The brand mark `XO_OX` is their names interleaved.

### 10.2 The Water Column

Every engine lives at a depth in the water column, from open sky (pure feliX) to ocean floor (pure Oscar):

```
☀  OPEN SKY — OPENSKY (pure feliX, V1 pending)
│
~~~~~ THE SURFACE ~~~~~
│  OddfeliX, XOnset
│
│  SUNLIT SHALLOWS
│  XOblique, XOptic, XOrigami, XOstinato (V1 pending)
│
│  THE REEF
│  OddOscar, XOblong, XOracle
│
│  OPEN WATER
│  XOdyssey, XOrbital, XObese
│
│  THERMOCLINE
│  XOverdub, XOuïe (V1 pending), XOpal, XOcelot
│
│  THE DEEP
│  XOverbite, XObsidian, XOrganon
│
│  THE ABYSS
│  XOuroboros, XObscura
│
⚓  OCEAN FLOOR — OCEANDEEP (pure Oscar, V1 pending)
```

### 10.3 The feliX-Oscar Polarity

This is the primary axis for all sound design and visual decision-making:

| feliX (X) | Oscar (O) |
|-----------|-----------|
| Surface | Depth |
| Transient | Sustain |
| Bright | Warm |
| Rhythmic | Patient |
| Electric | Regenerative |
| Fast attack | Slow attack |
| Neon Tetra Blue | Axolotl Gill Pink |

### 10.4 Utility Engines (V2 Identity)

Utility engines are a distinct class from synthesis engines. Synthesis engines = aquatic creatures that MAKE sound. Utility engines = musicians who SHAPE sound. The first utility bundle is the "rapper" bundle (artists who chop, sample, layer, transform). Future bundles follow musical genre archetypes.

---

## 11. Platform Targets & Constraints

### 11.1 Current Shipping Platforms

| Platform | Format | Status |
|----------|--------|--------|
| macOS | AU (Audio Unit) | Shipping — auval PASS |
| macOS | Standalone | Shipping |
| iOS/iPadOS | AUv3 | Spec complete, implementation phase |
| iOS/iPadOS | Standalone | Spec complete, implementation phase |
| macOS | VST3 | V2 scope |

### 11.2 Plugin Window Size

- Desktop minimum: 800px wide (PlaySurface minimum) + header + preset browser
- The PlaySurface alone is 1060×344px
- Full plugin window is larger — no fixed pixel spec found, but layout implies ~1060×700+
- iOS: adaptive layout via `MobileLayoutManager`, full-screen

### 11.3 JUCE Implementation Notes

Design maps to JUCE's `LookAndFeel` system:

```
XOlokunLookAndFeel extends juce::LookAndFeel_V4
Each engine registers its accent color on activation
Shell colors come from ThemeManager (light/dark toggle)
Engine panels create child LookAndFeel overrides
  setting rotarySliderFillColour to their accent
```

The font `juce::Font(name, size, style)` constructor is deprecated in JUCE 8 — there are 5 pre-existing warnings in `XOlokunEditor.h:129-133` from this. Any redesign should use the JUCE 8 `FontOptions` API.

### 11.4 Asset Pipeline Locations

```
assets/
├── icons/
│   ├── engines/    (SVG, 16/24/32px variants — NOT yet created)
│   ├── ui/         (SVG, action/state icons — NOT yet created)
│   └── mood/       (SVG, mood category icons — NOT yet created)
├── fonts/
│   ├── Inter-*.ttf
│   ├── SpaceGrotesk-Bold.ttf
│   └── JetBrainsMono-Regular.ttf
└── images/
    ├── logo-light.svg  (NOT yet created)
    ├── logo-dark.svg   (NOT yet created)
    └── splash.png      (NOT yet created)
```

Icon assets and logo SVGs do not yet exist as files. This is a significant gap — all icons and the logo are described as specifications but not rendered.

---

## 12. Accessibility

### 12.1 Requirements

- All knobs: keyboard-navigable (arrow keys for value change, tab for focus)
- Color is never the sole indicator — always paired with shape or text
- Minimum contrast ratio: 4.5:1 for body text, 3:1 for large text
- Screen reader labels on all interactive elements (`setTitle`/`setDescription` in JUCE)
- Focus ring: 2px XO Gold outline on all focusable elements

### 12.2 Color Blindness

Mood colors were tested for deuteranopia, protanopia, and tritanopia. Foundation (terracotta) and Flux (crimson) are distinguishable by luminance difference. All mood badges include text labels — never color-only.

### 12.3 Reduced Motion

All animations have an instant fallback. OpticVisualizer drops to Scope mode + 10 Hz. This is implemented in code — the designer spec must maintain this path for any new animated component.

---

## 13. Dark Mode

### 13.1 Translation Rules

| Element | Light Mode | Dark Mode |
|---------|-----------|----------|
| Background | `#F8F6F3` | `#1A1A1A` |
| Surface | `#FFFFFF` | `#2D2D2D` |
| Border | `#DDDAD5` | `#4A4A4A` |
| Text primary | `#1A1A1A` | `#EEEEEE` |
| Text secondary | `#6B6965` | `#C8C8C8` |
| Engine accents | Unchanged | Unchanged |
| XO Gold | Unchanged | Unchanged |
| Knob tracks | 30% accent | 40% accent (brighter) |
| Shadows | Subtle drop shadow | Removed — use border |

### 13.2 The Invariance Rule

Engine accent colors and XO Gold **never change between modes**. They are brand constants. Only the neutral shell adapts.

---

## 14. Marketing & Screenshot Standards

### 14.1 Plugin Screenshots

- Always light mode (default brand presentation)
- 2x retina resolution (3840×2160 for 4K display)
- Show a cross-engine preset with coupling visualization active
- Mood Map view as the hero screenshot
- DNA sparklines visible in preset browser

### 14.2 Social Media Templates

- Square 1080×1080 (Instagram/Threads)
- 16:9 1920×1080 (YouTube thumbnails)
- Dark background variant for video overlays
- Always include XO_OX logo + "XOlokun" wordmark
- 2px XO Gold border on all social images

---

## 15. What Needs a Facelift — Pain Points & Gaps

This section documents known design gaps and areas flagged in internal docs as needing attention. These are the direct brief for what a designer consultation should address.

### 15.1 Assets That Don't Exist Yet

**Critical gap:** The design system is fully specified but the assets are not created.

- No SVG files for the logo (light or dark variant)
- No engine icon SVG files (16/24/32px per engine)
- No UI action/state icon set
- No mood category icons
- No splash screen
- No press kit materials

All of these are described as specifications in `xolokun_brand_identity_and_launch.md` but exist only as text, not as files. A designer's first deliverable should be this icon and logo system.

### 15.2 Light-Accent Engines Need Contrast Treatment

OBSIDIAN (`#E8E0D8`) and OVERBITE (`#F0EDE8`) are near-white accents. On a light mode shell (`#F8F6F3`), these engine panels would be nearly invisible — no contrast separation between the engine panel and the gallery wall.

These two engines need a specific design treatment:
- Either a thin border stroke (using the accent as a border rather than a fill)
- Or a background tint for their panels that uses the accent as a very subtle wash
- The current spec does not resolve this — it is an open design problem

### 15.3 OVERWORLD `#39FF14` in Light Mode

Neon Green `#39FF14` is extremely saturated and designed for dark/stage contexts. On the warm white shell it can feel jarring. The OVERWORLD panel may need a light-mode-specific treatment where the accent is desaturated or the panel has a darker inner surface to provide context for the neon.

### 15.4 Coupling Strip Dark Island

The coupling strip renders with a dark background (`#1E1E1E`) even in light mode — creating a deliberate dark "island" within the light shell. This is architecturally intentional (it's "the golden corridor") but visually it can feel disconnected. A designer should evaluate whether this dark strip reads correctly as a corridor or whether it feels like an unfinished panel.

### 15.5 Engine Character Preservation (34 Unique Panels)

The design system specifies that each engine panel should have "texture/material hints" (brushed metal, wood grain, holographic sheen, liquid glass, concrete, PCB) to preserve the character of each standalone instrument. However, only 6 engines have detailed texture specs. The remaining 28 engine panels have no texture guidance — they rely solely on their accent color for visual identity.

This is a significant design gap: 28 engine panels need material/texture identities.

### 15.6 Typography Deprecation Warning

Five warnings exist in `XOlokunEditor.h:129-133` from deprecated `juce::Font(name, size, style)` constructor calls. Any typography redesign must use the JUCE 8 `FontOptions` API. This is a developer/designer handoff item.

### 15.7 Mobile UI Is Spec-Only

The full iOS layout specification exists in `Docs/xolokun_mobile_and_midi_spec.md` and the component headers exist in `Source/UI/Mobile/`, but mobile UI has not been implemented or visually designed beyond the specification text. This is a complete design deliverable pending.

### 15.8 No Visual Hierarchy for 34 Engines in the Browser

With 34 engines (growing toward 40+ in V2), the engine selector UI and preset browser engine badges need a clear visual hierarchy. The current spec describes individual engine panels well but does not address how users navigate between 34 engine choices — how they scan, compare, and select engines. This is an information architecture gap.

### 15.9 PlaySurface Visual Refinement

The PlaySurface uses dark mode internally (stage aesthetic) while the rest of the plugin is light. The boundary between the light engine panels and the dark PlaySurface needs visual design attention — how does this transition feel? Is there a gradient? A hard line? A frame?

---

## 16. Brand Voice Quick Reference

For any copy, UI labels, or marketing materials:

- **Confident, not arrogant** — "This sounds incredible" not "the most advanced synth ever"
- **Warm, not corporate** — Write like a producer talking to another producer
- **Specific, not vague** — "Kick pumps the pad engine's filter" not "innovative sound design"
- **Inviting, not exclusive** — "Load a preset, twist a macro, hear what happens"

---

## 17. CSS Variables (Web / Documentation)

For web-based preset browser, documentation site, or any web component:

```css
:root {
  /* Shell */
  --xo-bg:             #F8F6F3;
  --xo-surface:        #FFFFFF;
  --xo-border:         #E0DCD6;
  --xo-text:           #1A1A1A;
  --xo-text-secondary: #7A7670;

  /* Brand constants */
  --xo-gold:           #E9C46A;
  --xo-gold-light:     #F4DFA0;
  --xo-gold-dark:      #C4A24E;

  /* Moods */
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

## 18. Document Index — Where to Go Deeper

| Topic | Document |
|-------|---------|
| Complete design system spec | `Docs/xolokun_technical_design_system.md` |
| Brand identity, logo, voice | `Docs/xolokun_brand_identity_and_launch.md` |
| Aquatic mythology / engine identities | `Docs/xo_ox_aquatic_mythology.md` |
| Engine identity cards (all 34) | `Docs/engine_identity_cards.md` |
| PlaySurface spec | `Docs/xo_signature_playsurface_spec.md` |
| Mobile layout spec | `Docs/xolokun_mobile_and_midi_spec.md` |
| Live color constants (C++) | `Source/UI/XOlokunEditor.h` (GalleryColors namespace) |
| Coupling strip component | `Source/UI/CouplingStrip/CouplingStripEditor.h` |
| PlaySurface component | `Source/UI/PlaySurface/PlaySurface.h` |
| OpticVisualizer component | `Source/UI/OpticVisualizer/OpticVisualizer.h` |
| Mobile components | `Source/UI/Mobile/` |
| Utility engine concepts (V2) | `Docs/specs/utility_engine_concepts.md` |
| V2 roadmap | `Docs/xolokun_v2_roadmap.md` |

---

*This document was compiled from all design-related sources in the XO_OX-XOmnibus repo on 2026-03-17. The canonical sources listed in Section 18 take precedence if any conflict exists.*
