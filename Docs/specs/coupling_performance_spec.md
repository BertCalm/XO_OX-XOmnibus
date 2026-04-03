# Coupling Performance System — Architecture Spec

**Status:** APPROVED (RAC Brief 2026-03-20)
**Scope:** V1.2 → V2 (phased delivery)
**Author:** RAC (Ringleader + Architect + Consultant)
**Visionary Origin:** Vision Quest 001 — Coupling as Live Performance Tool

---

## 1. Problem Statement

Coupling is currently a preset-design-time decision. Routes are stored as JSON pairs, loaded on the message thread, and fixed for the duration of playback. A performer cannot morph coupling depth, switch coupling types, or play the relationship between engines in real time.

No competitor offers performable inter-engine relationships. This is a category-defining feature.

---

## 2. Architecture: Real-Time Coupling Param Model

### 2.1 Design Principle: Overlay, Not Replacement

The existing JSON coupling persistence model stays unchanged. Real-time APVTS params act as a **live performance overlay** on top of preset data:

```
Preset Load → JSON coupling pairs → MegaCouplingMatrix (baseline)
                                          ↓
                            APVTS coupling params (overlay)
                                          ↓
                            Audio thread reads overlay values
                            Falls back to baseline when overlay inactive
```

Presets continue to save/load coupling as JSON. The APVTS params are ephemeral performance state unless the user explicitly "bakes" them into the preset.

### 2.2 APVTS Parameter Design (4 Route Slots)

Each of 4 performance route slots gets 4 parameters:

| Param ID | Type | Range | Default | Notes |
|----------|------|-------|---------|-------|
| `cp_r1_active` | bool | 0/1 | 0 | Route slot enabled |
| `cp_r1_type` | int | 0–13 | 0 (AmpToFilter) | CouplingType enum index |
| `cp_r1_amount` | float | -1.0–1.0 | 0.0 | Coupling depth (bipolar) |
| `cp_r1_source` | int | 0–3 | 0 | Source engine slot index |
| `cp_r1_target` | int | 0–3 | 1 | Target engine slot index |

Repeat for `cp_r2_*`, `cp_r3_*`, `cp_r4_*` → **20 new APVTS parameters total**.

**Param ID prefix rationale:** `cp_` (coupling performance) distinguishes from future `c_` (coupling preset) params. Short prefix keeps APVTS keys lean.

### 2.3 Sync: APVTS ↔ MegaCouplingMatrix

On the **audio thread** in `processBlock()`:

```cpp
// After loading preset routes (baseline):
auto baseRoutes = couplingMatrix.loadRoutes();

// Overlay performance routes:
for (int i = 0; i < 4; ++i) {
    if (cpActive[i]->load() > 0.5f) {
        auto& route = performanceRoutes[i];
        route.type = static_cast<CouplingType>(cpType[i]->load());
        route.amount = cpAmount[i]->load();
        route.sourceSlot = static_cast<int>(cpSource[i]->load());
        route.destSlot = static_cast<int>(cpTarget[i]->load());
        // Override or append to baseRoutes
    }
}
```

**Override behavior:** If a performance route targets the same source→dest pair as a preset route, the performance route's type and amount win. Otherwise, it appends.

### 2.4 Preset Interaction

| Action | Behavior |
|--------|----------|
| Load preset | JSON coupling loads into baseline. Performance overlay resets to inactive. |
| Save preset | Option: "Include performance coupling?" If yes, bake overlay into JSON pairs. |
| Performance tweak | Modifies overlay only. Baseline unchanged. |
| "Bake" button | Copies current overlay state into baseline and saves preset. |

---

## 3. Coupling Type Switching — Crossfade Engine

### 3.1 The Problem

Abruptly switching coupling type mid-buffer can produce:
- **Audio-rate types (FM, Ring, Wavetable, Buffer):** Discontinuities, clicks, pops
- **Control-rate types (AmpToFilter, LFOToPitch, etc.):** Smoother due to SRO decimation, but still possible jumps
- **KnotTopology:** Bidirectional coupling makes switching especially risky

### 3.2 Solution: Dual-Evaluation Crossfade

When `cp_rN_type` changes:

```cpp
// Detect type change
if (currentType != targetType) {
    crossfadeActive = true;
    crossfadeProgress = 0.0f;
    crossfadeDuration = kCouplingCrossfadeMs; // 50ms default
    previousType = currentType;
}

// During crossfade: evaluate BOTH coupling types
if (crossfadeActive) {
    float fade = crossfadeProgress / crossfadeDuration;
    float prevSample = evaluateCoupling(previousType, amount, source, numSamples);
    float nextSample = evaluateCoupling(targetType, amount, source, numSamples);
    output = prevSample * (1.0f - fade) + nextSample * fade;
    crossfadeProgress += bufferDurationMs;
    if (crossfadeProgress >= crossfadeDuration) crossfadeActive = false;
}
```

**CPU impact:** ~2x coupling CPU during crossfade windows (50ms). At 15 coupling types × 4 routes, worst case is negligible. Coupling evaluation is already lightweight (control-rate decimation).

### 3.3 Type Safety Classification

| Type | Switch Safety | Crossfade Needed? |
|------|--------------|-------------------|
| AmpToFilter | Safe | No (control-rate, smooth) |
| AmpToPitch | Safe | No |
| LFOToPitch | Safe | No |
| EnvToMorph | Safe | No |
| AudioToFM | Unsafe | Yes (50ms) |
| AudioToRing | Unsafe | Yes (50ms) |
| FilterToFilter | Safe | No |
| AmpToChoke | Safe | No (ducking is inherently smooth) |
| RhythmToBlend | Safe | No |
| EnvToDecay | Safe | No |
| PitchToPitch | Safe | No |
| AudioToWavetable | Unsafe | Yes (50ms) |
| AudioToBuffer | Unsafe | Yes (100ms — buffer swap) |
| KnotTopology | Unsafe | Yes (100ms — bidirectional) |

Control-rate types (10/14): hard-switchable. Audio-rate types (4/14): need crossfade.

---

## 4. Macro System Extension

### 4.1 New MacroTarget Type: CouplingRoute

Extend `MacroTarget` to support coupling route amounts:

```cpp
struct MacroTarget {
    juce::String engineId;      // Existing
    juce::String parameterId;   // Existing
    float minValue = 0.0f;      // Existing
    float maxValue = 1.0f;      // Existing
    bool inverted = false;      // Existing

    // NEW: coupling route targeting
    bool isCouplingTarget = false;
    int couplingRouteSlot = -1;  // 0-3, maps to cp_rN_amount
};
```

**macro3 ("COUPLING")** gets a default coupling target: routes coupling depth for the active performance routes. This means turning the COUPLING macro knob sweeps coupling depth in real time.

### 4.2 MPCe Quad-Corner Mapping

The 4 macro knobs map naturally to MPCe quad-corner 3D pads:

```
         macro2 (MOVEMENT)
              ↑
              |
macro1 ←------+------→ macro3 (COUPLING)
(CHARACTER)   |
              ↓
         macro4 (SPACE)
```

Coupling depth on the right axis. X-axis sweep = character ↔ coupling. Y-axis = movement ↔ space. This is the natural "play the space between engines" interface.

---

## 5. Performance View UI

### 5.1 Component: PerformanceViewPanel

Fills the main content area (same bounds as OverviewPanel / EngineDetailPanel / ChordMachinePanel).

**Layout:**

```
┌─────────────────────────────────────────────────────┐
│  [P] PERFORMANCE VIEW                    [BAKE] [X] │
├───────────────────────┬─────────────────────────────┤
│                       │                             │
│   CouplingStripEditor │    Route Detail Panel       │
│   (node + arc viz)    │    ┌─────────────────────┐  │
│                       │    │ Route 1: OBRIX → OPAL│  │
│   ○ Engine A          │    │ Type: [AmpToFilter ▼]│  │
│    ╲                  │    │ Depth: ═══════●═══   │  │
│     ╲  arc            │    │ [MIDI Learn]         │  │
│      ╲               │    ├─────────────────────┤  │
│       ○ Engine B      │    │ Route 2: ...         │  │
│                       │    │ ...                  │  │
│   Click nodes to      │    │                      │  │
│   select routes       │    │ Route 4: [inactive]  │  │
│                       │    └─────────────────────┘  │
├───────────────────────┴─────────────────────────────┤
│  ◉ CHARACTER   ◉ MOVEMENT   ◉ COUPLING   ◉ SPACE   │
│  [macro1]      [macro2]     [macro3]     [macro4]   │
└─────────────────────────────────────────────────────┘
```

**Key elements:**
- Left: `CouplingStripEditor` (already built — node+arc visualization)
- Right: Route detail panel with type dropdown, depth slider, MIDI learn button
- Bottom: Macro knobs (duplicated from MacroSection for performance context)
- Header: "BAKE" button (save overlay to preset), close button

### 5.2 Toggle Button

Add `perfToggleBtn` to header, labeled "P", same pattern as CM and D toggles.

### 5.3 View Switching

```cpp
void showPerformanceView() {
    fadeOutCurrentPanel();
    juce::Timer::callAfterDelay(kFadeMs, [safeThis = SafePointer(this)] {
        if (safeThis) {
            safeThis->performanceViewPanel.setVisible(true);
            safeThis->fadeIn(&safeThis->performanceViewPanel);
        }
    });
}
```

---

## 6. MIDI Integration

### 6.1 Default CC Mappings (Performance Coupling)

| CC | Target | Rationale |
|----|--------|-----------|
| CC3 | `cp_r1_amount` | Undefined CC, free for coupling |
| CC9 | `cp_r2_amount` | Undefined CC |
| CC14 | `cp_r3_amount` | Undefined CC |
| CC15 | `cp_r4_amount` | Undefined CC |
| CC85 | `cp_r1_type` | Undefined CC, coupling type sweep |

All are MIDI-learnable and overridable via MIDILearnManager.

### 6.2 MIDILearnManager Extension

No structural changes needed — coupling params are standard APVTS params. MIDILearnManager already maps CC → paramId. The new `cp_*` params just become additional valid targets.

---

## 7. Future: Coupling Type Sequencer (V2)

A step sequencer that sequences **coupling type changes** over time, not notes.

**Concept:**
- 16-step pattern (sync to host tempo)
- Each step: coupling type + amount for each active route
- Performer triggers/stops the sequence
- Coupling morphs happen on beat boundaries

**Implementation approach:** New component, likely a `CouplingSequencer` class that writes to the `cp_rN_type` and `cp_rN_amount` params on a timer synced to host BPM.

Not designed here — separate spec when V1.x coupling performance is stable.

---

## 8. Future: Multi-Engine Coupling Graphs (V2)

Current: 4 engine slots, 4 performance route slots, point-to-point coupling.
Future: N-engine networks with directed graph topology.

**Requirements for graph coupling:**
- Variable route count (not fixed at 4)
- Graph visualization (CouplingStripEditor already supports arbitrary node count)
- Topology presets (save/load graph structures independently of engine presets)

Not designed here — requires fundamental re-architecture of the slot system.

---

## 9. Backward Compatibility

### 9.1 Existing Presets

| Scenario | Behavior |
|----------|----------|
| Old preset loaded in new build | JSON coupling loads normally. All `cp_*` params default to inactive. Zero behavior change. |
| New preset loaded in old build | `cp_*` params silently ignored (APVTS ignores unknown keys). JSON coupling works normally. |
| Preset with baked performance coupling | Both JSON pairs AND `cp_*` params populated. New build uses overlay. Old build uses JSON only. |

### 9.2 Version Guardian Checklist

- [ ] No existing param IDs renamed or removed
- [ ] All new params have safe defaults (inactive/zero)
- [ ] JSON coupling format unchanged
- [ ] Old presets produce identical audio output in new build
- [ ] `cp_` prefix does not collide with any existing engine prefix

---

## 10. Implementation Phases

### Phase A: Foundation (V1.2)
1. Add 20 APVTS params (`cp_r1_*` through `cp_r4_*`)
2. Wire overlay logic in processBlock
3. Extend MacroSystem to target coupling amounts
4. Add crossfade engine for type switching

### Phase B: UI (V1.2)
1. Wire CouplingStripEditor into XOceanusEditor
2. Build PerformanceViewPanel
3. Add "P" toggle button
4. Route detail controls (type dropdown, depth slider)

### Phase C: MIDI (V1.2)
1. Add default CC mappings for coupling params
2. MIDI Learn UI in Performance View
3. MPCe quad-corner documentation

### Phase D: Polish (V1.3)
1. Coupling performance presets (demo content)
2. "Bake" button functionality
3. Tutorial content
4. Community seeding

### Phase E: Sequencer (V2)
1. Coupling type sequencer design
2. Step editor UI
3. Host tempo sync

### Phase F: Graphs (V2+)
1. Multi-engine graph topology
2. Graph visualization
3. Topology presets
