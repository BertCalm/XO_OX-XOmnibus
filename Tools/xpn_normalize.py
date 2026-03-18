#!/usr/bin/env python3
"""
xpn_normalize.py — Sample level normalization for XPN/MPC pack production.

Usage:
    python xpn_normalize.py --input ./stems/ --output ./normalized/ --mode peak
    python xpn_normalize.py --input ./stems/ --output ./normalized/ --mode rms --target -18
    python xpn_normalize.py --input ./stems/ --output ./normalized/ --mode auto --recursive
"""

import argparse
import math
import os
import random
import struct
import wave
from pathlib import Path

# ---------------------------------------------------------------------------
# WAV read/write helpers (16-bit and 24-bit, stdlib only)
# ---------------------------------------------------------------------------

def read_wav(path: str):
    """
    Returns (frames_float, sample_rate, n_channels, bit_depth, n_frames).
    frames_float is a flat list of floats in [-1.0, 1.0].
    Supports 16-bit and 24-bit PCM only.
    """
    with wave.open(path, "rb") as wf:
        n_channels = wf.getnchannels()
        sample_rate = wf.getframerate()
        sample_width = wf.getsampwidth()  # bytes per sample
        n_frames = wf.getnframes()
        raw = wf.readframes(n_frames)

    bit_depth = sample_width * 8

    if bit_depth == 16:
        n_samples = len(raw) // 2
        samples_int = list(struct.unpack(f"<{n_samples}h", raw))
        scale = 32768.0
        frames_float = [s / scale for s in samples_int]

    elif bit_depth == 24:
        n_samples = len(raw) // 3
        frames_float = []
        for i in range(n_samples):
            b0, b1, b2 = raw[i*3], raw[i*3+1], raw[i*3+2]
            value = b0 | (b1 << 8) | (b2 << 16)
            if value >= 0x800000:
                value -= 0x1000000
            frames_float.append(value / 8388608.0)

    else:
        raise ValueError(f"Unsupported bit depth: {bit_depth} (only 16 and 24 supported)")

    return frames_float, sample_rate, n_channels, bit_depth, n_frames


def _tpdf_dither():
    """Generate a single TPDF (Triangular Probability Density Function) dither value.

    Two uniform random values are summed to produce a triangular distribution
    centered at zero with range [-1.0, +1.0]. This is the industry-standard
    dither shape for audio quantization.
    """
    return (random.random() - 0.5) + (random.random() - 0.5)


def write_wav(path: str, frames_float: list, sample_rate: int, n_channels: int,
              bit_depth: int, dither: bool = True):
    """
    Write frames_float (flat list of floats in [-1.0, 1.0]) to a WAV file.
    Supports 16-bit and 24-bit PCM.

    When dither=True (default), TPDF dithering is applied before quantization
    to decorrelate quantization noise from the signal.
    """
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)

    if bit_depth == 16:
        scale = 32767.0
        dither_scale = 1.0 / (2 ** (bit_depth - 1)) if dither else 0.0
        clamped = []
        for s in frames_float:
            if dither:
                s = s + _tpdf_dither() * dither_scale
            clamped.append(max(-32768, min(32767, int(round(s * scale)))))
        raw = struct.pack(f"<{len(clamped)}h", *clamped)
        sample_width = 2

    elif bit_depth == 24:
        dither_scale = 1.0 / (2 ** (bit_depth - 1)) if dither else 0.0
        raw_bytes = bytearray()
        for s in frames_float:
            if dither:
                s = s + _tpdf_dither() * dither_scale
            value = int(round(s * 8388607.0))
            value = max(-8388608, min(8388607, value))
            if value < 0:
                value += 0x1000000
            raw_bytes.append(value & 0xFF)
            raw_bytes.append((value >> 8) & 0xFF)
            raw_bytes.append((value >> 16) & 0xFF)
        raw = bytes(raw_bytes)
        sample_width = 3

    else:
        raise ValueError(f"Unsupported bit depth: {bit_depth}")

    with wave.open(path, "wb") as wf:
        wf.setnchannels(n_channels)
        wf.setsampwidth(sample_width)
        wf.setframerate(sample_rate)
        wf.writeframes(raw)


# ---------------------------------------------------------------------------
# Level measurement
# ---------------------------------------------------------------------------

def compute_peak(frames: list) -> float:
    """Return peak absolute sample value (linear, 0.0–1.0)."""
    if not frames:
        return 0.0
    return max(abs(s) for s in frames)


def compute_rms(frames: list) -> float:
    """Return RMS value (linear, 0.0–1.0)."""
    if not frames:
        return 0.0
    mean_sq = sum(s * s for s in frames) / len(frames)
    return math.sqrt(mean_sq)


def linear_to_db(linear: float) -> float:
    if linear <= 0.0:
        return -math.inf
    return 20.0 * math.log10(linear)


def db_to_linear(db: float) -> float:
    return 10.0 ** (db / 20.0)


# ---------------------------------------------------------------------------
# Normalization logic
# ---------------------------------------------------------------------------

MAX_GAIN_DB = 24.0  # guard: skip files needing more than this gain


def compute_gain(frames: list, mode: str, target_db: float) -> tuple:
    """
    Returns (gain_linear, current_peak_db, current_rms_db, reason_skipped_or_None).
    mode: 'peak' or 'rms'
    """
    peak = compute_peak(frames)
    rms = compute_rms(frames)

    peak_db = linear_to_db(peak)
    rms_db = linear_to_db(rms)

    if mode == "peak":
        current_db = peak_db
    else:  # rms
        current_db = rms_db

    if current_db == -math.inf or peak == 0.0:
        return 1.0, peak_db, rms_db, "silent file (skipped)"

    gain_db = target_db - current_db

    if gain_db > MAX_GAIN_DB:
        return 1.0, peak_db, rms_db, f"gain needed ({gain_db:.1f} dB) exceeds +{MAX_GAIN_DB:.0f} dB limit (skipped)"

    return db_to_linear(gain_db), peak_db, rms_db, None


def normalize_file(input_path: str, output_path: str, mode: str, target_db: float) -> dict:
    """
    Normalize a single WAV file.
    Returns a result dict with per-file stats.
    """
    result = {
        "input": input_path,
        "output": output_path,
        "mode": mode,
        "target_db": target_db,
        "input_peak_db": None,
        "input_rms_db": None,
        "gain_db": None,
        "output_peak_db": None,
        "skipped": False,
        "skip_reason": None,
        "error": None,
    }

    try:
        frames, sample_rate, n_channels, bit_depth, n_frames = read_wav(input_path)
    except Exception as e:
        result["error"] = str(e)
        result["skipped"] = True
        result["skip_reason"] = f"read error: {e}"
        return result

    gain_linear, peak_db, rms_db, skip_reason = compute_gain(frames, mode, target_db)
    result["input_peak_db"] = peak_db
    result["input_rms_db"] = rms_db

    if skip_reason:
        result["skipped"] = True
        result["skip_reason"] = skip_reason
        result["gain_db"] = 0.0
        return result

    gain_db = linear_to_db(gain_linear)
    result["gain_db"] = gain_db

    normalized = [s * gain_linear for s in frames]

    # Compute output peak for reporting
    out_peak = compute_peak(normalized)
    result["output_peak_db"] = linear_to_db(out_peak)

    try:
        write_wav(output_path, normalized, sample_rate, n_channels, bit_depth)
    except Exception as e:
        result["error"] = str(e)
        result["skipped"] = True
        result["skip_reason"] = f"write error: {e}"

    return result


# ---------------------------------------------------------------------------
# Auto mode: try to import classifier, fall back to peak
# ---------------------------------------------------------------------------

def resolve_mode_for_file(wav_path: str, base_mode: str) -> tuple:
    """
    Returns (effective_mode, effective_target_db).
    For 'auto' mode, attempts to use xpn_classify_instrument.
    """
    if base_mode != "auto":
        return base_mode, None  # target determined by caller

    try:
        from xpn_classify_instrument import classify_wav  # type: ignore
        instrument_class = classify_wav(wav_path)
        # Drums/percussive → peak norm; pads/melodic → rms norm
        if instrument_class and instrument_class.lower() in ("drum", "drums", "percussive", "kick", "snare", "hat", "perc"):
            return "peak", -3.0
        else:
            return "rms", -18.0
    except ImportError:
        return "peak", -3.0  # classifier not available, fall back to peak


# ---------------------------------------------------------------------------
# Directory scanning
# ---------------------------------------------------------------------------

def collect_wav_files(input_dir: str, recursive: bool) -> list:
    input_path = Path(input_dir)
    if recursive:
        return sorted(input_path.rglob("*.wav")) + sorted(input_path.rglob("*.WAV"))
    else:
        return sorted(input_path.glob("*.wav")) + sorted(input_path.glob("*.WAV"))


def build_output_path(input_file: Path, input_dir: str, output_dir: str) -> str:
    input_root = Path(input_dir).resolve()
    relative = input_file.resolve().relative_to(input_root)
    return str(Path(output_dir) / relative)


# ---------------------------------------------------------------------------
# Summary table printing
# ---------------------------------------------------------------------------

def print_summary(results: list):
    print()
    print(f"{'File':<40} {'Mode':<5} {'In Peak':>8} {'In RMS':>8} {'Gain':>7} {'Out Peak':>9}  Status")
    print("-" * 95)

    n_normalized = 0
    n_skipped = 0

    for r in results:
        filename = os.path.basename(r["input"])
        if len(filename) > 39:
            filename = "..." + filename[-36:]

        mode_str = r["mode"][:5]

        def fmt_db(v):
            if v is None or v == -math.inf:
                return "  -inf"
            return f"{v:+7.1f}"

        in_peak = fmt_db(r.get("input_peak_db"))
        in_rms = fmt_db(r.get("input_rms_db"))
        gain = fmt_db(r.get("gain_db"))
        out_peak = fmt_db(r.get("output_peak_db"))

        if r.get("error"):
            status = f"ERROR: {r['error']}"
            n_skipped += 1
        elif r.get("skipped"):
            status = f"SKIPPED: {r['skip_reason']}"
            n_skipped += 1
        else:
            status = "OK"
            n_normalized += 1

        print(f"{filename:<40} {mode_str:<5} {in_peak} {in_rms} {gain} {out_peak}  {status}")

    print("-" * 95)
    print(f"{n_normalized} file(s) normalized, {n_skipped} skipped.")
    print()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="XPN sample level normalization for MPC pack production."
    )
    parser.add_argument("--input", required=True, help="Input directory containing WAV files.")
    parser.add_argument("--output", required=True, help="Output directory for normalized WAV files.")
    parser.add_argument(
        "--mode",
        choices=["peak", "rms", "auto"],
        default="peak",
        help="Normalization mode: peak (default -3 dBFS), rms (default -18 dBFS), auto (classify per file).",
    )
    parser.add_argument(
        "--target",
        type=float,
        default=None,
        help="Target level in dBFS (negative number). Defaults: peak=-3, rms=-18, auto=per-instrument.",
    )
    parser.add_argument(
        "--recursive",
        action="store_true",
        help="Recurse into subdirectories.",
    )
    args = parser.parse_args()

    # Resolve default targets
    if args.target is not None:
        default_target = args.target
    elif args.mode == "rms":
        default_target = -18.0
    else:
        default_target = -3.0  # peak and auto fallback

    if not os.path.isdir(args.input):
        print(f"ERROR: Input directory not found: {args.input}")
        raise SystemExit(1)

    wav_files = collect_wav_files(args.input, args.recursive)

    if not wav_files:
        print(f"No WAV files found in: {args.input}")
        raise SystemExit(0)

    print(f"Found {len(wav_files)} WAV file(s). Mode: {args.mode}, Target: {default_target:.1f} dBFS")
    print(f"Output: {args.output}")
    print()

    results = []

    for wav_path in wav_files:
        input_str = str(wav_path)
        output_str = build_output_path(wav_path, args.input, args.output)

        # Resolve effective mode/target (handles auto classification)
        effective_mode, auto_target = resolve_mode_for_file(input_str, args.mode)
        effective_target = auto_target if auto_target is not None else default_target

        result = normalize_file(input_str, output_str, effective_mode, effective_target)
        results.append(result)

    print_summary(results)


if __name__ == "__main__":
    main()
