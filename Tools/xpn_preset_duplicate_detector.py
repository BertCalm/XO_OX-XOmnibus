#!/usr/bin/env python3
"""
XOceanus Preset Duplicate Detector — Catch copy-paste presets across the library.

Detects:
  1. Exact name duplicates (case-insensitive) within the same engine's files — ERROR
  2. Near-duplicate names (differ only in numbers/suffixes like v1/v2, A/B, alt) — WARNING
  3. DNA near-duplicates (Euclidean distance < threshold, same primary engine, same mood) — WARNING
  4. Parameter near-duplicates (>90% param overlap within ±0.01) — ERROR

Usage:
    python xpn_preset_duplicate_detector.py <presets_dir> [options]

Options:
    --engine FILTER         Filter to a specific engine (e.g. Opal)
    --mood FILTER           Filter to a specific mood (e.g. Foundation)
    --dna-threshold FLOAT   Euclidean DNA distance threshold (default: 0.1)
    --param-threshold FLOAT Minimum fraction of matching params to flag (default: 0.9)
    --format text|json      Output format (default: text)
    --strict                Exit with non-zero code if any warnings exist
"""

import json
import glob
import argparse
import math
import re
import sys
from collections import defaultdict
from pathlib import Path


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def dna_distance(dna_a: dict, dna_b: dict) -> float:
    """Euclidean distance between two 6D DNA vectors."""
    keys = ("brightness", "warmth", "movement", "density", "space", "aggression")
    return math.sqrt(sum((dna_a.get(k, 0) - dna_b.get(k, 0)) ** 2 for k in keys))


def normalize_name(name: str) -> str:
    """Lowercase and strip whitespace."""
    return name.strip().lower()


# Suffixes/patterns that indicate a variant rather than a unique preset
_VARIANT_RE = re.compile(
    r'[\s_\-]*(v\d+|\d+|[ab]|alt\d*|mk\d+|edit|remix|ver\s*\d*)$',
    re.IGNORECASE
)

def strip_variant(name: str) -> str:
    """Remove trailing variant tokens to get a canonical base name."""
    return _VARIANT_RE.sub('', name.strip().lower()).strip()


def param_overlap(params_a: dict, params_b: dict, tol: float = 0.01) -> float:
    """
    Fraction of shared parameter keys whose values agree within ±tol.
    Returns 0.0 if no shared keys.
    """
    shared_keys = set(params_a) & set(params_b)
    if not shared_keys:
        return 0.0
    matching = sum(
        1 for k in shared_keys
        if isinstance(params_a[k], (int, float)) and isinstance(params_b[k], (int, float))
        and abs(params_a[k] - params_b[k]) <= tol
    )
    return matching / len(shared_keys)


def flat_params(preset: dict) -> dict:
    """Flatten nested parameters dict into a single key→value mapping."""
    flat = {}
    raw = preset.get("parameters") or {}
    for engine_block in raw.values():
        if isinstance(engine_block, dict):
            flat.update(engine_block)
    return flat


def primary_engine(preset: dict) -> str:
    engines = preset.get("engines") or []
    return engines[0] if engines else ""


# ---------------------------------------------------------------------------
# Load presets
# ---------------------------------------------------------------------------

def load_presets(presets_dir: Path, engine_filter, mood_filter) -> list:
    pattern = str(presets_dir / "**" / "*.xometa")
    records = []
    for path in glob.glob(pattern, recursive=True):
        try:
            with open(path, encoding="utf-8") as fh:
                data = json.load(fh)
        except (json.JSONDecodeError, OSError):
            continue

        p = Path(path)
        # Derive mood from directory structure: Presets/XOceanus/<mood>/...
        parts = p.parts
        mood_from_path = ""
        for i, part in enumerate(parts):
            if part == "XOceanus" and i + 1 < len(parts):
                mood_from_path = parts[i + 1]
                break

        data.setdefault("_path", str(p))
        data.setdefault("_rel", p.name)
        data.setdefault("_mood_dir", mood_from_path)

        eng = primary_engine(data)
        mood = data.get("mood") or mood_from_path

        if engine_filter and eng.lower() != engine_filter.lower():
            continue
        if mood_filter and mood.lower() != mood_filter.lower():
            continue

        records.append(data)

    return records


# ---------------------------------------------------------------------------
# Detection passes
# ---------------------------------------------------------------------------

def detect_exact_name_duplicates(presets: list[dict]) -> list[dict]:
    """
    ERROR: Two presets with the same name (case-insensitive) sharing the same
    primary engine.
    """
    findings = []
    # Group by (normalized_name, primary_engine)
    groups: dict[tuple, list[dict]] = defaultdict(list)
    for p in presets:
        key = (normalize_name(p.get("name", "")), primary_engine(p))
        groups[key].append(p)

    for (name, engine), group in groups.items():
        if len(group) < 2:
            continue
        paths = [g["_path"] for g in group]
        findings.append({
            "type": "exact_name",
            "severity": "ERROR",
            "name": name,
            "engine": engine,
            "paths": paths,
            "message": f'Exact name "{group[0].get("name")}" appears {len(group)} times for engine {engine}',
        })
    return findings


def detect_near_duplicate_names(presets: list[dict]) -> list[dict]:
    """
    WARNING: Preset names that share the same base after stripping variant tokens
    (v1/v2, 01/02, A/B, alt) within the same primary engine.
    """
    findings = []
    groups: dict[tuple, list[dict]] = defaultdict(list)
    for p in presets:
        base = strip_variant(p.get("name", ""))
        if not base:
            continue
        key = (base, primary_engine(p))
        groups[key].append(p)

    for (base, engine), group in groups.items():
        if len(group) < 2:
            continue
        # Skip if all names are literally identical (already caught by exact pass)
        names = [normalize_name(g.get("name", "")) for g in group]
        if len(set(names)) == 1:
            continue
        paths = [g["_path"] for g in group]
        findings.append({
            "type": "near_name",
            "severity": "WARNING",
            "base": base,
            "engine": engine,
            "names": [g.get("name") for g in group],
            "paths": paths,
            "message": f'Near-duplicate names sharing base "{base}" for engine {engine}',
        })
    return findings


def detect_dna_near_duplicates(presets: list[dict], threshold: float) -> list[dict]:
    """
    WARNING: Pairs with DNA distance < threshold, same primary engine, same mood.
    O(n^2) — acceptable for library sizes up to ~5000.
    """
    findings = []
    # Only compare within (engine, mood) buckets
    buckets: dict[tuple, list[dict]] = defaultdict(list)
    for p in presets:
        dna = p.get("dna")
        if not isinstance(dna, dict) or len(dna) < 6:
            continue
        key = (primary_engine(p), p.get("mood") or p.get("_mood_dir", ""))
        buckets[key].append(p)

    seen: set[frozenset] = set()
    for (engine, mood), group in buckets.items():
        for i in range(len(group)):
            for j in range(i + 1, len(group)):
                a, b = group[i], group[j]
                dist = dna_distance(a["dna"], b["dna"])
                if dist < threshold:
                    pair_key = frozenset([a["_path"], b["_path"]])
                    if pair_key in seen:
                        continue
                    seen.add(pair_key)
                    findings.append({
                        "type": "dna_near_dup",
                        "severity": "WARNING",
                        "engine": engine,
                        "mood": mood,
                        "dna_distance": round(dist, 4),
                        "threshold": threshold,
                        "paths": [a["_path"], b["_path"]],
                        "names": [a.get("name"), b.get("name")],
                        "message": (
                            f'DNA near-duplicate: "{a.get("name")}" ↔ "{b.get("name")}" '
                            f'(dist={dist:.4f}, threshold={threshold})'
                        ),
                    })
    return findings


def detect_param_near_duplicates(presets: list[dict], param_threshold: float) -> list[dict]:
    """
    ERROR: Pairs with >param_threshold parameter overlap (same keys, values within ±0.01),
    within the same primary engine.
    """
    findings = []
    buckets: dict[str, list[dict]] = defaultdict(list)
    for p in presets:
        buckets[primary_engine(p)].append(p)

    seen: set[frozenset] = set()
    for engine, group in buckets.items():
        flat_cache = {p["_path"]: flat_params(p) for p in group}
        for i in range(len(group)):
            for j in range(i + 1, len(group)):
                a, b = group[i], group[j]
                pa, pb = flat_cache[a["_path"]], flat_cache[b["_path"]]
                if not pa or not pb:
                    continue
                overlap = param_overlap(pa, pb)
                if overlap >= param_threshold:
                    pair_key = frozenset([a["_path"], b["_path"]])
                    if pair_key in seen:
                        continue
                    seen.add(pair_key)
                    # Also compute DNA distance for context
                    dna_dist = None
                    if isinstance(a.get("dna"), dict) and isinstance(b.get("dna"), dict):
                        dna_dist = round(dna_distance(a["dna"], b["dna"]), 4)
                    findings.append({
                        "type": "param_near_dup",
                        "severity": "ERROR",
                        "engine": engine,
                        "param_overlap": round(overlap, 4),
                        "dna_distance": dna_dist,
                        "paths": [a["_path"], b["_path"]],
                        "names": [a.get("name"), b.get("name")],
                        "message": (
                            f'Parameter duplicate: "{a.get("name")}" ≈ "{b.get("name")}" '
                            f'({overlap*100:.0f}% param overlap'
                            + (f', DNA dist {dna_dist}' if dna_dist is not None else '')
                            + ')'
                        ),
                    })
    return findings


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def relative_path(path: str, base: Path) -> str:
    try:
        return str(Path(path).relative_to(base))
    except ValueError:
        return path


def format_text(all_findings: list, preset_count: int, engine_filter,
                mood_filter, presets_dir: Path) -> str:
    errors = [f for f in all_findings if f["severity"] == "ERROR"]
    warnings = [f for f in all_findings if f["severity"] == "WARNING"]

    scope_parts = []
    if engine_filter:
        scope_parts.append(f"{engine_filter} engine")
    if mood_filter:
        scope_parts.append(f"{mood_filter} mood")
    scope = ", ".join(scope_parts) if scope_parts else "all engines, all moods"

    lines = [f"DUPLICATE AUDIT — {preset_count} presets ({scope})", ""]

    def rel(p):
        return relative_path(p, presets_dir)

    if errors:
        lines.append(f"ERRORS ({len(errors)}):")
        for f in errors:
            if f["type"] == "exact_name":
                lines.append("  Exact names:")
                for p in f["paths"]:
                    lines.append(f"    {rel(p)}")
                lines.append(f"    → {f['message']}")
            elif f["type"] == "param_near_dup":
                lines.append("  Parameter duplicates:")
                for p in f["paths"]:
                    lines.append(f"    {rel(p)}")
                detail = f"    → {f['param_overlap']*100:.0f}% parameter overlap"
                if f.get("dna_distance") is not None:
                    detail += f", DNA distance {f['dna_distance']}"
                lines.append(detail)
            lines.append("")
    else:
        lines.append("ERRORS (0): none")
        lines.append("")

    if warnings:
        lines.append(f"WARNINGS ({len(warnings)}):")
        for f in warnings:
            if f["type"] == "near_name":
                lines.append("  Near-duplicate names:")
                for name, path in zip(f["names"], f["paths"]):
                    lines.append(f"    {rel(path)}  [{name}]")
                lines.append(f"    → different names, same preset?")
            elif f["type"] == "dna_near_dup":
                lines.append("  DNA near-duplicates:")
                for name, path in zip(f["names"], f["paths"]):
                    lines.append(f"    {rel(path)}  [{name}]")
                lines.append(f"    → DNA distance {f['dna_distance']} (threshold: {f['threshold']})")
            lines.append("")
    else:
        lines.append("WARNINGS (0): none")

    lines.append("")
    status = "PASS" if not errors else "FAIL"
    lines.append(f"Result: {status} — {len(errors)} error(s), {len(warnings)} warning(s)")
    return "\n".join(lines)


def format_json(all_findings: list[dict], preset_count: int) -> str:
    errors = [f for f in all_findings if f["severity"] == "ERROR"]
    warnings = [f for f in all_findings if f["severity"] == "WARNING"]
    output = {
        "preset_count": preset_count,
        "error_count": len(errors),
        "warning_count": len(warnings),
        "status": "PASS" if not errors else "FAIL",
        "findings": all_findings,
    }
    return json.dumps(output, indent=2)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Detect duplicate or near-duplicate .xometa presets."
    )
    parser.add_argument("presets_dir", help="Path to presets root (e.g. Presets/XOceanus)")
    parser.add_argument("--engine", default=None, help="Filter to a specific engine name")
    parser.add_argument("--mood", default=None, help="Filter to a specific mood")
    parser.add_argument("--dna-threshold", type=float, default=0.1,
                        help="Euclidean DNA distance threshold for near-duplicate DNA (default: 0.1)")
    parser.add_argument("--param-threshold", type=float, default=0.9,
                        help="Min param overlap fraction to flag as duplicate (default: 0.9)")
    parser.add_argument("--format", choices=["text", "json"], default="text",
                        help="Output format (default: text)")
    parser.add_argument("--strict", action="store_true",
                        help="Exit non-zero if any warnings exist (in addition to errors)")
    args = parser.parse_args()

    presets_dir = Path(args.presets_dir).resolve()
    if not presets_dir.exists():
        print(f"ERROR: presets_dir not found: {presets_dir}", file=sys.stderr)
        sys.exit(2)

    presets = load_presets(presets_dir, args.engine, args.mood)

    findings: list[dict] = []
    findings += detect_exact_name_duplicates(presets)
    findings += detect_near_duplicate_names(presets)
    findings += detect_dna_near_duplicates(presets, args.dna_threshold)
    findings += detect_param_near_duplicates(presets, args.param_threshold)

    # Sort: errors first, then warnings; within each group by type then path
    findings.sort(key=lambda f: (0 if f["severity"] == "ERROR" else 1, f["type"], f.get("paths", [""])[0]))

    if args.format == "json":
        print(format_json(findings, len(presets)))
    else:
        print(format_text(findings, len(presets), args.engine, args.mood, presets_dir))

    errors = [f for f in findings if f["severity"] == "ERROR"]
    warnings = [f for f in findings if f["severity"] == "WARNING"]

    if errors:
        sys.exit(1)
    if args.strict and warnings:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
