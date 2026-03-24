# XOceanic — XOlokun Integration Spec

**Engine:** OCEANIC
**Source instrument:** XOceanic
**Version:** 1.0 (standalone complete — Phase 3 ready)
**Date:** March 2026

---

## Engine Identity

| Field | Value |
|-------|-------|
| Gallery code | OCEANIC |
| Engine ID (adapter) | `"Oceanic"` |
| XOlokun short name | OCEANIC |
| Accent color | Phosphorescent Teal `#00B4A0` |
| Parameter prefix | `oceanic_` |
| Max voices | 1 (paraphonic — one shared voice path) |
| Practical polyphony | Unlimited (all 128 MIDI notes, constant CPU) |
| CPU budget | <6% single, <21% in dual-engine config |
| DSP location | `src/engine/` + `src/dsp/` inline `.h` headers |

---

## Gallery Role

**Sonic gap filled:** Paraphonic string ensemble + effects-as-revelation. No engine in the gallery does paraphonic synthesis (all notes sharing one filter/envelope path). No engine treats effects as tools for *revealing* hidden spectral content rather than processing/coloring.

**Character:** Warm vintage string ensemble body with a bioluminescent effects processor that reveals impossible colors hiding inside the harmonic content. The creature and the eyes that see it.

---

## Signal Chain

```
MIDI → Note Gate Table (128 entries)
     → Divide-Down Oscillator Bank (6 waveshapes per stop)
     → Registration Mixer (Violin, Viola, Cello, Bass, Contrabass, Horn)
     → Brightness Control
     → Triple Ensemble Chorus (3 BBD lines, phase-offset LFOs)
     → Paraphonic Filter (Cytomic SVF: LP/BP/HP)
     → Paraphonic Amp Envelope (shared ADSR)
     → [Optional: external coupling audio mixed in]
     → Chromatophore Pedalboard:
         FREEZE → SCATTER → TIDE → ABYSS → MIRROR
     → Chromatophore Modulator (organic pulsing)
     → Dry/Wet Mixer
     → Output Width → Master Volume → Soft Limiter
     → Output
```

---

## Parameter Namespace

**Prefix:** `oceanic_`
**Total parameters:** 55

Key parameters for coupling targets:

| ID | Range | Coupling relevance |
|----|-------|-------------------|
| `oceanic_filterCutoff` | 20-20000 Hz | Primary modulation target |
| `oceanic_ensemble` | 0-1 | Ensemble depth — EnvToMorph target |
| `oceanic_couplingLevel` | 0-1 | External audio input level |
| `oceanic_pedalMix` | 0-1 | Pedalboard wet/dry |
| `oceanic_drift` | 0-1 | Analog pitch drift |
| `oceanic_abyssMix` | 0-1 | Shimmer reverb send |
| `oceanic_freezeMix` | 0-1 | Spectral freeze amount |

---

## Coupling Compatibility

### OCEANIC as Coupling Target (other engines → OCEANIC)

| Coupling Type | Source | What OCEANIC Does | Musical Effect |
|---------------|--------|------------------|----------------|
| `AudioToWavetable` | ANY | Source audio mixed into pedalboard input | **Primary coupling** — any engine through chromatophore chain |
| `AmpToFilter` | ONSET, OVERBITE, ODDFELIX | Source amplitude → `filterCutoff` mod | Drum/bass hits sweep the string filter |
| `EnvToMorph` | ODYSSEY, ODDOSCAR | Source envelope → `ensemble` depth | External crescendos intensify ensemble shimmer |
| `LFOToPitch` | ODDOSCAR, OBLONG | Source LFO → master pitch drift | Cross-engine organic pitch wander |
| `FilterToFilter` | Any with filter | Source filter → OCEANIC filter cutoff | Filter tracking between engines |

### OCEANIC as Coupling Source (OCEANIC → other engines)

`getSampleForCoupling()` returns: post-pedalboard stereo output, normalized ±1.

| Target | Coupling Type | Musical Effect |
|--------|--------------|----------------|
| OVERDUB | `getSample` → send input | Shimmer strings through dub tape delay |
| OPAL | `AudioToWavetable` | String output granulated into time cloud |
| ODYSSEY | `AmpToFilter` | String amplitude modulates JOURNEY filter |
| OBESE | `EnvToMorph` | String envelope controls Mojo blend |

### Coupling types OCEANIC should NOT receive

| Type | Why |
|------|-----|
| `AmpToChoke` | Kills the sustained string pad — no musical use |
| `AudioToRing` | Ring mod on strings = ugly metallic artifacts |
| `AudioToFM` | FM on divide-down oscillators = noise, not music |
| `PitchToPitch` | Paraphonic — pitch determined by note gating, not continuous |

---

## Macro Mapping (M1-M4)

| Macro | Label | Primary Target | Secondary Target | Behavior |
|-------|-------|---------------|-----------------|----------|
| M1 | DEPTH | `pedalMix` ↑, `ensemble` ↑ | `chromDepth` ↑, `abyssMix` slight ↑ | Dry Solina → full bioluminescent processing |
| M2 | CURRENT | `chromRate` ↑, `chromSeparation` ↑ | `tideWarp` ↑, `scatterDensity` ↑, `drift` ↑ | Still water → churning deep current |
| M3 | COUPLING | `couplingLevel` ↑ | Context-dependent pedal engagement | External audio portal depth |
| M4 | ABYSS | `abyssMix` ↑, `abyssDecay` ↑ | `mirrorMix` ↑, `outputWidth` ↑, `separation` ↑ | Close intimate → infinite ocean abyss |

All 4 macros verified to produce audible change across entire range in every preset.

---

## Voice Architecture

| Field | Value |
|-------|-------|
| Type | Paraphonic (NOT polyphonic) |
| Max MIDI notes | 128 (all available simultaneously) |
| Voice stealing | N/A |
| Legato | N/A (shared envelope) |
| Portamento | N/A |
| CPU scaling | Constant — does not increase with note count |

---

## Preset Library

| Mood | Current | Target | Character |
|------|---------|--------|-----------|
| Foundation | 6 | 15 | Dry ensemble strings, warm and grounded |
| Atmosphere | 6 | 25 | Processed strings, glowing shimmer pads |
| Entangled | 4 | 25 | Coupling showcases for specific engine pairs |
| Prism | 6 | 20 | Bright chromatophore textures, scattered light |
| Flux | 6 | 15 | Rhythmic pedalboard, pulsing chromatophore |
| Aether | 6 | 20 | Frozen, infinite, suspended string aether |
| **Total** | **34** | **120** | All `.xometa` format, `oceanic_` namespaced |

---

## Visual Identity

- **Accent color:** Phosphorescent Teal `#00B4A0`
- **Material/texture:** Bioluminescent sea creature skin — semi-translucent, color shifting, organic pulse underneath
- **Icon concept:** Deep-sea creature silhouette with phosphorescent spots — the creature is the ensemble, the spots are the pedals
- **Panel character:** Dark translucent surface with alive-feeling color underneath. Pedalboard section has distinct stomp-box outlines with individual glow states.

---

## Integration Checklist (Phase 3 → 4)

```
[ ] Adapter: XOceanicAdapter.h implementing SynthEngine
[ ] Adapter: getEngineId() returns "Oceanic"
[ ] Adapter: getAccentColour() returns #00B4A0
[ ] Adapter: getMaxVoices() returns 1
[ ] Adapter: createParameterLayout() delegates to oceanic_ layout
[ ] Adapter: getSampleForCoupling() returns valid post-pedalboard stereo sample
[ ] Coupling: AudioToWavetable feeds external audio into pedalboard input
[ ] Coupling: AmpToFilter modulates oceanic_filterCutoff correctly
[ ] Coupling: ONSET×OCEANIC tested — drums breathe the string filter
[ ] Coupling: ODYSSEY×OCEANIC tested — Climax through chromatophores
[ ] Presets: all 120 in .xometa with oceanic_ namespace
[ ] Macros: M1-M4 audible in every preset
[ ] CPU: <6% solo, <21% in dual config
[ ] No NaN, no clicks, no DC offset
[ ] REGISTER_ENGINE(XOceanicAdapter) in Adapter.cpp
```

---

*Process: `/new-xo-engine` | Spec: `Docs/concepts/xoceanic_master_spec.md`*
