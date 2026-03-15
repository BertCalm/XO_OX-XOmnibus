# Skill: /dna-designer

**Invoke with:** `/dna-designer`
**Status:** LIVE
**Purpose:** Assign accurate, musically meaningful 6D Sonic DNA values to XOmnibus presets — covering brightness, warmth, movement, density, space, and aggression with consistent methodology across all 31 engines.

---

## When to Use This Skill

Use this skill when:
- Writing new `.xometa` presets that need DNA values
- Running `python3 Tools/add_missing_dna.py` is not precise enough
- A preset's DNA feels wrong (wrong "Find Similar" results, wrong mood map position)
- Auditing a batch of presets for DNA coverage gaps
- A new engine needs DNA calibration guidelines

---

## The 6 Dimensions: Definition + Measurement

### 1. `brightness` — Spectral High-End Content

**What it measures:** How much high-frequency energy is present. Not just filter position — also harmonics, presence, air, sparkle.

| Value | Characteristic Sounds |
|-------|----------------------|
| 0.0–0.2 | Sub bass only, no harmonics, very dark |
| 0.2–0.4 | Warmly filtered, muffled, velvet darkness |
| 0.4–0.6 | Balanced midrange, "a face in the crowd" |
| 0.6–0.8 | Open, clear, some shimmer |
| 0.8–1.0 | Air, sparkle, glassy high end, very bright |

**How to estimate:**
1. Where is the filter cutoff? (0–20kHz → 0–1 linear, then apply harmonic content modifier)
2. Does the engine add harmonics via FM, saturation, or wavetable? (+0.1 to +0.2)
3. Does the sound have a prominent transient attack? (+0.05 to +0.15)

**Engine-specific calibration:**
- ONSET: Kick presets typically 0.15–0.35; hi-hat presets 0.65–0.90
- ODDFELIX: Most presets 0.45–0.85 (high-harmonic percussive)
- ODYSSEY: Can span full range based on journey position
- ORGANON: Entropy drives brightness up; low entropy = 0.2–0.5

---

### 2. `warmth` — Saturation, Sub, and Roundness

**What it measures:** Presence of low harmonics, analog saturation/tape character, slow organic drift, sub content, fundamental weight.

| Value | Characteristic Sounds |
|-------|----------------------|
| 0.0–0.2 | Cold, digital precision, clinical |
| 0.2–0.4 | Mildly warm, some coloration |
| 0.4–0.6 | Balanced warmth, "sounds like a real instrument" |
| 0.6–0.8 | Distinctly warm, saturated, analog-feeling |
| 0.8–1.0 | Thick, saturated, tube-warm, vintage |

**How to estimate:**
1. Is there drive/saturation active? (0.0 off → up to +0.4 for heavy drive)
2. Is there sub bass or fundamental weight? (+0.1 to +0.2)
3. Are LFOs slow (< 0.1 Hz)? Slow drift feels warmer (+0.1)
4. Is the filter closed to warm frequencies? Filter in the 40–60% cutoff range (typical for warm pads) adds +0.1 to +0.2 to warmth.

**Engine-specific:**
- OVERDUB: Tape delay adds warmth (+0.15); Spring Reverb adds warmth (+0.1)
- OBLONG: CuriosityEngine at low rates creates warm organic character
- OBESE: Heavy sampler character has high warmth baseline

---

### 3. `movement` — Temporal Activity

**What it measures:** How much the sound changes over time. LFO depth × LFO speed, sequence density, auto-modulation, filter sweeps, tremolo.

| Value | Characteristic Sounds |
|-------|----------------------|
| 0.0–0.1 | Static, frozen in time, no modulation |
| 0.1–0.3 | Subtle breathing, gentle drift |
| 0.3–0.5 | Clearly moving, LFO audible |
| 0.5–0.7 | Actively modulating, rhythmic motion |
| 0.7–1.0 | Constantly shifting, never the same twice |

**Formula:**
```
movement = (lfo1Depth × min(lfo1Rate / 5.0, 1.0) × 0.4)
          + (lfo2Depth × min(lfo2Rate / 5.0, 1.0) × 0.3)
          + (envFilterDepth × 0.2)
          + (sequencer active? 0.3 : 0)
          + (coupling active? 0.1 : 0)
```

**Special cases:**
- OPTIC AutoPulse: if active, movement ≥ 0.5
- OUROBOROS: Lorenz/Aizawa attractor = movement 0.6–0.9 always
- ORGANON: High entropy = movement 0.6+
- Entangled mood: movement should never be below 0.4

---

### 4. `density` — Mass and Layering

**What it measures:** How "thick" or "full" the sound is. Polyphony stacking, unison count, oscillator layering, detuning spread.

| Value | Characteristic Sounds |
|-------|----------------------|
| 0.0–0.2 | Solo monophonic, very thin, single strand |
| 0.2–0.4 | Light — 2-voice, minimal unison |
| 0.4–0.6 | Moderate — 4-voice, some unison/detune |
| 0.6–0.8 | Thick — many voices, wide unison |
| 0.8–1.0 | Massive — max polyphony, extreme unison, full stacking |

**Formula:**
```
density = (voiceCount / maxVoices × 0.4)
         + (unisonCount / 4 × 0.3)
         + (detuneWidth > 20 ? 0.2 : detuneWidth / 100 × 0.1)
         + (dual engine preset? +0.1 : 0)
         + (3+ engine preset? +0.2 : 0)
```

**Note:** Coupled presets using 2 engines add +0.1 to density by default.

---

### 5. `space` — Depth and Distance

**What it measures:** Perceived acoustic space — reverb tail, delay depth, width, the feeling of physical distance.

| Value | Characteristic Sounds |
|-------|----------------------|
| 0.0–0.1 | Completely dry, intimate, in-your-face |
| 0.1–0.3 | Room treatment, small space |
| 0.3–0.5 | Medium hall, moderate delay |
| 0.5–0.7 | Large hall, significant echo |
| 0.7–1.0 | Cathedral, vast space, infinite trails |

**Formula:**
```
space = (reverbMix × 0.6) + (delayFeedback × 0.3) + (stereoWidth > 0.7 ? 0.1 : 0)
```

**Special cases:**
- OVERDUB Spring Reverb: inherently spacious (+0.1 baseline)
- OSPREY/OSTERIA ShoreSystem: wide stereo = +0.1 to space
- Short attack + dry = space ≤ 0.2 (Foundation mood typical)

---

### 6. `aggression` — Edge and Attack

**What it measures:** Sonic harshness, edge, punch, distortion, attack transients, resonance peaks, abrasive qualities.

| Value | Characteristic Sounds |
|-------|----------------------|
| 0.0–0.1 | Velvet smooth, no edge at all |
| 0.1–0.3 | Gentle character, soft focus |
| 0.3–0.5 | Moderate edge, some bite |
| 0.5–0.7 | Clearly aggressive, defined transients |
| 0.7–1.0 | Harsh, distorted, industrial, teeth |

**Formula:**
```
aggression = (resonance × 0.3)
            + (driveAmount × 0.3)
            + (attackTime < 0.005 ? 0.3 : attackTime < 0.03 ? 0.15 : 0)
            + (noiseContent > 0.3 ? 0.1 : 0)
```

**Special cases:**
- OVERBITE BITE macro maxed: aggression 0.7+
- OUROBOROS at resonance: aggression 0.6–0.9
- OBLONG curiosity/feral mode: aggression can reach 0.5
- Atmosphere pads should rarely exceed 0.4

---

## Mood Profile Templates

Use these as starting reference ranges — fine-tune based on the actual sound:

| Mood | brightness | warmth | movement | density | space | aggression |
|------|-----------|--------|----------|---------|-------|-----------|
| Foundation | 0.2–0.6 | 0.3–0.8 | 0.1–0.5 | 0.4–0.9 | 0.0–0.3 | 0.2–0.9 |
| Atmosphere | 0.2–0.7 | 0.3–0.9 | 0.2–0.7 | 0.1–0.5 | 0.4–1.0 | 0.0–0.3 |
| Entangled | 0.3–0.8 | 0.2–0.8 | **0.4–1.0** | 0.3–0.8 | 0.2–0.8 | 0.1–0.7 |
| Prism | 0.4–0.9 | 0.1–0.6 | 0.1–0.6 | 0.2–0.6 | 0.1–0.5 | 0.1–0.6 |
| Flux | 0.3–0.9 | 0.0–0.5 | **0.5–1.0** | 0.2–0.7 | 0.1–0.6 | **0.4–1.0** |
| Aether | 0.2–0.8 | 0.2–0.7 | 0.4–0.9 | 0.1–0.5 | **0.5–1.0** | 0.0–0.3 |

**Bold** = dimension that defines the mood character.

---

## Coverage Gap Detection

After writing a batch of presets, check coverage against the fleet standards:

```bash
python3 Tools/audit_sonic_dna.py
```

A **high-end gap** = max value for a dimension across all presets for an engine < 0.70
A **low-end gap** = min value for a dimension across all presets for an engine > 0.30

To fix:
- High-end gap in `brightness` → write an airy, open, high-filter preset
- Low-end gap in `aggression` → write a smooth, velvet-soft preset (aggression = 0.05–0.15)
- High-end gap in `movement` → write a heavily modulated, active preset
- Low-end gap in `movement` → write a static, frozen texture

---

## Automated Tools

| Task | Command |
|------|---------|
| Auto-compute DNA from tags + context | `python3 Tools/add_missing_dna.py` |
| Audit existing DNA coverage | `python3 Tools/audit_sonic_dna.py` |
| Compute DNA for a specific preset | `python3 Tools/compute_preset_dna.py --preset path.xometa` |
| Find presets with missing DNA | `python3 Tools/find_missing_dna.py` |
| Breed two presets (DNA-aware) | `python3 Tools/breed_presets.py` |

The automated tools (`add_missing_dna.py`) apply keyword-driven heuristics. They are useful for batch work but often less precise than hand-crafted DNA for emotionally important presets. Use manual DNA for all showcase/hero presets.
