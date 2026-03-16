# XPN Sonic DNA Automation — R&D Spec

**Date**: 2026-03-16
**Status**: Research / Pre-implementation
**Topic**: Automatically computing 6D Sonic DNA values from rendered WAV audio

---

## Background

XOmnibus presets carry a 6D Sonic DNA vector: `brightness`, `warmth`, `movement`, `density`, `space`, `aggression`. Each dimension is a float 0.0–1.0. Currently these are assigned manually by a sound designer listening to each preset — accurate but slow, and prone to inconsistency across the fleet (2,369 presets and growing).

The XPN tools already include `xpn_classify_instrument.py`, which performs DFT-based spectral analysis on raw PCM. It already outputs `spectral_centroid_hz`, `hf_energy_ratio`, and `transient_strength`. This R&D proposes building `xpn_auto_dna.py` on top of that engine to derive all six dimensions algorithmically.

---

## Dimension Analysis

### 1. Brightness

**Formula**: `spectral_centroid_hz / 10000.0`, clamped 0.0–1.0.

Spectral centroid is the frequency-weighted center of spectral mass — the single most reliable proxy for perceived brightness. A centroid at 10 kHz maps to 1.0; sub-bass content near 100 Hz maps to ~0.01.

**Source**: `compute_spectral_centroid_hz()` in `xpn_classify_instrument.py` — already computed.
**Audio-only**: Yes.
**Confidence**: HIGH. Spectral centroid vs. human brightness ratings is one of the most validated correlations in MIR literature.

---

### 2. Warmth

**Formula**: `1.0 - hf_energy_ratio`, where `hf_energy_ratio` is the fraction of spectral energy above 4 kHz.

Warmth is the perceptual complement of harshness/air. A signal with most energy below 4 kHz (cello body resonance, low-pass filtered pads, analog bass) reads as warm. High HF energy (hi-hats, bright leads, crispy transients) reads as cold.

**Source**: `compute_hf_energy_ratio()` in `xpn_classify_instrument.py` — already computed.
**Audio-only**: Yes.
**Confidence**: HIGH. The brightness/warmth inverse relationship is well-established; the 4 kHz cutoff may need tuning per instrument category.

---

### 3. Movement

**Formula**: Standard deviation of spectral centroid across 10 equal-length time windows, normalized by `max_centroid_hz`.

Render the sample into 10 non-overlapping windows. Compute spectral centroid per window. High variance = centroid sweeping (filter envelopes, LFOs, pitch modulation). Low variance = static timbre (sustained pad, sine tone, held string).

`movement = min(std(centroids) / 2000.0, 1.0)`

The denominator 2000 Hz is a calibration constant: a centroid swinging ±2 kHz across windows represents "fully moving." Tune against a test set of LFO-heavy presets vs. drones.

**Audio-only**: Yes. This is purely temporal spectral analysis.
**Confidence**: MEDIUM. Works well for filter sweeps and pitch LFOs. Underestimates amplitude-only modulation (tremolo without spectral change). Phase 2 could add RMS variance as a secondary signal.

---

### 4. Density

**Formula**: Count FFT bins whose magnitude exceeds `peak_magnitude - 40 dB`, divide by total bin count.

`density = active_bins / total_bins`

A sine wave has one active bin — density near 0. A dense pad chord or noise texture will have hundreds of active bins across the full spectrum — density near 1.

Use a single DFT window of 4096 samples taken from the sustain region (skip first 50 ms to clear the transient). The -40 dB threshold relative to the peak bin is calibrated to human perception of "filled-in" frequency content.

**Audio-only**: Yes.
**Confidence**: MEDIUM. Accurate for harmonic richness and noise content. Can be fooled by heavily compressed signals where everything is near-peak level; add a noise floor guard at -80 dBFS absolute.

---

### 5. Space

**Formula**: Measure time for RMS to decay from peak value to -60 dB. Map: `min(decay_seconds / 10.0, 1.0)`.

A 10-second decay (long reverb tail, sustained pad) maps to 1.0. A dry percussion hit with sub-100 ms decay maps near 0.01.

Implementation: compute RMS in 10 ms sliding windows from the sample start. Find the window with peak RMS. Walk forward until RMS drops 60 dB from peak. Record elapsed time.

**Audio-only**: Yes, as long as the rendered WAV includes the full reverb tail. Render must be long enough (at least 10 seconds beyond note-off) or this underestimates space for long reverb presets.
**Confidence**: MEDIUM-HIGH. Direct physical measurement of decay time. Caveat: space perception also involves early reflections and stereo width, which this formula ignores. A stereo width term could be added as a Phase 2 refinement.

---

### 6. Aggression

**Formula**: `0.6 * transient_strength + 0.4 * hf_energy_ratio`

Aggression combines two independent physical correlates: the sharpness of the attack (transient strength = peak/RMS ratio, normalized) and the harshness of sustained frequency content (HF energy). A kick drum: high transient, low HF → mid aggression. A distorted lead: moderate transient, very high HF → high aggression. A soft pad: both low → near 0.

**Source**: Both components already computed in `xpn_classify_instrument.py`.
**Audio-only**: Yes.
**Confidence**: HIGH for percussive content. MEDIUM for sustained synths where subjective "aggression" is more culturally coded (a growling bass vs. a gentle string — both can have similar transient_strength).

---

## Proposed Tool: `xpn_auto_dna.py`

**Location**: `Tools/xpn_auto_dna.py`
**Dependencies**: `xpn_classify_instrument.py` (already in Tools/), Python stdlib only (struct, math)

### Interface

```
python xpn_auto_dna.py <wav_or_directory> [--output json|yaml|inline]
```

Single WAV → prints DNA dict to stdout.
Directory → scans recursively for `.wav` files, outputs one DNA dict per file as JSONL.

### Output format

```json
{
  "file": "MyPreset_C3.wav",
  "sonic_dna": {
    "brightness": 0.42,
    "warmth": 0.71,
    "movement": 0.18,
    "density": 0.55,
    "space": 0.34,
    "aggression": 0.29
  },
  "confidence": {
    "brightness": "high",
    "warmth": "high",
    "movement": "medium",
    "density": "medium",
    "space": "medium",
    "aggression": "high"
  }
}
```

### Implementation sketch (~100 lines)

```python
# xpn_auto_dna.py — Auto-compute 6D Sonic DNA from WAV audio
# Depends on: xpn_classify_instrument.py (imports analyze_wav_file)

import sys, os, json, math, struct
from xpn_classify_instrument import (
    read_wav_mono, compute_dft_magnitudes,
    compute_spectral_centroid_hz, compute_hf_energy_ratio,
    compute_transient_strength
)

BRIGHTNESS_NORM  = 10000.0   # Hz — centroid at 10kHz = 1.0
MOVEMENT_NORM    = 2000.0    # Hz std dev calibration
DENSITY_FLOOR_DB = -40.0     # dB below peak bin to count as "active"
SPACE_NORM_S     = 10.0      # seconds — full decay = 1.0
AGGRESSION_W     = (0.6, 0.4) # transient, hf weights

N_MOVEMENT_WINDOWS = 10
DFT_SIZE = 4096

def compute_movement(mono, sample_rate):
    n = len(mono)
    window_size = n // N_MOVEMENT_WINDOWS
    if window_size < DFT_SIZE:
        return 0.0  # sample too short to measure
    centroids = []
    for i in range(N_MOVEMENT_WINDOWS):
        chunk = mono[i * window_size : i * window_size + DFT_SIZE]
        mags = compute_dft_magnitudes(chunk)
        centroids.append(compute_spectral_centroid_hz(mags, sample_rate))
    mean = sum(centroids) / len(centroids)
    variance = sum((c - mean) ** 2 for c in centroids) / len(centroids)
    std = math.sqrt(variance)
    return min(std / MOVEMENT_NORM, 1.0)

def compute_density(mono, sample_rate):
    # Use sustain region: skip first 50ms
    skip = int(sample_rate * 0.05)
    chunk = mono[skip : skip + DFT_SIZE]
    if len(chunk) < DFT_SIZE:
        chunk = mono[:DFT_SIZE]
    mags = compute_dft_magnitudes(chunk)
    if not mags or max(mags) == 0:
        return 0.0
    peak = max(mags)
    threshold = peak * (10 ** (DENSITY_FLOOR_DB / 20.0))
    active = sum(1 for m in mags if m >= threshold)
    return active / len(mags)

def compute_space(mono, sample_rate):
    window_samples = int(sample_rate * 0.01)  # 10ms windows
    n = len(mono)
    rms_windows = []
    for i in range(0, n - window_samples, window_samples):
        chunk = mono[i : i + window_samples]
        rms = math.sqrt(sum(x * x for x in chunk) / len(chunk))
        rms_windows.append(rms)
    if not rms_windows or max(rms_windows) == 0:
        return 0.0
    peak_rms = max(rms_windows)
    threshold = peak_rms * (10 ** (-60.0 / 20.0))
    peak_idx = rms_windows.index(peak_rms)
    decay_windows = 0
    for rms in rms_windows[peak_idx:]:
        if rms < threshold:
            break
        decay_windows += 1
    decay_seconds = decay_windows * 0.01
    return min(decay_seconds / SPACE_NORM_S, 1.0)

def auto_dna(wav_path):
    mono, sample_rate = read_wav_mono(wav_path)
    mags = compute_dft_magnitudes(mono[:DFT_SIZE])
    centroid = compute_spectral_centroid_hz(mags, sample_rate)
    hf_ratio  = compute_hf_energy_ratio(mags, sample_rate)
    transient = compute_transient_strength(mono, sample_rate)
    brightness  = min(centroid / BRIGHTNESS_NORM, 1.0)
    warmth      = 1.0 - hf_ratio
    movement    = compute_movement(mono, sample_rate)
    density     = compute_density(mono, sample_rate)
    space       = compute_space(mono, sample_rate)
    aggression  = AGGRESSION_W[0] * transient + AGGRESSION_W[1] * hf_ratio
    return {k: round(v, 3) for k, v in {
        "brightness": brightness, "warmth": warmth,
        "movement": movement, "density": density,
        "space": space, "aggression": aggression
    }.items()}
```

---

## Integration Path

1. **Fleet render pass**: Render each preset to a 10-second WAV at C3 sustain. Store in `Renders/DNA/`.
2. **Auto-DNA run**: `python xpn_auto_dna.py Renders/DNA/ --output jsonl > fleet_dna.jsonl`
3. **Calibration**: Manually verify 50 presets against human-assigned DNA. Adjust normalizers (`BRIGHTNESS_NORM`, `MOVEMENT_NORM`, etc.) until mean absolute error < 0.1 per dimension.
4. **Human override layer**: Auto values become defaults. Sound designer can override any dimension in the preset JSON; tool detects presence of existing DNA and skips (or flags as `"source": "manual"`).
5. **CI integration**: On new preset batch, run auto-DNA as part of the XPN packaging pipeline.

---

## Open Questions

- **Stereo width for space**: Should `space` incorporate a stereo field measurement (mid/side energy ratio)? Would catch wide reverbs that decay quickly in amplitude but read as spacious.
- **Movement calibration**: 2000 Hz std dev as "fully moving" is a guess. Needs empirical validation against known LFO-heavy presets.
- **Density on pads**: Heavily layered pads may report low density if they are band-limited. A per-octave bin count rather than flat bin count may correlate better with perceived density on sustained material.
- **Render protocol**: Decay-time measurement requires silence after note-off. Fleet render spec must enforce a minimum 8-second tail after note release.
