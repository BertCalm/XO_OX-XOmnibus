#!/usr/bin/env python3
"""
XPN Sample Category Report — XO_OX Designs
Generates a categorized inventory of all WAV samples in a .xpn pack.

Groups samples by type (kick/snare/hat/etc for drum kits, or octave/velocity
layer for melodic programs) and reports per-category statistics.

Usage:
    python xpn_sample_category_report.py <pack.xpn>
    python xpn_sample_category_report.py <pack.xpn> [--program NAME]
    python xpn_sample_category_report.py <pack.xpn> [--format text|json|csv]
    python xpn_sample_category_report.py <pack.xpn> [--output report.txt]
"""

import argparse
import csv
import io
import json
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile
from collections import defaultdict
from pathlib import Path
from typing import Optional


# ---------------------------------------------------------------------------
# Drum category detection
# ---------------------------------------------------------------------------

DRUM_KEYWORDS: dict[str, list[str]] = {
    "KICK":  ["kick", "bd", "bass drum", "bassdrum", "bass_drum", "kik", "kck"],
    "SNARE": ["snare", "snr", "sd", "snaredrum", "rim"],
    "HAT":   ["hat", "hihat", "hi hat", "hi_hat", "hh"],
    "CLAP":  ["clap", "clp", "handclap", "hand_clap"],
    "TOM":   ["tom", "ltom", "mtom", "htom", "floor tom", "floor_tom"],
    "PERC":  ["perc", "rim", "shaker", "tamb", "tambourine", "cowbell",
              "conga", "bongo", "clave", "woodblock", "triangle", "guiro",
              "cabasa", "maracas"],
    "FX":    ["fx", "sfx", "noise", "riser", "sweep", "impact", "crash",
              "cymbal", "cym", "ride", "splash", "reverse", "rev", "vinyl",
              "crackle", "glitch"],
}

# GM MIDI note ranges → drum category
NOTE_RANGES: list[tuple[int, int, str]] = [
    (35, 36, "KICK"),
    (37, 37, "SNARE"),
    (38, 38, "SNARE"),
    (39, 39, "CLAP"),
    (40, 40, "SNARE"),
    (41, 41, "TOM"),
    (42, 46, "HAT"),
    (43, 43, "TOM"),
    (45, 45, "TOM"),
    (47, 47, "TOM"),
    (49, 49, "FX"),
    (50, 50, "TOM"),
    (51, 53, "PERC"),
    (31, 34, "PERC"),
]

NOTE_TO_CATEGORY: dict[int, str] = {}
for _lo, _hi, _cat in NOTE_RANGES:
    for _n in range(_lo, _hi + 1):
        NOTE_TO_CATEGORY[_n] = _cat


def classify_drum(name: str, root_note: Optional[int]) -> str:
    """Return drum category for a sample given its filename stem and MIDI root note."""
    low = name.lower()
    for cat, keywords in DRUM_KEYWORDS.items():
        for kw in keywords:
            if kw in low:
                return cat
    if root_note is not None and root_note in NOTE_TO_CATEGORY:
        return NOTE_TO_CATEGORY[root_note]
    if root_note is not None and root_note >= 54:
        return "FX"
    return "UNCATEGORIZED"


# ---------------------------------------------------------------------------
# WAV duration helper (stdlib only — no scipy/numpy)
# ---------------------------------------------------------------------------

def wav_duration_and_size(data: bytes) -> tuple[float, int]:
    """
    Parse a WAV file's header to extract duration in seconds.
    Returns (duration_seconds, file_size_bytes).
    Falls back to (0.0, len(data)) on any parse error.
    """
    size = len(data)
    try:
        if len(data) < 44 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
            return 0.0, size
        offset = 12
        sample_rate = 0
        num_channels = 0
        bits_per_sample = 0
        data_chunk_size = 0
        while offset + 8 <= len(data):
            chunk_id = data[offset:offset + 4]
            chunk_size = struct.unpack_from("<I", data, offset + 4)[0]
            if chunk_id == b"fmt ":
                num_channels = struct.unpack_from("<H", data, offset + 10)[0]
                sample_rate = struct.unpack_from("<I", data, offset + 12)[0]
                bits_per_sample = struct.unpack_from("<H", data, offset + 22)[0]
            elif chunk_id == b"data":
                data_chunk_size = chunk_size
                break
            offset += 8 + chunk_size
            if chunk_size % 2:
                offset += 1  # word-align
        if sample_rate > 0 and num_channels > 0 and bits_per_sample > 0:
            bytes_per_sample = (bits_per_sample // 8) * num_channels
            total_samples = data_chunk_size // bytes_per_sample
            duration = total_samples / sample_rate
            return duration, size
    except Exception:
        pass
    return 0.0, size


# ---------------------------------------------------------------------------
# XPN parsing
# ---------------------------------------------------------------------------

DRUM_TYPES = {"Drum", "DrumKit", "DRUM_KIT", "drum", "drum_kit"}


def is_drum_program(program_el: ET.Element) -> bool:
    ptype = program_el.get("type", "")
    return ptype in DRUM_TYPES or "drum" in ptype.lower()


def octave_label(root_note: Optional[int]) -> str:
    if root_note is None:
        return "Unknown"
    note_names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    octave = (root_note // 12) - 1
    note = note_names[root_note % 12]
    return f"{note}{octave}"


def octave_range_label(root_note: Optional[int]) -> str:
    if root_note is None:
        return "Unknown octave"
    octave = (root_note // 12) - 1
    return f"C{octave}–B{octave}"


def velocity_layer_label(vel_start: Optional[int]) -> str:
    if vel_start is None:
        return "Layer ?"
    if vel_start < 32:
        return "Layer 1 (pp)"
    if vel_start < 64:
        return "Layer 2 (mp)"
    if vel_start < 96:
        return "Layer 3 (mf)"
    return "Layer 4 (ff)"


def parse_xpn(path: Path, target_program: Optional[str] = None) -> list[dict]:
    """
    Open a .xpn ZIP archive and extract per-program sample inventories.
    Returns a list of program dicts, each containing:
      name, type, is_drum, samples: [{name, root_note, vel_start, duration, size_bytes, category}]
    """
    programs = []

    with zipfile.ZipFile(path) as zf:
        names_in_zip = set(zf.namelist())

        # Find all XPM program files
        xpm_files = [n for n in names_in_zip if n.endswith(".xpm")]
        if not xpm_files:
            print(f"Warning: no .xpm files found in {path.name}", file=sys.stderr)
            return programs

        # Cache WAV data to avoid reading the same file twice
        wav_cache: dict[str, bytes] = {}

        def get_wav(rel_path: str) -> bytes:
            if rel_path not in wav_cache:
                # Try exact path first, then basename match
                if rel_path in names_in_zip:
                    wav_cache[rel_path] = zf.read(rel_path)
                else:
                    base = Path(rel_path).name
                    matches = [n for n in names_in_zip if n.endswith(base)]
                    if matches:
                        wav_cache[rel_path] = zf.read(matches[0])
                    else:
                        wav_cache[rel_path] = b""
            return wav_cache[rel_path]

        for xpm_name in sorted(xpm_files):
            xml_data = zf.read(xpm_name).decode("utf-8", errors="replace")
            try:
                root_el = ET.fromstring(xml_data)
            except ET.ParseError as e:
                print(f"Warning: failed to parse {xpm_name}: {e}", file=sys.stderr)
                continue

            prog_el = root_el if root_el.tag == "Program" else root_el.find("Program")
            if prog_el is None:
                prog_el = root_el

            prog_name = prog_el.get("name", Path(xpm_name).stem)
            prog_type = prog_el.get("type", "Unknown")

            if target_program and target_program.lower() not in prog_name.lower():
                continue

            drum = is_drum_program(prog_el)
            samples: list[dict] = []

            # Walk all Instrument/Layer elements for sample file references
            for instr_el in prog_el.iter("Instrument"):
                root_note_str = instr_el.get("rootNote") or instr_el.get("RootNote")
                root_note = int(root_note_str) if root_note_str and root_note_str.isdigit() else None

                for layer_el in instr_el.iter("Layer"):
                    sample_file = (
                        layer_el.get("sampleFile")
                        or layer_el.get("SampleFile")
                        or layer_el.get("file")
                        or layer_el.get("File")
                        or ""
                    )
                    if not sample_file:
                        continue

                    vel_start_str = layer_el.get("velStart") or layer_el.get("VelStart") or "0"
                    vel_start = int(vel_start_str) if vel_start_str.isdigit() else 0

                    wav_data = get_wav(sample_file)
                    duration, size = wav_duration_and_size(wav_data) if wav_data else (0.0, 0)

                    stem = Path(sample_file).stem

                    if drum:
                        category = classify_drum(stem, root_note)
                    else:
                        # Melodic: group by octave range
                        category = octave_range_label(root_note)

                    samples.append({
                        "name": stem,
                        "root_note": root_note,
                        "vel_start": vel_start,
                        "duration": duration,
                        "size_bytes": size,
                        "category": category,
                    })

            programs.append({
                "name": prog_name,
                "type": prog_type,
                "is_drum": drum,
                "samples": samples,
            })

    return programs


# ---------------------------------------------------------------------------
# Statistics helpers
# ---------------------------------------------------------------------------

def compute_stats(samples: list[dict]) -> dict[str, dict]:
    """Group samples by category and compute per-category stats."""
    groups: dict[str, list[dict]] = defaultdict(list)
    for s in samples:
        groups[s["category"]].append(s)

    stats = {}
    for cat, items in groups.items():
        total_dur = sum(s["duration"] for s in items)
        total_bytes = sum(s["size_bytes"] for s in items)
        stats[cat] = {
            "count": len(items),
            "avg_duration": total_dur / len(items) if items else 0.0,
            "total_duration": total_dur,
            "total_bytes": total_bytes,
        }
    return stats


DRUM_ORDER = ["KICK", "SNARE", "HAT", "CLAP", "TOM", "PERC", "FX", "UNCATEGORIZED"]


def sort_categories(stats: dict, is_drum: bool) -> list[str]:
    if is_drum:
        ordered = [c for c in DRUM_ORDER if c in stats]
        ordered += sorted(c for c in stats if c not in DRUM_ORDER)
        return ordered
    return sorted(stats.keys())


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

BAR_FULL = "█"
BAR_EMPTY = "░"
BAR_WIDTH = 8


def make_bar(count: int, total: int, width: int = BAR_WIDTH) -> str:
    if total == 0:
        return BAR_EMPTY * width
    filled = round(count / total * width)
    return BAR_FULL * filled + BAR_EMPTY * (width - filled)


def fmt_bytes(b: int) -> str:
    if b >= 1_048_576:
        return f"{b / 1_048_576:.1f} MB"
    if b >= 1024:
        return f"{b / 1024:.1f} KB"
    return f"{b} B"


def format_text(pack_name: str, programs: list[dict]) -> str:
    lines = [f"SAMPLE INVENTORY — {pack_name}", ""]

    for prog in programs:
        samples = prog["samples"]
        n = len(samples)
        stats = compute_stats(samples)
        categories = sort_categories(stats, prog["is_drum"])
        total_bytes = sum(s["total_bytes"] for s in stats.values())

        lines.append(f"PROGRAM: {prog['name']} ({prog['type']}, {n} samples)")

        for cat in categories:
            s = stats[cat]
            bar = make_bar(s["count"], n)
            pct = s["count"] / n * 100 if n else 0
            avg_dur = f"{s['avg_duration']:.1f}s" if s["avg_duration"] else "—"
            size_str = fmt_bytes(s["total_bytes"])
            lines.append(
                f"  {cat:<12} {bar}  {s['count']:>3} samples  "
                f"({pct:>5.1f}%)  avg {avg_dur:<6}  total {size_str}"
            )

        lines.append(f"\n  Total: {n} samples, {fmt_bytes(total_bytes)}")
        lines.append("")

    return "\n".join(lines)


def format_json(pack_name: str, programs: list[dict]) -> str:
    output = {"pack": pack_name, "programs": []}
    for prog in programs:
        stats = compute_stats(prog["samples"])
        categories = sort_categories(stats, prog["is_drum"])
        prog_out = {
            "name": prog["name"],
            "type": prog["type"],
            "is_drum": prog["is_drum"],
            "total_samples": len(prog["samples"]),
            "total_bytes": sum(s["size_bytes"] for s in prog["samples"]),
            "categories": [],
        }
        for cat in categories:
            s = stats[cat]
            prog_out["categories"].append({
                "category": cat,
                "count": s["count"],
                "avg_duration_s": round(s["avg_duration"], 3),
                "total_duration_s": round(s["total_duration"], 3),
                "total_bytes": s["total_bytes"],
            })
        output["programs"].append(prog_out)
    return json.dumps(output, indent=2)


def format_csv(pack_name: str, programs: list[dict]) -> str:
    buf = io.StringIO()
    writer = csv.writer(buf)
    writer.writerow(["pack", "program", "type", "is_drum", "category",
                     "count", "avg_duration_s", "total_duration_s", "total_bytes"])
    for prog in programs:
        stats = compute_stats(prog["samples"])
        for cat, s in stats.items():
            writer.writerow([
                pack_name,
                prog["name"],
                prog["type"],
                prog["is_drum"],
                cat,
                s["count"],
                round(s["avg_duration"], 3),
                round(s["total_duration"], 3),
                s["total_bytes"],
            ])
    return buf.getvalue()


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate a categorized sample inventory for a .xpn pack."
    )
    parser.add_argument("pack", help="Path to the .xpn pack file")
    parser.add_argument("--program", metavar="NAME",
                        help="Filter to a single program by name (partial match)")
    parser.add_argument("--format", choices=["text", "json", "csv"], default="text",
                        help="Output format (default: text)")
    parser.add_argument("--output", metavar="FILE",
                        help="Write output to FILE instead of stdout")
    args = parser.parse_args()

    pack_path = Path(args.pack)
    if not pack_path.exists():
        print(f"Error: file not found: {pack_path}", file=sys.stderr)
        sys.exit(1)
    if not zipfile.is_zipfile(pack_path):
        print(f"Error: not a valid ZIP/XPN file: {pack_path}", file=sys.stderr)
        sys.exit(1)

    programs = parse_xpn(pack_path, target_program=args.program)
    if not programs:
        print("No programs found (check --program filter or pack structure).", file=sys.stderr)
        sys.exit(1)

    pack_name = pack_path.name
    fmt = args.format

    if fmt == "text":
        output = format_text(pack_name, programs)
    elif fmt == "json":
        output = format_json(pack_name, programs)
    else:
        output = format_csv(pack_name, programs)

    if args.output:
        Path(args.output).write_text(output, encoding="utf-8")
        print(f"Report written to {args.output}")
    else:
        print(output)


if __name__ == "__main__":
    main()
