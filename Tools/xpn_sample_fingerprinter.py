#!/usr/bin/env python3
"""
xpn_sample_fingerprinter.py — WAV sample deduplication and provenance tracking
XO_OX XOlokun Tools Suite

Generates multi-tier fingerprints for WAV files:
  Tier 1: Exact hash (MD5 of raw audio data bytes)
  Tier 2: Format fingerprint (sample_rate, bit_depth, channels, duration_ms)
  Tier 3: Content fingerprint (RMS over 8 equal time segments)

Usage:
  python xpn_sample_fingerprinter.py <dir> [--compare <dir2>] [--rebuild] [--output report.txt]
"""

import argparse
import hashlib
import json
import math
import struct
from pathlib import Path


DB_FILENAME = "sample_fingerprints.json"
CONTENT_SEGMENTS = 8
NEAR_DUPLICATE_THRESHOLD = 0.08   # max mean RMS delta for near-duplicate
SUSPICIOUS_THRESHOLD = 0.05       # tighter threshold for cross-format suspicious matches


# ---------------------------------------------------------------------------
# WAV parsing (stdlib only — struct-parsed RIFF, no wave module)
# ---------------------------------------------------------------------------

def _read_riff_chunks(data: bytes) -> dict:
    """Parse RIFF/WAVE file, return dict mapping chunk_id (bytes) -> chunk_data (bytes)."""
    if len(data) < 12:
        raise ValueError("File too small to be a valid WAV")
    riff_id = data[0:4]
    if riff_id not in (b"RIFF", b"RF64"):
        raise ValueError(f"Not a RIFF file (got {riff_id!r})")
    wave_id = data[8:12]
    if wave_id != b"WAVE":
        raise ValueError(f"Not a WAVE file (got {wave_id!r})")

    chunks: dict = {}
    pos = 12
    while pos + 8 <= len(data):
        chunk_id = data[pos:pos + 4]
        chunk_size = struct.unpack_from("<I", data, pos + 4)[0]
        chunk_data = data[pos + 8: pos + 8 + chunk_size]
        # First occurrence wins (handles duplicate data chunks gracefully)
        if chunk_id not in chunks:
            chunks[chunk_id] = chunk_data
        pos += 8 + chunk_size + (chunk_size & 1)  # word-align
    return chunks


def _parse_fmt(fmt_data: bytes) -> dict:
    """Parse fmt chunk bytes, return audio format metadata dict."""
    if len(fmt_data) < 16:
        raise ValueError("fmt chunk too small")
    audio_format, channels, sample_rate, byte_rate, block_align, bit_depth = \
        struct.unpack_from("<HHIIHH", fmt_data, 0)
    return {
        "audio_format": audio_format,   # 1=PCM, 3=IEEE float, 65534=extensible
        "channels": channels,
        "sample_rate": sample_rate,
        "bit_depth": bit_depth,
        "block_align": block_align,
    }


def _decode_sample(raw: bytes, bit_depth: int, audio_format: int) -> float:
    """Decode a single audio sample bytes to float in range [-1.0, 1.0]."""
    if audio_format == 3 and bit_depth == 32:
        return struct.unpack_from("<f", raw)[0]
    if bit_depth == 8:
        return (struct.unpack_from("<B", raw)[0] - 128) / 128.0
    if bit_depth == 16:
        return struct.unpack_from("<h", raw)[0] / 32768.0
    if bit_depth == 24:
        b0, b1, b2 = raw[0], raw[1], raw[2]
        signed = b2 << 16 | b1 << 8 | b0
        if signed & 0x800000:
            signed -= 0x1000000
        return signed / 8388608.0
    if bit_depth == 32:
        return struct.unpack_from("<i", raw)[0] / 2147483648.0
    return 0.0


def _compute_rms_segments(pcm_bytes: bytes, fmt: dict, num_segments: int) -> list:
    """
    Compute RMS amplitude for num_segments equal time windows across pcm_bytes.
    Returns list of floats (RMS, roughly 0.0–1.0).
    """
    channels = fmt["channels"]
    bit_depth = fmt["bit_depth"]
    audio_format = fmt["audio_format"]
    block_align = fmt["block_align"]
    bytes_per_sample = max(1, bit_depth // 8)

    if block_align == 0 or len(pcm_bytes) == 0:
        return [0.0] * num_segments

    total_frames = len(pcm_bytes) // block_align
    if total_frames == 0:
        return [0.0] * num_segments

    segment_frames = max(1, total_frames // num_segments)
    segments = []

    for seg in range(num_segments):
        start_frame = seg * segment_frames
        end_frame = min(total_frames, start_frame + segment_frames)
        if start_frame >= total_frames:
            segments.append(0.0)
            continue

        sum_sq = 0.0
        count = 0
        for frame in range(start_frame, end_frame):
            frame_offset = frame * block_align
            for ch in range(channels):
                sample_offset = frame_offset + ch * bytes_per_sample
                if sample_offset + bytes_per_sample > len(pcm_bytes):
                    break
                raw = pcm_bytes[sample_offset: sample_offset + bytes_per_sample]
                val = _decode_sample(raw, bit_depth, audio_format)
                sum_sq += val * val
                count += 1

        rms = math.sqrt(sum_sq / count) if count > 0 else 0.0
        segments.append(round(rms, 6))

    return segments


# ---------------------------------------------------------------------------
# Main fingerprint computation
# ---------------------------------------------------------------------------

def fingerprint_wav(path: Path) -> dict:
    """
    Compute all three fingerprint tiers for a WAV file.

    Returns dict:
      path         - relative or absolute string path (caller sets final value)
      exact_hash   - MD5 hex digest of raw audio data chunk bytes
      format_fp    - [sample_rate, bit_depth, channels, duration_ms_rounded_10ms]
      content_fp   - list of CONTENT_SEGMENTS RMS floats
      meta         - human-readable format metadata + file_size
    """
    data = path.read_bytes()
    chunks = _read_riff_chunks(data)

    if b"fmt " not in chunks:
        raise ValueError("Missing fmt chunk")
    if b"data" not in chunks:
        raise ValueError("Missing data chunk")

    fmt = _parse_fmt(chunks[b"fmt "])
    pcm_bytes = chunks[b"data"]

    # Tier 1 — exact duplicate detection
    exact_hash = hashlib.md5(pcm_bytes).hexdigest()

    # Tier 2 — same recording at different encodings
    block_align = fmt["block_align"]
    total_frames = len(pcm_bytes) // block_align if block_align else 0
    duration_raw_ms = (total_frames / fmt["sample_rate"] * 1000) if fmt["sample_rate"] > 0 else 0
    duration_ms = int(round(duration_raw_ms / 10) * 10)  # rounded to nearest 10 ms
    format_fp = [fmt["sample_rate"], fmt["bit_depth"], fmt["channels"], duration_ms]

    # Tier 3 — audio content similarity
    content_fp = _compute_rms_segments(pcm_bytes, fmt, CONTENT_SEGMENTS)

    return {
        "path": str(path),
        "exact_hash": exact_hash,
        "format_fp": format_fp,
        "content_fp": content_fp,
        "meta": {
            "sample_rate": fmt["sample_rate"],
            "bit_depth": fmt["bit_depth"],
            "channels": fmt["channels"],
            "duration_ms": duration_ms,
            "file_size": path.stat().st_size,
        },
    }


# ---------------------------------------------------------------------------
# Database build / load
# ---------------------------------------------------------------------------

def build_database(directory: Path, rebuild: bool = False) -> dict:
    """
    Scan directory recursively for WAV files, build/update fingerprint DB.
    Returns DB dict keyed by relative path string.
    """
    db_path = directory / DB_FILENAME
    db: dict = {}

    if db_path.exists() and not rebuild:
        try:
            db = json.loads(db_path.read_text(encoding="utf-8"))
            print(f"  Loaded existing DB: {len(db)} entries from {db_path.name}")
        except (json.JSONDecodeError, OSError):
            db = {}

    # Collect unique WAVs (case-insensitive extension)
    seen: set = set()
    unique_wavs = []
    for wav in sorted(directory.rglob("*")):
        if wav.suffix.lower() == ".wav" and str(wav) not in seen:
            seen.add(str(wav))
            unique_wavs.append(wav)

    new_count = error_count = 0
    for wav in unique_wavs:
        key = str(wav.relative_to(directory))
        if key in db and not rebuild:
            continue
        try:
            fp = fingerprint_wav(wav)
            fp["path"] = key  # store relative path
            db[key] = fp
            new_count += 1
        except Exception as exc:
            print(f"  WARN: Could not fingerprint {wav.name}: {exc}")
            error_count += 1

    db_path.write_text(json.dumps(db, indent=2), encoding="utf-8")
    print(f"  Scanned {len(unique_wavs)} WAVs — {new_count} new, {error_count} errors")
    print(f"  DB written: {db_path}")
    return db


# ---------------------------------------------------------------------------
# Comparison / duplicate detection
# ---------------------------------------------------------------------------

def _quality_score(meta: dict) -> int:
    """Higher = better quality encoding."""
    return meta["sample_rate"] * meta["bit_depth"]


def _content_distance(fp_a: list, fp_b: list) -> float:
    """Mean absolute difference between two RMS segment vectors."""
    n = min(len(fp_a), len(fp_b))
    if n == 0:
        return 1.0
    return sum(abs(a - b) for a, b in zip(fp_a[:n], fp_b[:n])) / n


def find_duplicates(db_a: dict, db_b: dict, dir_a: Path, dir_b: Path) -> dict:
    """
    Cross-directory comparison. Returns dict with three match categories:
      exact_duplicates  — identical MD5
      near_duplicates   — same format_fp + content distance < NEAR_DUPLICATE_THRESHOLD
      suspicious        — different format but content distance < SUSPICIOUS_THRESHOLD
    """
    exact, near, suspicious = [], [], []

    for key_a, fp_a in db_a.items():
        for key_b, fp_b in db_b.items():
            path_a = str(dir_a / key_a)
            path_b = str(dir_b / key_b)

            if fp_a["exact_hash"] == fp_b["exact_hash"]:
                exact.append(_match_entry(path_a, fp_a, path_b, fp_b))
                continue

            dist = _content_distance(fp_a["content_fp"], fp_b["content_fp"])
            same_format = fp_a["format_fp"] == fp_b["format_fp"]

            if same_format and dist < NEAR_DUPLICATE_THRESHOLD:
                entry = _match_entry(path_a, fp_a, path_b, fp_b)
                entry["content_distance"] = round(dist, 4)
                near.append(entry)
            elif not same_format and dist < SUSPICIOUS_THRESHOLD:
                entry = _match_entry(path_a, fp_a, path_b, fp_b)
                entry["content_distance"] = round(dist, 4)
                suspicious.append(entry)

    return {"exact_duplicates": exact, "near_duplicates": near, "suspicious": suspicious}


def find_internal_duplicates(db: dict, directory: Path) -> dict:
    """Find duplicates within a single directory's database."""
    keys = list(db.keys())
    exact, near = [], []

    for i in range(len(keys)):
        for j in range(i + 1, len(keys)):
            fp_a = db[keys[i]]
            fp_b = db[keys[j]]
            path_a = str(directory / keys[i])
            path_b = str(directory / keys[j])

            if fp_a["exact_hash"] == fp_b["exact_hash"]:
                exact.append(_match_entry(path_a, fp_a, path_b, fp_b))
                continue

            dist = _content_distance(fp_a["content_fp"], fp_b["content_fp"])
            if fp_a["format_fp"] == fp_b["format_fp"] and dist < NEAR_DUPLICATE_THRESHOLD:
                entry = _match_entry(path_a, fp_a, path_b, fp_b)
                entry["content_distance"] = round(dist, 4)
                near.append(entry)

    return {"exact_duplicates": exact, "near_duplicates": near, "suspicious": []}


def _match_entry(path_a: str, fp_a: dict, path_b: str, fp_b: dict) -> dict:
    return {
        "file_a": path_a,
        "file_b": path_b,
        "meta_a": fp_a["meta"],
        "meta_b": fp_b["meta"],
    }


# ---------------------------------------------------------------------------
# Report generation
# ---------------------------------------------------------------------------

def _fmt_meta(meta: dict) -> str:
    sr = meta["sample_rate"]
    bd = meta["bit_depth"]
    ch = "mono" if meta["channels"] == 1 else f"{meta['channels']}ch"
    dur = meta["duration_ms"]
    size_kb = meta["file_size"] // 1024
    return f"{sr}Hz / {bd}bit / {ch} / {dur}ms / {size_kb}KB"


def _recommend_keep(meta_a: dict, meta_b: dict, path_a: str, path_b: str) -> str:
    qa = _quality_score(meta_a)
    qb = _quality_score(meta_b)
    if qa > qb:
        return f"KEEP: {path_a}"
    if qb > qa:
        return f"KEEP: {path_b}"
    return "KEEP: either (equal quality)"


def _storage_waste(matches: list) -> int:
    """Estimate bytes recoverable by removing the lower-quality duplicate."""
    total = 0
    for m in matches:
        qa = _quality_score(m["meta_a"])
        qb = _quality_score(m["meta_b"])
        discard_meta = m["meta_b"] if qa >= qb else m["meta_a"]
        total += discard_meta["file_size"]
    return total


def generate_report(results: dict, dir_a: Path, dir_b: Path = None) -> str:
    lines = []
    sep = "=" * 72
    thin = "-" * 72
    lines += [sep, "XPN SAMPLE FINGERPRINTER — DEDUPLICATION REPORT", sep]

    if dir_b:
        lines.append(f"Directory A : {dir_a}")
        lines.append(f"Directory B : {dir_b}")
    else:
        lines.append(f"Directory   : {dir_a}")
    lines.append("")

    exact = results["exact_duplicates"]
    near = results["near_duplicates"]
    suspicious = results["suspicious"]
    waste_bytes = _storage_waste(exact) + _storage_waste(near)
    waste_mb = waste_bytes / (1024 * 1024)

    lines += [
        "SUMMARY",
        f"  Exact duplicates   : {len(exact)}",
        f"  Near-duplicates    : {len(near)}",
        f"  Suspicious matches : {len(suspicious)}",
        f"  Est. storage waste : {waste_mb:.2f} MB",
        "",
    ]

    if exact:
        lines += [thin, f"EXACT DUPLICATES ({len(exact)})",
                  "  Identical audio data — safe to remove the lower-quality copy.", ""]
        for idx, m in enumerate(exact, 1):
            lines.append(f"  [{idx}]")
            lines.append(f"      A: {m['file_a']}")
            lines.append(f"         {_fmt_meta(m['meta_a'])}")
            lines.append(f"      B: {m['file_b']}")
            lines.append(f"         {_fmt_meta(m['meta_b'])}")
            lines.append(f"      {_recommend_keep(m['meta_a'], m['meta_b'], m['file_a'], m['file_b'])}")
            lines.append("")

    if near:
        lines += [thin, f"NEAR-DUPLICATES ({len(near)})",
                  "  Same format, very similar content — likely minor edits or re-saves.", ""]
        for idx, m in enumerate(near, 1):
            dist = m.get("content_distance", "?")
            lines.append(f"  [{idx}]  content distance: {dist}")
            lines.append(f"      A: {m['file_a']}")
            lines.append(f"         {_fmt_meta(m['meta_a'])}")
            lines.append(f"      B: {m['file_b']}")
            lines.append(f"         {_fmt_meta(m['meta_b'])}")
            lines.append(f"      {_recommend_keep(m['meta_a'], m['meta_b'], m['file_a'], m['file_b'])}")
            lines.append("")

    if suspicious:
        lines += [thin, f"SUSPICIOUS MATCHES ({len(suspicious)})",
                  "  Different format, nearly identical content — possible leaked or re-encoded samples.", ""]
        for idx, m in enumerate(suspicious, 1):
            dist = m.get("content_distance", "?")
            lines.append(f"  [{idx}]  content distance: {dist}")
            lines.append(f"      A: {m['file_a']}")
            lines.append(f"         {_fmt_meta(m['meta_a'])}")
            lines.append(f"      B: {m['file_b']}")
            lines.append(f"         {_fmt_meta(m['meta_b'])}")
            lines.append("")

    if not exact and not near and not suspicious:
        lines += ["No duplicates or suspicious matches found.", ""]

    lines.append(sep)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="WAV sample fingerprinter — deduplication and provenance tracking",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("dir", help="Directory of WAV files to scan")
    parser.add_argument("--compare", metavar="DIR2",
                        help="Compare against a second WAV directory")
    parser.add_argument("--rebuild", action="store_true",
                        help="Force full rescan, ignoring cached DB entries")
    parser.add_argument("--output", metavar="FILE",
                        help="Write report to a text file (default: stdout only)")
    args = parser.parse_args()

    dir_a = Path(args.dir).resolve()
    if not dir_a.is_dir():
        parser.error(f"Not a directory: {dir_a}")

    print("\nXPN Sample Fingerprinter")
    print(f"Scanning: {dir_a}")
    db_a = build_database(dir_a, rebuild=args.rebuild)

    if args.compare:
        dir_b = Path(args.compare).resolve()
        if not dir_b.is_dir():
            parser.error(f"Not a directory: {dir_b}")
        print(f"Comparing against: {dir_b}")
        db_b = build_database(dir_b, rebuild=args.rebuild)
        results = find_duplicates(db_a, db_b, dir_a, dir_b)
        report = generate_report(results, dir_a, dir_b)
    else:
        print("Checking for internal duplicates...")
        results = find_internal_duplicates(db_a, dir_a)
        report = generate_report(results, dir_a)

    print()
    print(report)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(report, encoding="utf-8")
        print(f"Report saved: {out_path}")


if __name__ == "__main__":
    main()
