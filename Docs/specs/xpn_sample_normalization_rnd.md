# XPN Sample Normalization — R&D Spec

**Date**: 2026-03-16
**Status**: Draft
**Scope**: Level normalization standards for XO_OX XPN/MPC packs

---

## 1. Target Levels by Instrument Type

The MPC's internal mixer runs at 32-bit float headroom, so samples do not need to be brickwalled. The targets below balance transient preservation with mix-ready consistency.

| Category | Peak (dBFS) | RMS (dBFS) | Notes |
|---|---|---|---|
| Kick | -3 | -12 to -18 | Low-mid body means perceived loudness is high; RMS range is wide to accommodate subs vs. punchy clicks |
| Snare | -3 | -14 to -20 | Crack transient dominates; wide RMS band allows rimshots vs. fat snares to coexist |
| Hi-hats / Cymbals | -6 | -18 to -24 | Extra 3 dB peak guard prevents cymbals from piercing through a dense pad layout |
| Open hats / Rides | -6 | -18 to -22 | Sustain-heavy; RMS ceiling tighter than closed hats |
| Bass notes | -3 | -12 to -16 | Fundamental must sit loud; melodic bass lines compete with kick at -18 RMS |
| Melodic / Pads | -6 | -18 to -24 | Pad content is mixed in; lower peak ceiling keeps it from jumping above drums |
| Percussion / FX | -6 | -20 to -26 | Supporting role — deliberately quieter than primary hits |

**Headroom rationale**: The MPC default pad volume is 100 (unity). A kick at -3 dBFS leaves 3 dB before the pad clips at the MPC's internal bus. With 16 pads potentially triggering simultaneously, 3 dB is minimal — pad volume discipline is expected at the mix stage, not baked into the samples.

---

## 2. Normalization Algorithms

### Peak Normalization
Scales so the loudest sample in the file reaches a target peak. Fast, deterministic, and universally supported.

**Problem for drums**: Two snares can share a -3 dBFS peak while differing by 10 dB in RMS. The denser snare will sound drastically louder on the pad, breaking kit consistency.

### RMS Normalization
Scales so the mean power of the signal reaches a target. Better for perceived loudness consistency but sensitive to silence padding — a sample with 500 ms of decay will compute lower RMS than a dry 100 ms hit, even if they sound equally loud.

**Problem for short transients**: A tight kick at -18 RMS requires heavy gain, which can clip the peak or forces a low ceiling that leaves headroom unused.

### LUFS Normalization (EBU R128)
Gated loudness measurement that ignores silence. Designed for broadcast; accounts for human loudness perception weighting across frequencies.

**Problem for single-shot samples**: Gated LUFS measurement on a 200 ms snare hit is unstable — the gate window often covers the entire file, making results equivalent to RMS on short files with no meaningful improvement.

### Recommendation for XPN Packs

**Use peak normalization as the primary pass, then apply per-category RMS gain trim.**

1. Peak-normalize each sample to its category ceiling (kick -3, cymbal -6, etc.)
2. Measure resulting RMS; if it falls outside the target RMS window, apply a secondary trim of up to ±3 dB
3. If RMS trim would push the peak above -1 dBFS, skip the trim and flag the file for manual review

This hybrid approach preserves transient character while closing the worst consistency gaps.

---

## 3. Inter-Pack Consistency — XO_OX Pack Standard

When a producer loads samples from ONSET alongside samples from OHM or OPAL, the pads must feel balanced without manual volume adjustment.

**XO_OX Pack Standard (XPS-1)**:
- All primary hit categories target the levels in Section 1
- All packs are normalized using the same tool and the same algorithm version
- Pack metadata includes a `normalization_version` field in `manifest.json` so future re-normalizations are traceable
- A reference test kit (`XO_OX_Reference_Kit.xpn`) ships with one sample from each category normalized to XPS-1; producers can A/B any new sample against the reference

---

## 4. Velocity Layer Level Staging

Velocity layers must feel like the same instrument played softer, not a different sample with different EQ. Pure gain reduction is the baseline; real-world instruments also get darker and shorter at low velocities, but that is a sound design concern, not a normalization concern.

**dB reduction per layer (relative to top velocity layer):**

| Layers | Stage (top → bottom) |
|---|---|
| 2-layer | 0, -8 dB |
| 4-layer | 0, -6, -12, -20 dB |
| 8-layer | 0, -3, -6, -9, -13, -17, -22, -28 dB |

The curve is slightly logarithmic — the jump from layer 1 to 2 is smaller than the jump from layer 7 to 8. This mirrors how MIDI velocity curves map to finger pressure and keeps soft ghost notes audible without making medium-velocity hits too loud.

**Application**: Apply velocity staging after normalization. The normalized file represents the top layer. Lower layers are stored at their reduced gain, or the XPN keygroup `VelocityEnd` + `Level` fields encode the staging at load time.

---

## 5. Proposed Tool: `xpn_normalize.py`

CLI usage:

```
python xpn_normalize.py --input ./stems/ --output ./normalized/ --mode peak --target -3
python xpn_normalize.py --input ./stems/ --output ./normalized/ --mode rms --target -18
python xpn_normalize.py --input ./stems/ --output ./normalized/ --mode hybrid --target -3
```

Full implementation using Python stdlib only (no numpy, no scipy, no external audio libs — requires only `wave` module; 16-bit PCM WAV only):

```python
#!/usr/bin/env python3
"""
xpn_normalize.py — XO_OX XPN Sample Normalizer
Supports peak, RMS, and hybrid normalization for 16-bit PCM WAV files.
No external dependencies — stdlib only.
"""

import argparse
import math
import os
import struct
import sys
import wave
from pathlib import Path


def read_wav(path: str):
    """Read a WAV file. Returns (frames_int, n_channels, sample_rate, n_frames)."""
    with wave.open(path, 'rb') as wf:
        n_channels = wf.getnchannels()
        sample_rate = wf.getframerate()
        sample_width = wf.getsampwidth()
        n_frames = wf.getnframes()
        raw = wf.readframes(n_frames)

    if sample_width != 2:
        raise ValueError(f"Only 16-bit PCM supported, got {sample_width * 8}-bit: {path}")

    total_samples = n_frames * n_channels
    fmt = f"<{total_samples}h"
    samples = list(struct.unpack(fmt, raw))
    return samples, n_channels, sample_rate, n_frames


def write_wav(path: str, samples: list, n_channels: int, sample_rate: int):
    """Write samples (16-bit int list) to a WAV file."""
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    total_samples = len(samples)
    fmt = f"<{total_samples}h"
    raw = struct.pack(fmt, *samples)
    n_frames = total_samples // n_channels
    with wave.open(path, 'wb') as wf:
        wf.setnchannels(n_channels)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.setnframes(n_frames)
        wf.writeframes(raw)


def db_to_linear(db: float) -> float:
    return 10 ** (db / 20.0)


def linear_to_db(linear: float) -> float:
    if linear <= 0:
        return -math.inf
    return 20 * math.log10(linear)


def measure_peak(samples: list) -> float:
    """Return peak amplitude as fraction of full scale (0.0–1.0)."""
    max_abs = max(abs(s) for s in samples)
    return max_abs / 32767.0


def measure_rms(samples: list) -> float:
    """Return RMS amplitude as fraction of full scale."""
    sum_sq = sum(s * s for s in samples)
    rms = math.sqrt(sum_sq / len(samples))
    return rms / 32767.0


def apply_gain(samples: list, gain_linear: float, hard_clip_db: float = -0.1) -> list:
    """Scale samples by gain_linear; hard-clip at hard_clip_db dBFS."""
    clip = int(db_to_linear(hard_clip_db) * 32767)
    out = []
    for s in samples:
        v = int(s * gain_linear)
        v = max(-clip, min(clip, v))
        out.append(v)
    return out


def normalize_peak(samples: list, target_db: float) -> tuple:
    peak = measure_peak(samples)
    if peak == 0:
        return samples, 0.0
    target_lin = db_to_linear(target_db)
    gain = target_lin / peak
    gain_db = linear_to_db(gain)
    return apply_gain(samples, gain), gain_db


def normalize_rms(samples: list, target_db: float, peak_ceiling_db: float = -1.0) -> tuple:
    rms = measure_rms(samples)
    if rms == 0:
        return samples, 0.0
    target_lin = db_to_linear(target_db)
    gain = target_lin / rms
    # Check peak ceiling
    peak_after = measure_peak(samples) * gain
    ceiling = db_to_linear(peak_ceiling_db)
    if peak_after > ceiling:
        gain = ceiling / measure_peak(samples)
    gain_db = linear_to_db(gain)
    return apply_gain(samples, gain), gain_db


def normalize_hybrid(samples: list, target_peak_db: float,
                     rms_min_db: float = -24.0, rms_max_db: float = -12.0,
                     trim_limit_db: float = 3.0) -> tuple:
    """Peak normalize then apply up to ±trim_limit_db RMS trim."""
    normed, gain_db = normalize_peak(samples, target_peak_db)
    rms_db = linear_to_db(measure_rms(normed))
    trim = 0.0
    if rms_db < rms_min_db:
        trim = min(rms_min_db - rms_db, trim_limit_db)
    elif rms_db > rms_max_db:
        trim = max(rms_max_db - rms_db, -trim_limit_db)
    if trim != 0.0:
        peak_after_trim = measure_peak(normed) * db_to_linear(trim)
        if peak_after_trim <= db_to_linear(-1.0):
            normed = apply_gain(normed, db_to_linear(trim))
            gain_db += trim
    return normed, gain_db


def process_file(src: str, dst: str, mode: str, target_db: float):
    samples, n_ch, sr, _ = read_wav(src)
    if mode == "peak":
        out, gain_db = normalize_peak(samples, target_db)
    elif mode == "rms":
        out, gain_db = normalize_rms(samples, target_db)
    elif mode == "hybrid":
        out, gain_db = normalize_hybrid(samples, target_db)
    else:
        raise ValueError(f"Unknown mode: {mode}")
    write_wav(dst, out, n_ch, sr)
    return gain_db


def main():
    parser = argparse.ArgumentParser(description="XO_OX XPN Sample Normalizer")
    parser.add_argument("--input", required=True, help="Input directory of WAV files")
    parser.add_argument("--output", required=True, help="Output directory")
    parser.add_argument("--mode", choices=["peak", "rms", "hybrid"], default="hybrid")
    parser.add_argument("--target", type=float, default=-3.0,
                        help="Target level in dBFS (peak or RMS depending on mode)")
    parser.add_argument("--recursive", action="store_true", help="Recurse into subdirectories")
    args = parser.parse_args()

    src_root = Path(args.input)
    dst_root = Path(args.output)
    pattern = "**/*.wav" if args.recursive else "*.wav"
    files = sorted(src_root.glob(pattern))

    if not files:
        print(f"No WAV files found in {src_root}", file=sys.stderr)
        sys.exit(1)

    print(f"Mode: {args.mode}  Target: {args.target} dBFS  Files: {len(files)}")
    print(f"{'File':<50} {'Gain':>8}")
    print("-" * 60)

    errors = []
    for wav in files:
        rel = wav.relative_to(src_root)
        dst = dst_root / rel
        try:
            gain_db = process_file(str(wav), str(dst), args.mode, args.target)
            sign = "+" if gain_db >= 0 else ""
            print(f"{str(rel):<50} {sign}{gain_db:>6.2f} dB")
        except Exception as e:
            print(f"{str(rel):<50}  ERROR: {e}", file=sys.stderr)
            errors.append((str(rel), str(e)))

    print("-" * 60)
    print(f"Done. {len(files) - len(errors)} normalized, {len(errors)} errors.")
    if errors:
        sys.exit(1)


if __name__ == "__main__":
    main()
```

**Limitations of this implementation**: 16-bit PCM WAV only. 24-bit and 32-bit float WAV files (common from DAW bounces) require a separate read path using `struct` with 3-byte or 4-byte unpacking. For production use, replace the read/write layer with `soundfile` or `pydub` once those are available in the build environment.

---

## Summary

| Decision | Recommendation |
|---|---|
| Primary normalization algorithm | Peak normalization to category ceiling |
| Consistency across a kit | Hybrid: peak + RMS trim (±3 dB max) |
| Inter-pack standard | XPS-1 — uniform tool + manifest versioning |
| Velocity layer staging | 4-layer: 0 / -6 / -12 / -20 dB |
| Reference kit | `XO_OX_Reference_Kit.xpn` ships with each release |
