#!/usr/bin/env python3
"""
validate_docs.py — XOlokun documentation consistency checker.

Reads canonical product stats from Tools/project_stats.json and scans
public-facing docs for stale values. Exits non-zero if inconsistencies
are found, making it suitable for CI enforcement.

Usage:
    python3 Tools/validate_docs.py           # concise output
    python3 Tools/validate_docs.py --verbose # show each checked file

The intent is that project_stats.json is the ONE place you update a
number, then run this script to discover which docs still need updating.
"""

import argparse
import json
import re
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Stale patterns — old preset-count values that should no longer appear
# in public-facing docs. Historical/archived documents are excluded below.
# ---------------------------------------------------------------------------
STALE_PRESET_PATTERNS = [
    (re.compile(r'19,000\+?'), "19,000+"),
    (re.compile(r'~19,000'),   "~19,000"),
    (re.compile(r'22,000\+?'), "22,000+"),
    (re.compile(r'~22,000'),   "~22,000"),
]

# ---------------------------------------------------------------------------
# Public-facing docs to validate.
# Historical files (seances, retreats, audit reports, old drafts) are
# intentionally excluded — they are records of a moment in time and are
# never updated retroactively.
# ---------------------------------------------------------------------------
CHECKED_DOCS = [
    "README.md",
    "CONTRIBUTING.md",
    "CLAUDE.md",
    "SDK/README.md",
    "site/index.html",
    "site/field-guide/the-coupling-explainer.html",
    "site/press-kit/index.html",
    "Docs/design/xolokun-definitive-ui-spec.md",
    "Docs/design/xolokun-spatial-architecture.md",
    "Docs/design/xolokun-ui-blessing-session.md",
    "Docs/design/xomnibus_design_guidelines.md",
    "Docs/design/xomnibus_ui_master_spec_v2.md",
]


def load_stats(repo_root: Path) -> dict:
    stats_path = repo_root / "Tools" / "project_stats.json"
    with stats_path.open() as f:
        return json.load(f)


def check_file(path: Path) -> list[tuple[int, str, str]]:
    """Return list of (line_no, matched_text, description) for stale values."""
    if not path.exists():
        return []
    hits = []
    with path.open(errors="replace") as fh:
        for lineno, line in enumerate(fh, 1):
            for pattern, description in STALE_PRESET_PATTERNS:
                if pattern.search(line):
                    hits.append((lineno, line.strip(), description))
                    break  # one hit per line is enough
    return hits


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate doc consistency against project_stats.json")
    parser.add_argument("--verbose", "-v", action="store_true",
                        help="Show each checked file even if clean")
    args = parser.parse_args()

    # Locate repo root (two levels up from this script)
    repo_root = Path(__file__).resolve().parent.parent
    stats = load_stats(repo_root)

    canonical = stats["preset_count_display"]
    print(f"Canonical preset count : {canonical}")
    print(f"Checking {len(CHECKED_DOCS)} public-facing docs for stale values\n")

    errors: list[tuple[str, int, str, str]] = []
    warnings: list[str] = []

    for rel in CHECKED_DOCS:
        path = repo_root / rel
        hits = check_file(path)
        if not path.exists():
            warnings.append(f"  SKIP  {rel}  (file not found)")
            continue
        if hits:
            for lineno, text, desc in hits:
                errors.append((rel, lineno, text, desc))
            print(f"  FAIL  {rel}")
            for lineno, text, desc in hits:
                print(f"        line {lineno}: stale value '{desc}' — {text[:80]}")
        elif args.verbose:
            print(f"  OK    {rel}")

    if warnings:
        print()
        for w in warnings:
            print(w)

    print()
    if errors:
        print(f"RESULT: {len(errors)} stale reference(s) found in {len(set(e[0] for e in errors))} file(s).")
        print(f"  Update those files to use the canonical value: {canonical}")
        print(f"  Then re-run: python3 Tools/validate_docs.py --verbose")
        return 1

    print(f"RESULT: All {len(CHECKED_DOCS)} checked docs are consistent with project_stats.json.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
