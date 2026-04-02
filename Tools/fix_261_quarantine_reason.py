#!/usr/bin/env python3
"""
fix_261_quarantine_reason.py — Add default quarantine_reason to quarantined presets.

Closes issue #261.

3,991 presets in Presets/XOceanus/_quarantine/ lack a ``quarantine_reason``
field, making triage impossible without inspecting each file individually.
This script adds ``"quarantine_reason": "legacy_uncategorized"`` to any
quarantined preset that does not already have the field.

Usage:
    python3 Tools/fix_261_quarantine_reason.py --dry-run
    python3 Tools/fix_261_quarantine_reason.py
"""

import json
import sys
from pathlib import Path

QUARANTINE_ROOT = (
    Path(__file__).parent.parent / "Presets" / "XOceanus" / "_quarantine"
)
DEFAULT_REASON = "legacy_uncategorized"


def fix_preset(data: dict) -> tuple[dict, bool]:
    """Return (fixed_data, was_changed).

    Inserts ``quarantine_reason`` after the ``name`` field (or at the end of
    the top-level object) when it is absent.
    """
    if "quarantine_reason" in data:
        return data, False

    fixed = dict(data)
    fixed["quarantine_reason"] = DEFAULT_REASON
    return fixed, True


def process_all(dry_run: bool = False) -> dict:
    """Walk all quarantined presets and apply fixes.  Returns counters dict."""
    counters: dict = {"fixed": 0, "already_tagged": 0, "errors": 0}

    files = sorted(QUARANTINE_ROOT.rglob("*.xometa"))
    for filepath in files:
        try:
            raw = filepath.read_text(encoding="utf-8")
            data = json.loads(raw)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  ERROR reading {filepath.name}: {exc}")
            counters["errors"] += 1
            continue

        fixed, changed = fix_preset(data)
        if not changed:
            counters["already_tagged"] += 1
            continue

        counters["fixed"] += 1
        if dry_run:
            print(
                f"  WOULD TAG: "
                f"{filepath.relative_to(QUARANTINE_ROOT.parent.parent.parent)}"
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

    return counters


def main() -> None:
    dry_run = "--dry-run" in sys.argv
    mode = "DRY RUN" if dry_run else "FIXING"
    print(f"=== fix_261_quarantine_reason.py — {mode} ===")
    print(f"Scanning: {QUARANTINE_ROOT}")
    print()

    if not QUARANTINE_ROOT.exists():
        print(f"ERROR: quarantine directory not found: {QUARANTINE_ROOT}")
        sys.exit(1)

    counters = process_all(dry_run)

    print()
    print("Results:")
    print(f"  Presets tagged (added quarantine_reason): {counters['fixed']}")
    print(f"  Already had quarantine_reason            : {counters['already_tagged']}")
    print(f"  Errors                                   : {counters['errors']}")

    if dry_run and counters["fixed"] > 0:
        print()
        print("Run without --dry-run to apply fixes.")


if __name__ == "__main__":
    main()
