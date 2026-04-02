# Bob (Oblong) Aggression Expansion

**Date:** 2026-03-14
**Triggered by:** Sonic DNA Audit Round 7G finding: Oblong aggression max = 0.65 across 347 presets.
**Action:** Write 10 high-aggression presets to push the dimension ceiling above 0.75.

---

## The DNA Gap

Before this expansion:

| Dimension | Range (347 presets) | Gap |
|-----------|---------------------|-----|
| aggression | 0.00–0.65 | max < 0.70 — high-end gap flagged |

The Oblong engine (XOblongBob) has a full drive system: `bob_fltDrive` (0–1.0) applies softclip gain up to 5× pre-filter, `bob_fltChar` adds character saturation to the resonance tail, `bob_bobMode` fans out to five targets simultaneously (drift, filterChar, texLevel, modDepth, fxDepth). All existing presets left these parameters in the 0.0–0.15 range for `bob_fltDrive` and 0.4–0.6 range for `bob_fltChar`. No preset had touched the real ceiling.

---

## High-Aggression Parameter Space

The following parameter combinations produce the most aggressive Oblong sounds:

| Parameter | Normal range (existing) | Aggressive range |
|-----------|------------------------|-----------------|
| `bob_fltDrive` | 0.0–0.15 | 0.78–1.0 |
| `bob_fltChar` | 0.4–0.65 | 0.85–1.0 |
| `bob_fltReso` | 0.3–0.55 | 0.65–0.92 |
| `bob_bobMode` | 0.0–0.55 | 0.65–1.0 |
| `bob_dustAmount` | 0.0–0.1 | 0.4–0.7 |
| `bob_oscB_sync` | mostly Off | On for harmonic shredding |
| `bob_oscB_fm` | 0.0 | 0.22–0.65 for digital crunch |
| `bob_texMode` / `bob_texLevel` | Blanket/0.0 | Static/0.3–0.75 for noise edge |
| `bob_fltMode` | mostly LP (0) | BP (1) for resonant bite |
| `bob_ampSustain` | 0.55–0.75 | 0.0 for percussive burst |
| `bob_curMode` | Sniff/Wander | Twitch (3) for chaos |

---

## 10 Presets Written

### Flux Mood

| Preset | File | Target DNA |
|--------|------|------------|
| Industrial Clank | `Presets/XOceanus/Flux/Bob_Industrial_Clank.xometa` | B=0.75 W=0.15 M=0.55 D=0.45 S=0.20 **A=0.90** |
| Fuzz Bass | `Presets/XOceanus/Flux/Bob_Fuzz_Bass.xometa` | B=0.42 W=0.72 M=0.35 D=0.55 S=0.20 **A=0.87** |
| Glitch Stutter | `Presets/XOceanus/Flux/Bob_Glitch_Stutter.xometa` | B=0.70 W=0.18 M=0.92 D=0.52 S=0.15 **A=0.93** |
| Acid Drive | `Presets/XOceanus/Flux/Bob_Acid_Drive.xometa` | B=0.62 W=0.38 M=0.80 D=0.40 S=0.18 **A=0.88** |
| Noise Burst | `Presets/XOceanus/Flux/Bob_Noise_Burst.xometa` | B=0.85 W=0.12 M=0.45 D=0.62 S=0.15 **A=0.95** |
| Terror Sub | `Presets/XOceanus/Flux/Bob_Terror_Sub.xometa` | B=0.25 W=0.78 M=0.42 D=0.52 S=0.22 **A=0.85** |

### Prism Mood

| Preset | File | Target DNA |
|--------|------|------------|
| Max Fold | `Presets/XOceanus/Prism/Bob_Max_Fold.xometa` | B=0.88 W=0.20 M=0.65 D=0.60 S=0.25 **A=0.97** |
| Growl Lead | `Presets/XOceanus/Prism/Bob_Growl_Lead.xometa` | B=0.68 W=0.35 M=0.78 D=0.48 S=0.28 **A=0.86** |
| Crunch Layer | `Presets/XOceanus/Prism/Bob_Crunch_Layer.xometa` | B=0.72 W=0.32 M=0.50 D=0.88 S=0.30 **A=0.88** |
| Distorted Pad | `Presets/XOceanus/Prism/Bob_Distorted_Pad.xometa` | B=0.58 W=0.42 M=0.45 D=0.55 S=0.48 **A=0.79** |

---

## After Expansion

| Dimension | Range (347 → 357 presets) | Gap |
|-----------|---------------------------|-----|
| aggression | 0.00–**0.97** | None — high-end gap resolved |

The aggression ceiling moves from 0.65 to 0.97. The gap flag (max < 0.70) is fully resolved.
7 of 10 presets have aggression ≥ 0.85. The lowest in the batch is Distorted Pad at 0.79 — still well above the former ceiling.

---

## Preset Character Notes

**Industrial Clank** — Cushion Pulse (wave 3) with narrow width (shape=0.82) through Sync+BP filter. Short percussive envelope (0 sustain, 180ms decay). The metallic ping of a steel beam. Most useful for rhythmic clank sequences.

**Fuzz Bass** — Velvet Saw (wave 2) + Sub Harmonic (OscB wave 3) together through LP at 1800Hz with drive=0.92. Warm but vicious. The sub gives body; the drive shreds it. Mono with 80ms glide.

**Glitch Stutter** — S&H LFO at 9.5Hz shredding the filter, Twitch curiosity at full amount (0.95), Static texture, FM from OscB. Digital, chaotic, non-repeating. No two notes identical.

**Acid Drive** — Near-self-oscillating BP filter (reso=0.88), fast motion envelope depth=0.95, drive=0.82. The BP mode creates a sharp resonant peak that the motion envelope sweeps upward on every note. Mono for proper acid behavior.

**Noise Burst** — Static texture at 0.72, Cushion Pulse (wave 3) with max shape width. Zero sustain envelope — pure transient. Sync + FM together. This is the loudest transient in the Bob catalog.

**Terror Sub** — Sub Harmonic dominant (OscB blend=0.85), LP at 600Hz with drive=0.90. Slow triangle LFO at 0.15Hz adds a menacing tremor. Warm but aggressive — the weight of the drive is felt more than heard.

**Max Fold** — `bob_bobMode=1.0` fans all 5 macro targets to maximum simultaneously. Velvet Saw, FM=0.55, S&H LFO at 3.5Hz, Twitch curiosity, drive=1.0, char=1.0, Static texture. This is the ceiling of what the Oblong engine can produce. Aggression 0.97.

**Growl Lead** — FM between oscillators (fm=0.65) with Investigate curiosity (perpetual slow filter morph). LP instead of BP keeps the body in. The FM creates inharmonic sidebands that morph with the filter. Mono with glide for lead playing.

**Crunch Layer** — 8-voice poly (polyphony=3 → 8 voices), OscB detune=22 cents, drive=0.88. Every chord is a wall of grinding saws. Smooth random LFO adds micro-movement without disorder.

**Distorted Pad** — Same aggression infrastructure (drive=0.80, char=0.88) but with long attack (350ms) and release (800ms). The aggression is sustained and breathes. Wander curiosity at 0.50 adds slow filter drift. The one aggressive preset you can hold.

---

## Technique Reference

Three techniques enabled the aggression ceiling above 0.90:

1. **Drive + Character stack:** `bob_fltDrive` applies softclip at (1 + drive×4)× gain before the filter. `bob_fltChar` saturates the resonance tail. Running both at 0.85+ creates a layered clipping architecture — the signal clips going in, and clips again in the resonance.

2. **BobMode macro at high values:** `bob_bobMode >= 0.7` additively boosts filterChar by +0.63, drift by +0.56, and fxDepth by +0.72. Combined with a high base char value, the effective character exceeds 1.0 (clamped at 1.0). This maxes out all saturation paths simultaneously.

3. **BP filter mode (fltMode=1):** The Snout BP filter runs the resonance output through an additional `softClip(bp * (1 + res), char * 0.4)` stage. At high resonance (0.85+) and high character (0.9+), this stage produces a sharp, biting transient on each note — the "clank" character that gives the engine its most extreme sounds.

---

*Expansion by XO_OX Designs. Re-run `python3 Tools/audit_sonic_dna.py` to confirm gap closure.*
