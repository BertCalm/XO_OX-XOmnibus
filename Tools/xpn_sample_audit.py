#!/usr/bin/env python3
"""
xpn_sample_audit.py — Audit WAV samples in a .xpn pack before release.

Usage:
    python xpn_sample_audit.py --xpn pack.xpn [--strict] [--format text|json]

Checks per sample:
    1. Sample rate (flag non-44100 Hz)
    2. Bit depth (error on 8-bit; note 16/24/32-float)
    3. Channels (mono vs stereo — info only)
    4. Duration (flag <50ms or >30s)
    5. DC offset (mean amplitude >0.02 in first 1024 frames)
    6. Clipping (any frame at or above 0 dBFS)
    7. Silence tail (RMS <−60 dBFS in last 512 frames)
"""

import argparse
import json
import math
import os
import struct
import sys
import tempfile
import wave
import zipfile
from pathlib import Path


# ---------------------------------------------------------------------------
# Severity constants
# ---------------------------------------------------------------------------
ERROR = "ERROR"
WARNING = "WARNING"
INFO = "INFO"


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def frames_to_ms(n_frames: int, frame_rate: int) -> float:
    return (n_frames / frame_rate) * 1000.0


def frames_to_seconds(n_frames: int, frame_rate: int) -> float:
    return n_frames / frame_rate


def read_frames_as_float(wav: wave.Wave_read, start_frame: int, count: int) -> list[float]:
    """Read `count` frames starting at `start_frame`, return as float in [-1, 1]."""
    wav.setpos(start_frame)
    raw = wav.readframes(count)
    n_channels = wav.getnchannels()
    sampwidth = wav.getsampwidth()
    n_samples = len(raw) // sampwidth

    samples: list[float] = []

    if sampwidth == 1:
        # 8-bit unsigned PCM: center at 128
        for i in range(n_samples):
            val = raw[i]
            samples.append((val - 128) / 128.0)

    elif sampwidth == 2:
        # 16-bit signed PCM
        count_actual = n_samples
        unpacked = struct.unpack(f"<{count_actual}h", raw[:count_actual * 2])
        samples = [v / 32768.0 for v in unpacked]

    elif sampwidth == 3:
        # 24-bit signed PCM (packed)
        for i in range(n_samples):
            b = raw[i * 3: i * 3 + 3]
            val = struct.unpack("<i", b + (b'\xff' if b[2] & 0x80 else b'\x00'))[0]
            samples.append(val / 8388608.0)

    elif sampwidth == 4:
        # Could be 32-bit int PCM or 32-bit float — detect via RIFF header
        # Python's wave module reports sampwidth=4 for both; we attempt float first
        count_actual = n_samples
        try:
            unpacked_f = struct.unpack(f"<{count_actual}f", raw[:count_actual * 4])
            # If values are all in [-2, 2] treat as float (most float WAVs are [-1,1])
            if all(-2.0 <= v <= 2.0 for v in unpacked_f[:min(8, count_actual)]):
                samples = list(unpacked_f)
            else:
                unpacked_i = struct.unpack(f"<{count_actual}i", raw[:count_actual * 4])
                samples = [v / 2147483648.0 for v in unpacked_i]
        except struct.error:
            samples = []

    return samples


def detect_bit_depth_label(sampwidth: int, wav_path: str) -> tuple[str, bool]:
    """Return (label, is_float32).  Peek at RIFF format chunk to detect IEEE float."""
    is_float = False
    try:
        with open(wav_path, "rb") as f:
            header = f.read(36)
            # audio format at byte 20–21: 1=PCM, 3=IEEE float
            if len(header) >= 22:
                audio_fmt = struct.unpack_from("<H", header, 20)[0]
                if audio_fmt == 3:
                    is_float = True
    except Exception:
        pass

    bits = sampwidth * 8
    if is_float and bits == 32:
        return "32-bit float", True
    return f"{bits}-bit", False


# ---------------------------------------------------------------------------
# Per-sample analysis
# ---------------------------------------------------------------------------

def analyze_wav(wav_path: str) -> dict:
    """
    Returns:
        {
            "name": str,
            "issues": [{"severity": str, "code": str, "message": str}, ...],
            "meta": {frame_rate, sampwidth, n_channels, n_frames, duration_ms, bit_depth_label}
        }
    """
    issues: list[dict] = []
    meta: dict = {}

    try:
        with wave.open(wav_path, "rb") as wav:
            frame_rate = wav.getframerate()
            sampwidth = wav.getsampwidth()
            n_channels = wav.getnchannels()
            n_frames = wav.getnframes()
            duration_ms = frames_to_ms(n_frames, frame_rate)
            duration_s = frames_to_seconds(n_frames, frame_rate)

            bit_depth_label, is_float32 = detect_bit_depth_label(sampwidth, wav_path)

            meta = {
                "frame_rate": frame_rate,
                "sampwidth": sampwidth,
                "bit_depth_label": bit_depth_label,
                "is_float32": is_float32,
                "n_channels": n_channels,
                "n_frames": n_frames,
                "duration_ms": round(duration_ms, 2),
            }

            # --- 1. Sample rate ---
            if frame_rate != 44100:
                issues.append({
                    "severity": WARNING,
                    "code": "SAMPLE_RATE",
                    "message": f"Sample rate is {frame_rate} Hz (expected 44100 Hz)",
                })

            # --- 2. Bit depth ---
            bits = sampwidth * 8
            if bits == 8:
                issues.append({
                    "severity": ERROR,
                    "code": "BIT_DEPTH_8",
                    "message": "8-bit depth is too low for release quality",
                })
            elif bits == 16:
                issues.append({
                    "severity": INFO,
                    "code": "BIT_DEPTH_16",
                    "message": "16-bit depth (acceptable; 24-bit preferred)",
                })
            elif bits == 24:
                pass  # ideal, no issue
            elif is_float32:
                issues.append({
                    "severity": INFO,
                    "code": "BIT_DEPTH_FLOAT32",
                    "message": "32-bit float — confirm DAW/MPC compatibility",
                })
            else:
                issues.append({
                    "severity": INFO,
                    "code": f"BIT_DEPTH_{bits}",
                    "message": f"Unusual bit depth: {bit_depth_label}",
                })

            # --- 3. Channels ---
            if n_channels == 1:
                issues.append({
                    "severity": INFO,
                    "code": "MONO",
                    "message": "Mono sample",
                })
            elif n_channels == 2:
                issues.append({
                    "severity": INFO,
                    "code": "STEREO",
                    "message": "Stereo sample",
                })
            else:
                issues.append({
                    "severity": WARNING,
                    "code": "MULTI_CHANNEL",
                    "message": f"Unexpected channel count: {n_channels}",
                })

            # --- 4. Duration ---
            if duration_ms < 50:
                issues.append({
                    "severity": WARNING,
                    "code": "TOO_SHORT",
                    "message": f"Duration {duration_ms:.1f} ms is suspiciously short (< 50 ms — possible mis-trim)",
                })
            if duration_s > 30:
                issues.append({
                    "severity": WARNING,
                    "code": "TOO_LONG",
                    "message": f"Duration {duration_s:.1f} s exceeds 30 s (may be too long for MPC pad)",
                })

            # --- 5. DC offset (first 1024 frames) ---
            if n_frames > 0:
                check_frames = min(1024, n_frames)
                samples_dc = read_frames_as_float(wav, 0, check_frames)
                if samples_dc:
                    mean_amp = sum(samples_dc) / len(samples_dc)
                    if abs(mean_amp) > 0.02:
                        issues.append({
                            "severity": WARNING,
                            "code": "DC_OFFSET",
                            "message": f"DC offset detected (mean={mean_amp:+.4f} in first {check_frames} frames)",
                        })

            # --- 6. Clipping ---
            if n_frames > 0:
                wav.setpos(0)
                all_raw = wav.readframes(n_frames)
                clipped = False

                if sampwidth == 2:
                    n_samp = len(all_raw) // 2
                    unpacked = struct.unpack(f"<{n_samp}h", all_raw[:n_samp * 2])
                    clipped = any(abs(v) >= 32767 for v in unpacked)
                elif sampwidth == 3:
                    n_samp = len(all_raw) // 3
                    for i in range(n_samp):
                        b = all_raw[i * 3: i * 3 + 3]
                        val = struct.unpack("<i", b + (b'\xff' if b[2] & 0x80 else b'\x00'))[0]
                        if abs(val) >= 8388607:
                            clipped = True
                            break
                elif sampwidth == 4 and is_float32:
                    n_samp = len(all_raw) // 4
                    unpacked_f = struct.unpack(f"<{n_samp}f", all_raw[:n_samp * 4])
                    clipped = any(abs(v) >= 1.0 for v in unpacked_f)
                elif sampwidth == 4:
                    n_samp = len(all_raw) // 4
                    unpacked_i = struct.unpack(f"<{n_samp}i", all_raw[:n_samp * 4])
                    clipped = any(abs(v) >= 2147483647 for v in unpacked_i)
                elif sampwidth == 1:
                    clipped = any(b == 0 or b == 255 for b in all_raw)

                if clipped:
                    issues.append({
                        "severity": ERROR,
                        "code": "CLIPPING",
                        "message": "Clipping detected — sample value at or above 0 dBFS",
                    })

            # --- 7. Silence tail (last 512 frames) ---
            if n_frames >= 512:
                tail_start = n_frames - 512
                tail_samples = read_frames_as_float(wav, tail_start, 512)
                if tail_samples:
                    rms = math.sqrt(sum(v * v for v in tail_samples) / len(tail_samples))
                    if rms > 0:
                        rms_db = 20.0 * math.log10(rms)
                    else:
                        rms_db = -float("inf")
                    if rms_db < -60.0:
                        issues.append({
                            "severity": INFO,
                            "code": "SILENCE_TAIL",
                            "message": f"Last 512 frames RMS = {rms_db:.1f} dBFS — trim may be possible",
                        })

    except Exception as exc:
        issues.append({
            "severity": ERROR,
            "code": "READ_ERROR",
            "message": f"Could not read WAV: {exc}",
        })

    return {
        "name": os.path.basename(wav_path),
        "path": wav_path,
        "issues": issues,
        "meta": meta,
    }


# ---------------------------------------------------------------------------
# XPN extraction
# ---------------------------------------------------------------------------

def extract_wavs_from_xpn(xpn_path: str, dest_dir: str) -> list[str]:
    """Extract all .wav files from the .xpn ZIP, return list of extracted paths."""
    wav_paths: list[str] = []
    with zipfile.ZipFile(xpn_path, "r") as zf:
        dest_root = Path(dest_dir).resolve()
        for entry in zf.namelist():
            if entry.lower().endswith(".wav"):
                target = (dest_root / entry).resolve()
                if not target.is_relative_to(dest_root):
                    raise ValueError(f"ZipSlip blocked: {entry!r}")
                extracted = zf.extract(entry, dest_dir)
                wav_paths.append(extracted)
    return sorted(wav_paths)


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

SEVERITY_ORDER = {ERROR: 0, WARNING: 1, INFO: 2}
SEVERITY_COLOR = {ERROR: "\033[91m", WARNING: "\033[93m", INFO: "\033[96m"}
RESET = "\033[0m"


def _colored(text: str, severity: str) -> str:
    if sys.stdout.isatty():
        return f"{SEVERITY_COLOR.get(severity, '')}{text}{RESET}"
    return text


def print_text_report(results: list[dict], summary: dict) -> None:
    print()
    print("=" * 72)
    print("  XPN SAMPLE AUDIT REPORT")
    print("=" * 72)

    for r in results:
        if not r["issues"]:
            continue
        # Check if any non-INFO issues
        has_notable = any(i["severity"] in (ERROR, WARNING) for i in r["issues"])
        if not has_notable:
            # Only print info-only samples briefly
            info_codes = ", ".join(i["code"] for i in r["issues"])
            print(f"\n  {r['name']}  [{INFO}] {info_codes}")
            continue

        print(f"\n  {r['name']}")
        if r["meta"]:
            m = r["meta"]
            print(f"    {m.get('bit_depth_label','?')} | {m.get('frame_rate','?')} Hz | "
                  f"{m.get('n_channels','?')}ch | {m.get('duration_ms','?')} ms")
        for issue in sorted(r["issues"], key=lambda i: SEVERITY_ORDER[i["severity"]]):
            label = _colored(f"[{issue['severity']:7s}]", issue["severity"])
            print(f"    {label} {issue['code']}: {issue['message']}")

    print()
    print("=" * 72)
    print("  SUMMARY")
    print("=" * 72)
    print(f"  Total samples    : {summary['total_samples']}")
    print(f"  Clean samples    : {summary['clean_samples']}")
    print(f"  Samples w/ errors: {summary['samples_with_errors']}")
    print(f"  Samples w/ warns : {summary['samples_with_warnings']}")
    print()
    for code, count in sorted(summary["issue_counts"].items()):
        print(f"    {code:<25s} {count}")
    print()
    verdict = _colored("PASS", INFO) if summary["pass"] else _colored("FAIL", ERROR)
    print(f"  Overall: {verdict}")
    print("=" * 72)
    print()


def build_summary(results: list[dict]) -> dict:
    total = len(results)
    with_errors = 0
    with_warnings = 0
    clean = 0
    issue_counts: dict[str, int] = {}

    for r in results:
        has_error = any(i["severity"] == ERROR for i in r["issues"])
        has_warning = any(i["severity"] == WARNING for i in r["issues"])
        if has_error:
            with_errors += 1
        elif has_warning:
            with_warnings += 1
        else:
            clean += 1
        for issue in r["issues"]:
            code = issue["code"]
            issue_counts[code] = issue_counts.get(code, 0) + 1

    return {
        "total_samples": total,
        "clean_samples": clean,
        "samples_with_errors": with_errors,
        "samples_with_warnings": with_warnings,
        "issue_counts": issue_counts,
        "pass": with_errors == 0,
    }


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Audit WAV samples inside a .xpn pack for release quality issues."
    )
    parser.add_argument("--xpn", required=True, help="Path to the .xpn pack file")
    parser.add_argument(
        "--strict", action="store_true", help="Exit 1 if any ERROR is found"
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    args = parser.parse_args()

    xpn_path = args.xpn
    if not os.path.isfile(xpn_path):
        print(f"ERROR: File not found: {xpn_path}", file=sys.stderr)
        return 2

    with tempfile.TemporaryDirectory(prefix="xpn_audit_") as tmpdir:
        try:
            wav_paths = extract_wavs_from_xpn(xpn_path, tmpdir)
        except zipfile.BadZipFile as exc:
            print(f"ERROR: Could not open .xpn as ZIP: {exc}", file=sys.stderr)
            return 2

        if not wav_paths:
            print("No .wav files found in pack.", file=sys.stderr)
            return 2

        results = [analyze_wav(p) for p in wav_paths]

    summary = build_summary(results)

    if args.format == "json":
        output = {"results": results, "summary": summary}
        print(json.dumps(output, indent=2))
    else:
        print_text_report(results, summary)

    if args.strict and not summary["pass"]:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
