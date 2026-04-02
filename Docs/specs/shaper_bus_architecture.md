# XOceanus Shaper Bus — Architecture Specification

**Date:** 2026-03-19 | **Status:** APPROVED (RAC Brief ratified)
**Author:** Ringleader + Architect + Consultant | **Version:** 1.0

---

## 1. Overview

The Shaper Bus adds a new processing layer to XOceanus: **utility engines that shape sound rather than synthesize it.** 12 shaper types across 6 categories, deployed in 6 slots (4 per-engine inserts + 2 post-mix bus).

```
Engine 1 → [Insert Shaper] ─┐
Engine 2 → [Insert Shaper] ─┼─ Sum → [Bus 1] → [Bus 2] → MasterFXChain → Output
Engine 3 → [Insert Shaper] ─┤
Engine 4 → [Insert Shaper] ─┘
         ↕ MegaCouplingMatrix ↕
    (engines + shapers all participate)
```

### Key Differentiators
- **Per-engine insert processing** — no competitor offers this in a multi-engine synth
- **Bidirectional coupling** — shapers send spectral analysis AND receive modulation
- **MIDI-aware** — sidechain triggers, key-tracked filtering, rhythm sync
- **Cross-platform adaptive** — 6 slots on desktop/iPad, 3 editable on iPhone, baked for MPC

---

## 2. ShaperEngine Interface

```cpp
// Source/Core/ShaperEngine.h
class ShaperEngine {
    void prepare(double sampleRate, int maxBlockSize);
    void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi, int numSamples);
    void releaseResources();
    void reset();
    float getSampleForCoupling(int channel, int sampleIndex) const;
    void applyCouplingInput(CouplingType type, float amount, const float* buf, int n);
    ParameterLayout createParameterLayout();
    void attachParameters(AudioProcessorValueTreeState& apvts);
    String getShaperId() const;
    Colour getAccentColour() const;
    SilenceGate silenceGate;  // SRO: bypass when input silent
};
```

**Not a SynthEngine.** No voices, no polyphony, no MPE. Audio in → audio out with MIDI awareness.

---

## 3. The 12 Launch Shapers (V1)

| # | Name | ID | Prefix | Category | Accent | Creature |
|---|------|----|--------|----------|--------|----------|
| 1 | XObserve | OBSERVE | `obs_` | Tone | Spectral Amber `#E8A020` | Mantis Shrimp |
| 2 | XOxide | OXIDE | `oxide_` | Tone | Forge Orange `#D4500A` | Mimic Octopus |
| 3 | XOpress | PRESS | `press_` | Dynamics | Pressure Blue `#2B4570` | Sperm Whale |
| 4 | XOgate | GATE | `gate_` | Dynamics | Trigger Silver `#A8B2BD` | Pistol Shrimp |
| 5 | XOltre | OLTRE | `oltre_` | Space | Beyond Violet `#4A0E4E` | Dolphin |
| 6 | XOverb | VERB | `verb_` | Space | Cathedral Slate `#6B7B8D` | Blue Whale |
| 7 | XOmen | OMEN | `omen_` | Modulation | Premonition Indigo `#2D1B69` | Box Jellyfish |
| 8 | XOware | OWARE | `oware_` | Modulation | Akan Ember `#E85D04` | Mantis Shrimp |
| 9 | XOlvido | OLVIDO | `olvido_` | Creative | Forgotten Gold `#8B6914` | Nautilus |
| 10 | XOscillograph | OSCILLO | `oscillo_` | Creative | Waveform Grey `#36454F` | Sperm Whale |
| 11 | XOtagai | OTAGAI | `otagai_` | Utility | Mutual Green `#228B22` | Man o' War |
| 12 | XOrganum | ORGANUM | `organum_` | Utility | Polyphony Blue `#1E90FF` | Humpback Whale |

---

## 4. Slot Architecture

### ShaperRegistry
- 4 insert slots (one per engine, indexed 0-3)
- 2 bus slots (post-mix, indexed 0-1)
- Any of the 12 shaper types in any slot
- Slots can be empty (bypass) or loaded
- 50ms crossfade on shaper hot-swap (same as engine swap)
- SilenceGate per slot — zero CPU when input is silent

### Signal Flow
1. Engine N renders to its own buffer
2. If Insert[N] is loaded and not bypassed: Insert[N].processBlock(engineBuffer)
3. Engine buffers summed to mix bus
4. If Bus[0] loaded: Bus[0].processBlock(mixBuffer)
5. If Bus[1] loaded: Bus[1].processBlock(mixBuffer)
6. MasterFXChain processes final output

### Coupling Integration
- Shapers register as coupling nodes in MegaCouplingMatrix
- Insert shapers can receive coupling from ANY engine (not just their host)
- Insert shapers send coupling output (e.g., XObserve sends per-band energy)
- Bus shapers send/receive on the mixed signal
- New coupling types for shapers: `SpectralToFilter`, `EnergyToAmp`

---

## 5. Preset Schema

New optional `shaperBus` field in .xometa:

```json
{
  "schema_version": 1,
  "name": "Coral Construction",
  "engines": ["Obrix"],
  "parameters": { ... },
  "shaperBus": {
    "inserts": {
      "0": { "type": "Observe", "bypass": false, "params": { "obs_b1_gain": 3.0 } },
      "1": null,
      "2": null,
      "3": null
    },
    "bus": {
      "0": { "type": "Oxide", "bypass": false, "params": { "oxide_drive": 50 } },
      "1": null
    }
  }
}
```

**Backward compatibility:** Presets without `shaperBus` load with all slots empty. No crash, no default shapers.

---

## 6. Cross-Platform UI

### macOS (Gallery Model)
- Insert: collapsible 28pt strip at bottom of each engine panel, inside accent frame
- Bus: 80pt horizontal strip below engine grid
- Signal flow diagram: 24pt strip showing routing with colored dots
- Popup mode below 900px width

### iPad
- Insert: 56pt strip below each engine in 4-zone grid
- Bus: full-width bottom strip, 80pt, divided into 2 halves
- Apple Pencil: 4x precision on shaper knobs
- Split-view fallback: iPhone model at <600pt

### iPhone (PRIMARY)
- **3 active editable slots** (2 inserts + 1 bus) — all 6 process audio
- Insert: 44pt accent-colored strip below engine carousel card
- Bus: 44pt persistent bottom bar
- Signal flow: 8pt vertical strip on left edge
- Tap strip → ParameterDrawer (Half state)
- Swipe strip → cycle shaper types
- Long-press bypass → global bypass all
- iPhone SE: icon-only strips, bus hidden behind drawer tab

### MPC (XPN Bake)
- Shapers rendered into exported samples (offline processing)
- Q-Link mapping: Q1-Q2 = insert character/secondary, Q3-Q4 = bus character/mix
- `.xometa` stores shaper config for render pipeline

---

## 7. OBRIX Pocket (iPhone V1)

A mobile-first version of OBRIX optimized for iPhone touch interaction:
- Brick selector as swipeable cards (not dropdowns)
- 2 sources + 2 processors + 2 modulators + 1 effect (reduced from 2+3+4+3)
- FLASH gesture triggered by shake or tap gesture
- Insert shaper slot visible — the first shaper experience on mobile
- Accent: Reef Jade `#1E8B7E` (same as OBRIX)
- Shares `obrix_` parameter prefix — subset of desktop OBRIX params

---

## 8. Phased Shipping

| Phase | Ships | Shapers | Platform |
|-------|-------|---------|----------|
| V1.0 | Interface + routing + 2 shapers | XObserve + XOxide | macOS |
| V1.1 | Bus slots + 4 more shapers | +XOpress, XOgate, XOltre, XOverb | macOS + iPad |
| V1.2 | Remaining 6 + OBRIX Pocket | +all creative/utility/mod | All platforms |
| V2.0 | Expansion packs | Genre-specific shaper collections | All |

---

## 9. Governance Notes (Architect)

- All 12 parameter prefixes FROZEN on V1.0 ship
- D004 applies — every shaper parameter must affect audio
- Backward-compatible preset schema — old presets load clean
- DB001 (chaining vs mutual exclusivity) — documented: inserts chain, this is a design choice
- Blessing B016 (Brick Independence) extends — each shaper independently useful

---

*XO_OX Designs | Shaper Bus Architecture v1.0 | Ratified 2026-03-19*
