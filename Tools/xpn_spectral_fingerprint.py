#!/usr/bin/env python3
"""
xpn_spectral_fingerprint.py — XO_OX Spectral Analysis Tool

Computes a lightweight spectral "fingerprint" (brightness proxy, spectral
centroid, zero-crossing rate, RMS energy, peak frequency) from WAV files.
Used by xpn_cover_art.py to drive sound-derived visual modulation.

Pure stdlib only: wave, struct, math — no numpy/scipy required.

Usage:
    # Fingerprint a single WAV
    python3 xpn_spectral_fingerprint.py sample.wav

    # Fingerprint all WAVs in a directory
    python3 xpn_spectral_fingerprint.py /path/to/wavs/

    # Write fingerprints to JSON for use by cover art generator
    python3 xpn_spectral_fingerprint.py /path/to/wavs/ --output fingerprints.json

Library usage (from xpn_cover_art.py):
    from xpn_spectral_fingerprint import compute_fingerprint, load_fingerprints
    fp = compute_fingerprint("kick.wav")
    # fp = {"brightness": 0.42, "centroid": 2800.0, "zcr": 0.12,
    #        "rms": 0.35, "peak_freq": 65.0, "duration_s": 0.45}
"""

import argparse
import json
import math
import os
import struct
import wave
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# WAV reader
# ---------------------------------------------------------------------------

def _read_wav_mono(path: str) -> tuple[list[float], int]:
    """
    Read a WAV file and return (mono_samples_float, sample_rate).
    Supports 8-bit, 16-bit, 24-bit PCM.
    Raises ValueError on unsupported formats.
    """
    try:
        with wave.open(path, "rb") as wf:
            n_channels = wf.getnchannels()
            sample_rate = wf.getframerate()
            sample_width = wf.getsampwidth()  # bytes per sample per channel
            n_frames = wf.getnframes()
            raw = wf.readframes(n_frames)
    except Exception as e:
        raise ValueError(f"Cannot open WAV {path}: {e}")

    bits = sample_width * 8

    if bits == 8:
        # 8-bit WAV is unsigned
        samples_i = [b - 128 for b in raw]
        max_val = 128.0
    elif bits == 16:
        fmt = f"<{n_frames * n_channels}h"
        samples_i = list(struct.unpack(fmt, raw))
        max_val = 32768.0
    elif bits == 24:
        samples_i = []
        step = 3 * n_channels
        for i in range(n_frames):
            for c in range(n_channels):
                off = i * step + c * 3
                b = raw[off: off + 3]
                val = struct.unpack("<i", b + (b"\xff" if b[2] & 0x80 else b"\x00"))[0]
                samples_i.append(val)
        max_val = 8388608.0
    elif bits == 32:
        fmt = f"<{n_frames * n_channels}i"
        samples_i = list(struct.unpack(fmt, raw))
        max_val = 2147483648.0
    else:
        raise ValueError(f"Unsupported bit depth: {bits}")

    # Normalize to float [-1.0, 1.0]
    float_all = [s / max_val for s in samples_i]

    # Mix down to mono
    if n_channels == 1:
        mono = float_all
    else:
        mono = [
            sum(float_all[i * n_channels + c] for c in range(n_channels)) / n_channels
            for i in range(n_frames)
        ]

    return mono, sample_rate


# ---------------------------------------------------------------------------
# Spectral analysis (DFT-free approximations — stdlib only)
# ---------------------------------------------------------------------------

def _rms(samples: list[float]) -> float:
    """Root mean square energy."""
    if not samples:
        return 0.0
    ss = sum(s * s for s in samples)
    return math.sqrt(ss / len(samples))


def _zero_crossing_rate(samples: list[float]) -> float:
    """Fraction of consecutive sample pairs that cross zero."""
    if len(samples) < 2:
        return 0.0
    crossings = sum(
        1 for i in range(1, len(samples))
        if (samples[i - 1] >= 0) != (samples[i] >= 0)
    )
    return crossings / (len(samples) - 1)


def _spectral_centroid_approx(samples: list[float], sample_rate: int,
                               n_bins: int = 64) -> float:
    """
    Approximate spectral centroid in Hz via a coarse energy-weighted average.

    Divides the signal into n_bins frequency sub-bands by computing the
    average squared amplitude of non-overlapping temporal segments at rates
    mapped to frequency bands.  This is a fast proxy, not a true FFT.
    """
    n = len(samples)
    if n < n_bins * 2:
        return float(sample_rate) / 4.0  # fallback for very short files

    bin_size = n // n_bins
    energies = []
    for b in range(n_bins):
        seg = samples[b * bin_size: (b + 1) * bin_size]
        energies.append(sum(s * s for s in seg) / len(seg))

    total_e = sum(energies)
    if total_e < 1e-12:
        return 0.0

    # Map bin index to frequency (linear approximation)
    freq_per_bin = (sample_rate / 2.0) / n_bins
    centroid = sum(
        (b + 0.5) * freq_per_bin * energies[b]
        for b in range(n_bins)
    ) / total_e
    return centroid


def _peak_frequency_approx(samples: list[float], sample_rate: int,
                            n_bins: int = 64) -> float:
    """Approximate peak frequency via the highest-energy temporal bin."""
    n = len(samples)
    if n < n_bins * 2:
        return 0.0

    bin_size = n // n_bins
    best_e = -1.0
    best_b = 0
    for b in range(n_bins):
        seg = samples[b * bin_size: (b + 1) * bin_size]
        e = sum(s * s for s in seg)
        if e > best_e:
            best_e = e
            best_b = b

    freq_per_bin = (sample_rate / 2.0) / n_bins
    return (best_b + 0.5) * freq_per_bin


def _brightness(centroid: float, sample_rate: int) -> float:
    """
    Normalised brightness [0.0, 1.0].
    Maps spectral centroid to a perceptual brightness scale.
    0.0 = very dark (sub-bass), 1.0 = very bright (air/cymbal).
    """
    nyquist = sample_rate / 2.0
    # Use log scale: 20 Hz → 0.0, nyquist → 1.0
    if centroid <= 20.0:
        return 0.0
    if centroid >= nyquist:
        return 1.0
    return math.log(centroid / 20.0) / math.log(nyquist / 20.0)


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def compute_fingerprint(path: str, max_frames: int = 44100 * 10) -> dict:
    """
    Compute a spectral fingerprint for a WAV file.

    Args:
        path:       Path to the WAV file.
        max_frames: Maximum number of samples to analyse (default 10 s at 44.1 kHz).
                    Longer files are truncated for speed.

    Returns:
        dict with keys:
            brightness  — [0.0, 1.0]  perceptual brightness
            centroid    — Hz          spectral centroid
            zcr         — [0.0, 1.0]  zero-crossing rate
            rms         — [0.0, 1.0]  RMS amplitude
            peak_freq   — Hz          approximate peak frequency
            duration_s  — seconds     actual file duration
    """
    mono, sr = _read_wav_mono(path)
    duration_s = len(mono) / sr

    # Truncate for performance
    analysis = mono[:max_frames]

    centroid = _spectral_centroid_approx(analysis, sr)
    peak_f = _peak_frequency_approx(analysis, sr)
    rms = _rms(analysis)
    zcr = _zero_crossing_rate(analysis)
    bright = _brightness(centroid, sr)

    return {
        "brightness": round(bright, 4),
        "centroid":   round(centroid, 1),
        "zcr":        round(zcr, 4),
        "rms":        round(min(rms, 1.0), 4),
        "peak_freq":  round(peak_f, 1),
        "duration_s": round(duration_s, 3),
    }


def load_fingerprints(directory: str) -> dict:
    """
    Load pre-computed fingerprints from a JSON file, or compute them
    on the fly from all WAV files in a directory.

    If `directory/fingerprints.json` exists, it is loaded directly.
    Otherwise every *.wav/*.WAV file is fingerprinted and the result
    is returned (but NOT written to disk — call save_fingerprints if needed).

    Returns:
        dict mapping filename (basename) → fingerprint dict.
    """
    fp_file = Path(directory) / "fingerprints.json"
    if fp_file.exists():
        try:
            with open(fp_file) as f:
                return json.load(f)
        except Exception:
            pass  # fall through to live computation

    d = Path(directory)
    result = {}
    for wav in sorted(d.glob("*.wav")) + sorted(d.glob("*.WAV")):
        try:
            result[wav.name] = compute_fingerprint(str(wav))
        except (ValueError, OSError):
            pass
    return result


def save_fingerprints(fingerprints: dict, output_path: str) -> None:
    """Write fingerprints dict to a JSON file."""
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(fingerprints, f, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="XPN Spectral Fingerprint — compute perceptual features from WAV files",
    )
    parser.add_argument("path", help="WAV file or directory of WAV files")
    parser.add_argument("--output", "-o", help="Write fingerprints to JSON file")
    parser.add_argument("--json", action="store_true",
                        help="Output results as JSON (stdout)")
    args = parser.parse_args()

    target = Path(args.path)
    wav_files: list[Path] = []

    if target.is_file():
        wav_files = [target]
    elif target.is_dir():
        wav_files = sorted(target.glob("*.wav")) + sorted(target.glob("*.WAV"))
        if not wav_files:
            print(f"No WAV files found in {target}")
            return 1
    else:
        print(f"Path not found: {target}")
        return 1

    results = {}
    for wav in wav_files:
        try:
            fp = compute_fingerprint(str(wav))
            results[wav.name] = fp
            if not args.json and not args.output:
                print(f"{wav.name}:")
                for k, v in fp.items():
                    print(f"  {k:<12s}: {v}")
        except (ValueError, OSError) as e:
            print(f"  [ERROR] {wav.name}: {e}")

    if args.output:
        save_fingerprints(results, args.output)
        print(f"Written: {args.output}  ({len(results)} fingerprints)")

    if args.json:
        print(json.dumps(results, indent=2))

    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())
