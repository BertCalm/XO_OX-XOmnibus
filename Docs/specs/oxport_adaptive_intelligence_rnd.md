# Oxport Adaptive Intelligence — R&D Spec
*2026-03-16 | XO_OX Tools Suite*

Oxport currently applies static heuristics: fixed velocity curves, manual root note assignment, hardcoded trim thresholds. This document details 8 adaptive intelligence features where the tool analyzes audio content and makes instrument-aware decisions. Each idea is rated for complexity and flagged for integration path.

---

## 1. Instrument-Aware Velocity Curve Shaping

**What it does**: Detects instrument type (kick, snare, melodic, pad, transient perc) and applies a velocity-to-amplitude curve matched to how that instrument behaves acoustically. Kicks need convex curves (exponential punch). Pads need concave (gradual swell). Snares need near-linear with a hard noise floor.

**Technical approach**: RMS energy profile + spectral centroid + transient density classify the instrument. Lookup table of 6–8 preset curve shapes, selected by classification score.

**Complexity**: Sonnet-ready. Pure heuristic, no ML. Classification needs ~30 labeled examples to tune thresholds.

**Value**: High. Vibe's musical velocity curve is a manual parameter today — making it automatic per-sample eliminates the most tedious per-kit tuning step.

**Integration**: Standalone `xpn_adaptive_velocity.py`, callable from oxport.py as a pre-pass.

```python
def classify_instrument(wav_path):
    y, sr = librosa.load(wav_path, sr=None)
    centroid = librosa.feature.spectral_centroid(y=y, sr=sr).mean()
    rms = librosa.feature.rms(y=y).mean()
    onset_density = len(librosa.onset.onset_detect(y=y, sr=sr)) / (len(y) / sr)
    if centroid < 800 and rms > 0.1:
        return "kick"       # → convex curve
    elif onset_density > 5 and centroid > 3000:
        return "snare"      # → near-linear
    elif onset_density < 1:
        return "pad"        # → concave
    return "perc"           # → default Vibe curve
```

---

## 2. Root Note Auto-Detection from Pitched Audio

**What it does**: For melodic samples (synth hits, bass notes, tonal perc), detects the fundamental frequency and maps to the nearest MIDI note. Sets `RootNote` in the keygroup XML automatically.

**Technical approach**: YIN algorithm (period detection) or librosa `pyin` for probabilistic pitch detection. Confidence threshold filters unpitched material. Nearest-semitone rounding + octave sanity check (reject sub-20Hz / above 8kHz fundamentals).

**Complexity**: Sonnet-ready. librosa.pyin is 3 lines. Edge case: detuned samples (filter, drift) need tolerance window of ±30 cents before rounding.

**Value**: Very high for melodic packs. Manual root assignment is error-prone and the most common cause of bad MPC playback.

**Integration**: Integrated into oxport.py as a per-sample pass in the keygroup builder.

```python
def detect_root_note(wav_path):
    y, sr = librosa.load(wav_path, sr=None)
    f0, voiced_flag, _ = librosa.pyin(y, fmin=20, fmax=4000, sr=sr)
    voiced = f0[voiced_flag]
    if len(voiced) < 10:
        return None  # unpitched — skip
    median_hz = float(np.median(voiced))
    midi_note = round(12 * np.log2(median_hz / 440) + 69)
    return max(0, min(127, midi_note))
```

---

## 3. Smart Sample Trimming — Silence, Transients, Loop Points

**What it does**: Three sub-functions: (a) trim leading/trailing silence below a dB threshold; (b) detect the primary transient onset for start-point placement; (c) identify a stable sustain region for loop-point suggestion.

**Technical approach**: (a) librosa.effects.trim with adaptive threshold derived from the sample's peak level. (b) librosa onset_detect, take first onset frame. (c) Find the longest contiguous region where short-time RMS variance is below 5% of mean — that plateau is the loop candidate.

**Complexity**: Sonnet-ready for (a) and (b). Loop point suggestion (c) is Sonnet-ready for pads/strings; transient-heavy hits need a "no loop" fallback.

**Value**: High. Eliminates pre-roll clicks and over-long tails, two of the most common issues in raw synth exports.

**Integration**: Standalone `xpn_adaptive_trim.py`, called during sample preparation before XPN build.

```python
def smart_trim(wav_path, threshold_db=-50):
    y, sr = librosa.load(wav_path, sr=None)
    y_trimmed, _ = librosa.effects.trim(y, top_db=abs(threshold_db))
    onset_frames = librosa.onset.onset_detect(y=y_trimmed, sr=sr)
    start_sample = librosa.frames_to_samples(onset_frames[0]) if len(onset_frames) else 0
    rms = librosa.feature.rms(y=y_trimmed, frame_length=2048, hop_length=512)[0]
    stable = np.where(np.abs(rms - rms.mean()) < 0.05 * rms.mean())[0]
    loop_start = librosa.frames_to_samples(stable[0]) if len(stable) > 10 else None
    return start_sample, loop_start
```

---

## 4. Intelligent Keygroup Range Assignment by Instrument Register

**What it does**: For multi-sample melodic patches, assigns low/high note boundaries to each keygroup using equal-tempered split logic, but biased toward the instrument's natural register. A bass patch at C1 should not span up to C6; a synth lead at C5 should not map down to C0.

**Technical approach**: Detect root notes for all samples in a set. Sort ascending. Midpoint splits between adjacent root notes. Clamp the outermost groups to a register window (±2 octaves from the median root).

**Complexity**: Sonnet-ready. Pure arithmetic after root detection.

**Value**: Medium-high. Prevents stretch artifacts from extreme transposition at patch edges.

**Integration**: Integrated into oxport.py keygroup builder, after root note detection pass.

```python
def assign_keygroup_ranges(root_notes_sorted):
    ranges = []
    for i, root in enumerate(root_notes_sorted):
        low = (root + root_notes_sorted[i-1]) // 2 if i > 0 else max(0, root - 24)
        high = (root + root_notes_sorted[i+1]) // 2 if i < len(root_notes_sorted)-1 else min(127, root + 24)
        ranges.append((low, high))
    return ranges
```

---

## 5. Automatic Tag and Category Suggestion from Audio Analysis

**What it does**: Proposes XPN category tags (Kick, Snare, Bass, Lead, Pad, FX, Texture) and mood descriptors (Dark, Bright, Warm, Aggressive) from audio features. These populate the `Category` and `Tags` fields in the XPN manifest.

**Technical approach**: Feature vector: spectral centroid, spectral rolloff, zero-crossing rate, MFCC mean (13 coefficients), RMS, onset density. Rule-based thresholds for instrument type. Mood: brightness = rolloff/sr ratio; aggression = ZCR × peak RMS.

**Complexity**: Sonnet-ready for rule-based. Opus-level if upgrading to a trained sklearn classifier on 500+ labeled samples.

**Value**: Medium. Saves manual tagging time, especially for large synthesis batch exports (100+ samples per kit).

**Integration**: Standalone `xpn_adaptive_tags.py`, output merged into manifest builder.

```python
def suggest_tags(wav_path):
    y, sr = librosa.load(wav_path, sr=None)
    centroid = librosa.feature.spectral_centroid(y=y, sr=sr).mean()
    zcr = librosa.feature.zero_crossing_rate(y).mean()
    rms = librosa.feature.rms(y=y).mean()
    tags = []
    if centroid < 600: tags.append("Bass")
    elif centroid > 5000: tags.append("Bright")
    if zcr > 0.1 and rms > 0.05: tags.append("Aggressive")
    elif rms < 0.02: tags.append("Texture")
    return tags
```

---

## 6. Preset DNA Prediction from Rendered Audio

**What it does**: Given a rendered audio sample from an XO_OX engine, predicts likely Sonic DNA values (e.g., Warm=0.8, Dark=0.6, Punchy=0.2) without access to the source preset. Useful for validating that rendered samples match their declared DNA, and for tagging samples from external sources.

**Technical approach**: Train a small regression model (sklearn Ridge or RandomForest) on (audio features) → (DNA vector) pairs derived from existing preset database. Feature set: MFCCs, spectral contrast, chroma, onset strength. The 2,369-preset library provides training data.

**Complexity**: Opus-level for initial training pipeline. Sonnet-ready for inference once model is serialized. Requires feature extraction from rendered audio of known presets — a one-time batch job.

**Value**: High long-term. Closes the loop between preset design intent and acoustic output.

**Integration**: Standalone `xpn_adaptive_dna.py` with `--train` and `--predict` modes.

```python
# Inference path (post-training)
def predict_dna(wav_path, model, scaler):
    y, sr = librosa.load(wav_path, sr=None)
    mfcc = librosa.feature.mfcc(y=y, sr=sr, n_mfcc=13).mean(axis=1)
    contrast = librosa.feature.spectral_contrast(y=y, sr=sr).mean(axis=1)
    features = np.concatenate([mfcc, contrast]).reshape(1, -1)
    features_scaled = scaler.transform(features)
    dna_vector = model.predict(features_scaled)[0]
    return dict(zip(DNA_KEYS, dna_vector))
```

---

## 7. Kit Completeness Scoring

**What it does**: Analyzes a drum kit's sample set and scores how complete it is as a usable production kit. Checks for presence of foundational elements: kick, snare, closed hat, open hat, clap or snap, mid/low percussion. Outputs a score (0–100) and a gap report listing missing archetypes.

**Technical approach**: Classify all samples using the instrument classifier (Idea 1). Map classifications to archetype slots. Score = (filled slots / required slots) × 100. Weight kick + snare highest (required), hats medium, extras optional.

**Complexity**: Sonnet-ready. Depends on instrument classifier; no new signal processing.

**Value**: High for QA workflow. A kit scoring below 60 should not ship. Catches half-built kits before packaging.

**Integration**: Integrated into oxport.py as a pre-flight check, with `--strict` flag that aborts on score < threshold.

```python
REQUIRED = {"kick": 20, "snare": 20, "closed_hat": 15, "open_hat": 10, "clap": 10}
OPTIONAL = {"tom": 5, "perc": 10, "fx": 10}

def score_kit(classified_samples):
    filled = {k: any(s == k for s in classified_samples) for k in REQUIRED}
    score = sum(REQUIRED[k] for k, present in filled.items() if present)
    score += sum(v for k, v in OPTIONAL.items() if k in classified_samples)
    gaps = [k for k, present in filled.items() if not present]
    return score, gaps
```

---

## 8. Cross-Pack Consistency Checking — Level and Spectral Balance

**What it does**: Given two or more packs intended to work together (e.g., XObese Character + Onset Drums), checks that samples are consistent in perceived loudness (integrated LUFS) and broadband spectral slope. Flags outliers that will sound thin, boomy, or jarring in the same session.

**Technical approach**: Compute integrated loudness via pyloudnorm (ITU-R BS.1770-4). Compute spectral slope as linear regression over 1/3-octave band energies. Flag samples outside ±3 LU from pack median, or with slope deviation > 1.5 dB/octave from median.

**Complexity**: Sonnet-ready. pyloudnorm is a direct dependency. Spectral slope is numpy polyfit over librosa mel bands.

**Value**: Medium-high for multi-pack releases. Inconsistent levels are the most common complaint in third-party sample libraries.

**Integration**: Standalone `xpn_adaptive_consistency.py`, run as a cross-pack QA step separate from per-pack build.

```python
import pyloudnorm as pyln

def check_pack_consistency(wav_paths):
    meter = pyln.Meter(44100)
    loudness_values = []
    for path in wav_paths:
        y, sr = soundfile.read(path)
        lufs = meter.integrated_loudness(y)
        loudness_values.append((path, lufs))
    median_lufs = np.median([v for _, v in loudness_values])
    outliers = [(p, l) for p, l in loudness_values if abs(l - median_lufs) > 3.0]
    return median_lufs, outliers
```

---

## Summary Table

| # | Feature | Complexity | Integration Path | Priority |
|---|---------|------------|-----------------|----------|
| 1 | Velocity curve shaping | Sonnet | Standalone → oxport pre-pass | High |
| 2 | Root note detection | Sonnet | Integrated into oxport keygroup builder | Very High |
| 3 | Smart trim / loop points | Sonnet | Standalone → sample prep step | High |
| 4 | Keygroup range assignment | Sonnet | Integrated into oxport | Medium-High |
| 5 | Tag / category suggestion | Sonnet (rules) / Opus (ML) | Standalone → manifest builder | Medium |
| 6 | Preset DNA prediction | Opus (train) / Sonnet (infer) | Standalone `--train`/`--predict` | High (long-term) |
| 7 | Kit completeness scoring | Sonnet | Integrated — pre-flight check | High |
| 8 | Cross-pack consistency | Sonnet | Standalone QA tool | Medium-High |

**Recommended build order**: 2 → 3 → 1 → 7 → 4 → 8 → 5 → 6. Root detection and trimming unblock all downstream keygroup work. Kit scoring and velocity curves are the highest-ROI QA additions. DNA prediction is the most ambitious and should follow after the classifier infrastructure is mature.

**Shared dependency**: Ideas 1, 5, and 7 all require the same instrument classifier — build it once as `xpn_classify_instrument.py` and import across all tools.
