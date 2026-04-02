# Morning Plan — Shaper Bus Campaign (2026-03-20)

**What was completed last night (2026-03-19):**
- ShaperEngine.h interface (locked)
- ShaperRegistry.h (4 insert + 2 bus slot management)
- XObserve shaper (6-band EQ, feliX↔Oscar, Tide LFOs, spectral coupling)
- XOxide shaper (6-mode saturation, Lorenz chaos, per-harmonic control)
- Architecture spec: `Docs/specs/shaper_bus_architecture.md`
- Full RAC brief ratified

**What remains — run these prompts in order:**

---

## Session 1: Wire + Build (Sonnet, ~1 hour)

```
Wire the Shaper Bus into XOceanusProcessor. The ShaperEngine.h interface and
ShaperRegistry.h are at Source/Core/. ObserveShaper and OxideShaper are at
Source/Shapers/Observe/ and Source/Shapers/Oxide/.

Tasks:
1. Add #include "Core/ShaperRegistry.h" to XOceanusProcessor.h
2. Add ShaperRegistry member and shaper parameter registration in createParameterLayout
3. In renderBlock: after each engine renders, call shaperRegistry.processInsert(slot, buffer)
4. After engine sum, call shaperRegistry.processBus(buffer)
5. Add Source/Shapers/ files to CMakeLists.txt target_sources
6. Register both shapers in XOceanusProcessor.cpp
7. Build and verify zero errors: cmake -B build -G Ninja && cmake --build build
8. Run /build-sentinel to confirm fleet still compiles
```

## Session 2: Remaining 10 Shapers (Opus, ~3 hours)

```
Build the remaining 10 shapers following the pattern established by ObserveShaper.h
and OxideShaper.h. Each shaper goes in Source/Shapers/{Name}/{Name}Shaper.h + .cpp.

Build order (2 at a time, build-check between each pair):

Batch A — Dynamics:
- XOpress (PRESS, press_): Compressor/limiter with sidechain.
  Coupling output: gain reduction amount.
- XOgate (GATE, gate_): Noise gate + transient designer.
  Coupling output: gate state (open/closed) + transient envelope.

Batch B — Space:
- XOltre (OLTRE, oltre_): 3D spatial panner (azimuth, elevation, distance).
  Coupling output: spatial position coordinates.
- XOverb (VERB, verb_): Algorithmic reverb with feliX↔Oscar character.
  Coupling output: reverb energy / decay tail level.

Batch C — Modulation:
- XOmen (OMEN, omen_): Stochastic modulation generator (probability-weighted).
  Coupling output: current modulation value.
- XOware (OWARE, oware_): West African rhythmic gate (Mancala-inspired patterns).
  Coupling output: gate rhythm pattern as coupling signal.

Batch D — Creative:
- XOlvido (OLVIDO, olvido_): Living tape / Basinski degradation engine.
  Coupling output: degradation level / age of signal.
- XOscillograph (OSCILLO, oscillo_): Impossible-space convolution.
  Coupling output: impulse response energy.

Batch E — Utility:
- XOtagai (OTAGAI, otagai_): Adaptive feedback matrix (self-modifying routing).
  Coupling output: feedback energy / matrix state.
- XOrganum (ORGANUM, organum_): Counterpoint harmonizer (parallel voice generation).
  Coupling output: generated harmony intervals.

For each shaper: implement ShaperEngine interface, register via REGISTER_SHAPER,
add to CMakeLists, verify build. Use existing specs in Docs/specs/utility_engine_concepts.md
and Docs/specs/xobserve_engine_spec.md / xoxide_engine_spec.md for design reference.
```

## Session 3: Cross-Platform UI (Opus, ~2 hours)

```
Build the Shaper Bus UI for macOS Gallery Model and iPad. Reference the UI research
in the git log commit message from 2026-03-19 (search for "UI/UX research").

macOS:
1. ShaperInsertStrip.h — 28pt collapsible strip at bottom of each engine panel
   - Shows: shaper type icon + name + bypass LED (collapsed)
   - Click to expand: full parameter row (120pt)
   - Empty state: dashed outline + "+" icon
2. ShaperBusStrip.h — 80pt horizontal strip below engine grid
   - Two halves: Bus Shaper 1 and Bus Shaper 2
   - Type selector, 6-8 key params, bypass toggle per slot
3. SignalFlowDiagram.h — 24pt strip showing:
   [E1] [E2] [E3] [E4] → SUM → [Bus1] → [Bus2] → Master FX
   Active slots lit in accent colors, bypassed dimmed, empty dashed
4. Global SHAPE bypass toggle in master strip header (XO Gold)

iPad:
- 56pt strips (larger knobs)
- Apple Pencil 4x precision mode
- Split-view fallback to iPhone model at <600pt
```

## Session 4: iPhone + OBRIX Pocket (Opus, ~2-3 hours)

```
Build iPhone Shaper Bus UI + OBRIX Pocket. iPhone is the PRIMARY concern.

iPhone Shaper UI:
1. Adaptive slot count: 3 editable (2 inserts + 1 bus), all 6 process audio
2. Insert strip: 44pt accent-colored below engine carousel card
   - Tap: opens ParameterDrawer (Half state) with shaper params
   - Swipe: cycle through 12 shaper types with haptic feedback
   - Long-press bypass: global bypass all shapers
3. Bus bar: 44pt persistent bottom bar (above safe area)
   - Type name + bypass + one "character" knob
4. Signal flow: 8pt vertical strip on left edge with colored dots
5. iPhone SE (320pt): icon-only strips, bus hidden behind drawer tab

OBRIX Pocket:
- Mobile-first OBRIX subset in Source/Engines/Obrix/ObrixPocket.h
- Reduced brick count: 2 sources + 2 processors + 2 modulators + 1 effect
- Touch-optimized: brick selector as swipeable cards
- FLASH gesture via shake or tap gesture
- Shares obrix_ parameter prefix (subset of desktop OBRIX params)
- First insert shaper slot pre-wired — the mobile shaper introduction
```

## Session 5: Preset Schema + MPC Bake (Sonnet, ~1 hour)

```
1. Update PresetManager.h to parse/write the shaperBus field in .xometa files
2. Backward compatibility: presets without shaperBus load with empty slots
3. Update validate_presets.py to validate shaperBus field (optional, warn if invalid type)
4. XPN export: add shaper offline rendering to the export pipeline
   - When exporting, shapers process the rendered samples before writing to disk
   - The .xpn contains pre-shaped audio — no plugin needed at playback
5. Q-Link mapping: Q1-Q2 = insert character/secondary, Q3-Q4 = bus character/mix
```

## Session 6: Full Audit Chain (Sonnet/Opus mix, ~2 hours)

```
Run in this order:
1. /build-sentinel — verify everything compiles, auval passes
2. /sweep — full codebase sweep focusing on the new Shapers/ directory
3. /architect — governance review on the shaper architecture
4. /synth-seance OBSERVE — ghost panel evaluates XObserve
5. /synth-seance OXIDE — ghost panel evaluates XOxide
6. /producers-guild — 25 specialists review the shaper system as a whole
7. /sisters — process review on the entire Shaper Bus campaign
8. /post-engine-completion-checklist × 2 (Observe + Oxide)
9. Commit + push all audit results
```

---

## Model Recommendations

| Session | Model | Effort | Why |
|---------|-------|--------|-----|
| 1 (Wire + Build) | Sonnet | Medium | Pattern-following: wiring existing code |
| 2 (10 Shapers) | Opus | High | Novel DSP design for each shaper |
| 3 (Desktop UI) | Opus | High | Novel UI components |
| 4 (iPhone + OBRIX Pocket) | Opus | High | Most critical UX work |
| 5 (Schema + MPC) | Sonnet | Medium | Mechanical wiring |
| 6 (Audits) | Mixed | Medium | Sonnet for sweeps, Opus for seances |

**Estimated total: 6 sessions, ~12 hours of execution.**
**Minimum viable: Sessions 1-2 = working shapers with no UI (use preset JSON to load).**

---

*Left by the Ringleader, 2026-03-19 late evening. Ruby is watching the board.*
