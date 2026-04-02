# PlaySurface Visual Design Spec

## Direction: Engine Accent Adaptive + Refined Glow

The PlaySurface takes on the visual personality of whatever engine is loaded. All accent colors are derived at runtime from `engine->getAccentColour()`. Default (no engine): XO Gold `#E9C46A`.

## Color System

All UI elements use the engine accent color at varying alpha/tint levels:

| Element | Normal | Hover | Active/Hit |
|---------|--------|-------|------------|
| Pad background | accent @ 0.07 | radial-gradient accent @ 0.25 center | radial-gradient accent @ 0.42 center |
| Pad border | accent @ 0.18 | accent @ 0.35 | accent @ 0.50 |
| Pad text | accent @ 0.55 | accent @ 0.90 | #FFFFFF |
| Pad glow | none | 0 0 10px accent @ 0.15 | 0 0 14px accent @ 0.25 |
| Button off | accent @ 0.08 bg, accent @ 0.55 text | — | accent @ 0.25 bg, lighten(accent,60%) text |
| Orbit ring | accent @ 0.22 border | — | — |
| Orbit cursor | lighten(accent,60%) center → accent @ 0.50 edge | — | 2.5s pulse animation |
| Orbit trail | 2 ghost points, accent @ 0.22 / 0.12 | — | — |
| Strip bg | linear-gradient accent @ 0.08 → 0.03 | — | — |
| Strip bar | linear-gradient accent @ 0.70 → transparent | — | 0 0 8px accent @ 0.30 shadow |
| Strip label | accent @ 0.40 | — | — |
| Trigger | accent @ 0.06 bg, accent @ 0.15 border, accent @ 0.55 text | accent @ 0.16 bg, accent @ 0.35 border | — |
| Trigger hint | accent @ 0.45 | — | — |
| Orbit tab inactive | accent @ 0.40 text | — | accent @ 0.95 text, accent @ 0.12 bg |
| PANIC trigger | red @ 0.05 bg, red @ 0.25 border (always, ignores engine accent) | — | — |

## Pad Grid

- **Square pads** (1:1 aspect ratio, MPC-style)
- 4x4 grid with 3-4px gap
- Warm memory ghost: 20px radial gradient circle, accent @ 0.15, fades over 1.5s
- Grid background: subtle radial gradient, accent @ 0.04 center → #0c0c0e edge

## Scale Modes (toggle in header)

### Mode 1: Filter (MPC-style)
- Pads only show notes within the selected scale
- 16 pads = 16 consecutive scale degrees from the root
- Non-scale notes are skipped entirely
- Root note pad gets a subtle bottom border highlight in XO Gold

### Mode 2: Highlight (Push-style)
- All 16 chromatic pads shown
- In-scale notes: full accent color (normal brightness)
- Out-of-scale notes: dimmed (accent @ 0.15 border, accent @ 0.20 text)
- Hitting an out-of-scale pad quantizes to nearest in-scale note
- Root note highlighted with XO Gold bottom accent

### Toggle
- Button labeled "SCL" in the header row (next to PAD/FRETLESS/DRUM)
- Click cycles: OFF → FILTER → HIGHLIGHT → OFF
- When OFF: chromatic layout, no quantization
- Scale selector: dropdown or compact row (9 scales: Chromatic, Major, Minor, Dorian, Mixolydian, Pentatonic Maj, Pentatonic Min, Blues, Harmonic Minor)
- Root key selector: 12 compact letter buttons (C through B)

## Orbit Zone

- Ring size derived from container: `jmin(width, height) * 0.85`
- Crosshair lines at center (accent @ 0.07)
- Cursor: 13px, radial gradient, 2.5s gentle pulse animation
- 2-point trail (not 5-point cascade): sizes 8px and 6px, decreasing opacity
- Tab strip: FREE / LOCK / SNAP — active tab gets accent @ 0.10 bg

## Performance Strip

- Mode-specific background tint (already implemented via kStripModeTints)
- Bar: 3px wide, linear gradient from accent @ 0.65 to transparent
- Subtle floor glow under the bar: 18px wide radial gradient, accent @ 0.08

## Trigger Pads

- FIRE / X-SEND / ECHO: engine accent colored
- PANIC: always red (independent of engine accent)
- Keyboard hints (Z/X/C/V): 9px, accent @ 0.35

## Window Chrome

- Custom title bar (not native OS): dark bg #161618, "XOceanus — PlaySurface" in center
- PIN button in title bar for always-on-top toggle
- macOS traffic light dots (close/minimize/maximize)
- Window default: 720x520, resizable 320-1200, always-on-top by default

## Engine Color Update

When the active engine changes (via slot selection or engine swap):
1. Editor reads `engine->getAccentColour()`
2. Calls `playSurfaceWindow->getPlaySurface().setAccentColour(colour)`
3. PlaySurface stores the colour and calls `repaint()` on all zones
4. All paint() methods read the stored accent colour for their drawing

When no engine is loaded: fallback to XO Gold `#E9C46A`.
