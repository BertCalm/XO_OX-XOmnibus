"""
xpn_sample_pool_builder.py — XO_OX Sample Pool Builder

Assembles a structured sample pool directory from a flat directory of raw WAV
renders. Classifies each WAV by transient character, frequency centroid, and
duration, then copies files into the organized pool structure expected by
xpn_drum_export.py and xpn_keygroup_export.py.

Output structure:
    {output-dir}/
        drums/
            kick/   — low-frequency transients
            snare/  — mid-frequency transients
            hat/    — high-frequency transients
            perc/   — other percussive/transient files
        melodic/
            bass/   — sustained, low frequency
            mid/    — sustained, mid frequency
            high/   — sustained, high frequency
        atmo/       — long, diffuse atmospheric/pad files

Usage:
    python xpn_sample_pool_builder.py \\
        --source-dir ./raw_renders/ \\
        --output-dir ./sample_pool/ \\
        [--dry-run] \\
        [--min-size-bytes 1000]
"""

import argparse
import math
import os
import shutil
import struct
import sys
import wave
from collections import defaultdict
from pathlib import Path


# ---------------------------------------------------------------------------
# WAV analysis helpers (stdlib only — wave + struct + math)
# ---------------------------------------------------------------------------

def read_wav_pcm(path: str):
    """
    Open a WAV file and return (frames_list, framerate, nchannels, sampwidth).
    frames_list is a flat list of float samples in [-1.0, 1.0] (mono-mixed).
    Returns None on any read error.
    """
    try:
        with wave.open(path, "rb") as wf:
            nchannels = wf.getnchannels()
            sampwidth = wf.getsampwidth()
            framerate = wf.getframerate()
            nframes = wf.getnframes()
            raw = wf.readframes(nframes)
    except Exception:
        return None

    # Decode raw bytes to floats
    if sampwidth == 1:
        fmt = f"{len(raw)}B"
        samples = [(s - 128) / 128.0 for s in struct.unpack(fmt, raw)]
    elif sampwidth == 2:
        fmt = f"{len(raw) // 2}h"
        samples = [s / 32768.0 for s in struct.unpack(fmt, raw)]
    elif sampwidth == 3:
        # 24-bit: unpack manually as 3-byte little-endian signed ints
        samples = []
        for i in range(0, len(raw) - 2, 3):
            val = raw[i] | (raw[i + 1] << 8) | (raw[i + 2] << 16)
            if val >= 0x800000:
                val -= 0x1000000
            samples.append(val / 8388608.0)
    elif sampwidth == 4:
        fmt = f"{len(raw) // 4}i"
        samples = [s / 2147483648.0 for s in struct.unpack(fmt, raw)]
    else:
        return None

    # Mix down to mono
    if nchannels > 1:
        mono = []
        for i in range(0, len(samples) - nchannels + 1, nchannels):
            mono.append(sum(samples[i:i + nchannels]) / nchannels)
        samples = mono

    return samples, framerate, nchannels, sampwidth


def compute_rms(samples):
    if not samples:
        return 0.0
    return math.sqrt(sum(s * s for s in samples) / len(samples))


def is_transient(samples, framerate: int, attack_window_ms: int = 200) -> bool:
    """
    Returns True if the RMS energy in the first `attack_window_ms` milliseconds
    is significantly higher than the RMS of the rest of the file, indicating a
    percussive transient attack.
    """
    attack_frames = int(framerate * attack_window_ms / 1000)
    if len(samples) < attack_frames * 2:
        # Short file — treat the whole thing as attack
        return compute_rms(samples) > 0.01

    attack = samples[:attack_frames]
    body = samples[attack_frames:]
    rms_attack = compute_rms(attack)
    rms_body = compute_rms(body)

    if rms_body < 1e-6:
        return rms_attack > 0.01

    return rms_attack > rms_body * 3.0


def frequency_centroid_hz(samples, framerate: int) -> float:
    """
    Estimate the spectral centroid frequency via a simple DFT over a short
    analysis window (first 2048 samples). Returns frequency in Hz.
    """
    N = min(2048, len(samples))
    if N < 64:
        return 1000.0  # default mid

    window = samples[:N]
    # Apply Hann window to reduce spectral leakage
    windowed = [window[i] * (0.5 - 0.5 * math.cos(2 * math.pi * i / (N - 1)))
                for i in range(N)]

    # DFT — only need the first half (positive frequencies)
    half = N // 2
    magnitudes = []
    for k in range(half):
        re = sum(windowed[n] * math.cos(2 * math.pi * k * n / N) for n in range(N))
        im = sum(windowed[n] * math.sin(2 * math.pi * k * n / N) for n in range(N))
        magnitudes.append(math.sqrt(re * re + im * im))

    total_mag = sum(magnitudes)
    if total_mag < 1e-9:
        return 1000.0

    centroid_bin = sum(k * magnitudes[k] for k in range(half)) / total_mag
    return centroid_bin * framerate / N


def classify_sample(path: str) -> str:
    """
    Classify a WAV file into one of: kick, snare, hat, perc, bass, mid, high, atmo.
    Returns 'unknown' on read failure.
    """
    result = read_wav_pcm(path)
    if result is None:
        return "unknown"

    samples, framerate, _, _ = result
    duration_s = len(samples) / framerate

    transient = is_transient(samples, framerate)
    centroid = frequency_centroid_hz(samples, framerate)
    short = duration_s < 0.5
    long_dur = duration_s > 3.0

    # Frequency band
    if centroid > 2000:
        freq_band = "high"
    elif centroid >= 500:
        freq_band = "mid"
    else:
        freq_band = "low"

    # Classification logic
    if transient:
        if freq_band == "low":
            return "kick"
        elif freq_band == "mid":
            return "snare"
        else:
            return "hat"
    else:
        if long_dur:
            return "atmo"
        elif freq_band == "low":
            return "bass"
        elif freq_band == "mid":
            return "mid"
        else:
            return "high"

    return "perc"  # unreachable but satisfies linter


# ---------------------------------------------------------------------------
# Pool directory layout
# ---------------------------------------------------------------------------

CATEGORY_PATHS = {
    "kick":    "drums/kick",
    "snare":   "drums/snare",
    "hat":     "drums/hat",
    "perc":    "drums/perc",
    "bass":    "melodic/bass",
    "mid":     "melodic/mid",
    "high":    "melodic/high",
    "atmo":    "atmo",
    "unknown": "misc",
}


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def build_pool(source_dir: str, output_dir: str, dry_run: bool, min_size: int):
    source = Path(source_dir)
    output = Path(output_dir)

    if not source.is_dir():
        print(f"ERROR: source directory not found: {source}", file=sys.stderr)
        sys.exit(1)

    wav_files = sorted(source.glob("*.wav")) + sorted(source.glob("*.WAV"))
    wav_files = [f for f in wav_files if f.stat().st_size >= min_size]

    if not wav_files:
        print(f"No WAV files found in {source} (min size: {min_size} bytes).")
        return

    print(f"Found {len(wav_files)} WAV files in {source}")
    if dry_run:
        print("[DRY RUN] No files will be copied.\n")

    # Create output subdirectories
    if not dry_run:
        for rel_path in CATEGORY_PATHS.values():
            (output / rel_path).mkdir(parents=True, exist_ok=True)

    counts: dict[str, int] = defaultdict(int)
    category_counters: dict[str, int] = defaultdict(int)

    for wav in wav_files:
        category = classify_sample(str(wav))
        counts[category] += 1
        category_counters[category] += 1

        index = category_counters[category]
        dest_rel = CATEGORY_PATHS.get(category, "misc")
        dest_name = f"{index:03d}_{category}_{wav.name}"
        dest_path = output / dest_rel / dest_name

        if dry_run:
            print(f"  [{category:7s}] {wav.name}  →  {dest_rel}/{dest_name}")
        else:
            shutil.copy2(str(wav), str(dest_path))
            print(f"  [{category:7s}] {wav.name}  →  {dest_rel}/{dest_name}")

    # Summary
    total = sum(counts.values())
    print(f"\n{'─' * 50}")
    print(f"Classification summary ({total} files):")
    for cat in ["kick", "snare", "hat", "perc", "bass", "mid", "high", "atmo", "unknown"]:
        n = counts.get(cat, 0)
        if n:
            bar = "█" * min(n, 40)
            print(f"  {cat:8s}  {n:4d}  {bar}")
    print(f"{'─' * 50}")

    if not dry_run:
        print(f"\nPool written to: {output.resolve()}")


def main():
    parser = argparse.ArgumentParser(
        description="Build a structured XPN sample pool from a flat WAV directory.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--source-dir", required=True,
                        help="Flat directory containing raw WAV renders.")
    parser.add_argument("--output-dir", required=True,
                        help="Destination pool directory (created if absent).")
    parser.add_argument("--dry-run", action="store_true",
                        help="Print planned copies without writing anything.")
    parser.add_argument("--min-size-bytes", type=int, default=1000,
                        help="Skip WAV files smaller than this (default: 1000).")
    args = parser.parse_args()

    build_pool(
        source_dir=args.source_dir,
        output_dir=args.output_dir,
        dry_run=args.dry_run,
        min_size=args.min_size_bytes,
    )


if __name__ == "__main__":
    main()
