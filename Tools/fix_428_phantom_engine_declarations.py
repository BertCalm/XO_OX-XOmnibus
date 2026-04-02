#!/usr/bin/env python3
"""
fix_428_phantom_engine_declarations.py — Remove phantom engine declarations.

Closes issue #428.

319 presets list an engine in the "engines" array but provide no parameters
for that engine in the "parameters" object.  This script removes the
unreferenced engine from the "engines" array, keeping only engines that
actually have parameter keys.

Usage:
    python3 Tools/fix_428_phantom_engine_declarations.py --dry-run
    python3 Tools/fix_428_phantom_engine_declarations.py
"""

import json
import sys
from pathlib import Path

PRESETS_ROOT = Path(__file__).parent.parent / "Presets" / "XOlokun"


def fix_preset(data: dict) -> tuple[dict, list[str]]:
    """Return (fixed_data, removed_engines).

    Removes any engine from the ``"engines"`` array that has no corresponding
    key in the ``"parameters"`` object.  Returns the original data unchanged
    (same object) when no fix is needed.
    """
    engines = data.get("engines")
    parameters = data.get("parameters")

    if not isinstance(engines, list) or len(engines) < 2:
        # Nothing to trim: single-engine presets are fine with empty params,
        # and presets without an engines array are a different schema issue.
        return data, []

    if not isinstance(parameters, dict):
        # No parameters block at all — different schema issue (#339), skip.
        return data, []

    removed = [eng for eng in engines if eng not in parameters]
    if not removed:
        return data, []

    fixed = dict(data)
    fixed["engines"] = [eng for eng in engines if eng in parameters]
    return fixed, removed


def process_all(dry_run: bool = False) -> dict:
    """Walk all presets and apply fixes.  Returns counters dict."""
    counters: dict = {
        "fixed": 0,
        "skipped": 0,
        "errors": 0,
        "engines_removed": 0,
    }

    files = sorted(PRESETS_ROOT.rglob("*.xometa"))
    for filepath in files:
        # Skip quarantine — those are known-bad and handled separately.
        if "_quarantine" in str(filepath):
            counters["skipped"] += 1
            continue

        try:
            raw = filepath.read_text(encoding="utf-8")
            data = json.loads(raw)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  ERROR reading {filepath.name}: {exc}")
            counters["errors"] += 1
            continue

        fixed, removed = fix_preset(data)
        if not removed:
            counters["skipped"] += 1
            continue

        counters["fixed"] += 1
        counters["engines_removed"] += len(removed)

        if dry_run:
            print(
                f"  WOULD REMOVE {removed} from engines in: "
                f"{filepath.relative_to(PRESETS_ROOT.parent.parent)}"
            )
        else:
            try:
                filepath.write_text(
                    json.dumps(fixed, indent=2, ensure_ascii=False) + "\n",
                    encoding="utf-8",
                )
            except OSError as exc:
                print(f"  ERROR writing {filepath.name}: {exc}")
                counters["errors"] += 1
                counters["fixed"] -= 1
                counters["engines_removed"] -= len(removed)

    return counters


def main() -> None:
    dry_run = "--dry-run" in sys.argv
    mode = "DRY RUN" if dry_run else "FIXING"
    print(f"=== fix_428_phantom_engine_declarations.py — {mode} ===")
    print(f"Scanning: {PRESETS_ROOT}")
    print()

    counters = process_all(dry_run)

    print()
    print("Results:")
    print(f"  Presets fixed          : {counters['fixed']}")
    print(f"  Engine entries removed : {counters['engines_removed']}")
    print(f"  Skipped (no fix needed): {counters['skipped']}")
    print(f"  Errors                 : {counters['errors']}")

    if dry_run and counters["fixed"] > 0:
        print()
        print("Run without --dry-run to apply fixes.")


if __name__ == "__main__":
    main()
