# Modulation Visualization — Design Spec (#24, IL-3)

**Status:** locked 2026-05-05 (Day 4 brainstorm)
**Owner:** TBD on Day 5 dispatch
**Implementation plan:** Docs/plans/2026-05-05-modviz-implementation-plan.md
**Issue:** #24

---

## Goal

Surface the live depth of every active global mod route visually on the knobs it
affects, so the user can see at a glance which parameters are being modulated,
how deeply, and by which source — without opening the mod matrix. The indicator
is always present when a route exists (faint) and brightens during live mod
movement. Hovering a knob shows a tooltip labelling the source and current
depth percentage. Scope is limited to the currently-focused slot's
`EngineDetailPanel` and the 4 `MacroHeroStrip` macro pillars.

---

## Locked decisions

| Q | Decision | Meaning |
|---|---|---|
| Q1 | **B — Depth arc** | A thin secondary arc on the knob ring; length = mod depth, direction = sign |
| Q2 | **D — Detail panel (focused slot) + macros (always)** | Indicator on the focused slot's `EngineDetailPanel` knobs + 4 `MacroHeroStrip` macros; all other slots deferred |
| Q3 | **D — Always-on when routed + animate-on-value-change** | Faint arc when route exists; brightens during live mod movement |
| Q4 | **D — Color + label on hover** | Source-coded arc color; hover tooltip shows `"← LFO 1 · 62%"` |
| Q5 | **B — Only params with active mod route** | Discover targets by reading `ModRoutingModel` at engine load + route change; cache the list; poll only cached params at 30 Hz |

---

## Architect conditions

> **30 Hz `juce::Timer` reading `std::atomic<float>` with `memory_order_relaxed`.
> NO mutex on the audio thread. NO raw shared memory accessed from the message
> thread without atomics.**

The timer lives on the message thread (as JUCE timers always do). Every value
it reads from audio-thread state must come through a `std::atomic<float>`.

---

## Audit findings

### Audit 1 — Atomic transport for engine mod values

**What already exists:**

`GalleryKnob` (Source/UI/Gallery/GalleryKnob.h) already has `setModulation(float
amount, juce::Colour colour)` and `setBadgeRoutes(const std::vector<float>&)`.
`GalleryLookAndFeel::drawRotarySlider` already reads `"modAmount"` / `"modColour"`
properties and draws a 2.4 px secondary arc at `arcRadius = radius - 3.0f`, plus a
badge ring at `radius + 2 px`. **The rendering infrastructure is complete and
deployed.**

`EngineDetailPanel` already runs a 30 Hz `juce::Timer` that calls
`refreshModulationArcs()`, which reads the coupling matrix (not the mod routing
model) and calls `ParameterGrid::setModulationForKeywords()`. The plumbing
pattern for mod arcs is in production.

`ModRoutingModel` (Source/Future/UI/ModRouting/DragDropModRouter.h) is
message-thread-only and exposes `getRoutesCopy()` and `getRoutesForParam()` —
safe to call from the 30 Hz timer. Routes contain `sourceId`, `destParamId`, and
`depth` (bipolar, `[-1, +1]`).

`XOceanusProcessor::globalLFO1_` is a `std::atomic<float>` written by the audio
thread. There is a setter `setGlobalLFO1()` but **no public getter** for the
message thread. A one-line getter must be added:
```cpp
float readGlobalLFO1() const noexcept { return globalLFO1_.load(std::memory_order_relaxed); }
```

`MacroSystem::getValue(int macroIndex)` reads `macroParams[i]->load()` — a raw
atomic load, message-thread safe (confirmed line 371).

**What is NOT available on the message thread:**

`routeModAccum_` is audio-thread-only plain `float` array (comment at line 1256:
"written by the audio thread only"). The message-thread timer **must not read
it**. Instead, to get the live mod value for each source:

| Source | Message-thread read path |
|--------|--------------------------|
| LFO1 | `proc.readGlobalLFO1()` (new 1-line getter) |
| LFO2 | Not yet implemented — no audio→message atomic exists; LFO2 routes will show static depth arc (faint/always-on) until a `globalLFO2_` atomic is added |
| Macro (Tone/Tide/Couple/Depth) | `proc.getMacroSystem().getValue(macroIndex)` — already atomic |
| ModWheel / Aftertouch | `proc.getAPVTS().getRawParameterValue("modWheel")->load()` and `proc.getAPVTS().getRawParameterValue("aftertouch")->load()` — APVTS raw atomics |
| Velocity | Per-voice, not a global scalar; show static depth arc only |
| XY (XYX0–XYY3) | `proc.getXYX(slot)` / `proc.getXYY(slot)` — already atomic (W8B) |
| SeqStepValue / BeatPhase / LiveGate / SeqStepPitch | `slotSequencers_[slot].getLiveVelocity()` etc. — already atomic reads |

**Summary:** For LFO1 (the primary wired source), one new getter is needed.
LFO2 is unimplemented in the audio engine (TODO comment at line 2374) — its arc
will be static. All other active sources already have message-thread-safe reads.
No new atomics, no mutex, no raw shared memory.

### Audit 2 — FXParameterManifest coverage

`FXParameterManifest.h` catalogs ~180 FX chain display params for the
`EpicSlotPanel` accordion. It is irrelevant to mod viz. Mod route targets are
discovered at runtime via `ModRoutingModel::getRoutesForParam()` and
`apvts.getRawParameterValue(destParamId)` — which works for any registered APVTS
parameter regardless of whether it appears in the manifest. **Manifest coverage
does not matter for Q5=B.**

### Audit 3 — GalleryKnob geometry

`GalleryLookAndFeel::drawRotarySlider` (line 305):
```
float arcRadius = radius - 3.0f;
```
- The knob body fills the full `diameter` circle.
- The arc track + fill arc are drawn at `arcRadius = radius - 3 px`.
- The existing modulation arc (section 6b) **already draws at the same
  `arcRadius`**, overlaid as a 2.4 px stroke at 45% alpha, with a 2 px endpoint
  dot.
- The badge ring (section 6c) draws at `radius + 2 px` (outside the track).

**The arc-at-arcRadius layer is already implemented and in production for coupling
routes (via `EngineDetailPanel::refreshModulationArcs` / `ParameterGrid::
setModulationForKeywords`).** The modviz work is purely a matter of feeding mod
routing data into the existing `setModulation()` call instead of the coupling
data that is currently fed there.

**Verdict: Yes — geometry is fine. No redesign needed. The rendering layer is
already ship-quality.**

**Important collision note:** The existing `refreshModulationArcs()` feeds
coupling data to knobs via `setModulation()`. The new mod routing visualization
must coexist with coupling arcs. Strategy: **use `setBadgeRoutes()` for mod
routing arcs** (the outer badge ring at `radius + 2 px`, chain-teal by default)
OR replace the existing single-arc `setModulation()` with a multi-route
`setBadgeRoutes()` call. Given that `setBadgeRoutes()` supports multiple routes
per knob (segmented arc), it is the correct target for mod-routing visualization.
This avoids clobbering the coupling arc in `setModulation()`.

---

## Source colors (Q4 = D)

All colors from existing `AccentColors.h` / `Tokens.h` / `GalleryColors.h`. No
new tokens.

| Mod source | Color token | Hex | Semantic |
|------------|-------------|-----|----------|
| LFO1 | `XO::Tokens::Color::accent()` | `#3CB4BE` (teal) | Primary LFO |
| LFO2 | `XOceanus::AccentColors::chainBright` | `#90F2FA` (bright cyan) | Secondary LFO |
| Macro (Tone/Tide/Couple/Depth) | `XO::Tokens::Color::warning()` | `#E89B4A` (amber) | Macro sources |
| Velocity | `XO::Tokens::Color::primary()` | `#E9C46A` (XO Gold) | Performance |
| ModWheel | `XO::Tokens::Color::primary()` | `#E9C46A` (XO Gold) | Performance |
| Aftertouch | `XO::Tokens::Color::primary()` | `#E9C46A` (XO Gold) | Performance |
| XY surface | `XOceanus::AccentColors::chainAccent` | `#6CEBF4` (electric cyan) | Spatial |
| SeqStep/Beat | `XOceanus::AccentColors::chainPrimary` | `#2CC0C8` (teal-blue) | Sequencer |
| MIDI CC | `juce::Colour(0xFF9B7FD4)` | #9B7FD4 (purple) | External MIDI |
| Unknown/other | `XOceanus::AccentColors::chainDim` | `#146068` (dim teal) | Fallback |

Tooltip label: `"← {SourceName} · {percent}%"`, e.g. `"← LFO 1 · 62%"`.
Source names from `modSourceIdToString()` in `ModSourceHandle.h`.

---

## Visual treatment (Q1 = B + Q3 = D)

### Depth arc geometry

- Drawn by the existing `GalleryLookAndFeel` section 6c badge ring mechanism via
  `GalleryKnob::setBadgeRoutes()`.
- Arc sits at `radius + 2 px` (just outside the value track).
- Stroke width: 2 px (existing badge ring spec).
- Color: source-coded (table above), from `AccentColors.h` / `Tokens.h`.

### Opacity states (Q3 = D)

| State | Alpha |
|-------|-------|
| Route exists, mod value ≈ 0 (source inactive) | 0.25f |
| Route exists, mod value moving | lerp(0.25f → 0.75f) based on `abs(liveVal * depth)` |
| Hover (any active route) | 0.85f + tooltip shown |

### Animation (30 Hz timer)

- At each 30 Hz tick, compute `liveDepth = sourceValue * route.depth` for each
  route targeting this knob.
- Pass all per-route depths in one `setBadgeRoutes()` call — the badge ring
  renders each as a segment.
- Alpha per-segment is derived from `abs(liveDepth)`: faint when source is
  near-zero, bright during active modulation.
- No separate animation easing needed — the 30 Hz polling provides natural
  smoothing. `A11y::prefersReducedMotion()` check already present in
  `EngineDetailPanel`; if reduced motion, show static depth arc (route.depth
  only, no live source value scaling).

### Hover tooltip

- When the user hovers a knob, the existing JUCE tooltip machinery fires
  (`SettableTooltipClient`). The Day 5 implementation sets tooltip text on the
  knob to include the mod source and depth.
- Format: `"{KnobLabel}\n← {Source} · {pct}%"` for each active route,
  newline-separated.
- This reuses `GalleryKnob`'s existing `SettableTooltipClient` inheritance (it
  inherits from `juce::Slider` which is a `SettableTooltipClient`).

---

## Scope (Q2 = D)

### In scope — Day 5

- `EngineDetailPanel`: all `GalleryKnob` instances inside the focused slot's
  `ParameterGrid`. Uses the existing `refreshModulationArcs()` 30 Hz timer path,
  extended to also call `setBadgeRoutes()` for mod routing routes (in addition to
  the existing coupling arc path in `setModulation()`).
- `MacroHeroStrip`: the 4 macro pillar `juce::Slider` instances. These are NOT
  `GalleryKnob`; they are plain `juce::Slider` (vertical). The badge ring is not
  drawn by the default slider LookAndFeel. **Two options for Day 5:**
  - Option A (preferred): Convert macro pillars from `juce::Slider` to a thin
    wrapper that draws its own mod arc overlay in `paint()`.
  - Option B (simpler): Draw an overlay component on top of each pillar in
    `MacroHeroStrip::paint()` — a colored depth bar on the side of the pillar.
  Day 5 implementor should choose Option B (simpler, lower risk) unless time
  permits Option A.

### Deferred to v1.1

- All-slots visualization (slots other than the focused one).
- Pre-mod destination indicator (showing where the param will move before
  committing a route).
- LFO waveform overlay on the knob face.
- Custom source colors (palette is locked to the table above; no user-definable
  colors).
- LFO2 live value animation (LFO2 audio→message atomic does not exist yet; static
  depth arc shown instead).
- Encoder-style "ring around the outside" (requires LookAndFeel geometry change —
  deferred).

---

## Performance budget (Q5 = B)

- **Timer:** 30 Hz `juce::Timer` already running in `EngineDetailPanel`. No new
  timer needed.
- **Reads per tick:** At most 32 routes (hard cap `ModRoutingModel::MaxRoutes`).
  For each active route, one `atomic<float>::load(relaxed)` for the source value.
  O(routes * knobs) lookup — with caching (see below), this is O(active_routes).
- **Route cache:** On engine load or route change, `EngineDetailPanel` (via its
  `ChangeListener` registration on `ModRoutingModel`) rebuilds a
  `std::vector<ModVizRoute>` containing pre-resolved knob pointers and source
  IDs. The 30 Hz tick only iterates this cached list.
- **Knob list:** `ParameterGrid` already indexes `GalleryKnob*` by parameter ID.
  The cache stores a direct `GalleryKnob*` pointer per route — O(1) lookup per
  tick per route.
- **Cache invalidation:** Rebuild the cache on:
  1. `ModRoutingModel::ChangeListener::changeListenerCallback()` — route
     added/removed/depth-changed.
  2. `EngineDetailPanel::loadSlot()` — engine changed.
- **No audio-thread mutation from the timer.** All writes go to knob
  `NamedValueSet` properties (message-thread-only paint state).

---

## Out of scope

- All-slots viz (requires 4× the route lookups, no UI surface to host them)
- Pre-mod indicator (showing where the param will go before routing is confirmed)
- LFO waveform overlay (resulting depth value only, not the source waveform)
- Custom source colors (palette locked to `Tokens.h` / `AccentColors.h`)
- Global parameter mod viz (Orrery cutoff, Onset level/punch/tone/grit — these
  have dedicated engine-side read paths; no knob pointer in EngineDetailPanel)
