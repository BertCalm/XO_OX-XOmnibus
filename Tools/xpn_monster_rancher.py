#!/usr/bin/env python3
"""
XPN Monster Rancher — XO_OX Designs
Rex + Hex :: XPN format android + hacker android

Like the 1997 Tecmo game that scanned any CD and generated a unique monster,
this tool scans any audio file, analyzes it, and generates a complete XPM kit
or keygroup whose character derives entirely from the source material.

Usage:
    python xpn_monster_rancher.py source.wav --output ./output/ --mode auto
    python xpn_monster_rancher.py source.wav --output ./output/ --mode kit --pads 16
    python xpn_monster_rancher.py source.wav --output ./output/ --mode keygroup --root-note auto
    python xpn_monster_rancher.py source_dir/ --output ./output/ --batch
    python xpn_monster_rancher.py --dna '{"brightness": 0.8, "warmth": 0.3}' --engine ONSET --output ./output/
"""

import argparse
import array
import json
import math
import struct
import sys
import wave
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional, Tuple
from xml.sax.saxutils import escape as xml_escape

# ---------------------------------------------------------------------------
# Optional numpy/scipy — used only if available
# ---------------------------------------------------------------------------
try:
    import numpy as np
    import scipy.signal
    import scipy.io.wavfile
    _NUMPY_AVAILABLE = True
except ImportError:
    _NUMPY_AVAILABLE = False


# ---------------------------------------------------------------------------
# XPM Header (canonical)
# ---------------------------------------------------------------------------
_XPM_HEADER = (
    '<?xml version="1.0" encoding="UTF-8"?>\n\n'
    '<MPCVObject>\n'
    '  <Version>\n'
    '    <File_Version>1.7</File_Version>\n'
    '    <Application>MPC-V</Application>\n'
    '    <Application_Version>2.10.0.0</Application_Version>\n'
    '    <Platform>OSX</Platform>\n'
    '  </Version>'
)

# ---------------------------------------------------------------------------
# Band definitions — mirror XOptic's 8-band Butterworth filter bank
# ---------------------------------------------------------------------------
_BAND_EDGES = [0, 80, 200, 500, 1000, 4000, 8000, 16000, None]  # Hz (None = Nyquist)
_BAND_NAMES = [
    "Sub",        # 0–80 Hz
    "Bass",       # 80–200 Hz
    "LoMid",      # 200–500 Hz
    "Mid",        # 500–1k Hz
    "HiMid",      # 1k–4k Hz
    "Presence",   # 4k–8k Hz
    "Air",        # 8k–16k Hz
    "Ultra",      # 16k–Nyquist
]

# MIDI note names for pitch detection output
_NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def _midi_to_hz(midi: int) -> float:
    return 440.0 * (2.0 ** ((midi - 69) / 12.0))


def _hz_to_midi(hz: float) -> int:
    if hz <= 0:
        return 60
    midi = 69 + 12 * math.log2(hz / 440.0)
    return max(0, min(127, round(midi)))


def _midi_to_note_name(midi: int) -> str:
    return f"{_NOTE_NAMES[midi % 12]}{(midi // 12) - 1}"


def _fmt(v: float) -> str:
    return f"{v:.6f}"


# ===========================================================================
# AUDIO I/O
# ===========================================================================

class AudioData:
    """Container for decoded audio."""
    def __init__(self, samples: List[float], sample_rate: int,
                 num_channels: int, num_frames: int, bit_depth: int):
        self.samples = samples          # mono float, normalized -1..1
        self.sample_rate = sample_rate
        self.num_channels = num_channels
        self.num_frames = num_frames
        self.bit_depth = bit_depth
        self.duration_s = num_frames / max(sample_rate, 1)


def _read_wav_stdlib(path: Path) -> Optional[AudioData]:
    """Read WAV using stdlib wave module."""
    try:
        with wave.open(str(path), 'rb') as wf:
            sr = wf.getframerate()
            nch = wf.getnchannels()
            sw = wf.getsampwidth()
            nframes = wf.getnframes()
            raw = wf.readframes(nframes)
    except Exception:
        return None

    bit_depth = sw * 8

    if bit_depth == 8:
        # 8-bit WAV is unsigned
        vals = list(raw)
        samples_all = [(v - 128) / 128.0 for v in vals]
    elif bit_depth == 16:
        n = len(raw) // 2
        vals = struct.unpack(f'<{n}h', raw[:n * 2])
        samples_all = [v / 32768.0 for v in vals]
    elif bit_depth == 24:
        samples_all = []
        for i in range(0, len(raw) - 2, 3):
            val = raw[i] | (raw[i + 1] << 8) | (raw[i + 2] << 16)
            if val & 0x800000:
                val -= 0x1000000
            samples_all.append(val / 8388608.0)
    elif bit_depth == 32:
        n = len(raw) // 4
        vals = struct.unpack(f'<{n}i', raw[:n * 4])
        samples_all = [v / 2147483648.0 for v in vals]
    else:
        return None

    # Mix down to mono
    if nch > 1:
        mono = []
        for i in range(0, len(samples_all) - nch + 1, nch):
            mono.append(sum(samples_all[i:i + nch]) / nch)
        samples_all = mono
        nframes = len(samples_all)

    return AudioData(samples_all, sr, nch, nframes, bit_depth)


def _read_wav_scipy(path: Path) -> Optional[AudioData]:
    """Read WAV using scipy.io.wavfile as fallback."""
    if not _NUMPY_AVAILABLE:
        return None
    try:
        sr, data = scipy.io.wavfile.read(str(path))
        if data.ndim > 1:
            data = data.mean(axis=1)
        if data.dtype == np.int16:
            samples = (data / 32768.0).tolist()
            bd = 16
        elif data.dtype == np.int32:
            samples = (data / 2147483648.0).tolist()
            bd = 32
        elif data.dtype == np.float32 or data.dtype == np.float64:
            samples = data.astype(float).tolist()
            bd = 32
        else:
            samples = data.astype(float).tolist()
            bd = 16
        return AudioData(samples, int(sr), 1, len(samples), bd)
    except Exception:
        return None


def load_audio(path: Path) -> Optional[AudioData]:
    """Load WAV audio, trying stdlib first then scipy."""
    audio = _read_wav_stdlib(path)
    if audio is None:
        audio = _read_wav_scipy(path)
    return audio


# ===========================================================================
# ANALYSIS PIPELINE
# ===========================================================================

def _rms_windows(samples: List[float], window_samples: int,
                 hop_samples: int) -> List[float]:
    """Compute RMS in overlapping windows. Returns list of RMS values."""
    rms_vals = []
    n = len(samples)
    pos = 0
    while pos + window_samples <= n:
        chunk = samples[pos:pos + window_samples]
        rms = math.sqrt(sum(x * x for x in chunk) / window_samples)
        rms_vals.append(rms)
        pos += hop_samples
    return rms_vals


def spectral_fingerprint(samples: List[float], sample_rate: int) -> List[float]:
    """
    Compute 8-band spectral energy fingerprint (mirrors XOptic filter bank).
    Uses numpy FFT if available, otherwise falls back to DFT on a downsampled frame.
    Returns 8 energy values normalized 0.0-1.0.
    """
    if not samples:
        return [0.0] * 8

    n = len(samples)
    nyquist = sample_rate / 2.0

    if _NUMPY_AVAILABLE:
        # Use a representative window (up to 65536 samples from the middle)
        center = n // 2
        win_size = min(65536, n)
        half = win_size // 2
        start = max(0, center - half)
        end = min(n, start + win_size)
        window_data = np.array(samples[start:end], dtype=np.float64)
        # Apply Hann window
        hann = np.hanning(len(window_data))
        window_data = window_data * hann
        fft_mag = np.abs(np.fft.rfft(window_data))
        freqs = np.fft.rfftfreq(len(window_data), d=1.0 / sample_rate)

        band_energies = []
        for i in range(8):
            lo = _BAND_EDGES[i]
            hi_raw = _BAND_EDGES[i + 1]
            hi = hi_raw if hi_raw is not None else nyquist
            hi = min(hi, nyquist)
            mask = (freqs >= lo) & (freqs < hi)
            if mask.any():
                energy = float(np.sqrt(np.mean(fft_mag[mask] ** 2)))
            else:
                energy = 0.0
            band_energies.append(energy)
    else:
        # Pure-Python fallback: compute DFT on a short window (512 samples)
        win_size = min(512, n)
        center = n // 2
        half = win_size // 2
        start = max(0, center - half)
        window_data = samples[start:start + win_size]
        actual_len = len(window_data)

        # Apply simple Hann window
        hann = [0.5 * (1.0 - math.cos(2.0 * math.pi * k / (actual_len - 1)))
                for k in range(actual_len)]
        windowed = [window_data[k] * hann[k] for k in range(actual_len)]

        # DFT (O(N^2) — only practical for small N)
        half_n = actual_len // 2 + 1
        fft_mag = []
        for k in range(half_n):
            re = sum(windowed[j] * math.cos(2 * math.pi * k * j / actual_len)
                     for j in range(actual_len))
            im = sum(windowed[j] * math.sin(2 * math.pi * k * j / actual_len)
                     for j in range(actual_len))
            fft_mag.append(math.sqrt(re * re + im * im))

        freq_resolution = sample_rate / actual_len
        band_energies = []
        for i in range(8):
            lo = _BAND_EDGES[i]
            hi_raw = _BAND_EDGES[i + 1]
            hi = hi_raw if hi_raw is not None else nyquist
            hi = min(hi, nyquist)
            lo_bin = max(0, int(lo / freq_resolution))
            hi_bin = min(half_n - 1, int(hi / freq_resolution))
            if hi_bin > lo_bin:
                chunk_mags = fft_mag[lo_bin:hi_bin]
                energy = math.sqrt(sum(v * v for v in chunk_mags) / len(chunk_mags))
            else:
                energy = 0.0
            band_energies.append(energy)

    # Normalize 0-1
    max_e = max(band_energies) if any(e > 0 for e in band_energies) else 1.0
    if max_e == 0:
        max_e = 1.0
    return [e / max_e for e in band_energies]


def detect_onsets(samples: List[float], sample_rate: int,
                  window_ms: float = 10.0, threshold_multiplier: float = 1.5,
                  min_gap_ms: float = 80.0) -> List[float]:
    """
    Simple energy-threshold onset detector.
    RMS in 10ms windows, threshold at threshold_multiplier × local mean.
    Returns list of onset times in ms.
    """
    if not samples or sample_rate <= 0:
        return []

    hop = max(1, int(sample_rate * window_ms / 1000.0))
    rms_vals = _rms_windows(samples, hop, hop)

    if not rms_vals:
        return []

    # Compute local mean with a sliding window (0.5s window)
    local_window = max(1, int(500.0 / window_ms))
    onsets_ms = []
    last_onset_ms = -999999.0
    min_gap = min_gap_ms

    for i, rms in enumerate(rms_vals):
        # Local mean: median of neighbors
        lo = max(0, i - local_window // 2)
        hi = min(len(rms_vals), i + local_window // 2 + 1)
        local_chunk = rms_vals[lo:hi]
        local_mean = sum(local_chunk) / len(local_chunk) if local_chunk else 0.001

        # Threshold check
        if rms > threshold_multiplier * local_mean and local_mean > 1e-6:
            onset_ms = i * window_ms
            # Enforce minimum gap between onsets
            if onset_ms - last_onset_ms >= min_gap:
                onsets_ms.append(onset_ms)
                last_onset_ms = onset_ms

    return onsets_ms


def detect_pitch(samples: List[float], sample_rate: int,
                 min_hz: float = 50.0, max_hz: float = 4000.0) -> Tuple[float, float]:
    """
    Autocorrelation-based pitch estimator.
    Returns (dominant_pitch_hz, confidence) where confidence is 0.0-1.0.
    Operates on a stable segment (low zero-crossing rate).
    Returns (0.0, 0.0) if no pitched content detected.
    """
    if not samples or sample_rate <= 0:
        return (0.0, 0.0)

    n = len(samples)

    # Find the most stable (lowest ZCR) segment of 4096 samples
    seg_size = min(4096, n)
    best_zcr = float('inf')
    best_start = 0

    step = max(1, (n - seg_size) // 20)  # Check ~20 positions
    for start in range(0, max(1, n - seg_size), step):
        chunk = samples[start:start + seg_size]
        zcr = sum(1 for i in range(1, len(chunk))
                  if (chunk[i] >= 0) != (chunk[i - 1] >= 0)) / len(chunk)
        if zcr < best_zcr:
            best_zcr = zcr
            best_start = start

    # If ZCR is very high everywhere, signal is likely noise/percussive
    if best_zcr > 0.4:
        return (0.0, 0.0)

    seg = samples[best_start:best_start + seg_size]
    seg_len = len(seg)

    # Compute autocorrelation
    min_lag = max(1, int(sample_rate / max_hz))
    max_lag = min(seg_len // 2, int(sample_rate / min_hz))

    if max_lag <= min_lag:
        return (0.0, 0.0)

    # Normalize segment
    seg_mean = sum(seg) / seg_len
    seg_centered = [s - seg_mean for s in seg]
    seg_energy = sum(s * s for s in seg_centered)
    if seg_energy < 1e-10:
        return (0.0, 0.0)

    # Autocorrelation using sliding dot product
    # r[lag] = sum(seg[i] * seg[i+lag])
    if _NUMPY_AVAILABLE:
        arr = np.array(seg_centered, dtype=np.float64)
        # Full autocorrelation via FFT
        fft_size = 1
        while fft_size < 2 * seg_len:
            fft_size <<= 1
        ft = np.fft.rfft(arr, n=fft_size)
        acorr_full = np.fft.irfft(ft * np.conj(ft))
        r0 = float(acorr_full[0])
        if r0 < 1e-10:
            return (0.0, 0.0)
        acorr = [float(acorr_full[lag]) / r0 for lag in range(min_lag, max_lag + 1)]
    else:
        # Pure Python autocorrelation (slow for large segments — we limit seg_size above)
        r0 = sum(s * s for s in seg_centered)
        if r0 < 1e-10:
            return (0.0, 0.0)
        acorr = []
        for lag in range(min_lag, max_lag + 1):
            corr = sum(seg_centered[i] * seg_centered[i + lag]
                       for i in range(seg_len - lag))
            acorr.append(corr / r0)

    # Find peak in autocorrelation
    if not acorr:
        return (0.0, 0.0)

    # Find the first strong peak (confidence > 0.3)
    best_lag_offset = max(range(len(acorr)), key=lambda i: acorr[i])
    confidence = acorr[best_lag_offset]

    if confidence < 0.25:
        return (0.0, 0.0)

    lag = min_lag + best_lag_offset
    pitch_hz = sample_rate / lag

    # Sanity check
    if pitch_hz < min_hz or pitch_hz > max_hz:
        return (0.0, 0.0)

    return (pitch_hz, float(min(1.0, max(0.0, confidence))))


def compute_felix_oscar(samples: List[float], sample_rate: int,
                        fingerprint: List[float]) -> float:
    """
    Spectral centroid relative to Nyquist → feliX (high centroid) vs Oscar (low centroid).
    Returns -1.0 (full Oscar) to +1.0 (full feliX).
    """
    if not fingerprint or sum(fingerprint) == 0:
        return 0.0

    # Compute weighted centroid using band midpoints
    band_mids = []
    nyquist = sample_rate / 2.0
    for i in range(8):
        lo = _BAND_EDGES[i]
        hi_raw = _BAND_EDGES[i + 1]
        hi = hi_raw if hi_raw is not None else nyquist
        hi = min(hi, nyquist)
        band_mids.append((lo + hi) / 2.0)

    total_energy = sum(fingerprint)
    if total_energy <= 0:
        return 0.0

    centroid = sum(fingerprint[i] * band_mids[i] for i in range(8)) / total_energy

    # Map to -1..1 relative to Nyquist
    # Midpoint of log-frequency range ≈ 1–2 kHz is "neutral"
    neutral_hz = 1500.0
    log_neutral = math.log2(neutral_hz / 50.0)
    log_max = math.log2(nyquist / 50.0) if nyquist > 50 else 1.0
    log_centroid = math.log2(max(centroid, 50.0) / 50.0)

    score = (log_centroid - log_neutral) / max(log_max / 2.0, 0.1)
    return max(-1.0, min(1.0, score))


def estimate_tempo(onset_times_ms: List[float]) -> float:
    """
    Infer tempo from inter-onset intervals.
    Cluster IOIs, pick most common → BPM estimate.
    Returns 0.0 if not enough onsets.
    """
    if len(onset_times_ms) < 4:
        return 0.0

    iois = [onset_times_ms[i + 1] - onset_times_ms[i]
            for i in range(len(onset_times_ms) - 1)]

    # Filter IOIs in plausible range: 100ms (600 BPM) to 2000ms (30 BPM)
    iois = [ioi for ioi in iois if 100.0 <= ioi <= 2000.0]
    if len(iois) < 3:
        return 0.0

    # Cluster by rounding to nearest 10ms
    bucket_size = 10.0
    counts: Dict[int, int] = {}
    for ioi in iois:
        bucket = round(ioi / bucket_size)
        counts[bucket] = counts.get(bucket, 0) + 1

    # Also check 2× and 0.5× harmonics of each bucket (deal with subdivisions)
    best_bucket = max(counts, key=lambda k: counts[k])
    best_ioi_ms = best_bucket * bucket_size

    bpm = 60000.0 / best_ioi_ms
    # Normalize to 60-300 BPM range by doubling/halving
    while bpm < 60.0:
        bpm *= 2.0
    while bpm > 300.0:
        bpm /= 2.0

    return round(bpm, 1)


def dynamic_profile(samples: List[float], sample_rate: int) -> str:
    """
    Characterize amplitude envelope as 'percussive', 'sustained', or 'evolving'.
    Uses RMS in 50ms windows.
    """
    if not samples or sample_rate <= 0:
        return "sustained"

    hop = max(1, int(sample_rate * 0.05))  # 50ms
    rms_vals = _rms_windows(samples, hop, hop)

    if len(rms_vals) < 4:
        return "sustained"

    # Peak position relative to total length
    peak_idx = max(range(len(rms_vals)), key=lambda i: rms_vals[i])
    peak_rel = peak_idx / len(rms_vals)

    # Attack time (how fast we reach peak)
    peak_val = rms_vals[peak_idx]
    attack_samples = next(
        (i for i in range(peak_idx + 1)
         if rms_vals[i] >= peak_val * 0.5),
        peak_idx
    )
    attack_rel = attack_samples / max(len(rms_vals), 1)

    # Tail: energy after peak vs peak
    if peak_idx < len(rms_vals) - 1:
        tail_mean = sum(rms_vals[peak_idx + 1:]) / (len(rms_vals) - peak_idx - 1)
        tail_ratio = tail_mean / max(peak_val, 1e-10)
    else:
        tail_ratio = 0.0

    if attack_rel < 0.15 and tail_ratio < 0.3:
        return "percussive"
    elif tail_ratio > 0.5:
        return "sustained"
    else:
        return "evolving"


def zero_crossing_rate(samples: List[float]) -> float:
    """Return zero-crossing rate (crossings per sample)."""
    if len(samples) < 2:
        return 0.0
    crossings = sum(1 for i in range(1, len(samples))
                    if (samples[i] >= 0.0) != (samples[i - 1] >= 0.0))
    return crossings / len(samples)


def analyze_audio(audio: AudioData) -> dict:
    """Run the full analysis pipeline and return a dict of results."""
    samples = audio.samples
    sr = audio.sample_rate

    fingerprint = spectral_fingerprint(samples, sr)
    felix_oscar = compute_felix_oscar(samples, sr, fingerprint)
    onsets = detect_onsets(samples, sr)
    pitch_hz, pitch_conf = detect_pitch(samples, sr)
    tempo_bpm = estimate_tempo(onsets)
    dyn_profile = dynamic_profile(samples, sr)
    zcr = zero_crossing_rate(samples)

    pitch_midi = _hz_to_midi(pitch_hz) if pitch_hz > 0 else None

    return {
        "spectral_fingerprint": [round(v, 4) for v in fingerprint],
        "felix_oscar_score": round(felix_oscar, 4),
        "detected_pitch_hz": round(pitch_hz, 2) if pitch_hz > 0 else None,
        "detected_pitch_midi": pitch_midi,
        "pitch_confidence": round(pitch_conf, 3),
        "detected_tempo_bpm": tempo_bpm if tempo_bpm > 0 else None,
        "onset_count": len(onsets),
        "onset_times_ms": [round(t, 1) for t in onsets],
        "dynamic_profile": dyn_profile,
        "zero_crossing_rate": round(zcr, 5),
        "duration_s": round(audio.duration_s, 3),
        "sample_rate": sr,
    }


def choose_mode(analysis: dict) -> str:
    """
    Determine best generation mode from analysis results.
    Returns 'kit', 'keygroup', or 'both'.
    """
    onset_count = analysis["onset_count"]
    zcr = analysis["zero_crossing_rate"]
    pitch_hz = analysis["detected_pitch_hz"]
    pitch_conf = analysis.get("pitch_confidence", 0.0)
    dyn = analysis["dynamic_profile"]

    is_percussive = dyn == "percussive" or zcr > 0.2
    has_pitch = pitch_hz is not None and pitch_conf > 0.4
    has_onsets = onset_count >= 4

    if has_onsets and is_percussive and not has_pitch:
        return "kit"
    elif has_pitch and not is_percussive:
        return "keygroup"
    elif has_pitch and has_onsets:
        return "both"
    elif has_onsets:
        return "kit"
    else:
        return "keygroup"


# ===========================================================================
# AUDIO CLIP EXTRACTION
# ===========================================================================

def _normalize_clip(samples: List[float]) -> List[float]:
    """Peak-normalize a clip to 0 dBFS headroom."""
    peak = max(abs(s) for s in samples) if samples else 0.0
    if peak < 1e-8:
        return samples
    scale = 0.98 / peak
    return [s * scale for s in samples]


def _fade_in_out(samples: List[float], fade_samples: int) -> List[float]:
    """Apply linear fade-in and fade-out."""
    n = len(samples)
    fade = min(fade_samples, n // 8)
    result = list(samples)
    for i in range(fade):
        ramp = i / max(fade, 1)
        result[i] = result[i] * ramp
        result[n - 1 - i] = result[n - 1 - i] * ramp
    return result


def _write_wav(path: Path, samples: List[float], sample_rate: int,
               bit_depth: int = 16):
    """Write a mono float sample list to a 16-bit PCM WAV file."""
    path.parent.mkdir(parents=True, exist_ok=True)
    n = len(samples)
    with wave.open(str(path), 'wb') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)  # always write 16-bit output
        wf.setframerate(sample_rate)
        # Convert to 16-bit
        ints = []
        for s in samples:
            v = max(-1.0, min(1.0, s))
            ints.append(int(v * 32767))
        data = struct.pack(f'<{n}h', *ints)
        wf.writeframes(data)


def extract_onset_clips(audio: AudioData, onset_times_ms: List[float],
                        num_pads: int = 8,
                        clip_ms: float = 400.0) -> List[Tuple[int, str, List[float]]]:
    """
    Extract clips centered on detected onsets.
    Returns list of (dominant_band_idx, band_name, clip_samples).
    """
    samples = audio.samples
    sr = audio.sample_rate
    clip_samples = int(sr * clip_ms / 1000.0)

    # Pick evenly spaced onsets if we have more than num_pads
    if len(onset_times_ms) > num_pads:
        step = len(onset_times_ms) / num_pads
        picked = [onset_times_ms[int(i * step)] for i in range(num_pads)]
    else:
        picked = onset_times_ms[:num_pads]

    # Pad to num_pads with evenly spaced fallback positions
    if len(picked) < num_pads:
        duration_ms = audio.duration_s * 1000.0
        extra_step = duration_ms / max(num_pads + 1, 2)
        for i in range(num_pads - len(picked)):
            picked.append(extra_step * (len(picked) + 1))

    # Pre-compute fingerprint for the whole file (for band assignment)
    whole_fp = spectral_fingerprint(samples, sr)

    clips = []
    for onset_ms in picked:
        center = int(sr * onset_ms / 1000.0)
        # Clip starts at onset (not centered) — typical for drum hits
        start = max(0, center)
        end = min(len(samples), start + clip_samples)
        if end - start < 64:
            # Too short, expand backward
            start = max(0, end - clip_samples)
        clip = samples[start:end]

        # Compute band fingerprint for this clip
        clip_fp = spectral_fingerprint(clip, sr)
        dom_band = max(range(8), key=lambda i: clip_fp[i]) if clip_fp else 0

        clip = _normalize_clip(clip)
        clip = _fade_in_out(clip, min(256, len(clip) // 8))
        clips.append((dom_band, _BAND_NAMES[dom_band], clip))

    return clips


def extract_pitched_segment(audio: AudioData, pitch_hz: float,
                            min_ms: float = 500.0,
                            max_ms: float = 4000.0) -> Tuple[List[float], int, int]:
    """
    Extract the most stable pitched segment.
    Returns (clip_samples, loop_start, loop_end).
    loop_start/loop_end are sample-accurate zero-crossing aligned.
    """
    samples = audio.samples
    sr = audio.sample_rate
    n = len(samples)

    min_len = int(sr * min_ms / 1000.0)
    max_len = int(sr * max_ms / 1000.0)

    if n <= min_len:
        # Use the whole file
        clip = list(samples)
    else:
        # Find the lowest-ZCR window of min_len samples
        step = max(1, (n - min_len) // 40)
        best_zcr = float('inf')
        best_start = 0
        for start in range(0, n - min_len, step):
            chunk = samples[start:start + min_len]
            zcr = zero_crossing_rate(chunk)
            if zcr < best_zcr:
                best_zcr = zcr
                best_start = start

        end = min(n, best_start + max_len)
        clip = samples[best_start:end]

    clip = _normalize_clip(clip)

    # Detect loop points in sustained region
    # Use middle third for looping (skip attack and release)
    loop_region_start = len(clip) // 3
    loop_region_end = 2 * len(clip) // 3

    if pitch_hz > 0:
        # Align loop points to zero crossings near ideal loop boundaries
        period_samples = int(sr / pitch_hz)
        loop_len_periods = max(4, (loop_region_end - loop_region_start) // period_samples)
        ideal_loop_len = period_samples * loop_len_periods
        ideal_start = loop_region_start
        ideal_end = min(len(clip) - 1, ideal_start + ideal_loop_len)

        # Snap to nearest zero crossing
        def _nearest_zc(pos: int, direction: int = 1) -> int:
            search_range = min(period_samples, 512)
            for offset in range(search_range):
                i = pos + offset * direction
                if 0 < i < len(clip) - 1:
                    if (clip[i - 1] < 0 and clip[i] >= 0) or (clip[i - 1] >= 0 and clip[i] < 0):
                        return i
            return pos

        loop_start = _nearest_zc(ideal_start, 1)
        loop_end = _nearest_zc(ideal_end, -1)

        if loop_end <= loop_start:
            loop_end = min(len(clip) - 1, loop_start + ideal_loop_len)
    else:
        loop_start = loop_region_start
        loop_end = loop_region_end

    clip = _fade_in_out(clip, min(256, len(clip) // 8))
    return clip, loop_start, loop_end


# ===========================================================================
# XPM GENERATION
# ===========================================================================

def _kit_qlink_xml() -> str:
    return (
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>PITCH</Name>\n'
        '        <Parameter>TuneCoarse</Parameter>\n'
        '        <Min>-12</Min>\n'
        '        <Max>12</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>BITE</Name>\n'
        '        <Parameter>Resonance</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.600000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SPACE</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
    )


def _keygroup_qlink_xml() -> str:
    return (
        '    <QLinks>\n'
        '      <QLink number="1">\n'
        '        <Name>TONE</Name>\n'
        '        <Parameter>FilterCutoff</Parameter>\n'
        '        <Min>0.200000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="2">\n'
        '        <Name>ATTACK</Name>\n'
        '        <Parameter>VolumeAttack</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>1.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="3">\n'
        '        <Name>RELEASE</Name>\n'
        '        <Parameter>VolumeRelease</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>2.000000</Max>\n'
        '      </QLink>\n'
        '      <QLink number="4">\n'
        '        <Name>SPACE</Name>\n'
        '        <Parameter>Send1</Parameter>\n'
        '        <Min>0.000000</Min>\n'
        '        <Max>0.700000</Max>\n'
        '      </QLink>\n'
        '    </QLinks>\n'
    )


def _velocity_to_filter_for_band(band_idx: int) -> float:
    """Higher bands → more filter velocity tracking (presence/air benefit from opening)."""
    return round(max(0.0, (band_idx - 2) * 0.07), 2)


def _decay_for_band(band_idx: int) -> float:
    """Sub/bass bands decay slower, high bands decay faster."""
    decays = [0.8, 0.6, 0.45, 0.35, 0.25, 0.18, 0.12, 0.08]
    return decays[min(band_idx, 7)]


def _cutoff_for_band(band_idx: int) -> float:
    """Cutoff tracks band — low bands are closed, high bands are open."""
    return round(0.3 + band_idx * 0.10, 2)


def _felix_oscar_vel_layers(felix_oscar: float) -> List[Tuple[int, int, float]]:
    """
    feliX (high centroid) → steep curve (transient, responsive).
    Oscar (low centroid) → linear curve (warm, forgiving).
    Returns [(vel_start, vel_end, volume), ...] × 4 layers.
    """
    # Normalize 0-1
    f = (felix_oscar + 1.0) / 2.0  # 0=Oscar, 1=feliX

    # feliX: hard hits get loud fast (aggressive lower split points)
    # Oscar: linear, equal bands
    if f >= 0.5:
        # Interpolate toward steep curve
        t = (f - 0.5) * 2.0
        layers = [
            (1,  int(15 + (1 - t) * 16), 0.30 + t * 0.05),
            (int(16 + (1 - t) * 16), int(35 + (1 - t) * 28), 0.55 - t * 0.05),
            (int(36 + (1 - t) * 28), int(65 + (1 - t) * 30), 0.78 + t * 0.02),
            (int(66 + (1 - t) * 29), 127, 0.97),
        ]
    else:
        # Interpolate toward linear
        t = f * 2.0  # 0=pure Oscar, 1=center
        layers = [
            (1,  int(32 - (1 - t) * 0), 0.35),
            (int(33 - (1 - t) * 0), 63, 0.55),
            (64, 95, 0.75),
            (96, 127, 0.97),
        ]

    # Clamp and ensure non-overlapping
    result = []
    prev_end = 0
    for i, (vs, ve, vol) in enumerate(layers):
        vs = max(prev_end + 1, vs) if i > 0 else max(1, vs)
        ve = max(vs + 1, min(127, ve))
        result.append((vs, ve, round(vol, 4)))
        prev_end = ve

    # Ensure last layer ends at 127
    if result:
        last = result[-1]
        result[-1] = (last[0], 127, last[2])

    return result


def _empty_drum_layer() -> str:
    return (
        '          <Layer number="1">\n'
        '            <Active>False</Active>\n'
        '            <SampleName></SampleName>\n'
        '            <SampleFile></SampleFile>\n'
        '            <File></File>\n'
        '            <VelStart>0</VelStart>\n'
        '            <VelEnd>0</VelEnd>\n'
        '          </Layer>'
    )


def _drum_layer_block(layer_num: int, vel_start: int, vel_end: int,
                      volume: float, sample_name: str, sample_file: str,
                      file_path: str) -> str:
    return (
        f'          <Layer number="{layer_num}">\n'
        f'            <Active>True</Active>\n'
        f'            <Volume>{_fmt(volume)}</Volume>\n'
        f'            <Pan>0.500000</Pan>\n'
        f'            <Pitch>0.000000</Pitch>\n'
        f'            <TuneCoarse>0</TuneCoarse>\n'
        f'            <TuneFine>0</TuneFine>\n'
        f'            <SampleName>{xml_escape(sample_name)}</SampleName>\n'
        f'            <SampleFile>{xml_escape(sample_file)}</SampleFile>\n'
        f'            <File>{xml_escape(file_path)}</File>\n'
        f'            <RootNote>0</RootNote>\n'
        f'            <KeyTrack>True</KeyTrack>\n'
        f'            <OneShot>True</OneShot>\n'
        f'            <Loop>False</Loop>\n'
        f'            <LoopStart>0</LoopStart>\n'
        f'            <LoopEnd>0</LoopEnd>\n'
        f'            <LoopXFade>0</LoopXFade>\n'
        f'            <VolumeAttack>0.000000</VolumeAttack>\n'
        f'            <VolumeDecay>0.000000</VolumeDecay>\n'
        f'            <VolumeSustain>1.000000</VolumeSustain>\n'
        f'            <VolumeRelease>0.050000</VolumeRelease>\n'
        f'            <VelStart>{vel_start}</VelStart>\n'
        f'            <VelEnd>{vel_end}</VelEnd>\n'
        f'          </Layer>'
    )


def generate_kit_xpm(
    monster_name: str,
    pad_samples: List[dict],  # list of {sample_name, sample_file, file_path, band_idx, active}
    felix_oscar: float,
    fingerprint: List[float],
    midi_notes: Optional[List[int]] = None,
) -> str:
    """
    Generate a Drum-type XPM for the Monster Rancher kit output.
    pad_samples: up to 16 entries, assigned to pads in frequency order.
    """
    prog_name = xml_escape(f"Monster-{monster_name}"[:32])

    # Default MIDI layout: chromatic from C1 (36)
    if midi_notes is None:
        midi_notes = [36 + i for i in range(16)]

    # Velocity layers shaped by feliX-Oscar score
    vel_layers = _felix_oscar_vel_layers(felix_oscar)

    # Build pad note map (pad 1-16)
    pad_note_entries = []
    for pad_idx in range(min(len(pad_samples), 16)):
        note = midi_notes[pad_idx] if pad_idx < len(midi_notes) else 36 + pad_idx
        pad_note_entries.append(f'        <Pad number="{pad_idx + 1}" note="{note}"/>')

    # Build 128-instrument block
    instruments_parts = []
    note_to_pad = {}
    for pad_idx in range(min(len(pad_samples), 16)):
        note = midi_notes[pad_idx] if pad_idx < len(midi_notes) else 36 + pad_idx
        note_to_pad[note] = pad_idx

    for i in range(128):
        if i in note_to_pad:
            pad_idx = note_to_pad[i]
            ps = pad_samples[pad_idx]
            band_idx = ps.get("band_idx", 0)
            is_active = ps.get("active", False)

            decay = _decay_for_band(band_idx)
            cutoff = _cutoff_for_band(band_idx)
            vel_to_filter = _velocity_to_filter_for_band(band_idx)

            if is_active:
                sn = ps.get("sample_name", "")
                sf = ps.get("sample_file", "")
                fp = ps.get("file_path", "")
                # 4 velocity layers for expressive playing
                layers_xml = "\n".join(
                    _drum_layer_block(
                        layer_num=j + 1,
                        vel_start=vel_layers[j][0],
                        vel_end=vel_layers[j][1],
                        volume=vel_layers[j][2],
                        sample_name=sn,
                        sample_file=sf,
                        file_path=fp,
                    )
                    for j in range(4)
                )
            else:
                layers_xml = _empty_drum_layer()

            instruments_parts.append(
                f'      <Instrument number="{i}">\n'
                f'        <AudioRoute>\n'
                f'          <AudioRoute>0</AudioRoute>\n'
                f'          <AudioRouteSubIndex>0</AudioRouteSubIndex>\n'
                f'          <InsertsEnabled>False</InsertsEnabled>\n'
                f'        </AudioRoute>\n'
                f'        <Send1>0.000000</Send1>\n'
                f'        <Send2>0.000000</Send2>\n'
                f'        <Send3>0.000000</Send3>\n'
                f'        <Send4>0.000000</Send4>\n'
                f'        <Volume>0.707946</Volume>\n'
                f'        <Mute>False</Mute>\n'
                f'        <Pan>0.500000</Pan>\n'
                f'        <TuneCoarse>0</TuneCoarse>\n'
                f'        <TuneFine>0</TuneFine>\n'
                f'        <Mono>True</Mono>\n'
                f'        <Polyphony>1</Polyphony>\n'
                f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
                f'        <LowNote>0</LowNote>\n'
                f'        <HighNote>127</HighNote>\n'
                f'        <IgnoreBaseNote>False</IgnoreBaseNote>\n'
                f'        <ZonePlay>1</ZonePlay>\n'
                f'        <MuteGroup>0</MuteGroup>\n'
                f'        <MuteTarget1>0</MuteTarget1>\n'
                f'        <MuteTarget2>0</MuteTarget2>\n'
                f'        <MuteTarget3>0</MuteTarget3>\n'
                f'        <MuteTarget4>0</MuteTarget4>\n'
                f'        <SimultTarget1>0</SimultTarget1>\n'
                f'        <SimultTarget2>0</SimultTarget2>\n'
                f'        <SimultTarget3>0</SimultTarget3>\n'
                f'        <SimultTarget4>0</SimultTarget4>\n'
                f'        <LfoPitch>0.000000</LfoPitch>\n'
                f'        <LfoCutoff>0.000000</LfoCutoff>\n'
                f'        <LfoVolume>0.000000</LfoVolume>\n'
                f'        <LfoPan>0.000000</LfoPan>\n'
                f'        <OneShot>True</OneShot>\n'
                f'        <FilterType>2</FilterType>\n'
                f'        <Cutoff>{cutoff:.6f}</Cutoff>\n'
                f'        <Resonance>0.050000</Resonance>\n'
                f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
                f'        <AfterTouchToFilter>0.000000</AfterTouchToFilter>\n'
                f'        <VelocityToStart>0.000000</VelocityToStart>\n'
                f'        <VelocityToFilterAttack>0.000000</VelocityToFilterAttack>\n'
                f'        <VelocityToFilter>{vel_to_filter:.6f}</VelocityToFilter>\n'
                f'        <VelocityToFilterEnvelope>0.000000</VelocityToFilterEnvelope>\n'
                f'        <FilterAttack>0.000000</FilterAttack>\n'
                f'        <FilterDecay>0.000000</FilterDecay>\n'
                f'        <FilterSustain>1.000000</FilterSustain>\n'
                f'        <FilterRelease>0.000000</FilterRelease>\n'
                f'        <FilterHold>0.000000</FilterHold>\n'
                f'        <FilterDecayType>True</FilterDecayType>\n'
                f'        <FilterADEnvelope>True</FilterADEnvelope>\n'
                f'        <VolumeHold>0.000000</VolumeHold>\n'
                f'        <VolumeDecayType>True</VolumeDecayType>\n'
                f'        <VolumeADEnvelope>True</VolumeADEnvelope>\n'
                f'        <VolumeAttack>0.000000</VolumeAttack>\n'
                f'        <VolumeDecay>{decay:.6f}</VolumeDecay>\n'
                f'        <VolumeSustain>0.000000</VolumeSustain>\n'
                f'        <VolumeRelease>0.050000</VolumeRelease>\n'
                f'        <VelocityToPitch>0.000000</VelocityToPitch>\n'
                f'        <VelocityToVolumeAttack>0.000000</VelocityToVolumeAttack>\n'
                f'        <VelocitySensitivity>1.000000</VelocitySensitivity>\n'
                f'        <VelocityToPan>0.000000</VelocityToPan>\n'
                f'        <LFO>\n'
                f'          <Speed>0.000000</Speed>\n'
                f'          <Amount>0.000000</Amount>\n'
                f'          <Type>0</Type>\n'
                f'          <Sync>False</Sync>\n'
                f'          <Retrigger>True</Retrigger>\n'
                f'        </LFO>\n'
                f'        <Layers>\n'
                f'{layers_xml}\n'
                f'        </Layers>\n'
                f'      </Instrument>'
            )
        else:
            # Unused instrument — empty slot
            empty_layer = _empty_drum_layer()
            instruments_parts.append(
                f'      <Instrument number="{i}">\n'
                f'        <Volume>0.707946</Volume>\n'
                f'        <Pan>0.500000</Pan>\n'
                f'        <TuneCoarse>0</TuneCoarse>\n'
                f'        <TuneFine>0</TuneFine>\n'
                f'        <Mono>True</Mono>\n'
                f'        <Polyphony>1</Polyphony>\n'
                f'        <FilterKeytrack>0.000000</FilterKeytrack>\n'
                f'        <LowNote>0</LowNote>\n'
                f'        <HighNote>127</HighNote>\n'
                f'        <ZonePlay>1</ZonePlay>\n'
                f'        <MuteGroup>0</MuteGroup>\n'
                f'        <OneShot>True</OneShot>\n'
                f'        <FilterType>2</FilterType>\n'
                f'        <Cutoff>1.000000</Cutoff>\n'
                f'        <Resonance>0.000000</Resonance>\n'
                f'        <VolumeAttack>0.000000</VolumeAttack>\n'
                f'        <VolumeDecay>0.300000</VolumeDecay>\n'
                f'        <VolumeSustain>0.000000</VolumeSustain>\n'
                f'        <VolumeRelease>0.050000</VolumeRelease>\n'
                f'        <Layers>\n'
                f'{empty_layer}\n'
                f'        </Layers>\n'
                f'      </Instrument>'
            )

    instruments_xml = "\n".join(instruments_parts)
    pad_note_xml = "\n".join(pad_note_entries)
    qlink_xml = _kit_qlink_xml()

    import json as _json
    pad_json = _json.dumps(
        {"ProgramPads": {"Universal": {"value0": False},
                         "Type": {"value0": 5},
                         "universalPad": 32512}},
        separators=(",", ":"),
    )

    return (
        f'{_XPM_HEADER}\n'
        f'  <Program type="Drum">\n'
        f'    <Name>{prog_name}</Name>\n'
        f'    <ProgramPads>{pad_json}</ProgramPads>\n'
        f'    <PadNoteMap>\n'
        f'{pad_note_xml}\n'
        f'    </PadNoteMap>\n'
        f'    <PadGroupMap>\n'
        f'    </PadGroupMap>\n'
        f'{qlink_xml}'
        f'    <Instruments>\n'
        f'{instruments_xml}\n'
        f'    </Instruments>\n'
        f'  </Program>\n'
        f'</MPCVObject>\n'
    )


def generate_keygroup_xpm(
    monster_name: str,
    sample_name: str,
    sample_file: str,
    file_path: str,
    root_midi: int,
    loop_start: int,
    loop_end: int,
    dynamic_profile_str: str,
    felix_oscar: float,
) -> str:
    """Generate a Keygroup-type XPM for the Monster Rancher keygroup output."""
    prog_name = xml_escape(f"Monster-{monster_name}"[:32])

    # Map root MIDI to low/high zone spanning full keyboard
    low_note = 0
    high_note = 127

    # Envelope from dynamic profile
    if dynamic_profile_str == "percussive":
        attack = 0.0
        decay = 0.3
        sustain = 0.0
        release = 0.1
    elif dynamic_profile_str == "sustained":
        attack = 0.01
        decay = 0.0
        sustain = 1.0
        release = 0.5
    else:  # evolving
        attack = 0.05
        decay = 0.2
        sustain = 0.6
        release = 0.4

    # Velocity layers from feliX-Oscar
    vel_layers = _felix_oscar_vel_layers(felix_oscar)

    has_loop = loop_end > loop_start + 64
    loop_str = "True" if has_loop else "False"
    loop_xfade = min(256, (loop_end - loop_start) // 8) if has_loop else 0

    # Build 4 velocity layers
    layers_xml = ""
    for j, (vs, ve, vol) in enumerate(vel_layers):
        layers_xml += (
            f'            <Layer number="{j + 1}">\n'
            f'              <Active>True</Active>\n'
            f'              <Volume>{_fmt(vol)}</Volume>\n'
            f'              <Pan>0.500000</Pan>\n'
            f'              <Pitch>0.000000</Pitch>\n'
            f'              <TuneCoarse>0</TuneCoarse>\n'
            f'              <TuneFine>0</TuneFine>\n'
            f'              <SampleName>{xml_escape(sample_name)}</SampleName>\n'
            f'              <SampleFile>{xml_escape(sample_file)}</SampleFile>\n'
            f'              <File>{xml_escape(file_path)}</File>\n'
            f'              <RootNote>{root_midi}</RootNote>\n'
            f'              <KeyTrack>True</KeyTrack>\n'
            f'              <OneShot>False</OneShot>\n'
            f'              <Loop>{loop_str}</Loop>\n'
            f'              <LoopStart>{loop_start}</LoopStart>\n'
            f'              <LoopEnd>{loop_end}</LoopEnd>\n'
            f'              <LoopXFade>{loop_xfade}</LoopXFade>\n'
            f'              <VolumeAttack>{_fmt(attack)}</VolumeAttack>\n'
            f'              <VolumeDecay>{_fmt(decay)}</VolumeDecay>\n'
            f'              <VolumeSustain>{_fmt(sustain)}</VolumeSustain>\n'
            f'              <VolumeRelease>{_fmt(release)}</VolumeRelease>\n'
            f'              <VelStart>{vs}</VelStart>\n'
            f'              <VelEnd>{ve}</VelEnd>\n'
            f'            </Layer>\n'
        )

    instrument_xml = (
        f'      <Instrument number="0">\n'
        f'        <Active>True</Active>\n'
        f'        <Volume>1.000000</Volume>\n'
        f'        <Pan>0.500000</Pan>\n'
        f'        <Tune>0</Tune>\n'
        f'        <Transpose>0</Transpose>\n'
        f'        <VolumeAttack>{_fmt(attack)}</VolumeAttack>\n'
        f'        <VolumeHold>0.000000</VolumeHold>\n'
        f'        <VolumeDecay>{_fmt(decay)}</VolumeDecay>\n'
        f'        <VolumeSustain>{_fmt(sustain)}</VolumeSustain>\n'
        f'        <VolumeRelease>{_fmt(release)}</VolumeRelease>\n'
        f'        <FilterType>2</FilterType>\n'
        f'        <Cutoff>1.000000</Cutoff>\n'
        f'        <Resonance>0.000000</Resonance>\n'
        f'        <FilterEnvAmt>0.000000</FilterEnvAmt>\n'
        f'        <LowNote>{low_note}</LowNote>\n'
        f'        <HighNote>{high_note}</HighNote>\n'
        f'        <RootNote>{root_midi}</RootNote>\n'
        f'        <KeyTrack>True</KeyTrack>\n'
        f'        <OneShot>False</OneShot>\n'
        f'        <Layers>\n'
        f'{layers_xml}'
        f'        </Layers>\n'
        f'      </Instrument>\n'
    )

    qlink_xml = _keygroup_qlink_xml()

    return (
        f'{_XPM_HEADER}\n'
        f'  <Program type="Keygroup">\n'
        f'    <Name>{prog_name}</Name>\n'
        f'    <KeygroupNumKeygroups>1</KeygroupNumKeygroups>\n'
        f'    <KeygroupPitchBendRange>2</KeygroupPitchBendRange>\n'
        f'    <KeygroupWheelToLfo>0.000000</KeygroupWheelToLfo>\n'
        f'{qlink_xml}'
        f'    <Instruments>\n'
        f'{instrument_xml}'
        f'    </Instruments>\n'
        f'  </Program>\n'
        f'</MPCVObject>\n'
    )


# ===========================================================================
# DNA-DRIVEN MODE (no source audio)
# ===========================================================================

def generate_from_dna(dna: dict, engine: str, output_dir: Path,
                      num_pads: int = 8) -> dict:
    """
    Generate an XPM from Sonic DNA parameters only (no source audio).
    Creates a placeholder keygroup XPM whose parameters derive from the DNA.
    """
    output_dir.mkdir(parents=True, exist_ok=True)

    brightness = dna.get("brightness", 0.5)
    warmth = dna.get("warmth", 0.5)
    aggression = dna.get("aggression", 0.5)
    movement = dna.get("movement", 0.5)
    density = dna.get("density", 0.5)
    space = dna.get("space", 0.5)

    # Derive feliX-Oscar from brightness
    felix_oscar = (brightness - 0.5) * 2.0

    # Dynamic profile from aggression + movement
    if aggression > 0.6 and movement > 0.5:
        dyn = "percussive"
    elif warmth > 0.6 or density > 0.6:
        dyn = "sustained"
    else:
        dyn = "evolving"

    monster_name = f"DNA_{engine}"
    sample_name = f"Monster_{engine}_DNA"
    sample_file = f"{sample_name}.wav"
    file_path = f"Samples/{monster_name}/{sample_file}"

    # Root pitch from warmth (warm → low, bright → high)
    root_midi = int(40 + brightness * 40)

    xpm_content = generate_keygroup_xpm(
        monster_name=monster_name,
        sample_name=sample_name,
        sample_file=sample_file,
        file_path=file_path,
        root_midi=root_midi,
        loop_start=0,
        loop_end=0,
        dynamic_profile_str=dyn,
        felix_oscar=felix_oscar,
    )

    xpm_filename = f"Monster_{engine}_DNA.xpm"
    xpm_path = output_dir / xpm_filename
    xpm_path.write_text(xpm_content, encoding="utf-8")

    metadata = {
        "source_file": None,
        "dna_input": dna,
        "engine": engine,
        "analysis": {
            "spectral_fingerprint": [brightness, warmth, 0.5, 0.5, brightness, brightness * 0.8, brightness * 0.5, brightness * 0.2],
            "felix_oscar_score": round(felix_oscar, 4),
            "detected_pitch_hz": round(_midi_to_hz(root_midi), 2),
            "detected_pitch_midi": root_midi,
            "detected_tempo_bpm": None,
            "onset_count": 0,
            "dynamic_profile": dyn,
            "mode_chosen": "keygroup",
        },
        "generated": datetime.now(timezone.utc).isoformat(),
        "xpn_output": xpm_filename,
    }

    meta_path = output_dir / "monster_metadata.json"
    meta_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    print(f"  [DNA] XPM: {xpm_path}")
    print(f"  [DNA] Metadata: {meta_path}")
    return metadata


# ===========================================================================
# MAIN GENERATION PIPELINE
# ===========================================================================

def process_file(source_path: Path, output_dir: Path,
                 mode: str = "auto",
                 num_pads: int = 8,
                 root_note_override: Optional[int] = None) -> dict:
    """
    Full Monster Rancher pipeline for a single WAV file.
    Returns the metadata dict.
    """
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"[Monster Rancher] Scanning: {source_path.name}")

    audio = load_audio(source_path)
    if audio is None:
        print(f"  ERROR: Could not read audio from {source_path}", file=sys.stderr)
        return {}

    if audio.num_frames < 64:
        print(f"  WARNING: Very short file ({audio.num_frames} samples) — minimal analysis possible")

    print(f"  Duration: {audio.duration_s:.2f}s  SR: {audio.sample_rate}Hz  Depth: {audio.bit_depth}-bit")

    # --- Analysis ---
    print("  Analyzing...")
    analysis = analyze_audio(audio)

    print(f"  Onsets: {analysis['onset_count']}  Pitch: {analysis['detected_pitch_hz']} Hz  "
          f"feliX-Oscar: {analysis['felix_oscar_score']:+.2f}  "
          f"Profile: {analysis['dynamic_profile']}")

    # --- Mode selection ---
    if mode == "auto":
        chosen_mode = choose_mode(analysis)
    else:
        chosen_mode = mode

    analysis["mode_chosen"] = chosen_mode
    print(f"  Mode: {chosen_mode}")

    monster_name = source_path.stem.replace(" ", "_")
    samples_subdir = output_dir / "Samples" / monster_name
    samples_subdir.mkdir(parents=True, exist_ok=True)

    generated_files = []

    # -----------------------------------------------------------------------
    # KIT MODE
    # -----------------------------------------------------------------------
    if chosen_mode in ("kit", "both"):
        onset_times = analysis["onset_times_ms"]
        felix_oscar = analysis["felix_oscar_score"]
        fingerprint = analysis["spectral_fingerprint"]

        print(f"  Extracting {num_pads} pad clips...")
        clips = extract_onset_clips(audio, onset_times, num_pads=num_pads)

        # Sort clips by dominant band (frequency order: low → high)
        clips_sorted = sorted(clips, key=lambda c: c[0])

        pad_samples_list = []
        for pad_idx, (band_idx, band_name, clip) in enumerate(clips_sorted):
            pad_num = pad_idx + 1
            sample_basename = f"Monster_{monster_name}_{band_name}_{pad_num:02d}"
            wav_filename = f"{sample_basename}.wav"
            wav_path = samples_subdir / wav_filename
            _write_wav(wav_path, clip, audio.sample_rate)

            pad_samples_list.append({
                "sample_name": sample_basename,
                "sample_file": wav_filename,
                "file_path": f"Samples/{monster_name}/{wav_filename}",
                "band_idx": band_idx,
                "active": True,
            })
            print(f"    Pad {pad_num:2d}: {band_name} → {wav_filename}")

        # MIDI notes: C1 chromatic upward from 36
        midi_notes = [36 + i for i in range(len(pad_samples_list))]

        xpm_content = generate_kit_xpm(
            monster_name=monster_name,
            pad_samples=pad_samples_list,
            felix_oscar=felix_oscar,
            fingerprint=fingerprint,
            midi_notes=midi_notes,
        )

        xpm_filename = f"Monster_{monster_name}_Kit.xpm"
        xpm_path = output_dir / xpm_filename
        xpm_path.write_text(xpm_content, encoding="utf-8")
        generated_files.append(str(xpm_path))
        print(f"  Kit XPM: {xpm_path}")

    # -----------------------------------------------------------------------
    # KEYGROUP MODE
    # -----------------------------------------------------------------------
    if chosen_mode in ("keygroup", "both"):
        pitch_hz = analysis["detected_pitch_hz"] or 261.63  # default to C4
        pitch_conf = analysis.get("pitch_confidence", 0.0)

        if root_note_override is not None:
            root_midi = root_note_override
        else:
            root_midi = _hz_to_midi(pitch_hz) if pitch_hz > 0 else 60

        root_note_name = _midi_to_note_name(root_midi)
        print(f"  Root note: {root_note_name} (MIDI {root_midi}, {pitch_hz:.1f} Hz, conf={pitch_conf:.2f})")

        print("  Extracting pitched segment...")
        pitched_clip, loop_start, loop_end = extract_pitched_segment(
            audio, pitch_hz
        )

        sample_basename = f"Monster_{monster_name}_{root_note_name}"
        wav_filename = f"{sample_basename}.wav"
        wav_path = samples_subdir / wav_filename
        _write_wav(wav_path, pitched_clip, audio.sample_rate)
        print(f"  Keygroup sample: {wav_filename}  Loop: {loop_start}–{loop_end}")

        file_path_str = f"Samples/{monster_name}/{wav_filename}"

        xpm_content = generate_keygroup_xpm(
            monster_name=monster_name,
            sample_name=sample_basename,
            sample_file=wav_filename,
            file_path=file_path_str,
            root_midi=root_midi,
            loop_start=loop_start,
            loop_end=loop_end,
            dynamic_profile_str=analysis["dynamic_profile"],
            felix_oscar=analysis["felix_oscar_score"],
        )

        xpm_filename = f"Monster_{monster_name}_Keygroup.xpm"
        xpm_path = output_dir / xpm_filename
        xpm_path.write_text(xpm_content, encoding="utf-8")
        generated_files.append(str(xpm_path))
        print(f"  Keygroup XPM: {xpm_path}")

    # -----------------------------------------------------------------------
    # Metadata
    # -----------------------------------------------------------------------
    metadata = {
        "source_file": str(source_path),
        "analysis": analysis,
        "generated": datetime.now(timezone.utc).isoformat(),
        "xpn_output": generated_files[0] if len(generated_files) == 1 else generated_files,
    }

    meta_path = output_dir / "monster_metadata.json"
    meta_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")
    print(f"  Metadata: {meta_path}")

    return metadata


def batch_process(source_dir: Path, output_dir: Path,
                  mode: str = "auto", num_pads: int = 8) -> List[dict]:
    """Process all WAV files in a directory."""
    wav_files = sorted(source_dir.glob("*.wav")) + sorted(source_dir.glob("*.WAV"))
    if not wav_files:
        print(f"No WAV files found in {source_dir}", file=sys.stderr)
        return []

    results = []
    for wav_path in wav_files:
        sub_output = output_dir / wav_path.stem
        meta = process_file(wav_path, sub_output, mode=mode, num_pads=num_pads)
        results.append(meta)

    return results


# ===========================================================================
# CLI
# ===========================================================================

def main():
    parser = argparse.ArgumentParser(
        description="XPN Monster Rancher — scan any audio and generate a monster XPM kit or keygroup.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    parser.add_argument(
        "source",
        nargs="?",
        type=Path,
        help="Source WAV file or directory (with --batch). Omit when using --dna.",
    )
    parser.add_argument(
        "--output", "-o",
        type=Path,
        default=Path("./monster_output"),
        help="Output directory (default: ./monster_output)",
    )
    parser.add_argument(
        "--mode", "-m",
        choices=["auto", "kit", "keygroup", "both"],
        default="auto",
        help="Generation mode: auto (default), kit, keygroup, or both",
    )
    parser.add_argument(
        "--pads",
        type=int,
        default=8,
        choices=[4, 8, 12, 16],
        help="Number of pads for kit mode (default: 8)",
    )
    parser.add_argument(
        "--root-note",
        default="auto",
        help="Root note for keygroup mode: 'auto' or MIDI number 0-127 (default: auto)",
    )
    parser.add_argument(
        "--batch",
        action="store_true",
        help="Process all WAV files in source directory",
    )
    parser.add_argument(
        "--dna",
        type=str,
        default=None,
        help='Sonic DNA JSON string, e.g. \'{"brightness": 0.8, "warmth": 0.3}\'. '
             'Generates XPM from DNA alone (no source audio).',
    )
    parser.add_argument(
        "--engine",
        type=str,
        default="MONSTER",
        help="Engine name for DNA-driven mode (default: MONSTER)",
    )

    args = parser.parse_args()

    output_dir = args.output

    # -----------------------------------------------------------------------
    # DNA-only mode
    # -----------------------------------------------------------------------
    if args.dna is not None:
        try:
            dna_dict = json.loads(args.dna)
        except json.JSONDecodeError as e:
            print(f"ERROR: Invalid DNA JSON: {e}", file=sys.stderr)
            sys.exit(1)

        result = generate_from_dna(dna_dict, args.engine, output_dir)
        print("\nDone.")
        return

    # -----------------------------------------------------------------------
    # Require source for non-DNA modes
    # -----------------------------------------------------------------------
    if args.source is None:
        parser.error("source is required unless --dna is specified")

    if not args.source.exists():
        print(f"ERROR: Source not found: {args.source}", file=sys.stderr)
        sys.exit(1)

    # Parse root-note override
    root_note_override = None
    if args.root_note != "auto":
        try:
            root_note_override = int(args.root_note)
            if not (0 <= root_note_override <= 127):
                raise ValueError
        except ValueError:
            print(f"ERROR: --root-note must be 'auto' or 0-127, got: {args.root_note}",
                  file=sys.stderr)
            sys.exit(1)

    # -----------------------------------------------------------------------
    # Batch mode
    # -----------------------------------------------------------------------
    if args.batch:
        if not args.source.is_dir():
            print(f"ERROR: --batch requires a directory, got: {args.source}", file=sys.stderr)
            sys.exit(1)
        results = batch_process(
            source_dir=args.source,
            output_dir=output_dir,
            mode=args.mode,
            num_pads=args.pads,
        )
        print(f"\nBatch complete: {len(results)} files processed.")
        return

    # -----------------------------------------------------------------------
    # Single file mode
    # -----------------------------------------------------------------------
    if args.source.is_dir():
        print(f"ERROR: Source is a directory. Use --batch for directory processing.", file=sys.stderr)
        sys.exit(1)

    meta = process_file(
        source_path=args.source,
        output_dir=output_dir,
        mode=args.mode,
        num_pads=args.pads,
        root_note_override=root_note_override,
    )

    if not meta:
        sys.exit(1)

    print(f"\nMonster summoned. Files in: {output_dir}")


if __name__ == "__main__":
    main()
