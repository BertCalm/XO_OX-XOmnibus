# PlaySurface V2 — XOuija Design Specification

**Document**: `2026-03-26-playsurface-v2-xouija-design.md`
**Status**: CANONICAL — Phase 1 implementation source of truth
**Date**: 2026-03-26
**UIX Studio Sign-off**: Unanimous (Ulf, Issea, Xavier, Lucy)
**Blessings ratified**: B041, B042, B043

---

## Table of Contents

1. [Overview and Goals](#1-overview-and-goals)
2. [Layout Architecture](#2-layout-architecture)
3. [XOuija Panel](#3-xouija-panel)
4. [The Planchette (B042)](#4-the-planchette-b042)
5. [Gesture Trail as Modulation Source (B043)](#5-gesture-trail-as-modulation-source-b043)
6. [Performance Gesture Buttons](#6-performance-gesture-buttons)
7. [GOODBYE](#7-goodbye)
8. [Note Input Zone](#8-note-input-zone)
9. [Performance Strip](#9-performance-strip)
10. [Visual Language](#10-visual-language)
11. [Data Model — CC / OSC Mapping](#11-data-model--cc--osc-mapping)
12. [Implementation Phases](#12-implementation-phases)
13. [What Is Eliminated from V1](#13-what-is-eliminated-from-v1)
14. [UIX Studio Architect Notes](#14-uix-studio-architect-notes)
15. [Blessings Reference](#15-blessings-reference)

---

## 1. Overview and Goals

PlaySurface V2 introduces the **XOuija** — a hybrid harmonic navigator and influence depth controller that replaces the V1 Orbit zone. The redesign consolidates the V1 performance pad column into the XOuija panel, reduces total component count by 5 (Lucy audit), and establishes the physical and symbolic vocabulary for the Phase 3 phone-as-controller workflow.

### Core Design Principles

- **XOuija is both input and display.** It visualizes the current harmonic relationship between the planchette position and the active key at all times, even when influence depth is zero.
- **Ouija aesthetics are tasteful, not horror.** Vintage spirit-board typography. Bioluminescent trails. A planchette that is an entity, not a cursor. GOODBYE, not PANIC.
- **Muscle memory transfers to Phase 3.** XOuija panel proportions (~9:19.5, matching an iPhone in portrait) mean gestures learned on desktop translate directly to phone-as-controller in Phase 3.
- **Progressive disclosure via influence depth (B041).** The Y-axis YES/NO axis is also an attentional hierarchy axis — users who don't want harmonic intelligence keep influence at 0 ("NO") and XOuija operates as a pure display. Users who want full coupling pull influence to 100% ("YES").
- **All parameters are CC/OSC-addressable from day one.** Every XOuija value ships with a MIDI CC assignment (CC 85–90) so Phase 3 networking is additive, not a rewrite.

---

## 2. Layout Architecture

### 2.1 Top-Level Structure

```
+------------------+------------------------------------------+
|                  |                                          |
|   XOuija Panel   |         Note Input Zone                  |
|   ~165px wide    |         (fills remaining width)          |
|   ~420px tall    |                                          |
|                  |   [PAD | DRUM | KEYS]  <- mode tabs      |
|   Circle of      |                                          |
|   Fifths         |   4x4 pad grid   OR                      |
|                  |   Seaboard keys  OR                      |
|   [Planchette]   |   Drum grid                              |
|                  |                                          |
|   [FREEZE][HOME] |                                          |
|   [DRIFT]        |                                          |
|   [  GOODBYE  ]  |                                          |
+------------------+------------------------------------------+
|         Performance Strip (full width, 64px)               |
|  [DubSpace|FilterSweep|Coupling|DubSiren]  XY surface       |
+------------------------------------------------------------+
```

### 2.2 Panel Dimensions and Proportions

| Panel | Width | Height | Notes |
|-------|-------|--------|-------|
| XOuija | ~165px | ~420px | Ratio ~9:19.5 (iPhone portrait). Exact px scales with plugin window. |
| Note Input | remaining | ~420px | Fills all horizontal space to the right of XOuija. |
| Performance Strip | 100% | 64px | Full width spanning both columns. |

- XOuija minimum width: 155px. Maximum: 185px. Beyond this range the circle-of-fifths markers lose legibility.
- Performance strip height: 64px. This is an increase from the V1 dense mockup (42px). The increase was recommended unanimously by the UIX Studio to provide adequate touch target depth for vertical gesture reading.
- No separator lines between XOuija and Note Input. The background tint contrast creates the visual division.

### 2.3 What is Not Present

The following V1 elements are **absent** from V2 layout:

- No standalone performance pad column (FREEZE/HOME/DRIFT are now inline in XOuija)
- No Orbit zone (XOuija is its functional and visual replacement)
- No PANIC button (replaced by GOODBYE — see Section 7)
- No FIRE/XOSEND/ECHO CUT as permanent fixed buttons (now configurable bank options — see Section 6)

---

## 3. XOuija Panel

### 3.1 Role and Axes

The XOuija panel is a 2D continuous input surface that simultaneously serves as:

1. **Harmonic field navigator** — X-axis maps to position on the circle of fifths
2. **Influence depth controller** — Y-axis maps to how strongly the harmonic field reshapes engine timbral response
3. **Visual feedback surface** — markers, planchette, and trail all update in real time even when touch is inactive

### 3.2 X-Axis — Circle of Fifths Navigation

**Mapping**: Linear, left-to-right across the XOuija surface.

```
LEFT                    CENTER                   RIGHT
Gb/F#  Db  Ab  Eb  Bb   F   C   G   D   A   E   B   F#/Gb
  |    |   |   |   |    |   |   |   |   |   |   |   |
 -6   -5  -4  -3  -2   -1   0  +1  +2  +3  +4  +5  +6
         (flat side)      (home)    (sharp side)
```

- C sits at exact horizontal center.
- Each step left or right is one fifth away from center.
- Enharmonic equivalents (Gb/F#) share the extreme edges and wrap logically but do not display twice.
- The X position determines which notes are considered "in key" for the pad grid, keyboard, and drum mode labeling.
- CC 85 output range: 0.0 (Gb, leftmost) to 1.0 (F#, rightmost). Center (C) = 0.5.

### 3.3 Y-Axis — Influence Depth

**Mapping**: Continuous float 0.0–1.0. Bottom = 0.0 ("NO"), Top = 1.0 ("YES").

```
TOP     YES  (influence = 1.0)
        |   full harmonic gravity — engine timbral response
        |   actively reshaped toward consonance/dissonance
        |   at current circle position
        |
CENTER  (influence = 0.5)
        |   partial gravity — subtle timbre shift
        |
BOTTOM  NO   (influence = 0.0)
             XOuija is passive / display only
             engine sounds exactly as programmed
```

- At influence = 0.0: pad/keyboard coloring still reflects harmonic relationships (display-only mode). No audio parameter changes.
- At influence = 1.0: engine's timbral parameters (filter cutoff, resonance routing, harmonic partials) are pulled toward the consonance/dissonance gradient implied by the circle position.
- The YES/NO labels are rendered in tiny serif italic (Georgia/Times, ~9px, opacity 0.20). They are present as a quiet reference, not a primary label.
- CC 86 output range: 0.0 (bottom/NO) to 1.0 (top/YES).

### 3.4 Circle of Fifths Marker Rendering

Each of the 12 notes (Gb through F#) has a marker rendered on the XOuija surface.

**Typography**: Georgia or Times New Roman, italic weight. This is the spirit-board serif reference — vintage, legible, not stylized horror.

**Vertical arc**: Markers do not sit on a flat horizontal line. Center notes (C, G, F) sit slightly lower; edge notes (Gb, Db, B, F#) sit slightly higher. The arc amplitude is subtle — approximately ±8px at the extremes. This hints at the circular topology of the circle of fifths without literally curving the layout.

**Size and opacity by distance from current home key**:

| Distance from home | Size | Opacity | Color |
|---|---|---|---|
| 0 (home key) | 100% (largest) | 100% | Teal #2A9D8F |
| 1–2 fifths away | 85% | 75% | Teal → XO Gold gradient |
| 3–4 fifths away | 70% | 50% | XO Gold #E9C46A |
| 5–6 fifths away | 55% | 35% | Warm Red #E07A5F |

- When the planchette is between two markers, both adjacent markers interpolate their size/opacity toward the home state proportionally.
- The "home key" for marker sizing purposes is always the nearest marker to the current planchette X position.

### 3.5 Tension Coloring System

Three-color gradient applied globally across pads, keyboard, and planchette itself:

| State | Color | Hex |
|---|---|---|
| Home / consonant (0–2 fifths from home) | Teal | `#2A9D8F` |
| Moderate tension (3–4 fifths from home) | XO Gold | `#E9C46A` |
| Maximum tension / dissonant (5–6 fifths) | Warm Red | `#E07A5F` |

- Interpolation is smooth — no abrupt jumps. Colors blend linearly between the three anchor points as planchette moves.
- This same color temperature is applied to: (a) active pad backgrounds, (b) keyboard key glow, (c) planchette border and pip, (d) bioluminescent trail color, (e) Performance Strip cursor glow.

### 3.6 Surface Texture

- XOuija background: very low opacity (~4–6%) noise texture overlay. Subtle grain evokes the feel of a real board surface without being literal.
- Recommended implementation: pre-generated noise image applied as CSS background-image or JUCE `drawImageAt` with alpha ~0.05.
- The noise texture is static (not animated).

---

## 4. The Planchette (B042)

**Blessing B042**: "The Planchette as Autonomous Entity" — ratified 7-1.

The planchette is not a cursor. It is a persistent entity on the board with its own momentum, memory, and agency.

### 4.1 Visual Specification

| Property | Value |
|---|---|
| Shape | Translucent oval lens |
| Size | ~68px wide × 46px tall |
| Border | Engine accent color, 1.5px solid |
| Background | Semi-transparent: rgba(engineAccentColor, 0.15) |
| Inner glow | box-shadow inset: engine accent color at 40% opacity, 12px blur |
| Center pip | 6px diameter circle, engine accent color, radial glow (box-shadow: 0 0 8px 3px engineAccentColor) |
| Interior text | Current key + influence% e.g. "G · 42%" |
| Text style | Georgia italic, ~10px, engine accent color at 85% opacity |

- Engine accent color is supplied at runtime by the active engine. Each of the 73 engines has its own accent color registered in the engine metadata.
- The planchette renders above all XOuija surface elements (z-order: surface texture < markers < trail < planchette).

### 4.2 Lissajous Idle Drift

When no touch/mouse input is active and the 400ms warm memory hold has expired, the planchette drifts autonomously in a Lissajous figure.

| Property | Value |
|---|---|
| X frequency | 0.3 Hz |
| Y frequency | 0.2 Hz |
| Amplitude | 15% of XOuija surface dimensions |
| Phase offset | π/4 (45 degrees) to prevent degenerate straight-line path |
| Center of drift | Last released planchette position (not panel center) |

- Drift is purely visual — it does not emit CC events and does not change the harmonic field or influence depth during idle drift. The last locked-on values persist.
- Animation runs on a requestAnimationFrame / JUCE Timer at 60fps.
- The drift center follows the last held position, meaning after releasing, the planchette drifts around the point you left it.

### 4.3 Spring Lock-On

When touch or mouse press begins:

| Property | Value |
|---|---|
| Animation duration | 150ms |
| Easing | cubic-bezier(0.25, 0.46, 0.45, 0.94) (ease-out) |
| Target | Exact touch/mouse position |

- The 150ms spring feel distinguishes intentional touch from accidental brush.
- During the 150ms spring, CC output begins immediately at the final target value — do not wait for animation to complete.

### 4.4 Warm Memory Hold

| Property | Value |
|---|---|
| Hold duration | 400ms after input release |
| Behavior during hold | Planchette stays exactly at last touch position. No drift. |
| Behavior after hold | Lissajous idle drift resumes, centered on hold position. |

- This prevents the planchette from immediately skittering away when the user briefly lifts their finger.
- CC output during hold: values remain at last-touched position. Values do not change until drift or new touch.

### 4.5 Bioluminescent Trail

| Property | Value |
|---|---|
| Buffer type | Ring buffer |
| Buffer length | 8–12 points (configurable, default 10) |
| Fade duration | 1.5 seconds from point creation |
| Fade curve | Exponential decay (not linear) |
| Color | Engine accent color |
| Point size | Starts at ~5px, shrinks to 1px as it fades |
| Glow | Each point has a radial glow at ~50% opacity of its current alpha |

- Trail only records during active touch/mouse drag, not during idle drift.
- Trail points are spaced by distance, not time — approximately every 4px of planchette movement.
- The trail is part of the B043 modulation source system (see Section 5).
- Visual metaphor: deep-sea bioluminescence. Organic, ethereal, not neon.

---

## 5. Gesture Trail as Modulation Source (B043)

**Blessing B043**: "Gesture Trail as First-Class Modulation Source" — ratified 6-2.

### 5.1 Data Structure

```cpp
struct TrailPoint {
    float x;           // normalized 0.0–1.0 within XOuija surface
    float y;           // normalized 0.0–1.0
    float velocity;    // derived from distance/time between consecutive points
    double timestamp;  // seconds since plugin load (monotonic clock)
};

// Ring buffer
TrailPoint trailBuffer[256];
int trailHead = 0;
int trailCount = 0;
```

- Buffer size: 256 tuples (power of two for masking arithmetic).
- Velocity is computed as Euclidean distance between current and previous point divided by time delta, clamped 0.0–1.0.
- Timestamp uses a monotonic clock (not wall clock) to survive DAW transport operations.

### 5.2 FREEZE — Trail Capture

When the FREEZE button is activated (keyboard: Z):

1. The current 256-tuple ring buffer is copied to a frozen snapshot buffer.
2. The frozen trail begins replaying as a looping modulation source.
3. Playback speed is 1:1 with original gesture timing (timestamps preserved).
4. The frozen trail's X and Y positions replay as CC output on CC 85 and CC 86 respectively.
5. The bioluminescent trail visual replays the frozen path at 50% opacity (to distinguish from live trail).
6. CC 88 is set to 127 (frozen = true).

To unfreeze: press FREEZE again (toggle). CC 88 returns to 0.

### 5.3 Two-Trail Interference

When two trails are simultaneously frozen (e.g., one pre-frozen and one newly frozen):

- Their X positions are summed and clamped: `x_out = clamp(x_A + x_B - 0.5, 0.0, 1.0)`.
- Their Y positions are multiplied: `y_out = y_A * y_B`.
- The resulting interference signal is coupleable via the mod matrix as a single modulation source labeled "XOuija Trail Mix".
- This is a Phase 1 feature — the visual representation is two overlapping trail paths at 50% opacity each.

### 5.4 Mod Matrix Exposure

The trail is exposed to the mod matrix as:

| Source name | Output signal | Notes |
|---|---|---|
| XOuija Circle | CC 85 (0.0–1.0) | Current or frozen planchette X |
| XOuija Influence | CC 86 (0.0–1.0) | Current or frozen planchette Y |
| XOuija Velocity | derived from trail | Instantaneous gesture speed |
| XOuija Trail Mix | interference result | Only present when two trails frozen |

---

## 6. Performance Gesture Buttons

### 6.1 Location

The gesture buttons are a horizontal button bar positioned at the **bottom of the XOuija panel**, between the circle-of-fifths surface and GOODBYE. They are **not** in a separate column — they are integrated into the XOuija panel.

```
[XOuija harmonic surface]

[  FREEZE  ] [  HOME  ] [  DRIFT  ]   <- button bar (default bank)

[          GOODBYE          ]
```

### 6.2 Default Button Assignments (XOuija bank)

| Button | Keyboard | Default Action |
|---|---|---|
| FREEZE | Z | Capture current trail as looping modulation source (toggle). See B043. |
| HOME | X | Snap planchette to circle center (C, influence 0.5). 150ms animation. |
| DRIFT | C | Toggle autonomous Lissajous drift on/off. When off, planchette holds position. |

### 6.3 Configurable Banks

The three button slots auto-follow the Performance Strip mode (see Section 9). When the strip mode changes, the button bank switches automatically. A Lock button (see Section 6.4) pins the bank.

| Strip Mode | Button Bank | Button 1 | Button 2 | Button 3 |
|---|---|---|---|---|
| (any — default) | XOuija | FREEZE (Z) | HOME (X) | DRIFT (C) |
| DubSpace | Dub | FIRE (chord trigger) | ECHO CUT | SEND BURST |
| FilterSweep | XOuija | FREEZE (Z) | HOME (X) | DRIFT (C) |
| Coupling | Coupling | COUPLING BURST | FREEZE ROUTES | DECOUPLE ALL |
| DubSiren | Performance | FIRE | FREEZE TRAIL | SUSTAIN ALL |

### 6.4 Lock Button

- A small lock icon (padlock, ~14px) is positioned at the top-right corner of the gesture button bar.
- When locked: the current bank is pinned regardless of strip mode changes.
- When unlocked (default): auto-follow behavior is active.
- Lock state is saved with project state.

### 6.5 Button Visual Specification

| Property | Value |
|---|---|
| Height | 28px |
| Width | Equal thirds of XOuija panel width minus gutters |
| Background (inactive) | rgba(255,255,255,0.06) |
| Background (active/pressed) | Engine accent color at 30% opacity |
| Border | 1px solid rgba(255,255,255,0.12) |
| Text | 9px, uppercase, letter-spacing 0.08em, opacity 0.65 (inactive) / 1.0 (active) |
| Corner radius | 3px |

GOODBYE is visually distinct and always fixed — see Section 7.

---

## 7. GOODBYE

GOODBYE replaces V1's PANIC button. It performs the same All Notes Off function but with a distinct visual and interaction design that matches the ouija aesthetic.

### 7.1 Location

Bottom of the XOuija panel, below the gesture button bar. Full panel width.

### 7.2 Visual Specification

| Property | Value |
|---|---|
| Color | Warm Terracotta `#E07A5F` (not danger-red) |
| Height | 32px |
| Width | Full XOuija panel width |
| Text | "GOODBYE" in Georgia italic, 11px, white at 90% opacity |
| Background | `#E07A5F` at 80% opacity normally, 100% opacity on press |
| Border | none |
| Corner radius | 4px |

GOODBYE is not styled as an alarm button. It is the peaceful, deliberate end of a session — the natural companion to the ouija metaphor.

### 7.3 Action Sequence

When GOODBYE is triggered (button press or keyboard shortcut V):

1. **All Notes Off**: sends All Notes Off on all 16 MIDI channels (MIDI CC 123 value 0, on channels 1–16).
2. **Planchette animation**: planchette slides to board center (X=0.5, Y=0.0 — the bottom-center "GOODBYE" position). Duration: 300ms. Easing: cubic-bezier(0.25, 0.46, 0.45, 0.94).
3. **Harmonic field reset**: circle position snaps to C (X=0.5), influence depth snaps to 0.0. CC 85 emits 0.5, CC 86 emits 0.0.
4. **Trail cleared**: bioluminescent trail buffer is cleared. Any frozen trail is unfrozen (CC 88 = 0).
5. **Visual**: during the 300ms animation, the planchette visually drifts downward toward the GOODBYE label position. The trail fades as it moves.

### 7.4 Keyboard Shortcut

| Key | Action |
|---|---|
| V | GOODBYE (All Notes Off + planchette reset) |

---

## 8. Note Input Zone

The Note Input Zone occupies the right panel (full height above the Performance Strip). Three modes are selectable via header tabs: PAD, DRUM, KEYS.

### 8.1 Mode Tab Bar

- Tab bar sits at the top of the Note Input Zone.
- Three tabs: PAD | DRUM | KEYS
- Active tab: engine accent color background, full opacity label.
- Inactive tabs: transparent background, 50% opacity label.
- Tab height: 28px.

---

### 8.2 PAD Mode

**PAD mode is the existing 4×4 MPC-style grid, enhanced with XOuija-reactive coloring.**

#### Grid Layout

| Property | Value |
|---|---|
| Grid | 4 columns × 4 rows |
| Pad aspect ratio | 1:1 enforced (square). `aspect-ratio: 1` in CSS; `bounds.proportionallyFit(square)` in JUCE. |
| Banks | A, B, C, D |
| Bank A base note | C2 (MIDI note 36, MPC standard) |
| Bank B base note | C3 (MIDI note 48) |
| Bank C base note | C4 (MIDI note 60) |
| Bank D base note | C5 (MIDI note 72) |

#### MPC Standard Note Mapping (Bank A, pad positions)

```
[A13 C4] [A14 C#4] [A15 D4] [A16 D#4]   <- row 4 (top)
[A09 G#3] [A10 A3] [A11 A#3] [A12 B3]   <- row 3
[A05 E3] [A06 F3]  [A07 F#3] [A08 G3]   <- row 2
[A01 C3] [A02 C#3] [A03 D3]  [A04 D#3]  <- row 1 (bottom)
```

(Standard MPC layout; Bank A starts at C2/note 36 in 0-indexed MIDI or C3/note 48 in 1-indexed — confirm with existing codebase convention.)

#### XOuija-Reactive Coloring

| Pad state | Visual treatment |
|---|---|
| In-key (note is diatonic to current harmonic field) | Glow with engine accent color, opacity 100% |
| Root note of current key | XO Gold `#E9C46A` bottom border (4px), accent glow |
| Out-of-key (chromatic, not in current scale) | Dimmed to ~20% opacity of normal state |
| Pressed (active note on) | Full brightness, slight scale-up (1.03×), immediate |

Coloring updates in real time as the planchette moves. The color temperature (teal → gold → red) reflects current harmonic tension.

#### Scale Modes

Two sub-modes for how out-of-key notes behave (toggled by a small icon button in the Note Input header, right side):

| Mode | Icon | Behavior |
|---|---|---|
| Filter | Funnel icon | Pads grid shows only diatonic scale notes. Out-of-key positions are dark/inert and do not trigger. The grid remaps to always show 8 diatonic notes across a 4×4 layout. |
| Highlight | Star/highlight icon | All 16 chromatic pads are active. Out-of-key pads are visually dimmed to 20%. Pressing an out-of-key pad: note is quantized to the nearest in-key note before output (quantize-to-nearest logic). |

Default scale mode: Highlight.

#### Velocity and Aftertouch

- **Velocity**: derived from Y-position within the pad at moment of press. Top of pad = maximum velocity (127). Bottom of pad = minimum velocity (1). Linear mapping.
- **Aftertouch**: when a pad is held and the finger/mouse is dragged further downward (continued Y-drag), this generates aftertouch CC output. Drag distance from press point maps 0–127.

---

### 8.3 DRUM Mode

DRUM mode is unchanged from V1 behavior.

| Property | Value |
|---|---|
| Grid | 4 columns × 4 rows |
| Banks | A, B, C, D |
| Bank A mapping | MPC standard drum map (Kick=A01, Snare=A02, etc.) |
| Bank shift | +16 notes per bank |
| XOuija influence | None — drums are chromatic by nature, unaffected by harmonic field |
| Coloring | V1 drum coloring retained. No XOuija tension colors applied. |

---

### 8.4 KEYS Mode (NEW)

KEYS mode is a continuous Seaboard-style keyboard surface.

#### Layout

| Property | Value |
|---|---|
| Visible range | 2 octaves at a time |
| Scrollable range | C1 to C7 |
| Natural key color | Warm white `#F0EDE8` |
| Sharp/flat key color | Charcoal `#2A2A2A` |
| Natural key proportions | Standard piano proportions (white keys taller, black keys shorter and narrower, overlapping) |
| Sharp key visual | Raised above natural key plane (z-order, not literal 3D) |

#### Octave Navigation

Three methods to scroll the visible octave range:

| Method | Action |
|---|---|
| Mouse wheel | Scroll left/right to move visible range by 1 semitone per tick; scroll left/right while holding Shift to move by 1 octave |
| Background drag | Click and drag the keyboard background (not a key) to scroll continuously |
| Octave ± buttons | Two buttons (labeled "OCT-" and "OCT+") in the Note Input header, far right. Step by 1 octave. Range: C1–C7. |

Current visible range displayed as a small label in the header (e.g., "C3–B4").

#### Expression — Platform-Adaptive

**Mouse / Trackpad (2D)**:

| Gesture | Action |
|---|---|
| Click (Y position at press) | Velocity — top of key = hard (127), bottom = soft (1). Same Y-velocity logic as PAD mode. |
| X-drag while holding | Pitch glide — continuous pitch bend as finger moves between keys. This is not a fixed semitone jump; it generates smooth pitch bend CC output proportional to X distance from press origin. |
| Release | Note off. Pitch bend returns to 0. |

**iOS (3D, Phase 2)**:

| Gesture | Action |
|---|---|
| Pressure | Velocity |
| X-slide | Pitch glide (pitch bend) |
| Y-slide (up/down on key) | CC74 (brightness / timbre) |

#### XOuija-Reactive Visual

| Key state | Visual treatment |
|---|---|
| In-key diatonic note | Glow with engine accent color applied to key surface |
| Root note of current key | XO Gold `#E9C46A` bottom border (5px on natural keys, 3px on sharp keys) |
| Out-of-key chromatic note | 20% opacity (strongly dimmed) |
| Bridge tone (in both home key and current key) | Dual-color indicator: left half home-key color, right half current-key color |
| Active (pressed) | Full brightness, slight elevation shadow |
| Glide trail | Engine accent color line extending from press origin, 200ms fade-out |

Color temperature of glide trail, key glows, and border colors shifts continuously with XOuija tension: teal (#2A9D8F) → XO Gold (#E9C46A) → Warm Red (#E07A5F).

#### External Controller Mirror

When an external MIDI keyboard, Seaboard, or MPC pad controller is connected and sends note-on events:

- The corresponding on-screen key lights up to show the played note.
- XOuija harmonic coloring is still applied — the light-up uses the tension-aware color, not a plain white.
- This is display only — the on-screen key does not generate duplicate note events.

---

## 9. Performance Strip

### 9.1 Dimensions and Position

| Property | Value |
|---|---|
| Position | Bottom of PlaySurface, full width |
| Height | 64px |
| Width | 100% of plugin window width |

### 9.2 Mode Tabs — Inside the Strip

The four mode tabs are located **inside the Performance Strip**, at the left edge of the strip. This was a UIX Studio recommendation — tabs contextualized within the strip communicate that they control the strip, not the Note Input Zone.

```
[DubSpace] [FilterSweep] [Coupling] [DubSiren]    . . . [XY gestural surface] . . .
```

| Property | Value |
|---|---|
| Tab width | ~72px each |
| Tab height | 22px |
| Tab position | Vertically centered within strip, left-aligned |
| Active tab | Engine accent color background, full opacity |
| Inactive tab | Transparent, 45% opacity text |
| Tab font | 8px uppercase, letter-spacing 0.1em |

### 9.3 XY Gestural Controller Surface

The area to the right of the mode tabs is the gestural surface.

#### Crosshair Gridlines

Two lines rendered at low opacity communicate "this is a 2D drag surface":

| Line | Position | Opacity |
|---|---|---|
| Horizontal center line | Vertical midpoint of strip | 0.10 |
| Vertical center line | Horizontal midpoint of gestural area | 0.10 |

Lines are 1px wide, white.

#### Cursor

| Property | Value |
|---|---|
| Size | 10px diameter dot |
| Fill | Radial gradient: center = engine accent color (100%), edge = engine accent color (0%) |
| Glow halo | box-shadow / JUCE blur: engine accent color, 8px radius, 60% opacity |
| Vertical bar | 1px wide line extending from cursor down to strip floor |
| Floor glow | At the bottom of the vertical bar: small horizontal glow spread, engine accent color, 30% opacity |

The vertical bar + floor glow gives the cursor a "standing on a surface" quality — it casts presence downward.

#### Gesture Trail

Same visual system as XOuija trail (B043):

| Property | Value |
|---|---|
| Color | Engine accent color |
| Fade duration | 1.5 seconds |
| Point spacing | Every ~4px of cursor movement |
| Point size | Starts ~4px, shrinks to 1px as it fades |

#### Spring-Back Behavior

When touch/mouse is released, the cursor springs back to the mode-specific home position.

| Mode | Home position |
|---|---|
| DubSpace | Center (0.5, 0.5) |
| FilterSweep | Bottom-left (0.0, 0.0) — filter closed, no resonance |
| Coupling | Center (0.5, 0.5) |
| DubSiren | Bottom-center (0.5, 0.0) |

Spring-back: 250ms, cubic-bezier(0.25, 0.46, 0.45, 0.94).

#### Axis Labels

Edge labels communicate what each axis controls. Labels are 8px, 35% opacity, uppercase.

| Mode | Bottom-center label | Right-center label | Top-center label | Left-center label |
|---|---|---|---|---|
| DubSpace | DELAY FB | REVERB | DELAY TIME | DRY |
| FilterSweep | CUTOFF | RESONANCE | — | — |
| Coupling | SPREAD | DEPTH | — | — |
| DubSiren | PITCH | SIREN DEPTH | — | — |

### 9.4 Mode-Specific Background Tint

Each mode applies a distinct low-opacity tint to the strip background:

```cpp
const Colour kStripModeTints[] = {
    Colour(42,  157, 143).withAlpha(0.12f),  // DubSpace    — teal
    Colour(233, 196, 106).withAlpha(0.10f),  // FilterSweep — gold
    Colour(244, 162, 97 ).withAlpha(0.10f),  // Coupling    — amber
    Colour(224, 122, 95 ).withAlpha(0.12f),  // DubSiren    — terracotta
};
```

### 9.5 Auto-Follow Integration

When the active strip mode changes, the XOuija gesture button bank auto-follows (see Section 6.3), unless the bank lock is engaged.

---

## 10. Visual Language

### 10.1 Color Palette

| Name | Hex | Usage |
|---|---|---|
| Teal (consonant) | `#2A9D8F` | Home key markers, in-key pads/keys, consonant cursor |
| XO Gold (moderate) | `#E9C46A` | Mid-tension state, root note indicator, moderate markers |
| Warm Red (dissonant) | `#E07A5F` | Far-tension state, GOODBYE, dissonant markers |
| Warm White (keys) | `#F0EDE8` | Natural piano keys |
| Charcoal (black keys) | `#2A2A2A` | Sharp/flat piano keys |
| Engine accent | (per engine) | Planchette border/pip, trail, cursor, active pads, tab highlighting |

### 10.2 Typography

| Element | Font | Style | Size | Opacity |
|---|---|---|---|---|
| Circle of fifths markers | Georgia / Times New Roman | Italic | Varies by distance (see 3.4) | Varies by distance |
| YES / NO labels | Georgia / Times New Roman | Italic | 9px | 0.20 |
| GOODBYE label | Georgia / Times New Roman | Italic | 11px | 0.90 |
| Planchette text | Georgia / Times New Roman | Italic | 10px | 0.85 |
| Button labels | System sans-serif | Uppercase, letter-spacing 0.08em | 9px | 0.65 (inactive) / 1.0 (active) |
| Axis labels | System sans-serif | Uppercase, letter-spacing 0.10em | 8px | 0.35 |
| Tab labels | System sans-serif | Uppercase, letter-spacing 0.10em | 8px | 0.45 (inactive) / 1.0 (active) |

### 10.3 Ouija Aesthetic Boundaries

The following defines what "tasteful ouija" means for this project:

**Yes (included)**:
- Serif italic typography on the XOuija surface
- Planchette as translucent oval entity
- YES/NO axis labels in tiny low-opacity serif
- Warm terracotta for GOODBYE (peaceful, not alarming)
- Bioluminescent trail (organic phosphorescence glow)
- Subtle noise texture on XOuija background

**No (excluded)**:
- Horror fonts (dripping, cracked, bone-shaped)
- Red color for GOODBYE or any danger-coded element
- Literal board imagery (sun/moon/stars motifs from traditional ouija)
- Flame or skull iconography
- Aggressive animation (shaking, flashing)

### 10.4 Opacity Hierarchy (B041 — Dark Cockpit)

**Blessing B041**: "Dark Cockpit Attentional Design" — five-level opacity hierarchy.

| Level | Opacity | Element examples |
|---|---|---|
| 1 — Active / focused | 100% | Pressed pad, planchette (during touch), active tab |
| 2 — Primary | 75–85% | In-key markers at home, active buttons, planchette text |
| 3 — Secondary | 45–65% | Inactive tabs, nearby markers, button labels (inactive) |
| 4 — Tertiary | 20–35% | Out-of-key pads/keys, axis labels, far markers |
| 5 — Ghost | 10–15% | YES/NO labels, crosshair gridlines, surface texture |

The influence depth Y-axis is itself a B041 attentional depth axis: at influence=0.0 (NO), the XOuija operates at opacity level 4. At influence=1.0 (YES), XOuija elements escalate toward level 1.

---

## 11. Data Model — CC / OSC Mapping

All XOuija parameters are CC-mapped from Phase 1 and OSC-addressable in Phase 3. Phase 1 implementers must emit CC output for all CC-mapped parameters — the OSC channel is Phase 3 additive.

### 11.1 CC Map (Phase 1 — Required)

| Parameter | MIDI CC | Range | Direction |
|---|---|---|---|
| Circle position (X) | CC 85 | float 0.0–1.0 (7-bit: 0–127) | Out |
| Influence depth (Y) | CC 86 | float 0.0–1.0 (7-bit: 0–127) | Out + In |
| Pressure (iOS Phase 2) | CC 87 | float 0.0–1.0 (7-bit: 0–127) | Out (Phase 2) |
| Trail freeze state | CC 88 | bool: 0 = unfrozen, 127 = frozen | Out |
| Home snap trigger | CC 89 | trigger: 127 = snap (no hold state) | Out + In |
| Drift toggle | CC 90 | bool: 0 = drift off, 127 = drift on | Out + In |

CC input (In): receiving these CCs on the plugin's MIDI input channel moves the planchette/influence remotely. This enables Phase 3 phone control over CC before OSC is implemented.

### 11.2 OSC Map (Phase 3 — Reference)

| Parameter | OSC Address | Data type | Range |
|---|---|---|---|
| Circle position | `/xouija/circle` | float | 0.0–1.0 |
| Influence depth | `/xouija/influence` | float | 0.0–1.0 |
| Pressure (iOS) | `/xouija/pressure` | float | 0.0–1.0 |
| Gyro X (iOS) | `/xouija/gyro/x` | float | -1.0 – +1.0 |
| Gyro Y (iOS) | `/xouija/gyro/y` | float | -1.0 – +1.0 |
| Trail freeze | `/xouija/trail/freeze` | bool | 0 / 1 |
| Home snap | `/xouija/home` | trigger (no args) | — |
| Drift toggle | `/xouija/drift` | bool | 0 / 1 |

OSC uses full float precision — this is why gyroscope data (which exceeds MIDI 7-bit fidelity) is OSC-only. CC 87 provides a degraded fallback for gyro-derived pressure.

### 11.3 MIDI Channel Assignment

- XOuija CC output: MIDI channel 1 by default.
- Configurable in the plugin's MIDI settings page (out of scope for PlaySurface spec).
- All Notes Off (GOODBYE): sent on channels 1–16, no channel filter.

---

## 12. Implementation Phases

### 12.1 Phase 1 — Desktop (This Spec)

Everything in Sections 2–11 of this document is Phase 1 scope. Summary checklist:

**XOuija Zone**
- [ ] XOuija panel layout: ~165px wide, ~420px tall, portrait proportions
- [ ] Circle of fifths X-axis with 12 markers (Gb through F#)
- [ ] Marker typography: Georgia/Times italic, size/opacity by harmonic distance
- [ ] Marker arc: ±8px vertical offset at extremes
- [ ] Y-axis influence depth, YES/NO labels (9px Georgia italic, opacity 0.20)
- [ ] Tension coloring: teal → XO Gold → warm red gradient
- [ ] Surface noise texture (opacity 0.04–0.06)

**Planchette (B042)**
- [ ] Translucent oval lens, 68×46px, engine accent border
- [ ] Center pip 6px with glow
- [ ] Text: current key + influence% in Georgia italic 10px
- [ ] Lissajous idle drift: 0.3Hz × 0.2Hz, 15% amplitude, π/4 phase offset
- [ ] Spring lock-on: 150ms cubic-bezier ease-out
- [ ] Warm memory hold: 400ms after release
- [ ] Bioluminescent trail: 8–12 point ring buffer, 1.5s exponential fade

**Gesture Trail (B043)**
- [ ] 256-tuple ring buffer (x, y, velocity, timestamp)
- [ ] FREEZE: copy buffer to frozen snapshot, loop as CC 85/86 output
- [ ] Two-trail interference: sum X (clamped), multiply Y
- [ ] Mod matrix exposure: XOuija Circle, XOuija Influence, XOuija Velocity, XOuija Trail Mix

**Gesture Buttons**
- [ ] Button bar: FREEZE (Z) / HOME (X) / DRIFT (C) as default bank
- [ ] 4 configurable banks: XOuija, Dub, Coupling, Performance
- [ ] Auto-follow: switches bank when strip mode changes
- [ ] Lock button: pins bank regardless of strip mode
- [ ] Button visual spec (28px height, equal thirds, engine accent on active)

**GOODBYE**
- [ ] Terracotta #E07A5F, 32px, full XOuija width, Georgia italic "GOODBYE"
- [ ] Action: All Notes Off ch.1–16 + planchette to center-bottom 300ms + harmonic reset + trail clear
- [ ] Keyboard shortcut: V

**Note Input Zone**
- [ ] PAD mode: 4×4 square grid, aspect-ratio:1 enforced, 4 banks (A/B/C/D)
- [ ] PAD XOuija coloring: in-key glow, root gold border, out-of-key 20% opacity
- [ ] PAD scale modes: Filter and Highlight (quantize-to-nearest for Highlight)
- [ ] PAD velocity from Y-position, aftertouch from continued Y-drag
- [ ] DRUM mode: V1 behavior retained, unaffected by XOuija
- [ ] KEYS mode: 2-octave keyboard, C1–C7 scrollable
- [ ] KEYS scroll methods: wheel, background drag, OCT±buttons
- [ ] KEYS expression: Y-velocity at press, X-drag pitch glide
- [ ] KEYS XOuija reactive: in-key glow, root gold border, out-of-key 20% dim, bridge dual-color, glide trail 200ms

**Performance Strip**
- [ ] 64px height, full width
- [ ] Mode tabs inside strip, left-aligned, 22px height
- [ ] Crosshair gridlines (horizontal + vertical, opacity 0.10)
- [ ] Cursor: 10px dot, radial gradient, glow halo, vertical bar, floor glow
- [ ] Gesture trail: engine accent, 1.5s fade
- [ ] Spring-back to mode home position (250ms ease-out)
- [ ] Axis labels at edges (8px uppercase, 35% opacity)
- [ ] Mode-specific background tints (kStripModeTints)
- [ ] Auto-follow integration with gesture button banks

**CC Output**
- [ ] CC 85: circle X on every planchette move
- [ ] CC 86: influence Y on every planchette move
- [ ] CC 88: trail freeze state changes
- [ ] CC 89: home snap trigger
- [ ] CC 90: drift toggle state changes
- [ ] CC input on CC 86, 89, 90 moves planchette/influence remotely

### 12.2 Phase 2 — iOS (Future)

- XOuija as standalone iOS surface (AUv3 + standalone app)
- Multi-touch support (simultaneous planchette + gesture buttons)
- Pressure (3D Touch / Capacitive) → CC 87
- Gyroscope input → OSC /xouija/gyro/x, /xouija/gyro/y
- Tilt-to-drift planchette (gyro Y controls drift direction)
- 3D Seaboard expression: pressure (velocity) + X-glide (pitch bend) + Y-slide (CC74 brightness)
- Responsive layout adapts XOuija panel to portrait phone proportions

### 12.3 Phase 3 — Network Bridge (Future)

- Network MIDI (rtpMIDI) for CC-level communication
- BLE MIDI as fallback for Bluetooth proximity use
- OSC channel for high-resolution gyro/pressure data (float, exceeds 7-bit MIDI fidelity)
- Auto-discovery via Bonjour/mDNS — phone appears as "XOceanus Controller" on local network
- Desktop mirror: remote planchette position displayed as a second ghost planchette on desktop XOuija surface
- Graceful degradation: if OSC unavailable → fall back to MIDI-only CC 85–90
- Two players: two phones can each control a planchette; their trails can freeze and create interference (B043 two-trail mode over network)

---

## 13. What Is Eliminated from V1

| V1 Element | Status | Reason / Replacement |
|---|---|---|
| Orbit zone | ELIMINATED | XOuija is a strictly better XY controller with harmonic intelligence. All orbit functionality (XY position, influence) is absorbed. |
| Performance pad column | ELIMINATED | FREEZE / HOME / DRIFT absorbed into XOuija panel as the configurable gesture button bar. Reduces component count by 3. |
| PANIC button | REPLACED | GOODBYE performs identical All Notes Off function with better nomenclature, terracotta color, and a planchette animation that makes session ending intentional and graceful. |
| FIRE as permanent button | DEMOTED | FIRE is now in the Dub bank and Performance bank (configurable). It is not a fixed permanent button. Available when contextually relevant. |
| XOSEND as permanent button | DEMOTED | Now a configurable bank option. Not a fixed permanent button. |
| ECHO CUT as permanent button | DEMOTED | Now in Dub bank. Not a fixed permanent button. |

---

## 14. UIX Studio Architect Notes

Design reviewed by all four UIX Studio architects. Recorded for implementation awareness:

### Ulf
> "Layout breathes despite density. The XOuija panel's portrait proportions create a natural breathing room that the earlier column-heavy layout never had. Progressive disclosure through the XOuija influence axis is elegant — users who don't need harmonic intelligence simply ignore the Y-axis and it stays at NO. They never see a feature they didn't ask for."

### Issea
> "Ma in XOuija is intentional. The planchette needs space to drift — the relative emptiness of the harmonic surface between markers is not wasted space, it is the drift territory. The Performance Strip's negative space now has purpose through the crosshair gridlines — the empty area communicates '2D gestural surface' rather than 'unfinished.'"

### Xavier
> "Strip reads as interactive surface. The 64px height increase (from 42px) provides adequate touch targets for vertical gesture reading. Mode tabs contextualized inside the strip is correct — at the previous position in the header, users could not tell if tabs controlled the note zone or the strip. Now context is unambiguous."

### Lucy
> "Component count reduced by 5 versus the previous layout design. Gesture buttons painted inline, no wrapper panel required. Repaint cost is acceptable — the XOuija gradient fields should be cached in `resized()` using off-screen ImageBuffers, not recalculated per-frame. GOODBYE's terracotta is correct; do not let implementation drift it toward red."

---

## 15. Blessings Reference

| Blessing ID | Title | Ratification | Sections Referencing |
|---|---|---|---|
| B041 | Dark Cockpit Attentional Design — Five-Level Opacity Hierarchy | Accepted | 3.3 (YES/NO axis), 10.4 (opacity table) |
| B042 | The Planchette as Autonomous Entity | 7-1 | Section 4 (entire) |
| B043 | Gesture Trail as First-Class Modulation Source | 6-2 | Section 5 (entire), 4.5 (visual trail) |

### B041 Summary
Five levels of visual attention priority for dark-background plugin UI. Level 1 (100% opacity) = active/focused elements. Level 5 (~12% opacity) = ambient/ghost elements. Applied to XOuija to create progressive disclosure: high-influence state escalates element prominence; low-influence state demotes them to background.

### B042 Summary
The planchette is not a cursor — it is an entity. It has autonomous behavior (Lissajous drift), memory (warm hold), momentum (spring lock-on), and presence (bioluminescent trail). These properties are not decorative — they encode state (drift = no recent input, trail = recent path, warm hold = just released). Remove any of these properties and the planchette becomes a cursor.

### B043 Summary
The gesture trail is promoted from visual decoration to a first-class DSP/modulation signal. The 256-tuple ring buffer is the canonical data structure. FREEZE captures it. The mod matrix consumes it. Two frozen trails create interference. This enables performance gestures to have a lasting effect on the sound beyond the moment of input.

---

*End of PlaySurface V2 — XOuija Design Specification*

*Document version: 1.0 | 2026-03-26 | Canonical Phase 1 source*
