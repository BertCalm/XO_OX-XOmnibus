# XPN Smart Trim & Loop Point Detection — R&D Spec

**Date**: 2026-03-16
**Status**: Research / Pseudocode — Sonnet-ready for implementation
**Purpose**: Automatic sample trimming and loop point detection for XPN/MPC sample preparation

---

## 1. Silence Trimming

Detect and remove leading/trailing silence below a threshold. The naive -60 dBFS cutoff fails on soft pads that open with a slow attack — the first 200ms can be entirely below threshold yet musically intentional.

**Recommended thresholds by instrument type:**
- Drums / transients: -60 dBFS (aggressive trim)
- Plucked strings, piano: -50 dBFS
- Pads, strings (bowed), brass: -40 dBFS (conservative — preserve slow attacks)
- Organ, sustained tones: -45 dBFS

```python
def trim_silence(samples, sr, threshold_db=-60, hold_ms=10):
    threshold_linear = 10 ** (threshold_db / 20)
    hold_samples = int(hold_ms * sr / 1000)
    rms_window = int(0.005 * sr)  # 5ms RMS window

    def rms(block):
        return (sum(x**2 for x in block) / len(block)) ** 0.5

    # Find first window exceeding threshold
    start = 0
    for i in range(0, len(samples) - rms_window, rms_window // 2):
        if rms(samples[i:i+rms_window]) > threshold_linear:
            start = max(0, i - hold_samples)
            break

    # Find last window exceeding threshold (scan from end)
    end = len(samples)
    for i in range(len(samples) - rms_window, start, -(rms_window // 2)):
        if rms(samples[i:i+rms_window]) > threshold_linear:
            end = min(len(samples), i + rms_window + hold_samples)
            break

    return samples[start:end], start, end
```

---

## 2. Transient Alignment

For drum hits, align the waveform so the transient peak lands at sample 0 (or N samples before for pre-ring/stick click). The challenge: noise floor fluctuations produce false "first rising edge" reads. Use onset detection on a smooth energy envelope, not raw sample peaks.

```python
def find_transient_onset(samples, sr, pre_ring_ms=2):
    pre_ring = int(pre_ring_ms * sr / 1000)
    window = int(0.001 * sr)  # 1ms hop

    energy = []
    for i in range(0, len(samples) - window, window):
        e = sum(x**2 for x in samples[i:i+window])
        energy.append((i, e))

    # Onset = first frame where energy exceeds 6dB above noise floor estimate
    noise_floor = sorted([e for _, e in energy[:20]])[10]  # median of first 20 frames
    onset_threshold = noise_floor * 4  # 6 dB above floor

    for idx, (pos, e) in enumerate(energy):
        if e > onset_threshold:
            # Confirm it's a real transient: next 3 frames must also rise
            if idx + 3 < len(energy) and energy[idx+1][1] > e:
                return max(0, pos - pre_ring)

    return 0
```

---

## 3. Loop Point Detection

### Zero-Crossing Method (fast, works for simple waveforms)
```python
def find_loop_zc(samples, target_end):
    # Search ±50ms around target for a zero crossing
    search = int(0.05 * len(samples))
    for i in range(target_end, target_end - search, -1):
        if samples[i-1] < 0 and samples[i] >= 0:  # upward zero cross
            return i
    return target_end
```

### RMS-Matching Method (reliable for pads/sustained tones)
```python
def find_loop_rms(samples, loop_start, window_ms, sr):
    win = int(window_ms * sr / 1000)
    start_rms = rms(samples[loop_start:loop_start + win])
    best_pos, best_diff = loop_start, float('inf')
    # Search second half of sample for matching RMS window
    for i in range(len(samples) // 2, len(samples) - win):
        diff = abs(rms(samples[i:i+win]) - start_rms)
        if diff < best_diff:
            best_diff, best_pos = diff, i
    return best_pos
```

### Phase-Coherence Method (best for harmonically complex tones)
FFT a 2048-sample window at loop start and candidate loop end. Minimize the mean absolute phase difference across the 8 strongest harmonics. Expensive but produces click-free loops on rich timbres.

**Method recommendation by instrument type:**

| Instrument | Best Method | Notes |
|---|---|---|
| Pads / atmosphere | RMS-matching | Slow attack makes ZC unreliable |
| Bowed strings | Phase-coherence | Bow noise creates ZC false positives |
| Brass | Phase-coherence | Strong harmonics, phase drift is audible |
| Organ | Zero-crossing | Stable waveform, ZC is sufficient |

---

## 4. One-Shot vs Loop Detection

Auto-classify a sample based on its amplitude envelope shape.

```python
def classify_sample(samples, sr):
    segment = int(0.05 * sr)  # 50ms segments
    rms_curve = [rms(samples[i:i+segment]) for i in range(0, len(samples)-segment, segment)]

    peak_idx = rms_curve.index(max(rms_curve))
    tail_rms = sum(rms_curve[peak_idx + 5:]) / max(1, len(rms_curve) - peak_idx - 5)
    peak_rms = rms_curve[peak_idx]
    decay_ratio = tail_rms / peak_rms if peak_rms > 0 else 0

    sustain_frames = sum(1 for v in rms_curve[peak_idx:] if v > peak_rms * 0.3)
    sustain_duration_ms = sustain_frames * 50

    if decay_ratio < 0.1 or sustain_duration_ms < 200:
        return "one_shot"   # decays rapidly — drum hit, pluck
    elif decay_ratio > 0.5 and sustain_duration_ms > 500:
        return "loop"       # sustains strongly — pad, organ, string
    else:
        return "one_shot"   # default safe for ambiguous cases
```

---

## 5. XPM Field Mapping

Trim and loop points map to the following XPM XML fields (in `<Sample>` elements inside `<Layer>`):

| Algorithm output | XPM field | Notes |
|---|---|---|
| Trim start (samples) | `<SampleStart>` | Integer, sample offset from file start |
| Trim end (samples) | `<SampleEnd>` | Integer, sample offset from file start |
| Loop start (samples) | `<LoopStart>` | Must be >= SampleStart |
| Loop end (samples) | `<LoopEnd>` | Must be <= SampleEnd |
| Loop enabled flag | `<LoopOn>` | `"1"` = on, `"0"` = off |

All values are **absolute sample offsets** from the beginning of the audio file — not relative to SampleStart. `<LoopOn>` should be set from the one-shot vs loop classifier output: `"1"` for loop, `"0"` for one-shot.

---

## 6. Proposed Tool: `xpn_smart_trim.py`

**Status**: Sonnet-ready for full implementation.

### CLI Spec

```
python xpn_smart_trim.py --input <path/to/samples/> --xpm <program.xpm>
  [--instrument-type drums|pads|strings|brass|organ|auto]
  [--silence-threshold -60]
  [--pre-ring-ms 2]
  [--loop-method zc|rms|phase|auto]
  [--dry-run]
  [--output-xpm <updated_program.xpm>]
```

### Behavior

1. Parse the input XPM to extract all `<Sample>` file references.
2. For each audio file:
   a. Run `classify_sample()` — one-shot or loop?
   b. Run `trim_silence()` with threshold appropriate to `--instrument-type` (or per-file if `auto`).
   c. If drum/transient: run `find_transient_onset()`, set SampleStart accordingly.
   d. If loop: run loop detection (method per `--loop-method` or instrument type default), set LoopStart/LoopEnd, LoopOn=1.
3. Write updated SampleStart/SampleEnd/LoopStart/LoopEnd/LoopOn values back into XPM XML.
4. In `--dry-run` mode: print a report table only, no file writes.

### Algorithms to implement (priority order)
1. `trim_silence` — all instrument types
2. `find_transient_onset` — drums only
3. `find_loop_rms` — pads, organ
4. `find_loop_zc` — organ fallback
5. `classify_sample` — auto mode
6. `find_loop_phase` — strings, brass (Phase 2 / optional)

### Input/Output
- **Input**: Directory of WAV/AIFF files + existing `.xpm` program file
- **Output**: Modified `.xpm` with updated sample region fields, plus optional `trim_report.csv` logging per-file decisions

---

## References
- XPM format: `Tools/` in XOlokun repo — existing keygroup/drum export tools for field names
- Related: `xpn-tools.md` (MPC export pipeline), `sound_design_best_practices_xpn.md`
