#!/usr/bin/env python3
"""
xpn_xometa_completeness_auditor.py

Deep completeness audit of .xometa preset files.
Checks 12 quality dimensions per file and produces a per-file health score.

Usage:
    python Tools/xpn_xometa_completeness_auditor.py --presets-dir Presets/
    python Tools/xpn_xometa_completeness_auditor.py --presets-dir Presets/ --min-score 9
    python Tools/xpn_xometa_completeness_auditor.py --presets-dir Presets/ --fix-names
"""

import argparse
import json
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

# ── Constants ────────────────────────────────────────────────────────────────

VALID_MOODS = {"Foundation", "Atmosphere", "Entangled", "Prism", "Flux", "Aether", "Family"}

SONIC_DNA_DIMENSIONS = {"brightness", "warmth", "density", "motion", "space", "character"}

REQUIRED_MACROS = {"CHARACTER", "MOVEMENT", "COUPLING", "SPACE"}

CHECK_NAMES = [
    "name_valid",           # 1
    "engine_present",       # 2
    "mood_valid",           # 3
    "description_length",   # 4
    "tags_present",         # 5
    "sonic_dna_complete",   # 6
    "macros_present",       # 7
    "macro_descriptions",   # 8
    "parameters_nonempty",  # 9
    "entangled_engines",    # 10
    "name_format",          # 11
    "name_unique",          # 12
]

CHECK_LABELS = {
    "name_valid":          "1. Name non-empty, ≤30 chars",
    "engine_present":      "2. Engine/engines field present",
    "mood_valid":          "3. Mood is one of 7 valid moods",
    "description_length":  "4. Description ≥10 chars",
    "tags_present":        "5. Tags list ≥1 entry",
    "sonic_dna_complete":  "6. Sonic DNA all 6 dims (0.0–1.0)",
    "macros_present":      "7. Macros has CHARACTER/MOVEMENT/COUPLING/SPACE",
    "macro_descriptions":  "8. Macro descriptions ≥5 chars each",
    "parameters_nonempty": "9. Parameters dict non-empty",
    "entangled_engines":   "10. Entangled mood → ≥2 engines",
    "name_format":         "11. Name is Title Case (no snake_case/ALL CAPS)",
    "name_unique":         "12. Name unique within mood folder",
}

# ── Helpers ───────────────────────────────────────────────────────────────────

def is_title_case_valid(name: str) -> bool:
    """
    Passes if:
    - Not snake_case (no underscores)
    - Not ALL CAPS (more than 1 word all uppercase)
    - Reasonably title-cased: each word either starts with capital or is a short
      connector word (of, the, a, an, in, on, at, for, with, etc.)
    """
    if "_" in name:
        return False

    words = name.split()
    if not words:
        return False

    # Detect ALL CAPS: all alpha chars are uppercase and there are >1 words
    alpha_chars = [c for c in name if c.isalpha()]
    if alpha_chars and all(c.isupper() for c in alpha_chars) and len(words) > 1:
        return False

    # Connectors that are allowed lowercase
    connectors = {"of", "the", "a", "an", "in", "on", "at", "for", "with",
                  "and", "or", "but", "nor", "to", "from", "by", "into",
                  "via", "as", "up", "is", "it"}

    for i, word in enumerate(words):
        # Strip leading punctuation/numbers to get alpha portion
        alpha = re.sub(r"[^a-zA-Z]", "", word)
        if not alpha:
            continue
        if i == 0:
            # First word must start with capital
            if not alpha[0].isupper():
                return False
        else:
            if alpha.lower() in connectors:
                continue
            if not alpha[0].isupper():
                return False

    return True


def to_title_case(name: str) -> str:
    """Auto-titlecase a name (used by --fix-names)."""
    connectors = {"of", "the", "a", "an", "in", "on", "at", "for", "with",
                  "and", "or", "but", "nor", "to", "from", "by", "into",
                  "via", "as", "up", "is", "it"}
    words = name.replace("_", " ").split()
    result = []
    for i, word in enumerate(words):
        if i == 0 or word.lower() not in connectors:
            result.append(word.capitalize())
        else:
            result.append(word.lower())
    return " ".join(result)


# ── Core audit ────────────────────────────────────────────────────────────────

def audit_file(data: dict, path: Path, name_registry: dict) -> dict:
    """
    Run all 12 checks on a parsed .xometa dict.
    name_registry: dict mapping (mood, name) -> list[Path] — populated in-place.
    Returns a result dict with per-check bools and metadata.
    """
    checks = {}

    # 1. Name non-empty, ≤30 chars
    name = data.get("name", "")
    checks["name_valid"] = bool(name) and len(name) <= 30

    # 2. Engine / engines present
    engine = data.get("engine") or data.get("engines")
    if isinstance(engine, list):
        checks["engine_present"] = len(engine) > 0
    else:
        checks["engine_present"] = bool(engine)

    # 3. Mood valid
    mood = data.get("mood", "")
    checks["mood_valid"] = mood in VALID_MOODS

    # 4. Description ≥10 chars
    desc = data.get("description", "")
    checks["description_length"] = isinstance(desc, str) and len(desc.strip()) >= 10

    # 5. Tags ≥1
    tags = data.get("tags", [])
    checks["tags_present"] = isinstance(tags, list) and len(tags) >= 1

    # 6. Sonic DNA — all 6 dimensions present and in [0.0, 1.0]
    dna = data.get("sonic_dna", {})
    if isinstance(dna, dict):
        dna_ok = SONIC_DNA_DIMENSIONS.issubset(dna.keys())
        if dna_ok:
            for dim in SONIC_DNA_DIMENSIONS:
                val = dna[dim]
                if not (isinstance(val, (int, float)) and 0.0 <= float(val) <= 1.0):
                    dna_ok = False
                    break
    else:
        dna_ok = False
    checks["sonic_dna_complete"] = dna_ok

    # 7. Macros has required keys
    macros = data.get("macros", {})
    if isinstance(macros, dict):
        present_keys = {k.upper() for k in macros.keys()}
        checks["macros_present"] = REQUIRED_MACROS.issubset(present_keys)
    else:
        checks["macros_present"] = False

    # 8. Macro descriptions ≥5 chars
    macro_desc_ok = True
    if isinstance(macros, dict) and checks["macros_present"]:
        for key in macros:
            if key.upper() in REQUIRED_MACROS:
                val = macros[key]
                # value can be a string description or a dict with "description"
                if isinstance(val, dict):
                    desc_text = val.get("description", "") or ""
                else:
                    desc_text = str(val) if val else ""
                if len(desc_text.strip()) < 5:
                    macro_desc_ok = False
                    break
    else:
        macro_desc_ok = False
    checks["macro_descriptions"] = macro_desc_ok

    # 9. Parameters dict non-empty
    params = data.get("parameters", {})
    checks["parameters_nonempty"] = isinstance(params, dict) and len(params) > 0

    # 10. Entangled mood → engines has ≥2 entries
    if mood == "Entangled":
        engines_val = data.get("engines", [])
        if isinstance(engines_val, list):
            checks["entangled_engines"] = len(engines_val) >= 2
        else:
            checks["entangled_engines"] = False
    else:
        checks["entangled_engines"] = True  # N/A for non-Entangled

    # 11. Name format (no snake_case, no ALL CAPS, Title Case)
    if name:
        checks["name_format"] = is_title_case_valid(name)
    else:
        checks["name_format"] = False

    # 12. Name uniqueness — register and check later (flag=True initially, resolved in second pass)
    # We use a tuple key of (mood_folder, name) for scoping
    mood_folder = path.parent.name
    registry_key = (mood_folder, name.strip().lower())
    if registry_key not in name_registry:
        name_registry[registry_key] = []
    name_registry[registry_key].append(path)
    # Placeholder: will be resolved after all files are processed
    checks["name_unique"] = None  # resolved post-scan

    score = sum(1 for v in checks.values() if v is True)
    # Don't count the None placeholder in score yet
    return {
        "path": path,
        "name": name,
        "mood": mood,
        "checks": checks,
        "score": score,  # will be updated after uniqueness resolution
    }


def resolve_uniqueness(results: list, name_registry: dict):
    """Set name_unique check and finalize scores."""
    for r in results:
        path = r["path"]
        name = r["name"]
        mood_folder = path.parent.name
        registry_key = (mood_folder, name.strip().lower())
        paths = name_registry.get(registry_key, [])
        is_unique = len(paths) <= 1
        r["checks"]["name_unique"] = is_unique
        # Recompute score
        r["score"] = sum(1 for v in r["checks"].values() if v is True)


# ── Fix names ────────────────────────────────────────────────────────────────

def fix_name_in_file(path: Path, dry_run: bool = False) -> bool:
    """
    If name fails title-case check, rewrite it in-place.
    Returns True if a fix was applied.
    """
    try:
        text = path.read_text(encoding="utf-8")
        data = json.loads(text)
    except Exception:
        return False

    name = data.get("name", "")
    if not name or is_title_case_valid(name):
        return False

    fixed = to_title_case(name)
    if fixed == name:
        return False

    data["name"] = fixed
    if not dry_run:
        path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n",
                        encoding="utf-8")
    return True


# ── Scan ─────────────────────────────────────────────────────────────────────

def scan_presets(presets_dir: Path) -> list:
    """Walk presets_dir and return sorted list of .xometa paths."""
    found = []
    for root, _dirs, files in os.walk(presets_dir):
        for fname in files:
            if fname.endswith(".xometa"):
                found.append(Path(root) / fname)
    found.sort()
    return found


# ── Report ────────────────────────────────────────────────────────────────────

def print_report(results: list, min_score: int, presets_dir: Path):
    total = len(results)
    if total == 0:
        print("No .xometa files found.")
        return

    passed = sum(1 for r in results if r["score"] >= 12)
    fleet_pct = 100.0 * passed / total

    # Failure frequencies per check
    fail_freq = defaultdict(int)
    for r in results:
        for check_name in CHECK_NAMES:
            if not r["checks"].get(check_name):
                fail_freq[check_name] += 1

    print("=" * 70)
    print("  XOmeta Completeness Audit")
    print(f"  Scanned: {presets_dir}")
    print("=" * 70)
    print(f"\n  Total files  : {total}")
    print(f"  Perfect (12/12): {passed}  ({fleet_pct:.1f}% fleet health)")

    # Files below threshold
    below = [r for r in results if r["score"] < min_score]
    if below:
        print(f"\n  Files below --min-score {min_score}: {len(below)}")

    # Worst 20
    worst = sorted(results, key=lambda r: r["score"])[:20]
    print(f"\n{'─' * 70}")
    print(f"  Worst 20 files (by score)")
    print(f"{'─' * 70}")
    print(f"  {'Score':>6}  {'Mood':<14}  {'Name':<32}  Path")
    for r in worst:
        rel = r["path"].relative_to(presets_dir) if presets_dir in r["path"].parents else r["path"]
        mood_str = (r["mood"] or "—")[:14]
        name_str = (r["name"] or "—")[:32]
        flag = " ⚠" if r["score"] < min_score else ""
        print(f"  {r['score']:>5}/12  {mood_str:<14}  {name_str:<32}  {rel}{flag}")

    # Check failure frequencies
    print(f"\n{'─' * 70}")
    print("  Check Failure Frequencies")
    print(f"{'─' * 70}")
    sorted_fails = sorted(fail_freq.items(), key=lambda x: -x[1])
    for check_name, count in sorted_fails:
        pct = 100.0 * count / total
        label = CHECK_LABELS.get(check_name, check_name)
        bar = "█" * int(pct / 5)
        print(f"  {pct:5.1f}% ({count:>4})  {label}")
        if bar:
            print(f"           {bar}")

    checks_with_no_failures = [n for n in CHECK_NAMES if fail_freq[n] == 0]
    if checks_with_no_failures:
        print(f"\n  All-pass checks: {', '.join(CHECK_LABELS[n] for n in checks_with_no_failures)}")

    print(f"\n{'─' * 70}")

    # Per-engine breakdown
    engine_scores = defaultdict(list)
    for r in results:
        engine_key = r["path"].parent.name
        engine_scores[engine_key].append(r["score"])

    print("  Per-Folder Health (avg score / 12)")
    print(f"{'─' * 70}")
    folder_avgs = []
    for folder, scores in sorted(engine_scores.items()):
        avg = sum(scores) / len(scores)
        folder_avgs.append((folder, avg, len(scores)))
    folder_avgs.sort(key=lambda x: x[1])
    for folder, avg, count in folder_avgs:
        bar = "█" * int(avg * 10 / 12)
        status = "✓" if avg >= 10 else ("△" if avg >= 8 else "✗")
        print(f"  {status} {folder:<28} {avg:5.2f}/12  (n={count:>4})  {bar}")

    print(f"\n{'═' * 70}")

    # Summary verdict
    any_below = any(r["score"] < min_score for r in results)
    if any_below:
        print(f"  RESULT: FAIL — {len(below)} file(s) score below {min_score}/12")
    else:
        print(f"  RESULT: PASS — all files meet --min-score {min_score}/12")
    print(f"{'═' * 70}\n")


# ── CLI ───────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(
        description="Deep completeness audit of .xometa preset files."
    )
    parser.add_argument(
        "--presets-dir",
        type=Path,
        default=Path("Presets"),
        help="Root directory to scan for .xometa files (default: ./Presets)",
    )
    parser.add_argument(
        "--min-score",
        type=int,
        default=8,
        metavar="N",
        help="Flag files scoring below N/12 (default: 8). Also sets exit code.",
    )
    parser.add_argument(
        "--fix-names",
        action="store_true",
        help="Auto-titlecase names that fail check #11 (writes files in-place).",
    )
    parser.add_argument(
        "--verbose",
        action="store_true",
        help="Print per-file check details for files below --min-score.",
    )
    args = parser.parse_args()

    presets_dir = args.presets_dir.resolve()
    if not presets_dir.exists():
        print(f"Error: --presets-dir '{presets_dir}' does not exist.", file=sys.stderr)
        sys.exit(2)

    xometa_paths = scan_presets(presets_dir)
    if not xometa_paths:
        print(f"No .xometa files found under '{presets_dir}'.")
        sys.exit(0)

    # Optionally fix names first
    fix_count = 0
    if args.fix_names:
        for p in xometa_paths:
            if fix_name_in_file(p):
                fix_count += 1
        if fix_count:
            print(f"  --fix-names: updated {fix_count} file(s).")

    # First pass: audit all files
    name_registry: dict = {}
    results = []
    parse_errors = []

    for p in xometa_paths:
        try:
            text = p.read_text(encoding="utf-8")
            data = json.loads(text)
        except json.JSONDecodeError as e:
            parse_errors.append((p, str(e)))
            # Treat as empty dict — will fail most checks
            data = {}
        except Exception as e:
            parse_errors.append((p, str(e)))
            data = {}

        result = audit_file(data, p, name_registry)
        results.append(result)

    # Second pass: resolve uniqueness
    resolve_uniqueness(results, name_registry)

    # Print parse errors
    if parse_errors:
        print(f"\n  ⚠ JSON parse errors ({len(parse_errors)} files):")
        for p, err in parse_errors:
            rel = p.relative_to(presets_dir) if presets_dir in p.parents else p
            print(f"    {rel}: {err}")

    # Verbose: print check details for failing files
    if args.verbose:
        failing = [r for r in results if r["score"] < args.min_score]
        if failing:
            print(f"\n  Verbose detail for {len(failing)} files below {args.min_score}/12:\n")
            for r in failing:
                rel = r["path"].relative_to(presets_dir) if presets_dir in r["path"].parents else r["path"]
                print(f"  {rel}  [{r['score']}/12]")
                for cn in CHECK_NAMES:
                    val = r["checks"].get(cn)
                    icon = "✓" if val else ("✗" if val is False else "–")
                    print(f"    {icon}  {CHECK_LABELS[cn]}")
                print()

    print_report(results, args.min_score, presets_dir)

    # Exit code
    any_below = any(r["score"] < args.min_score for r in results)
    sys.exit(1 if any_below else 0)


if __name__ == "__main__":
    main()
