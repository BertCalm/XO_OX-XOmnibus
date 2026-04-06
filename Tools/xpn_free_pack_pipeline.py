#!/usr/bin/env python3
"""
xpn_free_pack_pipeline.py — XO_OX Free Pack Build Pipeline

Complete automated pipeline for building a free/entry-level XO_OX expansion
pack (e.g. TIDE TABLES) from a preset directory. Single-command orchestration:

  1. Load     — scan preset dir for .xometa files matching engine filter
  2. Filter   — select best N presets: DNA diversity + mood balance + quality
  3. Build    — generate expansion.json from pack spec
  4. Validate — run 10 inline QA checks (no external imports)
  5. Manifest — create bundle_manifest.json
  6. Write    — copy selected files + manifests to output dir
  7. Report   — mood distribution, DNA centroid, validation status

Usage:
    python xpn_free_pack_pipeline.py \\
        --preset-dir Source/Presets \\
        --pack-name "TIDE TABLES" \\
        --engine ONSET \\
        [--count 30] \\
        [--output-dir build/tide_tables] \\
        [--dry-run]

Exit codes: 0 = success, 1 = validation warnings, 2 = validation failures
"""

import argparse
import json
import math
import shutil
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

MANUFACTURER = "XO_OX Designs"
OXPORT_VERSION = "2.0"
SCHEMA_VERSION = 1

MOODS = ["Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family", "Submerged", "Coupling", "Crystalline", "Deep", "Ethereal", "Kinetic", "Luminous", "Organic", "Shadow"]
DNA_AXES = ["brightness", "warmth", "movement", "density", "space", "aggression"]

GENERIC_NAMES = {
    "untitled", "default", "new program", "new kit", "program",
    "kit", "sample", "unnamed", "test", "temp", "tmp", "init",
}

REQUIRED_EXPANSION_FIELDS = ("name", "version", "description", "engines")

# ---------------------------------------------------------------------------
# Step 1: Load presets
# ---------------------------------------------------------------------------

def load_presets(preset_dir: Path, engine_filter: Optional[str]) -> List[Dict[str, Any]]:
    """Scan preset_dir recursively for .xometa files. Optionally filter by engine name."""
    presets = []
    for path in sorted(preset_dir.rglob("*.xometa")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  [WARN] Could not parse {path.name}: {exc}", file=sys.stderr)
            continue
        data["_path"] = path
        if engine_filter:
            engines = [e.upper() for e in data.get("engines", [])]
            if engine_filter.upper() not in engines:
                continue
        presets.append(data)
    return presets


# ---------------------------------------------------------------------------
# Step 2: Filter / select presets
# ---------------------------------------------------------------------------

def _dna_vector(preset: Dict[str, Any]) -> List[float]:
    dna = preset.get("dna") or {}
    return [float(dna.get(axis, 0.5)) for axis in DNA_AXES]


def _dna_distance(a: List[float], b: List[float]) -> float:
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def _quality_score(preset: Dict[str, Any]) -> float:
    """Score 0–1. Favours 2–3 word names + complete DNA + tags."""
    score = 0.0
    name = preset.get("name", "")
    words = len(name.split())
    if 2 <= words <= 3:
        score += 0.4
    elif words == 1:
        score += 0.1
    else:
        score += 0.25

    dna = preset.get("dna") or {}
    completeness = sum(1 for ax in DNA_AXES if ax in dna) / len(DNA_AXES)
    score += 0.4 * completeness

    tags = preset.get("tags") or []
    score += 0.2 * min(1.0, len(tags) / 4)

    return score


def _select_presets(
    presets: List[Dict[str, Any]],
    count: int,
) -> List[Dict[str, Any]]:
    """
    Greedy farthest-point selection for DNA diversity, with mood balance and
    quality score as tiebreakers.

    Algorithm:
      1. Sort candidates by quality descending.
      2. Seed with the highest-quality preset.
      3. Iteratively add the preset whose DNA vector is farthest from all
         already-selected vectors.
      4. After diversity pass: backfill missing moods from quality-sorted
         remaining candidates up to count.
    """
    if not presets:
        return []

    # Sort by quality descending
    candidates = sorted(presets, key=_quality_score, reverse=True)
    n = min(count, len(candidates))

    selected: List[Dict[str, Any]] = [candidates[0]]
    remaining = candidates[1:]
    vecs = [_dna_vector(candidates[0])]

    # Greedy farthest-point
    while len(selected) < n and remaining:
        best_idx = 0
        best_dist = -1.0
        for i, p in enumerate(remaining):
            v = _dna_vector(p)
            min_dist = min(_dna_distance(v, sv) for sv in vecs)
            if min_dist > best_dist:
                best_dist = min_dist
                best_idx = i
        chosen = remaining.pop(best_idx)
        selected.append(chosen)
        vecs.append(_dna_vector(chosen))

    # Mood backfill: ensure at least 1 per mood if possible
    selected_moods = {p.get("mood") for p in selected}
    for mood in MOODS:
        if len(selected) >= n:
            break
        if mood not in selected_moods:
            for p in remaining:
                if p.get("mood") == mood:
                    selected.append(p)
                    remaining.remove(p)
                    selected_moods.add(mood)
                    break

    return selected[:n]


# ---------------------------------------------------------------------------
# Step 3: Build expansion.json
# ---------------------------------------------------------------------------

def build_expansion_json(
    pack_name: str,
    version: str,
    description: str,
    engines: List[str],
    tags: List[str],
) -> Dict[str, Any]:
    now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    return {
        "name": pack_name,
        "version": version,
        "description": description,
        "author": MANUFACTURER,
        "created": now,
        "format_version": OXPORT_VERSION,
        "schema_version": SCHEMA_VERSION,
        "engines": engines,
        "tags": tags,
        "type": "free",
        "tier": "entry",
    }


# ---------------------------------------------------------------------------
# Step 4: Inline QA validation (10 checks)
# ---------------------------------------------------------------------------

def _check(check_id: int, name: str, passed: bool, msg_pass: str, msg_fail: str,
           warn: bool = False) -> Dict[str, Any]:
    if passed:
        return {"id": check_id, "name": name, "status": "PASS", "message": msg_pass}
    status = "WARN" if warn else "FAIL"
    return {"id": check_id, "name": name, "status": status, "message": msg_fail}


def validate_pack(
    selected: List[Dict[str, Any]],
    expansion: Dict[str, Any],
    output_dir: Path,
    count_requested: int,
) -> Tuple[List[Dict[str, Any]], int]:
    """
    Run 10 QA checks. Returns (results_list, exit_code).
    exit_code: 0=PASS, 1=WARN, 2=FAIL
    """
    results = []

    # 1. expansion.json has required fields
    missing = [f for f in REQUIRED_EXPANSION_FIELDS if not expansion.get(f)]
    results.append(_check(1, "expansion.json required fields",
                          not missing,
                          "All required fields present",
                          f"Missing fields: {missing}"))

    # 2. Pack name is non-generic
    name_lower = expansion.get("name", "").lower().strip()
    results.append(_check(2, "Pack name not generic",
                          name_lower not in GENERIC_NAMES and bool(name_lower),
                          f"Pack name '{expansion.get('name')}' is valid",
                          f"Pack name '{expansion.get('name')}' is generic or empty"))

    # 3. Preset count meets requested
    results.append(_check(3, "Preset count",
                          len(selected) >= count_requested,
                          f"{len(selected)} presets selected (>= {count_requested})",
                          f"Only {len(selected)} presets found (requested {count_requested})",
                          warn=True))

    # 4. All selected presets have valid JSON (already guaranteed by load, check schema_version)
    bad_schema = [p.get("name", "?") for p in selected
                  if p.get("schema_version") != SCHEMA_VERSION]
    results.append(_check(4, "Preset schema versions",
                          not bad_schema,
                          "All presets have correct schema_version",
                          f"Wrong schema_version: {bad_schema}",
                          warn=True))

    # 5. No duplicate preset names
    names = [p.get("name", "") for p in selected]
    dupes = [n for n in names if names.count(n) > 1]
    results.append(_check(5, "No duplicate preset names",
                          not dupes,
                          "All preset names are unique",
                          f"Duplicate names: {list(set(dupes))}"))

    # 6. No generic preset names
    bad_names = [p.get("name", "") for p in selected
                 if p.get("name", "").lower().strip() in GENERIC_NAMES]
    results.append(_check(6, "No generic preset names",
                          not bad_names,
                          "No generic preset names detected",
                          f"Generic preset names found: {bad_names}"))

    # 7. All presets have DNA vectors
    missing_dna = [p.get("name", "?") for p in selected
                   if not p.get("dna") or not all(ax in p["dna"] for ax in DNA_AXES)]
    results.append(_check(7, "Complete DNA vectors",
                          not missing_dna,
                          "All presets have complete DNA vectors",
                          f"Incomplete DNA in: {missing_dna}",
                          warn=True))

    # 8. Mood coverage (at least 3 of 7 moods represented)
    moods_present = {p.get("mood") for p in selected if p.get("mood")}
    results.append(_check(8, "Mood coverage",
                          len(moods_present) >= 3,
                          f"{len(moods_present)} moods represented: {sorted(moods_present)}",
                          f"Only {len(moods_present)} moods covered — aim for 3+",
                          warn=True))

    # 9. Engine list non-empty in expansion.json
    engines = expansion.get("engines", [])
    results.append(_check(9, "Engines list non-empty",
                          bool(engines),
                          f"Engines declared: {engines}",
                          "No engines declared in expansion.json"))

    # 10. All presets have at least one tag
    no_tags = [p.get("name", "?") for p in selected
               if not p.get("tags")]
    results.append(_check(10, "Presets have tags",
                          not no_tags,
                          "All presets have at least one tag",
                          f"Presets with no tags: {no_tags}",
                          warn=True))

    # Compute exit code
    statuses = {r["status"] for r in results}
    if "FAIL" in statuses:
        code = 2
    elif "WARN" in statuses:
        code = 1
    else:
        code = 0

    return results, code


# ---------------------------------------------------------------------------
# Step 5: Generate bundle_manifest.json
# ---------------------------------------------------------------------------

def build_bundle_manifest(
    pack_name: str,
    version: str,
    selected: List[Dict[str, Any]],
    engines: List[str],
) -> Dict[str, Any]:
    now = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    preset_entries = []
    for p in selected:
        preset_entries.append({
            "name": p.get("name", ""),
            "mood": p.get("mood", ""),
            "engines": p.get("engines", []),
            "file": Path(p["_path"]).name,
        })
    mood_dist: Dict[str, int] = {}
    for p in selected:
        mood = p.get("mood", "Unknown")
        mood_dist[mood] = mood_dist.get(mood, 0) + 1

    return {
        "pack_name": pack_name,
        "version": version,
        "generated": now,
        "manufacturer": MANUFACTURER,
        "preset_count": len(selected),
        "engines": engines,
        "mood_distribution": mood_dist,
        "presets": preset_entries,
    }


# ---------------------------------------------------------------------------
# Step 6: Write output
# ---------------------------------------------------------------------------

def write_output(
    selected: List[Dict[str, Any]],
    expansion: Dict[str, Any],
    manifest: Dict[str, Any],
    output_dir: Path,
    dry_run: bool,
) -> None:
    if not dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    for p in selected:
        src = Path(p["_path"])
        dst = output_dir / src.name
        if dry_run:
            print(f"  [DRY] copy {src} -> {dst}")
        else:
            shutil.copy2(src, dst)

    exp_path = output_dir / "expansion.json"
    manifest_path = output_dir / "bundle_manifest.json"

    if dry_run:
        print(f"  [DRY] write {exp_path}")
        print(f"  [DRY] write {manifest_path}")
    else:
        exp_path.write_text(json.dumps(expansion, indent=2), encoding="utf-8")
        manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")


# ---------------------------------------------------------------------------
# Step 7: Summary report
# ---------------------------------------------------------------------------

def _dna_centroid(selected: List[Dict[str, Any]]) -> Dict[str, float]:
    if not selected:
        return {ax: 0.5 for ax in DNA_AXES}
    centroid = {}
    for ax in DNA_AXES:
        vals = [float(p.get("dna", {}).get(ax, 0.5)) for p in selected]
        centroid[ax] = round(sum(vals) / len(vals), 3)
    return centroid


def print_summary(
    pack_name: str,
    selected: List[Dict[str, Any]],
    validation_results: List[Dict[str, Any]],
    exit_code: int,
    dry_run: bool,
    output_dir: Path,
) -> None:
    sep = "-" * 60
    print(f"\n{'=' * 60}")
    print(f"  {pack_name} — Free Pack Build Report")
    print(f"{'=' * 60}")

    if dry_run:
        print("  [DRY RUN — no files written]")

    print(f"\n  Presets selected : {len(selected)}")

    # Mood distribution
    mood_dist: Dict[str, int] = {}
    for p in selected:
        m = p.get("mood", "Unknown")
        mood_dist[m] = mood_dist.get(m, 0) + 1
    print("\n  Mood distribution:")
    for mood in MOODS:
        count = mood_dist.get(mood, 0)
        bar = "#" * count
        print(f"    {mood:<12} {bar} ({count})")
    for mood, count in mood_dist.items():
        if mood not in MOODS:
            print(f"    {mood:<12} {'#' * count} ({count})")

    # DNA centroid
    centroid = _dna_centroid(selected)
    print("\n  DNA centroid:")
    for ax, val in centroid.items():
        filled = int(val * 20)
        bar = "=" * filled + "." * (20 - filled)
        print(f"    {ax:<12} [{bar}] {val:.3f}")

    # Validation
    print(f"\n  Validation ({len(validation_results)} checks):")
    statuses = {"PASS": 0, "WARN": 0, "FAIL": 0}
    for r in validation_results:
        s = r["status"]
        statuses[s] = statuses.get(s, 0) + 1
        icon = "OK" if s == "PASS" else ("!!" if s == "FAIL" else " W")
        print(f"    [{icon}] #{r['id']:02d} {r['name']}: {r['message']}")

    overall = "PASS" if exit_code == 0 else ("WARN" if exit_code == 1 else "FAIL")
    print(f"\n  Overall: {overall} "
          f"({statuses['PASS']} pass, {statuses['WARN']} warn, {statuses['FAIL']} fail)")

    if not dry_run:
        print(f"\n  Output directory : {output_dir}")

    print(f"{'=' * 60}\n")


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="XO_OX Free Pack Pipeline — build a complete expansion pack in one command.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--preset-dir", required=True, type=Path,
                        help="Directory to scan for .xometa files")
    parser.add_argument("--pack-name", required=True,
                        help="Pack display name, e.g. 'TIDE TABLES'")
    parser.add_argument("--engine", required=True,
                        help="Engine filter (uppercase short name), e.g. ONSET")
    parser.add_argument("--count", type=int, default=30,
                        help="Number of presets to select (default: 30)")
    parser.add_argument("--output-dir", type=Path, default=None,
                        help="Output directory (default: build/<pack-name-slug>)")
    parser.add_argument("--version", default="1.0.0",
                        help="Pack version string (default: 1.0.0)")
    parser.add_argument("--description", default=None,
                        help="Pack description (auto-generated if omitted)")
    parser.add_argument("--tags", default="",
                        help="Comma-separated extra tags")
    parser.add_argument("--dry-run", action="store_true",
                        help="Preview without writing any files")
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    preset_dir = args.preset_dir.expanduser().resolve()
    if not preset_dir.is_dir():
        print(f"ERROR: preset-dir does not exist: {preset_dir}", file=sys.stderr)
        return 2

    slug = args.pack_name.lower().replace(" ", "_")
    output_dir = args.output_dir or Path(f"build/{slug}")
    output_dir = output_dir.expanduser().resolve()

    description = args.description or (
        f"{args.pack_name} — a free XO_OX expansion pack featuring {args.engine} "
        f"engine presets. The gateway to the XO_OX ecosystem."
    )
    tags = [t.strip() for t in args.tags.split(",") if t.strip()]
    tags += ["free", "entry", args.engine.lower()]

    print(f"\nXO_OX Free Pack Pipeline: {args.pack_name}")
    print(f"  Preset dir : {preset_dir}")
    print(f"  Engine     : {args.engine}")
    print(f"  Count      : {args.count}")
    print(f"  Output     : {output_dir}")

    # Step 1: Load
    print("\n[1/7] Loading presets...")
    all_presets = load_presets(preset_dir, args.engine)
    print(f"      Found {len(all_presets)} matching presets")
    if not all_presets:
        print("ERROR: No presets found. Check --preset-dir and --engine.", file=sys.stderr)
        return 2

    # Step 2: Filter / select
    print(f"\n[2/7] Selecting {args.count} presets (DNA diversity + mood balance + quality)...")
    selected = _select_presets(all_presets, args.count)
    print(f"      Selected {len(selected)} presets")

    # Step 3: Build expansion.json
    print("\n[3/7] Building expansion.json...")
    engines = sorted({e for p in selected for e in p.get("engines", [])})
    if args.engine not in engines:
        engines.insert(0, args.engine)
    expansion = build_expansion_json(args.pack_name, args.version, description, engines, tags)
    print(f"      Engines: {engines}")

    # Step 4: Validate
    print("\n[4/7] Running QA validation (10 checks)...")
    validation_results, exit_code = validate_pack(selected, expansion, output_dir, args.count)
    pass_count = sum(1 for r in validation_results if r["status"] == "PASS")
    print(f"      {pass_count}/10 checks passed")

    # Step 5: Manifest
    print("\n[5/7] Generating bundle_manifest.json...")
    manifest = build_bundle_manifest(args.pack_name, args.version, selected, engines)

    # Step 6: Write
    print(f"\n[6/7] Writing output {'(DRY RUN)' if args.dry_run else ''}...")
    write_output(selected, expansion, manifest, output_dir, args.dry_run)
    if not args.dry_run:
        print(f"      Written to {output_dir}")

    # Step 7: Summary
    print("\n[7/7] Summary report:")
    print_summary(args.pack_name, selected, validation_results, exit_code, args.dry_run, output_dir)

    return exit_code


if __name__ == "__main__":
    sys.exit(main())
