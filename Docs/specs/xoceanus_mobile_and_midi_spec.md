# XOceanus — Mobile Layout, Touch Interaction & MIDI Controller Specification

**Version:** 1.0
**Author:** XO_OX Designs
**Date:** 2026-03-08
**Scope:** iOS/iPadOS layout, touch gesture model, sensor integration, MIDI controller support
**Targets:** iPhone (AUv3 + Standalone), iPad (AUv3 + Standalone)
**Framework:** JUCE 8, shared codebase with macOS, conditional compilation via `XO_MOBILE=1`
**Status:** Specification Complete — Ready for Implementation

**Related Documents:**
- `xoceanus_technical_design_system.md` — authoritative design system (Gallery Model, color, typography, components)
- `xo_signature_playsurface_spec.md` — PlaySurface zones, modes, and interaction model (desktop reference)

---

## 1. Design Philosophy for Mobile

### 1.1 The Instrument in Your Hands

Mobile is not a scaled-down desktop. It is a different relationship with the instrument. A laptop sits on a desk — the producer reaches toward it. A phone or tablet rests in the hands — the instrument wraps around the producer. This changes everything: the hierarchy of controls, the size of targets, the role of gesture, and the meaning of space on screen.

XOceanus mobile treats the phone and tablet as first-class instruments, not compromised views of a desktop application.

### 1.2 Touch is Better Than Mouse

For the PlaySurface, touch is the superior input modality:

- **Multi-touch** — play chords, modulate two parameters independently, perform with all ten fingers
- **Pressure/force** — natural velocity and aftertouch without MIDI hardware
- **Direct manipulation** — no cursor indirection; the finger IS the controller
- **Gestural expression** — swipe, pinch, rotate map to musical actions that a mouse cannot express

The desktop PlaySurface was designed with touch in mind. Mobile is where it comes home.

### 1.3 Gallery Model on Mobile

The Gallery Model adapts across form factors. The warm white walls (`#F8F6F3`) stay consistent. The engine accent colors stay consistent. The proportions of the rooms change:

| Form Factor | Gallery Metaphor |
|-------------|-----------------|
| Desktop (macOS) | Full museum — all exhibitions visible, wide corridors |
| iPad | Gallery wing — most zones visible, tight but complete |
| iPhone Portrait | Single room — one exhibition at a time, swipe between rooms |
| iPhone Landscape | Performance hall — maximum stage, minimal backstage |

### 1.4 Intuitive Mode as Default

On mobile, Intuitive mode is the default experience. The 4 macros — **CHARACTER**, **MOVEMENT**, **COUPLING**, **SPACE** — are the primary controls, always accessible regardless of which zone is active. Advanced mode is available but requires deliberate opt-in. This matches the mobile mental model: start simple, go deep when ready.

### 1.5 Touch Target Minimum

All interactive elements meet the Apple Human Interface Guidelines minimum of **44pt**. This applies to:
- Pad cells in Note Input
- Performance Pads (FIRE, XOSEND, ECHO CUT, PANIC)
- Knob hit areas (even 24pt visual knobs have 44pt touch targets)
- Buttons, tabs, toggles
- Drawer handles and swipe targets

---

## 2. iPhone Layout — Portrait

Full-screen zones, swipe between them in a horizontal carousel. The header and performance pads remain fixed; the central zone scrolls.

```
┌─────────────────────────────────┐
│  [XO] [Preset ◄ ►]  [M1-M4]   │  <- Compact header (44pt)
├─────────────────────────────────┤
│                                 │
│         ACTIVE ZONE             │
│         (full remaining height) │
│                                 │
│         Swipe L/R carousel:     │
│         ┌───┐ ┌───┐ ┌───┐      │
│         │PAD│ │FRT│ │DRM│      │
│         └───┘ └───┘ └───┘      │
│         ┌───┐ ┌───┐            │
│         │ENG│ │PRE│            │
│         └───┘ └───┘            │
│                                 │
│         ● ● ● ○ ○  (page dots) │
├─────────────────────────────────┤
│  [FIRE][SEND][ CUT][PANIC]     │  <- Perf Pads always visible (56pt)
├─────────────────────────────────┤
│  ═══ Performance Strip ═══     │  <- Peek handle (44pt), swipe up
│       (peek / expand)           │     to expand to full 160pt
└─────────────────────────────────┘
```

### 2.1 Header Bar (44pt fixed)

| Element | Size | Behavior |
|---------|------|----------|
| XO_OX logo | 28x28pt | Tap: app menu (settings, MIDI config, about) |
| Preset name | Flexible width | Tap: open preset browser page. Left/right arrows: prev/next preset |
| Macro circles (M1-M4) | 24pt visual, 44pt touch | Tap: popup full-size 40pt knob with value readout. Swipe down from header: expand to macro strip |

The macro circles show the current value as a filled arc in XO Gold (`#E9C46A`). Each circle is labeled below with a 9px micro label: **CHR**, **MOV**, **CPL**, **SPC**.

Swipe down from header reveals a **Macro Strip** — a horizontal bar (80pt height) with four 40pt macro knobs at full size, labeled in UPPERCASE. Swipe up or tap outside to dismiss.

### 2.2 Active Zone Carousel

Five pages, swiped horizontally with momentum scrolling and snap-to-page:

| Page | Content | Notes |
|------|---------|-------|
| 1. PAD | 4x4 note grid, scale-locked | Default landing page |
| 2. FRETLESS | Full-width ribbon, pitch on Y, mod on X | Multi-touch, per-finger tracking |
| 3. DRUM | Labeled kit pads, 4x4 | Engine-specific voice labels |
| 4. ENGINE | Parameter knobs for active engine(s) | Scrollable, grouped by section |
| 5. PRESET | Preset browser with mood tabs, DNA map | Full preset browsing experience |

Page indicator dots sit at the bottom of the active zone, above the Perf Pads bar.

### 2.3 Orbit Path Access

The Orbit Path is not a carousel page. It is accessed via **2-finger pinch** on any active zone page. Pinch in: Orbit Path overlays the current zone at 85% opacity with a 200pt circular Orbit control centered on screen. Pinch out or tap outside: dismiss overlay. This keeps Orbit available everywhere without consuming a carousel slot.

### 2.4 Performance Pads Bar (56pt fixed)

Four pads spanning the full width, equal size. Each pad is 44pt tall with 6pt vertical padding:

```
┌──────────┬──────────┬──────────┬──────────┐
│   FIRE   │  XOSEND  │ ECHO CUT │  PANIC   │
│  (red)   │  (gold)  │  (teal)  │  (grey)  │
└──────────┴──────────┴──────────┴──────────┘
```

- FIRE: momentary, glows red while held
- XOSEND: toggle, stays lit when active
- ECHO CUT: momentary, teal flash
- PANIC: tap to kill all notes/effects, grey with red flash on activation

### 2.5 Performance Strip (Peek + Expand)

The Performance Strip lives at the bottom edge in a collapsed **peek** state showing only a drag handle (44pt). Swipe up to expand to full height (160pt). In expanded state, the strip provides full-width XY control matching the desktop Performance Strip spec (4 modes: DUB SEND, FILTER SWEEP, PITCH WARP, STUTTER GATE).

| State | Height | Content |
|-------|--------|---------|
| Peek | 44pt | Handle bar + mode label + current XY readout |
| Expanded | 160pt | Full XY touch surface + mode selector tabs |

---

## 3. iPhone Layout — Landscape (Performance Mode)

Landscape orientation triggers **Performance Mode** — a layout optimized for maximum playable area with all controls accessible without page-switching.

```
┌──────────────────────────────────────────────────────────────┐
│ [XO][Preset]                                                 │
│                                                              │
│  [M1]         NOTE INPUT (PAD / FRETLESS / DRUM)      [FIRE]│
│  [M2]                                                 [SEND]│
│  [M3]         Takes ~80% width                        [CUT ]│
│  [M4]         Full playable area                      [PNIC]│
│               Multi-touch optimized                          │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│  ══════════ Performance Strip (full width, 64pt) ══════════  │
└──────────────────────────────────────────────────────────────┘
```

### 3.1 Layout Regions

| Region | Position | Size | Content |
|--------|----------|------|---------|
| Header | Top-left | 44pt height, ~120pt width | XO logo + preset name (compact) |
| Macro Strip | Left edge | 44pt wide, full height | M1-M4 stacked vertically, 44pt each |
| Note Input | Center | ~80% of width, remaining height | Active PlaySurface mode (PAD/FRETLESS/DRUM) |
| Perf Pads | Right edge | 56pt wide, full height | FIRE/SEND/CUT/PANIC stacked vertically |
| Perf Strip | Bottom | Full width, 64pt | XY performance surface |

### 3.2 Behavior

- Macro knobs appear as filled arc circles on the left edge. Tap to adjust with popup slider.
- Note Input mode is switchable via 3-finger swipe left/right (PAD -> FRETLESS -> DRUM cycle).
- Orbit Path: 2-finger pinch on Note Input area, same as portrait.
- Engine parameters and preset browser are not directly accessible in landscape — rotate to portrait for those tasks. This is a deliberate design choice: landscape = performance, portrait = configuration.
- Performance Strip has only 64pt height (vs 160pt expanded in portrait) but the full-width format provides excellent horizontal resolution for XY control.

---

## 4. iPad Layout

Near-desktop experience. All PlaySurface zones are visible simultaneously without page switching. The iPad's screen real estate allows the Gallery Model to present multiple exhibitions at once.

```
┌──────────────────────────────────────────────────────────────────┐
│  [XO_OX] [Preset: "Amber Tide" ◄ ►]   [M1][M2][M3][M4]   [=]  │
├──────────────────────────────────┬────────────┬─────────────────┤
│                                  │            │                 │
│       NOTE INPUT                 │   ORBIT    │   PERF PADS     │
│       (PAD / FRETLESS / DRUM)    │   PATH     │                 │
│                                  │            │   [  FIRE  ]    │
│       PAD: 4x4 grid             │   ~200pt   │   [ XOSEND ]    │
│       ~120pt per pad cell        │   circle   │   [ECH CUT ]    │
│                                  │            │   [ PANIC  ]    │
│       FRETLESS: full ribbon      │   Free /   │                 │
│                                  │   Snap     │   Each pad      │
│       DRUM: labeled kit grid     │   modes    │   ~80x80pt      │
│                                  │            │                 │
├──────────────────────────────────┴────────────┴─────────────────┤
│  ══════════════ Performance Strip (full width, 80pt) ══════════  │
├──────────────────────────────────────────────────────────────────┤
│  [Mood tabs]  [Preset grid / DNA map / Engine params drawer]     │
└──────────────────────────────────────────────────────────────────┘
```

### 4.1 Layout Regions

| Region | Position | Approximate Size | Content |
|--------|----------|-----------------|---------|
| Header | Top, full width | 48pt height | Logo, preset nav, macro knobs (40pt), settings |
| Note Input | Left, ~55% width | Remaining height above strip | Active input mode |
| Orbit Path | Center-right, ~20% width | 200pt circle area | XY expression control |
| Perf Pads | Far right, ~15% width | 4 pads stacked | FIRE, XOSEND, ECHO CUT, PANIC |
| Perf Strip | Below zones, full width | 80pt height | XY performance surface |
| Bottom Section | Below strip, full width | ~200pt height | Swipeable content area |

### 4.2 Bottom Section — Swipeable Content

The bottom section swipes horizontally between three views:

| View | Content |
|------|---------|
| Preset Browser | Mood tabs + preset card grid, DNA sparklines visible |
| Engine Parameters | Knob panels for active engine(s) — 2 panels side by side when 2 engines active |
| Sequencer | Step sequencer grid (when available) |

In Intuitive mode, the bottom section defaults to the Preset Browser. In Advanced mode, it defaults to Engine Parameters.

### 4.3 iPad Split View / Slide Over

XOceanus supports iPadOS multitasking:

| Mode | Behavior |
|------|----------|
| Full screen | Standard layout as above |
| Split View (50%) | Bottom section collapses; Orbit Path moves to overlay (pinch access) |
| Split View (33%) | Equivalent to iPhone portrait layout |
| Slide Over | iPhone portrait layout in narrow floating window |

### 4.4 iPad Landscape vs Portrait

| Orientation | Layout Change |
|-------------|--------------|
| Landscape | Standard layout (above diagram) |
| Portrait | Note Input stacks above Orbit + Perf Pads; bottom section gets more vertical space for preset browsing |

---

## 5. Touch Interaction Model

Every gesture maps to a specific musical action. These tables define the complete touch vocabulary across all PlaySurface modes.

### 5.1 Pad Mode

| Gesture | Action | Details |
|---------|--------|---------|
| Tap | Note on | Velocity derived from touch size/force (0-127) |
| Release | Note off | Release velocity from lift speed |
| Hold | Sustained note | Aftertouch from continued pressure changes |
| Swipe within pad (Y-axis) | Send amount | Vertical gesture controls send level to FX bus |
| Swipe within pad (X-axis) | Engine blend | In multi-engine mode, crossfades between coupled engines |
| Multi-touch (2+ pads) | Chord | All touched pads sound simultaneously with independent velocity |
| Long press (500ms) | Pad options popup | Scale assignment, mode switch, MIDI note assignment |

### 5.2 Fretless Mode

| Gesture | Action | Details |
|---------|--------|---------|
| Touch anywhere | Note on | Y-position determines pitch; velocity from touch force |
| Slide vertically | Pitch glide | Portamento — continuous pitch change following finger |
| Slide horizontally | X-axis modulation | Default target: filter cutoff. Configurable per-engine |
| Multi-touch | Multi-note | Each finger tracked independently with its own pitch and mod |
| Pinch (2 fingers) | Vibrato depth | Distance between fingers controls vibrato LFO depth |

### 5.3 Drum Mode

| Gesture | Action | Details |
|---------|--------|---------|
| Tap | Trigger drum voice | Velocity from force; each pad mapped to a kit piece |
| Swipe within pad (X-axis) | Engine blend | Circuit <-> Algorithm crossfade (XOnset-specific) |
| Swipe within pad (Y-axis) | Decay time | Vertical position scales voice decay envelope |
| Multi-tap rapid | Roll / flam | Repeated taps within 100ms window trigger roll behavior |

### 5.4 Orbit Path

| Gesture | Action | Details |
|---------|--------|---------|
| Touch and drag | Move cursor | Sends XY position to mapped parameters continuously |
| Release (Free mode) | Momentum continue | Cursor continues moving with velocity, decays with friction coefficient |
| Release (Snap mode) | Spring-back | Cursor animates back to center with spring physics |
| 2-finger rotate | Alternative mapping | Rotation angle maps to configurable target (e.g., LFO rate, reverb size) |

### 5.5 Performance Strip

| Gesture | Action | Details |
|---------|--------|---------|
| Touch and drag | XY modulation | X and Y axes modulate targets based on active strip mode |
| Release (Spring mode) | Snap to center | Both axes spring back to zero with 200ms ease-out |
| Release (Latch mode) | Stay at position | Values hold at release point until next touch |

### 5.6 Performance Pads

| Gesture | Action | Details |
|---------|--------|---------|
| Tap FIRE | Trigger performance macro | Momentary — active while held |
| Tap XOSEND | Toggle send routing | Latching — stays active until tapped again |
| Tap ECHO CUT | Kill delay feedback | Momentary — clears delay buffer while held |
| Tap PANIC | All notes off + FX reset | Instant — sends all-notes-off, resets FX tails |

### 5.7 Universal Gestures

These gestures work regardless of which zone is active:

| Gesture | Action | Context |
|---------|--------|---------|
| 2-finger pinch in | Open Orbit Path overlay | Any zone (iPhone) |
| 2-finger pinch out | Dismiss Orbit Path overlay | Orbit overlay active |
| 3-finger swipe L/R | Switch Note Input mode | iPhone landscape (PAD/FRETLESS/DRUM cycle) |
| Swipe up from bottom edge | Expand Performance Strip | iPhone portrait |
| Swipe down from header | Expand Macro Strip | iPhone portrait |
| Long press any knob | MIDI Learn or parameter options | Global |

---

## 6. Sensor Integration

### 6.1 Touch Pressure / Force

**Velocity Mapping:**

| Property | Source | Range | Notes |
|----------|--------|-------|-------|
| Touch force | `UITouch.force` | 0.0 - 6.67 | iPhone 6s+ with 3D Touch / Haptic Touch |
| Mapped velocity | Linear scale | 0 - 127 | Configurable curve (linear, exponential, logarithmic) |
| Fallback (no force) | Touch radius | Variable | Older devices use `majorRadius` as proxy |

**Aftertouch:**

Continuous pressure changes while a note is held generate channel aftertouch (or poly aftertouch in MPE mode). The force value is sampled at the audio callback rate and smoothed with a 10ms lowpass to prevent jitter.

### 6.2 Accelerometer Modulation

Configurable motion-to-modulation mapping. Disabled by default — opt-in via Settings > Motion Control.

| Axis | Default Mapping | Range | Notes |
|------|----------------|-------|-------|
| Tilt X (roll) | Filter cutoff | -45 to +45 degrees | Tilt left = cutoff down, tilt right = cutoff up |
| Tilt Y (pitch) | Pitch bend | -30 to +30 degrees | Tilt forward = pitch down, tilt back = pitch up |
| Tilt Z (yaw) | Unmapped | - | Available for user assignment |

**Jitter Prevention:**
- Dead zone: +/- 3 degrees around neutral position
- Smoothing: 50ms exponential moving average
- Rate limiting: updates at 60Hz max (not audio rate)
- Calibration: "Set neutral" button in settings captures current orientation as zero point

### 6.3 Gyroscope — Orbit Path Mapping

Optional "Tilt to Modulate" mode. When enabled, phone rotation maps directly to Orbit Path XY position:

- Gyroscope X rotation -> Orbit X position
- Gyroscope Y rotation -> Orbit Y position
- Sensitivity: configurable 0.25x to 4.0x
- Auto-center: returns to center when phone is level
- Conflicts with accelerometer modulation — only one motion mode active at a time

### 6.4 Haptic Feedback (Taptic Engine)

Haptic responses provide tactile confirmation of musical actions:

| Event | Haptic Type | UIKit API | Intensity |
|-------|------------|-----------|-----------|
| Pad strike | Light impact | `UIImpactFeedbackGenerator(.light)` | Proportional to velocity |
| Performance Strip snap-back | Medium impact | `UIImpactFeedbackGenerator(.medium)` | Fixed |
| Preset load complete | Success notification | `UINotificationFeedbackGenerator(.success)` | Fixed |
| Knob detent position | Selection changed | `UISelectionFeedbackGenerator()` | Fixed |
| PANIC pad activation | Heavy impact | `UIImpactFeedbackGenerator(.heavy)` | Fixed |
| Orbit Path boundary hit | Rigid impact | `UIImpactFeedbackGenerator(.rigid)` | Fixed |

Haptic feedback is globally toggleable in Settings. When the device is in Silent Mode, haptics still fire (they are independent of audio mute).

---

## 7. Parameter Access — Drawer System

On iPhone, engine parameters live in a bottom drawer that can be pulled up over the active zone. On iPad, parameters are always visible in the bottom section.

### 7.1 iPhone Drawer States

```
CLOSED (default):
┌─────────────────────────────┐
│                             │
│        ACTIVE ZONE          │
│                             │
│                             │
├─────────────────────────────┤  <- Handle bar visible (8pt)


PEEK:
┌─────────────────────────────┐
│                             │
│        ACTIVE ZONE          │
│       (compressed)          │
│                             │
├─────────────────────────────┤
│  Macro knobs + preset name  │  <- 100pt
│  + engine badges            │
├─────────────────────────────┤


HALF:
┌─────────────────────────────┐
│     ACTIVE ZONE             │
│     (top 50%)               │
├─────────────────────────────┤
│  Key params for active      │  <- 50% screen height
│  engine(s): 8-12 knobs      │
│  grouped by section         │
│  (Osc, Filter, Amp, FX)    │
├─────────────────────────────┤


FULL:
┌─────────────────────────────┐
│ [X close]  Engine Params    │  <- 90% screen height
│                             │
│  All parameters, scrollable │
│  Tabbed by engine           │
│  [Engine A] [Engine B]      │
│                             │
│  ┌─────┐ ┌─────┐ ┌─────┐  │
│  │ Osc │ │ Flt │ │ Amp │  │
│  └─────┘ └─────┘ └─────┘  │
│  ┌─────┐ ┌─────┐ ┌─────┐  │
│  │ Mod │ │ FX  │ │ Mix │  │
│  └─────┘ └─────┘ └─────┘  │
│                             │
└─────────────────────────────┘
```

### 7.2 Drawer Interaction

| Gesture | Action |
|---------|--------|
| Swipe up from bottom edge | Open drawer (Closed -> Peek -> Half -> Full) |
| Swipe down on drawer | Close drawer (Full -> Half -> Peek -> Closed) |
| Tap handle bar | Toggle between Closed and Peek |
| Tap outside drawer (while open) | Close to Closed |

### 7.3 Mode Defaults

| Mode | Default Drawer State | Maximum Drawer State |
|------|---------------------|---------------------|
| Intuitive | Closed (Peek available) | Half |
| Advanced | Peek | Full |

### 7.4 Knob Interaction in Drawer

- Standard knobs: 32pt visual with 44pt touch target
- Drag vertically on knob to adjust value (not rotary — vertical drag is more natural on touch)
- Double-tap knob to reset to default
- Long press knob for options: MIDI Learn, parameter range, modulation assignment
- Value readout appears above finger during drag (offset so it is not occluded)

---

## 8. MIDI Controller Support

This section applies to both desktop (macOS) and mobile (iOS/iPadOS) via CoreMIDI. The MIDI system is unified across platforms.

### 8.1 MIDI Learn

Any automatable parameter in XOceanus can be bound to an external MIDI controller via MIDI Learn.

**Workflow:**
1. Right-click (desktop) or long-press (mobile) on any knob, slider, or button
2. Select "MIDI Learn" from context menu
3. Move the desired MIDI controller (knob, fader, button, pad)
4. Binding is established — visual confirmation with a small MIDI plug icon on the control

**Storage Options:**
| Scope | Behavior |
|-------|----------|
| Per-preset | Mapping saved inside preset state — different controllers per sound |
| Global | Mapping saved in app settings — persists across all presets |
| User choice | Toggle in MIDI Settings: "Store mappings per preset" checkbox |

**Visual Feedback:**
- Learned parameters show a small MIDI plug icon (8x8pt) in the top-right corner
- When a MIDI message arrives for a learned parameter, the knob animates to the new value
- MIDI Learn mode highlights all learnable controls with an XO Gold border pulse

### 8.2 MPE (MIDI Polyphonic Expression)

XOceanus supports full MPE via both MIDI 2.0 and legacy MPE (MCM — MPE Configuration Message).

**Per-Note Expression Mapping:**

| MPE Dimension | XOceanus Target | PlaySurface Equivalent |
|---------------|----------------|----------------------|
| Per-note pitch bend | Fretless Y-axis | Pitch glide |
| Per-note pressure | Velocity / aftertouch | Touch force |
| Per-note slide (CC74) | X-axis mod target | Fretless X-axis / filter cutoff |

**MPE Controller Compatibility:**

MPE controllers map 1:1 to Fretless mode. When an MPE controller is detected, XOceanus automatically suggests switching to Fretless mode (user can decline).

**Zone Configuration:**

| Setting | Options | Default |
|---------|---------|---------|
| MPE Zone | Lower zone, Upper zone, Both | Lower zone |
| Member channels | 1-15 configurable | 15 (channels 2-16) |
| Master channel | 1 (lower) or 16 (upper) | 1 |
| Pitch bend range | 1-96 semitones | 48 semitones |

### 8.3 Standard CC Mapping (Defaults)

These default mappings are active out of the box. All are overridable via MIDI Learn.

| CC Number | CC Name | Default Target | Notes |
|-----------|---------|---------------|-------|
| CC1 | Mod Wheel | M2: MOVEMENT | Global modulation depth |
| CC2 | Breath Controller | M1: CHARACTER | Expression control — breath controllers map naturally |
| CC7 | Volume | Master Volume | Standard volume control |
| CC11 | Expression | M4: SPACE | FX depth — expression pedal controls reverb/delay |
| CC74 | Brightness | Filter cutoff | MPE slide also routes here |
| CC64 | Sustain Pedal | Sustain | Standard sustain behavior — notes held until pedal release |

### 8.4 Controller Presets

Pre-configured MIDI mappings for common hardware. Selectable from Settings > MIDI > Controller Preset. Each preset auto-maps the controller's physical controls to XOceanus parameters.

| Controller | Pad Mapping | Knob/Fader Mapping | Special |
|-----------|-------------|-------------------|---------|
| Akai MPD218/226 | Pads -> Pad Mode notes (velocity-sensitive) | Knobs -> M1-M4 macros, Faders -> engine params (osc, filter, amp, FX) | Bank A/B switches engine focus |
| Arturia KeyStep | Keys -> notes | Touch strip -> Performance Strip X-axis, Knobs -> M1-M4 macros | Arp/Seq pass-through supported |
| Novation Launchpad | Grid -> Pad Mode (8x8 scaled to scale-locked notes) | Side buttons -> Perf Pads (FIRE, SEND, CUT, PANIC), Faders -> macros | RGB LED feedback mirrors pad state |
| Akai MPC (pads) | Pads -> notes (Drum mode default) | Q-Links -> M1-M4 macros | TC/Control = Perf Strip X |
| Roli Seaboard | MPE: X -> mod, Y -> pitch, Z -> pressure | Slide -> filter cutoff | Auto-enables Fretless mode |
| Generic Keyboard | Keys -> notes (standard MIDI mapping) | Mod wheel -> M2, Pitch bend -> pitch | Sustain pedal -> CC64 sustain |

### 8.5 External Keyboard + Screen Surface Coexistence

When an external MIDI keyboard is connected and sending notes, the on-screen PlaySurface behavior changes:

**Note Input Zone:**
- Notes come from the external keyboard, NOT from on-screen pads
- On-screen PlaySurface switches to **Visual Feedback Mode**:
  - Active notes illuminate their corresponding pads
  - Velocity is shown as color intensity (heatmap: light = soft, saturated = hard)
  - The surface becomes a real-time visualization of what is being played
  - Tapping pads in this mode does NOT generate duplicate notes

**Non-Note Zones (unchanged):**
- Orbit Path: still generates parameter modulation via touch — it is not a note source
- Performance Strip: still active for XY modulation
- Performance Pads: FIRE, XOSEND, ECHO CUT, PANIC all still functional via touch
- Macro knobs: still adjustable via touch

**Auto-Detection:**
- External keyboard detected via CoreMIDI device connection notification
- Visual Feedback Mode activates automatically when note-on messages arrive from external source
- If external keyboard disconnects or stops sending for 5 seconds, pads revert to normal input mode
- Manual override available: Settings > MIDI > "Always use on-screen input" checkbox

---

## 9. CPU Budget on Mobile

### 9.1 Performance Targets

| Engine Configuration | iPhone Budget | iPad Budget | Voice Strategy |
|---------------------|--------------|------------|----------------|
| 1 engine | < 25% CPU | < 20% CPU | Full voice count (per engine spec) |
| 2 engines | < 45% CPU | < 35% CPU | 60% voice count per engine |
| 3 engines | Not recommended | < 50% CPU | Eco quality forced, 40% voice count |

### 9.2 Platform Constraints

| Platform | Max Engines | Quality Mode | Notes |
|----------|------------|-------------|-------|
| iPhone (all) | 2 | Eco mode available | 3 engines blocked in UI with explanation |
| iPad (standard) | 2 | Full quality default | 3 engines available with Eco forced |
| iPad Pro (M-series) | 3 | Full quality | All configurations supported |

### 9.3 Voice Reduction Strategy

Voice count automatically scales based on active engine count:

```
Voice pool = base_voices * engine_voice_factor[active_engine_count]

engine_voice_factor[1] = 1.00   (100% — full polyphony)
engine_voice_factor[2] = 0.60   (60% — reduced per engine)
engine_voice_factor[3] = 0.40   (40% — minimal per engine, Eco forced)
```

Voice stealing uses oldest-note priority. When a voice is stolen, it applies a 5ms fade-out to prevent clicks.

### 9.4 Buffer Size Configuration

| Setting | Buffer Size | Latency | Use Case |
|---------|------------|---------|----------|
| Low Latency | 128 samples | ~2.9ms @ 44.1kHz | Live performance, Fretless mode |
| Default | 256 samples | ~5.8ms @ 44.1kHz | General use, recommended |
| Max Polyphony | 512 samples | ~11.6ms @ 44.1kHz | Dense pads, maximum voice count |

Buffer size is configurable in Settings > Audio. The app monitors CPU usage and displays a warning badge if sustained usage exceeds 80%.

### 9.5 Eco Quality Mode

When Eco mode is active (forced for 3-engine on iPad, optional otherwise):
- Oversampling disabled (1x instead of 2x/4x)
- FX quality reduced (shorter reverb tails, simpler delay interpolation)
- Mod matrix update rate halved (every 2 blocks instead of every block)
- Wavetable interpolation: linear instead of cubic

---

## 10. AUv3 Integration

### 10.1 Supported Hosts

| Host | Platform | Status |
|------|----------|--------|
| GarageBand | iOS/iPadOS | Primary target — must work flawlessly |
| AUM | iOS/iPadOS | Pro audio routing — full MIDI + audio chain |
| Cubasis | iPad | DAW workflow — automation, mixing |
| BeatMaker 3 | iPad | Sample + synth workflow |
| Drambo | iPad | Modular routing |

### 10.2 AUv3 State Management

| Operation | Implementation |
|-----------|---------------|
| State save | Full parameter state + active engine config + preset name serialized via `AUAudioUnit.fullState` |
| State restore | Deserialize and apply — engine activation, parameter values, MIDI mappings |
| Preset browsing | Native XOceanus preset browser within AUv3 view (not host preset menu) |
| User presets | AUv3 user preset API supported — host can save/recall independently |

### 10.3 Audio + MIDI

| Feature | Implementation |
|---------|---------------|
| MIDI input | From host MIDI bus — notes, CC, program change |
| Audio output | Stereo main bus to host (additional buses for send FX if host supports) |
| Parameter automation | All parameters exposed to host automation via `AUParameterTree` |
| Tempo sync | Host tempo read via `AUAudioUnit.musicalContext` — sequencer and delay sync |
| Transport | Start/stop from host triggers sequencer start/stop |

### 10.4 AUv3 View Sizing

| Platform | View Mode | Size |
|----------|-----------|------|
| iPad (floating) | Resizable | Min 480x320, max full screen |
| iPad (docked) | Host-determined | Adapts to host panel size, minimum 480x320 |
| iPhone (expanded) | Full screen | Matches device screen, standard iPhone portrait layout |

The AUv3 view uses the same adaptive layout system as the standalone app. When the view is smaller than 600pt wide, it switches to iPhone portrait layout regardless of device.

---

## 11. Offline Rendering (Mobile)

### 11.1 Use Case

XPN export on mobile is a secondary feature. The primary export workflow is on desktop. Mobile rendering supports:
- Rendering individual presets to WAV for sharing
- Building small XPN expansion bundles
- Quick export for sampling into other mobile apps

### 11.2 Rendering Pipeline

```
┌─────────────────────────────────────────┐
│  Render Engine (background thread)       │
│                                          │
│  AVAudioEngine (offline render mode)     │
│  ├── XOceanus AU instance (internal)    │
│  ├── Render MIDI sequence per note      │
│  ├── Write to WAV via ExtAudioFile      │
│  └── Progress callback to UI            │
│                                          │
│  Constraints:                            │
│  - Foreground only (no background task) │
│  - One preset at a time                 │
│  - Progress bar in UI                   │
│  - Cancel button                        │
└─────────────────────────────────────────┘
```

### 11.3 Export Options

| Format | Details |
|--------|---------|
| WAV | 44.1kHz / 16-bit (default), 48kHz / 24-bit (optional) |
| XPN bundle | Collection of WAVs + XPM metadata, zipped |
| Share targets | Files app, AirDrop, iCloud Drive, other apps via Share Sheet |

### 11.4 Rendering Constraints

| Constraint | Value | Reason |
|-----------|-------|--------|
| Max concurrent renders | 1 | CPU budget — mobile cannot spare cores |
| Background rendering | Not supported | iOS suspends background audio processing |
| Max notes per render batch | 128 | Memory constraint — WAV buffers |
| Estimated time per note | ~2-4 seconds | Depends on preset complexity and note duration |

---

## 12. Implementation Notes

### 12.1 Conditional Compilation

All mobile-specific code is gated behind `XO_MOBILE`:

```cpp
#if XO_MOBILE
    // iOS/iPadOS specific: touch handling, sensors, haptics
    #include "MobileTouchHandler.h"
    #include "SensorManager.h"
    #include "HapticEngine.h"
#else
    // macOS specific: mouse handling, keyboard shortcuts
    #include "DesktopInputHandler.h"
#endif

// Shared across all platforms:
#include "PlaySurface.h"
#include "MIDILearnManager.h"
#include "PerformanceStrip.h"
```

### 12.2 Touch Handling Architecture

```cpp
// MobileTouchHandler.h
// Wraps UITouch events into platform-agnostic PlaySurface events

struct TouchEvent {
    int touchId;            // Unique per finger
    float x, y;             // Normalized 0-1 within zone
    float force;            // 0.0 - 1.0 (normalized from UITouch.force)
    float radius;           // Touch size (fallback velocity)
    TouchPhase phase;       // Began, Moved, Ended, Cancelled
    int64_t timestampMs;
};

// Multi-touch tracked per zone — each zone maintains its own
// active touch set to prevent cross-zone interference
```

### 12.3 Layout Breakpoints

```cpp
enum class LayoutMode {
    iPhonePortrait,     // width < 430pt
    iPhoneLandscape,    // width >= 430pt && height < 430pt
    iPadCompact,        // width >= 430pt && width < 600pt (Split View 33%)
    iPadRegular,        // width >= 600pt && width < 1024pt
    iPadFull,           // width >= 1024pt
    Desktop             // macOS (no XO_MOBILE)
};

// Layout mode determined by juce::Component::getWidth() at top level
// Recalculated on resize, orientation change, multitasking change
```

### 12.4 Sensor Manager

```cpp
// SensorManager.h
// Manages accelerometer, gyroscope, haptics
// All sensor data smoothed and rate-limited before reaching DSP

class SensorManager {
    bool motionEnabled = false;         // User opt-in
    float deadZoneDegrees = 3.0f;
    float smoothingMs = 50.0f;
    int updateRateHz = 60;

    // Outputs (normalized -1.0 to 1.0)
    float tiltX, tiltY, tiltZ;
    float gyroX, gyroY;
};
```

### 12.5 MIDI Learn Manager

```cpp
// MIDILearnManager.h
// Shared across desktop and mobile — no platform-specific code

struct MIDIMapping {
    int ccNumber;               // 0-127
    int channel;                // 0-15 (0 = omni)
    juce::String paramId;       // APVTS parameter ID
    float rangeMin, rangeMax;   // Mapping range (default 0-1)
    bool isPerPreset;           // true = saved with preset, false = global
};

// Storage: global mappings in app settings JSON
//          per-preset mappings in preset .xocmeta state
```

### 12.6 CPU Monitor

```cpp
// CPUMonitor.h
// Tracks audio thread CPU usage and triggers mitigation

class CPUMonitor {
    float currentLoad;          // 0.0 - 1.0
    float warningThreshold = 0.80f;
    float criticalThreshold = 0.95f;

    // At warning: show badge in UI
    // At critical: reduce voice count, suggest Eco mode
    // Sampled once per audio callback, smoothed over 1 second
};
```

---

## 13. Summary — Platform Capability Matrix

| Feature | iPhone | iPad | iPad Pro | macOS |
|---------|--------|------|----------|-------|
| Max engines | 2 | 2-3 | 3 | 3 |
| PlaySurface zones visible | 1 (carousel) | All | All | All |
| Orbit Path | Pinch overlay | Visible | Visible | Visible |
| Performance Strip | Peek/expand | Always visible | Always visible | Always visible |
| Performance Pads | Always visible | Always visible | Always visible | Always visible |
| Macro knobs | Header (24pt) | Header (40pt) | Header (40pt) | Header (40pt) |
| Parameter drawer | Yes (4 states) | Bottom section | Bottom section | Engine panels |
| Accelerometer mod | Yes | Yes | Yes | No |
| Gyroscope Orbit | Yes | Yes | Yes | No |
| Haptic feedback | Yes (Taptic) | Yes (limited) | Yes | No |
| MPE support | Yes | Yes | Yes | Yes |
| MIDI Learn | Yes | Yes | Yes | Yes |
| AUv3 | Yes | Yes | Yes | No (AU/VST3) |
| Offline render | Limited | Yes | Yes | Full |
| Multitasking | No | Split/Slide | Split/Slide | Windows |

---

*This document defines the mobile interaction contract and MIDI integration layer. All implementation should reference this spec alongside the Technical Design System and PlaySurface Specification.*
