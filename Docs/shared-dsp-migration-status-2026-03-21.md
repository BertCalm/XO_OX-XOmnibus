# Shared DSP Migration Status
**Date**: 2026-03-21
**Audited by**: automated grep + line-count analysis
**Scope**: 44 engines × 7 shared utilities in `Source/DSP/`

---

## Summary

| Metric | Value |
|--------|-------|
| Total engines audited | 44 |
| Total possible migrations (44 × 7) | 308 |
| Total adopted | 71 |
| **Overall coverage** | **23.1%** |
| Engines with zero migrations | 21 |
| Engines fully migrated (6–7/7) | 2 (Oware 6/7, Orca 5/7) |
| Estimated removable LOC (inline duplication) | **~367 lines** |
| Estimated removable LOC (inline struct bodies) | **~110 additional lines** |
| **Total estimated removable** | **~477 lines** |

---

## Per-Utility Adoption Rate

| Utility | Path | Adopters | Coverage | Notes |
|---------|------|----------|----------|-------|
| `StandardLFO.h` | `Source/DSP/StandardLFO.h` | 18/44 | **41%** | Best-adopted utility |
| `StandardADSR.h` | `Source/DSP/StandardADSR.h` | 18/44 | **41%** | Tied for best |
| `VoiceAllocator.h` | `Source/DSP/VoiceAllocator.h` | 20/44 | **45%** | Highest absolute adoption |
| `GlideProcessor.h` | `Source/DSP/GlideProcessor.h` | 4/44 | **9%** | Critical gap — glide is inline in 26 engines |
| `ParameterSmoother.h` | `Source/DSP/ParameterSmoother.h` | 5/44 | **11%** | Smoothing patterns scattered fleet-wide |
| `PitchBendUtil.h` | `Source/DSP/PitchBendUtil.h` | 5/44 | **11%** | pitchWheel math duplicated in 15 engines |
| `FilterEnvelope.h` | `Source/DSP/FilterEnvelope.h` | 1/44 | **2%** | Almost entirely unadopted |

**Key finding**: The three newest utilities (GlideProcessor, FilterEnvelope, PitchBendUtil) have the lowest adoption — they were written after most engines were already built. StandardLFO/ADSR/VoiceAllocator were integrated retroactively in the Wave 1 migration pass; the Wave 2 pass (glide, smoothing, pitch bend) has not yet happened.

---

## Migration Matrix — 44 Engines × 7 Utilities

`Y` = includes the shared header | `N` = not yet migrated | `(n)` = inline count of duplicated pattern lines

| Engine | LOC | LFO | ADSR | FiltEnv | VoiceAlloc | Glide | PSmooth | PBend | Migrated |
|--------|-----|-----|------|---------|------------|-------|---------|-------|----------|
| **Bite** | 2372 | N | Y | N | Y | N(18) | N | N | 2/7 |
| **Bob** | 1740 | Y | Y | N | Y | N(4) | N | N | 3/7 |
| **Drift** | 1736 | Y | Y | N | Y | N(3) | N(3) | N | 3/7 |
| **Dub** | 1327 | N(12) | Y | N | Y | N(4) | N | N | 2/7 |
| **Fat** | 1544 | Y | Y | N | Y | N(4) | N | Y | 4/7 |
| **Morph** | 1245 | N(10) | Y | N | Y | N(16) | N | Y | 3/7 |
| **Obbligato** | 510 | N | N | N | N | N | N | N | 0/7 |
| **Oblique** | 1700 | N(7) | N(15) | N | N | N(12) | N | Y | 1/7 |
| **Obrix** | 1758 | N★ | N★ | N | N | N(10) | N | N | 0/7 |
| **Obscura** | 1730 | Y | Y | N | Y | N(16) | Y | N | 4/7 |
| **Obsidian** | 1433 | Y | Y | N | Y | N(12) | N(5) | N | 3/7 |
| **OceanDeep** | 943 | N(9) | N | N | N | N | N(8) | N(3) | 0/7 |
| **Oceanic** | 1331 | Y | Y | N | Y | N(13) | Y | N | 4/7 |
| **Ocelot** | 152† | N | N | N | N | N | N | N | 0/7 |
| **Octopus** | 1455 | Y | Y | N | Y | N(11) | N(6) | N | 3/7 |
| **Ohm** | 616 | N | N | N | N | N | N | N | 0/7 |
| **Ole** | 374 | N | N | N | N | N | N | N | 0/7 |
| **Ombre** | 946 | N | N | N | N | N | N | N | 0/7 |
| **Onset** | 2206 | N★ | N(8) | N | N | N | N | N | 0/7 |
| **Opal** | 2414 | Y | Y | N | Y | N(24) | N | N | 3/7 |
| **OpenSky** | 1417 | Y‡ | Y | N | Y | N | N | N | 3/7 |
| **Optic** | 1054 | N | N(5) | N | N | N | N | N | 0/7 |
| **Oracle** | 1641 | Y | Y | N | Y | Y | N | N | 4/7 |
| **Orbital** | 1628 | N | N(2) | N | N | N(1) | N | N | 0/7 |
| **Orbweave** | 1125 | N★ | N★ | N | N | N(9) | N | N(3) | 0/7 |
| **Orca** | 1348 | Y | Y | N | Y | Y | Y | N | 5/7 |
| **Organism** | 997 | N | N | N | N | N | N(3) | N | 0/7 |
| **Organon** | 1587 | N | N | N | N | N | N | N | 0/7 |
| **Origami** | 1863 | Y | N★ | N | Y | Y | Y | N | 4/7 |
| **Orphica** | 635 | N | N | N | N | N | N | N | 0/7 |
| **Osprey** | 2050 | Y | Y | N | Y | N(33) | N(3) | N | 3/7 |
| **Osteria** | 1921 | Y | Y | N | Y | N(5) | N | N | 3/7 |
| **Ostinato** | 2189 | Y | N(5) | N | N | N | N | N | 1/7 |
| **Ottoni** | 462 | N | N | N | N | N | N | N | 0/7 |
| **Ouie** | 1903 | Y | Y | N | Y | N(9) | N(3) | N | 3/7 |
| **Ouroboros** | 1546 | N | N | N | N | N | N | N | 0/7 |
| **Outwit** | 716† | N | N | N | N | N(12) | N | N | 0/7 |
| **Overlap** | 775† | N | N | N | N | N(2) | N | N | 0/7 |
| **Overtone** | 921 | N | N | N | N | N | N(3) | N | 0/7 |
| **Overworld** | 711 | N | N | N | N | N(1) | N | N | 0/7 |
| **Oware** | 915 | Y | N(2) | Y | Y | Y | Y | Y | 6/7 |
| **Owlfish** | 245† | N | N | N | N | N | N | N | 0/7 |
| **Oxbow** | 691 | Y | N | N | N | N | N(4) | N | 1/7 |
| **Snap** | 1095 | N(5) | Y | N | Y | N | N | Y | 3/7 |

**Notes:**
★ = has an inline struct definition (not just raw variables) — higher migration effort
‡ = includes StandardLFO but also has a local `SkyBreathingLFO` specialty struct
† = adapter engine or engine with split sub-files; LOC shown is for primary header only
Numbers in `N(n)` = estimated lines of inline duplicated code for that pattern

---

## Inline Struct Inventory (Highest-Effort Migrations)

These engines have full inline struct definitions that must be replaced, not just variable refactors:

| Engine | Inline Struct | Lines Est. | Equivalent Shared Util |
|--------|--------------|-----------|------------------------|
| Obrix | `ObrixADSR` (line 128) | ~35 | `StandardADSR` |
| Obrix | `ObrixLFO` (line 175) | ~40 | `StandardLFO` |
| Orbweave | `OrbweaveADSR` (line 42) | ~30 | `StandardADSR` |
| Orbweave | `OrbweaveLFO` (line 89) | ~40 | `StandardLFO` |
| Osteria | `OsteriaADSR` (line 118) | ~35 | `StandardADSR` |
| Osteria | `OsteriaLFO` (line 606) | ~35 | `StandardLFO` |
| Onset | `BreathingLFO` (line 1347) | ~25 | `StandardLFO` |
| Fat | `BreathingLFO` (line 693) | ~25 | `StandardLFO` |
| Origami | `OrigamiADSR` (line 114) | ~40 | `StandardADSR` |
| Opal | inline Stage enum (line 262) | ~15 | `StandardADSR` |

Total inline struct code: ~320 lines across 9 engines (some already include the shared header for other utilities but haven't swapped out the old struct yet).

---

## Lines of Code That Could Be Removed

### By utility (fleet-wide estimate)

| Inline Pattern | Engines Affected | Est. Removable Lines |
|---------------|------------------|---------------------|
| Inline glide/portamento vars | 26 engines | ~180 |
| Inline ADSR vars + structs | 12 engines | ~95 |
| Inline LFO vars + structs | 11 engines | ~60 |
| Inline parameter smoothing | 8 engines | ~37 |
| Inline pitch bend math | 6 engines | ~25 |
| Inline filter envelope vars | 4 engines | ~20 |
| **Total** | | **~417 lines** |

### Top 10 engines by removable LOC

| Rank | Engine | Est. Removable | % of Engine LOC |
|------|--------|---------------|-----------------|
| 1 | Osprey | ~36L | 1.8% |
| 2 | Oblique | ~34L | 2.0% |
| 3 | Morph | ~26L | 2.1% |
| 4 | Opal | ~24L | 1.0% |
| 5 | Orbweave | ~21L | 1.9% |
| 6 | OceanDeep | ~20L | 2.1% |
| 7 | Bite | ~18L | 0.8% |
| 8 | Obsidian | ~17L | 1.2% |
| 9 | Octopus | ~17L | 1.2% |
| 10 | Obscura | ~16L | 0.9% |

---

## Priority List for Remaining Migrations

Ranked by urgency score (inline line count + missing utility penalty). Each entry shows which utilities to add in the recommended order.

### Tier 1 — High Priority (structural debt + most inline duplication)

| Rank | Engine | Cur | Removable | Recommended Next |
|------|--------|-----|-----------|------------------|
| 1 | **Oblique** | 1/7 | ~34L | StandardLFO → StandardADSR → GlideProcessor → VoiceAllocator → FilterEnvelope |
| 2 | **Orbweave** | 0/7 | ~21L | StandardADSR → StandardLFO → GlideProcessor → PitchBendUtil → VoiceAllocator |
| 3 | **OceanDeep** | 0/7 | ~20L | StandardLFO → PitchBendUtil → ParameterSmoother |
| 4 | **Osprey** | 3/7 | ~33L | GlideProcessor (single utility — huge payoff) |
| 5 | **Obrix** | 0/7 | ~15L | StandardADSR → StandardLFO → GlideProcessor → PitchBendUtil → VoiceAllocator |

### Tier 2 — Medium Priority (meaningful inline debt, manageable effort)

| Rank | Engine | Cur | Removable | Recommended Next |
|------|--------|-----|-----------|------------------|
| 6 | **Outwit** | 0/7 | ~12L | GlideProcessor |
| 7 | **Morph** | 3/7 | ~26L | StandardLFO → GlideProcessor |
| 8 | **Onset** | 0/7 | ~9L | StandardADSR → StandardLFO → VoiceAllocator → FilterEnvelope |
| 9 | **Opal** | 3/7 | ~24L | GlideProcessor |
| 10 | **Bite** | 2/7 | ~18L | GlideProcessor |
| 11 | **Dub** | 2/7 | ~16L | StandardLFO → GlideProcessor |
| 12 | **Optic** | 0/7 | ~5L | StandardADSR → FilterEnvelope |
| 13 | **Orbital** | 0/7 | ~3L | StandardADSR → GlideProcessor → VoiceAllocator |
| 14 | **Obsidian** | 3/7 | ~17L | GlideProcessor → ParameterSmoother |
| 15 | **Octopus** | 3/7 | ~17L | GlideProcessor → ParameterSmoother |
| 16 | **Origami** | 4/7 | ~9L | StandardADSR → FilterEnvelope |
| 17 | **Osteria** | 3/7 | ~5L | GlideProcessor |
| 18 | **Ostinato** | 1/7 | ~5L | StandardADSR → VoiceAllocator → FilterEnvelope |

### Tier 3 — Low Priority (minimal inline debt, structural consistency value only)

| Rank | Engine | Cur | Notes |
|------|--------|-----|-------|
| 19 | Organism | 0/7 | Only ParameterSmoother needed (~3L) |
| 20 | Overtone | 0/7 | Only ParameterSmoother needed (~3L) |
| 21 | Overlap | 0/7 | Adapter engine — GlideProcessor + VoiceAllocator when expanding |
| 22 | Overworld | 0/7 | Only GlideProcessor needed (~1L) |
| 23 | Ouie | 3/7 | GlideProcessor + ParameterSmoother |
| 24 | Obscura | 4/7 | GlideProcessor only (~16L) |
| 25 | Oceanic | 4/7 | GlideProcessor only (~13L) |
| 26 | Drift | 3/7 | GlideProcessor + ParameterSmoother |
| 27 | Snap | 3/7 | StandardLFO only |
| 28 | Bob | 3/7 | GlideProcessor only |
| 29 | Oxbow | 1/7 | ParameterSmoother only |
| 30 | Oware | 6/7 | StandardADSR (only missing one) |
| 31 | Oracle | 4/7 | GlideProcessor (has it for pitch but not glide port.) |
| 32 | Orca | 5/7 | PitchBendUtil — closest to full migration |

### Tier 4 — Assess First (may not need all utilities)

| Engine | Notes |
|--------|-------|
| Obbligato | No voices, no LFO — likely only VoiceAllocator if ever polyphonised |
| Ocelot | 152-line adapter; DSP lives in OcelotVoice.h (3176 total LOC in folder) — assess Voice.h |
| Ohm | Small engine, needs assessment of which utils are applicable |
| Ole | Small engine (374 LOC), likely only VoiceAllocator |
| Ombre | Medium engine — needs voice/LFO assessment |
| Organon | 1587 LOC with no migrations at all — high-value full audit |
| Orphica | 635 LOC — assess all 7 |
| Ottoni | 462 LOC — likely VoiceAllocator |
| Ouroboros | 1546 LOC, zero migrations — full audit recommended |
| Owlfish | Adapter engine with 2382 LOC across 5 sub-files; DSP in OwlfishVoice.h |

---

## Engines with Partial Migrations Still Carrying Old Code

These engines already include a shared util but still have old inline patterns coexisting:

| Engine | Includes | Still Has Inline |
|--------|----------|-----------------|
| Origami | StandardLFO, VoiceAllocator, GlideProcessor, ParameterSmoother | `OrigamiADSR` struct (needs swap) |
| OpenSky | StandardLFO | Local `SkyBreathingLFO` specialty struct (intentional — keep) |
| Oware | All except StandardADSR | Raw `attackTime`/`releaseTime` vars (2 lines — low risk) |

---

## Migration Campaign Recommendations

### Quick wins (GlideProcessor single-utility pass)
GlideProcessor is the most underutilised utility (9% adoption) despite glide patterns appearing in 26 engines. A targeted pass adding only `GlideProcessor.h` to the following engines would yield ~160 removable lines across a single afternoon of work:

**Osprey** (33L), **Morph** (16L), **Opal** (24L), **Bite** (18L), **Obscura** (16L), **Obsidian** (12L), **Oceanic** (13L), **Octopus** (11L), **Ouie** (9L), **Orbweave** (9L), **Outwit** (12L), **Dub** (4L), **Bob** (4L)

### Biggest single engine payoffs
1. **Oblique** — 0→full migration would remove ~34 lines and close a 1/7 engine
2. **Orbweave** — 0→full migration removes ~21 lines + 2 inline structs (~70 additional)
3. **Obrix** — flagship engine at 0/7; clearing inline ObrixADSR + ObrixLFO alone removes ~75 lines and de-forks the most-played engine

### FilterEnvelope push
`FilterEnvelope.h` at 2% adoption is the biggest opportunity for tonal improvement (not just code health). Seance scores consistently flagged missing filter envelopes as a quality gap. Priority targets: **Optic**, **Ostinato**, **Onset**, **Oblique**, **Orbital**.

---

## Reference: Shared Utility Locations

```
Source/DSP/StandardLFO.h        — 5-shape LFO, sample-rate-correct, S&H via LCG
Source/DSP/StandardADSR.h       — AD/AHD/ADSR modes, linear attack, exp decay/release
Source/DSP/FilterEnvelope.h     — ADSR for filter/pitch mod, separate from amp envelope
Source/DSP/VoiceAllocator.h     — LRU + release-priority stealing, works with any Voice type
Source/DSP/GlideProcessor.h     — Hz-space portamento, exp IIR, 0 = instant snap
Source/DSP/ParameterSmoother.h  — one-pole smoothing, 5ms default, zipper-free automation
Source/DSP/PitchBendUtil.h      — MIDI 14-bit → bipolar → semitones → freq ratio pipeline
```
