#!/usr/bin/env python3
"""
xpn_pack_bundle_sizer.py — Estimate and audit final .xpn bundle size before building.

Usage:
  python xpn_pack_bundle_sizer.py --pack-dir /path/to/pack
  python xpn_pack_bundle_sizer.py --samples-dir /path/to/samples
  python xpn_pack_bundle_sizer.py --pack-dir /path/to/pack --target-mb 200
  python xpn_pack_bundle_sizer.py --pack-dir /path/to/pack --json
"""

import argparse
import json
import struct
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# WAV analysis
# ---------------------------------------------------------------------------

def parse_wav_header(path: Path) -> dict:
    """
    Parse a WAV file header and return metadata.
    Returns dict with keys: bit_depth, sample_rate, channels, num_frames,
    duration_sec, is_pcm, raw_bytes.
    Returns None on parse failure.
    """
    try:
        raw_bytes = path.stat().st_size
        with open(path, "rb") as f:
            riff = f.read(4)
            if riff != b"RIFF":
                return {"raw_bytes": raw_bytes, "is_pcm": False,
                        "bit_depth": None, "sample_rate": None,
                        "channels": None, "num_frames": None, "duration_sec": None}
            f.read(4)  # file size
            wave = f.read(4)
            if wave != b"WAVE":
                return {"raw_bytes": raw_bytes, "is_pcm": False,
                        "bit_depth": None, "sample_rate": None,
                        "channels": None, "num_frames": None, "duration_sec": None}

            # Walk chunks to find fmt
            bit_depth = None
            sample_rate = None
            channels = None
            num_frames = None
            is_pcm = False
            data_bytes = 0

            while True:
                chunk_id = f.read(4)
                if len(chunk_id) < 4:
                    break
                chunk_size_raw = f.read(4)
                if len(chunk_size_raw) < 4:
                    break
                chunk_size = struct.unpack("<I", chunk_size_raw)[0]

                if chunk_id == b"fmt ":
                    fmt_data = f.read(chunk_size)
                    if len(fmt_data) >= 16:
                        audio_format = struct.unpack("<H", fmt_data[0:2])[0]
                        channels = struct.unpack("<H", fmt_data[2:4])[0]
                        sample_rate = struct.unpack("<I", fmt_data[4:8])[0]
                        # byte_rate = struct.unpack("<I", fmt_data[8:12])[0]
                        # block_align = struct.unpack("<H", fmt_data[12:14])[0]
                        bit_depth = struct.unpack("<H", fmt_data[14:16])[0]
                        # audio_format 1 = PCM, 3 = IEEE float, 65534 = extensible
                        is_pcm = audio_format in (1, 3, 65534)
                elif chunk_id == b"data":
                    data_bytes = chunk_size
                    # skip data bytes
                    f.seek(chunk_size, 1)
                else:
                    f.seek(chunk_size, 1)

            if channels and bit_depth and sample_rate and data_bytes:
                bytes_per_frame = channels * (bit_depth // 8)
                if bytes_per_frame > 0:
                    num_frames = data_bytes // bytes_per_frame
                    duration_sec = num_frames / sample_rate if sample_rate else None
                else:
                    num_frames = None
                    duration_sec = None
            else:
                num_frames = None
                duration_sec = None

            return {
                "raw_bytes": raw_bytes,
                "is_pcm": is_pcm,
                "bit_depth": bit_depth,
                "sample_rate": sample_rate,
                "channels": channels,
                "num_frames": num_frames,
                "duration_sec": duration_sec,
            }
    except Exception:
        try:
            raw_bytes = path.stat().st_size
        except Exception:
            raw_bytes = 0
        return {"raw_bytes": raw_bytes, "is_pcm": False,
                "bit_depth": None, "sample_rate": None,
                "channels": None, "num_frames": None, "duration_sec": None}


def compression_ratio(wav_info: dict) -> float:
    """
    Estimate ZIP compression ratio for a WAV file.
    PCM WAV audio compresses modestly in ZIP (~0.85 of original size).
    Non-PCM or unknown: assume already compressed (~0.99).
    """
    if wav_info.get("is_pcm"):
        return 0.85
    return 0.99


# ---------------------------------------------------------------------------
# Scanning
# ---------------------------------------------------------------------------

def scan_wavs(root: Path) -> list:
    """Recursively find all .wav files and parse their headers."""
    results = []
    for p in sorted(root.rglob("*.wav")):
        info = parse_wav_header(p)
        info["path"] = str(p)
        info["name"] = p.name
        results.append(info)
    # also lowercase .WAV
    seen = {r["path"] for r in results}
    for p in sorted(root.rglob("*.WAV")):
        if str(p) not in seen:
            info = parse_wav_header(p)
            info["path"] = str(p)
            info["name"] = p.name
            results.append(info)
    return results


def scan_xpms(root: Path) -> list:
    results = []
    for p in sorted(root.rglob("*.xpm")):
        try:
            size = p.stat().st_size
        except Exception:
            size = 0
        results.append({"path": str(p), "name": p.name, "raw_bytes": size})
    return results


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def fmt_bytes(n: float) -> str:
    if n < 1024:
        return f"{n:.0f} B"
    if n < 1024 ** 2:
        return f"{n / 1024:.1f} KB"
    if n < 1024 ** 3:
        return f"{n / 1024**2:.2f} MB"
    return f"{n / 1024**3:.3f} GB"


def analyze(wav_list: list, xpm_list: list, target_mb) -> dict:
    # WAV totals
    wav_count = len(wav_list)
    wav_total_bytes = sum(w["raw_bytes"] for w in wav_list)
    wav_avg_bytes = wav_total_bytes / wav_count if wav_count else 0

    # Largest files (top 10)
    sorted_wavs = sorted(wav_list, key=lambda w: w["raw_bytes"], reverse=True)
    largest = [{"name": w["name"], "bytes": w["raw_bytes"], "human": fmt_bytes(w["raw_bytes"])}
               for w in sorted_wavs[:10]]

    # Format breakdown
    bit_depth_counts: dict = {}
    sample_rate_counts: dict = {}
    for w in wav_list:
        bd = w.get("bit_depth")
        sr = w.get("sample_rate")
        bd_key = f"{bd}-bit" if bd else "unknown"
        sr_key = f"{sr}Hz" if sr else "unknown"
        bit_depth_counts[bd_key] = bit_depth_counts.get(bd_key, 0) + 1
        sample_rate_counts[sr_key] = sample_rate_counts.get(sr_key, 0) + 1

    # Compression estimate
    estimated_compressed_bytes = sum(
        w["raw_bytes"] * compression_ratio(w) for w in wav_list
    )
    xpm_total_bytes = sum(x["raw_bytes"] for x in xpm_list)
    # XPM are XML text — compress well
    xpm_compressed_bytes = xpm_total_bytes * 0.30

    projected_xpn_bytes = estimated_compressed_bytes + xpm_compressed_bytes
    projected_xpn_mb = projected_xpn_bytes / (1024 ** 2)

    # Warnings
    warnings = []
    if projected_xpn_mb > 1024:
        warnings.append("CRITICAL: Projected .xpn exceeds 1 GB — MPC Live internal storage limit")
    elif projected_xpn_mb > 500:
        warnings.append("WARNING: Projected .xpn exceeds 500 MB — large for MPC Live")

    raw_total_mb = wav_total_bytes / (1024 ** 2)
    if raw_total_mb > 2048:
        warnings.append("WARNING: Raw WAV total exceeds 2 GB — consider splitting into multiple packs")

    # Budget mode
    budget = None
    if target_mb is not None:
        target_bytes = target_mb * (1024 ** 2)
        # How many average-sized WAVs fit in the target compressed budget?
        avg_compressed = wav_avg_bytes * 0.87  # blended ratio
        if avg_compressed > 0:
            budget_count = int(target_bytes / avg_compressed)
        else:
            budget_count = 0
        budget = {
            "target_mb": target_mb,
            "target_bytes": int(target_bytes),
            "avg_wav_bytes": int(wav_avg_bytes),
            "avg_wav_human": fmt_bytes(wav_avg_bytes),
            "estimated_samples_that_fit": budget_count,
            "current_wav_count": wav_count,
            "over_budget": projected_xpn_mb > target_mb,
            "headroom_mb": round(target_mb - projected_xpn_mb, 2),
        }

    return {
        "wav": {
            "count": wav_count,
            "total_bytes": wav_total_bytes,
            "total_human": fmt_bytes(wav_total_bytes),
            "avg_bytes": int(wav_avg_bytes),
            "avg_human": fmt_bytes(wav_avg_bytes),
            "largest_files": largest,
            "bit_depth_breakdown": bit_depth_counts,
            "sample_rate_breakdown": sample_rate_counts,
        },
        "xpm": {
            "count": len(xpm_list),
            "total_bytes": xpm_total_bytes,
            "total_human": fmt_bytes(xpm_total_bytes),
        },
        "projection": {
            "estimated_wav_compressed_bytes": int(estimated_compressed_bytes),
            "estimated_xpm_compressed_bytes": int(xpm_compressed_bytes),
            "projected_xpn_bytes": int(projected_xpn_bytes),
            "projected_xpn_mb": round(projected_xpn_mb, 2),
            "projected_xpn_human": fmt_bytes(projected_xpn_bytes),
        },
        "warnings": warnings,
        "budget": budget,
    }


# ---------------------------------------------------------------------------
# Human-readable report
# ---------------------------------------------------------------------------

def print_report(result: dict) -> None:
    w = result["wav"]
    x = result["xpm"]
    p = result["projection"]

    print("=" * 60)
    print("  XPN PACK BUNDLE SIZER")
    print("=" * 60)

    print(f"\n  WAV FILES")
    print(f"    Count          : {w['count']}")
    print(f"    Total size     : {w['total_human']}")
    print(f"    Average size   : {w['avg_human']}")

    print(f"\n  Bit Depth Breakdown")
    for k, v in sorted(w["bit_depth_breakdown"].items()):
        bar = "#" * min(v, 40)
        print(f"    {k:12s}: {v:5d}  {bar}")

    print(f"\n  Sample Rate Breakdown")
    for k, v in sorted(w["sample_rate_breakdown"].items()):
        bar = "#" * min(v, 40)
        print(f"    {k:12s}: {v:5d}  {bar}")

    print(f"\n  Largest Files (top {len(w['largest_files'])})")
    for lf in w["largest_files"]:
        print(f"    {lf['human']:>10s}  {lf['name']}")

    print(f"\n  XPM FILES")
    print(f"    Count          : {x['count']}")
    print(f"    Total size     : {x['total_human']}")

    print(f"\n  PROJECTED .XPN BUNDLE")
    print(f"    WAV compressed : {fmt_bytes(p['estimated_wav_compressed_bytes'])}")
    print(f"    XPM compressed : {fmt_bytes(p['estimated_xpm_compressed_bytes'])}")
    print(f"    Total .xpn     : {p['projected_xpn_human']}  ({p['projected_xpn_mb']:.2f} MB)")

    budget = result.get("budget")
    if budget:
        print(f"\n  SAMPLE BUDGET  (target: {budget['target_mb']} MB)")
        print(f"    Average WAV    : {budget['avg_wav_human']}")
        print(f"    Samples that fit: {budget['estimated_samples_that_fit']}")
        print(f"    Current count  : {budget['current_wav_count']}")
        if budget["over_budget"]:
            print(f"    STATUS         : OVER BUDGET by {abs(budget['headroom_mb']):.2f} MB")
        else:
            print(f"    STATUS         : Under budget — {budget['headroom_mb']:.2f} MB headroom")

    warnings = result.get("warnings", [])
    if warnings:
        print(f"\n  !! WARNINGS")
        for warn in warnings:
            print(f"    {warn}")
    else:
        print(f"\n  OK: No size warnings.")

    print("=" * 60)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Estimate and audit XPN pack bundle size before building."
    )
    parser.add_argument("--pack-dir", type=str, default=None,
                        help="Root directory of the pack (scans WAV + XPM)")
    parser.add_argument("--samples-dir", type=str, default=None,
                        help="Scan WAV files only (no XPM)")
    parser.add_argument("--target-mb", type=float, default=None,
                        help="Budget target in MB (sample budget mode)")
    parser.add_argument("--json", action="store_true",
                        help="Output machine-readable JSON instead of human report")
    args = parser.parse_args()

    if not args.pack_dir and not args.samples_dir:
        parser.error("Provide --pack-dir or --samples-dir")

    scan_root = Path(args.pack_dir or args.samples_dir)
    if not scan_root.exists():
        print(f"ERROR: Directory not found: {scan_root}", file=sys.stderr)
        sys.exit(1)

    wav_list = scan_wavs(scan_root)
    xpm_list = scan_xpms(scan_root) if args.pack_dir else []

    result = analyze(wav_list, xpm_list, args.target_mb)

    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print_report(result)

    sys.exit(0)


if __name__ == "__main__":
    main()
