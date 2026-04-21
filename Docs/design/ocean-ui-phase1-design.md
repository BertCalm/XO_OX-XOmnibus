# Ocean UI Phase 1+6 — Design Document

> **Status:** APPROVED + RAC AUDITED — 5 design decisions + 2 RAC decisions locked. 3 blockers resolved. Ready for implementation.
> **Date:** 2026-04-09
> **Scope:** OceanView refactor (buoys, freeform layout, waveform wreaths) + Ocean wave surface (master output visualization)
> **Prototype:** `Tools/ui-preview/submarine.html`

---

## 1. What Exists vs. What We Need

| Feature | Current JUCE | Prototype | Gap |
|---------|-------------|-----------|-----|
| Engine representation | `EngineOrbit` — creature sprites at fixed polar coords | Circular buoys with waveform wreaths, freeform draggable | Replace rendering, add drag, add wreath |
| FX representation | None in ocean | Hexagonal buoys (small=FX engine, large=master FX) | New component |
| Ocean background | Static cached radial gradient + depth rings | Animated waveform traces driven by audio output | Add wave surface to paint() |
| Coupling chains | `CouplingSubstrate` — Bezier curves with particles | Same + coupling knot at midpoint | Add knot (Phase 2) |
| Engine positioning | Polar coordinates from center, computed in resized() | Freeform drag-anywhere | Replace layout system |
| Audio visualization | None | Waveform wreath per engine + master output waves | Add ring buffers |
| First launch | `emptyStateLabel_` text | Animated lifesaver ring | Replace empty state |
| State machine | Orbital/ZoomIn/SplitTransform/BrowserOpen | Flat layout with floating detail overlay | Simplify states |

---

## 2. Architecture Decision: Refactor EngineOrbit, Don't Replace

**Decision:** Keep `EngineOrbit` class, refactor its `paint()` and add drag support. Do NOT create a new `BuoyComponent` class.

**Rationale:**
- `EngineOrbit` already handles engine data binding (`setEngine`, `setVoiceCount`, `setCouplingLean`)
- Its 30Hz timer drives breath animation — we'll repurpose this for wreath animation
- Mouse handlers exist for click/double-click — we add drag
- `CouplingSubstrate` already knows about orbit centres via `setCreatureCenter()`
- Less code churn = fewer bugs

**What changes in EngineOrbit:**
- `paint()`: Replace creature sprite with circle buoy + waveform wreath
- Add `mouseDown`/`mouseDrag`/`mouseUp` for freeform dragging
- Add `waveformBuffer_` ring buffer reader for real audio wreath
- Add `position_` (freeform Point<float>) replacing polar placement
- Remove `magnetOffset_` system (replaced by drag)
- Remove `InteractionState` enum (no more Orbital/ZoomIn/etc — flat layout)
- Keep breath animation, voice count, coupling lean

---

## 3. State Machine Simplification

**Current states:** Orbital, ZoomIn, SplitTransform, BrowserOpen
**New states:** `Ocean` (default), `DetailOpen`, `BrowserOpen`

| State | Layout | What's visible |
|-------|--------|---------------|
| `Ocean` | All buoys at freeform positions, macros below, keyboard at bottom | Everything |
| `DetailOpen` | Same as Ocean but floating detail overlay + backdrop | Ocean + overlay |
| `BrowserOpen` | Full-window preset browser | Browser only |

**Key change:** No more ZoomIn/SplitTransform. Double-click a buoy → floating overlay (like prototype). The ocean layout never changes — the detail panel floats on top.

---

## 4. Files to Modify

### 4.1 `Source/UI/Ocean/EngineOrbit.h` — MAJOR REFACTOR

#### Remove:
- `InteractionState` enum (Orbital/ZoomIn/SplitTransform/Minimized)
- `interactionState_` member
- `setInteractionState()` method
- `magnetOffset_`, `targetMagnetOffset_`, `dwellTicks_`, `isShimmering_`, `shimmerPhase_` (proximity system)
- `setMouseProximity()` method
- `currentScale_`, `targetScale_` (scale lerping for state transitions)
- `kZoomInSize`, `kMinimizedSize` constants
- ZoomIn glow ring drawing in paint()
- Shimmer drawing in paint()
- State-dependent label drawing logic

#### Add:
```cpp
// Freeform position (normalized 0-1 within ocean bounds)
juce::Point<float> normalizedPosition_ { 0.5f, 0.5f };

// Drag state
bool isDragging_ = false;
juce::Point<float> dragOffset_ {};

// Waveform wreath
static constexpr int kWreathSamples = 256;
std::array<float, kWreathSamples> wreathBuffer_ {};
int wreathWritePos_ = 0;
float wreathPhase_ = 0.0f;        // animation phase
int wreathHarmonics_ = 8;         // cycles around the ring
float wreathFlare_ = 0.0f;        // note-on brightness burst

// Ripple effects
struct Ripple { float progress = 0.0f; };
std::vector<Ripple> ripples_;

// Buoy type
enum class BuoyType { Engine, FxEngine, MasterFx };
BuoyType buoyType_ = BuoyType::Engine;
```

#### New paint() drawing order:
1. Water reflection ellipse (subtle, below buoy)
2. Glow halo (radial gradient, accent color)
3. **If Engine:** Circle body fill (radial gradient) → Waveform wreath ring → Inner reference ring
4. **If FX:** Hexagon body fill → Hexagon border → Inner hexagon
5. Engine icon/emoji (temporary) or creature sprite (future)
6. Name label below buoy
7. Type sublabel ("MASTER FX" / "FX ENGINE")
8. Ripple rings (expanding, fading)

#### Waveform wreath paint() pseudocode:
```cpp
// Read from ring buffer (lock-free)
const float wreathRadius = radius + 3.0f;
const float wreathAmp = 4.0f + intensity * 6.0f + wreathFlare_ * 12.0f;
const int steps = 128;

juce::Path wreathPath;
for (int i = 0; i <= steps; ++i)
{
    const float angle = (float(i) / steps) * juce::MathConstants<float>::twoPi;
    const float waveT = angle * wreathHarmonics_ + wreathPhase_;
    const float sample = wreathBuffer_[int(waveT * kWreathSamples / twoPi) % kWreathSamples];
    const float displacement = sample * wreathAmp;
    const float r = wreathRadius + displacement;
    const float px = cx + r * std::cos(angle);
    const float py = cy + r * std::sin(angle);
    if (i == 0) wreathPath.startNewSubPath(px, py);
    else        wreathPath.lineTo(px, py);
}
wreathPath.closeSubPath();

// Glow pass
g.setColour(accentColour_.withAlpha(0.1f));
g.strokePath(wreathPath, PathStrokeType(6.0f));

// Core pass
g.setColour(accentColour_.withAlpha(0.5f + intensity * 0.3f + wreathFlare_ * 0.4f));
g.strokePath(wreathPath, PathStrokeType(1.5f + intensity * 0.5f));
```

#### Drag handling:
```cpp
void mouseDown(const juce::MouseEvent& e) override
{
    isDragging_ = true;
    dragOffset_ = e.position;  // local coords
    // Don't fire onClicked yet — wait for mouseUp
}

void mouseDrag(const juce::MouseEvent& e) override
{
    if (!isDragging_) return;
    auto parentPos = e.getEventRelativeTo(getParentComponent()).position;
    auto parentBounds = getParentComponent()->getLocalBounds();
    normalizedPosition_.x = juce::jlimit(0.05f, 0.95f, parentPos.x / parentBounds.getWidth());
    normalizedPosition_.y = juce::jlimit(0.05f, 0.95f, parentPos.y / parentBounds.getHeight());
    // Parent will call resized() or reposition us
    if (onPositionChanged) onPositionChanged(slotIndex_);
}

void mouseUp(const juce::MouseEvent& e) override
{
    isDragging_ = false;
    // Detect click vs drag (small movement threshold)
    if (e.getDistanceFromDragStart() < 4)
    {
        // Single click — no action in our design (drag only)
    }
}

void mouseDoubleClick(const juce::MouseEvent&) override
{
    if (onDoubleClicked) onDoubleClicked(slotIndex_);
}
```

### 4.2 `Source/UI/Ocean/OceanBackground.h` — ADD WAVE SURFACE

#### Add members:
```cpp
// Master output ring buffer (written by processor, read by UI)
static constexpr int kWaveSamples = 512;
std::array<float, kWaveSamples> masterWaveBuffer_ {};
float waveTime_ = 0.0f;
float intensity_ = 0.0f;  // RMS-derived, 0-1
```

#### Add to paint() AFTER the cached gradient:
```cpp
void paintWaveSurface(juce::Graphics& g, float w, float h)
{
    const float primaryY = h * 0.45f;
    const float baseAmp = 1.5f + intensity_ * 14.0f;

    // Build wave points from buffer
    constexpr int points = 120;
    std::array<float, points> waveY;
    for (int i = 0; i < points; ++i)
    {
        // In real implementation: read from masterWaveBuffer_
        // For now: same sine composite as prototype
        float t = waveTime_;
        float fi = float(i);
        waveY[i] = std::sin(t * 0.8f + fi * 0.08f) * baseAmp
                 + std::sin(t * 1.3f + fi * 0.15f) * baseAmp * 0.6f
                 + std::sin(t * 2.1f + fi * 0.22f) * intensity_ * 7.0f;
    }

    // 3 depth layers (subtle background)
    for (int layer = 0; layer < 3; ++layer)
    {
        float yBase = h * (0.22f + layer * 0.18f);
        float alpha = 0.025f + intensity_ * 0.03f - layer * 0.006f;
        float scale = 0.5f - layer * 0.1f;

        juce::Path depthPath;
        for (int i = 0; i < points; ++i)
        {
            float x = (float(i) / points) * w;
            float y = yBase + waveY[i] * scale;
            if (i == 0) depthPath.startNewSubPath(x, y);
            else        depthPath.lineTo(x, y);
        }
        g.setColour(juce::Colour(60, 180, 170).withAlpha(alpha));
        g.strokePath(depthPath, juce::PathStrokeType(0.7f));
    }

    // Primary waveform trace
    juce::Path primaryPath;
    for (int i = 0; i < points; ++i)
    {
        float x = (float(i) / points) * w;
        float y = primaryY + waveY[i] * 1.2f;
        if (i == 0) primaryPath.startNewSubPath(x, y);
        else        primaryPath.lineTo(x, y);
    }

    // Glow fill underneath
    juce::Path fillPath(primaryPath);
    fillPath.lineTo(w, primaryY + 40.0f);
    fillPath.lineTo(0, primaryY + 40.0f);
    fillPath.closeSubPath();
    juce::ColourGradient glowFill(
        juce::Colour(60, 180, 170).withAlpha(0.04f + intensity_ * 0.06f),
        0, primaryY,
        juce::Colours::transparentBlack,
        0, primaryY + 40.0f, false);
    g.setGradientFill(glowFill);
    g.fillPath(fillPath);

    // Thick primary line
    g.setColour(juce::Colour(60, 180, 170).withAlpha(0.15f + intensity_ * 0.35f));
    g.strokePath(primaryPath, juce::PathStrokeType(2.0f + intensity_ * 1.5f));

    // Bright core
    g.setColour(juce::Colour(120, 220, 210).withAlpha(0.08f + intensity_ * 0.25f));
    g.strokePath(primaryPath, juce::PathStrokeType(1.0f));
}
```

#### Key decision: OceanBackground must become animated
- Currently it's `setOpaque(true)` with a cached image and no timer
- We need to add a timer OR have it repainted by OceanView's existing refresh
- **Recommendation:** Remove cached image approach. OceanBackground gets a 30Hz timer to advance `waveTime_` and repaint. The gradient is still cached; only the wave surface redraws each frame.
- **Risk:** OceanBackground is currently opaque for performance. Adding animated wave lines on top of the cached gradient is fine — draw gradient image first, then wave paths on top. The gradient image rebuild stays lazy (only on resize).

### 4.3 `Source/UI/Ocean/OceanView.h` — LAYOUT REFACTOR

#### State machine simplification:
```cpp
enum class ViewState { Ocean, DetailOpen, BrowserOpen };
// Remove: Orbital, ZoomIn, SplitTransform
```

#### Remove from resized():
- `layoutOrbital()` — replaced by `layoutOcean()`
- `layoutZoomIn()` — removed (detail is now a floating overlay)
- `layoutSplitTransform()` — removed

#### New `layoutOcean()`:
```cpp
void layoutOcean()
{
    auto area = getLocalBounds().withTrimmedBottom(kDashboardH + kWaterlineH + kStatusBarH);

    background_.setBounds(area);
    substrate_.setBounds(area);
    ambientEdge_.setBounds(area);

    // Position buoys at their freeform normalized positions
    for (int i = 0; i < 5; ++i)
    {
        if (!orbits_[i].hasEngine()) { orbits_[i].setVisible(false); continue; }
        orbits_[i].setVisible(true);
        auto pos = orbits_[i].getNormalizedPosition();
        int sz = orbits_[i].getBuoySize() + kBreathPadding * 2;
        int x = int(pos.x * area.getWidth()) - sz / 2;
        int y = int(pos.y * area.getHeight()) - sz / 2;
        orbits_[i].setBounds(x, y, sz, sz);
        substrate_.setCreatureCenter(i, orbits_[i].getCenter());
    }

    // Macros in dashboard strip (below waterline)
    // ... dashboard layout
}
```

#### New `layoutDashboard()`:
Below the waterline separator. Contains:
- Macro strip (72px height)
- Tab bar (KEYS/PAD/DRUM/XY)
- Play area (MPE keyboard or empty when right panel open)

#### Floating detail overlay:
```cpp
// New member:
std::unique_ptr<DetailOverlay> detailOverlay_;

// On double-click:
void showDetailOverlay(int slot)
{
    viewState_ = ViewState::DetailOpen;
    detailOverlay_ = std::make_unique<DetailOverlay>(/* params */);
    detailOverlay_->loadSlot(slot);
    addAndMakeVisible(*detailOverlay_);
    // Position: inset from ocean edges (like prototype)
    auto area = getLocalBounds();
    detailOverlay_->setBounds(area.reduced(50, 52).withBottom(area.getBottom() - 16));
}
```

### 4.4 `Source/Core/WaveformRingBuffer.h` — NEW FILE

> **RAC FINDING F1-F3: This section is CANCELLED.** `WaveformFifo` already exists in `XOceanusProcessor.h`. Use `processor.getWaveformFifo(slot).readLatest()` for per-engine data. Add `WaveformFifo masterOutputFifo_` to the processor for master output. Do NOT create this file. Do NOT modify `SynthEngine.h`.

Lock-free ring buffer for audio visualization:

```cpp
#pragma once
#include <array>
#include <atomic>

template <int Size = 256>
struct WaveformRingBuffer
{
    // Called from audio thread
    void write(const float* data, int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            buffer_[writePos_.load(std::memory_order_relaxed)] = data[i];
            writePos_.store((writePos_.load(std::memory_order_relaxed) + 1) % Size,
                           std::memory_order_release);
        }
    }

    // Called from UI thread
    void read(float* dest, int numSamples) const noexcept
    {
        int pos = writePos_.load(std::memory_order_acquire);
        int start = (pos - numSamples + Size) % Size;
        for (int i = 0; i < numSamples; ++i)
            dest[i] = buffer_[(start + i) % Size];
    }

    float getRMS(int numSamples = 64) const noexcept
    {
        int pos = writePos_.load(std::memory_order_acquire);
        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            float s = buffer_[(pos - 1 - i + Size) % Size];
            sum += s * s;
        }
        return std::sqrt(sum / numSamples);
    }

private:
    std::array<float, Size> buffer_ {};
    std::atomic<int> writePos_ { 0 };
};
```

### 4.5 `Source/Core/SynthEngine.h` — ADD RING BUFFER

> **RAC FINDING F2: This section is CANCELLED.** SynthEngine base class is off-limits for UI visualization. The existing `WaveformFifo` in `XOceanusProcessor` is the correct data source.

Each engine gets a waveform ring buffer:
```cpp
// In SynthEngine base class:
WaveformRingBuffer<256> uiWaveformBuffer_;

// At end of processBlock(), after audio output:
uiWaveformBuffer_.write(outputBuffer.getReadPointer(0), numSamples);
```

### 4.6 `Source/XOceanusProcessor.h` — ADD MASTER RING BUFFER

```cpp
// Master output ring buffer for ocean wave surface
WaveformRingBuffer<512> masterOutputBuffer_;

// At end of processBlock():
masterOutputBuffer_.write(buffer.getReadPointer(0), buffer.getNumSamples());
```

---

## 5. Layout Dimensions (from prototype)

```
┌─────────────────────────────────────────────┐
│  Ocean Viewport (flex: 1, min-height: 0)    │
│  ┌─ HUD nav bar (absolute, top: 12px) ──┐  │
│  │ [Engines] [< Preset >♡] [Chain] [⚙]  │  │
│  └───────────────────────────────────────┘  │
│                                             │
│     ○ buoys float here ○                    │
│         (freeform drag)        ⬡            │
│              ≈≈≈ wave surface ≈≈≈           │
│                                             │
├─────────────────────────────────────────────┤ ← waterline (6px)
│  Dashboard (height: 340px when keys shown)  │
│  ┌─ Macro strip (72px) ────────────────┐   │
│  │ [CHAR] [MOVE] [COUP] [SPACE]  VOL   │   │
│  └──────────────────────────────────────┘   │
│  ┌─ Tab bar ────────────────────────────┐   │
│  │ KEYS  PAD  DRUM  XY                  │   │
│  └──────────────────────────────────────┘   │
│  ┌─ Play area (flex: 1) ───────────────┐   │
│  │ [MPE Keyboard / empty if right open] │   │
│  └──────────────────────────────────────┘   │
├─────────────────────────────────────────────┤
│  Status bar (24px)                          │
└─────────────────────────────────────────────┘
```

When PAD/DRUM/XY is open:
- Right panel slides from right side of OCEAN (not dashboard)
- Dashboard collapses to macros + tabs only (no play area)
- Ocean viewport shrinks horizontally

---

## 6. Risk Register

| # | Risk | Impact | Mitigation |
|---|------|--------|------------|
| R1 | Removing polar layout breaks coupling substrate positioning | HIGH | `CouplingSubstrate.setCreatureCenter()` already takes arbitrary Points — no change needed |
| R2 | 30Hz OceanBackground repaint too expensive | MED | Only wave paths repaint; gradient stays cached. Profile on M1. |
| R3 | Ring buffer read races with audio thread write | HIGH | `std::atomic<int>` write position with acquire/release semantics. No locks. |
| R4 | Freeform drag positions lost on resize | MED | Store as normalized (0-1) coords, recompute pixel positions in resized() |
| R5 | Removing state machine breaks existing keyboard shortcuts | LOW | Remap shortcuts to new states. Escape closes detail overlay. |
| R6 | EngineOrbit currently 72px — buoys need to be 36px radius (72px diameter) | NONE | Already matches. |
| R7 | FX buoys need hexagon paint() but EngineOrbit draws circles | MED | Add `buoyType_` enum, branch in paint() |
| R8 | Depth-zone radius mismatch (background 0.30 vs layout 0.38) | LOW | Irrelevant — removing polar layout eliminates zone-based positioning |
| R9 | `ParameterGrid` in detail panel expects SplitTransform layout | MED | Detail panel is now a floating overlay — test that viewport scrolling still works |
| R10 | PlaySurfaceOverlay slide-up conflicts with new dashboard | HIGH | Rethink: PlaySurface becomes part of dashboard, not a separate overlay |

---

## 7. Implementation Sequence

### Step 1: WaveformRingBuffer (30 min)
- Create `Source/Core/WaveformRingBuffer.h`
- Add to `SynthEngine.h` (per-engine buffer)
- Add to `XOceanusProcessor.h` (master output buffer)
- Wire `write()` calls in `processBlock()`
- **Test:** Build, auval, verify no audio thread regression

### Step 2: OceanBackground Wave Surface (2 hrs)
- Add 30Hz timer to OceanBackground
- Implement `paintWaveSurface()` with simulated sine data first
- Wire to master ring buffer for real audio data
- Keep cached gradient image for background
- **Test:** Build, visual check, profile paint time

### Step 3: EngineOrbit Buoy Rendering (3 hrs)
- Strip out creature rendering, magnetism, shimmer, scale lerp
- Implement circle body + waveform wreath in paint()
- Wire wreath to per-engine ring buffer
- Add hexagon variant for FX buoys
- Keep breath animation, voice count, accent color
- **Test:** Build, visual check against prototype

### Step 4: Freeform Drag (1 hr)
- Add normalized position storage
- Implement mouseDown/mouseDrag/mouseUp
- Update OceanView layout to use freeform positions
- Persist positions (store in preset or PropertiesFile)
- **Test:** Drag buoys, verify coupling substrate follows

### Step 5: OceanView State Machine Simplification (2 hrs)
- Remove Orbital/ZoomIn/SplitTransform states
- Implement Ocean + DetailOpen + BrowserOpen
- Create floating detail overlay
- Remap keyboard shortcuts
- **Test:** Double-click buoy → overlay opens, Escape closes

### Step 6: Dashboard Layout (2 hrs)
- Add waterline separator component
- Move MacroSection to dashboard strip
- Add tab bar (KEYS/PAD/DRUM/XY)
- Integrate PlaySurface into dashboard (not overlay)
- **Test:** Tab switching, macro interaction

### Step 7: First Launch State (30 min)
- Replace emptyStateLabel with lifesaver ring drawing
- Click lifesaver → open engine picker
- **Test:** Fresh state with no engines

### Step 8: Ripple Effects + Visual Feedback (1 hr)
- Add ripple ring system to EngineOrbit
- Wire to MIDI note-on events
- Wreath flare on note-on
- **Test:** Play notes, verify visual response

---

## 8. Adversarial Audit Checkpoints

After EACH step above, run these checks:

### Build Gate
- [ ] `cmake --build build` succeeds with 0 errors
- [ ] `auval -v aumu Xocn XoOx` passes
- [ ] No new warnings added

### Visual Gate
- [ ] Screenshot matches prototype element being implemented
- [ ] Dark mode renders correctly
- [ ] Colors match prototype (teal `#3CB4AA`, plum accents)

### Performance Gate
- [ ] `paint()` time < 2ms per frame at 30Hz (measured with `juce::Time::getMillisecondCounterHiRes()`)
- [ ] No audio dropouts during UI interaction
- [ ] Memory stable (no growth over 60 seconds of operation)

### Regression Gate
- [ ] Existing presets still load correctly
- [ ] Engine hot-swap still works with 50ms crossfade
- [ ] Coupling routes still display and function
- [ ] MIDI Learn still works on knobs
- [ ] Undo/redo in detail panel still works

### Thread Safety Gate
- [ ] Ring buffer read/write verified with TSan (Thread Sanitizer)
- [ ] No `jassert` failures in debug build
- [ ] No `JUCE_ASSERT_MESSAGE_THREAD` violations

---

## 9. Parameter Wiring Map

Every interactive element in the prototype must connect to a real parameter or action:

| Prototype Element | JUCE Parameter/Action | Binding Method |
|---|---|---|
| Macro CHAR knob | `macro1` APVTS param | `SliderAttachment` |
| Macro MOVE knob | `macro2` APVTS param | `SliderAttachment` |
| Macro COUP knob | `macro3` APVTS param | `SliderAttachment` |
| Macro SPACE knob | `macro4` APVTS param | `SliderAttachment` |
| Volume slider | `masterVolume` APVTS param | `SliderAttachment` |
| Preset name click | Open `PresetBrowser` | Callback |
| Preset prev/next | `proc.loadPresetByIndex(±1)` | Callback |
| Favorite button | `proc.getPresetManager().toggleFavorite()` | Callback |
| Engine picker | `proc.loadEngine(slot, engineId)` | `EngineRegistry` |
| Detail panel knobs | Auto-generated from APVTS by `ParameterGrid` | `SliderAttachment` (lazy) |
| ADSR sliders | `{prefix}_attack/decay/sustain/release` | `SliderAttachment` |
| Coupling type | `cp_r{N}_type` APVTS param | `ComboBoxAttachment` |
| Coupling depth | `cp_r{N}_amount` APVTS param | `SliderAttachment` |
| Coupling direction | `MegaCouplingMatrix::setRouteDirection()` | Direct call |
| Chain creation | `MegaCouplingMatrix::addRoute()` | Direct call |
| Chain deletion | `MegaCouplingMatrix::removeRoute()` | Direct call |
| MPE keyboard | `PlaySurface` → `MidiCollector` | Already wired |
| Pad/Drum grids | `PlaySurface` pad mode | Already wired |
| XY pad | `PlaySurface` XY mode or custom | Needs mapping |
| Settings polyphony | `proc.setMaxVoices(n)` | Direct call |
| Settings MIDI channel | `proc.setMidiChannel(n)` | Direct call |

---

## 10. Design Decisions — ALL LOCKED (2026-04-09)

| # | Question | Decision | Rationale |
|---|----------|----------|-----------|
| D1 | Buoy positions | **Global, keyed by slot index** (PropertiesFile) | Simple, persistent, no preset bloat. Position sticks per-slot regardless of which engine is loaded. |
| D2 | Max engines | **Keep 4 slots** (matches EngineRegistry) | Ocean shows what's actually making sound. 77-engine fleet lives in the picker. |
| D3 | FX buoys | **Epic Chain slots only** (3 large hexagons) | Per-engine FX shown inside engine detail panel, not as separate ocean entities. |
| D4 | Right panel width | **Fixed 420px, clamped at 40%** (`min(420px, 40%)`) | Predictable pad sizes, safe at minimum window width. |
| D5 | Creatures vs icons | **Hybrid** — creature sprite inside buoy, waveform wreath outside | Most visually distinctive option, leverages existing CreatureRenderer code. |

---

## 11. RAC Audit Findings (2026-04-09)

### BLOCKERS (resolved)

| # | Finding | Resolution |
|---|---------|------------|
| F1 | `WaveformRingBuffer` duplicates existing `WaveformFifo` in `XOceanusProcessor.h` (lines 39-93) | **CANCELLED.** Do not create `WaveformRingBuffer.h`. Wire `EngineOrbit` to read from `processor.getWaveformFifo(slot).readLatest()`. |
| F2 | `SynthEngine` base class modification puts UI concerns in DSP interface, changes memory layout for 88 engines | **CANCELLED.** Do not touch `SynthEngine.h`. All visualization buffers live in the processor. |
| F3 | Proposed `write()` emits `memory_order_release` per sample — ARM performance bug (48K fences/sec) | **CANCELLED.** Using existing `WaveformFifo` which has correct fence pattern. |

### WARNINGS (addressed)

| # | Finding | Resolution |
|---|---------|------------|
| F4 | `std::vector<Ripple>` can allocate on audio thread | Use `std::array<Ripple, 8>` with ring counter |
| F5 | OceanBackground 30Hz timer adds redundant wakeup | OceanView's timer calls `background_.repaint()` — no separate timer |
| F6 | Steps 3+5 both touch `InteractionState` — must be atomic | MERGED into single Step 3 |
| F7 | `DetailOverlay` referenced but never specified | Wraps existing `EngineDetailPanel` in a floating overlay container (see revised Step 4) |
| F8 | Position persistence decision deferred to implementation | LOCKED: PropertiesFile only, keyed by slot index. Never APVTS. |
| F9 | FX buoy ownership conflicts with EpicChainSlotController/SidebarPanel | LOCKED (D7): Ocean buoys are PRIMARY. Sidebar FX tab removed. |
| F10 | NexusDisplay silently omitted from layout | LOCKED (D6): NexusDisplay removed. HUD nav bar handles preset identity. |
| F11 | `wreathHarmonics_` is static — wreath should show live audio | Wire to `WaveformFifo.readLatest()` for real engine output |
| F12 | Don't break WaveformDisplay.h / CompactEngineTile.h consumers | `WaveformFifo` is preserved unchanged. Legacy consumers unaffected. |

### New Design Decisions

| # | Decision | Detail |
|---|----------|--------|
| D6 | NexusDisplay | **REMOVED.** HUD nav bar handles preset name, prev/next, favorites. |
| D7 | FX ownership | **Ocean buoys PRIMARY.** Sidebar FX tab removed. EpicChainSlotController ownership transfers to Ocean layer. |

---

## 12. Revised Implementation Sequence (post-RAC)

### Step 1: Master Output WaveformFifo (30 min)
- Add `WaveformFifo masterOutputFifo_` to `XOceanusProcessor.h`
- Write to it after `masterFX.processBlock()` in `processBlock()`
- Expose via `getMasterWaveformFifo()` accessor
- **DO NOT** create WaveformRingBuffer.h
- **DO NOT** modify SynthEngine.h
- **Test:** Build, auval, verify no audio regression

### Step 2: OceanBackground Wave Surface (2 hrs)
- Keep cached gradient image (rebuild only on resize)
- Add `paintWaveSurface()` called from paint() after gradient
- Read from `masterOutputFifo_.readLatest()` via 10Hz push from editor timer
- OceanView's timer calls `background_.repaint()` — no separate timer on OceanBackground
- **Test:** Build, visual check, profile paint time < 2ms

### Step 3: EngineOrbit → Buoy Refactor (3 hrs) — MERGED Steps 3+5
- Remove `InteractionState` enum and all state-dependent code IN THE SAME STEP
- Remove magnetism, shimmer, scale lerp
- Replace creature sprite with circle body + waveform wreath
- Wreath reads from `processor.getWaveformFifo(slotIndex_).readLatest()` at 30Hz tick
- Add hexagon variant for FX buoys (BuoyType enum)
- Add `std::array<Ripple, 8>` (not vector) for note-on effects
- Keep breath animation, voice count, coupling lean, accent color
- Remove NexusDisplay from OceanView
- **Test:** Build, visual check against prototype, verify creature sprites render inside buoy circles

### Step 4: Floating Detail Overlay (2 hrs)
- Create `DetailOverlay` — thin wrapper around existing `EngineDetailPanel`
  - Constructor: `DetailOverlay(XOceanusProcessor& proc)`
  - Contains: backdrop (semi-transparent), rounded panel, close button
  - Child: `EngineDetailPanel` inside a viewport (preserves scroll)
  - Positioned: inset from ocean edges (top:52, left:50, right:50, bottom:16)
- OceanView state machine: Ocean → DetailOpen (overlay visible) → back via Escape/backdrop click
- Double-click buoy → `showDetailOverlay(slot)`
- **Test:** Double-click buoy, verify ParameterGrid knobs are wired to APVTS

### Step 5: Freeform Drag (1 hr)
- Add normalized position to EngineOrbit (Point<float>, 0-1 range)
- Implement mouseDown/mouseDrag/mouseUp with 4px click-vs-drag threshold
- OceanView `layoutOcean()` positions buoys from normalized coords
- Persist to PropertiesFile keyed by slot index (NEVER APVTS)
- Clamp to 5%-95% of ocean bounds
- **Test:** Drag buoys, verify CouplingSubstrate follows, restart plugin and verify positions restored

### Step 6: Dashboard Layout (2 hrs)
- Add waterline separator component (6px, teal gradient)
- Move MacroSection to dashboard strip below waterline
- Add tab bar component (KEYS/PAD/DRUM/XY)
- Integrate PlaySurface into dashboard (replaces PlaySurfaceOverlay slide-up)
- Remove PlaySurfaceOverlay (replaced by dashboard keyboard)
- Right panel for PAD/DRUM/XY slides from ocean right edge
- Dashboard collapses to macros+tabs when right panel open
- **Test:** Tab switching, macro knob APVTS binding, keyboard MIDI output

### Step 7: First Launch State (30 min)
- Replace emptyStateLabel with lifesaver ring in OceanView paint()
- Animated: bobbing, pulsing, "CLICK ME" text
- Click → open engine picker
- `firstLaunch_` flag cleared when first engine added
- **Test:** Fresh state with no engines loaded

### Step 8: Visual Feedback (1 hr)
- Ripple rings on note-on (std::array<Ripple,8>, triggered from UI timer draining MIDI note queue)
- Wreath flare: brightness burst on note-on, 6% decay per tick
- Pad/drum afterglow (CSS → JUCE: scheduled repaint with alpha decay)
- Key pressure bloom (inset glow on active key)
- **Test:** Play notes via MIDI, verify visual response, verify no audio thread allocation

---

## 13. Adversarial Audit Gates (per step)

After EACH step, run ALL of these checks:

### Build Gate
- [ ] `cmake --build build` — 0 errors
- [ ] `auval -v aumu Xocn XoOx` — PASS
- [ ] No new warnings

### Visual Gate  
- [ ] Screenshot matches prototype element
- [ ] Dark mode correct
- [ ] Colors: teal #3CB4AA, XO Gold #E9C46A

### Performance Gate
- [ ] paint() < 2ms per frame at 30Hz
- [ ] No audio dropouts during UI interaction
- [ ] Memory stable over 60 seconds

### Regression Gate
- [ ] Existing presets load correctly
- [ ] Engine hot-swap works (50ms crossfade)
- [ ] Coupling routes display and function
- [ ] MIDI Learn works on knobs
- [ ] WaveformDisplay.h consumers still work
- [ ] CompactEngineTile.h consumers still work

### Thread Safety Gate
- [ ] Ring buffer reads use acquire semantics
- [ ] No std::vector grows triggered from audio thread
- [ ] No jassert failures in debug build
- [ ] No JUCE_ASSERT_MESSAGE_THREAD violations
