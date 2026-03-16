#!/usr/bin/env python3
"""
XPN Preset DNA Recalibrator

Audits .xometa files for discrepancies between declared sonic_dna values and
values estimated from the preset's own parameter block. Flags large gaps and
optionally writes recalibrated DNA values back to disk.

Usage:
    python xpn_preset_dna_recalibrator.py <presets_dir> [--engine FILTER]
        [--dry-run] [--apply] [--threshold 0.3] [--format text|json]

Modes:
    --dry-run   Show discrepancies without writing (default)
    --apply     Write recalibrated DNA to .xometa files (creates .bak backups)

Options:
    --engine    Case-insensitive substring filter on path (e.g. "OPAL", "ORACLE")
    --threshold Minimum delta to flag as a discrepancy (default: 0.3)
    --format    Output format: text (default) or json
"""

import argparse
import glob
import json
import shutil
import sys
from datetime import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
# DNA dimension keyword maps
# Keys in 'params' are matched as lowercase substrings against parameter names
# ---------------------------------------------------------------------------

DIMENSION_KEYWORDS = {
    "brightness": ["cutoff", "brightness", "tone", "high", "treble", "presence", "shimmer"],
    "warmth":     ["warmth", "saturation", "drive", "warm", "tape", "vintage", "satdrive", "harmonic"],
    "movement":   ["lfo", "vibrato", "flutter", "rate", "speed", "moddepth", "mod_depth", "modrate",
                   "tremble", "wobble", "motion", "animate"],
    "density":    ["voices", "unison", "density", "poly", "grain", "spread", "detune", "chorus", "width"],
    "space":      ["reverb", "delay", "room", "space", "hall", "ambience", "wet", "blend", "mix", "echo"],
    "aggression": ["drive", "distortion", "crush", "bit", "aggression", "attack", "bite", "grit",
                   "overdrive", "clip", "fuzz", "crunch"],
}

DIMS = list(DIMENSION_KEYWORDS.keys())


def clamp(v: float, lo: float = 0.0, hi: float = 1.0) -> float:
    return max(lo, min(hi, float(v)))


def is_numeric(v) -> bool:
    return isinstance(v, (int, float)) and not isinstance(v, bool)


def estimate_dna_from_params(params: dict) -> dict:
    """
    Estimate 6D DNA from a flat {param_name: value} dict.
    Only considers numeric values in [0.0, 1.0].
    Returns {dim: value_or_None} — None means no matching params found.
    """
    result = {}
    for dim, keywords in DIMENSION_KEYWORDS.items():
        matched = []
        for key, val in params.items():
            if not is_numeric(val):
                continue
            norm_val = float(val)
            # Only treat values that look like 0–1 floats
            if norm_val < 0.0 or norm_val > 1.0:
                continue
            key_lower = key.lower()
            if any(kw in key_lower for kw in keywords):
                matched.append(norm_val)
        result[dim] = (sum(matched) / len(matched)) if matched else None
    return result


def get_dna_block(data: dict):
    """Return the sonic_dna/dna block from a parsed .xometa dict."""
    return data.get("sonic_dna") or data.get("dna")


def get_params_block(data: dict) -> dict:
    """Return the parameters block from a parsed .xometa dict (flat or nested)."""
    raw = data.get("parameters") or data.get("params") or {}
    # Flatten one level: {"engine": {"param": val}} → {"param": val}
    flat = {}
    for k, v in raw.items():
        if isinstance(v, dict):
            flat.update(v)
        else:
            flat[k] = v
    return flat


def load_xometa(path: Path):
    try:
        with open(path, "r", encoding="utf-8") as fh:
            return json.load(fh)
    except (json.JSONDecodeError, IOError) as e:
        print(f"  WARN: could not parse {path}: {e}", file=sys.stderr)
        return None


def find_discrepancies(declared: dict, estimated: dict, threshold: float) -> list:
    """
    Compare declared and estimated DNA.
    Returns list of {dim, declared, estimated, delta} for gaps >= threshold.
    """
    issues = []
    for dim in DIMS:
        est = estimated.get(dim)
        if est is None:
            continue  # no matching params → keep declared value, no flag
        dec = float(declared.get(dim, 0.5))
        delta = abs(dec - est)
        if delta >= threshold:
            issues.append({
                "dim": dim,
                "declared": dec,
                "estimated": est,
                "delta": delta,
            })
    return issues


def recalibrate_dna(declared: dict, estimated: dict) -> dict:
    """
    Build a new DNA dict: replace declared values only where we have an estimate.
    """
    updated = dict(declared)
    for dim in DIMS:
        est = estimated.get(dim)
        if est is not None:
            updated[dim] = round(clamp(est), 4)
    return updated


# ---------------------------------------------------------------------------
# Main scan
# ---------------------------------------------------------------------------

def scan(presets_dir: Path, engine_filter: str, threshold: float):
    """
    Scan all .xometa files under presets_dir.
    Returns list of result dicts for files with discrepancies.
    """
    pattern = str(presets_dir / "**" / "*.xometa")
    files = sorted(glob.glob(pattern, recursive=True))

    if engine_filter:
        ef_lower = engine_filter.lower()
        files = [f for f in files if ef_lower in f.lower()]

    total = len(files)
    results = []

    for fpath in files:
        path = Path(fpath)
        data = load_xometa(path)
        if data is None:
            continue

        declared = get_dna_block(data)
        if declared is None:
            continue  # no DNA block — handled by other tools

        params = get_params_block(data)
        if not params:
            continue  # no parameters to analyze

        estimated = estimate_dna_from_params(params)
        issues = find_discrepancies(declared, estimated, threshold)

        if issues:
            rel = str(path.relative_to(presets_dir))
            results.append({
                "path": fpath,
                "rel_path": rel,
                "issues": issues,
                "declared": {d: float(declared.get(d, 0.5)) for d in DIMS},
                "estimated": estimated,
            })

    return total, results


# ---------------------------------------------------------------------------
# Apply mode
# ---------------------------------------------------------------------------

def apply_recalibration(results: list) -> tuple:
    """
    Write recalibrated DNA back to .xometa files.
    Creates .bak backup before overwriting.
    Returns (applied_count, error_count).
    """
    applied = 0
    errors = 0
    for entry in results:
        path = Path(entry["path"])
        data = load_xometa(path)
        if data is None:
            errors += 1
            continue
        declared = get_dna_block(data)
        if declared is None:
            errors += 1
            continue

        new_dna = recalibrate_dna(declared, entry["estimated"])

        # Backup
        bak = path.with_suffix(".xometa.bak")
        try:
            shutil.copy2(path, bak)
        except IOError as e:
            print(f"  ERROR: backup failed for {path}: {e}", file=sys.stderr)
            errors += 1
            continue

        # Write updated DNA
        if "sonic_dna" in data:
            data["sonic_dna"] = new_dna
        elif "dna" in data:
            data["dna"] = new_dna

        try:
            with open(path, "w", encoding="utf-8") as fh:
                json.dump(data, fh, indent=2, ensure_ascii=False)
                fh.write("\n")
            applied += 1
        except IOError as e:
            print(f"  ERROR: write failed for {path}: {e}", file=sys.stderr)
            errors += 1

    return applied, errors


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def format_text(total: int, results: list[dict], threshold: float, engine_filter: str) -> str:
    lines = []
    lines.append(f"DNA RECALIBRATION AUDIT — {total} presets scanned")
    if engine_filter:
        lines.append(f"Engine filter: {engine_filter}")
    lines.append(f"Threshold: ≥ {threshold}")
    lines.append("")

    if not results:
        lines.append("No discrepancies found. All declared DNA values are within threshold.")
        return "\n".join(lines)

    lines.append(f"Discrepancies found (threshold ≥ {threshold}): {len(results)} presets")
    lines.append("")

    for entry in results:
        lines.append(f"  {entry['rel_path']}")
        for issue in sorted(entry["issues"], key=lambda x: -x["delta"]):
            dim = issue["dim"]
            dec = issue["declared"]
            est = issue["estimated"]
            delta = issue["delta"]
            lines.append(
                f"    {dim:<12} declared={dec:.2f}  estimated={est:.2f}  Δ={delta:.2f} ⚠"
            )
        lines.append("")

    return "\n".join(lines)


def format_json(total: int, results: list[dict], threshold: float, engine_filter: str) -> str:
    out = {
        "generated": datetime.utcnow().isoformat() + "Z",
        "total_scanned": total,
        "engine_filter": engine_filter or None,
        "threshold": threshold,
        "discrepancy_count": len(results),
        "discrepancies": [
            {
                "path": e["rel_path"],
                "issues": [
                    {
                        "dimension": i["dim"],
                        "declared": round(i["declared"], 4),
                        "estimated": round(i["estimated"], 4),
                        "delta": round(i["delta"], 4),
                    }
                    for i in sorted(e["issues"], key=lambda x: -x["delta"])
                ],
            }
            for e in results
        ],
    }
    return json.dumps(out, indent=2)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        description="Audit and recalibrate 6D Sonic DNA in .xometa preset files.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("presets_dir", type=Path, help="Root directory containing .xometa files")
    p.add_argument("--engine", default="", metavar="FILTER",
                   help="Case-insensitive substring filter on file paths (e.g. OPAL)")
    p.add_argument("--dry-run", action="store_true", default=True,
                   help="Show discrepancies without writing (default)")
    p.add_argument("--apply", action="store_true",
                   help="Write recalibrated DNA to files (creates .bak backups)")
    p.add_argument("--threshold", type=float, default=0.3,
                   help="Flag discrepancies larger than this (default: 0.3)")
    p.add_argument("--format", choices=["text", "json"], default="text",
                   help="Output format (default: text)")
    return p


def main():
    parser = build_parser()
    args = parser.parse_args()

    presets_dir = args.presets_dir.resolve()
    if not presets_dir.is_dir():
        parser.error(f"presets_dir does not exist or is not a directory: {presets_dir}")

    if args.threshold <= 0.0 or args.threshold > 1.0:
        parser.error("--threshold must be in (0.0, 1.0]")

    total, results = scan(presets_dir, args.engine, args.threshold)

    # Output report
    if args.format == "json":
        print(format_json(total, results, args.threshold, args.engine))
    else:
        print(format_text(total, results, args.threshold, args.engine))

    # Apply recalibration if requested
    if args.apply:
        if not results:
            print("Nothing to apply.")
        else:
            print(f"\nApplying recalibration to {len(results)} files...")
            applied, errors = apply_recalibration(results)
            print(f"  Written: {applied}  Errors: {errors}")
            if errors:
                sys.exit(1)


if __name__ == "__main__":
    main()
