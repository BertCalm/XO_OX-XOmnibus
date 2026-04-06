#!/usr/bin/env python3
"""
XPN Pack Diet Analyzer
======================
Analyzes an XPN pack directory or ZIP to identify size reduction opportunities
for convenient distribution (target: <500 MB).

Usage:
    python xpn_pack_diet_analyzer.py <pack_dir_or_zip> [--target-mb 500]
                                     [--prune-orphans] [--output diet_report.txt]
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
import tempfile
import zipfile
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# WAV helpers
# ---------------------------------------------------------------------------

def _read_wav_duration(data: bytes) -> Optional[float]:
    """Return duration in seconds for a WAV file given its raw bytes, or None."""
    try:
        if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
            return None
        offset = 12
        sample_rate = None
        num_channels = None
        bits_per_sample = None
        data_size = None
        while offset + 8 <= len(data):
            chunk_id = data[offset:offset + 4]
            chunk_size = struct.unpack_from("<I", data, offset + 4)[0]
            chunk_data = data[offset + 8: offset + 8 + chunk_size]
            if chunk_id == b"fmt ":
                if len(chunk_data) >= 16:
                    num_channels = struct.unpack_from("<H", chunk_data, 2)[0]
                    sample_rate = struct.unpack_from("<I", chunk_data, 4)[0]
                    bits_per_sample = struct.unpack_from("<H", chunk_data, 14)[0]
            elif chunk_id == b"data":
                data_size = chunk_size
            offset += 8 + chunk_size
            if chunk_size % 2 != 0:
                offset += 1  # word-align
        if sample_rate and num_channels and bits_per_sample and data_size:
            bytes_per_sample = bits_per_sample // 8
            if bytes_per_sample > 0 and num_channels > 0 and sample_rate > 0:
                total_samples = data_size // (bytes_per_sample * num_channels)
                return total_samples / sample_rate
    except Exception as exc:
        print(f"[WARN] Reading WAV duration from bytes: {exc}", file=sys.stderr)
    return None


# ---------------------------------------------------------------------------
# XPM parsing — extract referenced sample paths
# ---------------------------------------------------------------------------

def _extract_sample_refs_from_xpm(xpm_text: str) -> list[str]:
    """Return list of sample file paths referenced in an XPM XML document."""
    refs: list[str] = []
    try:
        root = ET.fromstring(xpm_text)
        for elem in root.iter():
            # MPC XPM stores sample paths in <SampleFile> or <FileName> elements
            # and occasionally as attributes named SampleFile / FileName
            tag = elem.tag
            if tag in ("SampleFile", "FileName", "SamplePath"):
                if elem.text and elem.text.strip():
                    refs.append(elem.text.strip())
            for attr in ("SampleFile", "FileName", "SamplePath", "file"):
                val = elem.get(attr, "")
                if val:
                    refs.append(val.strip())
    except ET.ParseError as exc:
        print(f"[WARN] Parsing XPM XML for sample references: {exc}", file=sys.stderr)
    return refs


# ---------------------------------------------------------------------------
# Pack loader — supports directory or ZIP
# ---------------------------------------------------------------------------

class PackFile:
    """Thin abstraction over a file entry in a pack (dir or ZIP)."""

    def __init__(self, rel_path: str, size: int, read_fn):
        self.rel_path = rel_path          # slash-separated, relative to pack root
        self.size = size
        self._read_fn = read_fn
        self.suffix = Path(rel_path).suffix.lower()
        self.name = Path(rel_path).name

    def read(self) -> bytes:
        return self._read_fn()


def load_pack(source: Path) -> list[PackFile]:
    """Load all files from a pack directory or ZIP, returning PackFile list."""
    files: list[PackFile] = []

    if source.is_dir():
        for p in sorted(source.rglob("*")):
            if p.is_file():
                rel = str(p.relative_to(source)).replace("\\", "/")
                size = p.stat().st_size

                def _make_reader(path=p):
                    def _reader():
                        return path.read_bytes()
                    return _reader

                files.append(PackFile(rel, size, _make_reader()))

    elif source.suffix.lower() in (".xpn", ".zip"):
        with zipfile.ZipFile(source, "r") as zf:
            for info in zf.infolist():
                if info.is_dir():
                    continue
                rel = info.filename.replace("\\", "/")
                size = info.file_size  # uncompressed size
                compressed_size = info.compress_size

                def _make_reader(zf_path=source, name=info.filename):
                    def _reader():
                        with zipfile.ZipFile(zf_path, "r") as _zf:
                            return _zf.read(name)
                    return _reader

                pf = PackFile(rel, size, _make_reader())
                pf.compressed_size = compressed_size
                files.append(pf)
    else:
        raise ValueError(f"Unsupported source: {source}. Must be a directory, .xpn, or .zip file.")

    # Attach compressed_size = size for directory files (no compression)
    for f in files:
        if not hasattr(f, "compressed_size"):
            f.compressed_size = f.size

    return files


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------

def analyze_pack(files: list[PackFile], prune_orphans: bool = False) -> dict:
    """Run all analyses and return a structured results dict."""

    # --- Basic inventory ---
    total_uncompressed = sum(f.size for f in files)
    total_compressed = sum(f.compressed_size for f in files)

    # Size by file type
    by_type: dict[str, int] = defaultdict(int)
    for f in files:
        by_type[f.suffix or "(no ext)"] += f.size

    # Size by top-level subdirectory
    by_subdir: dict[str, int] = defaultdict(int)
    for f in files:
        parts = f.rel_path.split("/")
        key = parts[0] if len(parts) > 1 else "(root)"
        by_subdir[key] += f.size

    # WAV files
    wav_files = [f for f in files if f.suffix == ".wav"]
    xpm_files = [f for f in files if f.suffix == ".xpm"]

    # --- Top 10 largest samples ---
    top_samples = sorted(wav_files, key=lambda f: f.size, reverse=True)[:10]

    # --- Audio duration totals ---
    total_duration_s: float = 0.0
    duration_sample_count = 0
    for f in wav_files:
        dur = _read_wav_duration(f.read())
        if dur is not None:
            total_duration_s += dur
            duration_sample_count += 1

    # Content density: seconds of audio per MB of WAV data
    wav_size_mb = sum(f.size for f in wav_files) / (1024 * 1024)
    content_density = (total_duration_s / wav_size_mb) if wav_size_mb > 0 else 0.0

    # --- XPM reference scanning ---
    # Build map: sample filename (lowercased) -> set of XPM paths that reference it
    sample_xpm_refs: dict[str, set[str]] = defaultdict(set)
    by_program: dict[str, int] = {}

    for xpm in xpm_files:
        text = xpm.read().decode("utf-8", errors="replace")
        refs = _extract_sample_refs_from_xpm(text)
        # Normalize: just the filename portion
        for ref in refs:
            fname = Path(ref.replace("\\", "/")).name.lower()
            if fname:
                sample_xpm_refs[fname].add(xpm.rel_path)
        # Program size = XPM itself + all WAVs it references
        prog_size = xpm.size
        for ref in refs:
            fname = Path(ref.replace("\\", "/")).name.lower()
            for wf in wav_files:
                if wf.name.lower() == fname:
                    prog_size += wf.size
        by_program[xpm.rel_path] = prog_size

    # Classify WAV samples
    wav_name_to_file: dict[str, PackFile] = {}
    for f in wav_files:
        wav_name_to_file[f.name.lower()] = f

    orphaned: list[PackFile] = []
    shared: list[PackFile] = []
    for f in wav_files:
        refs = sample_xpm_refs.get(f.name.lower(), set())
        if len(refs) == 0:
            orphaned.append(f)
        elif len(refs) > 1:
            shared.append(f)

    orphaned_size = sum(f.size for f in orphaned)

    # --- Compression potential ---
    wav_compressed = sum(f.compressed_size for f in wav_files)
    wav_uncompressed = sum(f.size for f in wav_files)
    if wav_uncompressed > 0:
        wav_compression_ratio = 1.0 - (wav_compressed / wav_uncompressed)
    else:
        wav_compression_ratio = 0.0

    return {
        "total_uncompressed_bytes": total_uncompressed,
        "total_compressed_bytes": total_compressed,
        "by_type": dict(by_type),
        "by_subdir": dict(by_subdir),
        "by_program": by_program,
        "wav_count": len(wav_files),
        "xpm_count": len(xpm_files),
        "top_samples": [(f.rel_path, f.size) for f in top_samples],
        "orphaned": [(f.rel_path, f.size) for f in orphaned],
        "orphaned_size_bytes": orphaned_size,
        "shared": [(f.rel_path, len(sample_xpm_refs[f.name.lower()])) for f in shared],
        "total_duration_s": total_duration_s,
        "duration_sample_count": duration_sample_count,
        "content_density_s_per_mb": content_density,
        "wav_compression_ratio": wav_compression_ratio,
        "wav_size_bytes": wav_uncompressed,
    }


# ---------------------------------------------------------------------------
# Report formatting
# ---------------------------------------------------------------------------

MB = 1024 * 1024


def _mb(n: int) -> str:
    return f"{n / MB:.1f} MB"


def _pct(n: float) -> str:
    return f"{n * 100:.1f}%"


def format_report(results: dict, target_mb: int) -> str:
    lines: list[str] = []
    a = lines.append

    total_mb = results["total_uncompressed_bytes"] / MB
    total_comp_mb = results["total_compressed_bytes"] / MB

    a("=" * 70)
    a("XPN Pack Diet Analyzer")
    a("=" * 70)
    a("")

    # --- Summary ---
    a("PACK SUMMARY")
    a("-" * 40)
    a(f"  Total uncompressed size : {_mb(results['total_uncompressed_bytes'])}  ({total_mb:.0f} MB)")
    a(f"  Total compressed size   : {_mb(results['total_compressed_bytes'])}  ({total_comp_mb:.0f} MB)")
    a(f"  WAV files               : {results['wav_count']}")
    a(f"  XPM programs            : {results['xpm_count']}")
    dur_min = results["total_duration_s"] / 60
    a(f"  Total audio duration    : {dur_min:.1f} min  ({results['duration_sample_count']} samples measured)")
    a(f"  Content density         : {results['content_density_s_per_mb']:.1f} s/MB  ({dur_min:.1f} min in {results['wav_size_bytes']/MB:.1f} MB WAV)")
    a("")

    # --- Size by file type ---
    a("SIZE BY FILE TYPE")
    a("-" * 40)
    sorted_types = sorted(results["by_type"].items(), key=lambda x: x[1], reverse=True)
    for ext, sz in sorted_types:
        a(f"  {ext:<12}  {_mb(sz):>12}")
    a("")

    # --- Size by subdirectory ---
    a("SIZE BY SUBDIRECTORY")
    a("-" * 40)
    sorted_dirs = sorted(results["by_subdir"].items(), key=lambda x: x[1], reverse=True)
    for subdir, sz in sorted_dirs[:15]:
        a(f"  {subdir:<30}  {_mb(sz):>12}")
    if len(sorted_dirs) > 15:
        a(f"  ... ({len(sorted_dirs) - 15} more subdirectories)")
    a("")

    # --- Top 10 largest samples ---
    a("TOP 10 LARGEST SAMPLES")
    a("-" * 40)
    for i, (path, sz) in enumerate(results["top_samples"], 1):
        a(f"  {i:2d}. {_mb(sz):>10}  {path}")
    a("")

    # --- Orphaned samples ---
    orphaned = results["orphaned"]
    a("ORPHANED SAMPLES (referenced by 0 XPM programs — safe to remove)")
    a("-" * 40)
    if orphaned:
        a(f"  Count      : {len(orphaned)}")
        a(f"  Total size : {_mb(results['orphaned_size_bytes'])}")
        for path, sz in sorted(orphaned, key=lambda x: x[1], reverse=True)[:20]:
            a(f"    {_mb(sz):>10}  {path}")
        if len(orphaned) > 20:
            a(f"    ... ({len(orphaned) - 20} more orphaned files)")
    else:
        a("  None found — all WAV files are referenced by at least one XPM.")
    a("")

    # --- Shared samples ---
    shared = results["shared"]
    a("SHARED SAMPLES (referenced by 2+ XPM programs — do not remove)")
    a("-" * 40)
    if shared:
        a(f"  Count : {len(shared)}")
        for path, ref_count in sorted(shared, key=lambda x: x[1], reverse=True)[:10]:
            a(f"    {ref_count:3d} refs  {path}")
        if len(shared) > 10:
            a(f"    ... ({len(shared) - 10} more shared files)")
    else:
        a("  None — no samples are shared across multiple programs.")
    a("")

    # --- Compression potential ---
    a("COMPRESSION POTENTIAL")
    a("-" * 40)
    a(f"  WAV files compress at approx. {_pct(results['wav_compression_ratio'])} reduction in ZIP.")
    if results["wav_compression_ratio"] < 0.05:
        a("  WAV (PCM) is already low-entropy; ZIP compression yields minimal savings.")
        a("  Better strategy: trim orphans or reduce sample count.")
    a("")

    # --- Size reduction targets ---
    a(f"SIZE REDUCTION TARGETS  (current: {total_mb:.0f} MB)")
    a("-" * 40)
    thresholds = [100, 250, 500]
    if target_mb not in thresholds:
        thresholds.append(target_mb)
    thresholds = sorted(set(thresholds))
    for threshold in thresholds:
        needed = total_mb - threshold
        marker = "  <-- your target" if threshold == target_mb else ""
        if needed <= 0:
            a(f"  {threshold:>5} MB : already under threshold{marker}")
        else:
            orphan_mb = results["orphaned_size_bytes"] / MB
            after_orphan = total_mb - orphan_mb
            status = "achievable by removing orphans" if after_orphan <= threshold else "requires sample curation"
            a(f"  {threshold:>5} MB : need to remove {needed:.0f} MB  ({status}){marker}")
    a("")

    # --- Recommendations ---
    a("RECOMMENDATIONS")
    a("-" * 40)
    recs: list[str] = []
    if orphaned:
        recs.append(
            f"1. Remove {len(orphaned)} orphaned samples ({_mb(results['orphaned_size_bytes'])}) — "
            "run with --prune-orphans to get the list."
        )
    top_sz = results["top_samples"][0][1] if results["top_samples"] else 0
    if top_sz > 10 * MB:
        recs.append(
            f"2. Largest sample is {_mb(top_sz)} — consider trimming silence or splitting into smaller clips."
        )
    if results["content_density_s_per_mb"] < 10:
        recs.append(
            "3. Content density is low (<10 s/MB). Consider using shorter samples or "
            "higher compression (e.g., 16-bit rather than 24-bit WAV)."
        )
    if not recs:
        recs.append("Pack looks lean. No major reduction opportunities detected.")
    for r in recs:
        a(f"  {r}")
    a("")
    a("=" * 70)

    return "\n".join(lines)


def format_orphan_list(results: dict) -> str:
    """Return plain list of orphaned sample filenames."""
    lines = ["# Orphaned samples (safe to remove — not referenced by any XPM program)", ""]
    for path, sz in sorted(results["orphaned"], key=lambda x: x[1], reverse=True):
        lines.append(Path(path).name)
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Analyze XPN pack size and identify reduction opportunities."
    )
    parser.add_argument("pack", help="Path to pack directory or .xpn/.zip file")
    parser.add_argument(
        "--target-mb", type=int, default=500,
        help="Target file size in MB for distribution (default: 500)"
    )
    parser.add_argument(
        "--prune-orphans", action="store_true",
        help="Output a plain list of orphaned sample filenames (never deletes files)"
    )
    parser.add_argument(
        "--output", metavar="FILE",
        help="Write report to this file instead of stdout"
    )
    parser.add_argument(
        "--json", action="store_true",
        help="Output raw analysis JSON instead of formatted report"
    )
    args = parser.parse_args()

    source = Path(args.pack)
    if not source.exists():
        print(f"ERROR: '{source}' does not exist.", file=sys.stderr)
        sys.exit(1)

    print(f"Loading pack: {source} ...", file=sys.stderr)
    files = load_pack(source)
    print(f"  {len(files)} files found. Analyzing ...", file=sys.stderr)

    results = analyze_pack(files, prune_orphans=args.prune_orphans)

    if args.json:
        output_text = json.dumps(results, indent=2, default=str)
    elif args.prune_orphans:
        output_text = format_orphan_list(results)
    else:
        output_text = format_report(results, target_mb=args.target_mb)

    if args.output:
        Path(args.output).write_text(output_text, encoding="utf-8")
        print(f"Report written to: {args.output}", file=sys.stderr)
    else:
        print(output_text)


if __name__ == "__main__":
    main()
