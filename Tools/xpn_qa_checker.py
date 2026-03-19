#!/usr/bin/env python3
"""
xpn_qa_checker.py — Perceptual quality checker for rendered WAV files.
Oxport QA pipeline stage.

6 checks per file:
  1. silence          — RMS < -60 dBFS → SILENT
  2. clipping         — >0.1% of samples at digital maximum → CLIPPING
  3. true_peak        — 4x oversampled true peak > -1 dBTP → TRUE_PEAK
  4. dc_offset        — sample mean > 0.01 → DC_OFFSET
  5. phase_cancel     — stereo: summed RMS < 10% of avg L/R RMS → PHASE_CANCELLED
  6. undynamic        — RMS std-dev across 500ms windows < 0.001 → UNDYNAMIC

Usage:
    python xpn_qa_checker.py samples/kick_vel_01.wav
    python xpn_qa_checker.py samples/
    python xpn_qa_checker.py samples/ --json
    python xpn_qa_checker.py samples/ --fail-fast
"""

import argparse
import json
import math
import sys
import wave
from pathlib import Path
from typing import Optional

try:
    import numpy as np
    _NUMPY_AVAILABLE = True
except ImportError:
    _NUMPY_AVAILABLE = False

# ---------------------------------------------------------------------------
# Remediation messages
# ---------------------------------------------------------------------------

REMEDIATION = {
    "SILENT":           "Verify the render pipeline produced audio — check gain staging, "
                        "engine active state, and that the correct preset was rendered",
    "CLIPPING":         "Apply true-peak limiting at -1 dBTP before export",
    "TRUE_PEAK":        "True peak exceeds -1 dBTP — apply a true-peak limiter before export "
                        "to prevent inter-sample clipping on D/A conversion",
    "DC_OFFSET":        "Apply a high-pass filter at 5–10 Hz (or DC-blocking filter) "
                        "before export to remove DC bias",
    "PHASE_CANCELLED":  "Check stereo field processing — excessive L/R polarity inversion "
                        "or mid-side routing may be collapsing the mix to silence in mono",
    "UNDYNAMIC":        "Check if this sample has meaningful timbral variation; "
                        "consider replacing with a more expressive render",
    "ATTACK_PRESENCE":  "First 50ms has no transient (peak < 0.1) — MPC players expect "
                        "impact on note-on. Check render start point or add a transient shaper",
    "LAYER_BALANCE":    "Adjacent velocity layers differ by < 3 dB — velocity response will "
                        "feel flat. Re-render with more dynamic contrast between layers",
    "TONAL_CONSISTENCY":"Spectral centroid shifts > 50% between adjacent velocity layers — "
                        "layers may not sound like the same instrument. Check sample assignment",
}

# ---------------------------------------------------------------------------
# Core WAV reader — stdlib wave + numpy (or pure Python fallback)
# ---------------------------------------------------------------------------

def _read_wav(path: Path) -> tuple[np.ndarray, int, int]:
    """
    Returns (samples_float32, sample_rate, n_channels).
    samples_float32 shape: (n_frames,) for mono, (n_frames, 2) for stereo.
    Raises ValueError on unsupported bit depth.
    """
    with wave.open(str(path), "rb") as wf:
        n_channels = wf.getnchannels()
        sampwidth  = wf.getsampwidth()   # bytes per sample
        framerate  = wf.getframerate()
        n_frames   = wf.getnframes()
        raw        = wf.readframes(n_frames)

    if sampwidth == 2:
        dtype = np.int16
        norm  = 32768.0
    elif sampwidth == 3:
        # 24-bit: unpack 3-byte little-endian words
        raw_array = np.frombuffer(raw, dtype=np.uint8)
        n_samples = n_frames * n_channels
        # Pad to 4 bytes per sample (little-endian → int32)
        padded = np.zeros((n_samples, 4), dtype=np.uint8)
        padded[:, 1] = raw_array[0::3]
        padded[:, 2] = raw_array[1::3]
        padded[:, 3] = raw_array[2::3]
        samples_int = padded.view(np.int32).reshape(-1) >> 8
        dtype = None
        norm  = 8388608.0
        samples_f = samples_int.astype(np.float64) / norm
        if n_channels > 1:
            samples_f = samples_f.reshape(-1, n_channels)
        return samples_f.astype(np.float32), framerate, n_channels
    elif sampwidth == 4:
        dtype = np.int32
        norm  = 2147483648.0
    else:
        raise ValueError(f"Unsupported bit depth: {sampwidth * 8}-bit")

    samples_int = np.frombuffer(raw, dtype=dtype)
    samples_f   = samples_int.astype(np.float64) / norm
    if n_channels > 1:
        samples_f = samples_f.reshape(-1, n_channels)
    return samples_f.astype(np.float32), framerate, n_channels


# ---------------------------------------------------------------------------
# Individual checks
# ---------------------------------------------------------------------------

def _check_silence(samples: "np.ndarray") -> dict:
    flat = samples.flatten()
    rms  = float(np.sqrt(np.mean(flat ** 2)))
    if rms < 1e-10:
        db = -120.0
    else:
        db = 20.0 * math.log10(rms)
    passed = db >= -60.0
    result: dict = {"pass": passed, "rms_db": round(db, 2)}
    if not passed:
        result["issue"] = "SILENT"
    return result


def _check_clipping(samples: "np.ndarray") -> dict:
    flat      = samples.flatten()
    n_total   = len(flat)
    clipped   = int(np.sum(np.abs(flat) >= 0.999))
    pct       = (clipped / n_total * 100.0) if n_total > 0 else 0.0
    passed    = pct <= 0.1
    result: dict = {"pass": passed, "clipped_samples": clipped, "percentage": round(pct, 4)}
    if not passed:
        result["issue"] = "CLIPPING"
    return result


def _check_dc_offset(samples: "np.ndarray") -> dict:
    flat   = samples.flatten()
    mean   = float(np.mean(flat))
    passed = abs(mean) <= 0.01
    result: dict = {"pass": passed, "mean": round(mean, 6)}
    if not passed:
        result["issue"] = "DC_OFFSET"
    return result


def _check_phase_cancellation(samples: "np.ndarray", n_channels: int) -> dict:
    if n_channels < 2:
        return {"pass": True, "skipped": "mono"}
    left  = samples[:, 0].astype(np.float64)
    right = samples[:, 1].astype(np.float64)
    summed     = left + right
    rms_sum    = float(np.sqrt(np.mean(summed ** 2)))
    rms_left   = float(np.sqrt(np.mean(left   ** 2)))
    rms_right  = float(np.sqrt(np.mean(right  ** 2)))
    avg_lr     = (rms_left + rms_right) / 2.0
    if avg_lr < 1e-10:
        # Both channels silent — silence check will flag it
        return {"pass": True, "note": "both channels silent"}
    ratio  = rms_sum / avg_lr
    passed = ratio >= 0.1
    result: dict = {"pass": passed, "sum_to_avg_ratio": round(ratio, 4)}
    if not passed:
        result["issue"] = "PHASE_CANCELLED"
    return result


def _check_undynamic(samples: "np.ndarray", framerate: int) -> dict:
    flat        = samples.flatten() if samples.ndim > 1 else samples
    window_size = int(framerate * 0.5)   # 500ms
    if window_size < 1:
        return {"pass": True, "skipped": "too short"}
    n_windows = len(flat) // window_size
    if n_windows < 2:
        return {"pass": True, "skipped": "too short for windowed analysis"}
    rms_vals = []
    for i in range(n_windows):
        chunk = flat[i * window_size:(i + 1) * window_size]
        rms_vals.append(float(np.sqrt(np.mean(chunk.astype(np.float64) ** 2))))
    rms_std = float(np.std(rms_vals))
    passed  = rms_std >= 0.001
    result: dict = {"pass": passed, "rms_std": round(rms_std, 6), "windows_analyzed": n_windows}
    if not passed:
        result["issue"] = "UNDYNAMIC"
    return result


def _estimate_true_peak(samples: "np.ndarray") -> float:
    """4x oversampled true-peak estimation using linear interpolation.

    Inter-sample peaks can exceed the digital peak by up to ~3 dB.
    This estimates the true peak by interpolating 4 points between each
    pair of adjacent samples and tracking the maximum absolute value.

    Returns the true peak as a linear amplitude value (0.0-1.0+).
    """
    flat = samples.flatten()
    if len(flat) < 2:
        return float(np.max(np.abs(flat))) if len(flat) > 0 else 0.0

    max_tp = float(np.max(np.abs(flat)))

    # 4x oversampled linear interpolation between adjacent samples
    s0 = flat[:-1]
    s1 = flat[1:]
    for k in range(1, 4):
        t = k / 4.0
        interpolated = s0 + t * (s1 - s0)
        candidate = float(np.max(np.abs(interpolated)))
        if candidate > max_tp:
            max_tp = candidate

    return max_tp


# ---------------------------------------------------------------------------
# Musical quality checks
# ---------------------------------------------------------------------------
# These go beyond technical correctness to check musical intent:
# - ATTACK_PRESENCE: MPC players expect impact — flag weak transients
# - LAYER_BALANCE: Adjacent velocity layers should differ meaningfully
# - TONAL_CONSISTENCY: Layers of the same voice should sound like the same instrument

def _check_attack_presence(samples: "np.ndarray", framerate: int) -> dict:
    """
    Verify the first 50ms has a transient (peak > 0.1).
    MPC players expect impact on note-on — a missing attack feels broken.
    """
    attack_frames = int(framerate * 0.050)  # 50ms
    if attack_frames < 1 or len(samples) < attack_frames:
        return {"pass": True, "skipped": "too short"}
    flat = samples[:attack_frames].flatten()
    peak = float(np.max(np.abs(flat)))
    passed = peak > 0.1
    result: dict = {"pass": passed, "attack_peak": round(peak, 4)}
    if not passed:
        result["issue"] = "ATTACK_PRESENCE"
    return result


def _check_layer_balance(layer_samples_list: list) -> dict:
    """
    Check that adjacent velocity layers differ by > 3 dB RMS.
    If layers are too similar, the velocity response feels flat/broken.

    layer_samples_list: list of numpy arrays, one per layer, ordered by velocity.
    Returns pass/fail with per-pair dB differences.
    """
    if len(layer_samples_list) < 2:
        return {"pass": True, "skipped": "single layer"}

    rms_values = []
    for samples in layer_samples_list:
        flat = samples.flatten()
        rms = float(np.sqrt(np.mean(flat.astype(np.float64) ** 2)))
        rms_values.append(rms)

    pair_diffs_db = []
    all_pass = True
    for i in range(len(rms_values) - 1):
        if rms_values[i] < 1e-10 or rms_values[i + 1] < 1e-10:
            pair_diffs_db.append(None)
            continue
        diff_db = abs(20.0 * math.log10(rms_values[i + 1] / rms_values[i]))
        pair_diffs_db.append(round(diff_db, 2))
        if diff_db < 3.0:
            all_pass = False

    result: dict = {"pass": all_pass, "pair_diffs_db": pair_diffs_db}
    if not all_pass:
        result["issue"] = "LAYER_BALANCE"
    return result


def _spectral_centroid(samples: "np.ndarray", framerate: int) -> float:
    """
    Compute spectral centroid in Hz using FFT.
    Returns 0.0 if the signal is silent.
    """
    flat = samples.flatten().astype(np.float64)
    if len(flat) < 256:
        return 0.0
    # Use a window to reduce spectral leakage
    n = len(flat)
    window = np.hanning(n)
    spectrum = np.abs(np.fft.rfft(flat * window))
    freqs = np.fft.rfftfreq(n, d=1.0 / framerate)
    total_energy = float(np.sum(spectrum))
    if total_energy < 1e-10:
        return 0.0
    centroid = float(np.sum(freqs * spectrum) / total_energy)
    return centroid


def _check_tonal_consistency(layer_samples_list: list, framerate: int) -> dict:
    """
    Compare spectral centroid between adjacent velocity layers.
    Flag if centroid shifts by > 50% — suggests layers don't sound like
    the same instrument (mismatched samples, wrong render, etc.).
    """
    if len(layer_samples_list) < 2:
        return {"pass": True, "skipped": "single layer"}

    centroids = [_spectral_centroid(s, framerate) for s in layer_samples_list]

    pair_shifts = []
    all_pass = True
    for i in range(len(centroids) - 1):
        c0, c1 = centroids[i], centroids[i + 1]
        if c0 < 1.0 or c1 < 1.0:
            pair_shifts.append(None)
            continue
        shift_pct = abs(c1 - c0) / min(c0, c1) * 100.0
        pair_shifts.append(round(shift_pct, 1))
        if shift_pct > 50.0:
            all_pass = False

    result: dict = {
        "pass": all_pass,
        "centroids_hz": [round(c, 1) for c in centroids],
        "pair_shift_pct": pair_shifts,
    }
    if not all_pass:
        result["issue"] = "TONAL_CONSISTENCY"
    return result


# True peak threshold: -1 dBTP = 10^(-1/20) = 0.891
_TRUE_PEAK_THRESHOLD = 0.891


def _check_true_peak(samples: "np.ndarray") -> dict:
    """Check if true peak exceeds -1 dBTP (0.891 linear)."""
    tp_linear = _estimate_true_peak(samples)
    if tp_linear > 0.0:
        tp_db = 20.0 * math.log10(tp_linear)
    else:
        tp_db = -120.0
    passed = tp_linear <= _TRUE_PEAK_THRESHOLD
    result: dict = {"pass": passed, "true_peak_linear": round(tp_linear, 6),
                    "true_peak_dbtp": round(tp_db, 2)}
    if not passed:
        result["issue"] = "TRUE_PEAK"
    return result


# ---------------------------------------------------------------------------
# Per-file checker
# ---------------------------------------------------------------------------

def check_file(path: Path) -> dict:
    """Run all 5 QA checks on a single WAV file. Returns structured result dict."""
    result: dict = {"file": path.name}

    if not _NUMPY_AVAILABLE:
        result["error"] = "numpy not available — install with: pip install numpy"
        result["overall"] = "ERROR"
        result["issues"] = []
        result["remediation"] = {}
        return result

    try:
        samples, framerate, n_channels = _read_wav(path)
    except Exception as exc:
        result["error"] = str(exc)
        result["overall"] = "ERROR"
        result["issues"] = []
        result["remediation"] = {}
        return result

    checks: dict = {}
    issues: list[str] = []

    checks["silence"]           = _check_silence(samples)
    checks["clipping"]          = _check_clipping(samples)
    checks["true_peak"]         = _check_true_peak(samples)
    checks["dc_offset"]         = _check_dc_offset(samples)
    checks["phase_cancellation"] = _check_phase_cancellation(samples, n_channels)
    checks["undynamic"]         = _check_undynamic(samples, framerate)
    checks["attack_presence"]   = _check_attack_presence(samples, framerate)

    for check_result in checks.values():
        issue = check_result.pop("issue", None)
        if issue:
            issues.append(issue)

    overall = "FAIL" if issues else "PASS"
    remediation = {issue: REMEDIATION.get(issue, "") for issue in issues}

    result["checks"]      = checks
    result["overall"]     = overall
    result["issues"]      = issues
    result["remediation"] = remediation
    return result


def check_velocity_layers(layer_paths: list[Path]) -> dict:
    """
    Run musical QA checks across a set of velocity layer WAV files.

    layer_paths should be ordered by velocity (v1=softest first, v4=loudest last).

    Checks:
      - LAYER_BALANCE: adjacent layers differ by > 3 dB
      - TONAL_CONSISTENCY: spectral centroid shift < 50% between adjacent layers

    Returns structured result dict with per-check details.
    """
    result: dict = {"files": [p.name for p in layer_paths]}

    if not _NUMPY_AVAILABLE:
        result["error"] = "numpy not available"
        result["overall"] = "ERROR"
        result["issues"] = []
        result["remediation"] = {}
        return result

    layer_samples = []
    framerate = 44100
    for p in layer_paths:
        try:
            samples, fr, _ = _read_wav(p)
            layer_samples.append(samples)
            framerate = fr
        except Exception as exc:
            result["error"] = f"Failed to read {p.name}: {exc}"
            result["overall"] = "ERROR"
            result["issues"] = []
            result["remediation"] = {}
            return result

    checks: dict = {}
    issues: list[str] = []

    checks["layer_balance"]      = _check_layer_balance(layer_samples)
    checks["tonal_consistency"]  = _check_tonal_consistency(layer_samples, framerate)

    for check_result in checks.values():
        issue = check_result.pop("issue", None)
        if issue:
            issues.append(issue)

    overall = "FAIL" if issues else "PASS"
    remediation = {issue: REMEDIATION.get(issue, "") for issue in issues}

    result["checks"]      = checks
    result["overall"]     = overall
    result["issues"]      = issues
    result["remediation"] = remediation
    return result


# ---------------------------------------------------------------------------
# CLI helpers
# ---------------------------------------------------------------------------

def _collect_wavs(target: Path) -> list[Path]:
    if target.is_file():
        if target.suffix.lower() == ".wav":
            return [target]
        print(f"ERROR: {target} is not a .wav file", file=sys.stderr)
        return []
    if target.is_dir():
        wavs = sorted(target.glob("*.wav")) + sorted(target.glob("*.WAV"))
        # Deduplicate (case-insensitive filesystems may return both)
        seen: set[str] = set()
        deduped: list[Path] = []
        for w in wavs:
            key = str(w).lower()
            if key not in seen:
                seen.add(key)
                deduped.append(w)
        return deduped
    print(f"ERROR: {target} does not exist", file=sys.stderr)
    return []


def _print_console_result(result: dict) -> None:
    fname   = result["file"]
    overall = result.get("overall", "ERROR")
    issues  = result.get("issues", [])
    error   = result.get("error")

    if error:
        print(f"[ERROR] {fname} — {error}")
    elif overall == "PASS":
        print(f"[PASS] {fname}")
    else:
        # Build issue summary with details
        detail_parts = []
        checks = result.get("checks", {})
        for issue in issues:
            if issue == "CLIPPING":
                pct = checks.get("clipping", {}).get("percentage", "?")
                detail_parts.append(f"CLIPPING ({pct}%)")
            elif issue == "TRUE_PEAK":
                tp_db = checks.get("true_peak", {}).get("true_peak_dbtp", "?")
                detail_parts.append(f"TRUE_PEAK ({tp_db} dBTP)")
            elif issue == "UNDYNAMIC":
                std = checks.get("undynamic", {}).get("rms_std", "?")
                detail_parts.append(f"UNDYNAMIC (rms_std={std})")
            elif issue == "DC_OFFSET":
                mean = checks.get("dc_offset", {}).get("mean", "?")
                detail_parts.append(f"DC_OFFSET (mean={mean})")
            elif issue == "SILENT":
                db = checks.get("silence", {}).get("rms_db", "?")
                detail_parts.append(f"SILENT ({db} dBFS)")
            else:
                detail_parts.append(issue)
        print(f"[FAIL] {fname} — {', '.join(detail_parts)}")


# ---------------------------------------------------------------------------
# XPM Q-Link validation (format-level, not WAV-level)
# ---------------------------------------------------------------------------

QLINK_MAX_NAME_LEN = 10  # MPC XL OLED display limit

REMEDIATION["QLINK_NAME_TOO_LONG"] = (
    "Q-Link label exceeds 10 characters — will be truncated on MPC XL OLED "
    "display. Shorten the label in the exporter's Q-Link XML generation."
)
REMEDIATION["QLINK_MISSING"] = (
    "No Q-Link assignments found in XPM. MPC Q-Link knobs will be unassigned. "
    "Run through oxport with standardized macro mapping."
)


def check_xpm_qlinks(xpm_path: Path) -> dict:
    """
    Validate Q-Link assignments in an XPM program file.

    Checks:
      - Q-Link names exist and are ≤10 chars (MPC XL OLED limit)
      - At least 1 Q-Link is assigned (missing = user gets blank knobs)

    Returns dict with 'file', 'issues', 'qlinks' list, 'overall'.
    """
    import re

    result: dict = {"file": xpm_path.name, "issues": [], "qlinks": []}

    try:
        content = xpm_path.read_text(encoding="utf-8")
    except Exception as e:
        result["overall"] = "ERROR"
        result["error"] = str(e)
        return result

    # Extract Q-Link names
    names = re.findall(r"<QLink[^>]*>.*?<Name>(.*?)</Name>.*?</QLink>",
                       content, re.DOTALL)

    if not names:
        result["issues"].append("QLINK_MISSING")
    else:
        for name in names:
            result["qlinks"].append(name)
            if len(name) > QLINK_MAX_NAME_LEN:
                result["issues"].append("QLINK_NAME_TOO_LONG")

    result["overall"] = "FAIL" if result["issues"] else "PASS"
    result["remediation"] = {i: REMEDIATION.get(i, "") for i in result["issues"]}
    return result


def check_xpm_directory(xpm_dir: Path) -> list[dict]:
    """Run Q-Link validation on all .xpm files in a directory."""
    results = []
    for xpm in sorted(xpm_dir.rglob("*.xpm")):
        results.append(check_xpm_qlinks(xpm))
    return results


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(
        description="xpn_qa_checker — Perceptual QA for rendered WAV files",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("target",
                        help="WAV file or directory of WAV files to check")
    parser.add_argument("--json", action="store_true",
                        help="Output results as JSON")
    parser.add_argument("--fail-fast", action="store_true",
                        help="Stop on first FAIL result")
    args = parser.parse_args(argv)

    target = Path(args.target)
    wavs   = _collect_wavs(target)

    if not wavs:
        print("No WAV files found.", file=sys.stderr)
        return 1

    results: list[dict] = []
    n_pass = 0
    n_fail = 0
    n_error = 0
    all_issues: list[str] = []   # "issue_code in filename" strings for summary

    for wav_path in wavs:
        result = check_file(wav_path)
        results.append(result)

        overall = result.get("overall", "ERROR")
        if overall == "PASS":
            n_pass += 1
        elif overall == "ERROR":
            n_error += 1
        else:
            n_fail += 1
            for issue in result.get("issues", []):
                all_issues.append(f"{issue} in {wav_path.name}")

        if not args.json:
            _print_console_result(result)

        if args.fail_fast and overall not in ("PASS",):
            if not args.json:
                print(f"\n[FAIL-FAST] Stopped after first failure.")
            break

    if args.json:
        print(json.dumps(results if len(results) > 1 else results[0], indent=2))
    else:
        total  = n_pass + n_fail + n_error
        issues_str = f" | {len(all_issues)} issue{'s' if len(all_issues) != 1 else ''}: " + \
                     "; ".join(all_issues[:5]) + \
                     (" ..." if len(all_issues) > 5 else "") \
                     if all_issues else ""
        error_str  = f" | {n_error} error{'s' if n_error != 1 else ''}" if n_error else ""
        print(f"\nSummary: {n_pass}/{total} passed{error_str}{issues_str}")

    return 0 if (n_fail == 0 and n_error == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
