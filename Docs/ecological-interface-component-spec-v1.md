# XO_OX Ecological Interface — Component Specification v1.0

**Studio:** UIX Design Studio (Ulf, Issea, Xavier) — XO_OX Designs
**Date:** 2026-03-22
**Paradigm:** The Ecological Interface — an environmental monitoring station for a synthetic ocean.
Parameters are habitat conditions. The player is a marine biologist. Sound is what 47 creature-engines produce in response.
**Mode:** Ecological dark-mode variant of the Gallery Model. Background `#0E0E10`. (Gallery Model warm white `#F8F6F3` = site/light mode — separate system.)

---

## Design System Constants (Ecological Mode)

```
Plugin background:      #0E0E10   (warm dark — not pure black)
Engine slot glass:      rgba(255,255,255,0.04) fill
                        1px solid rgba(255,255,255,0.08) border
XO Gold (brand):        #E9C46A   (macros, coupling strip, active states — constant)
Track / empty arc:      rgba(255,255,255,0.12)
Advice zone green:      #2ECC71   (normal habitat range)
Advice zone yellow:     #F4D03F   (creature stress)
Advice zone red:        #E74C3C   (critical / self-oscillation)
Setpoint marker:        rgba(255,255,255,0.85)  (white triangle)
Vignette inner shadow:  rgba(0,0,0,0.55)
Typography — display:   Nebulica (fallback: Space Grotesk Bold)
Typography — body:      Inter Regular
Typography — values:    JetBrains Mono
Focus ring:             #58A6FF  (2px, circular for knobs, rounded rect for buttons)
```

Engine accent colors map directly from `GalleryColors::accentForEngine()` in `XOmnibusEditor.h`. All 47 values are present there. The ecological arc color = engine accent color throughout this spec.

---

## Component 1: Ecological Instrument Knob

### Overview
The core interaction unit. Not a generic rotary knob — a circular ecological instrument face modeled on OpenBridge 6.1 (maritime circular dial with needle, arc, tick marks, advice zones) layered with CarPlay dark card aesthetics. The creature silhouette lives in the center. The gap between the setpoint marker and the filled arc IS the performance feel — the instrument shows both player intent and DSP reality simultaneously.

### Sizes

| Variant | Total diameter | Arc radius | Center clear zone | Label area |
|---------|---------------|------------|-------------------|------------|
| Small   | 40px          | 15px       | 10px radius       | 10px below |
| Medium  | 64px          | 27px       | 17px radius       | 12px below |
| Large   | 96px          | 40px       | 26px radius       | 14px below |

The large variant (96px) is used for primary parameters (e.g., CUTOFF, RESONANCE on the engine face). Medium (64px) for secondary parameters. Small (40px) for tertiary parameters in dense layouts and the collapsed indicator ring.

### Layer Stack (paint order, bottom to top)

**Layer 0 — Card background**
- Circular clip region, diameter = total size
- Fill: `rgba(255,255,255,0.03)` (slight lift from plugin background)
- No stroke at this layer

**Layer 1 — Porthole vignette**
- Radial gradient centered at knob center
- From: `rgba(0,0,0,0)` at radius 0.4× total
- To: `rgba(0,0,0,0.45)` at radius 0.5× total (edge)
- Creates the "looking through glass" recession

**Layer 2 — Track arc (full range)**
- Centered arc from -225° to +45° (270° sweep, standard rotary convention)
- Note: 0° = 12 o'clock. Arc starts at 7 o'clock, sweeps clockwise to 5 o'clock.
- Stroke width: 3px (medium), 2px (small), 4px (large)
- Color: `rgba(255,255,255,0.12)` — near-invisible empty track

**Layer 3 — Advice zone arcs (painted over track)**
Three partial arcs on the same radius as the track, painted in sequence:
- Green zone: lower 40% of range (0–40% = habitat normal). Color: `#2ECC71` at 30% opacity.
- Yellow zone: 40–75% of range (creature approaching stress). Color: `#F4D03F` at 30% opacity.
- Red zone: 75–100% of range (critical / self-oscillation risk). Color: `#E74C3C` at 30% opacity.

Zone boundaries are per-parameter metadata. DSP-critical params (e.g., filter resonance near self-oscillation) use asymmetric zone splits. Defaults: green 0–40%, yellow 40–75%, red 75–100%. Stored as `{greenMax: 0.4, yellowMax: 0.75}` floats in parameter metadata.

**Layer 4 — Filled value arc (DSP reality)**
- Same arc geometry, sweeping from start angle to the angle corresponding to the current **smoothed parameter value**
- This is the real-time DSP readout — it trails player gestures by the smoothing window (5ms default from `ParameterSmoother.h`)
- Color: engine accent color, full opacity
- Stroke width: same as track arc + 0.5px (slightly thicker to read over track)
- End cap: rounded (`PathStrokeType::rounded`)
- When value is in red zone: accent color interpolates toward `#E74C3C` at 40% weight (`accentColor.interpolatedWith(Colour(0xFFE74C3C), 0.4f)`)

**Layer 5 — Setpoint marker (player intent)**
- A small filled isoceles triangle, pointing inward toward knob center
- Position: on the arc radius, at the angle corresponding to the **raw unsmoothed parameter value** (the setpoint the player last touched)
- Size: 5px base × 7px height (medium), 4×6 (small), 7×9 (large)
- Color: `rgba(255,255,255,0.85)`
- The gap between this marker and the arc end = the lag between intent and reality = performance expressiveness
- When setpoint == DSP value (settled): marker sits exactly at arc end, triangle appears "docked"

**Layer 6 — Tick marks**
- 5 major ticks at 0%, 25%, 50%, 75%, 100% of range
- Major tick: 3px long line at the arc radius + 2px outset, 1px stroke, `rgba(255,255,255,0.35)`
- 4 minor ticks between each major pair (9 minors in total visible zone, i.e., at 12.5% spacing)
- Minor tick: 2px long, 0.5px stroke, `rgba(255,255,255,0.18)`
- Ticks are NOT drawn over advice zone colored arcs — they sit 1px outside the arc stroke

**Layer 7 — Creature icon zone (center circle)**
- Filled circle, radius = center clear zone value per size table
- Fill: `rgba(0,0,0,0.55)` — dark receded center, creates instrument well depth
- Stroke: 1px `rgba(255,255,255,0.08)`
- Contains the creature vector silhouette (Layer 8) and value readout (Layer 9)

**Layer 8 — Creature vector silhouette**
- Simplified bezier outline, one path per engine
- Size: fills ~70% of center clear zone diameter
- Idle state: silhouette fill = engine accent color at 20% opacity, stroke = accent at 60% opacity, 0.5px
- Note-on state: silhouette fill pulses to accent at 50% opacity over 80ms attack, decays over note release
- Animation: 2–3 keyframe vector morph on the bezier path control points, not a sprite. Amplitude of morph = velocity. Max morph displacement = 15% of silhouette bounding box.
- Reduced motion: when OS reduced-motion is active (`A11y::prefersReducedMotion()`), animation is disabled; silhouette brightens to accent at 40% opacity during note-on instead.
- Small (40px): silhouette only, no value readout. Center zone too small for text.

**Layer 9 — Value readout (medium and large only)**
- JetBrains Mono, 8pt (medium), 11pt (large)
- Displayed below or beside the silhouette within the center circle
- Color: `rgba(255,255,255,0.70)`
- Format: parameter-aware. Examples:
  - Frequency: "1.2k" (Hz abbreviated)
  - Percentage: "73%"
  - Semitones: "+3st"
  - Raw 0–1: "0.73" (2 decimal places)
- When knob is being dragged: value updates at 60fps. When settled: static.

**Layer 10 — Parameter label**
- Below the knob, centered
- Inter Medium, 7pt (small), 8pt (medium), 9pt (large)
- Color: `rgba(255,255,255,0.45)`
- ALL CAPS, max 8 characters (truncate with ellipsis)
- Examples: "CUTOFF", "RESO", "ATTACK", "FM AMT"
- On hover: label brightens to `rgba(255,255,255,0.75)`

**Layer 11 — Focus ring**
- Drawn only when `hasKeyboardFocus(true)`
- 2px circle at total radius + 2px outset
- Color: `#58A6FF`
- Does not interfere with porthole vignette — drawn outside the knob bounds

### JUCE paintComponent Description

```cpp
void paintEcologicalKnob(juce::Graphics& g, float cx, float cy, float totalRadius,
                         float setpointNorm, float dspValueNorm,
                         juce::Colour accentColor,
                         float greenMax, float yellowMax,
                         bool noteOn, float velocity)
{
    const float startRad = juce::MathConstants<float>::pi * 1.25f;  // 225° (7 o'clock)
    const float endRad   = juce::MathConstants<float>::pi * 2.75f;  // 495° = 135° (5 o'clock)
    const float sweep    = endRad - startRad;  // 270°
    const float arcR     = totalRadius * 0.70f;
    const float arcW     = totalRadius * 0.055f; // track stroke weight

    // Layer 1: porthole vignette (radial gradient approximated as ellipse sequence)
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0x00000000), cx, cy,
        juce::Colour(0x8A000000), cx + totalRadius, cy, true));
    g.fillEllipse(cx - totalRadius, cy - totalRadius, totalRadius*2, totalRadius*2);

    // Layer 2: track
    juce::Path track;
    track.addCentredArc(cx, cy, arcR, arcR, 0, startRad, endRad, true);
    g.setColour(juce::Colour(0x1FFFFFFF));
    g.strokePath(track, juce::PathStrokeType(arcW, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Layer 3: advice zones
    auto drawZoneArc = [&](float normStart, float normEnd, juce::Colour zoneColor) {
        float aStart = startRad + normStart * sweep;
        float aEnd   = startRad + normEnd   * sweep;
        juce::Path z;
        z.addCentredArc(cx, cy, arcR, arcR, 0, aStart, aEnd, true);
        g.setColour(zoneColor.withAlpha(0.30f));
        g.strokePath(z, juce::PathStrokeType(arcW, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    };
    drawZoneArc(0.0f,     greenMax,  juce::Colour(0xFF2ECC71));
    drawZoneArc(greenMax, yellowMax, juce::Colour(0xFFF4D03F));
    drawZoneArc(yellowMax, 1.0f,     juce::Colour(0xFFE74C3C));

    // Layer 4: filled arc (DSP value)
    float dspAngle = startRad + dspValueNorm * sweep;
    juce::Colour fillColor = accentColor;
    if (dspValueNorm >= yellowMax)
        fillColor = accentColor.interpolatedWith(juce::Colour(0xFFE74C3C), 0.4f);
    juce::Path fill;
    fill.addCentredArc(cx, cy, arcR, arcR, 0, startRad, dspAngle, true);
    g.setColour(fillColor);
    g.strokePath(fill, juce::PathStrokeType(arcW + 0.5f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    // Layer 5: setpoint triangle marker
    float spAngle = startRad + setpointNorm * sweep;
    float spX = cx + arcR * std::cos(spAngle - juce::MathConstants<float>::halfPi);
    float spY = cy + arcR * std::sin(spAngle - juce::MathConstants<float>::halfPi);
    // Triangle pointing inward; construct 3 points rotated to spAngle
    float triH = totalRadius * 0.085f, triW = totalRadius * 0.055f;
    juce::Path tri;
    tri.addTriangle(0, -triH*0.5f, -triW*0.5f, triH*0.5f, triW*0.5f, triH*0.5f);
    juce::AffineTransform t = juce::AffineTransform::rotation(spAngle).translated(spX, spY);
    g.setColour(juce::Colour(0xD9FFFFFF));
    g.fillPath(tri, t);

    // Layer 7: center well
    float cR = totalRadius * 0.38f;
    g.setColour(juce::Colour(0x8C000000));
    g.fillEllipse(cx - cR, cy - cR, cR*2, cR*2);
    g.setColour(juce::Colour(0x14FFFFFF));
    g.drawEllipse(cx - cR, cy - cR, cR*2, cR*2, 1.0f);

    // Layer 8: creature silhouette — getCreaturePath(engineId) returns normalized Path
    // centered at origin, fitting within unit circle radius 1
    // (implementation: per-engine static path, transformed here)
    float silR = cR * 0.70f;
    float silAlpha = noteOn ? (0.20f + 0.30f * velocity) : 0.20f;
    g.setColour(accentColor.withAlpha(silAlpha));
    // fillPath(creatureSilhouette, AffineTransform::scale(silR).translated(cx, cy));

    // Layer 11: focus ring
    // if (hasKeyboardFocus(true)) drawCircularFocusRing(g, cx, cy, totalRadius + 2);
}
```

### States

| State    | Arc behavior | Setpoint marker | Creature silhouette | Label |
|----------|-------------|-----------------|---------------------|-------|
| Idle     | DSP arc static, setpoint docked | White, alpha 0.85 | Accent 20% fill, 60% stroke | Alpha 0.45 |
| Hover    | No change | White, alpha 0.95 | Accent 30% fill, 80% stroke | Alpha 0.75 |
| Dragging | Setpoint marker moves instantly; arc follows with 5ms lag | White, full opacity | Accent 40% fill | Label shows value instead of name |
| Note-on  | No change to arc | Same | Silhouette pulses per velocity | Same |
| Warning (yellow) | Accent interpolates toward yellow | Yellow tint | Silhouette gains yellow tint | Same |
| Critical (red)   | Arc flashes at 2Hz (250ms on/off) | Red tint | Silhouette pulses at 2Hz | Label turns red |
| Disabled | Arc alpha 0.3, no advice zones | Not shown | Silhouette alpha 0.1 | Alpha 0.25 |

---

## Component 2: Engine Slot Panel

### Overview
The porthole container. Each of the 47 engines lives in one of up to 8 active engine slots. The slot is styled as a porthole — you are looking through rounded glass at the creature's habitat. Engine accent color anchors the header bar and all knob arcs within the slot.

### Dimensions
- Minimum slot size: 280px wide × 220px tall (compact layout, 3 engines visible)
- Comfortable slot size: 360px wide × 280px tall (2 engines visible)
- Maximum (single engine focus): full panel width minus coupling gutter (30px each side)
- Corner radius: 12px
- Internal padding: 12px all sides

### Layer Stack

**Layer 0 — Porthole glass background**
- Fill: `rgba(255,255,255,0.04)` — barely-there glass lift
- Border: `1px solid rgba(255,255,255,0.08)` — glass edge
- Border radius: 12px
- Box shadow / drop shadow (JUCE: DropShadow): `0 4px 24px rgba(0,0,0,0.6)` — depth below surface

**Layer 1 — Inner vignette (porthole recession)**
- Drawn as a soft radial gradient outset from the panel edges
- Fill: transparent center → `rgba(0,0,0,0.35)` at all four edges
- Implementation in JUCE: 4 separate `LinearGradient` fills along each edge, 24px wide, `rgba(0,0,0,0.0)` → `rgba(0,0,0,0.35)`
- Creates the sense of curved glass bowing the view inward at the edges

**Layer 2 — Habitat zone header**
- Height: 28px, full slot width, top-aligned
- Left side: creature silhouette icon at 16px diameter, vertically centered, engine accent color at 70% opacity
- Engine name: Inter Medium 10pt, `rgba(255,255,255,0.90)`, uppercase, adjacent to icon, 6px gap
- Right side: habitat status text — "STABLE" / "STRESS" / "CRITICAL" in Inter Medium 8pt
  - STABLE: `rgba(255,255,255,0.35)`
  - STRESS: `#F4D03F`
  - CRITICAL: `#E74C3C` with 2Hz blink
- Accent color bar: 2px tall × full width rule at bottom of header, engine accent color at 60% opacity
- Below bar: 8px padding gap before first knob row

**Layer 3 — Knob grid (4–8 knobs)**
- Knobs arranged in a responsive grid: prefer 4-column for ≥280px width, 3-column for narrow
- Small knobs (40px) for 6–8 parameter slots; medium (64px) for 4–5 parameter slots
- Primary parameters (CUTOFF, RESONANCE, envelope parameters) always occupy medium or large knobs
- Each cell: knob centered in cell, label below
- Cell sizing: `(panelWidth - padding*2) / columns`, equal height cells
- First row always shows the engine's most expressive parameters (determined per-engine in parameter metadata `"ecologicalPriority"` field)

**Layer 4 — Coupling arc attachment points**
- Four compass-point anchors on the slot border: North, South, East, West
- Each anchor: 6px diameter filled circle, engine accent color, alpha 0.0 when no route attached
- When a coupling route is active: anchor brightens to accent color alpha 1.0, gains a 2px glow ring (accent at 30% alpha, radius +4px)
- Anchors are hit-testable for future drag-to-connect interaction
- East anchor = preferred output (left-to-right routing convention)
- West anchor = preferred input

**Layer 5 — Collapsed / expanded toggle**
- Bottom-right corner: 16px × 16px chevron button
- Collapsed: chevron-down glyph, `rgba(255,255,255,0.30)`
- Expanded: chevron-up glyph, `rgba(255,255,255,0.30)`
- On hover: glyph brightens to accent color

### JUCE paintComponent key calls

```cpp
// Panel background
g.setColour(juce::Colour(0x0AFFFFFF));   // rgba(255,255,255,0.04)
g.fillRoundedRectangle(bounds, 12.0f);
g.setColour(juce::Colour(0x14FFFFFF));   // rgba(255,255,255,0.08)
g.drawRoundedRectangle(bounds.reduced(0.5f), 12.0f, 1.0f);

// Header accent bar (bottom of header strip)
auto headerBounds = bounds.removeFromTop(28.0f);
g.setColour(accentColor.withAlpha(0.60f));
g.fillRect(headerBounds.removeFromBottom(2.0f));

// Inner vignette — four edge strips
// (repeat for each edge, rotate gradient direction)
g.setGradientFill(juce::ColourGradient(
    juce::Colour(0x59000000), bounds.getX(), bounds.getCentreY(),  // alpha ~35%
    juce::Colour(0x00000000), bounds.getX() + 24.0f, bounds.getCentreY(), false));
g.fillRect(bounds.withWidth(24.0f));  // left vignette strip
```

### Coupling Attachment Points (JUCE)

```cpp
// Attachment point positions (relative to slot bounds):
// North: (cx, bounds.getY())
// South: (cx, bounds.getBottom())
// East:  (bounds.getRight(), cy)
// West:  (bounds.getX(), cy)

void drawAttachmentPoint(juce::Graphics& g, float px, float py,
                         bool active, juce::Colour accent)
{
    float r = 3.0f;
    if (active) {
        g.setColour(accent.withAlpha(0.30f));
        g.fillEllipse(px - r*2.33f, py - r*2.33f, r*4.67f, r*4.67f);  // glow ring
        g.setColour(accent);
    } else {
        g.setColour(juce::Colour(0x00000000));
    }
    g.fillEllipse(px - r, py - r, r*2, r*2);
}
```

### States

| State      | Border | Header bar | Vignette | Attachment points |
|------------|--------|------------|----------|-------------------|
| Idle       | 8% white | Accent 60% | Present | Hidden (alpha 0) |
| Hover      | 14% white | Accent 80% | Slightly lighter | Show if routes exist |
| Selected   | Accent color 100% 1.5px | Accent 100% | Lighter | All visible |
| Coupling active | Accent 80% 1.5px pulsing | Accent 100% | Normal | Active anchors glow |
| Disabled (no engine) | 6% white dashed | `rgba(255,255,255,0.15)` | Heavy vignette | Hidden |

---

## Component 3: Macro Slider (Neptune Fader Filmstrip)

### Overview
The Neptune fader filmstrip (129-frame mint vertical fader, production-ready) provides the visual vocabulary for all 4 macro sliders (CHARACTER, MOVEMENT, COUPLING, SPACE). Each macro slot uses the filmstrip with an engine accent color tint override. For non-teal engines the mint maps to the engine's accent color via color matrix application.

### Dimensions
- Slot width: 32px
- Slot height: full panel height (minus 20px padding top and bottom)
- 4 macro slots side by side in the macro strip, total strip width: 128px + 12px gaps = 164px
- Touch target on iOS: minimum 44px × 44px per WCAG 2.5.8 — achieved by expanding the hit region 6px on each horizontal side without expanding the visual frame

### JUCE Implementation: ImageStrip

The Neptune filmstrip is a vertical contact sheet: 129 frames stacked vertically, each frame 32px wide × `(totalFilmstripHeight / 129)` px tall.

```cpp
class NeptuneMacroSlider : public juce::Slider
{
public:
    NeptuneMacroSlider(const juce::Image& filmstrip,
                       juce::Colour engineAccent,
                       const juce::String& macroLabel)
        : filmstripImage(filmstrip), accent(engineAccent)
    {
        setSliderStyle(juce::Slider::LinearBarVertical);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setRange(0.0, 1.0);
    }

    void paint(juce::Graphics& g) override
    {
        double norm = (getValue() - getMinimum()) / (getMaximum() - getMinimum());
        int frameIndex = juce::jlimit(0, 128, (int)(norm * 128.0));

        // Each frame: filmstripImage.getHeight() / 129 pixels tall
        int frameH = filmstripImage.getHeight() / 129;
        juce::Rectangle<int> srcRect(0, frameIndex * frameH,
                                      filmstripImage.getWidth(), frameH);

        // Tint application: draw frame, then overlay a multiply/screen blend
        // JUCE approach: draw frame normally, then draw solid color rect
        // at reduced opacity with FillType using accent color
        g.drawImage(filmstripImage, 0, 0, getWidth(), getHeight(),
                    srcRect.getX(), srcRect.getY(),
                    srcRect.getWidth(), srcRect.getHeight());

        // Color tint overlay (replaces mint with engine accent color)
        // Blend mode approximation: draw accent over the frame at ~25% opacity
        // In practice, pre-generate 47 tinted filmstrip variants at build time
        // or use a JUCE ColourMatrix shader if OpenGL renderer is active.
        g.setColour(accent.withAlpha(0.25f));
        g.fillRect(getLocalBounds());
    }
};
```

**Production recommendation:** Pre-generate 47 tinted filmstrip images at build time (one per engine accent color) using an image processing step in the CMake pipeline. Bake each tinted variant into BinaryData. This avoids per-frame runtime tinting and ensures accurate color. Use Photoshop-equivalent "Hue/Saturation" adjustment targeting the mint hue range (165°–180°) and rotating to the target accent hue.

### Macro Label Treatment
- Below each slider: Inter Medium 7pt, ALL CAPS
- Colors: CHARACTER = `#E9C46A` (XO Gold), MOVEMENT = accent tint, COUPLING = `#E9C46A`, SPACE = accent tint
- On interaction: label pulses to full white for 200ms

### Macro Value Readout
- Small floating pill above the thumb during drag only
- Pill: `rgba(0,0,0,0.75)` fill, 4px border radius, 4px horizontal padding, 2px vertical
- Value: JetBrains Mono 9pt, `rgba(255,255,255,0.90)`
- Disappears 800ms after pointer release (fade out 200ms)

### States

| State    | Filmstrip | Tint | Label | Value pill |
|----------|-----------|------|-------|------------|
| Idle     | Frame per value | Accent 25% | Gold/accent alpha 0.7 | Hidden |
| Hover    | Same frame | Accent 35% | Alpha 1.0 | Hidden |
| Dragging | Frame updates at 60fps | Accent 35% | White | Visible |
| Disabled | Frame 64 (midpoint) | Grey tint | Alpha 0.3 | Hidden |

---

## Component 4: Coupling Arc

### Overview
The visual connection between engine slots. Not a static line — an animated Bezier curve that breathes with coupling intensity. The arc gradient uses the source engine's accent color at the source end and the destination engine's accent color at the destination end, transitioning through an interpolated midpoint color.

### Geometry
- Arc type: cubic Bezier, two control points
- Source point: East attachment of source slot
- Destination point: West attachment of destination slot (or nearest compass anchor)
- Control point 1: source.x + `offsetMagnitude`, source.y
- Control point 2: dest.x - `offsetMagnitude`, dest.y
- `offsetMagnitude` = `abs(dest.x - source.x) * 0.45` for horizontal connections; adjust to `abs(dest.y - source.y) * 0.45` for vertical
- For same-row engines (horizontal): arcs bow upward or downward by ±40px in the control Y offset to avoid overlapping other arcs. Direction alternates by route index.

### Stroke
- Minimum stroke width: 1.0px (amount = 0.01, barely visible)
- Maximum stroke width: 4.0px (amount = 1.0, full coupling)
- Formula: `strokeWidth = 1.0 + amount * 3.0` — matches existing `CouplingStripEditor.h` convention
- Alpha: `alpha = 0.3 + amount * 0.7` — same convention

### Gradient
JUCE does not natively support gradient strokes on paths. Implementation pattern:
1. Draw the arc stroke in source engine accent color at `alpha * 0.6`
2. Draw a second pass in destination accent color at `alpha * 0.6` using a clipping region covering the destination half of the path (clip from midpoint to destination)
3. Draw a third pass at midpoint using `srcColor.interpolatedWith(dstColor, 0.5f)` at `alpha * 0.4` for a 4px blended zone

Alternatively, if the OpenGL renderer is active, use a custom shader to pass a gradient uniform along the path's arc-length parameter.

### Coupling Type Chip
- At arc midpoint: a small rounded pill showing the coupling type short label
- Size: 28px wide × 12px tall, border radius 4px
- Background: `rgba(0,0,0,0.70)`
- Border: 1px source accent color at 50% alpha
- Text: JetBrains Mono 7pt, source accent color at 90% alpha
- Labels: "Amp>F", "Amp>P", "LFO>P", "Env>M", "FM", "Ring", "F>F", "Choke", "Rhy>B", "Env>D", "P>P", "WT", "A>B" — from `CouplingStripEditor::couplingTypeShortLabel()`

### Pulsing Animation
When a note is active through either coupled engine:
- Arc stroke width modulates ±0.5px at the note's frequency (clamped 0.5–8Hz for visual legibility)
- Alpha modulates ±0.15 at the same rate
- Animation driven by a timer callback on the UI thread at 60fps
- Reduced motion: no pulse; arc simply brightens to full alpha while note held

### States

| State       | Stroke | Alpha | Chip | Pulse |
|-------------|--------|-------|------|-------|
| Inactive    | 0px (hidden) | 0 | Hidden | Off |
| Active (low amount, 0–0.3) | 1.0–1.9px | 0.3–0.51 | Visible | Off |
| Active (mid, 0.3–0.7) | 1.9–3.1px | 0.51–0.79 | Visible | Off |
| Active (high, 0.7–1.0) | 3.1–4.0px | 0.79–1.0 | Visible | Off |
| Note-on pulsing | ±0.5px modulation | ±0.15 modulation | Chip pulses matching | 60fps |
| Normalled route | Arc color = XO Gold (`#E9C46A`) at 50% opacity per existing code | 0.5× normal alpha | Gold border chip | Off |
| Selected (clicked) | Stroke +1.0px | Full alpha | Chip highlighted XO Gold | Off |

---

## Component 5: Preset Browser / Ecosystem Atlas

### Overview
The preset browser becomes an Ecosystem Atlas. Each preset thumbnail communicates the sonic environment visually: which engines are active, what coupling routes connect them, and the rough environmental condition of each engine. The thumbnail is generative — produced at preset-save time and stored as a small PNG.

### Thumbnail Specification

**Size:** 120px wide × 80px tall (2:1.33 ratio, card landscape orientation)
**Background:** `#0E0E10` (plugin background, dark)
**Generated at:** preset save time (offline, non-real-time, runs on worker thread)

**Thumbnail content (render order):**

1. **Engine node circles:** One small circle per active engine slot
   - Diameter: 14px
   - Fill: engine accent color at 60% opacity
   - Stroke: engine accent color at 90% opacity, 1px
   - Arranged in a 2×4 or 3×3 grid centered in the thumbnail, with 6px gaps
   - Empty slots: `rgba(255,255,255,0.08)` circle, no fill

2. **Coupling arcs (thumbnail-scale):**
   - Bezier arcs between node centers, stroke 1px
   - Color: `srcColor.interpolatedWith(dstColor, 0.5f)` at `0.3 + amount * 0.5` alpha
   - Drawn before the node circles so nodes read on top

3. **Environmental condition rings:**
   - Each active node has a thin outer ring (3px gap outside node, 2px stroke)
   - Ring covers the arc from 0° to `currentValueNorm × 360°`
   - Ring color: green zone = `#2ECC71`, yellow = `#F4D03F`, red = `#E74C3C` — determined by the preset's primary parameter value for that engine

4. **Mood color wash:**
   - Top-right corner: 20×20px filled square in the preset's mood color
   - Mood colors: Foundation `#8B6914`, Atmosphere `#4A7B9D`, Entangled `#7B2D8B`, Prism `#00B4A0`, Flux `#E8701A`, Aether `#A78BFA`, Family `#E8839B`, Submerged `#1B4F8A`
   - Opacity: 80%
   - Corner radius: 3px (right-aligned, 4px margin)

5. **Preset name text (on thumbnail):**
   - Bottom strip: 16px tall, `rgba(0,0,0,0.65)` fill
   - Preset name: Inter Medium 7pt, `rgba(255,255,255,0.85)`, left-aligned, 4px left margin
   - Truncated with ellipsis at 90px width

### Browser List Layout

Each preset entry in the list:
- Row height: 88px
- Left: thumbnail 120×80px, vertically centered, 4px margin
- Right of thumbnail (8px gap):
  - Row 1: Preset name — Space Grotesk Bold 11pt, `rgba(255,255,255,0.90)`
  - Row 2: Engine names — Inter Regular 8pt, `rgba(255,255,255,0.45)`, comma-separated, max 3 shown then "+N more"
  - Row 3: Mood tag pill + 6D Sonic DNA bar
    - Mood pill: 6pt Inter Medium, mood color background at 20%, mood color text, 6px horizontal padding, 3px border radius
    - DNA bar: 6 micro-bars (brightness, warmth, movement, density, space, aggression), each 8px wide × 12px tall, colored `rgba(255,255,255,0.15)` to accent with fill per DNA value
  - Row 4: Coupling count — "3 routes" in JetBrains Mono 7pt, `rgba(255,255,255,0.30)`
- Right edge: heart icon (favorite) + XO Gold star (Guru Bin tier) 18px touch target each

### Browser Search / Filter Bar
- Top of browser: full-width search input
- Background: `rgba(255,255,255,0.06)`, border radius 6px, 1px `rgba(255,255,255,0.10)` border
- Placeholder: "Search ecosystem..." in Inter Regular 9pt, `rgba(255,255,255,0.30)`
- Active: border brightens to `rgba(255,255,255,0.25)`
- Filter pills below search: mood categories as scrollable horizontal pill row
  - Each pill: 8pt Inter Medium, mood color text, `rgba(mood,0.15)` background, active = `rgba(mood,0.80)` background white text
  - All filter bar / pill height: 24px, border radius 12px, 8px horizontal padding

### Thumbnail Generation (JUCE OfflineAudioContext equivalent)

```cpp
// Run on background thread via MessageManager::callAsync or ThreadPool
juce::Image generatePresetThumbnail(const XOPreset& preset, int w, int h)
{
    juce::Image img(juce::Image::ARGB, w, h, true);
    juce::Graphics g(img);

    // Background
    g.fillAll(juce::Colour(0xFF0E0E10));

    // Position engine nodes in grid
    int activeCount = countActiveEngines(preset);
    auto positions = computeNodePositions(w, h, activeCount);

    // Draw coupling arcs first
    for (const auto& route : preset.couplingRoutes) {
        drawThumbnailArc(g, positions[route.src], positions[route.dst],
                         getEngineAccent(route.src), getEngineAccent(route.dst),
                         route.amount);
    }

    // Draw engine nodes over arcs
    for (int i = 0; i < activeCount; ++i)
        drawThumbnailNode(g, positions[i], getEngineAccent(i),
                          getEngineConditionNorm(preset, i));

    // Mood swatch
    drawMoodSwatch(g, w, h, preset.mood);

    // Name strip
    drawNameStrip(g, w, h, preset.name);

    return img;
}
```

---

## Component 6: Collapsed Indicator

### Overview
The 24px circular indicator for overview tables, collapsed engine lists, and any context where a full slot panel does not fit. Communicates creature identity and habitat health at a glance.

### Dimensions
- Total diameter: 24px
- Center creature zone: 12px diameter
- Health ring: 2px stroke at 11px radius (1px gap from creature zone edge)

### Layer Stack

**Layer 0 — Background circle**
- Fill: `rgba(255,255,255,0.04)`
- Stroke: 1px `rgba(255,255,255,0.08)`

**Layer 1 — Health ring**
- Partial arc from 0° to `healthNorm × 360°` — clockwise from 12 o'clock
- Health norm = weighted average of all active parameters' normalized distances from advice zone red threshold. 100% = all parameters in green. 0% = all parameters in red.
- Color: interpolation across green/yellow/red based on healthNorm:
  - 1.0–0.6: `#2ECC71`
  - 0.6–0.3: `#F4D03F`
  - 0.3–0.0: `#E74C3C`
- Background ring track: `rgba(255,255,255,0.10)`, full 360°

**Layer 2 — Creature icon**
- Creature vector silhouette scaled to 8px diameter
- Color: engine accent color at 70% opacity
- Note-on: brightens to 100% opacity

**Layer 3 — Focus ring**
- 2px circle at 14px radius when keyboard-focused
- Color: `#58A6FF`

### Touch target
- Visual: 24px
- Hit region: 44px × 44px centered (iOS), 24px × 24px (desktop) per WCAG 2.5.8

### States

| State     | Health ring | Creature icon | Background |
|-----------|-------------|---------------|------------|
| Idle/stable | Green arc, alpha 0.90 | Accent 70% | Base |
| Stress    | Yellow arc, alpha 0.90 | Accent 70% | Slight yellow wash `rgba(244,208,63,0.05)` |
| Critical  | Red arc, 2Hz blink | Accent 70% | Slight red wash |
| No engine | No ring | Dash glyph, grey | `rgba(255,255,255,0.03)` |
| Note-on   | Ring brightens +20% | Accent 100% | Base |

---

## Component 7: Tab Navigation (ENGINE / EFFECTS / COUPLING / SETTINGS)

### Overview
Top-level navigation for the plugin editor. Four tabs, dark pill treatment. Active tab marked with engine-accent-color underline (not a generic blue underline). This is the one place tab chrome appears — kept minimal to not compete with engine content.

### Dimensions
- Tab bar total height: 36px
- Individual tab width: calculated as `(totalWidth - 32px margins - 3 × 12px gaps) / 4`
  - At 900px plugin width: `(900 - 32 - 36) / 4 = 208px` per tab
- Tab pill height: 28px (8px above/below from bar height)
- Border radius: 6px

### Visual Treatment

**Inactive tab:**
- Fill: `rgba(255,255,255,0.04)`
- Border: 1px `rgba(255,255,255,0.08)`
- Label: Inter Medium 9pt, `rgba(255,255,255,0.45)`, ALL CAPS
- Bottom border (the "underline slot"): 2px solid `rgba(255,255,255,0.08)`

**Hover tab:**
- Fill: `rgba(255,255,255,0.07)`
- Label: `rgba(255,255,255,0.70)`

**Active tab:**
- Fill: `rgba(255,255,255,0.08)` — slightly lifted vs. inactive
- Border: 1px `rgba(255,255,255,0.14)`
- Label: `rgba(255,255,255,0.95)`, Inter Medium 9pt, slight weight increase (use Inter SemiBold if available)
- Bottom border underline: 2px solid engine accent color, inset 0px from bottom of pill. Width matches label text width +8px, centered under label.
  - Note: For ENGINE tab = accent of the currently-focused engine slot. For EFFECTS, COUPLING, SETTINGS tabs = XO Gold `#E9C46A` (brand constant, no specific engine association).
- No background transition on activation — only the underline and label change

**Tab bar background:**
- Full-width strip behind all tabs
- Fill: `rgba(0,0,0,0.30)` — slightly darker than plugin background to anchor navigation
- Bottom border: 1px `rgba(255,255,255,0.06)`

### JUCE paintComponent

```cpp
void drawTab(juce::Graphics& g, juce::Rectangle<float> r,
             const juce::String& label, bool active, bool hover,
             juce::Colour engineAccent)
{
    // Background pill
    g.setColour(active ? juce::Colour(0x14FFFFFF)
               : hover ? juce::Colour(0x0DFFFFFF)
                       : juce::Colour(0x0AFFFFFF));
    g.fillRoundedRectangle(r, 6.0f);

    g.setColour(active ? juce::Colour(0x23FFFFFF) : juce::Colour(0x14FFFFFF));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);

    // Label
    float labelAlpha = active ? 0.95f : hover ? 0.70f : 0.45f;
    g.setColour(juce::Colour(1.0f, 1.0f, 1.0f, labelAlpha));
    g.setFont(GalleryFonts::label(9.0f));
    g.drawText(label, r, juce::Justification::centred);

    // Active underline
    if (active) {
        juce::String w_str = label; // measure text width
        float textW = g.getCurrentFont().getStringWidthFloat(label) + 8.0f;
        float ux = r.getCentreX() - textW * 0.5f;
        float uy = r.getBottom() - 3.0f;  // 3px from bottom of pill
        g.setColour(engineAccent);
        g.fillRect(ux, uy, textW, 2.0f);
    }
}
```

### Tab Content Areas
Each tab reveals its content panel below. Transitions: crossfade 120ms. No slide animation (respects reduced motion by default at 120ms).

- ENGINE: engine slot grid + parameter detail, macro strip visible
- EFFECTS: MasterFX chain panels
- COUPLING: full-screen CouplingStripEditor, enlarged node view
- SETTINGS: plugin settings, theme toggle, MIDI config, accessibility options

---

## Component 8: iOS AUv3 Adaptations

### General Principles
iOS AUv3 runs at smaller screen real estate (iPad: ~1024×768 in split-screen, ~1366×1024 full). Touch targets must be minimum 44×44pt per WCAG 2.5.8 and Apple HIG. The "diving interface" metaphor extends the porthole concept — operating the synth on iPad is like diving with a wrist instrument panel.

### Knob → Slider Conversion Rules

The ecological knob face is preserved on iPad at 64px and 96px (medium and large). At 40px (small), the knob is replaced by a compact horizontal bar slider on iOS.

**Compact bar slider (iOS small parameter replacement):**
- Height: 28pt (touch-safe)
- Width: full available cell width (minimum 88pt for safe touch)
- Track: full width, 4pt height, corner radius 2pt, `rgba(255,255,255,0.12)`
- Fill arc: advice zone gradient applied along track length (left = green, right = red, consistent with knob zone convention)
- Thumb: 20×28pt draggable region, visual thumb = engine accent color filled circle 14pt diameter
- Value readout: JetBrains Mono 9pt pill appearing above thumb during drag

**Minimum touch targets applied globally:**
- All knobs that remain as knobs: wrap in a transparent 44×44pt UIView/Component that routes touch events, even if the visual knob is 40pt or smaller
- Tab bar height: 44pt minimum (increase from desktop 36px)
- All buttons: minimum 44×44pt hit region

### Modal Sheets (Bottom Sheet from Bottom)
The preset browser, coupling editor, and settings screen adapt to bottom sheets on iOS:
- Sheet appearance: slide up from bottom over 300ms, spring easing
- Background overlay: `rgba(0,0,0,0.55)`, tappable to dismiss
- Sheet handle bar: 32pt wide × 4pt tall, `rgba(255,255,255,0.25)`, 8pt from top of sheet, centered
- Sheet background: `#0E0E10` with 20pt corner radius on top two corners only
- Maximum height: 85% of screen height
- Minimum height: 50% of screen height (then scrollable)

**Sheets used for:**
- Preset Browser / Ecosystem Atlas: bottom sheet, full searchable list
- Coupling Route Editor: bottom sheet with coupling matrix view
- Engine Swap: bottom sheet with engine picker (replaces dropdown on desktop)
- Settings: bottom sheet

### Porthole as Diving Interface (iOS-specific treatment)

On iPad, the engine slots enlarge. In the full-screen AUv3 view:
- Single engine view: one porthole fills 80% of the screen width, 60% of height
- Porthole border treatment becomes heavier: 2px solid `rgba(255,255,255,0.14)` + outer glow `rgba(engine_accent, 0.15)` at 8pt blur radius
- Vignette deepens: inner shadow opacity increases to 0.55 (from 0.35 desktop)
- Header creature silhouette scales to 28pt diameter
- Knob minimum size on iPad: 64pt medium, 96pt large only — no 40pt knobs in full AUv3 view

**iPhone AUv3 (compact):**
- Limited to 2-knob strip + macro faders + transport
- Collapsed indicators (24px) replace full engine slots
- Tap a collapsed indicator → modal sheet with single engine full view
- No multi-engine parallel view on iPhone

### Gesture Enhancements (iOS only)

| Gesture | Target | Action |
|---------|--------|--------|
| Tap knob | Any knob | Select, shows value pill |
| Drag knob | Any knob | Sets value (linear vertical drag, 150pt = full range) |
| Two-finger drag knob | Any knob | Fine adjust (1/5 sensitivity) |
| Double-tap knob | Any knob | Reset to default |
| Long-press knob | Any knob | Context menu: "Copy value", "Set to default", "MIDI learn" |
| Pinch engine slot | Engine slot | Expand/collapse slot |
| Swipe-right on preset row | Preset browser | Add to favorites |
| Swipe-left on preset row | Preset browser | Delete from user presets (factory presets: shows "protected") |
| Two-finger swipe left on full view | Main view | Switch to next engine slot |

---

## Appendix A: Creature Silhouette Path Guidelines

Each engine requires one SVG-compatible Bezier path for its creature silhouette. The path must:

1. Fit within a 1.0 unit radius circle centered at (0,0)
2. Use at most 24 Bezier segments (keeping the icon readable at 12px center well)
3. Represent the engine's mythological creature character, simplified to recognizable silhouette
4. Have one "animation variant" path — same number of anchor points, slightly different control points — used as the frame 2 morph target on note-on

Examples:
- ODDFELIX (neon tetra): torpedo fish body, distinct lateral stripe implied by a path indentation
- ODDOSCAR (axolotl): salamander body, external gill branches visible as small bumps from head
- ORGANON (bioluminescent creature): ctenophore / comb jelly — vertical oval with symmetric branching
- ORACLE: deep-sea anglerfish — rotund body, large jaw, bioluminescent lure on dorsal fin
- OBRIX (reef system): coral polyp — radial symmetry, 8 tentacle arms from center node

Path delivery format: JSON array of `{type: "M"|"C"|"L"|"Z", args: [...]}` per SVG path command. Stored in `Resources/CreaturePaths/{engineId}.json`.

---

## Appendix B: Advice Zone Default Configurations

| Parameter class | Green max | Yellow max | Notes |
|-----------------|-----------|-----------|-------|
| Filter cutoff   | 0.80 | 0.95 | Upper 5% = near Nyquist aliasing |
| Resonance       | 0.65 | 0.85 | Above 85% = near self-oscillation for most engines |
| Attack time     | 1.0  | 1.0  | No stress zone — all values valid |
| Decay time      | 1.0  | 1.0  | No stress zone |
| FM amount       | 0.60 | 0.80 | Upper 20% = extreme inharmonicity |
| Pitch (fine)    | 1.0  | 1.0  | No stress zone |
| LFO rate        | 0.75 | 0.90 | Upper 10% = audio-rate aliasing territory |
| Macro knobs     | 1.0  | 1.0  | No stress zone — macros are expressive intent |
| Drive / distortion | 0.55 | 0.80 | Engine-dependent stress |
| Coupling amount | 0.70 | 0.90 | Above 90% = potential DSP instability |

Per-engine overrides stored in engine metadata. The OBRIX engine for instance sets resonance yellow zone to 0.75 (reef bleaching metaphor) and red to 0.90.

---

## Appendix C: Implementation Priority

| Priority | Component | Reason |
|----------|-----------|--------|
| P0 | Ecological Instrument Knob | Every parameter interaction depends on this |
| P0 | Engine Slot Panel | Primary layout unit |
| P1 | Tab Navigation | Required for multi-view architecture |
| P1 | Collapsed Indicator | Required for Overview tab |
| P2 | Coupling Arc | Coupling tab depends on this |
| P2 | Macro Slider (Neptune) | Macro strip already exists; tinting pass is incremental |
| P3 | Preset Browser / Ecosystem Atlas | Thumbnail generation can be deferred post-V1 |
| P3 | iOS AUv3 Adaptations | Separate build, scheduled for V1.2 iOS ship |

---

*End of XO_OX Ecological Interface — Component Specification v1.0*
*UIX Design Studio — Ulf, Issea, Xavier — XO_OX Designs — 2026-03-22*
