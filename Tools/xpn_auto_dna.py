#!/usr/bin/env python3
"""
xpn_auto_dna.py — Automatically computes XOlokun 6D Sonic DNA values from WAV audio.

Analyzes a WAV file using only stdlib + wave module (no librosa, no numpy).
Delegates brightness/warmth/aggression to xpn_classify_instrument feature
functions; computes movement, density, and space from scratch using the same
DFT primitives already in the classifier.

6D Dimensions (all 0.0 to 1.0):
    brightness  : spectral centroid normalized — min(centroid_hz / 8000, 1.0)
    warmth      : inverse HF energy          — max(0, 1.0 - hf_energy_ratio * 2.0)
    movement    : spectral variance over time — normalized std dev of 8-window centroids
    density     : active spectral bins        — bins > -40 dB rel. peak / total bins
    space       : decay time estimate         — time to -40 dB below peak, normalized
    aggression  : 0.6 * transient_strength + 0.4 * hf_energy_ratio

Usage (CLI):
    python xpn_auto_dna.py sample.wav
    python xpn_auto_dna.py ./stems/          # directory scan with summary stats

Import:
    from xpn_auto_dna import compute_dna
    dna = compute_dna("/path/to/sample.wav")
    # {"brightness": 0.7, "warmth": 0.3, "movement": 0.4,
    #  "density": 0.6, "space": 0.2, "aggression": 0.8}
"""

import sys
import math
import json
import os
import argparse

# Pull the low-level feature functions from the classifier.
# These are pure math — no side effects.
from xpn_classify_instrument import (
    load_wav_mono,
    compute_dft_magnitudes,
    compute_spectral_centroid_hz,
    compute_hf_energy_ratio,
    compute_transient_strength,
    DFT_WINDOW,
)


# ---------------------------------------------------------------------------
# Movement — spectral centroid variance across 8 time windows
# ---------------------------------------------------------------------------

NUM_MOVEMENT_WINDOWS = 8


def compute_movement(mono: list[float], sample_rate: int) -> float:
    """
    Spectral movement = normalized std dev of per-window spectral centroids.

    Splits the audio into NUM_MOVEMENT_WINDOWS equal segments, computes the
    spectral centroid of each using a short DFT window (1024 samples), then
    returns the std dev of those centroids normalized against 2000 Hz.

    A static pad → low movement (centroid barely shifts across windows).
    A sweep or evolving texture → high movement (centroid travels widely).

    Returns: min(std_dev_hz / 2000.0, 1.0)
    """
    n_samples = len(mono)
    if n_samples == 0:
        return 0.0

    segment_len = max(1, n_samples // NUM_MOVEMENT_WINDOWS)
    dft_n = min(1024, segment_len)

    centroids = []
    for i in range(NUM_MOVEMENT_WINDOWS):
        start = i * segment_len
        end = start + segment_len
        segment = mono[start:end]
        if not segment:
            continue
        mags = compute_dft_magnitudes(segment, dft_n)
        c = compute_spectral_centroid_hz(mags, sample_rate)
        centroids.append(c)

    if len(centroids) < 2:
        return 0.0

    mean = sum(centroids) / len(centroids)
    variance = sum((c - mean) ** 2 for c in centroids) / len(centroids)
    std_dev = math.sqrt(variance)
    return min(std_dev / 2000.0, 1.0)


# ---------------------------------------------------------------------------
# Density — fraction of FFT bins above -40 dB relative to spectral peak
# ---------------------------------------------------------------------------

DENSITY_DB_THRESHOLD = -40.0  # dB below spectral magnitude peak


def compute_density(mono: list[float], sample_rate: int) -> float:
    """
    Spectral density = proportion of FFT bins carrying significant energy.

    Uses the full DFT window on the first DFT_WINDOW samples (attack region).
    A bin is "active" if its magnitude is within 40 dB of the peak bin.
    Thin spectral content (pure tone, sub bass) → low density.
    Broadband noise, complex textures → high density.

    Returns: active_bins / total_bins (range 0.0 to 1.0)
    """
    dft_n = min(DFT_WINDOW, len(mono))
    if dft_n == 0:
        return 0.0

    mags = compute_dft_magnitudes(mono, dft_n)
    if not mags:
        return 0.0

    peak_mag = max(mags)
    if peak_mag < 1e-12:
        return 0.0

    # -40 dB in linear magnitude = 10^(-40/20) ≈ 0.01 * peak
    threshold = peak_mag * (10.0 ** (DENSITY_DB_THRESHOLD / 20.0))
    active = sum(1 for m in mags if m >= threshold)
    return active / len(mags)


# ---------------------------------------------------------------------------
# Space — decay time normalized to 8 seconds
# ---------------------------------------------------------------------------

SPACE_DB_DROP   = -40.0   # dB: how far below peak RMS we measure the tail
SPACE_NORM_SECS = 8.0     # 8 seconds → 1.0
RMS_HOP_MS      = 20.0    # RMS window hop in milliseconds


def compute_space(mono: list[float], sample_rate: int) -> float:
    """
    Decay time estimate = time from peak RMS to -40 dB below peak RMS,
    normalized to SPACE_NORM_SECS.

    Algorithm:
      1. Compute short-time RMS in overlapping hops of RMS_HOP_MS.
      2. Find the frame with peak RMS.
      3. Scan forward from that frame until RMS drops 40 dB below peak
         (or until we run out of audio).
      4. Decay seconds = (end_frame - peak_frame) * hop_seconds.

    A dry percussive hit with no reverb → very short decay → low space.
    A washed reverb tail or long pad → long decay → high space.

    Returns: min(decay_seconds / 8.0, 1.0)
    """
    if not mono:
        return 0.0

    hop_samples = max(1, int(sample_rate * RMS_HOP_MS / 1000.0))
    n_samples = len(mono)

    # Build RMS envelope.
    rms_frames = []
    i = 0
    while i < n_samples:
        frame = mono[i : i + hop_samples]
        if not frame:
            break
        rms = math.sqrt(sum(s * s for s in frame) / len(frame))
        rms_frames.append(rms)
        i += hop_samples

    if not rms_frames:
        return 0.0

    peak_rms = max(rms_frames)
    if peak_rms < 1e-9:
        return 0.0

    peak_frame_idx = rms_frames.index(peak_rms)

    # Threshold: -40 dB in power = 10^(-40/20) in amplitude.
    decay_threshold = peak_rms * (10.0 ** (SPACE_DB_DROP / 20.0))

    # Walk forward from peak frame to find where RMS drops below threshold.
    decay_frame_idx = len(rms_frames) - 1  # default: reached end
    for j in range(peak_frame_idx, len(rms_frames)):
        if rms_frames[j] <= decay_threshold:
            decay_frame_idx = j
            break

    hop_seconds = hop_samples / sample_rate
    decay_seconds = (decay_frame_idx - peak_frame_idx) * hop_seconds
    return min(decay_seconds / SPACE_NORM_SECS, 1.0)


# ---------------------------------------------------------------------------
# Public API — compute_dna
# ---------------------------------------------------------------------------

def compute_dna(wav_path: str) -> dict:
    """
    Compute the 6D Sonic DNA for a WAV file.

    Returns a dict with keys: brightness, warmth, movement, density, space, aggression.
    All values are floats in [0.0, 1.0], rounded to 3 decimal places.

    Raises: FileNotFoundError, wave.Error, struct.error for bad/unsupported files.
    """
    mono, sample_rate = load_wav_mono(wav_path)
    if not mono:
        return {
            "brightness": 0.0,
            "warmth":     0.0,
            "movement":   0.0,
            "density":    0.0,
            "space":      0.0,
            "aggression": 0.0,
        }

    # --- Shared features (computed once, reused across dimensions) ---
    dft_n = min(DFT_WINDOW, len(mono))
    mags = compute_dft_magnitudes(mono, dft_n)
    centroid_hz    = compute_spectral_centroid_hz(mags, sample_rate)
    hf_ratio       = compute_hf_energy_ratio(mags, sample_rate)
    transient_str  = compute_transient_strength(mono, sample_rate)

    # --- 6D dimensions ---
    brightness = min(centroid_hz / 8000.0, 1.0)
    warmth     = max(0.0, 1.0 - hf_ratio * 2.0)
    movement   = compute_movement(mono, sample_rate)
    density    = compute_density(mono, sample_rate)
    space      = compute_space(mono, sample_rate)
    aggression = min(1.0, 0.6 * transient_str + 0.4 * hf_ratio)

    return {
        "brightness": round(brightness, 3),
        "warmth":     round(warmth,     3),
        "movement":   round(movement,   3),
        "density":    round(density,    3),
        "space":      round(space,      3),
        "aggression": round(aggression, 3),
    }


# ---------------------------------------------------------------------------
# Directory scan — returns per-file results + summary stats
# ---------------------------------------------------------------------------

def scan_directory(dir_path: str) -> dict:
    """
    Scan all WAV files in a directory (non-recursive) and compute DNA for each.

    Returns:
        {
            "files": {filename: dna_dict, ...},
            "summary": {dim: mean_value, ...}
        }
    """
    wav_files = sorted(
        f for f in os.listdir(dir_path)
        if f.lower().endswith(".wav")
    )

    if not wav_files:
        return {"files": {}, "summary": {}}

    results = {}
    for fname in wav_files:
        full_path = os.path.join(dir_path, fname)
        try:
            results[fname] = compute_dna(full_path)
        except Exception as e:
            results[fname] = {"error": str(e)}

    # Summary stats — mean of each dimension across successful results.
    dimensions = ["brightness", "warmth", "movement", "density", "space", "aggression"]
    valid = [r for r in results.values() if "error" not in r]
    summary = {}
    if valid:
        for dim in dimensions:
            mean_val = sum(r[dim] for r in valid) / len(valid)
            summary[f"mean_{dim}"] = round(mean_val, 3)
        summary["files_analyzed"] = len(valid)
        summary["files_failed"]   = len(results) - len(valid)

    return {"files": results, "summary": summary}


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description=(
            "xpn_auto_dna — compute XOlokun 6D Sonic DNA from WAV audio (stdlib only)\n\n"
            "Single file:  python xpn_auto_dna.py sample.wav\n"
            "Directory:    python xpn_auto_dna.py ./stems/"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "path",
        help="Path to a WAV file or a directory containing WAV files",
    )
    args = parser.parse_args()

    target = args.path

    if os.path.isdir(target):
        output = scan_directory(target)
        print(json.dumps(output, indent=2))
    elif os.path.isfile(target):
        dna = compute_dna(target)
        print(json.dumps(dna, indent=2))
    else:
        print(f"Error: '{target}' is not a file or directory.", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
