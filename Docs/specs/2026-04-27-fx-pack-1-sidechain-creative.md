# FX Pack 1 — Sidechain Creative (Otrium / Oblate / Oligo)

**Date:** 2026-04-27
**Author:** XO_OX Designs
**Status:** Detailed Design Spec — paper design, scaffolds Phase 0 API
**Parent plan:** `Docs/specs/2026-04-27-fx-engine-build-plan.md`

---

## 1. Purpose

Pack 1 is the first new FX pack and the proof-of-life consumer for Phase 0's three wildcard primitives. It also produces the **demo chain** that makes the MegaCouplingMatrix legible to first-time users (Otrium). Build target: ships alongside Phase 0.

Three chains, all using the **Coupling-driven** primitive; Oligo additionally uses **DNA-as-modulator**.

---

## 2. Otrium — Triangular Coupling Pump · prefix `otrm_`

**Concept:** 3 partner engines duck each other in a phase-staggered loop. The MegaCouplingMatrix's `TriangularCoupling` primitive is reused with re-interpreted semantics — A's envelope ducks B, B's ducks C, C's ducks A.

**Signal flow (5 stages):**
1. **Triangular Input Router** — accepts 3 partner engine refs (A/B/C) via 3 separate `TriangularCoupling` routes
2. **Phase-Stagger Envelope Followers** — 0°/120°/240° envelope detectors per partner
3. **Cross-Triangle Mixer** — A-env ducks B path, B-env ducks C path, C-env ducks A path
4. **VCA Bank** — 3 sidechain compressors with cross-routed sidechains
5. **Output Mixer** — sums, with internal/external mix ratio

**Parameters (12):**
- `otrm_pumpDepth` (0–1) · `otrm_pumpRate` (0.001–40 Hz, **D005 floor**) · `otrm_attack` (0.1–200 ms) · `otrm_release` (10–2000 ms)
- `otrm_phaseSkew` (0–360°, default 120) · `otrm_topology` (Equilateral / Isosceles / Chaotic / Cyclical)
- `otrm_partnerA_idx` · `otrm_partnerB_idx` · `otrm_partnerC_idx` (each 0–3)
- `otrm_couplingDepth` (0–1) — autonomous LFO ↔ partner-driven blend
- `otrm_dnaTilt` (0–1) — partner aggression DNA tilts duck spectrum
- `otrm_mix` (0–1)

**Coupling sources published:** *none.* The original spec listed `otrm.envA/B/C`, `otrm.phaseAngle`, `otrm.totalDuck` — these were struck on 2026-05-01 (Otrium seance recommendation 4). FX chains do not have a `getCouplingSample`-style hook today; building one for a single consumer is dis-economical. Otrium remains a *consumer* of coupling (partner audio + DNA), not a publisher. Re-evaluate if a second chain ever needs to publish.
**Coupling targets accepted:** 3× `TriangularCoupling` partner routes, M1 CHARACTER, aftertouch
**Wildcard:** the matrix demo — phase-staggered loop impossible elsewhere.

---

## 3. Oblate — Spectral Gate · prefix `obla_`

**Concept:** STFT gate where each FFT bin is gated by the partner engine's spectrum. Partner brightness DNA tilts the threshold curve.

**Signal flow (5 stages):**
1. **STFT Analyzer** (FFT, selectable window 256–2048)
2. **Sidechain Key Extractor** — partner spectrum → per-bin key amplitudes
3. **Per-Band Gate Threshold Computer** — DNA-tilted threshold per bin
4. **Anti-Zip Smoothing** — preserves transients, kills metallic artifacts
5. **ISTFT Resynthesis**

**Parameters (11):**
- `obla_threshold` (–60 to 0 dB) · `obla_ratio` (1–∞) · `obla_attack` (0.1–50 ms) · `obla_release` (5–500 ms)
- `obla_keyEngine` (0–3) — sidechain partner
- `obla_fftSize` (256/512/1024/2048)
- `obla_tilt` (–1 to 1) — frequency-dependent threshold tilt
- `obla_dnaCoupling` (0–1) — partner brightness DNA scales tilt
- `obla_smoothing` (0–1)
- `obla_breathRate` (0.001–2 Hz, **D005 floor**)
- `obla_mix` (0–1)

**Coupling sources published:** `obla.gainPerBand`, `obla.totalGate`, `obla.spectralCentroid`
**Coupling targets accepted:** partner audio (sidechain key), partner brightness DNA, M1 CHARACTER, aftertouch
**Wildcard:** sidechain key driven by partner *spectrum*, not just amplitude.

---

## 4. Oligo — Frequency-Selective Ducker · prefix `olig_`

**Concept:** 4-band Linkwitz-Riley split with per-band ducking. Partner DNA shapes per-band release time (brightness → high band, density → lo-mid, aggression → low band).

**Signal flow (5 stages):**
1. **4-Band LR Split** (low / lo-mid / hi-mid / high)
2. **Per-Band Envelope Followers** from sidechain key
3. **Per-Band VCAs** with depth/attack/release
4. **DNA-Aware Release Scaler** — release per band scales with partner DNA
5. **Band Recombine** with phase correction

**Parameters (13):**
- `olig_lowDepth` (0–1) · `olig_loMidDepth` · `olig_hiMidDepth` · `olig_highDepth`
- `olig_attack` (0.1–50 ms) · `olig_release` (10–1000 ms) — base values, scaled per band by DNA
- `olig_dnaScale` (0–1) — strength of DNA-aware release scaling
- `olig_lowSplit` (40–200 Hz) · `olig_midSplit` (200–2000 Hz) · `olig_highSplit` (2000–10000 Hz)
- `olig_keyEngine` (0–3)
- `olig_breathRate` (0.001–2 Hz, **D005 floor**) — drift on crossovers
- `olig_mix` (0–1)

**Coupling sources published:** `olig.gainLow/LoMid/HiMid/High`, `olig.totalDuck`
**Coupling targets accepted:** partner audio, partner DNA (3 axes), M1 CHARACTER, aftertouch
**Wildcard:** per-band release time mapped to partner DNA.

---

## 5. Phase 0 API Requirements (the key deliverable)

Pack 1 reveals exactly what the three Phase 0 primitives must expose:

### 5.1 `DNAModulationBus.h`
```cpp
class DNAModulationBus {
public:
    enum class Axis { Brightness, Warmth, Movement, Density, Space, Aggression };

    void  setBaseDNA(int engineSlot, std::array<float, 6> dna) noexcept;
    void  applyMacroWarp(int engineSlot, float characterMacro) noexcept; // block-rate, smoothed
    float get(int engineSlot, Axis axis) const noexcept;                  // 0..1, smoothed
    int   registerCouplingSource(int engineSlot, Axis axis);              // returns matrix source ID
};
```
Used by Otrium (`dnaTilt`), Oblate (`dnaCoupling`), Oligo (`dnaScale`). Per-engine warp at block-rate per **D1**.

### 5.2 `MoodModulationBus.h`
Not consumed by Pack 1 — defer interface freeze until Pack 8 (Mastering) needs it. Build a stub header in Phase 0 to lock the file path and namespace; minimal API.

### 5.3 `Aging.h` (AGE convention)
Not consumed by Pack 1 — defer concrete helpers until Pack 2 (Analog Warmth). Stub header only.

### 5.4 `MegaCouplingMatrix` — no changes required
Otrium reuses existing `TriangularCoupling` (already block-rate, already routes 3-channel state). Three separate routes wired by user/preset. Verified against `Source/Core/SynthEngine.h:44-54`.

---

## 6. Doctrine Compliance Matrix

| Doctrine | Otrium | Oblate | Oligo |
|---|---|---|---|
| D001 velocity → timbre | velocity → DNA aggression → pumpDepth | velocity → threshold | velocity → DNA → per-band release |
| D002 modulation | 3 envelopes + LFO + 3 mod sources | breath LFO + DNA + sidechain + M1 + AT | breath LFO + DNA × 3 + sidechain |
| D003 physics | n/a (control FX) | STFT/ISTFT cited | LR4 phase math cited |
| D004 dead params | every param drives audio | ✓ | ✓ |
| D005 must breathe | `pumpRate` floor 0.001 Hz | `breathRate` floor 0.001 Hz | `breathRate` floor 0.001 Hz |
| D006 expression | aftertouch + mod wheel routable | aftertouch → threshold | aftertouch → mid bands |

---

## 7. Files to Create

```
Source/DSP/Effects/OtriumChain.h
Source/DSP/Effects/OblateChain.h
Source/DSP/Effects/OligoChain.h
Source/Engines/Otrium/OtriumEngine.h
Source/Engines/Oblate/OblateEngine.h
Source/Engines/Oligo/OligoEngine.h
```

Plus modifications to:
- `Source/Core/EpicChainSlotController.h` — chain enum + factory
- `Source/Core/PresetManager.h` — `validEngineNames` + `frozenPrefixForEngine`
- `Source/XOceanusProcessor.cpp` — `REGISTER_ENGINE` × 3
- `Docs/engines.json` — 3 entries
- `CLAUDE.md` — engine count + 3 prefix rows
- `Docs/reference/engine-color-table.md` — 3 color rows
- `Docs/specs/xoceanus_master_specification.md` §3.1 — 3 rows

---

## 8. Demo Preset Concepts (≥5 per chain)

**Otrium:** Triangular Throb (3 perc) · Three-Way Pad (3 atmosphere) · Bass Triangle (3 bass) · Chaos Topology · Stereo Triangle (panned A/center/B)
**Oblate:** Vocal Cathedral · Spectral Stutter · Frequency Mirror · Dynamic Hall · Ghosted Choir
**Oligo:** Pumping Lows · DNA Pump · Frequency Flutter · Selective Sidechain · Band Cascade

---

## 9. Build Sequence

1. Phase 0 ships `DNAModulationBus` (full API) + stub `MoodModulationBus`/`Aging.h`
2. Scaffold the 3 chains via `/new-xo-engine`
3. Wire `TriangularCoupling` consumption in Otrium (no matrix changes)
4. `/validate-engine` × 3 → `/synth-seance` × 3 (per **D2**)
5. Build ≥15 demo presets across 3 chains
6. Update CLAUDE.md, engines.json, master spec, color table, seance cross-ref

---

## 10. Resolved API Decisions

Locked 2026-04-27.

### A1 — Per-axis DNA warp weights: **support both, default uniform**

`DNAModulationBus::applyMacroWarp` exposes two overloads:

```cpp
void applyMacroWarp(int engineSlot, float characterMacro) noexcept;                          // uniform
void applyMacroWarp(int engineSlot, float characterMacro, std::array<float,6> weights);      // per-axis
```

Uniform default keeps the simple case simple; per-axis enables the expressive design the 6D structure exists for (preset where M1 mostly warps brightness/movement, leaves warmth/aggression alone).

### A2 — Oblate STFT latency: **default FFT 1024, HQ Mode toggle for 2048**

Default `obla_fftSize = 1024` (~23 ms latency, strong spectral resolution). 2048 is exposed via an HQ Mode toggle. Oblate declares PDC for the active FFT size; switching size produces a one-time host compensation event, acceptable.

**Param impact:** add `obla_hqMode` (bool, default false) — when true, allows `obla_fftSize` selector to reach 2048.

### A3 — Otrium Cyclical topology tempo-sync: **both, user-selectable**

New parameter `otrm_syncMode` (Free / Sync). In Sync mode `otrm_pumpRate` becomes a beat division (1/16 → 32 bars). D005 still satisfied via the breath LFO regardless of Sync state.

**Param impact:** Otrium parameter count rises from 12 to 13.
