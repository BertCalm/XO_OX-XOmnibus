#!/usr/bin/env python3
"""
xpn_sample_rate_auditor.py — XO_OX XPN Pack Sample Rate Auditor

Scans a directory of WAV files and reports sample rate / bit depth consistency.
XO_OX standard: 44100 Hz, 24-bit, mono or stereo.

Usage:
    python xpn_sample_rate_auditor.py <dir> [--target-rate 44100] [--target-depth 24]
                                             [--output audit.txt] [--json]
"""

import argparse
import json
import struct
import sys
from collections import defaultdict
from pathlib import Path


# ---------------------------------------------------------------------------
# WAV header parsing
# ---------------------------------------------------------------------------

class WavInfo:
    __slots__ = (
        "path", "sample_rate", "bit_depth", "channels",
        "num_samples", "duration_sec", "file_size_bytes",
        "audio_format",  # 1=PCM, 3=IEEE float
    )

    def __init__(self, path):
        self.path = path
        self.sample_rate = None
        self.bit_depth = None
        self.channels = None
        self.num_samples = None
        self.duration_sec = None
        self.file_size_bytes = path.stat().st_size
        self.audio_format = None


def parse_wav(path: Path) -> WavInfo:
    """Parse RIFF/WAV header and return a WavInfo.  Raises on malformed files."""
    info = WavInfo(path)
    with open(path, "rb") as f:
        # RIFF header: ChunkID(4) + ChunkSize(4) + Format(4)
        header = f.read(12)
        if len(header) < 12:
            raise ValueError("File too short for RIFF header")
        chunk_id = header[0:4]
        riff_format = header[8:12]
        if chunk_id != b"RIFF" or riff_format != b"WAVE":
            raise ValueError("Not a RIFF/WAVE file")

        # Walk sub-chunks until we find 'fmt ' and 'data'
        found_fmt = False
        found_data = False
        while True:
            sub_header = f.read(8)
            if len(sub_header) < 8:
                break
            sub_id = sub_header[0:4]
            sub_size = struct.unpack_from("<I", sub_header, 4)[0]

            if sub_id == b"fmt ":
                fmt_data = f.read(sub_size)
                if sub_size < 16:
                    raise ValueError("fmt chunk too small")
                (audio_fmt, channels, sample_rate,
                 byte_rate, block_align, bits_per_sample) = struct.unpack_from("<HHIIHH", fmt_data, 0)
                info.audio_format = audio_fmt
                info.channels = channels
                info.sample_rate = sample_rate
                info.bit_depth = bits_per_sample
                found_fmt = True

            elif sub_id == b"data":
                data_size = sub_size
                found_data = True
                # Seek past data — we don't need to read samples
                f.seek(sub_size, 1)
                if found_fmt and info.bit_depth and info.channels and info.sample_rate:
                    bytes_per_sample = max(1, info.bit_depth // 8)
                    frame_size = bytes_per_sample * info.channels
                    info.num_samples = data_size // frame_size if frame_size else 0
                    info.duration_sec = (
                        info.num_samples / info.sample_rate if info.sample_rate else 0.0
                    )
            else:
                # Skip unknown chunk (pad to even boundary)
                f.seek(sub_size + (sub_size % 2), 1)

            if found_fmt and found_data:
                break

    if not found_fmt:
        raise ValueError("No fmt chunk found")
    return info


# ---------------------------------------------------------------------------
# Issue classification helpers
# ---------------------------------------------------------------------------

UNUSUAL_RATES = {8000, 11025, 16000, 22050, 32000, 96000, 176400, 192000}
NORMAL_RATES  = {44100, 48000, 88200}

def classify_rate(rate: int, target_rate: int) -> str:
    """Return severity string for a sample rate."""
    if rate == target_rate:
        return "OK"
    if rate in UNUSUAL_RATES:
        return "FAIL"
    return "WARN"


def classify_depth(depth: int, target_depth: int, audio_fmt: int) -> str:
    if depth == target_depth and audio_fmt == 1:
        return "OK"
    if depth == 8:
        return "WARN"
    if audio_fmt == 3:          # IEEE float
        return "INFO"
    if depth == 32:
        return "INFO"
    if depth != target_depth:
        return "WARN"
    return "OK"


# ---------------------------------------------------------------------------
# Formatting helpers
# ---------------------------------------------------------------------------

def fmt_duration(seconds: float) -> str:
    if seconds is None:
        return "?"
    m = int(seconds) // 60
    s = seconds - m * 60
    return f"{m}m {s:.1f}s" if m else f"{s:.2f}s"


def fmt_size(bytes_: int) -> str:
    for unit in ("B", "KB", "MB", "GB"):
        if bytes_ < 1024:
            return f"{bytes_:.1f} {unit}"
        bytes_ /= 1024
    return f"{bytes_:.1f} TB"


def fmt_audio_format(code: int) -> str:
    return {1: "PCM", 3: "IEEE float"}.get(code, f"0x{code:04X}")


# ---------------------------------------------------------------------------
# Core audit logic
# ---------------------------------------------------------------------------

def audit_directory(
    root: Path,
    target_rate: int = 44100,
    target_depth: int = 24,
) -> dict:
    """Walk root, parse every .wav, return structured audit result."""
    wav_files = sorted(root.rglob("*.wav")) + sorted(root.rglob("*.WAV"))
    # Deduplicate (rglob may return both on case-insensitive FS)
    seen = set()
    unique_wavs = []
    for p in wav_files:
        key = str(p.resolve())
        if key not in seen:
            seen.add(key)
            unique_wavs.append(p)

    results = []   # list of (WavInfo, issues_list)
    parse_errors = []

    for wav_path in unique_wavs:
        try:
            info = parse_wav(wav_path)
        except Exception as exc:
            parse_errors.append({"path": str(wav_path), "error": str(exc)})
            continue

        issues = []

        # Sample rate check
        rate_sev = classify_rate(info.sample_rate, target_rate)
        if rate_sev != "OK":
            issues.append({
                "severity": rate_sev,
                "field": "sample_rate",
                "detail": f"{info.sample_rate} Hz (target {target_rate} Hz)",
            })

        # Bit depth check
        depth_sev = classify_depth(info.bit_depth, target_depth, info.audio_format)
        if depth_sev != "OK":
            label = fmt_audio_format(info.audio_format)
            issues.append({
                "severity": depth_sev,
                "field": "bit_depth",
                "detail": f"{info.bit_depth}-bit {label} (target {target_depth}-bit PCM)",
            })

        # Long stereo file warning
        if info.channels == 2 and info.duration_sec and info.duration_sec > 10.0:
            issues.append({
                "severity": "WARN",
                "field": "duration",
                "detail": f"Stereo sample {fmt_duration(info.duration_sec)} > 10s (large file)",
            })

        results.append((info, issues))

    # Group by (sample_rate, bit_depth, channels)
    groups = defaultdict(list)
    for info, issues in results:
        key = (info.sample_rate, info.bit_depth, info.channels, info.audio_format)
        groups[key].append((info, issues))

    # Aggregate stats
    total_duration = sum(
        i.duration_sec for i, _ in results if i.duration_sec is not None
    )
    total_size = sum(i.file_size_bytes for i, _ in results)
    needs_resample = [
        i for i, _ in results
        if i.sample_rate != target_rate or i.bit_depth != target_depth or i.audio_format != 1
    ]

    return {
        "root": str(root),
        "target_rate": target_rate,
        "target_depth": target_depth,
        "total_files": len(results),
        "parse_errors": parse_errors,
        "groups": {
            f"{sr}Hz/{bd}bit/{ch}ch/{fmt_audio_format(af)}": [
                {
                    "path": str(i.path.relative_to(root)),
                    "duration_sec": round(i.duration_sec, 4) if i.duration_sec else None,
                    "file_size_bytes": i.file_size_bytes,
                    "issues": iss,
                }
                for i, iss in items
            ]
            for (sr, bd, ch, af), items in sorted(groups.items())
        },
        "total_duration_sec": round(total_duration, 2),
        "total_size_bytes": total_size,
        "needs_resample": [str(i.path.relative_to(root)) for i in needs_resample],
        "all_issues": [
            {
                "path": str(i.path.relative_to(root)),
                "severity": iss["severity"],
                "field": iss["field"],
                "detail": iss["detail"],
            }
            for i, issues in results
            for iss in issues
        ],
    }


# ---------------------------------------------------------------------------
# Human-readable report
# ---------------------------------------------------------------------------

SEV_ORDER = {"FAIL": 0, "WARN": 1, "INFO": 2, "OK": 3}

def render_report(audit: dict) -> str:
    lines = []
    W = 72

    def rule(char="─"):
        lines.append(char * W)

    def header(title):
        rule("═")
        lines.append(f"  {title}")
        rule("═")

    header("XO_OX XPN Sample Rate Auditor")
    lines.append(f"  Directory : {audit['root']}")
    lines.append(f"  Standard  : {audit['target_rate']} Hz / {audit['target_depth']}-bit PCM")
    lines.append(f"  WAV files : {audit['total_files']}")
    lines.append(f"  Total dur : {fmt_duration(audit['total_duration_sec'])}")
    lines.append(f"  Total size: {fmt_size(audit['total_size_bytes'])}")
    lines.append("")

    # ── Distribution by format ──────────────────────────────────────────────
    rule()
    lines.append("  FORMAT DISTRIBUTION")
    rule()
    for fmt_key, items in audit["groups"].items():
        count = len(items)
        ok_marker = "✓" if fmt_key.startswith(f"{audit['target_rate']}Hz/{audit['target_depth']}bit") and "float" not in fmt_key else "!"
        lines.append(f"  [{ok_marker}] {fmt_key:<35}  {count:>4} file(s)")
    lines.append("")

    # ── Issue list ───────────────────────────────────────────────────────────
    all_issues = sorted(audit["all_issues"], key=lambda x: SEV_ORDER.get(x["severity"], 9))
    if all_issues:
        rule()
        lines.append("  ISSUES")
        rule()
        for iss in all_issues:
            sev = iss["severity"]
            lines.append(f"  [{sev:<4}] {iss['path']}")
            lines.append(f"         {iss['field']}: {iss['detail']}")
        lines.append("")
    else:
        lines.append("  No issues found — pack is clean.")
        lines.append("")

    # ── Resample candidates ─────────────────────────────────────────────────
    if audit["needs_resample"]:
        rule()
        lines.append(f"  FILES NEEDING RESAMPLE TO {audit['target_rate']} Hz / {audit['target_depth']}-bit PCM")
        rule()
        for path in audit["needs_resample"]:
            lines.append(f"  • {path}")
        lines.append("")

    # ── Parse errors ────────────────────────────────────────────────────────
    if audit["parse_errors"]:
        rule()
        lines.append("  PARSE ERRORS")
        rule()
        for err in audit["parse_errors"]:
            lines.append(f"  [ERROR] {err['path']}")
            lines.append(f"          {err['error']}")
        lines.append("")

    # ── Summary ─────────────────────────────────────────────────────────────
    rule("═")
    fail_count = sum(1 for i in all_issues if i["severity"] == "FAIL")
    warn_count = sum(1 for i in all_issues if i["severity"] == "WARN")
    info_count = sum(1 for i in all_issues if i["severity"] == "INFO")
    lines.append(
        f"  SUMMARY  FAIL:{fail_count}  WARN:{warn_count}  INFO:{info_count}"
        f"  |  Resample needed: {len(audit['needs_resample'])}/{audit['total_files']}"
    )
    rule("═")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Audit WAV sample rates in an XPN pack directory."
    )
    parser.add_argument("directory", type=Path, help="Root directory to scan")
    parser.add_argument(
        "--target-rate", type=int, default=44100,
        help="Target sample rate in Hz (default: 44100)"
    )
    parser.add_argument(
        "--target-depth", type=int, default=24,
        help="Target bit depth (default: 24)"
    )
    parser.add_argument(
        "--output", type=Path, default=None,
        help="Write text report to this file"
    )
    parser.add_argument(
        "--json", action="store_true",
        help="Output JSON instead of human-readable report"
    )
    args = parser.parse_args()

    root = args.directory.resolve()
    if not root.is_dir():
        print(f"ERROR: '{root}' is not a directory.", file=sys.stderr)
        sys.exit(1)

    audit = audit_directory(root, target_rate=args.target_rate, target_depth=args.target_depth)

    if args.json:
        output_text = json.dumps(audit, indent=2)
    else:
        output_text = render_report(audit)

    print(output_text)

    if args.output:
        args.output.write_text(output_text, encoding="utf-8")
        print(f"\nReport written to: {args.output}", file=sys.stderr)

    # Exit code: 2 = FAIL issues, 1 = WARN only, 0 = clean
    severities = {i["severity"] for i in audit["all_issues"]}
    if "FAIL" in severities or audit["parse_errors"]:
        sys.exit(2)
    elif "WARN" in severities:
        sys.exit(1)
    else:
        sys.exit(0)


if __name__ == "__main__":
    main()
