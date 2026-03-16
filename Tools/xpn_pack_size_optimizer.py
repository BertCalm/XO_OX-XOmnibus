#!/usr/bin/env python3
from __future__ import annotations
"""
XPN Pack Size Optimizer

Analyzes a .xpn pack's sample inventory and reports optimization opportunities
to reduce file size without quality loss.

Usage:
    python xpn_pack_size_optimizer.py <pack.xpn> [--format text|json]
                                       [--threshold-silence 100]
                                       [--threshold-correlation 0.95]
"""

import argparse
import hashlib
import json
import math
import struct
import zipfile
from pathlib import Path


# ---------------------------------------------------------------------------
# WAV parsing helpers
# ---------------------------------------------------------------------------

def _read_u16le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def _read_u32le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def _read_i16le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<h", data, offset)[0]


def parse_wav_header(raw: bytes) -> dict | None:
    """
    Parse a WAV file's fmt and data chunks.

    Returns a dict with keys:
        num_channels, sample_rate, bits_per_sample, num_frames,
        data_offset, data_size, audio_format
    or None on parse failure.
    """
    if len(raw) < 44:
        return None
    if raw[0:4] != b"RIFF" or raw[8:12] != b"WAVE":
        return None

    pos = 12
    fmt = {}
    data_offset = None
    data_size = None

    while pos + 8 <= len(raw):
        chunk_id = raw[pos:pos + 4]
        chunk_size = _read_u32le(raw, pos + 4)
        chunk_data_start = pos + 8

        if chunk_id == b"fmt ":
            if chunk_size < 16:
                return None
            fmt["audio_format"] = _read_u16le(raw, chunk_data_start)
            fmt["num_channels"] = _read_u16le(raw, chunk_data_start + 2)
            fmt["sample_rate"] = _read_u32le(raw, chunk_data_start + 4)
            fmt["bits_per_sample"] = _read_u16le(raw, chunk_data_start + 14)
        elif chunk_id == b"data":
            data_offset = chunk_data_start
            data_size = chunk_size
            break

        pos = chunk_data_start + chunk_size
        if chunk_size % 2 != 0:
            pos += 1  # RIFF padding byte

    if not fmt or data_offset is None or data_size is None:
        return None

    bytes_per_frame = fmt["num_channels"] * (fmt["bits_per_sample"] // 8)
    num_frames = data_size // bytes_per_frame if bytes_per_frame else 0

    return {
        "num_channels": fmt["num_channels"],
        "sample_rate": fmt["sample_rate"],
        "bits_per_sample": fmt["bits_per_sample"],
        "audio_format": fmt["audio_format"],  # 1=PCM, 3=IEEE float
        "num_frames": num_frames,
        "data_offset": data_offset,
        "data_size": data_size,
    }


def read_frames_f32(raw: bytes, header: dict, start_frame: int, count: int) -> list[list[float]]:
    """
    Read `count` frames starting at `start_frame`.
    Returns list of [ch0, ch1, ...] per frame as float in [-1.0, 1.0].
    """
    bps = header["bits_per_sample"]
    nc = header["num_channels"]
    af = header["audio_format"]
    bytes_per_sample = bps // 8
    bytes_per_frame = nc * bytes_per_sample
    offset = header["data_offset"] + start_frame * bytes_per_frame

    frames = []
    for _ in range(count):
        if offset + bytes_per_frame > len(raw):
            break
        frame = []
        for ch in range(nc):
            s_offset = offset + ch * bytes_per_sample
            if af == 3 and bps == 32:
                val = struct.unpack_from("<f", raw, s_offset)[0]
            elif af == 3 and bps == 64:
                val = struct.unpack_from("<d", raw, s_offset)[0]
            elif bps == 16:
                val = _read_i16le(raw, s_offset) / 32768.0
            elif bps == 24:
                b0, b1, b2 = raw[s_offset], raw[s_offset + 1], raw[s_offset + 2]
                raw_int = b0 | (b1 << 8) | (b2 << 16)
                if raw_int & 0x800000:
                    raw_int -= 0x1000000
                val = raw_int / 8388608.0
            elif bps == 32 and af == 1:
                raw_int = _read_u32le(raw, s_offset)
                if raw_int & 0x80000000:
                    raw_int -= 0x100000000
                val = raw_int / 2147483648.0
            else:
                val = 0.0
            frame.append(val)
        frames.append(frame)
        offset += bytes_per_frame

    return frames


# ---------------------------------------------------------------------------
# Analysis functions
# ---------------------------------------------------------------------------

def measure_silence_tail(raw: bytes, header: dict, threshold_ms: float) -> float:
    """
    Returns the length of trailing silence in milliseconds.
    Uses the last 8192 frames, scanning backward.
    """
    sr = header["sample_rate"]
    nf = header["num_frames"]
    if nf == 0 or sr == 0:
        return 0.0

    check_frames = min(nf, 8192)
    start = nf - check_frames
    frames = read_frames_f32(raw, header, start, check_frames)

    silence_frames = 0
    for frame in reversed(frames):
        if max(abs(v) for v in frame) < 0.001:
            silence_frames += 1
        else:
            break

    return (silence_frames / sr) * 1000.0


def measure_channel_correlation(raw: bytes, header: dict) -> float | None:
    """
    Returns Pearson r between L and R channels using first 4096 frames.
    Returns None if mono.
    """
    if header["num_channels"] < 2:
        return None

    nf = header["num_frames"]
    check_frames = min(nf, 4096)
    if check_frames < 2:
        return None

    frames = read_frames_f32(raw, header, 0, check_frames)
    left = [f[0] for f in frames]
    right = [f[1] for f in frames]

    n = len(left)
    mean_l = sum(left) / n
    mean_r = sum(right) / n
    dev_l = [v - mean_l for v in left]
    dev_r = [v - mean_r for v in right]

    numerator = sum(a * b for a, b in zip(dev_l, dev_r))
    denom = math.sqrt(sum(a * a for a in dev_l) * sum(b * b for b in dev_r))
    if denom == 0:
        return 1.0
    return numerator / denom


def sha256_of_data(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


# ---------------------------------------------------------------------------
# Core analysis
# ---------------------------------------------------------------------------

def analyze_pack(
    xpn_path: str,
    threshold_silence_ms: float = 100.0,
    threshold_correlation: float = 0.95,
) -> dict:
    xpn_path = Path(xpn_path)
    if not xpn_path.exists():
        raise FileNotFoundError(f"Pack not found: {xpn_path}")

    results = {
        "pack_name": xpn_path.name,
        "compressed_size": xpn_path.stat().st_size,
        "samples": [],
        "duplicates": [],
        "total_uncompressed": 0,
    }

    sha_map: dict[str, list[str]] = {}  # sha256 -> list of filenames

    with zipfile.ZipFile(xpn_path, "r") as zf:
        wav_entries = [e for e in zf.infolist() if e.filename.lower().endswith(".wav")]

        for entry in wav_entries:
            raw = zf.read(entry.filename)
            file_size = len(raw)
            results["total_uncompressed"] += file_size

            header = parse_wav_header(raw)
            if header is None:
                results["samples"].append({
                    "filename": Path(entry.filename).name,
                    "file_size": file_size,
                    "parse_error": True,
                })
                continue

            sr = header["sample_rate"]
            nc = header["num_channels"]
            bps = header["bits_per_sample"]
            nf = header["num_frames"]
            duration_ms = (nf / sr * 1000.0) if sr else 0.0

            silence_ms = measure_silence_tail(raw, header, threshold_silence_ms)
            correlation = measure_channel_correlation(raw, header)

            # SHA of audio data only (skip header)
            audio_data = raw[header["data_offset"]: header["data_offset"] + header["data_size"]]
            sha = sha256_of_data(audio_data)
            fname = entry.filename
            sha_map.setdefault(sha, []).append(fname)

            # Estimate savings from silence trimming
            silence_bytes_saved = 0
            if silence_ms > threshold_silence_ms:
                bytes_per_frame = nc * (bps // 8)
                silence_frames = int(silence_ms / 1000.0 * sr)
                trim_frames = max(0, silence_frames - int(threshold_silence_ms / 1000.0 * sr))
                silence_bytes_saved = trim_frames * bytes_per_frame

            # 32-bit float → 24-bit savings (25% of data size)
            bit_depth_bytes_saved = 0
            if bps == 32 and header["audio_format"] == 3:
                bit_depth_bytes_saved = header["data_size"] // 4  # 32→24 = 25% reduction

            sample_record = {
                "filename": Path(fname).name,
                "filepath": fname,
                "file_size": file_size,
                "duration_ms": round(duration_ms, 1),
                "sample_rate": sr,
                "bits_per_sample": bps,
                "num_channels": nc,
                "audio_format": "float" if header["audio_format"] == 3 else "pcm",
                "silence_tail_ms": round(silence_ms, 1),
                "silence_bytes_saved": silence_bytes_saved,
                "bit_depth_bytes_saved": bit_depth_bytes_saved,
                "channel_correlation": round(correlation, 4) if correlation is not None else None,
                "sha256": sha,
            }
            results["samples"].append(sample_record)

    # Identify duplicates
    for sha, filenames in sha_map.items():
        if len(filenames) > 1:
            # Bytes wasted = all but one copy
            dup_files = [
                s for s in results["samples"]
                if s.get("sha256") == sha and not s.get("parse_error")
            ]
            if dup_files:
                size_each = dup_files[0]["file_size"]
                results["duplicates"].append({
                    "sha256": sha,
                    "filenames": filenames,
                    "bytes_saved": size_each * (len(filenames) - 1),
                })

    return results


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def _mb(b: int) -> str:
    return f"{b / 1_048_576:.1f} MB"


def _pct(part: int, total: int) -> str:
    if total == 0:
        return "0.0%"
    return f"{part / total * 100:.1f}%"


def format_text(results: dict, threshold_silence_ms: float, threshold_correlation: float) -> str:
    lines = []
    pack_name = results["pack_name"]
    total_unc = results["total_uncompressed"]
    compressed = results["compressed_size"]

    lines.append(f"PACK SIZE ANALYSIS — {pack_name}")
    lines.append("")
    lines.append(f"Total uncompressed: {_mb(total_unc)}   Compressed (.xpn): {_mb(compressed)}")
    lines.append("")

    # Aggregate savings
    samples = [s for s in results["samples"] if not s.get("parse_error")]

    silence_samples = [s for s in samples if s["silence_tail_ms"] > threshold_silence_ms]
    silence_bytes = sum(s["silence_bytes_saved"] for s in silence_samples)

    float32_samples = [s for s in samples if s["bits_per_sample"] == 32 and s["audio_format"] == "float"]
    float32_bytes = sum(s["bit_depth_bytes_saved"] for s in float32_samples)

    dup_bytes = sum(d["bytes_saved"] for d in results["duplicates"])
    dup_pairs = len(results["duplicates"])

    stereo_mono_samples = [
        s for s in samples
        if s["channel_correlation"] is not None and s["channel_correlation"] >= threshold_correlation
    ]
    # Stereo→mono saves ~50% of data size
    stereo_mono_bytes = sum(s["file_size"] // 2 for s in stereo_mono_samples)

    total_savings = silence_bytes + float32_bytes + dup_bytes + stereo_mono_bytes
    optimized_size = max(0, total_unc - total_savings)

    lines.append("OPTIMIZATION OPPORTUNITIES:")
    if silence_samples:
        lines.append(
            f"  Silence trimming ({len(silence_samples)} samples): "
            f"-{_mb(silence_bytes)} ({_pct(silence_bytes, total_unc)})"
        )
    if float32_samples:
        lines.append(
            f"  32-bit→24-bit downgrade ({len(float32_samples)} samples): "
            f"-{_mb(float32_bytes)} ({_pct(float32_bytes, total_unc)})"
        )
    if dup_pairs:
        lines.append(
            f"  Duplicate samples ({dup_pairs} groups): "
            f"-{_mb(dup_bytes)} ({_pct(dup_bytes, total_unc)})"
        )
    if stereo_mono_samples:
        lines.append(
            f"  Near-mono stereo ({len(stereo_mono_samples)} samples, r≥{threshold_correlation}): "
            f"-{_mb(stereo_mono_bytes)} ({_pct(stereo_mono_bytes, total_unc)})"
        )
    if total_savings == 0:
        lines.append("  None found — pack is already well-optimized.")

    lines.append("")
    if total_savings > 0:
        lines.append(
            f"Estimated optimized size: {_mb(optimized_size)} "
            f"(-{_mb(total_savings)}, -{_pct(total_savings, total_unc)})"
        )
        lines.append("")

    # Top 10 by file size
    sorted_samples = sorted(samples, key=lambda s: s["file_size"], reverse=True)
    lines.append("SAMPLES BY SIZE (top 10):")
    header_line = f"  {'Filename':<40} {'Dur':>7}  {'Bit':>5}  {'SR':>6}  {'Size':>8}  Notes"
    lines.append(header_line)
    lines.append("  " + "-" * 80)

    for s in sorted_samples[:10]:
        notes = []
        if s["silence_tail_ms"] > threshold_silence_ms:
            notes.append(f"{s['silence_tail_ms'] / 1000:.1f}s silence tail")
        if s["bits_per_sample"] == 32 and s["audio_format"] == "float":
            notes.append("32-bit float")
        if s.get("channel_correlation") is not None and s["channel_correlation"] >= threshold_correlation:
            notes.append(f"near-mono (r={s['channel_correlation']:.3f})")
        note_str = ", ".join(notes) if notes else ""
        dur_str = f"{s['duration_ms'] / 1000:.2f}s"
        bit_str = f"{s['bits_per_sample']}-bit"
        sr_str = f"{s['sample_rate'] // 1000}kHz"
        size_str = _mb(s["file_size"])
        fname = s["filename"][:38]
        lines.append(f"  {fname:<40} {dur_str:>7}  {bit_str:>5}  {sr_str:>6}  {size_str:>8}  {note_str}")

    # Duplicates detail
    if results["duplicates"]:
        lines.append("")
        lines.append("DUPLICATE SAMPLES:")
        for dup in results["duplicates"]:
            lines.append(f"  SHA {dup['sha256'][:12]}… ({_mb(dup['bytes_saved'])} wasted):")
            for fn in dup["filenames"]:
                lines.append(f"    {fn}")

    # Parse errors
    errors = [s for s in results["samples"] if s.get("parse_error")]
    if errors:
        lines.append("")
        lines.append(f"PARSE ERRORS ({len(errors)} files — skipped from analysis):")
        for e in errors:
            lines.append(f"  {e['filename']}")

    return "\n".join(lines)


def format_json(results: dict) -> str:
    # Remove internal sha256 from top-level samples to keep output clean
    out = dict(results)
    out["samples"] = [
        {k: v for k, v in s.items() if k != "sha256"} for s in results["samples"]
    ]
    return json.dumps(out, indent=2)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Analyze a .xpn pack and suggest size optimizations."
    )
    parser.add_argument("pack", help="Path to the .xpn file")
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    parser.add_argument(
        "--threshold-silence",
        type=float,
        default=100.0,
        metavar="MS",
        help="Trailing silence threshold in ms (default: 100)",
    )
    parser.add_argument(
        "--threshold-correlation",
        type=float,
        default=0.95,
        metavar="R",
        help="Pearson r threshold for near-mono detection (default: 0.95)",
    )
    args = parser.parse_args()

    results = analyze_pack(
        args.pack,
        threshold_silence_ms=args.threshold_silence,
        threshold_correlation=args.threshold_correlation,
    )

    if args.format == "json":
        print(format_json(results))
    else:
        print(format_text(results, args.threshold_silence, args.threshold_correlation))


if __name__ == "__main__":
    main()
