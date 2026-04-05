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

def _k_weight_coeffs(sample_rate: int) -> tuple:
    """Compute BS.1770-4 K-weighting biquad coefficients for a given sample rate.

    ITU-R BS.1770-4 specifies a two-stage K-weighting filter (fixes GitHub #811):

    Stage 1 — Pre-filter (high shelf, models head acoustics):
      Analog prototype: high-shelf with Vh=1.584893192161 (+4 dB), Vb=sqrt(Vh),
      f0=1681.974450955533 Hz, Q=0.7071067811865476.
      Coefficients via bilinear transform K = tan(pi*f0/sr).

    Stage 2 — RLB weighting (high-pass, models room acoustics):
      Analog prototype: Butterworth high-pass, f0=38.13547087602444 Hz, Q=0.5.
      Numerator coefficients are ALWAYS (1.0, -2.0, 1.0) for a 2nd-order BLT HP;
      only the denominator (a1, a2) is sample-rate dependent.

    At 48kHz the bilinear transform evaluates to the reference table in BS.1770-4
    Annex 1 within < 0.002 dB across the full audio band (well within the spec's
    0.2 LU measurement uncertainty). A 48kHz fast-path uses the exact table values
    to match the standard's reference implementation precisely.

    For all other sample rates the bilinear transform is used, which is correct
    per the specification. Do NOT use the 48kHz table values at other sample rates
    — they will be wrong (this was the original bug: #811).

    Returns: (b0_s1, b1_s1, b2_s1, a1_s1, a2_s1,
               b0_s2, b1_s2, b2_s2, a1_s2, a2_s2)

    Reference: ITU-R BS.1770-4 (2015), Annex 1, §1.1 and §1.2.
    """
    # Fast-path: exact reference coefficients from ITU-R BS.1770-4 Annex 1 Table 1
    # (48kHz is the normative reference sample rate in the standard).
    if sample_rate == 48000:
        return (
            # Stage 1 (pre-filter / high shelf)
             1.53512485958697, -2.69169618940638, 1.19839281085285,
            -1.69065929318241,  0.73248077421585,
            # Stage 2 (RLB / high-pass)
             1.0,              -2.0,               1.0,
            -1.99004745483398,  0.99007225036621,
        )

    sr = float(sample_rate)

    # --- Stage 1: Pre-filter (high shelf) ---
    # Analog prototype parameters from ITU-R BS.1770-4 Annex 1
    Vh = 1.584893192161    # shelf passband gain (+4 dB = 10^(4/20))
    Vb = math.sqrt(Vh)
    f0_s1 = 1681.974450955533   # shelf corner frequency (Hz)
    Q_s1  = 0.7071067811865476  # 1/sqrt(2)
    K_s1  = math.tan(math.pi * f0_s1 / sr)   # bilinear pre-warp
    K2    = K_s1 * K_s1

    # Bilinear-transform high-shelf (standard textbook formula for shelving EQ)
    denom_s1 = 1.0 + K_s1 / Q_s1 + K2
    b0_s1 = (Vh + Vb * K_s1 / Q_s1 + K2) / denom_s1
    b1_s1 = 2.0 * (K2 - Vh)               / denom_s1
    b2_s1 = (Vh - Vb * K_s1 / Q_s1 + K2) / denom_s1
    a1_s1 = 2.0 * (K2 - 1.0)              / denom_s1
    a2_s1 = (1.0 - K_s1 / Q_s1 + K2)     / denom_s1

    # --- Stage 2: RLB weighting (high-pass) ---
    # Analog prototype parameters from ITU-R BS.1770-4 Annex 1
    f0_s2 = 38.13547087602444  # high-pass corner frequency (Hz)
    Q_s2  = 0.5                # BS.1770-4 specifies exactly Q=0.5 for this stage
    K_s2  = math.tan(math.pi * f0_s2 / sr)   # bilinear pre-warp
    K2_s2 = K_s2 * K_s2

    # Bilinear-transform 2nd-order high-pass.
    # Numerator of a BLT 2nd-order HP is ALWAYS (1, -2, 1) × (1/a0) in normalized form,
    # but the ITU convention keeps b=(1,-2,1) unnormalized and divides only the
    # denominator. Since the DF-I sum b0*x - a1*y - a2*y must be consistent,
    # we use the standard normalized form: divide everything by a0.
    # The frequency response is identical in both conventions.
    denom_s2 = 1.0 + K_s2 / Q_s2 + K2_s2
    b0_s2 =  1.0 / denom_s2
    b1_s2 = -2.0 / denom_s2
    b2_s2 =  1.0 / denom_s2
    a1_s2 =  2.0 * (K2_s2 - 1.0)          / denom_s2
    a2_s2 =  (1.0 - K_s2 / Q_s2 + K2_s2) / denom_s2

    return (b0_s1, b1_s1, b2_s1, a1_s1, a2_s1,
            b0_s2, b1_s2, b2_s2, a1_s2, a2_s2)


def _apply_biquad(frames: list, b0: float, b1: float, b2: float,
                  a1: float, a2: float) -> list:
    """Apply a single biquad IIR filter (Direct Form I) to a list of samples."""
    out = [0.0] * len(frames)
    x1 = x2 = y1 = y2 = 0.0
    for i, x0 in enumerate(frames):
        y0 = b0 * x0 + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        out[i] = y0
        x2, x1 = x1, x0
        y2, y1 = y1, y0
    return out


def _k_weight_filter(frames: list, sample_rate: int) -> list:
    """Apply BS.1770-4 K-weighting to audio frames.

    Two cascaded biquad stages (fixes GitHub #811):
      Stage 1 — Pre-filter (high shelf, +4 dB head-acoustics model)
      Stage 2 — RLB weighting filter (Butterworth high-pass, f0≈38 Hz, Q=0.5)

    Coefficients are computed via bilinear transform from the analog prototype
    for the given sample_rate, so they are correct at any sample rate.
    At 48 kHz these match the reference table in ITU-R BS.1770-4 Annex 1.

    Reference: ITU-R BS.1770-4 (2015), Annex 1, §1.1 and §1.2.
    """
    b0_s1, b1_s1, b2_s1, a1_s1, a2_s1, \
    b0_s2, b1_s2, b2_s2, a1_s2, a2_s2 = _k_weight_coeffs(sample_rate)

    # Stage 1: pre-filter (high shelf)
    filtered = _apply_biquad(frames, b0_s1, b1_s1, b2_s1, a1_s1, a2_s1)

    # Stage 2: RLB weighting high-pass
    output = _apply_biquad(filtered, b0_s2, b1_s2, b2_s2, a1_s2, a2_s2)

    return output


def compute_lufs(frames: list, sample_rate: int, n_channels: int) -> float:
    """Compute integrated LUFS per ITU-R BS.1770-4.

    Algorithm (fixes GitHub #812):
      1. K-weight each channel independently (corrected coefficients from #811 fix)
      2. Compute the mean-square power per channel per 400ms block (75% overlap,
         step = 100ms)
      3. For each block, sum the mean-square values across channels (per BS.1770-4
         §2.2 — do NOT average; summing preserves the correct loudness for mono
         content panned to one channel, or unequal L/R levels)
      4. Absolute gate: discard blocks whose summed power is below -70 LUFS
         (i.e. power < 10^((-70+0.691)/10))
      5. Relative gate: compute the mean loudness of absolute-gated blocks, then
         discard blocks more than -10 dB below that mean
      6. Final integrated loudness = mean of double-gated blocks → LUFS

    Block size: 400ms (ITU-R BS.1770-4 §2.2)
    Step size:  100ms (75% overlap, ITU-R BS.1770-4 §2.2)
    Absolute gate: -70 LUFS (ITU-R BS.1770-4 §2.8)
    Relative gate: -10 LU below absolute-gated mean (ITU-R BS.1770-4 §2.8)

    Reference: ITU-R BS.1770-4 (2015), §2.2 and §2.8.
    """
    n_frames = len(frames) // n_channels if n_channels > 1 else len(frames)
    if n_frames == 0:
        return -70.0

    # De-interleave channels and K-weight each independently.
    # BS.1770-4 §2.2: K-weight each channel, then sum mean-square powers.
    channels = []
    if n_channels > 1:
        for c in range(n_channels):
            chan = frames[c::n_channels]
            channels.append(_k_weight_filter(chan, sample_rate))
    else:
        channels.append(_k_weight_filter(frames, sample_rate))

    # Block parameters: 400ms window, 100ms step (75% overlap)
    window_samples = int(round(sample_rate * 0.400))
    step_samples   = int(round(sample_rate * 0.100))

    if window_samples < 1:
        return -70.0

    # Determine number of complete blocks
    ch_len = len(channels[0])
    if ch_len < window_samples:
        # Signal shorter than one block — measure as a single block
        total_ms = sum(
            sum(s * s for s in ch) / max(len(ch), 1)
            for ch in channels
        )
        if total_ms < 1e-20:
            return -70.0
        return -0.691 + 10.0 * math.log10(total_ms)

    # Compute summed mean-square power per block across all channels
    # BS.1770-4 §2.2: z_l = (1/T) * sum(x_l^2); summed_power = sum_l(z_l)
    window_powers = []
    start = 0
    while start + window_samples <= ch_len:
        block_power = 0.0
        for ch in channels:
            chunk = ch[start:start + window_samples]
            block_power += sum(s * s for s in chunk) / window_samples
        window_powers.append(block_power)
        start += step_samples

    if not window_powers:
        return -70.0

    # Absolute gate: -70 LUFS (BS.1770-4 §2.8)
    # Threshold in mean-square power: 10^((-70 + 0.691) / 10)
    abs_gate_power = 10.0 ** ((-70.0 + 0.691) / 10.0)
    gated_abs = [p for p in window_powers if p > abs_gate_power]

    if not gated_abs:
        return -70.0

    # Mean loudness of absolute-gated blocks (BS.1770-4 §2.8)
    mean_abs_power = sum(gated_abs) / len(gated_abs)
    abs_gated_lufs = -0.691 + 10.0 * math.log10(mean_abs_power) if mean_abs_power > 0 else -70.0

    # Relative gate: -10 LU below absolute-gated mean (BS.1770-4 §2.8)
    rel_gate_lufs  = abs_gated_lufs - 10.0
    rel_gate_power = 10.0 ** ((rel_gate_lufs + 0.691) / 10.0)
    gated_rel = [p for p in gated_abs if p > rel_gate_power]

    if not gated_rel:
        return -70.0

    # Final integrated loudness = mean of double-gated blocks
    mean_rel_power = sum(gated_rel) / len(gated_rel)
    if mean_rel_power <= 0:
        return -70.0

    integrated_lufs = -0.691 + 10.0 * math.log10(mean_rel_power)
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
