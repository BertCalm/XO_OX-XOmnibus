"""
xpn_pack_analytics.py — XO_OX XPN fleet catalog analytics

Scans a directory of released .xpn packs (ZIP archives) and generates a
catalog analytics report covering mood distribution, engine coverage,
Sonic DNA fleet averages/ranges, feliX-Oscar space gaps, and summary stats.

Usage:
    python xpn_pack_analytics.py --packs-dir ./dist [--format text|json|markdown] [--output report.txt]
    python xpn_pack_analytics.py --packs path/a.xpn path/b.xpn [--format markdown]

Output formats:
    text      (default) Clean ASCII report with section headers
    json      Machine-readable dict
    markdown  Tables suitable for pasting into docs
"""

import argparse
import json
import os
import sys
import zipfile
from pathlib import Path
from statistics import mean
from typing import Any

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

DNA_DIMENSIONS = ["brightness", "warmth", "movement", "density", "space", "aggression"]

ALL_ENGINES = [
    "ODDFELIX", "ODDOSCAR", "OVERDUB", "ODYSSEY", "OBLONG", "OBESE",
    "ONSET", "OVERWORLD", "OPAL", "ORGANON", "OUROBOROS", "OBSIDIAN",
    "ORIGAMI", "ORACLE", "OBSCURA", "OCEANIC", "OCELOT", "OVERBITE",
    "ORBITAL", "OPTIC", "OBLIQUE", "OSPREY", "OSTERIA", "OWLFISH",
    "OHM", "ORPHICA", "OBBLIGATO", "OTTONI", "OLE", "OVERLAP", "OUTWIT",
    "OMBRE", "ORCA", "OCTOPUS",
    # Concept engines (no packs yet but should appear in coverage gaps)
    "OSTINATO", "OPENSKY", "OCEANDEEP", "OUIE",
]

VALID_MOODS = {
    "Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family",
    "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous",
    "Organic", "Shadow"
}

PLACEHOLDER_DNA = {d: None for d in DNA_DIMENSIONS}

# ---------------------------------------------------------------------------
# Pack reading
# ---------------------------------------------------------------------------

def read_pack(path: Path) -> dict[str, Any]:
    """
    Open a .xpn ZIP and extract analytics data from bundle_manifest.json.
    Returns a dict with keys: path, name, engine, mood, version, dna, preset_count, warning.
    Falls back gracefully if the manifest is absent or malformed.
    """
    result: dict[str, Any] = {
        "path": str(path),
        "name": path.stem,
        "engine": None,
        "mood": None,
        "version": None,
        "dna": dict(PLACEHOLDER_DNA),
        "preset_count": None,
        "warning": None,
    }

    try:
        with zipfile.ZipFile(path, "r") as zf:
            names = zf.namelist()
            # bundle_manifest.json may sit at root or inside a subdirectory
            manifest_candidates = [n for n in names if n.endswith("bundle_manifest.json")]
            if not manifest_candidates:
                result["warning"] = "No bundle_manifest.json found — using placeholder DNA"
                return result

            manifest_path = manifest_candidates[0]
            with zf.open(manifest_path) as f:
                data = json.load(f)

            result["name"] = data.get("pack_name", path.stem)
            result["version"] = data.get("version")
            result["preset_count"] = data.get("preset_count") or data.get("presetCount")

            # Engine — may be a list (multi-engine pack) or a string
            engines_raw = data.get("engines") or data.get("engine")
            if isinstance(engines_raw, list):
                result["engine"] = [str(e).upper() for e in engines_raw]
            elif isinstance(engines_raw, str):
                result["engine"] = engines_raw.upper()

            # Mood
            mood_raw = data.get("mood") or data.get("moods")
            if isinstance(mood_raw, list):
                result["mood"] = [str(m).capitalize() for m in mood_raw]
            elif isinstance(mood_raw, str):
                result["mood"] = mood_raw.capitalize()

            # Sonic DNA — nested under "sonic_dna" or flat keys
            dna_block = data.get("sonic_dna") or data.get("sonicDNA") or {}
            if not dna_block:
                # Try flat keys as fallback
                dna_block = {d: data.get(d) for d in DNA_DIMENSIONS if d in data}

            parsed_dna: dict[str, float | None] = {}
            for dim in DNA_DIMENSIONS:
                raw = dna_block.get(dim)
                try:
                    val = float(raw)
                    parsed_dna[dim] = max(0.0, min(10.0, val))
                except (TypeError, ValueError):
                    parsed_dna[dim] = None
            result["dna"] = parsed_dna

    except zipfile.BadZipFile:
        result["warning"] = "Not a valid ZIP/XPN archive — skipped"
    except json.JSONDecodeError as exc:
        result["warning"] = f"bundle_manifest.json parse error: {exc}"
    except Exception as exc:  # noqa: BLE001
        result["warning"] = f"Unexpected error reading pack: {exc}"

    return result


def scan_directory(directory: Path) -> list[dict[str, Any]]:
    """Return analytics dicts for all .xpn files found in directory (non-recursive)."""
    packs = sorted(directory.glob("*.xpn"))
    if not packs:
        # Try one level deep
        packs = sorted(directory.glob("**/*.xpn"))
    return [read_pack(p) for p in packs]


# ---------------------------------------------------------------------------
# Analytics computation
# ---------------------------------------------------------------------------

def compute_analytics(packs: list[dict[str, Any]]) -> dict[str, Any]:
    """Aggregate all pack data into a structured analytics dict."""

    total = len(packs)
    warnings = [p for p in packs if p["warning"]]

    # ---- Preset count ----
    preset_counts = [p["preset_count"] for p in packs if p["preset_count"] is not None]
    total_presets = sum(preset_counts) if preset_counts else None

    # ---- Mood distribution ----
    mood_counts: dict[str, int] = {}
    for p in packs:
        moods = p["mood"] if isinstance(p["mood"], list) else ([p["mood"]] if p["mood"] else ["Unknown"])
        for m in moods:
            mood_counts[m] = mood_counts.get(m, 0) + 1

    mood_distribution = {
        m: {"count": c, "pct": round(c / total * 100, 1) if total else 0}
        for m, c in sorted(mood_counts.items(), key=lambda x: -x[1])
    }

    # ---- Engine coverage ----
    covered_engines: set[str] = set()
    for p in packs:
        if isinstance(p["engine"], list):
            covered_engines.update(p["engine"])
        elif p["engine"]:
            covered_engines.add(p["engine"])

    missing_engines = sorted(set(ALL_ENGINES) - covered_engines)
    covered_list = sorted(covered_engines)

    # ---- DNA fleet stats ----
    dna_stats: dict[str, dict[str, Any]] = {}
    for dim in DNA_DIMENSIONS:
        values = [p["dna"][dim] for p in packs if p["dna"].get(dim) is not None]
        if values:
            dna_stats[dim] = {
                "min": round(min(values), 2),
                "max": round(max(values), 2),
                "mean": round(mean(values), 2),
                "count": len(values),
            }
        else:
            dna_stats[dim] = {"min": None, "max": None, "mean": None, "count": 0}

    # ---- feliX-Oscar DNA gap analysis (brightness × warmth quadrants) ----
    # feliX = high brightness (>= 5), Oscar = low brightness (< 5)
    # warmth axis: warm >= 5, cool < 5
    # Quadrants: bright-warm / bright-cool / dark-warm / dark-cool
    quadrants: dict[str, int] = {
        "bright-warm": 0,
        "bright-cool": 0,
        "dark-warm": 0,
        "dark-cool": 0,
    }
    dna_mapped = 0
    for p in packs:
        b = p["dna"].get("brightness")
        w = p["dna"].get("warmth")
        if b is not None and w is not None:
            q_b = "bright" if b >= 5 else "dark"
            q_w = "warm" if w >= 5 else "cool"
            quadrants[f"{q_b}-{q_w}"] += 1
            dna_mapped += 1

    min_q = min(quadrants.values()) if dna_mapped else 0
    gaps = [q for q, c in quadrants.items() if c == min_q and dna_mapped > 0]

    quadrant_pcts = {
        q: {"count": c, "pct": round(c / dna_mapped * 100, 1) if dna_mapped else 0}
        for q, c in quadrants.items()
    }

    return {
        "total_packs": total,
        "total_presets": total_presets,
        "packs_with_warnings": len(warnings),
        "warnings": [{"name": w["name"], "warning": w["warning"]} for w in warnings],
        "mood_distribution": mood_distribution,
        "engine_coverage": {
            "covered": covered_list,
            "covered_count": len(covered_list),
            "missing": missing_engines,
            "missing_count": len(missing_engines),
            "total_registered": len(ALL_ENGINES),
        },
        "dna_stats": dna_stats,
        "dna_gap_analysis": {
            "quadrants": quadrant_pcts,
            "underrepresented": gaps,
            "packs_with_dna": dna_mapped,
        },
    }


# ---------------------------------------------------------------------------
# Formatters
# ---------------------------------------------------------------------------

def _bar(pct: float, width: int = 20) -> str:
    filled = round(pct / 100 * width)
    return "[" + "#" * filled + "." * (width - filled) + "]"


def format_text(a: dict[str, Any]) -> str:
    lines = []
    sep = "=" * 60

    lines += [sep, "  XO_OX XPN FLEET ANALYTICS", sep, ""]

    lines += [f"  Total packs analysed : {a['total_packs']}"]
    if a["total_presets"] is not None:
        lines += [f"  Total presets (est.) : {a['total_presets']}"]
    if a["packs_with_warnings"]:
        lines += [f"  Packs with warnings  : {a['packs_with_warnings']}"]
    lines += [""]

    # Mood distribution
    lines += ["--- MOOD DISTRIBUTION " + "-" * 38]
    for mood, d in a["mood_distribution"].items():
        bar = _bar(d["pct"])
        lines += [f"  {mood:<14} {bar} {d['count']:>3}  ({d['pct']:>5.1f}%)"]
    lines += [""]

    # Engine coverage
    ec = a["engine_coverage"]
    lines += ["--- ENGINE COVERAGE " + "-" * 40]
    lines += [f"  Covered : {ec['covered_count']} / {ec['total_registered']}"]
    if ec["covered"]:
        lines += ["  Engines : " + ", ".join(ec["covered"])]
    if ec["missing"]:
        lines += [f"  Missing : " + ", ".join(ec["missing"])]
    lines += [""]

    # DNA stats
    lines += ["--- SONIC DNA FLEET STATS (0–10 scale) " + "-" * 21]
    lines += [f"  {'Dimension':<12}  {'Min':>5}  {'Max':>5}  {'Mean':>5}  {'N':>4}"]
    lines += [f"  {'-'*12}  {'-----':>5}  {'-----':>5}  {'-----':>5}  {'----':>4}"]
    for dim, s in a["dna_stats"].items():
        if s["count"]:
            lines += [
                f"  {dim:<12}  {s['min']:>5.2f}  {s['max']:>5.2f}  {s['mean']:>5.2f}  {s['count']:>4}"
            ]
        else:
            lines += [f"  {dim:<12}  {'n/a':>5}  {'n/a':>5}  {'n/a':>5}  {0:>4}"]
    lines += [""]

    # feliX-Oscar gap analysis
    gap = a["dna_gap_analysis"]
    lines += ["--- feliX–OSCAR SPACE GAP ANALYSIS " + "-" * 25]
    lines += [f"  Packs with DNA : {gap['packs_with_dna']}"]
    lines += [f"  {'Quadrant':<14}  {'Count':>5}  {'%':>6}"]
    lines += [f"  {'-'*14}  {'-----':>5}  {'------':>6}"]
    for q, d in gap["quadrants"].items():
        marker = "  <-- GAP" if q in gap["underrepresented"] and gap["packs_with_dna"] else ""
        lines += [f"  {q:<14}  {d['count']:>5}  {d['pct']:>5.1f}%{marker}"]
    if gap["underrepresented"] and gap["packs_with_dna"]:
        lines += [f"\n  Underrepresented zone(s): {', '.join(gap['underrepresented'])}"]
    lines += [""]

    # Warnings
    if a["warnings"]:
        lines += ["--- WARNINGS " + "-" * 47]
        for w in a["warnings"]:
            lines += [f"  {w['name']}: {w['warning']}"]
        lines += [""]

    lines += [sep]
    return "\n".join(lines)


def format_markdown(a: dict[str, Any]) -> str:
    lines = []

    lines += ["# XO_OX XPN Fleet Analytics\n"]
    lines += [f"**Total packs:** {a['total_packs']}  "]
    if a["total_presets"] is not None:
        lines += [f"**Total presets (estimated):** {a['total_presets']}  "]
    lines += [""]

    # Mood distribution
    lines += ["## Mood Distribution\n"]
    lines += ["| Mood | Count | % of Fleet |"]
    lines += ["|------|------:|-----------:|"]
    for mood, d in a["mood_distribution"].items():
        lines += [f"| {mood} | {d['count']} | {d['pct']:.1f}% |"]
    lines += [""]

    # Engine coverage
    ec = a["engine_coverage"]
    lines += ["## Engine Coverage\n"]
    lines += [f"**{ec['covered_count']} / {ec['total_registered']} engines covered**\n"]
    if ec["covered"]:
        lines += ["**Covered:** " + ", ".join(f"`{e}`" for e in ec["covered"]) + "\n"]
    if ec["missing"]:
        lines += ["**Missing:** " + ", ".join(f"`{e}`" for e in ec["missing"]) + "\n"]

    # DNA stats
    lines += ["## Sonic DNA Fleet Stats\n"]
    lines += ["| Dimension | Min | Max | Mean | N |"]
    lines += ["|-----------|----:|----:|-----:|--:|"]
    for dim, s in a["dna_stats"].items():
        if s["count"]:
            lines += [f"| {dim} | {s['min']:.2f} | {s['max']:.2f} | {s['mean']:.2f} | {s['count']} |"]
        else:
            lines += [f"| {dim} | n/a | n/a | n/a | 0 |"]
    lines += [""]

    # Gap analysis
    gap = a["dna_gap_analysis"]
    lines += ["## feliX–Oscar Space Gap Analysis\n"]
    lines += [f"*Packs with DNA: {gap['packs_with_dna']}*\n"]
    lines += ["| Quadrant | Count | % | Gap? |"]
    lines += ["|----------|------:|--:|------|"]
    for q, d in gap["quadrants"].items():
        is_gap = "YES" if q in gap["underrepresented"] and gap["packs_with_dna"] else ""
        lines += [f"| {q} | {d['count']} | {d['pct']:.1f}% | {is_gap} |"]
    lines += [""]

    # Warnings
    if a["warnings"]:
        lines += ["## Warnings\n"]
        for w in a["warnings"]:
            lines += [f"- **{w['name']}**: {w['warning']}"]
        lines += [""]

    return "\n".join(lines)


def format_json(a: dict[str, Any]) -> str:
    return json.dumps(a, indent=2)


FORMATTERS = {
    "text": format_text,
    "json": format_json,
    "markdown": format_markdown,
}

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Analyse a fleet of XO_OX .xpn packs and report DNA coverage, mood distribution, and engine gaps.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    source = p.add_mutually_exclusive_group(required=True)
    source.add_argument(
        "--packs-dir",
        metavar="DIR",
        help="Directory to scan for *.xpn files.",
    )
    source.add_argument(
        "--packs",
        nargs="+",
        metavar="FILE",
        help="Explicit list of .xpn file paths.",
    )
    p.add_argument(
        "--format",
        choices=["text", "json", "markdown"],
        default="text",
        help="Output format (default: text).",
    )
    p.add_argument(
        "--output",
        metavar="FILE",
        help="Write report to FILE instead of stdout.",
    )
    return p


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    # Gather packs
    if args.packs_dir:
        directory = Path(args.packs_dir)
        if not directory.is_dir():
            parser.error(f"--packs-dir '{directory}' is not a directory")
        packs = scan_directory(directory)
    else:
        packs = [read_pack(Path(p)) for p in args.packs]

    if not packs:
        print("No .xpn packs found. Nothing to analyse.", file=sys.stderr)
        sys.exit(1)

    # Warn about skipped files
    for p in packs:
        if p["warning"] and "Not a valid ZIP" in p["warning"]:
            print(f"WARNING: {p['name']}: {p['warning']}", file=sys.stderr)

    analytics = compute_analytics(packs)
    report = FORMATTERS[args.format](analytics)

    if args.output:
        out_path = Path(args.output)
        out_path.write_text(report, encoding="utf-8")
        print(f"Report written to {out_path}", file=sys.stderr)
    else:
        print(report)


if __name__ == "__main__":
    main()
