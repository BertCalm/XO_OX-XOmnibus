# XOceanus UI Components — Dimensions & Layout Specifications

**Report Generated:** 2026-03-26
**Codebase:** XO_OX-XOceanus (Gallery Model)
**Primary Files Analyzed:**
- `Source/UI/Gallery/ParameterGrid.h` (knob grid)
- `Source/UI/Gallery/StatusBar.h` (bottom strip)
- `Source/UI/Gallery/PresetBrowserPanel.h` (preset browser)
- `Source/UI/Gallery/HeaderIndicators.h` (CPU meter, MIDI indicator)
- `Source/UI/Gallery/SidebarPanel.h` (Column C tabbed sidebar)
- `Source/UI/Gallery/ColumnLayoutManager.h` (master layout manager)
- `Source/UI/XOceanusEditor.h` (main window + layout orchestration)

---

## 1. Window & Column Layout

### Main Editor Window
- **Default Size:** 1100 × 700 px
- **Resizable:** Yes, with limits 960 × 600 (min) to 1600 × 1000 (max)

### Column Proportions (at 1100px reference width)

| Component | Width | Height | Notes |
|-----------|-------|--------|-------|
| **Header** | full | 52 px | Surface layer bg, XO logo mark (30×30px centered), "XOceanus" title |
| **Status Bar** | full | 28 px | Bottom strip, trigger pads + status labels + slot dots + lock button |
| **PlaySurface** | full | 220 px | Floating performance interface (when visible, collapses if body < 320px) |
| **FieldMap** | Column B | 65 px | Bottom strip of Column B (wavetable visualizer, etc.) |
| **Column A** | 260 px | `bodyH` | Engine rack (4 primary tiles + ghost tile) |
| **Column B** | 520 px | `bodyH` | Main canvas (overview/detail/chord/perf panels) |
| **Column C** | 320 px | `bodyH` | Sidebar (tabs: PRESET/COUPLE/FX/PLAY/EXPORT/SETTINGS) |

**Scaling Rules:**
- Reference resolution: 1100 px width
- At < 960 px: proportional shrink (minimums: ColA 180, ColB 400, ColC 48)
- At ≤ 800 px (MPC mode): 2-column fallback (ColA + ColB only, ColC hidden)
- Cinematic mode: ColA + ColC collapse, ColB → full width

---

## 2. ParameterGrid Component

**Location:** `Source/UI/Gallery/ParameterGrid.h`

### Layout Constants (Lines 563–571)

```
kCellW            = 60 px     — width of each knob cell
kCellH            = 52 px     — height of each knob cell
kKnobSize         = 32 px     — diameter of rotary knob widget
kPad              = 8 px      — top/bottom padding
kHeaderRowH       = 22 px     — height of section header strip (one per section)
kVisibilityMargin = 100 px    — pre-load margin for smooth scrolling
```

### Grid Composition

**Grid structure in Column B (~490px available width):**
- Knobs per row: ~8 columns (490 / 60 = ~8.17 cells)
- Knob positioning within cell:
  - **Knob X:** `cx + (kCellW - kKnobSize) / 2` = `cx + 14 px` (centered)
  - **Knob Y:** `cy` (top-aligned within cell)
  - **Label Y:** `cy + kKnobSize + 4 px` = `cy + 36 px` (below knob with 4px gap)
  - **Label height:** 12 px
  - **Label font:** JetBrains Mono 8pt (T3 color)

**Section Headers (Lines 284–297):**
- **Font:** Space Grotesk Bold 9.5px (uppercase)
- **Height:** 22 px (kHeaderRowH)
- **Content:**
  - 7×7 px colored dot (left, 10px from edge, vertically centered)
  - Section title (OSC/FILTER/MOD/FX/MACRO/OTHER)
  - Collapse arrow (right-aligned, T3 color)
  - Bottom border: 1px @ `border()` color (rgba white 7%)

**Section Colors & Dots (Lines 73–84):**
| Section | Color | Hex Code |
|---------|-------|----------|
| OSC | Sunlit Blue | `#48CAE4` |
| FILTER | Warm Red | `#FF6B6B` |
| MOD | Phosphor Green | `#00FF41` |
| FX | Prism Violet | `#BF40FF` |
| MACRO | XO Gold | `#E9C46A` |
| OTHER | Text Muted | `#9E9B97` |

**Left Border Bar (Line 323):**
- Width: 3 px
- Color: section color @ 45% alpha
- Position: left edge of knob cell (cx, cy + 2)

### Lazy Attachment Strategy

- Timer interval: 10 Hz
- Pre-load margin: 100 px (knobs created when viewport edge is 100 px away)
- Destruction on scroll-out: immediate teardown when cell leaves padded-visible-rect
- Knob count scales with viewport size; full param list always drives geometry

---

## 3. StatusBar Component

**Location:** `Source/UI/Gallery/StatusBar.h`

### Fixed Height
- **Height:** 28 px (StatusBar fills this strip at bottom)

### Layout (Lines 234–269)

**Left Section — Trigger Pads (Lines 235–248):**
```
padW            = 52 px
padH            = 26 px  — (UX12: increased from 20 for touch target)
padGap          = 4 px   — gap between adjacent pads
padTop          = (statusBarH - padH) / 2
x               = 6 px   — left margin

Buttons (in order):
  FIRE        (x=6,   padTop, 52×26)
  XOSEND      (x=62,  padTop, 52×26)
  ECHO CUT    (x=118, padTop, 52×26)
  PANIC       (x=174, padTop, 52×26)
```

**Center Section — Status Labels (Lines 256–269):**
```
Labels divide the center space into thirds:
  bpmLabel      (centreLeft,              0, labelW, statusBarH)
  voiceLabel    (centreLeft + labelW,     0, labelW, statusBarH)
  cpuLabel      (centreLeft + labelW*2,   0, labelW, statusBarH)

Font: JetBrains Mono 10pt, centered
Colors:
  bpmLabel:    T2 (secondary text)
  voiceLabel:  T3 (tertiary text)
  cpuLabel:    T3 (tertiary text)
```

**Right Section — Slot Dots (Lines 185–228):**
```
dotR            = 4 px       — radius (7px diameter dots)
dotDia          = 8 px       — diameter
dotSpacing      = 4 px       — gap between dots
dotsWidth       = kNumSlots * dotDia + (kNumSlots - 1) * dotSpacing
                = 5 * 8 + 4 * 4 = 56 px total

Position:
  dotY          = statusBarH / 2 (vertically centered)
  dotsRight     = getWidth() - 6 - 22 - 6 - 2 (before lock button)
  dotsLeft      = dotsRight - dotsWidth

Inactive (idle):   T4 color @ 30% opacity
Active (playing):  engine accent color @ 100%, glow rings (3 layers)
```

**Lock Button (Lines 250–254):**
```
lockW           = 22 px
lockH           = 18 px
lockPad         = 6 px
Position:       (getWidth() - lockPad - lockW, (statusBarH - lockH) / 2)
```

**Top Border:**
- 1 px line at y=0
- Color: `border()` (rgba white 7%)

**Background:**
- `GalleryColors::surface()` (shell white in dark mode)

---

## 4. PresetBrowserPanel Component

**Location:** `Source/UI/Gallery/PresetBrowserPanel.h`

### Default Size
- **Width:** 380 px
- **Height:** 340 px
(Set in constructor, Line 84)

### Layout (Lines 177–219)

**Search Field Row (Lines 182–184):**
```
searchRow       = top 28 px of bounds (reduced by 8,6 margins)
countLabel      = right 56 px of searchRow
searchField     = remaining space, padded (0, 2)
Font:           Inter 11pt
Placeholder:    "Search presets..." (T3 @ 40% opacity)
Background:     elevated() color
Border:         border() color @ 1px
```

**Mood Pill Filter (Lines 188–213):**
```
pillH           = 20 px
hGap            = 4 px (horizontal gap)
vGap            = 4 px (vertical gap)
16 pills total  = ALL + 15 moods (Foundation, Atmosphere, Entangled, Prism, Flux, Aether, Family, Submerged, Coupling, Crystalline, Deep, Ethereal, Kinetic, Luminous, Organic)

Pill widths:    measured per-button, min 32px, text-based width + 18px padding
Text font:      Inter 9pt
Border-radius:  10 px (pill shape)
Padding:        2px vertical, 8px horizontal
Active:         XO Gold background (14% opacity) + XO Gold text
Inactive:       transparent background, T3 text
Flex-wrap:      auto-wrap to next row when needed
```

**Preset List (Lines 66–74, 217–218):**
```
listBox.rowHeight = 32 px per preset row (7px padding + content)
Background:       transparent (parent paints bg)
Border:           border() color @ 1px outline
Width:            fill remaining space
Height:           fill remaining space after search + mood pills
```

**List Item Paint (Lines 90–151):**

For each preset row:
```
Selected BG:      rgba white 4.5% fill (0x0BFFFFFF)
Selected border:  2px left bar in accent color

Mood pip:         5×5 px circle @ 70% opacity
                  Positioned at (10, centerY - 2.5)
                  Color varies by mood (15 mood-specific colors defined Lines 108–124)

Preset name:      left-aligned, Inter 11.5pt, T1/T2 color
                  Bounds: (22, 0, w-36, h)

Engine badge:     right-aligned, JetBrains Mono 8pt, T3 @ 50%
                  3-char uppercase (e.g., "ODD")
                  Bounds: (w-28, 0, 26, h)
                  Only shown if multi-engine preset
```

**Count Label (Lines 76–81):**
```
Font:     JetBrains Mono 8.5pt
Color:    textMid() @ 55% opacity
Position: right-aligned above list
Text:     "{n} presets"
```

---

## 5. HeaderIndicators Component

**Location:** `Source/UI/Gallery/HeaderIndicators.h`

### CPUMeter (Lines 41–98)

**Size:**
- Width: 60 px (implicit, pill shape)
- Height: 20 px

**Layout:**
```
Background:     slotBg() @ 85% opacity, 3px rounded corners
Text:           "CPU: X.X%" (one decimal place)
Font:           JetBrains Mono 10pt
Position:       padding (4px horizontal, 0 vertical)
Border:         1px @ border() color, 3px radius
```

**Color Coding:**
| CPU Load | Color | Hex |
|----------|-------|-----|
| < 30% | Green | `#4ADE80` |
| 30–70% | Amber | `#F5C97A` |
| > 70% | Red | `#EF4444` |

### MIDIActivityIndicator (Lines 117–243)

**Size:**
- Width: 8 px (default, fills available)
- Height: 8 px

**Visual States:**

1. **Idle:** T4 color @ 30% opacity (barely visible)
2. **Flash (note-on):**
   - Engine accent color @ 100% opacity
   - Alpha decays over ~200ms
   - Decay rate: 0.15 per tick @ 30 Hz = ~6.7 ticks to 0
3. **Learning (MIDI learn active):**
   - Amber `#F5C97A` pulsing @ ~2Hz
   - Brightness = 0.5 + 0.5 * sin(learnPhase)
   - Phase step: 0.21f rad/tick @ 30Hz ≈ 1 Hz perceived "breathing"

**Paint Details (Lines 156–199):**
```
Dot diameter:   computed from bounds (aspect ratio 1:1)
Ring (optional): darker(0.35) of dot color when active
Ring alpha:     learning mode: 0.3 + 0.3*sin(phase)
                flash mode: flashAlpha * 0.4
```

---

## 6. SidebarPanel Component (Column C)

**Location:** `Source/UI/Gallery/SidebarPanel.h`

### Tab Bar (Lines 415–417)

**Fixed Dimensions:**
```
kTabBarH        = 38 px   — tab button bar height
kUnderlineH     = 2 px    — active tab bottom accent
```

**Tab Buttons (6 total, Lines 344–360):**
```
Tab labels:     PRESET, COUPLE, FX, PLAY, EXPORT, SETTINGS
Font:           Space Grotesk SemiBold 9.5pt (display), uppercase
Widths:         proportional (text width + 22px padding)

Active tab:     full-opacity text + 2px XO Gold underline
Inactive tab:   T3 color @ 75% opacity (rgba 80,76,70,0.75)
Background:     surface() color
Border:         border() line @ 1px (bottom of tab bar)
```

**Content Area (below tab bar):**
```
Height:         sidebar height - 38px
Padding:        8px all sides
Tabs shown:     PRESET (PresetBrowser), COUPLE (CouplingInspector), FX (FXInspector),
                PLAY (PlayControlPanel), EXPORT (ExportTabPanel), SETTINGS (SettingsPanel)
```

**Collapsed Mode (< 48px width):**
```
Displays vertical icon strip instead of full tabs:
  One letter per tab (P/C/F/P/E/S)
  Font: display 11pt
  Active indicator: 2px left bar in XO Gold
  Icon color: T1 active, T3 inactive
```

**Export Tab Sublayout (Lines 373–387):**
```
ExportTabPanel:         top (fill available)
OutshineSidebarPanel:   bottom 180px
Divider:                natural from setBounds() calls
```

---

## 7. Overall Window Layout at 1100×700

```
┌─────────────────────────────────────────────────────┐ y=0
│           HEADER (52px)                             │
│  [Logo] "XOceanus"  [ENGINES▾] [CM] [P] [PS] [DK]   │
├──────────────┬──────────────────────────┬───────────┤ y=52
│              │                          │           │
│  Column A    │      Column B            │ Column C  │
│  (260px)     │      (520px)             │ (320px)   │
│              │                          │           │
│  Tiles 1-4   │  Overview/Detail/etc.   │ Tabs:     │
│  Ghost tile  │                          │ PRESET    │
│              │  ┌──────────────────┐    │ COUPLE    │
│              │  │ FieldMap (65px)  │    │ FX        │
│              │  └──────────────────┘    │ PLAY      │
│              │                          │ EXPORT    │
├──────────────┴──────────────────────────┴───────────┤ y=672
│  STATUS BAR (28px)                                  │
│  [FIRE][XOSEND][ECHO CUT][PANIC] | BPM Voices CPU │ ●●●●● 🔒│
└─────────────────────────────────────────────────────┘ y=700
```

---

## 8. Typography Constants (GalleryFonts)

Used consistently across all components:

| Usage | Font | Size | Weight |
|-------|------|------|--------|
| **Section headers** | Space Grotesk | 9.5 pt | Bold (600) |
| **Tab labels** | Space Grotesk | 9.5 pt | SemiBold (600) |
| **Display (window title)** | Space Grotesk | 12 pt | SemiBold |
| **Body text** | Inter | 11–11.5 pt | Regular |
| **Values / labels** | JetBrains Mono | 8–10 pt | Regular |
| **Status labels** | JetBrains Mono | 10 pt | Regular |
| **Knob labels** | JetBrains Mono | 8 pt | Regular |

---

## 9. Color Palette (GalleryColors)

### Semantic Colors

| Role | Variable | Dark Mode | Notes |
|------|----------|-----------|-------|
| **Surface** | `surface()` | `#1A1A1C` | One level above shell |
| **Shell** | `shellWhite()` | `#0E0E10` | Background layer |
| **Body** | (hardcoded) | `#080809` | Below shell, editor interior |
| **Border** | `border()` | rgba(255,255,255,0.07) | Separator lines |
| **T1** | `t1()` | Primary text | Highest contrast |
| **T2** | `t2()` | Secondary text | Medium contrast |
| **T3** | `t3()` | Tertiary text | Subtle |
| **T4** | `t4()` | Minimal text | Barely visible |
| **XO Gold** | `xoGold` | `#E9C46A` | Accent (macros, active states) |
| **Elevated** | `elevated()` | `#242426` | Button background |
| **Raised** | `raised()` | `#2C2C2E` | Hover/pressed state |
| **Text Mid** | `textMid()` | Secondary mid-tone | Placeholder text |
| **Slot BG** | `slotBg()` | Faint background | Behind slot indicators |
| **Border Gray** | `borderGray()` | `#4A4A4A` | Neutral border |
| **Empty Slot** | `emptySlot()` | Very dark | Empty slot indicator |

### Section Accent Colors (ParameterGrid, Lines 77–84)

| Section | Color | Hex Code | Usage |
|---------|-------|----------|-------|
| OSC | Sunlit Blue | `#48CAE4` | Oscillator parameters |
| FILTER | Warm Red | `#FF6B6B` | Filter parameters |
| MOD | Phosphor Green | `#00FF41` | Modulation/envelope |
| FX | Prism Violet | `#BF40FF` | Effects parameters |
| MACRO | XO Gold | `#E9C46A` | Macro parameters |
| OTHER | Text Muted | `#9E9B97` | Uncategorized |

---

## 10. Key Layout Calculations

### Column B Grid (ParameterGrid in 520px width)

```
Available width:    520 px
Cell width:         60 px
Cells per row:      520 / 60 = 8.66 ≈ 8 columns
Centered knobs:     (60 - 32) / 2 = 14 px offset from cell left edge

Section heights:
  Header:           22 px
  Per knob row:     52 px
  (Row count)       (section param count + 7) / 8  [ceiling division]
```

### StatusBar Geometry

```
Pads:       x = 6; FIRE @ 6, XOSEND @ 62, ECHO CUT @ 118, PANIC @ 174
Centre gap: centreLeft = x + 6 (x = 230), centreRight = getWidth() - 34 - dots - gaps
Dots:       56px wide, right-aligned before lock (34px reserved)
```

---

## File Reference Guide

| File | Purpose | Key Constants |
|------|---------|----------------|
| `ColumnLayoutManager.h` | Master layout equations | kHeaderH=52, kStatusBarH=28, column widths |
| `ParameterGrid.h` | Knob grid auto-layout | kCellW=60, kCellH=52, kKnobSize=32 |
| `StatusBar.h` | Bottom control strip | padW=52, padH=26, dotR=4 |
| `PresetBrowserPanel.h` | Preset modal browser | pillH=20, rowHeight=32 |
| `HeaderIndicators.h` | CPU meter + MIDI flash | dotR=4, flashDecay=0.15/tick |
| `SidebarPanel.h` | Column C tabbed sidebar | kTabBarH=38, kUnderlineH=2 |
| `XOceanusEditor.h` | Main editor window | setSize(1100,700), resizeLimits 960-1600 |
| `GalleryColors.h` | Theme + colors (external) | Dark mode: surface=`#1A1A1C`, shell=`#0E0E10` |
| `GalleryFonts.h` | Typography (external) | display(), body(), value() helpers |

---

**End of Report**
