#!/usr/bin/env python3
"""
xpn_qa_checker.py — Perceptual quality checker for rendered WAV files.
Oxport QA pipeline stage.

5 checks per file:
  1. silence          — RMS < -60 dBFS → SILENT
  2. clipping         — >0.1% of samples at digital maximum → CLIPPING
  3. dc_offset        — sample mean > 0.01 → DC_OFFSET
  4. phase_cancel     — stereo: summed RMS < 10% of avg L/R RMS → PHASE_CANCELLED
  5. undynamic        — RMS std-dev across 500ms windows < 0.001 → UNDYNAMIC

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
    "DC_OFFSET":        "Apply a high-pass filter at 5–10 Hz (or DC-blocking filter) "
                        "before export to remove DC bias",
    "PHASE_CANCELLED":  "Check stereo field processing — excessive L/R polarity inversion "
                        "or mid-side routing may be collapsing the mix to silence in mono",
    "UNDYNAMIC":        "Check if this sample has meaningful timbral variation; "
                        "consider replacing with a more expressive render",
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
    checks["dc_offset"]         = _check_dc_offset(samples)
    checks["phase_cancellation"] = _check_phase_cancellation(samples, n_channels)
    checks["undynamic"]         = _check_undynamic(samples, framerate)

    for check_result in checks.values():
        issue = check_result.pop("issue", None)
        if issue:
            issues.append(issue)

    overall = "FAIL" if issues else "PASS"
    remediation = {issue: REMEDIATION[issue] for issue in issues}

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
