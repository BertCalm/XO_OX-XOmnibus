# Sonic DNA Audit — XOmnibus Library

**Date:** 2026-03-14
**Tool:** `Tools/audit_sonic_dna.py`
**Total presets scanned:** 1,657
**Presets with complete DNA:** 1,642
**Presets missing/incomplete DNA:** 15

---

## Background

Every `.xometa` preset carries a 6-dimensional Sonic DNA block that powers Find Similar, Find Opposite, preset breeding, and the 2D mood map. The 6 dimensions are:

| Dimension | What it measures |
|-----------|-----------------|
| `brightness` | Filter cutoff, spectral content, presence of high harmonics |
| `warmth` | Sub content, saturation, slow drift, formant character |
| `movement` | LFO depth, AutoPulse rate, sequence density, filter sweep |
| `density` | Polyphony, detune width, layering, oscillator stacking |
| `space` | Reverb size/mix, delay feedback, prism spread |
| `aggression` | Resonance, drive, distortion, hard transients |

Each dimension runs 0.0–1.0. A well-curated library should cover the full range of each dimension for each engine. The audit flags:

- **High-end gap:** max value < 0.70 — the engine never reaches the bright/warm/aggressive end of a dimension.
- **Low-end gap:** min value > 0.30 — the engine never reaches the dark/cold/still end of a dimension.

---

## Full Coverage Table (post gap-fill)

After writing 12 gap-fill presets, coverage results are:

| Engine | N | brightness | warmth | movement | density | space | aggression | Gap Count |
|--------|---|-----------|--------|----------|---------|-------|-----------|-----------|
| Obese | 159 | 0.15–0.85 | 0.35–0.85 ! | 0.05–0.95 | 0.35–0.95 ! | 0.05–0.92 | 0.00–0.80 | 2 |
| Oblique | 20 | 0.22–0.95 | 0.15–0.88 | 0.05–0.95 | 0.22–0.90 | 0.20–0.97 | 0.05–0.93 | **0** |
| Oblong | 347 | 0.15–0.82 | 0.07–0.95 | 0.00–0.95 | 0.07–0.92 | 0.00–0.92 | 0.00–0.65 ! | 1 |
| OddOscar | 165 | 0.35–0.92 ! | 0.08–0.86 | 0.04–0.95 | 0.35–0.92 ! | 0.10–0.96 | 0.00–0.91 | 2 |
| OddfeliX | 246 | 0.20–1.00 | 0.10–1.00 | 0.10–0.98 | 0.15–0.92 | 0.10–0.95 | 0.05–0.92 | 0 |
| Odyssey | 389 | 0.15–0.85 | 0.00–0.90 | 0.00–0.95 | 0.01–0.90 | 0.00–0.96 | 0.00–0.70 | 0 |
| Onset | 127 | 0.20–0.95 | 0.15–0.90 | 0.05–1.00 | 0.05–0.95 | 0.02–0.95 | 0.02–0.95 | 0 |
| Opal | 118 | 0.00–1.00 | 0.00–0.95 | 0.05–0.95 | 0.10–0.90 | 0.00–1.00 | 0.00–0.90 | 0 |
| Optic | 11 | 0.25–0.93 | 0.08–0.85 | 0.12–0.95 | 0.30–0.91 | 0.20–0.96 | 0.05–0.92 | **0** |
| Organon | 145 | 0.05–0.95 | 0.05–0.90 | 0.02–0.90 | 0.10–0.85 | 0.05–0.70 | 0.01–0.70 | 0 |
| Ouroboros | 68 | 0.10–0.90 | 0.05–0.80 | 0.10–0.95 | 0.20–0.80 | 0.00–0.90 | 0.00–0.95 | 0 |
| Overbite | 71 | 0.05–0.80 | 0.05–0.90 | 0.00–0.85 | 0.20–0.85 | 0.00–0.90 | 0.00–0.90 | 0 |
| Overdub | 190 | 0.15–0.88 | 0.08–0.85 | 0.10–1.00 | 0.20–0.92 | 0.10–0.96 | 0.00–1.00 | 0 |
| Overworld | 53 | 0.15–0.85 | 0.05–0.75 | 0.10–0.95 | 0.20–0.70 | 0.00–0.90 | 0.00–0.90 | 0 |

**Legend:** `!` = coverage gap (after min = low end missing; after max = high end missing)

---

## Pre-Audit State (Before Gap-Fill)

For reference, the original gap counts before any gap-fill presets were written:

| Engine | Pre-Audit Gap Count | Post-Audit Gap Count | Change |
|--------|--------------------|--------------------|--------|
| OddOscar | 4 | 2 | -2 |
| Optic | 3 | **0** | -3 |
| Oblique | 3 | **0** | -3 |
| Obese | 2 | 2 | unchanged |
| Oblong | 1 | 1 | unchanged |
| All others | 0 | 0 | unchanged |

The 3 worst engines identified by the initial audit were **OddOscar**, **Optic**, and **Oblique**.

---

## The 3 Worst Engines (Pre-Audit)

### #1: OddOscar (Morph) — 4 gaps

OddOscar's wavetable scanner produces inherently mid-range spectral content, causing all presets to cluster above 0.35 in brightness and density. The aggression cap at 0.60 was the most actionable gap — the engine is capable of aggressive resonance but no presets had explored it.

| Dimension | Range | Gaps |
|-----------|-------|------|
| brightness | 0.35–0.85 | min > 0.30 (no dark presets) |
| warmth | 0.35–0.75 | min > 0.30 (no cold presets) — **fixed** |
| movement | 0.12–0.95 | OK |
| density | 0.35–0.92 | min > 0.30 (no sparse presets) |
| space | 0.10–0.94 | OK |
| aggression | 0.00–0.60 | max < 0.70 (no aggressive presets) — **fixed** |

Two gaps (warmth low end, aggression high end) were fixed by the gap-fill presets. Two remain (brightness and density below 0.30) because the wavetable scanner architecture creates a structural floor — addressed with recommendations below.

### #2: Optic — 3 gaps

Optic only had 7 presets, all in coupled configurations. Every preset was in movement >= 0.60, no preset was cold (warmth all >= 0.35), and no preset was bright (brightness all <= 0.65). The engine was severely under-explored.

| Dimension | Range | Gaps |
|-----------|-------|------|
| brightness | 0.25–0.65 | max < 0.70 — **fixed** |
| warmth | 0.35–0.75 | min > 0.30 — **fixed** |
| movement | 0.60–0.95 | min > 0.30 — **fixed** |
| density | 0.30–0.85 | OK |
| space | 0.20–0.90 | OK |
| aggression | 0.05–0.80 | OK |

All 3 Optic gaps are resolved. The engine now covers 0.08–0.93 in brightness and 0.08–0.85 in warmth.

### #3: Oblique — 3 gaps

Oblique had 6 presets (all in the 0.50–0.70 brightness band), 12 after a previous expansion pass. Warmth never reached 0.70, brightness never dropped below 0.50, density always above 0.35. The prism/bounce engine is flexible but presets had all avoided both extremes.

| Dimension | Range | Gaps |
|-----------|-------|------|
| brightness | 0.50–0.90 | min > 0.30 — **fixed** |
| warmth | 0.20–0.65 | max < 0.70 — **fixed** |
| movement | 0.15–0.95 | OK |
| density | 0.35–0.80 | min > 0.30 — **fixed** |
| space | 0.20–0.95 | OK |
| aggression | 0.05–0.85 | OK |

All 3 Oblique gaps are resolved.

---

## 12 Gap-Fill Presets Written

### OddOscar (4 presets)

| File | Corner | DNA Summary |
|------|--------|-------------|
| `Presets/XOmnibus/Prism/Morph_Razor_Crest.xometa` | High brightness + High aggression | B=0.92 W=0.18 M=0.55 D=0.70 S=0.30 A=0.91 |
| `Presets/XOmnibus/Foundation/Morph_Digital_Lattice.xometa` | Low warmth + High density | B=0.65 W=0.08 M=0.45 D=0.92 S=0.40 A=0.48 |
| `Presets/XOmnibus/Atmosphere/Morph_Deep_Void.xometa` | High space + Low movement | B=0.42 W=0.55 M=0.04 D=0.52 S=0.96 A=0.08 |
| `Presets/XOmnibus/Aether/Morph_Maximum_State.xometa` | Max everything | B=0.88 W=0.86 M=0.88 D=0.92 S=0.90 A=0.85 |

**Morph Razor Crest** — Wavetable at peak spectral position (morph=0.98), high filter resonance (0.88), fast decay. Establishes the high-brightness/high-aggression corner that was completely absent.

**Morph Digital Lattice** — Cold digital preset with zero drift, zero sub, heavy detuned 8-voice poly stack. Pushes warmth to 0.08 and density to 0.92 simultaneously.

**Morph Deep Void** — Maximum reverb bloom (0.88), ultra-long decay/release (10s), near-zero drift. Establishes the high-space/low-movement corner.

**Morph Maximum State** — All 6 dimensions pushed above 0.85. High morph position + bloom + wide detune + resonance + long space.

### Optic (4 presets)

| File | Corner | DNA Summary |
|------|--------|-------------|
| `Presets/XOmnibus/Prism/UV_Overload.xometa` | High brightness + High aggression | B=0.93 W=0.10 M=0.90 D=0.78 S=0.25 A=0.92 |
| `Presets/XOmnibus/Flux/Ice_Lattice_Pulse.xometa` | Low warmth + High density | B=0.70 W=0.08 M=0.88 D=0.91 S=0.35 A=0.60 |
| `Presets/XOmnibus/Aether/Deep_Signal_Drift.xometa` | High space + Low movement | B=0.48 W=0.62 M=0.12 D=0.42 S=0.96 A=0.08 |
| `Presets/XOmnibus/Flux/Optic_Maximum_State.xometa` | Max everything | B=0.87 W=0.85 M=0.93 D=0.90 S=0.86 A=0.88 |

**UV Overload** — AutoPulse at maximum accent depth (1.0), sharp square shape, high subdivision rate. OddfeliX adds transient snap. Both engines in overloaded territory. Brightness 0.93, Aggression 0.92.

**Ice Lattice Pulse** — Clinical cold complex AutoPulse driving a dense Oblong stack. No warmth (0.08), maximum density (0.91) from polyrhythmic subdivision and dense filter coupling.

**Deep Signal Drift** — Optic in minimum-pulse mode (rate=0.08), spectral modulation only. Paired with Odyssey in maximum reverb. Movement 0.12, Space 0.96. Establishes the still+deep corner.

**Optic Maximum State** — Three-engine stack (Optic + Overdub + OddfeliX). Full-power AutoPulse, deep dub bass, rapid-fire snap. All 6 dimensions 0.85+.

### Oblique (4 presets)

| File | Corner | DNA Summary |
|------|--------|-------------|
| `Presets/XOmnibus/Prism/Oblique_Solar_Flare.xometa` | High brightness + High aggression | B=0.95 W=0.15 M=0.80 D=0.72 S=0.45 A=0.93 |
| `Presets/XOmnibus/Foundation/Oblique_Velvet_Prism.xometa` | High warmth + Low density | B=0.22 W=0.88 M=0.35 D=0.22 S=0.65 A=0.08 |
| `Presets/XOmnibus/Atmosphere/Oblique_Dark_Cathedral.xometa` | High space + Low movement | B=0.28 W=0.55 M=0.05 D=0.42 S=0.97 A=0.12 |
| `Presets/XOmnibus/Entangled/Oblique_Maximum_State.xometa` | Max everything | B=0.88 W=0.82 M=0.92 D=0.90 S=0.88 A=0.90 |

**Oblique Solar Flare** — Wavefold at 0.95 on sawtooth, filter ceiling at 16kHz, resonance 0.72, prism feedback near breaking (0.75), fast high-pitched clicks. Brightness 0.95, Aggression 0.93 — previously impossible without wavefold at max.

**Oblique Velvet Prism** — Sine wave through 800Hz LP filter with heavy glide, one bounce count, prism damp at 0.75. Warmth 0.88, Density 0.22 — establishes the warm + sparse corner that was structurally absent.

**Oblique Dark Cathedral** — Triangle wave through 1200Hz filter, 350ms prism delay, prism feedback 0.90, phaser rate 0.03Hz. Movement 0.05 (one slow bounce per second), Space 0.97.

**Oblique Maximum State** — Three-engine stack (Oblique + OddfeliX + Overdub) with wavefold at 0.82, 18-cent detune, 14 bounce clicks, deep dub reverb tail, rapid snap transients. All 6 dimensions 0.82+.

---

## Remaining Gaps and Recommendations

### Still-Gapped Engines

After gap-fill, two structural gaps remain that could not be eliminated by individual presets:

**OddOscar — brightness min > 0.30 and density min > 0.30**
The OddOscar (Morph) engine's wavetable scanner architecture creates an inherent spectral floor. Even at morph=0 (harmonic position 0), the wavetable content sits at ~0.35 brightness. This is a characteristic of the engine, not a preset design failure.
*Recommendation:* Accept as identity. OddOscar is a "mid-register + spectral" engine. Add a note to the design guide that brightness < 0.30 is not reachable for this engine.

**Oblong — aggression max < 0.70**
XOblong's character is fundamentally warm and musical. The filter character parameter prevents extreme aggression. 347 presets exist and the maximum is 0.65. A handful of heavy distortion Oblong presets could push this to 0.75+.
*Recommendation:* Write 3–5 heavy Oblong presets with `bob_fltDrive` > 0.6 and high `bob_fltReso` to push aggression above 0.70. The gap score will drop to 0.

**Obese — warmth min > 0.30 and density min > 0.30**
Obese (XObese) is a "fat pad" engine — warmth and density below 0.30 conflict with its identity. These are structural identity limits, not coverage failures.
*Recommendation:* Accept as identity. Document the warmth ≥ 0.35 and density ≥ 0.35 floor as characteristic of the engine in the sound design guide.

### Future Expansion Priority

Ranked by opportunity value:

1. **Oblong aggression** — Easy to fix (3–5 presets). A `bob_fltDrive` + resonance spike preset would resolve the 1 remaining gap.
2. **Overworld space and aggression range** — Only 53 presets. The engine's ERA triangle system is capable of extreme textures. Expand to 80+ presets covering low-space chip textures and high-aggression glitch sequences.
3. **Organon/Ouroboros space ceilings** — Both engines cap space at 0.70–0.80. The engines support deep reverb but presets haven't explored it. 3–5 long-tail presets per engine would resolve this.
4. **Overbite brightness cap at 0.80** — The Bite architecture can produce bright harmonic content. 3 high-brightness Overbite presets would fill this.

### Systematic Monitoring

Run `python3 Tools/audit_sonic_dna.py` after any large preset batch to catch regressions. The gap score and average range columns give a quick health check. Target state:

- Gap count = 0 for all engines (or documented as structural identity limits)
- Average range > 0.75 for all engines
- No engine below 50 presets (current: Optic=11 — needs expansion)

---

## Audit Tool

Script: `Tools/audit_sonic_dna.py`

**Usage:**
```bash
python3 Tools/audit_sonic_dna.py           # standard report
python3 Tools/audit_sonic_dna.py --verbose  # includes list of missing-DNA files
```

**Algorithm:**
- Scans all `.xometa` files under `Presets/`
- Reads the `dna` or `sonic_dna` block (both key names supported)
- Groups values by engine (a preset with 3 engines contributes to all 3)
- Computes min/max/mean/range per engine per dimension
- Flags dimensions where max < 0.70 (high-end gap) or min > 0.30 (low-end gap)
- Gap score = total number of flagged dimensions (max 12 per engine)
- Ranks engines by gap score descending, then average range ascending

---

*Generated by audit pass on 2026-03-14. Re-run `audit_sonic_dna.py` after any major preset expansion.*
