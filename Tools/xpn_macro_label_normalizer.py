#!/usr/bin/env python3
"""
xpn_macro_label_normalizer.py

Normalizes macro labels across .xometa presets to ensure consistency.
Standardizes the 4 canonical XO_OX macro names (CHARACTER, MOVEMENT,
COUPLING, SPACE) while preserving engine-specific macro names.

Usage:
    python xpn_macro_label_normalizer.py <dir> [--dry-run] [--write] [--output report.txt]

Default mode is --dry-run (report only). Use --write to apply changes in-place.
"""

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path

# ---------------------------------------------------------------------------
# Canonical macro names and their known aliases (positional: m1..m4)
# ---------------------------------------------------------------------------

CANONICALS = ["CHARACTER", "MOVEMENT", "COUPLING", "SPACE"]

# Maps lowercase alias → canonical. Built from the alias table so lookups
# are O(1) and case-insensitive.
ALIAS_TABLE: dict[str, str] = {}

_RAW_ALIASES: dict[str, list[str]] = {
    "CHARACTER": [
        "character", "Character", "CHAR", "Char", "char",
        "Color", "color", "COLOR",
        "Timbre", "timbre", "TIMBRE",
        "Tone", "tone", "TONE",
    ],
    "MOVEMENT": [
        "movement", "Movement",
        "Motion", "motion", "MOTION",
        "Animate", "animate", "ANIMATE",
        "Evolve", "evolve", "EVOLVE",
    ],
    "COUPLING": [
        "coupling", "Coupling",
        "Couple", "couple", "COUPLE",
        "Link", "link", "LINK",
        "Connect", "connect", "CONNECT",
    ],
    "SPACE": [
        "space", "Space",
        "Room", "room", "ROOM",
        "Width", "width", "WIDTH",
        "Depth", "depth", "DEPTH",
    ],
}

for _canonical, _aliases in _RAW_ALIASES.items():
    for _alias in _aliases:
        ALIAS_TABLE[_alias.lower()] = _canonical
    # The canonical itself always maps to itself
    ALIAS_TABLE[_canonical.lower()] = _canonical

# Numeric-style keys used in some formats — treated as pass-through
NUMERIC_KEYS = {"m1", "m2", "m3", "m4"}


# ---------------------------------------------------------------------------
# Core normalizer
# ---------------------------------------------------------------------------

def normalize_label(label: str) -> tuple[str, bool]:
    """
    Return (normalized_label, was_changed).

    Rules:
    - Numeric keys (m1–m4): pass-through unchanged.
    - Known alias → canonical.
    - Canonical already: unchanged.
    - Unknown / engine-specific: pass-through unchanged.
    """
    if label.lower() in NUMERIC_KEYS:
        return label, False

    canonical = ALIAS_TABLE.get(label.lower())
    if canonical is None:
        # Engine-specific or unknown — keep as-is
        return label, False

    changed = label != canonical
    return canonical, changed


def process_file(path: Path, write: bool) -> dict:
    """
    Process a single .xometa file.

    Returns a result dict with keys:
        path, engine, preset_name, original_labels, normalized_labels,
        changes (list of (index, old, new)), had_changes, error
    """
    result = {
        "path": str(path),
        "engine": None,
        "preset_name": None,
        "original_labels": [],
        "normalized_labels": [],
        "changes": [],
        "had_changes": False,
        "error": None,
    }

    try:
        raw = path.read_text(encoding="utf-8")
        data = json.loads(raw)
    except Exception as exc:
        result["error"] = f"Parse error: {exc}"
        return result

    # Derive engine name from parent directory tree or "engines" key
    engines_field = data.get("engines") or data.get("engine")
    if isinstance(engines_field, list) and engines_field:
        result["engine"] = engines_field[0]
    elif isinstance(engines_field, str):
        result["engine"] = engines_field
    else:
        # Fallback: walk up from file looking for engine folder name
        for part in reversed(path.parts):
            if part not in ("Presets", "Foundation", "Atmosphere", "Entangled",
                            "Prism", "Flux", "Aether", "Family"):
                candidate = part
                if not candidate.endswith(".xometa"):
                    result["engine"] = candidate
                    break

    result["preset_name"] = data.get("name", path.stem)

    macro_labels = data.get("macroLabels")
    if not isinstance(macro_labels, list):
        # Nothing to normalize in this file
        return result

    result["original_labels"] = list(macro_labels)
    normalized = []
    changes = []

    for i, label in enumerate(macro_labels):
        if not isinstance(label, str):
            normalized.append(label)
            continue
        norm, changed = normalize_label(label)
        normalized.append(norm)
        if changed:
            changes.append((i, label, norm))

    result["normalized_labels"] = normalized
    result["changes"] = changes
    result["had_changes"] = bool(changes)

    if write and changes:
        data["macroLabels"] = normalized
        try:
            path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                            encoding="utf-8")
        except Exception as exc:
            result["error"] = f"Write error: {exc}"

    return result


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def build_report(results: list[dict], write: bool) -> str:
    mode_label = "WRITE MODE" if write else "DRY-RUN MODE"
    lines: list[str] = []

    lines.append("=" * 72)
    lines.append(f"XPN Macro Label Normalizer — {mode_label}")
    lines.append("=" * 72)

    total_files = len(results)
    errored = [r for r in results if r["error"]]
    changed = [r for r in results if r["had_changes"]]
    no_macros = [r for r in results if not r["original_labels"] and not r["error"]]

    lines.append(f"\nScanned:        {total_files} .xometa files")
    lines.append(f"With macros:    {total_files - len(no_macros) - len(errored)}")
    lines.append(f"Normalized:     {len(changed)}")
    lines.append(f"Errors:         {len(errored)}")

    # --- Per-engine breakdown ---
    engine_data: dict[str, dict] = defaultdict(lambda: {
        "custom": set(), "canonical": set(), "normalized_count": 0, "file_count": 0
    })

    for r in results:
        if r["error"]:
            continue
        eng = r["engine"] or "Unknown"
        engine_data[eng]["file_count"] += 1

        labels = r["normalized_labels"] or r["original_labels"]
        for label in labels:
            if label in CANONICALS:
                engine_data[eng]["canonical"].add(label)
            elif label.lower() not in NUMERIC_KEYS:
                engine_data[eng]["custom"].add(label)

        if r["had_changes"]:
            engine_data[eng]["normalized_count"] += 1

    lines.append("\n" + "-" * 72)
    lines.append("ENGINE MACRO SUMMARY")
    lines.append("-" * 72)
    lines.append(f"{'Engine':<24} {'Presets':>7}  {'Normalized':>10}  Labels Used")

    for eng in sorted(engine_data):
        ed = engine_data[eng]
        all_labels = sorted(ed["canonical"]) + sorted(ed["custom"])
        label_str = ", ".join(all_labels) if all_labels else "(none)"
        custom_flag = "  [custom]" if ed["custom"] else ""
        lines.append(
            f"  {eng:<22} {ed['file_count']:>7}  {ed['normalized_count']:>10}  "
            f"{label_str}{custom_flag}"
        )

    # --- Canonical-usage engines vs custom-macro engines ---
    canonical_engines = sorted(
        eng for eng, ed in engine_data.items() if ed["custom"] == set()
    )
    custom_engines = sorted(
        eng for eng, ed in engine_data.items() if ed["custom"]
    )

    lines.append("\n" + "-" * 72)
    lines.append("ENGINES USING CANONICAL MACROS ONLY")
    lines.append("-" * 72)
    if canonical_engines:
        for eng in canonical_engines:
            lines.append(f"  {eng}")
    else:
        lines.append("  (none)")

    lines.append("\n" + "-" * 72)
    lines.append("ENGINES WITH CUSTOM (ENGINE-SPECIFIC) MACRO NAMES")
    lines.append("-" * 72)
    if custom_engines:
        for eng in custom_engines:
            custom = ", ".join(sorted(engine_data[eng]["custom"]))
            lines.append(f"  {eng:<24}  {custom}")
    else:
        lines.append("  (none)")

    # --- Detailed change log ---
    if changed:
        lines.append("\n" + "-" * 72)
        lines.append("NORMALIZED PRESETS (detail)")
        lines.append("-" * 72)
        for r in changed:
            lines.append(f"\n  {r['path']}")
            lines.append(f"    Engine: {r['engine']}  |  Preset: {r['preset_name']}")
            for idx, old, new in r["changes"]:
                lines.append(f"    m{idx+1}: \"{old}\" → \"{new}\"")

    # --- Errors ---
    if errored:
        lines.append("\n" + "-" * 72)
        lines.append("ERRORS")
        lines.append("-" * 72)
        for r in errored:
            lines.append(f"  {r['path']}")
            lines.append(f"    {r['error']}")

    lines.append("\n" + "=" * 72)
    action = "applied" if write else "would be applied (use --write to apply)"
    lines.append(f"  {len(changed)} normalization(s) {action}.")
    lines.append("=" * 72 + "\n")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Normalize XO_OX macro labels in .xometa preset files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "directory",
        type=Path,
        help="Root directory to scan recursively for .xometa files.",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--dry-run",
        action="store_true",
        default=True,
        help="Report normalizations without modifying files (default).",
    )
    mode.add_argument(
        "--write",
        action="store_true",
        default=False,
        help="Apply normalizations in-place.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        metavar="FILE",
        help="Write report to FILE instead of (or in addition to) stdout.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    root = args.directory
    if not root.exists():
        print(f"Error: directory not found: {root}", file=sys.stderr)
        return 1
    if not root.is_dir():
        print(f"Error: not a directory: {root}", file=sys.stderr)
        return 1

    xometa_files = sorted(root.rglob("*.xometa"))
    if not xometa_files:
        print(f"No .xometa files found under: {root}", file=sys.stderr)
        return 0

    write_mode = args.write  # --write overrides --dry-run
    results = [process_file(p, write=write_mode) for p in xometa_files]

    report = build_report(results, write=write_mode)

    print(report)

    if args.output:
        try:
            args.output.write_text(report, encoding="utf-8")
            print(f"Report written to: {args.output}")
        except Exception as exc:
            print(f"Could not write report file: {exc}", file=sys.stderr)
            return 1

    errored = [r for r in results if r["error"]]
    return 1 if errored else 0


if __name__ == "__main__":
    sys.exit(main())
