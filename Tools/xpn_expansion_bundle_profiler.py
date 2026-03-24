#!/usr/bin/env python3
"""
xpn_expansion_bundle_profiler.py
XO_OX XOlokun — XPN Expansion Bundle Profiler

Profiles a released or in-progress XPN expansion bundle (ZIP file) and generates
a comprehensive "bundle fingerprint" useful for QA, marketing copy, and release
comparison across versions.

Usage:
    python xpn_expansion_bundle_profiler.py <bundle.zip> [--json] [--output profile.txt]
"""

import argparse
import json
import struct
import sys
import zipfile
from collections import Counter, defaultdict
from pathlib import Path


# ---------------------------------------------------------------------------
# WAV header parsing
# ---------------------------------------------------------------------------

WAV_RIFF_MAGIC = b"RIFF"
WAV_WAVE_MAGIC = b"WAVE"
WAV_FMT_CHUNK  = b"fmt "

def parse_wav_header(data: bytes):
    """
    Parse a WAV file header from raw bytes.
    Returns a dict with: channels, sample_rate, bit_depth, num_samples, duration_s
    Returns None if the data is too short or not a valid WAV.
    """
    if len(data) < 44:
        return None
    # RIFF header: 4 bytes magic, 4 bytes file size, 4 bytes WAVE
    if data[0:4] != WAV_RIFF_MAGIC or data[8:12] != WAV_WAVE_MAGIC:
        return None

    offset = 12
    fmt_info = None
    data_size = None

    while offset + 8 <= len(data):
        chunk_id   = data[offset:offset+4]
        chunk_size = struct.unpack_from("<I", data, offset+4)[0]
        chunk_data = data[offset+8 : offset+8+chunk_size]

        if chunk_id == WAV_FMT_CHUNK and len(chunk_data) >= 16:
            # fmt chunk: audio_format(2), channels(2), sample_rate(4),
            #            byte_rate(4), block_align(2), bits_per_sample(2)
            audio_format, channels, sample_rate, byte_rate, block_align, bit_depth = \
                struct.unpack_from("<HHIIHH", chunk_data, 0)
            fmt_info = {
                "audio_format": audio_format,
                "channels": channels,
                "sample_rate": sample_rate,
                "bit_depth": bit_depth,
            }

        elif chunk_id == b"data":
            data_size = chunk_size

        offset += 8 + chunk_size
        # Word-align
        if chunk_size % 2 != 0:
            offset += 1

    if fmt_info is None or data_size is None:
        return None

    channels    = fmt_info["channels"]
    sample_rate = fmt_info["sample_rate"]
    bit_depth   = fmt_info["bit_depth"]

    bytes_per_sample = max(1, bit_depth // 8)
    num_samples = data_size // (bytes_per_sample * max(1, channels))
    duration_s  = num_samples / sample_rate if sample_rate > 0 else 0.0

    return {
        "channels":    channels,
        "sample_rate": sample_rate,
        "bit_depth":   bit_depth,
        "num_samples": num_samples,
        "duration_s":  round(duration_s, 4),
    }


# ---------------------------------------------------------------------------
# XPM program type detection
# ---------------------------------------------------------------------------

PROGRAM_TYPE_KEYWORDS = {
    "DrumProgram":      "Drum",
    "KeygroupProgram":  "Keygroup",
    "PluginProgram":    "Plugin",
    "MidiProgram":      "MIDI",
    "CVProgram":        "CV",
    "ClipProgram":      "Clip",
}

def detect_program_type(xml_bytes: bytes) -> str:
    """Heuristically detect XPM program type from XML content."""
    # Scan only first 2 KB — program type is always near the top
    head = xml_bytes[:2048].decode("utf-8", errors="replace")
    for keyword, label in PROGRAM_TYPE_KEYWORDS.items():
        if keyword in head:
            return label
    return "Unknown"


# ---------------------------------------------------------------------------
# Cover art detection
# ---------------------------------------------------------------------------

COVER_ART_EXTENSIONS = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp"}

def is_cover_art(name: str) -> bool:
    return Path(name).suffix.lower() in COVER_ART_EXTENSIONS


# ---------------------------------------------------------------------------
# Core profiling logic
# ---------------------------------------------------------------------------

def profile_bundle(zip_path: str) -> dict:
    """
    Open the ZIP bundle and return a complete fingerprint dict.
    """
    path = Path(zip_path)
    if not path.exists():
        raise FileNotFoundError(f"Bundle not found: {zip_path}")

    bundle_size_bytes = path.stat().st_size

    programs: list[dict]  = []
    samples:  list[dict]  = []
    cover_art: list[str]  = []
    expansion_meta: dict  = {}
    other_files: list[str] = []

    total_uncompressed = 0
    unreadable_wavs = 0

    with zipfile.ZipFile(zip_path, "r") as zf:
        entries = zf.infolist()

        for entry in entries:
            total_uncompressed += entry.file_size
            name = entry.filename
            ext  = Path(name).suffix.lower()

            # ---- expansion.json ----------------------------------------
            if Path(name).name.lower() == "expansion.json":
                try:
                    raw = zf.read(name)
                    expansion_meta = json.loads(raw.decode("utf-8", errors="replace"))
                except Exception:
                    expansion_meta = {"_parse_error": True}
                continue

            # ---- XPM programs ------------------------------------------
            if ext == ".xpm":
                try:
                    raw  = zf.read(name)
                    ptype = detect_program_type(raw)
                except Exception:
                    ptype = "Unknown"
                programs.append({
                    "path":  name,
                    "name":  Path(name).stem,
                    "type":  ptype,
                    "size":  entry.file_size,
                })
                continue

            # ---- WAV samples -------------------------------------------
            if ext == ".wav":
                try:
                    raw    = zf.read(name)
                    hdr    = parse_wav_header(raw)
                except Exception:
                    hdr = None

                if hdr is None:
                    unreadable_wavs += 1
                    samples.append({
                        "path":        name,
                        "name":        Path(name).stem,
                        "size":        entry.file_size,
                        "parseable":   False,
                        "channels":    None,
                        "sample_rate": None,
                        "bit_depth":   None,
                        "num_samples": None,
                        "duration_s":  None,
                    })
                else:
                    samples.append({
                        "path":        name,
                        "name":        Path(name).stem,
                        "size":        entry.file_size,
                        "parseable":   True,
                        **hdr,
                    })
                continue

            # ---- Cover art ---------------------------------------------
            if is_cover_art(name):
                cover_art.append(name)
                continue

            # ---- Everything else ---------------------------------------
            other_files.append(name)

    # ------------------------------------------------------------------ #
    # Aggregate statistics                                                 #
    # ------------------------------------------------------------------ #

    # Programs
    program_type_counts = Counter(p["type"] for p in programs)

    # Samples
    parseable_samples = [s for s in samples if s["parseable"]]
    total_duration_s  = sum(s["duration_s"] for s in parseable_samples)

    sample_rate_counts  = Counter(s["sample_rate"] for s in parseable_samples)
    bit_depth_counts    = Counter(s["bit_depth"]   for s in parseable_samples)
    channel_counts      = Counter(s["channels"]    for s in parseable_samples)

    # Compression ratio
    compressed_size   = bundle_size_bytes
    uncompressed_size = total_uncompressed
    compression_ratio = (
        round(uncompressed_size / compressed_size, 3)
        if compressed_size > 0 else None
    )

    # expansion.json fields we care about
    meta_name        = expansion_meta.get("name",        "")
    meta_version     = expansion_meta.get("version",     "")
    meta_description = expansion_meta.get("description", "")
    meta_tags        = expansion_meta.get("tags",        [])

    fingerprint = {
        "bundle": {
            "path":               str(path.resolve()),
            "filename":           path.name,
            "compressed_size_bytes":   compressed_size,
            "uncompressed_size_bytes": uncompressed_size,
            "compression_ratio":  compression_ratio,
        },
        "expansion": {
            "name":        meta_name,
            "version":     meta_version,
            "description": meta_description,
            "tags":        meta_tags,
        },
        "programs": {
            "total":        len(programs),
            "by_type":      dict(program_type_counts),
            "unique_names": len({p["name"] for p in programs}),
            "list":         programs,
        },
        "samples": {
            "total":              len(samples),
            "parseable":          len(parseable_samples),
            "unreadable":         unreadable_wavs,
            "total_duration_s":   round(total_duration_s, 2),
            "total_duration_min": round(total_duration_s / 60, 3),
            "by_sample_rate":     {str(k): v for k, v in sample_rate_counts.most_common()},
            "by_bit_depth":       {str(k): v for k, v in bit_depth_counts.most_common()},
            "by_channels":        {str(k): v for k, v in channel_counts.most_common()},
        },
        "cover_art": {
            "count": len(cover_art),
            "files": cover_art,
        },
        "other_files": {
            "count": len(other_files),
            "files": other_files,
        },
    }

    return fingerprint


# ---------------------------------------------------------------------------
# Human-readable summary renderer
# ---------------------------------------------------------------------------

def human_summary(fp: dict) -> str:
    lines = []
    sep = "─" * 60

    def h(title: str):
        lines.append("")
        lines.append(title)
        lines.append(sep)

    lines.append("XPN EXPANSION BUNDLE FINGERPRINT")
    lines.append("=" * 60)

    # Expansion metadata
    exp = fp["expansion"]
    h("EXPANSION METADATA")
    lines.append(f"  Name        : {exp['name'] or '(not set)'}")
    lines.append(f"  Version     : {exp['version'] or '(not set)'}")
    lines.append(f"  Tags        : {', '.join(exp['tags']) if exp['tags'] else '(none)'}")
    if exp["description"]:
        # Word-wrap description at 56 chars
        words = exp["description"].split()
        line_buf, col = [], 0
        desc_lines = []
        for w in words:
            if col + len(w) + 1 > 56:
                desc_lines.append(" ".join(line_buf))
                line_buf, col = [w], len(w)
            else:
                line_buf.append(w)
                col += len(w) + 1
        if line_buf:
            desc_lines.append(" ".join(line_buf))
        lines.append(f"  Description :")
        for dl in desc_lines:
            lines.append(f"    {dl}")

    # Bundle size
    b = fp["bundle"]
    h("BUNDLE SIZE")
    lines.append(f"  File                 : {b['filename']}")
    lines.append(f"  Compressed size      : {b['compressed_size_bytes']:,} bytes  "
                 f"({b['compressed_size_bytes'] / 1_048_576:.2f} MB)")
    lines.append(f"  Uncompressed size    : {b['uncompressed_size_bytes']:,} bytes  "
                 f"({b['uncompressed_size_bytes'] / 1_048_576:.2f} MB)")
    ratio = b["compression_ratio"]
    lines.append(f"  Compression ratio    : {ratio}× (uncompressed / compressed)" if ratio else
                 f"  Compression ratio    : n/a")

    # Programs
    p = fp["programs"]
    h("PROGRAMS")
    lines.append(f"  Total programs       : {p['total']}")
    lines.append(f"  Unique program names : {p['unique_names']}")
    if p["by_type"]:
        lines.append("  By type:")
        for ptype, cnt in sorted(p["by_type"].items()):
            lines.append(f"    {ptype:<18}: {cnt}")

    # Samples
    s = fp["samples"]
    h("SAMPLES")
    lines.append(f"  Total WAV files      : {s['total']}")
    lines.append(f"  Parseable headers    : {s['parseable']}")
    if s["unreadable"]:
        lines.append(f"  Unreadable headers   : {s['unreadable']}  ⚠")
    lines.append(f"  Total audio duration : {s['total_duration_s']:,.2f} s  "
                 f"({s['total_duration_min']:.2f} min)")
    if s["by_sample_rate"]:
        lines.append("  Sample rates (Hz):")
        for sr, cnt in s["by_sample_rate"].items():
            lines.append(f"    {sr:<10}: {cnt} file(s)")
    if s["by_bit_depth"]:
        lines.append("  Bit depths:")
        for bd, cnt in s["by_bit_depth"].items():
            lines.append(f"    {bd}-bit      : {cnt} file(s)")
    if s["by_channels"]:
        labels = {"1": "mono", "2": "stereo"}
        lines.append("  Channel config:")
        for ch, cnt in s["by_channels"].items():
            label = labels.get(ch, f"{ch}-ch")
            lines.append(f"    {label:<10}: {cnt} file(s)")

    # Cover art
    ca = fp["cover_art"]
    h("COVER ART")
    if ca["count"] == 0:
        lines.append("  (none found)")
    else:
        for f in ca["files"]:
            lines.append(f"  {f}")

    # Other files
    oth = fp["other_files"]
    if oth["count"] > 0:
        h("OTHER FILES")
        for f in oth["files"]:
            lines.append(f"  {f}")

    lines.append("")
    lines.append("=" * 60)
    lines.append("END OF FINGERPRINT")
    lines.append("")
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Profile an XPN expansion bundle ZIP and generate a fingerprint.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python xpn_expansion_bundle_profiler.py XObese.zip
  python xpn_expansion_bundle_profiler.py XObese.zip --json
  python xpn_expansion_bundle_profiler.py XObese.zip --output profile.txt
  python xpn_expansion_bundle_profiler.py XObese.zip --json --output profile.json
"""
    )
    parser.add_argument("bundle",  help="Path to the XPN expansion ZIP bundle")
    parser.add_argument("--json",  action="store_true",
                        help="Output raw JSON fingerprint instead of human-readable summary")
    parser.add_argument("--output", metavar="FILE",
                        help="Write output to FILE instead of stdout")
    args = parser.parse_args()

    try:
        fingerprint = profile_bundle(args.bundle)
    except FileNotFoundError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1
    except zipfile.BadZipFile:
        print(f"ERROR: Not a valid ZIP file: {args.bundle}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"ERROR: Unexpected error — {e}", file=sys.stderr)
        return 1

    if args.json:
        output_text = json.dumps(fingerprint, indent=2)
    else:
        output_text = human_summary(fingerprint)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(output_text, encoding="utf-8")
        print(f"Profile written to: {out_path.resolve()}")
    else:
        print(output_text)

    return 0


if __name__ == "__main__":
    sys.exit(main())
