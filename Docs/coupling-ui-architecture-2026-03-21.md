# Coupling Visualizer — UI Architecture
**Date:** 2026-03-21
**Status:** Design complete, scaffold implemented
**Component:** `Source/UI/CouplingVisualizer/CouplingVisualizer.h`
**Replaces/Extends:** `CouplingStripEditor` (linear list view → full graph view)

---

## 1. Design Intent

The Coupling Visualizer makes cross-engine routing *visible and tactile*. The 4 engine slots become nodes in a graph. Every active coupling route becomes an animated arc. Signal energy — derived from per-route RMS — pulses through the arc in real time, so you can *see* modulation flowing before you hear its effect.

The design follows the Gallery Model: warm white shell (#F8F6F3) background, engine accent colors on nodes, XO Gold (#E9C46A) for active/drag states. Light mode default; dark mode via `GalleryColors::darkMode()`.

---

## 2. Layout

```
┌──────────────────────────────────────────────────────────────────┐
│  COUPLING VISUALIZER                              [+] [CLR] [?]  │
│                                                                   │
│            ┌───────────┐                                         │
│            │  SLOT A   │  ←— engine accent color node           │
│            │  [name]   │       28px radius, filled ring          │
│            └───────────┘                                         │
│         ↗ arc (Bézier)  ↘                                        │
│   ┌───────────┐        ┌───────────┐                             │
│   │  SLOT B   │        │  SLOT C   │                             │
│   │  [name]   │        │  [name]   │                             │
│   └───────────┘        └───────────┘                             │
│                   ↑                                               │
│            ┌───────────┐                                         │
│            │  SLOT D   │                                         │
│            │  [name]   │                                         │
│            └───────────┘                                         │
│                                                                   │
│  ◈ AmpToFilter (2)   ◈ AudioToFM (1)   ◈ KnotTopology (1)      │
│  [drag from output port → input port to add route]               │
└──────────────────────────────────────────────────────────────────┘
```

### Slot Node Positions (diamond layout)

The 4 slots are arranged at the cardinal points of a diamond inscribed in the component bounds. This gives maximum visual separation between all pairs and avoids arcs crossing nodes.

```
         Slot 0 (top)
              ●
             / \
            /   \
   Slot 1  ●     ●  Slot 2
    (left)   \   /  (right)
              \ /
              ●
         Slot 3 (bottom)
```

Positions as fractions of component size (cx, cy = centre):

| Slot | x          | y          |
|------|-----------|-----------|
| 0    | cx        | cy - r    |
| 1    | cx - r    | cy        |
| 2    | cx + r    | cy        |
| 3    | cx        | cy + r    |

Where `r` = 0.38 × min(width, height). This keeps nodes well inside bounds at all reasonable plugin window sizes.

---

## 3. Node Rendering

Each slot node is a filled circle with a coloured ring:

- **No engine loaded:** ring colour `#DDDAD5` (borderGray), fill `#EAE8E4` (emptySlot). Text: "SLOT n" in textMid.
- **Engine loaded:** ring colour = engine accent colour, fill = accent at 15% opacity. Engine name in Inter 9pt bold.
- **Active (playing):** ring strokes to 3px. A soft glow halo (paint second, larger ellipse at 20% opacity, 1.5× radius) pulses with the engine's coupling RMS.
- **Drag target (drop zone):** ring animates to XO Gold (#E9C46A) at 80% opacity.

### Output / Input Ports

Each node exposes 8 micro-ports arranged around its circumference at 45° intervals (N, NE, E, SE, S, SW, W, NW). Ports are 6px diameter ellipses. On hover, they expand to 10px and show a tooltip with the port function.

- **Output ports** (top half of ring, 270°–90° arc): coloured with engine accent.
- **Input ports** (bottom half of ring, 90°–270° arc): coloured with engine accent at 60% opacity.

Port positions are computed from slot centre + unit vector at angle × (nodeRadius + 6px).

---

## 4. Arc Rendering

### Bézier Geometry

Each coupling route becomes a cubic Bézier from the source node's output port to the destination node's input port.

Control points push perpendicular to the direct line by a distance proportional to the chord length, ensuring arcs never pass through nodes:

```
P0 = source port position
P3 = dest port position
chord = P3 - P0
perp  = normalize(rotated chord 90°) × chord.length × 0.35f
P1 = P0 + forward_third + perp
P2 = P3 - forward_third + perp
```

Multiple routes between the same pair of slots receive staggered perpendicular offsets (`±0.35`, `±0.55`, `±0.75` × chord.length) so arcs don't overlap.

### Stroke Width and Alpha

```
strokeWidth = 1.5f + route.amount * 2.5f    // 1.5–4.0 px
baseAlpha   = 0.25f + route.amount * 0.55f  // 0.25–0.80
```

Normalled routes (pre-wired defaults) draw at 50% of computed alpha with a dashed stroke pattern.

### Glow Layer

Each arc is painted twice:

1. **Glow pass:** stroke at 3× width, coupling type colour at 12% opacity. Creates a soft bloom.
2. **Line pass:** stroke at computed width, coupling type colour at full alpha.

This is inexpensive (two `g.strokePath()` calls per route) and produces the neon-wire aesthetic.

### Animated Signal Pulse

The arc brightness is modulated by `routeRMS[routeIndex]` — a per-route float polled from the audio thread at 30 Hz via `std::atomic<float>`.

On each timer tick:
```
pulseBrightness = 0.6f + routeRMS * 0.4f   // 0.6–1.0 multiplied into alpha
```

The pulse creates a shimmer: bright when audio is flowing, dim when the route is inactive but connected.

For KnotTopology routes, *both* arc directions animate simultaneously, pulsing in opposite phase (phase offset = π) to reinforce the bidirectional entanglement metaphor.

### Arrowhead

At the destination end of the arc, a filled triangle arrowhead (8×6px) is painted in the coupling type colour. The triangle is rotated to match the arc's arrival tangent angle.

---

## 5. Coupling Type Color Map

All colours are chosen to:
- Read clearly on both `#F8F6F3` (light shell) and `#1A1A1A` (dark mode) backgrounds.
- Not clash with common engine accent colours.
- Form an internally consistent palette (warm → cool progression, danger types in red/orange).

| CouplingType     | Color Name              | Hex       | ARGB Constant     | Visual metaphor           |
|-----------------|------------------------|-----------|-------------------|---------------------------|
| AmpToFilter      | Cerulean                | #2196F3   | 0xFF2196F3        | Water / filter cutoff sky |
| AmpToPitch       | Cobalt                  | #1565C0   | 0xFF1565C0        | Deeper blue, pitch depth  |
| LFOToPitch       | Periwinkle              | #7986CB   | 0xFF7986CB        | LFO undulation, soft blue |
| EnvToMorph       | Orchid                  | #BA68C8   | 0xFFBA68C8        | Morph / transformation    |
| AudioToFM        | Vermilion               | #E53935   | 0xFFE53935        | FM = hot, danger-adjacent |
| AudioToRing      | Deep Orange             | #F4511E   | 0xFFF4511E        | Ring mod harshness        |
| FilterToFilter   | Steel Teal              | #4DB6AC   | 0xFF4DB6AC        | Filter chain, clean       |
| AmpToChoke       | Charcoal                | #546E7A   | 0xFF546E7A        | Choke = suppression, dark |
| RhythmToBlend    | Warm Amber              | #FFB300   | 0xFFFFB300        | Rhythm = pulse, warm      |
| EnvToDecay       | Sage Green              | #66BB6A   | 0xFF66BB6A        | Decay = organic, natural  |
| PitchToPitch     | Lavender                | #9C8FC4   | 0xFF9C8FC4        | Harmonic relationship     |
| AudioToWavetable | Cyan                    | #00ACC1   | 0xFF00ACC1        | Wavetable = spectral      |
| AudioToBuffer    | Mango                   | #FB8C00   | 0xFFFB8C00        | Buffer = stored / warm    |
| KnotTopology     | XO Gold                 | #E9C46A   | 0xFFE9C46A        | KNOT = brand identity     |

### Usage Rules

- KnotTopology arcs always render in XO Gold regardless of source engine colour. This makes KNOT routes instantly recognizable.
- Normalled (pre-wired) routes use the coupling type colour at 40% opacity, with a 4px-on / 4px-off dash pattern.
- User-defined routes use full opacity and solid stroke.
- A small colour swatch (8×8 px filled circle) appears at the arc midpoint for quick identification when multiple route types are present.

---

## 6. Drag-to-Connect Interaction

### State Machine

```
IDLE
  │  mouseDown on output port
  ▼
DRAGGING_FROM_SOURCE
  │  mouseDrag (rubber-band arc drawn from source port to cursor)
  │  mouseUp on input port → CREATE ROUTE
  │  mouseUp elsewhere    → CANCEL
  ▼
IDLE
```

### Visual Feedback During Drag

- A live Bézier arc is drawn from the source port to the current mouse position.
- The arc uses the *type colour of the last-used coupling type* (stored in `lastUsedType`).
- Valid drop targets (input ports on other nodes) highlight with XO Gold rings.
- Invalid targets (output ports, same node's ports, self-routes) show a red X overlay (10px, 2px stroke).

### Route Creation Dialog

On successful drop, a minimal inline popup appears near the dropped port:

```
┌─────────────────────────┐
│ Add Route               │
│ Type:  [AmpToFilter ▾]  │
│ Amount: ━━━━●━━━━  0.75 │
│ [Cancel]      [Add]     │
└─────────────────────────┘
```

The popup is a `juce::CalloutBox` containing a minimal form. On "Add", `couplingMatrix.addRoute()` is called on the message thread. The new route appears immediately in the next paint cycle.

### Keyboard Shortcuts

| Key                 | Action                                      |
|--------------------|---------------------------------------------|
| Delete / Backspace  | Delete selected arc                         |
| Escape              | Cancel drag / deselect                      |
| 1–9                 | Set selected route amount to n×0.1          |
| T                   | Cycle coupling type on selected route       |
| Cmd+Z               | Remove last-added user route                |

---

## 7. Right-Click Context Menu on Arcs

Right-clicking anywhere within 8px of an arc opens a `juce::PopupMenu`:

```
AmpToFilter: Slot 1 → Slot 3  (amount: 0.75)
───────────────────────────────
  Set Amount…
  Change Type →  [submenu: all 14 types]
  Toggle Normalled/User
  ───────────────────────────
  Delete Route
```

"Set Amount…" opens a `juce::AlertWindow` with a slider. Amount change calls `couplingMatrix.removeUserRoute()` + `couplingMatrix.addRoute()` atomically.

Hit-testing an arc: for each active route, evaluate the Bézier at 64 uniform t-values and check if the mouse position is within 8px of any sample point. The closest arc wins.

---

## 8. Animation and Performance Budget

### Timer Rate

`startTimerHz(30)` — 33ms per frame. The audio thread runs at 44.1–96 kHz; 30 Hz polling is adequate for human perception of amplitude-driven brightness.

### Per-Frame Work (message thread)

| Operation                         | Cost estimate         |
|----------------------------------|-----------------------|
| Read 14 atomic RMS floats        | ~14 ns                |
| Paint 4 node ellipses            | ~0.1 ms               |
| Paint up to 16 route arcs (×2 glow+line) | ~0.4 ms      |
| Paint arrowheads (×16)           | ~0.1 ms               |
| Paint drag rubber-band arc       | ~0.02 ms              |
| **Total**                        | **< 1 ms per frame**  |

At 30 fps this costs ≈ 30 ms/s = 3% of one CPU core. The audio thread is never touched from paint().

### Audio Thread Contribution

The `MegaCouplingMatrix::processBlock()` is the only audio-thread writer. After each block, it updates one `std::atomic<float>` per active route with the block's RMS magnitude. Total: 1 atomic store per active route per block. No memory allocation.

### Reduced Motion Mode

When `reducedMotion = true` (mirrors OpticVisualizer pattern):
- Timer drops to 10 Hz.
- Pulse animation disabled (arcs render at static brightness = `0.6f + amount * 0.4f`).
- Drag rubber-band arc renders as a straight line instead of Bézier.

---

## 9. RMS Feedback from Audio Thread

`MegaCouplingMatrix` exposes per-route RMS via a fixed-size atomic array:

```cpp
// In MegaCouplingMatrix (to be added):
static constexpr int MaxRMSSlots = 64; // matches MaxRoutes
std::array<std::atomic<float>, MaxRMSSlots> routeRMSOut = {};

// Called at end of processBlock(), audio thread:
// routeRMSOut[routeIndex].store(rms, std::memory_order_relaxed);
```

The CouplingVisualizer reads these values in `timerCallback()` and maps them to arc brightness. Because `MaxRMSSlots` is compile-time constant, no allocation or synchronization beyond the atomic store/load is needed.

---

## 10. Integration Points

### Where It Lives

`CouplingVisualizer` is a drop-in replacement for `CouplingStripEditor` inside `PerformanceViewPanel`. The constructor signature is identical:

```cpp
CouplingVisualizer viz(matrix, slotNameFn, slotColorFn);
```

`PerformanceViewPanel::resized()` gives it the left panel area (same bounds as the current `couplingStrip`).

### Relationship to CouplingStripEditor

`CouplingStripEditor` remains in the codebase as the compact strip view used in the main editor sidebar (narrow viewport, <120px height). `CouplingVisualizer` is the full-panel view shown in `PerformanceViewPanel` only. Both read from the same `MegaCouplingMatrix` — no duplication of data.

### Thread Safety

- `CouplingVisualizer::timerCallback()` runs on the message thread.
- It calls `couplingMatrix.getRoutes()` (atomic load) once per tick.
- It reads `routeRMSOut[i].load(std::memory_order_relaxed)` per route.
- No locks. No audio-thread calls. No shared mutable state outside atomics.

---

## 11. Accessibility

- All interactive elements (nodes, arcs, ports) carry ARIA roles via `juce::AccessibilityHandler`.
- Keyboard navigation: Tab cycles through nodes; Enter opens the route editor for the selected node.
- Screen reader: each arc announces "Route: [type] from [engine A] to [engine B], amount [n]%".
- Reduced motion: respects `juce::Desktop::getInstance().isReducedMotion()`.
- Contrast: all coupling type colours meet WCAG AA (4.5:1) against both shell white and dark backgrounds. AmpToChoke (#546E7A on #F8F6F3 = 4.6:1).

---

## 12. Future Work (V2)

| Feature                       | Notes                                               |
|------------------------------|-----------------------------------------------------|
| Route sequencer               | Coupling Phase E — cycle through presets on beat    |
| Wet/dry per-arc control       | Slider on arc midpoint, drag vertically             |
| Topology presets as thumbnails | Gallery of named graph configurations               |
| MIDI learn on arc amount      | Right-click → "MIDI Learn Amount"                   |
| WebAudio equivalent           | For aquarium.html Phase 2 (VQ 002)                 |
