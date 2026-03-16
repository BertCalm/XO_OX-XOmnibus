#!/usr/bin/env python3
"""
xpn_coupling_preset_auditor.py — Coupling quality auditor for .xometa preset files.

Checks:
  1. Entangled presets must have active coupling (non-empty pairs, amount > threshold)
  2. Non-Entangled presets with coupling get a WARNING (verify it doesn't require coupling)
  3. Entangled presets with only 1 engine get a WARNING (multi-engine preferred)
  4. Foundation/Atmosphere presets using 3-4 engines get a WARNING (may be too complex)
  5. Coupling amount sub-threshold (< 0.1) flagged as WEAK
  6. Entangled presets lacking both feliX-family and Oscar-family engines get a WARNING

Usage:
    python xpn_coupling_preset_auditor.py <presets_dir>
    python xpn_coupling_preset_auditor.py <presets_dir> --mood Entangled
    python xpn_coupling_preset_auditor.py <presets_dir> --format json
    python xpn_coupling_preset_auditor.py <presets_dir> --strict
"""

import argparse
import glob
import json
import sys
from pathlib import Path
from collections import defaultdict
from typing import Dict, List, Optional

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

COUPLING_THRESHOLD = 0.1  # amounts below this are sub-threshold / no audible effect

# feliX-family engines (bright, high-frequency, neon tetra side)
FELIX_ENGINES = {
    "OddfeliX", "Snap",
    "Overdub", "Dub", "XOverdub",
    "Odyssey", "Drift", "XOdyssey",
    "Onset", "XOnset",
    "Overworld", "XOverworld",
    "Opal", "XOpal",
    "Obese", "Fat", "XObese",
    "Origami",
    "Oracle",
    "Optic",
    "Oblique",
    "Osprey",
    "Ohm",
    "Orphica",
    "Obbligato",
    "Ottoni",
    "Ole",
    "Opensky",
    "Ostinato",
}

# Oscar-family engines (warm, low-frequency, axolotl side)
OSCAR_ENGINES = {
    "OddOscar", "Morph",
    "Oblong", "Bob", "XOblong",
    "Orbital", "XOrbital",
    "Organon", "XOrganon",
    "Ouroboros", "XOuroboros",
    "Obsidian",
    "Obscura",
    "Oceanic",
    "Ocelot",
    "Overbite", "Bite", "XOverbite",
    "Owlfish",
    "Osteria",
    "Octopus",
    "Orca",
    "Ombre",
    "Oblique",
    "Oceandeep",
    "Ouie",
}

ENTANGLED_MOOD = "Entangled"
SIMPLE_MOODS = {"Foundation", "Atmosphere"}

# Coupling strength bands
def classify_coupling_amount(amount: float) -> str:
    if amount < COUPLING_THRESHOLD:
        return "WEAK"
    elif amount < 0.35:
        return "MODERATE"
    else:
        return "STRONG"

# ---------------------------------------------------------------------------
# Preset parsing
# ---------------------------------------------------------------------------

def load_preset(path: Path) -> Optional[dict]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, OSError) as e:
        return None


def get_coupling_pairs(data: dict) -> list:
    """Return list of coupling pair dicts from the preset, normalising field names."""
    coupling = data.get("coupling")
    if not coupling:
        return []
    if isinstance(coupling, dict):
        return coupling.get("pairs") or []
    return []


def max_coupling_amount(pairs: list) -> float:
    """Return the highest coupling amount across all pairs."""
    if not pairs:
        return 0.0
    return max(float(p.get("amount", 0.0)) for p in pairs)


def coupling_strength_label(pairs: list) -> str:
    if not pairs:
        return "NONE"
    amt = max_coupling_amount(pairs)
    if amt < COUPLING_THRESHOLD:
        return "WEAK"
    elif amt < 0.35:
        return "MODERATE"
    else:
        return "STRONG"


def has_felix_engine(engines: list) -> bool:
    return any(e in FELIX_ENGINES for e in engines)


def has_oscar_engine(engines: list) -> bool:
    return any(e in OSCAR_ENGINES for e in engines)

# ---------------------------------------------------------------------------
# Audit logic
# ---------------------------------------------------------------------------

def audit_preset(path: Path) -> dict:
    """
    Returns a result dict:
      {
        "path": Path,
        "name": str,
        "mood": str,
        "engines": [...],
        "coupling_strength": "NONE"|"WEAK"|"MODERATE"|"STRONG",
        "errors": [...],
        "warnings": [...],
      }
    """
    data = load_preset(path)
    result = {
        "path": path,
        "name": path.stem,
        "mood": "Unknown",
        "engines": [],
        "coupling_strength": "NONE",
        "errors": [],
        "warnings": [],
    }

    if data is None:
        result["errors"].append("could not parse JSON")
        return result

    mood = data.get("mood", "Unknown")
    engines = data.get("engines") or []
    pairs = get_coupling_pairs(data)
    ci_label = data.get("couplingIntensity", "")

    result["mood"] = mood
    result["engines"] = engines
    result["coupling_strength"] = coupling_strength_label(pairs)

    # -----------------------------------------------------------------------
    # Check 1: Entangled must have coupling
    # -----------------------------------------------------------------------
    if mood == ENTANGLED_MOOD:
        if not pairs:
            # coupling field may be null or pairs=[]; check couplingIntensity too
            if not ci_label or ci_label in ("None", ""):
                result["errors"].append("no coupling found in Entangled preset")
            else:
                # has a declared intensity but no pairs — structurally incomplete
                result["warnings"].append(
                    f"couplingIntensity='{ci_label}' declared but coupling.pairs is empty"
                )
        else:
            max_amt = max_coupling_amount(pairs)
            if max_amt < COUPLING_THRESHOLD:
                result["warnings"].append(
                    f"coupling amount {max_amt:.2f} (WEAK, below threshold {COUPLING_THRESHOLD})"
                )

    # -----------------------------------------------------------------------
    # Check 2: Non-Entangled WITH coupling
    # -----------------------------------------------------------------------
    if mood != ENTANGLED_MOOD and pairs:
        result["warnings"].append(
            f"non-Entangled preset ({mood}) has coupling pairs — verify it doesn't require coupling to sound good"
        )

    # -----------------------------------------------------------------------
    # Check 3: Entangled single-engine
    # -----------------------------------------------------------------------
    if mood == ENTANGLED_MOOD and len(engines) < 2:
        result["errors"].append("single engine in Entangled preset (coupling requires 2+ engines)")

    # -----------------------------------------------------------------------
    # Check 4: Foundation/Atmosphere with 3-4 engines
    # -----------------------------------------------------------------------
    if mood in SIMPLE_MOODS and len(engines) >= 3:
        result["warnings"].append(
            f"{mood} preset uses {len(engines)} engines (may be too complex for init context)"
        )

    # -----------------------------------------------------------------------
    # Check 5: feliX-Oscar balance in Entangled
    # -----------------------------------------------------------------------
    if mood == ENTANGLED_MOOD and len(engines) >= 2:
        has_f = has_felix_engine(engines)
        has_o = has_oscar_engine(engines)
        if not has_f:
            result["warnings"].append("Entangled preset has no feliX-family engine (unbalanced — Oscar-only)")
        if not has_o:
            result["warnings"].append("Entangled preset has no Oscar-family engine (unbalanced — feliX-only)")

    return result

# ---------------------------------------------------------------------------
# Aggregation
# ---------------------------------------------------------------------------

def run_audit(presets_dir: Path, mood_filter: Optional[str] = None) -> list:
    pattern = str(presets_dir / "**" / "*.xometa")
    all_files = glob.glob(pattern, recursive=True)

    results = []
    for fpath in sorted(all_files):
        path = Path(fpath)
        r = audit_preset(path)
        if mood_filter and r["mood"].lower() != mood_filter.lower():
            continue
        results.append(r)
    return results

# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def text_report(results: list, strict: bool = False) -> str:
    lines = []

    # Group by mood for the header
    by_mood: dict[str, list] = defaultdict(list)
    for r in results:
        by_mood[r["mood"]].append(r)

    errors = [r for r in results if r["errors"]]
    warnings = [r for r in results if r["warnings"] and not r["errors"]]

    # Determine scope label
    moods_present = sorted(by_mood.keys())
    if len(moods_present) == 1:
        scope = f"{moods_present[0]}/"
    elif moods_present:
        scope = ", ".join(moods_present)
    else:
        scope = "all moods"

    lines.append(f"COUPLING AUDIT — {scope} ({len(results)} presets)")
    lines.append("")

    # Errors
    if errors:
        lines.append(f"ERRORs ({len(errors)}):")
        for r in errors:
            rel = r["path"].name
            for msg in r["errors"]:
                lines.append(f"  {rel} — {msg}")
    else:
        lines.append("ERRORs (0): none")

    lines.append("")

    # Warnings (include strict marker)
    warn_label = "WARNINGs" + (" [treated as errors in --strict]" if strict else "")
    if warnings:
        lines.append(f"{warn_label} ({len(warnings)}):")
        for r in warnings:
            rel = r["path"].name
            for msg in r["warnings"]:
                lines.append(f"  {rel} — {msg}")
    else:
        lines.append(f"{warn_label} (0): none")

    lines.append("")

    # Coupling distribution
    strength_counts: dict[str, int] = defaultdict(int)
    for r in results:
        strength_counts[r["coupling_strength"]] += 1

    total = len(results)
    lines.append("Fleet coupling distribution:")
    for label in ("NONE", "WEAK", "MODERATE", "STRONG"):
        count = strength_counts.get(label, 0)
        pct = int(count / total * 100) if total else 0
        lines.append(f"  {label:<10} {count:>4} presets  ({pct}%)")

    lines.append("")

    # Per-mood summary
    if len(by_mood) > 1:
        lines.append("Per-mood breakdown:")
        for mood in sorted(by_mood.keys()):
            mood_list = by_mood[mood]
            m_errors = sum(1 for r in mood_list if r["errors"])
            m_warn = sum(1 for r in mood_list if r["warnings"])
            lines.append(f"  {mood:<14} {len(mood_list):>4} presets  {m_errors} errors  {m_warn} warnings")
        lines.append("")

    # Exit summary
    total_errors = len(errors)
    total_warnings = sum(len(r["warnings"]) for r in results)
    if strict:
        total_errors += total_warnings
    if total_errors == 0:
        lines.append("PASS — no coupling issues found")
    else:
        lines.append(f"FAIL — {total_errors} issue(s) require attention")

    return "\n".join(lines)


def json_report(results: list) -> str:
    out = []
    for r in results:
        out.append({
            "file": str(r["path"]),
            "name": r["name"],
            "mood": r["mood"],
            "engines": r["engines"],
            "coupling_strength": r["coupling_strength"],
            "errors": r["errors"],
            "warnings": r["warnings"],
        })
    return json.dumps(out, indent=2)

# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Audit .xometa preset files for coupling quality.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "presets_dir",
        type=Path,
        help="Root directory containing .xometa preset files (searched recursively)",
    )
    parser.add_argument(
        "--mood",
        metavar="FILTER",
        help="Only audit presets with this mood (e.g. Entangled, Foundation)",
    )
    parser.add_argument(
        "--format",
        choices=["text", "json"],
        default="text",
        help="Output format (default: text)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Treat warnings as errors (non-zero exit if any warnings)",
    )

    args = parser.parse_args()

    if not args.presets_dir.exists():
        print(f"ERROR: directory not found: {args.presets_dir}", file=sys.stderr)
        return 2

    results = run_audit(args.presets_dir, mood_filter=args.mood)

    if not results:
        print("No .xometa files found.", file=sys.stderr)
        return 1

    if args.format == "json":
        print(json_report(results))
    else:
        print(text_report(results, strict=args.strict))

    # Exit code
    has_errors = any(r["errors"] for r in results)
    has_warnings = any(r["warnings"] for r in results)
    if has_errors:
        return 1
    if args.strict and has_warnings:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
