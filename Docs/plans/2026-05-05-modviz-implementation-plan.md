# Modulation Visualization — Implementation Plan

**Design spec:** Docs/plans/2026-05-05-modviz-design.md
**Issue:** #24 (IL-3)
**Status:** ready for Day 5 dispatch
**Estimated effort:** 3–4 hours (sonnet session)
**Estimated diff size:** 200–300 lines

> **From `feedback-plan-vs-actual-diff-inflation.md`:** This estimate is derived
> from audit findings. The rendering infrastructure (`GalleryKnob::setModulation`,
> `setBadgeRoutes`, `GalleryLookAndFeel` section 6b/6c) is already complete and
> deployed. The 30 Hz timer already runs. The primary work is wiring, not
> building new rendering. Inflation risk: LOW.

---

## Surprises from audit (read before implementing)

1. **GalleryKnob already renders mod arcs.** `setModulation()` and
   `setBadgeRoutes()` are fully implemented and used by the coupling system.
   Do NOT create a `ModulationIndicator.h` widget — use `setBadgeRoutes()` on
   existing `GalleryKnob` instances.

2. **`refreshModulationArcs()` already runs at 30 Hz** in `EngineDetailPanel`.
   The mod routing visualization extends this existing method — it does not
   replace it. Coupling arcs (`setModulation()`) and mod routing arcs
   (`setBadgeRoutes()`) occupy different visual layers and can coexist.

3. **`routeModAccum_` is audio-thread-only.** Do NOT read it from the message
   thread. Read source values through the per-source atomic paths listed in the
   design spec (Audit 1 table).

4. **`globalLFO1_` has no public getter.** Add one before wiring. One line.

5. **MacroHeroStrip uses `juce::Slider`, not `GalleryKnob`.** The badge ring
   is not drawn by the default slider LookAndFeel. Use an overlay approach
   (Task 5 describes the simpler Option B).

6. **`ModRoutingModel` has a `ChangeListener` broadcaster.** Register
   `EngineDetailPanel` as a `juce::ChangeListener` to invalidate the route cache
   on route changes. The broadcaster fires on `addRoute` / `removeRoute` /
   `setRouteDepth` / `clearAllRoutes` / `fromValueTree`.

---

## Files touched

### New files
None. The rendering infrastructure is already in place.

### Existing files modified

- `Source/XOceanusProcessor.h` [MUST EXIST] — add `readGlobalLFO1()` getter
  (1 line, public, `const noexcept`)
- `Source/UI/Gallery/EngineDetailPanel.h` [MUST EXIST] — primary wiring site:
  cache struct, `ChangeListener` registration, extended `refreshModulationArcs()`,
  macro pillar overlay paint
- `Source/UI/Gallery/MacroHeroStrip.h` [MUST EXIST] — expose knob pointers or
  add a `setModVizDepths()` update method for the 4 macro pillars
- `Source/UI/Gallery/ParameterGrid.h` [MUST EXIST] — read-only: `findKnobForParam()`
  or similar lookup already exists; verify before adding

### Reference files (read-only, do NOT modify)

- `Source/UI/Gallery/GalleryKnob.h` [REFERENCE — DO NOT MODIFY] — `setModulation()`,
  `setBadgeRoutes()`, `clearBadgeRoutes()`
- `Source/UI/Gallery/GalleryLookAndFeel.h` [REFERENCE — DO NOT MODIFY] — section
  6b/6c arc rendering; badge ring at `radius + 2 px`
- `Source/Future/UI/ModRouting/DragDropModRouter.h` [REFERENCE — DO NOT MODIFY] —
  `ModRoutingModel`, `ModRoute` struct, `ChangeListener` API
- `Source/Future/UI/ModRouting/ModSourceHandle.h` [REFERENCE — DO NOT MODIFY] —
  `ModSourceId` enum, `modSourceIdToString()` for tooltip labels
- `Source/Core/MacroSystem.h` [REFERENCE — DO NOT MODIFY] — `getValue(int)` read path
- `Source/UI/AccentColors.h` [REFERENCE — DO NOT MODIFY] — color palette
- `Source/UI/Tokens.h` [REFERENCE — DO NOT MODIFY] — color palette
- `Docs/plans/2026-05-05-modviz-design.md` [REFERENCE] — design decisions

---

## Tasks (ordered)

### Task 1 — Add `readGlobalLFO1()` to XOceanusProcessor

**File:** `Source/XOceanusProcessor.h` [MUST EXIST]

After line 162 (the existing `setGlobalLFO1()` setter), add:

```cpp
// Message-thread read — safe via relaxed atomic (one-block-late jitter acceptable).
float readGlobalLFO1() const noexcept { return globalLFO1_.load(std::memory_order_relaxed); }
```

No other changes to the processor. This is a 1-line addition.

**Verify:** `grep -n "readGlobalLFO1" Source/XOceanusProcessor.h` returns the
new line. `grep -n "globalLFO1_" Source/XOceanusProcessor.h` confirms the
backing `std::atomic<float>` at line ~1264.

---

### Task 2 — Define `ModVizRoute` cache struct in EngineDetailPanel

**File:** `Source/UI/Gallery/EngineDetailPanel.h` [MUST EXIST]

Add a private struct and member near the top of the private section (after the
existing `refreshModulationArcs()` declaration):

```cpp
// ── Mod routing visualization cache ─────────────────────────────────────
// Rebuilt on engine load and on ModRoutingModel changes.
// Consumed by the 30 Hz timerCallback to update badge arcs without
// re-scanning ModRoutingModel on every tick.
struct ModVizRoute
{
    int          sourceId   = -1;
    float        depth      = 0.0f;    // bipolar, [-1, +1]
    GalleryKnob* knob       = nullptr; // non-owning, lifetime = ParameterGrid lifetime
    juce::String paramId;              // for tooltip construction
    juce::String sourceName;           // from modSourceIdToString()
};
std::vector<ModVizRoute> modVizRouteCache_;
```

Also add `bool modVizCacheDirty_ = true;` so the first `timerCallback` triggers
a rebuild.

---

### Task 3 — Register ChangeListener on ModRoutingModel

**File:** `Source/UI/Gallery/EngineDetailPanel.h` [MUST EXIST]

`EngineDetailPanel` already inherits from `juce::ChangeListener` for another
purpose. **Verify this before adding a second inheritance.**

If `EngineDetailPanel` does NOT already inherit `juce::ChangeListener`:
- Add `private juce::ChangeListener` to the inheritance list.
- Implement `changeListenerCallback(juce::ChangeBroadcaster*)`:

```cpp
void changeListenerCallback(juce::ChangeBroadcaster* source) override
{
    // ModRoutingModel changed — invalidate the mod viz route cache.
    if (source == &processor.getModRoutingModel())
        modVizCacheDirty_ = true;
    // (existing coupling-matrix change handling, if any, goes here too)
}
```

In the constructor (or `loadSlot()`), register:
```cpp
processor.getModRoutingModel().addListener(this);
```

In `~EngineDetailPanel()`, remove:
```cpp
processor.getModRoutingModel().removeListener(this);
```

**STOP gate:** If `EngineDetailPanel` already inherits `juce::ChangeListener` and
already has a `changeListenerCallback`, extend the existing method rather than
adding a second one.

---

### Task 4 — Implement `rebuildModVizCache()`

**File:** `Source/UI/Gallery/EngineDetailPanel.h` [MUST EXIST]

Add a private method called from `timerCallback` when `modVizCacheDirty_` is
true:

```cpp
void rebuildModVizCache()
{
    modVizCacheDirty_ = false;
    modVizRouteCache_.clear();

    if (activeSlot_ < 0)
        return;

    auto* viewed = viewport.getViewedComponent();
    auto* grid   = dynamic_cast<ParameterGrid*>(viewed);
    if (!grid)
        return;

    const auto routes = processor.getModRoutingModel().getRoutesCopy();
    for (const auto& r : routes)
    {
        // Resolve the knob pointer for this parameter.
        // ParameterGrid::findKnobForParam() must exist — see verification note below.
        auto* knob = grid->findKnobForParam(r.destParamId);
        if (!knob)
            continue;

        ModVizRoute mvr;
        mvr.sourceId   = r.sourceId;
        mvr.depth      = r.depth;
        mvr.knob       = knob;
        mvr.paramId    = r.destParamId;
        mvr.sourceName = xoceanus::modSourceIdToString(
            static_cast<xoceanus::ModSourceId>(r.sourceId));
        modVizRouteCache_.push_back(mvr);
    }
}
```

**Verify `ParameterGrid::findKnobForParam` exists before use.** If it does not
exist, add it to `ParameterGrid.h`:

```cpp
// Returns the GalleryKnob* for a given APVTS parameter ID, or nullptr.
// Call from rebuildModVizCache() only (message thread, non-realtime path).
GalleryKnob* findKnobForParam(const juce::String& paramId) const
{
    for (const auto& lk : liveKnobs_)
        if (lk && lk->paramId == paramId)
            return lk->knob;
    return nullptr;
}
```

**Inflation warning:** If `ParameterGrid`'s internal list structure is not as
described above, stop and read `ParameterGrid.h` before proceeding. The actual
field names may differ. Do not guess.

---

### Task 5 — Implement source-value read helpers

**File:** `Source/UI/Gallery/EngineDetailPanel.h` [MUST EXIST]

Add a private method that reads the live source value for a given `ModSourceId`:

```cpp
float readModSourceValue(int sourceId, int slotIndex) const noexcept
{
    using Id = xoceanus::ModSourceId;
    switch (static_cast<Id>(sourceId))
    {
        case Id::LFO1:
            return processor.readGlobalLFO1(); // Task 1 addition

        case Id::MacroTone:   return processor.getMacroSystem().getValue(0);
        case Id::MacroTide:   return processor.getMacroSystem().getValue(1);
        case Id::MacroCouple: return processor.getMacroSystem().getValue(2);
        case Id::MacroDepth:  return processor.getMacroSystem().getValue(3);

        case Id::ModWheel:
            if (auto* p = processor.getAPVTS().getRawParameterValue("modWheel"))
                return p->load(std::memory_order_relaxed);
            return 0.0f;

        case Id::Aftertouch:
            if (auto* p = processor.getAPVTS().getRawParameterValue("aftertouch"))
                return p->load(std::memory_order_relaxed);
            return 0.0f;

        // XY surface — W8B atomics
        case Id::XYX0: return processor.getXYX(0) * 2.0f - 1.0f;
        case Id::XYX1: return processor.getXYX(1) * 2.0f - 1.0f;
        case Id::XYX2: return processor.getXYX(2) * 2.0f - 1.0f;
        case Id::XYX3: return processor.getXYX(3) * 2.0f - 1.0f;
        case Id::XYY0: return processor.getXYY(0) * 2.0f - 1.0f;
        case Id::XYY1: return processor.getXYY(1) * 2.0f - 1.0f;
        case Id::XYY2: return processor.getXYY(2) * 2.0f - 1.0f;
        case Id::XYY3: return processor.getXYY(3) * 2.0f - 1.0f;

        // LFO2 not yet wired — show static depth arc
        case Id::LFO2:
        // Velocity is per-voice — no global scalar available on message thread
        case Id::Velocity:
        default:
            return 1.0f; // return 1.0 so liveDepth = depth (static arc shows full configured depth)
    }
}
```

**Note on `modWheel` / `aftertouch` parameter IDs:** verify that these are the
actual APVTS IDs registered in `XOceanusProcessor.cpp`'s `createParameterLayout`.
If different, update accordingly. Use `grep "modWheel\|aftertouch"
Source/XOceanusProcessor.cpp` to confirm.

---

### Task 6 — Extend `refreshModulationArcs()` to update badge routes

**File:** `Source/UI/Gallery/EngineDetailPanel.h` [MUST EXIST]

Extend the existing `refreshModulationArcs()` method. Add a call to rebuild the
cache if dirty, then update badge routes for each cached knob:

```cpp
void refreshModulationArcs()
{
    // --- EXISTING coupling arc logic (do NOT remove) ---
    // ... (keep all existing code that calls setModulation() for coupling routes) ...

    // --- NEW: mod routing badge arcs ---
    if (modVizCacheDirty_)
        rebuildModVizCache();

    // Group routes by knob pointer (one knob may have multiple routes).
    juce::HashMap<GalleryKnob*, std::vector<float>> knobToDepths;
    juce::HashMap<GalleryKnob*, juce::String>        knobToTooltip;

    for (const auto& mvr : modVizRouteCache_)
    {
        if (!mvr.knob) continue;
        const float srcVal   = readModSourceValue(mvr.sourceId, activeSlot_);
        const float liveDpth = srcVal * mvr.depth;

        knobToDepths[mvr.knob].push_back(liveDpth);

        // Build tooltip suffix: "← LFO 1 · 62%"
        const int pct = juce::roundToInt(std::abs(liveDpth) * 100.0f);
        juce::String entry = juce::String(u8"← ") + mvr.sourceName
                             + " \xB7 " + juce::String(pct) + "%";
        auto& tip = knobToTooltip.getReference(mvr.knob);
        if (tip.isNotEmpty()) tip += "\n";
        tip += entry;
    }

    // Apply to each knob.
    for (juce::HashMap<GalleryKnob*, std::vector<float>>::Iterator it(knobToDepths); it.next();)
    {
        auto* knob = it.getKey();
        knob->setBadgeRoutes(it.getValue());

        // Append mod viz tooltip suffix to existing knob tooltip.
        // Preserve the knob's existing tooltip (e.g. param name).
        const auto& suffix = knobToTooltip[knob];
        if (suffix.isNotEmpty())
            knob->setTooltip(knob->getName() + "\n" + suffix);
    }

    // Clear badge routes on knobs no longer in the cache (route removed).
    // ParameterGrid already has clearAllModulationArcs() — add clearAllBadgeRoutes() if missing.
    // Alternative: call clearBadgeRoutes() only on knobs removed from the cache since last tick.
    // Simplest safe approach: clear all, then re-apply from cache each tick.
    // (The setBadgeRoutes() no-op guard in GalleryKnob prevents unnecessary repaints.)
}
```

**STOP gate:** If `ParameterGrid` does not have `clearAllBadgeRoutes()`,
add it in the same pattern as `clearAllModulationArcs()`:
```cpp
void clearAllBadgeRoutes() {
    for (auto& lk : liveKnobs_)
        if (lk && lk->knob)
            lk->knob->clearBadgeRoutes();
}
```
Call `grid->clearAllBadgeRoutes()` at the top of the badge-arc section (before
re-applying), so removed routes don't leave stale arcs. The `setBadgeRoutes()`
no-op guard (`toString()` comparison) prevents repaints on unchanged knobs.

---

### Task 7 — MacroHeroStrip mod viz (macro pillar overlays)

**File:** `Source/UI/Gallery/MacroHeroStrip.h` [MUST EXIST]

MacroHeroStrip uses `juce::Slider` (not `GalleryKnob`). The badge ring does not
apply. Use Option B: draw a colored depth bar in `MacroHeroStrip::paint()`.

Add a public method to `MacroHeroStrip`:

```cpp
// Called from EngineDetailPanel::refreshModulationArcs() at 30 Hz.
// amounts[i] = live modulation depth [-1, +1] for macro pillar i (0-3).
// Pass 0.0f for unrouted pillars.
void setMacroModDepths(const std::array<float, 4>& amounts,
                       const std::array<juce::Colour, 4>& colours)
{
    bool changed = false;
    for (int i = 0; i < 4; ++i)
    {
        if (macroModDepths_[i] != amounts[i] || macroModColours_[i] != colours[i])
        {
            macroModDepths_[i]  = amounts[i];
            macroModColours_[i] = colours[i];
            changed = true;
        }
    }
    if (changed) repaint();
}
```

Add private members:
```cpp
std::array<float, 4>        macroModDepths_  = {0.0f, 0.0f, 0.0f, 0.0f};
std::array<juce::Colour, 4> macroModColours_ = {
    juce::Colour(XO::Tokens::Color::Warning),
    juce::Colour(XO::Tokens::Color::Warning),
    juce::Colour(XO::Tokens::Color::Warning),
    juce::Colour(XO::Tokens::Color::Warning)
};
```

In `MacroHeroStrip::paint()`, after painting the pillar sliders, add:
```cpp
// Mod viz overlay: colored depth bar on right edge of each pillar
for (int i = 0; i < 4; ++i)
{
    if (!pillars[i].isVisible()) continue;
    const float depth = macroModDepths_[i];
    if (std::abs(depth) < 0.005f) continue;

    auto pb = pillars[i].getBounds().toFloat();
    const float barW = 3.0f;
    const float barMaxH = pb.getHeight() * 0.8f;
    const float barH    = barMaxH * std::abs(depth);
    const float barX    = pb.getRight() - barW - 1.0f;
    const float barY    = depth > 0.0f
                          ? pb.getCentreY() - barH
                          : pb.getCentreY();

    const float alpha = 0.25f + 0.5f * std::abs(depth);
    g.setColour(macroModColours_[i].withAlpha(alpha));
    g.fillRoundedRectangle(barX, barY, barW, barH, 1.5f);
}
```

In `EngineDetailPanel::refreshModulationArcs()`, collect macro route depths and
call `macroHero.setMacroModDepths(...)`. Macro param IDs are discovered from
`MacroHeroStrip`'s existing `foundIds` list (expose them via a getter or use
`MacroSystem::getValue()` keyed by index).

---

### Task 8 — Build + auval smoke test

**File:** none — build verification only

```bash
eval "$(fnm env)" && fnm use 20
npm run build   # runs tsc + Next.js — skip if pure JUCE project
cd /path/to/cmake/build
cmake --build . --config Release 2>&1 | tail -20
```

For the JUCE plugin:
```bash
# auval at three sample rates
auval -v aumu XOcn XOox  # 44.1 kHz (default)
# Set AU host sample rate to 48000 and 96000 in Logic/auval and repeat
```

**Expected:** zero new errors, no data races under Thread Sanitizer (if enabled),
no new deprecation warnings.

---

### Task 9 — Smoke test (manual)

1. Load a preset that has at least one active global mod route (LFO1 → any
   engine parameter). If none exist, add one via the mod matrix UI.
2. Focus the engine slot that has the route.
3. **Verify:** A faint arc appears on the target knob's badge ring (outer ring,
   teal).
4. **Verify:** The arc brightens and animates in sync with the LFO waveform.
5. Hover the knob. **Verify:** Tooltip shows `"← LFO 1 · {pct}%"`.
6. Remove the route. **Verify:** Badge arc clears within one 30 Hz tick (~33 ms).
7. Add a Macro route. **Verify:** Amber arc appears on the target knob.
8. Focus a different slot. **Verify:** Arcs on the previous slot's knobs clear.
9. Check the 4 macro pillars with an active macro mod route. **Verify:** Colored
   depth bar appears on the right edge of the routed pillar.
10. Toggle reduced-motion preference. **Verify:** Arc is static (no animation).

---

## Source color resolution helper (for implementor convenience)

Add as a private static method in `EngineDetailPanel`:

```cpp
static juce::Colour modSourceColour(int sourceId) noexcept
{
    using Id = xoceanus::ModSourceId;
    switch (static_cast<Id>(sourceId))
    {
        case Id::LFO1:        return juce::Colour(XO::Tokens::Color::Accent);
        case Id::LFO2:        return XOceanus::AccentColors::chainBright;
        case Id::MacroTone:
        case Id::MacroTide:
        case Id::MacroCouple:
        case Id::MacroDepth:  return juce::Colour(XO::Tokens::Color::Warning);
        case Id::Velocity:
        case Id::ModWheel:
        case Id::Aftertouch:  return juce::Colour(XO::Tokens::Color::Primary);
        case Id::XYX0: case Id::XYX1: case Id::XYX2: case Id::XYX3:
        case Id::XYY0: case Id::XYY1: case Id::XYY2: case Id::XYY3:
                              return XOceanus::AccentColors::chainAccent;
        case Id::SeqStepValue:
        case Id::BeatPhase:
        case Id::LiveGate:
        case Id::SeqStepPitch: return XOceanus::AccentColors::chainPrimary;
        case Id::MidiCC:      return juce::Colour(0xFF9B7FD4); // purple — no existing token
        default:              return XOceanus::AccentColors::chainDim;
    }
}
```

Store in `ModVizRoute.colour` during `rebuildModVizCache()` to avoid recomputing
per tick.

---

## Out-of-scope reminders

Per design spec — do NOT implement in Day 5:
- All-slots visualization (other than focused slot)
- Pre-mod destination indicator
- LFO waveform overlay
- Custom source colors
- LFO2 live animation (no audio→message atomic; return 1.0f from
  `readModSourceValue` for static arc)

---

## Estimated diff

| Component | Lines (estimate) |
|-----------|-----------------|
| `XOceanusProcessor.h` — `readGlobalLFO1()` getter | 2 |
| `EngineDetailPanel.h` — `ModVizRoute` struct + cache member | 15 |
| `EngineDetailPanel.h` — `ChangeListener` registration | 10 |
| `EngineDetailPanel.h` — `rebuildModVizCache()` | 40 |
| `EngineDetailPanel.h` — `readModSourceValue()` | 35 |
| `EngineDetailPanel.h` — extended `refreshModulationArcs()` | 50 |
| `MacroHeroStrip.h` — `setMacroModDepths()` + paint overlay | 50 |
| `ParameterGrid.h` — `findKnobForParam()` + `clearAllBadgeRoutes()` (if missing) | 20 |
| **Total** | **~222 lines** |

Actual diff may be 10–20% higher if `ParameterGrid`'s internal structure requires
more adaptation than assumed. If the implementor discovers that `ParameterGrid`
stores knobs in a substantially different structure, they should STOP and report
before inflating further.
