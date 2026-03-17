# SRO Fleet Audit

**Date:** 2026-03-17
**Engines scanned:** 34
**Auditor:** SRO Team (automated scan)

---

## ZERO-IDLE STATUS

```
SilenceGate integrated:  0 / 34 engines
SilenceGate missing:    34 / 34 engines — ALL ENGINES
```

**Every engine in the fleet lacks SilenceGate.** This is the single highest-impact optimization available. Silent engines currently burn full CPU.

---

## CONTROL-RATE COUPLING

```
ControlRateReducer integrated:  0 / 34 engines
All 34 engines process coupling at audio rate (or stub)
```

**No engine uses ControlRateReducer.** However, many engines (SNAP, MORPH, DUB, DRIFT, BOB, FAT, BITE, ONSET, OVERWORLD, OLE, OTTONI, OBBLIGATO, ORPHICA, OHM) already use scalar-accumulator coupling — effectively control-rate without the formal component. The remaining engines (OVERLAP, OUTWIT, OPAL, ORBITAL, ORCA, OCTOPUS, OMBRE, OCEANIC, OSPREY, etc.) process coupling per-sample from source buffers and are candidates for 1/32 decimation via ControlRateReducer.

---

## SROTables (LUT) USAGE

```
SROTables integrated:  0 / 34 engines
```

**No engine uses SROTables lookup tables.** All trigonometric/transcendental math uses either `std::` or `FastMath` inline approximations.

---

## MATH HOTSPOTS

Engines ranked by `std::sin/cos/tan/exp/pow/tanh` call count (includes all `.h` files per engine):

| Engine | std:: Calls | Per-Sample Hotspots | Notes |
|--------|------------|---------------------|-------|
| **OCELOT** | **35** | `std::sin` × ~8 (oscillators, LFOs), `std::tanh` × 6 (saturation in voice pool + wavefolder), `std::cos` × 2, `std::pow` × ~12, `std::exp` × 4 | **Worst offender.** Multi-file architecture (7 DSP files) with per-sample sin/tanh throughout. Local tanh saturation in WaveFolder.h, OcelotVoice.h, OcelotVoicePool.h, OcelotEmergent.h. |
| **OPAL** | **21** | `std::cos` × 7 (grain windows), `std::sin` × 6 (oscillators, pan), `std::pow` × 5, `std::tan` × 1 | Grain windows use `std::cos` per-grain. OpalDSP.h duplicates OpalEngine.h grain envelope code. |
| **FAT** | **13** | `std::tan` × 2 (**filter prewarp — PER SAMPLE**), `std::exp` × 4 (envelope coeffs), `std::pow` × 3 | **Critical:** ZDF Ladder and ZDF SVF both call `std::tan(pi * cutoff / sr)` per sample. `std::tan` is the most expensive transcendental (~50 cycles). Should cache at block rate or use `CytomicSVF::setCoefficients_fast()`. |
| **ORIGAMI** | **12** | `std::cos/sin` × 6 (FFT twiddle factors, spectral pan), `std::pow` × 3 (warp), `std::exp` × 2 | FFT twiddle factors precomputed in prepare — benign. Per-voice spectral pan (`std::cos/sin` line 813-814) is per-sample. |
| **OWLFISH** | **11** | `std::sin` × 1 (grain LFO), `std::pow` × 5 (pitch, filter cutoff), `std::exp` × 3, `std::tan` × 1 | Has its own `OwlfishFastMath.h` (local duplicate of FastMath.h) and `OwlfishCytomicSVF.h` (local duplicate of CytomicSVF). |
| **BOB** | **10** | `std::sin` × 1 (wavetable init), `std::tan` × 2 (**filter prewarp — PER SAMPLE**), `std::exp` × 3 (envelope), `std::pow` × 2 | **Critical:** BobFilter ZDF SVF calls `std::tan(pi * cutoff * invSR)` per sample in `process()`. Same issue as FAT. Should cache coefficients at block rate. |
| **ORBITAL** | **9** | `std::pow` × 4 (partial tilt, formant), `std::exp` × 4 (Gaussian, decay), `std::sin` × 1 (drift) | Partial tilt computation (`std::pow` line 86) is per-partial per-block — potentially hot with 32 partials. |
| **ONSET** | **9** | `std::exp` × 3 (decay coefficients), `std::pow` × 2 (LoFi — block-constant), `std::cos/sin` × 2 (blend pan) | Per-voice blend pan uses `std::cos/sin` (line 1775-1776). Rest is block-constant. |
| **ORACLE** | **8** | `std::sin` × 2 (breakpoint init), `std::exp` × 2 (smoothing), `std::pow` × 3 (frequency), `std::tan` × 1 (Cauchy) | `std::tan` for Cauchy distribution (line 1316) is per-voice stochastic — hot path. `std::pow` for maqam frequency is per-voice. |
| **BITE** | **8** | `std::sin` × 1 (wavetable init), `std::exp` × 4 (envelope/decay coeffs), `std::pow` × 3 | Wavetable init is one-time. Envelope `std::exp` is per-note-on. `std::pow` for bitcrusher is per-sample (line 620). |
| **OHM** | **7** | `std::sin` × 6 (FM oscillators — per-sample!), `std::pow` × 1 | **Critical:** OHM uses raw `std::sin` for all FM oscillator output (lines 39-67, 113). 6 per-sample `std::sin` calls across Dad's instruments. Prime candidate for LUT or fastSin. |
| **OVERLAP** | **7** | `std::sin` × 2 (LFO, pan), `std::cos` × 1 (pan), `std::pow` × 3 (filter), `std::tan` × 1 (SVF) | No FastMath usage at all. No CytomicSVF. Local SVF implementation (line 224). |
| **OCEANIC** | **7** | `std::pow` × 3 (frequency, particles), `std::exp` × 2 (smoothing, glide), `std::cos/sin` × 2 (pan) | Per-voice pan uses `std::cos/sin`. Particle frequency uses `std::pow` per particle. |
| **OBSIDIAN** | **7** | `std::cos` × 1 (LUT init — one-time), `std::sin` × 2 (LFO, wavefold), `std::exp` × 2, `std::pow` × 2 | Builds its own cosine LUT at init (line 404) — could use SROTables. Per-sample wavefold `std::sin` (line 1231). |
| **OCTOPUS** | **6** | `std::sin` × 3 (oscillator + harmonics + fold — per-sample), `std::exp` × 3 | Arm oscillators use `std::sin` per-sample per-arm (up to 8 arms × harmonics). Significant hotspot. |
| **ORCA** | **6** | `std::sin` × 2 (oscillator + harmonics — per-sample), `std::exp` × 3, `std::pow` × 1 | Similar to Octopus — wavetable synthesis with per-sample `std::sin` for additive harmonics. |
| **OBLIQUE** | **6** | `std::cos/sin` × 4 (bounce panning — per-bounce), `std::pow` × 2 | Bounce panning uses `std::cos/sin` per active bounce. Block-constant `std::pow` for decay. |
| **DRIFT** | **6** | `std::sin` × 1 (LFO), `std::exp` × 4 (smoothing, envelope coeffs) | LFO `std::sin` is per-sample. Envelope coeffs are per-note-on. Mostly benign. |
| **OSPREY** | **5** | `std::cos/sin` × 2 (pan), `std::exp` × 2 (glide), `std::pow` × 1 | Per-voice pan is per-sample. Glide coeffs are per-note-on. |
| **OBSCURA** | **5** | `std::sin` × 1 (mode displacement), `std::cos` × 1 (Hann window), `std::exp` × 2, `std::pow` × 1 | Hann window (line 1577) computed per-sample in overlap-add. |
| **MORPH** | **5** | `std::sin` × 1 (wavetable init), `std::exp` × 1 (glide), `std::pow` × 3 (detune) | Wavetable init is one-time. `std::pow` for detune per unison voice — moderate. |
| **OUROBOROS** | **4** | `std::cos/sin` × 4 (3D projection — per-sample) | Strange attractor projection uses 4 trig calls per sample. Justified by DSP design (B003 Leash). |
| **OTTONI** | **4** | `std::sin` × 3 (micro-drift, valve pitch, chorus — per-sample), `std::pow` × 1 | Per-sample `std::sin` for vibrato and chorus LFOs. |
| **OSTERIA** | **4** | `std::pow` × 1 (reverb decay), `std::cos/sin` × 2 (pan), `std::pow` × 1 | Reverb decay is block-constant. Pan is per-voice. |
| **OBBLIGATO** | **4** | `std::sin` × 3 (flutter, chorus, phaser — per-sample), `std::pow` × 1 | Three per-sample `std::sin` for modulation. |
| **DUB** | **4** | `std::exp` × 4 (coefficients — all block/init-time) | All benign — coefficient calculations only. |
| **SNAP** | **3** | `std::sin` × 1 (LFO vibrato — per-sample), `std::pow` × 2 (detune, freq) | Per-sample sin for LFO. `std::pow` for midiToFreq (line 999). |
| **OVERWORLD** | **3** | `std::sin` × 1 (era drift), `std::exp` × 2 (decay, portamento) | Per-sample `std::sin` for era drift. Coeffs are block-constant. No FastMath. No CytomicSVF. |
| **ORPHICA** | **3** | `std::sin` × 1 (sub oscillator — per-sample), `std::cos` × 1 (grain window), `std::pow` × 1 | Sub oscillator uses raw `std::sin`. |
| **ORGANON** | **2** | `std::pow` × 1 (modal frequency), `std::sin` × 1 (phason modulation) | Minimal — well-optimized. Phason `std::sin` is per-voice. |
| **OLE** | **2** | `std::sin` × 1 (tremolo — per-sample), `std::pow` × 1 (freq) | Light usage. |
| **OUTWIT** | **1** | `std::sin` × 1 (LFO sine shape) | Minimal — only one LFO shape uses `std::sin`. |
| **OMBRE** | **1** | `std::pow` × 1 (midiToFreq) | Minimal — only frequency calculation. |

---

## KERNEL REUSE

### CytomicSVF Usage

```
Using CytomicSVF:      22 / 34 engines
Missing CytomicSVF:    12 engines
```

**Engines without CytomicSVF** (may have local filter implementations):

| Engine | Local Filter Status |
|--------|-------------------|
| BOB | Has own SVF with `std::tan` prewarp (lines 626-629) |
| FAT | Has own SVF filter with coefficient caching (FatSVF class) |
| MORPH | No SVF — uses different filter topology |
| OCELOT | Local SVF in OcelotFloor.h (line 325: `std::sin` prewarp) |
| OHM | No SVF — FM-only architecture |
| OLE | No SVF — Constellation stub engine |
| OBBLIGATO | No SVF — uses one-pole smoothing |
| OTTONI | No SVF — Constellation stub engine |
| OUROBOROS | No SVF — strange attractor, no traditional filter |
| OUTWIT | No SVF — Phase 3 adapter |
| OVERLAP | Local SVF (line 224: `std::tan` prewarp) — no CytomicSVF, no FastMath |
| OVERWORLD | No SVF — era-based crossfade engine |

### Local Duplicates (Upcycle Candidates)

| Engine | Duplicate | Kernel Equivalent |
|--------|-----------|-------------------|
| **OWLFISH** | `OwlfishFastMath.h` — local copy of FastMath | `DSP/FastMath.h` |
| **OWLFISH** | `OwlfishCytomicSVF.h` — local copy of CytomicSVF | `DSP/CytomicSVF.h` |
| **OCELOT** | `WaveFolder.h` — `std::tanh(x * drive)` saturation | `FastMath::fastTanh()` or `Saturator.h` |
| **OCELOT** | `OcelotVoicePool.h` — `std::tanh` output clamp | `FastMath::softClip()` |
| **OCELOT** | `OcelotFloor.h` — local SVF (`std::sin` prewarp) | `CytomicSVF` |
| **OCELOT** | `EcosystemMatrix.h` — local sigmoid (`1/(1+std::exp(-x))`) | `FastMath` candidate |
| **OVERLAP** | Local SVF with `std::tan` prewarp | `CytomicSVF` |
| **BOB** | Local SVF with `std::tan` prewarp | Could use `CytomicSVF` (but established engine — evaluate carefully) |
| **FAT** | FatSVF with coefficient caching | Could use `CytomicSVF` (but has unique dirty-flag optimization) |
| **OBSIDIAN** | Local cosine LUT (line 404) | `SROTables::cos()` when available |

### Saturator Usage

```
Using Saturator.h:  2 / 34 engines (ORBITAL, ORPHICA)
Local saturation:   ~6 engines use std::tanh or manual waveshaping
```

---

## FastMath / Kernel Adoption

```
Using FastMath:     32 / 34 engines
Not using FastMath:  2 engines — OVERLAP, OVERWORLD
```

**OVERLAP and OVERWORLD** were late additions (Phase 3 adapters) and missed FastMath integration entirely. OVERLAP also has no CytomicSVF.

---

## FLEET SUMMARY

```
                    INTEGRATED    MISSING
SilenceGate:           0           34      ← PRIORITY 1
ControlRateReducer:    0           34      ← PRIORITY 2
SROTables:             0           34      ← PRIORITY 3
FastMath:             32            2      ← GOOD (fix OVERLAP, OVERWORLD)
CytomicSVF:           22           12      ← ACCEPTABLE (some engines don't need SVF)
Saturator:             2           ~6      ← MODERATE (local tanh in hot paths)
```

---

## TOP 10 OPTIMIZATION TARGETS

Ranked by estimated CPU savings potential:

| Rank | Engine | Issue | Action | Est. Savings |
|------|--------|-------|--------|-------------|
| 1 | **ALL 34** | No SilenceGate | Integrate SilenceGate fleet-wide | **100% idle CPU eliminated** |
| 2 | **OCELOT** | 35 std:: calls, 6× per-sample `std::tanh`, local SVF, local wavefolder | Replace std::tanh → fastTanh, local SVF → CytomicSVF, WaveFolder → Saturator | **~25-30%** |
| 3 | **OHM** | 6× per-sample `std::sin` for FM oscillators, no CytomicSVF, no FastMath freq | Replace std::sin → fastSin or LUT for FM core | **~20-25%** |
| 4 | **OPAL** | 21 std:: calls, grain windows use `std::cos` per-grain, duplicate DSP in OpalDSP.h + OpalEngine.h | LUT grain windows, deduplicate OpalDSP/OpalEngine | **~15-20%** |
| 5 | **OCTOPUS** | Per-sample `std::sin` × 3 per arm (up to 8 arms × harmonics) | LUT for additive synthesis oscillators | **~15-20%** |
| 6 | **ORCA** | Per-sample `std::sin` × 2 for wavetable+harmonics | LUT for additive synthesis | **~10-15%** |
| 7 | **OWLFISH** | Local FastMath + CytomicSVF duplicates, 11 std:: calls | Delete local copies, use kernel | **~10%** (code health) |
| 8 | **OVERLAP** | No FastMath, no CytomicSVF, local SVF with `std::tan`, 7 std:: calls | Full kernel integration | **~10-15%** |
| 9 | **ORBITAL** | Per-partial `std::pow/exp` (32 partials), Gaussian per-partial | LUT for partial tilt, cache Gaussian | **~10%** |
| 10 | **ORIGAMI** | Per-voice spectral pan `std::cos/sin`, FFT twiddle per-frame | Cache spectral pan, precompute twiddles | **~5-10%** |

---

## RECOMMENDATIONS

### Immediate (High Impact, Low Risk)

1. **SilenceGate fleet-wide rollout** — 6 lines per engine adapter. The single biggest win. Every engine that isn't sounding currently burns full CPU.

2. **OCELOT triage** — 35 std:: calls across 7 DSP files. Replace `std::tanh` → `fastTanh` in WaveFolder.h, OcelotVoice.h, OcelotVoicePool.h. Replace local SVF in OcelotFloor.h → CytomicSVF.

3. **OHM FM oscillators** — 6× `std::sin` per sample for Dad's instruments. Replace with `fastSin` or sin LUT. This is a Constellation engine running FM synthesis on raw `std::sin` — the savings are dramatic.

### Short-Term (Moderate Impact)

4. **OWLFISH dedup** — Delete `OwlfishFastMath.h` and `OwlfishCytomicSVF.h`, point imports to kernel. Zero behavioral change, pure code health.

5. **OVERLAP + OVERWORLD kernel integration** — Both late additions missing FastMath. OVERLAP also needs CytomicSVF to replace its local SVF.

6. **Additive synthesis LUTs** — OCTOPUS, ORCA, OHM, OPAL all run `std::sin` per-partial per-sample. A shared sin LUT would lift all four simultaneously.

### Phase 2 (ControlRateReducer)

7. **ControlRateReducer for slow coupling types** — All 34 engines process coupling at audio rate. LFO→pitch, env→morph, amp→filter coupling should be decimated to 1/32.

---

## THE AUDITOR'S VERDICT

> *"34 engines. Zero SilenceGate. Zero ControlRateReducer. Zero LUTs. Every silent slot burns full CPU. Every LFO coupling recalculates 48,000 times per second. Every FM oscillator calls std::sin when a table would do.*
>
> *The fleet sounds extraordinary. The fleet runs like it's never heard of efficiency.*
>
> *The good news: FastMath is in 32 of 34 engines, CytomicSVF in 22. The foundation is there. The SRO components exist and are tested. The integration pattern is 6 lines per engine.*
>
> *The fleet doesn't need a rebuild. It needs a rollout."*

---

*Generated by SRO Fleet Audit — `Source/DSP/SRO/SROAuditor.h`*
*Next audit: post-SilenceGate integration*
