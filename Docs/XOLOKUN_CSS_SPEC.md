# XOlokun V04 Polished — Complete CSS Design Specification

Reference: `Docs/mockups/xolokun-v04-polished.html`

**Date**: 2026-03-26
**Version**: 1.0
**Target**: JUCE UI implementation must match these exact specifications.

---

## CSS Custom Properties (Design Tokens)

### Color Palette — Background Layers
```css
--bg:         #0E0E10    /* Main background, darkest */
--surface:    #1A1A1C    /* Header, footer, dividers */
--elevated:   #242426    /* Button backgrounds, pills */
--raised:     #2C2C2F    /* Tooltips, overlays */
```

### Color Palette — Text (T1–T4 Hierarchy)
```css
--t1: #F0EDE8    /* Primary text, lightest (headings, labels) */
--t2: #9E9B97    /* Secondary text (subtext, hover) */
--t3: #5E5C5A    /* Tertiary text (meta, disabled) */
--t4: #3A3938    /* Quaternary text, dimmest (very faint) */
```

### Color Palette — Accent (Teal/Cyan)
```css
--accent:         #1E8B7E       /* Primary accent, solid teal */
--accent-dim:     rgba(30,139,126,0.14)    /* 14% opacity */
--accent-glow:    rgba(30,139,126,0.32)    /* 32% opacity (glows, shadows) */
--accent-bright:  rgba(30,139,126,0.70)    /* 70% opacity (active states) */
```

### Color Palette — Gold (Secondary)
```css
--gold:       #E9C46A               /* Solid gold, buttons, badges */
--gold-dim:   rgba(233,196,106,0.14)   /* 14% opacity */
--gold-glow:  rgba(233,196,106,0.28)   /* 28% opacity (glows) */
```

### Color Palette — Borders
```css
--border:     rgba(255,255,255,0.07)    /* Light dividers, inactive borders */
--border-md:  rgba(255,255,255,0.11)    /* Medium, hover states */
```

### Color Palette — Additional
```css
/* Used in status bar only */
--panic-red:  #FF6B6B     /* Panic button color */

/* Dark background behind plugin */
body-background: #080809    /* Outer page background */
```

### Font Families
```css
--f-display: 'Space Grotesk', system-ui, sans-serif    /* Headers, buttons, nav */
--f-body:    'Inter', system-ui, sans-serif             /* Body text, labels */
--f-mono:    'JetBrains Mono', monospace                /* Code, values, meta */
```

### Animation & Easing
```css
--ease-out:    cubic-bezier(0.0, 0.0, 0.2, 1)   /* Smooth fade out */
--ease-snap:   cubic-bezier(0.34, 1.56, 0.64, 1) /* Bouncy, snappy */
--dur-fast:    120ms                              /* Quick interactions */
--dur-mid:     220ms                              /* Standard transitions */
--dur-slow:    380ms                              /* Slow, attention-grabbing */
```

---

## Global Layout Dimensions

### Plugin Shell
- **Width**: 1100px
- **Height**: 700px
- **Border radius**: 12px
- **Background**: var(--bg) — #0E0E10
- **Box shadow**:
  ```
  0 0 0 1px rgba(255,255,255,0.055)     /* Inner border outline */
  0 4px 12px rgba(0,0,0,0.5)             /* Close shadow */
  0 20px 60px rgba(0,0,0,0.6)            /* Mid shadow */
  0 60px 120px rgba(0,0,0,0.4)           /* Far shadow */
  ```
- **User select**: none
- **Flex layout**: column

---

## HEADER (52px fixed height)

### Header Container
- **Height**: 52px (min-height: 52px)
- **Display**: flex
- **Align items**: center
- **Padding**: 0 14px
- **Gap**: 10px
- **Background**: var(--surface) — #1A1A1C
- **Border bottom**: 1px solid var(--border)
- **z-index**: 20
- **Position**: relative

### Logo Mark
- **Width**: 30px
- **Height**: 30px
- **Flex shrink**: 0
- **Display**: flex (centered)
- SVG: two overlapping circles (accent teal + gold)

### Engines Button
- **Font family**: var(--f-display)
- **Font size**: 10px
- **Font weight**: 600
- **Letter spacing**: 0.16em
- **Text transform**: uppercase
- **Color**: var(--t2) (hover: var(--t1))
- **Background**: var(--elevated)
- **Border**: 1px solid var(--border) (hover: var(--border-md))
- **Border radius**: 5px
- **Padding**: 5px 10px
- **Cursor**: pointer
- **Transition**: color 120ms, border-color 120ms
- **White space**: nowrap

### Preset Navigation
- **Layout**: flex, gap: 6px, align items center

#### Preset Arrow Buttons (prev/next)
- **Width/Height**: 22px
- **Border**: 1px solid var(--border-md) (hover: rgba(255,255,255,0.22))
- **Border radius**: 4px
- **Color**: var(--t2) (hover: var(--t1))
- **Font size**: 9px
- **Transition**: all 120ms

#### Preset Name Display
- **Font family**: var(--f-body)
- **Font size**: 12.5px
- **Font weight**: 500
- **Color**: var(--t1)
- **Max width**: 160px
- **Overflow**: hidden, text-overflow ellipsis, white-space nowrap

#### Preset Icon Buttons (favorite, DNA)
- **Width/Height**: 22px
- **Background**: none
- **Border**: none
- **Color**: var(--t3) (hover: var(--t1), active: var(--gold))
- **Font size**: 12px
- **Border radius**: 3px
- **Transition**: color 120ms

#### A/B Toggle Group
- **Display**: flex
- **Border**: 1px solid var(--border-md)
- **Border radius**: 4px
- **Overflow**: hidden
- **Margin left**: 2px
- **Gap**: 0

##### A/B Toggle Buttons
- **Font family**: var(--f-mono)
- **Font size**: 10px
- **Font weight**: 500
- **Padding**: 4px 8px
- **Color**: var(--t3) (active: var(--t1), hover: var(--t2))
- **Active**: background var(--elevated)
- **Transition**: all 120ms

#### Undo Button
- **Width/Height**: 22px
- **Border**: 1px solid var(--border) (hover: var(--border-md))
- **Border radius**: 4px
- **Color**: var(--t3) (hover: var(--t1))
- **Font size**: 11px
- **Transition**: all 120ms

### Header Macros (4× 40px knobs)
- **Display**: flex
- **Gap**: 8px
- **Margin left**: auto
- **Knob size**: 40px (rendered as SVG/canvas)

#### Macro Group
- **Display**: flex column
- **Gap**: 2px
- **Align items**: center

#### Macro Label
- **Font family**: var(--f-mono)
- **Font size**: 7px
- **Letter spacing**: 0.1em
- **Color**: var(--gold) at 65% opacity
- **Text transform**: uppercase

### Header Indicators (right side)
- **Display**: flex
- **Gap**: 6px
- **Align items**: center
- **Margin left**: 10px

#### Indicator Pills
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Font weight**: 500
- **Letter spacing**: 0.08em
- **Color**: var(--t3) (hover: var(--t2), active: var(--gold))
- **Padding**: 3px 7px
- **Border radius**: 3px
- **Background**: var(--elevated)
- **Border**: 1px solid var(--border) (active: rgba(233,196,106,0.25))
- **Transition**: all 120ms

#### CPU Display
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Color**: var(--t3)
- **Padding**: 3px 6px

#### Settings Button
- **Width/Height**: 28px
- **Border**: 1px solid var(--border) (hover: var(--border-md))
- **Border radius**: 5px
- **Color**: var(--t3) (hover: var(--t1))
- **Font size**: 13px
- **Transition**: all 120ms

#### Export Button (golden, prominent)
- **Font family**: var(--f-display)
- **Font size**: 9.5px
- **Font weight**: 600
- **Letter spacing**: 0.16em
- **Text transform**: uppercase
- **Color**: #1A1A1C (dark text on gold)
- **Background**: var(--gold) — #E9C46A
- **Hover**: #F0CE7A
- **Border**: none
- **Border radius**: 4px
- **Padding**: 5px 12px
- **Box shadow**: 0 2px 8px var(--gold-glow), hover 0 4px 14px rgba(233,196,106,0.4)

---

## MAIN BODY (flex: 1)
- **Display**: flex
- **Overflow**: hidden
- **Layout**: row (3-column: A, B, C)

---

## COLUMN A: Engine Rack (260px fixed width)

### Col A Container
- **Width**: 260px (min-width: 260px)
- **Display**: flex column
- **Border right**: 1px solid var(--border)
- **Overflow**: hidden

### Slots Container
- **Flex**: 1
- **Display**: flex column
- **Overflow**: hidden

### Engine Slot Tile (6 instances)
- **Display**: flex column
- **Padding**: 9px 11px 8px 14px
- **Border bottom**: 1px solid var(--border)
- **Cursor**: pointer
- **Flex**: 1
- **Background**: transparent (hover: rgba(255,255,255,0.025), active: rgba(255,255,255,0.04))
- **Transition**: background 120ms

#### Slot Left Accent Bar (::before pseudo)
- **Content**: ''
- **Position**: absolute, left: 0, top: 0, bottom: 0
- **Width**: 3px
- **Background**: var(--slot-accent, var(--t4)) — dynamically set per slot
- **Border radius**: 0 2px 2px 0
- **Transition**: background 220ms
- **Active**: box-shadow 0 0 8px var(--slot-accent)

#### Slot Header
- **Display**: flex
- **Gap**: 6px
- **Align items**: center
- **Margin bottom**: 5px

##### Slot Number
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Color**: var(--t3)
- **Width**: 12px

##### Slot Name
- **Font family**: var(--f-display)
- **Font size**: 11px
- **Font weight**: 600
- **Letter spacing**: 0.13em
- **Text transform**: uppercase
- **Color**: var(--slot-accent) — transitions with accent bar
- **Transition**: color 220ms
- **Flex**: 1

##### Slot Power Button
- **Width/Height**: 16px
- **Border**: 1px solid var(--border-md)
- **Border radius**: 50%
- **Background**: none
- **Color**: var(--t3) (hover: var(--t1), on: var(--slot-accent))
- **Font size**: 8px
- **Cursor**: pointer
- **Transition**: all 120ms
- **Flex shrink**: 0

#### Slot Macros (4 knobs)
- **Display**: flex
- **Gap**: 5px
- **Margin bottom**: 5px
- **Knob size**: 24px each

#### Slot Waveform
- **Height**: 22px
- **Margin bottom**: 5px
- **Border radius**: 3px
- **Background**: rgba(255,255,255,0.025)
- **SVG polyline**: stroke var(--slot-accent), stroke-width 1.2, opacity 0.7

#### Slot Footer
- **Display**: flex
- **Justify content**: space-between
- **Align items**: center

##### Mood Dots (4 dots)
- **Display**: flex
- **Gap**: 4px
- **Width/Height**: 6px each
- **Border radius**: 50%
- **Background**: var(--slot-accent) at opacity 0.6
- **.dim state**: opacity 0.22

##### FX Indicator (dots + label)
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Color**: var(--t4)
- **Display**: flex
- **Gap**: 3px
- **Align items**: center

###### FX Dots (3 dots)
- **Width/Height**: 5px
- **Border radius**: 50%
- **Background**: var(--t4) (on: var(--gold))

#### Slot CPU Bar
- **Height**: 3px
- **Margin top**: 5px
- **Background**: rgba(255,255,255,0.06)
- **Border radius**: 2px
- **Overflow**: hidden

##### CPU Fill
- **Height**: 100%
- **Border radius**: 2px
- **Background**: var(--slot-accent) at opacity 0.55
- **Transition**: width 0.8s ease

### Empty Slot (add engine)
- **Display**: flex (centered)
- **Flex**: 1
- **Cursor**: pointer
- **Background**: transparent (hover: rgba(255,255,255,0.02))
- **Border bottom**: 1px solid var(--border)

#### Plus Icon
- **Width/Height**: 28px
- **Border**: 1px dashed var(--border-md)
- **Border radius**: 6px
- **Font size**: 16px
- **Color**: var(--t4) (hover: var(--t3))
- **Transition**: all 120ms

#### Empty Label
- **Font family**: var(--f-body)
- **Font size**: 10px
- **Color**: var(--t4) (hover: var(--t3))

### Coupling Mini-Graph
- **Height**: 80px
- **Padding**: 8px 11px 6px
- **Border top**: 1px solid var(--border)
- **Display**: flex column
- **Gap**: 4px

#### Coupling Label
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Letter spacing**: 0.1em
- **Color**: var(--t3)
- **Text transform**: uppercase
- **Display**: flex
- **Gap**: 5px
- **Align items**: center

#### Coupling Dots
- **Width/Height**: 5px
- **Border radius**: 50%
- **Background**: var(--gold)
- **Opacity**: 0.7

---

## COLUMN B: Main Engine Parameters (flex: 1, min-width: 0)

### Col B Container
- **Display**: flex column
- **Border right**: 1px solid var(--border)
- **Overflow**: hidden

### Engine Nameplate
- **Padding**: 10px 16px 0
- **Display**: flex
- **Gap**: 10px
- **Align items**: center

#### Engine Name (Large)
- **Font family**: var(--f-display)
- **Font size**: 13px
- **Font weight**: 700
- **Letter spacing**: 0.22em
- **Text transform**: uppercase
- **Color**: var(--accent)
- **Text shadow**: 0 0 16px var(--accent-glow)
- **Transition**: color 380ms, text-shadow 380ms

#### Engine Tagline
- **Font family**: var(--f-body)
- **Font size**: 10px
- **Color**: var(--t3)
- **Flex**: 1

#### Randomize Button
- **Width/Height**: 22px
- **Border**: 1px solid var(--border) (hover: var(--border-md))
- **Border radius**: 4px
- **Color**: var(--t3) (hover: var(--t1))
- **Font size**: 12px
- **Transition**: all 120ms

### Signal Flow Strip
- **Display**: flex
- **Padding**: 5px 16px 7px
- **Gap**: 0
- **Border bottom**: 1px solid var(--border)
- **Align items**: center

#### Signal Flow Blocks
- **Font family**: var(--f-mono)
- **Font size**: 8.5px
- **Font weight**: 500
- **Letter spacing**: 0.06em
- **Text transform**: uppercase
- **Color**: var(--t3) (hover: var(--t1), active: var(--accent))
- **Padding**: 3px 8px
- **Border radius**: 3px
- **Cursor**: pointer
- **White space**: nowrap
- **Transition**: all 120ms

#### Signal Flow Arrows (between blocks)
- **Font size**: 9px
- **Color**: var(--t4)
- **Padding**: 0 2px
- **Flex shrink**: 0

### Macros Row (4 large knobs, ~40px each)
- **Display**: flex
- **Justify content**: space-around
- **Align items**: flex-start
- **Padding**: 12px 16px 10px
- **Border bottom**: 1px solid var(--border)
- **Background**: linear-gradient(180deg, rgba(255,255,255,0.015) 0%, transparent 100%)

#### Macro Group
- **Display**: flex column
- **Gap**: 5px
- **Align items**: center

#### Macro Label
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Letter spacing**: 0.14em
- **Text transform**: uppercase
- **Color**: var(--gold) at opacity 0.75

#### Macro Value
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Color**: var(--t3)

### Collapsible Sections (scrolling)
- **Flex**: 1
- **Overflow-y**: auto
- **Overflow-x**: hidden
- **Scrollbar width**: thin
- **Scrollbar color**: var(--t4) transparent

#### Section
- **Border bottom**: 1px solid var(--border)

#### Section Header (clickable)
- **Display**: flex
- **Height**: 32px
- **Padding**: 0 16px
- **Gap**: 7px
- **Cursor**: pointer
- **Align items**: center
- **Transition**: background 120ms
- **Background**: transparent (hover: rgba(255,255,255,0.025), expanded: rgba(255,255,255,0.03))

##### Section Color Dot
- **Width/Height**: 7px
- **Border radius**: 50%
- **Flex shrink**: 0
- **Background**: theme color (varies per section)

##### Section Title
- **Font family**: var(--f-display)
- **Font size**: 9.5px
- **Font weight**: 600
- **Letter spacing**: 0.18em
- **Text transform**: uppercase
- **Color**: var(--t2)
- **Flex**: 1

##### Section Collapse Arrow
- **Font size**: 9px
- **Color**: var(--t4)
- **Transition**: transform 220ms
- **Expanded state**: rotate(90deg)

##### Section Tint (overlay background)
- **Position**: absolute (inset: 0)
- **Opacity**: 0.04
- **Pointer events**: none

#### Section Body (content area)
- **Display**: flex (when expanded) / none
- **Flex wrap**: wrap
- **Padding**: 8px 16px 12px
- **Gap**: 10px 14px

#### Parameter Group (knob + label)
- **Display**: flex column
- **Align items**: center
- **Gap**: 3px
- **Min width**: 52px

##### Parameter Label
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Letter spacing**: 0.06em
- **Color**: var(--t3)
- **Text align**: center
- **Max width**: 58px
- **Overflow**: hidden
- **Text overflow**: ellipsis
- **White space**: nowrap

##### Parameter Value
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Color**: var(--t4)

### Col B Bottom (sticky, 80px height)
- **Height**: 80px (min-height: 80px)
- **Border top**: 1px solid var(--border)
- **Display**: flex
- **Padding**: 6px 16px
- **Gap**: 12px
- **Background**: var(--surface)

#### Waveform Canvas Wrap
- **Flex**: 1
- **Display**: flex column
- **Gap**: 3px

##### Wave Label
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Color**: var(--t4)
- **Letter spacing**: 0.1em
- **Text transform**: uppercase

#### ADSR Wrap
- **Flex**: 1
- **Display**: flex column
- **Gap**: 3px

---

## COLUMN C: Browser/FX (320px fixed width)

### Col C Container
- **Width**: 320px (min-width: 320px)
- **Display**: flex column
- **Overflow**: hidden

### Tab Bar
- **Display**: flex
- **Border bottom**: 1px solid var(--border)
- **Background**: var(--surface)
- **Padding**: 0 4px

#### Tab Button
- **Font family**: var(--f-display)
- **Font size**: 9.5px
- **Font weight**: 600
- **Letter spacing**: 0.14em
- **Text transform**: uppercase
- **Color**: var(--t3) (hover: var(--t2), active: var(--t1))
- **Border bottom**: 2px solid transparent (active: var(--accent))
- **Padding**: 0 11px
- **Height**: 38px
- **Cursor**: pointer
- **White space**: nowrap
- **Transition**: all 120ms

### Tab Panel
- **Display**: none (active: flex)
- **Flex direction**: column
- **Flex**: 1
- **Overflow**: hidden

### Preset Browser Panel
- **Display**: flex column
- **Flex**: 1
- **Overflow**: hidden

#### Browser Filters
- **Padding**: 8px 12px 6px
- **Display**: flex column
- **Gap**: 5px
- **Border bottom**: 1px solid var(--border)

##### Filter Pills Row
- **Display**: flex
- **Gap**: 4px
- **Flex wrap**: wrap

##### Filter Pill Button
- **Font family**: var(--f-mono)
- **Font size**: 8.5px
- **Letter spacing**: 0.06em
- **Color**: var(--t3) (hover: var(--t2), active: var(--accent))
- **Background**: var(--elevated) (active: var(--accent-dim))
- **Border**: 1px solid var(--border) (hover: var(--border-md), active: rgba(30,139,126,0.35))
- **Border radius**: 3px
- **Padding**: 3px 8px
- **Cursor**: pointer
- **Transition**: all 120ms

##### Mood Pills Row
- **Display**: flex
- **Gap**: 4px
- **Flex wrap**: wrap

##### Mood Pill Button
- **Font family**: var(--f-body)
- **Font size**: 9px
- **Color**: var(--t3) (hover: var(--t2), active: var(--gold))
- **Background**: none (active: var(--gold-dim))
- **Border**: 1px solid var(--border) (active: rgba(233,196,106,0.35))
- **Border radius**: 10px (pill shape)
- **Padding**: 2px 8px
- **Cursor**: pointer
- **Transition**: all 120ms

#### Preset List (scrolling)
- **Flex**: 1
- **Overflow-y**: auto
- **Overflow-x**: hidden
- **Scrollbar width**: thin
- **Scrollbar color**: var(--t4)
- **Padding**: 4px 0

##### Preset Card
- **Display**: flex
- **Align items**: center
- **Padding**: 7px 12px 7px 14px
- **Gap**: 8px
- **Cursor**: pointer
- **Border left**: 2px solid transparent
- **Background**: transparent (hover: rgba(255,255,255,0.03), active: rgba(255,255,255,0.045))
- **Border left**: transparent (active: var(--accent))
- **Transition**: all 120ms

###### Play Button (circle)
- **Width/Height**: 20px
- **Border radius**: 50%
- **Border**: 1px solid var(--border-md) (hover: rgba(255,255,255,0.25))
- **Background**: none
- **Color**: var(--t3) (hover: var(--t1))
- **Font size**: 8px
- **Cursor**: pointer
- **Flex shrink**: 0
- **Transition**: all 120ms

###### Preset Info
- **Flex**: 1
- **Min width**: 0

###### Preset Title
- **Font family**: var(--f-body)
- **Font size**: 11.5px
- **Font weight**: 500
- **Color**: var(--t1)
- **Overflow**: hidden
- **Text overflow**: ellipsis
- **White space**: nowrap

###### Preset Meta
- **Display**: flex
- **Align items**: center
- **Gap**: 6px
- **Margin top**: 2px

###### Preset Engine Badge
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Letter spacing**: 0.06em
- **Padding**: 1px 5px
- **Border radius**: 2px
- **Border**: 1px solid var(--border)
- **Color**: var(--t3)

###### Preset Mood Pip (color dot)
- **Width/Height**: 5px
- **Border radius**: 50%
- **Flex shrink**: 0
- **Background**: theme color

###### Preset Favorite Star
- **Font size**: 10px
- **Color**: var(--t4) (hover: var(--gold), active: var(--gold))
- **Cursor**: pointer
- **Flex shrink**: 0
- **Transition**: color 120ms

### Couple Tab Panel
- **Padding**: 12px
- **Display**: flex column
- **Gap**: 10px
- **Flex**: 1
- **Overflow-y**: auto

#### Couple Route Card
- **Background**: var(--elevated)
- **Border**: 1px solid var(--border) (hover: var(--border-md))
- **Border radius**: 6px
- **Padding**: 10px 12px
- **Display**: flex column
- **Gap**: 7px
- **Cursor**: pointer
- **Transition**: border-color 120ms

##### Couple Route Header
- **Display**: flex
- **Align items**: center
- **Gap**: 8px

##### Couple From Label
- **Font family**: var(--f-mono)
- **Font size**: 9.5px
- **Font weight**: 500
- **Color**: var(--t2)
- **Letter spacing**: 0.1em

##### Couple Arrow Icon
- **Color**: var(--gold)
- **Font size**: 11px

##### Couple Type Badge
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Letter spacing**: 0.1em
- **Text transform**: uppercase
- **Padding**: 2px 7px
- **Border radius**: 3px
- **Background**: var(--gold-dim)
- **Color**: var(--gold)
- **Border**: 1px solid rgba(233,196,106,0.25)
- **Margin left**: auto

##### Couple Knobs Row
- **Display**: flex
- **Gap**: 12px

##### Couple Knob Group
- **Display**: flex column
- **Align items**: center
- **Gap**: 3px

##### Couple Knob Label
- **Font family**: var(--f-mono)
- **Font size**: 8px
- **Color**: var(--t4)
- **Text transform**: uppercase
- **Letter spacing**: 0.08em

### FX Panel
- **Padding**: 8px 12px
- **Display**: flex column
- **Gap**: 4px
- **Flex**: 1
- **Overflow-y**: auto

#### FX Slot Row
- **Display**: flex
- **Align items**: center
- **Gap**: 8px
- **Padding**: 7px 10px
- **Border**: 1px solid var(--border) (hover: var(--border-md))
- **Border radius**: 5px
- **Background**: var(--elevated)
- **Cursor**: pointer
- **Transition**: border-color 120ms

##### FX Icon
- **Font size**: 13px
- **Width**: 20px
- **Text align**: center

##### FX Name
- **Font family**: var(--f-body)
- **Font size**: 11px
- **Color**: var(--t2)
- **Flex**: 1

##### FX Bypass Dot
- **Width/Height**: 7px
- **Border radius**: 50%
- **Background**: var(--accent)
- **Box shadow**: 0 0 4px var(--accent-glow)
- **Cursor**: pointer
- **Off state**: background var(--t4), box-shadow none

##### FX Wet/Dry Value
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Color**: var(--t3)
- **Width**: 28px
- **Text align**: right

### Export Panel
- **Padding**: 12px
- **Display**: flex column
- **Gap**: 10px
- **Flex**: 1
- **Overflow-y**: auto

#### Export Row
- **Display**: flex
- **Align items**: center
- **Justify content**: space-between
- **Gap**: 8px

##### Export Label
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Color**: var(--t3)
- **Letter spacing**: 0.08em
- **Text transform**: uppercase

##### Export Option Group
- **Display**: flex
- **Gap**: 2px
- **Border**: 1px solid var(--border)
- **Border radius**: 4px
- **Overflow**: hidden

##### Export Option Button
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Padding**: 3px 8px
- **Background**: none (active: var(--elevated))
- **Border**: none
- **Color**: var(--t3) (active: var(--t1))
- **Cursor**: pointer
- **Transition**: all 120ms

##### Export Path
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Color**: var(--t3)
- **Background**: var(--elevated)
- **Border**: 1px solid var(--border)
- **Border radius**: 3px
- **Padding**: 4px 8px
- **Flex**: 1
- **Overflow**: hidden
- **Text overflow**: ellipsis
- **White space**: nowrap

##### Export Big Button
- **Width**: 100%
- **Font family**: var(--f-display)
- **Font size**: 11px
- **Font weight**: 700
- **Letter spacing**: 0.2em
- **Text transform**: uppercase
- **Color**: #1A1A1C (dark)
- **Background**: var(--gold) — #E9C46A
- **Hover**: #F0CE7A
- **Border**: none
- **Border radius**: 6px
- **Padding**: 10px
- **Box shadow**: 0 4px 16px var(--gold-glow)
- **Cursor**: pointer
- **Transition**: all 120ms

### Settings Panel
- **Padding**: 12px
- **Display**: flex column
- **Gap**: 8px
- **Flex**: 1

#### Settings Row
- **Display**: flex
- **Align items**: center
- **Justify content**: space-between
- **Padding**: 6px 0
- **Border bottom**: 1px solid var(--border)

##### Settings Label
- **Font family**: var(--f-body)
- **Font size**: 11px
- **Color**: var(--t2)

#### Toggle Switch Component
- **Width**: 32px
- **Height**: 18px
- **Border radius**: 9px
- **Background**: var(--t4) (on: var(--accent))
- **Cursor**: pointer
- **Transition**: background 120ms
- **Position**: relative

##### Toggle Knob
- **Position**: absolute
- **Width**: 14px
- **Height**: 14px
- **Border radius**: 50%
- **Background**: var(--t1)
- **Top**: 2px
- **Left**: 2px (on: translateX(14px))
- **Transition**: transform 120ms

---

## KNOB COMPONENT

### Knob Wrap
- **Position**: relative
- **Display**: flex
- **Align items**: center
- **Justify content**: center
- **Cursor**: ns-resize

### Knob Body
- **Border radius**: 50%
- **Position**: relative
- **Display**: flex
- **Flex shrink**: 0
- **Size**: varies by context:
  - Header macros: 40px
  - Slot macros: 24px
  - Section parameters: varies (fits 52px min-width)

### Knob SVG
- **Position**: absolute (inset: 0)
- **Width/Height**: 100%
- **Pointer events**: none

---

## STATUS BAR (28px fixed height)

### Status Bar Container
- **Height**: 28px (min-height: 28px)
- **Display**: flex
- **Align items**: center
- **Padding**: 0 14px
- **Gap**: 10px
- **Background**: var(--surface)
- **Border top**: 1px solid var(--border)

### Status Action Buttons
- **Font family**: var(--f-mono)
- **Font size**: 8.5px
- **Font weight**: 500
- **Letter spacing**: 0.12em
- **Text transform**: uppercase
- **Color**: var(--t3) (hover: var(--t1))
- **Background**: var(--elevated)
- **Border**: 1px solid var(--border) (hover: var(--border-md))
- **Border radius**: 3px
- **Padding**: 3px 8px
- **Cursor**: pointer
- **Transition**: all 120ms
- **.fire variant**: color var(--accent-bright), border-color rgba(30,139,126,0.25)
- **.panic variant**: color #FF6B6B, border-color rgba(255,107,107,0.25)

### Status Divider
- **Width**: 1px
- **Height**: 14px
- **Background**: var(--border)

### Status BPM Display
- **Font family**: var(--f-mono)
- **Font size**: 9.5px
- **Color**: var(--t2)

### Status Label
- **Font family**: var(--f-mono)
- **Font size**: 8.5px
- **Color**: var(--t4)
- **Letter spacing**: 0.08em

### Status Value
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Color**: var(--t2)

### Status Spacer
- **Flex**: 1

### Slot Status Dots (right side)
- **Display**: flex
- **Gap**: 4px
- **Align items**: center

#### Slot Dot
- **Width/Height**: 7px
- **Border radius**: 50%
- **Background**: var(--t4)
- **Transition**: background 220ms, box-shadow 220ms
- **.on state**: background var(--accent), box-shadow 0 0 5px var(--accent-glow)
- **.gold state**: background var(--gold), box-shadow 0 0 5px var(--gold-glow)

---

## TOOLTIP

### Tooltip Container
- **Position**: absolute
- **Background**: var(--raised) — #2C2C2F
- **Border**: 1px solid var(--border-md)
- **Border radius**: 5px
- **Padding**: 4px 8px
- **Font family**: var(--f-mono)
- **Font size**: 9px
- **Color**: var(--t1)
- **White space**: nowrap
- **Pointer events**: none
- **z-index**: 100
- **Box shadow**: 0 4px 16px rgba(0,0,0,0.4)
- **Opacity**: 0 (visible: 1)
- **Transition**: opacity 120ms

---

## SCROLLBAR (global)

### Scrollbar Styling
- **Scrollbar width**: thin
- **Scrollbar color**: var(--t4) transparent

#### WebKit Scrollbar (Chrome/Safari)
- **Width**: 4px
- **Thumb background**: var(--t4)
- **Thumb border radius**: 2px

---

## SPECIFIC SLOT ACCENT COLORS

Used via inline `style="--slot-accent:#COLORHERE;"`:

| Slot | Engine    | Accent Color |
|------|-----------|--------------|
| 1    | OBRIX     | #1E8B7E      |
| 2    | OPERA     | #D4AF37      |
| 3    | OXYTOCIN  | #9B5DE5      |

---

## CRITICAL NOTES FOR JUCE IMPLEMENTATION

1. **All dimensions in px** — JUCE will use scaled points; convert directly or apply DPI scaling.
2. **All colors are exact hex** — match precisely, no approximations.
3. **Font stack fallback**: Always include system-ui, sans-serif, monospace fallbacks.
4. **Border opacity**: `var(--border)` is 7% white, `var(--border-md)` is 11% white.
5. **Transitions**: Use specified easing curves and durations exactly.
6. **Slot accent**: Dynamically applied via CSS variable on `.slot-tile` parent; child elements inherit.
7. **Knob rendering**: SVG-based with custom stroke/fill colors.
8. **Scrollbars**: Thin, dark gray (#3A3938), 4px wide, subtle.
9. **Text shadows**: Engine name uses 16px glow shadow for depth.
10. **Box shadows**: Multi-layer shadows on plugin shell for depth perception.

