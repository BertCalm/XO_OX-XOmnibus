#!/usr/bin/env python3
"""
XOceanus Preset Smoke Test (#57)

Fast, CI-friendly sanity check: load every .xometa, verify required schema
fields are present and well-formed, and report any files that fail.

Exit code 0 = all presets pass.
Exit code 1 = one or more presets failed.

Usage:
    python3 test_preset_smoke.py [--presets-dir PATH]

Checks per preset:
  1. File is valid JSON
  2. "name" field is a non-empty string
  3. "engines" field is a non-empty list
  4. "parameters" field is a dict
  5. "mood" field is a non-empty string in the canonical mood set
  6. "macros" field is a list with exactly 4 entries
  7. "sonicDNA" field is a dict with all 6 dimensions in [0, 1]
  8. No NaN or Inf values anywhere in "parameters"
"""

import argparse
import glob
import json
import math
import os
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_PRESETS_DIR = os.path.join(REPO_ROOT, "Presets")

VALID_MOODS = {
    "Foundation", "Atmosphere", "Entangled", "Prism", "Flux",
    "Aether", "Family", "Submerged", "Coupling", "Crystalline",
    "Deep", "Ethereal", "Kinetic", "Luminous", "Organic",
    "Shadow",
}

DNA_DIMENSIONS = {"brightness", "warmth", "movement", "density", "space", "aggression"}

# Directories that are intentionally incomplete / work-in-progress and excluded from CI.
EXCLUDED_DIRS = {"Quarantine", "Tutorials"}


def is_excluded(path: str) -> bool:
    parts = path.replace("\\", "/").split("/")
    return any(part in EXCLUDED_DIRS for part in parts)


def check_preset(path: str) -> list[str]:
    """Return a list of error strings. Empty list means the preset passes."""
    errors = []

    # 1. Valid JSON
    try:
        with open(path, encoding="utf-8") as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        return [f"Invalid JSON: {e}"]
    except OSError as e:
        return [f"Cannot read file: {e}"]

    # 2. "name" — non-empty string
    name = data.get("name")
    if not isinstance(name, str) or not name.strip():
        errors.append('"name" is missing or empty')

    # 3. "engines" — non-empty list
    engines = data.get("engines")
    if not isinstance(engines, list) or len(engines) == 0:
        errors.append('"engines" is missing or empty')

    # 4. "parameters" — dict
    parameters = data.get("parameters")
    if not isinstance(parameters, dict):
        errors.append('"parameters" is missing or not a dict')
    else:
        # 8. No NaN / Inf in parameter values
        for key, val in parameters.items():
            if isinstance(val, float) and (math.isnan(val) or math.isinf(val)):
                errors.append(f'"parameters.{key}" is NaN or Inf')

    # 5. "mood" — in canonical set
    mood = data.get("mood")
    if not isinstance(mood, str) or mood not in VALID_MOODS:
        errors.append(f'"mood" is invalid or missing: {mood!r}')

    # 6. "macroLabels" — exactly 4 entries (field name in actual .xometa schema)
    macros = data.get("macroLabels")
    if not isinstance(macros, list):
        errors.append('"macroLabels" is missing or not a list')
    elif len(macros) != 4:
        errors.append(f'"macroLabels" has {len(macros)} entries (expected 4)')

    # 7. "dna" — dict with all 6 dimensions in [0, 1] (field name in actual schema)
    dna = data.get("dna")
    if not isinstance(dna, dict):
        errors.append('"dna" is missing or not a dict')
    else:
        missing = DNA_DIMENSIONS - set(dna.keys())
        if missing:
            errors.append(f'"dna" missing dimensions: {sorted(missing)}')
        for dim in DNA_DIMENSIONS:
            val = dna.get(dim)
            if val is not None:
                if not isinstance(val, (int, float)):
                    errors.append(f'"dna.{dim}" is not a number: {val!r}')
                elif not (0.0 <= float(val) <= 1.0):
                    errors.append(f'"dna.{dim}" out of [0,1]: {val}')

    return errors


def main():
    parser = argparse.ArgumentParser(description="XOceanus preset smoke test")
    parser.add_argument(
        "--presets-dir",
        default=DEFAULT_PRESETS_DIR,
        help="Root directory to search for .xometa files",
    )
    args = parser.parse_args()

    pattern = os.path.join(args.presets_dir, "**", "*.xometa")
    paths = sorted(glob.glob(pattern, recursive=True))

    if not paths:
        print(f"ERROR: No .xometa files found under {args.presets_dir}", file=sys.stderr)
        sys.exit(1)

    failures: list[tuple[str, list[str]]] = []
    skipped = 0
    for path in paths:
        if is_excluded(path):
            skipped += 1
            continue
        errors = check_preset(path)
        if errors:
            failures.append((path, errors))

    checked = len(paths) - skipped
    passed = checked - len(failures)

    print(f"Preset smoke test: {passed}/{checked} passed  (skipped {skipped} Quarantine/Tutorials)")

    if failures:
        print(f"\nFAILURES ({len(failures)}):")
        for path, errors in failures:
            rel = os.path.relpath(path, REPO_ROOT)
            print(f"\n  {rel}")
            for err in errors:
                print(f"    - {err}")
        sys.exit(1)
    else:
        print("All presets passed smoke test.")
        sys.exit(0)


if __name__ == "__main__":
    main()
