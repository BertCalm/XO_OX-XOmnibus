# PlaySurface Deep Audit — 2026-03-24

**Auditor**: Claude Sonnet 4.6 (agent sweep)
**Scope**: All code in `Source/UI/PlaySurface/` + `Source/UI/Mobile/MobilePlaySurface.h`
**Design reference**: `Docs/design/playsurface-design-spec.md` (v2.0, signed off 2026-03-23)
**Total code audited**: 2,030 lines (PlaySurface.h: 1,091 / ToucheExpression.h: 212 / MobilePlaySurface.h: 727)
**Spec lines**: 973 lines across design spec

---

## Executive Summary

The existing PlaySurface code is a **solid foundation with significant gaps**. The foundation (Zones 1–4, mobile surface) is architecturally correct and has several excellent implementation details. However, the design spec added on 2026-03-23 describes a substantially more capable system that is **0% implemented beyond the foundation layer**. The gap is not a reason to panic — it is a clear, 22-day implementation roadmap. This document maps every gap precisely.

**Status by surface:**

| Component | Code Exists | Spec Coverage | Gap |
|-----------|-------------|---------------|-----|
| `NoteInputZone` (Pad/Fretless/Drum) | Yes (PlaySurface.h) | Foundation only | Missing: B042/B043 Planchette, XOuija container, snap strength, MIDI output pipeline |
| `OrbitPathZone` | Yes (PlaySurface.h) | Foundation only | Not yet an ExpressionPanel XY Pad |
| `PerformanceStrip` | Yes (PlaySurface.h) | Foundation only | Spring targets wrong, no MIDI CC output |
| `PerformancePads` (FIRE/XOSEND/ECHO CUT/PANIC) | Yes (PlaySurface.h) | Foundation only | No keyUp/note-off for Z/X/C/V |
| `ToucheExpression` | Yes (ToucheExpression.h) | Complete for its scope | Good; not yet wired to MIDI CC |
| `MobilePlaySurface` | Yes (Mobile/) | Foundation only | Multi-touch capped at 2 fingers; missing haptics for scale-tone crossings |
| `XOuijaComponent` | **NOT BUILT** | Spec written | Phase 2 — 3 days |
| `MPCPadGrid` | **NOT BUILT** | Spec written | Phase 1 — 3 days |
| `SeaboardKeyboard` | **NOT BUILT** | Spec written | Phase 3 — 3 days |
| `ExpressionPanel` | **NOT BUILT** | Spec written | Phase 4 — 2 days |
| `TideController` | **NOT BUILT** | Spec written | Phase 6 — 1 day |
| `PlaySurfaceContainer` | **NOT BUILT** | Spec written | Phase 1 — part of MPCPadGrid phase |

**PlaySurface.h is not mounted in `XOlokunEditor.h`** — only `MobilePlaySurface.h` includes `PlaySurface.h`. The desktop surface is not yet visible in the plugin UI.

---

## Part 1: NoteInputZone (Pad / Fretless / Drum) — Detailed Audit

### 1.1 Pad Grid (Pad mode)

**CORRECT behaviors:**
- 4×4 grid with proper hit testing (`col = x/width * 4`, `row = kPadRows-1-y/height*4`)
- Velocity computed from Y position (`1.0 - y/height`, clamped 0.3–1.0)
- Warm memory (1.5s decay, `kWarmMemoryDur`) with white ghost ring
- Pad velocity glow decay at 30fps via `tick()`
- Scale quantization library (9 scales) with `quantizeToScale()`
- Octave offset (±3 octaves, ±36 semitones)
- Zone depth coloring (row 0=midnight/violet, row 3=sunlit/cyan) — correct and spec-aligned
- Note labels with `noteNames[]` array, octave derivation formula `note / 12 - 1`

**BUG — Pad numbering mismatch vs. MPC standard:**
The spec (section 3.2) requires MPC standard numbering: **Pad 1 = bottom-left, Pad 16 = top-right**. The current code computes:
```cpp
int pad = row * kPadCols + col;  // row 0 = top in array
```
But it also correctly inverts row for Y hit testing:
```cpp
int row = PS::kPadRows - 1 - (int)((float)e.y / b.getHeight() * PS::kPadRows);
```
This means the internal `pad` index 0 is bottom-left (correct), but the **MIDI note mapping** uses `midiNoteForPad()` which maps pad 0 → `kBaseNote (48) + 0*4 + 0 = C3`. The spec requires Pad 1 = MIDI note 37 (C#2) in drum mode, and a different chromatic mapping in pad mode. The current chromatic base of C3 (MIDI 48) does not match MPC Bank A.

**BUG — Drum mode note table wrong:**
```cpp
static const char* drumNames[] = {
    "Kick","Snare","Kick","Snare",  // row 0 — has TWO kicks and TWO snares
    "Clap","HH-C","HH-O","Acc",
    "Tom","Perc A","Perc B","Acc",
    "","","","",                     // row 3 — four unnamed pads
};
```
- Row 0 has `"Kick","Snare","Kick","Snare"` — duplicate Kick on pad 3 is wrong. MPC Bank A has `Side Stick`, `Kick B`, `Snare A`, `Snare B`.
- Row 3 is entirely empty labels — all four pads in the top row are unnamed.
- The drum labels are display-only; the underlying MIDI note is still from `midiNoteForPad()` using chromatic mapping, **not** the MPC standard drum MIDI notes from the spec (pad 1 = 37, pad 2 = 36, pad 3 = 42, etc.).

**MISSING — No velocity curve selection:**
The spec (section 3.4) defines four curves: Linear, Logarithmic, Exponential, Fixed. The current code uses only a linear Y-position-to-velocity mapping with a hard floor of 0.3. No logarithmic curve (`pow(input, 0.6)`) is implemented. **This will feel wrong to MPC users whose muscle memory is built on MPC's logarithmic curve.**

**MISSING — No pad banks (A/B/C/D):**
The spec (section 3.5) requires 4 banks. The current code has no bank concept. All 16 pads are always the same octave/note set.

**MISSING — No choke groups:**
Spec section 3.5 requires choke group support (triggering one pad silences others in the group). No implementation.

**MISSING — Hover state:**
Spec section 3.3 describes a hover state where pads brighten by 8%. The current code has no `mouseMove()` handler.

**MISSING — Hit flash:**
Spec: "A brief flash pulse (50ms, white at 20% opacity) provides the 'impact' feel." The warm memory ghost ring is present but the initial hit flash is not — the velocity fill fills immediately but there is no transient white flash.

**MISSING — Velocity heatmap mode toggle:**
The spec (section 3.3) describes a toggleable "velocity heatmap mode" that shows all 16 pads' most recent velocities simultaneously as persistent fills, decaying over 4 beats. The current code's `padVelocity` array does partially implement this, but it is always on (not a toggle) and does not sync to host tempo.

**MISSING — MPCe quad-corner support:**
Spec section 3.8 describes four corners per pad sending both the note AND a CC for the corresponding macro. No implementation.

**MISSING — XPM program correlation:**
Spec section 3.6: when an XPM drum program is loaded, pad colors shift to indicate sample type, labels show sample names, velocity layers show as dividers. No implementation (requires XPM integration).

**MISSING — Keyboard mapping for pads:**
Spec section 3.7 (Xavier): Q/W/E/R = row 3, A/S/D/F = row 2, Z/X/C/V = row 1. The current `PerformancePads` uses Z/X/C/V for FIRE/XOSEND/ECHO CUT/PANIC, which would conflict with pad keyboard mapping. These need to be separated.

### 1.2 Fretless Mode

**CORRECT behaviors:**
- Y-axis pitch mapping: `note = 24 + (int)(yNorm * 72.0f)` — C1 to C7 (correct range per spec)
- Scale quantization applied to fretless pitch
- Zone-depth ocean gradient (midnight/twilight/sunlit) correctly implemented
- Zone boundary lines at 45% and 80% height
- Scale fret lines with zone-appropriate colors
- XO Gold ring cursor with velocity arc indicator
- Velocity derived from Y position (0.35–1.0 range)
- `lastFretlessVelocity_` tracked for ring glow scaling

**BUG — Fretless is pitch-on-Y, not pitch-on-X:**
The spec (section 2.4) is definitive: **X = pitch, Y = expression**. Left = low, right = high. The current code maps **Y = pitch** (bottom = low, top = high). This is a fundamental axis swap. The fretless strip in its current form is a **vertical pitch strip**, not the 2D XOuija surface the spec describes.

The spec's XOuija is a 2D surface where you can be at (low pitch, high expression) or (high pitch, low expression) or any combination. The current strip gives no independent control of pitch vs. expression simultaneously — moving up changes both pitch and velocity together.

**BUG — Continuous pitch / stuck note on drag:**
When dragging in fretless mode, this code runs:
```cpp
if (isDown || note != lastNote)
{
    if (lastNote >= 0 && onNoteOff) onNoteOff(lastNote);
    lastNote = note;
    if (onNoteOn) onNoteOn(note, velocity);
}
```
This triggers rapid note-on/note-off pairs as the pitch quantizes during drag. For truly continuous fretless pitch, the implementation should send **pitch bend messages** (CC or MIDI pitch bend) between quantized notes, not a new note-on for every semitone boundary crossed. This produces clicks on most engines. The spec (section 2.4) says "pitch glides continuously" — that requires pitch bend, not repeated note-ons.

**MISSING — X-axis expression mapping:**
In fretless mode, the X axis is completely unused. There is no expression parameter mapped to it. The spec maps Y to expression (filter cutoff by default). The current code conflates Y = pitch AND Y = velocity, leaving no axis for independent expression control.

**MISSING — Planchette:**
The Planchette (spec section 2.2) is the central visual element of XOuija — the translucent lens showing note name, octave, frequency, and engine parameter values. Not present. The current code shows only a gold ring cursor at the current note's Y position.

**MISSING — Planchette idle drift (Blessing B042):**
B042 ratified: Lissajous idle drift at 0.3Hz/0.2Hz when no touch active. The current code shows nothing when `lastNote < 0`.

**MISSING — Bioluminescent velocity trail (B043):**
B043 ratified: the gesture trail is a first-class modulation source (ring buffer of 256 x/y/velocity/time tuples, coupleable to DSP). The current warm memory system in `NoteInputZone` is a pad-grid-only concept (tracking pad index, not XY position). There is no XY position trail for the fretless surface.

**MISSING — Ripple on note trigger:**
Spec section 2.3: concentric ripple on touch (3 rings, 300ms, 40pt max radius). Not implemented.

**MISSING — Scale snap strength:**
The spec (section 2.4) calls for configurable snap strength: 0% = fully fretless, 100% = hard quantize, 50% = magnetic pull. The current code uses binary hard-quantize (either `quantizeToScale()` returns the exact scale note, or chromatic returns unchanged). No magnetic/partial-snap behavior.

**MISSING — Engine color tinting:**
Spec: Planchette border tints to active engine's accent color. Current cursor is always XO Gold, regardless of engine.

**MISSING — Glissando rate / portamento:**
Spec section 2.4: configurable glide rate 0–500ms. No glide implemented — note-ons fire immediately.

### 1.3 Single-Touch Only (Desktop)

The current code tracks only a single `lastNote` (one integer). Multi-touch is not handled in `NoteInputZone`. The spec requires:
- iPad: up to 5 simultaneous Planchettes (polyphonic)
- The `juce::MouseInputSource` index approach described in the spec is not implemented

---

## Part 2: OrbitPathZone — Detailed Audit

### 2.1 Correct behaviors

- Free/Lock/Snap physics modes, all three working
- Boundary reflection (elastic) in Free mode
- Spring return in Snap mode (coefficient 0.08)
- Amber trail ring buffer (60 points)
- Velocity-from-delta for momentum
- Tab strip (FREE/LOCK/SNAP) with Chromatophore Amber styling
- Repaint guard: only repaints when position changed or dragging

### 2.2 Bug: Axis Labels Hardcoded

```cpp
g.drawText("CUTOFF", ...);
g.drawText("RES", ...);
```
The spec (section 5.3) describes the XY Pad as having configurable axis labels (right-click to assign parameters). The current labels are hardcoded to CUTOFF/RES and cannot be changed.

### 2.3 Bug: `onPositionChanged` Output Not Wired to MIDI/Parameters

`OrbitPathZone` calls `onPositionChanged(x, y)` with normalized values. But in `PlaySurface`, **no callback is set**. The `getNoteInput()`, `getOrbitPath()`, `getStrip()`, `getPerfPads()` accessors exist, but no wiring happens inside `PlaySurface` — callers must set these callbacks externally. Since `PlaySurface.h` is not included in `XOlokunEditor.h`, the callbacks are never set. The OrbitPath produces no audio output.

### 2.4 Missing: Lock Mode Spring on Release

In Lock mode (`PhysicsMode::Lock`), the `tick()` function does nothing on release — it just stays put. This is correct. But the spec's "XY Pad" variant of this uses the same three physics modes. The current Lock mode has no `center-detent` snap behavior mentioned for the pitch bend strip. This is fine as-is for the orbit; it becomes an issue when the orbit zone is repurposed as the spec's XY Pad controller.

### 2.5 Missing: Right-Click Parameter Assignment

No `mouseRightButtonDown()` / context menu. Spec section 5.3 requires right-click to assign parameters.

---

## Part 3: PerformanceStrip — Detailed Audit

### 3.1 Correct behaviors

- DubSpace/FilterSweep/Coupling/DubSiren modes
- Gesture trail (45 points, amber)
- Spring-back when not touching
- Axis labels per mode
- 30fps tick with repaint guard

### 3.2 Bug: Spring Target for FilterSweep

```cpp
static constexpr SpringTarget springTargets[] = {
    { 0.3f, 0.2f },  // DubSpace
    { 0.3f, 0.3f },  // FilterSweep (cutoff ~1200Hz log)
    { 0.3f, 0.15f }, // Coupling
    { 0.5f, 0.5f },  // DubSiren (center)
};
```
DubSiren correctly springs to center (0.5, 0.5). But FilterSweep springs to (0.3, 0.3) — a non-neutral position. A filter sweep strip probably should spring to a low-cutoff position (0.0, 0.0) or a meaningful default, not an arbitrary (0.3, 0.3). This is a design decision but should be explicit.

### 3.3 Bug: FilterSweep Fill Not Applied

```cpp
case StripMode::FilterSweep:
    g.setColour(juce::Colour(kSurfaceCard));
    break;
```
This sets a color but then calls `g.setOpacity(0.3f)` and `g.fillRect(b)`. The FilterSweep mode has no gradient fill — it's a plain dark card at 30% opacity. Compare to DubSpace which correctly applies a gradient. This means FilterSweep looks visually identical to Coupling (both fall through to the same `setOpacity(0.3f) fillRect` path). FilterSweep should have its own gradient (e.g., dark-to-bright left-to-right for the cutoff axis).

### 3.4 Missing: MIDI CC Output

The `onPositionChanged(x, y)` callback fires, but like the OrbitPath, there is no CC output from the strip. The strip affects nothing in the audio engine in the current implementation.

---

## Part 4: PerformancePads — Detailed Audit

### 4.1 Correct behaviors

- FIRE/XOSEND/ECHO CUT/PANIC with correct colors
- Z/X/C/V keyboard shortcuts shown as hint text
- `handleKey(key, isDown)` for keyboard input
- Pressed visual state

### 4.2 Bug: No keyUp/Note-Off for XOSEND and ECHO CUT

```cpp
bool keyPressed(const juce::KeyPress& key) override
{
    return perfPads.handleKey(key, true);  // Always isDown=true
}
```
`PlaySurface::keyPressed()` always passes `isDown=true`. There is no `keyStateChanged()` override in `PlaySurface` for key releases. XOSEND and ECHO CUT take a `bool held` parameter — they are supposed to behave differently when held vs. momentarily pressed. From the keyboard, they can never be released via keyState because no `keyStateChanged()` is implemented. This means XOSEND/ECHO CUT held behavior is inaccessible from keyboard.

### 4.3 Minor: `mouseUp` Does Not Validate Touch Remains Inside

```cpp
void mouseUp(const juce::MouseEvent& e) override
{
    int pad = padFromY(e.y);
    if (pad < 0 || pad > 3) return;  // This check fires the "up" event for pad under cursor
```
If the user presses pad 0 (FIRE), drags out of the component, and releases, the padFromY may return a different pad. The released pad does not match the pressed pad, leaving the wrong pad in `padStates[i] = true`. This can cause visually stuck pads.

### 4.4 Missing: Visual Keyboard Binding on Each Pad

Spec section 3.7 (Xavier): "Visual indication of keyboard binding on each pad (small key letter in bottom-right corner, 30% opacity)." The current code shows keyboard hints for Z/X/C/V on the performance pads, which is correct for those. But the spec's 4×4 MPC pad grid (not yet built) should show Q/W/E/R, A/S/D/F, Z/X/C/V keyboard bindings per pad.

---

## Part 5: ToucheExpression — Detailed Audit

### 5.1 Correct behaviors

- Ondes Martenot-inspired pressure/expression zone
- 4 modes: Intensity/Timbral/Spatial/Expression
- Pressure (Y) and lateral (X) tracking
- Mode-appropriate accent colors
- XO Gold crosshair dot + vertical line indicator
- `onExpressionChanged`, `onTouchBegin`, `onTouchEnd` callbacks
- `setSmoothingMs()` API exists (though smoothing is NOT yet applied to the output — see bug below)

### 5.2 Bug: Smoothing Declared but Not Applied

```cpp
void setSmoothingMs(float ms)
{
    smoothMs = std::max(1.0f, ms);
}
```
`smoothMs` is stored but never used. `updateFromMouse()` directly writes `currentState.lateral` and `currentState.pressure` from raw mouse coordinates without any smoothing. The spec mentions "smoothing is per-axis to prevent zipper noise." This will cause audible zipper noise when connected to audio parameters. A simple one-pole smoother (matching the fleet-standard 5ms `ParameterSmoother.h`) should be applied here.

### 5.3 Bug: Thread-Safety of `getState()`

```cpp
ExpressionState getState() const { return currentState; }
```
The comment says "call from audio thread via atomic copy." But `ExpressionState` is a plain struct (not `std::atomic<>`) and is written from the GUI thread in `updateFromMouse()` and read from the audio thread. This is a data race — undefined behavior on multi-core systems. Should use `std::atomic<ExpressionState>` or a lock-free FIFO, or copy via `juce::AbstractFifo`.

### 5.4 Bug: Velocity Hardcoded to 1.0

```cpp
void mouseDown(const juce::MouseEvent& event) override
{
    currentState.velocity = 1.0f; // Could be refined with timing
```
The comment acknowledges this is a placeholder. Desktop velocity should be derived from click timing (time between initial mouseDown and first mouseDrag), as specified for the MPC pad grid. For a touch surface, it should come from `UITouch.force`.

### 5.5 Missing: Mode Actually Does Nothing

The four modes (Intensity/Timbral/Spatial/Expression) change the label and color, but the `onExpressionChanged` callback fires the same `ExpressionState` regardless of mode. There is no mode-specific processing — e.g., Intensity mode should map pressure to amplitude + filter (two parameters), not just one. The mode selection is purely cosmetic in the current implementation.

### 5.6 Missing: Not Wired to Any MIDI CC or Parameter

`ToucheExpression` is defined in its own file and never included in `XOlokunEditor.h`. It is not mounted anywhere in the desktop plugin UI. The callbacks (`onExpressionChanged`, `onTouchBegin`, `onTouchEnd`) are never connected to any parameter or MIDI CC.

---

## Part 6: MobilePlaySurface — Detailed Audit

### 6.1 Correct behaviors

- Carousel mode (iPhone) / simultaneous zones (iPad)
- 30fps timer with orbit momentum physics
- Trail system with age/force tracking
- Scale table rebuilding from Scale enum
- `NoteCallback` and `ModCallback` functional style APIs
- Gyroscope-driven orbit (SensorManager integration)
- Haptic events on PadStrike and OrbitBoundary
- `forceToVelocity()` with exponential curve `0.1 + force^2 * 0.9`

### 6.2 Bug: MobilePlaySurface Scale Table Has Incorrect Pad-Row Assignment

```cpp
int row = 3 - (i / 4);   // Flips so note 0 is at bottom-left
int col = i % 4;
int padIdx = row * 4 + col;
padNotes[padIdx] = juce::jlimit(0, 127, baseNote + intervals[i]);
```
This assigns note 0 (index i=0) to row 3 col 0, which is the **bottom-left pad** (correct, this is Pad 1 in MPC convention). But then note 1 (i=1) goes to row 3 col 1, note 2 goes to row 3 col 2, etc. Row fills left-to-right from bottom — this matches MPC layout. However, the visual rendering in `paintPadGrid()` draws row 0 at the **top** (y=0):
```cpp
float y = area.getY() + row * cellH + gap;  // row 0 = top
```
The note table assigns low notes to row 3 (bottom of the note array = padIdx 12–15 = top-right pads visually). **The note assignment is inverted relative to the visual layout.** Low notes should be at the bottom of the visual grid (row 0 visually = row 3 in the note table). This needs verification against actual output but the asymmetry is a red flag.

### 6.3 Bug: Fretless Mode Uses Y-Axis for Pitch (Same Axis Confusion as Desktop)

```cpp
float pitch = (1.0f - e.normalizedY);  // Invert so top = high
```
Same issue as desktop: mobile fretless maps Y to pitch. The spec says X = pitch, Y = expression. The mobile fretless strip should be a horizontal instrument, not vertical.

### 6.4 Bug: Fretless Sends Discrete Note-On, Not Pitch Bend

```cpp
int midiNote = static_cast<int>(std::round(midiFloat));
// ...
case TouchPhase::Moved:
{
    // Continuous pitch change (portamento) via pitch bend
    if (modCallback) modCallback(e.normalizedX, 1.0f - e.normalizedY, 1);
    break;
}
```
On `Began`, a note-on fires for the quantized MIDI note. On `Moved`, only the `modCallback` fires — no pitch bend, no note update. So if you touch at C3 and slide to D3, you hear C3 for the entire duration. The modCallback comment says "via pitch bend" but this is aspirational — the modCallback does NOT send MIDI pitch bend messages; it just calls the user-provided callback with XY floats. The pitch of the playing note never changes on drag.

### 6.5 Bug: Multi-Touch Capped at 1 Voice

`MobilePlaySurface` registers a single `noteInputZoneId` zone that covers the entire pad area. `handleNoteInput()` tracks no per-finger state — only one note is ever active at a time (`noteCallback` fires per-touch but each new touch fires `noteCallback(midiNote, 0.0f, false)` for the previous note implicitly only via `TouchPhase::Ended`). If two fingers are down simultaneously, both fire `Began` callbacks but neither fires `Ended` until each finger releases. Two notes can technically be active, but there is no tracking of which finger owns which note, so release of finger 1 may silently fail to cancel its note if finger 2 is still down. The `MobileTouchHandler` (referenced but not audited — not in the PlaySurface directory) presumably handles this, but `handlePadInput()` uses no per-source tracking.

### 6.6 Bug: Drum Mode MIDI Notes Ignore padNotes Table

```cpp
void handleDrumInput(const TouchEvent& e)
{
    // ...
    int midiNote = 36 + padIndex;  // GM drum map starting at kick
```
In drum mode, MIDI notes are hardcoded to `36 + padIndex` (GM chromatic from C2), completely ignoring the `padNotes` table built by `rebuildScaleTable()`. This bypasses any scale/root/octave settings and produces a linear chromatic note assignment rather than the spec's MPC drum map (kick=37, snare=36, etc.).

### 6.7 Missing: Haptic Feedback for Scale-Tone Crossings

Spec section 6.2: "Scale-tone crossing (XOuija): `UISelectionFeedbackGenerator` (light tick)." The mobile surface fires haptics for `PadStrike` and `OrbitBoundary` but not for scale-tone crossings during fretless play. The `HapticEngine::Event` enum presumably needs a `ScaleToneCrossing` event added.

### 6.8 Missing: Two-Finger Vertical Swipe for Octave Shift

Spec section 6.2: "Two-finger vertical swipe on surface: Octave shift." Not implemented — `mouseDown` detects second finger to show orbit overlay (`e.source.getIndex() == 1`), but there is no swipe gesture for octave shifting.

### 6.9 Missing: Pinch to Zoom Octave Range (Seaboard)

Spec section 4.4: two-finger pinch on the Seaboard to zoom octave range. The Seaboard surface doesn't exist yet in code, but the gesture infrastructure in `MobilePlaySurface` doesn't handle pinch at all.

---

## Part 7: MIDI Output Pipeline — Critical Gap

**This is the most critical architectural gap.** The spec (section 8.3) describes a clean MIDI pipeline:

```cpp
// In each surface's note trigger:
auto msg = juce::MidiMessage::noteOn(channel, note, velocity);
midiCollector.addMessageToQueue(msg);

// In processBlock():
midiCollector.removeNextBlockOfMessages(midiBuffer, numSamples);
```

**None of this exists.** The current implementation:
- `NoteInputZone` calls `onNoteOn(note, velocity)` callback — this is a raw `std::function<void(int, float)>`
- `OrbitPathZone` calls `onPositionChanged(x, y)` callback
- `PerformanceStrip` calls `onPositionChanged(x, y)` callback
- `PerformancePads` calls `onFire()`, `onXoSend(bool)`, `onEchoCut(bool)`, `onPanic()`
- `ToucheExpression` calls `onExpressionChanged(state)`, `onTouchBegin()`, `onTouchEnd()`

All of these callbacks fire on the **GUI thread**. None of them produce `juce::MidiMessage` objects. None of them use a `MidiMessageCollector` or any lock-free queue. **The play surface currently has no mechanism to deliver notes to the audio engine.**

The `PlaySurface` class does not hold a reference to a `MidiMessageCollector`, `AudioProcessor`, `AudioProcessorValueTreeState`, or any audio-thread-accessible object. There is no wiring between the play surface and sound production.

**In practice**: A user pressing a pad in the current implementation triggers a visual glow but produces **no sound**.

---

## Part 8: Scale Quantization — Correctness Audit

### 8.1 `quantizeToScale()` — Correct but Suboptimal

```cpp
int quantizeToScale(int note) const
{
    if (currentScale == 0) return note; // Chromatic
    auto& intervals = scales[(size_t)currentScale].intervals;
    int best = note;
    int bestDist = 999;
    for (int octSearch = -1; octSearch <= 1; ++octSearch)
    {
        for (int interval : intervals)
        {
            int candidate = rootKey + interval + ((note / 12) + octSearch) * 12;
            int dist = std::abs(candidate - note);
            if (dist < bestDist) { bestDist = dist; best = candidate; }
        }
    }
    return juce::jlimit(0, 127, best);
}
```

This correctly finds the nearest scale tone. However:
- **Integer division bug**: `note / 12` is integer division. For note=23 (B1), `23/12 = 1`. For rootKey=0 and interval=0: candidate = 0 + 0 + (1)*12 = 12. That's C1. But the nearest C to B1 might actually be C2 (note 24). The ±1 octSearch handles this, so it works, but the initial octave computation is biased toward the lower octave.
- **Not microtonal**: Returns `int`, not `float`. For the XOuija fretless surface, microtonality requires floating-point pitch quantization (return a float representing MIDI note + fraction, to be converted to pitch bend).

### 8.2 Missing Scales

The spec mentions the same 9 scales as the code. The MobilePlaySurface has 8 (missing "Harm Minor"). Both match the spec. However, the spec from the Seaboard section mentions "scale-locked legato" as a pitch bend mode. Neither surface currently implements fractional pitch (microtonality between scale tones).

---

## Part 9: Visual Design Compliance

### 9.1 Color Compliance

The code uses a dark surface (`#1A1A1A` / `PS::kSurfaceBg`). The spec's "Gallery Model" specifies a warm white shell (`#F8F6F3`) for the plugin frame, with dark surfaces for the play zones specifically. The play zones correctly use dark backgrounds. The `MobilePlaySurface` correctly uses Gallery White (`0xFFF8F6F3`) for its background.

**Issue**: `ToucheExpression` uses Gallery White (`0xFFF8F6F3`) as its background — correct for the Gallery Model. But `PlaySurface` and all its zones use the dark surface background. There is a visual inconsistency: `ToucheExpression` was designed for the light Gallery frame, while `PlaySurface` is fully dark. When mounted together, they will clash.

### 9.2 Font Sizes

```cpp
g.setFont(juce::Font(9.0f));  // Pad note labels
g.setFont(juce::Font(8.0f));  // Fretless octave labels
g.setFont(juce::Font(7.0f));  // Axis labels
```
The spec mandates minimum 11pt for Planchette text during fast gestures. The 7pt and 8pt labels are below readable size on Retina displays at normal plugin viewing distance. WCAG 2.5.5 minimum is cited in the code comments for touch targets (24px minimum), but font sizes are well below accessible minimums.

### 9.3 Warm Memory Ghost

The warm memory implementation:
```cpp
g.drawEllipse(cx - 8, cy - 8, 16, 16, 1.5f);
```
16pt ghost ring. The spec says "warm memory ghosts (white ring fade) must be the ONLY post-hit animation." This is honored — no bouncing, no scaling, just the ghost ring. Correct.

---

## Part 10: spec vs. Code Gap Summary Table

This is the definitive map of what the spec says should exist vs. what is implemented.

### Surface Components

| Spec Component | Location in Spec | Code Status | Gap Type |
|----------------|-----------------|-------------|----------|
| `XOuijaComponent` with Planchette | §2, B042, B043 | NOT BUILT | Phase 2 (3 days) |
| Planchette idle drift (Lissajous) | §2.2 | NOT BUILT | Phase 8 (part) |
| Planchette engine-color border | §2.2 | NOT BUILT | Phase 2 |
| Bioluminescent trail (B043) | §2.3 | NOT BUILT | Phase 8 |
| Ripple on note trigger | §2.3 | NOT BUILT | Phase 2 |
| X=pitch, Y=expression on XOuija | §2.4 | **WRONG** (Y=pitch currently) | Fix in Phase 2 |
| Scale snap strength (0–100%) | §2.4 | NOT BUILT | Phase 2 |
| Glissando rate configurable | §2.4 | NOT BUILT | Phase 2 |
| `MPCPadGrid` with 16 `PadComponent` children | §3, §8.1 | NOT BUILT | Phase 1 (3 days) |
| MPC standard pad numbering (1=bottom-left) | §3.2 | **WRONG** in note mapping | Fix in Phase 1 |
| MPC standard drum MIDI notes | §3.8 | **WRONG** (uses chromatic) | Fix in Phase 1 |
| Velocity curve selection (4 curves) | §3.4 | NOT BUILT | Phase 1 |
| MPC logarithmic curve matching | §3.4 | NOT BUILT | Phase 1 |
| Pad banks A/B/C/D | §3.5 | NOT BUILT | Phase 1 |
| Choke groups | §3.5 | NOT BUILT | Phase 1 |
| Hover state (8% brightening) | §3.3 | NOT BUILT | Phase 1 |
| Hit flash (50ms white at 20%) | §3.3 | NOT BUILT | Phase 1 |
| Velocity heatmap mode (toggle, beat-synced) | §3.3 | Partial (always on, not beat-synced) | Phase 1 |
| XPM program correlation | §3.6 | NOT BUILT | Phase 5+ |
| MPCe quad-corner support | §3.8 | NOT BUILT | Phase 10 |
| `SeaboardKeyboard` with MPE | §4, §8.1 | NOT BUILT | Phase 3 (3 days) |
| Per-note pitch bend (MPE) | §4.3 | NOT BUILT | Phase 3 |
| Per-note slide CC74 | §4.3 | NOT BUILT | Phase 3 |
| Per-note pressure | §4.3 | NOT BUILT | Phase 3 |
| Glide between keys (Seaboard) | §4.4 | NOT BUILT | Phase 3 |
| Ciani gesture intent detection | §11.3 | NOT BUILT | Phase 7 |
| External MIDI keyboard visualization | §4.7 | NOT BUILT | Phase 7 |
| `ExpressionPanel` | §5, §8.1 | NOT BUILT | Phase 4 (2 days) |
| Mod Wheel (spring-return toggle, CC1) | §5.1 | NOT BUILT | Phase 4 |
| Pitch Bend Strip (spring-return, center detent) | §5.2 | NOT BUILT | Phase 4 |
| XY Pad (wrapping OrbitPathZone) | §5.3 | NOT BUILT | Phase 4 |
| Breath/Expression Display (CC read-only) | §5.4 | NOT BUILT | Phase 4 |
| Macro Strips x4 (CHAR/MOVE/COUP/SPACE) | §5.5 | NOT BUILT | Phase 4 |
| Coupling Crossfader | §5.6.1 | NOT BUILT | Phase 4 |
| `TideController` water sim | §5.6.2 | NOT BUILT | Phase 6 (1 day) |
| Engine Selector Dial | §5.6.3 | NOT BUILT | Phase 4 |
| `PlaySurfaceContainer` with tab switching | §7.2, §8.1 | NOT BUILT | Phase 1 |
| Surface state in `.xometa` preset JSON | §7.2 | NOT BUILT | Phase 5 |
| `MidiMessageCollector` MIDI pipeline | §8.3 | NOT BUILT | Phase 5 (critical) |
| APVTS parameter attachments for expression | §8.4 | NOT BUILT | Phase 4 |
| Accessibility handlers | §8.6 | Partial (title/description set) | Phase 1+ |
| High-contrast mode | §8.6 | NOT BUILT | Phase 9 |
| Reduced motion mode | §8.6 | NOT BUILT | Phase 9 |
| iPad haptics for scale-tone crossings | §6.2 | NOT BUILT | Phase 9 |
| iPad two-finger octave swipe | §6.2 | NOT BUILT | Phase 9 |
| Surface persistence in preset | §7.2 | NOT BUILT | Phase 5 |
| PlaySurface mounted in XOlokunEditor | — | NOT MOUNTED | Phase 5 |

### Bugs in Existing Code

| Bug | File | Line(s) | Severity |
|-----|------|---------|----------|
| Fretless uses Y=pitch instead of X=pitch | PlaySurface.h | 209–231 | P0 — wrong axis |
| Fretless sends note-on for each semitone (no pitch bend) | PlaySurface.h | 225–230 | P0 — audible clicks |
| Drum MIDI notes don't match MPC standard | PlaySurface.h | 294–303 | P0 — wrong notes |
| `ToucheExpression.getState()` data race (GUI→audio thread) | ToucheExpression.h | 79 | P0 — UB |
| `ToucheExpression` smoothing stored but never applied | ToucheExpression.h | 73–76, 167–184 | P1 — zipper noise |
| `ToucheExpression.velocity` hardcoded to 1.0 | ToucheExpression.h | 134 | P1 — no dynamics |
| `ToucheExpression` modes are cosmetic only (no routing) | ToucheExpression.h | 41–48 | P1 — dead params |
| `PerformancePads.mouseUp()` can release wrong pad on drag-out | PlaySurface.h | 874–882 | P1 — stuck pads |
| `keyPressed` always passes `isDown=true`, no keyUp | PlaySurface.h | 1065–1068 | P1 — XOSEND/ECHO CUT broken |
| `PerformanceStrip` FilterSweep has no gradient fill | PlaySurface.h | 701–702 | P2 — visual inconsistency |
| `NoteInputZone` drum labels have duplicate Kick, empty row 3 | PlaySurface.h | 296–304 | P2 — wrong labels |
| Mobile fretless uses Y=pitch (same as desktop) | MobilePlaySurface.h | 301–305 | P0 — wrong axis |
| Mobile fretless never changes pitch on drag | MobilePlaySurface.h | 319–323 | P0 — stuck note |
| Mobile drum mode ignores padNotes table | MobilePlaySurface.h | 347 | P1 — wrong notes |
| Mobile pad-row visual/note assignment potentially inverted | MobilePlaySurface.h | 503–509, 544–549 | P1 — verify |
| No MIDI output from any surface (entire pipeline missing) | All | All | P0 — silent |

---

## Part 11: Architectural Observations

### 11.1 The `PlaySurface.h` is Orphaned

`PlaySurface.h` is only included by `MobilePlaySurface.h` (which imports it but doesn't use most of it — it reimplements similar functionality independently). `XOlokunEditor.h` does not include `PlaySurface.h`. The desktop plugin currently has **no play surface** mounted in its UI.

This is understandable as Phase 0 (foundation code exists, mounting is Phase 5 in the spec), but it means no integration testing has been possible.

### 11.2 Two Parallel Implementations

The codebase now has two partially-implemented play surface systems:
1. `PlaySurface.h` — desktop zones (NoteInputZone, OrbitPathZone, PerformanceStrip, PerformancePads)
2. `MobilePlaySurface.h` — mobile reimplementation (mostly duplicates functionality with iOS adaptations)

The spec (Appendix C) envisions 6 new files being created:
- `XOuijaComponent.h`
- `MPCPadGrid.h`
- `SeaboardKeyboard.h`
- `ExpressionPanel.h`
- `TideController.h`
- `PlaySurfaceContainer.h`

These would then be shared between desktop and mobile, with `MobilePlaySurface` becoming a thin adapter. Currently there is no such sharing.

### 11.3 The Existing Foundation is Good

Despite the gaps, what exists is architecturally sound:
- The 30fps timer-driven animation pattern is correct
- The ring buffer trail system is correct
- The scale quantization library is correct
- The warm memory ghost ring is elegant and spec-aligned
- The ocean depth zone coloring is beautiful and correctly maps pitch register to depth
- The OrbitPath physics (bounce, spring, Lock) are well implemented
- `ToucheExpression`'s Ondes Martenot concept is inspired (just not wired up)

The foundation should NOT be rewritten. The new components (XOuija, MPCPadGrid, Seaboard) should be built as **new files** that share the existing constants (`PS::` namespace), color scheme, and scale library.

### 11.4 B043 Gesture Trail as Modulation Source — Implementation Path

Blessing B043 (Gesture Trail as First-Class Modulation Source) requires the trail ring buffer to be promoted to a replayable, freezable, coupleable DSP modulation signal. This means:

1. The `NoteInputZone` trail needs to become an XY trail (not just pad-index tracking)
2. The trail needs to be accessible to `MegaCouplingMatrix` as a coupling source
3. The trail needs a timestamp on each point (the spec says "ring buffer of 256 x/y/velocity/time tuples")

Current `warmMemory` array: `{ int pad; float age; }` — only tracks pad index, no XY position, no velocity, no time. This needs to be replaced with:
```cpp
struct GestureTrailPoint { float x, y, velocity, time; };
std::array<GestureTrailPoint, 256> gestureTrail;
```

This is in Phase 8 (Planchette + sensitivity map), estimated 2 days.

---

## Part 12: "Most Playable" Test — Current State

The spec (section 10.2) defines three 30-second tests. Current pass/fail:

### XOuija 30-Second Test
**FAIL** — for the following reasons:
1. No XOuija surface exists yet (Phase 2)
2. The existing fretless strip maps Y to pitch (wrong axis)
3. No Planchette
4. No trail
5. No MIDI output (silent)

### MPC Grid 30-Second Test
**FAIL** — for the following reasons:
1. No MPCPadGrid component exists yet (Phase 1)
2. The existing NoteInputZone pad mode has wrong drum MIDI notes
3. No velocity curve selection
4. No bank switching
5. No MIDI output (silent)

### Seaboard 30-Second Test
**FAIL** — Seaboard doesn't exist yet (Phase 3).

**No surface currently passes the 30-second test.**

---

## Part 13: Priority Implementation Queue

Based on severity and dependency order:

### P0 Fixes (Required Before Any Integration Testing)

1. **Wire MIDI output pipeline** — Add `juce::MidiMessageCollector*` to `PlaySurface`, call `addMessageToQueue()` from all note-on/off handlers. Mount `PlaySurface` in `XOlokunEditor`. Without this, nothing sounds.

2. **Fix fretless axis** — Swap X/Y in `handleFretlessTouch()`. X = pitch (left=low, right=high), Y = expression (bottom=0, top=1). This requires rearchitecting the fretless rendering too (currently a vertical strip, needs to be a 2D surface).

3. **Fix `ToucheExpression` data race** — Make `currentState` atomic or use a lock-free FIFO before connecting to any audio parameter.

4. **Fix drum MIDI notes** — Replace `midiNoteForPad()` in drum mode with the MPC standard note table from spec section 3.8.

5. **Fix mobile fretless pitch** — Same axis fix as desktop.

### P1 Fixes (Required for Correct Behavior)

6. **Add `keyStateChanged()` to `PlaySurface`** for XOSEND/ECHO CUT key-release detection.
7. **Fix `PerformancePads.mouseUp()`** pad-index tracking on drag-out.
8. **Fix `ToucheExpression` smoothing** — apply one-pole filter in `updateFromMouse()`.
9. **Implement `ToucheExpression` mode routing** — each mode sends to different parameter targets.

### Phase 1 New Builds (3 days)
10. `PlaySurfaceContainer` (tab switching, timer orchestration)
11. `MPCPadGrid` with 16 `PadComponent` children, MPC note table, velocity curves, bank switching

### Phase 2 (3 days)
12. `XOuijaComponent` with X=pitch, Y=expression, Planchette (basic), bioluminescent trail (no idle drift yet)

### Phase 3 (3 days)
13. `SeaboardKeyboard` with basic MPE (note + pitch bend)

### Phase 4 (2 days)
14. `ExpressionPanel` with all 7 controllers wired to APVTS

### Phase 5 (1 day)
15. Mount all components in `XOlokunEditor`, wire MIDI output, add surface state to `.xometa`

### Phases 6–10 (remaining 12 days)
Per the spec's Appendix D.

---

## Conclusion

The PlaySurface foundation is architecturally solid and aesthetically on-brand. The ocean depth zones, warm memory, OrbitPath physics, and `ToucheExpression` design are all excellent decisions. The biggest issues are:

1. **The MIDI pipeline does not exist** — no sound is produced by any surface
2. **The fretless axis is swapped** relative to the XOuija spec
3. **The drum note table is wrong** relative to MPC convention
4. **Five of the eight spec components are not yet built** (XOuija, MPCPadGrid, Seaboard, ExpressionPanel, TideController, PlaySurfaceContainer)
5. **The desktop PlaySurface is not mounted** in the plugin editor

The path to "most playable virtual instrument ever created" is clear and has a known timeline (~22 days of implementation). The foundation is ready. The spec is signed off. The Blessings B042 (Planchette as Autonomous Entity) and B043 (Gesture Trail as Modulation Source) are genuinely novel and will differentiate XOlokun from every other play surface on the market when implemented.

The critical first step is wiring MIDI output so that any of the existing surfaces can actually produce sound.

---

*Audit complete. 2026-03-24. All findings documented. File saved to `Docs/sweep-playsurface-deep-2026-03-24.md`.*
