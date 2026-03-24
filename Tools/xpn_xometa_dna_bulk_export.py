#!/usr/bin/env python3
"""
XPN XOMeta DNA Bulk Exporter — XO_OX Designs
=============================================
Exports all .xometa preset Sonic DNA values to CSV, JSON, or Markdown
for fleet-wide analysis, spreadsheet import, and gap detection.

Computes per-preset:
  - 6D DNA: brightness, warmth, movement, density, space, aggression
  - feliX score: (brightness + (1-warmth) + movement) / 3
  - Oscar score: (warmth + (1-brightness) + density) / 3
  - quadrant: dominant personality axis

Usage:
  python xpn_xometa_dna_bulk_export.py <presets_dir> \\
      [--engine FILTER] [--mood FILTER] \\
      [--format csv|json|markdown] [--output FILE]

Examples:
  # All presets to stdout as CSV
  python xpn_xometa_dna_bulk_export.py ../Presets/XOlokun/

  # Filter to OPAL engine, JSON output
  python xpn_xometa_dna_bulk_export.py ../Presets/XOlokun/ --engine OPAL --format json

  # Markdown summary to file
  python xpn_xometa_dna_bulk_export.py ../Presets/XOlokun/ --format markdown --output dna_report.md
"""

import argparse
import csv
import glob
import json
import os
import sys
from collections import defaultdict

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DIMS = ["brightness", "warmth", "movement", "density", "space", "aggression"]
DEFAULT_DNA_VALUE = 0.5  # neutral fallback — preserve signal, don't crash

CSV_HEADERS = [
    "filename", "path", "engine", "mood",
    "brightness", "warmth", "movement", "density", "space", "aggression",
    "felix_score", "oscar_score", "quadrant",
]


# ---------------------------------------------------------------------------
# DNA extraction helpers
# ---------------------------------------------------------------------------

def extract_dna(data: dict) -> dict:
    """
    Extract the sonic DNA block from a .xometa dict.
    Handles key variants: sonic_dna, dna, sonicDNA.
    Returns a flat dict of {dim: float} with DEFAULT_DNA_VALUE for any missing dim.
    """
    raw = data.get("sonic_dna") or data.get("dna") or data.get("sonicDNA") or {}
    return {dim: float(raw.get(dim, DEFAULT_DNA_VALUE)) for dim in DIMS}


def compute_scores(dna: dict) -> tuple:
    """
    Compute feliX and Oscar composite scores from a DNA dict.
    Returns (felix_score, oscar_score, quadrant).
    """
    b = dna["brightness"]
    w = dna["warmth"]
    m = dna["movement"]
    d = dna["density"]

    felix = (b + (1.0 - w) + m) / 3.0
    oscar = (w + (1.0 - b) + d) / 3.0

    if felix >= oscar:
        quadrant = "feliX-bright" if b >= 0.5 else "feliX-warm"
    else:
        quadrant = "Oscar-bright" if b >= 0.5 else "Oscar-warm"

    return round(felix, 4), round(oscar, 4), quadrant


# ---------------------------------------------------------------------------
# Preset loading
# ---------------------------------------------------------------------------

def load_records(presets_dir: str, engine_filter: str = None, mood_filter: str = None) -> list:
    """
    Recursively scan presets_dir for .xometa files.
    Returns a list of record dicts (one per preset).
    Skips unparseable files with a stderr warning.
    """
    pattern = os.path.join(presets_dir, "**", "*.xometa")
    files = sorted(glob.glob(pattern, recursive=True))

    if not files:
        print(f"WARN: no .xometa files found under {presets_dir}", file=sys.stderr)
        return []

    records = []

    for fpath in files:
        try:
            with open(fpath, encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, IOError) as exc:
            print(f"WARN: skipping {fpath}: {exc}", file=sys.stderr)
            continue

        # Resolve engine — first entry in engines array, or "UNKNOWN"
        engines_list = data.get("engines", [])
        engine = engines_list[0] if engines_list else data.get("engine", "UNKNOWN")

        # Resolve mood — explicit field, or parent directory name
        mood = data.get("mood") or os.path.basename(os.path.dirname(fpath))

        # Apply filters (substring match, case-insensitive)
        if engine_filter and engine_filter.lower() not in engine.lower():
            continue
        if mood_filter and mood_filter.lower() != mood.lower():
            continue

        filename = os.path.basename(fpath)
        rel_path = os.path.relpath(fpath, presets_dir)
        dna = extract_dna(data)
        felix, oscar, quadrant = compute_scores(dna)

        records.append({
            "filename": filename,
            "path": rel_path,
            "engine": engine,
            "mood": mood,
            "brightness": round(dna["brightness"], 4),
            "warmth":     round(dna["warmth"],     4),
            "movement":   round(dna["movement"],   4),
            "density":    round(dna["density"],    4),
            "space":      round(dna["space"],      4),
            "aggression": round(dna["aggression"], 4),
            "felix_score": felix,
            "oscar_score": oscar,
            "quadrant": quadrant,
        })

    return records


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def format_csv(records: list) -> str:
    """Render records as CSV string."""
    import io
    buf = io.StringIO()
    writer = csv.DictWriter(buf, fieldnames=CSV_HEADERS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(records)
    return buf.getvalue()


def format_json(records: list) -> str:
    """Render records as JSON array."""
    return json.dumps(records, indent=2)


def format_markdown(records: list) -> str:
    """
    Render a Markdown report:
      1. Engine × Mood count matrix
      2. Full CSV data block at end
    """
    lines = []

    # --- Summary header ---
    lines.append("# XOMeta Sonic DNA Bulk Export")
    lines.append(f"\n**Total presets:** {len(records)}\n")

    if not records:
        lines.append("_No presets found._")
        return "\n".join(lines)

    # --- Engine × Mood matrix ---
    lines.append("## Engine × Mood Count Matrix\n")

    # Collect all engines and moods
    engines = sorted({r["engine"] for r in records})
    moods = sorted({r["mood"] for r in records})

    # Count matrix
    matrix: dict = defaultdict(lambda: defaultdict(int))
    engine_totals: dict = defaultdict(int)
    for r in records:
        matrix[r["engine"]][r["mood"]] += 1
        engine_totals[r["engine"]] += 1

    # Table header
    header_cols = ["Engine"] + moods + ["Total"]
    lines.append("| " + " | ".join(header_cols) + " |")
    lines.append("| " + " | ".join(["---"] * len(header_cols)) + " |")

    for eng in engines:
        row = [eng]
        for mood in moods:
            row.append(str(matrix[eng][mood]) if matrix[eng][mood] else "—")
        row.append(str(engine_totals[eng]))
        lines.append("| " + " | ".join(row) + " |")

    # Totals row
    totals_row = ["**Total**"]
    for mood in moods:
        totals_row.append(str(sum(matrix[e][mood] for e in engines)))
    totals_row.append(str(len(records)))
    lines.append("| " + " | ".join(totals_row) + " |")

    # --- feliX / Oscar distribution summary ---
    lines.append("\n## Quadrant Distribution\n")
    quad_counts: dict = defaultdict(int)
    for r in records:
        quad_counts[r["quadrant"]] += 1

    quad_order = ["feliX-bright", "feliX-warm", "Oscar-bright", "Oscar-warm"]
    lines.append("| Quadrant | Count | % |")
    lines.append("| --- | --- | --- |")
    for q in quad_order:
        count = quad_counts[q]
        pct = f"{100.0 * count / len(records):.1f}"
        lines.append(f"| {q} | {count} | {pct}% |")

    # --- DNA averages per engine ---
    lines.append("\n## DNA Averages by Engine\n")
    lines.append("| Engine | brightness | warmth | movement | density | space | aggression | feliX | Oscar |")
    lines.append("| --- | --- | --- | --- | --- | --- | --- | --- | --- |")

    for eng in engines:
        eng_records = [r for r in records if r["engine"] == eng]
        n = len(eng_records)
        avgs = {
            dim: round(sum(r[dim] for r in eng_records) / n, 3)
            for dim in DIMS
        }
        avg_felix = round(sum(r["felix_score"] for r in eng_records) / n, 3)
        avg_oscar = round(sum(r["oscar_score"] for r in eng_records) / n, 3)
        lines.append(
            f"| {eng} | {avgs['brightness']} | {avgs['warmth']} | {avgs['movement']} "
            f"| {avgs['density']} | {avgs['space']} | {avgs['aggression']} "
            f"| {avg_felix} | {avg_oscar} |"
        )

    # --- Raw CSV appendix ---
    lines.append("\n## Raw Data (CSV)\n")
    lines.append("```csv")
    lines.append(format_csv(records).rstrip())
    lines.append("```")

    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        description="Export .xometa Sonic DNA values to CSV/JSON/Markdown.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "presets_dir",
        help="Root directory to scan recursively for .xometa files.",
    )
    parser.add_argument(
        "--engine",
        metavar="FILTER",
        help="Only include presets whose engine contains this string (case-insensitive).",
    )
    parser.add_argument(
        "--mood",
        metavar="FILTER",
        help="Only include presets with this exact mood (case-insensitive).",
    )
    parser.add_argument(
        "--format",
        choices=["csv", "json", "markdown"],
        default="csv",
        help="Output format (default: csv).",
    )
    parser.add_argument(
        "--output",
        metavar="FILE",
        help="Write output to FILE instead of stdout.",
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)

    presets_dir = os.path.abspath(args.presets_dir)
    if not os.path.isdir(presets_dir):
        print(f"ERROR: presets_dir not found: {presets_dir}", file=sys.stderr)
        sys.exit(1)

    records = load_records(presets_dir, engine_filter=args.engine, mood_filter=args.mood)

    if args.format == "csv":
        output = format_csv(records)
    elif args.format == "json":
        output = format_json(records)
    else:
        output = format_markdown(records)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as fh:
            fh.write(output)
        print(f"Wrote {len(records)} presets → {args.output}", file=sys.stderr)
    else:
        sys.stdout.write(output)


if __name__ == "__main__":
    main()
