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
# LUFS normalization (simplified ITU-R BS.1770 approximation)
# ---------------------------------------------------------------------------

def _k_weight_filter(frames: list, sample_rate: int) -> list:
    """Apply simplified K-weighting to audio frames.

    K-weighting consists of two stages:
      1. High-shelf boost of +4 dB at 1500 Hz (models head acoustics)
      2. High-pass filter at 38 Hz (removes sub-bass energy)

    This is a simplified first-order approximation suitable for
    integrated loudness estimation.
    """
    # Stage 1: High-shelf +4 dB at 1500 Hz (first-order shelving filter)
    # Gain = 10^(4/20) ~ 1.585
    fc_shelf = 1500.0
    gain_linear = 1.5849  # +4 dB
    w0 = 2.0 * math.pi * fc_shelf / sample_rate
    cos_w0 = math.cos(w0)
    alpha = math.sin(w0) / 2.0 * 0.7071  # Q = 0.7071 (Butterworth)

    A = math.sqrt(gain_linear)
    # Shelf coefficients (peaking approximation for simplicity)
    b0 = 1.0 + alpha * A
    b1 = -2.0 * cos_w0
    b2 = 1.0 - alpha * A
    a0 = 1.0 + alpha / A
    a1 = -2.0 * cos_w0
    a2 = 1.0 - alpha / A

    # Normalize
    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0

    # Apply biquad filter (stage 1)
    filtered = [0.0] * len(frames)
    x1 = x2 = y1 = y2 = 0.0
    for i, x0 in enumerate(frames):
        y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        filtered[i] = y0
        x2, x1 = x1, x0
        y2, y1 = y1, y0

    # Stage 2: High-pass at 38 Hz (first-order)
    fc_hp = 38.0
    rc = 1.0 / (2.0 * math.pi * fc_hp)
    dt = 1.0 / sample_rate
    hp_alpha = rc / (rc + dt)

    output = [0.0] * len(filtered)
    prev_x = 0.0
    prev_y = 0.0
    for i, x0 in enumerate(filtered):
        y0 = hp_alpha * (prev_y + x0 - prev_x)
        output[i] = y0
        prev_x = x0
        prev_y = y0

    return output


def compute_lufs(frames: list, sample_rate: int, n_channels: int) -> float:
    """Compute integrated LUFS using simplified BS.1770 algorithm.

    Steps:
      1. K-weight the signal
      2. Compute mean-square power in 400ms overlapping windows
      3. Absolute gate at -70 LUFS
      4. Relative gate at -10 LUFS below ungated mean
      5. Return gated integrated loudness in LUFS
    """
    # Mix to mono if stereo (simplified — full spec sums per-channel power)
    if n_channels > 1:
        mono = []
        for i in range(0, len(frames) - n_channels + 1, n_channels):
            mono.append(sum(frames[i:i + n_channels]) / n_channels)
    else:
        mono = list(frames)

    if not mono:
        return -70.0

    # K-weight
    weighted = _k_weight_filter(mono, sample_rate)

    # 400ms window, 75% overlap (step = 100ms)
    window_samples = int(sample_rate * 0.4)
    step_samples = int(sample_rate * 0.1)

    if window_samples < 1 or len(weighted) < window_samples:
        # Too short for windowed analysis — compute single-window
        ms = sum(s * s for s in weighted) / len(weighted)
        if ms < 1e-20:
            return -70.0
        return -0.691 + 10.0 * math.log10(ms)

    # Compute mean-square per window
    window_powers = []
    for start in range(0, len(weighted) - window_samples + 1, step_samples):
        chunk = weighted[start:start + window_samples]
        ms = sum(s * s for s in chunk) / window_samples
        window_powers.append(ms)

    if not window_powers:
        return -70.0

    # Absolute gate: -70 LUFS
    abs_gate_power = 10.0 ** ((-70.0 + 0.691) / 10.0)
    gated_abs = [p for p in window_powers if p > abs_gate_power]

    if not gated_abs:
        return -70.0

    # Ungated mean power
    mean_abs = sum(gated_abs) / len(gated_abs)
    ungated_lufs = -0.691 + 10.0 * math.log10(mean_abs) if mean_abs > 0 else -70.0

    # Relative gate: -10 LUFS below ungated mean
    rel_gate_lufs = ungated_lufs - 10.0
    rel_gate_power = 10.0 ** ((rel_gate_lufs + 0.691) / 10.0)
    gated_rel = [p for p in gated_abs if p > rel_gate_power]

    if not gated_rel:
        return -70.0

    mean_rel = sum(gated_rel) / len(gated_rel)
    if mean_rel <= 0:
        return -70.0

    integrated_lufs = -0.691 + 10.0 * math.log10(mean_rel)
    return integrated_lufs


def normalize_file_lufs(input_path: str, output_path: str,
                        target_lufs: float = -14.0) -> dict:
    """Normalize a single WAV file to a target LUFS level.

    Returns a result dict with per-file stats.
    """
    result = {
        "input": input_path,
        "output": output_path,
        "mode": "lufs",
        "target_db": target_lufs,
        "input_peak_db": None,
        "input_rms_db": None,
        "input_lufs": None,
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

    peak = compute_peak(frames)
    rms = compute_rms(frames)
    result["input_peak_db"] = linear_to_db(peak)
    result["input_rms_db"] = linear_to_db(rms)

    if peak == 0.0:
        result["skipped"] = True
        result["skip_reason"] = "silent file (skipped)"
        result["gain_db"] = 0.0
        return result

    current_lufs = compute_lufs(frames, sample_rate, n_channels)
    result["input_lufs"] = round(current_lufs, 2)

    if current_lufs <= -70.0:
        result["skipped"] = True
        result["skip_reason"] = "below noise floor (skipped)"
        result["gain_db"] = 0.0
        return result

    gain_db = target_lufs - current_lufs
    if gain_db > MAX_GAIN_DB:
        result["skipped"] = True
        result["skip_reason"] = (f"gain needed ({gain_db:.1f} dB) exceeds "
                                 f"+{MAX_GAIN_DB:.0f} dB limit (skipped)")
        result["gain_db"] = 0.0
        return result

    gain_linear = db_to_linear(gain_db)
    result["gain_db"] = round(gain_db, 2)

    normalized = [s * gain_linear for s in frames]
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
        choices=["peak", "rms", "lufs", "auto"],
        default="peak",
        help="Normalization mode: peak (default -3 dBFS), rms (default -18 dBFS), "
             "lufs (default -14 LUFS integrated), auto (classify per file).",
    )
    parser.add_argument(
        "--target",
        type=float,
        default=None,
        help="Target level in dBFS/LUFS (negative number). "
             "Defaults: peak=-3, rms=-18, lufs=-14, auto=per-instrument.",
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
    elif args.mode == "lufs":
        default_target = -14.0
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

        if args.mode == "lufs":
            result = normalize_file_lufs(input_str, output_str, target_lufs=default_target)
        else:
            # Resolve effective mode/target (handles auto classification)
            effective_mode, auto_target = resolve_mode_for_file(input_str, args.mode)
            effective_target = auto_target if auto_target is not None else default_target
            result = normalize_file(input_str, output_str, effective_mode, effective_target)
        results.append(result)

    print_summary(results)


if __name__ == "__main__":
    main()
